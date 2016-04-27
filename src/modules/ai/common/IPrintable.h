#pragma once

#include <iostream>

namespace ai {

class IPrintable {
public:
	virtual ~IPrintable() {
	}

	virtual std::ostream& print(std::ostream& output, int level) const = 0;
};

inline std::ostream& operator<<(std::ostream& output, const IPrintable& p) {
	return p.print(output, 0);
}

}
