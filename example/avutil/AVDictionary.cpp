//此例是为了学会AVDictionary结构体赋值，获取值，以及利用它对AVClass对象赋值
//主要学会av_dict_set_xxx、av_dict_get_xxx、av_dict_copy、av_dict_count等函数
//学会使用av_opt_set_dict2函数赋值
#include "avutil_common.h"
#include <iostream>

extern "C"{
    #include <libavutil/dict.h>
}

const AVOption stuInfoAVOption[] = {
    {.name = "stuName", .help = "姓名", .offset = offsetof(struct StuInfo, _stuName), .type = AV_OPT_TYPE_STRING, {.str = "张xx"}, .min = 0.0, .max = 0.0, .flags = 0, .unit = NULL},
    {"stuNumber", "学号", offsetof(struct StuInfo, _stuNo), AV_OPT_TYPE_UINT64, {.i64 = 100000}, 0.0, 999999999.0, 0, NULL},
    {"classInfo", "班级", offsetof(struct StuInfo, _classInfo), AV_OPT_TYPE_STRING, {.str = "张江xx学校"}, 0.0, 0.0, 0},
    {NULL}
};

static AVClass stuInfo_avcls = {
    .class_name = "AVOption stuInfo example",
    .item_name = av_default_item_name, //这个函数实质上就是获取上边的class_name
    .option =  stuInfoAVOption, //默认参数
    .version = LIBAVUTIL_VERSION_INT
};


const AVOption scoreInfoAVOption[] = {
    {"chinese", "语文成绩", offsetof(struct ScoreInfo, _chinese), AV_OPT_TYPE_FLOAT, {.dbl = 60.0}, 0.0, 150.0, 0, NULL},
    {"math", "数学成绩", offsetof(struct ScoreInfo, _math), AV_OPT_TYPE_FLOAT, {.dbl = 60.0}, 0.0, 150.0, 0},
    {"english", "英语成绩", offsetof(struct ScoreInfo, _english), AV_OPT_TYPE_FLOAT, {.dbl = 60.0}, 0.0, 150.0, 0},
    {NULL} //此处必须以{NULL}结束，否则会出现赋值错误问题
};


static void* score_info_next(void *obj, void *prev){
    ScoreInfo* scoreInfo = (ScoreInfo*)obj;
    if(!prev && scoreInfo->opaque){
        return scoreInfo->opaque;
    }
    return NULL;
}

static const AVClass* socre_info_child_class_iterate(void **iter){
    const AVClass *c = *iter ? NULL : &stuInfo_avcls;
    *iter = (void*)(uintptr_t)c;
    return c;
}

//C++初始化AVClass成员变量不能乱序
static AVClass scoreInfo_avcls = {
    .class_name = "AVOption scoreInfo example",
    .item_name = av_default_item_name, //这个函数实质上就是获取上边的class_name
    .option =  scoreInfoAVOption, //默认参数
    .version = LIBAVUTIL_VERSION_INT,
    .log_level_offset_offset = 0,
    .parent_log_context_offset = 0,
    .child_next = score_info_next,
    .category = AV_CLASS_CATEGORY_NA,
    .get_category = NULL,
    .query_ranges = NULL,
    .child_class_iterate = socre_info_child_class_iterate 
};

void printDict(AVDictionary* dict){
    AVDictionaryEntry* entry = NULL;
    //key不能赋值为NULL
    while((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)) != NULL){
        std::cout << "key:" << entry->key << " value:" << entry->value << std::endl;
    }
}

void AVDictionary_Example(){
    AVDictionary *dict = NULL;
    av_dict_set(&dict, "chinese", "120", 0);
    av_dict_set(&dict, "math", "100.5", 0);
    av_dict_set(&dict, "english", "130", 0);
    av_dict_set(&dict, "physical", "89", 0);

    std::cout << "dict count:" << av_dict_count(dict);

    AVDictionary* scoreInfoDict = NULL;
    av_dict_copy(&scoreInfoDict, dict, 0);

    av_dict_free(&dict);

    printDict(scoreInfoDict);
    
    AVDictionary* stuInfoDict = NULL;
    av_dict_set(&stuInfoDict, "stuName", "小菜菜", 0);
    av_dict_set(&stuInfoDict, "stuNumber", "1238888", 0);
    av_dict_set(&stuInfoDict, "classInfo", "三年级9班", 0);
    av_dict_set(&stuInfoDict, "fakeInfo", "xxxxx", 0);

    ScoreInfo* scoreInfo = (ScoreInfo*)av_mallocz(sizeof(ScoreInfo));
    scoreInfo->_cls = &scoreInfo_avcls;
    StuInfo* stuInfo = (StuInfo*)av_mallocz(sizeof(StuInfo));
    stuInfo->_cls = &stuInfo_avcls;
    scoreInfo->opaque = stuInfo;
    //当obj中没有找到对应的属性，对应的dict会返回未赋值的项，且原来的dict被释放
    av_opt_set_dict(scoreInfo->opaque, &stuInfoDict);

    std::cout << "输出stuInfoDict未赋值的key->value" << std::endl;
    printDict(stuInfoDict);

    av_opt_set_dict(scoreInfo, &scoreInfoDict);
    std::cout << "输出scoreInfoDict未赋值的key->value" << std::endl;
    printDict(scoreInfoDict);
    
    std::cout << "输出赋值后的信息" << std::endl;
    printfScoreInfo(scoreInfo);

    av_opt_free(scoreInfo->opaque);
    av_opt_free(scoreInfo);
    av_freep(&scoreInfo);
}