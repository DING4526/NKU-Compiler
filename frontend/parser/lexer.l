%{
    #include <frontend/parser/yacc.h>
    #include <frontend/parser/scanner.h>
    #include <frontend/parser/location.hh>
    #include <frontend/parser/parser.h>
    #include <stdexcept>
    #include <string>
    #include <limits>      
    #include <cmath>       

    using namespace FE;

    static location loc;

    // YY_USER_ACTION 宏会自动插入到每个规则的代码块开头
    // 如下方的 "int"               { RETT(INT, loc) }
    // 实际上会被转换为
    // "int"               { loc.step(); loc.columns(yyleng); RETT(INT, loc) }
    // 这样就能自动更新 loc 的位置
    // yyleng 是当前匹配的字符串的长度
    #define YY_USER_ACTION      \
        loc.step();             \
        loc.columns(yyleng);

    #define yywrap() 1
    #define RETT(x, y) \
        return YaccParser::make_##x(y);

    #define TAB_WIDTH 4

    int handleTab();
    long long convertToInt(const char* str, const char end, bool& isLongLong);
    float convertToFloatDec(const char* str);
    float convertToFloatHex(const char* str);
%}

%option c++
%option noyywrap debug
%option yyclass="Scanner"
%option prefix="Yacc_"

%x COMMENT

%{
/*
在词法分析中，你需要重点考虑result的四种类型，分别为int_token, float_token, string_token, symbol_token。
之所以要定义这四种类型，是因为我们只返回这个token的类型是不够的，我们还需要知道这个token的属性。
例如我们需要知道INT_CONST具体的值是多少。
如果你需要其它 token，需要先到 yacc.y 中定义 token，然后在这里实现。

TODO(Lab1)：参考SysY2022的定义，实现更多的词法，如浮点数、十六进制/八进制数等
*/
%}

/* ========= 命名正则（Definitions） ========= */

/* ===== 基本字符类 ===== */
DIGIT        [0-9]
NZDIG        [1-9]
OCTDIG       [0-7]
HEXDIG       [0-9A-Fa-f]
IDSTART      [A-Za-z_]
IDCONT       [A-Za-z0-9_]

/* ===== 复合词法单元 ===== */
ID           {IDSTART}{IDCONT}*
WS           [ \f\r\v]+
SL_COMMENT   "//"[^\n]*

/* ===== 整型常量（含后缀） ===== */
INTSUF       ([uU]([lL]{1,2})?|([lL]{1,2})[uU]?)     
DECINT       ({NZDIG}{DIGIT}*{INTSUF}?)      
OCTINT       (0{OCTDIG}*{INTSUF}?)          
HEXINT       (0[xX]{HEXDIG}+{INTSUF}?)       

/* ===== 浮点常量（含后缀） ===== */
/* 指数部分 */
EXP          [eE][+-]?{DIGIT}+           
BEXP         [pP][+-]?{DIGIT}+           
FLSUFF       [fFlL]                        

/* 十进制小数三种形态 */
DFRAC1       ({DIGIT}+\.{DIGIT}+)            
DFRAC2       ({DIGIT}+\.)                  
DFRAC3       (\.{DIGIT}+)                  
/* 十进制浮点主体：小数部分（可带指数）或 整数+指数 */
DECFLTBODY   ({DFRAC1}|{DFRAC2}|{DFRAC3})({EXP})?|{DIGIT}+{EXP}
DECFLT       ({DECFLTBODY}{FLSUFF}?)          

/* 十六进制小数三种形态 */
HXFRAC1      (0[xX]{HEXDIG}+\.{HEXDIG}*)
HXFRAC2      (0[xX]{HEXDIG}*\.{HEXDIG}+)
HXINTHEX     (0[xX]{HEXDIG}+)
/* 十六进制浮点主体：小数+指数 或 整数+指数 */
HEXFLTBODY   ({HXFRAC1}|{HXFRAC2}){BEXP}|{HXINTHEX}{BEXP}
HEXFLT       ({HEXFLTBODY}{FLSUFF}?)          

/* ===== 字符串（含转义） ===== */
STRING       \"([^\"\\]|\\.)*\"



%%



\n      { loc.lines(yyleng); loc.step(); }
{WS}    { /* empty */ }
[\t]    { loc.columns(handleTab()); }


"if"                { RETT(IF, loc) }
"else"              { RETT(ELSE, loc) }
"for"               { RETT(FOR, loc) }
"while"             { RETT(WHILE, loc) }
"continue"          { RETT(CONTINUE, loc) }
"break"             { RETT(BREAK, loc) }
"switch"            { RETT(SWITCH, loc) }
"case"              { RETT(CASE, loc) }
"goto"              { RETT(GOTO, loc) }
"do"                { RETT(DO, loc) }
"return"            { RETT(RETURN, loc) }
"const"             { RETT(CONST, loc) }
"int"               { RETT(INT, loc) }
"float"             { RETT(FLOAT, loc) }
"bool"              { RETT(BOOL, loc) }
"long long"         { RETT(LL, loc) }
"void"              { RETT(VOID, loc) }

";"                 { RETT(SEMICOLON, loc) }
","                 { RETT(COMMA, loc) }
"("                 { RETT(LPAREN, loc) }
")"                 { RETT(RPAREN, loc) }
"["                 { RETT(LBRACKET, loc) }
"]"                 { RETT(RBRACKET, loc) }
"{"                 { RETT(LBRACE, loc) }
"}"                 { RETT(RBRACE, loc) }


{SL_COMMENT}              { 
    auto str = std::string(yytext);
    // return YaccParser::make_SLASH_COMMENT(str, loc);  //思考为什么要把这一行注释掉
}


"/*"            { BEGIN(COMMENT); }

<COMMENT>"*/"   { BEGIN(INITIAL); }
<COMMENT>\n     { loc.lines(1); loc.step(); }
<COMMENT>.      { /* 吃一个非换行字符 */ }
<COMMENT><<EOF>> {
    std::string msg = "unterminated block comment";
    return YaccParser::make_ERR_TOKEN(msg, loc);
}


{ID} {
    auto str = std::string(yytext);
    return YaccParser::make_IDENT(str, loc); 
}

{STRING}          { 
    std::string str(yytext);
    if (!str.empty()) str = str.substr(1, str.size() - 2);
    return YaccParser::make_STR_CONST(str, loc); 
}


{HEXFLT} {
    try {
        std::string s(yytext, yyleng);
        if (!s.empty()) {
            char c = s.back();
            if (c=='f'||c=='F'||c=='l'||c=='L') s.pop_back(); 
        }
        float v = convertToFloatHex(s.c_str());
        return YaccParser::make_FLOAT_CONST(v, loc);
    } catch (...) {
        return YaccParser::make_ERR_TOKEN(std::string(yytext), loc);
    }
}

{DECFLT} {
    try {
        std::string s(yytext, yyleng);
        if (!s.empty()) {
            char c = s.back();
            if (c=='f'||c=='F'||c=='l'||c=='L') s.pop_back();
        }
        float v = convertToFloatDec(s.c_str());
        return YaccParser::make_FLOAT_CONST(v, loc);
    } catch (...) {
        return YaccParser::make_ERR_TOKEN(std::string(yytext), loc);
    }
}

({HEXINT}|{OCTINT}|{DECINT}) {
    try {
        // 先剥后缀：U/u 与 L/l 或 LL/ll
        std::string s(yytext, yyleng);
        bool hasU = false;
        (void)hasU;
        int  Ls   = 0;  // 0/1/2
        while (!s.empty()) {
            char c = s.back();
            if (c=='u'||c=='U') { hasU = true; s.pop_back(); }
            else if (c=='l'||c=='L') {
                s.pop_back(); ++Ls;
                if (!s.empty() && (s.back()=='l'||s.back()=='L')) { s.pop_back(); ++Ls; }
            } else break;
        }

        // 解析数值（自动识别 0x/前导0 的进制）
        bool isLL = false;
        long long val = convertToInt(s.c_str(), '\0', isLL);

        // 合成 token：有 LL 后缀或超出 int 范围 --> LL_CONST
        if (Ls >= 2 || isLL)
            return YaccParser::make_LL_CONST(val, loc);
        else
            return YaccParser::make_INT_CONST(static_cast<int>(val), loc);
    } catch (...) {
        return YaccParser::make_ERR_TOKEN(std::string(yytext), loc);
    }
}



"=="                { RETT(EQ, loc); }
"!="                { RETT(NE, loc); }
"<="                { RETT(LE, loc); }
">="                { RETT(GE, loc); }
"&&"                { RETT(AND, loc); }
"||"                { RETT(OR, loc); }
"<<"                { RETT(SHL, loc); }
">>"                { RETT(SHR, loc); }
"+="                { RETT(PLUSEQ, loc); }
"-="                { RETT(MINUSEQ, loc); }
"*="                { RETT(MULTEQ, loc); }
"/="                { RETT(DIVEQ, loc); }
"%="                { RETT(MODEQ, loc); }

"="                 { RETT(ASSIGN, loc); }
"+"                 { RETT(PLUS, loc); }
"-"                 { RETT(MINUS, loc); }
"*"                 { RETT(STAR, loc); }
"/"                 { RETT(SLASH, loc); }
"%"                 { RETT(PERCENT, loc); }
"<"                 { RETT(LT, loc); }
">"                 { RETT(GT, loc); }
"!"                 { RETT(NOT, loc); }
"&"                 { RETT(BAND, loc); }
"|"                 { RETT(BOR, loc); }
"^"                 { RETT(BXOR, loc); }
"~"                 { RETT(BNOT, loc); }


. {
    std::string s = std::string(yytext);
    return YaccParser::make_ERR_TOKEN(s, loc);
}


<<EOF>>         { RETT(END, loc); }

%%

int handleTab()
{
    // 该函数用于处理制表符，根据当前位置计算制表符应占用的列数
    // 如，在TAB_WIDTH为4的情况下：
    // 对于" \t"，则会对 '\t' 返回 3
    // 对于"   \t"，则会对 '\t' 返回 1
    // 对于"    \t"，则会对 '\t' 返回 4

    int& c = loc.begin.column;
    return TAB_WIDTH - ((c - 1) % TAB_WIDTH) - 1;
}

long long convertToInt(const char* str, const char end, bool& isLongLong)
{
    // 该函数用于将字符串转换为整数，能正确处理格式无误的十进制、十六进制和八进制整数
    // 接受一个 isLongLong 引用参数，用于指示结果是否超出 int 范围

    int         base       = 10;
    long long   result     = 0;
    int         isNegative = 0;
    const char* ptr        = str;
    static int  zeroOffset = '0';
    static int  aOffset    = 'a' - 10;
    static int  AOffset    = 'A' - 10;
    int*        offset     = NULL;
    isLongLong             = false;

    if (*ptr == '-')
    {
        isNegative = 1;
        ++ptr;
    }
    else if (*ptr == '+') { ++ptr; }

    if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
    {
        base = 16;
        ptr += 2;
    }
    else if (ptr[0] == '0')
    {
        base = 8;
        ptr += 1;
    }

    while (*ptr != end)
    {
        int value = 0;

        if (base == 16)
        {
            if (*ptr >= '0' && *ptr <= '9')
                offset = &zeroOffset;
            else if (*ptr >= 'a' && *ptr <= 'f')
                offset = &aOffset;
            else if (*ptr >= 'A' && *ptr <= 'F')
                offset = &AOffset;
        }
        else { offset = &zeroOffset; }

        value  = *ptr - *offset;
        result = result * base + value;
        ++ptr;
    }

    if (isNegative) result = -result;

    if (result > std::numeric_limits<int>::max() || result < std::numeric_limits<int>::min())
    {
        isLongLong = true;
        if (result > std::numeric_limits<long long>::max() || result < std::numeric_limits<long long>::min())
        {
            throw std::out_of_range(str + std::string(" overflow or underflow for long long"));
        }
    }

    return result;
}

float convertToFloatDec(const char* str)
{
    // 该函数用于将字符串转换为浮点数，能正确处理格式无误的十进制浮点数

    const char* head = str;
    if (str == NULL) { return 0.0f; }

    int sign = 1;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+') { str++; }

    double integerPart = 0.0;
    while (isdigit(*str))
    {
        integerPart = integerPart * 10 + (*str - '0');
        str++;
    }

    double fractionPart = 0.0;
    if (*str == '.')
    {
        str++;
        double divisor = 10.0;
        while (isdigit(*str))
        {
            fractionPart += (*str - '0') / divisor;
            divisor *= 10.0;
            str++;
        }
    }

    double value = integerPart + fractionPart;

    if (*str == 'e' || *str == 'E')
    {
        str++;
        int expSign = 1;
        if (*str == '-')
        {
            expSign = -1;
            str++;
        }
        else if (*str == '+') { str++; }

        int exponent = 0;
        while (isdigit(*str))
        {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        value *= pow(10, expSign * exponent);
    }

    value = sign * value;

    if (value > std::numeric_limits<float>::max() || value < -std::numeric_limits<float>::max())
    {
        throw std::out_of_range(
            head + ((value > std::numeric_limits<float>::max()) ? std::string(" overflow") : std::string(" underflow")));
    }

    return static_cast<float>(value);
}

float convertToFloatHex(const char* str)
{
    // 该函数用于将字符串转换为浮点数，能正确处理格式无误的十六进制浮点数

    const char* head = str;
    if (str == NULL) { return 0.0f; }

    int sign = 1;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+') { str++; }

    if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) { str += 2; }

    unsigned long long integerPart = 0;
    while (isxdigit(*str))
    {
        int digit = 0;
        if (isdigit(*str)) { digit = (*str - '0'); }
        else if (*str >= 'a' && *str <= 'f') { digit = (*str - 'a' + 10); }
        else if (*str >= 'A' && *str <= 'F') { digit = (*str - 'A' + 10); }
        integerPart = integerPart * 16 + digit;
        str++;
    }

    double fractionPart = 0.0;
    if (*str == '.')
    {
        str++;
        unsigned long long numerator   = 0;
        unsigned long long denominator = 1;
        while (isxdigit(*str))
        {
            int digit = 0;
            if (isdigit(*str)) { digit = (*str - '0'); }
            else if (*str >= 'a' && *str <= 'f') { digit = (*str - 'a' + 10); }
            else if (*str >= 'A' && *str <= 'F') { digit = (*str - 'A' + 10); }
            numerator = numerator * 16 + digit;
            denominator *= 16;
            str++;
        }
        fractionPart = (double)numerator / (double)denominator;
    }

    double value = (double)integerPart + fractionPart;

    if (*str == 'p' || *str == 'P')
    {
        str++;
        int expSign = 1;
        if (*str == '-')
        {
            expSign = -1;
            str++;
        }
        else if (*str == '+') { str++; }

        int exponent = 0;
        while (isdigit(*str))
        {
            exponent = exponent * 10 + (*str - '0');
            str++;
        }

        value *= pow(2, expSign * exponent);
    }

    value = sign * value;

    if (value > std::numeric_limits<float>::max() || value < -std::numeric_limits<float>::max())
    {
        throw std::out_of_range(
            head + ((value > std::numeric_limits<float>::max()) ? std::string(" overflow") : std::string(" underflow")));
    }

    return static_cast<float>(value);
}
