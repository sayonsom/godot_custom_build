extends Camera3D

# ─────────────────────────────────────────────────────────────────────────────
# Orbit camera — SmartThings-style isometric map controls
#   Desktop : right-drag to rotate, scroll wheel to zoom, middle-drag to pan
#   Android : 1-finger drag to rotate, 2-finger pinch to zoom,
#             2-finger drag to pan
# ─────────────────────────────────────────────────────────────────────────────

const ZOOM_MIN    := 6.0
const ZOOM_MAX    := 45.0
const ROT_SPEED   := 0.35   # deg per screen-pixel
const PAN_SPEED   := 0.018  # world-units per screen-pixel per unit of distance
const ZOOM_PINCH  := 0.04   # zoom fraction per pixel of pinch delta
const ZOOM_WHEEL  := 2.5    # zoom units per wheel click

# State
var yaw      : float = -30.0  # horizontal rotation (deg)
var pitch    : float = 52.0   # elevation angle (deg, 20=low, 88=top-down)
var distance : float = 26.0   # distance from target
var target   : Vector3 = Vector3(0.0, 0.0, -9.0)   # look-at pivot (house centre)

# Touch tracking
var _t       : Dictionary = {}   # index -> Vector2
var _prev_pd : float = 0.0       # prev pinch distance
var _prev_mp : Vector2 = Vector2.ZERO

func _ready() -> void:
	_apply()

# ── Public API ────────────────────────────────────────────────────────────────
func reset_view() -> void:
	yaw = -30.0; pitch = 52.0; distance = 26.0; target = Vector3(0, 0, -9)
	_apply()

func set_top_down(enable: bool) -> void:
	pitch = 85.0 if enable else 55.0
	_apply()

# ── Input ─────────────────────────────────────────────────────────────────────
func _unhandled_input(event: InputEvent) -> void:
	# ── Desktop mouse ──────────────────────────────────────────────────────────
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			distance = clamp(distance - ZOOM_WHEEL, ZOOM_MIN, ZOOM_MAX)
			_apply()
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			distance = clamp(distance + ZOOM_WHEEL, ZOOM_MIN, ZOOM_MAX)
			_apply()

	elif event is InputEventMouseMotion:
		var mask: int = int(event.button_mask)
		if (mask & MOUSE_BUTTON_MASK_LEFT) or (mask & MOUSE_BUTTON_MASK_RIGHT):
			# Left OR right drag → rotate  (emulator sends left-drag)
			yaw   -= event.relative.x * ROT_SPEED
			pitch  = clamp(pitch - event.relative.y * ROT_SPEED * 0.5,
					20.0, 88.0)
			_apply()
		elif mask & MOUSE_BUTTON_MASK_MIDDLE:
			# Pan
			_pan_by(event.relative)

	# ── Touch ──────────────────────────────────────────────────────────────────
	elif event is InputEventScreenTouch:
		if event.pressed:
			_t[event.index] = event.position
		else:
			_t.erase(event.index)
			_prev_pd = 0.0

	elif event is InputEventScreenDrag:
		_t[event.index] = event.position

		if _t.size() == 1:
			yaw  -= event.relative.x * ROT_SPEED
			pitch = clamp(pitch - event.relative.y * ROT_SPEED * 0.4,
					20.0, 88.0)
			_apply()

		elif _t.size() >= 2:
			var keys := _t.keys()
			var p1: Vector2 = _t[keys[0]]
			var p2: Vector2 = _t[keys[1]]
			var pd  := p1.distance_to(p2)
			var mp  := (p1 + p2) * 0.5

			if _prev_pd > 0.0:
				# Pinch zoom
				distance = clamp(
					distance + (_prev_pd - pd) * ZOOM_PINCH,
					ZOOM_MIN, ZOOM_MAX)
				# Two-finger pan
				_pan_by(mp - _prev_mp)
				_apply()

			_prev_pd = pd
			_prev_mp = mp

# ── Helpers ───────────────────────────────────────────────────────────────────
func _pan_by(screen_delta: Vector2) -> void:
	var yaw_rad := deg_to_rad(yaw)
	var right := Vector3( cos(yaw_rad), 0.0, -sin(yaw_rad))
	var fwd   := Vector3(-sin(yaw_rad), 0.0, -cos(yaw_rad))
	var scale := PAN_SPEED * distance
	target -= right * screen_delta.x * scale
	target += fwd   * screen_delta.y * scale
	_apply()

func _apply() -> void:
	var pr  := deg_to_rad(pitch)
	var yr  := deg_to_rad(yaw)
	position = target + Vector3(
		cos(pr) * sin(yr),
		sin(pr),
		cos(pr) * cos(yr)
	) * distance
	look_at(target, Vector3.UP)
