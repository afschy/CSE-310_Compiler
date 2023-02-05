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
void optimize_push_pop(string* inputStrings, vector<string>* tokenList, bool* flags, const int n) {
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
            if(!flags[k]) continue;
            if(tokenList[k].size()>=2 && tokenList[k][1]==pushed) {
                useFlag = true;
                break;
            }
            if(tokenList[k].size()>=2 && (pushed.find("[" + tokenList[k][1] + "]") != std::string::npos)) {
                useFlag = true;
                break;
            }

            if(tokenList[k].size()>=4 && tokenList[k][1]=="WORD" && tokenList[k][3]==pushed) {
                useFlag = true;
                break;
            }
            if(tokenList[k].size()>=4 && tokenList[k][1]=="WORD" && (pushed.find("[" + tokenList[k][3] + "]") != std::string::npos)) {
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

// Removes addition or subtraction of 0 and multiplication by 1
void remove_redundant_arithmetic(vector<string>* tokenList, bool* flags, const int n) {
    for(int i=0; i<n; i++) {
        if(!flags[i]) continue;
        if(tokenList[i].size()>=4 && (tokenList[i][0]=="ADD" || tokenList[i][0]=="SUB") && is_const(tokenList[i][3]) && stoi(tokenList[i][3])==0)
            flags[i] = false;
    }

    //! May be problematic
    for(int i=0; i<n; i++) {
        if(!flags[i]) continue;
        if(tokenList[i].size()<1 || tokenList[i][0]!="MOV") continue;
        if(tokenList[i].size()<2 || !is_reg(tokenList[i][1])) continue;
        if(tokenList[i].size()<4 || !is_const(tokenList[i][3]) || std::stoi(tokenList[i][3])!=0) continue;
        // MOV REG , 0
        string reg = tokenList[i][1];

        for(int j=i+1; j<n; j++) {
            if(!flags[j]) continue;
            if(tokenList[j].size() && tokenList[j][0][0]==';') continue; // Ignore comments
            if(tokenList[j].size() && (tokenList[j][0]=="CALL" || tokenList[j][0][0]=='L' || tokenList[j][0][0]=='J')) break; // Can't cross labels, function calls or jumps
            if(tokenList[j].size() && (reg=="AX" || reg=="DX") && (tokenList[j][0]=="MUL" || tokenList[j][0]=="IMUL" || tokenList[j][0]=="DIV" || tokenList[j][0]=="IDIV" || tokenList[j][0]=="CWD"))
                break;

            if(tokenList[j].size()<2) continue; // REG can't be changed
            if(tokenList[j][0]!="PUSH" && tokenList[j][1]==reg) break; // REG overwritten
            if(tokenList[j].size()>=4 && tokenList[j][0]=="ADD" && tokenList[j][3]==reg) flags[j] = false;
            if(tokenList[j].size()>=4 && tokenList[j][0]=="SUB" && tokenList[j][3]==reg) flags[j] = false;
        }
    }

    //! May be problematic
    for(int i=0; i<n; i++) {
        if(!flags[i]) continue;
        if(tokenList[i].size()<1 || tokenList[i][0]!="MOV") continue;
        if(tokenList[i].size()<2 || !is_reg(tokenList[i][1])) continue;
        if(tokenList[i].size()<4 || !is_const(tokenList[i][3]) || std::stoi(tokenList[i][3])!=1) continue;
        // MOV REG , 1
        string reg = tokenList[i][1];

        for(int j=i+1; j<n; j++) {
            if(!flags[j]) continue;
            if(tokenList[j].size() && tokenList[j][0][0]==';') continue; // Ignore comments
            if(tokenList[j].size() && (tokenList[j][0]=="CALL" || tokenList[j][0][0]=='L' || tokenList[j][0][0]=='J')) break; // Can't cross labels, function calls or jumps
            if(tokenList[j].size() && (reg=="AX" || reg=="DX") && (tokenList[j][0]=="MUL" || tokenList[j][0]=="IMUL" || tokenList[j][0]=="DIV" || tokenList[j][0]=="IDIV" || tokenList[j][0]=="CWD"))
                break;

            if(tokenList[j].size()<2) continue; // REG can't be changed
            if(tokenList[j][0]=="MUL" && tokenList[j][1]==reg) flags[j] = false;
            if(tokenList[j][0]=="IMUL" && tokenList[j][1]==reg) flags[j] = false;
            if(tokenList[j][0]!="PUSH" && tokenList[j][1]==reg) break; // REG overwritten
        }
    }
}

// Replaces double moves with single moves and removes redundant moves
void optimize_move(string* inputStrings, vector<string>* tokenList, bool* flags, const int n) {
    string op11, op12, op21, op22;
    for(int i=0; i<n-1; i++) {
        if(!flags[i]) continue;
        if(tokenList[i].size() != 4 || tokenList[i][0] != "MOV") continue;

        // Find next instruction that was not removed in a previous optimization stage
        int j;
        for(j=i+1; j<n; j++) {
            if(!flags[j]) continue;
            if(!tokenList[j].size()) continue; // Empty instruction
            if(tokenList[j].size()>0 && tokenList[j][0][0]==';') continue; // Ignore comments
            break;
        }
        if(j==n) continue; // No next instruction exists
        if(tokenList[j].size() != 4 || tokenList[j][0] != "MOV") continue;

        op11 = tokenList[i][1];
        op12 = tokenList[i][3];
        op21 = tokenList[j][1];
        op22 = tokenList[j][3];

        // MOV a , b
        // MOV b , a
        if(op11 == op22 && op21 == op12) {
            flags[j] = false;
            continue;
        }

        if(op11 != op22) continue;
        if(!is_reg(op11)) continue;
        if(is_mem(op21) && is_mem(op12)) continue;

        // MOV reg_i , const/mem
        // MOV dest , reg_i
        flags[i] = false;
        inputStrings[j] = "\tMOV ";
        if(is_mem(op21) && is_const(op12))
            inputStrings[j] += "WORD PTR ";
        inputStrings[j] += op21 + " , " + op12;
        tokenize(inputStrings[j], tokenList[j]);
    }

    //! May be problematic
    for(int i=0; i<n; i++) {
        if(!flags[i]) continue;
        if(tokenList[i].size()<4 || tokenList[i][0]!="MOV") continue;
        // MOV REG/MEM , something
        string subject = tokenList[i][1];
        if(tokenList[i].size()==6 && tokenList[i][2]=="PTR" && subject=="WORD") subject = tokenList[i][3];

        for(int j=i+1; j<n; j++) {
            if(!flags[j]) continue;
            if(tokenList[j].size()>0 && tokenList[j][0][0]==';') continue; // Ignore comments
            if(tokenList[j].size() && (tokenList[j][0]=="CALL" || tokenList[j][0][0]=='L' || tokenList[j][0][0]=='J')) break; // Can't cross labels, function calls or jumps

            if(tokenList[j].size()==4 && tokenList[j][0]=="MOV" && tokenList[j][1]==subject) {
                flags[i] = false;
                break;
            }

            //! May be problematic
            if(tokenList[j].size()==6 && tokenList[j][0]=="MOV" && tokenList[j][1]=="WORD" && tokenList[j][3]==subject) {
                flags[i] = false;
                break;
            }

            if(tokenList[j].size()>=2 && tokenList[j][0]=="POP" && tokenList[j][1]==subject) {
                flags[i] = false;
                break;
            }
            
            bool useFlag = false;
            for(int k=0; k<tokenList[j].size(); k++) 
                if(tokenList[j][k] == subject) useFlag = true;
            if(useFlag) break;
            if(tokenList[j].size() && (subject=="AX" || subject=="DX") && (tokenList[j][0]=="MUL" || tokenList[j][0]=="IMUL" || tokenList[j][0]=="DIV" || tokenList[j][0]=="IDIV" || tokenList[j][0]=="CWD"))
                break;
        }
    }
}

void optimize(const string& oldFileName, const string& newFileName, const int chunkSize) {
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
        optimize_move(inputStrings, tokenList, flags, n);

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

void run_optimizer(const int iter, const int chunkSize) {
    copy_whole_file("code.asm", "temp.asm");
    optimize("temp.asm", "optimized_code.asm", chunkSize);
    for(int count=chunkSize; count>1; count-=2) {
        for(int i=0; i<iter; i++) {
            copy_whole_file("optimized_code.asm", "temp.asm");
            optimize("temp.asm", "optimized_code.asm", count);
        }
    }
    // for(int i=0; i<iter; i++) {
    //     copy_whole_file("optimized_code.asm", "temp.asm");
    //     optimize("temp.asm", "optimized_code.asm", chunkSize);
    // }
}

#endif