// Kotlin (dev.zamin.editor.ZaminNative) <-> C++ engine ko'prigi
#include <jni.h>
#include <android/native_window_jni.h>

#include <memory>
#include <mutex>

#include "zamin/core/Engine.h"
#include "zamin/core/Log.h"

namespace {

struct NativeState {
    std::unique_ptr<zamin::Engine> engine;
    ANativeWindow* window = nullptr;
};

NativeState* fromHandle(jlong handle) { return reinterpret_cast<NativeState*>(handle); }

} // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_dev_zamin_editor_ZaminNative_nativeCreate(JNIEnv*, jclass) {
    auto* state = new NativeState();
    state->engine = std::make_unique<zamin::Engine>();
    state->engine->start();
    return reinterpret_cast<jlong>(state);
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeDestroy(JNIEnv*, jclass, jlong handle) {
    auto* state = fromHandle(handle);
    if (!state) return;
    state->engine->setWindow(nullptr);
    state->engine->stop();
    if (state->window) ANativeWindow_release(state->window);
    delete state;
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeSurfaceCreated(JNIEnv* env, jclass, jlong handle,
                                                       jobject surface) {
    auto* state = fromHandle(handle);
    if (!state) return;
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    state->engine->setWindow(window);
    if (state->window) ANativeWindow_release(state->window);
    state->window = window;
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeSurfaceChanged(JNIEnv*, jclass, jlong handle, jint width,
                                                       jint height) {
    auto* state = fromHandle(handle);
    if (!state) return;
    state->engine->onResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeSurfaceDestroyed(JNIEnv*, jclass, jlong handle) {
    auto* state = fromHandle(handle);
    if (!state) return;
    state->engine->setWindow(nullptr); // render oqimi to'xtaguncha bloklaydi
    if (state->window) ANativeWindow_release(state->window);
    state->window = nullptr;
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeSetPaused(JNIEnv*, jclass, jlong handle, jboolean paused) {
    auto* state = fromHandle(handle);
    if (state) state->engine->setPaused(paused);
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeOnDrag(JNIEnv*, jclass, jlong handle, jfloat dx, jfloat dy) {
    auto* state = fromHandle(handle);
    if (state) state->engine->onDrag(dx, dy);
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeOnZoom(JNIEnv*, jclass, jlong handle, jfloat factor) {
    auto* state = fromHandle(handle);
    if (state) state->engine->onZoom(factor);
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeOnPan(JNIEnv*, jclass, jlong handle, jfloat dx, jfloat dy) {
    auto* state = fromHandle(handle);
    if (state) state->engine->onPan(dx, dy);
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeSetMode2D(JNIEnv*, jclass, jlong handle, jboolean mode2D) {
    auto* state = fromHandle(handle);
    if (state) state->engine->setMode2D(mode2D);
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeFocus(JNIEnv*, jclass, jlong handle, jint id) {
    auto* state = fromHandle(handle);
    if (state) state->engine->focus(static_cast<uint32_t>(id));
}

// data: [px py pz, rx ry rz, sx sy sz, r g b a]
JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeUpsertEntity(JNIEnv* env, jclass, jlong handle, jint id,
                                                     jint parent, jint mesh, jboolean visible,
                                                     jfloatArray data) {
    auto* state = fromHandle(handle);
    if (!state || !data || env->GetArrayLength(data) < 13) return;
    jfloat d[13];
    env->GetFloatArrayRegion(data, 0, 13, d);
    zamin::EntityDesc e;
    e.id = static_cast<uint32_t>(id);
    e.parent = static_cast<uint32_t>(parent);
    e.mesh = mesh >= 0 && mesh < static_cast<jint>(zamin::MeshKind::Count)
                 ? static_cast<zamin::MeshKind>(mesh)
                 : zamin::MeshKind::None;
    e.visible = visible;
    e.position = {d[0], d[1], d[2]};
    e.rotation = {d[3], d[4], d[5]};
    e.scale = {d[6], d[7], d[8]};
    for (int i = 0; i < 4; ++i) e.color[i] = d[9 + i];
    state->engine->upsertEntity(e);
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeRemoveEntity(JNIEnv*, jclass, jlong handle, jint id) {
    auto* state = fromHandle(handle);
    if (state) state->engine->removeEntity(static_cast<uint32_t>(id));
}

JNIEXPORT void JNICALL
Java_dev_zamin_editor_ZaminNative_nativeSetSelected(JNIEnv*, jclass, jlong handle, jint id) {
    auto* state = fromHandle(handle);
    if (state) state->engine->setSelected(static_cast<uint32_t>(id));
}

JNIEXPORT jint JNICALL
Java_dev_zamin_editor_ZaminNative_nativePick(JNIEnv*, jclass, jlong handle, jfloat x, jfloat y) {
    auto* state = fromHandle(handle);
    return state ? static_cast<jint>(state->engine->pick(x, y)) : 0;
}

// Muvaffaqiyatli bo'lsa yangi lokal pozitsiya [x, y, z], aks holda null
JNIEXPORT jfloatArray JNICALL
Java_dev_zamin_editor_ZaminNative_nativeDragEntity(JNIEnv* env, jclass, jlong handle, jint id,
                                                   jfloat x0, jfloat y0, jfloat x1, jfloat y1) {
    auto* state = fromHandle(handle);
    zamin::Vec3 pos;
    if (!state || !state->engine->dragEntity(static_cast<uint32_t>(id), x0, y0, x1, y1, pos))
        return nullptr;
    jfloatArray arr = env->NewFloatArray(3);
    const jfloat values[3] = {pos.x, pos.y, pos.z};
    env->SetFloatArrayRegion(arr, 0, 3, values);
    return arr;
}

JNIEXPORT jfloatArray JNICALL
Java_dev_zamin_editor_ZaminNative_nativeGetStats(JNIEnv* env, jclass, jlong handle) {
    constexpr int kCount = 6;
    jfloatArray arr = env->NewFloatArray(kCount);
    auto* state = fromHandle(handle);
    if (!state) return arr;
    const zamin::EngineStats s = state->engine->stats();
    const jfloat values[kCount] = {s.fps, s.frameMs, static_cast<jfloat>(s.frames), s.cameraYaw,
                                   s.cameraPitch, static_cast<jfloat>(s.drawCalls)};
    env->SetFloatArrayRegion(arr, 0, kCount, values);
    return arr;
}

JNIEXPORT jstring JNICALL
Java_dev_zamin_editor_ZaminNative_nativeGetDeviceName(JNIEnv* env, jclass, jlong handle) {
    auto* state = fromHandle(handle);
    return env->NewStringUTF(state ? state->engine->deviceName().c_str() : "");
}

} // extern "C"
