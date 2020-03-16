//
// Created by Administrator on 2020/3/14.
//控制层
//

#ifndef FFMPEGPLAYERDEMO_FFMPEGCONTROL_H
#define FFMPEGPLAYERDEMO_FFMPEGCONTROL_H

#include <pthread.h>
#include <android/native_window.h>
#include "JavaCallHelper.h"
#include "VideoChannel.h"
#include "AudioChannel.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/time.h"
};


class FFmpegControl {
public:
    FFmpegControl(JavaCallHelper *javaCallHelper, const char* dataSource);
    ~FFmpegControl();
    void prepare();//准备
    void ffmpegPrepare();
    void start();
    void play();
    void setRenderCallback(RenderFrame renderFrame);

private:
    bool isPlaying;
    pthread_t pid_prepare;  //准备线程
    pthread_t pid_play;  //播放线程
    AVFormatContext *formatContext;
    char *url;
    JavaCallHelper *javaCallHelper;
    AudioChannel *audioChannel;
    VideoChannel *videoChannel;
    RenderFrame renderFrame;

};


#endif //FFMPEGPLAYERDEMO_FFMPEGCONTROL_H
