extends Node

@export var controller_path: NodePath
@export var bridge_singleton_name: StringName = &"RuntimePlacementBridge"

@onready var controller: RuntimePlacementController3D = get_node_or_null(controller_path)

var _bridge: Object = null

func _ready() -> void:
	if controller == null:
		push_error("RuntimePlacementNativeBridge: controller_path is not set or invalid.")
		return

	if Engine.has_singleton(bridge_singleton_name):
		_bridge = Engine.get_singleton(bridge_singleton_name)

	if _bridge != null and _bridge.has_signal("command_requested"):
		var handler := Callable(self, "_on_bridge_command_requested")
		if !_bridge.is_connected("command_requested", handler):
			_bridge.connect("command_requested", handler)

	controller.selection_changed.connect(_on_controller_selection_changed)
	controller.target_transform_changed.connect(_on_controller_transform_changed)
	controller.move_mode_changed.connect(_on_controller_move_mode_changed)
	_emit_state("ready")

func _exit_tree() -> void:
	if _bridge != null and _bridge.has_signal("command_requested"):
		var handler := Callable(self, "_on_bridge_command_requested")
		if _bridge.is_connected("command_requested", handler):
			_bridge.disconnect("command_requested", handler)

func _on_bridge_command_requested(command: String, payload_json: String) -> void:
	var payload := _parse_payload(payload_json)
	var result := _execute_command(command, payload)
	_emit_event("command_result", {
		"command": command,
		"result": result
	})
	if bool(result.get("ok", false)):
		_emit_state("command:%s" % command)

func _execute_command(command: String, payload: Dictionary) -> Dictionary:
	if controller == null:
		return {
			"ok": false,
			"message": "controller is missing"
		}

	match command:
		"request_state":
			return {"ok": true}
		"clear_selection":
			controller.clear_selection()
			return {"ok": true}
		"select_at_screen":
			var selected := controller.select_target_at_screen_position(Vector2(_num(payload, "x"), _num(payload, "y")))
			return {"ok": selected}
		"move_selected_to_screen":
			var moved_screen := controller.move_selected_to_screen_position(Vector2(_num(payload, "x"), _num(payload, "y")))
			return {"ok": moved_screen}
		"move_selected_to_world":
			var moved_world := controller.move_selected_to_world_position(Vector3(_num(payload, "x"), _num(payload, "y"), _num(payload, "z")))
			return {"ok": moved_world}
		"rotate_selected":
			controller.rotate_selected(_num(payload, "delta_degrees"))
			return {"ok": true}
		"flip_selected_x":
			controller.flip_selected_x()
			return {"ok": true}
		"flip_selected_z":
			controller.flip_selected_z()
			return {"ok": true}
		"arm_move_mode":
			controller.arm_move_mode()
			return {"ok": true}
		"disarm_move_mode":
			controller.disarm_move_mode()
			return {"ok": true}
		"place_target_on_floor":
			var target_path := String(payload.get("target_path", ""))
			var target := get_node_or_null(target_path) as Node3D
			if target == null:
				return {
					"ok": false,
					"message": "target_path not found: %s" % target_path
				}
			var placed := controller.place_target_on_floor(
				target,
				Vector3(_num(payload, "x"), _num(payload, "y"), _num(payload, "z"))
			)
			return {"ok": placed}
		"set_outline_thickness":
			controller.outline_thickness = _num(payload, "value", controller.outline_thickness)
			return {"ok": true}
		"set_require_double_tap_to_move":
			controller.require_double_tap_to_move = _bool(payload, "enabled", controller.require_double_tap_to_move)
			return {"ok": true}
		"set_clear_selection_on_empty_tap":
			controller.clear_selection_on_empty_tap = _bool(payload, "enabled", controller.clear_selection_on_empty_tap)
			return {"ok": true}
		"set_interaction_enabled":
			controller.interaction_enabled = _bool(payload, "enabled", controller.interaction_enabled)
			return {"ok": true}
		"set_selection_enabled":
			controller.selection_enabled = _bool(payload, "enabled", controller.selection_enabled)
			return {"ok": true}
		"set_translation_enabled":
			controller.translation_enabled = _bool(payload, "enabled", controller.translation_enabled)
			return {"ok": true}
		"set_rotation_enabled":
			controller.rotation_enabled = _bool(payload, "enabled", controller.rotation_enabled)
			return {"ok": true}
		"set_move_guides_enabled":
			controller.move_guides_enabled = _bool(payload, "enabled", controller.move_guides_enabled)
			return {"ok": true}
		"set_drag_bounds_enabled":
			controller.drag_bounds_enabled = _bool(payload, "enabled", controller.drag_bounds_enabled)
			return {"ok": true}
		"set_drag_bounds":
			controller.drag_bounds = Rect2(
				_num(payload, "x"),
				_num(payload, "z"),
				_num(payload, "width"),
				_num(payload, "depth")
			)
			return {"ok": true}
		_:
			return {
				"ok": false,
				"message": "Unknown command: %s" % command
			}

func _on_controller_selection_changed(_target: Node3D) -> void:
	_emit_state("selection_changed")

func _on_controller_transform_changed(_target: Node3D) -> void:
	_emit_state("target_transform_changed")

func _on_controller_move_mode_changed(_armed: bool) -> void:
	_emit_state("move_mode_changed")

func _emit_state(reason: String) -> void:
	var selected := controller.get_selected_target() if controller != null else null
	var state := {
		"reason": reason,
		"has_selection": selected != null,
		"selected_path": String(selected.get_path()) if selected != null else "",
		"move_mode_armed": controller.is_move_mode_armed() if controller != null else false,
		"dragging": controller.is_dragging_selected() if controller != null else false,
		"rotating": controller.is_rotating_selected() if controller != null else false
	}
	_emit_event("state", state)

func _emit_event(event_name: String, payload: Dictionary) -> void:
	if _bridge == null or !_bridge.has_method("onGodotEvent"):
		return
	_bridge.call("onGodotEvent", event_name, JSON.stringify(payload))

func _parse_payload(payload_json: String) -> Dictionary:
	if payload_json.is_empty():
		return {}
	var parsed := JSON.parse_string(payload_json)
	return parsed if parsed is Dictionary else {}

func _num(payload: Dictionary, key: String, default_value: float = 0.0) -> float:
	return float(payload.get(key, default_value))

func _bool(payload: Dictionary, key: String, default_value: bool = false) -> bool:
	return bool(payload.get(key, default_value))
