#include "FaceTrackingRenderer.h"
#include "FaceTrackingUtilities.h"
#include "pxccapture.h"
#include <map>
#include "SerialClass.h"
// duo begin
#include <fstream>
using namespace std;
// duo end



FaceTrackingRenderer::~FaceTrackingRenderer()
{
	CloseHandle(m_rendererSignal);
}

FaceTrackingRenderer::FaceTrackingRenderer(HWND window) : m_window(window)
{
	m_expressionMap = InitExpressionsMap();
	m_rendererSignal = CreateEvent(NULL, FALSE, FALSE, NULL);

}

void FaceTrackingRenderer::SignalRenderer()
{
	SetEvent(m_rendererSignal);
}

void FaceTrackingRenderer::SetCallback(OnFinishedRenderingCallback callback)
{
	m_callback = callback;
}

void FaceTrackingRenderer::SetOutput(PXCFaceData* output)
{
	m_currentFrameOutput = output;
}

void FaceTrackingRenderer::Render()
{
	WaitForSingleObject(m_rendererSignal, INFINITE);

	DrawFrameRate();
	DrawGraphics(m_currentFrameOutput);
	RefreshUserInterface();

	m_callback();
}

void FaceTrackingRenderer::DrawBitmap(PXCImage* colorImage)
{
	if (m_bitmap) 
	{
		DeleteObject(m_bitmap);
		m_bitmap = 0;
	}

	PXCImage::ImageInfo info = colorImage->QueryInfo();
	PXCImage::ImageData data;
	if (colorImage->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &data) >= PXC_STATUS_NO_ERROR)
	{
		HWND hwndPanel = GetDlgItem(m_window, IDC_PANEL);
		HDC dc = GetDC(hwndPanel);
		BITMAPINFO binfo;
		memset(&binfo, 0, sizeof(binfo));
		binfo.bmiHeader.biWidth = data.pitches[0]/4;
		binfo.bmiHeader.biHeight = - (int)info.height;
		binfo.bmiHeader.biBitCount = 32;
		binfo.bmiHeader.biPlanes = 1;
		binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		binfo.bmiHeader.biCompression = BI_RGB;
		Sleep(1);
		m_bitmap = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, data.planes[0], &binfo, DIB_RGB_COLORS);

		ReleaseDC(hwndPanel, dc);
		colorImage->ReleaseAccess(&data);
	}
}

void FaceTrackingRenderer::DrawGraphics(PXCFaceData* faceOutput)
{
	assert(faceOutput != NULL);
	if (!m_bitmap) return;

	const int numFaces = faceOutput->QueryNumberOfDetectedFaces();
	for (int i = 0; i < numFaces; ++i) 
	{
		PXCFaceData::Face* trackedFace = faceOutput->QueryFaceByIndex(i);		
		assert(trackedFace != NULL);
		if (FaceTrackingUtilities::IsModuleSelected(m_window, IDC_LOCATION) && trackedFace->QueryDetection() != NULL)
			DrawLocation(trackedFace);
		if (FaceTrackingUtilities::IsModuleSelected(m_window, IDC_LANDMARK) && trackedFace->QueryLandmarks() != NULL) 
			DrawLandmark(trackedFace);
		if (FaceTrackingUtilities::IsModuleSelected(m_window, IDC_POSE) && trackedFace->QueryPose() != NULL)
			DrawPose(trackedFace, i);
		if (FaceTrackingUtilities::IsModuleSelected(m_window, IDC_EXPRESSIONS) && trackedFace->QueryExpressions() != NULL)
			DrawExpressions(trackedFace, i);
		if (FaceTrackingUtilities::IsModuleSelected(m_window, IDC_RECOGNITION) && trackedFace->QueryRecognition() != NULL)
			DrawRecognition(trackedFace, i);
	}
}

void FaceTrackingRenderer::DrawRecognition(PXCFaceData::Face* trackedFace, const int faceId)
{
	PXCFaceData::RecognitionData* recognitionData = trackedFace->QueryRecognition();
	if(recognitionData == NULL)
		return;

	HWND panelWindow = GetDlgItem(m_window, IDC_PANEL);
	HDC dc1 = GetDC(panelWindow);

	if (!dc1)
	{
		return;
	}
	HDC dc2 = CreateCompatibleDC(dc1);
	if(!dc2) 
	{
		ReleaseDC(panelWindow, dc1);
		return;
	}
	
	SelectObject(dc2, m_bitmap);

	BITMAP bitmap; 
	GetObject(m_bitmap, sizeof(bitmap), &bitmap);

	HFONT hFont = CreateFont(FaceTrackingUtilities::TextHeight, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"MONOSPACE");
	SelectObject(dc2, hFont);

	WCHAR line1[64];
	int recognitionID = recognitionData->QueryUserID();
	if(recognitionID != -1)
	{
		swprintf_s<sizeof(line1) / sizeof(pxcCHAR)>(line1, L"Registered ID: %d",recognitionID);
	}
	else
	{
		swprintf_s<sizeof(line1) / sizeof(pxcCHAR)>(line1, L"Not Registered");
	}
	PXCRectI32 rect;
	memset(&rect, 0, sizeof(rect));
	int yStartingPosition;
	if (trackedFace->QueryDetection())
	{
		SetBkMode(dc2, TRANSPARENT);
		trackedFace->QueryDetection()->QueryBoundingRect(&rect);
		yStartingPosition = rect.y;
	}	
	else
	{		
		const int yBasePosition = bitmap.bmHeight - FaceTrackingUtilities::TextHeight;
		yStartingPosition = yBasePosition - faceId * FaceTrackingUtilities::TextHeight;
		WCHAR userLine[64];
		swprintf_s<sizeof(userLine) / sizeof(pxcCHAR)>(userLine, L" User: %d", faceId);
		wcscat_s(line1, userLine);
	}
	SIZE textSize;
	GetTextExtentPoint32(dc2, line1, wcslen(line1), &textSize);
	int x = rect.x + rect.w + 1;
	if(x + textSize.cx > bitmap.bmWidth)
		x = rect.x - 1 - textSize.cx;

	TextOut(dc2, x, yStartingPosition, line1, wcslen(line1));

	DeleteDC(dc2);
	ReleaseDC(panelWindow, dc1);
	DeleteObject(hFont);
}

void FaceTrackingRenderer::DrawExpressions(PXCFaceData::Face* trackedFace, const int faceId)
{
	PXCFaceData::ExpressionsData* expressionsData = trackedFace->QueryExpressions();
	if (!expressionsData)
		return;

	HWND panelWindow = GetDlgItem(m_window, IDC_PANEL);
	HDC dc1 = GetDC(panelWindow);
	HDC dc2 = CreateCompatibleDC(dc1);
	if (!dc2) 
	{
		ReleaseDC(panelWindow, dc1);
		return;
	}

	SelectObject(dc2, m_bitmap);
	BITMAP bitmap; 
	GetObject(m_bitmap, sizeof(bitmap), &bitmap);

	HPEN cyan = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));

	if (!cyan)
	{
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}
	SelectObject(dc2, cyan);

	const int maxColumnDisplayedFaces = 5;
	const int widthColumnMargin = 570;
	const int rowMargin = FaceTrackingUtilities::TextHeight;
	const int yStartingPosition = faceId % maxColumnDisplayedFaces * m_expressionMap.size() * FaceTrackingUtilities::TextHeight;
	const int xStartingPosition = widthColumnMargin * (faceId / maxColumnDisplayedFaces);

	WCHAR tempLine[200];
	int yPosition = yStartingPosition;
	swprintf_s<sizeof(tempLine) / sizeof(pxcCHAR)> (tempLine, L"ID: %d", trackedFace->QueryUserID());
	TextOut(dc2, xStartingPosition, yPosition, tempLine, wcslen(tempLine));
	yPosition += rowMargin;

	for (auto expressionIter = m_expressionMap.begin(); expressionIter != m_expressionMap.end(); expressionIter++)
	{
		PXCFaceData::ExpressionsData::FaceExpressionResult expressionResult;
		if (expressionsData->QueryExpression(expressionIter->first, &expressionResult))
		{
			int intensity = expressionResult.intensity;
			std::wstring expressionName = expressionIter->second;
			swprintf_s<sizeof(tempLine) / sizeof(WCHAR)>(tempLine, L"%s = %d", expressionName.c_str(), intensity);
			TextOut(dc2, xStartingPosition, yPosition, tempLine, wcslen(tempLine));
			yPosition += rowMargin;

		}
	}

	DeleteObject(cyan);
	DeleteDC(dc2);
	ReleaseDC(panelWindow, dc1);
}

void FaceTrackingRenderer::DrawPose(PXCFaceData::Face* trackedFace, const int faceId)
{
	const PXCFaceData::PoseData* poseData = trackedFace->QueryPose();
	if (poseData == NULL) 
		return;

	PXCFaceData::PoseEulerAngles angles;
	pxcBool poseAnglesExist = poseData->QueryPoseAngles(&angles);
	if (!poseAnglesExist) 
		return;

	HWND panelWindow = GetDlgItem(m_window, IDC_PANEL);
	HDC dc1 = GetDC(panelWindow);
	HDC dc2 = CreateCompatibleDC(dc1);
	if (!dc2) 
	{
		ReleaseDC(panelWindow, dc1);
		return;
	}

	SelectObject(dc2, m_bitmap);
	BITMAP bitmap; 
	GetObject(m_bitmap, sizeof(bitmap), &bitmap);
	HPEN cyan = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));

	if (!cyan)
	{
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}

	SelectObject(dc2, cyan);

	const int maxColumnDisplayedFaces = 5;
	const int widthColumnMargin = 570;
	const int rowMargin = FaceTrackingUtilities::TextHeight;
	const int yStartingPosition = faceId % maxColumnDisplayedFaces * 5 * FaceTrackingUtilities::TextHeight; 
	const int xStartingPosition = bitmap.bmWidth - 64 - - widthColumnMargin * (faceId / maxColumnDisplayedFaces);

	WCHAR tempLine[64];
	int yPosition = yStartingPosition;

	swprintf_s<sizeof(tempLine) / sizeof(pxcCHAR)> (tempLine, L"ID: %d", trackedFace->QueryUserID());
	TextOut(dc2, xStartingPosition, yPosition, tempLine, wcslen(tempLine));

	


	yPosition += rowMargin;
	swprintf_s<sizeof(tempLine) / sizeof(WCHAR) > (tempLine, L"Yaw : %.0f", angles.yaw);
	TextOut(dc2, xStartingPosition, yPosition, tempLine, wcslen(tempLine));

	yPosition += rowMargin;
	swprintf_s<sizeof(tempLine) / sizeof(WCHAR) > (tempLine, L"Pitch: %.0f", angles.pitch);
	TextOut(dc2, xStartingPosition, yPosition, tempLine, wcslen(tempLine));

	yPosition += rowMargin;
	swprintf_s<sizeof(tempLine) / sizeof(WCHAR) > (tempLine, L"Roll : %.0f ", angles.roll);
	TextOut(dc2, xStartingPosition, yPosition, tempLine, wcslen(tempLine));

	// duo TODO use the angles to command the robotic hand begin

	WCHAR tempFacePos[12];
	tempFacePos[0] = 'Y';
	tempFacePos[3] = ':';
	tempFacePos[4] = 'R';
	tempFacePos[7] = ':';
	tempFacePos[8] = 'P';
	tempFacePos[11] = '\0';

	// yaw (head face towards left or right)
	if (angles.yaw > 15.0) {
		// strong left yaw
		tempFacePos[1] = 'S';
		tempFacePos[2] = 'L';
	}
	else if (angles.yaw > 10.0) {
		// weak left yaw
		tempFacePos[1] = 'W';
		tempFacePos[2] = 'L';
	}
	else if (angles.yaw < -20.0) {
		// strong right yaw
		tempFacePos[1] = 'S';
		tempFacePos[2] = 'R';
	}
	else if (angles.yaw < -15.0) {
		// weak right yaw
		tempFacePos[1] = 'W';
		tempFacePos[2] = 'R';
	}
	else {
		// no considerable yaw
		tempFacePos[1] = 'N';
		tempFacePos[2] = 'N';
	}

	// row (inclines head to left or right, taking ears toward shoulders)
	if (angles.roll > 15.0) {
		// strong left roll
		tempFacePos[5] = 'S';
		tempFacePos[6] = 'L';
	}
	else if (angles.roll > 10.0) {
		// weak left roll
		tempFacePos[5] = 'W';
		tempFacePos[6] = 'L';
	}
	else if (angles.roll < -20.0) {
		// strong right roll
		tempFacePos[5] = 'S';
		tempFacePos[6] = 'R';
	}
	else if (angles.roll < -15.0) {
		// weak right roll
		tempFacePos[5] = 'W';
		tempFacePos[6] = 'R';
	}
	else {
		// no considerable roll
		tempFacePos[5] = 'N';
		tempFacePos[6] = 'N';
	}

	// pitch (turn head up or down)
	if (angles.pitch > 15.0) {
		// strong upper pitch
		tempFacePos[9] = 'S';
		tempFacePos[10] = 'U';
	}
	else if (angles.pitch > 10.0) {
		// weak upper pitch
		tempFacePos[9] = 'W';
		tempFacePos[10] = 'U';
	}
	else if (angles.pitch < -16.0) {
		// strong down pitch
		tempFacePos[9] = 'S';
		tempFacePos[10] = 'D';
	}
	else if (angles.pitch < -12.0) {
		// weak down pitch
		tempFacePos[9] = 'W';
		tempFacePos[10] = 'D';
	}
	else {
		// no considerable pitch
		tempFacePos[9] = 'N';
		tempFacePos[10] = 'N';
	}

	char mbTempFacePos[256];
	wcstombs(mbTempFacePos, tempFacePos, 255);
	ofstream facePosFile(L"testeOutput.txt", ios::out);
	facePosFile << mbTempFacePos;
	facePosFile.close();

	yPosition += rowMargin;
	TextOut(dc2, xStartingPosition - 64, yPosition, tempFacePos, wcslen(tempFacePos));

	// duo TODO use the angles to command the robotic hand end


	DeleteObject(cyan);
	DeleteDC(dc2);
	ReleaseDC(panelWindow, dc1);
}

void FaceTrackingRenderer::DrawLandmark(PXCFaceData::Face* trackedFace)
{
	const PXCFaceData::LandmarksData* landmarkData = trackedFace->QueryLandmarks();
	if (landmarkData == NULL)
		return;

	HWND panelWindow = GetDlgItem(m_window, IDC_PANEL);
	HDC dc1 = GetDC(panelWindow);
	HDC dc2 = CreateCompatibleDC(dc1);

	if (!dc2) 
	{
		ReleaseDC(panelWindow, dc1);
		return;
	}

	HFONT hFont = CreateFont(8, 0, 0, 0, FW_LIGHT, 0, 0, 0, 0, 0, 0, 2, 0, L"MONOSPACE");

	if (!hFont)
	{
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}


	SetBkMode(dc2, TRANSPARENT);

	SelectObject(dc2, m_bitmap);
	SelectObject(dc2, hFont);

	BITMAP bitmap;
	GetObject(m_bitmap, sizeof(bitmap), &bitmap);

	pxcI32 numPoints = landmarkData->QueryNumPoints();
	PXCFaceData::LandmarkPoint* data = new PXCFaceData::LandmarkPoint[numPoints];
	landmarkData->QueryPoints(data);
	for (int i = 0; i < numPoints; ++i) 
	{
		int x = (int)data[i].image.x + LANDMARK_ALIGNMENT;
		int y = (int)data[i].image.y + LANDMARK_ALIGNMENT;		
		if (data[i].confidenceImage)
		{
			SetTextColor(dc2, RGB(255, 255, 255));
			TextOut(dc2, x, y, L"•", 1);
		}
		else
		{
			SetTextColor(dc2, RGB(255, 0, 0));
			TextOut(dc2, x, y, L"x", 1);
		}
	}
	delete[] data;

	DeleteObject(hFont);
	DeleteDC(dc2);
	ReleaseDC(panelWindow, dc1);
}

void FaceTrackingRenderer::DrawLocation(PXCFaceData::Face* trackedFace)
{
	const PXCFaceData::DetectionData* detectionData = trackedFace->QueryDetection();
	if (detectionData == NULL) 
		return;	

	HWND panelWindow = GetDlgItem(m_window, IDC_PANEL);
	HDC dc1 = GetDC(panelWindow);
	HDC dc2 = CreateCompatibleDC(dc1);

	if (!dc2) 
	{
		ReleaseDC(panelWindow, dc1);
		return;
	}

	SelectObject(dc2, m_bitmap);

	BITMAP bitmap;
	GetObject(m_bitmap, sizeof(bitmap), &bitmap);

	HPEN cyan = CreatePen(PS_SOLID, 3, RGB(255 ,255 , 0));

	if (!cyan)
	{
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}
	SelectObject(dc2, cyan);

	PXCRectI32 rectangle;
	pxcBool hasRect = detectionData->QueryBoundingRect(&rectangle);
	if (!hasRect)
	{
		DeleteObject(cyan);
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}

	MoveToEx(dc2, rectangle.x, rectangle.y, 0);
	LineTo(dc2, rectangle.x, rectangle.y + rectangle.h);
	LineTo(dc2, rectangle.x + rectangle.w, rectangle.y + rectangle.h);
	LineTo(dc2, rectangle.x + rectangle.w, rectangle.y);
	LineTo(dc2, rectangle.x, rectangle.y);

	WCHAR line[64];
	swprintf_s<sizeof(line)/sizeof(pxcCHAR)>(line,L"%d",trackedFace->QueryUserID());
	TextOut(dc2,rectangle.x, rectangle.y, line, wcslen(line));
	DeleteObject(cyan);

	DeleteDC(dc2);
	ReleaseDC(panelWindow, dc1);
}

void FaceTrackingRenderer::DrawFrameRate()
{
	m_frameRateCalcuator.Tick();
	if (m_frameRateCalcuator.IsFrameRateReady())
	{
		int fps = m_frameRateCalcuator.GetFrameRate();

		pxcCHAR line[1024];
		swprintf_s<1024>(line, L"Rate (%d fps)", fps);
		FaceTrackingUtilities::SetStatus(m_window, line, statusPart);
	}
}

void FaceTrackingRenderer::RefreshUserInterface()
{
	if (!m_bitmap) return;

	HWND panel = GetDlgItem(m_window, IDC_PANEL);
	RECT rc;
	GetClientRect(panel, &rc);

	HDC dc = GetDC(panel);
	if(!dc)
	{
		return;
	}

	HBITMAP bitmap = CreateCompatibleBitmap(dc, rc.right, rc.bottom);

	if(!bitmap)
	{

		ReleaseDC(panel, dc);
		return;
	}
	HDC dc2 = CreateCompatibleDC(dc);
	if (!dc2)
	{
		DeleteObject(bitmap);
		ReleaseDC(m_window,dc);
		return;
	}
	SelectObject(dc2, bitmap);
	SetStretchBltMode(dc2, COLORONCOLOR);

	/* Draw the main window */
	HDC dc3 = CreateCompatibleDC(dc);

	if (!dc3)
	{
		DeleteDC(dc2);
		DeleteObject(bitmap);
		ReleaseDC(m_window,dc);
		return;
	}

	SelectObject(dc3, m_bitmap);
	BITMAP bm;
	GetObject(m_bitmap, sizeof(BITMAP), &bm);

	bool scale = Button_GetState(GetDlgItem(m_window, IDC_SCALE)) & BST_CHECKED;
	if (scale)
	{
		RECT rc1 = GetResizeRect(rc, bm);
		StretchBlt(dc2, rc1.left, rc1.top, rc1.right, rc1.bottom, dc3, 0, 0,bm.bmWidth, bm.bmHeight, SRCCOPY);	
	} 
	else
	{
		BitBlt(dc2, 0, 0, rc.right, rc.bottom, dc3, 0, 0, SRCCOPY);
	}

	DeleteDC(dc3);
	DeleteDC(dc2);
	ReleaseDC(m_window,dc);

	HBITMAP bitmap2 = (HBITMAP)SendMessage(panel, STM_GETIMAGE, 0, 0);
	if (bitmap2) DeleteObject(bitmap2);
	SendMessage(panel, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
	InvalidateRect(panel, 0, TRUE);
	 DeleteObject(bitmap);
}

RECT FaceTrackingRenderer::GetResizeRect(RECT rectangle, BITMAP bitmap)
{
	RECT resizedRectangle;
	float sx = (float)rectangle.right / (float)bitmap.bmWidth;
	float sy = (float)rectangle.bottom / (float)bitmap.bmHeight;
	float sxy = sx < sy ? sx : sy;
	resizedRectangle.right = (int)(bitmap.bmWidth * sxy);
	resizedRectangle.left = (rectangle.right - resizedRectangle.right) / 2 + rectangle.left;
	resizedRectangle.bottom = (int)(bitmap.bmHeight * sxy);
	resizedRectangle.top = (rectangle.bottom - resizedRectangle.bottom) / 2 + rectangle.top;
	return resizedRectangle;
}

std::map<PXCFaceData::ExpressionsData::FaceExpression, std::wstring> FaceTrackingRenderer::InitExpressionsMap()
{
	std::map<PXCFaceData::ExpressionsData::FaceExpression, std::wstring> map;
	map[PXCFaceData::ExpressionsData::EXPRESSION_SMILE] =  std::wstring(L"Smile");
	map[PXCFaceData::ExpressionsData::EXPRESSION_MOUTH_OPEN] = std::wstring(L"Mouth Open");
	map[PXCFaceData::ExpressionsData::EXPRESSION_KISS] = std::wstring(L"Kiss");
	map[PXCFaceData::ExpressionsData::EXPRESSION_EYES_TURN_LEFT] = std::wstring(L"Eyes Turn Left");
	map[PXCFaceData::ExpressionsData::EXPRESSION_EYES_TURN_RIGHT] = std::wstring(L"Eyes Turn Right");
	map[PXCFaceData::ExpressionsData::EXPRESSION_EYES_UP] = std::wstring(L"Eyes Up");
	map[PXCFaceData::ExpressionsData::EXPRESSION_EYES_DOWN] = std::wstring(L"Eyes Down");
	map[PXCFaceData::ExpressionsData::EXPRESSION_BROW_RAISER_LEFT] = std::wstring(L"Brow Raised Left");
	map[PXCFaceData::ExpressionsData::EXPRESSION_BROW_RAISER_RIGHT] = std::wstring(L"Brow Raised Right");
	map[PXCFaceData::ExpressionsData::EXPRESSION_BROW_LOWERER_LEFT] = std::wstring(L"Brow Lowered Left");
	map[PXCFaceData::ExpressionsData::EXPRESSION_BROW_LOWERER_RIGHT] = std::wstring(L"Brow Lowered Right");
	map[PXCFaceData::ExpressionsData::EXPRESSION_EYES_CLOSED_LEFT] = std::wstring(L"Closed Eye Left");
	map[PXCFaceData::ExpressionsData::EXPRESSION_EYES_CLOSED_RIGHT] = std::wstring(L"Closed Eye Right");
	map[PXCFaceData::ExpressionsData::EXPRESSION_TONGUE_OUT] = std::wstring(L"Tongue Out");
	return map;
}

