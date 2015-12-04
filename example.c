#include "gason-c.c"
#include "util.c"
#include <stdlib.h>
#if !defined(_WIN32) && !defined(NDEBUG)
#include <execinfo.h>
#include <signal.h>
#endif

char *strdup(char *input)
{
  size_t len = strlen(input);
  char *ret = (char *)malloc(len + 1);
  memcpy(ret, input, len);
  ret[len] = '\0';
  return ret;
}

void parse() {
  char *json = strdup("{\"fatih\": 274}");
  char *endptr = NULL;
  gason_value_t *value = NULL;
  gason_allocator_t *al = gason_allocator_new();
  int result;

  result = gason_parse(json, &endptr, &value, &al);
  printf("gason_parse result: %s\n", gason_str_error(result));

  size_t length;
  char *ret = gason_encode(value, &length, 0);
  printf("encoded(pretty: 0): \n%s\n", ret);

  gason_allocator_deallocate(al);
  free(ret);
  free(json);
}

void basic_examples() {
  gason_allocator_t *al = gason_allocator_new();

  gason_value_t *val;

  // string value

  char *str = "hello gason!";
  val = gason_value_new_type(al, G_JSON_STRING, (void *)str);

  printf("String value: ");
  dump_value(val, 0);
  putchar('\n');

  // double/number value

  val = gason_value_new_double(al, 26584.2554);

  printf("Double value: ");
  dump_value(val, 0);
  putchar('\n');

  // boolean true

  val = gason_value_new_type(al, G_JSON_TRUE, NULL);

  printf("Boolean true: ");
  dump_value(val, 0);
  putchar('\n');

  // boolean false

  val = gason_value_new_type(al, G_JSON_FALSE, NULL);

  printf("Boolean false: ");
  dump_value(val, 0);
  putchar('\n');

  // null

  val = gason_value_new_type(al, G_JSON_NULL, NULL);

  printf("null: ");
  dump_value(val, 0);
  putchar('\n');

  gason_allocator_deallocate(al);
}

void create_object() {
  gason_allocator_t *al = gason_allocator_new();
  gason_value_t *object = NULL;
  gason_value_t *val = NULL;
  gason_node_t *node = NULL;

  val = gason_value_new_double(al, 274);
  node = (gason_node_t *)gason_allocator_allocate(al, sizeof(gason_node_t));
  node->key = "x";
  node->value = *val;
  node->next = NULL;
  object = gason_value_new_type(al, G_JSON_OBJECT, node);

  gason_object_add_string(al, object, "foo", "bar");

  printf("Object created. Here is a dump:\n");
  dump_value(object, 0);
  putchar('\n');

  gason_allocator_deallocate(al);
}

void create_array() {
  gason_allocator_t *al = gason_allocator_new();
  gason_value_t *array;
  gason_value_t *val;
  gason_node_t *node;
  gason_node_t *tmp;

  val = gason_value_new_double(al, 274);
  node = (gason_node_t *)gason_allocator_allocate(al, sizeof(gason_node_t));
  node->key = NULL;
  node->value = *val;
  node->next = NULL;

  val = gason_value_new_double(al, 227);
  tmp = node;
  node = (gason_node_t *)gason_allocator_allocate(al, sizeof(gason_node_t));
  node->key = NULL;
  node->value = *val;
  node->next = NULL;
  tmp->next = node;

  array = gason_value_new_type(al, G_JSON_ARRAY, tmp);

  printf("Array created. Here is a dump:\n");
  dump_value(array, 0);
  putchar('\n');

  gason_allocator_deallocate(al);
}

void on_abort(int x) {
	void *callstack[64];
	int size = backtrace(callstack, sizeof(callstack)/sizeof(callstack[0]));
	char **strings = backtrace_symbols(callstack, size);
	for (int i = 0; i < size; ++i)
		fprintf(stderr, "%s\n", strings[i]);
	free(strings);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
#if !defined(_WIN32) && !defined(NDEBUG)
  signal(SIGABRT, on_abort);
#endif
  // parsing and encoding example
  printf("Parsing encoding example ========================================\n");
  parse();

  // dynamically json value creation example
  printf("\nBasic gason_value creation examples =============================\n");
  basic_examples();

  // complex examples
  printf("\nObject creation ===============================================\n");
  create_object();

  printf("\nArray creation ================================================\n");
  create_array();

  return 0;
}
