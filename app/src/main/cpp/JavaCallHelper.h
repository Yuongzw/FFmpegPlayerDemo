//
// Created by Administrator on 2020/3/14.
//

#ifndef FFMPEGPLAYERDEMO_JAVACALLHELPER_H
#define FFMPEGPLAYERDEMO_JAVACALLHELPER_H

#include <jni.h>

//C 反射得到Java类
class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *javaVm, JNIEnv *env, jobject &jobj);

    ~JavaCallHelper();

    void onError(int thread, int errorCode);
    void onPrepare(int thread);
    void onProgress(int thread, int progress);

private:
    JavaVM *javaVm;
    JNIEnv *env;
    jobject jobj;
    jmethodID jmid_prepare;
    jmethodID jmid_error;
    jmethodID jmid_progress;
};


#endif //FFMPEGPLAYERDEMO_JAVACALLHELPER_H
