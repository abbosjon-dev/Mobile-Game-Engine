package dev.zamin.editor.ui

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.Color
import dev.zamin.editor.project.AssetItem
import dev.zamin.editor.project.AssetKind
import dev.zamin.editor.project.ProjectStorage

/** Engine'dagi zamin::GizmoKind bilan bir xil tartib: meshsiz obyektlarning viewport belgisi. */
enum class Gizmo { None, Light, Camera, Empty }

enum class ObjectType(val label: String, val gizmo: Gizmo) {
    Scene("Sahna", Gizmo.None), Camera("Kamera", Gizmo.Camera), Light("Yorug'lik", Gizmo.Light),
    Mesh("Mesh", Gizmo.Empty), Sprite("Sprite", Gizmo.Empty), Empty("Bo'sh", Gizmo.Empty),
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
    fun snapshot() = EntitySnapshot(id, parentId ?: 0, mesh, type.gizmo, visible, position, rotation, scale, color)
}

data class EntitySnapshot(
    val id: Int,
    val parent: Int,
    val mesh: MeshKind,
    /** Mesh bo'lmasa (MeshKind.None) viewport'da shu belgi chiziladi. */
    val gizmo: Gizmo,
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

/** Yangi loyihaning standart sahnasi. */
fun defaultSceneObjects(): List<SceneObject> = listOf(
    SceneObject(1, "Asosiy sahna", ObjectType.Scene, null),
    SceneObject(2, "Asosiy kamera", ObjectType.Camera, 1, position = Vec3(0f, 2.2f, 6.5f), rotation = Vec3(12f, 180f, 0f)),
    SceneObject(3, "Quyosh", ObjectType.Light, 1, position = Vec3(-2.6f, 3.2f, -1.2f), rotation = Vec3(50f, 30f, 0f)),
    SceneObject(4, "Piramida", ObjectType.Mesh, 1, MeshKind.Pyramid, Color(0xFF3DB98A), Vec3(0f, 0.5f, 0f)),
    SceneObject(5, "Kub", ObjectType.Mesh, 1, MeshKind.Cube, Color(0xFFF2A33C), Vec3(-1.8f, 0.5f, 0.8f), Vec3(0f, 25f, 0f)),
    SceneObject(6, "Sfera", ObjectType.Mesh, 1, MeshKind.Sphere, Color(0xFF4D8CF2), Vec3(1.7f, 0.5f, -0.6f)),
    SceneObject(7, "Yer", ObjectType.Mesh, 1, MeshKind.Plane, Color(0xFF3F4A45), scale = Vec3(10f, 1f, 10f)),
    SceneObject(8, "O'yinchi", ObjectType.Empty, 1, position = Vec3(1.2f, 0f, 2f)),
    SceneObject(9, "Qahramon sprite", ObjectType.Sprite, 8, MeshKind.Quad, Color(0xFFE8604C), Vec3(0f, 0.75f, 0f), scale = Vec3(1f, 1.5f, 1f)),
)

/**
 * Editor UI holati. Sahna obyektlari engine bilan EngineSceneSync orqali sinxronlanadi.
 * project berilsa — assetlar va sahna fayllari shu loyiha papkasidan o'qiladi/yoziladi.
 */
class EditorState(val project: ProjectStorage? = null) {
    var tool by mutableStateOf(Tool.Move)
    var is3D by mutableStateOf(true)
    var playState by mutableStateOf(PlayState.Edit)
        private set
    var selectedId by mutableStateOf<Int?>(4)
    var projectName by mutableStateOf(project?.name ?: "Mening o'yinim")
    /** Ochiq sahna fayli (loyihaga nisbatan yo'l). */
    var scenePath by mutableStateOf(ProjectStorage.MAIN_SCENE)
        private set
    /** Qisqa xabar (saqlandi, xato va h.k.), UI uni bir necha soniya ko'rsatadi. */
    var message by mutableStateOf<String?>(null)
    /** Har oshganda kamera tanlangan obyektga qaratiladi. */
    var focusRequest by mutableIntStateOf(0)
        private set

    val objects = mutableStateListOf<SceneObject>().apply { addAll(defaultSceneObjects()) }

    // ---- Assetlar ----
    /** Assetlar panelida ochiq papka ("" — loyiha ildizi). */
    var assetDir by mutableStateOf("")
        private set
    var assets by mutableStateOf<List<AssetItem>>(emptyList())
        private set
    /** Platforma fayl tanlagichini ochadi (MainActivity o'rnatadi; testlarda null). */
    var onImportRequest: (() -> Unit)? = null
    /** Telefonda ilova ochilganda ochiq turadigan panel (skrinshot testlari uchun). */
    var initialPhonePanel: PhonePanel? = null

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

    private val editable get() = playState == PlayState.Edit

    fun addObject(kind: CreateKind) {
        val parent = objects.firstOrNull { it.type == ObjectType.Scene }?.id
        // 2D rejimda obyektlar XY tekisligida (z = 0), 3D da yer ustida
        val pos = when {
            kind.mesh == MeshKind.Plane -> Vec3(0f, 0f, 0f)
            kind.type == ObjectType.Light -> Vec3(0f, 4f, 0f)
            kind.type == ObjectType.Camera -> Vec3(0f, 1.6f, 6f)
            else -> Vec3(0f, 0.5f, 0f)
        }
        val obj = SceneObject(
            nextId++, uniqueName(kind.label.substringBefore(" (")), kind.type, parent,
            kind.mesh, kind.color, pos,
            // Kamera +Z ga qaraydi: sahnaga qaratish uchun 180° buriladi
            rotation = if (kind.type == ObjectType.Camera) Vec3(10f, 180f, 0f) else Vec3(0f, 0f, 0f),
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

    // ---- Play rejimi ----
    // O'ynash boshlanganda sahna nusxasi olinadi; to'xtatilganda aynan shu holat qaytariladi,
    // ya'ni o'yin davomidagi o'zgarishlar sahnani buzmaydi (Unity'dagi kabi).
    private var editSnapshot: List<ObjectData>? = null
    private var editSelection: Int? = null

    fun play() {
        if (playState != PlayState.Edit) return
        editSnapshot = objects.map { it.toData() }
        editSelection = selectedId
        playState = PlayState.Playing
    }

    fun stop() {
        if (playState == PlayState.Edit) return
        editSnapshot?.let { saved -> replaceObjects(saved) }
        selectedId = editSelection
        editSnapshot = null
        playState = PlayState.Edit
    }

    fun togglePause() {
        playState = when (playState) {
            PlayState.Playing -> PlayState.Paused
            PlayState.Paused -> PlayState.Playing
            PlayState.Edit -> PlayState.Edit
        }
    }

    private fun replaceObjects(data: List<ObjectData>) {
        objects.clear()
        objects.addAll(data.map { it.toObject() })
        nextId = maxOf(100, (data.maxOfOrNull { it.id } ?: 0) + 1)
    }

    // ---- Loyiha va sahna fayllari ----
    fun sceneData() = SceneData(scenePath.substringAfterLast('/').removeSuffix(".scene"), is3D, objects.map { it.toData() })

    /** Loyihani ochadi: asosiy sahna bo'lsa yuklanadi, bo'lmasa standart sahna yoziladi. */
    fun openProject() {
        val p = project ?: return
        p.ensureLayout()
        if (p.exists(ProjectStorage.MAIN_SCENE)) loadScene(ProjectStorage.MAIN_SCENE) else saveScene()
        refreshAssets()
    }

    fun saveScene(): Boolean {
        val p = project ?: return false
        if (!editable) {
            message = "O'yin rejimida saqlab bo'lmaydi"
            return false
        }
        val ok = p.writeText(scenePath, SceneFile.encode(sceneData()))
        message = if (ok) "Saqlandi: ${scenePath.substringAfterLast('/')}" else "Saqlashda xato"
        refreshAssets()
        return ok
    }

    fun loadScene(path: String): Boolean {
        val p = project ?: return false
        if (!editable) return false
        val text = p.readText(path) ?: run { message = "Fayl topilmadi"; return false }
        val scene = try {
            SceneFile.decode(text)
        } catch (e: IllegalArgumentException) {
            message = "Sahnani o'qib bo'lmadi: ${e.message}"
            return false
        }
        replaceObjects(scene.objects)
        is3D = scene.is3D
        scenePath = path
        selectedId = null
        message = "Ochildi: ${path.substringAfterLast('/')}"
        return true
    }

    fun refreshAssets() {
        assets = project?.list(assetDir) ?: emptyList()
    }

    fun openAssetDir(dir: String) {
        assetDir = dir
        refreshAssets()
    }

    /** Assetga teginish: papka — ichiga kirish, sahna — ochish. */
    fun openAsset(item: AssetItem) {
        when (item.kind) {
            AssetKind.Folder -> openAssetDir(item.path)
            AssetKind.Scene -> loadScene(item.path)
            else -> {}
        }
    }

    fun onImported(path: String?) {
        message = if (path != null) "Import qilindi: ${path.substringAfterLast('/')}" else "Import muvaffaqiyatsiz"
        refreshAssets()
    }
}
