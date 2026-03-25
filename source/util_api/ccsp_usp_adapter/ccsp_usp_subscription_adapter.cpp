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

#include "ccsp_usp_subscription_adapter.h"
#include "ccsp_usp_types.h"

#include <usp/controller.h>
#include <usp/types.h>
#include <usp/error.h>

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * Internal subscription record for re-subscribe on reconnection.
 */
struct SubscriptionRecord {
    std::string path;
    usp::NotificationType type;
    std::string usp_handle;
    CcspUspNotificationType ccsp_type;

    // One of these will be set depending on type
    CcspUspValueChangeCallback value_change_cb;
    CcspUspObjectCreationCallback object_creation_cb;
    CcspUspObjectDeletionCallback object_deletion_cb;
    CcspUspEventCallback event_cb;
    CcspUspOperationCompleteCallback op_complete_cb;
    void* user_data;
};

/**
 * Get the consumer handle from a bus_handle. This is shared with the consumer adapter
 * since subscriptions use the same ControllerSession.
 */
struct CcspUspConsumerHandle;

// Registry of active subscriptions for re-subscribe on reconnection
static std::mutex s_sub_mutex;
static std::unordered_map<std::string, SubscriptionRecord> s_subscriptions;
static uint64_t s_sub_counter = 0;

static std::string generate_sub_id()
{
    return "ccsp_sub_" + std::to_string(++s_sub_counter);
}

// Forward declaration: get ControllerSession from consumer handle
extern usp::ControllerSession* ccsp_usp_get_controller_session(void* bus_handle);

extern "C" {

int CcspUspAdapter_SubscribeValueChange(
    void* bus_handle,
    const char* path,
    CcspUspValueChangeCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle)
{
    if (!bus_handle || !path || !callback || !handle)
        return CCSP_FAILURE;

    auto* session = ccsp_usp_get_controller_session(bus_handle);
    if (!session)
        return CCSP_FAILURE;

    std::string sub_id = generate_sub_id();

    usp::SubscribeOptions opts;
    opts.paths = {path};
    opts.type = usp::NotificationType::ValueChange;
    opts.id = sub_id;

    // Capture callback and user_data for the notification handler
    auto cb = callback;
    auto ud = user_data;
    opts.on_notify = [cb, ud](const usp::Notification& notif) {
        cb(notif.param_path.c_str(), notif.param_value.c_str(), ud);
    };

    auto result = session->subscribe(opts);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    // Store subscription record for re-subscribe
    {
        std::lock_guard<std::mutex> lock(s_sub_mutex);
        SubscriptionRecord rec;
        rec.path = path;
        rec.type = usp::NotificationType::ValueChange;
        rec.usp_handle = result.value();
        rec.ccsp_type = CCSP_USP_NOTIFY_VALUE_CHANGE;
        rec.value_change_cb = callback;
        rec.user_data = user_data;
        s_subscriptions[sub_id] = std::move(rec);
    }

    // Return opaque handle (allocated copy of sub_id)
    char* handle_str = static_cast<char*>(malloc(sub_id.size() + 1));
    if (!handle_str)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;
    std::strcpy(handle_str, sub_id.c_str());
    *handle = handle_str;

    return CCSP_SUCCESS;
}

int CcspUspAdapter_SubscribeObjectCreation(
    void* bus_handle,
    const char* obj_path,
    CcspUspObjectCreationCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle)
{
    if (!bus_handle || !obj_path || !callback || !handle)
        return CCSP_FAILURE;

    auto* session = ccsp_usp_get_controller_session(bus_handle);
    if (!session)
        return CCSP_FAILURE;

    std::string sub_id = generate_sub_id();

    usp::SubscribeOptions opts;
    opts.paths = {obj_path};
    opts.type = usp::NotificationType::ObjectCreation;
    opts.id = sub_id;

    auto cb = callback;
    auto ud = user_data;
    opts.on_notify = [cb, ud](const usp::Notification& notif) {
        cb(notif.obj_path.c_str(), ud);
    };

    auto result = session->subscribe(opts);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    {
        std::lock_guard<std::mutex> lock(s_sub_mutex);
        SubscriptionRecord rec;
        rec.path = obj_path;
        rec.type = usp::NotificationType::ObjectCreation;
        rec.usp_handle = result.value();
        rec.ccsp_type = CCSP_USP_NOTIFY_OBJECT_CREATION;
        rec.object_creation_cb = callback;
        rec.user_data = user_data;
        s_subscriptions[sub_id] = std::move(rec);
    }

    char* handle_str = static_cast<char*>(malloc(sub_id.size() + 1));
    if (!handle_str)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;
    std::strcpy(handle_str, sub_id.c_str());
    *handle = handle_str;

    return CCSP_SUCCESS;
}

int CcspUspAdapter_SubscribeObjectDeletion(
    void* bus_handle,
    const char* obj_path,
    CcspUspObjectDeletionCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle)
{
    if (!bus_handle || !obj_path || !callback || !handle)
        return CCSP_FAILURE;

    auto* session = ccsp_usp_get_controller_session(bus_handle);
    if (!session)
        return CCSP_FAILURE;

    std::string sub_id = generate_sub_id();

    usp::SubscribeOptions opts;
    opts.paths = {obj_path};
    opts.type = usp::NotificationType::ObjectDeletion;
    opts.id = sub_id;

    auto cb = callback;
    auto ud = user_data;
    opts.on_notify = [cb, ud](const usp::Notification& notif) {
        cb(notif.obj_path.c_str(), ud);
    };

    auto result = session->subscribe(opts);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    {
        std::lock_guard<std::mutex> lock(s_sub_mutex);
        SubscriptionRecord rec;
        rec.path = obj_path;
        rec.type = usp::NotificationType::ObjectDeletion;
        rec.usp_handle = result.value();
        rec.ccsp_type = CCSP_USP_NOTIFY_OBJECT_DELETION;
        rec.object_deletion_cb = callback;
        rec.user_data = user_data;
        s_subscriptions[sub_id] = std::move(rec);
    }

    char* handle_str = static_cast<char*>(malloc(sub_id.size() + 1));
    if (!handle_str)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;
    std::strcpy(handle_str, sub_id.c_str());
    *handle = handle_str;

    return CCSP_SUCCESS;
}

int CcspUspAdapter_SubscribeEvent(
    void* bus_handle,
    const char* obj_path,
    const char* event_name,
    CcspUspEventCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle)
{
    if (!bus_handle || !obj_path || !callback || !handle)
        return CCSP_FAILURE;

    auto* session = ccsp_usp_get_controller_session(bus_handle);
    if (!session)
        return CCSP_FAILURE;

    std::string sub_id = generate_sub_id();

    usp::SubscribeOptions opts;
    opts.paths = {obj_path};
    opts.type = usp::NotificationType::Event;
    opts.id = sub_id;

    auto cb = callback;
    auto ud = user_data;
    opts.on_notify = [cb, ud](const usp::Notification& notif) {
        std::vector<const char*> keys;
        std::vector<const char*> values;
        for (auto& [k, v] : notif.event_params) {
            keys.push_back(k.c_str());
            values.push_back(v.c_str());
        }
        cb(notif.obj_path.c_str(), notif.event_name.c_str(),
           keys.data(), values.data(), static_cast<int>(keys.size()), ud);
    };

    auto result = session->subscribe(opts);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    {
        std::lock_guard<std::mutex> lock(s_sub_mutex);
        SubscriptionRecord rec;
        rec.path = obj_path;
        rec.type = usp::NotificationType::Event;
        rec.usp_handle = result.value();
        rec.ccsp_type = CCSP_USP_NOTIFY_EVENT;
        rec.event_cb = callback;
        rec.user_data = user_data;
        s_subscriptions[sub_id] = std::move(rec);
    }

    char* handle_str = static_cast<char*>(malloc(sub_id.size() + 1));
    if (!handle_str)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;
    std::strcpy(handle_str, sub_id.c_str());
    *handle = handle_str;

    return CCSP_SUCCESS;
}

int CcspUspAdapter_SubscribeOperationComplete(
    void* bus_handle,
    const char* command_path,
    CcspUspOperationCompleteCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle)
{
    if (!bus_handle || !command_path || !callback || !handle)
        return CCSP_FAILURE;

    auto* session = ccsp_usp_get_controller_session(bus_handle);
    if (!session)
        return CCSP_FAILURE;

    std::string sub_id = generate_sub_id();

    usp::SubscribeOptions opts;
    opts.paths = {command_path};
    opts.type = usp::NotificationType::OperationComplete;
    opts.id = sub_id;

    auto cb = callback;
    auto ud = user_data;
    opts.on_notify = [cb, ud](const usp::Notification& notif) {
        std::vector<const char*> keys;
        std::vector<const char*> values;
        for (auto& [k, v] : notif.output_args) {
            keys.push_back(k.c_str());
            values.push_back(v.c_str());
        }
        int success = (notif.op_err_code == usp::ErrorCode::Ok) ? 1 : 0;
        cb(notif.obj_path.c_str(), notif.command_name.c_str(),
           notif.command_key.c_str(), success,
           keys.data(), values.data(), static_cast<int>(keys.size()),
           static_cast<unsigned int>(notif.op_err_code),
           notif.op_err_msg.c_str(), ud);
    };

    auto result = session->subscribe(opts);
    if (!result)
        return ccsp_usp_error_to_ccsp(result.error_code());

    {
        std::lock_guard<std::mutex> lock(s_sub_mutex);
        SubscriptionRecord rec;
        rec.path = command_path;
        rec.type = usp::NotificationType::OperationComplete;
        rec.usp_handle = result.value();
        rec.ccsp_type = CCSP_USP_NOTIFY_OPERATION_COMPLETE;
        rec.op_complete_cb = callback;
        rec.user_data = user_data;
        s_subscriptions[sub_id] = std::move(rec);
    }

    char* handle_str = static_cast<char*>(malloc(sub_id.size() + 1));
    if (!handle_str)
        return CCSP_ERR_MEMORY_ALLOC_FAIL;
    std::strcpy(handle_str, sub_id.c_str());
    *handle = handle_str;

    return CCSP_SUCCESS;
}

int CcspUspAdapter_Unsubscribe(
    void* bus_handle,
    CcspUspSubscriptionHandle handle)
{
    if (!bus_handle || !handle)
        return CCSP_FAILURE;

    auto* session = ccsp_usp_get_controller_session(bus_handle);
    if (!session)
        return CCSP_FAILURE;

    char* sub_id_str = static_cast<char*>(handle);
    std::string sub_id(sub_id_str);

    std::string usp_handle;
    {
        std::lock_guard<std::mutex> lock(s_sub_mutex);
        auto it = s_subscriptions.find(sub_id);
        if (it == s_subscriptions.end())
            return CCSP_ERR_NOT_EXIST;
        usp_handle = it->second.usp_handle;
        s_subscriptions.erase(it);
    }

    auto status = session->unsubscribe(usp_handle);
    free(sub_id_str);

    return status.ok() ? CCSP_SUCCESS : ccsp_usp_error_to_ccsp(status.error_code());
}

} // extern "C"

// Called by consumer adapter on reconnection to re-subscribe all active subscriptions
void ccsp_usp_resubscribe_all(usp::ControllerSession* session)
{
    std::lock_guard<std::mutex> lock(s_sub_mutex);
    for (auto& [sub_id, rec] : s_subscriptions) {
        usp::SubscribeOptions opts;
        opts.paths = {rec.path};
        opts.type = rec.type;
        opts.id = sub_id;

        switch (rec.ccsp_type) {
        case CCSP_USP_NOTIFY_VALUE_CHANGE: {
            auto cb = rec.value_change_cb;
            auto ud = rec.user_data;
            opts.on_notify = [cb, ud](const usp::Notification& notif) {
                cb(notif.param_path.c_str(), notif.param_value.c_str(), ud);
            };
            break;
        }
        case CCSP_USP_NOTIFY_OBJECT_CREATION: {
            auto cb = rec.object_creation_cb;
            auto ud = rec.user_data;
            opts.on_notify = [cb, ud](const usp::Notification& notif) {
                cb(notif.obj_path.c_str(), ud);
            };
            break;
        }
        case CCSP_USP_NOTIFY_OBJECT_DELETION: {
            auto cb = rec.object_deletion_cb;
            auto ud = rec.user_data;
            opts.on_notify = [cb, ud](const usp::Notification& notif) {
                cb(notif.obj_path.c_str(), ud);
            };
            break;
        }
        case CCSP_USP_NOTIFY_EVENT: {
            auto cb = rec.event_cb;
            auto ud = rec.user_data;
            opts.on_notify = [cb, ud](const usp::Notification& notif) {
                std::vector<const char*> keys;
                std::vector<const char*> values;
                for (auto& [k, v] : notif.event_params) {
                    keys.push_back(k.c_str());
                    values.push_back(v.c_str());
                }
                cb(notif.obj_path.c_str(), notif.event_name.c_str(),
                   keys.data(), values.data(), static_cast<int>(keys.size()), ud);
            };
            break;
        }
        case CCSP_USP_NOTIFY_OPERATION_COMPLETE: {
            auto cb = rec.op_complete_cb;
            auto ud = rec.user_data;
            opts.on_notify = [cb, ud](const usp::Notification& notif) {
                std::vector<const char*> keys;
                std::vector<const char*> values;
                for (auto& [k, v] : notif.output_args) {
                    keys.push_back(k.c_str());
                    values.push_back(v.c_str());
                }
                int success = (notif.op_err_code == usp::ErrorCode::Ok) ? 1 : 0;
                cb(notif.obj_path.c_str(), notif.command_name.c_str(),
                   notif.command_key.c_str(), success,
                   keys.data(), values.data(), static_cast<int>(keys.size()),
                   static_cast<unsigned int>(notif.op_err_code),
                   notif.op_err_msg.c_str(), ud);
            };
            break;
        }
        }

        auto result = session->subscribe(opts);
        if (result)
            rec.usp_handle = result.value();
    }
}
