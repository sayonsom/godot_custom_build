package com.test.godotdemo

import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import androidx.fragment.app.FragmentActivity
import org.godotengine.godot.GodotFragment
import org.godotengine.godot.GodotHost

/**
 * Embedded mode: Godot runs inside a Fragment alongside native Android views.
 *
 * This demonstrates the key integration pattern for apps that embed Godot
 * as one component within a larger native Android UI. TalkBack should
 * navigate seamlessly between the native Android views and the Godot
 * fragment's accessible controls.
 */
class EmbeddedActivity : FragmentActivity(), GodotHost {

    private var godotFragment: GodotFragment? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.MATCH_PARENT
            )
        }

        // Native Android button ABOVE the Godot fragment.
        val topButton = Button(this).apply {
            text = "Native Android Button (Above Godot)"
            contentDescription = "Native Android button above the Godot view"
            setOnClickListener {
                (it as Button).text = "Tapped! (Native Android)"
            }
        }
        layout.addView(topButton)

        // Status text.
        val statusText = TextView(this).apply {
            text = "Godot is embedded below. Enable TalkBack and swipe to navigate."
            setPadding(24, 12, 24, 12)
        }
        layout.addView(statusText)

        // FrameLayout for the Godot fragment.
        val fragmentContainer = FrameLayout(this).apply {
            id = android.R.id.content + 100 // Unique ID for fragment container.
            layoutParams = LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                0,
                1f // Take remaining space.
            )
        }
        layout.addView(fragmentContainer)

        // Native Android button BELOW the Godot fragment.
        val bottomButton = Button(this).apply {
            text = "Native Android Button (Below Godot)"
            contentDescription = "Native Android button below the Godot view"
            setOnClickListener {
                // Switch back to standalone mode.
                startActivity(Intent(this@EmbeddedActivity, StandaloneActivity::class.java))
            }
        }
        layout.addView(bottomButton)

        setContentView(layout)

        // Add GodotFragment.
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

    // -- GodotHost interface --

    override fun getActivity() = this

    override fun getGodot() = godotFragment?.godot
}
