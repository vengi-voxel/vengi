/**
 * @file
 */

#include "ThumbnailerProvider.h"
#include "../Thumbnailer.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "io/Filesystem.h"

#include <Windows.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <stdint.h>
#include <stdio.h>

extern void DllAddRef();
extern void DllRelease();

class DLLThumbnailer : public Thumbnailer {
private:
	using Super = Thumbnailer;
	HBITMAP *_phbmp;
protected:
	HBITMAP rgbaToBitmap(const uint32_t *src, uint32_t const imgW, uint32_t const imgH, bool const flip) {
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

	bool saveImage(const image::ImagePtr &image) override {
		*_phbmp = rgbaToBitmap((const uint32_t *)image->data(), image->width(), image->height(), false);
		return true;
	}

public:
	DLLThumbnailer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider, HBITMAP *phbmp)
		: Super(filesystem, timeProvider), _phbmp(phbmp) {
	}
};

ThumbnailerProvider::ThumbnailerProvider() : count(1) {
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
	char filename[10240];
	size_t size;
	wcstombs_s(&size, filename, 10240, pfilePath, 10240);
	m_pPathFile = core::String(filename);
	return S_OK;
}

IFACEMETHODIMP ThumbnailerProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) {
	const io::FilesystemPtr &filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = core::make_shared<core::TimeProvider>();
	DLLThumbnailer app(filesystem, timeProvider, phbmp);
	char argv1[32];
	core::String::formatBuf(argv1, sizeof(argv1), "%s", app.fullAppname().c_str());
	char argv2[32];
	core::String::formatBuf(argv2, sizeof(argv2), "--size");
	char argv3[32];
	core::String::formatBuf(argv3, sizeof(argv3), "%u", (unsigned int)cx);
	char argv4[32];
	core::String::formatBuf(argv4, sizeof(argv4), "--input");
	char argv5[1024];
	core::String::formatBuf(argv5, sizeof(argv5), "%s", m_pPathFile.c_str());
	char *argv[] = {argv1, argv2, argv3, argv4, argv5};
	app.startMainLoop(5, argv);
	*pdwAlpha = WTSAT_ARGB;
	return (*phbmp) ? S_OK : S_FALSE;
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
