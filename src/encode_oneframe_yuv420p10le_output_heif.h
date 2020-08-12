#ifndef ENCODE_ONEFRAME_YUV420P10LE_OUTPUT_HEIF_H_H_
#define ENCODE_ONEFRAME_YUV420P10LE_OUTPUT_HEIF_H_H_
#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "avframe_util.h"
#include "codecimpl.h"
#include <Heif.h>
#include <HEVCCodedImageItem.h>
#include <HEVCDecoderConfiguration.h>

using namespace HEIFPP;

int encode_yuv420p10le_output_heif(){

    uint8_t *vps_pps_sps = new uint8_t[1024];
    unsigned int par_size = 0;
    unsigned char *h265raw;  

    uint64_t h265raw_size = 0; 


    auto callback = [&](AVCodecContext* ctx,const AVPacket* avpkt){
        uint64_t start_pos = 0;
        uint64_t end_pos = avpkt->size;
        uint64_t nulu_size;
        uint8_t* source = avpkt->data;
        uint8_t* nulu_data;
        int nuluNum = 0;
        uint8_t head[4] = {0x00, 0x00, 0x00, 0x01};
        h265raw = new uint8_t[end_pos + 10];
        while((nulu_data = getNulu(source, start_pos, end_pos, nulu_size)) != nullptr){
            nuluNum++;
            int nulu_type = (nulu_data[0] & 0x7E)>>1;
            // NAL_BLA_W_LP = 16, 
            // NAL_BLA_W_RADL = 17, 
            // NAL_BLA_N_LP = 18, 
            // NAL_IDR_W_RADL = 19, 
            // NAL_IDR_N_LP = 20, 
            // NAL_CRA_NUT = 21, 
            // NAL_VPS = 32, 
            // NAL_SPS = 33, 
            // NAL_PPS = 34,
            if(nulu_type == 32){
                printf("VPS\n");
                memcpy(vps_pps_sps + par_size, head, 4);
                par_size += 4;
                memcpy(vps_pps_sps + par_size, nulu_data, nulu_size);
                par_size += nulu_size;
            }else if(nulu_type == 33){
                printf("SPS\n");
                memcpy(vps_pps_sps + par_size, head, 4);
                par_size += 4;
                memcpy(vps_pps_sps + par_size, nulu_data, nulu_size);
                par_size += nulu_size;
            }else if(nulu_type == 34){
                printf("PPS\n");
                memcpy(vps_pps_sps + par_size, head, 4);
                par_size += 4;
                memcpy(vps_pps_sps + par_size, nulu_data, nulu_size);
                par_size += nulu_size;
            }else if(nulu_type == 39 || nulu_type == 40){
                printf("SEI\n");
                memcpy(h265raw + h265raw_size, head, 4);
                h265raw_size += 4;
                memcpy(h265raw + h265raw_size, nulu_data, nulu_size);
                h265raw_size += nulu_size;
            }else if(nulu_type == 16 || nulu_type == 17 || nulu_type == 18 ||
                     nulu_type == 19 || nulu_type == 20 || nulu_type == 21){
                 printf("I Frame, type: %d\n", nulu_type);
                memcpy(h265raw + h265raw_size, head, 4);
                h265raw_size += 4;
                memcpy(h265raw + h265raw_size, nulu_data, nulu_size);
                h265raw_size += nulu_size;
            }else{
                printf("############%d###########\n", nulu_type);
            }
            delete []nulu_data;
        }
    };

    const char * oFileName = "./test2.heif";

#if 1
    FILE* rFile = fopen("./output.yuv","rb");
	if(rFile == NULL)
		return -1;
	
    
    int width = 352;
    int height = 288;


    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H265);

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
    InitVideoAVCodecCtx(pCodecCtx, AV_CODEC_ID_H265, width, height);
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P10LE;
    
    int ret = avcodec_open2(pCodecCtx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

    
    AVFrame* inframe = av_frame_alloc();

    inframe->format = AV_PIX_FMT_YUV420P10LE;
	inframe->width  = width;
	inframe->height = height;
	av_frame_get_buffer(inframe, 32);

	ReadYUV420P10LEFromFile(inframe, rFile);//从yuv文件填充AVFrame
    inframe->pts = 0;
    encode(pCodecCtx, inframe, callback);
    av_frame_unref(inframe);

    encode(pCodecCtx, nullptr, callback);

    // Heif* readHeif = new Heif();
    // if(readHeif->load("./111.heic") != Result::OK){
    //     return -1;
    // }

    // HEVCCodedImageItem* primaryItem = (HEVCCodedImageItem*)readHeif->getPrimaryItem();
    // width = primaryItem->width();
    // height = primaryItem->height();
    // DecoderConfig* readerDecoderConfig = primaryItem->getDecoderConfiguration();
    // readerDecoderConfig->getConfig(vps_pps_sps, par_size);
    // const unsigned char* imageData = primaryItem->getItemData();
    // uint64_t datasize = primaryItem->getItemDataSize();

    
    // h265raw_size = datasize;
    // h265raw = new unsigned char[h265raw_size];
    // memcpy(h265raw, imageData, h265raw_size);

#else
    Heif* readHeif = new Heif();
    if(readHeif->load("./111.heic") != Result::OK){
        return -1;
    }

    HEVCCodedImageItem* primaryItem = (HEVCCodedImageItem*)readHeif->getPrimaryItem();
    int width = primaryItem->width();
    int height = primaryItem->height();
    DecoderConfig* readerDecoderConfig = primaryItem->getDecoderConfiguration();
    uint32_t vps_pps_sps_size;
    readerDecoderConfig->getConfig(vps_pps_sps, vps_pps_sps_size);
    const unsigned char* imageData = primaryItem->getItemData();
    uint64_t datasize = primaryItem->getItemDataSize();

    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H265); 
    AVCodecContext* avcodecCtx =  avcodec_alloc_context3(codec);
    if(avcodec_open2(avcodecCtx, codec, nullptr) < 0){
        return -1;
    }

    


    AVPacket pkt;
    av_init_packet(&pkt);
    
    uint8_t* data = new uint8_t[vps_pps_sps_size+datasize];

    memcpy(data, vps_pps_sps, vps_pps_sps_size);
    memcpy(data+vps_pps_sps_size, imageData, datasize);

    av_packet_from_data(&pkt, data, vps_pps_sps_size+datasize);
     
    AVFrame * inframe; 
    auto decodecallback = [&](AVCodecContext *ctx, const AVFrame* frame){
         inframe = av_frame_clone(frame);
         printf("解码成功\n");
    };

    decode(avcodecCtx, &pkt, decodecallback);
    av_packet_unref(&pkt);

    inframe->pts = 0;

    const AVCodec *encodec = avcodec_find_encoder(AV_CODEC_ID_H265);

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(encodec);
    InitVideoAVCodecCtx(pCodecCtx, AV_CODEC_ID_H265, width, height);
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P10LE;
    
    int ret = avcodec_open2(pCodecCtx, encodec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

    vps_pps_sps = new unsigned char[pCodecCtx->extradata_size];
    par_size = pCodecCtx->extradata_size;
    memcpy(vps_pps_sps, pCodecCtx->extradata, pCodecCtx->extradata_size);

    encode(pCodecCtx, inframe, callback);
    av_frame_unref(inframe);

    encode(pCodecCtx, nullptr, callback);

#endif

    Heif *heif = new Heif();
    heif->setMajorBrand(HEIF::FourCC("mif1"));
    // heif->setMajorBrand(HEIF::FourCC("heix"));
    heif->addCompatibleBrand(HEIF::FourCC("heix"));

   

    HEVCDecoderConfiguration* config = new HEVCDecoderConfiguration(heif);
    HEIF::ErrorCode error = config->setConfig(vps_pps_sps, par_size);

    HEIF::MediaFormat format = config->getMediaFormat();
    
    HEVCCodedImageItem* imageItem = new HEVCCodedImageItem(heif);
    imageItem->setSize(width, height);
    Result r = imageItem->setDecoderConfiguration(config);
    imageItem->setItemData(h265raw, h265raw_size);
    

    heif->setPrimaryItem(imageItem);

    Result saveRet = heif->save(oFileName);

    if(saveRet != Result::OK){
        return -1;
    }

    return 0;
}

#endif