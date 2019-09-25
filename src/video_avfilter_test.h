#ifndef VIDEO_AVFILTER_TEST_H_H_
#define VIDEO_AVFILTER_TEST_H_H_

#include <cstdio>
#include "global.h"
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "avframe_util.h"


int video_avfilter_test(){

	FILE* rFile = fopen("./akiyo_cif.yuv","rb");
	if(rFile == NULL)
		return -1;
	FILE* wFile = fopen("./akiyo_qcif.yuv","wb");

    int width = 352;
    int height = 288;

    //创建filtergraph
	AVFilterGraph *filter_graph = avfilter_graph_alloc();
	/*************************************************buffer过滤器***************************************************/
	//根据名字获取buffer过滤器
	const AVFilter *buffersrc = avfilter_get_by_name("buffer");
	AVFilterContext* buffersrc_ctx; //每一个avfilter都会有一个设备上下文与之对应
	char in_args[512];
	AVPixelFormat pix_fmts = AV_PIX_FMT_YUV420P;
	snprintf(in_args, sizeof(in_args),"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", width, height, pix_fmts, 1, 25, 1, 1);
	//给buffersrc过滤器传入参数，“in”这个名字实质上是给buffersrc_ctx起了个名字,便于filtergraph管理和定位
	//当前是视频数据做为源数据，所以当前的参数如上，具体：buffer=width=320:height=240:pix_fmt=yuv410p:time_base=1/24:sar=1
	//buffer=video_size=320x240:pixfmt=6:time_base=1/24:pixel_aspect=1/1
	avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", in_args, NULL, filter_graph);
	/*************************************************buffersink过滤器***************************************************/
	//根据名字获取buffersink过滤器，buffersink过滤器是输出过滤后的数据，如缩放后的yuv420p数据
	const AVFilter *buffersink = avfilter_get_by_name("buffersink");
	//输出数据的格式设置
	AVBufferSinkParams *buffersink_params = av_buffersink_params_alloc();
	AVPixelFormat out_pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_NONE};
	buffersink_params->pixel_fmts = out_pix_fmts;
	AVFilterContext* buffersink_ctx;
	avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, buffersink_params, filter_graph);
	av_free(buffersink_params);
	/**********************************************scale过滤器**********************************************************/
	//根据名字获取scale过滤器，这个过滤器ffmpeg库已经实现了这个过滤器，其内部实现和swscale库实现一样
	const AVFilter *scalefilter = avfilter_get_by_name("scale");
	AVFilterContext* scalefilter_ctx;
	char scale_args[512];
	snprintf(scale_args, sizeof(scale_args), "%d:%d", width/2, height/2);//参数是如scale=128:64
	avfilter_graph_create_filter(&scalefilter_ctx, scalefilter, "resize", scale_args, NULL, filter_graph);
   /********************************************转换frame格式********************************************************/
    const AVFilter *pixfmtfilter = avfilter_get_by_name("format");
	AVFilterContext* pixfmtfilter_ctx;
	char pixfmt_args[512];
	snprintf(pixfmt_args, sizeof(pixfmt_args), "pix_fmts=%d", AV_PIX_FMT_YUVJ420P);//format=pix_fmts=yuv420p
	avfilter_graph_create_filter(&pixfmtfilter_ctx, pixfmtfilter, "format", pixfmt_args, NULL, filter_graph);
	/**********************************************各个设备上下文链接*************************************************************************/
	int ret = avfilter_link(buffersrc_ctx, 0, scalefilter_ctx, 0);
	ret = avfilter_link(scalefilter_ctx, 0, pixfmtfilter_ctx, 0);
	ret = avfilter_link(pixfmtfilter_ctx, 0, buffersink_ctx, 0);
	/*****************************************到此为止已经将各个filter串连起来***********************************************/
	//到此为止，过滤器的图初始化完毕
	avfilter_graph_config(filter_graph, NULL);

	// int ret = 0;

	// //new一个pin,并与buffer过滤器设备上正文相关联
	// AVFilterInOut *outputs = avfilter_inout_alloc();
	// outputs->name       = av_strdup("in");
	// outputs->filter_ctx = buffersrc_ctx;
	// outputs->pad_idx    = 0;
	// outputs->next       = NULL;
	// //new一个pin,并与buffersink过滤器设备上正文相关联
	// AVFilterInOut *inputs  = avfilter_inout_alloc();
	// inputs->name       = av_strdup("out");
	// inputs->filter_ctx = buffersink_ctx;
	// inputs->pad_idx    = 0;
	// inputs->next       = NULL;

	// char scale_args[512];
	// snprintf(scale_args, sizeof(scale_args), "scale=%d:%d", width/2, height/2);//参数是如scale=128:64
	// //在两个pin之间插入一个字符串描述的过滤器，如上的scale绽放过滤器
	// avfilter_graph_parse_ptr(filter_graph, scale_args, &inputs, &outputs, NULL);
	// //到此为止，过滤器的图初始化完毕
	// avfilter_graph_config(filter_graph, NULL);

 	AVFrame* inframe = av_frame_alloc();
 	AVFrame *filt_frame = av_frame_alloc();

	while(1){
		if(feof(rFile))
			break;

		inframe->format = pix_fmts;
		inframe->width  = width;
		inframe->height = height;
		av_frame_get_buffer(inframe, 32);

		ReadYUV420FromFile(inframe, rFile);//从yuv文件填充AVFrame
        //向buffer设备上下文填充视频帧
        if (av_buffersrc_add_frame_flags(buffersrc_ctx, inframe, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
			av_frame_unref(inframe);
			break;
		}
	   while (1) {//从buffersink设备上下文获取视频帧
		   ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
		   if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			   break;
		   if (ret < 0)
			   break;
		   //WriteYUV420ToFile(filt_frame, wFile);//将处理后的AVFrame写入到文件
	
		   av_frame_unref(filt_frame);

	   }
	   av_frame_unref(inframe);
	}

	av_frame_free(&inframe);
	av_frame_free(&filt_frame);

	avfilter_graph_free(&filter_graph);

	fclose(rFile);
	fclose(wFile);

	return 0;
}

#endif


