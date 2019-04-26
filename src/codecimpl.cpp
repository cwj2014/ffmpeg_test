#include "codecimpl.h"
extern "C"{
    #include <libavcodec/avcodec.h>
}

int encode(AVCodecContext *ctx, const AVFrame *frame, std::shared_ptr<EncodeCallback> callback){
    auto onSuccess =  std::bind(&EncodeCallback::OnSuccess, callback, std::placeholders::_1, std::placeholders::_2);
    return encode(ctx, frame, onSuccess);
}

int encode(AVCodecContext *ctx, const AVFrame *frame, OnEncodeSuccess onSucess){
    int ret;
    ret = avcodec_send_frame(ctx, frame);
    if(ret < 0){
        return -1;
    }
    AVPacket *pkt = av_packet_alloc();
    int result = 0;
    while(ret >=0){
        ret =  avcodec_receive_packet(ctx, pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break;
        }else if(ret < 0){
            result = -1;
            break;
        }else{
            onSucess(ctx, pkt);
            av_packet_unref(pkt);
        }
    }
    av_packet_free(&pkt);
    return result;
}

int decode(AVCodecContext *ctx, const AVPacket* packet, std::shared_ptr<DecodeCallback> callback){
    auto onSuccess =  std::bind(&DecodeCallback::OnSuccess, callback, std::placeholders::_1, std::placeholders::_2);
    return decode(ctx, packet, onSuccess);
}

int decode(AVCodecContext *ctx, const AVPacket* packet, OnDecodeSuccess onSucess){
    int ret = avcodec_send_packet(ctx, packet);
	if (ret < 0) {
		return -1;
	}
	AVFrame* frame = av_frame_alloc();
    int result = 0;
	while(ret >= 0){
		ret = avcodec_receive_frame(ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
			break;
		}else if (ret < 0) {
			result = -1;
            break;
		}else{
            onSucess(ctx, frame);
            av_frame_unref(frame);
        }
	}
	av_frame_free(&frame);
    return result;
}