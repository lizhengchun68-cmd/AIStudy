#include "scheduler/context_store_errors.h"

namespace AIstudy {
namespace scheduler {
namespace {

thread_local std::string g_last_context_store_detail;

} // namespace

const std::string& lastContextStoreDetail() {
    return g_last_context_store_detail;
}

void clearLastContextStoreDetail() {
    g_last_context_store_detail.clear();
}

void recordContextStoreFailure(const std::string& detail) {
    g_last_context_store_detail = detail;
}

} // namespace scheduler
} // namespace AIstudy
