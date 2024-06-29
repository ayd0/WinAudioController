#ifndef AUDIO_SESSION_MANAGER
#define AUDIO_SESSION_MANAGER

#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <locale>
#include <codecvt>

#include "SerialReader.h"

class AudioSessionManager {
	IMMDeviceEnumerator* pEnumerator;
	IMMDevice* pDevice;
	IAudioSessionManager2* pSessionManager;
	IAudioSessionEnumerator* pSessionEnumerator;
	IAudioSessionControl* pSessionControl;
	IAudioSessionControl2* pSessionControl2;
	ISimpleAudioVolume* pAudioVolume;

	int selectedSession;
	int sessionCount;
	SerialReader reader;
	RemoteButton selectedBtn;
	HRESULT hr;

public:
	AudioSessionManager();
	~AudioSessionManager();

	void Listen(int delay);

private:
	void PrintHRErr();
	void Cleanup();
	void EarlyExit(int code);
	void InitializeASM();
	void HandleBtn();
	void HandleAudioSessions(int sessionTarget, bool printName = true, bool printVol = true, bool updateVol = false);
	void GetSessionEnumerator();
	void GetSessionControl2(int idx);
	void PrintAudioSession(int idx, bool printVol = false);
	void SetAudioSessionVolume();
	void GetSimpleAudioVolume();
};

#endif // AUDIO_SESION_MANAGER