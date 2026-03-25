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
#include "ccsp_psm_helper.h"
#include "ccsp_usp_provider_adapter.h"
#include "ccsp_usp_consumer_adapter.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#ifndef BOOLEAN
typedef unsigned char BOOLEAN;
#endif

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

/*--- PSM helper functions ---*/
/*
 * These delegate to the already-shimmed CcspBaseIf_* APIs, targeting the PSM component.
 * The USP broker routes requests by parameter path, so dst_component_id and dbus_path
 * are carried for compatibility but ignored by the USP adapter layer.
 */

static void psm_build_name(char* buf, size_t bufsz, const char* prefix)
{
    if (prefix && prefix[0] != '\0')
        snprintf(buf, bufsz, "%s%s", prefix, CCSP_DBUS_PSM);
    else
        snprintf(buf, bufsz, "%s", CCSP_DBUS_PSM);
}

#ifdef PSM_SLAP_VAR
int PSM_Set_Record_Value(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pRecordName,
    unsigned int const ulRecordType,
    PSLAP_VARIABLE pValue)
{
    (void)ulRecordType;
    if (!bus_handle || !pRecordName || !pValue)
        return CCSP_FAILURE;

    CCSP_MESSAGE_BUS_INFO* bus_info = (CCSP_MESSAGE_BUS_INFO*)bus_handle;
    parameterValStruct_t val[1];
    char buf[128];
    char* var_string = NULL;

    val[0].parameterName = (char*)pRecordName;
    val[0].parameterValue = buf;

    switch (pValue->Syntax) {
    case SLAP_VAR_SYNTAX_int:
        snprintf(buf, sizeof(buf), "%d", pValue->Variant.varInt);
        val[0].type = ccsp_int;
        break;
    case SLAP_VAR_SYNTAX_uint32:
        snprintf(buf, sizeof(buf), "%u", (unsigned int)pValue->Variant.varUint32);
        val[0].type = ccsp_unsignedInt;
        break;
    case SLAP_VAR_SYNTAX_bool:
        snprintf(buf, sizeof(buf), "%s", pValue->Variant.varBool ? PSM_TRUE : PSM_FALSE);
        val[0].type = ccsp_boolean;
        break;
    case SLAP_VAR_SYNTAX_string:
        val[0].parameterValue = pValue->Variant.varString;
        val[0].type = ccsp_string;
        break;
    case SLAP_VAR_SYNTAX_TYPE_ucharArray:
    {
        SLAP_UCHAR_ARRAY* arr = pValue->Variant.varUcharArray;
        var_string = bus_info->mallocfunc(arr->VarCount * 2 + 1);
        if (!var_string) return CCSP_Message_Bus_OOM;
        unsigned int i;
        for (i = 0; i < arr->VarCount; i++)
            snprintf(&var_string[i * 2], 3, "%02X", arr->Array.arrayUchar[i]);
        val[0].parameterValue = var_string;
        val[0].type = ccsp_byte;
        break;
    }
    default:
        return CCSP_CR_ERR_INVALID_PARAM;
    }

    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), pSubSystemPrefix);

    int ret = CcspBaseIf_setParameterValues(
        bus_handle, psmName, CCSP_DBUS_PATH_PSM,
        0, 0, val, 1, 1, NULL);

    if (var_string)
        bus_info->freefunc(var_string);
    return ret;
}

int PSM_Get_Record_Value(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pRecordName,
    unsigned int* ulRecordType,
    PSLAP_VARIABLE pValue)
{
    if (!bus_handle || !pRecordName || !pValue)
        return CCSP_FAILURE;

    CCSP_MESSAGE_BUS_INFO* bus_info = (CCSP_MESSAGE_BUS_INFO*)bus_handle;
    char* parameterNames[1];
    int size = 0;
    parameterValStruct_t** val = NULL;
    parameterNames[0] = (char*)pRecordName;

    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), pSubSystemPrefix);

    int ret = CcspBaseIf_getParameterValues(
        bus_handle, psmName, CCSP_DBUS_PATH_PSM,
        parameterNames, 1, &size, &val);

    if (ret != CCSP_SUCCESS)
        return ret;
    if (size < 1) {
        free_parameterValStruct_t(bus_handle, size, val);
        return CCSP_CR_ERR_INVALID_PARAM;
    }

    if (ulRecordType)
        *ulRecordType = val[0]->type;

    switch (val[0]->type) {
    case ccsp_int:
        pValue->Syntax = SLAP_VAR_SYNTAX_int;
        pValue->Variant.varInt = atoi(val[0]->parameterValue);
        break;
    case ccsp_unsignedInt:
        pValue->Syntax = SLAP_VAR_SYNTAX_uint32;
        pValue->Variant.varUint32 = (unsigned int)atoll(val[0]->parameterValue);
        break;
    case ccsp_boolean:
        pValue->Syntax = SLAP_VAR_SYNTAX_bool;
        if (!strcmp(val[0]->parameterValue, PSM_FALSE) ||
            strcasecmp(val[0]->parameterValue, "false") == 0)
            pValue->Variant.varBool = SLAP_FALSE;
        else
            pValue->Variant.varBool = TRUE;
        break;
    case ccsp_string:
        pValue->Syntax = SLAP_VAR_SYNTAX_string;
        if (pValue->Variant.varString)
            bus_info->freefunc(pValue->Variant.varString);
        pValue->Variant.varString = bus_info->mallocfunc(strlen(val[0]->parameterValue) + 1);
        if (pValue->Variant.varString)
            strcpy(pValue->Variant.varString, val[0]->parameterValue);
        break;
    case ccsp_byte:
    {
        int ulUcharCount = (int)strlen(val[0]->parameterValue) / 2;
        SLAP_UCHAR_ARRAY* varArr = (SLAP_UCHAR_ARRAY*)bus_info->mallocfunc(
            sizeof(SLAP_UCHAR_ARRAY) + ulUcharCount);
        if (varArr) {
            varArr->Size = sizeof(SLAP_UCHAR_ARRAY) + ulUcharCount;
            varArr->VarCount = ulUcharCount;
            varArr->Syntax = SLAP_VAR_SYNTAX_ucharArray;
            char* p = val[0]->parameterValue;
            int i;
            for (i = 0; i < ulUcharCount; i++) {
                unsigned int tmp = 0;
                char hex[3] = { p[i*2], p[i*2+1], 0 };
                sscanf(hex, "%02X", &tmp);
                varArr->Array.arrayUchar[i] = (unsigned char)tmp;
            }
            if (pValue->Variant.varUcharArray)
                bus_info->freefunc(pValue->Variant.varUcharArray);
            pValue->Variant.varUcharArray = varArr;
        }
        pValue->Syntax = SLAP_VAR_SYNTAX_TYPE_ucharArray;
        break;
    }
    default:
        ret = CCSP_CR_ERR_INVALID_PARAM;
    }

    free_parameterValStruct_t(bus_handle, size, val);
    return ret;
}
#endif /* PSM_SLAP_VAR */

int PSM_Set_Record_Value2(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pRecordName,
    unsigned int const ulRecordType,
    char const* const pVal)
{
    if (!bus_handle || !pRecordName || !pVal)
        return CCSP_FAILURE;

    if (ulRecordType == ccsp_boolean) {
        if (strcmp(pVal, PSM_FALSE) && strcmp(pVal, PSM_TRUE))
            return CCSP_CR_ERR_INVALID_PARAM;
    }

    parameterValStruct_t val[1];
    val[0].parameterName = (char*)pRecordName;
    val[0].type = ulRecordType;
    val[0].parameterValue = (char*)pVal;

    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), pSubSystemPrefix);

    return CcspBaseIf_setParameterValues(
        bus_handle, psmName, CCSP_DBUS_PATH_PSM,
        0, 0, val, 1, 1, NULL);
}

int PSM_Get_Record_Value2(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pRecordName,
    unsigned int* ulRecordType,
    char** pValue)
{
    if (!bus_handle || !pRecordName || !pValue)
        return CCSP_FAILURE;

    *pValue = NULL;
    CCSP_MESSAGE_BUS_INFO* bus_info = (CCSP_MESSAGE_BUS_INFO*)bus_handle;
    char* parameterNames[1];
    parameterValStruct_t** val = NULL;
    int size = 0;
    parameterNames[0] = (char*)pRecordName;

    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), pSubSystemPrefix);

    int ret = CcspBaseIf_getParameterValues(
        bus_handle, psmName, CCSP_DBUS_PATH_PSM,
        parameterNames, 1, &size, &val);

    if (ret == CCSP_SUCCESS && size > 0 && val && val[0]) {
        if (ulRecordType)
            *ulRecordType = val[0]->type;
        if (val[0]->type == ccsp_boolean) {
            *pValue = bus_info->mallocfunc(6); /* "FALSE\0" */
            if (*pValue)
                snprintf(*pValue, 6, "%s",
                    (strcasecmp(val[0]->parameterValue, "true") == 0) ? "TRUE" : "FALSE");
        } else {
            *pValue = bus_info->mallocfunc(strlen(val[0]->parameterValue) + 1);
            if (*pValue)
                strcpy(*pValue, val[0]->parameterValue);
        }
    }
    free_parameterValStruct_t(bus_handle, size, val);
    return ret;
}

int PSM_Del_Record(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pRecordName)
{
    if (!bus_handle || !pRecordName)
        return CCSP_FAILURE;

    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), pSubSystemPrefix);

    /*
     * Legacy implementation "deletes" a PSM record by setting its
     * accessControlBitmask to 0.  In USP mode the CcspBaseIf_setParameterAttributes
     * shim is a no-op, so we delegate to the same semantic — the PSM agent on the
     * broker side interprets this as a delete.
     *
     * For subtree deletes (name ends with '.'), we enumerate children first.
     */
    size_t len = strlen(pRecordName);

    if (len > 0 && pRecordName[len - 1] == '.') {
        /* Subtree delete: enumerate then delete each child */
        parameterInfoStruct_t** parameter = NULL;
        int size = 0;
        int ret = CcspBaseIf_getParameterNames(
            bus_handle, psmName, CCSP_DBUS_PATH_PSM,
            (char*)pRecordName, 0, &size, &parameter);
        if (ret != CCSP_SUCCESS)
            return ret;
        int i;
        for (i = 0; i < size; i++) {
            parameterAttributeStruct_t attr;
            attr.parameterName = parameter[i]->parameterName;
            attr.notificationChanged = 0;
            attr.notification = 0;
            attr.access = 0;
            attr.accessControlChanged = 1;
            attr.accessControlBitmask = 0;
            ret = CcspBaseIf_setParameterAttributes(
                bus_handle, psmName, CCSP_DBUS_PATH_PSM, 0, &attr, 1);
            if (ret != CCSP_SUCCESS)
                break;
        }
        free_parameterInfoStruct_t(bus_handle, size, parameter);
        return ret;
    } else {
        parameterAttributeStruct_t attr;
        attr.parameterName = (char*)pRecordName;
        attr.notificationChanged = 0;
        attr.notification = 0;
        attr.access = 0;
        attr.accessControlChanged = 1;
        attr.accessControlBitmask = 0;
        return CcspBaseIf_setParameterAttributes(
            bus_handle, psmName, CCSP_DBUS_PATH_PSM, 0, &attr, 1);
    }
}

int PsmGetNextLevelInstances(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pParentPath,
    unsigned int* pulNumInstance,
    unsigned int** ppInstanceArray)
{
    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), pSubSystemPrefix);

    return CcspBaseIf_GetNextLevelInstances(
        bus_handle, psmName, CCSP_DBUS_PATH_PSM,
        (char*)pParentPath, pulNumInstance, ppInstanceArray);
}

int PsmEnumRecords(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pParentPath,
    dbus_bool nextLevel,
    unsigned int* pulNumRec,
    PCCSP_BASE_RECORD* ppRecArray)
{
    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), pSubSystemPrefix);

    return CcspBaseIf_EnumRecords(
        bus_handle, psmName, CCSP_DBUS_PATH_PSM,
        (char*)pParentPath, nextLevel, pulNumRec, ppRecArray);
}

int PsmGroupGet(
    void* bus_handle,
    const char* subsys,
    const char* names[],
    int nname,
    parameterValStruct_t*** records,
    int* nrec)
{
    if (!bus_handle || !names || !records || !nrec)
        return CCSP_FAILURE;

    char psmName[256];
    psm_build_name(psmName, sizeof(psmName), subsys);

    return CcspBaseIf_getParameterValues(
        bus_handle, psmName, CCSP_DBUS_PATH_PSM,
        (char**)names, nname, nrec, records);
}

void PsmFreeRecords(
    void* bus_handle,
    parameterValStruct_t** records,
    int nrec)
{
    free_parameterValStruct_t(bus_handle, nrec, records);
}

int PSM_Reset_UserChangeFlag(
    void* bus_handle,
    char const* const pSubSystemPrefix,
    char const* const pathName)
{
    char record_name[256];
    snprintf(record_name, sizeof(record_name), "UserChanged.%s", pathName);
    return PSM_Del_Record(bus_handle, pSubSystemPrefix, record_name);
}

int Rbus_to_CCSP_error_mapper(int error_code)
{
    /* In USP mode, pass through — error codes are already mapped at the adapter layer */
    (void)error_code;
    return CCSP_FAILURE;
}

int Rbus2_to_CCSP_error_mapper(int error_code)
{
    (void)error_code;
    return CCSP_FAILURE;
}

int getPartnerId(char* partnerID)
{
    if (!partnerID)
        return -1;
    partnerID[0] = '\0';
    return 0;
}

BOOLEAN waitConditionReady(
    void* hMBusHandle,
    const char* dst_component_id,
    char* dbus_path,
    char* src_component_id)
{
#define WAIT_MAX_TIME 10
#define WAIT_INTERVAL 2000
#define CCSP_COMMON_COMPONENT_HEALTH_Green 3
    int times = 0;
    int ret = 0;
    int health = 0;

    while (times++ < WAIT_MAX_TIME) {
        ret = CcspBaseIf_getHealth(hMBusHandle, dst_component_id, dbus_path, &health);
        if (health != CCSP_COMMON_COMPONENT_HEALTH_Green || ret != CCSP_SUCCESS) {
            CCSP_Msg_SleepInMilliSeconds(WAIT_INTERVAL);
        } else {
            return 1; /* true */
        }
    }
    return 0; /* false — timed out */
}
