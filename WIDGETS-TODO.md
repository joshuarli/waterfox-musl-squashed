# Minimal Wayland Widget Backend

This is a handoff for a future investigation on real Linux hardware. The goal is to remove the GTK stack from the runtime path, not to redesign the browser chrome.

## Goal

Build a Waterfox/Gecko Linux widget backend that is Wayland-only and intentionally small:

- one normal fullscreen/maximized top-level browser window
- pointer and keyboard input
- UTF-8 text input good enough for the URL bar and web forms
- clipboard copy/paste
- font discovery/rasterization through fontconfig/freetype/harfbuzz
- no X11
- no GTK/GDK/GLib/GIO/Pango
- no accessibility
- no drag and drop
- no native GTK/portal file picker
- minimal browser-owned file picker is allowed
- no printing
- no desktop portal integration

The browser chrome is already mostly Gecko-rendered XUL/HTML/CSS/JS. Replacing it with more web components would not remove GTK by itself. GTK currently matters because `widget/gtk` is Firefox's Linux platform backend: native windows, Wayland surfaces, input, clipboard, IME, popup widgets, screen metrics, cursors, theme/look-and-feel, and compositor plumbing.

## Current Starting Point

The active build uses:

```sh
ac_add_options --enable-default-toolkit=cairo-gtk3-wayland-only
```

In toolkit/moz.configure, all non-Windows/macOS/iOS/Android Linux choices currently map `cairo-gtk3*` to:

```python
MOZ_WIDGET_TOOLKIT = "gtk"
```

The relevant backend directories are:

- widget/gtk: current Linux GTK/Wayland backend
- widget/headless: useful for stubs and no-window behavior
- widget/generic: generic compositor IPDL definitions
- widget/moz.build: selects backend dirs, IPDL, includes, and toolkit-specific XPIDL

Important GTK files to study first:

- widget/gtk/nsWindow.cpp: main native window implementation
- widget/gtk/nsWindow.h: surface area that a replacement backend must satisfy
- widget/gtk/nsClipboardWayland.cpp: Wayland clipboard behavior
- widget/gtk/nsWaylandDisplay.cpp: Wayland display globals and event handling
- widget/gtk/MozContainerWayland.cpp): GTK container bridge to Wayland surfaces
- widget/gtk/WindowSurfaceWaylandMultiBuffer.cpp: software surface path
- widget/gtk/nsGtkKeyUtils.cpp: key mapping details
- widget/gtk/IMContextWrapper.cpp: GTK IME integration to avoid or replace
- widget/gtk/nsLookAndFeel.cpp: GTK theme/settings source
- widget/gtk/ScreenHelperGTK.cpp: monitor/screen metrics

## Current Progress

The first minwayland QEMU/debug proof is in place:

- `--enable-default-toolkit=cairo-minwayland` configures
  `MOZ_WIDGET_TOOLKIT=minwayland`, `MOZ_WIDGET_MINWAYLAND=1`, and `MOZ_WAYLAND=1`.
- `docker/waterfox-musl/mozconfig.minwayland` plus `configure-minwayland` and
  `build-minwayland` build an arm64 musl debug objdir at
  `.wfx-cache/obj-aarch64-alpine-linux-musl-minwayland`.
- `package-minwayland` stages an artifact at `.wfx-cache/dist/minwayland-root`,
  and `qemu-image-minwayland` boots it in the existing kiosk QEMU path.
- `widget/minwayland` provides the active Linux widget backend for this profile:
  raw Wayland display/event handling, a fullscreen/maximized `xdg_toplevel`,
  `xdg_popup` popup widgets, software shared-memory drawing, pointer input,
  wheel input, keyboard input through xkbcommon, basic editor shortcuts, screen
  metrics, and hardcoded look-and-feel defaults.
- Hamburger menu, context menu, URL bar typing, Backspace, Enter, Ctrl/Cmd+L,
  Ctrl/Cmd+A, and browser-internal copy/paste have been verified with QEMU
  repros. Repeated hamburger open/close stress repros also pass after removing
  display-mutex self-locks and keeping native Wayland surface creation on the
  main thread.
- The minwayland configure path depends on raw Wayland, xkbcommon,
  fontconfig, and freetype rather than GTK/GDK/GLib/GIO/Pango.
- Staged runtime comparison against the GTK stage1 root shows the expected
  reduction: no direct GTK/GDK/GLib/GIO/Pango/ATK runtime dependency, no
  `libmozgtk.so` or `libmozwayland.so`, and a smaller direct and transitive
  shared-library dependency set.

The first tranche is closed for the debug QEMU proof. Remaining work is focused
on integration depth rather than proving the backend shape.

## Remaining Scope

Required next:

- Wayland clipboard bridging via `wl_data_device_manager`, verified in QEMU with
  `wl-copy` and `wl-paste`.
- Minimal browser-owned file picker for uploads and simple save paths, without
  GTK or portal dependencies.

Deferred or explicitly out of scope for the current kiosk profile:

- Deep IME support: preedit/composition/candidate-window behavior for complex
  input methods. Basic UTF-8 key input is already sufficient for URL bar and
  normal ASCII form entry.
- Drag and drop (DnD): Wayland data-device drag sessions between applications or
  inside the browser. This remains out of scope.
- Accessibility: not required for this profile.
- Native desktop dialogs: Wayland itself has no native dialog protocol. GTK or
  portal-backed file pickers remain out of scope; minwayland should use a
  browser-owned picker if file selection is needed.
- Print dialogs and printing UI are out of scope.

## Minimal File Picker Requirements

The first minwayland file picker should be deliberately small and owned by the
browser/runtime, not a desktop integration layer.

Required:

- Support `nsIFilePicker::modeOpen` for `<input type=file>` and explicit upload
  flows.
- Support `modeOpenMultiple` for multi-file upload.
- Return `nsIFilePicker::returnOK` with selected `nsIFile` objects, or
  `returnCancel`.
- Honor `displayDirectory` when provided, otherwise start in a constrained
  default such as `$HOME`, `/home/wfx`, `/run/wfx-profile`, or a configured
  kiosk downloads/uploads directory.
- Provide keyboard and pointer navigation, including Enter to accept and Escape
  to cancel.
- Filter out `.` and `..`, show directories before files, and allow entering and
  leaving directories within the allowed root policy.
- Avoid hidden dependencies on GTK, portals, DBus, GLib, GIO, or desktop MIME
  services.

Can be deferred:

- `modeSave`, overwrite confirmation, and default-extension handling.
- `modeGetFolder`.
- Rich previews, thumbnails, icons, recent files, bookmarks, sorting controls,
  search, and MIME sniffing.
- Remote URLs and virtual filesystems.
- Camera capture modes.

Implementation options:

- Preferred first cut: an in-browser chrome dialog backed by the minwayland
  `nsIFilePicker` implementation. The widget service supplies directory entries
  and receives the selected path; Gecko/XUL/HTML handles rendering.
- Simpler but rougher fallback: a controlled path-entry dialog that accepts a
  typed file path under an allowed root. This is enough for diagnostics, but it
  is not a good long-term upload UX.

## Proposed Backend Shape

Create a new backend, tentatively:

```text
widget/minwayland/
```

Add a configure choice such as:

```sh
--enable-default-toolkit=cairo-minwayland
```

Map it to:

```python
MOZ_WIDGET_TOOLKIT = "minwayland"
MOZ_WIDGET_MINWAYLAND = 1
MOZ_WAYLAND = 1
```

Do not reuse `gtk` as the toolkit name. It keeps accidental GTK assumptions visible at compile time.

Initial external libraries should be small:

- `wayland-client`
- `wayland-cursor` only if cursors are not drawn manually
- `wayland-protocols`
- `xkbcommon`
- `fontconfig`
- `freetype`
- `harfbuzz`
- probably `cairo` initially, unless the compositor path can avoid it

Avoid adding GLib as a convenience event loop. Use Firefox's existing app shell/event mechanisms plus raw Wayland file descriptor dispatch.

## First Milestone

The first useful proof is not a full browser. It is:

1. Build with `MOZ_WIDGET_TOOLKIT=minwayland`. Done.
2. Start Waterfox under a Wayland compositor. Done in QEMU/cage.
3. Create one top-level `xdg_toplevel`. Done.
4. Paint a nonblank browser window. Done.
5. Accept pointer movement/clicks and keyboard input. Done.
6. Render popup widgets for browser chrome menus. Done.
7. Support browser-internal copy/paste shortcuts. Done.
8. Exit cleanly during normal QEMU repro runs. Done.

Use software WebRender first. Hardware acceleration, dmabuf, direct scanout, EGL, and VAAPI are later work.

## Likely Required Pieces

Implement or adapt equivalents for:

- `nsIWidget` via a new `nsWindow`
- `nsAppShell` or a minimal event-loop bridge
- `nsClipboard` for Wayland data-control/data-device copy/paste
- `nsLookAndFeel` with hardcoded sane defaults instead of desktop theme probing
- screen helper for one output, then multiple outputs later
- compositor widget classes, probably starting from generic/headless plus the GTK Wayland software surface path
- keyboard mapping from raw Wayland/xkbcommon to Gecko key events
- pointer events and cursor state
- popup windows

Popup windows are important even for humble browser use. The hamburger menu, context menus, URL bar suggestions, permission prompts, and select dropdowns all exercise popup widget behavior. If real `xdg_popup` support is too much at first, consider an explicit temporary hack that draws popups inside the main toplevel. Document that hack clearly if used.

## Likely Compile Breaks

Expect references to `MOZ_WIDGET_GTK`, `MOZ_GTK3_CFLAGS`, GTK-specific XPIDL, and GTK-only headers to surface outside `widget/gtk`.

Search patterns:

```sh
rg -n "MOZ_WIDGET_GTK|MOZ_GTK|MOZ_WAYLAND|gtk/|nsWindow|nsClipboard|nsLookAndFeel|Gdk|Gtk|glib|gobject|gio" \
  widget toolkit browser gfx dom layout xpcom
```

Try hard to fix these by isolating toolkit-specific code rather than making `minwayland` pretend to be GTK.

## Suggested Iteration Loop

Work on real Linux Wayland hardware first. QEMU/HVF has already cost too much time for rendering bugs and is not the right loop for this backend bring-up.

Start with configure-only:

```sh
docker/waterfox-musl/wfx-musl configure
```

Then build incrementally:

```sh
docker/waterfox-musl/wfx-musl build
```

Once it links, run outside QEMU under a real compositor with verbose widget logging enabled. Keep a tiny standalone Wayland smoke program beside the backend if it helps validate display, keyboard, and clipboard assumptions independently of Gecko.

## Non-Goals

- Do not attempt to statically link the full browser as part of this backend work.
- Do not port the browser chrome to a new web UI to solve GTK removal; that is a separate problem and does not remove the platform backend dependency.
- Do not support X11.
- Do not support GTK portals, native GTK dialogs, native menus, tray integration, MPRIS, print dialogs, or desktop theme fidelity.
- Do not chase perfect IME support in the first milestone. URL bar ASCII/UTF-8 input is enough to prove the backend path.

## Risk Assessment

This is a major port, not a package tweak. The architecture is modular enough to attempt because Firefox already has separate `widget/*` backends, but the GTK backend has accumulated Linux-specific behavior for years.

The most likely hard problems are:

- compositor integration and surface lifetime
- popup positioning and focus grabs
- keyboard/IME correctness
- clipboard protocol edge cases
- implicit GTK assumptions outside `widget/gtk`
- keeping the patch set rebased across Waterfox/Firefox ESR updates

The payoff, if it works, is large: no GTK/GDK/GLib/GIO/Pango runtime dependency, fewer `.so` loads, less legacy C platform code in the trusted runtime, and a browser runtime shaped specifically for the kiosk-like Waterfox/musl environment.
