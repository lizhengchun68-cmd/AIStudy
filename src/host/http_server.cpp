#include "host/http_server.h"
#include "host/host_exit_codes.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace AIstudy {
namespace host {
namespace {

std::string jsonContentType() {
    return "Content-Type: application/json; charset=utf-8\r\n";
}

std::string httpResponse(int status, const std::string& status_text, const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << ' ' << status_text << "\r\n"
        << jsonContentType() << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;
    return oss.str();
}

std::string urlDecodePath(const std::string& encoded) {
    std::string out;
    out.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            const int hi = encoded[i + 1];
            const int lo = encoded[i + 2];
            auto hex = [](int c) -> int {
                if (c >= '0' && c <= '9') {
                    return c - '0';
                }
                if (c >= 'A' && c <= 'F') {
                    return c - 'A' + 10;
                }
                if (c >= 'a' && c <= 'f') {
                    return c - 'a' + 10;
                }
                return -1;
            };
            const int h = hex(hi);
            const int l = hex(lo);
            if (h >= 0 && l >= 0) {
                out.push_back(static_cast<char>((h << 4) | l));
                i += 2;
                continue;
            }
        }
        out.push_back(encoded[i] == '+' ? ' ' : encoded[i]);
    }
    return out;
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
};

bool parseHttpRequest(const std::string& raw, HttpRequest& out) {
    const auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return false;
    }
    const std::string headers = raw.substr(0, header_end);
    out.body = raw.substr(header_end + 4);

    std::istringstream header_stream(headers);
    std::string request_line;
    if (!std::getline(header_stream, request_line)) {
        return false;
    }
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }
    std::istringstream rl(request_line);
    std::string version;
    if (!(rl >> out.method >> out.path >> version)) {
        return false;
    }

    std::string line;
    size_t content_length = 0;
    while (std::getline(header_stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') {
            value.erase(value.begin());
        }
        for (char& c : name) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (name == "content-length") {
            content_length = static_cast<size_t>(std::stoul(value));
        }
    }

    if (out.body.size() < content_length && raw.size() >= header_end + 4 + content_length) {
        out.body = raw.substr(header_end + 4, content_length);
    } else if (out.body.size() > content_length && content_length > 0) {
        out.body.resize(content_length);
    }
    return true;
}

std::string handleRequest(SkillHost& host, const HttpRequest& req) {
    if (req.method == "GET" && req.path == "/v1/health") {
        return httpResponse(200, "OK", host.healthJson());
    }
    if (req.method == "GET" && req.path == "/v1/skills") {
        return httpResponse(200, "OK", host.listSkillsJson());
    }
    if (req.method == "POST" && req.path == "/v1/execute") {
        const std::string body = host.execute(req.body);
        const auto code = SkillHost::exitCodeFromExecuteResponse(body);
        if (code == HostExitCode::HostError) {
            return httpResponse(500, "Internal Server Error", body);
        }
        if (code == HostExitCode::ExecuteFailed) {
            return httpResponse(422, "Unprocessable Entity", body);
        }
        return httpResponse(200, "OK", body);
    }
    if (req.method == "GET" && startsWith(req.path, "/v1/skills/")) {
        std::string skill_id = urlDecodePath(req.path.substr(std::string("/v1/skills/").size()));
        return httpResponse(200, "OK", host.describeSkillJson(skill_id));
    }
    const std::string err = R"({"ok":false,"error":"not found"})";
    return httpResponse(404, "Not Found", err);
}

} // namespace

int runHttpServer(SkillHost& host, int port) {
#ifdef _WIN32
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[AIstudy] HTTP: WSAStartup failed\n";
        return static_cast<int>(HostExitCode::HostError);
    }

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) {
        WSACleanup();
        return static_cast<int>(HostExitCode::HostError);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<u_short>(port));

    if (bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[AIstudy] HTTP: bind failed on port " << port << "\n";
        closesocket(listen_sock);
        WSACleanup();
        return static_cast<int>(HostExitCode::HostError);
    }

    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listen_sock);
        WSACleanup();
        return static_cast<int>(HostExitCode::HostError);
    }

    std::cerr << "[AIstudy] HTTP listening on http://127.0.0.1:" << port
              << " (POST /v1/execute, GET /v1/health, GET /v1/skills)\n";

    for (;;) {
        SOCKET client = accept(listen_sock, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            continue;
        }

        std::string raw;
        char buf[4096];
        for (;;) {
            const int n = recv(client, buf, sizeof(buf), 0);
            if (n <= 0) {
                break;
            }
            raw.append(buf, buf + n);
            if (raw.find("\r\n\r\n") != std::string::npos) {
                HttpRequest req;
                if (parseHttpRequest(raw, req)) {
                    const auto cl_pos = raw.find("Content-Length:");
                    size_t need = req.body.size();
                    if (cl_pos != std::string::npos) {
                        const auto line_end = raw.find("\r\n", cl_pos);
                        std::string cl_line = raw.substr(cl_pos, line_end - cl_pos);
                        const auto colon = cl_line.find(':');
                        need = static_cast<size_t>(std::stoul(cl_line.substr(colon + 1)));
                    }
                    const auto hdr_end = raw.find("\r\n\r\n") + 4;
                    while (raw.size() - hdr_end < need) {
                        const int more = recv(client, buf, sizeof(buf), 0);
                        if (more <= 0) {
                            break;
                        }
                        raw.append(buf, buf + more);
                    }
                    break;
                }
            }
            if (raw.size() > 1024 * 1024) {
                break;
            }
        }

        HttpRequest req;
        std::string response;
        if (parseHttpRequest(raw, req)) {
            response = handleRequest(host, req);
        } else {
            response = httpResponse(400, "Bad Request", R"({"ok":false,"error":"bad request"})");
        }

        send(client, response.data(), static_cast<int>(response.size()), 0);
        closesocket(client);
    }
#else
    (void)host;
    (void)port;
    std::cerr << "[AIstudy] HTTP server is only implemented on Windows in M6.\n";
    return static_cast<int>(HostExitCode::HostError);
#endif
}

} // namespace host
} // namespace AIstudy
