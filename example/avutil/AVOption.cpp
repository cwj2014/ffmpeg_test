//本例子主要测试三类函数 av_opt_set、av_opt_get、av_opt_set_default
//带有AVClass的子成员赋值、获取值，获取 AV_OPT_SEARCH_CHILDREN 标识

#include "avutil_common.h"
#include <iostream>

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

//通过av_opt_get获取对象值
void printfScoreInfo(void* scoreInfo){
    uint8_t* out = NULL;
    if(av_opt_get(scoreInfo, "stuName", AV_OPT_SEARCH_CHILDREN, &out)>=0){
        std::cout << "姓名：" << out << std::endl;
        av_freep(&out);
    }
    if(av_opt_get(scoreInfo, "stuNumber", AV_OPT_SEARCH_CHILDREN, &out) >= 0){
        std::cout << "学号:" << strtol((char*)out, NULL, 10) << std::endl;
        av_freep(&out);
    }
    if(av_opt_get(scoreInfo, "classInfo", AV_OPT_SEARCH_CHILDREN, &out)>=0){
        std::cout << "班级：" << out << std::endl;
        av_freep(&out);
    }
    double marks;
    if(av_opt_get_double(scoreInfo, "chinese", 0, &marks) >= 0){
       std::cout << "语文成绩:" << marks << std::endl;
    }
    if(av_opt_get_double(scoreInfo, "math", 0, &marks) >= 0){
       std::cout << "数学成绩:" << marks << std::endl;
    }  

    if(av_opt_get_double(scoreInfo, "english", 0, &marks) >= 0){
       std::cout << "英语成绩:" << marks << std::endl;
    }
} 

//通过av_opt_set_defaults设置默认参数
static void* set_default_info(){
    struct ScoreInfo* scoreInfo = (struct ScoreInfo*)av_mallocz(sizeof(struct ScoreInfo));
    scoreInfo->_cls = &scoreInfo_avcls;
    struct StuInfo* stuInfo = (struct StuInfo*)av_mallocz(sizeof(struct StuInfo));
    scoreInfo->opaque = stuInfo;
    stuInfo->_cls = &stuInfo_avcls;
    av_opt_set_defaults(scoreInfo->opaque);
    av_opt_set_defaults(scoreInfo);
    return scoreInfo;
}

//通过av_opt_frees释放对象
void freeScoreInfo(void** scoreInfo){
    av_opt_free(*scoreInfo); //释放ScoreInfo内部分配的内存，如此对象的_strName

    void* opaque = ((ScoreInfo*)(*scoreInfo))->opaque;
    if( opaque != NULL){
        av_opt_free(opaque);
    }
    av_freep(scoreInfo);
}
//通过av_opt_set族函数设值
static void set_name(void* ScoreInfo, const char* name){
    av_opt_set(ScoreInfo, "stuName", name, AV_OPT_SEARCH_CHILDREN);
}
//设置学号
static void set_stuNo(void* ScoreInfo, uint64_t no){
    av_opt_set_int(ScoreInfo, "stuNumber", no, AV_OPT_SEARCH_CHILDREN);
}

//设置班级
static void set_stuClass(void* ScoreInfo, const char* classInfo){
    av_opt_set(ScoreInfo, "classInfo", classInfo, AV_OPT_SEARCH_CHILDREN);
}

//通过av_opt_set族函数设值
static void set_chinese_marks(void* ScoreInfo, float marks){
    av_opt_set_double(ScoreInfo, "chinese", marks, 0);
}
//通过av_opt_set族函数设值
static void set_math_marks(void* ScoreInfo, float marks){
    av_opt_set_double(ScoreInfo, "math", marks, 0);
}
//通过av_opt_set族函数设值
static void set_english_marks(void* ScoreInfo, float marks){
    av_opt_set_double(ScoreInfo, "english", marks, 0);
}


void AVOption_Example(){
    void* obj = set_default_info();
    printfScoreInfo(obj);
    set_name(obj, "蔡菜");
    set_stuNo(obj, 13818888);
    set_stuClass(obj, "张江大学");
    set_chinese_marks(obj, 120.5);
    set_math_marks(obj, 125.5);
    set_english_marks(obj, 140.5);
    printfScoreInfo(obj);
    freeScoreInfo(&obj);
}

