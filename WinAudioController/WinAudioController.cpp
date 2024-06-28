#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <comdef.h>

#include "SerialReader.h"

void PrintHRErr(HRESULT hr) {
	_com_error err(hr);
	std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
}

IAudioSessionEnumerator* GetSessionEnumerator(IAudioSessionManager2* pSessionManager) {
	IAudioSessionEnumerator* pSessionEnumerator = NULL;
	HRESULT hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session enumerator: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		return NULL;
	}
	return pSessionEnumerator;
}

void ListAudioSessions(IAudioSessionManager2* pSessionManager) {
	IAudioSessionEnumerator* pSessionEnumerator = GetSessionEnumerator(pSessionManager);

	int sessionCount = 0;
	HRESULT hr = pSessionEnumerator->GetCount(&sessionCount);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session count: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pSessionEnumerator->Release();
		return;
	}

	for (int i = 0; i < sessionCount; ++i) {
		IAudioSessionControl* pSessionControl = NULL;
		hr = pSessionEnumerator->GetSession(i, &pSessionControl);
		if (FAILED(hr)) {
			std::cerr << "Failed to get session control: " << std::hex << hr << std::endl;
			PrintHRErr(hr);
			continue;
		}

		IAudioSessionControl2* pSessionControl2 = NULL;
		hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
		if (FAILED(hr)) {
			std::cerr << "Failed to query IAudioSessionControl2: " << std::hex << hr << std::endl;
			PrintHRErr(hr);
			pSessionControl->Release();
			continue;
		}

		DWORD processId = 0;
		hr = pSessionControl2->GetProcessId(&processId);
		if (FAILED(hr)) {
			std::cerr << "Failed to get process ID: " << std::hex << hr << std::endl;
			PrintHRErr(hr);
			pSessionControl2->Release();
			pSessionControl->Release();
			continue;
		}

		LPWSTR displayName = NULL;
		hr = pSessionControl2->GetDisplayName(&displayName);
		if (SUCCEEDED(hr) && displayName && displayName[0] != L'\0') {
			wprintf(L"Session %d: Process ID = %d, Display Name = %s\n", i, processId, displayName ? displayName : L"Unknown");
			CoTaskMemFree(displayName);
		}
		else {
			LPWSTR sessionIdentifier = NULL;
			hr = pSessionControl2->GetSessionIdentifier(&sessionIdentifier);
			if (SUCCEEDED(hr) && sessionIdentifier) {
				wprintf(L"Session %d: Process ID = %d, Session Identifier = %s\n", i, processId, sessionIdentifier);
				CoTaskMemFree(sessionIdentifier);
			}
			else {
				wprintf(L"Session %d: Process ID = %d, Display Name = Unknown\n", i, processId);
			}
		}

		ISimpleAudioVolume* pAudioVolume = NULL;
		hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
		if (SUCCEEDED(hr)) {
			float currentVolume = 0.0f;
			hr = pAudioVolume->GetMasterVolume(&currentVolume);
			if (SUCCEEDED(hr)) {
				wprintf(L"\tCurrent Volume: %f\n", currentVolume);

				float setVolume = 0.5f * currentVolume;
				/*
				hr = pAudioVolume->SetMasterVolume(setVolume, NULL);
				if (SUCCEEDED(hr)) {
					wprintf(L"\tVolume set to: %f", setVolume);
				}
				else {
					std::cerr << "\tFailed to set volume: " << std::hex << hr << std::endl;
					PrintHRErr(hr);
				}
				*/
			}
			else {
				std::cerr << "\tFailed to get current volume: " << std::hex << hr << std::endl;
				PrintHRErr(hr);
			}
			pAudioVolume->Release();
		}
		else {
			std::cerr << "\tFailed to query ISimpleAudioVolume: " << std::hex << hr << std::endl;
			PrintHRErr(hr);
		}

		pSessionControl2->Release();
		pSessionControl->Release();
	}
	pSessionEnumerator->Release();
}

int main()
{
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioSessionManager2* pSessionManager = NULL;
	IAudioSessionEnumerator* pSessionEnumerator = NULL;

	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		std::cerr << "COM library initialization failed: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		return -1;
	}

	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&pEnumerator);
	if (FAILED(hr)) {
		std::cerr << "Failed to create device enumerator: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		CoUninitialize();
		return -1;
	}

	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	if (FAILED(hr)) {
		std::cerr << "Failed to get default audio endpoint: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pEnumerator->Release();
		CoUninitialize();
		return -1;
	}

	hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);
	if (FAILED(hr)) {
		std::cerr << "Failed to get audio session manager: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pDevice->Release();
		pEnumerator->Release();
		CoUninitialize();
		return -1;
	}

	SerialReader reader("COM3");

	while (true) {
		std::string data = reader.readData();
		if (!data.empty()) {
			std::cout << data << std::endl;

			RemoteButton btn = reader.getButton(data);
			if (btn == RemoteButton::POWER) {
				std::cout << "POWER" << std::endl;
				ListAudioSessions(pSessionManager);
			}
			switch (btn) {
			}
		}
		Sleep(100);
	}
	
	pSessionEnumerator->Release();
	pSessionManager->Release();
	pDevice->Release();
	pEnumerator->Release();

	CoUninitialize();

	return 0;
}
