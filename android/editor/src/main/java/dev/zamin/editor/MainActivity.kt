package dev.zamin.editor

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.ui.viewinterop.AndroidView
import dev.zamin.editor.ui.EditorScreen
import dev.zamin.editor.ui.EditorState
import dev.zamin.editor.ui.ZaminTheme

class MainActivity : ComponentActivity() {
    private lateinit var engine: ZaminEngine
    private val state = EditorState()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        engine = ZaminEngine()
        setContent {
            ZaminTheme {
                EditorScreen(engine, state) { modifier ->
                    AndroidView(factory = { ctx -> EngineSurfaceView(ctx, engine, state) }, modifier = modifier)
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        engine.setPaused(false)
    }

    override fun onPause() {
        engine.setPaused(true)
        super.onPause()
    }

    override fun onDestroy() {
        engine.close()
        super.onDestroy()
    }
}
