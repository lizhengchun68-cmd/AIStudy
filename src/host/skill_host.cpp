#include "host/skill_host.h"
#include "scheduler/skill_context_store.h"
#include "scheduler/skill_registry.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Stringifier.h>
#include <Poco/JSON/Parser.h>
#include <iostream>
#include <sstream>

namespace AIstudy {
namespace host {

SkillHost::SkillHost() {
    scheduler::registerBuiltinSkills(dispatcher_);
}

void SkillHost::logLoadFailuresToStderr() const {
    for (const auto& failure : dispatcher_.loadFailures()) {
        std::cerr << "[AIstudy] WARNING: failed to load skill '" << failure.binding_id
                  << "' from " << failure.manifest_path << ": " << failure.error
                  << std::endl;
    }
}

std::string SkillHost::listSkillsJson() const {
    return dispatcher_.listSkillsJson();
}

std::string SkillHost::describeSkillJson(const std::string& skill_id) const {
    return dispatcher_.describeSkillJson(skill_id);
}

std::string SkillHost::healthJson() const {
    return dispatcher_.healthJson();
}

std::string SkillHost::execute(const std::string& envelope_json) {
    return dispatcher_.execute(envelope_json);
}

std::string SkillHost::dropContextJson(const std::string& context_id) {
    Poco::JSON::Object::Ptr root(new Poco::JSON::Object);
    const auto dropped = scheduler::ContextStore::instance().dropContext(context_id);
    root->set("protocol", "1");
    root->set("ok", dropped.ok());
    if (dropped.ok()) {
        root->set("dropped", dropped.value());
        root->set("context_id", context_id);
    }
    std::ostringstream oss;
    Poco::JSON::Stringifier::condense(root, oss);
    return oss.str();
}

HostExitCode SkillHost::exitCodeFromExecuteResponse(const std::string& response_json) {
    try {
        Poco::JSON::Parser parser;
        Poco::JSON::Object::Ptr root =
            parser.parse(response_json).extract<Poco::JSON::Object::Ptr>();
        if (!root || !root->has("ok")) {
            return HostExitCode::HostError;
        }
        return root->getValue<bool>("ok") ? HostExitCode::Success : HostExitCode::ExecuteFailed;
    } catch (...) {
        return HostExitCode::HostError;
    }
}

} // namespace host
} // namespace AIstudy
