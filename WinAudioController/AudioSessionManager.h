#ifndef AUDIO_SESSION_MANAGER
#define AUDIO_SESSION_MANAGER

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <locale>
#include <codecvt>

class AudioSessionManager {
	IMMDeviceEnumerator* pEnumerator;
	IMMDevice* pDevice;
	IAudioSessionManager2* pSessionManager;
	IAudioSessionEnumerator* pSessionEnumerator;

public:
	AudioSessionManager() {
		InitializeASM();
	}

private:
	void InitializeASM() {
		pEnumerator = NULL;
		pDevice = NULL;
		pSessionManager = NULL;
		pSessionEnumerator = NULL;
	}
};

#endif // AUDIO_SESION_MANAGER