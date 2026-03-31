extends Control

# ─────────────────────────────────────────────────────────────────────────────
# 2D floor plan – flat top-down view matching room layout in main.gd
# ─────────────────────────────────────────────────────────────────────────────

const ROOMS: Array = [
	["Media room",    Rect2(-12, -9,  6, 6), Color(0.72, 0.83, 0.94)],
	["Living room",   Rect2(-6,  -9,  6, 6), Color(0.78, 0.92, 0.78)],
	["Kitchen",       Rect2( 0,  -9,  6, 6), Color(0.94, 0.92, 0.76)],
	["Primary suite", Rect2( 6,  -9,  6, 6), Color(0.78, 0.92, 0.86)],
	["Bedroom",       Rect2(-12,-15,  6, 6), Color(0.94, 0.80, 0.88)],
	["Porch",         Rect2(-6, -15,  6, 6), Color(0.94, 0.90, 0.76)],
	["Dining room",   Rect2( 0, -15,  6, 6), Color(0.80, 0.92, 0.96)],
	["Laundry",       Rect2( 6, -15,  3, 6), Color(0.78, 0.92, 0.86)],
	["Bathroom",      Rect2( 9, -15,  3, 6), Color(0.80, 0.94, 0.90)],
]

# World bounds: x in [-12..12], z in [-15..-3]
const WORLD_X0 := -12.0
const WORLD_Z0 := -15.0
const WORLD_W  := 24.0
const WORLD_D  := 12.0
const MARGIN   := 40.0

var selected := -1

func set_selected(i: int) -> void:
	selected = i
	queue_redraw()

func _draw() -> void:
	var sx := (size.x - MARGIN * 2.0) / WORLD_W
	var sy := (size.y - MARGIN * 2.0) / WORLD_D

	for i in ROOMS.size():
		var info: Array  = ROOMS[i]
		var rname: String = info[0]
		var rm: Rect2    = info[1]
		var col: Color   = info[2]

		var px := MARGIN + (rm.position.x - WORLD_X0) * sx
		var py := MARGIN + (rm.position.y - WORLD_Z0) * sy  # z→y
		var pw := rm.size.x * sx
		var ph := rm.size.y * sy
		var prect := Rect2(px, py, pw, ph)

		# Fill
		draw_rect(prect, col, true)

		# Selection highlight
		if i == selected:
			draw_rect(prect, Color(0.2, 0.5, 1.0, 0.3), true)
			draw_rect(prect, Color(0.1, 0.3, 0.9, 1.0), false, 3.0)
		else:
			draw_rect(prect, Color(0.5, 0.5, 0.6, 0.7), false, 1.5)

		# Room name
		var font      := ThemeDB.fallback_font
		var fsz       := 13
		var ts        := font.get_string_size(rname,
				HORIZONTAL_ALIGNMENT_LEFT, -1, fsz)
		var tx        := px + pw * 0.5 - ts.x * 0.5
		var ty        := py + ph * 0.5 + fsz * 0.4
		draw_string(font, Vector2(tx, ty), rname,
				HORIZONTAL_ALIGNMENT_LEFT, -1, fsz,
				Color(0.15, 0.15, 0.35, 1))

		# Device dot markers
		var ndev := 3 if i < 4 else 2
		for d in ndev:
			var dt := (float(d) + 0.5) / float(ndev)
			var dx := px + pw * (0.15 + dt * 0.7)
			var dy := py + ph * 0.25
			draw_circle(Vector2(dx, dy), 5.0, Color(0.2, 0.5, 0.95))

	# Legend title
	var font := ThemeDB.fallback_font
	draw_string(font, Vector2(MARGIN, MARGIN - 12),
			"Floor Plan  –  2D view",
			HORIZONTAL_ALIGNMENT_LEFT, -1, 15,
			Color(0.2, 0.2, 0.5))

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		var sx := (size.x - MARGIN * 2.0) / WORLD_W
		var sy := (size.y - MARGIN * 2.0) / WORLD_D
		for i in ROOMS.size():
			var rm: Rect2 = ROOMS[i][1]
			var px := MARGIN + (rm.position.x - WORLD_X0) * sx
			var py := MARGIN + (rm.position.y - WORLD_Z0) * sy
			var prect := Rect2(px, py, rm.size.x * sx, rm.size.y * sy)
			if prect.has_point(event.position):
				selected = i
				queue_redraw()
				break
