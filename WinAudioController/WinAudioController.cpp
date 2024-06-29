#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <comdef.h>
#include <locale>
#include <codecvt>

#include "AudioSessionManager.h"

int main()
{
	AudioSessionManager ASM;
	ASM.Listen(100);

	return 0;
}
