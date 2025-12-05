#include "frontend/ast/ast_defs.h"
#include "frontend/ast/decl.h"
#include "frontend/ast/expr.h"
#include "ivisitor.h"
#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>
#include <string>

namespace FE::AST
{
    bool ASTChecker::visit(Initializer& node)
    {
        // 示例实现：单个初始化器的语义检查
        // 1) 访问初始化值表达式
        // 2) 将子表达式的属性拷贝到当前节点
        ASSERT(node.init_val && "Null initializer value");
        bool res  = apply(*this, *node.init_val);
        node.attr = node.init_val->attr;
        if(node.attr.val.value.type->getBaseType() == Type_t::VOID
            || node.attr.val.value.type->getTypeGroup() == TypeGroup::POINTER) {
                std::string error_str = "unavaliable type initializer at line " + std::to_string(node.line_num);
                errors.emplace_back(error_str);
                res = 0;
            }
        return res;
    }

    bool ASTChecker::visit(InitializerList& node)
    {
        // 示例实现：初始化器列表的语义检查
        // 遍历列表中的每个初始化器并逐个访问
        if (!node.init_list) return true;
        bool res = true;
        for (auto* init : *(node.init_list))
        {
            if (!init) continue;
            res &= apply(*this, *init);
        }
        return res;
    }

    bool ASTChecker::visit(VarDeclarator& node)
    {
        // TODO(Lab3-1): 实现变量声明器的语义检查
        // 访问左值表达式，同步属性，处理初始化器（如果有）
        // (void)node;
        // TODO("Lab3-1: Implement VarDeclarator semantic checking");
        LeftValExpr *lval = (LeftValExpr*)node.lval;
        auto attr = symTable.getSymbol(lval->entry);
        bool res = 1;
        if(lval->indices) {
            for(auto x : *lval->indices) {
                res &= apply(*this, *x);
                res &= x->attr.val.isConstexpr
                       && x->attr.val.value.type->getBaseType() == Type_t::INT
                       && x->attr.val.value.type->getTypeGroup() == TypeGroup::BASIC;
            }
        }
        if(node.init) res &= apply(*this, *node.init);
        else return res;
        if(!res) return false;
        std::vector<int> &dims = attr->arrayDims;
        std::vector<VarValue> &inits = attr->initList;
        if(lval->indices) {
            int sum = 1;
            for(auto x : *lval->indices) {
                dims.emplace_back(x->attr.val.value.getInt());
                sum *= dims.back();
            }
            if(!node.init) return true;
            inits.resize(sum);
            // auto initlist = ((InitializerList*)node.init)->init_list;
            // for(auto x : *initlist) {

            // }
        } else {
            inits = { node.init->attr.val.value };
        }
        return true;
    }

    bool ASTChecker::visit(ParamDeclarator& node)
    {
        // TODO(Lab3-1): 实现函数形参的语义检查
        // 检查形参重定义，处理数组形参的类型退化，将形参加入符号表
        bool res = 1;
        auto attr = new VarAttr(node.type, false, symTable.getScopeDepth());
        if(node.dims) {
            attr->type = TypeFactory::getPtrType(node.type);
            for(auto x : *node.dims) {
                res &= apply(*this, *x);
                res &= x->attr.val.isConstexpr;
                if(res) attr->arrayDims.emplace_back(x->attr.val.getInt());
                else {
                    std::string error_str = "illegal param at line " + std::to_string(node.line_num);
                    errors.emplace_back(error_str);
                    return false;
                }
            }
        }
        if(node.type->getBaseType() == Type_t::VOID) {
            std::string error_str = "illegal param of type void at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
        }
        symTable.addSymbol(node.entry, attr);
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement ParamDeclarator semantic checking");
    }

    bool ASTChecker::visit(VarDeclaration& node)
    {
        // TODO(Lab3-1): 实现变量声明的语义检查
        // 遍历声明列表，检查重定义，处理数组维度和初始化，将符号加入符号表
        auto level = symTable.getScopeDepth();
        bool res = 1;
        if(node.type->getBaseType() == Type_t::VOID) {
            std::string error_str = "you can't define a void var at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            res = 0;
        }
        for(auto x : *node.decls) {
            auto lval = (LeftValExpr*)x->lval;
            auto atp = symTable.getSymbol(lval->entry);
            if(atp && atp->scopeLevel == level) {
                res = 0;
                std::string error_str = "multi definition of var " + lval->entry->getName()
                                + " at line " + std::to_string(node.line_num);
                errors.emplace_back(error_str);
                continue;
            }
            symTable.addSymbol(lval->entry,
            new VarAttr(lval->indices ? TypeFactory::getPtrType(node.type) : node.type,
                    node.isConstDecl, level));
            res &= apply(*this, *x);
        }
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement VarDeclaration semantic checking");
    }
}  // namespace FE::AST
