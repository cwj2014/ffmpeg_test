#ifndef REMUXING_TEST_H_H_
#define REMUXING_TEST_H_H_

#include "global.h"
#include <map>

struct StreamInfo{
    AVStream* in;
    AVStream* out;
};
//本例子是将mp4容器的音视频文件原码转换成flv格式的音视频
int remuxing_test(){
    const char* inFileName = "./半壶纱.mp4";
    //const char* inFileName = "./akiyo_cif.mp4";
    const char* outFileName = "./半壶纱.flv";
    int ret = -1;
    AVFormatContext* in_avformat_ctx = nullptr;
    AVFormatContext* out_avformat_ctx = nullptr;
    std::map<int, StreamInfo> infoMap;
    AVPacket * pkt;
    //带goto语言的变量都需要提前定义如下
    if(avformat_open_input(&in_avformat_ctx, inFileName, nullptr, nullptr) < 0){
        goto end;
    }
    if(avformat_find_stream_info(in_avformat_ctx, nullptr)){
        goto end;
    }
    if(avformat_alloc_output_context2(&out_avformat_ctx, nullptr, nullptr, outFileName) < 0){
        goto end;
    }
    
    for(int i=0; i<in_avformat_ctx->nb_streams; i++){
        AVStream* stream = in_avformat_ctx->streams[i];
        if(stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
           stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
           stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE){
               continue;
        }
        AVStream* out = avformat_new_stream(out_avformat_ctx, nullptr);
        avcodec_parameters_copy(out->codecpar, stream->codecpar);
        //下边这个参数很重要，如果不重置这个参数会出现类似于Tag avc1 incompatible with output codec id '27' ([7][0][0][0])这个的错误
        out->codecpar->codec_tag = 0;
        infoMap.insert(std::pair<int, StreamInfo>(i, StreamInfo{stream,out}));
    }
     
    for(auto &iter: infoMap){
        StreamInfo streamInfo = iter.second;
        printf("in:分母%d,分子:%d, out:分母%d,分子:%d\n", 
        streamInfo.in->time_base.den, streamInfo.in->time_base.num, streamInfo.out->time_base.den, streamInfo.out->time_base.num);
    } 
    

    if (!(out_avformat_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_avformat_ctx->pb, outFileName, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", outFileName);
            goto end;
        }
    }

    if(avformat_write_header(out_avformat_ctx, nullptr) < 0){
        goto end;
    }

    for(auto &iter: infoMap){
        StreamInfo streamInfo = iter.second;
        printf("in:分母%d,分子:%d, out:分母%d,分子:%d\n", 
        streamInfo.in->time_base.den, streamInfo.in->time_base.num, streamInfo.out->time_base.den, streamInfo.out->time_base.num);
    } 
    

    pkt = av_packet_alloc();
    while(1){
        ret = av_read_frame(in_avformat_ctx, pkt);
         if(ret!=0){
            printf("read error or file end\n");
            break;
        }
        auto iter = infoMap.find(pkt->stream_index);
        if(iter == infoMap.end()){
             av_packet_unref(pkt);
             continue;
        }
        AVStream *in_stream = iter->second.in;
        AVStream *out_stream = iter->second.out;
        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;

        ret = av_interleaved_write_frame(out_avformat_ctx, pkt);
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
        av_packet_unref(pkt);
    }

    //ret = av_interleaved_write_frame(out_avformat_ctx, nullptr);
    // if (ret < 0) {
    //     fprintf(stderr, "Error muxing packet\n");
    // }

    av_write_trailer(out_avformat_ctx);
    
    av_packet_free(&pkt);
    
    ret = 0;
end:
    avformat_close_input(&in_avformat_ctx);

    /* close output */
    if (out_avformat_ctx && !(out_avformat_ctx->flags & AVFMT_NOFILE))
        avio_closep(&out_avformat_ctx->pb);
    avformat_free_context(out_avformat_ctx);

    return ret;
}
#endif