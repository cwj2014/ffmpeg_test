#ifndef FILE_AVFRAME_UTIL_H_H_
#define FILE_AVFRAME_UTIL_H_H_

#include <cstdio>

struct AVFrame;
//写入YUV420
void WriteYUV420ToFile(const AVFrame *frame, FILE* f);
//读取YUV420
void ReadYUV420FromFile(AVFrame *frame, FILE *f);
//写入PCM
void WritePCMToFile(const AVFrame *frame, FILE* f);
//读取PCM
void ReadPCMFromFile(AVFrame* frame, FILE* f);

#endif