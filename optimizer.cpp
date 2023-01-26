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
using std::string, std::vector, std::getline;
#ifndef ENDL
#define ENDL "\n"
#endif

std::ifstream oldfile;
std::ofstream newfile;

bool is_reg(const string& str) {
    if(str=="AX"||str=="BX"||str=="CX"||str=="DX"||str=="SP"||str=="BP"||str=="SI"||str=="DI") return true;
    return false;
}

bool is_const(const string& str) {
    if(str.length()==0) return false;
    if(str[0]>='0' && str[0]<='9') return true;
    if((str[0]=='+'||str[0]=='-') && str[1]>='0' && str[1]<='9') return true;
    return false;
}

bool is_mem (const string& str) {
    return !(is_reg(str) || is_const(str));
}

void tokenize(const string& str, vector<string>& tokenList) {
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

void copy_whole_file(const string& oldFileName, const string& newFileName) {
    std::ifstream file1;
    std::ofstream file2;
    file1.open(oldFileName);
    file2.open(newFileName);
    string line;
    while(getline(file1, line)) file2 << line << ENDL;
    file1.close();
    file2.close();
}

// Replaces redundant push-pops with MOV instructions or removes them entirely
void optimize_push_pop(string* inputStrings, vector<string>* tokenList, bool* flags, int n) {
    string pushed, popped;
    for(int i=0; i<n; i++) {
        if(!flags[i]) continue;
        if(!tokenList[i].size() || tokenList[i][0] != "PUSH") continue;

        pushed = tokenList[i][1];
        popped = "";
        int j;
        for(j=i+1; j<n; j++) {
            if(tokenList[j].size() && (tokenList[j][0]=="CALL" || tokenList[j][0]=="PUSH")) 
                break;

            if(tokenList[j].size() && (tokenList[j][0][0]=='L' || tokenList[j][0][0]=='J')) 
                break;

            if(!tokenList[j].size() || tokenList[j][0] != "POP") 
                continue;
            popped = tokenList[j][1];
            break;
        }
        if(popped == "") continue; // Corresponding POP instruction not found

        /* PUSH k
           POP k */
        if(j==i+1 && pushed==popped) { 
            flags[i] = false;
            flags[j] = false;
            continue;
        }

        /* PUSH a
           POP b */
        if(j==i+1) {
            flags[i] = false;
            inputStrings[j] = "\tMOV " + popped + " , " + pushed;
            tokenize(inputStrings[j], tokenList[j]);
            continue;
        }

        // To see if the pushed variable/register was changed before the pop
        bool useFlag = false;
        for(int k=i+1; k<j; k++) {
            if(tokenList[k].size()>=2 && tokenList[k][1]==pushed) {
                useFlag = true;
                break;
            }
            if(tokenList[k].size()>=2 && (pushed.find("[" + tokenList[k][1] + "]") != std::string::npos)) {
                useFlag = true;
                break;
            }
            if(pushed=="AX" || pushed=="DX" || (pushed.find("[AX]") != std::string::npos) || (pushed.find("[DX]") != std::string::npos)) {
                if(tokenList[k].size()>=1 && (tokenList[k][0]=="MUL" || tokenList[k][0]=="IMUL")) {
                    useFlag = true;
                    break;
                }
                if(tokenList[k].size()>=1 && (tokenList[k][0]=="DIV" || tokenList[k][0]=="IDIV")) {
                    useFlag = true;
                    break;
                }
            }
        }
        if(useFlag) continue; // push-pop is not redundant

        if(pushed == popped) {
            flags[i] = false;
            flags[j] = false;
            continue;
        }

        flags[i] = false;
        inputStrings[j] = "\tMOV " + popped + " , " + pushed;
        tokenize(inputStrings[j], tokenList[j]);
    }
}

void remove_redundant_arithmetic(vector<string>* tokenList, bool* flags, int n) {
    for(int i=0; i<n; i++) {
        if(!flags[i]) continue;
        if(tokenList[i].size()>=4 && (tokenList[i][0]=="ADD" || tokenList[i][0]=="SUB") && is_const(tokenList[i][3]) && stoi(tokenList[i][3])==0)
            flags[i] = false;
        if(tokenList[i].size()>=2 && (tokenList[i][0]=="MUL" || tokenList[i][0]=="IMUL") && is_const(tokenList[i][1]) && stoi(tokenList[i][1])==1)
            flags[i] = false;
    }
}

void optimize(const string& oldFileName, const string& newFileName, int chunkSize) {
    oldfile.open(oldFileName);
    newfile.open(newFileName);
    
    string* inputStrings = new string[chunkSize];
    vector<string>* tokenList = new vector<string>[chunkSize];
    
    // For deciding which lines will go to the output file
    bool* flags = new bool[chunkSize];
    
    // How many lines in current chunk of input
    int n;
    // Finished taking input
    bool endFlag = false;

    while(true) {
        n = chunkSize;
        for(int i=0; i<chunkSize; i++) {
            flags[i] = true;
            if(!getline(oldfile, inputStrings[i])) {
                n = i;
                endFlag = true;
                break;
            }
            
            tokenize(inputStrings[i], tokenList[i]);
            if(tokenList[i].size()>=2 && tokenList[i][1]=="ENDP") { // Can't optimize through function boundaries
                n = i+1;
                break;
            }
        }

        optimize_push_pop(inputStrings, tokenList, flags, n);
        remove_redundant_arithmetic(tokenList, flags, n);

        for(int i=0; i<n; i++) {
            if(!flags[i]) continue;
            newfile << inputStrings[i] << ENDL;
        }

        if(endFlag) break;
    }

    delete[] inputStrings;
    delete[] tokenList;
    delete[] flags;
    oldfile.close();
    newfile.close();
}

void run_optimizer(int iter, int chunkSize) {
    copy_whole_file("code.asm", "temp.asm");
    optimize("temp.asm", "optimized_code.asm", chunkSize);
    for(int i=0; i<iter; i++) {
        copy_whole_file("optimized_code.asm", "temp.asm");
        optimize("temp.asm", "optimized_code.asm", chunkSize);
    }
}

// int main() {
//     run_optimizer(8, 32);
// }

#endif