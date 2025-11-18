#include <frontend/symbol/symbol_table.h>
#include <debug.h>

namespace FE::Sym
{
    using emap_t = std::map<Entry*, FE::AST::VarAttr*>;
    void SymTable::reset_impl() {
        for(auto x : TableList) {
            for(auto y : *x) {
                if(y.second) delete y.second;
            }
            x->clear();
            delete x;
        }
        latTable = nullptr;
        if(curTable) curTable->clear();
        else curTable = new emap_t;
        TableList.clear();
    }

    void SymTable::enterScope_impl() {
        TableList.emplace_back(new emap_t());
        latTable = TableList.back();
    }

    void SymTable::exitScope_impl() {
        for(auto x : *latTable) {
            delete (*curTable)[x.first];
            if(!x.second) {
                (*curTable).erase(x.first);
            } else {
                (*curTable)[x.first] = x.second;
            }
        }
        latTable->clear();
        delete latTable;
        TableList.pop_back();
        latTable = TableList.back();
    }

    void SymTable::addSymbol_impl(Entry* entry, FE::AST::VarAttr* attr){
        if(curTable->count(entry)) {
            (*latTable)[entry] = (*curTable)[entry];
        } else (*latTable)[entry] = nullptr;
        (*curTable)[entry] = attr;
    }

    FE::AST::VarAttr* SymTable::getSymbol_impl(Entry* entry)
    {
        if(curTable->count(entry))
            return (*curTable)[entry];
        else return nullptr;
    }

    bool SymTable::isGlobalScope_impl() {
        return TableList.size() <= 1;
    }

    int SymTable::getScopeDepth_impl() {
        return TableList.size();
    }
}  // namespace FE::Sym
