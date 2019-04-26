#ifndef AUDIO_CONVERT_TOOL_H_H_
#define AUDIO_CONVERT_TOOL_H_H_

#include "global.h"
/**
 * 这个类实现了音频PCM数据从一种格式到另一种格式的转换，包括（采样率、声道数、单个采样的存储格式AVSampleFormat）
 */ 
class SwrCtxManager{
public:
    /**
     * src_channel_layout: 输入音频数据的声道数，如单声道、双声道等
     * src_sample_rate：输入音频数据的采样率，如44100Hz,16000Hz
     * src_sample_format：输入音频的存储格式，如AV_SAMPLE_FMT_S16
     * dest_channel_layout: 输出音频数据的声道数，如单声道、双声道等
     * dest_sample_rate：输出音频数据的采样率，如44100Hz,16000Hz
     * dest_sample_format：输出音频的存储格式，如AV_SAMPLE_FMT_FLTP
     */ 
    SwrCtxManager(int64_t src_channel_layout, int32_t src_sample_rate, AVSampleFormat src_sample_format, 
                      int64_t dest_channel_layout, int32_t dest_sample_rate, AVSampleFormat dest_sample_format)
                      :swr_ctx(nullptr),
                       m_src_channel_layout(src_channel_layout),
                       m_src_sample_rate(src_sample_rate),
                       m_src_sample_format(src_sample_format),
                       m_dest_channel_layout(dest_channel_layout),
                       m_dest_sample_rate(dest_sample_rate),
                       m_dest_sample_format(dest_sample_format),
                       m_max_dst_nb_samples(0),
                       m_dst_nb_channels(0),
                       m_dest_data(nullptr){
    }
    /**
     * 初始化
     */ 
    bool Init(){
        swr_ctx = swr_alloc();
        if(swr_ctx == nullptr){
            return false;
        }
        av_opt_set_int(swr_ctx, "in_channel_layout",    m_src_channel_layout, 0);
        av_opt_set_int(swr_ctx, "in_sample_rate",       m_src_sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", m_src_sample_format, 0);

        av_opt_set_int(swr_ctx, "out_channel_layout",    m_dest_channel_layout, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate",       m_dest_sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", m_dest_sample_format, 0);

        /* initialize the resampling context */
        if (swr_init(swr_ctx) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            return false;
        }
        m_dst_nb_channels = av_get_channel_layout_nb_channels(m_dest_channel_layout);
        return true;
    }
    /**
     * 转换格式实现
     * src: 待转换的音频采样，格式与与初始化时传入的参数保持一致
     * int: 样本数
     * return: <0 error, >=0成功
     */ 
    int Convert(const uint8_t** src, int in_count){
        //计算可能输出的样本数
        int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, m_src_sample_rate) +
                                        in_count, m_dest_sample_rate, m_src_sample_rate, AV_ROUND_UP);
        if(m_max_dst_nb_samples == 0){//首次分配内存
            int dst_linesize;
            av_samples_alloc_array_and_samples(&m_dest_data, &dst_linesize, m_dst_nb_channels, dst_nb_samples, m_dest_sample_format, 0);
            m_max_dst_nb_samples = dst_nb_samples;
        }else if(m_max_dst_nb_samples < dst_nb_samples){//当内存不够重新分配内存
            av_freep(&m_dest_data[0]);
            int dst_linesize;
            if(av_samples_alloc(m_dest_data, &dst_linesize, m_dst_nb_channels, dst_nb_samples, m_dest_sample_format, 0) < 0){
                av_freep(&m_dest_data[0]);
                free(m_dest_data);
                m_dest_data = nullptr;
                return -1;
            }
            m_max_dst_nb_samples = dst_nb_samples;
        } 
        //调用转换函数实现转换
        return swr_convert(swr_ctx, m_dest_data, dst_nb_samples, src, in_count);
    }
    /**
     * 当转换成功后，通过这个接口获取转换后的数据
     */ 
    const uint8_t ** GetConvertedBuffer() const{
        return (const uint8_t **)m_dest_data;
    }
    ~SwrCtxManager(){
          if(swr_ctx){
              swr_free(&swr_ctx);
          }
          if(m_dest_data){
              av_freep(&m_dest_data[0]);
          }
          av_free(m_dest_data);
    }
private:
     SwrContext *swr_ctx;
     int64_t m_src_channel_layout;
     int64_t m_dest_channel_layout;
     int32_t m_src_sample_rate;
     int32_t m_dest_sample_rate;
     AVSampleFormat m_src_sample_format;
     AVSampleFormat m_dest_sample_format;
     int m_max_dst_nb_samples;
     int m_dst_nb_channels;
     uint8_t **m_dest_data;
};

#endif