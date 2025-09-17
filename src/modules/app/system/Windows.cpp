/**
 * @file
 */

#include "core/StandardLib.h"
#include <process.h>
#include <stdio.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

static HANDLE alarmThread = nullptr;
static HANDLE alarmCancel = nullptr;

static unsigned __stdcall alarm_thread(void *param) {
	int seconds = *(int *)param;
	core_free(param);

	// Wait for either cancel or timeout
	DWORD res = WaitForSingleObject(alarmCancel, seconds * 1000);
	if (res == WAIT_TIMEOUT) {
		fprintf(stderr, "alarm_win: timeout (%d seconds) expired — terminating\n", seconds);
		TerminateProcess(GetCurrentProcess(), 1);
	}
	return 0;
}

void alarm(int seconds) {
	// Cancel any previous alarm
	if (alarmThread) {
		SetEvent(alarmCancel); // signal cancel
		WaitForSingleObject(alarmThread, INFINITE);
		CloseHandle(alarmThread);
		alarmThread = nullptr;
	}

	if (seconds <= 0)
		return; // POSIX alarm(0): cancel

	if (!alarmCancel) {
		alarmCancel = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	int *param = (int *)core_malloc(sizeof(int));
	if (!param)
		return;
	*param = seconds;

	unsigned tid;
	alarmThread = (HANDLE)_beginthreadex(nullptr, 0, alarm_thread, param, 0, &tid);
}
