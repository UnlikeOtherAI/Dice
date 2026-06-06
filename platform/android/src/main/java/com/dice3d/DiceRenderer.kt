package com.dice3d

import android.view.Surface

class DiceRenderer {
    private var nativePtr: Long = nativeCreate()

    fun attachSurface(surface: Surface, width: Int, height: Int) =
        nativeAttachSurface(nativePtr, surface, width, height)

    fun detachSurface() = nativeDetachSurface(nativePtr)

    fun resize(width: Int, height: Int) = nativeResize(nativePtr, width, height)

    fun addDie(
        sides: Int,
        bevel: Float = 0.05f,
        r: Float, g: Float, b: Float, a: Float = 1f,
        whiteNumbers: Boolean = true
    ): Int = nativeAddDie(nativePtr, sides, bevel, r, g, b, a, if (whiteNumbers) 1 else 0)

    fun removeDie(handle: Int) = nativeRemoveDie(nativePtr, handle)

    fun roll(handle: Int, result: Int, duration: Float) =
        nativeRoll(nativePtr, handle, result, duration)

    /** Load the compiled PBR material (.filamat bytes). Call before addDie to render numbers. */
    fun loadMaterial(data: ByteArray) = nativeLoadMaterial(nativePtr, data)

    /** Load the face-number atlas as raw RGBA8 pixels (width*height*4 bytes). Call after loadMaterial, before addDie. */
    fun loadAtlas(rgba: ByteArray, width: Int, height: Int) = nativeLoadAtlas(nativePtr, rgba, width, height)

    fun tick(dt: Float) = nativeTick(nativePtr, dt)
    fun renderFrame() = nativeRenderFrame(nativePtr)

    fun destroy() {
        if (nativePtr != 0L) {
            nativeDetachSurface(nativePtr)
            nativeDestroy(nativePtr)
            nativePtr = 0
        }
    }

    companion object {
        init { System.loadLibrary("dice3d") }

        @JvmStatic private external fun nativeCreate(): Long
        @JvmStatic private external fun nativeDestroy(ptr: Long)
        @JvmStatic private external fun nativeAttachSurface(ptr: Long, surface: Surface, w: Int, h: Int)
        @JvmStatic private external fun nativeDetachSurface(ptr: Long)
        @JvmStatic private external fun nativeResize(ptr: Long, w: Int, h: Int)
        @JvmStatic private external fun nativeAddDie(ptr: Long, sides: Int, bevel: Float,
                                                      r: Float, g: Float, b: Float, a: Float, white: Int): Int
        @JvmStatic private external fun nativeRemoveDie(ptr: Long, handle: Int)
        @JvmStatic private external fun nativeRoll(ptr: Long, handle: Int, result: Int, duration: Float)
        @JvmStatic private external fun nativeLoadMaterial(ptr: Long, data: ByteArray)
        @JvmStatic private external fun nativeLoadAtlas(ptr: Long, rgba: ByteArray, w: Int, h: Int)
        @JvmStatic private external fun nativeTick(ptr: Long, dt: Float)
        @JvmStatic private external fun nativeRenderFrame(ptr: Long)
    }
}
