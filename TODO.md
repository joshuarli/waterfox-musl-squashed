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

Status: implemented for the folded debug path.

Tasks:
- Treat `.wfx-cache/dist/folded-root` as a first-class staged root through
  `package-folded`, `smoke-webdriver-bidi-folded`, and `qemu-image-folded`.
- Keep QEMU rootfs cleanup separate from Waterfox artifact cleanup. The
  compositor may still need Alpine GCC runtime packages even when Waterfox does
  not.
- Record the folded artifact path, dependency report, and mozconfig in the
  manifest through `package-stage1`.

Validation:
- Compare folded and non-folded `*.needed.txt` reports.
- Confirm WaterfoxBlocker remains absent.
- Confirm no rejected dependency families are introduced.

## Phase 3: Static Font Stack

Status: active implementation target.

Scope:
- Build a static-only font stack for the folded experiment under
  `/opt/wfx/build-deps/fontstack`.
- Static archives: zlib, expat, freetype, and fontconfig.
- Prefix the private static expat symbols used by fontconfig so they do not
  collide with Gecko's built-in expat.
- Use pkg-config `--static` only for fontconfig/freetype checks. Do not apply
  static pkg-config globally, because Wayland and xkbcommon remain phase 4.
- Keep runtime font files and fontconfig configuration in the QEMU image.

Expected result:
- `libfontconfig.so.1` disappears from folded staged Waterfox `DT_NEEDED`.
- `libfreetype.so.6` disappears from folded staged Waterfox `DT_NEEDED`.
- Package scans continue to reject GTK, GLib, X11, DBus, Vulkan, ALSA, and
  dynamic mimalloc.

Validation:
- `configure-folded` passes and records the fontstack library path in
  `FT2_LIBS`.
- `package-folded` passes using `waterfox-minwayland-folded-allowed-needed.txt`.
- `smoke-webdriver-bidi-folded` passes.
- `qemu-image-folded` rebuilds from `.wfx-cache/dist/folded-root`.

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
