#include <jni.h>
#include <android/native_window_jni.h>
#include "dice3d/dice3d.h"

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_dice3d_DiceRenderer_nativeCreate(JNIEnv*, jobject) {
    return (jlong)(void*)dice3d_create();
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeDestroy(JNIEnv*, jobject, jlong ptr) {
    dice3d_destroy((Dice3DSceneRef)(void*)ptr);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeAttachSurface(
    JNIEnv* env, jobject, jlong ptr, jobject surface,
    jint width, jint height)
{
    ANativeWindow* win = ANativeWindow_fromSurface(env, surface);
    dice3d_attach_surface((Dice3DSceneRef)(void*)ptr, win,
                          (uint32_t)width, (uint32_t)height);
    ANativeWindow_release(win);  // release our ref; Filament retains internally
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeDetachSurface(JNIEnv*, jobject, jlong ptr) {
    dice3d_detach_surface((Dice3DSceneRef)(void*)ptr);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeResize(JNIEnv*, jobject, jlong ptr,
                                           jint w, jint h) {
    dice3d_resize((Dice3DSceneRef)(void*)ptr, (uint32_t)w, (uint32_t)h);
}

JNIEXPORT jint JNICALL
Java_com_dice3d_DiceRenderer_nativeAddDie(
    JNIEnv*, jobject, jlong ptr, jint sides,
    jfloat bevel, jfloat r, jfloat g, jfloat b, jfloat a, jint white)
{
    return (jint)dice3d_add_die((Dice3DSceneRef)(void*)ptr, sides, bevel,
                                r, g, b, a, white);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeRemoveDie(JNIEnv*, jobject, jlong ptr, jint handle) {
    dice3d_remove_die((Dice3DSceneRef)(void*)ptr, (uint32_t)handle);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeRoll(
    JNIEnv*, jobject, jlong ptr, jint handle, jint result, jfloat duration)
{
    dice3d_roll((Dice3DSceneRef)(void*)ptr, (uint32_t)handle, result, duration);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeLoadMaterial(
    JNIEnv* env, jobject, jlong ptr, jbyteArray data)
{
    jsize len = env->GetArrayLength(data);
    jbyte* buf = env->GetByteArrayElements(data, nullptr);
    dice3d_load_material((Dice3DSceneRef)(void*)ptr, buf, (size_t)len);
    env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeLoadAtlas(
    JNIEnv* env, jobject, jlong ptr, jbyteArray rgba, jint w, jint h)
{
    jbyte* buf = env->GetByteArrayElements(rgba, nullptr);
    dice3d_load_atlas((Dice3DSceneRef)(void*)ptr, buf, (uint32_t)w, (uint32_t)h);
    env->ReleaseByteArrayElements(rgba, buf, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeTick(JNIEnv*, jobject, jlong ptr, jfloat dt) {
    dice3d_tick((Dice3DSceneRef)(void*)ptr, dt);
}

JNIEXPORT void JNICALL
Java_com_dice3d_DiceRenderer_nativeRenderFrame(JNIEnv*, jobject, jlong ptr) {
    dice3d_render_frame((Dice3DSceneRef)(void*)ptr);
}

} // extern "C"
