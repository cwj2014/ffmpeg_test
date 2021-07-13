//此例主要研究ffmpeg先进先出的数据结构AVFifoBuffer
// typedef struct AVFifoBuffer {
//     uint8_t *buffer;
//     uint8_t *rptr, *wptr, *end;
//     uint32_t rndx, wndx;
// } AVFifoBuffer;
//使用数组来模拟队列, 这个数据结构最重要的是end = buffer + size(buffersize)，所以可以通过end - buffer 得到数据区大小
//rndex表示读取位置， wndex表示写入位置, 这个结构体最厉害之处是对这两个变量永远都只有加法操作，一开始我也不理解，万一越界了呢？

// unsigned char aread = 120;
// unsigned char awrite = 255;
// awrite += 5; //此时awrite变成了4，加1变成0，加2变成1......
// unsigned char size = (unsigned char)(awrite - aread); //(unsigned char)(4 - 120) = 140,和目标值相同

// 4 - 120 = -116 负数的2进制是正数据的反码+1

// 116的原码： 01110100
// 116的反码： 10001011
// +1所生成的补码：10001100
// 10001100反表示的无符号正数为： 1 * 2^7 + 1 * 2^3 + 1 * 2^2 = 128 + 8 + 4 = 140

//初始化 av_fifo_alloc/relloc, 释放函数av_fifo_free/freep
//写入函数 av_fifo_generic_write, 读取函数av_fifo_generic_read
//获取缓冲区大小函数 av_fifo_space, 获取缓冲区数据大小av_fifo_size
//另外的读取函数av_fifo_generic_peek_at / av_fifo_generic_peek, 这两个函数也是读取数据，但是读取后并没有丢掉数据rptr还是在原来的位置
//丢掉数据函数av_fifo_drain,最读指针偏移size个位置

extern "C"{
    #include <libavutil/fifo.h>
}
#include <iostream>

void AVFifoBuffer_Example(){

    AVFifoBuffer* avfifo = av_fifo_alloc(1024);
    std::cout << "avfifo: buffer size :" << av_fifo_space(avfifo) << ", data size:" << av_fifo_size(avfifo) << std::endl;

    av_fifo_realloc2(avfifo, 2048);
    std::cout << "after realloc avfifo: buffer size :" << av_fifo_space(avfifo) << ", data size:" << av_fifo_size(avfifo) << std::endl;
    uint8_t data[] = "1234567890abcdefghijklmn";
    av_fifo_generic_write(avfifo, (void*)data, sizeof(data), NULL);
    std::cout << "after write avfifo: buffer size :" << av_fifo_space(avfifo) << ", data size:" << av_fifo_size(avfifo) << std::endl;
    uint8_t dest[11] = {'0'};
    av_fifo_generic_read(avfifo, (void*)dest, 10, NULL);
    std::cout << "after read avfifo: data size:" << av_fifo_size(avfifo) << ", read data:" << (char*)dest << std::endl;
    
    av_fifo_generic_peek_at(avfifo, dest, 2, 10, NULL);
    std::cout << "after peek at avfifo: data size:" << av_fifo_size(avfifo) << ", read data:" << (char*)dest << std::endl;

    av_fifo_generic_peek(avfifo, dest, 10, NULL);
    std::cout << "after peek avfifo: data size:" << av_fifo_size(avfifo) << ", read data:" << (char*)dest << std::endl;

    av_fifo_drain(avfifo, 3);
    std::cout << "after drain avfifo: data size:" << av_fifo_size(avfifo) << std::endl;

    av_fifo_freep(&avfifo);

}


