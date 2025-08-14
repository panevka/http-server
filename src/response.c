#include <stdio.h>
#include <string.h>

void resolve_request_headers(char *headers, size_t len){
  const size_t MAX_METHOD_LENGTH = 3;
  const size_t MAX_URI_LENGTH = 8000;
  const size_t MAX_PROTOCOL_LENGTH = 20;

  char method[MAX_METHOD_LENGTH + 1] = {0};
  char uri[MAX_URI_LENGTH + 1] = {0};
  char protocol[MAX_PROTOCOL_LENGTH + 1] = {0};

  const char *sp1 = memchr(headers, ' ', len);
  if(!sp1) return;

  size_t method_len = (size_t) (sp1 - headers);
  if(method_len > MAX_METHOD_LENGTH) method_len = MAX_METHOD_LENGTH;
  memcpy(method, headers, method_len);
  method[method_len] = '\0';

  const char *sp2 = memchr(sp1 + 1, ' ', len - (sp1 + 1 - headers));
    if (!sp2) return;

  size_t uri_len = sp2 - (sp1 + 1);
  if (uri_len > MAX_URI_LENGTH) uri_len = MAX_URI_LENGTH;
  memcpy(uri, sp1 + 1, uri_len);
  uri[uri_len] = '\0';

  const char *crlf = memchr(sp2 + 1, '\r', len - (sp2 + 1 - headers));
  if (!crlf) crlf = headers + len; // no CRLF found

  size_t proto_len = crlf - (sp2 + 1);
  if (proto_len > MAX_PROTOCOL_LENGTH) proto_len = MAX_PROTOCOL_LENGTH;
  memcpy(protocol, sp2 + 1, proto_len);
  protocol[proto_len] = '\0';

  printf("Method: %s\n", method);
  printf("URI: %s\n", uri);
  printf("Protocol version: %s\n", protocol);

}
