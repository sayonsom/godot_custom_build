extends Camera3D

const ZOOM_MIN := 9.0
const ZOOM_MAX := 40.0
const ROT_SPEED := 0.34
const PAN_SPEED := 0.016
const ZOOM_PINCH := 0.02
const ZOOM_WHEEL := 2.0
const YAW_MIN := -78.0
const YAW_MAX := 34.0
const PITCH_MIN := 60.0
const PITCH_MAX := 76.0
const TARGET_BOUNDS := Rect2(-6.9, -6.9, 13.8, 13.8)

var yaw := -33.0
var pitch := 66.0
var distance := 31.0
var target := Vector3(0.0, 0.0, 0.0)

var _touches: Dictionary = {}
var _previous_pinch_distance := 0.0
var _previous_twist_angle := 0.0

@onready var placement_controller: RuntimePlacementController3D = $"../PlacementController"

func _ready() -> void:
	_apply()

func reset_view() -> void:
	yaw = -33.0
	pitch = 66.0
	distance = 31.0
	target = Vector3(0.0, 0.0, 0.0)
	_apply()

func focus_floor(floor_target: Vector3) -> void:
	target = floor_target
	distance = clamp(distance, 26.0, ZOOM_MAX)
	_apply()

func _unhandled_input(event: InputEvent) -> void:
	if get_viewport().gui_is_dragging():
		_reset_touch_gesture()
		return

	if _is_object_interaction_active():
		_reset_touch_gesture()
		return

	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			distance -= ZOOM_WHEEL
			_apply()
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			distance += ZOOM_WHEEL
			_apply()

	elif event is InputEventMouseMotion:
		var mask: int = int(event.button_mask)
		if mask & MOUSE_BUTTON_MASK_RIGHT:
			yaw -= event.relative.x * ROT_SPEED
			pitch -= event.relative.y * ROT_SPEED * 0.45
			_apply()
		elif mask & MOUSE_BUTTON_MASK_MIDDLE:
			_pan_by(event.relative)

	elif event is InputEventScreenTouch:
		if event.pressed:
			_touches[event.index] = event.position
		else:
			_touches.erase(event.index)
			_previous_pinch_distance = 0.0
			_previous_twist_angle = 0.0

	elif event is InputEventScreenDrag:
		_touches[event.index] = event.position

		if _is_camera_locked_by_selection():
			_previous_pinch_distance = 0.0
			if _touches.size() >= 2:
				var keys := _touches.keys()
				var p1: Vector2 = _touches[keys[0]]
				var p2: Vector2 = _touches[keys[1]]
				var twist_angle := (p2 - p1).angle()
				if _previous_twist_angle != 0.0 and placement_controller != null:
					var delta_rad := wrapf(twist_angle - _previous_twist_angle, -PI, PI)
					placement_controller.rotate_selected(rad_to_deg(delta_rad))
				_previous_twist_angle = twist_angle
			else:
				_previous_twist_angle = 0.0
			return

		if _touches.size() == 1:
			yaw -= event.relative.x * ROT_SPEED
			pitch -= event.relative.y * ROT_SPEED * 0.35
			_apply()
		elif _touches.size() >= 2:
			var keys := _touches.keys()
			var p1: Vector2 = _touches[keys[0]]
			var p2: Vector2 = _touches[keys[1]]
			var pinch_distance := p1.distance_to(p2)

			if _previous_pinch_distance > 0.0:
				distance += (_previous_pinch_distance - pinch_distance) * ZOOM_PINCH
				_apply()

			_previous_pinch_distance = pinch_distance

func _is_object_interaction_active() -> bool:
	return placement_controller != null and (placement_controller.is_dragging_selected() or placement_controller.is_rotating_selected())

func _is_camera_locked_by_selection() -> bool:
	return placement_controller != null and placement_controller.get_selected_target() != null

func _reset_touch_gesture() -> void:
	_touches.clear()
	_previous_pinch_distance = 0.0
	_previous_twist_angle = 0.0

func _pan_by(screen_delta: Vector2) -> void:
	var yaw_rad := deg_to_rad(yaw)
	var right := Vector3(cos(yaw_rad), 0.0, -sin(yaw_rad))
	var forward := Vector3(-sin(yaw_rad), 0.0, -cos(yaw_rad))
	var scale := PAN_SPEED * distance
	target -= right * screen_delta.x * scale
	target += forward * screen_delta.y * scale
	_apply()

func _apply() -> void:
	_clamp_view()
	var pitch_rad := deg_to_rad(pitch)
	var yaw_rad := deg_to_rad(yaw)
	position = target + Vector3(
		cos(pitch_rad) * sin(yaw_rad),
		sin(pitch_rad),
		cos(pitch_rad) * cos(yaw_rad)
	) * distance
	look_at(target, Vector3.UP)

func _clamp_view() -> void:
	yaw = clamp(yaw, YAW_MIN, YAW_MAX)
	pitch = clamp(pitch, PITCH_MIN, PITCH_MAX)
	distance = clamp(distance, ZOOM_MIN, ZOOM_MAX)
	target.x = clamp(target.x, TARGET_BOUNDS.position.x, TARGET_BOUNDS.position.x + TARGET_BOUNDS.size.x)
	target.z = clamp(target.z, TARGET_BOUNDS.position.y, TARGET_BOUNDS.position.y + TARGET_BOUNDS.size.y)
