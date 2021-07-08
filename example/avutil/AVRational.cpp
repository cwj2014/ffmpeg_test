//此例主要学会创建AVRational、转成double、倒数，double转AVRational等
//该对象在从文件-》解码-》显示/ 编码-》写入文件等场景有非常多的运用
//av_make_q 、av_q2d、 av_d2q、 av_inv_q等

extern "C"{
    #include <libavutil/rational.h>
}
#include <iostream>

void AVRational_Example(){

    AVRational o = av_make_q(1, 25);
    
    std::cout << "av_q2d(av_make_q(1, 25)) = " << av_q2d(o) << std::endl;

    AVRational o2 = av_inv_q(o);

    std::cout << "av_inv_q(av_make_q(1, 25)): num = "  << o2.num << ", denominator = " << o2.den << std::endl;
    
    double d = 1.25;

    AVRational o3 = av_d2q(d, 100);

    std::cout << "av_d2q(1.25, 100) : num = " << o3.num << ", denominator = " << o3.den << std::endl;
}