#ifndef SAUCE_NAMED_H_
#define SAUCE_NAMED_H_

#include <string>

namespace sauce {

/**
 * Wrap dependency requests with Named to choose one of several (statically) named alternatives.
 */
template<typename Iface, typename Name>
class Named {};

/**
 * The name of all statically unnamed dependencies.
 */
class Unnamed {};

/**
 * The name of all dynamically unnamed dependencies.
 */
inline std::string const & unnamed() {
  static std::string const unnamed = "unnamed";
  return unnamed;
}

}

#endif // SAUCE_NAMED_H_
