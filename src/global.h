
#ifndef GLOBAL_H_H_
#define GLOBAL_H_H_

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

char av_error[AV_ERROR_MAX_STRING_SIZE] = { 0 };
#define av_err2str(errnum) \
    av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, errnum)


void InitVideoAVCodecCtx(AVCodecContext* c, AVCodecID codecId, int width, int height){
    c->codec_id = codecId;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
// 	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = width;
	c->height = height;
	/* frames per second */
	c->time_base = (AVRational){1, 25};
	c->framerate = (AVRational){25, 1};

	c->gop_size = 12;
	c->max_b_frames = 2;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->qmin = 10;
	c->qmax = 51;
    //如果设置了这个参数，只会有一次pps和sps输出
	//c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	if (c->codec_id == AV_CODEC_ID_H264)
	 	av_opt_set(c->priv_data, "preset", "slow", 0);
}

int InitABufferFilter(AVFilterGraph* filterGraph, AVFilterContext** filterctx, const char* name, 
                        AVRational timebase, int samplerate, AVSampleFormat format, uint64_t channel_layout){
	const AVFilter* bufferfilter = avfilter_get_by_name("abuffer");
    *filterctx = NULL;
    char in_args[512];
    snprintf(in_args, sizeof(in_args),
			"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRId64,
			timebase.num, timebase.den, samplerate,
			av_get_sample_fmt_name(format),
			channel_layout);
    return avfilter_graph_create_filter(filterctx, bufferfilter, name, in_args, NULL, filterGraph);
}

int InitABufferSinkFilter(AVFilterGraph* filterGraph, AVFilterContext** filterctx, const char* name, 
         AVSampleFormat format, int samplerate, uint64_t channel_layout){
	const AVFilter* buffersinkfilter = avfilter_get_by_name("abuffersink");

	AVSampleFormat out_sample_fmts[2];
	out_sample_fmts[0]= format;
	out_sample_fmts[1] = AV_SAMPLE_FMT_NONE;

    int64_t out_channel_layouts[2];
	out_channel_layouts[0] = channel_layout;
	out_channel_layouts[1] = -1;

    int out_sample_rates[2];
	out_sample_rates[0] = samplerate;
	out_sample_rates[1] = -1;

    int ret = avfilter_graph_create_filter(filterctx, buffersinkfilter, name, NULL, NULL, filterGraph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        
    }
	do{
		ret = av_opt_set_int_list(*filterctx, "sample_fmts", out_sample_fmts, -1,
								AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
			break;
		}
		ret = av_opt_set_int_list(*filterctx, "channel_layouts", out_channel_layouts, -1,
								AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
			break;
		}
		ret = av_opt_set_int_list(*filterctx, "sample_rates", out_sample_rates, -1,
								AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
			break;
		}
	}while(0);
	
	return ret;
}


uint8_t* getNulu(const uint8_t* source, uint64_t& start_pos, uint64_t end_pos, uint64_t& nulu_size){
    nulu_size = 0;
    uint64_t pos = start_pos;
    start_pos = 0;
    while(pos < end_pos - 4){
        if(source[pos]== 0x00 && source[pos+1] == 0x00){
            if(source[pos+2] == 0x01){
                start_pos = pos + 3;
            }else if(source[pos+2] == 0x00 && source[pos+3] == 0x01){
                start_pos = pos + 4;
            }
        }
        if(start_pos != 0){
            break;
        }
        pos += 1;
    }
    uint8_t* nulu = nullptr;
    if(start_pos != 0){
        pos = start_pos;
        while(pos < end_pos - 4){
            if(source[pos]== 0x00 && source[pos+1] == 0x00){
                if(source[pos+2] == 0x01){
                   break;
                }else if(source[pos+2] == 0x00 && source[pos+3] == 0x01){
                    break;
                }
            }
            pos += 1;
            nulu_size += 1;
        }
        if(pos >= end_pos - 4){//last nulu
             nulu_size += end_pos - pos; 
        }
        nulu = new uint8_t[nulu_size];
        memcpy(nulu, source + start_pos, nulu_size);
        start_pos += nulu_size;
    }
    return nulu;
}

#endif