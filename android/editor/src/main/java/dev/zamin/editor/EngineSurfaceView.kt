package dev.zamin.editor

import android.annotation.SuppressLint
import android.content.Context
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.ViewConfiguration
import dev.zamin.editor.ui.EditorState
import dev.zamin.editor.ui.ViewportGestures

/** Engine viewport'i: Vulkan shu Surface'ga chizadi, sensor hodisalarini ViewportGestures ga beradi. */
@SuppressLint("ViewConstructor", "ClickableViewAccessibility")
class EngineSurfaceView(context: Context, private val engine: ZaminEngine, state: EditorState) :
    SurfaceView(context), SurfaceHolder.Callback {

    private val gestures = ViewportGestures(engine, state, ViewConfiguration.get(context).scaledTouchSlop.toFloat())
    private var focusX = 0f
    private var focusY = 0f

    // Ikki barmoq bilan yaqinlashtirish/uzoqlashtirish va surish
    private val scaleDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
            focusX = detector.focusX
            focusY = detector.focusY
            return true
        }

        override fun onScale(detector: ScaleGestureDetector): Boolean {
            gestures.onZoom(detector.scaleFactor)
            gestures.onPan(detector.focusX - focusX, detector.focusY - focusY)
            focusX = detector.focusX
            focusY = detector.focusY
            return true
        }
    })

    init {
        holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        engine.surfaceCreated(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        engine.surfaceChanged(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        engine.surfaceDestroyed()
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        scaleDetector.onTouchEvent(event)
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> gestures.onDown(event.x, event.y, event.eventTime)
            MotionEvent.ACTION_POINTER_DOWN -> gestures.onMultiTouch()
            MotionEvent.ACTION_MOVE -> if (event.pointerCount == 1) gestures.onMove(event.x, event.y)
            MotionEvent.ACTION_UP -> gestures.onUp(event.eventTime)
            MotionEvent.ACTION_CANCEL -> gestures.onMultiTouch()
        }
        return true
    }
}
