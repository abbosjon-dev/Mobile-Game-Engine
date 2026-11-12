package dev.zamin.editor

import android.annotation.SuppressLint
import android.content.Context
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView

/** Engine viewport'i: Vulkan shu Surface'ga chizadi. */
@SuppressLint("ViewConstructor", "ClickableViewAccessibility")
class EngineSurfaceView(context: Context, private val engine: ZaminEngine) :
    SurfaceView(context), SurfaceHolder.Callback {

    private var lastX = 0f
    private var lastY = 0f

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
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                lastX = event.x
                lastY = event.y
            }
            MotionEvent.ACTION_MOVE -> {
                engine.drag(event.x - lastX, event.y - lastY)
                lastX = event.x
                lastY = event.y
            }
        }
        return true
    }
}
