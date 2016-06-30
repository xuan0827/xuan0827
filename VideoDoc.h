
// VideoDoc.h : CVideoDoc 類別的介面
//
#define __D3DRM_H__


#pragma once
#include "GraphBase.h"
#include "Retangle.h"
#include "RogerFunction.h"
#include "ImgProcess.h"
#include "AdaboostDetection.h"
#include "ipoint.h"

// using CxImage Library
#include ".\CxImage\ximage.h"
#include ".\CxImage\ximajpg.h"
#include <qedit.h>

typedef struct {
	int		left, right, top, bottom;
	int cx, cy;
	double	score;
} VEHICLE;


class CVideoDoc : public CDocument
	, public CDxVideoGraphBase
	, public ISampleGrabberCB
{
protected: // 僅從序列化建立
	CVideoDoc();
	DECLARE_DYNCREATE(CVideoDoc)

	// 屬性
public:
	BOOL Known;

	int	  FrameCnt;
	int	  FrameCnt2;
	// learn vehicle Height along the road 
	vector <VEHICLE> V_histoy;
	BOOL HFlag; 
	int *EstimatedH;

//	TCHAR **m_strDevStrNames;   // Cap device friendly name
//	TCHAR **m_strDevDispNames;  // Cap devive name

	//視窗控制
	HWND hWin;
	HDC hdcStill;
	HDC CompMemDC;
	HBITMAP  testbmp,oldbmp;
	int  ImgW,ImgH;
	bool UsingSmallFrame;

	//textout
	CPoint DrawTextPosition;

	//Video
	BYTE *pData;	// Pointer to the actual image buffer
	BYTE *p_pData;
	//BYTE *tData;
	BYTE *BufferRAW;

	CString m_filename;
	//原始寬高 
	int nWidth;
	int nHeight;
	int	nPixel;
	int	nSize;
	int nBPP;
	int	nEffw;
	
	bool IsPause;	//是否暫停


	//ROI
	Retangle *SelectRect;
	CPoint ROI_LU,ROI_RB; //in the pData

	//Bitmap
	BITMAPINFOHEADER bih;

	//process
	//double *histogram;

	//參數
	int EdgemapSize;
	int *Edgemap;
	double *dROI_GRAY_IntegralImage;
	double *dROI_GRADIENT_IntegralImage;
// 	vector<MyIpoint>ipts1;
// 	IpVec myipts;

	int _height, _width, _size, _pixel, _effw;
	BYTE *ROI_RGB  ;
	BYTE *ROI_GRAY ;
	BYTE *ROI_GRADIENT;

	//Adaboost
	bool m_bLoadVehicleCascade;
	bool m_bLoadPedestrianCascade;
	CString m_strCarCascadeFile;
	CAdaboostDetection _ObjectDetection;
	CvHaarClassifierCascade *_VehicleCascade, _PedestrianCascade;
	CvMemStorage* _storage;
	IplImage *_gray;

	CARINFO car;
	CARHEADINFO MyCar; // defined by Chen

	//Candidate
	CarCandidate CarCollect[30];
	vector<VEHICLE> VehicleCandidates;
	int CarCollect_count;
	//DetectionResult
	DetectedResult DetResult;
	CRogerFunction FuncMe;
	CImgProcess FuncImg;
	//作業
public:
	// The interface needed by ISampleGrabber Filter

	// fake out any COM ref counting
	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release() { return 1; }
	// fake out any COM QI'ing
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv) {
		if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) {
			*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
			return NOERROR;
		}    
		return E_NOINTERFACE;
	}

	// The frame callback function called on each frame
	// We can add the processing code in this function
	STDMETHODIMP SampleCB(double SampleTime, IMediaSample *pSample);
	// Another callback function. I am not using it now.
	STDMETHODIMP BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen);

	// We have to set the mediaType as initialization, we can not get it 
	// through the pSample->GetMediaType(), it will be NULL if the media
	// type did not change.
	void  SetGrabMediaType( AM_MEDIA_TYPE mt ) { m_mediaType = mt; }

	// Members for adding the ISampleGrabber to the filter graph
	// Overide this function to add extra filters to process media data
	virtual HRESULT addExtraFilters(IPin * pIn, IPin ** ppOut);
	// The media type we are processing
	AM_MEDIA_TYPE       m_mediaType;

	// 	void  AddDevicesToMenu();   // add capture device to menu
	// 	void  StartPreview(int i, bool fSetup = false);  // begin capture

	// 覆寫
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

	// 程式碼實作
public:
	virtual ~CVideoDoc();
	int TrainingVehicleHeight(int _height, int top, int bottom);

//	void DetectionCore_Chen(BYTE *output, BYTE *pBufferRAW, int s_w, int s_h, int s_bpp);
	void DetectionCore_from_ROI();
	void DetectionCore_by_Sampling();
	void DetectionCore_Yen(BYTE *output, BYTE *pBufferRAW, int s_w, int s_h, int s_bpp, bool Display_Switch1, bool Display_Switch2, bool Display_Switch3);
	void ProduceCadndidate(BYTE *output, int *Edgemap, int s_w, int s_h, int s_bpp);
	void EdgeMapSearch(BYTE *output, int *EdgeMap, int s_h, int s_w, int s_bpp, int top, int button, int index);
	void AdaboostObjDect(BYTE *output, IplImage *_gray, int s_w, int s_h, int s_bpp); //cvIntegral
	int CalculateOverlapArea(int lx1, int rx1,int top1,int bottom1, int lx2, int rx2, int top2, int bottom2);
	void Cximage2Iplimage();

	LONGLONG GetFrameNum(){return GetFrameNumber();}
	double GetFrameRate(){return GetAvgFrameRate();}
	SIZE GetVideoSizeNumber(){return GetVideoSize();}

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// 產生的訊息對應函式
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 為搜尋處理常式設定搜尋內容的 Helper 函式
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	afx_msg void OnFileOpen();
	afx_msg void OnVideoStephome();
	afx_msg void OnUpdateVideoStephome(CCmdUI *pCmdUI);
	afx_msg void OnVideoStepbackward();
	afx_msg void OnUpdateVideoStepbackward(CCmdUI *pCmdUI);
	afx_msg void OnVideoStepforward();
	afx_msg void OnUpdateVideoStepforward(CCmdUI *pCmdUI);
	afx_msg void OnVideoSeekend();
	afx_msg void OnUpdateVideoSeekend(CCmdUI *pCmdUI);
	// 	afx_msg void OnCaptureDevice0();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	afx_msg void OnVideoPlay();
	afx_msg void OnVideoPause();
	afx_msg void OnUpdateVideoPlay(CCmdUI *pCmdUI);
	afx_msg void OnUpdateVideoPause(CCmdUI *pCmdUI);
	//void preprocess(void);
};
