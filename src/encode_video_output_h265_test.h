#ifndef ENCODE_VIDEO_OUTPUT_H265_TEST_H_H_
#define ENCODE_VIDEO_OUTPUT_H265_TEST_H_H_
#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "avframe_util.h"
#include "codecimpl.h"


int encode_video_output_h265_test(){

    FILE* rFile = fopen("./akiyo_cif.yuv","rb");
	if(rFile == NULL)
		return -1;
	const char * oFileName = "./test1.h265";
    FILE* oFile = fopen(oFileName, "wb");
    if(rFile == NULL)
        return -1;
    int width = 352;
    int height = 288;


    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H265);

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
    InitVideoAVCodecCtx(pCodecCtx, AV_CODEC_ID_H265, width, height);
    
    int ret = avcodec_open2(pCodecCtx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

    auto callback = [&](AVCodecContext* ctx,const AVPacket* avpkt){
            
            fwrite(avpkt->data,avpkt->size,1,oFile);

            int flags = avpkt->flags;
            printf("avpkt->flag:%d, 0x%x%x%x%x%x\n",  flags & AV_PKT_FLAG_KEY, avpkt->data[0], avpkt->data[1], avpkt->data[2], avpkt->data[3], avpkt->data[4]);
        };

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


    avcodec_close(pCodecCtx);
    av_frame_free(&inframe);
 
	fclose(rFile);
    fclose(oFile);
    
    printf("完成输出\n");
    return 0;
}

#endif