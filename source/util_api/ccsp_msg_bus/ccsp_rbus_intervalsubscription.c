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
#define _GNU_SOURCE 1 //needed for pthread_mutexattr_settype
#include "ccsp_rbus_value_change.h"
#include "rbus_message_bus.h"
#include "ccsp_base_api.h"
#include "ccsp_message_bus.h"
#include "ccsp_trace.h"
#include <rtmessage/rtVector.h>
#include <rtmessage/rtTime.h>
#include <rbus/rbus.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>

#define ERROR_CHECK(CMD) \
{ \
  int err; \
  if((err=CMD) != 0) \
  { \
    CcspTraceError("Error %d:%s running command \n" #CMD, err, strerror(err)); \
  } \
}

extern void get_recursive_wildcard_parameterNames(void* bus_handle, char *parameterName, rbusMessage *req, int *param_size);
static rtVector gRecord = NULL;
static pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct sRecord
{
    int running;
    pthread_mutex_t  mutex;
    pthread_t        thread;
    pthread_cond_t   cond;
    void* handle;
    char* listener;
    char* parameter;
    int32_t componentId;
    rbusFilter_t filter;
    int32_t interval;
    int32_t duration;
} sRecord;

#include "ccsp_rbus_serializer.h"
static void init_thread(sRecord* rec)
{
    pthread_mutexattr_t attrib;
    pthread_condattr_t cattrib;

    rec->running = 0;

    pthread_mutexattr_init(&attrib);
    pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&rec->mutex, &attrib);

    pthread_condattr_init(&cattrib);
    pthread_condattr_setclock(&cattrib, CLOCK_MONOTONIC);
    pthread_cond_init(&rec->cond, &cattrib);
    pthread_condattr_destroy(&cattrib);
}

static void sub_Free(void* p)
{
    sRecord* rec = (sRecord*)p;
    if(rec)
    {
        pthread_mutex_destroy(&rec->mutex);
        pthread_cond_destroy(&rec->cond);
        if(rec->listener)
            free(rec->listener);
        if(rec->parameter)
            free(rec->parameter);
        if(rec->filter)
            rbusFilter_Release(rec->filter);
        free(rec);
    }
}

static sRecord* sub_Find(void* handle, const char* listener,
        const char* parameter, int32_t componentId,
        rbusFilter_t filter, int32_t interval,
        int32_t duration)
{
    size_t i;
    for(i=0; i < rtVector_Size(gRecord); ++i)
    {
        sRecord* rec = (sRecord*)rtVector_At(gRecord, i);
        if( rec && rec->handle == handle &&
                strcmp(rec->listener, listener) == 0 &&
                strcmp(rec->parameter, parameter) == 0 &&
                rec->componentId == componentId &&
                rbusFilter_Compare(rec->filter, filter) == 0 &&
                rec->interval == interval && rec->duration == duration)
        {
            return rec;
        }
    }
    return NULL;
}

static parameterValStruct_t** rbusValueChange_GetParameterValue(sRecord* rec, int* size)
{
    int rc;
    int num_values = 0;
    int param_size = 0;
    int i;
    bool is_wildcard_query = false;
    char fullName[RBUS_MAX_NAME_LENGTH] = {0};
    parameterValStruct_t **values = 0;
    char **parameterNames = 0;
    rbusMessage req;
    CCSPBASEIF_GETPARAMETERVALUES getParameterValues =
        ((CCSP_Base_Func_CB* )(((CCSP_MESSAGE_BUS_INFO*)rec->handle)->CcspBaseIf_func))->getParameterValues;

    void* getParameterValues_data =
        ((CCSP_Base_Func_CB* )(((CCSP_MESSAGE_BUS_INFO*)rec->handle)->CcspBaseIf_func))->getParameterValues_data;
    rbusMessage_Init(&req);
    snprintf(fullName, RBUS_MAX_NAME_LENGTH, "%s", rec->parameter);
    *size = num_values;
    char *tmptr = strstr(rec->parameter, "*");
    /* wildcard */
    if (tmptr)
    {
        get_recursive_wildcard_parameterNames(rec->handle, fullName, &req, &param_size);
        is_wildcard_query = true;
    }
    else
    {
        rbusMessage_SetString(req, fullName);
        param_size += 1;

    }

    if (param_size > 0)
    {

        parameterNames = ((CCSP_MESSAGE_BUS_INFO*)rec->handle)->mallocfunc(param_size*sizeof(char *));
        memset(parameterNames, 0, param_size*sizeof(char *));
    }

    for(i = 0; i < param_size; i++)
    {
        parameterNames[i] = NULL;
        if (req)
        {
            rbusMessage_GetString(req, (const char**)&parameterNames[i]);
            RBUS_LOG("%s parameterNames[%d]: %s\n", __FUNCTION__, i, parameterNames[i]);
        }
    }

    rc = getParameterValues(0, parameterNames, param_size, &num_values, &values, getParameterValues_data);

    ((CCSP_MESSAGE_BUS_INFO*)rec->handle)->freefunc(parameterNames);

    rbusMessage_Release(req);
    if(rc == CCSP_SUCCESS)
    {
        if(!is_wildcard_query && (num_values > 1))
        {
            CcspTraceWarning(("%s %s unexpected num_vals %d\n", __FUNCTION__, rec->parameter, num_values));
            free_parameterValStruct_t(rec->handle, num_values, values);
            return NULL;
        }
        else
        {
            CcspTraceDebug(("%s %s success\n", __FUNCTION__, rec->parameter));
            *size = num_values;
            return values;
        }
    }
    else
    {
        CcspTraceError(("%s %s failed %d\n", __FUNCTION__, rec->parameter, rc));
    }
    return NULL;
}

static void* rbusInterval_PublishingThreadFunc(void *rec)
{
    CcspTraceWarning(("%s: start\n", __FUNCTION__));
    struct sRecord *sub_rec = (struct sRecord*)rec;
    int count = 0;
    int duration_count = 0;
    char *tmp = NULL;
    bool duration_complete = false;
    pthread_mutex_lock(&sub_rec->mutex);
    while(sub_rec->running)
    {
        parameterValStruct_t **val = NULL;
        int i, err;
        int size = 0;
        rtTime_t timeout;
        rtTimespec_t ts;
        rtTime_Later(NULL, (sub_rec->interval*1000), &timeout);
        err = pthread_cond_timedwait(&sub_rec->cond,
                &sub_rec->mutex,
                rtTime_ToTimespec(&timeout, &ts));
        if(err != 0 && err != ETIMEDOUT)
        {
            CcspTraceError(("Error %d:%s running command pthread_cond_timedwait", err, strerror(err)));
        }

        val = rbusValueChange_GetParameterValue(sub_rec, &size);
        if(val)
        {
            rbusMessage msg;
            rbusProperty_t tmpProperties = NULL;
            rbusObject_t data = NULL;
            rbusMessage_Init(&msg);
            rbusObject_Init(&data, NULL);
            rbusCoreError_t err;
            rbusProperty_Init(&tmpProperties, "numberOfEntries", NULL);
            rbusProperty_SetInt32(tmpProperties, size);

            for (i = 0; i < size; i++)
            {
                rbusProperty_AppendString(tmpProperties, val[i]->parameterName, val[i]->parameterValue);
            }
            /* wildcard event*/
            tmp = strstr( sub_rec->parameter, "*");
            if (tmp)
            {
                rbusObject_SetProperty(data, tmpProperties);
            }
            else
            {
                rbusObject_SetProperty(data, rbusProperty_GetNext(tmpProperties)); /* "numberOfEntries" property not requried for normal event */
            }
            rbusProperty_Release(tmpProperties);
            if (sub_rec->duration != 0)
            {
                duration_count = sub_rec->duration/sub_rec->interval;
                if (count >= duration_count)
                    duration_complete = true;
            }

            rbusEvent_t event = {0};
            event.name = sub_rec->parameter; /* use the same eventName the consumer subscribed with */
            event.data = data;
            event.type = RBUS_EVENT_INTERVAL;
            if (duration_complete)
            {
                /* Update event type after duration timeout*/
                event.type = RBUS_EVENT_DURATION_COMPLETE;
            }
            ccsp_rbusEventData_appendToMessage(&event, sub_rec->filter, sub_rec->interval, sub_rec->duration, sub_rec->componentId, msg);
            CcspTraceDebug(("%s: publising event %s to listener %s componentId %d\n", __FUNCTION__,
                        sub_rec->parameter, sub_rec->listener, sub_rec->componentId));
            err = rbus_publishSubscriberEvent(
                    ((CCSP_MESSAGE_BUS_INFO*)sub_rec->handle)->component_id,
                    sub_rec->parameter,
                    sub_rec->listener,
                    msg,
                    0, /* As subscription Id and rawdata is not supported in ccsp */
                    false);
            rbusMessage_Release(msg);
            rbusObject_Release(data);
            if(err != RBUSCORE_SUCCESS)
            {
                CcspTraceError(("%s rbus_publishSubscriberEvent failed error %d\n", __FUNCTION__, err));
            }
            free_parameterValStruct_t(sub_rec->handle, size, val);
            /*Duration complete*/
            if (duration_complete)
            {
                break;
            }
            else
            {
                count++;
            }
        }
        else
        {
            CcspTraceWarning(("%s getParameterValues failed\n", __FUNCTION__));
        }
    }
    pthread_mutex_unlock(&sub_rec->mutex);
    /*Removing duraiton completed sub from gRecord*/ 
    if (duration_complete) 
    {
        pthread_mutex_lock(&gMutex);
        rtVector_RemoveItem(gRecord, sub_rec, sub_Free);
        pthread_mutex_unlock(&gMutex);
    }
    CcspTraceWarning(("%s: stop\n", __FUNCTION__));
    return NULL;
}

int Ccsp_RbusInterval_Subscribe(void* handle, const char* listener,
        const char* parameter,
        int32_t componentId,
        int32_t interval,
        int32_t duration,
        rbusFilter_t filter)
{
    sRecord* rec;
    CcspTraceDebug(("%s: %s\n", __FUNCTION__, parameter));

    /*verify bus handle is valid*/
    if( handle == NULL ||
            ((CCSP_MESSAGE_BUS_INFO*)handle)->CcspBaseIf_func == NULL ||
            ((CCSP_Base_Func_CB* )(((CCSP_MESSAGE_BUS_INFO*)handle)->CcspBaseIf_func))->getParameterValues == NULL)
    {
        CcspTraceError(("%s NULL bus info\n", __FUNCTION__));
        return CCSP_FAILURE;
    }

    if (!gRecord)
        rtVector_Create(&gRecord);

    pthread_mutex_lock(&gMutex);
    rec = sub_Find(handle, listener, parameter, componentId, filter, interval, duration);
    pthread_mutex_unlock(&gMutex);

    if(!rec)
    {
        rec = (sRecord*)malloc(sizeof(sRecord));
        init_thread(rec);
        rec->handle = handle;
        rec->listener = strdup(listener);
        rec->parameter = strdup(parameter);
        rec->componentId = componentId;
        rec->filter = filter;
        rec->interval = interval;
        rec->duration = duration;
        if(rec->filter)
            rbusFilter_Retain(rec->filter);
        CcspTraceDebug(("%s: %s new subscriber from listener %s componentId %d\n", __FUNCTION__,
                    rec->parameter, rec->listener, rec->componentId));
        rec->running = 1;
        pthread_create(&rec->thread, NULL, rbusInterval_PublishingThreadFunc, rec);

        pthread_mutex_lock(&gMutex);
        rtVector_PushBack(gRecord, rec);
        pthread_mutex_unlock(&gMutex);
    }
    else
    {
        CcspTraceDebug(("%s: ignoring duplicate subscribe for %s from listener %s componentId %d\n",
                    __FUNCTION__, rec->parameter, rec->listener, rec->componentId));
    }
    if(filter)
    {
        rbusFilter_Release(filter);
    }
    return CCSP_SUCCESS;
}


int Ccsp_RbusInterval_Unsubscribe(void* handle, const char* listener,
        const char* parameter,
        int32_t componentId,
        int32_t interval,
        int32_t duration,
        rbusFilter_t filter)
{
    sRecord* rec = NULL;
    (void)(handle);
    CcspTraceDebug(("%s: %s\n", __FUNCTION__, parameter));
    if(!gRecord)
    {
        return CCSP_FAILURE;
    }

    pthread_mutex_lock(&gMutex);
    rec = sub_Find(handle, listener, parameter, componentId, filter, interval, duration);
    pthread_mutex_unlock(&gMutex);

    if(filter)
    {
        rbusFilter_Release(filter);
    }

    if(rec)
    {
        if(rec->running)
        {
            rec->running = 0;
            pthread_cond_signal(&rec->cond);
            pthread_join(rec->thread, NULL);
        }
        pthread_mutex_lock(&gMutex);
        rtVector_RemoveItem(gRecord, rec, sub_Free);
        pthread_mutex_unlock(&gMutex);
    }
    else
    {
        CcspTraceWarning(("%s: param not found: %s\n", __FUNCTION__, parameter));
        return CCSP_FAILURE;
    }
    return CCSP_SUCCESS;
}
