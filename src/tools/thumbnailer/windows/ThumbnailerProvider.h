/**
 * @file
 */

#pragma once

#include <windows.h>
#include <thumbcache.h>

// COM objects are destroyed through Release(); the inherited interfaces have no virtual destructor.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

class ThumbnailerProvider : public IInitializeWithStream, public IInitializeWithFile, public IThumbnailProvider {
private:
	long count;
	IStream *_stream;

protected:
	virtual ~ThumbnailerProvider();

public:
	ThumbnailerProvider();
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	IFACEMETHODIMP_(ULONG) AddRef() override;
	IFACEMETHODIMP_(ULONG) Release() override;
	HRESULT STDMETHODCALLTYPE Initialize(IStream *stream, DWORD grfMode) override;
	HRESULT STDMETHODCALLTYPE Initialize(LPCWSTR pszFilePath, DWORD grfMode) override;
	IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) override;
};

class ThumbnailerProviderFactory : public IClassFactory {
private:
	LONG count;

protected:
	virtual ~ThumbnailerProviderFactory();

public:
	ThumbnailerProviderFactory();
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	IFACEMETHODIMP_(ULONG) AddRef() override;
	IFACEMETHODIMP_(ULONG) Release() override;
	IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) override;
	IFACEMETHODIMP LockServer(BOOL fLock) override;
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
