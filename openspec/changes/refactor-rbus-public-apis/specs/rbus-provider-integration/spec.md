## ADDED Requirements

### Requirement: Per-Method Handler Registration

All CCSP RPC methods SHALL be registered as rbus data elements with dedicated handler callbacks via `rbus_regDataElements()`, replacing the monolithic `rbus_registerObj()` raw callback dispatch.

#### Scenario: GetParameterValues handler registration
- **WHEN** a CCSP component calls `CCSP_Message_Bus_Init()`
- **THEN** a `methodHandler` callback for GetParameterValues SHALL be registered as an `RBUS_ELEMENT_TYPE_METHOD` data element

#### Scenario: All methods registered with non-NULL handlers
- **WHEN** `rbus_regDataElements()` is called during component initialization
- **THEN** every method element (GetParameterValues, SetParameterValues, GetParameterNames, GetHealth, Commit, AddTblRow, DeleteTblRow, GetAttributes, SetAttributes) SHALL have a non-NULL `methodHandler` in its `rbusCallbackTable_t`

#### Scenario: No raw callback registration
- **WHEN** the component initialization sequence completes
- **THEN** `rbus_registerObj()` with a raw `rbus_callback_t` SHALL NOT be called

### Requirement: Dedicated Method Handlers

Each RPC method SHALL be implemented as a separate handler function that receives and returns rbus-managed types (`rbusObject_t`, `rbusProperty_t`, `rbusValue_t`) without manual `rbusMessage` serialization.

#### Scenario: GetParameterValues returns values via handler
- **WHEN** a GetParameterValues request arrives
- **THEN** the handler SHALL invoke `func->getParameterValues()` and return the result through the rbus method handler output parameters

#### Scenario: SetParameterValues with rollback
- **WHEN** a SetParameterValues request with rollback=1 fails with `CCSP_ERR_INVALID_PARAMETER_VALUE`
- **THEN** the handler SHALL restore original values by calling `func->setParameterValues()` with cached pre-change values, matching current rollback behavior

#### Scenario: AddTblRow returns instance number
- **WHEN** an AddTblRow request arrives
- **THEN** the handler SHALL invoke `func->AddTblRow()` and return the new instance number through the rbus handler output

### Requirement: Event Subscription via Handler Callbacks

Subscription lifecycle (subscribe/unsubscribe) SHALL be managed through rbus `eventSubHandler` callbacks on event-typed data elements, replacing manual `METHOD_SUBSCRIBE`/`METHOD_UNSUBSCRIBE` dispatch.

#### Scenario: Subscribe with filter
- **WHEN** a consumer subscribes to a parameter with a filter condition
- **THEN** the `eventSubHandler` callback SHALL receive the filter via rbus framework parameters and route it to the appropriate subscription type (value-change or interval)

#### Scenario: Unsubscribe cleanup
- **WHEN** a consumer unsubscribes from a parameter
- **THEN** the `eventSubHandler` callback SHALL clean up the subscription record and stop any associated polling thread

### Requirement: Public API Event Publishing

Value-change and interval-subscription event publishing SHALL use `rbusEvent_Publish()` with `rbusObject_t` payloads, replacing manual `rbusEventData_appendToMessage` and raw message construction.

#### Scenario: Value-change event publishing
- **WHEN** a polled parameter value changes in `ccsp_rbus_value_change.c`
- **THEN** the event SHALL be published via `rbusEvent_Publish()` with an `rbusObject_t` containing the new value

#### Scenario: Interval subscription publishing
- **WHEN** an interval timer fires in `ccsp_rbus_intervalsubscription.c`
- **THEN** the current parameter value SHALL be published via `rbusEvent_Publish()`

### Requirement: No Private rbus API Dependencies

The ccsp-common-library source code SHALL NOT contain `extern` declarations or direct calls to rbus internal/private functions.

#### Scenario: No extern declarations of private functions
- **WHEN** the source tree is searched for `extern` declarations of `rbusObject_appendToMessage`, `rbusObject_initFromMessage`, `rbusPropertyList_appendToMessage`, `rbusFilter_AppendToMessage`, `rbusFilter_InitFromMessage`, `rbusEventData_appendToMessage`, or `rbusValue_appendToMessage`
- **THEN** no matches SHALL be found

#### Scenario: Build without private symbols
- **WHEN** ccsp-common-library is compiled and linked against a version of librbus that does not export internal serialization symbols
- **THEN** the build SHALL succeed with no unresolved symbol errors

### Requirement: CCSP API Source Compatibility

The public ccsp-common-library API (`CCSP_Message_Bus_Init`, `CCSP_Message_Bus_Exit`, `CCSP_Message_Bus_Register_Path`, `CcspBaseIf_*` functions, and the `CCSP_Base_Func_CB` callback structure) SHALL remain source-compatible for downstream CCSP components.

#### Scenario: Existing component builds unchanged
- **WHEN** a CCSP component that uses `CCSP_Message_Bus_Init()` and registers `CCSP_Base_Func_CB` callbacks is compiled against the refactored ccsp-common-library
- **THEN** the component SHALL compile without modification

#### Scenario: Callback invocation equivalence
- **WHEN** an rbus method request arrives that maps to a CCSP callback (e.g., `getParameterValues`)
- **THEN** the callback SHALL be invoked with the same parameters and receive the same data as under the previous raw-dispatch architecture
