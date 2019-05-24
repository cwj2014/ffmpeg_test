#ifndef DECODE_VIDEO_OUTPUT_YUV420_TEST_H_H_
#define DECODE_VIDEO_OUTPUT_YUV420_TEST_H_H_

#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "file_avframe_util.h"
#include "codecimpl.h"


int decode_video_output_yuv420_test(){
    const char * inFileName = "./123.mp4";
    AVFormatContext * avformatctx = nullptr;
    if(avformat_open_input(&avformatctx, inFileName, nullptr, nullptr)<0){
        return -1;
    }
    avformat_find_stream_info(avformatctx, nullptr);
    AVCodecContext** pAVCodecCtx = (AVCodecContext**)malloc(avformatctx->nb_streams*sizeof(AVCodecContext*));
    int audio_stream_index, video_stream_index;
    for(int i=0; i<avformatctx->nb_streams; i++){
        AVStream* stream = avformatctx->streams[i];
        const AVCodec* pCodec =  avcodec_find_decoder(stream->codecpar->codec_id);
        pAVCodecCtx[i] = avcodec_alloc_context3(pCodec);
        avcodec_parameters_to_context(pAVCodecCtx[i], stream->codecpar);

        av_codec_set_pkt_timebase(pAVCodecCtx[i], stream->time_base);
        if(avcodec_open2(pAVCodecCtx[i], pCodec, nullptr)<0){
            return -1;
        } 
        if(pAVCodecCtx[i]->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i;
        }else if(pAVCodecCtx[i]->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
        }  
    }

    FILE* oFile = fopen("./out352x288.yuv", "wb");

    auto callback = [&](AVCodecContext *ctx, const AVFrame* frame){
        if(ctx->codec_type==AVMEDIA_TYPE_VIDEO){
            AVFrame* avFrame = av_frame_clone(frame);
            int num =  1; //分子
            int den = avformatctx->streams[video_stream_index]->r_frame_rate.num;//分母
            avFrame->pts = av_rescale_q(frame->pts, avformatctx->streams[video_stream_index]->time_base, AVRational{num, den});
            printf("pts:%ld 转成 %ld\n", frame->pts, avFrame->pts);
            WriteYUV420ToFile(avFrame, oFile);
        }else if(ctx->codec_type==AVMEDIA_TYPE_AUDIO){
            //printf("解码音频\n");
            int  t_data_size = av_samples_get_buffer_size(
                            NULL, frame->channels,
                            frame->nb_samples,
                            (AVSampleFormat)frame->format,
                            0);
            // if(av_sample_fmt_is_planar((AVSampleFormat)frame->format)){
            //     uint8_t *buf = (uint8_t *)malloc(t_data_size);
            //     interleave(inFrame->data, buf,
            //        inFrame->channels, (AVSampleFormat)inFrame->format, t_data_size);
            // }else{
                
            // }
        }
    };

    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    while(1){
        int ret = av_read_frame(avformatctx, packet);
        if(ret!=0){
            printf("read error or file end\n");
            break;
        }
        decode(pAVCodecCtx[packet->stream_index], packet, callback);
        av_packet_unref(packet);
    }
     
    decode(pAVCodecCtx[video_stream_index], nullptr, callback); 

    
    for(int i=0; i<avformatctx->nb_streams; i++){
        avcodec_close(pAVCodecCtx[i]);
    }
    free(pAVCodecCtx);

    avformat_close_input(&avformatctx);

    av_packet_free(&packet);

    return 0;
}

#endif