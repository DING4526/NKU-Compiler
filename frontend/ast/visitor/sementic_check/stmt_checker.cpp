#include "frontend/ast/ast_defs.h"
#include "frontend/ast/stmt.h"
#include "ivisitor.h"
#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>
#include <string>

namespace FE::AST
{
    bool ASTChecker::visit(ExprStmt& node)
    {
        // 示例实现：表达式语句的语义检查
        // 空表达式直接通过，否则访问内部表达式
        if (!node.expr) return true;
        return apply(*this, *node.expr);
    }

    bool ASTChecker::visit(FuncDeclStmt& node)
    {
        // TODO(Lab3-1): 实现函数声明的语义检查
        // 检查作用域，记录函数信息，处理形参和函数体，检查返回语句
        bool res = true;
        if(funcDecls.count(node.entry)) {
            std::string error_str = "multi definition of func " + node.entry->getName()
                + " at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            res = false;
        }
        if(node.entry->getName() == "main") mainExists = true;
        funcDecls[node.entry] = &node;
        symTable.enterScope();
        curFuncRetType = node.retType;
        funcHasReturn = false;
        for(auto x : *node.params) {
            res &= apply(*this, *x);
        }
        auto blkStmt = static_cast<BlockStmt*>(node.body);
        if(blkStmt->stmts) {
            for(auto x : *blkStmt->stmts) {
                res &= apply(*this, *x);
            }
        }
        symTable.exitScope();
        if(!funcHasReturn) {
            std::string error_str = "Func may have no return for func " + node.entry->getName()
                + " at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            res = false;
        }
        funcHasReturn = false;
        curFuncRetType = TypeFactory::getBasicType(Type_t::UNK);
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement FuncDeclStmt semantic checking");
    }

    bool ASTChecker::visit(VarDeclStmt& node)
    {
        // TODO(Lab3-1): 实现变量声明语句的语义检查
        // 空声明直接通过，否则委托给变量声明处理
        if(node.decl->decls->empty()) return true;
        bool res = apply(*this, *node.decl);
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement VarDeclStmt semantic checking");
    }

    bool ASTChecker::visit(BlockStmt& node)
    {
        // TODO(Lab3-1): 实现块语句的语义检查
        // 进入新作用域，逐条访问语句，退出作用域
        if(!node.stmts) return true;
        symTable.enterScope();
        bool res = true;  // 修复：初始化为true
        for(auto x : *node.stmts) {
            res &= apply(*this, *x);
        }
        symTable.exitScope();
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement BlockStmt semantic checking");
    }

    bool ASTChecker::visit(ReturnStmt& node)
    {
        // TODO(Lab3-1): 实现返回语句的语义检查
        // 设置返回标记，检查作用域，检查返回值类型匹配
        bool res = apply(*this, *node.retExpr);
        if(curFuncRetType->getBaseType() == Type_t::UNK) {
            std::string error_str = "return stmt not in func at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            return false;
        }
        funcHasReturn = 1;
        if(!res) return false;
        if(node.retExpr->attr.val.value.type->getTypeGroup() == TypeGroup::POINTER) {
            std::string error_str = "unavaliable return type: pointer , at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            return false;
        }
        if(node.retExpr->attr.val.value.type->getBaseType() == curFuncRetType->getBaseType()) {
            return true;
        } else if(node.retExpr->attr.val.value.type->getBaseType() != Type_t::VOID && curFuncRetType->getBaseType() != Type_t::VOID) {
            return true;
        } else {
            std::string error_str = "unmatched return type at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            return false;
        }
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement ReturnStmt semantic checking");
    }

    bool ASTChecker::visit(WhileStmt& node)
    {
        // TODO(Lab3-1): 实现while循环的语义检查
        // 检查作用域，访问条件表达式，管理循环深度，访问循环体
        bool res = apply(*this, *node.cond);
        if(res && (node.cond->attr.val.value.type->getBaseType() == Type_t::VOID
            || node.cond->attr.val.value.type->getTypeGroup() == TypeGroup::POINTER)) {
                std::string error_str = "invalid type expr for while condition at line " + std::to_string(node.line_num);
                errors.emplace_back(error_str);
                res = false;
            }
        ++loopDepth;
        res &= apply(*this, *node.body);
        --loopDepth;
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement WhileStmt semantic checking");
    }

    bool ASTChecker::visit(IfStmt& node)
    {
        // TODO(Lab3-1): 实现if语句的语义检查
        // 检查作用域，访问条件表达式，分别访问then和else分支
        bool res = apply(*this, *node.cond);
        bool tmp = funcHasReturn;
        funcHasReturn = false;
        if(res && (node.cond->attr.val.value.type->getBaseType() == Type_t::VOID
            || node.cond->attr.val.value.type->getTypeGroup() == TypeGroup::POINTER)) {
                res = false;
                std::string error_str = "invalid type expr as if condition at line " + std::to_string(node.line_num);
                errors.emplace_back(error_str);
            }
        res &= apply(*this, *node.thenStmt);
        bool ifhasret = funcHasReturn;
        funcHasReturn = false;
        if(node.elseStmt) {
            res &= apply(*this, *node.elseStmt);
            ifhasret &= funcHasReturn;
        } else ifhasret = false;
        funcHasReturn = ifhasret | tmp;
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement IfStmt semantic checking");
    }

    bool ASTChecker::visit(BreakStmt& node)
    {
        // TODO(Lab3-1): 实现break语句的语义检查
        // 检查是否在循环内使用
        (void)node;
        return loopDepth;
        // TODO("Lab3-1: Implement BreakStmt semantic checking");
    }

    bool ASTChecker::visit(ContinueStmt& node)
    {
        // TODO(Lab3-1): 实现continue语句的语义检查
        // 检查是否在循环内使用
        (void)node;
        return loopDepth;
        // TODO("Lab3-1: Implement ContinueStmt semantic checking");
    }

    bool ASTChecker::visit(ForStmt& node)
    {
        // TODO(Lab3-1): 实现for循环的语义检查
        // 检查作用域，访问初始化、条件、步进表达式，管理循环深度
        symTable.enterScope();
        ++loopDepth;
        bool res = true;
        if(node.init) res &= apply(*this, *node.init);
        if(node.cond) {
            res &= apply(*this, *node.cond);
        }
        if(node.step) res &= apply(*this, *node.step);
        if(node.body) res &= apply(*this, *node.body);
        --loopDepth;
        symTable.exitScope();
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement ForStmt semantic checking");
    }
}  // namespace FE::AST
