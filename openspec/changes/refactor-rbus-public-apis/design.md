## Context

ccsp-common-library acts as a CCSP-to-rbus bridge. Its core job is to receive rbus RPC calls, translate them to CCSP callback invocations, and return results. Today it does this by registering a raw `rbus_callback_t` via `rbus_registerObj()` and funneling all traffic through one 700-line dispatcher (`thread_path_message_func_rbus`) that manually serializes/deserializes `rbusMessage` payloads using 7 rbus-internal functions.

The rbus public provider API (`rbus_regDataElements` with handler callbacks) already handles method routing, subscription lifecycle, and message serialization — the exact work the dispatcher does manually. Adopting the public API aligns ccsp-common-library with the intended rbus provider pattern, removes the private-API coupling, and yields cleaner, more maintainable code.

Stakeholders: ccsp-common-library maintainers, rbus team, all CCSP component owners (consumers of ccsp-common-library).

Constraints:
- Functional equivalence with the current dispatcher — every CCSP callback MUST be invoked with the same parameters
- The public API (`CCSP_Message_Bus_Init`, `CCSP_Message_Bus_Register_Path`, `CcspBaseIf_*`) MUST remain source-compatible for downstream CCSP components
- Only public rbus headers (`<rbus/rbus.h>`, `<rbus/rbus_object.h>`, etc.) may be used

## Goals / Non-Goals

- **Goal:** Replace `rbus_registerObj()` raw callback with per-element handler callbacks via `rbus_regDataElements()`
- **Goal:** Decompose the monolithic dispatcher into dedicated handler functions (one per RPC method)
- **Goal:** Eliminate all `extern` declarations and calls to rbus-private serialization functions
- **Goal:** Use `rbusEvent_Publish()` for event/subscription publishing instead of manual `rbusEventData_appendToMessage`
- **Goal:** Maintain full behavioral compatibility with existing CCSP components
- **Non-Goal:** Change the CCSP callback structures (`CCSP_Base_Func_CB`) or their function signatures
- **Non-Goal:** Migrate to a different data model or change parameter registration semantics
- **Non-Goal:** Optimize performance — this is a correctness and maintainability refactor

## Decisions

### 1. Handler-per-method decomposition

Split `thread_path_message_func_rbus` into individual handler functions. The current dispatch table maps directly:

| Method string | New handler function | rbus element type |
|---|---|---|
| `METHOD_GETPARAMETERVALUES` | `ccsp_rbus_getParameterValues_handler` | METHOD |
| `METHOD_SETPARAMETERVALUES` | `ccsp_rbus_setParameterValues_handler` | METHOD |
| `METHOD_GETPARAMETERNAMES` | `ccsp_rbus_getParameterNames_handler` | METHOD |
| `METHOD_GETHEALTH` | `ccsp_rbus_getHealth_handler` | METHOD |
| `METHOD_COMMIT` | `ccsp_rbus_commit_handler` | METHOD |
| `METHOD_ADDTBLROW` | `ccsp_rbus_addTableRow_handler` | METHOD |
| `METHOD_DELETETBLROW` | `ccsp_rbus_deleteTableRow_handler` | METHOD |
| `METHOD_RPC` (GetAttributes) | `ccsp_rbus_getAttributes_handler` | METHOD |
| `METHOD_RPC` (SetAttributes) | `ccsp_rbus_setAttributes_handler` | METHOD |
| `METHOD_RPC` (parameterValueChangeSignal) | `ccsp_rbus_paramValueChangeSignal_handler` | METHOD |
| `METHOD_SUBSCRIBE` | Handled by rbus `eventSubHandler` callback | EVENT |
| `METHOD_UNSUBSCRIBE` | Handled by rbus `eventSubHandler` callback | EVENT |

*Alternative:* Keep the monolithic dispatcher but just copy serialization functions locally (Option 1 from analysis.md) → Rejected because it treats the symptom (private API usage) not the root cause (architectural bypass of the public API).

### 2. Registration strategy

Currently, `rbus_regDataElements` is called in three places with all-NULL handlers:
- `ccsp_message_bus.c` lines 1074-1112: registers methods/events for specific components
- `ccsp_base_api.c` lines 1510-1520: registers property elements

**New approach:** Populate the handler struct fields when registering elements:
- Methods: set `methodHandler` to the corresponding handler function
- Properties: set `getHandler`/`setHandler` pointing to CCSP callback bridge functions
- Events: set `eventSubHandler` for subscription lifecycle management
- Tables: set `tableAddRowHandler`/`tableRemoveRowHandler` for table operations

The raw `rbus_registerObj()` call and `rbus_unregisterObj()` / re-register pattern will be removed.

### 3. Subscription refactor

Current subscription handling (`METHOD_SUBSCRIBE`/`METHOD_UNSUBSCRIBE` in the dispatcher) manually parses filter/interval/duration from `rbusMessage` and routes to `ccsp_rbus_event_subscribe_override_handler()`. 

**New approach:** Use rbus `eventSubHandler` callbacks. When rbus delivers a subscribe/unsubscribe event to the handler, it provides the event name, subscriber info, and filter directly — no manual `rbusFilter_InitFromMessage` needed.

For publishing, replace `rbusEventData_appendToMessage` + raw message send with `rbusEvent_Publish()` in both `ccsp_rbus_value_change.c` and `ccsp_rbus_intervalsubscription.c`.

### 4. File organization

The handlers will live in `ccsp_message_bus.c` as static functions (they are tightly coupled to `CCSP_Base_Func_CB` and `bus_info`). No new source files are needed — the decomposition reduces complexity within the existing file.

*Alternative:* Extract handlers to a separate `ccsp_rbus_handlers.c` → Deferred; can be done as a follow-up if the file becomes too large.

## Risks / Trade-offs

- **Risk:** Subtle behavioral differences between the raw callback dispatch and rbus handler dispatch (e.g., error code mapping, response format) → Mitigation: Methodical side-by-side comparison of each handler's input/output against the current dispatcher. Integration tests MUST pass.
- **Risk:** rbus handler API may not support all the custom behavior in the current dispatcher (e.g., `publishOnSubscribe` with initial value, rollback on `SetParameterValues`) → Mitigation: The `methodHandler` callback receives `rbusObject_t` in/out parameters, allowing full control over response content. Subscription-specific behavior can be handled in `eventSubHandler`.
- **Risk:** Large diff touching the core message bus file → Mitigation: Incremental approach — refactor one method at a time, validating each before moving to the next. Tasks are ordered by complexity (simplest methods first).
- **Trade-off:** Larger scope than Option 1, but addresses root cause. Option 1 would still leave the monolithic dispatcher and raw callback pattern in place.

## Open Questions

- Does the rbus `methodHandler` callback provide the `rtMessageHeader` (reply_topic) that the current raw callback receives? If not, is it needed for any of the CCSP methods? (The current code references `hdr` but primarily for subscription routing.)
- Should property elements registered in `ccsp_base_api.c` get `getHandler`/`setHandler` callbacks, or should property access continue to route through method-level handlers? (Recommend: method-level first for minimal behavioral change, property-level handlers as a follow-up.)
