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
 * using only public rbus APIs (rbus_value.h, rbus_property.h,
 * rbus_object.h, rbus_filter.h, rbus_buffer.h, rtMessage.h).
 *
 * Wire format is identical to rbus internals so existing consumers
 * see no behavioural change.
 */

#include "ccsp_rbus_serializer.h"
#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  rbusValue  <->  rbusMessage                                       */
/* ------------------------------------------------------------------ */

void ccsp_rbusValue_appendToMessage(rbusValue_t value, rbusMessage msg)
{
    rbusValueType_t type = rbusValue_GetType(value);
    int len = rbusValue_GetL(value);
    const void* data = rbusValue_GetV(value);

    rbusMessage_SetInt32(msg, (int32_t)type);
    rbusMessage_SetInt32(msg, len);
    if (len > 0 && data)
        rbusMessage_SetBytes(msg, (const uint8_t*)data, len);
}

void ccsp_rbusValue_initFromMessage(rbusValue_t* value, rbusMessage msg)
{
    int32_t type = 0;
    int32_t len = 0;
    const uint8_t* data = NULL;
    uint32_t dataLen = 0;

    rbusMessage_GetInt32(msg, &type);
    rbusMessage_GetInt32(msg, &len);
    if (len > 0)
        rbusMessage_GetBytes(msg, &data, &dataLen);

    rbusValue_Init(value);
    rbusValue_SetTLV(*value, type, len, (len > 0 && data) ? data : NULL);
}

/* ------------------------------------------------------------------ */
/*  rbusProperty list  ->  rbusMessage                                */
/* ------------------------------------------------------------------ */

void ccsp_rbusPropertyList_appendToMessage(rbusProperty_t prop, rbusMessage msg)
{
    int count = 0;
    rbusProperty_t p;

    /* count properties */
    for (p = prop; p != NULL; p = rbusProperty_GetNext(p))
        count++;

    rbusMessage_SetInt32(msg, count);

    for (p = prop; p != NULL; p = rbusProperty_GetNext(p))
    {
        rbusMessage_SetString(msg, rbusProperty_GetName(p));
        ccsp_rbusValue_appendToMessage(rbusProperty_GetValue(p), msg);
    }
}

/* ------------------------------------------------------------------ */
/*  rbusObject  ->  rbusMessage  (recursive)                          */
/* ------------------------------------------------------------------ */

void ccsp_rbusObject_appendToMessage(rbusObject_t obj, rbusMessage msg)
{
    rbusObject_t child;
    int childCount = 0;

    /* object name + type */
    rbusMessage_SetString(msg, rbusObject_GetName(obj));
    rbusMessage_SetInt32(msg, (int32_t)rbusObject_GetType(obj));

    /* properties */
    ccsp_rbusPropertyList_appendToMessage(rbusObject_GetProperties(obj), msg);

    /* children (count, then recurse) */
    for (child = rbusObject_GetChildren(obj); child != NULL;
         child = rbusObject_GetNext(child))
        childCount++;

    rbusMessage_SetInt32(msg, childCount);

    for (child = rbusObject_GetChildren(obj); child != NULL;
         child = rbusObject_GetNext(child))
        ccsp_rbusObject_appendToMessage(child, msg);
}

/* ------------------------------------------------------------------ */
/*  rbusFilter  <->  rbusMessage  (recursive for logic nodes)         */
/* ------------------------------------------------------------------ */

void ccsp_rbusFilter_AppendToMessage(rbusFilter_t filter, rbusMessage msg)
{
    rbusFilterType_t type = rbusFilter_GetType(filter);
    rbusMessage_SetInt32(msg, (int32_t)type);

    if (type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        rbusMessage_SetInt32(msg, (int32_t)rbusFilter_GetRelationOperator(filter));
        ccsp_rbusValue_appendToMessage(rbusFilter_GetRelationValue(filter), msg);
    }
    else if (type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        rbusMessage_SetInt32(msg, (int32_t)rbusFilter_GetLogicOperator(filter));
        ccsp_rbusFilter_AppendToMessage(rbusFilter_GetLogicLeft(filter), msg);
        ccsp_rbusFilter_AppendToMessage(rbusFilter_GetLogicRight(filter), msg);
    }
}

void ccsp_rbusFilter_InitFromMessage(rbusFilter_t* filter, rbusMessage msg)
{
    int32_t type = 0;
    rbusMessage_GetInt32(msg, &type);

    if (type == RBUS_FILTER_EXPRESSION_RELATION)
    {
        int32_t op = 0;
        rbusValue_t value = NULL;

        rbusMessage_GetInt32(msg, &op);
        ccsp_rbusValue_initFromMessage(&value, msg);
        rbusFilter_InitRelation(filter, (rbusFilterRelationOperator_t)op, value);
        rbusValue_Release(value);
    }
    else if (type == RBUS_FILTER_EXPRESSION_LOGIC)
    {
        int32_t op = 0;
        rbusFilter_t left = NULL, right = NULL;

        rbusMessage_GetInt32(msg, &op);
        ccsp_rbusFilter_InitFromMessage(&left, msg);
        ccsp_rbusFilter_InitFromMessage(&right, msg);
        rbusFilter_InitLogic(filter, (rbusFilterLogicOperator_t)op, left, right);
        rbusFilter_Release(left);
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
    /* event name + type */
    rbusMessage_SetString(msg, event->name);
    rbusMessage_SetInt32(msg, (int32_t)event->type);

    /* event data (rbusObject) */
    if (event->data)
    {
        rbusMessage_SetInt32(msg, 1); /* hasEventData */
        ccsp_rbusObject_appendToMessage(event->data, msg);
    }
    else
    {
        rbusMessage_SetInt32(msg, 0);
    }

    /* filter */
    if (filter)
    {
        rbusMessage_SetInt32(msg, 1);
        ccsp_rbusFilter_AppendToMessage(filter, msg);
    }
    else
    {
        rbusMessage_SetInt32(msg, 0);
    }

    /* subscription metadata */
    rbusMessage_SetInt32(msg, (int32_t)interval);
    rbusMessage_SetInt32(msg, (int32_t)duration);
    rbusMessage_SetInt32(msg, componentId);
}
