package dev.zamin.editor.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.AccountTree
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.zamin.editor.EngineHost
import dev.zamin.editor.EngineStats
import kotlinx.coroutines.delay

private enum class Panel(val title: String, val icon: ImageVector) {
    Hierarchy("Ierarxiya", Icons.Filled.AccountTree),
    Inspector("Inspektor", Icons.Filled.Tune),
    Assets("Assetlar", Icons.Filled.Folder),
}

/**
 * Asosiy editor oynasi.
 * - Planshet (kenglik >= 840dp): chapda ierarxiya, markazda viewport, o'ngda inspektor, pastda assetlar.
 * - Telefon: to'liq viewport + pastki navigatsiya orqali ochiladigan panel.
 */
@Composable
fun EditorScreen(
    engine: EngineHost,
    viewport: @Composable (Modifier) -> Unit,
) {
    var stats by remember { mutableStateOf(EngineStats(0f, 0f, 0)) }
    var gpuName by remember { mutableStateOf("") }
    var playing by remember { mutableStateOf(false) }

    LaunchedEffect(engine) {
        while (true) {
            stats = engine.stats()
            if (gpuName.isEmpty()) gpuName = engine.deviceName()
            delay(500)
        }
    }

    Column(
        Modifier
            .fillMaxSize()
            .background(ZaminColors.Background)
            .safeDrawingPadding()
    ) {
        Toolbar(playing = playing, onTogglePlay = { playing = !playing })

        BoxWithConstraints(Modifier.fillMaxSize()) {
            val isTablet = maxWidth >= 840.dp
            if (isTablet) {
                TabletLayout(viewport, stats, gpuName)
            } else {
                PhoneLayout(viewport, stats, gpuName)
            }
        }
    }
}

@Composable
private fun Toolbar(playing: Boolean, onTogglePlay: () -> Unit) {
    Row(
        Modifier
            .fillMaxWidth()
            .height(48.dp)
            .background(ZaminColors.PanelHeader)
            .padding(horizontal = 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text("Zamin", color = ZaminColors.Accent, fontWeight = FontWeight.Bold, fontSize = 18.sp)
        Spacer(Modifier.width(8.dp))
        Text("Game Engine", color = ZaminColors.TextMuted, fontSize = 13.sp)
        Spacer(Modifier.weight(1f))
        IconButton(onClick = onTogglePlay) {
            Icon(
                if (playing) Icons.Filled.Pause else Icons.Filled.PlayArrow,
                contentDescription = if (playing) "To'xtatish" else "O'ynash",
                tint = if (playing) ZaminColors.Sun else ZaminColors.Accent,
            )
        }
    }
}

@Composable
private fun TabletLayout(viewport: @Composable (Modifier) -> Unit, stats: EngineStats, gpuName: String) {
    Column(Modifier.fillMaxSize()) {
        Row(Modifier.weight(1f)) {
            PanelBox(Panel.Hierarchy, Modifier.width(260.dp).fillMaxHeight())
            Viewport(viewport, stats, gpuName, Modifier.weight(1f).fillMaxHeight())
            PanelBox(Panel.Inspector, Modifier.width(300.dp).fillMaxHeight())
        }
        PanelBox(Panel.Assets, Modifier.fillMaxWidth().height(200.dp))
    }
}

@Composable
private fun PhoneLayout(viewport: @Composable (Modifier) -> Unit, stats: EngineStats, gpuName: String) {
    var open by remember { mutableStateOf<Panel?>(null) }
    Column(Modifier.fillMaxSize()) {
        Viewport(viewport, stats, gpuName, Modifier.weight(1f).fillMaxWidth())
        open?.let { PanelBox(it, Modifier.fillMaxWidth().height(260.dp)) }
        NavigationBar(containerColor = ZaminColors.PanelHeader, tonalElevation = 0.dp) {
            Panel.entries.forEach { p ->
                NavigationBarItem(
                    selected = open == p,
                    onClick = { open = if (open == p) null else p },
                    icon = { Icon(p.icon, contentDescription = p.title) },
                    label = { Text(p.title, fontSize = 11.sp) },
                )
            }
        }
    }
}

@Composable
private fun Viewport(
    viewport: @Composable (Modifier) -> Unit,
    stats: EngineStats,
    gpuName: String,
    modifier: Modifier,
) {
    Box(modifier.border(1.dp, ZaminColors.Border)) {
        viewport(Modifier.fillMaxSize())
        StatsOverlay(stats, gpuName, Modifier.align(Alignment.TopStart).padding(8.dp))
    }
}

@Composable
private fun StatsOverlay(stats: EngineStats, gpuName: String, modifier: Modifier) {
    Column(
        modifier
            .background(ZaminColors.Background.copy(alpha = 0.7f), RoundedCornerShape(6.dp))
            .padding(horizontal = 8.dp, vertical = 4.dp)
    ) {
        val mono = FontFamily.Monospace
        Text("FPS ${"%.0f".format(stats.fps)}  ${"%.1f".format(stats.frameMs)} ms",
            color = ZaminColors.Accent, fontFamily = mono, fontSize = 11.sp)
        if (gpuName.isNotEmpty()) {
            Text("Vulkan · $gpuName", color = ZaminColors.TextMuted, fontFamily = mono, fontSize = 10.sp)
        }
    }
}

@Composable
private fun PanelBox(panel: Panel, modifier: Modifier) {
    Column(modifier.background(ZaminColors.Panel).border(1.dp, ZaminColors.Border)) {
        Row(
            Modifier
                .fillMaxWidth()
                .background(ZaminColors.PanelHeader)
                .padding(horizontal = 10.dp, vertical = 6.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(6.dp),
        ) {
            Icon(panel.icon, contentDescription = null, tint = ZaminColors.TextMuted, modifier = Modifier.size(16.dp))
            Text(panel.title, color = ZaminColors.TextMuted, fontSize = 12.sp, fontWeight = FontWeight.SemiBold)
        }
        Box(Modifier.fillMaxSize().padding(10.dp)) {
            val text = when (panel) {
                Panel.Hierarchy -> "▸ Sahna\n    ▸ Kamera\n    ▸ Yorug'lik\n    ▸ Piramida"
                Panel.Inspector -> "Obyekt tanlanmagan"
                Panel.Assets -> "Assetlar hali yo'q"
            }
            Text(text, color = ZaminColors.TextMuted, fontSize = 12.sp)
        }
    }
}
