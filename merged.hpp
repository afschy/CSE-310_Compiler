#ifndef MERGED_CPP
#define MERGED_CPP

#include <iostream>
#include <string>
#include <vector>
typedef unsigned long long ull;

extern FILE* logout;
extern FILE* tokenout;
class SymbolInfo;
struct FunctionContainer{
    std::string retType;
    std::vector<SymbolInfo> params;

    FunctionContainer(std::string r){retType = r;}

    FunctionContainer(std::string r, std::vector<SymbolInfo> p){
        retType = r;
        params = p;
    }
};

class SymbolInfo{
public:
    int id;
    int x, y;

    std::string name;
    std::string type;
    SymbolInfo* next;
    int line;
    bool isArray;
    FunctionContainer* func;

    SymbolInfo(int id=-1, int x=-1, std::string name="", std::string type="", SymbolInfo* next = nullptr){
        this->id = id;
        this->x = x;
        this->name = name;
        this->type = type;
        this->next = next;
        isArray = false;
        func = nullptr;
    }

    SymbolInfo(const char* name, const char* type, int line){
        this->name = name;
        this->type = type;
        this->line = line;
        next = nullptr;
        isArray = false;
        func = nullptr;
    }

    SymbolInfo(SymbolInfo* s) {
        id = s->id;
        x = s->x;
        y = s->y;
        name = s->name;
        type = s->type;
        next = s->next;
        line = s->line;
        isArray = s->isArray;
        if(s->func == nullptr || s->func == NULL) func = s->func;
        else func = new FunctionContainer(s->func->retType, s->func->params); 
    }

    ~SymbolInfo(){
        if(func != nullptr) delete func;
    }
    
    void setId(int _id){id = _id;}
    void setX(int _x){x = _x;}
    void setY(int _y){y = _y;}
    void setName(std::string str){name = str;}
    void setType(std::string str){type = str;}
    void setNext(SymbolInfo* p){next = p;}
    
    int getId(){return id;}
    int getX(){return x;}
    int getY(){return y;}
    std::string getName(){return name;}
    std::string getType(){return type;}
    SymbolInfo* getNext(){return next;}

    void print(FILE* fp){
        if(func != nullptr){
            fprintf(fp, "<%s, %s, %s> ", name.c_str(), type.c_str(), func->retType.c_str());
            return;
        }
        fprintf(fp, "<%s", name.c_str());
        if(isArray)
            fprintf(fp, ", ARRAY");
        fprintf(fp, ", %s> ", type.c_str());
    }
};

class ScopeTable{
public:
    SymbolInfo **container;
    int bucket_count;
    int id;
    ScopeTable* parent_scope;

    static ull sdbm_hash(std::string &str){
        ull hash = 0, len = str.length();
        for(int i=0; i<len; i++)
            hash = str[i] + (hash<<6) + (hash<<16) - hash;
        return hash;
    }

    int hash(std::string &str){
        return sdbm_hash(str) % bucket_count;
    }

    ScopeTable(int size, int i, ScopeTable* parent=nullptr){
        bucket_count = size;
        id = i;
        container = new SymbolInfo*[bucket_count];
        for(int i=0; i<bucket_count; i++){
            container[i] = nullptr;
        }
        parent_scope = parent;

        // std::cout<<"\tScopeTable# "<<id<<" created\n";
    }

    ~ScopeTable(){
        for(int i=0; i<bucket_count; i++){
            SymbolInfo* ptr = container[i];
            SymbolInfo* temp;
            while(ptr != nullptr){
                temp = ptr;
                ptr = ptr->getNext();
                delete ptr;
            }
        }
        delete[] container;
        // std::cout<<"\tScopeTable# "<<id<<" removed\n";
    }

    void set_parent_scope(ScopeTable* ptr){parent_scope = ptr;}
    ScopeTable* get_parent_scope(){return parent_scope;}
    int get_bucket_count(){return bucket_count;}
    int get_id(){return id;}

    bool insert(std::string &name, std::string &type){
        int hash_index = hash(name);
        if(lookup(name) != nullptr) return false;
        SymbolInfo* new_object_ptr = new SymbolInfo(id, hash_index+1, name, type, nullptr);

        if(container[hash_index] == nullptr){
            new_object_ptr->setY(1);
            container[hash_index] = new_object_ptr;
            return true;
        }

        SymbolInfo *ptr = container[hash_index];
        int index = 1;
        while(ptr->getNext() != nullptr){
            ptr = ptr->getNext();
            index++;
        }
        new_object_ptr->setY(index+1);
        ptr->setNext(new_object_ptr);
        return true;
    }

    bool insert(SymbolInfo* s){
        int hash_index = hash(s->name);
        if(lookup(s->name) != nullptr) return false;
        s->id = this->id;
        s->x = hash_index + 1;
        s->next = nullptr;

        if(container[hash_index] == nullptr){
            s->setY(1);
            container[hash_index] = s;
            return true;
        }

        SymbolInfo *ptr = container[hash_index];
        int index = 1;
        while(ptr->getNext() != nullptr){
            ptr = ptr->getNext();
            index++;
        }
        s->setY(index+1);
        ptr->setNext(s);
        return true;
    }

    SymbolInfo* lookup(std::string &name){
        int hash_index = hash(name);
        
        SymbolInfo* ptr = container[hash_index];
        while(ptr != nullptr){
            if(ptr->getName() == name) break;
            ptr = ptr->getNext();
        }

        return ptr;
    }

    bool remove(std::string &name){
        int hash_index = hash(name);
        if(container[hash_index] == nullptr) return false;

        if(container[hash_index]->getName() == name){
            SymbolInfo* temp = container[hash_index];
            container[hash_index] = container[hash_index]->getNext();
            delete temp;
            return true;
        }

        SymbolInfo* ptr = container[hash_index];
        while(ptr->getNext() != nullptr && (ptr->getNext())->getName() != name)
            ptr = ptr->getNext();
        if(ptr->getNext() == nullptr) return false;

        SymbolInfo* ptr_to_remove = ptr->getNext();
        SymbolInfo* next_ptr = ptr_to_remove->getNext();
        ptr->setNext(next_ptr);
        delete ptr_to_remove;
        return true;
    }

    void print(){
		fprintf(logout, "\tScopeTable# %d\n", id);
        for(int i=0; i<bucket_count; i++){
            if(container[i] == nullptr) continue;
			fprintf(logout, "\t%d--> ", i+1);
            SymbolInfo *ptr = container[i];
            while(ptr != nullptr){
				ptr->print(logout);
                ptr = ptr->getNext();
            }
            fprintf(logout, "\n");
        }
    }
};

class SymbolTable{
public:
    ScopeTable *current_table;
    int table_count;
    int global_bucket_size;

    SymbolTable(int size){
        global_bucket_size = size;
        current_table = new ScopeTable(global_bucket_size, 1);
        table_count = 1;
    }

    ~SymbolTable(){
        ScopeTable *curr = current_table;
        while(curr != nullptr){
            ScopeTable *temp = curr;
            curr = curr->get_parent_scope();
            delete temp;
        }
    }

    int get_table_count(){return table_count;}
    int get_top_id(){return current_table->get_id();}

    void enter_scope(){
        table_count++;
        ScopeTable* new_table = new ScopeTable(global_bucket_size, table_count, current_table);
        current_table = new_table;
    }

    bool exit_scope(){
        if(get_top_id() == 1) return false;
        ScopeTable* temp = current_table;
        current_table = current_table->get_parent_scope();
        delete temp;
        return true;
    }
    
    bool insert(std::string &name, std::string &type){
        return current_table->insert(name, type);
    }

    bool insert(SymbolInfo* s){
        return current_table->insert(s);
    }

    bool remove(std::string &name){
        return current_table->remove(name);
    }

    SymbolInfo* lookup(std::string &name){
        ScopeTable *curr = current_table;
        SymbolInfo *result = nullptr;
        while(curr != nullptr){
            result = curr->lookup(name);
            if(result != nullptr) break;
            curr = curr->get_parent_scope();
        }
        return result;
    }

    void print_current_table(){
        current_table->print();
    }

    void print_all_tables(){
        ScopeTable *curr_ptr = current_table;
        while(curr_ptr != nullptr){
            curr_ptr->print();
            curr_ptr = curr_ptr->get_parent_scope();
        }
    }
};

#endif