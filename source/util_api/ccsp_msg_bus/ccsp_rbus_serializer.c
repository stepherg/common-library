/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Local reimplementation of rbus internal serialization functions
 * using only public rbus APIs.
 *
 * Wire format is identical to rbus internals (rbus.c
 * rbusValue_appendToMessage / rbusValue_initFromMessage) so existing
 * consumers see no behavioural change.
 *
 * Type-specific encoding:
 *   INT16..UINT32  -> SetInt32 / GetInt32
 *   INT64..UINT64  -> SetInt64 / GetInt64
 *   SINGLE..DOUBLE -> SetDouble / GetDouble
 *   everything else (BOOLEAN,CHAR,BYTE,STRING,BYTES,DATETIME,...) ->
 *       SetBytes(GetV,GetL) / GetBytes+SetTLV
 */

#include "ccsp_rbus_serializer.h"
#include <stdint.h>
#include <string.h>

/*
 * rbus_buffer.h declares rbusValue_GetV / rbusValue_GetL / rbusValue_SetTLV.
 * These are public functions exported from librbus but the header may not
 * be installed in every build sysroot.  Provide fallback declarations.
 */
#if defined(__has_include) && __has_include(<rbus/rbus_buffer.h>)
#include <rbus/rbus_buffer.h>
#else
extern uint8_t const* rbusValue_GetV(rbusValue_t v);
extern uint32_t       rbusValue_GetL(rbusValue_t v);
extern void           rbusValue_SetTLV(rbusValue_t v, rbusValueType_t type,
                                       uint32_t length, void const* value);
#endif

/* ------------------------------------------------------------------ */
/*  rbusValue  ->  rbusMessage                                        */
/*  Caller is responsible for writing the name string first.          */
/*  This writes: type(int32) + type-specific payload.                 */
/* ------------------------------------------------------------------ */

void ccsp_rbusValue_appendToMessage(rbusValue_t value, rbusMessage msg)
{
    rbusValueType_t type = rbusValue_GetType(value);

    rbusMessage_SetInt32(msg, (int32_t)type);

    if (type == RBUS_OBJECT)
    {
        ccsp_rbusObject_appendToMessage(rbusValue_GetObject(value), msg);
    }
    else if (type == RBUS_PROPERTY)
    {
        ccsp_rbusPropertyList_appendToMessage(rbusValue_GetProperty(value), msg);
    }
    else
    {
        switch (type)
        {
        case RBUS_INT16:
            rbusMessage_SetInt32(msg, (int32_t)rbusValue_GetInt16(value));
            break;
        case RBUS_UINT16:
            rbusMessage_SetInt32(msg, (int32_t)rbusValue_GetUInt16(value));
            break;
        case RBUS_INT32:
            rbusMessage_SetInt32(msg, rbusValue_GetInt32(value));
            break;
        case RBUS_UINT32:
            rbusMessage_SetInt32(msg, (int32_t)rbusValue_GetUInt32(value));
            break;
        case RBUS_INT64:
            rbusMessage_SetInt64(msg, rbusValue_GetInt64(value));
            break;
        case RBUS_UINT64:
            rbusMessage_SetInt64(msg, (int64_t)rbusValue_GetUInt64(value));
            break;
        case RBUS_SINGLE:
            rbusMessage_SetDouble(msg, (double)rbusValue_GetSingle(value));
            break;
        case RBUS_DOUBLE:
            rbusMessage_SetDouble(msg, rbusValue_GetDouble(value));
            break;
        default:
        {
            /* BOOLEAN, CHAR, BYTE, INT8, UINT8, STRING, BYTES, DATETIME, … */
            uint8_t const* buff = rbusValue_GetV(value);
            uint32_t len = rbusValue_GetL(value);
            rbusMessage_SetBytes(msg, buff ? buff : (const uint8_t*)"", len);
            break;
        }
        }
    }
}

/* ------------------------------------------------------------------ */
/*  rbusValue  <-  rbusMessage                                        */
/*  Reads: type(int32) + type-specific payload.                       */
/*  Name has already been consumed by the caller.                     */
/* ------------------------------------------------------------------ */

void ccsp_rbusValue_initFromMessage(rbusValue_t* value, rbusMessage msg)
{
    int32_t type = 0;

    rbusMessage_GetInt32(msg, &type);
    rbusValue_Init(value);

    if (type == RBUS_OBJECT)
    {
        rbusObject_t obj = NULL;
        ccsp_rbusObject_initFromMessage(&obj, msg);
        rbusValue_SetObject(*value, obj);
        rbusObject_Release(obj);
    }
    else if (type == RBUS_PROPERTY)
    {
        rbusProperty_t prop = NULL;
        ccsp_rbusPropertyList_initFromMessage(&prop, msg);
        rbusValue_SetProperty(*value, prop);
        rbusProperty_Release(prop);
    }
    else
    {
        int32_t ival;
        int64_t i64;
        double fval;
        uint8_t const* data = NULL;
        uint32_t length = 0;

        switch (type)
        {
        case RBUS_INT16:
            rbusMessage_GetInt32(msg, &ival);
            rbusValue_SetInt16(*value, (int16_t)ival);
            break;
        case RBUS_UINT16:
            rbusMessage_GetInt32(msg, &ival);
            rbusValue_SetUInt16(*value, (uint16_t)ival);
            break;
        case RBUS_INT32:
            rbusMessage_GetInt32(msg, &ival);
            rbusValue_SetInt32(*value, (int32_t)ival);
            break;
        case RBUS_UINT32:
            rbusMessage_GetInt32(msg, &ival);
            rbusValue_SetUInt32(*value, (uint32_t)ival);
            break;
        case RBUS_INT64:
            rbusMessage_GetInt64(msg, &i64);
            rbusValue_SetInt64(*value, (int64_t)i64);
            break;
        case RBUS_UINT64:
            rbusMessage_GetInt64(msg, &i64);
            rbusValue_SetUInt64(*value, (uint64_t)i64);
            break;
        case RBUS_SINGLE:
            rbusMessage_GetDouble(msg, &fval);
            rbusValue_SetSingle(*value, (float)fval);
            break;
        case RBUS_DOUBLE:
            rbusMessage_GetDouble(msg, &fval);
            rbusValue_SetDouble(*value, fval);
            break;
        default:
            /* BOOLEAN, CHAR, BYTE, INT8, UINT8, STRING, BYTES, DATETIME, … */
            rbusMessage_GetBytes(msg, &data, &length);
            rbusValue_SetTLV(*value, (rbusValueType_t)type, length, data);
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  rbusProperty list  <->  rbusMessage                               */
/* ------------------------------------------------------------------ */

void ccsp_rbusPropertyList_appendToMessage(rbusProperty_t prop, rbusMessage msg)
{
    int count = 0;
    rbusProperty_t p;

    for (p = prop; p != NULL; p = rbusProperty_GetNext(p))
        count++;

    rbusMessage_SetInt32(msg, count);

    for (p = prop; p != NULL; p = rbusProperty_GetNext(p))
    {
        /* name(string) written here, type+data written by value append */
        rbusMessage_SetString(msg, rbusProperty_GetName(p));
        ccsp_rbusValue_appendToMessage(rbusProperty_GetValue(p), msg);
    }
}

void ccsp_rbusPropertyList_initFromMessage(rbusProperty_t* prop, rbusMessage msg)
{
    int32_t count = 0;
    int i;
    rbusProperty_t first = NULL, prev = NULL;

    rbusMessage_GetInt32(msg, &count);

    for (i = 0; i < count; i++)
    {
        const char* name = NULL;
        rbusValue_t value = NULL;
        rbusProperty_t p = NULL;

        rbusMessage_GetString(msg, &name);
        ccsp_rbusValue_initFromMessage(&value, msg);
        rbusProperty_Init(&p, name, value);
        rbusValue_Release(value);

        if (prev)
        {
            rbusProperty_SetNext(prev, p);
            rbusProperty_Release(p);
        }
        else
        {
            first = p;
        }
        prev = p;
    }
    *prop = first;
}

/* ------------------------------------------------------------------ */
/*  rbusObject  <->  rbusMessage  (recursive)                         */
/* ------------------------------------------------------------------ */

void ccsp_rbusObject_appendToMessage(rbusObject_t obj, rbusMessage msg)
{
    rbusObject_t child;
    int childCount = 0;

    rbusMessage_SetString(msg, rbusObject_GetName(obj));
    rbusMessage_SetInt32(msg, (int32_t)rbusObject_GetType(obj));

    ccsp_rbusPropertyList_appendToMessage(rbusObject_GetProperties(obj), msg);

    for (child = rbusObject_GetChildren(obj); child != NULL;
         child = rbusObject_GetNext(child))
        childCount++;

    rbusMessage_SetInt32(msg, childCount);

    for (child = rbusObject_GetChildren(obj); child != NULL;
         child = rbusObject_GetNext(child))
        ccsp_rbusObject_appendToMessage(child, msg);
}

void ccsp_rbusObject_initFromMessage(rbusObject_t* obj, rbusMessage msg)
{
    const char* name = NULL;
    int32_t type = 0;
    int32_t childCount = 0;
    rbusProperty_t props = NULL;
    int i;

    rbusMessage_GetString(msg, &name);
    rbusMessage_GetInt32(msg, &type);

    if (type == RBUS_OBJECT_SINGLE_INSTANCE)
        rbusObject_Init(obj, name);
    else
        rbusObject_InitMultiInstance(obj, name);

    ccsp_rbusPropertyList_initFromMessage(&props, msg);
    rbusObject_SetProperties(*obj, props);
    if (props)
        rbusProperty_Release(props);

    rbusMessage_GetInt32(msg, &childCount);

    for (i = 0; i < childCount; i++)
    {
        rbusObject_t child = NULL;
        ccsp_rbusObject_initFromMessage(&child, msg);

        rbusObject_t last = rbusObject_GetChildren(*obj);
        if (last == NULL)
        {
            rbusObject_SetChildren(*obj, child);
        }
        else
        {
            while (rbusObject_GetNext(last) != NULL)
                last = rbusObject_GetNext(last);
            rbusObject_SetNext(last, child);
        }
        rbusObject_Release(child);
    }
}

/* ------------------------------------------------------------------ */
/*  rbusFilter  <->  rbusMessage  (recursive for logic nodes)         */
/* ------------------------------------------------------------------ */

void ccsp_rbusFilter_AppendToMessage(rbusFilter_t filter, rbusMessage msg)
{
    rbusMessage_SetInt32(msg, (int32_t)rbusFilter_GetType(filter));

    if (rbusFilter_GetType(filter) == RBUS_FILTER_EXPRESSION_RELATION)
    {
        rbusMessage_SetInt32(msg, (int32_t)rbusFilter_GetRelationOperator(filter));
        /* rbus writes value with name="filter": name(string) + type + data */
        rbusMessage_SetString(msg, "filter");
        ccsp_rbusValue_appendToMessage(rbusFilter_GetRelationValue(filter), msg);
    }
    else if (rbusFilter_GetType(filter) == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        rbusMessage_SetInt32(msg, (int32_t)rbusFilter_GetLogicOperator(filter));
        ccsp_rbusFilter_AppendToMessage(rbusFilter_GetLogicLeft(filter), msg);
        if (rbusFilter_GetLogicOperator(filter) != RBUS_FILTER_OPERATOR_NOT)
            ccsp_rbusFilter_AppendToMessage(rbusFilter_GetLogicRight(filter), msg);
    }
}

void ccsp_rbusFilter_InitFromMessage(rbusFilter_t* filter, rbusMessage msg)
{
    int32_t type = 0;
    int32_t op = 0;

    rbusMessage_GetInt32(msg, &type);

    if (type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        const char* name = NULL;
        rbusValue_t val = NULL;

        rbusMessage_GetInt32(msg, &op);
        rbusMessage_GetString(msg, &name); /* consume "filter" name */
        ccsp_rbusValue_initFromMessage(&val, msg);
        rbusFilter_InitRelation(filter, (rbusFilter_RelationOperator_t)op, val);
        rbusValue_Release(val);
    }
    else if (type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        rbusFilter_t left = NULL, right = NULL;

        rbusMessage_GetInt32(msg, &op);
        ccsp_rbusFilter_InitFromMessage(&left, msg);
        if (op != RBUS_FILTER_OPERATOR_NOT)
            ccsp_rbusFilter_InitFromMessage(&right, msg);
        rbusFilter_InitLogic(filter, (rbusFilter_LogicOperator_t)op, left, right);
        rbusFilter_Release(left);
        if (right)
            rbusFilter_Release(right);
    }
    else
    {
        *filter = NULL;
    }
}

/* ------------------------------------------------------------------ */
/*  rbusEventData  ->  rbusMessage                                    */
/* ------------------------------------------------------------------ */

void ccsp_rbusEventData_appendToMessage(
    rbusEvent_t* event,
    rbusFilter_t filter,
    uint32_t interval,
    uint32_t duration,
    int32_t componentId,
    rbusMessage msg)
{
    rbusMessage_SetString(msg, event->name);
    rbusMessage_SetInt32(msg, (int32_t)event->type);

    if (event->data)
    {
        rbusMessage_SetInt32(msg, 1);
        ccsp_rbusObject_appendToMessage(event->data, msg);
    }
    else
    {
        rbusMessage_SetInt32(msg, 0);
    }

    if (filter)
    {
        rbusMessage_SetInt32(msg, 1);
        ccsp_rbusFilter_AppendToMessage(filter, msg);
    }
    else
    {
        rbusMessage_SetInt32(msg, 0);
    }

    rbusMessage_SetInt32(msg, (int32_t)interval);
    rbusMessage_SetInt32(msg, (int32_t)duration);
    rbusMessage_SetInt32(msg, componentId);
}
