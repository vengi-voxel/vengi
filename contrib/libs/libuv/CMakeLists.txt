set(LIB libuv)

include_directories(include src)
set(SOURCES
	include/uv.h
	include/uv/tree.h
	include/uv/errno.h
	include/uv/threadpool.h
	include/uv/version.h
	src/idna.c
	src/fs-poll.c
	src/heap-inl.h
	src/inet.c
	src/queue.h
	src/timer.c
	src/threadpool.c
	src/uv-common.c
	src/uv-common.h
	src/version.c
	src/uv-data-getter-setters.c
	)

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0600 -D_GNU_SOURCE -D_CRT_SECURE_NO_WARNINGS)
	set (SOURCES ${SOURCES}
		include/uv/win.h
		src/win/async.c
		src/win/atomicops-inl.h
		src/win/core.c
		src/win/detect-wakeup.c
		src/win/dl.c
		src/win/error.c
		src/win/fs.c
		src/win/fs-event.c
		src/win/getaddrinfo.c
		src/win/getnameinfo.c
		src/win/handle.c
		src/win/handle-inl.h
		src/win/internal.h
		src/win/loop-watcher.c
		src/win/pipe.c
		src/win/thread.c
		src/win/poll.c
		src/win/process.c
		src/win/process-stdio.c
		src/win/req.c
		src/win/req-inl.h
		src/win/signal.c
		src/win/stream.c
		src/win/stream-inl.h
		src/win/tcp.c
		src/win/tty.c
		src/win/timer.c
		src/win/udp.c
		src/win/util.c
		src/win/winapi.c
		src/win/winapi.h
		src/win/winsock.c
		src/win/winsock.h
		)

	engine_add_library(LIB ${LIB} SRCS ${SOURCES} UNITY)
	if (NOT LIBUV_FOUND)
		target_link_libraries(${LIB} advapi32 iphlpapi psapi userenv shell32 ws2_32)
		target_include_directories(${LIB} PUBLIC include/)
	endif()
else()
	add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)
	include_directories(src/unix)
	set(SOURCES ${SOURCES}
		include/uv/unix.h
		include/uv/linux.h
		include/uv/sunos.h
		include/uv/darwin.h
		include/uv/bsd.h
		include/uv/aix.h
		src/unix/async.c
		src/unix/atomic-ops.h
		src/unix/core.c
		src/unix/dl.c
		src/unix/fs.c
		src/unix/getaddrinfo.c
		src/unix/getnameinfo.c
		src/unix/internal.h
		src/unix/loop.c
		src/unix/loop-watcher.c
		src/unix/pipe.c
		src/unix/poll.c
		src/unix/process.c
		src/unix/signal.c
		src/unix/spinlock.h
		src/unix/stream.c
		src/unix/tcp.c
		src/unix/thread.c
		src/unix/tty.c
		src/unix/udp.c
	)

	if (APPLE)
		add_definitions(-D_DARWIN_USE_64_BIT_INODE=1 -D_DARWIN_UNLIMITED_SELECT=1)
		set(SOURCES ${SOURCES}
			src/unix/proctitle.c
			src/unix/kqueue.c
			src/unix/darwin.c
			src/unix/fsevents.c
			src/unix/darwin-proctitle.c
		)

		#This is necessary to mute harmless warnings, as documented by https://github.com/libuv/libuv/issues/454
		set_source_files_properties(src/unix/stream.c PROPERTIES COMPILE_FLAGS -Wno-gnu-folding-constant)
	endif()

	if (NOT APPLE)
		add_definitions(-Wstrict-aliasing)
	endif()

	if ((${CMAKE_SYSTEM_NAME} MATCHES "Linux") OR (${CMAKE_SYSTEM_NAME} MATCHES "Android"))
		add_definitions(-D_GNU_SOURCE -D_POSIX_C_SOURCE=200112)
		set(SOURCES ${SOURCES}
			src/unix/proctitle.c
			src/unix/linux-core.c
			src/unix/linux-inotify.c
			src/unix/linux-syscalls.c
			src/unix/linux-syscalls.h
		)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
		add_definitions(-D__EXTENSIONS__ -D_XOPEN_SOURCE=500)
		set(SOURCES ${SOURCES}
			src/unix/sunos.c
			)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "AIX")
		add_definitions(-D_ALL_SOURCE -D_XOPEN_SOURCE=500 -D_LINUX_SOURCE_COMPAT)
		set (SOURCES ${SOURCES}
			src/unix/aic.c
		)
		find_library(PERFSTAT_LIB NAMES perfstat)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "DragonFlyBSD")
		set (SOURCES ${SOURCES}
			src/unix/freebsd.c
		)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
		set (SOURCES ${SOURCES}
			src/unix/openbsd.c
		)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
		set (SOURCES ${SOURCES}
			src/unix/netbsd.c
		)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "DragonFlyBSD" OR
			${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
		set (SOURCES ${SOURCES}
			src/unix/kqueue.c
		)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
		set (SOURCES ${SOURCES}
			include/uv/android-ifaddrs.h
			src/unix/android-ifaddrs.c
		)
	endif()

	engine_add_library(LIB ${LIB} libuv SRCS ${SOURCES})
	if (NOT LIBUV_FOUND)
		find_library(M_LIB NAMES m)
		find_package(Threads)
		target_link_libraries(${LIB} ${CMAKE_THREAD_LIBS_INIT} ${M_LIB})
		target_include_directories(${LIB} PUBLIC include/)

		if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
			find_library(DL_LIB NAMES dl)
			find_library(RT_LIB NAMES rt)
			target_link_libraries(${LIB} ${DL_LIB} ${RT_LIB})
		endif()

		if (${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
			find_library(KSTAT_LIB NAMES kstat)
			find_library(NSL_LIB NAMES nsl)
			find_library(SENDFILE_LIB NAMES sendfile)
			find_library(SOCKET_LIB NAMES socket)
			target_link_libraries(${LIB} ${KSTAT_LIB} ${NSL_LIB} ${SENDFILE_LIB} ${SOCKET_LIB})
		endif()

		if (${CMAKE_SYSTEM_NAME} MATCHES "AIX")
			find_library(PERFSTAT_LIB NAMES perfstat)
			target_link_libraries(${LIB} ${PERFSTAT_LIB})
		endif()

		if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "DragonFlyBSD" OR
				${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
			find_library(KVM_LIB NAMES kvm)
			target_link_libraries(${LIB} ${KVM_LIB})
		endif()
	endif()
endif()
