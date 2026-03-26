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

#include "ccsp_usp_provider_adapter.h"
#include "ccsp_usp_types.h"

#include <usp/agent.h>
#include <usp/schema.h>
#include <usp/types.h>
#include <usp/error.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Internal state for a provider-side USP adapter handle.
 */
struct CcspUspProviderHandle {
    std::unique_ptr<usp::AgentSession> session;
    usp::RunHandle run_handle;
    CCSP_Base_Func_CB callbacks;
    CCSP_MESSAGE_BUS_MALLOC mallocfunc;
    CCSP_MESSAGE_BUS_FREE freefunc;
    std::string component_id;
    std::string socket_path;
    std::mutex mutex;
};

static std::string read_socket_path(const char* config_file)
{
    // Default USP broker socket path
    // TODO: Parse config_file for actual socket path
    (void)config_file;
    return "/tmp/usp/broker_agent_path";
}

extern "C" {

int CcspUspAdapter_ProviderInit(
    char* component_id,
    char* config_file,
    void** bus_handle,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE freefunc)
{
    if (!component_id || !bus_handle)
        return CCSP_FAILURE;

    auto handle = new (std::nothrow) CcspUspProviderHandle();
    if (!handle)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;

    handle->component_id = component_id;
    handle->socket_path = read_socket_path(config_file);
    handle->mallocfunc = mallocfunc ? mallocfunc : (CCSP_MESSAGE_BUS_MALLOC)malloc;
    handle->freefunc = freefunc ? freefunc : (CCSP_MESSAGE_BUS_FREE)free;
    std::memset(&handle->callbacks, 0, sizeof(handle->callbacks));

    usp::SessionConfig config;
    config.endpoint_id = handle->component_id;
    config.socket_path = handle->socket_path;

    auto result = usp::AgentSession::create(config);
    if (!result) {
        delete handle;
        return CCSP_ERR_NOT_CONNECT;
    }

    handle->session = std::move(result.value());

    auto start_result = handle->session->start();
    if (!start_result) {
        delete handle;
        return CCSP_ERR_NOT_CONNECT;
    }
    handle->run_handle = std::move(start_result.value());

    *bus_handle = handle;
    return CCSP_SUCCESS;
}

void CcspUspAdapter_ProviderExit(void* bus_handle)
{
    if (!bus_handle)
        return;

    auto* handle = static_cast<CcspUspProviderHandle*>(bus_handle);
    handle->run_handle.stop();
    handle->session.reset();
    delete handle;
}

void CcspUspAdapter_SetCallback(void* bus_handle, CCSP_Base_Func_CB* func)
{
    if (!bus_handle || !func)
        return;

    auto* handle = static_cast<CcspUspProviderHandle*>(bus_handle);
    std::lock_guard<std::mutex> lock(handle->mutex);
    handle->callbacks = *func;
}

int CcspUspAdapter_RegisterBase(
    void* bus_handle,
    const char* dst_component_id,
    const char* component_name,
    int component_version,
    const char* dbus_path,
    const char* subsystem_prefix)
{
    (void)dst_component_id;
    (void)component_name;
    (void)component_version;
    (void)dbus_path;
    (void)subsystem_prefix;

    if (!bus_handle)
        return CCSP_FAILURE;

    // In USP, registration happens via provide() + handle().
    // RegisterBase is a no-op — the session is already connected.
    return CCSP_SUCCESS;
}

int CcspUspAdapter_RegisterCapabilities(
    void* bus_handle,
    const char* dst_component_id,
    const char* component_name,
    int component_version,
    const char* dbus_path,
    const char* subsystem_prefix,
    name_spaceType_t* name_space,
    int size)
{
    (void)dst_component_id;
    (void)component_version;
    (void)dbus_path;
    (void)subsystem_prefix;

    if (!bus_handle || !name_space || size <= 0)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspProviderHandle*>(bus_handle);

    // Build schema from namespace array
    auto schema_handlers = ccsp_usp_build_schema_and_handlers(
        name_space, size, &handle->callbacks, handle->mallocfunc, handle->freefunc,
        handle->session.get());

    for (auto& [path, schema, obj_handlers] : schema_handlers) {
        usp::AgentHandlers agent_handlers;

        // Default on_operate returns "not supported" for commands.
        // CCSP uses set-parameter-to-trigger-action patterns rather than
        // explicit command dispatch. Components can override via custom handler.
        agent_handlers.on_operate = [](const usp::OperateRequest& req) -> usp::OperateResponse {
            usp::OperateResponse resp;
            resp.result = usp::OperateResponse::CommandFailure{
                usp::ErrorCode::OperateNotAllowed,
                "Command not supported via CCSP adapter: " + req.command
            };
            return resp;
        };

        auto status = handle->session->provide(path, std::move(schema), agent_handlers);
        if (!status)
            return ccsp_usp_error_to_ccsp(status.error_code());

        for (auto& [obj_path, handlers] : obj_handlers) {
            auto h_status = handle->session->handle(obj_path, std::move(handlers));
            if (!h_status)
                return ccsp_usp_error_to_ccsp(h_status.error_code());
        }
    }

    return CCSP_SUCCESS;
}

int CcspUspAdapter_SendSystemReadySignal(void* bus_handle)
{
    if (!bus_handle)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspProviderHandle*>(bus_handle);
    auto status = handle->session->emit_event("Device.", "SystemReady", {});
    return status.ok() ? CCSP_SUCCESS : CCSP_FAILURE;
}

int CcspUspAdapter_SendSystemRebootSignal(void* bus_handle)
{
    if (!bus_handle)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspProviderHandle*>(bus_handle);
    auto status = handle->session->emit_event("Device.", "SystemReboot", {});
    return status.ok() ? CCSP_SUCCESS : CCSP_FAILURE;
}

int CcspUspAdapter_SendParameterValueChangeSignal(
    void* bus_handle,
    parameterSigStruct_t* val,
    int size)
{
    if (!bus_handle || !val)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspProviderHandle*>(bus_handle);
    for (int i = 0; i < size; i++) {
        if (val[i].parameterName && val[i].newValue) {
            handle->session->emit_value_change(val[i].parameterName, val[i].newValue);
        }
    }
    return CCSP_SUCCESS;
}

int CcspUspAdapter_SendDeviceProfileChangeSignal(
    void* bus_handle,
    char* component_name,
    char* component_dbus_path,
    unsigned char isAvailable)
{
    if (!bus_handle)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspProviderHandle*>(bus_handle);
    std::map<std::string, std::string> params;
    if (component_name)
        params["ComponentName"] = component_name;
    if (component_dbus_path)
        params["ComponentPath"] = component_dbus_path;
    params["IsAvailable"] = isAvailable ? "true" : "false";

    auto status = handle->session->emit_event("Device.", "DeviceProfileChange", params);
    return status.ok() ? CCSP_SUCCESS : CCSP_FAILURE;
}

} // extern "C"
