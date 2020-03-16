//
// Created by Administrator on 2020/3/14.
//

#include "FFmpegControl.h"
#include "macro.h"

FFmpegControl::FFmpegControl(JavaCallHelper *javaCallHelper, const char *dataSource) {
    url = new char[strlen(dataSource) + 1];
    strcpy(url, dataSource);
    this->javaCallHelper = javaCallHelper;

}

FFmpegControl::~FFmpegControl() {

}


void *prepareFFmpegForThread(void *args) {
    FFmpegControl *control = static_cast<FFmpegControl *>(args);
    control->ffmpegPrepare();
    return 0;
}
void *startForThread(void *args) {
    FFmpegControl *control = static_cast<FFmpegControl *>(args);
    control->play();
    return 0;
}

void FFmpegControl::prepare() {
    pthread_create(&pid_prepare, NULL, prepareFFmpegForThread, this);
}

void FFmpegControl::ffmpegPrepare() {
    //执行线程   --》 子线程
    //初始化网络模块
    avformat_network_init();
    //获取 AVFormatContext 总的上下文
    formatContext = avformat_alloc_context();
    //打开url
    AVDictionary *opts = NULL;
    av_dict_set(&opts, "timeout", "3000000", 0);//设置超时时间
    int ret = avformat_open_input(&formatContext, url, NULL, &opts);
    if (ret) {  //如果不为0 则打开失败
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    //寻找视频流(通知FFmpeg将流解析出来)
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        //获取流
        AVStream *stream = formatContext->streams[i];

        //获取流的解码参数
        AVCodecParameters *avCodecParameters = formatContext->streams[i]->codecpar;
        //获取解码器
        AVCodec *avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
        if (!avCodec) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }
        //创建解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(avCodec);
        if (!codecContext) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        //将解码器参数复制到解码器上下文中
        if (avcodec_parameters_to_context(codecContext, avCodecParameters) < 0) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        //打开解码器
        if (avcodec_open2(codecContext, avCodec, NULL) != 0) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        //音频
        if (avCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, javaCallHelper, codecContext, stream->time_base);
        } else if (avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {    //视频
            AVRational frame_rate = stream->avg_frame_rate;//帧率
            //获取帧率
            double fps = av_q2d(frame_rate);
            videoChannel = new VideoChannel(i, javaCallHelper, codecContext,  stream->time_base);
            videoChannel->setRenderCallback(renderFrame);
            videoChannel->setFps(fps);
        }
    }
    //如果两个为空  说明没有音视频
    if (!audioChannel && !videoChannel) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }
    videoChannel->audioChannel = audioChannel;
    //如果有一个成功，都可以播放
    if (javaCallHelper) {
        javaCallHelper->onPrepare(THREAD_CHILD);
    }

}


void FFmpegControl::play() {
    int ret = 0;
    while (isPlaying) {
        //由于读取数据包的速度远远大于渲染的速度，所以要加以控制
        if (audioChannel && audioChannel->pkt_queue.size() >100) {
            av_usleep(1000 * 10);//10 ms
            continue;
        }
        if (videoChannel && videoChannel->pkt_queue.size() >100) {
            av_usleep(1000 * 10);//10 ms
            continue;
        }
        //读取包
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(formatContext, packet);
        if (ret == 0) {
            //将数据包加入队列
            if (audioChannel && packet->stream_index == audioChannel->channelId) {
                audioChannel->pkt_queue.enQueue(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->channelId) {
                videoChannel->pkt_queue.enQueue(packet);
            }
        } else if(ret == AVERROR_EOF) {
            //读取完毕，但不一定播放完毕
            if (videoChannel && videoChannel->pkt_queue.empty() && audioChannel && audioChannel->pkt_queue.empty()) {
                //播放完毕
                LOGE("播放完毕");
                break;
            }
        } else {
            break;
        }
    }
    isPlaying = false;
    audioChannel->stop();
    videoChannel->stop();
}

void FFmpegControl::start() {
    isPlaying = true;
    if (audioChannel) {
        audioChannel->play();
    }
    if (videoChannel) {
        videoChannel->play();
    }
    pthread_create(&pid_play, NULL, startForThread, this);
}

void FFmpegControl::setRenderCallback(RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}

