#include "common/logger/logger.h"
#include "host/host_exit_codes.h"
#include "host/http_server.h"
#include "host/skill_host.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

using AIstudy::host::HostExitCode;
using AIstudy::host::SkillHost;

void printHelp() {
    std::cout << "AIstudy Skill Host (protocol v1)\n"
              << "Usage: AIstudy [options]\n"
              << "  --list                List registered skills (JSON)\n"
              << "  --describe <skill_id> Show skill manifest / schema\n"
              << "  --health              Host health probe (JSON)\n"
              << "  --serve [port]        HTTP JSON API on 127.0.0.1 (default 8765)\n"
              << "  (no arguments)        Read one JSON envelope from stdin, write one line to stdout\n"
              << "Exit codes (execute mode): 0=ok, 1=usage, 2=ok:false, 3=host error\n"
              << "See dosc/host-runtime.md\n";
}

std::string readStdinAll() {
    std::ostringstream oss;
    oss << std::cin.rdbuf();
    return oss.str();
}

int toProcessExit(HostExitCode code) {
    return static_cast<int>(code);
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace AIstudy;

    common::logger::Logger::initialize();

    SkillHost host;
    host.logLoadFailuresToStderr();

    if (argc >= 2) {
        const std::string arg1 = argv[1];
        if (arg1 == "--list" || arg1 == "-list") {
            std::cout << host.listSkillsJson() << std::endl;
            return toProcessExit(HostExitCode::Success);
        }
        if ((arg1 == "--describe" || arg1 == "-describe") && argc >= 3) {
            std::cout << host.describeSkillJson(argv[2]) << std::endl;
            return toProcessExit(HostExitCode::Success);
        }
        if (arg1 == "--health" || arg1 == "-health") {
            std::cout << host.healthJson() << std::endl;
            return toProcessExit(HostExitCode::Success);
        }
        if (arg1 == "--serve" || arg1 == "-serve") {
            int port = 8765;
            if (argc >= 3) {
                port = std::atoi(argv[2]);
            }
            if (port <= 0 || port > 65535) {
                std::cerr << "[AIstudy] invalid port\n";
                return toProcessExit(HostExitCode::UsageError);
            }
            return AIstudy::host::runHttpServer(host, port);
        }
        if (arg1 == "--help" || arg1 == "-h") {
            printHelp();
            return toProcessExit(HostExitCode::Success);
        }
        printHelp();
        return toProcessExit(HostExitCode::UsageError);
    }

    const std::string envelope = readStdinAll();
    if (envelope.empty()) {
        printHelp();
        return toProcessExit(HostExitCode::UsageError);
    }

    const std::string response = host.execute(envelope);
    std::cout << response << std::endl;
    return toProcessExit(SkillHost::exitCodeFromExecuteResponse(response));
}
