package com.test.godotdemo

import android.os.Bundle
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Button
import android.widget.HorizontalScrollView
import android.graphics.Color
import android.util.TypedValue
import android.view.Gravity
import androidx.fragment.app.FragmentActivity
import com.test.godotdemo.api.FloorPlannerPluginHost
import org.godotengine.godot.Godot
import org.godotengine.godot.GodotFragment
import org.godotengine.godot.GodotHost
import org.godotengine.godot.plugin.GodotPlugin
import java.io.InputStream
import java.nio.charset.StandardCharsets

class EmbeddedActivity : FragmentActivity(), GodotHost {

    private var godotFragment: GodotFragment? = null
    private val floorPlannerHost = FloorPlannerPluginHost()
    private var planEditEnabled = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.parseColor("#F4F5F8"))
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
            )
        }

        val topBar = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.WHITE)
            setPadding(dp(20), dp(18), dp(20), dp(14))
            elevation = dp(4).toFloat()
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )
        }

        val title = TextView(this).apply {
            text = "Embedded Floor Planner"
            setTextColor(Color.parseColor("#202738"))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 22f)
            setTypeface(typeface, android.graphics.Typeface.BOLD)
        }
        topBar.addView(title)

        val subtitle = TextView(this).apply {
            text = "Native Android host with the Godot floor planner embedded below."
            setTextColor(Color.parseColor("#5F6B84"))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f)
            setPadding(0, dp(4), 0, 0)
        }
        topBar.addView(subtitle)

        val helper = TextView(this).apply {
            text = "Host API demo: switch floors, toggle plan edit, rotate and clear from native Android buttons."
            setTextColor(Color.parseColor("#4362C1"))
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
            gravity = Gravity.START
            setPadding(0, dp(10), 0, 0)
        }
        topBar.addView(helper)

        val actionsRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.START
            setPadding(0, dp(10), 0, 0)
        }

        actionsRow.addView(apiButton("1F") {
            floorPlannerHost.controller.switchFloor(0)
        })
        actionsRow.addView(apiButton("2F") {
            floorPlannerHost.controller.switchFloor(1)
        })
        actionsRow.addView(apiButton("3F") {
            floorPlannerHost.controller.switchFloor(2)
        })
        actionsRow.addView(apiButton("Rotate +") {
            floorPlannerHost.controller.rotateSelected(15.0)
        })
        actionsRow.addView(apiButton("Clear") {
            floorPlannerHost.controller.clearSelection()
        })
        actionsRow.addView(apiButton("Plan Edit OFF") { button ->
            planEditEnabled = !planEditEnabled
            button.text = if (planEditEnabled) "Plan Edit ON" else "Plan Edit OFF"
            floorPlannerHost.controller.setPlanEdit(planEditEnabled)
        })

        val actionsScroller = HorizontalScrollView(this).apply {
            isHorizontalScrollBarEnabled = false
            addView(actionsRow)
        }
        topBar.addView(actionsScroller)

        layout.addView(topBar)

        val fragmentContainer = FrameLayout(this).apply {
            id = android.R.id.content + 100
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1f
            )
        }
        layout.addView(fragmentContainer)

        setContentView(layout)

        if (savedInstanceState == null) {
            godotFragment = GodotFragment()
            supportFragmentManager.beginTransaction()
                .replace(fragmentContainer.id, godotFragment!!)
                .commit()
        } else {
            godotFragment = supportFragmentManager
                .findFragmentById(fragmentContainer.id) as? GodotFragment
        }
    }

    override fun onDestroy() {
        floorPlannerHost.dispose()
        super.onDestroy()
    }

    override fun getActivity() = this

    override fun getGodot() = godotFragment?.godot

    override fun getHostPlugins(engine: Godot): Set<GodotPlugin> {
        return floorPlannerHost.createHostPlugins(engine)
    }

    override fun getCommandLine(): MutableList<String> {
        return try {
            assets.open("_cl_").use { stream ->
                val args = parseCommandLine(stream)
                args.removeAll(listOf("--fullscreen"))
                args
            }
        } catch (_: Exception) {
            mutableListOf()
        }
    }

    private fun parseCommandLine(stream: InputStream): MutableList<String> {
        val argc = readIntLE(stream)
        if (argc <= 0) {
            return mutableListOf()
        }

        val args = ArrayList<String>(argc)
        repeat(argc) {
            val length = readIntLE(stream)
            if (length <= 0 || length > 65535) {
                return args
            }

            val bytes = stream.readNBytes(length)
            if (bytes.size == length) {
                args += String(bytes, StandardCharsets.UTF_8)
            }
        }
        return args
    }

    private fun readIntLE(stream: InputStream): Int {
        val bytes = stream.readNBytes(4)
        if (bytes.size < 4) {
            return -1
        }

        return ((bytes[3].toInt() and 0xFF) shl 24) or
            ((bytes[2].toInt() and 0xFF) shl 16) or
            ((bytes[1].toInt() and 0xFF) shl 8) or
            (bytes[0].toInt() and 0xFF)
    }

    private fun dp(value: Int): Int {
        return TypedValue.applyDimension(
            TypedValue.COMPLEX_UNIT_DIP,
            value.toFloat(),
            resources.displayMetrics
        ).toInt()
    }

    private fun apiButton(label: String, onClick: (Button) -> Unit): Button {
        val button = Button(this).apply {
            text = label
            isAllCaps = false
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f)
            setPadding(dp(10), dp(4), dp(10), dp(4))
            setOnClickListener { onClick(this) }
        }
        button.layoutParams = LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        ).apply {
            marginEnd = dp(6)
        }
        return button
    }
}
