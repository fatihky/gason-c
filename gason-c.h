#ifndef _GASON_C_H
#define _GASON_C_H

#ifdef __cplusplus
  #define GASON_C_CPP(x) x
#else 
  #define GASON_C_CPP(x)
#endif

GASON_C_CPP(extern "C" {)

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>

#define G_JSON_VALUE_PAYLOAD_MASK 0x00007FFFFFFFFFFFULL
#define G_JSON_VALUE_NAN_MASK 0x7FF8000000000000ULL
#define G_JSON_VALUE_NULL 0x7FFF800000000000ULL
#define G_JSON_VALUE_TAG_MASK 0xF
#define G_JSON_VALUE_TAG_SHIFT 47

enum gason_tag {
  G_JSON_NUMBER = 0,
  G_JSON_STRING,
  G_JSON_TRUE,
  G_JSON_FALSE,
  G_JSON_ARRAY,
  G_JSON_OBJECT,
  G_JSON_NULL = 0xF
};

typedef enum    gason_tag           gason_tag_t;
typedef struct  gason_value_s       gason_value_t;
typedef struct  gason_node_s        gason_node_t;
typedef struct  gason_iterator_s    gason_iterator_t;
typedef struct  gason_allocator_s   gason_allocator_t;

struct  gason_value_s {
  uint64_t ival;
  double fval;
};

struct  gason_node_s {
  gason_value_t value;
  gason_node_t *next;
  char *key;
};

struct  gason_iterator_s {
  gason_node_t *p;
};

struct gason_allocator_Zone {
  struct gason_allocator_Zone *next;
  size_t used;
};

struct gason_allocator_s {
	struct gason_allocator_Zone *head;
};

gason_value_t *gason_value_new();
void gason_value_free(gason_value_t *self);
gason_value_t *gason_value_new_double(gason_allocator_t *al, double x);
gason_value_t *gason_value_new_type(gason_allocator_t *al, gason_tag_t tag, void *p);
bool           gason_value_is_double(gason_value_t *v);
gason_tag_t    gason_value_get_tag(gason_value_t *v);
void           gason_value_set_payload(gason_value_t *v, gason_tag_t tag, void *p);
uint64_t       gason_value_get_payload(gason_value_t *v);
double         gason_value_to_number(gason_value_t *v);
// bool           gason_value_to_bool(gason_value_t *v);
char          *gason_value_to_string(gason_value_t *v);
gason_node_t  *gason_value_to_node(gason_value_t *v);

// object methods
const gason_value_t *gason_object_get_prop(gason_value_t *v, char *p);
const gason_value_t *gason_array_get_prop(gason_value_t *v, int index);
int gason_value_insert_child(gason_allocator_t *al,
  gason_value_t *self,
  char *propName,
  gason_value_t val);
int gason_value_add_string(gason_allocator_t *al, gason_value_t *self,
  char *propName,
  char *value);
int gason_value_add_number(gason_allocator_t *al, gason_value_t *self,
  char *propName,
  double value);
int gason_value_add_bool(gason_allocator_t *al, gason_value_t *self,
  char *propName,
  bool value);
int gason_value_add_null(gason_allocator_t *al, gason_value_t *self,
  char *propName);

gason_node_t  *gason_node_new();
gason_value_t  gason_node_val(gason_node_t *n);
gason_node_t  *gason_node_next(gason_node_t *n);
char          *gason_node_key(gason_node_t *n);

gason_iterator_t *gason_iterator_new();
void gason_iterator_walk(gason_iterator_t *it); // ++ operator in c++
bool gason_iterator_end(gason_iterator_t *it);
gason_node_t *gason_iterator_node(gason_iterator_t *it);

#define G_JSON_ERRNO_MAP(XX)                           \
    XX(OK, "ok")                                     \
    XX(BAD_NUMBER, "bad number")                     \
    XX(BAD_STRING, "bad string")                     \
    XX(BAD_IDENTIFIER, "bad identifier")             \
    XX(STACK_OVERFLOW, "stack overflow")             \
    XX(STACK_UNDERFLOW, "stack underflow")           \
    XX(MISMATCH_BRACKET, "mismatch bracket")         \
    XX(UNEXPECTED_CHARACTER, "unexpected character") \
    XX(UNQUOTED_KEY, "unquoted key")                 \
    XX(BREAKING_BAD, "breaking bad")                 \
    XX(ALLOCATION_FAILURE, "allocation failure")

enum gason_errno {
#define XX(no, str) GASON_##no,
    G_JSON_ERRNO_MAP(XX)
#undef XX
};

const char *gason_str_error(int err);

gason_allocator_t *gason_allocator_new();
void *gason_allocator_allocate(gason_allocator_t *a, size_t size);
void gason_allocator_deallocate(gason_allocator_t *a);

int gason_parse(char *s, char **endptr, gason_value_t **value, gason_allocator_t **al);
char *gason_encode(gason_value_t *val, size_t *length, int pretty);

GASON_C_CPP(})

#endif
