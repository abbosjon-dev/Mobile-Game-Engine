package dev.zamin.editor.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Audiotrack
import androidx.compose.material.icons.filled.Code
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Image
import androidx.compose.material.icons.filled.Landscape
import androidx.compose.material.icons.filled.Lightbulb
import androidx.compose.material.icons.filled.Palette
import androidx.compose.material.icons.filled.Videocam
import androidx.compose.material.icons.outlined.CropFree
import androidx.compose.material.icons.outlined.Image
import androidx.compose.material.icons.outlined.Public
import androidx.compose.material.icons.outlined.ViewInAr
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

fun ObjectType.icon(): ImageVector = when (this) {
    ObjectType.Scene -> Icons.Outlined.Public
    ObjectType.Camera -> Icons.Filled.Videocam
    ObjectType.Light -> Icons.Filled.Lightbulb
    ObjectType.Mesh -> Icons.Outlined.ViewInAr
    ObjectType.Sprite -> Icons.Outlined.Image
    ObjectType.Empty -> Icons.Outlined.CropFree
}

fun ObjectType.tint(): Color = when (this) {
    ObjectType.Scene -> ZaminColors.TextMuted
    ObjectType.Camera -> Color(0xFF9B8CF0)
    ObjectType.Light -> ZaminColors.Sun
    ObjectType.Mesh -> ZaminColors.AxisZ
    ObjectType.Sprite -> Color(0xFFE87DB0)
    ObjectType.Empty -> ZaminColors.TextMuted
}

fun AssetKind.icon(): ImageVector = when (this) {
    AssetKind.Folder -> Icons.Filled.Folder
    AssetKind.Scene -> Icons.Filled.Landscape
    AssetKind.Material -> Icons.Filled.Palette
    AssetKind.Texture -> Icons.Filled.Image
    AssetKind.Mesh -> Icons.Outlined.ViewInAr
    AssetKind.Script -> Icons.Filled.Code
    AssetKind.Audio -> Icons.Filled.Audiotrack
}

fun AssetKind.tint(): Color = when (this) {
    AssetKind.Folder -> Color(0xFFD9B45A)
    AssetKind.Scene -> ZaminColors.Accent
    AssetKind.Material -> Color(0xFFE87DB0)
    AssetKind.Texture -> Color(0xFF5FB8E0)
    AssetKind.Mesh -> ZaminColors.AxisZ
    AssetKind.Script -> Color(0xFFB48CF0)
    AssetKind.Audio -> Color(0xFFF08C5A)
}

/** Panel sarlavhasi: belgi, nom va o'ng tomonda amallar. */
@Composable
fun PanelHeader(
    title: String,
    icon: ImageVector,
    actions: @Composable RowScope.() -> Unit = {},
) {
    Row(
        Modifier
            .fillMaxWidth()
            .height(ZaminDims.PanelHeader)
            .background(ZaminColors.PanelHeader)
            .padding(start = 12.dp, end = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Icon(icon, null, tint = ZaminColors.TextMuted, modifier = Modifier.size(15.dp))
        Spacer(Modifier.width(8.dp))
        Text(
            title.uppercase(), color = ZaminColors.TextMuted, fontSize = 11.sp,
            fontWeight = FontWeight.SemiBold, letterSpacing = 0.8.sp,
        )
        Spacer(Modifier.weight(1f))
        actions()
    }
}

/** Kichik belgili tugma (toolbar va panel amallari uchun). */
@Composable
fun ToolIconButton(
    icon: ImageVector,
    description: String,
    selected: Boolean = false,
    tint: Color = ZaminColors.TextMuted,
    size: Dp = 34.dp,
    onClick: () -> Unit,
) {
    Box(
        Modifier
            .size(size)
            .clip(RoundedCornerShape(7.dp))
            .background(if (selected) ZaminColors.AccentDim else Color.Transparent)
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Icon(
            icon, contentDescription = description,
            tint = if (selected) ZaminColors.Accent else tint,
            modifier = Modifier.size(size * 0.55f),
        )
    }
}

/** Bir nechta tugmani bitta ramkada guruhlash. */
@Composable
fun ToolGroup(content: @Composable RowScope.() -> Unit) {
    Row(
        Modifier
            .clip(RoundedCornerShape(9.dp))
            .background(ZaminColors.Field)
            .border(1.dp, ZaminColors.Border, RoundedCornerShape(9.dp))
            .padding(2.dp),
        horizontalArrangement = Arrangement.spacedBy(2.dp),
        verticalAlignment = Alignment.CenterVertically,
        content = content,
    )
}

/** Viewport ustidagi kichik "chip" (masalan: Perspektiv, Yoritilgan). */
@Composable
fun OverlayChip(text: String, icon: ImageVector? = null, onClick: () -> Unit = {}) {
    Row(
        Modifier
            .clip(RoundedCornerShape(7.dp))
            .background(Color(0xCC16191E))
            .border(1.dp, ZaminColors.Border, RoundedCornerShape(7.dp))
            .clickable(onClick = onClick)
            .padding(horizontal = 9.dp, vertical = 5.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        if (icon != null) {
            Icon(icon, null, tint = ZaminColors.TextMuted, modifier = Modifier.size(13.dp))
            Spacer(Modifier.width(5.dp))
        }
        Text(text, color = ZaminColors.Text, fontSize = 11.sp)
    }
}
