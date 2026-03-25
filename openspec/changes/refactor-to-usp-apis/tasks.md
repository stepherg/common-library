## 1. Adapter Scaffolding & Build Setup

- [x] 1.1 Create `source/util_api/ccsp_usp_adapter/` directory with `ccsp_usp_provider_adapter.h`, `ccsp_usp_consumer_adapter.h`, `ccsp_usp_subscription_adapter.h` header files declaring `extern "C"` API surfaces
- [x] 1.2 Create corresponding `.cpp` implementation files: `ccsp_usp_provider_adapter.cpp`, `ccsp_usp_consumer_adapter.cpp`, `ccsp_usp_subscription_adapter.cpp`
- [x] 1.3 Update `source/Makefile.am` to build the adapter as a shared library (`libccsp_usp_adapter`) with C++17 flags and link against `-lusp`
- [x] 1.4 Update `configure.ac` to add USP library detection (`PKG_CHECK_MODULES` or `AC_CHECK_LIB`) and a `--with-usp` configure flag
- [x] 1.5 Create CCSP-to-USP type mapping table in a shared `ccsp_usp_types.h` mapping `dataType_e` to `usp::ValueType` and CCSP error codes to `usp::ErrorCode`
- [ ] 1.6 Git checkpoint: `git add -A && git commit -m "feat: scaffold USP adapter modules and build setup" && git push`

## 2. Provider Adapter — Session & Schema

- [ ] 2.1 Implement `CCSP_Message_Bus_Init` wrapper that creates `usp::AgentSession` from `component_id` and config file, stores session in bus_handle, and starts background event loop via `AgentSession::start()`
- [ ] 2.2 Implement `CCSP_Message_Bus_Exit` wrapper that calls `RunHandle::stop()` and destroys the `AgentSession`
- [ ] 2.3 Implement namespace-to-schema converter: parse `name_spaceType_t` arrays into `usp::Schema` with `ObjectDef` entries, setting `is_multi_instance`, `ParamDef` access/type, and `unique_keys`
- [ ] 2.4 Implement `CcspBaseIf_registerCapabilities` wrapper that calls the namespace-to-schema converter and then `AgentSession::provide(path, schema, handlers)`
- [ ] 2.5 Git checkpoint: `git add -A && git commit -m "feat: implement provider adapter session and schema registration" && git push`

## 3. Provider Adapter — ObjectHandlers

- [ ] 3.1 Implement `ObjectHandlers::get` adapter: translate `ObjectContext` to full parameter path, invoke `CCSPBASEIF_GETPARAMETERVALUES` callback, convert `parameterValStruct_t` array to `ParamMap`
- [ ] 3.2 Implement `ObjectHandlers::set` adapter: construct `parameterValStruct_t` from path + value, invoke `CCSPBASEIF_SETPARAMETERVALUES` callback, return `Status` from callback result
- [ ] 3.3 Implement `ObjectHandlers::instances` adapter: invoke `CCSPBASEIF_GETPARAMETERNAMES` for the object path, collect `nextLevel` entries, extract instance numbers, return `vector<uint32_t>`
- [ ] 3.4 Implement `ObjectHandlers::add` adapter: invoke `CCSPBASEIF_ADDTBLROW` callback with object path, return `Result<uint32_t>` with new instance number
- [ ] 3.5 Implement `ObjectHandlers::del` adapter: reconstruct full instance path from `ObjectContext`, invoke `CCSPBASEIF_DELETETBLROW` callback, return `Status`
- [ ] 3.6 Implement `AgentHandlers::on_operate` adapter: map `OperateRequest` to component's command callback, translate input/output args, return `OperateResponse`
- [ ] 3.7 Git checkpoint: `git add -A && git commit -m "feat: implement provider adapter ObjectHandlers" && git push`

## 4. Consumer Adapter — Core Operations

- [ ] 4.1 Implement consumer-side init: create `usp::ControllerSession`, start background event loop, store in bus handle structure
- [ ] 4.2 Implement `CcspBaseIf_getParameterValues` wrapper: accept but ignore `dest_componentName` and `dbus_path` parameters, call `ControllerSession::get_sync(path)`, convert `GetResponse::ResolvedPath` entries to `parameterValStruct_t` array, map errors to CCSP codes
- [ ] 4.3 Implement `CcspBaseIf_setParameterValues` wrapper: accept but ignore destination parameters, convert `parameterValStruct_t` array to `vector<ParamValue>`, call `set_sync(params)`, map errors and populate invalid param output
- [ ] 4.4 Implement `CcspBaseIf_addTblRow` wrapper: accept but ignore destination parameters, call `add_sync(obj_path, {})`, extract instance number from `AddResponse`, return CCSP result code
- [ ] 4.5 Implement `CcspBaseIf_deleteTableRow` wrapper: accept but ignore destination parameters, call `del_sync(obj_path)`, return CCSP result code
- [ ] 4.6 Implement operate wrapper: call `operate_sync(command, input_args)`, convert `OperateResponse` output args to CCSP-compatible format
- [ ] 4.7 Implement `CcspBaseIf_getParameterNames` wrapper: call `get_sync(path)` and extract parameter name list from response, or use `get_instances_sync` for object enumeration
- [ ] 4.8 Git checkpoint: `git add -A && git commit -m "feat: implement consumer adapter core operations" && git push`

## 5. Consumer Adapter — Error Mapping & Discovery Elimination

- [ ] 5.1 Implement bidirectional error code mapping functions: `usp::ErrorCode` → CCSP int and CCSP int → `usp::ErrorCode`
- [ ] 5.2 Stub `CcspBaseIf_discComponentSupportingNamespace` to return `CCSP_SUCCESS` with a synthetic `componentStruct_t` (valid component name and path) so existing discover-then-query callers don't hit failure paths
- [ ] 5.3 Stub `CcspBaseIf_requestSessionID` / `CcspBaseIf_informEndOfSession` as no-ops (USP multi-param set provides atomicity)
- [ ] 5.4 Git checkpoint: `git add -A && git commit -m "feat: implement error mapping and eliminate component discovery" && git push`

## 6. Subscription Adapter — Value Change

- [ ] 6.1 Implement subscribe function wrapping `ControllerSession::subscribe()` with `NotificationType::ValueChange`, storing active subscriptions in a local registry
- [ ] 6.2 Implement notification callback wrapper that converts `usp::Notification` (param_path, param_value) to CCSP event callback format
- [ ] 6.3 Implement unsubscribe function wrapping `ControllerSession::unsubscribe()`
- [ ] 6.4 Implement provider-side auto-emit: after `ObjectHandlers::set` succeeds, call `AgentSession::emit_value_change(path, value)`
- [ ] 6.5 Git checkpoint: `git add -A && git commit -m "feat: implement subscription adapter value change support" && git push`

## 7. Subscription Adapter — Object & Event Notifications

- [ ] 7.1 Implement ObjectCreation subscription wrapper and `emit_object_creation` call in `ObjectHandlers::add` adapter
- [ ] 7.2 Implement ObjectDeletion subscription wrapper and `emit_object_deletion` call in `ObjectHandlers::del` adapter
- [ ] 7.3 Implement custom Event subscription wrapper for system signals: map `CcspBaseIf_SendsystemReadySignal` → `emit_event("Device.", "SystemReady", {})`
- [ ] 7.4 Implement OperationComplete subscription wrapper and emit calls for async command completion (success and failure paths)
- [ ] 7.5 Implement reconnection re-subscribe: store `SubscribeOptions` per active subscription, re-issue all on `on_connected` callback
- [ ] 7.6 Git checkpoint: `git add -A && git commit -m "feat: implement object, event, and operation complete notifications" && git push`

## 8. Deprecation & Legacy Removal

- [ ] 8.1 Mark `Ccsp_RbusValueChange_Subscribe` and `Ccsp_RbusInterval_Subscribe` as deprecated, returning error codes
- [ ] 8.2 Remove `ccsp_rbus_serializer.c` / `ccsp_rbus_serializer.h` — USP handles serialization internally
- [ ] 8.3 Remove DBus-specific code paths and `#ifdef rbus_enabled` conditionals from `ccsp_message_bus.c`
- [ ] 8.4 Remove `ccsp_rbus_value_change.c/.h` and `ccsp_rbus_intervalsubscription.c/.h` polling subsystems
- [ ] 8.5 Create compatibility header mapping old API names to new adapter functions via `#define` macros for downstream components that include legacy headers
- [ ] 8.6 Git checkpoint: `git add -A && git commit -m "refactor: remove legacy rbus/DBus code and polling subsystems" && git push`

## 9. Integration Testing

- [ ] 9.1 Write adapter unit test: provider registers schema, consumer gets/sets parameters through adapter functions using USP transport
- [ ] 9.2 Write adapter unit test: add and delete table rows through consumer adapter, verify instance enumeration
- [ ] 9.3 Write adapter unit test: subscribe to value change, provider sets value, verify consumer receives notification
- [ ] 9.4 Write adapter unit test: subscribe to object creation/deletion, verify notifications on add/delete
- [ ] 9.5 Write adapter unit test: invoke sync operate command through consumer adapter, verify output args
- [ ] 9.6 Verify build: ensure `make` compiles the adapter library and links correctly with USP, and downstream components can link against the adapter
- [ ] 9.7 Git checkpoint: `git add -A && git commit -m "test: add USP adapter integration tests" && git push`
