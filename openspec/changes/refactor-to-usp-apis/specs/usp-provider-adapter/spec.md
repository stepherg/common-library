## ADDED Requirements

### Requirement: Provider adapter initializes USP agent session
The adapter SHALL create a `usp::AgentSession` when `CCSP_Message_Bus_Init` is called, using the component_id as the endpoint_id and the config file to resolve the socket path.

#### Scenario: Successful initialization
- **WHEN** a CCSP component calls `CCSP_Message_Bus_Init(component_id, config_file, &bus_handle, malloc, free)`
- **THEN** the adapter SHALL create a `usp::AgentSession` with `endpoint_id` derived from `component_id`, store it in `bus_handle`, and return `CCSP_SUCCESS`

#### Scenario: Connection failure
- **WHEN** `AgentSession::create()` returns an error (e.g., broker not running)
- **THEN** the adapter SHALL return `CCSP_ERR_NOT_CONNECT` and set `bus_handle` to NULL

### Requirement: Provider adapter registers schema from namespace arrays
The adapter SHALL convert `name_spaceType_t` arrays into `usp::Schema` objects and register them with the agent session via `AgentSession::provide()`.

#### Scenario: Register namespace with parameters
- **WHEN** a component calls `CcspBaseIf_registerCapabilities` with a `name_spaceType_t` array containing `Device.X_Test.Param1` (ReadWrite, string) and `Device.X_Test.Param2` (ReadOnly, int)
- **THEN** the adapter SHALL construct a `usp::Schema` with an `ObjectDef` for `Device.X_Test.` containing `ParamDef{name="Param1", access=ReadWrite, type=String}` and `ParamDef{name="Param2", access=ReadOnly, type=Int}`, and call `provide("Device.X_Test.", schema, handlers)`

#### Scenario: Register multi-instance object
- **WHEN** a namespace registration includes a path ending with `{i}.` (e.g., `Device.WiFi.SSID.{i}.`)
- **THEN** the adapter SHALL set `is_multi_instance = true` on the corresponding `ObjectDef`

### Requirement: Provider adapter maps CCSP callbacks to ObjectHandlers
The adapter SHALL translate the `CCSP_Base_Func_CB` callback table into per-object `usp::ObjectHandlers` registrations.

#### Scenario: Get parameter callback mapping
- **WHEN** the USP dispatch engine calls the `ObjectHandlers::get` handler for `Device.X_Test.`
- **THEN** the adapter SHALL invoke the component's `CCSPBASEIF_GETPARAMETERVALUES` callback with the resolved parameter path and return the result as a `ParamMap`

#### Scenario: Set parameter callback mapping
- **WHEN** the USP dispatch engine calls the `ObjectHandlers::set` handler with `(ctx, "Param1", "newvalue")`
- **THEN** the adapter SHALL invoke the component's `CCSPBASEIF_SETPARAMETERVALUES` callback with a `parameterValStruct_t` containing the full path and value, and return `Status` based on the callback's return code

#### Scenario: Add table row callback mapping
- **WHEN** the USP dispatch engine calls the `ObjectHandlers::add` handler for a multi-instance object
- **THEN** the adapter SHALL invoke the component's `CCSPBASEIF_ADDTBLROW` callback and return `Result<uint32_t>` with the new instance number

#### Scenario: Delete table row callback mapping
- **WHEN** the USP dispatch engine calls the `ObjectHandlers::del` handler with an instance identified in `ctx.instances`
- **THEN** the adapter SHALL invoke the component's `CCSPBASEIF_DELETETBLROW` callback with the full instance path and return the resulting `Status`

#### Scenario: Instance enumeration
- **WHEN** the USP dispatch engine calls the `ObjectHandlers::instances` handler
- **THEN** the adapter SHALL invoke the component's `CCSPBASEIF_GETPARAMETERNAMES` callback for the object path to discover child instances, extract instance numbers, and return them as `vector<uint32_t>`

### Requirement: Provider adapter maps CCSP data types to USP ValueType
The adapter SHALL maintain a mapping between CCSP `dataType_e` enum values and `usp::ValueType` enum values.

#### Scenario: Type mapping completeness
- **WHEN** the adapter constructs a `ParamDef` from a `name_spaceType_t` entry
- **THEN** the following mappings SHALL be applied: `ccsp_string → String`, `ccsp_int → Int`, `ccsp_unsignedInt → UnsignedInt`, `ccsp_boolean → Boolean`, `ccsp_dateTime → DateTime`, `ccsp_base64 → Base64`, `ccsp_long → Long`, `ccsp_unsignedLong → UnsignedLong`

### Requirement: Provider adapter runs USP event loop
The adapter SHALL manage the USP event loop lifecycle so that CCSP components do not need to call USP event loop functions directly.

#### Scenario: Background event loop start
- **WHEN** `CCSP_Message_Bus_Init` completes successfully
- **THEN** the adapter SHALL call `AgentSession::start()` to launch a background event loop thread and store the `RunHandle`

#### Scenario: Clean shutdown
- **WHEN** a CCSP component calls `CCSP_Message_Bus_Exit`
- **THEN** the adapter SHALL call `RunHandle::stop()` to terminate the event loop thread and destroy the `AgentSession`

### Requirement: Provider adapter supports operate handlers
The adapter SHALL route USP Operate requests to CCSP command handlers when registered.

#### Scenario: Sync command invocation
- **WHEN** a USP Operate request arrives for a command registered via the adapter's `AgentHandlers::on_operate`
- **THEN** the adapter SHALL invoke the component's registered command callback with input arguments and return the output arguments as `OperateResponse::SyncResult`
