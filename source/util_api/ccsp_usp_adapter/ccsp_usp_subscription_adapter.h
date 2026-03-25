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

#ifndef CCSP_USP_SUBSCRIPTION_ADAPTER_H
#define CCSP_USP_SUBSCRIPTION_ADAPTER_H

#include "ccsp_base_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Subscription notification types (mirrors usp::NotificationType).
 */
typedef enum {
    CCSP_USP_NOTIFY_VALUE_CHANGE = 0,
    CCSP_USP_NOTIFY_OBJECT_CREATION,
    CCSP_USP_NOTIFY_OBJECT_DELETION,
    CCSP_USP_NOTIFY_EVENT,
    CCSP_USP_NOTIFY_OPERATION_COMPLETE
} CcspUspNotificationType;

/**
 * Callback for value change notifications.
 *
 * @param param_path    Full parameter path that changed
 * @param param_value   New value as string
 * @param user_data     User-provided context
 */
typedef void (*CcspUspValueChangeCallback)(
    const char* param_path,
    const char* param_value,
    void* user_data
);

/**
 * Callback for object creation notifications.
 *
 * @param obj_path      Path of created object instance
 * @param user_data     User-provided context
 */
typedef void (*CcspUspObjectCreationCallback)(
    const char* obj_path,
    void* user_data
);

/**
 * Callback for object deletion notifications.
 *
 * @param obj_path      Path of deleted object instance
 * @param user_data     User-provided context
 */
typedef void (*CcspUspObjectDeletionCallback)(
    const char* obj_path,
    void* user_data
);

/**
 * Callback for custom event notifications.
 *
 * @param obj_path      Object path the event was emitted on
 * @param event_name    Event name
 * @param param_keys    Array of event parameter names
 * @param param_values  Array of event parameter values
 * @param param_count   Number of event parameters
 * @param user_data     User-provided context
 */
typedef void (*CcspUspEventCallback)(
    const char* obj_path,
    const char* event_name,
    const char** param_keys,
    const char** param_values,
    int param_count,
    void* user_data
);

/**
 * Callback for operation complete notifications.
 *
 * @param obj_path      Object path
 * @param command_name  Command that completed
 * @param command_key   Command key for correlation
 * @param success       1 if success, 0 if failure
 * @param output_keys   Output argument names (NULL on failure)
 * @param output_values Output argument values (NULL on failure)
 * @param output_count  Number of output arguments
 * @param err_code      Error code (0 on success)
 * @param err_msg       Error message (NULL on success)
 * @param user_data     User-provided context
 */
typedef void (*CcspUspOperationCompleteCallback)(
    const char* obj_path,
    const char* command_name,
    const char* command_key,
    int success,
    const char** output_keys,
    const char** output_values,
    int output_count,
    unsigned int err_code,
    const char* err_msg,
    void* user_data
);

/**
 * Opaque subscription handle.
 */
typedef void* CcspUspSubscriptionHandle;

/**
 * Subscribe to value change notifications on a parameter path.
 *
 * @param bus_handle    Consumer session handle
 * @param path          Parameter path to monitor
 * @param callback      Function called on value change
 * @param user_data     User context passed to callback
 * @param handle        [out] Subscription handle for unsubscribe
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_SubscribeValueChange(
    void* bus_handle,
    const char* path,
    CcspUspValueChangeCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle
);

/**
 * Subscribe to object creation notifications.
 *
 * @param bus_handle    Consumer session handle
 * @param obj_path      Multi-instance object path to monitor
 * @param callback      Function called on instance creation
 * @param user_data     User context
 * @param handle        [out] Subscription handle
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_SubscribeObjectCreation(
    void* bus_handle,
    const char* obj_path,
    CcspUspObjectCreationCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle
);

/**
 * Subscribe to object deletion notifications.
 *
 * @param bus_handle    Consumer session handle
 * @param obj_path      Multi-instance object path to monitor
 * @param callback      Function called on instance deletion
 * @param user_data     User context
 * @param handle        [out] Subscription handle
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_SubscribeObjectDeletion(
    void* bus_handle,
    const char* obj_path,
    CcspUspObjectDeletionCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle
);

/**
 * Subscribe to custom event notifications.
 *
 * @param bus_handle    Consumer session handle
 * @param obj_path      Object path to listen on
 * @param event_name    Event name filter (or NULL for all events)
 * @param callback      Function called on event
 * @param user_data     User context
 * @param handle        [out] Subscription handle
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_SubscribeEvent(
    void* bus_handle,
    const char* obj_path,
    const char* event_name,
    CcspUspEventCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle
);

/**
 * Subscribe to operation complete notifications.
 *
 * @param bus_handle    Consumer session handle
 * @param command_path  Command path to monitor
 * @param callback      Function called on operation complete
 * @param user_data     User context
 * @param handle        [out] Subscription handle
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_SubscribeOperationComplete(
    void* bus_handle,
    const char* command_path,
    CcspUspOperationCompleteCallback callback,
    void* user_data,
    CcspUspSubscriptionHandle* handle
);

/**
 * Unsubscribe from any notification type.
 *
 * @param bus_handle    Consumer session handle
 * @param handle        Subscription handle from subscribe call
 * @return CCSP_SUCCESS or CCSP error code
 */
int CcspUspAdapter_Unsubscribe(
    void* bus_handle,
    CcspUspSubscriptionHandle handle
);

#ifdef __cplusplus
}
#endif

#endif /* CCSP_USP_SUBSCRIPTION_ADAPTER_H */
