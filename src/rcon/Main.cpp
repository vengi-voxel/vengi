/**
 * @file
 */
#include "AIApplication.h"
#include "AIDebugger.h"
#include "AIDebuggerWidget.h"

#ifdef AI_PROFILER
#include <google/profiler.h>
#endif

int main(int argc, char **argv) {
#ifdef AI_PROFILER
	ProfilerStart("simpleai-debugger.prof");
#endif

	ai::debug::AIApplication app(argc, argv);
	return app.exec();
}
