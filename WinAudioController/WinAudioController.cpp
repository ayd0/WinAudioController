#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <locale>
#include <codecvt>

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

IAudioSessionControl2* GetSessionControl2(IAudioSessionEnumerator* pSessionEnumerator, int idx) {
	IAudioSessionControl* pSessionControl = NULL;
	HRESULT hr = pSessionEnumerator->GetSession(idx, &pSessionControl);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session control: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		return NULL;
	}

	IAudioSessionControl2* pSessionControl2 = NULL;
	hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
	if (FAILED(hr)) {
		std::cerr << "Failed to query IAudioSessionControl2: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pSessionControl->Release();
		return NULL;
	}

	pSessionControl->Release();
	return pSessionControl2;
}

ISimpleAudioVolume* GetSimpleAudioVolume(IAudioSessionControl2* pSessionControl2) {
	ISimpleAudioVolume* pAudioVolume = NULL;
	HRESULT hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
	if (FAILED(hr)) {
		std::cerr << "\tFailed to query ISimpleAudioVolume: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
	}

	return pAudioVolume;
}

void PrintAudioSession(IAudioSessionControl2* pSessionControl2, int idx, bool printVol=false) {
	DWORD processId = 0;
	HRESULT hr = pSessionControl2->GetProcessId(&processId);
	if (FAILED(hr)) {
		std::cerr << "Failed to get process ID: " << std::hex << hr << std::endl;
		PrintHRErr(hr);
		pSessionControl2->Release();
		return;
	}

	LPWSTR displayName = NULL;
	hr = pSessionControl2->GetDisplayName(&displayName);
	if (SUCCEEDED(hr) && displayName && displayName[0] != L'\0') {
		// wprintf(L"Session %d: Process ID = %d, Display Name = %s\n", idx, processId, displayName ? displayName : L"Unknown");
		wprintf(L"%d: Display Name = %s\n", idx, displayName ? displayName : L"Unknown");
		CoTaskMemFree(displayName);
	}
	else {
		LPWSTR sessionIdentifier = NULL;
		hr = pSessionControl2->GetSessionIdentifier(&sessionIdentifier);
		if (SUCCEEDED(hr) && sessionIdentifier) {
			// wprintf(L"Session %d: Process ID = %d, Session Identifier = %s\n", idx, processId, sessionIdentifier);
			wprintf(L"%d: Session Identifier = %s\n", idx, sessionIdentifier);
			CoTaskMemFree(sessionIdentifier);
		}
		else {
			// wprintf(L"%d: Display Name = Unknown\n", idx, processId);
			wprintf(L"Session %d: Process ID = %d, Display Name = Unknown\n", idx);
		}
	}
	if (printVol) {
		ISimpleAudioVolume* pAudioVolume = GetSimpleAudioVolume(pSessionControl2);
		if (pAudioVolume) {
			float currentVolume = 0.0f;
			hr = pAudioVolume->GetMasterVolume(&currentVolume);
			if (SUCCEEDED(hr)) {
				wprintf(L"\tVolume: %f\n", currentVolume);
			}
			else {
				std::cerr << "\tFailed to get current volume: " << std::hex << hr << std::endl;
				PrintHRErr(hr);
			}
			pAudioVolume->Release();
		}
	}
}

void SetAudioSessionVolume(IAudioSessionControl2* pSessionControl2, RemoteButton btn) {
	ISimpleAudioVolume* pAudioVolume = NULL;
	HRESULT hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
	if (SUCCEEDED(hr)) {
		float currentVolume = 0.0f;
		hr = pAudioVolume->GetMasterVolume(&currentVolume);
		if (SUCCEEDED(hr)) {
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
				wprintf(L"\tVolume set to: %f\n", setVolume);
			}
			else {
				std::cerr << "\tFailed to set volume: " << std::hex << hr << std::endl;
				PrintHRErr(hr);
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
}

void HandleAudioSessions(IAudioSessionManager2* pSessionManager, int sessionTarget, bool printVol=true, bool updateVol = false, RemoteButton btn = RemoteButton::UNKNOWN) {
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
			IAudioSessionControl2* pSessionControl2 = GetSessionControl2(pSessionEnumerator, i);
			if (pSessionControl2) {
				PrintAudioSession(pSessionControl2, i, printVol);
			}
			if (updateVol && pSessionControl2) {
				SetAudioSessionVolume(pSessionControl2, btn);
			}
			pSessionControl2->Release();
		}
		std::cout << std::endl;
	}
	else if (sessionTarget < sessionCount) {
		IAudioSessionControl2* pSessionControl2 = GetSessionControl2(pSessionEnumerator, sessionTarget);
		if (pSessionControl2) {
			PrintAudioSession(pSessionControl2, sessionTarget, printVol);
		}
		if (updateVol && pSessionControl2) {
			SetAudioSessionVolume(pSessionControl2, btn);
		}
		pSessionControl2->Release();
	}
	pSessionEnumerator->Release();
}

void HandleBtn(RemoteButton btn, IAudioSessionManager2* pSessionManager, int selectedSession, std::string& data) {
	switch (btn) {
		case RemoteButton::POWER :
			HandleAudioSessions(pSessionManager, -1, false);
			break;
		case RemoteButton::VOL_UP :
			HandleAudioSessions(pSessionManager, selectedSession, false, true, btn);
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
			HandleAudioSessions(pSessionManager, selectedSession, false, true, btn);
			break;
		case RemoteButton::VOL_DOWN :
			HandleAudioSessions(pSessionManager, selectedSession, false, true, btn);
			break;
		case RemoteButton::UP :
			HandleAudioSessions(pSessionManager, selectedSession, false, true, btn);
			break;
		case RemoteButton::BTN_0 :
			HandleAudioSessions(pSessionManager, 0);
			selectedSession = 0;
			break;
		case RemoteButton::EQ :
			std::cout << "EQ" << std::endl;
			break;
		case RemoteButton::REPT :
			std::cout << "REPT" << std::endl;
			break;
		case RemoteButton::BTN_1 :
			HandleAudioSessions(pSessionManager, 1);
			selectedSession = 1;
			break;
		case RemoteButton::BTN_2 :
			HandleAudioSessions(pSessionManager, 2);
			selectedSession = 2;
			break;
		case RemoteButton::BTN_3 :
			HandleAudioSessions(pSessionManager, 3);
			selectedSession = 3;
			break;
		case RemoteButton::BTN_4 :
			HandleAudioSessions(pSessionManager, 4);
			selectedSession = 4;
			break;
		case RemoteButton::BTN_5 :
			HandleAudioSessions(pSessionManager, 5);
			selectedSession = 5;
			break;
		case RemoteButton::BTN_6 :
			HandleAudioSessions(pSessionManager, 6);
			selectedSession = 6;
			break;
		case RemoteButton::BTN_7 :
			HandleAudioSessions(pSessionManager, 7);
			selectedSession = 7;
			break;
		case RemoteButton::BTN_8 :
			HandleAudioSessions(pSessionManager, 8);
			selectedSession = 8;
			break;
		case RemoteButton::BTN_9 :
			HandleAudioSessions(pSessionManager, 9);
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

	HandleAudioSessions(pSessionManager, -1, false);

	int selectedSession = 0;
	SerialReader reader("COM3");

	while (true) {
		std::string data = reader.readData();
		if (!data.empty()) {
			RemoteButton btn = reader.getButton(data);
			HandleBtn(btn, pSessionManager, selectedSession, data);
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
