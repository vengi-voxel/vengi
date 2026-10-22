/**
 * @file
 */

#include "ThumbnailerProvider.h"

#include <shlwapi.h>
#include <windows.h>
#include <new>
#include <stdint.h>
#include <stdio.h>
#include <tchar.h>
#include <wincodec.h>

extern void DllAddRef();
extern void DllRelease();

static HRESULT traceResult(const WCHAR *operation, HRESULT result) {
#ifndef _DEBUG
	if (GetEnvironmentVariableW(L"VENGI_THUMBNAILER_DEBUG", NULL, 0) == 0) {
		return result;
	}
#endif
	WCHAR message[256];
	_snwprintf(message, sizeof(message) / sizeof(WCHAR), L"vengi-voxelthumb pid=%lu %ls: 0x%08lx\n",
			   GetCurrentProcessId(), operation, (unsigned long)result);
	message[sizeof(message) / sizeof(WCHAR) - 1] = L'\0';
	OutputDebugStringW(message);
	return result;
}

static HRESULT findThumbnailerExe(WCHAR *exePath) {
	HMODULE module;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						   (LPCWSTR)&findThumbnailerExe, &module)) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	WCHAR directory[MAX_PATH];
	const DWORD length = GetModuleFileNameW(module, directory, MAX_PATH);
	if (length == 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (length >= MAX_PATH || !PathRemoveFileSpecW(directory)) {
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	}
	const WCHAR *names[] = {L"vengi-thumbnailer.exe", L"thumbnailer\\vengi-thumbnailer.exe"};
	for (const WCHAR *name : names) {
		if (!PathCombineW(exePath, directory, name)) {
			continue;
		}
		const DWORD attributes = GetFileAttributesW(exePath);
		if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
			return S_OK;
		}
	}
	return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

static WCHAR *rendererEnvironment(const WCHAR *home) {
	WCHAR *inherited = GetEnvironmentStringsW();
	if (!inherited) {
		return NULL;
	}
	size_t size = 1;
	for (const WCHAR *entry = inherited; *entry; entry += lstrlenW(entry) + 1) {
		size += lstrlenW(entry) + 1;
	}
	const WCHAR variable[] = L"APP_HOMEPATH";
	size += lstrlenW(variable) + 1 + lstrlenW(home) + 2;
	WCHAR *environment = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
	if (environment) {
		WCHAR *output = environment;
		bool inserted = false;
		for (const WCHAR *entry = inherited; *entry; entry += lstrlenW(entry) + 1) {
			const WCHAR *separator = wcschr(entry + 1, L'=');
			const int nameLength = separator ? (int)(separator - entry) : lstrlenW(entry);
			const int order = CompareStringOrdinal(entry, nameLength, variable, lstrlenW(variable), TRUE);
			if (order == CSTR_EQUAL) {
				continue;
			}
			if (!inserted && order == CSTR_GREATER_THAN) {
				lstrcpyW(output, variable);
				lstrcatW(output, L"=");
				lstrcatW(output, home);
				lstrcatW(output, L"/");
				output += lstrlenW(output) + 1;
				inserted = true;
			}
			lstrcpyW(output, entry);
			output += lstrlenW(entry) + 1;
		}
		if (!inserted) {
			lstrcpyW(output, variable);
			lstrcatW(output, L"=");
			lstrcatW(output, home);
			lstrcatW(output, L"/");
			output += lstrlenW(output) + 1;
		}
		*output = L'\0';
	}
	FreeEnvironmentStringsW(inherited);
	return environment;
}

static HRESULT runThumbnailer(const WCHAR *exePath, const WCHAR *inputFile, UINT cx, const WCHAR *outputFile,
							 const WCHAR *directory) {
	WCHAR command[4096];
	const int length = _snwprintf(command, sizeof(command) / sizeof(WCHAR),
								 L"\"%ls\" --input \"%ls\" --output \"%ls\" --size %u", exePath, inputFile, outputFile, cx);
	if (length < 0 || length >= (int)(sizeof(command) / sizeof(WCHAR))) {
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	}
	STARTUPINFOW startup = {};
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESHOWWINDOW;
	startup.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION process = {};
	// A Shell request must not reuse a user's PID marker, crash-recovery UI, or saved configuration.
	WCHAR *environment = rendererEnvironment(directory);
	if (!environment) {
		return E_OUTOFMEMORY;
	}
	const BOOL started = CreateProcessW(exePath, command, NULL, NULL, FALSE, CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
									 environment, directory, &startup, &process);
	const DWORD error = started ? ERROR_SUCCESS : GetLastError();
	HeapFree(GetProcessHeap(), 0, environment);
	if (!started) {
		return traceResult(L"CreateProcess", HRESULT_FROM_WIN32(error));
	}
	const DWORD waitResult = WaitForSingleObject(process.hProcess, 30000);
	HRESULT hr;
	if (waitResult == WAIT_OBJECT_0) {
		DWORD exitCode;
		hr = GetExitCodeProcess(process.hProcess, &exitCode) ? HRESULT_FROM_WIN32(exitCode)
															 : HRESULT_FROM_WIN32(GetLastError());
	} else {
		hr = waitResult == WAIT_TIMEOUT ? HRESULT_FROM_WIN32(ERROR_TIMEOUT) : HRESULT_FROM_WIN32(GetLastError());
		// A timed-out renderer must not keep running and writing to deleted temporary files.
		TerminateProcess(process.hProcess, ERROR_TIMEOUT);
		WaitForSingleObject(process.hProcess, 5000);
	}
	CloseHandle(process.hProcess);
	CloseHandle(process.hThread);
	return traceResult(L"Renderer exit", hr);
}

static HRESULT createTempDirectory(WCHAR *path) {
	WCHAR directory[MAX_PATH];
	const DWORD length = GetTempPathW(MAX_PATH, directory);
	if (length == 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (length >= MAX_PATH) {
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	}
	if (!GetTempFileNameW(directory, L"vxt", 0, path)) {
		return traceResult(L"Create temporary file", HRESULT_FROM_WIN32(GetLastError()));
	}
	if (!DeleteFileW(path) || !CreateDirectoryW(path, NULL)) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	return S_OK;
}

static void removeTempDirectory(const WCHAR *directory) {
	WCHAR pattern[MAX_PATH];
	if (!PathCombineW(pattern, directory, L"*")) {
		return;
	}
	WIN32_FIND_DATAW data;
	HANDLE search = FindFirstFileW(pattern, &data);
	if (search != INVALID_HANDLE_VALUE) {
		do {
			if (lstrcmpW(data.cFileName, L".") == 0 || lstrcmpW(data.cFileName, L"..") == 0) {
				continue;
			}
			WCHAR path[MAX_PATH];
			if (!PathCombineW(path, directory, data.cFileName)) {
				continue;
			}
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
					RemoveDirectoryW(path);
				} else {
					removeTempDirectory(path);
				}
			} else {
				DeleteFileW(path);
			}
		} while (FindNextFileW(search, &data));
		FindClose(search);
	}
	if (!RemoveDirectoryW(directory)) {
		traceResult(L"Remove request directory", HRESULT_FROM_WIN32(GetLastError()));
	}
}

static HRESULT copyStream(IStream *stream, const WCHAR *path) {
	HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	LARGE_INTEGER start = {};
	HRESULT hr = stream->Seek(start, STREAM_SEEK_SET, NULL);
	if (SUCCEEDED(hr)) {
		BYTE buffer[65536];
		for (;;) {
			ULONG bytesRead = 0;
			hr = stream->Read(buffer, sizeof(buffer), &bytesRead);
			if (FAILED(hr) || bytesRead == 0) {
				break;
			}
			DWORD written = 0;
			if (!WriteFile(file, buffer, bytesRead, &written, NULL)) {
				hr = HRESULT_FROM_WIN32(GetLastError());
				break;
			}
			if (written != bytesRead) {
				hr = STG_E_WRITEFAULT;
				break;
			}
		}
	}
	CloseHandle(file);
	return hr;
}

static const WCHAR *streamExtension(const WCHAR *name) {
	if (!name) {
		return L".tmp";
	}
	name = PathFindFileNameW(name);
	// Keep compound extensions such as .ben.json, without passing a Unicode source name to the renderer.
	for (const WCHAR *extension = name; *extension; ++extension) {
		if (*extension != L'.' || lstrlenW(extension) >= 32) {
			continue;
		}
		bool valid = true;
		for (const WCHAR *character = extension; *character; ++character) {
			if (!((*character >= L'a' && *character <= L'z') || (*character >= L'A' && *character <= L'Z') ||
				  (*character >= L'0' && *character <= L'9') || *character == L'.')) {
				valid = false;
				break;
			}
		}
		if (valid) {
			return extension;
		}
	}
	return L".tmp";
}

static HRESULT loadPngAsHBitmap(const WCHAR *pngPath, HBITMAP *phbmp) {
	HRESULT hr;
	IWICImagingFactory *factory = NULL;
	IWICBitmapDecoder *decoder = NULL;
	IWICBitmapFrameDecode *frame = NULL;
	IWICFormatConverter *converter = NULL;

	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
						  (void **)&factory);
	if (FAILED(hr)) {
		goto cleanup;
	}

	hr = factory->CreateDecoderFromFilename(pngPath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
	if (FAILED(hr)) {
		goto cleanup;
	}

	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) {
		goto cleanup;
	}

	hr = factory->CreateFormatConverter(&converter);
	if (FAILED(hr)) {
		goto cleanup;
	}

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0,
							   WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		goto cleanup;
	}

	{
		UINT width, height;
		hr = converter->GetSize(&width, &height);
		if (FAILED(hr)) {
			goto cleanup;
		}
		if (width == 0 || height == 0 || width > UINT32_MAX / 4 || height > UINT32_MAX / (width * 4)) {
			hr = E_INVALIDARG;
			goto cleanup;
		}

		BITMAPINFO bmi;
		ZeroMemory(&bmi, sizeof(bmi));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = (LONG)width;
		bmi.bmiHeader.biHeight = -((LONG)height);
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		void *pixels = NULL;
		*phbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pixels, NULL, 0);
		if (!*phbmp) {
			hr = E_OUTOFMEMORY;
			goto cleanup;
		}

		const UINT stride = width * 4;
		const UINT bufferSize = stride * height;
		hr = converter->CopyPixels(NULL, stride, bufferSize, (BYTE *)pixels);
		if (FAILED(hr)) {
			DeleteObject(*phbmp);
			*phbmp = NULL;
			goto cleanup;
		}
	}

cleanup:
	if (converter)
		converter->Release();
	if (frame)
		frame->Release();
	if (decoder)
		decoder->Release();
	if (factory)
		factory->Release();
	return hr;
}

ThumbnailerProvider::ThumbnailerProvider() : count(1), _stream(NULL) {
	DllAddRef();
}

ThumbnailerProvider::~ThumbnailerProvider() {
	if (_stream) {
		_stream->Release();
	}
	DllRelease();
}

IFACEMETHODIMP ThumbnailerProvider::QueryInterface(REFIID riid, void **ppv) {
	static const QITAB qit[] = {
		QITABENT(ThumbnailerProvider, IInitializeWithStream),
		QITABENT(ThumbnailerProvider, IInitializeWithFile),
		QITABENT(ThumbnailerProvider, IThumbnailProvider),
		{nullptr, 0},
	};
	WCHAR iid[40];
	StringFromGUID2(riid, iid, sizeof(iid) / sizeof(WCHAR));
	return traceResult(iid, QISearch(this, qit, riid, ppv));
}

IFACEMETHODIMP_(ULONG) ThumbnailerProvider::AddRef() {
	return InterlockedIncrement(&count);
}

IFACEMETHODIMP_(ULONG) ThumbnailerProvider::Release() {
	LONG refs = InterlockedDecrement(&count);
	if (refs == 0) {
		delete this;
	}
	return refs;
}

HRESULT ThumbnailerProvider::Initialize(LPCWSTR pfilePath, DWORD /*grfMode*/) {
	traceResult(L"Initialize(file)", S_OK);
	if (!pfilePath || !pfilePath[0]) {
		return E_INVALIDARG;
	}
	if (_stream) {
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
	}
	if (lstrlenW(pfilePath) >= MAX_PATH) {
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	}
	IStream *stream = NULL;
	HRESULT hr = SHCreateStreamOnFileEx(pfilePath, STGM_READ | STGM_SHARE_DENY_WRITE, 0, FALSE, NULL, &stream);
	if (SUCCEEDED(hr)) {
		hr = Initialize(stream, STGM_READ);
		stream->Release();
	}
	return hr;
}

HRESULT ThumbnailerProvider::Initialize(IStream *stream, DWORD /*grfMode*/) {
	traceResult(L"Initialize(stream)", S_OK);
	if (!stream) {
		return E_INVALIDARG;
	}
	if (_stream) {
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
	}
	_stream = stream;
	_stream->AddRef();
	return S_OK;
}

IFACEMETHODIMP ThumbnailerProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
	traceResult(L"GetThumbnail", S_OK);
	if (!phbmp || !pdwAlpha) {
		return E_POINTER;
	}
	*phbmp = NULL;
	*pdwAlpha = WTSAT_UNKNOWN;
	if (cx == 0) {
		return E_INVALIDARG;
	}
	if (!_stream) {
		return CO_E_NOTINITIALIZED;
	}

	WCHAR exePath[MAX_PATH];
	HRESULT hr = findThumbnailerExe(exePath);
	if (FAILED(hr)) {
		return traceResult(L"Find renderer", hr);
	}

	WCHAR temporary[MAX_PATH];
	hr = createTempDirectory(temporary);
	if (FAILED(hr)) {
		return hr;
	}
	WCHAR pngFile[MAX_PATH];
	WCHAR inputFile[MAX_PATH] = {};
	if (!PathCombineW(pngFile, temporary, L"thumbnail.png")) {
		removeTempDirectory(temporary);
		return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
	}
	{
		STATSTG info = {};
		_stream->Stat(&info, STATFLAG_DEFAULT);
		const WCHAR *extension = streamExtension(info.pwcsName);
		if (lstrlenW(temporary) + 7 + lstrlenW(extension) >= MAX_PATH) {
			hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
		} else {
			PathCombineW(inputFile, temporary, L"input");
			lstrcatW(inputFile, extension);
			hr = traceResult(L"Copy input stream", copyStream(_stream, inputFile));
		}
		CoTaskMemFree(info.pwcsName);
	}
	if (SUCCEEDED(hr)) {
		hr = runThumbnailer(exePath, inputFile, cx, pngFile, temporary);
	}
	if (SUCCEEDED(hr)) {
		hr = traceResult(L"Decode renderer PNG", loadPngAsHBitmap(pngFile, phbmp));
	}
	removeTempDirectory(temporary);
	if (SUCCEEDED(hr)) {
		*pdwAlpha = WTSAT_ARGB;
	}

	return traceResult(L"GetThumbnail complete", hr);
}

ThumbnailerProviderFactory::ThumbnailerProviderFactory() : count(1) {
	DllAddRef();
}

ThumbnailerProviderFactory::~ThumbnailerProviderFactory() {
	DllRelease();
}

IFACEMETHODIMP ThumbnailerProviderFactory::QueryInterface(REFIID riid, void **ppv) {
	static const QITAB qit[] = {
		QITABENT(ThumbnailerProviderFactory, IClassFactory),
		{nullptr, 0},
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ThumbnailerProviderFactory::AddRef() {
	return InterlockedIncrement(&count);
}

IFACEMETHODIMP_(ULONG) ThumbnailerProviderFactory::Release() {
	LONG refs = InterlockedDecrement(&count);
	if (refs == 0) {
		delete this;
	}
	return refs;
}

IFACEMETHODIMP ThumbnailerProviderFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) {
	HRESULT hr = CLASS_E_NOAGGREGATION;
	if (pUnkOuter == NULL) {
		hr = E_OUTOFMEMORY;
		if (ThumbnailerProvider *provider = new (std::nothrow) ThumbnailerProvider()) {
			hr = provider->QueryInterface(riid, ppv);
			provider->Release();
		}
	}
	return hr;
}

IFACEMETHODIMP ThumbnailerProviderFactory::LockServer(BOOL fLock) {
	if (fLock) {
		DllAddRef();
	} else {
		DllRelease();
	}
	return S_OK;
}
