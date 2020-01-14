/**
 * @file
 */

#pragma once

#ifdef WIN32
#define network_cleanup() WSACleanup()
#define network_return int
#else
#define network_return ssize_t
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <signal.h>
#define closesocket close
#define INVALID_SOCKET  -1
#define network_cleanup()
#endif

extern bool networkInit();

extern void networkNonBlocking(SOCKET socket);
