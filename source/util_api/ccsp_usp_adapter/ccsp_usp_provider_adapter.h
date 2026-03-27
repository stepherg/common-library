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

#ifndef CCSP_USP_PROVIDER_ADAPTER_H
#define CCSP_USP_PROVIDER_ADAPTER_H

#include "ccsp_base_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a USP AgentSession, replacing CCSP_Message_Bus_Init for provider components.
 *
 * Creates a usp::AgentSession using the component_id as endpoint_id and the config_file
 * to resolve the USP broker socket path. Starts a background event loop.
 *
 * @param component_id   Unique component identifier (becomes USP endpoint_id)
 * @param config_file    Path to config file containing socket_path
 * @param bus_handle     [out] Opaque handle wrapping the AgentSession
 * @param mallocfunc     Memory allocator (used for CCSP-compatible allocations)
 * @param freefunc       Memory deallocator
 * @return CCSP_SUCCESS on success, CCSP_ERR_NOT_CONNECT on failure
 */
int CcspUspAdapter_ProviderInit(
    char* component_id,
    char* config_file,
    void** bus_handle,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE freefunc
);

/**
 * Shut down the USP AgentSession, replacing CCSP_Message_Bus_Exit for providers.
 *
 * Stops the background event loop and destroys the AgentSession.
 *
 * @param bus_handle    Handle returned by CcspUspAdapter_ProviderInit
 */
void CcspUspAdapter_ProviderExit(void* bus_handle);

/**
 * Register a data model namespace with the USP broker.
 *
 * Converts name_spaceType_t arrays and CCSP_Base_Func_CB callbacks into
 * usp::DataObjects into a Registration and registers via AgentSession::register_objects_sync().
 *
 * @param bus_handle        Handle from CcspUspAdapter_ProviderInit
 * @param dst_component_id  Ignored (USP handles routing)
 * @param component_name    Component name for registration
 * @param component_version Component version
 * @param dbus_path         Ignored (USP handles routing)
 * @param subsystem_prefix  Subsystem prefix
 * @param name_space        Array of namespace declarations
 * @param size              Number of entries in name_space
 * @return CCSP_SUCCESS on success
 */
int CcspUspAdapter_RegisterCapabilities(
    void* bus_handle,
    const char* dst_component_id,
    const char* component_name,
    int component_version,
    const char* dbus_path,
    const char* subsystem_prefix,
    name_spaceType_t* name_space,
    int size
);

/**
 * Set the provider-side callback table.
 *
 * Stores the CCSP_Base_Func_CB callbacks so that USP DataObject handlers can delegate
 * get/set/add/delete operations to the component's existing handlers.
 *
 * @param bus_handle    Handle from CcspUspAdapter_ProviderInit
 * @param func          Callback table
 */
void CcspUspAdapter_SetCallback(
    void* bus_handle,
    CCSP_Base_Func_CB* func
);

/**
 * Register base component information with the USP broker.
 *
 * @param bus_handle        Handle from init
 * @param dst_component_id  Ignored
 * @param component_name    Component name
 * @param component_version Version
 * @param dbus_path         Ignored
 * @param subsystem_prefix  Subsystem prefix
 * @return CCSP_SUCCESS
 */
int CcspUspAdapter_RegisterBase(
    void* bus_handle,
    const char* dst_component_id,
    const char* component_name,
    int component_version,
    const char* dbus_path,
    const char* subsystem_prefix
);

/**
 * Send system ready signal via USP event notification.
 */
int CcspUspAdapter_SendSystemReadySignal(void* bus_handle);

/**
 * Send system reboot signal via USP event notification.
 */
int CcspUspAdapter_SendSystemRebootSignal(void* bus_handle);

/**
 * Send parameter value change signal via USP notification.
 */
int CcspUspAdapter_SendParameterValueChangeSignal(
    void* bus_handle,
    parameterSigStruct_t* val,
    int size
);

/**
 * Send device profile change signal via USP event notification.
 */
int CcspUspAdapter_SendDeviceProfileChangeSignal(
    void* bus_handle,
    char* component_name,
    char* component_dbus_path,
    unsigned char isAvailable
);

#ifdef __cplusplus
}
#endif

#endif /* CCSP_USP_PROVIDER_ADAPTER_H */
