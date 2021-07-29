//此例学习打开文件(demux)，以及获取文件中相关流信息，结构体AVFormatContext中的AVInputFormat跟demux直接相关，如果要自己写解析文件的部分就是需要实现相关的读取函数
//avformat_open_input 打开文件函数
//avformat_close_input 关闭文件函数
//avformat_find_stream_info, 查找所有流类型
//av_find_best_stream, 查找对应的流类型

extern "C"{
    #include <libavformat/avformat.h>
}
#include <iostream>

void AVFormatInput_Example(){
    const char* fileName = "/home/caiyu/VID_20210703_171254.mp4";
    AVFormatContext* avFmtCtx = NULL;
    int videoIndex = -1;
    int audioIndex = -1;
    int ret = -1;
    AVPacket* avpkt = av_packet_alloc();

    if(avformat_open_input(&avFmtCtx, fileName, NULL, NULL) < 0){
        goto end;
    }
    //这个函数必须先执行才能av_find_best_stream
    avformat_find_stream_info(avFmtCtx, NULL);

    std::cout << "此媒体文件有：" << avFmtCtx->nb_streams << "条AVStream \n";

    videoIndex = av_find_best_stream(avFmtCtx, AVMEDIA_TYPE_VIDEO, -1,  -1,  NULL, 0);

    audioIndex = av_find_best_stream(avFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    std::cout << "视频streamIndex:" << videoIndex << ",音频streamIndex:" << audioIndex << std::endl;
    
    
    while(!(ret = av_read_frame(avFmtCtx, avpkt))){
        if(avpkt->stream_index == videoIndex){
            std::cout << "读取到视频包, pts = " << avpkt->pts << std::endl;
        }else if(avpkt->stream_index == audioIndex){
            std::cout << "读取到音频包， pts = " << avpkt->pts << std::endl;
        }else{
            std::cout << "读取到其他数据包, streamIndex =" << avpkt->stream_index <<", pts = " << avpkt->pts << std::endl;
        }
    }
end:
    if(avFmtCtx != NULL){
        avformat_close_input(&avFmtCtx);
    }
    av_packet_free(&avpkt);
}