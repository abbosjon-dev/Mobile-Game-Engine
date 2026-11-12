package dev.zamin.editor.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

object ZaminColors {
    val Background = Color(0xFF111317)
    val Panel = Color(0xFF1A1D23)
    val PanelAlt = Color(0xFF1F232A)
    val PanelHeader = Color(0xFF16191E)
    val Field = Color(0xFF262A32)
    val FieldHover = Color(0xFF2E333C)
    val Border = Color(0xFF2A2F37)
    val Divider = Color(0xFF23272E)
    val Accent = Color(0xFF3DB98A)       // yashil — "zamin"
    val AccentDim = Color(0x333DB98A)
    val Selection = Color(0xFF214A3C)
    val Sun = Color(0xFFF2B33D)
    val Text = Color(0xFFE3E7ED)
    val TextMuted = Color(0xFF8D96A5)
    val TextFaint = Color(0xFF5E6672)
    val AxisX = Color(0xFFE5534B)
    val AxisY = Color(0xFF6BC46D)
    val AxisZ = Color(0xFF4C8DF0)
    val Danger = Color(0xFFE5534B)
}

object ZaminDims {
    val RowHeight = 30.dp
    val PanelHeader = 34.dp
    val Toolbar = 50.dp
    val TextSize = 13.sp
    val SmallText = 11.sp
}

private val scheme = darkColorScheme(
    primary = ZaminColors.Accent,
    secondary = ZaminColors.Sun,
    background = ZaminColors.Background,
    surface = ZaminColors.Panel,
    surfaceVariant = ZaminColors.PanelAlt,
    outline = ZaminColors.Border,
    onSurface = ZaminColors.Text,
    onBackground = ZaminColors.Text,
)

@Composable
fun ZaminTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = scheme, content = content)
}
