extends Node

const FLOOR_HEIGHT := 0.06
const WALL_HEIGHT := 1.45
const WALL_THICKNESS := 0.12
const ROOM_DRAG_PLANE_Y := FLOOR_HEIGHT * 0.5
const ROOM_HEIGHT := FLOOR_HEIGHT
const ROOM_LABEL_Y := 0.18
const ROOM_DEFAULT_SIZE := Vector2(3.2, 2.4)
const ROOM_TOUCH_MARGIN := 0.28
const ROOM_BORDER_THICKNESS := 0.16
const ROOM_BORDER_HEIGHT := WALL_HEIGHT + 0.08
const ROOM_COLOR_PALETTE := [
	Color(0.95, 0.92, 0.84),
	Color(0.87, 0.93, 0.90),
	Color(0.89, 0.91, 0.96),
	Color(0.93, 0.89, 0.95),
	Color(0.91, 0.94, 0.90),
	Color(0.90, 0.93, 0.97),
]

const FLOOR_PLANS := [
	{
		"id": "floor_1",
		"tab": "1F",
		"name": "Garden Level",
		"limits": Rect2(-6.8, -6.8, 13.6, 13.6),
		"camera_target": Vector3(0.0, 0.0, 0.0),
		"rooms": [
			{
				"name": "Living room",
				"rect": Rect2(-6.8, -6.8, 6.2, 5.6),
				"color": Color(0.95, 0.92, 0.84),
			},
			{
				"name": "Dining room",
				"rect": Rect2(-0.3, -6.8, 6.5, 4.2),
				"color": Color(0.87, 0.93, 0.90),
			},
			{
				"name": "Kitchen",
				"rect": Rect2(-0.3, -2.3, 6.5, 6.0),
				"color": Color(0.89, 0.91, 0.96),
			},
		],
		"dividers": [],
		"seeds": [
			{
				"asset_id": "sofa",
				"position": Vector3(-4.8, FLOOR_HEIGHT * 0.5, -4.6),
				"rotation": 180.0,
			},
			{
				"asset_id": "chair",
				"position": Vector3(2.4, FLOOR_HEIGHT * 0.5, -4.8),
				"rotation": 145.0,
			},
		],
	},
	{
		"id": "floor_2",
		"tab": "2F",
		"name": "Bedroom Level",
		"limits": Rect2(-6.8, -6.8, 13.6, 13.6),
		"camera_target": Vector3(0.0, 0.0, 0.0),
		"rooms": [
			{
				"name": "Primary suite",
				"rect": Rect2(-6.8, -6.8, 7.0, 5.2),
				"color": Color(0.93, 0.89, 0.95),
			},
			{
				"name": "Landing",
				"rect": Rect2(0.5, -6.8, 5.7, 3.4),
				"color": Color(0.91, 0.94, 0.90),
			},
			{
				"name": "Guest room",
				"rect": Rect2(0.5, -3.1, 5.7, 6.8),
				"color": Color(0.90, 0.93, 0.97),
			},
		],
		"dividers": [],
		"seeds": [
			{
				"asset_id": "chair",
				"position": Vector3(3.1, FLOOR_HEIGHT * 0.5, -1.2),
				"rotation": 90.0,
			},
		],
	},
	{
		"id": "floor_3",
		"tab": "Studio",
		"name": "Studio Level",
		"limits": Rect2(-6.8, -6.8, 13.6, 13.6),
		"camera_target": Vector3(0.0, 0.0, 0.0),
		"rooms": [
			{
				"name": "Lounge",
				"rect": Rect2(-6.8, -6.8, 5.4, 5.8),
				"color": Color(0.96, 0.91, 0.84),
			},
			{
				"name": "Studio",
				"rect": Rect2(-1.0, -6.8, 7.2, 4.1),
				"color": Color(0.88, 0.94, 0.96),
			},
			{
				"name": "Library",
				"rect": Rect2(-1.0, -2.3, 7.2, 6.5),
				"color": Color(0.92, 0.94, 0.88),
			},
		],
		"dividers": [],
		"seeds": [
			{
				"asset_id": "sofa",
				"position": Vector3(-4.5, FLOOR_HEIGHT * 0.5, -4.7),
				"rotation": 180.0,
			},
		],
	},
]

const ASSET_CATALOG := {
	"sofa": {
		"name": "Glam Velvet Sofa",
		"subtitle": "Living room",
		"path": "res://assets/GlamVelvetSofa.glb",
		"scale": 1.0,
		"rotation": 180.0,
		"accent": Color(0.39, 0.55, 0.94),
	},
	"chair": {
		"name": "Sheen Chair",
		"subtitle": "Dining room",
		"path": "res://assets/SheenChair.glb",
		"scale": 1.0,
		"rotation": 135.0,
		"accent": Color(0.31, 0.70, 0.65),
	},
}

const INVENTORY_ITEM := preload("res://inventory_item.gd")

@onready var ui_root: Control = $UI/Root
@onready var scene_root: Node3D = $Scene3D
@onready var floors_root: Node3D = $Scene3D/Floors
@onready var camera_3d: Camera3D = $Scene3D/Camera3D
@onready var placement_controller: RuntimePlacementController3D = $Scene3D/PlacementController
@onready var title_label: Label = $UI/Root/TopBar/TopBarMargin/TopBarVBox/TitleLabel
@onready var subtitle_label: Label = $UI/Root/TopBar/TopBarMargin/TopBarVBox/SubtitleLabel
@onready var floor_tabs: HBoxContainer = $UI/Root/TopBar/TopBarMargin/TopBarVBox/FloorTabs
@onready var btn_plan_edit: Button = $UI/Root/TopBar/TopBarMargin/TopBarVBox/PlanTools/BtnPlanEdit
@onready var btn_add_room: Button = $UI/Root/TopBar/TopBarMargin/TopBarVBox/PlanTools/BtnAddRoom
@onready var inventory_panel: PanelContainer = $UI/Root/Inventory
@onready var inventory_list: BoxContainer = $UI/Root/Inventory/Margin/InventoryList
@onready var bottom_bar: PanelContainer = $UI/Root/BottomBar
@onready var selection_label: Label = $UI/Root/BottomBar/Bar/SelectionLabel
@onready var status_label: Label = $UI/Root/StatusLabel
@onready var btn_rotate_left: Button = $UI/Root/BottomBar/Bar/BtnRotateLeft
@onready var btn_rotate_right: Button = $UI/Root/BottomBar/Bar/BtnRotateRight
@onready var btn_flip_x: Button = $UI/Root/BottomBar/Bar/BtnFlipX
@onready var btn_flip_z: Button = $UI/Root/BottomBar/Bar/BtnFlipZ
@onready var btn_delete: Button = $UI/Root/BottomBar/Bar/BtnDelete
@onready var btn_clear: Button = $UI/Root/BottomBar/Bar/BtnClear
@onready var drop_target: Control = $UI/Root/DropTarget

var _floor_states: Array = []
var _floor_buttons: Array = []
var _placed_counts_by_floor: Dictionary = {}
var _current_floor_index := -1
var _inventory_drag_active := false
var _inventory_drag_pointer := -1
var _inventory_drag_is_touch := true
var _inventory_drag_asset_id := ""
var _inventory_drag_preview: PanelContainer
var _inventory_drag_preview_label: Label
var _plan_edit_mode := false
var _selected_room_index := -1
var _room_drag_active := false
var _room_drag_pointer := -1
var _room_drag_offset := Vector2.ZERO
var _room_drag_start_rect := Rect2()
var _room_drag_items: Array = []

func _ready() -> void:
	set_process_input(true)
	_build_stage()
	_ensure_inventory_drag_preview()
	_build_floor_tabs()
	_build_inventory()
	_connect_ui()
	_seed_demo_items()
	_set_active_floor(0)
	_update_selection_ui(null)

func _input(event: InputEvent) -> void:
	if _plan_edit_mode:
		if _handle_plan_edit_input(event):
			get_viewport().set_input_as_handled()
		return

	if !_inventory_drag_active:
		return

	get_viewport().set_input_as_handled()

	if _inventory_drag_is_touch:
		if event is InputEventScreenDrag and event.index == _inventory_drag_pointer:
			_update_inventory_drag_preview(event.position)
		elif event is InputEventScreenTouch and event.index == _inventory_drag_pointer and !event.pressed:
			_finish_inventory_drag(event.position)
	else:
		if event is InputEventMouseMotion:
			_update_inventory_drag_preview(event.position)
		elif event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and !event.pressed:
			_finish_inventory_drag(event.position)

func _build_stage() -> void:
	for floor_data in FLOOR_PLANS:
		var floor_root := Node3D.new()
		floor_root.name = floor_data["id"]
		floor_root.visible = false
		floors_root.add_child(floor_root)

		var shell := Node3D.new()
		shell.name = "Shell"
		floor_root.add_child(shell)

		var rooms_root := Node3D.new()
		rooms_root.name = "Rooms"
		floor_root.add_child(rooms_root)

		var placed_items := Node3D.new()
		placed_items.name = "PlacedItems"
		floor_root.add_child(placed_items)

		_build_floor_shell(shell, floor_data)
		var rooms := _build_floor_rooms(rooms_root, floor_data["rooms"])
		_floor_states.append({
			"data": floor_data,
			"root": floor_root,
			"rooms_root": rooms_root,
			"rooms": rooms,
			"placed_items": placed_items,
		})

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-52.0, 36.0, 0.0)
	sun.light_energy = 1.1
	sun.shadow_enabled = true
	scene_root.add_child(sun)

	var fill := OmniLight3D.new()
	fill.position = Vector3(0.0, 7.5, -6.0)
	fill.omni_range = 40.0
	fill.light_energy = 0.55
	scene_root.add_child(fill)

	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color(0.93, 0.95, 0.98)
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color(0.88, 0.91, 0.96)
	environment.ambient_light_energy = 0.9
	var world_env := WorldEnvironment.new()
	world_env.environment = environment
	scene_root.add_child(world_env)

func _build_floor_shell(parent: Node3D, floor_data: Dictionary) -> void:
	var wall_material := StandardMaterial3D.new()
	wall_material.albedo_color = Color(0.96, 0.96, 0.97)
	wall_material.roughness = 0.95

	var limits: Rect2 = floor_data["limits"]
	_add_wall_segment(parent, Vector3(limits.position.x, WALL_HEIGHT * 0.5, limits.position.y + limits.size.y * 0.5), Vector3(WALL_THICKNESS, WALL_HEIGHT, limits.size.y + WALL_THICKNESS), wall_material, "WallWest")
	_add_wall_segment(parent, Vector3(limits.position.x + limits.size.x, WALL_HEIGHT * 0.5, limits.position.y + limits.size.y * 0.5), Vector3(WALL_THICKNESS, WALL_HEIGHT, limits.size.y + WALL_THICKNESS), wall_material, "WallEast")
	_add_wall_segment(parent, Vector3(limits.position.x + limits.size.x * 0.5, WALL_HEIGHT * 0.5, limits.position.y), Vector3(limits.size.x + WALL_THICKNESS, WALL_HEIGHT, WALL_THICKNESS), wall_material, "WallNorth")
	_add_wall_segment(parent, Vector3(limits.position.x + limits.size.x * 0.5, WALL_HEIGHT * 0.5, limits.position.y + limits.size.y), Vector3(limits.size.x + WALL_THICKNESS, WALL_HEIGHT, WALL_THICKNESS), wall_material, "WallSouth")

	for divider in floor_data["dividers"]:
		_add_wall_segment(parent, divider["position"], divider["size"], wall_material, divider["name"])

func _build_floor_rooms(parent: Node3D, rooms_data: Array) -> Array:
	var rooms: Array = []
	for room_data in rooms_data:
		var room := {
			"name": room_data["name"],
			"rect": room_data["rect"],
			"color": room_data["color"],
		}
		_instantiate_room_nodes(parent, room, rooms.size())
		rooms.append(room)
	return rooms

func _instantiate_room_nodes(parent: Node3D, room: Dictionary, room_index: int) -> void:
	var room_root := Node3D.new()
	room_root.name = "Room_%02d" % room_index
	parent.add_child(room_root)

	var floor_node := MeshInstance3D.new()
	floor_node.name = "Floor"
	var mesh := BoxMesh.new()
	floor_node.mesh = mesh
	var material := StandardMaterial3D.new()
	material.roughness = 0.88
	floor_node.material_override = material
	room_root.add_child(floor_node)

	var wall_nodes: Array = []
	var wall_material := StandardMaterial3D.new()
	wall_material.roughness = 0.95
	for i in range(4):
		var wall_node := MeshInstance3D.new()
		wall_node.name = "Wall_%d" % i
		wall_node.mesh = BoxMesh.new()
		wall_node.material_override = wall_material
		room_root.add_child(wall_node)
		wall_nodes.append(wall_node)

	var border_nodes: Array = []
	var border_material := StandardMaterial3D.new()
	border_material.albedo_color = Color(0.16, 0.47, 1.0, 0.95)
	border_material.roughness = 0.25
	border_material.metallic = 0.0
	border_material.emission_enabled = true
	border_material.emission = Color(0.16, 0.47, 1.0)
	border_material.emission_energy_multiplier = 0.7
	for i in range(4):
		var border_node := MeshInstance3D.new()
		border_node.name = "Border_%d" % i
		border_node.mesh = BoxMesh.new()
		border_node.material_override = border_material
		border_node.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		room_root.add_child(border_node)
		border_nodes.append(border_node)

	var label := Label3D.new()
	label.name = "Label"
	label.font_size = 42
	label.pixel_size = 0.0085
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	label.no_depth_test = true
	label.modulate = Color(0.18, 0.22, 0.38)
	label.outline_size = 6
	label.outline_modulate = Color(1, 1, 1, 0.85)
	room_root.add_child(label)

	room["room_root"] = room_root
	room["floor_node"] = floor_node
	room["label_node"] = label
	room["material"] = material
	room["wall_material"] = wall_material
	room["wall_nodes"] = wall_nodes
	room["border_nodes"] = border_nodes
	_apply_room_visual(room, false)

func _apply_room_visual(room: Dictionary, selected: bool) -> void:
	var rect: Rect2 = room["rect"]
	var x0 := rect.position.x
	var x1 := rect.position.x + rect.size.x
	var z0 := rect.position.y
	var z1 := rect.position.y + rect.size.y
	var x_center := (x0 + x1) * 0.5
	var z_center := (z0 + z1) * 0.5
	var center := Vector3(x_center, 0.0, z_center)

	var floor_node: MeshInstance3D = room["floor_node"]
	var room_mesh: BoxMesh = floor_node.mesh
	room_mesh.size = Vector3(rect.size.x, ROOM_HEIGHT, rect.size.y)
	floor_node.position = center + Vector3(0.0, 0.006 if selected else 0.0, 0.0)

	var material: StandardMaterial3D = room["material"]
	var base_color: Color = room["color"]
	material.albedo_color = base_color.lightened(0.14) if selected else base_color
	material.emission_enabled = selected
	material.emission = Color(0.18, 0.47, 1.0)
	material.emission_energy_multiplier = 0.35 if selected else 0.0

	var wall_material: StandardMaterial3D = room["wall_material"]
	wall_material.albedo_color = base_color.lightened(0.32).lerp(Color(0.94, 0.95, 0.97), 0.55)
	wall_material.emission_enabled = selected
	wall_material.emission = Color(0.15, 0.46, 0.98)
	wall_material.emission_energy_multiplier = 0.36 if selected else 0.0

	var wall_nodes: Array = room["wall_nodes"]
	var west_wall: MeshInstance3D = wall_nodes[0]
	var east_wall: MeshInstance3D = wall_nodes[1]
	var north_wall: MeshInstance3D = wall_nodes[2]
	var south_wall: MeshInstance3D = wall_nodes[3]
	(west_wall.mesh as BoxMesh).size = Vector3(WALL_THICKNESS, WALL_HEIGHT, rect.size.y + WALL_THICKNESS)
	(east_wall.mesh as BoxMesh).size = Vector3(WALL_THICKNESS, WALL_HEIGHT, rect.size.y + WALL_THICKNESS)
	(north_wall.mesh as BoxMesh).size = Vector3(rect.size.x + WALL_THICKNESS, WALL_HEIGHT, WALL_THICKNESS)
	(south_wall.mesh as BoxMesh).size = Vector3(rect.size.x + WALL_THICKNESS, WALL_HEIGHT, WALL_THICKNESS)
	west_wall.position = Vector3(x0, WALL_HEIGHT * 0.5, z_center)
	east_wall.position = Vector3(x1, WALL_HEIGHT * 0.5, z_center)
	north_wall.position = Vector3(x_center, WALL_HEIGHT * 0.5, z0)
	south_wall.position = Vector3(x_center, WALL_HEIGHT * 0.5, z1)

	var border_nodes: Array = room["border_nodes"]
	var west_border: MeshInstance3D = border_nodes[0]
	var east_border: MeshInstance3D = border_nodes[1]
	var north_border: MeshInstance3D = border_nodes[2]
	var south_border: MeshInstance3D = border_nodes[3]
	(west_border.mesh as BoxMesh).size = Vector3(ROOM_BORDER_THICKNESS, ROOM_BORDER_HEIGHT, rect.size.y + ROOM_BORDER_THICKNESS)
	(east_border.mesh as BoxMesh).size = Vector3(ROOM_BORDER_THICKNESS, ROOM_BORDER_HEIGHT, rect.size.y + ROOM_BORDER_THICKNESS)
	(north_border.mesh as BoxMesh).size = Vector3(rect.size.x + ROOM_BORDER_THICKNESS, ROOM_BORDER_HEIGHT, ROOM_BORDER_THICKNESS)
	(south_border.mesh as BoxMesh).size = Vector3(rect.size.x + ROOM_BORDER_THICKNESS, ROOM_BORDER_HEIGHT, ROOM_BORDER_THICKNESS)
	var border_y := ROOM_BORDER_HEIGHT * 0.5
	west_border.position = Vector3(x0, border_y, z_center)
	east_border.position = Vector3(x1, border_y, z_center)
	north_border.position = Vector3(x_center, border_y, z0)
	south_border.position = Vector3(x_center, border_y, z1)
	for border_node in border_nodes:
		border_node.visible = selected

	var label: Label3D = room["label_node"]
	label.text = room["name"]
	label.position = Vector3(center.x, ROOM_LABEL_Y, center.z)
	label.modulate = Color(0.11, 0.21, 0.42) if selected else Color(0.18, 0.22, 0.38)

func _refresh_active_floor_room_visuals() -> void:
	if _current_floor_index < 0:
		return
	var floor_state: Dictionary = _floor_states[_current_floor_index]
	var rooms: Array = floor_state["rooms"]
	for i in range(rooms.size()):
		_apply_room_visual(rooms[i], i == _selected_room_index)

func _pick_room_at_screen(screen_position: Vector2) -> Dictionary:
	var floor_hit: Variant = _screen_to_floor(screen_position)
	if floor_hit == null or _current_floor_index < 0:
		return {
			"index": -1,
			"point": Vector2.ZERO,
		}

	var point := Vector2(floor_hit.x, floor_hit.z)
	var floor_state: Dictionary = _floor_states[_current_floor_index]
	var rooms: Array = floor_state["rooms"]
	for i in range(rooms.size() - 1, -1, -1):
		var room_rect: Rect2 = rooms[i]["rect"].grow(ROOM_TOUCH_MARGIN)
		if room_rect.has_point(point):
			return {
				"index": i,
				"point": point,
			}

	return {
		"index": -1,
		"point": point,
	}

func _select_room(room_index: int) -> void:
	if _selected_room_index == room_index:
		return
	_selected_room_index = room_index
	_refresh_active_floor_room_visuals()
	_update_plan_edit_ui()

func _clear_room_selection() -> void:
	_selected_room_index = -1
	_end_room_drag()
	_room_drag_offset = Vector2.ZERO
	_room_drag_start_rect = Rect2()
	_refresh_active_floor_room_visuals()
	_update_plan_edit_ui()

func _collect_room_furniture(room_rect: Rect2) -> Array:
	var captured: Array = []
	if _current_floor_index < 0:
		return captured

	var placed_items: Node3D = _floor_states[_current_floor_index]["placed_items"]
	for child in placed_items.get_children():
		if !(child is Node3D):
			continue
		var node := child as Node3D
		var floor_position := Vector2(node.position.x, node.position.z)
		if room_rect.has_point(floor_position):
			captured.append({
				"node": node,
				"start_position": node.position,
			})
	return captured

func _begin_room_drag(pointer_id: int, picked_point: Vector2) -> void:
	if _current_floor_index < 0 or _selected_room_index < 0:
		return
	var active_room: Rect2 = _floor_states[_current_floor_index]["rooms"][_selected_room_index]["rect"]
	_room_drag_active = true
	_room_drag_pointer = pointer_id
	_room_drag_offset = picked_point - active_room.position
	_room_drag_start_rect = active_room
	_room_drag_items = _collect_room_furniture(active_room)

func _end_room_drag() -> void:
	_room_drag_active = false
	_room_drag_pointer = -1
	_room_drag_items.clear()

func _move_room_furniture_with_drag(room_delta: Vector2) -> void:
	if _current_floor_index < 0 or _room_drag_items.is_empty():
		return

	for item in _room_drag_items:
		var node: Node3D = item["node"]
		if node == null or !is_instance_valid(node):
			continue
		var start_position: Vector3 = item["start_position"]
		var moved := start_position + Vector3(room_delta.x, 0.0, room_delta.y)
		node.position = _clamp_to_floor(moved, _current_floor_index)

func _clamp_room_rect_to_limits(room_rect: Rect2, limits: Rect2) -> Rect2:
	var rect := room_rect
	rect.size.x = clamp(rect.size.x, 1.8, limits.size.x)
	rect.size.y = clamp(rect.size.y, 1.6, limits.size.y)
	rect.position.x = clamp(rect.position.x, limits.position.x, limits.position.x + limits.size.x - rect.size.x)
	rect.position.y = clamp(rect.position.y, limits.position.y, limits.position.y + limits.size.y - rect.size.y)
	return rect

func _move_selected_room_to(screen_position: Vector2) -> bool:
	if _current_floor_index < 0 or _selected_room_index < 0:
		return false

	var floor_hit: Variant = _screen_to_floor(screen_position)
	if floor_hit == null:
		return false

	var floor_state: Dictionary = _floor_states[_current_floor_index]
	var rooms: Array = floor_state["rooms"]
	if _selected_room_index >= rooms.size():
		return false

	var room: Dictionary = rooms[_selected_room_index]
	var start_rect: Rect2 = _room_drag_start_rect if _room_drag_active else room["rect"]
	var new_origin := Vector2(floor_hit.x, floor_hit.z) - _room_drag_offset
	var new_rect: Rect2 = start_rect
	new_rect.position = new_origin
	new_rect = _clamp_room_rect_to_limits(new_rect, floor_state["data"]["limits"])
	var room_delta := new_rect.position - start_rect.position
	room["rect"] = new_rect
	rooms[_selected_room_index] = room
	floor_state["rooms"] = rooms
	_floor_states[_current_floor_index] = floor_state
	_apply_room_visual(room, true)
	_move_room_furniture_with_drag(room_delta)
	return true

func _is_pointer_over_blocking_ui(screen_position: Vector2) -> bool:
	var control: Control = get_viewport().gui_find_control(screen_position)
	return control != null and control.get_mouse_filter() == Control.MOUSE_FILTER_STOP

func _is_pointer_in_room_edit_area(screen_position: Vector2) -> bool:
	return drop_target.get_global_rect().has_point(screen_position)

func _handle_plan_edit_input(event: InputEvent) -> bool:
	if event is InputEventScreenTouch:
		var touch_event := event as InputEventScreenTouch
		if touch_event.pressed:
			if !_is_pointer_in_room_edit_area(touch_event.position):
				return false
			if _is_pointer_over_blocking_ui(touch_event.position):
				return false
			var pick := _pick_room_at_screen(touch_event.position)
			if pick["index"] >= 0:
				_select_room(pick["index"])
				_begin_room_drag(touch_event.index, pick["point"])
				status_label.text = "Room selected. Drag to reposition it with all furniture inside on %s." % _get_current_floor_data()["name"]
				return true
			_clear_room_selection()
			status_label.text = "Plan edit active. Tap a room to select it, or use Add Room."
			return true

		if _room_drag_active and touch_event.index == _room_drag_pointer:
			_end_room_drag()
			return true
		return false

	if event is InputEventScreenDrag:
		var drag_event := event as InputEventScreenDrag
		if !_room_drag_active or drag_event.index != _room_drag_pointer:
			return false
		return _move_selected_room_to(drag_event.position)

	if event is InputEventMouseButton:
		var mouse_button := event as InputEventMouseButton
		if mouse_button.button_index != MOUSE_BUTTON_LEFT:
			return false
		if mouse_button.pressed:
			if !_is_pointer_in_room_edit_area(mouse_button.position):
				return false
			if _is_pointer_over_blocking_ui(mouse_button.position):
				return false
			var pick := _pick_room_at_screen(mouse_button.position)
			if pick["index"] >= 0:
				_select_room(pick["index"])
				_begin_room_drag(0, pick["point"])
				status_label.text = "Room selected. Drag to reposition it with all furniture inside on %s." % _get_current_floor_data()["name"]
				return true
			_clear_room_selection()
			status_label.text = "Plan edit active. Tap a room to select it, or use Add Room."
			return true
		if _room_drag_active and _room_drag_pointer == 0:
			_end_room_drag()
			return true
		return false

	if event is InputEventMouseMotion and _room_drag_active and _room_drag_pointer == 0:
		var mouse_motion := event as InputEventMouseMotion
		return _move_selected_room_to(mouse_motion.position)

	return false

func _build_floor_tabs() -> void:
	for child in floor_tabs.get_children():
		child.queue_free()

	_floor_buttons.clear()
	for index in range(FLOOR_PLANS.size()):
		var floor_data: Dictionary = FLOOR_PLANS[index]
		var button := Button.new()
		button.toggle_mode = true
		button.focus_mode = Control.FOCUS_NONE
		button.custom_minimum_size = Vector2(76.0, 34.0)
		button.text = floor_data["tab"]
		button.pressed.connect(_on_floor_button_pressed.bind(index))
		floor_tabs.add_child(button)
		_floor_buttons.append(button)

func _build_inventory() -> void:
	for asset_id in ["sofa", "chair"]:
		var item_data: Dictionary = ASSET_CATALOG[asset_id]
		var card := INVENTORY_ITEM.new()
		card.asset_id = asset_id
		card.display_name = item_data["name"]
		card.subtitle = item_data["subtitle"]
		card.accent_color = item_data["accent"]
		card.drag_requested.connect(_on_inventory_drag_requested)
		inventory_list.add_child(card)

func _ensure_inventory_drag_preview() -> void:
	if _inventory_drag_preview != null:
		return

	_inventory_drag_preview = PanelContainer.new()
	_inventory_drag_preview.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_inventory_drag_preview.visible = false
	_inventory_drag_preview.custom_minimum_size = Vector2(190.0, 56.0)
	_inventory_drag_preview.z_index = 20

	var preview_style := StyleBoxFlat.new()
	preview_style.bg_color = Color(0.28, 0.46, 0.92, 0.95)
	preview_style.corner_radius_top_left = 12
	preview_style.corner_radius_top_right = 12
	preview_style.corner_radius_bottom_right = 12
	preview_style.corner_radius_bottom_left = 12
	preview_style.shadow_color = Color(0, 0, 0, 0.18)
	preview_style.shadow_size = 6
	_inventory_drag_preview.add_theme_stylebox_override("panel", preview_style)

	_inventory_drag_preview_label = Label.new()
	_inventory_drag_preview_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_inventory_drag_preview_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_inventory_drag_preview_label.anchors_preset = Control.PRESET_FULL_RECT
	_inventory_drag_preview_label.grow_horizontal = Control.GROW_DIRECTION_BOTH
	_inventory_drag_preview_label.grow_vertical = Control.GROW_DIRECTION_BOTH
	_inventory_drag_preview_label.add_theme_font_size_override("font_size", 16)
	_inventory_drag_preview_label.modulate = Color(1, 1, 1)
	_inventory_drag_preview.add_child(_inventory_drag_preview_label)

	ui_root.add_child(_inventory_drag_preview)

func _on_inventory_drag_requested(asset_id: String, display_name: String, accent: Color, screen_position: Vector2, pointer_id: int, is_touch: bool) -> void:
	if _plan_edit_mode:
		status_label.text = "Plan edit mode is active. Turn it off to place furniture."
		return

	if _inventory_drag_active:
		return

	_inventory_drag_active = true
	_inventory_drag_pointer = pointer_id
	_inventory_drag_is_touch = is_touch
	_inventory_drag_asset_id = asset_id

	var preview_style := _inventory_drag_preview.get_theme_stylebox("panel") as StyleBoxFlat
	if preview_style != null:
		preview_style = preview_style.duplicate()
		preview_style.bg_color = accent
		_inventory_drag_preview.add_theme_stylebox_override("panel", preview_style)

	_inventory_drag_preview_label.text = display_name
	_inventory_drag_preview.visible = true
	status_label.text = "Drag %s onto %s." % [display_name, _get_current_floor_data()["name"]]
	_update_inventory_drag_preview(screen_position)

func _update_inventory_drag_preview(screen_position: Vector2) -> void:
	if _inventory_drag_preview == null:
		return

	var preview_position := screen_position + Vector2(18.0, -74.0)
	var viewport_size := get_viewport().get_visible_rect().size
	preview_position.x = clamp(preview_position.x, 16.0, viewport_size.x - _inventory_drag_preview.custom_minimum_size.x - 16.0)
	preview_position.y = clamp(preview_position.y, 16.0, viewport_size.y - _inventory_drag_preview.custom_minimum_size.y - 16.0)
	_inventory_drag_preview.position = preview_position
	drop_target.set_drag_feedback(_is_valid_inventory_drop_position(screen_position))

func _finish_inventory_drag(screen_position: Vector2) -> void:
	var asset_id := _inventory_drag_asset_id
	var should_drop := _is_valid_inventory_drop_position(screen_position)
	_cancel_inventory_drag()
	if should_drop:
		_on_asset_dropped(asset_id, screen_position)
	else:
		status_label.text = "Drag cancelled. Drop furniture onto %s to place it." % _get_current_floor_data()["name"]

func _cancel_inventory_drag() -> void:
	_inventory_drag_active = false
	_inventory_drag_pointer = -1
	_inventory_drag_asset_id = ""
	if _inventory_drag_preview != null:
		_inventory_drag_preview.visible = false
	drop_target.set_drag_feedback(false)

func _is_valid_inventory_drop_position(screen_position: Vector2) -> bool:
	if _plan_edit_mode:
		return false
	return drop_target.get_global_rect().has_point(screen_position) and _screen_to_floor(screen_position) != null

func _connect_ui() -> void:
	drop_target.asset_dropped.connect(_on_asset_dropped)
	btn_plan_edit.toggled.connect(_set_plan_edit_mode)
	btn_add_room.pressed.connect(_add_room_to_current_floor)
	btn_rotate_left.pressed.connect(func() -> void: placement_controller.rotate_selected(-15.0))
	btn_rotate_right.pressed.connect(func() -> void: placement_controller.rotate_selected(15.0))
	btn_flip_x.pressed.connect(func() -> void: placement_controller.flip_selected_x())
	btn_flip_z.pressed.connect(func() -> void: placement_controller.flip_selected_z())
	btn_delete.pressed.connect(_delete_selected)
	btn_clear.pressed.connect(func() -> void: placement_controller.clear_selection())
	placement_controller.selection_changed.connect(_update_selection_ui)
	placement_controller.target_transform_changed.connect(_on_target_transform_changed)

func _set_plan_edit_mode(enabled: bool) -> void:
	_plan_edit_mode = enabled
	btn_plan_edit.button_pressed = enabled
	btn_plan_edit.text = "Plan Edit: ON" if enabled else "Plan Edit: OFF"
	btn_add_room.disabled = !enabled

	if enabled:
		_cancel_inventory_drag()
		placement_controller.clear_selection()
		placement_controller.set_process_input(false)
		drop_target.mouse_filter = Control.MOUSE_FILTER_IGNORE
		inventory_panel.visible = false
		_clear_room_selection()
		status_label.text = "Plan edit mode enabled. Select a room and drag it, or add a room. Furniture is frozen."
	else:
		_end_room_drag()
		placement_controller.set_process_input(true)
		drop_target.mouse_filter = Control.MOUSE_FILTER_PASS
		inventory_panel.visible = true
		_clear_room_selection()
		_update_selection_ui(placement_controller.get_selected_target())

	_update_plan_edit_ui()

func _update_plan_edit_ui() -> void:
	if _plan_edit_mode:
		btn_rotate_left.disabled = true
		btn_rotate_right.disabled = true
		btn_flip_x.disabled = true
		btn_flip_z.disabled = true
		btn_delete.disabled = true
		btn_clear.disabled = true

		if _selected_room_index >= 0 and _current_floor_index >= 0:
			var room: Dictionary = _floor_states[_current_floor_index]["rooms"][_selected_room_index]
			selection_label.text = "Room: %s" % room["name"]
		else:
			selection_label.text = "Room: none"

func _add_room_to_current_floor() -> void:
	if !_plan_edit_mode or _current_floor_index < 0:
		return

	var floor_state: Dictionary = _floor_states[_current_floor_index]
	var rooms: Array = floor_state["rooms"]
	var limits: Rect2 = floor_state["data"]["limits"]
	var size := ROOM_DEFAULT_SIZE
	size.x = min(size.x, limits.size.x)
	size.y = min(size.y, limits.size.y)

	var max_x := limits.size.x - size.x
	var max_y := limits.size.y - size.y
	var seed := float(rooms.size() + 1)
	var x_ratio := fmod(seed * 0.37, 0.82)
	var y_ratio := fmod(seed * 0.53, 0.82)
	var origin := Vector2(
		limits.position.x + max_x * x_ratio,
		limits.position.y + max_y * y_ratio
	)

	var room := {
		"name": "Room %02d" % (rooms.size() + 1),
		"rect": Rect2(origin, size),
		"color": ROOM_COLOR_PALETTE[rooms.size() % ROOM_COLOR_PALETTE.size()],
	}

	_instantiate_room_nodes(floor_state["rooms_root"], room, rooms.size())
	rooms.append(room)
	floor_state["rooms"] = rooms
	_floor_states[_current_floor_index] = floor_state
	_select_room(rooms.size() - 1)
	status_label.text = "Added %s on %s. Drag it to shape the floor plan." % [room["name"], floor_state["data"]["name"]]

func _seed_demo_items() -> void:
	for floor_index in range(FLOOR_PLANS.size()):
		for seed in FLOOR_PLANS[floor_index]["seeds"]:
			_spawn_asset_on_floor(seed["asset_id"], seed["position"], floor_index, seed["rotation"])

func _set_active_floor(index: int) -> void:
	if index < 0 or index >= _floor_states.size():
		return
	if _current_floor_index == index:
		return

	placement_controller.clear_selection()

	if _current_floor_index >= 0:
		_floor_states[_current_floor_index]["root"].visible = false

	_current_floor_index = index
	var floor_state: Dictionary = _floor_states[index]
	floor_state["root"].visible = true
	placement_controller.set_targets_root_path(placement_controller.get_path_to(floor_state["placed_items"]))
	_selected_room_index = -1
	_end_room_drag()
	_refresh_active_floor_room_visuals()

	if camera_3d.has_method("focus_floor"):
		camera_3d.focus_floor(floor_state["data"]["camera_target"])

	for button_index in range(_floor_buttons.size()):
		_floor_buttons[button_index].button_pressed = button_index == index

	title_label.text = "%s  |  %s" % [floor_state["data"]["name"], floor_state["data"]["tab"]]
	if _plan_edit_mode:
		subtitle_label.text = "Plan edit mode: select a room rectangle, drag to reposition, and add rooms."
		status_label.text = "%s is active for plan editing. Tap a room to select it, drag it, or add a room." % floor_state["data"]["name"]
	else:
		subtitle_label.text = "Switch floors, keep each layout, then drag furniture into the current floor plan."
		status_label.text = "%s is active. Tap a model to select, double-tap it to move, and tap outside to clear selection." % floor_state["data"]["name"]
	_update_plan_edit_ui()

func _on_floor_button_pressed(index: int) -> void:
	_set_active_floor(index)

func _on_asset_dropped(asset_id: String, screen_position: Vector2) -> void:
	if _plan_edit_mode:
		return

	var floor_hit: Variant = _screen_to_floor(screen_position)
	if floor_hit == null:
		return

	var wrapper := _spawn_asset(asset_id, _clamp_to_floor(floor_hit))
	if wrapper:
		placement_controller.select_target(wrapper)
		status_label.text = "Placed %s on %s. Tap once to select, double-tap to move, and drag upper/blue handle to rotate." % [ASSET_CATALOG[asset_id]["name"], _get_current_floor_data()["name"]]

func _spawn_asset(asset_id: String, world_position: Vector3) -> Node3D:
	return _spawn_asset_on_floor(asset_id, world_position, _current_floor_index)

func _spawn_asset_on_floor(asset_id: String, world_position: Vector3, floor_index: int, rotation_override: Variant = null) -> Node3D:
	if !ASSET_CATALOG.has(asset_id) or floor_index < 0 or floor_index >= _floor_states.size():
		return null

	var item_data: Dictionary = ASSET_CATALOG[asset_id]
	var packed: PackedScene = load(item_data["path"])
	if packed == null:
		push_error("Missing asset scene: %s" % item_data["path"])
		return null

	var instantiated := packed.instantiate()
	if !(instantiated is Node3D):
		push_error("Imported asset root is not a Node3D: %s" % item_data["path"])
		instantiated.queue_free()
		return null

	var wrapper := Node3D.new()
	wrapper.name = _next_asset_name(asset_id, floor_index)
	_floor_states[floor_index]["placed_items"].add_child(wrapper)
	wrapper.position = _clamp_to_floor(world_position, floor_index)
	wrapper.rotation_degrees.y = rotation_override if rotation_override != null else item_data["rotation"]
	wrapper.scale = Vector3.ONE * float(item_data["scale"])

	wrapper.add_child(instantiated)
	_normalize_asset_root(wrapper, instantiated)
	return wrapper

func _normalize_asset_root(_wrapper: Node3D, asset_root: Node3D) -> void:
	var bounds: Variant = _compute_visual_bounds(asset_root)
	if bounds == null:
		return

	var offset := Vector3(
		-(bounds.position.x + bounds.size.x * 0.5),
		-bounds.position.y,
		-(bounds.position.z + bounds.size.z * 0.5)
	)
	asset_root.position += offset

func _compute_visual_bounds(root: Node3D) -> Variant:
	var gathered := _gather_bounds_recursive(root, Transform3D.IDENTITY)
	return gathered["aabb"] if gathered["found"] else null

func _gather_bounds_recursive(node: Node, accumulated: Transform3D) -> Dictionary:
	var found := false
	var result := AABB()

	if node is GeometryInstance3D:
		var local_aabb: AABB = node.get_aabb()
		if local_aabb.has_volume() or local_aabb.has_surface():
			for corner in range(8):
				var world_corner := accumulated * local_aabb.get_endpoint(corner)
				if !found:
					result = AABB(world_corner, Vector3.ZERO)
					found = true
				else:
					result = result.expand(world_corner)

	for child in node.get_children():
		if child is Node3D:
			var child_result := _gather_bounds_recursive(child, accumulated * child.transform)
			if child_result["found"]:
				if !found:
					result = child_result["aabb"]
					found = true
				else:
					result = _merge_aabb(result, child_result["aabb"])

	return {
		"aabb": result,
		"found": found,
	}

func _merge_aabb(a: AABB, b: AABB) -> AABB:
	var merged := a
	for corner in range(8):
		merged = merged.expand(b.get_endpoint(corner))
	return merged

func _screen_to_floor(screen_position: Vector2) -> Variant:
	var ray_from := camera_3d.project_ray_origin(screen_position)
	var ray_dir := camera_3d.project_ray_normal(screen_position)
	return Plane(Vector3.UP, Vector3(0.0, FLOOR_HEIGHT * 0.5, 0.0)).intersects_ray(ray_from, ray_dir)

func _clamp_to_floor(world_position: Vector3, floor_index: int = _current_floor_index) -> Vector3:
	var limits: Rect2 = FLOOR_PLANS[floor_index]["limits"]
	return Vector3(
		clamp(world_position.x, limits.position.x + 0.6, limits.position.x + limits.size.x - 0.6),
		FLOOR_HEIGHT * 0.5,
		clamp(world_position.z, limits.position.y + 0.6, limits.position.y + limits.size.y - 0.6)
	)

func _update_selection_ui(target: Node3D) -> void:
	if _plan_edit_mode:
		_update_plan_edit_ui()
		return

	var has_selection := target != null
	selection_label.text = "Selected: %s" % target.name if has_selection else "Selected: nothing"
	btn_rotate_left.disabled = !has_selection
	btn_rotate_right.disabled = !has_selection
	btn_flip_x.disabled = !has_selection
	btn_flip_z.disabled = !has_selection
	btn_delete.disabled = !has_selection
	btn_clear.disabled = !has_selection

	if _current_floor_index < 0:
		return

	if has_selection:
		status_label.text = "Editing %s on %s. Double-tap this item to move it, drag upper part or blue handle to rotate, tap outside to clear." % [target.name, _get_current_floor_data()["name"]]
	else:
		status_label.text = "%s is active. Drag furniture in, tap to select, double-tap selected furniture to move, tap outside to clear." % _get_current_floor_data()["name"]

func _on_target_transform_changed(target: Node3D) -> void:
	if target != null:
		target.position = _clamp_to_floor(target.position)

func _delete_selected() -> void:
	if _plan_edit_mode:
		return

	var target := placement_controller.get_selected_target()
	if target == null:
		return

	placement_controller.clear_selection()
	target.queue_free()
	status_label.text = "Removed %s from %s." % [target.name, _get_current_floor_data()["name"]]

func _next_asset_name(asset_id: String, floor_index: int) -> String:
	var floor_id: String = FLOOR_PLANS[floor_index]["id"]
	var counts: Dictionary = _placed_counts_by_floor.get(floor_id, {})
	var count := int(counts.get(asset_id, 0)) + 1
	counts[asset_id] = count
	_placed_counts_by_floor[floor_id] = counts
	return "%s_%s_%02d" % [asset_id.capitalize(), FLOOR_PLANS[floor_index]["tab"], count]

func _get_current_floor_data() -> Dictionary:
	return FLOOR_PLANS[_current_floor_index]

func _add_box(parent: Node3D, position: Vector3, size: Vector3, material: StandardMaterial3D, name: String) -> void:
	var box := MeshInstance3D.new()
	box.name = name
	var mesh := BoxMesh.new()
	mesh.size = size
	box.mesh = mesh
	box.material_override = material
	box.position = position
	parent.add_child(box)

func _add_wall_segment(parent: Node3D, position: Vector3, size: Vector3, material: StandardMaterial3D, name: String) -> void:
	_add_box(parent, position, size, material, name)

func _add_room_label(parent: Node3D, position: Vector3, room_name: String) -> void:
	var label := Label3D.new()
	label.text = room_name
	label.font_size = 42
	label.pixel_size = 0.0085
	label.position = position
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	label.no_depth_test = true
	label.modulate = Color(0.18, 0.22, 0.38)
	label.outline_size = 6
	label.outline_modulate = Color(1, 1, 1, 0.85)
	parent.add_child(label)
