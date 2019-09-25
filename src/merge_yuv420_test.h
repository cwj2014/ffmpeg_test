#ifndef MERGE_YUV420_TEST_H_H_
#define MERGE_YUV420_TEST_H_H_

#include <stdio.h>
#include "avframe_util.h"
#include "global.h"

void merge_yuv420_test(){
    FILE* rFile = fopen("./akiyo_cif.yuv","rb");
	if(rFile == NULL)
		return;
	const char* oFileName1 = "./akiyo_cif_horizontal.yuv";
    const char* oFileName2 = "./akiyo_cif_vertical.yuv";
    int width = 352;
    int height = 288;
    FILE* oFile1 = fopen(oFileName1, "wb");
    FILE* oFile2 = fopen(oFileName2, "wb");

    AVFrame* frame1 = av_frame_alloc();
    AVFrame* frame2 = av_frame_alloc();
    int i=0;
    while(1){
		if(feof(rFile))
			break;

		frame1->format = AV_PIX_FMT_YUV420P;
		frame1->width  = width;
		frame1->height = height;
		av_frame_get_buffer(frame1, 32);

		ReadYUV420FromFile(frame1, rFile);//从yuv文件填充AVFrame

        frame2->format = AV_PIX_FMT_YUV420P;
		frame2->width  = width;
		frame2->height = height;
		av_frame_get_buffer(frame2, 32);

		ReadYUV420FromFile(frame2, rFile);//从yuv文件填充AVFrame

        AVFrame* mergeHorizonal = YUV420HorizontalMerge(frame1, frame2);
        AVFrame* mergeVertical = YUV420VerticalMerge(frame1, frame2);

        WriteYUV420ToFile(mergeHorizonal, oFile1);
        WriteYUV420ToFile(mergeVertical, oFile2);
  

        av_frame_unref(frame1);
        av_frame_unref(frame2);

        av_frame_free(&mergeHorizonal);
        av_frame_free(&mergeVertical);
    }

    fclose(rFile);
    fclose(oFile1);
    fclose(oFile2);
}


#endif