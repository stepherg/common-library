## Why

The CCSP common library currently uses a dual-stack messaging layer (rbus + DBus) with significant complexity: hand-rolled serialization, manual namespace registration via a Component Registrar, polling-based value-change detection, and session/WriteID tracking for atomic transactions. A new USP library (`temp/usp/libs/usp/`) provides a cleaner, schema-driven API with per-object handler dispatch, structured error types (`Result<T>`, `Status`), push-based notifications, and built-in event-loop integration. Migrating to USP APIs eliminates the legacy DBus path, removes the custom serialization layer, and replaces the polling subscription model with declarative schema + handler registration.

## What Changes

- **BREAKING**: Replace `CCSP_Message_Bus_Init` / `CCSP_Message_Bus_Register_Path` connection management with `usp::AgentSession::create` / `usp::ControllerSession::create`
- **BREAKING**: Replace `CCSP_Base_Func_CB` callback table and `thread_path_message_func` raw dispatcher with `usp::Schema` declarations + `usp::ObjectHandlers` per-object registrations via `AgentSession::handle()`
- **BREAKING**: Replace `CcspBaseIf_getParameterValues` / `CcspBaseIf_setParameterValues` consumer APIs with `ControllerSession::get_sync()` / `set_sync()` (and async variants)
- **BREAKING**: Replace `CcspBaseIf_registerBase` / `CcspBaseIf_registerCapabilities` / namespace registration with `AgentSession::provide()` schema-based registration
- **BREAKING**: Replace polling-based `Ccsp_RbusValueChange_Subscribe` and interval subscriptions with `AgentSession::emit_value_change()` push notifications and `ControllerSession::subscribe()` with `NotificationType::ValueChange`
- Replace `CcspBaseIf_addTblRow` / `CcspBaseIf_deleteTableRow` with `ControllerSession::add_sync()` / `del_sync()` and `ObjectHandlers::add` / `ObjectHandlers::del`
- Replace `CcspBaseIf_SendparameterValueChangeSignal` and other system signals with `AgentSession::emit_event()` / `emit_value_change()` / `emit_object_creation()` / `emit_object_deletion()`
- Remove `ccsp_rbus_serializer.c/.h` custom serialization — USP handles wire format internally
- Remove DBus-specific code paths, `rbus_enabled` conditionals, and legacy DBus fallback
- Introduce a C adapter layer (`ccsp_usp_adapter`) that bridges existing C-based CCSP callback signatures to the C++ USP API, enabling incremental migration of downstream components

## Capabilities

### New Capabilities
- `usp-provider-adapter`: C adapter wrapping `usp::AgentSession` + `usp::Schema` + `usp::ObjectHandlers` so existing CCSP provider components can register data model elements through familiar C function pointers while using USP transport underneath
- `usp-consumer-adapter`: C adapter wrapping `usp::ControllerSession` so existing consumer code (TR-069 PA, CLI, WebUI) can call get/set/add/delete/operate via C functions that delegate to USP sync/async APIs
- `usp-subscription-adapter`: C adapter for `ControllerSession::subscribe()` / `AgentSession::emit_*()` replacing the polling-based value-change and interval subscription subsystems with USP push notifications

### Modified Capabilities

## Impact

- **Source files**: `source/util_api/ccsp_msg_bus/` (entire directory refactored or replaced), `source/ccsp/components/common/MessageBusHelper/`, `source/ccsp/components/common/DataModel/dml/components/DslhCpeController/`
- **Headers**: `ccsp_base_api.h`, `ccsp_message_bus.h`, `rbus_message_bus.h`, `ccsp_rbus_subscription.h`, `ccsp_rbus_value_change.h`, `ccsp_rbus_intervalsubscription.h`
- **Build system**: `configure.ac`, `Makefile.am` files need USP library linking (`-lusp`) and C++ compilation support
- **Dependencies**: New dependency on `usp` library (C++17); rbus/DBus dependencies become optional or removed
- **Downstream components**: All CCSP components using `CCSP_Base_Func_CB`, `CcspBaseIf_*` APIs, or `CCSP_Message_Bus_Init` will need to link against the new adapter layer — but their source code changes are minimized by the adapter's API compatibility
- **Configuration**: `ccsp_msg.cfg` replaced with USP `SessionConfig` (endpoint_id + socket_path); `basic.conf` DBus config no longer needed
