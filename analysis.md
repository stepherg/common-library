
---

## Why the private functions are used

ccsp-common-library is a **CCSP-to-rbus bridge** that works at the raw message transport layer. Instead of using the high-level rbus provider API (`rbus_regDataElements` with get/set/method/event handler callbacks), it registers a direct `rbus_callback_t` and manually handles raw `rbusMessage` objects — receiving them from the routing layer, deserializing them, dispatching to CCSP callbacks, then serializing the response back.

These 6 private functions are rbus's **internal serialization codec** that converts rbus data types (`rbusObject_t`, `rbusProperty_t`, `rbusFilter_t`, `rbusEvent_t`) to/from the binary wire format carried inside `rbusMessage`. Since ccsp-common-library bypasses the normal rbus API and produces/consumes the same wire format, it needs these same serialization routines.

The functions break down into 3 pairs:

| Private function | Used in | Direction |
|---|---|---|
| `rbusObject_appendToMessage` | ccsp_message_bus.c | Serialize object → response message |
| `rbusObject_initFromMessage` | ccsp_message_bus.c | Deserialize object ← request message |
| `rbusPropertyList_appendToMessage` | ccsp_message_bus.c | Serialize property list → response |
| `rbusFilter_AppendToMessage` | ccsp_rbus_value_change.c, ccsp_rbus_intervalsubscription.c | Serialize filter → publish message |
| `rbusFilter_InitFromMessage` | ccsp_message_bus.c, ccsp_rbus_value_change.c | Deserialize filter ← subscribe payload |
| `rbusEventData_appendToMessage` | ccsp_message_bus.c, ccsp_rbus_intervalsubscription.c | Serialize event → publish message |

---

## Can the code be rewritten to use only include?

**Yes — but there is no single right answer.** There are three viable approaches, with very different scopes:

### Option 1: Copy the serialization functions into ccsp-common-library (minimal change)

All 6 private functions (sourced from rbus.c) are implementable using **only public APIs** from include. Inspecting their bodies confirms they only call:
- rbus_object.h: `rbusObject_GetName`, `rbusObject_GetType`, `rbusObject_GetProperties`, `rbusObject_GetChildren`, `rbusObject_GetNext`, `rbusObject_Init`, `rbusObject_InitMultiInstance`, `rbusObject_SetProperties`, `rbusObject_SetChildren`, `rbusObject_SetNext`
- rbus_property.h: `rbusProperty_GetName`, `rbusProperty_GetValue`, `rbusProperty_GetNext`, `rbusProperty_Init`, `rbusProperty_SetValue`, `rbusProperty_SetNext`, `rbusProperty_Release`
- rbus_filter.h: `rbusFilter_GetType`, `rbusFilter_GetRelationOperator`, `rbusFilter_GetRelationValue`, `rbusFilter_GetLogicOperator`, `rbusFilter_GetLogicLeft`, `rbusFilter_GetLogicRight`, `rbusFilter_InitRelation`, `rbusFilter_InitLogic`, `rbusFilter_Release`
- rbus_value.h: `rbusValue_GetType`, `rbusValue_Init`, `rbusValue_Release`, `rbusValue_SetFromString`, etc.
- rbus_buffer.h (already public): `rbusValue_GetV`, `rbusValue_GetL`, `rbusValue_SetTLV`
- `rbusMessage_*` from rtmessage (already used throughout)

There is one indirect private dependency: `rbusValue_appendToMessage` is called by `rbusFilter_AppendToMessage` and `rbusPropertyList_appendToMessage`. It too is implementable from public APIs. The whole set of ~7 functions forms a self-contained codec group that could be copied as-is into a new file (e.g. `ccsp_rbus_serializer.c`) under ccsp-common-library, replacing the `extern` declarations with local implementations.

This is the **lowest-risk option** — no protocol compatibility changes, no architectural refactoring.

### Option 2: Refactor ccsp-common-library to use the high-level rbus provider API

The root cause of the problem is architectural: ccsp-common-library bypasses the public provider API. A full refactor using `rbus_regDataElements` with proper handler callbacks would eliminate *all* manual serialization. However, this would be a substantial rewrite touching all the method/property/subscription dispatch logic in ccsp_message_bus.c, and would require verifying consistent behavior for all CCSP wire protocol details.

---
