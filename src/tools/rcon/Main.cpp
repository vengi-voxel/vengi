/**
 * @file
 */
#include "rcon/RconAIApplication.h"
#include "rcon/RconAIDebugger.h"
#include "AIDebuggerWidget.h"

#ifdef AI_PROFILER
#include <google/profiler.h>
#endif

int main(int argc, char **argv) {
#ifdef AI_PROFILER
	ProfilerStart("simpleai-debugger.prof");
#endif

	rcon::RconAIApplication app(argc, argv);
	app.init();
	return rcon::RconAIApplication::exec();
}
