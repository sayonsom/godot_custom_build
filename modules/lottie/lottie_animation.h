/**************************************************************************/
/*  lottie_animation.h                                                    */
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
/* permit persons to whom the Software is furnished to do so, subject to  */
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

#include "core/templates/hash_map.h"
#include "core/templates/list.h"
#include "scene/2d/node_2d.h"
#include "scene/resources/image_texture.h"

#include <thorvg.h>

class ZIPReader;

class LottieAnimation : public Node2D {
	GDCLASS(LottieAnimation, Node2D);

public:
	enum EngineOption {
		ENGINE_OPTION_THORVG_SW = 1,
	};

	enum CullingMode {
		CULLING_DISABLED = 0,
		CULLING_VIEWPORT = 1,
	};

private:
	String animation_path;
	String selected_dotlottie_animation;
	float speed = 1.0f;
	bool looping = true;
	bool autoplay = false;
	bool playing = false;
	bool fit_into_box = true;
	Vector2i fit_box_size = Vector2i(512, 512);
	bool dynamic_resolution = true;
	float resolution_threshold = 0.15f;
	Vector2 offset = Vector2();
	int engine_option = ENGINE_OPTION_THORVG_SW;
	bool frame_cache_enabled = false;
	int frame_cache_budget_mb = 256;
	int frame_cache_step_frames = 1;
	Vector2i max_render_size = Vector2i(4096, 4096);
	int culling_mode = CULLING_DISABLED;
	float culling_margin_px = 0.0f;
	float elapsed_time = 0.0f;
	float last_rendered_frame = -1.0f;
	String resolved_source_path;

	std::unique_ptr<tvg::SwCanvas> canvas;
	std::unique_ptr<tvg::Animation> animation;
	tvg::Picture *picture = nullptr; // Owned by animation, do not delete.

	Ref<ImageTexture> texture;
	PackedByteArray pixel_buffer;
	uint32_t source_width = 0;
	uint32_t source_height = 0;
	uint32_t render_width = 0;
	uint32_t render_height = 0;

	HashMap<int, Ref<Image>> frame_cache;
	List<int> frame_cache_order;

	void _clear_state(bool p_clear_texture = true);
	void _clear_frame_cache();
	void _trim_frame_cache();
	String _resolve_source_path();
	String _resolve_dotlottie_animation_entry(const Ref<ZIPReader> &p_zip_reader) const;
	bool _extract_dotlottie_archive(const Ref<ZIPReader> &p_zip_reader, const String &p_cache_dir) const;
	void _load_animation();
	bool _ensure_render_target(bool p_force = false);
	void _render_frame(float p_frame);
	void _render_frame_internal(float p_frame);
	Vector2 _get_draw_size() const;
	Vector2i _get_requested_render_size() const;
	Vector2i _clamp_render_size(const Vector2i &p_size) const;
	bool _should_resize_render_target(const Vector2i &p_requested_size) const;
	bool _is_culled() const;
	bool _is_frame_cache_candidate(float p_frame) const;
	int _get_frame_cache_key(float p_frame) const;
	Ref<Image> _get_cached_frame(int p_frame_key) const;
	void _store_cached_frame(int p_frame_key, const Ref<Image> &p_image);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_animation_path(const String &p_path);
	String get_animation_path() const;

	void set_selected_dotlottie_animation(const String &p_animation_id);
	String get_selected_dotlottie_animation() const;

	void set_playing(bool p_playing);
	bool is_playing() const;

	void set_autoplay(bool p_autoplay);
	bool is_autoplay() const;
	bool get_autoplay() const;

	void set_looping(bool p_looping);
	bool is_looping() const;
	bool get_looping() const;

	void set_speed(float p_speed);
	float get_speed() const;

	void set_fit_into_box(bool p_fit_into_box);
	bool is_fit_into_box() const;
	bool get_fit_into_box() const;

	void set_fit_box_size(const Vector2i &p_fit_box_size);
	Vector2i get_fit_box_size() const;

	void set_dynamic_resolution(bool p_dynamic_resolution);
	bool is_dynamic_resolution() const;
	bool get_dynamic_resolution() const;

	void set_resolution_threshold(float p_resolution_threshold);
	float get_resolution_threshold() const;

	void set_offset(const Vector2 &p_offset);
	Vector2 get_offset() const;

	void set_engine_option(int p_engine_option);
	int get_engine_option() const;

	void set_frame_cache_enabled(bool p_enabled);
	bool is_frame_cache_enabled() const;

	void set_frame_cache_budget_mb(int p_budget_mb);
	int get_frame_cache_budget_mb() const;

	void set_frame_cache_step(int p_step_frames);
	int get_frame_cache_step() const;

	void set_max_render_size(const Vector2i &p_size);
	Vector2i get_max_render_size() const;

	void set_culling_mode(int p_mode);
	int get_culling_mode() const;

	void set_culling_margin_px(float p_margin);
	float get_culling_margin_px() const;

	void set_render_size(const Vector2i &p_size);
	Vector2i get_render_size() const;

	void set_use_animation_size(bool p_enabled);
	bool is_using_animation_size() const;

	void set_frame(float p_frame);
	float get_frame() const;

	void play();
	void pause();
	void stop();
	void render_static();
	void seek(float p_frame);

	float get_total_frames() const;
	float get_duration() const;
	Vector2i get_animation_size() const;

	// Compatibility aliases for earlier iterations.
	void set_lottie_file_path(const String &p_path);
	String get_lottie_file_path() const;
	void set_loop(bool p_loop);
	bool get_loop() const;
	float get_current_frame() const;

	LottieAnimation();
	~LottieAnimation();
};

VARIANT_ENUM_CAST(LottieAnimation::EngineOption);
VARIANT_ENUM_CAST(LottieAnimation::CullingMode);
