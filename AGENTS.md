# Agent Handoff: Waterfox Musl Build

This repository is a Waterfox checkout adapted to produce arm64 Linux musl
artifacts for Laputa. The current primary proof is the `minwayland` browser
profile: Waterfox runs on a small Wayland-only widget backend under the custom
seatd + cage/wlroots QEMU kiosk environment, without GTK/GDK/GLib/GIO/Pango in
the staged Waterfox runtime closure.

Alpine's Firefox package and patches are available at
`/Users/josh/d/kominka/aports/community/firefox/` and remain the first reference
when adapting musl or packaging fixes.

## Hard Requirements

- Build and package for Linux arm64 musl, not macOS.
- Use Alpine edge for build-time dependencies unless a pinned source build is
  intentionally used.
- Use clang/LLVM and mold for compiling and linking Waterfox and source-built
  runtime libraries. Do not select GCC/G++ as the compiler/linker.
- Alpine may install GCC runtime packages such as `libgcc` or `libstdc++` as
  dormant transitive runtime dependencies.
- The active stage 1 browser direction is `cairo-minwayland`, not GTK.
- Do not ship X11, XCB, Xwayland, GLX, Vulkan, DBus, portals, notification,
  secret-service, speechd, cups, or dynamic `libmimalloc.so` in the stage 1
  browser runtime.
- Do not ship GTK/GDK/GLib/GIO/Pango/ATK in the minwayland Waterfox runtime.
- Minwayland remains no-audio by default. ALSA debug commands exist as an
  explicit diagnostic profile; do not merge audio into the stage 1 artifact
  unless the task explicitly targets that profile.
- Mimalloc v3 is linked through the browser allocator path statically. The final
  Waterfox artifact must not need `libmimalloc.so`.
- Debug/dev builds are the default iteration path. Release builds use separate
  release mozconfigs and objdirs only when explicitly requested.
- Do not run pre-commit hooks. Do not push.

## Status

- Docker toolchain image, wrapper, cache layout, and minwayland mozconfig are in
  place.
- `docker/waterfox-musl/mozconfig.minwayland` configures
  `--enable-default-toolkit=cairo-minwayland`, producing
  `MOZ_WIDGET_TOOLKIT=minwayland`.
- `configure-minwayland`, `build-minwayland`, and `package-minwayland` pass for
  the debug/dev path.
- The minwayland staged artifact scans cleanly against
  `docker/waterfox-musl/waterfox-minwayland-allowed-needed.txt`.
- QEMU boots to seatd + custom cage/wlroots + minwayland Waterfox. Cocoa QEMU
  visibly renders Waterfox, keyboard and mouse input work, QEMU user networking
  works, and repeat boots no longer hit profile read-only panics.
- Hamburger menu, context menu, URL bar typing, and the old black-frame popup
  regression are covered by `docker/waterfox-musl/qemu-repro-hamburger`.
- Wayland text clipboard bridging is implemented in `widget/minwayland` via
  `wl_data_device_manager` and was verified in QEMU with guest `wl-copy` and
  `wl-paste`.
- WaterfoxBlocker is excluded from the musl build with
  `--disable-waterfox-blocker`; staged artifact scans find no blocker files or
  registration strings.
- A minimal browser-owned file picker remains the next major minwayland widget
  gap. Drag and drop, print dialogs, GTK/portal/native dialogs, and
  accessibility are explicitly out of scope for this tranche.
- Legacy GTK stage1/debug and release profiles still exist in the wrapper for
  comparison and historical packaging work, but they are not the primary path
  for GTK removal.
- The experimental folded-library release profile builds and packages, but it
  is not a usable release path yet: QEMU boots and browser chrome works, while
  loading normal websites fails with the in-content "Try Again" network error
  page.

## Main Commands

Run commands from the repository root.

Build or refresh the toolchain image:

```sh
docker/waterfox-musl/wfx-musl image
```

Report current Alpine edge versions:

```sh
docker/waterfox-musl/wfx-musl versions
```

Configure minwayland:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl configure-minwayland
```

Build minwayland Waterfox:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl build-minwayland
```

Package minwayland Waterfox and run static dependency checks:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl package-minwayland
```

Build the QEMU image from the minwayland staged root:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl qemu-image-minwayland
```

Run visible QEMU proof:

```sh
WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 docker/waterfox-musl/wfx-musl qemu-run
```

Serial-only QEMU proof:

```sh
timeout 90s env WFX_QEMU_DISPLAY=none WFX_QEMU_ACCEL=hvf docker/waterfox-musl/wfx-musl qemu-run
```

Legacy GTK stage1 path, only when explicitly requested:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl configure
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl build
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl package
```

Release path, only when explicitly requested:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl configure-release
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl build-release
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl package-release
```

Do not build release binaries unless the user explicitly asks for release work.

## Development Loop

Prefer the narrowest loop that exercises the code changed.

1. Minwayland Gecko source/config changes: run `configure-minwayland` if build
   flags changed, then `build-minwayland`, then `package-minwayland`.
2. QEMU rootfs/init changes: run `qemu-image-minwayland`, then a bounded
   serial-only `qemu-run`. Use Cocoa once serial shows seatd, wlroots, and
   Waterfox surfaces.
3. Widget/input/popup/clipboard changes: run the relevant
   `qemu-repro-hamburger` scenario after `package-minwayland` and
   `qemu-image-minwayland`.
4. Runtime sysroot changes for the legacy GTK path: run `sysroot`,
   `sysroot-smoke`, and `check` before rebuilding Waterfox unless the change
   only affects QEMU packaging.
5. Keep `WFX_JOBS=8 WFX_CARGO_JOBS=8` while OrbStack has enough memory. If
   memory pressure returns, reduce jobs before changing code.

Expected warm-cache timings:

- Tiny incremental C++ build: seconds to minutes.
- `package-minwayland`: slower than incremental build because staging and tar/xz
  are mostly serial.
- `qemu-image-minwayland`: tens of seconds after APK/cache warmup.
- QEMU boot to Waterfox: about 20-30 seconds.

## Cache And Artifacts

Generated content lives under `.wfx-cache/` and is not tracked.

- `.wfx-cache/apk`: APK cache.
- `.wfx-cache/sources`: source tarballs.
- `.wfx-cache/build`: unpacked/intermediate dependency builds.
- `.wfx-cache/build-deps`: build-only source-built deps, such as mimalloc v3.
- `.wfx-cache/sysroot`: legacy GTK runtime sysroot mounted as `/opt/wfx/sysroot`.
- `.wfx-cache/mozbuild`: Gecko build state.
- `.wfx-cache/sccache`: compiler cache.
- `.wfx-cache/cargo`: Cargo cache.
- `.wfx-cache/obj-aarch64-alpine-linux-musl-minwayland`: minwayland debug objdir.
- `.wfx-cache/obj-aarch64-alpine-linux-musl`: legacy GTK debug objdir.
- `.wfx-cache/obj-aarch64-alpine-linux-musl-release`: release objdir.
- `.wfx-cache/dist/minwayland-root`: staged minwayland root used by
  `qemu-image-minwayland`.
- `.wfx-cache/dist`: staged roots, packages, manifests, dependency reports, and
  smoke reports.
- `.wfx-cache/kiosk-compositor`: custom DRM/libinput cage/wlroots install.
- `.wfx-cache/qemu`: QEMU rootfs, kernel, initramfs, disk image, and repro
  screenshots/logs.

Current minwayland artifact path:

```text
.wfx-cache/dist/waterfox-140.11.0esr.en-US.linux-musl-aarch64.stage1-minwayland.tar.xz
```

Current legacy GTK debug artifact path:

```text
.wfx-cache/dist/waterfox-140.11.0esr.en-US.linux-musl-aarch64.stage1-debug.tar.xz
```

The latest manifest is
`.wfx-cache/dist/waterfox-140.11.0esr.en-US.linux-musl-aarch64.manifest.txt`.
The manifest records the Waterfox git commit/dirty state, Alpine release,
toolchain commands, mozconfig, APK package list, source lock, dependency
reports, and artifact paths.

The squashed `/tmp` checkout may not initially have all caches. Reuse caches
from `/Users/josh/d/waterfox-musl/.wfx-cache/` only when needed. For QEMU-only
minwayland iteration, `dist/minwayland-root` and `kiosk-compositor` are usually
enough.

## Version And Source Policy

Alpine edge is the source of truth for package versions when Alpine carries the
needed package. Run `docker/waterfox-musl/wfx-musl versions` before refreshing
source locks or final manifests.

Use upstream latest only when Alpine edge does not provide the required major
version. The current example is mimalloc v3: Alpine edge carries mimalloc v2,
so v3 is source-built and pinned in `docker/waterfox-musl/sources.lock`.

Do not enable Alpine system NSS, NSPR, SQLite, ICU, media codec, or other large
system-library substitutions as part of stage 1 unless a task explicitly targets
that dependency policy.

## Build Configuration

The primary debug mozconfig is `docker/waterfox-musl/mozconfig.minwayland`.

Important minwayland choices:

- `--host=aarch64-alpine-linux-musl`
- `--target=aarch64-alpine-linux-musl`
- `--enable-default-toolkit=cairo-minwayland`
- `--enable-linker=mold`
- `--enable-debug`
- `--disable-optimize`
- `--enable-rust-debug`
- `--disable-printing`
- `--disable-webmidi-midir`
- `--enable-mimalloc-replace`
- `--with-mimalloc-prefix=/opt/wfx/build-deps/mimalloc`
- `--disable-waterfox-blocker`

The minwayland objdir is:

```text
/cache/obj-aarch64-alpine-linux-musl-minwayland
```

The legacy GTK debug mozconfig is `docker/waterfox-musl/mozconfig.stage1`; it
uses `--enable-default-toolkit=cairo-gtk3-wayland-only` and should only be used
for comparison or tasks that explicitly target the old GTK stage.

The release mozconfig is `docker/waterfox-musl/mozconfig.release`; the folded
release mozconfig is `docker/waterfox-musl/mozconfig.folded-release`. Both are
legacy GTK release paths unless a task explicitly updates them.

If a fresh release objdir fails early with missing
`dist/system_wrappers/.moz-case-probe`, create the directory and rerun:

```sh
mkdir -p .wfx-cache/obj-aarch64-alpine-linux-musl-release/dist/system_wrappers
```

## Runtime Closure

Minwayland Waterfox should depend on raw Wayland, xkbcommon, fontconfig,
freetype, musl, libstdc++/libgcc runtime support, and the bundled Gecko
libraries. It must not depend on GTK/GDK/GLib/GIO/Pango/ATK, `libmozgtk.so`, or
`libmozwayland.so`.

The legacy custom GTK sysroot under `/opt/wfx/sysroot` still exists for older
stage1 packaging and diagnostics. It covers zlib, expat, libffi, pcre2, libpng,
libjpeg-turbo, freetype, fontconfig, pixman, fribidi, harfbuzz, cairo, wayland,
wayland-protocols, xkeyboard-config, libxkbcommon, gdk-pixbuf, and GTK 3.

Do not use Alpine stock `gtk+3.0` for the final Waterfox runtime because it
pulls X libraries. The QEMU rootfs may include Alpine GTK only for the
`gtk-popup-repro` diagnostic app; that is not part of the minwayland Waterfox
runtime closure.

Do not use Alpine stock `cage` for the QEMU proof because its wlroots stack
pulls unwanted XCB/Vulkan dependencies. Use the custom kiosk compositor build.

## Dependency And Rejection Rules

Static scans are in `check-waterfox-deps`, `package-stage1`,
`build-test-compositor`, `build-kiosk-compositor`, and `check-config`.

For minwayland packaging, these are now the defaults:

```text
WFX_STAGE_ROOT=/cache/dist/minwayland-root
WFX_WATERFOX_ALLOWED_NEEDED=/work/docker/waterfox-musl/waterfox-minwayland-allowed-needed.txt
```

Legacy GTK package commands opt into their old stage roots and allowlists.

Rejected runtime families include:

- X11, X11-xcb, XCB, Xcomposite, Xcursor, Xdamage, Xext, Xfixes, Xi, Xinerama,
  Xrandr, Xrender, Xtst, Xxf86vm, xkbfile, Xwayland, and GLX.
- Vulkan.
- PulseAudio, PipeWire audio, and ALSA unless explicitly using the ALSA debug
  profile.
- DBus, portals, notification, secret-service.
- speechd and cups.
- GTK/GDK/GLib/GIO/Pango/ATK in minwayland Waterfox artifacts.
- dynamic `libmimalloc.so`.

Relrhack and packed relative relocations are disabled. The earlier path emitted
Android packed relocation dynamic tags; Alpine musl did not apply those during
`dlopen`, which crashed during NSPR initialization. Do not re-enable without
checking `llvm-readelf` output and a packaged runtime smoke.

## QEMU Proof

Preferred minwayland image build:

```sh
WFX_JOBS=8 WFX_CARGO_JOBS=8 docker/waterfox-musl/wfx-musl qemu-image-minwayland
```

Preferred visible command:

```sh
WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 docker/waterfox-musl/wfx-musl qemu-run
```

Default path is `hvf + virtio + cocoa`. QEMU user networking is enabled by
default. The guest profile lives in `/run/wfx-profile`, so repeat boots do not
dirty the root image or hit read-only profile cleanup failures.

Important QEMU defaults and knobs:

- `WFX_QEMU_DISPLAY`, default `cocoa,zoom-to-fit=on,show-cursor=on`.
- `WFX_QEMU_ACCEL`, default `hvf`.
- `WFX_QEMU_GPU`, default `virtio` with `hvf`, `bochs` with `tcg`.
- `WFX_QEMU_WIDTH`, default `1024`; use `800` for faster visible iteration.
- `WFX_QEMU_HEIGHT`, default `768`; use `600` for faster visible iteration.
- `WFX_QEMU_INPUT`, default `virtio`; accepted values are `virtio`, `usb`,
  `both`, and `none`.
- `WFX_QEMU_NETWORK`, default `user`; `none` disables networking.
- `WFX_QEMU_SERIAL`, default `stdio`.
- `WFX_QEMU_MONITOR`, default `none`.
- `WFX_QEMU_QMP`, optional QMP socket.
- `WFX_QEMU_GPU_OPTS`, optional raw comma-separated device options.
- `WFX_QEMU_VNC`, optional VNC endpoint.
- `WFX_QEMU_GUEST_BROWSER`, accepted values are `waterfox`, `firefox`, and
  `gtk-popup-repro`.
- `WFX_QEMU_GUEST_COMPOSITOR`, accepted values are `cage` and `weston`.
- `WFX_QEMU_GUEST_WLR_RENDERER`, optional guest override for `WLR_RENDERER`.
- `WFX_QEMU_GUEST_DRM_NO_ATOMIC`, optional guest override for
  `WLR_DRM_NO_ATOMIC`.
- `WFX_QEMU_GUEST_SOFTWARE_WEBRENDER=1`, diagnostic Software WebRender/SWGL
  comparison.
- `WFX_QEMU_GUEST_FULL_DAMAGE=0`, disables the QEMU-proof default
  `MOZ_WAYLAND_FULL_DAMAGE=1`.
- `WFX_QEMU_GUEST_CLIPBOARD_SEED`, seeds the guest Wayland clipboard with
  `wl-copy` for paste verification.
- `WFX_QEMU_GUEST_CLIPBOARD_PROBE_AFTER`, runs guest `wl-paste` after the given
  number of seconds and logs the value to serial.
- `WFX_QEMU_INCLUDE_ALPINE_FIREFOX=1`, build-time image option for Alpine
  Firefox A/B diagnostics.
- `WFX_QEMU_INCLUDE_WESTON=1`, build-time image option for Weston compositor
  A/B diagnostics. Increase image size with `WFX_QEMU_IMAGE_MB=3072`.

If the Cocoa window is blank, check the launch line first. `hvf` should use
`gpu=virtio`; `tcg` should use `gpu=bochs`. `hvf + bochs + Cocoa` can leave the
host window black even when QEMU `screendump` captures a valid framebuffer, so
it is diagnostic-only and requires `WFX_QEMU_ALLOW_HVF_BOCHS=1`.

`WFX_QEMU_GPU=virtio-mmio` currently fails before browser launch in direct
kernel boot because wlroots sees zero DRM GPUs. `stdvga` and `secondary-vga`
are diagnostic display paths. `cirrus` with HVF trips QEMU's framebuffer
page-alignment assertion and is blocked.

## Automated QEMU Repros

Use `docker/waterfox-musl/qemu-repro-hamburger` as the fast visible regression
loop for menu/text rendering and simple QMP input flows. It boots QEMU, waits
for Waterfox, clicks through QMP, writes full-frame PPM screenshots, writes a
right-edge strip, records luma-based black-screen stats in `repro.txt`, and
exits nonzero on effectively black frames.

Hamburger menu:

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=hamburger WFX_REPRO_BOOT_WAIT=35 WFX_REPRO_AFTER_CLICK_WAIT=4 \
  docker/waterfox-musl/qemu-repro-hamburger
```

Context menu:

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=context WFX_REPRO_BOOT_WAIT=35 WFX_REPRO_AFTER_CLICK_WAIT=4 \
  WFX_REPRO_CLICK_X=400 WFX_REPRO_CLICK_Y=300 WFX_REPRO_BUTTON=right \
  docker/waterfox-musl/qemu-repro-hamburger
```

URL bar typing:

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=urlbar WFX_REPRO_BOOT_WAIT=35 WFX_REPRO_AFTER_CLICK_WAIT=2 \
  WFX_REPRO_CLICK_X=330 WFX_REPRO_CLICK_Y=49 WFX_REPRO_TEXT=abc.com \
  docker/waterfox-musl/qemu-repro-hamburger
```

Guest `wl-copy` to Waterfox paste:

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=minwayland-clipboard-wlcopy-to-browser \
  WFX_REPRO_BOOT_WAIT=35 WFX_REPRO_AFTER_CLICK_WAIT=2 \
  WFX_REPRO_CLICK_X=330 WFX_REPRO_CLICK_Y=49 \
  WFX_REPRO_KEYS_BEFORE_TEXT=ctrl-v \
  WFX_QEMU_GUEST_CLIPBOARD_SEED=from-wl-copy \
  docker/waterfox-musl/qemu-repro-hamburger
```

Waterfox copy to guest `wl-paste`:

```sh
env WFX_QEMU_WIDTH=800 WFX_QEMU_HEIGHT=600 \
  WFX_REPRO_ID=minwayland-clipboard-browser-to-wlpaste \
  WFX_REPRO_BOOT_WAIT=35 WFX_REPRO_AFTER_CLICK_WAIT=12 \
  WFX_REPRO_CLICK_X=330 WFX_REPRO_CLICK_Y=49 \
  WFX_REPRO_TEXT_BEFORE_KEYS=from-browser \
  WFX_REPRO_KEYS_BEFORE_TEXT=ctrl-a,ctrl-c \
  WFX_QEMU_GUEST_CLIPBOARD_PROBE_AFTER=45 \
  docker/waterfox-musl/qemu-repro-hamburger
```

Check the reverse clipboard result with:

```sh
rg -n "wfx-clipboard-probe" .wfx-cache/qemu/repro-hamburger-minwayland-clipboard-browser-to-wlpaste/serial.log
```

The GTK control app is `docker/waterfox-musl/gtk-popup-repro.c`, packaged into
the QEMU image as `/usr/bin/wfx-gtk-popup-repro`. It is a diagnostic comparator
only:

```sh
WFX_QEMU_GUEST_BROWSER=gtk-popup-repro docker/waterfox-musl/qemu-repro-hamburger
```

## Important Fixes And Diagnostics

QEMU/rootfs issues already solved:

- Direct kernel boot could not see `/dev/vda`; `qemu-image` now creates
  `initramfs-virt`.
- `apk --no-scripts` left BusyBox applets missing; `qemu-image` installs
  applet symlinks with `busybox --install -s`.
- Cage needed `libgcc_s.so.1`; Waterfox needed `libstdc++.so.6`.
- Alpine `seatd` does not support the earlier `-s` socket option.
- QEMU input requires wlroots `drm,libinput` backends; virtio keyboard/tablet
  now create guest event devices.
- Waterfox glxtest no longer reports missing `libpci` or `libEGL` after adding
  Mesa EGL/GLES, GBM, Gallium, and `pciutils-libs` to the QEMU rootfs.
- Cage debugoptimized builds used to force wlroots debug logs. The Cage patch
  now defaults runtime wlroots logging to errors while preserving `-D` as the
  explicit debug opt-in.
- The kiosk profile disables TRR/DoH and ORB JavaScript validation to keep QEMU
  networking native and avoid a debug-only JSOracle utility-process assertion.

Waterfox/Gecko issues already solved:

- `browser/locales/en-US/browser/waterfox.ftl` restores a resource Waterfox
  browser chrome loads unconditionally. Without it, URL bar typing hit
  `selectedBrowser` and `UrlbarInput` exceptions after localization bundle
  failures.
- The old GTK URL bar/menu rendering failure was caused by zero-sized chrome
  fonts from invalid GTK look-and-feel text scale. The historical fix is in
  `widget/gtk/nsLookAndFeel.cpp`; do not chase that path for minwayland unless
  a task explicitly targets the legacy GTK backend.
- `widget/minwayland` now provides windows, software frame presentation,
  pointer input, keyboard input, popup/menu behavior, and regular Wayland text
  clipboard.
- `--disable-waterfox-blocker` prevents WaterfoxBlocker from being built or
  registered in the musl build. The QEMU kiosk profile also disables blocker
  prefs as a runtime fallback.

Useful historical diagnostics:

- Alpine Firefox reproduced the same UI repaint failures as Waterfox before the
  font-scale fix, so the Waterfox feature set was not the primary cause.
- Weston reproduced the same UI repaint failures as cage/wlroots before the
  font-scale fix, so the issue was not cage-specific.
- Software WebRender/SWGL could log
  `RenderCompositorSWGL failed mapping default framebuffer, no dt` while
  clicking popups; it is no longer forced by default.
- `virtio_gpu: driver missing` and WebRender texture-crop warnings were
  investigated while the font issue was unresolved; do not chase them again
  unless a new visible rendering failure appears.

## Debugging References

Useful docs and reports:

- `WIDGETS-TODO.md`: current minwayland widget scope, completed state, and file
  picker requirements.
- `.wfx-cache/dist/*.manifest.txt`
- `.wfx-cache/dist/*deps.txt`
- `.wfx-cache/qemu/repro-hamburger-*/repro.txt`
- QEMU serial output from `qemu-run`

For dependency resolution inside the assembled QEMU rootfs:

```sh
docker run --rm --platform linux/arm64 \
  --volume "$PWD/.wfx-cache:/cache" \
  waterfox-musl:edge-clang22 \
  chroot /cache/qemu/rootfs /bin/sh -euc \
  'export LD_LIBRARY_PATH=/opt/wfx/kiosk-compositor/lib:/opt/waterfox:/opt/wfx/sysroot/lib; ldd /opt/waterfox/waterfox; ldd /opt/wfx/kiosk-compositor/bin/cage'
```

Local Wayland references:

- `~/d/wl-clipboard`: useful reference for `wl_data_device_manager`,
  `wl_data_source`, and `wl_data_offer` clipboard behavior.
- `~/d/sway`: useful reference for production Wayland/wlroots client and
  protocol handling patterns.

## Completion Criteria

Before calling minwayland work complete, verify the relevant subset:

- `configure-minwayland` passes if config changed.
- `build-minwayland` passes.
- `package-minwayland` passes and static dependency scans pass.
- WaterfoxBlocker remains excluded.
- The minwayland staged artifact has no GTK/GDK/GLib/GIO/Pango/ATK,
  `libmozgtk.so`, `libmozwayland.so`, X11/XCB/Xwayland/GLX, Vulkan, DBus,
  portal, cups, speechd, or dynamic `libmimalloc.so` dependency.
- QEMU image is rebuilt with `qemu-image-minwayland` from the current staged
  root.
- QEMU boots without profile/rootfs panics.
- Cocoa QEMU visibly renders Waterfox.
- `Ctrl-C` from a real terminal terminates QEMU cleanly.
- Keyboard and mouse input reach the guest.
- Hamburger menu, context menu, and URL bar typed text pass the QEMU repro
  screenshots.
- Clipboard changes pass both guest `wl-copy -> Waterfox paste` and Waterfox
  copy -> guest `wl-paste` repros.
