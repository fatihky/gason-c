#include "gason-c.c"

char *strdup(char *input)
{
  size_t len = strlen(input);
  char *ret = (char *)malloc(len + 1);
  memcpy(ret, input, len);
  ret[len] = '\0';
  return ret;
}

int main(int argc, char *argv[])
{
  char *json = strdup("{\"fatih\": \"kaya\"}");
  char *endptr = NULL;
  gason_value_t *value = NULL;
  gason_allocator_t *al = gason_allocator_new();
  int result;

  result = gason_parse(json, &endptr, &value, &al);
  printf("gason_parse result: %s\n", gason_str_error(result));

  size_t length;
  char *ret = gason_encode(value, &length, 1);
  printf("encoded(pretty: 1): \n%s\n", ret);

  gason_allocator_deallocate(al);
  return 0;
}
