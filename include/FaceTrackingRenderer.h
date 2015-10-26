#pragma once

#include <windows.h>
#include <WindowsX.h>
#include <wchar.h>
#include <string>
#include <assert.h>
#include <map>
#include "resource.h"
#include "pxcfacedata.h"
#include "FaceTrackingFrameRateCalculator.h"



typedef void (*OnFinishedRenderingCallback)();

class PXCImage;

class FaceTrackingRenderer
{
public:
	FaceTrackingRenderer(HWND window);
	~FaceTrackingRenderer();

	void SignalRenderer();
	void SetCallback(OnFinishedRenderingCallback callback);
	void SetOutput(PXCFaceData* output);
	void DrawBitmap(PXCImage* colorImage);
	void Render();

private:
	static const int LANDMARK_ALIGNMENT = -3;
	PXCFaceData* m_currentFrameOutput;
	HANDLE m_rendererSignal;
	HWND m_window;
	HBITMAP m_bitmap;
	FaceTrackingFrameRateCalculator m_frameRateCalcuator;
	OnFinishedRenderingCallback m_callback;
	std::map<PXCFaceData::ExpressionsData::FaceExpression, std::wstring> m_expressionMap;

	void DrawFrameRate();
	void DrawLocation(PXCFaceData::Face* trackedFace);
	void DrawLandmark(PXCFaceData::Face* trackedFace);
	void DrawPose(PXCFaceData::Face* trackedFace, const int faceId);
	void DrawExpressions(PXCFaceData::Face* trackedFace, const int faceId);
	void DrawRecognition(PXCFaceData::Face* trackedFace, const int faceId);
	void DrawGraphics(PXCFaceData* faceOutput);
	void RefreshUserInterface();
	RECT GetResizeRect(RECT rectangle, BITMAP bitmap);
	std::map<PXCFaceData::ExpressionsData::FaceExpression, std::wstring> InitExpressionsMap();
};
