# Building the AAR from Source

**Repo:** https://github.com/sayonsom/godot_custom_build

This guide produces `godot-lib.template_release.aar` with AccessKit TalkBack + Lottie baked in.

---

## Prerequisites

### 1. Operating System
- **macOS** (Apple Silicon or Intel) or **Linux** (Ubuntu 22.04+)
- Windows: use WSL2 with Ubuntu

### 2. Android SDK + NDK
```bash
# Install Android Studio or just the command-line tools
# https://developer.android.com/studio#command-tools

# Set environment variable (adjust path to your install)
export ANDROID_HOME=$HOME/Library/Android/sdk   # macOS
# export ANDROID_HOME=$HOME/Android/Sdk          # Linux

# Install required SDK components
$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager \
  "platform-tools" \
  "platforms;android-34" \
  "build-tools;34.0.0" \
  "ndk;26.3.11075529"
```

### 3. JDK 17
```bash
# macOS
brew install openjdk@17
export JAVA_HOME=/opt/homebrew/opt/openjdk@17

# Ubuntu
sudo apt install openjdk-17-jdk
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
```

### 4. SCons Build System
```bash
# macOS
brew install scons

# Ubuntu / pip
pip3 install scons
```

### 5. Python 3.10+
```bash
python3 --version  # must be 3.10 or newer
```

---

## Build Steps

### Step 1: Clone the repo
```bash
git clone https://github.com/sayonsom/godot_custom_build.git
cd godot_custom_build
```

### Step 2: Build native .so for each ABI

```bash
# ARM64 (required — covers 95%+ of modern Android devices)
scons platform=android target=template_release arch=arm64 \
  accesskit=yes module_lottie_enabled=yes disable_path_overrides=no \
  -j$(nproc)

# ARM32 (optional — older 32-bit devices)
scons platform=android target=template_release arch=arm32 \
  accesskit=yes module_lottie_enabled=yes disable_path_overrides=no \
  -j$(nproc)

# x86_64 (optional — emulators and Chromebooks)
scons platform=android target=template_release arch=x86_64 \
  accesskit=yes module_lottie_enabled=yes disable_path_overrides=no \
  -j$(nproc)
```

> **macOS note:** Replace `$(nproc)` with `$(sysctl -n hw.logicalcpu)`.
>
> **ANDROID_HOME not found?** Prefix each scons command with:
> `ANDROID_HOME=/path/to/your/sdk scons ...`

Each command produces a `.so` file that gets automatically placed in:
```
platform/android/java/lib/libs/release/<abi>/libgodot_android.so
```

### Step 3: Package the AAR with Gradle

```bash
cd platform/android/java
./gradlew generateGodotTemplates
```

### Step 4: Grab the AAR

The output is at:
```
platform/android/java/lib/build/outputs/aar/godot-lib.template_release.aar
```

Copy it wherever you need:
```bash
cp lib/build/outputs/aar/godot-lib.template_release.aar ~/Desktop/godot-lib.aar
```

---

## Quick One-Liner (ARM64 only)

```bash
git clone https://github.com/sayonsom/godot_custom_build.git && \
cd godot_custom_build && \
scons platform=android target=template_release arch=arm64 \
  accesskit=yes module_lottie_enabled=yes disable_path_overrides=no \
  -j$(nproc) && \
cd platform/android/java && \
./gradlew generateGodotTemplates && \
echo "AAR ready at: $(pwd)/lib/build/outputs/aar/godot-lib.template_release.aar"
```

---

## Build Flags Reference

| Flag | Default | Description |
|------|---------|-------------|
| `platform=android` | — | Target platform (required) |
| `target=template_release` | — | Release build (optimized, no editor) |
| `arch=arm64` | — | Target ABI: `arm64`, `arm32`, or `x86_64` |
| `accesskit=yes` | `yes` | Enable AccessKit TalkBack accessibility |
| `module_lottie_enabled=yes` | `yes` | Enable ThorVG Lottie animation support |
| `disable_path_overrides=no` | `yes` | Keep CLI path arguments (needed for `--main-pack`) |
| `-j$(nproc)` | `1` | Parallel compilation jobs |

---

## Troubleshooting

### "Invalid target platform android"
`ANDROID_HOME` is not set or the NDK is missing. Run:
```bash
export ANDROID_HOME=$HOME/Library/Android/sdk  # or your path
ls $ANDROID_HOME/ndk/  # should list NDK version directories
```

### Gradle fails with "SDK location not found"
Create `platform/android/java/local.properties`:
```
sdk.dir=/path/to/your/Android/sdk
```

### "No module named SCons"
SCons is installed but not in PATH. Try:
```bash
/opt/homebrew/bin/scons ...   # macOS Homebrew
~/.local/bin/scons ...        # pip install
```

### Build succeeds but AAR doesn't contain all ABIs
Each `scons` command builds one ABI. Run all three (arm64, arm32, x86_64) before running Gradle. Gradle bundles whichever `.so` files it finds in `lib/libs/release/<abi>/`.

### AAR is ~72MB — is that normal?
Yes. It contains the full Godot runtime with AccessKit, ThorVG (SVG + Lottie), and Vulkan/OpenGL renderers for up to 3 ABIs.
