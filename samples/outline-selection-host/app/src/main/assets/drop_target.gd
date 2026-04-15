extends Control

signal asset_dropped(asset_id: String, screen_position: Vector2)

var _highlight_drop := false

func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_PASS

func set_drag_feedback(active: bool) -> void:
	if _highlight_drop == active:
		return
	_highlight_drop = active
	queue_redraw()

func _can_drop_data(_at_position: Vector2, data: Variant) -> bool:
	set_drag_feedback(data is Dictionary and data.has("asset_id"))
	return _highlight_drop

func _drop_data(at_position: Vector2, data: Variant) -> void:
	set_drag_feedback(false)
	if data is Dictionary and data.has("asset_id"):
		asset_dropped.emit(String(data["asset_id"]), get_global_rect().position + at_position)

func _notification(what: int) -> void:
	if what == NOTIFICATION_DRAG_END:
		set_drag_feedback(false)

func _draw() -> void:
	if !_highlight_drop:
		return

	var rect := Rect2(Vector2.ZERO, size).grow(-10.0)
	draw_rect(rect, Color(0.17, 0.47, 1.0, 0.08), true)
	draw_rect(rect, Color(0.17, 0.47, 1.0, 0.85), false, 3.0)
