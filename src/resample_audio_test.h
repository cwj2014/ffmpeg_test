#ifndef RESAMPLE_AUDIO_TEST_H_H_
#define RESAMPLE_AUDIO_TEST_H_H_

#include "global.h"
#include "codecimpl.h"
#include <memory>
#include "audio_convert_tool.h"


/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec *codec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;

    if (!codec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;

    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);

        if (nb_channels > best_nb_channels) {
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}


/**
 * Initialize one input frame for writing to the output file.
 * The frame will be exactly frame_size samples large.
 * @param[out] frame                Frame to be initialized
 * @param      output_codec_context Codec context of the output file
 * @param      frame_size           Size of the frame
 * @return Error code (0 if successful)
 */
static int init_output_frame(AVFrame **frame,
                             AVCodecContext *output_codec_context,
                             int frame_size)
{
    int error;

    /* Create a new frame to store the audio samples. */
    if (!(*frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate output frame\n");
        return AVERROR_EXIT;
    }

    /* Set the frame's parameters, especially its size and format.
     * av_frame_get_buffer needs this to allocate memory for the
     * audio samples of the frame.
     * Default channel layouts based on the number of channels
     * are assumed for simplicity. */
    (*frame)->nb_samples     = frame_size;
    (*frame)->channel_layout = output_codec_context->channel_layout;
    (*frame)->format         = output_codec_context->sample_fmt;
    (*frame)->sample_rate    = output_codec_context->sample_rate;

    /* Allocate the samples of the created frame. This call will make
     * sure that the audio frame can hold as many samples as specified. */
    if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
        fprintf(stderr, "Could not allocate output frame samples (error '%s')\n",
                av_err2str(error));
        av_frame_free(frame);
        return error;
    }

    return 0;
}


int resample_audio_test(){
    const char * inFileName = "./半壶纱.mp4";
    AVFormatContext * avformatctx = nullptr;
    if(avformat_open_input(&avformatctx, inFileName, nullptr, nullptr)<0){
        return -1;
    }
    avformat_find_stream_info(avformatctx, nullptr);
    AVCodecContext *audioContext = nullptr;
    int audio_stream_index, video_stream_index;
    for(int i=0; i<avformatctx->nb_streams; i++){
        AVStream* stream = avformatctx->streams[i];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            const AVCodec* pCodec =  avcodec_find_decoder(stream->codecpar->codec_id);
            audioContext = avcodec_alloc_context3(pCodec);
            avcodec_parameters_to_context(audioContext, stream->codecpar);
            av_codec_set_pkt_timebase(audioContext, stream->time_base);

            if(avcodec_open2(audioContext, pCodec, nullptr)<0){
               return -1;
            }
            audio_stream_index = i;
            break;
        }     
    }
    
    const char* oFileName = "test.MP3";
    AVFormatContext* oAVformatCtx = nullptr;
    if(avformat_alloc_output_context2(&oAVformatCtx, nullptr, nullptr, oFileName)<0){
        return -1;
    }
    AVStream* oAudioStream = avformat_new_stream(oAVformatCtx, nullptr);
    if(oAudioStream==nullptr){
        return -1;
    }

    if(avio_open(&oAVformatCtx->pb, oFileName, AVIO_FLAG_READ_WRITE)<0){
        return -1;
    }
    const AVCodec *codec = avcodec_find_encoder(oAVformatCtx->oformat->audio_codec);
    AVCodecContext* outputCodecCtx = avcodec_alloc_context3(codec);
    if (!outputCodecCtx) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }
    /* put sample parameters */
    outputCodecCtx->bit_rate = 32000;
    /* check that the encoder supports s16 pcm input */
    outputCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    if (!check_sample_fmt(codec, outputCodecCtx->sample_fmt)) {
        fprintf(stderr, "Encoder does not support sample format %s",
                av_get_sample_fmt_name(outputCodecCtx->sample_fmt));
        exit(1);
    }
    /* select other audio parameters supported by the encoder */
    outputCodecCtx->sample_rate    = 16000;//select_sample_rate(codec);
    outputCodecCtx->channel_layout = select_channel_layout(codec);
    outputCodecCtx->channels       = av_get_channel_layout_nb_channels(outputCodecCtx->channel_layout);
    outputCodecCtx->time_base = AVRational{1, outputCodecCtx->sample_rate};

    /* open it */
    if (avcodec_open2(outputCodecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    //非常重要
    avcodec_parameters_from_context(oAudioStream->codecpar, outputCodecCtx);

    avformat_write_header(oAVformatCtx, nullptr);
    
    std::shared_ptr<SwrCtxManager> swrCtxManager = nullptr;
    if(audioContext->sample_rate != outputCodecCtx->sample_rate ||
       audioContext->channel_layout != outputCodecCtx->channel_layout ||
       audioContext->sample_fmt != outputCodecCtx->sample_fmt){ //先做重采样再mp3编码
              
        swrCtxManager = std::make_shared<SwrCtxManager>(audioContext->channel_layout, audioContext->sample_rate, (AVSampleFormat)audioContext->sample_fmt, outputCodecCtx->channel_layout, outputCodecCtx->sample_rate, (AVSampleFormat)outputCodecCtx->sample_fmt);
        if(!swrCtxManager->Init()){
            return -1;
        }    
    }

    AVAudioFifo *fifo = av_audio_fifo_alloc(outputCodecCtx->sample_fmt, outputCodecCtx->channels, 1);
    
    
    auto encodeCallback = [&](AVCodecContext *ctx, const AVPacket* avpkt){
        AVPacket* pkt = av_packet_clone(avpkt);
        pkt->stream_index = oAudioStream->index;
        av_interleaved_write_frame(oAVformatCtx, pkt);
        av_packet_unref(pkt);
    };
    
    //是否读文件结束
    bool read_eof = false;

    auto callback = [&](AVCodecContext *ctx, const AVFrame* frame){
        if(swrCtxManager != nullptr){ //转换
            int ret = swrCtxManager->Convert((const uint8_t**)frame->extended_data, frame->nb_samples);//wr_convert(swrCtxManager->swr_ctx, dst_data, dst_nb_samples, (const uint8_t**)frame->extended_data, frame->nb_samples);
            if(ret > 0){//add to Audio_FIFO
                av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + ret);
                av_audio_fifo_write(fifo, (void**)swrCtxManager->GetConvertedBuffer(), ret);
            }

        }else{//加入Audio_FIFO
            av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame->nb_samples);
            av_audio_fifo_write(fifo, (void**)frame->extended_data, frame->nb_samples);
        }

        int readsize = outputCodecCtx->frame_size;
        while(av_audio_fifo_size(fifo) >= readsize||
             (read_eof && av_audio_fifo_size(fifo)> 0)){//当文件已经读结束，需要把没有凑成readsize的也要编码
            AVFrame *frame;
            const int frame_size = FFMIN(av_audio_fifo_size(fifo), readsize);
            init_output_frame(&frame, outputCodecCtx, frame_size);
            if (av_audio_fifo_read(fifo, (void **)frame->data, frame_size) < frame_size) {
                fprintf(stderr, "Could not read data from FIFO\n");
                av_frame_free(&frame);
                return AVERROR_EXIT;
            }
            static int64_t pts = 0;
            frame->pts = pts;
            pts += frame->nb_samples;
            encode(outputCodecCtx, frame, encodeCallback);
        }
    };

    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    while(1){
        int ret = av_read_frame(avformatctx, packet);
        if(ret!=0){
            printf("read error or file end\n");
            read_eof = true;
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

    av_write_trailer(oAVformatCtx);
    

    return 0;
}


#endif