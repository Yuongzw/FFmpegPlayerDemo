//
// Created by Administrator on 2020/3/14.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int streamId, JavaCallHelper *javaCallHelper,
                           AVCodecContext *codecContext, AVRational time_base)
        : BaseChannel(streamId, javaCallHelper, codecContext, time_base) {
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    //CD 标准  44100 双声道 2字节
    out_buffer = static_cast<uint8_t *>(malloc(out_sample_rate * out_samplesize * out_channels));

}

void *initOpenSLESForThread(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->initOpenSLES();
    return 0;
}

void *audioDecodeForThread(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decodeData();
    return 0;
}

//OpenSLES回调函数
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //pcm 原始音频数据
    int data_size = audioChannel->getPcmData();
    if (data_size > 0) {
        (*bq)->Enqueue(bq, audioChannel->out_buffer, data_size);
    }
}


void AudioChannel::play() {
    //获取转换器上下文 SwrContext用于audio  SwsContext用于video
    swr_ctx = swr_alloc();
    /*
    * swr_alloc_set_opts(struct SwrContext *s,
                                     int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                     int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                                     int log_offset, void *log_ctx)
    */
    //设置转换参数到转换器上下文中
    swr_alloc_set_opts(swr_ctx,
            AV_CH_LAYOUT_STEREO,//输出声道布局(通道数)
            AV_SAMPLE_FMT_S16,//输出的参数(固定)
            out_sample_rate,//输出采样率
            avCodecContext->channel_layout,//输入声道布局(通道数)
            avCodecContext->sample_fmt,//采样位数
            avCodecContext->sample_rate, //采样频率
            0,0);
    //初始化转换器上下文
    swr_init(swr_ctx);
    isPlaying = true;
    pkt_queue.setWork(1);
    frame_queue.setWork(1);

    //创建初始化openesl线程
    pthread_create(&pid_init_opensles, NULL, initOpenSLESForThread, this);

    //创建音频解码线程
    pthread_create(&pid_audio_decode, NULL, audioDecodeForThread, this);
}


void AudioChannel::stop() {

}

//初始化OpenSLES
void AudioChannel::initOpenSLES() {
    //音频引擎
    SLEngineItf engineInterface = NULL;
    //音频对象
    SLObjectItf engineObject = NULL;
    //混音器
    SLObjectItf outputMixObject = NULL;
    //播放器
    SLObjectItf bqPlayerObject = NULL;
    //回调接口
    SLPlayItf bqPlayerInterface = NULL;
    //缓冲队列
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = NULL;

    SLresult result;
    /*1、创建音频引擎*/
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("创建音频引擎失败");
        return;
    }
    //初始化音频引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("初始化音频引擎失败");
        return;
    }
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    //获取一个引擎接口
    if (result != SL_RESULT_SUCCESS) {
        LOGE("获取引擎接口失败");
        return;
    }

    /*2、创建混音器*/
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("创建混音器失败");
        return;
    }
    //初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("初始化混音器失败");
        return;
    }

    /*3、创建播放器*/
    //配置音频源
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,//播放pcm格式的数据
                                   2, //双声道
                                   SL_SAMPLINGRATE_44_1,//44100的采样频率
                                   SL_PCMSAMPLEFORMAT_FIXED_16,//位数
                                   SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一样就行
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
                                   SL_BYTEORDER_LITTLEENDIAN};//小端模式
    SLDataSource slDataSource = {&loc_bufq, &format_pcm};

    //配置音频接收器
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};//混音器队列
    SLDataSink audioSnk = {&loc_outmix, NULL};
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};//播放队列的id
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};//是否采取播放器默认的队列
    (*engineInterface)->CreateAudioPlayer(engineInterface,//音频引擎
                                          &bqPlayerObject, //播放器
                                          &slDataSource,//播放器参数，播放缓冲队列，播放格式
                                          &audioSnk,//播放缓冲区
                                          1,//播放接口回调个数
                                          ids,//播放器的队列id
                                          req//是否采取播放器默认的队列
    );
    // 初始化播放器
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("初始化播放器失败");
        return;
    }

    /*4、设置缓冲队列和回调函数*/
    //获取Player接口，得到接口后调用
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("获取Player接口失败");
        return;
    }
    // 获取缓冲区队列接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("获取缓冲区队列接口失败");
        return;
    }
    // 在缓冲区队列上注册回调
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("注册回调函数失败");
        return;
    }

    /*5、设置播放状态*/
    result = (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("设置播放状态失败");
        return;
    }

    /*6、启动回调函数*/
    bqPlayerCallback(bqPlayerBufferQueue, this);
}

//解码数据
void AudioChannel::decodeData() {
    AVPacket *packet = 0;
    while (isPlaying) {
        if(!isPlaying) {
            break;
        }
        //取出音频 packet
        int ret = pkt_queue.deQueue(packet);
        if (!ret) {//如果没有音频数据  稍微要等等
            continue;
        }
        //发送一帧音频
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(packet);
        if (ret == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (ret < 0) {
            //失败
            break;
        }
        //获取AVFrame
        AVFrame *avFrame = av_frame_alloc();
        //接收一帧音频
        ret = avcodec_receive_frame(avCodecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) {
            //需要更多数据
            continue;
        } else if (ret < 0) {
            //失败
            break;
        }
        //向队列里压数据
        frame_queue.enQueue(avFrame);
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(1000 * 10);
        }
    }
}

int AudioChannel::getPcmData() {
    AVFrame *frame = 0;
    int data_size;
    while (isPlaying) {
        if (!isPlaying) {
            break;
        }
        int ret = frame_queue.deQueue(frame);
        if (!ret) {
            continue;
        }

        //转换一帧音频
        uint64_t dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                out_sample_rate,
                frame->sample_rate,
                AV_ROUND_UP
                );
        int nb = swr_convert(swr_ctx, &out_buffer, dst_nb_samples,
                    (const uint8_t **)(frame->data), frame->nb_samples);
        //转换之后数据的大小
        data_size = nb * out_channels * out_samplesize;
        //计算时间线
        clock = frame->pts * av_q2d(time_base);
        break;
    }
    releaseAvFrame(frame);
    return data_size;

}
