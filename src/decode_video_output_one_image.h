#ifndef DECODE_VIDEO_OUTPUT_ONE_IMAGE_H_H_H
#define DECODE_VIDEO_OUTPUT_ONE_IMAGE_H_H_H

#include "global.h"
#include "codecimpl.h"

static AVFormatContext* inFormatCtx = NULL;
static AVCodecContext* vDecCodecContext = NULL;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;

/**
 * 生成缩略图，默认第一帧视频帧生成缩略图，宽高为视频宽高
 * @inputVideoFile: 输入视频文件
 * @outImageFile: 输出图片文件
 * @width: 缩略图片宽
 * @height: 缩略图片高
 * @nframe: 截取第几帧视频生成缩略图
 */ 
int thumb(const char* inputVideoFile, const char* outImageFile, unsigned int width = 0, unsigned int height = 0, unsigned int nframe = 1);


static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVRational time_base = inFormatCtx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            vDecCodecContext->width, vDecCodecContext->height, vDecCodecContext->pix_fmt,
            time_base.num, time_base.den,
            vDecCodecContext->sample_aspect_ratio.num, vDecCodecContext->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
        goto end;
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                    &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int decode_video_output_one_image_test(){
    const char* inputVideoFile = "./123.mp4";
    const char* outImageFile = "./123.jpeg";
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int nframe = 1;
    return thumb(inputVideoFile, outImageFile, width, height, nframe);
}


int thumb(const char* inputVideoFile, const char* outImageFile, unsigned int width, unsigned int height, unsigned int nframe){
    
    AVFormatContext* outFormatCtx = NULL;
    AVCodecContext* vEncCodecContext = NULL;
    AVCodec* vDecCodec = NULL;
    AVCodec* vEncCodec = NULL;
    int ret = -1;
    int m, n;
    AVPacket avpacket;
    AVFrame* avframe = NULL;
    AVRational timebase;
    OnDecodeSuccess decodeCallback;
    OnEncodeSuccess encodeCallback;
    bool bVideoFilter = false;
    if(width > 0 && height > 0){
        bVideoFilter = true;
    }
    if(avformat_open_input(&inFormatCtx, inputVideoFile, NULL, NULL) < 0){
        return -1;
    }
    if(avformat_find_stream_info(inFormatCtx, nullptr) < 0){
        goto end;
    }
    for(int i=0; i<inFormatCtx->nb_streams; i++){
        AVStream* stream = inFormatCtx->streams[i];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i;
            const AVCodec* pCodec =  avcodec_find_decoder(stream->codecpar->codec_id);
            vDecCodecContext = avcodec_alloc_context3(pCodec);
            if(avcodec_parameters_to_context(vDecCodecContext, stream->codecpar)<0){
                goto end;
            }
            if(avcodec_open2(vDecCodecContext, pCodec, NULL) < 0){
                goto end;
            }
            timebase = vDecCodecContext->time_base;
            if(!bVideoFilter){
                width = stream->codecpar->width;
                height = stream->codecpar->height;
            }
            break;
        }
    }
    if(video_stream_index == -1)
        goto end;
    if(avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, outImageFile) < 0){
        goto end;
    }
    avformat_new_stream(outFormatCtx, 0);
    
    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
      ret = avio_open(&outFormatCtx->pb, outImageFile, AVIO_FLAG_WRITE);
      if (ret < 0) {
          fprintf(stderr, "Could not open output file '%s'", outImageFile);
          goto end;
      }
    }
   
    //init_filters(NULL);

    vEncCodec = avcodec_find_encoder(outFormatCtx->oformat->video_codec);
    vEncCodecContext = avcodec_alloc_context3(vEncCodec);
    vEncCodecContext->codec_id = vEncCodec->id;
    vEncCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    vEncCodecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
    //vEncCodecContext->color_range = AVCOL_RANGE_MPEG;
    vEncCodecContext->width = width; 
    vEncCodecContext->height = height;
    vEncCodecContext->time_base.num = 1;
    vEncCodecContext->time_base.den = 25;
    if (avcodec_open2(vEncCodecContext, vEncCodec,NULL) < 0){
        goto end;
    }

    if(avformat_write_header(outFormatCtx, nullptr) < 0){
        goto end;
    }
     
    //计算从第几个关键帧开始解码，解几帧之后开始保存
    //m = nframe / timebase.den;
    //n = nframe % timebase.den;

    decodeCallback = [&](AVCodecContext *ctx, const AVFrame* frame){
              //if(n == 0){//定入图片文件
                 encode(vEncCodecContext, frame, encodeCallback);
              //}else{
              //    n--;
              //}
    };

    encodeCallback = [&](AVCodecContext* ctx, const AVPacket* avpkt){
        AVPacket* pkt = av_packet_clone(avpkt);
        av_write_frame(outFormatCtx, pkt);
        av_packet_unref(pkt);
        av_write_trailer(outFormatCtx);
    };

    av_init_packet(&avpacket);
    while((ret = av_read_frame(inFormatCtx, &avpacket)) == 0){
        if(avpacket.stream_index != video_stream_index){
            continue;
        }
        // if(m > 0){
        //     if(avpacket.flags & AV_PKT_FLAG_KEY){
        //         m--;
        //     }
        //     continue;
        // }
        ret = decode(vDecCodecContext, &avpacket, decodeCallback);
        // av_packet_unref(&avpacket);
        // if(n <= 0){
        //     break;
        // }
        break;
    }
    decode(vDecCodecContext, nullptr, decodeCallback);
end:
   avformat_close_input(&inFormatCtx);
   if(vDecCodecContext){
       avcodec_close(vDecCodecContext);
   }
   if(vEncCodecContext){
       avcodec_close(vEncCodecContext);
   }
   if (outFormatCtx && !(outFormatCtx->oformat->flags & AVFMT_NOFILE)){
     avio_closep(&outFormatCtx->pb);
   }
   return ret;
}


#endif