package dev.zamin.editor.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ChevronRight
import androidx.compose.material.icons.outlined.FileDownload
import androidx.compose.material.icons.outlined.Folder
import androidx.compose.material.icons.outlined.Save
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.zamin.editor.project.AssetItem
import dev.zamin.editor.project.AssetKind

@Composable
fun AssetsPanel(state: EditorState, modifier: Modifier = Modifier) {
    var selected by remember { mutableStateOf<String?>(null) }
    Column(modifier.background(ZaminColors.Panel)) {
        PanelHeader("Assetlar", Icons.Outlined.Folder) {
            ToolIconButton(Icons.Outlined.Save, "Sahnani saqlash", size = 28.dp) { state.saveScene() }
            state.onImportRequest?.let { ToolIconButton(Icons.Outlined.FileDownload, "Import qilish", size = 28.dp, onClick = it) }
        }
        // Yo'l (breadcrumb): har bir bo'lakka teginib o'sha papkaga qaytish mumkin
        Row(
            Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()).padding(horizontal = 8.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            val parts = if (state.assetDir.isEmpty()) emptyList() else state.assetDir.split('/')
            Crumb(state.projectName, active = parts.isEmpty()) { state.openAssetDir("") }
            parts.forEachIndexed { i, part ->
                Icon(Icons.Filled.ChevronRight, null, tint = ZaminColors.TextFaint, modifier = Modifier.size(14.dp))
                Crumb(part, active = i == parts.lastIndex) { state.openAssetDir(parts.take(i + 1).joinToString("/")) }
            }
        }
        if (state.assets.isEmpty()) {
            Box(Modifier.fillMaxSize().padding(16.dp), contentAlignment = Alignment.Center) {
                Text(
                    if (state.project == null) "Loyiha ochilmagan" else "Bo'sh papka",
                    color = ZaminColors.TextFaint, fontSize = 12.sp,
                )
            }
        } else {
            LazyVerticalGrid(
                columns = GridCells.Adaptive(84.dp),
                modifier = Modifier.fillMaxSize().padding(horizontal = 8.dp),
                horizontalArrangement = Arrangement.spacedBy(6.dp),
                verticalArrangement = Arrangement.spacedBy(6.dp),
            ) {
                items(state.assets, key = { it.path }) { a ->
                    // Birinchi teginish — tanlash, ikkinchisi — ochish (papka yoki sahna)
                    AssetTile(a, selected == a.path, open = a.path == state.scenePath) {
                        if (selected == a.path || a.kind == AssetKind.Folder) {
                            state.openAsset(a)
                            selected = null
                        } else {
                            selected = a.path
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun Crumb(text: String, active: Boolean, onClick: () -> Unit) {
    Text(
        text,
        color = if (active) ZaminColors.Text else ZaminColors.TextMuted,
        fontSize = 12.sp,
        maxLines = 1,
        modifier = Modifier.clip(RoundedCornerShape(5.dp)).clickable(onClick = onClick).padding(horizontal = 4.dp, vertical = 3.dp),
    )
}

@Composable
private fun AssetTile(asset: AssetItem, selected: Boolean, open: Boolean, onClick: () -> Unit) {
    Column(
        Modifier
            .clip(RoundedCornerShape(8.dp))
            .background(if (selected) ZaminColors.Selection else Color.Transparent)
            .clickable(onClick = onClick)
            .padding(6.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Box(
            Modifier
                .fillMaxWidth()
                .aspectRatio(1.25f)
                .clip(RoundedCornerShape(7.dp))
                .background(ZaminColors.PanelAlt),
            contentAlignment = Alignment.Center,
        ) {
            Icon(asset.kind.icon(), null, tint = asset.kind.tint(), modifier = Modifier.size(28.dp))
            if (open) {
                // Hozir ochiq sahna belgisi
                Box(
                    Modifier.align(Alignment.TopEnd).padding(5.dp).size(7.dp)
                        .clip(RoundedCornerShape(4.dp)).background(ZaminColors.Accent),
                )
            }
        }
        Spacer(Modifier.height(5.dp))
        Text(
            asset.name, color = ZaminColors.Text, fontSize = 11.sp, maxLines = 1,
            overflow = TextOverflow.Ellipsis, textAlign = TextAlign.Center, modifier = Modifier.width(80.dp),
        )
    }
}
