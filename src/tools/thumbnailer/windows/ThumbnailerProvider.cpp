/**
 * @file
 */

#include "ThumbnailerProvider.h"

#include <Shlwapi.h>
#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <tchar.h>
#include <wincodec.h>

extern void DllAddRef();
extern void DllRelease();
extern TCHAR dllPath[MAX_PATH];

static bool findThumbnailerExe(TCHAR *exePath, DWORD size) {
	// get directory containing the DLL
	TCHAR dir[MAX_PATH];
	lstrcpyn(dir, dllPath, MAX_PATH);
	PathRemoveFileSpec(dir);

	// try same directory as DLL
	PathCombine(exePath, dir, _T("vengi-thumbnailer.exe"));
	if (GetFileAttributes(exePath) != INVALID_FILE_ATTRIBUTES) {
		return true;
	}

	// try thumbnailer/ subdirectory
	PathCombine(exePath, dir, _T("thumbnailer\\vengi-thumbnailer.exe"));
	if (GetFileAttributes(exePath) != INVALID_FILE_ATTRIBUTES) {
		return true;
	}

	return false;
}

static HRESULT runThumbnailer(const TCHAR *exePath, const WCHAR *inputFile, UINT cx, const TCHAR *outputFile) {
	TCHAR cmdLine[4096];
	_sntprintf(cmdLine, sizeof(cmdLine) / sizeof(TCHAR), _T("\"%s\" --input \"%ls\" --output \"%s\" --size %u"),
			   exePath, inputFile, outputFile, cx);

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// wait up to 30 seconds
	DWORD waitResult = WaitForSingleObject(pi.hProcess, 30000);
	DWORD exitCode = 1;
	if (waitResult == WAIT_OBJECT_0) {
		GetExitCodeProcess(pi.hProcess, &exitCode);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	if (waitResult != WAIT_OBJECT_0 || exitCode != 0) {
		return E_FAIL;
	}
	return S_OK;
}

static HRESULT loadPngAsHBitmap(const TCHAR *pngPath, HBITMAP *phbmp) {
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

#ifdef UNICODE
	hr = factory->CreateDecoderFromFilename(pngPath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
#else
	WCHAR widePath[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, pngPath, -1, widePath, MAX_PATH);
	hr = factory->CreateDecoderFromFilename(widePath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
#endif
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

	hr = converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0,
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

ThumbnailerProvider::ThumbnailerProvider() : count(1) {
	_filePath[0] = L'\0';
	DllAddRef();
}

ThumbnailerProvider::~ThumbnailerProvider() {
	DllRelease();
}

IFACEMETHODIMP ThumbnailerProvider::QueryInterface(REFIID riid, void **ppv) {
	static const QITAB qit[] = {
		QITABENT(ThumbnailerProvider, IInitializeWithFile),
		QITABENT(ThumbnailerProvider, IThumbnailProvider),
		{0},
	};
	return QISearch(this, qit, riid, ppv);
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

HRESULT ThumbnailerProvider::Initialize(LPCWSTR pfilePath, DWORD grfMode) {
	lstrcpynW(_filePath, pfilePath, MAX_PATH);
	return S_OK;
}

IFACEMETHODIMP ThumbnailerProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
	*phbmp = NULL;
	*pdwAlpha = WTSAT_ARGB;

	TCHAR exePath[MAX_PATH];
	if (!findThumbnailerExe(exePath, MAX_PATH)) {
		OutputDebugString(_T("vengi-thumbnailer.exe not found"));
		return E_FAIL;
	}

	// create temp file for the output PNG
	TCHAR tempDir[MAX_PATH];
	GetTempPath(MAX_PATH, tempDir);
	TCHAR tempFile[MAX_PATH];
	GetTempFileName(tempDir, _T("vxt"), 0, tempFile);

	// rename to .png so the thumbnailer writes PNG format
	TCHAR pngFile[MAX_PATH];
	lstrcpyn(pngFile, tempFile, MAX_PATH);
	lstrcat(pngFile, _T(".png"));
	MoveFile(tempFile, pngFile);

	HRESULT hr = runThumbnailer(exePath, _filePath, cx, pngFile);
	if (SUCCEEDED(hr)) {
		hr = loadPngAsHBitmap(pngFile, phbmp);
	}

	DeleteFile(pngFile);
	// also clean up the original temp file name in case MoveFile failed
	DeleteFile(tempFile);

	return hr;
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
		{0},
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
		InterlockedIncrement(&count);
	} else {
		InterlockedDecrement(&count);
	}
	return S_OK;
}
