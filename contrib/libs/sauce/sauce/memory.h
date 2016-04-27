#ifndef SAUCE_MEMORY_H_
#define SAUCE_MEMORY_H_

#if SAUCE_STD_SMART_PTR
  #include <sauce/internal/memory/std.h>
#elif SAUCE_STD_TR1_SMART_PTR
  #include <sauce/internal/memory/tr1.h>
#elif SAUCE_BOOST_SMART_PTR
  #include <sauce/internal/memory/boost.h>
#else
  #include <sauce/internal/memory/std.h>
#endif

#endif // SAUCE_MEMORY_H_
