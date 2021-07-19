//AVAudioFifo相关的api是实现音频解码的数据结构，是基于AVFifoBuffer实现的二次封装
// struct AVAudioFifo {
//     AVFifoBuffer **buf;             /**< single buffer for interleaved, per-channel buffers for planar */
//     int nb_buffers;                 /**< number of buffers */
//     int nb_samples;                 /**< number of samples currently in the FIFO */
//     int allocated_samples;          /**< current allocated size, in samples */

//     int channels;                   /**< number of channels */
//     enum AVSampleFormat sample_fmt; /**< sample format */
//     int sample_size;                /**< size, in bytes, of one sample in a buffer */
// };

//av_audio_fifo_alloc初始化函数， av_audio_fifo_realloc再次分配函数， av_audio_fifo_free释放函数
//av_audio_fifo_read读取数据函数， av_audio_fifo_write写入数据函数
//av_audio_fifo_peek/peek_at 仅读取数据不丢弃数据
//av_audio_fifo_drain 丢弃数据函数
//av_audio_fifo_space 可存储samples大小， av_audio_fifo_size当前存储samples大小

extern "C"{
    #include <libavutil/samplefmt.h>
    #include <libavutil/audio_fifo.h>
}
#include <iostream>

void AVAudioFifo_Example(){
    AVAudioFifo* av_audio_fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, 2, 10240);
    std::cout << "av_audio_fifo_alloc: " << av_audio_fifo_space(av_audio_fifo) << std::endl;
    av_audio_fifo_realloc(av_audio_fifo, 20480);
    std::cout << "av_audio_fifo_realloc: " << av_audio_fifo_space(av_audio_fifo) << std::endl;
    
    int channels = 2;
    int samples = 1024;
    
    int size = av_samples_get_buffer_size(NULL, channels, samples, AV_SAMPLE_FMT_S16, 0);
    int planes = av_sample_fmt_is_planar(AV_SAMPLE_FMT_S16) ? channels : 1;
    uint8_t** buffer = (uint8_t**)av_mallocz_array(planes, sizeof(uint8_t*));
    for(int i=0; i<planes; i++){
        buffer[i] = (uint8_t*)av_malloc(size);
    }
    av_samples_set_silence(buffer, 0, samples, channels, AV_SAMPLE_FMT_S16);
    av_audio_fifo_write(av_audio_fifo, (void**)buffer, samples);

    std::cout << "av_audio_fifo_write: " << av_audio_fifo_size(av_audio_fifo) << std::endl;

    av_audio_fifo_read(av_audio_fifo, (void**)buffer, 512);

    std::cout << "av_audio_fifo_read: " << av_audio_fifo_size(av_audio_fifo) << std::endl;

    av_audio_fifo_peek(av_audio_fifo, (void**)buffer, 512);

    av_audio_fifo_drain(av_audio_fifo, 512);

    std::cout << "after av_audio_fifo_drain: " << av_audio_fifo_size(av_audio_fifo) << std::endl;

    for(int i=0; i<planes; i++){
        av_freep(&buffer[i]);
    }
    av_freep(&buffer);
    
    if(av_audio_fifo != NULL){
        av_audio_fifo_free(av_audio_fifo);
        av_audio_fifo = NULL;
    }
}