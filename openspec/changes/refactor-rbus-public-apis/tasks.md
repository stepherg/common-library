## 1. Extract Simple Method Handlers (lowest risk first)
- [ ] 1.1 Create `ccsp_rbus_getHealth_handler()` — extract METHOD_GETHEALTH logic from `thread_path_message_func_rbus`; register with non-NULL `methodHandler` in `rbus_regDataElements`
- [ ] 1.2 Create `ccsp_rbus_commit_handler()` — extract METHOD_COMMIT logic
- [ ] 1.3 Create `ccsp_rbus_addTableRow_handler()` — extract METHOD_ADDTBLROW logic
- [ ] 1.4 Create `ccsp_rbus_deleteTableRow_handler()` — extract METHOD_DELETETBLROW logic
- [ ] 1.5 Verify simple handlers compile and pass existing tests

## 2. Extract Complex Method Handlers
- [ ] 2.1 Create `ccsp_rbus_getParameterNames_handler()` — extract METHOD_GETPARAMETERNAMES logic (includes access flag computation and rowNamesOnly filtering)
- [ ] 2.2 Create `ccsp_rbus_getParameterValues_handler()` — extract METHOD_GETPARAMETERVALUES logic (includes wildcard expansion)
- [ ] 2.3 Create `ccsp_rbus_setParameterValues_handler()` — extract METHOD_SETPARAMETERVALUES logic (includes rollback, commit flag, property list caching); remove `rbusPropertyList_appendToMessage` usage
- [ ] 2.4 Verify complex handlers preserve identical CCSP callback invocation behavior

## 3. Extract RPC Sub-Method Handlers
- [ ] 3.1 Create `ccsp_rbus_getAttributes_handler()` — extract METHOD_RPC GetAttributes logic; remove `rbusObject_initFromMessage`/`rbusObject_appendToMessage` usage
- [ ] 3.2 Create `ccsp_rbus_setAttributes_handler()` — extract METHOD_RPC SetAttributes logic; remove `rbusObject_initFromMessage` usage
- [ ] 3.3 Create `ccsp_rbus_paramValueChangeSignal_handler()` — extract METHOD_RPC parameterValueChangeSignal logic; remove `rbusObject_initFromMessage` usage
- [ ] 3.4 Verify RPC sub-method handlers match current behavior

## 4. Refactor Registration Flow
- [ ] 4.1 Update `CCSP_Message_Bus_Init()` — remove `rbus_registerObj()` raw callback registration
- [ ] 4.2 Update `rbus_regDataElements()` calls in `ccsp_message_bus.c` to pass non-NULL handler callbacks for all method and event elements
- [ ] 4.3 Remove `CCSP_Message_Bus_Register_Path_Priv_rbus()` and `thread_path_message_func_rbus()` (now dead code)
- [ ] 4.4 Remove `bus_info->rbus_callback` field if no longer needed

## 5. Refactor Subscription Handling
- [ ] 5.1 Create `eventSubHandler` callback for subscription lifecycle — replace METHOD_SUBSCRIBE/METHOD_UNSUBSCRIBE dispatch with rbus framework subscription callbacks
- [ ] 5.2 Wire `eventSubHandler` into event-typed data element registrations
- [ ] 5.3 Update `ccsp_rbus_subscription.c` to work with the new handler-based subscription flow

## 6. Refactor Event Publishing
- [ ] 6.1 In `ccsp_rbus_value_change.c` — replace `rbusEventData_appendToMessage` + raw message send with `rbusEvent_Publish()` using `rbusObject_t` payload; remove `extern` declarations of private functions
- [ ] 6.2 In `ccsp_rbus_intervalsubscription.c` — replace `rbusEventData_appendToMessage` + `rbusFilter_AppendToMessage` with `rbusEvent_Publish()`; remove private function declarations
- [ ] 6.3 In `ccsp_message_bus.c` — remove all remaining `extern` declarations of rbus-private serialization functions

## 7. Remove Private API Dependencies
- [ ] 7.1 Remove `extern void rbusFilter_InitFromMessage(...)` declarations from all files
- [ ] 7.2 Remove `extern void rbusEventData_appendToMessage(...)` declarations from all files
- [ ] 7.3 Remove `extern void rbusObject_initFromMessage(...)` / `rbusObject_appendToMessage(...)` declarations
- [ ] 7.4 Remove `extern void rbusPropertyList_appendToMessage(...)` declaration
- [ ] 7.5 Remove inline `void rbusFilter_AppendToMessage(...)` declarations from value_change and intervalsubscription files

## 8. Verification
- [ ] 8.1 Confirm the project compiles without unresolved symbols
- [ ] 8.2 Grep the entire source tree for `extern.*rbus.*appendToMessage\|extern.*rbus.*initFromMessage\|extern.*rbus.*InitFromMessage\|extern.*rbus.*AppendToMessage` — expect zero matches
- [ ] 8.3 Run existing unit/integration tests — all MUST pass
- [ ] 8.4 Verify CCSP components that link against ccsp-common-library compile without changes
- [ ] 8.5 End-to-end validation: verify GetParameterValues, SetParameterValues, GetParameterNames, subscriptions, and table operations work on a reference platform
