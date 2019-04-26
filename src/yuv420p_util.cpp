#include "yuv420p_util.h"

extern "C"{
#include <libavutil/frame.h>
}

//写入YUV420
void WriteYUV420ToFile(const AVFrame *frame, FILE* f){
    if(frame->linesize[0] != frame->width){
		for(int i=0; i<frame->height; i++){//写入Y数据
			//如width=351, frame->linesize[0] = 352, 涉及到数据对齐的问题，
			//所以不能直接使用frame->width*frame->height大小直接copy
			fwrite(frame->data[0] + i*frame->linesize[0], 1, frame->width, f);
		}
	}else{
		fwrite(frame->data[0], 1, frame->width*frame->height, f);
	}
	if(frame->linesize[1] != frame->width / 2){
		for(int i=0; i<frame->height / 2; i++){//写入U数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fwrite(frame->data[1] + i*frame->linesize[1], 1, frame->width / 2, f);
		}
	}else{
		fwrite(frame->data[1], 1, (frame->width*frame->height)>>2, f);
	}
	if(frame->linesize[2] != frame->width / 2){
		for(int i=0; i<frame->height / 2; i++){//写入V数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fwrite(frame->data[2] + i*frame->linesize[2], 1, frame->width / 2, f);
		}
	}else{
		fwrite(frame->data[2], 1, (frame->width*frame->height)>>2, f);
	}
}
//读取YUV420
void ReadYUV420FromFile(AVFrame *frame, FILE *f){
    if(frame->linesize[0] != frame->width){
		for(int i=0; i<frame->height; i++){//写入Y数据
			//如width=351, frame->linesize[0] = 352, 涉及到数据对齐的问题，
			//所以不能直接使用frame->width*frame->height大小直接copy
			fread(frame->data[0] + i*frame->linesize[0], 1, frame->width, f);
		}
	}else{
		fread(frame->data[0], 1, frame->width * frame->height , f);
	}
	if(frame->linesize[1] != frame->width / 2){
		for(int i=0; i<frame->height / 2; i++){//写入U数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fread(frame->data[1] + i*frame->linesize[1], 1, frame->width / 2, f);
		}
	}else{
		fread(frame->data[1], 1, (frame->width*frame->height)>>2, f);
	}
	if(frame->linesize[2] != frame->width / 2){
		for(int i=0; i<frame->height / 2; i++){//写入V数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fread(frame->data[2] + i*frame->linesize[2], 1, frame->width / 2, f);
		}
	}else{
		fread(frame->data[2], 1, (frame->width*frame->height)>>2, f);
	}
}