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

#ifndef CCSP_USP_CONSUMER_ADAPTER_H
#define CCSP_USP_CONSUMER_ADAPTER_H

#include "ccsp_base_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a USP ControllerSession for consumer-side operations.
 *
 * Creates a usp::ControllerSession and starts a background event loop.
 *
 * @param component_id   Unique component identifier (becomes USP endpoint_id)
 * @param config_file    Path to config file containing socket_path
 * @param bus_handle     [out] Opaque handle wrapping the ControllerSession
 * @param mallocfunc     Memory allocator
 * @param freefunc       Memory deallocator
 * @return CCSP_SUCCESS on success, CCSP_ERR_NOT_CONNECT on failure
 */
int CcspUspAdapter_ConsumerInit(
    char* component_id,
    char* config_file,
    void** bus_handle,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE freefunc
);

/**
 * Shut down the USP ControllerSession.
 */
void CcspUspAdapter_ConsumerExit(void* bus_handle);

/**
 * Get parameter values via USP.
 *
 * Delegates to ControllerSession::get_sync(). The dst_component_id and dbus_path
 * parameters are accepted but ignored — USP routes by parameter path.
 *
 * @param bus_handle        Handle from consumer init
 * @param dst_component_id  Ignored (USP handles routing)
 * @param dbus_path         Ignored (USP handles routing)
 * @param parameterNames    Array of parameter paths to query
 * @param size              Number of paths
 * @param val_size          [out] Number of results
 * @param val               [out] Array of parameter name/value/type structs
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_GetParameterValues(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    char* parameterNames[],
    int size,
    int* val_size,
    parameterValStruct_t*** val
);

/**
 * Set parameter values via USP.
 *
 * Delegates to ControllerSession::set_sync(). Destination parameters are ignored.
 *
 * @param bus_handle            Handle from consumer init
 * @param dst_component_id      Ignored
 * @param dbus_path             Ignored
 * @param sessionId             Ignored (USP provides atomicity via multi-param set)
 * @param writeID               Ignored
 * @param val                   Array of parameter name/value/type structs to set
 * @param size                  Number of parameters
 * @param commit                Ignored
 * @param invalidParameterName  [out] Name of first invalid parameter on error
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_SetParameterValues(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    int sessionId,
    unsigned int writeID,
    parameterValStruct_t* val,
    int size,
    dbus_bool commit,
    char** invalidParameterName
);

/**
 * Get parameter names via USP.
 *
 * @param bus_handle        Handle from consumer init
 * @param dst_component_id  Ignored
 * @param dbus_path         Ignored
 * @param parameterName     Object or parameter path
 * @param nextLevel         If true, return only next-level children
 * @param size              [out] Number of results
 * @param val               [out] Array of parameter info structs
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_GetParameterNames(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    char* parameterName,
    dbus_bool nextLevel,
    int* size,
    parameterInfoStruct_t*** val
);

/**
 * Add a table row via USP.
 *
 * @param bus_handle        Handle from consumer init
 * @param dst_component_id  Ignored
 * @param dbus_path         Ignored
 * @param sessionId         Ignored
 * @param objectName        Table object path ending with "."
 * @param instanceNumber    [out] New instance number
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_AddTblRow(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    int sessionId,
    char* objectName,
    int* instanceNumber
);

/**
 * Delete a table row via USP.
 *
 * @param bus_handle        Handle from consumer init
 * @param dst_component_id  Ignored
 * @param dbus_path         Ignored
 * @param sessionId         Ignored
 * @param objectName        Instance path ending with "."
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_DeleteTblRow(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    int sessionId,
    char* objectName
);

/**
 * Invoke a USP Operate command.
 *
 * @param bus_handle    Handle from consumer init
 * @param command       Command path (e.g., "Device.IP.Diagnostics.IPPing()")
 * @param input_keys    Array of input argument names
 * @param input_values  Array of input argument values
 * @param input_count   Number of input arguments
 * @param output_keys   [out] Array of output argument names (caller frees)
 * @param output_values [out] Array of output argument values (caller frees)
 * @param output_count  [out] Number of output arguments
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_Operate(
    void* bus_handle,
    const char* command,
    const char** input_keys,
    const char** input_values,
    int input_count,
    char*** output_keys,
    char*** output_values,
    int* output_count
);

/**
 * Discover component supporting namespace — stub.
 *
 * Returns a synthetic componentStruct_t so existing discover-then-query
 * callers don't hit failure paths. USP handles routing internally.
 *
 * @param bus_handle        Handle from consumer init
 * @param dst_component_id  Ignored
 * @param name_space        Namespace to discover
 * @param subsystem_prefix  Subsystem prefix
 * @param components        [out] Synthetic component array
 * @param size              [out] Always 1
 * @return CCSP_SUCCESS
 */
int CcspUspAdapter_DiscComponentSupportingNamespace(
    void* bus_handle,
    const char* dst_component_id,
    const char* name_space,
    const char* subsystem_prefix,
    componentStruct_t*** components,
    int* size
);

/**
 * Request session ID — no-op stub.
 * USP provides atomicity via multi-param set.
 */
int CcspUspAdapter_RequestSessionID(
    void* bus_handle,
    const char* dst_component_id,
    int priority,
    int* sessionID
);

/**
 * Inform end of session — no-op stub.
 */
int CcspUspAdapter_InformEndOfSession(
    void* bus_handle,
    const char* dst_component_id,
    int sessionID
);

/**
 * Check if system is ready via USP.
 */
int CcspUspAdapter_IsSystemReady(
    void* bus_handle,
    const char* dst_component_id,
    dbus_bool* val
);

#ifdef __cplusplus
}
#endif

#endif /* CCSP_USP_CONSUMER_ADAPTER_H */
