#include "avframe_util.h"

extern "C"{
#include <libavutil/frame.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
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
void WriteYUV420P10LEToFile(const AVFrame *frame, FILE* f){
	if(frame->linesize[0] != frame->width * 2){
		for(int i=0; i<frame->height; i++){//写入Y数据
			//如width=351, frame->linesize[0] = 352, 涉及到数据对齐的问题，
			//所以不能直接使用frame->width*frame->height大小直接copy
			fwrite(frame->data[0] + i*frame->linesize[0], 2, frame->width, f);
		}
	}else{
		fwrite(frame->data[0], 2, frame->width*frame->height, f);
	}
	if(frame->linesize[1] != frame->width){
		for(int i=0; i<frame->height / 2; i++){//写入U数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fwrite(frame->data[1] + i*frame->linesize[1], 2, frame->width / 2, f);
		}
	}else{
		fwrite(frame->data[1], 2, (frame->width*frame->height)>>2, f);
	}
	if(frame->linesize[2] != frame->width){
		for(int i=0; i<frame->height / 2; i++){//写入V数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fwrite(frame->data[2] + i*frame->linesize[2], 2, frame->width / 2, f);
		}
	}else{
		fwrite(frame->data[2], 2, (frame->width*frame->height)>>2, f);
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

void ReadYUV420P10LEFromFile(AVFrame *frame, FILE *f){
	if(frame->linesize[0] != frame->width * 2){
		for(int i=0; i<frame->height; i++){//写入Y数据
			//如width=351, frame->linesize[0] = 352, 涉及到数据对齐的问题，
			//所以不能直接使用frame->width*frame->height大小直接copy
			fread(frame->data[0] + i*frame->linesize[0], 2, frame->width, f);
		}
	}else{
		fread(frame->data[0], 2, frame->width * frame->height , f);
	}
	if(frame->linesize[1] != frame->width){
		for(int i=0; i<frame->height / 2; i++){//写入U数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fread(frame->data[1] + i*frame->linesize[1], 2, frame->width / 2, f);
		}
	}else{
		fread(frame->data[1], 2, (frame->width*frame->height)>>2, f);
	}
	if(frame->linesize[2] != frame->width){
		for(int i=0; i<frame->height / 2; i++){//写入V数据
			//如width=351, 涉及到数据对齐的问题，
			//所以不能直接使用(frame->width*frame->height) >> 2大小直接copy
			fread(frame->data[2] + i*frame->linesize[2], 2, frame->width / 2, f);
		}
	}else{
		fread(frame->data[2], 2, (frame->width*frame->height)>>2, f);
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

//横向合并YUV420，要求left的高度和right的高度相同
AVFrame* YUV420HorizontalMerge(const AVFrame* left, const AVFrame* right){
	if(left == NULL || right == NULL)
		return NULL;
	if(left->height != right->height)
		return NULL;
	AVFrame* destFrame = av_frame_alloc();
	int dest_height = left->height;
	int dest_width =  left->width+left->width;
	destFrame->format = AV_PIX_FMT_YUV420P;
	destFrame->width = dest_width;
	destFrame->height = dest_height;
	av_frame_get_buffer(destFrame, 32);

	for(int i=0; i<dest_height; i++){
		//复制left一行Y
		memcpy(destFrame->data[0] + i*destFrame->linesize[0], left->data[0] + i * left->linesize[0] , left->width);
		//复制right一行Y
		memcpy(destFrame->data[0] + i*destFrame->linesize[0] + left->width, right->data[0] + i * right->linesize[0], right->width);
	}

	for(int j=0; j<dest_height/2; j++){
		//复制left一行U
		memcpy(destFrame->data[1] + j*destFrame->linesize[1], left->data[1] + j * left->linesize[1], left->width/2);
		//复制right一行U
		memcpy(destFrame->data[1] + j*destFrame->linesize[1] + left->width/2, right->data[1] + j * right->linesize[1], right->width/2);

		//复制left一行V
		memcpy(destFrame->data[2] + j*destFrame->linesize[2], left->data[2] + j * left->linesize[2], left->width/2);
		//复制right一行V
		memcpy(destFrame->data[2] + j*destFrame->linesize[2] + left->width/2, right->data[2] + j * right->linesize[2], right->width/2);
	}
	return destFrame;
}
//纵向合并YUV420，要求up和down的宽度相同
AVFrame* YUV420VerticalMerge(const AVFrame* up, const AVFrame* down){
	if(up == NULL || down == NULL)
		return NULL;
	if(up->width != down->width)
		return NULL;
	AVFrame* destFrame = av_frame_alloc();
	int dest_height = up->height + down->height;
	int dest_width =  up->width;
	destFrame->format = AV_PIX_FMT_YUV420P;
	destFrame->width = dest_width;
	destFrame->height = dest_height;
	av_frame_get_buffer(destFrame, 32);

	{//复制Y
		int lines = 0;
		for(int i=0; i<up->height; i++){
			//复制up一行Y
			memcpy(destFrame->data[0] + (lines++)*destFrame->linesize[0], up->data[0] + i * up->linesize[0] , up->width);
		}
		for(int j=0; j<down->height; j++){
			//复制down一行Y
			memcpy(destFrame->data[0] + (lines++)*destFrame->linesize[0], down->data[0] + j * down->linesize[0] , down->width);
		}
	}

	{//复制UV
		int lines = 0;
		for(int i=0; i<up->height/2; i++){
			//复制up一行U
			memcpy(destFrame->data[1] + lines*destFrame->linesize[1], up->data[1] + i * up->linesize[1] , up->width / 2);
			//复制up一行V
			memcpy(destFrame->data[2] + lines*destFrame->linesize[2], up->data[2] + i * up->linesize[2] , up->width / 2);

			lines++;
		}
		for(int j=0; j<down->height/2; j++){
			//复制down一行U
			memcpy(destFrame->data[1] + lines*destFrame->linesize[1], down->data[1] + j * down->linesize[1] , down->width/2);
			//复制down一行V
			memcpy(destFrame->data[2] + lines*destFrame->linesize[2], down->data[2] + j * down->linesize[2] , down->width/2);

			lines++;
		}
	}
	return destFrame;
}
