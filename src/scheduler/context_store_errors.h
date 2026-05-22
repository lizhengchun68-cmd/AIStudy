#ifndef AISTUDY_SCHEDULER_CONTEXT_STORE_ERRORS_H
#define AISTUDY_SCHEDULER_CONTEXT_STORE_ERRORS_H

#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief Last ContextStore / skill_execution_context failure detail (thread-local). */
const std::string& lastContextStoreDetail();

void clearLastContextStoreDetail();

/** @brief Set thread-local detail before returning StatusOr::Fail from ContextStore. */
void recordContextStoreFailure(const std::string& detail);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_CONTEXT_STORE_ERRORS_H
