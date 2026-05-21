#ifndef AISTUDY_SCHEDULER_SKILL_REGISTRY_H
#define AISTUDY_SCHEDULER_SKILL_REGISTRY_H

#include "scheduler/Dispatcher.h"
#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief 项目根目录（编译期 AISTUDY_PROJECT_ROOT 或运行时回退） */
std::string projectRootPath();

/** @brief 从 skills/<id>/manifest.json 加载并注册所有内置 Skill */
void registerBuiltinSkills(Dispatcher& dispatcher);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_REGISTRY_H
