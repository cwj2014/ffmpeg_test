#ifndef AVUTIL_COMMON_H_H_
#define AVUTIL_COMMON_H_H_

extern "C"{
    #include <libavutil/opt.h>
}

struct StuInfo{
    AVClass* _cls;
    char *_stuName; //姓名
    uint64_t _stuNo; //学号
    char* _classInfo; //班级
};


struct ScoreInfo{
    AVClass* _cls;
    void* opaque; //私有信息，指向StuInfo
    float _chinese; //语文
    float _math; //数学
    float _english;//英语
};

void printfScoreInfo(void* scoreInfo);

void freeScoreInfo(void** scoreInfo);

#endif