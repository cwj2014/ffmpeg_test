#ifndef ENCODE_VIDEO_OUTPUT_MP3_TEST_H_H_
#define ENCODE_VIDEO_OUTPUT_MP3_TEST_H_H_

#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "file_avframe_util.h"
#include "codecimpl.h"

int encode_video_output_mp4_test(){

    FILE* rFile = fopen("./akiyo_cif.yuv","rb");
	if(rFile == NULL)
		return -1;
	const char * oFileName = "./akiyo_cif.mp4";
    int width = 352;
    int height = 288;

    AVFormatContext * avformatctx;
    if(avformat_alloc_output_context2(&avformatctx, nullptr, nullptr, oFileName)<0){
        return -1;
    }
    AVStream* vStream = avformat_new_stream(avformatctx, nullptr);
    if(vStream == nullptr){
        return -1;
    }
    
    printf("index:%d, n: %d\n",vStream->index, avformatctx->nb_streams);

    if(avio_open(&avformatctx->pb, oFileName, AVIO_FLAG_READ_WRITE)<0){
        return -1;
    }

    const AVCodec *codec = avcodec_find_encoder(avformatctx->oformat->video_codec);

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
    InitVideoAVCodecCtx(pCodecCtx, avformatctx->oformat->video_codec, width, height);
    
    int ret = avcodec_open2(pCodecCtx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}
    avcodec_parameters_from_context(vStream->codecpar, pCodecCtx);
    //vStream->time_base = AVRational{1, 25000};

    auto callback = [&](AVCodecContext* ctx,const AVPacket* avpkt){
            
            AVPacket* pkt = av_packet_clone(avpkt);
            av_packet_rescale_ts(pkt, pCodecCtx->time_base, vStream->time_base);
            pkt->stream_index = vStream->index;
            av_write_frame(avformatctx, pkt);
            av_packet_unref(pkt);
            av_packet_free(&pkt);
        };

    if(avformat_write_header(avformatctx, nullptr)<0){
        return -1;
    }
    AVFrame* inframe = av_frame_alloc();
    int i=0;
    while(1){
		if(feof(rFile))
			break;

		inframe->format = AV_PIX_FMT_YUV420P;
		inframe->width  = width;
		inframe->height = height;
		av_frame_get_buffer(inframe, 32);

		ReadYUV420FromFile(inframe, rFile);//从yuv文件填充AVFrame
        inframe->pts = i++;
        encode(pCodecCtx, inframe, callback);

        av_frame_unref(inframe);
    }
    
    encode(pCodecCtx, nullptr, callback);

    av_write_trailer(avformatctx);

    avcodec_close(pCodecCtx);
    av_frame_free(&inframe);

    avio_close(avformatctx->pb);
	avformat_free_context(avformatctx);
 
	fclose(rFile);
    printf("完成输出\n");
    return 0;
}

#endif