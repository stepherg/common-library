/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
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

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/**********************************************************************

        For Dbus base library Implementation,
        Common Component Software Platform (CCSP)

    ---------------------------------------------------------------

    environment:

        platform dependent

    ---------------------------------------------------------------

    author:

        Qiang Tu

    ---------------------------------------------------------------

    revision:

        06/17/11    initial revision.

**********************************************************************/

#ifndef CCSP_MESSAGE_BUS_H
#define CCSP_MESSAGE_BUS_H

#include <stddef.h>
#include <pthread.h>

//match the error code in ccsp_base_api.h
#define CCSP_Message_Bus_OK             100
#define CCSP_Message_Bus_OOM            101
#define CCSP_Message_Bus_ERROR          102

#define CCSP_MESSAGE_BUS_CANNOT_CONNECT 190
#define CCSP_MESSAGE_BUS_TIMEOUT        191
#define CCSP_MESSAGE_BUS_NOT_EXIST      192
#define CCSP_MESSAGE_BUS_NOT_SUPPORT    193 //remote can't support this api

// resource limits
#define CCSP_MESSAGE_BUS_MAX_CONNECTION         15
#define CCSP_MESSAGE_BUS_MAX_FILTER             50
#define CCSP_MESSAGE_BUS_MAX_PATH               20

#define CCSP_MESSAGE_BUS_TIMEOUT_MAX_SECOND     60

#define WRITEID         "WRITEID"
#define PARAM_SIZE      "PARAM_SIZE"
#define SESSIONID       "SESSIONID"
#ifdef COMMIT
    #undef COMMIT
#endif
#define COMMIT          "COMMIT"
#define INVALID_PARAM   "INVALID_PARAM"
#define RESULT          "RESULT"
#define OBJNAME         "OBJNAME"
#define INST_NUM        "INST_NUM"
#define PRIORITY        "PRIORITY"
#define PARAM_NAME      "PARAM_NAME"
#define NEXT_LEVEL      "NEXT_LEVEL"


typedef void*(*CCSP_MESSAGE_BUS_MALLOC) ( size_t size ); // this signature is different from standard malloc
typedef void (*CCSP_MESSAGE_BUS_FREE)   ( void * ptr );

/*
 * Minimal CCSP_MESSAGE_BUS_INFO — the USP adapter (CCSP_USP_BUS_HANDLE)
 * embeds this layout as its common prefix so that casts from void *bus_handle
 * to CCSP_MESSAGE_BUS_INFO * remain safe.
 */
typedef struct _CCSP_MESSAGE_BUS_INFO
{
    char                    component_id[256];
    CCSP_MESSAGE_BUS_MALLOC mallocfunc;
    CCSP_MESSAGE_BUS_FREE   freefunc;

} CCSP_MESSAGE_BUS_INFO;

typedef struct _CCSP_DEADLOCK_DETECTION_INFO
{
    pthread_mutex_t info_mutex;
    char *          messageType;
    void *          parameterInfo;
    unsigned long   size;
    unsigned long   enterTime;
    unsigned long   detectionDuration;
    unsigned long   timepassed;

} CCSP_DEADLOCK_DETECTION_INFO;

#define deadlock_detection_log_linenum 200
#define deadlock_detection_log_linelen 192
#define deadlock_detection_log_file    "/var/log/ccsp_emergency.log"

typedef char DEADLOCK_ARRAY[deadlock_detection_log_linenum][deadlock_detection_log_linelen];

// EXTERNAL INTERFACES

void CCSP_Msg_SleepInMilliSeconds(int milliSecond);

int  CCSP_Msg_IsRbus_enabled(void);

/*if mallocfunc, freefunc,config_file is NULL, default value will be used */
int CCSP_Message_Bus_Init
(
    char*             component_id,
    char * config_file,
    void **bus_handle,
    CCSP_MESSAGE_BUS_MALLOC mallocfunc,
    CCSP_MESSAGE_BUS_FREE   freefunc
);

void CCSP_Message_Bus_Exit
(
    void *bus_handle
);

/*can be called multi-time, sender, path,interface,event_name at least 1 is not null*/
int  CCSP_Message_Bus_Register_Event
(
    void* bus_handle,
    const char* sender,
    const char* path,
    const char* interface,
    const char* event_name
);

int  CCSP_Message_Bus_UnRegister_Event
(
    void* bus_handle,
    const char* sender,
    const char* path,
    const char* interface,
    const char* event_name
);

void  CCSP_Message_Bus_Set_Event_Callback
(
    void* bus_handle,
    void*   callback,
    void * user_data
);

/*can register more than one time*/
#define CCSP_Message_Bus_Register_Path CCSP_Message_Bus_Register_Path2

int CCSP_Message_Bus_Register_Path2
(
    void* bus_handle,
    const char* path,
    void* funcptr,
    void * user_data
);

/*send on a connection, private function — not supported in USP mode */
int  CCSP_Message_Bus_Send_Str
(
    void *conn,
    char* component_id,
    const char* path,
    const char* interface,
    const char* method,
    char* request
);

int CCSP_Message_Bus_Send_Msg
(
    void* bus_handle,
    void *message,
    int timeout_seconds,
    void **result
);

/*not recommended  for multithread high speed app, it will crash */
int CCSP_Message_Bus_Send_Msg_Block
(
    void* bus_handle,
    void *message,
    int timeout_seconds,
    void **result
);

#define DBUS_REGISTER_PATH_TIMES 10

#endif

