package com.dice3d.demo

import android.app.Activity
import android.graphics.BitmapFactory
import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.widget.FrameLayout
import com.dice3d.DiceView

/**
 * Minimal on-device proof: render a 3D d20 (dice3d / Filament GLES), load the
 * PBR material + face-number atlas so the pips are legible, then roll it to a
 * known result (17) so we can screenshot the tumble start and the settled face.
 */
class MainActivity : Activity() {
    private lateinit var dice: DiceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val root = FrameLayout(this).apply { setBackgroundColor(Color.parseColor("#101014")) }
        dice = DiceView(this)
        root.addView(
            dice,
            FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT,
            ),
        )
        setContentView(root)

        // Give the SurfaceView time to attach (renderer.attachSurface in surfaceChanged),
        // then load assets, add a red d20, and roll it to 17 over 3s.
        Handler(Looper.getMainLooper()).postDelayed({ setupAndRoll() }, 900)
    }

    private fun setupAndRoll() {
        val r = dice.renderer

        // PBR material (compiled .filamat) — required for number rendering.
        assets.open("dice.filamat").use { r.loadMaterial(it.readBytes()) }

        // Face-number atlas → raw RGBA8 pixels.
        val bmp = assets.open("d20.png").use { BitmapFactory.decodeStream(it) }
        val w = bmp.width
        val h = bmp.height
        val px = IntArray(w * h)
        bmp.getPixels(px, 0, w, 0, 0, w, h)
        val rgba = ByteArray(w * h * 4)
        for (i in px.indices) {
            val c = px[i]
            rgba[i * 4] = ((c shr 16) and 0xFF).toByte()       // R
            rgba[i * 4 + 1] = ((c shr 8) and 0xFF).toByte()     // G
            rgba[i * 4 + 2] = (c and 0xFF).toByte()             // B
            rgba[i * 4 + 3] = ((c ushr 24) and 0xFF).toByte()   // A
        }
        r.loadAtlas(rgba, w, h)

        // Red d20, white numbers.
        val handle = r.addDie(20, 0.05f, 0.85f, 0.2f, 0.2f, 1f, true)
        // Land on 17 over 3 seconds.
        r.roll(handle, 17, 3.0f)
    }

    override fun onDestroy() {
        super.onDestroy()
        dice.destroy()
    }
}
