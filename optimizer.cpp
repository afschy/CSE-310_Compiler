#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using std::string, std::vector;

bool is_reg(string& str) {
    if(str=="AX"||str=="BX"||str=="CX"||str=="DX"||str=="SP"||str=="BP"||str=="SI"||str=="DI") return true;
    return false;
}

bool is_const(string& str) {
    if(str.length()==0) return false;
    if(str[0]>='0' && str[0]<='9') return true;
    if((str[0]=='+'||str[0]=='-') && str[1]>='0' && str[1]<='9') return true;
    return false;
}

bool is_mem(string& str) {
    return !(is_reg(str) || is_const(str));
}

void tokenizer(string& str, vector<string>& tokenList) {
    tokenList.clear();
    string curr = "";
    for(int i=0; i<str.length(); i++) {
        if(str[i]==' ' || str[i]=='\t') {
            if(curr.length()) tokenList.push_back(curr);
            curr = "";
        }
        else curr += str[i];
    }
    if(curr.length()) tokenList.push_back(curr);
}

int main() {
    
}

#endif