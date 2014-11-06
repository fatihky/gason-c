#include "gason-c.h"

GASON_C_CPP(extern "C" {)

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define __ printf("%s:%d\n", __func__, __LINE__);

#define new_c(x) (x *)malloc(sizeof(x))

#define G_JSON_ZONE_SIZE 4096
#define G_JSON_STACK_SIZE 32

typedef struct gason_allocator_Zone Zone;

/*
 * gason_value_t
 */

gason_value_t *gason_value_new()
{
	gason_value_t *ret = new_c(gason_value_t);
	ret->ival = G_JSON_VALUE_NULL;
	return ret;
}

gason_value_t *gason_value_new_double(double x)
{
	gason_value_t *ret = new_c(gason_value_t);
	ret->fval = x;
	return ret;
}

gason_value_t *gason_value_new_type(gason_tag_t tag, void *p)
{
    uint64_t x = (uint64_t)p;
    assert(tag <= G_JSON_VALUE_TAG_MASK);
    assert(x <= G_JSON_VALUE_PAYLOAD_MASK);
    gason_value_t *ret = new_c(gason_value_t);
    ret->ival = G_JSON_VALUE_NAN_MASK | ((uint64_t)tag << G_JSON_VALUE_TAG_SHIFT) | x;
	return ret;
}

bool gason_value_is_double(gason_value_t *v)
{
    return (int64_t)v->ival <= (int64_t)G_JSON_VALUE_NAN_MASK;
}

gason_tag_t gason_value_get_tag(gason_value_t *v)
{
    return gason_value_is_double(v) ? G_JSON_NUMBER : (enum gason_tag)((v->ival >> G_JSON_VALUE_TAG_SHIFT) & G_JSON_VALUE_TAG_MASK);
}

uint64_t gason_value_get_payload(gason_value_t *v)
{
    assert(!gason_value_is_double(v));
    return v->ival & G_JSON_VALUE_PAYLOAD_MASK;
}

double gason_value_to_number(gason_value_t *v)
{
    assert(gason_value_get_tag(v) == G_JSON_NUMBER);
    return v->fval;
}

bool gason_value_to_bool(gason_value_t *v)
{
    assert(gason_value_get_tag(v) == G_JSON_BOOL);
    return (bool)gason_value_get_payload(v);
}

char *gason_value_to_string(gason_value_t *v)
{
    assert(gason_value_get_tag(v) == G_JSON_STRING);
    return (char *)gason_value_get_payload(v);
}

gason_node_t *gason_value_to_node(gason_value_t *v)
{
    assert(gason_value_get_tag(v) == G_JSON_ARRAY || gason_value_get_tag(v) == G_JSON_OBJECT);
    return (gason_node_t *)gason_value_get_payload(v);
}

/*
 * gason_node_t
 */

gason_node_t *gason_node_new()
{
    gason_node_t *ret = new_c(gason_node_t);
    memset(ret, '\0', sizeof(gason_node_t));
    return ret;
}

gason_value_t gason_node_val(gason_node_t *n)
{
    return n->value;
}

gason_node_t *gason_node_next(gason_node_t *n)
{
    return n->next;
}

char *gason_node_key(gason_node_t *n)
{
	return n->key;
}

/*
 * gason_iterator_t
 */

gason_iterator_t *gason_iterator_new()
{
	gason_iterator_t *ret = new_c(gason_iterator_t);
	ret->p = NULL;
	return ret;
}

void gason_iterator_walk(gason_iterator_t *it)
{
	it->p = it->p->next;
}

bool gason_iterator_end(gason_iterator_t *it)
{
	return it->p->next ? true : false;
}

gason_node_t *gason_iterator_node(gason_iterator_t *it)
{
	return it->p;
}

const char *gason_str_error(int err)
{
    switch (err) {
#define XX(no, str) \
    case GASON_##no: \
        return str;
        G_JSON_ERRNO_MAP(XX)
#undef XX
    default:
        return "unknown";
    }
}

gason_allocator_t *gason_allocator_new()
{
    gason_allocator_t *ret = new_c(gason_allocator_t);
    memset(ret, '\0', sizeof(gason_allocator_t));
    return ret;
}

void *gason_allocator_allocate(gason_allocator_t *a, size_t size)
{
    size = (size + 7) & ~7;

    if (a->head && a->head->used + size <= G_JSON_ZONE_SIZE)
    {
        char *p = (char *)a->head + a->head->used;
        a->head->used += size;
        return p;
    }

    size_t allocSize = sizeof(Zone) + size;
    Zone *zone = (Zone *)malloc(allocSize <= G_JSON_ZONE_SIZE ? G_JSON_ZONE_SIZE : allocSize);
    zone->used = allocSize;
    if (allocSize <= G_JSON_ZONE_SIZE || a->head == NULL)
    {
		zone->next = a->head;
		a->head = zone;
    } else {
		zone->next = a->head->next;
		a->head->next = zone;
    }
    return (char *)zone + sizeof(Zone);
}

void gason_allocator_deallocate(gason_allocator_t *a)
{
	while (a->head)
	{
		Zone *next = a->head->next;
		free(a->head);
		a->head = next;
	}
}

#ifdef __SSE4_2__
#include <nmmintrin.h>
#define cmpistri(haystack, needle, flags)                             \
    do {                                                              \
        __m128i a = _mm_loadu_si128((const __m128i *)(needle));       \
        int i;                                                        \
        do {                                                          \
            __m128i b = _mm_loadu_si128((const __m128i *)(haystack)); \
            i = _mm_cmpistri(a, b, flags);                            \
            haystack += i;                                            \
        } while (i == 16);                                            \
    } while (0)
#endif

/*
static inline bool isspace(char c) {
    return c == ' ' || (c >= '\t' && c <= '\r');
}
*/

static inline bool isdelim(char c) {
    return c == ',' || c == ':' || c == ']' || c == '}' || isspace(c) || !c;
}

/*
static inline bool isdigit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool isxdigit(char c) {
    return (c >= '0' && c <= '9') || ((c & ~' ') >= 'A' && (c & ~' ') <= 'F');
}*/

static inline int char2int(char c) {
    if (c <= '9')
        return c - '0';
    return (c & ~' ') - 'A' + 10;
}

static double string2double(char *s, char **endptr) {
    char ch = *s;
    if (ch == '+' || ch == '-')
        ++s;

    double result = 0;
    while (isdigit(*s))
        result = (result * 10) + (*s++ - '0');

    if (*s == '.') {
        ++s;

        double fraction = 1;
        while (isdigit(*s)) {
            fraction *= 0.1;
            result += (*s++ - '0') * fraction;
        }
    }

    if (*s == 'e' || *s == 'E') {
        ++s;

        double base = 10;
        if (*s == '+')
            ++s;
        else if (*s == '-') {
            ++s;
            base = 0.1;
        }

        int exponent = 0;
        while (isdigit(*s))
            exponent = (exponent * 10) + (*s++ - '0');

        double power = 1;
        for (; exponent; exponent >>= 1, base *= base)
            if (exponent & 1)
                power *= base;

        result *= power;
    }

    *endptr = s;
    return ch == '-' ? -result : result;
}

static inline gason_node_t *insertAfter(gason_node_t *tail, gason_node_t *node)
{
    if(!tail)
        return node->next = node;
    node->next = tail->next;
    tail->next = node;
    return node;
}

static inline gason_value_t *listToValue(gason_tag_t tag, gason_node_t *tail)
{
	if (tail)
	{
		gason_node_t *head = tail->next;
		tail->next = NULL;
		return gason_value_new_type(tag, head);
	}
	return gason_value_new_type(tag, NULL);
}

int gason_parse(char *s, char **endptr, gason_value_t **value, gason_allocator_t **al)
{
	gason_node_t *tails[G_JSON_STACK_SIZE];
	gason_tag_t tags[G_JSON_STACK_SIZE];
	char *keys[G_JSON_STACK_SIZE];
	gason_value_t *o;
	int pos = -1;
	bool separator = true;
	*endptr = s;

	while (*s)
	{
#ifdef __SSE4_2__
		cmpistri(s, "\x20\t\n\v\f\r", _SIDD_NEGATIVE_POLARITY);
#else
		while (isspace(*s)) ++s;
#endif
		*endptr = s++;
		switch (**endptr)
		{
		case '-':
			if (!isdigit(*s) && *s != '.')
			{
				*endptr = s;
				return GASON_BAD_NUMBER;
			}
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			o = gason_value_new_double(string2double(*endptr, &s));
			if (!isdelim(*s))
			{
				*endptr = s;
				return GASON_BAD_NUMBER;
			}
			break;
		case '"':
			o = gason_value_new_type(G_JSON_STRING, s);
#ifdef __SSE4_2__
			cmpistri(s, "\"\\\t\n", 0);
#endif
			for (char *it = s; *s; ++it, ++s)
			{
                int c = *it = *s;
				if (c == '\\')
				{
                    c = *++s;
                    switch (c) {
                    case '\\':
                    case '"':
                    case '/':
                        *it = c;
                        break;
                    case 'b':
                        *it = '\b';
                        break;
                    case 'f':
                        *it = '\f';
                        break;
                    case 'n':
                        *it = '\n';
                        break;
                    case 'r':
                        *it = '\r';
                        break;
                    case 't':
                        *it = '\t';
                        break;
                    case 'u':
                        c = 0;
                        for (int i = 0; i < 4; ++i) {
                            if (isxdigit(*++s)) {
                                c = c * 16 + char2int(*s);
                            } else {
                                *endptr = s;
                                return GASON_BAD_STRING;
                            }
                        }
                        if (c < 0x80) {
                            *it = c;
                        } else if (c < 0x800) {
                            *it++ = 0xC0 | (c >> 6);
                            *it = 0x80 | (c & 0x3F);
                        } else {
                            *it++ = 0xE0 | (c >> 12);
                            *it++ = 0x80 | ((c >> 6) & 0x3F);
                            *it = 0x80 | (c & 0x3F);
                        }
                        break;
                    default:
                        *endptr = s;
                        return GASON_BAD_STRING;
                    }
                } else if ((unsigned int)c < ' ' || c == '\x7F') {
                    *endptr = s;
                    return GASON_BAD_STRING;
                } else if (c == '"') {
                    *it = 0;
                    ++s;
                    break;
                }
            }
            if (!isdelim(*s)) {
                *endptr = s;
                return GASON_BAD_STRING;
            }
            break;
        case 't':
            for (const char *it = "rue"; *it; ++it, ++s) {
                if (*it != *s)
                    return GASON_BAD_IDENTIFIER;
            }
            if (!isdelim(*s))
                return GASON_BAD_IDENTIFIER;
            o = gason_value_new_type(G_JSON_BOOL, (void *)true);
            break;
        case 'f':
            for (const char *it = "alse"; *it; ++it, ++s) {
                if (*it != *s)
                    return GASON_BAD_IDENTIFIER;
            }
            if (!isdelim(*s))
                return GASON_BAD_IDENTIFIER;
            o = gason_value_new_type(G_JSON_BOOL, (void *)false);
            break;
        case 'n':
            for (const char *it = "ull"; *it; ++it, ++s) {
                if (*it != *s)
                    return GASON_BAD_IDENTIFIER;
            }
            if (!isdelim(*s))
                return GASON_BAD_IDENTIFIER;
            o = gason_value_new();
            break;
        case ']':
            if (pos == -1)
                return GASON_STACK_UNDERFLOW;
            if (tags[pos] != G_JSON_ARRAY)
                return GASON_MISMATCH_BRACKET;
            o = listToValue(G_JSON_ARRAY, tails[pos--]);
            break;
        case '}':
            if (pos == -1)
                return GASON_STACK_UNDERFLOW;
            if (tags[pos] != G_JSON_OBJECT)
                return GASON_MISMATCH_BRACKET;
            if (keys[pos] != NULL)
                return GASON_UNEXPECTED_CHARACTER;
            o = listToValue(G_JSON_OBJECT, tails[pos--]);
            break;
        case '[':
            if (++pos == G_JSON_STACK_SIZE)
                return GASON_STACK_OVERFLOW;
            tails[pos] = NULL;
            tags[pos] = G_JSON_ARRAY;
            keys[pos] = NULL;
            separator = true;
            continue;
        case '{':
            if (++pos == G_JSON_STACK_SIZE)
                return GASON_STACK_OVERFLOW;
            tails[pos] = NULL;
            tags[pos] = G_JSON_OBJECT;
            keys[pos] = NULL;
            separator = true;
            continue;
        case ':':
            if (separator || keys[pos] == NULL)
                return GASON_UNEXPECTED_CHARACTER;
            separator = true;
            continue;
        case ',':
            if (separator || keys[pos] != NULL)
                return GASON_UNEXPECTED_CHARACTER;
            separator = true;
            continue;
        case '\0':
            continue;
        default:
            return GASON_UNEXPECTED_CHARACTER;
        }

        separator = false;

        if (pos == -1)
        {
            *endptr = s;
            *value = o;
            return GASON_OK;
        }

        if (tags[pos] == G_JSON_OBJECT) {
            if (!keys[pos])
            {
                if (gason_value_get_tag(o) != G_JSON_STRING)
                    return GASON_UNQUOTED_KEY;
                keys[pos] = gason_value_to_string(o);
                continue;
            }
            tails[pos] = insertAfter(tails[pos], (gason_node_t *)gason_allocator_allocate(*al, sizeof(gason_node_t)));
            tails[pos]->key = keys[pos];
            keys[pos] = NULL;
        } else {
            tails[pos] = insertAfter(tails[pos], (gason_node_t *)gason_allocator_allocate(*al, sizeof(gason_node_t) - sizeof(char *)));
        }
        tails[pos]->value = *o;
    }
    return GASON_BREAKING_BAD;
}

GASON_C_CPP(})
