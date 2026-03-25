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
 * Integration tests for the USP adapter layer.
 *
 * These tests verify that the adapter functions correctly translate between
 * CCSP API semantics and USP library calls. They require a running USP broker
 * or mock transport to pass end-to-end.
 *
 * For unit testing without a broker, mock the AgentSession/ControllerSession
 * create() calls.
 */

#include "gtest/gtest.h"

extern "C" {
#include "ccsp_base_api.h"
#include "ccsp_usp_provider_adapter.h"
#include "ccsp_usp_consumer_adapter.h"
#include "ccsp_usp_subscription_adapter.h"
}

#include "ccsp_usp_types.h"

#include <cstring>
#include <string>
#include <vector>

/**
 * Test fixture for USP adapter tests.
 * Note: These tests verify API surface correctness and error handling.
 * Full integration tests require a USP broker endpoint.
 */
class UspAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/* ===== Type Mapping Tests ===== */

TEST_F(UspAdapterTest, DataTypeMapping_CcspToUsp)
{
    EXPECT_EQ(usp::ValueType::String, ccsp_datatype_to_usp(ccsp_string));
    EXPECT_EQ(usp::ValueType::Int, ccsp_datatype_to_usp(ccsp_int));
    EXPECT_EQ(usp::ValueType::UnsignedInt, ccsp_datatype_to_usp(ccsp_unsignedInt));
    EXPECT_EQ(usp::ValueType::Boolean, ccsp_datatype_to_usp(ccsp_boolean));
    EXPECT_EQ(usp::ValueType::DateTime, ccsp_datatype_to_usp(ccsp_dateTime));
    EXPECT_EQ(usp::ValueType::Base64, ccsp_datatype_to_usp(ccsp_base64));
    EXPECT_EQ(usp::ValueType::Long, ccsp_datatype_to_usp(ccsp_long));
    EXPECT_EQ(usp::ValueType::UnsignedLong, ccsp_datatype_to_usp(ccsp_unsignedLong));
    EXPECT_EQ(usp::ValueType::Decimal, ccsp_datatype_to_usp(ccsp_float));
    EXPECT_EQ(usp::ValueType::Decimal, ccsp_datatype_to_usp(ccsp_double));
    EXPECT_EQ(usp::ValueType::HexBinary, ccsp_datatype_to_usp(ccsp_byte));
    EXPECT_EQ(usp::ValueType::String, ccsp_datatype_to_usp(ccsp_none));
}

TEST_F(UspAdapterTest, DataTypeMapping_UspToCcsp)
{
    EXPECT_EQ(ccsp_string, usp_valuetype_to_ccsp(usp::ValueType::String));
    EXPECT_EQ(ccsp_int, usp_valuetype_to_ccsp(usp::ValueType::Int));
    EXPECT_EQ(ccsp_unsignedInt, usp_valuetype_to_ccsp(usp::ValueType::UnsignedInt));
    EXPECT_EQ(ccsp_boolean, usp_valuetype_to_ccsp(usp::ValueType::Boolean));
    EXPECT_EQ(ccsp_dateTime, usp_valuetype_to_ccsp(usp::ValueType::DateTime));
    EXPECT_EQ(ccsp_base64, usp_valuetype_to_ccsp(usp::ValueType::Base64));
    EXPECT_EQ(ccsp_long, usp_valuetype_to_ccsp(usp::ValueType::Long));
    EXPECT_EQ(ccsp_unsignedLong, usp_valuetype_to_ccsp(usp::ValueType::UnsignedLong));
    EXPECT_EQ(ccsp_float, usp_valuetype_to_ccsp(usp::ValueType::Decimal));
    EXPECT_EQ(ccsp_byte, usp_valuetype_to_ccsp(usp::ValueType::HexBinary));
    EXPECT_EQ(ccsp_string, usp_valuetype_to_ccsp(usp::ValueType::Unknown));
}

/* ===== Error Code Mapping Tests ===== */

TEST_F(UspAdapterTest, ErrorMapping_UspToCcsp)
{
    EXPECT_EQ(CCSP_SUCCESS, ccsp_usp_error_to_ccsp(usp::ErrorCode::Ok));
    EXPECT_EQ(CCSP_FAILURE, ccsp_usp_error_to_ccsp(usp::ErrorCode::GeneralFailure));
    EXPECT_EQ(CCSP_ERR_INVALID_PARAMETER_VALUE,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::InvalidArgument));
    EXPECT_EQ(CCSP_ERR_NOT_WRITABLE,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::ReadOnlyViolation));
    EXPECT_EQ(CCSP_ERR_NOT_SUPPORT,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::AddNotAllowed));
    EXPECT_EQ(CCSP_ERR_METHOD_NOT_SUPPORTED,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::OperateNotAllowed));
    EXPECT_EQ(CCSP_ERR_INVALID_PARAMETER_NAME,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::UnknownParam));
    EXPECT_EQ(CCSP_ERR_INVALID_PARAMETER_NAME,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::InvalidPath));
    EXPECT_EQ(CCSP_ERR_TIMEOUT,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::Timeout));
    EXPECT_EQ(CCSP_ERR_NOT_CONNECT,
              ccsp_usp_error_to_ccsp(usp::ErrorCode::NotConnected));
}

TEST_F(UspAdapterTest, ErrorMapping_CcspToUsp)
{
    EXPECT_EQ(usp::ErrorCode::Ok, ccsp_to_usp_error(CCSP_SUCCESS));
    EXPECT_EQ(usp::ErrorCode::GeneralFailure, ccsp_to_usp_error(CCSP_FAILURE));
    EXPECT_EQ(usp::ErrorCode::NotConnected, ccsp_to_usp_error(CCSP_ERR_NOT_CONNECT));
    EXPECT_EQ(usp::ErrorCode::Timeout, ccsp_to_usp_error(CCSP_ERR_TIMEOUT));
    EXPECT_EQ(usp::ErrorCode::UnknownParam, ccsp_to_usp_error(CCSP_ERR_NOT_EXIST));
    EXPECT_EQ(usp::ErrorCode::InvalidPath,
              ccsp_to_usp_error(CCSP_ERR_INVALID_PARAMETER_NAME));
    EXPECT_EQ(usp::ErrorCode::InvalidArgument,
              ccsp_to_usp_error(CCSP_ERR_INVALID_PARAMETER_VALUE));
    EXPECT_EQ(usp::ErrorCode::ReadOnlyViolation,
              ccsp_to_usp_error(CCSP_ERR_NOT_WRITABLE));
}

/* ===== Access Mapping Tests ===== */

TEST_F(UspAdapterTest, AccessMapping_CcspToUsp)
{
    EXPECT_EQ(usp::ParamAccess::ReadOnly, ccsp_access_to_usp(CCSP_RO));
    EXPECT_EQ(usp::ParamAccess::ReadWrite, ccsp_access_to_usp(CCSP_RW));
    EXPECT_EQ(usp::ParamAccess::WriteOnly, ccsp_access_to_usp(CCSP_WO));
}

/* ===== Path Splitting Tests ===== */

TEST_F(UspAdapterTest, SplitParamPath_Parameter)
{
    auto [obj, param] = split_param_path("Device.WiFi.SSID.{i}.SSID");
    EXPECT_EQ("Device.WiFi.SSID.{i}.", obj);
    EXPECT_EQ("SSID", param);
}

TEST_F(UspAdapterTest, SplitParamPath_ObjectPath)
{
    auto [obj, param] = split_param_path("Device.WiFi.SSID.{i}.");
    EXPECT_EQ("Device.WiFi.SSID.{i}.", obj);
    EXPECT_EQ("", param);
}

TEST_F(UspAdapterTest, SplitParamPath_SingletonParam)
{
    auto [obj, param] = split_param_path("Device.DeviceInfo.ModelName");
    EXPECT_EQ("Device.DeviceInfo.", obj);
    EXPECT_EQ("ModelName", param);
}

/* ===== Provider Adapter NULL-safety Tests ===== */

TEST_F(UspAdapterTest, ProviderInit_NullComponentId)
{
    void* handle = nullptr;
    int ret = CcspUspAdapter_ProviderInit(nullptr, nullptr, &handle, nullptr, nullptr);
    EXPECT_EQ(CCSP_FAILURE, ret);
    EXPECT_EQ(nullptr, handle);
}

TEST_F(UspAdapterTest, ProviderInit_NullBusHandle)
{
    int ret = CcspUspAdapter_ProviderInit((char*)"test", nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

TEST_F(UspAdapterTest, ProviderExit_NullHandle)
{
    /* Should not crash */
    CcspUspAdapter_ProviderExit(nullptr);
}

TEST_F(UspAdapterTest, SetCallback_NullHandle)
{
    CCSP_Base_Func_CB cb;
    memset(&cb, 0, sizeof(cb));
    /* Should not crash */
    CcspUspAdapter_SetCallback(nullptr, &cb);
}

/* ===== Consumer Adapter NULL-safety Tests ===== */

TEST_F(UspAdapterTest, ConsumerInit_NullComponentId)
{
    void* handle = nullptr;
    int ret = CcspUspAdapter_ConsumerInit(nullptr, nullptr, &handle, nullptr, nullptr);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

TEST_F(UspAdapterTest, ConsumerExit_NullHandle)
{
    CcspUspAdapter_ConsumerExit(nullptr);
}

TEST_F(UspAdapterTest, GetParameterValues_NullHandle)
{
    int val_size = 0;
    parameterValStruct_t** val = nullptr;
    char* names[] = { (char*)"Device.Test." };
    int ret = CcspUspAdapter_GetParameterValues(
        nullptr, nullptr, nullptr, names, 1, &val_size, &val);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

TEST_F(UspAdapterTest, SetParameterValues_NullHandle)
{
    parameterValStruct_t pv;
    pv.parameterName = (char*)"Device.Test.Param";
    pv.parameterValue = (char*)"value";
    pv.type = ccsp_string;
    int ret = CcspUspAdapter_SetParameterValues(
        nullptr, nullptr, nullptr, 0, 0, &pv, 1, 1, nullptr);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

TEST_F(UspAdapterTest, AddTblRow_NullHandle)
{
    int instanceNumber = 0;
    int ret = CcspUspAdapter_AddTblRow(
        nullptr, nullptr, nullptr, 0, (char*)"Device.Test.", &instanceNumber);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

TEST_F(UspAdapterTest, DeleteTblRow_NullHandle)
{
    int ret = CcspUspAdapter_DeleteTblRow(
        nullptr, nullptr, nullptr, 0, (char*)"Device.Test.1.");
    EXPECT_EQ(CCSP_FAILURE, ret);
}

/* ===== Subscription Adapter NULL-safety Tests ===== */

TEST_F(UspAdapterTest, SubscribeValueChange_NullHandle)
{
    CcspUspSubscriptionHandle sub = nullptr;
    int ret = CcspUspAdapter_SubscribeValueChange(
        nullptr, "Device.Test.Param",
        [](const char*, const char*, void*){}, nullptr, &sub);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

TEST_F(UspAdapterTest, SubscribeObjectCreation_NullHandle)
{
    CcspUspSubscriptionHandle sub = nullptr;
    int ret = CcspUspAdapter_SubscribeObjectCreation(
        nullptr, "Device.Test.",
        [](const char*, void*){}, nullptr, &sub);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

TEST_F(UspAdapterTest, Unsubscribe_NullHandle)
{
    int ret = CcspUspAdapter_Unsubscribe(nullptr, nullptr);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

/* ===== Discovery Stub Tests ===== */

TEST_F(UspAdapterTest, DiscComponent_NullHandle)
{
    componentStruct_t** components = nullptr;
    int size = 0;
    int ret = CcspUspAdapter_DiscComponentSupportingNamespace(
        nullptr, nullptr, "Device.Test.", nullptr, &components, &size);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

/* ===== Session ID Stub Tests ===== */

TEST_F(UspAdapterTest, RequestSessionID_NullHandle)
{
    int sessionID = -1;
    int ret = CcspUspAdapter_RequestSessionID(nullptr, nullptr, 0, &sessionID);
    EXPECT_EQ(CCSP_SUCCESS, ret);
    EXPECT_EQ(0, sessionID);
}

TEST_F(UspAdapterTest, InformEndOfSession_NullHandle)
{
    int ret = CcspUspAdapter_InformEndOfSession(nullptr, nullptr, 0);
    EXPECT_EQ(CCSP_SUCCESS, ret);
}

/* ===== RegisterBase Stub Test ===== */

TEST_F(UspAdapterTest, RegisterBase_NullHandle)
{
    int ret = CcspUspAdapter_RegisterBase(
        nullptr, "cr", "component", 1, "/path", "subsys");
    EXPECT_EQ(CCSP_FAILURE, ret);
}

/* ===== RegisterCapabilities NULL Test ===== */

TEST_F(UspAdapterTest, RegisterCapabilities_NullHandle)
{
    name_spaceType_t ns;
    ns.name_space = (char*)"Device.Test.Param";
    ns.dataType = ccsp_string;
    int ret = CcspUspAdapter_RegisterCapabilities(
        nullptr, "cr", "component", 1, "/path", "subsys", &ns, 1);
    EXPECT_EQ(CCSP_FAILURE, ret);
}

/* ===== Schema Builder Tests ===== */

TEST_F(UspAdapterTest, SchemaBuilder_NullNamespace)
{
    CCSP_Base_Func_CB cb;
    memset(&cb, 0, sizeof(cb));
    auto result = ccsp_usp_build_schema_and_handlers(
        nullptr, 0, &cb, malloc, free);
    EXPECT_TRUE(result.empty());
}

TEST_F(UspAdapterTest, SchemaBuilder_EmptySize)
{
    name_spaceType_t ns;
    ns.name_space = (char*)"Device.Test.Param";
    ns.dataType = ccsp_string;
    CCSP_Base_Func_CB cb;
    memset(&cb, 0, sizeof(cb));
    auto result = ccsp_usp_build_schema_and_handlers(
        &ns, 0, &cb, malloc, free);
    EXPECT_TRUE(result.empty());
}

TEST_F(UspAdapterTest, SchemaBuilder_SingleParam)
{
    name_spaceType_t ns;
    ns.name_space = (char*)"Device.DeviceInfo.ModelName";
    ns.dataType = ccsp_string;
    CCSP_Base_Func_CB cb;
    memset(&cb, 0, sizeof(cb));
    auto result = ccsp_usp_build_schema_and_handlers(
        &ns, 1, &cb, malloc, free);
    ASSERT_EQ(1u, result.size());

    auto& [root, schema, handlers] = result[0];
    EXPECT_FALSE(root.empty());

    auto& objects = schema.objects();
    ASSERT_EQ(1u, objects.size());
    EXPECT_EQ("Device.DeviceInfo.", objects[0].path);
    EXPECT_FALSE(objects[0].is_multi_instance);
    ASSERT_EQ(1u, objects[0].params.size());
    EXPECT_EQ("ModelName", objects[0].params[0].name);
    EXPECT_EQ(usp::ValueType::String, objects[0].params[0].type);
}

TEST_F(UspAdapterTest, SchemaBuilder_MultiInstance)
{
    name_spaceType_t ns[2];
    ns[0].name_space = (char*)"Device.WiFi.SSID.{i}.SSID";
    ns[0].dataType = ccsp_string;
    ns[1].name_space = (char*)"Device.WiFi.SSID.{i}.Enable";
    ns[1].dataType = ccsp_boolean;
    CCSP_Base_Func_CB cb;
    memset(&cb, 0, sizeof(cb));
    auto result = ccsp_usp_build_schema_and_handlers(
        ns, 2, &cb, malloc, free);
    ASSERT_EQ(1u, result.size());

    auto& [root, schema, handlers] = result[0];
    auto& objects = schema.objects();
    ASSERT_EQ(1u, objects.size());
    EXPECT_EQ("Device.WiFi.SSID.{i}.", objects[0].path);
    EXPECT_TRUE(objects[0].is_multi_instance);
    ASSERT_EQ(2u, objects[0].params.size());
}

TEST_F(UspAdapterTest, SchemaBuilder_MultipleObjects)
{
    name_spaceType_t ns[3];
    ns[0].name_space = (char*)"Device.DeviceInfo.ModelName";
    ns[0].dataType = ccsp_string;
    ns[1].name_space = (char*)"Device.WiFi.SSID.{i}.SSID";
    ns[1].dataType = ccsp_string;
    ns[2].name_space = (char*)"Device.WiFi.SSID.{i}.Enable";
    ns[2].dataType = ccsp_boolean;
    CCSP_Base_Func_CB cb;
    memset(&cb, 0, sizeof(cb));
    auto result = ccsp_usp_build_schema_and_handlers(
        ns, 3, &cb, malloc, free);
    ASSERT_EQ(1u, result.size());

    auto& [root, schema, handlers] = result[0];
    auto& objects = schema.objects();
    EXPECT_EQ(2u, objects.size());
}
