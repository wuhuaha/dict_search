#ifndef __PINYIN_H__
#define __PINYIN_H__

typedef struct {
    char py_num[16];    //数字音调形式整拼
    char shengmu[8];    //声母
    char yunmu[8];      //韵母
    char shengdiao;     //数字声调
    char py_mark[16];   //标志音调形式整拼
    char py_single[16]; //无音调形式整拼
    char important[8];  //重要的部分（主发音）
    char first_char;    //拼音首字符
}  pinyin_convert_arry;

typedef struct {
    char py[8];
    char code[4];
}  pinyin_convert_code;

//word:待匹配语句拼音  key:关键词拼音
int compute_edit_distance(char*  word, char*  key);

int  compute_code_edit_distance(char*  word, char*  key);

int py_entry_to_code(char *py, char *code, char *single);

int py_to_code(char *py, char *code, char *single);
#endif