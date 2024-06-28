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

void PrintAudioSession(IAudioSessionEnumerator* pSessionEnumerator, int idx, bool updateVol, RemoteButton btn) {
	IAudioSessionControl* pSessionControl = NULL;
	HRESULT hr = pSessionEnumerator->GetSession(idx, &pSessionControl);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session control: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		return;
	}

	IAudioSessionControl2* pSessionControl2 = NULL;
	hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
	if (FAILED(hr)) {
		std::cerr << "Failed to query IAudioSessionControl2: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pSessionControl->Release();
		return;
	}

	DWORD processId = 0;
	hr = pSessionControl2->GetProcessId(&processId);
	if (FAILED(hr)) {
		std::cerr << "Failed to get process ID: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pSessionControl2->Release();
		pSessionControl->Release();
		return;
	}

	LPWSTR displayName = NULL;
	hr = pSessionControl2->GetDisplayName(&displayName);
	if (SUCCEEDED(hr) && displayName && displayName[0] != L'\0') {
		wprintf(L"Session %d: Process ID = %d, Display Name = %s\n", idx, processId, displayName ? displayName : L"Unknown");
		CoTaskMemFree(displayName);
	}
	else {
		LPWSTR sessionIdentifier = NULL;
		hr = pSessionControl2->GetSessionIdentifier(&sessionIdentifier);
		if (SUCCEEDED(hr) && sessionIdentifier) {
			wprintf(L"Session %d: Process ID = %d, Session Identifier = %s\n", idx, processId, sessionIdentifier);
			CoTaskMemFree(sessionIdentifier);
		}
		else {
			wprintf(L"Session %d: Process ID = %d, Display Name = Unknown\n", idx, processId);
		}
	}

	ISimpleAudioVolume* pAudioVolume = NULL;
	hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
	if (SUCCEEDED(hr)) {
		float currentVolume = 0.0f;
		hr = pAudioVolume->GetMasterVolume(&currentVolume);
		if (SUCCEEDED(hr)) {
			if (updateVol) {
				float setVolume = 0.0f;
				if (btn == RemoteButton::UP || btn == RemoteButton::VOL_UP) {
					setVolume = currentVolume + 0.2f;
					if (setVolume > 1.0f) {
						setVolume = 1.0f;
					}
				}
				else if (btn == RemoteButton::DOWN || btn == RemoteButton::VOL_DOWN) {
					setVolume = currentVolume - 0.2f;
					if (setVolume < 0.0f) {
						setVolume = 0.0f;
					}
				}
				hr = pAudioVolume->SetMasterVolume(setVolume, NULL);
				if (SUCCEEDED(hr)) {
					wprintf(L"\tVolume set to: %f", setVolume);
				}
				else {
					std::cerr << "\tFailed to set volume: " << std::hex << hr << std::endl;
					PrintHRErr(hr);
				}
			}
			else {
				wprintf(L"\tCurrent Volume: %f\n", currentVolume);
			}
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

void ListAudioSessions(IAudioSessionManager2* pSessionManager, int sessionTarget, bool updateVol=false, RemoteButton btn=RemoteButton::UNKNOWN) {
	IAudioSessionEnumerator* pSessionEnumerator = GetSessionEnumerator(pSessionManager);

	int sessionCount = 0;
	HRESULT hr = pSessionEnumerator->GetCount(&sessionCount);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session count: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pSessionEnumerator->Release();
		return;
	}

	if (sessionTarget == -1) {
		for (int i = 0; i < sessionCount; ++i) {
			PrintAudioSession(pSessionEnumerator, i, updateVol, btn);
		}
	}
	else if (sessionTarget < sessionCount) {
		PrintAudioSession(pSessionEnumerator, sessionTarget, updateVol, btn);
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

	int selectedSession = 0;
	SerialReader reader("COM3");

	while (true) {
		std::string data = reader.readData();
		if (!data.empty()) {
			RemoteButton btn = reader.getButton(data);

			switch (btn) {
				case RemoteButton::POWER :
					ListAudioSessions(pSessionManager, -1);
					break;
				case RemoteButton::VOL_UP :
					ListAudioSessions(pSessionManager, selectedSession, true, btn);
					break;
				case RemoteButton::FUNC :
					std::cout << "FUNC" << std::endl;
					break;
				case RemoteButton::REWIND :
					std::cout << "REWIND" << std::endl;
					break;
				case RemoteButton::PAUSE :
					std::cout << "PAUSE" << std::endl;
					break;
				case RemoteButton::FAST_FORWARD :
					std::cout << "FAST_FORWARD" << std::endl;
					break;
				case RemoteButton::DOWN :
					ListAudioSessions(pSessionManager, selectedSession, true, btn);
					break;
				case RemoteButton::VOL_DOWN :
					ListAudioSessions(pSessionManager, selectedSession, true, btn);
					break;
				case RemoteButton::UP :
					ListAudioSessions(pSessionManager, selectedSession, true, btn);
					break;
				case RemoteButton::BTN_0 :
					ListAudioSessions(pSessionManager, 0);
					selectedSession = 0;
					break;
				case RemoteButton::EQ :
					std::cout << "EQ" << std::endl;
					break;
				case RemoteButton::REPT :
					std::cout << "REPT" << std::endl;
					break;
				case RemoteButton::BTN_1 :
					ListAudioSessions(pSessionManager, 1);
					selectedSession = 1;
					break;
				case RemoteButton::BTN_2 :
					ListAudioSessions(pSessionManager, 2);
					selectedSession = 2;
					break;
				case RemoteButton::BTN_3 :
					ListAudioSessions(pSessionManager, 3);
					selectedSession = 3;
					break;
				case RemoteButton::BTN_4 :
					ListAudioSessions(pSessionManager, 4);
					selectedSession = 4;
					break;
				case RemoteButton::BTN_5 :
					ListAudioSessions(pSessionManager, 5);
					selectedSession = 5;
					break;
				case RemoteButton::BTN_6 :
					ListAudioSessions(pSessionManager, 6);
					selectedSession = 6;
					break;
				case RemoteButton::BTN_7 :
					ListAudioSessions(pSessionManager, 7);
					selectedSession = 7;
					break;
				case RemoteButton::BTN_8 :
					ListAudioSessions(pSessionManager, 8);
					selectedSession = 8;
					break;
				case RemoteButton::BTN_9 :
					ListAudioSessions(pSessionManager, 9);
					selectedSession = 9;
					break;
				case RemoteButton::UNKNOWN :
					std::cerr << "Unknown button: " << data << std::endl;
					break;
				default:
					std::cout << data << std::endl;
					std::cout << "LENGTH: " << data.size() << std::endl;
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
