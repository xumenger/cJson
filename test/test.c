/*********************************************************************************
 * Copyright(C), xumenger
 * FileName     : test.h
 * Author       : xumenger
 * Version      : V1.0.0 
 * Date         : 2017-08-09
 * Description  : 
     1.对cJson的单元测试和测试用例
**********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/cJsonStruct.h"
#include "../src/cJson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

/*宏的编写技巧参考：https://zhuanlan.zhihu.com/p/22460835*/
#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_EQ_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_EQ_FALSE(actual) EXPECT_EQ_BASE((actual) != 0, "false", "true", "%s")

static void test_parse_null(){
    CJSONValue v;
    v.type = TYPE_FALSE;
    EXPECT_EQ_INT(PARSE_OK, Parse(&v, "null"));
    EXPECT_EQ_INT(TYPE_NULL, GetType(&v));
}

static void test_parse_true(){
    CJSONValue v;
    v.type = TYPE_FALSE;
    EXPECT_EQ_INT(PARSE_OK, Parse(&v, "true"));
    EXPECT_EQ_INT(TYPE_TRUE, GetType(&v));
}

static void test_parse_false(){
    CJSONValue v;
    v.type = TYPE_NULL;
    EXPECT_EQ_INT(PARSE_OK, Parse(&v, "false"));
    EXPECT_EQ_INT(TYPE_FALSE, GetType(&v));
}

#define TEST_NUMBER(expect, json)\
    do {\
        CJSONValue v;\
        EXPECT_EQ_INT(PARSE_OK, Parse(&v, json));\
        EXPECT_EQ_INT(TYPE_NUMBER, GetType(&v));\
        EXPECT_EQ_DOUBLE(expect, GetNumber(&v));\
    } while(0)

static void test_parse_number(){
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1e+10, "-1e+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000");    //must underflow
}

#define TEST_STRING(expect, json)\
    do {\
        CJSONValue v;\
        INIT_VALUE_NULL(&v);\
        EXPECT_EQ_INT(PARSE_OK, Parse(&v, json));\
        EXPECT_EQ_INT(TYPE_STRING, GetType(&v));\
        EXPECT_EQ_STRING(expect, GetString(&v), GetStringLength(&v));\
        FreeValue(&v);\
    }while(0)

static void test_parse_string(){
    TEST_STRING("",  "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
#if 0
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
}

//ANSI C(C 89)并没有size_t打印方法
//上面的代码使用条件编译去区分 VC 和其他编译器
#if defined(_MSC_VER)
//VS2015之前的VC版本中使用非标准的"%Iu"
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect)==(actual), (size_t)expect, (size_t)actual, "%Iu")
#else
//C99中加入了"%zu"
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect)==(actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_array(){
    size_t i, j;
    CJSONValue v;
    INIT_VALUE_NULL(&v);

    EXPECT_EQ_INT(PARSE_OK, Parse(&v, "[null, false, true, 123, \"abc\"]"));
    EXPECT_EQ_INT(TYPE_ARRAY, GetType(&v));
    EXPECT_EQ_SIZE_T(5, GetArraySize(&v));

    EXPECT_EQ_INT(TYPE_NULL, GetType(GetArrayElement(&v, 0)));
    EXPECT_EQ_INT(TYPE_FALSE, GetType(GetArrayElement(&v, 1)));
    EXPECT_EQ_INT(TYPE_TRUE, GetType(GetArrayElement(&v, 2)));
    EXPECT_EQ_INT(TYPE_NUMBER, GetType(GetArrayElement(&v, 3)));
    EXPECT_EQ_INT(TYPE_STRING, GetType(GetArrayElement(&v, 4)));

    EXPECT_EQ_DOUBLE(123.0, GetNumber(GetArrayElement(&v, 3)));
    EXPECT_EQ_STRING("abc", GetString(GetArrayElement(&v, 4)), GetStringLength(GetArrayElement(&v, 4)));

    FreeValue(&v);

    INIT_VALUE_NULL(&v);
    EXPECT_EQ_INT(PARSE_OK, Parse(&v, "[ [ ], [ 0 ], [ 0, 1 ], [ 0, 1, 2 ] ]"));
    EXPECT_EQ_SIZE_T(4, GetArraySize(&v));
    for(i=0; i<4; i++){
        CJSONValue *a = GetArrayElement(&v, i);
        EXPECT_EQ_INT(TYPE_ARRAY, GetType(a));
        EXPECT_EQ_SIZE_T(i, GetArraySize(a));
        for(j=0; j<i; j++){
            CJSONValue *e = GetArrayElement(a, j);
            EXPECT_EQ_INT(TYPE_NUMBER, GetType(e));
            EXPECT_EQ_DOUBLE((double)j, GetNumber(e));
        }
    }
    FreeValue(&v);
}

static void test_parse_object(){
    CJSONValue v;
    size_t i;

    INIT_VALUE_NULL(&v);
    EXPECT_EQ_INT(PARSE_OK, Parse(&v, " { } "));
    EXPECT_EQ_INT(TYPE_OBJECT, GetType(&v));
    EXPECT_EQ_SIZE_T(0, GetObjectSize(&v));
    FreeValue(&v);

    INIT_VALUE_NULL(&v);
    EXPECT_EQ_INT(PARSE_OK, Parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\" , "
        "\"a\" : [1, 2, 3 ] , "
        "\"o\" : {\"1\" : 1, \"2\" : 2, \"3\" : 3} "
        " } "
    ));
    EXPECT_EQ_INT(TYPE_OBJECT, GetType(&v));
    EXPECT_EQ_SIZE_T(7, GetObjectSize(&v));

    EXPECT_EQ_STRING("n", GetObjectKey(&v, 0), GetObjectKeyLength(&v, 0));
    EXPECT_EQ_INT(TYPE_NULL, GetType(GetObjectValue(&v, 0)));
    
    EXPECT_EQ_STRING("f", GetObjectKey(&v, 1), GetObjectKeyLength(&v, 1));
    EXPECT_EQ_INT(TYPE_FALSE, GetType(GetObjectValue(&v, 1)));
    
    EXPECT_EQ_STRING("t", GetObjectKey(&v, 2), GetObjectKeyLength(&v, 2));
    EXPECT_EQ_INT(TYPE_TRUE, GetType(GetObjectValue(&v, 2)));
    
    EXPECT_EQ_STRING("i", GetObjectKey(&v, 3), GetObjectKeyLength(&v, 3));
    EXPECT_EQ_INT(TYPE_NUMBER, GetType(GetObjectValue(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, GetNumber(GetObjectValue(&v, 3)));
    
    EXPECT_EQ_STRING("s", GetObjectKey(&v, 4), GetObjectKeyLength(&v, 4));
    EXPECT_EQ_INT(TYPE_STRING, GetType(GetObjectValue(&v, 4)));
    EXPECT_EQ_STRING("abc", GetString(GetObjectValue(&v, 4)), GetStringLength(GetObjectValue(&v, 4)));

    EXPECT_EQ_STRING("a", GetObjectKey(&v, 5), GetObjectKeyLength(&v, 5));
    EXPECT_EQ_INT(TYPE_ARRAY, GetType(GetObjectValue(&v, 5)));
    EXPECT_EQ_SIZE_T(3, GetArraySize(GetObjectValue(&v, 5)));
    for(i = 0; i < 3; i++){
        CJSONValue *e = GetArrayElement(GetObjectValue(&v, 5), i);
        EXPECT_EQ_INT(TYPE_NUMBER, GetType(e));
        EXPECT_EQ_DOUBLE(i + 1.0, GetNumber(e));
    }

    EXPECT_EQ_STRING("o", GetObjectKey(&v, 6), GetObjectKeyLength(&v, 6));
    {
        CJSONValue *o = GetObjectValue(&v, 6);
        EXPECT_EQ_INT(TYPE_OBJECT, GetType(o));
        for(i = 0; i < 3; i++){
            CJSONValue *ov = GetObjectValue(o, i);
            EXPECT_EQ_TRUE('1' + i == GetObjectKey(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, GetObjectKeyLength(o, i));
            EXPECT_EQ_INT(TYPE_NUMBER, GetType(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, GetNumber(ov));
        }
    }
    FreeValue(&v);
}

#define TEST_ERROR(error, json)\
    do {\
        CJSONValue v;\
        v.type = TYPE_FALSE;\
        EXPECT_EQ_INT(error, Parse(&v, json));\
        EXPECT_EQ_INT(TYPE_NULL, GetType(&v));\
    } while(0)

static void test_parse_expect_value(){
    TEST_ERROR(PARSE_EXPECT_VALUE, "");
    TEST_ERROR(PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value(){
    /*invalid null*/
    TEST_ERROR(PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(PARSE_INVALID_VALUE, "?");

    /*invalid true*/
    TEST_ERROR(PARSE_INVALID_VALUE, "tru");
    TEST_ERROR(PARSE_INVALID_VALUE, "tur");
    
    /*invalid false*/
    TEST_ERROR(PARSE_INVALID_VALUE, "fals");
    TEST_ERROR(PARSE_INVALID_VALUE, "ff");

    /*invalid number*/
    TEST_ERROR(PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(PARSE_INVALID_VALUE, ".123");
    TEST_ERROR(PARSE_INVALID_VALUE, "1.");
    TEST_ERROR(PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular(){
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "null x");

    #if 0
    /* invalid number 暂不支持解析*/
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x123");
    #endif
}

static void test_parse_number_to_big(){
    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark(){
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
#if 0
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char(){
#if 0
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\x01\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\x1F\"");
#endif
}

static void test_parse_miss_comma_or_square_bracket(){
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key(){
    TEST_ERROR(PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{true:1,");
}

static void test_parse_miss_colon(){
    TEST_ERROR(PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket(){
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1】");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_access_null(){
    CJSONValue v;
    INIT_VALUE_NULL(&v);
    SetString(&v, "a", 1);
    SET_VALUE_NULL(&v);
    EXPECT_EQ_INT(TYPE_NULL, GetType(&v));
    FreeValue(&v);
}

static void test_access_boolean(){
    CJSONValue v;
    INIT_VALUE_NULL(&v);
    SetString(&v, "b", 1);
    SetBoolean(&v, TYPE_FALSE);
    EXPECT_EQ_INT(TYPE_FALSE, GetType(&v));
    FreeValue(&v);
}

static void test_access_number(){
    CJSONValue v;
    INIT_VALUE_NULL(&v);
    SetString(&v, "c", 1);
    SetNumber(&v, 100);
    EXPECT_EQ_INT(TYPE_NUMBER, GetType(&v));
    FreeValue(&v);
}

static void test_access_string(){
    CJSONValue v;
    INIT_VALUE_NULL(&v);
    SetString(&v, "", 0);
    EXPECT_EQ_STRING("", GetString(&v), GetStringLength(&v));
    SetString(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", GetString(&v), GetStringLength(&v));
    FreeValue(&v);
}

#define TEST_ROUNDTRIP(json)\
    do {\
        CJSONValue v;\
        char *json2;\
        size_t length;\
        INIT_VALUE_NULL(&v);\
        EXPECT_EQ_INT(PARSE_OK, Parse(&v, json));\
        EXPECT_EQ_INT(STRINGIFY_OK, Stringify(&v, &json2, &length));\
        EXPECT_EQ_STRING(json, json2, length);\
        FreeValue(&v);\
        free(json2);\
    } while(0)

static void test_stringify(){
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");

    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("12345");
    TEST_ROUNDTRIP("-1.234");
    TEST_ROUNDTRIP("1.23e-20");
    TEST_ROUNDTRIP("1.23e+20");
    
    TEST_ROUNDTRIP("\"abcdef\"");
    // TEST_ROUNDTRIP("\"\"");
    // TEST_ROUNDTRIP("\"Hello\nWorld\"");

    TEST_ROUNDTRIP("[123,234,[1,2]]");

    TEST_ROUNDTRIP("{\"employees\":[{\"firstName\":\"Bill\",\"lastName\":\"Gates\"},{\"firstName\":\"George\",\"lastName\":\"Bush\"},{\"firstName\":\"Thomas\",\"lastName\":\"Carter\"}]}");
}

static void test_parse(){
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();

    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_to_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();

    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main(){
    test_parse();
    test_stringify();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
