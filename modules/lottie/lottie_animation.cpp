/**************************************************************************/
/*  lottie_animation.cpp                                                  */
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

#include "lottie_animation.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/math/math_funcs.h"
#include "modules/zip/zip_reader.h"

void LottieAnimation::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_animation_path", "value"), &LottieAnimation::set_animation_path);
	ClassDB::bind_method(D_METHOD("get_animation_path"), &LottieAnimation::get_animation_path);

	ClassDB::bind_method(D_METHOD("set_selected_dotlottie_animation", "value"), &LottieAnimation::set_selected_dotlottie_animation);
	ClassDB::bind_method(D_METHOD("get_selected_dotlottie_animation"), &LottieAnimation::get_selected_dotlottie_animation);

	ClassDB::bind_method(D_METHOD("set_playing", "value"), &LottieAnimation::set_playing);
	ClassDB::bind_method(D_METHOD("is_playing"), &LottieAnimation::is_playing);

	ClassDB::bind_method(D_METHOD("set_autoplay", "value"), &LottieAnimation::set_autoplay);
	ClassDB::bind_method(D_METHOD("is_autoplay"), &LottieAnimation::is_autoplay);
	ClassDB::bind_method(D_METHOD("get_autoplay"), &LottieAnimation::get_autoplay);

	ClassDB::bind_method(D_METHOD("set_looping", "value"), &LottieAnimation::set_looping);
	ClassDB::bind_method(D_METHOD("is_looping"), &LottieAnimation::is_looping);
	ClassDB::bind_method(D_METHOD("get_looping"), &LottieAnimation::get_looping);

	ClassDB::bind_method(D_METHOD("set_speed", "value"), &LottieAnimation::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &LottieAnimation::get_speed);

	ClassDB::bind_method(D_METHOD("set_fit_into_box", "value"), &LottieAnimation::set_fit_into_box);
	ClassDB::bind_method(D_METHOD("is_fit_into_box"), &LottieAnimation::is_fit_into_box);
	ClassDB::bind_method(D_METHOD("get_fit_into_box"), &LottieAnimation::get_fit_into_box);

	ClassDB::bind_method(D_METHOD("set_fit_box_size", "value"), &LottieAnimation::set_fit_box_size);
	ClassDB::bind_method(D_METHOD("get_fit_box_size"), &LottieAnimation::get_fit_box_size);

	ClassDB::bind_method(D_METHOD("set_dynamic_resolution", "value"), &LottieAnimation::set_dynamic_resolution);
	ClassDB::bind_method(D_METHOD("is_dynamic_resolution"), &LottieAnimation::is_dynamic_resolution);
	ClassDB::bind_method(D_METHOD("get_dynamic_resolution"), &LottieAnimation::get_dynamic_resolution);

	ClassDB::bind_method(D_METHOD("set_engine_option", "value"), &LottieAnimation::set_engine_option);
	ClassDB::bind_method(D_METHOD("get_engine_option"), &LottieAnimation::get_engine_option);

	ClassDB::bind_method(D_METHOD("set_frame_cache_budget_mb", "value"), &LottieAnimation::set_frame_cache_budget_mb);
	ClassDB::bind_method(D_METHOD("get_frame_cache_budget_mb"), &LottieAnimation::get_frame_cache_budget_mb);
	ClassDB::bind_method(D_METHOD("set_frame_cache_enabled", "value"), &LottieAnimation::set_frame_cache_enabled);
	ClassDB::bind_method(D_METHOD("is_frame_cache_enabled"), &LottieAnimation::is_frame_cache_enabled);
	ClassDB::bind_method(D_METHOD("set_frame_cache_step", "value"), &LottieAnimation::set_frame_cache_step);
	ClassDB::bind_method(D_METHOD("get_frame_cache_step"), &LottieAnimation::get_frame_cache_step);

	ClassDB::bind_method(D_METHOD("set_max_render_size", "value"), &LottieAnimation::set_max_render_size);
	ClassDB::bind_method(D_METHOD("get_max_render_size"), &LottieAnimation::get_max_render_size);

	ClassDB::bind_method(D_METHOD("set_offset", "value"), &LottieAnimation::set_offset);
	ClassDB::bind_method(D_METHOD("get_offset"), &LottieAnimation::get_offset);

	ClassDB::bind_method(D_METHOD("set_resolution_threshold", "value"), &LottieAnimation::set_resolution_threshold);
	ClassDB::bind_method(D_METHOD("get_resolution_threshold"), &LottieAnimation::get_resolution_threshold);

	ClassDB::bind_method(D_METHOD("get_culling_margin_px"), &LottieAnimation::get_culling_margin_px);
	ClassDB::bind_method(D_METHOD("get_culling_mode"), &LottieAnimation::get_culling_mode);
	ClassDB::bind_method(D_METHOD("get_duration"), &LottieAnimation::get_duration);
	ClassDB::bind_method(D_METHOD("get_frame"), &LottieAnimation::get_frame);
	ClassDB::bind_method(D_METHOD("get_render_size"), &LottieAnimation::get_render_size);
	ClassDB::bind_method(D_METHOD("get_total_frames"), &LottieAnimation::get_total_frames);
	ClassDB::bind_method(D_METHOD("is_using_animation_size"), &LottieAnimation::is_using_animation_size);

	ClassDB::bind_method(D_METHOD("pause"), &LottieAnimation::pause);
	ClassDB::bind_method(D_METHOD("play"), &LottieAnimation::play);
	ClassDB::bind_method(D_METHOD("render_static"), &LottieAnimation::render_static);
	ClassDB::bind_method(D_METHOD("seek", "frame"), &LottieAnimation::seek);
	ClassDB::bind_method(D_METHOD("set_culling_margin_px", "margin"), &LottieAnimation::set_culling_margin_px);
	ClassDB::bind_method(D_METHOD("set_culling_mode", "mode"), &LottieAnimation::set_culling_mode);
	ClassDB::bind_method(D_METHOD("set_frame", "frame"), &LottieAnimation::set_frame);
	ClassDB::bind_method(D_METHOD("set_render_size", "size"), &LottieAnimation::set_render_size);
	ClassDB::bind_method(D_METHOD("set_use_animation_size", "enabled"), &LottieAnimation::set_use_animation_size);
	ClassDB::bind_method(D_METHOD("stop"), &LottieAnimation::stop);

	// Compatibility aliases.
	ClassDB::bind_method(D_METHOD("set_lottie_file_path", "value"), &LottieAnimation::set_lottie_file_path);
	ClassDB::bind_method(D_METHOD("get_lottie_file_path"), &LottieAnimation::get_lottie_file_path);
	ClassDB::bind_method(D_METHOD("set_loop", "value"), &LottieAnimation::set_loop);
	ClassDB::bind_method(D_METHOD("get_loop"), &LottieAnimation::get_loop);
	ClassDB::bind_method(D_METHOD("get_current_frame"), &LottieAnimation::get_current_frame);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "animation_path", PROPERTY_HINT_FILE, "*.json,*.lottie"), "set_animation_path", "get_animation_path");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "is_autoplay");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dotlottie/selected_animation"), "set_selected_dotlottie_animation", "get_selected_dotlottie_animation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "dynamic_resolution"), "set_dynamic_resolution", "is_dynamic_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "engine_option", PROPERTY_HINT_ENUM, "ThorVG Software:1"), "set_engine_option", "get_engine_option");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "fit_box_size"), "set_fit_box_size", "get_fit_box_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "fit_into_box"), "set_fit_into_box", "is_fit_into_box");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "frame_cache/budget_mb", PROPERTY_HINT_RANGE, "1,2048,1,suffix:MB"), "set_frame_cache_budget_mb", "get_frame_cache_budget_mb");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "frame_cache/enabled"), "set_frame_cache_enabled", "is_frame_cache_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "frame_cache/step_frames", PROPERTY_HINT_RANGE, "1,60,1"), "set_frame_cache_step", "get_frame_cache_step");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "looping"), "set_looping", "is_looping");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "max_render_size"), "set_max_render_size", "get_max_render_size");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "resolution_threshold", PROPERTY_HINT_RANGE, "0.01,1.0,0.01"), "set_resolution_threshold", "get_resolution_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), "set_speed", "get_speed");

	// Hidden compatibility serialization aliases for older scenes.
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lottie_file_path", PROPERTY_HINT_FILE, "*.json,*.lottie", PROPERTY_USAGE_NO_EDITOR), "set_lottie_file_path", "get_lottie_file_path");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_loop", "get_loop");

	ADD_SIGNAL(MethodInfo("animation_finished"));
	ADD_SIGNAL(MethodInfo("animation_loaded", PropertyInfo(Variant::BOOL, "success")));
	ADD_SIGNAL(MethodInfo("frame_changed", PropertyInfo(Variant::FLOAT, "frame")));

	BIND_ENUM_CONSTANT(ENGINE_OPTION_THORVG_SW);
	BIND_ENUM_CONSTANT(CULLING_DISABLED);
	BIND_ENUM_CONSTANT(CULLING_VIEWPORT);
}

void LottieAnimation::_clear_frame_cache() {
	frame_cache.clear();
	frame_cache_order.clear();
}

void LottieAnimation::_clear_state(bool p_clear_texture) {
	canvas.reset();
	animation.reset();
	picture = nullptr;
	pixel_buffer.clear();
	render_width = 0;
	render_height = 0;
	source_width = 0;
	source_height = 0;
	resolved_source_path = String();
	elapsed_time = 0.0f;
	last_rendered_frame = -1.0f;
	_clear_frame_cache();
	if (p_clear_texture) {
		texture.unref();
	}
}

void LottieAnimation::_trim_frame_cache() {
	if (!frame_cache_enabled || render_width == 0 || render_height == 0) {
		_clear_frame_cache();
		return;
	}

	const uint64_t frame_bytes = (uint64_t)render_width * (uint64_t)render_height * 4ULL;
	if (frame_bytes == 0) {
		_clear_frame_cache();
		return;
	}

	const uint64_t budget_bytes = (uint64_t)MAX(frame_cache_budget_mb, 1) * 1024ULL * 1024ULL;
	const int max_frames = (int)(budget_bytes / frame_bytes);
	if (max_frames < 1) {
		_clear_frame_cache();
		return;
	}

	while ((int)frame_cache.size() > max_frames && !frame_cache_order.is_empty()) {
		List<int>::Element *front = frame_cache_order.front();
		frame_cache.erase(front->get());
		frame_cache_order.erase(front);
	}
}

String LottieAnimation::_resolve_dotlottie_animation_entry(const Ref<ZIPReader> &p_zip_reader) const {
	PackedStringArray archive_files = p_zip_reader->get_files();
	String fallback_json;
	for (int i = 0; i < archive_files.size(); i++) {
		String normalized = archive_files[i].replace("\\", "/");
		if (normalized.contains("..") || normalized.begins_with("/")) {
			continue;
		}
		if (normalized.get_extension().to_lower() == "json") {
			if (fallback_json.is_empty()) {
				fallback_json = normalized;
			}
			if (normalized.begins_with("animations/")) {
				break;
			}
		}
	}

	if (!p_zip_reader->file_exists("manifest.json", false)) {
		return fallback_json;
	}

	PackedByteArray manifest_bytes = p_zip_reader->read_file("manifest.json", false);
	if (manifest_bytes.is_empty()) {
		return fallback_json;
	}

	String manifest_text = String::utf8(reinterpret_cast<const char *>(manifest_bytes.ptr()), manifest_bytes.size());
	Variant manifest_variant = JSON::parse_string(manifest_text);
	if (manifest_variant.get_type() != Variant::DICTIONARY) {
		return fallback_json;
	}

	Dictionary manifest = manifest_variant;
	Array animations = manifest.get("animations", Array());
	String preferred_id = selected_dotlottie_animation;
	if (preferred_id.is_empty()) {
		if (manifest.has("initial") && manifest["initial"].get_type() == Variant::STRING) {
			preferred_id = manifest["initial"];
		} else if (manifest.has("activeAnimationId") && manifest["activeAnimationId"].get_type() == Variant::STRING) {
			preferred_id = manifest["activeAnimationId"];
		}
	}

	String manifest_first_path;
	for (int i = 0; i < animations.size(); i++) {
		if (animations[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary animation_dict = animations[i];
		String animation_id = animation_dict.get("id", "");
		String animation_path_candidate = animation_dict.get("path", "");
		if (animation_path_candidate.is_empty()) {
			animation_path_candidate = animation_dict.get("url", "");
		}
		animation_path_candidate = animation_path_candidate.replace("\\", "/");
		if (manifest_first_path.is_empty() && !animation_path_candidate.is_empty()) {
			manifest_first_path = animation_path_candidate;
		}
		if (!preferred_id.is_empty() && (animation_id == preferred_id || animation_path_candidate == preferred_id)) {
			return animation_path_candidate;
		}
	}

	if (!manifest_first_path.is_empty()) {
		return manifest_first_path;
	}

	return fallback_json;
}

bool LottieAnimation::_extract_dotlottie_archive(const Ref<ZIPReader> &p_zip_reader, const String &p_cache_dir) const {
	PackedStringArray archive_files = p_zip_reader->get_files();
	Error err = DirAccess::make_dir_recursive_absolute(p_cache_dir);
	ERR_FAIL_COND_V(err != OK, false);

	for (int i = 0; i < archive_files.size(); i++) {
		String relative_path = archive_files[i].replace("\\", "/");
		if (relative_path.is_empty() || relative_path.ends_with("/") || relative_path.contains("..") || relative_path.begins_with("/")) {
			continue;
		}

		String output_path = p_cache_dir.path_join(relative_path);
		err = DirAccess::make_dir_recursive_absolute(output_path.get_base_dir());
		ERR_FAIL_COND_V(err != OK, false);

		PackedByteArray file_bytes = p_zip_reader->read_file(relative_path, false);
		Ref<FileAccess> output_file = FileAccess::open(output_path, FileAccess::WRITE);
		ERR_FAIL_COND_V(output_file.is_null(), false);
		output_file->store_buffer(file_bytes);
	}

	return true;
}

String LottieAnimation::_resolve_source_path() {
	if (animation_path.is_empty()) {
		return String();
	}

	if (animation_path.get_extension().to_lower() != "lottie") {
		return animation_path;
	}

	Ref<ZIPReader> zip_reader;
	zip_reader.instantiate();
	if (zip_reader->open(animation_path) != OK) {
		return String();
	}

	String entry_path = _resolve_dotlottie_animation_entry(zip_reader);
	if (entry_path.is_empty()) {
		zip_reader->close();
		return String();
	}

	String cache_root = "user://dotlottie_cache/" + itos((int)animation_path.hash());
	if (!_extract_dotlottie_archive(zip_reader, cache_root)) {
		zip_reader->close();
		return String();
	}

	zip_reader->close();
	return cache_root.path_join(entry_path);
}

bool LottieAnimation::_ensure_render_target(bool p_force) {
	if (!animation || picture == nullptr) {
		return false;
	}

	const Vector2i requested_size = _get_requested_render_size();
	if (requested_size.x <= 0 || requested_size.y <= 0) {
		return false;
	}

	const bool needs_resize = p_force || texture.is_null() || _should_resize_render_target(requested_size);
	if (!needs_resize) {
		return true;
	}

	render_width = requested_size.x;
	render_height = requested_size.y;
	pixel_buffer.resize(render_width * render_height * 4);
	memset(pixel_buffer.ptrw(), 0, pixel_buffer.size());
	_clear_frame_cache();

	canvas.reset();
	canvas = tvg::SwCanvas::gen();
	if (!canvas) {
		ERR_PRINT("LottieAnimation: Failed to create ThorVG SwCanvas.");
		return false;
	}

	const tvg::Result size_result = picture->size((float)render_width, (float)render_height);
	if (size_result != tvg::Result::Success) {
		ERR_PRINT("LottieAnimation: Failed to resize ThorVG picture.");
		return false;
	}

	const tvg::Result target_result = canvas->target(reinterpret_cast<uint32_t *>(pixel_buffer.ptrw()), render_width, render_width, render_height, tvg::SwCanvas::ABGR8888S);
	if (target_result != tvg::Result::Success) {
		ERR_PRINT("LottieAnimation: Failed to assign ThorVG target buffer.");
		return false;
	}

	canvas->push(tvg::cast<tvg::Picture>(picture));

	Ref<Image> image = Image::create_from_data(render_width, render_height, false, Image::FORMAT_RGBA8, pixel_buffer);
	texture = ImageTexture::create_from_image(image);
	return true;
}

Vector2i LottieAnimation::_clamp_render_size(const Vector2i &p_size) const {
	Vector2i clamped = p_size;
	clamped.x = CLAMP(clamped.x, 1, MAX(max_render_size.x, 1));
	clamped.y = CLAMP(clamped.y, 1, MAX(max_render_size.y, 1));
	return clamped;
}

Vector2 LottieAnimation::_get_draw_size() const {
	if (fit_into_box) {
		return Vector2((float)MAX(fit_box_size.x, 1), (float)MAX(fit_box_size.y, 1));
	}

	if (source_width == 0 || source_height == 0) {
		return Vector2();
	}

	return Vector2((float)source_width, (float)source_height);
}

Vector2i LottieAnimation::_get_requested_render_size() const {
	Vector2 requested = _get_draw_size();
	if (requested.x <= 0.0f || requested.y <= 0.0f) {
		return Vector2i();
	}

	if (dynamic_resolution) {
		const Vector2 global_scale = get_global_transform().get_scale();
		const float max_scale = MAX(Math::abs(global_scale.x), Math::abs(global_scale.y));
		requested *= MAX(max_scale, 1.0f);
	}

	return _clamp_render_size(Vector2i(
			MAX((int)Math::ceil(requested.x), 1),
			MAX((int)Math::ceil(requested.y), 1)));
}

bool LottieAnimation::_should_resize_render_target(const Vector2i &p_requested_size) const {
	if (render_width == 0 || render_height == 0) {
		return true;
	}
	if (p_requested_size.x == (int)render_width && p_requested_size.y == (int)render_height) {
		return false;
	}

	const float width_delta = Math::abs((float)p_requested_size.x - (float)render_width) / MAX((float)render_width, 1.0f);
	const float height_delta = Math::abs((float)p_requested_size.y - (float)render_height) / MAX((float)render_height, 1.0f);
	return MAX(width_delta, height_delta) >= resolution_threshold;
}

bool LottieAnimation::_is_culled() const {
	if (culling_mode == CULLING_DISABLED || !is_inside_tree() || get_viewport() == nullptr) {
		return false;
	}

	Rect2 local_rect(offset, _get_draw_size());
	Rect2 global_rect = get_global_transform().xform(local_rect);
	Rect2 visible_rect = get_viewport_rect().grow(culling_margin_px);
	return !visible_rect.intersects(global_rect);
}

bool LottieAnimation::_is_frame_cache_candidate(float p_frame) const {
	if (!frame_cache_enabled) {
		return false;
	}

	const int rounded_frame = (int)Math::round(p_frame);
	return frame_cache_step_frames > 0 && Math::abs(p_frame - (float)rounded_frame) < 0.001f && (rounded_frame % frame_cache_step_frames) == 0;
}

int LottieAnimation::_get_frame_cache_key(float p_frame) const {
	return (int)Math::round(p_frame);
}

Ref<Image> LottieAnimation::_get_cached_frame(int p_frame_key) const {
	const Ref<Image> *cached = frame_cache.getptr(p_frame_key);
	if (cached != nullptr) {
		return *cached;
	}
	return Ref<Image>();
}

void LottieAnimation::_store_cached_frame(int p_frame_key, const Ref<Image> &p_image) {
	if (!frame_cache_enabled || p_image.is_null() || frame_cache.has(p_frame_key)) {
		return;
	}

	const uint64_t frame_bytes = (uint64_t)render_width * (uint64_t)render_height * 4ULL;
	if (frame_bytes == 0) {
		return;
	}

	const uint64_t budget_bytes = (uint64_t)MAX(frame_cache_budget_mb, 1) * 1024ULL * 1024ULL;
	const int max_frames = (int)(budget_bytes / frame_bytes);
	if (max_frames < 1) {
		return;
	}

	while ((int)frame_cache.size() >= max_frames && !frame_cache_order.is_empty()) {
		List<int>::Element *front = frame_cache_order.front();
		frame_cache.erase(front->get());
		frame_cache_order.erase(front);
	}

	frame_cache.insert(p_frame_key, p_image);
	frame_cache_order.push_back(p_frame_key);
}

void LottieAnimation::_render_frame_internal(float p_frame) {
	if (!animation || !canvas || pixel_buffer.is_empty()) {
		return;
	}

	const int frame_key = _get_frame_cache_key(p_frame);
	if (_is_frame_cache_candidate(p_frame)) {
		Ref<Image> cached_image = _get_cached_frame(frame_key);
		if (cached_image.is_valid()) {
			if (texture.is_valid()) {
				texture->update(cached_image);
			} else {
				texture = ImageTexture::create_from_image(cached_image);
			}
			return;
		}
	}

	animation->frame(p_frame);
	memset(pixel_buffer.ptrw(), 0, pixel_buffer.size());

	canvas->update();
	canvas->draw();
	canvas->sync();

	Ref<Image> image = Image::create_from_data(render_width, render_height, false, Image::FORMAT_RGBA8, pixel_buffer);
	if (texture.is_valid()) {
		texture->update(image);
	} else {
		texture = ImageTexture::create_from_image(image);
	}

	if (_is_frame_cache_candidate(p_frame)) {
		_store_cached_frame(frame_key, image);
	}
}

void LottieAnimation::_render_frame(float p_frame) {
	if (!_ensure_render_target()) {
		return;
	}

	_render_frame_internal(p_frame);
	last_rendered_frame = p_frame;
	emit_signal(SNAME("frame_changed"), p_frame);
	queue_redraw();
}

void LottieAnimation::_load_animation() {
	_clear_state();

	if (animation_path.is_empty()) {
		set_process(false);
		return;
	}

	resolved_source_path = _resolve_source_path();
	if (resolved_source_path.is_empty()) {
		emit_signal(SNAME("animation_loaded"), false);
		return;
	}

	animation = tvg::Animation::gen();
	if (!animation) {
		ERR_PRINT("LottieAnimation: Failed to create ThorVG Animation.");
		emit_signal(SNAME("animation_loaded"), false);
		return;
	}

	picture = animation->picture();
	if (!picture) {
		ERR_PRINT("LottieAnimation: Failed to get picture from Animation.");
		animation.reset();
		emit_signal(SNAME("animation_loaded"), false);
		return;
	}

	tvg::Result load_result = tvg::Result::InvalidArguments;

	// Prefer loading through FileAccess so res:// works reliably in exported/embedded Android builds.
	Ref<FileAccess> source_file = FileAccess::open(resolved_source_path, FileAccess::READ);
	if (source_file.is_valid()) {
		const uint64_t source_size = source_file->get_length();
		if (source_size > 0 && source_size <= UINT32_MAX) {
			PackedByteArray source_bytes;
			source_bytes.resize(source_size);
			source_file->get_buffer(source_bytes.ptrw(), source_size);

			String mime_hint = resolved_source_path.get_extension().to_lower();
			if (mime_hint == "json" || mime_hint == "lottie") {
				mime_hint = "lottie";
			}
			const std::string mime_hint_std = mime_hint.utf8().get_data();
			load_result = picture->load(reinterpret_cast<const char *>(source_bytes.ptr()), (uint32_t)source_size, mime_hint_std, true);
			if (load_result != tvg::Result::Success) {
				load_result = picture->load(reinterpret_cast<const char *>(source_bytes.ptr()), (uint32_t)source_size, "", true);
			}
		}
	}

	// Fallback for direct filesystem paths.
	if (load_result != tvg::Result::Success) {
		const String global_path = ProjectSettings::get_singleton()->globalize_path(resolved_source_path);
		load_result = picture->load(global_path.utf8().get_data());
	}

	if (load_result != tvg::Result::Success) {
		ERR_PRINT(vformat("LottieAnimation: Failed to load animation '%s' (error %d).", resolved_source_path, (int)load_result));
		animation.reset();
		picture = nullptr;
		emit_signal(SNAME("animation_loaded"), false);
		return;
	}

	float width = 0.0f;
	float height = 0.0f;
	picture->size(&width, &height);
	if (width <= 0.0f || height <= 0.0f) {
		width = 512.0f;
		height = 512.0f;
	}

	source_width = MAX((uint32_t)Math::ceil(width), (uint32_t)1);
	source_height = MAX((uint32_t)Math::ceil(height), (uint32_t)1);

	if (!_ensure_render_target(true)) {
		texture.unref();
		emit_signal(SNAME("animation_loaded"), false);
		return;
	}

	_render_frame(0.0f);
	emit_signal(SNAME("animation_loaded"), true);
}

void LottieAnimation::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (!animation_path.is_empty()) {
				_load_animation();
			}

			if (autoplay || playing) {
				play();
			} else {
				pause();
				render_static();
			}
		} break;

		case NOTIFICATION_PROCESS: {
			if (!playing || !animation) {
				return;
			}

			const float total = animation->totalFrame();
			const float duration = animation->duration();
			if (total <= 0.0f || duration <= 0.0f) {
				return;
			}

			elapsed_time += get_process_delta_time() * speed;
			const float fps = total / duration;
			float target_frame = elapsed_time * fps;

			if (target_frame >= total) {
				if (looping) {
					elapsed_time = Math::fmod(elapsed_time, duration);
					target_frame = elapsed_time * fps;
				} else {
					target_frame = MAX(total - 1.0f, 0.0f);
					playing = false;
					set_process(false);
					emit_signal(SNAME("animation_finished"));
				}
			}

			if (!_is_culled() && Math::abs(target_frame - last_rendered_frame) >= 0.5f) {
				_render_frame(target_frame);
			} else {
				last_rendered_frame = target_frame;
			}
		} break;

		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED:
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (!animation) {
				return;
			}
			if (_ensure_render_target()) {
				if (!_is_culled()) {
					_render_frame_internal(last_rendered_frame >= 0.0f ? last_rendered_frame : 0.0f);
				}
				queue_redraw();
			}
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree() && animation && !_is_culled()) {
				render_static();
			}
		} break;

		case NOTIFICATION_DRAW: {
			if (texture.is_valid() && !_is_culled()) {
				// Snap draw rect to device pixels to avoid subtle drift in embedded hosts.
				const Vector2 draw_pos(Math::round(offset.x), Math::round(offset.y));
				const Vector2 draw_size = _get_draw_size().round();
				draw_texture_rect(texture, Rect2(draw_pos, draw_size), false);
			}
		} break;
	}
}

void LottieAnimation::set_animation_path(const String &p_path) {
	if (animation_path == p_path) {
		return;
	}

	animation_path = p_path;
	if (is_inside_tree()) {
		const bool should_play = autoplay || playing;
		_load_animation();
		if (should_play) {
			play();
		}
	}
}

String LottieAnimation::get_animation_path() const {
	return animation_path;
}

void LottieAnimation::set_selected_dotlottie_animation(const String &p_animation_id) {
	if (selected_dotlottie_animation == p_animation_id) {
		return;
	}

	selected_dotlottie_animation = p_animation_id;
	if (is_inside_tree() && animation_path.get_extension().to_lower() == "lottie") {
		const bool should_play = autoplay || playing;
		_load_animation();
		if (should_play) {
			play();
		}
	}
}

String LottieAnimation::get_selected_dotlottie_animation() const {
	return selected_dotlottie_animation;
}

void LottieAnimation::set_playing(bool p_playing) {
	if (p_playing) {
		play();
	} else {
		pause();
	}
}

bool LottieAnimation::is_playing() const {
	return playing;
}

void LottieAnimation::set_autoplay(bool p_autoplay) {
	autoplay = p_autoplay;
}

bool LottieAnimation::is_autoplay() const {
	return autoplay;
}

bool LottieAnimation::get_autoplay() const {
	return autoplay;
}

void LottieAnimation::set_looping(bool p_looping) {
	looping = p_looping;
}

bool LottieAnimation::is_looping() const {
	return looping;
}

bool LottieAnimation::get_looping() const {
	return looping;
}

void LottieAnimation::set_speed(float p_speed) {
	speed = MAX(p_speed, 0.01f);
}

float LottieAnimation::get_speed() const {
	return speed;
}

void LottieAnimation::set_fit_into_box(bool p_fit_into_box) {
	if (fit_into_box == p_fit_into_box) {
		return;
	}

	fit_into_box = p_fit_into_box;
	if (_ensure_render_target(true)) {
		render_static();
	}
}

bool LottieAnimation::is_fit_into_box() const {
	return fit_into_box;
}

bool LottieAnimation::get_fit_into_box() const {
	return fit_into_box;
}

void LottieAnimation::set_fit_box_size(const Vector2i &p_fit_box_size) {
	Vector2i clamped = Vector2i(MAX(p_fit_box_size.x, 1), MAX(p_fit_box_size.y, 1));
	if (fit_box_size == clamped) {
		return;
	}

	fit_box_size = clamped;
	if (_ensure_render_target(true)) {
		render_static();
	}
}

Vector2i LottieAnimation::get_fit_box_size() const {
	return fit_box_size;
}

void LottieAnimation::set_dynamic_resolution(bool p_dynamic_resolution) {
	if (dynamic_resolution == p_dynamic_resolution) {
		return;
	}

	dynamic_resolution = p_dynamic_resolution;
	if (_ensure_render_target(true)) {
		render_static();
	}
}

bool LottieAnimation::is_dynamic_resolution() const {
	return dynamic_resolution;
}

bool LottieAnimation::get_dynamic_resolution() const {
	return dynamic_resolution;
}

void LottieAnimation::set_resolution_threshold(float p_resolution_threshold) {
	const float clamped = CLAMP(p_resolution_threshold, 0.01f, 1.0f);
	if (Math::is_equal_approx(resolution_threshold, clamped)) {
		return;
	}

	resolution_threshold = clamped;
	if (_ensure_render_target(true)) {
		render_static();
	}
}

float LottieAnimation::get_resolution_threshold() const {
	return resolution_threshold;
}

void LottieAnimation::set_offset(const Vector2 &p_offset) {
	if (offset.is_equal_approx(p_offset)) {
		return;
	}

	offset = p_offset;
	queue_redraw();
}

Vector2 LottieAnimation::get_offset() const {
	return offset;
}

void LottieAnimation::set_engine_option(int p_engine_option) {
	engine_option = p_engine_option == ENGINE_OPTION_THORVG_SW ? ENGINE_OPTION_THORVG_SW : ENGINE_OPTION_THORVG_SW;
}

int LottieAnimation::get_engine_option() const {
	return engine_option;
}

void LottieAnimation::set_frame_cache_enabled(bool p_enabled) {
	if (frame_cache_enabled == p_enabled) {
		return;
	}

	frame_cache_enabled = p_enabled;
	if (!frame_cache_enabled) {
		_clear_frame_cache();
	}
}

bool LottieAnimation::is_frame_cache_enabled() const {
	return frame_cache_enabled;
}

void LottieAnimation::set_frame_cache_budget_mb(int p_budget_mb) {
	frame_cache_budget_mb = MAX(p_budget_mb, 1);
	_trim_frame_cache();
}

int LottieAnimation::get_frame_cache_budget_mb() const {
	return frame_cache_budget_mb;
}

void LottieAnimation::set_frame_cache_step(int p_step_frames) {
	frame_cache_step_frames = MAX(p_step_frames, 1);
	_clear_frame_cache();
}

int LottieAnimation::get_frame_cache_step() const {
	return frame_cache_step_frames;
}

void LottieAnimation::set_max_render_size(const Vector2i &p_size) {
	Vector2i clamped(MAX(p_size.x, 1), MAX(p_size.y, 1));
	if (max_render_size == clamped) {
		return;
	}

	max_render_size = clamped;
	if (_ensure_render_target(true)) {
		render_static();
	}
}

Vector2i LottieAnimation::get_max_render_size() const {
	return max_render_size;
}

void LottieAnimation::set_culling_mode(int p_mode) {
	culling_mode = p_mode == CULLING_VIEWPORT ? CULLING_VIEWPORT : CULLING_DISABLED;
	queue_redraw();
}

int LottieAnimation::get_culling_mode() const {
	return culling_mode;
}

void LottieAnimation::set_culling_margin_px(float p_margin) {
	culling_margin_px = MAX(p_margin, 0.0f);
	queue_redraw();
}

float LottieAnimation::get_culling_margin_px() const {
	return culling_margin_px;
}

void LottieAnimation::set_render_size(const Vector2i &p_size) {
	fit_into_box = true;
	set_fit_box_size(p_size);
}

Vector2i LottieAnimation::get_render_size() const {
	if (render_width > 0 && render_height > 0) {
		return Vector2i((int)render_width, (int)render_height);
	}
	return _get_requested_render_size();
}

void LottieAnimation::set_use_animation_size(bool p_enabled) {
	set_fit_into_box(!p_enabled);
}

bool LottieAnimation::is_using_animation_size() const {
	return !fit_into_box;
}

void LottieAnimation::set_frame(float p_frame) {
	seek(p_frame);
}

float LottieAnimation::get_frame() const {
	if (animation) {
		return animation->curFrame();
	}
	return MAX(last_rendered_frame, 0.0f);
}

void LottieAnimation::play() {
	if (!animation && !animation_path.is_empty()) {
		_load_animation();
	}

	playing = animation != nullptr;
	set_process(playing);
}

void LottieAnimation::pause() {
	playing = false;
	set_process(false);
}

void LottieAnimation::stop() {
	playing = false;
	set_process(false);
	elapsed_time = 0.0f;
	if (animation) {
		_render_frame(0.0f);
	}
}

void LottieAnimation::render_static() {
	if (!animation) {
		return;
	}

	float target_frame = animation->curFrame();
	if (last_rendered_frame >= 0.0f) {
		target_frame = last_rendered_frame;
	}
	_render_frame(target_frame);
}

void LottieAnimation::seek(float p_frame) {
	if (!animation) {
		return;
	}

	const float total = animation->totalFrame();
	const float duration = animation->duration();
	if (total <= 0.0f || duration <= 0.0f) {
		return;
	}

	const float clamped = CLAMP(p_frame, 0.0f, MAX(total - 1.0f, 0.0f));
	const float fps = total / duration;
	elapsed_time = clamped / fps;
	_render_frame(clamped);
}

float LottieAnimation::get_total_frames() const {
	if (!animation) {
		return 0.0f;
	}
	return animation->totalFrame();
}

float LottieAnimation::get_duration() const {
	if (!animation) {
		return 0.0f;
	}
	return animation->duration();
}

Vector2i LottieAnimation::get_animation_size() const {
	return Vector2i((int)source_width, (int)source_height);
}

void LottieAnimation::set_lottie_file_path(const String &p_path) {
	set_animation_path(p_path);
}

String LottieAnimation::get_lottie_file_path() const {
	return get_animation_path();
}

void LottieAnimation::set_loop(bool p_loop) {
	set_looping(p_loop);
}

bool LottieAnimation::get_loop() const {
	return get_looping();
}

float LottieAnimation::get_current_frame() const {
	return get_frame();
}

LottieAnimation::LottieAnimation() {
	set_notify_transform(true);
	set_notify_local_transform(true);
	set_process(false);
}

LottieAnimation::~LottieAnimation() {
	_clear_state(false);
}
