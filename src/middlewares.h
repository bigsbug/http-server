#ifndef MIDDLWARES_H
#define MIDDLWARES_H

#include "http.h"
void middleware_add_content_length(HttpResponse *response,HttpRequest *request);
void middleware_add_encoding(HttpResponse *response,HttpRequest *request);
void middleware_add_connection_status(HttpResponse *response,HttpRequest *request);
#endif