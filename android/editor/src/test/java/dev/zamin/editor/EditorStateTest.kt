package dev.zamin.editor

import androidx.compose.ui.graphics.Color
import dev.zamin.editor.project.AssetKind
import dev.zamin.editor.project.ProjectStorage
import dev.zamin.editor.ui.EditorState
import dev.zamin.editor.ui.MeshKind
import dev.zamin.editor.ui.ObjectData
import dev.zamin.editor.ui.ObjectType
import dev.zamin.editor.ui.PlayState
import dev.zamin.editor.ui.SceneData
import dev.zamin.editor.ui.SceneFile
import dev.zamin.editor.ui.Vec3
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

class EditorStateTest {
    @get:Rule
    val tmp = TemporaryFolder()

    private fun project() = ProjectStorage(tmp.newFolder("loyiha"))

    @Test
    fun sceneFileRoundTrip() {
        val scene = SceneData(
            "Test", is3D = false,
            listOf(
                ObjectData(1, "Ildiz", ObjectType.Scene, null, MeshKind.None, Color.White, Vec3(0f, 0f, 0f), Vec3(0f, 0f, 0f), Vec3(1f, 1f, 1f)),
                ObjectData(
                    7, "Qizil kub", ObjectType.Mesh, 1, MeshKind.Cube, Color(0x80E8604C),
                    Vec3(1.5f, -2f, 3.25f), Vec3(10f, 20f, 30f), Vec3(2f, 1f, 0.5f), visible = false, expanded = false,
                ),
            ),
        )
        val decoded = SceneFile.decode(SceneFile.encode(scene))
        assertEquals(scene, decoded)
    }

    @Test(expected = IllegalArgumentException::class)
    fun sceneFileRejectsForeignJson() {
        SceneFile.decode("""{"format":"boshqa","objects":[]}""")
    }

    @Test
    fun playRestoresSceneOnStop() {
        val state = EditorState()
        val before = state.objects.map { it.position }
        state.play()
        assertEquals(PlayState.Playing, state.playState)
        // O'yin davomida obyekt siljiydi va yangisi qo'shiladi
        state.objects.first { it.id == 5 }.position = Vec3(9f, 9f, 9f)
        state.objects.removeAll { it.id == 6 }
        state.togglePause()
        assertEquals(PlayState.Paused, state.playState)
        state.stop()
        assertEquals(PlayState.Edit, state.playState)
        assertEquals(before, state.objects.map { it.position })
        assertTrue(state.objects.any { it.id == 6 })
    }

    @Test
    fun saveAndLoadThroughProject() {
        val p = project()
        val state = EditorState(p)
        state.openProject()
        assertTrue(p.exists(ProjectStorage.MAIN_SCENE))
        // O'zgartirib saqlaymiz, yangi state'da ochamiz
        state.objects.first { it.id == 4 }.color = Color(0xFF123456)
        state.is3D = false
        assertTrue(state.saveScene())

        val other = EditorState(p)
        other.openProject()
        assertEquals(Color(0xFF123456), other.find(4)!!.color)
        assertFalse(other.is3D)
        // Yangi obyekt id lari yuklangan id lar bilan to'qnashmaydi
        other.addObject(dev.zamin.editor.ui.CreateKind.Cube)
        assertEquals(other.objects.size, other.objects.map { it.id }.toSet().size)
    }

    @Test
    fun cannotSaveWhilePlaying() {
        val state = EditorState(project())
        state.openProject()
        state.play()
        assertFalse(state.saveScene())
    }

    @Test
    fun assetsListRealFilesAndNavigate() {
        val p = project()
        val state = EditorState(p)
        state.openProject()
        assertEquals(ProjectStorage.DEFAULT_FOLDERS.sortedBy { it.lowercase() }, state.assets.map { it.name })
        assertTrue(state.assets.all { it.kind == AssetKind.Folder })

        state.openAsset(state.assets.first { it.name == ProjectStorage.SCENES })
        assertEquals(ProjectStorage.SCENES, state.assetDir)
        assertEquals(listOf("Asosiy.scene"), state.assets.map { it.name })
        assertEquals(AssetKind.Scene, state.assets.single().kind)
    }

    @Test
    fun importAvoidsOverwriteAndEscapes() {
        val p = project()
        p.ensureLayout()
        val a = p.import("Teksturalar", "Maysa.png", "a".byteInputStream())
        val b = p.import("Teksturalar", "Maysa.png", "b".byteInputStream())
        assertEquals("Teksturalar/Maysa.png", a)
        assertEquals("Teksturalar/Maysa (1).png", b)
        // Loyihadan tashqariga chiqish taqiqlangan
        assertNull(p.import("../tashqari", "x.txt", "x".byteInputStream()))
        assertNull(p.readText("../../etc/passwd"))
    }
}
