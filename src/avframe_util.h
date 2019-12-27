#ifndef _AVFRAME_UTIL_H_H_
#define _AVFRAME_UTIL_H_H_

#include <cstdio>

struct AVFrame;
//写入YUV420
void WriteYUV420ToFile(const AVFrame *frame, FILE* f);
void WriteYUV420P10LEToFile(const AVFrame *frame, FILE* f);
//读取YUV420
void ReadYUV420FromFile(AVFrame *frame, FILE *f);
void ReadYUV420P10LEFromFile(AVFrame *frame, FILE *f);
//写入PCM
void WritePCMToFile(const AVFrame *frame, FILE* f);
//读取PCM
void ReadPCMFromFile(AVFrame* frame, FILE* f);

//横向合并YUV420，要求left的高度和right的高度相同
AVFrame* YUV420HorizontalMerge(const AVFrame* left, const AVFrame* right);
//纵向合并YUV420，要求up和down的宽度相同
AVFrame* YUV420VerticalMerge(const AVFrame* up, const AVFrame* down);

#endif