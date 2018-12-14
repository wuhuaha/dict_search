//#include "edit_distance_sim.h"
#include <vector>
#include <string>
#include<iostream>
#include <algorithm>

namespace search_pinyin{
class EditDistanceSimilarity{
    public:
    EditDistanceSimilarity();
    ~EditDistanceSimilarity();
    int destroy();
    int init();
    int compute_edit_distance(std::string word,
                                        std::string key);
};



EditDistanceSimilarity::EditDistanceSimilarity(){
}

EditDistanceSimilarity::~EditDistanceSimilarity(){
    destroy();
}

int EditDistanceSimilarity::init() {
    return 0;
}

int EditDistanceSimilarity::destroy() {
    return 0;
}
//word:待匹配语句拼音  key:关键词拼音
int EditDistanceSimilarity::compute_edit_distance(std::string word, std::string key)
{
    int mini_distance, py_add, py_delete, py_change;
    size_t len1 = word.size();
    size_t len2 = key.size();
    mini_distance = len2;
    if (len1 == 0) {
        std::cout << "<error>len1="  << len1  << std::endl;
        return -1;
    }
    if (len2 == 0) {
        std::cout << "<error>len2="  << len2  << std::endl;
        return -1;
    }
    std::vector<std::vector<int> > matrix(len1 + 1, std::vector<int>(len2 + 1));
    for (size_t i = 0; i <= len1; ++i) {
        matrix[i][0] = 0;
    }
    for (size_t j = 0; j <= len2; ++j) {
        matrix[0][j] = j;
    }
    // 动态规划
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            int cost = 0;
            if (word[i - 1] != key[j - 1]) {
                cost = 1;
            }
            //假设所有操作均是对key而言
            py_add = matrix[i][j - 1] + 1;
            py_delete = matrix[i -1][j] + 1;
            py_change = matrix[i -1][j - 1] + cost;

            matrix[i][j] = std::min(std::min(py_add, py_delete), py_change);
        }
    }
    for(size_t i = 0; i <= len1; ++i){
        std::cout << matrix[i][len2] << "|";
        if(matrix[i][len2] < mini_distance)
            mini_distance = matrix[i][len2];
    }
    std::cout << std::endl;
    return mini_distance;    
}

}//namespace search_pinyin
int main()
{
    search_pinyin::EditDistanceSimilarity similarity;
    std::string test, key;
    while(1){
        std::cin  >> test;
        std::cin  >> key;
        int distance = similarity.compute_edit_distance(test, key);
        std::cout << distance << std::endl; 
    }
}
