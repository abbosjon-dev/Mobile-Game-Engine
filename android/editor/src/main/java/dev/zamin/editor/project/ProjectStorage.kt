package dev.zamin.editor.project

import java.io.File
import java.io.InputStream

/** Asset turi fayl kengaytmasidan aniqlanadi. */
enum class AssetKind { Folder, Scene, Material, Texture, Mesh, Script, Audio, Other }

/** Loyiha ichidagi fayl yoki papka. path — loyiha ildiziga nisbatan ("Sahnalar/Asosiy.scene"). */
data class AssetItem(val name: String, val kind: AssetKind, val path: String)

fun assetKindOf(name: String): AssetKind = when (name.substringAfterLast('.', "").lowercase()) {
    "scene" -> AssetKind.Scene
    "mat" -> AssetKind.Material
    "png", "jpg", "jpeg", "webp", "ktx2" -> AssetKind.Texture
    "glb", "gltf", "obj", "fbx" -> AssetKind.Mesh
    "luau", "lua" -> AssetKind.Script
    "ogg", "wav", "mp3" -> AssetKind.Audio
    else -> AssetKind.Other
}

/**
 * Loyiha fayllari ilovaning ichki xotirasida saqlanadi (scoped storage muammosi yo'q).
 * Tashqi fayllar SAF orqali import qilinib shu yerga nusxalanadi.
 */
class ProjectStorage(val root: File) {
    val name: String get() = root.name

    fun ensureLayout() {
        DEFAULT_FOLDERS.forEach { File(root, it).mkdirs() }
    }

    /** Papka ichidagi elementlar: avval papkalar, keyin fayllar (alifbo bo'yicha). */
    fun list(dir: String = ""): List<AssetItem> {
        val base = resolve(dir) ?: return emptyList()
        val files = base.listFiles()?.filterNot { it.name.startsWith(".") } ?: return emptyList()
        return files
            .sortedWith(compareBy<File>({ !it.isDirectory }, { it.name.lowercase() }))
            .map { f ->
                val rel = if (dir.isEmpty()) f.name else "$dir/${f.name}"
                AssetItem(f.name, if (f.isDirectory) AssetKind.Folder else assetKindOf(f.name), rel)
            }
    }

    fun readText(path: String): String? = resolve(path)?.takeIf { it.isFile }?.readText()

    fun writeText(path: String, text: String): Boolean {
        val f = resolve(path) ?: return false
        f.parentFile?.mkdirs()
        // Avval vaqtinchalik faylga yozamiz — yozish uzilsa eski sahna buzilmaydi
        val tmp = File(f.parentFile, ".${f.name}.tmp")
        tmp.writeText(text)
        return tmp.renameTo(f) || run { f.writeText(text); tmp.delete(); true }
    }

    fun exists(path: String): Boolean = resolve(path)?.exists() == true

    /** Tashqi oqimni papkaga nusxalaydi; nom band bo'lsa "(1)" qo'shiladi. Natija — yangi yo'l. */
    fun import(dir: String, fileName: String, input: InputStream): String? {
        val base = resolve(dir) ?: return null
        base.mkdirs()
        val safe = fileName.replace(Regex("[/\\\\:*?\"<>|]"), "_").ifBlank { "fayl" }
        var target = File(base, safe)
        var i = 1
        while (target.exists()) {
            val stem = safe.substringBeforeLast('.')
            val ext = safe.substringAfterLast('.', "")
            target = File(base, if (ext.isEmpty()) "$stem ($i)" else "$stem ($i).$ext")
            i++
        }
        target.outputStream().use { input.copyTo(it) }
        return if (dir.isEmpty()) target.name else "$dir/${target.name}"
    }

    /** Loyiha ildizidan tashqariga chiqadigan yo'llarni ("../") rad etadi. */
    private fun resolve(path: String): File? {
        val f = if (path.isEmpty()) root else File(root, path)
        val rootPath = root.canonicalPath
        val p = f.canonicalPath
        return if (p == rootPath || p.startsWith(rootPath + File.separator)) f else null
    }

    companion object {
        const val SCENES = "Sahnalar"
        val DEFAULT_FOLDERS = listOf(SCENES, "Materiallar", "Skriptlar", "Teksturalar", "Audio")
        const val MAIN_SCENE = "$SCENES/Asosiy.scene"
    }
}
