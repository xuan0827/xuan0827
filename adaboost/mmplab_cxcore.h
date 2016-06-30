#ifndef _CXCORE_INTERNAL_H_
#define _CXCORE_INTERNAL_H_

#if defined _MSC_VER && _MSC_VER >= 1200
    /* disable warnings related to inline functions */
    #pragma warning( disable: 4711 4710 4514 )
#endif

typedef unsigned long ulong;

#ifdef __BORLANDC__
    #define     WIN32
    #define     CV_DLL
    #undef      _CV_ALWAYS_PROFILE_
    #define     _CV_ALWAYS_NO_PROFILE_
#endif

#include "cxcore.h"
#include "cxmisc.h"

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>


#ifndef _CXCORE_IPP_H_
#define _CXCORE_IPP_H_

/****************************************************************************************\
*                                      Copy/Set                                          *
\****************************************************************************************/

/* temporary disable ipp zero and copy functions as they affect subsequent functions' performance */
IPCVAPI_EX( CvStatus, icvCopy_8u_C1R, "ippiCopy_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,
                  ( const uchar* src, int src_step,
                    uchar* dst, int dst_step, CvSize size ))

IPCVAPI_EX( CvStatus, icvSetByte_8u_C1R, "ippiSet_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,
                  ( uchar value, uchar* dst, int dst_step, CvSize size ))

IPCVAPI_EX( CvStatus, icvCvt_32f64f, "ippsConvert_32f64f",
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const float* src, double* dst, int len ))
IPCVAPI_EX( CvStatus, icvCvt_64f32f, "ippsConvert_64f32f",
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const double* src, float* dst, int len ))

#define IPCV_COPYSET( flavor, arrtype, scalartype )                                 \
IPCVAPI_EX( CvStatus, icvCopy##flavor, "ippiCopy" #flavor,                          \
                                    CV_PLUGINS1(CV_PLUGIN_IPPI),                    \
                                   ( const arrtype* src, int srcstep,               \
                                     arrtype* dst, int dststep, CvSize size,        \
                                     const uchar* mask, int maskstep ))             \
IPCVAPI_EX( CvStatus, icvSet##flavor, "ippiSet" #flavor,                            \
                                    0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,              \
                                  ( arrtype* dst, int dststep,                      \
                                    const uchar* mask, int maskstep,                \
                                    CvSize size, const arrtype* scalar ))

IPCV_COPYSET( _8u_C1MR, uchar, int )
IPCV_COPYSET( _16s_C1MR, ushort, int )
IPCV_COPYSET( _8u_C3MR, uchar, int )
IPCV_COPYSET( _8u_C4MR, int, int )
IPCV_COPYSET( _16s_C3MR, ushort, int )
IPCV_COPYSET( _16s_C4MR, int64, int64 )
IPCV_COPYSET( _32f_C3MR, int, int )
IPCV_COPYSET( _32f_C4MR, int, int )
IPCV_COPYSET( _64s_C3MR, int64, int64 )
IPCV_COPYSET( _64s_C4MR, int64, int64 )


/****************************************************************************************\
*                                       Arithmetics                                      *
\****************************************************************************************/

#define IPCV_BIN_ARITHM( name )                                     \
IPCVAPI_EX( CvStatus, icv##name##_8u_C1R,                           \
    "ippi" #name "_8u_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
( const uchar* src1, int srcstep1, const uchar* src2, int srcstep2, \
  uchar* dst, int dststep, CvSize size, int scalefactor ))          \
IPCVAPI_EX( CvStatus, icv##name##_16u_C1R,                          \
    "ippi" #name "_16u_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
( const ushort* src1, int srcstep1, const ushort* src2, int srcstep2,\
  ushort* dst, int dststep, CvSize size, int scalefactor ))         \
IPCVAPI_EX( CvStatus, icv##name##_16s_C1R,                          \
    "ippi" #name "_16s_C1RSfs", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
( const short* src1, int srcstep1, const short* src2, int srcstep2, \
  short* dst, int dststep, CvSize size, int scalefactor ))          \
IPCVAPI_EX( CvStatus, icv##name##_32s_C1R,                          \
    "ippi" #name "_32s_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const int* src1, int srcstep1, const int* src2, int srcstep2,     \
  int* dst, int dststep, CvSize size ))                             \
IPCVAPI_EX( CvStatus, icv##name##_32f_C1R,                          \
    "ippi" #name "_32f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const float* src1, int srcstep1, const float* src2, int srcstep2, \
  float* dst, int dststep, CvSize size ))                           \
IPCVAPI_EX( CvStatus, icv##name##_64f_C1R,                          \
    "ippi" #name "_64f_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),           \
( const double* src1, int srcstep1, const double* src2, int srcstep2,\
  double* dst, int dststep, CvSize size ))


IPCV_BIN_ARITHM( Add )
IPCV_BIN_ARITHM( Sub )

#undef IPCV_BIN_ARITHM

/****************************************************************************************\
*                                     Logical operations                                 *
\****************************************************************************************/

#define IPCV_LOGIC( name )                                              \
IPCVAPI_EX( CvStatus, icv##name##_8u_C1R,                               \
    "ippi" #name "_8u_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,           \
( const uchar* src1, int srcstep1, const uchar* src2, int srcstep2,     \
  uchar* dst, int dststep, CvSize size ))

IPCV_LOGIC( And )
IPCV_LOGIC( Or )
IPCV_LOGIC( Xor )

#undef IPCV_LOGIC

IPCVAPI_EX( CvStatus, icvNot_8u_C1R, "ippiNot_8u_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),
( const uchar* src, int step1, uchar* dst, int step, CvSize size ))

/****************************************************************************************\
*                                Image Statistics                                        *
\****************************************************************************************/

///////////////////////////////////////// Mean //////////////////////////////////////////

#define IPCV_DEF_MEAN_MASK( flavor, srctype )           \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C1MR,          \
"ippiMean_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV), \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))            \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C2MR,          \
"ippiMean_" #flavor "_C2MR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/, \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))            \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C3MR,          \
"ippiMean_" #flavor "_C3MR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/, \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))            \
IPCVAPI_EX( CvStatus, icvMean_##flavor##_C4MR,          \
"ippiMean_" #flavor "_C4MR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/, \
( const srctype* img, int imgstep, const uchar* mask,   \
  int maskStep, CvSize size, double* mean ))

IPCV_DEF_MEAN_MASK( 8u, uchar )
IPCV_DEF_MEAN_MASK( 16u, ushort )
IPCV_DEF_MEAN_MASK( 16s, short )
IPCV_DEF_MEAN_MASK( 32s, int )
IPCV_DEF_MEAN_MASK( 32f, float )
IPCV_DEF_MEAN_MASK( 64f, double )

#undef IPCV_DEF_MEAN_MASK

//////////////////////////////////// Mean_StdDev ////////////////////////////////////////

#undef IPCV_MEAN_SDV_PLUGIN
#define ICV_MEAN_SDV_PLUGIN 0 /* CV_PLUGINS1(IPPCV) */

#define IPCV_DEF_MEAN_SDV( flavor, srctype )                                \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C1R,                        \
"ippiMean_StdDev_" #flavor "_C1R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C2R,                        \
"ippiMean_StdDev_" #flavor "_C2R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C3R,                        \
"ippiMean_StdDev_" #flavor "_C3R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C4R,                        \
"ippiMean_StdDev_" #flavor "_C4R", ICV_MEAN_SDV_PLUGIN,                     \
( const srctype* img, int imgstep, CvSize size, double* mean, double* sdv ))\
                                                                            \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C1MR,                       \
"ippiMean_StdDev_" #flavor "_C1MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,                                          \
  const uchar* mask, int maskStep,                                          \
  CvSize size, double* mean, double* sdv ))                                 \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C2MR,                       \
"ippiMean_StdDev_" #flavor "_C2MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,  const uchar* mask, int maskStep,        \
  CvSize size, double* mean, double* sdv ))                                 \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C3MR,                       \
"ippiMean_StdDev_" #flavor "_C3MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,                                          \
  const uchar* mask, int maskStep,                                          \
  CvSize size, double* mean, double* sdv ))                                 \
IPCVAPI_EX( CvStatus, icvMean_StdDev_##flavor##_C4MR,                       \
"ippiMean_StdDev_" #flavor "_C4MR", ICV_MEAN_SDV_PLUGIN,                    \
( const srctype* img, int imgstep,                                          \
  const uchar* mask, int maskStep,                                          \
  CvSize size, double* mean, double* sdv ))

IPCV_DEF_MEAN_SDV( 8u, uchar )
IPCV_DEF_MEAN_SDV( 16u, ushort )
IPCV_DEF_MEAN_SDV( 16s, short )
IPCV_DEF_MEAN_SDV( 32s, int )
IPCV_DEF_MEAN_SDV( 32f, float )
IPCV_DEF_MEAN_SDV( 64f, double )

#undef IPCV_DEF_MEAN_SDV
#undef IPCV_MEAN_SDV_PLUGIN

//////////////////////////////////// MinMaxIndx /////////////////////////////////////////

#define IPCV_DEF_MIN_MAX_LOC( flavor, srctype, extrtype )       \
IPCVAPI_EX( CvStatus, icvMinMaxIndx_##flavor##_C1R,             \
"ippiMinMaxIndx_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV), \
( const srctype* img, int imgstep,                              \
  CvSize size, extrtype* minVal, extrtype* maxVal,              \
  CvPoint* minLoc, CvPoint* maxLoc ))                           \
                                                                \
IPCVAPI_EX( CvStatus, icvMinMaxIndx_##flavor##_C1MR,            \
"ippiMinMaxIndx_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV),\
( const srctype* img, int imgstep,                              \
  const uchar* mask, int maskStep,                              \
  CvSize size, extrtype* minVal, extrtype* maxVal,              \
  CvPoint* minLoc, CvPoint* maxLoc ))

IPCV_DEF_MIN_MAX_LOC( 8u, uchar, float )
IPCV_DEF_MIN_MAX_LOC( 16u, ushort, float )
IPCV_DEF_MIN_MAX_LOC( 16s, short, float )
IPCV_DEF_MIN_MAX_LOC( 32s, int, double )
IPCV_DEF_MIN_MAX_LOC( 32f, int, float )
IPCV_DEF_MIN_MAX_LOC( 64f, int64, double )

#undef IPCV_DEF_MIN_MAX_LOC

////////////////////////////////////////// Sum //////////////////////////////////////////

#define IPCV_DEF_SUM_NOHINT( flavor, srctype )                              \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C1R,                                \
            "ippiSum_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C2R,                                \
           "ippiSum_" #flavor "_C2R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C3R,                                \
           "ippiSum_" #flavor "_C3R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C4R,                                \
           "ippiSum_" #flavor "_C4R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))

#define IPCV_DEF_SUM_NOHINT_NO_IPP( flavor, srctype )                       \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C1R,                                \
            "ippiSum_" #flavor "_C1R", 0,                                   \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C2R,                                \
           "ippiSum_" #flavor "_C2R", 0,                                    \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C3R,                                \
           "ippiSum_" #flavor "_C3R", 0,                                    \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C4R,                                \
           "ippiSum_" #flavor "_C4R", 0,                                    \
                                         ( const srctype* img, int imgstep, \
                                           CvSize size, double* sum ))

#define IPCV_DEF_SUM_HINT( flavor, srctype )                                \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C1R,                                \
            "ippiSum_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),         \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C2R,                                \
           "ippiSum_" #flavor "_C2R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C3R,                                \
           "ippiSum_" #flavor "_C3R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))      \
IPCVAPI_EX( CvStatus, icvSum_##flavor##_C4R,                                \
           "ippiSum_" #flavor "_C4R", CV_PLUGINS1(CV_PLUGIN_IPPI),          \
                        ( const srctype* img, int imgstep,                  \
                          CvSize size, double* sum, CvHintAlgorithm ))

IPCV_DEF_SUM_NOHINT( 8u, uchar )
IPCV_DEF_SUM_NOHINT( 16s, short )
IPCV_DEF_SUM_NOHINT_NO_IPP( 16u, ushort )
IPCV_DEF_SUM_NOHINT( 32s, int )
IPCV_DEF_SUM_HINT( 32f, float )
IPCV_DEF_SUM_NOHINT( 64f, double )

#undef IPCV_DEF_SUM_NOHINT
#undef IPCV_DEF_SUM_HINT

////////////////////////////////////////// CountNonZero /////////////////////////////////

#define IPCV_DEF_NON_ZERO( flavor, srctype )                            \
IPCVAPI_EX( CvStatus, icvCountNonZero_##flavor##_C1R,                   \
    "ippiCountNonZero_" #flavor "_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,   \
    ( const srctype* img, int imgstep, CvSize size, int* nonzero ))

IPCV_DEF_NON_ZERO( 8u, uchar )
IPCV_DEF_NON_ZERO( 16s, ushort )
IPCV_DEF_NON_ZERO( 32s, int )
IPCV_DEF_NON_ZERO( 32f, int )
IPCV_DEF_NON_ZERO( 64f, int64 )

#undef IPCV_DEF_NON_ZERO

////////////////////////////////////////// Norms /////////////////////////////////

#define IPCV_DEF_NORM_NOHINT_C1( flavor, srctype )                                      \
IPCVAPI_EX( CvStatus, icvNorm_Inf_##flavor##_C1R,                                       \
            "ippiNorm_Inf_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                \
                                             ( const srctype* img, int imgstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNorm_L1_##flavor##_C1R,                                        \
           "ippiNorm_L1_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                  \
                                             ( const srctype* img, int imgstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNorm_L2_##flavor##_C1R,                                        \
           "ippiNorm_L2_" #flavor "_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,             \
                                             ( const srctype* img, int imgstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_Inf_##flavor##_C1R,                                   \
           "ippiNormDiff_Inf_" #flavor "_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,        \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L1_##flavor##_C1R,                                    \
           "ippiNormDiff_L1_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),              \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L2_##flavor##_C1R,                                    \
           "ippiNormDiff_L2_" #flavor "_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,         \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               CvSize size, double* norm ))

#define IPCV_DEF_NORM_HINT_C1( flavor, srctype )                                        \
IPCVAPI_EX( CvStatus, icvNorm_Inf_##flavor##_C1R,                                       \
            "ippiNorm_Inf_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                \
                                        ( const srctype* img, int imgstep,              \
                                          CvSize size, double* norm ))                  \
IPCVAPI_EX( CvStatus, icvNorm_L1_##flavor##_C1R,                                        \
           "ippiNorm_L1_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                  \
                                        ( const srctype* img, int imgstep,              \
                                          CvSize size, double* norm, CvHintAlgorithm )) \
IPCVAPI_EX( CvStatus, icvNorm_L2_##flavor##_C1R,                                        \
           "ippiNorm_L2_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),                  \
                                        ( const srctype* img, int imgstep,              \
                                          CvSize size, double* norm, CvHintAlgorithm )) \
IPCVAPI_EX( CvStatus, icvNormDiff_Inf_##flavor##_C1R,                                   \
           "ippiNormDiff_Inf_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),             \
                                        ( const srctype* img1, int imgstep1,            \
                                          const srctype* img2, int imgstep2,            \
                                          CvSize size, double* norm ))                  \
IPCVAPI_EX( CvStatus, icvNormDiff_L1_##flavor##_C1R,                                    \
           "ippiNormDiff_L1_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),              \
                                        ( const srctype* img1, int imgstep1,            \
                                          const srctype* img2, int imgstep2,            \
                                          CvSize size, double* norm, CvHintAlgorithm )) \
IPCVAPI_EX( CvStatus, icvNormDiff_L2_##flavor##_C1R,                                    \
           "ippiNormDiff_L2_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),              \
                                        ( const srctype* img1, int imgstep1,            \
                                          const srctype* img2, int imgstep2,            \
                                          CvSize size, double* norm, CvHintAlgorithm ))

#define IPCV_DEF_NORM_MASK_C1( flavor, srctype )                                        \
IPCVAPI_EX( CvStatus, icvNorm_Inf_##flavor##_C1MR,                                      \
           "ippiNorm_Inf_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV),               \
                                             ( const srctype* img, int imgstep,         \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNorm_L1_##flavor##_C1MR,                                       \
            "ippiNorm_L1_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV),               \
                                             ( const srctype* img, int imgstep,         \
                                                const uchar* mask, int maskstep,        \
                                                CvSize size, double* norm ))            \
IPCVAPI_EX( CvStatus, icvNorm_L2_##flavor##_C1MR,                                       \
           "ippiNorm_L2_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV),                \
                                             ( const srctype* img, int imgstep,         \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_Inf_##flavor##_C1MR,                                  \
           "ippiNormDiff_Inf_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV),           \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L1_##flavor##_C1MR,                                   \
           "ippiNormDiff_L1_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV),            \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))             \
IPCVAPI_EX( CvStatus, icvNormDiff_L2_##flavor##_C1MR,                                   \
           "ippiNormDiff_L2_" #flavor "_C1MR", CV_PLUGINS1(CV_PLUGIN_IPPCV),            \
                                             ( const srctype* img1, int imgstep1,       \
                                               const srctype* img2, int imgstep2,       \
                                               const uchar* mask, int maskstep,         \
                                               CvSize size, double* norm ))

IPCV_DEF_NORM_NOHINT_C1( 8u, uchar )
IPCV_DEF_NORM_MASK_C1( 8u, uchar )

IPCV_DEF_NORM_NOHINT_C1( 16u, ushort )
IPCV_DEF_NORM_MASK_C1( 16u, ushort )

IPCV_DEF_NORM_NOHINT_C1( 16s, short )
IPCV_DEF_NORM_MASK_C1( 16s, short )

IPCV_DEF_NORM_NOHINT_C1( 32s, int )
IPCV_DEF_NORM_MASK_C1( 32s, int )

IPCV_DEF_NORM_HINT_C1( 32f, float )
IPCV_DEF_NORM_MASK_C1( 32f, float )

IPCV_DEF_NORM_NOHINT_C1( 64f, double )
IPCV_DEF_NORM_MASK_C1( 64f, double )

#undef IPCV_DEF_NORM_HONINT_C1
#undef IPCV_DEF_NORM_HINT_C1
#undef IPCV_DEF_NORM_MASK_C1


////////////////////////////////////// AbsDiff ///////////////////////////////////////////

#define IPCV_ABS_DIFF( flavor, arrtype )                    \
IPCVAPI_EX( CvStatus, icvAbsDiff_##flavor##_C1R,            \
"ippiAbsDiff_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPCV),\
( const arrtype* src1, int srcstep1,                        \
  const arrtype* src2, int srcstep2,                        \
  arrtype* dst, int dststep, CvSize size ))

IPCV_ABS_DIFF( 8u, uchar )
IPCV_ABS_DIFF( 16u, ushort )
IPCV_ABS_DIFF( 16s, short )
IPCV_ABS_DIFF( 32s, int )
IPCV_ABS_DIFF( 32f, float )
IPCV_ABS_DIFF( 64f, double )

#undef IPCV_ABS_DIFF

////////////////////////////// Comparisons //////////////////////////////////////////

#define IPCV_CMP( arrtype, flavor )                                                 \
IPCVAPI_EX( CvStatus, icvCompare_##flavor##_C1R,                                    \
            "ippiCompare_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),             \
            ( const arrtype* src1, int srcstep1, const arrtype* src2, int srcstep2, \
              arrtype* dst, int dststep, CvSize size, int cmp_op ))                 \
IPCVAPI_EX( CvStatus, icvCompareC_##flavor##_C1R,                                   \
            "ippiCompareC_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),            \
            ( const arrtype* src1, int srcstep1, arrtype scalar,                    \
              arrtype* dst, int dststep, CvSize size, int cmp_op ))                 \
IPCVAPI_EX( CvStatus, icvThreshold_GT_##flavor##_C1R,                               \
            "ippiThreshold_GT_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
            ( const arrtype* pSrc, int srcstep, arrtype* pDst, int dststep,         \
              CvSize size, arrtype threshold ))                                     \
IPCVAPI_EX( CvStatus, icvThreshold_LT_##flavor##_C1R,                               \
            "ippiThreshold_LT_" #flavor "_C1R", CV_PLUGINS1(CV_PLUGIN_IPPI),        \
            ( const arrtype* pSrc, int srcstep, arrtype* pDst, int dststep,         \
              CvSize size, arrtype threshold ))
IPCV_CMP( uchar, 8u )
IPCV_CMP( short, 16s )
IPCV_CMP( float, 32f )
#undef IPCV_CMP

/****************************************************************************************\
*                                       Utilities                                        *
\****************************************************************************************/

////////////////////////////// Copy Pixel <-> Plane /////////////////////////////////

#define IPCV_PIX_PLANE( flavor, arrtype )                                           \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C2P2R,                                     \
    "ippiCopy_" #flavor "_C2P2R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,                 \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C3P3R,                                     \
    "ippiCopy_" #flavor "_C3P3R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C4P4R,                                     \
    "ippiCopy_" #flavor "_C4P4R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype* src, int srcstep, arrtype** dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_CnC1CR,                                    \
    "ippiCopy_" #flavor "_CnC1CR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,               \
    ( const arrtype* src, int srcstep, arrtype* dst, int dststep,                   \
      CvSize size, int cn, int coi ))                                               \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_C1CnCR,                                    \
    "ippiCopy_" #flavor "_CnC1CR", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,               \
    ( const arrtype* src, int srcstep, arrtype* dst, int dststep,                   \
      CvSize size, int cn, int coi ))                                               \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P2C2R,                                     \
    "ippiCopy_" #flavor "_P2C2R", 0/*CV_PLUGINS1(CV_PLUGIN_IPPI)*/,                 \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P3C3R,                                     \
    "ippiCopy_" #flavor "_P3C3R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))   \
IPCVAPI_EX( CvStatus, icvCopy_##flavor##_P4C4R,                                     \
    "ippiCopy_" #flavor "_P4C4R", CV_PLUGINS1(CV_PLUGIN_IPPI),                      \
    ( const arrtype** src, int srcstep, arrtype* dst, int dststep, CvSize size ))

IPCV_PIX_PLANE( 8u, uchar )
IPCV_PIX_PLANE( 16s, ushort )
IPCV_PIX_PLANE( 32f, int )
IPCV_PIX_PLANE( 64f, int64 )

#undef IPCV_PIX_PLANE

/****************************************************************************************/
/*                            Math routines and RNGs                                    */
/****************************************************************************************/

IPCVAPI_EX( CvStatus, icvInvSqrt_32f, "ippsInvSqrt_32f_A21",
           CV_PLUGINS1(CV_PLUGIN_IPPVM),
           ( const float* src, float* dst, int len ))
IPCVAPI_EX( CvStatus, icvSqrt_32f, "ippsSqrt_32f_A21, ippsSqrt_32f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const float* src, float* dst, int len ))
IPCVAPI_EX( CvStatus, icvInvSqrt_64f, "ippsInvSqrt_64f_A50",
           CV_PLUGINS1(CV_PLUGIN_IPPVM),
           ( const double* src, double* dst, int len ))
IPCVAPI_EX( CvStatus, icvSqrt_64f, "ippsSqrt_64f_A50, ippsSqrt_64f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const double* src, double* dst, int len ))

IPCVAPI_EX( CvStatus, icvLog_32f, "ippsLn_32f_A21, ippsLn_32f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const float *x, float *y, int n ) )
IPCVAPI_EX( CvStatus, icvLog_64f, "ippsLn_64f_A50, ippsLn_64f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const double *x, double *y, int n ) )
IPCVAPI_EX( CvStatus, icvExp_32f, "ippsExp_32f_A21, ippsExp_32f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const float *x, float *y, int n ) )
IPCVAPI_EX( CvStatus, icvExp_64f, "ippsExp_64f_A50, ippsExp_64f",
           CV_PLUGINS2(CV_PLUGIN_IPPVM,CV_PLUGIN_IPPS),
           ( const double *x, double *y, int n ) )
IPCVAPI_EX( CvStatus, icvFastArctan_32f, "ippibFastArctan_32f",
           CV_PLUGINS1(CV_PLUGIN_IPPCV),
           ( const float* y, const float* x, float* angle, int len ))

/****************************************************************************************/
/*                                  Error handling functions                            */
/****************************************************************************************/

IPCVAPI_EX( CvStatus, icvCheckArray_32f_C1R,
           "ippiCheckArray_32f_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,
           ( const float* src, int srcstep,
             CvSize size, int flags,
             double min_val, double max_val ))

IPCVAPI_EX( CvStatus, icvCheckArray_64f_C1R,
           "ippiCheckArray_64f_C1R", 0/*CV_PLUGINS1(CV_PLUGIN_OPTCV)*/,
           ( const double* src, int srcstep,
             CvSize size, int flags,
             double min_val, double max_val ))

/****************************************************************************************/
/*                    Affine transformations of matrix/image elements                   */
/****************************************************************************************/

#define IPCV_TRANSFORM( suffix, ipp_suffix, cn )                \
IPCVAPI_EX( CvStatus, icvColorTwist##suffix##_C##cn##R,         \
        "ippiColorTwist" #ipp_suffix "_C" #cn                   \
        "R,ippiColorTwist" #ipp_suffix "_C" #cn "R",            \
        CV_PLUGINS2(CV_PLUGIN_IPPI, CV_PLUGIN_IPPCC),           \
        ( const void* src, int srcstep, void* dst, int dststep, \
          CvSize roisize, const float* twist_matrix ))

IPCV_TRANSFORM( _8u, 32f_8u, 3 )
IPCV_TRANSFORM( _16u, 32f_16u, 3 )
IPCV_TRANSFORM( _16s, 32f_16s, 3 )
IPCV_TRANSFORM( _32f, _32f, 3 )
IPCV_TRANSFORM( _32f, _32f, 4 )

#undef IPCV_TRANSFORM

#define IPCV_TRANSFORM_N1( suffix )                             \
IPCVAPI_EX( CvStatus, icvColorToGray##suffix,                   \
        "ippiColorToGray" #suffix ",ippiColorToGray" #suffix,   \
        CV_PLUGINS2(CV_PLUGIN_IPPI,CV_PLUGIN_IPPCC),            \
        ( const void* src, int srcstep, void* dst, int dststep, \
          CvSize roisize, const float* coeffs ))

IPCV_TRANSFORM_N1( _8u_C3C1R )
IPCV_TRANSFORM_N1( _16u_C3C1R )
IPCV_TRANSFORM_N1( _16s_C3C1R )
IPCV_TRANSFORM_N1( _32f_C3C1R )
IPCV_TRANSFORM_N1( _8u_AC4C1R )
IPCV_TRANSFORM_N1( _16u_AC4C1R )
IPCV_TRANSFORM_N1( _16s_AC4C1R )
IPCV_TRANSFORM_N1( _32f_AC4C1R )

#undef IPCV_TRANSFORM_N1

/****************************************************************************************/
/*                  Matrix routines from BLAS/LAPACK compatible libraries               */
/****************************************************************************************/

IPCVAPI_C_EX( void, icvBLAS_GEMM_32f, "sgemm, mkl_sgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_64f, "dgemm, mkl_dgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_32fc, "cgemm, mkl_cgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))

IPCVAPI_C_EX( void, icvBLAS_GEMM_64fc, "zgemm, mkl_zgemm", CV_PLUGINS2(CV_PLUGIN_MKL,CV_PLUGIN_MKL),
                        (const char *transa, const char *transb, int *n, int *m, int *k,
                         const void *alpha, const void *a, int *lda, const void *b, int *ldb,
                         const void *beta, void *c, int *ldc ))


#define IPCV_DFT( init_flavor, fwd_flavor, inv_flavor )                                 \
IPCVAPI_EX( CvStatus, icvDFTInitAlloc_##init_flavor, "ippsDFTInitAlloc_" #init_flavor,  \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( void**, int, int, CvHintAlgorithm ))         \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTFree_##init_flavor, "ippsDFTFree_" #init_flavor,            \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( void* ))                                     \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTGetBufSize_##init_flavor, "ippsDFTGetBufSize_" #init_flavor,\
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const void* spec, int* buf_size ))           \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTFwd_##fwd_flavor, "ippsDFTFwd_" #fwd_flavor,                \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const void* src, void* dst,                  \
            const void* spec, void* buffer ))                                           \
                                                                                        \
IPCVAPI_EX( CvStatus, icvDFTInv_##inv_flavor, "ippsDFTInv_" #inv_flavor,                \
            CV_PLUGINS1(CV_PLUGIN_IPPS), ( const void* src, void* dst,                  \
            const void* spec, void* buffer ))

IPCV_DFT( C_32fc, CToC_32fc, CToC_32fc )
IPCV_DFT( R_32f, RToPack_32f, PackToR_32f )
IPCV_DFT( C_64fc, CToC_64fc, CToC_64fc )
IPCV_DFT( R_64f, RToPack_64f, PackToR_64f )
#undef IPCV_DFT

#endif /*_CXCORE_IPP_H_*/
// -128.f ... 255.f
extern const float icv8x32fTab[];
#define CV_8TO32F(x)  icv8x32fTab[(x)+128]

extern const ushort icv8x16uSqrTab[];
#define CV_SQR_8U(x)  icv8x16uSqrTab[(x)+255]

extern const char* icvHersheyGlyphs[];
const int icvPixSize[] = 
{
	sizeof(uchar)*1, sizeof(char)*1, sizeof(ushort)*1, sizeof(short)*1,
	sizeof(int)*1, sizeof(float)*1, sizeof(double)*1, 0,

	sizeof(uchar)*2, sizeof(char)*2, sizeof(ushort)*2, sizeof(short)*2,
	sizeof(int)*2, sizeof(float)*2, sizeof(double)*2, 0,

	sizeof(uchar)*3, sizeof(char)*3, sizeof(ushort)*3, sizeof(short)*3,
	sizeof(int)*3, sizeof(float)*3, sizeof(double)*3, 0,

	sizeof(uchar)*4, sizeof(char)*4, sizeof(ushort)*4, sizeof(short)*4,
	sizeof(int)*4, sizeof(float)*4, sizeof(double)*4, 0
};
extern const signed char icvDepthToType[];

#define icvIplToCvDepth( depth ) \
    icvDepthToType[(((depth) & 255) >> 2) + ((depth) < 0)]

extern const uchar icvSaturate8u[];
#define CV_FAST_CAST_8U(t)   (assert(-256 <= (t) && (t) <= 512), icvSaturate8u[(t)+256])
#define CV_MIN_8U(a,b)       ((a) - CV_FAST_CAST_8U((a) - (b)))
#define CV_MAX_8U(a,b)       ((a) + CV_FAST_CAST_8U((b) - (a)))

typedef CvFunc2D_3A1I CvArithmBinMaskFunc2D;
typedef CvFunc2D_2A1P1I CvArithmUniMaskFunc2D;


/****************************************************************************************\
*                                   Complex arithmetics                                  *
\****************************************************************************************/

struct CvComplex32f;
struct CvComplex64f;

struct CvComplex32f
{
    float re, im;

    CvComplex32f() {}
    CvComplex32f( float _re, float _im=0 ) : re(_re), im(_im) {}
    explicit CvComplex32f( const CvComplex64f& v );
    //CvComplex32f( const CvComplex32f& v ) : re(v.re), im(v.im) {}
    //CvComplex32f& operator = (const CvComplex32f& v ) { re = v.re; im = v.im; return *this; }
    operator CvComplex64f() const;
};

struct CvComplex64f
{
    double re, im;

    CvComplex64f() {}
    CvComplex64f( double _re, double _im=0 ) : re(_re), im(_im) {}
    explicit CvComplex64f( const CvComplex32f& v );
    //CvComplex64f( const CvComplex64f& v ) : re(v.re), im(v.im) {}
    //CvComplex64f& operator = (const CvComplex64f& v ) { re = v.re; im = v.im; return *this; }
    operator CvComplex32f() const;
};

inline CvComplex32f::CvComplex32f( const CvComplex64f& v ) : re((float)v.re), im((float)v.im) {}
inline CvComplex64f::CvComplex64f( const CvComplex32f& v ) : re(v.re), im(v.im) {}

inline CvComplex32f operator + (CvComplex32f a, CvComplex32f b)
{
    return CvComplex32f( a.re + b.re, a.im + b.im );
}

inline CvComplex32f& operator += (CvComplex32f& a, CvComplex32f b)
{
    a.re += b.re;
    a.im += b.im;
    return a;
}

inline CvComplex32f operator - (CvComplex32f a, CvComplex32f b)
{
    return CvComplex32f( a.re - b.re, a.im - b.im );
}

inline CvComplex32f& operator -= (CvComplex32f& a, CvComplex32f b)
{
    a.re -= b.re;
    a.im -= b.im;
    return a;
}

inline CvComplex32f operator - (CvComplex32f a)
{
    return CvComplex32f( -a.re, -a.im );
}

inline CvComplex32f operator * (CvComplex32f a, CvComplex32f b)
{
    return CvComplex32f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}

inline double abs(CvComplex32f a)
{
    return sqrt( (double)a.re*a.re + (double)a.im*a.im );
}

inline CvComplex32f conj(CvComplex32f a)
{
    return CvComplex32f( a.re, -a.im );
}


inline CvComplex32f operator / (CvComplex32f a, CvComplex32f b)
{
    double t = 1./((double)b.re*b.re + (double)b.im*b.im);
    return CvComplex32f( (float)((a.re*b.re + a.im*b.im)*t),
                         (float)((-a.re*b.im + a.im*b.re)*t) );
}

inline CvComplex32f operator * (double a, CvComplex32f b)
{
    return CvComplex32f( (float)(a*b.re), (float)(a*b.im) );
}

inline CvComplex32f operator * (CvComplex32f a, double b)
{
    return CvComplex32f( (float)(a.re*b), (float)(a.im*b) );
}

inline CvComplex32f::operator CvComplex64f() const
{
    return CvComplex64f(re,im);
}


inline CvComplex64f operator + (CvComplex64f a, CvComplex64f b)
{
    return CvComplex64f( a.re + b.re, a.im + b.im );
}

inline CvComplex64f& operator += (CvComplex64f& a, CvComplex64f b)
{
    a.re += b.re;
    a.im += b.im;
    return a;
}

inline CvComplex64f operator - (CvComplex64f a, CvComplex64f b)
{
    return CvComplex64f( a.re - b.re, a.im - b.im );
}

inline CvComplex64f& operator -= (CvComplex64f& a, CvComplex64f b)
{
    a.re -= b.re;
    a.im -= b.im;
    return a;
}

inline CvComplex64f operator - (CvComplex64f a)
{
    return CvComplex64f( -a.re, -a.im );
}

inline CvComplex64f operator * (CvComplex64f a, CvComplex64f b)
{
    return CvComplex64f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}

inline double abs(CvComplex64f a)
{
    return sqrt( (double)a.re*a.re + (double)a.im*a.im );
}

inline CvComplex64f operator / (CvComplex64f a, CvComplex64f b)
{
    double t = 1./((double)b.re*b.re + (double)b.im*b.im);
    return CvComplex64f( (a.re*b.re + a.im*b.im)*t,
                         (-a.re*b.im + a.im*b.re)*t );
}

inline CvComplex64f operator * (double a, CvComplex64f b)
{
    return CvComplex64f( a*b.re, a*b.im );
}

inline CvComplex64f operator * (CvComplex64f a, double b)
{
    return CvComplex64f( a.re*b, a.im*b );
}

inline CvComplex64f::operator CvComplex32f() const
{
    return CvComplex32f((float)re,(float)im);
}

inline CvComplex64f conj(CvComplex64f a)
{
    return CvComplex64f( a.re, -a.im );
}

inline CvComplex64f operator + (CvComplex64f a, CvComplex32f b)
{
    return CvComplex64f( a.re + b.re, a.im + b.im );
}

inline CvComplex64f operator + (CvComplex32f a, CvComplex64f b)
{
    return CvComplex64f( a.re + b.re, a.im + b.im );
}

inline CvComplex64f operator - (CvComplex64f a, CvComplex32f b)
{
    return CvComplex64f( a.re - b.re, a.im - b.im );
}

inline CvComplex64f operator - (CvComplex32f a, CvComplex64f b)
{
    return CvComplex64f( a.re - b.re, a.im - b.im );
}

inline CvComplex64f operator * (CvComplex64f a, CvComplex32f b)
{
    return CvComplex64f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}

inline CvComplex64f operator * (CvComplex32f a, CvComplex64f b)
{
    return CvComplex64f( a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re );
}


typedef CvStatus (CV_STDCALL * CvCopyMaskFunc)(const void* src, int src_step,
                                               void* dst, int dst_step, CvSize size,
                                               const void* mask, int mask_step);

CvCopyMaskFunc icvGetCopyMaskFunc( int elem_size );

CvStatus CV_STDCALL icvSetZero_8u_C1R( uchar* dst, int dststep, CvSize size );

CvStatus CV_STDCALL icvScale_32f( const float* src, float* dst, int len, float a, float b );
CvStatus CV_STDCALL icvScale_64f( const double* src, double* dst, int len, double a, double b );

CvStatus CV_STDCALL icvLUT_Transform8u_8u_C1R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_16u_C1R( const uchar* src, int srcstep, ushort* dst,
                                                int dststep, CvSize size, const ushort* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_32s_C1R( const uchar* src, int srcstep, int* dst,
                                                int dststep, CvSize size, const int* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_64f_C1R( const uchar* src, int srcstep, double* dst,
                                                int dststep, CvSize size, const double* lut );

CvStatus CV_STDCALL icvLUT_Transform8u_8u_C2R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_8u_C3R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );
CvStatus CV_STDCALL icvLUT_Transform8u_8u_C4R( const uchar* src, int srcstep, uchar* dst,
                                               int dststep, CvSize size, const uchar* lut );

typedef CvStatus (CV_STDCALL * CvLUT_TransformFunc)( const void* src, int srcstep, void* dst,
                                                     int dststep, CvSize size, const void* lut );

CV_INLINE CvStatus
icvLUT_Transform8u_8s_C1R( const uchar* src, int srcstep, char* dst,
                            int dststep, CvSize size, const char* lut )
{
    return icvLUT_Transform8u_8u_C1R( src, srcstep, (uchar*)dst,
                                      dststep, size, (const uchar*)lut );
}

CV_INLINE CvStatus
icvLUT_Transform8u_16s_C1R( const uchar* src, int srcstep, short* dst,
                            int dststep, CvSize size, const short* lut )
{
    return icvLUT_Transform8u_16u_C1R( src, srcstep, (ushort*)dst,
                                       dststep, size, (const ushort*)lut );
}

CV_INLINE CvStatus
icvLUT_Transform8u_32f_C1R( const uchar* src, int srcstep, float* dst,
                            int dststep, CvSize size, const float* lut )
{
    return icvLUT_Transform8u_32s_C1R( src, srcstep, (int*)dst,
                                       dststep, size, (const int*)lut );
}

#endif /*_CXCORE_INTERNAL_H_*/
