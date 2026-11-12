package dev.zamin.editor

import android.annotation.SuppressLint
import android.content.Context
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.SurfaceHolder
import android.view.SurfaceView

/** Engine viewport'i: Vulkan shu Surface'ga chizadi. */
@SuppressLint("ViewConstructor", "ClickableViewAccessibility")
class EngineSurfaceView(context: Context, private val engine: ZaminEngine) :
    SurfaceView(context), SurfaceHolder.Callback {

    private var lastX = 0f
    private var lastY = 0f

    // Ikki barmoq bilan yaqinlashtirish/uzoqlashtirish
    private val scaleDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScale(detector: ScaleGestureDetector): Boolean {
            engine.zoom(detector.scaleFactor)
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
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_UP -> {
                val idx = if (event.actionMasked == MotionEvent.ACTION_POINTER_UP && event.actionIndex == 0) 1 else 0
                lastX = event.getX(idx)
                lastY = event.getY(idx)
            }
            MotionEvent.ACTION_MOVE -> {
                // Bitta barmoq — kamerani aylantirish (orbit)
                if (event.pointerCount == 1 && !scaleDetector.isInProgress) {
                    engine.drag(event.x - lastX, event.y - lastY)
                }
                lastX = event.getX(0)
                lastY = event.getY(0)
            }
        }
        return true
    }
}
