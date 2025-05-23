/**
 * @file
 */

#include <ShlObj.h>
#include <Windows.h>
#include <tchar.h>
#include <new>

#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "voxelformat/VolumeFormat.h"
#include "ThumbnailerProvider.h"

// https://learn.microsoft.com/en-us/windows/win32/shell/thumbnail-providers
#define SHELLEX_THUMBNAIL_CLSID _T("ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}")
#define THUMBNAIL_HANDLER_TITLE _T("Voxel thumbnailer handler")
#define THUMBNAIL_HANDLER_CLSID _T("{CD1F0EA0-283C-4D90-A41D-DEBD9207D91F}")

static const CLSID CLSID_ThumbnailHandler = {
	0xCD1F0EA0, 0x283C, 0x4D90, {0xA4, 0x1D, 0xDE, 0xBD, 0x92, 0x07, 0xD9, 0x1F}};

static TCHAR dllPath[MAX_PATH] = {0};

static LONG dllRefs = 0;

#ifndef BAIL_ON_FAIL
#define BAIL_ON_FAIL(code)                                                                                             \
	if (FAILED((hr = (code))))                                                                                         \
	return hr
#endif

static HRESULT setRegKey(HKEY root, LPTSTR key, LPTSTR val, LPTSTR data) {
	HKEY hKey;
	HRESULT hr = HRESULT_FROM_WIN32(
		RegCreateKeyEx(root, key, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL));
	if (SUCCEEDED(hr)) {
		hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, val, 0, REG_SZ, reinterpret_cast<LPBYTE>(data),
											  static_cast<DWORD>(_tcslen(data) * sizeof(TCHAR))));
		RegCloseKey(hKey);
	}
	return hr;
}

BOOL APIENTRY DllMain(HMODULE hInstDLL, DWORD reason, LPVOID /*reserved*/) {
	if (reason == DLL_PROCESS_ATTACH) {
		OutputDebugString(_T("DllMain"));
		if (GetModuleFileName(hInstDLL, dllPath, sizeof(dllPath)) == 0) {
#ifdef _DEBUG
			OutputDebugString(_T("Failed to obtain DLL path"));
#endif
		}
		DisableThreadLibraryCalls(hInstDLL);
	}
	return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) {
	HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
	if (IsEqualCLSID(CLSID_ThumbnailHandler, rclsid)) {
		hr = E_OUTOFMEMORY;
		if (IClassFactory *factory = new (std::nothrow) ThumbnailerProviderFactory()) {
			hr = factory->QueryInterface(riid, ppv);
			factory->Release();
		}
	}
	return hr;
}

void DllAddRef() {
#ifdef _DEBUG
	OutputDebugString(_T("DllAddRef"));
#endif
	InterlockedIncrement(&dllRefs);
}

void DllRelease() {
#ifdef _DEBUG
	OutputDebugString(_T("DllRelease"));
#endif
	InterlockedDecrement(&dllRefs);
}

STDAPI DllCanUnloadNow() {
#ifdef _DEBUG
	OutputDebugString(_T("DllCanUnloadNow"));
#endif
	return (dllRefs == 0) ? S_OK : S_FALSE;
}

// regsvr32 vengi-voxelthumb.dll
STDAPI DllRegisterServer() {
	HRESULT hr = E_FAIL;
	if (_tcslen(dllPath)) {
		BAIL_ON_FAIL(setRegKey(HKEY_LOCAL_MACHINE, _T("Software\\Classes\\CLSID\\") THUMBNAIL_HANDLER_CLSID, NULL,
							   THUMBNAIL_HANDLER_TITLE));
		BAIL_ON_FAIL(setRegKey(HKEY_LOCAL_MACHINE,
							   _T("Software\\Classes\\CLSID\\") THUMBNAIL_HANDLER_CLSID _T("\\InProcServer32"), NULL,
							   dllPath));
		BAIL_ON_FAIL(setRegKey(HKEY_LOCAL_MACHINE,
							   _T("Software\\Classes\\CLSID\\") THUMBNAIL_HANDLER_CLSID _T("\\InProcServer32"),
							   _T("ThreadingModel"), _T("Apartment")));
		for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
			for (const core::String &ext : desc->exts) {
				core::String id = core::String::format("Software\\Classes\\.%s\\" SHELLEX_THUMBNAIL_CLSID, ext.c_str());
				BAIL_ON_FAIL(setRegKey(HKEY_LOCAL_MACHINE, id.c_str(), NULL, THUMBNAIL_HANDLER_CLSID));
			}
		}
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	}
#ifdef _DEBUG
	if (SUCCEEDED(hr)) {
		OutputDebugString(_T("Thumbnailer successfully registered"));
	}
#endif
	return hr;
}

// regsvr32 /u vengi-voxelthumb.dll
STDAPI DllUnregisterServer() {
	HRESULT hr =
		HRESULT_FROM_WIN32(RegDeleteTree(HKEY_LOCAL_MACHINE, _T("Software\\Classes\\CLSID\\") THUMBNAIL_HANDLER_CLSID));
	if (SUCCEEDED(hr)) {
		for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
			for (const core::String &ext : desc->exts) {
				core::String id = core::String::format("Software\\Classes\\.%s\\" SHELLEX_THUMBNAIL_CLSID, ext.c_str());
				hr = RegDeleteTree(HKEY_LOCAL_MACHINE, id.c_str());
			}
		}
	}
#ifdef _DEBUG
	if (SUCCEEDED(hr)) {
		OutputDebugString(_T("Thumbnailer successfully unregistered"));
	}
#endif
	return hr;
}
