#ifndef AUDIO_FILTER_ARESAMPLE_OUTPUT_PCM_H_H_
#define AUDIO_FILTER_ARESAMPLE_OUTPUT_PCM_H_H_

#include "global.h"
#include "codecimpl.h"
#include "avframe_util.h"

void audio_filter_aresample_test(){
    const char* input_file = "./V90405-190106.mp4";
    AVFormatContext * inputFormatContext = NULL;
    if(avformat_open_input(&inputFormatContext, input_file, NULL, NULL) < 0){
        return;
    }
    int audio_index = av_find_best_stream(inputFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audio_index < 0){
        return;
    }
    AVStream* stream = inputFormatContext->streams[audio_index];
    const AVCodec *avcodec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVCodecContext* avcodec_ctx = avcodec_alloc_context3(avcodec);
    avcodec_parameters_to_context(avcodec_ctx, stream->codecpar);
    if(avcodec_open2(avcodec_ctx, avcodec, NULL) < 0){
        return;
    }

    AVFilterGraph* filtergraph = avfilter_graph_alloc();

    
    AVFilterContext* srcFilterCtx = NULL;
    int ret = InitABufferFilter(filtergraph, &srcFilterCtx, "src", avcodec_ctx->time_base, avcodec_ctx->sample_rate, 
                                avcodec_ctx->sample_fmt, avcodec_ctx->channel_layout);

    static const AVSampleFormat pre_mix_sample_fmt = AV_SAMPLE_FMT_S16;
    static const int64_t pre_mix_channel_layout = AV_CH_LAYOUT_STEREO;
    static const int pre_sample_rate = 32000;

    AVFilterContext* sinkFilterCtx = NULL;
    ret = InitABufferSinkFilter(filtergraph, &sinkFilterCtx, "out", pre_mix_sample_fmt, pre_sample_rate, pre_mix_channel_layout);
     
    // char in_args[512];
    // const AVFilter* aresample =  avfilter_get_by_name("aresample");
    // snprintf(in_args, sizeof(in_args),"%d", pre_sample_rate);
    // AVFilterContext* aresampleFilterCtx = NULL;
    // ret = avfilter_graph_create_filter(&aresampleFilterCtx, aresample, "aformat", in_args, NULL, filtergraph);

    // ret = avfilter_link(srcFilterCtx, 0, aresampleFilterCtx, 0);
    // ret = avfilter_link(aresampleFilterCtx, 0, sinkFilterCtx, 0);

    ret = avfilter_link(srcFilterCtx, 0, sinkFilterCtx, 0);

    ret = avfilter_graph_config(filtergraph, NULL);
    if(ret < 0)
        return;


    FILE* pcmFile = fopen("./aresample.pcm", "wb");

    AVPacket avpkt;
    av_init_packet(&avpkt);
    AVFrame *filt_frame = av_frame_alloc();
    bool exit = false;
    while(!exit){
        ret = av_read_frame(inputFormatContext, &avpkt);
        if(ret == 0){
            if(avpkt.stream_index == audio_index){
                ret = decode(avcodec_ctx, &avpkt, [&](AVCodecContext* ctx, const AVFrame* avframe){
                    AVFrame* inframe = av_frame_clone(avframe);
                    if (av_buffersrc_add_frame_flags(srcFilterCtx, inframe, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    }
                    av_frame_unref(inframe);
                });
                if(ret < 0){
                    printf("11111 decode avpkt1 failed\n");
                }
            }
            av_packet_unref(&avpkt);
            
        }else{
            ret = decode(avcodec_ctx, nullptr, [&](AVCodecContext* ctx, const AVFrame* avframe){
                AVFrame* inframe = av_frame_clone(avframe);
                if (av_buffersrc_add_frame_flags(srcFilterCtx, inframe, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
			        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
		        }
                av_frame_unref(inframe);
            });
            exit = true;;
        }
        
        while (1) {//从buffersink设备上下文获取视频帧
		   ret = av_buffersink_get_frame(sinkFilterCtx, filt_frame);
		   if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
			   break;
           }
		   if (ret < 0){
               exit = true;
			   break;
           }
	       WritePCMToFile(filt_frame, pcmFile);//将处理后的AVFrame写入到文件
		   av_frame_unref(filt_frame);
	   }
    }

    avformat_close_input(&inputFormatContext);
    avcodec_close(avcodec_ctx);
    av_frame_free(&filt_frame);
	avfilter_graph_free(&filtergraph);
}

#endif