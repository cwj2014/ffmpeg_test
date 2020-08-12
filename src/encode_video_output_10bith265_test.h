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

#define NO_VIDEO_FILTER 1

int encode_video_output_h265_test2(){

#if NO_VIDEO_FILTER
    const char *inFileName = "./output.yuv";
#else
    const char* inFileName = "./akiyo_cif.yuv";
#endif
    FILE* rFile = fopen(inFileName,"rb");
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

#if NO_VIDEO_FILTER
#else
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
#endif

    auto callback = [&](AVCodecContext* ctx,const AVPacket* avpkt){
            uint64_t start_pos = 0;
            uint64_t end_pos = avpkt->size;
            uint64_t nulu_size;
            uint8_t* source = avpkt->data;
            uint8_t* nulu_data;
            int nuluNum = 0;
            uint8_t head[4] = {0x00, 0x00, 0x00, 0x01};
            while((nulu_data = getNulu(source, start_pos, end_pos, nulu_size)) != nullptr){
                nuluNum++;
                int nulu_type = (nulu_data[0] & 0x7E)>>1;
                // NAL_BLA_W_LP = 16, 
                // NAL_BLA_W_RADL = 17, 
                // NAL_BLA_N_LP = 18, 
                // NAL_IDR_W_RADL = 19, 
                // NAL_IDR_N_LP = 20, 
                // NAL_CRA_NUT = 21, 
                // NAL_VPS = 32, 
                // NAL_SPS = 33, 
                // NAL_PPS = 34,
                if(nulu_type == 32){
                    printf("VPS\n");
                }else if(nulu_type == 33){
                    printf("SPS\n");
                }else if(nulu_type == 34){
                    printf("PPS\n");
                }else if(nulu_type == 39 || nulu_type == 40){
                    printf("SEI:%d\n", nulu_type);
                }else if(nulu_type == 16 || nulu_type == 17 || nulu_type == 18 ||
                        nulu_type == 19 || nulu_type == 20 || nulu_type == 21){
                        printf("I Frame, type: %d\n", nulu_type);
                }else{
                    printf("############%d###########\n", nulu_type);
                }
                fwrite(head, 1, 4, oFile);
                fwrite(nulu_data, 1, nulu_size, oFile);
                delete []nulu_data;
            }
            printf("-----------------%d-----------\n", avpkt->pts);
        };

    AVFrame* inframe = av_frame_alloc();
#if NO_VIDEO_FILTER
#else
    AVFrame *filt_frame = av_frame_alloc();
#endif
    int i=0;
    while(1){
		if(feof(rFile))
			break;

		
#if NO_VIDEO_FILTER
    inframe->format = AV_PIX_FMT_YUV420P10LE;
	inframe->width  = width;
	inframe->height = height;
	av_frame_get_buffer(inframe, 32);
    ReadYUV420P10LEFromFile(inframe, rFile);//从yuv文件填充AVFrame
    inframe->pts = i++;
    encode(pCodecCtx, inframe, callback);
#else
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
#endif
		
        
        av_frame_unref(inframe);
    }
#if NO_VIDEO_FILTER
#else
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
#endif

    
    encode(pCodecCtx, nullptr, callback);

    avcodec_close(pCodecCtx);
#if NO_VIDEO_FILTER
#else
    av_frame_free(&filt_frame);
#endif
    av_frame_free(&inframe);
	fclose(rFile);
    fclose(oFile);
    printf("完成输出\n");
    return 0;
}

#endif