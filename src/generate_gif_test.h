#ifndef GENERATE_GIF_TEST_H_H_
#define GENERATE_GIF_TEST_H_H_

#include "global.h"
#include "codecimpl.h"
#include "video_filter_tool.h"

/*
@inputFile: 输入文件
@outputFile： 输出文件
@time: 时长
@width: gif宽
@height: gif高
*/
int create_gif(const char* inputFile, const char* outputFile, int time, int width, int height){
    int ret = -1;
    AVFormatContext* in_avformat_ctx = nullptr;
    AVFormatContext* out_avformat_ctx = nullptr;
    //视频进行解码再编码 
    AVCodecContext *vDecContext = nullptr;
    AVCodecContext *vEncContext = nullptr;
    int video_stream_index = -1;
    AVPacket *packet;
    AVFrame *filt_frame;
    AVFilterContext* buffersrcCtx;
    AVFilterContext* buffersinkCtx;
    std::shared_ptr<VideoFilterManager> videoFilter = nullptr;
    AVPixelFormat dest_pix_fmt = AV_PIX_FMT_PAL8;
    int dest_fps = 10;
    //带goto语言的变量都需要提前定义如下
    if(avformat_open_input(&in_avformat_ctx, inputFile, nullptr, nullptr) < 0){
        goto end;
    }
    video_stream_index = av_find_best_stream(in_avformat_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_index == -1){
        goto end;
    }
    if(avformat_alloc_output_context2(&out_avformat_ctx, nullptr, nullptr, outputFile) < 0){
        goto end;
    }

    {//初始化编解码块
        AVStream* stream = in_avformat_ctx->streams[video_stream_index];
        AVStream* out = avformat_new_stream(out_avformat_ctx, nullptr);
            
        const AVCodec* decCodec = avcodec_find_decoder(stream->codecpar->codec_id);
        vDecContext = avcodec_alloc_context3(decCodec);
        avcodec_parameters_to_context(vDecContext, stream->codecpar);
        if(avcodec_open2(vDecContext, decCodec, nullptr)<0){
            goto end;
        }
        
        const AVCodec* encCodec = avcodec_find_encoder(out_avformat_ctx->oformat->video_codec);
        vEncContext = avcodec_alloc_context3(encCodec);

        vEncContext->width = width;
        vEncContext->height = height;
        vEncContext->codec_type = AVMEDIA_TYPE_VIDEO;
        vEncContext->pix_fmt = dest_pix_fmt;
        vEncContext->time_base = { 1, dest_fps };

        if(avcodec_open2(vEncContext, encCodec, nullptr)<0){
            goto end;
        }
        avcodec_parameters_from_context(out->codecpar, vEncContext);
    }

    if (!(out_avformat_ctx->oformat->flags & AVFMT_NOFILE)) {
      ret = avio_open(&out_avformat_ctx->pb, outputFile, AVIO_FLAG_WRITE);
      if (ret < 0) {
          fprintf(stderr, "Could not open output file '%s'", outputFile);
          goto end;
      }
    }

    if(avformat_write_header(out_avformat_ctx, nullptr) < 0){
        goto end;
    }

    videoFilter = std::make_shared<VideoFilterManager>();

    {//配置filter块
        char in_args[512];
	    AVPixelFormat pix_fmts = AV_PIX_FMT_YUV420P;
        AVRational timebase = in_avformat_ctx->streams[video_stream_index]->time_base;
	    snprintf(in_args, sizeof(in_args),"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", 
            vDecContext->width, vDecContext->height, AV_PIX_FMT_YUV420P, timebase.num, timebase.den, vDecContext->sample_aspect_ratio.num, vDecContext->sample_aspect_ratio.den);
        
        buffersrcCtx = videoFilter->CreateBufferFilter(in_args, "in"); 
        enum AVPixelFormat dest_pix_fmts[] = { dest_pix_fmt, AV_PIX_FMT_NONE };
        buffersinkCtx = videoFilter->CreateBufferSinkFilter(dest_pix_fmts, "out");

        AVFilterInOut* inputs = avfilter_inout_alloc();
        AVFilterInOut* outputs = avfilter_inout_alloc();

        outputs->name = av_strdup("in");
        outputs->filter_ctx = buffersrcCtx;
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = buffersinkCtx;
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        //生成gif最关键的filter配置
        char filter_desc[1024];
        snprintf(filter_desc, sizeof(filter_desc),"format=pix_fmts=rgb32,fps=%d,scale=%d:%d:flags=lanczos,split [o1] [o2];[o1] palettegen [p]; [o2] fifo [o3];[o3] [p] paletteuse"
            ,dest_fps, width, height);
        if(videoFilter->InsertFilter(inputs, outputs, filter_desc) < 0){
            av_log(nullptr, AV_LOG_ERROR, "parse filter graph error\n");
            goto end;
        }
    }

    if(!videoFilter->FilterConfig()){
        goto end;
    }

    packet = av_packet_alloc();
    av_init_packet(packet);
    filt_frame = av_frame_alloc();
    while(1){
        int ret = av_read_frame(in_avformat_ctx, packet);
        if(ret!=0){
            printf("read error or file end\n");
            break;
        }
        if(packet->stream_index == video_stream_index){
            if (av_q2d(in_avformat_ctx->streams[video_stream_index]->time_base) * packet->pts > time) {
                av_packet_unref(packet);
                break;;
            }
            decode(vDecContext, packet, [&](AVCodecContext* ctx, const AVFrame* frame){
                //对frame进行filter处理
                AVFrame* f = av_frame_clone(frame);
                 // processing one frame
                f->pts = frame->best_effort_timestamp;
                videoFilter->AddFrame(buffersrcCtx, f);
                av_frame_free(&f);
            });
        }
        
        av_packet_unref(packet);
    }
     

    if ((ret = videoFilter->AddFrame(buffersrcCtx, NULL)) >= 0) {
        while (1) {//从buffersink设备上下文获取视频帧
            ret = videoFilter->GetFrame(buffersinkCtx, filt_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                break;
            
            //再次gif编码
            encode(vEncContext, filt_frame, [&](AVCodecContext* ctx, const AVPacket* avpkt){
                //写入gif文件
                AVPacket* pkt = av_packet_clone(avpkt);
                av_interleaved_write_frame(out_avformat_ctx, pkt);
                av_packet_free(&pkt);
            });

            av_frame_unref(filt_frame);
        } 
    }

    av_packet_free(&packet);
    av_frame_free(&filt_frame);
    av_write_trailer(out_avformat_ctx);
    ret = 0;   

end:
   avformat_close_input(&in_avformat_ctx);
   if(vDecContext){
       avcodec_close(vDecContext);
   }
   if(vEncContext){
       avcodec_close(vEncContext);
   }
   if (out_avformat_ctx && !(out_avformat_ctx->oformat->flags & AVFMT_NOFILE)){
     avio_closep(&out_avformat_ctx->pb);
   }
   avformat_free_context(out_avformat_ctx);

   return ret;
}


int create_gif_test(){
    const char* inputfile = "./wd_091_tempMovie 6.mov";
    const char* outputfile = "./gen.gif";
    int time = 15;
    int width = 128;
    int height = 128;
    return create_gif(inputfile, outputfile, time, width, height);
}

#endif