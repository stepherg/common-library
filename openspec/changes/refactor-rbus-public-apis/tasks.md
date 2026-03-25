## 1. Extract Simple Method Handlers (lowest risk first)
- [x] 1.1 Create `ccsp_rbus_getHealth_handler()` — extract METHOD_GETHEALTH logic from `thread_path_message_func_rbus`; register with non-NULL `methodHandler` in `rbus_regDataElements`
- [x] 1.2 Create `ccsp_rbus_commit_handler()` — extract METHOD_COMMIT logic
- [x] 1.3 Create `ccsp_rbus_addTableRow_handler()` — extract METHOD_ADDTBLROW logic
- [x] 1.4 Create `ccsp_rbus_deleteTableRow_handler()` — extract METHOD_DELETETBLROW logic
- [x] 1.5 Verify simple handlers compile and pass existing tests

## 2. Extract Complex Method Handlers
- [x] 2.1 Create `ccsp_rbus_getParameterNames_handler()` — extract METHOD_GETPARAMETERNAMES logic (includes access flag computation and rowNamesOnly filtering)
- [x] 2.2 Create `ccsp_rbus_getParameterValues_handler()` — extract METHOD_GETPARAMETERVALUES logic (includes wildcard expansion)
- [x] 2.3 Create `ccsp_rbus_setParameterValues_handler()` — extract METHOD_SETPARAMETERVALUES logic (includes rollback, commit flag, property list caching); remove `rbusPropertyList_appendToMessage` usage
- [x] 2.4 Verify complex handlers preserve identical CCSP callback invocation behavior

## 3. Extract RPC Sub-Method Handlers
- [x] 3.1 Create `ccsp_rbus_getAttributes_handler()` — extract METHOD_RPC GetAttributes logic; remove `rbusObject_initFromMessage`/`rbusObject_appendToMessage` usage
- [x] 3.2 Create `ccsp_rbus_setAttributes_handler()` — extract METHOD_RPC SetAttributes logic; remove `rbusObject_initFromMessage` usage
- [x] 3.3 Create `ccsp_rbus_paramValueChangeSignal_handler()` — extract METHOD_RPC parameterValueChangeSignal logic; remove `rbusObject_initFromMessage` usage
- [x] 3.4 Verify RPC sub-method handlers match current behavior

## 4. Refactor Registration Flow
_DEFERRED: Removing `rbus_registerObj()` requires implementing `getHandler`, `setHandler`, `tableAddRowHandler`, `tableRemoveRowHandler`, and `eventSubHandler` for property elements. The per-property `setHandler` cannot replicate the atomic multi-parameter set semantics of the current raw-callback `setParameterValues` handler (which receives all params in one call). Additionally, `rbus_setCommit()` has no dedicated handler in `rbusCallbackTable_t`. These need validation on a build platform with integration tests before the raw callback can be safely removed._
- [ ] 4.1 Update `CCSP_Message_Bus_Init()` — remove `rbus_registerObj()` raw callback registration
- [x] 4.2 Update `rbus_regDataElements()` calls in `ccsp_message_bus.c` to pass non-NULL handler callbacks for all method and event elements
- [ ] 4.3 Remove `CCSP_Message_Bus_Register_Path_Priv_rbus()` and `thread_path_message_func_rbus()` (now dead code)
- [ ] 4.4 Remove `bus_info->rbus_callback` field if no longer needed

## 5. Refactor Subscription Handling
_DEFERRED: The `eventSubHandler` approach requires `getHandler` for `publishOnSubscribe` initial value delivery. Depends on Phase 4 property handlers._
- [ ] 5.1 Create `eventSubHandler` callback for subscription lifecycle — replace METHOD_SUBSCRIBE/METHOD_UNSUBSCRIBE dispatch with rbus framework subscription callbacks
- [ ] 5.2 Wire `eventSubHandler` into event-typed data element registrations
- [ ] 5.3 Update `ccsp_rbus_subscription.c` to work with the new handler-based subscription flow

## 6. Eliminate Private Serialization Dependencies
_Approach: created `ccsp_rbus_serializer.c/.h` — local reimplementations of the rbus-internal serialization codec using only public APIs (`rbus_value.h`, `rbus_property.h`, `rbus_object.h`, `rbus_filter.h`, `rbus_buffer.h`, `rtMessage.h`). Wire format is identical, so consumers see no change. Switching to `rbusEvent_Publish()` was deferred because it changes pub/sub semantics (broadcast vs. targeted) and risks regressions for interval-based subscriptions._
- [x] 6.1 In `ccsp_rbus_value_change.c` — replace `extern rbusFilter_InitFromMessage` and inline `rbusFilter_AppendToMessage` with local `ccsp_rbusFilter_*` functions
- [x] 6.2 In `ccsp_rbus_intervalsubscription.c` — replace `rbusFilter_AppendToMessage` + `rbusEventData_appendToMessage` with local `ccsp_rbusEventData_appendToMessage`
- [x] 6.3 In `ccsp_message_bus.c` — replace all `extern` declarations of rbus-private serialization functions with `#include "ccsp_rbus_serializer.h"`

## 7. Remove Private API Dependencies
- [x] 7.1 Remove `extern void rbusFilter_InitFromMessage(...)` declarations from all files
- [x] 7.2 Remove `extern void rbusEventData_appendToMessage(...)` declarations from all files
- [x] 7.3 Remove `extern void rbusObject_initFromMessage(...)` / `rbusObject_appendToMessage(...)` declarations
- [x] 7.4 Remove `extern void rbusPropertyList_appendToMessage(...)` declaration
- [x] 7.5 Remove inline `void rbusFilter_AppendToMessage(...)` declarations from value_change and intervalsubscription files

## 8. Verification
- [ ] 8.1 Confirm the project compiles without unresolved symbols
- [x] 8.2 Grep the entire source tree for `extern.*rbus.*appendToMessage\|extern.*rbus.*initFromMessage\|extern.*rbus.*InitFromMessage\|extern.*rbus.*AppendToMessage` — expect zero matches
- [ ] 8.3 Run existing unit/integration tests — all MUST pass
- [ ] 8.4 Verify CCSP components that link against ccsp-common-library compile without changes
- [ ] 8.5 End-to-end validation: verify GetParameterValues, SetParameterValues, GetParameterNames, subscriptions, and table operations work on a reference platform
