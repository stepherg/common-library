/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2019 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Local reimplementation of rbus internal serialization functions
 * using only public rbus APIs.  This eliminates the need for extern
 * declarations of rbus-private symbols.
 *
 * Each function produces the same wire format as the rbus-internal
 * original so that callers and consumers see no behavioural change.
 */

#ifndef CCSP_RBUS_SERIALIZER_H
#define CCSP_RBUS_SERIALIZER_H

#include <rbus/rbus.h>
#include <rbus/rbuscore.h>

void ccsp_rbusValue_appendToMessage(rbusValue_t value, rbusMessage msg);
void ccsp_rbusValue_initFromMessage(rbusValue_t* value, rbusMessage msg);

void ccsp_rbusPropertyList_appendToMessage(rbusProperty_t prop, rbusMessage msg);

void ccsp_rbusObject_appendToMessage(rbusObject_t obj, rbusMessage msg);

void ccsp_rbusFilter_AppendToMessage(rbusFilter_t filter, rbusMessage msg);
void ccsp_rbusFilter_InitFromMessage(rbusFilter_t* filter, rbusMessage msg);

void ccsp_rbusEventData_appendToMessage(
    rbusEvent_t* event,
    rbusFilter_t filter,
    uint32_t interval,
    uint32_t duration,
    int32_t componentId,
    rbusMessage msg);

#endif /* CCSP_RBUS_SERIALIZER_H */
