/**************************************************************************/
/*  RuntimePlacementBridgePlugin.kt                                       */
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

import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import org.godotengine.godot.plugin.SignalInfo
import org.godotengine.godot.plugin.UsedByGodot

/**
 * Generic host bridge plugin for runtime 3D placement controllers.
 *
 * Native side emits [command_requested] to the Godot scene.
 * Godot side sends back events by calling [onGodotEvent].
 */
class RuntimePlacementBridgePlugin(godot: Godot) : GodotPlugin(godot) {

	companion object {
		const val PLUGIN_NAME: String = "RuntimePlacementBridge"

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
	private var commandChannelReady = false

	override fun getPluginName(): String = PLUGIN_NAME

	override fun getPluginSignals(): Set<SignalInfo> = SIGNALS

	override fun onGodotMainLoopStarted() {
		commandChannelReady = true
		runOnHostThread {
			readyListener?.invoke()
		}
	}

	override fun onMainDestroy() {
		commandChannelReady = false
	}

	fun isCommandChannelReady(): Boolean = commandChannelReady

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
