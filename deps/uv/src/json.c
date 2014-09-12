#define JSON_TOKENS 256

#include "jsmn.h"

int json_token_streq(char *js, jsmntok_t *t, char *s)
{
    return (strncmp(js + t->start, s, t->end - t->start) == 0
            && strlen(s) == (size_t) (t->end - t->start));
}

jsmntok_t *uv_json_tokenise(char *js, size_t len)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    unsigned int n = JSON_TOKENS;
    jsmntok_t *tokens = malloc(sizeof(jsmntok_t) * n);
    assert(tokens != NULL);

    int ret = jsmn_parse(&parser, js, len, tokens, n);

    while (ret == JSMN_ERROR_NOMEM)
    {
        n = n * 2 + 1;
        tokens = realloc(tokens, sizeof(jsmntok_t) * n);
        assert(tokens != NULL);
        ret = jsmn_parse(&parser, js, len, tokens, n);
    }

    if (ret == JSMN_ERROR_INVAL) {
        printf("jsmn_parse: invalid JSON string");
        abort();
    }
    if (ret == JSMN_ERROR_PART) {
        printf("jsmn_parse: truncated JSON string");
        abort();
    }

    return tokens;
}

int uv_parse_json_int(char *js, size_t len) {
  jsmntok_t *tokens = uv_json_tokenise(js, len);
  
  typedef enum { START, KEY, RETVAL, SKIP, STOP } parse_state;
  parse_state state = START;

  size_t object_tokens = 0;
  size_t i, j;

  for (i = 0, j = 1; j > 0; i++, j--) {
    jsmntok_t * t = &tokens[i];

    // Should never reach uninitialized tokens
    assert(t->start != -1 && t->end != -1);

    if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
      j += t->size;

    switch (state) {
      case START:
        if (t->type != JSMN_OBJECT) {
          printf("Invalid response: root element must be an object.");
          abort();
        }

        state = KEY;
        object_tokens = t->size;

        if (object_tokens == 0)
          state = STOP;

        if (object_tokens % 2 != 0) {
          printf("Invalid response: object must have even number of children.");
          abort();
        }

        break;

      case KEY:
        object_tokens--;

        if (t->type != JSMN_STRING) {
          printf("Invalid response: object keys must be strings.");
          abort();
        }

        state = SKIP;

        if (json_token_streq(js, t, "result")) {
          state = RETVAL;
        }

        break;

      case SKIP:
        if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE) {
          printf("Invalid response: object values must be strings or primitives.");
          abort();
        }

        object_tokens--;
        state = KEY;

        if (object_tokens == 0)
          state = STOP;

        break;

      case RETVAL:
        if (t->type != JSMN_PRIMITIVE) {
          printf("Invalid response: object values must be strings or primitives.");
          abort();
        }

        char *end;
        long int value = strtol(js+t->start, &end, 10);
        if (end != (js+t->end)) {
          printf("Invalid response: response is not an integer.");
          abort();
        }
        
        free(tokens);
        return value;

      case STOP:
        // Just consume the tokens
        break;

      default:
        printf("Invalid state %u", state);
        abort();
    }
  }
}