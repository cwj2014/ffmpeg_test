//此例为了学会两个类型的api， a × b / c 对结果不同的函数av_rescale_rnd
//对两个不周pts做比较的函数 pts1 × time_base1 < pts2 x time_base2 ? -1 :(pts1 x time_base1 == pts2 x time_base2 ? 0 : 1) av_compare_ts
//这两个函数在容器转换， 音频文件和视频合成时非常重要

extern "C"{
    #include <libavutil/mathematics.h>
}

#include <iostream>

void AVMath_Example(){
    //av_rescale_rnd是所有av_rescale_xxx函数的具体实体函数
    //av_rescale_q ->av_rescale_q_rnd - > av_rescale_rnd
    //av_rescale ->av_rescale_rnd

    std::cout << "av_rescale_q_rnd(1, AVRational{1, 25}, AVRational{1, 90000}, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX):"
              << av_rescale_q_rnd(1, AVRational{1, 25}, AVRational{1, 90000}, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX)) << std::endl;
    
    std::cout <<"av_compare_ts(1, AVRational{1, 25}, 50, AVRational{1, 48000}):"
              << av_compare_ts(1, AVRational{1, 25}, 50, AVRational{1, 48000}) << std::endl;
}