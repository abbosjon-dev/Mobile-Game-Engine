package dev.zamin.editor.ui

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxScope
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.outlined.GridOn
import androidx.compose.material.icons.outlined.LightMode
import androidx.compose.material.icons.outlined.Videocam
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.text.TextMeasurer
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.zamin.editor.EngineStats
import kotlin.math.cos
import kotlin.math.sin

/** Viewport ustidagi elementlar: ko'rinish rejimi, o'qlar belgisi, statistika. */
@Composable
fun BoxScope.ViewportOverlay(state: EditorState, stats: EngineStats, gpuName: String) {
    val playing = state.playState != PlayState.Edit
    if (playing) {
        Box(Modifier.matchParentSize().border(2.dp, ZaminColors.Accent))
    }
    Row(
        Modifier.align(Alignment.TopStart).padding(10.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp),
    ) {
        OverlayChip(if (state.is3D) "Perspektiv" else "Ortografik", Icons.Outlined.Videocam)
        OverlayChip("Yoritilgan", Icons.Outlined.LightMode)
        OverlayChip("To'r", Icons.Outlined.GridOn)
    }
    if (state.is3D) {
        AxisGizmo(stats.cameraYaw, stats.cameraPitch, Modifier.align(Alignment.TopEnd).padding(8.dp).size(84.dp))
    }
    if (playing) {
        Box(
            Modifier.align(Alignment.TopCenter).padding(top = 50.dp)
                .background(ZaminColors.Accent, RoundedCornerShape(6.dp))
                .padding(horizontal = 10.dp, vertical = 4.dp),
        ) {
            Text(
                if (state.playState == PlayState.Paused) "PAUZA" else "O'YIN REJIMI",
                color = Color(0xFF0E2A20), fontSize = 11.sp, fontWeight = FontWeight.Bold,
            )
        }
    }
    Column(
        Modifier.align(Alignment.BottomStart).padding(10.dp)
            .background(Color(0xB316191E), RoundedCornerShape(7.dp))
            .padding(horizontal = 9.dp, vertical = 5.dp),
    ) {
        val mono = FontFamily.Monospace
        Text(
            "${"%.0f".format(stats.fps)} FPS · ${"%.1f".format(stats.frameMs)} ms",
            color = ZaminColors.Accent, fontFamily = mono, fontSize = 11.sp,
        )
        if (gpuName.isNotEmpty()) {
            Text("Vulkan · $gpuName", color = ZaminColors.TextFaint, fontFamily = mono, fontSize = 10.sp, maxLines = 1)
        }
    }
}

private data class AxisProj(val label: String, val color: Color, val x: Float, val y: Float, val depth: Float, val positive: Boolean)

/** Kamera yo'nalishiga mos X/Y/Z o'qlar belgisi (engine matritsasi bilan bir xil formula). */
@Composable
fun AxisGizmo(yaw: Float, pitch: Float, modifier: Modifier) {
    val measurer = rememberTextMeasurer()
    Canvas(modifier) {
        drawCircle(Color(0x8016191E))
        val r = size.minDimension / 2f
        val center = Offset(size.width / 2f, size.height / 2f)
        val axes = listOf(
            Triple("X", ZaminColors.AxisX, floatArrayOf(1f, 0f, 0f)),
            Triple("Y", ZaminColors.AxisY, floatArrayOf(0f, 1f, 0f)),
            Triple("Z", ZaminColors.AxisZ, floatArrayOf(0f, 0f, 1f)),
        )
        val projected = mutableListOf<AxisProj>()
        for ((label, color, v) in axes) {
            for (sign in listOf(1f, -1f)) {
                val x = v[0] * sign; val y = v[1] * sign; val z = v[2] * sign
                val cy = cos(yaw); val sy = sin(yaw)
                val x1 = cy * x + sy * z
                val z1 = -sy * x + cy * z
                val cp = cos(pitch); val sp = sin(pitch)
                val y2 = cp * y - sp * z1
                val z2 = sp * y + cp * z1
                projected += AxisProj(label, color, x1, -y2, z2, sign > 0)
            }
        }
        projected.sortedBy { it.depth }.forEach { a -> drawAxis(a, center, r * 0.68f, measurer) }
    }
}

private fun DrawScope.drawAxis(a: AxisProj, center: Offset, len: Float, measurer: TextMeasurer) {
    val end = Offset(center.x + a.x * len, center.y + a.y * len)
    val dot = if (a.positive) 9.dp.toPx() else 6.dp.toPx()
    if (a.positive) drawLine(a.color, center, end, strokeWidth = 2.dp.toPx())
    drawCircle(if (a.positive) a.color else a.color.copy(alpha = 0.35f), radius = dot, center = end)
    if (a.positive) {
        val layout = measurer.measure(a.label, TextStyle(color = Color(0xFF111317), fontSize = 10.sp, fontWeight = FontWeight.Bold))
        drawText(layout, topLeft = Offset(end.x - layout.size.width / 2f, end.y - layout.size.height / 2f))
    }
}
