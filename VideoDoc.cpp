
// VideoDoc.cpp : CVideoDoc 類別的實作
//

#include "stdafx.h"
// SHARED_HANDLERS 可以定義在實作預覽、縮圖和搜尋篩選條件處理常式的
// ATL 專案中，並允許與該專案共用文件程式碼。
#ifndef SHARED_HANDLERS
#include "Video.h"
#endif

#include "MainFrm.h"
#include "VideoDoc.h"
#include "VideoView.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CVideoDoc
#define CV_CALL( Func )                                             \
{                                                                   \
	Func;                                                           \
	CV_CHECK();                                                     \
}
IMPLEMENT_DYNCREATE(CVideoDoc, CDocument)

	BEGIN_MESSAGE_MAP(CVideoDoc, CDocument)
		ON_COMMAND(ID_FILE_OPEN, &CVideoDoc::OnFileOpen)
		ON_COMMAND(ID_VIDEO_PLAY, &CVideoDoc::OnVideoPlay)
		ON_COMMAND(ID_VIDEO_PAUSE, &CVideoDoc::OnVideoPause)
		ON_COMMAND(ID_VIDEO_STEPHOME, &CVideoDoc::OnVideoStephome)
		ON_COMMAND(ID_VIDEO_STEPBACKWARD, &CVideoDoc::OnVideoStepbackward)
		ON_COMMAND(ID_VIDEO_STEPFORWARD, &CVideoDoc::OnVideoStepforward)
		ON_COMMAND(ID_VIDEO_SEEKEND, &CVideoDoc::OnVideoSeekend)
		ON_UPDATE_COMMAND_UI(ID_VIDEO_PLAY, &CVideoDoc::OnUpdateVideoPlay)
		ON_UPDATE_COMMAND_UI(ID_VIDEO_PAUSE, &CVideoDoc::OnUpdateVideoPause)
		ON_UPDATE_COMMAND_UI(ID_VIDEO_STEPHOME, &CVideoDoc::OnUpdateVideoStephome)
		ON_UPDATE_COMMAND_UI(ID_VIDEO_STEPBACKWARD, &CVideoDoc::OnUpdateVideoStepbackward)
		ON_UPDATE_COMMAND_UI(ID_VIDEO_STEPFORWARD, &CVideoDoc::OnUpdateVideoStepforward)
		ON_UPDATE_COMMAND_UI(ID_VIDEO_SEEKEND, &CVideoDoc::OnUpdateVideoSeekend)
		// 	ON_COMMAND(ID_DEVICE0, &CVideoDoc::OnCaptureDevice0)
	END_MESSAGE_MAP()

#define VehicleWidth  120 // 76 // 80
#define VehicleHeight 60 // 38 // 40

	CVideoDoc::CVideoDoc()
		:Known(FALSE),
		FrameCnt(0), FrameCnt2(0),
		hdcStill(NULL), CompMemDC(NULL),
		oldbmp(NULL),testbmp(NULL),
		pData(NULL),
		IsPause(FALSE),BufferRAW(NULL),
		Edgemap(NULL),EdgemapSize(0),
		p_pData(NULL),UsingSmallFrame(FALSE)
	{
		// TODO: 在此加入一次建構程式碼
		_storage=NULL;
		SelectRect = NULL;
		m_bLoadVehicleCascade = m_bLoadPedestrianCascade = false;
		_gray = NULL;
		_storage = NULL;
		dROI_GRAY_IntegralImage = dROI_GRADIENT_IntegralImage = NULL;
		V_histoy.clear();
		HFlag=FALSE; 
		EstimatedH=NULL;
		VehicleCandidates.clear();
	}

	CVideoDoc::~CVideoDoc()
	{
		if( EstimatedH ) delete [] EstimatedH;

		if (dROI_GRAY_IntegralImage) delete[] dROI_GRAY_IntegralImage;
		if (dROI_GRADIENT_IntegralImage) delete[] dROI_GRADIENT_IntegralImage;
		
		if(SelectRect) delete [] SelectRect;

		if(BufferRAW)
			delete [] BufferRAW;
		if(Edgemap)
			delete []Edgemap;
		if(p_pData && UsingSmallFrame)
		{
			delete []p_pData;
		}
		
		//if (ROI_RGB) delete[] ROI_RGB;
		//if (ROI_GRAY) delete[] ROI_GRAY;
		//if (ROI_GRADIENT) delete[] ROI_GRADIENT;
		cvReleaseImage(&_gray);

		if(_storage )	cvReleaseMemStorage(&_storage);
		if( car.Local_Line.size() > 0 )
		{
			for(int i=0; i < car.Local_Line.size(); i++ )
				delete [] car.Local_Line[i].VerticalPeak;
			car.Local_Line.clear();

		}

		V_histoy.clear();
		VehicleCandidates.clear();
		
	}

	BOOL CVideoDoc::OnNewDocument()
	{
		if (!CDocument::OnNewDocument())
			return FALSE;

		return TRUE;
	}




	// CVideoDoc 序列化

	void CVideoDoc::Serialize(CArchive& ar)
	{
		if (ar.IsStoring())
		{
			// TODO: 在此加入儲存程式碼
		}
		else
		{
			// TODO: 在此加入載入程式碼
		}
	}

#ifdef SHARED_HANDLERS

	// 縮圖的支援
	void CVideoDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
	{
		// 修改這段程式碼以繪製文件的資料
		dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

		CString strText = _T("TODO: implement thumbnail drawing here");
		LOGFONT lf;

		CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
		pDefaultGUIFont->GetLogFont(&lf);
		lf.lfHeight = 36;

		CFont fontDraw;
		fontDraw.CreateFontIndirect(&lf);

		CFont* pOldFont = dc.SelectObject(&fontDraw);
		dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
		dc.SelectObject(pOldFont);
	}

	// 搜尋處理常式的支援
	void CVideoDoc::InitializeSearchContent()
	{
		CString strSearchContent;
		// 設定來自文件資料的搜尋內容。
		// 內容部分應該以 ";" 隔開

		// 範例:  strSearchContent = _T("point;rectangle;circle;ole object;");
		SetSearchContent(strSearchContent);
	}

	void CVideoDoc::SetSearchContent(const CString& value)
	{
		if (value.IsEmpty())
		{
			RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
		}
		else
		{
			CMFCFilterChunkValueImpl *pChunk = NULL;
			ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
			if (pChunk != NULL)
			{
				pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
				SetChunkValue(pChunk);
			}
		}
	}

#endif // SHARED_HANDLERS

	// CVideoDoc 診斷

#ifdef _DEBUG
	void CVideoDoc::AssertValid() const
	{
		CDocument::AssertValid();
	}

	void CVideoDoc::Dump(CDumpContext& dc) const
	{
		CDocument::Dump(dc);
	}
#endif //_DEBUG


	// CVideoDoc 命令
	void CVideoDoc::OnFileOpen() 
	{
		// Choose the video file by fileopen dialog (MEDIA_FILE_FILTER_TEXT
		// specifies the file types we support
		CFileDialog open_dlg( true, 0, 0, OFN_FILEMUSTEXIST, MEDIA_FILE_FILTER_TEXT, NULL );
		if( open_dlg.DoModal() == IDOK )
		{
			// Get the video file name and open it.
			CString str = open_dlg.GetPathName();
			OnOpenDocument(str.GetBuffer(0));
			SetTitle(open_dlg.GetFileTitle());
			m_filename = open_dlg.GetFileTitle();
			HFlag=FALSE;
			V_histoy.clear();
			if(EstimatedH)
				delete [] EstimatedH;
			EstimatedH=NULL;
		}
		Pause();
	}

	BOOL CVideoDoc::OnOpenDocument(LPCTSTR lpszPathName)
	{
		if (!__super::OnOpenDocument(lpszPathName))
			return FALSE;

		// TODO:  在此加入特別建立的程式碼

		return SUCCEEDED(RenderFile(lpszPathName));
	}

	HRESULT CVideoDoc::addExtraFilters(IPin *pIn, IPin **ppOut)
	{
		HRESULT hr;

		// Grab filter which get image data for each frame
		CComPtr<IBaseFilter>	pGrabFilter = NULL;
		// The interface to set callback function
		CComPtr<ISampleGrabber>	pSampleGrabber = NULL;

		CComPtr<IPin>			pTmpPin = NULL;
		// Now Add the ISampleGrabber Filter to grab the video frames
		JIF(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID *)&pGrabFilter));
		hr = pGrabFilter->QueryInterface(IID_ISampleGrabber, (void **)&pSampleGrabber);
		if(FAILED(hr))
			return hr;

		// Now Set the working mode of this ISampleGrabber
		// Specify what media type to process, to make sure it is connected correctly
		// We now process full decompressed RGB32 image data
		AM_MEDIA_TYPE			mt;
		ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
		mt.majortype = MEDIATYPE_Video;
		mt.subtype = MEDIASUBTYPE_RGB24;
		mt.formattype = FORMAT_VideoInfo;
		pSampleGrabber->SetMediaType(&mt);

		// Set working mode as continuous with no buffer
		pSampleGrabber->SetOneShot(FALSE);
		pSampleGrabber->SetBufferSamples(FALSE);

		// Add the filter to the graph
		JIF(m_pFilterGraph->AddFilter(pGrabFilter, L"Grabber"));
		JIF(get_pin(pGrabFilter, PINDIR_INPUT, 0, &pTmpPin));
		JIF(m_pGraphBuilder->Connect(pIn, pTmpPin));

		// Set up the callback interface of this grabber
		pSampleGrabber->SetCallback((ISampleGrabberCB *)this, 0);

		// Set the media type (needed to get the bmp header info
		pSampleGrabber->GetConnectedMediaType(&mt);
		SetGrabMediaType(mt);

		// Return the output pin of the last filter for further rendering
		JIF(get_pin(pGrabFilter, PINDIR_OUTPUT, 0, ppOut));
		return S_OK;
	}

	STDMETHODIMP CVideoDoc::BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) {
		return E_NOTIMPL;
	}

	void CVideoDoc::OnVideoPlay()
	{
		// TODO: 在此加入您的命令處理常式程式碼
		Play();
		IsPause = false;
	}

	void CVideoDoc::OnVideoPause()
	{
		// TODO: 在此加入您的命令處理常式程式碼
		Pause();
		IsPause = true;

	}

	void CVideoDoc::OnVideoStephome()
	{
		// TODO: 在此加入您的命令處理常式程式碼
		SeekToStart();
		//Play();
		//IsPause = true;
		// 	UpdateDocTitle();
	}

	void CVideoDoc::OnVideoStepbackward()
	{
		// TODO: 在此加入您的命令處理常式程式碼
		Seek(-1);
	}

	void CVideoDoc::OnVideoStepforward()
	{
		// TODO: 在此加入您的命令處理常式程式碼
		Seek(1);
	}

	void CVideoDoc::OnVideoSeekend()
	{
		// TODO: 在此加入您的命令處理常式程式碼
		SeekToEnd();
		Pause();
		IsPause = true;
	}

	void CVideoDoc::OnUpdateVideoPlay(CCmdUI *pCmdUI)
	{
		// TODO: 在此加入您的命令更新 UI 處理常式程式碼
		pCmdUI->Enable(GetVideoSource() == -1);
	}

	void CVideoDoc::OnUpdateVideoPause(CCmdUI *pCmdUI)
	{
		// TODO: 在此加入您的命令更新 UI 處理常式程式碼
		pCmdUI->Enable(GetVideoSource() == -1);
	}

	void CVideoDoc::OnUpdateVideoStephome(CCmdUI *pCmdUI)
	{
		// TODO: 在此加入您的命令更新 UI 處理常式程式碼
		pCmdUI->Enable(GetVideoSource() == -1);
	}


	void CVideoDoc::OnUpdateVideoStepbackward(CCmdUI *pCmdUI)
	{
		// TODO: 在此加入您的命令更新 UI 處理常式程式碼
		pCmdUI->Enable(GetVideoSource() == -1);
	}

	void CVideoDoc::OnUpdateVideoStepforward(CCmdUI *pCmdUI)
	{
		// TODO: 在此加入您的命令更新 UI 處理常式程式碼
		pCmdUI->Enable(GetVideoSource() == -1);
	}

	void CVideoDoc::OnUpdateVideoSeekend(CCmdUI *pCmdUI)
	{
		// TODO: 在此加入您的命令更新 UI 處理常式程式碼
		pCmdUI->Enable(GetVideoSource() == -1);
	}
	
	STDMETHODIMP CVideoDoc::SampleCB(double SampleTime, IMediaSample *pSample)
	{
		// get current media type
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)m_mediaType.pbFormat;


		nBPP = pvi->bmiHeader.biBitCount / 8;
		nWidth = pvi->bmiHeader.biWidth;
		nHeight = pvi->bmiHeader.biHeight;
		nPixel = nWidth * nHeight;
		nSize = nPixel * nBPP;
		nEffw = (nWidth * nBPP + 3) >> 2 << 2;

		pSample->GetPointer(&pData);

		p_pData = pData;

		EdgemapSize = (nHeight)*(nWidth + 1);

		CString      strendname;
		CString      WindowCaption;

		if (!BufferRAW)
			BufferRAW = new BYTE[nPixel];
		if (!Edgemap)
			Edgemap = new int[nPixel];

		if (!dROI_GRAY_IntegralImage) dROI_GRAY_IntegralImage = new double[nPixel];
		if (!dROI_GRADIENT_IntegralImage) dROI_GRADIENT_IntegralImage = new double[nPixel];


		//		if(!_gray){
		//			_gray = cvCreateImage(cvSize(nWidth,nHeight),IPL_DEPTH_8U,1);
		//	}

		//		if(_gray && SelectRect){
		//			cvReleaseImage(&_gray);
		//			_gray = cvCreateImage(cvSize(nWidth,nHeight),IPL_DEPTH_8U,1);
		//		}
		//	Cximage2Iplimage(); 

		if (SelectRect)
		{
			DetectionCore_from_ROI();
		}

		::PostMessage(hWin, WM_SHOWPIC, 0, 0L);
		FrameCnt++;
		FrameCnt2++;
		return(NOERROR);
	}

int CVideoDoc::TrainingVehicleHeight(int _height, int top, int bottom)
{
	int constriant = _height/2;
	int iteration=0;
	int topH=0, bottomH=0;
	int topY=0, bottomY=0;
	if(EstimatedH)
		delete [] EstimatedH;
	EstimatedH = new int[nWidth];
	memset(EstimatedH,0,sizeof(int) * nWidth);

	for(int i=0; i < V_histoy.size(); i++)
		for(int j=+1; j< V_histoy.size(); j++)
			if( abs(V_histoy[i].cy-V_histoy[j].cy) > constriant )
			{
				if(V_histoy[i].cy > V_histoy[j].cy )
				{
					topY += V_histoy[j].cy;
					bottomY += V_histoy[i].cy;
					topH += (V_histoy[j].bottom-V_histoy[j].top);
					bottomH += (V_histoy[i].bottom-V_histoy[i].top);
				}
				else
				{
					topY += V_histoy[i].cy;
					bottomY += V_histoy[j].cy;
					topH += (V_histoy[i].bottom-V_histoy[i].top);
					bottomH += (V_histoy[j].bottom-V_histoy[j].top);
				}

				iteration++;
				if(iteration > 100 ) {i=j=V_histoy.size()+1; }
			}


	if( iteration < 3)
		return false;

	topY /= iteration;
	topH /= iteration;
	bottomH /=iteration;
	bottomY/=iteration;

	// interpolation all other possisble heights
	double rate=0;

	if( topY != bottomY ) rate = (double) (topH-bottomH) / (topY-bottomY);

	for(int i=top; i < bottom; i++)
	{ EstimatedH[i]=(int) ((i-bottomY) * rate + bottomH); 
	}

	return true;
}
	void  CVideoDoc::DetectionCore_by_Sampling()
	{
		BYTE *pBufferRAW = BufferRAW;

//		FuncMe.I_CopyBuffer321( p_pData, pBufferRAW, Edgemap, nWidth, nHeight, nEffw);//轉灰階+edgemap


	}
	void CVideoDoc::DetectionCore_Yen(BYTE *output, BYTE *pBufferRAW, int s_w, int s_h, int s_bpp, bool Display_Switch1, bool Display_Switch2, bool Display_Switch3){
		CRogerFunction FuncMe;
		CImgProcess    FuncImg;	
		CPoint LB,RU,P1,P2;
		BYTE *pBuffer1_2,*pBuffer1_1 = NULL;
		int *DirectionMap1;
		int nDimension = 5;
		int Pixel = s_w*s_h;
		int sum = 0;
		bool using_roi = false;
		
	
		//IplImage *_gray;
		int nVlength = 64;

		DirectionMap1 = new int[nPixel];
		pBuffer1_2 = pBufferRAW;
		if(!_storage) _storage=cvCreateMemStorage(0);

		if( car.Local_Line.size() > 0 )
		{
			for(int i=0; i < car.Local_Line.size(); i++ )
				delete [] car.Local_Line[i].VerticalPeak;
			car.Local_Line.clear();
		}
		

		if(!SelectRect){
			LB.x = 0;
			LB.y = 0;
		}else
			using_roi = true;
			
//		FuncMe.I_SURF1( pBuffer1_2, ipts1, s_w, s_h, 8 , 10 , nVlength, FALSE/*double size*/);//4,8
//		FuncMe.SelfSURFMatch2( ipts1, car, output, nWidth, nHeight, 0, ROI_LU, true, using_roi, UsingSmallFrame);






		//s_w,s_h為灰階影像之寬高
		//nWidth,nHeight為可能縮放過的影像寬高
		//nWidth,oHeight為輸入frame之原始寬高
 		FuncMe.HistMaxLine_video1(output,car, pBuffer1_1, pBuffer1_2, DirectionMap1, output, s_w, s_h, s_bpp, nWidth, nHeight, ROI_LU, true, FALSE, FALSE, using_roi,UsingSmallFrame);
// 		//產生candidate
 		ProduceCadndidate(output, Edgemap, nWidth, nWidth, s_bpp);
// 		//Adaboost物件偵測
 		AdaboostObjDect(output, _gray, nWidth, nHeight, nBPP);

// 		if(Display_Switch1){
// 			if(SelectRect){
// 				for(int i=0;i<ipts1.size();i++){
// 					P1.x = (int)ipts1[i].x + ROI_LU.x;
// 					P1.y = (int)ipts1[i].y + ROI_LU.y;
// 					FuncImg.DrawCross( output, nWidth, nHeight, 3, P1, 4 ,RGB(255,0,0),3);
// 				}
// 			}else{
// 				for(int i=0;i<ipts1.size();i++){
// 					P1.x = (int)ipts1[i].x;
// 					P1.y = (int)ipts1[i].y;
// 					FuncImg.DrawCross( output, nWidth, nHeight, 3, P1, 4 ,RGB(255,0,0),3);
// 				}
// 			}
// 		}

		if(Display_Switch2){
			cvNamedWindow("Result", 1);
			cvShowImage("Result", _gray);
			cvSaveImage("C:\\abc.jpg",_gray);
			cvWaitKey(0);
		}
		
		delete []DirectionMap1;
		delete []pBuffer1_1;
	}
	void CVideoDoc::EdgeMapSearch(BYTE *output, int *EdgeMap, int s_h, int s_w, int s_bpp, int top, int button, int index){
		//top和button input前要先上下對調
		CRogerFunction FuncMe;
		CImgProcess    FuncImg;
		int i,j,k,k2,check_max;
		int pCenter = car.Local_Line[index].pCenter.x;
		int midindex = 0;
		int avg = 0;
		int sum = 0;
		int _h = abs(top-button);
		int _w = _h*1.5;
		int start = car.Local_Line[index].pCenter.x - _h;
		int end = car.Local_Line[index].pCenter.x + _h;
		int *ptr = EdgeMap;
		int *sub = new int[s_w];
		int *target = new int [s_w];
		memset(target,0,sizeof(int)*s_w);
		int *tmp = new int [s_w];
		int peaktotal[50];

		if(start<0)
			start = 0;
		if(end > s_w)
			end = s_w;
		//	if(_h < s_h/3){
		ptr += (button)*(s_w+1);
		for(i=start;i<end;i++){
			sub[i] = ptr[i];
		}

		ptr = EdgeMap;
		ptr += (top)*(s_w+1);
		for(i=start;i<end;i++){
			tmp[i] = ptr[i] - sub[i];
			//avg += ptr[i];
		}
		//smooth
		k=5;
		k2 = 2*k-1;
		for(i=start + k;i< end-k;i++){
			for(j=-k;j<k;j++){
				target[i]+=tmp[i+j];
			}
			target[i]/=k2;
		}

		for(i=start;i<end;i++){
			avg += target[i];
		}
		avg /= (end - start);
		avg*=1.2;

		k=10;
		k2 = 2*k-1;
		for(i=start+k;i<end-k;i++){
			check_max=0;
			for(j=-k;j<k;j++){
				if(ptr[i]>ptr[i+j]){
					check_max++;
				}else if(i==i+j){
				}else
					break;
			}
			if(check_max==k2 && ptr[i]>avg){
				if(i > pCenter && midindex == 0)
					midindex = sum-1;
				peaktotal[sum] = i;
				sum++;

			}
		}
		if(midindex <= 0)
			car.Local_Line[index].VerticalPeakMidIndex = -1;
		else
			car.Local_Line[index].VerticalPeakMidIndex = midindex;
		car.Local_Line[index].VerticalPeakTotal = sum;

 		car.Local_Line[index].VerticalPeak = new int[sum];

		memcpy(car.Local_Line[index].VerticalPeak,peaktotal,sizeof(int)*(sum));
		//
		delete []sub;
		delete []target;
		delete []tmp;
	}
	void CVideoDoc::ProduceCadndidate(BYTE *output, int *Edgemap, int s_w, int s_h, int s_bpp){
	/*	CRogerFunction FuncMe;
		CImgProcess    FuncImg;
		CPoint P1,P2;

		int sum = 0;

		for(int i=0;i<car.Local_Line.size();i++){	
			int pCenter = car.Local_Line[i].pCenter.x;
			for(int j=0;j<4;j++){ // 4組candidate

				//抓出垂直PEAK
				if(abs(car.Local_Line[i].Candidate_RU[j].y - car.Local_Line[i].Candidate_LB[j].y) < nHeight/2 && abs(car.Local_Line[i].Candidate_RU[j].y - car.Local_Line[i].Candidate_LB[j].y) > 0) 
					EdgeMapSearch(output,Edgemap,nHeight,nWidth,nBPP,car.Local_Line[i].Candidate_RU[j].y,car.Local_Line[i].Candidate_LB[j].y,i);

// 				for(int k=0;k<CarDetected.Local_Line[i].VerticalPeakTotal;k++){
// 					P1 = CPoint(CarDetected.Local_Line[i].VerticalPeak[k],CarDetected.Local_Line[i].Candidate_RU[j].y);
// 					P2 = CPoint(CarDetected.Local_Line[i].VerticalPeak[k],CarDetected.Local_Line[i].Candidate_LB[j].y);
// 					//gpMsgbar->ShowMessage("%d ",CarDetectedB.Local_Line[i].VerticalPeak[m]);
// 					if(j%2==1)
// 						FuncImg.DrawLine(pData,nWidth,nHeight,nBPP,P1,P2,RGB(255,255,0));
// 					else
// 						FuncImg.DrawLine(pData,nWidth,nHeight,nBPP,P1,P2,RGB(0,255,255));
//
				//過濾
				if(car.Local_Line[i].VerticalPeakMidIndex > 0){
					//gpMsgbar->ShowMessage("MidPeak(%d),Center(%d)\n",CarDetectedB.Local_Line[i].VerticalPeak[CarDetectedB.Local_Line[i].VerticalPeakMidIndex],CarDetectedB.Local_Line[i].pCenter.x);
					for(int k=car.Local_Line[i].VerticalPeakMidIndex ; k >= 0 ; k--){
						int _h = abs(car.Local_Line[i].Candidate_LB[j].y - car.Local_Line[i].Candidate_RU[j].y);
						int _w;
						int tmp_w = pCenter - car.Local_Line[i].VerticalPeak[k];
						// 						if(tmp_w < _h*0.75)
						// 							tmp_w = cvRound(_h*0.75);
						P1.y = car.Local_Line[i].Candidate_LB[j].y;
						P1.x = car.Local_Line[i].VerticalPeak[k];
						P2.y = car.Local_Line[i].Candidate_RU[j].y;
						P2.x = pCenter + tmp_w;
						//P2 = P1.x + tmp_w * 2;
						_w = P2.x - P1.x;
						double ratio = (double)_h/_w;
						//gpMsgbar->ShowMessage("temp_w:%d _h:%d\n",tmp_w,_h);
						if(tmp_w > _h*0.5 && tmp_w < _h && _h < nHeight/3 && _w < nWidth/3 && ratio < 0.75 && ratio >= 0.45){
							if(SelectRect){
								P1.x += ROI_LU.x;
								P1.y += ROI_LU.y;
								P2.x += ROI_LU.x;
								P2.y += ROI_LU.y;
							}
							CarCollect[sum].LB = P1;
							CarCollect[sum].RU = P2;
							sum++;
						}
					}

				}
			}
		}
		CarCollect_count = sum;
		gpMsgbar->ShowMessage("Frame:%d Total:%d\n",FrameCnt,CarCollect_count);*/

	}
	void CVideoDoc::AdaboostObjDect(BYTE *output, IplImage *_gray, int s_w, int s_h, int s_bpp){
		CRogerFunction FuncMe;
		CImgProcess    FuncImg;
		CvSeq* _result;
		CvPoint pt1,pt2;
		CPoint LB,RU;
		CPoint drawP1,drawP2;
		int sum=0;
		

		for(int i=0 ; i<CarCollect_count ; i++){
// 			if(SelectRect){
// 				pt1.x = CarCollect[i].LB.x + ROI_LU.x; //LB
// 				pt1.y = nHeight - (CarCollect[i].LB.y + ROI_LU.y) - 1;
// 				pt2.x = CarCollect[i].RU.x + ROI_LU.x;//RU
// 				pt2.y = nHeight - (CarCollect[i].RU.y + ROI_RU.y) - 1;
// 			}else{
				pt1.x = CarCollect[i].LB.x; //LB
				pt1.y = nHeight - CarCollect[i].LB.y - 1;
				pt2.x = CarCollect[i].RU.x;//RU
				pt2.y = nHeight - CarCollect[i].RU.y - 1;
//			}

// 			if(SelectRect){
// 				cvRectangle( _gray, pt1, pt2, CV_RGB(255,0,0), 3, 8, 0 );
// 				cvNamedWindow("Result", 1);
// 				cvShowImage("Result", _gray);
// 				cvWaitKey(0);
// 			}

			_result = mycvHaarDetectObjects_v4( _gray, _VehicleCascade, _storage,1.1,1, CV_HAAR_FIND_BIGGEST_OBJECT, cvSize(24,16),pt1,pt2); //CV_HAAR_DO_CANNY_PRUNING
			if(_result->total != 0){
				gpMsgbar->ShowMessage("result:%d\n",_result->total);
				for(int m = 0; m < (_result ? _result->total : 0); m++ ){
					CvRect* r = (CvRect*)cvGetSeqElem( _result, m );
					pt1.x = r->x;
					pt2.x = (r->x+r->width);
					pt1.y = r->y;
					pt2.y = (r->y+r->height);

					LB.x = r->x;
					LB.y = abs(nHeight - pt2.y -1);
					//LB.y = abs(nHeight - (r->y+r->height) -1);
					RU.x = (r->x+r->width);
					RU.y = abs(nHeight - pt1.y -1);
					if(UsingSmallFrame){
						LB.x*=2;
						LB.y*=2;
						RU.x*=2;
						RU.y*=2;

					}
					for(int j=0;j<5;j++){
						if(UsingSmallFrame){
							drawP1 = CPoint(LB.x-j,LB.y-j);
							drawP2 = CPoint(RU.x+j,RU.y+j);
							drawP1.x *= 2;
							drawP1.y *= 2;
							drawP2.x *= 2;
							drawP2.y *= 2;
							FuncImg.DrawRect(output,nWidth,nHeight,nBPP,CPoint(LB.x-j,LB.y-j), CPoint(RU.x+j,RU.y+j), RGB(255,0,0));
						}else
							FuncImg.DrawRect(output,nWidth,nHeight,nBPP,CPoint(LB.x-j,LB.y-j), CPoint(RU.x+j,RU.y+j), RGB(255,0,0));
					}
					DetResult.Vehicle[sum].LB = LB;
					DetResult.Vehicle[sum++].RU = RU;
					DetResult.VehicleCount = sum;
				}
			}
		}
		
		
		//_result = mycvHaarDetectObjects_v4( _gray, _VehicleCascade, _storage,1.1,1, CV_HAAR_FIND_BIGGEST_OBJECT, cvSize(24,16),pt1,pt2); //CV_HAAR_DO_CANNY_PRUNING

		// 							if(_result->total != 0){
		// 								//if(1==2){
		// 								gpMsgbar->ShowMessage("result:%d\n",_result->total);
		// 								for(int m = 0; m < (_result ? _result->total : 0); m++ ){
		// 									CvRect* r = (CvRect*)cvGetSeqElem( _result, m );
		// 
		// 									// 									//LB.x = r->x;
		// 									// 									int t = pt1.x;
		// 									// 									LB.x = t;
		// 									// 									//LB.y = abs(nHeight - pt2.y -1);
		// 									// 									LB.y = abs(nHeight - (r->y+r->height) -1);
		// 									// 									RU.x = (r->x+r->width);
		// 									// 									t = pt2.y;
		// 									// 									//RU.y = abs(nHeight - pt1.y -1);
		// 									// 									RU.y = abs(nHeight - t -1);
		// 
		// 									pt1.x = r->x;
		// 									pt2.x = (r->x+r->width);
		// 									pt1.y = r->y;
		// 									pt2.y = (r->y+r->height);
		// 									LB.x = r->x;
		// 
		// 									LB.y = abs(nHeight - pt2.y -1);
		// 									LB.y = abs(nHeight - (r->y+r->height) -1);
		// 									RU.x = (r->x+r->width);
		// 									RU.y = abs(nHeight - pt1.y -1);
		// 
		// 									FuncImg.DrawRect(output,nWidth,nHeight,nBPP,LB, RU, RGB(255,0,0));
		// 									//Pause();
		// 								}
		// 							}
	}
	void CVideoDoc::Cximage2Iplimage(){ //CxImage和IplImage 大小必須相同
		char *ptr = _gray->imageData;
		BYTE *pImageArray;
//方法1
// 		memcpy(ptr,BufferRAW,sizeof(BYTE)*nPixel);
// 		cvFlip(_gray,_gray,1);
//方法2
		pImageArray = BufferRAW;
		pImageArray += nWidth*(nHeight-1);
		for( int i=0 ; i < nHeight ; ptr += nWidth, pImageArray -= nWidth, i++){
			for( int j=0,k=0 ; j < nWidth; j++,k++){
				ptr[ j ]   = pImageArray[k] ; 
			}
		}

	}

	int CVideoDoc::CalculateOverlapArea(int leftA, int rightA,int topA,int bottomA, int leftB, int rightB, int topB, int bottomB)  
	{
		int lx, rx,w,h;
		lx = leftA;
		if(leftA < leftB) lx =leftB;
		rx=rightA;
		if(rx > rightB) rx = rightB;
		w=rx-lx; if(w < 0 ) return 0;
		int ty, by;
		ty = topA;
		if( topB > ty ) ty = topB;
		by = bottomA;
		if( bottomB < by ) by = bottomB;
		h = by -ty;
		if( h < 0 ) return 0;
		return w*h;  
	}
	void  CVideoDoc::DetectionCore_from_ROI()
	{
		int i, j, k;// x, y, z;

		BYTE *ColorMap;
		BYTE *GrayMap;// = BufferRAW;
		vector<VEHICLE> tempVehicleList;

		VehicleCandidates.clear();
		// 從ROI 中做邊緣偵測與灰階化, 其中邊緣的圖會多一個pixel 來存 邊的平均值	

		int temp;
		double sum;
		int shift = 2;
		int *edge = Edgemap;
		//////////////////////////////////////////////////////////////////////////

		BYTE *p_ROI_RGB = ROI_RGB;
		BYTE *p_ROI_GRAY = ROI_GRAY;
		GrayMap = BufferRAW;
		ColorMap = p_pData + ROI_LU.y * nEffw + ROI_LU.x  * nBPP;
		//////////////////////////////////////////////////////////////////////////
		for (int i = 0; i < _height; i++, GrayMap += _width, ColorMap += nEffw, p_ROI_RGB += _effw, p_ROI_GRAY += _width)
		{
			for (j = 0, k = 0; j < _width; j++, k += nBPP)
			{
				p_ROI_RGB[k] = ColorMap[k];
				p_ROI_RGB[k + 1] = ColorMap[k + 1];
				p_ROI_RGB[k + 2] = ColorMap[k + 2];

				temp = (int)ColorMap[k] + ColorMap[k + 1] + ColorMap[k + 2];
				GrayMap[j] = (BYTE)(temp / 3);
				p_ROI_GRAY[j] = (BYTE)(temp / 3);

			}
		}
		//////////////////////////////////////////////////////////////////////////
		int diffGrad = 1;
		FuncMe.Gray2Gradient(ROI_GRAY, ROI_GRADIENT, Edgemap, _width, _height, diffGrad);
		//FuncMe.SaveGrayImage(ROI_GRADIENT, _width, _height, _effw, "Gradient");

		p_pData = pData + ROI_LU.y * nEffw + ROI_LU.x * nBPP;
		BYTE *p_ROI_GRADIENT = ROI_GRADIENT;
		for (i = 0; i < _height; i++, p_pData += nEffw, p_ROI_GRADIENT += _width)
		{
			for (j = 0, k = 0; j < _width; j++, k += 3)
			{
				p_pData[k] = p_ROI_GRADIENT[j];
				p_pData[k + 1] = p_ROI_GRADIENT[j];
				p_pData[k + 2] = p_ROI_GRADIENT[j];
			}
		}
		// Get Integral Images
		double rs = 0.0f, gs = 0.0;
		GrayMap = BufferRAW;
		for (j = 0; j < _width; j++)
		{
			rs += (GrayMap[j] / 255.0);
			gs += (Edgemap[j] / 255.0);

			dROI_GRAY_IntegralImage[j] = rs;
			dROI_GRADIENT_IntegralImage[j] = gs;
		}

		double *p_dROI_GRAY_IntegralImage = dROI_GRAY_IntegralImage + _width;
		double *p_dROI_GRAY_IntegralImage1 = dROI_GRAY_IntegralImage;

		double *p_dROI_GRADIENT_IntegralImage = dROI_GRADIENT_IntegralImage + _width;
		double *p_dROI_GRADIENT_IntegralImage1 = dROI_GRADIENT_IntegralImage;
		GrayMap = BufferRAW + _width;
		edge = Edgemap + _width;
		for (int i = 1; i < _height; ++i, edge += _width, GrayMap += _width, p_dROI_GRAY_IntegralImage += _width, p_dROI_GRAY_IntegralImage1 += _width, p_dROI_GRADIENT_IntegralImage += _width, p_dROI_GRADIENT_IntegralImage1 += _width)
		{
			rs = 0.0f;
			gs = 0.0f;
			for (int j = 0; j < _width; ++j)
			{
				rs += (GrayMap[j] / 255.0);
				gs += (edge[j] / 255.0);
				p_dROI_GRAY_IntegralImage[j] = rs + p_dROI_GRAY_IntegralImage1[j];
				if (gs < 0)
					gs = gs;
				p_dROI_GRADIENT_IntegralImage[j] = gs + p_dROI_GRADIENT_IntegralImage1[j];
			}
		}

		// SURF 特徵擷取
		CPoint Center;
		// 		FuncMe.MYSURF_SURF_GetFromIntegral(IntegralGradient , myipts, _width, _height, Surf_Para, 2 , nVlength, true );
		//Replace to parallel///
		// 		FuncMe.MP_Color2Gradient(ROI_RGB, ROI_GRAY, _width, _height, _effw);

		//Keypoint storage
		std::vector<N_parallelsurf::N_KeyPoint> keyPoints1;
		FuncMe.Parallel_SURF_Extract(ROI_GRAY, _width, _height, keyPoints1);

		// SURF Matching
		NewMatchPair matchpair;
		vector<NewMatchPair>matchpair_bin;

		//int nDist;
		//double dist, d1, d2;
		CPoint mPL, mPR, P1, P2;

		//float dTheta;
		//int idxi, idxj;
		int nBias = 5;
		//////////////////////////////////////////////////////////////////////////

		double *ShadowHistogram;
		if (_height > _width) ShadowHistogram = new double[_height];
		else ShadowHistogram = new double[_width];

		MyCar.All_MatchPair_bin.clear();
		//Replace to parallel///

		nBias = 5;

		FuncMe.Symmetric_Parallel_SURF_Match(keyPoints1, MyCar.All_MatchPair_bin);
		//  Finding Central Line
		vector <int> CenterPosX;
		double *GramW;
		if (_height > _width) GramW = new double[_height];
		else GramW = new double[_width];
		memset(GramW, 0, sizeof(double)*_width);
		double *SmoothingW;
		if (_height > _width) SmoothingW = new double[_height];
		else SmoothingW = new double[_width];

		FuncMe.Find_Center_Line(MyCar.All_MatchPair_bin, CenterPosX, _width, VehicleWidth, GramW, SmoothingW);
		////////觀察SURF點情形/////////////////////////////////////////////////////////////////////////////////
		// 		for (i = 0; i < keyPoints1.size(); i++)
		// 		{
		// 			P1.x = keyPoints1[i]._x + ROI_LU.x;
		// 			P1.y = keyPoints1[i]._y + ROI_LU.y;
		// 			FuncImg.DrawCross(pData, nWidth, nHeight, nBPP, P1, 3, RGB(0, 255, 0));
		// 		}
		// 觀察全部對稱線
		//for (i = 0; i < MyCar.All_MatchPair_bin.size(); i++)
		//{
		//	P1.x = MyCar.All_MatchPair_bin[i].Point_L.x + ROI_LU.x;
		//	P1.y = MyCar.All_MatchPair_bin[i].Point_L.y + ROI_LU.y;
		//	P2.x = MyCar.All_MatchPair_bin[i].Point_R.x + ROI_LU.x;
		//	P2.y = MyCar.All_MatchPair_bin[i].Point_R.y + ROI_LU.y;
		//	FuncImg.DrawLine(pData, nWidth, nHeight, nBPP, P1, P2, RGB(255, 0, 0));
		//}
		///New centerline with it's matchpair//////////////////////////////////////////////////////////////////////////////////////
		Car_Candidate car_centerline;
		NewMatchPair line_matchpair;
		for (i = 0; i < MyCar.Car_Candidate_bin.size(); i++)
		{
			MyCar.Car_Candidate_bin[i].line_matchpair_bin.clear();
			MyCar.Car_Candidate_bin.clear();
		}
		//將對稱對 assign to its own centerline
		for (i = 0; i < CenterPosX.size(); i++)
		{
			car_centerline.nCenterLinPosX = CenterPosX[i];
			MyCar.Car_Candidate_bin.push_back(car_centerline);
		}
		for (i = 0; i < MyCar.Car_Candidate_bin.size(); i++)
		{
			for (j = 0; j < MyCar.All_MatchPair_bin.size(); j++)
			{
				if (abs(MyCar.Car_Candidate_bin[i].nCenterLinPosX - MyCar.All_MatchPair_bin[j].CenterPoint.x) < 5)
				{
					line_matchpair.Point_L = MyCar.All_MatchPair_bin[j].Point_L;
					line_matchpair.Point_R = MyCar.All_MatchPair_bin[j].Point_R;
					line_matchpair.nCenterPosX = CenterPosX[i];
					MyCar.Car_Candidate_bin[i].line_matchpair_bin.push_back(line_matchpair);
				}
			}
		}
		for (i = 0; i < MyCar.Car_Candidate_bin.size(); i++)
		{
			if ( MyCar.Car_Candidate_bin[i].line_matchpair_bin.size() < 5 )
			{
				MyCar.Car_Candidate_bin.erase(MyCar.Car_Candidate_bin.begin() + i);
				i = -1;
			}
		}
		CenterPosX.clear();
		for (i = 0; i < MyCar.Car_Candidate_bin.size(); i++)
		{
			CenterPosX.push_back(MyCar.Car_Candidate_bin[i].nCenterLinPosX);
		}
		//觀察
// 		gpMsgbar->ShowMessage("%d\n", MyCar.All_MatchPair_bin.size());
// 		gpMsgbar->ShowMessage("\n");
		for (i = 0; i < MyCar.Car_Candidate_bin.size(); i++)
		{
// 			gpMsgbar->ShowMessage("(%d).size[%d]...[%d]\n", i, MyCar.Car_Candidate_bin[i].line_matchpair_bin.size(), MyCar.Car_Candidate_bin[i].nCenterLinPosX );
			for (j = 0; j < MyCar.Car_Candidate_bin[i].line_matchpair_bin.size(); j++ )
			{
// 				gpMsgbar->ShowMessage("L(%d,%d) R(%d,%d)\n", MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_L.x, MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_L.y, MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_R.x, MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_R.y);
				P1.x = MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_L.x + ROI_LU.x;
				P1.y = MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_L.y + ROI_LU.y;
				P2.x = MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_R.x + ROI_LU.x;
				P2.y = MyCar.Car_Candidate_bin[i].line_matchpair_bin[j].Point_R.y + ROI_LU.y;
				FuncImg.DrawLine(pData, nWidth, nHeight, nBPP, P1, P2, RGB(255, 255, 0));
			}
		}
		//////////////////////////////////////////////////////////////////////////

		//  searching y_candidate
		//////////////////////////////////////////////////////////////////////////////
		int best_xposition;
		double max_peak = 0.0;
		vector <int> CenterPosY;
		shift = 5;
		int sh = _height - shift;
		int tolerance;
		int offest, area;
		offest = shift * _width;
		area = (2 * VehicleWidth + 1) * (2 * shift + 1);

		for (int c = 0; c < CenterPosX.size(); c++)
		{
			// 觀察中心線 ///////////////////////////////////////////////////////////////////////
			P1.x = CenterPosX[c] + ROI_LU.x;
			P1.y = ROI_LU.y;
			P2.x = P1.x;
			P2.y = ROI_RB.y;
			FuncImg.DrawLine(pData, nWidth, nHeight, 3, P1, P2, RGB(255, 0, 0));
			/////////////////////////////////////////////////////////////////////////////////////
			best_xposition = CenterPosX[c];

			//// 計算水平邊的強度
			int best_yposition = 0, y;
			j = 0; int preY = 0; max_peak = 0;

			FuncMe.Find_Y_Position(best_xposition, CenterPosY, dROI_GRADIENT_IntegralImage, dROI_GRAY_IntegralImage, GramW, ShadowHistogram, SmoothingW, _width, _height, VehicleWidth, VehicleHeight);
			// shadow detection

			vector <int> ShadowLine;

			j = 0;
			preY = 0;
			for (int k = shift; k < sh; k++)
			{
				if (ShadowHistogram[k] < 0.2) // shadow
				{
					if (k > preY + VehicleHeight || j == 0) // j=0 check whether it is the first peak 
					{
						ShadowLine.push_back(k); j++; preY = k;
					}
					else
					{
						y = ShadowLine[j - 1];
						if (ShadowHistogram[y] > ShadowHistogram[k]) // 兩個相鄰過近, 找最小的
						{
							ShadowLine[j - 1] = k;
							preY = k;
						}
					}

				}
			}
			CPoint LU, RB;
			for (int k = 0; k < ShadowLine.size(); k++)
			{
				//觀察 shadowLine/////////////////////////////////////////////////////////
// 				P1.x = CenterPosX[c] - VehicleWidth + ROI_LU.x;
// 				P2.x = CenterPosX[c] + VehicleWidth + ROI_LU.x;
// 				P1.y = P2.y = ROI_LU.y + ShadowLine[k];
// 				FuncImg.DrawLine(pData, nWidth, nHeight, nBPP, P1, P2, RGB(0, 255, 0));
				LU.y = ROI_LU.y + ShadowLine[k];
				RB.y = ROI_LU.y + ShadowLine[k] + 2* VehicleHeight;
				RB.x = CenterPosX[c] + VehicleWidth + ROI_LU.x;
				LU.x = CenterPosX[c] - VehicleWidth + ROI_LU.x;
				FuncImg.DrawRect(pData, nWidth, nHeight, nBPP, LU, RB, RGB(255, 0, 255));
				///////////////////////////////////////////////////////////////////////////
				// 陰影往上找可能的車輛
				int sline = ShadowLine[k], vw = 0, y, dis, h, mcounter, ew, y_tolerance, w_constraint;
				double ratio, eh, min;
				VEHICLE V;

				tolerance = VehicleWidth / 4; mcounter = 0;
				y_tolerance = 2 * VehicleHeight;
				w_constraint = VehicleWidth / 2;

				for (j = 0; j < MyCar.All_MatchPair_bin.size(); j++)
				{
					Center = MyCar.All_MatchPair_bin[j].CenterPoint;
					dis = abs(Center.x - CenterPosX[c]);
					h = sline - Center.y;
					ew = (MyCar.All_MatchPair_bin[j].Point_R.x - MyCar.All_MatchPair_bin[j].Point_L.x);
					if (dis < tolerance && Center.y < sline && h < y_tolerance && ew > w_constraint)
					{
						vw += ew;;
						mcounter++;
					}
				}

				if (mcounter > 2) vw /= mcounter;
				else continue;


				for (j = CenterPosY.size() - 1; j >= 0; j--)
				{
					y = CenterPosY[j];
					if (y < sline)
					{
						eh = sline - y;
						ratio = eh / vw;
						ew = eh * 1.6;
						if (ratio > 0.5 && ratio < 2 && ew > vw)
						{   // set vehicle information
							V.top = y; V.bottom = sline;
							if (HFlag)
								if (eh < EstimatedH[sline] * 0.8)
								{
									eh = EstimatedH[sline];
									V.top = sline - eh;
								}

							V.left = CenterPosX[c] - eh * 0.8;
							V.cx = CenterPosX[c];
							V.cy = (V.top + V.bottom) / 2;
							V.score = 0;
							if (V.left < 0) V.left = 0;
							V.right = CenterPosX[c] + eh * 0.8;
							if (V.right >= _width - 1) V.right = _width;
							tempVehicleList.push_back(V);

						}
					}
				}

				min = 100;
				for (j = 0; j < tempVehicleList.size(); j++)
				{
					ratio = (double)(tempVehicleList[j].right - tempVehicleList[j].left) / vw;
					if (min > ratio)
					{
						min = ratio;
						V = tempVehicleList[j];
					}
				}
				if (j > 0) VehicleCandidates.push_back(V);
				tempVehicleList.clear();

			}



			ShadowLine.clear();
			CenterPosY.clear();
		} // end for( i=0; i < Center....)

		// eliminate overlapping vehicle among different central lines
		if (CenterPosX.size() > 1 && VehicleCandidates.size() > 1)
		{
			int area;
			// voting 
			for (int i = 0; i < MyCar.All_MatchPair_bin.size(); i++)
			{
				int cx, cy;
				cx = MyCar.All_MatchPair_bin[i].CenterPoint.x;
				cy = MyCar.All_MatchPair_bin[i].CenterPoint.y;
				for (j = 0; j < VehicleCandidates.size(); j++)
				{
					if (cy > VehicleCandidates[j].top && cy < VehicleCandidates[j].bottom)
						if (abs(cx - VehicleCandidates[j].cx) < tolerance)
						{
							VehicleCandidates[j].score++;
							break;
						}
				}
			}

			// filtering 
			for (int i = 0; i < VehicleCandidates.size(); i++)
				for (int j = i + 1; j < VehicleCandidates.size(); j++)
				{
					area = CalculateOverlapArea(VehicleCandidates[i].left, VehicleCandidates[i].right, VehicleCandidates[i].top, VehicleCandidates[i].bottom,
						VehicleCandidates[j].left, VehicleCandidates[j].right, VehicleCandidates[j].top, VehicleCandidates[j].bottom);
					if (area > 1000)  // 重疊過大 
					{
						if (VehicleCandidates[i].score > VehicleCandidates[j].score)
						{ // eliminate the candidate with the smaller score 
							VehicleCandidates.erase(VehicleCandidates.begin() + j); j--;
						}
						else
						{
							VehicleCandidates.erase(VehicleCandidates.begin() + i);
							if (i > 0) i--;
							else { i = 0; break; }
						}
					}
				}
		}

		if (!HFlag) // V_histoy.size() < 30 )
			for (int i = 0; i < VehicleCandidates.size(); i++)
			{
				// 			P1.x =  VehicleCandidates[i].left + ROI_LU.x;
				// 			P1.y =  ROI_RU.y - VehicleCandidates[i].top;
				// 			P2.x =  VehicleCandidates[i].right + ROI_LU.x;
				// 			P2.y =  ROI_RU.y - VehicleCandidates[i].top;
				// 			FuncImg.DrawLine(p_pData, nWidth, nHeight,3, P1, P2,RGB(255,255,0));
				// 			P2.y =  ROI_RU.y - VehicleCandidates[i].bottom;
				// 			P2.x =  VehicleCandidates[i].left + ROI_LU.x;
				// 			FuncImg.DrawLine(p_pData, nWidth, nHeight,3, P1, P2,RGB(255,255,0));
				// 			P1.x =  VehicleCandidates[i].right + ROI_LU.x;
				// 			P1.y =  ROI_RU.y - VehicleCandidates[i].bottom;
				// 			FuncImg.DrawLine(p_pData, nWidth, nHeight,3, P1, P2,RGB(255,255,0));
				// 			P2.y =  ROI_RU.y - VehicleCandidates[i].top;
				// 			P2.x =  VehicleCandidates[i].right + ROI_LU.x;
				// 			FuncImg.DrawLine(p_pData, nWidth, nHeight,3, P1, P2,RGB(255,255,0));

				// for recording and leaning Vehicle Heights 
				// {if( V_histoy.size() < 30 )
				V_histoy.push_back(VehicleCandidates[i]);
				// }
			}

		if (!HFlag)
		{
			if (V_histoy.size() >= 30)
			{
				TrainingVehicleHeight(_height, 0, _height);
				HFlag = TRUE;
				V_histoy.clear();
			}
		}

		delete[] ShadowHistogram;
		delete[] GramW;
		delete[] SmoothingW;

		CenterPosX.clear();
		CenterPosY.clear();
		//	VehicleCandidates.clear();
	}


