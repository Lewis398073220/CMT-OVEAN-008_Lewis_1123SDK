/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.3 */

#ifndef PB_GOOGLE_PROTOBUF_STRUCT_PB_H_INCLUDED
#define PB_GOOGLE_PROTOBUF_STRUCT_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _google_protobuf_NullValue {
    google_protobuf_NullValue_NULL_VALUE = 0
} google_protobuf_NullValue;

/* Struct definitions */
typedef struct _google_protobuf_ListValue {
    pb_callback_t values;
} google_protobuf_ListValue;

typedef struct _google_protobuf_Struct {
    pb_callback_t fields;
} google_protobuf_Struct;

typedef struct _google_protobuf_Value {
    pb_size_t which_kind;
    union {
        google_protobuf_NullValue null_value;
        double number_value;
        char string_value[100];
        bool bool_value;
    } kind;
} google_protobuf_Value;

typedef struct _google_protobuf_Struct_FieldsEntry {
    pb_callback_t key;
    bool has_value;
    google_protobuf_Value value;
} google_protobuf_Struct_FieldsEntry;


/* Helper constants for enums */
#define _google_protobuf_NullValue_MIN google_protobuf_NullValue_NULL_VALUE
#define _google_protobuf_NullValue_MAX google_protobuf_NullValue_NULL_VALUE
#define _google_protobuf_NullValue_ARRAYSIZE ((google_protobuf_NullValue)(google_protobuf_NullValue_NULL_VALUE+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define google_protobuf_Struct_init_default      {{{NULL}, NULL}}
#define google_protobuf_Struct_FieldsEntry_init_default {{{NULL}, NULL}, false, google_protobuf_Value_init_default}
#define google_protobuf_Value_init_default       {0, {_google_protobuf_NullValue_MIN}}
#define google_protobuf_ListValue_init_default   {{{NULL}, NULL}}
#define google_protobuf_Struct_init_zero         {{{NULL}, NULL}}
#define google_protobuf_Struct_FieldsEntry_init_zero {{{NULL}, NULL}, false, google_protobuf_Value_init_zero}
#define google_protobuf_Value_init_zero          {0, {_google_protobuf_NullValue_MIN}}
#define google_protobuf_ListValue_init_zero      {{{NULL}, NULL}}

/* Field tags (for use in manual encoding/decoding) */
#define google_protobuf_ListValue_values_tag     1
#define google_protobuf_Struct_fields_tag        1
#define google_protobuf_Value_null_value_tag     1
#define google_protobuf_Value_number_value_tag   2
#define google_protobuf_Value_string_value_tag   3
#define google_protobuf_Value_bool_value_tag     4
#define google_protobuf_Struct_FieldsEntry_key_tag 1
#define google_protobuf_Struct_FieldsEntry_value_tag 2

/* Struct field encoding specification for nanopb */
#define google_protobuf_Struct_FIELDLIST(X, a) \
X(a, CALLBACK, REPEATED, MESSAGE,  fields,            1)
#define google_protobuf_Struct_CALLBACK pb_default_field_callback
#define google_protobuf_Struct_DEFAULT NULL
#define google_protobuf_Struct_fields_MSGTYPE google_protobuf_Struct_FieldsEntry

#define google_protobuf_Struct_FieldsEntry_FIELDLIST(X, a) \
X(a, CALLBACK, SINGULAR, STRING,   key,               1) \
X(a, STATIC,   OPTIONAL, MESSAGE,  value,             2)
#define google_protobuf_Struct_FieldsEntry_CALLBACK pb_default_field_callback
#define google_protobuf_Struct_FieldsEntry_DEFAULT NULL
#define google_protobuf_Struct_FieldsEntry_value_MSGTYPE google_protobuf_Value

#define google_protobuf_Value_FIELDLIST(X, a) \
X(a, STATIC,   ONEOF,    ENUM,     (kind,null_value,kind.null_value),   1) \
X(a, STATIC,   ONEOF,    DOUBLE,   (kind,number_value,kind.number_value),   2) \
X(a, STATIC,   ONEOF,    STRING,   (kind,string_value,kind.string_value),   3) \
X(a, STATIC,   ONEOF,    BOOL,     (kind,bool_value,kind.bool_value),   4)
#define google_protobuf_Value_CALLBACK NULL
#define google_protobuf_Value_DEFAULT NULL

#define google_protobuf_ListValue_FIELDLIST(X, a) \
X(a, CALLBACK, REPEATED, MESSAGE,  values,            1)
#define google_protobuf_ListValue_CALLBACK pb_default_field_callback
#define google_protobuf_ListValue_DEFAULT NULL
#define google_protobuf_ListValue_values_MSGTYPE google_protobuf_Value

extern const pb_msgdesc_t google_protobuf_Struct_msg;
extern const pb_msgdesc_t google_protobuf_Struct_FieldsEntry_msg;
extern const pb_msgdesc_t google_protobuf_Value_msg;
extern const pb_msgdesc_t google_protobuf_ListValue_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define google_protobuf_Struct_fields &google_protobuf_Struct_msg
#define google_protobuf_Struct_FieldsEntry_fields &google_protobuf_Struct_FieldsEntry_msg
#define google_protobuf_Value_fields &google_protobuf_Value_msg
#define google_protobuf_ListValue_fields &google_protobuf_ListValue_msg

/* Maximum encoded size of messages (where known) */
/* google_protobuf_Struct_size depends on runtime parameters */
/* google_protobuf_Struct_FieldsEntry_size depends on runtime parameters */
#define google_protobuf_Value_size               101
/* google_protobuf_ListValue_size depends on runtime parameters */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
