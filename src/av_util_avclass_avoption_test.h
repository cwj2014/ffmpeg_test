#ifndef AV_UTIL_AVCLASS_AVOPTION_TEST_H_H_
#define AV_UTIL_AVCLASS_AVOPTION_TEST_H_H_

#include "global.h"

typedef struct AVCLSTEST{
    const AVClass* avcls;
    AVDictionary *userinfo;
    AVDictionary *score;
    int kkk;
    //userinfo opts
    char* name;
    char* gender;
    char* school;
    //score opts
    float math;
    float lenglish;
}AVCLSTEST;

#define OPTION_OFFSET(x) offsetof(AVCLSTEST, x)
#define VE 0
static const AVOption av_cls_test_options[] = {
    {"student_name", "学生姓名", OPTION_OFFSET(name), AV_OPT_TYPE_STRING, {.str = "张xxx"}, 0, 0, VE, nullptr},
    {"student_gender", "学生性别", OPTION_OFFSET(gender), AV_OPT_TYPE_STRING, {.str = "人妖"}, 0, 0, VE, nullptr},
    {"student_school", "所在学校", OPTION_OFFSET(school), AV_OPT_TYPE_STRING, {.str = "家里蹲大学"}, 0, 0, VE, nullptr},
    {"math", "数学成绩", OPTION_OFFSET(math), AV_OPT_TYPE_FLOAT, {.dbl = 59.99}, 0, 100, VE, nullptr},
    {"lenglish", "英语成绩", OPTION_OFFSET(lenglish), AV_OPT_TYPE_FLOAT, {.dbl = 59.99}, 0, 100, VE, nullptr},
    { NULL }
};

const AVClass g_avcls{
    .class_name = "AVCLSTEST",
    .item_name = av_default_item_name,
    .option = av_cls_test_options,
    .version = LIBAVUTIL_VERSION_INT
};


int avclass_avoption_test(){
    AVCLSTEST* ccc = (AVCLSTEST*)av_mallocz(sizeof(AVCLSTEST));
    ccc->avcls = &g_avcls;
    ccc->kkk = 10;
    //根据option中的默认値给结构体赋值
    av_opt_set_defaults(ccc);
    printf("姓名:%s, 性别:%s, 所在学校:%s, 数学：%f,英语:%f\n", ccc->name, ccc->gender, ccc->school, ccc->math, ccc->lenglish);

    av_dict_set(&ccc->userinfo, "student_name", "菜菜", 0);
    av_dict_set(&ccc->userinfo, "student_gender", "女", 0);
    av_dict_set(&ccc->userinfo, "student_school", "上海大学", 0);
    av_dict_set(&ccc->userinfo, "student_age", "25", 0);

    av_dict_set(&ccc->score, "math", "99", 0);
    av_dict_set(&ccc->score, "lenglish", "90", 0);
    
    AVDictionaryEntry* t = nullptr;
    while(t = av_dict_get(ccc->userinfo, "", t, AV_DICT_IGNORE_SUFFIX)){
        int ret = av_opt_set(ccc, t->key, t->value, AV_OPT_SEARCH_CHILDREN);
        if(ret == AVERROR_OPTION_NOT_FOUND){
            printf("not find option :%s\n", t->key);
        }
    }

    av_opt_set_dict(ccc, &ccc->score);

    printf("赋值后：\n");
    printf("姓名:%s, 性别:%s, 所在学校:%s, 数学：%f,英语:%f\n", ccc->name, ccc->gender, ccc->school, ccc->math, ccc->lenglish);

    if(ccc->userinfo){
        av_dict_free(&ccc->userinfo);
    }
    if(ccc->score){
        av_dict_free(&ccc->score);
    }
    av_opt_free(ccc);
    av_freep(&ccc);
    return 0;
}

#endif