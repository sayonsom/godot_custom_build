package com.test.godotdemo

import com.test.godotdemo.api.FloorPlannerPluginHost
import org.godotengine.godot.Godot
import org.godotengine.godot.GodotActivity
import org.godotengine.godot.plugin.GodotPlugin

/**
 * Standalone mode: Godot takes over the entire Activity.
 *
 * This is the simplest integration. GodotActivity handles everything:
 * rendering, input, lifecycle, and accessibility (via AccessKit).
 *
 * The Godot project files (project.godot, main.tscn, etc.) must be
 * placed in the app's assets/ directory.
 */
class StandaloneActivity : GodotActivity() {
    private val floorPlannerHost = FloorPlannerPluginHost()

    override fun getHostPlugins(engine: Godot): Set<GodotPlugin> {
        return floorPlannerHost.createHostPlugins(engine)
    }

    override fun onDestroy() {
        floorPlannerHost.dispose()
        super.onDestroy()
    }
}
