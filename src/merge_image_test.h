#ifndef MERGE_IMAGE_TEST_H_H_
#define MERGE_IMAGE_TEST_H_H_

#include "global.h"
#include <vector>
#include <string>
#include "avframe_util.h"
#include "codecimpl.h"

static AVFrame* GetFrameFromImageFile(const char* inputfile){
    AVFormatContext* inFormatCtx = NULL;
    AVCodecContext* vDecCodecContext = NULL;
    AVCodec* vDecCodec = NULL;
    int video_stream_index = -1;
    int ret = -1;
    AVPacket avpacket;
    AVFrame* outFrame = NULL;
    if(avformat_open_input(&inFormatCtx, inputfile, NULL, NULL) < 0){
        return NULL;
    }
    video_stream_index = av_find_best_stream(inFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(video_stream_index == -1){
        goto end;
    }
    {
        AVStream* stream = inFormatCtx->streams[video_stream_index];
            
        const AVCodec* decCodec = avcodec_find_decoder(stream->codecpar->codec_id);
        vDecCodecContext = avcodec_alloc_context3(decCodec);
        avcodec_parameters_to_context(vDecCodecContext, stream->codecpar);
        if(avcodec_open2(vDecCodecContext, decCodec, nullptr)<0){
            goto end;
        }
    }
    av_init_packet(&avpacket);
    while((ret = av_read_frame(inFormatCtx, &avpacket)) == 0){
        if(avpacket.stream_index != video_stream_index){
            continue;
        }
        decode(vDecCodecContext, &avpacket, [&](AVCodecContext *ctx, const AVFrame* frame){
            outFrame = av_frame_clone(frame);
        });
        break;
    }
end:
   avformat_close_input(&inFormatCtx);
   if(vDecCodecContext){
       avcodec_close(vDecCodecContext);
   }
   return outFrame;
}

static int WriteFrameToFile(const char* outputfile, AVFrame* merge){
    AVFormatContext* outFormatCtx = NULL;
    AVCodecContext* vEncCodecContext = NULL;
    int ret = -1;
    if(avformat_alloc_output_context2(&outFormatCtx, NULL, NULL, outputfile) < 0){
        goto end;
    }
    avformat_new_stream(outFormatCtx, 0);
    
    if (!(outFormatCtx->oformat->flags & AVFMT_NOFILE)) {
      ret = avio_open(&outFormatCtx->pb, outputfile, AVIO_FLAG_WRITE);
      if (ret < 0) {
          fprintf(stderr, "Could not open output file '%s'", outputfile);
          goto end;
      }
    }
    {//初始化编码器
        const AVCodec* vEncCodec = avcodec_find_encoder(outFormatCtx->oformat->video_codec);
        vEncCodecContext = avcodec_alloc_context3(vEncCodec);
        vEncCodecContext->codec_id = vEncCodec->id;
        vEncCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
        vEncCodecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
        vEncCodecContext->width = merge->width; 
        vEncCodecContext->height = merge->height;
        vEncCodecContext->time_base.num = 1;
        vEncCodecContext->time_base.den = 25;
        if (avcodec_open2(vEncCodecContext, vEncCodec,NULL) < 0){
            goto end;
        }
    }

    if(avformat_write_header(outFormatCtx, nullptr) < 0){
        goto end;
    }
    merge->pts = 0;
    encode(vEncCodecContext, merge, [&](AVCodecContext* ctx, const AVPacket* avpkt){
        AVPacket* pkt = av_packet_clone(avpkt);
        av_write_frame(outFormatCtx, pkt);
        av_packet_unref(pkt);
    });

    ret = av_write_trailer(outFormatCtx);

end:
    if(vEncCodecContext){
        avcodec_close(vEncCodecContext);
    }
    if (outFormatCtx && !(outFormatCtx->oformat->flags & AVFMT_NOFILE)){
        avio_closep(&outFormatCtx->pb);
    }
    return ret;
}

int VerticalMergeImageFiles(std::vector<std::string> imageFiles, const char* outputfile){
    AVFrame* up = GetFrameFromImageFile(imageFiles[0].c_str());
    AVFrame* down = NULL;
    int ret = -1;
    for(int i=1; i<imageFiles.size(); i++){
        down = GetFrameFromImageFile(imageFiles[i].c_str());
        AVFrame* merge = YUV420VerticalMerge(up, down);
        if(!merge){
            goto error;
        }
        av_frame_free(&up);
        up = NULL;
        av_frame_free(&down);
        down = NULL;
        up = merge;
    }
    //写入图片
    ret = WriteFrameToFile(outputfile, up);
error:
    if(up){
        av_frame_free(&up);
    }
    if(down){
        av_frame_free(&down);
    }
    return ret;
}



void merge_files_test(){
    std::vector<std::string> images;
    for(int i=1; i<=3; i++){
        std::string path = std::string("./images/") + std::to_string(i)+std::string(".jpeg");
        images.push_back(path);
    }
    const char* outfile = "merge.jpeg";
    VerticalMergeImageFiles(images, outfile);
}

#endif