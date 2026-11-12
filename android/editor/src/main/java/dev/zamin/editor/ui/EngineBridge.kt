package dev.zamin.editor.ui

import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.snapshotFlow
import dev.zamin.editor.EngineHost
import kotlin.math.exp
import kotlin.math.hypot

/**
 * Editor holatini engine sahnasiga uzatadi: faqat o'zgargan obyektlar yuboriladi,
 * o'chirilganlari engine'dan ham olib tashlanadi.
 */
@Composable
fun EngineSceneSync(state: EditorState, engine: EngineHost) {
    LaunchedEffect(state, engine) {
        val sent = HashMap<Int, EntitySnapshot>()
        snapshotFlow { state.objects.map { it.snapshot() } }.collect { list ->
            val alive = list.mapTo(HashSet()) { it.id }
            sent.keys.filter { it !in alive }.forEach {
                engine.removeEntity(it)
                sent.remove(it)
            }
            for (s in list) {
                if (sent[s.id] != s) {
                    engine.upsertEntity(s.id, s.parent, s.mesh.ordinal, s.visible, s.data())
                    sent[s.id] = s
                }
            }
        }
    }
    LaunchedEffect(state, engine) {
        snapshotFlow { state.selectedId }.collect { engine.setSelected(it ?: 0) }
    }
    LaunchedEffect(state, engine) {
        snapshotFlow { state.is3D }.collect { engine.setMode2D(!it) }
    }
    LaunchedEffect(state, engine) {
        snapshotFlow { state.focusRequest }.collect { if (it > 0) state.selectedId?.let(engine::focus) }
    }
}

/**
 * Viewport sensor boshqaruvi (Unity uslubi, telefonga moslangan):
 * - teginish: obyektni tanlash (bo'sh joy — tanlovni bekor qilish);
 * - ikki marta teginish: kamerani obyektga qaratish;
 * - tanlangan obyekt ustidan surish: tanlangan asbob bilan ko'chirish/aylantirish/masshtab;
 * - bo'sh joydan surish: kamerani aylantirish (2D da surish);
 * - ikki barmoq: surish (pan) va yaqinlashtirish (pinch).
 */
class ViewportGestures(private val engine: EngineHost, private val state: EditorState, private val touchSlop: Float) {
    private enum class Mode { None, Camera, Manipulate, MultiTouch }

    private var mode = Mode.None
    private var downX = 0f
    private var downY = 0f
    private var lastX = 0f
    private var lastY = 0f
    private var downTime = 0L
    private var hitId = 0
    private var moved = false
    private var lastTapTime = 0L
    private var lastTapId = 0

    fun onDown(x: Float, y: Float, timeMs: Long) {
        downX = x; downY = y; lastX = x; lastY = y; downTime = timeMs
        moved = false
        hitId = engine.pick(x, y)
        val editing = state.playState == PlayState.Edit
        mode = if (editing && state.tool != Tool.Hand && hitId != 0 && hitId == state.selectedId) {
            Mode.Manipulate
        } else {
            Mode.Camera
        }
    }

    fun onMove(x: Float, y: Float) {
        if (mode == Mode.None || mode == Mode.MultiTouch) return
        if (!moved && hypot(x - downX, y - downY) < touchSlop) return
        moved = true
        val dx = x - lastX
        val dy = y - lastY
        when (mode) {
            Mode.Camera -> engine.orbit(dx, dy)
            Mode.Manipulate -> manipulate(dx, dy, x, y)
            else -> {}
        }
        lastX = x; lastY = y
    }

    /** Ikkinchi barmoq qo'yildi: bitta barmoq rejimi tugaydi. */
    fun onMultiTouch() {
        mode = Mode.MultiTouch
    }

    fun onPan(dx: Float, dy: Float) = engine.pan(dx, dy)

    fun onZoom(factor: Float) = engine.zoom(factor)

    fun onUp(timeMs: Long) {
        val wasTap = mode != Mode.MultiTouch && !moved && timeMs - downTime < 400
        mode = Mode.None
        if (!wasTap) return
        val doubleTap = hitId != 0 && hitId == lastTapId && timeMs - lastTapTime < 350
        state.selectedId = if (hitId != 0) hitId else null
        if (doubleTap) state.requestFocus()
        lastTapTime = timeMs
        lastTapId = hitId
    }

    private fun manipulate(dx: Float, dy: Float, x: Float, y: Float) {
        val obj = state.find(hitId) ?: return
        when (state.tool) {
            Tool.Move -> engine.dragEntity(obj.id, lastX, lastY, x, y)?.let {
                obj.position = Vec3(it[0], it[1], it[2])
            }
            Tool.Rotate -> {
                val r = obj.rotation
                obj.rotation = if (state.is3D) r.copy(y = wrap(r.y + dx * 0.5f)) else r.copy(z = wrap(r.z - dx * 0.5f))
            }
            Tool.Scale -> {
                val f = exp((dx - dy) * 0.005f)
                val s = obj.scale * f
                if (s.x in 0.01f..1000f && s.y in 0.01f..1000f && s.z in 0.01f..1000f) obj.scale = s
            }
            Tool.Hand -> engine.orbit(dx, dy)
        }
    }

    private fun wrap(deg: Float): Float {
        var d = deg % 360f
        if (d > 180f) d -= 360f
        if (d < -180f) d += 360f
        return d
    }
}
