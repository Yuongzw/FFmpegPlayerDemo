//
// Created by Administrator on 2020/3/14.
//

#include "JavaCallHelper.h"
#include "macro.h"

JavaCallHelper::JavaCallHelper(JavaVM *javaVm, JNIEnv *env, jobject &jobj) {
    this->javaVm = javaVm;
    this->env = env;
    this->jobj = this->env->NewGlobalRef(jobj);//实例化一个全局的对象，以免会被回收

    //反射
    jclass jclazz = env->GetObjectClass(this->jobj);
    jmid_error= env->GetMethodID(jclazz, "onError", "(I)V");
    jmid_prepare= env->GetMethodID(jclazz, "onPrepare", "()V");
    jmid_progress= env->GetMethodID(jclazz, "onProgress", "(I)V");
}

JavaCallHelper::~JavaCallHelper() {

}

void JavaCallHelper::onError(int thread, int errorCode) {
    if (thread == THREAD_CHILD) {
        //如果是子线程，需要绑定主线程
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_error, errorCode);
        javaVm->DetachCurrentThread();//解绑

    } else {    //如果是主线程，直接调用回调方法
        env->CallVoidMethod(jobj, jmid_error, errorCode);
    }
}

void JavaCallHelper::onPrepare(int thread) {
    if (thread == THREAD_CHILD) {
        //如果是子线程，需要绑定主线程
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_prepare);
        javaVm->DetachCurrentThread();//解绑

    } else {    //如果是主线程，直接调用回调方法
        env->CallVoidMethod(jobj, jmid_prepare);
    }
}

void JavaCallHelper::onProgress(int thread, int progress) {
    if (thread == THREAD_CHILD) {
        //如果是子线程，需要绑定主线程
        JNIEnv *jniEnv;
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_progress, progress);
        javaVm->DetachCurrentThread();//解绑

    } else {    //如果是主线程，直接调用回调方法
        env->CallVoidMethod(jobj, jmid_progress, progress);
    }
}
