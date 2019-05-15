#ifndef AV_UTIL_DICTIONARY_TEST_H_H_
#define AV_UTIL_DICTIONARY_TEST_H_H_

#include "global.h"


void av_dictionary_test(){
    AVDictionary *dict = nullptr;
    //添加两个键值对
    av_dict_set(&dict, "name", "cccccc", 0);
    av_dict_set_int(&dict, "age", 28, 0);
    //获取字典中的键值对数量
    int count = av_dict_count(dict);
    printf("dict count:%d\n", count);
    //根据主键获取值
    AVDictionaryEntry* entry = av_dict_get(dict, "name", NULL, 0);
    printf("key:%s, value:%s\n", "name", entry->value);
    
    //for 遍历字典
    entry = nullptr;
    for(int i=0; i<count; i++){
        entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX);
        printf("key:%s, value:%s\n", entry->key, entry->value);
    }

    //while遍历字典
    printf("while遍历：\n");
    entry = nullptr;
    //最重要的是第三个参数和第四个参数， 当entry==nullptr时从字典开始位置获取，当entry!=nullptr则从entry指向的下个位置开始获取
    //AV_DICT_IGNORE_SUFFIX， 获取开始位置的第一个键值对
    while(entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)){
        printf("key:%s, value:%s\n", entry->key, entry->value);
    }
    //释放AVDictionary
    av_dict_free(&dict);
}


#endif