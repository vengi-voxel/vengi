/**
 * @file
 */

#pragma once

#include <future>

namespace core {

template<typename T>
using Future = std::future<T>;

}
