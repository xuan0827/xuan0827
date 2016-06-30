// GraphBase.cpp: implementation of the CDxVideoGraphBase class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

// for direct show
#include <streams.h>
#include <mtype.h>

#include "GraphBase.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CDxVideoGraphBase: The class handle the DirectShow graph building
//////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Constructor : Init member variable and search for available devices
///////////////////////////////////////////////////////////////////////
CDxVideoGraphBase::CDxVideoGraphBase()
{
	CoInitialize(NULL);					// Init COM

	m_pCaptureGraphBuilder2	= NULL;
	m_pGraphBuilder			= NULL;
	m_pFilterGraph			= NULL;
	m_pMediaControl			= NULL;
	m_pVideoWindow			= NULL;
	m_pMediaSeeking			= NULL;
	m_pSrcFilter			= NULL;
	m_pRenderFilter			= NULL;

    m_hRenderWnd			= NULL;
	m_pCaptureSetting		= NULL;
	m_fIsScaling			= FALSE;	// no scaling
    
	m_fRegisterFilterGraph	= TRUE;		// register for GraphEdt to view
	m_dwGraphRegister		= 0;		// the register no. of the graph

	m_wcMediaName[0]		= '\0';
	m_wcOutputFile[0]		= '\0';

	// enumerate all available video capture devices
    m_nNumOfCapDev = 0;

	HRESULT hr;
	CComPtr<ICreateDevEnum> pCreateDevEnum;
	CComPtr<IEnumMoniker> pEnumMoniker = NULL;

	hr = CoCreateInstance(CLSID_SystemDeviceEnum,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_ICreateDevEnum,
							(void**)&pCreateDevEnum);

	// no capture device enumerator
	if(FAILED(hr))
		return;

	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);

	// no capture device
	if(FAILED(hr) || !pEnumMoniker)
		return;

	// Search all available devices and save in m_pCapMonikers[]
	ULONG cFetched;

	// Note that if the Next() call succeeds but there are no monikers,
	// it will return S_FALSE (which is not a failure).  Therefore, we
	// check that the return code is S_OK instead of using SUCCEEDED()
    while(S_OK == pEnumMoniker->Next(1, m_pCapMonikers + m_nNumOfCapDev, &cFetched))
		m_nNumOfCapDev++;
}

CDxVideoGraphBase::~CDxVideoGraphBase()
{
	// Release the stored available capture device
	for(int i=0;i<m_nNumOfCapDev;i++)
		SafeRelease(m_pCapMonikers[i]);

	ReleaseFilterGraph();			// destroy the filter graph
	CoUninitialize();				// release COM
}

HRESULT CDxVideoGraphBase::SetRenderWindow(HWND hWnd, RECT *pImgRect)
{
	m_hRenderWnd = hWnd;		// set the render window
	if(pImgRect == NULL)		// displaye video in original size at up-left corner.
	{
		m_fIsScaling = FALSE;
		m_oVideoRect.left = m_oVideoRect.top = 0;
	}
    else
	{							// Set the rect to display the video and mark the flag
		m_oVideoRect = *pImgRect;
		m_fIsScaling = TRUE;
	}

    // update the video window size
	return setVideoWindowSize();
}

HRESULT CDxVideoGraphBase::SetRenderWindow(HWND hWnd, int x, int y) 
{
	m_hRenderWnd = hWnd;		// set the render window
	m_fIsScaling = FALSE;		// no scaling
	m_oVideoRect.left = x;
	m_oVideoRect.top = y;

	// update the video window size
	return setVideoWindowSize();
}

// ReleaseFilterGraph : Release the whole graph
void CDxVideoGraphBase::ReleaseFilterGraph()
{
	// Destroy the whole filter graph
	stopGraph();
	SafeRelease(m_pMediaControl);
	SafeRelease(m_pMediaSeeking);
	SafeRelease(m_pSrcFilter);
	SafeRelease(m_pRenderFilter);
	SafeRelease(m_pVideoWindow);
	SafeRelease(m_pFilterGraph);
	SafeRelease(m_pGraphBuilder);
	SafeRelease(m_pCaptureGraphBuilder2);
}

HRESULT CDxVideoGraphBase::RenderFile(const TCHAR *vname)
{
	// set the VideoSource to indicate we are play video file
	m_nVideoSource = VideoFilePlayback;

	// change the given file name to wide character in m_wcMediaName
	USES_CONVERSION;
	TCHAR tempVName[MAX_PATH];
	_tcscpy(tempVName, vname);
	wcscpy(m_wcMediaName, T2W(tempVName));

	HRESULT hr;
	// Create the essensial element of a video graph
	JIF(createFilterGraph());

	// Create seeking control to navigate through the video file
	// May fail for live camera. (but no need to return even we fail)
	hr = m_pGraphBuilder->QueryInterface(IID_IMediaSeeking, (void**)&m_pMediaSeeking);
	if(SUCCEEDED(hr))
		m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_FRAME);
	else
		m_pMediaSeeking = NULL;

	// Start the graph (start playing)
	JIF(startGraph());
	return hr;
}

// GetNumCapDev : Return the total number of available capture devices
int CDxVideoGraphBase::GetNumCapDev()
{
	return m_nNumOfCapDev;
}

bool CDxVideoGraphBase::GetCapDevName(int i, TCHAR *pDevStrName, TCHAR *pDevDispName)
{
	if(pDevStrName)
		pDevStrName[0] = '\0';

	if(pDevDispName)
		pDevDispName[0] = '\0';

	HRESULT hr;
	if(i < m_nNumOfCapDev)
	{
		IMoniker *pM = m_pCapMonikers[i];
		// search through the capture devices
		// Get friendly name (for user to see it)
		if(pDevStrName)
		{
			IPropertyBag *pBag;
			hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
			if(SUCCEEDED(hr))
			{
				VARIANT var;
				var.vt = VT_BSTR;
				hr = pBag->Read(L"FriendlyName", &var, NULL);
				if(hr == NOERROR)
				{
					USES_CONVERSION;
					_tcscpy(pDevStrName, OLE2T(var.bstrVal));
					SysFreeString(var.bstrVal);
				}
				pBag->Release();
			}
		}

		// Get Device Name (machine will understand this string)
		if(pDevDispName)
		{
			WCHAR *szDispName;
			if(SUCCEEDED(pM->GetDisplayName(0, 0, &szDispName)))
			{
				wsprintf(pDevDispName, TEXT("%S"), szDispName?szDispName:L"");
				CoTaskMemFree(szDispName);
			}
		}

		return(TRUE);
	}
	else
		return(FALSE);
}

HRESULT CDxVideoGraphBase::PreviewLive(int capInd, bool fSetup)
{
    // check if the specified capture device exists
    if ( capInd >= m_nNumOfCapDev  || capInd < 0 ) return E_FAIL;
    // set VideoSource so that createFilterGraph() knows what to build
    m_nVideoSource = (enum VideoSource)capInd;
    m_fShowCaptureProperties = fSetup;

    HRESULT hr;
    // Build graph according to the setting: e.g VideoSource ...
    JIF(createFilterGraph());
    // Start the graph
    JIF(startGraph());
    return hr;
}

HRESULT CDxVideoGraphBase::PreviewLive(TCHAR* capName, bool fSetup)
{
    int ind = 0;
    // if capName is not empty, find corresponding capture device
    // Otherwise use the first available capture device (ind == 0).
    if (_tcsclen(capName) > 0) {
        // change to WCHAR
        WCHAR wcCapName[1024];
		USES_CONVERSION;
		TCHAR tempVName[MAX_PATH];
		_tcscpy(tempVName, capName);
		wcscpy(wcCapName, T2W(tempVName));
        // Create IMoniker from DisplayName
        IBindCtx *lpBC;
        HRESULT hr = CreateBindCtx(0, &lpBC);
        IMoniker *pmVideo = 0;
        if (SUCCEEDED(hr))
        {
            DWORD dwEaten;
            hr = MkParseDisplayName(lpBC, wcCapName, &dwEaten,
                                    &pmVideo);
            lpBC->Release();
        }
        else return E_FAIL;  // not a valid capture device name

        // Find the IMoniker from the available capture devices
        for (ind = 0; ind < m_nNumOfCapDev; ind++)
            if (S_OK == m_pCapMonikers[ind]->IsEqual(pmVideo)) break;
        SafeRelease(pmVideo);
    }

    // Build capture graph using this capture device
    return PreviewLive(ind, fSetup);
}

///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::CaptureToFile(const TCHAR *vname)
{
    // change the given file name to wide character in m_wcOutputFile
    if( vname && _tcsclen(vname) > 0 ) {
		// change the given file name to wide character in m_wcMediaName
		USES_CONVERSION;
		TCHAR tempVName[MAX_PATH];
		_tcscpy(tempVName, vname);
		wcscpy(m_wcOutputFile, T2W(tempVName));
    }

    HRESULT hr;
    // Build graph according to the setting: e.g VideoSource ...
    JIF(createFilterGraph());
    
    // Start the graph
    JIF(startGraph());

    m_wcOutputFile[0] = '\0';
    return hr;
}

///////////////////////////////////////////////////////////////////////
// StartCapture: Begin capture to file
///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::StartCapture()
{
    // Set the start time to 0 and stop time to MAX_TIME
    REFERENCE_TIME stop = MAX_TIME;
    HRESULT hr = m_pCaptureGraphBuilder2->ControlStream(&PIN_CATEGORY_CAPTURE, NULL,
                NULL, NULL, &stop, 0, 0);
    return hr;
}

///////////////////////////////////////////////////////////////////////
// StopCapture: Stop capturing to file
///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::StopCapture()
{
    // Set the start time to MAX_TIME to prevent capture
    REFERENCE_TIME start = MAX_TIME;
    HRESULT hr = m_pCaptureGraphBuilder2->ControlStream(&PIN_CATEGORY_CAPTURE, NULL,
                NULL, &start, NULL, 0, 0);
    return hr;
}


///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::AdjustCamera()
{
    // Setup the camera by property page
    HRESULT hr = S_FALSE;  // Treated as success.
    // If the current graph is for capturing.
    if (m_nVideoSource >= 0 && m_pSrcFilter) {
        // display the property page of source filter
        JIF(showPropertyPage(m_pSrcFilter, L"Adjust Camera"));
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////
void CDxVideoGraphBase::Play()
{
	// Begin the palying
	if(m_pMediaControl)
		m_pMediaControl->Run();
}

void CDxVideoGraphBase::Pause()
{
	// Pause the video
	if(m_pMediaControl)
	{
		m_pMediaControl->Pause();
		//m_pMediaControl->StopWhenReady();
	}
}

void CDxVideoGraphBase::Stop()
{
	// Stop the video
	if(m_pMediaControl)
	{
		m_pMediaControl->Stop();
		m_pMediaControl->StopWhenReady();
	}
}

LONGLONG CDxVideoGraphBase::Seek(int offset)
{
    LONGLONG cur_pos = -1;
    OAFilterState  fs;
    if (m_pMediaSeeking) {
        // Wait for the other seeking operation to finish.
        m_pMediaControl->GetState(INFINITE, &fs);
        // Seek with respect to current position.
        m_pMediaSeeking->GetCurrentPosition(&cur_pos);
        cur_pos += offset;
        m_pMediaSeeking->SetPositions(&cur_pos, 
                                      AM_SEEKING_AbsolutePositioning,
                                      NULL, AM_SEEKING_NoPositioning);
        // wait until the seeking is finished
        m_pMediaControl->GetState(INFINITE, &fs);
        // display the new frame
        m_pMediaControl->StopWhenReady();
    }
    return cur_pos;
};

///////////////////////////////////////////////////////////////////////
LONGLONG CDxVideoGraphBase::SeekToStart()
{
    // Goto the start of the video file
    LONGLONG cur_pos = 0;
    OAFilterState  fs;
    if (m_pMediaSeeking) {
        // Wait for the other seeking operation finish.
        m_pMediaControl->GetState(INFINITE, &fs);
        // Seek to start
        m_pMediaSeeking->SetPositions(&cur_pos, 
                                      AM_SEEKING_AbsolutePositioning,
                                      NULL, AM_SEEKING_NoPositioning);
        // this function will wait until the seeking is finished
        m_pMediaControl->GetState(INFINITE, &fs);
        // display the new frame
        m_pMediaControl->StopWhenReady();
    }
    return cur_pos;
}

///////////////////////////////////////////////////////////////////////
LONGLONG CDxVideoGraphBase::SeekToEnd()
{
    // Goto the end of the video file
    LONGLONG cur_pos = 0;
    OAFilterState  fs;
    if (m_pMediaSeeking) {
        // Wait for the other seeking operation finish.
        m_pMediaControl->GetState(INFINITE, &fs);
        // Seek to the last frame
        m_pMediaSeeking->GetDuration(&cur_pos);
        m_pMediaSeeking->SetPositions(&cur_pos, 
                                      AM_SEEKING_AbsolutePositioning,
                                      NULL, AM_SEEKING_NoPositioning);
        // this function will wait until the seeking is finished
        m_pMediaControl->GetState(INFINITE, &fs);
        // display the new frame
        m_pMediaControl->StopWhenReady();
    }
    return cur_pos;
}


///////////////////////////////////////////////////////////////////////
CDxVideoGraphBase::VideoSource CDxVideoGraphBase::GetVideoSource(TCHAR *vn)
{
    if (vn) {
        if (m_nVideoSource >= 0 )
            GetCapDevName(m_nVideoSource, vn);
        else if( m_nVideoSource < 0 ) {
			USES_CONVERSION;
			_tcscpy(vn, W2T(m_wcMediaName));
        }
    }
    return m_nVideoSource;
}

///////////////////////////////////////////////////////////////////////
SIZE CDxVideoGraphBase::GetVideoSize()
{
    // Get the size of the video through the IBasicVideo interface.
    SIZE    s = {0, 0};
    HRESULT hr = E_FAIL;
    IBasicVideo *pBasicVideo = NULL; 
    if (m_pGraphBuilder) 
        hr = m_pGraphBuilder->QueryInterface(IID_IBasicVideo, (void **)&pBasicVideo);
    if (SUCCEEDED(hr)) {
        pBasicVideo->get_SourceWidth(&(s.cx));
        pBasicVideo->get_SourceHeight(&(s.cy));
        SafeRelease(pBasicVideo);
    }
    return s;
}


///////////////////////////////////////////////////////////////////////
double CDxVideoGraphBase::GetAvgFrameRate()
{
    int frameRate = 0;
    HRESULT hr = E_FAIL;
    CComPtr<IQualProp> pQualProp = NULL;
    if (m_pRenderFilter)
        hr = m_pRenderFilter->QueryInterface(IID_IQualProp, (void **)&pQualProp);
    if (SUCCEEDED(hr))
        hr = pQualProp->get_AvgFrameRate(&frameRate);

    return double(frameRate/100.);
}

///////////////////////////////////////////////////////////////////////
LONGLONG CDxVideoGraphBase::GetFrameNumber()
{
    // Get current frame number
    LONGLONG cur_pos = -1;
    // return the current position in video file in frames number
    // (we set the unit to frame number format)
    if (m_pMediaSeeking)
        m_pMediaSeeking->GetCurrentPosition(&cur_pos);
    return cur_pos;
}

///////////////////////////////////////////////////////////////////////
bool CDxVideoGraphBase::IsGraphRunning()
{
    // Is the graph in running mode
    OAFilterState  fs;
    if (m_pMediaControl) {
        m_pMediaControl->GetState(INFINITE, &fs);
        if (fs == State_Running) return true;
    }
    return false;
}


///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::createFilterGraph()
{
    HRESULT hr;
    // First destroy and release all the old graph
    ReleaseFilterGraph();
    // Then, create the essential interface for building filter graph
    // IGraphBuilder, ICaptureGraphBuilder2, IFilterGraph, 
    // IMediaControl and IVideoWindow
    JIF(CoCreateInstance( CLSID_FilterGraph, NULL, CLSCTX_INPROC, 
                      IID_IGraphBuilder, (void **)&m_pGraphBuilder ));
    JIF(CoCreateInstance (CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
        IID_ICaptureGraphBuilder2, (void **) &m_pCaptureGraphBuilder2));
    JIF(m_pCaptureGraphBuilder2->SetFiltergraph(m_pGraphBuilder));

    JIF(m_pGraphBuilder->QueryInterface(IID_IFilterGraph,(void**)&m_pFilterGraph));
    JIF(m_pGraphBuilder->QueryInterface(IID_IMediaControl,(void**)&m_pMediaControl));
    JIF(m_pGraphBuilder->QueryInterface(IID_IVideoWindow, (void**)&m_pVideoWindow));
    
    // Create essential component and connect the graph
    // 1. Source Filter and get its output pin for further processing
    JIF(addSrcFilter(&m_pSrcFilter));
    CComPtr< IPin > pSrcOut = NULL, pProcessedOut = NULL;
    JIF(get_pin(m_pSrcFilter, PINDIR_OUTPUT, 0, &pSrcOut));
    // 2. Extra processing filters for the output pin
    JIF(addExtraFilters(pSrcOut, &pProcessedOut));
    // 3. Render the output pin of output of Extra Processing
    JIF(addRenderFilters(pProcessedOut));

    return hr; // success build the graph
}


///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::startGraph() 
{
    // Start the graph to render the video
    HRESULT hr = E_FAIL;
    if( m_pMediaControl && m_pVideoWindow )
    {
        // set render window to IVideoWindow. 
        // If m_hRenderWnd == NULL, video will be displayed in new win
        if (m_hRenderWnd) {
            JIF(m_pVideoWindow->put_Owner((OAHWND)m_hRenderWnd));
            JIF(m_pVideoWindow->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS));
            JIF(m_pVideoWindow->put_MessageDrain((OAHWND)m_hRenderWnd));
            // Set the display position
            JIF(setVideoWindowSize());
        }
        // If we want the filter graph accessible by GraphEdt.exe
        if (m_fRegisterFilterGraph) {
            // Add the graph to Rot so that the graphedit can view it
            hr = addGraphToRot(m_pGraphBuilder, &m_dwGraphRegister);
            if (FAILED(hr)) m_dwGraphRegister = 0;
        }
        // Start the graph
        JIF(m_pMediaControl->Run());
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////
void CDxVideoGraphBase::stopGraph() 
{
    // Stop the graph
    if( m_pMediaControl && m_pVideoWindow )
    {
        m_pMediaControl->Stop();
        m_pVideoWindow->put_Visible(OAFALSE);
        m_pVideoWindow->put_Owner(NULL);
        m_pVideoWindow->put_MessageDrain(0);
    }
    if (m_fRegisterFilterGraph) {
        if (m_dwGraphRegister)
            removeGraphFromRot(m_dwGraphRegister);
    }
}


///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::setVideoWindowSize()
{
    // if no graph exist, return S_FALSE which is treated as success
    HRESULT hr = S_FALSE; 
    // If no scaling is set, set display rectangle to video size
    if (m_pGraphBuilder && !m_fIsScaling) { // display at original size
        SIZE s = GetVideoSize();
        // Store the dimension of the image into the m_oVideoRect
        m_oVideoRect.right = m_oVideoRect.left + s.cx; 
        m_oVideoRect.bottom = m_oVideoRect.top + s.cy;
    }
    // Set the video window position
    if (m_pVideoWindow) {
        JIF(m_pVideoWindow->SetWindowPosition( m_oVideoRect.left, 
                                m_oVideoRect.top, 
                                m_oVideoRect.right - m_oVideoRect.left, 
                                m_oVideoRect.bottom - m_oVideoRect.top ));
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::addSrcFilter(IBaseFilter **ppSrc)
{
    // create the source filter
    HRESULT hr;
    if (m_nVideoSource >= 0) { // create the capture filter
        // Bind the specified capture Moniker to a filter object
        JIF(m_pCapMonikers[m_nVideoSource]->BindToObject(0,0,IID_IBaseFilter,
                                                     (void**)ppSrc));
        JIF(setupCapture(*ppSrc)); // setup the capture
    }
    else if (m_nVideoSource == -1) { // video file source filter
        JIF(m_pGraphBuilder->AddSourceFilter(m_wcMediaName, 
                                             L"Video File", ppSrc));
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::addExtraFilters(IPin * pIn, IPin ** ppOut) 
{
    // no extra filters is needed in this class
    // if you need extra filters, override this function
    *ppOut = pIn;
    // remember to increase the ref count after assign pointer
    // Otherwise, it will release pIn twice and cause memory problem.
    pIn->AddRef();
    return S_OK;
}


///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::addRenderFilters(IPin *pIn)
{
    HRESULT hr;
    // render the pPinIn (Preview Pin) with video renderer
    // we need m_pRenderFilter to get achieved frame rate. Otherwise,
    // just call m_pGraphBuilder->Render(*ppPinIn) is ok.
    JIF(CoCreateInstance(CLSID_VideoRenderer , NULL, CLSCTX_INPROC_SERVER, 
                 IID_IBaseFilter, (void **)&m_pRenderFilter));
    JIF(m_pFilterGraph->AddFilter(m_pRenderFilter, L"Video Renderer"));

    // If output file is not set, we just use video renderer.
    if (m_wcOutputFile[0] == '\0') {
        return (m_pGraphBuilder->Render(pIn));
    }

    // Otherwise, we need to split input to 2 (preview & capture)
    // Use Smart Tee to split the input pin to Preview & Capture Pins
    // Different from Inf Pin Tee, Smart Tee support independent 
    // control of capture pin and preview pin so that we can start 
    // preivew and capture independently. Inf Pin Tee does not.
    CComPtr <IPin>            pTmpPin   = NULL;
    CComPtr <IBaseFilter>     pf        = NULL;
    CComPtr <IFileSinkFilter> pSink     = NULL;
    CComPtr <IBaseFilter>     pSmartTee = NULL;
    JIF(CoCreateInstance(CLSID_SmartTee , NULL, CLSCTX_INPROC_SERVER, 
                         IID_IBaseFilter, (void **)&pSmartTee));
    JIF(m_pFilterGraph->AddFilter(pSmartTee, L"Smart Tee"));
    JIF(get_pin(pSmartTee, PINDIR_INPUT, 0, &pTmpPin));
    JIF(m_pGraphBuilder->Connect(pIn, pTmpPin));

    // Preview branch
    pTmpPin = NULL;
    JIF(get_pin(pSmartTee, PINDIR_OUTPUT, 1, &pTmpPin));// Preview Pin
    JIF(m_pGraphBuilder->Render(pTmpPin)); //use added m_pRenderFilter
    
    // Capture branch
    pTmpPin = NULL;
    JIF(get_pin(pSmartTee, PINDIR_OUTPUT, 0, &pTmpPin));// Capture pin
    // Render the capture pin with a file writer to save video
    JIF(m_pCaptureGraphBuilder2->SetOutputFileName(&MEDIASUBTYPE_Avi, 
                                     m_wcOutputFile, &pf, &pSink));
    JIF(m_pCaptureGraphBuilder2->RenderStream(NULL, NULL, pTmpPin, NULL, pf));

    return hr;
}

///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::setupCapture(IBaseFilter * pSrcFilter)
{
    HRESULT hr;
    // Add Capture filter and init ICaptureGraphBuilder2 interface
    // we may need it if want to capture video to files
    JIF(m_pGraphBuilder->AddFilter(pSrcFilter, L"Video Capture"));

    // Now decide the capturing config by the property page or set a 
    // default one. (i.e. Frame rate, resolution.....)
    CComPtr<IAMStreamConfig> pSC;  // Media Stream config interface
    // check capture device capabilities
    JIF(m_pCaptureGraphBuilder2->FindInterface(&PIN_CATEGORY_CAPTURE,
        &MEDIATYPE_Video, pSrcFilter, IID_IAMStreamConfig, (void **)&pSC));

    AM_MEDIA_TYPE *pmt;
    int iCount, iSize, ind;
    if (!m_fShowCaptureProperties) { // default capture setup
        // default capture: 320 * 240 resolution, 15f/s
        VIDEO_STREAM_CONFIG_CAPS caps;
        pSC->GetNumberOfCapabilities(&iCount, &iSize);
        for (ind = 0; ind < iCount; ind++) {
            pSC->GetStreamCaps(ind, &pmt, (BYTE *)&caps);
			//320...320x240
            if (caps.InputSize.cx == 640) {
                // Set the capturing frame rate
                VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
                pvi->AvgTimePerFrame = (LONGLONG)(10000000 / 15.);
                JIF(pSC->SetFormat(pmt));
                DeleteMediaType(pmt);
                break; // found default setup, quit the loop
            }
            DeleteMediaType(pmt);
        }
    }
    // if m_fShowCaptureProperties == 1
    // set capture format by property page
    if (m_fShowCaptureProperties) { 
        showPropertyPage(pSC, L"Setup the capture");
    }
    return hr;
}


///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::showPropertyPage(IUnknown* pIU, const WCHAR* name)
{
    HRESULT hr;
    if (pIU) {
        ISpecifyPropertyPages* pispp = 0;
        JIF(pIU->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pispp));

        CAUUID caGUID;
        if (SUCCEEDED(pispp->GetPages(&caGUID))) {
            OleCreatePropertyFrame(m_hRenderWnd, 0, 0,
                name,     // Caption for the dialog box
                1,        // Number of filters
                (IUnknown**)&pIU,
                caGUID.cElems,
                caGUID.pElems,
                0, 0, 0);
            // Release the memory
            CoTaskMemFree(caGUID.pElems);
        }
        SafeRelease(pispp);
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::get_pin(IBaseFilter * pFilter, PIN_DIRECTION dir, int n, IPin **ppPin)
{
    CComPtr< IEnumPins > pEnum;
    *ppPin = NULL;
    HRESULT hr;
    JIF(pFilter->EnumPins(&pEnum));

    ULONG ulFound;
    IPin *pPin;
    hr = E_FAIL;
    while(S_OK == pEnum->Next(1, &pPin, &ulFound))
    {
        PIN_DIRECTION pindir = (PIN_DIRECTION)3;
        pPin->QueryDirection(&pindir);
        if(pindir == dir) {
            if(n == 0) {
                hr = S_OK, *ppPin = pPin;
                break;
            }
            n--;
        } // if
        pPin->Release();
    } // while

    return hr;
}

///////////////////////////////////////////////////////////////////////
HRESULT CDxVideoGraphBase::addGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
{
    CComPtr<IMoniker> pMoniker;
    CComPtr<IRunningObjectTable> pROT;
    WCHAR wsz[128];
    HRESULT hr;

    JIF(GetRunningObjectTable(0, &pROT));

    swprintf(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph, 
              GetCurrentProcessId());

    hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) {
        hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////
void CDxVideoGraphBase::removeGraphFromRot(DWORD dwRegister)
{
    CComPtr<IRunningObjectTable> pROT;
    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
        pROT->Revoke(dwRegister);
    }
}

///////////////////////////////////////////////////////////////////////
void CDxVideoGraphBase::errorHandler(TCHAR *szFormat, ...)
{
    // Get the aregument of the function
    TCHAR szBuffer[512];
    va_list pArgs;
    va_start(pArgs, szFormat);
    _stprintf(szBuffer, szFormat, pArgs);
    va_end(pArgs);
    // Display the error message
    MessageBox(NULL, szBuffer, _T("DirectShow Error Message"), MB_OK | MB_ICONERROR);
}
