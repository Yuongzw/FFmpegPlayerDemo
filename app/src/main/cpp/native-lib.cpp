#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "FFmpegControl.h"
#include "JavaCallHelper.h"

ANativeWindow *window = 0;
FFmpegControl *fFmpegControl;
JavaCallHelper *javaCallHelper;
JavaVM *javaVm = NULL;

//获取JavaVM实例 该方法会主动调用
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    return JNI_VERSION_1_4;
}

void renderFrame(uint8_t *data, int linesize, int w, int h) {
    //渲染
    //设置ANativeWindow的缓冲区大小
    ANativeWindow_setBuffersGeometry(window, w, h, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, 0)) {
        ANativeWindow_release(window);
        window= 0;
        return;
    }
    //一行一行拷贝
    uint8_t *window_data = static_cast<uint8_t *>(buffer.bits);//展现到窗体的数据
    int window_linesize = buffer.stride * 4;    //每一行像素大小 argb
    uint8_t *src_data = data;//源数据
    for (int i = 0; i < buffer.height; ++i) {
        memcpy(window_data + i * window_linesize, src_data + i * linesize, window_linesize);
    }
    ANativeWindow_unlockAndPost(window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_yuong_ffmpegplayer_player_FFmpegPlayer_native_1prepare(JNIEnv *env, jobject thiz,
                                                                jstring data_source) {
    const char *dataSource = env->GetStringUTFChars(data_source, 0);

    javaCallHelper = new JavaCallHelper(javaVm, env, thiz);
    fFmpegControl = new FFmpegControl(javaCallHelper, dataSource);
    fFmpegControl->setRenderCallback(renderFrame);
    fFmpegControl->prepare();

    env->ReleaseStringUTFChars(data_source, dataSource);
}extern "C"
JNIEXPORT void JNICALL
Java_com_yuong_ffmpegplayer_player_FFmpegPlayer_native_1setSurface(JNIEnv *env, jobject thiz,
                                                                   jobject surface) {
    //先把之前的释放掉
    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    //创建window对象
    window = ANativeWindow_fromSurface(env, surface);

}extern "C"
JNIEXPORT void JNICALL
Java_com_yuong_ffmpegplayer_player_FFmpegPlayer_native_1start(JNIEnv *env, jobject thiz) {
    //播放
    if (fFmpegControl) {
        fFmpegControl->start();
    }
}