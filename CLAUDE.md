# CLAUDE.md -- Godot 4.6.2-rc2 Fork: AccessKit Android + ThorVG Lottie

## Project Identity

This is a fork of Godot Engine 4.6.2-rc2. We are adding two capabilities that do not exist in upstream Godot:

1. **AccessKit Android TalkBack support** -- full screen reader accessibility on Android via AccessKit
2. **Native Lottie animation playback** -- via ThorVG's Lottie loader, already bundled but disabled

The final deliverables are Android build artifacts (`.aar` + `.so` files) for engineers to embed in native Android apps or use in standalone Godot Android apps.

## Repository Structure

```
godot/                              # Forked from godotengine/godot @ 4.6.2-rc2
├── drivers/accesskit/              # Existing AccessKit driver (desktop only today)
│   ├── accessibility_driver_accesskit.cpp
│   └── accessibility_driver_accesskit.h
├── platform/android/               # Android platform layer -- our primary modification target
│   ├── display_server_android.cpp/h
│   ├── os_android.cpp/h
│   ├── java_godot_wrapper.cpp/h
│   ├── SCsub
│   ├── detect.py
│   └── java/lib/src/org/godotengine/godot/
│       ├── Godot.kt
│       ├── GodotActivity.kt
│       ├── GodotFragment.java
│       ├── GodotView.kt
│       └── accessibility/          # NEW -- Java-side a11y bridge
├── platform/windows/               # Reference: how AccessKit is wired on Windows
│   └── display_server_windows.cpp
├── platform/linuxbsd/              # Reference: how AccessKit is wired on Linux
│   └── display_server_x11.cpp
├── platform/macos/                 # Reference: how AccessKit is wired on macOS
│   └── display_server_macos.mm
├── thirdparty/thorvg/              # Bundled ThorVG -- SVG only today, we enable Lottie
│   ├── SCsub
│   └── src/loaders/lottie/         # Lottie loader sources (exist but not compiled)
├── thirdparty/accesskit/           # Pre-built accesskit-c static libs
│   └── android/                    # NEW -- we add Android ABI libs here
├── scene/2d/                       # Where LottieAnimation node goes
├── scene/resources/                # Where LottieResource goes
├── modules/lottie/                 # NEW -- Lottie module registration
├── accesskit-c/                    # Subproject: fork of AccessKit/accesskit-c with Android targets
└── tests/                          # Validation scripts and sample projects
```

## Build System

Godot uses **SCons** (not CMake, not Make). Key commands:

```bash
# Desktop build (for development testing)
scons platform=linuxbsd target=editor -j$(nproc)

# Android export template (the real target)
scons platform=android target=template_release arch=arm64 accesskit=yes lottie=yes
scons platform=android target=template_release arch=arm32 accesskit=yes lottie=yes
scons platform=android target=template_release arch=x86_64 accesskit=yes lottie=yes

# Android AAR packaging
cd platform/android/java/
./gradlew generateGodotTemplates
```

Build flags we introduce:
- `accesskit=yes` (default: yes on desktop, we make it yes on android too)
- `lottie=yes` (default: no, we add this)

The `ACCESSKIT_ENABLED` C++ preprocessor define gates all AccessKit code. On platforms where AccessKit is disabled, it compiles out cleanly.

## Key Technical Patterns

### How AccessKit works in Godot (study these first)

1. `drivers/accesskit/accessibility_driver_accesskit.cpp` -- The core driver. It manages the AccessKit tree, handles tree updates, and delegates to platform-specific accesskit-c functions.

2. Each platform's DisplayServer has `#ifdef ACCESSKIT_ENABLED` blocks:
   - `window_create()` calls `accessibility_driver->window_create(id, native_handle)`
   - Control node changes trigger tree updates via `DisplayServer::accessibility_update_*()` methods
   - The driver pushes incremental `TreeUpdate` structs to the accesskit-c adapter

3. accesskit-c provides platform-specific adapter functions:
   - Windows: `accesskit_windows_*` functions
   - macOS: `accesskit_macos_*` functions
   - Unix: `accesskit_unix_*` functions
   - Android: `accesskit_android_*` functions -- **these exist in the Rust crate but NOT in the C bindings yet**

4. The static libs are in `thirdparty/accesskit/` organized by platform

### How ThorVG works in Godot

1. `thirdparty/thorvg/SCsub` controls which ThorVG source files get compiled
2. Currently only SVG loader is enabled
3. ThorVG is used via `modules/svg/` for SVG import
4. The Lottie loader sources exist in ThorVG's source tree but are excluded from the SCsub file list

### Godot's Android build pipeline

1. SCons compiles C++ into `libgodot_android.so` for each ABI
2. Gradle packages the SO files + Java classes into `godot-lib.aar`
3. The AAR can be consumed as a dependency by any Android project
4. For embedded mode, engineers use `GodotFragment` inside their own Activity
5. For standalone mode, engineers use `GodotActivity` directly

## Phase Execution Order

**Do Phase 1 (ThorVG Lottie) before Phase 2 (AccessKit Android).** Lottie is lower risk and validates the build pipeline.

### Phase 1: ThorVG Lottie Native Support

Goal: `LottieAnimation` node that plays .json/.lottie files on all platforms including Android.

Steps:
1. Check which ThorVG version is bundled in `thirdparty/thorvg/`. Compare with latest ThorVG release. Update if the bundled version lacks Lottie support files.
2. Modify `thirdparty/thorvg/SCsub` to include Lottie loader source files from `src/loaders/lottie/`
3. Create `modules/lottie/` with:
   - `config.py` -- module config (depends on thorvg)
   - `register_types.cpp/h` -- registers LottieAnimation and LottieResource
   - `lottie_animation.cpp/h` -- Node2D subclass
   - `lottie_resource.cpp/h` -- Resource subclass for .json/.lottie files
   - `resource_importer_lottie.cpp/h` -- EditorImportPlugin for .json/.lottie
   - `SCsub` -- module build file
4. Implement rendering pipeline:
   - `tvg::SwCanvas` for software rendering
   - `tvg::Animation` for Lottie playback control
   - Render to `PackedByteArray` (ARGB32), convert to RGBA, create `Image`, update `ImageTexture`
   - Call `queue_redraw()` when frame changes, draw texture in `_draw()`
5. Test on desktop first (Linux editor)
6. Test Android build (verify .so includes Lottie symbols)
7. Test in embedded GodotFragment on Android

### Phase 2: AccessKit Android Adapter

Goal: Full TalkBack support for Godot Control nodes on Android, via AccessKit.

Steps:
1. **Build accesskit-c for Android:**
   - Fork `AccessKit/accesskit-c`
   - The `accesskit_android` Rust crate (v0.4.1) exists on crates.io
   - Add Android NDK cross-compilation targets to the CMake/Cargo build
   - Produce `libaccesskit.a` for arm64-v8a, armeabi-v7a, x86_64
   - The Android adapter needs JNI -- the Rust crate uses the `jni` crate internally
   - Place built libs in `thirdparty/accesskit/android/{abi}/libaccesskit.a`

2. **Wire AccessKit into Godot's Android platform:**
   - Modify `platform/android/detect.py`: set `accesskit=yes` as default for Android
   - Modify `platform/android/SCsub`: link `libaccesskit.a` for the target ABI
   - Modify `platform/android/display_server_android.cpp`:
     - In `DisplayServerAndroid::create()`: initialize AccessKit adapter
     - Pass Android-specific context (JNI env, Activity reference) to the adapter
     - Handle `FEATURE_ACCESSIBILITY_SCREEN_READER` -- return true when TalkBack is active
   - Study how `display_server_windows.cpp` does `accessibility_driver->window_create()` and replicate for Android

3. **Create Java-side accessibility bridge:**
   - New file: `platform/android/java/lib/src/org/godotengine/godot/accessibility/GodotAccessibilityDelegate.java`
   - This class implements `View.AccessibilityDelegate` or provides an `AccessibilityNodeProvider`
   - It bridges JNI calls from accesskit_android (Rust → C → JNI) to Android's accessibility framework
   - Must work in both standalone (GodotActivity) and embedded (GodotFragment) modes
   - In embedded mode: attach only to the GodotView, not the entire Activity window

4. **Implement full TalkBack feature set:**
   - Navigation order: AccessKit tree node ordering maps to `setTraversalAfter()`
   - Live regions: `LiveSetting::Polite/Assertive` → `TYPE_WINDOW_CONTENT_CHANGED`
   - Custom actions: AccessKit `CustomAction` → `AccessibilityNodeInfo.addAction()`
   - Focus management: `Focus` action → `ACTION_ACCESSIBILITY_FOCUS`
   - Text fields: `TextValue`/`TextSelection` → `setText()` + text change events
   - Scroll containers: `ScrollPosition` → `setScrollable()` + scroll events

5. **Test matrix:**
   - [ ] Standalone app: buttons, labels read by TalkBack
   - [ ] Standalone app: focus navigation order correct
   - [ ] Standalone app: live region announcements
   - [ ] Standalone app: custom actions
   - [ ] Standalone app: text input with TalkBack
   - [ ] Embedded fragment: TalkBack reads Godot UI
   - [ ] Embedded fragment: host app a11y tree not disrupted
   - [ ] Embedded fragment: focus moves between host and Godot views
   - [ ] Desktop regression: Windows/Linux AccessKit still works

### Phase 3: Packaging

1. Build all 3 ABIs (arm64, arm32, x86_64) with both features
2. Run Gradle to produce `godot-lib-release.aar`
3. Extract `.so` files from the AAR for engineers who need them standalone
4. Create integration guide document
5. Create sample projects (embedded + standalone)

## Code Conventions

- Follow Godot's code style: tabs for indentation, `snake_case` for methods/variables, `PascalCase` for classes
- All new C++ files need the Godot MIT license header
- Use `#ifdef ACCESSKIT_ENABLED` guards for all AccessKit code
- Use `#ifdef MODULE_LOTTIE_ENABLED` guards for Lottie code
- Register new classes in `register_types.cpp` using `GDREGISTER_CLASS()`
- Expose properties with `ClassDB::bind_method()` and `ADD_PROPERTY()`
- Signals with `ADD_SIGNAL(MethodInfo("signal_name"))`

## Dependencies & Toolchain

```
# System
Ubuntu 22.04+ (or Docker container)
Python 3.10+
SCons 4.x

# Android
Android SDK API 34+
Android NDK r26c+
JDK 17
Gradle 8.x (via wrapper in platform/android/java/)

# Rust (for building accesskit-c Android libs)
rustup with targets:
  - aarch64-linux-android
  - armv7-linux-androideabi
  - x86_64-linux-android
cargo-ndk

# Godot build deps
pkg-config, libx11-dev, libxcursor-dev, libxinerama-dev, libxi-dev, libxrandr-dev,
libgl-dev, libasound2-dev, libpulse-dev, libdbus-1-dev, libudev-dev
```

## Testing

- Desktop: run the editor, add LottieAnimation node, load a .json Lottie file, verify playback
- Desktop: enable screen reader (Orca on Linux, Narrator on Windows), verify Control nodes are announced
- Android: build APK, install on device with TalkBack enabled, verify UI elements are read
- Android embedded: create a host Android app with GodotFragment, verify TalkBack works within the fragment
- Lottie perf: load a complex Lottie (Bodymovin export, 60fps, 500+ layers), measure frame rate on a mid-range Android device (target: 30fps minimum for complex, 60fps for simple UI anims)

## What NOT To Do

- Do NOT modify the upstream Godot API surface unnecessarily -- keep the fork minimal and mergeable
- Do NOT use GDExtension for this -- it must be baked into the engine for embedded mode to work reliably
- Do NOT skip accesskit-c and call the Rust crate directly -- Godot's build system is SCons/C++, not Cargo
- Do NOT implement a Java-only TalkBack solution that bypasses AccessKit -- we need cross-platform consistency
- Do NOT enable ThorVG Lottie expressions (JS evaluation) by default -- security and binary size concerns
- Do NOT break the existing desktop AccessKit integration
