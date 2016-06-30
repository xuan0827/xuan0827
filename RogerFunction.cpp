// RogerFunction.cpp : implementation file
//

#include "stdafx.h"
#include "Video.h"
#include "RogerFunction.h"
#include "ImgProcess.h"
#include <math.h>
#include <iostream>
#include <string>
#include <fstream>
#include <limits>
#include <windows.h>
#include <algorithm>
#include "svm.h"
#include "VideoDoc.h"
#include <vector>
// #include "Myipoint.h"
// #include "MySURF.h"
/*#include "MyFasthessian.h"*/
#include "AdaboostDetection.h"
#include <iomanip>
//////////////////////////////////////////////////////////////////////////
// #include "fasthessian.h"
// #include "MySURF2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum {SCALESPACE, NOSCALESPACE} ;
#define NBINS 36
#define FLT_EPSILON     1.192092896e-07F        /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define  round(x)((int)(x + 0.5))
#define log2(x) (log(x)/log(2))

const int max_iter = 5 ;
const double win_factor = 1.5 ;

extern "C"
{
	int PASCAL WinMain(HINSTANCE inst,HINSTANCE dumb,LPSTR param,int show);
};

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CRogerFunction

IMPLEMENT_DYNCREATE(CRogerFunction, CDocument)

BOOL CRogerFunction::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

// CRogerFunction::~CRogerFunction()
// {
// }


BEGIN_MESSAGE_MAP(CRogerFunction, CDocument)
	//{{AFX_MSG_MAP(CRogerFunction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRogerFunction diagnostics

#ifdef _DEBUG
void CRogerFunction::AssertValid() const
{
	CDocument::AssertValid();
}

void CRogerFunction::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRogerFunction serialization

void CRogerFunction::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

void CRogerFunction::Integral_Image_Use( double *IImage, int s_w, CPoint LB, CPoint RU, float &Result)
{
	Result = IImage[ (RU.y)     * s_w + (RU.x)    ] - IImage[ (LB.y-1)   * s_w + (RU.x)    ] -IImage[ (RU.y)     * s_w + (LB.x-1)  ] + IImage[ (LB.y-1)   * s_w + (LB.x-1)  ];
}

void CRogerFunction::HORILINE( CARHEADINFO &Car, int *In_ImageNpixel, int s_w, int s_h, int s_bpp, BYTE *output, int nWidth, int nHeight ) 
{
	CImgProcess FuncImg;
	int i, j, k;
	CPoint P1, P2;

	float *GramSum = new float[s_w];
	memset( GramSum, 0, sizeof(float)*s_w);	
	float *GramT = new float[s_w];
	memset( GramT, 0, sizeof(float)*s_w);

	int *p_In_ImageNpixel = In_ImageNpixel;
	//////////////////////////////////////////////////////////////////////////
	int nH =  abs(Car.nY_Candidate_bin[0] - Car.nY_Candidate_bin[Car.nY_Candidate_bin.size()-1])  ;

	for (i=0; i<s_w; i++ )
	{
		for (j=Car.nY_Candidate_bin[0]; j<Car.nY_Candidate_bin[Car.nY_Candidate_bin.size()-1]; j++)
		{
			GramT[i] += ((float)In_ImageNpixel[j*s_w+i]/100);
		}
	}

	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_w-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
		}
	} 

	//Find Local Maximum
	float MaxV;
	k = 15;
	for (i=0; i<s_w-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}


	for (i=0; i<s_w; i++)
	{
		if ( GramSum[i] > 110 && abs(Car.nCenterLinePosX - i) > (s_w*0.3) )
		{
			if ( i > Car.nCenterLinePosX )
			{
				Car.nX_Right_Candidate_bin.push_back(i);
			}
			else
			{
				Car.nX_Left_Candidate_bin.push_back(i);
			}

		}
	}

	delete []GramSum;
	delete []GramT;
}

void CRogerFunction::I_GrayLevel(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw)
{
    int i,j,k;
    // Gray Level Processing
	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
			out_image[k]   = int((in_image[k]+in_image[k+1]+in_image[k+2])/3); //調成灰階 8bits
			out_image[k+1] = int((in_image[k]+in_image[k+1]+in_image[k+2])/3); //調成灰階 8bits
			out_image[k+2] = int((in_image[k]+in_image[k+1]+in_image[k+2])/3); //調成灰階 8bits
		}		
	} 
}
//本函式可將全彩圖片,轉成灰階 3 bytes to 1 bytes
void CRogerFunction::I_GrayLevel3_1(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw)
{
    int i,j,k;
    // Gray Level Processing
	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_w) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
			out_image[j]   = (in_image[k]+in_image[k+1]+in_image[k+2])/3;
		}		
	} 
}

void CRogerFunction::I_Negative(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw)
{
    int i,j,k;
	
    // Gray Level Processing
	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
			out_image[k]   = 255 - int((in_image[k]+in_image[k+1]+in_image[k+2])/3); //調成灰階 8bits
			out_image[k+1] = 255 - int((in_image[k]+in_image[k+1]+in_image[k+2])/3); //調成灰階 8bits
			out_image[k+2] = 255 - int((in_image[k]+in_image[k+1]+in_image[k+2])/3); //調成灰階 8bits
		}		
	} 
}
//本函式先將全彩或灰階圖片,轉成灰階圖片,再利用3x3 matrixs (暫用Laplacian)做邊緣偵測
void CRogerFunction::I_EdgeDetection(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	BYTE *maskval;
	int *temp1,*temp2;
	int i,j,k;
	int e[9]={	-1,-1,-1,
				-1, 8,-1,
				-1,-1,-1
	};

	maskval=out_image;
	
	temp1=new int[s_w*s_h];
	temp2=new int[s_w*s_h];
	
	// gray level process
	
	for (i=0 ; i<s_h ; i++, in_image+=s_Effw)
		for (j=0, k=0 ; j<s_w ; j++, k+=3)
			temp1[i*s_w+j]= int((in_image[k]+in_image[k+1]+in_image[k+2])/3);
	// mask operation
		for (i=1;i<s_h-1;i++)
			for (j=1;j<s_w-1;j++)
				temp2[i*s_w+j] = 
									e[0] * temp1[((i-1)*s_w)+(j-1)]+
									e[1] * temp1[((i-1)*s_w)+(j)]+
									e[2] * temp1[((i-1)*s_w)+(j+1)]+
									e[3] * temp1[i*s_w+(j-1)]+
									e[4] * temp1[i*s_w+j]+
									e[5] * temp1[i*s_w+(j+1)]+
									e[6] * temp1[((i+1)*s_w)+(j-1)]+
									e[7] * temp1[((i+1)*s_w)+j]+
									e[8] * temp1[((i+1)*s_w)+(j+1)];
			
// edge setting
			for (i=0; i<s_h ; i++,  maskval+=s_Effw)	
				for (j=0, k=0; j<s_w; j++, k+=3)
				{ 
					if (j<s_w-1)
					{
						
						/*if ((temp2[i*w+j]+temp2[i*w+j+1]<temp2[i*w+j])||(temp2[i*w+j]-temp2[i*w+j+1]>temp2[i*w+j]))*/
						if(temp2[i*s_w+j] < -20) 
						{	
							maskval[k    ] = 255;
							maskval[k + 1] = 255;
							maskval[k + 2] = 255;
						}
						else
						{
							maskval[k    ] = 0;
							maskval[k + 1] = 0;
							maskval[k + 2] = 0;	
						}
					}
					else
					{
						maskval[k    ] = 0;
						maskval[k + 1] = 0;
						maskval[k + 2] = 0;	
					}
			}

	delete []temp1;
	delete []temp2;
}

//本函式Erosion侵蝕處理
void CRogerFunction::I_Erosion(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	BYTE *maskval;
	int *temp1,*temp2;
	int i,j,modify;
	int e[3]={ 0, 0, 0 }; // Erosion's Structure Element
	int row, column;
    if(s_Effw == 3*s_w )        //每個pixel有三個bytes-> RGB, 實地測試 必須是4的倍數
	{
		row= s_Effw;
		modify=0;
	}
	else
	{
        row = s_Effw;
		modify = s_Effw - (3*s_w);
	}
	column =  s_h;

	maskval=out_image;
	temp1=new int[row * column];  //把 int temp1 及 temp2 當成判斷暫存區
	temp2=new int[row * column];
	
	for (i=0; i<column; i++)           //i控制行高 
		for(j=0; j< row; j++)          //j控制列寬
		{
		temp1[ i * row + j ] = in_image[ i * row + j];  //用temp1接受 in_image 成為一個影像暫存區
		temp2[ i * row +j  ] = 0;    //temp2全白
		}  
	for (i=1; i<column-1; i++)           //i控制行高 
		for(j=3; j< row-3; j+=3)        //j控制列寬
		{
			if (temp1[ (i-1)   *row+j-3] > 100 && temp1[ (i-1)   *row+j] > 100 && temp1[ (i-1)   *row+j+3] > 100 && 
				temp1[ i       *row+j-3] > 100 && temp1[ i       *row+j] > 100 && temp1[ i       *row+j+3] > 100 &&
				temp1[ (i+1)   *row+j-3] > 100 && temp1[ (i+1)   *row+j] > 100 && temp1[ (i+1)   *row+j+3] > 100)
				{
				temp2[ i   *row+j    ] = 255;
				temp2[ i   *row+j  +1] = 255;
				temp2[ i   *row+j  +2] = 255;
				}
			else
			{
				temp2[ i   *row+j    ] = 0;
				temp2[ i   *row+j  +1] = 0;
				temp2[ i   *row+j  +2] = 0;
			}
		}

	for (i=0; i<column; i++)           //i控制行高 
		for(j=0; j< row; j++)         //j控制列寬
		{
			out_image[ i * row - modify + j ] = temp2[ i * row + j];  

		}  
	delete []temp1;
	delete []temp2;
}
void CRogerFunction::I_Dilation(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	BYTE *maskval;
	int *temp1,*temp2;
	int i,j,modify;
	int row, column;
    if(s_w >= s_h )        //每個pixel有三個bytes-> RGB,  實地測試 必須是4的倍數
		{
			row= s_Effw;
			modify=0;
	}
	else
		{
			row = s_Effw;
			modify = s_Effw - (3*s_w);
	}
	column =  s_h;
	
	maskval=out_image;
	temp1=new int[row * column];  //把 int temp1 及 temp2 當成判斷暫存區
	temp2=new int[row * column];
	
	for (i=0; i<column; i++)           //i控制行高 
		for(j=0; j< row; j++)          //j控制列寬
		{
			temp1[ i * row + j ] = in_image[ i * row + j];  //用temp1接受 in_image 成為一個影像暫存區
			temp2[ i * row +j  ] = 0;
		}  
		for (i=1; i<column-1; i++)           //i控制行高 
			for(j=3; j< row-3; j+=3)        //j控制列寬
			{
				if (temp1[ i       *row+j] > 100 )
				{
					temp2[ (i-1)   *row+j-3    ] = 255;
					temp2[ (i-1)   *row+j-3  +1] = 255;
					temp2[ (i-1)   *row+j-3  +2] = 255;

					temp2[ (i-1)   *row+j    ] = 255;
					temp2[ (i-1)   *row+j  +1] = 255;
					temp2[ (i-1)   *row+j  +2] = 255;

					temp2[ (i-1)   *row+j+3    ] = 255;
					temp2[ (i-1)   *row+j+3  +1] = 255;
					temp2[ (i-1)   *row+j+3  +2] = 255;
//....

					temp2[ i       *row+j-3    ] = 255;
					temp2[ i       *row+j-3  +1] = 255;
					temp2[ i       *row+j-3  +2] = 255;

					temp2[ i   *row+j    ] = 255;
					temp2[ i   *row+j  +1] = 255;
					temp2[ i   *row+j  +2] = 255;

					temp2[ i       *row+j+3    ] = 255;
					temp2[ i       *row+j+3  +1] = 255;
					temp2[ i       *row+j+3  +2] = 255;
//........
					temp2[ (i+1)   *row+j-3    ] = 255;
					temp2[ (i+1)   *row+j-3  +1] = 255;
					temp2[ (i+1)   *row+j-3  +2] = 255;

					temp2[ (i+1)   *row+j    ] = 255;
					temp2[ (i+1)   *row+j  +1] = 255;
					temp2[ (i+1)   *row+j  +2] = 255;

					temp2[ (i+1)   *row+j+3    ] = 255;
					temp2[ (i+1)   *row+j+3  +1] = 255;
					temp2[ (i+1)   *row+j+3  +2] = 255;
				}
			}
			
			for (i=0; i<column; i++)           //i控制行高 
				for(j=0; j< row; j++)         //j控制列寬
				{
					out_image[ i * row - modify + j ] = temp2[ i * row + j];  
					
				}  
	delete []temp1;
	delete []temp2;
}
void CRogerFunction::I_GaussianSmooth3(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw, double in_sigma)
{
	//initial variable....................................................................
	int i;
	int nWidth	= s_w;
	int nHeight = s_h;
	int nEffw = s_Effw; 
	int k;
	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMulR, dDotMulG, dDotMulB;
	double dWeightSum;
	double *pdTmpR, *pdTmpG, *pdTmpB;
	int nCenter;
	double  dDis; 
	double  dValue; 
	double  dSum  ;
//..MakeGauss Smooth
	dSum = 0 ; 
	nWindowSize = 1 + 2 * ceil(3 * in_sigma);
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmpR		= new double[nWidth*nHeight];
	pdTmpG		= new double[nWidth*nHeight];
	pdTmpB		= new double[nWidth*nHeight];

	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(in_sigma*in_sigma)) / (sqrt(2 * PI) * in_sigma ); //gaussian 先作x軸向
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		(pdKernel)[i] /= dSum;
	}
//..End MakeGauss........	

	nHalfLen = nWindowSize / 2;

	for(y=0; y<nHeight; y++)
	{
		for(x=0, k=0; x<nWidth; x++, k+=3)
		{
			dDotMulR = 0;
			dDotMulG = 0;
			dDotMulB = 0;

			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMulR += (double)in_image[y*nWidth*3 + (k  )] * pdKernel[nHalfLen+i];
					dDotMulG += (double)in_image[y*nWidth*3 + (k+1)] * pdKernel[nHalfLen+i];
					dDotMulB += (double)in_image[y*nWidth*3 + (k+2)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmpR[y*nWidth + x] = dDotMulR/dWeightSum ;
			pdTmpG[y*nWidth + x] = dDotMulG/dWeightSum ;
			pdTmpB[y*nWidth + x] = dDotMulB/dWeightSum ;
		}
	}
	
	for(x=0, k=0; x<nWidth; x++, k+=3)
	{
		for(y=0; y<nHeight; y++ )
		{
			dDotMulR = 0;
			dDotMulG = 0;
			dDotMulB = 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMulR += (double)pdTmpR[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dDotMulG += (double)pdTmpG[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dDotMulB += (double)pdTmpB[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			out_image[y*nWidth*3 + k  ] = (unsigned char)(int)dDotMulR/dWeightSum ;
			out_image[y*nWidth*3 + k+1] = (unsigned char)(int)dDotMulG/dWeightSum ;
			out_image[y*nWidth*3 + k+2] = (unsigned char)(int)dDotMulB/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmpR;
	pdTmpR = NULL;
	delete []pdTmpG;
	pdTmpG = NULL;
	delete []pdTmpB;
	pdTmpB = NULL;

}
void CRogerFunction::I_GaussianSmooth1(BYTE *in_image, BYTE *out_image, int s_w, int s_h, double in_sigma)
{
	//initial variable....................................................................
	int i;
	int nWidth	= s_w;
	int nHeight = s_h;
	
	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double dWeightSum;
	double *pdTmp;
	int nCenter;
	double  dDis; 
	double  dValue; 
	double  dSum  ;
	//..MakeGauss Smooth
	dSum = 0 ; 
	//可變式窗
	nWindowSize = 1 + 2 * ceil(3 * in_sigma);
	//fix windowsize
// 	nWindowSize = 5;
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(in_sigma*in_sigma)) / (sqrt(2 * PI)* in_sigma  );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		pdKernel[i] = pdKernel[i] / dSum;
	}
	//..End MakeGauss........	
	
	nHalfLen = nWindowSize / 2;
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)in_image[y*nWidth + (i+x)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul/dWeightSum ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			out_image[y*nWidth + x] = (unsigned char)(int)dDotMul/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
	
}
void CRogerFunction::I_GaussianSmoothDBL(double *in_image, double *out_image, int s_w, int s_h, double in_sigma)
{
	int i;
	int nWidth	= s_w;
	int nHeight = s_h;
	
	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double dWeightSum;
	double *pdTmp;
	int nCenter;
	double  dDis; 
	double  dValue; 
	double  dSum  ;
	//..MakeGauss Smooth
	dSum = 0 ; 
	nWindowSize = 1 + 2 * ceil(3 * in_sigma);
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(in_sigma*in_sigma)) / (sqrt(2 * PI)* in_sigma  );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		pdKernel[i] = pdKernel[i] / dSum;
	}
	//..End MakeGauss........	
	
	nHalfLen = nWindowSize / 2;
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)in_image[y*nWidth + (i+x)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul/dWeightSum ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			out_image[y*nWidth + x] = (double)dDotMul/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
}
void CRogerFunction::I_Gaussian(double in_sigma)
{
	int i;
	int nWindowSize = 1 + 2 * ceil( 3 * in_sigma) ;

	int nWidth	= nWindowSize;
	int nHeight = nWindowSize;
	
	int x,y, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double *pdTmp;
	int nCenter;
	double  dDis; 
	double  dValue; 
	double  dSum  ;
	//..MakeGauss Smooth
	dSum = 0 ; 
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(in_sigma*in_sigma)) / ( 2 * PI * in_sigma * in_sigma  );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		pdKernel[i] = pdKernel[i] / dSum;
	}
	//..End MakeGauss........	
	
	nHalfLen = nWindowSize / 2;
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)/*in_image[y*nWidth + (i+x)] **/ pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
				}
			}
			gpMsgbar->ShowMessage("[%.3f]", (double)dDotMul ) ;
		}
		gpMsgbar->ShowMessage("\n");
	}
	gpMsgbar->ShowMessage("WindowSize: [%d]", nWindowSize);
	gpMsgbar->ShowMessage("\n");
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
}
	void CRogerFunction::I_GaussianSmoothDBL_STIP(double *in_image, double *out_image, int s_w, int s_h, double in_sigma, double T_interval)
{
	int i;
	int nWidth	= s_w;
	int nHeight = s_h;
	
	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double dWeightSum;
	double *pdTmp;
	int nCenter;
	double  dDis; 
	double  dValue; 
	double  dSum  ;
	//..MakeGauss Smooth
	dSum = 0 ; 
	nWindowSize = 1 + 2 * ceil( 3 * in_sigma) ;
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(in_sigma*in_sigma)) / ( 2 * PI * in_sigma * in_sigma  );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		pdKernel[i] = pdKernel[i] / dSum;
	}
	//..End MakeGauss........	
	
	nHalfLen = nWindowSize / 2;
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)in_image[y*nWidth + (i+x)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul/dWeightSum ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			out_image[y*nWidth + x] = (double)dDotMul/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
}
void CRogerFunction::I_SkinExtractionBW(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	BYTE *maskval;
    int i,j,k;
	float	r, g, up, bottom, cw, lips;
	float ir, ig, ib;
	
	maskval=out_image;
	
	//..........................................................................
	for (i=0; i<s_h ;i++, in_image+=s_Effw, maskval+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
			ir=in_image[k+2];
			ig=in_image[k+1];
			ib=in_image[k];
			r=ir/(ir+ig+ib);
			g=ig/(ir+ig+ib);
			up    =-1.3767*r*r+1.0743*r+0.25;
			bottom=-0.776*r*r+0.5601*r+0.18;
			cw=(r-0.33)*(r-0.33)+(g-0.33)*(g-0.33);
			lips=-0.776*r*r+0.5601*r+0.18;
			if(g<up && g>bottom && cw>0.001 && (ir-ig)>=15
				/*||(g<=lips && cw>0.001 && (ir-ig)>=15)*/)
			{
				//					maskval[k+2]=in_image[k+2];     //彩色輸出
				//					maskval[k+1]=in_image[k+1];
				//					maskval[k]=in_image[k];
				maskval[k+2]=255;					//黑白輸出
				maskval[k+1]=255;
				maskval[k]=255;					
			}
			else
			{
				maskval[k+2]=0;
				maskval[k+1]=0;
				maskval[k]=0;
			}
		}
	}		
	
}
void CRogerFunction::I_ConnectedComponent(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw) 
{
	BYTE *maskval;
	int *temp1,*temp2;
	int i,j,k,modify,Label=0, Label1=0, temp,flag=0, segment=0,cnt=300;
	int row, column, marka[200],markb[200];
    if(s_Effw == 3*s_w )        //每個pixel有三個bytes-> RGB, 實地測試 必須是4的倍數
	{
		row= s_Effw;
		modify=0;
	}
	else
	{
        row = s_Effw;
		modify = s_Effw - (3*s_w);
	}
	column =  s_h;
	
	maskval=out_image;
	temp1=new int[row * column];  //把 int temp1 及 temp2 當成判斷暫存區
	temp2=new int[row * column];
	for (i=0;i<200;i++) 
	{
	marka[i] = 0;
	markb[i] = 0;
	}
	for (i=0; i<column; i++)           //i控制行高 
		for(j=0; j< row; j++)          //j控制列寬
		{
			temp2[i*row+j] = 0;
			if(in_image[i*row+j] > 50)
				temp1[ i * row + j ] = 1;  //將二值化後的區塊都變成1
			else
				temp1[ i * row +j  ] = 0;
			
		if (i==0 || column-1 == 0) temp1[i*row+j] = 0;
		if (j==0 || row-1 == 0)    temp1[i*row+j] = 0; 
		}

//....Pass 1......................................................		
	for (i=1; i<column-1; i++)           //i控制行高 
		for(j=3; j< row-3; j++)          //j控制列寬
		{
			if (temp1[i*row+j] == 1 )
			{
				if (temp1[(i-1)*row+j]==0 && temp1[i*row+j-1]==0)
				{
					Label = Label + 1;
					temp2[i*row+j] = Label;
					marka[temp2[i*row+j]] = Label;
				}
				if (temp1[(i-1)*row+j]==1 && temp1[i*row+j-1]==0)
				{
					temp2[i*row+j] = temp2[(i-1)*row+j]; 
				}
				if (temp1[(i-1)*row+j]==0 && temp1[i*row+j-1]==1)
				{
					temp2[i*row+j] = temp2[i*row+j-1]; 
				}
				if ((temp1[(i-1)*row+j]==1 && temp1[i*row+j-1]==1 )&& (temp2[(i-1)*row+j] == temp2[i*row+j-1]))
				{
					temp2[i*row+j] = temp2[(i-1)*row+j]; 
				}
				if ((temp1[(i-1)*row+j]==1 && temp1[i*row+j-1]==1) && (temp2[(i-1)*row+j] != temp2[i*row+j-1]))
				{
					temp2[i*row+j] = temp2[(i-1)*row+j];
//					marka[temp2[i*row+j-1]] = temp2[(i-1)*row+j];
					marka[temp2[i*row+j-1]] = marka[temp2[(i-1)*row+j]];
				}
			}
		}
//.....Pass 2 Merge Classes................................

//更新索引號
		for (i=1; i<=Label; i++) markb[i] = marka[i];

			for (i=1; i<Label; i++)
				for(j=1; j<Label; j++)
				{
					if (markb[j] <= markb[j+1])
					{
						if (markb[j] == markb[j+1]) 
						{
							markb[j+1] = 0;
						}
						temp = markb[j];
						markb[j] = markb[j+1];
						markb[j+1] = temp;
					}
				}
				
				for (i=1;i<=Label;i++)
				{
					if (markb[i] != 0)
					{
						cnt=cnt++;
						for (j=1;j<=Label;j++)
						{
							if(marka[j] == markb[i]) marka[j] = cnt;
						}
					}
				}

//....查表轉換 markb 輸出.給 temp2.................................
			for (i=0; i<column; i++)           //i控制行高 
				for(j=0; j< row; j++)          //j控制列寬
				{
					for(k=1; k<=Label; k++)
					{
						if (temp2[i*row+j] == k )
						{
							temp2[i*row+j] = marka[k];
						}
					}
				}
//.........................................................
	for (i=0; i<s_h ;i++, temp2+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
		if(temp2[k] != 0)
		{
			if(temp2[k]  == 301 )
			{
				out_image[k]   = 255;
				out_image[k+1] = 0;
				out_image[k+2] = 255;
			}
			else if(temp2[k]  == 302 )
			{
				out_image[k]   = 0;
				out_image[k+1] = 255;
				out_image[k+2] = 255;
			}
			else if(temp2[k]  == 303 )
			{
				out_image[k]   = 0;
				out_image[k+1] = 255;
				out_image[k+2] = 155;
			}
			else if(temp2[k]  == 304 )
			{
				out_image[k]   = 0;
				out_image[k+1] = 155;
				out_image[k+2] = 255;
			}
			else if(temp2[k]  == 305 )
			{
				out_image[k]   = 0;
				out_image[k+1] = 155;
				out_image[k+2] = 155;
			}
			else if(temp2[k]  == 306 )
			{
				out_image[k]   = 115;
				out_image[k+1] = 0;
				out_image[k+2] = 125;
			}
			else if(temp2[k]  == 307 )
			{
				out_image[k]   = 115;
				out_image[k+1] = 255;
				out_image[k+2] = 0;
			}
			else if(temp2[k]  == 308 )
			{
				out_image[k]   = 125;
				out_image[k+1] = 5;
				out_image[k+2] = 155;
			}
			else if(temp2[k]  == 309 )
			{
				out_image[k]   = 100;
				out_image[k+1] = 200;
				out_image[k+2] = 15;
			}
			else
			{
				out_image[k]   = 245;
				out_image[k+1] = 155;
				out_image[k+2] = 15;			
			}
		}
		else
		{
			out_image[k]   = 0;
			out_image[k+1] = 0;
			out_image[k+2] = 0;
		}
		}		
	}  

	delete []temp1;
	delete []temp2;

}

void CRogerFunction::I_AutoThreshold(BYTE *in_image, BYTE *out_image, int s_w, int s_h) 
{
    int i, j, t,Judge=0, AutoT;
	float q1=0, q2=0, u1=0, u2=0, delta_1=0, delta_2=0, delta_w=0;
	BYTE *in_temp1;
	int Histogram[256];
	
	in_temp1 = in_image;
	
	for(i=0; i<=255; i++) Histogram[i]=0;
	
	for (i=0; i<s_h ;i++)
	{
		for (j=0; j<s_w ;j++)
		{
			Histogram[in_image[i*s_w+j]] = Histogram[in_image[i*s_w+j]] +1;
		}		
	}  
	for(t=50; t <=200; t++)
	{
		//......................................
		for(i=0; i < t; i++)
		{
			q1 = q1 + Histogram[i];
		}
		for(i=t; i <= 255; i++)
		{
			q2 = q2 + Histogram[i];	
		}
		// 	cnt = q1 + q2;
		// 	q1 = q1 / cnt;
		// 	q2 = q2 / cnt;
		//......................................
		for(i=0; i < t; i++)
		{
			u1 = u1 + (i*Histogram[i])/q1;
		}
		for(i=t; i <= 255; i++)
		{
			u2 = u2 + (i*Histogram[i])/q2;
		}
		//......................................
		i = q1*(1-q1)*(u1-u2);
		if(i > Judge) 
		{
			Judge = i;
			AutoT = t;
		}
	}	
	for (i=0; i<s_h ;i++)
	{
		for (j=0; j<s_w ;j++)
		{
			if(in_image[i*s_w+j] >= AutoT)
				out_image[i*s_w+j]   = 255;
			else
				out_image[i*s_w+j]   = 0;
		}		
	} 
}
// void CRogerFunction::I_TraceEdge (CPoint StartPoint, BYTE *pUnchEdge, int nWidth, int nHeight, int Class, int &PNum) 
// {
// 	//八個方向掃描
// 	int xNb[8] = {1, 1, 0,-1,-1,-1, 0, 1} ;
// 	int yNb[8] = {0, 1, 1, 1,0 ,-1,-1,-1} ;
// 	
// 	int yy ;
// 	int xx ;
// 	CPoint EdgePoint;
// 
// 	int k  ;
// 	int i, j;
// 	PNum++;
// 	for(k=0; k<8; k++)
// 	{
// 		EdgePoint.y = StartPoint.y + yNb[k] ;
// 		EdgePoint.x = StartPoint.x + xNb[k] ;
// 		if(pUnchEdge[EdgePoint.y * nWidth + EdgePoint.x] == 255  )
// 		{
// 			pUnchEdge[EdgePoint.y * nWidth+ EdgePoint.x] = 200 ;
// 			TraceEdge( EdgePoint, pUnchEdge, nWidth, nHeight, Class, PNum);
// 		}
// 	}
// }
void CRogerFunction::I_TraceEdgeMultiEndPoint (CPoint &StartPoint, vector<EdgeInfo> *vi, BYTE *pUnchEdge, int nWidth, int nHeight, int &PNum, int &Seq, int &TotPx)
{
	//八個方向掃描
	int xNb[8] = {1, 1, 0,-1,-1,-1, 0, 1} ;
	int yNb[8] = {0, 1, 1, 1,0 ,-1,-1,-1} ;
	
	CPoint EdgePoint;
	CPoint EndPoint;
	int k  ;
	int Flag = 0;

	EdgeInfo EdgeInformation00;
	EndPoint.x = StartPoint.x;
	EndPoint.y = StartPoint.y;
	TotPx++;

	for(k=0; k<8; k++)
	{
		EdgePoint.y = StartPoint.y + yNb[k] ;
		EdgePoint.x = StartPoint.x + xNb[k] ;
		if(pUnchEdge[EdgePoint.y * nWidth + EdgePoint.x] == 255   )
		{
			Flag = 1;
			pUnchEdge[EdgePoint.y * nWidth+ EdgePoint.x] = 200 ;
			I_TraceEdgeMultiEndPoint( EdgePoint, vi, pUnchEdge, nWidth, nHeight, PNum, Seq, TotPx);
		}
	}
	if ( Flag == 0 )
	{
		EdgeInformation00.End_P.x = EndPoint.x;
		EdgeInformation00.End_P.y = EndPoint.y;
		EdgeInformation00.Count = PNum;
		EdgeInformation00.Seq = Seq;
		EdgeInformation00.TotPx = TotPx;
		vi->push_back(EdgeInformation00);
		Seq++;
	}
}
void CRogerFunction::I_TraceEdge (CPoint &StartPoint, CPoint &EndPoint, BYTE *pUnchEdge, BYTE *out_image, int nWidth, int nHeight, int &PNum)
{
	//八個方向掃描
	int xNb[8] = {1, 1, 0,-1,-1,-1, 0, 1} ;
	int yNb[8] = {0, 1, 1, 1,0 ,-1,-1,-1} ;
	
	int k;
	CPoint EdgePoint;
	CPoint TmpPoint;
	int Flag = 0;
	PNum++;
	EndPoint.x = StartPoint.x;
	EndPoint.y = StartPoint.y;

	for(k=0; k<8; k++)
	{
		EdgePoint.y = StartPoint.y + yNb[k] ;
		EdgePoint.x = StartPoint.x + xNb[k] ;
		if(pUnchEdge[EdgePoint.y * nWidth + EdgePoint.x] == 255 && Flag == 0 )
		{
			Flag = 1;
			pUnchEdge[EdgePoint.y * nWidth+ EdgePoint.x] = 200 ;
			I_TraceEdge( EdgePoint, EndPoint, pUnchEdge, out_image, nWidth, nHeight, PNum);
		}
	}
	if ( Flag == 0 )
	{
		out_image[EndPoint.y * nWidth + EndPoint.x] = 255 ;
	}
}
void CRogerFunction::I_TraceEdge (CPoint &StartPoint, CPoint &EndPoint, BYTE *pUnchEdge, int nWidth, int nHeight, int &PNum) 
{
	//八個方向掃描
	int xNb[8] = {1, 1, 0,-1,-1,-1, 0, 1} ;
	int yNb[8] = {0, 1, 1, 1,0 ,-1,-1,-1} ;
	
	CPoint EdgePoint;
	CPoint TmpPoint;
	int k  ;
	int Flag = 0;
	PNum++;
	EndPoint.x = StartPoint.x;
	EndPoint.y = StartPoint.y;
	for(k=0; k<8; k++)
	{
		EdgePoint.y = StartPoint.y + yNb[k] ;
		EdgePoint.x = StartPoint.x + xNb[k] ;
		if(pUnchEdge[EdgePoint.y * nWidth + EdgePoint.x] == 255 && Flag == 0)
		{
			Flag = 1;
			pUnchEdge[EdgePoint.y * nWidth+ EdgePoint.x] = 200 ;
			I_TraceEdge( EdgePoint, EndPoint, pUnchEdge, nWidth, nHeight, PNum);
		}
	}
}
void CRogerFunction::I_TraceEdge (int y, int x, int nLowThd, BYTE *pUnchEdge, int *pnMag, int nWidth) 
{ 
	int xNb[8] = {1, 1, 0,-1,-1,-1, 0, 1} ;
	int yNb[8] = {0, 1, 1, 1,0 ,-1,-1,-1} ;
	
	int yy ;
	int xx ;
	
	int k  ;
	
	for(k=0; k<8; k++)
	{
		yy = y + yNb[k] ;
		xx = x + xNb[k] ;
		if(pUnchEdge[yy*nWidth+xx] == 128  && pnMag[yy*nWidth+xx]>=nLowThd)
		{
			pUnchEdge[yy*nWidth+xx] = 255 ;
			I_TraceEdge(yy, xx, nLowThd, pUnchEdge, pnMag, nWidth);
		}
	}
}


void CRogerFunction::I_CANNY(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw, double in_sigma, double in_Th_ratio, double in_Tl_ratio)
{
	// double in_sigma             高思濾波標準差
	// double in_Tl_ratio          低臨界值和高臨界值之間比例
	// double in_Th_ratio          高臨界值佔圖像總畫素比例
	//initial variable....................................................................
	int i, j;
	int nWidth	= s_w;
	int nHeight = s_h;
	int nEffw = s_Effw; //o_ImageUL->GetEffWidth()	=> 影像單一一列的總長度(四的倍數)
	int k;

	BYTE *ImgSrc;
	BYTE *ImgRlt;
 	ImgSrc = new BYTE[nWidth*nHeight];
	ImgRlt = new BYTE[nWidth*nHeight];
	//先將全彩轉成灰階,再轉入1/3的暫存器
	for (i=0;i<nHeight;i++) //i控制列(高)
	{
		for (j=0,  k=0; j<nWidth ; j++, k+=3)        //k為陣列指標
		{
			ImgSrc[i*nWidth+j]=(in_image[i*nEffw+k]+in_image[i*nEffw+k+1]+in_image[i*nEffw+k+2])/3;
		}
	}
//.....canny...

	double sigma = in_sigma;
	double dRatioHigh=in_Th_ratio;
	double dRatioLow=in_Tl_ratio;

	int  *pnGradX;
	int  *pnGradY;
	int  *pnGradMAG;

	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double dWeightSum;
	double *pdTmp;
	int nCenter;
	double  dDis; 

	double  dValue; 
	double  dSum  ;
	double dSqtOne;
	double dSqtTwo;
	int nPos;
	float gx  ;
	float gy  ;
	int g1, g2, g3, g4 ;
	double weight  ;
	double dTmp1   ;
	double dTmp2   ;
	double dTmp    ;
	int nThdHigh ;
	int nThdLow  ;
	int nHist[1024] ;
	int nEdgeNb     ;
	int nMaxMag     ;
	int nHighCount  ;
	CRogerFunction FuncMe;
//..MakeGauss Smooth
	dSum = 0 ; 
	nWindowSize = 1 + 2 * ceil(3 * sigma);
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(sigma*sigma)) / (sqrt(2 * PI) * sigma );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		(pdKernel)[i] /= dSum;
	}
//..End MakeGauss........	

	nHalfLen = nWindowSize / 2;

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)ImgSrc[y*nWidth + (i+x)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul/dWeightSum ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			ImgRlt[y*nWidth + x] = (unsigned char)(int)dDotMul/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
	for (i=0; i<nWidth*nHeight; i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...計算方向導數
	pnGradX		= new int[nWidth*nHeight];
	pnGradY		= new int[nWidth*nHeight];
	pnGradMAG	= new int[nWidth*nHeight];

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			pnGradX[y*nWidth+x] = (int) (ImgSrc[y*nWidth+min(nWidth-1,x+1)]  
				- ImgSrc[y*nWidth+max(0,x-1)]     );
		}
	}
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			pnGradY[y*nWidth+x] = (int) ( ImgSrc[min(nHeight-1,y+1)*nWidth + x]  
				- ImgSrc[max(0,y-1)*nWidth+ x ]     );
		}
	}
//...End 計算方向導數

//...計算梯度
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dSqtOne = pnGradX[y*nWidth + x] * pnGradX[y*nWidth + x];
			dSqtTwo = pnGradY[y*nWidth + x] * pnGradY[y*nWidth + x];
			pnGradMAG[y*nWidth + x] = (int)(sqrt(dSqtOne + dSqtTwo) + 0.5);
		}
	}
//...End計算梯度
//...Non-Maximum

	for(x=0; x<nWidth; x++)		
	{
		ImgRlt[x] = 0 ;
		ImgRlt[nHeight-1+x] = 0;
	}
	for(y=0; y<nHeight; y++)		
	{
		ImgRlt[y*nWidth] = 0 ;
		ImgRlt[y*nWidth + nWidth-1] = 0;
	}

	for(y=1; y<nHeight-1; y++)
	{
		for(x=1; x<nWidth-1; x++)
		{
			nPos = y*nWidth + x ;
			if(pnGradMAG[nPos] == 0 )
			{
				ImgRlt[nPos] = 0 ;
			}
			else
			{
				dTmp = pnGradMAG[nPos] ;
				
				gx = pnGradX[nPos]  ;
				gy = pnGradY[nPos]  ;

				if (abs(gy) < abs(gx)) 
				{
					weight = fabs(gx)/fabs(gy); 

					g2 = pnGradMAG[nPos-nWidth] ; 
					g4 = pnGradMAG[nPos+nWidth] ;

					if (gx*gy > 0) 
					{ 					
						g1 = pnGradMAG[nPos-nWidth-1] ;
						g3 = pnGradMAG[nPos+nWidth+1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					} 
				}
				else
				{
					weight = fabs(gy)/fabs(gx); 
					
					g2 = pnGradMAG[nPos+1] ; 
					g4 = pnGradMAG[nPos-1] ;
					if (gx*gy > 0) 
					{				
						g1 = pnGradMAG[nPos+nWidth+1] ;
						g3 = pnGradMAG[nPos-nWidth-1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					}
				}
					dTmp1 = weight*g1 + (1-weight)*g2 ;
					dTmp2 = weight*g3 + (1-weight)*g4 ;
					if(dTmp> dTmp1 && dTmp> dTmp2)
					{
						ImgRlt[nPos] = 128 ;
					}
					else
					{
						ImgRlt[nPos] = 0 ;
					}
			} //else
		} // for
	}
	for (i=0; i<nWidth*nHeight;i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...END Non-Maximum
//...Hysteresis

	
// 	EstimateThreshold
	nMaxMag = 0     ;

	for(k=0; k<1024; k++) 
	{
		nHist[k] = 0; 
	}
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			if(ImgSrc[y*nWidth+x]==128)
			{
				nHist[ pnGradMAG[y*nWidth+x] ]++;
			}
		}
	}
	
	nEdgeNb = nHist[0]  ;
	nMaxMag = 0         ;
	for(k=1; k<1024; k++)
	{
		if(nHist[k] != 0)
		{
			nMaxMag = k;
		}
		nEdgeNb += nHist[k];
	}
	nHighCount = (int)(dRatioHigh * nEdgeNb +0.5);
	
	k = 1;
	nEdgeNb = nHist[1];
	
	while( (k<(nMaxMag-1)) && (nEdgeNb < nHighCount) )
	{
		k++;
		nEdgeNb += nHist[k];
	}
	nThdHigh = k ;
	
	nThdLow  = (int)((nThdHigh) * dRatioLow+ 0.5);	
// 	End EstimateThreshold


	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			
			if((ImgSrc[nPos] == 128) && (pnGradMAG[nPos] >= nThdHigh))
			{
				ImgSrc[nPos] = 255;
				FuncMe.I_TraceEdge(y, x, nThdLow, ImgSrc, pnGradMAG, nWidth);
			}
		}
	}
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			if(ImgSrc[nPos] != 255)
			{
				ImgSrc[nPos] = 0 ;
			}
		}
	 }
//...END Hysteresis

// gray level process 灰階輸出
		for (i=0;i<nHeight;i++) //i控制列(高)
		{
			for (j=0, k=0; j<nWidth ; j++, k+=3)        //k為陣列指標
			{
					out_image[i*nEffw+k]  =ImgSrc[i*nWidth+j];
					out_image[i*nEffw+k+1]=ImgSrc[i*nWidth+j];
					out_image[i*nEffw+k+2]=ImgSrc[i*nWidth+j];
			}
		}

delete []ImgSrc;
ImgSrc = NULL;
delete []ImgRlt;
ImgRlt = NULL;
delete []pnGradMAG;
pnGradMAG = NULL;
delete []pnGradX;
pnGradX = NULL;
delete []pnGradY;
pnGradY = NULL;
}
void CRogerFunction::I_CANNY_GRAY(BYTE *in_image, BYTE *out_image, int s_w, int s_h, double in_sigma, double in_Th_ratio, double in_Tl_ratio)
{
	int i, j;
	int nWidth	= s_w;
	int nHeight = s_h;
	int k;

	BYTE *ImgSrc = new BYTE[nWidth*nHeight];
	BYTE *ImgRlt = new BYTE[nWidth*nHeight];
	memcpy(ImgSrc,in_image,sizeof(BYTE)*nWidth*nHeight);
//.....canny...

	double sigma = in_sigma;
	double dRatioHigh=in_Th_ratio;
	double dRatioLow=in_Tl_ratio;

	int  *pnGradX;
	int  *pnGradY;
	int  *pnGradMAG;

	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double dWeightSum;
	double *pdTmp;
	int nCenter;
	double  dDis; 

	double  dValue; 
	double  dSum  ;
	double dSqtOne;
	double dSqtTwo;
	int nPos;
	float gx  ;
	float gy  ;
	int g1, g2, g3, g4 ;
	double weight  ;
	double dTmp1   ;
	double dTmp2   ;
	double dTmp    ;
	int nThdHigh ;
	int nThdLow  ;
	int nHist[1024] ;
	int nEdgeNb     ;
	int nMaxMag     ;
	int nHighCount  ;
	CRogerFunction FuncMe;
//..MakeGauss Smooth
	dSum = 0 ; 
	nWindowSize = 1 + 2 * ceil(3 * sigma);
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(sigma*sigma)) / (sqrt(2 * PI) * sigma );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		(pdKernel)[i] /= dSum;
	}
//..End MakeGauss........	

	nHalfLen = nWindowSize / 2;

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)ImgSrc[y*nWidth + (i+x)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul/dWeightSum ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			ImgRlt[y*nWidth + x] = (unsigned char)(int)dDotMul/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
	for (i=0; i<nWidth*nHeight; i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...計算方向導數
	pnGradX		= new int[nWidth*nHeight];
	pnGradY		= new int[nWidth*nHeight];
	pnGradMAG	= new int[nWidth*nHeight];

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			pnGradX[y*nWidth+x] = (int) (ImgSrc[y*nWidth+min(nWidth-1,x+1)]  
				- ImgSrc[y*nWidth+max(0,x-1)]     );
		}
	}
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			pnGradY[y*nWidth+x] = (int) ( ImgSrc[min(nHeight-1,y+1)*nWidth + x]  
				- ImgSrc[max(0,y-1)*nWidth+ x ]     );
		}
	}
//...End 計算方向導數

//...計算梯度
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dSqtOne = pnGradX[y*nWidth + x] * pnGradX[y*nWidth + x];
			dSqtTwo = pnGradY[y*nWidth + x] * pnGradY[y*nWidth + x];
			pnGradMAG[y*nWidth + x] = (int)(sqrt(dSqtOne + dSqtTwo) + 0.5);
		}
	}
//...End計算梯度
//...Non-Maximum

	for(x=0; x<nWidth; x++)		
	{
		ImgRlt[x] = 0 ;
		ImgRlt[nHeight-1+x] = 0;
	}
	for(y=0; y<nHeight; y++)		
	{
		ImgRlt[y*nWidth] = 0 ;
		ImgRlt[y*nWidth + nWidth-1] = 0;
	}

	for(y=1; y<nHeight-1; y++)
	{
		for(x=1; x<nWidth-1; x++)
		{
			nPos = y*nWidth + x ;
			if(pnGradMAG[nPos] == 0 )
			{
				ImgRlt[nPos] = 0 ;
			}
			else
			{
				dTmp = pnGradMAG[nPos] ;
				
				gx = pnGradX[nPos]  ;
				gy = pnGradY[nPos]  ;

				if (abs(gy) < abs(gx)) 
				{
					weight = fabs(gx)/fabs(gy); 

					g2 = pnGradMAG[nPos-nWidth] ; 
					g4 = pnGradMAG[nPos+nWidth] ;

					if (gx*gy > 0) 
					{ 					
						g1 = pnGradMAG[nPos-nWidth-1] ;
						g3 = pnGradMAG[nPos+nWidth+1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					} 
				}
				else
				{
					weight = fabs(gy)/fabs(gx); 
					
					g2 = pnGradMAG[nPos+1] ; 
					g4 = pnGradMAG[nPos-1] ;
					if (gx*gy > 0) 
					{				
						g1 = pnGradMAG[nPos+nWidth+1] ;
						g3 = pnGradMAG[nPos-nWidth-1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					}
				}
					dTmp1 = weight*g1 + (1-weight)*g2 ;
					dTmp2 = weight*g3 + (1-weight)*g4 ;
					if(dTmp> dTmp1 && dTmp> dTmp2)
					{
						ImgRlt[nPos] = 128 ;
					}
					else
					{
						ImgRlt[nPos] = 0 ;
					}
			} //else
		} // for
	}
	for (i=0; i<nWidth*nHeight;i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...END Non-Maximum
//...Hysteresis

	
// 	EstimateThreshold
	nMaxMag = 0     ;

	for(k=0; k<1024; k++) 
	{
		nHist[k] = 0; 
	}
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			if(ImgSrc[y*nWidth+x]==128)
			{
				nHist[ pnGradMAG[y*nWidth+x] ]++;
			}
		}
	}
	
	nEdgeNb = nHist[0]  ;
	nMaxMag = 0         ;
	for(k=1; k<1024; k++)
	{
		if(nHist[k] != 0)
		{
			nMaxMag = k;
		}
		nEdgeNb += nHist[k];
	}
	nHighCount = (int)(dRatioHigh * nEdgeNb +0.5);
	
	k = 1;
	nEdgeNb = nHist[1];
	
	while( (k<(nMaxMag-1)) && (nEdgeNb < nHighCount) )
	{
		k++;
		nEdgeNb += nHist[k];
	}
	nThdHigh = k ;
	
	nThdLow  = (int)((nThdHigh) * dRatioLow+ 0.5);	
// 	End EstimateThreshold


	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			
			if((ImgSrc[nPos] == 128) && (pnGradMAG[nPos] >= nThdHigh))
			{
				ImgSrc[nPos] = 255;
				FuncMe.I_TraceEdge(y, x, nThdLow, ImgSrc, pnGradMAG, nWidth);
			}
		}
	}
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			if(ImgSrc[nPos] != 255)
			{
				ImgSrc[nPos] = 0 ;
			}
		}
	 }
//...END Hysteresis
	// gray level process 灰階輸出
	for (i=0;i<nHeight;i++) //i控制列(高)
	{
		for (j=0; j<nWidth ; j++)        //k為陣列指標
		{
			out_image[i*nWidth+j]  =ImgSrc[i*nWidth+j];
		}
	}


delete []ImgSrc;
ImgSrc = NULL;
delete []ImgRlt;
ImgRlt = NULL;
delete []pnGradMAG;
pnGradMAG = NULL;
delete []pnGradX;
pnGradX = NULL;
delete []pnGradY;
pnGradY = NULL;
}
void CRogerFunction::I_Thinner(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{

	int ly = s_w; //width
	int lx = s_h; //height
	char *f, *g;
	char n[10];
	char a[5] = {0, -1, 1, 0, 0};
	char b[5] = {0, 0, 0, 1, -1};
	char nrnd, cond, n48, n26, n24, n46, n68, n82, n123, n345, n567, n781;
	short k, shori;
	int i, j;
	long ii, jj, kk, kk1, kk2, kk3, size;
	char *image;
	size = (long)lx * (long)ly;
	image = new char[size];

	for ( i = 0; i < lx; ++i)
	{
		for ( j = 0; j < ly; ++j)
		{
			if (in_image[i*ly+j] != 0)
				image[i*ly+j] = 1;
			else
				image[i*ly+j] = 0;
		}
	}

	g = (char *)malloc(size);

	f = (char *)image;
	for(kk=0l; kk<size; kk++)
	{
		g[kk] = f[kk];
	}

	do
	{
		shori = 0;
		for(k=1; k<=4; k++)
		{
			for(i=1; i<lx-1; i++)
			{
				ii = i + a[k];

				for(j=1; j<ly-1; j++)
				{
					kk = i*ly + j;

					if(!f[kk])
						continue;

					jj = j + b[k];
					kk1 = ii*ly + jj;

					if(f[kk1])
						continue;

					kk1 = kk - ly -1;
					kk2 = kk1 + 1;
					kk3 = kk2 + 1;
					n[3] = f[kk1];
					n[2] = f[kk2];
					n[1] = f[kk3];
					kk1 = kk - 1;
					kk3 = kk + 1;
					n[4] = f[kk1];
					n[8] = f[kk3];
					kk1 = kk + ly - 1;
					kk2 = kk1 + 1;
					kk3 = kk2 + 1;
					n[5] = f[kk1];
					n[6] = f[kk2];
					n[7] = f[kk3];

					nrnd = n[1] + n[2] + n[3] + n[4]
					+n[5] + n[6] + n[7] + n[8];
					if(nrnd<=1)
						continue;

					cond = 0;
					n48 = n[4] + n[8];
					n26 = n[2] + n[6];
					n24 = n[2] + n[4];
					n46 = n[4] + n[6];
					n68 = n[6] + n[8];
					n82 = n[8] + n[2];
					n123 = n[1] + n[2] + n[3];
					n345 = n[3] + n[4] + n[5];
					n567 = n[5] + n[6] + n[7];
					n781 = n[7] + n[8] + n[1];

					if(n[2]==1 && n48==0 && n567>0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					if(n[6]==1 && n48==0 && n123>0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					if(n[8]==1 && n26==0 && n345>0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					if(n[4]==1 && n26==0 && n781>0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					if(n[5]==1 && n46==0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					if(n[7]==1 && n68==0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					if(n[1]==1 && n82==0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					if(n[3]==1 && n24==0)
					{
						if(!cond)
							continue;
						g[kk] = 0;
						shori = 1;
						continue;
					}

					cond = 1;
					if(!cond)
						continue;
					g[kk] = 0;
					shori = 1;
				}
			}

			for(i=0; i<lx; i++)
			{
				for(j=0; j<ly; j++)
				{
					kk = i*ly + j;
					f[kk] = g[kk];
				}
			}
		}
	}while(shori);
	for ( i = 0; i < lx; ++i)
	{
		for ( j = 0; j < ly; ++j)
		{
			if (image[i*ly+j] != 1)
				out_image[i*ly+j] = 0;
			else
				out_image[i*ly+j] = 255;
		}
	}
	free(g);
}
void CRogerFunction::I_SkinExtract(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	float cy, cb, cr;

	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
			cy = 0.2999*in_image[k+2]+0.587*in_image[k+1]+0.114*in_image[k];
			cb = 0.500*in_image[k]-0.169*in_image[k+2]-0.331*in_image[k+1]+127;
			cr = 0.500*in_image[k+2]-0.419*in_image[k+1]-0.081*in_image[k]+127;
		
			if( cr > 133 && cr < 173 && cb > 77 && cb < 127 )
			{
				out_image[k]=in_image[k];
				out_image[k+1]=in_image[k+1];
				out_image[k+2]=in_image[k+2];
			}
			else
			{
				out_image[k]=0;
				out_image[k+1]=0;
				out_image[k+2]=0;
			}
		}		
	}
} 

void CRogerFunction::I_Contour(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{		
	int i,j;
	for (i=0; i< s_w*s_h; i++)
		out_image[i] = 0;

	for (i=1; i<s_h-1; i++)
	{
		for(j=1; j<s_w-1; j++)
		{
			if (in_image[i*s_w+j  ] >=200 )
			{
				if (in_image[(i-1)*s_w+j-1] >=200 && 
					in_image[(i-1)*s_w+j  ] >=200 &&
					in_image[(i-1)*s_w+j+1] >=200 && 
					in_image[i    *s_w+j-1] >=200 &&
					in_image[i    *s_w+j+1] >=200 && 
					in_image[(i+1)*s_w+j-1] >=200 &&
					in_image[(i+1)*s_w+j  ] >=200 && 
					in_image[(i+1)*s_w+j+1] >=200 	)
					
					out_image[i*s_w+j] = 0;
				else
					out_image[i*s_w+j] = 255;
			}
			else
				out_image[i*s_w+j] = in_image[i*s_w+j];
		}
	}
}
void CRogerFunction::I_HoughLine(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{
	long i,j;
	int		*nTemp;
	int		iMaxDist;			// 變換域的尺寸
	int		iMaxAngleNumber;	
	int		iDist;				// 變換域的坐標
	int		iAngleNumber;	
	MaxValue MaxValue1;			// 存儲變換域中的兩個最大值
	MaxValue MaxValue2;
	MaxValue MaxValue3;
	//計算變換域的尺寸	最大距離
	iMaxDist = (int) sqrtf( s_w * s_w + s_h * s_h);	
	//角度從0－180，每格2度
	iMaxAngleNumber = 90;
	nTemp	=	new int[iMaxAngleNumber * iMaxDist];
	for(i=0; i<iMaxAngleNumber * iMaxDist; i++) nTemp[i] = 0;
	for(i=0; i<s_w * s_h; i++) out_image[i] = 0;

	for(i = 0; i <s_h; i++)
	{
		for(j = 0;j <s_w; j++)
		{
			//如果是白點，則在變換域的對應點上加1
			if(in_image[i * s_w +j] >=200 )
			{
				for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)//注意步長是2度
				{
					// zo = xcosT + ysinT
					iDist = (int) fabs(i*cos(iAngleNumber*2*PI/180.0) + j*sin(iAngleNumber*2*PI/180.0));
					//變換域的對應點上加1
					nTemp[iDist*iMaxAngleNumber+iAngleNumber] +=1; 
				}
			}	
		}
	}
	//找到變換域中的3個最大值點
	MaxValue1.Value=0;
	MaxValue2.Value=0;
	MaxValue3.Value=0;
	
// 	//找到第一個最大值點
	for (iDist=0; iDist<iMaxDist;iDist++)
	{
		for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)
		{
			if (nTemp[iDist*iMaxAngleNumber+iAngleNumber] > MaxValue1.Value && iDist > 0)
			{
				MaxValue1.Value = (int)nTemp[iDist*iMaxAngleNumber+iAngleNumber];
				MaxValue1.Dist = iDist;
				MaxValue1.AngleNumber = iAngleNumber;
			}
		}
	}
	//將第一個最大值點附近清零
	for (iDist = -9;iDist < 10;iDist++)
	{
		for(iAngleNumber=-1; iAngleNumber<2; iAngleNumber++)
		{
			if(iDist+MaxValue1.Dist >= 0 &&
			   iDist+MaxValue1.Dist < iMaxDist && 
				iAngleNumber+MaxValue1.AngleNumber >= 0 && 
				iAngleNumber+MaxValue1.AngleNumber <= iMaxAngleNumber)
			{
				nTemp[(iDist+MaxValue1.Dist)*iMaxAngleNumber+(iAngleNumber+MaxValue1.AngleNumber)]=0;
			}
		}
	}

// 	//找到第二個最大值點
	for (iDist=0; iDist<iMaxDist;iDist++)
	{
		for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)
		{
			if (nTemp[iDist*iMaxAngleNumber+iAngleNumber] > MaxValue2.Value && iDist > 0)
			{
				MaxValue2.Value = (int)nTemp[iDist*iMaxAngleNumber+iAngleNumber];
				MaxValue2.Dist = iDist;
				MaxValue2.AngleNumber = iAngleNumber;
			}
		}
	}
	//將第2個最大值點附近清零
	for (iDist = -9;iDist < 10;iDist++)
	{
		for(iAngleNumber=-1; iAngleNumber<2; iAngleNumber++)
		{
			if(iDist+MaxValue2.Dist >= 0 &&
				iDist+MaxValue2.Dist < iMaxDist && 
				iAngleNumber+MaxValue2.AngleNumber >= 0 && 
				iAngleNumber+MaxValue2.AngleNumber <= iMaxAngleNumber)
			{
				nTemp[(iDist+MaxValue2.Dist)*iMaxAngleNumber+(iAngleNumber+MaxValue2.AngleNumber)]=0;
			}
		}
	}
	
	// 	//找到第3個最大值點
	for (iDist=0; iDist<iMaxDist;iDist++)
	{
		for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)
		{
			if (nTemp[iDist*iMaxAngleNumber+iAngleNumber] > MaxValue3.Value && iDist > 0)
			{
				MaxValue3.Value = (int)nTemp[iDist*iMaxAngleNumber+iAngleNumber];
				MaxValue3.Dist = iDist;
				MaxValue3.AngleNumber = iAngleNumber;
			}
		}
	}
	//在緩存圖像中重繪直線
		for(i = 0; i <s_h; i++)
		{
			for(j = 0;j <s_w; j++)
			{	
				//zo = x cos T + y sin T
				//在第一條直線上
				if ( MaxValue1.Dist == (int) fabs(i*cos(MaxValue1.AngleNumber*2*PI/180.0) + j*sin(MaxValue1.AngleNumber*2*PI/180.0)))
				{
					out_image[i*s_w+j] = 100;
				}
				//在第二條直線上
				else if ( MaxValue2.Dist == (int) fabs(i*cos(MaxValue2.AngleNumber*2*PI/180.0) + j*sin(MaxValue2.AngleNumber*2*PI/180.0)))
				{
					out_image[i*s_w+j] = 100;
				}
				//在第3條直線上
				else if ( MaxValue3.Dist == (int) fabs(i*cos(MaxValue3.AngleNumber*2*PI/180.0) + j*sin(MaxValue3.AngleNumber*2*PI/180.0)))
				{
					out_image[i*s_w+j] = 100;
				}
				else
					out_image[i*s_w+j] = in_image[i*s_w+j];
				
			}
		}
	delete []nTemp;

}
void CRogerFunction::I_HoughCircle(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{
	long d,i,j,k,m,n;
	int		x, y;
	int		Rmin = 5;
	int		R = 10; //範圍 最小半徑~最小半徑+20
	int		*nTemp;
	MaxValue_C MaxValue1;			// 存儲變換域中的兩個最大值
	MaxValue_C MaxValue2;
	MaxValue_C MaxValue3;
	nTemp	=	new int[ (Rmin+R) * s_w * s_h ];
	for(i=0; i< (Rmin+R) * s_w * s_h; i++) nTemp[i] = 0;
	for (i=0; i< s_w*s_h; i++) out_image[i] = in_image[i];
	for(i = (Rmin+R)/2 ; i <s_h-(Rmin+R)/2 ; i++)
	{
		for(j = (Rmin+R)/2 ; j <s_w-(Rmin+R)/2 ; j++)
		{
			//如果是白點，則在變換域的對應點上加1
			if(in_image[i * s_w +j] >=200 )
			{
				for (k=Rmin; k<(Rmin+R) ; k++)
				{
					for (d=0; d<360; d++) // 0-360 degree
					{
						x = j - k * cos(d*PI/180); 
						y = /*s_h -*/ i - k * sin(d*PI/180);
						if (k*s_w*s_h + y*s_w +x >0 && k*s_w*s_h + y*s_w +x < (Rmin+R)*s_w*s_h)
						{
							nTemp[k*s_w*s_h + y*s_w +x]+=1;
						}
					}
				}
			}	
		}
	}
 	//找到變換域中的3個最大值點
	MaxValue1.Value=0;
	MaxValue2.Value=0;
	MaxValue3.Value=0;

	//找到第一個最大值點
	for(i = 1 ; i <s_h ; i++)
	{
		for(j = 1 ; j <s_w ; j++)
		{
			for (k=Rmin; k<(Rmin+R) ; k++)
			{
				if (nTemp[k*s_w*s_h + i*s_w +j] > MaxValue1.Value && nTemp[k*s_w*s_h + i*s_w +j] > 100)
				{
					MaxValue1.Value  = nTemp[k*s_w*s_h + i*s_w +j];
					MaxValue1.Radius = k;
					MaxValue1.x = j;
					MaxValue1.y = i;
				}
			}
		}
	}
	//清除第一個最大值點周圍
	for (m=-MaxValue1.Radius; m<=MaxValue1.Radius; m++)
	{
		for (n=-MaxValue1.Radius; n<=MaxValue1.Radius; n++)
		{
			for (d=0; d<(Rmin+R); d++)
			{
				nTemp[d*s_w*s_h+(MaxValue1.y+m)*s_w+(MaxValue1.x+n)] = 0;
			}
		}
	}

	delete []nTemp;

 	//在緩存圖像中重繪第1個圓
	for (d=0; d<360; d++) // 0-360 degree
	{
		x = MaxValue1.x + MaxValue1.Radius * cos(d*PI/180);
		y = MaxValue1.y + MaxValue1.Radius * sin(d*PI/180);
		out_image[ y * s_w + x ] = 100;
	}
}
void CRogerFunction::I_Equalization(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{
// 	//initial variable
	int i, j;
	float nPixel = s_w * s_h;
	float *HistoR;
	HistoR = new float[256];
//     //清除直方圖陣列內容
	for (i=0; i<256; i++)
	{
		HistoR[i] = 0;
	}
// 	//統計灰階出現次數
// 	//將各個灰階出現次數,再除以圖像大小w*h
//     // pr(x) = H(x) /A  為PDF probability density function
	for (i=0;i<s_h;i++) //i控制列(高)
	{
		for (j=0; j<s_w ; j++)        //k為陣列指標
		{
			HistoR[in_image[i*s_w+j]] += 1;
		}
	}
//     //計算CDF Cumulative Density Function
	for (i=1; i<256; i++)
	{
		HistoR[i] = HistoR[i] + HistoR[i-1];
	}
//     //產生轉換函數對映表
	for (i=0; i<256; i++)
	{
		HistoR[i] = (int)(HistoR[i] * 255 / nPixel);
	}
// 	//將輸入圖形轉換後輸出	
	for (i=0;i<s_h;i++) //i控制列(高)
	{
		for (j=0; j<s_w ; j++)        //k為陣列指標
		{
			out_image[i*s_w+j] = HistoR[in_image[i*s_w+j]];
		}
	}
	delete []HistoR;
}
	
void CRogerFunction::I_TriAngle(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{
	long i,j, m, n;
	int		*nTemp;
	int		iMaxDist;			// 變換域的尺寸
	int		iMaxAngleNumber;	
	int		iDist;				// 變換域的坐標
	int		iAngleNumber;
	BYTE	*TmpMem;
	int		*TmpMem1;
	TmpMem	= new BYTE[s_w*s_h];
	TmpMem1	= new int[s_w*s_h];
	TriPoint P1, P2, P3;
	P1.Value =0;
	P2.Value =0;
	P3.Value =0;

	for(i=0; i<s_w*s_h; i++)
	{
		TmpMem[i] = 0;
		TmpMem1[i] = 0;
	}

	MaxValue MaxValue1;			// 存儲變換域中的兩個最大值
	MaxValue MaxValue2;
	MaxValue MaxValue3;
	//計算變換域的尺寸	最大距離
	iMaxDist = (int) sqrtf( s_w * s_w + s_h * s_h);	
	//角度從0－180，每格2度
	iMaxAngleNumber = 90;
	nTemp	=	new int[iMaxAngleNumber * iMaxDist];
	for(i=0; i<iMaxAngleNumber * iMaxDist; i++) nTemp[i] = 0;
	for(i=0; i<s_w * s_h; i++) out_image[i] = 0;

	for(i = 0; i <s_h; i++)
	{
		for(j = 0;j <s_w; j++)
		{
			//如果是白點，則在變換域的對應點上加1
			if(in_image[i * s_w +j] >=200 )
			{
				for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)//注意步長是2度
				{
					// zo = xcosT + ysinT
					iDist = (int) fabs(i*cos(iAngleNumber*2*PI/180.0) + j*sin(iAngleNumber*2*PI/180.0));
					//變換域的對應點上加1
					nTemp[iDist*iMaxAngleNumber+iAngleNumber] +=1; 
				}
			}	
		}
	}
	//找到變換域中的3個最大值點
	MaxValue1.Value=0;
	MaxValue2.Value=0;
	MaxValue3.Value=0;
	
// 	//找到第一個最大值點
	for (iDist=0; iDist<iMaxDist;iDist++)
	{
		for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)
		{
			if (nTemp[iDist*iMaxAngleNumber+iAngleNumber] > MaxValue1.Value && iDist > 0)
			{
				MaxValue1.Value = (int)nTemp[iDist*iMaxAngleNumber+iAngleNumber];
				MaxValue1.Dist = iDist;
				MaxValue1.AngleNumber = iAngleNumber;
			}
		}
	}
	//將第一個最大值點附近清零
	for (iDist = -9;iDist < 10;iDist++)
	{
		for(iAngleNumber=-1; iAngleNumber<2; iAngleNumber++)
		{
			if(iDist+MaxValue1.Dist >= 0 &&
			   iDist+MaxValue1.Dist < iMaxDist && 
				iAngleNumber+MaxValue1.AngleNumber >= 0 && 
				iAngleNumber+MaxValue1.AngleNumber <= iMaxAngleNumber)
			{
				nTemp[(iDist+MaxValue1.Dist)*iMaxAngleNumber+(iAngleNumber+MaxValue1.AngleNumber)]=0;
			}
		}
	}

// 	//找到第二個最大值點
	for (iDist=0; iDist<iMaxDist;iDist++)
	{
		for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)
		{
			if (nTemp[iDist*iMaxAngleNumber+iAngleNumber] > MaxValue2.Value && iDist > 0)
			{
				MaxValue2.Value = (int)nTemp[iDist*iMaxAngleNumber+iAngleNumber];
				MaxValue2.Dist = iDist;
				MaxValue2.AngleNumber = iAngleNumber;
			}
		}
	}
	//將第2個最大值點附近清零
	for (iDist = -9;iDist < 10;iDist++)
	{
		for(iAngleNumber=-1; iAngleNumber<2; iAngleNumber++)
		{
			if(iDist+MaxValue2.Dist >= 0 &&
				iDist+MaxValue2.Dist < iMaxDist && 
				iAngleNumber+MaxValue2.AngleNumber >= 0 && 
				iAngleNumber+MaxValue2.AngleNumber <= iMaxAngleNumber)
			{
				nTemp[(iDist+MaxValue2.Dist)*iMaxAngleNumber+(iAngleNumber+MaxValue2.AngleNumber)]=0;
			}
		}
	}
	
	// 	//找到第3個最大值點
	for (iDist=0; iDist<iMaxDist;iDist++)
	{
		for(iAngleNumber=0; iAngleNumber<iMaxAngleNumber; iAngleNumber++)
		{
			if (nTemp[iDist*iMaxAngleNumber+iAngleNumber] > MaxValue3.Value && iDist > 0)
			{
				MaxValue3.Value = (int)nTemp[iDist*iMaxAngleNumber+iAngleNumber];
				MaxValue3.Dist = iDist;
				MaxValue3.AngleNumber = iAngleNumber;
			}
		}
	}
	//在緩存圖像中重繪直線
		for(i = 0; i <s_h; i++)
		{
			for(j = 0;j <s_w; j++)
			{	
				//zo = x cos T + y sin T
				//在第一條直線上
				if ( MaxValue1.Dist == (int) fabs(i*cos(MaxValue1.AngleNumber*2*PI/180.0) + j*sin(MaxValue1.AngleNumber*2*PI/180.0)))
				{
					TmpMem[i*s_w+j] = 255;
				}
				//在第二條直線上
				else if ( MaxValue2.Dist == (int) fabs(i*cos(MaxValue2.AngleNumber*2*PI/180.0) + j*sin(MaxValue2.AngleNumber*2*PI/180.0)))
				{
					TmpMem[i*s_w+j] = 255;
				}
				//在第3條直線上
				else if ( MaxValue3.Dist == (int) fabs(i*cos(MaxValue3.AngleNumber*2*PI/180.0) + j*sin(MaxValue3.AngleNumber*2*PI/180.0)))
				{
					TmpMem[i*s_w+j] = 255;
				}
				else
					TmpMem[i*s_w+j] = 0;
			}
		}

		for (i=1; i<s_h-1; i++)
		{
			for (j=1; j<s_w-1; j++)
			{
				for (m=-2; m<=2 ; m++)
				{
					for (n=-2; n<=2; n++)
					{
						TmpMem1[i*s_w+j] +=  TmpMem[(i+m)*s_w+(j+n)];
					}
				}
			}
		}
	for (i=0; i< s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			if (TmpMem1[i*s_w+j] > 2290)
			{
				out_image[i*s_w+j] = 255;
				gpMsgbar->ShowMessage("x:%d, y:%d, value %d\n", j, s_h-i, TmpMem1[i*s_w+j]);
			}
			else
				out_image[i*s_w+j] =0;

		}
	}
	delete []nTemp;
	delete []TmpMem;
	delete []TmpMem1;

}
void CRogerFunction::I_CopyBuffer123(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i,j,k;
	
	BYTE *p_in_image = in_image;
	BYTE *p_out_image = out_image;
	for (i = 0; i < s_h; i++, p_in_image += s_w, p_out_image += s_Effw)
	{
		for (j = 0, k = 0; j < s_w; j++, k += 3)
		{
			p_out_image[k] = p_out_image[k + 1] = p_out_image[k + 2] = (BYTE)p_in_image[j];
		}
	}
}
void CRogerFunction::I_CopyBuffer123(int *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i,j,k;
	int *p_in_image = in_image;
	BYTE *p_out_image = out_image;
	for (i = 0; i < s_h; i++, p_in_image += s_w, p_out_image+=s_Effw)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)
		{
			p_out_image[k] = p_out_image[k + 1] = p_out_image[k + 2] = (BYTE)p_in_image[j];
		}	
	}
}
// void CRogerFunction::I_CopyBuffer321(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
// {
// 	int i,j,k;
// 	for (i=0;i<s_h;i++) //i控制列(高)
// 	{
// 		for (j=0,  k=0; j<s_w ; j++, k+=3)        //k為陣列指標
// 		{
// // 			if( s_Effw < 3*s_w)
// 			out_image[i*s_w+j]= (int)(in_image[i*s_Effw+k]+in_image[i*s_Effw+k+1]+in_image[i*s_Effw+k+2])/3;
// 		}
// 	}
// }

// void CRogerFunction::sift(SiftInfo* SI, Img I) 
// {
// 	
// 	int 	M=I.M;
// 	int 	N=I.N;
// 	int		C=I.C;
// 	
// 	/* Lowe's equivalents choices */
// 	int 	S=3; //金字塔 S層
// 	int 	omin=-1;
// 	int 	O=(int)(log2(min(M, N)))-omin-3; //金字塔 O組
// 	double 	sigma0=1.6*pow(2,1/(double)S);
// 	double 	sigman=0.5;
// 	double 	thresh=0.15/S/2;
// 	double 	r=10.0;
// 	int 	NBP=4;
// 	int 	NBO=8;
// 	double 	magnif=3.0;
// 	
// 	/* SIFT Detector and Descriptor */
// 	
// 	gaussianss(&SI->gss,&I,sigman,O,S,omin,-1,S+1,sigma0);
// 
// 	diffss(&SI->dogss, &SI->gss);
// 
// 	/* frames and descriptors do it all */
// 	int frames_end=0, descriptors_end=0;
// 	SI->frames.N=0; SI->descriptors.N=0;
// 
// 	for(int o=0; o<SI->gss.O; o++) {
// 		
// 		/* siftlocalmax */	
// 		Img idx; idx.M=0; idx.N=0; idx.C=0;
// 		siftlocalmax(&idx, &SI->dogss.octave[o], 0.8*thresh);
// 		
// 		Img TMP;
// 		TMP.M=SI->dogss.octave[o].M;
// 		TMP.N=SI->dogss.octave[o].N;
// 		TMP.C=SI->dogss.octave[o].C;
// 		TMP.img.assign(SI->dogss.octave[o].M*SI->dogss.octave[o].N*SI->dogss.octave[o].C,0);
// 		for(int i=0; i<SI->dogss.octave[o].M*SI->dogss.octave[o].N*SI->dogss.octave[o].C; i++)
// 			TMP.img[i] = -1 * SI->dogss.octave[o].img[i];
// 		siftlocalmax(&idx, &TMP, 0.8*thresh);     //偵測local maxima....
// 
// 		/* ind2sub */
// 		int K = idx.N;
// 		Img oframes;
// 		ind2sub(&oframes, &SI->dogss.octave[o], idx, &SI->dogss) ;
// 		
// 	
// 		/* Remove points too close to the boundary */
// 		double rad;
// 		vector<double>::iterator it;
// 		vector<double> buffer = oframes.img;
// 		oframes.img.clear();
// 		for( int i=0; i<oframes.N; i++) {
// 			rad = magnif * SI->gss.sigma0 * pow(2.0, buffer[2+i*oframes.M]/SI->gss.S ) * NBP / 2 ;
// 			if(buffer[0+i*oframes.M]-rad>=1 && buffer[0+i*oframes.M]+rad<=SI->gss.octave[o].N && buffer[1+i*oframes.M]-rad>=1 && buffer[1+i*oframes.M]+rad<=SI->gss.octave[o].M) {
// 				it = oframes.img.end();
// 				oframes.img.insert(it, buffer.begin()+i*oframes.M, buffer.begin()+(i+1)*oframes.M);
// 			}
// 		}
// 		oframes.N = oframes.img.size()/oframes.M;
// 		
// 		/* siftrefinemx */
// 		siftrefinemx(&oframes, &oframes, &SI->dogss.octave[o], SI->dogss.smin, thresh, r);
// 		
// 		
// 		
// 		/* siftormx */
// 		siftormx(&oframes, &oframes, &SI->gss.octave[o], SI->gss.S, SI->gss.smin, SI->gss.sigma0);
// 		
// 	
// 		
// 		/* Store frames */
// 		SI->frames.img.resize(frames_end + oframes.N*4);
// 		SI->frames.M=oframes.M; SI->frames.N+=oframes.N; SI->frames.C=1;
// 		for(int i=0; i<oframes.N; i++) {
// 			SI->frames.img[frames_end+i*4  ] = pow(2.0,(o+SI->gss.omin)) * oframes.img[i*4];
// 			SI->frames.img[frames_end+i*4+1] = pow(2.0,(o+SI->gss.omin)) * oframes.img[i*4+1];
// 			SI->frames.img[frames_end+i*4+2] = pow(2.0,(o+SI->gss.omin)) * SI->gss.sigma0 * pow(2.0, oframes.img[i*4+2] / SI->gss.S);
// 			SI->frames.img[frames_end+i*4+3] = oframes.img[i*4+3];
// 		}
// 
// 		
// 		/* Descriptors */
// 		Img sh;
// 		siftdescriptor(&sh, &SI->gss.octave[o], &oframes, SI->gss.sigma0, SI->gss.S, SI->gss.smin, magnif, NBP, NBO);
// 		SI->descriptors.img.resize(descriptors_end + sh.N*sh.M);
// 		SI->descriptors.M=sh.M; SI->descriptors.N+=sh.N; SI->descriptors.C=1;
// 		for( int i=0; i<sh.N; i++) {
// 			for(int j=0; j<sh.M; j++) {
// 				SI->descriptors.img[descriptors_end+i*sh.M + j]=sh.img[i*sh.M + j];
// 			}
// 		}
// 
// 		descriptors_end += sh.M*sh.N;
// 		frames_end += oframes.N*4;
// 	}
// 	
// }

// void CRogerFunction::gaussianss(Gss* ss, Img* I, double sigman, int O, int S, int omin, int smin, int smax, double sigma0) {
// 	
// 	/* Do the job */
// 	
// 	/* Scale multiplicative step */
// 	double k=pow(2,1/(double)S);
// 	
// 	double dsigma0 = sigma0 * sqrt(1 - 1/(k*k));
// 	sigman = 0.5;
// 	
// 	/* % Scale space structure */
// 	ss->O = O;
// 	ss->S = S;
// 	ss->sigma0 = sigma0; 
// 	ss->omin = omin;
// 	ss->smin = smin;
// 	ss->smax = smax;
// 	
// 	/* If omin < 0, multiply the size of the image. */
// 	if(omin<0)
// 		for(int o=1; o<=-1*omin; o++) {
// 			doubleSize(I, *I, 1);	
// 		}
// 		
// 		int M = I->M;
// 		int N = I->N;
// 		int C = smax-smin+1;
// 		
// 		/* Index offset */
// 		int so=-1*smin+1;
// 		
// 		/* First octave */
// 		
// 		/* set ss->octave struct size first */
// 		Img TMP;
// 		TMP.M=M;
// 		TMP.N=N;
// 		TMP.C=C;
// 		TMP.img.assign(M*N*C,0);
// 		ss->octave.push_back(TMP); 
// 		
// 		/* imsmooth */
// 		imsmooth(&ss->octave[0], 1, *I, 1, sqrt(pow(sigma0*pow(k, smin),2.0) - pow(sigman/pow(2.0, omin),2.0)));
// 		
// 		/* others */
// 		for(int s=smin+1; s<=smax; s++) {
// 			
// 			double dsigma = pow(k, s) * dsigma0;
// 			
// 			/* insert */
// 			imsmooth(&ss->octave[0], s+so, ss->octave[0], s-1+so, dsigma);
// 			
// 		}
// 		
// 		/* Other octaves */
// 		for(int o=1; o<O; o++){
// 			
// 			int sbest = min(smin + S, smax);
// 			
// 			/* for halfsize */
// 			Img TMP;
// 			TMP.M=0;
// 			TMP.N=0;
// 			TMP.C=C;
// 			TMP.img.assign((int)((ss->octave[o-1].M+1)/2)*(int)((ss->octave[o-1].N+1)/2),0);
// 			
// 			halveSize(&TMP, ss->octave[o-1], sbest+so);
// 			/* end halfsize */
// 			
// 			double target_sigma = sigma0 * pow(k,smin);
// 			double prev_sigma = sigma0 * pow(k,(sbest - S));
// 			if(target_sigma > prev_sigma) {
// 				imsmooth(&TMP, 1, TMP, 1, sqrt(pow(target_sigma,2) - pow(prev_sigma,2)) );
// 			}
// 			
// 			ss->octave.push_back(TMP);
// 			ss->octave[o].img.resize(ss->octave[o].M*ss->octave[o].N*ss->octave[o].C); 
// 			
// 			for(int s=smin+1; s<=smax; s++) {
// 				
// 				double dsigma = pow(k, s) * dsigma0;
// 				
// 				/* insert */
// 				imsmooth(&ss->octave[o], s+so, ss->octave[o], s-1+so, dsigma);
// 				
// 			}
// 			
// 		}
// 		
// 		//for(int i=16*16*6-10; i<16*16*6; i++)
// 		//cout<<ss->octave[6].img[i]<<endl;
// }
// void CRogerFunction::doubleSize(Img* J, const Img I, const int I_c){
// 	
// 	int i,j;
// 	int M=I.M, N=I.N;
// 	vector<double>* J_pt = &J->img;
// 	const vector<double>* I_pt = &I.img;
// 	
// 	(*J_pt).assign(M*N*4, 0.0);
// 	
// 	/* (*J_pt)11 else*/
// 	for(j=0; j<N; j++) {
// 		for(i=0; i<M; i++) {
// 			(*J_pt)[4*j*M+2*i]=(*I_pt)[j*M+i];
// 		}
// 	}
// 	
// 	/* (*J_pt)22 else */
// 	for(j=0; j<N-1; j++) {
// 		for(i=0; i<M-1; i++) {
// 			(*J_pt)[2*M+1 + 4*j*M+2*i]=0.25*((*I_pt)[j*M+i]+(*I_pt)[j*M+i+1]+(*I_pt)[(j+1)*M+i]+(*I_pt)[(j+1)*M+i+1]);
// 		}
// 	}
// 	
// 	/* (*J_pt)21 else */
// 	for(j=0; j<N; j++) {
// 		for(i=0; i<M-1; i++) {
// 			(*J_pt)[1 + 4*j*M+2*i]=0.5*((*I_pt)[j*M+i]+(*I_pt)[j*M+i+1]);
// 		}
// 	}
// 	
// 	/* (*J_pt)12 else*/
// 	for(j=0; j<N-1; j++) {
// 		for(i=0; i<M; i++) {
// 			(*J_pt)[2*M + 4*j*M+2*i]=0.5*((*I_pt)[j*M+i]+(*I_pt)[(j+1)*M+i]);
// 		}
// 	}
// 	
// 	M *= 2;
// 	N *= 2;
// 	J->M=M; J->N=N;
// 	J->img.resize(M*N);
// }
// void CRogerFunction::halveSize(Img* J, const Img I, const int I_c) {
// 	
// 	int M=I.M, N=I.N;
// 	const double* I_pt = &I.img[(I_c-1)*M*N];
// 	
// 	int x=0;
// 	if(M%2 ==1 ) x=2;
// 	
// 	M =(M+1)/2; N =(N+1)/2;
// 	double* J_pt = &J->img[0];
// 	
// 	for(int j=0; j<N; j++) {
// 		for(int i=0; i<M; i++) {
// 			*(J_pt + j*M+i)=*(I_pt + j*(4*M-x)+2*i);
// 		}
// 	}
// 	
// 	J->M=M; J->N=N;
// 	J->img.resize(M*N);
// }
// void CRogerFunction::diffss(Dog* dss, const Gss* ss) {
// 	
// 	dss->smin = ss->smin;
// 	dss->smax = ss->smax-1 ;
// 	dss->omin = ss->omin ;
// 	dss->O = ss->O ;
// 	dss->S = ss->S ;
// 	dss->sigma0 = ss->sigma0 ;
// 	
// 	int S, M, N, MN;
// 	for(int o=0; o<dss->O;o++) {
// 		
// 		S = ss->octave[o].C;
// 		M = ss->octave[o].M;
// 		N = ss->octave[o].N;
// 		MN = M*N;
// 		
// 		Img TMP;
// 		TMP.M=M;
// 		TMP.N=N;
// 		TMP.C=S-1;
// 		TMP.img.assign(MN*(S-1),0);
// 		dss->octave.push_back(TMP);
// 		
// 		for(int s=0; s<S-1; s++) {
// 			for(int p=0; p<MN; p++) {
// 				dss->octave[o].img[s*MN+p] = ss->octave[o].img[(s+1)*MN+p] - ss->octave[o].img[s*MN+p];
// 			}
// 		}
// 	}
// 	//for(int i=0; i<10; i++)
// 	//cout<<dss->octave[6].img[4*MN+i]<<endl;
// 	
// }
// void CRogerFunction::siftlocalmax(Img* Mx, const Img* F, const double threshold) {
// 
// 	int M, N;
// 	const double* F_pt;
// 	int ndims; 
// 	int pdims = -1 ;
// 	int* offsets ;
// 	int* midx ;
// 	int* neighbors ;
// 	int nneighbors ;
// 	int* dims ;
// 	enum {MAXIMA=0} ;
// 	
// 	ndims = 3;
// 	{
// 		/* We need to make a copy because in one special case (see below)
// 		   we need to adjust dims[].
// 		*/
// 		dims = (int*) malloc(sizeof(int)*ndims) ;
// 		dims[0] = F->M; dims[1] = F->N; dims[2] = F->C;
// 	}
// 	M = dims[0] ;
// 	N = dims[1] ;
// 	F_pt = &F->img[0];
// 	
// 	if((ndims == 2) && (pdims < 0) && (M == 1 || N == 1)) {
// 		pdims = 1 ;
// 		M = (M>N)?M:N ;
// 		N = 1 ;
// 		dims[0]=M ;
// 		dims[1]=N ;
// 	}
// 	
// 	if(pdims < 0)
// 		pdims = ndims ;
// 		
// 		
//   /* ------------------------------------------------------------------
//    *   Do the job
//    * --------------------------------------------------------------- */  
//   {
//     int maxima_size = M*N ;
//     int* maxima_start = (int*) malloc(sizeof(int) * maxima_size) ;
//     int* maxima_iterator = maxima_start ;
//     int* maxima_end = maxima_start + maxima_size ;
//     int i,h,o ;
//     const double* pt = F_pt ;
// 
//     /* Compute the offsets between dimensions. */
//     offsets = (int*) malloc(sizeof(int) * ndims) ;
//     offsets[0] = 1 ;
//     for(h = 1 ; h < ndims ; ++h)
//       offsets[h] = offsets[h-1]*dims[h-1] ;
// 
//     /* Multi-index. */
//     midx = (int*) malloc(sizeof(int) * ndims) ;
//     for(h = 0 ; h < ndims ; ++h)
//       midx[h] = 1 ;
// 
//     /* Neighbors. */
//     nneighbors = 1 ;
//     o=0 ;
//     for(h = 0 ; h < pdims ; ++h) {
//       nneighbors *= 3 ;
//       midx[h] = -1 ;
//       o -= offsets[h] ;
//     }
//     nneighbors -= 1 ;
//     neighbors = (int*) malloc(sizeof(int) * nneighbors) ;
// 
//     /* Precompute offsets from offset(-1,...,-1,0,...0) to
//      * offset(+1,...,+1,0,...,0). */
//     i = 0 ;
// 
//     while(true) {
//       if(o != 0)
//         neighbors[i++] = o ;
//       h = 0 ;
//       while( o += offsets[h], (++midx[h]) > 1 ) {
//         o -= 3*offsets[h] ;
//         midx[h] = -1 ;
//         if(++h >= pdims)
//           goto stop ;
//       }
//     }
//   stop: ;
//         
//     /* Starts at the corner (1,1,...,1,0,0,...0) */
//     for(h = 0 ; h < pdims ; ++h) {
//       midx[h] = 1 ;
//       pt += offsets[h] ;
//     }
//     for(h = pdims ; h < ndims ; ++h) {
//       midx[h] = 0 ;
//     }
// 
//     /* ---------------------------------------------------------------
//      *                                                            Loop
//      * ------------------------------------------------------------ */
// 
//     /*
//       If any dimension in the first P is less than 3 elements wide
//       then just return the empty matrix (if we proceed without doing
//       anything we break the carry reporting algorithm below).
//     */
//     for(h=0 ; h < pdims ; ++h)
//       if(dims[h] < 3) goto end ;
//     
//     while(true) {
// 
//       /* Propagate carry along multi index midx */
//       h = 0 ;
//       double v;
//       bool is_greater;
//       while((midx[h]) >= dims[h] - 1) {
//         pt += 2*offsets[h] ; /* skip first and last el. */
//         midx[h] = 1 ;
//         if(++h >= pdims) 
//           goto next_layer ;
//         ++midx[h] ;
//       }
//       
//       /*
//         for(h = 0 ; h < ndims ; ++h ) 
//         mexPrintf("%d  ", midx[h]) ;
//         mexPrintf(" -- %d -- pdims %d \n", pt - F_pt,pdims) ;
//       */
// 
//       /*  Scan neighbors */
//       {		
//         v = *pt ;
//         is_greater = (v >= threshold) ;
//         i = 0  ;
//         while(is_greater && i < nneighbors)
//           is_greater = is_greater && (v > *(pt + neighbors[i++])) ;
//         
//         /* Add the local maximum */
//         if(is_greater) {
//           /* Need more space? */
//           if(maxima_iterator == maxima_end) {
//             maxima_size += M*N ;
//             maxima_start = (int*) realloc(maxima_start, 
//                                             maxima_size*sizeof(int)) ;
//             maxima_end = maxima_start + maxima_size ;
//             maxima_iterator = maxima_end - M*N ;
//           }
//           
//           *maxima_iterator++ = pt - F_pt + 1 ;
//         }
//         
//         /* Go to next element */
//         pt += 1 ;
//         ++midx[0] ;
//         continue ;
//         
//       next_layer: ;
//         if( h >= ndims )
//           goto end ;
//         
//         while((++midx[h]) >= dims[h]) {
//           midx[h] = 0 ;
//           if(++h >= ndims)
//             goto end ;
//         }
//       }
//     }    
//   end:;
//     /* Return. */
//     {
//       double* M_pt ;
//       
//       Mx->img.resize(maxima_iterator-maxima_start+Mx->M*Mx->N*Mx->C, 0);
//       
//       maxima_end = maxima_iterator ;
//       maxima_iterator = maxima_start ;
//       M_pt = &Mx->img[0]+Mx->M*Mx->N*Mx->C;
//       while(maxima_iterator != maxima_end) {
//         *M_pt++ = *maxima_iterator++ ;
//         //cout<<*(M_pt-1)<<endl;
//       }
//       Mx->M=1;
//       Mx->N=maxima_iterator-maxima_start+Mx->M*Mx->N*Mx->C;
//       Mx->C=1;
//     }
//     
//     /* Release space. */
//     free(offsets) ;
//     free(neighbors) ;
//     free(midx) ;
//     free(maxima_start) ;     
//   }
//   free(dims) ;
// 	
// }

// void CRogerFunction::ind2sub(Img* J, const Img* I, Img ndx, Dog* dogss) {
// 	
// 	int n=3;
// 	int k[3];
// 	k[1]=1; k[2]=I->M; k[3]=I->M*I->N;
// 	
// 	J->img.assign(ndx.M*ndx.N*3, 0);
// 	
// 	vector<double> vi(ndx.M*ndx.N), vj(ndx.M*ndx.N);
// 	
// 	int j;
// 	for(j=0; j<ndx.M*ndx.N; j++) {
// 		vi[j] = (int)(ndx.img[j]-1)%k[3] + 1;
// 		vj[j] = (ndx.img[j]-vi[j])/k[3] + 1;
// 		J->img[j*3+2]=vj[j]-1+dogss->smin;
// 	}
// 	ndx.img=vi;
// 	
// 	for(j=0; j<ndx.M*ndx.N; j++) {
// 		vi[j] = (int)(ndx.img[j]-1)%k[2] + 1;
// 		vj[j] = (ndx.img[j]-vi[j])/k[2] + 1;
// 		J->img[j*3+0]=vj[j]-1;
// 	}
// 	ndx.img=vi;
// 	
// 	for(j=0; j<ndx.M*ndx.N; j++) {
// 		vi[j] = (int)(ndx.img[j]-1)%k[1] + 1;
// 		vj[j] = (ndx.img[j]-vi[j])/k[1] + 1;
// 		J->img[j*3+1]=vj[j]-1;
// 	}
// 	
// 	J->M = 3;
// 	J->N = J->img.size()/3;
// 	J->C = 1;
// }
// void CRogerFunction::siftrefinemx(Img* out, Img* P, Img* D, const int smin, const double threshold, const double r) {
// 	
//   int M,N,S,K ;
//   int dimensions[3] ;
//   const double* P_pt ;
//   const double* D_pt ; 
//   double* result ;
//   
//   dimensions[0]=D->M; dimensions[1]=D->N; dimensions[2]=D->C;
//   M = dimensions[0] ;
//   N = dimensions[1] ;
//   S = dimensions[2] ; 
// 
//   K = P->N;
//   P_pt = &P->img[0];
//   D_pt = &D->img[0];
//   /* -----------------------------------------------------------------
//    *                                                        Do the job
//    * -------------------------------------------------------------- */
//   {    
//     double* buffer = (double*) malloc(K*3*sizeof(double)) ;
//     double* buffer_iterator = buffer ;
//     int p ;
//     const int yo = 1 ;
//     const int xo = M ;
//     const int so = M*N ;
//     
//     for(p = 0 ; p < K ; ++p) {
//       int x = ((int)*P_pt++) ;
//       int y = ((int)*P_pt++) ;
//       int s = ((int)*P_pt++) - smin ;
//       
//       int iter ;
//       double b[3] ;
//       
//       /* Local maxima extracted from the DOG
//        * have coorrinates 1<=x<=N-2, 1<=y<=M-2
//        * and 1<=s-mins<=S-2. This is also the range of the points
//        * that we can refine.
//        */
//       if(x < 1 || x > N-2 ||
//          y < 1 || y > M-2 ||
//          s < 1 || s > S-2) {
//         continue ;
//       }
// 
// #define at(dx,dy,ds) (*(pt + (dx)*xo + (dy)*yo + (ds)*so))
// 
//       {
//         const double* pt = D_pt + y*yo + x*xo + s*so ;      
//         double Dx=0,Dy=0,Ds=0,Dxx=0,Dyy=0,Dss=0,Dxy=0,Dxs=0,Dys=0 ;
//         int dx = 0 ;
//         int dy = 0 ;
//         int j, i, jj, ii ;
//         
//         for(iter = 0 ; iter < max_iter ; ++iter) {
// 
//           double A[3*3] ;          
// 
// #define Aat(i,j) (A[(i)+(j)*3])    
// 
//           x += dx ;
//           y += dy ;
//           pt = D_pt + y*yo + x*xo + s*so ;
//           
//           /* Compute the gradient. */
//           Dx = 0.5 * (at(+1,0,0) - at(-1,0,0)) ;
//           Dy = 0.5 * (at(0,+1,0) - at(0,-1,0));
//           Ds = 0.5 * (at(0,0,+1) - at(0,0,-1)) ;
//           
//           /* Compute the Hessian. */
//           Dxx = (at(+1,0,0) + at(-1,0,0) - 2.0 * at(0,0,0)) ;
//           Dyy = (at(0,+1,0) + at(0,-1,0) - 2.0 * at(0,0,0)) ;
//           Dss = (at(0,0,+1) + at(0,0,-1) - 2.0 * at(0,0,0)) ;
//           
//           Dxy = 0.25 * ( at(+1,+1,0) + at(-1,-1,0) - at(-1,+1,0) - at(+1,-1,0) ) ;
//           Dxs = 0.25 * ( at(+1,0,+1) + at(-1,0,-1) - at(-1,0,+1) - at(+1,0,-1) ) ;
//           Dys = 0.25 * ( at(0,+1,+1) + at(0,-1,-1) - at(0,-1,+1) - at(0,+1,-1) ) ;
//           
//           /* Solve linear system. */
//           Aat(0,0) = Dxx ;
//           Aat(1,1) = Dyy ;
//           Aat(2,2) = Dss ;
//           Aat(0,1) = Aat(1,0) = Dxy ;
//           Aat(0,2) = Aat(2,0) = Dxs ;
//           Aat(1,2) = Aat(2,1) = Dys ;
//           
//           b[0] = - Dx ;
//           b[1] = - Dy ;
//           b[2] = - Ds ;
//           
//           /* Gauss elimination */
//           for(j = 0 ; j < 3 ; ++j) {        
//             double maxa    = 0 ;
//             double maxabsa = 0 ;
//             int    maxi    = -1 ;
//             double tmp ;
//             
//             /* look for the maximally stable pivot */
//             for (i = j ; i < 3 ; ++i) {
//               double a    = Aat (i,j) ;
//               double absa = abs (a) ;
//               if (absa > maxabsa) {
//                 maxa    = a ;
//                 maxabsa = absa ;
//                 maxi    = i ;
//               }
//             }
//             
//             /* if singular give up */
//             if (maxabsa < 1e-10f) {
//               b[0] = 0 ;
//               b[1] = 0 ;
//               b[2] = 0 ;
//               break ;
//             }
//             
//             i = maxi ;
//             
//             /* swap j-th row with i-th row and normalize j-th row */
//             for(jj = j ; jj < 3 ; ++jj) {
//               tmp = Aat(i,jj) ; Aat(i,jj) = Aat(j,jj) ; Aat(j,jj) = tmp ;
//               Aat(j,jj) /= maxa ;
//             }
//             tmp = b[j] ; b[j] = b[i] ; b[i] = tmp ;
//             b[j] /= maxa ;
//             
//             /* elimination */
//             for (ii = j+1 ; ii < 3 ; ++ii) {
//               double x = Aat(ii,j) ;
//               for (jj = j ; jj < 3 ; ++jj) {
//                 Aat(ii,jj) -= x * Aat(j,jj) ;                
//               }
//               b[ii] -= x * b[j] ;
//             }
//           }
//           
//           /* backward substitution */
//           for (i = 2 ; i > 0 ; --i) {
//             double x = b[i] ;
//             for (ii = i-1 ; ii >= 0 ; --ii) {
//               b[ii] -= x * Aat(ii,i) ;
//             }
//           }
//           
//           /* If the translation of the keypoint is big, move the keypoint
//            * and re-iterate the computation. Otherwise we are all set.
//            */
//           dx= ((b[0] >  0.6 && x < N-2) ?  1 : 0 )
//             + ((b[0] < -0.6 && x > 1  ) ? -1 : 0 ) ;
//           
//           dy= ((b[1] >  0.6 && y < M-2) ?  1 : 0 )
//             + ((b[1] < -0.6 && y > 1  ) ? -1 : 0 ) ;
//           
//           if( dx == 0 && dy == 0 ) break ;
//           
//         }
//                                 
//         {
//           double val = at(0,0,0) + 0.5 * (Dx * b[0] + Dy * b[1] + Ds * b[2]) ; 
//           double score = (Dxx+Dyy)*(Dxx+Dyy) / (Dxx*Dyy - Dxy*Dxy) ; 
//           double xn = x + b[0] ;
//           double yn = y + b[1] ;
//           double sn = s + b[2] ;
// 
//           if(fabs(val) > threshold &&
//              score < (r+1)*(r+1)/r && 
//              score >= 0 &&
//              fabs(b[0]) < 1.5 &&
//              fabs(b[1]) < 1.5 &&
//              fabs(b[2]) < 1.5 &&
//              xn >= 0 &&
//              xn <= N-1 &&
//              yn >= 0 &&
//              yn <= M-1 &&
//              sn >= 0 &&
//              sn <= S-1) {
//             *buffer_iterator++ = xn ;
//             *buffer_iterator++ = yn ;
//             *buffer_iterator++ = sn+smin  ;
//           }
//         }
//       }
//     }      
// 
//     /* Copy the result into an array. */
//     {
//       int NL = (buffer_iterator - buffer)/3 ;
//       out->img.resize(3*NL, 0);
//       out->M=3; out->N=NL; out->C=1;
//       result = &out->img[0];
//       memcpy(result, buffer, sizeof(double) * 3 * NL) ;
//     }
//     free(buffer) ;
//   }
// 
// 
// }

// void CRogerFunction::siftormx(Img* out, Img* P, Img* G, const int S, const int smin, const double sigma0) {
// 	
//   int M,N,K ;
//   int dimensions[3] ;
//   const double* P_pt ;
//   const double* G_pt ;
//  // double* TH_pt ; 
//   double H_pt [ NBINS ] ;
//   
//   
//   dimensions[0]=G->M; dimensions[1]=G->N; dimensions[2]=G->C;
//   M = dimensions[0] ;
//   N = dimensions[1] ;
//   
//   K = P->N;
//   P_pt = &P->img[0];
//   G_pt = &G->img[0];
//   
//   /* ------------------------------------------------------------------
//    *                                                         Do the job
//    * --------------------------------------------------------------- */ 
//   {
//     int p ;
//     const int yo = 1 ;
//     const int xo = M ;
//     const int so = M*N ;
// 
//     int buffer_size = K*4 ;
//     double* buffer_start = (double*) malloc( buffer_size *sizeof(double)) ;
//     double* buffer_iterator = buffer_start ;
//     double* buffer_end = buffer_start + buffer_size ;
// 
//     for(p = 0 ; p < K ; ++p)//, TH_pt += 2) 
// 	{
//       const double x = *P_pt++ ;
//       const double y = *P_pt++ ;
//       const double s = *P_pt++ ;
//       int xi = ((int) (x+0.5)) ; /* Round them off. */
//       int yi = ((int) (y+0.5)) ;
//       int si = ((int) (s+0.5)) - smin ;
//       int xs ;
//       int ys ;
//       double sigmaw = win_factor * sigma0 * pow(2, ((double)s) / S) ;
//       int W = (int) floor(3.0 * sigmaw) ;
//       int bin ;
//       const double* pt ;
// 
//       /* Make sure that the rounded off keypoint index is within bound.
//        */
//       if(xi < 0   || 
//          xi > N-1 || 
//          yi < 0   || 
//          yi > M-1 || 
//          si < 0   || 
//          si > dimensions[2]-1 ) {
//         printf("Dropping %d: W %d x %d y %d si [%d,%d,%d,%d]\n",p,W,xi,yi,si,M,N,dimensions[2]) ;
//         continue ;
//       }
//       
//       /* Clear histogram buffer. */
//       {
//         int i ;
//         for(i = 0 ; i < NBINS ; ++i) 
//           H_pt[i] = 0 ;
//       }
// 
//       pt = G_pt + xi*xo + yi*yo + si*so ;      
// 
// #define at(dx,dy) (*(pt + (dx)*xo + (dy)*yo))
// 
//       for(xs = max(-W, 1-xi) ; xs <= min(+W, N -2 -xi) ; ++xs) {
//         for(ys = max(-W, 1-yi) ; ys <= min(+W, M -2 -yi) ; ++ys) {
//           double Dx = 0.5 * ( at(xs+1,ys) - at(xs-1,ys) ) ;
//           double Dy = 0.5 * ( at(xs,ys+1) - at(xs,ys-1) ) ;
//           double dx = ((double)(xi+xs)) - x;
//           double dy = ((double)(yi+ys)) - y;
//                                         
//           if(dx*dx + dy*dy >= W*W+0.5) continue ;
// 
//           {
//             double win = exp( - (dx*dx + dy*dy)/(2*sigmaw*sigmaw) ) ;
//             double mod = sqrt(Dx*Dx + Dy*Dy) ;
//             double theta = fmod((float)atan2(Dy, Dx) + 2*PI, 2*PI) ; 
//             bin = (int)( NBINS * theta / (2*PI) ) ;
//             H_pt[bin] += mod*win ;        
//           }
//         }
//       }
//                         
//       /* Smooth histogram */                    
//       {
//         int iter, i ;
// #ifdef LOWE_BUG
//         for (iter = 0; iter < 6; iter++) {
//           double prev  = H_pt[NBINS/2] ;
//           for (i = NBINS/2-1; i >= -NBINS/2 ; --i) {
//             int j  = (i     + NBINS) % NBINS ;
//             int jp = (i - 1 + NBINS) % NBINS ;
//             double newh = (prev + H_pt[j] + H_pt[jp]) / 3.0;
//             prev = H_pt[j] ;
//             H_pt[j] = newh ;
//           }
//         }          
// #else
//         for (iter = 0; iter < 6; iter++) {
//           double prev;
//           prev = H_pt[NBINS-1];
//           for (i = 0; i < NBINS; i++) {
//             double newh = (prev + H_pt[i] + H_pt[(i+1) % NBINS]) / 3.0;
//             prev = H_pt[i] ;
//             H_pt[i] = newh ;
//           }
//         }
// #endif
//       }
//                         
//       /* Find strongest peaks. */
//       {
//         int i ;
//         double maxh = H_pt[0] ;
//         for(i = 1 ; i < NBINS ; ++i)
//           maxh = max(maxh, H_pt[i]) ;
//                                 
//         for(i = 0 ; i < NBINS ; ++i) {
//           double h0 = H_pt[i] ;
//           double hm = H_pt[(i-1+NBINS) % NBINS] ;
//           double hp = H_pt[(i+1+NBINS) % NBINS] ;
//           
//           if( h0 > 0.8*maxh && h0 > hm && h0 > hp ) {
// 
//             double di = -0.5 * (hp-hm) / (hp+hm-2*h0) ; /*di=0;*/
//             double th = 2*PI*(i+di+0.5)/NBINS ;
// 
//             if( buffer_iterator == buffer_end ) {
//               int offset = buffer_iterator - buffer_start ;
//               buffer_size += 4*max(1, K/16) ;
//               buffer_start = (double*) realloc(buffer_start,
//                                                  buffer_size*sizeof(double)) ;
//               buffer_end = buffer_start + buffer_size ;
//               buffer_iterator = buffer_start + offset ;
//             }
//             
//             *buffer_iterator++ = x ;
//             *buffer_iterator++ = y ;
//             *buffer_iterator++ = s ;
//             *buffer_iterator++ = th ;
//           }
//         } /* Scan histogram */
//       } /* Find peaks */
//     }
// 
//     /* Save back the result. */
//     {
//       double* result ;
//       int NL = (buffer_iterator - buffer_start)/4 ;
//       out->img.resize(4*NL, 0);
//       out->M=4; out->N=NL; out->C=1;
//       result = &out->img[0];
//       memcpy(result, buffer_start, sizeof(double) * 4 * NL) ;
//       //for(int i=0; i<4*NL; i++) cout<<out->img[i]<<endl;
//     }
//     free(buffer_start) ;
//   }
//   
// }
// void CRogerFunction::siftdescriptor(Img* out, Img* G, Img* P, double sigma0, int S, int smin, double magnif, int NBP, int NBO) {
// 	
//   int M,N,K,num_levels=0 ;
//   int dimensions[3];
//   const double* P_pt ;
//   const double* G_pt ;
//   float* descr_pt ;
//   float* buffer_pt ;
//   int mode = NOSCALESPACE ;
//   int buffer_size=0;
// 
//   dimensions[0]=G->M; dimensions[1]=G->N; dimensions[2]=G->C;
//   M = dimensions[0] ;
//   N = dimensions[1] ;
//   
//   K = P->N;
//   P_pt = &P->img[0];
//   G_pt = &G->img[0];
//   
//     /* Standard (scale space) mode */ 
//     mode = SCALESPACE ;
//     num_levels = dimensions[2] ;
//     
//   /* -----------------------------------------------------------------
//    *                                   Pre-compute gradient and angles
//    * -------------------------------------------------------------- */
//   /* Alloc two buffers and make sure their size is multiple of 128 for
//    * better alignment (used also by the Altivec code below.)
//    */
//   buffer_size = (M*N*num_levels + 0x7f) & (~ 0x7f) ;
//   buffer_pt = (float*) malloc( sizeof(float) * 2 * buffer_size ) ;
//   descr_pt  = (float*) calloc( NBP*NBP*NBO*K,  sizeof(float)  ) ;
// 
//   {
//     /* Offsets to move in the scale space. */
//     const int yo = 1 ;
//     const int xo = M ;
//     const int so = M*N ;
//     int x,y,s ;
// 
// #define at(x,y) (*(pt + (x)*xo + (y)*yo))
// 
//     /* Compute the gradient */
//     for(s = 0 ; s < num_levels ; ++s) {
//       const double* pt = G_pt + s*so ;
//       for(x = 1 ; x < N-1 ; ++x) {
//         for(y = 1 ; y < M-1 ; ++y) {
//           float Dx = 0.5 * ( at(x+1,y) - at(x-1,y) ) ;
//           float Dy = 0.5 * ( at(x,y+1) - at(x,y-1) ) ;
//           buffer_pt[(x*xo+y*yo+s*so) + 0          ] = Dx ;
//           buffer_pt[(x*xo+y*yo+s*so) + buffer_size] = Dy ;
//         }
//       }
//     }
//     
//     /* Compute angles and modules */
//     {
//       float* pt = buffer_pt ;
//       int j = 0 ;
//       while (j < N*M*num_levels) {
//           float Dx = *(pt              ) ;
//           float Dy = *(pt + buffer_size) ;
//           *(pt              ) = sqrtf(Dx*Dx + Dy*Dy) ;
//           if (*pt > 0) 
//             *(pt + buffer_size) = atan2f(Dy, Dx) ;
//           else
//             *(pt + buffer_size) = 0 ;
//           j += 1 ;
//           pt += 1 ;
//       }
//     }
//   }
// 
// 
//   /* -----------------------------------------------------------------
//    *                                                        Do the job
//    * -------------------------------------------------------------- */ 
//   if(K > 0) {    
//     int p ;
// 
//     /* Offsets to move in the buffer */
//     const int yo = 1 ;
//     const int xo = M ;
//     const int so = M*N ;
// 
//     /* Offsets to move in the descriptor. */
//     /* Use Lowe's convention. */
//     const int binto = 1 ;
//     const int binyo = NBO * NBP ;
//     const int binxo = NBO ;
//     const int bino  = NBO * NBP * NBP ;
// 
//     for(p = 0 ; p < K ; ++p, descr_pt += bino) {
//       /* The SIFT descriptor is a  three dimensional histogram of the position
//        * and orientation of the gradient.  There are NBP bins for each spatial
//        * dimesions and NBO  bins for the orientation dimesion,  for a total of
//        * NBP x NBP x NBO bins.
//        *
//        * The support  of each  spatial bin  has an extension  of SBP  = 3sigma
//        * pixels, where sigma is the scale  of the keypoint.  Thus all the bins
//        * together have a  support SBP x NBP pixels wide  . Since weighting and
//        * interpolation of  pixel is used, another  half bin is  needed at both
//        * ends of  the extension. Therefore, we  need a square window  of SBP x
//        * (NBP + 1) pixels. Finally, since the patch can be arbitrarly rotated,
//        * we need to consider  a window 2W += sqrt(2) x SBP  x (NBP + 1) pixels
//        * wide.
//        */      
//       const float x = (float) *P_pt++ ;
//       const float y = (float) *P_pt++ ;
//       const float s = (float) (mode == SCALESPACE) ? (*P_pt++) : 0.0 ;
//       const float theta0 = (float) *P_pt++ ;
// 
//       const float st0 = sinf(theta0) ;
//       const float ct0 = cosf(theta0) ;
//       const int xi = (int) floor(x+0.5) ; /* Round-off */
//       const int yi = (int) floor(y+0.5) ;
//       const int si = (int) floor(s+0.5) - smin ;
//       const float sigma = sigma0 * powf(2, s / S) ;
//       const float SBP = magnif * sigma ;
//       const int W = (int) floor( sqrt(2.0) * SBP * (NBP + 1) / 2.0 + 0.5) ;      
//       int bin ;
//       int dxi ;
//       int dyi ;
//       const float* pt ;
//       float* dpt ;
//       
//       /* Check that keypoints are within bounds . */
// 
//       if(xi < 0   || 
//          xi > N-1 || 
//          yi < 0   || 
//          yi > M-1 ||
//          ((mode==SCALESPACE) && 
//           (si < 0   ||
//            si > dimensions[2]-1) ) )
//         continue ;
// 
//       /* Center the scale space and the descriptor on the current keypoint. 
//        * Note that dpt is pointing to the bin of center (SBP/2,SBP/2,0).
//        */
//       pt  = buffer_pt + xi*xo + yi*yo + si*so ;
//       dpt = descr_pt + (NBP/2) * binyo + (NBP/2) * binxo ;
//      
// #define atd(dbinx,dbiny,dbint) (*(dpt + (dbint)*binto + (dbiny)*binyo + (dbinx)*binxo))
//       
//       /*
//        * Process each pixel in the window and in the (1,1)-(M-1,N-1)
//        * rectangle.
//        */
//       for(dxi = max(-W, 1-xi) ; dxi <= min(+W, N-2-xi) ; ++dxi) {
//         for(dyi = max(-W, 1-yi) ; dyi <= min(+W, M-2-yi) ; ++dyi) {
// 
//           /* Compute the gradient. */
//           float mod   = *(pt + dxi*xo + dyi*yo + 0           ) ;
//           float angle = *(pt + dxi*xo + dyi*yo + buffer_size ) ;
// #ifdef LOWE_COMPATIBLE         
//           float theta = fast_mod(-angle + theta0) ;
// #else
//           float theta = fast_mod(angle - theta0) ;
// #endif
//           /* Get the fractional displacement. */
//           float dx = ((float)(xi+dxi)) - x;
//           float dy = ((float)(yi+dyi)) - y;
// 
//           /* Get the displacement normalized w.r.t. the keypoint orientation
//            * and extension. */
//           float nx = ( ct0 * dx + st0 * dy) / SBP ;
//           float ny = (-st0 * dx + ct0 * dy) / SBP ; 
//           float nt = NBO * theta / (2*PI) ;
// 
//           /* Get the gaussian weight of the sample. The gaussian window
//            * has a standard deviation of NBP/2. Note that dx and dy are in
//            * the normalized frame, so that -NBP/2 <= dx <= NBP/2. */
//           const float wsigma = NBP/2 ;
//           float win =  expf(-(nx*nx + ny*ny)/(2.0 * wsigma * wsigma)) ;
// 
//           /* The sample will be distributed in 8 adijacient bins. 
//            * Now we get the ``lower-left'' bin. */
//           int binx = fast_floor( nx - 0.5 ) ;
//           int biny = fast_floor( ny - 0.5 ) ;
//           int bint = fast_floor( nt ) ;
//           float rbinx = nx - (binx+0.5) ;
//           float rbiny = ny - (biny+0.5) ;
//           float rbint = nt - bint ;
//           int dbinx ;
//           int dbiny ;
//           int dbint ;
// 
//           /* Distribute the current sample into the 8 adijacient bins. */
//           for(dbinx = 0 ; dbinx < 2 ; ++dbinx) {
//             for(dbiny = 0 ; dbiny < 2 ; ++dbiny) {
//               for(dbint = 0 ; dbint < 2 ; ++dbint) {
//                 
//                 if( binx+dbinx >= -(NBP/2) &&
//                     binx+dbinx <   (NBP/2) &&
//                     biny+dbiny >= -(NBP/2) &&
//                     biny+dbiny <   (NBP/2) ) {
//                   float weight = win 
//                     * mod 
//                     * fabsf(1 - dbinx - rbinx)
//                     * fabsf(1 - dbiny - rbiny)
//                     * fabsf(1 - dbint - rbint) ;
// 
//                       atd(binx+dbinx, biny+dbiny, (bint+dbint) % NBO) += weight ;
//                 }
//               }            
//             }
//           }
//         }  
//       }
// 
// 
//       {
//         /* Normalize the histogram to L2 unit length. */        
//         normalize_histogram(descr_pt, descr_pt + NBO*NBP*NBP) ;
//         
//         /* Truncate at 0.2. */
//         for(bin = 0; bin < NBO*NBP*NBP ; ++bin) {
//           if (descr_pt[bin] > 0.2) descr_pt[bin] = (float)0.2;
//         }
//         
//         /* Normalize again. */
//         normalize_histogram(descr_pt, descr_pt + NBO*NBP*NBP) ;
//       }
//     }
//   }
// 
//   /* Restore pointer to the beginning of the descriptors. */
//   descr_pt -= NBO*NBP*NBP*K ;
// 
//   {
//     int k ;
//     double* L_pt ;
//     out->img.resize(NBP*NBP*NBO*K, 0);
//     out->M=NBP*NBP*NBO; out->N=K; out->C=1;
//     L_pt = &out->img[0];
//     for(k = 0 ; k < NBP*NBP*NBO*K ; ++k) {
//       L_pt[k] = descr_pt[k] ;
//     }
//   }
// 
//   free(descr_pt) ;  
//   free(buffer_pt) ;
// 
// }
// void CRogerFunction::imsmooth(Img* J, const int J_c, const Img I, const int I_c, const double s) {
// 	
// 	int M=I.M, N=I.N;
// 	const double* I_pt = &I.img[(I_c-1)*M*N];
// 	double* J_pt = &J->img[(J_c-1)*M*N];
// 	
// 	int W=(int) ceil(4*s);
// 	double* g0 = (double*) malloc( (2*W+1)*sizeof(double) ) ;
// 	double* buffer = (double*) malloc( M*N*sizeof(double) ) ;
// 	double acc=0.0;
// 	
// 	int j;
// 	for(j=0; j<2*W+1; j++) {
// 		g0[j] = exp(-0.5 * (j - W)*(j - W)/(s*s));
// 		acc += g0[j] ;
// 	}
// 	
// 	for(j=0; j<2*W+1; j++) {
// 		g0[j] /= acc ;
//     }
//     
// 	econvolve(buffer, I_pt, M, N, g0, W) ;
//     econvolve(J_pt, buffer, N, M, g0, W) ;
//     
// 	free(buffer);
// 	free(g0);
// }
// void CRogerFunction::econvolve(double* dst_pt, const double* src_pt, unsigned int M, unsigned int N, const double* filter_pt, int W) {
// 	
// 	int i,j;
// 	for(j=0; j<N; j++) {
// 		for(i=0; i<M; i++) {
// 			double acc=0.0;
// 			double const* g = filter_pt;
// 			double const* start = src_pt+(i-W);
// 			double const* stop;
// 			double        x;
// 			
// 			/* beginning */
// 			stop = src_pt;
// 			x    = *stop ;
// 			while( start <= stop ) { acc += (*g++) * x ; start++ ; }
// 			
// 			/* middle */
// 			stop =  src_pt + min(M-1, i+W) ;
// 			while( start <  stop ) { acc += (*g++) * (*start++) ;  }
// 			
// 			/* end */
// 			x    = *start ;
// 			stop = src_pt + (i+W);
// 			while( start <= stop ) { acc += (*g++) * x ; start++ ; } 
// 			
// 			/* save */
// 			*dst_pt = acc ; 
// 			dst_pt += N ;
// 			
// 			//if(j==N-2) printf("start:%lf\n",acc);
// 		}
// 		/* next column */
// 		src_pt += M ;
// 		dst_pt -= M*N - 1 ;
// 	}
// }
// float CRogerFunction::fast_mod(float th)
// {
// 	while(th < 0) th += (float)(2*PI) ;
// 	while(th > 2*PI) th -= (float)(2*PI) ;
// 	return th ;
// }
// int CRogerFunction::fast_floor(float x)
// {
// 	return (int)( x - ((x>=0)?0:1) ) ; 
// }
// void CRogerFunction::normalize_histogram(float* L_begin, float* L_end)
// {
// 	float* L_iter ;
// 	float norm=0.0 ;
// 	
// 	for(L_iter = L_begin; L_iter != L_end ; ++L_iter)
// 		norm += (*L_iter) * (*L_iter) ;
// 	
// 	norm = sqrtf(norm) ;
// 	/*  mexPrintf("%f\n",norm) ;*/
// 	
// 	for(L_iter = L_begin; L_iter != L_end ; ++L_iter)
// 		*L_iter /= (norm + FLT_EPSILON) ;
// }

// void CRogerFunction::im2double(Img* I) {
// 	
// 	int range[]={0,255};
// 	
// 	for(size_t i = 0; i < I->M*I->N*3; ++i) {
// 		I->img[i] /= range[1];
// 	}
// 	
// }

// void CRogerFunction::rgb2gray(Img* I) {
// 	
// 	double coef[]={0.298936021293776, 0.587043074451121, 0.114020904255103};
// 	
// 	int M=I->M, N=I->N, tmp, tmp3;
// 	for(size_t x = 0; x < N; ++x) {
// 		for(size_t y = 0; y < M; ++y) {
// 			tmp = y+x*M; tmp3 = tmp*3;
// 			I->img[tmp] = I->img[tmp3]*coef[0]+I->img[tmp3+1]*coef[1]+I->img[tmp3+2]*coef[2];
// 			if(I->img[tmp] < 0) I->img[tmp] = 0;
// 			if(I->img[tmp] > 1) I->img[tmp] = 1;
// 			//if(x==9 && y==M-1)
// 			//cout<<I->img[tmp3]<<" "<<I->img[tmp3+1]<<" "<<I->img[tmp3+2]<<endl;
// 		}
// 	}
// 	
// 	I->img.resize(M*N);
// 	
// }

// Pair* compare (Pair* pairs_iterator,
// 			   const double * L1_pt,
// 			   const double * L2_pt,
// 			   int K1, int K2, int ND, float thresh)
// {
//     int k1, k2 ;
//     const double maxval = numeric_limits<double>::infinity();
//     for(k1 = 0 ; k1 < K1 ; ++k1, L1_pt += ND ) {
// 		
// 		double best = maxval ;
// 		double second_best = maxval ;
// 		int bestk = -1 ;
// 		
// 		/* For each point P2[k2] in the second image... */
// 		for(k2 =  0 ; k2 < K2 ; ++k2, L2_pt += ND) {
// 			
// 			int bin ;
// 			double acc = 0 ;
// 			for(bin = 0 ; bin < ND ; ++bin) {
// 				double delta =
// 					((double) L1_pt[bin]) -
// 					((double) L2_pt[bin]) ;
// 				acc += delta*delta ;
// 			}
// 			
// 			/* Filter the best and second best matching point. */
// 			if(acc < best) {
// 				second_best = best ;
// 				best = acc ;
// 				bestk = k2 ;
// 			} else if(acc < second_best) {
// 				second_best = acc ;
// 			}
// 		} 
// 		
// 		L2_pt -= ND*K2 ;                                                  
// 		
// 		/* Lowe's method: accept the match only if unique. */ 
// 		//cout<<thresh * (double) best<<"   "<<(double) second_best<<"   "<<bestk<<endl;
// 		if(thresh * (double) best <= (double) second_best &&
// 			bestk != -1) {
// 			pairs_iterator->k1 = k1 ;
// 			pairs_iterator->k2 = bestk ;
// 			pairs_iterator->score = best ;
// 			pairs_iterator++ ;
// 		}
//     }
// 	
//     return pairs_iterator ;
//   }
// void CRogerFunction::siftmatch(Img* out, Img* L1, Img* L2) {
// 	
// 	int K1, K2, ND ;
// 	double* L1_pt ;
// 	double* L2_pt ;
// 	double thresh = 1.5 ;
// 	
// 	K1=L1->N;
// 	K2=L2->N;
// 	ND=L1->M;
// 	
// 	L1_pt=&L1->img[0];
// 	L2_pt=&L2->img[0];
// 	
// 		Pair* pairs_begin = (Pair*) malloc(sizeof(Pair) * (K1+K2)) ;
// 		Pair* pairs_iterator = pairs_begin ;
// 		
// 		pairs_iterator = compare(pairs_iterator, (const double*) L1_pt, (const double*) L2_pt, K1,K2,ND,thresh) ;
// 		
// 			Pair* pairs_end = pairs_iterator ;
// 			double* M_pt ;
// 			double* D_pt = NULL ;
// 			
// 			out->img.resize(2*(int)(pairs_end-pairs_begin), 0);
// 			out->M=2; out->N=pairs_end-pairs_begin; out->C=1;
// 			
// 			M_pt = &out->img[0] ;
// 			
// 			for(pairs_iterator = pairs_begin ; pairs_iterator < pairs_end ; ++pairs_iterator) 
// 			{
// 				*M_pt++ = pairs_iterator->k1 + 1 ;
// 				*M_pt++ = pairs_iterator->k2 + 1 ;
// 			}
// 		free(pairs_begin) ;
// 
// 	
// }

// void CRogerFunction::siftmain(BYTE* img1, int s_w1, int s_h1, int nEffw1, BYTE* img2, int s_w2, int s_h2, int nEffw2, Img &matches, Img &P1, Img &P2)
// {
// 	
// 	Img I1; //img double向量, 整數M, N, C
// 
// // ...imread
// 	int x, y, z, w;
// 	int i;
// 
// 	I1.M = s_h1; //第一張 高
// 	I1.N = s_w1; //第二張 寬
// 	I1.C = 1;
// 	I1.img.assign(I1.M * I1.N * 3, 0.0); //assign a element to a vector
// 
// 	// read body 第一張影像讀進 I1
// 	int tmp; 
// 
// 	for( y = I1.M-1, w=0; y >= 0; y--, w++) {
// 		for( x = 0, z=0; x < I1.N; ++x, z+=3) 
// 		{
// 			tmp=(y+x*I1.M)*3;
// 			I1.img[tmp+2] = img1[w*nEffw1+z+2];
// 			I1.img[tmp+1] = img1[w*nEffw1+z+1];
// 			I1.img[tmp  ] = img1[w*nEffw1+z  ];
// 		}
// 	}
// 
// //.......
// 	im2double(&I1); //影像pixel格式 BYTE -> double
// 	rgb2gray(&I1);  //影像格式 RGB -> GRAY
// 		
// //....................................
// 	Img I2;
// 	// ...imread
// 	I2.M = s_h2;
// 	I2.N = s_w2;
// 	I2.C = 1;
// 	I2.img.assign(I2.M * I2.N * 3, 0.0);
// 
// 	// read body
// 	for( y = I2.M-1, w=0; y >= 0; y--, w++) {
// 		for( x = 0, z=0; x < I2.N; ++x, z+=3) 
// 		{
// 			tmp=(y+x*I2.M)*3;
// 			I2.img[tmp+2] = img2[w*nEffw2+z+2];
// 			I2.img[tmp+1] = img2[w*nEffw2+z+1];
// 			I2.img[tmp  ] = img2[w*nEffw2+z  ];
// 		}
// 	}
// 
// 	im2double(&I2);
// 	rgb2gray(&I2);
// 	
// 	//........上面二張影像轉換後, 由以下開始處理SIFT.....
// 
// // 	typedef struct {
// // 		Img frames;
// // 		Img descriptors;
// // 		Gss gss;
// // 		Dog dogss;
// // 	} SiftInfo;
// 
// 	SiftInfo M1;
// 	sift(&M1, I1);
// 	
// 	SiftInfo M2;
// 	sift(&M2, I2);
// 	
// 	for(i=0; i< M1.descriptors.M * M1.descriptors.N; i++)
// 	{
// 		M1.descriptors.img[i] = round(512 * M1.descriptors.img[i]);
// 	}
// 	for(i=0; i< M2.descriptors.M * M2.descriptors.N; i++)
// 	{
// 		M2.descriptors.img[i] = round(512 * M2.descriptors.img[i]);
// 	}
// 
// 	siftmatch(&matches, &M1.descriptors, &M2.descriptors);
// 	P1 = M1.frames;
// 	P2 = M2.frames;
// }
void CRogerFunction::I_DOG(BYTE *in_image, BYTE * out_image, int s_w, int s_h, double Delta)
{
	//initial variable....................................................................
	int i;
	int size = s_w * s_h;
	
	BYTE *Buffer1_1 = new BYTE[size];

	BYTE *DOG1_1 = new BYTE[size];

//...原圖處理 first octave
	I_GaussianSmooth1(  in_image, Buffer1_1, s_w, s_h, Delta);

	for (i=0; i<s_w*s_h; i++)
	{
		DOG1_1[i] = abs(Buffer1_1[i] - in_image[i]);
	}

//原尺寸100%影像輸出
	memcpy(out_image,DOG1_1,sizeof(BYTE)*s_w*s_h);

	delete []Buffer1_1;
	delete []DOG1_1;
}
void CRogerFunction::I_ColorCarExtraction(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	float Zp;
	float Rp, Gp, Bp;

	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
			Rp = in_image[k+2];
			Gp = in_image[k+1];
			Bp = in_image[k  ];

			Zp = (Rp+Gp+Bp);
			Rp = Rp / Zp;
			Gp = Gp / Zp;
			Bp = Bp / Zp;
			
		if( Rp >=0.329568 && Rp <=0.368768 && Gp >=0.326464 && Gp <=0.342835 && Bp >= 0.289799 && Bp <=0.335829)
			{
				out_image[k]  =0;
				out_image[k+1]=255;
				out_image[k+2]=255;	
			}
			else
			{
				out_image[k]=in_image[k];
				out_image[k+1]=in_image[k+1];
				out_image[k+2]=in_image[k+2];
			}
		}		
	}
} 
void CRogerFunction::I_GradMagOut1(BYTE *in_image, BYTE *out_image, int s_w, int s_h) //以一個 byte輸入 輸出
{
	int i, j;
	int nPixel = s_w * s_h;
	BYTE *pdTmp;
	pdTmp = new BYTE[nPixel];

	memcpy( pdTmp, in_image, nPixel);
	//...計算方向導數
	int *pnGradX, *pnGradY;
	int x,y;
	pnGradX		= new int[s_w*s_h];
	pnGradY		= new int[s_w*s_h];

	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
	}
	//梯度計算
	for(y=0; y<s_h; y++) //x方向
	{
		for(x=0; x<s_w; x++)
		{
			pnGradX[y*s_w+x] = (int) (pdTmp[y*s_w+min(s_w-1,x+1)]  
				- pdTmp[y*s_w+max(0,x-1)]);
		}
	}
	for(x=0; x<s_w; x++) //y方向
	{
		for(y=0; y<s_h; y++)
		{
			pnGradY[y*s_w+x] = (int) ( pdTmp[min(s_h-1,y+1)*s_w + x]  
				- pdTmp[max(0,y-1)*s_w+ x ]);
		}
	}
	//...End 計算方向導數
	for(i=0; i<s_h; i++)
	{
		for(j=0; j<s_w; j++)
		{
			//梯度強度
			out_image[i*s_w+j] = sqrtf( pnGradX[i*s_w+j]*pnGradX[i*s_w+j]+pnGradY[i*s_w+j]*pnGradY[i*s_w+j] );
		}
	}
	delete []pdTmp;
	delete []pnGradX;
	delete []pnGradY;
}

void CRogerFunction::I_GradMagOut1_180(BYTE *in_image, BYTE *out_image, int *outDirect_image, int s_w, int s_h) //以一個 byte輸入 輸出
{
	int i;
	int nPixel = s_w * s_h;
	double dSqtOne;
	double dSqtTwo;
	int Theta;
	
	BYTE *pdTmp = new BYTE[nPixel];
	
	memcpy( pdTmp, in_image, nPixel);

	int *pnGradX = new int[s_w*s_h];
	int *pnGradY = new int[s_w*s_h];
	int x,y;

	BYTE *p_in_image  = in_image;
	BYTE *p_out_image = out_image;
	int  *p_pnGradX   = pnGradX;
	int  *p_pnGradY   = pnGradY;
	BYTE *p_pdTmp     = pdTmp;
	int *p_outDirect_image = outDirect_image;

	//...計算方向導數	
	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
	}
	//梯度計算
	for(y=0; y<s_h; y++, p_pnGradX+=s_w, p_pdTmp+=s_w) //x方向
	{
		for(x=0; x<s_w; x++)
		{
			p_pnGradX[x] = (int) (p_pdTmp[min(s_w-1,x+1)] - p_pdTmp[max(0,x-1)]);
		}
	}
	p_pdTmp = pdTmp, p_pnGradX=pnGradX;
	for(x=0; x<s_w; x++) //y方向
	{
		for(y=0; y<s_h; y++)
		{
			pnGradY[y*s_w+x] = (int) ( pdTmp[min(s_h-1,y+1)*s_w + x] - pdTmp[max(0,y-1)*s_w+ x ]);
		}
	}
	//...End 計算方向導數
	
	//...計算梯度
	for(y=0; y<s_h; y++, p_pnGradX+=s_w, p_pnGradY+=s_w, p_out_image+=s_w, p_outDirect_image+=s_w)
	{
		for(x=0; x<s_w; x++)
		{
			dSqtOne = p_pnGradX[x] * p_pnGradX[x];
			dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
			out_image[y*s_w + x] = (int)(sqrt(dSqtOne + dSqtTwo)/* + 0.5*/);
			//................
				if (p_pnGradX[x] ==0 && p_pnGradY[x] !=0)  // y 方向
				{
					p_outDirect_image[x]   = 90;
				}
				else if (p_pnGradX[x] !=0 && p_pnGradY[x] ==0) // x 方向
				{
					p_outDirect_image[x]   = 0;
				}
				else if ((p_pnGradX[x] > 0 && p_pnGradY[x] > 0) || (p_pnGradX[x] < 0 && p_pnGradY[x] < 0)) // 第一三象限
				{
					Theta = atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3;
					p_outDirect_image[x]   = Theta; 
				}
				else // 第二四象限
				{
					Theta = atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3;
					Theta = 180 + Theta;
					p_outDirect_image[x]   = Theta; 
				}
				///使用代碼表示
				if ( p_outDirect_image[x] < 36 ) 
					p_outDirect_image[x] = 1;
				else if( p_outDirect_image[x] >= 36 && p_outDirect_image[x] < 72 ) 
					p_outDirect_image[x] = 2;
				else if( p_outDirect_image[x] >= 72 && p_outDirect_image[x] < 108 ) 
					p_outDirect_image[x] = 3;
				else if( p_outDirect_image[x] >= 108 && p_outDirect_image[x] < 144 ) 
					p_outDirect_image[x] = 4;
				else 
					p_outDirect_image[x] = 5;
			}
			//................
	}
	//...End計算梯度
	delete []pdTmp;
	delete []pnGradX;
	delete []pnGradY;
}
void CRogerFunction::I_GradMagOut1_180(BYTE *in_image, BYTE *out_image, int *outDirect_image, int s_w, int s_h, int Dimemsion) //以一個 byte輸入 輸出
{
	int i;
	int nPixel = s_w * s_h;
	double dSqtOne;
	double dSqtTwo;
	int Theta;
	int nInterval = 180 / Dimemsion;
	
	BYTE *pdTmp = new BYTE[nPixel];
	
	memcpy( pdTmp, in_image, nPixel);
	
	int *pnGradX = new int[s_w*s_h];
	int *pnGradY = new int[s_w*s_h];
	int x,y;
	
	BYTE *p_in_image  = in_image;
	BYTE *p_out_image = out_image;
	int  *p_pnGradX   = pnGradX;
	int  *p_pnGradY   = pnGradY;
	BYTE *p_pdTmp     = pdTmp;
	int *p_outDirect_image = outDirect_image;
	
	//...計算方向導數	
	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
	}
	//梯度計算
	for(y=0; y<s_h; y++, p_pnGradX+=s_w, p_pdTmp+=s_w) //x方向
	{
		for(x=0; x<s_w; x++)
		{
			p_pnGradX[x] = (int) (p_pdTmp[min(s_w-1,x+1)] - p_pdTmp[max(0,x-1)]);
			//gpMsgbar->ShowMessage("%d %d %d\n",p_pnGradX[x],p_pdTmp[min(s_w-1,x+1)],p_pdTmp[max(0,x-1)]);
		}
	}
	p_pdTmp = pdTmp, p_pnGradX=pnGradX;
	for(x=0; x<s_w; x++) //y方向
	{
		for(y=0; y<s_h; y++)
		{
			pnGradY[y*s_w+x] = (int) ( pdTmp[min(s_h-1,y+1)*s_w + x] - pdTmp[max(0,y-1)*s_w+ x ]);
			
		}
	}
	//...End 計算方向導數
	
	//...計算梯度
	for(y=0; y<s_h; y++, p_pnGradX+=s_w, p_pnGradY+=s_w, p_out_image+=s_w, p_outDirect_image+=s_w)
	{
		for(x=0; x<s_w; x++)
		{
			dSqtOne = p_pnGradX[x] * p_pnGradX[x];
			dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
			out_image[y*s_w + x] = (int)(sqrt(dSqtOne + dSqtTwo)/* + 0.5*/);
			//................
			if (p_pnGradX[x] ==0 && p_pnGradY[x] !=0)  // y 方向
			{
				p_outDirect_image[x]   = 90;
			}
			else if (p_pnGradX[x] !=0 && p_pnGradY[x] ==0) // x 方向
			{
				p_outDirect_image[x]   = 0;
			}
			else if ((p_pnGradX[x] > 0 && p_pnGradY[x] > 0) || (p_pnGradX[x] < 0 && p_pnGradY[x] < 0)) // 第一三象限
			{
				Theta = atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3;
				p_outDirect_image[x]   = Theta; 
			}
			else // 第二四象限
			{
				Theta = atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3;
				Theta = 180 + Theta;
				p_outDirect_image[x]   = Theta; 
			}
			///使用代碼表示
			p_outDirect_image[x] = p_outDirect_image[x]/nInterval+1;
		}
		//................
	}
	//...End計算梯度
	delete []pdTmp;
	delete []pnGradX;
	delete []pnGradY;
}
void CRogerFunction::I_GradMagOut1_360(BYTE *in_image, BYTE *out_image, int *outDirect_image, int s_w, int s_h) //以一個 byte輸入 輸出
{
	int i;
	int nPixel = s_w * s_h;
	double dSqtOne;
	double dSqtTwo;
	int Theta;

	BYTE *pdTmp;
	pdTmp = new BYTE[nPixel];
	
	memcpy( pdTmp, in_image, nPixel);
	//...計算方向導數
	int *pnGradX, *pnGradY;
	int x,y;
	pnGradX		= new int[s_w*s_h];
	pnGradY		= new int[s_w*s_h];
	
	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
	}
	//梯度計算
	for(y=0; y<s_h; y++) //x方向
	{
		for(x=0; x<s_w; x++)
		{
			pnGradX[y*s_w+x] = (int) (pdTmp[y*s_w+min(s_w-1,x+1)]  
				- pdTmp[y*s_w+max(0,x-1)]);
		}
	}
	for(x=0; x<s_w; x++) //y方向
	{
		for(y=0; y<s_h; y++)
		{
			pnGradY[y*s_w+x] = (int) ( pdTmp[min(s_h-1,y+1)*s_w + x]  
				- pdTmp[max(0,y-1)*s_w+ x ]);
		}
	}
	//...End 計算方向導數

	//...計算梯度
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			dSqtOne = pnGradX[y*s_w + x] * pnGradX[y*s_w + x];
			dSqtTwo = pnGradY[y*s_w + x] * pnGradY[y*s_w + x];
			out_image[y*s_w + x] = (int)(sqrt(dSqtOne + dSqtTwo) + 0.5);
			//................
			if (out_image[y*s_w+x] != 0)
			{
				if (pnGradX[y*s_w + x] > 0 && pnGradY[y*s_w + x] ==0)
				{
					outDirect_image[y*s_w + x]   = 0;
				}				
				else if (pnGradX[y*s_w + x] == 0 && pnGradY[y*s_w + x] > 0)  
				{
					outDirect_image[y*s_w + x]   = 90;
				}
				else if (pnGradX[y*s_w + x] < 0 && pnGradY[y*s_w + x] == 0)  
				{
					outDirect_image[y*s_w + x]   = 180;
				}
				else if (pnGradX[y*s_w + x] == 0 && pnGradY[y*s_w + x] < 0) 
				{
					outDirect_image[y*s_w + x]   = 270;
				}
				else if (pnGradX[y*s_w + x] > 0 && pnGradY[y*s_w + x] > 0) // 第一象限
				{
					Theta = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3;
					outDirect_image[y*s_w + x]   = Theta; 
				}
				else if (pnGradX[y*s_w + x] < 0 && pnGradY[y*s_w + x] > 0) // 第二象限
				{
					Theta = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3;
					outDirect_image[y*s_w + x]   = Theta + 180; 
				}
				else if (pnGradX[y*s_w + x] < 0 && pnGradY[y*s_w + x] < 0) // 第三象限
				{
					Theta = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3;
					outDirect_image[y*s_w + x]   = Theta + 180; 
				}
				else // 第四象限
				{
					Theta = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3;
					outDirect_image[y*s_w + x]   = Theta + 360;    
				}
			}
			//................
		}
	}
//...End計算梯度
	delete []pdTmp;
}
void CRogerFunction::I_GradDirectionOut(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw) //3 bytes 輸入輸出
{
	int i, j, k;
	int nPixel = s_w * s_h;
	//先將 Size -> Pixel
	BYTE *pdTmp, *pdTmp1;
	pdTmp = new BYTE[nPixel];
	pdTmp1= new BYTE[nPixel];
	
	for (i=0;i<s_h;i++) 
	{
		for (j=0,  k=0; j<s_w ; j++, k+=3)        
		{
			pdTmp[i*s_w+j]=(in_image[i*s_Effw+k]+in_image[i*s_Effw+k+1]+in_image[i*s_Effw+k+2])/3;
		}
	}

	//...計算方向導數
	int *pnGradX, *pnGradY;
	int x,y;
	float *pnGradMAG;
	int TPix=0, Cnt90 =0, Cnt00 =0, Cnt45=0, Cnt135=0;
	pnGradX		= new int[s_w*s_h];
	pnGradY		= new int[s_w*s_h];
	pnGradMAG	= new float[s_w*s_h];
	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
		pnGradMAG[i]=0;
	}
	//梯度計算
	for(y=0; y<s_h; y++) //x方向
	{
		for(x=0; x<s_w; x++)
		{
			pnGradX[y*s_w+x] = (int) (pdTmp[y*s_w+min(s_w-1,x+1)]  
				- pdTmp[y*s_w+max(0,x-1)]     );
		}
	}
	for(x=0; x<s_w; x++) //y方向
	{
		for(y=0; y<s_h; y++)
		{
			pnGradY[y*s_w+x] = (int) ( pdTmp[min(s_h-1,y+1)*s_w + x]  
				- pdTmp[max(0,y-1)*s_w+ x ]     );
		}
	}
	//...End 計算方向導數
	//...計算梯度強度
	for(i=0; i<s_h; i++)
	{
		for(j=0; j<s_w; j++)
		{
			pnGradMAG[i*s_w+j] = sqrtf( pnGradX[i*s_w+j]*pnGradX[i*s_w+j]+pnGradY[i*s_w+j]*pnGradY[i*s_w+j] );
		}
	}
	//...End 計算梯度強度

	for(i=0; i<s_h; i++, out_image+=s_Effw)
	{
		for(j=0, k=0; j<s_w; j++, k+=3)
		{
			if(pnGradMAG[i*s_w+j] > 10)
			{
					if (pnGradX[i*s_w+j] ==0 && pnGradY[i*s_w+j] !=0)  // y 方向 0 度
					{
						out_image[k]   = 255;
						out_image[k+1] = 255;
						out_image[k+2] = 255;
					}
					else if (pnGradX[i*s_w+j] !=0 && pnGradY[i*s_w+j] ==0) // x 方向 90 度
					{
						out_image[k]   = 255;
						out_image[k+1] = 255;
						out_image[k+2] = 255;
					}
					else if ((pnGradX[i*s_w+j]*pnGradY[i*s_w+j]) > 0) //介於 第一象限 與 第三象限
					{
// 						Theta = atanf((double)pnGradX[i*s_w+j]/(double)pnGradY[i*s_w+j]); // 角度運算  第一象限 1 - 89
// 						Theta = Theta * 57.3;
							if (pnGradX[i*s_w+j] >0 && pnGradY[i*s_w+j]>0 ) //介於 第一象限
							{
								out_image[k]   = 0;    //紅色
								out_image[k+1] = 0;
								out_image[k+2] = 255;	
							}
							else //介於  第三象限 
							{
								out_image[k]   = 0;  //黃色
								out_image[k+1] = 255;
								out_image[k+2] = 255;
							}
					}
					else if ((pnGradX[i*s_w+j]*pnGradY[i*s_w+j]) < 0) //介於 第二象限 與 第四象限
					{
// 						Theta = atanf((double)pnGradX[i*s_w+j]/(double)pnGradY[i*s_w+j]);  
// 						Theta = Theta * 57.3;
							if (pnGradX[i*s_w+j]>0 && pnGradY[i*s_w+j]<0 ) //介於 第四象限
							{
								out_image[k]   = 0;  //綠色
								out_image[k+1] = 255;
								out_image[k+2] = 0;	
							}
							else //介於 第二象限
							{
								out_image[k]   = 255; //青色
								out_image[k+1] = 255;
								out_image[k+2] = 0;
							}
					}
			}
			else
			{
				out_image[k]   = 0; 
				out_image[k+1] = 0;
				out_image[k+2] = 0;
			}

		}
	}

	//將Pixel -> Size
// 	for (i=0;i<s_h;i++, out_image+=s_Effw) 
// 	{
// 		for (j=0,  k=0; j<s_w ; j++, k+=3)        
// 		{
// 			out_image[k] = out_image[k+1] = out_image[k+2] = pdTmp1[i*s_w+j];
// 		}
// 	}	

	delete []pdTmp;
	delete []pdTmp1;
	delete []pnGradX	;
	delete []pnGradY;
	delete []pnGradMAG;
}
void CRogerFunction::I_GradDirectionMap(BYTE *in_image, float *out_image, int s_w, int s_h) //3 bytes 輸入輸出
{

	int i, j, nNo=0;
	int nPixel = s_w * s_h;
	//先將 Size -> Pixel
	BYTE *pdTmp, *pdTmp1;
	pdTmp = new BYTE[nPixel];
	pdTmp1= new BYTE[nPixel];

	ofstream	fileOutput; 
	fileOutput.open("0TotPxOutput.txt", ios::out);

	//...計算方向導數
	int *pnGradX, *pnGradY;
	float Theta;
	int x,y;
	float *pnGradMAG;
	int TPix=0, Cnt90 =0, Cnt00 =0, Cnt45=0, Cnt135=0;
	pnGradX		= new int[s_w*s_h];
	pnGradY		= new int[s_w*s_h];
	pnGradMAG	= new float[s_w*s_h];
	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
		pnGradMAG[i]=0;
	}
	//梯度計算
	for(y=0; y<s_h; y++) //x方向
		for(x=0; x<s_w; x++)
			pnGradX[y*s_w+x] = (int) (in_image[y*s_w+min(s_w-1,x+1)] - in_image[y*s_w+max(0,x-1)] );
	for(x=0; x<s_w; x++) //y方向
		for(y=0; y<s_h; y++)
			pnGradY[y*s_w+x] = (int) ( in_image[min(s_h-1,y+1)*s_w + x] - in_image[max(0,y-1)*s_w+ x ] );
	//計算梯度強度
	for(i=0; i<s_h; i++)
		for(j=0; j<s_w; j++)
			pnGradMAG[i*s_w+j] = sqrtf( pnGradX[i*s_w+j]*pnGradX[i*s_w+j] + pnGradY[i*s_w+j]*pnGradY[i*s_w+j] );

	for(i=0; i<s_h; i++)
	{
		for(j=0; j<s_w; j++)
		{
			if(pnGradMAG[i*s_w+j] > 10)
			{
				nNo++;

					if (pnGradX[i*s_w+j] ==0 && pnGradY[i*s_w+j] !=0)  // y 方向
					{
						out_image[i*s_w+j]   = 90;
					}
					else if (pnGradX[i*s_w+j] !=0 && pnGradY[i*s_w+j] ==0) // x 方向
					{
						out_image[i*s_w+j]   = 0;
					}
					else if (pnGradX[i*s_w+j]>0 && pnGradY[i*s_w+j] > 0) //介於 第一象限
					{
						Theta = atanf((double)pnGradX[i*s_w+j]/(double)pnGradY[i*s_w+j]); // 角度運算  第一象限 1 - 89
						Theta = Theta * 57.3;
						out_image[i*s_w+j]   = Theta;    
					}
					else if (pnGradX[i*s_w+j]<0 && pnGradY[i*s_w+j] < 0) //介於 第三象限
					{
						Theta = atanf((double)pnGradX[i*s_w+j]/(double)pnGradY[i*s_w+j]); // 角度運算  第一象限 1 - 89
						Theta = -90 - Theta * 57.3;
						out_image[i*s_w+j]   =  Theta;    
					}
					else if (pnGradX[i*s_w+j]<0 && pnGradY[i*s_w+j] > 0) //介於 第二象限
					{
						Theta = atanf((double)pnGradX[i*s_w+j]/(double)pnGradY[i*s_w+j]);  
						Theta = 90 - Theta * 57.3;
						out_image[i*s_w+j]   = Theta;  
					}
					else if (pnGradX[i*s_w+j] >0 && pnGradY[i*s_w+j] <0) //介於 第四象限
					{
						Theta = atanf((double)pnGradX[i*s_w+j]/(double)pnGradY[i*s_w+j]);  
						Theta = Theta * 57.3;
						out_image[i*s_w+j]   = Theta;  
					}
					fileOutput << "(" << j << "," << i << ")" << "<--" << out_image[i*s_w+j] << endl;

			}
			else
			{
				out_image[i*s_w+j]   = 400; 
			}

		}
	}
	fileOutput << nNo << endl;
	fileOutput.close();
	delete []pdTmp;
	delete []pdTmp1;
	delete []pnGradX;
	delete []pnGradY;
	delete []pnGradMAG;
}
void CRogerFunction::I_RGB2C1C2C3(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	float r, g, b;
	float hue;
	float c1, c2 ,c3;
	
	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w ;j++, k+=3)           //j控制行(寬)
		{
			r = in_image[k+2];
			g = in_image[k+1];
			b = in_image[k  ];
			
			c1 = atan(r/max(g,b));
			c2 = atan(g/max(r,b));
			c3 = atan(b/max(r,g));

			hue = (float) atan((double)( pow(3, 0.5)*(g-b)/((r-g)+(r-b))));
			hue = hue * 57.3;
			c1 = c1 * 57.3;
			c2 = c2 * 57.3;
			c3 = c3 * 57.3;

				out_image[k]  = (int)c1;
				out_image[k+1]=	(int)c1;
				out_image[k+2]=	(int)c1;	
		}		
	}
}
void CRogerFunction::I_RGB2m1m2m3(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	float r1, g1, b1;
	float r2, g2, b2;
	float m1, m2, m3;
	
	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w-1 ;j++, k+=3)           //j控制行(寬)
		{
			r1 = in_image[k+2];
			g1 = in_image[k+1];
			b1 = in_image[k  ];

			r2 = in_image[k+5];
			g2 = in_image[k+4];
			b2 = in_image[k+3];

			m1 = (r1*g2)/(r2*g1);
			m2 = (r1*b2)/(r2*b1);
			m3 = (g1*b2)/(g2*b1);
			
			m1 = m1 * 255;

			out_image[k]  = (int)m1;
			out_image[k+1]=	(int)m1;
			out_image[k+2]=	(int)m1;	
		}		
	}
}

void CRogerFunction::I_LowPassFilter(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{
	int i, j;
	
	for (i=1; i<s_h-1 ;i++) //i控制列(高)
	{
		for (j=1; j<s_w-1 ;j++)           //j控制行(寬)
		{
			out_image[i*s_w+j] = (	in_image[(i-1)*s_w+(j-1)] +
									in_image[(i-1)*s_w+(j  )] +
									in_image[(i-1)*s_w+(j+1)] +
									in_image[(i  )*s_w+(j-1)] +
									in_image[(i  )*s_w+(j  )] +
									in_image[(i  )*s_w+(j+1)] +
									in_image[(i+1)*s_w+(j-1)] +
									in_image[(i+1)*s_w+(j  )] +
									in_image[(i+1)*s_w+(j+1)] ) / 9 ;
		}		
	}
}
void CRogerFunction::I_HighPassFilter(BYTE *in_image, BYTE *out_image, int s_w, int s_h)
{
	int i, j;
	
	for (i=1; i<s_h-1 ;i++) //i控制列(高)
	{
		for (j=1; j<s_w-1 ;j++)           //j控制行(寬)
		{
			out_image[i*s_w+j] = (	-1 * in_image[(i-1)*s_w+(j-1)] +
									-1 * in_image[(i-1)*s_w+(j  )] +
									-1 * in_image[(i-1)*s_w+(j+1)] +
									-1 * in_image[(i  )*s_w+(j-1)] +
									 8 * in_image[(i  )*s_w+(j  )] +
									-1 * in_image[(i  )*s_w+(j+1)] +
									-1 * in_image[(i+1)*s_w+(j-1)] +
									-1 * in_image[(i+1)*s_w+(j  )] +
									-1 * in_image[(i+1)*s_w+(j+1)] )  ;
		}		
	}
}

void CRogerFunction::I_RGBNormalization(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	float r, g, b, z;

	
	for (i=0; i<s_h ;i++, in_image+=s_Effw, out_image+=s_Effw) //i控制列(高)
	{
		for (j=0, k=0; j<s_w-1 ;j++, k+=3)           //j控制行(寬)
		{
			r = in_image[k+2];
			g = in_image[k+1];
			b = in_image[k  ];
			z = r + g + b;
			r = r / z * 255;
			g = g / z * 255;
			b = b / z * 255;
			if ( z != 0 )
			{
				out_image[k]  = b +z/3;
				out_image[k+1]=	g +z/3;
				out_image[k+2]=	r +z/3;
			}
			
	
		}		
	}
}
void CRogerFunction::I_ShadeEnhance(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	int nSize = s_h * s_Effw;
	float r1, g1, b1, Dr, Dg, Db;
	BYTE *pBuffer3_1 = new BYTE[nSize];
	BYTE *pBuffer3_2 = new BYTE[nSize];
	BYTE *pBuffer3_3 = new BYTE[nSize];	

	memcpy(pBuffer3_1, in_image, nSize);
	for (i=0;i<s_h;i++, in_image+=s_Effw) 
	{
		for (j=0,  k=0; j<s_w ; j++, k+=3)        
		{
			pBuffer3_2[i*s_Effw+k]   = in_image[k]   + 38 ;
			pBuffer3_2[i*s_Effw+k+1] = in_image[k+1] + 49;
			pBuffer3_2[i*s_Effw+k+2] = in_image[k+2] + 61 ;
		}
	}


// 	//輸出顯示模組
	for (i=0; i<s_h; i++) 
	{
		for (j=0,  k=0; j<s_w; j++, k+=3)        
		{
			r1 = pBuffer3_2[i*s_Effw+k+2];
			g1 = pBuffer3_2[i*s_Effw+k+1];
			b1 = pBuffer3_2[i*s_Effw+k  ];
			Dr = abs(pBuffer3_2[i*s_Effw+k+2] - pBuffer3_1[i*s_Effw+k+2]);
			Dg = abs(pBuffer3_2[i*s_Effw+k+1] - pBuffer3_1[i*s_Effw+k+1]);
			Db = abs(pBuffer3_2[i*s_Effw+k  ] - pBuffer3_1[i*s_Effw+k  ]); 
			if ( Dr <= 61 && Dg <= 49 && Db <=38
				&&
				r1 >= 114 && r1<=162 &&
				g1 >= 114 && g1<=160 &&
				b1 >= 103 && b1<=150 )
			{
				pBuffer3_3[i*s_Effw+k] = pBuffer3_3[i*s_Effw+k+1] = pBuffer3_3[i*s_Effw+k+2] = 0 ;
			}

			else
			{
				pBuffer3_3[i*s_Effw+k] = pBuffer3_3[i*s_Effw+k+1] = pBuffer3_3[i*s_Effw+k+2] = 255 ;
			}

			
		}
	}
	for (i=0;i<s_h;i++) 
	{
		for (j=0, k=0; j<s_w ; j++, k+=3)        
		{
			if (pBuffer3_3[i*s_Effw+k] == 255)
			{
				out_image[i*s_Effw+k] = pBuffer3_1[i*s_Effw+k];
				out_image[i*s_Effw+k+1] = pBuffer3_1[i*s_Effw+k+1];
				out_image[i*s_Effw+k+2] = pBuffer3_1[i*s_Effw+k+2];
			}
			else
			{
				out_image[i*s_Effw+k] = pBuffer3_1[i*s_Effw+k] + 38;
				out_image[i*s_Effw+k+1] = pBuffer3_1[i*s_Effw+k] +49;
				out_image[i*s_Effw+k+2] = pBuffer3_1[i*s_Effw+k] +61;
			}
		}
	}
	delete []pBuffer3_1;
	delete []pBuffer3_2;
	delete []pBuffer3_3;
}
void CRogerFunction::I_ShadeDet(BYTE *in_image1, BYTE *in_image2, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	float r1, g1, b1, Dr, Dg, Db;
	
	//輸出顯示模組
	for (i=0; i<s_h; i++, out_image+=s_Effw, in_image1+=s_Effw, in_image2+=s_Effw) 
	{
		for (j=0,  k=0; j<s_w; j++, k+=3)        
		{
			r1 = in_image1[k+2];
			g1 = in_image1[k+1];
			b1 = in_image1[k  ];
			Dr = abs(in_image1[k+2] - in_image2[k+2]);
			Dg = abs(in_image1[k+1] - in_image2[k+1]);
			Db = abs(in_image1[k  ] - in_image2[k  ]); 
			if ( Dr <= 61 && Dg <= 49 && Db <=38
				&&
				r1 >= 114 && r1<=162 &&
				g1 >= 114 && g1<=160 &&
				b1 >= 103 && b1<=150 )
				out_image[k] = out_image[k+1] = out_image[k+2] = 0 ;
			else
				out_image[k] = out_image[k+1] = out_image[k+2] = 255 ;
			
		}
	}	
}

void CRogerFunction::I_ShadeReduce(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	int nSize = s_h * s_Effw;
	float r1, g1, b1, Dr, Dg, Db;
	BYTE *pBuffer3_1 = new BYTE[nSize];
	BYTE *pBuffer3_2 = new BYTE[nSize];
	BYTE *pBuffer3_3 = new BYTE[nSize];	

	memcpy(pBuffer3_1, in_image, nSize);
	for (i=0;i<s_h;i++, in_image+=s_Effw) 
	{
		for (j=0,  k=0; j<s_w ; j++, k+=3)        
		{
			pBuffer3_2[i*s_Effw+k]   = in_image[k]   + 38 ;
			pBuffer3_2[i*s_Effw+k+1] = in_image[k+1] + 49;
			pBuffer3_2[i*s_Effw+k+2] = in_image[k+2] + 61 ;
		}
	}


// 	//輸出顯示模組
	for (i=0; i<s_h; i++) 
	{
		for (j=0,  k=0; j<s_w; j++, k+=3)        
		{
			r1 = pBuffer3_2[i*s_Effw+k+2];
			g1 = pBuffer3_2[i*s_Effw+k+1];
			b1 = pBuffer3_2[i*s_Effw+k  ];
			Dr = abs(pBuffer3_2[i*s_Effw+k+2] - pBuffer3_1[i*s_Effw+k+2]);
			Dg = abs(pBuffer3_2[i*s_Effw+k+1] - pBuffer3_1[i*s_Effw+k+1]);
			Db = abs(pBuffer3_2[i*s_Effw+k  ] - pBuffer3_1[i*s_Effw+k  ]); 
			if ( Dr <= 61 && Dg <= 49 && Db <=38
				&&
				r1 >= 114 && r1<=162 &&
				g1 >= 114 && g1<=160 &&
				b1 >= 103 && b1<=150 )
				pBuffer3_3[i*s_Effw+k] = pBuffer3_3[i*s_Effw+k+1] = pBuffer3_3[i*s_Effw+k+2] = 0 ;
			else
				pBuffer3_3[i*s_Effw+k] = pBuffer3_3[i*s_Effw+k+1] = pBuffer3_3[i*s_Effw+k+2] = 255 ;
			
		}
	}
	memcpy(out_image, pBuffer3_3,nSize);
	for (i=0; i< nSize; i++)
	{
		if (pBuffer3_3[i] == 255)
			out_image[i] = pBuffer3_1[i];
		else
			out_image[i] = 255;
	}
	delete []pBuffer3_1;
	delete []pBuffer3_2;
	delete []pBuffer3_3;
}
void CRogerFunction::I_ShadeReduce01(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	int nSize = s_h * s_Effw;
	float r1, g1, b1;
	BYTE *pBuffer3_1 = new BYTE[nSize];
	BYTE *pBuffer3_2 = new BYTE[nSize];
	BYTE *pBuffer3_3 = new BYTE[nSize];	
	
	memcpy(pBuffer3_1, in_image, nSize);
	
	// 	//輸出顯示模組
	for (i=0; i<s_h; i++) 
	{
		for (j=0,  k=0; j<s_w; j++, k+=3)        
		{
			r1 = pBuffer3_1[i*s_Effw+k+2];
			g1 = pBuffer3_1[i*s_Effw+k+1];
			b1 = pBuffer3_1[i*s_Effw+k  ];

			if ( 
				   (r1 >= 64 && r1<= 157 
				&& g1 >= 69 && g1<= 156
				&& b1 >= 68 && b1<= 152)
				||
				(  r1 >= 153 && r1<= 255 
				&& g1 >= 153 && g1<= 255
				&& b1 >= 153 && b1<= 255)
				
				)
				pBuffer3_3[i*s_Effw+k] = pBuffer3_3[i*s_Effw+k+1] = pBuffer3_3[i*s_Effw+k+2] = 0 ;
			else
				pBuffer3_3[i*s_Effw+k] = pBuffer3_3[i*s_Effw+k+1] = pBuffer3_3[i*s_Effw+k+2] = 255 ;
			
		}
	}
	memcpy(out_image, pBuffer3_3,nSize);
	for (i=0; i< nSize; i++)
	{
		if (pBuffer3_3[i] == 255)
			out_image[i] = pBuffer3_1[i];
		else
			out_image[i] = 255;
	}
	delete []pBuffer3_1;
	delete []pBuffer3_2;
	delete []pBuffer3_3;
}
void CRogerFunction::I_BubbleSort(int *DataList, int Num)
{
	int i, j;
	int tmp;
	Num = Num - 1;
	for (i=0; i<Num; i++)
	{
		for (j=0; j<Num-i; j++)
		{
			if (DataList[j] < DataList[j+1]) // > 遞減, < 遞增
			{
				tmp = DataList[j];
				DataList[j] = DataList[j+1];
				DataList[j+1] = tmp;
			}
		}
	}
}
void CRogerFunction::I_BubbleSort(double *DataList, int Num)
{
	int i, j;
	double tmp;
	Num = Num - 1;
	for (i=0; i<Num; i++)
	{
		for (j=0; j<Num-i; j++)
		{
			if (DataList[j] < DataList[j+1]) // > 遞減, < 遞增
			{
				tmp = DataList[j];
				DataList[j] = DataList[j+1];
				DataList[j+1] = tmp;
			}
		}
	}
}
void CRogerFunction::I_ShadeReduce02(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{

}
double CRogerFunction::I_SVM_Classifier(  char *line_in , struct svm_model* model)
{
	// Class = 0 -> Head ... heart_scale.model

	struct svm_node *SVMx;
	int max_nr_attr = 64;
// 	struct svm_model* model;
	int predict_probability=0;
	static char *line = NULL;
	static int max_line_len;
	int correct = 0;
	int total = 0;
	double error = 0;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;
	int svm_type;
	int nr_class;
	double *prob_estimates=NULL;
	
	int SVMi = 0;
	double target_label, predict_label;
	char *idx, *val, *label, *endptr;
	int inst_max_index = -1; // strtol gives 0 if wrong format, and precomputed kernel has <index> start from 0
	int len;
// 	if ( Class == 0 ) //headmodel.txt
// 	{
// 		if((model=svm_load_model("head.model"))==0) exit(1); //開啟.model
// 	}

	
	SVMx = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	
	svm_type=svm_get_svm_type(model);
	nr_class=svm_get_nr_class(model);


///////////////////////////////
			int *labels=(int *) malloc(nr_class*sizeof(int));
			svm_get_labels(model,labels);
			prob_estimates = (double *) malloc(nr_class*sizeof(double));

			free(labels);
	
///////////////////////////

	max_line_len = 1024;
	line = (char *)malloc(max_line_len*sizeof(char));
	
	strcpy(line, line_in);
	
	while(strrchr(line,'\n') == NULL)
	{
		max_line_len *= 2;
		line = (char *) realloc(line,max_line_len);
		len = (int) strlen(line);
	}
	
	label = strtok(line," \t");
	target_label = strtod(label,&endptr);
	
	while(1)
	{
		if(SVMi>=max_nr_attr-1)	// need one more for index = -1
		{
			max_nr_attr *= 2;
			SVMx = (struct svm_node *) realloc(SVMx,max_nr_attr*sizeof(struct svm_node));
		}
		
		idx = strtok(NULL,":");
		val = strtok(NULL," \t");
		
		if(val == NULL)
			break;
		errno = 0;
		SVMx[SVMi].index = (int) strtol(idx,&endptr,10);
////////////////////////////////////////
			inst_max_index = SVMx[SVMi].index;
///////////////////////////////////////
		errno = 0;
		SVMx[SVMi].value = strtod(val,&endptr);

		++SVMi;
	}
	SVMx[SVMi].index = -1;
///////////////////////////////////////
		predict_label = svm_predict_probability(model,SVMx,prob_estimates);
		predict_label = svm_predict(model,SVMx);

///////////////////////////////////////

	//輸出到檔案...開始偵測判斷
// 	predict_label = svm_predict(model,SVMx);

	if(predict_probability)
		free(prob_estimates);
	
// 	svm_destroy_model(model);
	free(SVMx);
	free(line);
	return predict_label;
}
double CRogerFunction::I_SVM_Classifier(  double *dIvec, struct svm_model* model)
{
	struct svm_node *SVMx;
	int max_nr_attr = 64;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;
	int SVMi = 0;
	double predict_label;
	SVMx = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	for (SVMi=0; SVMi<64; SVMi++)
	{
		if(SVMi>=max_nr_attr-1)	// need one more for index = -1
		{
			max_nr_attr *= 2;
			SVMx = (struct svm_node *) realloc(SVMx,max_nr_attr*sizeof(struct svm_node));
		}
		SVMx[SVMi].index = SVMi+1;
		SVMx[SVMi].value = dIvec[SVMi];
	}
	SVMx[SVMi].index = -1;
	predict_label = svm_predict(model,SVMx);
	free(SVMx);
	return predict_label;
}
double CRogerFunction::I_SVM_Classifier(  double *dIvec, struct svm_model* model, int nLength )
{
	struct svm_node *SVMx;
	int max_nr_attr = nLength;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;
	int SVMi = 0;
	double predict_label;
	SVMx = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	for (SVMi=0; SVMi<nLength; SVMi++)
	{
		if(SVMi>=max_nr_attr-1)	// need one more for index = -1
		{
			max_nr_attr *= 2;
			SVMx = (struct svm_node *) realloc(SVMx,max_nr_attr*sizeof(struct svm_node));
		}
		SVMx[SVMi].index = SVMi+1;
		SVMx[SVMi].value = dIvec[SVMi];
	}
	SVMx[SVMi].index = -1;
	predict_label = svm_predict(model,SVMx);
	free(SVMx);
	return predict_label;
}
double CRogerFunction::I_SVM_Classifier(  double *dIvec, struct svm_model* model, int nLength, SVMINFO_TBL &SVM_INFO )
{
	struct svm_node *SVMx;
	int max_nr_attr = nLength ;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;
	int SVMi = 0;
	double predict_label;
	///////////////////////////
	int predict_probability=0;
	int svm_type;
	int nr_class;
	double *prob_estimates=NULL;
	///////////////////////////
	SVMx = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	svm_type=svm_get_svm_type(model);
	nr_class=svm_get_nr_class(model);
	int *labels=(int *) malloc(nr_class*sizeof(int));
	svm_get_labels(model,labels);
	prob_estimates = (double *) malloc(nr_class*sizeof(double));
	for (SVMi=0; SVMi<nLength ; SVMi++)
	{
		if(SVMi>=max_nr_attr-1)	// need one more for index = -1
		{
			max_nr_attr *= 2;
			SVMx = (struct svm_node *) realloc(SVMx,max_nr_attr*sizeof(struct svm_node));
		}
		SVMx[SVMi].index = SVMi+1;
		SVMx[SVMi].value = dIvec[SVMi];
	}
	SVMx[SVMi].index = -1;
	
	svm_predict_probability(model,SVMx,prob_estimates);
	for(int i=0; i<nr_class; i++)
	{
		SVM_INFO.SVMTBL[i].nIndex = i;
		SVM_INFO.SVMTBL[i].nClass = labels[i];
		SVM_INFO.SVMTBL[i].dProbability = prob_estimates[i];
	}
	predict_label = svm_predict(model,SVMx);

	free(labels);
	free(SVMx);
	return predict_label;
}
double CRogerFunction::GWSVM_Classifier(  double *GW, struct svm_model* model)
{
	struct svm_node *SVMx;
	int max_nr_attr = 360;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;
	int SVMi = 0;
	double predict_label;
	SVMx = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	for (SVMi=0; SVMi<360; SVMi++)
	{
		if(SVMi>=max_nr_attr-1)	// need one more for index = -1
		{
			max_nr_attr *= 2;
			SVMx = (struct svm_node *) realloc(SVMx,max_nr_attr*sizeof(struct svm_node));
		}
		SVMx[SVMi].index = SVMi+1;
		SVMx[SVMi].value = GW[SVMi];
	}
	SVMx[SVMi].index = -1;
	predict_label = svm_predict(model,SVMx);
	free(SVMx);
	return predict_label;
}
void CRogerFunction::I_HOG_Descriptor_Mag( BYTE *in_image, char *line_in) //灰階 16 x 16 Image
{
	//initial variable
	int i, j;
	double Histo[36];
	float maxval=0;
	int Indx=0;
	int nPixel= 256 ; // 16 x 16 Image
	int Str_len;
	char Tmp[2];
	char Dimension[36][10]; // 維度
// 	double Area1_Deg=0, Area2_Deg=0, Area3_Deg=0, Area4_Deg=0;
	double Area1_Mag=0, Area2_Mag=0, Area3_Mag=0, Area4_Mag=0;
	double Tot_Mag =0;

	CImgProcess FuncImg;
	CRogerFunction FuncMe;

	//...計算方向導數
	int x,y;
	int *pnGradX, *pnGradY;
	int *pnDegree;
	float *pnGradMAG;
	pnGradX		= new int[nPixel];
	pnGradY		= new int[nPixel];
	pnDegree   	= new int[nPixel];
	pnGradMAG	= new float[nPixel];

	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
		pnGradMAG[i]=0;
		pnDegree[i]=0;
	}
	//計算方向導數
	for(y=0; y<16; y++) //x方向導數//y方向導數
	{
		for(x=0; x<16; x++)
		{
			pnGradX[y*16+x] = (int) ( in_image[y*16+min(16-1,x+1)] - in_image[y*16+max(0,x-1)] );
			pnGradY[y*16+x] = (int) ( in_image[min(16-1,y+1)*16 + x] - in_image[max(0,y-1)*16+ x ] );
		}
	}
	float dis;
	float weit;
	float in_sigma = 0.5;
	for(i=0; i<16; i++)//梯度強度計算
	{
		for(j=0; j<16; j++)
		{
// 			pnGradMAG[i*16+j] = sqrt( pnGradX[i*16+j]*pnGradX[i*16+j]+pnGradY[i*16+j]*pnGradY[i*16+j] );
// 			Tot_Mag += pnGradMAG[i*16+j];

			//修正各點梯度強度 與中心點成 (1-ln2) 成比例, 離中心越遠, 比重越低
			dis = sqrt( pow( (i-7.5),2) + pow( (j-7.5),2) );
			
// 			weit = (4 - log2(dis)) / 4;
// 			weit = (4 - log10(dis)) / 4;
			weit = exp(-(0.5)*dis*dis/(in_sigma*in_sigma)) / (sqrt(2 * PI) * in_sigma ); //Gaussian
// 			weit = (pow(2,10.61)-pow(2,dis))/pow(2,10.61);
// 			weit = (10.7 -  dis) / 10.7 ; //(1-x)
// 			weit = (-5/9 *x) + (95/9);	//(ax+b)	
			pnGradMAG[i*16+j] = weit * sqrtf( pnGradX[i*16+j]*pnGradX[i*16+j]+pnGradY[i*16+j]*pnGradY[i*16+j] );
			Tot_Mag += pnGradMAG[i*16+j];
		}
	}

	//計算角度 0 - 180 度
	for(i=0; i<16; i++)
	{
		for(j=0; j<16; j++)
		{
			if(pnGradX[i*16+j]<0) // x -> -
			{
				pnDegree[i*16+j] = atanf((double)pnGradY[i*16+j]/(double)pnGradX[i*16+j]) * 57.3 + 180; // x-> -, y-> +
				if (pnGradY[i*16+j]<0)	pnDegree[i*16+j] = pnDegree[i*16+j] - 180; // x -> - , y -> -
			}
			else
			{  // x -> +
				pnDegree[i*16+j] = atanf((double)pnGradY[i*16+j]/(double)pnGradX[i*16+j]) * 57.3; // x -> +, y -> +
				if (pnDegree[i*16+j]<0) pnDegree[i*16+j] += 180; // x-> + , y -> -
			}
		}
	}

	for (i=0; i<36; i++) Histo[i] = 0;

	for(i=0; i<16; i++) //不同 角度 歸類 並將 梯度強度 投票
	{
		for (j=0; j<16; j++)
		{

			if (i<8 && j<8)
			{
				if ( pnDegree[i*16+j] <  10 || pnDegree[i*16+j] >= 170 ) Histo[0] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  30 && pnDegree[i*16+j] >=  10 ) Histo[1] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  50 && pnDegree[i*16+j] >=  30 ) Histo[2] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  70 && pnDegree[i*16+j] >=  50 ) Histo[3] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  90 && pnDegree[i*16+j] >=  70 ) Histo[4] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 110 && pnDegree[i*16+j] >=  90 ) Histo[5] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 130 && pnDegree[i*16+j] >= 110 ) Histo[6] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 150 && pnDegree[i*16+j] >= 130 ) Histo[7] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 170 && pnDegree[i*16+j] >= 150 ) Histo[8] += pnGradMAG[i*16+j] / Tot_Mag;
			}
			if (i>=8 && j<8)
			{
				if ( pnDegree[i*16+j] <  10 || pnDegree[i*16+j] >= 170 ) Histo[ 9] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  30 && pnDegree[i*16+j] >=  10 ) Histo[10] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  50 && pnDegree[i*16+j] >=  30 ) Histo[11] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  70 && pnDegree[i*16+j] >=  50 ) Histo[12] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  90 && pnDegree[i*16+j] >=  70 ) Histo[13] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 110 && pnDegree[i*16+j] >=  90 ) Histo[14] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 130 && pnDegree[i*16+j] >= 110 ) Histo[15] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 150 && pnDegree[i*16+j] >= 130 ) Histo[16] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 170 && pnDegree[i*16+j] >= 150 ) Histo[17] += pnGradMAG[i*16+j] / Tot_Mag;
			}
			if (i<8 && j>=8)
			{
				if ( pnDegree[i*16+j] <  10 || pnDegree[i*16+j] >= 170 ) Histo[18] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  30 && pnDegree[i*16+j] >=  10 ) Histo[19] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  50 && pnDegree[i*16+j] >=  30 ) Histo[20] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  70 && pnDegree[i*16+j] >=  50 ) Histo[21] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  90 && pnDegree[i*16+j] >=  70 ) Histo[22] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 110 && pnDegree[i*16+j] >=  90 ) Histo[23] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 130 && pnDegree[i*16+j] >= 110 ) Histo[24] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 150 && pnDegree[i*16+j] >= 130 ) Histo[25] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 170 && pnDegree[i*16+j] >= 150 ) Histo[26] += pnGradMAG[i*16+j] / Tot_Mag;
			}
			if (i>=8 && j>=8)
			{
				if ( pnDegree[i*16+j] <  10 || pnDegree[i*16+j] >= 170 ) Histo[27] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  30 && pnDegree[i*16+j] >=  10 ) Histo[28] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  50 && pnDegree[i*16+j] >=  30 ) Histo[29] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  70 && pnDegree[i*16+j] >=  50 ) Histo[30] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] <  90 && pnDegree[i*16+j] >=  70 ) Histo[31] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 110 && pnDegree[i*16+j] >=  90 ) Histo[32] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 130 && pnDegree[i*16+j] >= 110 ) Histo[33] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 150 && pnDegree[i*16+j] >= 130 ) Histo[34] += pnGradMAG[i*16+j] / Tot_Mag;
				if ( pnDegree[i*16+j] < 170 && pnDegree[i*16+j] >= 150 ) Histo[35] += pnGradMAG[i*16+j] / Tot_Mag;
			}
		}
	}
//...測試 normalization 是否為 1
// 	double test = 0;
// 	for (i=0; i<36 ; i++)
// 	{
// 		test += Histo[i];
// 	}

// 	FuncMe.BubbleSort(Histo,8);
//找出最大值調整影像方向 (沒有旋轉問題即不用)
// 	for (i=0; i<9; i++)
// 	{
// 		if (Histo[i]>maxval)
// 		{
// 			maxval = Histo[i];
// 			Indx   = i;
// 		}
// 	}
// 	// Shifting
// 	if (Indx==0)
// 	{
// 		for (i=0; i<36; i++)
// 		{
// 			f1[i] = Histo[i];
// 		}
// 	}
// 	else
// 	{
// 		for (i=Indx; i<9; i++)
// 		{
// 			f1[i-Indx] = Histo[i];
// 			f1[i-Indx+9] = Histo[i+9];
// 			f1[i-Indx+18] = Histo[i+18];
// 			f1[i-Indx+27] = Histo[i+27];
// 		}
// 		for (i=0; i<Indx; i++)
// 		{
// 			f1[9-Indx+i] = Histo[i];
// 			f1[9-Indx+i+9] = Histo[i+9];
// 			f1[9-Indx+i+18] = Histo[i+18];
// 			f1[9-Indx+i+27] = Histo[i+27];
// 		}
// 	}

	for (i=0;i<36;i++)
		for (j=0;j<10;j++)
			Dimension[i][j] = '\0';

	//....維度資料轉換成SVM 格式 "1:-0.375 2:1.0000 3:0.333333 4:-0.132075 5:-0.502283 6:-1 7:1 8:0.664122 9:-1 10:-1 11:-1 12:-1 13:-1\n";  //-1
	for (i=0; i<36; i++) //維度資料轉成字串 即將 傳入 SVM line 中
		sprintf(Dimension[i],"%.3f ",Histo[i]);

	for (i=1; i<=36; i++)
	{
		strcat(line_in, strcat(itoa(i,Tmp,10),":"));
		strcat(line_in, Dimension[i-1]);
	}
	Str_len = strlen(line_in);
	line_in[Str_len-1] = '\n';
	//....維度資料轉換成SVM 格式 END

	delete []pnGradX;
	delete []pnGradY;
	delete []pnGradMAG;
	delete []pnDegree;	
}
void CRogerFunction::I_ROI(BYTE *in_image, int s_w, int s_h, int s_BPP, CPoint UL, BYTE *out_image, int d_w, int d_h)
{
	int i, j, k;
	for (i=0; i<d_h; i++)
	{
		for (j=0, k=0; j<d_w; j++, k+=3)
		{
			out_image[i*d_w*3+k]   = in_image[(i+(s_h - UL.y - d_h ) )*s_w*3+(k+UL.x*3)];
			out_image[i*d_w*3+k+1] = in_image[(i+(s_h - UL.y - d_h ) )*s_w*3+(k+1+UL.x*3)];
			out_image[i*d_w*3+k+2] = in_image[(i+(s_h - UL.y - d_h ) )*s_w*3+(k+2+UL.x*3)];
		}
	}
}
void CRogerFunction::I_ROIRAW(BYTE *in_image, int s_w, int s_h, CPoint UL, BYTE *out_image, int d_w, int d_h)
{
	int i, j;
	
	for (i=0; i<d_h; i++)
	{
		for (j=0; j<d_w; j++)
		{
			out_image[i*d_w+j]   = in_image[(i+(s_h - UL.y - d_h ) )*s_w+(j+UL.x)];
		}
	}
}
void CRogerFunction::I_quicksort(int *data, int first, int last, int size)
{
    int pivot;
    if(first < last)
    {
        int i = first;
        int j = last + 1;
        pivot = data[first];
        do
        {
            do i++;while(data[i] > pivot);      //從list的左邊找比pivot大的 
            do j--;while(data[j] < pivot);      //從list的右邊找筆pivot小的 
            if(i < j) I_swap(&data[i], &data[j]); //將上面找到的結果做交換 
        }while(i<j);
        
        I_swap(&data[first],&data[j]);
        
        I_quicksort(data, first, j-1, size);
        I_quicksort(data, j+1, last, size);
    }    
}
void CRogerFunction::I_swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}
void CRogerFunction::I_FindHCP_MaskNew( BYTE *in_image, BYTE *out_image, int s_Width, int s_Height) //輸入CANNY後之影像 nPixel大小
{
	int i, j, x, y;
	int ii, jj;
	int s_Pixel = s_Width*s_Height;
	float Rad=0;
	int Dis = 10;
	int Hist[36];
	int Tbl[36];
	int n1st, n2nd, Th0, Th1;

	for (y=10; y<s_Height-10; y++)
	{
		for (x=10; x<s_Width-10; x++)
		{
			if ( in_image[y*s_Width+x] == 255 )
			{
				for (i=0; i<36; i++) Hist[i] = 0;
				for (i=0; i<360; i+=2)
				{
					for (j=5; j<Dis; j++)
					{
						Rad = i*0.01745;
						ii = int(y+j*sin(Rad)); //y
						jj = int(x+j*cos(Rad)); //x
						if (in_image[ii*s_Width+jj]==255)
						{
							if      ( i>=0   && i<10  )	Hist[0]++;
							else if ( i>=10  && i<20  )	Hist[1]++;
							else if ( i>=20  && i<30  )	Hist[2]++;
							else if ( i>=30  && i<40  )	Hist[3]++;
							else if ( i>=40  && i<50  )	Hist[4]++;
							else if ( i>=50  && i<60  ) Hist[5]++;
							else if ( i>=60  && i<70  ) Hist[6]++;
							else if ( i>=70  && i<80  ) Hist[7]++;
							else if ( i>=80  && i<90  ) Hist[8]++;
							else if ( i>=90  && i<100 ) Hist[9]++;
							else if ( i>=100 && i<110 ) Hist[10]++;
							else if ( i>=110 && i<120 ) Hist[11]++;
							else if ( i>=120 && i<130 ) Hist[12]++;
							else if ( i>=130 && i<140 ) Hist[13]++;
							else if ( i>=140 && i<150 ) Hist[14]++;
							else if ( i>=150 && i<160 ) Hist[15]++;
							else if ( i>=160 && i<170 ) Hist[16]++;
							else if ( i>=170 && i<180 ) Hist[17]++;
							else if ( i>=180 && i<190 ) Hist[18]++;
							else if ( i>=190 && i<200 ) Hist[19]++;
							else if ( i>=200 && i<210 ) Hist[20]++;
							else if ( i>=210 && i<220 ) Hist[21]++;
							else if ( i>=220 && i<230 ) Hist[22]++;
							else if ( i>=230 && i<240 ) Hist[23]++;
							else if ( i>=240 && i<250 ) Hist[24]++;
							else if ( i>=250 && i<260 ) Hist[25]++;
							else if ( i>=260 && i<270 ) Hist[26]++;
							else if ( i>=270 && i<280 ) Hist[27]++;
							else if ( i>=280 && i<290 ) Hist[28]++;
							else if ( i>=290 && i<300 ) Hist[29]++;
							else if ( i>=300 && i<310 ) Hist[30]++;
							else if ( i>=310 && i<320 ) Hist[31]++;
							else if ( i>=320 && i<330 ) Hist[32]++;
							else if ( i>=330 && i<340 ) Hist[33]++;
							else if ( i>=340 && i<350 ) Hist[34]++;
							else if ( i>=350 && i<360 ) Hist[35]++;
						}
					}
				}
			}
			for (i=0; i<36; i++) Tbl[i] = Hist[i];
			I_quicksort(Hist,0,35,36);
			Th0 = Hist[0];
			Th1 = Hist[1];
			for (i=0; i< 36; i++)
			{
				if( Tbl[i] == Hist[0]) 
				{ 
					n1st = i; 
					Tbl[i] = 0;
					i = 36;
				}
			}
			for (i=0; i< 36; i++)
			{
				if( Tbl[i] == Hist[1]) 
				{ 
					n2nd = i; 
					Tbl[i] = 0;
					i = 36;
				}
			}

			if ((  (abs((n1st - n2nd)*10) <= 120 && abs((n1st - n2nd)*10) >= 30) 
				|| (abs((n1st - n2nd)*10) >= 240 && abs((n1st - n2nd)*10) <= 330))
				&& Th0 > 10 && Th1 > 10 )
			{
				out_image[(y+1)*s_Width+(x)] = 200;
				out_image[(y-1)*s_Width+(x)] = 200;
				out_image[(y)*s_Width+(x)] = 200;
				out_image[(y)*s_Width+(x+1)] = 200;
				out_image[(y)*s_Width+(x-1)] = 200;
// 				gpMsgbar->ShowMessage("-->(%d, %d): 1st: %d (%d) 2nd: %d (%d) Degree: %d\n", x, s_Height-y-1, n1st, Hist[0], n2nd, Hist[1], abs(n1st - n2nd)*10);
				
			}
		}
	}
}
void CRogerFunction::I_FindHCP_MaskNew( BYTE *in_image, CPoint &CenterPoint, int Radius, int *Hist, int s_w, int s_h)
{
	int i, j, x, y, c;
	int ii, jj;
	int s_Pixel = s_w*s_h;
	float Rad=0;
	int Dis = Radius;
	int Tot = 0;
	y = CenterPoint.y;
	x = CenterPoint.x;
// 	for (i=0; i<36; i++) Hist[i] = 0;
	for (i=0; i<360; i+=2)
	{
		for (j=5; j<Dis+5; j++)
		{
			Rad = i*0.01745;
			ii = int(y+j*sin(Rad)); //y
			jj = int(x+j*cos(Rad)); //x
			if( ii >=0 && ii < s_h && jj >=0 && jj < s_w)
			if (in_image[ii*s_w+jj]==255 )
			{
				if      ( i>=0   && i<10  )	Hist[0]++;
				else if ( i>=10  && i<20  )	Hist[1]++;
				else if ( i>=20  && i<30  )	Hist[2]++;
				else if ( i>=30  && i<40  )	Hist[3]++;
				else if ( i>=40  && i<50  )	Hist[4]++;
				else if ( i>=50  && i<60  ) Hist[5]++;
				else if ( i>=60  && i<70  ) Hist[6]++;
				else if ( i>=70  && i<80  ) Hist[7]++;
				else if ( i>=80  && i<90  ) Hist[8]++;
				else if ( i>=90  && i<100 ) Hist[9]++;
				else if ( i>=100 && i<110 ) Hist[10]++;
				else if ( i>=110 && i<120 ) Hist[11]++;
				else if ( i>=120 && i<130 ) Hist[12]++;
				else if ( i>=130 && i<140 ) Hist[13]++;
				else if ( i>=140 && i<150 ) Hist[14]++;
				else if ( i>=150 && i<160 ) Hist[15]++;
				else if ( i>=160 && i<170 ) Hist[16]++;
				else if ( i>=170 && i<180 ) Hist[17]++;
				else if ( i>=180 && i<190 ) Hist[18]++;
				else if ( i>=190 && i<200 ) Hist[19]++;
				else if ( i>=200 && i<210 ) Hist[20]++;
				else if ( i>=210 && i<220 ) Hist[21]++;
				else if ( i>=220 && i<230 ) Hist[22]++;
				else if ( i>=230 && i<240 ) Hist[23]++;
				else if ( i>=240 && i<250 ) Hist[24]++;
				else if ( i>=250 && i<260 ) Hist[25]++;
				else if ( i>=260 && i<270 ) Hist[26]++;
				else if ( i>=270 && i<280 ) Hist[27]++;
				else if ( i>=280 && i<290 ) Hist[28]++;
				else if ( i>=290 && i<300 ) Hist[29]++;
				else if ( i>=300 && i<310 ) Hist[30]++;
				else if ( i>=310 && i<320 ) Hist[31]++;
				else if ( i>=320 && i<330 ) Hist[32]++;
				else if ( i>=330 && i<340 ) Hist[33]++;
				else if ( i>=340 && i<350 ) Hist[34]++;
				else if ( i>=350 && i<360 ) Hist[35]++;
				Tot++;

			}
		}
	}
	for (i=0; i<36; i++) Hist[i] = Hist[i] * 255 / (Tot+0.00001);
	//校正方向
	int Tbl[36];
	int Max_Value;
	int MaxValueIdx;

	for (c=0; c<36; c++) Tbl[c] =Hist[c];
	I_quicksort( Tbl, 0, 35, 36);
	Max_Value = Tbl[0];
	//****找出最大值Index Value ****
	for (c=0; c<36; c++)
	{
		if( Hist[c] == Max_Value)
		{
			MaxValueIdx = c;
			c = 36;
		}
	}
	//****將角度予以正規化
	memset( Tbl, 0, sizeof(int)*36);
	for (c=MaxValueIdx; c<36; c++)
	{
		Tbl[c-MaxValueIdx] = Hist[c];
	}
	for (c=0; c<MaxValueIdx; c++)
	{
		Tbl[36-MaxValueIdx+c] = Hist[c];
	}
	for (c=0; c<36; c++) Hist[c] = Tbl[c];

}
void CRogerFunction::I_FindHCP_MaskNewTest( BYTE *in_image, CPoint &CenterPoint, int Radius, int *Hist, int s_w, int s_h)
{
	int i, j, x, y, c;
	int ii, jj;
	int s_Pixel = s_w*s_h;
	float Rad=0;
	int Dis = Radius;
	int Tot = 0;
	y = CenterPoint.y;
	x = CenterPoint.x;
	BYTE *MAP = new BYTE[s_Pixel];
	memset(MAP,0,sizeof(BYTE)*s_Pixel);

// 	for (i=0; i<36; i++) Hist[i] = 0;
	for (i=0; i<360; i+=2)
	{
		for (j=5; j<Dis+5; j++)
		{
			Rad = i*0.01745;
			ii = int(y+j*sin(Rad)); //y
			jj = int(x+j*cos(Rad)); //x
			if( ii >=0 && ii < s_h && jj >=0 && jj < s_w)
			if (in_image[ii*s_w+jj]==255 && MAP[ii*s_w+jj]==0 )
			{
				if      ( i>=0   && i<10  )	Hist[0]++;
				else if ( i>=10  && i<20  )	Hist[1]++;
				else if ( i>=20  && i<30  )	Hist[2]++;
				else if ( i>=30  && i<40  )	Hist[3]++;
				else if ( i>=40  && i<50  )	Hist[4]++;
				else if ( i>=50  && i<60  ) Hist[5]++;
				else if ( i>=60  && i<70  ) Hist[6]++;
				else if ( i>=70  && i<80  ) Hist[7]++;
				else if ( i>=80  && i<90  ) Hist[8]++;
				else if ( i>=90  && i<100 ) Hist[9]++;
				else if ( i>=100 && i<110 ) Hist[10]++;
				else if ( i>=110 && i<120 ) Hist[11]++;
				else if ( i>=120 && i<130 ) Hist[12]++;
				else if ( i>=130 && i<140 ) Hist[13]++;
				else if ( i>=140 && i<150 ) Hist[14]++;
				else if ( i>=150 && i<160 ) Hist[15]++;
				else if ( i>=160 && i<170 ) Hist[16]++;
				else if ( i>=170 && i<180 ) Hist[17]++;
				else if ( i>=180 && i<190 ) Hist[18]++;
				else if ( i>=190 && i<200 ) Hist[19]++;
				else if ( i>=200 && i<210 ) Hist[20]++;
				else if ( i>=210 && i<220 ) Hist[21]++;
				else if ( i>=220 && i<230 ) Hist[22]++;
				else if ( i>=230 && i<240 ) Hist[23]++;
				else if ( i>=240 && i<250 ) Hist[24]++;
				else if ( i>=250 && i<260 ) Hist[25]++;
				else if ( i>=260 && i<270 ) Hist[26]++;
				else if ( i>=270 && i<280 ) Hist[27]++;
				else if ( i>=280 && i<290 ) Hist[28]++;
				else if ( i>=290 && i<300 ) Hist[29]++;
				else if ( i>=300 && i<310 ) Hist[30]++;
				else if ( i>=310 && i<320 ) Hist[31]++;
				else if ( i>=320 && i<330 ) Hist[32]++;
				else if ( i>=330 && i<340 ) Hist[33]++;
				else if ( i>=340 && i<350 ) Hist[34]++;
				else if ( i>=350 && i<360 ) Hist[35]++;
				MAP[ii*s_w+jj] = 255;

				Tot++;

			}
		}
	}
	for (i=0; i<36; i++) Hist[i] = Hist[i] * 255 / (Tot+0.00001);
	//校正方向
	int Tbl[36];
	int Max_Value;
	int MaxValueIdx;

	for (c=0; c<36; c++) Tbl[c] =Hist[c];
	I_quicksort( Tbl, 0, 35, 36);
	Max_Value = Tbl[0];
	//****找出最大值Index Value ****
	for (c=0; c<36; c++)
	{
		if( Hist[c] == Max_Value)
		{
			MaxValueIdx = c;
			c = 36;
		}
	}
	//****將角度予以正規化
	memset( Tbl, 0, sizeof(int)*36);
	for (c=MaxValueIdx; c<36; c++)
	{
		Tbl[c-MaxValueIdx] = Hist[c];
	}
	for (c=0; c<MaxValueIdx; c++)
	{
		Tbl[36-MaxValueIdx+c] = Hist[c];
	}
	for (c=0; c<36; c++) Hist[c] = Tbl[c];
	delete []MAP;
}
void CRogerFunction::I_CANNY_Direct(BYTE *ImgSrc, BYTE *out_image, int *outDirect_image, int s_w, int s_h, double in_sigma, double in_Th_ratio, double in_Tl_ratio)
{
	int i, j;
	int nWidth	= s_w;
	int nHeight = s_h;
	int k;
	int Theta;

	BYTE *ImgRlt = new BYTE[nWidth*nHeight];
	int  *DirMap = new  int[nWidth*nHeight];

//.....canny...

	double sigma = in_sigma;
	double dRatioHigh=in_Th_ratio;
	double dRatioLow=in_Tl_ratio;

	int  *pnGradX;
	int  *pnGradY;
	int  *pnGradMAG;

	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double dWeightSum;
	double *pdTmp;
	int nCenter;
	double  dDis; 

	double  dValue; 
	double  dSum  ;
	double dSqtOne;
	double dSqtTwo;
	int nPos;
	float gx  ;
	float gy  ;
	int g1, g2, g3, g4 ;
	double weight  ;
	double dTmp1   ;
	double dTmp2   ;
	double dTmp    ;
	int nThdHigh ;
	int nThdLow  ;
	int nHist[1024] ;
	int nEdgeNb     ;
	int nMaxMag     ;
	int nHighCount  ;

	CRogerFunction FuncMe;
	
	for (i=0; i<nWidth*nHeight; i++) 
	{
		DirMap[i] = 0;
	}

//..MakeGauss Smooth
	dSum = 0 ; 
	nWindowSize = 1 + 2 * ceil(3 * sigma);
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(sigma*sigma)) / (sqrt(2 * PI) * sigma );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		(pdKernel)[i] /= dSum;
	}
//..End MakeGauss........	

	nHalfLen = nWindowSize / 2;

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)ImgSrc[y*nWidth + (i+x)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul/dWeightSum ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			ImgRlt[y*nWidth + x] = (unsigned char)(int)dDotMul/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
	for (i=0; i<nWidth*nHeight; i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...計算方向導數
	pnGradX		= new int[nWidth*nHeight];
	pnGradY		= new int[nWidth*nHeight];
	pnGradMAG	= new int[nWidth*nHeight];

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			pnGradX[y*nWidth+x] = (int) (ImgSrc[y*nWidth+min(nWidth-1,x+1)]  
				- ImgSrc[y*nWidth+max(0,x-1)]     );
		}
	}
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			pnGradY[y*nWidth+x] = (int) ( ImgSrc[min(nHeight-1,y+1)*nWidth + x]  
				- ImgSrc[max(0,y-1)*nWidth+ x ]     );
		}
	}
//...End 計算方向導數

//...計算梯度
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dSqtOne = pnGradX[y*nWidth + x] * pnGradX[y*nWidth + x];
			dSqtTwo = pnGradY[y*nWidth + x] * pnGradY[y*nWidth + x];
			pnGradMAG[y*nWidth + x] = (int)(sqrt(dSqtOne + dSqtTwo) + 0.5);
//................
			if (pnGradMAG[y*nWidth+x] != 0)
			{
				if (pnGradX[y*nWidth + x] ==0 && pnGradY[y*nWidth + x] !=0)  // y 方向
				{
					DirMap[y*nWidth + x]   = 90;
				}
				else if (pnGradX[y*nWidth + x] !=0 && pnGradY[y*nWidth + x] ==0) // x 方向
				{
					DirMap[y*nWidth + x]   = 1;
				}
				else 
				{
					Theta = atanf((double)pnGradY[y*nWidth + x]/(double)pnGradX[y*nWidth + x]) * 57.3;
					if ( Theta == 0) Theta = 1;
					DirMap[y*nWidth + x]   = Theta;    
				}
			}
// 			gpMsgbar->ShowMessage("-->(%d, %d): Degree: %d\n", x, y, DirMap[y*nWidth + x]);
//................
		}
	}
//...End計算梯度
//...Non-Maximum

	for(x=0; x<nWidth; x++)		
	{
		ImgRlt[x] = 0 ;
		ImgRlt[nHeight-1+x] = 0;
	}
	for(y=0; y<nHeight; y++)		
	{
		ImgRlt[y*nWidth] = 0 ;
		ImgRlt[y*nWidth + nWidth-1] = 0;
	}

	for(y=1; y<nHeight-1; y++)
	{
		for(x=1; x<nWidth-1; x++)
		{
			nPos = y*nWidth + x ;
			if(pnGradMAG[nPos] == 0 )
			{
				ImgRlt[nPos] = 0 ;
			}
			else
			{
				dTmp = pnGradMAG[nPos] ;
				
				gx = pnGradX[nPos]  ;
				gy = pnGradY[nPos]  ;

				if (abs(gy) < abs(gx)) 
				{
					weight = fabs(gx)/fabs(gy); 

					g2 = pnGradMAG[nPos-nWidth] ; 
					g4 = pnGradMAG[nPos+nWidth] ;

					if (gx*gy > 0) 
					{ 					
						g1 = pnGradMAG[nPos-nWidth-1] ;
						g3 = pnGradMAG[nPos+nWidth+1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					} 
				}
				else
				{
					weight = fabs(gy)/fabs(gx); 
					
					g2 = pnGradMAG[nPos+1] ; 
					g4 = pnGradMAG[nPos-1] ;
					if (gx*gy > 0) 
					{				
						g1 = pnGradMAG[nPos+nWidth+1] ;
						g3 = pnGradMAG[nPos-nWidth-1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					}
				}
					dTmp1 = weight*g1 + (1-weight)*g2 ;
					dTmp2 = weight*g3 + (1-weight)*g4 ;
					if(dTmp> dTmp1 && dTmp> dTmp2)
					{
						ImgRlt[nPos] = 128 ;
					}
					else
					{
						ImgRlt[nPos] = 0 ;
					}
			} //else
		} // for
	}
	for (i=0; i<nWidth*nHeight;i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...END Non-Maximum
//...Hysteresis

	
// 	EstimateThreshold
	nMaxMag = 0     ;

	for(k=0; k<1024; k++) 
	{
		nHist[k] = 0; 
	}
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			if(ImgSrc[y*nWidth+x]==128)
			{
				nHist[ pnGradMAG[y*nWidth+x] ]++;
			}
		}
	}
	
	nEdgeNb = nHist[0]  ;
	nMaxMag = 0         ;
	for(k=1; k<1024; k++)
	{
		if(nHist[k] != 0)
		{
			nMaxMag = k;
		}
		nEdgeNb += nHist[k];
	}
	nHighCount = (int)(dRatioHigh * nEdgeNb +0.5);
	
	k = 1;
	nEdgeNb = nHist[1];
	
	while( (k<(nMaxMag-1)) && (nEdgeNb < nHighCount) )
	{
		k++;
		nEdgeNb += nHist[k];
	}
	nThdHigh = k ;
	
	nThdLow  = (int)((nThdHigh) * dRatioLow+ 0.5);	
// 	End EstimateThreshold


	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			
			if((ImgSrc[nPos] == 128) && (pnGradMAG[nPos] >= nThdHigh))
			{
				ImgSrc[nPos] = 255;
				FuncMe.I_TraceEdge(y, x, nThdLow, ImgSrc, pnGradMAG, nWidth);
			}
		}
	}
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			if(ImgSrc[nPos] != 255)
			{
				ImgSrc[nPos] = 0 ;
			}
			else
				outDirect_image[nPos] = DirMap[nPos];
		}
	 }
//...END Hysteresis
	// gray level process 灰階輸出
	for (i=0;i<nHeight;i++) //i控制列(高)
	{
		for (j=0; j<nWidth ; j++)        //k為陣列指標
		{
			out_image[i*nWidth+j]  =ImgSrc[i*nWidth+j];
// 			gpMsgbar->ShowMessage("-->(%d, %d): Degree: %d\n", j, i, DirMap[i*nWidth + j]);
		}
	}

delete []ImgSrc;
ImgSrc = NULL;
delete []ImgRlt;
ImgRlt = NULL;
delete []pnGradMAG;
pnGradMAG = NULL;
delete []pnGradX;
pnGradX = NULL;
delete []pnGradY;
pnGradY = NULL;
delete []DirMap;
DirMap = NULL;
}

void CRogerFunction::I_CANNY_Direct_360(BYTE *ImgSrc, BYTE *out_image, int *outDirect_image, int s_w, int s_h, double in_sigma, double in_Th_ratio, double in_Tl_ratio)
{
	int i, j;
	int nWidth	= s_w;
	int nHeight = s_h;
	int k;
	int Theta;

	BYTE *ImgRlt = new BYTE[nWidth*nHeight];
	int  *DirMap = new  int[nWidth*nHeight];

//.....canny...

	double sigma = in_sigma;
	double dRatioHigh=in_Th_ratio;
	double dRatioLow=in_Tl_ratio;

	int  *pnGradX;
	int  *pnGradY;
	int  *pnGradMAG;

	int x,y, nWindowSize, nHalfLen;
	double *pdKernel;
	double dDotMul;
	double dWeightSum;
	double *pdTmp;
	int nCenter;
	double  dDis; 

	double  dValue; 
	double  dSum  ;
	double dSqtOne;
	double dSqtTwo;
	int nPos;
	float gx  ;
	float gy  ;
	int g1, g2, g3, g4 ;
	double weight  ;
	double dTmp1   ;
	double dTmp2   ;
	double dTmp    ;
	int nThdHigh ;
	int nThdLow  ;
	int nHist[1024] ;
	int nEdgeNb     ;
	int nMaxMag     ;
	int nHighCount  ;

	CRogerFunction FuncMe;
	
	for (i=0; i<nWidth*nHeight; i++) 
	{
		DirMap[i] = 400;
	}

//..MakeGauss Smooth
	dSum = 0 ; 
	nWindowSize = 1 + 2 * ceil(3 * sigma);
	nCenter = (nWindowSize) / 2;
	pdKernel	= new double[nWindowSize] ;
	pdTmp		= new double[nWidth*nHeight];
	for(i=0; i< (nWindowSize); i++)
	{
		dDis = (double)(i - nCenter);
		dValue = exp(-(0.5)*dDis*dDis/(sigma*sigma)) / (sqrt(2 * PI) * sigma );
		(pdKernel)[i] = dValue ;
		dSum += dValue;
	}
	for(i=0; i<(nWindowSize) ; i++)
	{
		(pdKernel)[i] /= dSum;
	}
//..End MakeGauss........	

	nHalfLen = nWindowSize / 2;

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+x) >= 0  && (i+x) < nWidth )
				{
					dDotMul += (double)ImgSrc[y*nWidth + (i+x)] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			pdTmp[y*nWidth + x] = dDotMul/dWeightSum ;
		}
	}
	
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			dDotMul		= 0;
			dWeightSum = 0;
			for(i=(-nHalfLen); i<=nHalfLen; i++)
			{
				if( (i+y) >= 0  && (i+y) < nHeight )
				{
					dDotMul += (double)pdTmp[(y+i)*nWidth + x] * pdKernel[nHalfLen+i];
					dWeightSum += pdKernel[nHalfLen+i];
				}
			}
			ImgRlt[y*nWidth + x] = (unsigned char)(int)dDotMul/dWeightSum ;
		}
	}
	delete []pdKernel;
	pdKernel = NULL;
	delete []pdTmp;
	pdTmp = NULL;
	for (i=0; i<nWidth*nHeight; i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...計算方向導數
	pnGradX		= new int[nWidth*nHeight];
	pnGradY		= new int[nWidth*nHeight];
	pnGradMAG	= new int[nWidth*nHeight];

	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			pnGradX[y*nWidth+x] = (int) (ImgSrc[y*nWidth+min(nWidth-1,x+1)]  
				- ImgSrc[y*nWidth+max(0,x-1)]     );
		}
	}
	for(x=0; x<nWidth; x++)
	{
		for(y=0; y<nHeight; y++)
		{
			pnGradY[y*nWidth+x] = (int) ( ImgSrc[min(nHeight-1,y+1)*nWidth + x]  
				- ImgSrc[max(0,y-1)*nWidth+ x ]     );
		}
	}
//...End 計算方向導數

//...計算梯度
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			dSqtOne = pnGradX[y*nWidth + x] * pnGradX[y*nWidth + x];
			dSqtTwo = pnGradY[y*nWidth + x] * pnGradY[y*nWidth + x];
			pnGradMAG[y*nWidth + x] = (int)(sqrt(dSqtOne + dSqtTwo) + 0.5);
//................
			if (pnGradMAG[y*nWidth+x] != 0)
			{
				if (pnGradX[y*nWidth + x] ==0 && pnGradY[y*nWidth + x] !=0)  // y 方向
				{
					DirMap[y*nWidth + x]   = 90;
				}
				else if (pnGradX[y*nWidth + x] !=0 && pnGradY[y*nWidth + x] ==0) // x 方向
				{
					DirMap[y*nWidth + x]   = 0;
				}
				else if (pnGradX[y*nWidth + x] > 0 && pnGradY[y*nWidth + x] > 0) // 第一象限
				{
					Theta = atanf((double)pnGradY[y*nWidth + x]/(double)pnGradX[y*nWidth + x]) * 57.3;
					DirMap[y*nWidth + x]   = Theta; 
				}
				else if (pnGradX[y*nWidth + x] < 0 && pnGradY[y*nWidth + x] > 0) // 第二象限
				{
					Theta = atanf((double)pnGradY[y*nWidth + x]/(double)pnGradX[y*nWidth + x]) * 57.3;
					Theta = 180 + Theta;
					DirMap[y*nWidth + x]   = Theta; 
				}
				else if (pnGradX[y*nWidth + x] < 0 && pnGradY[y*nWidth + x] < 0) // 第三象限
				{
					Theta = atanf((double)pnGradY[y*nWidth + x]/(double)pnGradX[y*nWidth + x]) * 57.3;
					Theta = -180 + Theta;
					DirMap[y*nWidth + x]   = Theta; 
				}
				else // 第四象限
				{
					Theta = atanf((double)pnGradY[y*nWidth + x]/(double)pnGradX[y*nWidth + x]) * 57.3;
					DirMap[y*nWidth + x]   = Theta;    
				}
			}
// 			gpMsgbar->ShowMessage("-->(%d, %d): Degree: %d\n", x, y, DirMap[y*nWidth + x]);
//................
		}
	}
//...End計算梯度
//...Non-Maximum

	for(x=0; x<nWidth; x++)		
	{
		ImgRlt[x] = 0 ;
		ImgRlt[nHeight-1+x] = 0;
	}
	for(y=0; y<nHeight; y++)		
	{
		ImgRlt[y*nWidth] = 0 ;
		ImgRlt[y*nWidth + nWidth-1] = 0;
	}

	for(y=1; y<nHeight-1; y++)
	{
		for(x=1; x<nWidth-1; x++)
		{
			nPos = y*nWidth + x ;
			if(pnGradMAG[nPos] == 0 )
			{
				ImgRlt[nPos] = 0 ;
			}
			else
			{
				dTmp = pnGradMAG[nPos] ;
				
				gx = pnGradX[nPos]  ;
				gy = pnGradY[nPos]  ;

				if (abs(gy) < abs(gx)) 
				{
					weight = fabs(gx)/fabs(gy); 

					g2 = pnGradMAG[nPos-nWidth] ; 
					g4 = pnGradMAG[nPos+nWidth] ;

					if (gx*gy > 0) 
					{ 					
						g1 = pnGradMAG[nPos-nWidth-1] ;
						g3 = pnGradMAG[nPos+nWidth+1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					} 
				}
				else
				{
					weight = fabs(gy)/fabs(gx); 
					
					g2 = pnGradMAG[nPos+1] ; 
					g4 = pnGradMAG[nPos-1] ;
					if (gx*gy > 0) 
					{				
						g1 = pnGradMAG[nPos+nWidth+1] ;
						g3 = pnGradMAG[nPos-nWidth-1] ;
					} 
					else 
					{ 
						g1 = pnGradMAG[nPos-nWidth+1] ;
						g3 = pnGradMAG[nPos+nWidth-1] ;
					}
				}
					dTmp1 = weight*g1 + (1-weight)*g2 ;
					dTmp2 = weight*g3 + (1-weight)*g4 ;
					if(dTmp> dTmp1 && dTmp> dTmp2)
					{
						ImgRlt[nPos] = 128 ;
					}
					else
					{
						ImgRlt[nPos] = 0 ;
					}
			} //else
		} // for
	}
	for (i=0; i<nWidth*nHeight;i++)
	{
		ImgSrc[i] = ImgRlt[i];
	}
//...END Non-Maximum
//...Hysteresis

	
// 	EstimateThreshold
	nMaxMag = 0     ;

	for(k=0; k<1024; k++) 
	{
		nHist[k] = 0; 
	}
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			if(ImgSrc[y*nWidth+x]==128)
			{
				nHist[ pnGradMAG[y*nWidth+x] ]++;
			}
		}
	}
	
	nEdgeNb = nHist[0]  ;
	nMaxMag = 0         ;
	for(k=1; k<1024; k++)
	{
		if(nHist[k] != 0)
		{
			nMaxMag = k;
		}
		nEdgeNb += nHist[k];
	}
	nHighCount = (int)(dRatioHigh * nEdgeNb +0.5);
	
	k = 1;
	nEdgeNb = nHist[1];
	
	while( (k<(nMaxMag-1)) && (nEdgeNb < nHighCount) )
	{
		k++;
		nEdgeNb += nHist[k];
	}
	nThdHigh = k ;
	
	nThdLow  = (int)((nThdHigh) * dRatioLow+ 0.5);	
// 	End EstimateThreshold


	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			
			if((ImgSrc[nPos] == 128) && (pnGradMAG[nPos] >= nThdHigh))
			{
				ImgSrc[nPos] = 255;
				FuncMe.I_TraceEdge(y, x, nThdLow, ImgSrc, pnGradMAG, nWidth);
			}
		}
	}
	
	for(y=0; y<nHeight; y++)
	{
		for(x=0; x<nWidth; x++)
		{
			nPos = y*nWidth + x ; 
			if(ImgSrc[nPos] != 255)
			{
				ImgSrc[nPos] = 0 ;
			}
			else
				outDirect_image[nPos] = DirMap[nPos];
		}
	 }
//...END Hysteresis
	// gray level process 灰階輸出
	for (i=0;i<nHeight;i++) //i控制列(高)
	{
		for (j=0; j<nWidth ; j++)        //k為陣列指標
		{
			out_image[i*nWidth+j]  =ImgSrc[i*nWidth+j];
// 			gpMsgbar->ShowMessage("-->(%d, %d): Degree: %d\n", j, i, DirMap[i*nWidth + j]);
		}
	}

delete []ImgSrc;
ImgSrc = NULL;
delete []ImgRlt;
ImgRlt = NULL;
delete []pnGradMAG;
pnGradMAG = NULL;
delete []pnGradX;
pnGradX = NULL;
delete []pnGradY;
pnGradY = NULL;
delete []DirMap;
DirMap = NULL;
}
void CRogerFunction::I_Sobel(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int Threshold)
{
	// x 方向微分
	int i, j;
	int nPixel = s_w * s_h;
	int *pnGradX = new int[s_w*s_h];
	int *pnGradY = new int[s_w*s_h];
	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
	}

	double dx[9]={-1,0,1,-2,0,2,-1,0,1};
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			pnGradX[i*s_w+j] = 
				dx[0] * in_image[((i-1)*s_w)+(j-1)]+
				dx[1] * in_image[((i-1)*s_w)+(j)]+
				dx[2] * in_image[((i-1)*s_w)+(j+1)]+
				dx[3] * in_image[i*s_w+(j-1)]+
				dx[4] * in_image[i*s_w+j]+
				dx[5] * in_image[i*s_w+(j+1)]+
				dx[6] * in_image[((i+1)*s_w)+(j-1)]+
				dx[7] * in_image[((i+1)*s_w)+j]+
				dx[8] * in_image[((i+1)*s_w)+(j+1)];
		}
	}

	// Y 方向微分
	double dy[9]={-1,-2,-1,0,0,0,1,2,1};
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			pnGradY[i*s_w+j] = 
				dy[0] * in_image[((i-1)*s_w)+(j-1)]+
				dy[1] * in_image[((i-1)*s_w)+(j)]+
				dy[2] * in_image[((i-1)*s_w)+(j+1)]+
				dy[3] * in_image[i*s_w+(j-1)]+
				dy[4] * in_image[i*s_w+j]+
				dy[5] * in_image[i*s_w+(j+1)]+
				dy[6] * in_image[((i+1)*s_w)+(j-1)]+
				dy[7] * in_image[((i+1)*s_w)+j]+
				dy[8] * in_image[((i+1)*s_w)+(j+1)];
		}
	}
	//....左右相剪


	//.......Threshold 設定為 200
	int sum = 0;
	int SSum = 0;
 	for(i=0; i<s_h; i++)
 	{
 		for(j=0; j<s_w; j++)
		{
			//可設訂pnGradX (垂直方向) , pnGradY (水平方向) , 同時 combine 也可以
			sum = abs(pnGradX[i*s_w+j]) + abs(pnGradY[i*s_w+j]);
			if (sum > 255) 
				sum = 255;
			else
				sum = 0;
// 			if (sum < 0 )  sum = 0;

			out_image[i*s_w+j] = /*255 - */(BYTE)sum;
		}
	}

//////////////////////////////////////////////////////////////////////////
	delete []pnGradX;
	delete []pnGradY;
}

void CRogerFunction::I_DerivativeX(double *in_image, double *out_image, int s_w, int s_h)
{
	// x 方向微分
	int i, j;
	double dx[9]={-1,0,1,-1,0,1,-1,0,1};
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			out_image[i*s_w+j] = 
				dx[0] * in_image[((i-1)*s_w)+(j-1)]+
				dx[1] * in_image[((i-1)*s_w)+(j)]+
				dx[2] * in_image[((i-1)*s_w)+(j+1)]+
				dx[3] * in_image[i*s_w+(j-1)]+
				dx[4] * in_image[i*s_w+j]+
				dx[5] * in_image[i*s_w+(j+1)]+
				dx[6] * in_image[((i+1)*s_w)+(j-1)]+
				dx[7] * in_image[((i+1)*s_w)+j]+
				dx[8] * in_image[((i+1)*s_w)+(j+1)];
		}
	}
}
void CRogerFunction::I_DerivativeY(double *in_image, double *out_image, int s_w, int s_h)
{
	// Y 方向微分
	int i, j;
	double dy[9]={-1,-1,-1,0,0,0,1,1,1};
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			out_image[i*s_w+j] = 
				dy[0] * in_image[((i-1)*s_w)+(j-1)]+
				dy[1] * in_image[((i-1)*s_w)+(j)]+
				dy[2] * in_image[((i-1)*s_w)+(j+1)]+
				dy[3] * in_image[i*s_w+(j-1)]+
				dy[4] * in_image[i*s_w+j]+
				dy[5] * in_image[i*s_w+(j+1)]+
				dy[6] * in_image[((i+1)*s_w)+(j-1)]+
				dy[7] * in_image[((i+1)*s_w)+j]+
				dy[8] * in_image[((i+1)*s_w)+(j+1)];
		}
	}
}
void CRogerFunction::I_DerivativeT(double *in_image0, double *in_image1, double *in_image2, double *out_image, int s_w, int s_h)
{
	// T 方向微分
	int i, j;
	double dt[9]={-1,0,1,-1,0,1,-1,0,1};
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			out_image[i*s_w+j] = 
				dt[0] * in_image0[(i-1)*s_w+j]+
				dt[1] * in_image1[(i-1)*s_w+j]+
				dt[2] * in_image2[(i-1)*s_w+j]+
				dt[3] * in_image0[ i   *s_w+j]+
				dt[4] * in_image1[ i   *s_w+j]+
				dt[5] * in_image2[ i   *s_w+j]+
				dt[6] * in_image0[(i+1)*s_w+j]+
				dt[7] * in_image1[(i+1)*s_w+j]+
				dt[8] * in_image2[(i+1)*s_w+j];
		}
	}
}
void CRogerFunction::I_HarrisCorner(BYTE *pImage, int s_w, int s_h, int Threshold, double Sigma, int Type)
{
	double *I   = new double[s_w*s_h];
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	int i, j, m, n ;
	memset(I, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);

	//微分
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			I[i*s_w+j] = (double)pImage[i*s_w+j];
		}
	}
	for (i=0; i<s_w*s_h; i++) pImage[i] = 0;

	I_DerivativeX(I, Ix, s_w, s_h);
	I_DerivativeY(I, Iy, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];

		}
	}
	//*** observe output start
// 	ofstream	fileOutput; 
// 	fileOutput.open("0Ix2H.txt", ios::out);
// 	for (i=0; i<s_h; i++)
// 		for (j=0; j<s_w; j++)
// 			if( Ix2[i*s_w+j] != 0)
// 				fileOutput <<  j << "," << i << "," << Ix2[i*s_w+j] << endl;
// 	fileOutput.close();
	//*** observe output end
//*******Gaussian******************************* 
	I_GaussianSmoothDBL(Ix2, Ix2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Iy2, Iy2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Ixy, Ixy, s_w, s_h, Sigma);
//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}

//**** 進行局部非極大值抑制獲得最終角點
	double Localmax;
	double WindowSize =  1 + 2 * ceil(3 * Sigma);
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定
	double Circle_Size = WindowSize;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold) //大於臨界值
				{
					gpMsgbar->ShowMessage("(%d, %d) %f\n", j, s_h-i, cim[i*s_w+j]);
					gpMsgbar->ShowMessage("Window Size = %.1f\n", Circle_Size);
					pImage[i*s_w+j]=255;   //滿足上二條件才是角點
// 					for (k=1; k<5; k++) //標籤大小 畫 +
// 					{
// 						pImage[(i)*s_w+(j-k)]=255;
// 						pImage[(i)*s_w+(j+k)]=255;
// 						pImage[(i-k)*s_w+(j)]=255;
// 						pImage[(i+k)*s_w+(j)]=255;
// 					}
// 					if ( Type == 0 )
// 					{
// 						for ( d=0; d<360; d++) // 0-360 degree //畫圓
// 						{
// 							x = j + Circle_Size * cos(d*PI/180);
// 							y = i + Circle_Size * sin(d*PI/180);
// 							pImage[ y * s_w + x ] = 255;
// 						}
// 					}
// 					if ( Type == 1 )
// 					{
// 						for ( d=-2; d<= 2 ; d++) // 0-360 degree //畫線
// 						{
// 							x = j + d ;
// 							y = i;
// 							pImage[ y * s_w + x ] = 255;
// 							x = j  ;
// 							y = i + d;
// 							pImage[ y * s_w + x ] = 255;
// 						}
// 					}

				}
		}
	}
	delete []I   ;
	delete []Ix  ;
	delete []Ix2 ;
	delete []Iy  ;
	delete []Iy2 ;
	delete []Ixy ;
	delete []cim ;
	delete []mx  ;
}

void CRogerFunction::I_HarrisCorner(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int Threshold, double Sigma)
{
	double *I   = new double[s_w*s_h];
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	int i, j, m, n;
	memset(I, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);

	//微分
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			I[i*s_w+j] = (double)in_image[i*s_w+j];
		}
	}
	for (i=0; i<s_w*s_h; i++) out_image[i] = 0;

	I_DerivativeX(I, Ix, s_w, s_h);
	I_DerivativeY(I, Iy, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];

		}
	}
	I_GaussianSmoothDBL(Ix2, Ix2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Iy2, Iy2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Ixy, Ixy, s_w, s_h, Sigma);
//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}

//**** 進行局部非極大值抑制獲得最終角點
	double Localmax;
	double WindowSize =  1 + 2 * ceil(3 * Sigma);
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定
	int Circle_Size = 5;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold) //大於臨界值
				{
					out_image[ i * s_w + j ] = 255;

// 					for ( d=0; d<360; d++) // 0-360 degree
// 					{
// 						x = j + Circle_Size * cos(d*PI/180);
// 						y = i + Circle_Size * sin(d*PI/180);
// 						out_image[ y * s_w + x ] = 255;
// 					}
				}
		}
	}
	delete []I   ;
	delete []Ix  ;
	delete []Ix2 ;
	delete []Iy  ;
	delete []Iy2 ;
	delete []Ixy ;
	delete []cim ;
	delete []mx  ;
}
void CRogerFunction::I_STIP(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{
	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	int i, j, m, n, x, y, d;
// 	int  *DirMap = new  int[s_w*s_h];

	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//****Direction Info.
	int Theta;

	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];
		
	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}
//*******Gaussian******************************* 

	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);


//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] =  (Ix2[i*s_w+j]*Iy2[i*s_w+j]*It2[i*s_w+j] +
				            Ixy[i*s_w+j]*Iyt[i*s_w+j]*Ixt[i*s_w+j] +
							Ixt[i*s_w+j]*Ixy[i*s_w+j]*Iyt[i*s_w+j] - 
							Iy2[i*s_w+j]*Ixt[i*s_w+j]*Ixt[i*s_w+j] -
							Ix2[i*s_w+j]*Iyt[i*s_w+j]*Iyt[i*s_w+j] -
							It2[i*s_w+j]*Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+It2[i*s_w+j]+0.000001);
		}
	}

//**** 進行局部非極大值抑制獲得最終角點
	double Localmax;
	double WindowSize =  5;
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	int Circle_Size = 10;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold) //大於臨界值
				{
					gpMsgbar->ShowMessage("(%d, %d) %f\n", j, s_h-i, cim[i*s_w+j]);
// 					out_Image[i*s_w+j]=255;   //滿足上二條件才是角點
					//*****畫cross
// 					for (k=1; k<5; k++) //標籤大小 畫 +
// 					{
// 						out_Image[(i)*s_w+(j-k)]=255;
// 						out_Image[(i)*s_w+(j+k)]=255;
// 						out_Image[(i-k)*s_w+(j)]=255;
// 						out_Image[(i+k)*s_w+(j)]=255;
// 					}
					//*****畫圓
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + Circle_Size * cos(d*PI/180);
						y = i + Circle_Size * sin(d*PI/180);
						out_Image[ y * s_w + x ] = 255;
					}
//......test start
					//*****畫圓心
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + 2 * cos(d*PI/180);
						y = i + 2 * sin(d*PI/180);
						out_Image[ y * s_w + x ] = 255;
					}
					w = Ix2[i*s_w+j]*Iy2[i*s_w+j]-2*Ixy[i*s_w+j];
					u = (-1*Ixt[i*s_w+j]*Iy2[i*s_w+j]+Iyt[i*s_w+j]*Ixy[i*s_w+j])/w;
					v = (-1*Ix2[i*s_w+j]*Iyt[i*s_w+j]+Ixt[i*s_w+j]*Ixy[i*s_w+j])/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf((double)v/(double)u) * 57.3;
					else // 第四象限
						Theta = atanf((double)v/(double)u) * 57.3;
					for ( d=0; d<Circle_Size; d++) //畫線
					{
						x = j + d * cos(Theta*PI/180)+1;
						y = i + d * sin(Theta*PI/180);
						out_Image[ y * s_w + x ] = 255;
					}
//...........test end
				}
		}
	}
	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;
}
void CRogerFunction::I_STIPLocalDirection(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{
	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	int i, j, m, n, x, y, d;
	int LocalDis=5;
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//****Direction Info.
	int Theta;

	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];
		
	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}
//*******Gaussian******************************* 

	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);


//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] =  (Ix2[i*s_w+j]*Iy2[i*s_w+j]*It2[i*s_w+j] +
				            Ixy[i*s_w+j]*Iyt[i*s_w+j]*Ixt[i*s_w+j] +
							Ixt[i*s_w+j]*Ixy[i*s_w+j]*Iyt[i*s_w+j] - 
							Iy2[i*s_w+j]*Ixt[i*s_w+j]*Ixt[i*s_w+j] -
							Ix2[i*s_w+j]*Iyt[i*s_w+j]*Iyt[i*s_w+j] -
							It2[i*s_w+j]*Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+It2[i*s_w+j]+0.000001);
		}
	}

//**** 進行局部非極大值抑制獲得最終角點
	double Localmax;
	double WindowSize =  5;
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt;
	int Circle_Size = 10;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold) //大於臨界值
				{
					gpMsgbar->ShowMessage("(%d, %d) %f\n", j, s_h-i, cim[i*s_w+j]);
// 					out_Image[i*s_w+j]=255;   //滿足上二條件才是角點
// 					for (k=1; k<5; k++) //標籤大小 畫 +
// 					{
// 						out_Image[(i)*s_w+(j-k)]=255;
// 						out_Image[(i)*s_w+(j+k)]=255;
// 						out_Image[(i-k)*s_w+(j)]=255;
// 						out_Image[(i+k)*s_w+(j)]=255;
// 					}
					//*****畫圓
// 					for ( d=0; d<360; d++) // 0-360 degree
// 					{
// 						x = j + Circle_Size * cos(d*PI/180);
// 						y = i + Circle_Size * sin(d*PI/180);
// 						out_Image[ y * s_w + x ] = 255;
// 					}
// 					for ( d=0; d<Circle_Size; d++)
// 					{
// 						
// 						out_Image[i * s_w + (j+d)] = 255;
// 					}
//......test start
					//*****畫圓心
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + 2 * cos(d*PI/180);
						y = i + 2 * sin(d*PI/180);
						out_Image[ y * s_w + x ] = 200;
					}
					//取interest point 週遭pixel來決定方向
					ix2 = iy2 = ixy = ixt = iyt = 0;
					for (m=-1*LocalDis; m<=LocalDis; m++)
					{
						for (n=-1*LocalDis; n<=LocalDis; n++)
						{
							ix2 += Ix2[(i+m)*s_w+(j+n)];
							iy2 += Iy2[(i+m)*s_w+(j+n)];
							ixy += Ixy[(i+m)*s_w+(j+n)];
							ixt += Ixt[(i+m)*s_w+(j+n)];
							iyt += Iyt[(i+m)*s_w+(j+n)];
						}
					}
					// Optical Flow
					w = ix2*iy2-2*ixy;
					u = (-1*ixt*iy2+iyt*ixy)/w;
					v = (-1*ix2*iyt+ixt*ixy)/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf((double)v/(double)u) * 57.3;
					else // 第四象限
						Theta = atanf((double)v/(double)u) * 57.3;
					for ( d=0; d<Circle_Size; d++)
					{
						x = j + d * cos(Theta*PI/180)+1;
						y = i + d * sin(Theta*PI/180);
						out_Image[ y * s_w + x ] = 200;
					}
					gpMsgbar->ShowMessage("(%d, %d) %f %f %f %d\n", j, s_h-i, Ix[i*s_w+j], Iy[i*s_w+j], It[i*s_w+j], Theta);
					

//...........test end
				}
		}
	}
	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;
}
void CRogerFunction::I_EigenValue_3x3(double A[EigenSize3x3][EigenSize3x3] , double &EigenValue)         
{ 
	//data   input 
// 	double   A[EigenSize3x3][EigenSize3x3]={	{  0,   0, -2}, 
// 												{  1,   2,  1},
// 												{  1,   0,  3}}; 
	int i;
	double   x[EigenSize3x3]={1,1,1}; 
	double   v[EigenSize3x3]={0,0,0}; 
	double   u[EigenSize3x3]={0,0,0}; 
	double   p[EigenSize3x3]={0,0,0}; 
	double   e=1e-10,delta=1; 
	int   k=0; 
	double Sol, Sol1;
	while( delta >= e) 
	{ 
		for(int   q=0;q <EigenSize3x3;q++)   p[q]=v[q]; 
		for(i=0;i <EigenSize3x3;i++) 
		{ 
			v[i]=0; 
			for(int   j=0;j <EigenSize3x3;j++) 
				v[i]+=A[i][j]*x[j]; 
		} 
		
		for(   i=0;i <EigenSize3x3-1;i++)   Sol=v[i] > v[i+1]? v[i] : v[i+1]; 
		for(   i=0;i <EigenSize3x3;i++)   u[i]=v[i]/Sol; 
		for(   i=0;i <EigenSize3x3-1;i++) 
		{
			Sol =v[i] > v[i+1]? v[i] : v[i+1]; 
			Sol1=p[i] > p[i+1]? p[i] : p[i+1];
		}
		delta=fabs(Sol-Sol1); 
		k++; 
		for(int   l=0;l <EigenSize3x3;l++)   x[l]=u[l]; 
	}
	EigenValue = Sol;
// 	cout   <<   "迭代次數:"   <<   k   <<   endl; 
// 	cout   <<   "矩陣的特征值:"   <<   Sol   <<   endl; 
// 	cout   <<   "( "   ; 
// 	for( i=0;i <EigenSize3x3;i++)   
// 		cout   <<   u[i]   <<   "   "   ; 
// 	cout     <<   ") "   <<   endl; 
}   
void CRogerFunction::I_EigenValue_2x2(double A[EigenSize2x2][EigenSize2x2] , double &EigenValue1, double &EigenValue2)         
{ 
	//data   input
// 	double   A[EigenSize2x2][EigenSize2x2]={	{ 3,  0}, 
// 												{ 8, -1}}; 	
//////////////////////////////////////////////////////////////////////////
//	  3, 2
//   -1, 0
//   eigenvalue= 1, 2
//	  0.5 , 0.25
//    0.25, 0.5
//   eigenvalue= 0.25, 0.75
// 	double AA[2][2] = {{3, 2},{-1,0}};
// 	double EigenV1=0, EigenV2=0;
// 	FuncMe.EigenValue_2x2(AA, EigenV1, EigenV2);
//////////////////////////////////////////////////////////////////////////

	// 採用線性代數EigenValue求算
	double B, C;
	B = -(A[0][0] + A[1][1]);
	C = (A[0][0]*A[1][1] - A[1][0]*A[0][1]);

	EigenValue1 = (-B + sqrtf(B*B-4*C))/2;
	EigenValue2 = (-B - sqrtf(B*B-4*C))/2;
} 
void CRogerFunction::I_OpticalFlow(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, vector<ObjectInformation> *vi, int Corner_Threshold, int OF_Threshold, double Sigma)
{
	ObjectInformation Object;
	int i, j, m, n, x, y, c, d;
	double Theta;
	double Localmax;
// 	double WindowSize =  10;
	double WindowSize = 1 + 2 * ceil(3 * Sigma);

	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//*********************************
	//////////////////////////////////////////////////////////////////////////
	double  *p_Ix   = Ix,
 			*p_Ix2  = Ix2,
 			*p_Iy   = Iy,
 			*p_Iy2  = Iy2,
 			*p_Ixy  = Ixy,
 			*p_cim  = cim,
 			*p_mx   = mx,
 			*p_It   = It,
 			*p_Ixt  = Ixt,
			*p_Iyt  = Iyt;
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];

	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++, p_Ix+=s_w, p_Iy+=s_w, p_It+=s_w, p_Ixy+=s_w, p_Ix2+=s_w, p_Iy2+=s_w, p_Ixt+=s_w, p_Iyt+=s_w)
	{
		for (j=1; j<s_w-1; j++)
		{
			p_Ix2[j] = p_Ix[j]*p_Ix[j];
			p_Iy2[j] = p_Iy[j]*p_Iy[j];
			p_Ixy[j] = p_Ix[j]*p_Iy[j];
			p_Ixt[j] = p_Ix[j]*p_It[j];
			p_Iyt[j] = p_Iy[j]*p_It[j];
		}
	}
	p_Ix=Ix, p_Iy=Iy, p_It=It, p_Ixy=Ixy, p_Ix2=Ix2, p_Iy2=Iy2, p_Ixt=Ixt, p_Iyt=Iyt;
	//////////////////////////////////////////////////////////////////////////
//*******Gaussian******************************* 
	I_GaussianSmoothDBL(Ix2, Ix2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Iy2, Iy2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Ixy, Ixy, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Ixt, Ixt, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Iyt, Iyt, s_w, s_h, Sigma);
	//****計算角點量//////////////////////////////////////////////////////////////////////////
	for (i=0; i<s_h; i++, p_cim+=s_w, p_Ix2+=s_w, p_Iy2+=s_w, p_Ixy+=s_w)
	{
		for (j=0; j<s_w; j++)
		{
			p_cim[j] = (p_Ix2[j]*p_Iy2[j] - p_Ixy[j]*p_Ixy[j])/(p_Ix2[j]+p_Iy2[j]+0.000001);//加0.000001防止除0
		}
	}
	p_cim=cim, p_Ix2=Ix2, p_Iy2=Iy2, p_Ixy=Ixy;
	//**** 進行局部非極大值抑制獲得最終角點//////////////////////////////////////////////////////////////////////////
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt, it;
	double T_Dis;
	for(i = WindowSize; i < s_h-WindowSize; i++)
	{
		for(j = WindowSize ; j < s_w-WindowSize; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
			{
				if(mx[i*s_w+j] > Corner_Threshold ) //大於臨界值
				{
				
					//取interest point 週遭pixel來決定方向
					ix2 = iy2 = ixy = ixt = iyt = it = 0;
					for ( d=0; d<360; d+=6) // 0-360 degree
					{
						for (c=0; c<WindowSize; c++)
						{
							x = j + c * cos(d*0.017453292);
							y = i + c * sin(d*0.017453292);
							ix2 += Ix2[ y * s_w + x ];
							iy2 += Iy2[ y * s_w + x ];
							ixy += Ixy[ y * s_w + x ];
							ixt += Ixt[ y * s_w + x ];
							iyt += Iyt[ y * s_w + x ];
							it  += It[ y * s_w + x ];
						}
					}
					// Optical Flow
					w = ix2*iy2-2*ixy;
					u = (-ixt*iy2+iyt*ixy)/w;
					v = (-ix2*iyt+ixt*ixy)/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf(v/u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf(v/u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf(v/u) * 57.3;
					else // 第四象限
						Theta = atanf(v/u) * 57.3;
					T_Dis = fabs(it) / 500;
					if ( T_Dis > OF_Threshold )
					{
						Object.x  = j;
						Object.y  = i;
						Object.Theta = Theta;
						Object.Type = 0;
						Object.Type_Amount = 0;
						Object.Life_Match   = 0;
						Object.Life_UnMatch = 0;
						Object.Status_Match = FALSE;
						vi->push_back(Object);
						for ( d=0; d< 8/*WindowSize* T_Dis/40*/ ; d++) //畫線
						{
							x = j + d * cos(Theta*0.017453292); //PI/180);
							y = i + d * sin(Theta*0.017453292);
							if( x > 0 && x < s_w && y > 0 && y< s_h)
								out_Image[ y * s_w + x ] = -200;
						}
						//*****畫圓心
						out_Image[(i-1)*s_w+j] = -50;
						out_Image[(i+1)*s_w+j] = -50;
						out_Image[i*s_w+j]     = -100;
						out_Image[i*s_w+(j-1)] = -50;
						out_Image[i*s_w+(j+1)] = -50;
					}

				}
			}
		}
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;
	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;
	delete []It;
	delete []Ixt;
	delete []Iyt;
}
void CRogerFunction::I_OpticalFlow(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{
	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	int i, j, m, n, x, y, d;
	int LocalDis=5;
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//****Direction Info.
	int Theta;

	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];
		
	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}
//*******Gaussian******************************* 

	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);

	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt;
	int Circle_Size = 10;
	for(i = Circle_Size; i < s_h - Circle_Size; i+=10)
	{
		for(j = Circle_Size ; j < s_w - Circle_Size; j+=10)
		{	

		//*****畫圓心
			for ( d=0; d<360; d++) // 0-360 degree
			{
				x = j + 2 * cos(d*PI/180);
				y = i + 2 * sin(d*PI/180);
				out_Image[ y * s_w + x ] = 255;
			}
		//取interest point 週遭pixel來決定方向
			ix2 = iy2 = ixy = ixt = iyt = 0;
			for (m=-1*LocalDis; m<=LocalDis; m++)
			{
				for (n=-1*LocalDis; n<=LocalDis; n++)
				{
					ix2 += Ix2[(i+m)*s_w+(j+n)];
					iy2 += Iy2[(i+m)*s_w+(j+n)];
					ixy += Ixy[(i+m)*s_w+(j+n)];
					ixt += Ixt[(i+m)*s_w+(j+n)];
					iyt += Iyt[(i+m)*s_w+(j+n)];
				}
			}
		// Optical Flow
			w = ix2*iy2-2*ixy;
			u = (-1*ixt*iy2+iyt*ixy)/w;
			v = (-1*ix2*iyt+ixt*ixy)/w;
			if (u ==0 && v !=0)  // y 方向
				Theta = 90;
			else if (u !=0 && v ==0) // x 方向
				Theta = 0;
			else if (u > 0 && v > 0) // 第一象限
				Theta = atanf((double)v/(double)u) * 57.3;
			else if (u < 0 && v > 0) // 第二象限
				Theta = 180 + atanf((double)v/(double)u) * 57.3;
			else if (u < 0 && v < 0) // 第三象限
				Theta = -180 + atanf((double)v/(double)u) * 57.3;
			else // 第四象限
				Theta = atanf((double)v/(double)u) * 57.3;
			for ( d=0; d<Circle_Size; d++)
			{
				x = j + d * cos(Theta*PI/180)+1;
				y = i + d * sin(Theta*PI/180);
				out_Image[ y * s_w + x ] = 255;
			}
			gpMsgbar->ShowMessage("(%d, %d) %f %f %f %d\n", j, s_h-i, Ix[i*s_w+j], Iy[i*s_w+j], It[i*s_w+j], Theta);
//...........test end
			
		}
	}
	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;
}

void CRogerFunction::I_HARRISDirectionEdge(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{
	int i, j, k, m, n, x, y, c, d;
	int LocalDis=5;
	int Theta;
	double Localmax;
	double WindowSize =  10;

	BYTE *T00S   = new BYTE[s_w*s_h]; // t-1
	BYTE *T01S   = new BYTE[s_w*s_h]; // t-1
	BYTE *T02S   = new BYTE[s_w*s_h]; // t-1

	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//*********************************
	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];

	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}

//*******Gaussian******************************* 
	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);


//Canny*************************************************************** 
//  	double Canny_Sigma = 0.2;
	double dRatioHigh= 0.8;
	double dRatioLow= 0.2;
	int nPos, g1, g2, g3, g4, nMaxMag, nEdgeNb, nHighCount;
	float gx, gy;
	int nHist[1024];
	double dTmp, dTmp1, dTmp2, weight;
	int nThdHigh ;
	int nThdLow  ;
	int *nGradMAG = new int[s_w*s_h];
	double *DirMap = new double[s_w*s_h];
	BYTE *CannyMap;
	BYTE *ImgRlt;
	CannyMap = new BYTE[s_w*s_h];
	ImgRlt = new BYTE[s_w*s_h];
	//梯度計算
	for (i=0; i<s_w*s_h; i++) nGradMAG[i] = (int)sqrt(Ix2[i]+Iy2[i]+0.5);
	//Non-Maximum
	for(x=0; x<s_w; x++)		
	{
		ImgRlt[x] = 0 ;
		ImgRlt[s_h-1+x] = 0;
	}
	for(y=0; y<s_h; y++)		
	{
		ImgRlt[y*s_w] = 0 ;
		ImgRlt[y*s_w + s_w-1] = 0;
	}
	for(y=1; y<s_h-1; y++)
	{
		for(x=1; x<s_w-1; x++)
		{
			nPos = y*s_w + x ;
			if(nGradMAG[nPos] == 0 )
			{
				ImgRlt[nPos] = 0 ;
			}
			else
			{
				dTmp = nGradMAG[nPos] ;
				gx = Ix[nPos]  ;
				gy = Iy[nPos]  ;
				if (abs(gy) < abs(gx)) 
				{
					weight = fabs(gx)/fabs(gy); 

					g2 = nGradMAG[nPos-s_w] ; 
					g4 = nGradMAG[nPos+s_w] ;

					if (gx*gy > 0) 
					{ 					
						g1 = nGradMAG[nPos-s_w-1] ;
						g3 = nGradMAG[nPos+s_w+1] ;
					} 
					else 
					{ 
						g1 = nGradMAG[nPos-s_w+1] ;
						g3 = nGradMAG[nPos+s_w-1] ;
					} 
				}
				else
				{
					weight = fabs(gy)/fabs(gx); 
					
					g2 = nGradMAG[nPos+1] ; 
					g4 = nGradMAG[nPos-1] ;
					if (gx*gy > 0) 
					{				
						g1 = nGradMAG[nPos+s_w+1] ;
						g3 = nGradMAG[nPos-s_w-1] ;
					} 
					else 
					{ 
						g1 = nGradMAG[nPos-s_w+1] ;
						g3 = nGradMAG[nPos+s_w-1] ;
					}
				}
					dTmp1 = weight*g1 + (1-weight)*g2 ;
					dTmp2 = weight*g3 + (1-weight)*g4 ;
					if(dTmp> dTmp1 && dTmp> dTmp2)
					{
						ImgRlt[nPos] = 128 ;
					}
					else
					{
						ImgRlt[nPos] = 0 ;
					}
			} //else
		} // for
	}
	for (i=0; i<s_w*s_h;i++) CannyMap[i] = ImgRlt[i];
//...END Non-Maximum
//...Hysteresis
// 	EstimateThreshold
	nMaxMag = 0;

	for(k=0; k<1024; k++) 
	{
		nHist[k] = 0; 
	}
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			if(CannyMap[y*s_w+x]==128)
			{
				nHist[ nGradMAG[y*s_w+x] ]++;
			}
		}
	}
	
	nEdgeNb = nHist[0]  ;
	nMaxMag = 0         ;
	for(k=1; k<1024; k++)
	{
		if(nHist[k] != 0)
		{
			nMaxMag = k;
		}
		nEdgeNb += nHist[k];
	}
	nHighCount = (int)(dRatioHigh * nEdgeNb +0.5);
	
	k = 1;
	nEdgeNb = nHist[1];
	
	while( (k<(nMaxMag-1)) && (nEdgeNb < nHighCount) )
	{
		k++;
		nEdgeNb += nHist[k];
	}
	nThdHigh = k ;
	
	nThdLow  = (int)((nThdHigh) * dRatioLow+ 0.5);	
// 	End EstimateThreshold
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			nPos = y*s_w + x ; 
			
			if((CannyMap[nPos] == 128) && (nGradMAG[nPos] >= nThdHigh))
			{
				CannyMap[nPos] = 255;
				I_TraceEdge(y, x, nThdLow, CannyMap, nGradMAG, s_w);
			}
		}
	}
	
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			nPos = y*s_w + x ; 
			if(CannyMap[nPos] != 255)
			{
				CannyMap[nPos] = 0 ;
			}
		}
	 }
//*** 消除canny map 細邊
//****DirectionMap
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			if (nGradMAG[y*s_w+x] != 0)
			{
				if (Ix[y*s_w + x] ==0 && Iy[y*s_w + x] !=0)  // y 方向
				{
					DirMap[y*s_w + x]   = 90;
				}
				else if (Ix[y*s_w + x] !=0 && Iy[y*s_w + x] ==0) // x 方向
				{
					DirMap[y*s_w + x]   = 0;
				}
				else if (Ix[y*s_w + x] > 0 && Iy[y*s_w + x] > 0) // 第一象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					DirMap[y*s_w + x]   = Theta; 
				}
				else if (Ix[y*s_w + x] < 0 && Iy[y*s_w + x] > 0) // 第二象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					Theta = 180 + Theta;
					DirMap[y*s_w + x]   = Theta; 
				}
				else if (Ix[y*s_w + x] < 0 && Iy[y*s_w + x] < 0) // 第三象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					Theta = -180 + Theta;
					DirMap[y*s_w + x]   = Theta; 
				}
				else // 第四象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					DirMap[y*s_w + x]   = Theta;    
				}
			}
		}
	}
//***CANNY MAP 輸出
// 	for (i=0; i<s_w*s_h; i++)
// 		out_Image[i] = CannyMap[i];
	//...END Hysteresis

//Canny End*********************************************************** 
//****計算角點量
// 	for (i=0; i<s_h; i++)
// 	{
// 		for (j=0; j<s_w; j++)
// 		{
// 			cim[i*s_w+j] =  (Ix2[i*s_w+j]*Iy2[i*s_w+j]*It2[i*s_w+j] +
// 				            Ixy[i*s_w+j]*Iyt[i*s_w+j]*Ixt[i*s_w+j] +
// 							Ixt[i*s_w+j]*Ixy[i*s_w+j]*Iyt[i*s_w+j] - 
// 							Iy2[i*s_w+j]*Ixt[i*s_w+j]*Ixt[i*s_w+j] -
// 							Ix2[i*s_w+j]*Iyt[i*s_w+j]*Iyt[i*s_w+j] -
// 							It2[i*s_w+j]*Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+It2[i*s_w+j]+0.000001);
// 		}
// 	}
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}
//**** 進行局部非極大值抑制獲得最終角點

	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt;
	int Circle_Size = 10;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold ) //大於臨界值
				{
					gpMsgbar->ShowMessage("It: (%d, %d) %f\n", j, s_h-i, It[i*s_w+j]);
// 					out_Image[i*s_w+j]=255;   //滿足上二條件才是角點
// 					for (k=1; k<5; k++) //標籤大小 畫 +
// 					{
// 						out_Image[(i)*s_w+(j-k)]=255;
// 						out_Image[(i)*s_w+(j+k)]=255;
// 						out_Image[(i-k)*s_w+(j)]=255;
// 						out_Image[(i+k)*s_w+(j)]=255;
// 					}
					//*****畫圓
// 					for ( d=0; d<360; d++) // 0-360 degree
// 					{
// 						x = j + Circle_Size * cos(d*PI/180);
// 						y = i + Circle_Size * sin(d*PI/180);
// 						out_Image[ y * s_w + x ] = 200;
// 					}
//......test start
					//*****畫圓心
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + 2 * cos(d*PI/180);
						y = i + 2 * sin(d*PI/180);
						out_Image[ y * s_w + x ] = 100;
					}
					//取interest point 週遭pixel來決定方向
					ix2 = iy2 = ixy = ixt = iyt = 0;
					for (m=-1*LocalDis; m<=LocalDis; m++)
					{
						for (n=-1*LocalDis; n<=LocalDis; n++)
						{
							ix2 += Ix2[(i+m)*s_w+(j+n)];
							iy2 += Iy2[(i+m)*s_w+(j+n)];
							ixy += Ixy[(i+m)*s_w+(j+n)];
							ixt += Ixt[(i+m)*s_w+(j+n)];
							iyt += Iyt[(i+m)*s_w+(j+n)];
						}
					}
					// Optical Flow
					w = ix2*iy2-2*ixy;
					u = (-1*ixt*iy2+iyt*ixy)/w;
					v = (-1*ix2*iyt+ixt*ixy)/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf((double)v/(double)u) * 57.3;
					else // 第四象限
						Theta = atanf((double)v/(double)u) * 57.3;
					for ( d=0; d<Circle_Size; d++) //畫線
					{
						x = j + d * cos(Theta*PI/180)+1;
						y = i + d * sin(Theta*PI/180);
						out_Image[ y * s_w + x ] = 200;
					}

					//圓範圍內Canny邊Direction
					for (c=0; c<Circle_Size; c++)
					{
						x = j + c * cos(DirMap[i*s_w+j]*PI/180)+1;
						y = i + c * sin(DirMap[i*s_w+j]*PI/180);
						out_Image[ y * s_w + x ] = 50;
					}
					//圓範圍內Canny邊px與區域圓心角度統計
					//....
					int Hist[180];
					int Tbl[180];
					//***************************************************
					for (d=0; d<180; d++) Hist[d] = 0;
					for (d=0; d<360; d++)
					{
 						for (c=0; c<Circle_Size; c++)
 						{
 							x = j + c * cos(d*PI/180);
 							y = i + c * sin(d*PI/180);
 							if ( CannyMap[y*s_w+x] == 255)
							{
								Theta = atanf((double)(y-i)/(double)(x-j + 0.00001)) * 57.3;
								if (Theta<0) Theta += 180;
								Hist[Theta]++;
							}
 						}
					}
					Hist[0] = 0;
					Hist[90] = 0;
					for (d=0; d<180; d++) Tbl[d] = Hist[d];
					I_quicksort(Hist,0,179,180);
					for (d=0; d< 180; d++)
					{
						if( Tbl[d] == Hist[0]) 
						{ 
							Theta = d;
							d = 180;
						}
					}
					for ( d=0; d<Circle_Size; d++) //畫線
					{
						x = j + d * cos(Theta*PI/180)+1;
						y = i + d * sin(Theta*PI/180);
						out_Image[ y * s_w + x ] = 100;
					}

 					//***************************************************
					//圓範圍內Canny邊edgelet
// 					for ( d=0; d<360; d++) // 0-360 degree
// 					{
// 						for (c=0; c<Circle_Size; c++)
// 						{
// 							x = j + c * cos(d*PI/180);
// 							y = i + c * sin(d*PI/180);
// 							if (CannyMap[y*s_w+x] == 255)
// 								out_Image[ y * s_w + x ] = 50;
// 						}
// 					}
//...........test end
				}
		}
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;

	delete []CannyMap;
	delete []ImgRlt;
	delete []nGradMAG;
	delete []DirMap;

	delete []T00S;
	delete []T01S;
	delete []T02S;
}
void CRogerFunction::I_OpticalFlowDir(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int Threshold, double Sigma)
{
	int i, j, m, n, x, y, c, d;
	int Theta;
	double Localmax;
// 	double WindowSize =  10;
	double WindowSize =  1 + 2 * ceil( 3 * Sigma);
	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//*********************************
	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];

	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}

//*******Gaussian******************************* 
	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);
//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}
//**** 進行局部非極大值抑制獲得最終角點

	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt, it;
	int Circle_Size = WindowSize;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold  ) //大於臨界值
				{

					//取interest point 週遭pixel來決定方向
					ix2 = iy2 = ixy = ixt = iyt = it = 0;
					for ( d=0; d<360; d++) // 0-360 degree
					{
						for (c=0; c<Circle_Size; c++)
						{
							x = j + c * cos(d*PI/180);
							y = i + c * sin(d*PI/180);
							ix2 += Ix2[ y * s_w + x ];
							iy2 += Iy2[ y * s_w + x ];
							ixy += Ixy[ y * s_w + x ];
							ixt += Ixt[ y * s_w + x ];
							iyt += Iyt[ y * s_w + x ];
							it  += It[ y * s_w + x ];

						}
					}
					// Optical Flow
					w = ix2*iy2-2*ixy;           //...camera's rule to find u, v's magnitude
					u = (-1*ixt*iy2+iyt*ixy)/w;
					v = (-1*ix2*iyt+ixt*ixy)/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf((double)v/(double)u) * 57.3;
					else // 第四象限
						Theta = atanf((double)v/(double)u) * 57.3;

					double T_Dis = Circle_Size* fabs(it) / 100000;
					if (T_Dis > 3)
					{
						gpMsgbar->ShowMessage("%f\n", T_Dis);
						for ( d=0; d<T_Dis; d++) //畫線
						{
							x = j + d * cos(Theta*PI/180)+1;
							y = i + d * sin(Theta*PI/180);
							if( x > 0 && x < s_w && y > 0 && y< s_h)
								out_Image[ y * s_w + x ] = 255;
						}
						//*****畫圓心
						for ( d=0; d<360; d++) // 0-360 degree
						{
							x = j + 2 * cos(d*PI/180);
							y = i + 2 * sin(d*PI/180);
							out_Image[ y * s_w + x ] = 255;
						}
						//*****畫圓
						for ( d=0; d<360; d++) // 0-360 degree
						{
							x = j + Circle_Size * cos(d*PI/180);
							y = i + Circle_Size * sin(d*PI/180);
							out_Image[ y * s_w + x ] = 255;
						}
					}
					//***************************************************

				}
		}
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;
}


void CRogerFunction::I_OpticalFlow2(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{

	int i;
	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	//********************************

	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);

	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];
		
	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);

	for (i=0; i<s_w*s_h; i++)
	{
/*		if (abs(It[i]) > 200)*/
			out_Image[i] = It[i];
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Iy;
	delete []cim;
	delete []mx;

	delete []It;
}
void CRogerFunction::I_OutputGraph(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{
	//本程式觀察 Ix Iy It等輸出
	int i, j;
	int LocalDis=5;
	double WindowSize =  10;

	BYTE *Temp   = new BYTE[s_w*s_h];
	memset(Temp, 0, sizeof(BYTE)*s_w*s_h);

	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//*********************************
	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];

	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}
	//本程式觀察 Ix Iy It等輸出
	for (i=0; i<s_w*s_h; i++)
	{
		out_Image[i] = It[i] ;
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;

}


void CRogerFunction::I_HARRISDirectionEdge1(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{
	int i, j, k, m, n, x, y, c, d;
	int LocalDis=5;
	int Theta;
	double Localmax;
	double WindowSize =  10;

	BYTE *Temp   = new BYTE[s_w*s_h];
	memset(Temp, 0, sizeof(BYTE)*s_w*s_h);

	BYTE *T00S   = new BYTE[s_w*s_h]; // t-1
	BYTE *T01S   = new BYTE[s_w*s_h]; // t-1
	BYTE *T02S   = new BYTE[s_w*s_h]; // t-1

	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//*********************************
	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];

	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}

//*******Gaussian******************************* 
	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);


//Canny*************************************************************** 
//  	double Canny_Sigma = 0.2;
	double dRatioHigh= 0.8;
	double dRatioLow= 0.2;
	int nPos, g1, g2, g3, g4, nMaxMag, nEdgeNb, nHighCount;
	float gx, gy;
	int nHist[1024];
	double dTmp, dTmp1, dTmp2, weight;
	int nThdHigh ;
	int nThdLow  ;
	int *nGradMAG = new int[s_w*s_h];
	double *DirMap = new double[s_w*s_h];
	BYTE *CannyMap;
	BYTE *ImgRlt;
	CannyMap = new BYTE[s_w*s_h];
	ImgRlt = new BYTE[s_w*s_h];
	//梯度計算
	for (i=0; i<s_w*s_h; i++) nGradMAG[i] = (int)sqrt(Ix2[i]+Iy2[i]+0.5);
	//Non-Maximum
	for(x=0; x<s_w; x++)		
	{
		ImgRlt[x] = 0 ;
		ImgRlt[s_h-1+x] = 0;
	}
	for(y=0; y<s_h; y++)		
	{
		ImgRlt[y*s_w] = 0 ;
		ImgRlt[y*s_w + s_w-1] = 0;
	}
	for(y=1; y<s_h-1; y++)
	{
		for(x=1; x<s_w-1; x++)
		{
			nPos = y*s_w + x ;
			if(nGradMAG[nPos] == 0 )
			{
				ImgRlt[nPos] = 0 ;
			}
			else
			{
				dTmp = nGradMAG[nPos] ;
				gx = Ix[nPos]  ;
				gy = Iy[nPos]  ;
				if (abs(gy) < abs(gx)) 
				{
					weight = fabs(gx)/fabs(gy); 

					g2 = nGradMAG[nPos-s_w] ; 
					g4 = nGradMAG[nPos+s_w] ;

					if (gx*gy > 0) 
					{ 					
						g1 = nGradMAG[nPos-s_w-1] ;
						g3 = nGradMAG[nPos+s_w+1] ;
					} 
					else 
					{ 
						g1 = nGradMAG[nPos-s_w+1] ;
						g3 = nGradMAG[nPos+s_w-1] ;
					} 
				}
				else
				{
					weight = fabs(gy)/fabs(gx); 
					
					g2 = nGradMAG[nPos+1] ; 
					g4 = nGradMAG[nPos-1] ;
					if (gx*gy > 0) 
					{				
						g1 = nGradMAG[nPos+s_w+1] ;
						g3 = nGradMAG[nPos-s_w-1] ;
					} 
					else 
					{ 
						g1 = nGradMAG[nPos-s_w+1] ;
						g3 = nGradMAG[nPos+s_w-1] ;
					}
				}
					dTmp1 = weight*g1 + (1-weight)*g2 ;
					dTmp2 = weight*g3 + (1-weight)*g4 ;
					if(dTmp> dTmp1 && dTmp> dTmp2)
					{
						ImgRlt[nPos] = 128 ;
					}
					else
					{
						ImgRlt[nPos] = 0 ;
					}
			} //else
		} // for
	}
	for (i=0; i<s_w*s_h;i++) CannyMap[i] = ImgRlt[i];
//...END Non-Maximum
//...Hysteresis
// 	EstimateThreshold
	nMaxMag = 0;

	for(k=0; k<1024; k++) 
	{
		nHist[k] = 0; 
	}
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			if(CannyMap[y*s_w+x]==128)
			{
				nHist[ nGradMAG[y*s_w+x] ]++;
			}
		}
	}
	
	nEdgeNb = nHist[0]  ;
	nMaxMag = 0         ;
	for(k=1; k<1024; k++)
	{
		if(nHist[k] != 0)
		{
			nMaxMag = k;
		}
		nEdgeNb += nHist[k];
	}
	nHighCount = (int)(dRatioHigh * nEdgeNb +0.5);
	
	k = 1;
	nEdgeNb = nHist[1];
	
	while( (k<(nMaxMag-1)) && (nEdgeNb < nHighCount) )
	{
		k++;
		nEdgeNb += nHist[k];
	}
	nThdHigh = k ;
	
	nThdLow  = (int)((nThdHigh) * dRatioLow+ 0.5);	
// 	End EstimateThreshold
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			nPos = y*s_w + x ; 
			
			if((CannyMap[nPos] == 128) && (nGradMAG[nPos] >= nThdHigh))
			{
				CannyMap[nPos] = 255;
				I_TraceEdge(y, x, nThdLow, CannyMap, nGradMAG, s_w);
			}
		}
	}
	
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			nPos = y*s_w + x ; 
			if(CannyMap[nPos] != 255)
			{
				CannyMap[nPos] = 0 ;
			}
		}
	 }
//*** 消除canny map 細邊
//****DirectionMap
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			if (nGradMAG[y*s_w+x] != 0)
			{
				if (Ix[y*s_w + x] ==0 && Iy[y*s_w + x] !=0)  // y 方向
				{
					DirMap[y*s_w + x]   = 90;
				}
				else if (Ix[y*s_w + x] !=0 && Iy[y*s_w + x] ==0) // x 方向
				{
					DirMap[y*s_w + x]   = 0;
				}
				else if (Ix[y*s_w + x] > 0 && Iy[y*s_w + x] > 0) // 第一象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					DirMap[y*s_w + x]   = Theta; 
				}
				else if (Ix[y*s_w + x] < 0 && Iy[y*s_w + x] > 0) // 第二象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					Theta = 180 + Theta;
					DirMap[y*s_w + x]   = Theta; 
				}
				else if (Ix[y*s_w + x] < 0 && Iy[y*s_w + x] < 0) // 第三象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					Theta = -180 + Theta;
					DirMap[y*s_w + x]   = Theta; 
				}
				else // 第四象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					DirMap[y*s_w + x]   = Theta;    
				}
			}
		}
	}
//***CANNY MAP 輸出
// 	for (i=0; i<s_w*s_h; i++)
// 		out_Image[i] = CannyMap[i];
	//...END Hysteresis

//Canny End*********************************************************** 
//****計算角點量
// 	for (i=0; i<s_h; i++)
// 	{
// 		for (j=0; j<s_w; j++)
// 		{
// 			cim[i*s_w+j] =  (Ix2[i*s_w+j]*Iy2[i*s_w+j]*It2[i*s_w+j] +
// 				            Ixy[i*s_w+j]*Iyt[i*s_w+j]*Ixt[i*s_w+j] +
// 							Ixt[i*s_w+j]*Ixy[i*s_w+j]*Iyt[i*s_w+j] - 
// 							Iy2[i*s_w+j]*Ixt[i*s_w+j]*Ixt[i*s_w+j] -
// 							Ix2[i*s_w+j]*Iyt[i*s_w+j]*Iyt[i*s_w+j] -
// 							It2[i*s_w+j]*Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+It2[i*s_w+j]+0.000001);
// 		}
// 	}
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}
//**** 進行局部非極大值抑制獲得最終角點

	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt;
	int Circle_Size = 10;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold ) //大於臨界值
				{
// 					out_Image[i*s_w+j]=255;   //滿足上二條件才是角點
// 					for (k=1; k<5; k++) //標籤大小 畫 +
// 					{
// 						out_Image[(i)*s_w+(j-k)]=-255;
// 						out_Image[(i)*s_w+(j+k)]=-255;
// 						out_Image[(i-k)*s_w+(j)]=-255;
// 						out_Image[(i+k)*s_w+(j)]=-255;
// 					}
					//*****畫圓
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + Circle_Size * cos(d*PI/180);
						y = i + Circle_Size * sin(d*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}
//......test start
					//*****畫圓心
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + 2 * cos(d*PI/180);
						y = i + 2 * sin(d*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}
// 					//圓範圍內顯示gradient magnitude
					for ( d=0; d<360; d++) // 0-360 degree
					{
						for (c=0; c<Circle_Size; c++)
						{
							x = j + c * cos(d*PI/180);
							y = i + c * sin(d*PI/180);
							out_Image[ y * s_w + x ] = nGradMAG[y * s_w + x];
						}
					}
					//取interest point 週遭pixel來決定移動方向
					ix2 = iy2 = ixy = ixt = iyt = 0;
// 					for (m=-1*LocalDis; m<=LocalDis; m++)
// 					{
// 						for (n=-1*LocalDis; n<=LocalDis; n++)
// 						{
// 							ix2 += Ix2[(i+m)*s_w+(j+n)];
// 							iy2 += Iy2[(i+m)*s_w+(j+n)];
// 							ixy += Ixy[(i+m)*s_w+(j+n)];
// 							ixt += Ixt[(i+m)*s_w+(j+n)];
// 							iyt += Iyt[(i+m)*s_w+(j+n)];
// 						}
// 					}
					//只取單點來決定移動方向
					ix2 = Ix2[i*s_w+j];
					iy2 = Iy2[i*s_w+j];
					ixy = Ixy[i*s_w+j];
					ixt = Ixt[i*s_w+j];
					iyt = Iyt[i*s_w+j];
					
					// Optical Flow
					w = ix2*iy2-2*ixy;
					u = (-1*ixt*iy2+iyt*ixy)/w;
					v = (-1*ix2*iyt+ixt*ixy)/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf((double)v/(double)u) * 57.3;
					else // 第四象限
						Theta = atanf((double)v/(double)u) * 57.3;
					for ( d=0; d<Circle_Size; d++) //畫線
					{
						x = j + d * cos(Theta*PI/180)+1;
						y = i + d * sin(Theta*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}

					//圓範圍內邊Direction
// 					for (c=0; c<Circle_Size; c++)
// 					{
// 						x = j + c * cos(DirMap[i*s_w+j]*PI/180)+1;
// 						y = i + c * sin(DirMap[i*s_w+j]*PI/180);
// 						out_Image[ y * s_w + x ] = -50;
// 					}
					//圓範圍內Canny邊px與區域圓心角度統計
					//....
// 					int Hist[180];
// 					int Tbl[180];
// 					//***************************************************
// 					for (d=0; d<180; d++) Hist[d] = 0;
// 					for (d=0; d<360; d++)
// 					{
//  						for (c=0; c<Circle_Size; c++)
//  						{
//  							x = j + c * cos(d*PI/180);
//  							y = i + c * sin(d*PI/180);
//  							if ( CannyMap[y*s_w+x] == 255)
// 							{
// 								Theta = atanf((double)(y-i)/(double)(x-j + 0.00001)) * 57.3;
// 								if (Theta<0) Theta += 180;
// 								Hist[Theta]++;
// 							}
//  						}
// 					}
// 					Hist[0] = 0;
// 					Hist[90] = 0;
// 					for (d=0; d<180; d++) Tbl[d] = Hist[d];
// 					quicksort(Hist,0,179,180);
// 					for (d=0; d< 180; d++)
// 					{
// 						if( Tbl[d] == Hist[0]) 
// 						{ 
// 							Theta = d;
// 							d = 180;
// 						}
// 					}
// 					for ( d=0; d<Circle_Size; d++) //畫線
// 					{
// 						x = j + d * cos(Theta*PI/180)+1;
// 						y = i + d * sin(Theta*PI/180);
// 						out_Image[ y * s_w + x ] = -100;
// 					}
					//圓範圍內顯示gradient magnitude 以八方向做統計點數
					double Total =0;
					int Cnt = 0;
					for ( d=0; d<360; d++) // 0-360 degree
					{
						for (c=0; c<Circle_Size; c++)
						{
							x = j + c * cos(d*PI/180);
							y = i + c * sin(d*PI/180);
							Total += nGradMAG[y * s_w + x];
							Cnt++;
						}
					}
					Total = Total / Cnt;
					for ( d=0; d<360; d++) // 0-360 degree
					{
						for (c=0; c<Circle_Size; c++)
						{
							x = j + c * cos(d*PI/180);
							y = i + c * sin(d*PI/180);
							out_Image[ y * s_w + x] = nGradMAG[ i * s_w +j ];
							if( nGradMAG[y * s_w + x] > Total )
							{
								out_Image[ y * s_w + x ] = 255;
								Temp[ y * s_w + x] = 255;
							}
							else
							{
								out_Image[ y * s_w + x ] = 0;
								Temp[ y * s_w + x] = 0;
							}
						}
					}
					//***************************************************
					int Hist[8];
					int Tbl[9];
					int PxNo=0;
					//***************************************************
					for (d=0; d<8; d++) Hist[d] = 0;
					for (d=0; d<360; d++)
					{
 						for (c=Circle_Size-2; c<Circle_Size; c++)
 						{
 							x = j + c * cos(d*PI/180);
 							y = i + c * sin(d*PI/180);
 							if ( Temp[y*s_w+x] == 255)
							{
								Theta = d / 45 ;
								Hist[Theta]++;
								PxNo++;
							}
						}
					}
					for (c=0; c<8; c++) Tbl[c] = Hist[c];
					I_quicksort( Tbl, 0, 7, 8);

					
					for(c=0; c<8; c++)
						for ( d=0; d< Circle_Size*Hist[c]/Tbl[0]; d++) //畫線
						{
							x = j + d * cos(c*45*PI/180)+1;
							y = i + d * sin(c*45*PI/180);
							out_Image[ y * s_w + x ] = -200;
						}
 					//***************************************************
					//圓範圍內Canny邊edgelet
// 					for ( d=0; d<360; d++) // 0-360 degree
// 					{
// 						for (c=0; c<Circle_Size; c++)
// 						{
// 							x = j + c * cos(d*PI/180);
// 							y = i + c * sin(d*PI/180);
// 							if (CannyMap[y*s_w+x] == 255)
// 								out_Image[ y * s_w + x ] = -150;
// 						}
// 					}
//...........test end
				}
		}
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;

	delete []CannyMap;
	delete []ImgRlt;
	delete []nGradMAG;
	delete []DirMap;

	delete []T00S;
	delete []T01S;
	delete []T02S;
}



void CRogerFunction::I_HARRISDirectionEdge2(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma)
{
	int i, j, m, n, x, y, c, d;
	int LocalDis=5;
	int Theta;
	double Localmax;
	double WindowSize =  10;

	BYTE *Temp   = new BYTE[s_w*s_h];
	memset(Temp, 0, sizeof(BYTE)*s_w*s_h);

	BYTE *T00S   = new BYTE[s_w*s_h]; // t-1
	BYTE *T01S   = new BYTE[s_w*s_h]; // t-1
	BYTE *T02S   = new BYTE[s_w*s_h]; // t-1

	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	//********************************
	memset(I0, 0, sizeof(double)*s_w*s_h);
	memset(I1, 0, sizeof(double)*s_w*s_h);
	memset(I2, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);
	//********************************
	memset(It, 0, sizeof(double)*s_w*s_h);
	memset(It2, 0, sizeof(double)*s_w*s_h);
	memset(Ixt, 0, sizeof(double)*s_w*s_h);
	memset(Iyt, 0, sizeof(double)*s_w*s_h);
	//*********************************
	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];

	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}

//*******Gaussian******************************* 
	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);


	int *nGradMAG = new int[s_w*s_h];
	double *DirMap = new double[s_w*s_h];

	//梯度計算
	for (i=0; i<s_w*s_h; i++) nGradMAG[i] = (int)sqrt(Ix2[i]+Iy2[i]+0.5);
//****DirectionMap
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{
			if (nGradMAG[y*s_w+x] != 0)
			{
				if (Ix[y*s_w + x] ==0 && Iy[y*s_w + x] !=0)  // y 方向
				{
					DirMap[y*s_w + x]   = 90;
				}
				else if (Ix[y*s_w + x] !=0 && Iy[y*s_w + x] ==0) // x 方向
				{
					DirMap[y*s_w + x]   = 0;
				}
				else if (Ix[y*s_w + x] > 0 && Iy[y*s_w + x] > 0) // 第一象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					DirMap[y*s_w + x]   = Theta; 
				}
				else if (Ix[y*s_w + x] < 0 && Iy[y*s_w + x] > 0) // 第二象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					Theta = 180 + Theta;
					DirMap[y*s_w + x]   = Theta; 
				}
				else if (Ix[y*s_w + x] < 0 && Iy[y*s_w + x] < 0) // 第三象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					Theta = -180 + Theta;
					DirMap[y*s_w + x]   = Theta; 
				}
				else // 第四象限
				{
					Theta = atanf((double)Ix[y*s_w + x]/(double)Iy[y*s_w + x]) * 57.3;
					DirMap[y*s_w + x]   = Theta;    
				}
			}
		}
	}
//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}
//**** 進行局部非極大值抑制獲得最終角點

	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt;
	int Circle_Size = 10;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold ) //大於臨界值
				{
					//*****畫圓
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + Circle_Size * cos(d*PI/180);
						y = i + Circle_Size * sin(d*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}
					//*****畫圓心
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + 2 * cos(d*PI/180);
						y = i + 2 * sin(d*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}
					//圓範圍內顯示gradient magnitude
					for ( d=0; d<360; d++) // 0-360 degree
					{
						for (c=0; c<Circle_Size; c++)
						{
							x = j + c * cos(d*PI/180);
							y = i + c * sin(d*PI/180);
							out_Image[ y * s_w + x ] = nGradMAG[y * s_w + x];
						}
					}
					//取interest point 週遭pixel來決定移動方向
						//只取單點來決定移動方向
					ix2 = Ix2[i*s_w+j];
					iy2 = Iy2[i*s_w+j];
					ixy = Ixy[i*s_w+j];
					ixt = Ixt[i*s_w+j];
					iyt = Iyt[i*s_w+j];
					
					// Optical Flow
					w = ix2*iy2-2*ixy;
					u = (-1*ixt*iy2+iyt*ixy)/w;
					v = (-1*ix2*iyt+ixt*ixy)/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf((double)v/(double)u) * 57.3;
					else // 第四象限
						Theta = atanf((double)v/(double)u) * 57.3;
					for ( d=0; d<Circle_Size; d++) //畫線
					{
						x = j + d * cos(Theta*PI/180)+1;
						y = i + d * sin(Theta*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}
					//圓範圍內顯示gradient 以八方向做統計點數 (4 x 4) x 4

					int Hist[8];
					int Tbl[9];
					int PxNo=0;
					int MaxValue;
					int MaxIdx;
					for (d=0; d<8; d++) Hist[d] = 0;
					//***************************************************
					int Cnt = 0;
					for (y=-9; y<=9; y++)
					{
						for (x=-9; x<=9; x++)
						{
							if (DirMap[(i+y)*s_w+(j+x)] < 0 )
								Theta = DirMap[(i+y)*s_w+(j+x)] + 360;
							else
								Theta = DirMap[(i+y)*s_w+(j+x)];
							Theta = Theta / 45;

							Hist[Theta]++;
						}
					}
// 					Hist[0] = 0;
					for (c=0; c<8; c++) Tbl[c] = Hist[c];
					I_quicksort( Hist, 0, 7, 8);
					MaxValue = Hist[0];
					//****找出最大值Index Value ****
					for (c=0; c<8; c++)
					{
						if( Tbl[c] == MaxValue)
						{
							MaxIdx = c;
							c=8;
						}
					}
					//****將角度予以正規化
					for (c=MaxIdx; c<8; c++)
					{
						Hist[c-MaxIdx] = Tbl[c];
					}
					for (c=0; c<MaxIdx; c++)
					{
						Hist[8-MaxIdx+c] = Tbl[c];
					}
					//********************
					for(c=0; c<8; c++)
						for ( d=0; d< Circle_Size*Hist[c]/MaxValue; d++) //畫線
						{
							x = j + d * cos(c*45*PI/180)+1;
							y = i + d * sin(c*45*PI/180);
							out_Image[ y * s_w + x ] = -200;
						}
				}
		}
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;


	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;

	delete []nGradMAG;
	delete []DirMap;

	delete []T00S;
	delete []T01S;
	delete []T02S;
}

void CRogerFunction::I_HARRISDirectionEdge3(BYTE *CannyMap, BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, int WindowSize)
{
	int i, j, m, n, x, y, c, d;
	int Theta;
	double Localmax;
	double Sigma = (WindowSize - 1) / 6 ; 
	double *I0   = new double[s_w*s_h]; // t-1
	double *I1   = new double[s_w*s_h]; // t .. Now
	double *I2   = new double[s_w*s_h]; // t+1
	//*******************************
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	//********************************
	double *It  = new double[s_w*s_h];
	double *It2 = new double[s_w*s_h];
	double *Ixt  = new double[s_w*s_h];
	double *Iyt  = new double[s_w*s_h];
	for (i=0; i<s_h*s_w; i++)	I0[i] = (double)T00[i];
	for (i=0; i<s_h*s_w; i++)	I1[i] = (double)T01[i];
	for (i=0; i<s_h*s_w; i++)	I2[i] = (double)T02[i];
	I_DerivativeX(I1, Ix, s_w, s_h);
	I_DerivativeY(I1, Iy, s_w, s_h);
	I_DerivativeT(I0, I1, I2, It, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			It2[i*s_w+j] = It[i*s_w+j]*It[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];
			Ixt[i*s_w+j] = Ix[i*s_w+j]*It[i*s_w+j];
			Iyt[i*s_w+j] = Iy[i*s_w+j]*It[i*s_w+j];
		}
	}
//*******Gaussian******************************* 
	I_GaussianSmoothDBL_STIP(Ix2, Ix2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iy2, Iy2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixy, Ixy, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(It2, It2, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Ixt, Ixt, s_w, s_h, Sigma, 3);
	I_GaussianSmoothDBL_STIP(Iyt, Iyt, s_w, s_h, Sigma, 3);

//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}
//**** 進行局部非極大值抑制獲得最終角點
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定後, 再(區域)計算方向
	double u, v, w;
	double ix2, iy2, ixy, ixt, iyt;
	int Circle_Size = WindowSize;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold  && abs(It[i*s_w+j]) > 20) //大於臨界值
				{
					//圓範圍內Canny邊edgelet
					for ( d=0; d<360; d++) // 0-360 degree
					{
						for (c=0; c<Circle_Size; c++)
						{
							x = j + c * cos(d*PI/180);
							y = i + c * sin(d*PI/180);
// 							if (CannyMap[y*s_w+x] == 255)
// 								out_Image[ y * s_w + x ] = -150;
							out_Image[y*s_w+x] = CannyMap[y*s_w+x];
						}
					}
					//*****畫圓心
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + 2 * cos(d*PI/180);
						y = i + 2 * sin(d*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}
					//*****畫圓
					for ( d=0; d<360; d++) // 0-360 degree
					{
						x = j + Circle_Size * cos(d*PI/180);
						y = i + Circle_Size * sin(d*PI/180);
						out_Image[ y * s_w + x ] = -200;
					}
					//取interest point 週遭pixel來決定方向
					ix2 = iy2 = ixy = ixt = iyt = 0;
					for ( d=0; d<360; d++) // 0-360 degree
					{
						for (c=0; c<Circle_Size; c++)
						{
							x = j + c * cos(d*PI/180);
							y = i + c * sin(d*PI/180);
							ix2 += Ix2[ y * s_w + x ];
							iy2 += Iy2[ y * s_w + x ];
							ixy += Ixy[ y * s_w + x ];
							ixt += Ixt[ y * s_w + x ];
							iyt += Iyt[ y * s_w + x ];
						}
					}
					// Optical Flow
					w = ix2*iy2-2*ixy;
					u = (-1*ixt*iy2+iyt*ixy)/w;
					v = (-1*ix2*iyt+ixt*ixy)/w;
					if (u ==0 && v !=0)  // y 方向
						Theta = 90;
					else if (u !=0 && v ==0) // x 方向
						Theta = 0;
					else if (u > 0 && v > 0) // 第一象限
						Theta = atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v > 0) // 第二象限
						Theta = 180 + atanf((double)v/(double)u) * 57.3;
					else if (u < 0 && v < 0) // 第三象限
						Theta = -180 + atanf((double)v/(double)u) * 57.3;
					else // 第四象限
						Theta = atanf((double)v/(double)u) * 57.3;
					gpMsgbar->ShowMessage("%f\n", It[i*s_w+j]);
					for ( d=0; d<Circle_Size*abs(It[i*s_w+j])/255; d++) //畫線
					{
						x = j + d * cos(Theta*PI/180)+1;
						y = i + d * sin(Theta*PI/180);
						out_Image[ y * s_w + x ] = -100;
					}
					//***************************************************

				}
		}
	}

	delete []I0;
	delete []I1;
	delete []I2;
	delete []Ix;
	delete []Ix2;

	delete []Iy;
	delete []Iy2;
	delete []Ixy;
	delete []cim;
	delete []mx;

	delete []It;
	delete []It2;
	delete []Ixt;
	delete []Iyt;
}

void CRogerFunction::I_Correlation(double *Hist, double *Hist1, int Hist_Size, double &Score)
{
	int i;
	double an =0;
	double bn =0;
	double cn =0;
	double Ave  = 0;
	double Ave1 = 0;
	for (i=0; i<Hist_Size; i++)
	{
		Ave  += Hist[i];
		Ave1 += Hist1[i];
	}
	Ave  /= Hist_Size;
	Ave1 /= Hist_Size;
	
	for (i=0; i<Hist_Size; i++)
	{
		an += ((Hist[i]-Ave)*(Hist1[i]-Ave1));
		bn += powf((Hist[i]-Ave)  ,2);
		cn += powf((Hist1[i]-Ave1),2);
	}
	Score = an / (sqrtf( bn * cn) + 0.00000000001) ;	
}

void CRogerFunction::I_Correlation(int *Hist, int *Hist1, int Hist_Size, double &Score)
{
	int i;
	double an =0;
	double bn =0;
	double cn =0;
	double Ave  = 0;
	double Ave1 = 0;
	for (i=0; i<Hist_Size; i++)
	{
		Ave  += Hist[i];
		Ave1 += Hist1[i];
	}
	Ave  /= Hist_Size;
	Ave1 /= Hist_Size;
	
	for (i=0; i<Hist_Size; i++)
	{
		an += ((Hist[i]-Ave)*(Hist1[i]-Ave1));
		bn += powf((Hist[i]-Ave)  ,2);
		cn += powf((Hist1[i]-Ave1),2);
	}
	Score = an / (sqrtf( bn * cn) + 0.00000000001) ;	
}
void CRogerFunction::I_HarrisCorner_V(BYTE *pImage, vector<CPoint> *vi, int s_w, int s_h, int Threshold, double Sigma)
{
	int x1, y1, Idx; 
	CPoint Corner_Px;
	double *I   = new double[s_w*s_h];
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	int i, j, m, n, d ,x, y;
	memset(I, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);

	//微分
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			I[i*s_w+j] = (double)pImage[i*s_w+j];
		}
	}
	for (i=0; i<s_w*s_h; i++) pImage[i] = 0;

	I_DerivativeX(I, Ix, s_w, s_h);
	I_DerivativeY(I, Iy, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];

		}
	}
//*******Gaussian******************************* 
	I_GaussianSmoothDBL(Ix2, Ix2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Iy2, Iy2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Ixy, Ixy, s_w, s_h, Sigma);
//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}

//**** 進行局部非極大值抑制獲得最終角點
	double Localmax;
	double WindowSize =  1 + 2 * ceil(3 * Sigma);
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定
	double Circle_Size = WindowSize;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold) //大於臨界值
				{
					x1 = int(j / (s_w / 5));
					y1 = int((s_h-i) / (s_h / 5));
					Idx = x1*5+y1; //判斷模板位置, 以減少判斷次數
					
					if( Idx==6 || Idx==7 ||Idx==8 ||Idx==11 ||Idx==12 ||Idx==13 ||Idx==16 ||Idx==17 ||Idx==18 /*||Idx==20*/ ||Idx==21 ||Idx==22 ||Idx==23 ) //大於臨界值
					{
						Corner_Px.x = j;
						Corner_Px.y = s_h-i;
						vi->push_back(Corner_Px);
						for ( d=0; d<360; d++) // 0-360 degree //畫圓
						{
							x = j + Circle_Size * cos(d*PI/180);
							y = i + Circle_Size * sin(d*PI/180);
							pImage[ y * s_w + x ] = 255;
						}
						for ( d=-2; d<= 2 ; d++) // 0-360 degree //畫線
						{
							x = j + d ;
							y = i;
							pImage[ y * s_w + x ] = 255;
							x = j  ;
							y = i + d;
							pImage[ y * s_w + x ] = 255;
						}
					}
				}
		}
	}
	delete []I   ;
	delete []Ix  ;
	delete []Ix2 ;
	delete []Iy  ;
	delete []Iy2 ;
	delete []Ixy ;
	delete []cim ;
	delete []mx  ;
}
void CRogerFunction::I_HarrisCorner_V(BYTE *pImage, vector<CPoint> *vi, CPoint RestrictLB, CPoint RestrictRU, int s_w, int s_h, int Threshold, double Sigma)
{
	CPoint Corner_Px;
	double *I   = new double[s_w*s_h];
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	int i, j, m, n;
	memset(I, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);

	//微分
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			I[i*s_w+j] = (double)pImage[i*s_w+j];
		}
	}
	for (i=0; i<s_w*s_h; i++) pImage[i] = 0;

	I_DerivativeX(I, Ix, s_w, s_h);
	I_DerivativeY(I, Iy, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];

		}
	}
//*******Gaussian******************************* 
	I_GaussianSmoothDBL(Ix2, Ix2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Iy2, Iy2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Ixy, Ixy, s_w, s_h, Sigma);
//****計算角點量
	for (i=RestrictLB.y+5; i<RestrictRU.y-5; i++)
	{
		for (j=RestrictLB.x+5; j<RestrictRU.x-5; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}

//**** 進行局部非極大值抑制獲得最終角點
	double Localmax;
	double WindowSize =  1 + 2 * ceil(3 * Sigma);
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定
	double Circle_Size = WindowSize;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
				if(mx[i*s_w+j] > Threshold && i > RestrictLB.y && i < RestrictRU.y && j > RestrictLB.x && j < RestrictRU.x ) //大於臨界值
				{
					Corner_Px.x = j;
					Corner_Px.y = i;
					vi->push_back(Corner_Px);
				}
		}
	}
	delete []I   ;
	delete []Ix  ;
	delete []Ix2 ;
	delete []Iy  ;
	delete []Iy2 ;
	delete []Ixy ;
	delete []cim ;
	delete []mx  ;
}
void CRogerFunction::I_HarrisCorner_Eigen(BYTE *pImage, int s_w, int s_h, int Threshold, double Sigma, int Type)
{
	double *I   = new double[s_w*s_h];
	double *Ix  = new double[s_w*s_h];
	double *Ix2 = new double[s_w*s_h];
	double *Iy  = new double[s_w*s_h];
	double *Iy2 = new double[s_w*s_h];
	double *Ixy = new double[s_w*s_h];
	double *cim = new double[s_w*s_h];
	double *mx  = new double[s_w*s_h];
	int i, j, m, n, d ,x, y;
	memset(I, 0, sizeof(double)*s_w*s_h);
	memset(Ix, 0, sizeof(double)*s_w*s_h);
	memset(Ix2, 0, sizeof(double)*s_w*s_h);
	memset(Iy, 0, sizeof(double)*s_w*s_h);
	memset(Iy2, 0, sizeof(double)*s_w*s_h);
	memset(Ixy, 0, sizeof(double)*s_w*s_h);
	memset(cim, 0, sizeof(double)*s_w*s_h);
	memset(mx, 0, sizeof(double)*s_w*s_h);

	//微分
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			I[i*s_w+j] = (double)pImage[i*s_w+j];
		}
	}
	for (i=0; i<s_w*s_h; i++) pImage[i] = 0;

	I_DerivativeX(I, Ix, s_w, s_h);
	I_DerivativeY(I, Iy, s_w, s_h);
	for (i=1; i<s_h-1; i++)
	{
		for (j=1; j<s_w-1; j++)
		{
			Ix2[i*s_w+j] = Ix[i*s_w+j]*Ix[i*s_w+j];
			Iy2[i*s_w+j] = Iy[i*s_w+j]*Iy[i*s_w+j];
			Ixy[i*s_w+j] = Ix[i*s_w+j]*Iy[i*s_w+j];

		}
	}
//*******Gaussian******************************* 
	I_GaussianSmoothDBL(Ix2, Ix2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Iy2, Iy2, s_w, s_h, Sigma);
	I_GaussianSmoothDBL(Ixy, Ixy, s_w, s_h, Sigma);
//****計算角點量
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			cim[i*s_w+j] = (Ix2[i*s_w+j]*Iy2[i*s_w+j] - Ixy[i*s_w+j]*Ixy[i*s_w+j])/(Ix2[i*s_w+j]+Iy2[i*s_w+j]+0.000001);//加0.000001防止除0
		}
	}

//**** 進行局部非極大值抑制獲得最終角點
	double Localmax;
	double WindowSize =  1 + 2 * ceil(3 * Sigma);
	for(i = 0; i < s_h; i++)
	{
		for(j = 0; j < s_w; j++)
		{
			Localmax=-1000000;
			if(i>int(WindowSize/2) && i<s_h-int(WindowSize/2) && j>int(WindowSize/2) && j<s_w-int(WindowSize/2))
				for(m=0; m<WindowSize; m++)
				{
					for(n=0; n<WindowSize; n++)
					{						
						if(cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))] > Localmax)
							Localmax=cim[(i+m-int(WindowSize/2))*s_w+(j+n-int(WindowSize/2))];
					}
				}
			if(Localmax > 0)
				mx[i*s_w+j]=Localmax;
			else
				mx[i*s_w+j]=0;
		}
	}
//*****角點決定
	double AA[2][2];
	double EigenV1, EigenV2;
	double Circle_Size = WindowSize;
	for(i = Circle_Size; i < s_h-Circle_Size; i++)
	{
		for(j = Circle_Size ; j < s_w-Circle_Size; j++)
		{	
			if(cim[i*s_w+j] == mx[i*s_w+j])  //獲得局部極大值
			{
				/////多此一舉/////////////////////////////////////////////////////////////////////
				AA[0][0] = Ix2[i*s_w+j];
				AA[0][1] = Ixy[i*s_w+j];
				AA[1][0] = Ixy[i*s_w+j];
				AA[1][1] = Iy2[i*s_w+j];
				I_EigenValue_2x2( AA, EigenV1, EigenV2);
				//////////////////////////////////////////////////////////////////////////
				if ( EigenV1 > 40000 && EigenV2 > 10000)
				{
					gpMsgbar->ShowMessage("(%d, %d)..EV1: %.3f..EV2: %.3f\n", j, s_h-i, EigenV1, EigenV2);
					if ( Type == 0 )
					{
						for ( d=0; d<360; d++) // 0-360 degree //畫圓
						{
							x = j + Circle_Size * cos(d*PI/180);
							y = i + Circle_Size * sin(d*PI/180);
							pImage[ y * s_w + x ] = 255;
						}
					}
					if ( Type == 1 )
					{
						for ( d=-2; d<= 2 ; d++) // 0-360 degree //畫線
						{
							x = j + d ;
							y = i;
							pImage[ y * s_w + x ] = 255;
							x = j  ;
							y = i + d;
							pImage[ y * s_w + x ] = 255;
						}
					}
				}

			}
		}
	}
	delete []I   ;
	delete []Ix  ;
	delete []Ix2 ;
	delete []Iy  ;
	delete []Iy2 ;
	delete []Ixy ;
	delete []cim ;
	delete []mx  ;
}




///KMEAN START//////////////////////////////////////////////////////////////////////////
void CRogerFunction::KMEANS(  DataType *DataMember, int DataNum = 50, int Dimension = 2, int K = 3)
{
// 	int DataNum = 50;               //聚類樣本數目
// 	int Dimension = 2;             //樣本維數
// 	int K = 3;                     //分類數
	int HALT=0;
	int Row=3;
	ClusterType *ClusterMember;
	ClusterType *OldCluster;
	int i, j, N, t;
	int pa,pb,fa;
	ClusterMember = new ClusterType[K];
	OldCluster    = new ClusterType[K];

	for(i=0;i<K;i++)
		ClusterMember[i].center=new double[Dimension];

	for(i=0;i<K;i++)
	{
		GetValue(ClusterMember[i].center,DataMember[i].data,Dimension);
		ClusterMember[i].sonnum=1;
		
		OldCluster[i].center=new double[Dimension];
		GetValue(OldCluster[i].center,ClusterMember[i].center,Dimension);
	}
	
	
	for(i=0;i<DataNum;i++)  
	{
		for(j=0;j<K;j++)
		{
			DataMember[i].uncle[j]=SquareDistance(DataMember[i].data,ClusterMember[j].center,Dimension);
		}
		pa=DataMember[i].father=FindFather(DataMember[i].uncle,K);
		if(i>=K)
		{
			ClusterMember[pa].sonnum+=1;
			NewCenterPlus(ClusterMember,pa,DataMember[i].data,Dimension);
			GetValue(OldCluster[pa].center,ClusterMember[pa].center,Dimension);
		} 
	}
	
	//開始聚類，直到聚類中心不再發生變化。××逐個修改法××
	while(!HALT)
	{
		//一次聚類循環︰1.重新歸類；2.修改類中心
		for(i=0;i<DataNum;i++)  
		{
			for(j=0;j<K;j++)
			{
				DataMember[i].uncle[j]=SquareDistance(DataMember[i].data,ClusterMember[j].center,Dimension);
			}
			
			fa=DataMember[i].father;
			
			if(fa!=FindFather(DataMember[i].uncle,K)&&ClusterMember[fa].sonnum>1)
			{
				
				pa=DataMember[i].father;
				ClusterMember[pa].sonnum-=1;
				
				pb=DataMember[i].father=FindFather(DataMember[i].uncle,K);
				ClusterMember[pb].sonnum+=1;
				
				NewCenterReduce(ClusterMember,pa,DataMember[i].data,Dimension);
				NewCenterPlus(ClusterMember,pb,DataMember[i].data,Dimension);
			}					
			
		}//endfor 

		//判斷聚類是否完成，HALT=1,停止聚類
		HALT=0;
		for(j=0;j<K;j++)
			if(I_Compare(OldCluster[j].center,ClusterMember[j].center,Dimension))
				break;
			if(j==K)
				HALT=1;
			for(j=0;j<K;j++)
				GetValue(OldCluster[j].center,ClusterMember[j].center,Dimension);
			
	}//endwhile
	// set Type...
	for(i=0;i<K;i++)
	{
		N=0;
		for(j=0;j<DataNum;j++)
			if(DataMember[j].father==i)
			{
				DataMember[j].Type = i;
				N=N+1;
			}
	}//end for
	///display
	for(i=0;i<K;i++)
	{
		N=0;
		gpMsgbar->ShowMessage("\n******************** 第 %d 類樣本:*******************\n", i);
		for(j=0;j<DataNum;j++)
			if(DataMember[j].father==i)
			{
				gpMsgbar->ShowMessage("%d:(%d, %d):",DataMember[j].Type, DataMember[j].x, DataMember[j].y);
				gpMsgbar->ShowMessage("[");
				for(t=0;t<Dimension;t++)
					gpMsgbar->ShowMessage("  %3.1f", DataMember[j].data[t]);
				gpMsgbar->ShowMessage("]\n");				
				if((N+1)%Row==0)
				gpMsgbar->ShowMessage("\n");
				N=N+1;
			}
			gpMsgbar->ShowMessage("\n");
	}//end for
	delete []ClusterMember;
	delete []OldCluster;
}
//幾個經常需要調用的小函數
void CRogerFunction::NewCenterPlus(ClusterType *p1,int t,double *p2,int dim)
{
	int i;
	for(i=0;i<dim;i++)
		p1[t].center[i]=p1[t].center[i]+(p2[i]-p1[t].center[i])/(p1[t].sonnum);
}
void CRogerFunction::NewCenterReduce(ClusterType *p1,int t,double *p2,int dim)
{
	int i;
	for(i=0;i<dim;i++)
		p1[t].center[i]=p1[t].center[i]+(p1[t].center[i]-p2[i])/(p1[t].sonnum);
}
void CRogerFunction::GetValue(double *str1,double *str2,int dim)
{
	int i;
	for(i=0;i<dim;i++)		
		str1[i]=str2[i];
}
int  CRogerFunction::FindFather(double *p,int k)
{
	int i,N=0;
	double min=30000;

	for(i=0;i<k;i++)	
		if(p[i]<min)
		{
			min=p[i];
			N=i;
		}
	return N;
}
double CRogerFunction::SquareDistance(double *str1,double *str2,int dim)
{
	double dis=0;
	int i;
	for(i=0;i<dim;i++)
		dis=dis+(double)(str1[i]-str2[i])*(str1[i]-str2[i]);
	return dis;
}
int	CRogerFunction::I_Compare(double *p1,double *p2,int dim)
{
	int i;
	for(i=0;i<dim;i++)
		if(p1[i]!=p2[i])
			return 1;
	return 0;
}	
///KMEAN END//////////////////////////////////////////////////////////////////////////
void CRogerFunction::I_ResampleRAW(double *InputBuf, int Width, int Height, double *OutputBuf, int ReWidth, int ReHeight)
{

	int Ratio = Width / ReWidth;
	int i, j;
	memset( OutputBuf, 0, sizeof(double)*ReWidth*ReHeight);
	for (i=0; i<Height; i++)
	{
		for (j=0; j<Width; j++)
		{
			OutputBuf[(i/Ratio)*ReWidth+(j/Ratio)] = OutputBuf[(i/Ratio)*ReWidth+(j/Ratio)] + InputBuf[i*Width+j]/(Ratio*Ratio);
		}
	}



}
void CRogerFunction::I_DrawCircle(BYTE *pImage, int Image_W, int Image_H, int Image_EFFW, CPoint CrossPoint, int CrossSize, COLORREF LineColor)
{
	int d, x, y;
	int i ;
	
	BYTE	Color_R = GetRValue(LineColor),
		Color_G = GetGValue(LineColor),
		Color_B = GetBValue(LineColor);
	
	for (i=CrossSize; i<CrossSize+3; i++)
	{
		for ( d=0; d<360; d++) // 0-360 degree
		{
			x = CrossPoint.x + i * cos(d*0.017453292);
			y = CrossPoint.y + i * sin(d*0.017453292);
			if( x > Image_W )	x = Image_W;
			if( x < 0 )			x = 0;
			if( y > Image_H )	y = Image_H;
			if( y < 0 )			y = 0;
			pImage[y*Image_EFFW+3*x]   = Color_B;
			pImage[y*Image_EFFW+3*x+1] = Color_G;
			pImage[y*Image_EFFW+3*x+2] = Color_R;
		}
	}
	
}
void CRogerFunction::I_DrawCircleEffw(BYTE *pImage, int Image_W, int Image_H, int Image_Effw, CPoint CrossPoint, int CrossSize, COLORREF LineColor)
{
	int d, x, y;
	int i, j, k;
	
	BYTE	Color_R = GetRValue(LineColor),
		Color_G = GetGValue(LineColor),
		Color_B = GetBValue(LineColor);
	for (i=0; i<Image_H; i++)
	{
		for (j=0, k=0; j<Image_W; j++, k+=3)
		{
			
			for ( d=0; d<360; d+=2) // 0-360 degree
			{
				x = CrossPoint.x + CrossSize * cos(d*0.017453292);
				y = CrossPoint.y + CrossSize * sin(d*0.017453292);
				pImage[y*Image_Effw+3*x]   = Color_B;
				pImage[y*Image_Effw+3*x+1] = Color_G;
				pImage[y*Image_Effw+3*x+2] = Color_R;
			}
		}
	}
	
}
void CRogerFunction::I_DrawCircleNew(BYTE *pImage, int Image_W, int Image_H, int Image_Effw, CARINFO &CarDetectedA, CPoint CrossPoint, int CrossSize, COLORREF LineColor)
{
	int d, x, y;
	int i, j, k;
	
	BYTE	Color_R = GetRValue(LineColor),
			Color_G = GetGValue(LineColor),
			Color_B = GetBValue(LineColor);

	for (i=0; i<Image_H; i++)
	{
		for (j=0, k=0; j<Image_W; j++, k+=3)
		{
			
			for ( d=0; d<360; d++) // 0-360 degree
			{
				x = CrossPoint.x + CrossSize * cos(d*0.017453292);
				y = CrossPoint.y + CrossSize * sin(d*0.017453292);
				pImage[y*Image_Effw+3*x]   = Color_B;
				pImage[y*Image_Effw+3*x+1] = Color_G;
				pImage[y*Image_Effw+3*x+2] = Color_R;
			}
		}
	}

	int nNewRadius = (CarDetectedA.RU1.x-CarDetectedA.LB1.x)/2;
	for (i=0; i<Image_H; i++)
	{
		for (j=0, k=0; j<Image_W; j++, k+=3)
		{
			
			for ( d=0; d<180; d++) // 0-360 degree
			{
				x = CrossPoint.x + nNewRadius * cos(d*0.017453292);
				y = (CrossPoint.y-CrossSize) + nNewRadius * sin(d*0.017453292);
				pImage[y*Image_Effw+3*x]   = Color_B;
				pImage[y*Image_Effw+3*x+1] = Color_G;
				pImage[y*Image_Effw+3*x+2] = Color_R;
			}
		}
	}	
}
void CRogerFunction::I_DrawRange1(BYTE *Display_image, CPoint RU, CPoint RB, CPoint LU, CPoint LB, int s_w, int s_h, int s_bpp, COLORREF LineColor)
{
	CImgProcess FuncImg;
	BYTE	Color_R = GetRValue(LineColor),
		Color_G = GetGValue(LineColor),
		Color_B = GetBValue(LineColor);
	// 	if ( RU.x < 0   ) RU.x = 0;
	if ( RU.x > s_w ) RU.x = s_w-1;
	// 	if ( RU.y < 0   ) RU.y = 0;
	// 	if ( RU.y > s_h ) RU.y = s_h;
	// 	if ( RB.x < 0   ) RB.x = 0;
	if ( RB.x > s_w ) RB.x = s_w-1;
	if ( RB.y < 0   ) RB.y = 0;
	// 	if ( RB.y > s_h ) RB.y = s_h;
	if ( LU.x < 0   ) LU.x = 0;
	// 	if ( LU.x > s_w ) LU.x = s_w;
	// 	if ( LU.y < 0   ) LU.y = 0;
	// 	if ( LU.y > s_h ) LU.y = s_h;
	if ( LB.x < 0   ) LB.x = 0;
	// 	if ( LB.x > s_w ) LB.x = s_w;
	if ( LB.y < 0   ) LB.y = 0;
	// 	if ( LB.y > s_h ) LB.y = s_h;
	
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, LB, RB, RGB(Color_R,Color_G,Color_B));
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, LB, LU, RGB(Color_R,Color_G,Color_B));
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, RB, RU, RGB(Color_R,Color_G,Color_B));
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, LU, RU, RGB(Color_R,Color_G,Color_B));
}
void CRogerFunction::I_DrawRange2(BYTE *Display_image, CPoint P1, CPoint P2, int s_w, int s_h, int s_bpp, COLORREF LineColor)
{
	CImgProcess FuncImg;
	CPoint LB, LU, RB, RU;
	BYTE	Color_R = GetRValue(LineColor),
		Color_G = GetGValue(LineColor),
		Color_B = GetBValue(LineColor);
	if ( P1.y > P2.y)
	{
		LU = P1;
		RB = P2;
		RU.x = P2.x;
		RU.y = P1.y;
		LB.x = P1.x;
		LB.y = P2.y;
	}
	else
	{
		LB = P1;
		RU = P2;
		LU.x = P1.x;
		LU.y = P2.y;
		RB.x = P2.x;
		RB.y = P1.y;
	}
	
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, LB, RB, RGB(Color_R,Color_G,Color_B));
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, LB, LU, RGB(Color_R,Color_G,Color_B));
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, RB, RU, RGB(Color_R,Color_G,Color_B));
	FuncImg.DrawLine( Display_image, s_w, s_h, s_bpp, LU, RU, RGB(Color_R,Color_G,Color_B));
}
void CRogerFunction::I_RangePx( vector<OBJRANGE> &vVehicle, vector<SURFINFO> &vIT_TBL, BYTE *Display_image, int s_w, int s_h, int s_bpp, COLORREF LineColor)
{
	CImgProcess FuncImg;
	int i, j, k, m;
	CPoint P1, P2, P1A, P1B, P2A, P2B;
	BYTE	Color_R = GetRValue(LineColor),
			Color_G = GetGValue(LineColor),
			Color_B = GetBValue(LineColor);
	double  dGpTheta;
	int     nCount;
	//範圍圍內的vMatch_TBL點//////////////////////////////////////////////////////////////////////////
	double dYbound1, dYbound2;
	for (i=0; i<vVehicle.size(); i++)
	{
//  		gpMsgbar->ShowMessage("***************\n");
//  		gpMsgbar->ShowMessage("(T1:%d T2:%d)\n",vVehicle[i].nType, vVehicle[i].nType1);

		dGpTheta = 0;
		nCount   = 0;
		P1A.x = vVehicle[i].P1A.x - 15;
		P1A.y = vVehicle[i].P1A.y + 15;
		P1B.x = vVehicle[i].P1B.x - 15;
		P1B.y = vVehicle[i].P1B.y - 15;
		P2A.x = vVehicle[i].P2A.x + 15;
		P2A.y = vVehicle[i].P2A.y + 15;
		P2B.x = vVehicle[i].P2B.x + 15;
		P2B.y = vVehicle[i].P2B.y - 15;
		//紀錄群編號
// 		vVehicle[i].nType = i;
		//尋找區域內的vIT_TBL點
		for (j=0; j<vIT_TBL.size(); j++)
		{
			P1.x = vIT_TBL[j].x;
			P1.y = vIT_TBL[j].y;
			k = P1B.y;
			m = P1A.y;
			dYbound1 = P1A.y + 5;
			dYbound2 = P1B.y - 5;
			if (  P1.x >= min( P1A.x, P2A.x) && P1.x <= max( P1A.x, P2A.x) && P1.y <= dYbound1 && P1.y >= dYbound2 )
			{	
// 				gpMsgbar->ShowMessage("(%d)觀察:T1: %d, T2: %d(vIT: %d)\n", i, vVehicle[i].nType, vVehicle[i].nType1, vIT_TBL[j].nType );

				if ( fabs( vVehicle[i].dObjDirection - vIT_TBL[j].dTheta ) < 60 )
				{
					dGpTheta += vIT_TBL[j].dTheta;
					nCount++;
				}
				vIT_TBL[j].nGroupType = i;
				vVehicle[i].vGroupPx.push_back( vIT_TBL[j] );
			}
		}
		dGpTheta /= nCount;
		vVehicle[i].nNum     = nCount;
		vVehicle[i].dGpTheta = dGpTheta;
	}
	//排序


	// 	for (i=0; i<vIT_TBL.size(); i++)
	// 	{
	// 		P1.x = vIT_TBL[i].x;
	// 		P1.y = vIT_TBL[i].y;
	// 		if ( vIT_TBL[i].nGroupType != -1 )
	// 		{
	// 			FuncImg.DrawCross( Display_image, s_w, s_h, s_bpp, P1, 4, RGB( Color_R, Color_G, Color_B), 4);
	// 		}
	// 	}
}
void CRogerFunction::I_DrawPxLine( BYTE *in_image, CPoint SPx, double dTheta, int s_w, int s_h, int s_bpp, int nLineLength, COLORREF LineColorS, COLORREF LineColorE )
{
	CImgProcess FuncImg;
	CPoint      EPx;
	BYTE	Color_RS = GetRValue(LineColorS),
		Color_GS = GetGValue(LineColorS),
		Color_BS = GetBValue(LineColorS);
	BYTE	Color_RE = GetRValue(LineColorE),
		Color_GE = GetGValue(LineColorE),
		Color_BE = GetBValue(LineColorE);
	EPx.x = SPx.x - nLineLength * cos( dTheta *0.01745 );
	EPx.y = SPx.y - nLineLength * sin( dTheta *0.01745 );
	FuncImg.DrawLine( in_image, s_w, s_h, s_bpp, SPx, EPx, RGB(Color_RS,Color_GS,Color_BS), RGB(Color_RE,Color_GE,Color_BE));
	
}


void CRogerFunction::I_SavePatch( BYTE *in_image, int s_w, int s_h, CPoint LB, CPoint RU, char *FileName )
{
	CImgProcess FuncImg;
	
	int n = 0;
	int i, j, k, x, y;
	CPoint P1, P2;
		////產生儲存小圖////////////////////////////////////////////////////////
		int w = abs(RU.x-LB.x); 
		int h = abs(RU.y-LB.y);
		BYTE *pixels = new BYTE[w*h*3];
		CxImage *newImage = new CxImage(0);	
		for (i=0, y=LB.y; i<h; i++, y++)
		{
			for (j=0, k=0, x=LB.x; j<w; j++, x++, k+=3)
			{
				pixels[(i)*(w*3)+k]   = in_image[(y)*s_w*3+(x)*3];
				pixels[(i)*(w*3)+k+1] = in_image[(y)*s_w*3+(x)*3+1];
				pixels[(i)*(w*3)+k+2] = in_image[(y)*s_w*3+(x)*3+2];
			}
		}
	
		newImage->CreateFromArray(pixels, w, h, 24, (w*3), false);
		newImage->SetJpegQuality(100);
		newImage->SetJpegScale(100);

		newImage->Save( FileName, CXIMAGE_FORMAT_BMP);
		newImage->Destroy();
		delete newImage;
		delete [] pixels;
}


void CRogerFunction::I_HistogramSCR( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp)
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double *GramW = new double[s_w];
	double *GramH = new double[s_h];
	memset( GramW, 0, sizeof(double)*s_w);
	memset( GramH, 0, sizeof(double)*s_h);
	double *Tmp   = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w);

	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<s_h; i++)
	{
		for (j=0; j<s_w; j++)
		{
			GramW[j] = GramW[j] + In_ImageNpixel[i*s_w+j];
			GramH[i] = GramH[i] + In_ImageNpixel[i*s_w+j];
		}
	}
	//////////////////////////////////////////////////////
		//Smooth
	k = 3;
	for (i=k; i<s_w-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			Tmp[i] += GramW[i+j]/(k*1000);
		}
	} 
	memcpy( GramW, Tmp, sizeof(double)*s_w);

	double TOT=0;
// 	for (i=0; i<s_w; i++)
// 	{
// 		TOT += GramW[i];
// 	}
// 	for (i=0; i<s_w; i++)
// 	{
// 		GramW[i] = GramW[i] * 4000 /TOT ;
// 	}
	for (i=0; i<s_w; i++)
	{
		P1 = CPoint(i,0);
		P2 = CPoint( i, GramW[i]);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
	}
	TOT=0;
	for (i=0; i<s_h; i++)
	{
		TOT += GramH[i];
	}
	for (i=0; i<s_h; i++)
	{
		GramH[i] = GramH[i] * 4000 /TOT ;
	}
	for (i=0; i<s_h; i++)
	{
		P1 = CPoint(0,i);
		P2 = CPoint( GramH[i] , i);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
	}
	delete []GramH;
	delete []GramW;
}
void CRogerFunction::I_Histogram1D( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax)
{
	int i, j, k;
	CPoint P1, P2;
	int MaxV;
	CImgProcess FuncImg;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CenterP.size() ; i++)
	{
		GramW[CenterP[i].x]++;
	}
	for (i=0; i< s_w; i++)
	{
		P1 = CPoint( i,0);
		P2 = CPoint( i, GramW[i]*10);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 255,0,0));
	}
	//取區域極值
	for (i=0; i<s_w; i++)
	{
		gpMsgbar->ShowMessage("[%.0f]", GramW[i]);
	}
	gpMsgbar->ShowMessage("\n");
	k= 3;
	for (i=0; i<s_w-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramW[i+j] >= MaxV )
			{
				MaxV = GramW[i+j];
			}
			else
				GramW[i+j] = 0;
		}

	}
	for (i=0; i<s_w; i++)
	{
		gpMsgbar->ShowMessage("[%.0f]", GramW[i]);
	}
	gpMsgbar->ShowMessage("\n");
// 	for (i=0; i< s_w; i++)
// 	{
// 		if ( GramW[i] > 0 )
// 		{
// 			XPosMax.push_back(i);
// 
// 			P1 = CPoint( i,0);
// 			P2 = CPoint( i, s_h-1);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 0,0,255));
// 		}
// 	}
// 	for (i=0; i<s_w; i++)
// 	{
// 		P1 = CPoint(i,0);
// 		P2 = CPoint( i, GramW[i]*20);
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
// 	}
	delete []Tmp;
	delete []GramW;
}
void CRogerFunction::I_Histogram1DMax( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax)
{
	int i;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	int TOTAL=0;
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CenterP.size() ; i++)
	{
		GramW[CenterP[i].x]++;
	}
	//觀察
// 	for (i=1; i<s_w-1; i++)
// 	{
// 		P1.x = i+1;
// 		P2.x = i+1;
// 		P1.y = 0;
// 		P2.y = GramW[i]*10;
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255, 0,0));
// 	}
	for (i=2; i<s_w-2; i++)
	{
		GramW[i] = 0.1*GramW[i-2]+0.2*GramW[i-1]+0.4*GramW[i]+0.2*GramW[i+1]+0.1*GramW[i+2];
	}



	for (i=1; i<s_w-1; i++)
	{
		P1.x = i+1;
		P2.x = i+1;
		P1.y = 0;
		P2.y = 0+GramW[i]*50;
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0, 255,0));
	}
// 	k = 2;
// 	for (i=0; i<s_w-k; i++)
// 	{
// 		if (GramW[i] == 0 && GramW[i+1] != 0 && GramW[i+2]==0)
// 			GramW[i+1] = 0;
// 	} 
// 	for(i=0; i<s_h; i++) gpMsgbar->ShowMessage("[%.2f].", GramW[i]);
// 	gpMsgbar->ShowMessage("\n");

	memcpy( Tmp, GramW, sizeof(double)*s_w);
	I_BubbleSort( Tmp, s_w );

	for (i=0; i<s_w; i++)
	{
		if ( GramW[i] == Tmp[0] )
		{
			XPosMax.push_back( i );
			P1 = CPoint( i,0);
			P2 = CPoint( i, s_h-1);
			i = s_w;
		}
	}

	delete []Tmp;
	delete []GramW;
}
void CRogerFunction::I_Histogram1DMaxNEW( BYTE *Display_ImageNsize, SelfINFO &CenterLine , int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH)
{
	int i;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	int TOTAL=0;
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CenterLine.CenterPoint.size() ; i++)
	{
		GramW[CenterLine.CenterPoint[i].x]++;
	}
	for (i=2; i<s_w-2; i++)
	{
		GramW[i] = 0.1*GramW[i-2]+0.2*GramW[i-1]+0.4*GramW[i]+0.2*GramW[i+1]+0.1*GramW[i+2];
	}

	memcpy( Tmp, GramW, sizeof(double)*s_w);
	I_BubbleSort( Tmp, s_w );
	
	for (i=0; i<s_w; i++)
	{
		if ( GramW[i] == Tmp[0] )
		{
			CenterLine.CenterPosX = i;
			if( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( i,0);
				P2 = CPoint( i, s_h-1);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 255,255,0));
			}
			i = s_w;
		}
	}
	//消除偏離點
	int nDist;
// 	if ( DISPLAY_SWITCH == TRUE )
// 	{
// 		for (i=0; i<CenterLine.CenterPoint.size(); i++)
// 		{
// 			P1 = CPoint( CenterLine.CenterPoint[i].x, CenterLine.CenterPoint[i].y);
// 			FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, P1, 10, RGB(255,89,0), 2 );
// 		}
// 	}
	for ( i=0 ; i<CenterLine.CenterPoint.size(); i++ )
	{
		nDist = abs( CenterLine.CenterPoint[i].x - CenterLine.CenterPosX );
		if ( nDist > 8 ) 
		{
			CenterLine.CenterPoint.erase( CenterLine.CenterPoint.begin()+i );
			i = 0;
		}
	}
	int *SortI = new int[CenterLine.CenterPoint.size()];
	memset( SortI, 0, sizeof(int)*CenterLine.CenterPoint.size());
	for (i=0; i<CenterLine.CenterPoint.size(); i++)
	{
		SortI[i] = CenterLine.CenterPoint[i].y;
	}
	I_BubbleSort( SortI, CenterLine.CenterPoint.size() );
	CenterLine.nHUP = SortI[0];
	CenterLine.nHBT = SortI[CenterLine.CenterPoint.size()-1];
	delete []SortI;
// 	if( DISPLAY_SWITCH == TRUE )
// 	{
// 		for (i=0; i<CenterLine.CenterPoint.size(); i++)
// 		{
// 			P1 = CPoint( CenterLine.CenterPoint[i].x, CenterLine.CenterPoint[i].y);
// 			FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, P1, 10, RGB(255,255,255), 3 );
// 		}
// 	}
	delete []Tmp;
	delete []GramW;
}
void CRogerFunction::I_Histogram1DMax1( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax)
{
	int i;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CenterP.size() ; i++)
	{
		GramW[CenterP[i].x]++;
	}
	memcpy( Tmp, GramW, sizeof(double)*s_w);
	I_BubbleSort( Tmp, s_w );
	for (i=0; i< s_w; i++)
	{
		if ( GramW[i] >= Tmp[0] )
		{
			XPosMax.push_back(i);
// 			P1 = CPoint( i,0);
// 			P2 = CPoint( i, s_h-1);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 255,255,0));
			i = s_w;
		}
	}
	// 	for (i=0; i<s_w; i++)
	// 	{
	// 		P1 = CPoint(i,0);
	// 		P2 = CPoint( i, GramW[i]*20);
	// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
	// 		// 		gpMsgbar->ShowMessage("[%.0f]", GramW[i]);
	// 	}
	delete []Tmp;
	delete []GramW;
}


void CRogerFunction::I_Histogram1DPositon( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int CenterPosX , int s_w, int s_h, int s_bpp)
{
	int i, j;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double *GramH = new double[s_h];
	memset( GramH, 0, sizeof(double)*s_h);
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);

	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<s_h; i++)
	{
		for (j=CenterPosX; j<CenterPosX+30; j++)
		{
			GramH[i] = GramH[i] + In_ImageNpixel[i*s_w+j];
		}
	}
	double TOT=0;
	for (i=0; i<s_h; i++)
	{
		TOT += GramH[i];
	}
	for (i=0; i<s_h; i++)
	{
		GramH[i] = GramH[i] * 3000 /TOT ;
	}
	//找出區域最大直
	for (i=2; i<s_h-2; i++)
	{
		GramT[i] = (GramH[i-2]+GramH[i-1]+GramH[i]+GramH[i+1]+GramH[i+2])/5;
	}
	
	for (i=0; i<s_h; i++)
	{
// 		if ( GramH[i] > 50 )
// 		{
// 			P1 = CPoint( 0,i);
// 			P2 = CPoint( 319 , i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
// 		}
		P1 = CPoint( 0,i);
		P2 = CPoint( GramT[i] , i);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
	}
	delete []GramH;
	delete []GramT;
}


void CRogerFunction::Histogram1DPositonH( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int CenterPosX , int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH ) 
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;

	LocalMaxLine LocalMax;

	double *GramR = new double[s_h];
	memset( GramR, 0, sizeof(double)*s_h);
	double *GramL = new double[s_h];
	memset( GramL, 0, sizeof(double)*s_h);
	double *GramSum = new double[s_h];
	memset( GramSum, 0, sizeof(double)*s_h);	
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);
	//////////////////////////////////////////////////////////////////////////
	int nCount =0;
	for (i=0; i<s_h; i++)
	{
		for (j=CenterPosX, k= CenterPosX; j<CenterPosX+30; j++, k--)
		{
			nCount++;
			GramR[i]   = GramR[i] + In_ImageNpixel[i*s_w+j];
			GramL[i]   = GramL[i] + In_ImageNpixel[i*s_w+k]; 
		}
	}
	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / nCount;
		GramL[i] = GramL[i] / nCount;
	}
	double TOT=0;
	double TOU=0;
	for (i=0; i<s_h; i++)
	{
		TOT += GramR[i];
		TOU += GramL[i];
	}

	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] * 1000 /TOT ;
		GramL[i] = GramL[i] * 1000 /TOU ;
		GramT[i] = fabs(GramR[i]) + fabs(GramL[i]); 
	}

	for (i=2; i<s_h-2; i++)
	{
		GramSum[i] = 0.1*GramT[i-2] + 0.2*GramT[i-1] + 0.5*GramT[i] + 0.2*GramT[i+1] + 0.1*GramT[i+2] ;
	}
	//Find Local Maximum
	double MaxV;
	k = 10;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}
	////////////////////
	for (i=0; i<s_h; i++)
	{
	   if( DISPLAY_SWITCH == TRUE )
	   {
		   P1 = CPoint( CenterPosX,i);
		   P2 = CPoint( CenterPosX+ GramR[i] , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
		   P1 = CPoint( CenterPosX,i);
		   P2 = CPoint( CenterPosX- GramL[i] , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
	   }

		if ( GramSum[i] > 10 )
		{
			P1 = CPoint( CenterPosX - 10,  i);
			P2 = CPoint( CenterPosX + 10 , i);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
			LocalMax.pLeft   = CPoint( CenterPosX - 10,  i);
			LocalMax.pCenter = CPoint( CenterPosX, i );
			LocalMax.pRight  = CPoint( CenterPosX + 10 , i);
			LocalMax.BDISPLAY= TRUE;
			LocalMax.dSimilar= 0;
			LocalMax.nDist   = 0;
			LocalMax.nTheta  = 0;
			Local_Line.push_back( LocalMax );
		}
	}
	delete []GramR;
	delete []GramL;
	delete []GramSum;
	delete []GramT;
}
void CRogerFunction::Histogram1DPositonW( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, LocalMaxLine &LocalLine1, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH) 
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;
	LocalMaxLine Line;

	double *GramB = new double[s_w];
	memset( GramB, 0, sizeof(double)*s_w);
	double *GramU = new double[s_w];
	memset( GramU, 0, sizeof(double)*s_w);
	double *GramSumB = new double[s_w];
	memset( GramSumB, 0, sizeof(double)*s_w);
	double *GramSumU = new double[s_w];
	memset( GramSumU, 0, sizeof(double)*s_w);
	//////////////////////////////////////////////////////////////////////////
 	int nCount =0;
	// LocalLine1.y > LocalLine2.y
// 	gpMsgbar->ShowMessage("y:(%d -> %d)\n",LocalLine2.pCenter.y, LocalLine1.pCenter.y);
// 	gpMsgbar->ShowMessage("x:(%d -> %d)\n",LocalLine1.pLeft.x,LocalLine1.pRight.x);

 	for (i=LocalLine1.pCenter.y, k=LocalLine1.pCenter.y; i > LocalLine1.pCenter.y - 3 ; i--, k++)
	{
		for (j=0; j<s_w; j++)
		{
			nCount++;
			GramB[j]   = GramB[j] + In_ImageNpixel[i*s_w+j];
			GramU[j]   = GramU[j] + In_ImageNpixel[k*s_w+j];
// 			gpMsgbar->ShowMessage("[%.1f]", GramB[i]);
		}
	}
// 	gpMsgbar->ShowMessage("\n\n");
	for (i=0; i<s_w; i++)
	{
		GramB[i] = GramB[i] / nCount;
		GramU[i] = GramU[i] / nCount;
	}
	double TOT=0;
	double TOU=0;
	for (i=0; i<s_w; i++)
	{
		TOT += GramB[i];
		TOU += GramU[i];
	}

	for (i=0; i<s_w; i++)
	{
		GramB[i] = GramB[i] * 1000 /TOT ;
		GramU[i] = GramU[i] * 1000 /TOU ;

	}
// 	//Smooth
 	k = 5;
 	for (i=k; i<s_w-k; i++)
 	{
 		for (j=-k; j<k; j++)
 		{
 			GramSumB[i] += GramB[i+j]/k ;
			GramSumU[i] += GramU[i+j]/k ;
 		}
 	}
// 	for (i=2; i<318; i++)
// 	{
// 		GramSum[i] = 0.05*GramB[i-2] + 0.1*GramB[i-1] + 0.7*GramB[i] + 0.1*GramB[i+1] + 0.05*GramB[i+2] ;
// 	}
// 	memcpy( GramSum, GramB, sizeof(double)*320);
	//Find Local Maximum
// 	double MaxV;
// 	k = 10;
// 	for (i=0; i<320-k; i++)
// 	{
// 		MaxV = 0;
// 		for (j=0; j<k; j++)
// 		{
// 			if ( GramSum[i+j] >= MaxV)
// 			{
// 				MaxV = GramSum[i+j];
// 			}
// 		}
// 		for (j=0; j<k; j++)
// 		{
// 			if (GramSum[i+j] != MaxV)
// 				GramSum[i+j] = 0;
// 		}
// 	}

	////////////////////
	
	for (i=0; i<s_w; i++)
	{
		if ( GramSumB[i] < k /*|| ( i > LocalLine1.pCenter.x-10 && i < LocalLine1.pCenter.x+10 )*/ )
		{
			GramSumB[i] = 0;
			GramSumU[i] = 0;
		}
	}
	if ( DISPLAY_SWITCH == TRUE )
	{
		for (i=0; i<320; i++)
		{
			P1 = CPoint( i , LocalLine1.pCenter.y );
			P2 = CPoint( i , LocalLine1.pCenter.y - GramSumB[i]/2 );
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			P1 = CPoint( i , LocalLine1.pCenter.y );
			P2 = CPoint( i , LocalLine1.pCenter.y + GramSumU[i]/2 );
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
		}
	}

	//derivative
	for ( i=LocalLine1.pCenter.x ; i > 0 ; i--)
	{
		if ( GramSumB[i-3]==0 && GramSumB[i-2]==0 && GramSumB[i-1]==0 && GramSumB[i+1]!=0 && GramSumB[i+2]!=0 && GramSumB[i+3]!=0 )
		{
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( i , LocalLine1.pCenter.y );
				P2 = CPoint( i , LocalLine1.pCenter.y - 5 );
				FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, s_bpp, P1, 8, RGB(255,0,255), 4);

			}
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
			LocalLine1.pLeft.x = i;
			i = 0;
		}
	}
	for ( i=LocalLine1.pCenter.x ; i < s_w ; i++)
	{
		if ( GramSumB[i-3]!=0 && GramSumB[i-2]!=0 && GramSumB[i-1]!=0 && GramSumB[i+1]==0 && GramSumB[i+2]==0 && GramSumB[i+3]==0 )
		{
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( i , LocalLine1.pCenter.y );
				P2 = CPoint( i , LocalLine1.pCenter.y - 5 );
				FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, s_bpp, P1, 8, RGB(0,255,255), 4);

			}
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			LocalLine1.pRight.x = i;
			i = s_w;
		}
	}
	//同一條縱線比長度
	LocalLine1.nDist = FuncImg.FindDistance(LocalLine1.pLeft.x, LocalLine1.pLeft.y, LocalLine1.pRight.x, LocalLine1.pRight.y);
	delete []GramB;
	delete []GramU;
	delete []GramSumB;
	delete []GramSumU;
}
void CRogerFunction::Histogram1DPositonW_Down( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, LocalMaxLine &LocalLine1, int FindDist, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH) 
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;
	LocalMaxLine Line;

	double *GramB = new double[s_w];
	memset( GramB, 0, sizeof(double)*s_w);
	double *GramSumB = new double[s_w];
	memset( GramSumB, 0, sizeof(double)*s_w);
	//////////////////////////////////////////////////////////////////////////
 	int nCount =0;
 	for (i=LocalLine1.pCenter.y; i > LocalLine1.pCenter.y - FindDist ; i--)
	{
		for (j=0; j<s_w; j++)
		{
			nCount++;
			GramB[j]   = GramB[j] + In_ImageNpixel[i*s_w+j];
		}
	}
// 	gpMsgbar->ShowMessage("\n\n");
	for (i=0; i<s_w; i++)
	{
		GramB[i] = GramB[i] / nCount;
	}
	double TOT=0;
	for (i=0; i<s_w; i++)
	{
		TOT += GramB[i];
	}

	for (i=0; i<s_w; i++)
	{
		GramB[i] = GramB[i] * 1000 /TOT ;
	}
// 	//Smooth
 	k = 5;
 	for (i=k; i<s_w-k; i++)
 	{
 		for (j=-k; j<k; j++)
 		{
 			GramSumB[i] += GramB[i+j]/k ;
 		}
 	}
// 	for (i=2; i<318; i++)
// 	{
// 		GramSum[i] = 0.05*GramB[i-2] + 0.1*GramB[i-1] + 0.7*GramB[i] + 0.1*GramB[i+1] + 0.05*GramB[i+2] ;
// 	}
// 	memcpy( GramSum, GramB, sizeof(double)*320);
	//Find Local Maximum
// 	double MaxV;
// 	k = 10;
// 	for (i=0; i<320-k; i++)
// 	{
// 		MaxV = 0;
// 		for (j=0; j<k; j++)
// 		{
// 			if ( GramSum[i+j] >= MaxV)
// 			{
// 				MaxV = GramSum[i+j];
// 			}
// 		}
// 		for (j=0; j<k; j++)
// 		{
// 			if (GramSum[i+j] != MaxV)
// 				GramSum[i+j] = 0;
// 		}
// 	}

	////////////////////
	
	for (i=0; i<s_w; i++)
	{
		if ( GramSumB[i] < k  )
		{
			GramSumB[i] = 0;
		}
	}
	if ( DISPLAY_SWITCH == TRUE )
	{
		for (i=0; i<s_w; i++)
		{
			P1 = CPoint( i , LocalLine1.pCenter.y );
			P2 = CPoint( i , LocalLine1.pCenter.y - GramSumB[i] );
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		}
	}
	//derivative
	for ( i=LocalLine1.pCenter.x ; i > 0 ; i--)
	{
		if ( GramSumB[i-3]==0 && GramSumB[i-2]==0 && GramSumB[i-1]==0 && GramSumB[i+1]!=0 && GramSumB[i+2]!=0 && GramSumB[i+3]!=0 )
		{
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( i , LocalLine1.pCenter.y );
				P2 = CPoint( i , LocalLine1.pCenter.y - 5 );
				FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, s_bpp, P1, 8, RGB(255,0,255), 2);

			}
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
			LocalLine1.pLeft.x = i;
			i = 0;
		}
	}
	for ( i=LocalLine1.pCenter.x ; i < s_w ; i++)
	{
		if ( GramSumB[i-3]!=0 && GramSumB[i-2]!=0 && GramSumB[i-1]!=0 && GramSumB[i+1]==0 && GramSumB[i+2]==0 && GramSumB[i+3]==0 )
		{
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( i , LocalLine1.pCenter.y );
				P2 = CPoint( i , LocalLine1.pCenter.y - 5 );
				FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, s_bpp, P1, 8, RGB(0,0,255), 2);

			}
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			LocalLine1.pRight.x = i;
			i = s_w;
		}
	}
	///左右相似度測試/////////////////////////////////////////////////////////
	LocalLine1.nRdist = FuncImg.FindDistance( LocalLine1.pCenter.x, LocalLine1.pCenter.y, LocalLine1.pRight.x, LocalLine1.pRight.y);
	LocalLine1.nLdist = FuncImg.FindDistance( LocalLine1.pLeft.x, LocalLine1.pLeft.y, LocalLine1.pCenter.x, LocalLine1.pCenter.y);
	gpMsgbar->ShowMessage("(左右距離差)%d - %d = %d -> ", LocalLine1.nLdist, LocalLine1.nRdist, abs(LocalLine1.nLdist - LocalLine1.nRdist) );
	k = min( LocalLine1.nRdist, LocalLine1.nLdist);
	double dScore;
	double *S1 = new double[k];
	double *S2 = new double[k];
	memset( S1, 0, sizeof(double)*k);
	memset( S2, 0, sizeof(double)*k);
	for ( i=0 ; i<k ; i++ )
	{
		S1[i] = GramSumB[LocalLine1.pCenter.x+i];
		S2[i] = GramSumB[LocalLine1.pCenter.x-i];
	}
	I_Correlation( S1, S2, k, dScore );
	LocalLine1.dSimilar = dScore;
	gpMsgbar->ShowMessage("(相似程度)%.3f", dScore);
	delete []S1;
	delete []S2;
	//////////////////////////////////////////////////////////////////////////
	delete []GramB;
	delete []GramSumB;
}
void CRogerFunction::I_Histogram1DMax2( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax)
{
	int i;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	//////////////////////////////////////////////////////////////////////////
	int TOTAL=0;
	for (i=0; i<CenterP.size() ; i++)
	{
		TOTAL += CenterP[i].x;
		GramW[CenterP[i].x]++;
	}
	TOTAL = TOTAL / CenterP.size();
// 	memcpy( Tmp, GramW, sizeof(double)*s_w);
// 	I_BubbleSort( Tmp, s_w );
	for (i=0; i< s_w; i++)
	{
		XPosMax.push_back( TOTAL);
	}
	delete []Tmp;
	delete []GramW;
}
void CRogerFunction::I_HistogramSCR00( vector <SURFINFO> &vMatch_TBL, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp)
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double *GramW = new double[s_w];
	double *GramH = new double[s_h];
	memset( GramW, 0, sizeof(double)*s_w);
	memset( GramH, 0, sizeof(double)*s_h);
	double *GramW1 = new double[s_w];
	double *GramH1 = new double[s_h];
	memset( GramW1, 0, sizeof(double)*s_w);
	memset( GramH1, 0, sizeof(double)*s_h);
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<vMatch_TBL.size(); i++)
	{
		GramW[vMatch_TBL[i].x]++ ;
		GramH[vMatch_TBL[i].y]++ ;
	}
	k=10;
	for (i=k; i<s_w-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramW1[i]+=GramW[i+j]; 
		}
	}
	memcpy( GramW, GramW1, sizeof(double)*s_w);
	for (i=0; i<s_w; i++)
	{
		P1 = CPoint(i,0);
		P2 = CPoint( i, GramW[i]);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
	}
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramH1[i]+=GramH[i+j]; 
		}
	}
	memcpy( GramH, GramH1, sizeof(double)*s_h);
	for (i=0; i<s_h; i++)
	{
		P1 = CPoint(0,i);
		P2 = CPoint( GramH[i] , i);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
	}
	delete []GramW;
	delete []GramH;
	delete []GramW1;
	delete []GramH1;
}
void CRogerFunction::I_HistogramSCRData( BYTE *In_ImageNpixel, double *GramW, int s_w, double *GramH, int s_h, double *GramWout, int s_wout, double *GramHout, int s_hout)
{
	int i, j;
	CPoint P1, P2;
	CImgProcess FuncImg;
	double dTot1 = 0, dTot2 = 0 ;
 	double *GramWT = new double[s_w];
 	double *GramHT = new double[s_h];
 	memset( GramWT, 0, sizeof(double)*s_w);
 	memset( GramHT, 0, sizeof(double)*s_h);
	//////////////////////////////////////////////////////////////////////////
	for (i=30; i<s_h-30; i++)
	{
		for (j=30; j<s_w-30; j++)
		{
			GramW[j] = GramW[j] + In_ImageNpixel[i*s_w+j];
			GramH[i] = GramH[i] + In_ImageNpixel[i*s_w+j];
			dTot1 += GramW[j];
			dTot2 += GramH[i];			
		}
	}
	for (i=0; i<s_w; i++)
	{
		GramWT[i] = GramW[i]/dTot1*10000;
	}

	for (i=0; i<s_h; i++)
	{
		GramHT[i] = GramH[i]/dTot2*10000;
	}
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<s_w; i++)
	{
		GramWout[i%s_wout] += GramWT[i] ;
	}
	for (i=0; i<s_h; i++)
	{
		GramHout[i%s_hout] += GramHT[i] ;
	}
	/////////////////////////////////////////////////////////////////////////
// 	for (i=0; i<s_w; i++)
// 	{
// 		gpMsgbar->ShowMessage("[%.2f]", GramW[i]);
// 	}
// 	gpMsgbar->ShowMessage("\n\n");
// 	for (i=0; i<s_h; i++)
// 	{
// 		gpMsgbar->ShowMessage("[%.2f]", GramH[i]);
// 	}
// 	gpMsgbar->ShowMessage("\n\n");
// 	//////////////////////////////////////////////////////////////////////////
// 	for (i=0; i<s_wout; i++)
// 	{
// 		gpMsgbar->ShowMessage("[%.2f].", GramWout[i]);
// 	}
// 	gpMsgbar->ShowMessage("\n\n");
// 	for (i=0; i<s_hout; i++)
// 	{
// 		gpMsgbar->ShowMessage("[%.2f].", GramHout[i]);
// 	}
// 	gpMsgbar->ShowMessage("\n\n");

	delete []GramWT;
	delete []GramHT;
}
void CRogerFunction::DISPLAY_RANGE( BYTE *Display_ImageNsize, vector <LocalMaxLine> &Local_Line, int s_w, int s_h, int s_bpp)
{
	//排除邊緣誤差
	CImgProcess FuncImg;
	int i;
	CPoint P1, P2;
	int Left_Min  = s_w;
	int Right_Max = 0;
	int Up_Min    = s_h;
	int Bottom_Max =0;
	for (i=0; i<Local_Line.size(); i++)
	{
		if( Local_Line[i].pCenter.y > s_h-10 || Local_Line[i].pCenter.y < 10 || Local_Line[i].pRight.x > s_w-10 || Local_Line[i].pLeft.x < 10 )
		{
			Local_Line.erase( Local_Line.begin()+i );
		}
	}
	for ( i=0; i<Local_Line.size(); i++)
	{
		if( Local_Line[i].pRight.x > Right_Max ) Right_Max = Local_Line[i].pRight.x;
		if( Local_Line[i].pLeft.x < Left_Min ) Left_Min = Local_Line[i].pLeft.x;
		if( Local_Line[i].pCenter.y < Up_Min ) Up_Min = Local_Line[i].pCenter.y;
		if( Local_Line[i].pCenter.y > Bottom_Max ) Bottom_Max = Local_Line[i].pCenter.y;
	}
	P1 = CPoint( Left_Min, Up_Min -15 );
	P2 = CPoint( Right_Max, Bottom_Max );
	//框出foreground
	I_DrawRange2( Display_ImageNsize, P1, P2, s_w, s_h, s_bpp, RGB(255,255,0));
	double dLAvg = 0;
	for (i=0; i<Local_Line.size(); i++)
	{
		P1 = CPoint( Local_Line[i].pLeft.x, Local_Line[i].pCenter.y);
		P2 = CPoint( Local_Line[i].pRight.x, Local_Line[i].pCenter.y);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
	}
}
void CRogerFunction::DISPLAY_RANGE_BESTSIMILARITY( BYTE *Display_ImageNsize, vector <LocalMaxLine> &Local_Line, int s_w, int s_h, int s_bpp)
{
	//排除邊緣誤差
	CImgProcess FuncImg;
	int i, j, k;
	CPoint P1, P2;
	int Left_Min  = s_w;
	int Right_Max = 0;
	int Up_Min    = s_h;
	int Bottom_Max =0;
	
	//先排除邊緣
	//再排除相似度低
	for (i=0; i<Local_Line.size(); i++)
	{

		j = abs(Local_Line[i].nLdist - Local_Line[i].nRdist);
		k = Local_Line[i].nLdist + Local_Line[i].nRdist;
		if( Local_Line[i].dSimilar < 0.6 || j > 15  || k > s_w/3 || Local_Line[i].pCenter.y > s_h-10 || Local_Line[i].pCenter.y < 10 /*|| Local_Line[i].pRight.x > s_w-10 || Local_Line[i].pLeft.x < 10*/ )
		{
			Local_Line[i].BDISPLAY = FALSE;
		}
	}
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( Local_Line[i].BDISPLAY == FALSE )
		{
			Local_Line.erase( Local_Line.begin()+i );
			i = 0;
		}
	}

	gpMsgbar->ShowMessage("Left Size: %d\n", Local_Line.size());
	for ( i=0; i<Local_Line.size(); i++)
	{
		gpMsgbar->ShowMessage("y:(%d), S: %f\n", s_h - Local_Line[i].pCenter.y, Local_Line[i].dSimilar);
		if( Local_Line[i].pRight.x > Right_Max ) Right_Max = Local_Line[i].pRight.x;
		if( Local_Line[i].pLeft.x < Left_Min ) Left_Min = Local_Line[i].pLeft.x;
		if( Local_Line[i].pCenter.y < Up_Min ) Up_Min = Local_Line[i].pCenter.y;
		if( Local_Line[i].pCenter.y > Bottom_Max ) Bottom_Max = Local_Line[i].pCenter.y;
	}
	P1 = CPoint( Left_Min, Up_Min -15 );
	P2 = CPoint( Right_Max, Bottom_Max+15 );
	//框出foreground
	I_DrawRange2( Display_ImageNsize, P1, P2, s_w, s_h, s_bpp, RGB(255,255,0));
	double dLAvg = 0;
	for (i=0; i<Local_Line.size(); i++)
	{
		P1 = CPoint( Local_Line[i].pLeft.x, Local_Line[i].pCenter.y);
		P2 = CPoint( Local_Line[i].pRight.x, Local_Line[i].pCenter.y);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
	}
}
void CRogerFunction::Histogram1DPositonH_SIM( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int CenterPosX , int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH ) 
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;

	LocalMaxLine LocalMax;

	double *GramR = new double[s_h];
	memset( GramR, 0, sizeof(double)*s_h);
	double *GramL = new double[s_h];
	memset( GramL, 0, sizeof(double)*s_h);

	double *GramRT = new double[s_h];
	memset( GramRT, 0, sizeof(double)*s_h);
	double *GramLT = new double[s_h];
	memset( GramLT, 0, sizeof(double)*s_h);

	double *GramSum = new double[s_h];
	memset( GramSum, 0, sizeof(double)*s_h);	
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);
	//////////////////////////////////////////////////////////////////////////
	double dCount =0;
	for (i=0; i<s_h-50; i++)
	{
		for (j=CenterPosX, k= CenterPosX; j<CenterPosX+30; j++, k--)
		{
			GramR[i]   = GramR[i] + In_ImageNpixel[i*s_w+j];
			GramL[i]   = GramL[i] + In_ImageNpixel[i*s_w+k]; 
		}
	}
	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / 150;
		GramL[i] = GramL[i] / 150;
		GramT[i] = GramL[i] + GramR[i];
		dCount += GramT[i];
	}
	dCount /= s_h;
	gpMsgbar->ShowMessage("平均水平: dCount: %.3f\n", dCount );
	//////////////////////////////////////////////////////////////////////////
// 	for (i=2; i<s_h-2; i++)
// 	{
// 		GramSum[i] = 0.1*GramT[i-2] + 0.2*GramT[i-1] + 0.4*GramT[i] + 0.2*GramT[i+1] + 0.1*GramT[i+2] ;
// 		GramRT[i] = 0.1*GramR[i-2] + 0.2*GramR[i-1] + 0.4*GramR[i] + 0.2*GramR[i+1] + 0.1*GramR[i+2] ;
// 		GramLT[i] = 0.1*GramL[i-2] + 0.2*GramL[i-1] + 0.4*GramL[i] + 0.2*GramL[i+1] + 0.1*GramL[i+2] ;
// 
// 	}
	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
			GramRT[i]  += GramR[i+j]/k;
			GramLT[i]  += GramL[i+j]/k;
		}
	} 
	//////////////////////////////////////////////////////////////////////////
	memcpy( GramR, GramRT, sizeof(double)*s_h);
	memcpy( GramL, GramLT, sizeof(double)*s_h);

	for (i=0; i<s_h; i++)
	{
		if ( GramSum[i] < 16 )
		{
			GramSum[i] = 0;
		}
// 		gpMsgbar->ShowMessage("(%.2f)", GramSum[i]);
	}
	//Find Local Maximum
	double MaxV;
	k = 5;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}

	for (i=10; i<s_h-10; i++)
	{
// 	   if( DISPLAY_SWITCH == TRUE )
// 	   {
// 		   P1 = CPoint( CenterPosX,i);
// 		   P2 = CPoint( CenterPosX+ GramR[i] , i);
// 		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
// 		   P1 = CPoint( CenterPosX,i);
// 		   P2 = CPoint( CenterPosX- GramL[i] , i);
// 		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
// 	   }
	   if( DISPLAY_SWITCH == TRUE )
	   {
		   P1 = CPoint( 0, i);
		   P2 = CPoint( GramSum[i], i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	   }
	   if ( GramSum[i] > (0.6 *dCount) )
	   { 
		   if( DISPLAY_SWITCH == TRUE )
		   {
			   P1 = CPoint(  CenterPosX - 5,  i);
			   P2 = CPoint(  CenterPosX + 5 , i);
			   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		   }
		   LocalMax.pLeft   = CPoint( CenterPosX - 10,  i);
		   LocalMax.pCenter = CPoint( CenterPosX, i );
		   LocalMax.pRight  = CPoint( CenterPosX + 10 , i);
		   LocalMax.BDISPLAY= FALSE;
		   LocalMax.dSimilar= 0;
		   LocalMax.nDist   = 0;
		   LocalMax.nTheta  = 0;
		   LocalMax.dTotalS = GramSum[i];
		   Local_Line.push_back( LocalMax );
		}
	}
	gpMsgbar->ShowMessage("LocalLine總數: %d\n", Local_Line.size());



	///左右相似度測試/////////////////////////////////////////////////////////
	int nDist;
	int nCnt = 0;
	double dScore;
	double dStrength1, dStrength2;
	////////////////////////////////////////////////////////////
	double *LocalHistogram = new double[Local_Line.size()];
	memset( LocalHistogram, 0, sizeof(double)*Local_Line.size());
	////////////////////////////////////////////////////////////
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( i != 0 )
		{
			nDist = Local_Line[i].pCenter.y - Local_Line[i-1].pCenter.y;
// 			gpMsgbar->ShowMessage(" l(i):%d -> l(i-1):%d, nDist:%d \n", s_h - Local_Line[i].pCenter.y, s_h - Local_Line[i-1].pCenter.y , nDist );
		}
		else
		{
			nDist = 15;
// 			gpMsgbar->ShowMessage(" l(i):%d -> Bottom: %d \n", s_h - Local_Line[i].pCenter.y, s_h - Local_Line[i].pCenter.y+15 );
		}
		dStrength1 = 0;
		dStrength2 = 0;
		double *S1 = new double[nDist];
		double *S2 = new double[nDist];
		memset( S1, 0, sizeof(double)*nDist);
		memset( S2, 0, sizeof(double)*nDist);
		for ( j=0 ; j< nDist ; j++ )
		{
			S1[j] = GramR[Local_Line[i].pCenter.y-j];
			dStrength1 += (S1[j]);
			S2[j] = GramL[Local_Line[i].pCenter.y-j];
			dStrength2 += (S2[j]);
		}
		Local_Line[i].nBottomY = Local_Line[i].pCenter.y - nDist ;
		I_Correlation( S1, S2, nDist, dScore );
		gpMsgbar->ShowMessage("y: %d - %d : (相似程度)%.3f...", s_h - Local_Line[i].pCenter.y, s_h - Local_Line[i].pCenter.y+nDist , dScore);
		Local_Line[i].nDist = nDist;
		Local_Line[i].dSimilar = dScore;
		Local_Line[i].dHoriStrength = dStrength1 + dStrength2 ;
		gpMsgbar->ShowMessage("Local水平強度:( %.2f)...", Local_Line[i].dHoriStrength);
		Local_Line[i].dHoriStrengthAVG = Local_Line[i].dHoriStrength / nDist ;
		gpMsgbar->ShowMessage("Local水平平均強度:( %.2f).......\n", Local_Line[i].dHoriStrengthAVG);
		LocalHistogram[i] = Local_Line[i].dTotalS;

		delete []S1;
		delete []S2;
	}
	//找出水平平均強度最強區段
// 	for ( i=0; i<Local_Line.size(); i++ )
//  		gpMsgbar->ShowMessage("B[%d:(%.2f)].", i, LocalHistogram[i]);
// 	gpMsgbar->ShowMessage("\n");
 	I_BubbleSort( LocalHistogram, Local_Line.size() );
// 	for ( i=0; i<Local_Line.size(); i++ )
// 		gpMsgbar->ShowMessage("A[%d:(%.2f)].", i, LocalHistogram[i]);
// 	gpMsgbar->ShowMessage("\n");
	int nMaxPosH;
	if( DISPLAY_SWITCH == TRUE )
	{
		for (i=0; i<Local_Line.size(); i++)
		{
			if ( Local_Line[i].dTotalS == LocalHistogram[0] )
			{
				Local_Line[i].BDISPLAY = TRUE;
				//找到1st最大平均強度
				P1 = CPoint( 125 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1  , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
				////////////////////////////////////////////////
				nMaxPosH = Local_Line[i].pCenter.y;
				gpMsgbar->ShowMessage("nMaxPosH:%d\n", nMaxPosH );
			}
			else if ( Local_Line[i].dTotalS == LocalHistogram[1]  )
			{
				//找到2nd最大平均強度
				Local_Line[i].BDISPLAY = TRUE;
				P1 = CPoint( 125 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			}
			else if ( Local_Line[i].dTotalS == LocalHistogram[2]  )
			{
				//找到2nd最大平均強度
				Local_Line[i].BDISPLAY = TRUE;
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
			}
			else if ( Local_Line[i].dTotalS == LocalHistogram[3]  )
			{
				//找到2nd最大平均強度
				Local_Line[i].BDISPLAY = TRUE;
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
			}
			else if ( Local_Line[i].dTotalS == LocalHistogram[4]  )
			{
				//找到2nd最大平均強度
				Local_Line[i].BDISPLAY = TRUE;
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
			else if ( Local_Line[i].dTotalS == LocalHistogram[5]  )
			{
				//找到2nd最大平均強度
				Local_Line[i].BDISPLAY = TRUE;
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
		}
	}
// 	for( i=0 ; i< Local_Line.size(); i++)
// 	{
// 		if( Local_Line[i].BDISPLAY == FALSE )
// 		{
// 			Local_Line.erase( Local_Line.begin() +i );
// 			i = -1;
// 		}
// 	}
	gpMsgbar->ShowMessage("LocalLine總數刪除後: %d\n", Local_Line.size());

	delete []LocalHistogram;
	////Test/////////////////////////////////////////////////////////////////
	
	for (i=0; i<Local_Line.size(); i++)
	{
		gpMsgbar->ShowMessage("(%d:%d)", s_h-Local_Line[i].pCenter.y, Local_Line[i].nDist);
	}
	int nDistConst; 
	if ( nMaxPosH < 50 )
		nDistConst = 65;
	else
		nDistConst = 50;

	double dAccum;
	nDist = 0;
	vector <Segment> SegPos;
	Segment SEG;
	for ( i=0; i<Local_Line.size(); i++)
	{
		dAccum = 0;
		for (j=i; j<Local_Line.size(); j++)
		{
			nDist = abs( Local_Line[i].pCenter.y - Local_Line[j].pCenter.y );
			if ( i != j && nDist < nDistConst )
			{
				dAccum += Local_Line[j].dTotalS;
				gpMsgbar->ShowMessage("(%d-%d)%d-%d: %.2f\n", s_h-Local_Line[i].pCenter.y, s_h-Local_Line[j].pCenter.y, i, j, dAccum );
				SEG.nStartH = Local_Line[i].pCenter.y;
				SEG.nEndH   = Local_Line[j].pCenter.y;
				SEG.nCenterX= Local_Line[i].pCenter.x;
				SEG.dTotal = dAccum;
				SEG.Remain = FALSE;
				SegPos.push_back( SEG );
			}
		}
	}
	gpMsgbar->ShowMessage("\n---------------------------\n");
	for (i=0; i<SegPos.size(); i++)
	{
		gpMsgbar->ShowMessage("(%d):%d_%d_%.2f\n", SegPos[i].nCenterX, s_h-SegPos[i].nStartH, s_h-SegPos[i].nEndH, SegPos[i].dTotal);
	}
	//找出最大值
	double *SEGBUFFER = new double[ SegPos.size() ];
	memset( SEGBUFFER, 0, sizeof(double)*SegPos.size());
	for (i=0; i<SegPos.size(); i++)
		SEGBUFFER[i] = SegPos[i].dTotal;
	I_BubbleSort( SEGBUFFER, SegPos.size() );

	for (i=0; i<SegPos.size(); i++)
	{
		if ( SegPos[i].dTotal == SEGBUFFER[0] )
		{
			SegPos[i].Remain = TRUE;
		}
	}
	gpMsgbar->ShowMessage("\n~~~~~~~~~~~~~~~~~\n");
	for (i=0; i<SegPos.size(); i++)
	{
		if( SegPos[i].Remain == TRUE )
		{
			gpMsgbar->ShowMessage("(%d):%d_%d_%.2f\n", SegPos[i].nCenterX, s_h-SegPos[i].nStartH, s_h-SegPos[i].nEndH, SegPos[i].dTotal);
			P1 = CPoint(SegPos[i].nCenterX-20,SegPos[i].nStartH);
			P2 = CPoint(SegPos[i].nCenterX+20,SegPos[i].nEndH);
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
		}
	}
	

	//////////////////////////////////////////////////////////////////////////
	delete []GramR;
	delete []GramL;
	delete []GramSum;
	delete []GramT;
}
void CRogerFunction::Histogram1DPositonW_NEW( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, LocalMaxLine &LocalLine1, int nFindDist, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH) 
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;
	LocalMaxLine Line;

	double *GramB = new double[s_w];
	memset( GramB, 0, sizeof(double)*s_w);
	double *GramU = new double[s_w];
	memset( GramU, 0, sizeof(double)*s_w);
	double *GramSum = new double[s_w];
	memset( GramSum, 0, sizeof(double)*s_w);

	double *GramT = new double[s_w];
	memset( GramT, 0, sizeof(double)*s_w);
	double *GramSumT = new double[s_w];
	memset( GramSumT, 0, sizeof(double)*s_w);

	//////////////////////////////////////////////////////////////////////////
 	double dCount =0;
 	for (i=LocalLine1.pCenter.y, k=LocalLine1.pCenter.y; i > LocalLine1.pCenter.y - nFindDist ; i--, k++)
	{
		for (j=30; j<s_w-30; j++)
		{
			GramB[j]   = GramB[j] + ((double)In_ImageNpixel[i*s_w+j]/*/nFindDist*/);
// 			GramU[j]   = GramU[j] + ((double)In_ImageNpixel[k*s_w+j]/*/nFindDist*/);
		}
	}
	for (i=0; i<s_w; i++)
	{
		GramSum[i] = (GramB[i] + GramU[i])/nFindDist;
		dCount += GramSum[i];
	}
	dCount /= s_w;
// 	gpMsgbar->ShowMessage("dCount: %.3f\n", dCount );
	memcpy( GramT, GramSum, sizeof(double)*s_w );
	for (i=2; i<s_w-2; i++)
	{
		GramSumT[i] = 0.1*GramT[i-2] + 0.2*GramT[i-1] + 0.4*GramT[i] + 0.2*GramT[i+1] + 0.1*GramT[i+2] ;
	}
	//Find Local Maximum
	double MaxV;
	k = 20;
	for (i=0; i<s_w-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSumT[i+j] >= MaxV)
			{
				MaxV = GramSumT[i+j] ;
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSumT[i+j] != MaxV)
				GramSumT[i+j] = 0;
		}
	}


	//////////////////////////////////////////////////////////////////////////
	for( i=0 ; i<s_w; i++ )
	{
		if( GramSum[i] <= ( 1.8 * dCount) )
		{
			GramSum[i] = 0 ;
		}
	}
	k=s_w/32;
	for (i=0 ; i< s_w-k; i++ )
	{
		if( GramSum[i] == 0 && GramSum[i+1] == 0 && GramSum[i+2] == 0 && GramSum[i+k-2] == 0 && GramSum[i+k-1] == 0 && GramSum[i+k] == 0)
		{
			for (j=0; j<k; j++) GramSum[i+j] = 0;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	double nAccumulator=0;
	if ( DISPLAY_SWITCH == TRUE )
	{
		for (i=0; i<s_w; i++)
		{
			nAccumulator += GramSum[i]/10;
			P1 = CPoint( i , LocalLine1.pCenter.y  );
			P2 = CPoint( i , LocalLine1.pCenter.y - GramSum[i]/10 );
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			P1 = CPoint( i , LocalLine1.pCenter.y  );
			P2 = CPoint( i , LocalLine1.pCenter.y - GramSumT[i]/10 );
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
		}
	}
	LocalLine1.dVertStrength = nAccumulator;
	gpMsgbar->ShowMessage("nAccumulator: %.3f ->>>", nAccumulator );
	//////////////////////////////////////////////////////////////////////////
	///左右相似度測試/////////////////////////////////////////////////////////
	int nDist;
	int nCnt = 0;
	double dScore;
	double dStrength1, dStrength2;
	dStrength1 = 0;
	dStrength2 = 0;
	nDist = 30;
	double *S1 = new double[nDist];
	double *S2 = new double[nDist];
	memset( S1, 0, sizeof(double)*nDist);
	memset( S2, 0, sizeof(double)*nDist);
	for ( j=0 ; j< nDist ; j++ )
	{
		S1[j] = GramSum[LocalLine1.pCenter.x+j];
		dStrength1 += (S1[j]);
		S2[j] = GramSum[LocalLine1.pCenter.x-j];
		dStrength2 += (S2[j]);
	}
	LocalLine1.dVertStrength = dStrength1 + dStrength2;
	I_Correlation( S1, S2, nDist, dScore );
	gpMsgbar->ShowMessage("y: %d - %d : (橫向相似程度)%.3f.. ", s_h - LocalLine1.pCenter.y, s_h - LocalLine1.pCenter.y+nDist , dScore);
	gpMsgbar->ShowMessage("左量: %.3f - 右量: %.3f..(差: %.3f)", dStrength2, dStrength1, dStrength1-dStrength2);
	gpMsgbar->ShowMessage("垂直強度%.3f\n", LocalLine1.dVertStrength/nDist );
	
	delete []S1;
	delete []S2;


	delete []GramB;
	delete []GramU;
	delete []GramSum;
	delete []GramT;
}
void CRogerFunction::Histogram1DPositonH_SIMNEW( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, SelfINFO &CenterLine , int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH ) 
{
	int i, j, k;
	CPoint P1, P2;
	CImgProcess FuncImg;
	LocalMaxLine LocalMax;
	double *GramR = new double[s_h];
	memset( GramR, 0, sizeof(double)*s_h);
	double *GramL = new double[s_h];
	memset( GramL, 0, sizeof(double)*s_h);

	double *GramRT = new double[s_h];
	memset( GramRT, 0, sizeof(double)*s_h);
	double *GramLT = new double[s_h];
	memset( GramLT, 0, sizeof(double)*s_h);

	double *GramSum = new double[s_h];
	memset( GramSum, 0, sizeof(double)*s_h);	
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);
	//////////////////////////////////////////////////////////////////////////
	double dCount =0;
	for (i=0; i<s_h; i++)
	{
		for (j=CenterLine.CenterPosX, k= CenterLine.CenterPosX; j<CenterLine.CenterPosX+30; j++, k--)
		{
			GramR[i]   = GramR[i] + In_ImageNpixel[i*s_w+j];
			GramL[i]   = GramL[i] + In_ImageNpixel[i*s_w+k]; 
		}
	}
	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / 150;
		GramL[i] = GramL[i] / 150;
		GramT[i] = GramL[i] + GramR[i];
		dCount += GramT[i];
	}
	dCount /= s_h;
	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
			GramRT[i]  += GramR[i+j]/k;
			GramLT[i]  += GramL[i+j]/k;
		}
	} 
	//////////////////////////////////////////////////////////////////////////
	memcpy( GramR, GramRT, sizeof(double)*s_h);
	memcpy( GramL, GramLT, sizeof(double)*s_h);

	for (i=0; i<s_h; i++)
	{
		if ( GramSum[i] < 16 )
		{
			GramSum[i] = 0;
		}
	}
	//中心區域加強權重
	for (i=0; i<s_h; i++)
	{
		if ( i > CenterLine.nHBT-20 && i<CenterLine.nHUP+20 )
			GramSum[i] = 2 * GramSum[i];
	}
	//Find Local Maximum
	double MaxV;
	k = 5;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}

	for (i=10; i<s_h-10; i++)
	{
// 	   if( DISPLAY_SWITCH == TRUE )
// 	   {
// 		   P1 = CPoint( CenterLine.CenterPosX,i);
// 		   P2 = CPoint( CenterLine.CenterPosX+ GramR[i]/2 , i);
// 		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
// 		   P1 = CPoint( CenterLine.CenterPosX,i);
// 		   P2 = CPoint( CenterLine.CenterPosX- GramL[i]/2 , i);
// 		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
// 	   }
	   if( DISPLAY_SWITCH == TRUE )
	   {
		   P1 = CPoint( 0, i);
		   P2 = CPoint( GramSum[i], i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	   }
	   if ( GramSum[i] > (0.6 *dCount) )
	   { 
		   if( DISPLAY_SWITCH == TRUE )
		   {
			   P1 = CPoint(  CenterLine.CenterPosX - 5,  i);
			   P2 = CPoint(  CenterLine.CenterPosX + 5 , i);
			   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		   }
		   LocalMax.pLeft   = CPoint( CenterLine.CenterPosX - 10,  i);
		   LocalMax.pCenter = CPoint( CenterLine.CenterPosX, i );
		   LocalMax.pRight  = CPoint( CenterLine.CenterPosX + 10 , i);
		   LocalMax.BDISPLAY= FALSE;
		   LocalMax.dSimilar= 0;
		   LocalMax.nDist   = 0;
		   LocalMax.nTheta  = 0;
		   LocalMax.dTotalS = GramSum[i];
		   Local_Line.push_back( LocalMax );
		}
	}
	///左右相似度測試/////////////////////////////////////////////////////////
	int nDist;
	int nCnt = 0;
	double dScore;
	double dStrength1, dStrength2;
	////////////////////////////////////////////////////////////
	double *LocalHistogram = new double[Local_Line.size()];
	memset( LocalHistogram, 0, sizeof(double)*Local_Line.size());
	////////////////////////////////////////////////////////////
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( i != 0 )
		{
			nDist = Local_Line[i].pCenter.y - Local_Line[i-1].pCenter.y;
		}
		else
		{
			nDist = 15;
		}
		dStrength1 = 0;
		dStrength2 = 0;
		double *S1 = new double[nDist];
		double *S2 = new double[nDist];
		memset( S1, 0, sizeof(double)*nDist);
		memset( S2, 0, sizeof(double)*nDist);
		for ( j=0 ; j< nDist ; j++ )
		{
			S1[j] = GramR[Local_Line[i].pCenter.y-j];
			dStrength1 += (S1[j]);
			S2[j] = GramL[Local_Line[i].pCenter.y-j];
			dStrength2 += (S2[j]);
		}
		Local_Line[i].nBottomY = Local_Line[i].pCenter.y - nDist ;
		I_Correlation( S1, S2, nDist, dScore );
		Local_Line[i].nDist = nDist;
		Local_Line[i].dSimilar = dScore;
		Local_Line[i].dHoriStrength = dStrength1 + dStrength2 ;
		Local_Line[i].dHoriStrengthAVG = Local_Line[i].dHoriStrength / nDist ;
		LocalHistogram[i] = Local_Line[i].dTotalS;

		delete []S1;
		delete []S2;
	}
 	I_BubbleSort( LocalHistogram, Local_Line.size() );
	int nMaxPosH;
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( Local_Line[i].dTotalS == LocalHistogram[0] )
		{
			Local_Line[i].BDISPLAY = TRUE;
			//找到1st最大平均強度
// 			if ( DISPLAY_SWITCH == TRUE )
// 			{
				P1 = CPoint( Local_Line[i].pCenter.x ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1  , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
// 			}
			////////////////////////////////////////////////
			nMaxPosH = Local_Line[i].pCenter.y;
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[1]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
// 			if ( DISPLAY_SWITCH == TRUE )
// 			{
				P1 = CPoint( Local_Line[i].pCenter.x ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
// 			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[2]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
// 			if ( DISPLAY_SWITCH == TRUE )
// 			{
				P1 = CPoint( Local_Line[i].pCenter.x ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
// 			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[3]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[4]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[5]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
		}
	}
// 	for( i=0 ; i< Local_Line.size(); i++)
// 	{
// 		if( Local_Line[i].BDISPLAY == FALSE )
// 		{
// 			Local_Line.erase( Local_Line.begin() +i );
// 			i = -1;
// 		}
// 	}
// 	gpMsgbar->ShowMessage("LocalLine總數刪除後: %d\n", Local_Line.size());

	delete []LocalHistogram;
	////Test/////////////////////////////////////////////////////////////////
	
// 	for (i=0; i<Local_Line.size(); i++)
// 	{
// 		gpMsgbar->ShowMessage("(%d:%d)", s_h-Local_Line[i].pCenter.y, Local_Line[i].nDist);
// 	}
	int nDistConst; 
	if ( nMaxPosH < s_h/3 )
		nDistConst = s_h/4;
	else
		nDistConst = s_h/5;

	double dAccum;
	nDist = 0;
	vector <Segment> SegPos;
	Segment SEG;
	for ( i=0; i<Local_Line.size(); i++)
	{
		dAccum = 0;
		for (j=i; j<Local_Line.size(); j++)
		{
			nDist = abs( Local_Line[i].pCenter.y - Local_Line[j].pCenter.y );
			if ( i != j && nDist < nDistConst )
			{
				dAccum += Local_Line[j].dTotalS;
// 				gpMsgbar->ShowMessage("(%d-%d)%d-%d: %.2f\n", s_h-Local_Line[i].pCenter.y, s_h-Local_Line[j].pCenter.y, i, j, dAccum );
				SEG.nStartH = Local_Line[i].pCenter.y;
				SEG.nEndH   = Local_Line[j].pCenter.y;
				SEG.nCenterX= Local_Line[i].pCenter.x;
				SEG.dTotal = dAccum;
				SEG.Remain = FALSE;
				SegPos.push_back( SEG );
			}
		}
	}
	//找出最大值
	double *SEGBUFFER = new double[ SegPos.size() ];
	memset( SEGBUFFER, 0, sizeof(double)*SegPos.size());
	for (i=0; i<SegPos.size(); i++)
		SEGBUFFER[i] = SegPos[i].dTotal;
	I_BubbleSort( SEGBUFFER, SegPos.size() );

	for (i=0; i<SegPos.size(); i++)
	{
		if ( SegPos[i].dTotal == SEGBUFFER[0] )
		{
			SegPos[i].Remain = TRUE;
		}
	}
	for (i=0; i<SegPos.size(); i++)
	{
		if( SegPos[i].Remain == TRUE )
		{
			nDist = abs(SegPos[i].nEndH+10 - SegPos[i].nStartH+10);
			nDist *= 0.6;
			P1 = CPoint(SegPos[i].nCenterX-nDist,SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+nDist,SegPos[i].nEndH+10);
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			P1 = CPoint(SegPos[i].nCenterX-nDist+1,SegPos[i].nStartH-9);
			P2 = CPoint(SegPos[i].nCenterX+nDist-1,SegPos[i].nEndH+9);
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			P1 = CPoint(SegPos[i].nCenterX-nDist*2.6,SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+nDist*2,SegPos[i].nEndH+nDist*2.2);
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
		}
	}
	

	//////////////////////////////////////////////////////////////////////////
	delete []GramR;
	delete []GramL;
	delete []GramSum;
	delete []GramT;
}
void CRogerFunction::Histogram1DPositonH_SIMNEW( MatchRectangle &Range, int *HistogramW, int *HistogramH, BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, SelfINFO &CenterLine,  int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH ) 
{
	int i, j, k, m, n;
	CPoint P1, P2;
	CImgProcess FuncImg;
	LocalMaxLine LocalMax;
	double *GramR = new double[s_h];
	memset( GramR, 0, sizeof(double)*s_h);
	double *GramL = new double[s_h];
	memset( GramL, 0, sizeof(double)*s_h);

	double *GramRT = new double[s_h];
	memset( GramRT, 0, sizeof(double)*s_h);
	double *GramLT = new double[s_h];
	memset( GramLT, 0, sizeof(double)*s_h);

	double *GramSum = new double[s_h];
	memset( GramSum, 0, sizeof(double)*s_h);	
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);
	//////////////////////////////////////////////////////////////////////////
	double dCount =0;
	for (i=0; i<s_h; i++)
	{
		for (j=CenterLine.CenterPosX, k= CenterLine.CenterPosX; j<CenterLine.CenterPosX+30; j++, k--)
		{
			GramR[i]   = GramR[i] + In_ImageNpixel[i*s_w+j];
			GramL[i]   = GramL[i] + In_ImageNpixel[i*s_w+k]; 
		}
	}
	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / 150;
		GramL[i] = GramL[i] / 150;
		GramT[i] = GramL[i] + GramR[i];
		dCount += GramT[i];
	}
	dCount /= s_h;
	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
			GramRT[i]  += GramR[i+j]/k;
			GramLT[i]  += GramL[i+j]/k;
		}
	} 
	//////////////////////////////////////////////////////////////////////////
	memcpy( GramR, GramRT, sizeof(double)*s_h);
	memcpy( GramL, GramLT, sizeof(double)*s_h);

	for (i=0; i<s_h; i++)
	{
		if ( GramSum[i] < 16 )
		{
			GramSum[i] = 0;
		}
		HistogramH[i] = GramSum[i];
	}
	//中心區域加強權重////////////////////////////////////////////////////
	for (i=0; i<s_h; i++)
	{
		if ( i > CenterLine.nHBT-20 && i<CenterLine.nHUP+20 )
		{
			GramSum[i] = 2 * GramSum[i];
		}
	}
	//Find Local Maximum
	double MaxV;
	k = 5;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}

	for (i=10; i<s_h-10; i++)
	{
	   if( DISPLAY_SWITCH == TRUE )
	   {
		   P1 = CPoint( CenterLine.CenterPosX,i);
		   P2 = CPoint( CenterLine.CenterPosX+ GramR[i]/2 , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
		   P1 = CPoint( CenterLine.CenterPosX,i);
		   P2 = CPoint( CenterLine.CenterPosX- GramL[i]/2 , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
	   }
	   if( DISPLAY_SWITCH == TRUE )
	   {
		   P1 = CPoint( 0, i);
		   P2 = CPoint( GramSum[i], i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	   }
	   if ( GramSum[i] > (0.6 *dCount) )
	   { 
		   if( DISPLAY_SWITCH == TRUE )
		   {
			   P1 = CPoint(  CenterLine.CenterPosX - 5,  i);
			   P2 = CPoint(  CenterLine.CenterPosX + 5 , i);
			   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		   }
		   LocalMax.pLeft   = CPoint( CenterLine.CenterPosX - 10,  i);
		   LocalMax.pCenter = CPoint( CenterLine.CenterPosX, i );
		   LocalMax.pRight  = CPoint( CenterLine.CenterPosX + 10 , i);
		   LocalMax.BDISPLAY= FALSE;
		   LocalMax.dSimilar= 0;
		   LocalMax.nDist   = 0;
		   LocalMax.nTheta  = 0;
		   LocalMax.dTotalS = GramSum[i];
		   Local_Line.push_back( LocalMax );
		}
	}
	///左右相似度測試/////////////////////////////////////////////////////////
	int nDist;
	int nCnt = 0;
	double dScore;
	double dStrength1, dStrength2;
	////////////////////////////////////////////////////////////
	double *LocalHistogram = new double[Local_Line.size()];
	memset( LocalHistogram, 0, sizeof(double)*Local_Line.size());
	////////////////////////////////////////////////////////////
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( i != 0 )
		{
			nDist = Local_Line[i].pCenter.y - Local_Line[i-1].pCenter.y;
		}
		else
		{
			nDist = 15;
		}
		dStrength1 = 0;
		dStrength2 = 0;
		double *S1 = new double[nDist];
		double *S2 = new double[nDist];
		memset( S1, 0, sizeof(double)*nDist);
		memset( S2, 0, sizeof(double)*nDist);
		for ( j=0 ; j< nDist ; j++ )
		{
			S1[j] = GramR[Local_Line[i].pCenter.y-j];
			dStrength1 += (S1[j]);
			S2[j] = GramL[Local_Line[i].pCenter.y-j];
			dStrength2 += (S2[j]);
		}
		Local_Line[i].nBottomY = Local_Line[i].pCenter.y - nDist ;
		I_Correlation( S1, S2, nDist, dScore );
		Local_Line[i].nDist = nDist;
		Local_Line[i].dSimilar = dScore;
		Local_Line[i].dHoriStrength = dStrength1 + dStrength2 ;
		Local_Line[i].dHoriStrengthAVG = Local_Line[i].dHoriStrength / nDist ;
		LocalHistogram[i] = Local_Line[i].dTotalS;

		delete []S1;
		delete []S2;
	}
 	I_BubbleSort( LocalHistogram, Local_Line.size() );
	int nMaxPosH;
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( Local_Line[i].dTotalS == LocalHistogram[0] )
		{
			Local_Line[i].BDISPLAY = TRUE;
			//找到1st最大平均強度
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( Local_Line[i].pCenter.x ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1  , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
			}
			////////////////////////////////////////////////
			nMaxPosH = Local_Line[i].pCenter.y;
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[1]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( Local_Line[i].pCenter.x ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[2]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( Local_Line[i].pCenter.x ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[3]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[4]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[5]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
		}
	}
// 	for( i=0 ; i< Local_Line.size(); i++)
// 	{
// 		if( Local_Line[i].BDISPLAY == FALSE )
// 		{
// 			Local_Line.erase( Local_Line.begin() +i );
// 			i = -1;
// 		}
// 	}
// 	gpMsgbar->ShowMessage("LocalLine總數刪除後: %d\n", Local_Line.size());

	delete []LocalHistogram;
	////Test/////////////////////////////////////////////////////////////////
	
// 	for (i=0; i<Local_Line.size(); i++)
// 	{
// 		gpMsgbar->ShowMessage("(%d:%d)", s_h-Local_Line[i].pCenter.y, Local_Line[i].nDist);
// 	}
	int nDistConst; 
	if ( nMaxPosH < s_h/3 )
		nDistConst = s_h/4;
	else
		nDistConst = s_h/5;

	double dAccum;
	nDist = 0;
	vector <Segment> SegPos;
	Segment SEG;
	for ( i=0; i<Local_Line.size(); i++)
	{
		dAccum = 0;
		for (j=i; j<Local_Line.size(); j++)
		{
			nDist = abs( Local_Line[i].pCenter.y - Local_Line[j].pCenter.y );
			if ( i != j && nDist < nDistConst )
			{
				dAccum += Local_Line[j].dTotalS;
// 				gpMsgbar->ShowMessage("(%d-%d)%d-%d: %.2f\n", s_h-Local_Line[i].pCenter.y, s_h-Local_Line[j].pCenter.y, i, j, dAccum );
				SEG.nStartH = Local_Line[i].pCenter.y;
				SEG.nEndH   = Local_Line[j].pCenter.y;
				SEG.nCenterX= Local_Line[i].pCenter.x;
				SEG.dTotal = dAccum;
				SEG.Remain = FALSE;
				SegPos.push_back( SEG );
			}
		}
	}
	//sliding windows 找出最大值
	double *SEGBUFFER = new double[ SegPos.size() ];
	memset( SEGBUFFER, 0, sizeof(double)*SegPos.size());
	for (i=0; i<SegPos.size(); i++)
		SEGBUFFER[i] = SegPos[i].dTotal;
	I_BubbleSort( SEGBUFFER, SegPos.size() );

	for (i=0; i<SegPos.size(); i++)
	{
		if ( SegPos[i].dTotal == SEGBUFFER[0] )
		{
			SegPos[i].Remain = TRUE;
		}
	}
// 	gpMsgbar->ShowMessage("~~~~%d\n", SegPos.size());

	for (i=0; i<SegPos.size(); i++)
	{
		if (SegPos[i].Remain == FALSE )
		{
			SegPos.erase( SegPos.begin()+i);
			i = 0;
		}
	}
	gpMsgbar->ShowMessage("~~~~%d\n", SegPos.size());
	for (i=0; i<SegPos.size(); i++)
	{
		if( SegPos[i].Remain == TRUE )
		{
// 			gpMsgbar->ShowMessage("(%d):%d_%d_%.2f\n", SegPos[i].nCenterX, s_h-SegPos[i].nStartH, s_h-SegPos[i].nEndH, SegPos[i].dTotal);
			P1 = CPoint(SegPos[i].nCenterX-35,SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+35,SegPos[i].nEndH+10);

			Range.mUL1 = CPoint(SegPos[i].nCenterX-35,SegPos[i].nStartH-10);
			Range.mRB1 = CPoint(SegPos[i].nCenterX+35,SegPos[i].nEndH+10);
			for (j=0; j<s_h; j++)
			{
				if ( j > P2.y || j < P1.y) HistogramH[j] = 0;
				
			}
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			P1 = CPoint(SegPos[i].nCenterX-34,SegPos[i].nStartH-9);
			P2 = CPoint(SegPos[i].nCenterX+34,SegPos[i].nEndH+9);
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));

			Range.mUL2 = CPoint(SegPos[i].nCenterX-90,SegPos[i].nStartH-10);
			Range.mRB2 = CPoint(SegPos[i].nCenterX+70,SegPos[i].nEndH+80);

			P1 = CPoint(SegPos[i].nCenterX-90,SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+70,SegPos[i].nEndH+80);
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));

			for (k=SegPos[i].nCenterX-35; k<SegPos[i].nCenterX+35; k++)
			{
				for (j=SegPos[i].nStartH-9; j<SegPos[i].nEndH+9; j++)
				{
					HistogramW[k]   = HistogramW[k] + In_ImageNpixel[j*s_w+k];
				}
			}
		}
	}
	if( DISPLAY_SWITCH == TRUE )
	for (i=0; i<SegPos.size(); i++)
	{
		//Test///
		memset( GramR, 0, sizeof(double)*s_h );
		memset( GramL, 0, sizeof(double)*s_h);
	
		for (j=SegPos[i].nStartH-9; j<SegPos[i].nEndH+9; j++)
		{
			for (m=SegPos[i].nCenterX, n= SegPos[i].nCenterX; m<SegPos[i].nCenterX+35; m++, n--)
			{
				GramR[j]   = GramR[j] + In_ImageNpixel[j*s_w+m];
				GramL[j]   = GramL[j] + In_ImageNpixel[j*s_w+n]; 
			}
		}
		for (m=SegPos[i].nStartH-9; m<SegPos[i].nEndH+9; m++)
		{
			P1 = CPoint( SegPos[i].nCenterX,m);
			P2 = CPoint( SegPos[i].nCenterX+ GramR[m]/200 , m);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
			P1 = CPoint( SegPos[i].nCenterX,m);
			P2 = CPoint( SegPos[i].nCenterX- GramL[m]/200 , m);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
		}
	}


	//////////////////////////////////////////////////////////////////////////
	delete []GramR;
	delete []GramL;
	delete []GramSum;
	delete []GramT;
}
void CRogerFunction::MatTransLR( double *ivec , double *Array_Out )
{
	int x, y;
	for (x=0 ; x<4; x++)
	{
		for (y=0; y<4; y++)
		{
			Array_Out[x*16+y*4  ] = - ivec[x*16-y*4+13] ;
			Array_Out[x*16+y*4+1] = - ivec[x*16-y*4+12] ;
			Array_Out[x*16+y*4+2] =   ivec[x*16-y*4+14] ;
			Array_Out[x*16+y*4+3] =   ivec[x*16-y*4+15] ;
		}
	}
// 				Array_Out[0] =  -ivec[13];
// 				Array_Out[1] =  -ivec[12];
// 				Array_Out[2] =   ivec[14];
// 				Array_Out[3] =   ivec[15];
// 				Array_Out[4] =  -ivec[9];
// 				Array_Out[5] =  -ivec[8];
// 				Array_Out[6] =   ivec[10];
// 				Array_Out[7] =   ivec[11];
// 				Array_Out[8] =  -ivec[5];
// 				Array_Out[9] =  -ivec[4];
// 				Array_Out[10] =  ivec[6];
// 				Array_Out[11] =  ivec[7];
// 				Array_Out[12] = -ivec[1];
// 				Array_Out[13] = -ivec[0];
// 				Array_Out[14] =  ivec[2];
// 				Array_Out[15] =  ivec[3];
// 				Array_Out[16] =  -ivec[29];
// 				Array_Out[17] =  -ivec[28];
// 				Array_Out[18] =   ivec[30];
// 				Array_Out[19] =   ivec[31];
// 				Array_Out[20] =  -ivec[25];
// 				Array_Out[21] =  -ivec[24];
// 				Array_Out[22] =   ivec[26];
// 				Array_Out[23] =   ivec[27];
// 				Array_Out[24] =  -ivec[21];
// 				Array_Out[25] =  -ivec[20];
// 				Array_Out[26] =  ivec[22];
// 				Array_Out[27] =  ivec[23];
// 				Array_Out[28] = -ivec[17];
// 				Array_Out[29] = -ivec[16];
// 				Array_Out[30] =  ivec[18];
// 				Array_Out[31] =  ivec[19];
// 				Array_Out[32] =  -ivec[45];
// 				Array_Out[33] =  -ivec[44];
// 				Array_Out[34] =   ivec[46];
// 				Array_Out[35] =   ivec[47];
// 				Array_Out[36] =  -ivec[41];
// 				Array_Out[37] =  -ivec[40];
// 				Array_Out[38] =   ivec[42];
// 				Array_Out[39] =   ivec[43];
// 				Array_Out[40] =  -ivec[37];
// 				Array_Out[41] =  -ivec[36];
// 				Array_Out[42] =  ivec[38];
// 				Array_Out[43] =  ivec[39];
// 				Array_Out[44] = -ivec[33];
// 				Array_Out[45] = -ivec[32];
// 				Array_Out[46] =  ivec[34];
// 				Array_Out[47] =  ivec[35];
// 				Array_Out[48] =  -ivec[61];
// 				Array_Out[49] =  -ivec[60];
// 				Array_Out[50] =   ivec[62];
// 				Array_Out[51] =   ivec[63];
// 				Array_Out[52] =  -ivec[57];
// 				Array_Out[53] =  -ivec[56];
// 				Array_Out[54] =   ivec[58];
// 				Array_Out[55] =   ivec[59];
// 				Array_Out[56] =  -ivec[53];
// 				Array_Out[57] =  -ivec[52];
// 				Array_Out[58] =  ivec[54];
// 				Array_Out[59] =  ivec[55];
// 				Array_Out[60] = -ivec[49];
// 				Array_Out[61] = -ivec[48];
// 				Array_Out[62] =  ivec[50];
// 			    Array_Out[63] =  ivec[51];
}
void CRogerFunction::MatTransUB( double *ivec , double *Array_Out )
{
	int x, y;
	for (x=0 ; x<4; x++)
	{
		for (y=0; y<4; y++)
		{
			Array_Out[x*16+y*4  ] =      ivec[(3-x)*16+y*4] ;
			Array_Out[x*16+y*4+1] =    ivec[(3-x)*16+y*4+1] ;
			Array_Out[x*16+y*4+2] =  - ivec[(3-x)*16+y*4+3] ;
			Array_Out[x*16+y*4+3] =  - ivec[(3-x)*16+y*4+2] ;
        }
	}

// 				Array_Out[0] =  ivec[48];
// 				Array_Out[1] =  ivec[49];
// 				Array_Out[2] =  -ivec[51];
// 				Array_Out[3] =  -ivec[50];
// 				Array_Out[4] =  ivec[52];
// 				Array_Out[5] =  ivec[53];
// 				Array_Out[6] =  -ivec[55];
// 				Array_Out[7] =  -ivec[54];
// 				Array_Out[8] =  ivec[56];
// 				Array_Out[9] =  ivec[57];
// 				Array_Out[10] = -ivec[59];
// 				Array_Out[11] = -ivec[58];
// 				Array_Out[12] = ivec[60];
// 				Array_Out[13] = ivec[61];
// 				Array_Out[14] = -ivec[63];
// 				Array_Out[15] = -ivec[62];
// 				Array_Out[16] = ivec[32];
// 				Array_Out[17] = ivec[33];
// 				Array_Out[18] = -ivec[35];
// 				Array_Out[19] = -ivec[34];
// 				Array_Out[20] = ivec[36];
// 				Array_Out[21] = ivec[37];
// 				Array_Out[22] = -ivec[39];
// 				Array_Out[23] = -ivec[38];
// 				Array_Out[24] = ivec[40];
// 				Array_Out[25] = ivec[41];
// 				Array_Out[26] = -ivec[43];
// 				Array_Out[27] = -ivec[42];
// 				Array_Out[28] = ivec[44];
// 				Array_Out[29] = ivec[45];
// 				Array_Out[30] = -ivec[47];
// 				Array_Out[31] = -ivec[46];
// 				Array_Out[32] = ivec[16];
// 				Array_Out[33] = ivec[17];
// 				Array_Out[34] = -ivec[19];
// 				Array_Out[35] = -ivec[18];
// 				Array_Out[36] = ivec[20];
// 				Array_Out[37] = ivec[21];
// 				Array_Out[38] = -ivec[23];
// 				Array_Out[39] = -ivec[22];
// 				Array_Out[40] = ivec[24];
// 				Array_Out[41] = ivec[25];
// 				Array_Out[42] = -ivec[27];
// 				Array_Out[43] = -ivec[26];
// 				Array_Out[44] = ivec[28];
// 				Array_Out[45] = ivec[29];
// 				Array_Out[46] = -ivec[31];
// 				Array_Out[47] = -ivec[30];
// 				Array_Out[48] = ivec[0];
// 				Array_Out[49] = ivec[1];
// 				Array_Out[50] = -ivec[3];
// 				Array_Out[51] = -ivec[2];
// 				Array_Out[52] = ivec[4];
// 				Array_Out[53] = ivec[5];
// 				Array_Out[54] = -ivec[7];
// 				Array_Out[55] = -ivec[6];
// 				Array_Out[56] = ivec[8];
// 				Array_Out[57] = ivec[9];
// 				Array_Out[58] = -ivec[11];
// 				Array_Out[59] = -ivec[10];
// 				Array_Out[60] = ivec[12];
// 				Array_Out[61] = ivec[13];
// 				Array_Out[62] = -ivec[15];
// 				Array_Out[63] = -ivec[14];
}
void CRogerFunction::MatTransLRUB( double *ivec , double *Array_Out )
{
				Array_Out[0] =  -ivec[61];
				Array_Out[1] =  -ivec[60];
				Array_Out[2] =  -ivec[63];
				Array_Out[3] =  -ivec[62];
				Array_Out[4] =  -ivec[57];
				Array_Out[5] =  -ivec[56];
				Array_Out[6] =  -ivec[59];
				Array_Out[7] =  -ivec[58];
				Array_Out[8] =  -ivec[53];
				Array_Out[9] =  -ivec[52];
				Array_Out[10] = -ivec[55];
				Array_Out[11] = -ivec[54];
				Array_Out[12] = -ivec[49];
				Array_Out[13] = -ivec[48];
				Array_Out[14] = -ivec[51];
				Array_Out[15] = -ivec[50];
				Array_Out[16] = -ivec[45];
				Array_Out[17] = -ivec[44];
				Array_Out[18] = -ivec[47];
				Array_Out[19] = -ivec[46];
				Array_Out[20] = -ivec[41];
				Array_Out[21] = -ivec[40];
				Array_Out[22] = -ivec[43];
				Array_Out[23] = -ivec[42];
				Array_Out[24] = -ivec[37];
				Array_Out[25] = -ivec[36];
				Array_Out[26] = -ivec[39];
				Array_Out[27] = -ivec[38];
				Array_Out[28] = -ivec[33];
				Array_Out[29] = -ivec[32];
				Array_Out[30] = -ivec[35];
				Array_Out[31] = -ivec[34];
				Array_Out[32] = -ivec[29];
				Array_Out[33] = -ivec[28];
				Array_Out[34] = -ivec[31];
				Array_Out[35] = -ivec[30];
				Array_Out[36] = -ivec[25];
				Array_Out[37] = -ivec[24];
				Array_Out[38] = -ivec[27];
				Array_Out[39] = -ivec[26];
				Array_Out[40] = -ivec[21];
				Array_Out[41] = -ivec[20];
				Array_Out[42] = -ivec[23];
				Array_Out[43] = -ivec[22];
				Array_Out[44] = -ivec[17];
				Array_Out[45] = -ivec[16];
				Array_Out[46] = -ivec[19];
				Array_Out[47] = -ivec[18];
				Array_Out[48] = -ivec[13];
				Array_Out[49] = -ivec[12];
				Array_Out[50] = -ivec[15];
				Array_Out[51] = -ivec[14];
				Array_Out[52] = -ivec[9];
				Array_Out[53] = -ivec[8];
				Array_Out[54] = -ivec[11];
				Array_Out[55] = -ivec[10];
				Array_Out[56] = -ivec[5];
				Array_Out[57] = -ivec[4];
				Array_Out[58] = -ivec[7];
				Array_Out[59] = -ivec[6];
				Array_Out[60] = -ivec[1];
				Array_Out[61] = -ivec[0];
				Array_Out[62] = -ivec[3];
				Array_Out[63] = -ivec[2];
}


void CRogerFunction::FindSymmetryAxis(BYTE *input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2){
	CImgProcess FuncImg;
	int i, j, k, l, index1, index2 ,index;
	double localmax;
	BOOL index_switch = FALSE;
	CPoint P1, P2,temp,tempr,templ;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *GramW1 = new double[s_w];
	memset( GramW1, 0, sizeof(double)*s_w);
	double *GramH = new double[s_h];
	memset( GramH, 0, sizeof(double)*s_h);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	int *CenterLinePosX = new int[s_w];
	memset( CenterLinePosX, 0, sizeof(int)*s_w);
	CenterLine tmp;
	MatcherPair tmpMatch;
	NewMatchPair sort_switch;
	vector <CenterLine> Local_Line;//n條對稱軸
	// 	int TOTAL=0;
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CarDetected.vResult.size(); i++) //消除重複點
	{
		for (j=i+1; j<CarDetected.vResult.size(); j++)
		{
			if (  i != j && CarDetected.vResult[i].CenterPoint.x == CarDetected.vResult[j].CenterPoint.x && CarDetected.vResult[i].CenterPoint.y == CarDetected.vResult[j].CenterPoint.y )
			{
				CarDetected.vResult.erase( CarDetected.vResult.begin()+j );
				i--;
			}
		}
	}
	for (i=0; i<CarDetected.vCenterPoint.size(); i++) //超過畫面3/4以上的點捨棄
	{
		if (  CarDetected.vCenterPoint[i].y > s_h*3/4 ) 
		{
			CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+i );
			i--;
		}
	}

	for (i=0; i<CarDetected.vResult.size()-1 ; i++)//排序
		for (j=0; j<CarDetected.vResult.size()-1-i ; j++)
			if (CarDetected.vResult[j].CenterPoint.x > CarDetected.vResult[j+1].CenterPoint.x)
			{
				sort_switch = CarDetected.vResult[j];
				CarDetected.vResult[j] = CarDetected.vResult[j+1];
				CarDetected.vResult[j+1] = sort_switch;
			}
			//	gpMsgbar->ShowMessage("vCenterPointSize:%d\n",CarDetected.vCenterPoint.size());
	for (i=0; i<CarDetected.vResult.size() ; i++){ //histogram
		GramW[CarDetected.vResult[i].CenterPoint.x]++;
				//		gpMsgbar->ShowMessage("( %d , %d ) ",CarDetected.vCenterPoint[i].x,CarDetected.vCenterPoint[i].y);
	}
			//	gpMsgbar->ShowMessage("\n");

	for (i=3; i<s_w-3; i++)//Smooth
		Tmp[i] = GramW[i-3]+GramW[i-2]+GramW[i-1]+3*GramW[i]+GramW[i+1]+GramW[i+2]+GramW[i+3];
	if(DISPLAY_SWITCH2){
		for (i=0;i<s_w;i++){
			P1.x = i;
			P1.y = 0;
			P2.x = i;
			P2.y = Tmp[i]*10;
			//GramW[i] = P2.y;
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
		}
	}

 	memcpy( GramW, Tmp, sizeof(double)*s_w);
// 			I_BubbleSort( Tmp, s_w );

	for(i=0, j=0, index=0, index1=0, index2=0,localmax=0;i<s_w;i++){ //將MatchPair分類
		if(GramW[i]>localmax && index1 != 0){
			localmax = GramW[i];
			index = i;
		}
		if(GramW[i]!=0 && index_switch == FALSE){
			index1 = i;
			index_switch = TRUE;
		}else if (GramW[i]==0 && index_switch == TRUE){
			index2 = i;
			index_switch =FALSE;
			if (index_switch == FALSE){
				//tmp.pCenter.x = (index1 + index2)/2;////////////////
				for(;j<CarDetected.vResult.size();j++)
					if (CarDetected.vResult[j].CenterPoint.x >= index1 && CarDetected.vResult[j].CenterPoint.x <= index2){
						tmpMatch.CenterPoint = CarDetected.vResult[j].CenterPoint;
						tmpMatch.LPoint = CarDetected.vResult[j].Point_L;
						tmpMatch.RPoint = CarDetected.vResult[j].Point_R;
						tmp.MatchPair.push_back(tmpMatch);
					}else
						break;
				tmp.pCenter.x = index;
				Local_Line.push_back(tmp);
				tmp.MatchPairCenter.clear();
				localmax = 0;
			}
		}
	}

	int Dist;
	for ( i=0 ; i<Local_Line.size(); 	i++ ){
		for(j=0;j<Local_Line[i].MatchPair.size();j++){
			Dist = abs( Local_Line[i].MatchPair[j].CenterPoint.x - Local_Line[i].pCenter.x );
			if ( Dist >= 8 ) 
			{
// 				if(i==4)
// 					gpMsgbar->ShowMessage("(%d,%d\n)",CarDetected.vCenterPoint[i].x,CarDetected.vCenterPoint[i].y);
				Local_Line[i].MatchPair.erase( Local_Line[i].MatchPair.begin()+j );
				j = 0;
			}
		}
	}

	for(i=0;i<Local_Line.size();i++){
		Local_Line[i].nMaxY = 0;
		Local_Line[i].nMinY = s_h;
		for (j=0;j<Local_Line[i].MatchPair.size();j++){
			if(Local_Line[i].nMaxY < Local_Line[i].MatchPair[j].CenterPoint.y)
				Local_Line[i].nMaxY = Local_Line[i].MatchPair[j].CenterPoint.y;
			if(Local_Line[i].nMinY > Local_Line[i].MatchPair[j].CenterPoint.y)
				Local_Line[i].nMinY = Local_Line[i].MatchPair[j].CenterPoint.y;
		}
	}
	if(DISPLAY_SWITCH1){
		for (i=0;i<Local_Line.size();i++){//Local_Line.size()對稱軸
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].pCenter.x,0), CPoint(Local_Line[i].pCenter.x, s_h-1), RGB( 255,0,0));
		}
	}
	//統計水平的histogram
	double *GramR = new double[s_h];
	double *GramL = new double[s_h];
	double *GramRT = new double[s_h];
	double *GramLT = new double[s_h];
	double *GramT = new double[s_h];
	double *GramSum = new double[s_h];
	BYTE *p_In_ImageNpixel;
	BYTE *p_In_ImageNpixel2;
	int *p_In_DirectionMap;
	VerPoint tmpVerPoint;//暫存每個垂直histogram的peak
	
// 	int nDist;
// 	Segment SEG;


	for(i=0;i<Local_Line.size();i++){//

		p_In_ImageNpixel = In_ImageNpixel;
		p_In_ImageNpixel2 = In_ImageNpixel2;
		p_In_DirectionMap = In_DirectionMap;
		memset( GramR, 0, sizeof(double)*s_h);
		memset( GramL, 0, sizeof(double)*s_h);
		memset( GramRT, 0, sizeof(double)*s_h);
		memset( GramLT, 0, sizeof(double)*s_h);
		memset( GramT, 0, sizeof(double)*s_h);
		memset( GramSum, 0, sizeof(double)*s_h);
		for (j=0; j<s_h*2/3; j++, p_In_ImageNpixel+=s_w, p_In_ImageNpixel2+=s_w)
		{
			for (k=Local_Line[i].pCenter.x, l= Local_Line[i].pCenter.x; k<Local_Line[i].pCenter.x+30; k++, l--)
			{
				GramR[j]   = GramR[j] + p_In_ImageNpixel[k];
				GramL[j]   = GramL[j] + p_In_ImageNpixel[l];
			}
		}
		for (j=0; j<s_h; j++){
			GramR[j] = GramR[j] / 150;//
			GramL[j] = GramL[j] / 150;//
			GramT[j] = GramL[j] + GramR[j];
		}
		//Smooth
		l = 3;   //60 20
		for (j=l; j<s_h-l; j++)
		{
			for (k=-l; k<l; k++)
			{
				GramSum[j] += GramT[j+k]/l;
				GramRT[j]  += GramR[j+k]/l;
				GramLT[j]  += GramL[j+k]/l;
			}
		} 
		memcpy( GramR, GramRT, sizeof(double)*s_h);
		memcpy( GramL, GramLT, sizeof(double)*s_h);
		for (j=0; j<s_h; j++){
			if ( GramSum[j] < 16 ){
				GramSum[j] = 0;
			}
		}
		for (j=0; j<s_h; j++)
		{
			if ( i > Local_Line[i].nMinY-20 && i < Local_Line[i].nMaxY+20 )
			{
				GramSum[i] = 2 * GramSum[i];
			}
		}
		double MaxV;
		l = 5;
		for (j=0; j<s_h-l; j++){
			MaxV = 0;
			for (k=0; k<l; k++){
				if ( GramSum[j+k] >= MaxV){
					MaxV = GramSum[j+k];
				}
			}
			for (k=0; k<l; k++){
				if (GramSum[j+k] != MaxV)
					GramSum[j+k] = 0;
			}
		}
		for (j=10; j<s_h-10; j++){
			// 			P1 = CPoint( Local_Line[l].pCenter.x,i);
			// 			P2 = CPoint( Local_Line[l].pCenter.x+ GramR[i]/100 , i);
			// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			// 			P1 = CPoint( Local_Line[l].pCenter.x,i);
			// 			P2 = CPoint( Local_Line[l].pCenter.x- GramL[i]/100 , i);
			// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			if(DISPLAY_SWITCH1){
				P1 = CPoint( 0, i);
				P2 = CPoint( GramSum[i]/10, i);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
			}
			if ( GramSum[j] > 0 ){ 
				if(DISPLAY_SWITCH1){
					P1 = CPoint(  Local_Line[i].pCenter.x - 10,  j);
					P2 = CPoint(  Local_Line[i].pCenter.x + 10 , j);
					FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,0));
				}
				tmpVerPoint.BDISPLAY= FALSE;
				tmpVerPoint.dTotalS = GramSum[j];
				tmpVerPoint.pCenter = CPoint( Local_Line[i].pCenter.x, j );
				Local_Line[i].pVer.push_back(tmpVerPoint);
			}
		}
		int nDist;
		int nCnt = 0;
		////////////////////////////////////////////////////////////
		double *LocalHistogram = new double[Local_Line[i].pVer.size()];
		memset( LocalHistogram, 0, sizeof(double)*Local_Line[i].pVer.size());
		//////////////////////////////////////////////////////////////////////////
		for (j=0; j<Local_Line[i].pVer.size(); j++)
		{
			LocalHistogram[j] = Local_Line[i].pVer[j].dTotalS;
		}
		I_BubbleSort( LocalHistogram, Local_Line[i].pVer.size() );

		int nMaxPosH;
		vector <TopPosDist> Top;
		TopPosDist TopInfo;
		vector<VerPoint> Local_pVer;

		for (j=0; j<Local_Line[i].pVer.size(); j++)
		{
			for( k =0; k< 6; k++)//找前12個peak
			{
				if ( Local_Line[i].pVer[j].dTotalS == LocalHistogram[k] )
				{
					Local_Line[i].pVer[j].BDISPLAY = TRUE;
					if (  DISPLAY_SWITCH1 == TRUE  )
					{
						P1 = CPoint( Local_Line[i].pVer[j].pCenter.x-10 ,  Local_Line[i].pVer[j].pCenter.y );
						P2 = CPoint( Local_Line[i].pVer[j].pCenter.x+10  , Local_Line[i].pVer[j].pCenter.y );
						FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
						CarDetected.cMP1 = P1;
					}
					////////////////////////////////////////////////
					nMaxPosH = Local_Line[i].pVer[j].pCenter.y;
					TopInfo.nPoxY = Local_Line[i].pVer[j].pCenter.y;
					TopInfo.nDist = 0;
					TopInfo.Flag  = FALSE;
					Top.push_back( TopInfo );
					Local_pVer.push_back( Local_Line[i].pVer[j] );
				}
			}
		}

		///Top5////////////////////////////////////////////////////////////////////
		// 	for (i=0; i<Top.size(); i++)
		// 	{
		// 		gpMsgbar->ShowMessage("%d: (%d)\n", i, s_h - Top[i]);
		// 	}
		/////Sliding Windows///////////////////////////////////////////////////////////////
		int nDistConst; 
		double dAccum;
		nDist = 0;
		vector <Segment> SegPos;
		Segment SEG;
		for ( j=0; j<Local_pVer.size(); j++)
		{
			dAccum = 0;

			if ( Local_pVer[j].pCenter.y < s_h/4 )//以中心區域的y座標來限制目標高度
				nDistConst = s_h/4;
			else if( Local_pVer[j].pCenter.y >= s_h/4 && Local_pVer[j].pCenter.y < s_h*2/3 )
				nDistConst = s_h/5;
			else if( Local_pVer[j].pCenter.y >= s_h*2/3 && Local_pVer[j].pCenter.y < s_h/6 )
				nDistConst = s_h/6;
			else
				nDistConst = s_h/7;
			// 		gpMsgbar->ShowMessage("slide Y: %d(%d) slide w: %d\n", s_h-Local_LineNew[i].pCenter.y, Local_LineNew[i].pCenter.y, nDistConst );
			for (k=j; k<Local_pVer.size(); k++)//高度太小的區間忽略
			{
				nDist = abs( Local_pVer[j].pCenter.y - Local_pVer[k].pCenter.y );
				if ( k != j && nDist <= nDistConst )
				{
					dAccum += Local_pVer[j].dTotalS;
					SEG.nStartH = Local_pVer[j].pCenter.y;
					SEG.nEndH   = Local_pVer[k].pCenter.y;
					SEG.nCenterX= Local_pVer[j].pCenter.x;
					SEG.dTotal = dAccum;
					SEG.Remain = FALSE;
					Local_Line[i].SegPos.push_back( SEG );
				}
			}
		}

		//sliding windows 找出最大值
		double *SEGBUFFER = new double[ Local_Line[i].SegPos.size() ];
		memset( SEGBUFFER, 0, sizeof(double)*Local_Line[i].SegPos.size());
		for (j=0; j<Local_Line[i].SegPos.size(); j++)
			SEGBUFFER[j] = Local_Line[i].SegPos[j].dTotal;
		I_BubbleSort( SEGBUFFER, Local_Line[i].SegPos.size() );

		for (j=0; j<Local_Line[i].SegPos.size(); j++)
		{
			if ( Local_Line[i].SegPos[j].dTotal == SEGBUFFER[0] )
			{
				Local_Line[i].SegPos[j].Remain = TRUE;
			}
			else
			{
				Local_Line[i].SegPos.erase( Local_Line[i].SegPos.begin() + j );
				j = 0;
			}
		}
		// 	gpMsgbar->ShowMessage("seg: %d\n", SegPos.size());
		for (j=0; j<Local_Line[i].SegPos.size(); j++)
		{
			int test = Local_Line[i].SegPos[j].nStartH;
			if ( Local_Line[i].SegPos[j].nStartH < s_h/4 )
				nDist = s_h/4;
			else if( Local_Line[i].SegPos[j].nStartH >= s_h/4 && Local_Line[i].SegPos[j].nStartH < s_h*2/3 )
				nDist = s_h/5;
			else if( Local_Line[i].SegPos[j].nStartH >= s_h*2/3 && Local_Line[i].SegPos[j].nStartH < s_h/6 )
				nDist = s_h/6;
			else
				nDist = s_h/7;
			// 		nDist = abs(SegPos[i].nEndH - SegPos[i].nStartH);
			// 			nDist *= 1;
			P1 = CPoint(Local_Line[i].SegPos[j].nCenterX-nDist,Local_Line[i].SegPos[j].nStartH);
			P2 = CPoint(Local_Line[i].SegPos[j].nCenterX+nDist,Local_Line[i].SegPos[j].nStartH+nDist);
			CarDetected.LB1 = P1;
			CarDetected.RU1 = P2;
			//nMaxPosH = P2.y - P1.y;
			CString a;
			a.Format("test_%d_%d",i,j);
			SaveImage(input,s_h,s_w,s_bpp,P1,P2,a);

			//FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, CarDetected.LB1, CarDetected.RU1, RGB(0,0,255));

			P1 = CPoint(Local_Line[i].SegPos[j].nCenterX-(nDist*2.4),Local_Line[i].SegPos[j].nStartH-10);
			P2 = CPoint(Local_Line[i].SegPos[j].nCenterX+(nDist*2.4),Local_Line[i].SegPos[j].nEndH+nDist*2.5);
			CarDetected.LB2 = P1;
			CarDetected.RU2 = P2;
		}
	}


	//算出區間後校正對稱軸??
	//

	delete []CenterLinePosX;
	delete []Tmp;
	delete []GramW;
	delete []GramW1;
	delete []GramR;
	delete []GramL;
	delete []GramRT;
	delete []GramLT;
	delete []GramSum;
 	delete []GramT;
}


void CRogerFunction::HistMaxLine( CARINFO &CarDetected, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH)
{
	CImgProcess FuncImg;
	int i, j;
	CPoint P1, P2;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
// 	int TOTAL=0;
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CarDetected.vCenterPoint.size(); i++)
	{
		for (j=i+1; j<CarDetected.vCenterPoint.size(); j++)
		{
			if (  i != j && CarDetected.vCenterPoint[i].x == CarDetected.vCenterPoint[j].x && CarDetected.vCenterPoint[i].y == CarDetected.vCenterPoint[j].y )
			{
				CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+j );
				i = 0;
			}
		}
	}
// 	for (i=0; i<CarDetected.vCenterPoint.size() ; i++)
// 	{
// 		gpMsgbar->ShowMessage("b(%d,%d)D: %d \n", CarDetected.vCenterPoint[i].x,s_h - CarDetected.vCenterPoint[i].y,CarDetected.vCenterPoint[i].y);
	// 	}
	for (i=0; i<CarDetected.vCenterPoint.size(); i++) //超過畫面3/4以上的點捨棄
	{
		if (  CarDetected.vCenterPoint[i].y > s_h*3/4 ) 
		{
			CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+i );
			i = 0;
		}
	}
// 	for (i=0; i<CarDetected.vCenterPoint.size() ; i++)
// 	{
// 		gpMsgbar->ShowMessage("a%d,%d)D: %d \n", CarDetected.vCenterPoint[i].x,s_h - CarDetected.vCenterPoint[i].y,CarDetected.vCenterPoint[i].y);
// 	}
	for (i=0; i<CarDetected.vCenterPoint.size() ; i++) //histogram
	{
		GramW[CarDetected.vCenterPoint[i].x]++;
// 		gpMsgbar->ShowMessage("(%d,%d)D: %d \n", CarDetected.vCenterPoint[i].x,CarDetected.vCenterPoint[i].y,CarDetected.vCenterPoint[i].x);
	}
	for (i=2; i<s_w-2; i++)//Smooth
	{
		Tmp[i] = GramW[i-2]+GramW[i-1]+2*GramW[i]+GramW[i+1]+GramW[i+2];
// 		gpMsgbar->ShowMessage("[(%d):(%.f)]",i,Tmp[i]);
	}
// 	for (i=0; i<CarDetected.vCenterPoint.size() ; i++)
// 	{
// 	 		gpMsgbar->ShowMessage("(%d,%d)D: %d \n", CarDetected.vCenterPoint[i].x,s_h - CarDetected.vCenterPoint[i].y,CarDetected.vCenterPoint[i].x);
// 	}	
	memcpy( GramW, Tmp, sizeof(double)*s_w);
	I_BubbleSort( Tmp, s_w );
	
	for (i=0; i<s_w; i++)
	{
		if ( GramW[i] == Tmp[0] )
		{
			CarDetected.nCenterLinePosX = i;
			if( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( i,0);
				P2 = CPoint( i, s_h-1);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 0,255,0));
			}
			i = s_w;
		}
	}
	//消除偏離點
	int nDist;
	for ( i=0 ; i< CarDetected.vCenterPoint.size(); 
	i++ )
	{
		nDist = abs( CarDetected.vCenterPoint[i].x - CarDetected.nCenterLinePosX );
		if ( nDist >= 8 ) 
		{
			CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+i );
			i = 0;
		}
	}
	int *SortI = new int[CarDetected.vCenterPoint.size()];
	memset( SortI, 0, sizeof(int)*CarDetected.vCenterPoint.size());
	for (i=0; i<CarDetected.vCenterPoint.size(); i++)
	{
		SortI[i] = CarDetected.vCenterPoint[i].y;
	}
	I_BubbleSort( SortI, CarDetected.vCenterPoint.size() );

	//收集中心線附近中心點群聚範圍
	CarDetected.nMaxY = SortI[0];
	CarDetected.nMinY = SortI[CarDetected.vCenterPoint.size()-1];
	delete []SortI;
	delete []Tmp;
	delete []GramW;
}

void CRogerFunction::HistMaxLine_video(BYTE * input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH){
	CImgProcess FuncImg;
	int i, j, k, l, sum, sum1,index1, index2 , CenterLineCounter=0;
	BOOL index_switch = FALSE;
	CPoint P1, P2,temp;
	int *GramW = new int[s_w];
	memset( GramW, 0, sizeof(int)*s_w);
	double *GramW1 = new double[s_w];
	memset( GramW1, 0, sizeof(double)*s_w);
	int *GramH = new int[s_h];
	memset( GramH, 0, sizeof(int)*s_h);
	int *Tmp = new int[s_w];
	memset( Tmp, 0, sizeof(int)*s_w );
	int *CenterLinePosX = new int[s_w];
	memset( CenterLinePosX, 0, sizeof(int)*s_w);
	CenterLine tmp;
	vector <CenterLine> Local_Line;//n條對稱軸

//	gpMsgbar->ShowMessage("消除重複點\n");
	for (i=0; i<CarDetected.vCenterPoint.size(); i++){ //消除重複點
		for (j=0; j<CarDetected.vCenterPoint.size(); j++){
			if (  i != j && CarDetected.vCenterPoint[i].x == CarDetected.vCenterPoint[j].x && CarDetected.vCenterPoint[i].y == CarDetected.vCenterPoint[j].y ){
				//gpMsgbar->ShowMessage("Erease L(%d,%d) C(%d,%d) C(%d,%d)\n",CarDetected.vLeftPoint[j].x,CarDetected.vLeftPoint[j].y,CarDetected.vCenterPoint[j].x,CarDetected.vCenterPoint[j].y,CarDetected.vRightPoint[j].x,CarDetected.vRightPoint[j].y);
				CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+j );
				CarDetected.vLeftPoint.erase( CarDetected.vLeftPoint.begin()+j );
				CarDetected.vRightPoint.erase( CarDetected.vRightPoint.begin()+j );
				j--;
			}
		}
 	}
//	gpMsgbar->ShowMessage("排序\n");

// 	for(i=0;i<CarDetected.vCenterPoint.size();i++)
// 		for(j=0;j<CarDetected.vCenterPoint.size();j++)
// 			if( i!=j && CarDetected.vCenterPoint[i].x > CarDetected.vCenterPoint[j].x){
// 				temp = CarDetected.vCenterPoint[i];
// 				CarDetected.vCenterPoint[i] = CarDetected.vCenterPoint[j];
// 				CarDetected.vCenterPoint[j] = temp;
// 				temp = CarDetected.vLeftPoint[i];
// 				CarDetected.vLeftPoint[i] = CarDetected.vLeftPoint[j];
// 				CarDetected.vLeftPoint[j] = temp;
// 				temp = CarDetected.vRightPoint[i];
// 				CarDetected.vRightPoint[i] = CarDetected.vRightPoint[j];
// 				CarDetected.vRightPoint[j] = temp;
// 			}

// 	gpMsgbar->ShowMessage("MatchPair:\n");
// 	for(j=0;j<CarDetected.vCenterPoint.size();j++){
// 		gpMsgbar->ShowMessage("\tL(%d,%d) C(%d,%d) C(%d,%d)\n",CarDetected.vLeftPoint[j].x,CarDetected.vLeftPoint[j].y,CarDetected.vCenterPoint[j].x,CarDetected.vCenterPoint[j].y,CarDetected.vRightPoint[j].x,CarDetected.vRightPoint[j].y);
// 	}

//			gpMsgbar->ShowMessage("histogram累計\n");
	for (i=0; i<CarDetected.vCenterPoint.size() ; i++) //histogram
		GramW[CarDetected.vCenterPoint[i].x]++;
//	gpMsgbar->ShowMessage("histogram smooth\n");
	for (i=5; i<s_w-5; i++)//Smooth
		Tmp[i] = GramW[i-4]+GramW[i-3]+GramW[i-2]+GramW[i-1]+3*GramW[i]+GramW[i+1]+GramW[i+2]+GramW[i+3]+GramW[i+4];

	memcpy( GramW, Tmp, sizeof(int)*s_w);
// 	for(i=0;i<s_w;i++){
// 		P1 = CPoint(  i, 0);
// 		P2 = CPoint(  i, GramW[i]*10);
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
// 	}
 	k=10;
	int check_max;
	int k2 = 2*k - 1;
// 	for(i=k;i<s_w-k;i++){
// 		check_max = 0;
// 		for(j=-k;j<k;j++){
// 			if(GramW[i]>GramW[i+j] && GramW[i] != 0){
// 				check_max++;
// 			}else if(i == i+j){
// 			}else
// 				break;
// 		}
// 		if(check_max == k2){
// 			P1 = CPoint(  i,  0);
// 			P2 = CPoint(  i , s_h-1);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
// 			i+=(k+1);
// 		}
// 	}



// 	I_BubbleSort( Tmp, s_w );
//	gpMsgbar->ShowMessage("分類MatchPair\n");
	int localmax,index;
	sum = 0;
	for(i=0, j=0, index=0, index1=0, index2=0, localmax = 0;i<s_w;i++){ //將MatchPair分類
		if(GramW[i]>localmax && index1 != 0){
			localmax = GramW[i];
			index = i;
			if(sum<localmax)
				sum = localmax;
		}
		if(GramW[i]!=0 && index_switch == FALSE){//開始
			index1 = i;
			index_switch = TRUE;
			localmax = GramW[i];
		}else if (GramW[i]==0 && index_switch == TRUE){//結尾
			index2 = i;
			index_switch = FALSE;
			//gpMsgbar->ShowMessage("MatchPair total:%d\n",CarDetected.vCenterPoint.size());
			for(j=0;j<CarDetected.vCenterPoint.size();j++)//把matchpair加入對稱軸
				if (CarDetected.vCenterPoint[j].x >= index1 && CarDetected.vCenterPoint[j].x <= index2){
					//gpMsgbar->ShowMessage("\t%d L(%d,%d) C(%d,%d) C(%d,%d) 加入%d~%d區間\n",i,CarDetected.vLeftPoint[j].x,CarDetected.vLeftPoint[j].y,CarDetected.vCenterPoint[j].x,CarDetected.vCenterPoint[j].y,CarDetected.vRightPoint[j].x,CarDetected.vRightPoint[j].y,index1,index2);
					tmp.MatchPairCenter.push_back(CarDetected.vCenterPoint[j]);
					MatcherPair _matcherpair;
					_matcherpair.CenterPoint = CarDetected.vCenterPoint[j];
					_matcherpair.LPoint = CarDetected.vLeftPoint[j];
					_matcherpair.RPoint = CarDetected.vRightPoint[j];
					_matcherpair.distance = FuncImg.FindDistance(_matcherpair.LPoint,_matcherpair.RPoint);
					tmp.MatchPair.push_back(_matcherpair);
				}
				tmp.pCenter.x = index;
				tmp.MatchPairSize=localmax;
				Local_Line.push_back(tmp);
				tmp.MatchPairCenter.clear();
				tmp.MatchPair.clear();
				localmax = 0;
				index1=0;
				index2=0;
		}
	}


		sum /= 2;
		for (i=0 ;i<Local_Line.size();i++)
		{
			if(Local_Line[i].MatchPairSize < sum){
				Local_Line.erase(Local_Line.begin()+i);//
				i--;
			}
		}
	//算重心
	/////////////////////////
// 	for(i=0;i<Local_Line.size();i++){
// 		memset(GramH,0,sizeof(int)*s_h);
// 		memset(Tmp,0,sizeof(int)*s_h);
// 		for(j=0;j<Local_Line[i].MatchPair.size();j++)
// 			GramH[Local_Line[i].MatchPair[j].CenterPoint.y]++;
// 		for (j=20; j<s_h-20; j++)//Smooth
// 			for(k=-20;k<20;k++)
// 				Tmp[j] += GramH[j+k];
// 		memcpy( GramH, Tmp, sizeof(int)*s_h);
// 		for(j=0;j<s_h;j++){
// 			if(GramH[j]!=0){
// 				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].pCenter.x,j), CPoint(Local_Line[i].pCenter.x+GramH[j]*20,j), RGB(0,255,255));
// 			}	
// 		}
// 	}

	/////////////////////////
	for(i=0;i<Local_Line.size();i++){
		//gpMsgbar->ShowMessage("Line:%d MatchPair total:%d\n",i,Local_Line[i].MatchPair.size());
		sum = 0;
		sum1 = 0;
		for(j=0;j<Local_Line[i].MatchPair.size();j++){
			//gpMsgbar->ShowMessage("\tPoint[%d]:L(%d,%d) C(%d,%d) R(%d,%d)\n",j,Local_Line[i].MatchPair[j].LPoint.x,Local_Line[i].MatchPair[j].LPoint.y,Local_Line[i].MatchPair[j].CenterPoint.x,Local_Line[i].MatchPair[j].CenterPoint.y,Local_Line[i].MatchPair[j].RPoint.x,Local_Line[i].MatchPair[j].RPoint.y);
			sum+=Local_Line[i].MatchPair[j].CenterPoint.x;
			sum1+=Local_Line[i].MatchPair[j].CenterPoint.y;
		}
		//gpMsgbar->ShowMessage(" x:%d y:%d\n",sum,sum1/Local_Line[i].MatchPair.size());
		sum/=Local_Line[i].MatchPair.size();
		sum1/=Local_Line[i].MatchPair.size();
		Local_Line[i].Fousc.x=sum;
		Local_Line[i].Fousc.y=sum1;
		//FuncImg.DrawCross(Display_ImageNsize, s_w, s_h, s_bpp, CPoint(sum,sum1), 20, RGB(255,0,255),0);
		//gpMsgbar->ShowMessage("\tFousc:(%d,%d)\n",Local_Line[i].Fousc.x,Local_Line[i].Fousc.y);
	}

	double *GramR = new double[s_h];
	double *GramL = new double[s_h];
	double *GramRT = new double[s_h];
	double *GramLT = new double[s_h];
	int *GramSum = new int[s_h];
	double *GramT = new double[s_h];
	int *GramTmp = new int[s_h];
	//int GramTmp[360];
	BYTE *p_In_ImageNpixel = In_ImageNpixel;
//	BYTE *p_In_ImageNpixel2 = In_ImageNpixel2;
	int *p_In_DirectionMap= In_DirectionMap;
	VerPoint LocalMax;
	int ww;

	int peakposition[30]={0};
// 	p_In_ImageNpixel+=s_w*60;
// 	memset(GramW,0,sizeof(int)*s_w);
// 	for(i=60;i<s_h*2/3;i++,p_In_ImageNpixel+=s_w){
// 		for (j=0;j<s_w;j++){//左減右
//			int SumG = 0;
//			SumG += abs(p_In_ImageNpixel[j-1] - p_In_ImageNpixel[j+1]);
// 			GramW[j] += p_In_ImageNpixel[j];
// 		}
// 	}

// 		for (i=0;i<s_w;i++){
// 			P1 = CPoint(i,0);
// 	 		P2 = CPoint(i,GramW[i]/100);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
// 		}

	for(l=0;l<Local_Line.size();l++){//
		//p_In_ImageNpixel = In_ImageNpixel;
		p_In_ImageNpixel = In_ImageNpixel2;
		//p_In_ImageNpixel2 = In_ImageNpixel2;
		p_In_DirectionMap = In_DirectionMap;
//		Local_Line[l].HistDensityH = new double[s_h];
// 		memset( GramR, 0, sizeof(double)*s_h);
// 		memset( GramL, 0, sizeof(double)*s_h);
		memset( GramRT, 0, sizeof(double)*s_h);
		memset( GramLT, 0, sizeof(double)*s_h);
		memset( GramSum, 0, sizeof(int)*s_h);
		memset( GramT, 0, sizeof(double)*s_h);
//		memset( Local_Line[l].HistDensityH, 0, sizeof(double)*s_h);
		memset( GramTmp, 0, sizeof(int)*s_h);
		ww = s_w*5;
 		p_In_ImageNpixel+=s_w*5;
// 		for (i=60; i<s_h*2/3; i++, p_In_ImageNpixel+=s_w)	//水平HISTOGRAM統計
		for (i=5; i<s_h-5; i++, p_In_ImageNpixel+=s_w)
		{
			int sumG = 0;
			for (j=k=Local_Line[l].pCenter.x; j<Local_Line[l].pCenter.x+20; j++, k--){
				sumG += abs(p_In_ImageNpixel[j-ww] - p_In_ImageNpixel[j+ww]);
				sumG += abs(p_In_ImageNpixel[k-ww] - p_In_ImageNpixel[k+ww]);
				GramSum[i]+=p_In_ImageNpixel[j] + p_In_ImageNpixel[k];
				// 				GramR[i]   = GramR[i] + p_In_ImageNpixel[j];
				// 				GramL[i]   = GramL[i] + p_In_ImageNpixel[k];
				// 				Local_Line[l].HistDensityH[i] += (p_In_ImageNpixel[j] + p_In_ImageNpixel[k])/60;
			}
			GramTmp[i] = sumG; 
		}
		//smooth
		k=7;
		k2 = 2*k-1;
		for(i=k;i<s_h-k;i++){
			for(j=-k;j<k;j++){
				GramH[i]+=GramTmp[i+j];
			}
			GramH[i]/=k2;
		}


// 		for (i=0;i<s_h;i++){
// 			P1 = CPoint(  Local_Line[l].pCenter.x,  i);
// 			P2 = CPoint(  Local_Line[l].pCenter.x + GramH[i]/50 , i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
// 			//gpMsgbar->ShowMessage("%d %d %d\n",l,i,GramSum[i]);
// 		}


		for(i=0,sum=0,sum1=0;i<s_h;i++){
			sum+=GramTmp[i];
			sum1+=GramSum[i];
		}
		int avg = sum/s_h;
		//avg*=1.2;
		k=10;
		k2 = 2*k - 1;
		int peaktotal[50];
		sum = 0;
		for(i=k;i<s_h-k;i++){
			check_max = 0;

			for(j=-k;j<k;j++){
				if(GramH[i]>GramH[i+j] && GramH[i] != 0){
					check_max++;
				}else if(i == i+j){
				}else
					break;
			}
			if(check_max == k2){
				if(GramH[i]>avg){
					if(DISPLAY_SWITCH1 == TRUE){
						P1 = CPoint(  Local_Line[l].pCenter.x - 30,  i);
						P2 = CPoint(  Local_Line[l].pCenter.x + 30 , i);
						FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
					}
					peaktotal[sum]=i;
					sum++;
				}
				i+=(k+1);
			}
		}
		Local_Line[l].HorizonPeak = new int[sum];
		Local_Line[l].PeakTotal = sum;
		memcpy(Local_Line[l].HorizonPeak,peaktotal,sizeof(int)*(sum));

		//陰影
		avg = sum1 /= s_h;
		avg/=3;
		int shadowcount = 0;
		index1 = 0;
		index2 = 0;
		CPoint Shadow_detection[50];
		for(i=0;i<s_h;i++){
			if(GramSum[i] < avg){
// 				P1 = CPoint(  Local_Line[l].pCenter.x - 15,  i);
// 				P2 = CPoint(  Local_Line[l].pCenter.x + 15 , i);
// 				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
				if(index1 == 0 && index2 ==0){
					index1 = index2 =i;
				}else if(i == index2 + 1 && index1 != 0){
					index2 = i;
				}
			}else if(index1 != 0 && index2 !=0){
				Shadow_detection[shadowcount].x = index1;
				Shadow_detection[shadowcount].y = index2;
				shadowcount++;
				index1=0;
				index2=0;
			}
		}
//		gpMsgbar->ShowMessage("total:%d\n",shadowcount);
		sum = 0;
		sum1 = 0;
		int maxshadow = 0;
		int maxshadow1 = 0;
		for(i=0;i<shadowcount;i++){//find maxshadow
//			gpMsgbar->ShowMessage("%d (%d,%d)\n",i,Shadow_detection[i].x,Shadow_detection[i].y);
			if(abs(Shadow_detection[i].x - Shadow_detection[i].y) > sum){
				maxshadow = i;
				sum = abs(Shadow_detection[i].x - Shadow_detection[i].y);
			}
		}
		for(i=0;i<shadowcount;i++){//find second maxshadow
			if(abs(Shadow_detection[i].x - Shadow_detection[i].y) > sum1 && i != maxshadow){
				maxshadow1 = i;
				sum1 = abs(Shadow_detection[i].x - Shadow_detection[i].y);
			}
		}

		Local_Line[l].nShadow[0] = Shadow_detection[maxshadow];
		Local_Line[l].nShadow[1] = Shadow_detection[maxshadow1];
		if(Local_Line[l].nShadow[0].y > Local_Line[l].nShadow[1].y){
			CPoint a = Local_Line[l].nShadow[0];
			Local_Line[l].nShadow[0] = Local_Line[l].nShadow[1];
			Local_Line[l].nShadow[1] = a;
		}

		//histgrom
// 		for(i=0;i<s_h;i++){
// 			P1 = CPoint(  Local_Line[l].pCenter.x,  i);
// 			P2 = CPoint(  Local_Line[l].pCenter.x + GramSum[i]/150 , i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
// 		}


//		gpMsgbar->ShowMessage("%d:max shadow:%d maxshadow1:%d",l,maxshadow,maxshadow1);
		if(DISPLAY_SWITCH1 == TRUE){
			for(i=0, j=Shadow_detection[maxshadow].x ; i<sum ;i++,j++){
				// 			P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				// 			P2 = CPoint(  Local_Line[l].pCenter.x + 15 , j);
				// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));

				P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x , j);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
				P1 = CPoint(  Local_Line[l].pCenter.x,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x + GramSum[j]/150 , j);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			}
			P1 = CPoint(  Local_Line[l].pCenter.x - 100,  j);
			P2 = CPoint(  Local_Line[l].pCenter.x + 100 , j);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
			for(i=0, j=Shadow_detection[maxshadow1].x ; i<sum1 ;i++,j++){
				// 			P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				// 			P2 = CPoint(  Local_Line[l].pCenter.x + 15 , j);
				// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
				P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x , j);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
				P1 = CPoint(  Local_Line[l].pCenter.x,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x + GramSum[j]/150 , j);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
			}
			P1 = CPoint(  Local_Line[l].pCenter.x - 100,  j);
			P2 = CPoint(  Local_Line[l].pCenter.x + 100 , j);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		}
	}
	//引擎蓋 5組水平邊
	//找陰影上5組水平邊中間隔最大的
	
	for(i=0;i<Local_Line.size();i++){
		index1=0;
		index2=0;
		int count = 0;
		index_switch = false;
		//for(j=0;j<2;j++){
			for(j=0;j<Local_Line[i].PeakTotal;j++){
				if(Local_Line[i].HorizonPeak[j]>Local_Line[i].nShadow[0].y && index_switch == false){
					index_switch = true;
					index1 = j;
				}
				if(Local_Line[i].HorizonPeak[j]>Local_Line[i].nShadow[1].y && index_switch == true){
					index2 = j;
					j = Local_Line[i].PeakTotal;
				}
			}
			int max1=0;
			int max2=0;
			int localmax1 = 0;
			int localmax2 = 0;
			for(k=index1+1;k<index1+4 && (k+1)<Local_Line[i].PeakTotal;k++){			
				int t = Local_Line[i].HorizonPeak[k+1] - Local_Line[i].HorizonPeak[k];
				if(max1 < t){
					max2 = max1;
					max1 = t;
					localmax2 = localmax1;
					localmax1 = k+1;
				}else if(max2 < t){
					max2 = t;
					localmax2 = k+1;
				}
			}

// 				ww = (Local_Line[i].HorizonPeak[localmax1] - Local_Line[i].nShadow[0].y)/4*3;
//  			P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[0].y);
//  			P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax1]);
// 				FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,255,0));
				Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[0].y;
				Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax1];

// 				ww = (Local_Line[i].HorizonPeak[localmax2] - Local_Line[i].nShadow[0].y)/4*3;
// 				P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[0].y);
// 				P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax2]);
// 				FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,255,0));
				Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[0].y;
				Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax2];
//			}


			max1=0;
			max2=0;
			localmax1 = 0;
			localmax2 = 0;
			for(k=index2+1;k<index2+4 && (k+1)<Local_Line[i].PeakTotal;k++ ){
				int t = Local_Line[i].HorizonPeak[k+1] - Local_Line[i].HorizonPeak[k];
				if(max1 < t){
					max2 = max1;
					max1 = t;
					localmax2 = localmax1;
					localmax1 = k+1;
				}else if(max2 < t){
					max2 = t;
					localmax2 = k+1;
				}
				
			}

// 			ww = (Local_Line[i].HorizonPeak[localmax1] - Local_Line[i].nShadow[1].y)/4*3;
// 			P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[1].y);
// 			P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax1]);
// 			FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,0,255));
			Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[1].y;
			Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax1];

// 			ww = (Local_Line[i].HorizonPeak[localmax2] - Local_Line[i].nShadow[1].y)/4*3;
// 			P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[1].y);
// 			P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax2]);
// 			FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,0,255));
			Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[1].y;
			Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax2];
//			}
		//}
		
	}


	if(DISPLAY_SWITCH1 == TRUE){
		for (i=0;i<Local_Line.size();i++){//對稱軸
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].pCenter.x,0), CPoint(Local_Line[i].pCenter.x, s_h-1), RGB( 255,0,0));
		}
	}
	CarDetected.Local_Line = Local_Line;
	

	delete []CenterLinePosX;
	delete []Tmp;
	delete []GramW;
	delete []GramW1;
	delete []GramR;
	delete []GramL;
	delete []GramRT;
	delete []GramLT;
	delete []GramSum;
	delete []GramT;
	delete []GramH;
	delete []GramTmp;
}

void CRogerFunction::HistMaxLine_video1(BYTE * input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, int Width, int Height, CPoint ROI_LB, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH, BOOL USING_ROI, BOOL USINGSMALLFRAME){
	CImgProcess FuncImg;
	int i, j, k, l, sum, sum1,index1, index2 , CenterLineCounter=0;
	BOOL index_switch = FALSE;
	CPoint P1, P2,temp,drawP1,drawP2;
	int *GramW = new int[s_w];
	memset( GramW, 0, sizeof(int)*s_w);
	double *GramW1 = new double[s_w];
	memset( GramW1, 0, sizeof(double)*s_w);
	int *GramH = new int[s_h];
	memset( GramH, 0, sizeof(int)*s_h);
	int *Tmp = new int[s_w];
	memset( Tmp, 0, sizeof(int)*s_w );
	int *CenterLinePosX = new int[s_w];
	memset( CenterLinePosX, 0, sizeof(int)*s_w);
	CenterLine tmp;
	vector <CenterLine> Local_Line;//n條對稱軸

	//	gpMsgbar->ShowMessage("消除重複點\n");
	for (i=0; i<CarDetected.vCenterPoint.size(); i++){ //消除重複點
		for (j=0; j<CarDetected.vCenterPoint.size(); j++){
			if (  i != j && CarDetected.vCenterPoint[i].x == CarDetected.vCenterPoint[j].x && CarDetected.vCenterPoint[i].y == CarDetected.vCenterPoint[j].y ){
				//gpMsgbar->ShowMessage("Erease L(%d,%d) C(%d,%d) C(%d,%d)\n",CarDetected.vLeftPoint[j].x,CarDetected.vLeftPoint[j].y,CarDetected.vCenterPoint[j].x,CarDetected.vCenterPoint[j].y,CarDetected.vRightPoint[j].x,CarDetected.vRightPoint[j].y);
				CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+j );
				CarDetected.vLeftPoint.erase( CarDetected.vLeftPoint.begin()+j );
				CarDetected.vRightPoint.erase( CarDetected.vRightPoint.begin()+j );
				j--;
			}
		}
	}

	for (i=0; i<CarDetected.vCenterPoint.size(); i++){ //消除重複點
		for (j=0; j<CarDetected.vCenterPoint.size(); j++){
			if (  i != j && PtInCircle(CarDetected.vCenterPoint[i],CarDetected.vCenterPoint[j],Height/10)){
				if(DISPLAY_SWITCH1 == TRUE){
					P1 = CarDetected.vCenterPoint[i];
					P2 = CarDetected.vCenterPoint[j];
					if(USING_ROI){
						P1.x += ROI_LB.x;
						P1.y += ROI_LB.y;
						P2.x += ROI_LB.x;
						P2.y += ROI_LB.y;
					}
					if(USINGSMALLFRAME){
						P1.x*=2;
						P1.y*=2;
						P2.x*=2;
						P2.y*=2;
						FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(0,0,0));
					}else
						FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(0,0,0));


				}
			}
		}
	}
	//	gpMsgbar->ShowMessage("排序\n");

	// 	for(i=0;i<CarDetected.vCenterPoint.size();i++)
	// 		for(j=0;j<CarDetected.vCenterPoint.size();j++)
	// 			if( i!=j && CarDetected.vCenterPoint[i].x > CarDetected.vCenterPoint[j].x){
	// 				temp = CarDetected.vCenterPoint[i];
	// 				CarDetected.vCenterPoint[i] = CarDetected.vCenterPoint[j];
	// 				CarDetected.vCenterPoint[j] = temp;
	// 				temp = CarDetected.vLeftPoint[i];
	// 				CarDetected.vLeftPoint[i] = CarDetected.vLeftPoint[j];
	// 				CarDetected.vLeftPoint[j] = temp;
	// 				temp = CarDetected.vRightPoint[i];
	// 				CarDetected.vRightPoint[i] = CarDetected.vRightPoint[j];
	// 				CarDetected.vRightPoint[j] = temp;
	// 			}

	// 	gpMsgbar->ShowMessage("MatchPair:\n");
	// 	for(j=0;j<CarDetected.vCenterPoint.size();j++){
	// 		gpMsgbar->ShowMessage("\tL(%d,%d) C(%d,%d) C(%d,%d)\n",CarDetected.vLeftPoint[j].x,CarDetected.vLeftPoint[j].y,CarDetected.vCenterPoint[j].x,CarDetected.vCenterPoint[j].y,CarDetected.vRightPoint[j].x,CarDetected.vRightPoint[j].y);
	// 	}

	//			gpMsgbar->ShowMessage("histogram累計\n");
	for (i=0; i<CarDetected.vCenterPoint.size() ; i++) //histogram
		GramW[CarDetected.vCenterPoint[i].x]++;
	//	gpMsgbar->ShowMessage("histogram smooth\n");
	for (i=5; i<s_w-5; i++)//Smooth
		Tmp[i] = GramW[i-4]+GramW[i-3]+GramW[i-2]+GramW[i-1]+3*GramW[i]+GramW[i+1]+GramW[i+2]+GramW[i+3]+GramW[i+4];

	memcpy( GramW, Tmp, sizeof(int)*s_w);
	// 	for(i=0;i<s_w;i++){
	// 		P1 = CPoint(  i, 0);
	// 		P2 = CPoint(  i, GramW[i]*10);
	// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	// 	}
	k=10;
	int check_max;
	int k2 = 2*k - 1;
	



	// 	I_BubbleSort( Tmp, s_w );
	//	gpMsgbar->ShowMessage("分類MatchPair\n");
	int localmax,index;
	sum = 0;
	for(i=0, j=0, index=0, index1=0, index2=0, localmax = 0;i<s_w;i++){ //將MatchPair分類
		if(GramW[i]>localmax && index1 != 0){
			localmax = GramW[i];
			index = i;
			if(sum<localmax)
				sum = localmax;
		}
		if(GramW[i]!=0 && index_switch == FALSE){//開始
			index1 = i;
			index_switch = TRUE;
			localmax = GramW[i];
		}else if (GramW[i]==0 && index_switch == TRUE){//結尾
			index2 = i;
			index_switch = FALSE;
			//gpMsgbar->ShowMessage("MatchPair total:%d\n",CarDetected.vCenterPoint.size());
			for(j=0;j<CarDetected.vCenterPoint.size();j++)//把matchpair加入對稱軸
				if (CarDetected.vCenterPoint[j].x >= index1 && CarDetected.vCenterPoint[j].x <= index2){
					//gpMsgbar->ShowMessage("\t%d L(%d,%d) C(%d,%d) C(%d,%d) 加入%d~%d區間\n",i,CarDetected.vLeftPoint[j].x,CarDetected.vLeftPoint[j].y,CarDetected.vCenterPoint[j].x,CarDetected.vCenterPoint[j].y,CarDetected.vRightPoint[j].x,CarDetected.vRightPoint[j].y,index1,index2);
					tmp.MatchPairCenter.push_back(CarDetected.vCenterPoint[j]);
					MatcherPair _matcherpair;
					_matcherpair.CenterPoint = CarDetected.vCenterPoint[j];
					_matcherpair.LPoint = CarDetected.vLeftPoint[j];
					_matcherpair.RPoint = CarDetected.vRightPoint[j];
					_matcherpair.distance = FuncImg.FindDistance(_matcherpair.LPoint,_matcherpair.RPoint);
					tmp.MatchPair.push_back(_matcherpair);
				}
				tmp.pCenter.x = index;
				tmp.MatchPairSize=localmax;
				Local_Line.push_back(tmp);
				tmp.MatchPairCenter.clear();
				tmp.MatchPair.clear();
				localmax = 0;
				index1=0;
				index2=0;
		}
	}


	sum /= 2;
	for (i=0 ;i<Local_Line.size();i++)
	{
		if(Local_Line[i].MatchPairSize < sum){
			Local_Line.erase(Local_Line.begin()+i);//
			i--;
		}
	}
	//算重心
	/////////////////////////
	// 	for(i=0;i<Local_Line.size();i++){
	// 		memset(GramH,0,sizeof(int)*s_h);
	// 		memset(Tmp,0,sizeof(int)*s_h);
	// 		for(j=0;j<Local_Line[i].MatchPair.size();j++)
	// 			GramH[Local_Line[i].MatchPair[j].CenterPoint.y]++;
	// 		for (j=20; j<s_h-20; j++)//Smooth
	// 			for(k=-20;k<20;k++)
	// 				Tmp[j] += GramH[j+k];
	// 		memcpy( GramH, Tmp, sizeof(int)*s_h);
	// 		for(j=0;j<s_h;j++){
	// 			if(GramH[j]!=0){
	// 				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].pCenter.x,j), CPoint(Local_Line[i].pCenter.x+GramH[j]*20,j), RGB(0,255,255));
	// 			}	
	// 		}
	// 	}

	/////////////////////////
	for(i=0;i<Local_Line.size();i++){
		//gpMsgbar->ShowMessage("Line:%d MatchPair total:%d\n",i,Local_Line[i].MatchPair.size());
		sum = 0;
		sum1 = 0;
		for(j=0;j<Local_Line[i].MatchPair.size();j++){
			//gpMsgbar->ShowMessage("\tPoint[%d]:L(%d,%d) C(%d,%d) R(%d,%d)\n",j,Local_Line[i].MatchPair[j].LPoint.x,Local_Line[i].MatchPair[j].LPoint.y,Local_Line[i].MatchPair[j].CenterPoint.x,Local_Line[i].MatchPair[j].CenterPoint.y,Local_Line[i].MatchPair[j].RPoint.x,Local_Line[i].MatchPair[j].RPoint.y);
			sum+=Local_Line[i].MatchPair[j].CenterPoint.x;
			sum1+=Local_Line[i].MatchPair[j].CenterPoint.y;
		}
		//gpMsgbar->ShowMessage(" x:%d y:%d\n",sum,sum1/Local_Line[i].MatchPair.size());
		sum/=Local_Line[i].MatchPair.size();
		sum1/=Local_Line[i].MatchPair.size();
		Local_Line[i].Fousc.x=sum;
		Local_Line[i].Fousc.y=sum1;
		//FuncImg.DrawCross(Display_ImageNsize, s_w, s_h, s_bpp, CPoint(sum,sum1), 20, RGB(255,0,255),0);
		//gpMsgbar->ShowMessage("\tFousc:(%d,%d)\n",Local_Line[i].Fousc.x,Local_Line[i].Fousc.y);
	}

	double *GramR = new double[s_h];
	double *GramL = new double[s_h];
	double *GramRT = new double[s_h];
	double *GramLT = new double[s_h];
	int *GramSum = new int[s_h];
	double *GramT = new double[s_h];
	int *GramTmp = new int[s_h];
	//int GramTmp[360];
	BYTE *p_In_ImageNpixel = In_ImageNpixel;
	//	BYTE *p_In_ImageNpixel2 = In_ImageNpixel2;
	int *p_In_DirectionMap= In_DirectionMap;
	VerPoint LocalMax;
	int ww;

	int peakposition[30]={0};
	// 	p_In_ImageNpixel+=s_w*60;
	// 	memset(GramW,0,sizeof(int)*s_w);
	// 	for(i=60;i<s_h*2/3;i++,p_In_ImageNpixel+=s_w){
	// 		for (j=0;j<s_w;j++){//左減右
	//			int SumG = 0;
	//			SumG += abs(p_In_ImageNpixel[j-1] - p_In_ImageNpixel[j+1]);
	// 			GramW[j] += p_In_ImageNpixel[j];
	// 		}
	// 	}

	// 		for (i=0;i<s_w;i++){
	// 			P1 = CPoint(i,0);
	// 	 		P2 = CPoint(i,GramW[i]/100);
	// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
	// 		}

	for(l=0;l<Local_Line.size();l++){//
		//p_In_ImageNpixel = In_ImageNpixel;
		p_In_ImageNpixel = In_ImageNpixel2;
		//p_In_ImageNpixel2 = In_ImageNpixel2;
		p_In_DirectionMap = In_DirectionMap;
		//		Local_Line[l].HistDensityH = new double[s_h];
		// 		memset( GramR, 0, sizeof(double)*s_h);
		// 		memset( GramL, 0, sizeof(double)*s_h);
		memset( GramRT, 0, sizeof(double)*s_h);
		memset( GramLT, 0, sizeof(double)*s_h);
		memset( GramSum, 0, sizeof(int)*s_h);
		memset( GramT, 0, sizeof(double)*s_h);
		//		memset( Local_Line[l].HistDensityH, 0, sizeof(double)*s_h);
		memset( GramTmp, 0, sizeof(int)*s_h);
		ww = s_w*5;
		p_In_ImageNpixel+=s_w*5;
		// 		for (i=60; i<s_h*2/3; i++, p_In_ImageNpixel+=s_w)	//水平HISTOGRAM統計
		for (i=5; i<s_h-5; i++, p_In_ImageNpixel+=s_w)
		{
			int sumG = 0;
			for (j=k=Local_Line[l].pCenter.x; j<Local_Line[l].pCenter.x+20; j++, k--){
				sumG += abs(p_In_ImageNpixel[j-ww] - p_In_ImageNpixel[j+ww]);
				sumG += abs(p_In_ImageNpixel[k-ww] - p_In_ImageNpixel[k+ww]);
				GramSum[i]+=p_In_ImageNpixel[j] + p_In_ImageNpixel[k];
				// 				GramR[i]   = GramR[i] + p_In_ImageNpixel[j];
				// 				GramL[i]   = GramL[i] + p_In_ImageNpixel[k];
				// 				Local_Line[l].HistDensityH[i] += (p_In_ImageNpixel[j] + p_In_ImageNpixel[k])/60;
			}
			GramTmp[i] = sumG; 
		}
		//smooth
		k=7;
		k2 = 2*k-1;
		for(i=k;i<s_h-k;i++){
			for(j=-k;j<k;j++){
				GramH[i]+=GramTmp[i+j];
			}
			GramH[i]/=k2;
		}


		// 		for (i=0;i<s_h;i++){
		// 			P1 = CPoint(  Local_Line[l].pCenter.x,  i);
		// 			P2 = CPoint(  Local_Line[l].pCenter.x + GramH[i]/50 , i);
		// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
		// 			//gpMsgbar->ShowMessage("%d %d %d\n",l,i,GramSum[i]);
		// 		}


		for(i=0,sum=0,sum1=0;i<s_h;i++){
			sum+=GramTmp[i];
			sum1+=GramSum[i];
		}
		int avg = sum/s_h;
		//avg*=1.2;
		k=10;
		k2 = 2*k - 1;
		int peaktotal[50];
		sum = 0;
		for(i=k;i<s_h-k;i++){
			check_max = 0;

			for(j=-k;j<k;j++){
				if(GramH[i]>GramH[i+j] && GramH[i] != 0){
					check_max++;
				}else if(i == i+j){
				}else
					break;
			}
			if(check_max == k2){
				if(GramH[i]>avg){
					if(DISPLAY_SWITCH1 == TRUE){
						P1 = CPoint(  Local_Line[l].pCenter.x - 30,  i);
						P2 = CPoint(  Local_Line[l].pCenter.x + 30 , i);
						if(USING_ROI){
							P1.x += ROI_LB.x;
							P1.y += ROI_LB.y;
							P2.x += ROI_LB.x;
							P2.y += ROI_LB.y;
						}
						if(USINGSMALLFRAME){
							P1.x*=2;
							P1.y*=2;
							P2.x*=2;
							P2.y*=2;
							FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(255,255,255));
						}else
							FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(255,255,255));
						
						
					}
					peaktotal[sum]=i;
					sum++;
				}
				i+=(k+1);
			}
		}
		Local_Line[l].HorizonPeak = new int[sum];
		Local_Line[l].PeakTotal = sum;
		memcpy(Local_Line[l].HorizonPeak,peaktotal,sizeof(int)*(sum));

		//陰影
		avg = sum1 /= s_h;
		avg/=3;
		int shadowcount = 0;
		index1 = 0;
		index2 = 0;
		CPoint Shadow_detection[50];
		for(i=0;i<s_h;i++){
			if(GramSum[i] < avg){
				// 				P1 = CPoint(  Local_Line[l].pCenter.x - 15,  i);
				// 				P2 = CPoint(  Local_Line[l].pCenter.x + 15 , i);
				// 				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
				if(index1 == 0 && index2 ==0){
					index1 = index2 =i;
				}else if(i == index2 + 1 && index1 != 0){
					index2 = i;
				}
			}else if(index1 != 0 && index2 !=0){
				Shadow_detection[shadowcount].x = index1;
				Shadow_detection[shadowcount].y = index2;
				shadowcount++;
				index1=0;
				index2=0;
			}
		}
		//		gpMsgbar->ShowMessage("total:%d\n",shadowcount);
		sum = 0;
		sum1 = 0;
		int maxshadow = 0;
		int maxshadow1 = 0;
		for(i=0;i<shadowcount;i++){//find maxshadow
			//			gpMsgbar->ShowMessage("%d (%d,%d)\n",i,Shadow_detection[i].x,Shadow_detection[i].y);
			if(abs(Shadow_detection[i].x - Shadow_detection[i].y) > sum){
				maxshadow = i;
				sum = abs(Shadow_detection[i].x - Shadow_detection[i].y);
			}
		}
		for(i=0;i<shadowcount;i++){//find second maxshadow
			if(abs(Shadow_detection[i].x - Shadow_detection[i].y) > sum1 && i != maxshadow){
				maxshadow1 = i;
				sum1 = abs(Shadow_detection[i].x - Shadow_detection[i].y);
			}
		}

		Local_Line[l].nShadow[0] = Shadow_detection[maxshadow];
		Local_Line[l].nShadow[1] = Shadow_detection[maxshadow1];
		if(Local_Line[l].nShadow[0].y > Local_Line[l].nShadow[1].y){
			CPoint a = Local_Line[l].nShadow[0];
			Local_Line[l].nShadow[0] = Local_Line[l].nShadow[1];
			Local_Line[l].nShadow[1] = a;
		}

		//histgrom
		// 		for(i=0;i<s_h;i++){
		// 			P1 = CPoint(  Local_Line[l].pCenter.x,  i);
		// 			P2 = CPoint(  Local_Line[l].pCenter.x + GramSum[i]/150 , i);
		// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
		// 		}


		//		gpMsgbar->ShowMessage("%d:max shadow:%d maxshadow1:%d",l,maxshadow,maxshadow1);
		if(DISPLAY_SWITCH1 == TRUE){
			for(i=0, j=Shadow_detection[maxshadow].x ; i<sum ;i++,j++){
				// 			P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				// 			P2 = CPoint(  Local_Line[l].pCenter.x + 15 , j);
				// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));

				P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x , j);
				if(USING_ROI){
					P1.x += ROI_LB.x;
					P1.y += ROI_LB.y;
					P2.x += ROI_LB.x;
					P2.y += ROI_LB.y;
				}
				if(USINGSMALLFRAME){
					P1.x*=2;
					P1.y*=2;
					P2.x*=2;
					P2.y*=2;
					FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(0,255,0));
				}else
					FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(0,255,0));
				P1 = CPoint(  Local_Line[l].pCenter.x,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x + GramSum[j]/150 , j);
				if(USING_ROI){
					P1.x += ROI_LB.x;
					P1.y += ROI_LB.y;
					P2.x += ROI_LB.x;
					P2.y += ROI_LB.y;
				}
				if(USINGSMALLFRAME){
					P1.x*=2;
					P1.y*=2;
					P2.x*=2;
					P2.y*=2;
					FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(0,255,0));
				}else
					FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(0,255,0));
			}
			P1 = CPoint(  Local_Line[l].pCenter.x - 100,  j);
			P2 = CPoint(  Local_Line[l].pCenter.x + 100 , j);
			if(USING_ROI){
				P1.x += ROI_LB.x;
				P1.y += ROI_LB.y;
				P2.x += ROI_LB.x;
				P2.y += ROI_LB.y;
			}
			if(USINGSMALLFRAME){
				P1.x*=2;
				P1.y*=2;
				P2.x*=2;
				P2.y*=2;
				FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(255,0,0));
			}else
				FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(255,0,0));
			for(i=0, j=Shadow_detection[maxshadow1].x ; i<sum1 ;i++,j++){
				// 			P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				// 			P2 = CPoint(  Local_Line[l].pCenter.x + 15 , j);
				// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
				P1 = CPoint(  Local_Line[l].pCenter.x - 15,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x , j);
				if(USING_ROI){
					P1.x += ROI_LB.x;
					P1.y += ROI_LB.y;
					P2.x += ROI_LB.x;
					P2.y += ROI_LB.y;
				}
				if(USINGSMALLFRAME){
					P1.x*=2;
					P1.y*=2;
					P2.x*=2;
					P2.y*=2;
					FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(0,0,255));
				}else
					FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(0,0,255));
				P1 = CPoint(  Local_Line[l].pCenter.x,  j);
				P2 = CPoint(  Local_Line[l].pCenter.x + GramSum[j]/150 , j);
				if(USING_ROI){
					P1.x += ROI_LB.x;
					P1.y += ROI_LB.y;
					P2.x += ROI_LB.x;
					P2.y += ROI_LB.y;
				}
				if(USINGSMALLFRAME){
					P1.x*=2;
					P1.y*=2;
					P2.x*=2;
					P2.y*=2;
					FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(0,0,255));
				}else
					FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(0,0,255));
			}
			P1 = CPoint(  Local_Line[l].pCenter.x - 100,  j);
			P2 = CPoint(  Local_Line[l].pCenter.x + 100 , j);
			if(USING_ROI){
				P1.x += ROI_LB.x;
				P1.y += ROI_LB.y;
				P2.x += ROI_LB.x;
				P2.y += ROI_LB.y;
			}
			if(USINGSMALLFRAME){
				P1.x*=2;
				P1.y*=2;
				P2.x*=2;
				P2.y*=2;
				FuncImg.DrawLine( Display_ImageNsize, Width*2, Height*2, s_bpp, P1, P2, RGB(255,0,0));
			}else
				FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB(255,0,0));
		}
	}
	//引擎蓋 5組水平邊
	//找陰影上5組水平邊中間隔最大的

	for(i=0;i<Local_Line.size();i++){
		index1=0;
		index2=0;
		int count = 0;
		index_switch = false;
		//for(j=0;j<2;j++){
		for(j=0;j<Local_Line[i].PeakTotal;j++){
			if(Local_Line[i].HorizonPeak[j]>Local_Line[i].nShadow[0].y && index_switch == false){
				index_switch = true;
				index1 = j;
			}
			if(Local_Line[i].HorizonPeak[j]>Local_Line[i].nShadow[1].y && index_switch == true){
				index2 = j;
				j = Local_Line[i].PeakTotal;
			}
		}
		int max1=0;
		int max2=0;
		int localmax1 = 0;
		int localmax2 = 0;
		for(k=index1+1;k<index1+4 && (k+1)<Local_Line[i].PeakTotal;k++){			
			int t = Local_Line[i].HorizonPeak[k+1] - Local_Line[i].HorizonPeak[k];
			if(max1 < t){
				max2 = max1;
				max1 = t;
				localmax2 = localmax1;
				localmax1 = k+1;
			}else if(max2 < t){
				max2 = t;
				localmax2 = k+1;
			}
		}

		// 				ww = (Local_Line[i].HorizonPeak[localmax1] - Local_Line[i].nShadow[0].y)/4*3;
		//  			P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[0].y);
		//  			P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax1]);
		// 				FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,255,0));
		Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[0].y;
		Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax1];

		// 				ww = (Local_Line[i].HorizonPeak[localmax2] - Local_Line[i].nShadow[0].y)/4*3;
		// 				P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[0].y);
		// 				P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax2]);
		// 				FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,255,0));
		Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[0].y;
		Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax2];
		//			}


		max1=0;
		max2=0;
		localmax1 = 0;
		localmax2 = 0;
		for(k=index2+1;k<index2+4 && (k+1)<Local_Line[i].PeakTotal;k++ ){
			int t = Local_Line[i].HorizonPeak[k+1] - Local_Line[i].HorizonPeak[k];
			if(max1 < t){
				max2 = max1;
				max1 = t;
				localmax2 = localmax1;
				localmax1 = k+1;
			}else if(max2 < t){
				max2 = t;
				localmax2 = k+1;
			}

		}

		// 			ww = (Local_Line[i].HorizonPeak[localmax1] - Local_Line[i].nShadow[1].y)/4*3;
		// 			P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[1].y);
		// 			P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax1]);
		// 			FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,0,255));
		Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[1].y;
		Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax1];

		// 			ww = (Local_Line[i].HorizonPeak[localmax2] - Local_Line[i].nShadow[1].y)/4*3;
		// 			P1 = CPoint(Local_Line[i].pCenter.x - ww , Local_Line[i].nShadow[1].y);
		// 			P2 = CPoint(Local_Line[i].pCenter.x + ww , Local_Line[i].HorizonPeak[localmax2]);
		// 			FuncImg.DrawRect(Display_ImageNsize,s_w,s_h,s_bpp,P1,P2,RGB(0,0,255));
		Local_Line[i].Candidate_LB[count].y = Local_Line[i].nShadow[1].y;
		Local_Line[i].Candidate_RU[count++].y = Local_Line[i].HorizonPeak[localmax2];
		//			}
		//}

	}


	if(DISPLAY_SWITCH1 == TRUE){
		for (i=0;i<Local_Line.size();i++){//對稱軸
			P1 = CPoint(Local_Line[i].pCenter.x,0);
			P2 = CPoint(Local_Line[i].pCenter.x,s_h-1);
			if(USING_ROI){
				P1.x += ROI_LB.x;
				P1.y += ROI_LB.y;
				P2.x += ROI_LB.x;
				P2.y += ROI_LB.y;
			}
			if(USINGSMALLFRAME){
				P1.x*=2;
				P1.y*=2;
				P2.x*=2;
				P2.y*=2;
				FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB( 255,0,0));
			}else
				FuncImg.DrawLine( Display_ImageNsize, Width, Height, s_bpp, P1, P2, RGB( 255,0,0));
		}
	}
	CarDetected.Local_Line = Local_Line;

	//release memory
	for(i=0;i<Local_Line.size();i++)
		if(Local_Line[i].HorizonPeak){
			delete [] Local_Line[i].HorizonPeak;
			Local_Line[i].HorizonPeak = NULL;
		}

	delete []CenterLinePosX;
	delete []Tmp;
	delete []GramW;
	delete []GramW1;
	delete []GramR;
	delete []GramL;
	delete []GramRT;
	delete []GramLT;
	delete []GramSum;
	delete []GramT;
	delete []GramH;
	delete []GramTmp;
}

void CRogerFunction::HistMaxLine_many(BYTE * input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH)
{
	CImgProcess FuncImg;
	int i, j, k, l, m, n, sum, index1, index2 , CenterLineCounter=0;
	BOOL index_switch = FALSE;
	CPoint P1, P2,temp;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *GramW1 = new double[s_w];
	memset( GramW1, 0, sizeof(double)*s_w);
	double *GramH = new double[s_h];
	memset( GramH, 0, sizeof(double)*s_h);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	int *CenterLinePosX = new int[s_w];
	memset( CenterLinePosX, 0, sizeof(int)*s_w);
	CenterLine tmp;
	vector <CenterLine> Local_Line;//n條對稱軸
	// 	int TOTAL=0;
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CarDetected.vCenterPoint.size(); i++) //消除重複點
	{
		for (j=i+1; j<CarDetected.vCenterPoint.size(); j++)
		{
			if (  i != j && CarDetected.vCenterPoint[i].x == CarDetected.vCenterPoint[j].x && CarDetected.vCenterPoint[i].y == CarDetected.vCenterPoint[j].y )
			{
				CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+j );
				i = 0;
			}
		}
	}

	for (i=0; i<CarDetected.vCenterPoint.size()-1 ; i++)
		for (j=0; j<CarDetected.vCenterPoint.size()-1-i ; j++)
			if (CarDetected.vCenterPoint[j].x > CarDetected.vCenterPoint[j+1].x)
			{
				temp = CarDetected.vCenterPoint[j];
				CarDetected.vCenterPoint[j] = CarDetected.vCenterPoint[j+1];
				CarDetected.vCenterPoint[j+1] = temp;
			}
	for (i=0; i<CarDetected.vCenterPoint.size() ; i++) //histogram
		GramW[CarDetected.vCenterPoint[i].x]++;

	for (i=3; i<s_w-3; i++)//Smooth
		Tmp[i] = GramW[i-3]+GramW[i-2]+GramW[i-1]+3*GramW[i]+GramW[i+1]+GramW[i+2]+GramW[i+3];

	memcpy( GramW, Tmp, sizeof(double)*s_w);
// 	I_BubbleSort( Tmp, s_w );

	int localmax,index;
	for(i=0, j=0, index=0, index1=0, index2=0, localmax = 0;i<s_w;i++){ //將MatchPair分類
		if(GramW[i]>localmax && index1 != 0){
			localmax = GramW[i];
			index = i;
		}
		if(GramW[i]!=0 && index_switch == FALSE){
			index1 = i;
			index_switch = TRUE;
		}else if (GramW[i]==0 && index_switch == TRUE){
			index2 = i;
			index_switch =FALSE;
			if (index_switch == FALSE){
				for(;j<CarDetected.vCenterPoint.size();j++)
					if (CarDetected.vCenterPoint[j].x >= index1 && CarDetected.vCenterPoint[j].x <= index2){
						tmp.MatchPairCenter.push_back(CarDetected.vCenterPoint[j]);
					}else
						break;
				tmp.pCenter.x = index;
				Local_Line.push_back(tmp);
				tmp.MatchPairCenter.clear();
				localmax = 0;
			}
		}
	}


// 	if(TEST_SWITCH){
// 		for (i=0,sum = 0;i<Local_Line.size();i++){
// 			//gpMsgbar->ShowMessage("第%d條對稱軸 %d\n", i, Local_Line[i].MatchPairCenter.size());
// 			sum += Local_Line[i].MatchPairCenter.size();
// 		}
// 		sum /= Local_Line.size();
// 		for (i=0 ;i<Local_Line.size();i++)
// 		{
// 			if(Local_Line[i].MatchPairCenter.size() < sum){
// 				Local_Line.erase(Local_Line.begin()+i);//
// 				i=0;
// 			}
// 		}
// 	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////
	

	VerPoint LocalMax;
	
	vector <CenterLine> Local_LineNew;
	double *GramR = new double[s_h];
	double *GramL = new double[s_h];
	double *GramRT = new double[s_h];
	double *GramLT = new double[s_h];
	double *GramSum = new double[s_h];
	double *GramT = new double[s_h];
	
	BYTE *p_In_ImageNpixel = In_ImageNpixel;
	BYTE *p_In_ImageNpixel2 = In_ImageNpixel2;
	int *p_In_DirectionMap= In_DirectionMap;
//////////////////////////////////////////////////////////////////////////////
	double dCount =0;
	//計算每條對稱軸的水平peak
	for(l=0;l<Local_Line.size();l++){//
		p_In_ImageNpixel = In_ImageNpixel;
		p_In_ImageNpixel2 = In_ImageNpixel2;
		p_In_DirectionMap = In_DirectionMap;
		Local_Line[l].HistDensityH = new double[s_h];
		memset( GramR, 0, sizeof(double)*s_h);
		memset( GramL, 0, sizeof(double)*s_h);
		memset( GramRT, 0, sizeof(double)*s_h);
		memset( GramLT, 0, sizeof(double)*s_h);
		memset( GramSum, 0, sizeof(double)*s_h);
		memset( GramT, 0, sizeof(double)*s_h);
		memset( Local_Line[l].HistDensityH, 0, sizeof(double)*s_h);
		for (i=0; i<s_h*2/3; i++, p_In_ImageNpixel+=s_w, p_In_ImageNpixel2+=s_w)	//水平HISTOGRAM統計
		{
			for (j=Local_Line[l].pCenter.x, k= Local_Line[l].pCenter.x; j<Local_Line[l].pCenter.x+30; j++, k--)
				{
					GramR[i]   = GramR[i] + p_In_ImageNpixel[j];
					GramL[i]   = GramL[i] + p_In_ImageNpixel[k];
					Local_Line[l].HistDensityH[i] += (p_In_ImageNpixel[j] + p_In_ImageNpixel[k])/60;
				}
		}
		for (i=0; i<s_h; i++)
		{
			GramR[i] = GramR[i] / 150;
			GramL[i] = GramL[i] / 150;
			GramT[i] = GramL[i] + GramR[i];
		}
		dCount /= s_h;
		//Smooth
		k = 3;   //60 20
		for (i=k; i<s_h-k; i++)
		{
			for (j=-k; j<k; j++)
			{
				GramSum[i] += GramT[i+j]/k;
				GramRT[i]  += GramR[i+j]/k;
				GramLT[i]  += GramL[i+j]/k;
			}
		} 
		//////////////////////////////////////////////////////////////////////////
		memcpy( GramR, GramRT, sizeof(double)*s_h);
		memcpy( GramL, GramLT, sizeof(double)*s_h);
// 		for (i=0; i<s_h; i++)
// 		{
// 			gpMsgbar->ShowMessage("%d:%f\n",i,GramSum[i]);
// 			if ( GramSum[i] < 16 )
// 			{
// 				GramSum[i] = 0;
// 			}
// 		}
		//中心區域加強權重////////////////////////////////////////////////////
		// 		for (i=0; i<s_h; i++)
		// 		{
		// 			if ( i > CarDetected.nMinY-20 && i < CarDetected.nMaxY+20 )
		// 			{
		// 				GramSum[i] = 2 * GramSum[i];
		// 			}
		// 		}
		//Find Local Maximum
		double MaxV;
		k = 5;
		for (i=0; i<s_h-k; i++){
			MaxV = 0;
			for (j=0; j<k; j++){
				if ( GramSum[i+j] >= MaxV){
					MaxV = GramSum[i+j];
				}
			}
			for (j=0; j<k; j++){
				if (GramSum[i+j] != MaxV)
					GramSum[i+j] = 0;
			}
		}
		for (i=10; i<s_h-10; i++){
// 			P1 = CPoint( Local_Line[l].pCenter.x,i);
// 			P2 = CPoint( Local_Line[l].pCenter.x+ GramR[i]/100 , i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
// 			P1 = CPoint( Local_Line[l].pCenter.x,i);
// 			P2 = CPoint( Local_Line[l].pCenter.x- GramL[i]/100 , i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));

// 			P1 = CPoint( 0, i);
// 			P2 = CPoint( GramSum[i]/10, i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));

			if ( GramSum[i] > 0 ){ 
 				if (DISPLAY_SWITCH2)
 				{
					P1 = CPoint(  Local_Line[l].pCenter.x - 10,  i);
					P2 = CPoint(  Local_Line[l].pCenter.x + 10 , i);
					FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,0));
 				}
				
				LocalMax.dTotalS = GramSum[i];
				LocalMax.pCenter = CPoint( Local_Line[l].pCenter.x, i );
				Local_Line[l].pVer.push_back(LocalMax);
			}
		}
	}
	////////////////////////////////////////////////////////////////
	memset( GramW, 0, sizeof(double)*s_w);  
	p_In_ImageNpixel = In_ImageNpixel;	//垂直HISTOGRAM統計
	p_In_ImageNpixel2 = In_ImageNpixel2;
	dCount =0;
	for (i=1/2*s_h ; i < s_h ; i++){
		for (j=0, k=0; j<s_w; j++, k++){
			GramW[k]   = GramW[k] + p_In_ImageNpixel[i*s_w+j];
		}
	}

	memcpy(GramW1,GramW,sizeof(double)*s_w);
// 	k = 3;
// 	for (i=k; i<s_w-k; i++){//Smooth
// 		for (j=-k; j<k; j++){
// 			GramW1[i] += GramW[i+j]/k;
// 		}
// 	} 
// 	if(DISPLAY_SWITCH2 == TRUE){
// 		double *GramW2 = new double[s_w];
// 		memcpy( GramW2, GramW1, sizeof(double)*s_w);
// 		for (i=0; i<s_w; i++){
// 			P1.x = i;
// 			P1.y = 0;
// 			P2.x = i;
// 			P2.y = GramW2[i]/200;
// 			//GramW[i] = P2.y;
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
// 		}
// 		delete []GramW2;
// 	}

	double MaxV;
	vector <int> VerEdgePoint;
	k = 5;
	for (i=0; i<s_w-k; i++)	{
		MaxV = 0;
		for (j=0; j<k; j++){
			if ( GramW1[i+j] >= MaxV)
			{
				MaxV = GramW1[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramW1[i+j] != MaxV)
				GramW1[i+j] = 0;
		}
	}

	if(DISPLAY_SWITCH2 == TRUE){	
		for (i=0; i<s_w; i++){
			P1.x = i;
			P1.y = 0;
			P2.x = i;
			P2.y = GramW1[i]/200;
			//GramW[i] = P2.y;
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
		}
	}

	for (i=0; i<s_w; i++){
		P1.x = i;
		P1.y = 0;
		P2.x = i;
		if(GramW1[i] != 0){
			P2.y = s_h/2;
			VerEdgePoint.push_back(i);
		}else
			P2.y = GramW1[i]/200;
		//GramW[i] = P2.y;
		//FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	}


	VerPoint Tp;
	
// 	for (i=0;i<Local_Line.size();i++)
// 	{
// 		gpMsgbar->ShowMessage("%d:",i);
// 		for(j=0;j<Local_Line[i].pVer.size();j++){
// 			gpMsgbar->ShowMessage("%.2f ",Local_Line[i].pVer[j].dTotalS);
// 		}
// 		gpMsgbar->ShowMessage("\n");
// 	}
// 	gpMsgbar->ShowMessage("\n");

	for (i=0;i<Local_Line.size();i++){//
		//確認每條對稱軸都存在n組對稱的波峰
		for (j = 0 ; j<k ; j++){  //
			index1 = abs(Local_Line[i].pCenter.x - VerEdgePoint[j]);
			for (k=VerEdgePoint.size()-1 ; VerEdgePoint[k]>Local_Line[i].pCenter.x ;k--){
				index2 =  abs(Local_Line[i].pCenter.x - VerEdgePoint[k]);
				//if( ((double)index1/index2) > 0.9 && ((double)index1/index2) < 1.1 ){
				if( abs(index1-index2) < 5){
					P1.x = VerEdgePoint[k];
					P1.y = 0;
					P2.x = VerEdgePoint[k];
					P2.y = s_h/2;
					if(DISPLAY_SWITCH2 == TRUE){
						FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
					}
					P1.x = VerEdgePoint[j];
					P1.y = 0;
					P2.x = VerEdgePoint[j];
					P2.y = s_h/2;
					if(DISPLAY_SWITCH2 == TRUE){
						FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
					}
					/*gpMsgbar->ShowMessage("%d,%d\n",VerEdgePoint[j],VerEdgePoint[k]);*/
					//gpMsgbar->ShowMessage("%d,%d\n",index1,index2);
					Local_Line[i].lPoint.push_back(VerEdgePoint[j]);
					Local_Line[i].rPoint.push_back(VerEdgePoint[k]);
				}
			}
		}
	}
	for (i=0;i<Local_Line.size();i++){
		if(Local_Line[i].lPoint.capacity()==0){
			Local_Line.erase(Local_Line.begin()+i);
			i=0;
		}
	}
	if(DISPLAY_SWITCH1 == TRUE){
		for (i=0;i<Local_Line.size();i++){//Local_Line.size()對稱軸
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].pCenter.x,0), CPoint(Local_Line[i].pCenter.x, s_h-1), RGB( 255,0,0));
		}
	}

	for (i=0;i<Local_Line.size();i++){//
		for(j=0;j<Local_Line[i].pVer.size()-1;j++)//sort
			for (k=0;k<Local_Line[i].pVer.size()-1-j;k++)
				if (Local_Line[i].pVer[k].dTotalS < Local_Line[i].pVer[k+1].dTotalS)
				{
					Tp = Local_Line[i].pVer[k];
					Local_Line[i].pVer[k] = Local_Line[i].pVer[k+1];
					Local_Line[i].pVer[k+1] = Tp;
				}
		for (j=0;j<Local_Line[i].pVer.size() && j<15;j++)//選出前15條橫切線
		{
// 			P1 = CPoint(  Local_Line[i].pVer[j].pCenter.x - 15,  Local_Line[i].pVer[j].pCenter.y);
// 			P2 = CPoint(  Local_Line[i].pVer[j].pCenter.x + 15 , Local_Line[i].pVer[j].pCenter.y);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
			Local_Line[i].Top.push_back(Local_Line[i].pVer[j]);
			Local_Line[i].Top[Local_Line[i].Top.size()-1].count=0;
		}
		for(j=0,sum=0;j<Local_Line[i].MatchPairCenter.size();j++)
			sum+=Local_Line[i].MatchPairCenter[j].y;
		Local_Line[i].pCenter.y = sum /= Local_Line[i].MatchPairCenter.size(); //算出對稱軸上MatchPair的重心
// 		P1 = CPoint(  Local_Line[i].pCenter.x - 20,  Local_Line[i].pCenter.y);
// 		P2 = CPoint(  Local_Line[i].pCenter.x + 20 , Local_Line[i].pCenter.y);
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
		
		for (j=0;j<Local_Line[i].Top.size()-1;j++){//將對稱軸上的橫切線排序
			for (k=0;k<Local_Line[i].Top.size()-1-j;k++)
				if (Local_Line[i].Top[k].pCenter.y > Local_Line[i].Top[k+1].pCenter.y)
				{
					Tp = Local_Line[i].Top[k];
					Local_Line[i].Top[k] = Local_Line[i].Top[k+1];
					Local_Line[i].Top[k+1] = Tp;
				}
		}
		for (j=0;j<Local_Line[i].Top.size();j++)//設定間隔
		{
			if (j==0){
				Local_Line[i].Top[j].Dpoint = Local_Line[i].Top[j].pCenter;
				Local_Line[i].Top[j].Upoint = Local_Line[i].Top[j+1].pCenter;
			}else if (j==Local_Line[i].Top.size()-1){
				Local_Line[i].Top[j].Dpoint = Local_Line[i].Top[j-1].pCenter;
				Local_Line[i].Top[j].Upoint = Local_Line[i].Top[j].pCenter;
			}else{
				Local_Line[i].Top[j].Dpoint = Local_Line[i].Top[j].pCenter;
				Local_Line[i].Top[j].Upoint = Local_Line[i].Top[j+1].pCenter;
			}
//			gpMsgbar->ShowMessage("\n");
		}
		for (j=0;j<Local_Line[i].MatchPairCenter.size();j++)
		{
			for (k=0;k< Local_Line[i].Top.size()-1;k++)
			{
				//if(Local_Line[i].MatchPairCenter[j].y > Local_Line[i].Top[k].Dpoint.y && Local_Line[i].MatchPairCenter[j].y < Local_Line[i].Top[k].Upoint.y)
				if(Local_Line[i].MatchPairCenter[j].y > Local_Line[i].Top[k].pCenter.y && Local_Line[i].MatchPairCenter[j].y < Local_Line[i].Top[k+1].pCenter.y){
					FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3,Local_Line[i].MatchPairCenter[j], 4 ,RGB(255,0,255),3);
					Local_Line[i].Top[k].count++;
				}
			}
		}

		for (j=0,index1 = -1,index2 = -1,index_switch = FALSE;j< Local_Line[i].Top.size();j++)
		{
			//gpMsgbar->ShowMessage("第%d條對稱軸第%d個區間:%d\n",i,j,Local_Line[i].Top[j].count);
			if(Local_Line[i].Top[j].count != 0 && index_switch ==FALSE){ //紀錄第一個不為零的區間
				index1=j;
				//index2=j;
				index_switch = TRUE;
				//FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].Top[j].Dpoint.x-100,Local_Line[i].Top[j].Dpoint.y), CPoint(Local_Line[i].Top[j].Dpoint.x+100,Local_Line[i].Top[j].Dpoint.y), RGB(255,255,0));
				//FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].Top[j].Dpoint.x-100,Local_Line[i].Top[j].Upoint.y), CPoint(Local_Line[i].Top[j].Dpoint.x+100,Local_Line[i].Top[j].Upoint.y), RGB(255,255,0));
				//FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].Top[j].Dpoint.x-50,Local_Line[i].Top[j].Dpoint.y), CPoint(Local_Line[i].Top[j].Dpoint.x+50,Local_Line[i].Top[j].Upoint.y), RGB(0,0,255));
			}
			if (Local_Line[i].Top[j].count != 0 && index_switch ==TRUE)	{//紀錄最後一個不為零的區間
				index2 = j;
			}
		}
/*		gpMsgbar->ShowMessage("第%d條線:%d,%d\n",i,index1,index2);*/
		if(index1 != -1 && index2 != -1){
			//if(i==16){
			Local_Line[i].bPoint = Local_Line[i].Top[index1].Dpoint.y;
			Local_Line[i].uPoint = Local_Line[i].Top[index2].Upoint.y;
			for (k=index1;k<=index2;k++){
				for(l=k;l<=index2;l++){
					for(m=0;m<Local_Line[i].lPoint.size();m++){//push candidate
						//for(n=0;n<Local_Line[i].lPoint.size();n++){
							P1 = CPoint( Local_Line[i].lPoint[m] , Local_Line[i].Top[k].Dpoint.y);
							P2 = CPoint( Local_Line[i].rPoint[m] , Local_Line[i].Top[l].Upoint.y);
							double s = ((double)abs(P1.y-P2.y) / abs(P1.x-P2.x));
							if(s >= 0.6 && s < 0.71){
								//gpMsgbar->ShowMessage("%d,%d\n", Local_Line[i].Top[index1].pCenter.y, Local_Line[i].Top[index2].pCenter.y);
								CString a;
								a.Format("%d_%d_%d_%d",i,k,l,m);
								SaveImage(input,s_h,s_w,s_bpp,P1,P2,a);
								//FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
								//}	
// 								Local_Line[i].Candidate_LB.push_back(P1);
// 								Local_Line[i].Candidate_RU.push_back(P2);
// 								CarDetected.Candidate_LB.push_back(P1);
// 								CarDetected.Candidate_RU.push_back(P2);
							}
						//}
					}
				}
			}
			//}
		}else{
			Local_Line[i].EREASE=TRUE;
		}
	}
	for (i=0;i<Local_Line.size();i++){
		if(Local_Line[i].EREASE==TRUE){
			Local_Line.erase(Local_Line.begin()+i);
			i=0;
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////修正對稱軸位置
// 	for (i=0;i<Local_Line.size();i++)
// 	{
// 		gpMsgbar->ShowMessage("第%d條\n",i);
// 		for (j=0;j<Local_Line[i].Top.size();j++)
// 		{
// 			P1 = Local_Line[i].Top[j].Dpoint;
// 			P2 = Local_Line[i].Top[j].Dpoint;
// 			P1.x-=5;
// 			P2.x+=5;
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
// 			P1 = Local_Line[i].Top[j].Upoint;
// 			P2 = Local_Line[i].Top[j].Upoint;
// 			P1.x-=5;
// 			P2.x+=5;
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
// 			gpMsgbar->ShowMessage("\t區間%d : %d\n",j,Local_Line[i].Top[j].count);
// 		}
// 	}
	//陰影/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 	int	nDist = 0;
// 	int nExplore;
// 	int nExploreWidth;
// 	int nCenter;
// 	int nBt;
// 	double *dTmpVector1;
// 	double *dTmpVector2;
// 	for (i=0;i<Local_Line.size();i++){
// 		for(j=0;j<Local_Line[i].Candidate_LB.size();j++){
// 			Local_Line[i].dShadowsGradient =0;
// 			Local_Line[i].dShadowsIntensity=0;
// 			Local_Line[i].nDistGradient=0;
// 			Local_Line[i].nDistIntensity=0;
// 
// 			nExplore      = 50 ;
// 			nExploreWidth = 15;
// 			//////////////////////////////////////////////////////////////////////////
// 			if ( Local_Line[i].Candidate_LB[j].y < s_h/4 )
// 				nExplore = s_h/4;
// 			else if ( Local_Line[i].Candidate_LB[j].y < s_h*2/3 )
// 				nExplore = s_h/5;
// 			else
// 				nExplore = s_h/6;
// 
// 			// 	gpMsgbar->ShowMessage("Explore: %d\n", nExplore);
// 			//////////////////////////////////////////////////////////////////////////
// 			nCenter = Local_Line[i].Candidate_LB[j].y+20;
// 			nBt = Local_Line[i].Candidate_LB[j].y - nExplore;
// 
// 			if( nBt <= 0 ) nBt = 0;
// 			int nBinSize = abs(nCenter - nBt) ;
// 			dTmpVector1 = new double[nBinSize];
// 			dTmpVector2 = new double[nBinSize];
// 			memset( dTmpVector1, 0, sizeof(double)*nBinSize);
// 			memset( dTmpVector2, 0, sizeof(double)*nBinSize );
// 			for ( k = nCenter , m =0; k > nBt ; k--, m++)
// 			{
// 				for (l=0; l< 8 ; l++)
// 				{
// 					if( k+l <= 0)
// 					{
// 						dTmpVector1[m] += 1000;
// 						k = nBt;
// 					}
// 					else
// 					{
// 						dTmpVector1[m] += Local_Line[i].HistDensityH[k+l]/10;
// 					}
// 				}
// 			}
// 			memcpy( dTmpVector2, dTmpVector1, sizeof(double)*nBinSize);
// 			I_BubbleSort( dTmpVector2, nBinSize );
// 			vector <int> vDist;
// 			for (k=0; k<nBinSize; k++)
// 			{
// 				if ( dTmpVector1[k] == dTmpVector2[nBinSize-1])
// 				{
// 					nDist = k;
// 					vDist.push_back( k );
// 					k = nBinSize;
// 				}
// 			}
// 			if (  DISPLAY_SWITCH1 == TRUE  )
// 				for (k=0; k<5; k++)
// 				{
// 					P1.x = Local_Line[i].pCenter.x -j;
// 					P1.y = nCenter-nDist;
// 					P2.x = Local_Line[i].pCenter.x +j;
// 					P2.y = nCenter-nDist +10;
// 					/*if ( DISPLAY_SWITCH == TRUE )*/ 
// 					FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
// 				}
// 				//////////////////////////////////////////////////////////////////////////
// 			Local_Line[i].nDistIntensity = nDist;
// 			if ( nBt ==0 && nCenter == 0 )
// 				Local_Line[i].dShadowsIntensity = 0;
// 			else
// 				Local_Line[i].dShadowsIntensity = dTmpVector2[nBinSize-1];
// 
// 				// 	gpMsgbar->ShowMessage("..............Modify............\n");
// 
// 			Local_Line[i].Candidate_LB[j].y = nCenter-nDist+10;
// 			
// 			//CarDetected.LB2.y = nCenter-nDist+10;
// 
// 			delete []dTmpVector1;
// 			delete []dTmpVector2;
// 		}
// 	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 	for(i=0;i<Local_Line.size();i++){
// 		for(j=0;j<Local_Line[i].Candidate_LB.size();j++){
// 			//FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, Local_Line[i].Candidate_LB[j], Local_Line[i].Candidate_RU[j], RGB(0,0,255));
// 			CString a;
// 			a.Format("\\shadow\\%d_%d",i,j);
// 			SaveImage(input,s_h,s_w,s_bpp,Local_Line[i].Candidate_LB[j],Local_Line[i].Candidate_RU[j],a);
// 			P1 = Local_Line[i].Candidate_LB[j];
// 			P2 = Local_Line[i].Candidate_RU[j];
// 			double s = ((double)abs(P1.y-P2.y) / abs(P1.x-P2.x));
// 			if(s >= 0.6 && s < 0.71){
// 				CarDetected.Candidate_LB.push_back(P1);
// 				CarDetected.Candidate_RU.push_back(P2);
// 				
// 				a.Format("\\test\\%d_%d",i,j);
// 				SaveImage(input,s_h,s_w,s_bpp,Local_Line[i].Candidate_LB[j],Local_Line[i].Candidate_RU[j],a);
// 			}
// 		}
// 	}
	CarDetected.Local_Line = Local_Line;

// 	for (i=0;i<Local_Line.size();i++)
// 	{
// 		gpMsgbar->ShowMessage("%d:",i);
// 		for(j=0;j<Local_Line[i].pVer.size();j++){
// 			gpMsgbar->ShowMessage("%.2f ",Local_Line[i].pVer[j].dTotalS);
// 		}
// 		gpMsgbar->ShowMessage("\n");
// 	}


	delete []CenterLinePosX;
	delete []Tmp;
	delete []GramW;
	delete []GramW1;
	delete []GramR;
	delete []GramL;
	delete []GramRT;
	delete []GramLT;
	delete []GramSum;
	delete []GramT;

}

void CRogerFunction::HistMaxLine_many1(BYTE * input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH){
	CImgProcess FuncImg;
	int i, j, k, l, index1, index2 , CenterLineCounter=0;
	BOOL index_switch = FALSE;
	CPoint P1, P2,temp;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *GramW1 = new double[s_w];
	memset( GramW1, 0, sizeof(double)*s_w);
	double *GramH = new double[s_h];
	memset( GramH, 0, sizeof(double)*s_h);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	int *CenterLinePosX = new int[s_w];
	memset( CenterLinePosX, 0, sizeof(int)*s_w);
	CenterLine tmp;
	vector <CenterLine> Local_Line;//n條對稱軸
	// 	int TOTAL=0;
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<CarDetected.vCenterPoint.size(); i++){ //消除重複點
		for (j=i+1; j<CarDetected.vCenterPoint.size(); j++){
			if (  i != j && CarDetected.vCenterPoint[i].x == CarDetected.vCenterPoint[j].x && CarDetected.vCenterPoint[i].y == CarDetected.vCenterPoint[j].y ){
				CarDetected.vCenterPoint.erase( CarDetected.vCenterPoint.begin()+j );
				i = 0;
			}
		}
	}

	for (i=0; i<CarDetected.vCenterPoint.size()-1 ; i++)
		for (j=0; j<CarDetected.vCenterPoint.size()-1-i ; j++)
			if (CarDetected.vCenterPoint[j].x > CarDetected.vCenterPoint[j+1].x){//把偵測到的對稱點排序
				temp = CarDetected.vCenterPoint[j];
				CarDetected.vCenterPoint[j] = CarDetected.vCenterPoint[j+1];
				CarDetected.vCenterPoint[j+1] = temp;
			}
			//	gpMsgbar->ShowMessage("vCenterPointSize:%d\n",CarDetected.vCenterPoint.size());
	for (i=0; i<CarDetected.vCenterPoint.size() ; i++){ //垂直matching pair histogram
		GramW[CarDetected.vCenterPoint[i].x]++;
				//		gpMsgbar->ShowMessage("( %d , %d ) ",CarDetected.vCenterPoint[i].x,CarDetected.vCenterPoint[i].y);
	}
			//	gpMsgbar->ShowMessage("\n");
	for (i=2; i<s_w-2; i++)//Smooth
		Tmp[i] = GramW[i-2]+GramW[i-1]+2*GramW[i]+GramW[i+1]+GramW[i+2];

	for (i=0;i<s_w;i++){
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(i,0), CPoint(i,(int)Tmp[i]*5), RGB( 0,0,0));
	}
	memcpy( GramW, Tmp, sizeof(double)*s_w);
// 			I_BubbleSort( Tmp, s_w );
	int localmax,index;
	for(i=0, j=0, index=0, index1=0, index2=0, localmax=0;i<s_w;i++){ //將MatchPair分類
		if(GramW[i]>localmax && index1 != 0){
			localmax = GramW[i];
			index = i;
		}
		if(GramW[i]!=0 && index_switch == FALSE){
			index1 = i;
			index_switch = TRUE;
		}else if (GramW[i]==0 && index_switch == TRUE){
			index2 = i;
			index_switch =FALSE;
			if (index_switch == FALSE){
				for(;j<CarDetected.vCenterPoint.size();j++)
					if (CarDetected.vCenterPoint[j].x >= index1 && CarDetected.vCenterPoint[j].x <= index2){
						tmp.MatchPairCenter.push_back(CarDetected.vCenterPoint[j]);
					}else
						break;
					tmp.pCenter.x=index;
					Local_Line.push_back(tmp);
					tmp.MatchPairCenter.clear();
					localmax = 0;
			}
		}
	}


	if(DISPLAY_SWITCH1 == TRUE){
		for (i=0;i<Local_Line.size();i++){//Local_Line.size()對稱軸
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CPoint(Local_Line[i].pCenter.x,0), CPoint(Local_Line[i].pCenter.x, s_h-1), RGB( 0,0,0));
		}
	}
// 			if(TEST_SWITCH){
// 				for (i=0,sum = 0;i<Local_Line.size();i++){
// 					//gpMsgbar->ShowMessage("第%d條對稱軸 %d\n", i, Local_Line[i].MatchPairCenter.size());
// 					sum += Local_Line[i].MatchPairCenter.size();
// 				}
// 				sum /= Local_Line.size();
// 				for (i=0 ;i<Local_Line.size();i++)
// 				{
// 					if(Local_Line[i].MatchPairCenter.size() < sum){
// 						Local_Line.erase(Local_Line.begin()+i);//
// 						//i=0;
// 						i--;
// 					}
// 				}
// 			}
	double *GramR = new double[s_h];
	double *GramL = new double[s_h];
	double *GramRT = new double[s_h];
	double *GramLT = new double[s_h];
	double *GramSum = new double[s_h];
	double *GramT = new double[s_h];
	double *HistDensityH = new double[s_h];
	BYTE *p_In_ImageNpixel = In_ImageNpixel;
	BYTE *p_In_ImageNpixel2 = In_ImageNpixel2;
	int *p_In_DirectionMap= In_DirectionMap;
	double dCount=0;
	/*for(l=0;l<Local_Line.size();l++){*/
	for(l=24;l<25;l++){
		p_In_ImageNpixel = In_ImageNpixel;
		p_In_ImageNpixel2 = In_ImageNpixel2;
		p_In_DirectionMap = In_DirectionMap;
		memset( GramR, 0, sizeof(double)*s_h);
		memset( GramL, 0, sizeof(double)*s_h);
		memset( GramRT, 0, sizeof(double)*s_h);
		memset( GramLT, 0, sizeof(double)*s_h);
		memset( GramSum, 0, sizeof(double)*s_h);
		memset( GramT, 0, sizeof(double)*s_h);

		for (i=0,k=0; i<s_h*2/3; i++, p_In_ImageNpixel+=s_w){	//水平HISTOGRAM統計
			for (j=Local_Line[l].pCenter.x, k= Local_Line[l].pCenter.x; j<Local_Line[l].pCenter.x+30; j++, k--){
				GramR[i]   = GramR[i] + p_In_ImageNpixel[j];
				GramL[i]   = GramL[i] + p_In_ImageNpixel[k];
				//HistDensityH[i] += (p_In_ImageNpixel[j] + p_In_ImageNpixel[k])/60;
			}
		}
		for (i=0; i<s_h; i++){
			GramT[i] = GramL[i] + GramR[i];
		}

		//Smooth
		k = 5;   //60 20
		for (i=k; i<s_h-k; i++)
		{
			for (j=-k; j<k; j++)
			{
				GramSum[i] += GramT[i+j]/k;
				GramRT[i]  += GramR[i+j]/k;
				GramLT[i]  += GramL[i+j]/k;
			}
		}
		//Find Local Maximum
		double MaxV;
		k = 5;
		for (i=0; i<s_h-k; i++){
			MaxV = 0;
			for (j=0; j<k; j++){
				if ( GramSum[i+j] >= MaxV){
					MaxV = GramSum[i+j];
				}
			}
			for (j=0; j<k; j++){
				if (GramSum[i+j] != MaxV)
					GramSum[i+j] = 0;
			}
		}
		if(DISPLAY_SWITCH1 == TRUE){
			for (i=0;i<s_h;i++){//Local_Line.size()對稱軸
				P1 = CPoint( Local_Line[l].pCenter.x,i);
				P2 = CPoint( Local_Line[l].pCenter.x+ GramSum[i]/50 , i);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
				P1 = CPoint( Local_Line[l].pCenter.x,i);
				P2 = CPoint( Local_Line[l].pCenter.x- GramSum[i]/50 , i);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
				if(GramSum[i]>0){
					P1 = CPoint(  0,  i);
					P2 = CPoint(  s_w-1 , i);
					FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
				}
			}
		}
	}

	delete []GramW;	
	delete []GramW1;		
	delete []GramH;	
	delete []Tmp;
	delete []CenterLinePosX;
}


void CRogerFunction::HistVERTLINE( CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH, BOOL DISPLAY_SWITCH1 ) 
{
	CImgProcess FuncImg;
	int i, j, k;
	CPoint P1, P2;
	CenterLine LocalMax;
	vector <CenterLine> Local_Line;
	double *GramR = new double[s_h];
	memset( GramR, 0, sizeof(double)*s_h);
	double *GramL = new double[s_h];
	memset( GramL, 0, sizeof(double)*s_h);

	double *GramRT = new double[s_h];
	memset( GramRT, 0, sizeof(double)*s_h);
	double *GramLT = new double[s_h];
	memset( GramLT, 0, sizeof(double)*s_h);

	double *GramSum = new double[s_h];
	memset( GramSum, 0, sizeof(double)*s_h);	
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);

	double *HistDensityH = new double[s_h];
	memset( HistDensityH, 0, sizeof(double)*s_h);

	BYTE *p_In_ImageNpixel = In_ImageNpixel;
	BYTE *p_In_ImageNpixel2 = In_ImageNpixel2;
	
	//////////////////////////////////////////////////////////////////////////
	double dCount =0;
	for (i=0; i<s_h; i++, p_In_ImageNpixel+=s_w, p_In_ImageNpixel2+=s_w)
	{
		for (j=CarDetected.nCenterLinePosX, k= CarDetected.nCenterLinePosX; j<CarDetected.nCenterLinePosX+30; j++, k--)
		{
			GramR[i]   = GramR[i] + p_In_ImageNpixel[j];
			GramL[i]   = GramL[i] + p_In_ImageNpixel[k];
			HistDensityH[i] += (p_In_ImageNpixel[j] + p_In_ImageNpixel[k])/60;
		}
	}
	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / 150;
		GramL[i] = GramL[i] / 150;
		GramT[i] = GramL[i] + GramR[i];
	}
	dCount /= s_h;
	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
			GramRT[i]  += GramR[i+j]/k;
			GramLT[i]  += GramL[i+j]/k;
		}
	} 
	//////////////////////////////////////////////////////////////////////////
	memcpy( GramR, GramRT, sizeof(double)*s_h);
	memcpy( GramL, GramLT, sizeof(double)*s_h);
	for (i=0; i<s_h; i++)
	{
		if ( GramSum[i] < 16 )
		{
			GramSum[i] = 0;
		}
	}
	//中心區域加強權重////////////////////////////////////////////////////
	for (i=0; i<s_h; i++)
	{
		if ( i > CarDetected.nMinY-20 && i < CarDetected.nMaxY+20 )
		{
			GramSum[i] = 2 * GramSum[i];
		}
	}
	//Find Local Maximum
	double MaxV;
	k = 5;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}
	for (i=10; i<s_h-10; i++)
	{
	   if(  DISPLAY_SWITCH1 == TRUE   )
	   {
		   P1 = CPoint( CarDetected.nCenterLinePosX,i);
		   P2 = CPoint( CarDetected.nCenterLinePosX+ GramR[i]/2 , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
		   P1 = CPoint( CarDetected.nCenterLinePosX,i);
		   P2 = CPoint( CarDetected.nCenterLinePosX- GramL[i]/2 , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
	   }
	   if( DISPLAY_SWITCH1 == TRUE )
	   {
		   P1 = CPoint( 0, i);
		   P2 = CPoint( GramSum[i]/10, i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	   }
	   if ( GramSum[i] > (0.6 *dCount) )
	   { 
		   if(  DISPLAY_SWITCH1 == TRUE  )
		   {
			   P1 = CPoint(  CarDetected.nCenterLinePosX - 5,  i);
			   P2 = CPoint(  CarDetected.nCenterLinePosX + 5 , i);
			   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		   }
		   LocalMax.pCenter = CPoint( CarDetected.nCenterLinePosX, i );
		   LocalMax.BDISPLAY= FALSE;
		   LocalMax.dTotalS = GramSum[i];
		   Local_Line.push_back( LocalMax );
		}
	}
	///////////////////////////////////////////////////////////
	int nDist;
	int nCnt = 0;
	////////////////////////////////////////////////////////////
	double *LocalHistogram = new double[Local_Line.size()];
	memset( LocalHistogram, 0, sizeof(double)*Local_Line.size());
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<Local_Line.size(); i++)
	{
		LocalHistogram[i] = Local_Line[i].dTotalS;
	}
 	I_BubbleSort( LocalHistogram, Local_Line.size() );

	int nMaxPosH;
	vector <int> Top5;
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( Local_Line[i].dTotalS == LocalHistogram[0] )
		{
			Local_Line[i].BDISPLAY = TRUE;
			//找到1st最大平均強度
			if (  DISPLAY_SWITCH1 == TRUE  )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1  , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
				CarDetected.cMP1 = P1;
			}
			////////////////////////////////////////////////
			nMaxPosH = Local_Line[i].pCenter.y;
			Top5.push_back(Local_Line[i].pCenter.y);

		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[1]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if (  DISPLAY_SWITCH1 == TRUE  )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
				CarDetected.cMP2 = P1;
			}
			Top5.push_back(Local_Line[i].pCenter.y);

		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[2]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if (  DISPLAY_SWITCH1 == TRUE  )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
				CarDetected.cMP3 = P1;
			}
			Top5.push_back(Local_Line[i].pCenter.y);

		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[3]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if (  DISPLAY_SWITCH1 == TRUE  )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
				CarDetected.cMP4 = P1;
			}
			Top5.push_back(Local_Line[i].pCenter.y);

		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[4]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if (  DISPLAY_SWITCH1 == TRUE  )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
				CarDetected.cMP5 = P1;
			}
			Top5.push_back(Local_Line[i].pCenter.y);
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[5]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if (  DISPLAY_SWITCH1 == TRUE  )
			{
				P1 = CPoint( 280 ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
			Top5.push_back(Local_Line[i].pCenter.y);

		}
	}
	delete []LocalHistogram;
	///Top5////////////////////////////////////////////////////////////////////
// 	for (i=0; i<Top5.size(); i++)
// 	{
// 		gpMsgbar->ShowMessage("%d: (%d)\n", i, s_h - Top5[i]);
// 	}
	/////Sliding Windows///////////////////////////////////////////////////////////////
	int nDistConst; 
	if ( nMaxPosH < s_h/3 )
		nDistConst = s_h/4;
	else if( nMaxPosH >= s_h/3 && nMaxPosH < s_h*2/3 )
		nDistConst = s_h/6;
	else
		nDistConst = s_h/8;

	double dAccum;
	nDist = 0;
	vector <Segment> SegPos;
	Segment SEG;
	for ( i=0; i<Local_Line.size(); i++)
	{
		dAccum = 0;
		for (j=i; j<Local_Line.size(); j++)
		{
			nDist = abs( Local_Line[i].pCenter.y - Local_Line[j].pCenter.y );
			if ( i != j && nDist < nDistConst )
			{
				dAccum += Local_Line[j].dTotalS;
				SEG.nStartH = Local_Line[i].pCenter.y;
				SEG.nEndH   = Local_Line[j].pCenter.y;
				SEG.nCenterX= Local_Line[i].pCenter.x;
				SEG.dTotal = dAccum;
				SEG.Remain = FALSE;
				SegPos.push_back( SEG );
			}
		}
	}
	//sliding windows 找出最大值
	double *SEGBUFFER = new double[ SegPos.size() ];
	memset( SEGBUFFER, 0, sizeof(double)*SegPos.size());
	for (i=0; i<SegPos.size(); i++)
		SEGBUFFER[i] = SegPos[i].dTotal;
	I_BubbleSort( SEGBUFFER, SegPos.size() );

	for (i=0; i<SegPos.size(); i++)
	{
		if ( SegPos[i].dTotal == SEGBUFFER[0] )
		{
			SegPos[i].Remain = TRUE;
		}
		else
		{
			SegPos.erase( SegPos.begin()+i);
			i = 0;
		}
	}
	for (i=0; i<SegPos.size(); i++)
	{
			nDist = abs(SegPos[i].nEndH+10 - SegPos[i].nStartH+10);
			nDist *= 1.3;
			P1 = CPoint(SegPos[i].nCenterX-nDist,SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+nDist,SegPos[i].nEndH+10);
			CarDetected.LB1 = P1;
			CarDetected.RU1 = P2;

			P1 = CPoint(SegPos[i].nCenterX-(nDist*2.4),SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+(nDist*2.4),SegPos[i].nEndH+nDist*2.5);
			CarDetected.LB2 = P1;
			CarDetected.RU2 = P2;
	}
	for (i=2; i<5; i++)
	{
		P1.x = CarDetected.nCenterLinePosX;
		P1.y = CarDetected.RU1.y;
		FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, s_bpp, P1, i, RGB(0,0,255),3);
	}
// 	// 修正上方邊界
	for ( i=Top5.size()-1 ; i>=0 ; i--)
	{
		if ( Top5[i] <= CarDetected.RU1.y)
		{
			CarDetected.RU1.y = Top5[i];
			i = 0;
		}

	}
	//////////////////////////////////////////////////////////////////////////
    //對稱histogram
// 	for (i=CarDetected.LB1.y; i<CarDetected.RU1.y; i++)
// 	{
// 		if(  DISPLAY_SWITCH1 == TRUE   )
// 		{
// 			P1 = CPoint( CarDetected.nCenterLinePosX,i);
// 			P2 = CPoint( CarDetected.nCenterLinePosX+ GramR[i]/2 , i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
// 			P1 = CPoint( CarDetected.nCenterLinePosX,i);
// 			P2 = CPoint( CarDetected.nCenterLinePosX- GramL[i]/2 , i);
// 			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
// 		}
// 	}
	//////////////////////////////////////////////////////////////////////////
	//找尋地面陰影
	CarDetected.dShadowsGradient =0;
	CarDetected.dShadowsIntensity=0;
	CarDetected.nDistGradient=0;
	CarDetected.nDistIntensity=0;
	nDist = 0;
	int nExplore;
	if ( CarDetected.LB1.y > s_h/2)
		nExplore = 10 ;
	else 
		nExplore = 30 ;

	int nCenter = CarDetected.LB1.y;
	int nBt = CarDetected.LB1.y - nExplore;

	if( nBt <= 0 ) nBt = 0;
	int nBinSize = abs(nCenter - nBt) ;
	double *dTmpVector1 = new double[nBinSize];
	double *dTmpVector2 = new double[nBinSize];
	memset( dTmpVector1, 0, sizeof(double)*nBinSize);
	memset( dTmpVector2, 0, sizeof(double)*nBinSize );
	for ( i = nCenter , k =0; i > nBt ; i--, k++)
	{
		for (j=0; j< 8 ; j++)
		{
			if( i+j <= 0)
			{
				dTmpVector1[k] += 1000;
 				i = nBt;
			}
			else
			{
				dTmpVector1[k] += HistDensityH[i+j]/10;
			}
		}
	}
// 	for (k=0; k<nBinSize; k++)
// 	{
// 		gpMsgbar->ShowMessage("[%d:%.2f]", k, dTmpVector1[k]);
// 	}
// 	gpMsgbar->ShowMessage("\n");
	memcpy( dTmpVector2, dTmpVector1, sizeof(double)*nBinSize);
	I_BubbleSort( dTmpVector2, nBinSize );
	vector <int> vDist;
// 	gpMsgbar->ShowMessage("Min: %f\n", dTmpVector2[nBinSize-1]);
	for (i=0; i<nBinSize; i++)
	{
		if ( dTmpVector1[i] == dTmpVector2[nBinSize-1])
		{
			nDist = i;
			vDist.push_back( i );
// 			gpMsgbar->ShowMessage("...%d:%.2f\n", i, dTmpVector1[i]);
			i = nBinSize;
		}
	}
	//可刪除
	if (  DISPLAY_SWITCH1 == TRUE  )
		for (i=0; i<5; i++)
		{
			P1 = CPoint(CarDetected.nCenterLinePosX-i, nCenter-nDist);
			P2 = CPoint(CarDetected.nCenterLinePosX+i, nCenter-nDist+10);
			if ( DISPLAY_SWITCH == TRUE ) FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
		}
	//////////////////////////////////////////////////////////////////////////
	CarDetected.nDistIntensity = nDist;
	if ( nBt ==0 && nCenter == 0 )
		CarDetected.dShadowsIntensity = 0;
	else
		CarDetected.dShadowsIntensity = dTmpVector2[nBinSize-1];

// 	gpMsgbar->ShowMessage("..............Modify............\n");

	CarDetected.LB1.y = nCenter-nDist+10;
	CarDetected.LB2.y = nCenter-nDist+10;

	//////////////////////////////////////////////////////////////////////////
	//找尋引擎蓋
	CarDetected.dShadowsGradient =0;
	CarDetected.dShadowsIntensity=0;
	CarDetected.nDistGradient=0;
	CarDetected.nDistIntensity=0;
	nDist = 0;
	
	nCenter = CarDetected.RU1.y;
	int nUp = CarDetected.RU1.y + 10 ;
	nBinSize = abs(nUp - nCenter) ;

	double *dTmpVector3 = new double[nBinSize];
	double *dTmpVector4 = new double[nBinSize];
	
	memset( dTmpVector3, 0, sizeof(double)*nBinSize);
	memset( dTmpVector4, 0, sizeof(double)*nBinSize);

	for ( i = nUp , k =0; i > nCenter ; i--, k++)
	{
		for (j=0; j<10; j++)
		{
			if( i+j <= 0)
			{
				dTmpVector3[k] += 1000;
				i = nCenter;
			}
			else
				dTmpVector3[k] += HistDensityH[i+j]/10;
		}
	}
	memcpy( dTmpVector4, dTmpVector3, sizeof(double)*nBinSize);
	I_BubbleSort( dTmpVector4, nBinSize );
	vector <int> vDist2;
	for (i=0; i<nBinSize; i++)
	{
		if ( dTmpVector3[i] == dTmpVector4[nBinSize-1] )
		{
			nDist = i;
			vDist2.push_back( i );
			i = nBinSize;
		}
	}
	
	P1 = CPoint(CarDetected.LB1.x,nCenter+nDist);
	P2 = CPoint(CarDetected.RU1.x,nCenter+nDist+10);
	CarDetected.RU1.y = P2.y;
// 	if ( DISPLAY_SWITCH == TRUE ) FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));

	/////////////////////////////////////////////////////////////////////////////////////
	nDist = abs(CarDetected.RU1.y - CarDetected.LB1.y);
	CarDetected.LB1.x = CarDetected.nCenterLinePosX - 1.3*nDist;
	CarDetected.RU1.x = CarDetected.nCenterLinePosX + 1.3*nDist;
	P1 = CarDetected.LB1;
	P2 = CarDetected.RU1;
	if ( DISPLAY_SWITCH == TRUE ) FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));

	if ( DISPLAY_SWITCH1 == TRUE  )
	{
		for (i=0; i<CarDetected.vResult.size(); i++  )
		{
			if ( CarDetected.vResult[i].CenterPoint.x > CarDetected.LB1.x && CarDetected.vResult[i].CenterPoint.x < CarDetected.RU1.x && 
				 CarDetected.vResult[i].CenterPoint.y > CarDetected.LB1.y && CarDetected.vResult[i].CenterPoint.y < CarDetected.RU1.y &&
				(( CarDetected.vResult[i].Point_L.x > CarDetected.LB1.x && CarDetected.vResult[i].Point_R.x < CarDetected.RU1.x && CarDetected.vResult[i].Point_L.x < CarDetected.nCenterLinePosX && CarDetected.vResult[i].Point_R.x > CarDetected.nCenterLinePosX) || 
				 ( CarDetected.vResult[i].Point_R.x > CarDetected.LB1.x && CarDetected.vResult[i].Point_L.x < CarDetected.RU1.x && CarDetected.vResult[i].Point_R.x < CarDetected.nCenterLinePosX && CarDetected.vResult[i].Point_L.x > CarDetected.nCenterLinePosX) )
			   )
			{
				FuncImg.DrawCross( Display_ImageNsize,s_w, s_h, s_bpp, CarDetected.vResult[i].Point_L, 4, RGB(0,0,255),3);
				FuncImg.DrawCross( Display_ImageNsize,s_w, s_h, s_bpp, CarDetected.vResult[i].Point_R, 4, RGB(0,0,255),3);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CarDetected.vResult[i].Point_L, CarDetected.vResult[i].Point_R, RGB(255,0,0));
			}
			else
			{
				CarDetected.vResult.erase( CarDetected.vResult.begin()+i);
				i=-1;
			}
		}
	}		
	CarDetected.nDist = CarDetected.LB1.y - (nCenter-nDist+10);

	if ( CarDetected.LB1.y > s_h*3/4 )
		CarDetected.nCenterLinePosX = 0;
	//////////////////////////////////////////////////////////////////////////
	///橫向找出引擎進氣口/////////////////////////////////////////////////////////////////
	int nW_Grille;
	nW_Grille = abs( CarDetected.LB1.x - CarDetected.RU1.x );
	double *GramW = new double[nW_Grille];
	memset( GramW, 0, sizeof(double)*nW_Grille);
	
	int nBinNo = 30;
	int nInterval = ceil( (float)nW_Grille/nBinNo)+1;
	double *GramBin = new double[nBinNo];
	memset( GramBin, 0, sizeof(double)*nBinNo);	
	
	p_In_ImageNpixel = In_ImageNpixel;
	dCount =0;
	
	for (i=CarDetected.RU1.y ; i > CarDetected.LB1.y ; i--)
	{
		for (j=CarDetected.LB1.x, k=0; j<CarDetected.RU1.x; j++, k++)
		{
			GramW[k]   = GramW[k] + In_ImageNpixel[i*s_w+j];
			GramBin[ k / nInterval ] = GramBin[ k / nInterval ] + In_ImageNpixel[i*s_w+j];
		}
		
	}
	for (i=0; i<nW_Grille; i++)
	{
		P1.x = CarDetected.LB1.x + i;
		P1.y = 0;
		P2.x = CarDetected.LB1.x + i;
		P2.y = GramW[i]/160;
		GramW[i] = P2.y;
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
	}
	for (i=0; i<nBinNo; i++)
	{
		P1.x = CarDetected.LB1.x + (i+0.5)*nInterval /*+ 0.5*nInterval*/;
		P1.y = 0;
		P2.x = CarDetected.LB1.x + (i+0.5)*nInterval /*+ 0.5*nInterval*/;
		P2.y = GramBin[i]/1000;
		GramBin[i] = P2.y;
		if( i<10 ) FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
		if( i>=10 && i<20 ) FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
		if( i>=20 && i<30 ) FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
	}	
	
	//////////////////////////////////////////////////////////////////////////
	delete []dTmpVector1;
	delete []dTmpVector2;
	delete []dTmpVector3;
	delete []dTmpVector4;
	
	delete []SEGBUFFER;
	delete []GramR;
	delete []GramL;
	delete []GramRT;
	delete []GramLT;
	delete []GramSum;
	delete []GramT;
	delete []GramW;
 	delete []GramBin;
}
void CRogerFunction::HistVERTLINE( CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH, BOOL DISPLAY_SWITCH1 )
///////////
{
	CImgProcess FuncImg;
	int i, j, k;
	CPoint P1, P2;
	CenterLine LocalMax;
	vector <CenterLine> Local_Line;
	vector <CenterLine> Local_LineNew;
	double *GramR = new double[s_h];
	memset( GramR, 0, sizeof(double)*s_h);
	double *GramL = new double[s_h];
	memset( GramL, 0, sizeof(double)*s_h);

	double *GramRT = new double[s_h];
	memset( GramRT, 0, sizeof(double)*s_h);
	double *GramLT = new double[s_h];
	memset( GramLT, 0, sizeof(double)*s_h);

	double *GramSum = new double[s_h];
	memset( GramSum, 0, sizeof(double)*s_h);	
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);

	double *HistDensityH = new double[s_h];
	memset( HistDensityH, 0, sizeof(double)*s_h);

	BYTE *p_In_ImageNpixel = In_ImageNpixel;
	BYTE *p_In_ImageNpixel2 = In_ImageNpixel2;
	int *p_In_DirectionMap= In_DirectionMap;
	
	//////////////////////////////////////////////////////////////////////////
	double dCount =0;
	for (i=0; i<s_h; i++, p_In_ImageNpixel+=s_w, p_In_ImageNpixel2+=s_w)
	{
		for (j=CarDetected.nCenterLinePosX, k= CarDetected.nCenterLinePosX; j<CarDetected.nCenterLinePosX+30; j++, k--)
		{
			GramR[i]   = GramR[i] + p_In_ImageNpixel[j];
			GramL[i]   = GramL[i] + p_In_ImageNpixel[k];
			HistDensityH[i] += (p_In_ImageNpixel[j] + p_In_ImageNpixel[k])/60;
		}
	}
	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / 150;
		GramL[i] = GramL[i] / 150;
		GramT[i] = GramL[i] + GramR[i];
	}
	dCount /= s_h;
	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
			GramRT[i]  += GramR[i+j]/k;
			GramLT[i]  += GramL[i+j]/k;
		}
	} 
	//////////////////////////////////////////////////////////////////////////
	memcpy( GramR, GramRT, sizeof(double)*s_h);
	memcpy( GramL, GramLT, sizeof(double)*s_h);
	for (i=0; i<s_h; i++)
	{
		if ( GramSum[i] < 16 )
		{
			GramSum[i] = 0;
		}
	}
	//中心區域加強權重////////////////////////////////////////////////////
	for (i=0; i<s_h; i++)
	{
		if ( i > CarDetected.nMinY-20 && i < CarDetected.nMaxY+20 )
		{
			GramSum[i] = 2 * GramSum[i];
		}
	}
	//Find Local Maximum
	double MaxV;
	k = 5;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}
	for (i=10; i<s_h-10; i++)
	{
	   if(  DISPLAY_SWITCH1 == TRUE   )
	   {//histogram
// 		   P1 = CPoint( CarDetected.nCenterLinePosX,i);
// 		   P2 = CPoint( CarDetected.nCenterLinePosX+ GramR[i]/2 , i);
// 		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
// 		   P1 = CPoint( CarDetected.nCenterLinePosX,i);
// 		   P2 = CPoint( CarDetected.nCenterLinePosX- GramL[i]/2 , i);
// 		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
	   }
	   if( DISPLAY_SWITCH1 == TRUE )
	   {
		   P1 = CPoint( 0, i);
		   P2 = CPoint( GramSum[i]/10, i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	   }
	   if ( GramSum[i] > (0.6 *dCount) )
	   { 
		   if(  DISPLAY_SWITCH1 == TRUE  )
		   {
			   P1 = CPoint(  CarDetected.nCenterLinePosX - 5,  i);
			   P2 = CPoint(  CarDetected.nCenterLinePosX + 5 , i);
			   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		   }
		   LocalMax.pCenter = CPoint( CarDetected.nCenterLinePosX, i );
		   LocalMax.BDISPLAY= FALSE;
		   LocalMax.dTotalS = GramSum[i];
		   Local_Line.push_back( LocalMax );
	   }
	}
	///////////////////////////////////////////////////////////
	int nDist;
	int nCnt = 0;
	////////////////////////////////////////////////////////////
	double *LocalHistogram = new double[Local_Line.size()];
	memset( LocalHistogram, 0, sizeof(double)*Local_Line.size());
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<Local_Line.size(); i++)
	{
		LocalHistogram[i] = Local_Line[i].dTotalS;
	}
 	I_BubbleSort( LocalHistogram, Local_Line.size() );

	int nMaxPosH;
	vector <TopPosDist> Top;
	TopPosDist TopInfo;
	for (i=0; i<Local_Line.size(); i++)
	{
		for( j =0; j< 12; j++)//找前12個peak
		{
			if ( Local_Line[i].dTotalS == LocalHistogram[j] )
			{
				Local_Line[i].BDISPLAY = TRUE;
				//找到1st最大平均強度
				if (  DISPLAY_SWITCH1 == TRUE  )
				{
					P1 = CPoint( 0 ,  Local_Line[i].pCenter.y );
					P2 = CPoint( s_w-1  , Local_Line[i].pCenter.y );
					FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
					CarDetected.cMP1 = P1;
				}
				////////////////////////////////////////////////
				nMaxPosH = Local_Line[i].pCenter.y;
				TopInfo.nPoxY = Local_Line[i].pCenter.y;
				TopInfo.nDist = 0;
				TopInfo.Flag  = FALSE;
				Top.push_back( TopInfo );
				Local_LineNew.push_back( Local_Line[i] );
			}

		}
	}

	///Top5////////////////////////////////////////////////////////////////////
// 	for (i=0; i<Top.size(); i++)
// 	{
// 		gpMsgbar->ShowMessage("%d: (%d)\n", i, s_h - Top[i]);
// 	}
	/////Sliding Windows///////////////////////////////////////////////////////////////
	int nDistConst; 
	double dAccum;
	nDist = 0;
	vector <Segment> SegPos;
	Segment SEG;
	for ( i=0; i<Local_LineNew.size(); i++)
	{
		dAccum = 0;
		
		if ( Local_LineNew[i].pCenter.y < s_h/4 )//以中心區域的y座標來限制目標高度
			nDistConst = s_h/4;
		else if( Local_LineNew[i].pCenter.y >= s_h/4 && Local_LineNew[i].pCenter.y < s_h*2/3 )
			nDistConst = s_h/5;
		else if( Local_LineNew[i].pCenter.y >= s_h*2/3 && Local_LineNew[i].pCenter.y < s_h/6 )
			nDistConst = s_h/6;
		else
			nDistConst = s_h/7;
		// 		gpMsgbar->ShowMessage("slide Y: %d(%d) slide w: %d\n", s_h-Local_LineNew[i].pCenter.y, Local_LineNew[i].pCenter.y, nDistConst );
		for (j=i; j<Local_LineNew.size(); j++)//高度太小的區間忽略
		{
			
			nDist = abs( Local_LineNew[i].pCenter.y - Local_LineNew[j].pCenter.y );
			if ( i != j && nDist <= nDistConst )
			{
				dAccum += Local_LineNew[j].dTotalS;
				SEG.nStartH = Local_LineNew[i].pCenter.y;
				SEG.nEndH   = Local_LineNew[j].pCenter.y;
				SEG.nCenterX= Local_LineNew[i].pCenter.x;
				SEG.dTotal = dAccum;
				SEG.Remain = FALSE;
				SegPos.push_back( SEG );
			}
		}
	}
	//sliding windows 找出最大值
	double *SEGBUFFER = new double[ SegPos.size() ];
	memset( SEGBUFFER, 0, sizeof(double)*SegPos.size());
	for (i=0; i<SegPos.size(); i++)
		SEGBUFFER[i] = SegPos[i].dTotal;
	I_BubbleSort( SEGBUFFER, SegPos.size() );
	
	for (i=0; i<SegPos.size(); i++)
	{
		if ( SegPos[i].dTotal == SEGBUFFER[0] )
		{
			SegPos[i].Remain = TRUE;
		}
		else
		{
			SegPos.erase( SegPos.begin()+i );
			i = 0;
		}
	}
// 	gpMsgbar->ShowMessage("seg: %d\n", SegPos.size());
	for (i=0; i<SegPos.size(); i++)
	{
		int test = SegPos[i].nStartH;
		if ( SegPos[i].nStartH < s_h/4 )
			nDist = s_h/4;
		else if( SegPos[i].nStartH >= s_h/4 && SegPos[i].nStartH < s_h*2/3 )
			nDist = s_h/5;
		else if( SegPos[i].nStartH >= s_h*2/3 && SegPos[i].nStartH < s_h/6 )
			nDist = s_h/6;
		else
			nDist = s_h/7;
// 		nDist = abs(SegPos[i].nEndH - SegPos[i].nStartH);
		// 			nDist *= 1;
		P1 = CPoint(SegPos[i].nCenterX-nDist,SegPos[i].nStartH);
		P2 = CPoint(SegPos[i].nCenterX+nDist,SegPos[i].nStartH+nDist);
		CarDetected.LB1 = P1;
		CarDetected.RU1 = P2;
		nMaxPosH = P2.y - P1.y;
// 		FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, CarDetected.LB1, CarDetected.RU1, RGB(0,0,255));
		
		P1 = CPoint(SegPos[i].nCenterX-(nDist*2.4),SegPos[i].nStartH-10);
		P2 = CPoint(SegPos[i].nCenterX+(nDist*2.4),SegPos[i].nEndH+nDist*2.5);
		CarDetected.LB2 = P1;
		CarDetected.RU2 = P2;
	}
	//////////////////////////////////////////////////////////////////////////
	//找尋地面陰影
	CarDetected.dShadowsGradient =0;
	CarDetected.dShadowsIntensity=0;
	CarDetected.nDistGradient=0;
	CarDetected.nDistIntensity=0;
	nDist = 0;
	int nExplore;
	int nExploreWidth;
	nExplore      = 50 ;
	nExploreWidth = 15;
	//////////////////////////////////////////////////////////////////////////
	if ( CarDetected.LB1.y < s_h/4 )
		nExplore = s_h/4;
	else if ( CarDetected.LB1.y < s_h*2/3 )
		nExplore = s_h/5;
	else
		nExplore = s_h/6;

// 	gpMsgbar->ShowMessage("Explore: %d\n", nExplore);
	//////////////////////////////////////////////////////////////////////////
	int nCenter = CarDetected.LB1.y+20;
	int nBt = CarDetected.LB1.y - nExplore;

	if( nBt <= 0 ) nBt = 0;
	int nBinSize = abs(nCenter - nBt) ;
	double *dTmpVector1 = new double[nBinSize];
	double *dTmpVector2 = new double[nBinSize];
	memset( dTmpVector1, 0, sizeof(double)*nBinSize);
	memset( dTmpVector2, 0, sizeof(double)*nBinSize );
	for ( i = nCenter , k =0; i > nBt ; i--, k++)
	{
		for (j=0; j< 8 ; j++)
		{
			if( i+j <= 0)
			{
				dTmpVector1[k] += 1000;
 				i = nBt;
			}
			else
			{
				dTmpVector1[k] += HistDensityH[i+j]/10;
			}
		}
	}
	memcpy( dTmpVector2, dTmpVector1, sizeof(double)*nBinSize);
	I_BubbleSort( dTmpVector2, nBinSize );
	vector <int> vDist;
	for (i=0; i<nBinSize; i++)
	{
		if ( dTmpVector1[i] == dTmpVector2[nBinSize-1])
		{
			nDist = i;
			vDist.push_back( i );
			i = nBinSize;
		}
	}
	//可刪除
	if (  DISPLAY_SWITCH1 == TRUE  )
		for (i=0; i<5; i++)
		{
			P1 = CPoint(CarDetected.nCenterLinePosX-i, nCenter-nDist);
			P2 = CPoint(CarDetected.nCenterLinePosX+i, nCenter-nDist+10);
			/*if ( DISPLAY_SWITCH == TRUE )*/ FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
		}
	//////////////////////////////////////////////////////////////////////////
	CarDetected.nDistIntensity = nDist;
	if ( nBt ==0 && nCenter == 0 )
		CarDetected.dShadowsIntensity = 0;
	else
		CarDetected.dShadowsIntensity = dTmpVector2[nBinSize-1];

// 	gpMsgbar->ShowMessage("..............Modify............\n");

	CarDetected.LB1.y = nCenter-nDist+10;
	CarDetected.LB2.y = nCenter-nDist+10;

	//////////////////////////////////////////////////////////////////////////
	//找尋引擎蓋

	int nHeightSource = CarDetected.RU1.y - CarDetected.LB1.y;
	int nWidthSource = CarDetected.RU1.x - CarDetected.LB1.x;
	CPoint SourceRU = CarDetected.RU1;
	int nHeightModify;

// 	FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, CarDetected.LB1, CarDetected.RU1, RGB(0,0,255));

	//找出離車頭最近的大區域start
	nBinSize = Top.size();
	int *SortTmp = new int[nBinSize];
	memset( SortTmp, 0, sizeof(int)*nBinSize);
	int nUp = CarDetected.RU1.y + 10;
	SortTmp[0] = 0;
// 	gpMsgbar->ShowMessage("[nUp: %d]", nUp);
	/////////////////////////////////////
	for (i=1 ; i<Top.size()-1; i++)
	/////////////////////////////////////
	{
		if ( Top[i].nPoxY >= (nUp) )
		{
			Top[i].nDist = abs(Top[i].nPoxY - Top[i-1].nPoxY);
			SortTmp[i] = Top[i].nDist;
			Top[i].Flag = TRUE;
// 			gpMsgbar->ShowMessage("[Y:%d_%d]", Top[i].nPoxY, Top[i].nDist);
		}
		else
		{
			Top[i].nDist = abs(Top[i].nPoxY - Top[i-1].nPoxY);
		}
	}
// 	gpMsgbar->ShowMessage("\n");
	I_BubbleSort( SortTmp, Top.size() );
	TopPosDist *Lmax = new TopPosDist[3];

	BOOL F1=FALSE, F2=FALSE, F3=FALSE;
	for (i=0; i< Top.size(); i++)
	{
		if( Top[i].nDist == SortTmp[0] && F1 == FALSE && Top[i].Flag == TRUE )
		{
			Lmax[0].nDist = SortTmp[0];
			Lmax[0].nPoxY = Top[i].nPoxY;
			F1 = TRUE;
		}else if( Top[i].nDist == SortTmp[1] && F2 == FALSE && Top[i].Flag == TRUE)
		{
			Lmax[1].nDist = SortTmp[1];
			Lmax[1].nPoxY = Top[i].nPoxY;
			F2 = TRUE;
		}else if( Top[i].nDist == SortTmp[2] && F3 == FALSE && Top[i].Flag == TRUE)
		{
			Lmax[2].nDist = SortTmp[2];
			Lmax[2].nPoxY = Top[i].nPoxY;
			F3 = TRUE;
		}
	}
	//找出比例相當的前兩名
// 	int nMax=0;
// 	SortTmp[0] = abs(Lmax[0].nDist-Lmax[1].nDist);
// 	SortTmp[1] = abs(Lmax[1].nDist-Lmax[2].nDist);
// 	SortTmp[2] = abs(Lmax[2].nDist-Lmax[0].nDist);
// 	nMax = max(SortTmp[0], max(SortTmp[1], SortTmp[2]));
// 	for (i=0; i<3; i++)
// 	{
// 		for (j=1; j<3; j++)
// 		{
// 			if( (Lmax[i].nDist - Lmax[j].nDist) == nMax )
// 			{
// 				Lmax[j].nDist = 0;
// 				j=3;
// 				i=3;
// 			}
// 		}
// 	}

	//////////////////////////////////////////////////////////////////////////
	int nMin = 1000;
	for (i=0; i< 3; i++)
	{
// 		gpMsgbar->ShowMessage("Sort:(%d)[Y:%d_%d]",i+1,Lmax[i].nPoxY, Lmax[i].nDist);
		nDist = abs(Lmax[i].nPoxY - nUp);
		if ( Lmax[i].nPoxY >= (nUp) && nDist <= nMin && Lmax[i].nDist > 10 )
		{
			nMin = nDist;
			TopInfo.nPoxY = Lmax[i].nPoxY;
			TopInfo.nDist = Lmax[i].nDist;
		}
	}
// 	gpMsgbar->ShowMessage("\n");
// 	gpMsgbar->ShowMessage("Select dist: %d\n", nMin );
// 	gpMsgbar->ShowMessage("Select:[Y:%d_%d]\n\n", TopInfo.nPoxY, TopInfo.nDist);
	P1.x = CarDetected.nCenterLinePosX;
	P1.y = TopInfo.nPoxY;
	if( TopInfo.nPoxY >= s_h -2)
		CarDetected.RU1.y = s_h - 2;
	else 
		CarDetected.RU1.y = TopInfo.nPoxY;

	delete []SortTmp;
	delete []Lmax;
	//找出離車頭最近的大區域end
	if (  DISPLAY_SWITCH1 == TRUE  )
		for (i=0; i<5; i++)
		{
			P1 = CPoint(CarDetected.nCenterLinePosX-i, CarDetected.RU1.y);
			P2 = CPoint(CarDetected.nCenterLinePosX+i, CarDetected.RU1.y+10);
			/*if ( DISPLAY_SWITCH == TRUE )*/ FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
		}
	//////////////////////////////////////////////////////////////////////////
	nDist = abs(CarDetected.RU1.y - CarDetected.LB1.y);
	double dVHratio = 0.9;
	if ( CarDetected.nCenterLinePosX - dVHratio*nDist <= 0 )
		CarDetected.LB1.x = 0;
	else
		CarDetected.LB1.x = CarDetected.nCenterLinePosX - dVHratio*nDist;
	if ( CarDetected.nCenterLinePosX + dVHratio*nDist >= s_w )
		CarDetected.RU1.x = s_w-1;
	else
		CarDetected.RU1.x = CarDetected.nCenterLinePosX + dVHratio*nDist;
	//////////////////////////////////////////////////////////////////////////
	nHeightModify = abs(CarDetected.RU1.y-CarDetected.LB1.y);
	nWidthSource = CarDetected.RU1.x - CarDetected.LB1.x;
	double dRatio;
	int    nOffset ;
	int    nOffset1 = 0;
	int nSquare = nHeightModify * nWidthSource;
	dRatio = (double)nSquare / (s_h-CarDetected.LB1.y);
// 	gpMsgbar->ShowMessage("修正前 %.3f\n", dRatio);
	//////////////////////////////////////////////////////////////////////////
	if ( dRatio > 104 || dRatio < 30 )
	{
		if( dRatio > 104 )
		{
			CarDetected.RU1 = SourceRU;
			nOffset=1;
		}
		else
		{
			nOffset=0.5;
			nOffset1 = 30;
		}


		if ( CarDetected.RU1.y < s_h/4 )
			nDist = s_h/4 * nOffset;
		else if( CarDetected.RU1.y >= s_h/4 && CarDetected.RU1.y < s_h*2/3 )
			nDist = s_h/6* nOffset;
		else if( CarDetected.RU1.y >= s_h*2/3 && CarDetected.RU1.y < s_h/6 )
			nDist = s_h/9;
		else
			nDist = s_h/10;
		double *Engine = new double[nDist];
		double *EngineTmp = new double[nDist];
		memset( Engine, 0, sizeof(double)*nDist );
		for (i=CarDetected.RU1.y+nOffset1, k=0; i<CarDetected.RU1.y+nDist+nOffset1; i++, k++)
		{
			for (j=CarDetected.nCenterLinePosX-10; j<CarDetected.nCenterLinePosX+10; j++)
			{
				Engine[k] += In_ImageNpixel[i*s_w+j];
			}
		}
		k = 5;
		for (i=0; i<nDist-k; i++)
		{
			for( j=0; j<k; j++)
			Engine[i] += (Engine[j])/k;
		}
		for (i=CarDetected.RU1.y, k=0; k<nDist; i++, k++)
		{
			P1.x = 0;
			P1.y = i;
			P2.x = Engine[k]/100;
			P2.y = i;
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0) );
		}
		memcpy( EngineTmp, Engine, sizeof(double)*nDist );
		I_BubbleSort( EngineTmp, nDist );
		int nIdx = 0;
		for (i=0; i<nDist; i++)
		{
			if ( Engine[i] == EngineTmp[0] )
			{
				nIdx = i;
				i = nDist;
			}
		}
	
		CarDetected.RU1.y = CarDetected.RU1.y + nIdx;

		for (i=0; i<5; i++)
		{
			P1 = CPoint(CarDetected.nCenterLinePosX-i, CarDetected.RU1.y);
			P2 = CPoint(CarDetected.nCenterLinePosX+i, CarDetected.RU1.y+10);
			/*if ( DISPLAY_SWITCH == TRUE )*/ FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		}
		delete []Engine;
		delete []EngineTmp;
	}
	/////////////////////////////////////////////////////////////////////////////////////
	nDist = abs(CarDetected.RU1.y - CarDetected.LB1.y);
	dVHratio = 0.9;
	if ( CarDetected.nCenterLinePosX - dVHratio*nDist <= 0 )
		CarDetected.LB1.x = 0;
	else
		CarDetected.LB1.x = CarDetected.nCenterLinePosX - dVHratio*nDist;
	if ( CarDetected.nCenterLinePosX + dVHratio*nDist >= s_w )
		CarDetected.RU1.x = s_w-1;
	else
		CarDetected.RU1.x = CarDetected.nCenterLinePosX + dVHratio*nDist;
	//////////////////////////////////////////////////////////////////////////
// 	P1 = CarDetected.LB1;
// 	P2 = CarDetected.RU1;
// 	if ( DISPLAY_SWITCH == TRUE ) FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));

// 	if ( DISPLAY_SWITCH1 == TRUE  )
// 	{
// 		for (i=0; i<CarDetected.vResult.size(); i++  )
// 		{
// 			if ( CarDetected.vResult[i].CenterPoint.x > CarDetected.LB1.x && CarDetected.vResult[i].CenterPoint.x < CarDetected.RU1.x && 
// 				 CarDetected.vResult[i].CenterPoint.y > CarDetected.LB1.y && CarDetected.vResult[i].CenterPoint.y < CarDetected.RU1.y &&
// 				(( CarDetected.vResult[i].Point_L.x > CarDetected.LB1.x && CarDetected.vResult[i].Point_R.x < CarDetected.RU1.x && CarDetected.vResult[i].Point_L.x < CarDetected.nCenterLinePosX && CarDetected.vResult[i].Point_R.x > CarDetected.nCenterLinePosX) || 
// 				 ( CarDetected.vResult[i].Point_R.x > CarDetected.LB1.x && CarDetected.vResult[i].Point_L.x < CarDetected.RU1.x && CarDetected.vResult[i].Point_R.x < CarDetected.nCenterLinePosX && CarDetected.vResult[i].Point_L.x > CarDetected.nCenterLinePosX) )
// 			   )
// 			{
// 				FuncImg.DrawCross( Display_ImageNsize,s_w, s_h, s_bpp, CarDetected.vResult[i].Point_L, 4, RGB(0,255,0),3);
// 				FuncImg.DrawCross( Display_ImageNsize,s_w, s_h, s_bpp, CarDetected.vResult[i].Point_R, 4, RGB(0,255,0),3);
// 				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, CarDetected.vResult[i].Point_L, CarDetected.vResult[i].Point_R, RGB(0,255,0));
// 			}
// 			else
// 			{
// 				CarDetected.vResult.erase( CarDetected.vResult.begin()+i);
// 				i=-1;
// 			}
// 		}
// 	}		
// 	CarDetected.nDist = CarDetected.LB1.y - (nCenter-nDist+10);
// 
// 	if ( CarDetected.LB1.y > s_h*3/4 )
// 		CarDetected.nCenterLinePosX = 0;
	//////////////////////////////////////////////////////////////////////////
	///橫向找出引擎進氣口/////////////////////////////////////////////////////////////////
	int nW_Grille;
	nW_Grille = abs( CarDetected.LB1.x - CarDetected.RU1.x );
	double *GramW = new double[nW_Grille];
	memset( GramW, 0, sizeof(double)*nW_Grille);

	double *GramDir = new double[nW_Grille];
	memset( GramDir, 0, sizeof(double)*nW_Grille );
	
	int nBinNo = 30;
	int nInterval = ceil((double)nW_Grille/nBinNo)+1;
	double *GramBin = new double[nBinNo];
	memset( GramBin, 0, sizeof(double)*nBinNo);	
	
	//垂直HISTOGRAM
	p_In_ImageNpixel = In_ImageNpixel;
	dCount =0;
	for (i=CarDetected.RU1.y ; i > CarDetected.LB1.y ; i--)
	{
		for (j=CarDetected.LB1.x, k=0; j<CarDetected.RU1.x; j++, k++)
		{
			GramW[k]   = GramW[k] + In_ImageNpixel[i*s_w+j];
		}
		
	}
	for (i=0; i<nW_Grille; i++)
	{
		P1.x = CarDetected.LB1.x + i;
		P1.y = 0;
		P2.x = CarDetected.LB1.x + i;
		P2.y = GramW[i]/160;
		GramW[i] = P2.y;
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
	}
	//////////////////////////////////////////////////////////////////////////
	if ( CarDetected.LB1.x <  0    ) CarDetected.LB1.x = 0;
	if ( CarDetected.RU1.x >= s_w  ) CarDetected.RU1.x = s_w -1;
	if ( CarDetected.RU1.y >= s_h  ) CarDetected.RU1.y = s_h -1;
	if ( CarDetected.LB1.y < 0     ) CarDetected.LB1.y = 0;
	//////////////////////////////////////////////////////////////////////////
	delete []dTmpVector1;
	delete []dTmpVector2;
 	delete []SEGBUFFER;
	delete []GramR;
	delete []GramL;
	delete []GramRT;
	delete []GramLT;
	delete []GramSum;
	delete []GramT;
//  	delete []GramW;
	delete []LocalHistogram;

}
void CRogerFunction::Analysis_CarDetected( CARINFO &CarDetected, CARINFO &CarDataBase, BYTE *Display_ImageNsize, int s_w, int s_h, int Offset, BOOL DISPLAY_SWITCH )
{
	CImgProcess FuncImg;
	int i, j;
	CPoint P1, P2;
	double dSURFmatchScore;
	double dScore1 =0;
	double dScore2 =0;
	//////////////////////////////////////////////////////////////////////////
	double dScoreW;
	double dScoreH;
	double dScoreW1;
	double dScoreH1;

	I_Correlation( CarDetected.HistogramW, CarDataBase.HistogramW, s_w, dScoreW );
	gpMsgbar->ShowMessage("W相似度: %.3f ...", dScoreW );
	I_Correlation( CarDetected.HistogramH, CarDataBase.HistogramH, s_h, dScoreH );
	gpMsgbar->ShowMessage("H相似度: %.3f\n", dScoreH );
	I_Correlation( CarDetected.HistogramW1, CarDataBase.HistogramW1, 70, dScoreW1 );
	gpMsgbar->ShowMessage("W1相似度: %.3f ...", dScoreW1 );
	I_Correlation( CarDetected.HistogramH1, CarDataBase.HistogramH1, 30, dScoreH1 );
	gpMsgbar->ShowMessage("H1相似度: %.3f\n", dScoreH1 );
	gpMsgbar->ShowMessage("CarDetected:RU1(%d,%d)LB1(%d,%d)RU2(%d,%d)LB2(%d,%d)\n",CarDetected.RU1.x, CarDetected.RU1.y,CarDetected.LB1.x,CarDetected.LB1.y,CarDetected.RU2.x, CarDetected.RU2.y,CarDetected.LB2.x,CarDetected.LB2.y);
	gpMsgbar->ShowMessage("CarDataBase:RU1(%d,%d)LB1(%d,%d)RU2(%d,%d)LB2(%d,%d)\n",CarDataBase.RU1.x, CarDataBase.RU1.y,CarDataBase.LB1.x,CarDataBase.LB1.y,CarDataBase.RU2.x, CarDataBase.RU2.y,CarDataBase.LB2.x,CarDataBase.LB2.y);

	//加 weighting
	int Weight11 = 0;
	int Weight12 = 0;
	int Weight21 = 0;
	int Weight22 = 0;
	// 	gpMsgbar->ShowMessage("RU:(%d,%d) LB:(%d,%d)\n", CarDetected.RU11.x, CarDetected.RU11.y, CarDetected.LB11.x, CarDetected.LB11.y );
	
	for ( i=0; i<CarDetected.vMatchPT.size(); i++)
	{
		if ( (  CarDetected.vMatchPT[i].mPoint1.x <=  CarDetected.RU1.x && CarDetected.vMatchPT[i].mPoint1.x >= CarDetected.LB1.x ) && (  CarDetected.vMatchPT[i].mPoint1.y >=  CarDetected.LB1.y && CarDetected.vMatchPT[i].mPoint1.y <= CarDetected.RU1.y ) )
		{
			CarDetected.vHeadMatch.push_back( CarDetected.vMatchPT[i] );
			Weight11++;
		}
		else if ( (  CarDetected.vMatchPT[i].mPoint1.x <=  CarDetected.RU2.x && CarDetected.vMatchPT[i].mPoint1.x >= CarDetected.LB2.x ) && (  CarDetected.vMatchPT[i].mPoint1.y >=  CarDetected.LB2.y && CarDetected.vMatchPT[i].mPoint1.y <= CarDetected.RU2.y ) )
		{
			CarDetected.vBodyMatch.push_back( CarDetected.vMatchPT[i] );
			Weight12++;
		}
	}
	for ( i=0; i<CarDetected.vMatchPT.size(); i++)
	{
 		if ( (  CarDetected.vMatchPT[i].mPoint2.x <=  CarDataBase.RU1.x+Offset && CarDetected.vMatchPT[i].mPoint2.x >= CarDataBase.LB1.x+Offset ) && (  CarDetected.vMatchPT[i].mPoint2.y >=  CarDataBase.LB1.y && CarDetected.vMatchPT[i].mPoint2.y <= CarDataBase.RU1.y ) )
 		{
			CarDataBase.vHeadMatch.push_back( CarDetected.vMatchPT[i] );
			Weight21++;
 		}
 		else if ( (  CarDetected.vMatchPT[i].mPoint2.x <=  CarDataBase.RU2.x+Offset && CarDetected.vMatchPT[i].mPoint2.x >= CarDataBase.LB2.x+Offset ) && (  CarDetected.vMatchPT[i].mPoint2.y >=  CarDataBase.LB2.y && CarDetected.vMatchPT[i].mPoint2.y <= CarDataBase.RU2.y ) )
 		{
			CarDataBase.vBodyMatch.push_back( CarDetected.vMatchPT[i] );
 			Weight22++;
 		}
	}
	gpMsgbar->ShowMessage("第一層(原始車頭Match點): %d <<>> (資料庫車頭Match點): %d\n", Weight11, Weight21 );
	gpMsgbar->ShowMessage("第二層(原始車身Match點): %d <<>> (資料庫車身Match點): %d\n", Weight12, Weight22 );
	gpMsgbar->ShowMessage("車頭區域: %d 點Matched\n",CarDetected.vHeadMatch.size());
	gpMsgbar->ShowMessage("車身區域: %d 點Matched\n",CarDetected.vBodyMatch.size());
	//////////////////////////////////////////////////////////////////////////
	//顯示Matching 點 與 連線
// 	for (i=0; i<CarDetected.vHeadMatch.size(); i++)
// 	{
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, CarDetected.vHeadMatch[i].mPoint1, CarDetected.vHeadMatch[i].mPoint2, RGB(0,255,0));
// 	}
// 	for (i=0; i<CarDetected.vBodyMatch.size(); i++)
// 	{
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, CarDetected.vBodyMatch[i].mPoint1, CarDetected.vBodyMatch[i].mPoint2, RGB(255,0,255));
// 	}
	/////找出車頭重心//////////////////////////////////////////////////////////
	CarDetected.HeadCentroid1 = CPoint(0,0);
	CarDetected.HeadCentroid2 = CPoint(0,0);
	if ( CarDetected.vHeadMatch.size() > 0 )
	{

		for ( i=0; i<CarDetected.vHeadMatch.size(); i++ )
		{
			CarDetected.HeadCentroid1.x += CarDetected.vHeadMatch[i].mPoint1.x;
			CarDetected.HeadCentroid1.y += CarDetected.vHeadMatch[i].mPoint1.y;
			CarDetected.HeadCentroid2.x += CarDetected.vHeadMatch[i].mPoint2.x;
			CarDetected.HeadCentroid2.y += CarDetected.vHeadMatch[i].mPoint2.y;
		}
		CarDetected.HeadCentroid1.x /= CarDetected.vHeadMatch.size();
		CarDetected.HeadCentroid1.y /= CarDetected.vHeadMatch.size();
		CarDetected.HeadCentroid2.x /= CarDetected.vHeadMatch.size();
		CarDetected.HeadCentroid2.y /= CarDetected.vHeadMatch.size();
		FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid1, 8, RGB(0,0,255), 3 );
		FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid1, 6, RGB(255,255,0), 3 );
		FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid1, 12, RGB(255,0,0), 1 );

		FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid2, 8, RGB(0,0,255), 3 );
		FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid2, 6, RGB(255,255,0), 3 );
		FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid2, 12, RGB(255,0,0), 1 );

	}
	//////////////////////////////////////////////////////////////////////////
	//車頭surf點至重心畫線
// 	for ( i=0; i<CarDetected.vHeadMatch.size(); i++)
// 	{
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid1, CarDetected.vHeadMatch[i].mPoint1, RGB(255,255,255) );
// 		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, CarDetected.HeadCentroid2, CarDetected.vHeadMatch[i].mPoint2, RGB(255,255,255) );
// 		
// 	}
	//計算車頭SURF最小Matching距離
	dSURFmatchScore = 0;
	CarDetected.dHSURFsimilar = 2;
	if ( CarDetected.vHeadMatch.size() != 0 )
	{
		for ( i=0; i<CarDetected.vHeadMatch.size(); i++ )
		{
			dSURFmatchScore += CarDetected.vHeadMatch[i].dSURF_MatchMinDist;
			FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.vHeadMatch[i].mPoint1 , 5, RGB(0,255,0), 3 );
			FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.vHeadMatch[i].mPoint2 , 5, RGB(0,255,0), 3 );
		}
		dSURFmatchScore /= CarDetected.vHeadMatch.size();
		gpMsgbar->ShowMessage("車頭SURF Matching平均最小距離(越小越好):%.3f\n", dSURFmatchScore);
		CarDetected.dHSURFsimilar = dSURFmatchScore;
	
	}
	//////////////////////////////////////////////////////////////////////////
	//計算車身SURF最小Matching距離
	dSURFmatchScore = 0;
	CarDetected.dBSURFsimilar = 2;
	if ( CarDetected.vBodyMatch.size() != 0 )
	{
		for ( i=0; i<CarDetected.vBodyMatch.size(); i++ )
		{
			dSURFmatchScore += CarDetected.vBodyMatch[i].dSURF_MatchMinDist;
			FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.vBodyMatch[i].mPoint1 , 5, RGB(255,0,255), 3 );
			FuncImg.DrawCross( Display_ImageNsize, s_w, s_h, 3, CarDetected.vBodyMatch[i].mPoint2 , 5, RGB(255,0,255), 3 );
		}
		dSURFmatchScore /= CarDetected.vBodyMatch.size();
		gpMsgbar->ShowMessage("車身SURF Matching平均最小距離(越小越好):%.3f\n", dSURFmatchScore);
		CarDetected.dBSURFsimilar = dSURFmatchScore;
	}
	//////////////////////////////////////////////////////////////////////////
	if ( CarDetected.vHeadMatch.size() > 1 )
	{
		//找出車頭surf點y軸的範圍
		int nyMax1 = 0;
		int nyMin1 = 1000;
		int nyMax2 = 0;
		int nyMin2 = 1000;
		int nDistX1, nDistX2;
		int nDistY1, nDistY2;
		if ( CarDetected.vHeadMatch.size() >= 2 )
		{
			for (i=0; i<CarDetected.vHeadMatch.size(); i++)
			{
				if ( CarDetected.vHeadMatch[i].mPoint1.y > nyMax1 )
				{
					nyMax1 = CarDetected.vHeadMatch[i].mPoint1.y; 
				}
				if ( CarDetected.vHeadMatch[i].mPoint1.y < nyMin1 )
				{
					nyMin1 = CarDetected.vHeadMatch[i].mPoint1.y; 
				}
				if ( CarDetected.vHeadMatch[i].mPoint2.y > nyMax2 )
				{
					nyMax2 = CarDetected.vHeadMatch[i].mPoint2.y; 
				}
				if ( CarDetected.vHeadMatch[i].mPoint2.y < nyMin2 )
				{
					nyMin2 = CarDetected.vHeadMatch[i].mPoint2.y; 
				}
			}
			nDistY1 = abs( nyMax1 - nyMin1 );
			nDistY2 = abs( nyMax2 - nyMin2 );
			gpMsgbar->ShowMessage("(1) yMAX:%d, yMIN:%d = %d\n", s_h - nyMax1, s_h - nyMin1, nDistY1 );
			gpMsgbar->ShowMessage("(2) yMAX:%d, yMIN:%d = %d\n", s_h - nyMax2, s_h - nyMin2, nDistY2 );
			P1 = CPoint(0, nyMin1);
			P2 = CPoint(50, nyMin1);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
			P1 = CPoint(0, nyMax1);
			P2 = CPoint(50, nyMax1);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
			P1 = CPoint(320, nyMin2);
			P2 = CPoint(370, nyMin2);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
			P1 = CPoint(320, nyMax2);
			P2 = CPoint(370, nyMax2);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
		}
		//////////////////////////////////////////////////////////////////////////
		//找出車頭surf點x軸的範圍
		int nxMax1 = 0;
		int nxMin1 = 1000;
		int nxMax2 = 0;
		int nxMin2 = 1000;
		if ( CarDetected.vHeadMatch.size() >= 2 )
		{
			for (i=0; i<CarDetected.vHeadMatch.size(); i++)
			{
				if ( CarDetected.vHeadMatch[i].mPoint1.x > nxMax1 )
				{
					nxMax1 = CarDetected.vHeadMatch[i].mPoint1.x; 
				}
				if ( CarDetected.vHeadMatch[i].mPoint1.x < nxMin1 )
				{
					nxMin1 = CarDetected.vHeadMatch[i].mPoint1.x; 
				}
				if ( CarDetected.vHeadMatch[i].mPoint2.x > nxMax2 )
				{
					nxMax2 = CarDetected.vHeadMatch[i].mPoint2.x; 
				}
				if ( CarDetected.vHeadMatch[i].mPoint2.x < nxMin2 )
				{
					nxMin2 = CarDetected.vHeadMatch[i].mPoint2.x; 
				}
			}
			nDistX1 = abs( nxMax1 - nxMin1 );
			nDistX2 = abs( nxMax2 - nxMin2 );
			gpMsgbar->ShowMessage("(1) xMAX:%d, xMIN:%d = %d\n", nxMax1, nxMin1, nDistX1 );
			gpMsgbar->ShowMessage("(2) xMAX:%d, xMIN:%d = %d\n", nxMax2, nxMin2, nDistX2 );
			P1 = CPoint(nxMin1, 0);
			P2 = CPoint(nxMin1, 50 );
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
			P1 = CPoint( nxMax1, 0);
			P2 = CPoint( nxMax1, 50);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
			P1 = CPoint( nxMin2, 0);
			P2 = CPoint( nxMin2, 50 );
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
			P1 = CPoint( nxMax2, 0);
			P2 = CPoint( nxMax2, 50);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(0,0,0));
		}
		//////////////////////////////////////////////////////////////////////////
		P1 = CPoint( nxMin1, nyMin1 );
		P2 = CPoint( nxMax1, nyMax1 );
		CarDetected.hLB1 = P1;
		CarDetected.hRU1 = P2;
		FuncImg.DrawRect( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(255,0,0));

		P1 = CPoint( nxMin2, nyMin2 );

		P2 = CPoint( nxMax2, nyMax2 );
		CarDataBase.hLB1 = P1;
		CarDataBase.hRU1 = P2;
		FuncImg.DrawRect( Display_ImageNsize, s_w, s_h, 3, P1, P2, RGB(255,0,0));
		gpMsgbar->ShowMessage("車頭SURF比例 X: %.2f, Y: %.2f\n", (double)nDistX1/(double)nDistX2, (double)nDistY1/(double)nDistY2 );
		//////////////////////////////////////////////////////////////////////////
		//把histogram抓出來比對
		gpMsgbar->ShowMessage("hLB(%d,%d), hRU(%d,%d)\n", CarDetected.hLB1.x, CarDetected.hLB1.y, CarDetected.hRU1.x, CarDetected.hRU1.y);
	}
	CarDetected.dHeadSimH = 0;
	CarDetected.dHeadSimW = 0;
	if ( CarDetected.vHeadMatch.size() > 1 && CarDataBase.vHeadMatch.size() > 1)
	{
		//第一張必須將HISTOGRAM放大或縮小,第二張DATABASE不要動
		int nSLength1 = CarDetected.hRU1.x-CarDetected.hLB1.x;
		int nSLength2 = CarDataBase.hRU1.x-CarDataBase.hLB1.x;	
		gpMsgbar->ShowMessage("S1: %d _ ", nSLength1 );
		gpMsgbar->ShowMessage("S2: %d\n", nSLength2 );
		//將二個不同寬度Histogram攤成一致
		int nSLength = max( nSLength1, nSLength2 );
// 		int nSLength = 40;
		double dSS;
		int *S1 = new int[nSLength];
		int *S2 = new int[nSLength];
		memset(S1,0,sizeof(int)*nSLength);
		memset(S2,0,sizeof(int)*nSLength);
		int *S1t = new int[nSLength];
		int *S2t = new int[nSLength];
		memset(S1t,0,sizeof(int)*nSLength);
		memset(S2t,0,sizeof(int)*nSLength);
		dSS = (double)nSLength / (double)nSLength1;
		for (i=CarDetected.hLB1.x, j=0 ; i<CarDetected.hRU1.x; i++, j++)
		{
			S1[(int)(dSS*j)] = CarDetected.HistogramW[i];
		}

		dSS = (double)nSLength / (double)nSLength2;
		for (i=CarDataBase.hLB1.x-Offset, j=0 ; i<CarDataBase.hRU1.x-Offset; i++, j++)
		{
			S2[(int)(dSS*j)] = CarDataBase.HistogramW[i];
		}
		for (i=0; i<nSLength-1; i++)
		{
			S1t[i] = S1[i]+S1[i+1];
			S2t[i] = S2[i]+S2[i+1];
		}
// 		for (i=0; i<nSLength; i++)
// 		{
// 			gpMsgbar->ShowMessage("[%d](%d)",i, S1t[i]);
// 		}
// 		gpMsgbar->ShowMessage("\n");
// 		for (i=0; i<nSLength; i++)
// 		{
// 			gpMsgbar->ShowMessage("[%d](%d)",i, S2t[i]);
// 		}
// 		gpMsgbar->ShowMessage("\n");
		/////////////////////////////////////////
		I_Correlation( S1t, S2t, nSLength, dScore1 );
		gpMsgbar->ShowMessage("W: Similarity: %f\n", dScore1 );
		CarDetected.dHeadSimW = dScore1;
		delete []S1;
		delete []S2;
	}

	//////////////////////////////////////////////////////////////////////////
	if ( CarDetected.vHeadMatch.size() > 1 && CarDataBase.vHeadMatch.size() > 1)
	{
		//第一張必須將HISTOGRAM放大或縮小,第二張DATABASE不要動
		int nSLength1 = CarDetected.hRU1.y-CarDetected.hLB1.y;
		int nSLength2 = CarDataBase.hRU1.y-CarDataBase.hLB1.y;	
		gpMsgbar->ShowMessage("SH1: %d _ ", nSLength1 );
		gpMsgbar->ShowMessage("SH2: %d\n", nSLength2 );
		//將二個不同寬度Histogram攤成一致
		int nSLength = max( nSLength1, nSLength2 );
// 		int nSLength = 40;
		double dSS;
		int *SH1 = new int[nSLength];
		int *SH2 = new int[nSLength];
		memset(SH1,0,sizeof(int)*nSLength);
		memset(SH2,0,sizeof(int)*nSLength);
		int *SH1t = new int[nSLength];
		int *SH2t = new int[nSLength];
		memset(SH1t,0,sizeof(int)*nSLength);
		memset(SH2t,0,sizeof(int)*nSLength);
		dSS = (double)nSLength / (double)nSLength1;
		for (i=CarDetected.hLB1.y, j=0 ; i<CarDetected.hRU1.y; i++, j++)
		{
			SH1[(int)(dSS*j)] = CarDetected.HistogramH[i];
		}

		dSS = (double)nSLength / (double)nSLength2;
		for (i=CarDataBase.hLB1.y, j=0 ; i<CarDataBase.hRU1.y; i++, j++)
		{
			SH2[(int)(dSS*j)] = CarDataBase.HistogramH[i];
		}
		for (i=0; i<nSLength-1; i++)
		{
			SH1t[i] = SH1[i]+SH1[i+1];
			SH2t[i] = SH2[i]+SH2[i+1];
		}
// 		for (i=0; i<nSLength; i++)
// 		{
// 			gpMsgbar->ShowMessage("[%d](%d)",i, SH1t[i]);
// 		}
// 		gpMsgbar->ShowMessage("\n");
// 		for (i=0; i<nSLength; i++)
// 		{
// 			gpMsgbar->ShowMessage("[%d](%d)",i, SH2t[i]);
// 		}
// 		gpMsgbar->ShowMessage("\n");
		/////////////////////////////////////////
		I_Correlation( SH1t, SH2t, nSLength, dScore2 );
		gpMsgbar->ShowMessage("H: Similarity: %f\n", dScore2 );
		CarDetected.dHeadSimH = dScore2;
		delete []SH1;
		delete []SH2;
		delete []SH1t;
		delete []SH2t;
	}
/////////////////////////////////////////////////////////////
// 	ofstream	fileOutput; 
// 	fileOutput.open("D:\\0LabVideo\\0out\\00.txt", ios::app);
// 	fileOutput << CarDetected.SourceFileName << "," << CarDetected.SourceFileName1 << "," << dScoreW << "," << dScoreH << "," << dScoreW1 << "," << dScoreH1 << "," 
// 		<< CarDetected.vBodyMatch.size() << ","
// 		<< CarDetected.vHeadMatch.size() << "," << CarDetected.dHSURFsimilar << "," << dScore1 << "," << dScore2		
// 		<< endl;
// 
// 	//////////////////////////////////////////////////////////////////////////
// 	fileOutput.close();
}
void CRogerFunction::FileTitle()
{
	///////////////////////////////////
// 	ofstream	fileOutput; 
// 	fileOutput.open("D:\\0LabVideo\\0Text\\00.txt", ios::out);
// 	//Analysis表頭
// // 	fileOutput << "Image1" << "," << "Image2" << "," << "HistW" << "," << "HistoH" << "," << "HistoW1" << "," << "HistoH1" << "," 
// // 		<< "車身Match數" << ","
// // 		<< "車頭Match數" << "," << "車頭SURF平均" << "," << "車頭W" << "," << "車頭H"		
// // 		<< endl;
// 	fileOutput << "目錄名" << "," << "車頭dSURF相似距離" << "," << "車身dSURF相似距離" << "," << "車頭點" << "," << "車身點" << "," << "車頭W相似surflocal" << "," "車頭H相似surflocal" << endl;
// 
// 	fileOutput.close();
	//////////////////////////////////////////////////////////////////////////
}


void CRogerFunction::FindCircleHOG( BYTE *In_ImageNpixel, int s_w, int s_h, CPoint Centroid, int Radius, double *GW )
{
	int x, y, i, j;
	memset( GW, 0, sizeof(double)*360 );
	for (i=0; i<360; i++)
	{
		for (j=0; j< Radius; j++)
		{
			x = Centroid.x + j * cos(i*0.017453292);
			y = Centroid.y + j * sin(i*0.017453292);
			if ( x < s_w && x >0 && y > 0 && y < s_h)
			{
				GW[i] += In_ImageNpixel[y*s_w+x];
			}
		}
	}
}
void CRogerFunction::FindCircleHOGNew1( BYTE *In_ImageNpixel, int s_w, int s_h, CPoint Centroid, int Radius, int nDegreeStart, int nDegreeEnd, double *GW )
{
	int x, y, i, j;
	int nSize = nDegreeEnd - nDegreeStart;
	memset( GW, 0, sizeof(double)*nSize );
	int circle_range = nSize/12;
	int degree_interval = nSize / circle_range ;
	int radius_interval = Radius / 12 +1;

	int nIdxRadius;
	int nIdxCircle;
	for (i=nDegreeStart+1; i<nDegreeEnd-1; i++)
	{
		for (j=0; j< Radius; j+=2)
		{
			x = Centroid.x + j * cos(i*0.017453292);
			y = Centroid.y + j * sin(i*0.017453292);
			nIdxRadius  = j/(radius_interval);
			nIdxCircle = (i/degree_interval);

			if ( nIdxCircle >=0 && nIdxCircle < 30 && nIdxRadius >=0 && nIdxRadius < 12 )
			{
				GW[ nIdxRadius *circle_range + nIdxCircle ] += In_ImageNpixel[y*s_w+x];
// 				if( (j/(radius_interval))*circle_range+(i/degree_interval) >= 180 )
// 					gpMsgbar->ShowMessage("\n");
				
			}
		}
	}
	double *Tmp = new double[nSize];
	memset(Tmp, 0, sizeof(double)*nSize);
	memcpy(Tmp, GW, sizeof(double)*nSize);
	I_BubbleSort(Tmp, nSize);
	for (i=0; i<nSize; i++) 
	{
		if( GW[i] < 0.0000000000000000000000001 && GW[i] != 0 )
			gpMsgbar->ShowMessage("\n");
		GW[i] /= (Tmp[0]+0.000000000001);
		if( GW[i] < 0.0000000000000000000000001 && GW[i] != 0 )
			gpMsgbar->ShowMessage("\n");


	}
	delete []Tmp;
}
void CRogerFunction::HistVERTLINE_NEW( CARINFO &CarDetected, double *GW1, double *GW2, double *GW3, BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH )
{
	CImgProcess FuncImg;
	int i, j, k, m, n;
	CPoint P1, P2;
	CenterLine LocalMax;
	vector <CenterLine> Local_Line;

	double *GramR = new double[s_h];
	memset( GramR, 0, sizeof(double)*s_h);
	double *GramL = new double[s_h];
	memset( GramL, 0, sizeof(double)*s_h);

	double *GramRT = new double[s_h];
	memset( GramRT, 0, sizeof(double)*s_h);
	double *GramLT = new double[s_h];
	memset( GramLT, 0, sizeof(double)*s_h);

	double *GramSum = new double[s_h];
	memset( GramSum, 0, sizeof(double)*s_h);	
	double *GramT = new double[s_h];
	memset( GramT, 0, sizeof(double)*s_h);

	double *HistogramH = new double[s_h];
	memset( HistogramH, 0, sizeof(double)*s_h);
	double *HistogramW = new double[s_w];
	memset( HistogramW, 0, sizeof(double)*s_w);

	//////////////////////////////////////////////////////////////////////////
	double dCount =0;
	for (i=0; i<s_h; i++)
	{
		for (j=CarDetected.nCenterLinePosX, k= CarDetected.nCenterLinePosX; j<CarDetected.nCenterLinePosX+30; j++, k--)
		{
			GramR[i]   = GramR[i] + In_ImageNpixel[i*s_w+j];
			GramL[i]   = GramL[i] + In_ImageNpixel[i*s_w+k]; 
		}
	}
	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / 150;
		GramL[i] = GramL[i] / 150;
		GramT[i] = GramL[i] + GramR[i];
		dCount += GramT[i];
	}
	dCount /= s_h;
	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
			GramRT[i]  += GramR[i+j]/k;
			GramLT[i]  += GramL[i+j]/k;
		}
	} 
	//////////////////////////////////////////////////////////////////////////
	memcpy( GramR, GramRT, sizeof(double)*s_h);
	memcpy( GramL, GramLT, sizeof(double)*s_h);

	for (i=0; i<s_h; i++)
	{
		if ( GramSum[i] < 16 )
		{
			GramSum[i] = 0;
		}
		HistogramH[i] = GramSum[i];
	}
	//中心區域加強權重////////////////////////////////////////////////////
	for (i=0; i<s_h; i++)
	{
		if ( i > CarDetected.nMinY-20 && i < CarDetected.nMaxY+20 )
		{
			GramSum[i] = 2 * GramSum[i];
		}
	}
	//Find Local Maximum
	double MaxV;
	k = 5;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}
	for (i=10; i<s_h-10; i++)
	{
	   if( DISPLAY_SWITCH == TRUE )
	   {
		   P1 = CPoint( CarDetected.nCenterLinePosX,i);
		   P2 = CPoint( CarDetected.nCenterLinePosX+ GramR[i]/2 , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
		   P1 = CPoint( CarDetected.nCenterLinePosX,i);
		   P2 = CPoint( CarDetected.nCenterLinePosX- GramL[i]/2 , i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
	   }
	   if( DISPLAY_SWITCH == TRUE )
	   {
		   P1 = CPoint( 0, i);
		   P2 = CPoint( GramSum[i], i);
		   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,255));
	   }
	   if ( GramSum[i] > (0.6 *dCount) )
	   { 
		   if( DISPLAY_SWITCH == TRUE )
		   {
			   P1 = CPoint(  CarDetected.nCenterLinePosX - 5,  i);
			   P2 = CPoint(  CarDetected.nCenterLinePosX + 5 , i);
			   FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
		   }
		   LocalMax.pCenter = CPoint( CarDetected.nCenterLinePosX, i );
		   LocalMax.BDISPLAY= FALSE;
		   LocalMax.dTotalS = GramSum[i];
		   Local_Line.push_back( LocalMax );
		}
	}
	///////////////////////////////////////////////////////////
	int nDist;
	int nCnt = 0;
	////////////////////////////////////////////////////////////
	double *LocalHistogram = new double[Local_Line.size()];
	memset( LocalHistogram, 0, sizeof(double)*Local_Line.size());
	////////////////////////////////////////////////////////////
	for (i=0; i<Local_Line.size(); i++)
	{
		LocalHistogram[i] = Local_Line[i].dTotalS;
	}
 	I_BubbleSort( LocalHistogram, Local_Line.size() );

	int nMaxPosH;
	for (i=0; i<Local_Line.size(); i++)
	{
		if ( Local_Line[i].dTotalS == LocalHistogram[0] )
		{
			Local_Line[i].BDISPLAY = TRUE;
			//找到1st最大平均強度
// 			if ( DISPLAY_SWITCH == TRUE )
// 			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1  , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,0));
				FindCircleHOG( In_ImageNpixel, s_w, s_h, P1, 15, GW1 );
// 			}
			////////////////////////////////////////////////
			nMaxPosH = Local_Line[i].pCenter.y;
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[1]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
// 			if ( DISPLAY_SWITCH == TRUE )
// 			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
				FindCircleHOG( In_ImageNpixel, s_w, s_h, P1, 15, GW2 );
// 			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[2]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
// 			if ( DISPLAY_SWITCH == TRUE )
// 			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,0,255));
				FindCircleHOG( In_ImageNpixel, s_w, s_h, P1, 15, GW3 );
// 			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[3]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,255));
			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[4]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));

			}
		}
		else if ( Local_Line[i].dTotalS == LocalHistogram[5]  )
		{
			//找到2nd最大平均強度
			Local_Line[i].BDISPLAY = TRUE;
			if ( DISPLAY_SWITCH == TRUE )
			{
				P1 = CPoint( CarDetected.nCenterLinePosX ,  Local_Line[i].pCenter.y );
				P2 = CPoint( s_w-1 , Local_Line[i].pCenter.y );
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			}
		}
	}
	delete []LocalHistogram;
	/////Sliding Windows///////////////////////////////////////////////////////////////
	int nDistConst; 
	if ( nMaxPosH < s_h/3 )
		nDistConst = s_h/4;
	else
		nDistConst = s_h/6;

	double dAccum;
	nDist = 0;
	vector <Segment> SegPos;
	Segment SEG;
	for ( i=0; i<Local_Line.size(); i++)
	{
		dAccum = 0;


		for (j=i; j<Local_Line.size(); j++)
		{
			nDist = abs( Local_Line[i].pCenter.y - Local_Line[j].pCenter.y );
			if ( i != j && nDist < nDistConst )
			{
				dAccum += Local_Line[j].dTotalS;
				SEG.nStartH = Local_Line[i].pCenter.y;
				SEG.nEndH   = Local_Line[j].pCenter.y;
				SEG.nCenterX= Local_Line[i].pCenter.x;
				SEG.dTotal = dAccum;
				SEG.Remain = FALSE;
				SegPos.push_back( SEG );
			}
		}
	}
	//sliding windows 找出最大值
	double *SEGBUFFER = new double[ SegPos.size() ];
	memset( SEGBUFFER, 0, sizeof(double)*SegPos.size());
	for (i=0; i<SegPos.size(); i++)
		SEGBUFFER[i] = SegPos[i].dTotal;
	I_BubbleSort( SEGBUFFER, SegPos.size() );

	for (i=0; i<SegPos.size(); i++)
	{
		if ( SegPos[i].dTotal == SEGBUFFER[0] )
		{
			SegPos[i].Remain = TRUE;
		}
		else
		{
			SegPos.erase( SegPos.begin()+i);
			i = 0;
		}
	}
	for (i=0; i<SegPos.size(); i++)
	{
// 		if( SegPos[i].Remain == TRUE )
// 		{
			nDist = abs(SegPos[i].nEndH+10 - SegPos[i].nStartH+10);
			nDist *= 0.6;
			P1 = CPoint(SegPos[i].nCenterX-nDist,SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+nDist,SegPos[i].nEndH+10);
			CarDetected.LB1 = P1;
			CarDetected.RU1 = P2;
// 			gpMsgbar->ShowMessage("(車頭區域) h:%d w:%d : ", abs(P1.y-P2.y), abs(P1.x-P2.x));
// 			gpMsgbar->ShowMessage("(%d,%d)_(%d,%d)\n", P1.x, P1.y, P2.x, P2.y);
			for (j=0; j<s_h; j++)
				if ( j > P2.y || j < P1.y) HistogramH[j] = 0;
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			P1 = CPoint(SegPos[i].nCenterX-nDist+1,SegPos[i].nStartH-9);
			P2 = CPoint(SegPos[i].nCenterX+nDist-1,SegPos[i].nEndH+9);
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
			int nTmp = 0;
			if( SegPos[i].nCenterX-nDist*2.6 < 0 ) 
				nTmp = 0 ;
			else
				nTmp = SegPos[i].nCenterX-nDist*2.6;

			P1 = CPoint(nTmp,SegPos[i].nStartH-10);
			P2 = CPoint(SegPos[i].nCenterX+nDist*2,SegPos[i].nEndH+nDist*2.2);
			CarDetected.LB2 = P1;
			CarDetected.RU2 = P2;
			FuncImg.DrawRect(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
			//////////////////////////////////////////////////////////////////////////
			memset( CarDetected.HistogramW, 0, sizeof(int)*s_w );
			memset( CarDetected.HistogramH, 0, sizeof(int)*s_h );
			memset( CarDetected.HistogramW1, 0, sizeof(int)*70);
			memset( CarDetected.HistogramH1, 0 , sizeof(int)*30);
			//////////////////////////////////////////////////////////////////////////
			
			for (k=SegPos[i].nCenterX-35, m=0; k<SegPos[i].nCenterX+35; k++, m++)
			{
				for (j=SegPos[i].nStartH-9; j<SegPos[i].nEndH+9; j++)
				{
					HistogramW[k]   = HistogramW[k] + In_ImageNpixel[j*s_w+k];
					CarDetected.HistogramW[k] = CarDetected.HistogramW[k] + In_ImageNpixel[j*s_w+k];
					CarDetected.HistogramW1[m] = CarDetected.HistogramW1[m] + In_ImageNpixel[j*s_w+k]; 
				}
			}
// 		}
	}
	int test;
	CPoint HeadCentroid;
	nDist = abs(CarDetected.LB1.y - CarDetected.RU1.y );
	HeadCentroid.y = CarDetected.LB1.y + nDist;
	HeadCentroid.x = CarDetected.nCenterLinePosX;
	FindCircleHOG( In_ImageNpixel, s_w, s_h, HeadCentroid, nDistConst, GW1 );
	FuncImg.DrawCross(Display_ImageNsize, s_w, s_h, 3, HeadCentroid, 10, RGB(255,255,255), 2);


	for (i=0; i<SegPos.size(); i++)
	{
		//Test///
		memset( GramR, 0, sizeof(double)*s_h );
		memset( GramL, 0, sizeof(double)*s_h);
		for (j=SegPos[i].nStartH-9; j<SegPos[i].nEndH+9; j++)
		{
			test = abs((SegPos[i].nStartH-9)-(SegPos[i].nEndH+9));
			for (m=SegPos[i].nCenterX, n= SegPos[i].nCenterX; m<SegPos[i].nCenterX+35; m++, n--)
			{
				GramR[j]   = GramR[j] + In_ImageNpixel[j*s_w+m];
				GramL[j]   = GramL[j] + In_ImageNpixel[j*s_w+n]; 
				CarDetected.HistogramH[j] = GramR[j] + GramL[j];
				CarDetected.HistogramH1[j%(test/30)] = GramR[j] + GramL[j];

			}
		}
		if ( DISPLAY_SWITCH == TRUE )
		{
			for (m=SegPos[i].nStartH-9; m<SegPos[i].nEndH+9; m++)
			{
				P1 = CPoint( SegPos[i].nCenterX,m);
				P2 = CPoint( SegPos[i].nCenterX+ GramR[m]/200 , m);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
				P1 = CPoint( SegPos[i].nCenterX,m);
				P2 = CPoint( SegPos[i].nCenterX- GramL[m]/200 , m);
				FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,0,255));
			}
		}

	}
	//顯示Local Histogram變化
	if ( DISPLAY_SWITCH == TRUE )
	{
		for (i=0; i<s_h; i++)
		{
			P1 = CPoint(0,i);
			P2 = CPoint(HistogramH[i]/2,i);
			FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(0,255,0));
		}
		for (i=0; i<s_w; i++)
		{
			P1 = CPoint(i,0);
			P2 = CPoint(i,HistogramW[i]/200);
			FuncImg.DrawLine(Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB(255,255,0));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	delete []GramR;
	delete []GramL;
	delete []GramRT;
	delete []GramLT;
	delete []GramSum;
	delete []GramT;
	delete []HistogramH;
	delete []HistogramW;
}

void CRogerFunction::HOG8x8( HOG_CELL *CELL, HOG_CELL *DELL, HOG_BLOCK *BLOCK, BYTE *In_ImageNpixel, int s_w, int s_h )
{
	CImgProcess FuncImg;
	int i, j, k, x, y;
	int nPixel = s_w * s_h;

	//...計算方向導數
	int *pnGradX = new int[nPixel];
	int *pnGradY = new int[nPixel];
	double *pnGradMAG = new double[nPixel];
	int *pnDegree = new int[nPixel];

	for (i=0; i<nPixel; i++) //歸零
	{
		pnGradX[i]=0;
		pnGradY[i]=0;
	}
	
	//梯度計算
	for(y=0; y<s_h; y++) //x方向
	{
		for(x=0; x<s_w; x++)
		{
			pnGradX[y*s_w+x] = (int) (In_ImageNpixel[y*s_w+min(s_w-1,x+1)]  
				- In_ImageNpixel[y*s_w+max(0,x-1)]);
		}
	}
	for(x=0; x<s_w; x++) //y方向
	{
		for(y=0; y<s_h; y++)
		{
			pnGradY[y*s_w+x] = (int) ( In_ImageNpixel[min(s_h-1,y+1)*s_w + x]  
				- In_ImageNpixel[max(0,y-1)*s_w+ x ]);
		}
	}
	//...End 計算方向導數

	//...計算梯度
	for(y=0; y<s_h; y++)
	{
		for(x=0; x<s_w; x++)
		{			
			//梯度強度
				pnGradMAG[y*s_w + x] = sqrtf( pnGradX[y*s_w + x]*pnGradX[y*s_w + x]+pnGradY[y*s_w + x]*pnGradY[y*s_w + x] );

				if (pnGradX[y*s_w + x] > 0 && pnGradY[y*s_w + x] ==0)
				{
					pnDegree[y*s_w +x]   = 0;
				}				
				else if (pnGradX[y*s_w + x] == 0 && pnGradY[y*s_w + x] > 0)  
				{
					pnDegree[y*s_w +x]  = 90;
				}
				else if (pnGradX[y*s_w + x] < 0 && pnGradY[y*s_w + x] == 0)  
				{
					pnDegree[y*s_w +x]   = 0;
				}
				else if (pnGradX[y*s_w + x] == 0 && pnGradY[y*s_w + x] < 0) 
				{
					pnDegree[y*s_w +x]   = 90;
				}
				else if (pnGradX[y*s_w + x] > 0 && pnGradY[y*s_w + x] > 0) // 第一象限
				{
					pnDegree[y*s_w +x] = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3;
				}
				else if (pnGradX[y*s_w + x] < 0 && pnGradY[y*s_w + x] > 0) // 第二象限
				{
					pnDegree[y*s_w +x] = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3 + 180;
				}
				else if (pnGradX[y*s_w + x] < 0 && pnGradY[y*s_w + x] < 0) // 第三象限
				{
					pnDegree[y*s_w +x] = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3;
				}
				else // 第四象限
				{
					pnDegree[y*s_w +x] = atanf((double)pnGradY[y*s_w + x]/(double)pnGradX[y*s_w + x]) * 57.3 + 180;
				}
			
				DELL[ y / (s_h/8+1) * 8 + x / (s_w/8+1) ].Hist[ pnDegree[y*s_w +x] / 21 ]++; 
				CELL[ y / (s_h/8+1) * 8 + x / (s_w/8+1) ].Hist[ pnDegree[y*s_w +x] / 21 ] += pnGradMAG[ y * s_w + x ];

		}
	}
// 	CPoint P1, P2;
// 	double dTotal = 0;
// 	for (i=0; i<8; i++)
// 	{
// 		for (j=0; j<8; j++)
// 		{
// 			for (k=0; k<9; k++)
// 				dTotal += CELL[i*8+j].Hist[k];
// 		}
// 	}
// 	for (i=0; i<8; i++)
// 	{
// 		for (j=0; j<8; j++)
// 		{
// 			for (k=0; k<9; k++)
// 			{
// 				CELL[i*8+j].Hist[k] /= dTotal ;
// 			}
// 		}
// 	}
	for (i=0; i<7; i++)
	{
		for (j=0; j<7; j++)
		{
			for (k=0; k<9; k++)
				BLOCK[i*7+j].Hist[k] = CELL[i*8+j].Hist[k];
			for (k=9, x=0; k<18; k++, x++)
				BLOCK[i*7+j].Hist[k] = CELL[i*8+j+1].Hist[x];
			for (k=18, x=0; k<27; k++, x++)
				BLOCK[i*7+j].Hist[k] = CELL[(i+1)*8+j].Hist[x];
			for (k=27, x=0; k<36; k++, x++)
				BLOCK[i*7+j].Hist[k] = CELL[(i+1)*8+j+1].Hist[x];
		}
	}
// 	for( k=0; k< 49; k++)
// 	for (i=0; i<36; i++)
// 		gpMsgbar->ShowMessage("[%.3f]", BLOCK[k].Hist[i]);
// 	gpMsgbar->ShowMessage("\n....................\n");
	
	//////////////////////////////////////////////////////////////////////////


//...End計算梯度
	delete []pnDegree;
	delete []pnGradX;
	delete []pnGradY;
	delete []pnGradMAG;

}

void CRogerFunction::HOG_ROI( CPoint LineUP1, CPoint LineBT1, BYTE *In_ImageNpixel1, int s_w1, int s_h1, CPoint LineUP2, CPoint LineBT2, BYTE *In_ImageNpixel2, int s_w2, int s_h2 )
{
// 	fstream file;
// 	file.open("D:\\0LabVideo\\0Text\\HOGTEST.txt", ios::app);
	int i, j, x, y;
	//防止上下誤判
    CPoint cTMP;
	if ( LineBT1.y > LineUP1.y )
	{
		cTMP = LineUP1;
		LineUP1 = LineBT1;
		LineBT1 = cTMP;
	}
	if ( LineBT2.y > LineUP2.y )
	{
		cTMP = LineUP2;
		LineUP2 = LineBT2;
		LineBT2 = cTMP;
	}
	int h1 = LineUP1.y - LineBT1.y;
	int h2 = LineUP2.y - LineBT2.y;

	//////////////////////////////////////////////////////////////////////////
	int nBinDim  = min( h1, h2 ) ;
	int nBinSize = nBinDim * nBinDim ; //64
	int nBinTot  = nBinSize * 9;       //576
	int nBlkDim = nBinDim - 1;
	int nBlkSize = nBlkDim * nBlkDim ; //49
	int nBlkTot  = nBlkSize * 36 ;     //1764
	int nMerge   = nBinTot + nBlkTot ; //2340
	//避免誤判高度不能差距太大
	if ( h1 >= nBinDim && h2 >= nBinDim && nBinDim > 8 )
	{
		int w1 = h1;
		int w2 = h2;
		int hw1 = h1/2 ;
		int hw2 = h2/2 ;
		BYTE *ROI1 = new BYTE[w1*h1];
		BYTE *ROI2 = new BYTE[w2*h2];

		HOG_CELL *CELL1 = new HOG_CELL[nBinSize];
		HOG_CELL *DELL1 = new HOG_CELL[nBinSize];
		HOG_BLOCK *BLOCK1 = new HOG_BLOCK[nBlkSize];
		double *ACELL1 = new double[nBinTot];
		double *ADELL1 = new double[nBinTot];

		double *ABLOCK1 = new double[nBlkTot];
		HOG_CELL *CELL2 = new HOG_CELL[nBinSize];
		HOG_CELL *DELL2 = new HOG_CELL[nBinSize];
		
		HOG_BLOCK *BLOCK2 = new HOG_BLOCK[nBlkSize];
		double *ACELL2 = new double[nBinTot];
		double *ADELL2 = new double[nBinTot];

		double *ABLOCK2 = new double[nBlkTot];
		
		for (i=LineBT1.y, y=0 ; i<LineUP1.y ; i++, y++)
		{
			for (j=LineUP1.x - hw1, x=0; j< LineUP1.x + hw1 ; j++, x++)
			{
				ROI1[y*w1+x] = In_ImageNpixel1[i*s_w1+j];
			}
		}
		for (i=LineBT2.y, y=0 ; i<LineUP2.y ; i++, y++)
		{
			for (j=LineUP2.x - hw2, x=0; j< LineUP2.x + hw2 ; j++, x++)
			{
				ROI2[y*w2+x] = In_ImageNpixel2[i*s_w2+j];
			}
		}

		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				CELL1[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				DELL1[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBlkSize; i++)
		{
			for (j=0; j<36; j++)
			{
				BLOCK1[i].Hist[j] = 0;
			}
		}
		HOG8x8( CELL1, DELL1, BLOCK1, ROI1, w1, h1 );
		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				CELL2[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				DELL2[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBlkSize; i++)
		{
			for (j=0; j<36; j++)
			{
				BLOCK2[i].Hist[j] = 0;
			}
		}
		HOG8x8( CELL2, DELL2, BLOCK2, ROI2, w2, h2 );

		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ACELL1[i*9+j] = CELL1[i].Hist[j];
		for (i=0; i<nBlkSize; i++)
			for (j=0; j<36; j++)
			ABLOCK1[i*36+j] = BLOCK1[i].Hist[j];
		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ACELL2[i*9+j] = CELL2[i].Hist[j];
		for (i=0; i<nBlkSize; i++)
			for (j=0; j<36; j++)
				ABLOCK2[i*36+j] = BLOCK2[i].Hist[j];
		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ADELL1[i*9+j] = DELL1[i].Hist[j];
		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ADELL2[i*9+j] = DELL2[i].Hist[j];
		//////////////////////////////////////////////////////////////////////////
		double *ACELLandBLOCK1 = new double[nMerge];
		double *ACELLandBLOCK2 = new double[nMerge];
		for (i=0; i<nBinTot; i++) ACELLandBLOCK1[i] =ACELL1[i];
		for (i=0, j=nBinTot; i<	nBlkTot; i++, j++) ACELLandBLOCK1[j] = ABLOCK1[i];
		for (i=0; i<nBinTot; i++) ACELLandBLOCK2[i] =ACELL2[i];
		for (i=0, j=nBinTot; i<	nBlkTot; i++, j++) ACELLandBLOCK2[j] = ABLOCK2[i];
		double dS1, dS2, dS3, dS4;
		I_Correlation( ACELL1, ACELL2, nBinTot, dS1 );
		I_Correlation( ABLOCK1, ABLOCK2, nBlkTot, dS2 );
		I_Correlation( ACELLandBLOCK1, ACELLandBLOCK2, nMerge, dS3 );
		I_Correlation( ADELL1, ADELL2, nBinTot, dS4 );
			
		gpMsgbar->ShowMessage("dS1: %.3f, dS2: %.3f, dS3: %.3f, Degree: %.3f\n",dS1, dS2, dS3, dS4 );

		delete []ROI1;
		delete []ROI2;
		delete []CELL1;
		delete []CELL2;
		delete []BLOCK1;
		delete []BLOCK2;
		delete []ACELL1;
		delete []ACELL2;
		delete []ABLOCK1;
		delete []ABLOCK2;
	}
	else
	{
		gpMsgbar->ShowMessage("區域過小,無法比較\n") ;
	
	}
// 	file.close();
}
void CRogerFunction::HOG_ROI( double &dS1, double &dS2, double &dS3, double &dS4, CPoint LineUP1, CPoint LineBT1, BYTE *In_ImageNpixel1, int s_w1, int s_h1, CPoint LineUP2, CPoint LineBT2, BYTE *In_ImageNpixel2, int s_w2, int s_h2 )
{
	int i, j, x, y;
	//防止上下誤判
    CPoint cTMP;
	if ( LineBT1.y > LineUP1.y )
	{
		cTMP = LineUP1;
		LineUP1 = LineBT1;
		LineBT1 = cTMP;
	}
	if ( LineBT2.y > LineUP2.y )
	{
		cTMP = LineUP2;
		LineUP2 = LineBT2;
		LineBT2 = cTMP;
	}
	int h1 = LineUP1.y - LineBT1.y;
	int h2 = LineUP2.y - LineBT2.y;

	//////////////////////////////////////////////////////////////////////////
	int nBinDim  = min( h1, h2 ) ;
	int nBinSize = nBinDim * nBinDim ; //64
	int nBinTot  = nBinSize * 9;       //576
	int nBlkDim = nBinDim - 1;
	int nBlkSize = nBlkDim * nBlkDim ; //49
	int nBlkTot  = nBlkSize * 36 ;     //1764
	int nMerge   = nBinTot + nBlkTot ; //2340
	//避免誤判高度不能差距太大
	if ( h1 >= nBinDim && h2 >= nBinDim && nBinDim > 8 )
	{
		int w1 = h1;
		int w2 = h2;
		int hw1 = h1/2 ;
		int hw2 = h2/2 ;
		BYTE *ROI1 = new BYTE[w1*h1];
		BYTE *ROI2 = new BYTE[w2*h2];

		HOG_CELL *CELL1 = new HOG_CELL[nBinSize];
		HOG_CELL *DELL1 = new HOG_CELL[nBinSize];
		HOG_BLOCK *BLOCK1 = new HOG_BLOCK[nBlkSize];
		double *ACELL1 = new double[nBinTot];
		double *ADELL1 = new double[nBinTot];

		double *ABLOCK1 = new double[nBlkTot];
		HOG_CELL *CELL2 = new HOG_CELL[nBinSize];
		HOG_CELL *DELL2 = new HOG_CELL[nBinSize];
		
		HOG_BLOCK *BLOCK2 = new HOG_BLOCK[nBlkSize];
		double *ACELL2 = new double[nBinTot];
		double *ADELL2 = new double[nBinTot];

		double *ABLOCK2 = new double[nBlkTot];
		
		for (i=LineBT1.y, y=0 ; i<LineUP1.y ; i++, y++)
		{
			for (j=LineUP1.x - hw1, x=0; j< LineUP1.x + hw1 ; j++, x++)
			{
				ROI1[y*w1+x] = In_ImageNpixel1[i*s_w1+j];
			}
		}
		for (i=LineBT2.y, y=0 ; i<LineUP2.y ; i++, y++)
		{
			for (j=LineUP2.x - hw2, x=0; j< LineUP2.x + hw2 ; j++, x++)
			{
				ROI2[y*w2+x] = In_ImageNpixel2[i*s_w2+j];
			}
		}

		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				CELL1[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				DELL1[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBlkSize; i++)
		{
			for (j=0; j<36; j++)
			{
				BLOCK1[i].Hist[j] = 0;
			}
		}
		HOG8x8( CELL1, DELL1, BLOCK1, ROI1, w1, h1 );
		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				CELL2[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				DELL2[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBlkSize; i++)
		{
			for (j=0; j<36; j++)
			{
				BLOCK2[i].Hist[j] = 0;
			}
		}
		HOG8x8( CELL2, DELL2, BLOCK2, ROI2, w2, h2 );

		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ACELL1[i*9+j] = CELL1[i].Hist[j];
		for (i=0; i<nBlkSize; i++)
			for (j=0; j<36; j++)
			ABLOCK1[i*36+j] = BLOCK1[i].Hist[j];
		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ACELL2[i*9+j] = CELL2[i].Hist[j];
		for (i=0; i<nBlkSize; i++)
			for (j=0; j<36; j++)
				ABLOCK2[i*36+j] = BLOCK2[i].Hist[j];
		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ADELL1[i*9+j] = DELL1[i].Hist[j];
		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ADELL2[i*9+j] = DELL2[i].Hist[j];
		//////////////////////////////////////////////////////////////////////////
		double *ACELLandBLOCK1 = new double[nMerge];
		double *ACELLandBLOCK2 = new double[nMerge];
		for (i=0; i<nBinTot; i++) ACELLandBLOCK1[i] =ACELL1[i];
		for (i=0, j=nBinTot; i<	nBlkTot; i++, j++) ACELLandBLOCK1[j] = ABLOCK1[i];
		for (i=0; i<nBinTot; i++) ACELLandBLOCK2[i] =ACELL2[i];
		for (i=0, j=nBinTot; i<	nBlkTot; i++, j++) ACELLandBLOCK2[j] = ABLOCK2[i];
// 		double dS1, dS2, dS3, dS4;
		I_Correlation( ACELL1, ACELL2, nBinTot, dS1 );
		I_Correlation( ABLOCK1, ABLOCK2, nBlkTot, dS2 );
		I_Correlation( ACELLandBLOCK1, ACELLandBLOCK2, nMerge, dS3 );
		I_Correlation( ADELL1, ADELL2, nBinTot, dS4 );
		delete []ROI1;
		delete []ROI2;
		delete []CELL1;
		delete []CELL2;
		delete []BLOCK1;
		delete []BLOCK2;
		delete []ACELL1;
		delete []ACELL2;
		delete []ABLOCK1;
		delete []ABLOCK2;
	}
// 	else
// 	{
// 		gpMsgbar->ShowMessage("區域過小,無法比較\n") ;
// 	
// 	}
}
void CRogerFunction::HOG_ROI_FILEOUT( double *ACELL1,CPoint LineUP1, CPoint LineBT1, BYTE *In_ImageNpixel1, int s_w1, int s_h1 )
{

	int i, j, x, y;
	//防止上下誤判
    CPoint cTMP;
	if ( LineBT1.y > LineUP1.y )
	{
		cTMP = LineUP1;
		LineUP1 = LineBT1;
		LineBT1 = cTMP;
	}
	int h1 = LineUP1.y - LineBT1.y;

	//////////////////////////////////////////////////////////////////////////
	int nBinDim  = 8;
	int nBinSize = nBinDim * nBinDim ; //64
	int nBinTot  = nBinSize * 9;       //576
	int nBlkDim = nBinDim - 1;
	int nBlkSize = nBlkDim * nBlkDim ; //49
	int nBlkTot  = nBlkSize * 36 ;     //1764
	int nMerge   = nBinTot + nBlkTot ; //2340
	//避免誤判高度不能差距太大
	if ( h1 >= nBinDim  )
	{
		int w1 = h1;
		int hw1 = h1/2 ;
		BYTE *ROI1 = new BYTE[w1*h1];

		HOG_CELL *CELL1 = new HOG_CELL[nBinSize];
		HOG_CELL *DELL1 = new HOG_CELL[nBinSize];
		HOG_BLOCK *BLOCK1 = new HOG_BLOCK[nBlkSize];
		double *ADELL1 = new double[nBinTot];
		double *ABLOCK1 = new double[nBlkTot];
		
		for (i=LineBT1.y, y=0 ; i<LineUP1.y ; i++, y++)
		{
			for (j=LineUP1.x - hw1, x=0; j< LineUP1.x + hw1 ; j++, x++)
			{
				ROI1[y*w1+x] = In_ImageNpixel1[i*s_w1+j];
			}
		}

		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				CELL1[i].Hist[j] = 0;
			}
		}
		for (i=0; i<nBinSize; i++)
		{
			for (j=0; j<9; j++)
			{
				DELL1[i].Hist[j] = 0;
			}
		}
		HOG8x8( CELL1, DELL1, BLOCK1, ROI1, w1, h1 );


		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ACELL1[i*9+j] = CELL1[i].Hist[j];
		for (i=0; i<nBlkSize; i++)
			for (j=0; j<36; j++)
			ABLOCK1[i*36+j] = BLOCK1[i].Hist[j];

		for (i=0; i<nBinSize; i++)
			for (j=0; j<9; j++)
				ADELL1[i*9+j] = DELL1[i].Hist[j];

		delete []ROI1;
		delete []CELL1;
		delete []BLOCK1;
		delete []ABLOCK1;
	}
	else
	{
		gpMsgbar->ShowMessage("X ") ;
	
	}
}
double CRogerFunction::HOGSVM_Classifier(  double *dACELL, struct svm_model* model)
{
	struct svm_node *SVMx;
	int max_nr_attr = 576;
	double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;
	int SVMi = 0;
	double predict_label;
	SVMx = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));
	for (SVMi=0; SVMi<576; SVMi++)
	{
		if(SVMi>=max_nr_attr-1)	// need one more for index = -1
		{
			max_nr_attr *= 2;
			SVMx = (struct svm_node *) realloc(SVMx,max_nr_attr*sizeof(struct svm_node));
		}
		SVMx[SVMi].index = SVMi+1;
		SVMx[SVMi].value = dACELL[SVMi];
	}
	SVMx[SVMi].index = -1;
	predict_label = svm_predict(model,SVMx);
	free(SVMx);
	return predict_label;
}
void CRogerFunction::I_FindDist( double *Hist1, double *Hist2, int Hist_Size, double &dist )
{
	int i;
	double sum = 0;
	for (i=0 ; i<Hist_Size ; i++)
	{
		sum += ( Hist1[i] - Hist2[i])*(Hist1[i] - Hist2[i]);
		dist = sqrt(sum);
	}
}
double CRogerFunction::ROI_HORIZONSCAN(CARINFO &CarDetectedA, CARINFO &CarDetectedB, BYTE *In_ROIGradientA, BYTE *In_ROIGradientB, int s_wA, int s_hA, int s_wB, int s_hB)
{
	CImgProcess FuncImg;
	int i, j, k;
	CPoint P1, P2;
	int nBsize2;
	int nIntervalA = abs(CarDetectedA.LB1.y-CarDetectedA.RU1.y);
	int nIntervalB = abs(CarDetectedB.LB1.y-CarDetectedB.RU1.y);
	nBsize2 = min( nIntervalA, nIntervalB );
	if( nBsize2 > 40 ) nBsize2 = 40 ;

	nIntervalA = abs(CarDetectedA.LB1.y-CarDetectedA.RU1.y) / nBsize2 +1;
	nIntervalB = abs(CarDetectedB.LB1.y-CarDetectedB.RU1.y) / nBsize2 +1;

	double *GramA = new double[nBsize2];
	memset( GramA, 0, sizeof(double)*nBsize2 );
	double *GramB = new double[nBsize2];
	memset( GramB, 0, sizeof(double)*nBsize2 );

	for (i= CarDetectedA.LB1.y, k=0 ; i<CarDetectedA.RU1.y ; i++, k++)
	{
		for (j=CarDetectedA.LB1.x; j<CarDetectedA.RU1.x; j++)
		{
			GramA[k/nIntervalA] += In_ROIGradientA[i*s_wA+j]; 
		}
	}
	//////////////////////////////////////////////////////////////////////////
	for (i= CarDetectedB.LB1.y, k=0 ; i<CarDetectedB.RU1.y ; i++, k++)
	{
		for (j=CarDetectedB.LB1.x; j<CarDetectedB.RU1.x; j++)
		{
			GramB[k/nIntervalB] += In_ROIGradientB[i*s_wB+j]; 
		}
	}

	double dScore;
	I_Correlation(GramA, GramB, nBsize2, dScore);
	return dScore;

	delete []GramA;
	delete []GramB;
}
double CRogerFunction::ROI_VERTICALSCAN( CARINFO &CarDetectedA, CARINFO &CarDetectedB, BYTE *In_ROIGradientA, BYTE *In_ROIGradientB, int s_wA, int s_hA, int s_wB, int s_hB )
{
	int i, j, k;
	int nInterval1;
	int nInterval2;
	nInterval1 = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x);
	nInterval2 = abs(CarDetectedB.LB1.x-CarDetectedB.RU1.x);
	int nBsize ;
	nBsize = min( nInterval1, nInterval2 );
	if ( nBsize > 50 ) nBsize = 50; 
	
	double *Hist1 = new double[nBsize];
	double *Hist2 = new double[nBsize];
	double *Hist1Max = new double[nBsize];
	double *Hist2Max = new double[nBsize];
	memset( Hist1, 0, sizeof(double)*nBsize );
	memset( Hist2, 0, sizeof(double)*nBsize );
	memset( Hist1Max, 0, sizeof(double)*nBsize );
	memset( Hist2Max, 0, sizeof(double)*nBsize );
	
	nInterval1 = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x)/nBsize +1;
	nInterval2 = abs(CarDetectedB.LB1.x-CarDetectedB.RU1.x)/nBsize +1;
	
	int nDistA;
	nDistA = (CarDetectedA.RU1.y - CarDetectedA.LB1.y)/2;
	
	for (i=CarDetectedA.LB1.y; i<CarDetectedA.LB1.y+nDistA; i++)
	{
		for (j=CarDetectedA.LB1.x, k=0; j<CarDetectedA.RU1.x; j++, k++)
		{
			Hist1[k/nInterval1]+=In_ROIGradientA[i*s_wA+j];
		}
	}
	memcpy( Hist1Max, Hist1, sizeof(double)*nBsize);
	I_BubbleSort( Hist1Max, nBsize );
	memcpy( Hist1, Hist1Max, sizeof(double)*nBsize);
// 	for (i=0; i<nBsize; i++) Hist1[i] /= Hist1Max[0];
	int nDistB;
	nDistB = (CarDetectedB.RU1.y - CarDetectedB.LB1.y)/2;
	for (i=CarDetectedB.LB1.y; i<CarDetectedB.LB1.y+nDistB; i++)
	{
		for (j=CarDetectedB.LB1.x, k=0; j<CarDetectedB.RU1.x; j++, k++)
		{
			Hist2[k/nInterval2]+=In_ROIGradientB[i*s_wB+j];
		}
	}
	memcpy( Hist2Max, Hist2, sizeof(double)*nBsize);
	I_BubbleSort( Hist2Max, nBsize );
	memcpy( Hist2, Hist2Max, sizeof(double)*nBsize);
// 	for (i=0; i<nBsize; i++) Hist2[i] /= Hist2Max[0];
	
	double dScore;
	I_Correlation( Hist1, Hist2, nBsize, dScore );
	return dScore;

	delete []Hist1;
	delete []Hist2;
	delete []Hist1Max;
	delete []Hist2Max;
}
void CRogerFunction::ROI_VERTICALSCAN( CARINFO &CarDetectedA, BYTE *In_ROIGradientA, int s_wA, int s_hA, double *Hist1 )
{
	int i, j, k;
	int nInterval1;
	nInterval1 = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x);
	int nBsize ;
	nBsize = 50;
	memset( Hist1, 0, sizeof(double)*50 );
	double *Hist1Max = new double[nBsize];
	memset( Hist1Max, 0, sizeof(double)*nBsize );
	
	nInterval1 = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x)/nBsize +1;
	
	for (i=CarDetectedA.LB1.y; i<CarDetectedA.RU1.y; i++)
	{
		for (j=CarDetectedA.LB1.x, k=0; j<CarDetectedA.RU1.x; j++, k++)
		{
			Hist1[k/nInterval1]+=In_ROIGradientA[i*s_wA+j];
		}
	}
	delete []Hist1Max;
}


double CRogerFunction::ROI_FIVEDIRECTION( CARINFO &CarDetectedA, CARINFO &CarDetectedB, int nColumn, int nRow, int *DirectionMap1, int s_wA, int s_hA, int *DirectionMap2, int s_wB, int s_hB )
{
	int i, j, k, m, n;
	int nDimension = 5;
	double dScore;

	double *GramDA1 = new double[nColumn*nRow];
	double *GramDA2 = new double[nColumn*nRow];
	double *GramDA3 = new double[nColumn*nRow];
	double *GramDA4 = new double[nColumn*nRow];
	double *GramDA5 = new double[nColumn*nRow];
	double *GramDB1 = new double[nColumn*nRow];
	double *GramDB2 = new double[nColumn*nRow];
	double *GramDB3 = new double[nColumn*nRow];
	double *GramDB4 = new double[nColumn*nRow];
	double *GramDB5 = new double[nColumn*nRow];
	memset( GramDA1, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA2, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA3, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA4, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA5, 0, sizeof(double)*nColumn*nRow);
	memset( GramDB1, 0, sizeof(double)*nColumn*nRow);
	memset( GramDB2, 0, sizeof(double)*nColumn*nRow);
	memset( GramDB3, 0, sizeof(double)*nColumn*nRow);
	memset( GramDB4, 0, sizeof(double)*nColumn*nRow);
	memset( GramDB5, 0, sizeof(double)*nColumn*nRow);

	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow +1 ;
	int nCIntervalB = abs(CarDetectedB.LB1.x - CarDetectedB.RU1.x)/nColumn +1;
	int nRIntervalB = abs(CarDetectedB.LB1.y - CarDetectedB.RU1.y)/nRow +1;

	int nDist = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x)/2;
	int nIDX;
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX, n=nDist; j<CarDetectedA.RU1.x; j++, n++ )
		{
			nIDX = (m/nRIntervalA)*nColumn+(n/nCIntervalA);
			if( nIDX >= nRow*nColumn ) nIDX = nRow*nColumn-1;

			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
					GramDA2[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 5 )
				GramDA5[nIDX]++;
		}
	}
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX-1, n=nDist-1; j>=CarDetectedA.LB1.x; j--, n-- )
		{
			nIDX = (m/nRIntervalA)*nColumn+(n/nCIntervalA);
			if( nIDX >= nRow*nColumn ) nIDX = nRow*nColumn-1;

			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
					GramDA2[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 5 )
				GramDA5[nIDX]++;
		}
	}
	nDist = abs(CarDetectedB.LB1.x-CarDetectedB.RU1.x)/2;
	for (i=CarDetectedB.LB1.y, m=0; i<CarDetectedB.RU1.y; i++,m++ )
	{
		for (j=CarDetectedB.nCenterLinePosX, n=nDist; j<CarDetectedB.RU1.x; j++, n++ )
		{
			if( DirectionMap2[i*s_wB+j] == 1 )
				GramDB1[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else if( DirectionMap2[i*s_wB+j] == 2 )
				GramDB2[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else if( DirectionMap2[i*s_wB+j] == 3 )
				GramDB3[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else if( DirectionMap2[i*s_wB+j] == 4 )
				GramDB4[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else
				GramDB5[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
		}
	}
	for (i=CarDetectedB.LB1.y, m=0; i<CarDetectedB.RU1.y; i++,m++ )
	{
		for (j=CarDetectedB.nCenterLinePosX-1, n=nDist-1; j>=CarDetectedB.LB1.x; j--, n-- )
		{
			if( DirectionMap2[i*s_wB+j] == 1 )
				GramDB1[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else if( DirectionMap2[i*s_wB+j] == 2 )
				GramDB2[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else if( DirectionMap2[i*s_wB+j] == 3 )
				GramDB3[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else if( DirectionMap2[i*s_wB+j] == 4 )
				GramDB4[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
			else
				GramDB5[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]++;
		}
	}
	double *TotalA = new double[nRow*nColumn*nDimension];
	double *TotalB = new double[nRow*nColumn*nDimension];
	memset( TotalA, 0, sizeof(double)*nRow*nColumn*nDimension);
	memset( TotalB, 0, sizeof(double)*nRow*nColumn*nDimension);
	double dRatio;
	for (i=0, k=0; i<nColumn*nRow; i++, k+=nDimension)
	{
		dRatio = 1;
// 		dRatio = GramDA1[i] + GramDA2[i] +GramDA3[i]+GramDA4[i]+GramDA5[i]+0.00000001;
		TotalA[k  ] = GramDA1[i]/ dRatio;
		TotalA[k+1] = GramDA2[i]/ dRatio;
		TotalA[k+2] = GramDA3[i]/ dRatio;
		TotalA[k+3] = GramDA4[i]/ dRatio;
		TotalA[k+4] = GramDA5[i]/ dRatio;
		dRatio = 1;
// 		dRatio = GramDB1[i] + GramDB2[i] +GramDB3[i]+GramDB4[i]+GramDB5[i]+0.00000001;
		TotalB[k  ] = GramDB1[i]/ dRatio;
		TotalB[k+1] = GramDB2[i]/ dRatio;
		TotalB[k+2] = GramDB3[i]/ dRatio;
		TotalB[k+3] = GramDB4[i]/ dRatio;
		TotalB[k+4] = GramDB5[i]/ dRatio;
	}
// 	///加入權重/////////////////////////////////////////////////////////////////
// 	double dCW;
// 	double dRW;
// 	for (i=0; i<nRow; i++)
// 	{	
// 		for (j=0, k=0; j<nColumn; j++, k+=nDimension)
// 		{
// 			if ( i == 0 )      dRW = 0.2;
// 			else if ( i == 1 ) dRW = 0.2;
// 			else if ( i == 2 ) dRW = 0.3;
// 			else               dRW = 0.3;
// 			
// 			if(j==0 || j==11) dCW = 0.1;
// 			else if(j==1 || j==10) dCW = 0.1;
// 			else if(j==2 || j==9) dCW = 0.2;
// 			else if(j==3 || j==8) dCW = 0.2;
// 			else if(j==4 || j==7) dCW = 0.2;
// 			else dCW = 0.2;
// 			
// 			TotalA[k] = TotalA[k]*dCW*dRW;
// 			TotalA[k+1] = TotalA[k+1]*dCW*dRW;
// 			TotalA[k+2] = TotalA[k+2]*dCW*dRW;
// 			TotalA[k+3] = TotalA[k+3]*dCW*dRW;
// 			TotalA[k+4] = TotalA[k+4]*dCW*dRW;
// 			TotalB[k] = TotalB[k]*dCW*dRW;
// 			TotalB[k+1] = TotalB[k+1]*dCW*dRW;
// 			TotalB[k+2] = TotalB[k+2]*dCW*dRW;
// 			TotalB[k+3] = TotalB[k+3]*dCW*dRW;
// 			TotalB[k+4] = TotalB[k+4]*dCW*dRW;
// 		}
// 	}
	I_Correlation( TotalA, TotalB, nColumn*nRow*nDimension, dScore );

	return dScore;

	delete []GramDA1;
	delete []GramDA2;
	delete []GramDA3;
	delete []GramDA4;
	delete []GramDA5;
	delete []GramDB1;
	delete []GramDB2;
	delete []GramDB3;
	delete []GramDB4;
	delete []GramDB5;
}
double CRogerFunction::ROI_FIVEDIRECTION( CARINFO &CarDetectedA, CARINFO &CarDetectedB, int nColumn, int nRow, BYTE *In_Image1, int s_wA, int s_hA, BYTE *In_Image2, int s_wB, int s_hB )
{
	int i, j, m, n;
	double dScore;

	double *GramDA1 = new double[nColumn*nRow];
	double *GramDB1 = new double[nColumn*nRow];
	memset( GramDA1, 0, sizeof(double)*nColumn*nRow);
	memset( GramDB1, 0, sizeof(double)*nColumn*nRow);
	int nDistA = (CarDetectedA.RU1.y-CarDetectedA.LB1.y)*2/3;
	int nDistB = (CarDetectedB.RU1.y-CarDetectedB.LB1.y)*2/3;

	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int nRIntervalA = nDistA/nRow +1 ;
	int nCIntervalB = abs(CarDetectedB.LB1.x - CarDetectedB.RU1.x)/nColumn +1;
	int nRIntervalB = nDistB/nRow +1;

	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.LB1.y+nDistA; i++,m++ )
	{
		for (j=CarDetectedA.LB1.x, n=0; j<CarDetectedA.RU1.x; j++, n++ )
		{
			GramDA1[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]+=In_Image1[i*s_wA+j];
		}
	}
	for (i=CarDetectedB.LB1.y, m=0; i<CarDetectedB.LB1.y+nDistB; i++,m++ )
	{
		for (j=CarDetectedB.LB1.x, n=0; j<CarDetectedB.RU1.x; j++, n++ )
		{
			GramDB1[(m/nRIntervalB)*nColumn+(n/nCIntervalB)]+=In_Image2[i*s_wB+j];
		}
	}
	I_Correlation( GramDA1, GramDB1, nColumn*nRow, dScore );

	return dScore;

	delete []GramDA1;
	delete []GramDB1;
}
void CRogerFunction::ROI_FIVEDIRECT( CARINFO &CarDetectedA, int nColumn, int nRow, int *DirectionMap1, int s_wA, int s_hA, double *TotalA )
{
	int i, j, k, m, n;
	int nDimension = 5;

	double *GramDA1 = new double[nColumn*nRow];
	double *GramDA2 = new double[nColumn*nRow];
	double *GramDA3 = new double[nColumn*nRow];
	double *GramDA4 = new double[nColumn*nRow];
	double *GramDA5 = new double[nColumn*nRow];
	memset( GramDA1, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA2, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA3, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA4, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA5, 0, sizeof(double)*nColumn*nRow);

	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn + 1;
	int nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow + 1 ;

	int nDist = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x)/2;
	int nIDX;
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX, n=nDist; j<CarDetectedA.RU1.x; j++, n++ )
		{
			nIDX = (m/nRIntervalA)*nColumn+(n/nCIntervalA);
			if( nIDX >= nRow*nColumn ) nIDX = nRow*nColumn-1;
			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
				GramDA2[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 5 )
				GramDA5[nIDX]++;
// 			if ( nIDX >= 48 || nIDX < 0 )
// 				gpMsgbar->ShowMessage("\n");
		}
	}
	gpMsgbar->ShowMessage("\n");
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX-1, n=nDist-1; j>=CarDetectedA.LB1.x; j--, n-- )
		{
			nIDX = (m/nRIntervalA)*nColumn+(n/nCIntervalA);
			if( nIDX >= nRow*nColumn ) nIDX = nRow*nColumn-1;

			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
				GramDA2[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4[nIDX]++;
			else if( DirectionMap1[i*s_wA+j] == 5 )
				GramDA5[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
		}
	}
	double dRatio;
	for (i=0, k=0; i<nColumn*nRow; i++, k+=nDimension)
	{
		dRatio = 0;
		dRatio = GramDA1[i] + GramDA2[i] +GramDA3[i]+GramDA4[i]+GramDA5[i]+0.00000001;
		TotalA[k  ] = GramDA1[i]/ dRatio;
		TotalA[k+1] = GramDA2[i]/ dRatio;
		TotalA[k+2] = GramDA3[i]/ dRatio;
		TotalA[k+3] = GramDA4[i]/ dRatio;
		TotalA[k+4] = GramDA5[i]/ dRatio;
	}

	delete []GramDA1;
	delete []GramDA2;
	delete []GramDA3;
	delete []GramDA4;
	delete []GramDA5;
}
void CRogerFunction::HarrisFindSymmetric( vector<CPoint> &vHarrisA, vector<CPoint> &vHarrisB, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet )
{
	CImgProcess FuncImg;
	int i, j, n;
	CPoint P1, P2;
	int nDistA, nDistB;

	for (i=0; i<vHarrisA.size(); i++)
	{
		n = vHarrisA[i].x - CarDetectedA.nCenterLinePosX;
		P1.x = CarDetectedA.nCenterLinePosX - n;
		P1.y = vHarrisA[i].y;
		if( P1.x < CarDetectedA.RU1.x && P1.x > CarDetectedA.LB1.x )
		{
			vHarrisAsymmet.push_back(vHarrisA[i]);
			vHarrisAsymmet.push_back( P1 );
		}
	}
	for (i=0; i<vHarrisB.size(); i++)
	{
		n = vHarrisB[i].x - CarDetectedB.nCenterLinePosX;
		P1.x = CarDetectedB.nCenterLinePosX - n;
		P1.y = vHarrisB[i].y;
		if( P1.x < CarDetectedB.RU1.x && P1.x > CarDetectedB.LB1.x )
		{
			vHarrisBsymmet.push_back(vHarrisB[i]);
//  			vHarrisBsymmet.push_back( P1 );
		}
	}
	//排除太近點
	double dDratio = 1.1;
	for (i=0; i<vHarrisAsymmet.size(); i++)
	{
		for (j=0; j<vHarrisAsymmet.size(); j++)
		{
			nDistA = FuncImg.FindDistance( vHarrisAsymmet[i].x, vHarrisAsymmet[i].y, vHarrisAsymmet[j].x, vHarrisAsymmet[j].y);
			if ( i != j && nDistA <= 5 )
			{
				vHarrisAsymmet.erase( vHarrisAsymmet.begin()+j );
				i = 0;
			}
		}
	}
	for (i=0; i<vHarrisBsymmet.size(); i++)
	{
		for (j=0; j<vHarrisBsymmet.size(); j++)
		{
			nDistB = FuncImg.FindDistance( vHarrisBsymmet[i].x, vHarrisBsymmet[i].y, vHarrisBsymmet[j].x, vHarrisBsymmet[j].y);
			if ( i != j && nDistB <= 5 )
			{
				vHarrisBsymmet.erase( vHarrisBsymmet.begin()+j );
				i = 0;
			}
		}
	}
}
void CRogerFunction::DrawGrid_Encode( vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet, int nColumn, int nRow, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW, 	vector <MatchCorner> &MCornerBin, BYTE *Display_ImageNsizeA, int s_wA, int s_hA, BYTE *Display_ImageNsizeB, int s_wB, int s_hB, int nDistThreshold  )
{
	CImgProcess FuncImg;
	int i, j;
	CPoint P1, P2;
	int nDist, nDistA, nDistB;
	double dDratio;
	vector <hpoint> vHarrisIdxA;
	vector <hpoint> vHarrisIdxB;
	hpoint Harris_Idx;

	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn;;
	int	nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow;
	int	nCIntervalB = abs(CarDetectedB.LB1.x - CarDetectedB.RU1.x)/nColumn;
 	int	nRIntervalB = abs(CarDetectedB.LB1.y - CarDetectedB.RU1.y)/nRow;
	//畫線 start//////////////////////////////////////////////////////////////
	P1.x = CarDetectedA.nCenterLinePosX;
	P1.y = CarDetectedA.LB1.y;
	P2.x = CarDetectedA.nCenterLinePosX;
	P2.y = CarDetectedA.RU1.y;
	FuncImg.DrawLine( Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,255,0));
	P1.x = CarDetectedB.nCenterLinePosX;
	P1.y = CarDetectedB.LB1.y;
	P2.x = CarDetectedB.nCenterLinePosX;
	P2.y = CarDetectedB.RU1.y;
	FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(255,255,0));
	//////////////////////////////////////////////////////////////////////////
	for (i=1; i<nColumn/2; i++)
	{
		P1.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(0,255,0));
		P1.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(0,255,0));
		//////////////////////////////////////////////////////////////////////////
		P1.x = CarDetectedB.nCenterLinePosX + i * nCIntervalB;
		P1.y = CarDetectedB.LB1.y;
		P2.x = CarDetectedB.nCenterLinePosX + i * nCIntervalB;
		P2.y = CarDetectedB.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(0,255,0));
		P1.x = CarDetectedB.nCenterLinePosX - i * nCIntervalB;
		P1.y = CarDetectedB.LB1.y;
		P2.x = CarDetectedB.nCenterLinePosX - i * nCIntervalB;
		P2.y = CarDetectedB.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(0,255,0));
	}
	for (i=1; i<nRow; i++)
	{
		P1.x = CarDetectedA.LB1.x ;
		P1.y = CarDetectedA.LB1.y + i * nRIntervalA;
		P2.x = CarDetectedA.RU1.x ;
		P2.y = CarDetectedA.LB1.y + i * nRIntervalA;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(0,255,0));
		//////////////////////////////////////////////////////////////////////////
		P1.x = CarDetectedB.LB1.x ;
		P1.y = CarDetectedB.LB1.y + i * nRIntervalB;
		P2.x = CarDetectedB.RU1.x ;
		P2.y = CarDetectedB.LB1.y + i * nRIntervalB;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(0,255,0));
	}
	//畫線 end//////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<vHarrisAsymmet.size(); i++)
	{
		Harris_Idx.Harris = vHarrisAsymmet[i];
		Harris_Idx.ClassIdx = (( vHarrisAsymmet[i].y - CarDetectedA.LB1.y)/nRIntervalA)*nColumn+((vHarrisAsymmet[i].x-CarDetectedA.LB1.x)/(nCIntervalA));
		vHarrisIdxA.push_back( Harris_Idx );
	}
	for (i=0; i<vHarrisBsymmet.size(); i++)
	{
		Harris_Idx.Harris = vHarrisBsymmet[i];
		Harris_Idx.ClassIdx = (( vHarrisBsymmet[i].y - CarDetectedB.LB1.y)/nRIntervalB)*nColumn+((vHarrisBsymmet[i].x-CarDetectedB.LB1.x)/(nCIntervalB));
		vHarrisIdxB.push_back( Harris_Idx );
	}
	//////////////////////////////////////////////////////////////////////////
	vHarrisAsymmet.clear();
	vHarrisBsymmet.clear();
	// Draw the line between harris corner PX
    // distance transform
	nDistA = CarDetectedA.RU1.y - CarDetectedA.LB1.y;
	nDistB = CarDetectedB.RU1.y - CarDetectedB.LB1.y;
	dDratio = (double)nDistB / (double)nDistA ;
	CPoint CenterA;
	CPoint CenterB;
	CPoint CenterNew;
	CenterA = CPoint( CarDetectedA.nCenterLinePosX, CarDetectedA.LB1.y);
	CenterB = CPoint( CarDetectedB.nCenterLinePosX, CarDetectedB.LB1.y);
	for (i=0; i<vHarrisIdxA.size(); i++)
	{
		if( vHarrisIdxA[i].ClassIdx != 2 && vHarrisIdxA[i].ClassIdx != 3 ) 
		{
			vHarrisAsymmet.push_back( vHarrisIdxA[i].Harris );
			Harris_Idx.Harris = vHarrisIdxA[i].Harris;
			Harris_Idx.ClassIdx = vHarrisIdxA[i].ClassIdx;
			vHarrisIdxANEW.push_back( Harris_Idx );
		}
	}
	int nModx, nMody;
	nModx = CenterA.x - CenterB.x;
	nMody = CenterA.y - CenterB.y;
	for (i=0; i<vHarrisIdxB.size(); i++)
	{
		if( vHarrisIdxB[i].ClassIdx != 2  && vHarrisIdxB[i].ClassIdx != 3 ) 
		{
			vHarrisBsymmet.push_back( vHarrisIdxB[i].Harris );
			nDist = FuncImg.FindDistance( vHarrisIdxB[i].Harris.x, vHarrisIdxB[i].Harris.y, CenterA.x, CenterA.y );
			nDist = nDist * dDratio;
			vHarrisIdxB[i].Harris.x = vHarrisIdxB[i].Harris.x + nModx;
			vHarrisIdxB[i].Harris.y = vHarrisIdxB[i].Harris.y + nMody;
			nDistA = (vHarrisIdxB[i].Harris.x - CenterA.x) / dDratio;
			nDistB = (vHarrisIdxB[i].Harris.y - CenterA.y) / dDratio;
			vHarrisIdxB[i].Harris.x = CenterA.x + nDistA;
			vHarrisIdxB[i].Harris.y = CenterA.y + nDistB;
			Harris_Idx.Harris = vHarrisIdxB[i].Harris;
			Harris_Idx.ClassIdx = vHarrisIdxB[i].ClassIdx;
			vHarrisIdxBNEW.push_back( Harris_Idx );
		}
	}
// 	gpMsgbar->ShowMessage("............dDratio: %.3f\n", dDratio);
	//////////////////////////////////////////////////////////////////////////
	//可用匈牙利
	MatchCorner          MPoint;
	double dDist;
	int nTableColumn = vHarrisIdxANEW.size();
	int nTableRow    = vHarrisIdxBNEW.size();
	int nBinSize = nTableColumn * nTableRow ;
	double *MatchTable = new double[nBinSize];
	for(i=0; i<nBinSize; i++) MatchTable[i] = 1000;
	for (i=0; i<nTableRow; i++)
	{
		for (j=0; j<nTableColumn; j++)
		{
			dDist = FuncImg.FindDistance(vHarrisIdxBNEW[i].Harris.x, vHarrisIdxBNEW[i].Harris.y, vHarrisIdxANEW[j].Harris.x, vHarrisIdxANEW[j].Harris.y);
			if( dDist <= nDistThreshold )
			{
				MatchTable[i*nTableColumn+j] = dDist ;
			}
		}
	}
	MatchCorner MatchTmp;
		for (i=0; i<nTableRow; i++)
	{
		double dMin = 320;
		BOOL   bFindFlag = FALSE;
		for (j=0; j<nTableColumn; j++)
		{
			if ( MatchTable[i*nTableColumn+j] < dMin )
			{
				dMin = MatchTable[i*nTableColumn+j];
				MPoint.cpB = vHarrisIdxBNEW[i].Harris;
				MPoint.cpA = vHarrisIdxANEW[j].Harris;
				MPoint.dDist = MatchTable[i*nTableColumn+j];
				bFindFlag = TRUE;
			}
		}
		if ( bFindFlag == TRUE )
		{
			MCornerBin.push_back( MPoint );
		}
	}
	delete []MatchTable;
}
void CRogerFunction::DrawGrid_Encode2( vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet, int nColumn, int nRow, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW,  BYTE *Display_ImageNsizeA, int s_wA, int s_hA, BYTE *Display_ImageNsizeB, int s_wB, int s_hB, int nDistThreshold  )
{
	CImgProcess FuncImg;
	int i;
	CPoint P1, P2;
	int nDistA, nDistB;
	double dDratio;
	vector <hpoint> vHarrisIdxA;
	vector <hpoint> vHarrisIdxB;
	hpoint Harris_Idx;

	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int	nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow;
	int	nCIntervalB = abs(CarDetectedB.LB1.x - CarDetectedB.RU1.x)/nColumn +1;
 	int	nRIntervalB = abs(CarDetectedB.LB1.y - CarDetectedB.RU1.y)/nRow;
	//畫線 start//////////////////////////////////////////////////////////////
	P1.x = CarDetectedA.nCenterLinePosX;
	P1.y = CarDetectedA.LB1.y;
	P2.x = CarDetectedA.nCenterLinePosX;
	P2.y = CarDetectedA.RU1.y;
	FuncImg.DrawLine( Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,255,0));
	P1.x = CarDetectedB.nCenterLinePosX;
	P1.y = CarDetectedB.LB1.y;
	P2.x = CarDetectedB.nCenterLinePosX;
	P2.y = CarDetectedB.RU1.y;
	FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(255,255,0));
	//////////////////////////////////////////////////////////////////////////
	for (i=1; i<nColumn/2; i++)
	{
		P1.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		P1.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
		P1.x = CarDetectedB.nCenterLinePosX + i * nCIntervalB;
		P1.y = CarDetectedB.LB1.y;
		P2.x = CarDetectedB.nCenterLinePosX + i * nCIntervalB;
		P2.y = CarDetectedB.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(255,0,255));
		P1.x = CarDetectedB.nCenterLinePosX - i * nCIntervalB;
		P1.y = CarDetectedB.LB1.y;
		P2.x = CarDetectedB.nCenterLinePosX - i * nCIntervalB;
		P2.y = CarDetectedB.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(255,0,255));
	}
	for (i=1; i<nRow; i++)
	{
		P1.x = CarDetectedA.LB1.x ;
		P1.y = CarDetectedA.LB1.y + i * nRIntervalA;
		P2.x = CarDetectedA.RU1.x ;
		P2.y = CarDetectedA.LB1.y + i * nRIntervalA;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
		P1.x = CarDetectedB.LB1.x ;
		P1.y = CarDetectedB.LB1.y + i * nRIntervalB;
		P2.x = CarDetectedB.RU1.x ;
		P2.y = CarDetectedB.LB1.y + i * nRIntervalB;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(255,0,255));
	}
	//畫線 end//////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<vHarrisAsymmet.size(); i++)
	{
		Harris_Idx.Harris = vHarrisAsymmet[i];
		Harris_Idx.ClassIdx = (( vHarrisAsymmet[i].y - CarDetectedA.LB1.y)/nRIntervalA)*nColumn+((vHarrisAsymmet[i].x-CarDetectedA.LB1.x)/(nCIntervalA));
		vHarrisIdxA.push_back( Harris_Idx );
// 		gpMsgbar->ShowMessage("(%d,%d)[%d]", vHarrisAsymmet[i].x, vHarrisAsymmet[i].y, Harris_Idx.ClassIdx);
	}
// 	gpMsgbar->ShowMessage("\n");
	for (i=0; i<vHarrisBsymmet.size(); i++)
	{
		Harris_Idx.Harris = vHarrisBsymmet[i];
		Harris_Idx.ClassIdx = (( vHarrisBsymmet[i].y - CarDetectedB.LB1.y)/nRIntervalB)*nColumn+((vHarrisBsymmet[i].x-CarDetectedB.LB1.x)/(nCIntervalB));
		vHarrisIdxB.push_back( Harris_Idx );
	}
	//////////////////////////////////////////////////////////////////////////
	vHarrisAsymmet.clear();
	vHarrisBsymmet.clear();
	// Draw the line between harris corner PX
    // distance transform
	nDistA = CarDetectedA.RU1.y - CarDetectedA.LB1.y;
	nDistB = CarDetectedB.RU1.y - CarDetectedB.LB1.y;
	dDratio = (double)nDistB / (double)nDistA ;
	CPoint CenterA;
	CPoint CenterB;
	CPoint CenterNew;
	CenterA = CPoint( CarDetectedA.nCenterLinePosX, CarDetectedA.LB1.y);
	CenterB = CPoint( CarDetectedB.nCenterLinePosX, CarDetectedB.LB1.y);
	for (i=0; i<vHarrisIdxA.size(); i++)
	{
		if( vHarrisIdxA[i].ClassIdx != nColumn/2-1 && vHarrisIdxA[i].ClassIdx != nColumn/2 ) 
		{
			vHarrisAsymmet.push_back( vHarrisIdxA[i].Harris );
			Harris_Idx.Harris = vHarrisIdxA[i].Harris;
			Harris_Idx.ClassIdx = vHarrisIdxA[i].ClassIdx;
			vHarrisIdxANEW.push_back( Harris_Idx );
		}
	}
	for (i=0; i<vHarrisIdxB.size(); i++)
	{
		if( vHarrisIdxB[i].ClassIdx != nColumn/2-1  && vHarrisIdxB[i].ClassIdx != nColumn/2 ) 
		{
			vHarrisBsymmet.push_back( vHarrisIdxB[i].Harris );
			Harris_Idx.Harris = vHarrisIdxB[i].Harris;
			Harris_Idx.ClassIdx = vHarrisIdxB[i].ClassIdx;
			vHarrisIdxBNEW.push_back( Harris_Idx );
		}
	}
}
void CRogerFunction::DrawGrid_Encode3( vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet, int nColumn, int nRow, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW,  BYTE *Display_ImageNsizeA, int s_wA, int s_hA, BYTE *Display_ImageNsizeB, int s_wB, int s_hB, int nDistThreshold  )
{
	CImgProcess FuncImg;
	int i;
	CPoint P1, P2;
	int nDistA, nDistB;
	double dDratio;
	vector <hpoint> vHarrisIdxA;
	vector <hpoint> vHarrisIdxB;
	hpoint Harris_Idx;

	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int	nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow;
	int	nCIntervalB = abs(CarDetectedB.LB1.x - CarDetectedB.RU1.x)/nColumn +1;
 	int	nRIntervalB = abs(CarDetectedB.LB1.y - CarDetectedB.RU1.y)/nRow;
	//畫線 start//////////////////////////////////////////////////////////////
	P1.x = CarDetectedA.nCenterLinePosX;
	P1.y = CarDetectedA.LB1.y;
	P2.x = CarDetectedA.nCenterLinePosX;
	P2.y = CarDetectedA.RU1.y;
	FuncImg.DrawLine( Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,255,0));
	P1.x = CarDetectedB.nCenterLinePosX;
	P1.y = CarDetectedB.LB1.y;
	P2.x = CarDetectedB.nCenterLinePosX;
	P2.y = CarDetectedB.RU1.y;
	FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(255,255,0));
	//////////////////////////////////////////////////////////////////////////
	for (i=1; i<nColumn/2; i++)
	{
		P1.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(0,255,0));
		P1.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(0,255,0));
		//////////////////////////////////////////////////////////////////////////
		P1.x = CarDetectedB.nCenterLinePosX + i * nCIntervalB;
		P1.y = CarDetectedB.LB1.y;
		P2.x = CarDetectedB.nCenterLinePosX + i * nCIntervalB;
		P2.y = CarDetectedB.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(0,255,0));
		P1.x = CarDetectedB.nCenterLinePosX - i * nCIntervalB;
		P1.y = CarDetectedB.LB1.y;
		P2.x = CarDetectedB.nCenterLinePosX - i * nCIntervalB;
		P2.y = CarDetectedB.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(0,255,0));
	}
	for (i=1; i<nRow; i++)
	{
		P1.x = CarDetectedA.LB1.x ;
		P1.y = CarDetectedA.LB1.y + i * nRIntervalA;
		P2.x = CarDetectedA.RU1.x ;
		P2.y = CarDetectedA.LB1.y + i * nRIntervalA;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(0,255,0));
		//////////////////////////////////////////////////////////////////////////
		P1.x = CarDetectedB.LB1.x ;
		P1.y = CarDetectedB.LB1.y + i * nRIntervalB;
		P2.x = CarDetectedB.RU1.x ;
		P2.y = CarDetectedB.LB1.y + i * nRIntervalB;
		FuncImg.DrawLine(Display_ImageNsizeB, s_wB, s_hB, 3, P1, P2, RGB(0,255,0));
	}
	//畫線 end//////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<vHarrisAsymmet.size(); i++)
	{
		Harris_Idx.Harris = vHarrisAsymmet[i];
		Harris_Idx.ClassIdx = (( vHarrisAsymmet[i].y - CarDetectedA.LB1.y)/nRIntervalA)*nColumn+((vHarrisAsymmet[i].x-CarDetectedA.LB1.x)/(nCIntervalA));
		vHarrisIdxA.push_back( Harris_Idx );
	}
	for (i=0; i<vHarrisBsymmet.size(); i++)
	{
		Harris_Idx.Harris = vHarrisBsymmet[i];
		Harris_Idx.ClassIdx = (( vHarrisBsymmet[i].y - CarDetectedB.LB1.y)/nRIntervalB)*nColumn+((vHarrisBsymmet[i].x-CarDetectedB.LB1.x)/(nCIntervalB));
		vHarrisIdxB.push_back( Harris_Idx );
	}
	//////////////////////////////////////////////////////////////////////////
	vHarrisAsymmet.clear();
	vHarrisBsymmet.clear();
	// Draw the line between harris corner PX
    // distance transform
	nDistA = CarDetectedA.RU1.y - CarDetectedA.LB1.y;
	nDistB = CarDetectedB.RU1.y - CarDetectedB.LB1.y;
	dDratio = (double)nDistB / (double)nDistA ;
	CPoint CenterA;
	CPoint CenterB;
	CPoint CenterNew;
	CenterA = CPoint( CarDetectedA.nCenterLinePosX, CarDetectedA.LB1.y);
	CenterB = CPoint( CarDetectedB.nCenterLinePosX, CarDetectedB.LB1.y);
	for (i=0; i<vHarrisIdxA.size(); i++)
	{
// 		if( vHarrisIdxA[i].ClassIdx != nColumn/2 && vHarrisIdxA[i].ClassIdx != nColumn/2+1 ) 
// 		{
			vHarrisAsymmet.push_back( vHarrisIdxA[i].Harris );
			Harris_Idx.Harris = vHarrisIdxA[i].Harris;
			Harris_Idx.ClassIdx = vHarrisIdxA[i].ClassIdx;
			vHarrisIdxANEW.push_back( Harris_Idx );
// 		}
	}
	for (i=0; i<vHarrisIdxB.size(); i++)
	{
// 		if( vHarrisIdxB[i].ClassIdx != nColumn/2  && vHarrisIdxB[i].ClassIdx != nColumn/2+1 ) 
// 		{
			vHarrisBsymmet.push_back( vHarrisIdxB[i].Harris );
			Harris_Idx.Harris = vHarrisIdxB[i].Harris;
			Harris_Idx.ClassIdx = vHarrisIdxB[i].ClassIdx;
			vHarrisIdxBNEW.push_back( Harris_Idx );
// 		}
	}
}
void CRogerFunction::DrawGrid(  CARHEADINFO &CarDetectedA, int nColumn, int nRow, BYTE *Display_ImageNsizeA, int s_wA, int s_hA  )
{
	CImgProcess FuncImg;
	int i;
	CPoint P1, P2;

	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int	nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow;
	//畫線 start//////////////////////////////////////////////////////////////
	P1.x = CarDetectedA.nCenterLinePosX;
	P1.y = CarDetectedA.LB1.y;
	P2.x = CarDetectedA.nCenterLinePosX;
	P2.y = CarDetectedA.RU1.y;
	FuncImg.DrawLine( Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,255,0));
	//////////////////////////////////////////////////////////////////////////
	for (i=1; i<nColumn/2; i++)
	{
		P1.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		P1.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
	}
	for (i=1; i<nRow; i++)
	{
		P1.x = CarDetectedA.LB1.x ;
		P1.y = CarDetectedA.LB1.y + i * nRIntervalA;
		P2.x = CarDetectedA.RU1.x ;
		P2.y = CarDetectedA.LB1.y + i * nRIntervalA;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
	}
	//畫線 end//////////////////////////////////////////////////////////////
}
void CRogerFunction::DrawGrid(  CARINFO &CarDetectedA, int nColumn, int nRow, BYTE *Display_ImageNsizeA, int s_wA, int s_hA  )
{
	CImgProcess FuncImg;
	int i;
	CPoint P1, P2;
	
	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int	nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow;
	//畫線 start//////////////////////////////////////////////////////////////
	P1.x = CarDetectedA.nCenterLinePosX;
	P1.y = CarDetectedA.LB1.y;
	P2.x = CarDetectedA.nCenterLinePosX;
	P2.y = CarDetectedA.RU1.y;
	FuncImg.DrawLine( Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,255,0));
	//////////////////////////////////////////////////////////////////////////
	for (i=1; i<nColumn/2; i++)
	{
		P1.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		P1.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
	}
	for (i=1; i<nRow; i++)
	{
		P1.x = CarDetectedA.LB1.x ;
		P1.y = CarDetectedA.LB1.y + i * nRIntervalA;
		P2.x = CarDetectedA.RU1.x ;
		P2.y = CarDetectedA.LB1.y + i * nRIntervalA;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
	}
	//畫線 end//////////////////////////////////////////////////////////////
}
void CRogerFunction::HOGSIM( CARINFO &CarDetectedA, BYTE *In_ROIGradientA, int *DirectionMap1, int s_wA, int s_hA, double *SimGram )
{
	int i, j, k;
	int nInterval1;
	nInterval1 = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x);
	int nBsize = 50; 
	
	double *Hist1 = new double[nBsize];
	double *Hist1Max = new double[nBsize];
	memset( Hist1, 0, sizeof(double)*nBsize );
	memset( Hist1Max, 0, sizeof(double)*nBsize );
	
	nInterval1 = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x)/nBsize +1;
	
	for (i=CarDetectedA.LB1.y; i<CarDetectedA.RU1.y; i++)
	{
		for (j=CarDetectedA.LB1.x, k=0; j<CarDetectedA.RU1.x; j++, k++)
		{
			Hist1[k/nInterval1]+=In_ROIGradientA[i*s_wA+j];
		}
	}
	for (i=0; i<50; i++)
	{
		SimGram[i] = Hist1[i];
 	}
	//////////////////////////////////////////////////////////////////////////
		
	int m, n;
	int nDimension = 5;
	int nColumn = 12;
	int nRow    = 4;
	double *GramDA1 = new double[nColumn*nRow];
	double *GramDA2 = new double[nColumn*nRow];
	double *GramDA3 = new double[nColumn*nRow];
	double *GramDA4 = new double[nColumn*nRow];
	double *GramDA5 = new double[nColumn*nRow];
	memset( GramDA1, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA2, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA3, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA4, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA5, 0, sizeof(double)*nColumn*nRow);
	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow +1 ;
	int nDist = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x)/2;
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX, n=nDist; j<CarDetectedA.RU1.x; j++, n++ )
		{
			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
					GramDA2[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else
				GramDA5[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
		}
	}
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX-1, n=nDist-1; j>=CarDetectedA.LB1.x; j--, n-- )
		{
			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
				GramDA2[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else
				GramDA5[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
		}
	}
	double *TotalA = new double[nRow*nColumn*nDimension];
	memset( TotalA, 0, sizeof(double)*nRow*nColumn*nDimension);
	double dRatio;
	for (i=0, k=0; i<nColumn*nRow; i++, k+=nDimension)
	{
		dRatio = 0;
		dRatio = GramDA1[i] + GramDA2[i] +GramDA3[i]+GramDA4[i]+GramDA5[i]+0.00000001;
		TotalA[k  ] = GramDA1[i]/ dRatio;
		TotalA[k+1] = GramDA2[i]/ dRatio;
		TotalA[k+2] = GramDA3[i]/ dRatio;
		TotalA[k+3] = GramDA4[i]/ dRatio;
		TotalA[k+4] = GramDA5[i]/ dRatio;
	}
	for (i=0, j=50; i<240; i++, j++)
	{
		SimGram[j] = TotalA[i];
	}
 	//////////////////////////////////////////////////////////////////////////
 	nColumn = 8;
 	nRow    = 3;
	double *GramDA1a = new double[nColumn*nRow];
	double *GramDA2a = new double[nColumn*nRow];
	double *GramDA3a = new double[nColumn*nRow];
	double *GramDA4a = new double[nColumn*nRow];
	double *GramDA5a = new double[nColumn*nRow];
	memset( GramDA1a, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA2a, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA3a, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA4a, 0, sizeof(double)*nColumn*nRow);
	memset( GramDA5a, 0, sizeof(double)*nColumn*nRow);
	nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow +1 ;
	nDist = abs(CarDetectedA.LB1.x-CarDetectedA.RU1.x)/2;
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX, n=nDist; j<CarDetectedA.RU1.x; j++, n++ )
		{
			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
					GramDA2a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else
				GramDA5a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
		}
	}
	for (i=CarDetectedA.LB1.y, m=0; i<CarDetectedA.RU1.y; i++,m++ )
	{
		for (j=CarDetectedA.nCenterLinePosX-1, n=nDist-1; j>=CarDetectedA.LB1.x; j--, n-- )
		{
			if( DirectionMap1[i*s_wA+j] == 1 )
				GramDA1a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 2 )
				GramDA2a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 3 )
				GramDA3a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else if( DirectionMap1[i*s_wA+j] == 4 )
				GramDA4a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
			else
				GramDA5a[(m/nRIntervalA)*nColumn+(n/nCIntervalA)]++;
		}
	}
	double *TotalAa = new double[nRow*nColumn*nDimension];
	memset( TotalAa, 0, sizeof(double)*nRow*nColumn*nDimension);
	for (i=0, k=0; i<nColumn*nRow; i++, k+=nDimension)
	{
		dRatio = 0;
		dRatio = GramDA1a[i] + GramDA2a[i] +GramDA3a[i]+GramDA4a[i]+GramDA5a[i]+0.00000001;
		TotalAa[k  ] = GramDA1a[i]/ dRatio;
		TotalAa[k+1] = GramDA2a[i]/ dRatio;
		TotalAa[k+2] = GramDA3a[i]/ dRatio;
		TotalAa[k+3] = GramDA4a[i]/ dRatio;
		TotalAa[k+4] = GramDA5a[i]/ dRatio;
	}
	for (i=0, j=290; i<120; i++, j++)
	{
		SimGram[j] = TotalAa[i];
	}
	delete []TotalA;
	delete []TotalAa;
	delete []GramDA1;
	delete []GramDA2;
	delete []GramDA3;
	delete []GramDA4;
	delete []GramDA5;
	delete []GramDA1a;
	delete []GramDA2a;
	delete []GramDA3a;
	delete []GramDA4a;
	delete []GramDA5a;
	delete []Hist1;
	delete []Hist1Max;
}

double CRogerFunction::NewDescriptor( vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW, CPoint CenterA, CPoint CenterB, double &dDist )
{
	CImgProcess FuncImg;
	int i;
	int nDistA, nDistB;
	dDist = 0;
	double *GramA = new double[60];
	double *GramB = new double[60];
	memset( GramA, 0, sizeof(double)*60 );
	memset( GramB, 0, sizeof(double)*60 );
	for (i=0; i<vHarrisIdxANEW.size(); i++)
	{
		nDistA = FuncImg.FindAngle( vHarrisIdxANEW[i].Harris, CenterA );
		nDistB = FuncImg.FindDistance( vHarrisIdxANEW[i].Harris.x, vHarrisIdxANEW[i].Harris.y, CenterA.x, CenterA.y );
		if ( nDistA > 180 ) nDistA -= 180;
		GramA[nDistA/3] += nDistB;
	}
	for (i=0; i<vHarrisIdxBNEW.size(); i++)
	{
		nDistA = FuncImg.FindAngle( vHarrisIdxBNEW[i].Harris, CenterB );
		nDistB = FuncImg.FindDistance( vHarrisIdxBNEW[i].Harris.x, vHarrisIdxBNEW[i].Harris.y, CenterB.x, CenterB.y );
		if( nDistA > 180 ) nDistA -= 180;
		GramB[nDistA/3] += nDistB;
	}
	//////////////////////////////////////////////////////////////////////////

	int nCount = 0;
	for (i=0; i<60; i++)
	{
		if (GramA[i] == 0  || GramB[i] == 0 )
		{
			GramA[i] = 0;
			GramB[i] = 0;
		}
		else
		{
			dDist += abs( GramA[i] - GramB[i] );
			nCount++;
		}
	}
	dDist /= nCount;
	double dScore;
	I_Correlation( GramA, GramB, 60, dScore );
	return dScore;
}
double CRogerFunction::FindROIHOG( CPoint LBA, CPoint RUA, CPoint LBB, CPoint RUB , CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector<NewMatchPair> &Result1, BYTE *ImagepxA, int s_wA, BYTE *ImagepxB, int s_wB )
{
	double dScore;
	int i, j, k;
	int nDistA = RUA.x - LBA.x;
	int	nDistB = RUB.x - LBB.x;
	int nBsize = min( nDistA, nDistB );
	if( nBsize > 50 ) nBsize = 50;
	double *HistL1 = new double[nBsize];
	double *HistL2 = new double[nBsize];
	memset( HistL1, 0, sizeof(double)*nBsize );
	memset( HistL2, 0, sizeof(double)*nBsize );
	int nInterval1 = abs( RUA.x - LBA.x) / nBsize + 1;
	int nInterval2 = abs( RUB.x - LBB.x) / nBsize + 1;
	for (i=LBA.y; i<RUA.y; i++)
	{
		for (j=LBA.x, k=0; j<RUA.x; j++, k++)
		{
			HistL1[k/nInterval1]+=ImagepxA[i*s_wA+j];
		}
	}
	for (i=LBB.y; i<RUB.y; i++)
	{
		for (j=LBB.x, k=0; j<RUB.x; j++, k++)
		{
			HistL2[k/nInterval2]+=ImagepxB[i*s_wB+j];
		}
	}
	I_Correlation( HistL1, HistL2, nBsize, dScore );
		
	delete []HistL1;
	delete []HistL2;
	return dScore;
}
void CRogerFunction::LargestCommonSubsequence(  vector <char> &vStr1, vector <char> &vStr2 , vector <int> &vResult )
{
	int nSizeStr1 = vStr1.size()-1;
	int nSizeStr2 = vStr2.size()-1;
	
	int vLength = (nSizeStr1+1)*(nSizeStr2+1);
	int *c = new int[vLength];
	char *b= new char[vLength];
	int i, j;
	for(i=1; i<=nSizeStr1; i++) gpMsgbar->ShowMessage(" %d", (int)vStr1[i]);
	gpMsgbar->ShowMessage("->(%d)", nSizeStr1);
	gpMsgbar->ShowMessage("\n");
	for(i=1; i<=nSizeStr2; i++) gpMsgbar->ShowMessage(" %d", (int)vStr2[i]);
	gpMsgbar->ShowMessage("->(%d)", nSizeStr2);
	gpMsgbar->ShowMessage("\n");
	int nStrLen =0;
	for (i = 1; i <= nSizeStr1; i++)	c[i*(nSizeStr2+1)] = 0;
	for (i = 0; i <= nSizeStr2; i++)	c[i] = 0;
	
	for (i = 1; i <= nSizeStr1; i++)
	{
		for (j = 1; j <= nSizeStr2; j++)
		{
			if (vStr1[i] == vStr2[j])
			{
				c[i*(nSizeStr2+1)+j] = c[(i-1)*(nSizeStr2+1)+(j-1)] + 1;
				b[i*(nSizeStr2+1)+j] = '\\';
			}
			else if (c[(i-1)*(nSizeStr2+1)+j] >= c[i*(nSizeStr2+1)+(j-1)])
			{
				c[i*(nSizeStr2+1)+j] = c[(i-1)*(nSizeStr2+1)+j];
				b[i*(nSizeStr2+1)+j] = '|';
			}
			else
			{
				c[i*(nSizeStr2+1)+j] = c[i*(nSizeStr2+1)+(j-1)];
				b[i*(nSizeStr2+1)+j] = '-';
			}
		}
	}
	gpMsgbar->ShowMessage("Output:  ");
	LCS( nSizeStr1, nSizeStr2, nSizeStr2, b, vStr1, nStrLen, vResult );
	gpMsgbar->ShowMessage(" [%d]\n",nStrLen);
	double dScore;
	dScore = nSizeStr1 + nSizeStr2 - 2*nStrLen;
	gpMsgbar->ShowMessage("m:%d n:%d nStrLen:%d (%.2f)\n", nSizeStr1, nSizeStr2, nStrLen, dScore );
	gpMsgbar->ShowMessage("與Model 配對比: %.3f  自己配對達成率: %.3f\n", (double)nStrLen/nSizeStr1, (double)nStrLen/nSizeStr2);
	for (i=0; i<vResult.size(); i++)
	{
		gpMsgbar->ShowMessage("[%d]", vResult[i]);
	}
	gpMsgbar->ShowMessage("\n");
}
void CRogerFunction::LCS(  int i, int j, int &n, char *b, vector<char> &x, int &nStrLen, vector <int> &vResult )
{
	if (i == 0 || j == 0) return;
	if (b[i*(n+1)+j] == '\\')
	{
		LCS(i-1, j-1, n, b, x, nStrLen, vResult);
		nStrLen++;
		vResult.push_back( (int)x[i] );
		// 		gpMsgbar->ShowMessage(" [%d]", (int)x[i] );
	}
	else if (b[i*(n+1)+j] == '|')
		LCS(i-1, j, n, b, x, nStrLen, vResult);
	else
		LCS(i, j-1, n, b, x, nStrLen, vResult);
}
///function combination


void CRogerFunction::Color2Gradient(BYTE *in_colorimage, int *gradient_image, int s_w, int s_h, int s_Effw, int diff_para )
{

	int i, j, k, x, y;
	int nPixel = s_w * s_h;
	float dSqtOne;
	float dSqtTwo;
	BYTE *in_grayimage = new BYTE[nPixel];
	int *pnGradX = new int[nPixel];
	int *pnGradY = new int[nPixel];
	memset(pnGradX,0,sizeof(int)*nPixel);
	memset(pnGradY,0,sizeof(int)*nPixel);

	// 	BYTE *p_in_image  = in_image;

	int *p_gradient_image = gradient_image;
	int  *p_pnGradX   = pnGradX;
	int  *p_pnGradY   = pnGradY;

	BYTE *p_in_grayimage       = in_grayimage;
	BYTE *p_in_colorimage      = in_colorimage;
	//////////////////////////////////////////////////////////////////////////

	for (i=0;i<s_h;i++, p_in_colorimage += s_Effw, p_in_grayimage += s_w )
		for (j=0,  k=0; j<s_w ; j++, k+=3)
			p_in_grayimage[j]= (int)(p_in_colorimage[k]+p_in_colorimage[k+1]+p_in_colorimage[k+2])/3;

	//...計算方向導數	
	//梯度計算
	int xshift1 = 3;
	k = diff_para;
	p_in_grayimage    = in_grayimage+k*s_w;
	p_pnGradX  = pnGradX+k*s_w;
	for(y=k; y<s_h-k; y++, p_pnGradX+=s_w, p_in_grayimage+=s_w) //x方向
	{
		for(x=k; x<s_w-k; x++)
		{
			p_pnGradX[x] = (int)(p_in_grayimage[x+k] - p_in_grayimage[x-k]);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	p_in_grayimage   = in_grayimage+k*s_w;
	p_pnGradX = pnGradX+k*s_w;
	int yshift  = k * s_w;
	int yshift1 = s_w;

	for(y=k; y<s_h-k; y++, p_pnGradY+=s_w, p_in_grayimage+=s_w) //y方向
	{
		for(x=k; x<s_w-k; x++)
		{
			p_pnGradY[x] = (int)( p_in_grayimage[x+yshift] - p_in_grayimage[x-yshift]);
		}
	}			
	//...End 計算方向導數

	//...計算梯度
	p_in_grayimage = in_grayimage;
	p_pnGradX = pnGradX;
	p_pnGradY = pnGradY;
	for(y=0; y<s_h; y++, p_pnGradX+=s_w, p_pnGradY+=s_w, p_gradient_image+=s_w )
	{
		for(x=0; x<s_w; x++)
		{
			dSqtOne = p_pnGradX[x] * p_pnGradX[x];
			dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
			p_gradient_image[x] = sqrtf(dSqtOne+dSqtTwo );

		}
	}
	//...End計算梯度

	delete []pnGradX;
	delete []pnGradY;
	delete []in_grayimage;
}


/*
void CRogerFunction::Gray2Gradient(BYTE *Grayimage, int *GradX,int *GradY, int *gradient_image, int s_w, int s_h, int diff_para )
{
	
	int k, x, y;
	int nPixel = s_w * s_h;
	float dSqtOne;
	float dSqtTwo;
	
	int *p_gradient_image = gradient_image;
	int  *p_GradX   = GradX;
	int  *p_GradY   = GradY;
	BYTE *p_grayimage       = Grayimage;
	
	//////////////////////////////////////////////////////////////////////////
	
	//...計算方向導數	
	//梯度計算
	int xshift1 = 3;
	k = xshift1;
	p_grayimage    = Grayimage;
	p_GradX  = GradX;
	p_GradY  = GradY;
	int hk,wk;
	hk = s_h -k; wk=s_w-k;
	for(y=0; y < k; y++, p_GradX+=s_w, p_grayimage+=s_w, p_GradY+=s_w)
		for(x=0; x< s_w; x++ ) 
		   {p_GradX[x]=0; p_GradY[x]=0;}

	for(y=k; y< hk; y++, p_GradX+=s_w, p_grayimage+=s_w) //x方向
	{
		for(x=0; x < k; x++) p_GradX[x]=0;
		for(; x< wk; x++)
		{
			p_GradX[x] = (int)(p_grayimage[x+k] - p_grayimage[x-k]);
		}
		for(;x<s_w;x++) p_GradX[x]=0;
	}

	p_GradY = GradY + hk * s_w;
	for(; y < s_h; y++, p_GradX+=s_w, p_GradY+=s_w)
		for(x=0; x< s_w; x++ ) 
		{p_GradX[x]= p_GradY[x]=0;}

	//////////////////////////////////////////////////////////////////////////
	p_grayimage   = Grayimage+k*s_w;
	p_GradY = GradY + k * s_w;

	int yshift  = k * s_w;
	int yshift1 = s_w;
	
	for(y=k; y<hk; y++, p_GradY+=s_w, p_grayimage+=s_w) //y方向
	{   for(x=0; x < k; x++) p_GradY[x]=0;
		for(; x< wk; x++)
		{
			p_GradY[x] = (int)( p_grayimage[x+yshift] - p_grayimage[x-yshift]);
		}

		for(;x<s_w;x++) p_GradY[x]=0;
	}			


	//...End 計算方向導數
	
	//...計算梯度

	p_GradX = GradX;
	p_GradY = GradY;

	for(y=0; y<s_h; y++, p_GradX+=s_w, p_GradY+=s_w, gradient_image+=s_w )
	{
		for(x=0; x<s_w; x++)
		{
			dSqtOne = p_GradX[x] * p_GradX[x];
			dSqtTwo = p_GradY[x] * p_GradY[x];
			gradient_image[x] = sqrtf(dSqtOne+dSqtTwo ) ;
			
		}
	}
} */



void CRogerFunction::Gray2Gradient(BYTE *Grayimage, int *GradX,int *GradY, int *gradient_image, int s_w, int s_h, int diff_para )
{

	int k, x, y;
	int nPixel = s_w * s_h;
	float dSqtOne;
	float dSqtTwo;

	// 	BYTE *p_in_image  = in_image;

	int *p_gradient_image = gradient_image;
	int  *p_pnGradX   = GradX;
	int  *p_pnGradY   = GradY;

	BYTE *p_in_grayimage       = Grayimage;
//	BYTE *p_in_colorimage      = in_colorimage;
	//////////////////////////////////////////////////////////////////////////

		//...計算方向導數	
		//梯度計算
		int xshift1 = 3;
		k = diff_para;
		p_in_grayimage    = Grayimage+k*s_w;
		p_pnGradX  = GradX+k*s_w;
		int sh,sw; 
		sh=s_h-k;
		sw=s_w-k;
		for(y=k; y<sh; y++, p_pnGradX+=s_w, p_in_grayimage+=s_w) //x方向
		{
			for(x=k; x<sw; x++)
			{
				p_pnGradX[x] = (int)(p_in_grayimage[x+k] - p_in_grayimage[x-k]);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		p_in_grayimage   = Grayimage+k*s_w;
		p_pnGradY = GradY+k*s_w;
		int yshift  = k * s_w;
		//	int yshift1 = s_w;

		for(y=k; y<sh; y++, p_pnGradY+=s_w, p_in_grayimage+=s_w) //y方向
		{
			for(x=k; x<sw; x++)
				p_pnGradY[x] = (int)( p_in_grayimage[x+yshift] - p_in_grayimage[x-yshift]);

		}			

		//...計算梯度
		p_in_grayimage = Grayimage;
		p_pnGradX = GradX;
		p_pnGradY = GradY;

		for(y=0; y< k; y++, p_pnGradX+=s_w, p_pnGradY+=s_w, p_gradient_image+=s_w )
			for(x=0; x<s_w; x++)
				p_gradient_image[x] = 0;


		for(; y<sh; y++, p_pnGradX+=s_w, p_pnGradY+=s_w, p_gradient_image+=s_w )
		{
			for(x=0; x< k; x++)
				p_gradient_image[x] = 0;

			for(; x<sw; x++)
			{
				dSqtOne = p_pnGradX[x] * p_pnGradX[x];
				dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
				p_gradient_image[x] = sqrtf(dSqtOne+dSqtTwo );

				if(p_gradient_image[x] < 0 )
					p_gradient_image[x]=p_gradient_image[x];

			}

			for(; x<s_w; x++)
				p_gradient_image[x] = 0;
		}


		for(; y<s_h; y++, p_gradient_image+=s_w )
		{
			for(x=0; x< s_w; x++)
				p_gradient_image[x] = 0;
		}


}


bool printA( const INT_FLOAT_TYPE lhs , const INT_FLOAT_TYPE rhs )
{
	return lhs.nIdx < rhs.nIdx ;
} 

void CRogerFunction::VERTLINE( CARHEADINFO &Car, int *In_ImageNpixel, int s_w, int s_h, int s_bpp/*, BYTE *output, int nWidth, int nHeight*/ ) 
{
	CImgProcess FuncImg;
	int i, j, k;
	CPoint P1, P2;
	float *GramR = new float[s_h];
	memset( GramR, 0, sizeof(float)*s_h);
	float *GramL = new float[s_h];
	memset( GramL, 0, sizeof(float)*s_h);

	float *GramSum = new float[s_h];
	memset( GramSum, 0, sizeof(float)*s_h);	
	float *GramT = new float[s_h];
	memset( GramT, 0, sizeof(float)*s_h);

	int *p_In_ImageNpixel = In_ImageNpixel;

	//////////////////////////////////////////////////////////////////////////

	for (i=0; i<s_h; i++, p_In_ImageNpixel+=s_w )
	{
		for (j=Car.nCenterLinePosX, k= Car.nCenterLinePosX; j<Car.nCenterLinePosX+30; j++, k--)
		{
			GramR[i]   = GramR[i] + p_In_ImageNpixel[j];
			GramL[i]   = GramL[i] + p_In_ImageNpixel[k];
		}
	}


	for (i=0; i<s_h; i++)
	{
		GramR[i] = GramR[i] / 150;
		GramL[i] = GramL[i] / 150;
		GramT[i] = GramL[i] + GramR[i];
	}
	//Smooth
	k = 3;   //60 20
	for (i=k; i<s_h-k; i++)
	{
		for (j=-k; j<k; j++)
		{
			GramSum[i] += GramT[i+j]/k;
		}
	}

	//Find Local Maximum
	float MaxV;
	k = 7;
	for (i=0; i<s_h-k; i++)
	{
		MaxV = 0;
		for (j=0; j<k; j++)
		{
			if ( GramSum[i+j] >= MaxV)
			{
				MaxV = GramSum[i+j];
			}
		}
		for (j=0; j<k; j++)
		{
			if (GramSum[i+j] != MaxV)
				GramSum[i+j] = 0;
		}
	}


	vector<INT_FLOAT_TYPE> YPos_Bin;
	INT_FLOAT_TYPE YPos;

	for (i=0; i<s_h; i++)
	{
		if ( GramSum[i] > 60 )
		{
			YPos.nIdx = i;
			YPos.fValue = GramSum[i];
			YPos_Bin.push_back(YPos);
		}
	}

	sort( YPos_Bin.begin(), YPos_Bin.end(), printA );

	for (i=0; i<YPos_Bin.size(); i++)
	{
		Car.nY_Candidate_bin.push_back( YPos_Bin[i].nIdx );
	}



	delete []GramR;
	delete []GramL;
	delete []GramSum;
	delete []GramT;
}






void CRogerFunction::NewHogDescriptor( CARHEADINFO &Car, BYTE *In_ImageNpixel, int s_w, int s_h, double *GWA, double *GWA1, double *GWA2, double *TotA )
{
	int i, j;
	CPoint Center_center;
	CPoint Center_left;
	CPoint Center_right;
	CPoint Center_New;
	int Radius;
	Radius = (Car.RU1.y-Car.LB1.y)/2;
	Center_center.x = Car.nCenterLinePosX;
	Center_center.y = Car.LB1.y + (Car.RU1.y-Car.LB1.y)/2;
	Center_left.x = Car.nCenterLinePosX - Radius;
	Center_left.y = Center_center.y;
	Center_right.x = Car.nCenterLinePosX + Radius;
	Center_right.y = Center_center.y;
	Center_New.x = Center_center.x;
	Center_New.y = Center_center.y - Radius;
	//////////////////////////////////////////////////////////////////////////
	int x, y;
	int nSize = 360;
	int circle_range = nSize/12;
	int degree_interval = nSize / circle_range ;
	int radius_interval = Radius / 12 +1;
	memset( GWA, 0, sizeof(double)*nSize );
	memset( GWA1, 0, sizeof(double)*nSize );
	memset( GWA2, 0, sizeof(double)*nSize );
	int nIdxCircle;
	int nIdxRadius;
	for (i= 1; i< 359; i++)
	{
		for (j=0; j< Radius; j+=2)
		{
				x = Center_center.x + j * cos(i*0.017453292);
				y = Center_center.y + j * sin(i*0.017453292);
				nIdxRadius  = j/(radius_interval);
				nIdxCircle = (i/degree_interval);
				if ( nIdxCircle >=0 && nIdxCircle < 30 && nIdxRadius >=0 && nIdxRadius < 12 )
				{
					GWA[ nIdxRadius *circle_range + nIdxCircle ] += In_ImageNpixel[y*s_w+x];
// 					gpMsgbar->ShowMessage("(%d)_(r:%d,d:%d)", (j/(radius_interval))*circle_range+(i/degree_interval),(j/(radius_interval)), (i/degree_interval) );
				}
				x = Center_left.x + j * cos(i*0.017453292);
				y = Center_left.y + j * sin(i*0.017453292);
				nIdxRadius  = j/(radius_interval);
				nIdxCircle = (i/degree_interval);

				if ( nIdxCircle >=0 && nIdxCircle < 30 && nIdxRadius >=0 && nIdxRadius < 12 )
					GWA1[ nIdxRadius *circle_range + nIdxCircle ] += In_ImageNpixel[y*s_w+x];

				x = Center_right.x + j * cos(i*0.017453292);
				y = Center_right.y + j * sin(i*0.017453292);
				nIdxRadius  = j/(radius_interval);
				nIdxCircle = (i/degree_interval);

				if (  nIdxCircle >=0 && nIdxCircle < 30 && nIdxRadius >=0 && nIdxRadius < 12 )
					GWA2[ nIdxRadius *circle_range + nIdxCircle ] += In_ImageNpixel[y*s_w+x];
		}
	}
	double *Tmp = new double[nSize];
	memset(Tmp, 0, sizeof(double)*nSize);
	memcpy(Tmp, GWA, sizeof(double)*nSize);
	I_BubbleSort(Tmp, nSize);
	for (i=0; i<nSize; i++) 
		GWA[i] /= (Tmp[0]+0.000000000001);
	memset(Tmp, 0, sizeof(double)*nSize);
	memcpy(Tmp, GWA1, sizeof(double)*nSize);
	I_BubbleSort(Tmp, nSize);
	for (i=0; i<nSize; i++) 
		GWA1[i] /= (Tmp[0]+0.000000000001);
	memset(Tmp, 0, sizeof(double)*nSize);
	memcpy(Tmp, GWA2, sizeof(double)*nSize);
	I_BubbleSort(Tmp, nSize);
	for (i=0; i<nSize; i++) 
		GWA2[i] /= (Tmp[0]+0.000000000001);
	//////////////////////////////////////////////////////////////////////////
	nSize = 180;
	double *GWA3 = new double[nSize];
	memset( GWA3, 0, sizeof(double)*nSize );
	circle_range = nSize/12;
	degree_interval = nSize / circle_range + 1 ;

	Radius = abs(Car.RU1.x-Car.LB1.x)/2;
	radius_interval = Radius / 12 +1;
	
	for (i=1; i<179; i++)
	{
		for (j=0; j< Radius; j+=2)
		{
			x = Center_New.x + j * cos(i*0.017453292);
			y = Center_New.y + j * sin(i*0.017453292);
			if ( x < s_w && x >0 && y > 0 && y < s_h)
			{
				GWA3[(j/(radius_interval))*circle_range+(i/degree_interval)] += In_ImageNpixel[y*s_w+x];
				if( (j/(radius_interval))*circle_range+(i/degree_interval) >= 180 )
					gpMsgbar->ShowMessage("More Than 180 \n");
			}
		}
	}
	double *Tmp1 = new double[nSize];
	memset(Tmp1, 0, sizeof(double)*nSize);
	memcpy(Tmp1, GWA3, sizeof(double)*nSize);
	I_BubbleSort(Tmp1, nSize);
	for (i=0; i<nSize; i++) 
	{
		GWA3[i] /= (Tmp1[0]+0.000000000001);
	}

	for (i=0; i<360; i++) TotA[i] = GWA[i];
	for (i=0, j=360; i<360; i++, j++)  TotA[j] = GWA1[i];
	for (i=0, j=720; i<360; i++, j++)  TotA[j] = GWA2[i];
	for (i=0, j=1080; i<180; i++, j++) TotA[j] = GWA3[i];
	
	delete []Tmp;
	delete []Tmp1;
	delete []GWA3;
}
void CRogerFunction::elipse_modify( double *GW , double in_sigma ) //適合 24 x 15 = 360
{
	int i, j;
	double dis;
	double weit;
	int  nCenter = 8;
	for (j=0; j<24; j++)
	{
		for( i=0; i<15; i++)
		{
			dis = powf( (i-nCenter),2);
			weit = exp(-(0.5)*dis/(in_sigma*in_sigma)) / (sqrt(2 * PI) * in_sigma ); //Gaussian
			GW[j*15+i]  -= weit;
		}
	}
}
void CRogerFunction::SURF_Correlation( SURFINFO &vIT1, SURFINFO &vIT2, double &dist )
{
 	double sum;
	int i;
	sum = 0.f;
	for (i=0 ; i<64 ; i++)
	{
		sum += ( vIT1.dIvec[i] - vIT2.dIvec[i])*(vIT1.dIvec[i] - vIT2.dIvec[i]);
		dist = sqrt(sum);
	}
}

void CRogerFunction::Color2GradientHOG(BYTE *in_image, BYTE *out_image, int *DirectionMap, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	int nPixel = s_w * s_h;
	double dSqtOne;
	double dSqtTwo;
	BYTE *pdTmp = new BYTE[nPixel];
	int *pnGradX = new int[nPixel];
	int *pnGradY = new int[nPixel];
	memset( pdTmp, 0, sizeof(BYTE)*nPixel );
	memset( pnGradX, 0, sizeof(int)*nPixel );
	memset( pnGradY, 0, sizeof(int)*nPixel );
	
	int x,y;
	
	BYTE *p_in_image  = in_image;
	BYTE *p_out_image = out_image;
	int  *p_pnGradX   = pnGradX;
	int  *p_pnGradY   = pnGradY;
	BYTE *p_pdTmp     = pdTmp;
	
	for (i=0;i<s_h;i++, p_in_image += s_Effw, p_pdTmp += s_w )
		for (j=0,  k=0; j<s_w ; j++, k+=3)
			p_pdTmp[j]= (int)(p_in_image[k]+p_in_image[k+1]+p_in_image[k+2])/3;
		
		//...計算方向導數	
		//梯度計算
		p_pdTmp = pdTmp;
		for(y=0; y<s_h; y++, p_pnGradX+=s_w, p_pdTmp+=s_w) //x方向
			for(x=0; x<s_w; x++)
				p_pnGradX[x] = (int) (p_pdTmp[min(s_w-1,x+1)] - p_pdTmp[max(0,x-1)]);
			//////////////////////////////////////////////////////////////////////////
			p_pdTmp = pdTmp, p_pnGradX=pnGradX;
			for(x=0; x<s_w; x++) //y方向
			{
				for(y=0; y<s_h; y++)
				{
					pnGradY[y*s_w+x] = (int) ( pdTmp[min(s_h-1,y+1)*s_w + x] - pdTmp[max(0,y-1)*s_w+ x ]);
				}
			}
			//...End 計算方向導數
			
			//...計算梯度
			for( y=0; y<s_h; y++, p_pnGradX+=s_w, p_pnGradY+=s_w, p_out_image+=s_w, DirectionMap+=s_w )
			{
				for(x=0; x<s_w; x++)
				{
					dSqtOne = p_pnGradX[x] * p_pnGradX[x];
					dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
					out_image[y*s_w + x] = (int)(sqrt(dSqtOne + dSqtTwo)/* + 0.5*/);
					if (p_pnGradX[x] ==0 && p_pnGradY[x] !=0)  // y 方向
					{
						DirectionMap[x]   = 90;
					}
					else if (p_pnGradX[x] !=0 && p_pnGradY[x] ==0) // x 方向
					{
						DirectionMap[x]   = 0;
					}
					else if ( p_pnGradX[x] > 0 && p_pnGradY[x] > 0) // 第1象限
					{
						DirectionMap[x]   = (int)(atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3); 
					}
					else if ( p_pnGradX[x] < 0 && p_pnGradY[x] > 0) // 第2象限
					{
						DirectionMap[x]   = (int)((atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3)+180); 
					}
					else if ( p_pnGradX[x] < 0 && p_pnGradY[x] < 0 )// 第3象限
					{
						DirectionMap[x]   = (int)((atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3)+180); 
					}
					else if ( p_pnGradX[x] > 0 && p_pnGradY[x] < 0 )// 第4象限
					{
						DirectionMap[x]  = (int)((atanf((double)p_pnGradY[x]/(double)p_pnGradX[x]) * 57.3)+360); 
					}
// 					if( DirectionMap[x] == 360  || (x ==0 && y == 44 ))
// 						gpMsgbar->ShowMessage("\n");
				}
			}
			//...End計算梯度
			delete []pdTmp;
			delete []pnGradX;
			delete []pnGradY;
}
void CRogerFunction::TwoBinSURFMatch( vector <SURFINFO> &vIT1, vector <SURFINFO> &vIT2, int &Number )
{
	// 	//自我比對//////////////////////////////////////////////////////////////////////////
	CImgProcess FuncImg;
	
	float dist, d1, d2, sum;
	CPoint mP1, mP2;
	CPoint P1, P2;
	int i, j, k;
	
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<vIT1.size(); i++)
	{
		d1 = d2 = FLT_MAX;
		P1.x = (int)vIT1[i].x;
		P1.y = (int)vIT1[i].y;
		for (j=0; j<vIT2.size(); j++)
		{
			P2.x = (int)vIT2[j].x;
			P2.y = (int)vIT2[j].y;
			if ( vIT1[i].nLaplace == vIT2[j].nLaplace && i != j )
			{
				sum = 0.f;
				for (k=0 ; k<64 ; k++)
				{
					sum += (vIT1[i].dIvec[k] - vIT2[j].dIvec[k] )*(vIT1[i].dIvec[k] -  vIT2[j].dIvec[k] );
					dist = sqrt(sum);
				}
				if(dist<d1) // if this feature matches better than current best
				{
					d2 = d1;
					d1 = dist;
					//////////
				}
				else if(dist<d2) // this feature matches better than second best
				{
					d2 = dist;
				}
			}
			//////////////////////////////////////////////////////////////////////////
		}
		if ( d1/d2 < 0.7 )
		{
			Number++;
		}
	}
}
void CRogerFunction::HOG_correlation( vector <HOG1152> &HOGHISTi, vector <HOG1152> &HOGHISTj, double &dScore )
{
	int i, j;
	dScore = 0;
	double dTotal=0;
	for (i=0; i<HOGHISTi.size(); i++)
	{
		for (j=0; j<HOGHISTj.size(); j++)
		{
			I_Correlation( HOGHISTi[i].HOGTOTAL, HOGHISTj[j].HOGTOTAL, 1152, dScore );
			
			dTotal += dScore;
		}
	}
    dScore = dTotal / (HOGHISTi.size()*HOGHISTj.size()+0.000000000001 );
}
BOOL CRogerFunction::WriteDirectory(CString dd) 
{ 
	HANDLE fFile; 
	WIN32_FIND_DATA fileinfo; 
	CStringArray m_arr; 
	int tt; 
	int x1=0; 
	CString temp=""; 
	fFile = FindFirstFile(dd,&fileinfo);

	//檢驗路徑是否存在
	if (fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) 
	{ 
		FindClose(fFile); 
		return TRUE; 
	} 
	m_arr.RemoveAll();

	//分開字符串，報每個目錄都保存於數組中
	for (x1=0;x1<dd.GetLength();x1++) 
	{ 
		if (dd.GetAt(x1)!='\\') 
		{ 
			temp+= dd.GetAt(x1); 
		} 
		else 
		{ 
			m_arr.Add(temp); 
			temp+="\\"; 
		} 
		if (x1==dd.GetLength()-1) 
		{ 
			m_arr.Add(temp); 
		} 
		FindClose(fFile ); 
	}

	//建立分級的路徑結構http://blog.csdn.net/tsyj810883979/archive/2009/11/17/4824204.aspx 
	for (x1=1;x1<m_arr.GetSize();x1++) 
	{ 
		temp=m_arr .GetAt(x1); 
		tt = CreateDirectory(temp,NULL); 
		if (tt) 
		{ 
			SetFileAttributes(temp,FILE_ATTRIBUTE_NORMAL); 
		} 
	}

	//檢驗建立是否成功

	m_arr.RemoveAll();

	if (fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) 
	{ 
		FindClose(fFile); 
		return TRUE; 
	} 
	else

	{ 
		FindClose(fFile); 
		return FALSE; 
	} 
}

void CRogerFunction::Integral_Image_Create( BYTE *In_GrayImage, int nWidth, int nHeight, double *IntegralImage, int integralw )
{
	int i, j, x, y;
	for ( i=0, y=1; i<nHeight; i++, y++)
	{
		for (j=0, x=1; j<nWidth; j++, x++)
		{
			IntegralImage[ y * integralw + x ] = IntegralImage[ (y) * integralw + (x-1) ] + IntegralImage[ (y-1) * integralw + (x) ] - IntegralImage[ (y-1) * integralw + (x-1) ] + (float)In_GrayImage[i*nWidth+j]/255;
		}
	}
}
void CRogerFunction::Integral_Image_CreateNew( BYTE *In_Image, int nWidth, int nHeight, double *IntegralImage )
{
	int i, j;
	
	
	// first row only
	double rs = 0.0f;
	for( j=0; j<nWidth; j++) 
	{
		rs += ((double) In_Image[j]/255); 
		IntegralImage[j] = rs;
	}
	
	double *p_IntegralImage = IntegralImage + nWidth;
	double *p_IntegralImage1 = IntegralImage;
	BYTE *p_In_Image      = In_Image + nWidth;
	for( i=1; i<nHeight; ++i, p_In_Image+=nWidth, p_IntegralImage+=nWidth ,  p_IntegralImage1+=nWidth) 
	{
		rs = 0.0f;
		for(int j=0; j<nWidth; ++j) 
		{
			rs += ((float) p_In_Image[j]/255); 
			p_IntegralImage[j] = rs + p_IntegralImage1[ j];
		}
	}
}
void CRogerFunction::I_CopyBuffer321(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i,j,k;
	for (i=0;i<s_h;i++, out_image+=s_w, in_image+=s_Effw ) //i控制列(高)
	{
		for (j=0,  k=0; j<s_w ; j++, k+=3)        //k為陣列指標
		{
			// 			if( s_Effw < 3*s_w)
			out_image[j]= (int)(in_image[k]+in_image[k+1]+in_image[k+2])/3;
		}
	}
}
void CRogerFunction::I_CopyBuffer321(BYTE *in_image, BYTE *out_image, int *edgemap, int s_w, int s_h, int s_Effw)
{
	int i,j,k;
	int sum,tmp;
	//int w3 = s_w*2;
	for ( i=0 ; i<s_h ;i++ , out_image+=s_w, edgemap+=(s_w+1), in_image+=s_Effw ) //i控制列(高)
	{
		sum = 0;
		for ( j=0, k=0 ; j<s_w ; j++, k+=3)        //k為陣列指標
		{
			// 			if( s_Effw < 3*s_w)
			out_image[j]= (int)(in_image[k]+in_image[k+1]+in_image[k+2])/3;

			if( j>=4 && j<=s_w-3 ){
				tmp = abs(out_image[j-4]-out_image[j]);
				edgemap[j-2] = tmp;
				if(i>0)
					edgemap[j-2] += edgemap[j-2-(s_w+1)];
				sum += tmp;
			}
		}
		edgemap[s_w] = sum/s_w; //平均值
	}
}
void CRogerFunction::I_CopyBuffer321(BYTE *in_image, float *out_image, int s_w, int s_h, int s_Effw)
{
	int i,j,k;
	for (i=0;i<s_h;i++, out_image+=s_w, in_image+=s_Effw ) //i控制列(高)
	{
		for (j=0,  k=0; j<s_w ; j++, k+=3)        //k為陣列指標
		{
			// 			if( s_Effw < 3*s_w)
			out_image[j]= (float)(in_image[k]+in_image[k+1]+in_image[k+2])/765;
		}
	}
}



void CRogerFunction::DrawCircle(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint CrossPoint, int CrossSize, COLORREF LineColor)
{
	int d, x, y;
	int i;
	int Image_EFFW = Image_W * Image_BPP;
	
	BYTE	Color_R = GetRValue(LineColor),
		Color_G = GetGValue(LineColor),
		Color_B = GetBValue(LineColor);
	
	for (i=CrossSize; i<CrossSize+1; i++)
	{
		for ( d=0; d<360; d++) // 0-360 degree
		{
			x = CrossPoint.x + i * cos(d*0.017453292);
			y = CrossPoint.y + i * sin(d*0.017453292);
			if( x > Image_W )	x = Image_W - 1;
			if( x < 0 )			x = 0;
			if( y > Image_H )	y = Image_H - 1;
			if( y < 0 )			y = 0;
			pImage[y*Image_EFFW+3*x]   = Color_B;
			pImage[y*Image_EFFW+3*x+1] = Color_G;
			pImage[y*Image_EFFW+3*x+2] = Color_R;
		}
	}
}
void CRogerFunction::MYHistogram1D( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax)
{
	CImgProcess FuncImg;
	
	int i, j, k;
	CPoint P1, P2;
	int MaxV, iDX;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i < (int)CenterP.size() ; i++)
	{
		GramW[CenterP[i].x]++;
	}
	
	k= 3;
	for (i=k; i<s_w-k; i++)
	{
		for (j=0; j<k; j++)
		{
			Tmp[i] += GramW[i+j];	
		}
		
	}
	MaxV = 0;
	iDX  = 0;
	for (i=0; i<s_w; i++)
	{
		if ( Tmp[i] > MaxV )
		{
			MaxV = Tmp[i];
			iDX = i;
		}
	}
	for (i=0; i< s_w; i++)
	{
		P1 = CPoint( i,0);
		P2 = CPoint( i, Tmp[i]*2);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 255,0,0));
	}
	P1 = CPoint( iDX, 0);
	P2 = CPoint( iDX, s_h-1 );
	FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 255,255,0));
	
	delete []Tmp;
	delete []GramW;
}




void CRogerFunction::MY_DrawGrid( CARHEADINFO &CarDetectedA, int nColumn, int nRow, BYTE *Display_ImageNsizeA, int s_wA, int s_hA  )
{
	CImgProcess FuncImg;
	int i;
	CPoint P1, P2;
	
	int nCIntervalA = abs(CarDetectedA.LB1.x - CarDetectedA.RU1.x)/nColumn +1;
	int	nRIntervalA = abs(CarDetectedA.LB1.y - CarDetectedA.RU1.y)/nRow;
	//畫線 start//////////////////////////////////////////////////////////////
	P1.x = CarDetectedA.nCenterLinePosX;
	P1.y = CarDetectedA.LB1.y;
	P2.x = CarDetectedA.nCenterLinePosX;
	P2.y = CarDetectedA.RU1.y;
	FuncImg.DrawLine( Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,255,0));
	//////////////////////////////////////////////////////////////////////////
	for (i=1; i<nColumn/2; i++)
	{
		P1.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX + i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		P1.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P1.y = CarDetectedA.LB1.y;
		P2.x = CarDetectedA.nCenterLinePosX - i * nCIntervalA;
		P2.y = CarDetectedA.RU1.y;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
	}
	for (i=1; i<nRow; i++)
	{
		P1.x = CarDetectedA.LB1.x ;
		P1.y = CarDetectedA.LB1.y + i * nRIntervalA;
		P2.x = CarDetectedA.RU1.x ;
		P2.y = CarDetectedA.LB1.y + i * nRIntervalA;
		FuncImg.DrawLine(Display_ImageNsizeA, s_wA, s_hA, 3, P1, P2, RGB(255,0,255));
		//////////////////////////////////////////////////////////////////////////
	}
	//畫線 end//////////////////////////////////////////////////////////////
}

void CRogerFunction::Color2Gradient7( float *in_grayimage, float *gradient_image, int s_w, int s_h, int s_Effw )
{
	
	int k, x, y;
	int nPixel = s_w * s_h;
	float dSqtOne;
	float dSqtTwo;
	float *pnGradX = new float[nPixel];
	float *pnGradY = new float[nPixel];
	memset(pnGradX,0,sizeof(float)*nPixel);
	memset(pnGradY,0,sizeof(float)*nPixel);
	
	float *p_gradient_image = gradient_image;
	float  *p_pnGradX   = pnGradX;
	float  *p_pnGradY   = pnGradY;
	float *p_in_grayimage       = in_grayimage;
	
	// 	int diff_para = 3;
	//////////////////////////////////////////////////////////////////////////
	
	//...計算方向導數	
	//梯度計算
	float fdiff;
	int xshift1 = 3 ;
	k = xshift1;
	p_in_grayimage    = in_grayimage+k*s_w;
	p_pnGradX  = pnGradX+k*s_w;
	for(y=k; y<s_h-k; y++, p_pnGradX+=s_w, p_in_grayimage+=s_w) //x方向
	{
		for(x=k; x<s_w-k; x++)
		{
			fdiff = fabs(p_in_grayimage[x+k] - p_in_grayimage[x-k]) ;
			
			// 			if( fdiff > 0 )
			p_pnGradX[x] = fdiff ;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	p_in_grayimage   = in_grayimage+k*s_w;
	p_pnGradY = pnGradY+k*s_w;
	int yshift  = k * s_w;
	int yshift1 = s_w;
	
	for(y=k; y<s_h-k; y++, p_pnGradY+=s_w, p_in_grayimage+=s_w) //y方向
	{
		for(x=k; x<s_w-k; x++)
		{
			fdiff = fabs(p_in_grayimage[x+yshift] - p_in_grayimage[x-yshift]);
			// 			if( fdiff > 0 )
			p_pnGradY[x] = fdiff ;
		}
	}			
	//...End 計算方向導數
	
	//...計算梯度
	p_in_grayimage = in_grayimage;
	p_pnGradX = pnGradX;
	p_pnGradY = pnGradY;
	for(y=0; y<s_h; y++, p_pnGradX+=s_w, p_pnGradY+=s_w, p_gradient_image+=s_w )
	{
		for(x=0; x<s_w; x++)
		{
			dSqtOne = p_pnGradX[x] * p_pnGradX[x];
			dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
			p_gradient_image[x] = sqrtf(dSqtOne+dSqtTwo ) ;
			
		}
	}
	//...End計算梯度
	
	delete []pnGradX;
	delete []pnGradY;	
}




void CRogerFunction::I_Histogram1D( BYTE *Display_ImageNsize, vector<NewMatchPair>&matchpair_bin , int s_w, int s_h, int s_bpp, vector<int> &XPosMax)
{
	CImgProcess FuncImg;
	
	int i, j, k;
	CPoint P1, P2;
	int MaxV, iDX;
	double *GramW = new double[s_w];
	memset( GramW, 0, sizeof(double)*s_w);
	double *Tmp = new double[s_w];
	memset( Tmp, 0, sizeof(double)*s_w );
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i < (int)matchpair_bin.size() ; i++)
	{
		GramW[matchpair_bin[i].CenterPoint.x]++;
	}
	
	k= 3;
	for (i=k; i<s_w-k; i++)
	{
		for (j=0; j<k; j++)
		{
			Tmp[i] += GramW[i+j];	
		}
		
	}
	MaxV = 0;
	iDX  = 0;
	for (i=0; i<s_w; i++)
	{
		if ( Tmp[i] > MaxV )
		{
			MaxV = Tmp[i];
			iDX = i;
		}
	}
	for (i=0; i< s_w; i++)
	{
		P1 = CPoint( i,0);
		P2 = CPoint( i, Tmp[i]*2);
		FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 255,0,0));
	}
	
	XPosMax.push_back(iDX);
	
	P1 = CPoint( iDX, 0);
	P2 = CPoint( iDX, s_h-1 );
	FuncImg.DrawLine( Display_ImageNsize, s_w, s_h, s_bpp, P1, P2, RGB( 255,255,0));
	
	delete []Tmp;
	delete []GramW;
}
void CRogerFunction::DivideSubRegion_New( BYTE *DisplayImage, float *integralimage, vector<int> &CenterPosX, int nWidth, int nHeight, vector<int> &Posoutput_bin, bool H, int Ext_No, BOOL DISPLAY_SW )
{
	// H = 1 垂直
	// H = 0 水平
	CImgProcess FuncImg;

	int nMarginS = 5 ;

	int nGramW;
	int nCandidateNo;
	if ( H == 1 )
		nCandidateNo = Ext_No;
	else
		nCandidateNo = Ext_No;

	if( H == 1 ) 
		nGramW = nHeight;
	else
		nGramW = nWidth;

	int i, j;
	CPoint P1, P2;

	float fMax = 0;
	CPoint LB, RU;
	float *Gram1 = new float[nGramW];
	float *Gram2 = new float[nGramW];
	memset( Gram1, 0.f, sizeof(float)*nGramW);
	memset( Gram2, 0.f, sizeof(float)*nGramW);
	
	for (i=nMarginS; i<nGramW-nMarginS; i++)
	{
		if( H == 1 )
		{
			LB = CPoint( CenterPosX[0]-50,  i -1 );
			RU = CPoint( CenterPosX[0]+50,  i +1 );
		}
		else
		{
			LB = CPoint( i-1,  nHeight*0.1 );
			RU = CPoint( i+1,  nHeight*0.6 );
			//觀察掃描範圍
// 			FuncImg.DrawRect(DisplayImage,nWidth,nHeight,3,LB,RU,RGB(255,0,255));
		}

		Gram1[i] = Integral_Image_UseNew( integralimage, nWidth, nHeight, LB, RU );

// 		if ( Gram1[i] > fMax )
// 		{
// 			fMax = Gram1[i];
// 			P1 = LB;
// 			P2 = RU;
// 		}
	}
// 	FuncImg.DrawRect( DisplayImage, nWidth, nHeight, 3, P1, P2, RGB( 0, 255, 255) );

	for (i=nMarginS; i<nGramW-nMarginS; i++)
	{
		Gram2[i] = fabs( Gram1[i-nMarginS] - Gram1[i+nMarginS] );
	}

	memset(Gram1,0.f, sizeof(float)*nGramW);

	for (i=nMarginS; i<nGramW-nMarginS; i++)
	{
		if (Gram2[i] != 0 )
		{
			
			for (j=-nMarginS; j<nMarginS; j++)
			{
				Gram1[i] += Gram2[i+j];
			}
		}
		
	}
	if ( H == 0 )
	{
		for (i=nMarginS; i<nGramW-nMarginS; i++)
		{
			if (Gram2[i] != 0 )
			{
				P1.x = i;
				P2.x = P1.x;
				P1.y = 0;
				P2.y = Gram1[i]/4000 ;
				if( DISPLAY_SW == TRUE )
					FuncImg.DrawLine(DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(255,255,0));
					
			}
		}
	}
	else
	{
		for (i=nMarginS; i<nGramW-nMarginS; i++)
		{
			if (Gram2[i] != 0 )
			{
				P1.x = 0;
				P2.x = Gram2[i]/4000;
				P1.y = i;
				P2.y = P1.y;
				if( DISPLAY_SW == TRUE )
					FuncImg.DrawLine(DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(255,255,0));
				
			}
		}
	}

	int nidx;
	int nMargin = 5;
	//區隔為細程度
	for (i=nMargin; i<nGramW-nMargin; i++)
	{
		fMax = 0.f;
		for (j=-nMargin; j<nMargin; j++)
		{
			fMax = max( fMax, Gram1[i+j]  );
		}
		for (j=-nMargin; j<nMargin; j++)
		{
			if ( Gram1[i+j] == fMax )
			{
				nidx = i + j;
			}
			
// 			if( Gram1[i+j] < fMax )
				Gram1[i+j] = 0.f;

		}
		Gram1[nidx] = fMax;
	}
	memcpy( Gram2, Gram1, sizeof(float)*nGramW );
	BubbleSort( Gram2, nGramW );
	//機動決定取出數量

// 	if( H == 0 )
// 	{
		for (i=0; i<nCandidateNo; i++)
		{
			if ( Gram2[i] > 1 )
			{
				nidx = i ;
			}
		}
		nCandidateNo = nidx;
// 	}
	
	for (i=0; i<nGramW; i++)
	{
		if ( Gram1[i] < Gram2[nCandidateNo] )
		{
			Gram1[i] = 0;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	if ( H == 0 )
	{
// 		for (i=-30; i<30; i++)
// 		{
// 			Gram1[i+CenterPosX[0]] = 0;
// 		}
			
	}
	for (i=5; i<nGramW-5; i++)
	{
		if ( Gram1[i] != 0 )
		{
// 			if( DISPLAY_SW == TRUE ) gpMsgbar->ShowMessage("** %d...%f\n", i, Gram1[i]);
			if ( H == 1 )
			{
				P1.x = 0;
				P1.y = i;
				P2.x = nWidth-1;
				P2.y = i;
				if( DISPLAY_SW == TRUE ) FuncImg.DrawLine( DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(0,255,255));
			}
			else
			{
				P1.x = i;
				P1.y = 0;
				P2.x = i;
				P2.y = nHeight-1;
				if( DISPLAY_SW == TRUE ) FuncImg.DrawLine( DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(0,255,255));
			}
			Posoutput_bin.push_back(i);
		}
		
	}
	
	delete []Gram1;
	delete []Gram2;
}
void CRogerFunction::DivideSubRegion_New_UnSymmet( BYTE *DisplayImage, float *integralimage, vector<int> &CenterPosX, int nWidth, int nHeight, vector<int> &Posoutput_bin, bool H, int Ext_No, float fRL_ratio, BOOL DISPLAY_SW )
{
	// H = 1 垂直
	// H = 0 水平
	CImgProcess FuncImg;

	int nMarginS = 5 ;

	int nGramW;
	int nCandidateNo;
	if ( H == 1 )
		nCandidateNo = Ext_No;
	else
		nCandidateNo = Ext_No;

	if( H == 1 ) 
		nGramW = nHeight;
	else
		nGramW = nWidth;

	int i, j;
	CPoint P1, P2;

	float fMax = 0;
	CPoint LB, RU;
	float *Gram1 = new float[nGramW];
	float *Gram2 = new float[nGramW];
	memset( Gram1, 0.f, sizeof(float)*nGramW);
	memset( Gram2, 0.f, sizeof(float)*nGramW);
	
	for (i=nMarginS; i<nGramW-nMarginS; i++)
	{
		if( H == 1 )
		{
			LB = CPoint( CenterPosX[0]-50*(fRL_ratio),  i -1 );
			RU = CPoint( CenterPosX[0]+50            ,  i +1 );
		}
		else
		{
			LB = CPoint( i-1,  nHeight/2 - 50 );
			RU = CPoint( i+1,  nHeight/2 + 25 );
		}

		Gram1[i] = Integral_Image_UseNew( integralimage, nWidth, nHeight, LB, RU );

	}
// 	FuncImg.DrawRect( DisplayImage, nWidth, nHeight, 3, P1, P2, RGB( 0, 255, 255) );
	for (i=nMarginS; i<nGramW-nMarginS; i++)
	{
// 		if( Gram1[i-3] - Gram1[i+3] < 0 )
		Gram2[i] = fabs( Gram1[i-nMarginS] - Gram1[i+nMarginS] );
	}
	memset(Gram1,0.f, sizeof(float)*nGramW);

	for (i=nMarginS; i<nGramW-nMarginS; i++)
	{
		if (Gram2[i] != 0 )
		{
			
			for (j=-nMarginS; j<nMarginS; j++)
			{
				Gram1[i] += Gram2[i+j];
			}
		}
		
	}
	if ( H == 0 )
	{
		for (i=nMarginS; i<nGramW-nMarginS; i++)
		{
			if (Gram2[i] != 0 )
			{
				P1.x = i;
				P2.x = P1.x;
				P1.y = 0;
				P2.y = Gram2[i]/400;
				if( DISPLAY_SW == TRUE )
					FuncImg.DrawLine(DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(255,255,0));
					
			}
		}
	}
	else
	{
		for (i=nMarginS; i<nGramW-nMarginS; i++)
		{
			if (Gram2[i] != 0 )
			{
				P1.x = 0;
				P2.x = Gram2[i]/400;
				P1.y = i;
				P2.y = P1.y;
				if( DISPLAY_SW == TRUE )
					FuncImg.DrawLine(DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(255,255,0));
				
			}
		}
	}

	int nidx;
	int nMargin = 5;
	//區隔為細程度
	for (i=nMargin; i<nGramW-nMargin; i++)
	{
		fMax = 0.f;
		for (j=-nMargin; j<nMargin; j++)
		{
			fMax = max( fMax, Gram1[i+j]  );
		}
		for (j=-nMargin; j<nMargin; j++)
		{
			if ( Gram1[i+j] == fMax )
			{
				nidx = i + j;
			}
			
// 			if( Gram1[i+j] < fMax )
				Gram1[i+j] = 0.f;

		}
		Gram1[nidx] = fMax;
	}
	memcpy( Gram2, Gram1, sizeof(float)*nGramW );
	BubbleSort( Gram2, nGramW );
	//機動決定取出數量

// 	if( H == 0 )
// 	{
		for (i=0; i<nCandidateNo; i++)
		{
			if ( Gram2[i] > 1 )
			{
				nidx = i ;
			}
		}
		nCandidateNo = nidx;
// 	}
	
	for (i=0; i<nGramW; i++)
	{
		if ( Gram1[i] < Gram2[nCandidateNo] )
		{
			Gram1[i] = 0;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	if ( H == 0 )
	{
		for (i=-30; i<30; i++)
		{
			Gram1[i+CenterPosX[0]] = 0;
		}
			
	}
	for (i=5; i<nGramW-5; i++)
	{
		if ( Gram1[i] != 0 )
		{
// 			if( DISPLAY_SW == TRUE ) gpMsgbar->ShowMessage("** %d...%f\n", i, Gram1[i]);
			if ( H == 1 )
			{
				P1.x = 0;
				P1.y = i;
				P2.x = nWidth-1;
				P2.y = i;
				if( DISPLAY_SW == TRUE ) FuncImg.DrawLine( DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(0,255,255));
			}
			else
			{
				P1.x = i;
				P1.y = 0;
				P2.x = i;
				P2.y = nHeight-1;
				if( DISPLAY_SW == TRUE ) FuncImg.DrawLine( DisplayImage, nWidth, nHeight, 3, P1, P2, RGB(0,255,255));
			}
			Posoutput_bin.push_back(i);
		}
		
	}
	
	delete []Gram1;
	delete []Gram2;
}
void CRogerFunction::headcand_New( 	vector<IntBool> &X_R_bin, vector<IntBool> &X_L_bin, vector <int> &CenterPosX, vector<int> &Y_bin, vector<CenterDistCandidate>&CenterDistCandidateNew_bin, BYTE *DisplayImage, float *integralimage, int s_w, int s_h, BOOL DISPLAY_SW )
{
	CImgProcess FuncImg;

	int i, j;
	CPoint P1, P2;

	Ybin_Cand Ycand;
	vector<Ybin_Cand>YbinCand_bin;
	CenterDistCandidate centerdistcandidate;

	float fMin, fMax;

	for (i=0; i<Y_bin.size(); i++)
	{
		for (j=0; j<Y_bin.size(); j++)
		{
			if ( i != j && Y_bin[i] < Y_bin[j] )
			{
				Ycand.ndist = Y_bin[j] - Y_bin[i];
				Ycand.YU    = Y_bin[j];
				Ycand.YB    = Y_bin[i];
				YbinCand_bin.push_back(Ycand);
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	if ( X_L_bin.size() <= X_R_bin.size() )
	{
		for (i=0; i<X_R_bin.size(); i++)
		{
			centerdistcandidate.ndist    = X_R_bin[i].ndist * 2;
			centerdistcandidate.nCenterX = X_R_bin[i].nCenterX;
			centerdistcandidate.nRightX   = X_R_bin[i].nX;
			centerdistcandidate.nLeftX  = X_R_bin[i].nCenterX - X_R_bin[i].ndist;
			
			centerdistcandidate.LB.x     = centerdistcandidate.nLeftX;
			centerdistcandidate.RU.x     = centerdistcandidate.nRightX;
			centerdistcandidate.fscore   = 0;
			fMin = (float)centerdistcandidate.ndist * 0.45;
			fMax = (float)centerdistcandidate.ndist * 0.55;
			
			if( centerdistcandidate.ndist > 200  )
			{
				for (j=0; j<YbinCand_bin.size(); j++)
				{
					if ( YbinCand_bin[j].ndist > fMin && YbinCand_bin[j].ndist < fMax )
					{
						centerdistcandidate.LB.y = YbinCand_bin[j].YB;
						centerdistcandidate.RU.y = YbinCand_bin[j].YU;
						CenterDistCandidateNew_bin.push_back(centerdistcandidate);
					}
					
				}
			}
		}
	}
	else
	{
		for (i=0; i<X_L_bin.size(); i++)
		{
			centerdistcandidate.ndist    = X_L_bin[i].ndist * 2;
			centerdistcandidate.nCenterX = X_L_bin[i].nCenterX;
			centerdistcandidate.nLeftX   = X_L_bin[i].nX;
			centerdistcandidate.nRightX  = X_L_bin[i].nCenterX + X_L_bin[i].ndist;
			
			centerdistcandidate.LB.x     = centerdistcandidate.nLeftX;
			centerdistcandidate.RU.x     = centerdistcandidate.nRightX;
			centerdistcandidate.fscore   = 0;
			fMin = (float)centerdistcandidate.ndist * 0.45;
			fMax = (float)centerdistcandidate.ndist * 0.55;
			
			if( centerdistcandidate.ndist > 200  )
			{
				for (j=0; j<YbinCand_bin.size(); j++)
				{
					if ( YbinCand_bin[j].ndist > fMin && YbinCand_bin[j].ndist < fMax )
					{
						centerdistcandidate.LB.y = YbinCand_bin[j].YB;
						centerdistcandidate.RU.y = YbinCand_bin[j].YU;
						CenterDistCandidateNew_bin.push_back(centerdistcandidate);
					}
					
				}
			}
		}
	}

	gpMsgbar->ShowMessage("size: %d\n", CenterDistCandidateNew_bin.size());

	if ( DISPLAY_SW == TRUE )
	{
		for (i=0; i<CenterDistCandidateNew_bin.size(); i++)
		{
			P1 = CenterDistCandidateNew_bin[i].LB;
			P2 = CenterDistCandidateNew_bin[i].RU;
			FuncImg.DrawRect(DisplayImage, s_w, s_h, 3, P1, P2, RGB(255,255,0));
		}
	}


}

void CRogerFunction::I_SavePatch( BYTE *in_image, int s_w, int s_h, CPoint LB, CPoint RU, char *FileName, BOOL CROSS_DISPLAY, BOOL RESIZE )
{
	CImgProcess FuncImg;
	
	CreateDirectory("D:\\0Graph1\\",NULL);
	int n = 0;
	int i, j, k, x, y;
	CPoint P1, P2;
	////產生儲存小圖////////////////////////////////////////////////////////
	int w = abs(RU.x-LB.x); 
	int h = abs(RU.y-LB.y);
	BYTE *pixels = new BYTE[w*h*3];
	CxImage *newImage = new CxImage(0);	
	if( CROSS_DISPLAY == TRUE )
	{
		for (i=0, y=LB.y; i<h; i++, y++)
		{
			for (j=0, k=0, x=LB.x; j<w; j++, x++, k+=3)
			{
				if ( i == h/2 || j == w/2 )
				{
					pixels[(i)*(w*3)+k]   = 0;
					pixels[(i)*(w*3)+k+1] = 0;
					pixels[(i)*(w*3)+k+2] = 255;
				}
				else
				{
					pixels[(i)*(w*3)+k]   = in_image[(y)*s_w*3+(x)*3];
					pixels[(i)*(w*3)+k+1] = in_image[(y)*s_w*3+(x)*3+1];
					pixels[(i)*(w*3)+k+2] = in_image[(y)*s_w*3+(x)*3+2];
				}
				
			}
		}
	}
	else
	{
		for (i=0, y=LB.y; i<h; i++, y++)
		{
			for (j=0, k=0, x=LB.x; j<w; j++, x++, k+=3)
			{
				pixels[(i)*(w*3)+k]   = in_image[(y)*s_w*3+(x)*3];
				pixels[(i)*(w*3)+k+1] = in_image[(y)*s_w*3+(x)*3+1];
				pixels[(i)*(w*3)+k+2] = in_image[(y)*s_w*3+(x)*3+2];
			}
		}
	}
	
	
	newImage->CreateFromArray(pixels, w, h, 24, (w*3), false);
	newImage->SetJpegQuality(100);
	newImage->SetJpegScale(100);
	char path[50]  = "D:\\0Graph1\\";
	CreateDirectory( path, NULL);
	char ex[5] = ".jpg";
	strcat( path, FileName);
	strcat(path, ex);
	
	if( RESIZE == TRUE )
		newImage->Resample2(w*3,h*3, CxImage::IM_BICUBIC2, CxImage::OM_REPEAT,NULL ,FALSE);
	
	newImage->Save(path, CXIMAGE_FORMAT_JPG);
	newImage->Destroy();
	delete newImage;
	delete [] pixels;
}
inline float CRogerFunction::Integral_Image_UseNew( float *IntegralImage, int s_w, int s_h, CPoint LB, CPoint RU )
{
	// 	float fResult;
	// 	fResult = IntegralImage[ (RU.y)     * s_w + (RU.x)    ] - 
	// 		      IntegralImage[ (LB.y-1)   * s_w + (RU.x)    ] - 
	// 			  IntegralImage[ (RU.y)     * s_w + (LB.x-1)  ] + 
	// 			  IntegralImage[ (LB.y-1)   * s_w + (LB.x-1)  ];
	// 		
	// 	return fResult;
	
	
	// The subtraction by one for row/col is because row/col is inclusive.
	int r1 = RU.y;
	int c1 = RU.x;
	int r2 = LB.y;
	int c2 = LB.x;
	//為了加速把判斷移除，因為本案例不會出現 x= 0, y =0 or x = sw or y = sh
	float A(0.0f), B(0.0f), C(0.0f), D(0.0f);
	
	/*if (r1 >= 0   && c1 >= 0  )*/ A = IntegralImage[ (r1)     * s_w + (c1)    ];
	/*if (r2-1 >= 0 && c1 >= 0  )*/ B = IntegralImage[ (r2-1)   * s_w + (c1)    ];
	/*if (r1 >= 0   && c2-1 >= 0)*/ C = IntegralImage[ (r1)     * s_w + (c2-1)  ];
	/*if (r2-1 >= 0 && c2-1 >= 0)*/ D = IntegralImage[ (r2-1)   * s_w + (c2-1)  ];
	
	return max(0.f, A - B - C + D);
	
}
void CRogerFunction::BubbleSort(float *DataList, int Num)
{
	int i, j;
	float tmp;
	Num = Num - 1;
	for (i=0; i<Num; i++)
	{
		for (j=0; j<Num-i; j++)
		{
			if (DataList[j] < DataList[j+1]) // > 遞減, < 遞增
			{
				tmp = DataList[j];
				DataList[j] = DataList[j+1];
				DataList[j+1] = tmp;
			}
		}
	}
}
void CRogerFunction::HSV_DISPLAY_NEW4( PX_COLOR_INFO *ROIPXINFO, int ROIW, int ROIH, int ROIWH, BYTE *HSV_Domain_Display, int s_w, int s_h, int s_effw, int &nH_Group_Number, int &nH, int &nS, int &nV, int nInterval, int &group, double *ColorHistogram, BOOL DISPLAY  )
{
	//Histogram for Local Maximum.............................................
	CImgProcess FuncImg;

	int i, j, k;
	CPoint P1, P2;

	nH_Group_Number=0;

	double zero = 0;
	double h_number = 0;
	int nHIdxR=0;
	int nHIdxL=0;
	double dHAve=0;
	double dHTmp1=0;
	double dHTmp2=0;
	double dHMax=0;
	// for ring structure
	int Ring   = 400;
	int Ring18 = Ring/20;
	int Ring18I = Ring18+1;
	double *Histogram_H = new double[Ring];
	double *Histogram_H_Tmp = new double[Ring];
	memset( Histogram_H, 0, sizeof(double)*Ring );
	memset( Histogram_H_Tmp, 0, sizeof(double)*Ring );

	double *Histogram_H18 = new double[Ring18];
	double *Histogram_H18_Tmp = new double[Ring18I];
	memset( Histogram_H18, 0, sizeof(double)*Ring18 );
	memset( Histogram_H18_Tmp, 0, sizeof(double)*Ring18I );
	
	for (i=0; i<ROIH; i++)
	{
		for (j=0; j<ROIW; j++)
		{
			Histogram_H[ ROIPXINFO[ i*ROIW+j ].H ]++;
		}
	}
	/////////////////////////
	zero = Histogram_H[0];
	h_number = ROIWH - zero;
	/////////////////////////
	Histogram_H[0] =0;
	////////////////////////////////////////////////////////////
	for (i=0, j=360; i<Ring-360; i++, j++)
	{
		Histogram_H[j] = Histogram_H[i];
	}
	//////////////////////////////////////////////////////////////////////////
	for (i=0; i<Ring; i++)
	{
		Histogram_H18[ i/Ring18 ] += Histogram_H[i];
	}

	//////////////////////////////////////////////////////////////////////////
	Histogram_H_Tmp[0] = Histogram_H[0];
	//integral image....
	for (i=1; i<Ring; i++)
	{
		Histogram_H_Tmp[i] = Histogram_H_Tmp[i-1] + Histogram_H[i]; 
	}
	////////////////////////////////////////
	Histogram_H18_Tmp[1] = Histogram_H18[0];
	for (i=1; i<Ring18; i++)
	{
		Histogram_H18_Tmp[i+1] = Histogram_H18_Tmp[i] + Histogram_H18[i]; 
	}
	//觀察Integral 是否正確
	for(i=0; i<Ring18; i++) gpMsgbar->ShowMessage("_(%d)[%4.f]", i, Histogram_H18[i]);
	gpMsgbar->ShowMessage("\n");
// 	for(i=0; i<Ring18I; i++) gpMsgbar->ShowMessage("[%4.f]", Histogram_H18_Tmp[i]);
// 	gpMsgbar->ShowMessage("\n");
	//////////////////////////////////////////////////////////////////////////

	//H 出現至少要60%以上比較具代表性
	h_number = h_number * 0.6;
	///////////////////////////////////
	//兩個一組
	double Max2 = 0;
	int group1;
	group = 0;
	for (group=2; group<Ring18; group++)
	{
		for (i=group; i<Ring18I; i++ )
		{
			dHTmp1 = ( Histogram_H18_Tmp[ i ] - Histogram_H18_Tmp[i-group] ) ;
			if ( dHTmp1 > Max2 )
			{
				Max2 = dHTmp1;
			}
		}

		for (i=group; i<Ring18I; i++ )
		{
			dHTmp1 = ( Histogram_H18_Tmp[ i ] - Histogram_H18_Tmp[i-group] ) ;
			if ( dHTmp1 >= h_number && dHTmp1 >= Max2 )
			{
				nHIdxL = i-group;
				nHIdxR = i-1;
				if ( nHIdxR > 17 )
				{
					nHIdxR -= 18;
				}
				if ( nHIdxL > 17 )
				{
					nHIdxL -= 18;
				}
				group1 = group;
				gpMsgbar->ShowMessage("group: %d\n", group);
				gpMsgbar->ShowMessage("L: %d _ R: %d\n", nHIdxL, nHIdxR);
				i=Ring18I;
				group = Ring18;


			}
		}
	}
	group = group1;

	///////////////////////////////////////////////////////////////////////////////

	int region_mount=0;
	nH = 0;
	int nMax=0;
	int index;

	if ( nHIdxR > nHIdxL )
	{
		for (i=0; i<ROIWH; i++)
		{
			if ( ROIPXINFO[i].H/20 >= nHIdxL && ROIPXINFO[i].H/20 <= nHIdxR && ROIPXINFO[i].H != 0 )
			{
				ROIPXINFO[i].flag_H = TRUE;
				nH_Group_Number++;
				ColorHistogram[ROIPXINFO[i].H]++;
			}
		}
		//////////////////////////////
		for(k=nHIdxL; k<=nHIdxR; k++)
		{
			if (Histogram_H18[k] > nMax )
			{
				nMax = Histogram_H18[k];
				index = k;
			}
		}
		nH = index * 20;
	}
	else
	{
		for (i=0; i<ROIWH; i++)
		{
			if ( ( ROIPXINFO[i].H/20 <= nHIdxR || ROIPXINFO[i].H/20 >= nHIdxL) && ROIPXINFO[i].H != 0   )
			{
				ROIPXINFO[i].flag_H = TRUE;
				nH_Group_Number++;
				ColorHistogram[ROIPXINFO[i].H]++;

			}
		}
		//////////////////////////////
		for(k=0; k<=nHIdxR; k++)
		{
			if (Histogram_H18[k] > nMax )
			{
				nMax = Histogram_H18[k];
				index = k;
			}
		}
		for (k=nHIdxL; k<18; k++)
		{
			if (Histogram_H18[k] > nMax )
			{
				nMax = Histogram_H18[k];
				index = k;
			}
		}
		nH = index * 20;
		
	}
	gpMsgbar->ShowMessage("index: %d\n", index);

	for (i=0; i<360; i++)
	{
		ColorHistogram[i] /= nH_Group_Number;
	}
	int nSlideWindows;
	//// S  //////////////////////////////////////////////////////////////////////
	int nSIdxR=0;
	int nSIdxL=0;
	double dSAve=0;
	double dSTmp1=0;
	double dSTmp2=0;
	double dSMax=0;
	double *Histogram_S = new double[100];
	double *Histogram_S_Tmp = new double[100];
	memset( Histogram_S, 0, sizeof(double)*100 );
	memset( Histogram_S_Tmp, 0, sizeof(double)*100 );
	
	for (i=0; i<ROIWH; i++ )
	{
		if( ROIPXINFO[i].flag_H == TRUE )
		{
			Histogram_S[ (int)(ROIPXINFO[i].S * 99) ]++;
		}
	}
	for(i=0, j=360; i<100; i++, j++)
	{
		ColorHistogram[j] = ( Histogram_S[i]/nH_Group_Number);
	}
	//////////////////////////////////////
	Histogram_S_Tmp[0] = Histogram_S[0];
	//integral image....
	for (i=1; i<100; i++)
	{
		Histogram_S_Tmp[i] = Histogram_S_Tmp[i-1] + Histogram_S[i]; 
	}
	//////////////////////////////////////////////////////////////////////////
	nSlideWindows = 10;
	for ( i=1; i<100-nSlideWindows; i++ )
	{
		dSTmp1 = ( Histogram_S_Tmp[ nSlideWindows + i ] - Histogram_S_Tmp[i-1] ) ;
		if( i==1 ) dSTmp1 += Histogram_S_Tmp[0];
		if ( dSTmp1 > dSMax )
		{
			dSMax = dSTmp1   ;
			nSIdxR = nSlideWindows + i;
			nSIdxL = i-1;
		}
	}
	
	nS = (nSIdxR-nSIdxL)/2 + nSIdxL;

// 	for (i=0; i<ROIWH; i++)
// 	{
// 		if ( ROIPXINFO[i].S*99 <= nSIdxR && ROIPXINFO[i].S*99 >= nSIdxL )
// 		{
// 			ROIPXINFO[i].flag_S = TRUE;
// 		}
// 	}

	//// V  //////////////////////////////////////////////////////////////////////
	int nVIdxR=0;
	int nVIdxL=0;
	double dVAve=0;
	double dVTmp1=0;
	double dVTmp2=0;
	double dVMax=0;
	double *Histogram_V = new double[100];
	double *Histogram_V_Tmp = new double[100];
	memset( Histogram_V, 0, sizeof(double)*100 );
	memset( Histogram_V_Tmp, 0, sizeof(double)*100 );
	
	for (i=0; i<ROIWH; i++ )
	{
		if( ROIPXINFO[i].flag_H == TRUE )
		{
			Histogram_V[ (int)(ROIPXINFO[i].V * 99) ]++;
		}
	}
	for(i=0, j=460; i<100; i++, j++)
	{
		ColorHistogram[j] = ( Histogram_V[i]/nH_Group_Number);
	}
	//////////////////////////////////////
	Histogram_V_Tmp[0] = Histogram_V[0];
	//integral image....
	for (i=1; i<100; i++)
	{
		Histogram_V_Tmp[i] = Histogram_V_Tmp[i-1] + Histogram_V[i]; 
	}
	//////////////////////////////////////////////////////////////////////////
	nSlideWindows = 10;
	for ( i=1; i<100-nSlideWindows; i++ )
	{
		dVTmp1 = ( Histogram_V_Tmp[ nSlideWindows + i ] - Histogram_V_Tmp[i-1] ) ;
		if( i==1 ) dVTmp1 += Histogram_V_Tmp[0];
		if ( dVTmp1 > dVMax )
		{
			dVMax = dVTmp1   ;
			nVIdxR = nSlideWindows + i;
			nVIdxL = i-1;
		}
	}

	nV = (nVIdxR-nVIdxL)/2 + nVIdxL;
	
// 	for (i=0; i<ROIWH; i++)
// 	{
// 
// 		if ( ROIPXINFO[i].V*99 <= nVIdxR && ROIPXINFO[i].V*99 >= nVIdxL )
// 		{
// 			ROIPXINFO[i].flag_V = TRUE;
// 		}
// 	}
	//////////////////////////////////////////////////////////////////////////
	int R, G, B;
	for (i=0; i<ROIWH; i++)
	{
		if ( ROIPXINFO[i].flag_H == TRUE )
		{
			R = ROIPXINFO[i].R*255;
			G = ROIPXINFO[i].G*255;
			B = ROIPXINFO[i].B*255;

			ColorHistogram[ 560 + R ]++;
			ColorHistogram[ 816 + G ]++;
			ColorHistogram[1072 + B ]++;
		}
	}
	for (i=560; i<1328; i++)
	{
		ColorHistogram[i] /= nH_Group_Number;
	}
// 	//////////////////////////////////////////////////////////////////////////
	double dMax;
	int    nSelect = 99;
	dMax = max( dHMax, max( dSMax, max( dVMax, zero ) ) );

// 	//////////////////////////////////////////////////////////////////////////
	int no1, no2, no3;
	no1=0; 
	no2=0; 
	no3=0;
// 	///DRAW HISTOGRAM

// 		//////////////////////////////////////////////////////////////////////////
		/////桌布顯示////////////////////////////////////////////////////////////
	if ( DISPLAY == TRUE )
	{
		int    H ;
		double S ;
		double V ;
		int R, G, B;
		int half = s_h /2;
		
		double di = (double)s_w/360;
		for (i=0; i<s_h/4; i++)
		{
			for( j= 0, k= 0; j< s_w; j++, k+=3)
			{
					H = j / di ;
					
					S =  i * 0.01 ; // 0.7
					V = (double) 0.7;
					HSV2RGB( H, S, V, R, G, B );
					if ( H != 0 )
					{
						HSV_Domain_Display[i*s_effw+k  ] = B ;
						HSV_Domain_Display[i*s_effw+k+1] = G ;
						HSV_Domain_Display[i*s_effw+k+2] = R ;
					}
			}
		}
		double *Histogram_H_Display = new double[s_w];
		memset( Histogram_H_Display, 0, sizeof(double)*s_w);
		di = (double) 360/s_w ; // 360 / 319
		for (i=0; i<360; i++)
		{
			Histogram_H_Display[(int)(i/di)] += Histogram_H[i];
		}

		for (i=0; i<s_w; i++)
		{
			P1.x = i /*+ s_w*/;
			P1.y = 0;
			
			P2.x = i /*+ s_w*/;
			P2.y = P1.y + Histogram_H_Display[i] / 20 ;
			FuncImg.DrawLine( HSV_Domain_Display, s_w, s_h, 3, P1, P2, RGB(255,0,0 ));
		}

		di = (double)s_w/18 ;
		P1.x = nHIdxL  * di + di * 0.5 ;
		P1.y = s_h/5;
		FuncImg.DrawCross( HSV_Domain_Display, s_w, s_h, 3, P1, 10, RGB(255,255,0), 2 );
		P1.x = nHIdxR * di + di * 0.5  ;
		P1.y = s_h/5;
		FuncImg.DrawCross( HSV_Domain_Display, s_w, s_h, 3, P1, 10, RGB(0,255,255), 2 );

		for (i=1; i<nInterval; i++)
		{
			P1.x = 360 / nInterval * i * s_w/360;
			P1.y = 0;
			P2.x = P1.x;
			P2.y = s_h/4;
			FuncImg.DrawLine( HSV_Domain_Display, s_w, s_h, 3, P1, P2, RGB(255,255,255) );
		}


		delete []Histogram_H_Display;

	}
	delete []Histogram_H;
	delete []Histogram_H_Tmp;
	delete []Histogram_H18;
	delete []Histogram_H18_Tmp;

	delete []Histogram_S;
	delete []Histogram_S_Tmp;

	delete []Histogram_V;
	delete []Histogram_V_Tmp;

}
void CRogerFunction::HSV2RGB( int H, double S, double V, int &R, int &G, int &B )
{
    // H = 0 - 360
	// S = 0 - 1
	// V = 0 - 1
	int i;
	double      hh, p, q, t, ff;
	
	V *= 255 ;

    hh = H;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = V * (1.0 - S);
    q = V * (1.0 - (S * ff));
    t = V * (1.0 - (S * (1.0 - ff)));
	
    switch(i) {
    case 0:
        R = V;
        G = t;
        B = p;
        break;
    case 1:
        R = q;
        G = V;
        B = p;
        break;
    case 2:
        R = p;
        G = V;
        B = t;
        break;
		
    case 3:
        R = p;
        G = q;
        B = V;
        break;
    case 4:
        R = t;
        G = p;
        B = V;
        break;
    case 5:
    default:
        R = V;
        G = p;
        B = q;
        break;
    }
	
// 	R *= 255;
// 	G *= 255;
// 	B *= 255;
}

void CRogerFunction::RGB2HSV( BYTE iR, BYTE iG, BYTE iB, int &H, double &S, double &V )
{

	double Maximum, Minimum;
	double R, G, B;
	B = (double)iB / 255;
	G = (double)iG / 255;
	R = (double)iR / 255;
	Maximum = max( R,  max( G, B ) );
	Minimum = min( R,  min( G, B ) );
			
			if( Maximum == Minimum )
			{
				H = 0;
			}
			else if( Maximum == R && G >= B )
			{
				H = 60 * (G-B)/(Maximum-Minimum) ;
			}
			else if( Maximum == R && G < B )
			{
				H = 60 * (G-B)/(Maximum-Minimum) + 360 ;
			}
			else if( Maximum == G )
			{
				H = 60 * (B-R)/(Maximum-Minimum) + 120 ;
			}
			else if( Maximum == B )
			{
				H = 60 * (R-G)/(Maximum-Minimum) + 240 ;
			}
			if( Maximum == 0 )
			{
				S = 0;
			}
			else
			{
				S = (Maximum-Minimum)/Maximum ;
			}
			V = Maximum ;
}
void CRogerFunction::RGB2HSV( double iR, double iG, double iB, int &H, double &S, double &V )
{
	
	double Maximum, Minimum;

	Maximum = max( iR,  max( iG, iB ) );
	Minimum = min( iR,  min( iG, iB ) );
	
	if( Maximum == Minimum )
	{
		H = 0;
	}
	else if( Maximum == iR && iG >= iB )
	{
		H = 60 * (iG-iB)/(Maximum-Minimum) ;
	}
	else if( Maximum == iR && iG < iB )
	{
		H = 60 * (iG-iB)/(Maximum-Minimum) + 360 ;
	}
	else if( Maximum == iG )
	{
		H = 60 * (iB-iR)/(Maximum-Minimum) + 120 ;
	}
	else if( Maximum == iB )
	{
		H = 60 * (iR-iG)/(Maximum-Minimum) + 240 ;
	}
	if( Maximum == 0 )
	{
		S = 0;
	}
	else
	{
		S = (Maximum-Minimum)/Maximum ;
	}
	V = Maximum ;
}

void CRogerFunction::SaveImage(BYTE *input, int nHeight, int nWidth, int nBpp, CPoint LB, CPoint RU, CString filename){
	CreateDirectory( "C:\\output\\", NULL );
	CxImage newImage;
	CString Path = "C:\\output\\";
	CString bmp=".bmp";
	int w = abs(RU.x - LB.x);
	int h = abs(RU.y - LB.y);
	int nEffw = nWidth * nBpp;
	BYTE *ImgBuffer = new BYTE[w*h*nBpp];
	BYTE *ptr1 = input;
	BYTE *ptr2 = ImgBuffer;
	for (int i=0,count1=0,count2=0;i<nHeight;i++,ptr1+=nEffw)
		for(int j=0;j<nEffw;j++)
			if(i>=LB.y && i<RU.y)
				if (j>=LB.x*nBpp && j<RU.x*nBpp)
					ptr2[count1++]=ptr1[j];

	
	newImage.CreateFromArray(ImgBuffer,w,h,8*nBpp,w*nBpp,0);
// 	newImage.Save("C:\\tmp\\test.bmp",CXIMAGE_FORMAT_BMP);
	newImage.Save(Path+filename+bmp,CXIMAGE_FORMAT_BMP);
// 	tempImage.Resample(width,height,2,0);
// 	tempImage.Save("C:\\Users\\WenYeh\\Desktop\\TESTFILE\\temp\\test.bmp",CXIMAGE_FORMAT_BMP);	newImage->Save(path, CXIMAGE_FORMAT_JPG);
//	newImage.Destroy();
//	delete []ImgBuffer;
}
void CRogerFunction::I_SaveImage(BYTE *SaveImage, int s_w, int s_h, int s_effw, char *SourceFileName)
{
	CreateDirectory("D:\\0Out\\", NULL);
	CxImage *newImage = new CxImage(0);
	newImage->CreateFromArray(SaveImage, s_w, s_h, 24, s_effw, false);
	newImage->SetJpegQuality(100);
	newImage->SetJpegScale(100);

	char tmp[] = "\\";
	char path[50] = "D:\\0Out\\";

	char ex[5] = ".bmp";

	strcat(path, SourceFileName);
	strcat(path, ex);
	newImage->Save(path, CXIMAGE_FORMAT_BMP);
	newImage->Destroy();
	delete newImage;
}

void CRogerFunction::TestAdaboost(BYTE *input, CARHEADINFO &CarDetectedA, int nHeight, int nWidth, int nBpp,  CxImage Image1, int frameCnt){
}

bool CRogerFunction::PtInCircle(CPoint P1, CPoint P2, int Distance){
	CImgProcess FuncImg;
	if( FuncImg.FindDistance(P1, P2) <= Distance)
		return true;
	else
		return false;
}
void CRogerFunction::Gray2Gradient(BYTE *in_grayimage, BYTE *ROI_GRADIENT, int *gradient_image, int s_w, int s_h, int diff_para)
{

	int i, j, k, x, y;
	int nPixel = s_w * s_h;
	float dSqtOne;
	float dSqtTwo;

	int *pnGradX = new int[nPixel];
	int *pnGradY = new int[nPixel];
	memset(pnGradX, 0, sizeof(int)*nPixel);
	memset(pnGradY, 0, sizeof(int)*nPixel);

	// 	BYTE *p_in_image  = in_image;

	int *p_gradient_image = gradient_image;
	int  *p_pnGradX = pnGradX;
	int  *p_pnGradY = pnGradY;

	BYTE *p_in_grayimage = in_grayimage;

	//...計算方向導數	
	//梯度計算
	int xshift1 = diff_para;
	k = diff_para;
	p_in_grayimage = in_grayimage + k*s_w;
	p_pnGradX = pnGradX + k*s_w;
	for (y = k; y < s_h - k; y++, p_pnGradX += s_w, p_in_grayimage += s_w) //x方向
	{
		for (x = k; x < s_w - k; x++)
		{
			p_pnGradX[x] = (int)(p_in_grayimage[x + k] - p_in_grayimage[x - k]);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	p_in_grayimage = in_grayimage + k*s_w;
	p_pnGradX = pnGradX + k*s_w;

	int yshift = k * s_w;
	int yshift1 = s_w;

	for (y = k; y < s_h - k; y++, p_pnGradY += s_w, p_in_grayimage += s_w) //y方向
	{
		for (x = k; x < s_w - k; x++)
		{
			p_pnGradY[x] = (int)(p_in_grayimage[x + yshift] - p_in_grayimage[x - yshift]);
		}
	}
	//...End 計算方向導數

	//...計算梯度
	p_in_grayimage = in_grayimage;
	p_pnGradX = pnGradX;
	p_pnGradY = pnGradY;
	for (y = 0; y < s_h; y++, p_pnGradX += s_w, p_pnGradY += s_w, p_gradient_image += s_w, ROI_GRADIENT+=s_w)
	{
		for (x = 0; x < s_w; x++)
		{
			dSqtOne = p_pnGradX[x] * p_pnGradX[x];
			dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
			p_gradient_image[x] = sqrtf(dSqtOne + dSqtTwo);
			ROI_GRADIENT[x] = (BYTE)p_gradient_image[x];
		}
	}
	//...End計算梯度

	delete[]pnGradX;
	delete[]pnGradY;
}
void parallel_row(int idx, BYTE *input, BYTE *output, int s_w, int s_Effw, BYTE *input_start, BYTE *output_start)
{
	input = input_start + (idx*s_Effw);
	output = output_start + (idx*s_w);
	int k = 0;
	int j;
	for (j = 0; j < s_w; j++)
	{
		output[j] = (int)(input[k] + input[k + 1] + input[k + 2]) / 3;
		k += 3;
	}
}
void parallel_row1(int y, int *p_pnGradX, int *p_pnGradY, BYTE *p_out_image, int *pnGradX, int *pnGradY, BYTE *out_image, int s_w, int s_Effw)
{
	double dSqtOne, dSqtTwo;
	p_pnGradX = pnGradX + y * s_w;
	p_pnGradY = pnGradY + y * s_w;
	p_out_image = out_image + y * s_w;
	for (int x = 0; x < s_w; x++)
	{
		dSqtOne = p_pnGradX[x] * p_pnGradX[x];
		dSqtTwo = p_pnGradY[x] * p_pnGradY[x];
		p_out_image[x] = (int)(sqrt(dSqtOne + dSqtTwo)/* + 0.5*/);
	}
}
void CRogerFunction::MP_Color2Gradient(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw)
{
	int i, j, k;
	int nPixel = s_w * s_h;
	double dSqtOne;
	double dSqtTwo;
	BYTE *pdTmp = new BYTE[nPixel];
	int *pnGradX = new int[nPixel];
	int *pnGradY = new int[nPixel];
	memset(pdTmp, 0, sizeof(BYTE)*nPixel);
	memset(pnGradX, 0, sizeof(int)*nPixel);
	memset(pnGradY, 0, sizeof(int)*nPixel);
	int x, y;
	BYTE *p_in_image = in_image;
	BYTE *p_out_image = out_image;
	int  *p_pnGradX = pnGradX;
	int  *p_pnGradY = pnGradY;
	BYTE *p_pdTmp = pdTmp;

#pragma omp parallel for
	for (i = 0; i < s_h; i++)
	{
		parallel_row(i, p_in_image, p_pdTmp, s_w, s_Effw, in_image, pdTmp);
	}
	//...計算方向導數	
	//梯度計算
	k = 3;
	p_pdTmp = pdTmp + k * s_w;
	p_pnGradX = pnGradX + k * s_w;

	for (y = k; y < s_h - k; y++, p_pnGradX += s_w, p_pdTmp += s_w) //x方向
	{
#pragma omp parallel for
		for (x = k; x < s_w - k; x++)
		{
			p_pnGradX[x] = (int)(p_pdTmp[(x + k)] - p_pdTmp[(x - k)]);
		}
	}
	//////////////////////////////////////////////////////////////////////////
	p_pdTmp = pdTmp + k*s_w;
	p_pnGradX = pnGradX + k*s_w;
	int yshift = k * s_w;
	int yshift1 = s_w;
	for (y = k; y < s_h - k; y++, p_pnGradY += s_w, p_pdTmp += s_w) //y方向
	{
#pragma omp parallel for
		for (x = k; x < s_w - k; x++)
		{
			p_pnGradY[x] = (int)(p_pdTmp[x + yshift] - p_pdTmp[x - yshift]);
		}
	}
	//...End 計算方向導數

	//...計算梯度
	p_pnGradX = pnGradX;
	p_pnGradY = pnGradY;
#pragma omp parallel for
	for (y = 0; y < s_h; y++)
	{
		parallel_row1(y, p_pnGradX, p_pnGradY, p_out_image, pnGradX, pnGradY, out_image, s_w, s_Effw);
	}
	//////////////////////////////////////////////////////////
	//...End計算梯度
	delete[]pdTmp;
	delete[]pnGradX;
	delete[]pnGradY;
}
void CRogerFunction::Parallel_SURF_MatTransLR(vector<double> &ivec, double *Array_Out)
{
	int a, b;

	double *p_Array_Out = Array_Out;

	for (a = 0; a < 4; a++, p_Array_Out += 16)
	{
		for (b = 0; b < 4; b++)
		{

			p_Array_Out[b * 4] = ivec[(3 - a) * 16 + b * 4];
			p_Array_Out[b * 4 + 1] = ivec[(3 - a) * 16 + b * 4 + 1];
			p_Array_Out[b * 4 + 2] = -ivec[(3 - a) * 16 + b * 4 + 3];
			p_Array_Out[b * 4 + 3] = -ivec[(3 - a) * 16 + b * 4 + 2];
		}
	}
}
void CRogerFunction::Parallel_SURF_Extract(BYTE *ROI_GRADIENT, int _width, int _height, vector<N_parallelsurf::N_KeyPoint> &keyPoints)
{
	N_SimpleGrayImage image1(ROI_GRADIENT, _width, _height);
	//Construct integral image
	N_parallelsurf::N_Image intImage1(const_cast<const unsigned char**> (image1.operator byte**()), image1.width(), image1.height());
	//Construct detector & descriptor
	N_parallelsurf::N_KeyPointDetector detector1;
	N_parallelsurf::N_KeyPointDescriptor descriptor1(intImage1, false);
	////Keypoint storage
	//std::vector<N_parallelsurf::N_KeyPoint> keyPoints;
	//Insertor inserts into keyPoints
	N_PsKeyPointVectInsertor insertor1(keyPoints);
	//Detect & describe
	// Detecting keypoints..
	detector1.detectKeyPoints(intImage1, insertor1);
	// Computing orientations..";
	descriptor1.assignOrientations(keyPoints.begin(), keyPoints.end());
	// Computing descriptors..";
	descriptor1.makeDescriptors(keyPoints.begin(), keyPoints.end());
}
void CRogerFunction::Symmetric_Parallel_SURF_Match(vector<N_parallelsurf::N_KeyPoint> &keyPoints, vector<NewMatchPair> &matchpair_bin)
{
	CImgProcess FuncImg;
	int i, j;
	CPoint Center;
	NewMatchPair matchpair;
	int nDist;
	int nBias = 5;
	int nIdxi, nIdxj;
	double dist, d1, d2;
	CPoint mPL, mPR, P1, P2;
	float sum;
	float dTheta;
	for (i = 0; i < keyPoints.size(); i++)
	{
		d1 = d2 = FLT_MAX;
		P1.x = (int)keyPoints[i]._x;
		P1.y = (int)keyPoints[i]._y;
		for (j = 0; j < keyPoints.size(); j++)
		{
			if (keyPoints[i]._trace == keyPoints[j]._trace && i != j && keyPoints[i]._used == false && keyPoints[j]._used == false)
			{
				P2.x = (int)keyPoints[j]._x;
				P2.y = (int)keyPoints[j]._y;
				int a, b;
				int idxi, idxj;
				sum = 0.f;
				for (a = 0; a < 4; a++)
				{
					for (b = 0; b < 4; b++)
					{
						idxi = a * 16 + b * 4;
						idxj = (3 - a) * 16 + b * 4;
						sum += (keyPoints[i]._vec[idxi] - keyPoints[j]._vec[idxj]) * (keyPoints[i]._vec[idxi] - keyPoints[j]._vec[idxj]);
						sum += (keyPoints[i]._vec[idxi + 1] - keyPoints[j]._vec[idxj + 1])*(keyPoints[i]._vec[idxi + 1] - keyPoints[j]._vec[idxj + 1]);
						sum += (keyPoints[i]._vec[idxi + 2] + keyPoints[j]._vec[idxj + 3])*(keyPoints[i]._vec[idxi + 2] + keyPoints[j]._vec[idxj + 3]);
						sum += (keyPoints[i]._vec[idxi + 3] + keyPoints[j]._vec[idxj + 2])*(keyPoints[i]._vec[idxi + 3] + keyPoints[j]._vec[idxj + 2]);
						dist = sqrt(sum);
					}
				}

				if (dist < d1)
				{
					d2 = d1;
					d1 = dist;
					//////////
					if (P1.x > P2.x)
					{
						mPL.x = P2.x;
						mPL.y = P2.y;
						mPR.x = P1.x;
						mPR.y = P1.y;
					}
					else
					{
						mPL.x = P1.x;
						mPL.y = P1.y;
						mPR.x = P2.x;
						mPR.y = P2.y;
					}
					nIdxi = i;
					nIdxj = j;
				}
				else if (dist < d2) // this feature matches better than second best
				{
					d2 = dist;
				}
			}
			//////////////////////////////////////////////////////////////////////////
		}
		if (d1 / d2 < 0.9)
		{
			dTheta = FuncImg.FindAngle(mPL, mPR);
			nDist = FuncImg.FindDistance(mPL.x, mPL.y, mPR.x, mPR.y);
			if ((dTheta < nBias || dTheta > 360 - nBias || (dTheta > 180 - nBias && dTheta < 180 + nBias)) /*&& nDist < nWidth/2*/)
			{
				keyPoints[nIdxi]._used = true;
				keyPoints[nIdxj]._used = true;
				Center.x = (mPL.x + mPR.x) / 2;
				Center.y = (mPL.y + mPR.y) / 2;
				//  					CenterPoint_bin.push_back(Center);
				//////////////////////////////////////////////////////////////////////////
				matchpair.CenterPoint = Center;
				matchpair.Point_L = mPL;
				matchpair.Point_R = mPR;
				matchpair_bin.push_back(matchpair);
			}
		}
	}
}
void CRogerFunction::Find_Center_Line(vector<NewMatchPair> &matchpair_bin, vector <int> &CenterPosX, int _width, int VehicleWidth, double *GramW, double *SmoothingW)
{
	int i, j, x;
	float sum;


	for (int i = 0; i < matchpair_bin.size(); i++)
	{
		GramW[matchpair_bin[i].CenterPoint.x]++;
	}
	int sw;
	double peak = 0.0;
	int shift = 2;
	sw = _width - shift;
	sum = 0;
	j = 0;
	for (i = shift; i < sw; i++) // smoothing
	{
		SmoothingW[i] = 0.1*GramW[i - 2] + 0.2*GramW[i - 1] + 0.4*GramW[i] + 0.2*GramW[i + 1] + 0.1*GramW[i + 2];
		sum += SmoothingW[i];
		if (SmoothingW[i]) j++;
		if (peak < SmoothingW[i]) peak = SmoothingW[i];
	}

	sum /= (j + 1);
	peak = (peak + sum) / 2; // threshold 

	double max_peak = 0.0;
	int preX = 0, best_xposition;
	j = 0;
	for (int i = VehicleWidth + shift; i < sw; i++)
	{
		if (SmoothingW[i] > sum
			&& SmoothingW[i - 1] <= SmoothingW[i]
			&& SmoothingW[i - 2] <= SmoothingW[i]
			&& SmoothingW[i + 1] <= SmoothingW[i]
			&& SmoothingW[i + 2] <= SmoothingW[i])
		{
			if ((i > preX + VehicleWidth && i < _width - VehicleWidth) || j == 0) // j=0 check whether it is the first peak 
			{
				CenterPosX.push_back(i);
				j++;
				preX = i;
				if (SmoothingW[i] > max_peak)
				{ // 紀錄最大的peak
					max_peak = SmoothingW[i]; best_xposition = i;
				}
			}
			else
			{
				x = CenterPosX[j - 1];
				if (SmoothingW[x] < SmoothingW[i]) // 兩個相鄰過近, 找最大的流下來
				{
					CenterPosX[j - 1] = i; preX = i;
					if (max_peak < SmoothingW[i])
					{
						best_xposition = i;
						max_peak = SmoothingW[i];
					}
				}
			}
		}
	}
}
void CRogerFunction::Find_Y_Position(int best_xposition, vector <int> &CenterPosY, double *dROI_GRADIENT_IntegralImage, double *dROI_GRAY_IntegralImage, double *GramW, double *ShadowHistogram, double *SmoothingW, int _width, int _height, int VehicleWidth, int VehicleHeight)
{
	double max_peak = 0.0;
	int shift = 5;
	int sh = _height - shift;
	int tolerance;
	int offest, area;
	offest = shift * _width;
	area = (2 * VehicleWidth + 1) * (2 * shift + 1);
	int j;

		// 計算水平邊的強度

		double *p_dROI_GRADIENT_IntegralImage = dROI_GRADIENT_IntegralImage + offest;
		double *p_dROI_GRAY_IntegralImage = dROI_GRAY_IntegralImage + offest;

		for (j = 0; j < shift; j++) { GramW[j] = 0; ShadowHistogram[j] = 255; }

		for (; j < sh; j++, p_dROI_GRADIENT_IntegralImage += _width, p_dROI_GRAY_IntegralImage += _width)
		{
			GramW[j] = p_dROI_GRADIENT_IntegralImage[best_xposition + VehicleWidth] + p_dROI_GRADIENT_IntegralImage[best_xposition - VehicleWidth - _width]
				- p_dROI_GRADIENT_IntegralImage[best_xposition + VehicleWidth - _width]
				- p_dROI_GRADIENT_IntegralImage[best_xposition - VehicleWidth] + 0.5;
			ShadowHistogram[j] = p_dROI_GRAY_IntegralImage[best_xposition + VehicleWidth + offest] + p_dROI_GRAY_IntegralImage[best_xposition - VehicleWidth - offest]
				- p_dROI_GRAY_IntegralImage[best_xposition + VehicleWidth - offest]
				- p_dROI_GRAY_IntegralImage[best_xposition - VehicleWidth + offest] + 0.5;

			ShadowHistogram[j] /= area;

			if (j > sh - shift) ShadowHistogram[j] = 255; // boundary condition

		}

		for (; j < _height; j++) { GramW[j] = 0; ShadowHistogram[j] = 255; }

		shift = 3;

		for (int k = shift; k < sh; k++) // 前後三個垂直位置都忽略
		{
			SmoothingW[k] = 0.0;
			for (j = -shift; j < shift; j++)
			{
				SmoothingW[k] += GramW[k + j];
			}

		}

		int sum = 150;

		int best_yposition = 0, y;
		j = 0; int preY = 0; max_peak = 0;
		for (int k = shift + VehicleHeight; k < sh; k++)
		{
			if (SmoothingW[k] > sum && SmoothingW[k - 1] <= SmoothingW[k] && SmoothingW[k - 2] <= SmoothingW[k]
				&& SmoothingW[k + 1] <= SmoothingW[k] && SmoothingW[k + 2] <= SmoothingW[k])
			{
				if (k > preY + VehicleHeight || j == 0) // j=0 check whether it is the first peak 
				{
					CenterPosY.push_back(k); j++; preY = k;
					if (SmoothingW[k] > max_peak)
					{ // 紀錄最大的peak
						max_peak = SmoothingW[k]; best_yposition = k;
					}
				}
				else
				{
					y = CenterPosY[j - 1];
					if (SmoothingW[y] < SmoothingW[k]) // 兩個相鄰過近, 找最大的流下來
					{
						CenterPosY[j - 1] = k; preY = k;
						best_yposition = k;
						max_peak = SmoothingW[k];
					}
				}

			}
		}
}
void CRogerFunction::SaveColorImage(BYTE *SaveImage, int s_w, int s_h, int s_effw, char *FileName)
{
	CxImage *newImage = new CxImage(0);
	newImage->CreateFromArray(SaveImage, s_w, s_h, 24, s_effw, false);
	newImage->SetJpegQuality(100);
	newImage->SetJpegScale(100);

	char path[300] = "D:\\0Out\\";
	CreateDirectory(path, NULL);

	strcat(path, FileName);
	char ex[5] = ".jpg";
	strcat(path, ex);
	newImage->Save(path, CXIMAGE_FORMAT_JPG);
	newImage->Destroy();
	delete newImage;
}
void CRogerFunction::SaveGrayImage(BYTE *SaveImage, int s_w, int s_h, int s_effw, char *FileName)
{
	int i, j, k;

	CxImage *newImage = new CxImage(0);
	newImage->CreateFromArray(SaveImage, s_w, s_h, 8, s_w, false);
	newImage->SetJpegQuality(100);
	newImage->SetJpegScale(100);


	char path[300] = "D:\\0Out\\";
	CreateDirectory(path, NULL);

	strcat(path, FileName);
	char ex[5] = ".jpg";
	strcat(path, ex);
	newImage->Save(path, CXIMAGE_FORMAT_JPG);
	newImage->Destroy();
	delete newImage;

}