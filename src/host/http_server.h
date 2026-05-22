#ifndef AISTUDY_HOST_HTTP_SERVER_H
#define AISTUDY_HOST_HTTP_SERVER_H

#include "host/skill_host.h"
#include <string>

namespace AIstudy {
namespace host {

/**
 * @brief Blocking HTTP/1.1 listener for local Agent integration (M6).
 * Routes: GET /v1/health, GET /v1/skills, GET /v1/skills/{id}, POST /v1/execute
 */
int runHttpServer(SkillHost& host, int port);

} // namespace host
} // namespace AIstudy

#endif // AISTUDY_HOST_HTTP_SERVER_H
