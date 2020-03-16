//
// Created by Administrator on 2020/3/14.
//

#ifndef FFMPEGPLAYERDEMO_VIDEOCHANNEL_H
#define FFMPEGPLAYERDEMO_VIDEOCHANNEL_H


extern "C" {
#include <libavcodec/avcodec.h>
};
#include "JavaCallHelper.h"
#include "BaseChannel.h"
#include "AudioChannel.h"

/*
 * uint8_t *：argb数据
 * int:一行的像素个数
 * int:图片宽
 * int:图片高
 */
typedef void (*RenderFrame)(uint8_t *, int, int, int);
class VideoChannel: public BaseChannel {
public:
    //streamId：流id；
    VideoChannel(int streamId, JavaCallHelper *javaCallHelper, AVCodecContext *codecContext, AVRational time_base);

    ~VideoChannel();

    void play();

    void stop();

    void decodePacket();

    void synchronizeFrame();

    void setRenderCallback(RenderFrame renderFrame);

    void setFps(double fps);

//    void dropFrame(queue<AVFrame *> &q);//丢Frame帧
    void dropPacket(queue<AVPacket *> &q);//丢Packet帧

    AudioChannel *audioChannel;

private:
    pthread_t pid_decoding;//解码线程
    pthread_t pid_synchronize;//同步线程
    RenderFrame renderFrame;
    double fps;

};


#endif //FFMPEGPLAYERDEMO_VIDEOCHANNEL_H
