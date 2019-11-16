set(LIB libuv)

if(MSVC)
  list(APPEND uv_cflags /W4)
elseif(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
  list(APPEND uv_cflags -fvisibility=hidden --std=gnu89)
  list(APPEND uv_cflags -Wall -Wextra -Wstrict-prototypes)
  list(APPEND uv_cflags -Wno-unused-parameter -Wno-shadow -Wno-cast-function-type)
endif()

set(uv_sources
    src/fs-poll.c
    src/idna.c
    src/inet.c
    src/random.c
    src/strscpy.c
    src/threadpool.c
    src/timer.c
    src/uv-common.c
    src/uv-data-getter-setters.c
    src/version.c)

if(WIN32)
  list(APPEND uv_defines WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0600)
  list(APPEND uv_libraries
       advapi32
       iphlpapi
       psapi
       shell32
       user32
       userenv
       ws2_32)
  list(APPEND uv_sources
       src/win/async.c
       src/win/core.c
       src/win/detect-wakeup.c
       src/win/dl.c
       src/win/error.c
       src/win/fs.c
       src/win/fs-event.c
       src/win/getaddrinfo.c
       src/win/getnameinfo.c
       src/win/handle.c
       src/win/loop-watcher.c
       src/win/pipe.c
       src/win/thread.c
       src/win/poll.c
       src/win/process.c
       src/win/process-stdio.c
       src/win/signal.c
       src/win/snprintf.c
       src/win/stream.c
       src/win/tcp.c
       src/win/tty.c
       src/win/udp.c
       src/win/util.c
       src/win/winapi.c
       src/win/winsock.c)
else()
  list(APPEND uv_defines _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    # Android has pthread as part of its c library, not as a separate
    # libpthread.so.
    list(APPEND uv_libraries pthread)
  endif()
  list(APPEND uv_sources
       src/unix/async.c
       src/unix/core.c
       src/unix/dl.c
       src/unix/fs.c
       src/unix/getaddrinfo.c
       src/unix/getnameinfo.c
       src/unix/loop-watcher.c
       src/unix/loop.c
       src/unix/pipe.c
       src/unix/poll.c
       src/unix/process.c
       src/unix/random-devurandom.c
       src/unix/signal.c
       src/unix/stream.c
       src/unix/tcp.c
       src/unix/thread.c
       src/unix/tty.c
       src/unix/udp.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "AIX")
  list(APPEND uv_defines
       _ALL_SOURCE
       _LINUX_SOURCE_COMPAT
       _THREAD_SAFE
       _XOPEN_SOURCE=500)
  list(APPEND uv_libraries perfstat)
  list(APPEND uv_sources src/unix/aix.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  list(APPEND uv_libs dl)
  list(APPEND uv_sources
       src/unix/android-ifaddrs.c
       src/unix/linux-core.c
       src/unix/linux-inotify.c
       src/unix/linux-syscalls.c
       src/unix/procfs-exepath.c
       src/unix/pthread-fixes.c
       src/unix/sysinfo-loadavg.c)
endif()

if(APPLE OR CMAKE_SYSTEM_NAME MATCHES "Android|Linux|OS/390")
  list(APPEND uv_sources src/unix/proctitle.c)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD")
  list(APPEND uv_sources src/unix/freebsd.c)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD|NetBSD|OpenBSD")
  list(APPEND uv_sources src/unix/posix-hrtime.c src/unix/bsd-proctitle.c)
  list(APPEND uv_libraries kvm)
endif()

if(APPLE OR CMAKE_SYSTEM_NAME MATCHES "DragonFly|FreeBSD|NetBSD|OpenBSD")
  list(APPEND uv_sources src/unix/bsd-ifaddrs.c src/unix/kqueue.c)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  list(APPEND uv_sources src/unix/random-getrandom.c)
endif()

if(APPLE OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  list(APPEND uv_sources src/unix/random-getentropy.c)
endif()

if(APPLE)
  list(APPEND uv_defines _DARWIN_UNLIMITED_SELECT=1 _DARWIN_USE_64_BIT_INODE=1)
  list(APPEND uv_sources
       src/unix/darwin-proctitle.c
       src/unix/darwin.c
       src/unix/fsevents.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND uv_defines _GNU_SOURCE _POSIX_C_SOURCE=200112)
  list(APPEND uv_libraries dl rt)
  list(APPEND uv_sources
       src/unix/linux-core.c
       src/unix/linux-inotify.c
       src/unix/linux-syscalls.c
       src/unix/procfs-exepath.c
       src/unix/random-getrandom.c
       src/unix/random-sysctl-linux.c
       src/unix/sysinfo-loadavg.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
  list(APPEND uv_sources src/unix/netbsd.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  list(APPEND uv_sources src/unix/openbsd.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "OS/390")
  list(APPEND uv_defines PATH_MAX=255)
  list(APPEND uv_defines _AE_BIMODAL)
  list(APPEND uv_defines _ALL_SOURCE)
  list(APPEND uv_defines _LARGE_TIME_API)
  list(APPEND uv_defines _OPEN_MSGQ_EXT)
  list(APPEND uv_defines _OPEN_SYS_FILE_EXT)
  list(APPEND uv_defines _OPEN_SYS_IF_EXT)
  list(APPEND uv_defines _OPEN_SYS_SOCK_EXT3)
  list(APPEND uv_defines _OPEN_SYS_SOCK_IPV6)
  list(APPEND uv_defines _UNIX03_SOURCE)
  list(APPEND uv_defines _UNIX03_THREADS)
  list(APPEND uv_defines _UNIX03_WITHDRAWN)
  list(APPEND uv_defines _XOPEN_SOURCE_EXTENDED)
  list(APPEND uv_sources
       src/unix/pthread-fixes.c
       src/unix/pthread-barrier.c
       src/unix/os390.c
       src/unix/os390-syscalls.c)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
  list(APPEND uv_defines __EXTENSIONS__ _XOPEN_SOURCE=500)
  list(APPEND uv_libraries kstat nsl sendfile socket)
  list(APPEND uv_sources src/unix/no-proctitle.c src/unix/sunos.c)
endif()

engine_add_library(LIB ${LIB} SRCS ${uv_sources})
if (NOT LIBUV_FOUND)
  target_compile_definitions(${LIB} PRIVATE ${uv_defines} BUILDING_UV_SHARED=1)
  target_compile_options(${LIB} PRIVATE ${uv_cflags})
  target_include_directories(${LIB} PUBLIC include)
  target_include_directories(${LIB} PRIVATE src)
  target_link_libraries(${LIB} PUBLIC ${uv_libraries})
else()
  target_link_libraries(${LIB} INTERFACE ${uv_libraries})
endif()
