package dev.zamin.editor

import android.view.Surface

/** C++ engine'ga past darajadagi JNI chaqiruvlar. */
internal object ZaminNative {
    init {
        System.loadLibrary("zamin")
    }

    @JvmStatic external fun nativeCreate(): Long
    @JvmStatic external fun nativeDestroy(handle: Long)
    @JvmStatic external fun nativeSurfaceCreated(handle: Long, surface: Surface)
    @JvmStatic external fun nativeSurfaceChanged(handle: Long, width: Int, height: Int)
    @JvmStatic external fun nativeSurfaceDestroyed(handle: Long)
    @JvmStatic external fun nativeSetPaused(handle: Long, paused: Boolean)
    @JvmStatic external fun nativeOnDrag(handle: Long, dx: Float, dy: Float)
    @JvmStatic external fun nativeGetStats(handle: Long): FloatArray
    @JvmStatic external fun nativeGetDeviceName(handle: Long): String
}

data class EngineStats(val fps: Float, val frameMs: Float, val frames: Long)

/** Editor UI engine'dan nimani so'rashi mumkin (testlarda soxta nusxa ishlatiladi). */
interface EngineHost {
    fun stats(): EngineStats
    fun deviceName(): String
}

/** Engine nusxasi uchun Kotlin o'rami. */
class ZaminEngine : EngineHost, AutoCloseable {
    private var handle: Long = ZaminNative.nativeCreate()

    fun surfaceCreated(surface: Surface) = ifAlive { ZaminNative.nativeSurfaceCreated(it, surface) }
    fun surfaceChanged(width: Int, height: Int) = ifAlive { ZaminNative.nativeSurfaceChanged(it, width, height) }
    fun surfaceDestroyed() = ifAlive { ZaminNative.nativeSurfaceDestroyed(it) }
    fun setPaused(paused: Boolean) = ifAlive { ZaminNative.nativeSetPaused(it, paused) }
    fun drag(dx: Float, dy: Float) = ifAlive { ZaminNative.nativeOnDrag(it, dx, dy) }

    override fun stats(): EngineStats {
        if (handle == 0L) return EngineStats(0f, 0f, 0)
        val s = ZaminNative.nativeGetStats(handle)
        return EngineStats(s[0], s[1], s[2].toLong())
    }

    override fun deviceName(): String = if (handle == 0L) "" else ZaminNative.nativeGetDeviceName(handle)

    override fun close() {
        if (handle != 0L) {
            ZaminNative.nativeDestroy(handle)
            handle = 0L
        }
    }

    private inline fun ifAlive(block: (Long) -> Unit) {
        if (handle != 0L) block(handle)
    }
}
