#ifndef VIEWS_H
#define VIEWS_H

#include "http.h"
HttpResponse *index_view(HttpRequest request);
HttpResponse *echo_view(HttpRequest request);
HttpResponse *user_agent_view(HttpRequest request);
HttpResponse *files_view(HttpRequest request);
HttpResponse *post_files_view(HttpRequest request);
#endif
