#ifndef AISTUDY_TESTS_SKILL_CONTRACT_UTIL_H
#define AISTUDY_TESTS_SKILL_CONTRACT_UTIL_H

#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <fstream>
#include <sstream>
#include <string>

namespace aistudy_test {

inline Poco::JSON::Object::Ptr parseJsonObject(const std::string& text) {
    Poco::JSON::Parser parser;
    auto var = parser.parse(text);
    return var.extract<Poco::JSON::Object::Ptr>();
}

/** @brief 契约对比前移除可变字段（request_id、duration_ms） */
inline void stripVolatileResponseFields(Poco::JSON::Object::Ptr obj) {
    if (!obj) {
        return;
    }
    obj->remove("request_id");
    if (obj->has("meta") && obj->isObject("meta")) {
        auto meta = obj->getObject("meta");
        meta->remove("duration_ms");
    }
}

inline std::string readTextFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

inline bool jsonEqual(Poco::JSON::Object::Ptr a, Poco::JSON::Object::Ptr b) {
    if (!a || !b) {
        return false;
    }
    if (a->size() != b->size()) {
        return false;
    }
    for (const auto& key : a->getNames()) {
        if (!b->has(key)) {
            return false;
        }
        const auto va = a->get(key);
        const auto vb = b->get(key);
        if (va.type() != vb.type()) {
            return false;
        }
        if (va.type() == typeid(Poco::JSON::Object::Ptr)) {
            if (!jsonEqual(va.extract<Poco::JSON::Object::Ptr>(),
                           vb.extract<Poco::JSON::Object::Ptr>())) {
                return false;
            }
        } else if (va.type() == typeid(Poco::JSON::Array::Ptr)) {
            auto aa = va.extract<Poco::JSON::Array::Ptr>();
            auto ab = vb.extract<Poco::JSON::Array::Ptr>();
            if (!aa || !ab || aa->size() != ab->size()) {
                return false;
            }
            for (size_t i = 0; i < aa->size(); ++i) {
                if (aa->get(static_cast<unsigned>(i)).type()
                    != ab->get(static_cast<unsigned>(i)).type()) {
                    return false;
                }
            }
        } else if (va.convert<std::string>() != vb.convert<std::string>()) {
            return false;
        }
    }
    return true;
}

} // namespace aistudy_test

#endif
