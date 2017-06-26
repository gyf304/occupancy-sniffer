#define HTTP_MAX_HEADERS 16

typedef struct http_header {
  char* name;
  char* value;
} http_header_t;

enum http_method {
  GET = 0,
  POST,
  UPDATE,
  DELETE
}

typedef struct http_context {

} http_context_t;

typedef struct http_request {
  char* url;
  enum http_method method;
  http_header_t headers;
  uint8_t header_count;
  char* body;
  uint32_t length;
} http_request_t;

typedef struct http_response {
  uint32_t status;
  http_header_t headers;
  uint8_t header_count;
  char* body;
  uint32_t length;
} http_response_t;

uint8_t http_send(http_request_t* request, http_response_t* response);
