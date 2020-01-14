/**
 * @file
 */

#pragma once

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#else
#define SOCKET int
#include <sys/select.h>
#endif
