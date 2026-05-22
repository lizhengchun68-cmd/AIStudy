#ifndef AISTUDY_SCHEDULER_SKILL_REGISTRY_H
#define AISTUDY_SCHEDULER_SKILL_REGISTRY_H

#include "scheduler/Dispatcher.h"
#include <string>

namespace AIstudy {
namespace scheduler {

/** @brief 项目根目录（编译期 AISTUDY_PROJECT_ROOT 或运行时回退） */
std::string projectRootPath();

/** @brief 从 skills/<id>/manifest.json 加载并注册所有内置 Skill（使用编译期/默认项目根） */
void registerBuiltinSkills(Dispatcher& dispatcher);

/** @brief 同上，显式指定项目根（供测试或自定义部署目录） */
void registerBuiltinSkills(Dispatcher& dispatcher, const std::string& project_root);

} // namespace scheduler
} // namespace AIstudy

#endif // AISTUDY_SCHEDULER_SKILL_REGISTRY_H
