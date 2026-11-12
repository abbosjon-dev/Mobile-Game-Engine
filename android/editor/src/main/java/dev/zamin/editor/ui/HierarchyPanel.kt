package dev.zamin.editor.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.ChevronRight
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.outlined.AccountTree
import androidx.compose.material.icons.outlined.DeleteOutline
import androidx.compose.material.icons.outlined.Search
import androidx.compose.material.icons.outlined.Visibility
import androidx.compose.material.icons.outlined.VisibilityOff
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
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
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

private data class TreeRow(val obj: SceneObject, val depth: Int, val hasChildren: Boolean)

private fun flatten(state: EditorState, query: String): List<TreeRow> {
    val out = mutableListOf<TreeRow>()
    fun walk(parent: Int?, depth: Int) {
        for (o in state.childrenOf(parent)) {
            val kids = state.childrenOf(o.id).isNotEmpty()
            if (query.isEmpty() || o.name.contains(query, ignoreCase = true)) out += TreeRow(o, depth, kids)
            if (o.expanded || query.isNotEmpty()) walk(o.id, depth + 1)
        }
    }
    walk(null, 0)
    return out
}

@Composable
fun HierarchyPanel(state: EditorState, modifier: Modifier = Modifier) {
    var query by remember { mutableStateOf("") }
    var addMenu by remember { mutableStateOf(false) }

    Column(modifier.background(ZaminColors.Panel)) {
        PanelHeader("Ierarxiya", Icons.Outlined.AccountTree) {
            Box {
                ToolIconButton(Icons.Filled.Add, "Obyekt qo'shish", size = 28.dp) { addMenu = true }
                DropdownMenu(expanded = addMenu, onDismissRequest = { addMenu = false }) {
                    listOf(ObjectType.Mesh, ObjectType.Sprite, ObjectType.Light, ObjectType.Camera, ObjectType.Empty).forEach { t ->
                        DropdownMenuItem(
                            text = { Text(t.label, fontSize = 13.sp) },
                            leadingIcon = { Icon(t.icon(), null, tint = t.tint(), modifier = Modifier.size(18.dp)) },
                            onClick = { state.addObject(t); addMenu = false },
                        )
                    }
                }
            }
            ToolIconButton(Icons.Outlined.DeleteOutline, "O'chirish", size = 28.dp) { state.deleteSelected() }
        }
        SearchField(query, { query = it }, Modifier.padding(8.dp))
        LazyColumn(Modifier.fillMaxSize()) {
            items(flatten(state, query), key = { it.obj.id }) { row ->
                HierarchyRow(state, row)
            }
        }
    }
}

@Composable
fun SearchField(value: String, onChange: (String) -> Unit, modifier: Modifier = Modifier) {
    Row(
        modifier
            .fillMaxWidth()
            .height(32.dp)
            .clip(RoundedCornerShape(8.dp))
            .background(ZaminColors.Field)
            .padding(horizontal = 9.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Icon(Icons.Outlined.Search, null, tint = ZaminColors.TextFaint, modifier = Modifier.size(15.dp))
        Spacer(Modifier.width(7.dp))
        Box(Modifier.weight(1f)) {
            if (value.isEmpty()) Text("Qidirish…", color = ZaminColors.TextFaint, fontSize = 12.sp)
            BasicTextField(
                value, onChange, singleLine = true,
                textStyle = TextStyle(color = ZaminColors.Text, fontSize = 12.sp),
                cursorBrush = SolidColor(ZaminColors.Accent),
                modifier = Modifier.fillMaxWidth(),
            )
        }
    }
}

@Composable
private fun HierarchyRow(state: EditorState, row: TreeRow) {
    val o = row.obj
    val selected = state.selectedId == o.id
    Row(
        Modifier
            .fillMaxWidth()
            .height(ZaminDims.RowHeight)
            .padding(horizontal = 4.dp)
            .clip(RoundedCornerShape(6.dp))
            .background(if (selected) ZaminColors.Selection else Color.Transparent)
            .clickable { state.selectedId = o.id }
            .padding(start = (row.depth * 14).dp, end = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(Modifier.size(20.dp).clickable(enabled = row.hasChildren) { o.expanded = !o.expanded }, contentAlignment = Alignment.Center) {
            if (row.hasChildren) {
                Icon(
                    if (o.expanded) Icons.Filled.ExpandMore else Icons.Filled.ChevronRight, null,
                    tint = ZaminColors.TextFaint, modifier = Modifier.size(16.dp),
                )
            }
        }
        Icon(o.type.icon(), null, tint = o.type.tint(), modifier = Modifier.size(16.dp))
        Spacer(Modifier.width(8.dp))
        Text(
            o.name,
            color = if (o.visible) ZaminColors.Text else ZaminColors.TextFaint,
            fontSize = ZaminDims.TextSize,
            fontWeight = if (o.type == ObjectType.Scene) FontWeight.SemiBold else FontWeight.Normal,
            maxLines = 1,
            modifier = Modifier.weight(1f),
        )
        if (o.type != ObjectType.Scene) {
            Box(Modifier.size(26.dp).clip(RoundedCornerShape(6.dp)).clickable { o.visible = !o.visible }, contentAlignment = Alignment.Center) {
                Icon(
                    if (o.visible) Icons.Outlined.Visibility else Icons.Outlined.VisibilityOff,
                    "Ko'rinish",
                    tint = if (o.visible) ZaminColors.TextFaint else ZaminColors.TextMuted.copy(alpha = 0.5f),
                    modifier = Modifier.size(15.dp),
                )
            }
        }
    }
}
