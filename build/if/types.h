/*****************************************************************
 * DO NOT EDIT - MACHINE-GENERATED FILE!
 * 
 * Source file : ../../sdios/if/types.idl
 * Platform    : V4 Generic
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.0.2 (roadrunner) on 07/05/2008 19:24
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/

#if !defined(__types_h__)
#define __types_h__

#define IDL4_OMIT_FRAME_POINTER 0
#define IDL4_USE_FASTCALL 0
#define IDL4_NEED_MALLOC 1
#define IDL4_API v4
#define IDL4_ARCH generic

#include "idl4/idl4.h"

#if IDL4_HEADER_REVISION < 20030403
#error You are using outdated versions of the IDL4 header files
#endif /* IDL4_HEADER_REVISION < 20030403 */

#include "sdi/types.h"

#define IF_LOGGING_ID 1

#define IF_LOCATOR_ID 2

#define MAX_INTERFACEID 8

#if !defined(_typedef___interfaceid_t)
#define _typedef___interfaceid_t
typedef CORBA_char interfaceid_t;
#endif /* !defined(_typedef___interfaceid_t) */

#if !defined(_typedef___logmessage_t)
#define _typedef___logmessage_t
typedef CORBA_char *logmessage_t;
#endif /* !defined(_typedef___logmessage_t) */

#if !defined(_typedef___buffer_t)
#define _typedef___buffer_t
typedef struct {
  CORBA_unsigned_long _maximum;
  CORBA_unsigned_long _length;
  CORBA_char *_buffer;
} buffer_t;
#endif /* !defined(_typedef___buffer_t) */

#define ex_type_conflict 1

#if !defined(_typedef___type_conflict)
#define _typedef___type_conflict
typedef struct type_conflict type_conflict;
#endif /* !defined(_typedef___type_conflict) */

#endif /* !defined(__types_h__) */
