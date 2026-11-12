package dev.zamin.editor

import androidx.compose.foundation.Image
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import app.cash.paparazzi.DeviceConfig
import app.cash.paparazzi.Paparazzi
import com.android.resources.ScreenOrientation
import dev.zamin.editor.ui.EditorScreen
import dev.zamin.editor.ui.ZaminTheme
import org.junit.Rule
import org.junit.Test
import android.graphics.BitmapFactory

/**
 * Editor UI skrinshotlari (Paparazzi).
 * Viewport o'rnida engine'ning haqiqiy offscreen Vulkan kadri ko'rsatiladi
 * (tools/screenshots.sh orqali tools/snapshot bilan yaratiladi).
 */
abstract class EditorScreenshotBase(device: DeviceConfig, private val frame: String) {
    @get:Rule
    val paparazzi = Paparazzi(deviceConfig = device, maxPercentDifference = 0.1)

    private val fakeEngine = object : EngineHost {
        override fun stats() = EngineStats(fps = 60f, frameMs = 16.6f, frames = 1200, cameraYaw = 0.6f, cameraPitch = 0.35f)
        override fun deviceName() = "llvmpipe (offscreen)"
    }

    @Test
    fun editor() {
        val stream = javaClass.classLoader!!.getResourceAsStream(frame)
        val bitmap = stream?.use { BitmapFactory.decodeStream(it) }?.asImageBitmap()
        paparazzi.snapshot {
            ZaminTheme {
                EditorScreen(fakeEngine) { modifier ->
                    if (bitmap != null) {
                        Image(
                            bitmap = bitmap,
                            contentDescription = null,
                            contentScale = ContentScale.Crop,
                            modifier = modifier,
                        )
                    }
                }
            }
        }
    }
}

class PhoneScreenshotTest : EditorScreenshotBase(DeviceConfig.PIXEL_6, "viewport_phone.png")

class TabletScreenshotTest : EditorScreenshotBase(
    DeviceConfig.PIXEL_C.copy(orientation = ScreenOrientation.LANDSCAPE, screenWidth = 2560, screenHeight = 1800),
    "viewport_tablet.png",
)
