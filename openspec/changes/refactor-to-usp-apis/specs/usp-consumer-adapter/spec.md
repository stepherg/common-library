## ADDED Requirements

### Requirement: Consumer adapter initializes USP controller session
The adapter SHALL create a `usp::ControllerSession` when a consumer-side bus handle is initialized, using the component's endpoint_id and the broker socket path.

#### Scenario: Successful consumer initialization
- **WHEN** a consumer component calls the adapter's init function with a component identifier
- **THEN** the adapter SHALL create a `ControllerSession` via `ControllerSession::create()`, start a background event loop, and return a valid bus handle

#### Scenario: Consumer reconnection on disconnect
- **WHEN** the controller session's `on_disconnected` callback fires
- **THEN** the adapter SHALL log a warning and rely on the USP library's built-in reconnection to re-establish the session

### Requirement: Consumer adapter provides get parameter values
The adapter SHALL implement `CcspBaseIf_getParameterValues`-compatible function that delegates to `ControllerSession::get_sync()`.

#### Scenario: Get single parameter
- **WHEN** a consumer calls `CcspBaseIf_getParameterValues(bus_handle, NULL, NULL, "Device.DeviceInfo.ModelName", 1, &size, &values)`
- **THEN** the adapter SHALL call `get_sync("Device.DeviceInfo.ModelName")`, convert the `GetResponse` to `parameterValStruct_t` array, set `size` to the number of results, and return `CCSP_SUCCESS`

#### Scenario: Get object (wildcard)
- **WHEN** a consumer calls `CcspBaseIf_getParameterValues` with path `Device.DeviceInfo.`
- **THEN** the adapter SHALL call `get_sync("Device.DeviceInfo.")` and return all parameters under that object as `parameterValStruct_t` entries

#### Scenario: Get nonexistent parameter
- **WHEN** `get_sync()` returns an error with `ErrorCode::InvalidPath`
- **THEN** the adapter SHALL return `CCSP_ERR_INVALID_PARAMETER_NAME`

### Requirement: Consumer adapter provides set parameter values
The adapter SHALL implement `CcspBaseIf_setParameterValues`-compatible function that delegates to `ControllerSession::set_sync()`.

#### Scenario: Set single parameter
- **WHEN** a consumer calls `CcspBaseIf_setParameterValues` with one `parameterValStruct_t` containing `("Device.X_Test.Param1", "newval", ccsp_string)`
- **THEN** the adapter SHALL call `set_sync("Device.X_Test.Param1", "newval")` and return `CCSP_SUCCESS` on success

#### Scenario: Set multiple parameters atomically
- **WHEN** a consumer calls `CcspBaseIf_setParameterValues` with an array of 3 `parameterValStruct_t` entries
- **THEN** the adapter SHALL construct a `vector<ParamValue>` from all entries and call `set_sync(params)` for atomic multi-parameter set

#### Scenario: Set read-only parameter
- **WHEN** `set_sync()` returns `ErrorCode::ReadOnlyViolation`
- **THEN** the adapter SHALL return `CCSP_ERR_NOT_WRITABLE` and populate the invalid parameter name

### Requirement: Consumer adapter provides add table row
The adapter SHALL implement `CcspBaseIf_addTblRow`-compatible function that delegates to `ControllerSession::add_sync()`.

#### Scenario: Add table row
- **WHEN** a consumer calls `CcspBaseIf_addTblRow(bus_handle, NULL, NULL, "Device.WiFi.SSID.", &instance)`
- **THEN** the adapter SHALL call `add_sync("Device.WiFi.SSID.", {})` and set `instance` to the returned instance number

#### Scenario: Add not allowed
- **WHEN** `add_sync()` returns an error with `ErrorCode::AddNotAllowed`
- **THEN** the adapter SHALL return `CCSP_ERR_NOT_SUPPORT`

### Requirement: Consumer adapter provides delete table row
The adapter SHALL implement `CcspBaseIf_deleteTableRow`-compatible function that delegates to `ControllerSession::del_sync()`.

#### Scenario: Delete table row
- **WHEN** a consumer calls `CcspBaseIf_deleteTableRow(bus_handle, NULL, NULL, "Device.WiFi.SSID.2.")`
- **THEN** the adapter SHALL call `del_sync("Device.WiFi.SSID.2.")` and return `CCSP_SUCCESS`

### Requirement: Consumer adapter provides operate command
The adapter SHALL implement a function for invoking USP Operate commands, compatible with existing CCSP method invocation patterns.

#### Scenario: Invoke sync command
- **WHEN** a consumer calls the adapter's operate function with command `Device.IP.Diagnostics.IPPing()` and input args `{"Host": "8.8.8.8"}`
- **THEN** the adapter SHALL call `operate_sync("Device.IP.Diagnostics.IPPing()", input_args)` and return the output arguments

### Requirement: Consumer adapter maps USP errors to CCSP error codes
The adapter SHALL translate `usp::ErrorCode` values to CCSP integer error codes.

#### Scenario: Error code mapping
- **WHEN** a USP operation returns an error
- **THEN** the adapter SHALL map: `GeneralFailure → CCSP_FAILURE`, `InvalidArgument → CCSP_ERR_INVALID_PARAMETER_VALUE`, `ReadOnlyViolation → CCSP_ERR_NOT_WRITABLE`, `UnknownParam → CCSP_ERR_INVALID_PARAMETER_NAME`, `InvalidPath → CCSP_ERR_INVALID_PARAMETER_NAME`, `Timeout → CCSP_ERR_TIMEOUT`, `NotConnected → CCSP_ERR_NOT_CONNECT`

### Requirement: Consumer adapter eliminates component discovery
The adapter SHALL NOT require consumers to call `CcspBaseIf_discComponentSupportingNamespace` before operations. The USP broker handles path-based routing. However, the adapter SHALL remain compatible with consumers that still call discovery first.

#### Scenario: Direct parameter access without discovery
- **WHEN** a consumer calls `CcspBaseIf_getParameterValues` with `dest_componentName = NULL`
- **THEN** the adapter SHALL send the get request directly to the USP broker without prior component discovery, and the broker SHALL route to the correct agent

#### Scenario: Parameter access after discovery (discover-then-query pattern)
- **WHEN** a consumer calls `CcspBaseIf_discComponentSupportingNamespace` and then passes the returned `componentName` and `dbusPath` to `CcspBaseIf_getParameterValues`
- **THEN** the adapter SHALL accept but ignore the `dest_componentName` and `dbus_path` parameters, routing the request to the USP broker by parameter path only

#### Scenario: Discovery stub returns valid struct
- **WHEN** a consumer calls `CcspBaseIf_discComponentSupportingNamespace` with any namespace
- **THEN** the adapter SHALL return `CCSP_SUCCESS` with `count = 1` and a `componentStruct_t` containing a synthetic component name and path, so that callers' NULL-checks and error handling do not trigger failure paths
