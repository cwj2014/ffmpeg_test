#ifndef DECODE_AUDIO_OUTPUT_PCM_TEST_H_H_
#define DECODE_AUDIO_OUTPUT_PCM_TEST_H_H_

#include "global.h"
#include "codecimpl.h"
#include <memory>
#include "audio_convert_tool.h"
#include "avframe_util.h"

//解码音频并将PCM数据写入文件
int decode_audio_output_pcm_test(){
    const char * inFileName = "./半壶纱.mp4";
    AVFormatContext * avformatctx = nullptr;
    if(avformat_open_input(&avformatctx, inFileName, nullptr, nullptr)<0){
        return -1;
    }
    int audio_stream_index = av_find_best_stream(avformatctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audio_stream_index < 0){
        return -1;
    }
    AVStream* stream = avformatctx->streams[audio_stream_index];
    const AVCodec* pCodec =  avcodec_find_decoder(stream->codecpar->codec_id);
    AVCodecContext *audioContext = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(audioContext, stream->codecpar);

    if(avcodec_open2(audioContext, pCodec, nullptr)<0){
        return -1;
    }
    //原始PCM格式
    FILE* pcmFile = fopen("./t_44100_2_float.pcm", "wb");
    FILE* convertPcmFile = fopen("./t_16000_1_s16.pcm", "wb");

    std::shared_ptr<SwrCtxManager> swrCtxManager = std::make_shared<SwrCtxManager>(audioContext->channel_layout, audioContext->sample_rate,
            (AVSampleFormat)audioContext->sample_fmt, AV_CH_LAYOUT_MONO, 16000, AV_SAMPLE_FMT_S16);
    if(!swrCtxManager->Init()){
        return -1;
    }

    AVFrame* outframe = av_frame_alloc();

    auto initFrameWidthData = [&](uint64_t channel_layout, int format, int sample_rate, int frame_size, void** frame_data){
        outframe->nb_samples = frame_size;
        outframe->channel_layout = channel_layout;
        outframe->format = format;
        outframe->sample_rate = sample_rate;
        av_frame_get_buffer(outframe, 0);//这个函数会根据channel_layout初始化channel(av_get_channel_layout_nb_channels)
        int datasize = av_get_bytes_per_sample((AVSampleFormat)outframe->format);
        for(int i=0; i<outframe->nb_samples; i++){
            for(int j=0; j<outframe->channels; j++){
                memcpy(outframe->data[j] + i*datasize, frame_data[j]+ i*datasize, datasize);
            }
        }
    };

    auto callback = [&](AVCodecContext *ctx, const AVFrame* frame){
        WritePCMToFile(frame, pcmFile);
        int ret = swrCtxManager->Convert((const uint8_t**)frame->extended_data, frame->nb_samples);
        if(ret > 0){
            initFrameWidthData(AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, 16000, ret, (void**)swrCtxManager->GetConvertedBuffer());
            WritePCMToFile(outframe, convertPcmFile);
            av_frame_unref(outframe);
        }
    };
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    while(1){
        int ret = av_read_frame(avformatctx, packet);
        if(ret!=0){
            printf("read error or file end\n");
            break;
        }
        if(packet->stream_index==audio_stream_index){
            decode(audioContext, packet, callback);
        }
        
        av_packet_unref(packet);
    }

    decode(audioContext, nullptr, callback);
    
    avcodec_close(audioContext);

    avformat_close_input(&avformatctx);

    av_packet_free(&packet);

    av_frame_free(&outframe);
    
    fclose(pcmFile);

    return 0;
}


#endif