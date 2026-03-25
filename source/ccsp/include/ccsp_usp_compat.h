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
 * Compatibility header for downstream components that still include
 * legacy ccsp_rbus_value_change.h or ccsp_rbus_intervalsubscription.h.
 *
 * These functions are deprecated — use the CcspUspAdapter_Subscribe* APIs
 * from ccsp_usp_subscription_adapter.h instead.
 *
 * Include this header OR the legacy headers — both will compile, but the
 * legacy functions will return CCSP_ERR_NOT_SUPPORT at runtime.
 */

#ifndef CCSP_USP_COMPAT_H
#define CCSP_USP_COMPAT_H

#include "ccsp_base_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @deprecated Use CcspUspAdapter_SubscribeValueChange() instead.
 */
static inline int Ccsp_RbusValueChange_Subscribe(
    void* handle, const char* listener, const char* parameter,
    const char* eventName, int componentId, int interval,
    int duration, void* filter)
{
    (void)handle; (void)listener; (void)parameter;
    (void)eventName; (void)componentId; (void)interval;
    (void)duration; (void)filter;
    return CCSP_ERR_NOT_SUPPORT;
}

/**
 * @deprecated Use CcspUspAdapter_Unsubscribe() instead.
 */
static inline int Ccsp_RbusValueChange_Unsubscribe(
    void* handle, const char* listener, const char* parameter,
    const char* eventName, int componentId, void* filter)
{
    (void)handle; (void)listener; (void)parameter;
    (void)eventName; (void)componentId; (void)filter;
    return CCSP_ERR_NOT_SUPPORT;
}

/**
 * @deprecated No longer needed.
 */
static inline int Ccsp_RbusValueChange_Close(void* handle)
{
    (void)handle;
    return CCSP_ERR_NOT_SUPPORT;
}

/**
 * @deprecated Use CcspUspAdapter_SubscribeValueChange() with interval options.
 */
static inline int Ccsp_RbusInterval_Subscribe(
    void* handle, const char* listener, const char* parameter,
    int componentId, int interval, int duration, void* filter)
{
    (void)handle; (void)listener; (void)parameter;
    (void)componentId; (void)interval; (void)duration; (void)filter;
    return CCSP_ERR_NOT_SUPPORT;
}

/**
 * @deprecated Use CcspUspAdapter_Unsubscribe() instead.
 */
static inline int Ccsp_RbusInterval_Unsubscribe(
    void* handle, const char* listener, const char* parameter,
    int componentId, int interval, int duration, void* filter)
{
    (void)handle; (void)listener; (void)parameter;
    (void)componentId; (void)interval; (void)duration; (void)filter;
    return CCSP_ERR_NOT_SUPPORT;
}

#ifdef __cplusplus
}
#endif

#endif /* CCSP_USP_COMPAT_H */
