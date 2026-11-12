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

JNIEXPORT jfloatArray JNICALL
Java_dev_zamin_editor_ZaminNative_nativeGetStats(JNIEnv* env, jclass, jlong handle) {
    jfloatArray arr = env->NewFloatArray(3);
    auto* state = fromHandle(handle);
    if (!state) return arr;
    const zamin::EngineStats s = state->engine->stats();
    const jfloat values[3] = {s.fps, s.frameMs, static_cast<jfloat>(s.frames)};
    env->SetFloatArrayRegion(arr, 0, 3, values);
    return arr;
}

JNIEXPORT jstring JNICALL
Java_dev_zamin_editor_ZaminNative_nativeGetDeviceName(JNIEnv* env, jclass, jlong handle) {
    auto* state = fromHandle(handle);
    return env->NewStringUTF(state ? state->engine->deviceName().c_str() : "");
}

} // extern "C"
