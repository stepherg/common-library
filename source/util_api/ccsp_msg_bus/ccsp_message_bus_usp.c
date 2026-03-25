/*
 * Copyright 2024 RDK Management
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

/**
 * USP shim for CCSP_Message_Bus_Init / Exit and related bus functions.
 * Replaces the legacy rbus/DBus ccsp_message_bus.c with USP adapter delegation.
 */

#include "ccsp_message_bus.h"
#include "ccsp_base_api.h"
#include "ccsp_usp_provider_adapter.h"
#include "ccsp_usp_consumer_adapter.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

/*
 * Internal state: tracks whether the handle is provider or consumer.
 * For the unified init, we create both a provider and consumer session
 * wrapped in a shim handle.
 */
typedef struct _CCSP_USP_BUS_HANDLE {
    /* Common prefix — must match CCSP_MESSAGE_BUS_INFO layout */
    char component_id[256];
    CCSP_MESSAGE_BUS_MALLOC mallocfunc;
    CCSP_MESSAGE_BUS_FREE freefunc;
    /* USP-specific fields */
    void* provider_handle;  /* CcspUspProviderHandle* */
    void* consumer_handle;  /* CcspUspConsumerHandle* */
} CCSP_USP_BUS_HANDLE;

void CCSP_Msg_SleepInMilliSeconds(int milliSecond)
{
    struct timeval tm;
    tm.tv_sec = milliSecond / 1000;
    tm.tv_usec = (milliSecond % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tm);
}

int CCSP_Msg_IsRbus_enabled(void)
{
    /* USP mode is always active */
    return 0;
}

int CCSP_Message_Bus_Init(
    char* component_id,
    char* config_file,
    void** bus_handle,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE freefunc)
{
    if (!component_id || !bus_handle)
        return CCSP_Message_Bus_ERROR;

    CCSP_MESSAGE_BUS_MALLOC mfunc = mallocfunc ? mallocfunc : (CCSP_MESSAGE_BUS_MALLOC)malloc;
    CCSP_MESSAGE_BUS_FREE ffunc = freefunc ? freefunc : (CCSP_MESSAGE_BUS_FREE)free;

    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)mfunc(sizeof(CCSP_USP_BUS_HANDLE));
    if (!h)
        return CCSP_Message_Bus_OOM;

    memset(h, 0, sizeof(*h));
    strncpy(h->component_id, component_id, sizeof(h->component_id) - 1);
    h->mallocfunc = mfunc;
    h->freefunc = ffunc;

    /* Initialize provider session */
    int ret = CcspUspAdapter_ProviderInit(component_id, config_file,
                                          &h->provider_handle, mfunc, ffunc);
    if (ret != CCSP_SUCCESS) {
        ffunc(h);
        return CCSP_MESSAGE_BUS_CANNOT_CONNECT;
    }

    /* Initialize consumer session */
    ret = CcspUspAdapter_ConsumerInit(component_id, config_file,
                                      &h->consumer_handle, mfunc, ffunc);
    if (ret != CCSP_SUCCESS) {
        CcspUspAdapter_ProviderExit(h->provider_handle);
        ffunc(h);
        return CCSP_MESSAGE_BUS_CANNOT_CONNECT;
    }

    *bus_handle = h;
    return CCSP_Message_Bus_OK;
}

void CCSP_Message_Bus_Exit(void* bus_handle)
{
    if (!bus_handle)
        return;

    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    CcspUspAdapter_ConsumerExit(h->consumer_handle);
    CcspUspAdapter_ProviderExit(h->provider_handle);
    h->freefunc(h);
}

int CCSP_Message_Bus_Register_Event(
    void* bus_handle,
    const char* sender,
    const char* path,
    const char* interface,
    const char* event_name)
{
    /* No-op: USP subscriptions replace event registration */
    (void)bus_handle; (void)sender; (void)path;
    (void)interface; (void)event_name;
    return CCSP_Message_Bus_OK;
}

int CCSP_Message_Bus_UnRegister_Event(
    void* bus_handle,
    const char* sender,
    const char* path,
    const char* interface,
    const char* event_name)
{
    (void)bus_handle; (void)sender; (void)path;
    (void)interface; (void)event_name;
    return CCSP_Message_Bus_OK;
}

void CCSP_Message_Bus_Set_Event_Callback(
    void* bus_handle,
    void* callback,
    void* user_data)
{
    /* No-op: USP uses push notifications */
    (void)bus_handle; (void)callback; (void)user_data;
}

int CCSP_Message_Bus_Register_Path2(
    void* bus_handle,
    const char* path,
    void* funcptr,
    void* user_data)
{
    /* No-op: USP uses provide() + handle() for path registration */
    (void)bus_handle; (void)path; (void)funcptr; (void)user_data;
    return CCSP_Message_Bus_OK;
}

int CCSP_Message_Bus_Send_Str(
    void* conn,
    char* component_id,
    const char* path,
    const char* interface,
    const char* method,
    char* request)
{
    /* Not supported in USP mode */
    (void)conn; (void)component_id; (void)path;
    (void)interface; (void)method; (void)request;
    return CCSP_MESSAGE_BUS_NOT_SUPPORT;
}

int CCSP_Message_Bus_Send_Msg(
    void* bus_handle,
    void* message,
    int timeout_seconds,
    void** result)
{
    /* Not supported in USP mode */
    (void)bus_handle; (void)message; (void)timeout_seconds; (void)result;
    return CCSP_MESSAGE_BUS_NOT_SUPPORT;
}

int CCSP_Message_Bus_Send_Msg_Block(
    void* bus_handle,
    void* message,
    int timeout_seconds,
    void** result)
{
    /* Not supported in USP mode */
    (void)bus_handle; (void)message; (void)timeout_seconds; (void)result;
    return CCSP_MESSAGE_BUS_NOT_SUPPORT;
}
