#pragma once

#include "http_defines.h"
#include "http_request.h"
#include "http_response.h"

HttpResponse http_handler(const HttpRequest &request, const std::string &root = NO_ROOT, const bool is_forbidden = false);
