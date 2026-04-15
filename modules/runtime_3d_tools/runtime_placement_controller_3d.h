/**************************************************************************/
/*  runtime_placement_controller_3d.h                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to   */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include <cstdint>

#include "core/math/rect2.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/mesh.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/material.h"

class RuntimePlacementController3D : public Node3D {
	GDCLASS(RuntimePlacementController3D, Node3D);

	NodePath camera_path;
	NodePath targets_root_path;

	float floor_height = 0.0f;
	float outline_padding = 0.06f;
	float outline_thickness = 0.06f;
	float outline_height = 0.025f;
	float handle_distance = 0.55f;
	float handle_radius = 0.18f;
	float handle_pick_radius = 36.0f;
	float rotation_snap_degrees = 15.0f;
	Color selection_color = Color(0.19f, 0.48f, 1.0f, 0.95f);
	bool interaction_enabled = true;
	bool selection_enabled = true;
	bool translation_enabled = true;
	bool rotation_enabled = true;
	bool clear_selection_on_empty_tap = true;
	bool require_double_tap_to_move = true;
	bool move_guides_enabled = false;
	bool drag_bounds_enabled = false;
	Rect2 drag_bounds = Rect2();

	ObjectID selected_target_id;
	AABB selected_bounds;
	bool selected_bounds_valid = false;
	Vector<Vector<Point2>> selected_outline_polygons;
	Vector2 selected_handle_anchor;
	Vector3 selected_handle_local_position = Vector3();
	bool outline_mesh_dirty = true;

	struct HighlightedMeshState {
		ObjectID mesh_instance_id;
		Ref<Material> material_override;
		Vector<Ref<Material>> surface_override_materials;
	};

	Vector<HighlightedMeshState> highlighted_meshes;

	bool dragging = false;
	bool rotating = false;
	bool pending_drag = false;
	bool move_mode_armed = false;
	int active_pointer = -1;
	Vector2 pending_drag_start_position = Vector2();
	Vector3 drag_offset = Vector3();
	float rotate_start_yaw = 0.0f;
	float rotate_start_angle = 0.0f;
	ObjectID last_tap_target_id;
	uint64_t last_tap_time_msec = 0;
	Vector2 last_tap_position = Vector2();
	bool last_move_visual_active = false;

	Node3D *overlay_root = nullptr;
	Node3D *move_guides_root = nullptr;
	Node3D *api_boundary_root = nullptr;
	MeshInstance3D *outline_mesh_instance = nullptr;
	MeshInstance3D *handle_stem = nullptr;
	MeshInstance3D *handle_disc = nullptr;
	MeshInstance3D *api_boundary_overlay_mesh = nullptr;
	MeshInstance3D *move_axis_x = nullptr;
	MeshInstance3D *move_axis_z = nullptr;
	MeshInstance3D *move_arrow_pos_x = nullptr;
	MeshInstance3D *move_arrow_neg_x = nullptr;
	MeshInstance3D *move_arrow_pos_z = nullptr;
	MeshInstance3D *move_arrow_neg_z = nullptr;
	MeshInstance3D *move_grid_x_back = nullptr;
	MeshInstance3D *move_grid_x_mid = nullptr;
	MeshInstance3D *move_grid_x_front = nullptr;
	MeshInstance3D *move_grid_z_left = nullptr;
	MeshInstance3D *move_grid_z_mid = nullptr;
	MeshInstance3D *move_grid_z_right = nullptr;

	Ref<StandardMaterial3D> selection_material;
	Ref<StandardMaterial3D> move_guides_material;
	Ref<StandardMaterial3D> api_boundary_material;
	Ref<ArrayMesh> outline_mesh;
	Ref<BoxMesh> handle_stem_mesh;
	Ref<CylinderMesh> handle_disc_mesh;
	Ref<BoxMesh> move_axis_box_mesh;
	Ref<CylinderMesh> move_arrow_cone_mesh;

	ObjectID api_boundary_object_id;
	ObjectID api_boundary_source_mesh_id;
	AABB api_boundary_mesh_aabb;
	bool api_boundary_has_mesh_aabb = false;
	bool api_boundary_is_valid = true;

	static void _bind_methods();

	void _notification(int p_what);
	void input(const Ref<InputEvent> &p_event) override;

	Camera3D *_get_camera() const;
	Node *_get_targets_root() const;
	Node3D *_get_selected_target() const;

	void _ensure_overlay();
	void _sync_selection_material();
	void _set_overlay_visible(bool p_visible);
	void _set_move_guides_visible(bool p_visible);
	void _update_move_guides(const AABB &p_bounds);
	void _update_overlay();
	void _rebuild_outline_mesh();
	void _clear_mesh_highlights();
	void _apply_mesh_highlights(Node *p_node);
	Ref<Material> _create_highlight_material(const Ref<Material> &p_source_material, const MeshInstance3D *p_mesh_instance) const;

	bool _compute_local_bounds(Node3D *p_target, AABB &r_bounds) const;
	bool _compute_local_footprint(Node3D *p_target, const AABB &p_fallback_bounds, Vector<Vector<Point2>> &r_polygons) const;
	void _gather_visual_bounds(Node *p_node, const Transform3D &p_to_target, AABB &r_bounds, bool &r_found) const;
	void _gather_projected_triangles(Node *p_node, const Transform3D &p_to_target, Vector<Vector<Point2>> &r_triangles, Vector<Point2> &r_points) const;
	float _get_mesh_local_outline_thickness(const MeshInstance3D *p_mesh_instance) const;
	AABB _get_padded_bounds(const AABB &p_bounds) const;
	Vector<Vector2> _cleanup_polygon(const Vector<Vector2> &p_polygon) const;
	Vector2 _compute_handle_anchor(const Vector<Vector<Point2>> &p_polygons, const AABB &p_bounds) const;

	bool _project_to_floor(const Vector2 &p_screen_position, Vector3 &r_world_position) const;
	bool _pick_target_at_screen_position(const Vector2 &p_screen_position, Node3D *&r_target, Vector3 *r_world_hit = nullptr) const;
	bool _get_target_screen_bounds(Node3D *p_target, const AABB &p_local_bounds, Rect2 &r_bounds) const;
	bool _get_selected_screen_bounds(Rect2 &r_bounds) const;
	bool _is_selected_move_zone_hit(const Vector2 &p_screen_position) const;
	bool _is_rotate_handle_hit(const Vector2 &p_screen_position) const;
	float _get_yaw_from_screen_position(const Vector2 &p_screen_position) const;
	float _apply_rotation_snap(float p_yaw_degrees) const;

	void _queue_drag(const Vector2 &p_screen_position);
	bool _try_begin_queued_drag(const Vector2 &p_screen_position);
	void _begin_drag(const Vector2 &p_screen_position);
	void _begin_rotation(const Vector2 &p_screen_position);
	void _end_interaction();
	void _set_move_mode_armed(bool p_armed);
	void _clamp_world_position_to_drag_bounds(Vector3 &r_world_position) const;
	void _clear_api_boundary_overlay();
	void _sync_api_boundary_material();
	void _rebuild_api_boundary_overlay(Node3D *p_target, MeshInstance3D *p_mesh_instance);

	bool _is_pointer_event_over_gui(const Vector2 &p_screen_position) const;

public:
	void set_camera_path(const NodePath &p_camera_path);
	NodePath get_camera_path() const;

	void set_targets_root_path(const NodePath &p_targets_root_path);
	NodePath get_targets_root_path() const;

	void set_floor_height(float p_floor_height);
	float get_floor_height() const;

	void set_outline_padding(float p_outline_padding);
	float get_outline_padding() const;

	void set_outline_thickness(float p_outline_thickness);
	float get_outline_thickness() const;

	void set_outline_height(float p_outline_height);
	float get_outline_height() const;

	void set_handle_distance(float p_handle_distance);
	float get_handle_distance() const;

	void set_handle_radius(float p_handle_radius);
	float get_handle_radius() const;

	void set_handle_pick_radius(float p_handle_pick_radius);
	float get_handle_pick_radius() const;

	void set_rotation_snap_degrees(float p_rotation_snap_degrees);
	float get_rotation_snap_degrees() const;

	void set_selection_color(const Color &p_selection_color);
	Color get_selection_color() const;

	void set_interaction_enabled(bool p_enabled);
	bool is_interaction_enabled() const;

	void set_selection_enabled(bool p_enabled);
	bool is_selection_enabled() const;

	void set_translation_enabled(bool p_enabled);
	bool is_translation_enabled() const;

	void set_rotation_enabled(bool p_enabled);
	bool is_rotation_enabled() const;

	void set_clear_selection_on_empty_tap(bool p_enabled);
	bool is_clear_selection_on_empty_tap_enabled() const;

	void set_require_double_tap_to_move(bool p_enabled);
	bool is_double_tap_to_move_required() const;

	void set_move_guides_enabled(bool p_enabled);
	bool is_move_guides_enabled() const;

	void set_drag_bounds_enabled(bool p_enabled);
	bool is_drag_bounds_enabled() const;

	void set_drag_bounds(const Rect2 &p_bounds);
	Rect2 get_drag_bounds() const;

	void select_target(Node3D *p_target);
	Node3D *get_selected_target() const;
	void clear_selection();

	Node3D *pick_target_at_screen_position(const Vector2 &p_screen_position) const;
	bool select_target_at_screen_position(const Vector2 &p_screen_position);
	Variant project_screen_to_floor(const Vector2 &p_screen_position) const;
	bool move_selected_to_screen_position(const Vector2 &p_screen_position);
	bool move_selected_to_world_position(const Vector3 &p_world_position);
	bool place_target_on_floor(Node3D *p_target, const Vector3 &p_world_position);

	void arm_move_mode();
	void disarm_move_mode();
	bool is_move_mode_armed() const;

	void rotate_selected(float p_delta_degrees);
	void flip_selected_x();
	void flip_selected_z();

	bool create_boundary(Node *p_object, const Transform3D &p_global_transform, MeshInstance3D *p_mesh_instance, const AABB &p_mesh_aabb);
	bool update_boundary(Node *p_object, const Transform3D &p_transform, MeshInstance3D *p_mesh_instance, bool p_is_valid);
	void update_color(bool p_is_valid);
	void delete_boundary();

	// Aliases matching external API spelling used by integrators.
	bool create_boundry(Node *p_object, const Transform3D &p_global_transform, MeshInstance3D *p_mesh_instance, const AABB &p_mesh_aabb);
	bool update_boundry(Node *p_object, const Transform3D &p_transform, MeshInstance3D *p_mesh_instance, bool p_is_valid);
	void delete_boundry();

	bool is_dragging_selected() const;
	bool is_rotating_selected() const;
};
