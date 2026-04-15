package com.test.godotdemo.api

import org.json.JSONObject
import java.util.ArrayDeque
import java.util.concurrent.CopyOnWriteArraySet

class FloorPlannerController {

    interface Listener {
        fun onStateChanged(state: JSONObject) {}
        fun onEvent(event: String, payload: JSONObject) {}
    }

    private val listeners = CopyOnWriteArraySet<Listener>()
    private val pendingCommands = ArrayDeque<Pair<String, String>>()

    @Volatile
    private var plugin: FloorPlannerBridgePlugin? = null

    fun addListener(listener: Listener) {
        listeners.add(listener)
    }

    fun removeListener(listener: Listener) {
        listeners.remove(listener)
    }

    internal fun bindPlugin(plugin: FloorPlannerBridgePlugin) {
        this.plugin = plugin
        plugin.setEventReceiver { event, payloadJson ->
            dispatchEvent(event, parsePayload(payloadJson))
        }
        plugin.setReadyListener {
            flushPendingCommands()
            requestState()
        }
    }

    internal fun unbindPlugin(plugin: FloorPlannerBridgePlugin) {
        if (this.plugin !== plugin) {
            return
        }
        plugin.setEventReceiver(null)
        plugin.setReadyListener(null)
        this.plugin = null
    }

    fun requestState() = sendCommand("request_state")

    fun switchFloor(index: Int) = sendCommand(
        "switch_floor",
        JSONObject().put("index", index)
    )

    fun switchFloorById(floorId: String) = sendCommand(
        "switch_floor",
        JSONObject().put("id", floorId)
    )

    fun setPlanEdit(enabled: Boolean) = sendCommand(
        "set_plan_edit",
        JSONObject().put("enabled", enabled)
    )

    fun addRoom() = sendCommand("add_room")

    fun rotateSelected(deltaDegrees: Double) = sendCommand(
        "rotate_selected",
        JSONObject().put("delta_degrees", deltaDegrees)
    )

    fun flipSelectedX() = sendCommand("flip_selected_x")

    fun flipSelectedZ() = sendCommand("flip_selected_z")

    fun clearSelection() = sendCommand("clear_selection")

    fun deleteSelected() = sendCommand("delete_selected")

    fun placeAsset(assetId: String, x: Double, z: Double, yawDegrees: Double? = null, floorIndex: Int? = null): Boolean {
        val payload = JSONObject()
            .put("asset_id", assetId)
            .put("x", x)
            .put("z", z)
        if (yawDegrees != null) {
            payload.put("rotation", yawDegrees)
        }
        if (floorIndex != null) {
            payload.put("floor_index", floorIndex)
        }
        return sendCommand("place_asset", payload)
    }

    fun setOutlineThickness(value: Double) = sendCommand(
        "set_outline_thickness",
        JSONObject().put("value", value)
    )

    private fun sendCommand(command: String, payload: JSONObject = JSONObject()): Boolean {
        val payloadJson = payload.toString()
        val activePlugin = plugin
        if (activePlugin == null) {
            pendingCommands.add(command to payloadJson)
            return false
        }
        if (!activePlugin.sendCommand(command, payloadJson)) {
            pendingCommands.add(command to payloadJson)
            return false
        }
        return true
    }

    private fun flushPendingCommands() {
        val activePlugin = plugin ?: return
        while (pendingCommands.isNotEmpty()) {
            val (command, payloadJson) = pendingCommands.removeFirst()
            if (!activePlugin.sendCommand(command, payloadJson)) {
                pendingCommands.addFirst(command to payloadJson)
                return
            }
        }
    }

    private fun dispatchEvent(event: String, payload: JSONObject) {
        if (event == "state") {
            listeners.forEach { it.onStateChanged(payload) }
        }
        listeners.forEach { it.onEvent(event, payload) }
    }

    private fun parsePayload(payloadJson: String): JSONObject {
        if (payloadJson.isBlank()) {
            return JSONObject()
        }
        return try {
            JSONObject(payloadJson)
        } catch (_: Exception) {
            JSONObject()
        }
    }
}
