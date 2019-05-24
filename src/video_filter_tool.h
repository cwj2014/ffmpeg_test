#ifndef VIDEO_FILTER_TOOL_H_H_
#define VIDEO_FILTER_TOOL_H_H_

#include "global.h"
#include <vector>
/**
 * 视频帧过滤器
 */
class VideoFilterManager{
public:
    VideoFilterManager()
        :filter_graph(NULL),
         buffersrc_ctx(NULL),
         buffersink_ctx(NULL){
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
    bool CreateBufferFilter(const char* filter){
        const AVFilter *buffersrc = avfilter_get_by_name("buffer");
        if(avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", filter, NULL, filter_graph)<0)
            return false;
        return true;    
    }
    /**
     * 创建视频帧出口过滤器
     * @pix_fmts: 输出的视频帧格式
     */ 
    bool CreateBufferSinkFilter(AVPixelFormat *pix_fmts){
        const AVFilter *buffersink = avfilter_get_by_name("buffersink");
	    //输出数据的格式设置
	    AVBufferSinkParams *buffersink_params = av_buffersink_params_alloc();
	    buffersink_params->pixel_fmts = pix_fmts;
        int ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, buffersink_params, filter_graph);
	    av_free(buffersink_params);
        return (ret >= 0);
    }
    /**
     * 添加其他过滤器,添加过滤器需要注意添加顺序
     * @in: 输入端
     * @out: 输入端
     */ 
    bool AddFilter(AVFilterContext* in, AVFilterContext* out){
        if((!in && !out)||
          (!buffersrc_ctx && !in)||
          (!buffersink_ctx && !out)){
            return false;
        }

        if(in == NULL){
            in = buffersrc_ctx;
        }else if( out == NULL){
            out = buffersink_ctx;
        }
        int ret = avfilter_link(in, 0, out, 0);
        return (ret == 0);
    }
    /**
     * 配置filter, 完成过滤器的连通
     */ 
    bool VideoFilterConfig(){
        if(avfilter_graph_config(filter_graph, NULL) < 0)
            return false;
        return true;
    }
    /**
     * 向入口过滤器输入视频帧数据
     * @frame: 输入数据
     */ 
    int AddFrame(AVFrame* frame){
        if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
			return -1;
		}
        return 0;
    }
    /**
     * 获取过滤后的视频帧数据
     * @frame画出数据
     */ 
    int GetFrame(AVFrame* frame){
        return av_buffersink_get_frame(buffersink_ctx, frame);
    }
private:
    AVFilterGraph *filter_graph;
    AVFilterContext* buffersrc_ctx;
    AVFilterContext* buffersink_ctx;
    std::vector<AVFilterContext*> otherFilters;
};

#endif