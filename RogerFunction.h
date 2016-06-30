#if !defined(AFX_ROGERFUNCTION_H__FCFC7E33_54C4_4427_BFC0_11B2962BA6E0__INCLUDED_)
#define AFX_ROGERFUNCTION_H__FCFC7E33_54C4_4427_BFC0_11B2962BA6E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once


#endif // _MSC_VER > 1000

//#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#define  EigenSize2x2  2
#define  EigenSize3x3  3

// RogerFunction.h : header file
//
#include ".\CxImage\ximage.h"

#include <vector>

#include "N_KeyPointDetector.h"
#include "N_KeyPointDescriptor.h"
#include "N_Image.h"
#include "N_SimpleGrayImage.h"

using namespace N_parallelsurf;
typedef unsigned char uchar;


using namespace std;
//using namespace surf;

// typedef struct  
// {
// 	double x;
// 	double y;
// 	double z;
// 
// }Point_3D;

typedef struct  
{
	CPoint RFLamp;
	CPoint RFWheel;
	CPoint RRWheel;
	BOOL   bRFLamp;
	BOOL   bRFWheel;
	BOOL   bRRWheel;

}Partial1;

typedef struct{
	int   nIdx;
	float fValue;
}INT_FLOAT_TYPE;

typedef struct  
{
	double l;
	double w;
	double h;

}UNITVECTOR;

typedef struct {
	int      x;
	int      y;
	int      Type;
	int      Type_Amount;
	double   Theta;
	int      Life_Match;
	int      Life_UnMatch;
	BOOL	 Status_Match;
}ObjectInformation;

typedef struct{
	CPoint pLeft;
	int    nLdist;
	CPoint pCenter;
	CPoint pRight;
	int    nRdist;
	int    nDist;
	int    nTheta;
	double dSimilar;
	BOOL   BDISPLAY;
	int    nBottomY;
	double dHoriStrength;
	double dHoriStrengthAVG;
	double dVertStrength;
	double dTotalS;
	CPoint LU, RB;
}LocalMaxLine;
typedef struct{
	double dTotal;
	int nCenterX;
	int nStartH;
	int nEndH;
	int nDist;
	BOOL Remain;
}Segment;
typedef struct{
	int CenterPosX;
	vector <CPoint>CenterPoint;
	//決定垂直軸Y搜尋範圍
	int nHUP;
	int nHBT;
}SelfINFO;
typedef struct{
	CPoint mUL1;
	CPoint mRB1;
	CPoint mUL2;
	CPoint mRB2;
}MatchRectangle;

typedef struct{
	int nNum;
	int nClass1;
	int nClass2;
	double dCELL[576];

}dCELL_TYPE;
typedef struct{
	CPoint cpA;
	CPoint cpB;
	double dDist;
	
}MatchCorner;
///kmeans start //////////////////////////////////////////////////////////////////////////
struct DataType
{
	int x;
	int y;
	int Type;
	double *data;
	int father;
	double *uncle;
};

struct ClusterType
{
	double *center;
	int sonnum;
};
///kmeans end //////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CRogerFunction document
typedef struct{
	int x;
	int y;
	int Value;
} TriPoint;
typedef struct{
	int Value;
	int Dist;
	int AngleNumber;
}	MaxValue;
typedef struct{
	int Value;
	int Radius;
	int x;
	int y;
}	MaxValue_C;

typedef struct {
	vector <double>		img;
	int 				M,N,C;
	} Img;

typedef struct {
	int 	O;
	int 	S;
	double 	sigma0;
	int 	omin;
	int 	smin;
	int 	smax;
	vector<Img> octave;
	} Gss;

typedef struct {
	int 	smin;
	int 	smax;
	int 	omin;
	int 	O;
	int 	S;
	double 	sigma0;
	vector<Img> octave;
	} Dog;

typedef struct {
	Img frames;
	Img descriptors;
	Gss gss;
	Dog dogss;
	} SiftInfo;

typedef struct 
{ 
	int k1 ; 
	int k2 ;
	double score ;
} Pair ;

typedef struct
{
	int    Count;
	int    Seq;
	int    TotPx;
	CPoint End_P;
} EdgeInfo;

typedef struct
{
	int    Count;
	CPoint S_P;
	CPoint E_P;
} EdgeInfo1;
typedef struct {
				int  nClass1;
				int  nClass2;
				int  nFlag;
				////////////////////
				int nPosition;
				////////////////座標
				int  x, y;
				// sign of Laplacian
				int    nLaplace;
				// descriptor
				double dIvec[64];
				///////////////////
				int    nType;
				int    dist;
				BOOL   status;
				
				int    nType_Amount;
				
				//Matching 輔助分數
				double dScore;
				int    nGroupType;
				double dTheta;
				
				///////////////////
				double dScale;
				// strength of the interest point
				double dStrength;				
				
}SURFINFO;
typedef struct  
{
	int  nClass1;
	int  nClass2;
	int  nFlag;
	char *SourceFileName;
	CPoint P1;
	int    laplance;
	double ivec[64];

}SURFREOCRD;

typedef struct
{
	int  nClass1;
	int  nClass2;

	double hog[1260];
}HOGRECORD;
typedef struct
{
	int  nClass1;
	int  nClass2;
	
	double hog[50];
}VERTRECORD;
typedef struct  
{
	CPoint P1A, P1B, P2A, P2B;
	CPoint Centroid;
	int nType;
	int nType1;
	int nType2;
	
	CPoint pFeature;
	CPoint pFeature1;
	CPoint pFeature2;

	double vFF1;
	double vF1F2;
	double vF2F;

	CPoint pT, pT1;
	int nDistance;
	double dGpTheta; //群方向
	int   nNum;      //群總數
	int  nSquareArea;
	int  nStayTime;
	int  nLifeTime;
	BOOL FLAG;
	BOOL TrackStatus;
	vector<SURFINFO> vGroupPx;
	vector<CPoint> TracePath;
	double         dObjDirection;
	//////////////////
	double dIvec[64];
	
}OBJRANGE;


typedef struct  
{
	double A2A3;
	double A1A2;

}VEHICLE_VECTOR;



typedef struct
{
	CPoint Matchpoint;
	int    Dist_Centroid;
} MatchPoint_Dist;

//////////////////////////////////////////////////////////////////////////
typedef struct{
	int    nCenterPosX;
	CPoint CenterPoint;
	CPoint Point_L;
	CPoint Point_R;
	
}NewMatchPair;
///New DataStructure///////////////////////////////////////////////////////////////////////
typedef struct{
	BOOL BDISPLAY;
	double dTotalS;
	int count;
	CPoint pCenter;
	CPoint Upoint;
	CPoint Dpoint;
	vector<CPoint> MatchPairCenter;
}VerPoint;

typedef struct{
	CPoint LPoint;
	CPoint CenterPoint;
	CPoint RPoint;
	int distance;
}MatcherPair;

typedef struct{
	int counter;
	CPoint pCenter;
	BOOL BDISPLAY;
	BOOL EREASE;
	double dTotalS;
	vector<int>rPoint;
	vector<int>lPoint;
	int uPoint;
	int bPoint;
	int nMaxY;
	int nMinY;
	double dShadowsGradient;
	double dShadowsIntensity;
	int nDistGradient;
	int nDistIntensity;
	int MatchPairSize;
	int *HorizonPeak;
	int PeakTotal;
	int *VerticalPeak;
	int VerticalPeakTotal;
	int VerticalPeakMidIndex;
	double *HistDensityH;
	vector<Segment> SegPos;
	vector<CPoint> MatchPairCenter;
	vector<MatcherPair>MatchPair;
	vector<VerPoint> pVer;
	vector<VerPoint> Top;
// 	vector<CPoint> Candidate_LB;
// 	vector<CPoint> Candidate_RU;
	CPoint Candidate_LB[4];
	CPoint Candidate_RU[4];
	CPoint nShadow[2];
	CPoint Fousc;
}CenterLine;

typedef struct{
	//DataSet點
	CPoint mPoint1;
	//DataBase點
	CPoint mPoint2;
	//二個點的角度
	int nDegree;
	//SURF相似度比對最小距離
	double dSURF_MatchMinDist;
	
}MatchPoint;
typedef struct{
	int  nClass;
	char CarTypeA[100];
	int iYear;
}CARTYPEINFO;

typedef struct{
	//第1部汽車檔名
	char *SourceFileName;
	//第2部汽車檔名
	char *SourceFileName1;
	//內部Matching Parameters
	//selfmatch後中心點座標群
	vector <CPoint> vCenterPoint;
	vector <CPoint> vLeftPoint;
	vector <CPoint> vRightPoint;
	vector<NewMatchPair> vResult;
	//selfmatch後中心點最大與最小y座標
	int nMaxY;
	int nMinY;
	//selfmatch中心線 x position
	int nCenterLinePosX;
	//selfmatch平均角度
	int nSelfMatchDegree;
	//車頭SURFMatch最小平均距離(相似度)
	double dHSURFsimilar;
	double dBSURFsimilar;
	//依據中心線選出的車頭區域(第一張圖)
	//第一輛第一層
	CPoint RU1;
	CPoint LB1;
	//依據中心線選出的車身區域
	//第一輛第二層
	CPoint RU2;
	CPoint LB2;

	//外部Matching Parameters
	int nOutSideMatch;
	//全部配對成功的點
	vector <MatchPoint> vMatchPT;
	//車頭配對成功的點
	vector <MatchPoint> vHeadMatch; 
	//車身配對成功的點(不含車頭)
	vector <MatchPoint> vBodyMatch; 
	//車頭配對成功相似度
	double dHeadSimW;
	double dHeadSimH;
	//第一張重心點
	CPoint Centroid1;
	//第二張重心點
	CPoint Centroid2;
	//車頭重心點
	CPoint HeadCentroid1;
	CPoint HeadCentroid2;
	//車頭重心比對偏移量
	int ndx, ndy;
	//車頭matching點範圍
	CPoint hRU1, hLB1;
	//車頭HistogramW
	int HistogramW[320];
	int HistogramH[240];
	int HistogramW1[70];
	int HistogramH1[30];
	//車頭區域全部SURF點
	vector <CenterLine> Local_Line;
	vector <CPoint> Candidate_LB;
	vector <CPoint> Candidate_RU;
	CPoint cMP1;
	CPoint cMP2;
	CPoint cMP3;
	CPoint cMP4;
	CPoint cMP5;
	//////////////////////////////////////////////////////////////////////////
	double dShadowsIntensity;
	int    nDistIntensity;
	double dShadowsGradient;
	int    nDistGradient;
	int    nDist;
}CARINFO;

typedef struct{
	CPoint LB;
	CPoint RU;
	int height;
	double score;
}CarCandidate;

typedef struct{
	CarCandidate Vehicle[20];
	int VehicleCount;
}DetectedResult;

//////////////////////////////////////////////////////////////////////////
//作積分表
typedef struct{
	CString FoldName;
	//車頭SURFMatch最小平均距離(相似度)
	double dHSURFsimilar;
	double dBSURFsimilar;
	double dNumber_vHead;
	double dNumber_vBody;
	//車頭配對成功相似度
	double dHeadSimW;
	double dHeadSimH;
	BOOL   Flag;
	int    nCount;

}ScoreTbl;

typedef struct
{
	double Hist[9];
}HOG_CELL;
typedef struct
{
	double Hist[36];
}HOG_BLOCK;

// typedef struct{
// 
// 	char	FileName[20];
// 	double  GW[180];
// }Temp;
typedef struct
{
	CPoint Harris;
	int    ClassIdx;
	
}hpoint;
//////////////////////////////////////////////////////////////////////////
typedef struct
{
	CPoint P1, P2;
	double dDistRatio;
	int    nTheta;
	int    ClassIdx;
}VisualVoc;
typedef struct
{
	int nPoxY;
	int nDist;
	BOOL Flag;

} TopPosDist;
typedef struct
{
	int nIndex;
	int nClass;
	double dProbability;
}SVMINFO;

typedef struct
{
	SVMINFO SVMTBL[50];
}SVMINFO_TBL;

typedef struct
{
	vector <SURFINFO> ipts_part;
}SURFVISUALVOC;


typedef struct{
	double HOGTOTAL[1152];
}HOG1152;
typedef struct{
    vector <HOG1152> HOGHIST;
}HOG_VEC;

typedef struct{
	char FileName[100];
}FileNameBin;


// typedef struct{
// 	
// 	//selfmatch中心線 x position
// 	int nCenterLinePosX;
// 	//selfmatch後中心點最大與最小y座標
// 	int nMaxY;
// 	int nMinY;
// 	//車頭範圍
// 	CPoint RU1;
// 	CPoint LB1;	
// 	CPoint RU1old;
// 	CPoint Roof;
// 	//車身範圍
// 	CPoint RU2;
// 	CPoint LB2;
// 	//selfmatch後中心點座標群
// 	vector <CPoint> vCenterPoint;
// 	vector<NewMatchPair> matchpair_bin;
// 	
// 	//////////////////////////////////
// 	double dShadowsIntensity;
// 	int    nDistIntensity;
// 	vector <SURFINFO> surf_ipts;
// 	vector <SURFINFO> surf_ipts_symmetric;
// 	//////////////////////////////////////////////////////////////////////////
// 	vector <int> Ytest;
// 	vector <int> Xtest;
// 	
// 	int nClassIdx;
// 	BOOL Selected;
// 	
// }MY_CARHEADINFO;
// //////////////////////////////////////////////////////////////////////////

typedef struct
{
	int  nX;
	// to centerline
	int  ndist;
	bool bfg;
	int nCenterX;
}IntBool;
typedef struct
{
	int nCenterX;
	int nLeftX;
	int nRightX;
	int ndist;
	int nDiff_LR;
	
	int nUpperY;
	int nBottomY;
	CPoint LB, RU;
	
	int nidx;
	
	float fscore;
	
}CenterDistCandidate;
typedef struct
{
	int YU;
	int YB;
	int ndist;
}Ybin_Cand;
typedef struct{
	
	CPoint Px;
	double   R;
	double   G;
	double   B;
	int    H ;
	double S ;
	double V ;
	BOOL   flag;
	BOOL   flag_H;
	BOOL   flag_S;
	BOOL   flag_V;
	BOOL   flag_zero;
	
	int    ColorLayer;
	int    RGB_color_Histo_No;
	int    RGB_intensity_Histo_No;
	
}PX_COLOR_INFO;

// typedef struct{
// 	MyIpoint PointL;
// 	MyIpoint PointR;
// 	CPoint   CenterPoint;
// 	int      DistL;
// 	int      DistR;
// 	
// }NewMatchSymmetriclPoint;


// typedef struct
// {
// 	vector <MyIpoint> ipts_part;
// }HeadPart_MyIpoint;
typedef struct{
	int nCenterLinPosX;
	vector<NewMatchPair>line_matchpair_bin;
}Car_Candidate;

typedef struct{
	//中心線 x position
	int nCenterLinePosX;
	vector<int>nY_Candidate_bin;
	vector<int>nX_Left_Candidate_bin;
	vector<int>nX_Right_Candidate_bin;

	//車頭範圍
	CPoint RU1 ;
	CPoint LB1 ;

	vector<NewMatchPair>All_MatchPair_bin;
	//New...Multi CenterLine with it's matchpairs......
	vector<Car_Candidate>Car_Candidate_bin;
}CARHEADINFO;

//////////////////////////////////////////////////////////////////////////
class N_PsKeyPointVectInsertor : public N_parallelsurf::N_KeyPointInsertor
{
public:
	N_PsKeyPointVectInsertor(std::vector<N_parallelsurf::N_KeyPoint>& keyPoints) : m_KeyPoints(keyPoints) {};
	inline virtual void operator() (const N_parallelsurf::N_KeyPoint &keyPoint)
	{
		m_KeyPoints.push_back(keyPoint);
	}

private:
	std::vector<N_parallelsurf::N_KeyPoint>& m_KeyPoints;
};

class CRogerFunction : public CDocument
{
protected:

	DECLARE_DYNCREATE(CRogerFunction)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRogerFunction)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:

	void CRogerFunction::I_GrayLevel(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw);
    void CRogerFunction::I_EdgeDetection(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw);
    void CRogerFunction::I_Erosion(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
    void CRogerFunction::I_Dilation(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);	
    void CRogerFunction::I_GaussianSmooth3(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw, double in_sigma);
    void CRogerFunction::I_GaussianSmooth1(BYTE *in_image, BYTE *out_image, int s_w, int s_h, double in_sigma);


	void CRogerFunction::I_SkinExtraction(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_SkinExtractionBW(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_ConnectedComponent(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_AutoThreshold(BYTE *in_image, BYTE *out_image, int s_w, int s_h); 
	void CRogerFunction::Gray2Gradient(BYTE *Grayimage, int *xgradient,int *ygradient, int *gradient_image, int s_w, int s_h, int diff_para );

	void CRogerFunction::CarDetection( CARHEADINFO &Car, BYTE *pData, int nWidth, int nHeight, int nhWidth, int nhHeight, int nhEffw, int nhPixel, int nhSize, int FrameCnt, BOOL &bRecognize );
//	void CRogerFunction::MYSURF_SURF_GetIntegral(double *in_image, vector< MyIpoint > &myipts, int s_w, int s_h, float dThres, float Tiny, int &VLength, bool bUpright );
// 	void CRogerFunction::MYSURF_SURF_GetFromIntegral(double *IntegralGradient, vector< MyIpoint > &myipts, int s_w, int s_h, float dThres, float Tiny, int &VLength, bool bUpright );
// 	void CRogerFunction::MySURF_MatTransLR(float *ivec , float *Array_Out );
	void CRogerFunction::HORILINE( CARHEADINFO &Car, int *In_ImageNpixel, int s_w, int s_h, int s_bpp, BYTE *output, int nWidth, int nHeight);
	void CRogerFunction::Integral_Image_Use(double *IImage, int s_w, CPoint LB, CPoint RU, float &Result);
//	void CRogerFunction::HessianMatrixNew( float *IImage, int s_w, int s_h, IpVec &ipts);
// 	void CRogerFunction::HessianMatrixNew(double *IImage, int s_w, int s_h, IpVec &ipts);
	void CRogerFunction::Color2Gradient(BYTE *in_colorimage, int *gradient_image, int s_w, int s_h, int s_Effw, int diff_para );

	void CRogerFunction::I_TraceEdge (CPoint &StartPoint, CPoint &EndPoint, BYTE *pUnchEdge, int nWidth, int nHeight, int &PNum);
	//輸入起始座標,傳回結束座標,追蹤多個終點
	void CRogerFunction::I_TraceEdgeMultiEndPoint (CPoint &StartPoint, vector<EdgeInfo> *vi, BYTE *pUnchEdge, int nWidth, int nHeight, int &PNum, int &Seq, int &TotPx);
	//輸入起始座標,傳回結束座標圖像點,追蹤多個終點
	void CRogerFunction::I_TraceEdge (CPoint &StartPoint, CPoint &EndPoint, BYTE *pUnchEdge, BYTE *out_image, int nWidth, int nHeight, int &PNum);
	void CRogerFunction::I_DrawCircle(BYTE *pImage, int Image_W, int Image_H, int Image_EFFW, CPoint CrossPoint, int CrossSize, COLORREF LineColor);
	void CRogerFunction::I_DrawCircleEffw(BYTE *pImage, int Image_W, int Image_H, int Image_Effw, CPoint CrossPoint, int CrossSize, COLORREF LineColor);
	
	void CRogerFunction::I_DrawCircleNew(BYTE *pImage, int Image_W, int Image_H, int Image_Effw, CARINFO &CarDetectedA, CPoint CrossPoint, int CrossSize, COLORREF LineColor);

	// 	void CRogerFunction::TraceEdge (CPoint StartPoint, BYTE *pUnchEdge, int nWidth, int nHeight, int Class, int &PNum);
	void CRogerFunction::I_CANNY(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw, double in_sigma=0.1, double in_Th_ratio=0.8, double in_Tl_ratio=0.6);
	void CRogerFunction::I_Thinner(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_Negative(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_SkinExtract(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_Contour(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_HoughLine(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_HoughCircle(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_Equalization(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_TriAngle(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_CopyBuffer123(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_CopyBuffer321(BYTE *in_image, BYTE *out_image, int *edgemap, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_CopyBuffer123(int *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);


	void CRogerFunction::I_DOG(BYTE *in_image, BYTE * out_image, int s_w, int s_h, double Delta);	
	void CRogerFunction::I_ColorCarExtraction(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_GradMagOut1(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_GradMagOut1_360(BYTE *in_image, BYTE *out_image, int *outDirect_image, int s_w, int s_h);
	void CRogerFunction::I_GradMagOut1_180(BYTE *in_image, BYTE *out_image, int *outDirect_image, int s_w, int s_h);
	void CRogerFunction::I_GradMagOut1_180(BYTE *in_image, BYTE *out_image, int *outDirect_image, int s_w, int s_h, int Dimemsion); //以一個 byte輸入 輸出
	void CRogerFunction::MatTransLR( double *ivec , double *Array_Out );
	void CRogerFunction::MatTransUB( double *ivec , double *Array_Out );
	void CRogerFunction::MatTransLRUB( double *ivec , double *Array_Out );

	
	void CRogerFunction::I_HistogramSCR( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp);
	void CRogerFunction::I_HistogramSCR00( vector <SURFINFO> &vMatch_TBL, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp);
	void CRogerFunction::I_HistogramSCRData( BYTE *In_ImageNpixel, double *GramW, int s_w, double *GramH, int s_h, double *GramWout, int s_wout, double *GramHout, int s_hout);
	
	void CRogerFunction::I_Histogram1D( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax);
	void CRogerFunction::I_Histogram1DMax( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax);
	void CRogerFunction::I_Histogram1DMax1( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax);	
	void CRogerFunction::I_Histogram1DMax2( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax);
	void CRogerFunction::I_Histogram1DMaxNEW( BYTE *Display_ImageNsize, SelfINFO &CenterPoint , int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH);

	void CRogerFunction::I_Histogram1DPositon( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int CenterPosX , int s_w, int s_h, int s_bpp);
// 	void CRogerFunction::I_ipts2vIT_TBL( vector<Ipoint> &ipts, vector<SURFINFO>&vIT_TBL);
// 	void CRogerFunction::I_ipts2vIT_TBL( Ipoint &ipts, SURFINFO &vIT);
// 	void CRogerFunction::I_ipts2vIT_TBL( Ipoint &ipts, SURFINFO &vIT, double *symmetric );
	
	void CRogerFunction::Histogram1DPositonH( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int CenterPosX , int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH );
	void CRogerFunction::Histogram1DPositonH_SIM( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int CenterPosX , int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH ) ;
	void CRogerFunction::Histogram1DPositonH_SIMNEW( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, SelfINFO &CenterLine , int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH ) ;
	void CRogerFunction::Histogram1DPositonH_SIMNEW( MatchRectangle &Range, int *HistogramW, int *HistogramH, BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, SelfINFO &CenterLine , int s_w, int s_h, int s_bpp, vector <LocalMaxLine> &Local_Line, BOOL DISPLAY_SWITCH ) ;

	void CRogerFunction::Histogram1DPositonW( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, LocalMaxLine &LocalLine1, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH) ;
	void CRogerFunction::Histogram1DPositonW_Down( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, LocalMaxLine &LocalLine1, int FindDist, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH) ;
	void CRogerFunction::Histogram1DPositonW_NEW( BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, LocalMaxLine &LocalLine1,int nFindDist,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH) ;
	
	void CRogerFunction::DISPLAY_RANGE( BYTE *Display_ImageNsize, vector <LocalMaxLine> &Local_Line, int s_w, int s_h, int s_bpp);

	void CRogerFunction::I_GradDirectionOut(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);	
	void CRogerFunction::I_RGB2C1C2C3(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_RGB2m1m2m3(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_RGBNormalization(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_LowPassFilter(BYTE *in_image, BYTE *out_image, int s_w, int s_h);
	void CRogerFunction::I_HighPassFilter(BYTE *in_image, BYTE *out_image, int s_w, int s_h);	
	void CRogerFunction::I_ShadeEnhance(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_ShadeDet(BYTE *in_image1, BYTE *in_image2, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_ShadeReduce(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_ShadeReduce01(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_ShadeReduce02(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_BubbleSort(int *DataList, int Num);
	void CRogerFunction::I_BubbleSort(double *DataList, int Num);

	void CRogerFunction::I_quicksort(int *data, int first, int last, int size);

	void CRogerFunction::I_Sobel(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int Threshold = 200);

	//...SVM
	double CRogerFunction::I_SVM_Classifier( char *line , struct svm_model* model );
	double CRogerFunction::I_SVM_Classifier(  double *dIvec, struct svm_model* model);
	double CRogerFunction::I_SVM_Classifier(  double *dIvec, struct svm_model* model, int nLength );

	double CRogerFunction::I_SVM_Classifier(  double *dIvec, struct svm_model* model, int nLength, SVMINFO_TBL &SVM_INFO );

	void CRogerFunction::I_HOG_Descriptor_Mag( BYTE *in_image, char *line_in); //灰階 16 x 16 Image
	void CRogerFunction::I_ROI(BYTE *in_image, int s_w, int s_h, int s_BPP, CPoint UL, BYTE *out_image, int d_w, int d_h);
	void CRogerFunction::I_ROIRAW(BYTE *in_image, int s_w, int s_h, CPoint UL, BYTE *out_image, int d_w, int d_h);
	void CRogerFunction::I_swap(int *a, int *b);                     //把a跟b指向的位置的值交換 
	void CRogerFunction::I_FindHCP_MaskNew( BYTE *in_image, BYTE *out_image, int s_Width, int s_Height); //輸入CANNY後之影像 nPixel大小

	void CRogerFunction::I_GradDirectionMap(BYTE *in_image, float *out_image, int s_w, int s_h);	
	void CRogerFunction::I_CANNY_Direct(BYTE *ImgSrc, BYTE *out_image, int *outDirect_image, int s_w, int s_h, double in_sigma, double in_Th_ratio, double in_Tl_ratio);
	void CRogerFunction::I_CANNY_Direct_360(BYTE *ImgSrc, BYTE *out_image, int *outDirect_image, int s_w, int s_h, double in_sigma, double in_Th_ratio, double in_Tl_ratio);
	void CRogerFunction::I_STIPLocalDirection(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);
	void CRogerFunction::I_STIP(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);
	void CRogerFunction::I_OpticalFlow(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);
	void CRogerFunction::I_OpticalFlow(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, vector<ObjectInformation> *vi, int Corner_Threshold, int OF_Threshold, double Sigma);

	void CRogerFunction::I_OutputGraph(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);

	void CRogerFunction::I_HarrisCorner(BYTE *pImage, int s_w, int s_h, int Threshold, double Sigma, int Type = 0); //Type=0圓, Type=1"+"
	void CRogerFunction::I_HarrisCorner_V(BYTE *pImage, vector<CPoint> *vi, int s_w, int s_h, int Threshold, double Sigma); 
	void CRogerFunction::I_HarrisCorner_V(BYTE *pImage, vector<CPoint> *vi, CPoint RestrictLB, CPoint RestrictRU, int s_w, int s_h, int Threshold, double Sigma);
	
	void CRogerFunction::I_HarrisCorner_Eigen(BYTE *pImage, int s_w, int s_h, int Threshold, double Sigma, int Type);

	
	void CRogerFunction::I_HARRISDirectionEdge(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);
	void CRogerFunction::I_HARRISDirectionEdge1(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);
	void CRogerFunction::I_HARRISDirectionEdge2(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);
	//輸入Canny 結果到 CannyMap
	void CRogerFunction::I_HARRISDirectionEdge3(BYTE *CannyMap, BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, int WindowSize);

	//*****Mathematics
	void CRogerFunction::I_EigenValue_3x3(double A[EigenSize3x3][EigenSize3x3], double &EigenValue);
	void CRogerFunction::I_EigenValue_2x2(double A[EigenSize2x2][EigenSize2x2], double &EigenValue1, double &EigenValue2);
	
	//************************************************************
	void CRogerFunction::I_DerivativeX(double *in_image, double *out_image, int s_w, int s_h);
	void CRogerFunction::I_DerivativeY(double *in_image, double *out_image, int s_w, int s_h);
	void CRogerFunction::I_DerivativeT(double *in_image0, double *in_image1, double *in_image2, double *out_image, int s_w, int s_h);
    void CRogerFunction::I_GaussianSmoothDBL(double *in_image, double *out_image, int s_w, int s_h, double in_sigma);
	void CRogerFunction::I_GaussianSmoothDBL_STIP(double *in_image, double *out_image, int s_w, int s_h, double in_sigma, double T_interval);
	void CRogerFunction::I_TraceEdge (int y, int x, int nLowThd, BYTE *pUnchEdge, int *pnMag, int nWidth); 
    void CRogerFunction::I_Gaussian(double in_sigma);

	void CRogerFunction::I_FindHCP_MaskNew( BYTE *in_image, CPoint &CenterPoint, int Radius, int *Hist, int s_w, int s_h); //輸入CANNY後之影像 nPixel大小
    void CRogerFunction::I_Correlation(int *Hist, int *Hist1, int Hist_Size, double &Score);
	void CRogerFunction::I_Correlation(double *Hist, double *Hist1, int Hist_Size, double &Score);
	
	void CRogerFunction::DISPLAY_RANGE_BESTSIMILARITY( BYTE *Display_ImageNsize, vector <LocalMaxLine> &Local_Line, int s_w, int s_h, int s_bpp);

	void CRogerFunction::I_GrayLevel3_1(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int s_Effw);
	void CRogerFunction::I_HarrisCorner(BYTE *in_image, BYTE * out_image, int s_w, int s_h, int Threshold, double Sigma);
	void CRogerFunction::I_CANNY_GRAY(BYTE *in_image, BYTE * out_image, int s_w, int s_h, double in_sigma=0.1, double in_Th_ratio=0.8, double in_Tl_ratio=0.6);
	void CRogerFunction::I_OpticalFlowDir(BYTE *T00, BYTE *T01, BYTE *T02, BYTE *out_Image, int s_w, int s_h, int Threshold, double Sigma);
	void CRogerFunction::I_OpticalFlow2(BYTE *T00, BYTE *T01, BYTE *T02, int *out_Image, int s_w, int s_h, int s_t, int Threshold, double Sigma);
	void CRogerFunction::I_FindHCP_MaskNewTest( BYTE *in_image, CPoint &CenterPoint, int Radius, int *Hist, int s_w, int s_h);
// 	void CRogerFunction::I_SURF( BYTE *in_image, vector< Ipoint > &ipts, int s_w, int s_h, double dThres, int &VLength );
// 	void CRogerFunction::I_SURF1( BYTE *in_image, vector< Ipoint > &ipts, int s_w, int s_h, double dThres, int Tiny, int &VLength, bool DoubleSize );
// 	void CRogerFunction::I_SURF_ROI( BYTE *in_image, vector< Ipoint > &ipts, int s_w, int s_h, CPoint LB, CPoint RU, double dThres, int Tiny, int &VLength, bool DoubleSize );

	///KMEANS START
	void CRogerFunction::KMEANS( DataType *DataMember, int DataNum, int Dimension, int K);
	void CRogerFunction::NewCenterPlus(ClusterType *p1,int t,double *p2,int dim);
	void CRogerFunction::NewCenterReduce(ClusterType *p1,int t,double *p2,int dim);
	void CRogerFunction::GetValue(double *str1,double *str2,int dim);
	int  CRogerFunction::FindFather(double *p,int k);
	double CRogerFunction::SquareDistance(double *str1,double *str2,int dim);
	int	CRogerFunction::I_Compare(double *p1,double *p2,int dim);
	///KMEAN END	
	void CRogerFunction::I_ResampleRAW(double *InputBuf, int Width, int Height, double *OutputBuf, int ReWidth, int ReHeight);
	void CRogerFunction::I_DrawRange1(BYTE *Display_image, CPoint RU, CPoint RB, CPoint LU, CPoint LB, int s_w, int s_h, int s_bpp, COLORREF LineColor);
	void CRogerFunction::I_DrawRange2(BYTE *Display_image, CPoint P1, CPoint P2, int s_w, int s_h, int s_bpp, COLORREF LineColor);
	void CRogerFunction::I_RangePx( vector<OBJRANGE> &vVehicle, vector<SURFINFO> &vIT_TBL, BYTE *Display_image, int s_w, int s_h, int s_bpp, COLORREF LineColor);
	void CRogerFunction::I_DrawPxLine( BYTE *in_image, CPoint SPx, double dTheta, int s_w, int s_h, int s_bpp, int nLineLength, COLORREF LineColorS, COLORREF LineColorE );

		void CRogerFunction::I_SaveImage( BYTE *SaveImage, int s_w, int s_h, int s_effw, int &FrameCnt , char *pathC);
		void CRogerFunction::I_SavePatch( BYTE *in_image, int s_w, int s_h, vector<SURFINFO> &vIT_TBL, int &FrameCnt, char *pathC );
		void CRogerFunction::I_SavePatch( BYTE *in_image, int s_w, int s_h, CPoint LB, CPoint RU, char *FileName );
	
		void CRogerFunction::FindSymmetryAxis(BYTE *input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2);

		void CRogerFunction::HistMaxLine( CARINFO &CarDetected, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH);
		void CRogerFunction::HistMaxLine_many(BYTE *input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH);
		void CRogerFunction::HistMaxLine_many1(BYTE *input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH);
		void CRogerFunction::HistMaxLine_video(BYTE *input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH);
		void CRogerFunction::HistMaxLine_video1(BYTE * input, CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, int Width, int Height, CPoint LB, BOOL DISPLAY_SWITCH1, BOOL DISPLAY_SWITCH2, BOOL TEST_SWITCH, BOOL USING_ROI, BOOL USINGSMALLFRAME);

		void CRogerFunction::HistVERTLINE( CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH, BOOL DISPLAY_SWITCH1 ) ;
		void CRogerFunction::HistVERTLINE( CARINFO &CarDetected, BYTE *In_ImageNpixel, BYTE *In_ImageNpixel2, int *In_DirectionMap, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH, BOOL DISPLAY_SWITCH1 ); 
		void CRogerFunction::HistVERTLINE_NEW( CARINFO &CarDetected, double *GW1, double *GW2, double *GW3, BYTE *In_ImageNpixel, BYTE *Display_ImageNsize, int s_w, int s_h, int s_bpp, BOOL DISPLAY_SWITCH );
		void CRogerFunction::HOG8x8( HOG_CELL *CELL, HOG_CELL *DELL, HOG_BLOCK *BLOCK, BYTE *In_ImageNpixel, int s_w, int s_h );
		void CRogerFunction::HOG_ROI( CPoint LineUP1, CPoint LineBT1, BYTE *In_ImageNpixel1, int s_w1, int s_h1, CPoint LineUP2, CPoint LineBT2, BYTE *In_ImageNpixel2, int s_w2, int s_h2 );
		void CRogerFunction::HOG_ROI( double &dS1, double &dS2, double &dS3, double &dS4, CPoint LineUP1, CPoint LineBT1, BYTE *In_ImageNpixel1, int s_w1, int s_h1, CPoint LineUP2, CPoint LineBT2, BYTE *In_ImageNpixel2, int s_w2, int s_h2 );
		
		void CRogerFunction::HOG_ROI_FILEOUT( double *ACELL1, CPoint LineUP1, CPoint LineBT1, BYTE *In_ImageNpixel1, int s_w1, int s_h1 );
		
//		void CRogerFunction::DiffImageMatch( vector< Ipoint > &ipts1, vector< Ipoint > &ipts2, CARINFO &CarDetected, BYTE *Display_ImageNsize,  int s_w, int s_h, int s_bpp, int OffSet, BOOL DISPLAY_SWITCH);
		void CRogerFunction::I_FindDist( double *Hist1, double *Hist2, int Hist_Size, double &dist );
		
		void CRogerFunction::FileTitle();
		void CRogerFunction::Analysis_CarDetected( CARINFO &CarDetected, CARINFO &CarDataBase, BYTE *Display_ImageNsize, int s_w, int s_h, int Offset, BOOL DISPLAY_SWITCH );
		void CRogerFunction::FindCircleHOG( BYTE *In_ImageNpixel, int s_w, int s_h, CPoint Centroid, int Radius, double *GW );
		void CRogerFunction::FindCircleHOGNew1( BYTE *In_ImageNpixel, int s_w, int s_h, CPoint Centroid, int Radius, int nDegreeStart, int nDegreeEnd, double *GW );

		double CRogerFunction::GWSVM_Classifier(  double *GW, struct svm_model* model);
		double CRogerFunction::HOGSVM_Classifier(  double *dACELL, struct svm_model* model);
		//////////////////////////////////////////////////////////////////////////
		double CRogerFunction::ROI_HORIZONSCAN(CARINFO &CarDetectedA, CARINFO &CarDetectedB, BYTE *In_ROIGradientA, BYTE *In_ROIGradientB, int s_wA, int s_hA, int s_wB, int s_hB);
		double CRogerFunction::ROI_VERTICALSCAN( CARINFO &CarDetectedA, CARINFO &CarDetectedB, BYTE *In_ROIGradientA, BYTE *In_ROIGradientB, int s_wA, int s_hA, int s_wB, int s_hB );
		void CRogerFunction::ROI_VERTICALSCAN( CARINFO &CarDetectedA, BYTE *In_ROIGradientA, int s_wA, int s_hA, double *Hist1  );
		
//		void CRogerFunction::ROI_ADJUST( CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector< Ipoint > &ipts1, vector< Ipoint > &ipts2, 	vector<NewMatchPair> &Result1 );
		double CRogerFunction::ROI_FIVEDIRECTION( CARINFO &CarDetectedA, CARINFO &CarDetectedB, int nColumn, int nRow, int *DirectionMap1, int s_wA, int s_hA, int *DirectionMap2, int s_wB, int s_hB );
		double CRogerFunction::ROI_FIVEDIRECTION( CARINFO &CarDetectedA, CARINFO &CarDetectedB, int nColumn, int nRow, BYTE *In_Image1, int s_wA, int s_hA, BYTE *In_Image2, int s_wB, int s_hB );
		void CRogerFunction::ROI_FIVEDIRECT( CARINFO &CarDetectedA, int nColumn, int nRow, int *DirectionMap1, int s_wA, int s_hA, double *TotalA );

		void CRogerFunction::HarrisFindSymmetric( vector<CPoint> &vHarrisA, vector<CPoint> &vHarrisB, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet );
		void CRogerFunction::DrawGrid_Encode( vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet, int nColumn, int nRow, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW, 	vector <MatchCorner> &MCornerBin, BYTE *Display_ImageNsizeA, int s_wA, int s_hA, BYTE *Display_ImageNsizeB, int s_wB, int s_hB, int nDistThreshold  );
		//////////////////////////////////////////////////////////////////////////
		void CRogerFunction::HOGSIM( CARINFO &CarDetectedA, BYTE *In_ROIGradientA, int *DirectionMap1, int s_wA, int s_hA, double *SimGram );
//		void CRogerFunction::ROI_ADJUST2( CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector< Ipoint > &ipts1, vector< Ipoint > &ipts2, vector<NewMatchPair> &Result1 );
		void CRogerFunction::DrawGrid_Encode2( vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet, int nColumn, int nRow, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW,  BYTE *Display_ImageNsizeA, int s_wA, int s_hA, BYTE *Display_ImageNsizeB, int s_wB, int s_hB, int nDistThreshold  );
		void CRogerFunction::DrawGrid_Encode3( vector <CPoint> &vHarrisAsymmet, vector <CPoint> &vHarrisBsymmet, int nColumn, int nRow, CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW,  BYTE *Display_ImageNsizeA, int s_wA, int s_hA, BYTE *Display_ImageNsizeB, int s_wB, int s_hB, int nDistThreshold  );
		void CRogerFunction::DrawGrid(  CARINFO &CarDetectedA, int nColumn, int nRow, BYTE *Display_ImageNsizeA, int s_wA, int s_hA  );
		void CRogerFunction::DrawGrid(  CARHEADINFO &CarDetectedA, int nColumn, int nRow, BYTE *Display_ImageNsizeA, int s_wA, int s_hA  );
	
		double CRogerFunction::NewDescriptor( vector <hpoint> &vHarrisIdxANEW, vector <hpoint> &vHarrisIdxBNEW, CPoint CenterA, CPoint CenterB, double &dDist );
		double CRogerFunction::FindROIHOG( CPoint LBA, CPoint RUA, CPoint LBB, CPoint RUB , CARINFO &CarDetectedA, CARINFO &CarDetectedB, vector<NewMatchPair> &Result1, BYTE *ImagepxA, int s_wA, BYTE *ImagepxB, int s_wB );
		//Largest Common Sequence
		void CRogerFunction::LargestCommonSubsequence( vector <char> &vStr1, vector <char> &vStr2, vector <int> &vResult );
		void CRogerFunction::LCS(  int i, int j, int &n, char *b, vector<char> &x, int &nStrLen, vector <int> &vResult );
		//function combination

		void CRogerFunction::VERTLINE( CARHEADINFO &CarDetected, int *In_ImageNpixel, int s_w, int s_h, int s_bpp ) ;
		void CRogerFunction::MaxLine( CARHEADINFO &CarDetected, int s_w, int s_h );
		void CRogerFunction::NewHogDescriptor( CARHEADINFO &Car, BYTE *In_ImageNpixel, int s_w, int s_h, double *GWA, double *GWA1, double *GWA2, double *TotA );
		void CRogerFunction::elipse_modify( double *GW , double in_sigma );
		void CRogerFunction::SURF_Correlation( SURFINFO &vIT1, SURFINFO &vIT2, double &dist );
		void CRogerFunction::TwoCarHeadSURF_Match( vector <CARHEADINFO> &Car_BIN1 , vector <CARHEADINFO> &Car_BIN2, double &Match_Amount );

		void CRogerFunction::Color2GradientHOG(BYTE *in_image, BYTE *out_image, int *DirectionMap, int s_w, int s_h, int s_Effw);
		void CRogerFunction::TwoBinSURFMatch( vector <SURFINFO> &vIT1, vector <SURFINFO> &vIT2, int &Number );
		void CRogerFunction::HOG_correlation( vector <HOG1152> &HOGHISTi, vector <HOG1152> &HOGHISTj, double &dScore );

		BOOL CRogerFunction::WriteDirectory(CString dd);

		void CRogerFunction::Integral_Image_Create( BYTE *In_GrayImage, int nWidth, int nHeight, double *IntegralImage, int integralw );
		void CRogerFunction::Integral_Image_CreateNew( BYTE *In_Image, int nWidth, int nHeight, double *IntegralImage );
		void CRogerFunction::I_CopyBuffer321(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
		void CRogerFunction::I_CopyBuffer321(BYTE *in_image, float *out_image, int s_w, int s_h, int s_Effw);
		void CRogerFunction::DrawCircle(BYTE *pImage, int Image_W, int Image_H, int Image_BPP, CPoint CrossPoint, int CrossSize, COLORREF LineColor);
		void CRogerFunction::MYHistogram1D( BYTE *Display_ImageNsize, vector<CPoint>CenterP , int s_w, int s_h, int s_bpp, vector<int> &XPosMax);
// 		void CRogerFunction::MySURF_Extract( float *grayimage, int nWidth, int nHeight, int nPixel, float thres, MyIpVec &myipts0  );

		void CRogerFunction::Color2Gradient7(BYTE *in_grayimage, float *gradient_image, int s_w, int s_h, int s_Effw );
// 		void CRogerFunction::MY_SURF1( float *grayimage, int nWidth, int nHeight, int nPixel, float thres, MyIpVec &myipts0  );
// 		void CRogerFunction::MY_SURF1( float *grayimage, int nWidth, int nHeight, int nPixel, float thres, MyIpVec &myipts0, float *integralimage  );

// 		void CRogerFunction::MY_SelfSURFMatch16(  MyIpVec &ipts1, CARHEADINFO &CarDetected, int s_w, int s_h );
		void CRogerFunction::MY_DrawGrid( CARHEADINFO &CarDetectedA, int nColumn, int nRow, BYTE *Display_ImageNsizeA, int s_wA, int s_hA  );
// 		void CRogerFunction::MY_ipts2vIT_TBL( MyIpoint &ipts, SURFINFO &vIT);

// 		void CRogerFunction::SymmetricalSURF00( MyIpVec myipts0, vector<NewMatchPair>&matchpair_bin );
		void CRogerFunction::Color2Gradient7( float *in_grayimage, float *gradient_image, int s_w, int s_h, int s_Effw );
// 		void CRogerFunction::MySURF_Extract( float *grayimage, int nWidth, int nHeight, int nPixel, float thres, MyIpVec &myipts0, float *integralimage, char *imageData  );
		void CRogerFunction::I_Histogram1D( BYTE *Display_ImageNsize, vector<NewMatchPair>&matchpair_bin , int s_w, int s_h, int s_bpp, vector<int> &XPosMax);
		void CRogerFunction::DivideSubRegion_New( BYTE *DisplayImage, float *integralimage, vector<int> &CenterPosX, int nWidth, int nHeight, vector<int> &Posoutput_bin, bool H, int Ext_No, BOOL DISPLAY_SW );
		void CRogerFunction::DivideSubRegion_New_UnSymmet( BYTE *DisplayImage, float *integralimage, vector<int> &CenterPosX, int nWidth, int nHeight, vector<int> &Posoutput_bin, bool H, int Ext_No, float fRL_ratio, BOOL DISPLAY_SW );
		void CRogerFunction::headcand_New( 	vector<IntBool> &X_R_bin, vector<IntBool> &X_L_bin, vector <int> &CenterPosX, vector<int> &Y_bin, vector<CenterDistCandidate>&CenterDistCandidateNew_bin, BYTE *DisplayImage, float *integralimage, int s_w, int s_h, BOOL DISPLAY_SW );
		void CRogerFunction::headcand_New( CARHEADINFO &Car,	vector<IntBool> &X_R_bin, vector<IntBool> &X_L_bin, vector <int> &CenterPosX, vector<int> &Y_bin, BYTE *DisplayImage, float *integralimage, int s_w, int s_h, float fLR_ratio, BOOL DISPLAY_SW );

		void CRogerFunction::I_SavePatch( BYTE *in_image, int s_w, int s_h, CPoint LB, CPoint RU, char *FileName, BOOL CROSS_DISPLAY, BOOL RESIZE );
		inline float CRogerFunction::Integral_Image_UseNew( float *IntegralImage, int s_w, int s_h, CPoint LB, CPoint RU );
		void CRogerFunction::BubbleSort(float *DataList, int Num);
		
		void CRogerFunction::HSV_DISPLAY_NEW4( PX_COLOR_INFO *ROIPXINFO, int ROIW, int ROIH, int ROIWH, BYTE *HSV_Domain_Display, int s_w, int s_h, int s_effw, int &nH_Group_Number, int &nH, int &nS, int &nV, int nInterval, int &group, double *ColorHistogram, BOOL DISPLAY  );
		
		void CRogerFunction::HSV2RGB( int H, double S, double V, int &R, int &G, int &B );
		void CRogerFunction::RGB2HSV( BYTE iR, BYTE iG, BYTE iB, int &H, double &S, double &V );
		void CRogerFunction::RGB2HSV( double iR, double iG, double iB, int &H, double &S, double &V );

		void CRogerFunction::CarRecognize( CARHEADINFO &Car, BYTE *input, BYTE *output, BYTE *pBufferA1_1, int nWidth, int nHeight, int nEffw, int nBpp, int thres, int TypeNo, vector <struct svm_model*> &mysvm_model_pointBin, vector<CARTYPEINFO> &NewCarTypeBIN, int *Class2Index, int &nConclusion, float &fProb );
		void CRogerFunction::CarRecognize_from_HeadCandidate( CARHEADINFO &Car, CPoint HeadLB, CPoint HeadRU, BYTE *input, BYTE *output, BYTE *pBufferA1_1, int nWidth, int nHeight, int nEffw, int nBpp, int thres, int TypeNo, vector <struct svm_model*> &mysvm_model_pointBin, vector<CARTYPEINFO> &NewCarTypeBIN, int *Class2Index, int &nConclusion, float &fProb, int &nFeatureNumber );
		void CRogerFunction::Gray2Gradient(BYTE *in_grayimage, BYTE *ROI_GRADIENT, int *gradient_image, int s_w, int s_h, int diff_para);
		void CRogerFunction::MP_Color2Gradient(BYTE *in_image, BYTE *out_image, int s_w, int s_h, int s_Effw);
		void CRogerFunction::Parallel_SURF_MatTransLR(vector<double> &ivec, double *Array_Out);

		void CRogerFunction::CarDifferentView_Center( CARHEADINFO &Car, BYTE *input, BYTE *output, BYTE *pBuffer1_1, float *integralimage, int nWidth, int nHeight, int nBpp, int nEffw, int thres, int TypeNo, vector <struct svm_model*> &mysvm_model_pointBin, vector<CARTYPEINFO> &NewCarTypeBIN, int *Class2Index, float fLR_ratio, int &nConclusion, float &fProb );

		bool CRogerFunction::PtInCircle(CPoint P1, CPoint P2, int Distance);
		void CRogerFunction::SaveImage(BYTE *input, int nHeight, int nWidth, int nBpp,CPoint LB, CPoint RU, CString filename);
		void CRogerFunction::I_SaveImage(BYTE *SaveImage, int s_w, int s_h, int s_effw, char *SourceFileName);
		void CRogerFunction::TestAdaboost(BYTE *input, CARHEADINFO &CarDetectedA, int nHeight, int nWidth, int nBpp,  CxImage Image1, int frameCnt);

		void CRogerFunction::Parallel_SURF_Extract(BYTE *ROI_GRADIENT, int _width, int _height, vector<N_parallelsurf::N_KeyPoint> &keyPoints);
		void CRogerFunction::Symmetric_Parallel_SURF_Match(vector<N_parallelsurf::N_KeyPoint> &keyPoints, vector<NewMatchPair> &matchpair_bin);
		void CRogerFunction::Find_Center_Line(vector<NewMatchPair> &matchpair_bin, vector <int> &CenterPosX, int _width, int VehicleWidth, double *GramW, double *SmoothingW);
		void CRogerFunction::Find_Y_Position(int best_xposition, vector <int> &CenterPosY, double *dROI_GRADIENT_IntegralImage, double *dROI_GRAY_IntegralImage, double *GramW, double *ShadowHistogram, double *SmoothingW, int _width, int _height, int VehicleWidth, int VehicleHeight);
		void CRogerFunction::SaveColorImage(BYTE *SaveImage, int s_w, int s_h, int s_effw, char *FileName);
		void CRogerFunction::SaveGrayImage(BYTE *SaveImage, int s_w, int s_h, int s_effw, char *FileName);


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	
		// Generated message map functions
protected:
	//{{AFX_MSG(CRogerFunction)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ROGERFUNCTION_H__FCFC7E33_54C4_4427_BFC0_11B2962BA6E0__INCLUDED_)
extern Img* imread(Img* I, const char* fileName);
