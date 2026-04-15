# Runtime Placement API for Existing GLTF/GLB Projects

This is the engine-level integration path. It does not depend on `house_demo` or `sample-host`.

## 1) Godot Scene Wiring

1. Add `RuntimePlacementController3D` to your scene.
2. Set:
   - `camera_path` to your runtime `Camera3D`.
   - `targets_root_path` to the `Node3D` that owns movable furniture wrappers.
3. Keep each movable item as a direct `Node3D` child under `targets_root_path`.

Example:

```gdscript
@onready var placement: RuntimePlacementController3D = $RuntimePlacementController3D
@onready var camera_3d: Camera3D = $World/Camera3D
@onready var placed_assets: Node3D = $World/PlacedAssets

func _ready() -> void:
	placement.camera_path = placement.get_path_to(camera_3d)
	placement.targets_root_path = placement.get_path_to(placed_assets)
	placement.outline_thickness = 0.13
	placement.require_double_tap_to_move = true
	placement.clear_selection_on_empty_tap = true
	placement.move_guides_enabled = true
```

## 2) Core Engine API

Selection/picking:

1. `pick_target_at_screen_position(screen_pos: Vector2) -> Node3D`
2. `select_target(target: Node3D)`
3. `select_target_at_screen_position(screen_pos: Vector2) -> bool`
4. `clear_selection()`

Movement/rotation:

1. `move_selected_to_screen_position(screen_pos: Vector2) -> bool`
2. `move_selected_to_world_position(world_pos: Vector3) -> bool`
3. `rotate_selected(delta_degrees: float)`
4. `flip_selected_x()`
5. `flip_selected_z()`
6. `arm_move_mode()`, `disarm_move_mode()`, `is_move_mode_armed()`

Placement helpers:

1. `project_screen_to_floor(screen_pos: Vector2) -> Vector3 | null`
2. `place_target_on_floor(target: Node3D, world_pos: Vector3) -> bool`

Behavior toggles:

1. `interaction_enabled`
2. `selection_enabled`
3. `translation_enabled`
4. `rotation_enabled`
5. `require_double_tap_to_move`
6. `clear_selection_on_empty_tap`
7. `move_guides_enabled`
8. `drag_bounds_enabled`
9. `drag_bounds`

Signals:

1. `selection_changed(target: Node3D)`
2. `target_transform_changed(target: Node3D)`
3. `move_mode_changed(armed: bool)`

Direct boundary APIs (engine-level, no sample dependency):

1. `create_boundary(object: Node, global_transform: Transform3D, mesh_instance: MeshInstance3D, mesh_aabb: AABB) -> bool`
2. `update_boundary(object: Node, transform: Transform3D, mesh_instance: MeshInstance3D, is_valid: bool) -> bool`
3. `update_color(is_valid: bool)` (`true` = blue, `false` = red)
4. `delete_boundary()`

Compatibility aliases for existing typo-based integrations:

1. `create_boundry(...)`
2. `update_boundry(...)`
3. `delete_boundry()`

## 3) Spawn GLTF/GLB and Place on Floor

```gdscript
func spawn_furniture(scene_path: String, wrapper_name: String, drop_world: Vector3) -> Node3D:
	var packed: PackedScene = load(scene_path)
	var model: Node3D = packed.instantiate()
	var wrapper := Node3D.new()
	wrapper.name = wrapper_name
	$World/PlacedAssets.add_child(wrapper)
	wrapper.add_child(model)
	$RuntimePlacementController3D.place_target_on_floor(wrapper, drop_world)
	return wrapper
```

## 4) Android Native Host API (From AAR)

The Godot Android library now exposes reusable API classes:

1. `org.godotengine.godot.runtime3dtools.RuntimePlacementPluginHost`
2. `org.godotengine.godot.runtime3dtools.RuntimePlacementController`
3. `org.godotengine.godot.runtime3dtools.RuntimePlacementBridgePlugin`

These replace sample-app-scoped `com.test.*` bridge classes.

## 5) Native Bridge Contract for Existing Projects

Use command/event messaging between Android host and your Godot scene.

1. Add a bridge node/script to your project (template file):
   - `modules/runtime_3d_tools/examples/runtime_placement_native_bridge.gd`
2. Set `controller_path` in that script to your `RuntimePlacementController3D`.
3. Keep bridge singleton name as `RuntimePlacementBridge` (default).

Android sends commands such as:

1. `request_state`
2. `select_at_screen`
3. `move_selected_to_screen`
4. `move_selected_to_world`
5. `rotate_selected`
6. `flip_selected_x`
7. `flip_selected_z`
8. `arm_move_mode`
9. `disarm_move_mode`
10. `set_outline_thickness`
11. `set_require_double_tap_to_move`
12. `set_drag_bounds`
13. `set_drag_bounds_enabled`

Godot sends events back using `onGodotEvent(event, payload_json)`:

1. `state`
2. `command_result`
