package dev.zamin.editor.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
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
import androidx.compose.material.icons.outlined.AccountTree
import androidx.compose.material.icons.outlined.Folder
import androidx.compose.material.icons.outlined.Tune
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.zamin.editor.EngineHost
import dev.zamin.editor.EngineStats
import kotlinx.coroutines.delay

private enum class PhonePanel(val title: String, val icon: ImageVector) {
    Hierarchy("Ierarxiya", Icons.Outlined.AccountTree),
    Inspector("Inspektor", Icons.Outlined.Tune),
    Assets("Assetlar", Icons.Outlined.Folder),
}

/**
 * Asosiy editor oynasi.
 * - Planshet (kenglik >= 840dp): chapda ierarxiya, markazda viewport, o'ngda inspektor, pastda assetlar.
 * - Telefon: to'liq viewport, chapda suzuvchi asboblar, pastda tablar va ochiladigan panel.
 */
@Composable
fun EditorScreen(
    engine: EngineHost,
    state: EditorState = remember { EditorState() },
    viewport: @Composable (Modifier) -> Unit,
) {
    var stats by remember { mutableStateOf(EngineStats()) }
    var gpuName by remember { mutableStateOf("") }

    LaunchedEffect(engine) {
        while (true) {
            stats = engine.stats()
            if (gpuName.isEmpty()) gpuName = engine.deviceName()
            delay(33)
        }
    }

    BoxWithConstraints(
        Modifier
            .fillMaxSize()
            .background(ZaminColors.Background)
            .safeDrawingPadding()
    ) {
        val isTablet = maxWidth >= 840.dp
        Column(Modifier.fillMaxSize()) {
            EditorToolbar(state, compact = !isTablet)
            Divider()
            if (isTablet) {
                TabletLayout(state, viewport, stats, gpuName)
            } else {
                PhoneLayout(state, viewport, stats, gpuName)
            }
        }
    }
}

@Composable
private fun Divider(vertical: Boolean = false) {
    if (vertical) Box(Modifier.width(1.dp).fillMaxHeight().background(ZaminColors.Border))
    else Box(Modifier.fillMaxWidth().height(1.dp).background(ZaminColors.Border))
}

@Composable
private fun TabletLayout(
    state: EditorState,
    viewport: @Composable (Modifier) -> Unit,
    stats: EngineStats,
    gpuName: String,
) {
    Row(Modifier.fillMaxSize()) {
        HierarchyPanel(state, Modifier.width(260.dp).fillMaxHeight())
        Divider(vertical = true)
        Column(Modifier.weight(1f)) {
            Box(Modifier.weight(1f).fillMaxWidth()) {
                viewport(Modifier.fillMaxSize())
                ViewportOverlay(state, stats, gpuName)
            }
            Divider()
            AssetsPanel(state, Modifier.fillMaxWidth().height(190.dp))
        }
        Divider(vertical = true)
        InspectorPanel(state, Modifier.width(320.dp).fillMaxHeight())
    }
}

@Composable
private fun PhoneLayout(
    state: EditorState,
    viewport: @Composable (Modifier) -> Unit,
    stats: EngineStats,
    gpuName: String,
) {
    var open by remember { mutableStateOf<PhonePanel?>(null) }
    Column(Modifier.fillMaxSize()) {
        Box(Modifier.weight(1f).fillMaxWidth()) {
            viewport(Modifier.fillMaxSize())
            ViewportOverlay(state, stats, gpuName)
            FloatingTools(state, Modifier.align(Alignment.CenterStart).padding(start = 10.dp))
        }
        open?.let { panel ->
            Column(
                Modifier
                    .fillMaxWidth()
                    .height(330.dp)
                    .clip(RoundedCornerShape(topStart = 14.dp, topEnd = 14.dp))
                    .background(ZaminColors.Panel)
            ) {
                Box(Modifier.fillMaxWidth().height(14.dp), contentAlignment = Alignment.Center) {
                    Box(Modifier.size(width = 36.dp, height = 4.dp).clip(RoundedCornerShape(2.dp)).background(ZaminColors.FieldHover))
                }
                when (panel) {
                    PhonePanel.Hierarchy -> HierarchyPanel(state, Modifier.fillMaxSize())
                    PhonePanel.Inspector -> InspectorPanel(state, Modifier.fillMaxSize())
                    PhonePanel.Assets -> AssetsPanel(state, Modifier.fillMaxSize())
                }
            }
        }
        Divider()
        Row(
            Modifier.fillMaxWidth().height(58.dp).background(ZaminColors.PanelHeader).padding(horizontal = 8.dp),
            horizontalArrangement = Arrangement.SpaceEvenly,
            verticalAlignment = Alignment.CenterVertically,
        ) {
            PhonePanel.entries.forEach { p ->
                val sel = open == p
                Column(
                    Modifier
                        .weight(1f)
                        .clip(RoundedCornerShape(10.dp))
                        .clickable { open = if (sel) null else p }
                        .padding(vertical = 6.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    Box(
                        Modifier
                            .size(width = 52.dp, height = 26.dp)
                            .clip(RoundedCornerShape(13.dp))
                            .background(if (sel) ZaminColors.AccentDim else Color.Transparent),
                        contentAlignment = Alignment.Center,
                    ) {
                        Icon(p.icon, p.title, tint = if (sel) ZaminColors.Accent else ZaminColors.TextMuted, modifier = Modifier.size(19.dp))
                    }
                    Spacer(Modifier.height(2.dp))
                    Text(
                        p.title, fontSize = 11.sp,
                        color = if (sel) ZaminColors.Text else ZaminColors.TextMuted,
                        fontWeight = if (sel) FontWeight.SemiBold else FontWeight.Normal,
                    )
                }
            }
        }
    }
}

/** Telefon uchun viewport chetidagi vertikal asboblar paneli. */
@Composable
private fun FloatingTools(state: EditorState, modifier: Modifier) {
    Column(
        modifier
            .clip(RoundedCornerShape(12.dp))
            .background(Color(0xE616191E))
            .border(1.dp, ZaminColors.Border, RoundedCornerShape(12.dp))
            .padding(4.dp),
        verticalArrangement = Arrangement.spacedBy(2.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Tool.entries.forEach { t ->
            ToolIconButton(t.icon(), t.label, selected = state.tool == t, size = 40.dp) { state.tool = t }
        }
        Box(Modifier.width(26.dp).height(1.dp).background(ZaminColors.Border))
        Box(
            Modifier.size(40.dp).clip(RoundedCornerShape(7.dp)).clickable { state.is3D = !state.is3D },
            contentAlignment = Alignment.Center,
        ) {
            Text(if (state.is3D) "3D" else "2D", color = ZaminColors.Accent, fontSize = 13.sp, fontWeight = FontWeight.Bold)
        }
    }
}
