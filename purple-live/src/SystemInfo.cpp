#include "SystemInfo.h"


#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

void SystemInfo::GetSystemInfo(string& cpuInfo, string& osInfo)
{
	// Obtain the initial locator to Windows Management on a particular host computer.
	IWbemLocator *locator = nullptr;
	IWbemServices *services = nullptr;

	CoInitialize(nullptr);

	auto hResult = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&locator);

	auto hasFailed = [&hResult]() {
		if (FAILED(hResult)) {
			auto error = _com_error(hResult);
			//			TRACE(error.ErrorMessage());
			//			TRACE(error.Description().Detach());
			return true;
		}
		return false;
	};

	auto getValue = [&hResult, &hasFailed](IWbemClassObject *classObject, LPCWSTR property)
	{
		wstring propertyValueText = L"Not set";
		VARIANT propertyValue;
		hResult = classObject->Get(property, 0, &propertyValue, 0, 0);
		if (!hasFailed())
		{
			if ((propertyValue.vt == VT_NULL) || (propertyValue.vt == VT_EMPTY))
			{
			}
			else if (propertyValue.vt & VT_ARRAY)
			{
				propertyValueText = L"Unknown"; //Array types not supported
			}
			else
			{
				propertyValueText = propertyValue.bstrVal;
			}
		}
		VariantClear(&propertyValue);
		return propertyValueText;
	};

	wstring cpu_manufacturer = L"Unknown";
	wstring cpu_name = L"Unknown";
	wstring os_version = L"Unknown";
	wstring os_bit = L"Unknown";
	if (!hasFailed())
	{
		// Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
		hResult = locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, 0, NULL, 0, 0, &services);

		if (!hasFailed())
		{
			// Set the IWbemServices proxy so that impersonation of the user (client) occurs.
			hResult = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
				RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

			if (!hasFailed()) 
			{
				// processor info
				IEnumWbemClassObject* classObjectEnumerator = nullptr;
				hResult = services->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM Win32_Processor"), 
					WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, 
					&classObjectEnumerator);

				if (!hasFailed()) 
				{
					IWbemClassObject *classObject;
					ULONG uReturn = 0;
					hResult = classObjectEnumerator->Next(WBEM_INFINITE, 1, &classObject, &uReturn);
					if (uReturn != 0) 
					{
						cpu_manufacturer = getValue(classObject, (LPCWSTR)L"Manufacturer");
						cpu_name = getValue(classObject, (LPCWSTR)L"Name");
					}
					classObject->Release();
				}
				classObjectEnumerator->Release();

				// os info
				classObjectEnumerator = nullptr;
				hResult = services->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM Win32_OperatingSystem"),
					WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
					&classObjectEnumerator);

				if (!hasFailed())
				{
					IWbemClassObject *classObject;
					ULONG uReturn = 0;
					hResult = classObjectEnumerator->Next(WBEM_INFINITE, 1, &classObject, &uReturn);
					if (uReturn != 0)
					{
						os_version = getValue(classObject, (LPCWSTR)L"Version");
						os_bit = getValue(classObject, (LPCWSTR)L"OSArchitecture");
					}
					classObject->Release();
				}
				classObjectEnumerator->Release();
			}
		}
	}

	if (locator) {
		locator->Release();
	}
	if (services) {
		services->Release();
	}
	CoUninitialize();

	wstring w_cpu = cpu_manufacturer + L"|" + cpu_name;
	cpuInfo.assign(w_cpu.begin(), w_cpu.end());
	wstring w_os = os_version + L"." + os_bit;
	osInfo.assign(w_os.begin(), w_os.end());
}
