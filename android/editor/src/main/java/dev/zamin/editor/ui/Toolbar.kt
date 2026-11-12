package dev.zamin.editor.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.SkipNext
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material.icons.outlined.OpenWith
import androidx.compose.material.icons.outlined.PanTool
import androidx.compose.material.icons.outlined.PhotoSizeSelectSmall
import androidx.compose.material.icons.outlined.Settings
import androidx.compose.material.icons.outlined.Sync
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

fun Tool.icon(): ImageVector = when (this) {
    Tool.Hand -> Icons.Outlined.PanTool
    Tool.Move -> Icons.Outlined.OpenWith
    Tool.Rotate -> Icons.Outlined.Sync
    Tool.Scale -> Icons.Outlined.PhotoSizeSelectSmall
}

@Composable
fun ZaminLogo(size: Int = 26) {
    Box(
        Modifier
            .size(size.dp)
            .clip(RoundedCornerShape(7.dp))
            .background(ZaminColors.Accent),
        contentAlignment = Alignment.Center,
    ) {
        Text("Z", color = Color.White, fontWeight = FontWeight.Black, fontSize = (size * 0.58f).sp)
    }
}

@Composable
private fun ProjectTitle(state: EditorState, compact: Boolean) {
    Row(
        Modifier.clip(RoundedCornerShape(7.dp)).clickable { }.padding(horizontal = 6.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        ZaminLogo()
        Spacer(Modifier.width(9.dp))
        Column {
            Text(state.projectName, color = ZaminColors.Text, fontSize = 13.sp, fontWeight = FontWeight.SemiBold, maxLines = 1)
            if (!compact) Text("Asosiy sahna", color = ZaminColors.TextFaint, fontSize = 10.sp)
        }
        Icon(Icons.Filled.ExpandMore, null, tint = ZaminColors.TextFaint, modifier = Modifier.size(16.dp))
    }
}

@Composable
fun PlayControls(state: EditorState) {
    ToolGroup {
        val playing = state.playState != PlayState.Edit
        ToolIconButton(
            if (playing) Icons.Filled.Stop else Icons.Filled.PlayArrow,
            if (playing) "To'xtatish" else "O'ynash",
            selected = playing,
            tint = ZaminColors.Accent,
        ) { state.playState = if (playing) PlayState.Edit else PlayState.Playing }
        ToolIconButton(
            Icons.Filled.Pause, "Pauza",
            selected = state.playState == PlayState.Paused,
        ) {
            state.playState = when (state.playState) {
                PlayState.Playing -> PlayState.Paused
                PlayState.Paused -> PlayState.Playing
                PlayState.Edit -> PlayState.Edit
            }
        }
        ToolIconButton(Icons.Filled.SkipNext, "Bir kadr") { }
    }
}

@Composable
fun DimensionToggle(state: EditorState) {
    ToolGroup {
        listOf(false to "2D", true to "3D").forEach { (is3D, label) ->
            val sel = state.is3D == is3D
            Box(
                Modifier
                    .height(30.dp)
                    .clip(RoundedCornerShape(7.dp))
                    .background(if (sel) ZaminColors.AccentDim else Color.Transparent)
                    .clickable { state.is3D = is3D }
                    .padding(horizontal = 11.dp),
                contentAlignment = Alignment.Center,
            ) {
                Text(label, color = if (sel) ZaminColors.Accent else ZaminColors.TextMuted, fontSize = 12.sp, fontWeight = FontWeight.Bold)
            }
        }
    }
}

@Composable
fun TransformTools(state: EditorState) {
    ToolGroup {
        Tool.entries.forEach { t ->
            ToolIconButton(t.icon(), t.label, selected = state.tool == t, size = 30.dp) { state.tool = t }
        }
    }
}

@Composable
private fun BuildButton(compact: Boolean) {
    Row(
        Modifier
            .height(34.dp)
            .clip(RoundedCornerShape(8.dp))
            .background(ZaminColors.Accent)
            .clickable { }
            .padding(horizontal = if (compact) 9.dp else 12.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Icon(Icons.Filled.Build, "Build", tint = Color(0xFF0E2A20), modifier = Modifier.size(16.dp))
        if (!compact) {
            Spacer(Modifier.width(6.dp))
            Text("Build", color = Color(0xFF0E2A20), fontSize = 13.sp, fontWeight = FontWeight.Bold)
        }
    }
}

/** Yuqori panel. Planshetda to'liq, telefonda ixcham. */
@Composable
fun EditorToolbar(state: EditorState, compact: Boolean) {
    Box(
        Modifier
            .fillMaxWidth()
            .height(ZaminDims.Toolbar)
            .background(ZaminColors.PanelHeader)
            .padding(horizontal = 8.dp),
    ) {
        Row(Modifier.align(Alignment.CenterStart), verticalAlignment = Alignment.CenterVertically) {
            ProjectTitle(state, compact)
            if (!compact) {
                Spacer(Modifier.width(12.dp))
                TransformTools(state)
                Spacer(Modifier.width(8.dp))
                DimensionToggle(state)
            }
        }
        if (!compact) {
            Box(Modifier.align(Alignment.Center)) { PlayControls(state) }
        }
        Row(
            Modifier.align(Alignment.CenterEnd),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(6.dp),
        ) {
            if (compact) PlayControls(state)
            BuildButton(compact)
            if (!compact) ToolIconButton(Icons.Outlined.Settings, "Sozlamalar") { }
        }
    }
}
