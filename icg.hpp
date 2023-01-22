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

void expression_statement(Node* node) {
    tempcode << get_label() << ":\t\t;" << "Line " << node->startLine << " to " << node->endLine << ENDL;
    if(node->children.size() == 1) return;
    expression(node->children[0]);
    tempcode << "\tPOP AX" << ENDL;
}

void expression(Node* node) {
    if(node->children.size() == 1) {
        logic_expression(node->children[0]);
        return;
    }
}

void logic_expression(Node* node) {
    if(node->children.size() == 1) {
        rel_expression(node->children[0]);
        return;
    }
}

void rel_expression(Node* node) {
    if(node->children.size() == 1) {
        simple_expression(node->children[0]);
        return;
    }
}

void simple_expression(Node* node) {
    if(node->children.size() == 1) {
        term(node->children[0]);
        return;
    }

    simple_expression(node->children[0]);
    term(node->children[2]);
    tempcode << "\tPOP AX" << ENDL;
    tempcode << "\tPOP BX" << ENDL;
    
    string op = node->children[1]->lexeme;
    if(op == "+") {
        tempcode << "\tADD AX, BX" << ENDL;
    }
    else {
        tempcode << "\tSUB AX, BX" << ENDL;
    }
    tempcode << "\tPUSH AX" << ENDL;
}

void term(Node* node) {
    if(node->children.size() == 1) {
        unary_expression(node->children[0]);
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
        tempcode << "\tMOV AX, " << stoi(node->children[0]->lexeme) << ENDL;
        tempcode << "\tPUSH AX" << ENDL;
        return;
    }

    if(label == "LPAREN") {
        expression(node->children[1]);
        return;
    }
}

#endif