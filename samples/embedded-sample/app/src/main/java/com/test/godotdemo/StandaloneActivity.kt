package com.test.godotdemo

import org.godotengine.godot.GodotActivity

/**
 * Standalone mode: Godot takes over the entire Activity.
 *
 * This is the simplest integration. GodotActivity handles everything:
 * rendering, input, lifecycle, and accessibility (via AccessKit).
 *
 * The Godot project files (project.godot, main.tscn, etc.) must be
 * placed in the app's assets/ directory.
 */
class StandaloneActivity : GodotActivity()
