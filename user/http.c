struct http_header {
  char* buffer;
  struct http_header* next;
};

struct http_context {
  struct http_header* header;
  char* content;
  uint32_t size;
  uint32_t max_size;
};

uint8_t http_create(struct http_context** context_ptr, char* path, char* method, uint32_t max_size) 
{
  struct http_context* context = os_malloc(sizeof(struct http_context));
  context_ptr = &context;
  context->content = os_malloc(max_size);
  context->max_size = max_size;
  context->header = os_malloc(sizeof(struct http_header));
  h->next = NULL;
  return 0;
}

uint8_t http_header_add(struct http_context* context, char* header)
{
  uint32_t len = os_strlen(header);
  // create new buffers and copy into them...
  char* header_buffer = os_malloc(len+1);
  os_memcpy(header_buffer, header, len+1);
  struct http_header* h = context->header;
  while (h->next != NULL) h=h->next;
  h->next = os_malloc(sizeof(struct http_header));
  h->buffer = header_buffer;
  return 0;
}

uint8_t http_write(struct http_context* context)

uint8_t http_buffer_export()
{

}

uint8_t http_destroy()
{

}
