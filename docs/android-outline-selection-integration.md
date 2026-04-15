# Android Native Integration Guide (Engine API, No Sample App Dependency)

This guide is for Android engineers embedding your existing Godot project.

It uses engine-level APIs shipped in the AAR and does not require copying `sample-host` classes.

## 1. What Is Included in the AAR

1. `RuntimePlacementController3D` engine node (selection, movement, rotation, outline).
2. Native host helper APIs:
   - `org.godotengine.godot.runtime3dtools.RuntimePlacementPluginHost`
   - `org.godotengine.godot.runtime3dtools.RuntimePlacementController`
   - `org.godotengine.godot.runtime3dtools.RuntimePlacementBridgePlugin`

## 2. Android App Setup

In your app module (`app/build.gradle.kts`):

```kotlin
android {
	defaultConfig {
		minSdk = 24
		targetSdk = 35
	}
	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_17
		targetCompatibility = JavaVersion.VERSION_17
	}
	kotlinOptions {
		jvmTarget = "17"
	}
	packaging {
		jniLibs {
			pickFirsts += "**/*.so"
		}
	}
}

dependencies {
	implementation(files("libs/godot-lib.template_release.aar"))
	implementation("androidx.fragment:fragment-ktx:1.8.6")
}
```

## 3. Embedded Host Activity (Minimal)

```kotlin
package com.yourapp.godot

import android.os.Bundle
import androidx.fragment.app.FragmentActivity
import org.godotengine.godot.Godot
import org.godotengine.godot.GodotFragment
import org.godotengine.godot.GodotHost
import org.godotengine.godot.plugin.GodotPlugin
import org.godotengine.godot.runtime3dtools.RuntimePlacementPluginHost

class RuntimePlacementActivity : FragmentActivity(), GodotHost {

	private var godotFragment: GodotFragment? = null
	private val placementHost = RuntimePlacementPluginHost()

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)
		val container = android.widget.FrameLayout(this).apply {
			id = android.R.id.content + 100
		}
		setContentView(container)
		godotFragment = if (savedInstanceState == null) {
			GodotFragment().also {
				supportFragmentManager.beginTransaction().replace(container.id, it).commitNow()
			}
		} else {
			supportFragmentManager.findFragmentById(container.id) as? GodotFragment
		}
	}

	override fun getActivity() = this
	override fun getGodot() = godotFragment?.godot

	override fun getHostPlugins(engine: Godot): Set<GodotPlugin> {
		return placementHost.createHostPlugins(engine)
	}

	override fun onDestroy() {
		placementHost.dispose()
		super.onDestroy()
	}
}
```

## 4. Godot Project Setup (Your Existing Scene)

1. Add `RuntimePlacementController3D` node to your scene.
2. Set `camera_path` and `targets_root_path`.
3. Add a bridge script node so native commands are mapped to controller APIs.
4. Use this template file:
   - `modules/runtime_3d_tools/examples/runtime_placement_native_bridge.gd`
5. In that script, set `controller_path` to your `RuntimePlacementController3D`.

## 5. Native Calls from Android

```kotlin
val placement = placementHost.controller

placement.requestState()
placement.setOutlineThickness(0.13)
placement.setRequireDoubleTapToMove(true)
placement.selectAtScreen(screenX = 540.0, screenY = 1200.0)
placement.armMoveMode()
placement.moveSelectedToScreen(screenX = 620.0, screenY = 1320.0)
placement.rotateSelected(15.0)
placement.disarmMoveMode()
```

Listen for state/events:

```kotlin
placement.addListener(object : org.godotengine.godot.runtime3dtools.RuntimePlacementController.Listener {
	override fun onStateChanged(state: org.json.JSONObject) {
		// has_selection, selected_path, move_mode_armed, dragging, rotating, reason
	}
	override fun onEvent(event: String, payload: org.json.JSONObject) {
		// state, command_result, error
	}
})
```

## 6. Command Contract (Bridge Level)

From Android to Godot:

1. `request_state`
2. `clear_selection`
3. `select_at_screen`
4. `move_selected_to_screen`
5. `move_selected_to_world`
6. `rotate_selected`
7. `flip_selected_x`
8. `flip_selected_z`
9. `arm_move_mode`
10. `disarm_move_mode`
11. `place_target_on_floor`
12. `set_outline_thickness`
13. `set_require_double_tap_to_move`
14. `set_clear_selection_on_empty_tap`
15. `set_interaction_enabled`
16. `set_selection_enabled`
17. `set_translation_enabled`
18. `set_rotation_enabled`
19. `set_move_guides_enabled`
20. `set_drag_bounds_enabled`
21. `set_drag_bounds`

From Godot to Android:

1. `state`
2. `command_result`

Engine direct boundary APIs exposed on `RuntimePlacementController3D`:

1. `create_boundary(object, global_transform, mesh_instance, mesh_aabb)`
2. `update_boundary(object, transform, mesh_instance, is_valid)`
3. `update_color(is_valid)` (`true` blue, `false` red)
4. `delete_boundary()`

## 7. Smoke Test (Must Pass)

1. App launches without black screen.
2. Single tap selects furniture with blue body outline.
3. Pinch/rotate on selected furniture rotates furniture only.
4. Double tap (or `armMoveMode`) enables movement mode.
5. Drag moves furniture on floor.
6. Tapping outside clears selection when enabled.
7. Native API `setOutlineThickness(0.13)` updates visual thickness.
