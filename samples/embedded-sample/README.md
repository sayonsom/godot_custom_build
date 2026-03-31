# Godot Embedded Sample — Lottie + AccessKit TalkBack

This sample demonstrates integrating the custom Godot 4.6.1 AAR (with Lottie animation and AccessKit Android TalkBack support) into a native Android app.

## Two Modes

### 1. Standalone Mode (`StandaloneActivity`)
Godot takes over the entire Activity. This is the default launcher activity.

### 2. Embedded Mode (`EmbeddedActivity`)
Godot runs inside a `GodotFragment` alongside native Android views. Native buttons sit above and below the Godot fragment. TalkBack should navigate seamlessly between native and Godot controls.

## Setup

1. Place `godot-lib-4.6.1-custom.aar` in `app/libs/`
2. Place your Godot project files in `app/src/main/assets/`
3. Open in Android Studio and run

## Testing TalkBack

1. Enable TalkBack: Settings > Accessibility > TalkBack
2. Launch the app
3. Swipe right to navigate between elements
4. Verify: Labels, buttons, and text fields are announced
5. Double-tap to activate buttons
6. In embedded mode, verify focus transitions between native and Godot views

## What This Tests

| Feature | How to Test |
|---------|-------------|
| Lottie playback | Animation should auto-play on launch |
| Play/Pause/Stop | Tap the control buttons |
| TalkBack navigation | Swipe right through all controls |
| Button activation | Double-tap with TalkBack active |
| Text input | Tap LineEdit, type with TalkBack |
| Counter (state) | Increment/decrement, verify TalkBack reads updates |
| Embedded focus | In embedded mode, swipe from native button into Godot and back |
