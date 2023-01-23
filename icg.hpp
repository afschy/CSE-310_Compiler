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

extern std::ofstream tempcode;
extern std::ofstream code;

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

void init();
void init_main();

// Functions corresponding to (almost )each non-terminal
void compound_statement(Node* node);
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

void init() {
    tempcode << ".MODEL SMALL" << ENDL;
    tempcode << ".STACK 100H" << ENDL;
    tempcode << ".DATA" << ENDL;
    for(int i=0; i<globalVarList.size(); i++) {
        tempcode << "\t" << globalVarList[i]->name << " DW 0" << ENDL;
        delete globalVarList[i];
    }
    globalVarList.clear();
    tempcode << ".CODE" << ENDL;
    init_main();
}

void init_main() {
    tempcode << "MAIN PROC" << ENDL;
    tempcode << "\tMOV AX , @DATA" << ENDL;
    tempcode << "\tMOV DS , AX" << ENDL;
    tempcode << "\tPUSH BP" << ENDL;
    tempcode << "\tMOV BP , SP" << ENDL;
    // table.enter_scope();
    for(int i=0; i<exprList.size(); i++)
        compound_statement(exprList[i]);
    tempcode << "MAIN ENDP" << ENDL;
    tempcode << "END MAIN" << ENDL;
}

void compound_statement(Node* node) {
    if(node->children.size() == 2) return; // LCURL RCURL
    table.enter_scope();
    tempcode << "\n\t; Compund statement starting at line " << node->startLine << ENDL;
    statements(node->children[1]);
    tempcode << "\t; Compund statement ending at line " << node->endLine << ENDL;
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
        tempcode << "\n\t; if statement starting at line " << node->startLine << ENDL;
        expression(node->children[2]);

        string next_label = get_label(), end_label = get_label();
        tempcode << "\tPOP AX" << ENDL;
        tempcode << "\tCMP AX , 0" << ENDL;
        tempcode << "\tJNE " << next_label << ENDL;
        tempcode << "\tJMP " << end_label << ENDL;
        tempcode << next_label << ":" << ENDL;
        statement(node->children[4]);
        tempcode << end_label << ":" << ENDL;
        tempcode << "\t; if statement ending at line " << node->endLine << ENDL;
        return;
    }

    // IF LPAREN expression RPAREN statement ELSE statement
    if(node->children[0]->label == "IF") {
        tempcode << "\n\t; if-else statement starting at line " << node->startLine << ENDL;
        expression(node->children[2]);

        string if_label = get_label(), else_label = get_label(), end_label = get_label();
        tempcode << "\tPOP AX" << ENDL;
        tempcode << "\tCMP AX , 0" << ENDL;
        tempcode << "\tJNE " << if_label << ENDL;
        tempcode << "\tJMP " << else_label << ENDL;
        tempcode << if_label << ":" << ENDL;
        statement(node->children[4]);
        tempcode << "\tJMP " << end_label << ENDL;
        tempcode << else_label << ":" << ENDL;
        statement(node->children[6]);
        tempcode << end_label << ":" << ENDL;
        tempcode << "\t; if-else statement ending at line " << node->endLine << ENDL;
        return;
    }

    // WHILE LPAREN expression RPAREN statement
    if(node->children[0]->label == "WHILE") {
        string start_label = get_label(), body_label = get_label(), end_label = get_label();
        
        tempcode << start_label << ":\t; while loop starting at line " << node->startLine << ENDL;
        expression(node->children[2]);
        tempcode << "\tPOP AX" << ENDL;
        tempcode << "\tCMP AX , 0" << ENDL;
        tempcode << "\tJNE " << body_label << ENDL;
        tempcode << "\tJMP " << end_label << ENDL;

        tempcode << body_label << ":" << ENDL;
        statement(node->children[4]);
        tempcode << "\tJMP " << start_label << ENDL;

        tempcode << end_label << ":\t; while loop ending at line " << node->endLine << ENDL;
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
        currStack -= 2;
        varList[i]->stackOffset = currStack;
        table.insert(varList[i]);
        tempcode << "\tSUB SP , 2" << ENDL;
    }
}

void expression_statement(Node* node) {
    if(node->children.size() == 1) return;
    tempcode << "\n\t; Expression statement starting at line " << node->startLine << ENDL;
    expression(node->children[0]);
    tempcode << "\tPOP AX" << ENDL;
    tempcode << "\t; Expression statement ending at line " << node->endLine << ENDL;
}

void variable(Node* node) {
    SymbolInfo* info = table.lookup(node->children[0]->lexeme);
    if(node->children.size() == 1 && info->id == 1) { // global non-array variable
        tempcode << "\tPUSH " << info->name << ENDL;
        return;
    }
    if(node->children.size() == 1) { // local non-array variable
        tempcode << "\tPUSH BP[" << info->stackOffset << "]" << ENDL;
        return;
    }
}

void expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    logic_expression(node->children[2]);
    Node* variable = node->children[0];
    SymbolInfo* info = table.lookup(variable->children[0]->lexeme);
    
    if(variable->children.size() == 1 && info->id == 1) { // global non-array variable
        tempcode << "\tPOP AX" << ENDL;
        tempcode << "\tMOV " << info->name << " , AX" << ENDL;
        tempcode << "\tPUSH AX" << ENDL;
        return;
    }

    if(variable->children.size() == 1) { // local non-array variable
        tempcode << "\tPOP AX" << ENDL;
        tempcode << "\tMOV BP[" << info->stackOffset << "] , AX" << ENDL;
        tempcode << "\tPUSH AX" << ENDL;
        return;
    }
}

void logic_expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    rel_expression(node->children[0]);
    rel_expression(node->children[2]);
    tempcode << "\tPOP AX" << ENDL;
    tempcode << "\tPOP BX" << ENDL;
    string zero_label = get_label(), end_label = get_label(); 

    if(node->children[1]->lexeme == "||") {
        tempcode << "\tOR AX , BX" << ENDL;
        tempcode << "\tCMP AX , 0" << ENDL;
        tempcode << "\tJE " << zero_label << ENDL;
        tempcode << "\tPUSH 1" << ENDL;
        tempcode << "\tJMP " << end_label << ENDL;
        tempcode << zero_label << ":" << ENDL;
        tempcode << "\tPUSH 0" << ENDL;
        tempcode << end_label << ":" << ENDL;
        return;
    }

    tempcode << "\tCMP AX , 0" << ENDL;
    tempcode << "\tJE " << zero_label << ENDL;
    tempcode << "\tCMP BX , 0" << ENDL;
    tempcode << "\tJE " << zero_label << ENDL;
    tempcode << "\tPUSH 1" << ENDL;
    tempcode << "\tJMP " << end_label << ENDL;
    tempcode << zero_label << ":" << ENDL;
    tempcode << "\tPUSH 0" << ENDL;
    tempcode << end_label << ":" << ENDL;
}

void rel_expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    simple_expression(node->children[0]);
    simple_expression(node->children[2]);
    tempcode << "\tPOP BX" << ENDL;
    tempcode << "\tPOP AX" << ENDL;
    string one_label = get_label(), end_label = get_label();
    string op = node->children[1]->lexeme;

    tempcode << "\tCMP AX , BX" << ENDL;

    if(op == "<") tempcode << "\tJL " << one_label << ENDL;
    if(op == "<=") tempcode << "\tJLE " << one_label << ENDL;
    if(op == ">") tempcode << "\tJG " << one_label << ENDL;
    if(op == ">=") tempcode << "\tJGE " << one_label << ENDL;
    if(op == "==") tempcode << "\tJE " << one_label << ENDL;
    if(op == "!=") tempcode << "\tJNE " << one_label << ENDL;

    tempcode << "\tPUSH 0" << ENDL;
    tempcode << "\tJMP " << end_label << ENDL;
    tempcode << one_label << ":" << ENDL;
    tempcode << "\tPUSH 1" << ENDL;
    tempcode << end_label << ":" << ENDL;
}

void simple_expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    simple_expression(node->children[0]);
    term(node->children[2]);
    tempcode << "\tPOP BX" << ENDL;
    tempcode << "\tPOP AX" << ENDL;
    
    if(node->children[1]->lexeme == "+") {
        tempcode << "\tADD AX , BX" << ENDL;
    }
    else {
        tempcode << "\tSUB AX , BX" << ENDL;
    }
    tempcode << "\tPUSH AX" << ENDL;
}

void term(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        return func(currNode);
    }

    term(node->children[0]);
    unary_expression(node->children[2]);
    tempcode << "\tPOP BX" << ENDL;
    tempcode << "\tPOP AX" << ENDL;
    
    if(node->children[1]->lexeme == "*") {
        tempcode << "\tIMUL BX" << ENDL;
        tempcode << "\tPUSH AX" << ENDL;
    }
    else if(node->children[1]->lexeme == "/") {
        tempcode << "\tCWD" << ENDL;
        tempcode << "\tIDIV BX" << ENDL;
        tempcode << "\tPUSH AX" << ENDL;
    }
    else {
        tempcode << "\tCWD" << ENDL;
        tempcode << "\tIDIV BX" << ENDL;
        tempcode << "\tPUSH DX" << ENDL;
    }
}

void unary_expression(Node* node) {
    if(node->children.size() == 1) 
        return factor(node->children[0]);

    unary_expression(node->children[1]);
    string op = node->children[0]->lexeme;
    if(op == "+") return;
    if(op == "-") {
        tempcode << "\tPOP AX" << ENDL;
        tempcode << "\tNEG AX" << ENDL;
        tempcode << "\tPUSH AX" << ENDL;
        return;
    }
    // op == "!"
    string one_label = get_label(), end_label = get_label();
    tempcode << "\tPOP AX" << ENDL;
    tempcode << "\tCMP AX , 0" << ENDL;
    tempcode << "\tJE " << one_label << ENDL;
    tempcode << "\tPUSH 0" << ENDL;
    tempcode << "\tJMP " << end_label << ENDL;
    tempcode << one_label << ":" << ENDL;
    tempcode << "\tPUSH 1" << ENDL;
    tempcode << end_label << ":" << ENDL;
}

void factor(Node* node) {
    string label = node->children[0]->label;
    if(label == "CONST_INT" || label == "CONST_FLOAT") {
        tempcode << "\tPUSH " << stoi(node->children[0]->lexeme) << ENDL;
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
                tempcode << "\tADD " << info->name << " , 1" << ENDL;
            }
            else tempcode << "\tSUB " << info->name << " , 1" << ENDL;
            return;
        }

        if(variable->children.size() == 1) { // local non-array variable
            if(node->children[1]->label == "INCOP") {
                tempcode << "\tADD BP[" << info->stackOffset << "] , 1" << ENDL;
            }
            else tempcode << "\tSUB BP[" << info->stackOffset << "] , 1" << ENDL;
            return;
        }
    }
}

#endif