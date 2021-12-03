/**
 * @file
 */

#pragma once

#include <Windows.h>
#include <thumbcache.h>
#include "core/String.h"

class ThumbnailerProvider : public IInitializeWithFile, public IThumbnailProvider {
private:
	// Reference count of component.
	long count;

	// Provided during initialization.
	core::String m_pPathFile;

protected:
	virtual ~ThumbnailerProvider();

public:
	ThumbnailerProvider();
	// IUnknown::QueryInterface()
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	// IUnknown::AddRef()
	IFACEMETHODIMP_(ULONG) AddRef() override;
	// IUnknown::Release()
	IFACEMETHODIMP_(ULONG) Release() override;

	// IInitializeWithFile::Initialize()
	IFACEMETHODIMP Initialize(LPCWSTR pfilePath, DWORD grfMode) override;

	// IThumbnailProvider::GetThumbnail()
	IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) override;
};

class ThumbnailerProviderFactory : public IClassFactory {
private:
	LONG count;

protected:
	virtual ~ThumbnailerProviderFactory();

public:
	ThumbnailerProviderFactory();
	// IUnknown::QueryInterface()
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	// IUnknown::AddRef()
	IFACEMETHODIMP_(ULONG) AddRef() override;
	// IUnknown::Release()
	IFACEMETHODIMP_(ULONG) Release() override;

	// IClassFactory::CreateInstance()
	IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv) override;
	// IClassFactory::LockServer()
	IFACEMETHODIMP LockServer(BOOL fLock) override;
};
