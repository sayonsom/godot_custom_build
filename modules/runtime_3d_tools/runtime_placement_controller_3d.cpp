/**************************************************************************/
/*  runtime_placement_controller_3d.cpp                                   */
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

#include "runtime_placement_controller_3d.h"

#include <cfloat>

#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/os/os.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/gui/control.h"
#include "scene/main/viewport.h"

static constexpr float DRAG_ACTIVATION_DISTANCE = 4.0f;
static constexpr uint64_t DOUBLE_TAP_TIMEOUT_MS = 320;
static constexpr float DOUBLE_TAP_MAX_DISTANCE = 44.0f;
static const Color INVALID_BOUNDARY_COLOR(0.95f, 0.18f, 0.16f, 0.95f);

void RuntimePlacementController3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_camera_path", "camera_path"), &RuntimePlacementController3D::set_camera_path);
	ClassDB::bind_method(D_METHOD("get_camera_path"), &RuntimePlacementController3D::get_camera_path);
	ClassDB::bind_method(D_METHOD("set_targets_root_path", "targets_root_path"), &RuntimePlacementController3D::set_targets_root_path);
	ClassDB::bind_method(D_METHOD("get_targets_root_path"), &RuntimePlacementController3D::get_targets_root_path);
	ClassDB::bind_method(D_METHOD("set_floor_height", "floor_height"), &RuntimePlacementController3D::set_floor_height);
	ClassDB::bind_method(D_METHOD("get_floor_height"), &RuntimePlacementController3D::get_floor_height);
	ClassDB::bind_method(D_METHOD("set_outline_padding", "outline_padding"), &RuntimePlacementController3D::set_outline_padding);
	ClassDB::bind_method(D_METHOD("get_outline_padding"), &RuntimePlacementController3D::get_outline_padding);
	ClassDB::bind_method(D_METHOD("set_outline_thickness", "outline_thickness"), &RuntimePlacementController3D::set_outline_thickness);
	ClassDB::bind_method(D_METHOD("get_outline_thickness"), &RuntimePlacementController3D::get_outline_thickness);
	ClassDB::bind_method(D_METHOD("set_outline_height", "outline_height"), &RuntimePlacementController3D::set_outline_height);
	ClassDB::bind_method(D_METHOD("get_outline_height"), &RuntimePlacementController3D::get_outline_height);
	ClassDB::bind_method(D_METHOD("set_handle_distance", "handle_distance"), &RuntimePlacementController3D::set_handle_distance);
	ClassDB::bind_method(D_METHOD("get_handle_distance"), &RuntimePlacementController3D::get_handle_distance);
	ClassDB::bind_method(D_METHOD("set_handle_radius", "handle_radius"), &RuntimePlacementController3D::set_handle_radius);
	ClassDB::bind_method(D_METHOD("get_handle_radius"), &RuntimePlacementController3D::get_handle_radius);
	ClassDB::bind_method(D_METHOD("set_handle_pick_radius", "handle_pick_radius"), &RuntimePlacementController3D::set_handle_pick_radius);
	ClassDB::bind_method(D_METHOD("get_handle_pick_radius"), &RuntimePlacementController3D::get_handle_pick_radius);
	ClassDB::bind_method(D_METHOD("set_rotation_snap_degrees", "rotation_snap_degrees"), &RuntimePlacementController3D::set_rotation_snap_degrees);
	ClassDB::bind_method(D_METHOD("get_rotation_snap_degrees"), &RuntimePlacementController3D::get_rotation_snap_degrees);
	ClassDB::bind_method(D_METHOD("set_selection_color", "selection_color"), &RuntimePlacementController3D::set_selection_color);
	ClassDB::bind_method(D_METHOD("get_selection_color"), &RuntimePlacementController3D::get_selection_color);
	ClassDB::bind_method(D_METHOD("set_interaction_enabled", "enabled"), &RuntimePlacementController3D::set_interaction_enabled);
	ClassDB::bind_method(D_METHOD("is_interaction_enabled"), &RuntimePlacementController3D::is_interaction_enabled);
	ClassDB::bind_method(D_METHOD("set_selection_enabled", "enabled"), &RuntimePlacementController3D::set_selection_enabled);
	ClassDB::bind_method(D_METHOD("is_selection_enabled"), &RuntimePlacementController3D::is_selection_enabled);
	ClassDB::bind_method(D_METHOD("set_translation_enabled", "enabled"), &RuntimePlacementController3D::set_translation_enabled);
	ClassDB::bind_method(D_METHOD("is_translation_enabled"), &RuntimePlacementController3D::is_translation_enabled);
	ClassDB::bind_method(D_METHOD("set_rotation_enabled", "enabled"), &RuntimePlacementController3D::set_rotation_enabled);
	ClassDB::bind_method(D_METHOD("is_rotation_enabled"), &RuntimePlacementController3D::is_rotation_enabled);
	ClassDB::bind_method(D_METHOD("set_clear_selection_on_empty_tap", "enabled"), &RuntimePlacementController3D::set_clear_selection_on_empty_tap);
	ClassDB::bind_method(D_METHOD("is_clear_selection_on_empty_tap_enabled"), &RuntimePlacementController3D::is_clear_selection_on_empty_tap_enabled);
	ClassDB::bind_method(D_METHOD("set_require_double_tap_to_move", "enabled"), &RuntimePlacementController3D::set_require_double_tap_to_move);
	ClassDB::bind_method(D_METHOD("is_double_tap_to_move_required"), &RuntimePlacementController3D::is_double_tap_to_move_required);
	ClassDB::bind_method(D_METHOD("set_move_guides_enabled", "enabled"), &RuntimePlacementController3D::set_move_guides_enabled);
	ClassDB::bind_method(D_METHOD("is_move_guides_enabled"), &RuntimePlacementController3D::is_move_guides_enabled);
	ClassDB::bind_method(D_METHOD("set_drag_bounds_enabled", "enabled"), &RuntimePlacementController3D::set_drag_bounds_enabled);
	ClassDB::bind_method(D_METHOD("is_drag_bounds_enabled"), &RuntimePlacementController3D::is_drag_bounds_enabled);
	ClassDB::bind_method(D_METHOD("set_drag_bounds", "bounds"), &RuntimePlacementController3D::set_drag_bounds);
	ClassDB::bind_method(D_METHOD("get_drag_bounds"), &RuntimePlacementController3D::get_drag_bounds);
	ClassDB::bind_method(D_METHOD("select_target", "target"), &RuntimePlacementController3D::select_target);
	ClassDB::bind_method(D_METHOD("get_selected_target"), &RuntimePlacementController3D::get_selected_target);
	ClassDB::bind_method(D_METHOD("clear_selection"), &RuntimePlacementController3D::clear_selection);
	ClassDB::bind_method(D_METHOD("pick_target_at_screen_position", "screen_position"), &RuntimePlacementController3D::pick_target_at_screen_position);
	ClassDB::bind_method(D_METHOD("select_target_at_screen_position", "screen_position"), &RuntimePlacementController3D::select_target_at_screen_position);
	ClassDB::bind_method(D_METHOD("project_screen_to_floor", "screen_position"), &RuntimePlacementController3D::project_screen_to_floor);
	ClassDB::bind_method(D_METHOD("move_selected_to_screen_position", "screen_position"), &RuntimePlacementController3D::move_selected_to_screen_position);
	ClassDB::bind_method(D_METHOD("move_selected_to_world_position", "world_position"), &RuntimePlacementController3D::move_selected_to_world_position);
	ClassDB::bind_method(D_METHOD("place_target_on_floor", "target", "world_position"), &RuntimePlacementController3D::place_target_on_floor);
	ClassDB::bind_method(D_METHOD("arm_move_mode"), &RuntimePlacementController3D::arm_move_mode);
	ClassDB::bind_method(D_METHOD("disarm_move_mode"), &RuntimePlacementController3D::disarm_move_mode);
	ClassDB::bind_method(D_METHOD("is_move_mode_armed"), &RuntimePlacementController3D::is_move_mode_armed);
	ClassDB::bind_method(D_METHOD("rotate_selected", "delta_degrees"), &RuntimePlacementController3D::rotate_selected);
	ClassDB::bind_method(D_METHOD("flip_selected_x"), &RuntimePlacementController3D::flip_selected_x);
	ClassDB::bind_method(D_METHOD("flip_selected_z"), &RuntimePlacementController3D::flip_selected_z);
	ClassDB::bind_method(D_METHOD("create_boundary", "object", "global_transform", "mesh_instance", "mesh_aabb"), &RuntimePlacementController3D::create_boundary);
	ClassDB::bind_method(D_METHOD("update_boundary", "object", "transform", "mesh_instance", "is_valid"), &RuntimePlacementController3D::update_boundary);
	ClassDB::bind_method(D_METHOD("update_color", "is_valid"), &RuntimePlacementController3D::update_color);
	ClassDB::bind_method(D_METHOD("delete_boundary"), &RuntimePlacementController3D::delete_boundary);
	ClassDB::bind_method(D_METHOD("create_boundry", "object", "global_transform", "mesh_instance", "mesh_aabb"), &RuntimePlacementController3D::create_boundry);
	ClassDB::bind_method(D_METHOD("update_boundry", "object", "transform", "mesh_instance", "is_valid"), &RuntimePlacementController3D::update_boundry);
	ClassDB::bind_method(D_METHOD("delete_boundry"), &RuntimePlacementController3D::delete_boundry);
	ClassDB::bind_method(D_METHOD("is_dragging_selected"), &RuntimePlacementController3D::is_dragging_selected);
	ClassDB::bind_method(D_METHOD("is_rotating_selected"), &RuntimePlacementController3D::is_rotating_selected);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "camera_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Camera3D"), "set_camera_path", "get_camera_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "targets_root_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_targets_root_path", "get_targets_root_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "floor_height", PROPERTY_HINT_RANGE, "-100,100,0.01,suffix:m"), "set_floor_height", "get_floor_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "outline_padding", PROPERTY_HINT_RANGE, "0,2,0.01,suffix:m"), "set_outline_padding", "get_outline_padding");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "outline_thickness", PROPERTY_HINT_RANGE, "0.005,1,0.005,suffix:m"), "set_outline_thickness", "get_outline_thickness");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "outline_height", PROPERTY_HINT_RANGE, "0.005,1,0.005,suffix:m"), "set_outline_height", "get_outline_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "handle_distance", PROPERTY_HINT_RANGE, "0.05,3,0.01,suffix:m"), "set_handle_distance", "get_handle_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "handle_radius", PROPERTY_HINT_RANGE, "0.05,2,0.01,suffix:m"), "set_handle_radius", "get_handle_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "handle_pick_radius", PROPERTY_HINT_RANGE, "4,128,1,suffix:px"), "set_handle_pick_radius", "get_handle_pick_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rotation_snap_degrees", PROPERTY_HINT_RANGE, "0,180,0.1,suffix:deg"), "set_rotation_snap_degrees", "get_rotation_snap_degrees");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "selection_color"), "set_selection_color", "get_selection_color");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "interaction_enabled"), "set_interaction_enabled", "is_interaction_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "selection_enabled"), "set_selection_enabled", "is_selection_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "translation_enabled"), "set_translation_enabled", "is_translation_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rotation_enabled"), "set_rotation_enabled", "is_rotation_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clear_selection_on_empty_tap"), "set_clear_selection_on_empty_tap", "is_clear_selection_on_empty_tap_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "require_double_tap_to_move"), "set_require_double_tap_to_move", "is_double_tap_to_move_required");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "move_guides_enabled"), "set_move_guides_enabled", "is_move_guides_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "drag_bounds_enabled"), "set_drag_bounds_enabled", "is_drag_bounds_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "drag_bounds"), "set_drag_bounds", "get_drag_bounds");

	ADD_SIGNAL(MethodInfo("selection_changed", PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_RESOURCE_TYPE, "Node3D")));
	ADD_SIGNAL(MethodInfo("target_transform_changed", PropertyInfo(Variant::OBJECT, "target", PROPERTY_HINT_RESOURCE_TYPE, "Node3D")));
	ADD_SIGNAL(MethodInfo("move_mode_changed", PropertyInfo(Variant::BOOL, "armed")));
}

void RuntimePlacementController3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_process_input(true);
			set_process_internal(true);
			_ensure_overlay();
			_update_overlay();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_end_interaction();
			_clear_api_boundary_overlay();
			_clear_mesh_highlights();
			_set_overlay_visible(false);
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (api_boundary_root != nullptr) {
				Node3D *api_boundary_object = ObjectDB::get_instance<Node3D>(api_boundary_object_id);
				if (api_boundary_object == nullptr || !api_boundary_object->is_inside_tree()) {
					_clear_api_boundary_overlay();
				}
			}

			Node3D *selected_target = _get_selected_target();
			if (selected_target == nullptr || !selected_target->is_inside_tree()) {
				clear_selection();
			} else {
				_update_overlay();
			}
		} break;
		default: {
		} break;
	}
}

void RuntimePlacementController3D::input(const Ref<InputEvent> &p_event) {
	Viewport *viewport = get_viewport();
	if (viewport == nullptr || !is_inside_tree()) {
		return;
	}
	if (!interaction_enabled) {
		if (dragging || rotating || pending_drag) {
			_end_interaction();
		}
		return;
	}
	if ((!translation_enabled && (dragging || pending_drag)) || (!rotation_enabled && rotating)) {
		_end_interaction();
	}
	if (viewport->gui_is_dragging()) {
		if (!dragging && !rotating) {
			active_pointer = -1;
		}
		pending_drag = false;
		return;
	}

	Ref<InputEventMouseButton> mouse_button = p_event;
	if (mouse_button.is_valid() && mouse_button->get_device() == InputEvent::DEVICE_ID_EMULATION) {
		return;
	}
	if (mouse_button.is_valid() && mouse_button->get_button_index() == MouseButton::LEFT) {
		const Vector2 position = mouse_button->get_position();
		if (mouse_button->is_pressed()) {
			active_pointer = 0;
			if (_is_pointer_event_over_gui(position) && !dragging && !rotating) {
				return;
			}

			Node3D *selected_target = _get_selected_target();
			Node3D *picked_target = nullptr;
			Vector3 hit_position;
			const bool has_pick = _pick_target_at_screen_position(position, picked_target, &hit_position);

			// If movement was armed (double tap), prioritize drag start over rotation zone checks.
			// Accept either a direct pick hit or the selected movement zone on screen.
			const bool armed_move_hit = selected_target != nullptr && translation_enabled && move_mode_armed &&
					((has_pick && picked_target == selected_target) || _is_selected_move_zone_hit(position));
			if (armed_move_hit) {
				_queue_drag(position);
				viewport->set_input_as_handled();
				return;
			}

			if (has_pick) {
				const uint64_t now = OS::get_singleton()->get_ticks_msec();
				if (picked_target != selected_target) {
					if (selection_enabled) {
						select_target(picked_target);
						if (translation_enabled && !require_double_tap_to_move) {
							_set_move_mode_armed(true);
							_queue_drag(position);
						} else {
							_set_move_mode_armed(false);
						}
					} else {
						_set_move_mode_armed(false);
					}
				} else {
					if (translation_enabled && !require_double_tap_to_move) {
						_set_move_mode_armed(true);
						_queue_drag(position);
					} else if (translation_enabled) {
						const bool same_tap_target = last_tap_target_id == picked_target->get_instance_id();
						const bool within_timeout = now >= last_tap_time_msec && (now - last_tap_time_msec) <= DOUBLE_TAP_TIMEOUT_MS;
						const bool near_last_tap = last_tap_position.distance_squared_to(position) <= DOUBLE_TAP_MAX_DISTANCE * DOUBLE_TAP_MAX_DISTANCE;
						if (same_tap_target && within_timeout && near_last_tap) {
							_set_move_mode_armed(true);
							_queue_drag(position);
						} else if (rotation_enabled && _is_rotate_handle_hit(position)) {
							_begin_rotation(position);
						}
					} else if (rotation_enabled && _is_rotate_handle_hit(position)) {
						_begin_rotation(position);
					}
				}

				last_tap_target_id = picked_target->get_instance_id();
				last_tap_time_msec = now;
				last_tap_position = position;
				viewport->set_input_as_handled();
				return;
			}

			if (selected_target != nullptr) {
				bool near_selected = _is_selected_move_zone_hit(position);
				if (!near_selected) {
					Rect2 selected_screen_bounds;
					if (_get_selected_screen_bounds(selected_screen_bounds)) {
						const Rect2 expanded_bounds = selected_screen_bounds.grow(MAX(handle_pick_radius * 1.15f, 64.0f));
						near_selected = expanded_bounds.has_point(position);
					}
				}

				if (near_selected) {
					const uint64_t now = OS::get_singleton()->get_ticks_msec();
					const bool same_tap_target = last_tap_target_id == selected_target->get_instance_id();
					const bool within_timeout = now >= last_tap_time_msec && (now - last_tap_time_msec) <= DOUBLE_TAP_TIMEOUT_MS;
					const bool near_last_tap = last_tap_position.distance_squared_to(position) <= DOUBLE_TAP_MAX_DISTANCE * DOUBLE_TAP_MAX_DISTANCE;
					if (translation_enabled && !require_double_tap_to_move) {
						_set_move_mode_armed(true);
						_queue_drag(position);
					} else if (translation_enabled && same_tap_target && within_timeout && near_last_tap) {
						_set_move_mode_armed(true);
						_queue_drag(position);
					} else if (rotation_enabled && _is_rotate_handle_hit(position)) {
						_begin_rotation(position);
					}
					last_tap_target_id = selected_target->get_instance_id();
					last_tap_time_msec = now;
					last_tap_position = position;
					viewport->set_input_as_handled();
					return;
				}

				if (clear_selection_on_empty_tap) {
					clear_selection();
					viewport->set_input_as_handled();
					return;
				}
			}
		} else if (active_pointer == 0) {
			_end_interaction();
		}
		return;
	}

	Ref<InputEventMouseMotion> mouse_motion = p_event;
	if (mouse_motion.is_valid() && mouse_motion->get_device() == InputEvent::DEVICE_ID_EMULATION) {
		return;
	}
	if (mouse_motion.is_valid() && active_pointer == 0 && (dragging || rotating || pending_drag)) {
		const Vector2 position = mouse_motion->get_position();
		if (pending_drag && !dragging && !rotating) {
			if (!_try_begin_queued_drag(position)) {
				viewport->set_input_as_handled();
				return;
			}
		}

		Node3D *selected_target = _get_selected_target();
		if (selected_target == nullptr) {
			_end_interaction();
			return;
		}

		if (dragging) {
			Vector3 world_position;
			if (_project_to_floor(position, world_position)) {
				Vector3 next_position = world_position + drag_offset;
				_clamp_world_position_to_drag_bounds(next_position);
				selected_target->set_global_position(next_position);
				emit_signal(SNAME("target_transform_changed"), selected_target);
			}
		} else if (rotating) {
			float yaw = _get_yaw_from_screen_position(position);
			selected_target->set_rotation_degrees(Vector3(selected_target->get_rotation_degrees().x, _apply_rotation_snap(rotate_start_yaw + yaw - rotate_start_angle), selected_target->get_rotation_degrees().z));
			emit_signal(SNAME("target_transform_changed"), selected_target);
		}

		viewport->set_input_as_handled();
		return;
	}

	Ref<InputEventScreenTouch> screen_touch = p_event;
	if (screen_touch.is_valid()) {
		const Vector2 position = screen_touch->get_position();
		const bool was_interacting = dragging || rotating || pending_drag;
		if (was_interacting && screen_touch->get_index() != active_pointer) {
			viewport->set_input_as_handled();
			return;
		}
		if (screen_touch->is_pressed()) {
			if (was_interacting) {
				viewport->set_input_as_handled();
				return;
			}
			if (_is_pointer_event_over_gui(position) && !dragging && !rotating) {
				active_pointer = -1;
				return;
			}
			active_pointer = screen_touch->get_index();

			Node3D *selected_target = _get_selected_target();
			Node3D *picked_target = nullptr;
			Vector3 hit_position;
			const bool has_pick = _pick_target_at_screen_position(position, picked_target, &hit_position);

			// If movement was armed (double tap), prioritize drag start over rotation zone checks.
			// Accept either a direct pick hit or the selected movement zone on screen.
			const bool armed_move_hit = selected_target != nullptr && translation_enabled && move_mode_armed &&
					((has_pick && picked_target == selected_target) || _is_selected_move_zone_hit(position));
			if (armed_move_hit) {
				_queue_drag(position);
				viewport->set_input_as_handled();
				return;
			}

			if (has_pick) {
				const uint64_t now = OS::get_singleton()->get_ticks_msec();
				if (picked_target != selected_target) {
					if (selection_enabled) {
						select_target(picked_target);
						if (translation_enabled && !require_double_tap_to_move) {
							_set_move_mode_armed(true);
							_queue_drag(position);
						} else {
							_set_move_mode_armed(false);
						}
					} else {
						_set_move_mode_armed(false);
					}
				} else {
					if (translation_enabled && !require_double_tap_to_move) {
						_set_move_mode_armed(true);
						_queue_drag(position);
					} else if (translation_enabled) {
						const bool same_tap_target = last_tap_target_id == picked_target->get_instance_id();
						const bool within_timeout = now >= last_tap_time_msec && (now - last_tap_time_msec) <= DOUBLE_TAP_TIMEOUT_MS;
						const bool near_last_tap = last_tap_position.distance_squared_to(position) <= DOUBLE_TAP_MAX_DISTANCE * DOUBLE_TAP_MAX_DISTANCE;
						if (same_tap_target && within_timeout && near_last_tap) {
							_set_move_mode_armed(true);
							_queue_drag(position);
						} else if (rotation_enabled && _is_rotate_handle_hit(position)) {
							_begin_rotation(position);
						}
					} else if (rotation_enabled && _is_rotate_handle_hit(position)) {
						_begin_rotation(position);
					}
				}

				last_tap_target_id = picked_target->get_instance_id();
				last_tap_time_msec = now;
				last_tap_position = position;
				viewport->set_input_as_handled();
				return;
			}

			if (selected_target != nullptr) {
				bool near_selected = _is_selected_move_zone_hit(position);
				if (!near_selected) {
					Rect2 selected_screen_bounds;
					if (_get_selected_screen_bounds(selected_screen_bounds)) {
						const Rect2 expanded_bounds = selected_screen_bounds.grow(MAX(handle_pick_radius * 1.15f, 64.0f));
						near_selected = expanded_bounds.has_point(position);
					}
				}

				if (near_selected) {
					const uint64_t now = OS::get_singleton()->get_ticks_msec();
					const bool same_tap_target = last_tap_target_id == selected_target->get_instance_id();
					const bool within_timeout = now >= last_tap_time_msec && (now - last_tap_time_msec) <= DOUBLE_TAP_TIMEOUT_MS;
					const bool near_last_tap = last_tap_position.distance_squared_to(position) <= DOUBLE_TAP_MAX_DISTANCE * DOUBLE_TAP_MAX_DISTANCE;
					if (translation_enabled && !require_double_tap_to_move) {
						_set_move_mode_armed(true);
						_queue_drag(position);
					} else if (translation_enabled && same_tap_target && within_timeout && near_last_tap) {
						_set_move_mode_armed(true);
						_queue_drag(position);
					} else if (rotation_enabled && _is_rotate_handle_hit(position)) {
						_begin_rotation(position);
					}
					last_tap_target_id = selected_target->get_instance_id();
					last_tap_time_msec = now;
					last_tap_position = position;
					viewport->set_input_as_handled();
					return;
				}

				if (clear_selection_on_empty_tap) {
					clear_selection();
					viewport->set_input_as_handled();
					return;
				}
			}
		} else if (active_pointer == screen_touch->get_index()) {
			_end_interaction();
			viewport->set_input_as_handled();
		}
		return;
	}

	Ref<InputEventScreenDrag> screen_drag = p_event;
	if (screen_drag.is_valid() && (dragging || rotating || pending_drag)) {
		if (active_pointer != screen_drag->get_index()) {
			viewport->set_input_as_handled();
			return;
		}

		const Vector2 position = screen_drag->get_position();
		if (pending_drag && !dragging && !rotating) {
			if (!_try_begin_queued_drag(position)) {
				viewport->set_input_as_handled();
				return;
			}
		}

		Node3D *selected_target = _get_selected_target();
		if (selected_target == nullptr) {
			_end_interaction();
			return;
		}

		if (dragging) {
			Vector3 world_position;
			if (_project_to_floor(position, world_position)) {
				Vector3 next_position = world_position + drag_offset;
				_clamp_world_position_to_drag_bounds(next_position);
				selected_target->set_global_position(next_position);
				emit_signal(SNAME("target_transform_changed"), selected_target);
			}
		} else if (rotating) {
			float yaw = _get_yaw_from_screen_position(position);
			selected_target->set_rotation_degrees(Vector3(selected_target->get_rotation_degrees().x, _apply_rotation_snap(rotate_start_yaw + yaw - rotate_start_angle), selected_target->get_rotation_degrees().z));
			emit_signal(SNAME("target_transform_changed"), selected_target);
		}

		viewport->set_input_as_handled();
	}
}

Camera3D *RuntimePlacementController3D::_get_camera() const {
	if (!camera_path.is_empty()) {
		return Object::cast_to<Camera3D>(get_node_or_null(camera_path));
	}

	Viewport *viewport = get_viewport();
	return viewport ? viewport->get_camera_3d() : nullptr;
}

Node *RuntimePlacementController3D::_get_targets_root() const {
	if (targets_root_path.is_empty()) {
		return nullptr;
	}
	return get_node_or_null(targets_root_path);
}

Node3D *RuntimePlacementController3D::_get_selected_target() const {
	return ObjectDB::get_instance<Node3D>(selected_target_id);
}

void RuntimePlacementController3D::_ensure_overlay() {
	if (overlay_root != nullptr) {
		return;
	}

	overlay_root = memnew(Node3D);
	overlay_root->set_name(SNAME("RuntimeSelectionOverlay"));
	add_child(overlay_root, false, INTERNAL_MODE_BACK);
	overlay_root->set_owner(nullptr);
	overlay_root->set_visible(false);

	outline_mesh.instantiate();
	handle_stem_mesh.instantiate();
	handle_disc_mesh.instantiate();

	outline_mesh_instance = memnew(MeshInstance3D);
	outline_mesh_instance->set_name(SNAME("OutlineMesh"));
	outline_mesh_instance->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	overlay_root->add_child(outline_mesh_instance, false, INTERNAL_MODE_BACK);
	outline_mesh_instance->set_owner(nullptr);

	handle_stem = memnew(MeshInstance3D);
	handle_stem->set_name(SNAME("RotateHandleStem"));
	handle_stem->set_mesh(handle_stem_mesh);
	handle_stem->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	overlay_root->add_child(handle_stem, false, INTERNAL_MODE_BACK);
	handle_stem->set_owner(nullptr);

	handle_disc = memnew(MeshInstance3D);
	handle_disc->set_name(SNAME("RotateHandleDisc"));
	handle_disc->set_mesh(handle_disc_mesh);
	handle_disc->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	overlay_root->add_child(handle_disc, false, INTERNAL_MODE_BACK);
	handle_disc->set_owner(nullptr);

	_sync_selection_material();
}

void RuntimePlacementController3D::_sync_selection_material() {
	if (selection_material.is_null()) {
		selection_material.instantiate();
		selection_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		selection_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
		selection_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
		selection_material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
		selection_material->set_render_priority(Material::RENDER_PRIORITY_MAX);
	}

	selection_material->set_albedo(selection_color);

	if (outline_mesh_instance != nullptr) {
		outline_mesh_instance->set_material_override(selection_material);
	}

	if (handle_stem != nullptr) {
		handle_stem->set_material_override(selection_material);
	}
	if (handle_disc != nullptr) {
		handle_disc->set_material_override(selection_material);
	}
}

void RuntimePlacementController3D::_set_overlay_visible(bool p_visible) {
	if (overlay_root != nullptr) {
		overlay_root->set_visible(p_visible);
	}
}

void RuntimePlacementController3D::_set_move_guides_visible(bool p_visible) {
	(void)p_visible;
	if (move_guides_root != nullptr) {
		move_guides_root->set_visible(false);
	}
}

void RuntimePlacementController3D::_update_move_guides(const AABB &p_bounds) {
	// Move guides are intentionally disabled at engine level.
	(void)p_bounds;
}

void RuntimePlacementController3D::_update_overlay() {
	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr || !selected_target->is_inside_tree()) {
		selected_bounds_valid = false;
		_clear_mesh_highlights();
		selected_outline_polygons.clear();
		_set_overlay_visible(false);
		return;
	}

	_ensure_overlay();

	if (!selected_bounds_valid) {
		if (!_compute_local_bounds(selected_target, selected_bounds)) {
			selected_outline_polygons.clear();
			selected_bounds_valid = false;
			_set_overlay_visible(false);
			return;
		}

	}

	selected_bounds_valid = true;
	const bool move_visual_active = translation_enabled && (move_mode_armed || dragging || pending_drag);
	if (selection_material.is_valid()) {
		selection_material->set_albedo(selection_color);
	}

	if (highlighted_meshes.is_empty() || last_move_visual_active != move_visual_active) {
		if (!highlighted_meshes.is_empty()) {
			_clear_mesh_highlights();
		}
		_apply_mesh_highlights(selected_target);
	}
	last_move_visual_active = move_visual_active;

	const Vector3 bounds_center = selected_bounds.position + selected_bounds.size * 0.5f;
	const Vector3 handle_anchor(bounds_center.x, selected_bounds.position.y + selected_bounds.size.y, bounds_center.z);
	selected_handle_local_position = handle_anchor + Vector3(0.0f, handle_distance, 0.0f);

	overlay_root->set_global_transform(selected_target->get_global_transform());
	outline_mesh_instance->set_visible(false);
	outline_mesh_instance->set_mesh(Ref<Mesh>());
	handle_stem->set_visible(false);
	handle_disc->set_visible(false);
	if (rotation_enabled) {
		handle_stem->set_position((handle_anchor + selected_handle_local_position) * 0.5f);
		handle_stem->set_scale(Vector3(outline_thickness * 0.8f, MAX(selected_handle_local_position.y - handle_anchor.y, outline_height), outline_thickness * 0.8f));
		handle_disc->set_position(selected_handle_local_position);
		handle_disc->set_scale(Vector3(handle_radius * 2.0f, outline_height, handle_radius * 2.0f));
	}
	_update_move_guides(selected_bounds);
	_set_move_guides_visible(move_guides_enabled && move_visual_active);

	_set_overlay_visible(true);
}

void RuntimePlacementController3D::_clear_mesh_highlights() {
	for (int i = 0; i < highlighted_meshes.size(); i++) {
		MeshInstance3D *mesh_instance = ObjectDB::get_instance<MeshInstance3D>(highlighted_meshes[i].mesh_instance_id);
		if (mesh_instance == nullptr) {
			continue;
		}

		mesh_instance->set_material_override(highlighted_meshes[i].material_override);

		const int surface_count = mesh_instance->get_surface_override_material_count();
		const int restore_count = MIN(surface_count, highlighted_meshes[i].surface_override_materials.size());
		for (int surface_index = 0; surface_index < restore_count; surface_index++) {
			mesh_instance->set_surface_override_material(surface_index, highlighted_meshes[i].surface_override_materials[surface_index]);
		}
		for (int surface_index = restore_count; surface_index < surface_count; surface_index++) {
			mesh_instance->set_surface_override_material(surface_index, Ref<Material>());
		}
	}

	highlighted_meshes.clear();
}

void RuntimePlacementController3D::_apply_mesh_highlights(Node *p_node) {
	MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(p_node);
	if (mesh_instance != nullptr && mesh_instance->is_visible_in_tree()) {
		HighlightedMeshState state;
		state.mesh_instance_id = mesh_instance->get_instance_id();
		state.material_override = mesh_instance->get_material_override();

		const int surface_override_count = mesh_instance->get_surface_override_material_count();
		state.surface_override_materials.resize(surface_override_count);
		for (int surface_index = 0; surface_index < surface_override_count; surface_index++) {
			state.surface_override_materials.write[surface_index] = mesh_instance->get_surface_override_material(surface_index);
		}

		if (state.material_override.is_valid()) {
			mesh_instance->set_material_override(_create_highlight_material(state.material_override, mesh_instance));
		} else {
			for (int surface_index = 0; surface_index < surface_override_count; surface_index++) {
				mesh_instance->set_surface_override_material(surface_index, _create_highlight_material(mesh_instance->get_active_material(surface_index), mesh_instance));
			}
		}

		highlighted_meshes.push_back(state);
	}

	const int child_count = p_node->get_child_count(false);
	for (int i = 0; i < child_count; i++) {
		_apply_mesh_highlights(p_node->get_child(i, false));
	}
}

Ref<Material> RuntimePlacementController3D::_create_highlight_material(const Ref<Material> &p_source_material, const MeshInstance3D *p_mesh_instance) const {
	Ref<BaseMaterial3D> highlight_material = p_source_material;
	if (highlight_material.is_valid()) {
		highlight_material = highlight_material->duplicate();
	}

	if (highlight_material.is_null()) {
		highlight_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	}

	if (highlight_material->get_render_priority() >= Material::RENDER_PRIORITY_MAX) {
		highlight_material->set_render_priority(Material::RENDER_PRIORITY_MAX - 1);
	}

	highlight_material->set_stencil_effect_outline_thickness(_get_mesh_local_outline_thickness(p_mesh_instance));
	highlight_material->set_stencil_mode(BaseMaterial3D::STENCIL_MODE_OUTLINE);
	highlight_material->set_stencil_effect_color(selection_color);
	return highlight_material;
}

void RuntimePlacementController3D::_rebuild_outline_mesh() {
	outline_mesh_dirty = false;

	if (outline_mesh_instance == nullptr) {
		return;
	}

	Ref<ArrayMesh> mesh;
	mesh.instantiate();

	Vector<Vector<Point2>> padded_polygons;
	for (int i = 0; i < selected_outline_polygons.size(); i++) {
		Vector<Vector2> polygon = _cleanup_polygon(selected_outline_polygons[i]);
		if (polygon.size() < 3) {
			continue;
		}

		Vector<Vector<Point2>> offset_polygons;
		if (outline_padding > CMP_EPSILON) {
			offset_polygons = Geometry2D::offset_polygon(polygon, outline_padding, Geometry2D::JOIN_MITER);
		}
		if (offset_polygons.is_empty()) {
			offset_polygons.push_back(polygon);
		}

		for (int j = 0; j < offset_polygons.size(); j++) {
			Vector<Vector2> cleaned = _cleanup_polygon(offset_polygons[j]);
			if (cleaned.size() >= 3) {
				padded_polygons.push_back(cleaned);
			}
		}
	}

	Vector<Vector3> vertices;
	Vector<int> indices;
	AABB mesh_bounds;
	bool has_mesh_bounds = false;
	const float center_y = selected_bounds.position.y + outline_height * 0.5f;
	const float half_thickness = outline_thickness * 0.5f;

	for (int polygon_index = 0; polygon_index < padded_polygons.size(); polygon_index++) {
		const Vector<Point2> &polygon = padded_polygons[polygon_index];
		for (int i = 0; i < polygon.size(); i++) {
			const Vector2 from = polygon[i];
			const Vector2 to = polygon[(i + 1) % polygon.size()];
			const Vector2 edge = to - from;
			const float edge_length = edge.length();
			if (edge_length <= CMP_EPSILON) {
				continue;
			}

			const Vector2 normal = Vector2(-edge.y, edge.x) / edge_length;
			const Vector2 offset = normal * half_thickness;
			const int base = vertices.size();

			const Vector3 v0(from.x + offset.x, center_y, from.y + offset.y);
			const Vector3 v1(from.x - offset.x, center_y, from.y - offset.y);
			const Vector3 v2(to.x - offset.x, center_y, to.y - offset.y);
			const Vector3 v3(to.x + offset.x, center_y, to.y + offset.y);

			vertices.push_back(v0);
			vertices.push_back(v1);
			vertices.push_back(v2);
			vertices.push_back(v3);

			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 0);
			indices.push_back(base + 2);
			indices.push_back(base + 3);

			if (!has_mesh_bounds) {
				mesh_bounds = AABB(v0, Vector3());
				has_mesh_bounds = true;
			}
			mesh_bounds.expand_to(v1);
			mesh_bounds.expand_to(v2);
			mesh_bounds.expand_to(v3);
		}
	}

	if (vertices.is_empty()) {
		selected_handle_anchor = Vector2(selected_bounds.position.x + selected_bounds.size.x * 0.5f, selected_bounds.position.z + selected_bounds.size.z);
		selected_handle_local_position = Vector3(selected_handle_anchor.x, center_y, selected_handle_anchor.y + handle_distance);
		outline_mesh_instance->set_mesh(Ref<Mesh>());
		outline_mesh = mesh;
		return;
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = vertices;
	arrays[Mesh::ARRAY_INDEX] = indices;
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	if (has_mesh_bounds) {
		mesh->set_custom_aabb(mesh_bounds);
	}

	selected_handle_anchor = _compute_handle_anchor(padded_polygons, selected_bounds);
	selected_handle_local_position = Vector3(selected_handle_anchor.x, center_y, selected_handle_anchor.y + handle_distance);
	outline_mesh_instance->set_mesh(mesh);
	outline_mesh = mesh;
}

bool RuntimePlacementController3D::_compute_local_bounds(Node3D *p_target, AABB &r_bounds) const {
	bool found = false;
	AABB bounds;
	_gather_visual_bounds(p_target, Transform3D(), bounds, found);

	if (!found) {
		return false;
	}

	if (bounds.size.y < outline_height) {
		bounds.size.y = outline_height;
	}
	if (bounds.size.x < outline_thickness) {
		bounds.position.x -= 0.5f * (outline_thickness - bounds.size.x);
		bounds.size.x = outline_thickness;
	}
	if (bounds.size.z < outline_thickness) {
		bounds.position.z -= 0.5f * (outline_thickness - bounds.size.z);
		bounds.size.z = outline_thickness;
	}

	r_bounds = bounds;
	return true;
}

bool RuntimePlacementController3D::_compute_local_footprint(Node3D *p_target, const AABB &p_fallback_bounds, Vector<Vector<Point2>> &r_polygons) const {
	r_polygons.clear();

	Vector<Vector<Point2>> projected_triangles;
	Vector<Point2> projected_points;
	_gather_projected_triangles(p_target, Transform3D(), projected_triangles, projected_points);

	if (projected_points.size() >= 3) {
		Vector<Point2> hull = Geometry2D::convex_hull(projected_points);
		if (hull.size() > 1 && hull[0].is_equal_approx(hull[hull.size() - 1])) {
			hull.resize(hull.size() - 1);
		}
		Vector<Vector2> cleaned = _cleanup_polygon(hull);
		if (cleaned.size() >= 3) {
			r_polygons.push_back(cleaned);
		}
	}

	if (r_polygons.is_empty()) {
		Vector<Point2> rectangle;
		rectangle.push_back(Vector2(p_fallback_bounds.position.x, p_fallback_bounds.position.z));
		rectangle.push_back(Vector2(p_fallback_bounds.position.x + p_fallback_bounds.size.x, p_fallback_bounds.position.z));
		rectangle.push_back(Vector2(p_fallback_bounds.position.x + p_fallback_bounds.size.x, p_fallback_bounds.position.z + p_fallback_bounds.size.z));
		rectangle.push_back(Vector2(p_fallback_bounds.position.x, p_fallback_bounds.position.z + p_fallback_bounds.size.z));
		r_polygons.push_back(rectangle);
	}

	return !r_polygons.is_empty();
}

void RuntimePlacementController3D::_gather_visual_bounds(Node *p_node, const Transform3D &p_to_target, AABB &r_bounds, bool &r_found) const {
	Node3D *node_3d = Object::cast_to<Node3D>(p_node);
	if (node_3d != nullptr) {
		VisualInstance3D *visual_instance = Object::cast_to<VisualInstance3D>(node_3d);
		if (visual_instance != nullptr) {
			GeometryInstance3D *geometry_instance = Object::cast_to<GeometryInstance3D>(node_3d);
			if (geometry_instance != nullptr) {
				const AABB local_aabb = visual_instance->get_aabb();
				if (local_aabb.has_surface() || local_aabb.has_volume()) {
					for (int i = 0; i < 8; i++) {
						const Vector3 point = p_to_target.xform(local_aabb.get_endpoint(i));
						if (!r_found) {
							r_bounds = AABB(point, Vector3());
							r_found = true;
						} else {
							r_bounds.expand_to(point);
						}
					}
				}
			}
		}
	}

	const int child_count = p_node->get_child_count(false);
	for (int i = 0; i < child_count; i++) {
		Node *child = p_node->get_child(i, false);
		Node3D *child_3d = Object::cast_to<Node3D>(child);
		if (child_3d == nullptr || !child_3d->is_visible_in_tree()) {
			continue;
		}

		_gather_visual_bounds(child_3d, p_to_target * child_3d->get_transform(), r_bounds, r_found);
	}
}

void RuntimePlacementController3D::_gather_projected_triangles(Node *p_node, const Transform3D &p_to_target, Vector<Vector<Point2>> &r_triangles, Vector<Point2> &r_points) const {
	Node3D *node_3d = Object::cast_to<Node3D>(p_node);
	if (node_3d == nullptr || !node_3d->is_visible_in_tree()) {
		return;
	}

	MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(node_3d);
	if (mesh_instance != nullptr) {
		Ref<Mesh> mesh = mesh_instance->get_mesh();
		if (mesh.is_valid()) {
			const Vector<Face3> faces = mesh->get_faces();
			for (int i = 0; i < faces.size(); i++) {
				Vector<Point2> triangle;
				triangle.resize(3);

				for (int vertex_index = 0; vertex_index < 3; vertex_index++) {
					const Vector3 point = p_to_target.xform(faces[i].vertex[vertex_index]);
					triangle.write[vertex_index] = Vector2(point.x, point.z);
					r_points.push_back(triangle[vertex_index]);
				}

				if (Math::abs((triangle[1] - triangle[0]).cross(triangle[2] - triangle[0])) > CMP_EPSILON) {
					r_triangles.push_back(triangle);
				}
			}
		}
	}

	const int child_count = p_node->get_child_count(false);
	for (int i = 0; i < child_count; i++) {
		Node *child = p_node->get_child(i, false);
		Node3D *child_3d = Object::cast_to<Node3D>(child);
		if (child_3d == nullptr || !child_3d->is_visible_in_tree()) {
			continue;
		}

		_gather_projected_triangles(child_3d, p_to_target * child_3d->get_transform(), r_triangles, r_points);
	}
}

float RuntimePlacementController3D::_get_mesh_local_outline_thickness(const MeshInstance3D *p_mesh_instance) const {
	if (p_mesh_instance == nullptr) {
		return outline_thickness;
	}

	const Basis basis = p_mesh_instance->is_inside_tree() ? p_mesh_instance->get_global_basis() : p_mesh_instance->get_basis();
	const Vector3 scale = basis.get_scale_abs().maxf(0.001f);
	const float max_scale = MAX(scale.x, MAX(scale.y, scale.z));
	return MAX(outline_thickness / max_scale, 0.001f);
}

AABB RuntimePlacementController3D::_get_padded_bounds(const AABB &p_bounds) const {
	AABB padded = p_bounds;
	padded.position.x -= outline_padding;
	padded.position.z -= outline_padding;
	padded.size.x += outline_padding * 2.0f;
	padded.size.z += outline_padding * 2.0f;
	return padded;
}

Vector<Vector2> RuntimePlacementController3D::_cleanup_polygon(const Vector<Vector2> &p_polygon) const {
	Vector<Vector2> points;
	points.reserve(p_polygon.size());

	for (int i = 0; i < p_polygon.size(); i++) {
		if (points.is_empty() || !points[points.size() - 1].is_equal_approx(p_polygon[i])) {
			points.push_back(p_polygon[i]);
		}
	}

	if (points.size() > 1 && points[0].is_equal_approx(points[points.size() - 1])) {
		points.resize(points.size() - 1);
	}

	if (points.size() < 3) {
		return Vector<Vector2>();
	}

	Vector<Vector2> simplified;
	simplified.reserve(points.size());
	const float distance_epsilon = MAX(outline_thickness * 0.35f, 0.01f);
	for (int i = 0; i < points.size(); i++) {
		const Vector2 prev = points[(i + points.size() - 1) % points.size()];
		const Vector2 current = points[i];
		const Vector2 next = points[(i + 1) % points.size()];
		if (current.distance_to(prev) <= distance_epsilon || current.distance_to(next) <= distance_epsilon) {
			continue;
		}

		const Vector2 incoming = current - prev;
		const Vector2 outgoing = next - current;
		if (incoming.length_squared() > CMP_EPSILON2 && outgoing.length_squared() > CMP_EPSILON2) {
			const Vector2 incoming_normalized = incoming.normalized();
			const Vector2 outgoing_normalized = outgoing.normalized();
			const float distance_to_segment = Geometry2D::get_distance_to_segment(current, prev, next);
			if ((Math::abs(incoming_normalized.cross(outgoing_normalized)) <= 0.02f && incoming_normalized.dot(outgoing_normalized) > 0.0f) || distance_to_segment <= distance_epsilon) {
				continue;
			}
		}

		simplified.push_back(current);
	}

	if (simplified.size() < 3) {
		return Vector<Vector2>();
	}

	if (Geometry2D::is_polygon_clockwise(simplified)) {
		Vector<Vector2> reversed;
		reversed.resize(simplified.size());
		for (int i = 0; i < simplified.size(); i++) {
			reversed.write[i] = simplified[simplified.size() - 1 - i];
		}
		return reversed;
	}

	return simplified;
}

Vector2 RuntimePlacementController3D::_compute_handle_anchor(const Vector<Vector<Point2>> &p_polygons, const AABB &p_bounds) const {
	float max_z = -FLT_MAX;
	for (int polygon_index = 0; polygon_index < p_polygons.size(); polygon_index++) {
		const Vector<Point2> &polygon = p_polygons[polygon_index];
		for (int i = 0; i < polygon.size(); i++) {
			max_z = MAX(max_z, polygon[i].y);
		}
	}

	if (max_z == -FLT_MAX) {
		return Vector2(p_bounds.position.x + p_bounds.size.x * 0.5f, p_bounds.position.z + p_bounds.size.z);
	}

	const float capture_epsilon = MAX(outline_thickness * 1.5f, 0.01f);
	float sum_x = 0.0f;
	int count = 0;
	for (int polygon_index = 0; polygon_index < p_polygons.size(); polygon_index++) {
		const Vector<Point2> &polygon = p_polygons[polygon_index];
		for (int i = 0; i < polygon.size(); i++) {
			if (Math::abs(polygon[i].y - max_z) <= capture_epsilon) {
				sum_x += polygon[i].x;
				count++;
			}
		}
	}

	if (count == 0) {
		return Vector2(p_bounds.position.x + p_bounds.size.x * 0.5f, max_z);
	}

	return Vector2(sum_x / count, max_z);
}

bool RuntimePlacementController3D::_project_to_floor(const Vector2 &p_screen_position, Vector3 &r_world_position) const {
	Camera3D *camera = _get_camera();
	if (camera == nullptr) {
		return false;
	}

	Vector3 ray_from = camera->project_ray_origin(p_screen_position);
	Vector3 ray_dir = camera->project_ray_normal(p_screen_position);
	Plane plane(Vector3(0, 1, 0), Vector3(0, floor_height, 0));
	return plane.intersects_ray(ray_from, ray_dir, &r_world_position);
}

bool RuntimePlacementController3D::_pick_target_at_screen_position(const Vector2 &p_screen_position, Node3D *&r_target, Vector3 *r_world_hit) const {
	r_target = nullptr;

	Camera3D *camera = _get_camera();
	Node *targets_root = _get_targets_root();
	if (camera == nullptr || targets_root == nullptr) {
		return false;
	}

	const Vector3 ray_from = camera->project_ray_origin(p_screen_position);
	const Vector3 ray_dir = camera->project_ray_normal(p_screen_position);

	double closest_distance = DBL_MAX;
	float closest_screen_metric = FLT_MAX;
	Node3D *screen_target = nullptr;
	Vector3 screen_target_world_hit;
	const int child_count = targets_root->get_child_count(false);
	for (int i = 0; i < child_count; i++) {
		Node3D *target = Object::cast_to<Node3D>(targets_root->get_child(i, false));
		if (target == nullptr || !target->is_visible_in_tree()) {
			continue;
		}

		AABB local_bounds;
		if (!_compute_local_bounds(target, local_bounds)) {
			continue;
		}

		Rect2 screen_bounds;
		if (_get_target_screen_bounds(target, local_bounds, screen_bounds)) {
			const float screen_pick_padding = MAX(handle_pick_radius * 0.45f, 42.0f);
			const Rect2 expanded_bounds = screen_bounds.grow(screen_pick_padding);
			if (expanded_bounds.has_point(p_screen_position)) {
				const float screen_metric = screen_bounds.get_center().distance_squared_to(p_screen_position);
				if (screen_metric < closest_screen_metric) {
					closest_screen_metric = screen_metric;
					screen_target = target;
					screen_target_world_hit = target->get_global_transform().xform(local_bounds.position + local_bounds.size * 0.5f);
				}
			}
		}

		const Transform3D target_transform = target->get_global_transform();
		const Transform3D inverse = target_transform.affine_inverse();
		const Vector3 local_from = inverse.xform(ray_from);
		const Vector3 local_dir = inverse.basis.xform(ray_dir).normalized();

		bool inside = false;
		Vector3 local_hit;
		if (!local_bounds.find_intersects_ray(local_from, local_dir, inside, &local_hit)) {
			continue;
		}

		const Vector3 world_hit = target_transform.xform(local_hit);
		const double distance = ray_from.distance_to(world_hit);
		if (distance < closest_distance) {
			closest_distance = distance;
			r_target = target;
			if (r_world_hit != nullptr) {
				*r_world_hit = world_hit;
			}
		}
	}

	if (r_target == nullptr && screen_target != nullptr) {
		r_target = screen_target;
		if (r_world_hit != nullptr) {
			*r_world_hit = screen_target_world_hit;
		}
	}

	return r_target != nullptr;
}

bool RuntimePlacementController3D::_get_target_screen_bounds(Node3D *p_target, const AABB &p_local_bounds, Rect2 &r_bounds) const {
	Camera3D *camera = _get_camera();
	if (p_target == nullptr || camera == nullptr) {
		return false;
	}

	const Transform3D target_transform = p_target->get_global_transform();
	bool found = false;
	Vector2 min_point;
	Vector2 max_point;
	for (int i = 0; i < 8; i++) {
		const Vector3 world_point = target_transform.xform(p_local_bounds.get_endpoint(i));
		if (camera->is_position_behind(world_point)) {
			continue;
		}

		const Vector2 screen_point = camera->unproject_position(world_point);
		if (!found) {
			min_point = screen_point;
			max_point = screen_point;
			found = true;
		} else {
			min_point.x = MIN(min_point.x, screen_point.x);
			min_point.y = MIN(min_point.y, screen_point.y);
			max_point.x = MAX(max_point.x, screen_point.x);
			max_point.y = MAX(max_point.y, screen_point.y);
		}
	}

	if (!found) {
		return false;
	}

	r_bounds.position = min_point;
	r_bounds.size = max_point - min_point;
	return true;
}

bool RuntimePlacementController3D::_get_selected_screen_bounds(Rect2 &r_bounds) const {
	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr || !selected_bounds_valid) {
		return false;
	}

	return _get_target_screen_bounds(selected_target, selected_bounds, r_bounds);
}

bool RuntimePlacementController3D::_is_selected_move_zone_hit(const Vector2 &p_screen_position) const {
	if (!translation_enabled) {
		return false;
	}

	if (!selected_bounds_valid) {
		return false;
	}

	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr) {
		return false;
	}

	Rect2 screen_bounds;
	if (!_get_selected_screen_bounds(screen_bounds)) {
		return false;
	}

	const float side_padding = MAX(handle_pick_radius * 0.55f, 22.0f);
	const float top_padding = MAX(handle_pick_radius * 0.20f, 10.0f);
	const float bottom_padding = MAX(handle_pick_radius * 0.65f, 28.0f);
	const Rect2 move_zone(
			Vector2(screen_bounds.position.x - side_padding, screen_bounds.position.y - top_padding),
			Vector2(screen_bounds.size.x + side_padding * 2.0f, screen_bounds.size.y + top_padding + bottom_padding));
	if (!move_zone.has_point(p_screen_position)) {
		return false;
	}

	const float split_y = screen_bounds.position.y + screen_bounds.size.y * 0.52f;
	return p_screen_position.y >= split_y;
}

bool RuntimePlacementController3D::_is_rotate_handle_hit(const Vector2 &p_screen_position) const {
	if (!rotation_enabled) {
		return false;
	}

	if (!selected_bounds_valid) {
		return false;
	}

	Node3D *selected_target = _get_selected_target();
	Camera3D *camera = _get_camera();
	if (selected_target == nullptr || camera == nullptr) {
		return false;
	}

	const Vector3 handle_world = selected_target->get_global_transform().xform(selected_handle_local_position);
	if (camera->is_position_behind(handle_world)) {
		return false;
	}

	if (camera->unproject_position(handle_world).distance_to(p_screen_position) <= handle_pick_radius) {
		return true;
	}

	Rect2 screen_bounds;
	if (!_get_selected_screen_bounds(screen_bounds)) {
		return false;
	}

	// On phones the projected handle can be visually close to the model and hard to hit.
	// Treat the upper portion of the selected body as a rotation zone while keeping the
	// lower portion as the movement zone.
	const float side_padding = MAX(handle_pick_radius * 0.45f, 18.0f);
	const float top_padding = MAX(handle_pick_radius * 0.85f, 28.0f);
	const float bottom_padding = MAX(handle_pick_radius * 0.15f, 10.0f);
	const Rect2 rotate_zone(
			Vector2(screen_bounds.position.x - side_padding, screen_bounds.position.y - top_padding),
			Vector2(screen_bounds.size.x + side_padding * 2.0f, screen_bounds.size.y + top_padding + bottom_padding));
	if (!rotate_zone.has_point(p_screen_position)) {
		return false;
	}

	const float split_y = screen_bounds.position.y + screen_bounds.size.y * 0.58f;
	return p_screen_position.y <= split_y;
}

float RuntimePlacementController3D::_get_yaw_from_screen_position(const Vector2 &p_screen_position) const {
	Node3D *selected_target = _get_selected_target();
	Camera3D *camera = _get_camera();
	if (selected_target == nullptr || camera == nullptr) {
		return 0.0f;
	}

	Vector3 rotation_center_local = Vector3();
	if (selected_bounds_valid) {
		rotation_center_local = selected_bounds.position + selected_bounds.size * 0.5f;
	}

	const Vector3 rotation_center_world = selected_target->get_global_transform().xform(rotation_center_local);
	if (!camera->is_position_behind(rotation_center_world)) {
		const Vector2 screen_center = camera->unproject_position(rotation_center_world);
		const Vector2 delta = p_screen_position - screen_center;
		if (delta.length_squared() >= CMP_EPSILON2) {
			return Math::rad_to_deg(Math::atan2(delta.x, -delta.y));
		}
	}

	Vector3 world_position;
	if (!_project_to_floor(p_screen_position, world_position)) {
		return 0.0f;
	}

	const Vector3 direction = world_position - selected_target->get_global_position();
	if (direction.length_squared() < CMP_EPSILON2) {
		return 0.0f;
	}

	return Math::rad_to_deg(Math::atan2(direction.x, direction.z));
}

float RuntimePlacementController3D::_apply_rotation_snap(float p_yaw_degrees) const {
	if (rotation_snap_degrees <= CMP_EPSILON) {
		return p_yaw_degrees;
	}

	return Math::round(p_yaw_degrees / rotation_snap_degrees) * rotation_snap_degrees;
}

void RuntimePlacementController3D::_queue_drag(const Vector2 &p_screen_position) {
	if (!translation_enabled) {
		return;
	}

	pending_drag = true;
	pending_drag_start_position = p_screen_position;
	dragging = false;
	rotating = false;
}

bool RuntimePlacementController3D::_try_begin_queued_drag(const Vector2 &p_screen_position) {
	if (!pending_drag) {
		return false;
	}

	if (pending_drag_start_position.distance_to(p_screen_position) < DRAG_ACTIVATION_DISTANCE) {
		return false;
	}

	_begin_drag(pending_drag_start_position);
	pending_drag = false;
	return dragging;
}

void RuntimePlacementController3D::_begin_drag(const Vector2 &p_screen_position) {
	if (!translation_enabled) {
		return;
	}

	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr) {
		return;
	}

	Vector3 world_position;
	if (!_project_to_floor(p_screen_position, world_position)) {
		return;
	}

	dragging = true;
	rotating = false;
	pending_drag = false;
	drag_offset = selected_target->get_global_position() - world_position;
}

void RuntimePlacementController3D::_begin_rotation(const Vector2 &p_screen_position) {
	if (!rotation_enabled) {
		return;
	}

	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr) {
		return;
	}

	dragging = false;
	rotating = true;
	pending_drag = false;
	rotate_start_yaw = selected_target->get_rotation_degrees().y;
	rotate_start_angle = _get_yaw_from_screen_position(p_screen_position);
}

void RuntimePlacementController3D::_end_interaction() {
	const bool disarm_move_mode = dragging;
	dragging = false;
	rotating = false;
	pending_drag = false;
	if (disarm_move_mode) {
		_set_move_mode_armed(false);
	}
	active_pointer = -1;
}

void RuntimePlacementController3D::_set_move_mode_armed(bool p_armed) {
	const bool next_armed = translation_enabled ? p_armed : false;
	if (move_mode_armed == next_armed) {
		return;
	}

	move_mode_armed = next_armed;
	emit_signal(SNAME("move_mode_changed"), move_mode_armed);
}

void RuntimePlacementController3D::_clamp_world_position_to_drag_bounds(Vector3 &r_world_position) const {
	if (!drag_bounds_enabled) {
		return;
	}

	const Rect2 normalized_bounds = drag_bounds.abs();
	r_world_position.x = CLAMP(r_world_position.x, normalized_bounds.position.x, normalized_bounds.position.x + normalized_bounds.size.x);
	r_world_position.z = CLAMP(r_world_position.z, normalized_bounds.position.y, normalized_bounds.position.y + normalized_bounds.size.y);
}

void RuntimePlacementController3D::_clear_api_boundary_overlay() {
	if (api_boundary_root != nullptr) {
		if (api_boundary_root->get_parent() != nullptr) {
			api_boundary_root->get_parent()->remove_child(api_boundary_root);
		}
		memdelete(api_boundary_root);
	}

	api_boundary_root = nullptr;
	api_boundary_overlay_mesh = nullptr;
	api_boundary_object_id = ObjectID();
	api_boundary_source_mesh_id = ObjectID();
	api_boundary_mesh_aabb = AABB();
	api_boundary_has_mesh_aabb = false;
}

void RuntimePlacementController3D::_sync_api_boundary_material() {
	if (api_boundary_material.is_null()) {
		api_boundary_material.instantiate();
		api_boundary_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		api_boundary_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
		api_boundary_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
		api_boundary_material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
		api_boundary_material->set_render_priority(Material::RENDER_PRIORITY_MAX);
		api_boundary_material->set_stencil_mode(BaseMaterial3D::STENCIL_MODE_OUTLINE);
	}

	MeshInstance3D *source_mesh = ObjectDB::get_instance<MeshInstance3D>(api_boundary_source_mesh_id);
	api_boundary_material->set_stencil_effect_outline_thickness(_get_mesh_local_outline_thickness(source_mesh));
	api_boundary_material->set_stencil_effect_color(api_boundary_is_valid ? selection_color : INVALID_BOUNDARY_COLOR);
	api_boundary_material->set_albedo(Color(1.0f, 1.0f, 1.0f, 0.0f));

	if (api_boundary_overlay_mesh != nullptr) {
		api_boundary_overlay_mesh->set_material_override(api_boundary_material);
	}
}

void RuntimePlacementController3D::_rebuild_api_boundary_overlay(Node3D *p_target, MeshInstance3D *p_mesh_instance) {
	_clear_api_boundary_overlay();

	ERR_FAIL_NULL(p_target);
	ERR_FAIL_NULL(p_mesh_instance);

	_sync_api_boundary_material();

	api_boundary_root = memnew(Node3D);
	api_boundary_root->set_name(SNAME("RuntimeApiBoundaryOverlay"));
	p_target->add_child(api_boundary_root, false, INTERNAL_MODE_BACK);
	api_boundary_root->set_owner(nullptr);

	api_boundary_overlay_mesh = memnew(MeshInstance3D);
	api_boundary_overlay_mesh->set_name(SNAME("BoundaryMesh"));
	api_boundary_overlay_mesh->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	api_boundary_overlay_mesh->set_mesh(p_mesh_instance->get_mesh());
	api_boundary_overlay_mesh->set_transform(p_mesh_instance->get_transform());
	api_boundary_overlay_mesh->set_skin(p_mesh_instance->get_skin());
	api_boundary_overlay_mesh->set_skeleton_path(p_mesh_instance->get_skeleton_path());
	api_boundary_overlay_mesh->set_material_override(api_boundary_material);
	api_boundary_root->add_child(api_boundary_overlay_mesh, false, INTERNAL_MODE_BACK);
	api_boundary_overlay_mesh->set_owner(nullptr);
}

bool RuntimePlacementController3D::_is_pointer_event_over_gui(const Vector2 &p_screen_position) const {
	Viewport *viewport = get_viewport();
	if (viewport == nullptr) {
		return false;
	}

	Control *control = viewport->gui_find_control(p_screen_position);
	return control != nullptr && control->get_mouse_filter() == Control::MOUSE_FILTER_STOP;
}

void RuntimePlacementController3D::set_camera_path(const NodePath &p_camera_path) {
	camera_path = p_camera_path;
}

NodePath RuntimePlacementController3D::get_camera_path() const {
	return camera_path;
}

void RuntimePlacementController3D::set_targets_root_path(const NodePath &p_targets_root_path) {
	targets_root_path = p_targets_root_path;
}

NodePath RuntimePlacementController3D::get_targets_root_path() const {
	return targets_root_path;
}

void RuntimePlacementController3D::set_floor_height(float p_floor_height) {
	floor_height = p_floor_height;
}

float RuntimePlacementController3D::get_floor_height() const {
	return floor_height;
}

void RuntimePlacementController3D::set_outline_padding(float p_outline_padding) {
	outline_padding = MAX(0.0f, p_outline_padding);
}

float RuntimePlacementController3D::get_outline_padding() const {
	return outline_padding;
}

void RuntimePlacementController3D::set_outline_thickness(float p_outline_thickness) {
	outline_thickness = MAX(0.005f, p_outline_thickness);
	_sync_api_boundary_material();
	if (Node3D *selected_target = _get_selected_target()) {
		_clear_mesh_highlights();
		_apply_mesh_highlights(selected_target);
	}
	_update_overlay();
}

float RuntimePlacementController3D::get_outline_thickness() const {
	return outline_thickness;
}

void RuntimePlacementController3D::set_outline_height(float p_outline_height) {
	outline_height = MAX(0.005f, p_outline_height);
	_update_overlay();
}

float RuntimePlacementController3D::get_outline_height() const {
	return outline_height;
}

void RuntimePlacementController3D::set_handle_distance(float p_handle_distance) {
	handle_distance = MAX(0.05f, p_handle_distance);
	_update_overlay();
}

float RuntimePlacementController3D::get_handle_distance() const {
	return handle_distance;
}

void RuntimePlacementController3D::set_handle_radius(float p_handle_radius) {
	handle_radius = MAX(0.05f, p_handle_radius);
}

float RuntimePlacementController3D::get_handle_radius() const {
	return handle_radius;
}

void RuntimePlacementController3D::set_handle_pick_radius(float p_handle_pick_radius) {
	handle_pick_radius = MAX(4.0f, p_handle_pick_radius);
}

float RuntimePlacementController3D::get_handle_pick_radius() const {
	return handle_pick_radius;
}

void RuntimePlacementController3D::set_rotation_snap_degrees(float p_rotation_snap_degrees) {
	rotation_snap_degrees = MAX(0.0f, p_rotation_snap_degrees);
}

float RuntimePlacementController3D::get_rotation_snap_degrees() const {
	return rotation_snap_degrees;
}

void RuntimePlacementController3D::set_selection_color(const Color &p_selection_color) {
	selection_color = p_selection_color;
	_sync_selection_material();
	_sync_api_boundary_material();
	if (Node3D *selected_target = _get_selected_target()) {
		_clear_mesh_highlights();
		_apply_mesh_highlights(selected_target);
	}
}

Color RuntimePlacementController3D::get_selection_color() const {
	return selection_color;
}

void RuntimePlacementController3D::set_interaction_enabled(bool p_enabled) {
	if (interaction_enabled == p_enabled) {
		return;
	}

	interaction_enabled = p_enabled;
	if (!interaction_enabled) {
		_end_interaction();
		_set_move_mode_armed(false);
	}
}

bool RuntimePlacementController3D::is_interaction_enabled() const {
	return interaction_enabled;
}

void RuntimePlacementController3D::set_selection_enabled(bool p_enabled) {
	selection_enabled = p_enabled;
}

bool RuntimePlacementController3D::is_selection_enabled() const {
	return selection_enabled;
}

void RuntimePlacementController3D::set_translation_enabled(bool p_enabled) {
	if (translation_enabled == p_enabled) {
		return;
	}

	translation_enabled = p_enabled;
	if (!translation_enabled) {
		pending_drag = false;
		if (dragging) {
			_end_interaction();
		}
		_set_move_mode_armed(false);
	}
	_update_overlay();
}

bool RuntimePlacementController3D::is_translation_enabled() const {
	return translation_enabled;
}

void RuntimePlacementController3D::set_rotation_enabled(bool p_enabled) {
	if (rotation_enabled == p_enabled) {
		return;
	}

	rotation_enabled = p_enabled;
	if (!rotation_enabled && rotating) {
		_end_interaction();
	}
}

bool RuntimePlacementController3D::is_rotation_enabled() const {
	return rotation_enabled;
}

void RuntimePlacementController3D::set_clear_selection_on_empty_tap(bool p_enabled) {
	clear_selection_on_empty_tap = p_enabled;
}

bool RuntimePlacementController3D::is_clear_selection_on_empty_tap_enabled() const {
	return clear_selection_on_empty_tap;
}

void RuntimePlacementController3D::set_require_double_tap_to_move(bool p_enabled) {
	require_double_tap_to_move = p_enabled;
}

bool RuntimePlacementController3D::is_double_tap_to_move_required() const {
	return require_double_tap_to_move;
}

void RuntimePlacementController3D::set_move_guides_enabled(bool p_enabled) {
	move_guides_enabled = p_enabled;
	_update_overlay();
}

bool RuntimePlacementController3D::is_move_guides_enabled() const {
	return move_guides_enabled;
}

void RuntimePlacementController3D::set_drag_bounds_enabled(bool p_enabled) {
	drag_bounds_enabled = p_enabled;
}

bool RuntimePlacementController3D::is_drag_bounds_enabled() const {
	return drag_bounds_enabled;
}

void RuntimePlacementController3D::set_drag_bounds(const Rect2 &p_bounds) {
	drag_bounds = p_bounds;
}

Rect2 RuntimePlacementController3D::get_drag_bounds() const {
	return drag_bounds;
}

void RuntimePlacementController3D::select_target(Node3D *p_target) {
	if (p_target == _get_selected_target()) {
		_update_overlay();
		return;
	}

	_clear_mesh_highlights();
	selected_target_id = p_target ? p_target->get_instance_id() : ObjectID();
	selected_bounds_valid = false;
	selected_outline_polygons.clear();
	outline_mesh_dirty = true;
	_set_move_mode_armed(false);
	last_move_visual_active = false;
	_end_interaction();
	if (p_target != nullptr) {
		_apply_mesh_highlights(p_target);
	}
	_update_overlay();
	emit_signal(SNAME("selection_changed"), p_target);
}

Node3D *RuntimePlacementController3D::get_selected_target() const {
	return _get_selected_target();
}

void RuntimePlacementController3D::clear_selection() {
	if (_get_selected_target() == nullptr && !selected_bounds_valid && highlighted_meshes.is_empty()) {
		return;
	}

	_clear_mesh_highlights();
	selected_target_id = ObjectID();
	selected_bounds_valid = false;
	selected_outline_polygons.clear();
	outline_mesh_dirty = true;
	_set_move_mode_armed(false);
	last_move_visual_active = false;
	last_tap_target_id = ObjectID();
	last_tap_time_msec = 0;
	_end_interaction();
	_set_overlay_visible(false);
	emit_signal(SNAME("selection_changed"), Variant());
}

Node3D *RuntimePlacementController3D::pick_target_at_screen_position(const Vector2 &p_screen_position) const {
	Node3D *picked_target = nullptr;
	Vector3 world_hit;
	if (!_pick_target_at_screen_position(p_screen_position, picked_target, &world_hit)) {
		return nullptr;
	}
	return picked_target;
}

bool RuntimePlacementController3D::select_target_at_screen_position(const Vector2 &p_screen_position) {
	if (!selection_enabled) {
		return false;
	}

	Node3D *picked_target = pick_target_at_screen_position(p_screen_position);
	if (picked_target == nullptr) {
		return false;
	}

	select_target(picked_target);
	return true;
}

Variant RuntimePlacementController3D::project_screen_to_floor(const Vector2 &p_screen_position) const {
	Vector3 world_position;
	if (!_project_to_floor(p_screen_position, world_position)) {
		return Variant();
	}

	return world_position;
}

bool RuntimePlacementController3D::move_selected_to_screen_position(const Vector2 &p_screen_position) {
	Vector3 world_position;
	if (!_project_to_floor(p_screen_position, world_position)) {
		return false;
	}

	return move_selected_to_world_position(world_position);
}

bool RuntimePlacementController3D::move_selected_to_world_position(const Vector3 &p_world_position) {
	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr) {
		return false;
	}

	Vector3 next_position = p_world_position;
	_clamp_world_position_to_drag_bounds(next_position);
	selected_target->set_global_position(next_position);
	emit_signal(SNAME("target_transform_changed"), selected_target);
	return true;
}

bool RuntimePlacementController3D::place_target_on_floor(Node3D *p_target, const Vector3 &p_world_position) {
	if (p_target == nullptr) {
		return false;
	}

	Vector3 next_position = p_world_position;
	next_position.y = floor_height;
	_clamp_world_position_to_drag_bounds(next_position);
	p_target->set_global_position(next_position);
	return true;
}

void RuntimePlacementController3D::arm_move_mode() {
	_set_move_mode_armed(true);
}

void RuntimePlacementController3D::disarm_move_mode() {
	_set_move_mode_armed(false);
}

bool RuntimePlacementController3D::is_move_mode_armed() const {
	return move_mode_armed;
}

void RuntimePlacementController3D::rotate_selected(float p_delta_degrees) {
	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr) {
		return;
	}

	Vector3 rotation = selected_target->get_rotation_degrees();
	rotation.y = _apply_rotation_snap(rotation.y + p_delta_degrees);
	selected_target->set_rotation_degrees(rotation);
	emit_signal(SNAME("target_transform_changed"), selected_target);
}

void RuntimePlacementController3D::flip_selected_x() {
	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr) {
		return;
	}

	Vector3 scale = selected_target->get_scale();
	scale.x *= -1.0f;
	selected_target->set_scale(scale);
	emit_signal(SNAME("target_transform_changed"), selected_target);
}

void RuntimePlacementController3D::flip_selected_z() {
	Node3D *selected_target = _get_selected_target();
	if (selected_target == nullptr) {
		return;
	}

	Vector3 scale = selected_target->get_scale();
	scale.z *= -1.0f;
	selected_target->set_scale(scale);
	emit_signal(SNAME("target_transform_changed"), selected_target);
}

bool RuntimePlacementController3D::create_boundary(Node *p_object, const Transform3D &p_global_transform, MeshInstance3D *p_mesh_instance, const AABB &p_mesh_aabb) {
	Node3D *target = Object::cast_to<Node3D>(p_object);
	ERR_FAIL_NULL_V(target, false);
	ERR_FAIL_NULL_V(p_mesh_instance, false);

	target->set_global_transform(p_global_transform);

	api_boundary_object_id = target->get_instance_id();
	api_boundary_source_mesh_id = p_mesh_instance->get_instance_id();
	api_boundary_mesh_aabb = p_mesh_aabb;
	api_boundary_has_mesh_aabb = true;
	api_boundary_is_valid = true;
	_rebuild_api_boundary_overlay(target, p_mesh_instance);
	_sync_api_boundary_material();
	return api_boundary_overlay_mesh != nullptr;
}

bool RuntimePlacementController3D::update_boundary(Node *p_object, const Transform3D &p_transform, MeshInstance3D *p_mesh_instance, bool p_is_valid) {
	Node3D *target = Object::cast_to<Node3D>(p_object);
	ERR_FAIL_NULL_V(target, false);
	ERR_FAIL_NULL_V(p_mesh_instance, false);

	target->set_global_transform(p_transform);
	api_boundary_is_valid = p_is_valid;

	const bool target_changed = api_boundary_object_id != target->get_instance_id();
	const bool mesh_changed = api_boundary_source_mesh_id != p_mesh_instance->get_instance_id();
	if (api_boundary_overlay_mesh == nullptr || target_changed || mesh_changed) {
		api_boundary_object_id = target->get_instance_id();
		api_boundary_source_mesh_id = p_mesh_instance->get_instance_id();
		_rebuild_api_boundary_overlay(target, p_mesh_instance);
	} else {
		api_boundary_overlay_mesh->set_mesh(p_mesh_instance->get_mesh());
		api_boundary_overlay_mesh->set_transform(p_mesh_instance->get_transform());
		api_boundary_overlay_mesh->set_skin(p_mesh_instance->get_skin());
		api_boundary_overlay_mesh->set_skeleton_path(p_mesh_instance->get_skeleton_path());
	}

	_sync_api_boundary_material();
	return api_boundary_overlay_mesh != nullptr;
}

void RuntimePlacementController3D::update_color(bool p_is_valid) {
	api_boundary_is_valid = p_is_valid;
	_sync_api_boundary_material();
}

void RuntimePlacementController3D::delete_boundary() {
	_clear_api_boundary_overlay();
}

bool RuntimePlacementController3D::create_boundry(Node *p_object, const Transform3D &p_global_transform, MeshInstance3D *p_mesh_instance, const AABB &p_mesh_aabb) {
	return create_boundary(p_object, p_global_transform, p_mesh_instance, p_mesh_aabb);
}

bool RuntimePlacementController3D::update_boundry(Node *p_object, const Transform3D &p_transform, MeshInstance3D *p_mesh_instance, bool p_is_valid) {
	return update_boundary(p_object, p_transform, p_mesh_instance, p_is_valid);
}

void RuntimePlacementController3D::delete_boundry() {
	delete_boundary();
}

bool RuntimePlacementController3D::is_dragging_selected() const {
	return dragging;
}

bool RuntimePlacementController3D::is_rotating_selected() const {
	return rotating;
}
