/**************************************************************************/
/*  RuntimePlacementController.kt                                         */
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

package org.godotengine.godot.runtime3dtools

import org.json.JSONObject
import java.util.ArrayDeque
import java.util.concurrent.CopyOnWriteArraySet

/**
 * Native-side controller for runtime placement commands.
 *
 * This class is intentionally scene-agnostic. It sends commands and receives events,
 * while the Godot project implements the command handling.
 */
class RuntimePlacementController {

	interface Listener {
		fun onStateChanged(state: JSONObject) {}
		fun onEvent(event: String, payload: JSONObject) {}
	}

	companion object {
		const val COMMAND_REQUEST_STATE = "request_state"
		const val COMMAND_CLEAR_SELECTION = "clear_selection"
		const val COMMAND_SELECT_AT_SCREEN = "select_at_screen"
		const val COMMAND_MOVE_SELECTED_TO_SCREEN = "move_selected_to_screen"
		const val COMMAND_MOVE_SELECTED_TO_WORLD = "move_selected_to_world"
		const val COMMAND_ROTATE_SELECTED = "rotate_selected"
		const val COMMAND_FLIP_SELECTED_X = "flip_selected_x"
		const val COMMAND_FLIP_SELECTED_Z = "flip_selected_z"
		const val COMMAND_ARM_MOVE_MODE = "arm_move_mode"
		const val COMMAND_DISARM_MOVE_MODE = "disarm_move_mode"
		const val COMMAND_PLACE_TARGET_ON_FLOOR = "place_target_on_floor"
		const val COMMAND_SET_OUTLINE_THICKNESS = "set_outline_thickness"
		const val COMMAND_SET_REQUIRE_DOUBLE_TAP_TO_MOVE = "set_require_double_tap_to_move"
		const val COMMAND_SET_CLEAR_SELECTION_ON_EMPTY_TAP = "set_clear_selection_on_empty_tap"
		const val COMMAND_SET_INTERACTION_ENABLED = "set_interaction_enabled"
		const val COMMAND_SET_SELECTION_ENABLED = "set_selection_enabled"
		const val COMMAND_SET_TRANSLATION_ENABLED = "set_translation_enabled"
		const val COMMAND_SET_ROTATION_ENABLED = "set_rotation_enabled"
		const val COMMAND_SET_MOVE_GUIDES_ENABLED = "set_move_guides_enabled"
		const val COMMAND_SET_DRAG_BOUNDS_ENABLED = "set_drag_bounds_enabled"
		const val COMMAND_SET_DRAG_BOUNDS = "set_drag_bounds"
	}

	private data class PendingCommand(
		val command: String,
		val payloadJson: String
	)

	private val listeners = CopyOnWriteArraySet<Listener>()
	private val pendingCommands = ArrayDeque<PendingCommand>()

	@Volatile
	private var plugin: RuntimePlacementBridgePlugin? = null

	fun addListener(listener: Listener) {
		listeners.add(listener)
	}

	fun removeListener(listener: Listener) {
		listeners.remove(listener)
	}

	internal fun bindPlugin(plugin: RuntimePlacementBridgePlugin) {
		this.plugin = plugin
		plugin.setEventReceiver { event, payloadJson ->
			dispatchEvent(event, parsePayload(payloadJson))
		}
		plugin.setReadyListener {
			flushPendingCommands()
			requestState()
		}
		if (plugin.isCommandChannelReady()) {
			flushPendingCommands()
			requestState()
		}
	}

	internal fun unbindPlugin(plugin: RuntimePlacementBridgePlugin) {
		if (this.plugin !== plugin) {
			return
		}
		plugin.setEventReceiver(null)
		plugin.setReadyListener(null)
		this.plugin = null
	}

	fun requestState() = sendRawCommand(COMMAND_REQUEST_STATE)

	fun clearSelection() = sendRawCommand(COMMAND_CLEAR_SELECTION)

	fun selectAtScreen(screenX: Double, screenY: Double) = sendRawCommand(
		COMMAND_SELECT_AT_SCREEN,
		JSONObject()
			.put("x", screenX)
			.put("y", screenY)
	)

	fun moveSelectedToScreen(screenX: Double, screenY: Double) = sendRawCommand(
		COMMAND_MOVE_SELECTED_TO_SCREEN,
		JSONObject()
			.put("x", screenX)
			.put("y", screenY)
	)

	fun moveSelectedToWorld(worldX: Double, worldY: Double, worldZ: Double) = sendRawCommand(
		COMMAND_MOVE_SELECTED_TO_WORLD,
		JSONObject()
			.put("x", worldX)
			.put("y", worldY)
			.put("z", worldZ)
	)

	fun rotateSelected(deltaDegrees: Double) = sendRawCommand(
		COMMAND_ROTATE_SELECTED,
		JSONObject().put("delta_degrees", deltaDegrees)
	)

	fun flipSelectedX() = sendRawCommand(COMMAND_FLIP_SELECTED_X)

	fun flipSelectedZ() = sendRawCommand(COMMAND_FLIP_SELECTED_Z)

	fun armMoveMode() = sendRawCommand(COMMAND_ARM_MOVE_MODE)

	fun disarmMoveMode() = sendRawCommand(COMMAND_DISARM_MOVE_MODE)

	fun placeTargetOnFloor(targetPath: String, worldX: Double, worldY: Double, worldZ: Double) = sendRawCommand(
		COMMAND_PLACE_TARGET_ON_FLOOR,
		JSONObject()
			.put("target_path", targetPath)
			.put("x", worldX)
			.put("y", worldY)
			.put("z", worldZ)
	)

	fun setOutlineThickness(value: Double) = sendRawCommand(
		COMMAND_SET_OUTLINE_THICKNESS,
		JSONObject().put("value", value)
	)

	fun setRequireDoubleTapToMove(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_REQUIRE_DOUBLE_TAP_TO_MOVE,
		JSONObject().put("enabled", enabled)
	)

	fun setClearSelectionOnEmptyTap(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_CLEAR_SELECTION_ON_EMPTY_TAP,
		JSONObject().put("enabled", enabled)
	)

	fun setInteractionEnabled(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_INTERACTION_ENABLED,
		JSONObject().put("enabled", enabled)
	)

	fun setSelectionEnabled(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_SELECTION_ENABLED,
		JSONObject().put("enabled", enabled)
	)

	fun setTranslationEnabled(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_TRANSLATION_ENABLED,
		JSONObject().put("enabled", enabled)
	)

	fun setRotationEnabled(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_ROTATION_ENABLED,
		JSONObject().put("enabled", enabled)
	)

	fun setMoveGuidesEnabled(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_MOVE_GUIDES_ENABLED,
		JSONObject().put("enabled", enabled)
	)

	fun setDragBoundsEnabled(enabled: Boolean) = sendRawCommand(
		COMMAND_SET_DRAG_BOUNDS_ENABLED,
		JSONObject().put("enabled", enabled)
	)

	fun setDragBounds(minX: Double, minZ: Double, width: Double, depth: Double) = sendRawCommand(
		COMMAND_SET_DRAG_BOUNDS,
		JSONObject()
			.put("x", minX)
			.put("z", minZ)
			.put("width", width)
			.put("depth", depth)
	)

	@JvmOverloads
	fun sendRawCommand(command: String, payload: JSONObject = JSONObject()): Boolean {
		val payloadJson = payload.toString()
		val activePlugin = plugin
		if (activePlugin == null) {
			pendingCommands.add(PendingCommand(command, payloadJson))
			return false
		}
		if (!activePlugin.sendCommand(command, payloadJson)) {
			pendingCommands.add(PendingCommand(command, payloadJson))
			return false
		}
		return true
	}

	private fun flushPendingCommands() {
		val activePlugin = plugin ?: return
		while (pendingCommands.isNotEmpty()) {
			val (command, payloadJson) = pendingCommands.removeFirst()
			if (!activePlugin.sendCommand(command, payloadJson)) {
				pendingCommands.addFirst(PendingCommand(command, payloadJson))
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
