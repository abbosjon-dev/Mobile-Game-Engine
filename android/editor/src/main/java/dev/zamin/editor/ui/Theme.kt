package dev.zamin.editor.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

object ZaminColors {
    val Background = Color(0xFF16181D)
    val Panel = Color(0xFF1E2128)
    val PanelHeader = Color(0xFF252932)
    val Border = Color(0xFF2E333D)
    val Accent = Color(0xFF3DB98A)   // yashil — "zamin"
    val Sun = Color(0xFFF2B33D)
    val TextMuted = Color(0xFF8A93A3)
}

private val scheme = darkColorScheme(
    primary = ZaminColors.Accent,
    secondary = ZaminColors.Sun,
    background = ZaminColors.Background,
    surface = ZaminColors.Panel,
    surfaceVariant = ZaminColors.PanelHeader,
    outline = ZaminColors.Border,
)

@Composable
fun ZaminTheme(content: @Composable () -> Unit) {
    MaterialTheme(colorScheme = scheme, content = content)
}
