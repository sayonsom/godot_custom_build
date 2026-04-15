extends PanelContainer

signal drag_requested(asset_id: String, display_name: String, accent_color: Color, screen_position: Vector2, pointer_id: int, is_touch: bool)

@export var asset_id := ""
@export var display_name := ""
@export var subtitle := ""
@export var accent_color := Color(0.3, 0.5, 0.9)

var _touch_pointer := -1
var _mouse_pressed := false

func _ready() -> void:
	custom_minimum_size = Vector2(220.0, 88.0)
	mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
	mouse_filter = Control.MOUSE_FILTER_STOP

	var background := StyleBoxFlat.new()
	background.bg_color = Color(1, 1, 1, 0.96)
	background.border_width_left = 2
	background.border_width_top = 2
	background.border_width_right = 2
	background.border_width_bottom = 2
	background.border_color = accent_color
	background.corner_radius_top_left = 14
	background.corner_radius_top_right = 14
	background.corner_radius_bottom_right = 14
	background.corner_radius_bottom_left = 14
	add_theme_stylebox_override("panel", background)

	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 14)
	margin.add_theme_constant_override("margin_top", 12)
	margin.add_theme_constant_override("margin_right", 14)
	margin.add_theme_constant_override("margin_bottom", 12)
	add_child(margin)

	var vbox := VBoxContainer.new()
	vbox.add_theme_constant_override("separation", 4)
	margin.add_child(vbox)

	var name_label := Label.new()
	name_label.text = display_name
	name_label.add_theme_font_size_override("font_size", 18)
	name_label.modulate = Color(0.12, 0.16, 0.24)
	vbox.add_child(name_label)

	var subtitle_label := Label.new()
	subtitle_label.text = "%s  |  drag onto the floor" % subtitle
	subtitle_label.add_theme_font_size_override("font_size", 13)
	subtitle_label.modulate = Color(0.34, 0.40, 0.50, 0.95)
	vbox.add_child(subtitle_label)

func _gui_input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		var screen_touch := event as InputEventScreenTouch
		if screen_touch.pressed:
			_touch_pointer = screen_touch.index
			drag_requested.emit(asset_id, display_name, accent_color, get_global_rect().position + screen_touch.position, screen_touch.index, true)
		elif screen_touch.index == _touch_pointer:
			_touch_pointer = -1
		accept_event()
		return

	if event is InputEventScreenDrag:
		var screen_drag := event as InputEventScreenDrag
		if screen_drag.index != _touch_pointer:
			return
		accept_event()
		return

	if event is InputEventMouseButton:
		var mouse_button := event as InputEventMouseButton
		if mouse_button.button_index != MOUSE_BUTTON_LEFT:
			return

		if mouse_button.pressed:
			_mouse_pressed = true
			drag_requested.emit(asset_id, display_name, accent_color, get_global_rect().position + mouse_button.position, -1, false)
		else:
			_mouse_pressed = false
		accept_event()
		return

	if event is InputEventMouseMotion and _mouse_pressed:
		accept_event()
