#ifndef DECODE_HEIC_OUTPUT_YUV420_TEST
#define DECODE_HEIC_OUTPUT_YUV420_TEST
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
#include <ExifItem.h>
#include <XMPItem.h>
#include <exiv2/exiv2.hpp>
#include <iostream>

using namespace HEIFPP;

void decode_heic_output_yuv420()
{


    Heif *readHeif = new Heif();
    // const char* heif_file_name = "./test2.heif";
    // const char* heif_file_name = "./example.heic";
    // const char *heif_file_name = "pw_2020_05_13_16_20_42_707_cloud_process.heic";
    const char *heif_file_name = "1233.heic";
    if (readHeif->load(heif_file_name) != Result::OK)
    {
        return;
    }

    const HEIF::FourCC majorBrand = readHeif->getMajorBrand();
    uint32_t compatatibleBrand = readHeif->compatibleBrands();
    for (int i = 0; i < compatatibleBrand; i++)
    {
        const HEIF::FourCC comBrand = readHeif->getCompatibleBrand(i);
        printf("%s\n", comBrand.value);
    }

    uint32_t itemCount = readHeif->getItemCount();
    uint8_t buffer[128] = {0};
    for (int i = 0; i < itemCount; i++)
    {
        Item *item = readHeif->getItem(i);
        if (item->isExifItem())
        {
            ExifItem *exifItem = (ExifItem *)item;
            uint64_t datasize = exifItem->getDataSize();
            const uint8_t *data = exifItem->getData();
            memcpy(buffer, data, datasize);
        }
        else if (item->isXMPItem())
        {
            XMPItem *xmpItem = (XMPItem *)item;
            std::string str((const char *)xmpItem->getData(), xmpItem->getDataSize());
            Exiv2::XmpData xmpData;
            if (0 != Exiv2::XmpParser::decode(xmpData, str))
            {
                throw Exiv2::Error(Exiv2::kerErrorMessage, "Failed to decode XMP data");
            }
            for (auto iter = xmpData.begin(); iter != xmpData.end(); iter++)
            {
                std::string key = iter->key();
                const Exiv2::Value &value = iter->value();
            }
            bool trueCut = false;
            if (xmpData.findKey(Exiv2::XmpKey("Xmp.dc.TrueCut")) != xmpData.end())
            {
                trueCut = xmpData["Xmp.dc.TrueCut"].value().toLong() == 1 ? true : false;
            }
            const Exiv2::Value &corporation_value = xmpData["Xmp.dc.corporation"].value();
            auto iter = xmpData.findKey(Exiv2::XmpKey("Xmp.dc.xxx"));
            if (iter != xmpData.end())
            {
                const Exiv2::Value &xxx_value = xmpData["Xmp.dc.xxx"].value();
            }
            std::string corporation = corporation_value.toString();
            std::cout << corporation << std::endl;
        }
    }

    uint32_t imageCount = readHeif->getImageCount();
    for (int i = 0; i < imageCount; i++)
    {
        ImageItem *imageItem = readHeif->getImage(i);
        int width = imageItem->width();
        int height = imageItem->height();

        printf("====ImageItem: name:%s, w:%d, h:%d, isThumb:%d, address:%p\n", imageItem->getName().c_str(), width, height, imageItem->isThumbnailImage() ? 1 : 0, imageItem);
    }

    HEVCCodedImageItem *primaryItem = (HEVCCodedImageItem *)readHeif->getPrimaryItem();
    uint32_t propertyCount = readHeif->getPropertyCount();
    for (int i = 0; i < propertyCount; i++)
    {
        ItemProperty *property = readHeif->getProperty(i);
    }

    uint32_t metadataCount = primaryItem->getMetadataCount();
    for (int i = 0; i < metadataCount; i++)
    {
        MetaItem *mataItem = primaryItem->getMetadata(i);
    }

    uint32_t thumbnailsCount = primaryItem->getThumbnailCount();
    for (int i = 0; i < thumbnailsCount; i++)
    {
        ImageItem *thumb = primaryItem->getThumbnail(i);
        uint32_t width = thumb->width();
        uint32_t height = thumb->height();
        printf("====thumb :w:%d, h:%d, address:%p\n", width, height, thumb);

        if (thumb->isCodedImage())
        {
            CodedImageItem *codeImageItem = (CodedImageItem *)thumb;
            HEIF::MediaFormat format = codeImageItem->getMediaFormat();
            HEIF::FourCC fourCC = codeImageItem->getDecoderCodeType();
            if (format == HEIF::MediaFormat::HEVC)
            {
                HEVCCodedImageItem *hevcCodedImageItem = (HEVCCodedImageItem *)codeImageItem;
                DecoderConfig *decoderConfig = hevcCodedImageItem->getDecoderConfiguration();
            }
        }
    }

    int width = primaryItem->width();
    int height = primaryItem->height();
    DecoderConfig *readerDecoderConfig = primaryItem->getDecoderConfiguration();
    uint32_t vps_pps_sps_size;
    uint8_t *vps_pps_sps;
    readerDecoderConfig->getConfig(vps_pps_sps, vps_pps_sps_size);
    const unsigned char *imageData = primaryItem->getItemData();
    uint64_t datasize = primaryItem->getItemDataSize();

    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H265);
    AVCodecContext *avcodecCtx = avcodec_alloc_context3(codec);

    avcodecCtx->extradata_size = vps_pps_sps_size;
    avcodecCtx->extradata = (uint8_t *)malloc(avcodecCtx->extradata_size);
    memcpy(avcodecCtx->extradata, vps_pps_sps, avcodecCtx->extradata_size);

    if (avcodec_open2(avcodecCtx, codec, nullptr) < 0)
    {
        return;
    }
    AVPacket pkt;
    av_init_packet(&pkt);

    // uint8_t* data = new uint8_t[vps_pps_sps_size+datasize];

    // memcpy(data, vps_pps_sps, vps_pps_sps_size);
    // memcpy(data+vps_pps_sps_size, imageData, datasize);

    uint8_t *data = new uint8_t[datasize];
    memcpy(data, imageData, datasize);

    av_packet_from_data(&pkt, data, datasize);
    FILE *oFile = fopen("./out_352x288_one_frame.yuv", "wb");
    auto decodecallback = [&](AVCodecContext *ctx, const AVFrame *frame) {
        WriteYUV420P10LEToFile(frame, oFile);
        printf("解码成功\n");
    };
    decode(avcodecCtx, &pkt, decodecallback);
    av_packet_unref(&pkt);

    decode(avcodecCtx, nullptr, decodecallback);

    avcodec_close(avcodecCtx);

    fclose(oFile);
}

#endif