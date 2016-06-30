// GraphBase.h: interface for the CDxVideoGraphBase class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include <dshow.h>		// DirectShow
#include <atlbase.h>	// ATL classes

// File filter for OpenFile dialog. It includes all the media file extension DirectX can process.
#define MEDIA_FILE_FILTER_TEXT \
    TEXT("Video Files (*.avi;*.asf;*.mpg;*.mpeg;*.mts)|*.avi;*.asf;*.mpg;*.mpeg;*.mts|")\
    TEXT("Images (*.jpg;*.gif;*.bmp)|*.jpg;*.gif;*.bmp|")\
    TEXT("All Files (*.*)|*.*||")

// Safe release the resouces.
#define SafeRelease(x) { if(x) { (x)->Release(); (x)= NULL; } }

// Check for error, return false if failed. (Jump-If-Failed)
#define JIF(x) if (FAILED(hr=(x))) { \
    errorHandler(TEXT("FAILED(hr=0x%x) in ") TEXT(#x) TEXT("\n"), hr);\
    return hr;}

class CDxVideoGraphBase  
{
public:
    // Video Source ( Video file, capture device 0, 1, ... )
    #define MAX_CAP_DEVICES 10  // maximum number of capture devices
    enum VideoSource {
        VideoFilePlayback = -1,  // video file
        VideoCapDevice0   = 0,   // cap device 0
        VideoCapDevice1, // 1    // cap device 1
        VideoCapDevice2, // 2    // ...
        VideoCapDevice3, // 3
        VideoCapDevice4, // 4
        VideoCapDevice5, // 5
        VideoCapDevice6, // 6
        VideoCapDevice7, // 7
        VideoCapDevice8, // 8
        VideoCapDevice9, // 9
        VideoCapDevice10 // 10
    };

public: // Operations
    ///////////////////////////////////////////////////////////////////
    // Constructor and Deconstructor
    ///////////////////////////////////////////////////////////////////
    CDxVideoGraphBase();
    virtual ~CDxVideoGraphBase();
    // Set the window to render the video
    virtual HRESULT SetRenderWindow(HWND hWnd, RECT * pImgRect = NULL);
    virtual HRESULT SetRenderWindow(HWND hWnd, int x, int y);
    virtual void    ReleaseFilterGraph();    // Release the whole Graph

    ///////////////////////////////////////////////////////////////////
    // Functions to Capture, Playback, Setup Cameras ...
    ///////////////////////////////////////////////////////////////////
    virtual HRESULT RenderFile(const TCHAR * vname);// Playback a file
    virtual int     GetNumCapDev();                // # capture devices
    // Get specified device name
    virtual bool    GetCapDevName(int i, TCHAR *pDevStrName=NULL, 
                                         TCHAR *pDevDispName=NULL);
    // Specify capture device by index or device name
    virtual HRESULT PreviewLive(int capInd = 0, bool fSetup = false);
    virtual HRESULT PreviewLive(TCHAR * capName, bool fSetup = false);
    virtual HRESULT CaptureToFile(const TCHAR *vname);
    virtual HRESULT StartCapture();           // begin capture to file
    virtual HRESULT StopCapture();            // stop  capture to file
    virtual HRESULT AdjustCamera();           // Setup current camera

    ///////////////////////////////////////////////////////////////////
    // Media Constrol Functions: Play, Stop, Seek
    ///////////////////////////////////////////////////////////////////
    void        Play();            // Play the video
    void        Pause();           // Pause the video
    void        Stop();            // Stop the video
    LONGLONG    Seek(int offset);  // seek w.r.t current position
    LONGLONG    SeekToStart();     // Goto the begin of the video
    LONGLONG    SeekToEnd();       // Goto the end of the video

    ///////////////////////////////////////////////////////////////////
    // Get the attribute of the filter graph
    ///////////////////////////////////////////////////////////////////
    VideoSource GetVideoSource(TCHAR * vn = NULL); // Get source name
    SIZE        GetVideoSize();         // Get the vidoe size
    double      GetAvgFrameRate();      // Get the rendering frame rate
    LONGLONG    GetFrameNumber();       // Get current frame number
    bool        IsGraphRunning();       // Get current graph status

protected: // Operations
    ///////////////////////////////////////////////////////////////////
    // Build the graph needed to render the video and control it
    ///////////////////////////////////////////////////////////////////
    virtual HRESULT createFilterGraph(); // Create the filter graph
    virtual HRESULT startGraph();        // Start the playing
    virtual void    stopGraph();         // Stop playing
    virtual HRESULT setVideoWindowSize();// Set render window

    ///////////////////////////////////////////////////////////////////
    // Build the essensial filters (called in createFilterGraph())
    ///////////////////////////////////////////////////////////////////
    virtual HRESULT addSrcFilter(IBaseFilter **ppSrc);
    virtual HRESULT addExtraFilters(IPin * pIn, IPin ** ppOut);
    virtual HRESULT addRenderFilters(IPin *pIn);

    ///////////////////////////////////////////////////////////////////
    // Helper functions:
    ///////////////////////////////////////////////////////////////////
    HRESULT setupCapture(IBaseFilter * pSrcFilter); // resolution, rate
    HRESULT showPropertyPage(IUnknown* pIU, const WCHAR* name);  // COM
    HRESULT get_pin(IBaseFilter * pFilter, PIN_DIRECTION dir, int n, IPin **ppPin);
    void    errorHandler(TCHAR * szFormat, ...);   // Show Error message

    ///////////////////////////////////////////////////////////////////
    // Helper functions for debug filter graph
    // register the filter graph built so that GraphEdt can view it
    ///////////////////////////////////////////////////////////////////
    HRESULT addGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
    void    removeGraphFromRot(DWORD dwRegister);

protected: // Attributes
    ///////////////////////////////////////////////////////////////////
    // DirectShow interface pointers
    ///////////////////////////////////////////////////////////////////
    IGraphBuilder *         m_pGraphBuilder; // build render graph
    ICaptureGraphBuilder2 * m_pCaptureGraphBuilder2; // capture graph
    IFilterGraph*           m_pFilterGraph;  // Filter Graph
    IMediaControl*          m_pMediaControl; // MediaControl
    IVideoWindow*           m_pVideoWindow;  // window to play video
    IMediaSeeking*          m_pMediaSeeking; // Seeking interface
    IBaseFilter *           m_pSrcFilter;    // Source filter
    IBaseFilter *           m_pRenderFilter; // Render filter

    ///////////////////////////////////////////////////////////////////
    // Source filter: (render file or capture live)
    ///////////////////////////////////////////////////////////////////
    VideoSource      m_nVideoSource;      // Source (file/cap devices)
    WCHAR            m_wcMediaName[1024]; // Video source name (string)
    IMoniker *       m_pCapMonikers[MAX_CAP_DEVICES];  // cap devices
    int              m_nNumOfCapDev;           // # cap devices
    bool             m_fShowCaptureProperties; // custom cap setting
    AM_MEDIA_TYPE *  m_pCaptureSetting;        // rate, resolution...

    ///////////////////////////////////////////////////////////////////
    // Output: display video in m_oVideoRect in m_hRenderWnd or output
    // video to a file
    ///////////////////////////////////////////////////////////////////
    HWND    m_hRenderWnd;         // The Window to display the video
    RECT    m_oVideoRect;         // The viewport to display the video
    bool    m_fIsScaling;         // scale video to the whole window
    WCHAR   m_wcOutputFile[1024]; // captured to this file

    ///////////////////////////////////////////////////////////////////
    // Do we want the filter graph to be viewable by GraphEdt
    ///////////////////////////////////////////////////////////////////
    bool    m_fRegisterFilterGraph;
    DWORD   m_dwGraphRegister;
};

class CVideoDoc;
