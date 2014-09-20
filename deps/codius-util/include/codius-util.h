#ifndef __CODIUS_UTIL_H_
#define __CODIUS_UTIL_H_

#define CODIUS_MAX_MESSAGE_SIZE 65536 // 256 MB
#define CODIUS_MAX_RESPONSE_SIZE 268435456 // 256 MB

#ifdef __cplusplus
extern "C" {
#endif

int codius_sync_call(const char* request_buf, size_t request_len,
                     char *response_buf, size_t *response_len);

int codius_parse_json_int(char *js, size_t len);

int codius_parse_json_str(char *js, size_t len, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* __CODIUS_UTIL_H_ */