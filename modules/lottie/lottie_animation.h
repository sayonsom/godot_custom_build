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

#include "scene/2d/node_2d.h"

#include "scene/resources/image_texture.h"

#include <thorvg.h>

class ThorVGLottie : public Node2D {
	GDCLASS(ThorVGLottie, Node2D);

	String lottie_file_path;
	float speed = 1.0f;
	bool loop = true;
	bool autoplay = true;
	bool playing = false;
	float elapsed_time = 0.0f;
	float last_rendered_frame = -1.0f;

	// ThorVG objects.
	std::unique_ptr<tvg::SwCanvas> canvas;
	std::unique_ptr<tvg::Animation> animation;
	tvg::Picture *picture = nullptr; // Owned by animation, do not delete.

	// Rendering.
	Ref<ImageTexture> texture;
	PackedByteArray pixel_buffer;
	uint32_t render_width = 0;
	uint32_t render_height = 0;

	void _load_animation();
	void _render_frame(float p_frame);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	void set_lottie_file_path(const String &p_path);
	String get_lottie_file_path() const;

	void set_speed(float p_speed);
	float get_speed() const;

	void set_loop(bool p_loop);
	bool get_loop() const;

	void set_autoplay(bool p_autoplay);
	bool get_autoplay() const;

	void play();
	void pause();
	void stop();
	void seek(int p_frame);
	bool is_playing() const;

	int get_current_frame() const;
	int get_total_frames() const;
	float get_duration() const;
	Vector2 get_animation_size() const;

	ThorVGLottie();
	~ThorVGLottie();
};
