/**
 * @file
 */

#pragma once

#include <Windows.h>
#include <thumbcache.h>

class ThumbnailerProvider : public IInitializeWithFile, public IThumbnailProvider {
private:
	long count;
	WCHAR _filePath[MAX_PATH];

protected:
	virtual ~ThumbnailerProvider();

public:
	ThumbnailerProvider();
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	IFACEMETHODIMP_(ULONG) AddRef() override;
	IFACEMETHODIMP_(ULONG) Release() override;
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
