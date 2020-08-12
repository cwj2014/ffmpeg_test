#ifndef ENCODE_FRAMES_YUV420P_OUTPUT_HEIF_H_H_
#define ENCODE_FRAMES_YUV420P_OUTPUT_HEIF_H_H_
#include "global.h"
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include "avframe_util.h"
#include "codecimpl.h"
#include <Heif.h>
#include <heifwriter.h>
#include <HEVCCodedImageItem.h>
#include <HEVCDecoderConfiguration.h>
#include <H26xTools.h>
#include <ImageSequence.h>
#include <VideoSample.h>
#include <cstring>
#include <ExifItem.h>
#include <XMPItem.h>
#include <MPEG7Item.h>
#include <exiv2/exiv2.hpp>
#include <vector>

using namespace HEIFPP;


// static bool  parseNalu(const uint8_t* data,  uint64_t datalen, HEIF::Array<HEIF::DecoderSpecificInfo>& decoderSpecificInfo,  HEIF::Array<uint8_t>& hevcData ){
//     NAL_State d;
//     d.init_parse(data, datalen);
//     int  flags = 0;
//     for(;;){
//         const std::uint8_t* nal_data = nullptr;
//         std::uint64_t nal_len        = 0;
//         if (!d.parse_byte_stream(nal_data, nal_len))
//         {
//             break;
//         }
//         int type;
//         type                =  (nal_data[0] >> 1) & 0x3f;
//         if(( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::PREFIX_SEI_NUT || ( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::SUFFIX_SEI_NUT){
//             continue;
//         }
//         if(( HEIF::DecoderSpecInfoType)type ==  HEIF::DecoderSpecInfoType::HEVC_VPS ||( HEIF::DecoderSpecInfoType) type == HEIF::DecoderSpecInfoType::HEVC_SPS || ( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::HEVC_PPS){
//                 std::uint32_t index = 0;
//                 if (( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::HEVC_VPS)
//                     index = 0;
//                 else if (( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::HEVC_SPS)
//                     index = 1;
//                 else
//                     index = 2;
//                 flags |= 1u << index;
//                 decoderSpecificInfo[index].decSpecInfoType    = ( HEIF::DecoderSpecInfoType)type;
//                 decoderSpecificInfo[index].decSpecInfoData    = HEIF::Array<uint8_t>(nal_len + 4);
//                 decoderSpecificInfo[index].decSpecInfoData[0] = decoderSpecificInfo[index].decSpecInfoData[1] =
//                 decoderSpecificInfo[index].decSpecInfoData[2] = 0;
//                 decoderSpecificInfo[index].decSpecInfoData[3]     = 1;
//                 std::memcpy(decoderSpecificInfo[index].decSpecInfoData.elements + 4, nal_data, nal_len);
//         }else if( type == 16 || type == 17 || type == 18 ||
//                      type == 19 || type == 20 || type == 21 ){
//                     hevcData = HEIF::Array<uint8_t>(nal_len + 4);
//                      hevcData[0] = hevcData[1] = hevcData[2] = 0;
//                      hevcData[3] = 1;
//                      std::memcpy(hevcData.elements + 4, nal_data, nal_len);
//          }else{
//                return false;
//          }
       
//     }

//     if (flags > 0 && flags != 7 )
//     {
//         return false;
//     }
//     return true;
// }

int encode_yuv420_output_heif2(){

    const char * oFileName = "./testsequeue.heic";
    FILE* rFile = fopen("./akiyo_cif.yuv","rb");
	if(rFile == NULL)
		return -1;
    int width = 352;
    int height = 288;

    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H265);
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);

    InitVideoAVCodecCtx(pCodecCtx, AV_CODEC_ID_H265, width, height);
    //pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    av_opt_set(pCodecCtx->priv_data, "preset", "slow", 0);
    av_opt_set(pCodecCtx->priv_data, "x265-params", "no-info=true", 0);

    int ret = avcodec_open2(pCodecCtx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

    HEIF::Array<HEIF::DecoderSpecificInfo> decoderSpecificInfo  = HEIF::Array<HEIF::DecoderSpecificInfo>(3);
    // HEIF::Array<uint8_t> hevcData;
    // if(!parseNalu(pCodecCtx->extradata, pCodecCtx->extradata_size, decoderSpecificInfo, hevcData)){
    //    return -1;
    // }
    bool b = false;
    std::vector<HEIF::Array<uint8_t>> hevcDatas;

    auto callback = [&](AVCodecContext* ctx,const AVPacket* avpkt){
            HEIF::Array<uint8_t> hevcData;
            if(!b || (avpkt->flags & AV_PKT_FLAG_KEY)){//avpkt->flags & AV_PKT_FLAG_KEY 每个Ｉ帧前都有可以有sps和pps，如果不把它们拆分出来直接保存到videosample中会造成花屏
                printf("=======1======\n");
                if(!parseNalu(avpkt->data, avpkt->size, decoderSpecificInfo, hevcData)){
                    printf("=======2======\n");
                }else{
                    b = true;
                }
            }else{
                hevcData = HEIF::Array<uint8_t>(avpkt->size);
                std::memcpy(hevcData.elements, avpkt->data, avpkt->size);
            }
            hevcDatas.push_back(hevcData);
    };

    AVFrame* inframe = av_frame_alloc();
    int i=0;
    while(1){
		if(feof(rFile))
			break;

		inframe->format = AV_PIX_FMT_YUV420P;
		inframe->width  = width;
		inframe->height = height;
		av_frame_get_buffer(inframe, 32);

		ReadYUV420FromFile(inframe, rFile);//从yuv文件填充AVFrame

        inframe->pts = i++;
        encode(pCodecCtx, inframe, callback);

        av_frame_unref(inframe);
    }
    
    encode(pCodecCtx, nullptr, callback);


    avcodec_close(pCodecCtx);
    av_frame_free(&inframe);
 
	fclose(rFile);

    Heif *heif = new Heif();
    // heif->setMajorBrand(HEIF::FourCC("mif1"));
    heif->setMajorBrand("msf1");
    heif->addCompatibleBrand(HEIF::FourCC("heic"));
    heif->addCompatibleBrand(HEIF::FourCC("hevc"));
    heif->addCompatibleBrand(HEIF::FourCC("mif1"));
    heif->addCompatibleBrand(HEIF::FourCC("iso8"));

    DecoderConfig* config = new HEVCDecoderConfiguration(heif);
    HEIF::ErrorCode error = config->setConfig(decoderSpecificInfo);


    VideoTrack* videoTrack = new VideoTrack(heif);
    videoTrack->setTimescale(1000);
    for (int i = 0; i < hevcDatas.size(); ++i) {
        HEIFPP::VideoSample* imageSeqSample = new HEIFPP::VideoSample(heif);
        imageSeqSample->setType(HEIF::FourCC("hvc1"));
        imageSeqSample->setDecoderConfiguration(config);
        imageSeqSample->setItemData(hevcDatas[i].elements, hevcDatas[i].size);
        imageSeqSample->setDuration(40);
        videoTrack->addSample(imageSeqSample);
    }

    HEVCCodedImageItem* imageItem = new HEVCCodedImageItem(heif);
    imageItem->setSize(width, height);
    Result r = imageItem->setDecoderConfiguration(config); 
    imageItem->setItemData(hevcDatas[0].elements, hevcDatas[0].size);
    heif->setPrimaryItem(imageItem);

    Result saveRet = heif->save(oFileName);

    if(saveRet != Result::OK){
        return -1;
    }   
    return 0;
}

#endif