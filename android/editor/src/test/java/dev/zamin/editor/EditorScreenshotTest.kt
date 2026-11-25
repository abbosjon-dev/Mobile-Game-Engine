package dev.zamin.editor

import android.graphics.BitmapFactory
import androidx.compose.foundation.Image
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import app.cash.paparazzi.DeviceConfig
import app.cash.paparazzi.Paparazzi
import com.android.resources.ScreenOrientation
import dev.zamin.editor.project.ProjectStorage
import dev.zamin.editor.ui.EditorScreen
import dev.zamin.editor.ui.EditorState
import dev.zamin.editor.ui.PhonePanel
import dev.zamin.editor.ui.ZaminTheme
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

/**
 * Editor UI skrinshotlari (Paparazzi).
 * Viewport o'rnida engine'ning haqiqiy offscreen Vulkan kadri ko'rsatiladi
 * (tools/screenshots.sh orqali tools/snapshot bilan yaratiladi).
 * Assetlar paneli vaqtinchalik papkadagi haqiqiy loyiha fayllarini ko'rsatadi.
 */
abstract class EditorScreenshotBase(device: DeviceConfig, private val frame: String) {
    @get:Rule
    val paparazzi = Paparazzi(deviceConfig = device, maxPercentDifference = 0.1)

    @get:Rule
    val tmp = TemporaryFolder()

    private val fakeEngine = object : EngineHost {
        override fun stats() = EngineStats(fps = 60f, frameMs = 16.6f, frames = 1200, cameraYaw = 0.6f, cameraPitch = 0.42f, drawCalls = 10)
        override fun deviceName() = "llvmpipe (offscreen)"
    }

    /** Namuna loyiha: standart papkalar, asosiy sahna va bir nechta asset. */
    protected fun demoState(): EditorState {
        val root = tmp.newFolder("Mening o'yinim")
        val state = EditorState(ProjectStorage(root))
        state.openProject()
        root.resolve("Skriptlar/Oyinchi.luau").writeText("function onStart() end")
        root.resolve("Teksturalar/Maysa.png").writeBytes(ByteArray(4))
        root.resolve("Audio/Fon.ogg").writeBytes(ByteArray(4))
        root.resolve("README.txt").writeText("Zamin loyihasi")
        state.refreshAssets()
        state.message = null
        state.selectedId = 4
        return state
    }

    protected fun snapshot(state: EditorState, frameName: String = frame) {
        val stream = javaClass.classLoader!!.getResourceAsStream(frameName)
        val bitmap = stream?.use { BitmapFactory.decodeStream(it) }?.asImageBitmap()
        paparazzi.snapshot {
            ZaminTheme {
                EditorScreen(fakeEngine, state) { modifier ->
                    if (bitmap != null) {
                        Image(bitmap = bitmap, contentDescription = null, contentScale = ContentScale.Crop, modifier = modifier)
                    }
                }
            }
        }
    }

    @Test
    fun editor() = snapshot(demoState())
}

private val TABLET = DeviceConfig.PIXEL_C.copy(orientation = ScreenOrientation.LANDSCAPE, screenWidth = 2560, screenHeight = 1800)

class PhoneScreenshotTest : EditorScreenshotBase(DeviceConfig.PIXEL_6, "viewport_phone.png") {
    /** O'yin rejimi: viewport'da sahnadagi kamera ko'rinishi, gizmo va to'r yo'q. */
    @Test
    fun play() = snapshot(demoState().apply { play() }, "viewport_play_phone.png")

    /** Assetlar paneli: loyiha papkasidagi haqiqiy fayllar, saqlash xabari. */
    @Test
    fun assets() = snapshot(
        demoState().apply {
            initialPhonePanel = PhonePanel.Assets
            saveScene()
        },
    )
}

class TabletScreenshotTest : EditorScreenshotBase(TABLET, "viewport_tablet.png") {
    /** Yorug'lik tanlangan, "Skriptlar" papkasi ochiq. */
    @Test
    fun gizmos() = snapshot(
        demoState().apply {
            selectedId = 3
            openAssetDir("Skriptlar")
        },
        "viewport_gizmos_tablet.png",
    )
}
