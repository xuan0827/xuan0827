#pragma once

#include ".\adaboost\cv.h"
#include ".\adaboost\highgui.h"
#include ".\adaboost\cxcore.h"

class CAdaboostDetection
{
public:
	CAdaboostDetection(void);
	~CAdaboostDetection(void);


public:
	CvMemStorage* storage;
	CvHaarClassifierCascade* cascade;
	IplImage    *m_image;
	IplImage    *m_gray ;
	IplImage    *m_small_img;
    CString     m_strBodyCascade;


    RECT        *m_Rect;

	BITMAPINFOHEADER         m_bih;
	
public:
	RECT     *m_pObjectRect;
	int      m_nObjectNumber;
	char     m_cxml_name[128];
	BOOL     m_bInitial;
	char     *m_cptr;
	int      m_nBPP;
	int      m_nPW;
	float      m_nscale;

public:
   int         ObjectDetection(BYTE *pImageArray,int nWidth,int nHeight,CString strCascadeFile);
   int         detect_and_draw( IplImage* img );
   char*       CStringToCharArray(CString str);
   RECT*       GetObjectRect();
};
