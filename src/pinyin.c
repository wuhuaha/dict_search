#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pinyin.h"
#include "py_code.h"
#include "friso_API.h"


int py_entry_to_code(char *py, char *code, char *class_code)
{
    pinyin_convert_arry* py_convert_entry_t = NULL;
    register int i = 0;
    //*code = *single = 0;
    //printf("py:%s\n",py);
    if( py == NULL || code == NULL || class_code == NULL ){
        return 0;
    }

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
        strcat(class_code, "#");
        return 2;
    }
    if(strcmp(py_convert_entry_t->shengmu, "none") != 0)
    {
        for(i = 0; i < 24; i++)
        {
            if(strcmp(py_convert_entry_t->shengmu, shengmu_code[i].py) == 0)
            {
            strcat(code, shengmu_code[i].code);
            strcat(class_code, shengmu_code[i].class_code);
            break;
            }
        }
    }
    if(strcmp(py_convert_entry_t->yunmu, "none") != 0)
    {
        for(i = 0; i < 37; i++)
        {
            if(strcmp(py_convert_entry_t->yunmu, yunmu_code[i].py) == 0)
            {
            strcat(code, yunmu_code[i].code);
            strcat(class_code, yunmu_code[i].class_code);
            break;
            }
        }
    }
    i = strlen(code);
    *(code + i) = py_convert_entry_t->shengdiao;
    *(code + i + 1) = 0;
    i = strlen(class_code);
    *(class_code + i) = py_convert_entry_t->shengdiao;
    *(class_code + i + 1) = 0;
    //sprintf(single, "%s", py_convert_entry_t->py_single);
    //printf("py:%s;\tcode:%s;\tsingle:%s\n", py, code, py_convert_entry_t->py_single);
    return 0;
}

int py_to_code(char *py, char *code, char *class_code)
{
    //printf("%s",py);
    string_split_entry sse;
    char pinyin_entry[256] = "";
    string_split_reset( &sse, ",", py);
    *code = *class_code = 0;
    if( py == NULL || code == NULL || class_code == NULL ){
        return 0;
    }
    while( string_split_next( &sse, pinyin_entry ) != NULL ) 
    {
        //printf("%s\t",pinyin_entry);
        py_entry_to_code(pinyin_entry, code, class_code);
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
    if( word == NULL || key == NULL ){
        return 0;
    }
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
    if(mini_distance == 1){
        similarity = 90;
    }else{
        similarity = (len2 - mini_distance) * 100 / len2;
    }    
    //printf("similarity of %s(word) and %s(key) is :%d\n", word, key, similarity);
    return similarity;    
}
/*
*计算返两个拼音的编码相似度并返回
*/
int  compute_code_edit_distance(char*  word, char*  key)
{
    //printf("word:%s\tkey:%s\n", word, key);
    if( word == NULL || key == NULL ){
        return 0;
    }
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
/*
*将src中除了声调“1”以外的其他部分复制给result
*返回 成功：result的字符数   失败：0
*/
int remove_spec_tone(fstring const src, fstring result)
{
    if((src == NULL) || (result == NULL) ||(*src == 0)){
        return 0;
    }
    register int i = 0, j = 0;
    while((src[i] != 0) && (src[i+1] != 0)){
        if(src[i] != '1'){
            result[j++] = src[i++];
        }else{
            i++;
        }
    }
    result[j] = 0;
    return j;
}
/*
*在key_list中搜索py中含有的与其元素匹配度最高的成员，搜索成功则返回匹配度
*并将结果所在的元素位数存到result中
* 成功：返回相似度；失败：返回0
*/
int search_pinyin(fstring py, friso_array_t key_list, int* result)
{
    register int i = 0;
    register int similarity = -1, sim_tmp, class_sim_tmp, sim_idex = 0;
    register key_entry* entry;
    char code_word[256] = "", code_entry[64] = "", class_word[256] = "", class_entry[64] = "", tone_word[256] = "", tone_entry[64] = "";
    if((strlen(py) >= 255) || (py == NULL) || (key_list == NULL) || (result == NULL) || (*py == 0)){
        printf("search pinyin input error\n");
        return 0;
    }
    py_to_code(py, code_word, class_word);
    printf("py:%s,py_code:%s,class_code:%s\n",py, code_word, class_word);
    remove_spec_tone(code_word, tone_word);
    for(i = 0; i < key_list->length; i++)
    {
        //计算code编辑距离
        entry = *(key_list->items + i);
        if((strstr(py, entry->pinyin)) != NULL) //直接包含
        {
            similarity = 100;
            printf("包含匹配，结果:%s,标签：%s\n", entry->word, entry->lable);
            *result = i;
            return similarity;
        }
        py_to_code(entry->pinyin, code_entry, class_entry);
        if((sim_tmp =  compute_edit_distance(code_word, code_entry)) == 100)
        {
            similarity = 100;
            printf("完全匹配，结果:%s,标签：%s\n", entry->word, entry->lable);
            *result = i;
            return similarity;
        }else{
            printf("similarity of  %s is %d\n", entry->word, sim_tmp);
        }
        if((class_sim_tmp =  compute_edit_distance(class_word, class_entry)) == 100)
        {
            if(sim_tmp < 83){
                sim_tmp = 83;
                printf("读音类型匹配，结果:%s,标签：%s\n", entry->word, entry->lable);
                *result = i;
            }
        }
        remove_spec_tone(code_entry, tone_entry);
         if((class_sim_tmp =  compute_edit_distance(tone_word, tone_entry)) == 100)
        {
            if(sim_tmp < 81){
                sim_tmp = 81;
                printf("去一声匹配，结果:%s,标签：%s\n", entry->word, entry->lable);
                *result = i;
            }            
        }
        if(sim_tmp > similarity){
            similarity = sim_tmp;
            sim_idex = i;
            //printf("目前匹配度最高为：%d\n",similarity);
        }
    }
    if(similarity > 80){
        entry = *(key_list->items + sim_idex);
        printf("非完全匹配，结果%s, 标签：%s, 匹配度：%d\n", entry->word, entry->lable, similarity);
        *result = sim_idex;
        return similarity;
    }
    printf("length:%d\n",key_list->length);
    return 0;
}