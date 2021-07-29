//此例主要学会AVStream结构体以及相关api， AVStream中最重要的变量是AVCodecParamter记录流媒体的相关编码信息
//AVStream在mux下有创建函数avformat_new_stream

extern "C"{
    #include <libavformat/avformat.h>
}
#include <iostream>

void AVStream_Example(){
    const char* ifilename = "/home/caiyu/VID_20210703_171254.mp4";
    const char* ovfilename = "/home/caiyu/VID_20210703_171254_only_video.mp4";
    const char* oafilename = "/home/caiyu/VID_20210703_171254_only_audio.mp4";
    AVFormatContext* ifmt_ctx = NULL;//必须要赋值为NULL,否则avformat_open_input失败
    AVFormatContext* ovfmt_ctx = NULL;
    AVFormatContext* oafmt_ctx = NULL;
    int videoIndex = -1;
    int audioIndex = -1;
    AVStream* ov_stream = NULL;
    AVStream* oa_stream = NULL;
    int ret = 0;
    AVPacket pkt;
    if(avformat_open_input(&ifmt_ctx, ifilename, NULL, NULL) < 0){
        goto end;
    }
    avformat_find_stream_info(ifmt_ctx, NULL);

    for(int i=0; i<ifmt_ctx->nb_streams; i++){
        AVStream* s = ifmt_ctx->streams[i];
        if(s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoIndex = i;
            std::cout << "视频宽：" << s->codecpar->width << "，视频高：" << s->codecpar->height << "，视频码率" << s->codecpar->bit_rate << std::endl;
        }else if(s->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audioIndex = i;
            std::cout << "音频采样率：" << s->codecpar->sample_rate << "音频类型" << s->codecpar->format << "声道数：" << s->codecpar->channels << std::endl;
        }
    }

    if(avformat_alloc_output_context2(&ovfmt_ctx, NULL, NULL, ovfilename) < 0){
        goto end;
    }

    ov_stream = avformat_new_stream(ovfmt_ctx, NULL);
    
    avcodec_parameters_copy(ov_stream->codecpar, ifmt_ctx->streams[videoIndex]->codecpar);
    ov_stream->codecpar->codec_tag = 0;

    if(!ovfmt_ctx->pb && !(ovfmt_ctx->flags & AVFMT_NOFILE)){
        avio_open(&ovfmt_ctx->pb, ovfilename, AVIO_FLAG_WRITE);
    }

    avformat_write_header(ovfmt_ctx, NULL);


    if(avformat_alloc_output_context2(&oafmt_ctx, NULL, NULL, oafilename) < 0){
        goto end;
    }

    oa_stream = avformat_new_stream(oafmt_ctx, NULL);
    avcodec_parameters_copy(oa_stream->codecpar, ifmt_ctx->streams[audioIndex]->codecpar);
    oa_stream->codecpar->codec_tag = 0;

    if(!oafmt_ctx->pb && !(oafmt_ctx->flags & AVFMT_NOFILE)){
        ret = avio_open(&oafmt_ctx->pb, oafilename, AVIO_FLAG_WRITE);
        if(ret < 0){
            goto end;
        }
    }

    if(avformat_write_header(oafmt_ctx, NULL) < 0){
        goto end;
    }

    while(1){
        ret = av_read_frame(ifmt_ctx, &pkt);
        if(ret < 0){
            break;
        }
        if(pkt.stream_index == videoIndex){
           pkt.stream_index = ov_stream->index;
           ret = av_interleaved_write_frame(ovfmt_ctx, &pkt);
        }else if(pkt.stream_index == audioIndex){
        //此处是个坑，如果不对index重新赋值，就会在出现错误
        //    if (pkt->stream_index < 0 || pkt->stream_index >= s->nb_streams) {
        //         av_log(s, AV_LOG_ERROR, "Invalid packet stream index: %d\n",
        //             pkt->stream_index);
        //         return AVERROR(EINVAL);
        //     }
           pkt.stream_index = oa_stream->index;
           ret = av_interleaved_write_frame(oafmt_ctx, &pkt);
        }
        av_packet_unref(&pkt);

        if(ret < 0){
            goto end;
        }
    }

    av_write_trailer(ovfmt_ctx);

    av_write_trailer(oafmt_ctx);
    
end:
    if(ifmt_ctx != NULL){
        avformat_close_input(&ifmt_ctx);
    }
    if(ovfmt_ctx){
        if(ovfmt_ctx->pb && !(ovfmt_ctx->flags & AVFMT_NOFILE)){
            avio_closep(&ovfmt_ctx->pb);
        }
        avformat_free_context(ovfmt_ctx);
        ovfmt_ctx = NULL;
    }
    if(oafmt_ctx != NULL){
        if(oafmt_ctx->pb){
            avio_closep(&oafmt_ctx->pb);
        }
        avformat_free_context(oafmt_ctx);
        oafmt_ctx = NULL;
    }
}