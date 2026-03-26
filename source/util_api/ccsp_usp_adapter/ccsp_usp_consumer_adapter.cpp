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

#include "ccsp_usp_consumer_adapter.h"
#include "ccsp_usp_types.h"

#include <usp/controller.h>
#include <usp/types.h>
#include <usp/error.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/**
 * Internal state for a consumer-side USP adapter handle.
 */
struct CcspUspConsumerHandle {
    std::unique_ptr<usp::ControllerSession> session;
    usp::RunHandle run_handle;
    CCSP_MESSAGE_BUS_MALLOC mallocfunc;
    CCSP_MESSAGE_BUS_FREE freefunc;
    std::string component_id;
    std::string socket_path;
};

static std::string read_socket_path_consumer(const char* config_file)
{
    (void)config_file;
    return "/tmp/usp/broker_controller_path";
}

extern "C" {

int CcspUspAdapter_ConsumerInit(
    char* component_id,
    char* config_file,
    void** bus_handle,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE freefunc)
{
    if (!component_id || !bus_handle)
        return CCSP_FAILURE;

    auto handle = new (std::nothrow) CcspUspConsumerHandle();
    if (!handle)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;

    handle->component_id = component_id;
    handle->socket_path = read_socket_path_consumer(config_file);
    handle->mallocfunc = mallocfunc ? mallocfunc : (CCSP_MESSAGE_BUS_MALLOC)malloc;
    handle->freefunc = freefunc ? freefunc : (CCSP_MESSAGE_BUS_FREE)free;

    usp::SessionConfig config;
    config.endpoint_id = ccsp_usp_make_endpoint_id(handle->component_id);
    config.socket_path = handle->socket_path;

    auto result = usp::ControllerSession::create(config);
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

void CcspUspAdapter_ConsumerExit(void* bus_handle)
{
    if (!bus_handle)
        return;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);
    handle->run_handle.stop();
    handle->session.reset();
    delete handle;
}

int CcspUspAdapter_GetParameterValues(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    char* parameterNames[],
    int size,
    int* val_size,
    parameterValStruct_t*** val)
{
    (void)dst_component_id;
    (void)dbus_path;

    if (!bus_handle || !parameterNames || size <= 0 || !val_size || !val)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);
    auto alloc = handle->mallocfunc;

    // Collect results from all requested paths
    std::vector<std::pair<std::string, std::string>> all_params;

    for (int i = 0; i < size; i++) {
        if (!parameterNames[i])
            continue;

        auto result = handle->session->get_sync(parameterNames[i]);
        if (!result)
            return ccsp_usp_error_to_ccsp(result.error_code());

        for (auto& rp : result.value().results) {
            for (auto& [name, value] : rp.params) {
                all_params.emplace_back(name, value);
            }
        }
    }

    *val_size = static_cast<int>(all_params.size());
    *val = static_cast<parameterValStruct_t**>(
        alloc(sizeof(parameterValStruct_t*) * all_params.size()));
    if (!*val)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;

    for (size_t i = 0; i < all_params.size(); i++) {
        (*val)[i] = static_cast<parameterValStruct_t*>(alloc(sizeof(parameterValStruct_t)));
        if (!(*val)[i])
            return CCSP_ERR_MEMORY_ALLOC_FAIL;

        (*val)[i]->parameterName = static_cast<char*>(alloc(all_params[i].first.size() + 1));
        std::strcpy((*val)[i]->parameterName, all_params[i].first.c_str());

        (*val)[i]->parameterValue = static_cast<char*>(alloc(all_params[i].second.size() + 1));
        std::strcpy((*val)[i]->parameterValue, all_params[i].second.c_str());

        (*val)[i]->type = ccsp_string; // USP returns string-serialized values
    }

    return CCSP_SUCCESS;
}

int CcspUspAdapter_SetParameterValues(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    int sessionId,
    unsigned int writeID,
    parameterValStruct_t* val,
    int size,
    dbus_bool commit,
    char** invalidParameterName)
{
    (void)dst_component_id;
    (void)dbus_path;
    (void)sessionId;
    (void)writeID;
    (void)commit;

    if (!bus_handle || !val || size <= 0)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);

    std::vector<usp::ParamValue> params;
    params.reserve(size);
    for (int i = 0; i < size; i++) {
        if (!val[i].parameterName || !val[i].parameterValue)
            continue;
        params.push_back({val[i].parameterName, val[i].parameterValue});
    }

    auto result = handle->session->set_sync(params);
    if (!result) {
        if (invalidParameterName && !params.empty()) {
            auto alloc = handle->mallocfunc;
            *invalidParameterName = static_cast<char*>(alloc(params[0].path.size() + 1));
            if (*invalidParameterName)
                std::strcpy(*invalidParameterName, params[0].path.c_str());
        }
        return ccsp_usp_error_to_ccsp(result.error_code());
    }

    // Check for per-parameter errors in the response
    if (!result.value().errors.empty()) {
        auto& first_err = result.value().errors[0];
        if (invalidParameterName) {
            auto alloc = handle->mallocfunc;
            *invalidParameterName = static_cast<char*>(alloc(first_err.path.size() + 1));
            if (*invalidParameterName)
                std::strcpy(*invalidParameterName, first_err.path.c_str());
        }
        return ccsp_usp_error_to_ccsp(first_err.err_code);
    }

    return CCSP_SUCCESS;
}

int CcspUspAdapter_GetParameterNames(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    char* parameterName,
    dbus_bool nextLevel,
    int* size,
    parameterInfoStruct_t*** val)
{
    (void)dst_component_id;
    (void)dbus_path;

    if (!bus_handle || !parameterName || !size || !val)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);
    auto alloc = handle->mallocfunc;

    // Use get_sync to discover parameter names under the path
    auto result = handle->session->get_sync(parameterName);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    std::vector<std::pair<std::string, bool>> names; // name, writable
    for (auto& rp : result.value().results) {
        for (auto& [name, value] : rp.params) {
            (void)value;
            // If nextLevel, filter to immediate children only
            if (nextLevel) {
                std::string relative = name.substr(std::string(parameterName).size());
                // Skip if it has more than one level of depth
                auto dot_pos = relative.find('.');
                if (dot_pos != std::string::npos && dot_pos != relative.size() - 1)
                    continue;
            }
            names.emplace_back(name, true); // Default writable; schema metadata not available here
        }
    }

    *size = static_cast<int>(names.size());
    *val = static_cast<parameterInfoStruct_t**>(
        alloc(sizeof(parameterInfoStruct_t*) * names.size()));
    if (!*val)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;

    for (size_t i = 0; i < names.size(); i++) {
        (*val)[i] = static_cast<parameterInfoStruct_t*>(alloc(sizeof(parameterInfoStruct_t)));
        if (!(*val)[i])
            return CCSP_ERR_MEMORY_ALLOC_FAIL;

        (*val)[i]->parameterName = static_cast<char*>(alloc(names[i].first.size() + 1));
        std::strcpy((*val)[i]->parameterName, names[i].first.c_str());
        (*val)[i]->writable = names[i].second ? 1 : 0;
    }

    return CCSP_SUCCESS;
}

int CcspUspAdapter_AddTblRow(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    int sessionId,
    char* objectName,
    int* instanceNumber)
{
    (void)dst_component_id;
    (void)dbus_path;
    (void)sessionId;

    if (!bus_handle || !objectName || !instanceNumber)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);
    std::vector<usp::ParamValue> empty_params;

    auto result = handle->session->add_sync(objectName, empty_params);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    auto& add_results = result.value().results;
    if (add_results.empty())
        return CCSP_FAILURE;

    // Extract instance number from the created instance path
    auto* created = std::get_if<usp::AddResponse::CreatedInstance>(&add_results[0]);
    if (!created)
        return CCSP_ERR_NOT_SUPPORT;

    // Parse instance number from instantiated_path (e.g., "Device.WiFi.SSID.3.")
    std::string path = created->instantiated_path;
    // Remove trailing dot
    if (!path.empty() && path.back() == '.')
        path.pop_back();
    auto last_dot = path.rfind('.');
    if (last_dot == std::string::npos)
        return CCSP_FAILURE;

    *instanceNumber = std::atoi(path.substr(last_dot + 1).c_str());
    return CCSP_SUCCESS;
}

int CcspUspAdapter_DeleteTblRow(
    void* bus_handle,
    const char* dst_component_id,
    char* dbus_path,
    int sessionId,
    char* objectName)
{
    (void)dst_component_id;
    (void)dbus_path;
    (void)sessionId;

    if (!bus_handle || !objectName)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);

    auto result = handle->session->del_sync(objectName);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    return CCSP_SUCCESS;
}

int CcspUspAdapter_Operate(
    void* bus_handle,
    const char* command,
    const char** input_keys,
    const char** input_values,
    int input_count,
    char*** output_keys,
    char*** output_values,
    int* output_count)
{
    if (!bus_handle || !command)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);
    auto alloc = handle->mallocfunc;

    std::map<std::string, std::string> input_args;
    for (int i = 0; i < input_count; i++) {
        if (input_keys[i] && input_values[i])
            input_args[input_keys[i]] = input_values[i];
    }

    auto result = handle->session->operate_sync(command, input_args);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    auto* sync_result = std::get_if<usp::OperateResponse::SyncResult>(&result.value().result);
    if (!sync_result) {
        auto* failure = std::get_if<usp::OperateResponse::CommandFailure>(&result.value().result);
        if (failure)
            return ccsp_usp_error_to_ccsp(failure->err_code);
        return CCSP_SUCCESS; // Async result
    }

    if (output_keys && output_values && output_count) {
        *output_count = static_cast<int>(sync_result->output_args.size());
        *output_keys = static_cast<char**>(alloc(sizeof(char*) * sync_result->output_args.size()));
        *output_values = static_cast<char**>(alloc(sizeof(char*) * sync_result->output_args.size()));

        int idx = 0;
        for (auto& [key, val] : sync_result->output_args) {
            (*output_keys)[idx] = static_cast<char*>(alloc(key.size() + 1));
            std::strcpy((*output_keys)[idx], key.c_str());
            (*output_values)[idx] = static_cast<char*>(alloc(val.size() + 1));
            std::strcpy((*output_values)[idx], val.c_str());
            idx++;
        }
    }

    return CCSP_SUCCESS;
}

int CcspUspAdapter_DiscComponentSupportingNamespace(
    void* bus_handle,
    const char* dst_component_id,
    const char* name_space,
    const char* subsystem_prefix,
    componentStruct_t*** components,
    int* size)
{
    (void)dst_component_id;
    (void)name_space;
    (void)subsystem_prefix;

    if (!bus_handle || !components || !size)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);
    auto alloc = handle->mallocfunc;

    // Return a synthetic component struct — USP handles routing internally
    *size = 1;
    *components = static_cast<componentStruct_t**>(alloc(sizeof(componentStruct_t*)));
    if (!*components)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;

    (*components)[0] = static_cast<componentStruct_t*>(alloc(sizeof(componentStruct_t)));
    if (!(*components)[0])
        return CCSP_ERR_MEMORY_ALLOC_FAIL;

    const char* synthetic_name = "usp_broker";
    const char* synthetic_path = "/usp/broker";

    (*components)[0]->componentName = static_cast<char*>(alloc(std::strlen(synthetic_name) + 1));
    std::strcpy((*components)[0]->componentName, synthetic_name);

    (*components)[0]->dbusPath = static_cast<char*>(alloc(std::strlen(synthetic_path) + 1));
    std::strcpy((*components)[0]->dbusPath, synthetic_path);

    (*components)[0]->type = ccsp_string;
    (*components)[0]->remoteCR_name = nullptr;
    (*components)[0]->remoteCR_dbus_path = nullptr;

    return CCSP_SUCCESS;
}

int CcspUspAdapter_RequestSessionID(
    void* bus_handle,
    const char* dst_component_id,
    int priority,
    int* sessionID)
{
    (void)bus_handle;
    (void)dst_component_id;
    (void)priority;

    // No-op: USP provides atomicity via multi-param set
    if (sessionID)
        *sessionID = 0;
    return CCSP_SUCCESS;
}

int CcspUspAdapter_InformEndOfSession(
    void* bus_handle,
    const char* dst_component_id,
    int sessionID)
{
    (void)bus_handle;
    (void)dst_component_id;
    (void)sessionID;

    // No-op: USP provides atomicity via multi-param set
    return CCSP_SUCCESS;
}

int CcspUspAdapter_IsSystemReady(
    void* bus_handle,
    const char* dst_component_id,
    dbus_bool* val)
{
    (void)dst_component_id;

    if (!bus_handle || !val)
        return CCSP_FAILURE;

    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);

    // Query the system ready status via USP get
    auto result = handle->session->get_sync("Device.DeviceInfo.X_CCSP_SystemReady");
    if (!result) {
        *val = 0;
        return CCSP_SUCCESS; // Not ready if we can't query
    }

    for (auto& rp : result.value().results) {
        for (auto& [name, value] : rp.params) {
            *val = (value == "true" || value == "1") ? 1 : 0;
            return CCSP_SUCCESS;
        }
    }

    *val = 0;
    return CCSP_SUCCESS;
}

} // extern "C"
