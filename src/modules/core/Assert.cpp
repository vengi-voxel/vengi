/**
 * @file
 */

#include "Assert.h"

#ifndef __WINDOWS__
#define HAVE_BACKWARD
#endif

#ifdef HAVE_BACKWARD
#include <backward.h>
#endif

void core_stacktrace() {
#ifdef HAVE_BACKWARD
	backward::StackTrace st;
	st.load_here(32);
	backward::TraceResolver tr;
	tr.load_stacktrace(st);
	for (size_t __stacktrace_i = 0; __stacktrace_i < st.size(); ++__stacktrace_i) {
		const backward::ResolvedTrace& trace = tr.resolve(st[__stacktrace_i]);
		printf("#%i %s %s [%p]\n", int(__stacktrace_i), trace.object_filename.c_str(), trace.object_function.c_str(), trace.addr);
	}
#endif
}
