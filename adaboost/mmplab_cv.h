#ifndef _CV_INTERNAL_H_
#define _CV_INTERNAL_H_

#if defined _MSC_VER && _MSC_VER >= 1200
/* disable warnings related to inline functions */
#pragma warning( disable: 4711 4710 4514 )
#endif

#include "cv.h"
#include "cxmisc.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>

typedef unsigned char uchar;
typedef unsigned short ushort;

#ifdef __BORLANDC__
#define     WIN32
#define     CV_DLL
#undef      _CV_ALWAYS_PROFILE_
#define     _CV_ALWAYS_NO_PROFILE_
#endif

/* helper tables */
extern const uchar icvSaturate8u_cv[];
#define CV_FAST_CAST_8U(t)  (assert(-256 <= (t) || (t) <= 512), icvSaturate8u_cv[(t)+256])
#define CV_CALC_MIN_8U(a,b) (a) -= CV_FAST_CAST_8U((a) - (b))
#define CV_CALC_MAX_8U(a,b) (a) += CV_FAST_CAST_8U((b) - (a))

// -256.f ... 511.f
extern const float icv8x32fTab_cv[];
#define CV_8TO32F(x)  icv8x32fTab_cv[(x)+256]

// (-128.f)^2 ... (255.f)^2
extern const float icv8x32fSqrTab[];
#define CV_8TO32F_SQR(x)  icv8x32fSqrTab[(x)+128]

CV_INLINE  CvDataType icvDepthToDataType( int type );
CV_INLINE  CvDataType icvDepthToDataType( int type )
{
    return (CvDataType)(
		((((int)cv8u)|((int)cv8s << 4)|((int)cv16u << 8)|
		((int)cv16s << 12)|((int)cv32s << 16)|((int)cv32f << 20)|
		((int)cv64f << 24)) >> CV_MAT_DEPTH(type)*4) & 15);
}

#define CV_HIST_DEFAULT_TYPE CV_32F

CV_EXTERN_C_FUNCPTR( void (CV_CDECL * CvWriteNodeFunction)(void* seq,void* node) )

#define _CvConvState CvFilterState

typedef struct CvPyramid
{
    uchar **ptr;
    CvSize *sz;
    double *rate;
    int *step;
    uchar *state;
    int level;
}
CvPyramid;




#ifndef _CV_GEOM_H_
#define _CV_GEOM_H_
#ifndef _CV_IMG_PROC_H_
#define _CV_IMG_PROC_H_
#ifndef _CV_IPP_H_
#define _CV_IPP_H_


#ifndef _CV_MATRIX_H_
#define _CV_MATRIX_H_

#define icvCopyVector( src, dst, len ) memcpy( (dst), (src), (len)*sizeof((dst)[0]))
#define icvSetZero( dst, len ) memset( (dst), 0, (len)*sizeof((dst)[0]))

#define icvCopyVector_32f( src, len, dst ) memcpy((dst),(src),(len)*sizeof(float))
#define icvSetZero_32f( dst, cols, rows ) memset((dst),0,(rows)*(cols)*sizeof(float))
#define icvCopyVector_64d( src, len, dst ) memcpy((dst),(src),(len)*sizeof(double))
#define icvSetZero_64d( dst, cols, rows ) memset((dst),0,(rows)*(cols)*sizeof(double))
#define icvCopyMatrix_32f( src, w, h, dst ) memcpy((dst),(src),(w)*(h)*sizeof(float))
#define icvCopyMatrix_64d( src, w, h, dst ) memcpy((dst),(src),(w)*(h)*sizeof(double))

#define icvCreateVector_32f( len )  (float*)cvAlloc( (len)*sizeof(float))
#define icvCreateVector_64d( len )  (double*)cvAlloc( (len)*sizeof(double))
#define icvCreateMatrix_32f( w, h )  (float*)cvAlloc( (w)*(h)*sizeof(float))
#define icvCreateMatrix_64d( w, h )  (double*)cvAlloc( (w)*(h)*sizeof(double))

#define icvDeleteVector( vec )  cvFree( &(vec) )
#define icvDeleteMatrix icvDeleteVector

#define icvAddMatrix_32f( src1, src2, dst, w, h ) \
    icvAddVector_32f( (src1), (src2), (dst), (w)*(h))

#define icvSubMatrix_32f( src1, src2, dst, w, h ) \
    icvSubVector_32f( (src1), (src2), (dst), (w)*(h))

#define icvNormVector_32f( src, len )  \
    sqrt(icvDotProduct_32f( src, src, len ))

#define icvNormVector_64d( src, len )  \
    sqrt(icvDotProduct_64d( src, src, len ))


#define icvDeleteMatrix icvDeleteVector

#define icvCheckVector_64f( ptr, len )
#define icvCheckVector_32f( ptr, len )

CV_INLINE double icvSum_32f( const float* src, int len )
{
    double s = 0;
    for( int i = 0; i < len; i++ ) s += src[i];

    icvCheckVector_64f( &s, 1 );

    return s;
}

CV_INLINE double icvDotProduct_32f( const float* src1, const float* src2, int len )
{
    double s = 0;
    for( int i = 0; i < len; i++ ) s += src1[i]*src2[i];

    icvCheckVector_64f( &s, 1 );

    return s;
}


CV_INLINE double icvDotProduct_64f( const double* src1, const double* src2, int len )
{
    double s = 0;
    for( int i = 0; i < len; i++ ) s += src1[i]*src2[i];

    icvCheckVector_64f( &s, 1 );

    return s;
}


CV_INLINE void icvMulVectors_32f( const float* src1, const float* src2,
                                  float* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src1[i] * src2[i];

    icvCheckVector_32f( dst, len );
}

CV_INLINE void icvMulVectors_64d( const double* src1, const double* src2,
                                  double* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src1[i] * src2[i];

    icvCheckVector_64f( dst, len );
}


CV_INLINE void icvAddVector_32f( const float* src1, const float* src2,
                                  float* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src1[i] + src2[i];

    icvCheckVector_32f( dst, len );
}

CV_INLINE void icvAddVector_64d( const double* src1, const double* src2,
                                  double* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src1[i] + src2[i];

    icvCheckVector_64f( dst, len );
}


CV_INLINE void icvSubVector_32f( const float* src1, const float* src2,
                                  float* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src1[i] - src2[i];

    icvCheckVector_32f( dst, len );
}

CV_INLINE void icvSubVector_64d( const double* src1, const double* src2,
                                  double* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src1[i] - src2[i];

    icvCheckVector_64f( dst, len );
}


#define icvAddMatrix_64d( src1, src2, dst, w, h ) \
    icvAddVector_64d( (src1), (src2), (dst), (w)*(h))

#define icvSubMatrix_64d( src1, src2, dst, w, h ) \
    icvSubVector_64d( (src1), (src2), (dst), (w)*(h))


CV_INLINE void icvSetIdentity_32f( float* dst, int w, int h )
{
    int i, len = MIN( w, h );
    icvSetZero_32f( dst, w, h );
    for( i = 0; len--; i += w+1 )
        dst[i] = 1.f;
}


CV_INLINE void icvSetIdentity_64d( double* dst, int w, int h )
{
    int i, len = MIN( w, h );
    icvSetZero_64d( dst, w, h );
    for( i = 0; len--; i += w+1 )
        dst[i] = 1.;
}


CV_INLINE void icvTrace_32f( const float* src, int w, int h, float* trace )
{
    int i, len = MIN( w, h );
    double sum = 0;
    for( i = 0; len--; i += w+1 )
        sum += src[i];
    *trace = (float)sum;

    icvCheckVector_64f( &sum, 1 );
}


CV_INLINE void icvTrace_64d( const double* src, int w, int h, double* trace )
{
    int i, len = MIN( w, h );
    double sum = 0;
    for( i = 0; len--; i += w+1 )
        sum += src[i];
    *trace = sum;

    icvCheckVector_64f( &sum, 1 );
}


CV_INLINE void icvScaleVector_32f( const float* src, float* dst,
                                   int len, double scale )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = (float)(src[i]*scale);

    icvCheckVector_32f( dst, len );
}


CV_INLINE void icvScaleVector_64d( const double* src, double* dst,
                                   int len, double scale )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src[i]*scale;

    icvCheckVector_64f( dst, len );
}


CV_INLINE void icvTransposeMatrix_32f( const float* src, int w, int h, float* dst )
{
    int i, j;

    for( i = 0; i < w; i++ )
        for( j = 0; j < h; j++ )
            *dst++ = src[j*w + i];
        
    icvCheckVector_32f( dst, w*h );
}

CV_INLINE void icvTransposeMatrix_64d( const double* src, int w, int h, double* dst )
{
    int i, j;

    for( i = 0; i < w; i++ )
        for( j = 0; j < h; j++ )
            *dst++ = src[j*w + i];

    icvCheckVector_64f( dst, w*h );
}

CV_INLINE void icvDetMatrix3x3_64d( const double* mat, double* det )
{
    #define m(y,x) mat[(y)*3 + (x)]
    
    *det = m(0,0)*(m(1,1)*m(2,2) - m(1,2)*m(2,1)) -
           m(0,1)*(m(1,0)*m(2,2) - m(1,2)*m(2,0)) +
           m(0,2)*(m(1,0)*m(2,1) - m(1,1)*m(2,0));

    #undef m

    icvCheckVector_64f( det, 1 );
}


CV_INLINE void icvMulMatrix_32f( const float* src1, int w1, int h1,
                                 const float* src2, int w2, int h2,
                                 float* dst )
{
    int i, j, k;

    if( w1 != h2 )
    {
        assert(0);
        return;
    }

    for( i = 0; i < h1; i++, src1 += w1, dst += w2 )
        for( j = 0; j < w2; j++ )
        {
            double s = 0;
            for( k = 0; k < w1; k++ )
                s += src1[k]*src2[j + k*w2];
            dst[j] = (float)s;
        }

    icvCheckVector_32f( dst, h1*w2 );
}


CV_INLINE void icvMulMatrix_64d( const double* src1, int w1, int h1,
                                 const double* src2, int w2, int h2,
                                 double* dst )
{
    int i, j, k;

    if( w1 != h2 )
    {
        assert(0);
        return;
    }

    for( i = 0; i < h1; i++, src1 += w1, dst += w2 )
        for( j = 0; j < w2; j++ )
        {
            double s = 0;
            for( k = 0; k < w1; k++ )
                s += src1[k]*src2[j + k*w2];
            dst[j] = s;
        }

    icvCheckVector_64f( dst, h1*w2 );
}


#define icvTransformVector_32f( matr, src, dst, w, h ) \
    icvMulMatrix_32f( matr, w, h, src, 1, w, dst )

#define icvTransformVector_64d( matr, src, dst, w, h ) \
    icvMulMatrix_64d( matr, w, h, src, 1, w, dst )


#define icvScaleMatrix_32f( src, dst, w, h, scale ) \
    icvScaleVector_32f( (src), (dst), (w)*(h), (scale) )

#define icvScaleMatrix_64d( src, dst, w, h, scale ) \
    icvScaleVector_64d( (src), (dst), (w)*(h), (scale) )

#define icvDotProduct_64d icvDotProduct_64f


CV_INLINE void icvInvertMatrix_64d( double* A, int n, double* invA )
{
    CvMat Am = cvMat( n, n, CV_64F, A );
    CvMat invAm = cvMat( n, n, CV_64F, invA );

    cvInvert( &Am, &invAm, CV_SVD );
}

CV_INLINE void icvMulTransMatrixR_64d( double* src, int width, int height, double* dst )
{
    CvMat srcMat = cvMat( height, width, CV_64F, src );
    CvMat dstMat = cvMat( width, width, CV_64F, dst );

    cvMulTransposed( &srcMat, &dstMat, 1 );
}

CV_INLINE void icvMulTransMatrixL_64d( double* src, int width, int height, double* dst )
{
    CvMat srcMat = cvMat( height, width, CV_64F, src );
    CvMat dstMat = cvMat( height, height, CV_64F, dst );

    cvMulTransposed( &srcMat, &dstMat, 0 );
}

CV_INLINE void icvMulTransMatrixR_32f( float* src, int width, int height, float* dst )
{
    CvMat srcMat = cvMat( height, width, CV_32F, src );
    CvMat dstMat = cvMat( width, width, CV_32F, dst );

    cvMulTransposed( &srcMat, &dstMat, 1 );
}

CV_INLINE void icvMulTransMatrixL_32f( float* src, int width, int height, float* dst )
{
    CvMat srcMat = cvMat( height, width, CV_32F, src );
    CvMat dstMat = cvMat( height, height, CV_32F, dst );

    cvMulTransposed( &srcMat, &dstMat, 0 );
}

CV_INLINE void icvCvt_32f_64d( const float* src, double* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = src[i];
}

CV_INLINE void icvCvt_64d_32f( const double* src, float* dst, int len )
{
    int i;
    for( i = 0; i < len; i++ )
        dst[i] = (float)src[i];
}

#endif/*_CV_MATRIX_H_*/

/* End of file. */


/****************************************************************************************\
*                                  Creating Borders                                      *
\****************************************************************************************/

#define IPCV_COPY_BORDER( bordertype, flavor )                                      \
IPCVAPI_EX( CvStatus, icvCopy##bordertype##Border_##flavor##R,                      \
    "ippiCopy" #bordertype "Border_" #flavor "R", CV_PLUGINS1(CV_PLUGIN_IPPI),      \
    ( const void* pSrc,  int srcStep, CvSize srcRoiSize, void* pDst,  int dstStep,  \
      CvSize dstRoiSize, int topBorderHeight, int leftBorderWidth ))                \
                                                                                    \
IPCVAPI_EX( CvStatus, icvCopy##bordertype##Border_##flavor##IR,                     \
    "ippiCopy" #bordertype "Border_" #flavor "IR", CV_PLUGINS1(CV_PLUGIN_IPPI),     \
    ( const void* pSrc,  int srcDstStep, CvSize srcRoiSize,                         \
      CvSize dstRoiSize, int topBorderHeight, int leftBorderWidth ))

IPCV_COPY_BORDER( Replicate, 8u_C1 )
IPCV_COPY_BORDER( Replicate, 16s_C1 )
IPCV_COPY_BORDER( Replicate, 8u_C3 )
IPCV_COPY_BORDER( Replicate, 32s_C1 )
IPCV_COPY_BORDER( Replicate, 16s_C3 )
IPCV_COPY_BORDER( Replicate, 16s_C4 )
IPCV_COPY_BORDER( Replicate, 32s_C3 )
IPCV_COPY_BORDER( Replicate, 32s_C4 )

/****************************************************************************************\
*                                        Moments                                         *
\****************************************************************************************/

#define IPCV_MOMENTS( suffix, ipp_suffix, cn )                      \
IPCVAPI_EX( CvStatus, icvMoments##suffix##_C##cn##R,                \
"ippiMoments" #ipp_suffix "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
( const void* img, int step, CvSize size, void* momentstate ))

IPCV_MOMENTS( _8u, 64f_8u, 1 )
IPCV_MOMENTS( _32f, 64f_32f, 1 )

#undef IPCV_MOMENTS

IPCVAPI_EX( CvStatus, icvMomentInitAlloc_64f,
            "ippiMomentInitAlloc_64f", CV_PLUGINS1(CV_PLUGIN_IPPI),
            (void** momentstate, CvHintAlgorithm hint ))

IPCVAPI_EX( CvStatus, icvMomentFree_64f,
            "ippiMomentFree_64f", CV_PLUGINS1(CV_PLUGIN_IPPI),
            (void* momentstate ))

IPCVAPI_EX( CvStatus, icvGetSpatialMoment_64f,
            "ippiGetSpatialMoment_64f", CV_PLUGINS1(CV_PLUGIN_IPPI),
            (const void* momentstate, int mOrd, int nOrd,
             int nChannel, CvPoint roiOffset, double* value ))

/****************************************************************************************\
*                                  Background differencing                               *
\****************************************************************************************/

/////////////////////////////////// Accumulation /////////////////////////////////////////

#define IPCV_ACCUM( flavor, arrtype, acctype )                                      \
IPCVAPI_EX( CvStatus, icvAdd_##flavor##_C1IR,                                       \
    "ippiAdd_" #flavor "_C1IR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                       \
    ( const arrtype* src, int srcstep, acctype* dst, int dststep, CvSize size ))    \
IPCVAPI_EX( CvStatus, icvAddSquare_##flavor##_C1IR,                                 \
    "ippiAddSquare_" #flavor "_C1IR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                 \
    ( const arrtype* src, int srcstep, acctype* dst, int dststep, CvSize size ))    \
IPCVAPI_EX( CvStatus, icvAddProduct_##flavor##_C1IR,                                \
    "ippiAddProduct_" #flavor "_C1IR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                \
    ( const arrtype* src1, int srcstep1, const arrtype* src2, int srcstep2,         \
      acctype* dst, int dststep, CvSize size ))                                     \
IPCVAPI_EX( CvStatus, icvAddWeighted_##flavor##_C1IR,                               \
    "ippiAddWeighted_" #flavor "_C1IR", CV_PLUGINS1(CV_PLUGIN_IPPCV),               \
    ( const arrtype* src, int srcstep, acctype* dst, int dststep,                   \
      CvSize size, acctype alpha ))                                                 \
                                                                                    \
IPCVAPI_EX( CvStatus, icvAdd_##flavor##_C1IMR,                                      \
    "ippiAdd_" #flavor "_C1IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                      \
    ( const arrtype* src, int srcstep, const uchar* mask, int maskstep,             \
      acctype* dst, int dststep, CvSize size ))                                     \
IPCVAPI_EX( CvStatus, icvAddSquare_##flavor##_C1IMR,                                \
    "ippiAddSquare_" #flavor "_C1IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                \
    ( const arrtype* src, int srcstep, const uchar* mask, int maskstep,             \
      acctype* dst, int dststep, CvSize size ))                                     \
IPCVAPI_EX( CvStatus, icvAddProduct_##flavor##_C1IMR,                               \
    "ippiAddProduct_" #flavor "_C1IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),               \
    ( const arrtype* src1, int srcstep1, const arrtype* src2, int srcstep2,         \
      const uchar* mask, int maskstep, acctype* dst, int dststep, CvSize size ))    \
IPCVAPI_EX( CvStatus, icvAddWeighted_##flavor##_C1IMR,                              \
    "ippiAddWeighted_" #flavor "_C1IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),              \
    ( const arrtype* src, int srcstep, const uchar* mask, int maskstep,             \
      acctype* dst, int dststep, CvSize size, acctype alpha ))                      \
                                                                                    \
IPCVAPI_EX( CvStatus, icvAdd_##flavor##_C3IMR,                                      \
    "ippiAdd_" #flavor "_C3IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                      \
    ( const arrtype* src, int srcstep, const uchar* mask, int maskstep,             \
      acctype* dst, int dststep, CvSize size ))                                     \
IPCVAPI_EX( CvStatus, icvAddSquare_##flavor##_C3IMR,                                \
    "ippiAddSquare_" #flavor "_C3IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                \
    ( const arrtype* src, int srcstep, const uchar* mask, int maskstep,             \
      acctype* dst, int dststep, CvSize size ))                                     \
IPCVAPI_EX( CvStatus, icvAddProduct_##flavor##_C3IMR,                               \
    "ippiAddProduct_" #flavor "_C3IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),               \
    ( const arrtype* src1, int srcstep1, const arrtype* src2, int srcstep2,         \
      const uchar* mask, int maskstep, acctype* dst, int dststep, CvSize size ))    \
IPCVAPI_EX( CvStatus, icvAddWeighted_##flavor##_C3IMR,                              \
    "ippiAddWeighted_" #flavor "_C3IMR", CV_PLUGINS1(CV_PLUGIN_IPPCV),              \
    ( const arrtype* src, int srcstep, const uchar* mask, int maskstep,             \
      acctype* dst, int dststep, CvSize size, acctype alpha ))

IPCV_ACCUM( 8u32f, uchar, float )
IPCV_ACCUM( 32f, float, float )

#undef IPCV_ACCUM

/****************************************************************************************\
*                                       Pyramids                                         *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvPyrDownGetBufSize_Gauss5x5,
           "ippiPyrDownGetBufSize_Gauss5x5", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( int roiWidth, CvDataType dataType, int channels, int* bufSize ))

IPCVAPI_EX( CvStatus, icvPyrUpGetBufSize_Gauss5x5,
           "ippiPyrUpGetBufSize_Gauss5x5", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( int roiWidth, CvDataType dataType, int channels, int* bufSize ))

#define ICV_PYRDOWN( flavor, cn )                                           \
IPCVAPI_EX( CvStatus, icvPyrDown_Gauss5x5_##flavor##_C##cn##R,              \
"ippiPyrDown_Gauss5x5_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPCV), \
( const void* pSrc, int srcStep, void* pDst, int dstStep,                   \
  CvSize roiSize, void* pBuffer ))

#define ICV_PYRUP( flavor, cn )                                             \
IPCVAPI_EX( CvStatus, icvPyrUp_Gauss5x5_##flavor##_C##cn##R,                \
"ippiPyrUp_Gauss5x5_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPCV),   \
( const void* pSrc, int srcStep, void* pDst, int dstStep,                   \
  CvSize roiSize, void* pBuffer ))

ICV_PYRDOWN( 8u, 1 )
ICV_PYRDOWN( 8u, 3 )
ICV_PYRDOWN( 32f, 1 )
ICV_PYRDOWN( 32f, 3 )

ICV_PYRUP( 8u, 1 )
ICV_PYRUP( 8u, 3 )
ICV_PYRUP( 32f, 1 )
ICV_PYRUP( 32f, 3 )

#undef ICV_PYRDOWN
#undef ICV_PYRUP

/****************************************************************************************\
*                                Geometric Transformations                               *
\****************************************************************************************/

#define IPCV_RESIZE( flavor, cn )                                           \
IPCVAPI_EX( CvStatus, icvResize_##flavor##_C##cn##R,                        \
            "ippiResize_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
           (const void* src, CvSize srcsize, int srcstep, CvRect srcroi,    \
            void* dst, int dststep, CvSize dstroi,                          \
            double xfactor, double yfactor, int interpolation ))

IPCV_RESIZE( 8u, 1 )
IPCV_RESIZE( 8u, 3 )
IPCV_RESIZE( 8u, 4 )

IPCV_RESIZE( 16u, 1 )
IPCV_RESIZE( 16u, 3 )
IPCV_RESIZE( 16u, 4 )

IPCV_RESIZE( 32f, 1 )
IPCV_RESIZE( 32f, 3 )
IPCV_RESIZE( 32f, 4 )

#undef IPCV_RESIZE

#define IPCV_WARPAFFINE_BACK( flavor, cn )                                  \
IPCVAPI_EX( CvStatus, icvWarpAffineBack_##flavor##_C##cn##R,                \
    "ippiWarpAffineBack_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
    (const void* src, CvSize srcsize, int srcstep, CvRect srcroi,           \
    void* dst, int dststep, CvRect dstroi,                                  \
    const double* coeffs, int interpolate ))

IPCV_WARPAFFINE_BACK( 8u, 1 )
IPCV_WARPAFFINE_BACK( 8u, 3 )
IPCV_WARPAFFINE_BACK( 8u, 4 )

IPCV_WARPAFFINE_BACK( 32f, 1 )
IPCV_WARPAFFINE_BACK( 32f, 3 )
IPCV_WARPAFFINE_BACK( 32f, 4 )

#undef IPCV_WARPAFFINE_BACK

#define IPCV_WARPPERSPECTIVE_BACK( flavor, cn )                             \
IPCVAPI_EX( CvStatus, icvWarpPerspectiveBack_##flavor##_C##cn##R,           \
    "ippiWarpPerspectiveBack_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
    (const void* src, CvSize srcsize, int srcstep, CvRect srcroi,           \
    void* dst, int dststep, CvRect dstroi,                                  \
    const double* coeffs, int interpolate ))

IPCV_WARPPERSPECTIVE_BACK( 8u, 1 )
IPCV_WARPPERSPECTIVE_BACK( 8u, 3 )
IPCV_WARPPERSPECTIVE_BACK( 8u, 4 )

IPCV_WARPPERSPECTIVE_BACK( 32f, 1 )
IPCV_WARPPERSPECTIVE_BACK( 32f, 3 )
IPCV_WARPPERSPECTIVE_BACK( 32f, 4 )

#undef IPCV_WARPPERSPECTIVE_BACK


#define IPCV_WARPPERSPECTIVE( flavor, cn )                                  \
IPCVAPI_EX( CvStatus, icvWarpPerspective_##flavor##_C##cn##R,               \
    "ippiWarpPerspective_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),\
    (const void* src, CvSize srcsize, int srcstep, CvRect srcroi,           \
    void* dst, int dststep, CvRect dstroi,                                  \
    const double* coeffs, int interpolate ))

IPCV_WARPPERSPECTIVE( 8u, 1 )
IPCV_WARPPERSPECTIVE( 8u, 3 )
IPCV_WARPPERSPECTIVE( 8u, 4 )

IPCV_WARPPERSPECTIVE( 32f, 1 )
IPCV_WARPPERSPECTIVE( 32f, 3 )
IPCV_WARPPERSPECTIVE( 32f, 4 )

#undef IPCV_WARPPERSPECTIVE

#define IPCV_REMAP( flavor, cn )                                        \
IPCVAPI_EX( CvStatus, icvRemap_##flavor##_C##cn##R,                     \
    "ippiRemap_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),     \
    ( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,      \
      const float* xmap, int xmapstep, const float* ymap, int ymapstep, \
      void* dst, int dststep, CvSize dstsize, int interpolation )) 

IPCV_REMAP( 8u, 1 )
IPCV_REMAP( 8u, 3 )
IPCV_REMAP( 8u, 4 )

IPCV_REMAP( 32f, 1 )
IPCV_REMAP( 32f, 3 )
IPCV_REMAP( 32f, 4 )

#undef IPCV_REMAP

/****************************************************************************************\
*                                      Morphology                                        *
\****************************************************************************************/

#define IPCV_MORPHOLOGY( minmaxtype, morphtype, flavor, cn )                    \
IPCVAPI_EX( CvStatus, icv##morphtype##Rect_##flavor##_C##cn##R,                 \
    "ippiFilter" #minmaxtype "BorderReplicate_" #flavor "_C" #cn "R",           \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( const void* src, int srcstep, void* dst,    \
    int dststep, CvSize roi, CvSize esize, CvPoint anchor, void* buffer ))      \
IPCVAPI_EX( CvStatus, icv##morphtype##Rect_GetBufSize_##flavor##_C##cn##R,      \
    "ippiFilter" #minmaxtype "GetBufferSize_" #flavor "_C" #cn "R",             \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( int width, CvSize esize, int* bufsize ))    \
                                                                                \
IPCVAPI_EX( CvStatus, icv##morphtype##_##flavor##_C##cn##R,                     \
    "ippi" #morphtype "BorderReplicate_" #flavor "_C" #cn "R",                  \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( const void* src, int srcstep,               \
    void* dst, int dststep, CvSize roi, int bordertype, void* morphstate ))

IPCV_MORPHOLOGY( Min, Erode, 8u, 1 )
IPCV_MORPHOLOGY( Min, Erode, 8u, 3 )
IPCV_MORPHOLOGY( Min, Erode, 8u, 4 )
IPCV_MORPHOLOGY( Min, Erode, 32f, 1 )
IPCV_MORPHOLOGY( Min, Erode, 32f, 3 )
IPCV_MORPHOLOGY( Min, Erode, 32f, 4 )
IPCV_MORPHOLOGY( Max, Dilate, 8u, 1 )
IPCV_MORPHOLOGY( Max, Dilate, 8u, 3 )
IPCV_MORPHOLOGY( Max, Dilate, 8u, 4 )
IPCV_MORPHOLOGY( Max, Dilate, 32f, 1 )
IPCV_MORPHOLOGY( Max, Dilate, 32f, 3 )
IPCV_MORPHOLOGY( Max, Dilate, 32f, 4 )

#undef IPCV_MORPHOLOGY

#define IPCV_MORPHOLOGY_INIT_ALLOC( flavor, cn )                            \
IPCVAPI_EX( CvStatus, icvMorphInitAlloc_##flavor##_C##cn##R,                \
    "ippiMorphologyInitAlloc_" #flavor "_C" #cn "R",                        \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( int width, const uchar* element,        \
    CvSize esize, CvPoint anchor, void** morphstate ))

IPCV_MORPHOLOGY_INIT_ALLOC( 8u, 1 )
IPCV_MORPHOLOGY_INIT_ALLOC( 8u, 3 )
IPCV_MORPHOLOGY_INIT_ALLOC( 8u, 4 )
IPCV_MORPHOLOGY_INIT_ALLOC( 32f, 1 )
IPCV_MORPHOLOGY_INIT_ALLOC( 32f, 3 )
IPCV_MORPHOLOGY_INIT_ALLOC( 32f, 4 )

#undef IPCV_MORPHOLOGY_INIT_ALLOC

IPCVAPI_EX( CvStatus, icvMorphFree, "ippiMorphologyFree",
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( void* morphstate ))


/****************************************************************************************\
*                                 Smoothing Filters                                      *
\****************************************************************************************/

#define IPCV_FILTER_MEDIAN( flavor, cn )                                            \
IPCVAPI_EX( CvStatus, icvFilterMedian_##flavor##_C##cn##R,                          \
            "ippiFilterMedian_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),  \
            ( const void* src, int srcstep, void* dst, int dststep,                 \
              CvSize roi, CvSize ksize, CvPoint anchor ))

IPCV_FILTER_MEDIAN( 8u, 1 )
IPCV_FILTER_MEDIAN( 8u, 3 )
IPCV_FILTER_MEDIAN( 8u, 4 )

#define IPCV_FILTER_BOX( flavor, cn )                                               \
IPCVAPI_EX( CvStatus, icvFilterBox_##flavor##_C##cn##R,                             \
            "ippiFilterBox_" #flavor "_C" #cn "R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,\
            ( const void* src, int srcstep, void* dst, int dststep,                 \
              CvSize roi, CvSize ksize, CvPoint anchor ))

IPCV_FILTER_BOX( 8u, 1 )
IPCV_FILTER_BOX( 8u, 3 )
IPCV_FILTER_BOX( 8u, 4 )
IPCV_FILTER_BOX( 32f, 1 )
IPCV_FILTER_BOX( 32f, 3 )
IPCV_FILTER_BOX( 32f, 4 )

#undef IPCV_FILTER_BOX

/****************************************************************************************\
*                                 Derivative Filters                                     *
\****************************************************************************************/

#define IPCV_FILTER_SOBEL_BORDER( suffix, flavor, srctype )                             \
IPCVAPI_EX( CvStatus, icvFilterSobel##suffix##GetBufSize_##flavor##_C1R,                \
    "ippiFilterSobel" #suffix "GetBufferSize_" #flavor "_C1R",                          \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( CvSize roi, int masksize, int* buffersize ))        \
IPCVAPI_EX( CvStatus, icvFilterSobel##suffix##Border_##flavor##_C1R,                    \
    "ippiFilterSobel" #suffix "Border_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),   \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int masksize,   \
      int bordertype, srctype bordervalue, void* buffer ))

IPCV_FILTER_SOBEL_BORDER( NegVert, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( Horiz, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( VertSecond, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( HorizSecond, 8u16s, uchar )
IPCV_FILTER_SOBEL_BORDER( Cross, 8u16s, uchar )

IPCV_FILTER_SOBEL_BORDER( NegVert, 32f, float )
IPCV_FILTER_SOBEL_BORDER( Horiz, 32f, float )
IPCV_FILTER_SOBEL_BORDER( VertSecond, 32f, float )
IPCV_FILTER_SOBEL_BORDER( HorizSecond, 32f, float )
IPCV_FILTER_SOBEL_BORDER( Cross, 32f, float )

#undef IPCV_FILTER_SOBEL_BORDER

#define IPCV_FILTER_SCHARR_BORDER( suffix, flavor, srctype )                            \
IPCVAPI_EX( CvStatus, icvFilterScharr##suffix##GetBufSize_##flavor##_C1R,               \
    "ippiFilterScharr" #suffix "GetBufferSize_" #flavor "_C1R",                         \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( CvSize roi, int* buffersize ))                      \
IPCVAPI_EX( CvStatus, icvFilterScharr##suffix##Border_##flavor##_C1R,                   \
    "ippiFilterScharr" #suffix "Border_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),  \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi,                 \
      int bordertype, srctype bordervalue, void* buffer ))

IPCV_FILTER_SCHARR_BORDER( Vert, 8u16s, uchar )
IPCV_FILTER_SCHARR_BORDER( Horiz, 8u16s, uchar )

IPCV_FILTER_SCHARR_BORDER( Vert, 32f, float )
IPCV_FILTER_SCHARR_BORDER( Horiz, 32f, float )

#undef IPCV_FILTER_SCHARR_BORDER


#define IPCV_FILTER_LAPLACIAN_BORDER( flavor, srctype )                                 \
IPCVAPI_EX( CvStatus, icvFilterLaplacianGetBufSize_##flavor##_C1R,                      \
    "ippiFilterLaplacianGetBufferSize_" #flavor "_C1R",                                 \
    CV_PLUGINS1(CV_PLUGIN_IPPCV), ( CvSize roi, int masksize, int* buffersize ))        \
IPCVAPI_EX( CvStatus, icvFilterLaplacianBorder_##flavor##_C1R,                          \
    "ippiFilterLaplacianBorder_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),          \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int masksize,   \
      int bordertype, srctype bordervalue, void* buffer ))

IPCV_FILTER_LAPLACIAN_BORDER( 8u16s, uchar )
IPCV_FILTER_LAPLACIAN_BORDER( 32f, float )

#undef IPCV_FILTER_LAPLACIAN_BORDER


/////////////////////////////////////////////////////////////////////////////////////////

#define IPCV_FILTER_SOBEL( suffix, ipp_suffix, flavor )                             \
IPCVAPI_EX( CvStatus, icvFilterSobel##suffix##_##flavor##_C1R,                      \
    "ippiFilterSobel" #ipp_suffix "_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),  \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int aperture ))

IPCV_FILTER_SOBEL( Vert, Vert, 8u16s )
IPCV_FILTER_SOBEL( Horiz, Horiz, 8u16s )
IPCV_FILTER_SOBEL( VertSecond, VertSecond, 8u16s )
IPCV_FILTER_SOBEL( HorizSecond, HorizSecond, 8u16s )
IPCV_FILTER_SOBEL( Cross, Cross, 8u16s )

IPCV_FILTER_SOBEL( Vert, VertMask, 32f )
IPCV_FILTER_SOBEL( Horiz, HorizMask, 32f )
IPCV_FILTER_SOBEL( VertSecond, VertSecond, 32f )
IPCV_FILTER_SOBEL( HorizSecond, HorizSecond, 32f )
IPCV_FILTER_SOBEL( Cross, Cross, 32f )

#undef IPCV_FILTER_SOBEL

#define IPCV_FILTER_SCHARR( suffix, ipp_suffix, flavor )                            \
IPCVAPI_EX( CvStatus, icvFilterScharr##suffix##_##flavor##_C1R,                     \
    "ippiFilterScharr" #ipp_suffix "_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI), \
    ( const void* src, int srcstep, void* dst, int dststep, CvSize roi ))

IPCV_FILTER_SCHARR( Vert, Vert, 8u16s )
IPCV_FILTER_SCHARR( Horiz, Horiz, 8u16s )
IPCV_FILTER_SCHARR( Vert, Vert, 32f )
IPCV_FILTER_SCHARR( Horiz, Horiz, 32f )

#undef IPCV_FILTER_SCHARR

/****************************************************************************************\
*                                   Generic Filters                                      *
\****************************************************************************************/

#define IPCV_FILTER( suffix, ipp_suffix, cn, ksizetype, anchortype )                    \
IPCVAPI_EX( CvStatus, icvFilter##suffix##_C##cn##R,                                     \
            "ippiFilter" #ipp_suffix "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
            ( const void* src, int srcstep, void* dst, int dststep, CvSize size,        \
              const float* kernel, ksizetype ksize, anchortype anchor ))

IPCV_FILTER( _8u, 32f_8u, 1, CvSize, CvPoint )
IPCV_FILTER( _8u, 32f_8u, 3, CvSize, CvPoint )
IPCV_FILTER( _8u, 32f_8u, 4, CvSize, CvPoint )

IPCV_FILTER( _16s, 32f_16s, 1, CvSize, CvPoint )
IPCV_FILTER( _16s, 32f_16s, 3, CvSize, CvPoint )
IPCV_FILTER( _16s, 32f_16s, 4, CvSize, CvPoint )

IPCV_FILTER( _32f, _32f, 1, CvSize, CvPoint )
IPCV_FILTER( _32f, _32f, 3, CvSize, CvPoint )
IPCV_FILTER( _32f, _32f, 4, CvSize, CvPoint )

IPCV_FILTER( Column_8u, Column32f_8u, 1, int, int )
IPCV_FILTER( Column_8u, Column32f_8u, 3, int, int )
IPCV_FILTER( Column_8u, Column32f_8u, 4, int, int )

IPCV_FILTER( Column_16s, Column32f_16s, 1, int, int )
IPCV_FILTER( Column_16s, Column32f_16s, 3, int, int )
IPCV_FILTER( Column_16s, Column32f_16s, 4, int, int )

IPCV_FILTER( Column_32f, Column_32f, 1, int, int )
IPCV_FILTER( Column_32f, Column_32f, 3, int, int )
IPCV_FILTER( Column_32f, Column_32f, 4, int, int )

IPCV_FILTER( Row_8u, Row32f_8u, 1, int, int )
IPCV_FILTER( Row_8u, Row32f_8u, 3, int, int )
IPCV_FILTER( Row_8u, Row32f_8u, 4, int, int )

IPCV_FILTER( Row_16s, Row32f_16s, 1, int, int )
IPCV_FILTER( Row_16s, Row32f_16s, 3, int, int )
IPCV_FILTER( Row_16s, Row32f_16s, 4, int, int )

IPCV_FILTER( Row_32f, Row_32f, 1, int, int )
IPCV_FILTER( Row_32f, Row_32f, 3, int, int )
IPCV_FILTER( Row_32f, Row_32f, 4, int, int )

#undef IPCV_FILTER


/****************************************************************************************\
*                                  Color Transformations                                 *
\****************************************************************************************/

#define IPCV_COLOR( funcname, ipp_funcname, flavor )                            \
IPCVAPI_EX( CvStatus, icv##funcname##_##flavor##_C3R,                           \
        "ippi" #ipp_funcname "_" #flavor "_C3R,"                                \
        "ippi" #ipp_funcname "_" #flavor "_C3R",                                \
        CV_PLUGINS2(CV_PLUGIN_IPPI,CV_PLUGIN_IPPCC),                            \
        ( const void* src, int srcstep, void* dst, int dststep, CvSize size ))

IPCV_COLOR( RGB2XYZ, RGBToXYZ, 8u )
IPCV_COLOR( RGB2XYZ, RGBToXYZ, 16u )
IPCV_COLOR( RGB2XYZ, RGBToXYZ, 32f )
IPCV_COLOR( XYZ2RGB, XYZToRGB, 8u )
IPCV_COLOR( XYZ2RGB, XYZToRGB, 16u )
IPCV_COLOR( XYZ2RGB, XYZToRGB, 32f )

IPCV_COLOR( RGB2HSV, RGBToHSV, 8u )
IPCV_COLOR( HSV2RGB, HSVToRGB, 8u )

IPCV_COLOR( RGB2HLS, RGBToHLS, 8u )
IPCV_COLOR( RGB2HLS, RGBToHLS, 32f )
IPCV_COLOR( HLS2RGB, HLSToRGB, 8u )
IPCV_COLOR( HLS2RGB, HLSToRGB, 32f )

IPCV_COLOR( BGR2Lab, BGRToLab, 8u )
IPCV_COLOR( Lab2BGR, LabToBGR, 8u )

IPCV_COLOR( RGB2Luv, RGBToLUV, 8u )
/*IPCV_COLOR( RGB2Luv, RGBToLUV, 32f )*/
IPCV_COLOR( Luv2RGB, LUVToRGB, 8u )
/*IPCV_COLOR( Luv2RGB, LUVToRGB, 32f )*/

/****************************************************************************************\
*                                  Motion Templates                                      *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvUpdateMotionHistory_8u32f_C1IR,
    "ippiUpdateMotionHistory_8u32f_C1IR", CV_PLUGINS1(CV_PLUGIN_IPPCV),
    ( const uchar* silIm, int silStep, float* mhiIm, int mhiStep,
      CvSize size,float  timestamp, float  mhi_duration ))

/****************************************************************************************\
*                                 Template Matching                                      *
\****************************************************************************************/

#define ICV_MATCHTEMPLATE( flavor, arrtype )                        \
IPCVAPI_EX( CvStatus, icvCrossCorrValid_Norm_##flavor##_C1R,        \
        "ippiCrossCorrValid_Norm_" #flavor "_C1R",                  \
        CV_PLUGINS1(CV_PLUGIN_IPPI),                                \
        ( const arrtype* pSrc, int srcStep, CvSize srcRoiSize,      \
        const arrtype* pTpl, int tplStep, CvSize tplRoiSize,        \
        float* pDst, int dstStep ))                                 \
IPCVAPI_EX( CvStatus, icvCrossCorrValid_NormLevel_##flavor##_C1R,   \
        "ippiCrossCorrValid_NormLevel_" #flavor "_C1R",             \
        CV_PLUGINS1(CV_PLUGIN_IPPI),                                \
        ( const arrtype* pSrc, int srcStep, CvSize srcRoiSize,      \
        const arrtype* pTpl, int tplStep, CvSize tplRoiSize,        \
        float* pDst, int dstStep ))                                 \
IPCVAPI_EX( CvStatus, icvSqrDistanceValid_Norm_##flavor##_C1R,      \
        "ippiSqrDistanceValid_Norm_" #flavor "_C1R",                \
        CV_PLUGINS1(CV_PLUGIN_IPPI),                                \
        ( const arrtype* pSrc, int srcStep, CvSize srcRoiSize,      \
        const arrtype* pTpl, int tplStep, CvSize tplRoiSize,        \
        float* pDst, int dstStep ))

ICV_MATCHTEMPLATE( 8u32f, uchar )
ICV_MATCHTEMPLATE( 32f, float )

/****************************************************************************************/
/*                                Distance Transform                                    */
/****************************************************************************************/

IPCVAPI_EX(CvStatus, icvDistanceTransform_3x3_8u32f_C1R,
    "ippiDistanceTransform_3x3_8u32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
    ( const uchar* pSrc, int srcStep, float* pDst,
      int dstStep, CvSize roiSize, const float* pMetrics ))

IPCVAPI_EX(CvStatus, icvDistanceTransform_5x5_8u32f_C1R,
    "ippiDistanceTransform_5x5_8u32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
    ( const uchar* pSrc, int srcStep, float* pDst,
      int dstStep, CvSize roiSize, const float* pMetrics ))

/****************************************************************************************\
*                               Thresholding functions                                   *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvCompareC_8u_C1R_cv,
            "ippiCompareC_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* src1, int srcstep1, uchar scalar,
              uchar* dst, int dststep, CvSize size, int cmp_op ))
IPCVAPI_EX( CvStatus, icvAndC_8u_C1R,
            "ippiAndC_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* src1, int srcstep1, uchar scalar,
              uchar* dst, int dststep, CvSize size ))
IPCVAPI_EX( CvStatus, icvThreshold_GTVal_8u_C1R,
            "ippiThreshold_GTVal_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* pSrc, int srcstep, uchar* pDst, int dststep,
              CvSize size, uchar threshold, uchar value ))
IPCVAPI_EX( CvStatus, icvThreshold_GTVal_32f_C1R,
            "ippiThreshold_GTVal_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const float* pSrc, int srcstep, float* pDst, int dststep,
              CvSize size, float threshold, float value ))
IPCVAPI_EX( CvStatus, icvThreshold_LTVal_8u_C1R,
            "ippiThreshold_LTVal_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const uchar* pSrc, int srcstep, uchar* pDst, int dststep,
              CvSize size, uchar threshold, uchar value ))
IPCVAPI_EX( CvStatus, icvThreshold_LTVal_32f_C1R,
            "ippiThreshold_LTVal_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
            ( const float* pSrc, int srcstep, float* pDst, int dststep,
              CvSize size, float threshold, float value ))

/****************************************************************************************\
*                                 Canny Edge Detector                                    *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvCannyGetSize, "ippiCannyGetSize", 0/*CV_PLUGINS1(CV_PLUGIN_IPPCV)*/,
           ( CvSize roiSize, int* bufferSize ))

IPCVAPI_EX( CvStatus, icvCanny_16s8u_C1R, "ippiCanny_16s8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPCV)*/,
    ( const short* pSrcDx, int srcDxStep, const short* pSrcDy, int srcDyStep,
      uchar*  pDstEdges, int dstEdgeStep, CvSize roiSize, float lowThresh,
      float  highThresh, void* pBuffer ))


/****************************************************************************************\
*                                 Radial Distortion Removal                              *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvUndistortGetSize, "ippiUndistortGetSize",
            CV_PLUGINS1(CV_PLUGIN_IPPCV), ( CvSize roiSize, int *pBufsize ))

IPCVAPI_EX( CvStatus, icvCreateMapCameraUndistort_32f_C1R,
            "ippiCreateMapCameraUndistort_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            (float *pxMap, int xStep, float *pyMap, int yStep, CvSize roiSize,
            float fx, float fy, float cx, float cy, float k1, float k2,
            float p1, float p2, uchar *pBuffer ))

#define ICV_UNDISTORT_RADIAL( flavor, cn, arrtype )                                 \
IPCVAPI_EX( CvStatus, icvUndistortRadial_##flavor##_C##cn##R,                       \
    "ippiUndistortRadial_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPCV),      \
    ( const arrtype* pSrc, int srcStep, uchar* pDst, int dstStep, CvSize roiSize,   \
      float fx, float fy, float cx, float cy, float k1, float k2, uchar *pBuffer ))

ICV_UNDISTORT_RADIAL( 8u, 1, uchar )
ICV_UNDISTORT_RADIAL( 8u, 3, uchar )

#undef ICV_UNDISTORT_RADIAL

/****************************************************************************************\
*                            Subpixel-accurate rectangle extraction                      *
\****************************************************************************************/

#define ICV_COPY_SUBPIX( flavor, cn, srctype, dsttype )                     \
IPCVAPI_EX( CvStatus, icvCopySubpix_##flavor##_C##cn##R,                    \
    "ippiCopySubpix_" #flavor "_C" #cn "R", CV_PLUGINS1(CV_PLUGIN_IPPCV),   \
    ( const srctype* pSrc, int srcStep, dsttype* pDst, int dstStep,         \
    CvSize size, float dx, float dy ))

ICV_COPY_SUBPIX( 8u, 1, uchar, uchar )
ICV_COPY_SUBPIX( 8u32f, 1, uchar, float )
//ICV_COPY_SUBPIX( 32f, 1, float, float )

IPCVAPI_EX( CvStatus, icvCopySubpix_32f_C1R,
    "ippiCopySubpix_32f_C1R", 0,
    ( const float* pSrc, int srcStep, float* pDst, int dstStep,
    CvSize size, float dx, float dy ))

#undef ICV_COPY_SUBPIX

/****************************************************************************************\
*                                Lucas-Kanade Optical Flow                               *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvOpticalFlowPyrLKInitAlloc_8u_C1R,
            "ippiOpticalFlowPyrLKInitAlloc_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( void** ppState, CvSize roiSize, int winSize, int hint ))

IPCVAPI_EX( CvStatus, icvOpticalFlowPyrLKFree_8u_C1R,
            "ippiOpticalFlowPyrLKFree_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( void* pState ))

IPCVAPI_EX( CvStatus, icvOpticalFlowPyrLK_8u_C1R,
            "ippiOpticalFlowPyrLK_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( CvPyramid *pPyr1, CvPyramid *pPyr2,
            const float *pPrev, float* pNext, char *pStatus,
            float *pError, int numFeat, int winSize,
            int maxLev, int maxIter, float threshold, void* state ))


/****************************************************************************************\
*                                 Haar Object Detector                                   *
\****************************************************************************************/

IPCVAPI_EX( CvStatus, icvIntegral_8u32s_C1R,
            "ippiIntegral_8u32s_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( const uchar* pSrc, int srcStep, int* pDst, int dstStep,
              CvSize roiSize, int val ))

IPCVAPI_EX( CvStatus, icvSqrIntegral_8u32s64f_C1R,
            "ippiSqrIntegral_8u32s64f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( const uchar* pSrc, int srcStep,
              int* pDst, int dstStep, double* pSqr, int sqrStep,
              CvSize roiSize, int val, double valSqr ))

IPCVAPI_EX( CvStatus, icvRectStdDev_32s32f_C1R,
            "ippiRectStdDev_32s32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( const int* pSrc, int srcStep,
              const double* pSqr, int sqrStep, float* pDst, int dstStep,
              CvSize roiSize, CvRect rect ))

IPCVAPI_EX( CvStatus, icvHaarClassifierInitAlloc_32f,
            "ippiHaarClassifierInitAlloc_32f", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( void **pState, const CvRect* pFeature, const float* pWeight,
              const float* pThreshold, const float* pVal1,
              const float* pVal2, const int* pNum, int length ))

IPCVAPI_EX( CvStatus, icvHaarClassifierFree_32f,
            "ippiHaarClassifierFree_32f", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( void *pState ))

IPCVAPI_EX( CvStatus, icvApplyHaarClassifier_32s32f_C1R,
            "ippiApplyHaarClassifier_32s32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),
            ( const int* pSrc, int srcStep, const float* pNorm,
              int normStep, uchar* pMask, int maskStep,
              CvSize roi, int *pPositive, float threshold,
              void *pState ))

#endif /*_CV_IPP_H_*/


#define  CV_COPY( dst, src, len, idx ) \
for( (idx) = 0; (idx) < (len); (idx)++) (dst)[idx] = (src)[idx]

#define  CV_SET( dst, val, len, idx )  \
for( (idx) = 0; (idx) < (len); (idx)++) (dst)[idx] = (val)

/* performs convolution of 2d floating-point array with 3x1, 1x3 or separable 3x3 mask */
void icvSepConvSmall3_32f( float* src, int src_step, float* dst, int dst_step,
						  CvSize src_size, const float* kx, const float* ky, float* buffer );

typedef CvStatus (CV_STDCALL * CvSobelFixedIPPFunc)
( const void* src, int srcstep, void* dst, int dststep, CvSize roi, int aperture );

typedef CvStatus (CV_STDCALL * CvFilterFixedIPPFunc)
( const void* src, int srcstep, void* dst, int dststep, CvSize roi );

#undef   CV_CALC_MIN
#define  CV_CALC_MIN(a, b) if((a) > (b)) (a) = (b)

#undef   CV_CALC_MAX
#define  CV_CALC_MAX(a, b) if((a) < (b)) (a) = (b)

#define CV_MORPH_ALIGN  4

#define CV_WHOLE   0
#define CV_START   1
#define CV_END     2
#define CV_MIDDLE  4

void
icvCrossCorr( const CvArr* _img, const CvArr* _templ,
			 CvArr* _corr, CvPoint anchor=cvPoint(0,0) );

CvStatus CV_STDCALL
icvCopyReplicateBorder_8u( const uchar* src, int srcstep, CvSize srcroi,
						  uchar* dst, int dststep, CvSize dstroi,
						  int left, int right, int cn, const uchar* value = 0 );

CvMat* icvIPPFilterInit( const CvMat* src, int stripe_size, CvSize ksize );

int icvIPPFilterNextStripe( const CvMat* src, CvMat* temp, int y,
						   CvSize ksize, CvPoint anchor );

int icvIPPSepFilter( const CvMat* src, CvMat* dst, const CvMat* kernelX,
					const CvMat* kernelY, CvPoint anchor );

#define ICV_WARP_SHIFT          10
#define ICV_WARP_MASK           ((1 << ICV_WARP_SHIFT) - 1)

#define ICV_LINEAR_TAB_SIZE     (ICV_WARP_MASK+1)
extern float icvLinearCoeffs[(ICV_LINEAR_TAB_SIZE+1)*2];
void icvInitLinearCoeffTab();

#define ICV_CUBIC_TAB_SIZE   (ICV_WARP_MASK+1)
extern float icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE+1)*2];

void icvInitCubicCoeffTab();

CvStatus CV_STDCALL icvGetRectSubPix_8u_C1R
( const uchar* src, int src_step, CvSize src_size,
 uchar* dst, int dst_step, CvSize win_size, CvPoint2D32f center );
CvStatus CV_STDCALL icvGetRectSubPix_8u32f_C1R
( const uchar* src, int src_step, CvSize src_size,
 float* dst, int dst_step, CvSize win_size, CvPoint2D32f center );
CvStatus CV_STDCALL icvGetRectSubPix_32f_C1R
( const float* src, int src_step, CvSize src_size,
 float* dst, int dst_step, CvSize win_size, CvPoint2D32f center );

#endif /*_CV_INTERNAL_H_*/

/* Finds distance between two points */
CV_INLINE  float  icvDistanceL2_32f( CvPoint2D32f pt1, CvPoint2D32f pt2 )
{
    float dx = pt2.x - pt1.x;
    float dy = pt2.y - pt1.y;
	
    return cvSqrt( dx*dx + dy*dy );
}


int  icvIntersectLines( double x1, double dx1, double y1, double dy1,
					   double x2, double dx2, double y2, double dy2,
					   double* t2 );


void icvCreateCenterNormalLine( CvSubdiv2DEdge edge, double* a, double* b, double* c );

void icvIntersectLines3( double* a0, double* b0, double* c0,
						double* a1, double* b1, double* c1,
						CvPoint2D32f* point );


#define _CV_BINTREE_LIST()                                          \
	struct _CvTrianAttr* prev_v;   /* pointer to the parent  element on the previous level of the tree  */    \
	struct _CvTrianAttr* next_v1;   /* pointer to the child  element on the next level of the tree  */        \
struct _CvTrianAttr* next_v2;   /* pointer to the child  element on the next level of the tree  */        

typedef struct _CvTrianAttr
{
	CvPoint pt;    /* Coordinates x and y of the vertex  which don't lie on the base line LMIAT  */
	char sign;             /*  sign of the triangle   */
	double area;       /*   area of the triangle    */
	double r1;   /*  The ratio of the height of triangle to the base of the triangle  */
	double r2;  /*   The ratio of the projection of the left side of the triangle on the base to the base */
	_CV_BINTREE_LIST()    /* structure double list   */
}
_CvTrianAttr;


/* curvature: 0 - 1-curvature, 1 - k-cosine curvature. */
CvStatus  icvApproximateChainTC89( CvChain*      chain,
								  int header_size,
								  CvMemStorage* storage,
								  CvSeq**   contour,
								  int method );

#endif /*_IPCVGEOM_H_*/



// default face cascade
//extern const char* icvDefaultFaceCascade[];

#endif /*_CV_INTERNAL_H_*/
