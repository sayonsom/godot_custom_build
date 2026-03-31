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

#include "core/io/file_access.h"

void ThorVGLottie::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_lottie_file_path", "path"), &ThorVGLottie::set_lottie_file_path);
	ClassDB::bind_method(D_METHOD("get_lottie_file_path"), &ThorVGLottie::get_lottie_file_path);

	ClassDB::bind_method(D_METHOD("set_speed", "speed"), &ThorVGLottie::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &ThorVGLottie::get_speed);

	ClassDB::bind_method(D_METHOD("set_loop", "loop"), &ThorVGLottie::set_loop);
	ClassDB::bind_method(D_METHOD("get_loop"), &ThorVGLottie::get_loop);

	ClassDB::bind_method(D_METHOD("set_autoplay", "autoplay"), &ThorVGLottie::set_autoplay);
	ClassDB::bind_method(D_METHOD("get_autoplay"), &ThorVGLottie::get_autoplay);

	ClassDB::bind_method(D_METHOD("play"), &ThorVGLottie::play);
	ClassDB::bind_method(D_METHOD("pause"), &ThorVGLottie::pause);
	ClassDB::bind_method(D_METHOD("stop"), &ThorVGLottie::stop);
	ClassDB::bind_method(D_METHOD("seek", "frame"), &ThorVGLottie::seek);
	ClassDB::bind_method(D_METHOD("is_playing"), &ThorVGLottie::is_playing);

	ClassDB::bind_method(D_METHOD("get_current_frame"), &ThorVGLottie::get_current_frame);
	ClassDB::bind_method(D_METHOD("get_total_frames"), &ThorVGLottie::get_total_frames);
	ClassDB::bind_method(D_METHOD("get_duration"), &ThorVGLottie::get_duration);
	ClassDB::bind_method(D_METHOD("get_animation_size"), &ThorVGLottie::get_animation_size);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "lottie_file_path", PROPERTY_HINT_FILE, "*.json,*.lottie"), "set_lottie_file_path", "get_lottie_file_path");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop"), "set_loop", "get_loop");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

	ADD_SIGNAL(MethodInfo("animation_finished"));
	ADD_SIGNAL(MethodInfo("animation_looped"));
}

void ThorVGLottie::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (!lottie_file_path.is_empty()) {
				_load_animation();
			}
			if (autoplay && animation) {
				play();
			}
			set_process(true);
		} break;

		case NOTIFICATION_PROCESS: {
			if (!playing || !animation) {
				return;
			}

			float total = animation->totalFrame();
			float dur = animation->duration();
			if (total <= 0.0f || dur <= 0.0f) {
				return;
			}

			elapsed_time += get_process_delta_time() * speed;

			float fps = total / dur;
			float target_frame = elapsed_time * fps;

			if (target_frame >= total) {
				if (loop) {
					elapsed_time = Math::fmod(elapsed_time, dur);
					target_frame = elapsed_time * fps;
					emit_signal(SNAME("animation_looped"));
				} else {
					target_frame = total - 1.0f;
					playing = false;
					emit_signal(SNAME("animation_finished"));
				}
			}

			if (Math::abs(target_frame - last_rendered_frame) >= 0.5f) {
				_render_frame(target_frame);
				last_rendered_frame = target_frame;
				queue_redraw();
			}
		} break;

		case NOTIFICATION_DRAW: {
			if (texture.is_valid()) {
				draw_texture(texture, Point2());
			}
		} break;
	}
}

void ThorVGLottie::_load_animation() {
	if (canvas) {
		canvas->clear(false);
	}
	canvas.reset();
	animation.reset();
	picture = nullptr;
	texture.unref();
	pixel_buffer.clear();
	render_width = 0;
	render_height = 0;
	elapsed_time = 0.0f;
	last_rendered_frame = -1.0f;

	if (lottie_file_path.is_empty()) {
		return;
	}

	// Read the file data.
	Ref<FileAccess> fa = FileAccess::open(lottie_file_path, FileAccess::READ);
	if (fa.is_null()) {
		ERR_PRINT(vformat("ThorVGLottie: Cannot open file '%s'.", lottie_file_path));
		return;
	}

	uint64_t len = fa->get_length();
	PackedByteArray data;
	data.resize(len);
	fa->get_buffer(data.ptrw(), len);

	// Create ThorVG animation.
	animation = tvg::Animation::gen();
	if (!animation) {
		ERR_PRINT("ThorVGLottie: Failed to create ThorVG Animation.");
		return;
	}

	picture = animation->picture();
	if (!picture) {
		ERR_PRINT("ThorVGLottie: Failed to get picture from Animation.");
		animation.reset();
		return;
	}

	// Load the Lottie data.
	tvg::Result result = picture->load((const char *)data.ptr(), len, "lottie", true);
	if (result != tvg::Result::Success) {
		ERR_PRINT(vformat("ThorVGLottie: Failed to load Lottie data from '%s' (error %d).", lottie_file_path, (int)result));
		animation.reset();
		picture = nullptr;
		return;
	}

	// Get the native size.
	float pw = 0, ph = 0;
	picture->size(&pw, &ph);
	if (pw <= 0 || ph <= 0) {
		pw = 512;
		ph = 512;
	}

	render_width = (uint32_t)pw;
	render_height = (uint32_t)ph;

	// Allocate pixel buffer (ARGB8888).
	pixel_buffer.resize(render_width * render_height * 4);
	memset(pixel_buffer.ptrw(), 0, pixel_buffer.size());

	// Create software canvas.
	canvas = tvg::SwCanvas::gen();
	if (!canvas) {
		ERR_PRINT("ThorVGLottie: Failed to create ThorVG SwCanvas.");
		animation.reset();
		picture = nullptr;
		return;
	}

	canvas->target((uint32_t *)pixel_buffer.ptrw(), render_width, render_width, render_height, tvg::SwCanvas::ABGR8888S);

	// Push the animation's picture to the canvas.
	// tvg::cast wraps the raw pointer in unique_ptr for the push() API.
	// The Animation retains internal ownership; Canvas borrows the picture
	// for rendering. On clear(false) the picture is kept alive.
	canvas->push(tvg::cast<tvg::Picture>(picture));

	// Render the first frame.
	_render_frame(0);

	// Create texture.
	Ref<Image> img = Image::create_from_data(render_width, render_height, false, Image::FORMAT_RGBA8, pixel_buffer);
	texture = ImageTexture::create_from_image(img);
}

void ThorVGLottie::_render_frame(float p_frame) {
	if (!animation || !canvas || pixel_buffer.is_empty()) {
		return;
	}

	animation->frame(p_frame);

	// Clear the pixel buffer.
	memset(pixel_buffer.ptrw(), 0, pixel_buffer.size());

	canvas->update();
	canvas->draw();
	canvas->sync();

	// Update the texture.
	if (texture.is_valid()) {
		Ref<Image> img = Image::create_from_data(render_width, render_height, false, Image::FORMAT_RGBA8, pixel_buffer);
		texture->update(img);
	}
}

void ThorVGLottie::set_lottie_file_path(const String &p_path) {
	if (lottie_file_path == p_path) {
		return;
	}
	lottie_file_path = p_path;
	if (is_inside_tree()) {
		_load_animation();
		if (autoplay) {
			play();
		}
		queue_redraw();
	}
}

String ThorVGLottie::get_lottie_file_path() const {
	return lottie_file_path;
}

void ThorVGLottie::set_speed(float p_speed) {
	speed = p_speed;
}

float ThorVGLottie::get_speed() const {
	return speed;
}

void ThorVGLottie::set_loop(bool p_loop) {
	loop = p_loop;
}

bool ThorVGLottie::get_loop() const {
	return loop;
}

void ThorVGLottie::set_autoplay(bool p_autoplay) {
	autoplay = p_autoplay;
}

bool ThorVGLottie::get_autoplay() const {
	return autoplay;
}

void ThorVGLottie::play() {
	if (!animation) {
		return;
	}
	playing = true;
}

void ThorVGLottie::pause() {
	playing = false;
}

void ThorVGLottie::stop() {
	playing = false;
	elapsed_time = 0.0f;
	if (animation) {
		_render_frame(0);
		last_rendered_frame = 0;
		queue_redraw();
	}
}

void ThorVGLottie::seek(int p_frame) {
	if (!animation) {
		return;
	}
	float total = animation->totalFrame();
	float dur = animation->duration();
	if (total <= 0.0f || dur <= 0.0f) {
		return;
	}
	float clamped = CLAMP((float)p_frame, 0.0f, total - 1.0f);
	float fps = total / dur;
	elapsed_time = clamped / fps;
	_render_frame(clamped);
	last_rendered_frame = clamped;
	queue_redraw();
}

bool ThorVGLottie::is_playing() const {
	return playing;
}

int ThorVGLottie::get_current_frame() const {
	if (!animation) {
		return 0;
	}
	return (int)animation->curFrame();
}

int ThorVGLottie::get_total_frames() const {
	if (!animation) {
		return 0;
	}
	return (int)animation->totalFrame();
}

float ThorVGLottie::get_duration() const {
	if (!animation) {
		return 0.0f;
	}
	return animation->duration();
}

Vector2 ThorVGLottie::get_animation_size() const {
	return Vector2((float)render_width, (float)render_height);
}

ThorVGLottie::ThorVGLottie() {
}

ThorVGLottie::~ThorVGLottie() {
	// Clear canvas without freeing the picture (owned by animation).
	if (canvas) {
		canvas->clear(false);
	}
	canvas.reset();
	// Animation owns the picture, releases it on destruction.
	animation.reset();
	picture = nullptr;
}
