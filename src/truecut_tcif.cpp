 #include "truecut_tcif.h"
 #include <cstring>
#include <Heif.h>
#include <HEVCCodedImageItem.h>
#include <HEVCDecoderConfiguration.h>
#include <H26xTools.h>
#include <ImageSequence.h>
#include <VideoSample.h>
#include <vector>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/opt.h>
#include <libavformat/version.h>
}

using namespace HEIFPP;

class FFmpegOutStream : public HEIF::OutputStreamInterface{
public:
    FFmpegOutStream(AVIOContext *s, const char* filename)
        :m_s(s),
        mFilename(filename){
    }
    virtual void seekp(std::uint64_t aPos) override
    {
        avio_seek(m_s, aPos,SEEK_SET);
    }
    virtual std::uint64_t tellp() override
    {
        return avio_seek(m_s, 0, SEEK_CUR);
    }
    virtual void write(const void* aBuf, std::uint64_t aCount) override
    {
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
    AVIOContext *m_s;
    std::string mFilename;
};


static bool  parseNalu(const uint8_t* data,  uint64_t datalen, HEIF::Array<HEIF::DecoderSpecificInfo>& decoderSpecificInfo,  HEIF::Array<uint8_t>& hevcData ){
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

typedef struct TCIFHandle{
    Heif*  heif;
    uint32_t width;
    uint32_t height;
    int flag;
    uint32_t timeScale;
    uint64_t duration;
    HEIF::Array<HEIF::DecoderSpecificInfo>* decoderSpecificInfo;
    bool  specificInfo;
    std::vector<HEIF::Array<uint8_t>> *hevcDatas;
}TCIFHandle;

TCIFHandle* createHandle(const uint8_t* extradata, uint64_t extradata_size,  uint32_t width, uint32_t height){
        TCIFHandle* handle =  (TCIFHandle*)malloc(sizeof(TCIFHandle));
        handle->hevcDatas = new std::vector<HEIF::Array<uint8_t>>();
        handle->width = width;
        handle->height = height;
        Heif *heif = new Heif();
        heif->setMajorBrand("msf1");
        heif->addCompatibleBrand(HEIF::FourCC("heic"));
        heif->addCompatibleBrand(HEIF::FourCC("hevc"));
        heif->addCompatibleBrand(HEIF::FourCC("mif1"));
        heif->addCompatibleBrand(HEIF::FourCC("iso8"));
        heif->addCompatibleBrand(HEIF::FourCC("mp41"));
        handle->heif = heif;
        handle->decoderSpecificInfo = new  HEIF::Array<HEIF::DecoderSpecificInfo>(3);
        handle->specificInfo = false;
        if(extradata != NULL && extradata_size >0){
            HEIF::Array<uint8_t> hevcData;
            if(!parseNalu(extradata, extradata_size, *handle->decoderSpecificInfo, hevcData)){
                 return NULL;
            }
             handle->specificInfo = true;
        }
        return handle;
}
int configHandle(TCIFHandle* handle, int flag, uint32_t timeScale, uint64_t duration){
    if(handle == NULL){
        return -1;
    }
    handle->flag = flag;
    handle->timeScale = timeScale;
    handle->duration = duration;
    return 0;
}
/**
 * delete tcif handle
 */
void deleteHandle(TCIFHandle* handle){
    if(handle != NULL){
        if(handle->heif != NULL){
            delete handle->heif;
            handle->heif = NULL;
        }
        if(handle->decoderSpecificInfo != NULL){
            delete handle->decoderSpecificInfo;
            handle->decoderSpecificInfo = NULL;
        }
        if(handle->hevcDatas != NULL){
            handle->hevcDatas->clear();
            delete handle->hevcDatas;
            handle->hevcDatas = NULL;
        }
        free(handle);
        handle = NULL;
    }
}
/**
 * save tcif 
 */
int saveImage(TCIFHandle* handle,  AVIOContext* avioContext, const char* url){
    if(handle == NULL || handle->heif == NULL || !handle->specificInfo || handle->hevcDatas->size() == 0){
        return -1;
    }
    Heif* heif = handle->heif;
    DecoderConfig* config = new HEVCDecoderConfiguration(heif);
    config->setConfig(*handle->decoderSpecificInfo);
    switch(handle->flag){
    case TCIF_SINGLE_IMAGE_FLAG:{//single frame
        HEVCCodedImageItem* imageItem = new HEVCCodedImageItem(heif);
        imageItem->setSize(handle->width, handle->height);
        imageItem->setDecoderConfiguration(config);
        imageItem->setItemData(handle->hevcDatas->at(0).elements, handle->hevcDatas->at(0).size);
        heif->setPrimaryItem(imageItem);
        break;
    }   
    case TCIF_MULTIPLE_IMAGE_FLAG:{// more than one frame
        for(std::size_t i = 0; i<handle->hevcDatas->size(); i++){
            HEVCCodedImageItem* imageItem = new HEVCCodedImageItem(heif);
            imageItem->setSize(handle->width, handle->height);
            imageItem->setDecoderConfiguration(config);
            imageItem->setItemData(handle->hevcDatas->at(i).elements, handle->hevcDatas->at(i).size);
            if(heif->getPrimaryItem() == NULL){
                heif->setPrimaryItem(imageItem);
            }
        }
        break;
    }
    case TCIF_VIDEO_FLAG:{//video
        HEVCCodedImageItem* imageItem = new HEVCCodedImageItem(heif);
        imageItem->setSize(handle->width, handle->height);
        imageItem->setDecoderConfiguration(config);
        imageItem->setItemData(handle->hevcDatas->at(0).elements, handle->hevcDatas->at(0).size);
        heif->setPrimaryItem(imageItem);
        VideoTrack* videoTrack = new VideoTrack(heif);
        videoTrack->setTimescale(handle->timeScale);
        for (std::size_t i = 0; i < handle->hevcDatas->size(); ++i) {
            HEIFPP::VideoSample* imageSeqSample = new HEIFPP::VideoSample(heif);
            imageSeqSample->setType(HEIF::FourCC("hvc1"));
            imageSeqSample->setDecoderConfiguration(config);
            imageSeqSample->setItemData(handle->hevcDatas->at(i).elements, handle->hevcDatas->at(i).size);
            imageSeqSample->setDuration(handle->duration);
            videoTrack->addSample(imageSeqSample);
        }
        break;
    }
    default:
       return -1;
    }
    HEIF::OutputStreamInterface* out = new FFmpegOutStream(avioContext, url);
    HEIFPP::Result r = heif->save(out);
    delete out;

    return (r == Result::OK ? 0 : -1);
}

/**
 * add avpacket into tcif
 */ 
int  addAVPacket(TCIFHandle* handle, AVPacket* avpkt){
    if(handle == NULL || handle->hevcDatas == NULL || avpkt == NULL){
        return -1;
    }
    HEIF::Array<uint8_t> hevcData;
    if(!handle->specificInfo || (avpkt->flags & AV_PKT_FLAG_KEY)){
        if(!parseNalu(avpkt->data, avpkt->size, *handle->decoderSpecificInfo, hevcData)){
            av_log(NULL, AV_LOG_ERROR, "parse nulu error!");
            return -1;
        }
        handle->specificInfo = true;
    }else{
        hevcData  = HEIF::Array<uint8_t>(avpkt->size);
        std::memcpy(hevcData.elements, avpkt->data, avpkt->size);
    }
    handle->hevcDatas->push_back(hevcData);
    return 0;
}