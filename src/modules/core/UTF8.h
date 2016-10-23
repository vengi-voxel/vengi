/**
 * @file
 */

#pragma once

namespace core {
namespace utf8 {

/**
 * @brief Extract the next utf8 char from the given stream and advances the stream pointer by the char length
 * @return -1 on end of string
 */
extern int next(const char** str);

}
}
