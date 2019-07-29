#ifndef VIDEO_FILTER_TOOL_H_H_
#define VIDEO_FILTER_TOOL_H_H_

#include "global.h"

/**
 * 视频帧过滤器
 */
class VideoFilterManager{
public:
    VideoFilterManager(){
        filter_graph = avfilter_graph_alloc();
    }
    ~VideoFilterManager(){
        if(filter_graph){
            avfilter_graph_free(&filter_graph);
            filter_graph = NULL;
        }
    }
    /**
     * 创建视频入口过滤器
     * @filter: 参数字符串，如video_size=320x240:pixfmt=6:time_base=1/24:pixel_aspect=1/1
     */ 
    AVFilterContext* CreateBufferFilter(const char* filter, const char* name){
        const AVFilter *buffersrc = avfilter_get_by_name("buffer");
        return CreateFilterContext(buffersrc, name, filter);
    }
    /**
     * 创建视频帧出口过滤器
     * @pix_fmts: 输出的视频帧格式
     */ 
    AVFilterContext* CreateBufferSinkFilter(AVPixelFormat *pix_fmts, const char* name){
        const AVFilter *buffersink = avfilter_get_by_name("buffersink");
	    //输出数据的格式设置
	    AVBufferSinkParams *buffersink_params = av_buffersink_params_alloc();
	    buffersink_params->pixel_fmts = pix_fmts;
        AVFilterContext* ctx = CreateFilterContext(buffersink, name, NULL, buffersink_params);
	    av_free(buffersink_params);
        return ctx;
    }
    /*
    *创建AVFilterContext
    *@filter: filter对象
    *@name: filter对应的名字
    *@filter_descr: filter参数字符串
    *@opaque: 其他参数
    */
    AVFilterContext* CreateFilterContext(const AVFilter* filter, const char* name, const char* filter_descr, void *opaque = NULL){
        AVFilterContext* ctx;
	    avfilter_graph_create_filter(&ctx, filter, name, filter_descr, opaque, filter_graph);
        return ctx;
    }
    //插入过滤器
    int InsertFilter(AVFilterInOut* inputs, AVFilterInOut* outputs, const char* filter_descr){
        return avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, nullptr);
    }
    /**
     * 配置filter, 完成过滤器的连通
     */ 
    bool FilterConfig(){
        if(avfilter_graph_config(filter_graph, NULL) < 0)
            return false;
        return true;
    }
    /**
     * 向入口过滤器输入视频帧数据
     * @frame: 输入数据
     */ 
    int AddFrame(AVFilterContext* buf_src_ctx, AVFrame* frame){
        if (av_buffersrc_add_frame_flags(buf_src_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
			return -1;
		}
        return 0;
    }
    /**
     * 获取过滤后的视频帧数据
     * @frame画出数据
     */ 
    int GetFrame(AVFilterContext* buf_sink_ctx, AVFrame* frame){
        return av_buffersink_get_frame(buf_sink_ctx, frame);
    }
private:
    AVFilterGraph *filter_graph;
};

#endif