#ifdef AISTUDY_HDF5_ENABLED

#include "common/io/json/json_hdf5_bridge.h"
#include "common/io/json/json_advanced.h"
#include "common/io/hdf5/hdf5_reader.h"
#include "common/io/hdf5/hdf5_writer.h"
#include "common/status/exception/error_category.h"
#include "common/status/status_or.h"
#include "common/status/exception/error_codes.h"
#include "common/status/exception/error_code_wrapper.h"
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/Dynamic/Var.h>
#include <H5Apublic.h>
#include <H5Fpublic.h>
#include <H5Tpublic.h>
#include <H5Ppublic.h>
#include <vector>

namespace AIstudy {
namespace common {
namespace io {
namespace json {

using common::io::hdf5::Hdf5Reader;
using common::io::hdf5::Hdf5Writer;

namespace {

ErrorCodeWrapper makeJsonErr(JsonError e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::JSON);
}

ErrorCodeWrapper makeHdf5Err(Hdf5Error e) {
    return ErrorCodeWrapper(static_cast<int>(e), ErrorCategory::HDF5);
}

extern "C" herr_t collectAttrNames(hid_t, const char* name, const H5A_info_t*, void* op) {
    auto* out = static_cast<std::vector<std::string>*>(op);
    if (name) out->push_back(name);
    return 0;
}

} // namespace

StatusOr<bool> jsonToHdf5(const std::string& jsonPath, const std::string& hdf5Path) {
    std::vector<std::string> paths = { jsonPath };
    auto results = readJsonFiles(paths);
    if (results.empty() || !results[0].ok()) {
        return results.empty()
            ? StatusOr<bool>::Fail(ErrorCodeWrapper(static_cast<int>(JsonError::FILE_READ_ERROR), ErrorCategory::JSON))
            : StatusOr<bool>::Fail(results[0].status());
    }
    Poco::JSON::Object::Ptr o = results[0].value();

    Hdf5Writer w;
    auto cr = w.createFile(hdf5Path);
    if (!cr.ok()) return cr;

    try {
        auto names = o->getNames();
        for (const auto& k : names) {
            Poco::Dynamic::Var v = o->get(k);
            if (v.isInteger()) {
                auto wr = w.writeAttribute("/", k, v.convert<int>());
                if (!wr.ok()) return wr;
            } else if (v.isNumeric() && !v.isInteger()) {
                auto wr = w.writeAttribute("/", k, v.convert<double>());
                if (!wr.ok()) return wr;
            } else if (v.isString()) {
                auto wr = w.writeAttribute("/", k, v.convert<std::string>());
                if (!wr.ok()) return wr;
            } else if (v.type() == typeid(Poco::JSON::Array::Ptr)) {
                auto arr = v.extract<Poco::JSON::Array::Ptr>();
                if (arr->size() > 0) {
                    std::vector<double> vec;
                    for (std::size_t i = 0; i < arr->size(); ++i) {
                        vec.push_back(arr->get(i).convert<double>());
                    }
                    auto wr = w.writeDataset("/" + k, vec);
                    if (!wr.ok()) return wr;
                }
            }
        }
    } catch (const std::exception&) {
        return StatusOr<bool>::Fail(makeJsonErr(JsonError::TYPE_MISMATCH));
    }
    w.closeFile();
    return StatusOr<bool>::Ok(true);
}

StatusOr<Poco::JSON::Object::Ptr> hdf5ToJson(const std::string& hdf5Path) {
    Hdf5Reader r;
    auto open = r.openFile(hdf5Path);
    if (!open.ok()) return StatusOr<Poco::JSON::Object::Ptr>::Fail(open.status());
    auto names = r.getAttributeNames("/");
    if (!names.ok()) { r.closeFile(); return StatusOr<Poco::JSON::Object::Ptr>::Fail(names.status()); }
    auto out = Poco::JSON::Object::Ptr(new Poco::JSON::Object);
    for (const auto& k : names.value()) {
        auto rd = r.readAttribute<double>("/", k);
        if (rd.ok()) { out->set(k, rd.value()); continue; }
        auto ri = r.readAttribute<int>("/", k);
        if (ri.ok()) { out->set(k, ri.value()); continue; }
        auto rs = r.readAttribute<std::string>("/", k);
        if (rs.ok()) { out->set(k, rs.value()); continue; }
    }
    r.closeFile();
    return StatusOr<Poco::JSON::Object::Ptr>::Ok(out);
}

} // namespace json
} // namespace io
} // namespace common
} // namespace AIstudy


#endif // AISTUDY_HDF5_ENABLED
