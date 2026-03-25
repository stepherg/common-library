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
 * USP shim for CcspBaseIf_SetCallback and server-side registration functions.
 * Replaces legacy rbus/DBus ccsp_base_api_srv.c with USP adapter delegation.
 */

#include "ccsp_base_api.h"
#include "ccsp_message_bus.h"
#include "ccsp_usp_provider_adapter.h"

/* Re-declare the shim handle from ccsp_message_bus_usp.c */
typedef struct _CCSP_USP_BUS_HANDLE {
    /* Common prefix — must match CCSP_MESSAGE_BUS_INFO layout */
    char component_id[256];
    CCSP_MESSAGE_BUS_MALLOC mallocfunc;
    CCSP_MESSAGE_BUS_FREE freefunc;
    /* USP-specific fields */
    void* provider_handle;
    void* consumer_handle;
} CCSP_USP_BUS_HANDLE;

/* Timeout variables kept for source compatibility */
int CcspBaseIf_timeout_protect_plus_seconds    = 5;
int CcspBaseIf_deadlock_detection_time_normal_seconds = -1;
int CcspBaseIf_deadlock_detection_time_getval_seconds = -1;

void CcspBaseIf_SetCallback(
    void* bus_handle,
    CCSP_Base_Func_CB* func)
{
    if (!bus_handle || !func)
        return;

    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    CcspUspAdapter_SetCallback(h->provider_handle, func);
}

void CcspBaseIf_SetCallback2(
    void* bus_handle,
    const char* name,
    void* func,
    void* user_data)
{
    /* Not supported in USP mode — use CcspBaseIf_SetCallback */
    (void)bus_handle; (void)name; (void)func; (void)user_data;
}

void CcspBaseIf_Set_Default_Event_Callback(
    void* bus_handle,
    void* callback,
    void* user_data)
{
    /* No-op: USP uses push notifications via subscription adapter */
    (void)bus_handle; (void)callback; (void)user_data;
}

void CcspBaseIf_deadlock_detection_log_save(void)
{
    /* No-op: deadlock detection is not applicable to USP mode */
}
