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
    @JvmStatic external fun nativeOnPan(handle: Long, dx: Float, dy: Float)
    @JvmStatic external fun nativeOnZoom(handle: Long, factor: Float)
    @JvmStatic external fun nativeSetMode2D(handle: Long, mode2D: Boolean)
    @JvmStatic external fun nativeFocus(handle: Long, id: Int)
    @JvmStatic external fun nativeUpsertEntity(handle: Long, id: Int, parent: Int, mesh: Int, visible: Boolean, data: FloatArray)
    @JvmStatic external fun nativeRemoveEntity(handle: Long, id: Int)
    @JvmStatic external fun nativeSetSelected(handle: Long, id: Int)
    @JvmStatic external fun nativePick(handle: Long, x: Float, y: Float): Int
    @JvmStatic external fun nativeDragEntity(handle: Long, id: Int, x0: Float, y0: Float, x1: Float, y1: Float): FloatArray?
    @JvmStatic external fun nativeGetStats(handle: Long): FloatArray
    @JvmStatic external fun nativeGetDeviceName(handle: Long): String
}

data class EngineStats(
    val fps: Float = 0f,
    val frameMs: Float = 0f,
    val frames: Long = 0,
    val cameraYaw: Float = 0.6f,
    val cameraPitch: Float = 0.45f,
    val drawCalls: Int = 0,
)

/**
 * Editor UI engine bilan shu interfeys orqali gaplashadi.
 * Testlarda soxta nusxa ishlatiladi, shuning uchun sahna metodlari bo'sh standartga ega.
 */
interface EngineHost {
    fun stats(): EngineStats
    fun deviceName(): String

    // ---- Kamera ----
    fun orbit(dx: Float, dy: Float) {}
    fun pan(dx: Float, dy: Float) {}
    fun zoom(factor: Float) {}
    fun setMode2D(mode2D: Boolean) {}
    fun focus(id: Int) {}

    // ---- Sahna ----
    /** data: [px py pz, rx ry rz, sx sy sz, r g b a] */
    fun upsertEntity(id: Int, parent: Int, mesh: Int, visible: Boolean, data: FloatArray) {}
    fun removeEntity(id: Int) {}
    fun setSelected(id: Int) {}
    /** Ekran nuqtasidagi obyekt id si (0 — hech narsa). */
    fun pick(x: Float, y: Float): Int = 0
    /** Obyektni barmoq ortidan siljitadi; yangi lokal pozitsiya yoki null. */
    fun dragEntity(id: Int, x0: Float, y0: Float, x1: Float, y1: Float): FloatArray? = null
}

/** Engine nusxasi uchun Kotlin o'rami. */
class ZaminEngine : EngineHost, AutoCloseable {
    private var handle: Long = ZaminNative.nativeCreate()

    fun surfaceCreated(surface: Surface) = ifAlive { ZaminNative.nativeSurfaceCreated(it, surface) }
    fun surfaceChanged(width: Int, height: Int) = ifAlive { ZaminNative.nativeSurfaceChanged(it, width, height) }
    fun surfaceDestroyed() = ifAlive { ZaminNative.nativeSurfaceDestroyed(it) }
    fun setPaused(paused: Boolean) = ifAlive { ZaminNative.nativeSetPaused(it, paused) }

    override fun orbit(dx: Float, dy: Float) = ifAlive { ZaminNative.nativeOnDrag(it, dx, dy) }
    override fun pan(dx: Float, dy: Float) = ifAlive { ZaminNative.nativeOnPan(it, dx, dy) }
    override fun zoom(factor: Float) = ifAlive { ZaminNative.nativeOnZoom(it, factor) }
    override fun setMode2D(mode2D: Boolean) = ifAlive { ZaminNative.nativeSetMode2D(it, mode2D) }
    override fun focus(id: Int) = ifAlive { ZaminNative.nativeFocus(it, id) }

    override fun upsertEntity(id: Int, parent: Int, mesh: Int, visible: Boolean, data: FloatArray) =
        ifAlive { ZaminNative.nativeUpsertEntity(it, id, parent, mesh, visible, data) }
    override fun removeEntity(id: Int) = ifAlive { ZaminNative.nativeRemoveEntity(it, id) }
    override fun setSelected(id: Int) = ifAlive { ZaminNative.nativeSetSelected(it, id) }
    override fun pick(x: Float, y: Float): Int = if (handle == 0L) 0 else ZaminNative.nativePick(handle, x, y)
    override fun dragEntity(id: Int, x0: Float, y0: Float, x1: Float, y1: Float): FloatArray? =
        if (handle == 0L) null else ZaminNative.nativeDragEntity(handle, id, x0, y0, x1, y1)

    override fun stats(): EngineStats {
        if (handle == 0L) return EngineStats()
        val s = ZaminNative.nativeGetStats(handle)
        return EngineStats(s[0], s[1], s[2].toLong(), s[3], s[4], s[5].toInt())
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
