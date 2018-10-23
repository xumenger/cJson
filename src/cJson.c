/*********************************************************************************
 * Copyright(C), xumenger
 * FileName     : cJson.c
 * Author       : xumenger
 * Version      : V1.0.0 
 * Date         : 2017-08-09
 * Description  : 
     1.cJson.h接口的代码实现
**********************************************************************************/
#include <stdio.h>
#include <assert.h>   /* assert() */
#include <errno.h>    /* errno, ERANGE */
#include <math.h>     /*HUGE_VAL*/
#include <stdlib.h>   /* NULL, strtod(), malloc(), realloc(), free() */
#include <string.h>   /*memcpy*/
#include "cJson.h"
#include "cJsonStruct.h"

//用 #ifndef X #define X ... #endif 的好处是：可在编译选项中自行设置宏，没设置就用缺省值
#ifndef STACK_INIT_SIZE
#define STACK_INIT_SIZE 256
#endif

#ifndef STRINGIFY_STACK_INIT_SIZE
#define STRINGIFY_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)      do { assert(*c->json == (ch)); c->json++;} while(0)
#define ISDIGIT(ch)        ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)    ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)        do { *(char *)ContextPush(c, sizeof(char)) = (ch); } while(0)
//在栈上申请len字节，将s字符串的内容拷贝进去
#define PUTS(c, s, len)    memcpy(ContextPush(c, len), s, len)

static void ParseWhiteSpace(CJSONContext *c);
static int ParseLiteral(CJSONContext *c, CJSONValue *v, const char *literal, CJSONType type);
static int ParseNumber(CJSONContext *c, CJSONValue *v);
static int ParseStringRaw(CJSONContext *c,  char **str, size_t *len);
static int ParseString(CJSONContext *c, CJSONValue *v);
static int ParseArray(CJSONContext *c, CJSONValue *v);
static int ParseObject(CJSONContext *c, CJSONValue *v);
static int ParseValue(CJSONContext *c, CJSONValue *v);
static int StringifyValue(CJSONContext *c, const CJSONValue *v);
static void *ContextPush(CJSONContext *c, size_t size);
static void *ContextPop(CJSONContext *c, size_t size);

/*******************************************************************************
* Function   : Parse
* Description: 按照 JSON 语法的 EBNF 简单翻译成解析函数
* Input      :
    * v, 一个Json节点; json, 一个待解析的Json格式字符串
* Output     :
* Return     : 
* Others     : 
*******************************************************************************/
int Parse(CJSONValue *v, const char *json)
{
    CJSONContext c;
    int ret;
    assert(NULL != v);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    INIT_VALUE_NULL(v);
    ParseWhiteSpace(&c);
    if((ret = ParseValue(&c, v)) == PARSE_OK){
        //Json文本应该有3部分：`ws value ws`
        //需要对三个部分都进行解析，解析空白，然后检查Json文本是否完结
        ParseWhiteSpace(&c);
        if(*c.json != '\0')
            ret = PARSE_ROOT_NOT_SINGULAR;
    }
    //加断言，保证所有数据都被弹出
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

/*******************************************************************************
* Function   : Stringify
* Description: 
    * 将在内存中的树形结构生成对应的JSON字符串
    * 该API提供length可选参数，当传入非空指针时，就能获得生成JSON的长度
    * 为什么需要获得长度，不是可以用strlen()获取吗？
    * 多了一个strlen()调用会带来不必要的性能消耗，理想地避免调用方有额外消耗
* Input      :
    * v, 树形结构的根节点
    * length, 可选，存储 JSON 的长度，传入 NULL 可忽略此参数
* Output     : 
    * json, json格式的字符串
* Return     : 
    * 
* Others     : 
*******************************************************************************/
int Stringify(const CJSONValue *v, char **json, size_t *length)
{
    CJSONContext c;
    int ret;
    assert(NULL != v);
    assert(NULL != json);
    //初始化一个STRINGIFY_STACK_INIT_SIZE大小的堆
    c.stack = (char *)malloc(c.size = STRINGIFY_STACK_INIT_SIZE);
    c.top = 0;
    //以v为根节点解析，将解析结果放到c中
    if((ret = StringifyValue(&c, v)) != STRINGIFY_OK){
        free(c.stack);
        *json = NULL;
        return ret;
    }
    //获取解析结果长度
    if(length)
        *length = c.top;
    //将栈顶设置为'\0'，表示字符串结尾
    PUTC(&c, '\0');
    *json = c.stack;
    return STRINGIFY_OK;
}

/*******************************************************************************
* Function   : GetType
* Description: 获取Json的某个节点值的类型
* Input      :
    * v, 一个Json节点
* Output     :
* Return     : JSON节点的类型
* Others     : 
*******************************************************************************/
CJSONType GetType(const CJSONValue *v)
{
    assert(NULL != v);
    return v->type;
}

/*******************************************************************************
* Function   : GetBoolean
* Description: 将JSON节点作为布尔类型获得其值
* Input      :
    * v, 一个Json节点
* Output     :
* Return     : TYPE_FALSE; TYPE_TRUE
* Others     : 
*******************************************************************************/
int GetBoolean(const CJSONValue *v)
{
    assert(NULL != v && (v->type == TYPE_TRUE || v->type == TYPE_FALSE));
    return v->type;
}

/*******************************************************************************
* Function   : SetBoolean
* Description: 将JSON节点作为布尔类型设置其值
* Input      :
    * v, 一个Json节点
    * b, 要设置的值
* Output     : 
* Return     : 
* Others     : 
*******************************************************************************/
void SetBoolean(CJSONValue *v, int b)
{
    FreeValue(v);
    v->type = b;
}

/*******************************************************************************
* Function   : GetNumber
* Description: 获取Json的某个数值型节点值的数值
* Input      :
    * v, 一个Json节点
* Output     :
* Return     : JSON节点的数值
* Others     : 
*******************************************************************************/
double GetNumber(const CJSONValue *v)
{
    assert(v != NULL && v->type == TYPE_NUMBER);
    return v->u.n;
}

/*******************************************************************************
* Function   : SetNumber
* Description: 设置JSON节点为数值类型
* Input      :
    * v, 一个Json节点
    * n, 要设置的数值
* Output     :
* Return     : 
* Others     : 
*******************************************************************************/
void SetNumber(CJSONValue *v, double n)
{
    FreeValue(v);
    v->u.n = n;
    v->type = TYPE_NUMBER;
}

/*******************************************************************************
* Function   : GetString
* Description: 获得JSON节点的字符串值
* Input      :
    * v, 一个Json节点
* Output     : 
* Return     : 字符串指针
* Others     : 
*******************************************************************************/
const char *GetString(const CJSONValue *v)
{
    assert(v != NULL && v->type == TYPE_STRING);
    return v->u.s.s;
}

/*******************************************************************************
* Function   : GetStringLength
* Description: 获取JSON字符串节点的字符串长度
* Input      : 
    * v, 一个Json节点
* Output     : 
* Return     : 字符串长度
* Others     : 
*******************************************************************************/
size_t GetStringLength(const CJSONValue *v)
{
    assert(v != NULL && v->type == TYPE_STRING);
    return v->u.s.len;
}

/*******************************************************************************
* Function   : SetString
* Description: 设置string类型节点的值
* Input      :
    * s, 字符串值
    * len, 字符串长度
* Output     :
    * v, Json的string节点
* Return     : 
* Others     : 
*******************************************************************************/
void SetString(CJSONValue *v, const char *s, size_t len)
{
    assert(v != NULL && (s != NULL || len == 0));
    FreeValue(v);
    v->u.s.s = (char *)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = TYPE_STRING;
}

/*******************************************************************************
* Function   : GetArraySize
* Description: 获取当前层数组元素个数
* Input      :
    * v, 当前层数组的起始指针
* Output     :
* Return     : 
* Others     : 
*******************************************************************************/
size_t GetArraySize(const CJSONValue *v)
{
    assert(NULL != v && v->type == TYPE_ARRAY);
    return v->u.a.size;
}

/*******************************************************************************
* Function   : GetArrayElement
* Description: 获取当前层数组的第index个元素
* Input      :
    * v, 当前层数组的起始指针
    * index, 元素的顺序
* Output     :
    * v, Json的string节点
* Return     : 
* Others     : 
*******************************************************************************/
CJSONValue *GetArrayElement(const CJSONValue *v, size_t index)
{
    assert(NULL != v && v->type == TYPE_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

/*******************************************************************************
* Function   : GetObjectSize
* Description: 获取当前JSON对象的元素个数
* Input      :
    * v, 当前层对象
* Output     :
* Return     : JSON对象元素个数
* Others     : 
*******************************************************************************/
size_t GetObjectSize(const CJSONValue *v)
{
    assert(v != NULL && v->type == TYPE_OBJECT);
    return v->u.o.size;
}

/*******************************************************************************
* Function   : GetObjectKey
* Description: 获取对象的第index个元素的键
* Input      :
    * v, 当前层数组的起始指针
    * index, 元素的顺序
* Output     : 
* Return     : 键指针
* Others     : 
*******************************************************************************/
const char *GetObjectKey(const CJSONValue *v, size_t index)
{
    assert(v != NULL && v->type == TYPE_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

/*******************************************************************************
* Function   : GetObjectKeyLength
* Description: 获取对象的第index个元素的键长度
* Input      :
    * v, 当前层数组的起始指针
    * index, 元素的顺序
* Output     :
* Return     : 键长度
* Others     : 
*******************************************************************************/
size_t GetObjectKeyLength(const CJSONValue *v, size_t index)
{
    assert(v != NULL && v->type == TYPE_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

/*******************************************************************************
* Function   : GetObjectValue
* Description: 获取对象的第index个元素
* Input      :
    * v, 当前层数组的起始指针
    * index, 元素的顺序
* Output     :
* Return     : 对象的第index个元素
* Others     : 
*******************************************************************************/
CJSONValue *GetObjectValue(const CJSONValue *v, size_t index)
{
    assert(v != NULL && v->type == TYPE_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}

/*******************************************************************************
* Function   : FreeValue
* Description: 释放以v为根节点的树的内存
* Input      :
    * v, Json的节点
* Output     : 
* Return     : 
* Others     : 
*******************************************************************************/
void FreeValue(CJSONValue *v)
{
    assert(v != NULL);
    size_t i;
    switch(v->type){
        case TYPE_STRING:
            free(v->u.s.s);
            break;
        case TYPE_ARRAY:
            for(i = 0; i < v->u.a.size; i++)
                FreeValue(&v->u.a.e[i]);
            free(v->u.a.e);
            break;
        case TYPE_OBJECT:
            for(i = 0; i < v->u.o.size; i++){
                free(v->u.o.m[i].k);
                FreeValue(&v->u.o.m[i].v);
            }
            free(v->u.o.m);
            break;
        default:
            break;
    }
    //避免重复释放
    v->type = TYPE_NULL;
}

/*-----------------------------------------------------------------------------
* Function   : ParseWhiteSpace
* Description: 解析字符串中的空格，如果发现空格、制表符等字符串指针后移
* Input      :
    * c, Json内容
* Output     :
* Return     : 
* Others     : 
-----------------------------------------------------------------------------*/
static void ParseWhiteSpace(CJSONContext *c)
{
    const char *p = c->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p =='\r')
        p++;
    c->json = p;
}

/*-----------------------------------------------------------------------------
* Function   : ParseLiteral
* Description: 按照固定字符串解析literal
* Input      :
    * c, Json内容
    * literal, 用于对比的固定字符串
    * type, 节点类型
* Output     :
    * v, Json节点
* Return     : 
    * PARSE_OK, 解析成功
    * PARSE_INVALID_VALUE, 解析得到非法值
* Others     : JSON的null、false、true的语法描述如下

JSON-text = ws value ws
ws = *(%x20 / %x09 / %x0A / %x0D)
value = null / false / true
null = "null"
false = "false"
true = "true" 
-----------------------------------------------------------------------------*/
static int ParseLiteral(CJSONContext *c, CJSONValue *v, const char *literal, CJSONType type)
{
    //在C中，数组长度、索引值最好使用size_t，而不是int或unsigned
    size_t i;
    EXPECT(c, literal[0]);
    for(i=0; literal[i+1]; i++)
        if(c->json[i] != literal[i+1])
            return PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return PARSE_OK;
}

/*-----------------------------------------------------------------------------
* Function   : ParseNumber
* Description: 解析数值类型
* Input      :
    * c, Json内容
* Output     :
    * v, Json节点
* Return     : 
    * PARSE_OK, 解析成功
    * PARSE_INVALID_VALUE, 解析得到非法值
* Others     : JSON数值类型的语法描述如下

number = [ "-" ] int [ frac ] [ exp ]
int = "0" / digit1-9 *digit
frac = "." 1*digit
exp = ("e" / "E") ["-" / "+"] 1*digit
-----------------------------------------------------------------------------*/
static int ParseNumber(CJSONContext *c, CJSONValue *v)
{
    const char *p = c->json;
    if(*p == '-') 
        p++;
    if(*p == '0') 
        p++;
    else{
        if(!ISDIGIT1TO9(*p))
            return PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    if(*p == '.'){
        p++;
        if(!ISDIGIT(*p))
            return PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    if(*p == 'e' || *p == 'E'){
        p++;
        if(*p == '+' || *p == '-')
            p++;
        if(!ISDIGIT(*p))
            return PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    //前面这些就是按照数值的语法描述对其进行语法校验！
    //这部分是JSON解析的一个重点：如何将语法描述翻译成代码

    //使用strtod转换成数值型
    v->u.n = strtod(c->json, NULL);

    //检查数值是否过大！
    if(errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return PARSE_NUMBER_TOO_BIG;

    v->type = TYPE_NUMBER;
    c->json = p;
    return PARSE_OK;
}

/*-----------------------------------------------------------------------------
* Function   : ParseStringRaw
* Description: 解析字符串
    * 先备份栈顶，然后把解析到的字符串压栈
    * 最后计算出长度并一次性将所有字符弹出，再设置至值里面即可
* Input      :
    * c, Json内容
    * len, 
* Output     :
    * str, 解析出来的字符串
* Return     : 
    * PARSE_OK, 解析成功
    * PARSE_MISS_QUOTATION_MARK, 没有遇到结束"
* Others     : 

string = quotation-mark *char quotation-mark
char = unescaped /
   escape (
      %x22 /                ; "  quotation mark   U+0022
      %x5C /                ; \  reverse solidus  U+005C
      %x2F /                ; /  solidus          U+002F
      %x62 /                ; b  backspace        U+0008
      %x66 /                ; f  form feed        U+000C
      %x6E /                ; n  line feed        U+000A
      %x72 /                ; r  carriage return  U+000D
      %x74 /                ; t  tab              U+0009
      %x75 4HEXDIG )        ; uXXXX               U+XXXX
escape = %x5C               ; \
quotation-mark = %x22       ; "
unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
-----------------------------------------------------------------------------*/
static int ParseStringRaw(CJSONContext *c, char **str, size_t *len)
{
    size_t head = c->top;      //先备份栈顶
    const char *p;

    EXPECT(c, '\"');
    p = c->json;
    for(;;){
        char ch = *p++;
        switch(ch){
            case '\"':
                *len = c->top - head;
                *str = ContextPop(c, *len);
                c->json = p;
                return PARSE_OK;
            //解析转义字符
            case '\\':
                switch(*p++){
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/' : PUTC(c, '/');  break;
                    case 'b' : PUTC(c, '\b'); break;
                    case 'f' : PUTC(c, '\f'); break;
                    case 'n' : PUTC(c, '\n'); break;
                    case 'r' : PUTC(c, '\r'); break;
                    case 't' : PUTC(c, '\t'); break;
                    default:
                       c->top = head;
                       return PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            case '\0':
                c->top = head;
                return PARSE_MISS_QUOTATION_MARK;
            default:
                //处理不合法字符串
                if((unsigned char)ch < 0x20){
                    c->top = head;
                    return PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c, ch);
        }
    }
}

/*-----------------------------------------------------------------------------
* Function   : ParseString
* Description: 
    * ParseStringRaw 主要用于解析普遍的字符串：键值对的键、JSON字符串
    * ParseString    用于在ParseStringRaw的基础上解析JSON的字符串对象
* Input      :
    * c, Json内容
* Output     :
    * v, Json节点
* Return     : 
    * PARSE_OK, 解析成功
    * PARSE_MISS_QUOTATION_MARK, 没有遇到结束"
* Others     : 
-----------------------------------------------------------------------------*/
static int ParseString(CJSONContext *c, CJSONValue *v)
{
    int ret;
    char *s;
    size_t len;
    if((ret = ParseStringRaw(c, &s, &len)) == PARSE_OK)
        SetString(v, s, len);
    return ret;
}

/*-----------------------------------------------------------------------------
* Function   : ParseArray
* Description: 解析JSON数组
* Input      : 
    * c, Json内容
    * v, Json节点
* Output     :
* Return     : 
    * PARSE_OK, 解析成功
    * PARSE_MISS_COMMA_OR_SQUARE_BRACKET
* Others     : 

array = %x5B ws [ value *(ws %x2C ws value) ] ws %x5D
-----------------------------------------------------------------------------*/
static int ParseArray(CJSONContext *c, CJSONValue *v)
{
    size_t size = 0;   //测试过程中遇到过因为未将size初始化导致错误！
    size_t i = 0;
    int ret;
    EXPECT(c, '[');
    ParseWhiteSpace(c);
    if(*c->json == ']'){
        c->json++;
        v->type = TYPE_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return PARSE_OK;
    }
    for(;;){
        //在循环中建立一个临时值，然后调用ParseValue把元素解析到这个临时值
        //然后把临时值压栈，当遇到]，把栈内的元素弹出，分配内存，生成数组值
        CJSONValue e;
        INIT_VALUE_NULL(&e);
        if((ret = ParseValue(c, &e)) != PARSE_OK)
            break;
        //把e的内容压入栈
        memcpy(ContextPush(c, sizeof(CJSONValue)), &e, sizeof(CJSONValue));
        size++;
        ParseWhiteSpace(c);
        if(*c->json == ','){
            c->json++;
            ParseWhiteSpace(c);
        }
        else if(*c->json == ']'){
            c->json++;
            v->type = TYPE_ARRAY;
            v->u.a.size = size;
            size *= sizeof(CJSONValue);
            memcpy(v->u.a.e = (CJSONValue *)malloc(size), ContextPop(c, size), size);
            return PARSE_OK;
        }
        else{
            ret = PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    for(i = 0; i < size; i++)
        FreeValue((CJSONValue *)ContextPop(c, sizeof(CJSONValue)));
    return ret;
}

/*-----------------------------------------------------------------------------
* Function   : ParseObject
* Description: 解析JSON对象
* Input      : 
    * c, Json内容
    * v, Json节点
* Output     :
* Return     : 
    * PARSE_OK, 解析成功
    * PARSE_MISS_KEY, 没找到键
    * PARSE_MISS_COLON, 没找到`:`
    * PARSE_MISS_COMMA_OR_CURLY_BRACKET, 没解析到`,`或`}`
* Others     : 

member = string ws %3A ws value
object = %x7B ws [ member *(ws %x2C ws member ) ] ws %x7D
-----------------------------------------------------------------------------*/
static int ParseObject(CJSONContext *c, CJSONValue *v)
{
    size_t i;
    size_t size = 0;
    CJSONMember m;
    int ret;

    EXPECT(c, '{');
    ParseWhiteSpace(c);
    if(*c->json == '}'){
        c->json++;
        v->type = TYPE_OBJECT;
        v->u.o.m = NULL;
        v->u.o.size = 0;
        return PARSE_OK;
    }
    m.k = NULL;
    for(;;){
        char *str;
        INIT_VALUE_NULL(&m.v);
        //解析键
        if(*c->json != '"'){
            ret = PARSE_MISS_KEY;
            break;
        }
        if((ret = ParseStringRaw(c, &str, &m.klen)) != PARSE_OK)
            break;
        memcpy(m.k = (char *)malloc(m.klen + 1), str, m.klen);
        m.k[m.klen] = '\0';
        //键和`:`之间可能有空格
        ParseWhiteSpace(c);
        if(*c->json != ':'){
            ret = PARSE_MISS_COLON;
            break;
        }
        c->json++;
        //`:`和值之间可能有空格
        ParseWhiteSpace(c);
        //解析值
        if((ret = ParseValue(c, &m.v)) != PARSE_OK)
            break;
        memcpy(ContextPush(c, sizeof(CJSONMember)), &m, sizeof(CJSONMember));
        size++;
        m.k = NULL;
        //对象的第一个元素和第二个元素之间可能有空格
        ParseWhiteSpace(c);
        if(*c->json == ','){
            c->json++;
            ParseWhiteSpace(c);
        }
        else if(*c->json == '}'){
            //解析到'}'说明解析完成，需要统计一共解析出来多少个元素
            size_t s = sizeof(CJSONMember) * size;
            c->json++;
            v->type = TYPE_OBJECT;
            v->u.o.size = size;
            //将暂存在栈上的对象的元素拷贝给对象
            memcpy(v->u.o.m = (CJSONMember *)malloc(s), ContextPop(c, s), s);
            return PARSE_OK;
        }
        else{
            ret = PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }

    //弹出并释放栈上的内存
    free(m.k);
    for(i = 0; i < size; i++){
        CJSONMember *m = (CJSONMember *)ContextPop(c, sizeof(CJSONMember));
        free(m->k);
        FreeValue(&m->v);
    }
    v->type = TYPE_NULL;
    return ret;
}

/*-----------------------------------------------------------------------------
* Function   : ParseValue
* Description: 判断下一个字符是n、t、f、0-9/-、"、[、{，选择具体调用哪个解析方法
* Input      :
    * c, Json内容
* Output     :
* Return     : 
    * PARSE_OK, 解析成功
    * PARSE_INVALID_VALUE, 解析得到非法值
    * PARSE_EXPECT_VALUE, 解析到结尾了，不能继续解析
* Others     : 
-----------------------------------------------------------------------------*/
static int ParseValue(CJSONContext *c, CJSONValue *v)
{
    //这里return直接跳出，所以不再需要用break！
    switch(*c->json){
        case 'n'  : return ParseLiteral(c, v, "null", TYPE_NULL);
        case 't'  : return ParseLiteral(c, v, "true", TYPE_TRUE);
        case 'f'  : return ParseLiteral(c, v, "false", TYPE_FALSE);
        case '0'  :
        case '1'  :
        case '2'  :
        case '3'  :
        case '4'  :
        case '5'  :
        case '6'  :
        case '7'  :
        case '8'  :
        case '9'  :
        case '-'  : return ParseNumber(c, v);
        case '\"' : return ParseString(c, v);
        case '['  : return ParseArray(c, v);
        case '{'  : return ParseObject(c, v);
        case '\0' : return PARSE_EXPECT_VALUE;
        default   : return PARSE_INVALID_VALUE;
    }
}

/*-----------------------------------------------------------------------------
* Function   : StringifyValue
* Description: 递归从根节点开始广度搜索遍历树的每个节点，生成JSON字符串
* Input      :
    * v, JSON结构根节点
* Output     :
    * c, 生成的JSON字符串
* Return     : 
    * STRINGIFY_OK, 生成成功
* Others     : 
-----------------------------------------------------------------------------*/
static int StringifyValue(CJSONContext *c, const CJSONValue *v)
{
    size_t i;
    switch(v->type){
        case TYPE_NULL : PUTS(c, "null", 4); break;
        case TYPE_FALSE : PUTS(c, "false", 5); break;
        case TYPE_TRUE : PUTS(c, "true", 4); break;
        case TYPE_NUMBER : 
            {
                char *buffer = ContextPush(c, 32);
                int length = sprintf(buffer, "%.17g", v->u.n);
                c->top -= (32-length);
                break;
            }
        case TYPE_STRING : 
            {
                PUTC(c, '"');
                PUTS(c, v->u.s.s, v->u.s.len);
                PUTC(c, '"');
                break;
            }
        case TYPE_ARRAY : 
            {
                PUTC(c, '[');
                for(i = 0; i < v->u.a.size; i++){
                    StringifyValue(c, &v->u.a.e[i]);
                    if(i != v->u.a.size - 1)
                        PUTC(c, ',');
                }
                PUTC(c, ']');
                break;
            }
        case TYPE_OBJECT :
            {
                PUTC(c, '{');
                for(i = 0; i < v->u.o.size; i++){
                    //键
                    PUTC(c, '"');
                    PUTS(c, v->u.o.m[i].k, v->u.o.m[i].klen);
                    PUTC(c, '"');
                    //`:`
                    PUTC(c, ':');
                    //值
                    StringifyValue(c, &v->u.o.m[i].v);
                    //最后一个元素没有`,`
                    if(i != v->u.o.size - 1)
                        PUTC(c, ',');
                }
                PUTC(c, '}');
                break;
            }
    }
    return STRINGIFY_OK;
}

/*-----------------------------------------------------------------------------
* Function   : ContextPush
* Description: 压入时，若空间不足，便回以1.5倍大小扩展
    * 为什么是1.5倍而不是2倍，参考：https://www.zhihu.com/question/25079705/answer/30030883
    * 每次可要要求压入任意大小的数据，它会返回数据起始的指针
* Input      :
    * c, Json内容
    * size, 
* Output     :
* Return     : 
* Others     : 栈的结构图如下
+==================+
||    stack   |   ||   <---- stack
||    ...     v   ||
||                ||
||                ||
||                ||
||                ||
||正在被占用的内存||
||================||   <---- top
||                ||
||    多余内存    ||
||                ||
+==================+
-----------------------------------------------------------------------------*/
static void *ContextPush(CJSONContext *c, size_t size)
{
    void *ret;
    assert(size > 0);
    if(c->top + size >= c->size){
        //确定栈合适的大小
        if(c->size == 0)
            c->size = STACK_INIT_SIZE;
        while(c->top + size >= c->size)
            c->size += c->size >> 1;        //c->size * 1.5
        //在堆上扩容
        c->stack = (char *)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

/*-----------------------------------------------------------------------------
* Function   : ContextPop
* Description: 从栈上将内容弹出来
* Input      :
    * c, Json内容
    * size, 弹出内容的字节数
* Output     : 
* Return     : 
* Others     : 栈的结构图如下
+==================+
||    stack   |   ||   <---- stack
||    ...     v   ||
||正在被占用的内存||
||================||   <---- top
||                ||   ^上移size字节
|| 移动的size字节 ||   |
|| 被释放         ||
||----------------||   
||                ||
||    多余内存    ||
||                ||
+==================+
-----------------------------------------------------------------------------*/
static void *ContextPop(CJSONContext *c, size_t size)
{
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}
