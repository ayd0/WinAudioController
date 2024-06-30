#include "AudioSessionManager.h"

AudioSessionManager::AudioSessionManager() : selectedSession(0), sessionCount(0), handler("COM3") {
	InitializeASM();
}
AudioSessionManager::~AudioSessionManager() {
	Cleanup();
}

void AudioSessionManager::Listen(int delay) {
	while (true) {
		handler.readData();
		if (!handler.data.empty()) {
			selectedBtn = handler.getButton();
			HandleBtn();
		}
		Sleep(delay);
	}
}

void AudioSessionManager::PrintHRErr() {
	_com_error err(hr);
	std::wcout << L"Error: " << err.ErrorMessage() << std::endl;
}

void AudioSessionManager::Cleanup() {
	if (pAudioVolume) {
		pAudioVolume->Release();
	}
	if (pSessionControl2) {
		pSessionControl2->Release();
	}
	if (pSessionControl) {
		pSessionControl->Release();
	}
	if (pSessionEnumerator) {
		pSessionEnumerator->Release();
	}
	if (pSessionManager) {
		pSessionManager->Release();
	}
	if (pDevice) {
		pDevice->Release();
	}
	if (pEnumerator) {
		pEnumerator->Release();
	}
	CoUninitialize();
}

void AudioSessionManager::EarlyExit(int code) {
	Cleanup();
	ExitProcess(code);
}

void AudioSessionManager::InitializeASM() {
	pEnumerator = NULL;
	pDevice = NULL;
	pSessionManager = NULL;
	pSessionEnumerator = NULL;
	pSessionControl = NULL;
	pSessionControl2 = NULL;
	pAudioVolume = NULL;

	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		std::cerr << "COM library initialization failed: " << std::hex << hr << std::endl;
		PrintHRErr();
		EarlyExit(-1);
	}

	hr = CoCreateInstance(
		__uuidof(MMDeviceEnumerator), NULL,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
		(void**)&pEnumerator);
	if (FAILED(hr)) {
		std::cerr << "Failed to create device enumerator: " << std::hex << hr << std::endl;
		PrintHRErr();
		CoUninitialize();
		EarlyExit(-1);
	}

	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	if (FAILED(hr)) {
		std::cerr << "Failed to get default audio endpoint: " << std::hex << hr << std::endl;
		PrintHRErr();
		pEnumerator->Release();
		CoUninitialize();
		EarlyExit(-1);
	}

	hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);
	if (FAILED(hr)) {
		std::cerr << "Failed to get audio session manager: " << std::hex << hr << std::endl;
		PrintHRErr();
		pDevice->Release();
		pEnumerator->Release();
		CoUninitialize();
		EarlyExit(-1);
	}

	HandleAudioSessions(-1);
}

void AudioSessionManager::HandleBtn() {
	switch (selectedBtn) {
		case RemoteButton::POWER :
			HandleAudioSessions(-1);
			break;
		case RemoteButton::VOL_UP :
			HandleAudioSessions(selectedSession, false, false, true);
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
			HandleAudioSessions(selectedSession, false, false, true);
			break;
		case RemoteButton::VOL_DOWN :
			HandleAudioSessions(selectedSession, false, false, true);
			break;
		case RemoteButton::UP :
			HandleAudioSessions(selectedSession, false, false, true);
			break;
		case RemoteButton::BTN_0 :
			HandleAudioSessions(0);
			selectedSession = 0;
			break;
		case RemoteButton::EQ :
			std::cout << "EQ" << std::endl;
			break;
		case RemoteButton::REPT :
			std::cout << "REPT" << std::endl;
			break;
		case RemoteButton::BTN_1 :
			HandleAudioSessions(1);
			selectedSession = 1;
			break;
		case RemoteButton::BTN_2 :
			HandleAudioSessions(2);
			selectedSession = 2;
			break;
		case RemoteButton::BTN_3 :
			HandleAudioSessions(3);
			selectedSession = 3;
			break;
		case RemoteButton::BTN_4 :
			HandleAudioSessions(4);
			selectedSession = 4;
			break;
		case RemoteButton::BTN_5 :
			HandleAudioSessions(5);
			selectedSession = 5;
			break;
		case RemoteButton::BTN_6 :
			HandleAudioSessions(6);
			selectedSession = 6;
			break;
		case RemoteButton::BTN_7 :
			HandleAudioSessions(7);
			selectedSession = 7;
			break;
		case RemoteButton::BTN_8 :
			HandleAudioSessions(8);
			selectedSession = 8;
			break;
		case RemoteButton::BTN_9 :
			HandleAudioSessions(9);
			selectedSession = 9;
			break;
		case RemoteButton::UNKNOWN :
			std::cerr << "Unknown button: " << handler.data << std::endl;
			break;
		default:
			std::cout << handler.data << std::endl;
			std::cout << "LENGTH: " << handler.data.size() << std::endl;
	}
}

void AudioSessionManager::HandleAudioSessions(int sessionTarget, bool printName, bool printVol, bool updateVol) {
	GetSessionEnumerator();

	hr = pSessionEnumerator->GetCount(&sessionCount);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session count: " << std::hex << hr << std::endl;
		PrintHRErr();
		pSessionEnumerator->Release();
		return;
	}

	if (sessionTarget == -1) {
		for (int i = 0; i < sessionCount; ++i) {
			GetSessionControl2(i);
			if (pSessionControl2) {
				if (printName) {
					PrintAudioSession(i, printVol);
				}
				if (updateVol) {
					SetAudioSessionVolume();
				}
				pSessionControl2->Release();
			}
		}
		std::cout << std::endl;
	}
	else if (sessionTarget < sessionCount) {
		GetSessionControl2(sessionTarget);
		if (pSessionControl2) {
			if (printName) {
				PrintAudioSession(sessionTarget, printVol);
			}
			if (updateVol) {
				SetAudioSessionVolume();
			}
			pSessionControl2->Release();
		}
	}
	pSessionEnumerator->Release();
}

void AudioSessionManager::GetSessionEnumerator() {
	hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session enumerator: " << std::hex << hr << std::endl;
		PrintHRErr();
		pSessionEnumerator = NULL;
	}
}

void AudioSessionManager::GetSessionControl2(int idx) {
	pSessionControl = NULL;
	hr = pSessionEnumerator->GetSession(idx, &pSessionControl);
	if (FAILED(hr)) {
		std::cerr << "Failed to get session control: " << std::hex << hr << std::endl;
		PrintHRErr();
		pSessionControl = NULL;
		pSessionControl2 = NULL;
	}

	pSessionControl2 = NULL;
	hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
	if (FAILED(hr)) {
		std::cerr << "Failed to query IAudioSessionControl2: " << std::hex << hr << std::endl;
		PrintHRErr();
		pSessionControl->Release();
		pSessionControl2 = NULL;
	}

	pSessionControl->Release();
}

void AudioSessionManager::PrintAudioSession(int idx, bool printVol) {
	DWORD processId = 0;
	hr = pSessionControl2->GetProcessId(&processId);
	if (FAILED(hr)) {
		std::cerr << "Failed to get process ID: " << std::hex << hr << std::endl;
		PrintHRErr();
		pSessionControl2->Release();
		return;
	}

	LPWSTR sessionIdentifier = NULL;
	hr = pSessionControl2->GetSessionIdentifier(&sessionIdentifier);
	std::string sessionName;
	if (SUCCEEDED(hr) && sessionIdentifier) {
		std::wstring ident = std::wstring(sessionIdentifier);
		size_t lastBsPos = ident.find_last_of(L'\\');
		size_t PerBPos = ident.find(L"%b");

		if (lastBsPos != std::wstring::npos && PerBPos != std::wstring::npos && PerBPos > lastBsPos) {
			std::wstring extractedName = ident.substr(lastBsPos + 1, PerBPos - lastBsPos - 1);
			wprintf(L"%d: %s\n", idx, extractedName.c_str());
			sessionName = std::string(extractedName.begin(), extractedName.end());
		}
		else {
			LPWSTR displayName = NULL;
			hr = pSessionControl2->GetDisplayName(&displayName);
			if (SUCCEEDED(hr) && displayName && displayName[0] != L'\0') {
				wprintf(L"%d: %s\n", idx, displayName ? displayName : L"Unknown");
				sessionName = std::string(displayName, displayName + wcslen(displayName));
				CoTaskMemFree(displayName);
			}
			else {
				wprintf(L"%d: %s\n", idx, sessionIdentifier);
				sessionName = std::string(sessionIdentifier, sessionIdentifier + wcslen(sessionIdentifier));
			}
		}

		CoTaskMemFree(sessionIdentifier);
	}
	else {
		wprintf(L"Session %d: Process ID = %d, Display Name = Unknown\n", idx);
		sessionName = "Unknown";
	}
	if (sessionName.size() > 16) {
		sessionName = sessionName.substr(0, 15);
	}
	sessionName += "\n";
	handler.writeData(sessionName);

	if (printVol) {
		GetSimpleAudioVolume();
		if (pAudioVolume) {
			float currentVolume = 0.0f;
			hr = pAudioVolume->GetMasterVolume(&currentVolume);
			if (SUCCEEDED(hr)) {
				wprintf(L"Vol: %f\n", currentVolume);
			}
			else {
				std::cerr << "\tFailed to get current volume: " << std::hex << hr << std::endl;
				PrintHRErr();
			}
			pAudioVolume->Release();
		}
	}
}

void AudioSessionManager::SetAudioSessionVolume() {
	pAudioVolume = NULL;
	hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
	if (SUCCEEDED(hr)) {
		float currentVolume = 0.0f;
		hr = pAudioVolume->GetMasterVolume(&currentVolume);
		if (SUCCEEDED(hr)) {
			float setVolume = 0.0f;
			if (selectedBtn == RemoteButton::UP || selectedBtn == RemoteButton::VOL_UP) {
				setVolume = currentVolume + 0.2f;
				if (setVolume > 1.0f) {
					setVolume = 1.0f;
				}
			}
			else if (selectedBtn == RemoteButton::DOWN || selectedBtn == RemoteButton::VOL_DOWN) {
				setVolume = currentVolume - 0.2f;
				if (setVolume < 0.0f) {
					setVolume = 0.0f;
				}
			}
			hr = pAudioVolume->SetMasterVolume(setVolume, NULL);
			if (SUCCEEDED(hr)) {
				wprintf(L"Vol: %f\n", setVolume);
				std::string vol = "Vol: " + std::to_string(currentVolume) + '\n';
				handler.writeData(vol);
			}
			else {
				std::cerr << "\tFailed to set volume: " << std::hex << hr << std::endl;
				PrintHRErr();
			}
		}
		else {
			std::cerr << "\tFailed to get current volume: " << std::hex << hr << std::endl;
			PrintHRErr();
		}
		pAudioVolume->Release();
	}
	else {
		std::cerr << "\tFailed to query ISimpleAudioVolume: " << std::hex << hr << std::endl;
		PrintHRErr();
	}
}

void AudioSessionManager::GetSimpleAudioVolume() {
	ISimpleAudioVolume* pAudioVolume = NULL;
	hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pAudioVolume);
	if (FAILED(hr)) {
		std::cerr << "\tFailed to query ISimpleAudioVolume: " << std::hex << hr << std::endl;
		PrintHRErr();
		pAudioVolume = NULL;
	}
}
