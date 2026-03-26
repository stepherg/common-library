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

#ifndef CCSP_USP_TYPES_H
#define CCSP_USP_TYPES_H

#ifdef __cplusplus

#include "ccsp_base_api.h"
#include <usp/error.h>
#include <usp/schema.h>
#include <usp/agent.h>
#include <usp/controller.h>
#include <usp/types.h>

#include <map>
#include <string>
#include <tuple>
#include <vector>

/**
 * Map CCSP dataType_e to usp::ValueType.
 */
inline usp::ValueType ccsp_datatype_to_usp(enum dataType_e type)
{
    switch (type) {
    case ccsp_string:       return usp::ValueType::String;
    case ccsp_int:          return usp::ValueType::Int;
    case ccsp_unsignedInt:  return usp::ValueType::UnsignedInt;
    case ccsp_boolean:      return usp::ValueType::Boolean;
    case ccsp_dateTime:     return usp::ValueType::DateTime;
    case ccsp_base64:       return usp::ValueType::Base64;
    case ccsp_long:         return usp::ValueType::Long;
    case ccsp_unsignedLong: return usp::ValueType::UnsignedLong;
    case ccsp_float:        return usp::ValueType::Decimal;
    case ccsp_double:       return usp::ValueType::Decimal;
    case ccsp_byte:         return usp::ValueType::HexBinary;
    case ccsp_none:         return usp::ValueType::String;
    default:                return usp::ValueType::Unknown;
    }
}

/**
 * Map usp::ValueType to CCSP dataType_e.
 */
inline enum dataType_e usp_valuetype_to_ccsp(usp::ValueType type)
{
    switch (type) {
    case usp::ValueType::String:       return ccsp_string;
    case usp::ValueType::Int:          return ccsp_int;
    case usp::ValueType::UnsignedInt:  return ccsp_unsignedInt;
    case usp::ValueType::Boolean:      return ccsp_boolean;
    case usp::ValueType::DateTime:     return ccsp_dateTime;
    case usp::ValueType::Base64:       return ccsp_base64;
    case usp::ValueType::Long:         return ccsp_long;
    case usp::ValueType::UnsignedLong: return ccsp_unsignedLong;
    case usp::ValueType::Decimal:      return ccsp_float;
    case usp::ValueType::HexBinary:    return ccsp_byte;
    case usp::ValueType::Unknown:      return ccsp_string;
    default:                           return ccsp_string;
    }
}

/**
 * Map CCSP access_e to usp::ParamAccess.
 */
inline usp::ParamAccess ccsp_access_to_usp(enum access_e access)
{
    switch (access) {
    case CCSP_RO: return usp::ParamAccess::ReadOnly;
    case CCSP_RW: return usp::ParamAccess::ReadWrite;
    case CCSP_WO: return usp::ParamAccess::WriteOnly;
    default:      return usp::ParamAccess::ReadOnly;
    }
}

/**
 * Map usp::ErrorCode to CCSP error code.
 */
inline int ccsp_usp_error_to_ccsp(usp::ErrorCode code)
{
    switch (code) {
    case usp::ErrorCode::Ok:               return CCSP_SUCCESS;
    case usp::ErrorCode::GeneralFailure:   return CCSP_FAILURE;
    case usp::ErrorCode::InvalidArgument:  return CCSP_ERR_INVALID_PARAMETER_VALUE;
    case usp::ErrorCode::ReadOnlyViolation:return CCSP_ERR_NOT_WRITABLE;
    case usp::ErrorCode::AddNotAllowed:    return CCSP_ERR_NOT_SUPPORT;
    case usp::ErrorCode::OperateNotAllowed:return CCSP_ERR_METHOD_NOT_SUPPORTED;
    case usp::ErrorCode::UnknownParam:     return CCSP_ERR_INVALID_PARAMETER_NAME;
    case usp::ErrorCode::InvalidPath:      return CCSP_ERR_INVALID_PARAMETER_NAME;
    case usp::ErrorCode::Timeout:          return CCSP_ERR_TIMEOUT;
    case usp::ErrorCode::NotConnected:     return CCSP_ERR_NOT_CONNECT;
    case usp::ErrorCode::TransportError:   return CCSP_ERR_NOT_CONNECT;
    case usp::ErrorCode::DecodeError:      return CCSP_ERR_INTERNAL_ERROR;
    case usp::ErrorCode::Reconnecting:     return CCSP_ERR_NOT_CONNECT;
    case usp::ErrorCode::Reentrant:        return CCSP_ERR_REQUEST_REJECTED;
    default:                               return CCSP_FAILURE;
    }
}

/**
 * Map CCSP error code to usp::ErrorCode.
 */
inline usp::ErrorCode ccsp_to_usp_error(int ccsp_code)
{
    switch (ccsp_code) {
    case CCSP_SUCCESS:                          return usp::ErrorCode::Ok;
    case CCSP_FAILURE:                          return usp::ErrorCode::GeneralFailure;
    case CCSP_ERR_MEMORY_ALLOC_FAIL:            return usp::ErrorCode::GeneralFailure;
    case CCSP_ERR_NOT_CONNECT:                  return usp::ErrorCode::NotConnected;
    case CCSP_ERR_TIMEOUT:                      return usp::ErrorCode::Timeout;
    case CCSP_ERR_NOT_EXIST:                    return usp::ErrorCode::UnknownParam;
    case CCSP_ERR_NOT_SUPPORT:                  return usp::ErrorCode::OperateNotAllowed;
    case CCSP_ERR_INVALID_PARAMETER_NAME:       return usp::ErrorCode::InvalidPath;
    case CCSP_ERR_INVALID_PARAMETER_TYPE:       return usp::ErrorCode::InvalidArgument;
    case CCSP_ERR_INVALID_PARAMETER_VALUE:      return usp::ErrorCode::InvalidArgument;
    case CCSP_ERR_NOT_WRITABLE:                 return usp::ErrorCode::ReadOnlyViolation;
    default:                                    return usp::ErrorCode::GeneralFailure;
    }
}

/**
 * Parse a namespace path to extract the object prefix and parameter name.
 * e.g., "Device.WiFi.SSID.{i}.SSID" -> ("Device.WiFi.SSID.{i}.", "SSID")
 */
inline std::pair<std::string, std::string> split_param_path(const std::string& full_path)
{
    // Find the last dot before the parameter name
    auto last_dot = full_path.rfind('.');
    if (last_dot == std::string::npos || last_dot == 0)
        return {full_path, ""};

    // Check if it ends with a dot (object path, not parameter)
    if (last_dot == full_path.size() - 1)
        return {full_path, ""};

    return {full_path.substr(0, last_dot + 1), full_path.substr(last_dot + 1)};
}

/**
 * Build schema and object handlers from CCSP namespace array and callback table.
 *
 * Returns a vector of (root_path, Schema, map<obj_path, ObjectHandlers>) for
 * registration via AgentSession::provide() and handle().
 */
using SchemaHandlerEntry = std::tuple<
    std::string,
    usp::Schema,
    std::vector<std::pair<std::string, usp::ObjectHandlers>>
>;

std::vector<SchemaHandlerEntry> ccsp_usp_build_schema_and_handlers(
    name_spaceType_t* name_space,
    int size,
    CCSP_Base_Func_CB* callbacks,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE freefunc,
    usp::AgentSession* session = nullptr
);

/**
 * Get the ControllerSession from a consumer handle.
 * Used by the subscription adapter.
 */
usp::ControllerSession* ccsp_usp_get_controller_session(void* bus_handle);

/**
 * Re-subscribe all active subscriptions after reconnection.
 */
void ccsp_usp_resubscribe_all(usp::ControllerSession* session);

/**
 * Build a USP endpoint ID by prepending the configured prefix to a component ID.
 */
std::string ccsp_usp_make_endpoint_id(const std::string& component_id);

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set the prefix prepended to component_id when constructing USP endpoint IDs.
 * Must be called before CcspUspAdapter_ProviderInit / ConsumerInit.
 * Pass NULL or "" to disable prefixing.
 *
 * Default: "rbus-ctrl::"
 */
void CcspUspAdapter_SetEndpointIdPrefix(const char* prefix);

/**
 * Get the current endpoint ID prefix (never returns NULL).
 */
const char* CcspUspAdapter_GetEndpointIdPrefix(void);

#ifdef __cplusplus
}
#endif

#endif /* CCSP_USP_TYPES_H */
