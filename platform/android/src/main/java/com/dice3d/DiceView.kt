package com.dice3d

import android.content.Context
import android.util.AttributeSet
import android.view.Choreographer
import android.view.SurfaceHolder
import android.view.SurfaceView

class DiceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : SurfaceView(context, attrs), SurfaceHolder.Callback, Choreographer.FrameCallback {

    val renderer = DiceRenderer()
    private var lastNanos: Long = 0
    private var running = false

    init {
        holder.addCallback(this)
        setZOrderOnTop(true)  // required for transparent SurfaceView
        holder.setFormat(android.graphics.PixelFormat.TRANSLUCENT)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {}

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        renderer.attachSurface(holder.surface, width, height)
        if (!running) {
            running = true
            lastNanos = System.nanoTime()
            Choreographer.getInstance().postFrameCallback(this)
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        running = false
        renderer.detachSurface()
    }

    override fun doFrame(frameTimeNanos: Long) {
        if (!running) return
        val dt = if (lastNanos == 0L) 0f else (frameTimeNanos - lastNanos) / 1_000_000_000f
        lastNanos = frameTimeNanos
        renderer.tick(dt)
        renderer.renderFrame()
        Choreographer.getInstance().postFrameCallback(this)
    }

    fun destroy() = renderer.destroy()
}
