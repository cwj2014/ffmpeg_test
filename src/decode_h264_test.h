#ifndef DECODEC_H264_TEST_H_H_
#define DECODEC_H264_TEST_H_H_

#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "file_avframe_util.h"
#include "codecimpl.h"


#define INBUF_SIZE 4096

int decode_h264_test(){
    const char* iFileName = "test.h264";
    FILE* ifile = fopen(iFileName, "rb");
    if(ifile == NULL)
        return -1;
    const char* oFileName = "hello.yuv";
    FILE* ofile = fopen(oFileName, "wb");
    if(ifile == NULL)
        ofile -1;
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264); 
    AVCodecContext* avcodecCtx =  avcodec_alloc_context3(codec);
    if(avcodec_open2(avcodecCtx, codec, nullptr) < 0){
        return -1;
    }
    AVCodecParserContext *parser = av_parser_init(codec->id);
    if (!parser) {
        fprintf(stderr, "parser not found\n");
        return -1;
    }
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        return -1;
    }

    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t   data_size;
    int ret;
    AVPacket *pkt= av_packet_alloc();
    av_init_packet(pkt);

     auto callback = [&](AVCodecContext *ctx, const AVFrame* frame){
         WriteYUV420ToFile(frame, ofile);
         printf("解码成功\n");
     };

    while (!feof(ifile)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, ifile);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, avcodecCtx, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                exit(1);
            }
            data      += ret;
            data_size -= ret;

            if (pkt->size)
                decode(avcodecCtx, pkt, callback);
        }
    }

    /* flush the decoder */
    decode(avcodecCtx, pkt, callback);

    fclose(ifile);
    fclose(ofile);

    av_parser_close(parser);
    avcodec_free_context(&avcodecCtx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

#endif