package dev.zamin.editor.ui

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import org.json.JSONArray
import org.json.JSONObject

/** Obyektning o'zgarmas nusxasi: sahna fayli va Play rejimi holatini tiklash uchun. */
data class ObjectData(
    val id: Int,
    val name: String,
    val type: ObjectType,
    val parentId: Int?,
    val mesh: MeshKind,
    val color: Color,
    val position: Vec3,
    val rotation: Vec3,
    val scale: Vec3,
    val visible: Boolean = true,
    val expanded: Boolean = true,
)

fun SceneObject.toData() = ObjectData(id, name, type, parentId, mesh, color, position, rotation, scale, visible, expanded)

fun ObjectData.toObject() = SceneObject(id, name, type, parentId, mesh, color, position, rotation, scale).also {
    it.visible = visible
    it.expanded = expanded
}

data class SceneData(val name: String, val is3D: Boolean, val objects: List<ObjectData>)

/**
 * Sahna fayli (.scene) — o'qiladigan JSON (git'ga qulay).
 * Format o'zgarsa VERSION oshiriladi; noma'lum maydonlar e'tiborsiz qoldiriladi.
 */
object SceneFile {
    const val FORMAT = "zamin.scene"
    const val VERSION = 1

    fun encode(scene: SceneData): String {
        val root = JSONObject()
            .put("format", FORMAT)
            .put("version", VERSION)
            .put("name", scene.name)
            .put("is3D", scene.is3D)
        val arr = JSONArray()
        scene.objects.forEach { o ->
            val j = JSONObject()
                .put("id", o.id)
                .put("name", o.name)
                .put("type", o.type.name)
                .put("mesh", o.mesh.name)
                .put("color", "#%08X".format(o.color.toArgb()))
                .put("position", vec(o.position))
                .put("rotation", vec(o.rotation))
                .put("scale", vec(o.scale))
                .put("visible", o.visible)
                .put("expanded", o.expanded)
            if (o.parentId != null) j.put("parent", o.parentId)
            arr.put(j)
        }
        root.put("objects", arr)
        return root.toString(2)
    }

    /** Buzilgan yoki boshqa formatdagi faylda IllegalArgumentException. */
    fun decode(text: String): SceneData {
        val root = try {
            JSONObject(text)
        } catch (e: Exception) {
            throw IllegalArgumentException("Sahna fayli JSON emas", e)
        }
        require(root.optString("format") == FORMAT) { "Sahna fayli emas" }
        require(root.optInt("version", 0) in 1..VERSION) { "Qo'llab-quvvatlanmaydigan versiya" }
        val arr = root.optJSONArray("objects") ?: JSONArray()
        val objects = (0 until arr.length()).map { i ->
            val j = arr.getJSONObject(i)
            ObjectData(
                id = j.getInt("id"),
                name = j.optString("name", "Obyekt"),
                type = enumOr(j.optString("type"), ObjectType.Empty),
                parentId = if (j.has("parent")) j.getInt("parent") else null,
                mesh = enumOr(j.optString("mesh"), MeshKind.None),
                color = parseColor(j.optString("color")),
                position = vec(j.optJSONArray("position"), Vec3(0f, 0f, 0f)),
                rotation = vec(j.optJSONArray("rotation"), Vec3(0f, 0f, 0f)),
                scale = vec(j.optJSONArray("scale"), Vec3(1f, 1f, 1f)),
                visible = j.optBoolean("visible", true),
                expanded = j.optBoolean("expanded", true),
            )
        }
        return SceneData(root.optString("name", "Sahna"), root.optBoolean("is3D", true), objects)
    }

    private fun vec(v: Vec3) = JSONArray().put(v.x.toDouble()).put(v.y.toDouble()).put(v.z.toDouble())

    private fun vec(a: JSONArray?, def: Vec3): Vec3 {
        if (a == null || a.length() < 3) return def
        return Vec3(a.optDouble(0, 0.0).toFloat(), a.optDouble(1, 0.0).toFloat(), a.optDouble(2, 0.0).toFloat())
    }

    private fun parseColor(s: String): Color {
        val hex = s.removePrefix("#")
        val v = hex.toLongOrNull(16) ?: return Color.White
        return if (hex.length <= 6) Color(0xFF000000 or v) else Color(v)
    }

    private inline fun <reified E : Enum<E>> enumOr(name: String, def: E): E =
        enumValues<E>().firstOrNull { it.name == name } ?: def
}
