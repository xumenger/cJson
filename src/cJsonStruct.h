/*********************************************************************************
 * Copyright(C), xumenger
 * FileName     : cJsonStruct.h
 * Author       : xumenger
 * Version      : V1.0.0 
 * Date         : 2017-08-09
 * Description  : 
     1.定义相关的结构体、枚举值
**********************************************************************************/
#ifndef CJSONSTRUCT_H
#define CJSONSTRUCT_H

//JSON中有6中数据类型，如果把true和false当做两个类型，那么就有7种
typedef enum{
    TYPE_NULL,
    TYPE_FALSE,
    TYPE_TRUE,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_OBJECT
}CJSONType;

typedef struct CJSONValue CJSONValue;
typedef struct CJSONMember CJSONMember;

//JSON的数据结构
struct CJSONValue{
    CJSONType type;
    //一个JSON节点不可能同时为数字和字符串，可以使用union来节省内存
    union{
        //object
        struct { CJSONMember *m; size_t size; } o;
        //array: 第一个元素的指针, 元素的个数
        struct {CJSONValue *e; size_t  size;} a;
        //string: 字符串指针, 字符串长度
    	struct{ char *s; size_t len;} s;
        //number
    	double n;
    }u;
};

struct CJSONMember{
    char *k;       //键
    size_t klen;   //键长度
    CJSONValue v;  //值
};

typedef struct{
    const char *json;
    /*
    解析字符串时，需要把解析的结果先存储在一个临时缓冲区
    最后再用SetString把缓冲区的结果设置进值之中
    如果每次解析字符串时都重新创建一个动态数组，那么是比较耗时的
    所以可以重用这个动态数组，根据需要动态扩展即可
    数组解析的时候也用到这个栈数据结构！
    */
    char *stack;
    size_t size;              //当前堆栈容量
    size_t top;               //栈顶位置，因为会扩展stack 
}CJSONContext;

//Parse函数的返回值枚举
enum {
    //解析器相关
    PARSE_OK = 0,
    PARSE_EXPECT_VALUE,                 //若一个JSON只含有空白，返回
    PARSE_INVALID_VALUE,                //若一个值之后，在空白之后还有其他字符，返回
    PARSE_ROOT_NOT_SINGULAR,            //若值不是JSON合法的值，返回
    PARSE_NUMBER_TOO_BIG,               //过大的数值
    PARSE_MISS_QUOTATION_MARK,          //没有遇到结束标记
    PARSE_INVALID_STRING_ESCAPE,        //
    PARSE_INVALID_STRING_CHAR,          //
    PARSE_MISS_COMMA_OR_SQUARE_BRACKET, //
    PARSE_MISS_KEY,
    PARSE_MISS_COLON,
    PARSE_MISS_COMMA_OR_CURLY_BRACKET,

    //生成器相关
    STRINGIFY_OK
};

#endif
