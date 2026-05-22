#ifndef AISTUDY_HOST_SKILL_HOST_H
#define AISTUDY_HOST_SKILL_HOST_H

/**
 * @file skill_host.h
 * @brief Skill Host facade: list / describe / health / execute (M6).
 */

#include "host/host_exit_codes.h"
#include "scheduler/Dispatcher.h"
#include <string>

namespace AIstudy {
namespace host {

/**
 * @brief Owns Dispatcher + skill registration; single entry for CLI/stdio/HTTP.
 */
class SkillHost {
public:
    SkillHost();

    void logLoadFailuresToStderr() const;

    std::string listSkillsJson() const;
    std::string describeSkillJson(const std::string& skill_id) const;
    std::string healthJson() const;
    std::string execute(const std::string& envelope_json);

    /** @brief Drop in-memory session only (M7a ops); returns JSON. */
    std::string dropContextJson(const std::string& context_id);

    /** @brief Map protocol v1 execute response JSON to process exit code. */
    static HostExitCode exitCodeFromExecuteResponse(const std::string& response_json);

    scheduler::Dispatcher& dispatcher() { return dispatcher_; }
    const scheduler::Dispatcher& dispatcher() const { return dispatcher_; }

private:
    scheduler::Dispatcher dispatcher_;
};

} // namespace host
} // namespace AIstudy

#endif // AISTUDY_HOST_SKILL_HOST_H
