#include "frontend/ast/ast_defs.h"
#include "frontend/ast/expr.h"
#include "ivisitor.h"
#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>
#include <string>

namespace FE::AST
{
    bool ASTChecker::visit(LeftValExpr& node)
    {
        // TODO(Lab3-1): 实现左值表达式的语义检查
        // 检查变量是否存在，处理数组下标访问，进行类型检查和常量折叠
        auto attr = symTable.getSymbol(node.entry);
        if(attr == nullptr) {
            std::string error_str = "undefined var " + node.entry->getName()
                        + " at line " + std::to_string(node.line_num);
            errors.push_back(error_str);
            return false;
        }
        node.attr.val.value.type = attr->type;
        node.attr.val.isConstexpr = attr->isConstDecl;
        //是指针类型
        if((node.indices && attr->arrayDims.size() > node.indices->size())
            || (!node.indices && attr->arrayDims.size())) {
            //node.isLval = false;
            node.attr.val.isConstexpr = false;
            node.attr.val.value.type = TypeFactory::getPtrType(attr->type);
        } else {
            //node.isLval = true;
        }
        int sum = 1, pos = 0;
        bool res = 1;
        //如果是数组或者数组指针，判断是否越界
        if(node.indices) {
            if(node.indices->size() > attr->arrayDims.size()) return false;
            for(auto x : *node.indices) {
                res &= apply(*this, *x);
                res &= x->attr.val.value.type->getTypeGroup() == TypeGroup::BASIC;
                res &= !x->attr.val.isConstexpr || x->attr.val.value.type->getBaseType() == Type_t::INT;
            }
            if(!res) {
                std::string error_str = "illegal shuzu at line " + std::to_string(node.line_num); 
                errors.emplace_back(error_str);
                return false;
            }
            for(size_t i = 0; i < node.indices->size(); ++i) {
                auto x = (*node.indices)[i];
                auto y = attr->arrayDims[i];
                if(!x->attr.val.isConstexpr) {
                    node.attr.val.isConstexpr = false;
                    continue;
                }
                int lit = x->attr.val.getInt();
                if(y != -1 && (lit >= y || lit < 0))
                    return false;
                sum *= y;
            }
            for(size_t i = 0; i < node.indices->size(); ++i) {
                sum /= attr->arrayDims[i];
                int x = ((LiteralExpr*)(*node.indices)[i])->literal.intValue;
                pos += x * sum;
            }
        }
        //如果是常量
        if(node.attr.val.isConstexpr) {
            node.attr.val = ExprValue(attr->initList[pos], true);
        }
        return true;
        // TODO("Lab3-1: Implement LeftValExpr semantic checking");
    }

    bool ASTChecker::visit(LiteralExpr& node)
    {
        // 示例实现：字面量表达式的语义检查
        // 字面量总是编译期常量，直接设置属性
        node.attr.val.isConstexpr = true;
        node.attr.val.value       = node.literal;
        return true;
    }

    bool ASTChecker::visit(UnaryExpr& node)
    {
        // TODO(Lab3-1): 实现一元表达式的语义检查
        // 访问子表达式，检查操作数类型，调用类型推断函数
        if(!apply(*this, *node.expr)) return false;
        auto tp = node.expr->attr.val.value.type;
        if(tp->getBaseType() == Type_t::VOID || tp->getTypeGroup() == TypeGroup::POINTER) {
            std::string error_str = "unavaliable unary expr at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            return false;
        }
        bool res = false;
        node.attr.val = typeInfer(node.expr->attr.val, node.op, node, res);
        return !res;
        // (void)node;
        // TODO("Lab3-1: Implement UnaryExpr semantic checking");
    }

    bool ASTChecker::visit(BinaryExpr& node)
    {
        // TODO(Lab3-1): 实现二元表达式的语义检查
        // 访问左右子表达式，检查操作数类型，调用类型推断
        bool res = apply(*this, *node.lhs) && apply(*this, *node.rhs);
        auto ltp = node.lhs->attr.val.value.type, rtp = node.rhs->attr.val.value.type;
        if(!res) return false;
        if(ltp->getBaseType() == Type_t::VOID || ltp->getTypeGroup() == TypeGroup::POINTER
            ||rtp->getBaseType() == Type_t::VOID || rtp->getTypeGroup() == TypeGroup::POINTER) {
                std::string error_str = "unavaliable binary expr at line " +std::to_string(node.line_num);
                errors.emplace_back(error_str);
                return false;
            }
        bool hasError = false;
        auto &lval = node.lhs->attr.val, &rval = node.rhs->attr.val;
        node.attr.val = typeInfer(lval, rval, node.op, node, hasError);
        return res && !hasError;
        // (void)node;
        // TODO("Lab3-1: Implement BinaryExpr semantic checking");
    }

    bool ASTChecker::visit(CallExpr& node)
    {
        // TODO(Lab3-1): 实现函数调用表达式的语义检查
        // 检查函数是否存在，访问实参列表，检查参数数量和类型匹配
        if(!funcDecls.count(node.func)) {
            std::string error_str = "undefined function " + node.func->getName()
                        + " at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            return false;
        }
        auto func = funcDecls[node.func];
        node.attr.val.value.type = func->retType;
        node.attr.val.isConstexpr = false;
        
        // treat empty param list as no params
        bool func_no_params = (!func->params) || (func->params->size() == 0);
        bool call_no_args   = (!node.args) || (node.args->size() == 0);
        if(func_no_params && call_no_args) return true;

        if(func->params && node.args && func->params->size() == node.args->size()) {
            bool res = 1, parunmatch = 0;
            for(size_t i = 0; i < func->params->size(); ++i) {
                auto param = (*func->params)[i];
                auto arg = (*node.args)[i];
                res &= apply(*this, *arg);
                //不是指针
                parunmatch |= arg->attr.val.value.type->getBaseType() == Type_t::VOID;
                if(arg->attr.val.value.type->getTypeGroup() == TypeGroup::BASIC) {
                    res &= !param->dims;
                } else { //是指针类型
                    parunmatch |= arg->attr.val.value.type->getBaseType() != param->type->getBaseType();
                    auto lval = (LeftValExpr*) arg;
                    auto vec = symTable.getSymbol(lval->entry)->arrayDims;
                    int num = 0;
                    if(lval->indices) num = lval->indices->size();
                    for(size_t i = 0; i < param->dims->size(); ++i) {
                        if((*param->dims)[i]->attr.val.getInt() == -1
                            || (*param->dims)[i]->attr.val.getInt() == vec[i + num])
                            continue;
                        else {
                            res = 0;
                            std::string error_str = "unmatched dim for par " + param->entry->getName() 
                                + " at line " + std::to_string(node.line_num);
                            errors.emplace_back(error_str);
                        }
                    }
                }
            }
            if(parunmatch) {
                std::string error_str = "unmatched par for func " + node.func->getName()
                    + " at line " + std::to_string(node.line_num) + " column " + std::to_string(node.col_num);
                errors.emplace_back(error_str);
            }
            return res && !parunmatch;
        } else {
            std::string error_str = "number of par not matched for func " + node.func->getName()
                    + " at line " + std::to_string(node.line_num) + " column " + std::to_string(node.col_num);
            errors.emplace_back(error_str);
            return false;
        }
        // (void)node;
        // TODO("Lab3-1: Implement CallExpr semantic checking");
    }

    bool ASTChecker::visit(CommaExpr& node)
    {
        // TODO(Lab3-1): 实现逗号表达式的语义检查
        // 依序访问各子表达式，结果为最后一个表达式的属性
        if(node.exprs->empty()) return true;
        bool res = 1;
        for(auto x : *node.exprs) {
            res &= apply(*this, *x);
        }
        node.attr = node.exprs->back()->attr;
        return res;
        // (void)node;
        // TODO("Lab3-1: Implement CommaExpr semantic checking");
    }
}  // namespace FE::AST
