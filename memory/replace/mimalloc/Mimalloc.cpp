/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "mimalloc.h"
#include "replace_malloc.h"

static const arena_id_t kMaxArenas = 4096;
static pthread_mutex_t sArenaLock = PTHREAD_MUTEX_INITIALIZER;
static mi_heap_t* sArenas[kMaxArenas];
static arena_id_t sNextArena = 1;

static mi_heap_t* HeapForArena(arena_id_t aArena) {
  if (aArena == 0 || aArena >= kMaxArenas) {
    return nullptr;
  }
  return sArenas[aArena];
}

static void* replace_malloc(size_t aSize) { return mi_malloc(aSize); }

static void* replace_calloc(size_t aNum, size_t aSize) {
  return mi_calloc(aNum, aSize);
}

static void* replace_realloc(void* aPtr, size_t aSize) {
  return mi_realloc(aPtr, aSize);
}

static void replace_free(void* aPtr) { mi_free(aPtr); }

static void* replace_memalign(size_t aAlignment, size_t aSize) {
  return mi_malloc_aligned(aSize, aAlignment);
}

static int replace_posix_memalign(void** aPtr, size_t aAlignment,
                                  size_t aSize) {
  return mi_posix_memalign(aPtr, aAlignment, aSize);
}

static void* replace_aligned_alloc(size_t aAlignment, size_t aSize) {
  return mi_aligned_alloc(aAlignment, aSize);
}

static void* replace_valloc(size_t aSize) {
  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize <= 0) {
    pageSize = 4096;
  }
  return mi_malloc_aligned(aSize, size_t(pageSize));
}

static size_t replace_malloc_usable_size(usable_ptr_t aPtr) {
  return mi_usable_size(aPtr);
}

static size_t replace_malloc_good_size(size_t aSize) {
  return mi_good_size(aSize);
}

static void replace_jemalloc_stats_internal(jemalloc_stats_t* aStats,
                                            jemalloc_bin_stats_t* aBinStats) {
  if (aStats) {
    memset(aStats, 0, sizeof(*aStats));
    long pageSize = sysconf(_SC_PAGESIZE);
    aStats->page_size = pageSize > 0 ? size_t(pageSize) : 4096;
    aStats->narenas = sNextArena;
    aStats->quantum = sizeof(void*);
  }
  if (aBinStats) {
    memset(aBinStats, 0, sizeof(*aBinStats));
  }
}

static size_t replace_jemalloc_stats_num_bins() { return 0; }

static void replace_jemalloc_stats_lite(jemalloc_stats_lite_t* aStats) {
  if (aStats) {
    memset(aStats, 0, sizeof(*aStats));
  }
}

static void replace_jemalloc_set_main_thread() {}

static void replace_jemalloc_purge_freed_pages() { mi_collect(true); }

static void replace_jemalloc_free_dirty_pages() { mi_collect(true); }

static void replace_moz_set_max_dirty_page_modifier(int32_t) {}

static bool replace_moz_enable_deferred_purge(bool) { return false; }

static purge_result_t replace_moz_may_purge_now(
    bool, uint32_t, const mozilla::Maybe<std::function<bool()>>&) {
  mi_collect(false);
  return Done;
}

static void replace_jemalloc_free_excess_dirty_pages() { mi_collect(true); }

static void replace_jemalloc_reset_small_alloc_randomization(bool) {}

static void replace_jemalloc_thread_local_arena(bool) {}

static void replace_jemalloc_ptr_info(const void* aPtr,
                                      jemalloc_ptr_info_t* aInfo) {
  if (!aInfo) {
    return;
  }

  if (!aPtr || !mi_is_in_heap_region(aPtr)) {
    *aInfo = jemalloc_ptr_info_t(TagUnknown, nullptr, 0, 0);
    return;
  }

  size_t size = mi_usable_size(aPtr);
  if (size == 0) {
    *aInfo = jemalloc_ptr_info_t(TagUnknown, nullptr, 0, 0);
    return;
  }
  *aInfo = jemalloc_ptr_info_t(TagLiveAlloc, const_cast<void*>(aPtr), size, 0);
}

static arena_id_t replace_moz_create_arena_with_params(arena_params_t*) {
  mi_heap_t* heap = mi_heap_new();
  if (!heap) {
    return 0;
  }

  pthread_mutex_lock(&sArenaLock);
  arena_id_t arena = sNextArena;
  if (arena < kMaxArenas) {
    sArenas[arena] = heap;
    sNextArena++;
  } else {
    arena = 0;
  }
  pthread_mutex_unlock(&sArenaLock);

  if (arena == 0) {
    mi_heap_delete(heap);
  }
  return arena;
}

static void replace_moz_dispose_arena(arena_id_t aArena) {
  mi_heap_t* heap = nullptr;

  pthread_mutex_lock(&sArenaLock);
  if (aArena > 0 && aArena < kMaxArenas) {
    heap = sArenas[aArena];
    sArenas[aArena] = nullptr;
  }
  pthread_mutex_unlock(&sArenaLock);

  if (heap) {
    mi_heap_delete(heap);
  }
}

static void* replace_moz_arena_malloc(arena_id_t aArena, size_t aSize) {
  mi_heap_t* heap = HeapForArena(aArena);
  return heap ? mi_heap_malloc(heap, aSize) : mi_malloc(aSize);
}

static void* replace_moz_arena_calloc(arena_id_t aArena, size_t aNum,
                                      size_t aSize) {
  mi_heap_t* heap = HeapForArena(aArena);
  return heap ? mi_heap_calloc(heap, aNum, aSize) : mi_calloc(aNum, aSize);
}

static void* replace_moz_arena_realloc(arena_id_t aArena, void* aPtr,
                                       size_t aSize) {
  mi_heap_t* heap = HeapForArena(aArena);
  return heap ? mi_heap_realloc(heap, aPtr, aSize) : mi_realloc(aPtr, aSize);
}

static void replace_moz_arena_free(arena_id_t, void* aPtr) { mi_free(aPtr); }

static void* replace_moz_arena_memalign(arena_id_t aArena, size_t aAlignment,
                                        size_t aSize) {
  mi_heap_t* heap = HeapForArena(aArena);
  return heap ? mi_heap_malloc_aligned(heap, aSize, aAlignment)
              : mi_malloc_aligned(aSize, aAlignment);
}

void replace_init(malloc_table_t* aTable, ReplaceMallocBridge**) {
  mi_process_init();
  mi_option_disable(mi_option_show_errors);
  mi_option_disable(mi_option_show_stats);
  mi_option_disable(mi_option_verbose);

  aTable->malloc = replace_malloc;
  aTable->calloc = replace_calloc;
  aTable->realloc = replace_realloc;
  aTable->free = replace_free;
  aTable->memalign = replace_memalign;
  aTable->posix_memalign = replace_posix_memalign;
  aTable->aligned_alloc = replace_aligned_alloc;
  aTable->valloc = replace_valloc;
  aTable->malloc_usable_size = replace_malloc_usable_size;
  aTable->malloc_good_size = replace_malloc_good_size;
  aTable->jemalloc_stats_internal = replace_jemalloc_stats_internal;
  aTable->jemalloc_stats_num_bins = replace_jemalloc_stats_num_bins;
  aTable->jemalloc_stats_lite = replace_jemalloc_stats_lite;
  aTable->jemalloc_set_main_thread = replace_jemalloc_set_main_thread;
  aTable->jemalloc_purge_freed_pages = replace_jemalloc_purge_freed_pages;
  aTable->jemalloc_free_dirty_pages = replace_jemalloc_free_dirty_pages;
  aTable->moz_set_max_dirty_page_modifier =
      replace_moz_set_max_dirty_page_modifier;
  aTable->moz_enable_deferred_purge = replace_moz_enable_deferred_purge;
  aTable->moz_may_purge_now = replace_moz_may_purge_now;
  aTable->jemalloc_free_excess_dirty_pages =
      replace_jemalloc_free_excess_dirty_pages;
  aTable->jemalloc_reset_small_alloc_randomization =
      replace_jemalloc_reset_small_alloc_randomization;
  aTable->jemalloc_thread_local_arena = replace_jemalloc_thread_local_arena;
  aTable->jemalloc_ptr_info = replace_jemalloc_ptr_info;
  aTable->moz_create_arena_with_params = replace_moz_create_arena_with_params;
  aTable->moz_dispose_arena = replace_moz_dispose_arena;
  aTable->moz_arena_malloc = replace_moz_arena_malloc;
  aTable->moz_arena_calloc = replace_moz_arena_calloc;
  aTable->moz_arena_realloc = replace_moz_arena_realloc;
  aTable->moz_arena_free = replace_moz_arena_free;
  aTable->moz_arena_memalign = replace_moz_arena_memalign;
}
