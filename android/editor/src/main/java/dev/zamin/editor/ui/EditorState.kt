package dev.zamin.editor.ui

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.Color

enum class ObjectType(val label: String) {
    Scene("Sahna"), Camera("Kamera"), Light("Yorug'lik"), Mesh("Mesh"), Sprite("Sprite"), Empty("Bo'sh"),
}

/** Engine'dagi zamin::MeshKind bilan bir xil tartib (ordinal JNI orqali yuboriladi). */
enum class MeshKind(val label: String) {
    None("—"), Cube("Kub"), Sphere("Sfera"), Pyramid("Piramida"), Plane("Tekislik"), Quad("Kvadrat"),
}

enum class Tool(val label: String) { Hand("Qo'l"), Move("Ko'chirish"), Rotate("Aylantirish"), Scale("Masshtab") }

enum class PlayState { Edit, Playing, Paused }

data class Vec3(val x: Float, val y: Float, val z: Float) {
    operator fun times(s: Float) = Vec3(x * s, y * s, z * s)
}

/** Yangi obyekt yaratish shablonlari ("+" menyusi). */
enum class CreateKind(val label: String, val type: ObjectType, val mesh: MeshKind, val color: Color) {
    Cube("Kub", ObjectType.Mesh, MeshKind.Cube, Color(0xFFF2A33C)),
    Sphere("Sfera", ObjectType.Mesh, MeshKind.Sphere, Color(0xFF4D8CF2)),
    Pyramid("Piramida", ObjectType.Mesh, MeshKind.Pyramid, Color(0xFF3DB98A)),
    Plane("Tekislik", ObjectType.Mesh, MeshKind.Plane, Color(0xFF7D8A84)),
    Sprite("Sprite (2D)", ObjectType.Sprite, MeshKind.Quad, Color(0xFFE8604C)),
    Light("Yorug'lik", ObjectType.Light, MeshKind.None, Color.White),
    Camera("Kamera", ObjectType.Camera, MeshKind.None, Color.White),
    Empty("Bo'sh obyekt", ObjectType.Empty, MeshKind.None, Color.White),
}

/** Inspektordagi tayyor ranglar palitrasi. */
val ColorPalette = listOf(
    Color(0xFFF2F2F2), Color(0xFF7D8A84), Color(0xFF3F4A45), Color(0xFFE8604C), Color(0xFFF2A33C),
    Color(0xFFF2D04C), Color(0xFF3DB98A), Color(0xFF4DC9D9), Color(0xFF4D8CF2), Color(0xFF9B6CF0),
    Color(0xFFE87DB0), Color(0xFF8A5A3C),
)

class SceneObject(
    val id: Int,
    name: String,
    val type: ObjectType,
    val parentId: Int?,
    mesh: MeshKind = MeshKind.None,
    color: Color = Color.White,
    position: Vec3 = Vec3(0f, 0f, 0f),
    rotation: Vec3 = Vec3(0f, 0f, 0f),
    scale: Vec3 = Vec3(1f, 1f, 1f),
) {
    var name by mutableStateOf(name)
    var visible by mutableStateOf(true)
    var expanded by mutableStateOf(true)
    var mesh by mutableStateOf(mesh)
    var color by mutableStateOf(color)
    var position by mutableStateOf(position)
    var rotation by mutableStateOf(rotation)
    var scale by mutableStateOf(scale)

    /** Engine'ga yuboriladigan holat (o'zgarganini aniqlash uchun). */
    fun snapshot() = EntitySnapshot(id, parentId ?: 0, mesh, visible, position, rotation, scale, color)
}

data class EntitySnapshot(
    val id: Int,
    val parent: Int,
    val mesh: MeshKind,
    val visible: Boolean,
    val position: Vec3,
    val rotation: Vec3,
    val scale: Vec3,
    val color: Color,
) {
    fun data() = floatArrayOf(
        position.x, position.y, position.z,
        rotation.x, rotation.y, rotation.z,
        scale.x, scale.y, scale.z,
        color.red, color.green, color.blue, color.alpha,
    )
}

enum class AssetKind { Folder, Scene, Material, Texture, Mesh, Script, Audio }

data class AssetItem(val name: String, val kind: AssetKind)

/** Editor UI holati. Sahna obyektlari engine bilan EngineSceneSync orqali sinxronlanadi. */
class EditorState {
    var tool by mutableStateOf(Tool.Move)
    var is3D by mutableStateOf(true)
    var playState by mutableStateOf(PlayState.Edit)
    var selectedId by mutableStateOf<Int?>(4)
    var projectName by mutableStateOf("Mening o'yinim")
    /** Har oshganda kamera tanlangan obyektga qaratiladi. */
    var focusRequest by mutableIntStateOf(0)
        private set

    val objects = mutableStateListOf(
        SceneObject(1, "Asosiy sahna", ObjectType.Scene, null),
        SceneObject(2, "Asosiy kamera", ObjectType.Camera, 1, position = Vec3(0f, 1.6f, -6f)),
        SceneObject(3, "Quyosh", ObjectType.Light, 1, position = Vec3(0f, 8f, 0f), rotation = Vec3(50f, -30f, 0f)),
        SceneObject(4, "Piramida", ObjectType.Mesh, 1, MeshKind.Pyramid, Color(0xFF3DB98A), Vec3(0f, 0.5f, 0f)),
        SceneObject(5, "Kub", ObjectType.Mesh, 1, MeshKind.Cube, Color(0xFFF2A33C), Vec3(-1.8f, 0.5f, 0.8f), Vec3(0f, 25f, 0f)),
        SceneObject(6, "Sfera", ObjectType.Mesh, 1, MeshKind.Sphere, Color(0xFF4D8CF2), Vec3(1.7f, 0.5f, -0.6f)),
        SceneObject(7, "Yer", ObjectType.Mesh, 1, MeshKind.Plane, Color(0xFF3F4A45), scale = Vec3(10f, 1f, 10f)),
        SceneObject(8, "O'yinchi", ObjectType.Empty, 1, position = Vec3(1.2f, 0f, 2f)),
        SceneObject(9, "Qahramon sprite", ObjectType.Sprite, 8, MeshKind.Quad, Color(0xFFE8604C), Vec3(0f, 0.75f, 0f), scale = Vec3(1f, 1.5f, 1f)),
    )

    val assets = listOf(
        AssetItem("Sahnalar", AssetKind.Folder),
        AssetItem("Materiallar", AssetKind.Folder),
        AssetItem("Skriptlar", AssetKind.Folder),
        AssetItem("Asosiy.scene", AssetKind.Scene),
        AssetItem("Tosh.mat", AssetKind.Material),
        AssetItem("Maysa.png", AssetKind.Texture),
        AssetItem("Daraxt.glb", AssetKind.Mesh),
        AssetItem("Oyinchi.luau", AssetKind.Script),
        AssetItem("Fon.ogg", AssetKind.Audio),
    )

    val selected: SceneObject? get() = objects.firstOrNull { it.id == selectedId }

    fun find(id: Int): SceneObject? = objects.firstOrNull { it.id == id }

    fun childrenOf(parentId: Int?): List<SceneObject> = objects.filter { it.parentId == parentId }

    fun requestFocus() {
        if (selectedId != null) focusRequest++
    }

    private var nextId = 100

    private fun uniqueName(base: String): String {
        if (objects.none { it.name == base }) return base
        var i = 1
        while (objects.any { it.name == "$base ($i)" }) i++
        return "$base ($i)"
    }

    fun addObject(kind: CreateKind) {
        val parent = objects.first { it.type == ObjectType.Scene }.id
        // 2D rejimda obyektlar XY tekisligida (z = 0), 3D da yer ustida
        val pos = when {
            kind.mesh == MeshKind.Plane -> Vec3(0f, 0f, 0f)
            kind.type == ObjectType.Light -> Vec3(0f, 4f, 0f)
            kind.type == ObjectType.Camera -> Vec3(0f, 1.6f, -6f)
            else -> Vec3(0f, 0.5f, 0f)
        }
        val obj = SceneObject(
            nextId++, uniqueName(kind.label.substringBefore(" (")), kind.type, parent,
            kind.mesh, kind.color, pos,
            scale = if (kind.mesh == MeshKind.Plane) Vec3(4f, 1f, 4f) else Vec3(1f, 1f, 1f),
        )
        objects.add(obj)
        selectedId = obj.id
    }

    fun duplicateSelected() {
        val src = selected ?: return
        if (src.type == ObjectType.Scene) return
        val copy = SceneObject(
            nextId++, uniqueName(src.name), src.type, src.parentId, src.mesh, src.color,
            src.position.copy(x = src.position.x + 1f), src.rotation, src.scale,
        )
        objects.add(copy)
        selectedId = copy.id
    }

    fun deleteSelected() {
        val sel = selected ?: return
        if (sel.type == ObjectType.Scene) return
        val toRemove = mutableSetOf(sel.id)
        var changed = true
        while (changed) {
            changed = objects.any { it.parentId in toRemove && toRemove.add(it.id) }
        }
        objects.removeAll { it.id in toRemove }
        selectedId = null
    }
}
