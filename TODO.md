# Folded-Library Roadmap

Goal: reduce the Waterfox runtime closure for ~/d/kominka/xsh/laputa/WATERFOX.md without making the first
iteration harder to debug in QEMU. Keep the active experiment on the debug
minwayland profile until the closure change is proven.

## Phase 1: Static GCC Runtime

Scope:
- Use the debug minwayland build as the base profile.
- Add only `-static-libstdc++ -static-libgcc`.
- Do not enable `MOZ_FOLD_LIBS`.
- Do not change font, Wayland, xkbcommon, NSS, NSPR, SQLite, codec, or GTK
  policy in this phase.

Expected result:
- `libstdc++.so.6` disappears from staged Waterfox `DT_NEEDED`.
- `libgcc_s.so.1` disappears from staged Waterfox `DT_NEEDED`.
- The QEMU rootfs may still install Alpine `libstdc++` and `libgcc` for the
  compositor stack, but Waterfox itself should no longer need them.

Commands:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl configure-folded
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl build-folded
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl package-folded
```

QEMU proof from the folded staged root:

```sh
WFX_STAGE_ROOT=.wfx-cache/dist/folded-root \
  WFX_JOBS=8 WFX_CARGO_JOBS=8 \
  docker/waterfox-musl/wfx-musl qemu-image-minwayland

WFX_STAGE_ROOT=.wfx-cache/dist/folded-root \
  WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  docker/waterfox-musl/wfx-musl qemu-run
```

Automated repros:

```sh
env WFX_STAGE_ROOT=.wfx-cache/dist/folded-root \
  WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=folded-hamburger WFX_REPRO_BOOT_WAIT=35 \
  WFX_REPRO_AFTER_CLICK_WAIT=4 \
  docker/waterfox-musl/qemu-repro-hamburger

env WFX_STAGE_ROOT=.wfx-cache/dist/folded-root \
  WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=folded-urlbar WFX_REPRO_BOOT_WAIT=35 \
  WFX_REPRO_AFTER_CLICK_WAIT=2 \
  WFX_REPRO_CLICK_X=330 WFX_REPRO_CLICK_Y=49 WFX_REPRO_TEXT=abc.com \
  docker/waterfox-musl/qemu-repro-hamburger
```

Pass criteria:
- `configure-folded` passes with `MOZ_WIDGET_TOOLKIT=minwayland`.
- `package-folded` passes the static dependency scan using the static-runtime
  allowlist.
- `.wfx-cache/dist/folded-root` contains no Waterfox `DT_NEEDED` entry for
  `libstdc++.so.6`, `libgcc_s.so.1`, or `libmimalloc.so`.
- Headless WebDriver BiDi smoke still passes if run against the folded staged
  root.
- QEMU visibly renders Waterfox.
- Hamburger menu and URL bar repro screenshots are nonblack and usable.
- Normal website loading works at least as well as the current minwayland debug
  profile.

Risk checks:
- Startup and content-process launch, because static libgcc changes unwind and
  compiler-helper resolution.
- Backtrace-ish paths, because Gecko contains stack walking code that observes
  libgcc/libstdc++ behavior.
- Shared-object links, because Alpine's `libstdc++.a` and `libgcc.a` must be
  usable in this build's link model.

Rollback:
- Remove the folded debug mozconfig and wrapper commands, or drop only the
  `-static-libstdc++ -static-libgcc` `LDFLAGS`.
- Keep the normal `minwayland` debug profile untouched as the baseline.

## Phase 2: Package Policy Cleanup

Do this only after Phase 1 passes QEMU.

Tasks:
- Update `~/d/kominka/xsh/laputa/WATERFOX.md` so `gcc-runtime` is conditional or removed
  for the folded debug artifact.
- Keep QEMU rootfs package cleanup separate from Waterfox artifact cleanup,
  because the compositor may still need Alpine GCC runtime packages.
- Record the folded artifact path and dependency report in the manifest.

Validation:
- Compare folded and non-folded `*.needed.txt` reports.
- Confirm WaterfoxBlocker remains absent.
- Confirm no rejected dependency families are introduced.

## Phase 3: Static Font Stack

Candidates:
- `libfontconfig.so.1`
- `libfreetype.so.6`

Only attempt this after Phase 1 is stable. These libraries are plausible
because they are direct Waterfox dependencies, but static-linking them does not
remove the need for font data, fontconfig config, and a cache strategy.

Required design work:
- Decide whether the sysroot builds static and shared variants or static-only
  variants for the experiment.
- Preserve explicit runtime handling for fonts and fontconfig configuration.
- Keep package scans strict so static linking does not hide an accidental GTK,
  GLib, X11, DBus, or Vulkan path.

## Phase 4: Static Wayland Client Pieces

Candidates:
- `libwayland-client.so.0`
- `libxkbcommon.so.0`

This mostly simplifies the Waterfox package closure. It probably does not
simplify the full Laputa image if the compositor stack already carries Wayland
and xkbcommon dynamically.

Validation:
- Pointer input, keyboard input, popups, menus, and clipboard repros.
- No change to the compositor runtime dependency policy.

## Deferred

Do not chase these as part of the folded-library tranche:
- Fully static musl Waterfox.
- ALSA or other audio stack folding.
- Mesa, GBM, EGL, libdrm, libudev, or VAAPI folding.
- NSS, NSPR, SQLite, or bundled codec reshaping.
- The old optimized folded release profile until the debug folded path is
  proven in QEMU.

Later media work:
- libva / Intel VAAPI investigation.
