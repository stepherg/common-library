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

#include "ccsp_usp_types.h"

#include <usp/agent.h>
#include <usp/controller.h>
#include <usp/schema.h>
#include <usp/types.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>



// Re-declare the consumer handle struct so we can extract the ControllerSession.
// This must match the definition in ccsp_usp_consumer_adapter.cpp.
struct CcspUspConsumerHandle {
    std::unique_ptr<usp::ControllerSession> session;
    usp::RunHandle run_handle;
    CCSP_MESSAGE_BUS_MALLOC mallocfunc;
    CCSP_MESSAGE_BUS_FREE freefunc;
    std::string component_id;
    std::string socket_path;
};

usp::ControllerSession* ccsp_usp_get_controller_session(void* bus_handle)
{
    if (!bus_handle)
        return nullptr;
    auto* handle = static_cast<CcspUspConsumerHandle*>(bus_handle);
    return handle->session.get();
}

/**
 * Extract the object path prefix and parameter name from a full namespace path.
 * "Device.WiFi.SSID.{i}.SSID" -> ("Device.WiFi.SSID.{i}.", "SSID")
 * "Device.WiFi.SSID.{i}."     -> ("Device.WiFi.SSID.{i}.", "")
 * "Device.DeviceInfo.ModelName" -> ("Device.DeviceInfo.", "ModelName")
 */
static std::pair<std::string, std::string> extract_object_and_param(const std::string& ns)
{
    if (ns.empty())
        return {"", ""};

    // If it ends with '.', it's an object path itself
    if (ns.back() == '.')
        return {ns, ""};

    auto last_dot = ns.rfind('.');
    if (last_dot == std::string::npos)
        return {"", ns};

    return {ns.substr(0, last_dot + 1), ns.substr(last_dot + 1)};
}

/**
 * Check if an object path indicates a multi-instance object.
 * Heuristic: contains "{i}" as a path segment.
 */
static bool is_multi_instance_path(const std::string& obj_path)
{
    return obj_path.find("{i}.") != std::string::npos;
}

/**
 * Struct to accumulate object info during namespace parsing.
 */
struct ObjectInfo {
    std::string path;
    bool is_multi = false;
    std::vector<usp::ParamDef> params;
};

usp::Registration ccsp_usp_build_registration(
    name_spaceType_t* name_space,
    int size,
    CCSP_Base_Func_CB* callbacks,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE freefunc,
    usp::AgentSession* session)
{
    usp::Registration registration;

    if (!name_space || size <= 0 || !callbacks)
        return registration;

    // Phase 1: Parse all namespaces and group by object path
    std::map<std::string, ObjectInfo> objects;

    for (int i = 0; i < size; i++) {
        if (!name_space[i].name_space)
            continue;

        std::string ns = name_space[i].name_space;
        auto [obj_path, param_name] = extract_object_and_param(ns);

        if (obj_path.empty())
            continue;

        // Skip non-USP namespaces (USP paths must start with "Device.")
        //if (obj_path.find("Device.") != 0)
        //    continue;

        auto& obj = objects[obj_path];
        obj.path = obj_path;
        obj.is_multi = is_multi_instance_path(obj_path);

        if (!param_name.empty()) {
            usp::ParamDef param;
            param.name = param_name;
            param.type = ccsp_datatype_to_usp(name_space[i].dataType);
            param.access = usp::ParamAccess::ReadWrite; // Default; callbacks enforce real access
            obj.params.push_back(std::move(param));
        }
    }

    if (objects.empty())
        return registration;

    // Phase 2: Build DataObjects with handlers for each object path
    // Capture callback pointers and alloc/free for the closures
    auto* get_cb = callbacks->getParameterValues;
    auto* get_data = callbacks->getParameterValues_data;
    auto* set_cb = callbacks->setParameterValues;
    auto* set_data = callbacks->setParameterValues_data;
    auto* names_cb = callbacks->getParameterNames;
    auto* names_data = callbacks->getParameterNames_data;
    auto* add_cb = callbacks->AddTblRow;
    auto* add_data = callbacks->AddTblRow_data;
    auto* del_cb = callbacks->DeleteTblRow;
    auto* del_data = callbacks->DeleteTblRow_data;
    auto mfunc = mallocfunc;
    auto ffunc = freefunc;

    for (auto& [obj_path, obj] : objects) {
        usp::DataObject data_obj;
        std::string captured_path = obj_path;

        // Build the ObjectDef
        data_obj.def.path = obj_path;
        data_obj.def.is_multi_instance = obj.is_multi;
        data_obj.def.access = obj.is_multi
            ? usp::ObjectAccess::AddDelete
            : usp::ObjectAccess::ReadOnly;
        data_obj.def.params = obj.params;

        // --- on_get handler ---
        if (get_cb) {
            data_obj.on_get = [get_cb, get_data, mfunc, ffunc, captured_path](
                const usp::ObjectContext& ctx) -> usp::ParamMap
            {
                usp::ParamMap pmap;

                // Reconstruct the full concrete path from the object template + instances
                std::string concrete_path = captured_path;
                size_t inst_idx = 0;
                size_t pos = 0;
                while ((pos = concrete_path.find("{i}", pos)) != std::string::npos
                       && inst_idx < ctx.instances.size()) {
                    std::string inst_str = std::to_string(ctx.instances[inst_idx]);
                    concrete_path.replace(pos, 3, inst_str);
                    pos += inst_str.size();
                    inst_idx++;
                }

                char* names[1] = { const_cast<char*>(concrete_path.c_str()) };
                int val_size = 0;
                parameterValStruct_t** vals = nullptr;

                int ret = get_cb(0, names, 1, &val_size, &vals, get_data);
                if (ret == CCSP_SUCCESS && vals) {
                    for (int i = 0; i < val_size; i++) {
                        if (vals[i] && vals[i]->parameterName && vals[i]->parameterValue) {
                            std::string full_name = vals[i]->parameterName;
                            auto dot = full_name.rfind('.');
                            std::string pname = (dot != std::string::npos)
                                ? full_name.substr(dot + 1) : full_name;
                            pmap[pname] = vals[i]->parameterValue;
                        }
                        if (vals[i]) {
                            if (vals[i]->parameterName) ffunc(vals[i]->parameterName);
                            if (vals[i]->parameterValue) ffunc(vals[i]->parameterValue);
                            ffunc(vals[i]);
                        }
                    }
                    ffunc(vals);
                }
                return pmap;
            };
        }

        // --- on_set handler ---
        if (set_cb) {
            data_obj.on_set = [set_cb, set_data, mfunc, ffunc, captured_path, session](
                const usp::ObjectContext& ctx,
                const std::string& param,
                const std::string& value) -> usp::Status
            {
                std::string concrete_path = captured_path;
                size_t inst_idx = 0;
                size_t pos = 0;
                while ((pos = concrete_path.find("{i}", pos)) != std::string::npos
                       && inst_idx < ctx.instances.size()) {
                    std::string inst_str = std::to_string(ctx.instances[inst_idx]);
                    concrete_path.replace(pos, 3, inst_str);
                    pos += inst_str.size();
                    inst_idx++;
                }

                std::string full_path = concrete_path + param;

                parameterValStruct_t pv;
                pv.parameterName = const_cast<char*>(full_path.c_str());
                pv.parameterValue = const_cast<char*>(value.c_str());
                pv.type = ccsp_string;

                char* invalid_name = nullptr;
                int ret = set_cb(0, 0, &pv, 1, 1, &invalid_name, set_data);
                if (invalid_name) {
                    ffunc(invalid_name);
                }

                if (ret != CCSP_SUCCESS)
                    return usp::Status(ccsp_to_usp_error(ret), "set failed");

                if (session)
                    session->emit_value_change(full_path, value);

                return usp::Status();
            };
        }

        // --- on_instances handler (multi-instance objects only) ---
        if (obj.is_multi && names_cb) {
            data_obj.on_instances = [names_cb, names_data, mfunc, ffunc, captured_path](
                const usp::ObjectContext& ctx) -> std::vector<uint32_t>
            {
                std::vector<uint32_t> instances;

                std::string concrete_path = captured_path;
                size_t inst_idx = 0;
                size_t pos = 0;
                while ((pos = concrete_path.find("{i}", pos)) != std::string::npos
                       && inst_idx < ctx.instances.size()) {
                    std::string inst_str = std::to_string(ctx.instances[inst_idx]);
                    concrete_path.replace(pos, 3, inst_str);
                    pos += inst_str.size();
                    inst_idx++;
                }

                int info_size = 0;
                parameterInfoStruct_t** infos = nullptr;

                int ret = names_cb(const_cast<char*>(concrete_path.c_str()),
                                   1, // nextLevel = true
                                   &info_size, &infos, names_data);
                if (ret == CCSP_SUCCESS && infos) {
                    for (int i = 0; i < info_size; i++) {
                        if (infos[i] && infos[i]->parameterName) {
                            std::string name = infos[i]->parameterName;
                            if (!name.empty() && name.back() == '.')
                                name.pop_back();
                            auto dot = name.rfind('.');
                            if (dot != std::string::npos) {
                                std::string seg = name.substr(dot + 1);
                                char* end = nullptr;
                                unsigned long num = std::strtoul(seg.c_str(), &end, 10);
                                if (end != seg.c_str() && *end == '\0')
                                    instances.push_back(static_cast<uint32_t>(num));
                            }
                        }
                        if (infos[i]) {
                            if (infos[i]->parameterName) ffunc(infos[i]->parameterName);
                            ffunc(infos[i]);
                        }
                    }
                    ffunc(infos);
                }
                return instances;
            };
        }

        // --- on_add handler (multi-instance objects only) ---
        if (obj.is_multi && add_cb) {
            data_obj.on_add = [add_cb, add_data, captured_path, session](
                const usp::ObjectContext& ctx,
                const std::map<std::string, std::string>& /*params*/) -> usp::Result<uint32_t>
            {
                std::string concrete_path = captured_path;
                size_t inst_idx = 0;
                size_t pos = 0;
                while ((pos = concrete_path.find("{i}", pos)) != std::string::npos
                       && inst_idx < ctx.instances.size()) {
                    std::string inst_str = std::to_string(ctx.instances[inst_idx]);
                    concrete_path.replace(pos, 3, inst_str);
                    pos += inst_str.size();
                    inst_idx++;
                }

                int instance_number = 0;
                int ret = add_cb(0, const_cast<char*>(concrete_path.c_str()),
                                 &instance_number, add_data);
                if (ret != CCSP_SUCCESS)
                    return usp::Result<uint32_t>(ccsp_to_usp_error(ret), "add failed");

                if (session) {
                    std::string instance_path = concrete_path +
                        std::to_string(instance_number) + ".";
                    session->emit_object_creation(instance_path, {});
                }

                return usp::Result<uint32_t>(static_cast<uint32_t>(instance_number));
            };
        }

        // --- on_del handler (multi-instance objects only) ---
        if (obj.is_multi && del_cb) {
            data_obj.on_del = [del_cb, del_data, captured_path, session](
                const usp::ObjectContext& ctx) -> usp::Status
            {
                std::string concrete_path = captured_path;
                size_t inst_idx = 0;
                size_t pos = 0;
                while ((pos = concrete_path.find("{i}", pos)) != std::string::npos
                       && inst_idx < ctx.instances.size()) {
                    std::string inst_str = std::to_string(ctx.instances[inst_idx]);
                    concrete_path.replace(pos, 3, inst_str);
                    pos += inst_str.size();
                    inst_idx++;
                }

                std::string del_path = concrete_path;

                int ret = del_cb(0, const_cast<char*>(concrete_path.c_str()), del_data);
                if (ret != CCSP_SUCCESS)
                    return usp::Status(ccsp_to_usp_error(ret), "del failed");

                if (session)
                    session->emit_object_deletion(del_path);

                return usp::Status();
            };
        }

        // --- on_operate handler ---
        // CCSP uses set-parameter-to-trigger-action patterns rather than
        // explicit command dispatch. Return "not supported" by default.
        data_obj.on_operate = [](const usp::OperateRequest& req) -> usp::OperateResponse {
            usp::OperateResponse resp;
            resp.result = usp::OperateResponse::CommandFailure{
                usp::ErrorCode::OperateNotAllowed,
                "Command not supported via CCSP adapter: " + req.command
            };
            return resp;
        };

        registration.add(std::move(data_obj));
    }

    registration.allow_partial = true;
    return registration;
}
