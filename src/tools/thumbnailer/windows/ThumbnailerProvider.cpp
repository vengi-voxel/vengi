/**
 * @file
 */

#include "ThumbnailerProvider.h"
#include "../ImageGenerator.h"
#include "io/FileStream.h"
#include "io/MemoryReadStream.h"

#include <Shlwapi.h>
#include <stdint.h>
#include <stdio.h>

static HBITMAP rgbaToBitmap(const uint32_t *src, uint32_t const imgW, uint32_t const imgH, bool const flip) {
	BITMAPINFO bmi = {sizeof(bmi.bmiHeader)};
	bmi.bmiHeader.biWidth = imgW;
	bmi.bmiHeader.biHeight = (flip) ? imgH : -static_cast<int32_t>(imgH);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void *pixels = NULL;
	HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pixels, NULL, 0);
	if (hbmp && pixels) {
		uint32_t *dst = static_cast<uint32_t *>(pixels);
		for (unsigned xy = imgW * imgH; xy > 0; xy--) {
			uint32_t rgba = *src++;
			*dst++ = ((rgba & 0x000000FF) << 16) | ((rgba & 0xFF00FF00)) | ((rgba & 0x00FF0000) >> 16);
		}
		GdiFlush();
	}
	return hbmp;
}

ThumbnailerProvider::ThumbnailerProvider() : count(1) {
}

ThumbnailerProvider::~ThumbnailerProvider() {
}

IFACEMETHODIMP ThumbnailerProvider::QueryInterface(REFIID riid, void **ppv) {
	static const QITAB qit[] = {
		QITABENT(ThumbnailerProvider, IThumbnailProvider),
		QITABENT(ThumbnailerProvider, IInitializeWithStream),
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

IFACEMETHODIMP ThumbnailerProvider::Initialize(LPCWSTR pfilePath, DWORD grfMode) {
	HRESULT hr = E_INVALIDARG;

	char filename[10240];
	size_t size;
	wcstombs_s(&size, filename, 10240, pfilePath, 10240);
	m_pPathFile = core::String(filename);
	return hr;
}

IFACEMETHODIMP ThumbnailerProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
	io::File file(m_pPathFile);
	io::FileStream stream(&file);
	const image::ImagePtr &image = thumbnailer::volumeThumbnail(file.name(), stream, cx);
	if (image) {
		*phbmp = rgbaToBitmap((uint32_t *)image->data(), image->width(), image->height(), false);
	}
	return (*phbmp) ? S_OK : S_FALSE;
}

ThumbnailerProviderFactory::ThumbnailerProviderFactory() : count(1) {
}

ThumbnailerProviderFactory::~ThumbnailerProviderFactory() {
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
