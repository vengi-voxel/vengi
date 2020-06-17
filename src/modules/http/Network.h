/**
 * @file
 */

#pragma once

#include <SDL_platform.h>
#ifdef __WINDOWS__
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
