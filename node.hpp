#ifndef NODE_HPP
#define NODE_HPP

#include <vector>
#include <string>
using std::string, std::vector;

struct Node{
    bool leaf;
    string label;
    vector<Node*> children;
    int startLine;
    int endLine;
    string lexeme;
    double eval;

    Node(bool leaf, string label, string lexeme="", int startLine=-1){
        this->leaf = leaf;
        this->label = label;
        this->lexeme = lexeme;
        this->startLine = startLine;
        endLine = startLine;
    }

    ~Node(){
        for(int i=0; i<children.size(); i++) delete children[i];
    }

    void print_derivation(int offset, FILE* out){
        update_line();
        for(int i=0; i<offset; i++) fprintf(out, " ");
        fprintf(out, "%s : ", label.c_str());
        for(int i=0; i<children.size(); i++)
            fprintf(out, "%s ", children[i]->label.c_str());
        fprintf(out, "\t<Line: %d-%d>\n", startLine, endLine);
    }

    void print_leaf(int offset, FILE* out){
        for(int i=0; i<offset; i++) fprintf(out, " ");
        fprintf(out, "%s : %s\t<Line: %d>\n", label.c_str(), lexeme.c_str(), startLine);
    }

    void update_line(){
        if(children.empty()) return;
        startLine = children[0]->startLine;
        endLine = children[children.size()-1]->endLine;
    }
};

#endif