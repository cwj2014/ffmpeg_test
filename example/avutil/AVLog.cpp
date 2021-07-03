//此例是为了学会av_log等函数，且理解AVClass中log_level_offset_offset参数的使用
//AV_LOG_XXX中越是重要的日志，level值越小，默认level = AV_LOG_INFO
//通过av_log_set_level可以改变整个运行环境的level， 当通过av_log打印日志时，仅当level的值小于等于设置的值才能正常输出
//也能通过av_log_set_callback修改默认的输出回调

#include "avutil_common.h"
#include <iostream>
#include <mutex>

struct logTest{
    AVClass* avcls;
    int log_level;
};


const AVOption logTestOption[] = {
    {"level", "设置日志输出level", offsetof(logTest, log_level), AV_OPT_TYPE_INT, {.i64 = AV_LOG_DEBUG}, AV_LOG_PANIC, AV_LOG_TRACE, 0, NULL},
    {NULL}
};

static AVClass logClass{
    .class_name = "log test",
    .item_name = av_default_item_name,
    .option = logTestOption,
    .version = LIBAVUTIL_VERSION_INT,
    .log_level_offset_offset = offsetof(logTest, log_level) //当有这个值时且av_log设置的level >= AV_LOG_FATAL
                                                            //level += *(int *) (((uint8_t *) avcl) + avc->log_level_offset_offset);
};

static std::mutex mtx;

static void av_log_custom_callback(void *avcl, int level, const char *fmt, va_list vl){
    if(level > av_log_get_level()){
        return;
    }
    mtx.lock();
    printf("av_log_custom_callback{level:%d, default_log_level:%d}\n", level, av_log_get_level());
    mtx.unlock();
}

void AVLog_Example(){
    void* logObj = av_mallocz(sizeof(logTest));
    *(AVClass**)logObj = &logClass;
    av_opt_set_defaults(logObj);

    int64_t level;
    av_opt_get_int(logObj, "level", 0, &level);
    std::cout << "logTest设置的默认level:" << level << std::endl;

    av_log(logObj, AV_LOG_FATAL, "日志输出输出level：%ld-------1\n", level);
    
    av_log_set_level(AV_LOG_TRACE);

    av_log(logObj, AV_LOG_FATAL, "日志输出输出level：%ld--------2\n", level);

    av_log_set_callback(av_log_custom_callback);

    av_log(logObj, AV_LOG_FATAL, "日志输出输出level：%ld--------3\n", level);
}