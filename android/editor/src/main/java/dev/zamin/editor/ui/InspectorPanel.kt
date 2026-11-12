package dev.zamin.editor.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.ChevronRight
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.outlined.CenterFocusStrong
import androidx.compose.material.icons.outlined.OpenWith
import androidx.compose.material.icons.outlined.Palette
import androidx.compose.material.icons.outlined.Tune
import androidx.compose.material.icons.outlined.ViewInAr
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
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@Composable
fun InspectorPanel(state: EditorState, modifier: Modifier = Modifier) {
    Column(modifier.background(ZaminColors.Panel)) {
        PanelHeader("Inspektor", Icons.Outlined.Tune)
        val obj = state.selected
        if (obj == null) {
            Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                Text("Obyekt tanlang", color = ZaminColors.TextFaint, fontSize = 12.sp)
            }
            return@Column
        }
        Column(Modifier.fillMaxSize().verticalScroll(rememberScrollState())) {
            ObjectHeader(obj, onFocus = { state.requestFocus() })
            ComponentSection("Transform", Icons.Outlined.OpenWith) {
                Vec3Row("Joylashuv", obj.position) { obj.position = it }
                Vec3Row("Aylanish", obj.rotation) { obj.rotation = it }
                Vec3Row("Masshtab", obj.scale) { obj.scale = it }
            }
            when (obj.type) {
                ObjectType.Mesh -> ComponentSection("Mesh Renderer", Icons.Outlined.ViewInAr) {
                    PropertyRow("Mesh") {
                        Dropdown(obj.mesh.label, Icons.Outlined.ViewInAr,
                            listOf(MeshKind.Cube, MeshKind.Sphere, MeshKind.Pyramid, MeshKind.Plane).map { it.label to it },
                        ) { obj.mesh = it }
                    }
                    ColorProperty("Rang", obj.color) { obj.color = it }
                }
                ObjectType.Sprite -> ComponentSection("Sprite Renderer", Icons.Outlined.Palette) {
                    PropertyRow("Sprite") { AssetField("Yo'q (rang)", Icons.Outlined.Palette) }
                    ColorProperty("Rang", obj.color) { obj.color = it }
                }
                ObjectType.Light -> ComponentSection("Yorug'lik", Icons.Outlined.Tune) {
                    PropertyRow("Turi") { AssetField("Yo'naltirilgan", null) }
                    Hint("Yorug'lik sozlamalari keyingi bosqichda engine'ga ulanadi")
                }
                ObjectType.Camera -> ComponentSection("Kamera", Icons.Outlined.Tune) {
                    PropertyRow("Proyeksiya") { AssetField("Perspektiv", null) }
                    Hint("O'yin kamerasi Play rejimi bilan birga ishlaydi")
                }
                else -> {}
            }
            AddComponentButton()
            Spacer(Modifier.height(24.dp))
        }
    }
}

@Composable
private fun ObjectHeader(obj: SceneObject, onFocus: () -> Unit) {
    Row(
        Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(
            Modifier.size(32.dp).clip(RoundedCornerShape(8.dp)).background(obj.type.tint().copy(alpha = 0.15f)),
            contentAlignment = Alignment.Center,
        ) { Icon(obj.type.icon(), null, tint = obj.type.tint(), modifier = Modifier.size(18.dp)) }
        Spacer(Modifier.width(10.dp))
        Column(Modifier.weight(1f)) {
            BasicTextField(
                obj.name, { obj.name = it }, singleLine = true,
                textStyle = TextStyle(color = ZaminColors.Text, fontSize = 15.sp, fontWeight = FontWeight.SemiBold),
                cursorBrush = SolidColor(ZaminColors.Accent),
            )
            Text(obj.type.label, color = ZaminColors.TextFaint, fontSize = 11.sp)
        }
        if (obj.type != ObjectType.Scene) {
            ToolIconButton(Icons.Outlined.CenterFocusStrong, "Kamerani qaratish", size = 32.dp, onClick = onFocus)
            Spacer(Modifier.width(6.dp))
            Toggle(obj.visible) { obj.visible = it }
        }
    }
    Box(Modifier.fillMaxWidth().height(1.dp).background(ZaminColors.Divider))
}

@Composable
private fun ComponentSection(title: String, icon: ImageVector, content: @Composable () -> Unit) {
    var open by remember { mutableStateOf(true) }
    Column {
        Row(
            Modifier
                .fillMaxWidth()
                .height(36.dp)
                .background(ZaminColors.PanelAlt)
                .clickable { open = !open }
                .padding(horizontal = 8.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Icon(if (open) Icons.Filled.ExpandMore else Icons.Filled.ChevronRight, null, tint = ZaminColors.TextFaint, modifier = Modifier.size(16.dp))
            Spacer(Modifier.width(4.dp))
            Icon(icon, null, tint = ZaminColors.Accent, modifier = Modifier.size(15.dp))
            Spacer(Modifier.width(8.dp))
            Text(title, color = ZaminColors.Text, fontSize = 13.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.weight(1f))
            Icon(Icons.Filled.MoreVert, null, tint = ZaminColors.TextFaint, modifier = Modifier.size(16.dp))
        }
        if (open) {
            Column(Modifier.padding(horizontal = 12.dp, vertical = 8.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                content()
            }
        }
        Box(Modifier.fillMaxWidth().height(1.dp).background(ZaminColors.Divider))
    }
}

@Composable
private fun PropertyRow(label: String, content: @Composable () -> Unit) {
    Row(Modifier.fillMaxWidth().height(30.dp), verticalAlignment = Alignment.CenterVertically) {
        Text(label, color = ZaminColors.TextMuted, fontSize = 12.sp, modifier = Modifier.width(96.dp), maxLines = 1)
        Box(Modifier.weight(1f)) { content() }
    }
}

@Composable
private fun Vec3Row(label: String, value: Vec3, onChange: (Vec3) -> Unit) {
    PropertyRow(label) {
        Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
            NumberField(value.x, ZaminColors.AxisX, Modifier.weight(1f)) { onChange(value.copy(x = it)) }
            NumberField(value.y, ZaminColors.AxisY, Modifier.weight(1f)) { onChange(value.copy(y = it)) }
            NumberField(value.z, ZaminColors.AxisZ, Modifier.weight(1f)) { onChange(value.copy(z = it)) }
        }
    }
}

private fun fmt(v: Float): String =
    if (v == v.toLong().toFloat()) v.toLong().toString() else "%.2f".format(v).trimEnd('0').trimEnd('.')

@Composable
private fun NumberField(value: Float, axis: Color?, modifier: Modifier = Modifier, onChange: (Float) -> Unit) {
    var text by remember(value) { mutableStateOf(fmt(value)) }
    Row(
        modifier
            .height(28.dp)
            .clip(RoundedCornerShape(6.dp))
            .background(ZaminColors.Field),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        if (axis != null) Box(Modifier.width(3.dp).fillMaxHeight().background(axis))
        BasicTextField(
            text,
            { t -> text = t; t.replace(',', '.').toFloatOrNull()?.let(onChange) },
            singleLine = true,
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal),
            textStyle = TextStyle(color = ZaminColors.Text, fontSize = 12.sp, fontFamily = FontFamily.Monospace),
            cursorBrush = SolidColor(ZaminColors.Accent),
            modifier = Modifier.padding(horizontal = 6.dp).fillMaxWidth(),
        )
    }
}

@Composable
private fun Hint(text: String) {
    Text(text, color = ZaminColors.TextFaint, fontSize = 11.sp, modifier = Modifier.padding(vertical = 2.dp))
}

@Composable
private fun <T> Dropdown(current: String, icon: ImageVector?, options: List<Pair<String, T>>, onSelect: (T) -> Unit) {
    var open by remember { mutableStateOf(false) }
    Box {
        AssetField(current, icon) { open = true }
        DropdownMenu(expanded = open, onDismissRequest = { open = false }) {
            options.forEach { (label, value) ->
                DropdownMenuItem(text = { Text(label, fontSize = 13.sp) }, onClick = { onSelect(value); open = false })
            }
        }
    }
}

@Composable
private fun ColorProperty(label: String, color: Color, onChange: (Color) -> Unit) {
    var open by remember { mutableStateOf(false) }
    PropertyRow(label) { ColorSwatch(color) { open = !open } }
    if (open) {
        // Tayyor palitra: 2 qator x 6 rang
        Column(verticalArrangement = Arrangement.spacedBy(6.dp), modifier = Modifier.padding(start = 96.dp)) {
            ColorPalette.chunked(6).forEach { row ->
                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    row.forEach { c ->
                        Box(
                            Modifier
                                .size(26.dp)
                                .clip(RoundedCornerShape(6.dp))
                                .background(c)
                                .border(
                                    if (c == color) 2.dp else 1.dp,
                                    if (c == color) ZaminColors.Text else ZaminColors.Border,
                                    RoundedCornerShape(6.dp),
                                )
                                .clickable { onChange(c) },
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun AssetField(name: String, icon: ImageVector?, onClick: () -> Unit = {}) {
    Row(
        Modifier.fillMaxWidth().height(28.dp).clip(RoundedCornerShape(6.dp)).background(ZaminColors.Field)
            .clickable(onClick = onClick).padding(horizontal = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        if (icon != null) {
            Icon(icon, null, tint = ZaminColors.TextMuted, modifier = Modifier.size(14.dp))
            Spacer(Modifier.width(6.dp))
        }
        Text(name, color = ZaminColors.Text, fontSize = 12.sp, maxLines = 1, modifier = Modifier.weight(1f))
        Icon(Icons.Filled.ExpandMore, null, tint = ZaminColors.TextFaint, modifier = Modifier.size(14.dp))
    }
}

@Composable
private fun ColorSwatch(color: Color, onClick: () -> Unit) {
    Row(Modifier.clip(RoundedCornerShape(5.dp)).clickable(onClick = onClick), verticalAlignment = Alignment.CenterVertically) {
        Box(Modifier.size(width = 44.dp, height = 22.dp).clip(RoundedCornerShape(5.dp)).background(color).border(1.dp, ZaminColors.Border, RoundedCornerShape(5.dp)))
        Spacer(Modifier.width(8.dp))
        val hex = "#%06X".format(0xFFFFFF and (color.value shr 32).toInt())
        Text(hex, color = ZaminColors.TextMuted, fontSize = 11.sp, fontFamily = FontFamily.Monospace)
    }
}

@Composable
fun Toggle(initial: Boolean, onChange: (Boolean) -> Unit = {}) {
    var on by remember(initial) { mutableStateOf(initial) }
    Box(
        Modifier
            .size(width = 34.dp, height = 20.dp)
            .clip(RoundedCornerShape(10.dp))
            .background(if (on) ZaminColors.Accent else ZaminColors.FieldHover)
            .clickable { on = !on; onChange(on) }
            .padding(3.dp),
        contentAlignment = if (on) Alignment.CenterEnd else Alignment.CenterStart,
    ) {
        Box(Modifier.size(14.dp).clip(RoundedCornerShape(7.dp)).background(Color.White))
    }
}

@Composable
private fun AddComponentButton() {
    Row(
        Modifier
            .padding(12.dp)
            .fillMaxWidth()
            .height(36.dp)
            .clip(RoundedCornerShape(8.dp))
            .border(1.dp, ZaminColors.Border, RoundedCornerShape(8.dp))
            .clickable { },
        horizontalArrangement = Arrangement.Center,
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Icon(Icons.Filled.Add, null, tint = ZaminColors.Accent, modifier = Modifier.size(16.dp))
        Spacer(Modifier.width(6.dp))
        Text("Komponent qo'shish", color = ZaminColors.Text, fontSize = 13.sp)
    }
}
