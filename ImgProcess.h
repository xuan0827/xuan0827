// ImgProcess.h: interface for the CImgProcess class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMGPROCESS_H__850296A9_BF4E_49BE_BECA_ECE4C93A683C__INCLUDED_)
#define AFX_IMGPROCESS_H__850296A9_BF4E_49BE_BECA_ECE4C93A683C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// 2007.10.31

typedef struct IMGPROCESS_REGION_ENTRY{
	int		c;
	int		cx, cy;
	int		label;
	int		left, right, top, bottom;
	BOOL	used;
}REGION_INFORMATION;

typedef struct LINEEQUATION_ENTRY{
	CPoint	PointA, PointB;
	double	Distance;
	double	Pa, Pb, Pc;
	double	TempFabsPaPbPc, TempSqrtPa2Pb2;
	BOOL	Usefull_Line;
}LINEEQUATION_INFORMATION;

#define Safe_Delete(ptr)				\
{										\
	if(ptr)								\
	{									\
		delete ptr;						\
		ptr	= NULL;						\
	}									\
}

#define Safe_DeleteArr(ptr)				\
{										\
	if(ptr)								\
	{									\
		delete[] ptr;					\
		ptr	= NULL;						\
	}									\
}

class CImgProcess  
{
public:
	CImgProcess();
	virtual ~CImgProcess();

public:
	void	pBufferX_to_p_X(BYTE *in_image, BYTE *out_image, int inw, int inh, int inBPP);
	void	Smooth(BYTE *in_image, BYTE *out_image, int inw, int inh);
	void	QuickSort(int *pointlist, int n);
	void	FlashSort(double a[], int n, int m, int *ctr);

	// Filters
	void	MedianFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP = 3);
	void	MaximumFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP = 3);
	void	MinimumFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP = 3);
	void	AverageFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int BPP = 3);
	BOOL	HighBoostFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, double BoostValue = 1.5, int BPP = 3);
	void	GaussianAverageFilter(BYTE *p_Process_Image, float *p_Result_Image, int n_Process_W, int n_Process_H, double S, int BPP = 3);
	void	GaussianAverageFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int MaskSize = 11, double Variance = 1.4, double Mean_X = 0, double Mean_Y = 0, int BPP = 3);
	void	GaussianAverageFilterDouble(double *InputData, double *OutputData, int Data_W, int Data_H, int MaskSize = 7, double Variance = 1.4, double Mean_X = 0, double Mean_Y = 0, int BPP = 3);
	void	KuwaharaFilter(BYTE *p_Process_Image, BYTE *p_Result_Image, int n_Process_W, int n_Process_H, int EachRegion_W = 3, int EachRegion_H = 3, int BPP = 3);

	// Edge Detection
	BOOL	SobelOperation(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int InputBPP, int SobelThreshold = 100, UINT SobelMode = 0, BOOL BinaryProcess = FALSE);
	BOOL	LaplicationOperation(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int InputBPP, int LaplacianThreshold = 100, BOOL BinaryProcess = FALSE);
	BOOL	GradientOperation(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int InputBPP, int GradientThreshold = 100, UINT GradientMode = 0, BOOL BinaryProcess = FALSE);
//	BOOL	LaplicationOfGaussianOperation(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int InputBPP, int LaplacianThreshold = 100, BOOL BinaryProcess = FALSE);

	BOOL	GradientOperationRAW(BYTE *pInput, BYTE *pOutput, int InputWidth, int InputHeight, int GradientThreshold = 100, UINT GradientMode = 0, BOOL BinaryProcess = FALSE);

	// Histogram, Equalization
	BOOL	Histogram(BYTE *p_Process_Image, int n_Process_W, int n_Process_H,
					double *t_PDFData, double *t_CDFData,
					BOOL b_PDF_Process_Enable = FALSE, BYTE *p_PDF_Image = NULL, int n_PDF_H = NULL,
					BOOL b_CDF_Process_Enable = FALSE, BYTE *p_CDF_Image = NULL, int n_CDF_H = NULL,
					int BPP = 3);
	BOOL	Equalization(BYTE *p_Process_Image, int n_Process_W, int n_Process_H,
					BOOL b_PDF_Process_Enable = FALSE, BYTE *p_PDF_Image = NULL, int n_PDF_H = NULL,
					BOOL b_CDF_Process_Enable = FALSE, BYTE *p_CDF_Image = NULL, int n_CDF_H = NULL,
					int BPP = 3);
	BOOL	EqualizationHSV(BYTE *p_InputImage, BYTE *p_OutputImage, int n_ImageWidth, int n_ImageHeight);

	BOOL	Histogram_RGB(BYTE *p_Process_Image, int n_Process_W, int n_Process_H,
					double *t_DataR, double *t_DataG, double *t_DataB,
					int NormalizeTo = 0, int BPP = 3);

	BOOL	Equalization(BYTE *p_Image, int Image_W, int Image_H, int Image_BPP = 3);


	// Morphology
	BOOL	Erosion(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h);
	BOOL	ErosionROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR);
	BOOL	Dilation(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h);
	BOOL	DilationROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR);
	BOOL	Opening(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h);
	BOOL	OpeningROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR);
	BOOL	Closing(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h);
	BOOL	ClosingROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR);
	BOOL    Average(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h);
	BOOL	AverageROI(BYTE *in_image, int inw, int inh, BYTE *out_image, int s_w, int s_h, CPoint UL, CPoint DR);
	BOOL	Different(BYTE *in1, BYTE *in2, BYTE *out, int inw, int inh);
	BOOL	Thresholding(BYTE *gray, BYTE *in, int inw, int inh, int upT, int bottomT);

	// Connected Component
	int		ConnectComponent(int *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, int Threshold = 10);
	int		ConnectComponentROI(int *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, CPoint ROI_UL, CPoint ROI_DR, int Threshold = 10);
	int		ConnectComponent_BinaryInput(BYTE *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, int Threshold = 10);
	int		ConnectComponent_BinaryInput(int *GrayImage, int Image_W, int Image_H, IMGPROCESS_REGION_ENTRY *Region_list, int Threshold = 10);

	// Corner Detect
	void	HarrisCornerDetector(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, int Threshold = 2500, double Weighting = 0.04);

	// Rotation
	BYTE*	Rotate(BYTE *pImage, int Image_W, int Image_H, double RotateAngle, CSize &RotateSize, int Image_BPP = 3);

	// Resample
	BOOL	Resample(BYTE *pOri_Image, int Ori_W, int Ori_H, BYTE *pDes_Image, int Des_W, int Des_H, int Image_BPP = 3, UINT Sampling_Mode = 2);
	void	ResampleRAW(BYTE *InputBuf, int Width, int Height, BYTE *OutputBuf, int ReWidth, int ReHeight);

	// Threshold
	void	MomentThreshold(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B);
	void	MomentThresholdROI(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B);
	int		MomentThresholdRAW(BYTE *pImage, int Image_W, int Image_H);
	int		MomentThresholdROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR);

	void	MountainBandThresholdRAW(BYTE *pImage, int Image_W, int Image_H, int &Ban1, int &Ban2, double DifferenceTHRatio = 0.7, int PreSkipBan = 10, int PreCalBan = 30);

	void	IsoDataThreshold(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B);
	void	IsoDataThresholdROI(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B);
	int		IsoDataThresholdRAW(BYTE *pImage, int Image_W, int Image_H);
	int		IsoDataThresholdROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR);

	void	MaximumEntropyThreshold(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B);
	void	MaximumEntropyThresholdROI(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, BYTE &CutoffThreshold_R, BYTE &CutoffThreshold_G, BYTE &CutoffThreshold_B);
	int		MaximumEntropyThresholdRAW(BYTE *pImage, int Image_W, int Image_H);
	int		MaximumEntropyThresholdROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR);

	int		MinimumResidueThresholdRAW(BYTE *pImage, int Image_W, int Image_H);


	// Direction
	double	MomentDirectionRAW(BYTE *pImage, int Image_W, int Image_H, CPoint &Center_Point, int Threshold = 128, int Assign_Value = -1);
	double	MomentDirectionROIRAW(BYTE *pImage, int Image_W, int Image_H, CPoint &Center_Point, CPoint UL, CPoint DR, int Threshold = 128, int Assign_Value = -1);

	// Draw Line, Rect to Buffer
	void	DrawLine(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint Line_P1, CPoint Line_P2, COLORREF LineColor);
	void	DrawLine(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint Line_P1, CPoint Line_P2, COLORREF LineColorS, COLORREF LineColorE);
	void	DrawCross(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint CrossPoint, int CrossSize, COLORREF LineColor, UINT TYPE = 0);
			// TYPE: 0 (¢æ), 1 (¢q), 2 (¢æ¢q), 3 (¤f)

	void	DrawRect(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CRect LineRect, COLORREF LineColor);
	void	DrawRect(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint UL, CPoint DR, COLORREF LineColor);

	// Logic
	void	Logic_AND(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize);
	void	Logic_OR(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize);
	void	Logic_XOR(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize, int Tolorance = 5);
	void	Logic_SUB(BYTE *pSource, BYTE *pRef, BYTE *pResult, int DataSize, int Tolorance = 5);

	// Buffer Copy
	void	CopyBuffers(BYTE *pDest, BYTE *pSource, int Dest_W, int Dest_H, int Source_W, int Source_H, int Dest_Start_X, int Dest_Start_Y, int Source_Start_X, int Source_Start_Y, int Source_End_X, int Source_End_Y, int Copy_BPP = 1);
	void	CopyBuffers(double *pDest, double *pSource, int Dest_W, int Dest_H, int Source_W, int Source_H, int Dest_Start_X, int Dest_Start_Y, int Source_Start_X, int Source_Start_Y, int Source_End_X, int Source_End_Y, int Copy_BPP = 1);
	void	CopyBuffers_Point(BYTE *SourceBuf, int Source_W, int Source_H, int Source_BPP, BYTE *DestBuf, int Dest_W, int Dest_H, int Dest_BPP, CPoint Source_P, CPoint Dest_P);
	void	CopyBuffers_Point(BYTE *SourceBuf, int Source_W, int Source_H, int Source_BPP, BYTE *DestBuf, int Dest_W, int Dest_H, int Dest_BPP, double Source_Px, double Source_Py, CPoint Dest_P);
	void	CopyBuffersRAW_Point(BYTE *SourceBuf, int Source_W, int Source_H, BYTE *DestBuf, int Dest_W, int Dest_H, CPoint Source_P, CPoint Dest_P);
	void	CopyBuffersRAW_Point(BYTE *SourceBuf, int Source_W, int Source_H, BYTE *DestBuf, int Dest_W, int Dest_H, double Source_Px, double Source_Py, CPoint Dest_P);


	// Focus Of Expansion(FOE)
	CPoint	FocusOfExpansion(LINEEQUATION_ENTRY *LineEq, int LineCount, int Iteration = 5, int TH_FOE2Line = 20, int TH_MinLineCount = 25);


	
	// Other
	double	FindAngle(CPoint P1, CPoint P2);
	int		FindDistance(CPoint P1, CPoint P2);
	double	FindDistance(double P1_x, double P1_y, double P2_x, double P2_y);
	void	ChamferDistanceRAW(BYTE *pImage, int Image_W, int Image_H, BYTE *pResult, BYTE Threshold = 128, int Max_V = 14, int Min_V = 10);
	void	LineFit(CPoint *PointsData, int PointsCount, double &b, double &M);

	void	Mosaic(BYTE *pImage, int Image_W, int Image_H, CPoint UL, CPoint DR, int Block_W = 10, int Block_H = 10, int Image_BPP = 3);
	BOOL	RGB2HSV(BYTE *p_InputImage, BYTE *p_OutputImage, int n_ImageWidth, int n_ImageHeight, int Optional);


private:
	void	PinV(double **A, double **B, int h);
	double	Gaussian(double Input, double Variance, double Mean = 0.0);
	double	Gaussian2D(double Input_X, double Input_Y, double Variance, double Mean_X = 0.0, double Mean_Y = 0.0);
	void	GaussianMatrix(int MatrixSize, double *GM, double Variance = 1.4, double Mean_X = 0, double Mean_Y = 0);
	int		GetGaussianMask(double s,float *Gaussian);
	BYTE	ByteValueCheck(int CheckValue);
	BOOL	HSV2RGB(BYTE *p_InputImage, BYTE *p_OutputImage, int n_ImageWidth, int n_ImageHeight);
	void	Union(int x, int y, int *p);
	float	b3spline(float x);


	template<class T>	static void swap(T& t1, T& t2)	{T	ts = t1; t1 = t2; t2 = ts;}
	template<class T>	static T Min(T t1, T t2)		{return(t1 > t2 ? t2 : t1);}
	template<class T>	static T Min3(T t1, T t2, T t3)	{return(t1 < t2 ? t1 < t3 ? t1 : t3 : t2 < t3 ? t2 : t3);}
	template<class T>	static T Max(T t1, T t2)		{return(t1 > t2 ? t1 : t2);}
	template<class T>	static T Max3(T t1, T t2, T t3)	{return(t1 > t2 ? t1 > t3 ? t1 : t3 : t2 > t3 ? t2 : t3);}
	template<class T>	static T log2(T v)				{return(log10(v) / log10(2.0));}


	static int Min(int Vector[], int Size);
	static int Min(double Vector[], int Size);
	static int Max(int Vector[], int Size);
	static int Max(double Vector[], int Size);
};

#endif // !defined(AFX_IMGPROCESS_H__850296A9_BF4E_49BE_BECA_ECE4C93A683C__INCLUDED_)
