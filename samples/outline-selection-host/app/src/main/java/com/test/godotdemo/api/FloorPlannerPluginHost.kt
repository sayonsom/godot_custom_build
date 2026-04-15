package com.test.godotdemo.api

import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin

class FloorPlannerPluginHost {
    val controller = FloorPlannerController()
    private var plugin: FloorPlannerBridgePlugin? = null

    fun createHostPlugins(engine: Godot): Set<GodotPlugin> {
        plugin?.let { controller.unbindPlugin(it) }
        val nextPlugin = FloorPlannerBridgePlugin(engine)
        plugin = nextPlugin
        controller.bindPlugin(nextPlugin)
        return setOf(nextPlugin)
    }

    fun dispose() {
        plugin?.let { controller.unbindPlugin(it) }
        plugin = null
    }
}
