package dev.zamin.editor

import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.ui.viewinterop.AndroidView
import dev.zamin.editor.project.ProjectStorage
import dev.zamin.editor.ui.EditorScreen
import dev.zamin.editor.ui.EditorState
import dev.zamin.editor.ui.PlayState
import dev.zamin.editor.ui.ZaminTheme
import java.io.File

class MainActivity : ComponentActivity() {
    private lateinit var engine: ZaminEngine
    private lateinit var state: EditorState

    // Tizim fayl tanlagichi (SAF): tanlangan fayl loyihaning ochiq papkasiga nusxalanadi
    private val importLauncher = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri != null) importFile(uri)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        engine = ZaminEngine()
        state = EditorState(ProjectStorage(File(filesDir, "projects/Mening o'yinim")))
        state.onImportRequest = { importLauncher.launch(arrayOf("*/*")) }
        state.openProject()
        setContent {
            ZaminTheme {
                EditorScreen(engine, state) { modifier ->
                    AndroidView(factory = { ctx -> EngineSurfaceView(ctx, engine, state) }, modifier = modifier)
                }
            }
        }
    }

    private fun importFile(uri: Uri) {
        val name = contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { c ->
            if (c.moveToFirst()) c.getString(0) else null
        } ?: uri.lastPathSegment ?: "fayl"
        val path = runCatching {
            contentResolver.openInputStream(uri)?.use { state.project?.import(state.assetDir, name, it) }
        }.getOrNull()
        state.onImported(path)
    }

    override fun onResume() {
        super.onResume()
        engine.setPaused(false)
    }

    override fun onPause() {
        engine.setPaused(true)
        // Ilova fonga o'tganda sahna avtomatik saqlanadi (o'yin rejimida emas)
        if (state.playState == PlayState.Edit) state.saveScene()
        super.onPause()
    }

    override fun onDestroy() {
        engine.close()
        super.onDestroy()
    }
}
