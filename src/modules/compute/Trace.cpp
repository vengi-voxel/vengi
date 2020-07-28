/**
 * @file
 */

#include "compute/Trace.h"

namespace compute {

TraceCLScoped::TraceCLScoped(const char* name, const char *msg) {
	traceCLBegin(name);
	core::traceMessage(msg);
}

TraceCLScoped::~TraceCLScoped() {
	traceCLEnd();
}

void traceCLBegin(const char* name) {
	core::traceBegin(name);
}

void traceCLEnd() {
	core::traceEnd();
}

}
