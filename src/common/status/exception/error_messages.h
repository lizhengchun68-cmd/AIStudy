/**
 * @file error_messages.h
 * @brief Initialize default error code messages
 */

#ifndef AISTUDY_ERROR_MESSAGES_H
#define AISTUDY_ERROR_MESSAGES_H

namespace AIstudy {

/**
 * @brief Initialize default error code descriptions
 * 
 * Registers default error messages for all predefined error codes.
 * This function should be called during library initialization,
 * typically at program startup or when the library is first used.
 * 
 * It's safe to call this function multiple times - it will only
 * register messages that haven't been registered yet.
 */
void initializeDefaultErrorMessages();

} // namespace AIstudy

#endif // AISTUDY_ERROR_MESSAGES_H
