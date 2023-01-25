#ifndef ICG_HPP
#define ICG_HPP
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "merged.hpp"
#include "node.hpp"
using std::vector, std::string, std::isnan, std::to_string;
#define ENDL "\n";

typedef void (*fptr_expr_void)(Node* node);

extern std::ofstream code;

// Used in return statements to jump to the label before RET
string return_label;

// Used to restore stack
int local_var_count;

int labelCount = 0;
int currStack = 0;

string get_label() {
    return "L" + to_string(labelCount++);
}

extern vector<Node*> unitList;
extern vector<SymbolInfo*> globalVarList;
extern vector<Node*> exprList;

// Has only the global variables and functions inserted at start
extern SymbolTable table;

// Functions corresponding to (almost )each non-terminal
void start();

void compound_statement(Node* node);
void compound_statement(Node* node, vector<SymbolInfo> paramList);
void statements(Node* node);
void statement(Node* node);
void var_declaration(Node* node);
void expression_statement(Node* node);

void variable(Node* node);
void expression(Node* node);
void logic_expression(Node* node);
void rel_expression(Node* node);
void simple_expression(Node* node);
void term(Node* node);
void unary_expression(Node* node);
void factor(Node* node);
void argument_list(Node* node);

fptr_expr_void get_expr_void_funct(string label) {
    if(label == "expression_statement") return expression_statement;
    if(label == "expression") return expression;
    if(label == "logic_expression") return logic_expression;
    if(label == "rel_expression") return rel_expression;
    if(label == "simple_expression") return simple_expression;
    if(label == "term") return term;
    if(label == "unary_expression") return unary_expression;
    if(label == "factor") return factor;
}

// To know the amount of stack space to set aside for local variables
int compound_statement_varcount(Node* node);
int statements_varcount(Node* node);
int statement_varcount(Node* node);
int var_declaration_varcount(Node* node);

int compound_statement_varcount(Node* node) {
    if(node->children.size() == 2) return 0; // LCURL RCURL
    return statements_varcount(node->children[1]);
}

int statements_varcount(Node* node) {
    // statements : statement
    if(node->children.size() == 1) {
        return statement_varcount(node->children[0]);
    }
    
    vector<Node*> statementList;
    Node* stmt = node;
    while(stmt->label == "statements") {
        if(stmt->children.size() == 1) 
            statementList.push_back(stmt->children[0]);
        else statementList.push_back(stmt->children[1]);
        stmt = stmt->children[0];
    }

    int counter = 0;
    for(int i=statementList.size()-1; i>=0; i--)
        counter += statement_varcount(statementList[i]);
    return counter;
}

int statement_varcount(Node* node) {
    if(node->children[0]->label == "var_declaration")
        return var_declaration_varcount(node->children[0]);
    if(node->children[0]->label == "compound_statement")
        return compound_statement_varcount(node->children[0]);

    // IF LPAREN expression RPAREN statement
    if(node->children[0]->label == "IF" && node->children.size() == 5) 
        return statement_varcount(node->children[4]);

    // IF LPAREN expression RPAREN statement ELSE statement
    if(node->children[0]->label == "IF") 
        return statement_varcount(node->children[4]) + statement_varcount(node->children[6]);

    // WHILE LPAREN expression RPAREN statement
    if(node->children[0]->label == "WHILE") 
        return statement_varcount(node->children[4]);

    // FOR LPAREN expression_statement expression_statement expression RPAREN statement
    if(node->children[0]->label == "FOR") 
        return statement_varcount(node->children[6]);
    
    return 0;
}

int var_declaration_varcount(Node* node) {
    Node* decl = node->children[1]; // declaration_list
    int counter = 0;
    while(decl->label == "declaration_list") {
        if(decl->children.size() == 1) { // ID
            counter += 2;
        }
        else if(decl->children.size() == 4) { // ID LTHIRD CONST_INT RTHIRD
            counter += 2 * stoi(decl->children[2]->lexeme);
        }
        else if(decl->children.size() == 3) { // declaration_list COMMA ID
            counter += 2;
        }
        else { // declaration_list COMMA ID LTHIRD CONST_INT RTHIRD
            counter += 2 * stoi(decl->children[4]->lexeme);
        }
        decl = decl->children[0];
    }
    return counter;   
}

void start() {
    code << ".MODEL SMALL" << ENDL;
    code << ".STACK 100H" << ENDL;
    code << ".DATA" << ENDL;
    for(int i=0; i<globalVarList.size(); i++) {
        if(globalVarList[i]->isArray)
            code << "\t" << globalVarList[i]->name << " DW " << globalVarList[i]->arrSize << " DUP (0000H)" << "\n";
        else
            code << "\t" << globalVarList[i]->name << " DW 0" << "\n";
        delete globalVarList[i];
    }
    globalVarList.clear();
    code << ".CODE" << ENDL;
    // init_main();

    Node* currFunc;
    SymbolInfo* info;
    for(int i=0; i<unitList.size(); i++) {
        if(unitList[i]->label != "func_definition") continue;
        currFunc = unitList[i];
        info = table.lookup(currFunc->children[1]->lexeme);
        code << "\n" << info->name << " PROC" << ENDL;

        if(info->name == "main") {
            code << "\tMOV AX , @DATA" << ENDL;
            code << "\tMOV DS , AX" << ENDL;
        }

        code << "\tPUSH BP" << ENDL;
        code << "\tMOV BP , SP" << ENDL;

        int varCount = compound_statement_varcount(currFunc->children[currFunc->children.size()-1]);
        code << "\tSUB SP , " << varCount << ENDL;

        return_label = get_label();
        currStack = 0;
        if(info->func != nullptr)
            compound_statement(currFunc->children[currFunc->children.size()-1], info->func->params);
        else
            compound_statement(currFunc->children[currFunc->children.size()-1]);

        code << return_label << ":" << ENDL;
        code << "\tADD SP , " << varCount << ENDL;
        code << "\tPOP BP" << ENDL;
        if(info->name == "main") {
            code << "\tMOV AH , 4CH" << ENDL;
            code << "\tINT 21H" << ENDL;
        }
        else {
            code << "\tRET";
            if(info->func != nullptr) code << " " << 2 * info->func->params.size();
            code << ENDL;
        }
        code << info->name << " ENDP" << ENDL;
    }
    code << "\nEND MAIN" << ENDL;
}

void compound_statement(Node* node) {
    if(node->children.size() == 2) return; // LCURL RCURL
    table.enter_scope();
    code << "\n\t; Compund statement starting at line " << node->startLine << ENDL;
    statements(node->children[1]);
    code << "\t; Compund statement ending at line " << node->endLine << ENDL;
    table.exit_scope();
}

void compound_statement(Node* node, vector<SymbolInfo> paramList) {
    if(node->children.size() == 2) return;
    table.enter_scope();

    int stackPos = 4;
    for(int i=0; i<paramList.size(); i++) {
        paramList[i].stackOffset = stackPos;
        stackPos += 2;
        table.insert(&paramList[i]);
    }

    code << "\n\t; Compund statement starting at line " << node->startLine << ENDL;
    statements(node->children[1]);
    code << "\t; Compund statement ending at line " << node->endLine << ENDL;
    table.exit_scope();
}

void statements(Node* node) {
    // statements : statement
    if(node->children.size() == 1) {
        return statement(node->children[0]);
    }
    
    vector<Node*> statementList;
    Node* stmt = node;
    while(stmt->label == "statements") {
        if(stmt->children.size() == 1) 
            statementList.push_back(stmt->children[0]);
        else statementList.push_back(stmt->children[1]);
        stmt = stmt->children[0];
    }

    for(int i=statementList.size()-1; i>=0; i--)
        statement(statementList[i]);
}

void statement(Node* node) {
    if(node->children[0]->label == "var_declaration")
        return var_declaration(node->children[0]);
    if(node->children[0]->label == "expression_statement")
        return expression_statement(node->children[0]);
    if(node->children[0]->label == "compound_statement")
        return compound_statement(node->children[0]);
    
    // IF LPAREN expression RPAREN statement
    if(node->children[0]->label == "IF" && node->children.size() == 5) {
        code << "\n\t; if statement starting at line " << node->startLine << ENDL;
        expression(node->children[2]);

        string next_label = get_label(), end_label = get_label();
        code << "\tPOP AX" << ENDL;
        code << "\tCMP AX , 0" << ENDL;
        code << "\tJNE " << next_label << ENDL;
        code << "\tJMP " << end_label << ENDL;
        code << next_label << ":" << ENDL;
        statement(node->children[4]);
        code << end_label << ":" << ENDL;
        code << "\t; if statement ending at line " << node->endLine << ENDL;
        return;
    }

    // IF LPAREN expression RPAREN statement ELSE statement
    if(node->children[0]->label == "IF") {
        code << "\n\t; if-else statement starting at line " << node->startLine << ENDL;
        expression(node->children[2]);

        string if_label = get_label(), else_label = get_label(), end_label = get_label();
        code << "\tPOP AX" << ENDL;
        code << "\tCMP AX , 0" << ENDL;
        code << "\tJNE " << if_label << ENDL;
        code << "\tJMP " << else_label << ENDL;
        code << if_label << ":" << ENDL;
        statement(node->children[4]);
        code << "\tJMP " << end_label << ENDL;
        code << else_label << ":" << ENDL;
        statement(node->children[6]);
        code << end_label << ":" << ENDL;
        code << "\t; if-else statement ending at line " << node->endLine << ENDL;
        return;
    }

    // WHILE LPAREN expression RPAREN statement
    if(node->children[0]->label == "WHILE") {
        string start_label = get_label(), body_label = get_label(), end_label = get_label();
        
        code << start_label << ":\t; while loop starting at line " << node->startLine << ENDL;
        expression(node->children[2]);
        code << "\tPOP AX" << ENDL;
        code << "\tCMP AX , 0" << ENDL;
        code << "\tJNE " << body_label << ENDL;
        code << "\tJMP " << end_label << ENDL;

        code << body_label << ":" << ENDL;
        statement(node->children[4]);
        code << "\tJMP " << start_label << ENDL;

        code << end_label << ":\t; while loop ending at line " << node->endLine << ENDL;
        return;
    }

    // FOR LPAREN expression_statement expression_statement expression RPAREN statement
    if(node->children[0]->label == "FOR") {
        code << "\n\t; for loop starting at line " << node->startLine;
        expression_statement(node->children[2]);
        string start_label = get_label(), body_label = get_label(), end_label = get_label();

        code << start_label << ":" << ENDL;
        expression_statement(node->children[3]);
        code << "\tCMP AX , 0" << ENDL;
        code << "\tJNE " << body_label << ENDL;
        code << "\tJMP " << end_label << ENDL;

        code << body_label << ":" << ENDL;
        statement(node->children[6]);
        expression(node->children[4]);
        code << "\tPOP AX" << ENDL;
        code << "\tJMP " << start_label << ENDL;

        code << end_label << ":\t; for loop ending at line " << node->endLine << ENDL;
        return;
    }

    // RETURN expression SEMICOLON
    if(node->children[0]->label == "RETURN" && node->children.size() == 3) {
        code << "\t; Return statement at line " << node->startLine << ENDL;
        expression(node->children[1]);
        code << "\tPOP AX" << ENDL;
        code << "\tJMP " << return_label << ENDL;
        return;
    }

    // RETURN SEMICOLON
    if(node->children[0]->label == "RETURN") {
        code << "\t; Return statement at line " << node->startLine << ENDL;
        code << "\tJMP " << return_label << ENDL;
    }
}

void var_declaration(Node* node) {
    string typeSpecifier = node->children[0]->children[0]->label;
    vector<SymbolInfo*> varList;
    Node* decl = node->children[1]; // declaration_list

    string name;
    int line;
    while(decl->label == "declaration_list") {
        if(decl->children.size() == 1) { // ID
            name = decl->children[0]->lexeme;
            line = decl->children[0]->startLine;
            varList.push_back(new SymbolInfo(name.c_str(), typeSpecifier.c_str(), line));
        }
        else if(decl->children.size() == 4) { // ID LTHIRD CONST_INT RTHIRD
            name = decl->children[0]->lexeme;
            line = decl->children[0]->startLine;
            varList.push_back(new SymbolInfo(name.c_str(), typeSpecifier.c_str(), line));
            varList[varList.size()-1]->isArray = true;
            varList[varList.size()-1]->arrSize = stoi(decl->children[2]->lexeme);
        }
        else if(decl->children.size() == 3) { // declaration_list COMMA ID
            name = decl->children[2]->lexeme;
            line = decl->children[2]->startLine;
            varList.push_back(new SymbolInfo(name.c_str(), typeSpecifier.c_str(), line));
        }
        else { // declaration_list COMMA ID LTHIRD CONST_INT RTHIRD
            name = decl->children[2]->lexeme;
            line = decl->children[2]->startLine;
            varList.push_back(new SymbolInfo(name.c_str(), typeSpecifier.c_str(), line));
            varList[varList.size()-1]->isArray = true;
            varList[varList.size()-1]->arrSize = stoi(decl->children[4]->lexeme);
        }
        decl = decl->children[0];
    }
    
    for(int i=varList.size()-1; i>=0; i--) {
        if(!varList[i]->isArray) {
            currStack -= 2;
            varList[i]->arrSize = 1;
        }
        else currStack -= 2 * varList[i]->arrSize;
        varList[i]->stackOffset = currStack;
        table.insert(varList[i]);
        // code << "\tSUB SP , " << 2*varList[i]->arrSize << ENDL;
    }
}

void expression_statement(Node* node) { 
    if(node->children.size() == 1) return;
    code << "\n\t; Expression statement starting at line " << node->startLine << ENDL;
    expression(node->children[0]);
    code << "\tPOP AX" << ENDL;
    code << "\t; Expression statement ending at line " << node->endLine << ENDL;
}

void variable(Node* node) {
    SymbolInfo* info = table.lookup(node->children[0]->lexeme);

    if(node->children.size() == 1 && info->id == 1) { // global non-array variable
        code << "\tPUSH " << info->name << ENDL;
        return;
    }

    if(node->children.size() == 1) { // local non-array variable
        code << "\tPUSH BP[" << info->stackOffset << "]" << ENDL;
        return;
    }

    if(info->id == 1) { // global array
        expression(node->children[2]);
        code << "\tPOP SI" << ENDL;
        code << "\tSHL SI , 1" << ENDL;
        code << "\tPUSH " << info->name << "[SI]" << ENDL;
        return;
    }

    // local array
    expression(node->children[2]);
    code << "\tPOP SI" << ENDL;
    code << "\tSHL SI , 1" << ENDL;
    code << "\tADD SI , " << info->stackOffset << ENDL;
    code << "\tPUSH BP[SI]" << ENDL;
}

void expression(Node* node) {
    if(!isnan(node->eval)) {
        code << "\tMOV AX , " << int(node->eval) << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }
    
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    // Starting variable assignment
    logic_expression(node->children[2]);
    Node* variable = node->children[0];
    SymbolInfo* info = table.lookup(variable->children[0]->lexeme);
    
    if(variable->children.size() == 1 && info->id == 1) { // global non-array variable
        code << "\tPOP AX" << ENDL;
        code << "\tMOV " << info->name << " , AX" << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }

    if(variable->children.size() == 1) { // local non-array variable
        code << "\tPOP AX" << ENDL;
        code << "\tMOV BP[" << info->stackOffset << "] , AX" << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }

    if(info->id == 1) { // global array
        code << "\tPOP BX" << ENDL;
        expression(variable->children[2]);
        code << "\tPOP SI" << ENDL;
        code << "\tSHL SI , 1" << ENDL;
        code << "\tMOV " << info->name << "[SI] , BX" << ENDL;
        code << "\tPUSH BX" << ENDL;
        return;
    }

    // local array
    code << "\tPOP BX" << ENDL;
    expression(variable->children[2]);
    code << "\tPOP SI" << ENDL;
    code << "\tSHL SI , 1" << ENDL;
    code << "\tADD SI , " << info->stackOffset << ENDL;
    code << "\tMOV BP[SI] , BX" << ENDL;
    code << "\tPUSH BX" << ENDL;
}

void logic_expression(Node* node) {
    if(!isnan(node->eval)) {
        code << "\tMOV AX , " << int(node->eval) << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }
    
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    rel_expression(node->children[0]);
    rel_expression(node->children[2]);
    code << "\tPOP AX" << ENDL;
    code << "\tPOP CX" << ENDL;
    string zero_label = get_label(), end_label = get_label(); 

    if(node->children[1]->lexeme == "||") {
        code << "\tOR AX , CX" << ENDL;
        code << "\tCMP AX , 0" << ENDL;
        code << "\tJE " << zero_label << ENDL;
        code << "\tPUSH 1" << ENDL;
        code << "\tJMP " << end_label << ENDL;
        code << zero_label << ":" << ENDL;
        code << "\tPUSH 0" << ENDL;
        code << end_label << ":" << ENDL;
        return;
    }

    code << "\tCMP AX , 0" << ENDL;
    code << "\tJE " << zero_label << ENDL;
    code << "\tCMP CX , 0" << ENDL;
    code << "\tJE " << zero_label << ENDL;
    code << "\tPUSH 1" << ENDL;
    code << "\tJMP " << end_label << ENDL;
    code << zero_label << ":" << ENDL;
    code << "\tPUSH 0" << ENDL;
    code << end_label << ":" << ENDL;
}

void rel_expression(Node* node) {
    if(!isnan(node->eval)) {
        code << "\tMOV AX , " << int(node->eval) << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }
    
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    simple_expression(node->children[0]);
    simple_expression(node->children[2]);
    code << "\tPOP CX" << ENDL;
    code << "\tPOP AX" << ENDL;
    string one_label = get_label(), end_label = get_label();
    string op = node->children[1]->lexeme;

    code << "\tCMP AX , CX" << ENDL;

    if(op == "<") code << "\tJL " << one_label << ENDL;
    if(op == "<=") code << "\tJLE " << one_label << ENDL;
    if(op == ">") code << "\tJG " << one_label << ENDL;
    if(op == ">=") code << "\tJGE " << one_label << ENDL;
    if(op == "==") code << "\tJE " << one_label << ENDL;
    if(op == "!=") code << "\tJNE " << one_label << ENDL;

    code << "\tPUSH 0" << ENDL;
    code << "\tJMP " << end_label << ENDL;
    code << one_label << ":" << ENDL;
    code << "\tPUSH 1" << ENDL;
    code << end_label << ":" << ENDL;
}

void simple_expression(Node* node) {
    if(!isnan(node->eval)) {
        code << "\tMOV AX , " << int(node->eval) << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }
    
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    simple_expression(node->children[0]);
    term(node->children[2]);
    code << "\tPOP CX" << ENDL;
    code << "\tPOP AX" << ENDL;
    
    if(node->children[1]->lexeme == "+") {
        code << "\tADD AX , CX" << ENDL;
    }
    else {
        code << "\tSUB AX , CX" << ENDL;
    }
    code << "\tPUSH AX" << ENDL;
}

void term(Node* node) {
    if(!isnan(node->eval)) {
        code << "\tMOV AX , " << int(node->eval) << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }
    
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    term(node->children[0]);
    unary_expression(node->children[2]);
    code << "\tPOP CX" << ENDL;
    code << "\tPOP AX" << ENDL;
    
    if(node->children[1]->lexeme == "*") {
        code << "\tIMUL CX" << ENDL;
        code << "\tPUSH AX" << ENDL;
    }
    else if(node->children[1]->lexeme == "/") {
        code << "\tCWD" << ENDL;
        code << "\tIDIV CX" << ENDL;
        code << "\tPUSH AX" << ENDL;
    }
    else {
        code << "\tCWD" << ENDL;
        code << "\tIDIV CX" << ENDL;
        code << "\tPUSH DX" << ENDL;
    }
}

void unary_expression(Node* node) {
    if(!isnan(node->eval)) {
        code << "\tMOV AX , " << int(node->eval) << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }

    if(node->children.size() == 1) 
        return factor(node->children[0]);

    unary_expression(node->children[1]);
    string op = node->children[0]->lexeme;
    if(op == "+") return;
    if(op == "-") {
        code << "\tPOP AX" << ENDL;
        code << "\tNEG AX" << ENDL;
        code << "\tPUSH AX" << ENDL;
        return;
    }
    // op == "!"
    string one_label = get_label(), end_label = get_label();
    code << "\tPOP AX" << ENDL;
    code << "\tCMP AX , 0" << ENDL;
    code << "\tJE " << one_label << ENDL;
    code << "\tPUSH 0" << ENDL;
    code << "\tJMP " << end_label << ENDL;
    code << one_label << ":" << ENDL;
    code << "\tPUSH 1" << ENDL;
    code << end_label << ":" << ENDL;
}

void factor(Node* node) {
    string label = node->children[0]->label;
    if(label == "CONST_INT" || label == "CONST_FLOAT") {
        code << "\tMOV AX , " << stoi(node->children[0]->lexeme) << ENDL;
        code << "\tPUSH AX"<< ENDL;
        return;
    }

    if(label == "LPAREN")
        return expression(node->children[1]);
    if(label == "variable" && node->children.size() == 1)
        return variable(node->children[0]);
    
    if(label == "variable") {
        variable(node->children[0]);
        Node* variable = node->children[0];
        SymbolInfo* info = table.lookup(variable->children[0]->lexeme);
        
        if(variable->children.size() == 1 && info->id == 1) { // global non-array variable
            if(node->children[1]->label == "INCOP") {
                code << "\tADD " << info->name << " , 1" << ENDL;
            }
            else code << "\tSUB " << info->name << " , 1" << ENDL;
            return;
        }

        if(variable->children.size() == 1) { // local non-array variable
            if(node->children[1]->label == "INCOP") {
                code << "\tADD BP[" << info->stackOffset << "] , 1" << ENDL;
            }
            else code << "\tSUB BP[" << info->stackOffset << "] , 1" << ENDL;
            return;
        }

        if(info->id == 1) { // global array
            if(node->children[1]->label == "INCOP") {
                code << "\tADD " << info->name << "[SI] , 1" << ENDL;
            }
            else code << "\tSUB " << info->name << "[SI] , 1" << ENDL;
            return;
        }

        // local array
        if(node->children[1]->label == "INCOP") {
            code << "\tADD " << "BP[SI] , 1" << ENDL;
        }
        else code << "\tSUB " << "BP[SI] , 1" << ENDL;
        return;
    }

    if(label == "ID") {
        argument_list(node->children[2]);
        code << "\tCALL " << node->children[0]->lexeme << ENDL;
        code << "\tPUSH AX" << ENDL; // Return value was stored in AX
    }
}

void argument_list(Node* node) {
    if(node->children.size() == 0) return;
    Node* arguments = node->children[0];
    while(arguments->label == "arguments") {
        if(arguments->children.size() == 1) logic_expression(arguments->children[0]);
        else logic_expression(arguments->children[2]);
        arguments = arguments->children[0];
    }
}

#endif