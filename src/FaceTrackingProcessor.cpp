#include "FaceTrackingProcessor.h"
#include <assert.h>
#include <string>
#include "pxcfaceconfiguration.h"
#include "pxcsensemanager.h"
#include "FaceTrackingUtilities.h"
#include "FaceTrackingAlertHandler.h"
#include "FaceTrackingRenderer.h"
#include "resource.h"

extern PXCSession* session;
extern FaceTrackingRenderer* renderer;

extern volatile bool isStopped;
extern pxcCHAR fileName[1024];

FaceTrackingProcessor::FaceTrackingProcessor(HWND window) : m_window(window), m_registerFlag(false), m_unregisterFlag(false) { }

void FaceTrackingProcessor::PerformRegistration()
{
	m_registerFlag = false;
	if(m_output->QueryFaceByIndex(0))
		m_output->QueryFaceByIndex(0)->QueryRecognition()->RegisterUser();
}

void FaceTrackingProcessor::PerformUnregistration()
{
	m_unregisterFlag = false;
	if(m_output->QueryFaceByIndex(0))
		m_output->QueryFaceByIndex(0)->QueryRecognition()->UnregisterUser();
}

void FaceTrackingProcessor::CheckForDepthStream(PXCSenseManager* pp, HWND hwndDlg)
{
	PXCFaceModule* faceModule = pp->QueryFace();
	if (faceModule == NULL) 
	{
		assert(faceModule);
		return;
	}
	PXCFaceConfiguration* config = faceModule->CreateActiveConfiguration();
	if (config == NULL)
	{
		assert(config);
		return;
	}

	PXCFaceConfiguration::TrackingModeType trackingMode = config->GetTrackingMode();
	config->Release();
	if (trackingMode == PXCFaceConfiguration::FACE_MODE_COLOR_PLUS_DEPTH)
	{
		PXCCapture::Device::StreamProfileSet profiles={};
		pp->QueryCaptureManager()->QueryDevice()->QueryStreamProfileSet(&profiles);
		if (!profiles.depth.imageInfo.format)
		{            
			std::wstring msg = L"Depth stream is not supported for device: ";
			msg.append(FaceTrackingUtilities::GetCheckedDevice(hwndDlg));           
			msg.append(L". \nUsing 2D tracking");
			MessageBox(hwndDlg, msg.c_str(), L"Face Tracking", MB_OK);            
		}
	}
}

void FaceTrackingProcessor::Process(HWND dialogWindow)
{
	PXCSenseManager* senseManager = session->CreateSenseManager();
	if (senseManager == NULL) 
	{
		FaceTrackingUtilities::SetStatus(dialogWindow, L"Failed to create an SDK SenseManager", statusPart);
		return;
	}

	/* Set Mode & Source */
	PXCCaptureManager* captureManager = senseManager->QueryCaptureManager();
	pxcCHAR* device = NULL;

	pxcStatus status = PXC_STATUS_NO_ERROR;
	if (FaceTrackingUtilities::GetRecordState(dialogWindow)) 
	{
		status = captureManager->SetFileName(fileName, true);
		captureManager->FilterByDeviceInfo(FaceTrackingUtilities::GetCheckedDevice(dialogWindow), 0, 0);
		device = FaceTrackingUtilities::GetCheckedDevice(dialogWindow);
	} 
	else if (FaceTrackingUtilities::GetPlaybackState(dialogWindow)) 
	{
		status = captureManager->SetFileName(fileName, false);
		senseManager->QueryCaptureManager()->SetRealtime(false);
	} 
	else 
	{
		device = FaceTrackingUtilities::GetCheckedDevice(dialogWindow);
		captureManager->FilterByDeviceInfo(device, 0, 0);
	}

	if (status < PXC_STATUS_NO_ERROR) 
	{
		FaceTrackingUtilities::SetStatus(dialogWindow, L"Failed to Set Record/Playback File", statusPart);
		return;
	}

	/* Set Module */
	senseManager->EnableFace();

	/* Initialize */
	FaceTrackingUtilities::SetStatus(dialogWindow, L"Init Started", statusPart);

	PXCFaceModule* faceModule = senseManager->QueryFace();
	if (faceModule == NULL)
	{
		assert(faceModule);
		return;
	}
	PXCFaceConfiguration* config = faceModule->CreateActiveConfiguration();
	if (config == NULL)
	{
		assert(config);
		return;
	}
	config->SetTrackingMode(FaceTrackingUtilities::GetCheckedProfile(dialogWindow));
	config->ApplyChanges();

	// Try default resolution first
	PXCCapture::Device::StreamProfileSet set;
	memset(&set, 0, sizeof(set));
	PXCRangeF32 frameRate = {0, 0};
	set.color.frameRate = frameRate;
	set.color.imageInfo.height = 360;
	set.color.imageInfo.width = 640;
	set.color.imageInfo.format = PXCImage::PIXEL_FORMAT_YUY2;
	captureManager->FilterByStreamProfiles(&set);

	if (senseManager->Init() < PXC_STATUS_NO_ERROR)
	{
		captureManager->FilterByStreamProfiles(NULL);
		if (senseManager->Init() < PXC_STATUS_NO_ERROR)
		{
			FaceTrackingUtilities::SetStatus(dialogWindow, L"Init Failed", statusPart);
			return;
		}
	}
	PXCCapture::DeviceInfo deviceInfo;
	PXCCapture::DeviceModel model = PXCCapture::DEVICE_MODEL_IVCAM;
	if (captureManager->QueryDevice() != NULL)
	{
		captureManager->QueryDevice()->QueryDeviceInfo(&deviceInfo);
		model = deviceInfo.model;
	}

	pxcI16 threshold = captureManager->QueryDevice()->QueryDepthConfidenceThreshold();
	pxcI32 filter_option =  captureManager->QueryDevice()->QueryIVCAMFilterOption();
	pxcI32 range_tradeoff = captureManager->QueryDevice()->QueryIVCAMMotionRangeTradeOff();
	if (model == PXCCapture::DEVICE_MODEL_IVCAM)
	{
		captureManager->QueryDevice()->SetDepthConfidenceThreshold(1);
		captureManager->QueryDevice()->SetIVCAMFilterOption(6);
		captureManager->QueryDevice()->SetIVCAMMotionRangeTradeOff(21);
		//pxcStatus r0 = captureManager->QueryDevice()->SetColorAutoExposure(false);
		//pxcStatus r = captureManager->QueryDevice()->SetColorAutoExposure(true);
		//pxcStatus r1 = captureManager->QueryDevice()->SetColorBrightness(1000);
		//pxcStatus r1 = captureManager->QueryDevice()->SetColorExposure(-5);

		if(FaceTrackingUtilities::GetCheckedModule(dialogWindow))
		{
			bool mirror = FaceTrackingUtilities::IsModuleSelected(dialogWindow, IDC_MIRROR);
			if(mirror)
			{
				captureManager->QueryDevice()->SetMirrorMode(PXCCapture::Device::MIRROR_MODE_HORIZONTAL);
			}
			else
			{

				captureManager->QueryDevice()->SetMirrorMode(PXCCapture::Device::MIRROR_MODE_DISABLED);
			}
		}
	}

		CheckForDepthStream(senseManager, dialogWindow);
		FaceTrackingAlertHandler alertHandler(dialogWindow);
		if (FaceTrackingUtilities::GetCheckedModule(dialogWindow))
		{
			config->detection.isEnabled = FaceTrackingUtilities::IsModuleSelected(dialogWindow, IDC_LOCATION);
			config->landmarks.isEnabled = FaceTrackingUtilities::IsModuleSelected(dialogWindow, IDC_LANDMARK);
			config->pose.isEnabled = FaceTrackingUtilities::IsModuleSelected(dialogWindow, IDC_POSE);
			if (FaceTrackingUtilities::IsModuleSelected(dialogWindow, IDC_EXPRESSIONS))
			{
				config->QueryExpressions()->Enable();
				config->QueryExpressions()->EnableAllExpressions();
			}
			else
			{
				config->QueryExpressions()->DisableAllExpressions();
				config->QueryExpressions()->Disable();
			}
			if (FaceTrackingUtilities::IsModuleSelected(dialogWindow, IDC_RECOGNITION))
			{
				config->QueryRecognition()->Enable();
			}

			config->QueryExpressions()->properties.maxTrackedFaces = 2; //TODO: add to GUI

			config->EnableAllAlerts();
			config->SubscribeAlert(&alertHandler);
			config->ApplyChanges();
		}
		FaceTrackingUtilities::SetStatus(dialogWindow, L"Streaming", statusPart);
		m_output = faceModule->CreateOutput();

		bool isNotFirstFrame = false;
		bool isFinishedPlaying = false;
		ResetEvent(FaceTrackingUtilities::GetRenderingFinishedSignal());

		renderer->SetCallback(FaceTrackingUtilities::SignalProcessor);

		if (!isStopped)
		{
			while (true)
			{
				if (senseManager->AcquireFrame(true) < PXC_STATUS_NO_ERROR)
				{
					isFinishedPlaying = true;
				}

				if (isNotFirstFrame)
				{
					WaitForSingleObject(FaceTrackingUtilities::GetRenderingFinishedSignal(), INFINITE);
				}

				if (isFinishedPlaying || isStopped)
				{
					if (isStopped)
						senseManager->ReleaseFrame();

					if (isFinishedPlaying)
						PostMessage(dialogWindow, WM_COMMAND, ID_STOP, 0);

					break;
				}

				m_output->Update();
				PXCCapture::Sample* sample = senseManager->QueryFaceSample();

				isNotFirstFrame = true;

				if (sample != NULL)
				{
					renderer->DrawBitmap(sample->color);
					renderer->SetOutput(m_output);
					renderer->SignalRenderer();
				}
				
				if (config->QueryRecognition()->properties.isEnabled)
				{
					if (m_registerFlag)
						PerformRegistration();

					if (m_unregisterFlag)
						PerformUnregistration();
				}

				senseManager->ReleaseFrame();
			}

			m_output->Release();
			FaceTrackingUtilities::SetStatus(dialogWindow, L"Stopped", statusPart);
		}
	
	if (model == PXCCapture::DEVICE_MODEL_IVCAM)
	{
		captureManager->QueryDevice()->SetDepthConfidenceThreshold(threshold);
		captureManager->QueryDevice()->SetIVCAMFilterOption(filter_option);
		captureManager->QueryDevice()->SetIVCAMMotionRangeTradeOff(range_tradeoff);
	}

	config->Release();
	senseManager->Close(); 
	senseManager->Release();
}

void FaceTrackingProcessor::RegisterUser()
{
	m_registerFlag = true;
}

void FaceTrackingProcessor::UnregisterUser()
{
	m_unregisterFlag = true;
}


