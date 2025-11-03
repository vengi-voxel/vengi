/**
 * @file
 */
#include "NetworkAdapters.h"
#include "core/StandardLib.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Windows Vista or later
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

namespace network {

core::DynamicArray<core::String> getNetworkAdapters() {
	core::DynamicArray<core::String> ips;
	ips.push_back("0.0.0.0"); // INADDR_ANY
#ifdef _WIN32
	ULONG bufLen = 15000;
	PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES *)core_malloc(bufLen);
	if (pAddresses == nullptr) {
		return ips;
	}
	DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, pAddresses, &bufLen);
	if (ret == ERROR_BUFFER_OVERFLOW) {
		core_free(pAddresses);
		pAddresses = (IP_ADAPTER_ADDRESSES *)core_malloc(bufLen);
		ret = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, pAddresses, &bufLen);
	}
	if (ret != NO_ERROR) {
		core_free(pAddresses);
		return ips;
	}
	for (PIP_ADAPTER_ADDRESSES curr = pAddresses; curr != nullptr; curr = curr->Next) {
		if (curr->OperStatus != IfOperStatusUp) {
			continue;
		}
		for (IP_ADAPTER_UNICAST_ADDRESS *ua = curr->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
			SOCKADDR *sa = ua->Address.lpSockaddr;
			if (sa->sa_family == AF_INET) {
				char buf[INET_ADDRSTRLEN];
				auto sin = (struct sockaddr_in *)sa;
				if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
					ips.push_back(core::String(buf));
				}
			}
		}
	}
	core_free(pAddresses);
#else
	ifaddrs *ifaddr;
	if (getifaddrs(&ifaddr) == -1) {
		return ips;
	}
	for (ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr && (ifa->ifa_flags & IFF_UP) && ifa->ifa_addr->sa_family == AF_INET) {
			char buf[INET_ADDRSTRLEN];
			struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
			if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
				ips.push_back(core::String(buf));
			}
		}
	}
	freeifaddrs(ifaddr);
#endif
	return ips;
}

} // namespace network
