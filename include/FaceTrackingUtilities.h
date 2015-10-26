#pragma once

#include <windows.h>
#include <map>
#include "pxcdefs.h"
#include "pxcfaceconfiguration.h"

enum StatusWindowPart { statusPart, alertPart };
extern std::map<int, PXCFaceConfiguration::TrackingModeType> s_profilesMap;

class FaceTrackingUtilities
{
public:
	static int GetChecked(HMENU menu);
	static pxcCHAR* GetCheckedDevice(HWND dialogWindow);
	static pxcCHAR* GetCheckedModule(HWND dialogWindow);
	static void SetStatus(HWND dialogWindow, pxcCHAR *line, StatusWindowPart part);
	static bool IsModuleSelected(HWND hwndDlg, const int moduleID);
	static bool GetRecordState(HWND hwndDlg);
	static bool GetPlaybackState(HWND DialogWindow);
	static PXCFaceConfiguration::TrackingModeType GetCheckedProfile(HWND dialogWindow);
	static HANDLE& GetRenderingFinishedSignal();
	static void SignalProcessor();
	static const int TextHeight = 16;
};
