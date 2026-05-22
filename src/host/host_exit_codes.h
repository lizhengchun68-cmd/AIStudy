#ifndef AISTUDY_HOST_HOST_EXIT_CODES_H
#define AISTUDY_HOST_HOST_EXIT_CODES_H

/**
 * @file host_exit_codes.h
 * @brief Process exit codes for AIstudy Skill Host (M6 stdio contract).
 * @see dosc/host-runtime.md
 */

namespace AIstudy {
namespace host {

/** @brief Host process exit codes (documented in host-runtime.md). */
enum class HostExitCode : int {
    Success = 0,
    /** Usage error, empty stdin, unknown CLI flag. */
    UsageError = 1,
    /** execute returned protocol response with ok:false. */
    ExecuteFailed = 2,
    /** Unparseable execute response or unexpected host failure. */
    HostError = 3,
};

} // namespace host
} // namespace AIstudy

#endif // AISTUDY_HOST_HOST_EXIT_CODES_H
