#ifndef __FRONTEND_SYMBOL_SYMBOL_TABLE_H__
#define __FRONTEND_SYMBOL_SYMBOL_TABLE_H__

#include <frontend/symbol/isymbol_table.h>
#include <map>
#include <vector>

namespace FE::Sym
{
    class SymTable : public iSymTable<SymTable>
    {
        friend iSymTable<SymTable>;
        using emap_t = std::map<FE::Sym::Entry*, FE::AST::VarAttr*>;
        std::vector<emap_t*> TableList;
        emap_t *curTable, *latTable;
        void reset_impl();

        void              addSymbol_impl(Entry* entry, FE::AST::VarAttr* attr);
        FE::AST::VarAttr* getSymbol_impl(Entry* entry);
        void              enterScope_impl();
        void              exitScope_impl();

        bool isGlobalScope_impl();
        int  getScopeDepth_impl();
    };
}  // namespace FE::Sym

#endif  // __FRONTEND_SYMBOL_SYMBOL_TABLE_H__
