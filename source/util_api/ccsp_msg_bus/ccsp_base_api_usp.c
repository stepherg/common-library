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
 * USP shim for CcspBaseIf_* functions.
 * Replaces legacy rbus/DBus ccsp_base_api.c with USP adapter delegation.
 */

#include "ccsp_base_api.h"
#include "ccsp_message_bus.h"
#include "ccsp_usp_provider_adapter.h"
#include "ccsp_usp_consumer_adapter.h"

#include <stdlib.h>
#include <string.h>

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

static CCSP_MESSAGE_BUS_FREE get_freefunc(void* bus_handle)
{
    if (!bus_handle) return free;
    return ((CCSP_USP_BUS_HANDLE*)bus_handle)->freefunc;
}

/* Timeout variables kept for source compatibility */
int CcspBaseIf_timeout_seconds        = 60;
int CcspBaseIf_timeout_getval_seconds = 120;

/*--- Delegation to USP adapter consumer APIs ---*/

int CcspBaseIf_getParameterValues(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    char* parameterNames[], int param_size,
    int* val_size, parameterValStruct_t*** parameterval)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_GetParameterValues(
        h->consumer_handle, dst_component_id, dbus_path,
        parameterNames, param_size, val_size, parameterval);
}

int CcspBaseIf_setParameterValues(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    int sessionId, unsigned int writeID,
    parameterValStruct_t* val, int size, dbus_bool commit,
    char** invalidParameterName)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_SetParameterValues(
        h->consumer_handle, dst_component_id, dbus_path,
        sessionId, writeID, val, size, commit, invalidParameterName);
}

int CcspBaseIf_getParameterNames(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    char* parameterName, dbus_bool nextLevel,
    int* size, parameterInfoStruct_t*** parameter)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_GetParameterNames(
        h->consumer_handle, dst_component_id, dbus_path,
        parameterName, nextLevel, size, parameter);
}

int CcspBaseIf_AddTblRow(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    int sessionId, char* objectName, int* instanceNumber)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_AddTblRow(
        h->consumer_handle, dst_component_id, dbus_path,
        sessionId, objectName, instanceNumber);
}

int CcspBaseIf_DeleteTblRow(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    int sessionId, char* objectName)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_DeleteTblRow(
        h->consumer_handle, dst_component_id, dbus_path,
        sessionId, objectName);
}

int CcspBaseIf_registerCapabilities(
    void* bus_handle, const char* dst_component_id,
    const char* component_name, int component_version,
    const char* dbus_path, const char* subsystem_prefix,
    name_spaceType_t* name_space, int size)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_RegisterCapabilities(
        h->provider_handle, dst_component_id, component_name,
        component_version, dbus_path, subsystem_prefix,
        name_space, size);
}

int CcspBaseIf_registerBase(
    void* bus_handle, const char* dst_component_id,
    const char* component_name, int component_version,
    const char* dbus_path, const char* subsystem_prefix)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_RegisterBase(
        h->provider_handle, dst_component_id, component_name,
        component_version, dbus_path, subsystem_prefix);
}

int CcspBaseIf_discComponentSupportingNamespace(
    void* bus_handle, const char* dst_component_id,
    const char* name_space, const char* subsystem_prefix,
    componentStruct_t*** components, int* size)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_DiscComponentSupportingNamespace(
        h->consumer_handle, dst_component_id, name_space,
        subsystem_prefix, components, size);
}

int CcspBaseIf_isSystemReady(
    void* bus_handle, const char* dst_component_id, dbus_bool* val)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_IsSystemReady(
        h->consumer_handle, dst_component_id, val);
}

int CcspBaseIf_requestSessionID(
    void* bus_handle, const char* dst_component_id,
    int priority, int* sessionID)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_RequestSessionID(
        h->consumer_handle, dst_component_id, priority, sessionID);
}

int CcspBaseIf_informEndOfSession(
    void* bus_handle, const char* dst_component_id, int sessionID)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_InformEndOfSession(
        h->consumer_handle, dst_component_id, sessionID);
}

/*--- Provider-side signal delegation ---*/

int CcspBaseIf_SendsystemReadySignal(void* bus_handle)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_SendSystemReadySignal(h->provider_handle);
}

int CcspBaseIf_SendsystemRebootSignal(void* bus_handle)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_SendSystemRebootSignal(h->provider_handle);
}

int CcspBaseIf_SendparameterValueChangeSignal(
    void* bus_handle, parameterSigStruct_t* val, int size)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_SendParameterValueChangeSignal(
        h->provider_handle, val, size);
}

int CcspBaseIf_SenddeviceProfileChangeSignal(
    void* bus_handle, char* component_name,
    char* component_dbus_path, unsigned char isAvailable)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_SendDeviceProfileChangeSignal(
        h->provider_handle, component_name, component_dbus_path, isAvailable);
}

/*--- Stub functions for APIs without USP equivalents ---*/

int CcspBaseIf_setCommit(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    int sessionId, unsigned int writeID, dbus_bool commit)
{
    /* No-op: USP set is atomic */
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    (void)sessionId; (void)writeID; (void)commit;
    return CCSP_SUCCESS;
}

int CcspBaseIf_setParameterAttributes(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    int sessionId, parameterAttributeStruct_t* val, int size)
{
    /* USP uses SubscribeOptions instead of parameter attributes */
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    (void)sessionId; (void)val; (void)size;
    return CCSP_SUCCESS;
}

int CcspBaseIf_getParameterAttributes(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    char* parameterNames[], int size,
    int* val_size, parameterAttributeStruct_t*** parameterAttributeval)
{
    /* Not supported in USP mode — return empty */
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    (void)parameterNames; (void)size;
    if (val_size) *val_size = 0;
    if (parameterAttributeval) *parameterAttributeval = NULL;
    return CCSP_SUCCESS;
}

int CcspBaseIf_freeResources(
    void* bus_handle, const char* dst_component_id,
    char* dbus_path, int priority)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path; (void)priority;
    return CCSP_SUCCESS;
}

int CcspBaseIf_queryStatus(
    void* bus_handle, const char* dst_component_id,
    char* dbus_path, int* internalState)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    if (internalState) *internalState = 1; /* running */
    return CCSP_SUCCESS;
}

int CcspBaseIf_healthCheck(
    void* bus_handle, const char* dst_component_id,
    char* dbus_path, int* health)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    if (health) *health = 1;
    return CCSP_SUCCESS;
}

int CcspBaseIf_getHealth(
    void* bus_handle, const char* dst_component_id,
    char* dbus_path, int* health)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    if (health) *health = 1;
    return CCSP_SUCCESS;
}

int CcspBaseIf_getAllocatedMemory(
    void* bus_handle, const char* dst_component_id,
    char* dbus_path, int* directAllocatedMemory)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    if (directAllocatedMemory) *directAllocatedMemory = 0;
    return CCSP_SUCCESS;
}

int CcspBaseIf_getMaxMemoryUsage(
    void* bus_handle, const char* dst_component_id,
    char* dbus_path, int* memoryUsage)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    if (memoryUsage) *memoryUsage = 0;
    return CCSP_SUCCESS;
}

int CcspBaseIf_unregisterNamespace(
    void* bus_handle, const char* dst_component_id,
    const char* component_name, const char* name_space)
{
    (void)bus_handle; (void)dst_component_id;
    (void)component_name; (void)name_space;
    return CCSP_SUCCESS;
}

int CcspBaseIf_unregisterComponent(
    void* bus_handle, const char* dst_component_id,
    const char* component_name)
{
    (void)bus_handle; (void)dst_component_id; (void)component_name;
    return CCSP_SUCCESS;
}

int CcspBaseIf_discComponentSupportingDynamicTbl(
    void* bus_handle, const char* dst_component_id,
    const char* name_space, const char* subsystem_prefix,
    componentStruct_t** component)
{
    /* Delegate to the namespace version */
    componentStruct_t** components = NULL;
    int size = 0;
    int ret = CcspBaseIf_discComponentSupportingNamespace(
        bus_handle, dst_component_id, name_space,
        subsystem_prefix, &components, &size);
    if (ret == CCSP_SUCCESS && size > 0 && components && components[0]) {
        *component = components[0];
        /* Free container array but not the struct itself */
        CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
        ffunc(components);
    } else {
        *component = NULL;
    }
    return ret;
}

int CcspBaseIf_discNamespaceSupportedByComponent(
    void* bus_handle, const char* dst_component_id,
    const char* component_name,
    name_spaceType_t*** name_space, int* size)
{
    /* Not supported in USP mode — return empty */
    (void)bus_handle; (void)dst_component_id; (void)component_name;
    if (name_space) *name_space = NULL;
    if (size) *size = 0;
    return CCSP_SUCCESS;
}

int CcspBaseIf_getRegisteredComponents(
    void* bus_handle, const char* dst_component_id,
    registeredComponent_t*** components, int* size)
{
    (void)bus_handle; (void)dst_component_id;
    if (components) *components = NULL;
    if (size) *size = 0;
    return CCSP_SUCCESS;
}

int CcspBaseIf_checkNamespaceDataType(
    void* bus_handle, const char* dst_component_id,
    name_spaceType_t* name_space, const char* subsystem_prefix,
    dbus_bool* typeMatch)
{
    (void)bus_handle; (void)dst_component_id;
    (void)name_space; (void)subsystem_prefix;
    if (typeMatch) *typeMatch = 1;
    return CCSP_SUCCESS;
}

int CcspBaseIf_dumpComponentRegistry(
    void* bus_handle, const char* dst_component_id)
{
    (void)bus_handle; (void)dst_component_id;
    return CCSP_SUCCESS;
}

int CcspBaseIf_getCurrentSessionID(
    void* bus_handle, const char* dst_component_id,
    int* priority, int* sessionID)
{
    (void)bus_handle; (void)dst_component_id;
    if (priority) *priority = 0;
    if (sessionID) *sessionID = 0;
    return CCSP_SUCCESS;
}

int CcspBaseIf_busCheck(
    void* bus_handle, const char* dst_component_id)
{
    (void)bus_handle; (void)dst_component_id;
    return CCSP_SUCCESS;
}

int CcspBaseIf_finalize(
    void* bus_handle, const char* dst_component_id)
{
    (void)bus_handle; (void)dst_component_id;
    return CCSP_SUCCESS;
}

int CcspBaseIf_initialize(
    void* bus_handle, const char* dst_component_id)
{
    (void)bus_handle; (void)dst_component_id;
    return CCSP_SUCCESS;
}

int CcspBaseIf_EnumRecords(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    char* pParentPath, dbus_bool nextLevel,
    unsigned int* pulNumRec, PCCSP_BASE_RECORD* ppRecArray)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    (void)pParentPath; (void)nextLevel;
    if (pulNumRec) *pulNumRec = 0;
    if (ppRecArray) *ppRecArray = NULL;
    return CCSP_SUCCESS;
}

int CcspBaseIf_GetNextLevelInstances(
    void* bus_handle, const char* dst_component_id, char* dbus_path,
    char* pObjectName, unsigned int* pNums, unsigned int** pNumArray)
{
    (void)bus_handle; (void)dst_component_id; (void)dbus_path;
    (void)pObjectName;
    if (pNums) *pNums = 0;
    if (pNumArray) *pNumArray = NULL;
    return CCSP_SUCCESS;
}

int CcspBaseIf_getObjType(
    char* parent_name, char* name, int* inst_num, char* buf)
{
    (void)parent_name; (void)name; (void)inst_num; (void)buf;
    return CCSP_SUCCESS;
}

int CcspBaseIf_SendtransferCompleteSignal(void* bus_handle)
{
    CCSP_USP_BUS_HANDLE* h = (CCSP_USP_BUS_HANDLE*)bus_handle;
    if (!h) return CCSP_FAILURE;
    return CcspUspAdapter_SendSystemReadySignal(h->provider_handle);
}

int CcspBaseIf_SendtransferFailedSignal(void* bus_handle)
{
    (void)bus_handle;
    return CCSP_SUCCESS;
}

int CcspBaseIf_SendTelemetryDataSignal(void* bus_handle, char* telemetry_data)
{
    (void)bus_handle; (void)telemetry_data;
    return CCSP_SUCCESS;
}

int CcspBaseIf_WebConfigSignal(void* bus_handle, char* webconfig)
{
    (void)bus_handle; (void)webconfig;
    return CCSP_SUCCESS;
}

int CcspBaseIf_SendSignal(void* bus_handle, char* event)
{
    (void)bus_handle; (void)event;
    return CCSP_SUCCESS;
}

int CcspBaseIf_SendSignal_WithData(void* bus_handle, char* event, char* data)
{
    (void)bus_handle; (void)event; (void)data;
    return CCSP_SUCCESS;
}

int CcspBaseIf_SenddiagCompleteSignal(void* bus_handle)
{
    (void)bus_handle;
    return CCSP_SUCCESS;
}

int CcspBaseIf_SendsystemKeepaliveSignal(void* bus_handle)
{
    (void)bus_handle;
    return CCSP_SUCCESS;
}

int CcspBaseIf_Register_Event(
    void* bus_handle, const char* sender, const char* event_name)
{
    (void)bus_handle; (void)sender; (void)event_name;
    return CCSP_SUCCESS;
}

int CcspBaseIf_UnRegister_Event(
    void* bus_handle, const char* sender, const char* event_name)
{
    (void)bus_handle; (void)sender; (void)event_name;
    return CCSP_SUCCESS;
}

int CcspBaseIf_GetRemoteParameterValue(
    void* bus_handle, const char* cr_component_id,
    const char* name_space, const char* subsystem_prefix,
    char* parameterNames[], int size,
    int* val_size, parameterValStruct_t*** val)
{
    /* Delegate to regular getParameterValues — USP handles routing */
    (void)cr_component_id; (void)name_space; (void)subsystem_prefix;
    return CcspBaseIf_getParameterValues(
        bus_handle, cr_component_id, NULL,
        parameterNames, size, val_size, val);
}

int CcspBaseIf_SetRemoteParameterValue(
    void* bus_handle, const char* cr_component_id,
    const char* name_space, const char* subsystem_prefix,
    int sessionId, unsigned int writeID,
    parameterValStruct_t* val, int size, dbus_bool commit,
    char** invalidParameterName)
{
    /* Delegate to regular setParameterValues — USP handles routing */
    (void)cr_component_id; (void)name_space; (void)subsystem_prefix;
    return CcspBaseIf_setParameterValues(
        bus_handle, cr_component_id, NULL,
        sessionId, writeID, val, size, commit, invalidParameterName);
}

/*--- Free functions ---*/

void free_parameterValStruct_t(void* bus_handle, int size, parameterValStruct_t** val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    int i;
    if (!val) return;
    for (i = 0; i < size; i++) {
        if (val[i]) {
            if (val[i]->parameterName) ffunc(val[i]->parameterName);
            if (val[i]->parameterValue) ffunc(val[i]->parameterValue);
            ffunc(val[i]);
        }
    }
    ffunc(val);
}

void free_parameterInfoStruct_t(void* bus_handle, int size, parameterInfoStruct_t** val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    int i;
    if (!val) return;
    for (i = 0; i < size; i++) {
        if (val[i]) {
            if (val[i]->parameterName) ffunc(val[i]->parameterName);
            ffunc(val[i]);
        }
    }
    ffunc(val);
}

void free_parameterAttributeStruct_t(void* bus_handle, int size, parameterAttributeStruct_t** val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    int i;
    if (!val) return;
    for (i = 0; i < size; i++) {
        if (val[i]) {
            if (val[i]->parameterName) ffunc(val[i]->parameterName);
            ffunc(val[i]);
        }
    }
    ffunc(val);
}

void free_componentStruct_t(void* bus_handle, int size, componentStruct_t** val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    int i;
    if (!val) return;
    for (i = 0; i < size; i++) {
        if (val[i]) {
            if (val[i]->componentName) ffunc(val[i]->componentName);
            if (val[i]->dbusPath) ffunc(val[i]->dbusPath);
            if (val[i]->remoteCR_name) ffunc(val[i]->remoteCR_name);
            if (val[i]->remoteCR_dbus_path) ffunc(val[i]->remoteCR_dbus_path);
            ffunc(val[i]);
        }
    }
    ffunc(val);
}

void free_componentStruct_t2(void* bus_handle, componentStruct_t* val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    if (!val) return;
    if (val->componentName) ffunc(val->componentName);
    if (val->dbusPath) ffunc(val->dbusPath);
    if (val->remoteCR_name) ffunc(val->remoteCR_name);
    if (val->remoteCR_dbus_path) ffunc(val->remoteCR_dbus_path);
    ffunc(val);
}

void free_name_spaceType_t(void* bus_handle, int size, name_spaceType_t** val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    int i;
    if (!val) return;
    for (i = 0; i < size; i++) {
        if (val[i]) {
            if (val[i]->name_space) ffunc(val[i]->name_space);
            ffunc(val[i]);
        }
    }
    ffunc(val);
}

void free_char_t(void* bus_handle, int size, char** val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    int i;
    if (!val) return;
    for (i = 0; i < size; i++) {
        if (val[i]) ffunc(val[i]);
    }
    ffunc(val);
}

void free_registeredComponent_t(void* bus_handle, int size, registeredComponent_t** val)
{
    CCSP_MESSAGE_BUS_FREE ffunc = get_freefunc(bus_handle);
    int i;
    if (!val) return;
    for (i = 0; i < size; i++) {
        if (val[i]) {
            if (val[i]->componentName) ffunc(val[i]->componentName);
            if (val[i]->dbusPath) ffunc(val[i]->dbusPath);
            ffunc(val[i]);
        }
    }
    ffunc(val);
}

void free_CCSP_BASE_RECORD(void* bus_handle, PCCSP_BASE_RECORD pInstanceArray)
{
    (void)bus_handle;
    (void)pInstanceArray;
}
