//通过此例我们主要学会音频av_sample_xxx相关api
//判断当前avsample是否是planar(线性) av_sample_fmt_is_planar
//最重要的两个api: 获取单个sample的大小av_get_bytes_per_sample 和 获取多个sample需要buffer size av_samples_get_buffer_size
//填充音频buffer缓冲的函数av_samples_fill_arrays，这个函数内存只是内存指针的地址赋值，并没有分配内存
//填充静音字符av_samples_set_silence

extern "C"{
    #include <libavutil/samplefmt.h>
}
#include <iostream>

void AVSample_Example(){
    std::cout << "AV_SAMPLE_FMT_DBLP is planar:" << av_sample_fmt_is_planar(AV_SAMPLE_FMT_DBLP) << std::endl;

    int samples = 1024;
    int channel = 2;
    int perSampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16P);
    std::cout << "AV_SAMPLE_FMT_S16P single sample size is:" << perSampleSize << std::endl;
    
    int linesize;
    int bufferSize = av_samples_get_buffer_size(&linesize, channel, samples, AV_SAMPLE_FMT_S16P, 0);

    std::cout << "av_samples_get_buffer_size : linesize = " << linesize << ", buffer size = " << bufferSize << std::endl;

    uint8_t* pAudio[2];
    uint8_t* audioData = (uint8_t*)av_malloc(bufferSize);
    av_samples_fill_arrays(pAudio, &linesize, audioData, channel, samples, AV_SAMPLE_FMT_S16P, 0);

    av_samples_set_silence(pAudio, 0, samples, channel, AV_SAMPLE_FMT_S16P);

    av_freep(&audioData);
}