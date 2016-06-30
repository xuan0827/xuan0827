// ImgProcess.cpp: implementation of the CImgProcess class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ImgProcess.h"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// 2007.10.31


#define		PI				3.141592653589793115997963468544185161590576171875
#define		PI2				6.28318530717959
#define		Dre2Pi			double(180.0 / PI)

#define SWAP3(list, i, j)					\
{											\
	register int *pi, *pj, tmp;				\
	pi = list + 3 * (i);					\
	pj = list + 3 * (j);					\
	tmp = *pi; *pi++ = *pj; *pj++ = tmp;	\
	tmp = *pi; *pi++ = *pj; *pj++ = tmp;	\
	tmp = *pi; *pi = *pj; *pj = tmp;		\
}


CImgProcess::CImgProcess()
{

}

CImgProcess::~CImgProcess()
{

}

int CImgProcess::Min(double Vector[], int Size)
{
	int i, ID = 0;
	double	Min_Value = Vector[ID];

	for(i=1;i<Size;i++)
		if(Min_Value > Vector[i])
		{
			Min_Value = Vector[i];
			ID = i;
		}

	return(ID);
}

int CImgProcess::Max(double Vector[], int Size)
{
	int i, ID = 0;
	double	Max_Value = Vector[ID];

	for(i=1;i<Size;i++)
		if(Max_Value < Vector[i])
		{
			Max_Value = Vector[i];
			ID = i;
		}

	return(ID);
}

int CImgProcess::Min(int Vector[], int Size)
{
	int i, ID = 0;
	int	Min_Value = Vector[ID];

	for(i=1;i<Size;i++)
		if(Min_Value > Vector[i])
		{
			Min_Value = Vector[i];
			ID = i;
		}

	return(ID);
}

int CImgProcess::Max(int Vector[], int Size)
{
	int i, ID = 0;
	int	Max_Value = Vector[ID];

	for(i=1;i<Size;i++)
		if(Max_Value < Vector[i])
		{
			Max_Value = Vector[i];
			ID = i;
		}

	return(ID);
}

BOOL CImgProcess::SobelOperation(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int InputBPP, int SobelThreshold, UINT SobelMode, BOOL BinaryProcess)
{
	// SobelMode : 0 (Sobel Vertical & Sobel Horizontal)
	//             1 (Sobel Horizontal)
	//             2 (Sobel Vertical)
	if(!pInput)
		return(FALSE);

	if(!((InputBPP == 3) || (InputBPP == 4)))
		return(FALSE);

	int		i, j, k, tempH, tempV,
			ImageCountWidth = InputBPP * (InputWidth - 1),
			ImagePW = (InputBPP * InputWidth + 3) >> 2 << 2,
			MaskPosition[] = {
				-ImagePW - InputBPP,	-ImagePW,	-ImagePW + InputBPP,
				-InputBPP,				0,			InputBPP,
				ImagePW - InputBPP,		ImagePW,	ImagePW + InputBPP},
			MaskOperatorHorizontal[] = {1, 0, -1, 2, 0, -2, 1, 0, -1},
			MaskOperatorVertical[] = {1, 2, 1, 0, 0, 0, -1, -2, -1};

	BYTE	*p_pInput = pInput,
			*p_pOutput = pOutput;
	memset(p_pOutput, 0, sizeof(BYTE) * InputHeight * ImagePW);

	p_pInput += ImagePW;
	p_pOutput += ImagePW;

	if(BinaryProcess)
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=ImagePW, p_pOutput+=ImagePW)
			for(j=InputBPP;j<ImageCountWidth;j++)
			{
				tempH = 0;
				tempV = 0;
				for(k=0;k<9;k++)
				{
					tempH += (p_pInput[j + MaskPosition[k]] * MaskOperatorHorizontal[k]);
					tempV += (p_pInput[j + MaskPosition[k]] * MaskOperatorVertical[k]);
				}

				switch(SobelMode)
				{
				case 1:		// Sobel Horizontal
				if(tempH > SobelThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;

				case 2:		// Sobel Vertical
				if(tempV > SobelThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;

				default:
				case 0:		// Sobel Vertical & Sobel Horizontal
				if((tempH > SobelThreshold) || (tempV > SobelThreshold))
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;
				}
			}
	}
	else
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=ImagePW, p_pOutput+=ImagePW)
			for(j=InputBPP;j<ImageCountWidth;j++)
			{
				tempH = 0;
				tempV = 0;
				for(k=0;k<9;k++)
				{
					tempH += (p_pInput[j + MaskPosition[k]] * MaskOperatorHorizontal[k]);
					tempV += (p_pInput[j + MaskPosition[k]] * MaskOperatorVertical[k]);
				}

				switch(SobelMode)
				{
				case 1:		// Sobel Horizontal
				if(tempH > SobelThreshold)
					p_pOutput[j] = ByteValueCheck(tempH);
				else
					p_pOutput[j] = 0;
					break;

				case 2:		// Sobel Vertical
				if(tempV > SobelThreshold)
					p_pOutput[j] = ByteValueCheck(tempV);
				else
					p_pOutput[j] = 0;
					break;

				default:
				case 0:		// Sobel Vertical & Sobel Horizontal
				if((tempH > SobelThreshold) || (tempV > SobelThreshold))
					p_pOutput[j] = ByteValueCheck((tempH + tempV) >> 1);
				else
					p_pOutput[j] = 0;
					break;
				}
			}
	}

	return(TRUE);
}

BOOL CImgProcess::LaplicationOperation(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int InputBPP, int LaplacianThreshold, BOOL BinaryProcess)
{
	if(!pInput)
		return(FALSE);

	if(!((InputBPP == 3) || (InputBPP == 4)))
		return(FALSE);

	int		i, j, k, temp,
			ImageCountWidth = InputBPP * (InputWidth - 1),
			ImagePW = (InputBPP * InputWidth + 3) >> 2 << 2,
			MaskPosition[] = {
				-ImagePW - InputBPP,	-ImagePW,	-ImagePW + InputBPP,
				-InputBPP,				0,			InputBPP,
				ImagePW - InputBPP,		ImagePW,	ImagePW + InputBPP},
			MaskOperator[] = {1, 1, 1, 1, -8, 1, 1, 1, 1};

	BYTE	*p_pInput = pInput,
			*p_pOutput = pOutput;
	memset(p_pOutput, 0, sizeof(BYTE) * InputHeight * ImagePW);

	p_pInput += ImagePW;
	p_pOutput += ImagePW;

	if(BinaryProcess)
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=ImagePW, p_pOutput+=ImagePW)
			for(j=InputBPP;j<ImageCountWidth;j++)
			{
				temp = 0;
				for(k=0;k<9;k++)
					temp += (p_pInput[j + MaskPosition[k]] * MaskOperator[k]);

				if(temp > LaplacianThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
			}
	}
	else
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=ImagePW, p_pOutput+=ImagePW)
			for(j=InputBPP;j<ImageCountWidth;j++)
			{
				temp = 0;
				for(k=0;k<9;k++)
					temp += (p_pInput[j + MaskPosition[k]] * MaskOperator[k]);

				if(temp > LaplacianThreshold)
					p_pOutput[j] = ByteValueCheck(temp);
				else
					p_pOutput[j] = 0;
			}
	}
	
	return(TRUE);
}

BOOL CImgProcess::GradientOperation(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int InputBPP, int GradientThreshold, UINT GradientMode, BOOL BinaryProcess)
{
	// GradientMode : 0 (Gradient Vertical & Gradient Horizontal)
	//                1 (Gradient Horizontal)
	//                2 (Gradient Vertical)
	if(!pInput)
		return(FALSE);

	if(!((InputBPP == 3) || (InputBPP == 4)))
		return(FALSE);

	int		i, j, k, tempH, tempV, temp,
			ImageCountWidth = InputBPP * (InputWidth - 1),
			ImagePW = (InputBPP * InputWidth + 3) >> 2 << 2,
			MaskPosition[] = {
				-ImagePW - InputBPP,	-ImagePW,	-ImagePW + InputBPP,
				-InputBPP,				0,			InputBPP,
				ImagePW - InputBPP,		ImagePW,	ImagePW + InputBPP},
			MaskOperatorHorizontal[] = {1, 0, -1, 1, 0, -1, 1, 0, -1},
			MaskOperatorVertical[] = {1, 1, 1, 0, 0, 0, -1, -1, -1};

	BYTE	*p_pInput = pInput,
			*p_pOutput = pOutput;
	memset(p_pOutput, 0, sizeof(BYTE) * InputHeight * ImagePW);

	p_pInput += ImagePW;
	p_pOutput += ImagePW;

	if(BinaryProcess)
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=ImagePW, p_pOutput+=ImagePW)
			for(j=InputBPP;j<ImageCountWidth;j++)
			{
				tempH = 0;
				tempV = 0;
				for(k=0;k<9;k++)
				{
					tempH += (p_pInput[j + MaskPosition[k]] * MaskOperatorHorizontal[k]);
					tempV += (p_pInput[j + MaskPosition[k]] * MaskOperatorVertical[k]);
				}

				switch(GradientMode)
				{
				case 1:		// Gradient Horizontal
				if(tempH > GradientThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;

				case 2:		// Gradient Vertical
				if(tempV > GradientThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;

				default:
				case 0:		// Gradient Vertical & Gradient Horizontal
					
				if(sqrtf((tempH * tempH) + (tempV * tempV)) > GradientThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;
				}
			}
	}
	else
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=ImagePW, p_pOutput+=ImagePW)
			for(j=InputBPP;j<ImageCountWidth;j++)
			{
				tempH = 0;
				tempV = 0;
				for(k=0;k<9;k++)
				{
					tempH += (p_pInput[j + MaskPosition[k]] * MaskOperatorHorizontal[k]);
					tempV += (p_pInput[j + MaskPosition[k]] * MaskOperatorVertical[k]);
				}

				switch(GradientMode)
				{
				case 1:		// Gradient Horizontal
				if(tempH > GradientThreshold)
					p_pOutput[j] = ByteValueCheck(tempH);
				else
					p_pOutput[j] = 0;
					break;

				case 2:		// Gradient Vertical
				if(tempV > GradientThreshold)
					p_pOutput[j] = ByteValueCheck(tempV);
				else
					p_pOutput[j] = 0;
					break;

				default:
				case 0:		// Gradient Vertical & Gradient Horizontal

				temp = (int)sqrtf((tempH * tempH) + (tempV * tempV));
				if(temp > GradientThreshold)
					p_pOutput[j] = ByteValueCheck(temp);
				else
					p_pOutput[j] = 0;
					break;
				}
			}
	}
	return(TRUE);
}

BOOL CImgProcess::GradientOperationRAW(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int GradientThreshold, UINT GradientMode, BOOL BinaryProcess)
{
	// GradientMode : 0 (Gradient Vertical & Gradient Horizontal)
	//                1 (Gradient Horizontal)
	//                2 (Gradient Vertical)
	if(!pInput)
		return(FALSE);

	int		i, j, k, tempH, tempV, temp,
			ImageCountWidth = InputWidth - 1,
			MaskPosition[] = {
				-InputWidth - 1,	-InputWidth,	-InputWidth + 1,
				-1,					0,				1,
				InputWidth - 1,		InputWidth,		InputWidth + 1},
			MaskOperatorHorizontal[] = {1, 0, -1, 1, 0, -1, 1, 0, -1},
			MaskOperatorVertical[] = {1, 1, 1, 0, 0, 0, -1, -1, -1};

	BYTE	*p_pInput = pInput,
			*p_pOutput = pOutput;
	memset(p_pOutput, 0, sizeof(BYTE) * InputHeight * InputWidth);

	p_pInput += InputWidth;
	p_pOutput += InputWidth;

	if(BinaryProcess)
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=InputWidth, p_pOutput+=InputWidth)
			for(j=1;j<ImageCountWidth;j++)
			{
				tempH = 0;
				tempV = 0;
				for(k=0;k<9;k++)
				{
					tempH += (p_pInput[j + MaskPosition[k]] * MaskOperatorHorizontal[k]);
					tempV += (p_pInput[j + MaskPosition[k]] * MaskOperatorVertical[k]);
				}

				switch(GradientMode)
				{
				case 1:		// Gradient Horizontal
				if(tempH > GradientThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;

				case 2:		// Gradient Vertical
				if(tempV > GradientThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;

				default:
				case 0:		// Gradient Vertical & Gradient Horizontal
					
				if(sqrtf((tempH * tempH) + (tempV * tempV)) > GradientThreshold)
					p_pOutput[j] = 255;
				else
					p_pOutput[j] = 0;
					break;
				}
			}
	}
	else
	{
		for(i=1;i<InputHeight-1;i++, p_pInput+=InputWidth, p_pOutput+=InputWidth)
			for(j=1;j<ImageCountWidth;j++)
			{
				tempH = 0;
				tempV = 0;
				for(k=0;k<9;k++)
				{
					tempH += (p_pInput[j + MaskPosition[k]] * MaskOperatorHorizontal[k]);
					tempV += (p_pInput[j + MaskPosition[k]] * MaskOperatorVertical[k]);
				}

				switch(GradientMode)
				{
				case 1:		// Gradient Horizontal
				if(tempH > GradientThreshold)
					p_pOutput[j] = ByteValueCheck(tempH);
				else
					p_pOutput[j] = 0;
					break;

				case 2:		// Gradient Vertical
				if(tempV > GradientThreshold)
					p_pOutput[j] = ByteValueCheck(tempV);
				else
					p_pOutput[j] = 0;
					break;

				default:
				case 0:		// Gradient Vertical & Gradient Horizontal

				temp = (int)sqrtf((tempH * tempH) + (tempV * tempV));
				if(temp > GradientThreshold)
					p_pOutput[j] = ByteValueCheck(temp);
				else
					p_pOutput[j] = 0;
					break;
				}
			}
	}
	return(TRUE);
}

BOOL CImgProcess::Histogram(BYTE *p_Process_Image, int n_Process_W, int n_Process_H, double *t_PDFData, double *t_CDFData, BOOL b_PDF_Process_Enable, BYTE *p_PDF_Image, int n_PDF_H, BOOL b_CDF_Process_Enable, BYTE *p_CDF_Image, int n_CDF_H, int BPP)
{
	if(!p_Process_Image)
		return(FALSE);

	int		i, j ,k, l,
			ImagePW = (BPP * (n_Process_W) + 3) >> 2 << 2;

	BYTE	*p_pProcessImage = p_Process_Image;

	double	PDFMaxValue[3] = {0},
			CDFMaxValue[3] = {0},
			*p_tPDFData = t_PDFData,	// B, G, R
			*p_tCDFData = t_CDFData;	// B, G, R

	memset(p_tPDFData, 0, sizeof(double) * 768);
	memset(p_tCDFData, 0, sizeof(double) * 768);

	// Establish Histgrame Table (PDF)
	for(i=0;i<n_Process_H;i++, p_pProcessImage+=ImagePW)
		for(j=0, k=0;j<n_Process_W;j++, k+=BPP)
		{
			p_tPDFData[p_pProcessImage[k]]++;
			p_tPDFData[256 + p_pProcessImage[k + 1]]++;
			p_tPDFData[512 + p_pProcessImage[k + 2]]++;
		}

	// Establish Histgrame Table (CDF)
	p_tPDFData = t_PDFData;
	p_tCDFData = t_CDFData;

	t_CDFData[0] = t_PDFData[0];
	t_CDFData[256] = t_PDFData[256];
	t_CDFData[512] = t_PDFData[512];

	for(i=0;i<3;i++, p_tPDFData+=256, p_tCDFData+=256)
		for(j=1;j<256;j++)
			p_tCDFData[j] = p_tCDFData[j - 1] + p_tPDFData[j];

	// Find Max PDF, CDF Value
	p_tPDFData = t_PDFData;
	p_tCDFData = t_CDFData;
	for(i=0;i<3;i++, p_tPDFData+=256, p_tCDFData+=256)
		for(j=0;j<256;j++)
		{
			if(PDFMaxValue[i] <= p_tPDFData[j])
				PDFMaxValue[i] = p_tPDFData[j];
			if(CDFMaxValue[i] <= p_tCDFData[j])
				CDFMaxValue[i] = p_tCDFData[j];
		}

	// Normize to 0 ~ 1
	p_tPDFData = t_PDFData;
	p_tCDFData = t_CDFData;
	for(i=0;i<3;i++, p_tPDFData+=256, p_tCDFData+=256)
		for(j=0;j<256;j++)
		{
			p_tPDFData[j] = p_tPDFData[j] / PDFMaxValue[i];
			p_tCDFData[j] = p_tCDFData[j] / CDFMaxValue[i];
		}

	// Establish PDF Distribution
	if(b_PDF_Process_Enable)
	{
		int PDF_Image_Size = 768 * n_PDF_H;

		memset(p_PDF_Image, 0, sizeof(BYTE) * PDF_Image_Size);

		BYTE PDFTable[768] = {0};
		for(i=0;i<768;i++)
			PDFTable[i] = (int)(t_PDFData[i] * n_PDF_H);

		BYTE	*p_pPDF_Image = p_PDF_Image,
				*p_PDFTable = PDFTable;

		for(i=0;i<n_PDF_H;i++, p_pPDF_Image+=768)
			for(j=0, k=0;j<256;j++, k+=3)
			{
				p_PDFTable = PDFTable;
				for(l=0;l<3;l++, p_PDFTable+=256)
				{
					if(p_PDFTable[j] == i)
						p_pPDF_Image[k + l] = 255;
				}
			}
	}

	// Establish CDF Distribution
	if(b_CDF_Process_Enable)
	{
		int CDF_Image_Size = 768 * n_CDF_H;

		memset(p_CDF_Image, 0, sizeof(BYTE) * CDF_Image_Size);

		BYTE CDFTable[768] = {0};
		for(i=0;i<768;i++)
			CDFTable[i] = (int)(t_CDFData[i] * n_CDF_H);

		BYTE	*p_pCDF_Image = p_CDF_Image,
				*p_CDFTable = CDFTable;

		for(i=0;i<n_CDF_H;i++, p_pCDF_Image+=768)
			for(j=0, k=0;j<256;j++, k+=3)
			{
				p_CDFTable = CDFTable;
				for(l=0;l<3;l++, p_CDFTable+=256)
				{
					if(p_CDFTable[j] == i)
						p_pCDF_Image[k + l] = 255;
				}
			}
	}

	return(TRUE);
}

BOOL CImgProcess::Equalization(BYTE *p_Image, int Image_W, int Image_H, int Image_BPP)
{
	if(!p_Image)
		return(FALSE);

	int		i, j ,k,
			Image_PW = (Image_BPP * (Image_W) + 3) >> 2 << 2;
	BYTE	*p_p_Image = p_Image;
	double	PDFData[768] = {0},
			CDFData[768] = {0};
	double	PDFMaxValue[3] = {0},
			CDFMaxValue[3] = {0},
			*p_tPDFData = PDFData,	// B, G, R
			*p_tCDFData = CDFData;	// B, G, R
	BYTE	CDFTransTable[3][256] = {0};

	memset(PDFData, 0, sizeof(double) * 768);
	memset(CDFData, 0, sizeof(double) * 768);

	// Establish Histgrame Table (PDF)
	for(i=0;i<Image_H;i++, p_p_Image+=Image_PW)
		for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
		{
			p_tPDFData[p_p_Image[k]]++;
			p_tPDFData[256 + p_p_Image[k + 1]]++;
			p_tPDFData[512 + p_p_Image[k + 2]]++;
		}

	// Establish Histgrame Table (CDF)
	p_tPDFData = PDFData;
	p_tCDFData = CDFData;

	CDFData[0] = PDFData[0];
	CDFData[256] = PDFData[256];
	CDFData[512] = PDFData[512];

	for(i=0;i<3;i++, p_tPDFData+=256, p_tCDFData+=256)
		for(j=1;j<256;j++)
			p_tCDFData[j] = p_tCDFData[j - 1] + p_tPDFData[j];

	// Find Max PDF, CDF Value
	p_tPDFData = PDFData;
	p_tCDFData = CDFData;
	for(i=0;i<3;i++, p_tPDFData+=256, p_tCDFData+=256)
		for(j=0;j<256;j++)
		{
			if(PDFMaxValue[i] <= p_tPDFData[j])
				PDFMaxValue[i] = p_tPDFData[j];
			if(CDFMaxValue[i] <= p_tCDFData[j])
				CDFMaxValue[i] = p_tCDFData[j];
		}

	// Normize to 0 ~ 1
	p_tPDFData = PDFData;
	p_tCDFData = CDFData;
	for(i=0;i<3;i++, p_tPDFData+=256, p_tCDFData+=256)
		for(j=0;j<256;j++)
		{
			p_tPDFData[j] = p_tPDFData[j] / PDFMaxValue[i];
			p_tCDFData[j] = p_tCDFData[j] / CDFMaxValue[i];
			CDFTransTable[i][j] = (int)(p_tCDFData[j] / CDFMaxValue[i] * 255);
		}

	for(i=0;i<Image_H;i++, p_p_Image+=Image_PW)
		for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
		{
			p_p_Image[k    ] = CDFTransTable[0][p_p_Image[k    ]];
			p_p_Image[k + 1] = CDFTransTable[1][p_p_Image[k + 1]];
			p_p_Image[k + 2] = CDFTransTable[2][p_p_Image[k + 2]];
		}

	return(TRUE);
}


BOOL CImgProcess::Equalization(BYTE *p_Process_Image, int n_Process_W, int n_Process_H, BOOL b_PDF_Process_Enable, BYTE *p_PDF_Image, int n_PDF_H, BOOL b_CDF_Process_Enable, BYTE *p_CDF_Image, int n_CDF_H, int BPP)
{
	if(!p_Process_Image)
		return(FALSE);

	double	PDFData[768] = {0},
			CDFData[768] = {0};

	if(!Histogram(p_Process_Image, n_Process_W, n_Process_H, PDFData, CDFData, FALSE, NULL, NULL, FALSE, NULL, NULL, BPP))
		return(FALSE);

	int		i, j, k,
			ProcessPW = ((BPP * n_Process_W) + 3) >> 2 << 2;
	BYTE	*p_pProcess_Image = p_Process_Image;

	BYTE	CDFTransTable[3][256] = {0};
	double	*p_CDFData = CDFData;

	for(i=0;i<3;i++, p_CDFData+=256)
		for(j=0;j<256;j++)
			CDFTransTable[i][j] = (int)(p_CDFData[j] * 255);

	for(i=0;i<n_Process_H;i++, p_pProcess_Image+=ProcessPW)
		for(j=0, k=0;j<n_Process_W;j++, k+=BPP)
		{
			p_pProcess_Image[k    ] = CDFTransTable[0][p_pProcess_Image[k    ]];
			p_pProcess_Image[k + 1] = CDFTransTable[1][p_pProcess_Image[k + 1]];
			p_pProcess_Image[k + 2] = CDFTransTable[2][p_pProcess_Image[k + 2]];
		}

	if(!Histogram(p_Process_Image, n_Process_W, n_Process_H,
				PDFData, CDFData,
				b_PDF_Process_Enable, p_PDF_Image, n_PDF_H,
				b_CDF_Process_Enable, p_CDF_Image, n_PDF_H,
				BPP)
				)
		return(FALSE);

	return(TRUE);
}

BOOL CImgProcess::RGB2HSV(BYTE *p_InputImage, BYTE *p_OutputImage, int n_ImageWidth, int n_ImageHeight, int Optional)
{
	if(!p_InputImage)
		return(FALSE);

	int	i, j, k,
		PW = n_ImageWidth * 3 + n_ImageWidth % 4,
		max = 0,
		min = 0,
		delta = 0;
	BYTE	*p_pInputImage = p_InputImage,
			*p_pOutputImage = p_OutputImage;

	for(i=0;i<n_ImageHeight;i++, p_pInputImage+=PW, p_pOutputImage+=PW)
		for(j=0, k=0;j<n_ImageWidth;j++, k+=3)
		{
			max = __max(__max(p_pInputImage[k], p_pInputImage[k + 1]), p_pInputImage[k + 2]);
			min = __min(__min(p_pInputImage[k], p_pInputImage[k + 1]), p_pInputImage[k + 2]);

			p_pOutputImage[k + 2] = max; //V,c=1
			delta = max - min;

			if(max != 0)
				p_pOutputImage[k + 1] = (delta * 255) / max;
			else
				p_pOutputImage[k + 1] = delta; //S,c=2

			if(delta == 0)
				delta = 1;

			// R == max // H, c = 0
			if(p_pInputImage[k + 2] == max)
				// (G - B) / delta
				p_pOutputImage[k] = 212 + 42 * (p_pInputImage[k + 1] - p_pInputImage[k]) / delta;
			else if(p_pInputImage[k + 1] == max) //G==max
				// (B - R) / delta
				p_pOutputImage[k] = 127 + 42 * (p_pInputImage[k] - p_pInputImage[k + 2]) / delta;
			else
				// (R - G) / delta
				p_pOutputImage[k] = 42 + 42 * (p_pInputImage[k + 2] - p_pInputImage[k + 1]) / delta;
		}

	p_pOutputImage = p_OutputImage;
	for(i=0;i<n_ImageHeight && Optional<3;i++, p_pOutputImage+=PW)
		for(j=0, k=0;j<n_ImageWidth;j++, k+=3)
		{
			p_pOutputImage[k] =
			p_pOutputImage[k + 1] =
			p_pOutputImage[k + 2] = p_pOutputImage[k + Optional];
		}

	return(TRUE);
}

BOOL CImgProcess::HSV2RGB(BYTE *p_InputImage, BYTE *p_OutputImage, int n_ImageWidth, int n_ImageHeight)
{
	if(!p_InputImage)
		return(FALSE);

	int	i, j, k, y, z, r, g, b, delta,
		PW = n_ImageWidth * 3 + n_ImageWidth % 4;
	BYTE	*p_pInputImage = p_InputImage,
			*p_pOutputImage = p_OutputImage;

	for(i=0;i<n_ImageHeight;i++, p_pInputImage+=PW, p_pOutputImage+=PW)
	{
		for(j=0, k=0;j<n_ImageWidth;j++, k+=3)
		{
			delta = (p_pInputImage[k + 2] * p_pInputImage[k + 1]) / 255;
			// lowest channel
			z = p_pInputImage[k + 2] - delta;
			// color phase (H)
			y = p_pInputImage[k];

			if(y > 169)	// max = R
			{ 
				r = p_pInputImage[k + 2]; //r=V
				y = ((p_pInputImage[k] - 212) * delta) / 42;								
				
				if(y < 0)
				{
					b = z - y;
					g = z;
				}			
				else
				{
					b = z;
					g = z + y;
				}
			} 
			else if(y > 84)	// max = G
			{
				g = p_pInputImage[k + 2];
				y = ((p_pInputImage[k] - 127) * delta) / 42;

				if(y < 0)
				{
					r = z - y;
					b = z;
				}
				else
				{
					r = z;
					b = z + y;
				}
			} 
			else	//max = B
			{ 
				b = p_pInputImage[k + 2];
				y = ((p_pInputImage[k] - 42) * delta) / 42;

				if(y < 0)
				{
					g = z - y;
					r = z;
				}
				else
				{
					g = z;
					r = z + y;
				}
			}
			p_pOutputImage[k] = b;
			p_pOutputImage[k + 1] = g;
			p_pOutputImage[k + 2] = r;
		}
	}
	return(TRUE);
}

BOOL CImgProcess::EqualizationHSV(BYTE *p_InputImage, BYTE *p_OutputImage, int n_ImageWidth, int n_ImageHeight)
{
	int	i, j, k,
		sum = 0,
		Ssum = 0,
		PW = n_ImageWidth * 3 + n_ImageWidth % 4;
	int	*ary;
	BYTE	*p_pOutputImage = p_OutputImage;

	RGB2HSV(p_InputImage, p_OutputImage, n_ImageWidth, n_ImageHeight, 3);

	ary = new int[256];

	memset(ary, 0, sizeof(int) * 256);

	for(i=0;i<n_ImageHeight;i++, p_pOutputImage+=PW)
		for(j=0, k=0;j<n_ImageWidth;j++, k+=3)
		{
			ary[p_pOutputImage[(UINT)(k + 2)]]++;
		}

	for(i=0;i<256;i++) 
	{
		sum += ary[i];
		ary[i] = sum;
	}

	for(i=0;i<256;i++) 
		ary[i] = (ary[i] * 255) / sum;

	p_pOutputImage = p_OutputImage;
	for(i=0;i<n_ImageHeight;i++, p_pOutputImage+=PW)
		for(j=0, k=0;j<n_ImageWidth;j++, k+=3)
		{
			p_pOutputImage[(UINT)(k + 2)] = ary[p_pOutputImage[(UINT)(k + 2)]];
		}

	delete [] ary;
	HSV2RGB(p_OutputImage, p_OutputImage, n_ImageWidth, n_ImageHeight);
	return(TRUE);
}

void CImgProcess::MinimumFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP)
{
	int i, j, k, m, n,
		Process_PW = (n_Process_W * BPP + 3) >> 2 << 2,
		tdata[9][2];
	int	MaskMap[] = {
		- Process_PW - BPP,	- Process_PW,	- Process_PW + BPP,
		- BPP,				0,				BPP,
		Process_PW - BPP,	Process_PW,		Process_PW + BPP
		};

	memcpy(p_Result_Image, p_Process_Image, sizeof(BYTE) * Process_PW * n_Process_H);
	BYTE	*p_pProcess_Image = p_Process_Image,
			*p_pResult_Image = p_Result_Image;

	p_pProcess_Image += Process_PW;
	p_pResult_Image += Process_PW;
	for(i=1;i<n_Process_H-1;i++, p_pProcess_Image+=Process_PW, p_pResult_Image+=Process_PW)
		for(j=1, k=BPP;j<n_Process_W-1;j++, k+=BPP)
			for(n=0;n<3;n++)
			{
				for(m=0;m<9;m++)
				{
					tdata[m][0] = m;
					tdata[m][1] = p_pProcess_Image[k + n + MaskMap[m]];
				}

				for(m=0;m<8;m++)
					if(tdata[m][1] < tdata[m + 1][1])
					{
						swap(tdata[m][0], tdata[m + 1][0]);
						swap(tdata[m][1], tdata[m + 1][1]);
					}

				p_pResult_Image[k + n] = p_pProcess_Image[k + n + MaskMap[tdata[8][0]]];

			}
}

void CImgProcess::MaximumFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP)
{
	int i, j, k, m, n,
		Process_PW = (n_Process_W * BPP + 3) >> 2 << 2,
		tdata[9][2];
	int	MaskMap[] = {
		- Process_PW - BPP,	- Process_PW,	- Process_PW + BPP,
		- BPP,				0,				BPP,
		Process_PW - BPP,	Process_PW,		Process_PW + BPP
		};

	memcpy(p_Result_Image, p_Process_Image, sizeof(BYTE) * Process_PW * n_Process_H);
	BYTE	*p_pProcess_Image = p_Process_Image,
			*p_pResult_Image = p_Result_Image;

	p_pProcess_Image += Process_PW;
	p_pResult_Image += Process_PW;
	for(i=1;i<n_Process_H-1;i++, p_pProcess_Image+=Process_PW, p_pResult_Image+=Process_PW)
		for(j=1, k=BPP;j<n_Process_W-1;j++, k+=BPP)
			for(n=0;n<3;n++)
			{
				for(m=0;m<9;m++)
				{
					tdata[m][0] = m;
					tdata[m][1] = p_pProcess_Image[k + n + MaskMap[m]];
				}

				for(m=0;m<8;m++)
					if(tdata[m][1] > tdata[m + 1][1])
					{
						swap(tdata[m][0], tdata[m + 1][0]);
						swap(tdata[m][1], tdata[m + 1][1]);
					}

				p_pResult_Image[k + n] = p_pProcess_Image[k + n + MaskMap[tdata[8][0]]];

			}
}

void CImgProcess::MedianFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP)
{
	int i, j, k, m, n,
		Process_PW = (n_Process_W * BPP + 3) >> 2 << 2,
		tdata[9][2];
	int	MaskMap[] = {
		- Process_PW - BPP,	- Process_PW,	- Process_PW + BPP,
		- BPP,				0,				BPP,
		Process_PW - BPP,	Process_PW,		Process_PW + BPP
		};

	memcpy(p_Result_Image, p_Process_Image, sizeof(BYTE) * Process_PW * n_Process_H);
	BYTE	*p_pProcess_Image = p_Process_Image,
			*p_pResult_Image = p_Result_Image;

	p_pProcess_Image += Process_PW;
	p_pResult_Image += Process_PW;
	for(i=1;i<n_Process_H-1;i++, p_pProcess_Image+=Process_PW, p_pResult_Image+=Process_PW)
		for(j=1, k=BPP;j<n_Process_W-1;j++, k+=BPP)
			for(n=0;n<3;n++)
			{
				for(m=0;m<9;m++)
				{
					tdata[m][0] = m;
					tdata[m][1] = p_pProcess_Image[k + n + MaskMap[m]];
				}

				for(m=0;m<8;m++)
					if(tdata[m][1] > tdata[m + 1][1])
					{
						swap(tdata[m][0], tdata[m + 1][0]);
						swap(tdata[m][1], tdata[m + 1][1]);
					}

				p_pResult_Image[k + n] = p_pProcess_Image[k + n + MaskMap[tdata[4][0]]];

			}
}

void CImgProcess::AverageFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP)
{
	int i, j, k, m, n, Temp,
		Process_PW = (n_Process_W * BPP + 3) >> 2 << 2,
		MaskMap[] = {
		- Process_PW - BPP,	- Process_PW,	- Process_PW + BPP,
		- BPP,				0,				BPP,
		Process_PW - BPP,	Process_PW,		Process_PW + BPP
		};

	memset(p_Result_Image, 0, sizeof(BYTE) * Process_PW * n_Process_H);
	BYTE	*p_pProcess_Image = p_Process_Image,
			*p_pResult_Image = p_Result_Image;

	p_pProcess_Image += Process_PW;
	p_pResult_Image += Process_PW;
	for(i=1;i<n_Process_H-1;i++, p_pProcess_Image+=Process_PW, p_pResult_Image+=Process_PW)
		for(j=1, k=BPP;j<n_Process_W-1;j++, k+=BPP)
			for(n=0;n<3;n++)
			{
				Temp = 0;
				for(m=0;m<9;m++)
					Temp += p_pProcess_Image[k + n + MaskMap[m]];
				p_pResult_Image[k + n] = Temp / 9;
			}
}

BOOL CImgProcess::Erosion(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h)
{
	int	i, j, x, y, maxvalue,
		half_mask_x = s_w >> 1,
		half_mask_y = s_h >> 1,
		offset = half_mask_y * inw;
	BYTE *in, *in1, *out;
	in = in_image + offset;
	out = out_image + offset;

	for(y=half_mask_y;y<inh-half_mask_y;y++, in+=inw, out+=inw) 
		for(x=half_mask_x;x<inw-half_mask_x;x++)
		{
			maxvalue = 255;
			in1 = in - offset + x;
			for(i=-half_mask_y;i<=half_mask_y;i++, in1+=inw)
				for(j=-half_mask_x;j<=half_mask_x;j++)
					maxvalue = Min(maxvalue, (int)in1[j]);
			out[x] = Max(maxvalue, 0);
		}
	return(TRUE);
}

BOOL CImgProcess::ErosionROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR)
{
	int	i, j, x, y, maxvalue,
		half_mask_x = s_w >> 1,
		half_mask_y = s_h >> 1,
		offset = half_mask_y * inw;
	BYTE *in, *in1, *out;
	in = in_image + UL.y * inw;
	out = out_image + UL.y * inw;

	for(y=UL.y;y<DR.y;y++, in+=inw, out+=inw) 
		for(x=UL.x;x<DR.x;x++)
		{
			maxvalue = 255;
			in1 = in - offset + x;
			for(i=-half_mask_y;i<=half_mask_y;i++, in1+=inw)
				for(j=-half_mask_x;j<=half_mask_x;j++)
					maxvalue = Min(maxvalue, (int)in1[j]);
			out[x] = Max(maxvalue, 0);
		}
	return(TRUE);
}

BOOL CImgProcess::Dilation(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h)
{
	int	i, j, x, y, maxvalue,
		half_mask_x = s_w >> 1,
		half_mask_y = s_h >> 1,
		offset = half_mask_y * inw;
	BYTE *in, *in1, *out;
	in = in_image + offset;
	out = out_image + offset;

	for(y=half_mask_y;y<inh-half_mask_y;y++, in+=inw, out+=inw)
		for(x=half_mask_x;x<inw-half_mask_x;x++)
		{
			maxvalue = 0;
			in1 = in - offset + x;
            for(i=-half_mask_y;i<=half_mask_y;i++, in1+=inw)
				for(j=-half_mask_x;j<=half_mask_x;j++)
					maxvalue = Max(maxvalue, (int)in1[j]);
			out[x] = Min(maxvalue, 255);
		}
	return(TRUE);
}

BOOL CImgProcess::DilationROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR)
{
	int	i, j, x, y, maxvalue,
		half_mask_x = s_w >> 1,
		half_mask_y = s_h >> 1,
		offset = half_mask_y * inw;
	BYTE *in, *in1, *out;
	in = in_image + UL.y * inw;
	out = out_image + UL.y * inw;

	for(y=UL.y;y<DR.y;y++, in+=inw, out+=inw)
		for(x=UL.x;x<DR.x;x++)
		{
			maxvalue = 0;
			in1 = in - offset + x;
            for(i=-half_mask_y;i<=half_mask_y;i++, in1+=inw)
				for(j=-half_mask_x;j<=half_mask_x;j++)
					maxvalue = Max(maxvalue, (int)in1[j]);
			out[x] = Min(maxvalue, 255);
		}
	return(TRUE);
}

BOOL CImgProcess::Opening(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h)
{
	BYTE *temp_img;
	temp_img = new BYTE[inw * inh];
	Erosion(in_image, inw, inh, temp_img, s_w, s_h);
    Dilation(temp_img, inw, inh, out_image, s_w, s_h);
	delete [] temp_img;	temp_img = NULL;
	return(TRUE);
}

BOOL CImgProcess::OpeningROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR)
{
	BYTE *temp_img;
	temp_img = new BYTE[inw * inh];
	ErosionROI(in_image, inw, inh, temp_img, s_w, s_h, UL, DR);
    DilationROI(temp_img, inw, inh, out_image, s_w, s_h, UL, DR);
	delete [] temp_img;	temp_img = NULL;
	return(TRUE);
}

BOOL CImgProcess::Closing(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h)
{
	BYTE *temp_img;
	temp_img = new BYTE[inw * inh];
	Dilation(in_image, inw, inh, temp_img, s_w, s_h);
	Erosion(temp_img, inw, inh, out_image, s_w, s_h);
	delete [] temp_img;	temp_img = NULL;
	return(TRUE);
}

BOOL CImgProcess::ClosingROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR)
{
	BYTE *temp_img;
	temp_img = new BYTE[inw * inh];
    DilationROI(in_image, inw, inh, temp_img, s_w, s_h, UL, DR);
	ErosionROI(temp_img, inw, inh, out_image, s_w, s_h, UL, DR);
	delete [] temp_img;	temp_img = NULL;
	return(TRUE);
}

BOOL CImgProcess::Different(BYTE *in1, BYTE *in2, BYTE *out, int inw, int inh)
{
	int i, size;
	size = inw * inh;
	for(i=0;i<size;i++)
		out[i] = in1[i] - in2[i];
	return(TRUE);
}



/*
BOOL CImgProcess::Thresholding(unsigned char *gray,unsigned char *in,int width, int height, int upT,int bottomT)
{
int			sum,i;
int			counter,size;

	sum = 0;
	counter = 0;
	size = width * height;

	for(i=0;i<size;i++)
	   if(in[i] > 0) {sum = sum + in[i]; counter++;}
	sum = sum/counter;

	if(upT == 0 && bottomT == 0) {
		upT = sum;
		bottomT = 255;
	}
	for(i=0;i<size;i++)
	   if(in[i] >upT && in[i] <=bottomT) in[i] = 255;
	else in[i] = 0;
	return(TRUE);
} // Threshold

*/

BOOL CImgProcess::Thresholding(BYTE *gray, BYTE *in, int inw, int inh, int upT, int bottomT)
{
	int	i, j, a, k;
	int	size, offset;
	int sum = 0, counter = 0, boundary = 5;
	double	b, rate;
//	int	total_sum[31];
	int c[31];

	size = inw * inh;
    offset = boundary * inw;
	for(i=0;i<31;i++)
	{ // total_sum[i]=0;
	  c[i]=0;
	}

	for(i=boundary, offset;i<inh-boundary;i++, offset+=inw)
		for(j=boundary;j<inw-boundary;j++)
			if(in[offset+i] > 0 && gray[offset+i] < 100)
			{
				a = k = in[offset+i];
				if(k > 30)
					k = 30;
				// total_sum[k]+=a;
				c[k]++;
				sum += a;
				counter++;
			}

	sum = sum / counter;
    counter = 0;
	k = 30;
	rate = 1.0 / inh;
	for(i=30;i>-1;i--)
	{
		counter += c[i];
		b = (double)(counter / size);
		if(b > rate)
			if(i > sum)
			{
				k = i;
				i = 0;
			}
	}

	if(k < 10)
		sum = k;

	if(upT == 0 && bottomT == 0)
	{
		upT = sum;
		bottomT = 255;
	}

	//	upT/=3;
	for(i=0;i<size;i++)
		if(in[i] > upT && in[i] <= bottomT && gray[i] < 50)
			in[i] = 255;
		else
			in[i] = 0;
	return(TRUE);
}

BOOL CImgProcess::Average(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h)
{
	int	i, j, x, y, maxvalue,
		half_mask_x = s_w >> 1,
		half_mask_y = s_h >> 1,
		offset = half_mask_y * inw,
		mask_size = s_w * s_h;
	BYTE *in, *in1, *out;
	in = in_image + offset;
	out = out_image + offset;
	
	for(y=half_mask_y;y<inh-half_mask_y;y++, in+=inw, out+=inw)
		for(x=half_mask_x;x<inw-half_mask_x;x++)
		{
			maxvalue = 0;
			in1 = in - offset + x;
			for(i=-half_mask_y;i<=half_mask_y;i++, in1+=inw)
				for(j=-half_mask_x;j<=half_mask_x;j++)
					maxvalue += in1[j];
			out[x] = maxvalue / mask_size;
		}
	return(TRUE);
} 

BOOL CImgProcess::AverageROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR)
{
	int	i, j, x, y, maxvalue,
		half_mask_x = s_w >> 1,
		half_mask_y = s_h >> 1,
		offset = half_mask_y * inw,
		mask_size = s_w * s_h;
	BYTE *in, *in1, *out;
	in = in_image + UL.y * inw;
	out = out_image + UL.y * inw;
	
	for(y=UL.y;y<DR.y;y++, in+=inw, out+=inw)
		for(x=UL.x;x<DR.x;x++)
		{
			maxvalue = 0;
			in1 = in - offset + x;
			for(i=-half_mask_y;i<=half_mask_y;i++, in1+=inw)
				for(j=-half_mask_x;j<=half_mask_x;j++)
					maxvalue += in1[j];
			out[x] = maxvalue / mask_size;
		}
	return(TRUE);
}

void CImgProcess::HarrisCornerDetector(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, int Threshold, double Weighting)
{
	int	i, j, k, m,
		Gradient, AddTempX, AddTempY, AddTempXY, AddTempYX,
		Image_PW = (Image_W * Image_BPP + 3) >> 2 << 2,
		Image_Size_8Bits = Image_W * Image_H,
		Image_Size = Image_PW * Image_H;

	int dy[] = {1, 0, -1, 2, 0, -2, 1, 0, -1},
		dx[] = {1, 2, 1, 0, 0, 0, -1, -2, -1},
		PositionTranslate[] = {
		-Image_PW - Image_BPP,	-Image_PW,	-Image_PW + Image_BPP,
		-Image_BPP,				0,			Image_BPP,
		Image_PW - Image_BPP,	Image_PW,	Image_PW + Image_BPP},
		PositionTranslate8Bits[] = {
		-Image_W - 1,	-Image_W,	-Image_W + 1,
		-1,				0,			1,
		Image_W - 1,	Image_W,	Image_W + 1};

	BYTE	*p_pImage = NULL;

	int	*Gx = new int[Image_Size_8Bits];
	int	*Gy = new int[Image_Size_8Bits];
	int	*Ixy = new int[Image_Size_8Bits];
	int	*Iyx = new int[Image_Size_8Bits];
	int	*p_Gx = NULL,
		*p_Gy = NULL,
		*p_Ixy = NULL,
		*p_Iyx = NULL;

	memset(Gx, 0, sizeof(int) * Image_Size_8Bits);
	memset(Gy, 0, sizeof(int) * Image_Size_8Bits);
	memset(Ixy, 0, sizeof(int) * Image_Size_8Bits);
	memset(Iyx, 0, sizeof(int) * Image_Size_8Bits);

	double R;

	p_pImage = (pImage + Image_PW + Image_BPP);
	p_Gx = (Gx + Image_W + 1);
	p_Gy = (Gy + Image_W + 1);

	for(i=1;i<Image_H-1;i++, p_pImage+=Image_PW, p_Gx+=Image_W, p_Gy+=Image_W)
		for(j=1, k=Image_BPP;j<Image_W-1;j++, k+=Image_BPP)
		{
			AddTempX = 0;
			AddTempY = 0;
			for(m=0;m<9;m++)
			{
				AddTempX += (p_pImage[k + PositionTranslate[m]] * dx[m]);
				AddTempY += (p_pImage[k + PositionTranslate[m]] * dy[m]);
			}
			p_Gx[j] = AddTempX;
			p_Gy[j] = AddTempY;
		}

	m = Image_W + 1;
	p_Gx = (Gx + m);
	p_Gy = (Gy + m);
	p_Ixy = (Ixy + m);
	p_Iyx = (Iyx + m);
	for(i=1;i<Image_H-1;i++, p_Gx+=Image_W, p_Gy+=Image_W, p_Ixy+=Image_W, p_Iyx+=Image_W)
		for(j=1;j<Image_W-1;j++)
		{
			AddTempXY = 0;
			AddTempYX = 0;
			for(m=0;m<9;m++)
			{
				AddTempXY += (p_Gx[j + PositionTranslate8Bits[m]] * dy[m]);
				AddTempYX += (p_Gy[j + PositionTranslate8Bits[m]] * dx[m]);
			}
			p_Ixy[j] = AddTempXY;
			p_Iyx[j] = AddTempYX;
		}

	memset(pImage, 0, sizeof(BYTE) * Image_Size);
	p_Gx = Gx;
	p_Gy = Gy;
	p_Ixy = Ixy;
	p_Iyx = Iyx;
	p_pImage = pImage;
	for(i=0;i<Image_H;i++, p_Gx+=Image_W, p_Gy+=Image_W, p_Ixy+=Image_W, p_Iyx+=Image_W, p_pImage+=Image_PW)
		for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
		{
			Gradient = ((p_Gx[j] * p_Gy[j]) - (p_Ixy[j] * p_Iyx[j]));
			R = Gradient - ((double)Weighting * (p_Gx[j] + p_Gy[j]) * (p_Gx[j] + p_Gy[j]));

			if(R > Threshold)
				p_pImage[k] = p_pImage[k + 1] = p_pImage[k + 2] = 255;
		}

	delete [] Gx;
	delete [] Gy;
	delete [] Ixy;
	delete [] Iyx;
}

BOOL CImgProcess::Histogram_RGB(BYTE *p_Process_Image, int n_Process_W, int n_Process_H, double *t_DataR, double *t_DataG, double *t_DataB, int NormalizeTo, int BPP)
{
	if(!p_Process_Image)
		return(FALSE);

	int		i, j ,k,
			ImagePW = (BPP * (n_Process_W) + 3) >> 2 << 2,
			Histogram_Max[3] = {0};

	BYTE	*p_pProcessImage = p_Process_Image;

	double	*p_tDataR = t_DataR,	// R
			*p_tDataG = t_DataG,	// G
			*p_tDataB = t_DataB;	// B

	memset(t_DataR, 0, sizeof(double) * 256);
	memset(t_DataG, 0, sizeof(double) * 256);
	memset(t_DataB, 0, sizeof(double) * 256);

	for(i=0;i<n_Process_H;i++, p_pProcessImage+=ImagePW)
		for(j=0, k=0;j<n_Process_W;j++, k+=BPP)
		{
			t_DataB[p_pProcessImage[k    ]]++;
			t_DataG[p_pProcessImage[k + 1]]++;
			t_DataR[p_pProcessImage[k + 2]]++;
		}

	if(NormalizeTo >= 0)
	{
		for(i=0;i<256;i++)
		{
			if(t_DataR[i] > Histogram_Max[0])
				Histogram_Max[0] = (int)t_DataR[i];
			if(t_DataG[i] > Histogram_Max[1])
				Histogram_Max[1] = (int)t_DataG[i];
			if(t_DataB[i] > Histogram_Max[2])
				Histogram_Max[2] = (int)t_DataB[i];
		}

		for(i=0;i<256;i++)
		{
			t_DataR[i] = (double)(t_DataR[i] * Histogram_Max[0]) / NormalizeTo;
			t_DataG[i] = (double)(t_DataG[i] * Histogram_Max[1]) / NormalizeTo;
			t_DataB[i] = (double)(t_DataB[i] * Histogram_Max[2]) / NormalizeTo;
		}
	}

	return(TRUE);
}

BYTE CImgProcess::ByteValueCheck(int CheckValue)
{
	CheckValue = abs(CheckValue);
	if(CheckValue > 255)
		return(255);
	return(CheckValue);
}

void CImgProcess::GaussianAverageFilter(BYTE *p_Process_Image, float *p_Result_Image, int n_Process_W, int n_Process_H, double S, int BPP)
{
	int	i, j, p, q, r, n, h, w,
		gaussian_filter_n,
		Process_PW = (n_Process_W * BPP + 3) >> 2 << 2;

	float	t1_r, t0_r, sum_r,
			t1_g, t0_g, sum_g,
			t1_b, t0_b, sum_b,
			Gaussian[20];
	float	*p_pResultImage;
	BYTE	*p_pProcessImage,
			*in2_r, *in3_r, *in4_r,
			*in2_g, *in3_g, *in4_g,
			*in2_b, *in3_b, *in4_b;

	if(S == 0)
	{
		gaussian_filter_n = 1;
		Gaussian[0] = 1.0;
	}
	else
		gaussian_filter_n = GetGaussianMask(S, Gaussian);

	if(BPP < 3)
	{
		::AfxMessageBox("Not a Color Image.");
		return;
	}

	p_pProcessImage = p_Process_Image + (n_Process_H - 1) * Process_PW;
    p_pResultImage = p_Result_Image;
    for(i=0;i<gaussian_filter_n;i++, p_pProcessImage-=Process_PW, p_pResultImage+=Process_PW)
		for(j=0;j<Process_PW;j++)
           	p_pResultImage[j] = (float)p_pProcessImage[j];

	h = n_Process_H - gaussian_filter_n;
	w = n_Process_W - gaussian_filter_n;
	for(;i<h;i++, p_pProcessImage-=Process_PW, p_pResultImage+=Process_PW)
	{
		for(j=0, n=0;j<gaussian_filter_n;j++, n+=BPP)
		{
			p_pResultImage[n    ] = (float)p_pProcessImage[n    ];
			p_pResultImage[n + 1] = (float)p_pProcessImage[n + 1];
			p_pResultImage[n + 2] = (float)p_pProcessImage[n + 2];		
		}

		for(;j<w;j++, n+=BPP)
		{
			in2_r = p_pProcessImage + n;
			in2_g = p_pProcessImage + n + 1;
			in2_b = p_pProcessImage + n + 2;
			t0_r = Gaussian[0] * in2_r[0];
			t0_g = Gaussian[0] * in2_g[0];
			t0_b = Gaussian[0] * in2_b[0];

			for(p=1, q=BPP;p<gaussian_filter_n;p++, q+=BPP)
			{
				t0_r += (Gaussian[p] * (in2_r[q] + in2_r[-q]));
				t0_g += (Gaussian[p] * (in2_g[q] + in2_g[-q]));
				t0_b += (Gaussian[p] * (in2_b[q] + in2_b[-q]));
			}

			sum_r = t0_r * Gaussian[0];
			sum_g = t0_g * Gaussian[0];
			sum_b = t0_b * Gaussian[0];
            in3_r = in2_r - Process_PW;
			in4_r = in2_r + Process_PW;
			in3_g = in2_g - Process_PW;
			in4_g = in2_g + Process_PW;
			in3_b = in2_b - Process_PW;
			in4_b = in2_b + Process_PW;

			for(r=1;r<gaussian_filter_n;r++, in3_r-=Process_PW, in4_r+=Process_PW, in3_g-=Process_PW, in4_g+=Process_PW, in3_b-=Process_PW, in4_b+=Process_PW)
			{
				t0_r = Gaussian[0] * in3_r[0];
				t1_r = Gaussian[0] * in4_r[0];
				t0_g = Gaussian[0] * in3_g[0];
				t1_g = Gaussian[0] * in4_g[0];
				t0_b = Gaussian[0] * in3_b[0];
				t1_b = Gaussian[0] * in4_b[0];

				for(p=1, q=BPP;p<gaussian_filter_n;p++, q+=BPP)
				{
					t0_r += (Gaussian[p] * (in3_r[q] + in3_r[-q]));
					t1_r += (Gaussian[p] * (in4_r[q] + in4_r[-q]));
					t0_g += (Gaussian[p] * (in3_g[q] + in3_g[-q]));
					t1_g += (Gaussian[p] * (in4_g[q] + in4_g[-q]));
					t0_b += (Gaussian[p] * (in3_b[q] + in3_b[-q]));
					t1_b += (Gaussian[p] * (in4_b[q] + in4_b[-q]));
				}

				sum_r += (Gaussian[r] * (t0_r + t1_r));
				sum_g += (Gaussian[r] * (t0_g + t1_g));
				sum_b += (Gaussian[r] * (t0_b + t1_b));
			}

			p_pResultImage[n    ] = sum_r;
			p_pResultImage[n + 1] = sum_g;
			p_pResultImage[n + 2] = sum_b;
		}

		for(;j<n_Process_W;j++, n+=BPP)
		{
           	p_pResultImage[n    ] = (float)p_pProcessImage[n    ];
			p_pResultImage[n + 1] = (float)p_pProcessImage[n + 1];
			p_pResultImage[n + 2] = (float)p_pProcessImage[n + 2];
		}
	}

	for(;i<n_Process_H;i++, p_pProcessImage-=Process_PW, p_pResultImage+=Process_PW)
		for(j=0;j<Process_PW;j++)
			p_pResultImage[j] = (float) p_pProcessImage[j];
}

int CImgProcess::GetGaussianMask(double s, float *Gaussian)
{
	int	i, n;
	float a, d = 0;
	for(n=0;n<20;++n)
	{
		a = (float)exp((- n * n) / (2 * s * s));
		if(a > 0.005 || n < 2)
		{
			a /= (float)(s * s);
			Gaussian[n] = a;
			d += a;
		}
		else
			break;
	}

	d = 2 * d - Gaussian[0];
	for(i=0;i<n;i++)
		Gaussian[i] /= d;
	return(n);
}

BOOL CImgProcess::HighBoostFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, double BoostValue, int BPP)
{
	if(!p_Process_Image)
		return(FALSE);

	if(BoostValue < 0)
		return(FALSE);

	int		i, j, k,
			ImageCountWidth = BPP * (n_Process_W - 1),
			ImagePW = (BPP * n_Process_W + 3) >> 2 << 2,
			MaskPosition[] = {
				-ImagePW - BPP,	-ImagePW,	-ImagePW + BPP,
				-BPP,			0,			BPP,
				ImagePW - BPP,	ImagePW,	ImagePW + BPP};

	double	temp, 
			MaskOperator[] = {-1, -1, -1, -1, (9 * BoostValue) - 1, -1, -1, -1, -1},
			SubValue = 9 * BoostValue - 9;

	BYTE	*p_p_Process_Image = p_Process_Image,
			*p_p_Result_Image = p_Result_Image;
	memset(p_Result_Image, 0, sizeof(BYTE) * n_Process_H * ImagePW);

	p_p_Process_Image += ImagePW;
	p_p_Result_Image += ImagePW;

	for(i=1;i<n_Process_H-1;i++, p_p_Process_Image+=ImagePW, p_p_Result_Image+=ImagePW)
		for(j=BPP;j<ImageCountWidth;j++)
		{
			temp = 0.0;
			for(k=0;k<9;k++)
				temp += (((double)p_p_Process_Image[j + MaskPosition[k]]) * MaskOperator[k]);
			temp /= SubValue;

			if(temp > 255)
				p_p_Result_Image[j] = 255;
			else if(temp < 0)
				p_p_Result_Image[j] = 0;
			else
				p_p_Result_Image[j] = (BYTE)temp;
		}

	return(TRUE);
}

double CImgProcess::Gaussian(double Input, double Variance, double Mean)
{
	double D = Input - Mean;
	return(exp((-D * D) / (2 * Variance * Variance)));
}

double CImgProcess::Gaussian2D(double Input_X, double Input_Y, double Variance, double Mean_X, double Mean_Y)
{
	double	Dx = (Input_X - Mean_X) * (Input_X - Mean_X),
			Dy = (Input_Y - Mean_Y) * (Input_Y - Mean_Y);
	return(exp((-Dx - Dy) / (2 * Variance * Variance)));
}

void CImgProcess::GaussianMatrix(int MatrixSize, double *GM, double Variance, double Mean_X, double Mean_Y)
{
	int	i, j, m, n,
		MatrixCenter = MatrixSize / 2;

	double TotalData = 0.0;
	double	*pGM = GM;

	for(i=0, m=-MatrixCenter;i<MatrixSize;i++, m++, pGM+=MatrixSize)
		for(j=0, n=-MatrixCenter;j<MatrixSize;j++, n++)
		{
			pGM[j] = Gaussian2D(m, n, Variance, Mean_X, Mean_Y);
			TotalData += pGM[j];
		}

	for(i=0;i<MatrixSize*MatrixSize;i++)
		GM[i] /= TotalData;
}

void CImgProcess::GaussianAverageFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int MaskSize, double Variance, double Mean_X, double Mean_Y, int BPP)
{
	if((MaskSize % 2) == 0)
		MaskSize--;
	
	int		i, j, k, m, n,
			MaskCenter = MaskSize / 2,
			ImageCountWidth = BPP * (n_Process_W - MaskCenter),
			ImagePW = (BPP * n_Process_W + 3) >> 2 << 2,
			TotalMaskCount = MaskSize * MaskSize;
	double	temp = 0.0;

	double	*GaussianMaskTable = new double[TotalMaskCount];
	GaussianMatrix(MaskSize, GaussianMaskTable, Variance, Mean_X, Mean_Y);

	int		*GaussianMaskPositionTable = new int[TotalMaskCount];
	for(i=0, m=-MaskCenter;i<MaskSize;i++, m++)
		for(j=0, n=-MaskCenter;j<MaskSize;j++, n++)
			GaussianMaskPositionTable[i * MaskSize + j] = (m * ImagePW) + (n * BPP);

	BYTE	*p_p_Process_Image = p_Process_Image,
			*p_p_Result_Image = p_Result_Image;
	memset(p_Result_Image, 0, sizeof(BYTE) * n_Process_H * ImagePW);

	p_p_Process_Image += (ImagePW * MaskCenter);
	p_p_Result_Image += (ImagePW * MaskCenter);

	for(i=MaskCenter;i<n_Process_H-MaskCenter;i++, p_p_Process_Image+=ImagePW, p_p_Result_Image+=ImagePW)
		for(j=(BPP*MaskCenter);j<ImageCountWidth;j++)
		{
			temp = 0.0;
			for(k=0;k<TotalMaskCount;k++)
				temp += (((double)p_p_Process_Image[j + GaussianMaskPositionTable[k]]) * GaussianMaskTable[k]);

			temp = (BYTE)(temp + 0.5);

			if(temp > 255)
				p_p_Result_Image[j] = 255;
			else if(temp < 0)
				p_p_Result_Image[j] = 0;
			else
				p_p_Result_Image[j] = (BYTE)temp;
		}

	delete [] GaussianMaskTable;
	delete [] GaussianMaskPositionTable;
}

int CImgProcess::ConnectComponent_BinaryInput(BYTE *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, int Threshold)
{
	int	i, j, k,
		index = 1,
		offset, boundary, x_start, x_end, y_start, y_end;
	
	BYTE a, b, c, d,
		*p_BufferImage, *p_GrayImage;

	if(!GrayImage || !Region_list)
		return(-1);

	BYTE	*BufferImage = new BYTE[Image_W * Image_H];
	int		*parent = new int[Image_W * Image_H];
	int		*Counter = NULL;

	// first pass
	boundary = 1;
	offset = boundary * Image_W;

	p_BufferImage = BufferImage + Image_W;
	p_GrayImage = GrayImage + Image_W;

	x_start = boundary;
	x_end = Image_W - boundary;
	y_start = boundary;
	y_end = Image_H - boundary;

	memset(BufferImage, 0, sizeof(BYTE) * Image_W * Image_H);
	memset(parent, 0, sizeof(int) * Image_W * Image_H);

	for(i=y_start;i<y_end;i++, p_GrayImage+=Image_W, p_BufferImage+=Image_W)
		for(j=x_start;j<x_end;j++)
			if(p_GrayImage[j] > 0)
			{
				a = p_BufferImage[j - Image_W - 1];
				b = p_BufferImage[j - Image_W];
				c = p_BufferImage[j - Image_W + 1];
				d = p_BufferImage[j - 1];

				if(a)
				{
					p_BufferImage[j] = a;
					if(b && a != b) Union(a, b, parent);
					if(c && a != c) Union(a, c, parent);
					if(d && a != d) Union(a, d, parent);
				}
				else if(b)
				{
					p_BufferImage[j] = b;
					if(c && b != c) Union(b, c, parent);
					if(d && b != d) Union(b, d, parent);
				}
				else if(c)
				{
					p_BufferImage[j] = c;
					if(d && c != d) Union(c, d, parent);
				}
				else if(d != 0)
					p_BufferImage[j] = d;
				else	// new label
				{
					p_BufferImage[j] = index;
					parent[index] = 0;
					index++;
				}
			}

	// Processing equivalent class
	for(i=1;i<index;i++)
		if(parent[i])		// find parents
		{
			j = i;
			while(parent[j])
				j = parent[j];
			parent[i] = j;
		}

	k = 1;
	for(i=1;i<index;i++)
		if(!parent[i])
		{
			parent[i] = k;
			k++;
		}
		else
		{
			j = parent[i];
			parent[i] = parent[j];
		}
	index = k;

	// Second Pass
	if(index == 1)
	{
		delete [] parent;
		delete [] BufferImage;
		return 0;
	}

	Counter = new int[index];

	// relabeling
	for(i=0;i<index;i++)
		Counter[i] = 0;

	p_BufferImage = BufferImage + Image_W;
	for(i=y_start;i<y_end;i++, p_BufferImage+=Image_W)
		for(j=x_start;j<x_end;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				k = parent[a];
				Counter[k]++;
				p_BufferImage[j] = k;
			}

	k = 0;
	for(i=0;i<index;i++)
		if(Counter[i] > Threshold)
		{
			Region_list[k].bottom = 0;
			Region_list[k].top = Image_H;
			Region_list[k].c = 0;
			Region_list[k].right = 0;
			Region_list[k].cx = 0;
			Region_list[k].cy = 0;
			Region_list[k].left = Image_W;
			Region_list[k].label = k + 1;
			parent[i] = k + 1;
			k++;
		}
		else
			parent[i] = 0;

	p_BufferImage = BufferImage;
	p_GrayImage = GrayImage;
	index = k;

	for(i=0;i<Image_H;i++, p_BufferImage+=Image_W, p_GrayImage+=Image_W)
		for(j=0;j<Image_W;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				if(Counter[a] > Threshold)
				{
					k = parent[a] - 1;
					Region_list[k].c += 1;
					Region_list[k].cx += j;
					Region_list[k].cy += i;
					
					if(Region_list[k].top > i)
						Region_list[k].top = i;
					if(Region_list[k].bottom < i)
						Region_list[k].bottom = i;
					if(Region_list[k].right < j)
						Region_list[k].right = j;
					if(Region_list[k].left > j)
						Region_list[k].left = j;

					p_GrayImage[j] = parent[a];
				}
				else
					p_GrayImage[j] = 0;
			}
			else
				p_GrayImage[j] = 0;

	for(i=0;i<index;i++)
	{
        Region_list[i].cx /= Region_list[i].c;
		Region_list[i].cy /= Region_list[i].c;
		Region_list[i].used = TRUE;
	}


	delete [] BufferImage;
	delete [] parent;
	delete [] Counter;
	return index;
}

int CImgProcess::ConnectComponent(int *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, int Threshold)
{
	int	i, j, k, a, j_P, j_N,
		O_o, O_a, O_b, O_c, O_d,
		T_a, T_b, T_c, T_d,
		index = 1,
		x_end, W_1, W_2;

	int *p_GrayImage = NULL;
	int *p_GrayImage_1 = NULL;
	int	*p_BufferImage = NULL;
	int	*p_BufferImage_1 = NULL;

	int	*BufferImage = new int[Image_W * Image_H];
	int	*parent = new int[Image_W * Image_H];
	int	*Counter = NULL;

	memset(BufferImage, 0, sizeof(int) * Image_W * Image_H);

	// first pass
	x_end = W_1 = Image_W - 1;
	W_2 = Image_W - 2;

	// First Line
	BufferImage[0] = index;
	parent[index] = 0;
	index++;

	p_GrayImage = GrayImage;
	p_BufferImage = BufferImage;
	for(j=1, j_N=0;j<Image_W;j++, j_N++)
	{
		O_o = p_GrayImage[j];
		O_d = p_GrayImage[j_N];
		T_d = p_BufferImage[j_N];

		if((O_o == O_d) && (T_d > 0))
		{
			p_BufferImage[j] = T_d;
		}
		else
		{
			p_BufferImage[j] = index;
			parent[index] = 0;
			index++;
		}
	}

	p_GrayImage = GrayImage + Image_W;
	p_BufferImage = BufferImage + Image_W;
	p_GrayImage_1 = GrayImage;
	p_BufferImage_1 = BufferImage;

	for(i=1;i<Image_H;i++, p_GrayImage+=Image_W, p_GrayImage_1+=Image_W, p_BufferImage+=Image_W, p_BufferImage_1+=Image_W)
	{
		O_o = p_GrayImage[0];
		O_b = p_GrayImage_1[0];
		O_c = p_GrayImage_1[1];
		T_b = p_BufferImage_1[0];
		T_c = p_BufferImage_1[1];

		if((O_o == O_b) && (T_b > 0))
		{
			p_BufferImage[0] = T_b;
			if(O_o == O_c)
				if((T_c > 0) && (T_b != T_c))
					Union(T_b, T_c, parent);
		}
		else if((O_o == O_c) && (T_c > 0))
		{
			p_BufferImage[0] = T_c;
		}
		else
		{
			p_BufferImage[0] = index;
			parent[index] = 0;
			index++;
		}

		for(j=1, j_P=2, j_N=0;j<x_end;j++, j_P++, j_N++)
		{
			O_o = p_GrayImage[j];			// Process Pixel
			O_a = p_GrayImage_1[j_N];
			O_b = p_GrayImage_1[j];
			O_c = p_GrayImage_1[j_P];
			O_d = p_GrayImage[j_N];
			T_a = p_BufferImage_1[j_N];
			T_b = p_BufferImage_1[j];
			T_c = p_BufferImage_1[j_P];
			T_d = p_BufferImage[j_N];

			if((O_o == O_a) && (T_a > 0))
			{
				p_BufferImage[j] = T_a;
				if(O_o == O_b)
					if((T_b > 0) && (T_a != T_b))
						Union(T_a, T_b, parent);
				if(O_o == O_c)
					if((T_c > 0) && (T_a != T_c))
						Union(T_a, T_c, parent);
				if(O_o == O_d)
					if((T_d > 0) && (T_a != T_d))
						Union(T_a, T_d, parent);
			}
			else if((O_o == O_b) && (T_b > 0))
			{
				p_BufferImage[j] = T_b;
				if(O_o == O_c)
					if((T_c > 0) && (T_b != T_c))
						Union(T_b, T_c, parent);
				if(O_o == O_d)
					if((T_d > 0) && (T_b != T_d))
						Union(T_b, T_d, parent);
			}
			else if((O_o == O_c) && (T_c > 0))
			{
				p_BufferImage[j] = T_c;
				if(O_o == O_d)
					if((T_d > 0) && (T_c != T_d))
						Union(T_c, T_d, parent);
			}
			else if((O_o == O_d) && (T_d > 0))
			{
				p_BufferImage[j] = T_d;
			}
			else
			{
				p_BufferImage[j] = index;
				parent[index] = 0;
				index++;
			}
		}

		O_o = p_GrayImage[W_1];			// Process Pixel
		O_a = p_GrayImage_1[W_2];

		O_b = p_GrayImage_1[W_1];
		O_d = p_GrayImage[W_2];
		T_a = p_BufferImage_1[W_2];
		T_b = p_BufferImage_1[W_1];
		T_d = p_BufferImage[W_2];

		if((O_o == O_a) && (T_a > 0))
		{
			p_BufferImage[W_1] = T_a;
			if(O_o == O_b)
				if((T_b > 0) && (T_a != T_b))
					Union(T_a, T_b, parent);
			if(O_o == O_d)
				if((T_d > 0) && (T_a != T_d))
					Union(T_a, T_d, parent);
		}
		else if((O_o == O_b) && (T_b > 0))
		{
			p_BufferImage[W_1] = T_b;
			if(O_o == O_d)
				if((T_d > 0) && (T_b != T_d))
					Union(T_b, T_d, parent);
		}
		else if((O_o == O_d) && (T_d > 0))
		{
			p_BufferImage[W_1] = T_d;
		}
		else
		{
			p_BufferImage[W_1] = index;
			parent[index] = 0;
			index++;
		}
	}

	if(index == 1)
	{
		delete [] BufferImage;	BufferImage = NULL;
		delete [] parent;		parent = NULL;
		return(0);
	}

	// Processing equivalent class
	for(i=1;i<index;i++)
		if(parent[i])		// find parents
		{
			j = i;
			while(parent[j])
				j = parent[j];
			parent[i] = j;
		}

	k = 1;
	for(i=1;i<index;i++)
		if(!parent[i])
		{
			parent[i] = k;
			k++;
		}
		else
		{
			j = parent[i];
			parent[i] = parent[j];
		}

	index = k;

	// Second Pass
	Counter = new int[index];

	// relabeling
	for(i=0;i<index;i++)
		Counter[i] = 0;

	p_BufferImage = BufferImage;
	for(i=0;i<Image_H;i++, p_BufferImage+=Image_W)
		for(j=0;j<Image_W;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				k = parent[a];
				Counter[k]++;
				p_BufferImage[j] = k;
			}

	k = 0;
	for(i=0;i<index;i++)
		if(Counter[i] >= Threshold)
		{
			Region_list[k].bottom = 0;
			Region_list[k].top = Image_H;
			Region_list[k].c = 0;
			Region_list[k].right = 0;
			Region_list[k].cx = 0;
			Region_list[k].cy = 0;
			Region_list[k].left = Image_W;
			Region_list[k].label = k + 1;
			parent[i] = k + 1;
			k++;
		}
		else
			parent[i] = 0;

	index = k;

	p_BufferImage = BufferImage;
	p_GrayImage = GrayImage;
	for(i=0;i<Image_H;i++, p_BufferImage+=Image_W, p_GrayImage+=Image_W)
		for(j=0;j<Image_W;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				if(Counter[a] >= Threshold)
				{
					k = parent[a] - 1;
					Region_list[k].c += 1;
					Region_list[k].cx += j;
					Region_list[k].cy += i;
					
					if(Region_list[k].top > i)
						Region_list[k].top = i;
					if(Region_list[k].bottom < i)
						Region_list[k].bottom = i;
					if(Region_list[k].right < j)
						Region_list[k].right = j;
					if(Region_list[k].left > j)
						Region_list[k].left = j;

					p_GrayImage[j] = parent[a];
				}
				else
					p_GrayImage[j] = 0;
			}
			else
				p_GrayImage[j] = 0;

	for(i=0;i<index;i++)
	{
        Region_list[i].cx /= Region_list[i].c;
		Region_list[i].cy /= Region_list[i].c;
	}

	delete [] BufferImage;	BufferImage = NULL;
	delete [] parent;		parent = NULL;
	delete [] Counter;		Counter = NULL;
	return(index);
}

int CImgProcess::ConnectComponentROI(int *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, CPoint ROI_UL, CPoint ROI_DR, int Threshold)
{
	int	i, j, k, a,
		O_o, O_a, O_b, O_c, O_d,
		T_a, T_b, T_c, T_d,
		index = 1,
		Process_W = ROI_DR.x - ROI_UL.x + 1,
		Process_H = ROI_DR.y - ROI_UL.y + 1,
		offset, boundary, x_start, x_end, y_start, y_end;

	if(!GrayImage || !Region_list)
		return(-1);

	int		*p_GrayImage = NULL,
			*p_BufferImage = NULL;
	int		*BufferImage = new int[Image_W * Image_H];
	int		*parent = new int[Image_W * Image_H];
	int		*Counter = NULL;

	memset(BufferImage, 0, sizeof(int) * Image_H * Image_W);
	memset(parent, 0, sizeof(int) * Image_W * Image_H);

	// first pass
	boundary = 1;
	offset = boundary * Image_W;

	x_start = boundary;
	x_end = Image_W - boundary;
	y_start = boundary;
	y_end = Image_H - boundary;

	if(x_start < ROI_UL.x)
		x_start = ROI_UL.x;
	if(x_end > ROI_DR.x)
		x_end = ROI_DR.x;
	if(y_start < ROI_UL.y)
		y_start = ROI_UL.y;
	if(y_end > ROI_DR.y)
		y_end = ROI_DR.y;

	p_BufferImage = BufferImage + Image_W * y_start;
	p_GrayImage = GrayImage + Image_W * y_start;

	for(i=y_start;i<y_end;i++, p_GrayImage+=Image_W, p_BufferImage+=Image_W)
		for(j=x_start;j<x_end;j++)
			if(p_GrayImage[j] > 0)
			{
				O_o = p_GrayImage[j];
				O_a = p_GrayImage[j - Image_W - 1];
				O_b = p_GrayImage[j - Image_W];
				O_c = p_GrayImage[j - Image_W + 1];
				O_d = p_GrayImage[j - 1];
				T_a = p_BufferImage[j - Image_W - 1];
				T_b = p_BufferImage[j - Image_W];
				T_c = p_BufferImage[j - Image_W + 1];
				T_d = p_BufferImage[j - 1];

				if((O_o == O_a) && (T_a > 0))
				{
					p_BufferImage[j] = T_a;
					if((O_o == O_b) && T_b && (T_a != T_b))	Union(T_a, T_b, parent);
					if((O_o == O_c) && T_c && (T_a != T_c)) Union(T_a, T_c, parent);
					if((O_o == O_d) && T_d && (T_a != T_d)) Union(T_a, T_d, parent);
				}
				else if((O_o == O_b) && (T_b > 0))
				{
					p_BufferImage[j] = T_b;
					if((O_o == O_c) && T_c && (T_b != T_c)) Union(T_b, T_c, parent);
					if((O_o == O_d) && T_d && (T_b != T_d)) Union(T_b, T_d, parent);
				}
				else if((O_o == O_c) && (T_c > 0))
				{
					p_BufferImage[j] = T_c;
					if((O_o == O_d) && T_d && (T_c != T_d)) Union(T_c, T_d, parent);
				}
				else if((O_o == O_d) && (T_d > 0))
				{
					p_BufferImage[j] = T_d;
				}
				else
				{
					p_BufferImage[j] = index;
					parent[index] = 0;
					index++;
				}
			}

	// Processing equivalent class
	for(i=1;i<index;i++)
		if(parent[i])		// find parents
		{
			j = i;
			while(parent[j])
				j = parent[j];
			parent[i] = j;
		}

	k = 1;
	for(i=1;i<index;i++)
		if(!parent[i])
		{
			parent[i] = k;
			k++;
		}
		else
		{
			j = parent[i];
			parent[i] = parent[j];
		}

	index = k;

	// Second Pass
	if(index == 1)
	{
		delete [] parent;
		delete [] BufferImage;
		return 0;
	}

	Counter = new int[index];

	// relabeling
	for(i=0;i<index;i++)
		Counter[i] = 0;

	p_BufferImage = BufferImage + Image_W * y_start;
	for(i=y_start;i<=y_end;i++, p_BufferImage+=Image_W)
		for(j=x_start;j<=x_end;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				k = parent[a];
				Counter[k]++;
				p_BufferImage[j] = k;
			}

	k = 0;
	for(i=0;i<index;i++)
		if(Counter[i] >= Threshold)
		{
			Region_list[k].bottom = 0;
			Region_list[k].top = Image_H;
			Region_list[k].c = 0;
			Region_list[k].right = 0;
			Region_list[k].cx = 0;
			Region_list[k].cy = 0;
			Region_list[k].left = Image_W;
			Region_list[k].label = k + 1;
			parent[i] = k + 1;
			k++;
		}
		else
			parent[i] = 0;

	p_BufferImage = BufferImage;
	p_GrayImage = GrayImage;
	index = k;

	for(i=0;i<Image_H;i++, p_BufferImage+=Image_W, p_GrayImage+=Image_W)
		for(j=0;j<Image_W;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				if(Counter[a] >= Threshold)
				{
					k = parent[a] - 1;
					Region_list[k].c += 1;
					Region_list[k].cx += j;
					Region_list[k].cy += i;
					
					if(Region_list[k].top > i)
						Region_list[k].top = i;
					if(Region_list[k].bottom < i)
						Region_list[k].bottom = i;
					if(Region_list[k].right < j)
						Region_list[k].right = j;
					if(Region_list[k].left > j)
						Region_list[k].left = j;

					p_GrayImage[j] = parent[a];
				}
				else
					p_GrayImage[j] = 0;
			}
			else
				p_GrayImage[j] = 0;

	for(i=0;i<index;i++)
	{
        Region_list[i].cx /= Region_list[i].c;
		Region_list[i].cy /= Region_list[i].c;
	}

	delete [] BufferImage;
	delete [] parent;
	delete [] Counter;
	return index;
}

void CImgProcess::Union(int x, int y, int *p)
{
	int j = x,
		k = y;
	
	while(p[j])	j = p[j];
	while(p[k])	k = p[k];

	if(j > k)	p[j] = k;
	if(j < k)	p[k] = j;
}

BOOL CImgProcess::Resample(BYTE *pOri_Image, int Ori_W, int Ori_H, BYTE *pDes_Image, int Des_W, int Des_H, int Image_BPP, UINT Sampling_Mode)
{
	if(Des_W == 0 || Des_H == 0)
		return(FALSE);

	int i, j, k, Pixel_Pos,
		Image_Ori_PW = (Ori_W * Image_BPP + 3) >> 2 << 2,
		Image_Des_PW = (Des_W * Image_BPP + 3) >> 2 << 2;


	if(Ori_W == Des_W && Ori_H == Des_H)
	{
		memcpy(pDes_Image, pOri_Image, sizeof(BYTE) * Image_Ori_PW * Ori_H);
		return(TRUE);
	}

	float xScale, yScale;
	xScale = (float)Ori_W / (float)Des_W;
	yScale = (float)Ori_H / (float)Des_H;

	BYTE *p_pDes_Image = NULL;
	BYTE *p_pOri_Image = NULL;


	switch(Sampling_Mode)
	{
		// nearest pixel
		case 0:
		{
			int D_X, D_Y;
			p_pDes_Image = pDes_Image;
			if(Image_BPP == 1)
			{
				for(i=0;i<Des_H;i++, p_pDes_Image+=Image_Des_PW)
				{
					D_Y = (int)(i * yScale + 0.5);
					for(j=0;j<Des_W;j++)
					{
						D_X = (int)(j * xScale + 0.5) * Image_BPP;
						p_pDes_Image[j] = pOri_Image[D_Y * Image_Ori_PW + D_X];
					}
				}
			}
			else
			{
				for(i=0;i<Des_H;i++, p_pDes_Image+=Image_Des_PW)
				{
					D_Y = (int)(i * yScale + 0.5);
					for(j=0, k=0;j<Des_W;j++, k+=Image_BPP)
					{
						D_X = (int)(j * xScale + 0.5) * Image_BPP;
						Pixel_Pos = D_Y * Image_Ori_PW + D_X;
						p_pDes_Image[k] = pOri_Image[Pixel_Pos];
						p_pDes_Image[k + 1] = pOri_Image[Pixel_Pos + 1];
						p_pDes_Image[k + 2] = pOri_Image[Pixel_Pos + 2];
					}
				}
			}

			break;
		}

		// bilinear interpolation	
		case 1:
		{
			if(!(Ori_W > Des_W && Ori_H > Des_H && Image_BPP == 3))
			{
				long ifX, ifY, ifX1, ifY1, xmax, ymax;
				float ir1, ir2, ig1, ig2, ib1, ib2, dx, dy, fX, fY;
				RGBQUAD rgb1, rgb2, rgb3, rgb4;

				xmax = Ori_W - 1;
				ymax = Ori_H - 1;

				p_pDes_Image = pDes_Image;
				for(i=0;i<Des_H;i++, p_pDes_Image+=Image_Des_PW)
				{
					fY = i * yScale;
					ifY = (int)fY;
					ifY1 = min(ymax, ifY + 1);
					dy = fY - ifY;

					for(j=0, k=0;j<Des_W;j++, k+=Image_BPP)
					{
						fX = j * xScale;
						ifX = (int)fX;
						ifX1 = min(xmax, ifX+1);
						dx = fX - ifX;

						// Interpolate using the four nearest pixels in the source
						p_pOri_Image = pOri_Image + ifY * Image_Ori_PW + ifX * Image_BPP;
						rgb1.rgbBlue = p_pOri_Image[0];
						rgb1.rgbGreen = p_pOri_Image[1];
						rgb1.rgbRed = p_pOri_Image[2];
						p_pOri_Image = pOri_Image + ifY * Image_Ori_PW + ifX1 * Image_BPP;
						rgb2.rgbBlue = p_pOri_Image[0];
						rgb2.rgbGreen = p_pOri_Image[1];
						rgb2.rgbRed = p_pOri_Image[2];
						p_pOri_Image = pOri_Image + ifY1 * Image_Ori_PW + ifX * Image_BPP;
						rgb3.rgbBlue = p_pOri_Image[0];
						rgb3.rgbGreen = p_pOri_Image[1];
						rgb3.rgbRed = p_pOri_Image[2];
						p_pOri_Image = pOri_Image + ifY1 * Image_Ori_PW + ifX1 * Image_BPP;
						rgb4.rgbBlue = p_pOri_Image[0];
						rgb4.rgbGreen = p_pOri_Image[1];
						rgb4.rgbRed = p_pOri_Image[2];

						// Interplate in x direction:
						ir1 = rgb1.rgbRed	* (1 - dy) + rgb3.rgbRed	* dy;
						ig1 = rgb1.rgbGreen	* (1 - dy) + rgb3.rgbGreen	* dy;
						ib1 = rgb1.rgbBlue	* (1 - dy) + rgb3.rgbBlue	* dy;
						ir2 = rgb2.rgbRed	* (1 - dy) + rgb4.rgbRed	* dy;
						ig2 = rgb2.rgbGreen	* (1 - dy) + rgb4.rgbGreen	* dy;
						ib2 = rgb2.rgbBlue	* (1 - dy) + rgb4.rgbBlue	* dy;

						// Interpolate in y:
						p_pDes_Image[k + 2] = (BYTE)(ir1 * (1 - dx) + ir2 * dx);
						p_pDes_Image[k + 1] = (BYTE)(ig1 * (1 - dx) + ig2 * dx);
						p_pDes_Image[k    ] = (BYTE)(ib1 * (1 - dx) + ib2 * dx);
					}
				} 
			}
			else
			{
				const long ACCURACY = 1000;
				int u, v = 0; // coordinates in dest image
				long ii, jj; // index for faValue
				p_pDes_Image = pDes_Image;

				float fEndX;
				long* naAccu  = new long[Image_BPP * Des_W + Image_BPP];
				long* naCarry = new long[Image_BPP * Des_W + Image_BPP];
				long* naTemp;
				long  nWeightX, nWeightY;
				long nScale = (long)(ACCURACY * xScale * yScale);

				memset(naAccu,  0, sizeof(long) * Image_BPP * Des_W);
				memset(naCarry, 0, sizeof(long) * Image_BPP * Des_W);

				float fEndY = yScale - 1.0f;

				for(i=0;i<Ori_H;i++)
				{
					p_pOri_Image = pOri_Image + i * Image_Ori_PW;
					u = ii = 0;
					fEndX = xScale - 1.0f;

					if((float)i < fEndY)
					{
						// complete source row goes into dest row
						for(j=0;j<Ori_W;j++)
						{
							if((float)j < fEndX)
							{
								// complete source pixel goes into dest pixel
								for (jj=0;jj<Image_BPP;jj++)
									naAccu[ii + jj] += (*p_pOri_Image++) * ACCURACY;
							}
							else
							{ 
								// source pixel is splitted for 2 dest pixels
								nWeightX = (long)(((float)i - fEndX) * ACCURACY);

								for(jj=0;jj<Image_BPP;jj++)
								{
									naAccu[ii] += (ACCURACY - nWeightX) * (*p_pOri_Image);
									naAccu[Image_BPP + ii++] += nWeightX * (*p_pOri_Image++);
								}

								fEndX += xScale;
								u++;
							}
						}
					}
					else
					{
						// source row is splitted for 2 dest rows       
						nWeightY = (long)(((float)i - fEndY) * ACCURACY);

						for(j=0;j<Ori_W;j++)
						{
							if((float)j < fEndX)
							{
								// complete source pixel goes into 2 pixel
								for(jj=0;jj<Image_BPP;jj++)
								{
									naAccu[ii + jj] += ((ACCURACY - nWeightY) * (*p_pOri_Image));
									naCarry[ii + jj] += nWeightY * (*p_pOri_Image++);
								}
							}
							else
							{
								// source pixel is splitted for 4 dest pixels
								nWeightX = (int)(((float)j - fEndX) * ACCURACY);
								for(jj=0;jj<Image_BPP;jj++)
								{
									naAccu[ii] += ((ACCURACY - nWeightY) * (ACCURACY - nWeightX)) * (*p_pOri_Image) / ACCURACY;
									*p_pDes_Image++ = (BYTE)(naAccu[ii] / nScale);
									naCarry[ii] += (nWeightY * (ACCURACY - nWeightX) * (*p_pOri_Image)) / ACCURACY;
									naAccu[ii + Image_BPP] += ((ACCURACY - nWeightY) * nWeightX * (*p_pOri_Image)) / ACCURACY;
									naCarry[ii + Image_BPP] = (nWeightY * nWeightX * (*p_pOri_Image)) / ACCURACY;
									ii++;
									p_pOri_Image++;
								}
								fEndX += xScale;
								u++;
							}
						}

						
						if(u < Des_W)
						{
							// possibly not completed due to rounding errors
							for(jj=0;jj<Image_BPP;jj++)
								*p_pDes_Image++ = (BYTE)(naAccu[ii++] / nScale);
						}

						naTemp = naCarry;
						naCarry = naAccu;
						naAccu = naTemp;

						memset(naCarry, 0, sizeof(int) * Image_BPP);    // need only to set first pixel zero

						p_pDes_Image = pDes_Image + (++v * Image_Des_PW);
						fEndY += yScale;
					}
				}

				
				if(v < Des_H)
				{
					// possibly not completed due to rounding errors
					for(ii=0;ii<Image_BPP*Des_W;ii++)
						*p_pDes_Image++ = (BYTE)(naAccu[ii] / nScale);
				}

				delete [] naAccu;
				delete [] naCarry;
			}
		}

		// bicubic interpolation
		default:
		case 2:
		{
			int   i_x, i_y, xx, yy, m, n;
			float f_x, f_y, a, b, rr, gg, bb, r1, r2, rgb;

			p_pDes_Image = pDes_Image;
			if(Image_BPP == 1)
			{
				for(i=0;i<Des_H;i++, p_pDes_Image+=Image_Des_PW)
				{
					f_y = (float)((float)i * yScale - 0.5);
					i_y = (int)floor(f_y);
					a = f_y - (float)floor(f_y);

					for(j=0;j<Des_W;j++)
					{
						f_x = (float)((float)j * xScale - 0.5);
						i_x = (int)floor(f_x);
						b = f_x - (float)floor(f_x);

						rgb = 0.0;

						for(m=-1;m<3;m++)
						{
							r1 = b3spline((float)m - a);
							yy = i_y + m;

							if(yy < 0)		yy = 0;
							if(yy >= Ori_H)	yy = Ori_H - 1;

							for(n=-1;n<3;n++)
							{
								r2 = r1 * b3spline(b - (float)n);
								xx = i_x + n;

								if(xx < 0)		xx = 0;
								if(xx >= Ori_W)	xx = Ori_W - 1;

								rgb += pOri_Image[yy * Image_Ori_PW + xx] * r2;
							}
						}

						p_pDes_Image[j] = (BYTE)rgb;
					}
				}
			}
			else
			{
				for(i=0;i<Des_H;i++, p_pDes_Image+=Image_Des_PW)
				{
					f_y = (float)(i * yScale - 0.5);
					i_y = (int)floor(f_y);
					a = f_y - (float)floor(f_y);

					for(j=0, k=0;j<Des_W;j++, k+=Image_BPP)
					{
						f_x = (float)(j * xScale - 0.5);
						i_x = (int)floor(f_x);
						b = f_x - (float)floor(f_x);

						rr = gg = bb = 0.0;

						for(m=-1;m<3;m++)
						{
							r1 = b3spline((float) m - a);
							yy = i_y + m;

							if(yy < 0)		yy = 0;
							if(yy >= Ori_H)	yy = Ori_H - 1;

							for(n=-1;n<3;n++)
							{
								r2 = r1 * b3spline(b - (float)n);
								xx = i_x + n;

								if(xx < 0)		xx = 0;
								if(xx >= Ori_W)	xx = Ori_W - 1;

								p_pOri_Image = pOri_Image + yy * Image_Ori_PW + xx * Image_BPP;

								rr += p_pOri_Image[2] * r2;
								gg += p_pOri_Image[1] * r2;
								bb += p_pOri_Image[0] * r2;
							}
						}

						p_pDes_Image[k    ] = (BYTE)bb;
						p_pDes_Image[k + 1] = (BYTE)gg;
						p_pDes_Image[k + 2] = (BYTE)rr;
					}
				}
			}
			break;
		}
	}

	return(TRUE);
}

float CImgProcess::b3spline(float x)
{
	float	a, b, c, d,
			xm1 = x - 1.0f, // Was calculatet anyway cause the "if((x-1.0f) < 0)"
			xp1 = x + 1.0f,
			xp2 = x + 2.0f;

	if((xp2) <= 0.0f)
		a = 0.0f;
	else
		a = xp2 * xp2 * xp2; // Only float, not float -> double -> float

	if((xp1) <= 0.0f)
		b = 0.0f;
	else
		b = xp1 * xp1 * xp1;

	if(x <= 0)
		c = 0.0f;
	else
		c = x * x * x;

	if((xm1) <= 0.0f)
		d = 0.0f;
	else
		d = xm1 * xm1 * xm1;

	return (0.16666666666666666667f * (a - (4.0f * b) + (6.0f * c) - (4.0f * d)));
}

BYTE* CImgProcess::Rotate(BYTE *pImage, int Image_W, int Image_H, double RotateAngle, CSize &RotateSize, int Image_BPP)
{
	double	ang = - RotateAngle * acos((float)0) / 90,
			cos_angle = cos(ang),
			sin_angle = sin(ang);

	int		Des_PW,
			Ori_PW = (Image_W * Image_BPP + 3) >> 2 << 2;

	// Calculate the size of the new bitmap
	POINT	p1 = {0, 0},
			p2 = {Image_W, 0},
			p3 = {0, Image_H},
			p4 = {Image_W - 1, Image_H};
	POINT	newP1, newP2, newP3, newP4,
			leftTop, rightTop, leftBottom, rightBottom;

	newP1.x = p1.x;
	newP1.y = p1.y;
	newP2.x = (long)floor(p2.x * cos_angle - p2.y * sin_angle);
	newP2.y = (long)floor(p2.x * sin_angle + p2.y * cos_angle);
	newP3.x = (long)floor(p3.x * cos_angle - p3.y * sin_angle);
	newP3.y = (long)floor(p3.x * sin_angle + p3.y * cos_angle);
	newP4.x = (long)floor(p4.x * cos_angle - p4.y * sin_angle);
	newP4.y = (long)floor(p4.x * sin_angle + p4.y * cos_angle);

	leftTop.x = min(min(newP1.x, newP2.x), min(newP3.x, newP4.x));
	leftTop.y = min(min(newP1.y, newP2.y), min(newP3.y, newP4.y));
	rightBottom.x = max(max(newP1.x, newP2.x), max(newP3.x, newP4.x));
	rightBottom.y = max(max(newP1.y, newP2.y), max(newP3.y, newP4.y));
	leftBottom.x = leftTop.x;
	leftBottom.y = 2 + rightBottom.y;
	rightTop.x = 2 + rightBottom.x;
	rightTop.y = leftTop.y;

	RotateSize.cx = rightTop.x - leftTop.x;
	RotateSize.cy = leftBottom.y - leftTop.y;

	Des_PW = (RotateSize.cx * Image_BPP + 3) >> 2 << 2;

	BYTE	*Des_Buffer = new BYTE[Des_PW * RotateSize.cy];
	memset(Des_Buffer, 128, sizeof(BYTE) * Des_PW * RotateSize.cy);
	BYTE	*p_Des_Buffer = Des_Buffer;
	BYTE	*p_Ori_Buffer = NULL;

	int i, j, k, newX, newY, oldX, oldY;

	if(Image_BPP == 1)
	{
		for(i=leftTop.y, newY=0;i<=leftBottom.y;i++, newY++, p_Des_Buffer+=Des_PW)
			for(j=leftTop.x, newX=0;j<=rightTop.x;j++, newX++)
			{
				oldX = (long)(j * cos_angle + i * sin_angle - 0.5);
				oldY = (long)(i * cos_angle - j * sin_angle - 0.5);

				if((oldX >= 0) && (oldX < Image_W) && (oldY >= 0) && (oldY < Image_H))
					p_Des_Buffer[newX] = pImage[oldY * Ori_PW + oldX];
			}
	}
	else
	{
		for(i=leftTop.y, newY=0;i<=leftBottom.y;i++, newY++, p_Des_Buffer+=Des_PW)
			for(j=leftTop.x, newX=0, k=0;j<=rightTop.x;j++, newX++, k+=Image_BPP)
			{
				oldX = (long)(j * cos_angle + i * sin_angle - 0.5);
				oldY = (long)(i * cos_angle - j * sin_angle - 0.5);

				if((oldX >= 0) && (oldX < Image_W) && (oldY >= 0) && (oldY < Image_H))
				{
					p_Ori_Buffer = pImage + oldY * Ori_PW + oldX * Image_BPP;

					p_Des_Buffer[k    ] = p_Ori_Buffer[0];
					p_Des_Buffer[k + 1] = p_Ori_Buffer[1];
					p_Des_Buffer[k + 2] = p_Ori_Buffer[2];
				}
			}
	}

	return(Des_Buffer);
}

void CImgProcess::GaussianAverageFilterDouble(double *InputData, double *OutputData, int Data_W, int Data_H, int MaskSize, double Variance, double Mean_X, double Mean_Y, int BPP)
{
	if((MaskSize % 2) == 0)		MaskSize--;

	int		i, j, k, m, n,
			MaskCenter = MaskSize / 2,
			ImageCountWidth = BPP * (Data_W - MaskCenter),
			ImagePW = (BPP * Data_W + 3) >> 2 << 2,
			TotalMaskCount = MaskSize * MaskSize;
	double	temp = 0.0;

	double	*GaussianMaskTable = new double[TotalMaskCount];
	GaussianMatrix(MaskSize, GaussianMaskTable, Variance, Mean_X, Mean_Y);

	int		*GaussianMaskPositionTable = new int[TotalMaskCount];
	for(i=0, m=-MaskCenter;i<MaskSize;i++, m++)
		for(j=0, n=-MaskCenter;j<MaskSize;j++, n++)
			GaussianMaskPositionTable[i * MaskSize + j] = (m * ImagePW) + (n * BPP);

	double	*p_InputData = InputData,
			*p_OutputData = OutputData;
	memset(OutputData, 0, sizeof(double) * ImagePW * Data_H);

	p_InputData += (ImagePW * MaskCenter);
	p_OutputData += (ImagePW * MaskCenter);

	for(i=MaskCenter;i<Data_H-MaskCenter;i++, p_InputData+=ImagePW, p_OutputData+=ImagePW)
		for(j=(BPP*MaskCenter);j<ImageCountWidth;j++)
		{
			temp = 0.0;
			for(k=0;k<TotalMaskCount;k++)
				temp += (p_InputData[j + GaussianMaskPositionTable[k]] * GaussianMaskTable[k]);
			p_OutputData[j] = temp;
		}

	delete [] GaussianMaskTable;
	delete [] GaussianMaskPositionTable;
}

int CImgProcess::MomentThresholdRAW(BYTE *pImage, int Image_W, int Image_H)
{
	int i, j,
		ImageSize = Image_W * Image_H,
		CutoffThreshold = 1;

	double	Temp,
			U, U1, U2, Q, ThresholdTemp,
			PixelCount[256] = {0};

	BYTE	*p_pImage = pImage;

	for(i=0;i<Image_H;i++, p_pImage+=Image_W)
		for(j=0;j<Image_W;j++)
			PixelCount[p_pImage[j]]++;

	U = 0;
	for(i=0;i<256;i++)
	{
		PixelCount[i] /= ImageSize;
		U += (i * PixelCount[i]);
	}

	Q = PixelCount[0] + PixelCount[1];
	if(Q == 0)
		U1 = 0;
	else
		U1 = PixelCount[1] / Q;
	U2 = (U - (Q * U1)) / (1 - Q);

	ThresholdTemp = fabs(Q * (1 - Q) * (U1 - U2));

	for(i=2;i<256;i++)
	{
		Temp = Q;
		Q += PixelCount[i];
		if(Q == 0)
			U1 = 0;
		else
			U1 = ((Temp * U1) + (i * PixelCount[i])) / Q;

		U2 = (U - (Q * U1)) / (1 - Q);

		Temp = fabs(Q * (1 - Q) * (U1 - U2));

		if(Temp > ThresholdTemp)
		{
			ThresholdTemp = Temp;
			CutoffThreshold = i;
		}
	}

	p_pImage = pImage;
	for(i=0;i<Image_H;i++, p_pImage+=Image_W)
		for(j=0;j<Image_W;j++)
		{
			p_pImage[j] = (p_pImage[j] >= CutoffThreshold ? 255 : 0);
		}

	return(CutoffThreshold);
}

void CImgProcess::MomentThreshold(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B)
{
	int i, j, k,
		Image_Size = Image_W * Image_H,
		Image_PW = (Image_W * Image_BPP + 3) >> 2 << 2;

	CutoffThreshold_R = 1;
	CutoffThreshold_G = 1;
	CutoffThreshold_B = 1;

	double	Temp_R, Temp_G, Temp_B,
			U_R, U_G, U_B,
			U1_R, U1_G, U1_B,
			U2_R, U2_G, U2_B,
			Q_R, Q_G, Q_B,
			ThresholdTemp_R, ThresholdTemp_G, ThresholdTemp_B,
			PixelCount_R[256] = {0},
			PixelCount_G[256] = {0},
			PixelCount_B[256] = {0};

	BYTE	*p_pImage = pImage;

	if(Image_BPP == 1)
	{
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Image_W;j++)
				PixelCount_R[p_pImage[j]]++;

		U_R = 0;
		for(i=0;i<256;i++)
		{
			PixelCount_R[i] /= Image_Size;
			U_R += (i * PixelCount_R[i]);
		}

		Q_R = PixelCount_R[0] + PixelCount_R[1];
		if(Q_R == 0)
			U1_R = 0;
		else
			U1_R = PixelCount_R[1] / Q_R;
		U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);

		ThresholdTemp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));

		for(i=2;i<256;i++)
		{
			Temp_R = Q_R;
			Q_R += PixelCount_R[i];
			if(Q_R == 0)
				U1_R = 0;
			else
				U1_R = ((Temp_R * U1_R) + (i * PixelCount_R[i])) / Q_R;

			U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);

			Temp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));

			if(Temp_R > ThresholdTemp_R)
			{
				ThresholdTemp_R = Temp_R;
				CutoffThreshold_R = i;
			}
		}

		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Image_W;j++)
				p_pImage[j] = (p_pImage[j] >= CutoffThreshold_R ? 255 : 0);
	}
	else
	{
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
			{
				PixelCount_R[p_pImage[k    ]]++;
				PixelCount_G[p_pImage[k + 1]]++;
				PixelCount_B[p_pImage[k + 2]]++;
			}

		U_R = 0;
		U_G = 0;
		U_B = 0;
		for(i=0;i<256;i++)
		{
			PixelCount_R[i] /= Image_Size;
			U_R += (i * PixelCount_R[i]);
			PixelCount_G[i] /= Image_Size;
			U_G += (i * PixelCount_G[i]);
			PixelCount_B[i] /= Image_Size;
			U_B += (i * PixelCount_B[i]);
		}

		Q_R = PixelCount_R[0] + PixelCount_R[1];
		Q_G = PixelCount_G[0] + PixelCount_G[1];
		Q_B = PixelCount_B[0] + PixelCount_B[1];
		if(Q_R == 0)	U1_R = 0;
		else			U1_R = PixelCount_R[1] / Q_R;
		U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);
		if(Q_G == 0)	U1_G = 0;
		else			U1_G = PixelCount_G[1] / Q_G;
		U2_G = (U_G - (Q_G * U1_G)) / (1 - Q_G);
		if(Q_B == 0)	U1_B = 0;
		else			U1_B = PixelCount_B[1] / Q_B;
		U2_B = (U_B - (Q_B * U1_B)) / (1 - Q_B);

		ThresholdTemp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));
		ThresholdTemp_G = fabs(Q_G * (1 - Q_G) * (U1_G - U2_G));
		ThresholdTemp_B = fabs(Q_B * (1 - Q_B) * (U1_B - U2_B));

		for(i=2;i<256;i++)
		{
			Temp_R = Q_R;
			Q_R += PixelCount_R[i];
			Temp_G = Q_G;
			Q_G += PixelCount_G[i];
			Temp_B = Q_B;
			Q_B += PixelCount_B[i];
			
			if(Q_R == 0)	U1_R = 0;
			else			U1_R = ((Temp_R * U1_R) + (i * PixelCount_R[i])) / Q_R;
			U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);
			if(Q_G == 0)	U1_G = 0;
			else			U1_G = ((Temp_G * U1_G) + (i * PixelCount_G[i])) / Q_G;
			U2_G = (U_G - (Q_G * U1_G)) / (1 - Q_G);
			if(Q_B == 0)	U1_B = 0;
			else			U1_B = ((Temp_B * U1_B) + (i * PixelCount_B[i])) / Q_B;
			U2_B = (U_B - (Q_B * U1_B)) / (1 - Q_B);

			Temp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));
			Temp_G = fabs(Q_G * (1 - Q_G) * (U1_G - U2_G));
			Temp_B = fabs(Q_B * (1 - Q_B) * (U1_B - U2_B));

			if(Temp_R > ThresholdTemp_R)
			{
				ThresholdTemp_R = Temp_R;
				CutoffThreshold_R = i;
			}
			if(Temp_G > ThresholdTemp_G)
			{
				ThresholdTemp_G = Temp_G;
				CutoffThreshold_G = i;
			}
			if(Temp_B > ThresholdTemp_B)
			{
				ThresholdTemp_B = Temp_B;
				CutoffThreshold_B = i;
			}
		}
	
		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
			{
				p_pImage[k    ] = (p_pImage[k    ] >= CutoffThreshold_R ? 255 : 0);	
				p_pImage[k + 1] = (p_pImage[k + 1] >= CutoffThreshold_G ? 255 : 0);	
				p_pImage[k + 2] = (p_pImage[k + 2] >= CutoffThreshold_B ? 255 : 0);	
			}
	}
}

double CImgProcess::MomentDirectionRAW(BYTE *pImage, int Image_W, int Image_H, CPoint &Center_Point, int Threshold, int Assign_Value)
{
	int i, j,
		ShiftX, ShiftY,
		Object_Points = 0;

	BYTE *p_pImage = NULL;

	double	ProjectD, ProjectU, Project,
			ValueUXY = 0.0,
			ValueDX = 0.0,
			ValueDY = 0.0,
			Direction = 0.0;

	Center_Point = CPoint(0, 0);

	if(Assign_Value == -1)
	{
		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_W)
			for(j=0;j<Image_W;j++)
				if(p_pImage[j] <= Threshold)
				{
					Center_Point.x += j;
					Center_Point.y += i;
					Object_Points++;
				}
		Center_Point.x /= Object_Points;
		Center_Point.y /= Object_Points;

		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_W)
			for(j=0;j<Image_W;j++)
				if(p_pImage[j] <= Threshold)
				{
					ShiftX = j - Center_Point.x;
					ShiftY = i - Center_Point.y;
					ValueUXY += (ShiftX * ShiftY);
					ValueDX += (ShiftX * ShiftX);
					ValueDY += (ShiftY * ShiftY);
				}
	}
	else
	{
		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_W)
			for(j=0;j<Image_W;j++)
				if(p_pImage[j] == Assign_Value)
				{
					Center_Point.x += j;
					Center_Point.y += i;
					Object_Points++;
				}
		Center_Point.x /= Object_Points;
		Center_Point.y /= Object_Points;

		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_W)
			for(j=0;j<Image_W;j++)
				if(p_pImage[j] == Assign_Value)
				{
					ShiftX = j - Center_Point.x;
					ShiftY = i - Center_Point.y;
					ValueUXY += (ShiftX * ShiftY);
					ValueDX += (ShiftX * ShiftX);
					ValueDY += (ShiftY * ShiftY);
				}
	}


	ProjectU = 2 * ValueUXY;
	ProjectD = ValueDX - ValueDY;
	Project = ProjectU / ProjectD;
			
	if(ProjectD == 0)
		Direction = 90 / Dre2Pi;
	else if(fabs(ValueUXY) <= 10)
	{
		if(ProjectD >= 0)
			Direction = 0;
		else
			Direction = 90 / Dre2Pi;
	}
	else
	{
		Direction = 0.5 * atan(ProjectU / ProjectD);
		if(ProjectD < 0)
		{
			if(Direction > 0)
				Direction -= (90 / Dre2Pi);
			else
				Direction += (90 / Dre2Pi);
		}
	}

	return(Direction);
}

double CImgProcess::MomentDirectionROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint &Center_Point, CPoint UL, CPoint DR, int Threshold, int Assign_Value)
{
	int i, j,
		ShiftX, ShiftY,
		Process_W = DR.x - UL.x + 1,
		Process_H = DR.y - UL.y + 1,
		Object_Points = 0;

	BYTE *p_pImage = NULL;
	double	ProjectD, ProjectU, Project,
			ValueUXY = 0.0,
			ValueDX = 0.0,
			ValueDY = 0.0,
			Direction = 0.0;

	Center_Point = CPoint(0, 0);

	if(Assign_Value == -1)
	{
		p_pImage = pImage + UL.y * Image_W + UL.x;
		for(i=0;i<Process_H;i++, p_pImage+=Image_W)
			for(j=0;j<Process_W;j++)
				if(p_pImage[j] <= Threshold)
				{
					Center_Point.x += j;
					Center_Point.y += i;
					Object_Points++;
				}

		if(Object_Points == 0)
			return(-1);
		Center_Point.x /= Object_Points;
		Center_Point.y /= Object_Points;

		p_pImage = pImage + UL.y * Image_W + UL.x;
		for(i=0;i<Process_H;i++, p_pImage+=Image_W)
			for(j=0;j<Process_W;j++)
				if(p_pImage[j] <= Threshold)
				{
					ShiftX = j - Center_Point.x;
					ShiftY = i - Center_Point.y;
					ValueUXY += (ShiftX * ShiftY);
					ValueDX += (ShiftX * ShiftX);
					ValueDY += (ShiftY * ShiftY);
				}
	}
	else
	{
		p_pImage = pImage + UL.y * Image_W + UL.x;
		for(i=0;i<Process_H;i++, p_pImage+=Image_W)
			for(j=0;j<Process_W;j++)
				if(p_pImage[j] == Assign_Value)
				{
					Center_Point.x += j;
					Center_Point.y += i;
					Object_Points++;
				}

		if(Object_Points == 0)
			return(-1);
		Center_Point.x /= Object_Points;
		Center_Point.y /= Object_Points;

		p_pImage = pImage + UL.y * Image_W + UL.x;
		for(i=0;i<Process_H;i++, p_pImage+=Image_W)
			for(j=0;j<Process_W;j++)
				if(p_pImage[j] == Assign_Value)
				{
					ShiftX = j - Center_Point.x;
					ShiftY = i - Center_Point.y;
					ValueUXY += (ShiftX * ShiftY);
					ValueDX += (ShiftX * ShiftX);
					ValueDY += (ShiftY * ShiftY);
				}
	}


	ProjectU = 2 * ValueUXY;
	ProjectD = ValueDX - ValueDY;
	Project = ProjectU / ProjectD;
			
	if(ProjectD == 0)
		Direction = 90 / Dre2Pi;
	else if(fabs(ValueUXY) <= 10)
	{
		if(ProjectD >= 0)
			Direction = 0;
		else
			Direction = 90 / Dre2Pi;
	}
	else
	{
		Direction = 0.5 * atan(ProjectU / ProjectD);
		if(ProjectD < 0)
		{
			if(Direction > 0)
				Direction -= (90 / Dre2Pi);
			else
				Direction += (90 / Dre2Pi);
		}
	}

	Center_Point.x += UL.x;
	Center_Point.y += UL.y;

	return(Direction);
}

int CImgProcess::MomentThresholdROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR)
{
	int i, j,
		Process_W = DR.x - UL.x + 1,
		Process_H = DR.y - UL.y + 1,
		ImageSize = Process_W * Process_H,
		CutoffThreshold = 1;

	double	Temp,
			U, U1, U2, Q, ThresholdTemp,
			PixelCount[256] = {0};

	BYTE	*p_pImage = pImage + UL.y * Image_W + UL.x;

	for(i=0;i<=Process_H;i++, p_pImage+=Image_W)
		for(j=0;j<=Process_W;j++)
			PixelCount[p_pImage[j]]++;

	U = 0;
	for(i=0;i<256;i++)
	{
		PixelCount[i] /= ImageSize;
		U += (i * PixelCount[i]);
	}

	Q = PixelCount[0] + PixelCount[1];
	if(Q == 0)
		U1 = 0;
	else
		U1 = PixelCount[1] / Q;
	U2 = (U - (Q * U1)) / (1 - Q);

	ThresholdTemp = fabs(Q * (1 - Q) * (U1 - U2));

	for(i=2;i<256;i++)
	{
		Temp = Q;
		Q += PixelCount[i];
		if(Q == 0)
			U1 = 0;
		else
			U1 = ((Temp * U1) + (i * PixelCount[i])) / Q;

		U2 = (U - (Q * U1)) / (1 - Q);

		Temp = fabs(Q * (1 - Q) * (U1 - U2));

		if(Temp > ThresholdTemp)
		{
			ThresholdTemp = Temp;
			CutoffThreshold = i;
		}
	}

	p_pImage = pImage + UL.y * Image_W + UL.x;
	for(i=0;i<=Process_H;i++, p_pImage+=Image_W)
		for(j=0;j<=Process_W;j++)
			p_pImage[j] = (p_pImage[j] >= CutoffThreshold ? 255 : 0);

	return(CutoffThreshold);
}

void CImgProcess::MomentThresholdROI(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B)
{
	int i, j, k,
		Process_W = DR.x - UL.x + 1,
		Process_H = DR.y - UL.y + 1,
		Image_Size = Process_W * Process_H,
		Image_PW = (Image_W * Image_BPP + 3) >> 2 << 2;

	CutoffThreshold_R = 1;
	CutoffThreshold_G = 1;
	CutoffThreshold_B = 1;

	double	Temp_R, Temp_G, Temp_B,
			U_R, U_G, U_B,
			U1_R, U1_G, U1_B,
			U2_R, U2_G, U2_B,
			Q_R, Q_G, Q_B,
			ThresholdTemp_R, ThresholdTemp_G, ThresholdTemp_B,
			PixelCount_R[256] = {0},
			PixelCount_G[256] = {0},
			PixelCount_B[256] = {0};

	BYTE	*p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);

	if(Image_BPP == 1)
	{
		for(i=0;i<=Process_H;i++, p_pImage+=Image_PW)
			for(j=0;j<=Process_W;j++)
				PixelCount_R[p_pImage[j]]++;

		U_R = 0;
		for(i=0;i<256;i++)
		{
			PixelCount_R[i] /= Image_Size;
			U_R += (i * PixelCount_R[i]);
		}

		Q_R = PixelCount_R[0] + PixelCount_R[1];
		if(Q_R == 0)
			U1_R = 0;
		else
			U1_R = PixelCount_R[1] / Q_R;
		U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);

		ThresholdTemp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));

		for(i=2;i<256;i++)
		{
			Temp_R = Q_R;
			Q_R += PixelCount_R[i];
			if(Q_R == 0)
				U1_R = 0;
			else
				U1_R = ((Temp_R * U1_R) + (i * PixelCount_R[i])) / Q_R;

			U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);

			Temp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));

			if(Temp_R > ThresholdTemp_R)
			{
				ThresholdTemp_R = Temp_R;
				CutoffThreshold_R = i;
			}
		}

		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<=Process_H;i++, p_pImage+=Image_PW)
			for(j=0;j<=Process_W;j++)
				p_pImage[j] = (p_pImage[j] >= CutoffThreshold_R ? 255 : 0);
	}
	else
	{
		for(i=0;i<=Process_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<=Process_W;j++, k+=Image_BPP)
			{
				PixelCount_R[p_pImage[k    ]]++;
				PixelCount_G[p_pImage[k + 1]]++;
				PixelCount_B[p_pImage[k + 2]]++;
			}

		U_R = 0;
		U_G = 0;
		U_B = 0;
		for(i=0;i<256;i++)
		{
			PixelCount_R[i] /= Image_Size;
			U_R += (i * PixelCount_R[i]);
			PixelCount_G[i] /= Image_Size;
			U_G += (i * PixelCount_G[i]);
			PixelCount_B[i] /= Image_Size;
			U_B += (i * PixelCount_B[i]);
		}

		Q_R = PixelCount_R[0] + PixelCount_R[1];
		Q_G = PixelCount_G[0] + PixelCount_G[1];
		Q_B = PixelCount_B[0] + PixelCount_B[1];
		if(Q_R == 0)	U1_R = 0;
		else			U1_R = PixelCount_R[1] / Q_R;
		U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);
		if(Q_G == 0)	U1_G = 0;
		else			U1_G = PixelCount_G[1] / Q_G;
		U2_G = (U_G - (Q_G * U1_G)) / (1 - Q_G);
		if(Q_B == 0)	U1_B = 0;
		else			U1_B = PixelCount_B[1] / Q_B;
		U2_B = (U_B - (Q_B * U1_B)) / (1 - Q_B);

		ThresholdTemp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));
		ThresholdTemp_G = fabs(Q_G * (1 - Q_G) * (U1_G - U2_G));
		ThresholdTemp_B = fabs(Q_B * (1 - Q_B) * (U1_B - U2_B));

		for(i=2;i<256;i++)
		{
			Temp_R = Q_R;
			Q_R += PixelCount_R[i];
			Temp_G = Q_G;
			Q_G += PixelCount_G[i];
			Temp_B = Q_B;
			Q_B += PixelCount_B[i];
			
			if(Q_R == 0)	U1_R = 0;
			else			U1_R = ((Temp_R * U1_R) + (i * PixelCount_R[i])) / Q_R;
			U2_R = (U_R - (Q_R * U1_R)) / (1 - Q_R);
			if(Q_G == 0)	U1_G = 0;
			else			U1_G = ((Temp_G * U1_G) + (i * PixelCount_G[i])) / Q_G;
			U2_G = (U_G - (Q_G * U1_G)) / (1 - Q_G);
			if(Q_B == 0)	U1_B = 0;
			else			U1_B = ((Temp_B * U1_B) + (i * PixelCount_B[i])) / Q_B;
			U2_B = (U_B - (Q_B * U1_B)) / (1 - Q_B);

			Temp_R = fabs(Q_R * (1 - Q_R) * (U1_R - U2_R));
			Temp_G = fabs(Q_G * (1 - Q_G) * (U1_G - U2_G));
			Temp_B = fabs(Q_B * (1 - Q_B) * (U1_B - U2_B));

			if(Temp_R > ThresholdTemp_R)
			{
				ThresholdTemp_R = Temp_R;
				CutoffThreshold_R = i;
			}
			if(Temp_G > ThresholdTemp_G)
			{
				ThresholdTemp_G = Temp_G;
				CutoffThreshold_G = i;
			}
			if(Temp_B > ThresholdTemp_B)
			{
				ThresholdTemp_B = Temp_B;
				CutoffThreshold_B = i;
			}
		}
	
		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<=Process_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<=Process_W;j++, k+=Image_BPP)
			{
				p_pImage[k    ] = (p_pImage[k    ] >= CutoffThreshold_R ? 255 : 0);	
				p_pImage[k + 1] = (p_pImage[k + 1] >= CutoffThreshold_G ? 255 : 0);	
				p_pImage[k + 2] = (p_pImage[k + 2] >= CutoffThreshold_B ? 255 : 0);	
			}
	}

}

void CImgProcess::DrawRect(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, COLORREF LineColor)
{
	UL.x = Max((int)UL.x, 0);
	UL.y = Max((int)UL.y, 0);
	DR.x = Min((int)DR.x, Image_W - 1);
	DR.y = Min((int)DR.y, Image_H - 1);

	int i, j, k,
		Image_PW = (Image_BPP * Image_W + 3) >> 2 << 2,
		Rect_W = DR.x - UL.x + 1,
		Rect_H = DR.y - UL.y - 1;

	BYTE	Color_R = GetRValue(LineColor),
			Color_G = GetGValue(LineColor),
			Color_B = GetBValue(LineColor);

	BYTE	*p_pImage = pImage + (Image_PW * UL.y);

	j = UL.x * Image_BPP;
	for(i=UL.x;i<=DR.x;i++, j+=Image_BPP)
	{
		p_pImage[j    ] = Color_B;	p_pImage[j + 1] = Color_G;	p_pImage[j + 2] = Color_R;
	}

	p_pImage += Image_PW;
	j = UL.x * Image_BPP;
	k = (DR.x + 1) * Image_BPP;

	for(i=1;i<Rect_H;i++, p_pImage+=Image_PW)
	{
		p_pImage[j    ] = Color_B;	p_pImage[j + 1] = Color_G;	p_pImage[j + 2] = Color_R;
		p_pImage[k    ] = Color_B;	p_pImage[k + 1] = Color_G;	p_pImage[k + 2] = Color_R;
	}

	j = UL.x * Image_BPP;
	for(i=UL.x;i<=DR.x;i++, j+=Image_BPP)
	{
		p_pImage[j    ] = Color_B;	p_pImage[j + 1] = Color_G;	p_pImage[j + 2] = Color_R;
	}
}

void CImgProcess::DrawRect(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CRect LineRect, COLORREF LineColor)
{
	LineRect.left = Max((int)LineRect.left, 0);
	LineRect.top = Max((int)LineRect.top, 0);
	LineRect.right = Min((int)LineRect.right, Image_W - 1);
	LineRect.bottom = Min((int)LineRect.bottom, Image_H - 1);

	int i, j, k,
		Image_PW = (Image_BPP * Image_W + 3) >> 2 << 2,
		Rect_W = LineRect.right - LineRect.left + 1,
		Rect_H = LineRect.bottom - LineRect.top - 1;

	BYTE	Color_R = GetRValue(LineColor),
			Color_G = GetGValue(LineColor),
			Color_B = GetBValue(LineColor);

	BYTE	*p_pImage = pImage + (Image_PW * LineRect.top);

	j = LineRect.left * Image_BPP;
	for(i=LineRect.left;i<=LineRect.right;i++, j+=Image_BPP)
	{
		p_pImage[j    ] = Color_B;	p_pImage[j + 1] = Color_G;	p_pImage[j + 2] = Color_R;
	}

	p_pImage += Image_PW;
	j = LineRect.left * Image_BPP;
	k = (LineRect.right + 1) * Image_BPP;

	for(i=1;i<Rect_H;i++, p_pImage+=Image_PW)
	{
		p_pImage[j    ] = Color_B;	p_pImage[j + 1] = Color_G;	p_pImage[j + 2] = Color_R;
		p_pImage[k    ] = Color_B;	p_pImage[k + 1] = Color_G;	p_pImage[k + 2] = Color_R;
	}

	j = LineRect.left * Image_BPP;
	for(i=LineRect.left;i<=LineRect.right;i++, j+=Image_BPP)
	{
		p_pImage[j    ] = Color_B;	p_pImage[j + 1] = Color_G;	p_pImage[j + 2] = Color_R;
	}
}

void CImgProcess::DrawLine(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint Line_P1, CPoint Line_P2, COLORREF LineColor)
{
	int	i, 
		dist = FindDistance(Line_P1, Line_P2),
		Image_PW = (Image_W * Image_BPP + 3) >> 2 << 2;
	if(dist == 0)
		return;

	double	radian = FindAngle(Line_P1, Line_P2) / Dre2Pi,
			cosR = cos(radian),
			sinR = sin(radian);

	BYTE	Color_R = GetRValue(LineColor),
			Color_G = GetGValue(LineColor),
			Color_B = GetBValue(LineColor);

	BYTE *p_pImage = NULL;

	if(Image_BPP == 1)
	{
		p_pImage = pImage + (Image_PW * Line_P1.y) + (Image_BPP * Line_P1.x);
		p_pImage[0] = Color_B;

		for(i=0;i<dist;i++)
		{
			p_pImage = pImage + (Image_PW * (Line_P1.y - (int)(i * sinR))) + (Image_BPP * (Line_P1.x - (int)(i * cosR)));
			p_pImage[0] = Color_B;
		}

		p_pImage = pImage + (Image_PW * Line_P2.y) + (Image_BPP * Line_P2.x);
		p_pImage[0] = Color_B;
	}
	if (Image_BPP == 3 )
	{
		p_pImage = pImage + (Image_PW * Line_P1.y) + (Image_BPP * Line_P1.x);
		p_pImage[0] = Color_B;	p_pImage[1] = Color_G;	p_pImage[2] = Color_R;

		for (i = 0; i < dist; i++)
		{
			int x;
			int y;
			x = Line_P1.x - (int)(i * cosR);
			y = Line_P1.y - (int)(i * sinR);
			p_pImage = pImage + (Image_PW * y + Image_BPP * x);
			p_pImage[0] = Color_B;	p_pImage[1] = Color_G;	p_pImage[2] = Color_R;
		}

		p_pImage = pImage + (Image_PW * Line_P2.y) + (Image_BPP * Line_P2.x);
		p_pImage[0] = Color_B;	p_pImage[1] = Color_G;	p_pImage[2] = Color_R;
	}
}

void CImgProcess::DrawLine(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint Line_P1, CPoint Line_P2, COLORREF LineColorS, COLORREF LineColorE)
{
	int	i, 
		dist = FindDistance(Line_P1, Line_P2),
		Image_PW = (Image_W * Image_BPP + 3) >> 2 << 2;
	if(dist == 0)
		return;

	double	radian = FindAngle(Line_P1, Line_P2) / Dre2Pi,
			cosR = cos(radian),
			sinR = sin(radian);

	BYTE	Color_R = GetRValue(LineColorS),
			Color_G = GetGValue(LineColorS),
			Color_B = GetBValue(LineColorS);
	double	Color_dR = (double)(GetRValue(LineColorE) - GetRValue(LineColorS)) / dist,
			Color_dG = (double)(GetGValue(LineColorE) - GetGValue(LineColorS)) / dist,
			Color_dB = (double)(GetBValue(LineColorE) - GetBValue(LineColorS)) / dist;

	BYTE *p_pImage = NULL;

	if(Image_BPP == 1)
	{
		p_pImage = pImage + (Image_PW * Line_P1.y) + (Image_BPP * Line_P1.x);
		p_pImage[0] = Color_B;

		Color_B = (BYTE)(Color_B + Color_dB + 0.5);

		for(i=0;i<dist;i++)
		{
			p_pImage = pImage + (Image_PW * (Line_P1.y - (int)(i * sinR))) + (Image_BPP * (Line_P1.x - (int)(i * cosR)));
			p_pImage[0] = Color_B;
			Color_B = (BYTE)(Color_B + Color_dB + 0.5);
		}

		p_pImage = pImage + (Image_PW * Line_P2.y) + (Image_BPP * Line_P2.x);
		p_pImage[0] = Color_B;
	}
	else
	{
		p_pImage = pImage + (Image_PW * Line_P1.y) + (Image_BPP * Line_P1.x);
		p_pImage[0] = Color_B;	p_pImage[1] = Color_G;	p_pImage[2] = Color_R;
		Color_R = (BYTE)(Color_R + Color_dR + 0.5);
		Color_G = (BYTE)(Color_G + Color_dG + 0.5);
		Color_B = (BYTE)(Color_B + Color_dB + 0.5);

		for(i=0;i<dist;i++)
		{
			p_pImage = pImage + (Image_PW * (Line_P1.y - (int)(i * sinR))) + (Image_BPP * (Line_P1.x - (int)(i * cosR)));
			p_pImage[0] = Color_B;	p_pImage[1] = Color_G;	p_pImage[2] = Color_R;
			Color_R = (BYTE)(Color_R + Color_dR + 0.5);
			Color_G = (BYTE)(Color_G + Color_dG + 0.5);
			Color_B = (BYTE)(Color_B + Color_dB + 0.5);
		}

		p_pImage = pImage + (Image_PW * Line_P2.y) + (Image_BPP * Line_P2.x);
		p_pImage[0] = Color_B;	p_pImage[1] = Color_G;	p_pImage[2] = Color_R;
	}
}

double CImgProcess::FindAngle(CPoint P1, CPoint P2)
{
	double angle = -1.0;

	// 計算兩點構成向量與水平的夾角
	if(((P1.x - P2.x) > 0) && ((P1.y - P2.y) > 0))			// 第1象限
	{
		angle = (atan((P1.y - P2.y) / (double)(P1.x - P2.x))) * Dre2Pi;
	}
	else if(((P1.x - P2.x) < 0) && ((P1.y - P2.y) > 0))		// 第2象限
	{
		angle = (atan((P1.y - P2.y) / (double)(P1.x - P2.x))) * Dre2Pi + 180;
	}
	else if(((P1.x - P2.x) < 0) && ((P1.y - P2.y) < 0)) 	// 第3象限
	{
		angle = (atan((P1.y - P2.y) / (double)(P1.x - P2.x))) * Dre2Pi + 180;
	}
	else if(((P1.x - P2.x) > 0) && ((P1.y - P2.y) < 0))		// 第4象限
	{
		angle = (atan((P1.y - P2.y) / (double)(P1.x - P2.x))) * Dre2Pi + 360;
	}

	if((P1.x >= P2.x) && (P1.y == P2.y))		angle = 0.0;
	else if((P1.x == P2.x) && (P1.y > P2.y))	angle = 90.0;
	else if((P1.x == P2.x) && (P1.y < P2.y))	angle = 270.0;
	else if((P1.x < P2.x) && (P1.y == P2.y))	angle = 180.0;

	return(angle);
}

int CImgProcess::FindDistance(CPoint P1, CPoint P2)
{
	int	W = (P1.x - P2.x) * (P1.x - P2.x),
		H = (P1.y - P2.y) * (P1.y - P2.y);
	return((int)ceil(sqrtf(W + H)));
}

double CImgProcess::FindDistance(double P1_x, double P1_y, double P2_x, double P2_y)
{
	double	W = (P1_x - P2_x) * (P1_x - P2_x),
			H = (P1_y - P2_y) * (P1_y - P2_y);
	return(sqrt(W + H));
}

void CImgProcess::ChamferDistanceRAW(BYTE *pImage, int Image_W, int Image_H, BYTE *pResult, BYTE Threshold, int Max_V, int Min_V)
{
	int i, j,
		value = 0,
		Image_Size = Image_H * Image_W;

	memset(pResult, 255, sizeof(BYTE) * Image_W * Image_H);

	for(i=0;i<Image_Size;i++)
		if(pImage[i] <= Threshold)
			pResult[i] = 0;

	BYTE *p_pResult = pResult + Image_W;
	for(i=1;i<Image_H-1;i++, p_pResult+=Image_W)
		for(j=1;j<Image_W-1;j++)
		{
			value = 255;

			if(value > p_pResult[j])							value = p_pResult[j];
			if(value > p_pResult[j - 1] + Min_V)				value = p_pResult[j - 1] + Min_V;
			if(value > p_pResult[-Image_W + j - 1] + Max_V)		value = p_pResult[-Image_W + j - 1] + Max_V;
			if(value > p_pResult[-Image_W + j] + Min_V)			value = p_pResult[-Image_W + j] + Min_V;
			if(value > p_pResult[-Image_W + j + 1] + Max_V)		value = p_pResult[-Image_W + j + 1] + Max_V;
			
			p_pResult[j] = value;
		}

	p_pResult = pResult + ((Image_H - 1) * Image_W);
	for(i=Image_H-1;i>1;i--, p_pResult-=Image_W)
		for(j=Image_W-1;j>1;j--)
		{
			value = 255;
			if(value > p_pResult[j])							value = p_pResult[j];
			if(value > p_pResult[j + 1] + Min_V)				value = p_pResult[j + 1] + Min_V;
			if(value > p_pResult[Image_W + j + 1] + Max_V)		value = p_pResult[Image_W + j + 1] + Max_V;
			if(value > p_pResult[Image_W + j] + Min_V)			value = p_pResult[Image_W + j] + Min_V;
			if(value > p_pResult[Image_W + j - 1] + Max_V)		value = p_pResult[Image_W + j - 1] + Max_V;

			p_pResult[j] = value;		 
		}
}

void CImgProcess::MountainBandThresholdRAW(BYTE *pImage, int Image_W, int Image_H, int &Ban1, int &Ban2, double DifferenceTHRatio, int PreSkipBan, int PreCalBan)
{
	Ban1 = 0;
	Ban2 = 255;

	int i,
		ImageSize = Image_W * Image_H,
		Count,
		PixelCount[256] = {0};
	double	Mean;

	for(i=0;i<ImageSize;i++)
		PixelCount[pImage[i]]++;

	// Find Ban1
	Mean = 0.0;
	Count = 0;
	for(i=PreSkipBan;i<PreCalBan;i++)
	{
		Mean += PixelCount[i];
		Count++;
	}
	Mean /= (double)Count;

	for(i=PreCalBan;i<256;i++)
	{
		Mean = (Mean * Count + PixelCount[i]) / (double)(++Count);

		if(fabs(PixelCount[i] - Mean) > (Mean * DifferenceTHRatio))
		{
			Ban1 = i;
			break;
		}
	}

	// Find Ban2
	Mean = 0.0;
	Count = 0;
	for(i=PreSkipBan;i<PreCalBan;i++)
	{
		Mean += PixelCount[255 - i];
		Count++;
	}
	Mean /= (double)Count;

	for(i=255-PreCalBan;i>Ban1;i--)
	{
		Mean = (Mean * Count + PixelCount[i]) / (double)(++Count);

		if(fabs(PixelCount[i] - Mean) > (Mean * DifferenceTHRatio))
		{
			Ban2 = i;
			break;
		}
	}

	for(i=0;i<ImageSize;i++)
		if(pImage[i] >= Ban2)		pImage[i] = 255;
		else if(pImage[i] <= Ban1)	pImage[i] = 0;
		else						pImage[i] = 128;
}

int CImgProcess::IsoDataThresholdRAW(BYTE *pImage, int Image_W, int Image_H)
{
	int i, Threshold_Temp,
		Threshold_V = 128,
		ImageSize = Image_W * Image_H,
		PixelCount[256] = {0};

	double	Count_1, Count_2, Mean_1, Mean_2;

	BOOL b_Process_Continue = TRUE;
	for(i=0;i<ImageSize;i++)
		PixelCount[pImage[i]]++;

	do{
		Mean_1 = 0;
		Mean_2 = 0;
		Count_1 = 0;
		Count_2 = 0;

		for(i=0;i<Threshold_V;i++)
		{
			Count_1 += PixelCount[i];
			Mean_1 += (PixelCount[i] * i);
		}
		for(;i<256;i++)
		{
			Count_2 += PixelCount[i];
			Mean_2 += (PixelCount[i] * i);
		}

		Mean_1 /= Count_1;
		Mean_2 /= Count_2;

		Threshold_Temp = (int)((Mean_1 + Mean_2) / 2);
		if(abs(Threshold_Temp - Threshold_V) > 1)
			Threshold_V = Threshold_Temp;
		else
			b_Process_Continue = FALSE;
	}while(b_Process_Continue);

	for(i=0;i<ImageSize;i++)
		if(pImage[i] >= Threshold_V)
			pImage[i] = 255;
		else
			pImage[i] = 0;
	return(Threshold_V);
}

void CImgProcess::Logic_AND(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize)
{
	int i;
	for(i=0;i<DataSize;i++)
		pResult[i] = Min(pSource[i], pRef[i]);
}

void CImgProcess::Logic_OR(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize)
{
	int i;
	for(i=0;i<DataSize;i++)
		pResult[i] = Max(pSource[i], pRef[i]);
}

void CImgProcess::Logic_XOR(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize, int Tolorance)
{
	int i;
	for(i=0;i<DataSize;i++)
		if(abs(pSource[i] - pRef[i]) < Tolorance)
			pResult[i] = 255;
		else
			pResult[i] = 0;
}

void CImgProcess::Logic_SUB(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize, int Tolorance)
{
	int i;
	for(i=0;i<DataSize;i++)
		if((pSource[i] + Tolorance) >= pRef[i])
			pResult[i] = 255;
		else
			pResult[i] = 0;
}

void CImgProcess::KuwaharaFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int EachRegion_W, int EachRegion_H, int BPP)
{
	if((EachRegion_H < 2) || (EachRegion_W < 2))
		return;

	int		i, j, k, m, n, o, p, q,
			Process_Skip_W = EachRegion_W - 1,
			Process_Skip_H = EachRegion_H - 1,
			ImagePW = (BPP * n_Process_W + 3) >> 2 << 2,
			Process_W = n_Process_W - Process_Skip_W,
			TotalMaskCount = EachRegion_H * EachRegion_W;

	double	Temp, Mean[4], Variance[4];

	int *ProcessTable = new int[4 * TotalMaskCount];

	ProcessTable[0] = - (BPP * Process_Skip_W) - (ImagePW * Process_Skip_H);
	ProcessTable[TotalMaskCount] = - (ImagePW * Process_Skip_H);
	ProcessTable[TotalMaskCount * 2] = - (BPP * Process_Skip_W);
	ProcessTable[TotalMaskCount * 3] = 0;

	for(m=0, n=0;m<4;m++, n+=TotalMaskCount)
	{
		k = 0;
		for(i=0;i<EachRegion_H;i++)
			for(j=0;j<EachRegion_W;j++)
			{
				ProcessTable[n + k] = ProcessTable[n] + (i * BPP) + (j * ImagePW);
				k++;
			}
	}

	memset(p_Result_Image, 0, sizeof(BYTE) * ImagePW * n_Process_H);
	BYTE *p_p_Process_Image = p_Process_Image + (ImagePW * Process_Skip_H);
	BYTE *p_p_Result_Image = p_Result_Image + (ImagePW * Process_Skip_H);

	if(BPP == 1)
	{
		for(i=Process_Skip_H;i<n_Process_H-Process_Skip_H;i++, p_p_Process_Image+=ImagePW, p_p_Result_Image+=ImagePW)
			for(j=Process_Skip_W;j<Process_W;j++)
			{
				memset(Mean, 0, sizeof(double) * 4);
				memset(Variance, 0, sizeof(double) * 4);
				for(m=0, q=0;m<4;m++, q+=TotalMaskCount)
				{
					for(n=0, p=q;n<TotalMaskCount;n++, p++)
						Mean[m] += p_p_Process_Image[j + ProcessTable[p]];
					Mean[m] /= TotalMaskCount;
				}
				for(m=0, q=0;m<4;m++, q+=TotalMaskCount)
				{
					for(n=0, p=q;n<TotalMaskCount;n++, p++)
					{
						Temp = p_p_Process_Image[j + ProcessTable[p]] - Mean[m];
						Variance[m] += (Temp * Temp);
					}
					Variance[m] /= TotalMaskCount;
				}

				p_p_Result_Image[j] = (BYTE)Mean[Min(Variance, 4)];
			}
	}
	else
	{
		for(i=Process_Skip_H;i<n_Process_H-Process_Skip_H;i++, p_p_Process_Image+=ImagePW, p_p_Result_Image+=ImagePW)
			for(j=Process_Skip_W, k=Process_Skip_W*BPP;j<Process_W;j++, k+=BPP)
				for(o=0;o<3;o++)
				{
					memset(Mean, 0, sizeof(double) * 4);
					memset(Variance, 0, sizeof(double) * 4);

					for(m=0, q=0;m<4;m++, q+=TotalMaskCount)
					{
						for(n=0, p=q;n<TotalMaskCount;n++, p++)
							Mean[m] += p_p_Process_Image[k + o + ProcessTable[p]];
						Mean[m] /= TotalMaskCount;
					}

					for(m=0, q=0;m<4;m++, q+=TotalMaskCount)
					{
						for(n=0, p=q;n<TotalMaskCount;n++, p++)
						{
							Temp = p_p_Process_Image[k + o + ProcessTable[p]] - Mean[m];
							Variance[m] += (Temp * Temp);
						}
						Variance[m] /= TotalMaskCount;
					}

					p_p_Result_Image[k + o] = (BYTE)Mean[Min(Variance, 4)];
				}
	}

	delete [] ProcessTable;
	ProcessTable = NULL;
}

void CImgProcess::IsoDataThreshold(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B)
{
	int i, j, k,
		Threshold_Temp,
		Count_1, Count_2,
		Threshold_Value[3],
		Image_PW = (Image_BPP * Image_W + 3) >> 2 << 2;
	Threshold_Value[0] = Threshold_Value[1] = Threshold_Value[2] = 128;
	BYTE *p_pImage = NULL;

	int PixelCount[3][256] = {0};

	double	Mean_1, Mean_2;
	BOOL b_Process_Continue;

	if(Image_BPP == 1)
	{
		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Image_W;j++)
				PixelCount[0][p_pImage[j]]++;
		b_Process_Continue = TRUE;

		do{
			Mean_1 = Mean_2 = 0.0;
			Count_1 = Count_2 = 0;
			for(i=0;i<Threshold_Value[0];i++)
			{
				Count_1 += PixelCount[0][i];
				Mean_1 += (PixelCount[0][i] * i);
			}
			for(;i<256;i++)
			{
				Count_2 += PixelCount[0][i];
				Mean_2 += (PixelCount[0][i] * i);
			}

			Mean_1 /= (double)Count_1;
			Mean_2 /= (double)Count_2;
			Threshold_Temp = (int)((Mean_1 + Mean_2) / 2);

			if(abs(Threshold_Temp - Threshold_Value[0]) > 1)
				Threshold_Value[0] = Threshold_Temp;
			else
				b_Process_Continue = FALSE;
		}while(b_Process_Continue);

		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Image_W;j++)
				if(p_pImage[j] >= Threshold_Value[0])
					p_pImage[j] = 255;
				else
					p_pImage[j] = 0;

		CutoffThreshold_B = CutoffThreshold_G = CutoffThreshold_R = Threshold_Value[0];
	}
	else
	{
		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
			{
				PixelCount[0][p_pImage[k    ]]++;
				PixelCount[1][p_pImage[k + 1]]++;
				PixelCount[2][p_pImage[k + 2]]++;
			}

		for(k=0;k<3;k++)
		{
			b_Process_Continue = TRUE;
			do{
				Mean_1 = Mean_2 = 0.0;
				Count_1 = Count_2 = 0;
				for(i=0;i<Threshold_Value[k];i++)
				{
					Count_1 += PixelCount[k][i];
					Mean_1 += (PixelCount[k][i] * i);
				}
				for(;i<256;i++)
				{
					Count_2 += PixelCount[k][i];
					Mean_2 += (PixelCount[k][i] * i);
				}

				Mean_1 /= (double)Count_1;
				Mean_2 /= (double)Count_2;
				Threshold_Temp = (int)((Mean_1 + Mean_2) / 2);

				if(abs(Threshold_Temp - Threshold_Value[k]) > 1)
					Threshold_Value[k] = Threshold_Temp;
				else
					b_Process_Continue = FALSE;
			}while(b_Process_Continue);
		}

		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
			{
				if(p_pImage[k    ] >= Threshold_Value[0])	p_pImage[k    ] = 255;
				else										p_pImage[k    ] = 0;
				if(p_pImage[k + 1] >= Threshold_Value[1])	p_pImage[k + 1] = 255;
				else										p_pImage[k + 1] = 0;
				if(p_pImage[k + 2] >= Threshold_Value[2])	p_pImage[k + 2] = 255;
				else										p_pImage[k + 2] = 0;
			}

		CutoffThreshold_R = Threshold_Value[2];
		CutoffThreshold_G = Threshold_Value[1];
		CutoffThreshold_B = Threshold_Value[0];
	}
}

int CImgProcess::MaximumEntropyThresholdRAW(BYTE *pImage, int Image_W, int Image_H)
{
	int i, j,
		Threshold_Value = 0,
		ImageSize = Image_W * Image_H,
		PixelCount[256] = {0};

	double	NormalizedHistogram[256], NormalizedCDF[256],
			h_Low[256], h_High[256];
	double BW_Temp, pTW, Temp, Max_Entropy;

	for(i=0;i<ImageSize;i++)
		PixelCount[pImage[i]]++;

	for(i=0;i<256;i++)
		NormalizedHistogram[i] = (double)((double)PixelCount[i] / (double)ImageSize);

	NormalizedCDF[0] = NormalizedHistogram[0];
	for(i=1;i<256;i++)
		NormalizedCDF[i] = NormalizedCDF[i - 1] + NormalizedHistogram[i];

	// Entropy for black and white parts of the histogram
	for(j=0;j<256;j++)
	{
		// Low Bound Entropy
		if(NormalizedCDF[j] > 0.0)
		{
			BW_Temp = 0;
			for(i=0;i<=j;i++)
				if(NormalizedHistogram[i] > 0.0)
				{
					Temp = NormalizedHistogram[i] / NormalizedCDF[j];
					BW_Temp -= (Temp * log(Temp));
				}
			h_Low[j] = BW_Temp;
		}
		else
			h_Low[j] = 0;

		// High Bound Entropy
		pTW = 1.0 - NormalizedCDF[j];
		if(pTW > 0.0)
		{
			BW_Temp = 0;
			for(i=j+1;i<256;i++)
				if(NormalizedHistogram[i] > 0.0)
				{
					Temp = NormalizedHistogram[i] / pTW;
					BW_Temp -= (Temp * log(Temp));
				}
			h_High[j] = BW_Temp;
		}
		else
			h_High[j] = 0;
	}

	// Find Histogram Index with Maximum Entropy
	Threshold_Value = 0;
	Max_Entropy = h_Low[0] + h_High[0];
	for(i=1;i<256;i++)
	{
		Temp = h_Low[i] + h_High[i];
		if(Temp > Max_Entropy)
		{
			Max_Entropy = Temp;
			Threshold_Value = i;
		}
	}

	for(i=0;i<ImageSize;i++)
		if(pImage[i] >= Threshold_Value)
			pImage[i] = 255;
		else
			pImage[i] = 0;

	return(Threshold_Value);
}

void CImgProcess::MaximumEntropyThreshold(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B)
{
	int i, j, k,
		Threshold_Value[3] = {0},

		PixelCount[3][256] = {0},
		PixelPerChannel = Image_H * Image_W,
		Image_PW = (Image_BPP * Image_W + 3) >> 2 << 2;

	double	NormalizedHistogram[256], NormalizedCDF[256], h_Low[256], h_High[256];
	double	BW_Temp, pTW, Temp, Max_Entropy;

	BYTE	*p_pImage = NULL;

	if(Image_BPP == 1)
	{
		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Image_W;j++)
				PixelCount[0][p_pImage[j]]++;

		for(i=0;i<256;i++)
			NormalizedHistogram[i] = (double)((double)PixelCount[0][i] / (double)PixelPerChannel);

		NormalizedCDF[0] = NormalizedHistogram[0];
		for(i=1;i<256;i++)
			NormalizedCDF[i] = NormalizedCDF[i - 1] + NormalizedHistogram[i];

		// Entropy for black and white parts of the histogram
		for(j=0;j<256;j++)
		{
			// Low Bound Entropy
			if(NormalizedCDF[j] > 0.0)
			{
				BW_Temp = 0;
				for(i=0;i<=j;i++)
					if(NormalizedHistogram[i] > 0.0)
					{
						Temp = NormalizedHistogram[i] / NormalizedCDF[j];
						BW_Temp -= (Temp * log(Temp));
					}
				h_Low[j] = BW_Temp;
			}
			else
				h_Low[j] = 0;

			// High Bound Entropy
			pTW = 1.0 - NormalizedCDF[j];
			if(pTW > 0.0)
			{
				BW_Temp = 0;
				for(i=j+1;i<256;i++)
					if(NormalizedHistogram[i] > 0.0)
					{
						Temp = NormalizedHistogram[i] / pTW;
						BW_Temp -= (Temp * log(Temp));
					}
				h_High[j] = BW_Temp;
			}
			else
				h_High[j] = 0;
		}

		// Find Histogram Index with Maximum Entropy
		Threshold_Value[0] = 0;
		Max_Entropy = h_Low[0] + h_High[0];
		for(i=1;i<256;i++)
		{
			Temp = h_Low[i] + h_High[i];
			if(Temp > Max_Entropy)
			{
				Max_Entropy = Temp;
				Threshold_Value[0] = i;
			}
		}

		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Image_W;j++)
				if(p_pImage[j] > Threshold_Value[0])
					p_pImage[j] = 255;
				else
					p_pImage[j] = 0;

		CutoffThreshold_R = CutoffThreshold_G = CutoffThreshold_B = Threshold_Value[0];
	}
	else
	{
		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
			{
				PixelCount[0][p_pImage[k    ]]++;
				PixelCount[1][p_pImage[k + 1]]++;
				PixelCount[2][p_pImage[k + 2]]++;
			}

		for(k=0;k<3;k++)
		{
			for(i=0;i<256;i++)
				NormalizedHistogram[i] = (double)((double)PixelCount[k][i] / (double)PixelPerChannel);

			NormalizedCDF[0] = NormalizedHistogram[0];
			for(i=1;i<256;i++)
				NormalizedCDF[i] = NormalizedCDF[i - 1] + NormalizedHistogram[i];

			// Entropy for black and white parts of the histogram
			for(j=0;j<256;j++)
			{
				// Low Bound Entropy
				if(NormalizedCDF[j] > 0.0)
				{
					BW_Temp = 0;
					for(i=0;i<=j;i++)
						if(NormalizedHistogram[i] > 0.0)
						{
							Temp = NormalizedHistogram[i] / NormalizedCDF[j];
							BW_Temp -= (Temp * log(Temp));
						}
					h_Low[j] = BW_Temp;
				}
				else
					h_Low[j] = 0;

				// High Bound Entropy
				pTW = 1.0 - NormalizedCDF[j];
				if(pTW > 0.0)
				{
					BW_Temp = 0;
					for(i=j+1;i<256;i++)
						if(NormalizedHistogram[i] > 0.0)
						{
							Temp = NormalizedHistogram[i] / pTW;
							BW_Temp -= (Temp * log(Temp));
						}
					h_High[j] = BW_Temp;
				}
				else
					h_High[j] = 0;
			}

			// Find Histogram Index with Maximum Entropy
			Threshold_Value[k] = 0;
			Max_Entropy = h_Low[0] + h_High[0];
			for(i=1;i<256;i++)
			{
				Temp = h_Low[i] + h_High[i];
				if(Temp > Max_Entropy)
				{
					Max_Entropy = Temp;
					Threshold_Value[k] = i;
				}
			}
		}

		p_pImage = pImage;
		for(i=0;i<Image_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Image_W;j++, k+=Image_BPP)
			{
				if(p_pImage[k    ] >= Threshold_Value[0])	p_pImage[k    ] = 255;
				else										p_pImage[k    ] = 0;
				if(p_pImage[k + 1] >= Threshold_Value[1])	p_pImage[k + 1] = 255;
				else										p_pImage[k + 1] = 0;
				if(p_pImage[k + 2] >= Threshold_Value[2])	p_pImage[k + 2] = 255;
				else										p_pImage[k + 2] = 0;
			}

		CutoffThreshold_R = Threshold_Value[2];
		CutoffThreshold_G = Threshold_Value[1];
		CutoffThreshold_B = Threshold_Value[0];
	}
}

void CImgProcess::IsoDataThresholdROI(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B)
{
	int i, j, k,
		Threshold_Temp,
		Count_1, Count_2,
		Threshold_Value[3],
		Process_W = DR.x - UL.x + 1,
		Process_H = DR.y - UL.y + 1,
		Image_PW = (Image_W * Image_BPP + 3) >> 2 << 2;

	Threshold_Value[0] = Threshold_Value[1] = Threshold_Value[2] = 128;
	BYTE *p_pImage = NULL;

	int PixelCount[3][256] = {0};

	double	Mean_1, Mean_2;
	BOOL b_Process_Continue;

	if(Image_BPP == 1)
	{
		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Process_W;j++)
				PixelCount[0][p_pImage[j]]++;
		b_Process_Continue = TRUE;

		do{
			Mean_1 = Mean_2 = 0.0;
			Count_1 = Count_2 = 0;
			for(i=0;i<Threshold_Value[0];i++)
			{
				Count_1 += PixelCount[0][i];
				Mean_1 += (PixelCount[0][i] * i);
			}
			for(;i<256;i++)
			{
				Count_2 += PixelCount[0][i];
				Mean_2 += (PixelCount[0][i] * i);
			}

			Mean_1 /= (double)Count_1;
			Mean_2 /= (double)Count_2;
			Threshold_Temp = (int)((Mean_1 + Mean_2) / 2);

			if(abs(Threshold_Temp - Threshold_Value[0]) > 1)
				Threshold_Value[0] = Threshold_Temp;
			else
				b_Process_Continue = FALSE;
		}while(b_Process_Continue);

		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Process_W;j++)
				if(p_pImage[j] >= Threshold_Value[0])
					p_pImage[j] = 255;
				else
					p_pImage[j] = 0;

		CutoffThreshold_B = CutoffThreshold_G = CutoffThreshold_R = Threshold_Value[0];
	}
	else
	{
		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Process_W;j++, k+=Image_BPP)
			{
				PixelCount[0][p_pImage[k    ]]++;
				PixelCount[1][p_pImage[k + 1]]++;
				PixelCount[2][p_pImage[k + 2]]++;
			}

		for(k=0;k<3;k++)
		{
			b_Process_Continue = TRUE;
			do{
				Mean_1 = Mean_2 = 0.0;
				Count_1 = Count_2 = 0;
				for(i=0;i<Threshold_Value[k];i++)
				{
					Count_1 += PixelCount[k][i];
					Mean_1 += (PixelCount[k][i] * i);
				}
				for(;i<256;i++)
				{
					Count_2 += PixelCount[k][i];
					Mean_2 += (PixelCount[k][i] * i);
				}

				Mean_1 /= (double)Count_1;
				Mean_2 /= (double)Count_2;
				Threshold_Temp = (int)((Mean_1 + Mean_2) / 2);

				if(abs(Threshold_Temp - Threshold_Value[k]) > 1)
					Threshold_Value[k] = Threshold_Temp;
				else
					b_Process_Continue = FALSE;
			}while(b_Process_Continue);
		}

		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Process_W;j++, k+=Image_BPP)
			{
				if(p_pImage[k    ] >= Threshold_Value[0])	p_pImage[k    ] = 255;
				else										p_pImage[k    ] = 0;
				if(p_pImage[k + 1] >= Threshold_Value[1])	p_pImage[k + 1] = 255;
				else										p_pImage[k + 1] = 0;
				if(p_pImage[k + 2] >= Threshold_Value[2])	p_pImage[k + 2] = 255;
				else										p_pImage[k + 2] = 0;
			}

		CutoffThreshold_R = Threshold_Value[2];
		CutoffThreshold_G = Threshold_Value[1];
		CutoffThreshold_B = Threshold_Value[0];
	}
}

void CImgProcess::MaximumEntropyThresholdROI(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B)
{
	int i, j, k,
		Threshold_Value[3] = {0},
		PixelCount[3][256] = {0},
		Process_W = DR.x - UL.x + 1,
		Process_H = DR.y - UL.y + 1,
		PixelPerChannel = Process_W * Process_H,
		Image_PW = (Image_BPP * Image_W + 3) >> 2 << 2;

	double	NormalizedHistogram[256], NormalizedCDF[256], h_Low[256], h_High[256];
	double	BW_Temp, pTW, Temp, Max_Entropy;

	BYTE	*p_pImage = NULL;

	if(Image_BPP == 1)
	{
		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Process_W;j++)
				PixelCount[0][p_pImage[j]]++;

		for(i=0;i<256;i++)
			NormalizedHistogram[i] = (double)((double)PixelCount[0][i] / (double)PixelPerChannel);

		NormalizedCDF[0] = NormalizedHistogram[0];
		for(i=1;i<256;i++)
			NormalizedCDF[i] = NormalizedCDF[i - 1] + NormalizedHistogram[i];

		// Entropy for black and white parts of the histogram
		for(j=0;j<256;j++)
		{
			// Low Bound Entropy
			if(NormalizedCDF[j] > 0.0)
			{
				BW_Temp = 0;
				for(i=0;i<=j;i++)
					if(NormalizedHistogram[i] > 0.0)
					{
						Temp = NormalizedHistogram[i] / NormalizedCDF[j];
						BW_Temp -= (Temp * log(Temp));
					}
				h_Low[j] = BW_Temp;
			}
			else
				h_Low[j] = 0;

			// High Bound Entropy
			pTW = 1.0 - NormalizedCDF[j];
			if(pTW > 0.0)
			{
				BW_Temp = 0;
				for(i=j+1;i<256;i++)
					if(NormalizedHistogram[i] > 0.0)
					{
						Temp = NormalizedHistogram[i] / pTW;
						BW_Temp -= (Temp * log(Temp));
					}
				h_High[j] = BW_Temp;
			}
			else
				h_High[j] = 0;
		}

		// Find Histogram Index with Maximum Entropy
		Threshold_Value[0] = 0;
		Max_Entropy = h_Low[0] + h_High[0];
		for(i=1;i<256;i++)
		{
			Temp = h_Low[i] + h_High[i];
			if(Temp > Max_Entropy)
			{
				Max_Entropy = Temp;
				Threshold_Value[0] = i;
			}
		}

		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0;j<Process_W;j++)
				if(p_pImage[j] > Threshold_Value[0])
					p_pImage[j] = 255;
				else
					p_pImage[j] = 0;

		CutoffThreshold_R = CutoffThreshold_G = CutoffThreshold_B = Threshold_Value[0];
	}
	else
	{
		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Process_W;j++, k+=Image_BPP)
			{
				PixelCount[0][p_pImage[k    ]]++;
				PixelCount[1][p_pImage[k + 1]]++;
				PixelCount[2][p_pImage[k + 2]]++;
			}

		for(k=0;k<3;k++)
		{
			for(i=0;i<256;i++)
				NormalizedHistogram[i] = (double)((double)PixelCount[k][i] / (double)PixelPerChannel);

			NormalizedCDF[0] = NormalizedHistogram[0];
			for(i=1;i<256;i++)
				NormalizedCDF[i] = NormalizedCDF[i - 1] + NormalizedHistogram[i];

			// Entropy for black and white parts of the histogram
			for(j=0;j<256;j++)
			{
				// Low Bound Entropy
				if(NormalizedCDF[j] > 0.0)
				{
					BW_Temp = 0;
					for(i=0;i<=j;i++)
						if(NormalizedHistogram[i] > 0.0)
						{
							Temp = NormalizedHistogram[i] / NormalizedCDF[j];
							BW_Temp -= (Temp * log(Temp));
						}
					h_Low[j] = BW_Temp;
				}
				else
					h_Low[j] = 0;

				// High Bound Entropy
				pTW = 1.0 - NormalizedCDF[j];
				if(pTW > 0.0)
				{
					BW_Temp = 0;
					for(i=j+1;i<256;i++)
						if(NormalizedHistogram[i] > 0.0)
						{
							Temp = NormalizedHistogram[i] / pTW;
							BW_Temp -= (Temp * log(Temp));
						}
					h_High[j] = BW_Temp;
				}
				else
					h_High[j] = 0;
			}

			// Find Histogram Index with Maximum Entropy
			Threshold_Value[k] = 0;
			Max_Entropy = h_Low[0] + h_High[0];
			for(i=1;i<256;i++)
			{
				Temp = h_Low[i] + h_High[i];
				if(Temp > Max_Entropy)
				{
					Max_Entropy = Temp;
					Threshold_Value[k] = i;
				}
			}
		}

		p_pImage = pImage + (UL.y * Image_PW) + (UL.x * Image_BPP);
		for(i=0;i<Process_H;i++, p_pImage+=Image_PW)
			for(j=0, k=0;j<Process_W;j++, k+=Image_BPP)
			{
				if(p_pImage[k    ] >= Threshold_Value[0])	p_pImage[k    ] = 255;
				else										p_pImage[k    ] = 0;
				if(p_pImage[k + 1] >= Threshold_Value[1])	p_pImage[k + 1] = 255;
				else										p_pImage[k + 1] = 0;
				if(p_pImage[k + 2] >= Threshold_Value[2])	p_pImage[k + 2] = 255;
				else										p_pImage[k + 2] = 0;
			}

		CutoffThreshold_R = Threshold_Value[2];
		CutoffThreshold_G = Threshold_Value[1];
		CutoffThreshold_B = Threshold_Value[0];
	}
}

int CImgProcess::IsoDataThresholdROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR)
{
	int i, j, Threshold_Temp,
		Threshold_V = 128,
		Process_W = DR.x - UL.x + 1,
		Process_H = DR.y - UL.y + 1,
		PixelCount[256] = {0};

	double	Count_1, Count_2, Mean_1, Mean_2;
	BOOL b_Process_Continue = TRUE;

	BYTE	*p_pImage = NULL;
	p_pImage = pImage + (UL.y * Image_W) + UL.x;
	for(i=0;i<Process_H;i++, p_pImage+=Image_W)
		for(j=0;j<Process_W;j++)
			PixelCount[p_pImage[j]]++;

	do{
		Mean_1 = 0;
		Mean_2 = 0;
		Count_1 = 0;
		Count_2 = 0;

		for(i=0;i<Threshold_V;i++)
		{
			Count_1 += PixelCount[i];
			Mean_1 += (PixelCount[i] * i);
		}
		for(;i<256;i++)
		{
			Count_2 += PixelCount[i];
			Mean_2 += (PixelCount[i] * i);
		}

		Mean_1 /= Count_1;
		Mean_2 /= Count_2;

		Threshold_Temp = (int)((Mean_1 + Mean_2) / 2);
		if(abs(Threshold_Temp - Threshold_V) > 1)
			Threshold_V = Threshold_Temp;
		else
			b_Process_Continue = FALSE;
	}while(b_Process_Continue);

	p_pImage = pImage + (UL.y * Image_W) + UL.x;
	for(i=0;i<Process_H;i++, p_pImage+=Image_W)
		for(j=0;j<Process_W;j++)
			if(p_pImage[j] >= Threshold_V)
				p_pImage[j] = 255;
			else
				p_pImage[j] = 0;

	return(Threshold_V);
}

int CImgProcess::MaximumEntropyThresholdROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR)
{
	int i, j,
		Threshold_Value = 0,
		Process_W = DR.x - UL.x + 1,
		Process_H = DR.y - UL.y + 1,
		ImageSize = Process_W * Process_H,
		PixelCount[256] = {0};

	double	NormalizedHistogram[256], NormalizedCDF[256],
			h_Low[256], h_High[256];
	double BW_Temp, pTW, Temp, Max_Entropy;

	BYTE	*p_pImage = NULL;
	p_pImage = pImage + (UL.y * Image_W) + UL.x;
	for(i=0;i<Process_H;i++, p_pImage+=Image_W)
		for(j=0;j<Process_W;j++)
			PixelCount[p_pImage[j]]++;

	for(i=0;i<256;i++)
		NormalizedHistogram[i] = (double)((double)PixelCount[i] / (double)ImageSize);

	NormalizedCDF[0] = NormalizedHistogram[0];
	for(i=1;i<256;i++)
		NormalizedCDF[i] = NormalizedCDF[i - 1] + NormalizedHistogram[i];

	// Entropy for black and white parts of the histogram
	for(j=0;j<256;j++)
	{
		// Low Bound Entropy
		if(NormalizedCDF[j] > 0.0)
		{
			BW_Temp = 0;
			for(i=0;i<=j;i++)
				if(NormalizedHistogram[i] > 0.0)
				{
					Temp = NormalizedHistogram[i] / NormalizedCDF[j];
					BW_Temp -= (Temp * log(Temp));
				}
			h_Low[j] = BW_Temp;
		}
		else
			h_Low[j] = 0;

		// High Bound Entropy
		pTW = 1.0 - NormalizedCDF[j];
		if(pTW > 0.0)
		{
			BW_Temp = 0;
			for(i=j+1;i<256;i++)
				if(NormalizedHistogram[i] > 0.0)
				{
					Temp = NormalizedHistogram[i] / pTW;
					BW_Temp -= (Temp * log(Temp));
				}
			h_High[j] = BW_Temp;
		}
		else
			h_High[j] = 0;
	}

	// Find Histogram Index with Maximum Entropy
	Threshold_Value = 0;
	Max_Entropy = h_Low[0] + h_High[0];
	for(i=1;i<256;i++)
	{
		Temp = h_Low[i] + h_High[i];
		if(Temp > Max_Entropy)
		{
			Max_Entropy = Temp;
			Threshold_Value = i;
		}
	}

	p_pImage = pImage + (UL.y * Image_W) + UL.x;
	for(i=0;i<Process_H;i++, p_pImage+=Image_W)
		for(j=0;j<Process_W;j++)
			if(p_pImage[j] >= Threshold_Value)
				p_pImage[j] = 255;
			else
				p_pImage[j] = 0;
	return(Threshold_Value);
}

int CImgProcess::MinimumResidueThresholdRAW(BYTE *pImage, int Image_W, int Image_H)
{
	int i, j,
		Threshold_Value = 0,
		ImageSize = Image_W * Image_H,
		PixelCount[256] = {0};

	double	NormalizedHistogram[256],
			h_Low[256], h_High[256];
	double Mean_Temp, Temp, Min_Residue;

	for(i=0;i<ImageSize;i++)
		PixelCount[pImage[i]]++;

	for(i=0;i<256;i++)
		NormalizedHistogram[i] = (double)((double)PixelCount[i] / (double)ImageSize);

	// Entropy for black and white parts of the histogram
	for(j=0;j<256;j++)
	{
		Mean_Temp = 0.0;
		Temp = 0.0;

		for(i=0;i<=j;i++)
		{
			Mean_Temp += (double(i * i) * NormalizedHistogram[i]);
			Temp += NormalizedHistogram[i];
		}

		if(Temp > 0)
			h_Low[j] = (Mean_Temp) / Temp;
		else
			h_Low[j] = 0.0;

		Mean_Temp = 0.0;
		Temp = 0.0;
		for(i=j+1;i<256;i++)
		{
			Mean_Temp += (double((255 - i) * (255 - i)) * NormalizedHistogram[i]);
			Temp += NormalizedHistogram[i];
		}

		if(Temp > 0)
			h_High[j] = (Mean_Temp) / Temp;
		else
			h_High[j] = 0.0;
	}

	// Find Residue Index with Minimum Residue
	Threshold_Value = 0;
	Min_Residue = h_Low[0] + h_High[0];
	for(i=1;i<256;i++)
	{
		Temp = h_Low[i] + h_High[i];
		if(Temp < Min_Residue)
		{
			Min_Residue = Temp;
			Threshold_Value = i;
		}
	}

	for(i=0;i<ImageSize;i++)
		if(pImage[i] >= Threshold_Value)
			pImage[i] = 255;
		else
			pImage[i] = 0;

	return(Threshold_Value);

	/*
	int i, j,
		Threshold_Value = 0,
		ImageSize = Image_W * Image_H,
		PixelCount[256] = {0};

	double	NormalizedHistogram[256],
			h_Low[256], h_High[256];
	double Mean_Temp, Variance_Temp, Temp, Min_Residue;

	for(i=0;i<ImageSize;i++)
		PixelCount[pImage[i]]++;

	for(i=0;i<256;i++)
		NormalizedHistogram[i] = (double)((double)PixelCount[i] / (double)ImageSize);

	// Entropy for black and white parts of the histogram
	for(j=0;j<256;j++)
	{
		Mean_Temp = 0.0;
		Temp = 0.0;
		for(i=0;i<=j;i++)
		{
			Mean_Temp += (i * NormalizedHistogram[i]);
			Temp += NormalizedHistogram[i];
		}

		if(Temp > 0)
		{
			Mean_Temp /= Temp;
			Variance_Temp = 0.0;

			for(i=0;i<=j;i++)
				Variance_Temp += fabs(((double)i - Mean_Temp) * ((double)i - Mean_Temp) * NormalizedHistogram[i]);

			h_Low[j] = Variance_Temp / Temp;
		}
		else
			h_Low[j] = 0.0;


		Mean_Temp = 0.0;
		Temp = 0.0;
		for(i=j+1;i<256;i++)
		{
			Mean_Temp += (i * NormalizedHistogram[i]);
			Temp += NormalizedHistogram[i];
		}

		if(Temp > 0)
		{
			Mean_Temp /= Temp;
			Variance_Temp = 0.0;

			for(i=j+1;i<256;i++)
				Variance_Temp += fabs(((double)i - Mean_Temp) * ((double)i - Mean_Temp) * NormalizedHistogram[i]);

			h_High[j] = Variance_Temp / Temp;
		}
		else
			h_High[j] = 0.0;
	}

	// Find Residue Index with Minimum Residue
	Threshold_Value = 0;
	Min_Residue = h_Low[0] + h_High[0];
	for(i=1;i<256;i++)
	{
		Temp = h_Low[i] + h_High[i];
		if(Temp < Min_Residue)
		{
			Min_Residue = Temp;
			Threshold_Value = i;
		}
	}

	for(i=0;i<ImageSize;i++)
		if(pImage[i] >= Threshold_Value)
			pImage[i] = 255;
		else
			pImage[i] = 0;

	return(Threshold_Value);*/
}

int CImgProcess::ConnectComponent_BinaryInput(int *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, int Threshold)
{
	int	i, j, k,
		index = 1,
		offset, boundary, x_start, x_end, y_start, y_end;
	
	int a, b, c, d,
		*p_BufferImage, *p_GrayImage;

	if(!GrayImage || !Region_list)
		return(-1);

	int	*BufferImage = new int[Image_W * Image_H];
	int	*parent = new int[Image_W * Image_H];
	int	*Counter = NULL;

	// first pass
	boundary = 1;
	offset = boundary * Image_W;

	p_BufferImage = BufferImage + Image_W;
	p_GrayImage = GrayImage + Image_W;

	x_start = boundary;
	x_end = Image_W - boundary;
	y_start = boundary;
	y_end = Image_H - boundary;

	memset(BufferImage, 0, sizeof(int) * Image_W * Image_H);
	memset(parent, 0, sizeof(int) * Image_W * Image_H);

	for(i=y_start;i<y_end;i++, p_GrayImage+=Image_W, p_BufferImage+=Image_W)
		for(j=x_start;j<x_end;j++)
			if(p_GrayImage[j] > 0)
			{
				a = p_BufferImage[j - Image_W - 1];
				b = p_BufferImage[j - Image_W];
				c = p_BufferImage[j - Image_W + 1];
				d = p_BufferImage[j - 1];

				if(a)
				{
					p_BufferImage[j] = a;
					if(b && a != b) Union(a, b, parent);
					if(c && a != c) Union(a, c, parent);
					if(d && a != d) Union(a, d, parent);
				}
				else if(b)
				{
					p_BufferImage[j] = b;
					if(c && b != c) Union(b, c, parent);
					if(d && b != d) Union(b, d, parent);
				}
				else if(c)
				{
					p_BufferImage[j] = c;
					if(d && c != d) Union(c, d, parent);
				}
				else if(d != 0)
					p_BufferImage[j] = d;
				else	// new label
				{
					p_BufferImage[j] = index;
					parent[index] = 0;
					index++;
				}
			}

	// Processing equivalent class
	for(i=1;i<index;i++)
		if(parent[i])		// find parents
		{
			j = i;
			while(parent[j])
				j = parent[j];
			parent[i] = j;
		}

	k = 1;
	for(i=1;i<index;i++)
		if(!parent[i])
		{
			parent[i] = k;
			k++;
		}
		else
		{
			j = parent[i];
			parent[i] = parent[j];
		}
	index = k;

	// Second Pass
	if(index == 1)
	{
		delete [] parent;
		delete [] BufferImage;
		return 0;
	}

	Counter = new int[index];

	// relabeling
	for(i=0;i<index;i++)
		Counter[i] = 0;

	p_BufferImage = BufferImage + Image_W;
	for(i=y_start;i<y_end;i++, p_BufferImage+=Image_W)
		for(j=x_start;j<x_end;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				k = parent[a];
				Counter[k]++;
				p_BufferImage[j] = k;
			}

	k = 0;
	for(i=0;i<index;i++)
		if(Counter[i] > Threshold)
		{
			Region_list[k].bottom = 0;
			Region_list[k].top = Image_H;
			Region_list[k].c = 0;
			Region_list[k].right = 0;
			Region_list[k].cx = 0;
			Region_list[k].cy = 0;
			Region_list[k].left = Image_W;
			Region_list[k].label = k + 1;
			parent[i] = k + 1;
			k++;
		}
		else
			parent[i] = 0;

	p_BufferImage = BufferImage;
	p_GrayImage = GrayImage;
	index = k;

	for(i=0;i<Image_H;i++, p_BufferImage+=Image_W, p_GrayImage+=Image_W)
		for(j=0;j<Image_W;j++)
			if(p_BufferImage[j] > 0)
			{
				a = p_BufferImage[j];
				if(Counter[a] > Threshold)
				{
					k = parent[a] - 1;
					Region_list[k].c += 1;
					Region_list[k].cx += j;
					Region_list[k].cy += i;
					
					if(Region_list[k].top > i)
						Region_list[k].top = i;
					if(Region_list[k].bottom < i)
						Region_list[k].bottom = i;
					if(Region_list[k].right < j)
						Region_list[k].right = j;
					if(Region_list[k].left > j)
						Region_list[k].left = j;

					p_GrayImage[j] = parent[a];
				}
				else
					p_GrayImage[j] = 0;
			}
			else
				p_GrayImage[j] = 0;

	for(i=0;i<index;i++)
	{
        Region_list[i].cx /= Region_list[i].c;
		Region_list[i].cy /= Region_list[i].c;
	}

	delete [] BufferImage;
	delete [] parent;
	delete [] Counter;
	return index;
}

void CImgProcess::ResampleRAW(BYTE *InputBuf, int Width, int Height, BYTE *OutputBuf, int ReWidth, int ReHeight)
{
	int		i, j, x0, y0, x1, y1;
	double	x, y, fx0, fx1, fxy, T_X, T_Y, T_X1, T_Y1;
	double	Wratio = (double)ReWidth / Width,
			Hratio = (double)ReHeight / Height;
	BYTE *OutTemp = OutputBuf;
	BYTE *pTemp = NULL;

	for(i=0;i<ReHeight;i++, OutTemp+=ReWidth)
		for(j=0;j<ReWidth;j++)
		{
			x = j / Wratio;
			y = i / Hratio;
			x0 = (int)floor(x);
			y0 = (int)floor(y);
			x1 = x0 + 1;
			y1 = y0 + 1;

			if(x0 > (Width - 1))
				x0 = Width - 1;
			if(y0 > (Height - 1))
				y0 = Height - 1;

			pTemp = InputBuf + (y0 * Width) + x0;

			T_X = x - x0;
			T_Y = y - y0;
			T_X1 = 1 - T_X;
			T_Y1 = 1 - T_Y;

			if(y1 >= Height)
			{
				if(x1 >= Width)
				{
					fxy = pTemp[0    ];
				}
				else
				{
					fxy = T_X1 * pTemp[0    ] + T_X * pTemp[1        ];
				}
			}
			else
			{
				if(x1 >= Width)
				{
					fxy = T_Y1 * pTemp[0    ] + T_Y * pTemp[Width];
				}
				else
				{
					fx0 = T_X1 * pTemp[0    ] + T_X * pTemp[1        ];
					fx1 = T_X1 * pTemp[Width] + T_X * pTemp[1 + Width];
					fxy = T_Y1 * fx0 + T_Y * fx1;
				}
			}
			OutTemp[j] = (BYTE)fxy;
		}
}

void CImgProcess::CopyBuffers(double *pDest, double *pSource, int Dest_W, int Dest_H, int Source_W, int Source_H, int Dest_Start_X, int Dest_Start_Y, int Source_Start_X, int Source_Start_Y, int Source_End_X, int Source_End_Y, int Copy_BPP)
{
	int i,
		Copy_W = (Source_End_X - Source_Start_X) * Copy_BPP,
		Copy_H = Source_End_Y - Source_Start_Y;

	double *p_pDest = pDest + Dest_Start_X * Copy_BPP + Dest_Start_Y * Dest_W;
	double *p_pSource = pSource + Source_Start_X * Copy_BPP + Source_Start_Y * Source_W;

	for(i=0;i<Copy_H;i++, p_pSource+=Source_W, p_pDest+=Dest_W)
		memcpy(p_pDest, p_pSource, sizeof(double) * Copy_W);
}

void CImgProcess::CopyBuffers(BYTE *pDest, BYTE *pSource, int Dest_W, int Dest_H, int Source_W, int Source_H, int Dest_Start_X, int Dest_Start_Y, int Source_Start_X, int Source_Start_Y, int Source_End_X, int Source_End_Y, int Copy_BPP)
{
	int i,
		Copy_W = (Source_End_X - Source_Start_X) * Copy_BPP,
		Copy_H = Source_End_Y - Source_Start_Y;

	BYTE *p_pDest = pDest + Dest_Start_X * Copy_BPP + Dest_Start_Y * Dest_W;
	BYTE *p_pSource = pSource + Source_Start_X * Copy_BPP + Source_Start_Y * Source_W;

	for(i=0;i<Copy_H;i++, p_pSource+=Source_W, p_pDest+=Dest_W)
		memcpy(p_pDest, p_pSource, sizeof(BYTE) * Copy_W);
}

void CImgProcess::FlashSort(double a[], int n, int m, int *ctr)
{
	/* Subroutine Flash(a, n, m, ctr)
	- Sorts array a with n elements by use of the index vector l of dimension m (with m about 0.1 n).
	- The routine runs fastest with a uniform distribution of elements.
	- The vector l is declare dynamically using the calloc function.
	- The variable ctr counts the number of times that flashsort is called.

	- THRESHOLD is a very important constant.
	- It is the minimum number of elements required in a subclass before recursion is used. */

	int THRESHOLD = 75;
	int	CLASS_SIZE = 75;     /* minimum value for m */

	// declare variables
	int *l,
		i, j, k,
		nmin = 0, nmax = 0,
		nmove, nx;

	double c1, c2, flash, hold;

	// allocate space for the l vector
	l = new int[m];

	for(i=0;i<n;i++)
		if(a[i] < a[nmin])
			nmin = i;
		else if(a[i] > a[nmax])
			nmax = i;

	if((a[nmax] == a[nmin]) && (ctr == 0))
	{
		printf("All the numbers are identical, the list is sorted\n");
		return;
	}

	c1 = (m - 1.0) / (a[nmax] - a[nmin]);
	c2 = a[nmin];

	l[0] = -1;	// since the base of the "a" (data) array is 0

	for(k=1;k<m;k++)
		l[k] = 0;

	for(i=0;i<n;i++)
	{
		k = (int)(floor(c1 * (a[i] - c2)));
		l[k] += 1;
	}

	for(k=1;k<m;k++)
		l[k] += l[k - 1];

	hold = a[nmax];
	a[nmax] = a[0];
	a[0] = hold; 

	nmove = 0;
	j = 0;
	k = m - 1;

	while(nmove < n)
	{
		while(j > l[k])
		{
			j++;
			k = (int)(floor(c1 * (a[j] - c2)));
		}

		flash = a[j];

		while(j <= l[k])
		{
			k = (int)(floor(c1 * (flash - c2)));
			hold = a[l[k]];
			a[l[k]] = flash;
			l[k]--;
			flash = hold;
			nmove++;
		}
	}

	// Choice of RECURSION or STRAIGHT INSERTION
	for(k=0;k<(m-1);k++)
		if((nx = l[k + 1] - l[k]) > THRESHOLD)	// then use recursion
		{
			FlashSort(&a[l[k] + 1], nx, CLASS_SIZE, ctr);
			(*ctr)++;
		}
		else									// use insertion sort
			for(i=l[k+1]-1;i>l[k];i--)
				if(a[i] > a[i + 1])
				{
					hold = a[i];
					j = i;

					while(hold > a[j + 1])
						a[j++] = a[j + 1];
					a[j] = hold;
				}

	delete [] l;
	l = NULL;
}

void CImgProcess::LineFit(CPoint *PointsData, int PointsCount, double &b, double &M)
{
	// Y = MX + b
	int i;
	double	t, sxoss, ss,
			sx = 0.0,
			sy = 0.0,
			st2 = 0.0;
	M = 0.0;

	for(i=0;i<PointsCount;i++)
	{
		sx += PointsData[i].x;
		sy += PointsData[i].y;
	}

	ss = PointsCount;
	sxoss = sx / ss;

	for(i=0;i<PointsCount;i++)
	{
		t = PointsData[i].x - sxoss;
		st2 += t * t;
		M += t * PointsData[i].y;
	}

	if(st2 == 0)
		M = 0.0;
	else
		M /= st2;
	b = (sy - (sx * M)) / ss;
/*
	double	Siga,		// 回傳標準差
			Sigb,		// 回傳標準差
			Chi2,		// 方均根誤差量
			Sigdat;
	Siga = sqrt((1.0 + (sx * sx) / (ss * st2)) / ss);
	Sigb = sqrt(1.0 / st2);
	Chi2 = 0.0;
	for(i=0;i<PointsCount;i++)
		Chi2 += ((PointsData[i].y - b - (M * PointsData[i].x) * (PointsData[i].y - b - (M * PointsData[i].x))));

	Sigdat = sqrt(Chi2 / (PointsCount - 2));
	Chi2 = sqrt(Chi2 / PointsCount);
	Siga *= Sigdat;
	Sigb *= Sigdat;
*/
}

void CImgProcess::CopyBuffers_Point(BYTE *SourceBuf, int Source_W, int Source_H, int Source_BPP, BYTE *DestBuf, int Dest_W, int Dest_H, int Dest_BPP, CPoint Source_P, CPoint Dest_P)
{
	int	i,
		Source_PW = (Source_W * Source_BPP + 3) >> 2 << 2,
		Dest_PW = (Dest_W * Dest_BPP + 3) >> 2 << 2;

	BYTE *p_Source = SourceBuf + Source_PW * Source_P.y + Source_BPP * Source_P.x;
	BYTE *p_Dest = DestBuf + Dest_PW * Dest_P.y + Dest_BPP * Dest_P.x;

	for(i=0;i<Dest_BPP;i++)
		p_Dest[i] = p_Source[i];
}

void CImgProcess::CopyBuffers_Point(BYTE *SourceBuf, int Source_W, int Source_H, int Source_BPP, BYTE *DestBuf, int Dest_W, int Dest_H, int Dest_BPP, double Source_Px, double Source_Py, CPoint Dest_P)
{
	int	i, j, m, n,
		Source_PW = (Source_W * Source_BPP + 3) >> 2 << 2,
		Dest_PW = (Dest_W * Dest_BPP + 3) >> 2 << 2;

	int	Px = (int)(floor(Source_Px)),
		Py = (int)(floor(Source_Py)),
		Px_2 = Px + 1,
		Py_2 = Py + 1;
	double	Ratio_X_1 = Source_Px - (double)Px,
			Ratio_X_2 = 1.0 - Ratio_X_1,
			Ratio_Y_1 = Source_Py - (double)Py,
			Ratio_Y_2 = 1.0 - Ratio_Y_1;
	BYTE	*p_Source = NULL,
			*p_Dest = DestBuf + Dest_PW * Dest_P.y + Dest_BPP * Dest_P.x;

	if(Px_2 >= Source_W)
	{
		if(Py_2 >= Source_H)
		{
			p_Source = SourceBuf + Source_PW * Py + Source_BPP * Px;
			for(i=0;i<Dest_BPP;i++)
				p_Dest[i] = p_Source[i];
		}
		else
		{
			p_Source = SourceBuf + Source_PW * Py + Source_BPP * Px;
			for(i=0, j=Source_BPP;i<Dest_BPP;i++, j++)
				p_Dest[i] = (BYTE)(Ratio_X_2 * (double)p_Source[i] + Ratio_X_1 * (double)p_Source[j]);
		}
	}
	else
	{
		if(Py_2 >= Source_H)
		{
			p_Source = SourceBuf + Source_PW * Py + Source_BPP * Px;
			for(i=0, j=Source_PW;i<Dest_BPP;i++, j++)
				p_Dest[i] = (BYTE)(Ratio_Y_2 * (double)p_Source[i] + Ratio_Y_1 * (double)p_Source[j]);
		}
		else
		{
			p_Source = SourceBuf + Source_PW * Py + Source_BPP * Px;
			for(i=0, j=Source_BPP, m=Source_PW, n=Source_PW+Source_BPP;i<Dest_BPP;i++, j++, m++, n++)
				p_Dest[i] = (BYTE)(
					Ratio_Y_2 * (Ratio_X_2 * (double)p_Source[i] + Ratio_X_1 * (double)p_Source[j]) + 
					Ratio_Y_1 * (Ratio_X_2 * (double)p_Source[m] + Ratio_X_1 * (double)p_Source[n]));
		}
	}
}

void CImgProcess::CopyBuffersRAW_Point(BYTE *SourceBuf, int Source_W, int Source_H, BYTE *DestBuf, int Dest_W, int Dest_H, CPoint Source_P, CPoint Dest_P)
{
	BYTE *p_Source = SourceBuf + Source_W * Source_P.y + Source_P.x;
	BYTE *p_Dest = DestBuf + Dest_W * Dest_P.y + Dest_P.x;

	p_Dest[0] = p_Source[0];
}

void CImgProcess::CopyBuffersRAW_Point(BYTE *SourceBuf, int Source_W, int Source_H, BYTE *DestBuf, int Dest_W, int Dest_H, double Source_Px, double Source_Py, CPoint Dest_P)
{
	int	Px = (int)(floor(Source_Px)),
		Py = (int)(floor(Source_Py)),
		Px_2 = Px + 1,
		Py_2 = Py + 1;
	double	Ratio_X_1 = Source_Px - (double)Px,
			Ratio_X_2 = 1.0 - Ratio_X_1,
			Ratio_Y_1 = Source_Py - (double)Py,
			Ratio_Y_2 = 1.0 - Ratio_Y_1;
	BYTE	*p_Source = NULL,
			*p_Dest = DestBuf + Dest_W * Dest_P.y + Dest_P.x;

	if(Px_2 >= Source_W)
	{
		if(Py_2 >= Source_H)
		{
			p_Source = SourceBuf + Source_W * Py + Px;
			p_Dest[0] = p_Source[0];
		}
		else
		{
			p_Source = SourceBuf + Source_W * Py + Px;
			p_Dest[0] = (BYTE)(Ratio_X_2 * (double)p_Source[0] + Ratio_X_1 * (double)p_Source[1]);
		}
	}
	else
	{
		if(Py_2 >= Source_H)
		{
			p_Source = SourceBuf + Source_W * Py + Px;
			p_Dest[0] = (BYTE)(Ratio_Y_2 * (double)p_Source[0] + Ratio_Y_1 * (double)p_Source[Source_W]);
		}
		else
		{
			p_Source = SourceBuf + Source_W * Py + Px;
			p_Dest[0] = (BYTE)(
				Ratio_Y_2 * (Ratio_X_2 * (double)p_Source[0] + Ratio_X_1 * (double)p_Source[1]) + 
				Ratio_Y_1 * (Ratio_X_2 * (double)p_Source[Source_W] + Ratio_X_1 * (double)p_Source[Source_W + 1]));
		}
	}
}

void CImgProcess::DrawCross(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint CrossPoint, int CrossSize, COLORREF LineColor, UINT TYPE)
{
	int HalfCrossSize = CrossSize / 2,
		Xm = CrossPoint.x - HalfCrossSize,
		Xp = CrossPoint.x + HalfCrossSize,
		Ym = CrossPoint.y - HalfCrossSize,
		Yp = CrossPoint.y + HalfCrossSize;

	Xm = Max(0, Xm);
	Ym = Max(0, Ym);
	Xp = Min(Image_W - 1, Xp);
	Yp = Min(Image_H - 1, Yp);

	CPoint	CorPoint[9];	// 0 1 2
							// 3 4 5
							// 6 7 8
	CorPoint[0].x = Xm;				CorPoint[0].y = Ym;
	CorPoint[1].x = CrossPoint.x;	CorPoint[1].y = Ym;
	CorPoint[2].x = Xp;				CorPoint[2].y = Ym;
	CorPoint[3].x = Xm;				CorPoint[3].y = CrossPoint.y;
	CorPoint[4].x = CrossPoint.x;	CorPoint[4].y = CrossPoint.y;
	CorPoint[5].x = Xp;				CorPoint[5].y = CrossPoint.y;
	CorPoint[6].x = Xm;				CorPoint[6].y = Yp;
	CorPoint[7].x = CrossPoint.x;	CorPoint[7].y = Yp;
	CorPoint[8].x = Xp;				CorPoint[8].y = Yp;


	if(TYPE == 0)		// Ｘ
	{
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y + HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y + HalfCrossSize), LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[0], CorPoint[8], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[2], CorPoint[6], LineColor);
	}
	else if(TYPE == 1)	// ┼
	{
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x, CrossPoint.y + HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y), CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y), LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[3], CorPoint[5], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[1], CorPoint[7], LineColor);
	}
	else if(TYPE == 2)	// Ｘ┼
	{
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y + HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y + HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x, CrossPoint.y + HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y), CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y), LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[0], CorPoint[8], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[2], CorPoint[6], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[3], CorPoint[5], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[1], CorPoint[7], LineColor);
	}
	else				// 口
	{
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y + HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y + HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y - HalfCrossSize), CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y - HalfCrossSize), LineColor);
//		DrawLine(pImage, Image_W, Image_H, Image_BPP, CPoint(CrossPoint.x + HalfCrossSize, CrossPoint.y + HalfCrossSize), CPoint(CrossPoint.x - HalfCrossSize, CrossPoint.y + HalfCrossSize), LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[0], CorPoint[2], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[6], CorPoint[8], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[0], CorPoint[6], LineColor);
		DrawLine(pImage, Image_W, Image_H, Image_BPP, CorPoint[2], CorPoint[8], LineColor);
	}
}

// (struct lineeq *lineptr,CPoint *foe,int linenum)
CPoint CImgProcess::FocusOfExpansion(LINEEQUATION_ENTRY *LineEq, int LineCount, int Iteration, int TH_FOE2Line, int TH_MinLineCount)
{
	int x, i, iL,
		Current_Line_Count;
	double	a, b,
			Residual, Avg_Residual, Threshold;
	double **A, **B;

	Current_Line_Count = 0;
	for(i=0;i<LineCount;i++)
		if(LineEq[i].Usefull_Line)
		{
			LineEq[i].TempSqrtPa2Pb2 = sqrt(LineEq[i].Pa * LineEq[i].Pa + LineEq[i].Pb * LineEq[i].Pb);
			Current_Line_Count++;
		}

	for(x=0;x<Iteration;x++)
	{
		A = new double*[Current_Line_Count];
		for(i=0;i<Current_Line_Count;i++)
			A[i] = new double[2];

		for(i=0, iL=0;iL<LineCount;iL++)
			if(LineEq[iL].Usefull_Line)
			{
				A[i][0] = LineEq[iL].Pa;
				A[i][1] = LineEq[iL].Pb;
				i++;
			}

		B = new double*[2];
		for(i=0;i<2;i++)
			B[i] = new double[Current_Line_Count];

		PinV(A, B, Current_Line_Count);
		a = 0.0;
		b = 0.0;

		for(i=0, iL=0;iL<LineCount;iL++)
			if(LineEq[iL].Usefull_Line)
			{
				a += B[0][i] * (-LineEq[iL].Pc);
				b += B[1][i] * (-LineEq[iL].Pc);
				i++;
			}

		
//		for(i=0,ptr=lineptr;ptr;i++,ptr=ptr->nextline)
//			a += B[0][i] * (-ptr->c);
//		for(i=0,ptr=lineptr;ptr;i++,ptr=ptr->nextline)
//			b += B[1][i] * (-ptr->c);

		Avg_Residual = 0;

		for(iL=0;iL<LineCount;iL++)
			if(LineEq[iL].Usefull_Line)
			{
				LineEq[iL].TempFabsPaPbPc = fabs(LineEq[iL].Pa * a + LineEq[iL].Pb * b + LineEq[iL].Pc);
				Residual = LineEq[iL].TempFabsPaPbPc / LineEq[iL].TempSqrtPa2Pb2;
				Avg_Residual += Residual;
			}

//		for(ptr=lineptr;ptr;ptr=ptr->nextline)
//		{
//			Residual = fabs(ptr->a*a+ptr->b*b+ptr->c) / sqrt(ptr->a*ptr->a+ptr->b*ptr->b);
//			Avg_Residual += Residual;
//		}

		Avg_Residual /= Current_Line_Count;
		Threshold = Avg_Residual;

		if(Threshold < TH_FOE2Line)
			Threshold = TH_FOE2Line;

		i = Current_Line_Count;
		for(iL=0;iL<LineCount;iL++)
			if(LineEq[iL].Usefull_Line)
			{
				Residual = LineEq[iL].TempFabsPaPbPc / LineEq[iL].TempSqrtPa2Pb2;
				if((Residual > Threshold) && (i >= TH_MinLineCount))
				{
					LineEq[iL].Usefull_Line = FALSE;
					i--;
				}
			}

/*
		for(ptr=lineptr;ptr;ptr=ptr->nextline)
		{
			Residual = fabs(ptr->a*a+ptr->b*b+ptr->c) / sqrt(ptr->a*ptr->a+ptr->b*ptr->b);

			if(Residual > Threshold && Current_Line_Count > 25)
			{
				if(ptr==lineptr)
				{
					lineptr=ptr->nextline;
					Current_Line_Count--;
				}
				else
				{
					struct lineeq* temp=new struct lineeq;
					for(temp=lineptr;temp->nextline!=ptr;temp=temp->nextline)
						;
					temp->nextline=ptr->nextline;
					Current_Line_Count--;
				}
			}
		}
*/
		for(i=0;i<Current_Line_Count;i++)
		{
			delete [] A[i];			A[i] = NULL;
		}

		delete [] B[0];			B[0] = NULL;
		delete [] B[1];			B[1] = NULL;

		delete [] A;		A = NULL;
		delete [] B;		B = NULL;

		Current_Line_Count = 0;
		for(iL=0;iL<LineCount;iL++)
			if(LineEq[iL].Usefull_Line)
				Current_Line_Count++;
	}

	return(CPoint((int)(a + 0.5), (int)(b + 0.5)));
}

void CImgProcess::PinV(double **A, double **B, int h)
{
	int i, j, k;
	double **At, AtA[2][2], inAtA[2][2];
	double sum, det;
	At = new double*[2];
	for(i=0;i<2;i++)
		At[i] = new double[h];

	for(i=0;i<2;i++)
		for(j=0;j<h;j++)
			At[i][j] = A[j][i];

	for(i=0;i<2;i++)
		for(j=0;j<2;j++)
		{
			sum = 0;
			for(k=0;k<h;k++)
				sum += At[i][k] * A[k][j];
			AtA[i][j] = sum;
		}

	det = AtA[0][0] * AtA[1][1] - AtA[0][1] * AtA[1][0];
	inAtA[0][0] = AtA[1][1] / det;
	inAtA[0][1] = -AtA[0][1] / det;
	inAtA[1][0] = -AtA[1][0] / det;
	inAtA[1][1] = AtA[0][0] / det;

	for(i=0;i<2;i++)
		for(j=0;j<h;j++)
		{
			sum = 0;
			for(k=0;k<2;k++)
				sum += inAtA[i][k] * At[k][j];
			B[i][j] = sum;
		}

	for(i=0;i<2;i++)
	{
		delete [] At[i];	At[i] = NULL;
	}
	delete [] At;	At = NULL;
}

void CImgProcess::QuickSort(int *pointlist, int n)
{
	unsigned int i, j, ln, rn;
	while(n > 1)
	{
		SWAP3(pointlist, 0, n / 2);
		for(i=0, j=n;;)
		{
			do
				--j;
				while(pointlist[3 * j + 2] < pointlist[2]);
			do
				++i;
				while(i < j && pointlist[3 * i + 2] > pointlist[2]);

			if(i >= j)
				break;
			SWAP3(pointlist, i, j);
		}

		SWAP3(pointlist, j, 0);
		ln = j;
		rn = n - ++j;
		if(ln < rn)
		{
			QuickSort(pointlist, ln);
			pointlist += 3 * j;
			n = rn;
		}
		else
		{
			QuickSort(pointlist + 3 * j, rn);
			n = ln;
		}
	}
}

void CImgProcess::Mosaic(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR, int Block_W, int Block_H, int Image_BPP)
{
	int i, j, k, mW, mH, mTa, mTb,
		Block_Size, Block_4W,
		Image_PW = (Image_BPP * Image_W + 3) >> 2 << 2,		
		Skip_W, Skip_H,
		ProcRegion_W = (DR.x - UL.x + 1),
		ProcRegion_H = (DR.y - UL.y + 1);
	Block_W = Min(Block_W, ProcRegion_W);
	Block_H = Min(Block_H, ProcRegion_H);
	Block_Size = Block_W * Block_H;
	Block_4W = Block_W * 4;

	int	*ColorTable = new int[Block_Size * 4];
	memset(ColorTable, 0, sizeof(int) * Block_Size * 4);

	Skip_W = ProcRegion_W / Block_W;
	Skip_H = ProcRegion_H / Block_H;

	BYTE *p_pImage = pImage + UL.y * Image_PW + UL.x * Image_BPP;
	for(i=0;i<ProcRegion_H;i++, p_pImage+=Image_PW)
	{
		mH = i / Skip_H;
		if(mH >= Block_H)	mH = Block_H - 1;
		mTa = mH * Block_4W;
		for(j=0, k=0;j<ProcRegion_W;j++, k+=Image_BPP)
		{
			mW = j / Skip_W;
			if(mW >= Block_W)	mW = Block_W - 1;
			mTb = mTa + (mW * 4);

			ColorTable[mTb    ]++;
			ColorTable[mTb + 1] += p_pImage[k    ];
			ColorTable[mTb + 2] += p_pImage[k + 1];
			ColorTable[mTb + 3] += p_pImage[k + 2];
		}
	}

	for(i=0, j=0;i<Block_Size;i++, j+=4)
		if(ColorTable[j] > 0)
		{
			ColorTable[j + 1] /= ColorTable[j];
			ColorTable[j + 2] /= ColorTable[j];
			ColorTable[j + 3] /= ColorTable[j];
		}

	p_pImage = pImage + UL.y * Image_PW + UL.x * Image_BPP;
	for(i=0;i<ProcRegion_H;i++, p_pImage+=Image_PW)
	{
		mH = i / Skip_H;
		if(mH >= Block_H)	mH = Block_H - 1;
		mTa = mH * Block_4W;
		for(j=0, k=0;j<ProcRegion_W;j++, k+=Image_BPP)
		{
			mW = j / Skip_W;
			if(mW >= Block_W)	mW = Block_W - 1;
			mTb = mTa + (mW * 4);

			p_pImage[k    ] = ColorTable[mTb + 1];
			p_pImage[k + 1] = ColorTable[mTb + 2];
			p_pImage[k + 2] = ColorTable[mTb + 3];
		}
	}


	delete [] ColorTable; ColorTable = NULL;
}

void CImgProcess::Smooth(BYTE *in_image, BYTE *out_image, int inw, int inh)
{
	for (int i=1; i<inh-1 ;i++) //i控制列(高)
	{
		for (int j=1; j<inw-1 ;j++)           //j控制行(寬)
		{

			if( (in_image[(i-1)*inw+(j-1)]+
				 in_image[(i-1)*inw+(j  )]+
				 in_image[(i-1)*inw+(j+1)]+
				 in_image[(i  )*inw+(j-1)]+
				 in_image[(i  )*inw+(j  )]+
				 in_image[(i  )*inw+(j+1)]+
				 in_image[(i+1)*inw+(j-1)]+
				 in_image[(i+1)*inw+(j  )]+
				 in_image[(i+1)*inw+(j+1)])/9 > 150)
			out_image[(i*inw)+j]= 255;
			else
			out_image[(i*inw)+j]= 0;
		}		
	} 
}


void CImgProcess::pBufferX_to_p_X(BYTE *in_image, BYTE *out_image, int inw, int inh, int inBPP)
{
	for(int i=0; i< inh; i++)
		for(int j=0,k=0;j<inw;j++,k+=3)
		{
			
			if (in_image[i*inw+j] == 255)
			{
				out_image[i*inw*inBPP+k] = 255;
				out_image[i*inw*inBPP+k+1] = 255;
				out_image[i*inw*inBPP+k+2] = 255;
			}
			else
			{
				out_image[i*inw*inBPP+k] = 0;
				out_image[i*inw*inBPP+k+1] = 0;
				out_image[i*inw*inBPP+k+2] = 0;
			}
			}
}
