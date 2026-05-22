#include <iostream>
#include <sstream>
#include <string>

#include "scheduler/Dispatcher.h"
#include "scheduler/skill_registry.h"

namespace {

void printHelp() {
    std::cout << "Usage: AIstudy [options]\n"
              << "  --list                List registered skills (JSON)\n"
              << "  --describe <skill_id> Show skill manifest / schema\n"
              << "  (no arguments)        Read one JSON envelope from stdin and execute\n";
}

std::string readStdinAll() {
    std::ostringstream oss;
    oss << std::cin.rdbuf();
    return oss.str();
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace AIstudy;

    scheduler::Dispatcher dispatcher;
    scheduler::registerBuiltinSkills(dispatcher);

    for (const auto& failure : dispatcher.loadFailures()) {
        std::cerr << "[AIstudy] WARNING: failed to load skill '" << failure.binding_id
                  << "' from " << failure.manifest_path << ": " << failure.error
                  << std::endl;
    }

    if (argc >= 2) {
        const std::string arg1 = argv[1];
        if (arg1 == "--list" || arg1 == "-list") {
            std::cout << dispatcher.listSkillsJson() << std::endl;
            return 0;
        }
        if ((arg1 == "--describe" || arg1 == "-describe") && argc >= 3) {
            std::cout << dispatcher.describeSkillJson(argv[2]) << std::endl;
            return 0;
        }
        if (arg1 == "--help" || arg1 == "-h") {
            printHelp();
            return 0;
        }
        printHelp();
        return 1;
    }

    const std::string envelope = readStdinAll();
    if (envelope.empty()) {
        printHelp();
        return 1;
    }

    std::cout << dispatcher.execute(envelope) << std::endl;
    return 0;
}
