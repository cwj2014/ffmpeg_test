//AVFrame是存储编码前/编码后的原始音视频数据的关键数据结构
//创建av_frame_alloc, 释放函数av_frame_free
//浅拷贝函数 av_frame_ref
//快速拷贝函数 av_frame_clone = av_frame_alloc + av_frame_ref
//重置数据存储区av_frame_unref
//移动数据区av_frame_move_ref
//根据avframe中的宽高，samples等信息分配具体数据存储区av_frame_get_buffer
//判断数据区是否可写av_frame_is_writable,和修改可写状态函数av_frame_make_writable
//创建framesidedata函数av_frame_new_side_data/av_frame_new_side_data_from_buf
//获取framesidedata函数av_frame_get_side_data
//移除framesidedata函数av_frame_remove_side_data



extern "C"{
    #include <libavutil/frame.h>
    #include <libavutil/channel_layout.h>
}
#include <iostream>

void AVFrame_Example(){
    AVFrame* avframe = av_frame_alloc();
    //视频编码
    {
        avframe->width = 352;
        avframe->height = 288;
        avframe->format = AV_PIX_FMT_YUV420P;
        av_frame_get_buffer(avframe, 0);
        if(av_frame_is_writable(avframe)){
            //复制数据
            std::cout << "复制视频数据" << std::endl;
        }else{
            av_frame_make_writable(avframe);
            std::cout << "after av_frame_make_writable: 复制视频数据\n";
        }
        av_frame_unref(avframe);
    }
    //音频编码
    {
        avframe->nb_samples = 1024;
        avframe->channel_layout = AV_CH_LAYOUT_MONO;
        avframe->format = AV_SAMPLE_FMT_S32;
        av_frame_get_buffer(avframe, 0);
        if(av_frame_is_writable(avframe)){
            std::cout << "复制音频数据\n";
        }else{
            av_frame_make_writable(avframe);
            std::cout << "after av_frame_make_writable: 复制音频数据\n";
        }
        av_frame_unref(avframe);
    }
    av_frame_free(&avframe);
    
}