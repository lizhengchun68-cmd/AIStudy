#ifndef AISTUDY_SCHEDULER_CONTEXT_CLOSE_HANDLER_H
#define AISTUDY_SCHEDULER_CONTEXT_CLOSE_HANDLER_H

#include "scheduler/Dispatcher.h"

namespace AIstudy {
namespace scheduler {

StatusOr<SkillResultJson> context_close_execute(const std::string& payload_json);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_CONTEXT_CLOSE_HANDLER_H
