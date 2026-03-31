extends Node

# ─────────────────────────────────────────────────────────────────────────────
# SmartThings-style house map demo
# ─────────────────────────────────────────────────────────────────────────────

# ── Room definitions ──────────────────────────────────────────────────────────
# Each entry: [name, Rect2(x, z, w, d) in world coords, Color, [device names]]
const ROOMS: Array = [
	["Media room",    Rect2(-12, -9,  6, 6), Color(0.72, 0.83, 0.94),
		["TV", "Speaker", "AC"]],
	["Living room",   Rect2(-6,  -9,  6, 6), Color(0.78, 0.92, 0.78),
		["Light", "Camera", "Thermostat"]],
	["Kitchen",       Rect2( 0,  -9,  6, 6), Color(0.94, 0.92, 0.76),
		["Oven", "Light", "Fridge"]],
	["Primary suite", Rect2( 6,  -9,  6, 6), Color(0.78, 0.92, 0.86),
		["Light", "AC"]],
	["Bedroom",       Rect2(-12,-15,  6, 6), Color(0.94, 0.80, 0.88),
		["Light", "Camera"]],
	["Porch",         Rect2(-6, -15,  6, 6), Color(0.94, 0.90, 0.76),
		["Light"]],
	["Dining room",   Rect2( 0, -15,  6, 6), Color(0.80, 0.92, 0.96),
		["Light", "AC"]],
	["Laundry",       Rect2( 6, -15,  3, 6), Color(0.78, 0.92, 0.86),
		["Washer", "Dryer"]],
	["Bathroom",      Rect2( 9, -15,  3, 6), Color(0.80, 0.94, 0.90),
		["Light", "Fan"]],
]

const WALL_H  := 0.6
const FLOOR_T := 0.08
const PIN_H   := 1.4    # pin height above floor

var is_3d_mode   := true
var selected_room := -1

# Node refs
@onready var cam:      Camera3D  = $Scene3D/Camera3D
@onready var scene_3d: Node3D    = $Scene3D
@onready var view_2d:  Control   = $UI/Root/View2D
@onready var btn_3d:   Button    = $UI/Root/BottomBar/BtnRow/Btn3D
@onready var btn_reset:Button    = $UI/Root/BottomBar/BtnRow/BtnReset
@onready var btn_floor:Button    = $UI/Root/BottomBar/BtnRow/BtnFloor
@onready var btn_dev:  Button    = $UI/Root/BottomBar/BtnRow/BtnDevices
@onready var orb:      Node      = $Scene3D/Camera3D  # orbit_camera.gd

func _ready() -> void:
	_build_house()
	_refresh_view()
	btn_3d.pressed.connect(_on_toggle_3d)
	btn_reset.pressed.connect(_on_reset)
	btn_floor.pressed.connect(_on_floor)
	btn_dev.pressed.connect(_on_devices)

# ── Build house ───────────────────────────────────────────────────────────────
func _build_house() -> void:
	var root := Node3D.new()
	root.name = "House"
	scene_3d.add_child(root)

	var mat_wall := StandardMaterial3D.new()
	mat_wall.albedo_color  = Color(0.96, 0.96, 0.96)
	mat_wall.roughness     = 0.9

	for i in ROOMS.size():
		var info: Array      = ROOMS[i]
		var rname: String    = info[0]
		var rect: Rect2      = info[1]
		var color: Color     = info[2]
		var devices: Array   = info[3]

		var cx := rect.position.x + rect.size.x * 0.5
		var cz := rect.position.y + rect.size.y * 0.5  # Rect2.y maps to world Z

		# Coloured floor slab
		var mat_floor := StandardMaterial3D.new()
		mat_floor.albedo_color = color
		mat_floor.roughness    = 0.85
		_add_box(root,
			Vector3(cx, 0.0, cz),
			Vector3(rect.size.x, FLOOR_T, rect.size.y),
			mat_floor, rname + "_Floor")

		# Thin walls on all four edges
		var hw := rect.size.x; var hd := rect.size.y
		# North / South
		_add_box(root, Vector3(cx, WALL_H*0.5, rect.position.y),
			Vector3(hw, WALL_H, 0.12), mat_wall, rname+"_WN")
		_add_box(root, Vector3(cx, WALL_H*0.5, rect.position.y + hd),
			Vector3(hw, WALL_H, 0.12), mat_wall, rname+"_WS")
		# West / East
		_add_box(root, Vector3(rect.position.x,      WALL_H*0.5, cz),
			Vector3(0.12, WALL_H, hd), mat_wall, rname+"_WW")
		_add_box(root, Vector3(rect.position.x + hw, WALL_H*0.5, cz),
			Vector3(0.12, WALL_H, hd), mat_wall, rname+"_WE")

		# Room label (Label3D floating, always faces camera)
		_add_label3d(root,
			Vector3(cx, 0.25, cz),
			rname, Color(0.12, 0.12, 0.30, 1), 52)

		# Device pins
		_add_device_pins(root, rect, devices)

	# Ambient fill light
	var ambient := OmniLight3D.new()
	ambient.light_energy = 0.6
	ambient.omni_range   = 60.0
	ambient.position     = Vector3(0, 10, -9)
	scene_3d.add_child(ambient)

	# Light background via WorldEnvironment
	var env        := Environment.new()
	env.background_mode   = Environment.BG_COLOR
	env.background_color  = Color(0.90, 0.92, 0.95)  # light blue-grey, like SmartThings
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color  = Color(0.85, 0.88, 0.92)
	env.ambient_light_energy = 0.8
	var we         := WorldEnvironment.new()
	we.environment  = env
	scene_3d.add_child(we)


func _add_device_pins(parent: Node3D, rect: Rect2, devices: Array) -> void:
	var n := devices.size()
	for j in n:
		var dev: String = devices[j]
		# Spread pins evenly across the room
		var t := (float(j) + 0.5) / float(n)
		var px := rect.position.x + rect.size.x * (0.2 + t * 0.6)
		var pz := rect.position.y + rect.size.y * 0.5

		# Pin stem (thin cylinder)
		var stem := MeshInstance3D.new()
		stem.name = "Pin_" + dev
		var cyl  := CylinderMesh.new()
		cyl.top_radius    = 0.04
		cyl.bottom_radius = 0.04
		cyl.height        = PIN_H
		stem.mesh = cyl
		var mat_pin := StandardMaterial3D.new()
		mat_pin.albedo_color = Color(1, 1, 1, 1)
		stem.set_surface_override_material(0, mat_pin)
		stem.position = Vector3(px, PIN_H * 0.5, pz)
		parent.add_child(stem)

		# Pin head (sphere)
		var head := MeshInstance3D.new()
		head.name = "PinHead_" + dev
		var sph  := SphereMesh.new()
		sph.radius = 0.22
		sph.height = 0.44
		head.mesh = sph
		var mat_head := StandardMaterial3D.new()
		mat_head.albedo_color = Color(0.15, 0.50, 0.95)
		head.set_surface_override_material(0, mat_head)
		head.position = Vector3(px, PIN_H + 0.22, pz)
		parent.add_child(head)

		# Device name label
		_add_label3d(parent, Vector3(px, PIN_H + 0.65, pz),
			dev, Color(0.1, 0.1, 0.5, 1), 30)


func _add_box(parent: Node3D, pos: Vector3, size: Vector3,
		mat: StandardMaterial3D, lbl: String) -> void:
	var mi   := MeshInstance3D.new()
	mi.name  = lbl
	var mesh := BoxMesh.new()
	mesh.size = size
	mi.mesh  = mesh
	mi.set_surface_override_material(0, mat)
	mi.position = pos
	parent.add_child(mi)


func _add_label3d(parent: Node3D, pos: Vector3,
		text: String, color: Color, font_sz: int) -> void:
	var lbl              := Label3D.new()
	lbl.text              = text
	lbl.font_size         = font_sz
	lbl.pixel_size        = 0.008
	lbl.billboard         = BaseMaterial3D.BILLBOARD_ENABLED
	lbl.modulate          = color
	lbl.position          = pos
	lbl.no_depth_test     = true
	lbl.outline_size      = 6
	lbl.outline_modulate  = Color(1, 1, 1, 0.85)
	parent.add_child(lbl)


# ── View toggle ───────────────────────────────────────────────────────────────
func _on_toggle_3d() -> void:
	is_3d_mode = !is_3d_mode
	_refresh_view()

func _refresh_view() -> void:
	scene_3d.visible = is_3d_mode
	view_2d.visible  = !is_3d_mode
	btn_3d.text = "2D" if is_3d_mode else "3D"

func _on_reset() -> void:
	if orb.has_method("reset_view"):
		orb.reset_view()

func _on_floor() -> void:
	# Placeholder — could show floor picker popup
	pass

func _on_devices() -> void:
	# Placeholder — could show device filter popup
	pass
