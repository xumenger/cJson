/*********************************************************************************
 * Copyright(C), xumenger
 * FileName     : cJson.h
 * Author       : xumenger
 * Version      : V1.0.0 
 * Date         : 2017-08-09
 * Description  : 
     1.定义cJson的接口
**********************************************************************************/
#ifndef CJSON_H
#define CJSON_H

#include "cJsonStruct.h"

//因为需要检查JSON节点的类型，所以需要在创建时对其初始化
#define INIT_VALUE_NULL(v)   do { (v)->type = TYPE_NULL; } while(0)
#define SET_VALUE_NULL(v)    FreeValue(v)

int Parse(CJSONValue *v, const char *json);
int Stringify(const CJSONValue *v, char **json, size_t *length);
CJSONType GetType(const CJSONValue *v);
int GetBoolean(const CJSONValue *v);
void SetBoolean(CJSONValue *v, int b);
double GetNumber(const CJSONValue *v);
void SetNumber(CJSONValue *v, double n);
const char *GetString(const CJSONValue *v);
size_t GetStringLength(const CJSONValue *v);
void SetString(CJSONValue *v, const char *s, size_t len);
size_t GetArraySize(const CJSONValue *v);
CJSONValue *GetArrayElement(const CJSONValue *v, size_t index);
size_t GetObjectSize(const CJSONValue *v);
const char *GetObjectKey(const CJSONValue *v, size_t index);
size_t GetObjectKeyLength(const CJSONValue *v, size_t index);
CJSONValue *GetObjectValue(const CJSONValue *v, size_t index);
void FreeValue(CJSONValue *v);

#endif
