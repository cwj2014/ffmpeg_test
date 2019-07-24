#ifndef DECODEC_AUDIO_MIX_OUTPUT_PCM_TEST_H_H_
#define DECODEC_AUDIO_MIX_OUTPUT_PCM_TEST_H_H_

#include "global.h"
#include "codecimpl.h"
#include "file_avframe_util.h"

//duration 0:first 1:longest: 2: shortest;
static const int mix_flags = 0;

//编码两个音频文件并混音输出PCM文件
void decode_mix_audio_test(){
    const char* first_input_audiofile = "./半壶纱.mp4";
    const char* second_input_audiofile = "./V90405-190106.mp4";
    AVFormatContext *firstAVFormatCtx = NULL;
    AVFormatContext *secondAVFormatCtx = NULL;
    if(avformat_open_input(&firstAVFormatCtx, first_input_audiofile, NULL, NULL)){
        return;
    }
    if(avformat_open_input(&secondAVFormatCtx, second_input_audiofile, NULL, NULL)){
        return;
    }
    int first_input_audio_index = av_find_best_stream(firstAVFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    int second_input_audio_index = av_find_best_stream(secondAVFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(first_input_audio_index < 0 || second_input_audio_index < 0)
        return;
    AVStream* firstAudioStream = firstAVFormatCtx->streams[first_input_audio_index];
    const AVCodec *fisrtAudioCodec = avcodec_find_decoder(firstAudioStream->codecpar->codec_id);
    AVCodecContext* firstAVCodecCtx = avcodec_alloc_context3(fisrtAudioCodec);
    avcodec_parameters_to_context(firstAVCodecCtx, firstAudioStream->codecpar);
    if(avcodec_open2(firstAVCodecCtx, fisrtAudioCodec, NULL) < 0)
        return;

    AVStream* secondAudioStream = secondAVFormatCtx->streams[second_input_audio_index];
    const AVCodec *secondAudioCodec = avcodec_find_decoder(secondAudioStream->codecpar->codec_id);
    AVCodecContext* secondAVCodecCtx = avcodec_alloc_context3(secondAudioCodec);
    avcodec_parameters_to_context(secondAVCodecCtx, secondAudioStream->codecpar);
    if(avcodec_open2(secondAVCodecCtx, secondAudioCodec, NULL) < 0)
        return;
    bool first_file_longest =  firstAudioStream->duration > secondAudioStream->duration;
    ///////////////////////////////////////////////////结束准备解码相关内容/////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////准备mix filter/////////////////////////////////////////////////////////////////////////////////////////
    AVFilterGraph* filterGraph = avfilter_graph_alloc();
    char in_args[1024];

    // const AVFilter* src1Filter = avfilter_get_by_name("abuffer");
    // AVFilterContext* src1FilterCtx = NULL;
    // snprintf(in_args, sizeof(in_args),
	// 		"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRId64,
	// 		firstAVCodecCtx->time_base.num, firstAVCodecCtx->time_base.den, firstAVCodecCtx->sample_rate,
	// 		av_get_sample_fmt_name(firstAVCodecCtx->sample_fmt),
	// 		firstAVCodecCtx->channel_layout);
    // printf("第一个文件音频信息：%s\n", in_args);
    // avfilter_graph_create_filter(&src1FilterCtx, src1Filter, "src1", in_args, NULL, filterGraph);

    // const AVFilter* src2Filter = avfilter_get_by_name("abuffer");
    // AVFilterContext* src2FilterCtx = NULL;
    // snprintf(in_args, sizeof(in_args),
	// 		"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRId64,
	// 		secondAVCodecCtx->time_base.num, secondAVCodecCtx->time_base.den, secondAVCodecCtx->sample_rate,
	// 		av_get_sample_fmt_name(secondAVCodecCtx->sample_fmt),secondAVCodecCtx->channel_layout);
    // printf("第二个文件音频信息：%s\n", in_args);
    // avfilter_graph_create_filter(&src2FilterCtx, src2Filter, "src2", in_args, NULL, filterGraph);

    // const AVFilter* sinkFilter = avfilter_get_by_name("abuffersink");
    // AVFilterContext* sinkFilterCtx = NULL;
    // static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_FLTP, AV_SAMPLE_FMT_NONE };
    // static const int64_t out_channel_layouts[] = {AV_CH_LAYOUT_STEREO, -1 };
    // static const int out_sample_rates[] = {44100, -1 };
    // int ret = avfilter_graph_create_filter(&sinkFilterCtx, sinkFilter, "out", NULL, NULL, filterGraph);
    // if (ret < 0) {
    //     av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        
    // }
    // ret = av_opt_set_int_list(sinkFilterCtx, "sample_fmts", out_sample_fmts, -1,
    //                           AV_OPT_SEARCH_CHILDREN);
    // if (ret < 0) {
    //     av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
  
    // }
    // ret = av_opt_set_int_list(sinkFilterCtx, "channel_layouts", out_channel_layouts, -1,
    //                           AV_OPT_SEARCH_CHILDREN);
    // if (ret < 0) {
    //     av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
    // }
    // ret = av_opt_set_int_list(sinkFilterCtx, "sample_rates", out_sample_rates, -1,
    //                           AV_OPT_SEARCH_CHILDREN);
    // if (ret < 0) {
    //     av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
    // }

    AVFilterContext* src1FilterCtx = NULL;
    AVFilterContext* src2FilterCtx = NULL;
    AVFilterContext* sinkFilterCtx = NULL;

    int ret = InitABufferFilter(filterGraph, &src1FilterCtx, "src1", firstAVCodecCtx->time_base, firstAVCodecCtx->sample_rate, 
                                firstAVCodecCtx->sample_fmt, firstAVCodecCtx->channel_layout);
    ret = InitABufferFilter(filterGraph, &src2FilterCtx, "src2", secondAVCodecCtx->time_base, secondAVCodecCtx->sample_rate, 
                                secondAVCodecCtx->sample_fmt, secondAVCodecCtx->channel_layout);

    ret = InitABufferSinkFilter(filterGraph, &sinkFilterCtx, "out", AV_SAMPLE_FMT_FLTP, 16000, AV_CH_LAYOUT_STEREO);

    
    //////////////////////////////////////////////////将输入音频先转换成同样的采样率、样本格式、通道数///////////////////////////////////////////////////////////
    // static const AVSampleFormat pre_mix_sample_fmt = AV_SAMPLE_FMT_FLTP;
    // static const int64_t pre_mix_channel_layout = AV_CH_LAYOUT_STEREO;
    // static const int pre_sample_rate = 32000;
    // const AVFilter* aformat = avfilter_get_by_name("aformat");
    // snprintf(in_args, sizeof(in_args),
	// 		"sample_fmts=%s:sample_rates=%d:channel_layouts=0x%"PRId64,
	// 		av_get_sample_fmt_name(pre_mix_sample_fmt), pre_sample_rate, pre_mix_channel_layout);
    // AVFilterContext* aformatFilterCtx1 = NULL;
    // ret = avfilter_graph_create_filter(&aformatFilterCtx1, aformat, "aformat1", in_args, NULL, filterGraph);

    // AVFilterContext* aformatFilterCtx2 = NULL;
    // ret = avfilter_graph_create_filter(&aformatFilterCtx2, aformat, "aformat2", in_args, NULL, filterGraph);

     
    const AVFilter* mixFilter = avfilter_get_by_name("amix");
    AVFilterContext* mixFilterCtx = NULL;
    //inputs=3:duration=first:dropout_transition=3
    bool first_file_end_exit;
    char duration[20];
    if(mix_flags == 0){
        strcpy(duration, "first");
        first_file_end_exit = true;
    }else if(mix_flags == 1){
        strcpy(duration, "longest");
        first_file_end_exit = first_file_longest ? true : false;
    }else if(mix_flags == 2){
        strcpy(duration, "shortest");
        first_file_end_exit = first_file_longest ? false : true;
    }else{
        return;
    }
    snprintf(in_args, sizeof(in_args),"inputs=%d:duration=%s:dropout_transition=%d", 2, duration, 3);
    ret = avfilter_graph_create_filter(&mixFilterCtx, mixFilter, "amix", in_args, NULL, filterGraph);

    
    // ret = avfilter_link(src1FilterCtx, 0, aformatFilterCtx1, 0);
    // ret = avfilter_link(src2FilterCtx, 0, aformatFilterCtx2, 0);

    // ret = avfilter_link(aformatFilterCtx1, 0, mixFilterCtx, 0);
    // ret = avfilter_link(aformatFilterCtx2, 0, mixFilterCtx, 1);
    
    ret = avfilter_link(src1FilterCtx, 0, mixFilterCtx, 0);
    ret = avfilter_link(src2FilterCtx, 0, mixFilterCtx, 1);
    ret = avfilter_link(mixFilterCtx, 0, sinkFilterCtx, 0);

    ret = avfilter_graph_config(filterGraph, NULL);
    if(ret < 0)
        return;
    ////////////////////////////////////////////////////开始解码混合///////////////////////////////////////////////////////////////////////
    
    FILE* pcmFile = fopen("./mix_pcm2.pcm", "wb");

    AVPacket avpkt1, avpkt2;
    av_init_packet(&avpkt1);
    av_init_packet(&avpkt2);
    AVFrame *filt_frame = av_frame_alloc();
    bool exit = false;
    while(!exit){
        ret = av_read_frame(firstAVFormatCtx, &avpkt1);
        if(ret == 0){
            if(avpkt1.stream_index == first_input_audio_index){
                ret = decode(firstAVCodecCtx, &avpkt1, [&](AVCodecContext* ctx, const AVFrame* avframe){
                    AVFrame* inframe = av_frame_clone(avframe);
                    if (av_buffersrc_add_frame_flags(src1FilterCtx, inframe, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    }
                    av_frame_unref(inframe);
                });
                if(ret < 0){
                    printf("11111 decode avpkt1 failed\n");
                }
            }
            av_packet_unref(&avpkt1);
            
        }else{
            ret = decode(firstAVCodecCtx, nullptr, [&](AVCodecContext* ctx, const AVFrame* avframe){
                AVFrame* inframe = av_frame_clone(avframe);
                if (av_buffersrc_add_frame_flags(src1FilterCtx, inframe, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
			        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
		        }
                av_frame_unref(inframe);
            });
            if(first_file_end_exit){
                exit = true;
            }
        }
        ret = av_read_frame(secondAVFormatCtx, &avpkt2);
        if(ret == 0){
            if(avpkt2.stream_index == second_input_audio_index){
                ret = decode(secondAVCodecCtx, &avpkt2, [&](AVCodecContext* ctx, const AVFrame* avframe){
                    AVFrame* inframe = av_frame_clone(avframe);
                    if (av_buffersrc_add_frame_flags(src2FilterCtx, inframe, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                    }
                    av_frame_unref(inframe);
                });
                if(ret < 0){
                    printf("11111 decode avpkt2 failed\n");
                }
            }
            av_packet_unref(&avpkt2);
        }else{
            ret = decode(secondAVCodecCtx, nullptr, [&](AVCodecContext* ctx, const AVFrame* avframe){
                AVFrame* inframe = av_frame_clone(avframe);
                if (av_buffersrc_add_frame_flags(src2FilterCtx, inframe, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
			        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
		        }
                av_frame_unref(inframe);
            });
            if(!first_file_end_exit){
                exit = true;
            }
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

    avformat_close_input(&firstAVFormatCtx);
    avformat_close_input(&secondAVFormatCtx);
    avcodec_close(firstAVCodecCtx);
    avcodec_close(secondAVCodecCtx);
    av_frame_free(&filt_frame);
	avfilter_graph_free(&filter_graph);

}

#endif