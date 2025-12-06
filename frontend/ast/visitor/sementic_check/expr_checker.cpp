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

// 保留：未定义变量检查、下标表达式类型检查；
// 暂时关闭：越界常量检查、数组元素级常量折叠、通过 initList[pos] 取值这些“高火力”操作。

        // 如果有数组下标，先检查每个下标表达式的类型是否合法
        if (node.indices){
            bool res = true;
            for (auto* idxExpr : *node.indices){
                res &= apply(*this, *idxExpr);
                auto* t = idxExpr->attr.val.value.type;

                // 要求是基本类型，且不能是 void；如果要更严，可以要求是整型
                if (t->getTypeGroup() != TypeGroup::BASIC || t->getBaseType() == Type_t::VOID){
                    std::string error_str = "illegal index expression at line "
                        + std::to_string(node.line_num);
                    errors.emplace_back(error_str);
                    res = false;
                }
            }
            if (!res) return false;

            // 有下标访问时，把结果类型视作“元素类型”，避免在后续算术中被当成 pointer
            Type* elemType = attr->type;
            if (attr->type->getTypeGroup() == TypeGroup::POINTER){
                // pointer 的 baseType 即元素基本类型
                elemType = TypeFactory::getBasicType(attr->type->getBaseType());
            }
            node.attr.val.value.type  = elemType;
            node.attr.val.isConstexpr = false;  // 这里不再尝试通过 initList 做常量折叠
        }
        else{
            // 没有下标，但这是一个数组变量本身的访问（例如传给函数当指针用）
            // 保持 attr->type，不做特别处理
            // 常量折叠仅在真正需要时再做，这里直接使用 attr->isConstDecl 即可
        }

        // //是指针类型
        // if((node.indices && attr->arrayDims.size() > node.indices->size())
        //     || (!node.indices && attr->arrayDims.size())) {
        //     //node.isLval = false;
        //     node.attr.val.isConstexpr = false;
        //     node.attr.val.value.type = TypeFactory::getPtrType(attr->type);
        // } else {
        //     //node.isLval = true;
        // }
        // int sum = 1, pos = 0;
        // bool res = 1;
        // //如果是数组或者数组指针，判断是否越界
        // if(node.indices) {
        //     if(node.indices->size() > attr->arrayDims.size()) return false;
        //     for(auto x : *node.indices) {
        //         res &= apply(*this, *x);
        //         res &= x->attr.val.value.type->getTypeGroup() == TypeGroup::BASIC;
        //         res &= !x->attr.val.isConstexpr || x->attr.val.value.type->getBaseType() == Type_t::INT;
        //     }
        //     if(!res) {
        //         std::string error_str = "illegal shuzu at line " + std::to_string(node.line_num); 
        //         errors.emplace_back(error_str);
        //         return false;
        //     }
        //     for(size_t i = 0; i < node.indices->size(); ++i) {
        //         auto x = (*node.indices)[i];
        //         auto y = attr->arrayDims[i];
        //         if(!x->attr.val.isConstexpr) {
        //             node.attr.val.isConstexpr = false;
        //             continue;
        //         }
        //         int lit = x->attr.val.getInt();
        //         if(y != -1 && (lit >= y || lit < 0))
        //             return false;
        //         sum *= y;
        //     }
        //     for(size_t i = 0; i < node.indices->size(); ++i) {
        //         sum /= attr->arrayDims[i];
        //         int x = ((LiteralExpr*)(*node.indices)[i])->literal.intValue;
        //         pos += x * sum;
        //     }
        // }

        //如果是常量
        int pos = 0;
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
        // 1. 函数是否存在
        if (!funcDecls.count(node.func)) {
            std::string error_str = "undefined function " + node.func->getName()
                                    + " at line " + std::to_string(node.line_num);
            errors.emplace_back(error_str);
            return false;
        }

        auto func = funcDecls[node.func];

        // 设置返回类型属性：这里非常重要，IR 生成要用到
        node.attr.val.value.type = func->retType;
        node.attr.val.isConstexpr = false;   // 函数调用默认不是编译期常量

        // 2. 参数个数检查（这个保留，否则很多明显错误会直接放进 IR 崩掉）
        bool func_no_params = (!func->params) || func->params->empty();
        bool call_no_args   = (!node.args)   || node.args->empty();

        if (func_no_params && call_no_args) {
            return true;
        }

        size_t param_cnt = func->params ? func->params->size() : 0;
        size_t arg_cnt   = node.args   ? node.args->size()   : 0;

        if (param_cnt != arg_cnt) {
            std::string error_str = "number of par not matched for func " + node.func->getName()
                                    + " at line " + std::to_string(node.line_num)
                                    + " column " + std::to_string(node.col_num);
            errors.emplace_back(error_str);
            return false;
        }

        // 3. 逐个实参做最基本的检查：表达式本身合法 + 不能是 void
        bool ok = true;
        for (size_t i = 0; i < param_cnt; ++i) {
            auto* arg = (*node.args)[i];
            ok &= apply(*this, *arg);

            auto* argType = arg->attr.val.value.type;
            if (!argType) continue;

            // 不允许 void 实参与参与运算，这是对 IR 来说致命错误
            if (argType->getBaseType() == Type_t::VOID) {
                ok = false;
                std::string error_str =
                    "unavaliable argument (void) for func " + node.func->getName() +
                    " at line " + std::to_string(node.line_num);
                errors.emplace_back(error_str);
            }

            // 数组/指针/维度：一律不在这里管，交给 VarAttr + IR codegen 处理
        }

        return ok;
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
