package dev.zamin.editor.ui

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

enum class ObjectType(val label: String) {
    Scene("Sahna"), Camera("Kamera"), Light("Yorug'lik"), Mesh("Mesh"), Sprite("Sprite"), Empty("Bo'sh"),
}

enum class Tool(val label: String) { Hand("Qo'l"), Move("Ko'chirish"), Rotate("Aylantirish"), Scale("Masshtab") }

enum class PlayState { Edit, Playing, Paused }

data class Vec3(val x: Float, val y: Float, val z: Float)

class SceneObject(
    val id: Int,
    name: String,
    val type: ObjectType,
    val parentId: Int?,
    position: Vec3 = Vec3(0f, 0f, 0f),
    rotation: Vec3 = Vec3(0f, 0f, 0f),
    scale: Vec3 = Vec3(1f, 1f, 1f),
) {
    var name by mutableStateOf(name)
    var visible by mutableStateOf(true)
    var expanded by mutableStateOf(true)
    var position by mutableStateOf(position)
    var rotation by mutableStateOf(rotation)
    var scale by mutableStateOf(scale)
}

enum class AssetKind { Folder, Scene, Material, Texture, Mesh, Script, Audio }

data class AssetItem(val name: String, val kind: AssetKind)

/** Editor UI holati. Bosqich 1 da engine sahnasi bilan sinxronlanadi. */
class EditorState {
    var tool by mutableStateOf(Tool.Move)
    var is3D by mutableStateOf(true)
    var playState by mutableStateOf(PlayState.Edit)
    var selectedId by mutableStateOf<Int?>(4)
    var projectName by mutableStateOf("Mening o'yinim")

    val objects = mutableStateListOf(
        SceneObject(1, "Asosiy sahna", ObjectType.Scene, null),
        SceneObject(2, "Asosiy kamera", ObjectType.Camera, 1, position = Vec3(0f, 1.6f, -6f)),
        SceneObject(3, "Quyosh", ObjectType.Light, 1, position = Vec3(0f, 8f, 0f), rotation = Vec3(50f, -30f, 0f)),
        SceneObject(4, "Piramida", ObjectType.Mesh, 1),
        SceneObject(5, "Yer", ObjectType.Mesh, 1, position = Vec3(0f, -0.5f, 0f), scale = Vec3(20f, 1f, 20f)),
        SceneObject(6, "O'yinchi", ObjectType.Empty, 1, position = Vec3(2f, 0f, 1f)),
        SceneObject(7, "Qahramon sprite", ObjectType.Sprite, 6),
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

    fun childrenOf(parentId: Int?): List<SceneObject> = objects.filter { it.parentId == parentId }

    private var nextId = 100

    fun addObject(type: ObjectType) {
        val parent = objects.first { it.type == ObjectType.Scene }.id
        val name = when (type) {
            ObjectType.Mesh -> "Kub"
            ObjectType.Light -> "Nuqtali yorug'lik"
            ObjectType.Camera -> "Kamera"
            ObjectType.Sprite -> "Sprite"
            else -> "Yangi obyekt"
        }
        val obj = SceneObject(nextId++, name, type, parent)
        objects.add(obj)
        selectedId = obj.id
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
