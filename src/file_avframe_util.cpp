#include "file_avframe_util.h"

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


// int linesize[AV_NUM_DATA_POINTERS];

//     /**
//      * pointers to the data planes/channels.
//      *
//      * For video, this should simply point to data[].
//      *
//      * For planar audio, each channel has a separate data pointer, and
//      * linesize[0] contains the size of each channel buffer.
//      * For packed audio, there is just one data pointer, and linesize[0]
//      * contains the total size of the buffer for all channels.
//      *
//      * Note: Both data and extended_data should always be set in a valid frame,
//      * but for planar audio with more channels that can fit in data,
//      * extended_data must be used in order to access all channels.
//      */
//     uint8_t **extended_data;

//写入PCM,裸PCM文件存在格式：LRLRLR......
void WritePCMToFile(const AVFrame *frame, FILE* f){
	//根据format得出每个sample所占的字节数,这里的字节数是指一个通道所占的字节数
	int datasize = av_get_bytes_per_sample((AVSampleFormat)frame->format);
	int planar = av_sample_fmt_is_planar((AVSampleFormat)frame->format);
	if(planar){
			for(int i=0; i<frame->nb_samples; i++){ //循环每个采样
				for(int j=0; j<frame->channels; j++){//如果每个采样有多个通道，则data[0]为第一个通道，data[1]为第二个通道，以此类推
					fwrite(frame->extended_data[j] + i*datasize, datasize, 1, f);
				}
			}
	}else{
		  fwrite(frame->extended_data[0], datasize * frame->channels, frame->nb_samples, f);
	}
	
}
//读取PCM
void ReadPCMFromFile(AVFrame* frame, FILE* f){
	//根据format得出每个sample所占的字节数,这里的字节数是指一个通道所占的字节数
	int datasize = av_get_bytes_per_sample((AVSampleFormat)frame->format);
	int planar = av_sample_fmt_is_planar((AVSampleFormat)frame->format);
	if(planar){
		for(int i=0; i<frame->nb_samples; i++){//循环每个采样
			for(int j=0; j<frame->channels; j++){//如果每个采样有多个通道，则data[0]为第一个通道，data[1]为第二个通道，以此类推
				fread(frame->extended_data[j] + i*datasize, datasize, 1, f);
			}
		}
	}else{
		fread(frame->extended_data[0], datasize * frame->channels, frame->nb_samples, f);
	}
}
