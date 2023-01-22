#ifndef ICG_HPP
#define ICH_HPP
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

string get_label() {
    return "L" + to_string(labelCount++);
}

void expression_statement(Node* node);
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

void expression_statement(Node* node) {
    tempcode << get_label() << ":\t\t;" << "Line " << node->startLine << " to " << node->endLine << ENDL;
    if(node->children.size() == 1) return;
    expression(node->children[0]);
    tempcode << "\tPOP AX" << ENDL;
}

void expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        func(currNode);
        return;
    }
}

void logic_expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        func(currNode);
        return;
    }
}

void rel_expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        func(currNode);
        return;
    }
}

void simple_expression(Node* node) {
    if(node->children.size() == 1) {
        Node* currNode = node;
        while(currNode->children.size() == 1 && currNode->label != "factor") currNode = currNode->children[0];
        fptr_expr_void func = get_expr_void_funct(currNode->label);
        func(currNode);
        return;
    }

    simple_expression(node->children[0]);
    term(node->children[2]);
    tempcode << "\tPOP AX" << ENDL;
    tempcode << "\tPOP BX" << ENDL;
    
    string op = node->children[1]->lexeme;
    if(op == "+") {
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
        func(currNode);
        return;
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
    if(node->children.size() == 1) {
        factor(node->children[0]);
        return;
    }
}

void factor(Node* node) {
    string label = node->children[0]->label;
    if(label == "CONST_INT" || label == "CONST_FLOAT") {
        tempcode << "\tPUSH " << stoi(node->children[0]->lexeme) << ENDL;
        return;
    }

    if(label == "LPAREN") {
        expression(node->children[1]);
        return;
    }
}

#endif