//此例的目标是学会ffmpeg底层内存分配方案，之前一直没有研究个AVBufferRef和AVBufferPool
//无论是AVFrame、AVPacket这一系列关键结构体都是使用这两个结构体实现数据存储
//对于AVBufferRef主要有创建av_buffer_alloc/av_buffer_relloc/ av_buffer_create， 增加引用计数函数av_buffer_ref和减少引用计数函数 av_buffer_unref
//对于AVBufferPool主是一个AVBufferRef的缓冲池，包括av_buffer_pool_init / av_buffer_pool_uninit / av_buffer_pool_get获取AVBufferRet

extern "C" {
    #include <libavutil/buffer.h>
    #include <libavutil/mem.h>
}

#include <iostream>
#include <string>

static void free_avbuffer(void* opaque, uint8_t* data){
    AVBufferRef* avbuffer = (AVBufferRef*)opaque;
    std::cout << (char*)avbuffer->data << "," << avbuffer->size << std::endl;
    av_buffer_unref(&avbuffer);
    av_freep(&data);
    av_freep(&avbuffer);
}

void AVBuffer_Example(){
    AVBufferRef* avbuffer = av_buffer_allocz(100);
    if(av_buffer_is_writable(avbuffer)){
        const char* s = "test avbuffer";
        strcpy((char*)avbuffer->data, s);
        avbuffer->size = strlen(s);
    }
    
    uint8_t* data = (uint8_t*)av_mallocz(100);
    AVBufferRef* avbuffer2 =  av_buffer_create(data, 100, free_avbuffer, avbuffer, 0);
    av_buffer_unref(&avbuffer2);

    AVBufferPool* avbufferPool = av_buffer_pool_init(100, NULL);

    AVBufferRef* avbuffer3 = av_buffer_pool_get(avbufferPool);
    std::cout << "avbuffer3->size = "<< avbuffer3->size << std::endl;

    av_buffer_pool_uninit(&avbufferPool);
}