/**
 * @file error_macros.h
 * @brief Macros for error-code-to-exception and checks
 *
 * SIMUTILS_THROW_IF_ERROR(ec, msg)  - if (ec) ec.toException(msg) with __FILE__/__LINE__/__func__
 * SIMUTILS_THROW_ON_ERROR(ec, msg)  - same (alias)
 * SIMUTILS_THROW_EXCEPTION(ec, msg) - always throw (with context)
 * SIMUTILS_CHECK_ERROR(ec)          - if (ec) ec.toException(ec.message()) with context
 */

#ifndef AISTUDY_ERROR_MACROS_H
#define AISTUDY_ERROR_MACROS_H

#include "error_code_wrapper.h"

#define SIMUTILS_THROW_IF_ERROR(ec, msg) \
    do { \
        const auto& _ec = (ec); \
        if (_ec) _ec.toException((msg), __FILE__, __LINE__, __func__); \
    } while (0)

#define SIMUTILS_THROW_ON_ERROR(ec, msg) SIMUTILS_THROW_IF_ERROR(ec, msg)

#define SIMUTILS_THROW_EXCEPTION(ec, msg) \
    (ec).toException((msg), __FILE__, __LINE__, __func__)

#define SIMUTILS_CHECK_ERROR(ec) \
    do { \
        const auto& _ec = (ec); \
        if (_ec) _ec.toException(_ec.message(), __FILE__, __LINE__, __func__); \
    } while (0)

#endif // AISTUDY_ERROR_MACROS_H
