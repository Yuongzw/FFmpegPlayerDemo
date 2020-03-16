//
// Created by Administrator on 2020/3/14.
//

#include "VideoChannel.h"


void dropFrame(queue<AVFrame *> &q) {
    while (!q.empty()) {
        LOGE("丢弃视频Frame帧。。。");
        AVFrame *frame = q.front();
        q.pop();
        BaseChannel::releaseAvFrame(frame);
    }
}

VideoChannel::VideoChannel(int streamId, JavaCallHelper *javaCallHelper,
                           AVCodecContext *codecContext, AVRational time_base)
        : BaseChannel(streamId, javaCallHelper, codecContext, time_base) {
    frame_queue.setSyncHandle(dropFrame);
}

VideoChannel::~VideoChannel() {

}

void *decode(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->decodePacket();
}

void *synchronize(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->synchronizeFrame();
}

void VideoChannel::play() {
    isPlaying = true;
    pkt_queue.setWork(1);
    frame_queue.setWork(1);

    //创建解码线程
    pthread_create(&pid_decoding, NULL, decode, this);
    //创建播放线程（同步线程）
    pthread_create(&pid_synchronize, NULL, synchronize, this);

}

void VideoChannel::stop() {

}

//解码  在子线程中执行
void VideoChannel::decodePacket() {
    AVPacket *packet = 0;
    while (isPlaying) {
        int ret = pkt_queue.deQueue(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //发送一帧视频
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
        //接收一帧视频
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
    releaseAvPacket(packet);
}

//播放函数
void VideoChannel::synchronizeFrame() {
    /*
    * srcW、srcH：输入视频的宽高
    * srcFormat：输入的编码方式
    * dstW、dstH：输出视频的宽高
    * dstFormat：输出的编码方式
    * flags：具体的转换方式
    * SWS_FAST_BILINEAR、SWS_POINT：重视速度
    * SWS_BICUBIC、SWS_SPLINE、SWS_LANCZOS：重视质量
    * 缩小：
    * SWS_FAST_BILINEAR、SWS_POINT：重视速度
    * SWS_BICUBIC、SWS_FAST_BILINEAR：重视质量
    * SWS_BICUBIC、SWS_SPLINE、SWS_LANCZOS：重视锐度
    *
    * SwsContext *sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
                                 int dstW, int dstH, enum AVPixelFormat dstFormat,
                                 int flags, SwsFilter *srcFilter,
                                 SwsFilter *dstFilter, const double *param);
    */
    //初始化转换器上下文
    SwsContext *swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width, avCodecContext->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BILINEAR, 0, 0, 0);
    uint8_t *dst_data[4];//argb
    int dst_linesize[4];
    av_image_alloc(dst_data, dst_linesize, avCodecContext->width, avCodecContext->height,
                   AV_PIX_FMT_RGBA, 1);
    AVFrame *frame = 0;
    while (isPlaying) {
        if (!isPlaying) {
            break;
        }
        int ret = frame_queue.deQueue(frame);
        if (!ret) {
            continue;
        }
        LOGE("解码一帧视频:%d", frame_queue.size());
        //将yuv数据转换为rgba数据
        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                  dst_data, dst_linesize);

        //计算视频时间线  把解码时间算进去
        /**官方解释
         * When decoding, this signals how much the picture must be delayed.
         * extra_delay = repeat_pict / (2*fps)
         */
        double decodeDelay = frame->repeat_pict / (fps * 2);//解码时间
        clock = frame->pts * av_q2d(time_base);
        double frame_delay = 1.0 / fps;
        //获取音频时间线
        double audioClock = audioChannel->clock;
        double diff = clock - audioClock;
        double delay = frame_delay + decodeDelay;//真正的延迟时间
        LOGE("相差：%f帧", diff);
        if (clock > audioClock) {//表示视频比音频快
            if (diff > 1) { //表示相差太久了
                av_usleep((delay * 2) * 1000000);
            } else {
                av_usleep((delay + diff) * 1000000);
            }
        } else {    //音频比视频快
            if (diff > 1) {//表示相差太久了
                //不休眠
            } else if (diff >= 0.05){    //可控范围内
                //视频需要追赶  丢帧
                releaseAvFrame(frame);
                frame_queue.sync();
            } else {
            }
        }
        //回调到主线程渲染
        renderFrame(dst_data[0], dst_linesize[0], frame->width, frame->height);
        //释放
        releaseAvFrame(frame);
    }

    //释放
    av_freep(&dst_data[0]);
    isPlaying = false;
    releaseAvFrame(frame);
    sws_freeContext(swsContext);
}

void VideoChannel::setRenderCallback(RenderFrame renderFrame) {
    this->renderFrame = renderFrame;
}

void VideoChannel::setFps(double fps) {
    this->fps = fps;
}

//丢帧
void VideoChannel::dropPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        LOGE("丢弃视频Packet帧。。。");
        AVPacket *packet = q.front();
        if (packet->flags != AV_PKT_FLAG_KEY) { //不是关键帧的丢弃
            q.pop();
            releaseAvPacket(packet);
        } else{
            break;
        }
    }
}

