/**
 * @file
 */
#pragma once

namespace ai {

class NonCopyable {
public:
	NonCopyable() {
	}
private:
	NonCopyable (const NonCopyable&);
	NonCopyable& operator= (const NonCopyable&);
};

}
