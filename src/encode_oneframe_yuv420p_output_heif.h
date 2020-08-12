#ifndef ENCODE_ONEFRAME_YUV420_OUTPUT_HEIF_H_H_
#define ENCODE_ONEFRAME_YUV420_OUTPUT_HEIF_H_H_
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
#include <cstring>
#include <ExifItem.h>
#include <XMPItem.h>
#include <MPEG7Item.h>
#include <exiv2/exiv2.hpp>

using namespace HEIFPP;


class FFmpegOutStream : public HEIF::OutputStreamInterface{
public:
    FFmpegOutStream(AVIOContext *s, const char* filename)
        :m_s(s),
        mFilename(filename){
    }
    ~FFmpegOutStream(){
        printf("~FFmpegOutStream \n");
    }
    virtual void seekp(std::uint64_t aPos) override
    {
        avio_seek(m_s, aPos,SEEK_SET);
        printf("====seekp==%llu\n", aPos);
    }
    virtual std::uint64_t tellp() override
    {
        std::uint64_t postion =  avio_seek(m_s, 0, SEEK_CUR);
        printf("====tellp==%llu\n", postion);
        return postion;
    }
    virtual void write(const void* aBuf, std::uint64_t aCount) override
    {
        printf("====write:%llu==\n", aCount);
        avio_write(m_s, (const unsigned char*)aBuf, aCount);
    }
    virtual void remove() override
    {
        if (!mFilename.empty())
        {
            avpriv_io_delete(mFilename.c_str());
        }
    }
private:
    std::string mFilename;
    AVIOContext *m_s;
};

bool  parseNalu(const uint8_t* data,  uint64_t datalen, HEIF::Array<HEIF::DecoderSpecificInfo>& decoderSpecificInfo,  HEIF::Array<uint8_t>& hevcData ){
    NAL_State d;
    d.init_parse(data, datalen);
    int  flags = 0;
    for(;;){
        const std::uint8_t* nal_data = nullptr;
        std::uint64_t nal_len        = 0;
        if (!d.parse_byte_stream(nal_data, nal_len))
        {
            break;
        }
        int type;
        type                =  (nal_data[0] >> 1) & 0x3f;
        if(( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::PREFIX_SEI_NUT || ( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::SUFFIX_SEI_NUT){
            continue;
        }
        if(( HEIF::DecoderSpecInfoType)type ==  HEIF::DecoderSpecInfoType::HEVC_VPS ||( HEIF::DecoderSpecInfoType) type == HEIF::DecoderSpecInfoType::HEVC_SPS || ( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::HEVC_PPS){
                std::uint32_t index = 0;
                if (( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::HEVC_VPS)
                    index = 0;
                else if (( HEIF::DecoderSpecInfoType)type == HEIF::DecoderSpecInfoType::HEVC_SPS)
                    index = 1;
                else
                    index = 2;
                flags |= 1u << index;
                decoderSpecificInfo[index].decSpecInfoType    = ( HEIF::DecoderSpecInfoType)type;
                decoderSpecificInfo[index].decSpecInfoData    = HEIF::Array<uint8_t>(nal_len + 4);
                decoderSpecificInfo[index].decSpecInfoData[0] = decoderSpecificInfo[index].decSpecInfoData[1] =
                decoderSpecificInfo[index].decSpecInfoData[2] = 0;
                decoderSpecificInfo[index].decSpecInfoData[3]     = 1;
                std::memcpy(decoderSpecificInfo[index].decSpecInfoData.elements + 4, nal_data, nal_len);
        }else if( type == 16 || type == 17 || type == 18 ||
                     type == 19 || type == 20 || type == 21 ){
                    hevcData = HEIF::Array<uint8_t>(nal_len + 4);
                     hevcData[0] = hevcData[1] = hevcData[2] = 0;
                     hevcData[3] = 1;
                     std::memcpy(hevcData.elements + 4, nal_data, nal_len);
         }else{
               return false;
         }
       
    }

    if (flags > 0 && flags != 7 )
    {
        return false;
    }
    return true;
}

int encode_yuv420_output_heif(){

    uint8_t *vps_pps_sps = new uint8_t[1024];
    unsigned int par_size = 0;
    unsigned char *h265raw;  

    uint64_t h265raw_size = 0; 

    HEIF::Array<HEIF::DecoderSpecificInfo> decoderSpecificInfo  = HEIF::Array<HEIF::DecoderSpecificInfo>(3);
    HEIF::Array<uint8_t> hevcData;

    AVFormatContext* avformatctx = NULL;


    auto callback = [&](AVCodecContext* ctx,const AVPacket* avpkt){

        // parseNalu(avpkt->data, avpkt->size, decoderSpecificInfo, hevcData);

        AVPacket* pkt = av_packet_clone(avpkt);
        av_write_frame(avformatctx, pkt);
        av_packet_unref (pkt);
        return;

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

    const char * oFileName = "./test.heic";
    //  const char * oFileName = "./test.txt";

    FILE* rFile = fopen("./akiyo_cif.yuv","rb");
	if(rFile == NULL)
		return -1;
    
    
    if(avformat_alloc_output_context2(&avformatctx, NULL, NULL, oFileName)  < 0)  {
         return -1;
    }

    AVStream* vStream = avformat_new_stream(avformatctx, nullptr);
    if(vStream == nullptr){
        return -1;
    }
    
    printf("index:%d, n: %d\n",vStream->index, avformatctx->nb_streams);

    if(avio_open(&avformatctx->pb, oFileName, AVIO_FLAG_READ_WRITE)<0){
        return -1;
    }

    const AVCodec *codec = avcodec_find_encoder(avformatctx->oformat->video_codec);
	
    
    int width = 352;
    int height = 288;


    // const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H265);

    AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
    InitVideoAVCodecCtx(pCodecCtx, AV_CODEC_ID_H265, width, height);
    
    int ret = avcodec_open2(pCodecCtx, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
		exit(1);
	}

   avcodec_parameters_from_context(vStream->codecpar, pCodecCtx);

    if(avformat_write_header(avformatctx, nullptr)<0){
        return -1;
    }

    
    AVFrame* inframe = av_frame_alloc();

    inframe->format = AV_PIX_FMT_YUV420P;
	inframe->width  = width;
	inframe->height = height;
	av_frame_get_buffer(inframe, 32);

	ReadYUV420FromFile(inframe, rFile);//从yuv文件填充AVFrame
    inframe->pts = 0;
    encode(pCodecCtx, inframe, callback);
    av_frame_unref(inframe);

    encode(pCodecCtx, nullptr, callback);


     av_write_trailer(avformatctx);

    avcodec_close(pCodecCtx);
    av_frame_free(&inframe);

    avio_close(avformatctx->pb);
	avformat_free_context(avformatctx);

    /*Heif *heif = new Heif();
    heif->setMajorBrand(HEIF::FourCC("mif1"));
    heif->addCompatibleBrand(HEIF::FourCC("heic"));

    DecoderConfig* config = new HEVCDecoderConfiguration(heif);
    // HEIF::ErrorCode error = config->setConfig(vps_pps_sps, par_size);
    
   if(!parseNalu(pCodecCtx->extradata, pCodecCtx->extradata_size, decoderSpecificInfo, hevcData)){
       return -1;
   }

    HEIF::ErrorCode error = config->setConfig(decoderSpecificInfo);
    
    HEVCCodedImageItem* imageItem = new HEVCCodedImageItem(heif);
    imageItem->setSize(width, height);
    Result r = imageItem->setDecoderConfiguration(config);
    // imageItem->setItemData(h265raw, h265raw_size);  
    imageItem->setItemData(hevcData.elements, hevcData.size);

    // imageItem->addThumbnail()

    heif->setPrimaryItem(imageItem);

    
    ExifItem* exifItem = new ExifItem(heif);
    char* exifdata = "1234567890";
    exifItem->setData((uint8_t*)exifdata, strlen(exifdata));
   // heif->addItem(exifItem);

    Exiv2::XmpData xmpData;
    

    // xmpData["Xmp.dc.source"]  = "xmpsample.cpp";    // a simple text value
    // xmpData["Xmp.dc.subject"] = "Palmtree";         // an array item
    // xmpData["Xmp.dc.subject"] = "Rubbertree";       // add a 2nd array item

    xmpData["Xmp.dc.corporation"] = "pixelworks";
    xmpData["Xmp.dc.TrueCut"] = true;
    
    std::string xmpPacket;
    if (0 != Exiv2::XmpParser::encode(xmpPacket, xmpData)) {
        throw Exiv2::Error(Exiv2::kerErrorMessage, "Failed to serialize XMP data");
    }

    XMPItem* xmpItem = new XMPItem(heif);
    char* xmpdata = "xmp test";
    xmpItem->setData((const uint8_t*)xmpPacket.c_str(), xmpPacket.length());
    //heif->addItem(xmpItem);

    MPEG7Item* mpeg7Item = new MPEG7Item(heif);
    char* mpeg7data = "mpeg7 test";
    int len = strlen(mpeg7data);
    mpeg7Item->setData((uint8_t*)mpeg7data, len);
   // heif->addItem(mpeg7Item);



    // HEVCDecoderConfiguration* config2 = new HEVCDecoderConfiguration(heif);
    // error = config2->setConfig(vps_pps_sps, par_size);
    
    // HEVCCodedImageItem* imageItem2 = new HEVCCodedImageItem(heif);
    // imageItem2->setSize(width, height);
    // r = imageItem2->setDecoderConfiguration(config2);
    // imageItem2->setItemData(h265raw, h265raw_size);

    //heif->addItem(imageItem2);
   AVIOContext* avio = NULL;
   if(avio_open(&avio, oFileName, AVIO_FLAG_WRITE | AVIO_FLAG_DIRECT)<0){
        return -1;
    }

//   const  char* s1 = "11111111111111111111";
//   avio_write(avio,(const unsigned char*)s1, strlen(s1));
//   avio_tell(avio);
//   avio_seek(avio, 2,SEEK_SET);
//   const  char* s2 = "2222";
//   avio_write(avio,(const unsigned char*)s2, strlen(s2));

//   avio_closep(&avio);

//   return 0;

   
    
    HEIF::OutputStreamInterface* aStream = new FFmpegOutStream(avio, oFileName);

    // Result saveRet = heif->save(oFileName);

    Result saveRet = heif->save(aStream);

    if(saveRet != Result::OK){
        return -1;
    }

    // aStream->remove();

    delete aStream;
    avio_close(avio);

    // HEIF::Writer* writer = HEIF::Writer::Create();

    // // partially configure writer output
    // HEIF::OutputConfig writerOutputConf{};
    // writerOutputConf.fileName        = oFileName;
    // writerOutputConf.progressiveFile = true;
    // writerOutputConf.majorBrand = HEIF::FourCC("mif1");
    // // add compatible brands to writer config
    // HEIF::Array<HEIF::FourCC> inputCompatibleBrands = HEIF::Array<HEIF::FourCC>(1);
    // inputCompatibleBrands[0] = HEIF::FourCC("heic");
    // writerOutputConf.compatibleBrands = inputCompatibleBrands;

    // if(HEIF::ErrorCode::OK !=writer->initialize(writerOutputConf)){
    //     return -1;
    // }

    // HEIF::DecoderConfiguration mConfig{};

    // auto convertFromRawData = [&](const std::uint8_t* aData, std::uint32_t aSize){
    //         // ISO / IEC 14496 - 15:2017 8.3.3.1 HEVC decoder configuration record :
    //         // It is recommended that the arrays be in the order VPS, SPS, PPS, prefix SEI, suffix SEI.

    //         mConfig.decoderSpecificInfo = HEIF::Array<HEIF::DecoderSpecificInfo>(3);

    //         // NOTE: only VPS,PPS,SPS is saved here, and we expect all three to exist.
    //         HEIFPP::NAL_State d;
    //         std::uint32_t flags;
    //         d.init_parse(aData, aSize);
    //         flags = 0;
    //         for (;;)
    //         {
    //             const std::uint8_t* nal_data = nullptr;
    //             std::uint64_t nal_len        = 0;
    //             if (!d.parse_byte_stream(nal_data, nal_len))
    //             {
    //                 break;
    //             }
    //             HEIF::DecoderSpecInfoType type;
    //             type                = (HEIF::DecoderSpecInfoType)((nal_data[0] >> 1) & 0x3f);
    //             std::uint32_t index = 0;
    //             if (type == HEIF::DecoderSpecInfoType::HEVC_VPS)
    //                 index = 0;
    //             else if (type == HEIF::DecoderSpecInfoType::HEVC_SPS)
    //                 index = 1;
    //             else if (type == HEIF::DecoderSpecInfoType::HEVC_PPS)
    //                 index = 2;
    //             else
    //             {
    //                 return HEIF::ErrorCode::MEDIA_PARSING_ERROR;
    //             }
    //             if ((flags & (1u << index)) != 0)
    //             {
    //                 return HEIF::ErrorCode::MEDIA_PARSING_ERROR;
    //             }
    //             flags |= 1u << index;
    //             mConfig.decoderSpecificInfo[index].decSpecInfoType    = type;
    //             mConfig.decoderSpecificInfo[index].decSpecInfoData    = HEIF::Array<uint8_t>(nal_len + 4);
    //             mConfig.decoderSpecificInfo[index].decSpecInfoData[0] = mConfig.decoderSpecificInfo[index].decSpecInfoData[1] =
    //                 mConfig.decoderSpecificInfo[index].decSpecInfoData[2] = 0;
    //             mConfig.decoderSpecificInfo[index].decSpecInfoData[3]     = 1;
    //             std::memcpy(mConfig.decoderSpecificInfo[index].decSpecInfoData.elements + 4, nal_data, nal_len);
    //         }
    //         if (flags != 7)
    //         {
    //             return HEIF::ErrorCode::MEDIA_PARSING_ERROR;
    //         }
    //         return HEIF::ErrorCode::OK;
    // };


    // HEIF::ErrorCode retCode = convertFromRawData(vps_pps_sps, par_size);

    // // feed new decoder config to writer and store input to output id pairing information
    // HEIF::DecoderConfigId outputDecoderConfigId;
    // retCode = writer->feedDecoderConfig(mConfig.decoderSpecificInfo, outputDecoderConfigId);
    

    // HEIF::Data imageData{};
    // imageData.size = h265raw_size;
    // imageData.data = new uint8_t[imageData.size];
    // std::memcpy(imageData.data, h265raw, imageData.size);
    // // feed image data to writer
    // imageData.mediaFormat     = HEIF::MediaFormat::HEVC;
    // imageData.decoderConfigId = outputDecoderConfigId;
    // HEIF::MediaDataId outputMediaId;
    // if(HEIF::ErrorCode::OK !=writer->feedMediaData(imageData, outputMediaId)){
    //     return -1;
    // }
    // delete[] imageData.data;

    // // create new image based on that data:
    // HEIF::ImageId outputImageId;
    // if(HEIF::ErrorCode::OK !=writer->addImage(outputMediaId, outputImageId)){
    //     return -1;
    // }

    // // if this input image was the primary image -> also mark output image as primary image
    // if(HEIF::ErrorCode::OK !=writer->setPrimaryItem(outputImageId)){
    //     return -1;
    // }
    
    // if(HEIF::ErrorCode::OK != writer->finalize()){
    //     printf("failed\n");
    // }

    // HEIF::Writer::Destroy(writer);
   */

    return 0;
}

#endif