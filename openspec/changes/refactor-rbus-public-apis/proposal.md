## Why

ccsp-common-library bypasses the public rbus provider API (`rbus_regDataElements` with get/set/method/event handler callbacks) and instead registers a raw `rbus_callback_t` via `rbus_registerObj()`. All incoming RPC methods are routed through a single monolithic dispatch function (`thread_path_message_func_rbus`) that manually parses raw `rbusMessage` payloads and calls 7 rbus-private serialization functions via `extern` declarations. This creates two problems:

1. **Private API coupling** — 7 internal rbus serialization functions (`rbusObject_appendToMessage`, `rbusObject_initFromMessage`, etc.) can break without notice when rbus is updated.
2. **Architectural bypass** — The monolithic dispatcher duplicates work that rbus already handles: method routing, parameter type validation, subscription lifecycle management, and message serialization. This makes the code harder to maintain and extend.

Refactoring to use the high-level rbus provider API eliminates both problems at the root.

## What Changes

- **Replace `rbus_registerObj()` raw callback with `rbus_regDataElements()` handler callbacks** — data elements currently registered with all-NULL handlers will get proper `methodHandler`, `getHandler`, `setHandler`, `tableAddRowHandler`, `tableRemoveRowHandler`, and `eventSubHandler` callbacks
- **Decompose `thread_path_message_func_rbus()`** — split the ~700-line monolithic dispatcher into dedicated handler functions, one per RPC method (GetParameterValues, SetParameterValues, GetParameterNames, GetHealth, Commit, AddTblRow, DeleteTblRow, GetAttributes, SetAttributes, parameterValueChangeSignal)
- **Remove raw `rbus_registerObj()` call** — the handler callbacks in `rbus_regDataElements` replace the need for the raw object-level callback
- **Remove all `extern` declarations** of rbus-private serialization functions — the rbus framework handles serialization when using the public handler API
- **Refactor subscription handling** — replace manual subscribe/unsubscribe dispatch and `rbusFilter`/`rbusEventData` serialization with `eventSubHandler` callbacks and `rbusEvent_Publish()`
- **Refactor value-change and interval-subscription publishing** — replace manual `rbusEventData_appendToMessage` and `rbusFilter_AppendToMessage` with `rbusEvent_Publish()` using `rbusObject_t` payloads
- **Update build configuration** — remove any obsolete source files; add new handler files if extracted

## Impact

- Affected specs: rbus-provider-integration (new capability)
- Affected code: `ccsp_message_bus.c` (major rewrite of init + dispatch), `ccsp_rbus_value_change.c` (event publishing), `ccsp_rbus_intervalsubscription.c` (event publishing), `ccsp_rbus_subscription.c` (subscription lifecycle), `ccsp_base_api.c` (element registration), build configuration
- **BREAKING** for any downstream code that depends on the raw `rbus_callback_t` registration pattern or the `thread_path_message_func_rbus` function signature
- Wire protocol compatibility: rbus framework manages serialization — interop with existing consumers/providers is maintained as the rbus public API guarantees wire compatibility
- Runtime behavior: Functionally equivalent — same CCSP callbacks are invoked with the same parameters
