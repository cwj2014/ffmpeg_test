#ifndef CUT_MP4_TEST_H_H_
#define CUT_MP4_TEST_H_H_


#include "global.h"

//判断是否需要继续读取packet
static inline bool isEndReadPacket(const uint8_t* stream_exit, const AVFormatContext* in_avformat_ctx){
    bool exit = true; 
    for(int i=0; i<in_avformat_ctx->nb_streams; i++){
        if(stream_exit[i]==0){
            exit = false;
            break;
        }
    }
    return exit;
}

/*
裁剪多媒体文件
@inputfile: 输入文件
@outputfile: 输出文件
@starttime: 开始时间
@endtime: 结束时间
*/
int cut_media_file2(const char* inputfile, const char* outputfile, float starttime, float endtime){
    int ret = -1;
    AVFormatContext* in_avformat_ctx = nullptr;
    AVFormatContext* out_avformat_ctx = nullptr;
    AVPacket * pkt;
    //计算时间差间隔
    int64_t *pre_dts;
    int64_t *pre_pts;
    //计算存储当前时间
    int64_t *cur_dts;
    int64_t *cur_pts;
    uint8_t *stream_exit;
    //视频进行解码再编码 
    AVCodecContext *vDecContext = nullptr;
    AVCodecContext *vEncContext = nullptr;
    int video_stream_index = -1;
    AVFrame* inframe;
    static int pts = 0;
    auto flush = [&](){
        encode(vEncContext, nullptr, [=](AVCodecContext* avc, const AVPacket* avpacket){         
            AVPacket* newpkt = av_packet_clone(avpacket);
            av_packet_rescale_ts(newpkt, vEncContext->time_base, out_avformat_ctx->streams[video_stream_index]->time_base);
            newpkt->stream_index = video_stream_index;
            int ret = av_interleaved_write_frame(out_avformat_ctx, newpkt);
            av_packet_free(&newpkt);
            if (ret < 0) {
                fprintf(stderr, "av_interleaved_write_frame error\n");
            }
        });
    };

    bool isNeedFlush = false;
    bool hasIFrame = false;

    //带goto语言的变量都需要提前定义如下
    if(avformat_open_input(&in_avformat_ctx, inputfile, nullptr, nullptr) < 0){
        goto end;
    }
    if(avformat_find_stream_info(in_avformat_ctx, nullptr)){
        goto end;
    }
    if(avformat_alloc_output_context2(&out_avformat_ctx, nullptr, nullptr, outputfile) < 0){
        goto end;
    }
    
    for(int i=0; i<in_avformat_ctx->nb_streams; i++){
        AVStream* stream = in_avformat_ctx->streams[i];
        AVStream* out = avformat_new_stream(out_avformat_ctx, nullptr);
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            const AVCodec* decCodec = avcodec_find_decoder(stream->codecpar->codec_id);
            vDecContext = avcodec_alloc_context3(decCodec);
            avcodec_parameters_to_context(vDecContext, stream->codecpar);
            if(avcodec_open2(vDecContext, decCodec, nullptr)<0){
                goto end;
            }
            
            const AVCodec* encCodec = avcodec_find_encoder(stream->codecpar->codec_id);
            vEncContext = avcodec_alloc_context3(encCodec);
            avcodec_parameters_to_context(vEncContext, stream->codecpar);
            //根据解码参数配置编码参数，如果能有办法从AVStream获取所有编码参数最好
            //那样就可以在碰到下一个关键帧直接数据copy
            {
                vEncContext->framerate = stream->r_frame_rate;
                vEncContext->time_base = (AVRational){vEncContext->framerate.den, vEncContext->framerate.num};

                vEncContext->gop_size = vDecContext->gop_size;
                vEncContext->has_b_frames = vDecContext->has_b_frames;
                vEncContext->max_b_frames = vDecContext->max_b_frames;
                vEncContext->pix_fmt = vDecContext->pix_fmt;
            }
            if(avcodec_open2(vEncContext, encCodec, nullptr)<0){
                goto end;
            }
            avcodec_parameters_from_context(out->codecpar, vEncContext);
            out->codecpar->codec_tag = 0;
            video_stream_index = i;

        }else{
            avcodec_parameters_copy(out->codecpar, stream->codecpar);
            //下边这个参数很重要，如果不重置这个参数会出现类似于Tag avc1 incompatible with output codec id '27' ([7][0][0][0])这个的错误
            out->codecpar->codec_tag = 0;
        }
    }
    

    if (!(out_avformat_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_avformat_ctx->pb, outputfile, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", outputfile);
            goto end;
        }
    }

    if(avformat_write_header(out_avformat_ctx, nullptr) < 0){
        goto end;
    }


    //跳转到指定帧
    ret = av_seek_frame(in_avformat_ctx, -1, starttime * AV_TIME_BASE, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    if (ret < 0) {
        fprintf(stderr, "Error seek\n");
        goto end;
    }

    // 根据流数量申请空间，并全部初始化为0
    pre_dts = (int64_t*)malloc(sizeof(int64_t) * in_avformat_ctx->nb_streams);
    memset(pre_dts, 0, sizeof(int64_t) * in_avformat_ctx->nb_streams);

    pre_pts = (int64_t*)malloc(sizeof(int64_t) * in_avformat_ctx->nb_streams);
    memset(pre_pts, 0, sizeof(int64_t) * in_avformat_ctx->nb_streams);
    
    stream_exit = (uint8_t*)malloc(sizeof(uint8_t)*in_avformat_ctx->nb_streams);
    memset(stream_exit, 0, sizeof(uint8_t) * in_avformat_ctx->nb_streams);

    pkt = av_packet_alloc();
    inframe = av_frame_alloc();

    while(!isEndReadPacket(stream_exit, in_avformat_ctx)){
        ret = av_read_frame(in_avformat_ctx, pkt);
         if(ret!=0){
            printf("read error or file end\n");
            break;
        }
        AVStream *in_stream = in_avformat_ctx->streams[pkt->stream_index];
        AVStream *out_stream = out_avformat_ctx->streams[pkt->stream_index];
        // 时间超过要截取的时间，就退出循环
        if (av_q2d(in_stream->time_base) * pkt->pts > endtime) {
            stream_exit[pkt->stream_index] = 1;
            av_packet_unref(pkt);
            continue;
        }
        //视频进行二次编码
        if(video_stream_index == pkt->stream_index){
            bool b = av_q2d(in_stream->time_base) * pkt->pts >= starttime;
            decode(vDecContext, pkt, [&](AVCodecContext* avctx, const AVFrame* frame){
                if(b){
                    inframe->format = AV_PIX_FMT_YUV420P;
                    inframe->width  = frame->width;
                    inframe->height = frame->height;
                    av_frame_get_buffer(inframe, 32);
                    av_image_copy(inframe->data, inframe->linesize, (const uint8_t**)frame->data, frame->linesize, AV_PIX_FMT_YUV420P,inframe->width, inframe->height);
                    
                    inframe->pts = pts++;
                    inframe->pict_type = AV_PICTURE_TYPE_NONE;
                    isNeedFlush = true;
                    encode(vEncContext, inframe, [=](AVCodecContext* avc, const AVPacket* avpacket){

                        AVPacket* newpkt = av_packet_clone(avpacket);
                        av_packet_rescale_ts(newpkt, vEncContext->time_base, out_stream->time_base);
                        newpkt->stream_index = video_stream_index;
                        //printf("before rescale_ts: pts:%lld, dts:%lld, after rescale_ts:pts:%lld, dts:%lld\n", avpacket->pts, avpacket->dts, newpkt->pts, newpkt->dts);
                        int ret = av_interleaved_write_frame(out_avformat_ctx, newpkt);
                        av_packet_free(&newpkt);
                        if (ret < 0) {
                            fprintf(stderr, "Error muxing packet\n");
                        }
                    });
                    av_frame_unref(inframe);
                }
            });
        }else{
            if(av_q2d(in_stream->time_base) * pkt->pts >= starttime){
                // 将截取后的每个流的起始dts 、pts保存下来，作为开始时间，用来做后面的时间基转换
                if (pre_dts[pkt->stream_index] == 0) {
                    pre_dts[pkt->stream_index] = pkt->dts;
                }
                if (pre_pts[pkt->stream_index] == 0) {
                    pre_pts[pkt->stream_index] = pkt->pts;
                }

                // 时间基转换
                pkt->pts = av_rescale_q_rnd(pkt->pts - pre_pts[pkt->stream_index], in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->dts = av_rescale_q_rnd(pkt->dts - pre_dts[pkt->stream_index], in_stream->time_base,out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

                //一帧视频播放时间必须在解码时间点之后，当出现pkt->pts < pkt->dts时会导致程序异常，所以我们丢掉有问题的帧，不会有太大影响。
                if (pkt->pts < pkt->dts) {
                    continue;
                }
                ret = av_interleaved_write_frame(out_avformat_ctx, pkt);
                if (ret < 0) {
                   fprintf(stderr, "Error muxing packet\n");
                   break;
                }
               
            }
        }
        av_packet_unref(pkt);
    }
    //二次编码
    if(isNeedFlush)
        flush();

    free(pre_dts);
    free(pre_pts);
    free(stream_exit);

    av_write_trailer(out_avformat_ctx);
    
    av_packet_free(&pkt);
    av_frame_free(&inframe);
    
    ret = 0;
end:
    avcodec_close(vDecContext);
    avcodec_close(vEncContext);
    
    avformat_close_input(&in_avformat_ctx);

    /* close output */
    if (out_avformat_ctx && !(out_avformat_ctx->flags & AVFMT_NOFILE))
        avio_closep(&out_avformat_ctx->pb);
    avformat_free_context(out_avformat_ctx);

    return ret;
}

/*
裁剪多媒体文件
@inputfile: 输入文件
@outputfile: 输出文件
@starttime: 开始时间
@time: 时长
*/
int cut_media_file(const char* inputfile, const char* outputfile, float starttime, float time){
    return cut_media_file2(inputfile, outputfile, starttime, starttime+time);
}

int cut_media_file_test(){
    const char* inputfile = "./半壶纱.mp4";
    const char* outputfile = "./cutfile.mp4";
    float starttime = 5;
    float time = 15;
    return cut_media_file(inputfile, outputfile, starttime, time);
}


#endif