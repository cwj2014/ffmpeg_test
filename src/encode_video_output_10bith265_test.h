#ifndef ENCODE_VIDEO_OUTPUT_10BITH265_TEST_H_H_
#define ENCODE_VIDEO_OUTPUT_10BITH265_TEST_H_H_
#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "avframe_util.h"
#include "codecimpl.h"
#include "video_filter_tool.h"


int encode_video_output_h265_test2(){

    FILE* rFile = fopen("./akiyo_cif.yuv","rb");
	if(rFile == NULL)
		return -1;
	const char * oFileName = "./test2.h265";
    FILE* oFile = fopen(oFileName, "wb");
    if(rFile == NULL)
        return -1;
    int width = 352;
    int height = 288;


    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H265);

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
    InitVideoAVCodecCtx(pCodecCtx, AV_CODEC_ID_H265, width, height);
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P10LE;
    
    int ret = avcodec_open2(pCodecCtx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

    std::shared_ptr<VideoFilterManager> videoFilter = std::make_shared<VideoFilterManager>();
    AVFilterContext *buffersrcCtx, *buffersinkCtx;

    {//配置filter块
        char in_args[512];
	    AVPixelFormat pix_fmts = AV_PIX_FMT_YUV420P;
        AVRational timebase = {
            .num = 1,
            .den = 25
        };
	    snprintf(in_args, sizeof(in_args),"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", 
            width, height, AV_PIX_FMT_YUV420P, timebase.num, timebase.den, 1,1);
        
        buffersrcCtx = videoFilter->CreateBufferFilter(in_args, "in"); 
        enum AVPixelFormat dest_pix_fmts[] = { AV_PIX_FMT_YUV420P10LE, AV_PIX_FMT_NONE };
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
        snprintf(filter_desc, sizeof(filter_desc),"format=pix_fmts=%d", AV_PIX_FMT_YUV420P10LE);
        if(videoFilter->InsertFilter(inputs, outputs, filter_desc) < 0){
            av_log(nullptr, AV_LOG_ERROR, "parse filter graph error\n");
            exit(1);
        }
    }

    if(!videoFilter->FilterConfig()){
        exit(1);
    }

    auto callback = [&](AVCodecContext* ctx,const AVPacket* avpkt){
            
            fwrite(avpkt->data,avpkt->size,1,oFile);

            int flags = avpkt->flags;
            printf("avpkt->flag:%d, 0x%x%x%x%x%x\n",  flags & AV_PKT_FLAG_KEY, avpkt->data[0], avpkt->data[1], avpkt->data[2], avpkt->data[3], avpkt->data[4]);
        };

    AVFrame* inframe = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    int i=0;
    while(1){
		if(feof(rFile))
			break;

		inframe->format = AV_PIX_FMT_YUV420P;
		inframe->width  = width;
		inframe->height = height;
		av_frame_get_buffer(inframe, 32);

		ReadYUV420FromFile(inframe, rFile);//从yuv文件填充AVFrame
        videoFilter->AddFrame(buffersrcCtx, inframe);

        while (1) {//从buffersink设备上下文获取视频帧
            ret = videoFilter->GetFrame(buffersinkCtx, filt_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                break;
            
            filt_frame->pts = i++;
            encode(pCodecCtx, filt_frame, callback);

            av_frame_unref(filt_frame);
        } 
        av_frame_unref(inframe);
    }
    if ((ret = videoFilter->AddFrame(buffersrcCtx, NULL)) >= 0) {
        while (1) {//从buffersink设备上下文获取视频帧
            ret = videoFilter->GetFrame(buffersinkCtx, filt_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                break;
            
            filt_frame->pts = i++;
            encode(pCodecCtx, filt_frame, callback);

            av_frame_unref(filt_frame);
        } 
    }

    
    encode(pCodecCtx, nullptr, callback);

    avcodec_close(pCodecCtx);
    av_frame_free(&filt_frame);
    av_frame_free(&inframe);
	fclose(rFile);
    fclose(oFile);
    printf("完成输出\n");
    return 0;
}

#endif