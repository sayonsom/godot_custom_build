# Godot 4.6.2-rc2 Fork: AccessKit Android TalkBack + ThorVG Lottie

## Executive Summary

Fork Godot 4.6.2-rc2 to add two capabilities that do not exist in upstream Godot today:

1. **AccessKit Android adapter integration** -- enabling full TalkBack support (navigation order, live regions, custom actions, focus management) for Godot Control nodes on Android
2. **Native Lottie animation playback via ThorVG** -- leveraging the ThorVG library already bundled in Godot for SVG, but extending it to load and play `.lottie` / `.json` Lottie files

Deliverables: `.aar` (Android Archive) + `.so` (native shared libraries for arm64-v8a, armeabi-v7a, x86_64) that engineers can integrate into both standalone Godot Android apps and embedded Godot fragments within existing native Android applications.

---

## Current State Assessment

### AccessKit in Godot (as of 4.6.x)

- Godot 4.5 integrated AccessKit via PR #76829 (~32K lines of code)
- Platform adapters wired: **Windows (UI Automation)**, **macOS (NSAccessibility)**, **Linux (AT-SPI via D-Bus)**
- **Android: NOT wired.** Godot's `platform/android/` display server has no AccessKit integration
- The AccessKit project *itself* now ships `accesskit_android` v0.4.1 (published March 2026 on crates.io, ~2K SLoC), which implements Android's `AccessibilityNodeProvider` via JNI
- The `accesskit-c` C bindings repo does NOT yet include Android pre-built static libraries
- Godot consumes AccessKit via `accesskit-c` static libs from `godotengine/godot-accesskit-c-static`

### ThorVG / Lottie in Godot (as of 4.6.x)

- ThorVG is bundled in `thirdparty/thorvg/` -- used exclusively for SVG rendering
- ThorVG itself fully supports Lottie playback (it's ThorVG's flagship feature)
- Godot builds ThorVG with SVG loader only; the Lottie loader is disabled at compile time
- The Kyooz/Lottie-godot GDExtension exists but breaks on Android embedded mode
- Godot proposal #9104 (by ThorVG's lead) suggests the integration is straightforward
- No native `LottieAnimation` node exists in Godot

---

## Architecture

### Work Package 1: AccessKit Android Adapter for Godot

#### Layer Stack

```
┌─────────────────────────────────────────────┐
│  Godot Control Nodes (Button, Label, etc.)  │
│  → set_accessibility_name(), role, etc.     │
├─────────────────────────────────────────────┤
│  DisplayServerAccessibility (existing)       │
│  → Pushes AccessKit tree updates            │
├─────────────────────────────────────────────┤
│  accesskit-c (C API)                        │  ← Need to build for Android targets
│  → accesskit_android_adapter_*() functions  │
├─────────────────────────────────────────────┤
│  accesskit_android (Rust crate)             │  ← New: compile for aarch64-linux-android
│  → JNI bridge to Android a11y framework    │
├─────────────────────────────────────────────┤
│  GodotFragment / GodotActivity (Java)       │  ← Modify: wire AccessibilityDelegate
│  → AccessibilityNodeProvider                │
├─────────────────────────────────────────────┤
│  Android TalkBack                           │
└─────────────────────────────────────────────┘
```

#### Key Engineering Tasks

**1.1 Build `accesskit-c` for Android NDK targets**

- Fork `AccessKit/accesskit-c`
- Add Cargo targets: `aarch64-linux-android`, `armv7-linux-androideabi`, `x86_64-linux-android`
- Build static libraries (`libaccesskit.a`) for each ABI
- This requires the Android NDK's `aarch64-linux-android-clang` toolchain and JNI headers
- The `accesskit_android` crate uses `jni` Rust crate internally -- needs `JAVA_HOME` and `ANDROID_NDK_HOME`

**1.2 Modify Godot's Android platform layer**

Files to modify:
- `platform/android/display_server_android.h/cpp` -- Add AccessKit adapter initialization, tree push on Control node changes
- `platform/android/java/lib/src/org/godotengine/godot/GodotView.java` -- Wire `AccessibilityDelegate` or implement `AccessibilityNodeProvider` that delegates to AccessKit's JNI-exposed tree
- `platform/android/java/lib/src/org/godotengine/godot/GodotFragment.java` -- Ensure a11y service announcements work in embedded mode
- `platform/android/SCsub` -- Link against Android `libaccesskit.a`
- `platform/android/detect.py` -- Add `accesskit_android=yes` build flag

**1.3 Implement full TalkBack feature set**

AccessKit's Android adapter maps to these Android accessibility features:

| Feature | AccessKit Mapping | Android API |
|---------|-------------------|-------------|
| Navigation order | Tree node ordering + `AccessibilityNodeInfo.setTraversalAfter()` | `AccessibilityNodeProvider.createAccessibilityNodeInfo()` |
| Live regions | `LiveSetting::Polite/Assertive` property | `AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED` + `CONTENT_CHANGE_TYPE_TEXT` |
| Custom actions | `CustomAction` list on nodes | `AccessibilityNodeInfo.addAction()` |
| Focus management | `Focus` action handling + `IsFocused` state | `AccessibilityNodeInfo.setFocusable()` + `performAction(ACTION_ACCESSIBILITY_FOCUS)` |
| Text fields | `TextSelection`, `TextValue` properties | `AccessibilityNodeInfo.setText()` + `TYPE_VIEW_TEXT_CHANGED` events |
| Scroll containers | `ScrollPosition`, `ScrollTo` action | `AccessibilityNodeInfo.setScrollable()` + scroll events |

**1.4 Handle both standalone and embedded modes**

- Standalone: `GodotActivity` owns the window -- AccessKit adapter attaches to the root `View`
- Embedded: `GodotFragment` is hosted inside another app's Activity -- AccessKit adapter attaches to the `GodotView` (SurfaceView subclass), must respect the host app's existing a11y tree
- Key challenge: in embedded mode, the host app may already have its own `AccessibilityNodeProvider` on the parent ViewGroup. Godot's implementation must provide a11y info only for the Godot view subtree, not conflict with the host's tree

---

### Work Package 2: ThorVG Lottie Support

#### Architecture

```
┌──────────────────────────────────────┐
│  LottieAnimation (new Node2D)        │
│  → .lottie_file, .speed, .loop      │
│  → play(), pause(), seek()           │
├──────────────────────────────────────┤
│  LottieResource (new Resource)       │
│  → Loads .json / .lottie files       │
│  → Creates tvg::Animation instance   │
├──────────────────────────────────────┤
│  ThorVG (already in thirdparty/)     │
│  → Enable lottie loader at build     │
│  → tvg::Animation + tvg::Canvas      │
├──────────────────────────────────────┤
│  Godot RenderingServer               │
│  → ImageTexture updated each frame   │
└──────────────────────────────────────┘
```

#### Key Engineering Tasks

**2.1 Enable Lottie loader in ThorVG build**

- Modify `thirdparty/thorvg/SCsub` to include Lottie source files
- Add `lottie` to the enabled loaders list
- ThorVG Lottie loader files are in `src/loaders/lottie/` within the ThorVG source
- May need to update the bundled ThorVG version if Godot's copy is too old for full Lottie support

**2.2 Create new engine classes**

New files:
- `scene/2d/lottie_animation.h/cpp` -- LottieAnimation node
- `scene/resources/lottie_resource.h/cpp` -- LottieResource loader
- `modules/lottie/register_types.h/cpp` -- Module registration (or integrate directly into scene/)

LottieAnimation API:

```gdscript
class LottieAnimation extends Node2D:
    # Properties
    var lottie_file: LottieResource
    var speed: float = 1.0
    var loop: bool = true
    var autoplay: bool = true
    var frame: int  # current frame (read/write)
    var total_frames: int  # read-only
    var duration: float  # read-only, seconds

    # Methods
    func play() -> void
    func pause() -> void
    func stop() -> void
    func seek(frame: int) -> void
    func is_playing() -> bool

    # Signals
    signal animation_finished()
    signal animation_looped()
```

**2.3 Rendering pipeline**

- ThorVG renders to a CPU pixel buffer (ARGB32)
- Convert ARGB → RGBA (SIMD-optimized, ThorVG provides this)
- Upload to `ImageTexture` via `Image::create_from_data()`
- Draw via `draw_texture()` in `_draw()` or via CanvasItem
- Frame updates driven by `_process(delta)` -- advance animation time, re-render if frame changed

**2.4 Android-specific considerations**

- ThorVG's software renderer works on all platforms including Android -- no GPU dependency
- The Lottie loader adds ~100KB to binary size (acceptable)
- `.lottie` files (ZIP-compressed) need decompression -- ThorVG handles this internally
- Performance: ThorVG is SIMD-optimized (NEON on ARM64) -- should hit 60fps for typical UI animations

---

### Work Package 3: Android Build Artifacts

#### Output Structure

```
godot-custom-android/
├── godot-lib-4.6.2.aar          # Main Godot library AAR
│   ├── jni/
│   │   ├── arm64-v8a/
│   │   │   └── libgodot_android.so
│   │   ├── armeabi-v7a/
│   │   │   └── libgodot_android.so
│   │   └── x86_64/
│   │   │   └── libgodot_android.so
│   ├── classes.jar               # Java classes (GodotFragment, etc.)
│   └── AndroidManifest.xml
├── godot-editor-4.6.2.apk       # Optional: custom editor for testing
└── export-templates/
    ├── android_debug.apk
    └── android_release.apk
```

#### Build Process

```bash
# 1. Clone and checkout
git clone https://github.com/godotengine/godot.git
cd godot
git checkout 4.6.2-rc2

# 2. Apply patches (AccessKit Android + ThorVG Lottie)
git apply patches/accesskit-android.patch
git apply patches/thorvg-lottie.patch

# 3. Place pre-built accesskit-c Android static libs
cp -r accesskit-c-android/ thirdparty/accesskit/

# 4. Build Android export templates
scons platform=android target=template_release arch=arm64 \
    accesskit=yes lottie=yes
scons platform=android target=template_release arch=arm32 \
    accesskit=yes lottie=yes
scons platform=android target=template_release arch=x86_64 \
    accesskit=yes lottie=yes

# 5. Build the AAR
cd platform/android/java/
./gradlew generateGodotTemplates

# 6. Artifacts appear in:
#    platform/android/java/lib/build/outputs/aar/godot-lib-release.aar
```

---

## Implementation Phases

### Phase 0: Environment Setup (1-2 days)

- [ ] Fork Godot 4.6.2-rc2
- [ ] Set up Android NDK r26+ build environment
- [ ] Set up Rust cross-compilation toolchain for Android targets
- [ ] Verify vanilla Godot Android build works (produce baseline .aar)
- [ ] Fork `accesskit-c`, verify C bindings build for desktop

### Phase 1: ThorVG Lottie (1-2 weeks) -- Lower risk, do first

- [ ] Update bundled ThorVG to latest (if needed for Lottie completeness)
- [ ] Enable Lottie loader in SCsub
- [ ] Implement LottieResource importer
- [ ] Implement LottieAnimation Node2D
- [ ] Test on desktop first (Linux/Windows)
- [ ] Cross-compile and test on Android
- [ ] Verify in embedded GodotFragment mode

### Phase 2: AccessKit Android (2-4 weeks) -- Higher risk, core work

- [ ] Build `accesskit-c` with Android adapter for all ABIs
- [ ] Expose `accesskit_android_adapter_*` C functions
- [ ] Modify `DisplayServerAndroid` to push AccessKit tree
- [ ] Implement `GodotAccessibilityDelegate.java` (Java-side bridge)
- [ ] Wire into `GodotView` for standalone mode
- [ ] Wire into `GodotFragment` for embedded mode
- [ ] Test basic TalkBack: button labels, navigation
- [ ] Test full TalkBack: focus order, live regions, custom actions
- [ ] Test text input fields with TalkBack
- [ ] Verify no regression on desktop AccessKit (Windows/macOS/Linux)

### Phase 3: Integration & Packaging (1 week)

- [ ] Build all-ABI export templates
- [ ] Package `.aar` with both features
- [ ] Extract standalone `.so` files for each ABI
- [ ] Create sample project: embedded GodotFragment with a11y + Lottie
- [ ] Create sample project: standalone Godot app with a11y + Lottie
- [ ] Write integration guide for engineers

### Phase 4: Validation (1 week)

- [ ] TalkBack audit with physical Android device
- [ ] European Accessibility Act (EAA) compliance check
- [ ] Lottie performance benchmark (target: 60fps for typical UI anims on mid-range devices)
- [ ] Memory profile (AccessKit tree overhead, Lottie frame buffers)
- [ ] Embedded mode regression test (host app's a11y tree not disrupted)

---

## Risk Register

| Risk | Severity | Mitigation |
|------|----------|------------|
| `accesskit_android` Rust crate is v0.4.1 -- early, API may change | Medium | Pin exact version, vendor the crate |
| `accesskit-c` has no Android build target yet -- we must add it | High | Follow existing CMake cross-compile patterns for Linux ARM |
| JNI bridge complexity (Rust → C → JNI → Java → Android a11y) | High | Minimize JNI surface; let AccessKit handle most of it |
| Embedded mode conflict with host app's a11y tree | Medium | Scope AccessKit adapter to GodotView only |
| ThorVG Lottie expressions (JS execution) on Android | Low | Disable by default; most UI anims don't use expressions |
| Godot 4.6.2-rc2 is a release candidate -- may have its own bugs | Low | Monitor upstream for rc3 or stable; rebase if needed |

---

## Prerequisites & Toolchain

```
# System
Ubuntu 22.04+ or macOS 13+

# Android
Android SDK: API 34+
Android NDK: r26c+
CMake: 3.22+
Gradle: 8.x (via wrapper)

# Rust (for accesskit-c Android build)
rustup target add aarch64-linux-android
rustup target add armv7-linux-androideabi
rustup target add x86_64-linux-android
cargo-ndk (for NDK-aware cargo builds)

# Godot build
SCons 4.x
Python 3.10+
JDK 17 (for Android Gradle)
```

---

## Decision: Why AccessKit Over Direct Android Implementation

The team chose AccessKit over a direct `AccessibilityNodeProvider` Java implementation for these reasons:

1. **Cross-platform consistency**: The same accessibility tree model used on Windows/macOS/Linux now extends to Android. One API surface for game developers.

2. **Upstream alignment**: Godot upstream already uses AccessKit. Our fork stays mergeable with upstream when they eventually add Android support themselves.

3. **AccessKit maturity**: `accesskit_android` v0.4.1 handles the hard parts -- mapping the AccessKit tree model to Android's `AccessibilityNodeInfo`, handling `AccessibilityEvent` dispatch, managing focus and live regions. Writing this from scratch would be 3-4 months of work.

4. **Embedded mode**: AccessKit's adapter model (push-based tree) is architecturally well-suited to the SurfaceView-based rendering Godot uses on Android. It doesn't require the View hierarchy that a direct implementation would.

---

## Next Steps

To begin execution, we need to:

1. Set up the Docker build environment for cross-compiling Godot + Rust for Android
2. Start with Phase 1 (ThorVG Lottie) as it's lower risk and validates the build pipeline
3. In parallel, build `accesskit-c` for Android targets to unblock Phase 2
