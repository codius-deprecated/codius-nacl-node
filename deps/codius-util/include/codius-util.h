#define CODIUS_MAX_MESSAGE_SIZE 16384

int codius_sync_call(const char* message, size_t len, char *resp_buf, size_t buf_size);

int codius_parse_json_int(char *js, size_t len);

int codius_parse_json_str(char *js, size_t len, char *buf, size_t buf_size);