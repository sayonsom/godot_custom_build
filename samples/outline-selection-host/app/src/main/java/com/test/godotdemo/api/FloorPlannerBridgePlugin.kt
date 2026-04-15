package com.test.godotdemo.api

import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import org.godotengine.godot.plugin.SignalInfo
import org.godotengine.godot.plugin.UsedByGodot

class FloorPlannerBridgePlugin(godot: Godot) : GodotPlugin(godot) {

    companion object {
        const val PLUGIN_NAME: String = "FloorPlannerBridge"
        private val COMMAND_SIGNAL = SignalInfo(
            "command_requested",
            String::class.java,
            String::class.java
        )
        private val SIGNALS = setOf(COMMAND_SIGNAL)
    }

    private var eventReceiver: ((event: String, payloadJson: String) -> Unit)? = null
    private var readyListener: (() -> Unit)? = null
    @Volatile
    private var commandChannelReady: Boolean = false

    override fun getPluginName(): String = PLUGIN_NAME

    override fun getPluginSignals(): Set<SignalInfo> = SIGNALS

    override fun onGodotMainLoopStarted() {
        commandChannelReady = true
        runOnHostThread {
            readyListener?.invoke()
        }
    }

    fun setEventReceiver(receiver: ((event: String, payloadJson: String) -> Unit)?) {
        eventReceiver = receiver
    }

    fun setReadyListener(listener: (() -> Unit)?) {
        readyListener = listener
    }

    fun sendCommand(command: String, payloadJson: String = "{}"): Boolean {
        if (!commandChannelReady) {
            return false
        }
        emitSignal(COMMAND_SIGNAL.name, command, payloadJson)
        return true
    }

    @UsedByGodot
    fun onGodotEvent(event: String, payloadJson: String) {
        runOnHostThread {
            eventReceiver?.invoke(event, payloadJson)
        }
    }
}
