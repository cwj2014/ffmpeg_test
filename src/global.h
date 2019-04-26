
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

	c->gop_size = 10;
	c->max_b_frames = 0;
	c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->qmin = 10;
	c->qmax = 51;
    //如果设置了这个参数，只会有一次pps和sps输出
	//c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	if (c->codec_id == AV_CODEC_ID_H264)
	 	av_opt_set(c->priv_data, "preset", "slow", 0);
}

#endif