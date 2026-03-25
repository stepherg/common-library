## Context

The CCSP common library is the shared middleware for all RDK-B components. It currently provides data model registration, parameter get/set, table management, subscriptions, and inter-component messaging through a dual rbus/DBus transport stack. The codebase contains ~15 source files in `source/util_api/ccsp_msg_bus/` implementing this transport, plus headers and helpers across `source/ccsp/`. A previous refactoring effort (branch `rbus_public_apis_only`) decomposed the monolithic raw message dispatcher into handler functions and eliminated private rbus serialization calls, but the core architecture still relies on CCSP-specific patterns: `CCSP_Base_Func_CB` callback tables, Component Registrar namespace discovery, polling-based value-change detection, and session/WriteID transaction tracking.

A new USP library (`temp/usp/libs/usp/`) provides a C++ API with schema-driven data model declaration, per-object handler dispatch, `Result<T>`/`Status` error handling, push-based notifications, and integrated event loops. The library is already available in the workspace. This design describes how to bridge the existing C-based CCSP component ecosystem to the USP API through a C adapter layer, preserving API compatibility for downstream consumers while migrating the transport.

## Goals / Non-Goals

**Goals:**
- Replace the rbus/DBus transport with USP for all provider and consumer operations
- Provide C adapter functions that match existing `CcspBaseIf_*` and `CCSP_Message_Bus_*` signatures so downstream components require minimal source changes
- Replace polling-based subscriptions with USP push notifications
- Eliminate the custom serialization layer (`ccsp_rbus_serializer`)
- Remove DBus-specific code paths and `rbus_enabled` conditionals
- Preserve the `CCSP_Base_Func_CB` callback registration pattern through the adapter

**Non-Goals:**
- Rewriting downstream CCSP components (TR-069 PA, WiFi Agent, etc.) — they should work through the adapter with minimal changes
- Changing the TR-181 data model structure or parameter semantics
- Implementing USP Controller-to-Controller federation
- Adding USP Record/MTP-level transport security (TLS, WebSocket) — using UDS transport only
- Migrating the Component Registrar (CR) itself — it is out of scope and will be addressed separately

## Decisions

### 1. C adapter layer wrapping C++ USP objects

**Decision**: Introduce `ccsp_usp_adapter.h/.c` (compiled as C++) that exposes C-linkage functions matching the existing CCSP API signatures. Internally, it holds `usp::AgentSession` and `usp::ControllerSession` instances.

**Rationale**: The USP library is C++17. CCSP components are pure C. An `extern "C"` adapter is the standard interop approach. The alternative — rewriting USP with a C API — would require forking the USP library and maintaining a parallel API surface.

**Alternatives considered**:
- *Generate C bindings from USP headers automatically* — Too fragile given template types (`Result<T>`, `std::variant`)
- *Rewrite CCSP components in C++* — Massive scope, not justified by this change alone

### 2. Schema built from existing namespace registrations

**Decision**: The adapter's `provide()` wrapper will accept the existing `name_spaceType_t` arrays and `CCSP_Base_Func_CB` callback table, and internally construct `usp::Schema` + `usp::ObjectHandlers` from them.

**Rationale**: This preserves backward compatibility. Components already declare their namespaces and callbacks — the adapter translates these into USP schema objects. Over time, components can migrate to declare `usp::Schema` directly.

**Alternatives considered**:
- *Require all components to define Schema objects* — Too large a change for the initial migration; breaks backward compatibility

### 3. Replace Component Registrar discovery with USP path-based dispatch

**Decision**: Remove `CcspBaseIf_discComponentSupportingNamespace()` calls. USP's broker handles path routing — the controller sends get/set to a path and the broker dispatches to the owning agent. The adapter's consumer-side functions will connect to the USP broker directly rather than querying a CR first.

**Rationale**: USP's architecture eliminates the need for a separate discovery service. The broker knows which agent owns which paths based on `AgentSession::provide()` registrations. This removes the two-step "discover then query" pattern.

**Alternatives considered**:
- *Keep CR as a USP agent that answers discovery queries* — Adds complexity without benefit since USP already routes by path

### 4. Map session/WriteID semantics to USP set with multiple ParamValues

**Decision**: Replace `CcspBaseIf_requestSessionID` / `informEndOfSession` session-based transactions with USP's `ControllerSession::set_sync(vector<ParamValue>)` which atomically sets multiple parameters in one request.

**Rationale**: USP's Set message natively supports multiple parameter paths in a single request, providing atomicity without a separate session mechanism. The `commit` flag maps to whether the set carries `required=true` semantics at the USP level.

**Alternatives considered**:
- *Implement a session layer on top of USP* — Unnecessary complexity; USP's multi-param Set covers the use case

### 5. Push-based subscriptions replace polling

**Decision**: Replace `Ccsp_RbusValueChange_Subscribe` polling threads with `ControllerSession::subscribe()` (ValueChange type) on the consumer side, and `AgentSession::emit_value_change()` on the provider side whenever a parameter changes.

**Rationale**: Polling is wasteful and introduces latency proportional to the poll interval. USP's push model delivers notifications immediately when the provider calls `emit_value_change()`. The provider already knows when values change (it's handling the set).

**Alternatives considered**:
- *Keep polling as a compatibility mode* — Defeats the purpose of migration; providers should emit changes

### 6. Three adapter modules (provider, consumer, subscription)

**Decision**: Split the adapter into three modules matching the proposal's capabilities:
- `ccsp_usp_provider_adapter` — wraps `AgentSession`, translates `CCSP_Base_Func_CB` to `ObjectHandlers`
- `ccsp_usp_consumer_adapter` — wraps `ControllerSession`, provides `CcspBaseIf_getParameterValues`-compatible functions
- `ccsp_usp_subscription_adapter` — wraps subscription/notification APIs, replaces value-change polling

**Rationale**: Separation of concerns. Components that are only consumers don't need provider code linked. Each module can be tested and migrated independently.

**Alternatives considered**:
- *Single monolithic adapter* — Harder to test, larger link footprint for consumer-only components

### 7. Type mapping via `usp::ValueType` enum

**Decision**: Map CCSP `dataType_e` to `usp::ValueType` at the adapter boundary. All values remain string-serialized (matching both CCSP and USP conventions) but type metadata is preserved in the schema for validation.

**Rationale**: Both systems use string representations for parameter values. The type enum is metadata only, used for schema declaration and validation. A simple mapping table in the adapter handles conversion.

## Risks / Trade-offs

- **[Risk] Downstream components not recompiling against new headers** → Mitigation: The adapter preserves existing function signatures and header names where possible. A compatibility header maps old names to new adapter functions via `#define` macros.
- **[Risk] C++17 build requirement for the adapter** → Mitigation: Only the adapter `.cpp` files need C++17. Downstream C components link against the adapter as a shared library without needing a C++ compiler themselves.
- **[Risk] Performance regression from C → C++ → USP serialization** → Mitigation: USP's internal serialization is optimized. The adapter adds one function-call indirection (C to `extern "C"` C++ wrapper) which is negligible. Profile critical paths (bulk get/set) during integration testing.
- **[Risk] Incomplete coverage of CCSP APIs** → Mitigation: Prioritize the core APIs used by most components (get/set/add/delete/subscribe). Less-used APIs (shared memory variants, bus check, free resources) can be stubbed initially and implemented incrementally.
- **[Risk] Thread safety between CCSP callback threads and USP event loop** → Mitigation: USP's `process_events()` is non-blocking and the adapter can use the background thread model (`start()` + `RunHandle`). CCSP callbacks will be invoked from USP's dispatch thread, matching the current rbus callback threading model.
- **[Risk] Loss of subscription persistence across restarts** → Mitigation: Current NVRAM-based subscription cache can be replicated by persisting `SubscribeOptions` and re-subscribing on `on_connected` callback.

## Migration Plan

1. **Phase 1 — Adapter scaffolding**: Create the three adapter modules with the C API surface. Implement session creation/teardown wrapping `AgentSession::create` and `ControllerSession::create`.
2. **Phase 2 — Provider adapter**: Implement `CCSP_Base_Func_CB` → `ObjectHandlers` translation. Components register through the adapter and respond to get/set/add/delete via their existing callbacks.
3. **Phase 3 — Consumer adapter**: Implement `CcspBaseIf_getParameterValues`-compatible functions that delegate to `ControllerSession::get_sync()`. Cover all core consumer APIs.
4. **Phase 4 — Subscription adapter**: Replace polling subsystem. Provider-side emit on value change; consumer-side subscribe with callbacks.
5. **Phase 5 — Build integration**: Update `configure.ac` and `Makefile.am` to link USP, compile adapter as C++, and remove rbus/DBus dependencies.
6. **Phase 6 — Remove legacy code**: Delete `ccsp_rbus_serializer`, DBus fallback paths, `rbus_enabled` conditionals, and unused CCSP message bus code. This is a hard cutover — no conditional compilation or dual-stack support.

Rollback: Each phase is independently revertible via git revert. If a critical issue is found post-cutover, the pre-migration code can be restored from version control.

## Open Questions

- What is the expected UDS socket path for the USP broker in production deployments?
- Are there components that use `CcspBaseIf_getParameterValues_rbus` directly (bypassing the base API) that would need special handling?
