#include "StdAfx.h"
#include "AdaboostDetection.h"

#define  Safe_DeleteArr(ptr) { if(ptr){	delete [] ptr;	ptr	= NULL;	}}

CAdaboostDetection::CAdaboostDetection(void)
{
	storage         = NULL;
	cascade         = NULL;
	m_pObjectRect   = NULL;

	m_image         = NULL;
	m_gray          = NULL;
	m_small_img     = NULL;

	m_nObjectNumber = 0;
	m_bInitial      = FALSE;
	m_cptr          = NULL;
	m_nscale        = 1.2;
	m_Rect          = NULL;

}

CAdaboostDetection::~CAdaboostDetection(void)
{
  if(m_bInitial)
  {
	cvReleaseImage(&m_image);
	cvReleaseImage(&m_gray);
	cvReleaseImage(&m_small_img);
	cvFree( &storage );
  }
  Safe_DeleteArr(m_cptr);
  Safe_DeleteArr(m_Rect);
}



int CAdaboostDetection::ObjectDetection(BYTE *pImageArray,int nWidth,int nHeight,CString strCascadeFile)
{
    
   if(!m_bInitial) 
   {
		memset( &m_bih, 0, sizeof( m_bih ) );
		m_bih.biSize     = sizeof( m_bih );
		m_bih.biWidth    = nWidth;
		m_bih.biHeight   = nHeight;
		m_bih.biPlanes   = 1;
		m_bih.biBitCount = 24;

		m_nBPP = m_bih.biBitCount / 8;
		m_nPW = (nWidth * m_nBPP + 3) >> 2 << 2;

		m_image = cvCreateImage( cvSize( nWidth, nHeight ), IPL_DEPTH_8U, m_nBPP );
		m_gray = cvCreateImage( cvSize( nWidth, nHeight), 8, 1 );
		m_small_img = cvCreateImage( cvSize( cvRound(nWidth/m_nscale),
			cvRound(nHeight/m_nscale)),
			8, 1 );
		cvZero( m_image );
		cvZero( m_gray );
		cvZero( m_small_img );
        char *p = CStringToCharArray(strCascadeFile);

		strcpy( m_cxml_name,p );
		cascade = (CvHaarClassifierCascade*)cvLoad( m_cxml_name, 0, 0, 0 );
		storage = cvCreateMemStorage(0);
		m_bInitial = TRUE;
    }

  if(m_bInitial)
   {
    for( int i=0; i < nHeight ; i++ )
	   for( int j=0; j < m_nPW; j+=m_nBPP )
	   {
		   m_image->imageData[ i*m_nPW + j ]   = pImageArray[ i*m_nPW + j ];
		   m_image->imageData[ i*m_nPW + j+1 ] = pImageArray[ i*m_nPW + j+1 ];
		   m_image->imageData[ i*m_nPW + j+2 ] = pImageArray[ i*m_nPW + j+2 ];
	   }
	   	m_nObjectNumber = detect_and_draw( m_image);

   }


	return m_nObjectNumber;
}

char* CAdaboostDetection::CStringToCharArray(CString str)
{
#ifdef _UNICODE
	LONG len;
	len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	m_cptr = new char [len+1];
	memset(m_cptr,0,len + 1);
	WideCharToMultiByte(CP_ACP, 0, str, -1, m_cptr, len + 1, NULL, NULL);
#else
	m_cptr = new char [str.GetAllocLength()+1];
	sprintf(m_cptr,_T("%s"),str);
#endif
	return m_cptr;

}

int  CAdaboostDetection::detect_and_draw( IplImage* img )
{
	static CvScalar colors[] = 
	{
		{{0,0,255}},
		{{0,128,255}},
		{{0,255,255}},
		{{0,255,0}},
		{{255,128,0}},
		{{255,255,0}},
		{{255,0,0}},
		{{255,0,255}}
	};

	int i;
	cvZero( m_gray );
	cvZero( m_small_img );
	cvCvtColor( img, m_gray, CV_BGR2GRAY );

	cvResize( m_gray, m_small_img, CV_INTER_LINEAR );
	cvEqualizeHist( m_small_img, m_small_img );
	cvClearMemStorage( storage );

	if( cascade )
	{
		double t = (double)cvGetTickCount();
		CvSeq* faces = cvHaarDetectObjects( m_small_img, cascade, storage,1.1, 2, 0,cvSize(30, 30) );//CV_HAAR_DO_CANNY_PRUNING
			//°»´ú
		t = (double)cvGetTickCount() - t;
		printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.));

        Safe_DeleteArr(m_Rect);
		  m_Rect = new RECT [faces->total];
		  



		for( i = 0; i < (faces ? faces->total : 0); i++ )//µe®Ø
		{
			CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
			CvPoint center;
			int radius;
			center.x = cvRound((r->x + r->width*0.5)*m_nscale);
			center.y = cvRound((r->y + r->height*0.5)*m_nscale);
			radius = cvRound((r->width + r->height)*0.25*m_nscale);

            m_Rect[i].left  = center.x-radius;
            m_Rect[i].right = center.x+radius;
            m_Rect[i].top   = center.y-radius;
			m_Rect[i].bottom = center.y+radius;

	//		cvCircle( img, center, radius, colors[i%8], 3, 8, 0 );
		}
		return faces->total;
	}
   return 0;
}
RECT*   CAdaboostDetection::GetObjectRect()
{
   return m_Rect;

}