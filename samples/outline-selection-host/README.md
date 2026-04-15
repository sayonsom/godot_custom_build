# Outline Selection Host Sample

This sample is the reference integration for the native Android API bridge.

## Use This When

1. You need a known-good baseline before integrating into your own app.
2. You need source files for the API bridge classes.

## Most Important Files

1. `app/src/main/java/com/test/godotdemo/api/FloorPlannerBridgePlugin.kt`
2. `app/src/main/java/com/test/godotdemo/api/FloorPlannerController.kt`
3. `app/src/main/java/com/test/godotdemo/api/FloorPlannerPluginHost.kt`
4. `app/src/main/assets/*` (Godot runtime assets)
5. `app/libs/godot-lib.template_release.aar`

## Build

```bash
./gradlew clean assembleDebug
```

APK:

`app/build/outputs/apk/debug/app-debug.apk`

For production integration steps, use:

`dist/outline-selection-android-handoff/INTEGRATION.md`
