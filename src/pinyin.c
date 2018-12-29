#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pinyin.h"
#include "py_code.h"
#include "friso_API.h"

int py_entry_to_code(char *py, char *code, char *single)
{
    pinyin_convert_arry* py_convert_entry_t = NULL;
    register int i = 0;
    //*code = *single = 0;
    //printf("py:%s\n",py);
   
    for(i = 0; i < 1459; i++)
    {
        if(strcmp(py, pinyin_convert[i].py_num) == 0){
            py_convert_entry_t = &pinyin_convert[i];
            //printf("shengmu:%s,yunmu:%s\n",py_convert_entry_t->shengmu, py_convert_entry_t->yunmu);
            break;
        }
    }
    if(i == 1459){        
        i = strlen(code);
        *(code + i) = '#';
        *(code + i + 1) = 0;
        strcat(single, "#");
        return 2;
    }
    
    for(i = 0; i < 24; i++)
    {
        if(strcmp(py_convert_entry_t->shengmu, shengmu_code[i].py) == 0)
        {
            strcat(code, shengmu_code[i].code);
            break;
        }
    }
    for(i = 0; i < 37; i++)
    {
        if(strcmp(py_convert_entry_t->yunmu, yunmu_code[i].py) == 0)
        {
            strcat(code, yunmu_code[i].code);
            break;
        }
    }
    i = strlen(code);
    *(code + i) = py_convert_entry_t->shengdiao;
    *(code + i + 1) = 0;
    strcat(single, py_convert_entry_t->py_single);
    //sprintf(single, "%s", py_convert_entry_t->py_single);
    //printf("py:%s;\tcode:%s;\tsingle:%s\n", py, code, py_convert_entry_t->py_single);
    return 0;
}

int py_to_code(char *py, char *code, char *single)
{
    //printf("%s",py);
    string_split_entry sse;
    char pinyin_entry[256] = "";
    string_split_reset( &sse, ",", py);
    while( string_split_next( &sse, pinyin_entry ) != NULL ) 
    {
        //printf("%s\t",pinyin_entry);
        py_entry_to_code(pinyin_entry, code, single);
    }
    //printf("end\n");
    return 0;
}

//word:待匹配语句拼音  key:关键词拼音
int compute_edit_distance(char*  word, char*  key)
{
    int similarity, mini_tmp, mini_distance, py_add, py_delete, py_change;
    int len1 = strlen(word);
    int len2 = strlen(key);
    //printf("len1=%d", len1);
    //printf("len2=%d\n", len2);
    mini_distance = len2;
    if (len1 == 0) {
        printf("<error>len1=%d", len1);
        return -1;
    }
    if (len2 == 0) {
        printf("<error>len2=%d", len2);
        return -1;
    }
    int i = 0, j = 0;
    int matrix[len1 + 1][len2 + 1];
    for ( ; i <= len1; ++i) {
        matrix[i][0] = 0;
    }
    for ( ; j <= len2; ++j) {
        matrix[0][j] = j;
    }
    // 动态规划
    for ( i = 1; i <= len1; ++i) {
        for ( j = 1; j <= len2; ++j) {
            int cost = 0;
            if (word[i - 1] != key[j - 1]) {
                cost = 1;
            }
            //假设所有操作均是对key而言
            py_add = matrix[i][j - 1] + 1;
            py_delete = matrix[i -1][j] + 1;
            py_change = matrix[i -1][j - 1] + cost;

            //matrix[i][j] = std::min(std::min(py_add, py_delete), py_change);
            mini_tmp = (py_delete < py_change) ? py_delete : py_change;
            matrix[i][j] = (py_add < mini_tmp) ? py_add : mini_tmp;
            //printf("py_add:%d,py_delete:%d,py_change:%d,mini:%d\n",py_add, py_delete, py_change, matrix[i][j]);
        }
    }
    for(i = 0; i <= len1; ++i){
        //printf("%d|",matrix[i][len2]);
        if(matrix[i][len2] < mini_distance)
            mini_distance = matrix[i][len2];
    }
    similarity = (len2 - mini_distance) * 100 / len2;
    //printf("similarity of %s(word) and %s(key) is :%d\n", word, key, similarity);
    return similarity;    
}

int  compute_code_edit_distance(char*  word, char*  key)
{
    //printf("word:%s\tkey:%s\n", word, key);
    int similarity = 0;
    char code_word[256] = "", code_key[64] = "", single_word[256] = "", single_key[64] = "";
    py_to_code(word, code_word, single_word);
    py_to_code(key, code_key, single_key);
    //printf("code_word:%s\tcode_key:%s\n", code_word, code_key);
    similarity =  compute_edit_distance(code_word, code_key);
    //similarity =  compute_edit_distance(word, key);
    printf("distance of %s[%s] and %s[%s] is :%d\n", word, code_word, key, code_key, similarity);
    return similarity;
}