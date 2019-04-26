#ifndef SEPARATE_MP4_OUTPUT_AUDIO_VIDEO_MP4_H_H_
#define SEPARATE_MP4_OUTPUT_AUDIO_VIDEO_MP4_H_H_

#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

    
//分离mp4文件并分别保存音频mp4文件和视频mp4文件
int separate_mp4_output_audio_video_mp4_test(){
    const char * inFileName = "./半壶纱.mp4";
    AVFormatContext * avformatctx;
    if(avformat_open_input(&avformatctx, inFileName, nullptr, nullptr)<0){
        return -1;
    }
    avformat_find_stream_info(avformatctx, nullptr);

    const char* outVideoFileName = "./video_output.mp4";
    const char* outAudioFileName = "./audio_output.mp4";

    AVFormatContext* videoFormatCtx;
    AVFormatContext* audioFormatCtx;

    if(avformat_alloc_output_context2(&videoFormatCtx, nullptr, nullptr, outVideoFileName)){
        return -1;
    }

    if(avformat_alloc_output_context2(&audioFormatCtx, nullptr, nullptr, outAudioFileName)){
        return -1;
    }

    AVStream* audioStream = avformat_new_stream(audioFormatCtx, nullptr);
    AVStream* videoStream = avformat_new_stream(videoFormatCtx, nullptr);

    int audio_stream_index, video_stream_index;
    for(int i=0; i<avformatctx->nb_streams; i++){
        AVStream* stream = avformatctx->streams[i];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i;
            avcodec_parameters_copy(videoStream->codecpar, stream->codecpar);
        }else if(stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            avcodec_parameters_copy(audioStream->codecpar, stream->codecpar);
            audio_stream_index = i;
        }  
    }
    

    if(avio_open(&videoFormatCtx->pb, outVideoFileName, AVIO_FLAG_WRITE)<0){
        return -1;
    }
    if(avio_open(&audioFormatCtx->pb, outAudioFileName, AVIO_FLAG_WRITE)<0){
        return -1;
    }

    avformat_write_header(videoFormatCtx, nullptr);
    avformat_write_header(audioFormatCtx, nullptr);
    
    AVPacket* pkt = av_packet_alloc();
    while(1){
        int ret = av_read_frame(avformatctx, pkt);
        if(ret!=0){
            break;
        }
        AVStream* in = avformatctx->streams[pkt->stream_index];
        AVStream* out = nullptr;
        AVFormatContext* outCtx;
        if(pkt->stream_index == video_stream_index){
            pkt->stream_index = 0;
            out = videoStream;
            outCtx = videoFormatCtx;
        }else if(pkt->stream_index == audio_stream_index){
            pkt->stream_index = 0;
            out = audioStream;
            outCtx = audioFormatCtx;
        }
        int64_t pts = pkt->pts;
        int64_t dts = pkt->dts;
        int64_t dur = pkt->duration;
        pkt->pts = av_rescale_q_rnd(pkt->pts, in->time_base, out->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, in->time_base, out->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, in->time_base, out->time_base);
        pkt->pos = -1;
        //printf("old_pts:%d, pkt->pts:%ld, old_dts:%ld, pkt->dts:%ld, old_duration:%ld, pkt->duration:%ld\n", pts, pkt->pts, dts, pkt->dts, dur, pkt->duration);
        av_interleaved_write_frame(outCtx, pkt);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);

    av_write_trailer(audioFormatCtx);
    av_write_trailer(videoFormatCtx);

    avio_close(videoFormatCtx->pb);
    avio_close(audioFormatCtx->pb);

    avformat_free_context(avformatctx);
    avformat_free_context(audioFormatCtx);
    avformat_free_context(videoFormatCtx);

    return 0;
}

#endif