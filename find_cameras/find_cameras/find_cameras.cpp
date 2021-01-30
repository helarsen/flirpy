// find_cameras.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <dshow.h>
#include <string>
#include <iostream>

#pragma comment(lib, "strmiids")

inline std::wstring convert(const std::string& as)
{
	// deal with trivial case of empty string
	if (as.empty())    return std::wstring();

	// determine required length of new string
	size_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), 0, 0);

	// construct new string of required length
	std::wstring ret(reqLength, L'\0');

	// convert old string to new string
	::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), &ret[0], (int)ret.length());

	// return new string ( compiler should optimize this away )
	return ret;
}

HRESULT EnumerateDevices(REFGUID category, IEnumMoniker **ppEnum)
{
	// Create the System Device Enumerator.
	ICreateDevEnum *pDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

	if (SUCCEEDED(hr))
	{
		// Create an enumerator for the category.
		hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
		if (hr == S_FALSE)
		{
			hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
		}
		pDevEnum->Release();
	}
	return hr;
}
/*
* get_id() looks up devices with matching friendlyName and stores and returns its id in an array.
* returns the size of the array.
*/
int get_id(IEnumMoniker *pEnum, std::string friendly_name, int camId[], int maxNCams)
{
	IMoniker *pMoniker = NULL;
	int id = 0;
	int idLoopCnt = 0;

	while ( (pEnum->Next(1, &pMoniker, NULL) == S_OK) & (idLoopCnt <maxNCams))
	{
		IPropertyBag *pPropBag;
		HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (FAILED(hr))
		{
			pMoniker->Release();
			continue;
		}

		VARIANT var;
		VariantInit(&var);

		// Get description or friendly name.
		hr = pPropBag->Read(L"Description", &var, 0);
		if (FAILED(hr))
		{
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
		}
		if (SUCCEEDED(hr))
		{
			std::wstring str(var.bstrVal);

			std::size_t found = str.find(convert(friendly_name));

			if (found != std::string::npos) {
				hr = pPropBag->Read(L"DevicePath", &var, 0);
				camId[idLoopCnt++] = id;
			}
			VariantClear(&var);
		}

		id++;

		pPropBag->Release(); // Here was a memory leak - function return inside while() skips Release()
		pMoniker->Release(); // Here was a memory leak - function return inside while() skips Release()
	}

	return idLoopCnt;
}

int find_device(std::string friendly_name, int camId[], int maxNCams) 
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	int nDevices = -1;
	if (SUCCEEDED(hr))
	{
		IEnumMoniker *pEnum;

		hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
		if (SUCCEEDED(hr))
		{
			nDevices = get_id(pEnum, friendly_name, camId, maxNCams);
			pEnum->Release();
		}
		CoUninitialize();
	}
	return nDevices;
}

int main(int argc, char** argv)
{
	const int maxNCams = 256; // in windows there will never be more than 256 video devices.
	int camIds[maxNCams]; // reserve storage for return of id's
	int nDev = 0;
	if (argc >= 2) {
		nDev =  find_device(argv[1], camIds, maxNCams);
		for (int i= 0; i< nDev; i++)
			std::cout << " " << camIds[i]; // If anything found
	}
	else if (argc == 1) { // Be helpful to users and provide a best default.
		nDev = find_device("FLIR Video", camIds, maxNCams);
		for (int i = 0; i < nDev; i++)
			std::cout << " " << camIds[i]; // If anything found
	}
	if (nDev==0)
		std::cout << " " << -1; // to make it compatible with previous version which returned -1 if none found
	return 0;
}