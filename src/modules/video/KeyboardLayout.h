/**
 * @file
 */

#include <stdint.h>

namespace video {

enum class KeyboardLayout : uint8_t { QWERTY, AZERTY, QWERTZ, COLEMAK, DVORAK, Max };

KeyboardLayout detectKeyboardLayout();

} // namespace video