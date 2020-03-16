//
// Created by Administrator on 2020/3/14.
//

#ifndef FFMPEGPLAYERDEMO_AUDIOCHANNEL_H
#define FFMPEGPLAYERDEMO_AUDIOCHANNEL_H

#include "JavaCallHelper.h"
#include "BaseChannel.h"
#include <SLES/OpenSLES_Android.h>

class AudioChannel: public BaseChannel{

public:
    AudioChannel(int streamId, JavaCallHelper *javaCallHelper, AVCodecContext *codecContext, AVRational time_base);

    void play();

    void stop();

    void initOpenSLES();

    void decodeData();

    int getPcmData();
    uint8_t *out_buffer;

private:
    pthread_t pid_init_opensles;
    pthread_t pid_audio_decode;
    SwrContext *swr_ctx = NULL;
    int out_channels;
    int out_samplesize;
    int out_sample_rate;

};


#endif //FFMPEGPLAYERDEMO_AUDIOCHANNEL_H
