## ADDED Requirements

### Requirement: Subscription adapter supports value change subscriptions
The adapter SHALL allow consumers to subscribe for parameter value change notifications using `ControllerSession::subscribe()` with `NotificationType::ValueChange`.

#### Scenario: Subscribe to value change
- **WHEN** a consumer calls the adapter's subscribe function with parameter path `Device.DeviceInfo.SoftwareVersion` and a callback function
- **THEN** the adapter SHALL call `ControllerSession::subscribe({paths: ["Device.DeviceInfo.SoftwareVersion"], type: ValueChange, on_notify: <wrapper>})` and return a subscription handle

#### Scenario: Receive value change notification
- **WHEN** the USP broker delivers a ValueChange notification for a subscribed parameter
- **THEN** the adapter SHALL invoke the consumer's registered callback with the parameter path and new value in the format expected by CCSP event handlers

#### Scenario: Unsubscribe
- **WHEN** a consumer calls the adapter's unsubscribe function with a valid handle
- **THEN** the adapter SHALL call `ControllerSession::unsubscribe(handle)` and return success

### Requirement: Provider adapter emits value change on parameter set
The adapter SHALL call `AgentSession::emit_value_change()` whenever a provider's set handler successfully updates a parameter that has active subscriptions.

#### Scenario: Emit after successful set
- **WHEN** the `ObjectHandlers::set` adapter callback completes successfully for parameter `Device.X_Test.Param1` with value `"42"`
- **THEN** the adapter SHALL call `emit_value_change("Device.X_Test.Param1", "42")`

#### Scenario: No emission on set failure
- **WHEN** the `ObjectHandlers::set` adapter callback returns a non-OK `Status`
- **THEN** the adapter SHALL NOT call `emit_value_change()`

### Requirement: Subscription adapter supports object creation notifications
The adapter SHALL allow consumers to subscribe for object creation events and providers to emit them.

#### Scenario: Subscribe to object creation
- **WHEN** a consumer subscribes with type `ObjectCreation` for path `Device.WiFi.SSID.`
- **THEN** the adapter SHALL call `ControllerSession::subscribe({paths: ["Device.WiFi.SSID."], type: ObjectCreation, on_notify: <wrapper>})` and return a handle

#### Scenario: Emit object creation
- **WHEN** the adapter's add handler successfully creates instance `Device.WiFi.SSID.3.`
- **THEN** the adapter SHALL call `emit_object_creation("Device.WiFi.SSID.3.", unique_keys)`

### Requirement: Subscription adapter supports object deletion notifications
The adapter SHALL allow consumers to subscribe for object deletion events and providers to emit them.

#### Scenario: Subscribe to object deletion
- **WHEN** a consumer subscribes with type `ObjectDeletion` for path `Device.WiFi.SSID.`
- **THEN** the adapter SHALL call `ControllerSession::subscribe({paths: ["Device.WiFi.SSID."], type: ObjectDeletion, on_notify: <wrapper>})` and return a handle

#### Scenario: Emit object deletion
- **WHEN** the adapter's delete handler successfully removes instance `Device.WiFi.SSID.2.`
- **THEN** the adapter SHALL call `emit_object_deletion("Device.WiFi.SSID.2.")`

### Requirement: Subscription adapter supports custom event notifications
The adapter SHALL allow providers to emit custom events and consumers to subscribe to them, replacing CCSP system signals.

#### Scenario: Emit system ready event
- **WHEN** a component calls the adapter's equivalent of `CcspBaseIf_SendsystemReadySignal`
- **THEN** the adapter SHALL call `emit_event("Device.", "SystemReady", {})` on the agent session

#### Scenario: Subscribe to custom event
- **WHEN** a consumer subscribes with type `Event` for path `Device.` and event name matching `SystemReady`
- **THEN** the adapter SHALL receive the notification via the `on_notify` callback with `Notification.event_name == "SystemReady"`

### Requirement: Subscription adapter supports operation complete notifications
The adapter SHALL allow consumers to subscribe for operation complete events for async commands.

#### Scenario: Subscribe to operation complete
- **WHEN** a consumer invokes an async command and subscribes for `OperationComplete` on the command path
- **THEN** the adapter SHALL create a subscription via `ControllerSession::subscribe({type: OperationComplete, ...})` and deliver the result when the operation finishes

#### Scenario: Emit operation complete success
- **WHEN** a provider's async command finishes successfully
- **THEN** the adapter SHALL call `emit_operation_complete(obj_path, command_name, command_key, output_args)`

#### Scenario: Emit operation complete failure
- **WHEN** a provider's async command fails
- **THEN** the adapter SHALL call `emit_operation_complete(obj_path, command_name, command_key, err_code, err_msg)`

### Requirement: Subscription adapter eliminates polling-based value change detection
The adapter SHALL NOT use polling threads to detect value changes. All value change detection SHALL be push-based via `emit_value_change()`.

#### Scenario: No polling threads created
- **WHEN** the adapter is initialized and subscriptions are registered
- **THEN** no background polling threads SHALL be created for value change detection

#### Scenario: Old polling API returns deprecation error
- **WHEN** code calls `Ccsp_RbusValueChange_Subscribe()` or `Ccsp_RbusInterval_Subscribe()`
- **THEN** the function SHALL return an error code indicating the API is deprecated

### Requirement: Subscription adapter re-subscribes on reconnection
The adapter SHALL persist active subscription state and re-subscribe when the session reconnects after a connection loss.

#### Scenario: Re-subscribe after disconnect
- **WHEN** the `on_connected` callback fires after a reconnection
- **THEN** the adapter SHALL re-issue all active subscriptions using their stored `SubscribeOptions`

#### Scenario: Subscription callbacks remain valid after reconnection
- **WHEN** a notification arrives after a reconnection cycle
- **THEN** the adapter SHALL deliver it to the original consumer callback without requiring re-registration
