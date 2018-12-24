#ifndef KEY_DEFINE
#define KEY_DEFINE

typedef struct key_word_pinyin_tag{
    char word[64];//词内容
    char pinyin[512];//词拼音
    char label[64]; //标签
    int  threshold;
    //int  emotion; //情感
    //int  weight;//权重
}key_word_pinyin;

typedef struct key_pinyin_tag{
    char key_type[64];
    key_word_pinyin *key_list;
    int  key_num;
} key_tag;

#endif
