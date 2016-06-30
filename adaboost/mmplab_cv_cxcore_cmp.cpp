#include "stdafx.h"
#include "mmplab_cxcore.h"

/****************************************************************************************\
*                                      InRange[S]                                        *
\****************************************************************************************/

#define ICV_DEF_IN_RANGE_CASE_C1( worktype, _toggle_macro_ )    \
for( x = 0; x < size.width; x++ )                               \
{                                                               \
    worktype a1 = _toggle_macro_(src1[x]),                      \
             a2 = src2[x], a3 = src3[x];                        \
    dst[x] = (uchar)-(_toggle_macro_(a2) <= a1 &&               \
                     a1 < _toggle_macro_(a3));                  \
}


#define ICV_DEF_IN_RANGE_CASE_C2( worktype, _toggle_macro_ )        \
for( x = 0; x < size.width; x++ )                                   \
{                                                                   \
    worktype a1 = _toggle_macro_(src1[x*2]),                        \
             a2 = src2[x*2], a3 = src3[x*2];                        \
    int f = _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);    \
    a1 = _toggle_macro_(src1[x*2+1]);                               \
    a2 = src2[x*2+1];                                               \
    a3 = src3[x*2+1];                                               \
    f &= _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);       \
    dst[x] = (uchar)-f;                                             \
}


#define ICV_DEF_IN_RANGE_CASE_C3( worktype, _toggle_macro_ )        \
for( x = 0; x < size.width; x++ )                                   \
{                                                                   \
    worktype a1 = _toggle_macro_(src1[x*3]),                        \
             a2 = src2[x*3], a3 = src3[x*3];                        \
    int f = _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);    \
    a1 = _toggle_macro_(src1[x*3+1]);                               \
    a2 = src2[x*3+1];                                               \
    a3 = src3[x*3+1];                                               \
    f &= _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);       \
    a1 = _toggle_macro_(src1[x*3+2]);                               \
    a2 = src2[x*3+2];                                               \
    a3 = src3[x*3+2];                                               \
    f &= _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);       \
    dst[x] = (uchar)-f;                                             \
}


#define ICV_DEF_IN_RANGE_CASE_C4( worktype, _toggle_macro_ )        \
for( x = 0; x < size.width; x++ )                                   \
{                                                                   \
    worktype a1 = _toggle_macro_(src1[x*4]),                        \
             a2 = src2[x*4], a3 = src3[x*4];                        \
    int f = _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);    \
    a1 = _toggle_macro_(src1[x*4+1]);                               \
    a2 = src2[x*4+1];                                               \
    a3 = src3[x*4+1];                                               \
    f &= _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);       \
    a1 = _toggle_macro_(src1[x*4+2]);                               \
    a2 = src2[x*4+2];                                               \
    a3 = src3[x*4+2];                                               \
    f &= _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);       \
    a1 = _toggle_macro_(src1[x*4+3]);                               \
    a2 = src2[x*4+3];                                               \
    a3 = src3[x*4+3];                                               \
    f &= _toggle_macro_(a2) <= a1 && a1 < _toggle_macro_(a3);       \
    dst[x] = (uchar)-f;                                             \
}


#define ICV_DEF_IN_RANGE_FUNC( flavor, arrtype, worktype,           \
                               _toggle_macro_, cn )                 \
static CvStatus CV_STDCALL                                          \
icvInRange_##flavor##_C##cn##R( const arrtype* src1, int step1,     \
                                const arrtype* src2, int step2,     \
                                const arrtype* src3, int step3,     \
                                uchar* dst, int step, CvSize size ) \
{                                                                   \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]);             \
    step3 /= sizeof(src3[0]); step /= sizeof(dst[0]);               \
                                                                    \
    for( ; size.height--; src1 += step1, src2 += step2,             \
                          src3 += step3, dst += step )              \
    {                                                               \
        int x;                                                      \
        ICV_DEF_IN_RANGE_CASE_C##cn( worktype, _toggle_macro_ )     \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}


#define ICV_DEF_IN_RANGE_CASE_CONST_C1( worktype, _toggle_macro_ )  \
for( x = 0; x < size.width; x++ )                                   \
{                                                                   \
    worktype a1 = _toggle_macro_(src1[x]);                          \
    dst[x] = (uchar)-(scalar[0] <= a1 && a1 < scalar[1]);           \
}


#define ICV_DEF_IN_RANGE_CASE_CONST_C2( worktype, _toggle_macro_ )  \
for( x = 0; x < size.width; x++ )                                   \
{                                                                   \
    worktype a1 = _toggle_macro_(src1[x*2]);                        \
    int f = scalar[0] <= a1 && a1 < scalar[2];                      \
    a1 = _toggle_macro_(src1[x*2+1]);                               \
    f &= scalar[1] <= a1 && a1 < scalar[3];                         \
    dst[x] = (uchar)-f;                                             \
}


#define ICV_DEF_IN_RANGE_CASE_CONST_C3( worktype, _toggle_macro_ )  \
for( x = 0; x < size.width; x++ )                                   \
{                                                                   \
    worktype a1 = _toggle_macro_(src1[x*3]);                        \
    int f = scalar[0] <= a1 && a1 < scalar[3];                      \
    a1 = _toggle_macro_(src1[x*3+1]);                               \
    f &= scalar[1] <= a1 && a1 < scalar[4];                         \
    a1 = _toggle_macro_(src1[x*3+2]);                               \
    f &= scalar[2] <= a1 && a1 < scalar[5];                         \
    dst[x] = (uchar)-f;                                             \
}


#define ICV_DEF_IN_RANGE_CASE_CONST_C4( worktype, _toggle_macro_ )  \
for( x = 0; x < size.width; x++ )                                   \
{                                                                   \
    worktype a1 = _toggle_macro_(src1[x*4]);                        \
    int f = scalar[0] <= a1 && a1 < scalar[4];                      \
    a1 = _toggle_macro_(src1[x*4+1]);                               \
    f &= scalar[1] <= a1 && a1 < scalar[5];                         \
    a1 = _toggle_macro_(src1[x*4+2]);                               \
    f &= scalar[2] <= a1 && a1 < scalar[6];                         \
    a1 = _toggle_macro_(src1[x*4+3]);                               \
    f &= scalar[3] <= a1 && a1 < scalar[7];                         \
    dst[x] = (uchar)-f;                                             \
}


#define ICV_DEF_IN_RANGE_CONST_FUNC( flavor, arrtype, worktype,     \
                                     _toggle_macro_, cn )           \
static CvStatus CV_STDCALL                                          \
icvInRangeC_##flavor##_C##cn##R( const arrtype* src1, int step1,    \
                                 uchar* dst, int step, CvSize size, \
                                 const worktype* scalar )           \
{                                                                   \
    step1 /= sizeof(src1[0]); step /= sizeof(dst[0]);               \
                                                                    \
    for( ; size.height--; src1 += step1, dst += step )              \
    {                                                               \
        int x;                                                      \
        ICV_DEF_IN_RANGE_CASE_CONST_C##cn( worktype, _toggle_macro_)\
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}


#define ICV_DEF_IN_RANGE_ALL( flavor, arrtype, worktype, _toggle_macro_ )   \
ICV_DEF_IN_RANGE_FUNC( flavor, arrtype, worktype, _toggle_macro_, 1 )       \
ICV_DEF_IN_RANGE_FUNC( flavor, arrtype, worktype, _toggle_macro_, 2 )       \
ICV_DEF_IN_RANGE_FUNC( flavor, arrtype, worktype, _toggle_macro_, 3 )       \
ICV_DEF_IN_RANGE_FUNC( flavor, arrtype, worktype, _toggle_macro_, 4 )       \
                                                                            \
ICV_DEF_IN_RANGE_CONST_FUNC( flavor, arrtype, worktype, _toggle_macro_, 1 ) \
ICV_DEF_IN_RANGE_CONST_FUNC( flavor, arrtype, worktype, _toggle_macro_, 2 ) \
ICV_DEF_IN_RANGE_CONST_FUNC( flavor, arrtype, worktype, _toggle_macro_, 3 ) \
ICV_DEF_IN_RANGE_CONST_FUNC( flavor, arrtype, worktype, _toggle_macro_, 4 )

ICV_DEF_IN_RANGE_ALL( 8u, uchar, int, CV_NOP )
ICV_DEF_IN_RANGE_ALL( 16u, ushort, int, CV_NOP )
ICV_DEF_IN_RANGE_ALL( 16s, short, int, CV_NOP )
ICV_DEF_IN_RANGE_ALL( 32s, int, int, CV_NOP )
ICV_DEF_IN_RANGE_ALL( 32f, float, float, CV_NOP )
ICV_DEF_IN_RANGE_ALL( 64f, double, double, CV_NOP )

#define icvInRange_8s_C1R 0
#define icvInRange_8s_C2R 0
#define icvInRange_8s_C3R 0
#define icvInRange_8s_C4R 0

#define icvInRangeC_8s_C1R 0
#define icvInRangeC_8s_C2R 0
#define icvInRangeC_8s_C3R 0
#define icvInRangeC_8s_C4R 0

CV_DEF_INIT_BIG_FUNC_TAB_2D( InRange, R )
CV_DEF_INIT_BIG_FUNC_TAB_2D( InRangeC, R )

typedef CvStatus (CV_STDCALL * CvInRangeCFunc)( const void* src, int srcstep,
                                                uchar* dst, int dststep,
                                                CvSize size, const void* scalar );

/*************************************** InRange ****************************************/

CV_IMPL void
cvInRange( const void* srcarr1, const void* srcarr2,
           const void* srcarr3, void* dstarr )
{
    static CvBigFuncTable inrange_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvInRange" );

    __BEGIN__;

    int type, coi = 0;
    int src1_step, src2_step, src3_step, dst_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat srcstub3, *src3 = (CvMat*)srcarr3;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    CvFunc2D_4A func;

    if( !inittab )
    {
        icvInitInRangeRTable( &inrange_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(src2) )
    {
        CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(src3) )
    {
        CV_CALL( src3 = cvGetMat( src3, &srcstub3, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(dst) )
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) ||
        !CV_ARE_TYPES_EQ( src1, src3 ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_IS_MASK_ARR( dst ))
        CV_ERROR( CV_StsUnsupportedFormat, "Destination image should be 8uC1 or 8sC1");

    if( !CV_ARE_SIZES_EQ( src1, src2 ) ||
        !CV_ARE_SIZES_EQ( src1, src3 ) ||
        !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & src2->type & src3->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = src2_step = src3_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        src2_step = src2->step;
        src3_step = src3->step;
        dst_step = dst->step;
    }

    if( CV_MAT_CN(type) > 4 )
        CV_ERROR( CV_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4" );

    func = (CvFunc2D_4A)(inrange_tab.fn_2d[type]);

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                     src3->data.ptr, src3_step, dst->data.ptr, dst_step, size ));

    __END__;
}


/************************************** InRangeS ****************************************/

CV_IMPL void
cvInRangeS( const void* srcarr, CvScalar lower, CvScalar upper, void* dstarr )
{
    static CvBigFuncTable inrange_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvInRangeS" );

    __BEGIN__;

    int sctype, type, coi = 0;
    int src1_step, dst_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    CvInRangeCFunc func;
    double buf[8];

    if( !inittab )
    {
        icvInitInRangeCRTable( &inrange_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(dst) )
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MASK_ARR( dst ))
        CV_ERROR( CV_StsUnsupportedFormat, "Destination image should be 8uC1 or 8sC1");

    if( !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    sctype = type = CV_MAT_TYPE(src1->type);
    if( CV_MAT_DEPTH(sctype) < CV_32S )
        sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;

    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        dst_step = dst->step;
    }

    if( CV_MAT_CN(type) > 4 )
        CV_ERROR( CV_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4" );

    func = (CvInRangeCFunc)(inrange_tab.fn_2d[type]);

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    cvScalarToRawData( &lower, buf, sctype, 0 );
    cvScalarToRawData( &upper, (char*)buf + CV_ELEM_SIZE(sctype), sctype, 0 );

    IPPI_CALL( func( src1->data.ptr, src1_step, dst->data.ptr,
                     dst_step, size, buf ));

    __END__;
}


/****************************************************************************************\
*                                         Cmp                                            *
\****************************************************************************************/

#define ICV_DEF_CMP_CASE_C1( __op__, _toggle_macro_ )                   \
for( x = 0; x <= size.width - 4; x += 4 )                               \
{                                                                       \
    int f0 = __op__( _toggle_macro_(src1[x]), _toggle_macro_(src2[x])); \
    int f1 = __op__( _toggle_macro_(src1[x+1]), _toggle_macro_(src2[x+1])); \
    dst[x] = (uchar)-f0;                                                \
    dst[x+1] = (uchar)-f1;                                              \
    f0 = __op__( _toggle_macro_(src1[x+2]), _toggle_macro_(src2[x+2])); \
    f1 = __op__( _toggle_macro_(src1[x+3]), _toggle_macro_(src2[x+3])); \
    dst[x+2] = (uchar)-f0;                                              \
    dst[x+3] = (uchar)-f1;                                              \
}                                                                       \
                                                                        \
for( ; x < size.width; x++ )                                            \
{                                                                       \
    int f0 = __op__( _toggle_macro_(src1[x]), _toggle_macro_(src2[x])); \
    dst[x] = (uchar)-f0;                                                \
}


#define ICV_DEF_CMP_FUNC( __op__, name, flavor, arrtype,        \
                          worktype, _toggle_macro_ )            \
static CvStatus CV_STDCALL                                      \
icv##name##_##flavor##_C1R( const arrtype* src1, int step1,     \
                            const arrtype* src2, int step2,     \
                            uchar* dst, int step, CvSize size ) \
{                                                               \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]);         \
    step /= sizeof(dst[0]);                                     \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          dst += step )                         \
    {                                                           \
        int x;                                                  \
        ICV_DEF_CMP_CASE_C1( __op__, _toggle_macro_ )           \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


#define ICV_DEF_CMP_CONST_CASE_C1( __op__, _toggle_macro_ )     \
for( x = 0; x <= size.width - 4; x += 4 )                       \
{                                                               \
    int f0 = __op__( _toggle_macro_(src1[x]), scalar );         \
    int f1 = __op__( _toggle_macro_(src1[x+1]), scalar );       \
    dst[x] = (uchar)-f0;                                        \
    dst[x+1] = (uchar)-f1;                                      \
    f0 = __op__( _toggle_macro_(src1[x+2]), scalar );           \
    f1 = __op__( _toggle_macro_(src1[x+3]), scalar );           \
    dst[x+2] = (uchar)-f0;                                      \
    dst[x+3] = (uchar)-f1;                                      \
}                                                               \
                                                                \
for( ; x < size.width; x++ )                                    \
{                                                               \
    int f0 = __op__( _toggle_macro_(src1[x]), scalar );         \
    dst[x] = (uchar)-f0;                                        \
}


#define ICV_DEF_CMP_CONST_FUNC( __op__, name, flavor, arrtype,  \
                                worktype, _toggle_macro_)       \
static CvStatus CV_STDCALL                                      \
icv##name##C_##flavor##_C1R( const arrtype* src1, int step1,    \
                             uchar* dst, int step,              \
                             CvSize size, worktype* pScalar )   \
{                                                               \
    worktype scalar = *pScalar;                                 \
    step1 /= sizeof(src1[0]); step /= sizeof(dst[0]);           \
                                                                \
    for( ; size.height--; src1 += step1, dst += step )          \
    {                                                           \
        int x;                                                  \
        ICV_DEF_CMP_CONST_CASE_C1( __op__, _toggle_macro_ )     \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


#define ICV_DEF_CMP_ALL( flavor, arrtype, worktype, _toggle_macro_ )            \
ICV_DEF_CMP_FUNC( CV_GT, CmpGT, flavor, arrtype, worktype, _toggle_macro_ )     \
ICV_DEF_CMP_FUNC( CV_EQ, CmpEQ, flavor, arrtype, worktype, _toggle_macro_ )     \
ICV_DEF_CMP_CONST_FUNC( CV_GT, CmpGT, flavor, arrtype, worktype, _toggle_macro_)\
ICV_DEF_CMP_CONST_FUNC( CV_GE, CmpGE, flavor, arrtype, worktype, _toggle_macro_)\
ICV_DEF_CMP_CONST_FUNC( CV_EQ, CmpEQ, flavor, arrtype, worktype, _toggle_macro_)

ICV_DEF_CMP_ALL( 8u, uchar, int, CV_NOP )
ICV_DEF_CMP_ALL( 16u, ushort, int, CV_NOP )
ICV_DEF_CMP_ALL( 16s, short, int, CV_NOP )
ICV_DEF_CMP_ALL( 32s, int, int, CV_NOP )
ICV_DEF_CMP_ALL( 32f, float, double, CV_NOP )
ICV_DEF_CMP_ALL( 64f, double, double, CV_NOP )

#define icvCmpGT_8s_C1R     0
#define icvCmpEQ_8s_C1R     0
#define icvCmpGTC_8s_C1R    0
#define icvCmpGEC_8s_C1R    0
#define icvCmpEQC_8s_C1R    0

CV_DEF_INIT_FUNC_TAB_2D( CmpGT, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpEQ, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpGTC, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpGEC, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CmpEQC, C1R )

icvCompare_8u_C1R_t icvCompare_8u_C1R_p = 0;
icvCompare_16s_C1R_t icvCompare_16s_C1R_p = 0;
icvCompare_32f_C1R_t icvCompare_32f_C1R_p = 0;

icvCompareC_8u_C1R_t icvCompareC_8u_C1R_p = 0;
icvCompareC_16s_C1R_t icvCompareC_16s_C1R_p = 0;
icvCompareC_32f_C1R_t icvCompareC_32f_C1R_p = 0;

icvThreshold_GT_8u_C1R_t icvThreshold_GT_8u_C1R_p = 0;
icvThreshold_GT_16s_C1R_t icvThreshold_GT_16s_C1R_p = 0;
icvThreshold_GT_32f_C1R_t icvThreshold_GT_32f_C1R_p = 0;

icvThreshold_LT_8u_C1R_t icvThreshold_LT_8u_C1R_p = 0;
icvThreshold_LT_16s_C1R_t icvThreshold_LT_16s_C1R_p = 0;
icvThreshold_LT_32f_C1R_t icvThreshold_LT_32f_C1R_p = 0;

/***************************************** cvCmp ****************************************/

CV_IMPL void
cvCmp( const void* srcarr1, const void* srcarr2,
       void* dstarr, int cmp_op )
{
    static CvFuncTable cmp_tab[2];
    static int inittab = 0;

    CV_FUNCNAME( "cvCmp" );

    __BEGIN__;

    int type, coi = 0;
    int invflag = 0;
    CvCmpOp ipp_cmp_op;
    int src1_step, src2_step, dst_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat *temp;
    CvSize size;
    CvFunc2D_3A func;

    if( !inittab )
    {
        icvInitCmpGTC1RTable( &cmp_tab[0] );
        icvInitCmpEQC1RTable( &cmp_tab[1] );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(src2) )
    {
        CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(dst) )
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    switch( cmp_op )
    {
    case CV_CMP_GT:
    case CV_CMP_EQ:
        break;
    case CV_CMP_GE:
        CV_SWAP( src1, src2, temp );
        invflag = 1;
        break;
    case CV_CMP_LT:
        CV_SWAP( src1, src2, temp );
        break;
    case CV_CMP_LE:
        invflag = 1;
        break;
    case CV_CMP_NE:
        cmp_op = CV_CMP_EQ;
        invflag = 1;
        break;
    default:
        CV_ERROR( CV_StsBadArg, "Unknown comparison operation" );
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( CV_MAT_CN( src1->type ) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input arrays must be single-channel");

    if( !CV_IS_MASK_ARR( dst ))
        CV_ERROR( CV_StsUnsupportedFormat, "Destination array should be 8uC1 or 8sC1");

    if( !CV_ARE_SIZES_EQ( src1, src2 ) ||
        !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = src2_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        src2_step = src2->step;
        dst_step = dst->step;
    }

    func = (CvFunc2D_3A)(cmp_tab[cmp_op == CV_CMP_EQ].fn_2d[type]);

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    ipp_cmp_op = cmp_op == CV_CMP_EQ ? cvCmpEq : cvCmpGreater;

    if( type == CV_8U && icvCompare_8u_C1R_p )
    {
        IPPI_CALL( icvCompare_8u_C1R_p( src1->data.ptr, src1_step, src2->data.ptr,
                            src2_step, dst->data.ptr, dst_step, size, ipp_cmp_op ));
    }
    else if( type == CV_16S && icvCompare_16s_C1R_p )
    {
        IPPI_CALL( icvCompare_16s_C1R_p( src1->data.s, src1_step, src2->data.s,
                            src2_step, dst->data.s, dst_step, size, ipp_cmp_op ));
    }
    else if( type == CV_32F && icvCompare_32f_C1R_p )
    {
        IPPI_CALL( icvCompare_32f_C1R_p( src1->data.fl, src1_step, src2->data.fl,
                            src2_step, dst->data.fl, dst_step, size, ipp_cmp_op ));
    }
    else
    {
        IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                         dst->data.ptr, dst_step, size ));
    }

    if( invflag )
        IPPI_CALL( icvNot_8u_C1R( dst->data.ptr, dst_step,
                           dst->data.ptr, dst_step, size ));

    __END__;
}


/*************************************** cvCmpS *****************************************/

CV_IMPL void
cvCmpS( const void* srcarr, double value, void* dstarr, int cmp_op )
{
    static CvFuncTable cmps_tab[3];
    static int inittab = 0;

    CV_FUNCNAME( "cvCmpS" );

    __BEGIN__;

    int y, type, coi = 0;
    int invflag = 0, ipp_cmp_op;
    int src1_step, dst_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    int ival = 0;

    if( !inittab )
    {
        icvInitCmpEQCC1RTable( &cmps_tab[CV_CMP_EQ] );
        icvInitCmpGTCC1RTable( &cmps_tab[CV_CMP_GT] );
        icvInitCmpGECC1RTable( &cmps_tab[CV_CMP_GE] );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(dst) )
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    switch( cmp_op )
    {
    case CV_CMP_GT:
    case CV_CMP_EQ:
    case CV_CMP_GE:
        break;
    case CV_CMP_LT:
        invflag = 1;
        cmp_op = CV_CMP_GE;
        break;
    case CV_CMP_LE:
        invflag = 1;
        cmp_op = CV_CMP_GT;
        break;
    case CV_CMP_NE:
        invflag = 1;
        cmp_op = CV_CMP_EQ;
        break;
    default:
        CV_ERROR( CV_StsBadArg, "Unknown comparison operation" );
    }

    if( !CV_IS_MASK_ARR( dst ))
        CV_ERROR( CV_StsUnsupportedFormat, "Destination array should be 8uC1 or 8sC1");

    if( CV_MAT_CN( src1->type ) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input array must be single-channel");

    if( !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        dst_step = dst->step;
    }

    if( CV_MAT_DEPTH(type) <= CV_32S )
    {
        ival = cvRound(value);
        if( type == CV_8U || type == CV_16S )
        {
            int minval = type == CV_8U ? 0 : -32768;
            int maxval = type == CV_8U ? 255 : 32767;
            int fillval = -1;
            if( ival < minval )
                fillval = cmp_op == CV_CMP_NE || cmp_op == CV_CMP_GE || cmp_op == CV_CMP_GT ? 255 : 0;
            else if( ival > maxval )
                fillval = cmp_op == CV_CMP_NE || cmp_op == CV_CMP_LE || cmp_op == CV_CMP_LT ? 255 : 0;
            if( fillval >= 0 )
            {
                fillval ^= invflag ? 255 : 0;
                for( y = 0; y < size.height; y++ )
                    memset( dst->data.ptr + y*dst_step, fillval, size.width );
                EXIT;
            }
        }
    }

    ipp_cmp_op = cmp_op == CV_CMP_EQ ? cvCmpEq :
                 cmp_op == CV_CMP_GE ? cvCmpGreaterEq : cvCmpGreater;
    if( type == CV_8U && icvCompare_8u_C1R_p )
    {
        IPPI_CALL( icvCompareC_8u_C1R_p( src1->data.ptr, src1_step, (uchar)ival,
                                         dst->data.ptr, dst_step, size, ipp_cmp_op ));
    }
    else if( type == CV_16S && icvCompare_16s_C1R_p )
    {
        IPPI_CALL( icvCompareC_16s_C1R_p( src1->data.s, src1_step, (short)ival,
                                          dst->data.s, dst_step, size, ipp_cmp_op ));
    }
    else if( type == CV_32F && icvCompare_32f_C1R_p )
    {
        IPPI_CALL( icvCompareC_32f_C1R_p( src1->data.fl, src1_step, (float)value,
                                          dst->data.fl, dst_step, size, ipp_cmp_op ));
    }
    else
    {
        CvFunc2D_2A1P func = (CvFunc2D_2A1P)(cmps_tab[cmp_op].fn_2d[type]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( type <= CV_32S )
        {
            IPPI_CALL( func( src1->data.ptr, src1_step, dst->data.ptr,
                             dst_step, size, &ival ));
        }
        else
        {
            IPPI_CALL( func( src1->data.ptr, src1_step, dst->data.ptr,
                             dst_step, size, &value ));
        }
    }

    if( invflag )
        IPPI_CALL( icvNot_8u_C1R( dst->data.ptr, dst_step,
                           dst->data.ptr, dst_step, size ));

    __END__;
}


/****************************************************************************************\
*                                       Min/Max                                          *
\****************************************************************************************/


#define ICV_DEF_MINMAX_FUNC( __op__, name, flavor, arrtype, \
                             worktype, _toggle_macro_ )     \
static CvStatus CV_STDCALL                                  \
icv##name##_##flavor##_C1R( const arrtype* src1, int step1, \
    const arrtype* src2, int step2,                         \
    arrtype* dst, int step, CvSize size )                   \
{                                                           \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]);     \
    step /= sizeof(dst[0]);                                 \
                                                            \
    for( ; size.height--; src1 += step1,                    \
            src2 += step2, dst += step )                    \
    {                                                       \
        int x;                                              \
        for( x = 0; x <= size.width - 4; x += 4 )           \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            worktype b0 = _toggle_macro_(src2[x]);          \
            worktype a1 = _toggle_macro_(src1[x+1]);        \
            worktype b1 = _toggle_macro_(src2[x+1]);        \
            a0 = __op__( a0, b0 );                          \
            a1 = __op__( a1, b1 );                          \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
            dst[x+1] = (arrtype)_toggle_macro_(a1);         \
            a0 = _toggle_macro_(src1[x+2]);                 \
            b0 = _toggle_macro_(src2[x+2]);                 \
            a1 = _toggle_macro_(src1[x+3]);                 \
            b1 = _toggle_macro_(src2[x+3]);                 \
            a0 = __op__( a0, b0 );                          \
            a1 = __op__( a1, b1 );                          \
            dst[x+2] = (arrtype)_toggle_macro_(a0);         \
            dst[x+3] = (arrtype)_toggle_macro_(a1);         \
        }                                                   \
                                                            \
        for( ; x < size.width; x++ )                        \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            worktype b0 = _toggle_macro_(src2[x]);          \
            a0 = __op__( a0, b0 );                          \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
        }                                                   \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


#define ICV_DEF_MINMAX_CONST_FUNC( __op__, name,            \
    flavor, arrtype, worktype, _toggle_macro_)              \
static CvStatus CV_STDCALL                                  \
icv##name##C_##flavor##_C1R( const arrtype* src1, int step1,\
                             arrtype* dst, int step,        \
                             CvSize size, worktype* pScalar)\
{                                                           \
    worktype scalar = _toggle_macro_(*pScalar);             \
    step1 /= sizeof(src1[0]); step /= sizeof(dst[0]);       \
                                                            \
    for( ; size.height--; src1 += step1, dst += step )      \
    {                                                       \
        int x;                                              \
        for( x = 0; x <= size.width - 4; x += 4 )           \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            worktype a1 = _toggle_macro_(src1[x+1]);        \
            a0 = __op__( a0, scalar );                      \
            a1 = __op__( a1, scalar );                      \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
            dst[x+1] = (arrtype)_toggle_macro_(a1);         \
            a0 = _toggle_macro_(src1[x+2]);                 \
            a1 = _toggle_macro_(src1[x+3]);                 \
            a0 = __op__( a0, scalar );                      \
            a1 = __op__( a1, scalar );                      \
            dst[x+2] = (arrtype)_toggle_macro_(a0);         \
            dst[x+3] = (arrtype)_toggle_macro_(a1);         \
        }                                                   \
                                                            \
        for( ; x < size.width; x++ )                        \
        {                                                   \
            worktype a0 = _toggle_macro_(src1[x]);          \
            a0 = __op__( a0, scalar );                      \
            dst[x] = (arrtype)_toggle_macro_(a0);           \
        }                                                   \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


#define ICV_DEF_MINMAX_ALL( flavor, arrtype, worktype,                             \
                            _toggle_macro_, _min_op_, _max_op_ )                   \
ICV_DEF_MINMAX_FUNC( _min_op_, Min, flavor, arrtype, worktype, _toggle_macro_ )    \
ICV_DEF_MINMAX_FUNC( _max_op_, Max, flavor, arrtype, worktype, _toggle_macro_ )    \
ICV_DEF_MINMAX_CONST_FUNC(_min_op_, Min, flavor, arrtype, worktype, _toggle_macro_)\
ICV_DEF_MINMAX_CONST_FUNC(_max_op_, Max, flavor, arrtype, worktype, _toggle_macro_)

ICV_DEF_MINMAX_ALL( 8u, uchar, int, CV_NOP, CV_MIN_8U, CV_MAX_8U )
ICV_DEF_MINMAX_ALL( 16u, ushort, int, CV_NOP, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 16s, short, int, CV_NOP, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 32s, int, int, CV_NOP, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 32f, int, int, CV_TOGGLE_FLT, CV_IMIN, CV_IMAX )
ICV_DEF_MINMAX_ALL( 64f, double, double, CV_NOP, MIN, MAX )

#define icvMin_8s_C1R     0
#define icvMax_8s_C1R     0
#define icvMinC_8s_C1R    0
#define icvMaxC_8s_C1R    0

CV_DEF_INIT_FUNC_TAB_2D( Min, C1R )
CV_DEF_INIT_FUNC_TAB_2D( Max, C1R )
CV_DEF_INIT_FUNC_TAB_2D( MinC, C1R )
CV_DEF_INIT_FUNC_TAB_2D( MaxC, C1R )

/*********************************** cvMin & cvMax **************************************/

static void
icvMinMax( const void* srcarr1, const void* srcarr2,
           void* dstarr, int is_max )
{
    static CvFuncTable minmax_tab[2];
    static int inittab = 0;

    CV_FUNCNAME( "icvMinMax" );

    __BEGIN__;

    int type, coi = 0;
    int src1_step, src2_step, dst_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    CvFunc2D_3A func;

    if( !inittab )
    {
        icvInitMinC1RTable( &minmax_tab[0] );
        icvInitMaxC1RTable( &minmax_tab[1] );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(src2) )
    {
        CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(dst) )
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) ||
        !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( CV_MAT_CN( src1->type ) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input arrays must be single-channel");

    if( !CV_ARE_SIZES_EQ( src1, src2 ) ||
        !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = src2_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        src2_step = src2->step;
        dst_step = dst->step;
    }

    func = (CvFunc2D_3A)(minmax_tab[is_max != 0].fn_2d[type]);

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                     dst->data.ptr, dst_step, size ));

    __END__;
}


CV_IMPL void
cvMin( const void* srcarr1, const void* srcarr2, void* dstarr )
{
    icvMinMax( srcarr1, srcarr2, dstarr, 0 );
}


CV_IMPL void
cvMax( const void* srcarr1, const void* srcarr2, void* dstarr )
{
    icvMinMax( srcarr1, srcarr2, dstarr, 1 );
}


/********************************* cvMinS / cvMaxS **************************************/

static void
icvMinMaxS( const void* srcarr, double value, void* dstarr, int is_max )
{
    static CvFuncTable minmaxs_tab[2];
    static int inittab = 0;

    CV_FUNCNAME( "icvMinMaxS" );

    __BEGIN__;

    int type, coi = 0;
    int src1_step, dst_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    CvFunc2D_2A1P func;
    union
    {
        int i;
        float f;
        double d;
    }
    buf;

    if( !inittab )
    {
        icvInitMinCC1RTable( &minmaxs_tab[0] );
        icvInitMaxCC1RTable( &minmaxs_tab[1] );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT(dst) )
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( CV_MAT_CN( src1->type ) != 1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input array must be single-channel");

    if( !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);

    if( CV_MAT_DEPTH(type) <= CV_32S )
    {
        buf.i = cvRound(value);
        if( CV_MAT_DEPTH(type) == CV_8U )
            buf.i = CV_CAST_8U(buf.i);
        else if( CV_MAT_DEPTH(type) == CV_8S )
            buf.i = CV_CAST_8S(buf.i);
        else if( CV_MAT_DEPTH(type) == CV_16U )
            buf.i = CV_CAST_16U(buf.i);
        else if( CV_MAT_DEPTH(type) == CV_16S )
            buf.i = CV_CAST_16S(buf.i);
    }
    else if( CV_MAT_DEPTH(type) == CV_32F )
        buf.f = (float)value;
    else
        buf.d = value;

    size = cvGetMatSize( src1 );

    if( CV_IS_MAT_CONT( src1->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        dst_step = dst->step;
    }

    func = (CvFunc2D_2A1P)(minmaxs_tab[is_max].fn_2d[type]);

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( is_max )
    {
        if( type == CV_8U && icvThreshold_LT_8u_C1R_p )
        {
            IPPI_CALL( icvThreshold_LT_8u_C1R_p( src1->data.ptr, src1_step, dst->data.ptr,
                                                 dst_step, size, (uchar)buf.i ));
            EXIT;
        }
        else if( type == CV_16S && icvThreshold_LT_16s_C1R_p )
        {
            IPPI_CALL( icvThreshold_LT_16s_C1R_p( src1->data.s, src1_step, dst->data.s,
                                                 dst_step, size, (short)buf.i ));
            EXIT;
        }
        else if( type == CV_32F && icvThreshold_LT_32f_C1R_p )
        {
            IPPI_CALL( icvThreshold_LT_32f_C1R_p( src1->data.fl, src1_step, dst->data.fl,
                                                 dst_step, size, buf.f ));
            EXIT;
        }
    }
    else
    {
        if( type == CV_8U && icvThreshold_GT_8u_C1R_p )
        {
            IPPI_CALL( icvThreshold_GT_8u_C1R_p( src1->data.ptr, src1_step, dst->data.ptr,
                                                 dst_step, size, (uchar)buf.i ));
            EXIT;
        }
        else if( type == CV_16S && icvThreshold_GT_16s_C1R_p )
        {
            IPPI_CALL( icvThreshold_GT_16s_C1R_p( src1->data.s, src1_step, dst->data.s,
                                                 dst_step, size, (short)buf.i ));
            EXIT;
        }
        else if( type == CV_32F && icvThreshold_GT_32f_C1R_p )
        {
            IPPI_CALL( icvThreshold_GT_32f_C1R_p( src1->data.fl, src1_step, dst->data.fl,
                                                 dst_step, size, buf.f ));
            EXIT;
        }
    }

    if( type == CV_8U && size.width*size.height >= 1024 )
    {
        int i;
        uchar tab[256];
        CvMat _tab = cvMat( 1, 256, CV_8U, tab );

        if( is_max )
        {
            for( i = 0; i < buf.i; i++ )
                tab[i] = (uchar)buf.i;
            for( ; i < 256; i++ )
                tab[i] = (uchar)i;
        }
        else
        {
            for( i = 0; i < buf.i; i++ )
                tab[i] = (uchar)i;
            for( ; i < 256; i++ )
                tab[i] = (uchar)buf.i;
        }

        cvLUT( src1, dst, &_tab );
        EXIT;
    }

    IPPI_CALL( func( src1->data.ptr, src1_step, dst->data.ptr,
                     dst_step, size, &buf ));

    __END__;
}


CV_IMPL void
cvMinS( const void* srcarr, double value, void* dstarr )
{
    icvMinMaxS( srcarr, value, dstarr, 0 );
}


CV_IMPL void
cvMaxS( const void* srcarr, double value, void* dstarr )
{
    icvMinMaxS( srcarr, value, dstarr, 1 );
}


/****************************************************************************************\
*                                  Absolute Difference                                   *
\****************************************************************************************/

#define  ICV_DEF_BIN_ABS_DIFF_2D(name, arrtype, temptype, abs_macro, cast_macro)\
IPCVAPI_IMPL( CvStatus,                                 \
name,( const arrtype* src1, int step1,                  \
       const arrtype* src2, int step2,                  \
       arrtype* dst, int step, CvSize size ),           \
       (src1, step1, src2, step2, dst, step, size))     \
{                                                       \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]); \
    step /= sizeof(dst[0]);                             \
                                                        \
    for( ; size.height--; src1 += step1, src2 += step2, \
                          dst += step )                 \
    {                                                   \
        int i;                                          \
                                                        \
        for( i = 0; i <= size.width - 4; i += 4 )       \
        {                                               \
            temptype t0 = src1[i] - src2[i];            \
            temptype t1 = src1[i+1] - src2[i+1];        \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[i] = cast_macro(t0);                    \
            dst[i+1] = cast_macro(t1);                  \
                                                        \
            t0 = src1[i+2] - src2[i+2];                 \
            t1 = src1[i+3] - src2[i+3];                 \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[i+2] = cast_macro(t0);                  \
            dst[i+3] = cast_macro(t1);                  \
        }                                               \
                                                        \
        for( ; i < size.width; i++ )                    \
        {                                               \
            temptype t0 = src1[i] - src2[i];            \
            t0 = (temptype)abs_macro(t0);               \
            dst[i] = cast_macro(t0);                    \
        }                                               \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


#define  ICV_DEF_UN_ABS_DIFF_2D( name, arrtype, temptype, abs_macro, cast_macro)\
static CvStatus CV_STDCALL                              \
name( const arrtype* src0, int step1,                   \
      arrtype* dst0, int step,                          \
      CvSize size, const temptype* scalar )             \
{                                                       \
    step1 /= sizeof(src0[0]); step /= sizeof(dst0[0]);  \
                                                        \
    for( ; size.height--; src0 += step1, dst0 += step ) \
    {                                                   \
        int i, len = size.width;                        \
        const arrtype* src = src0;                      \
        arrtype* dst = dst0;                            \
                                                        \
        for( ; (len -= 12) >= 0; dst += 12, src += 12 ) \
        {                                               \
            temptype t0 = src[0] - scalar[0];           \
            temptype t1 = src[1] - scalar[1];           \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[0] = cast_macro( t0 );                  \
            dst[1] = cast_macro( t1 );                  \
                                                        \
            t0 = src[2] - scalar[2];                    \
            t1 = src[3] - scalar[3];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[2] = cast_macro( t0 );                  \
            dst[3] = cast_macro( t1 );                  \
                                                        \
            t0 = src[4] - scalar[4];                    \
            t1 = src[5] - scalar[5];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[4] = cast_macro( t0 );                  \
            dst[5] = cast_macro( t1 );                  \
                                                        \
            t0 = src[6] - scalar[6];                    \
            t1 = src[7] - scalar[7];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[6] = cast_macro( t0 );                  \
            dst[7] = cast_macro( t1 );                  \
                                                        \
            t0 = src[8] - scalar[8];                    \
            t1 = src[9] - scalar[9];                    \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[8] = cast_macro( t0 );                  \
            dst[9] = cast_macro( t1 );                  \
                                                        \
            t0 = src[10] - scalar[10];                  \
            t1 = src[11] - scalar[11];                  \
                                                        \
            t0 = (temptype)abs_macro(t0);               \
            t1 = (temptype)abs_macro(t1);               \
                                                        \
            dst[10] = cast_macro( t0 );                 \
            dst[11] = cast_macro( t1 );                 \
        }                                               \
                                                        \
        for( (len) += 12, i = 0; i < (len); i++ )       \
        {                                               \
            temptype t0 = src[i] - scalar[i];           \
            t0 = (temptype)abs_macro(t0);               \
            dst[i] = cast_macro( t0 );                  \
        }                                               \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


#define  ICV_TO_8U(x)     ((uchar)(x))
#define  ICV_TO_16U(x)    ((ushort)(x))

ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_8u_C1R, uchar, int, CV_IABS, ICV_TO_8U )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_16u_C1R, ushort, int, CV_IABS, ICV_TO_16U )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_16s_C1R, short, int, CV_IABS, CV_CAST_16S )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_32s_C1R, int, int, CV_IABS, CV_CAST_32S )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_32f_C1R, float, float, fabs, CV_CAST_32F )
ICV_DEF_BIN_ABS_DIFF_2D( icvAbsDiff_64f_C1R, double, double, fabs, CV_CAST_64F )

ICV_DEF_UN_ABS_DIFF_2D( icvAbsDiffC_8u_CnR, uchar, int, CV_IABS, CV_CAST_8U )
ICV_DEF_UN_ABS_DIFF_2D( icvAbsDiffC_16u_CnR, ushort, int, CV_IABS, CV_CAST_16U )
ICV_DEF_UN_ABS_DIFF_2D( icvAbsDiffC_16s_CnR, short, int, CV_IABS, CV_CAST_16S )
ICV_DEF_UN_ABS_DIFF_2D( icvAbsDiffC_32s_CnR, int, int, CV_IABS, CV_CAST_32S )
ICV_DEF_UN_ABS_DIFF_2D( icvAbsDiffC_32f_CnR, float, float, fabs, CV_CAST_32F )
ICV_DEF_UN_ABS_DIFF_2D( icvAbsDiffC_64f_CnR, double, double, fabs, CV_CAST_64F )


#define  ICV_INIT_MINI_FUNC_TAB_2D( FUNCNAME, suffix )          \
static void icvInit##FUNCNAME##Table( CvFuncTable* tab )        \
{                                                               \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u_##suffix;     \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##suffix;   \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##suffix;   \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##suffix;   \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##suffix;   \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##suffix;   \
}


ICV_INIT_MINI_FUNC_TAB_2D( AbsDiff, C1R )
ICV_INIT_MINI_FUNC_TAB_2D( AbsDiffC, CnR )


CV_IMPL  void
cvAbsDiff( const void* srcarr1, const void* srcarr2, void* dstarr )
{
    static CvFuncTable adiff_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvAbsDiff" );

    __BEGIN__;

    int coi1 = 0, coi2 = 0, coi3 = 0;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    int src1_step, src2_step, dst_step;
    CvSize size;
    int type;

    if( !inittab )
    {
        icvInitAbsDiffTable( &adiff_tab );
        inittab = 1;
    }

    CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1 ));
    CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2 ));
    CV_CALL( dst = cvGetMat( dst, &dststub, &coi3 ));

    if( coi1 != 0 || coi2 != 0 || coi3 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src1 );
    type = CV_MAT_TYPE(src1->type);

    if( !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    if( !CV_ARE_TYPES_EQ( src1, src2 ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    size.width *= CV_MAT_CN( type );

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
    {
        size.width *= size.height;
        size.height = 1;
        src1_step = src2_step = dst_step = CV_STUB_STEP;
    }

    {
        CvFunc2D_3A func = (CvFunc2D_3A)
            (adiff_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                         dst->data.ptr, dst_step, size ));
    }

    __END__;
}


CV_IMPL void
cvAbsDiffS( const void* srcarr, void* dstarr, CvScalar scalar )
{
    static CvFuncTable adiffs_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvAbsDiffS" );

    __BEGIN__;

    int coi1 = 0, coi2 = 0;
    int type, sctype;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    int src_step, dst_step;
    double buf[12];
    CvSize size;

    if( !inittab )
    {
        icvInitAbsDiffCTable( &adiffs_tab );
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_TYPES_EQ(src, dst) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ(src, dst) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    sctype = type = CV_MAT_TYPE( src->type );
    if( CV_MAT_DEPTH(type) < CV_32S )
        sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;

    size = cvGetMatSize( src );
    size.width *= CV_MAT_CN( type );

    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        size.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }

    CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

    {
        CvFunc2D_2A1P func = (CvFunc2D_2A1P)
            (adiffs_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step, dst->data.ptr,
                         dst_step, size, buf ));
    }

    __END__;
}

/* End of file. */

// /****************************************************************************************\
// *                      Arithmetic operations (+, -) without mask                         *
// \****************************************************************************************/
// 
#define ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype, cast_macro, len )\
{                                                                   \
    int i;                                                          \
                                                                    \
    for( i = 0; i <= (len) - 4; i += 4 )                            \
    {                                                               \
        worktype t0 = __op__((src1)[i], (src2)[i]);                 \
        worktype t1 = __op__((src1)[i+1], (src2)[i+1]);             \
                                                                    \
        (dst)[i] = cast_macro( t0 );                                \
        (dst)[i+1] = cast_macro( t1 );                              \
                                                                    \
        t0 = __op__((src1)[i+2],(src2)[i+2]);                       \
        t1 = __op__((src1)[i+3],(src2)[i+3]);                       \
                                                                    \
        (dst)[i+2] = cast_macro( t0 );                              \
        (dst)[i+3] = cast_macro( t1 );                              \
    }                                                               \
                                                                    \
    for( ; i < (len); i++ )                                         \
    {                                                               \
        worktype t0 = __op__((src1)[i],(src2)[i]);                  \
        (dst)[i] = cast_macro( t0 );                                \
    }                                                               \
}

#define ICV_DEF_BIN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )   \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size ),                                   \
      (src1, step1, src2, step2, dst, step, size) )                         \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_BIN_ARI_OP_2D_SFS(__op__, name, type, worktype, cast_macro) \
IPCVAPI_IMPL( CvStatus, name,                                               \
    ( const type* src1, int step1, const type* src2, int step2,             \
      type* dst, int step, CvSize size, int /*scalefactor*/ ),              \
      (src1, step1, src2, step2, dst, step, size, 0) )                      \
{                                                                           \
    step1/=sizeof(src1[0]); step2/=sizeof(src2[0]); step/=sizeof(dst[0]);   \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            worktype t0 = __op__((src1)[0],(src2)[0]);                      \
            (dst)[0] = cast_macro( t0 );                                    \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src1 += step1, src2 += step2, dst += step )   \
        {                                                                   \
            ICV_DEF_BIN_ARI_OP_CASE( __op__, worktype,                      \
                                     cast_macro, size.width );              \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


#define ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,               \
                                src, scalar, dst, len )                     \
{                                                                           \
    int i;                                                                  \
                                                                            \
    for( ; ((len) -= 12) >= 0; (dst) += 12, (src) += 12 )                   \
    {                                                                       \
        worktype t0 = __op__((scalar)[0], (src)[0]);                        \
        worktype t1 = __op__((scalar)[1], (src)[1]);                        \
                                                                            \
        (dst)[0] = cast_macro( t0 );                                        \
        (dst)[1] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[2], (src)[2]);                                 \
        t1 = __op__((scalar)[3], (src)[3]);                                 \
                                                                            \
        (dst)[2] = cast_macro( t0 );                                        \
        (dst)[3] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[4], (src)[4]);                                 \
        t1 = __op__((scalar)[5], (src)[5]);                                 \
                                                                            \
        (dst)[4] = cast_macro( t0 );                                        \
        (dst)[5] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[6], (src)[6]);                                 \
        t1 = __op__((scalar)[7], (src)[7]);                                 \
                                                                            \
        (dst)[6] = cast_macro( t0 );                                        \
        (dst)[7] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[8], (src)[8]);                                 \
        t1 = __op__((scalar)[9], (src)[9]);                                 \
                                                                            \
        (dst)[8] = cast_macro( t0 );                                        \
        (dst)[9] = cast_macro( t1 );                                        \
                                                                            \
        t0 = __op__((scalar)[10], (src)[10]);                               \
        t1 = __op__((scalar)[11], (src)[11]);                               \
                                                                            \
        (dst)[10] = cast_macro( t0 );                                       \
        (dst)[11] = cast_macro( t1 );                                       \
    }                                                                       \
                                                                            \
    for( (len) += 12, i = 0; i < (len); i++ )                               \
    {                                                                       \
        worktype t0 = __op__((scalar)[i],(src)[i]);                         \
        (dst)[i] = cast_macro( t0 );                                        \
    }                                                                       \
}


#define ICV_DEF_UN_ARI_OP_2D( __op__, name, type, worktype, cast_macro )    \
static CvStatus CV_STDCALL name                                             \
    ( const type* src, int step1, type* dst, int step,                      \
      CvSize size, const worktype* scalar )                                 \
{                                                                           \
    step1 /= sizeof(src[0]); step /= sizeof(dst[0]);                        \
                                                                            \
    if( size.width == 1 )                                                   \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            worktype t0 = __op__(*(scalar),*(src));                         \
            *(dst) = cast_macro( t0 );                                      \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        for( ; size.height--; src += step1, dst += step )                   \
        {                                                                   \
            const type *tsrc = src;                                         \
            type *tdst = dst;                                               \
            int width = size.width;                                         \
                                                                            \
            ICV_DEF_UN_ARI_OP_CASE( __op__, worktype, cast_macro,           \
                                    tsrc, scalar, tdst, width );            \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}

// 
#define ICV_DEF_BIN_ARI_ALL( __op__, name, cast_8u )                                \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_8u_C1R, uchar, int, cast_8u )        \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )  \
ICV_DEF_BIN_ARI_OP_2D_SFS( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )   \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )         \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )     \
ICV_DEF_BIN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )

#define ICV_DEF_UN_ARI_ALL( __op__, name )                                          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_8u_C1R, uchar, int, CV_CAST_8U )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16u_C1R, ushort, int, CV_CAST_16U )       \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_16s_C1R, short, int, CV_CAST_16S )        \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32s_C1R, int, int, CV_CAST_32S )          \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_32f_C1R, float, float, CV_CAST_32F )      \
ICV_DEF_UN_ARI_OP_2D( __op__, icv##name##_64f_C1R, double, double, CV_CAST_64F )

#undef CV_SUB_R
#define CV_SUB_R(a,b) ((b) - (a))

ICV_DEF_BIN_ARI_ALL( CV_ADD, Add, CV_FAST_CAST_8U )
ICV_DEF_BIN_ARI_ALL( CV_SUB_R, Sub, CV_FAST_CAST_8U )

ICV_DEF_UN_ARI_ALL( CV_ADD, AddC )
ICV_DEF_UN_ARI_ALL( CV_SUB, SubRC )

#define ICV_DEF_INIT_ARITHM_FUNC_TAB( FUNCNAME, FLAG )          \
static  void  icvInit##FUNCNAME##FLAG##Table( CvFuncTable* tab )\
{                                                               \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u_##FLAG;       \
    tab->fn_2d[CV_8S] = 0;                                      \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##FLAG;     \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##FLAG;     \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##FLAG;     \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##FLAG;     \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##FLAG;     \
}

ICV_DEF_INIT_ARITHM_FUNC_TAB( Sub, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( SubRC, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( Add, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( AddC, C1R )
// 
// /****************************************************************************************\
// *                       External Functions for Arithmetic Operations                     *
// \****************************************************************************************/
// 
// /*************************************** S U B ******************************************/
// 
CV_IMPL void
cvSub( const void* srcarr1, const void* srcarr2,
       void* dstarr, const void* maskarr )
{
    static CvFuncTable sub_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvSub" );

    __BEGIN__;

    const CvArr* tmp;
    int y, dy, type, depth, cn, cont_flag = 0;
    int src1_step, src2_step, dst_step, tdst_step, mask_step;
    CvMat srcstub1, srcstub2, *src1, *src2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_3A func;
    CvFunc2D_3A1I func_sfs;
    CvCopyMaskFunc copym_func;
    CvSize size, tsize;

    CV_SWAP( srcarr1, srcarr2, tmp ); // to comply with IPP
    src1 = (CvMat*)srcarr1;
    src2 = (CvMat*)srcarr2;

    if( !CV_IS_MAT(src1) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src1) || CV_IS_MATND(src2) || CV_IS_MATND(dst))
        {
            CvArr* arrs[] = { src1, src2, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            if( maskarr )
                CV_ERROR( CV_StsBadMask,
                "This operation on multi-dimensional arrays does not support mask" );

            CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

            type = iterator.hdr[0]->type;
            iterator.size.width *= CV_MAT_CN(type);

            if( !inittab )
            {
                icvInitSubC1RTable( &sub_tab );
                inittab = 1;
            }

            depth = CV_MAT_DEPTH(type);
            if( depth <= CV_16S )
            {
                func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
                if( !func_sfs )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func_sfs( iterator.ptr[0], CV_STUB_STEP,
                                         iterator.ptr[1], CV_STUB_STEP,
                                         iterator.ptr[2], CV_STUB_STEP,
                                         iterator.size, 0 ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.ptr[2], CV_STUB_STEP,
                                     iterator.size ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0, coi3 = 0;
            
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1 ));
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3 ));
            if( coi1 + coi2 + coi3 != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    if( !mask )
    {
        if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
        {
            int len = size.width*size.height*cn;

            if( len <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = (const float*)(src1->data.ptr);
                    const float* src2data = (const float*)(src2->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = (float)(src2data[len-1] - src1data[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = (const double*)(src1->data.ptr);
                    const double* src2data = (const double*)(src2->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = src2data[len-1] - src1data[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }

        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    if( !inittab )
    {
        icvInitSubC1RTable( &sub_tab );
        inittab = 1;
    }

    if( depth <= CV_16S )
    {
        func = 0;
        func_sfs = (CvFunc2D_3A1I)(sub_tab.fn_2d[depth]);
        if( !func_sfs )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }
    else
    {
        func_sfs = 0;
        func = (CvFunc2D_3A)(sub_tab.fn_2d[depth]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src1_step = src2_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( depth <= CV_16S ?
            func_sfs( src1->data.ptr + y*src1->step, src1_step,
                      src2->data.ptr + y*src2->step, src2_step,
                      tdst->data.ptr, tdst_step,
                      cvSize( tsize.width*cn, tsize.height ), 0 ) :
            func( src1->data.ptr + y*src1->step, src1_step,
                  src2->data.ptr + y*src2->step, src2_step,
                  tdst->data.ptr, tdst_step,
                  cvSize( tsize.width*cn, tsize.height )));

        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}


CV_IMPL void
cvSubRS( const void* srcarr, CvScalar scalar, void* dstarr, const void* maskarr )
{
    static CvFuncTable subr_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvSubRS" );

    __BEGIN__;

    int sctype, y, dy, type, depth, cn, coi = 0, cont_flag = 0;
    int src_step, dst_step, tdst_step, mask_step;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_2A1P func;
    CvCopyMaskFunc copym_func;
    double buf[12];
    int is_nd = 0;
    CvSize size, tsize; 

    if( !inittab )
    {
        icvInitSubRCC1RTable( &subr_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src) )
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
        {
            CV_CALL( src = cvGetMat( src, &srcstub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
        {
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src, dst };
        CvMatND stubs[2];
        CvNArrayIterator iterator;

        if( maskarr )
            CV_ERROR( CV_StsBadMask,
            "This operation on multi-dimensional arrays does not support mask" );

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator ));

        sctype = type = CV_MAT_TYPE(iterator.hdr[0]->type);
        if( CV_MAT_DEPTH(sctype) < CV_32S )
            sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;
        iterator.size.width *= CV_MAT_CN(type);

        func = (CvFunc2D_2A1P)(subr_tab.fn_2d[CV_MAT_DEPTH(type)]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
       
        CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

        do
        {
            IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                             iterator.ptr[1], CV_STUB_STEP,
                             iterator.size, buf ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    sctype = type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    if( depth < CV_32S )
        sctype = (type & CV_MAT_CN_MASK) | CV_32SC1;

    size = cvGetMatSize( src );

    if( !maskarr )
    {
        if( CV_IS_MAT_CONT( src->type & dst->type ))
        {
            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
            {
                int len = size.width * size.height;

                if( type == CV_32FC1 )
                {
                    const float* srcdata = (const float*)(src->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = (float)(scalar.val[0] - srcdata[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( type == CV_64FC1 )
                {
                    const double* srcdata = (const double*)(src->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);
                
                    do
                    {
                        dstdata[len-1] = scalar.val[0] - srcdata[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }
        
        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    func = (CvFunc2D_2A1P)(subr_tab.fn_2d[depth]);
    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    src_step = src->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    CV_CALL( cvScalarToRawData( &scalar, buf, sctype, 1 ));

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( func( src->data.ptr + y*src->step, src_step,
                         tdst->data.ptr, tdst_step,
                         cvSize( tsize.width*cn, tsize.height ), buf ));
        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}
// 
// 
/******************************* A D D ********************************/

CV_IMPL void
cvAdd( const void* srcarr1, const void* srcarr2,
       void* dstarr, const void* maskarr )
{
    static CvFuncTable add_tab;
    static int inittab = 0;
    int local_alloc = 1;
    uchar* buffer = 0;

    CV_FUNCNAME( "cvAdd" );

    __BEGIN__;

    int y, dy, type, depth, cn, cont_flag = 0;
    int src1_step, src2_step, dst_step, tdst_step, mask_step;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvFunc2D_3A func;
    CvFunc2D_3A1I func_sfs;
    CvCopyMaskFunc copym_func;
    CvSize size, tsize;

    if( !CV_IS_MAT(src1) || !CV_IS_MAT(src2) || !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(src1) || CV_IS_MATND(src2) || CV_IS_MATND(dst))
        {
            CvArr* arrs[] = { src1, src2, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            if( maskarr )
                CV_ERROR( CV_StsBadMask,
                "This operation on multi-dimensional arrays does not support mask" );

            CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

            type = iterator.hdr[0]->type;
            iterator.size.width *= CV_MAT_CN(type);

            if( !inittab )
            {
                icvInitAddC1RTable( &add_tab );
                inittab = 1;
            }

            depth = CV_MAT_DEPTH(type);
            if( depth <= CV_16S )
            {
                func_sfs = (CvFunc2D_3A1I)(add_tab.fn_2d[depth]);
                if( !func_sfs )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func_sfs( iterator.ptr[0], CV_STUB_STEP,
                                         iterator.ptr[1], CV_STUB_STEP,
                                         iterator.ptr[2], CV_STUB_STEP,
                                         iterator.size, 0 ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                func = (CvFunc2D_3A)(add_tab.fn_2d[depth]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.ptr[2], CV_STUB_STEP,
                                     iterator.size ));
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0, coi3 = 0;
            
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi1 ));
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi2 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi3 ));
            if( coi1 + coi2 + coi3 != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    if( !mask )
    {
        if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
        {
            int len = size.width*size.height*cn;

            if( len <= CV_MAX_INLINE_MAT_OP_SIZE*CV_MAX_INLINE_MAT_OP_SIZE )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = (const float*)(src1->data.ptr);
                    const float* src2data = (const float*)(src2->data.ptr);
                    float* dstdata = (float*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = (float)(src1data[len-1] + src2data[len-1]);
                    }
                    while( --len );

                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = (const double*)(src1->data.ptr);
                    const double* src2data = (const double*)(src2->data.ptr);
                    double* dstdata = (double*)(dst->data.ptr);

                    do
                    {
                        dstdata[len-1] = src1data[len-1] + src2data[len-1];
                    }
                    while( --len );

                    EXIT;
                }
            }
            cont_flag = 1;
        }

        dy = size.height;
        copym_func = 0;
        tdst = dst;
    }
    else
    {
        int buf_size, elem_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src1->type & src2->type & dst->type & mask->type );
        elem_size = CV_ELEM_SIZE(type);

        dy = CV_MAX_LOCAL_SIZE/(elem_size*size.height);
        dy = MAX(dy,1);
        dy = MIN(dy,size.height);
        dstbuf = cvMat( dy, size.width, type );
        if( !cont_flag )
            dstbuf.step = cvAlign( dstbuf.step, 8 );
        buf_size = dstbuf.step ? dstbuf.step*dy : size.width*elem_size;
        if( buf_size > CV_MAX_LOCAL_SIZE )
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
            local_alloc = 0;
        }
        else
            buffer = (uchar*)cvStackAlloc( buf_size );
        dstbuf.data.ptr = buffer;
        tdst = &dstbuf;
        
        copym_func = icvGetCopyMaskFunc( elem_size );
    }

    if( !inittab )
    {
        icvInitAddC1RTable( &add_tab );
        inittab = 1;
    }

    if( depth <= CV_16S )
    {
        func = 0;
        func_sfs = (CvFunc2D_3A1I)(add_tab.fn_2d[depth]);
        if( !func_sfs )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }
    else
    {
        func_sfs = 0;
        func = (CvFunc2D_3A)(add_tab.fn_2d[depth]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    src1_step = src1->step;
    src2_step = src2->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;

    for( y = 0; y < size.height; y += dy )
    {
        tsize.width = size.width;
        tsize.height = dy;
        if( y + dy > size.height )
            tsize.height = size.height - y;
        if( cont_flag || tsize.height == 1 )
        {
            tsize.width *= tsize.height;
            tsize.height = 1;
            src1_step = src2_step = tdst_step = dst_step = mask_step = CV_STUB_STEP;
        }

        IPPI_CALL( depth <= CV_16S ?
            func_sfs( src1->data.ptr + y*src1->step, src1_step,
                      src2->data.ptr + y*src2->step, src2_step,
                      tdst->data.ptr, tdst_step,
                      cvSize( tsize.width*cn, tsize.height ), 0 ) :
            func( src1->data.ptr + y*src1->step, src1_step,
                  src2->data.ptr + y*src2->step, src2_step,
                  tdst->data.ptr, tdst_step,
                  cvSize( tsize.width*cn, tsize.height )));

        if( mask )
        {
            IPPI_CALL( copym_func( tdst->data.ptr, tdst_step, dst->data.ptr + y*dst->step,
                                   dst_step, tsize, mask->data.ptr + y*mask->step, mask_step ));
        }
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
}

/***************************************** M U L ****************************************/

#define ICV_DEF_MUL_OP_CASE( flavor, arrtype, worktype, _cast_macro1_,                  \
                             _cast_macro2_, _cvt_macro_ )                               \
static CvStatus CV_STDCALL                                                              \
    icvMul_##flavor##_C1R( const arrtype* src1, int step1,                              \
                           const arrtype* src2, int step2,                              \
                           arrtype* dst, int step,                                      \
                           CvSize size, double scale )                                  \
{                                                                                       \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]); step /= sizeof(dst[0]);         \
                                                                                        \
    if( fabs(scale - 1.) < DBL_EPSILON )                                                \
    {                                                                                   \
        for( ; size.height--; src1+=step1, src2+=step2, dst+=step )                     \
        {                                                                               \
            int i;                                                                      \
            for( i = 0; i <= size.width - 4; i += 4 )                                   \
            {                                                                           \
                worktype t0 = src1[i] * src2[i];                                        \
                worktype t1 = src1[i+1] * src2[i+1];                                    \
                                                                                        \
                dst[i] = _cast_macro2_(t0);                                             \
                dst[i+1] = _cast_macro2_(t1);                                           \
                                                                                        \
                t0 = src1[i+2] * src2[i+2];                                             \
                t1 = src1[i+3] * src2[i+3];                                             \
                                                                                        \
                dst[i+2] = _cast_macro2_(t0);                                           \
                dst[i+3] = _cast_macro2_(t1);                                           \
            }                                                                           \
                                                                                        \
            for( ; i < size.width; i++ )                                                \
            {                                                                           \
                worktype t0 = src1[i] * src2[i];                                        \
                dst[i] = _cast_macro2_(t0);                                             \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        for( ; size.height--; src1+=step1, src2+=step2, dst+=step )                     \
        {                                                                               \
            int i;                                                                      \
            for( i = 0; i <= size.width - 4; i += 4 )                                   \
            {                                                                           \
                double ft0 = scale*_cvt_macro_(src1[i])*_cvt_macro_(src2[i]);           \
                double ft1 = scale*_cvt_macro_(src1[i+1])*_cvt_macro_(src2[i+1]);       \
                worktype t0 = _cast_macro1_(ft0);                                       \
                worktype t1 = _cast_macro1_(ft1);                                       \
                                                                                        \
                dst[i] = _cast_macro2_(t0);                                             \
                dst[i+1] = _cast_macro2_(t1);                                           \
                                                                                        \
                ft0 = scale*_cvt_macro_(src1[i+2])*_cvt_macro_(src2[i+2]);              \
                ft1 = scale*_cvt_macro_(src1[i+3])*_cvt_macro_(src2[i+3]);              \
                t0 = _cast_macro1_(ft0);                                                \
                t1 = _cast_macro1_(ft1);                                                \
                                                                                        \
                dst[i+2] = _cast_macro2_(t0);                                           \
                dst[i+3] = _cast_macro2_(t1);                                           \
            }                                                                           \
                                                                                        \
            for( ; i < size.width; i++ )                                                \
            {                                                                           \
                worktype t0;                                                            \
                t0 = _cast_macro1_(scale*_cvt_macro_(src1[i])*_cvt_macro_(src2[i]));    \
                dst[i] = _cast_macro2_(t0);                                             \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}


ICV_DEF_MUL_OP_CASE( 8u, uchar, int, cvRound, CV_CAST_8U, CV_8TO32F )
ICV_DEF_MUL_OP_CASE( 16u, ushort, int, cvRound, CV_CAST_16U, CV_NOP )
ICV_DEF_MUL_OP_CASE( 16s, short, int, cvRound, CV_CAST_16S, CV_NOP )
ICV_DEF_MUL_OP_CASE( 32s, int, int, cvRound, CV_CAST_32S, CV_NOP )
ICV_DEF_MUL_OP_CASE( 32f, float, double, CV_NOP, CV_CAST_32F, CV_NOP )
ICV_DEF_MUL_OP_CASE( 64f, double, double, CV_NOP, CV_CAST_64F, CV_NOP )


ICV_DEF_INIT_ARITHM_FUNC_TAB( Mul, C1R )


typedef CvStatus (CV_STDCALL * CvScaledElWiseFunc)( const void* src1, int step1,
                                                    const void* src2, int step2,
                                                    void* dst, int step,
                                                    CvSize size, double scale );
// 
CV_IMPL void
cvMul( const void* srcarr1, const void* srcarr2, void* dstarr, double scale )
{
    static CvFuncTable mul_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvMul" );

    __BEGIN__;

    int type, depth, coi = 0;
    int src1_step, src2_step, dst_step;
    int is_nd = 0;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;
    CvScaledElWiseFunc func;

    if( !inittab )
    {
        icvInitMulC1RTable( &mul_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src1) )
    {
        if( CV_IS_MATND(src1) )
            is_nd = 1;
        else
        {
            CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(src2) )
    {
        if( CV_IS_MATND(src2) )
            is_nd = 1;
        else
        {
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
        {
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { src1, src2, dst };
        CvMatND stubs[3];
        CvNArrayIterator iterator;

        CV_CALL( cvInitNArrayIterator( 3, arrs, 0, stubs, &iterator ));

        type = iterator.hdr[0]->type;
        iterator.size.width *= CV_MAT_CN(type);

        func = (CvScaledElWiseFunc)(mul_tab.fn_2d[CV_MAT_DEPTH(type)]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        do
        {
            IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                             iterator.ptr[1], CV_STUB_STEP,
                             iterator.ptr[2], CV_STUB_STEP,
                             iterator.size, scale ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src1, src2 ) || !CV_ARE_TYPES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src1, src2 ) || !CV_ARE_SIZES_EQ( src1, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src1->type);
    size = cvGetMatSize( src1 );

    depth = CV_MAT_DEPTH(type);
    size.width *= CV_MAT_CN( type );

    if( CV_IS_MAT_CONT( src1->type & src2->type & dst->type ))
    {
        size.width *= size.height;

        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE && scale == 1 )
        {
            if( depth == CV_32F )
            {
                const float* src1data = (const float*)(src1->data.ptr);
                const float* src2data = (const float*)(src2->data.ptr);
                float* dstdata = (float*)(dst->data.ptr);
            
                do
                {
                    dstdata[size.width-1] = (float)
                        (src1data[size.width-1] * src2data[size.width-1]);
                }
                while( --size.width );

                EXIT;
            }

            if( depth == CV_64F )
            {
                const double* src1data = (const double*)(src1->data.ptr);
                const double* src2data = (const double*)(src2->data.ptr);
                double* dstdata = (double*)(dst->data.ptr);
            
                do
                {
                    dstdata[size.width-1] =
                        src1data[size.width-1] * src2data[size.width-1];
                }
                while( --size.width );

                EXIT;
            }
        }

        src1_step = src2_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1->step;
        src2_step = src2->step;
        dst_step = dst->step;
    }

    func = (CvScaledElWiseFunc)(mul_tab.fn_2d[CV_MAT_DEPTH(type)]);

    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                     dst->data.ptr, dst_step, size, scale ));

    __END__;
}
// 
// 
// /***************************************** D I V ****************************************/
// 
#define ICV_DEF_DIV_OP_CASE( flavor, arrtype, worktype, checktype, _start_row_macro_,   \
    _cast_macro1_, _cast_macro2_, _cvt_macro_, _check_macro_, isrc )                    \
                                                                                        \
static CvStatus CV_STDCALL                                                              \
icvDiv_##flavor##_C1R( const arrtype* src1, int step1,                                  \
                       const arrtype* src2, int step2,                                  \
                       arrtype* dst, int step,                                          \
                       CvSize size, double scale )                                      \
{                                                                                       \
    step1 /= sizeof(src1[0]); step2 /= sizeof(src2[0]); step /= sizeof(dst[0]);         \
                                                                                        \
    for( ; size.height--; src1+=step1, src2+=step2, dst+=step )                         \
    {                                                                                   \
        _start_row_macro_(checktype, src2);                                             \
        for( i = 0; i <= size.width - 4; i += 4 )                                       \
        {                                                                               \
            if( _check_macro_(isrc[i]) && _check_macro_(isrc[i+1]) &&                   \
                _check_macro_(isrc[i+2]) && _check_macro_(isrc[i+3]))                   \
            {                                                                           \
                double a = (double)_cvt_macro_(src2[i]) * _cvt_macro_(src2[i+1]);       \
                double b = (double)_cvt_macro_(src2[i+2]) * _cvt_macro_(src2[i+3]);     \
                double d = scale/(a * b);                                               \
                                                                                        \
                b *= d;                                                                 \
                a *= d;                                                                 \
                                                                                        \
                worktype z0 = _cast_macro1_(src2[i+1] * _cvt_macro_(src1[i]) * b);      \
                worktype z1 = _cast_macro1_(src2[i] * _cvt_macro_(src1[i+1]) * b);      \
                worktype z2 = _cast_macro1_(src2[i+3] * _cvt_macro_(src1[i+2]) * a);    \
                worktype z3 = _cast_macro1_(src2[i+2] * _cvt_macro_(src1[i+3]) * a);    \
                                                                                        \
                dst[i] = _cast_macro2_(z0);                                             \
                dst[i+1] = _cast_macro2_(z1);                                           \
                dst[i+2] = _cast_macro2_(z2);                                           \
                dst[i+3] = _cast_macro2_(z3);                                           \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                worktype z0 = _check_macro_(isrc[i]) ?                                  \
                   _cast_macro1_(_cvt_macro_(src1[i])*scale/_cvt_macro_(src2[i])) : 0;  \
                worktype z1 = _check_macro_(isrc[i+1]) ?                                \
                   _cast_macro1_(_cvt_macro_(src1[i+1])*scale/_cvt_macro_(src2[i+1])):0;\
                worktype z2 = _check_macro_(isrc[i+2]) ?                                \
                   _cast_macro1_(_cvt_macro_(src1[i+2])*scale/_cvt_macro_(src2[i+2])):0;\
                worktype z3 = _check_macro_(isrc[i+3]) ?                                \
                   _cast_macro1_(_cvt_macro_(src1[i+3])*scale/_cvt_macro_(src2[i+3])):0;\
                                                                                        \
                dst[i] = _cast_macro2_(z0);                                             \
                dst[i+1] = _cast_macro2_(z1);                                           \
                dst[i+2] = _cast_macro2_(z2);                                           \
                dst[i+3] = _cast_macro2_(z3);                                           \
            }                                                                           \
        }                                                                               \
                                                                                        \
        for( ; i < size.width; i++ )                                                    \
        {                                                                               \
            worktype z0 = _check_macro_(isrc[i]) ?                                      \
                _cast_macro1_(_cvt_macro_(src1[i])*scale/_cvt_macro_(src2[i])) : 0;     \
            dst[i] = _cast_macro2_(z0);                                                 \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}

// 
#define ICV_DEF_RECIP_OP_CASE( flavor, arrtype, worktype, checktype,            \
    _start_row_macro_, _cast_macro1_, _cast_macro2_,                            \
    _cvt_macro_, _check_macro_, isrc )                                          \
                                                                                \
static CvStatus CV_STDCALL                                                      \
icvRecip_##flavor##_C1R( const arrtype* src, int step1,                         \
                         arrtype* dst, int step,                                \
                         CvSize size, double scale )                            \
{                                                                               \
    step1 /= sizeof(src[0]); step /= sizeof(dst[0]);                            \
                                                                                \
    for( ; size.height--; src+=step1, dst+=step )                               \
    {                                                                           \
        _start_row_macro_(checktype, src);                                      \
        for( i = 0; i <= size.width - 4; i += 4 )                               \
        {                                                                       \
            if( _check_macro_(isrc[i]) && _check_macro_(isrc[i+1]) &&           \
                _check_macro_(isrc[i+2]) && _check_macro_(isrc[i+3]))           \
            {                                                                   \
                double a = (double)_cvt_macro_(src[i]) * _cvt_macro_(src[i+1]); \
                double b = (double)_cvt_macro_(src[i+2]) * _cvt_macro_(src[i+3]);\
                double d = scale/(a * b);                                       \
                                                                                \
                b *= d;                                                         \
                a *= d;                                                         \
                                                                                \
                worktype z0 = _cast_macro1_(src[i+1] * b);                      \
                worktype z1 = _cast_macro1_(src[i] * b);                        \
                worktype z2 = _cast_macro1_(src[i+3] * a);                      \
                worktype z3 = _cast_macro1_(src[i+2] * a);                      \
                                                                                \
                dst[i] = _cast_macro2_(z0);                                     \
                dst[i+1] = _cast_macro2_(z1);                                   \
                dst[i+2] = _cast_macro2_(z2);                                   \
                dst[i+3] = _cast_macro2_(z3);                                   \
            }                                                                   \
            else                                                                \
            {                                                                   \
                worktype z0 = _check_macro_(isrc[i]) ?                          \
                   _cast_macro1_(scale/_cvt_macro_(src[i])) : 0;                \
                worktype z1 = _check_macro_(isrc[i+1]) ?                        \
                   _cast_macro1_(scale/_cvt_macro_(src[i+1])):0;                \
                worktype z2 = _check_macro_(isrc[i+2]) ?                        \
                   _cast_macro1_(scale/_cvt_macro_(src[i+2])):0;                \
                worktype z3 = _check_macro_(isrc[i+3]) ?                        \
                   _cast_macro1_(scale/_cvt_macro_(src[i+3])):0;                \
                                                                                \
                dst[i] = _cast_macro2_(z0);                                     \
                dst[i+1] = _cast_macro2_(z1);                                   \
                dst[i+2] = _cast_macro2_(z2);                                   \
                dst[i+3] = _cast_macro2_(z3);                                   \
            }                                                                   \
        }                                                                       \
                                                                                \
        for( ; i < size.width; i++ )                                            \
        {                                                                       \
            worktype z0 = _check_macro_(isrc[i]) ?                              \
                _cast_macro1_(scale/_cvt_macro_(src[i])) : 0;                   \
            dst[i] = _cast_macro2_(z0);                                         \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


#define div_start_row_int(checktype, divisor) \
    int i

#define div_start_row_flt(checktype, divisor) \
    const checktype* isrc = (const checktype*)divisor; int i

#define div_check_zero_flt(x)  (((x) & 0x7fffffff) != 0)
#define div_check_zero_dbl(x)  (((x) & CV_BIG_INT(0x7fffffffffffffff)) != 0)

#if defined WIN64 && defined EM64T && defined _MSC_VER && !defined CV_ICC
#pragma optimize("",off)
#endif

ICV_DEF_DIV_OP_CASE( 8u, uchar, int, uchar, div_start_row_int,
                     cvRound, CV_CAST_8U, CV_8TO32F, CV_NONZERO, src2 )

#if defined WIN64 && defined EM64T && defined _MSC_VER && !defined CV_ICC
#pragma optimize("",on)
#endif


ICV_DEF_DIV_OP_CASE( 16u, ushort, int, ushort, div_start_row_int,
                     cvRound, CV_CAST_16U, CV_CAST_64F, CV_NONZERO, src2 )
ICV_DEF_DIV_OP_CASE( 16s, short, int, short, div_start_row_int,
                     cvRound, CV_CAST_16S, CV_NOP, CV_NONZERO, src2 )
ICV_DEF_DIV_OP_CASE( 32s, int, int, int, div_start_row_int,
                     cvRound, CV_CAST_32S, CV_CAST_64F, CV_NONZERO, src2 )
ICV_DEF_DIV_OP_CASE( 32f, float, double, int, div_start_row_flt,
                     CV_NOP, CV_CAST_32F, CV_NOP, div_check_zero_flt, isrc )
ICV_DEF_DIV_OP_CASE( 64f, double, double, int64, div_start_row_flt,
                     CV_NOP, CV_CAST_64F, CV_NOP, div_check_zero_dbl, isrc )

ICV_DEF_RECIP_OP_CASE( 8u, uchar, int, uchar, div_start_row_int,
                       cvRound, CV_CAST_8U, CV_8TO32F, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 16u, ushort, int, ushort, div_start_row_int,
                       cvRound, CV_CAST_16U, CV_CAST_64F, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 16s, short, int, short, div_start_row_int,
                       cvRound, CV_CAST_16S, CV_NOP, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 32s, int, int, int, div_start_row_int,
                       cvRound, CV_CAST_32S, CV_CAST_64F, CV_NONZERO, src )
ICV_DEF_RECIP_OP_CASE( 32f, float, double, int, div_start_row_flt,
                       CV_NOP, CV_CAST_32F, CV_NOP, div_check_zero_flt, isrc  )
ICV_DEF_RECIP_OP_CASE( 64f, double, double, int64, div_start_row_flt,
                       CV_NOP, CV_CAST_64F, CV_NOP, div_check_zero_dbl, isrc )

ICV_DEF_INIT_ARITHM_FUNC_TAB( Div, C1R )
ICV_DEF_INIT_ARITHM_FUNC_TAB( Recip, C1R )

typedef CvStatus (CV_STDCALL * CvRecipFunc)( const void* src, int step1,
                                             void* dst, int step,
                                             CvSize size, double scale );

CV_IMPL void
cvDiv( const void* srcarr1, const void* srcarr2, void* dstarr, double scale )
{
    static CvFuncTable div_tab;
    static CvFuncTable recip_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvDiv" );

    __BEGIN__;

    int type, coi = 0;
    int is_nd = 0;
    int src1_step, src2_step, dst_step;
    int src1_cont_flag = CV_MAT_CONT_FLAG;
    CvMat srcstub1, *src1 = (CvMat*)srcarr1;
    CvMat srcstub2, *src2 = (CvMat*)srcarr2;
    CvMat dststub,  *dst = (CvMat*)dstarr;
    CvSize size;

    if( !inittab )
    {
        icvInitDivC1RTable( &div_tab );
        icvInitRecipC1RTable( &recip_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(src2) )
    {
        if( CV_IS_MATND(src2))
            is_nd = 1;
        else
        {
            CV_CALL( src2 = cvGetMat( src2, &srcstub2, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( src1 )
    {
        if( CV_IS_MATND(src1))
            is_nd = 1;
        else
        {
            if( !CV_IS_MAT(src1) )
            {
                CV_CALL( src1 = cvGetMat( src1, &srcstub1, &coi ));
                if( coi != 0 )
                    CV_ERROR( CV_BadCOI, "" );
            }

            if( !CV_ARE_TYPES_EQ( src1, src2 ))
                CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

            if( !CV_ARE_SIZES_EQ( src1, src2 ))
                CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );
            src1_cont_flag = src1->type;
        }
    }

    if( !CV_IS_MAT(dst) )
    {
        if( CV_IS_MATND(dst))
            is_nd = 1;
        else
        {
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    if( is_nd )
    {
        CvArr* arrs[] = { dst, src2, src1 };
        CvMatND stubs[3];
        CvNArrayIterator iterator;

        CV_CALL( cvInitNArrayIterator( 2 + (src1 != 0), arrs, 0, stubs, &iterator ));

        type = iterator.hdr[0]->type;
        iterator.size.width *= CV_MAT_CN(type);

        if( src1 )
        {
            CvScaledElWiseFunc func =
                (CvScaledElWiseFunc)(div_tab.fn_2d[CV_MAT_DEPTH(type)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[2], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.ptr[0], CV_STUB_STEP,
                                 iterator.size, scale ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        else
        {
            CvRecipFunc func = (CvRecipFunc)(recip_tab.fn_2d[CV_MAT_DEPTH(type)]);

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[1], CV_STUB_STEP,
                                 iterator.ptr[0], CV_STUB_STEP,
                                 iterator.size, scale ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        EXIT;
    }

    if( !CV_ARE_TYPES_EQ( src2, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src2, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    type = CV_MAT_TYPE(src2->type);
    size = cvGetMatSize( src2 );
    size.width *= CV_MAT_CN( type );

    if( CV_IS_MAT_CONT( src1_cont_flag & src2->type & dst->type ))
    {
        size.width *= size.height;
        src1_step = src2_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }
    else
    {
        src1_step = src1 ? src1->step : 0;
        src2_step = src2->step;
        dst_step = dst->step;
    }

    if( src1 )
    {
        CvScaledElWiseFunc func = (CvScaledElWiseFunc)(div_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src1->data.ptr, src1_step, src2->data.ptr, src2_step,
                         dst->data.ptr, dst_step, size, scale ));
    }
    else
    {
        CvRecipFunc func = (CvRecipFunc)(recip_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src2->data.ptr, src2_step,
                         dst->data.ptr, dst_step, size, scale ));
    }

    __END__;
}
// 

#if defined WIN32 || defined WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct
{
    const char* file;
    int         line;
}
CvStackRecord;

typedef struct CvContext
{
    int  err_code;
    int  err_mode;
    CvErrorCallback error_callback;
    void*  userdata;
    char  err_msg[4096];
    CvStackRecord  err_ctx;
} CvContext;

#if defined WIN32 || defined WIN64
#define CV_DEFAULT_ERROR_CALLBACK  cvGuiBoxReport
#else
#define CV_DEFAULT_ERROR_CALLBACK  cvStdErrReport
#endif

static CvContext*
icvCreateContext(void)
{
    CvContext* context = (CvContext*)malloc( sizeof(*context) );

    context->err_mode = CV_ErrModeLeaf;
    context->err_code = CV_StsOk;

    context->error_callback = CV_DEFAULT_ERROR_CALLBACK;
    context->userdata = 0;

    return context;
}

static void
icvDestroyContext(CvContext* context)
{
    free(context);
}

#if defined WIN32 || defined WIN64
    static DWORD g_TlsIndex = TLS_OUT_OF_INDEXES;
#else
    static pthread_key_t g_TlsIndex;
#endif

static CvContext*
icvGetContext(void)
{
#ifdef CV_DLL
#if defined WIN32 || defined WIN64
    CvContext* context;

    //assert(g_TlsIndex != TLS_OUT_OF_INDEXES);
    if( g_TlsIndex == TLS_OUT_OF_INDEXES )
    {
        g_TlsIndex = TlsAlloc();
        if( g_TlsIndex == TLS_OUT_OF_INDEXES )
            FatalAppExit( 0, "Only set CV_DLL for DLL usage" );
    }

    context = (CvContext*)TlsGetValue( g_TlsIndex );
    if( !context )
    {
        context = icvCreateContext();
        if( !context )
            FatalAppExit( 0, "OpenCV. Problem to allocate memory for TLS OpenCV context." );

        TlsSetValue( g_TlsIndex, context );
    }
    return context;
#else
    CvContext* context = (CvContext*)pthread_getspecific( g_TlsIndex );
    if( !context )
    {
    context = icvCreateContext();
    if( !context )
    {
            fprintf(stderr,"OpenCV. Problem to allocate memory for OpenCV context.");
        exit(1);
    }
    pthread_setspecific( g_TlsIndex, context );
    }
    return context;
#endif
#else /* static single-thread library case */
    static CvContext* context = 0;

    if( !context )
        context = icvCreateContext();

    return context;
#endif
}


CV_IMPL int
cvStdErrReport( int code, const char *func_name, const char *err_msg,
                const char *file, int line, void* )
{
    if( code == CV_StsBackTrace || code == CV_StsAutoTrace )
        fprintf( stderr, "\tcalled from " );
    else
        fprintf( stderr, "OpenCV ERROR: %s (%s)\n\tin function ",
                 cvErrorStr(code), err_msg ? err_msg : "no description" );

    fprintf( stderr, "%s, %s(%d)\n", func_name ? func_name : "<unknown>",
             file != NULL ? file : "", line );

    if( cvGetErrMode() == CV_ErrModeLeaf )
    {
        fprintf( stderr, "Terminating the application...\n" );
        return 1;
    }
    else
        return 0;
}


CV_IMPL int
cvGuiBoxReport( int code, const char *func_name, const char *err_msg,
                const char *file, int line, void* )
{
#if !defined WIN32 && !defined WIN64
    return cvStdErrReport( code, func_name, err_msg, file, line, 0 );
#else
    if( code != CV_StsBackTrace && code != CV_StsAutoTrace )
    {
        size_t msg_len = strlen(err_msg ? err_msg : "") + 1024;
       // char* message = (char*)alloca(msg_len);     delete by Jacky
       //char title[100];                             delete by Jacky

//+++++++++++++++++  add by Jacky for char to LPWSTR  ++++++++++++++++++++++++//
		LPWSTR  message = new WCHAR[msg_len];   //add by Jacky
  
		int nSize1 = MultiByteToWideChar(CP_ACP, 0, err_msg, -1, NULL, 0);
		LPWSTR  werr_msg = new WCHAR[nSize1];
		MultiByteToWideChar(CP_ACP, 0, err_msg, -1, werr_msg, nSize1);
		LPWSTR  title  = new WCHAR[100];
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

        wsprintf( (LPSTR)message, (LPCSTR)"%s (%s)\nin function %s, %s(%d)\n\n"
                  "Press \"Abort\" to terminate application.\n"
                  "Press \"Retry\" to debug (if the app is running under debugger).\n"
                  "Press \"Ignore\" to continue (this is not safe).\n",
                  cvErrorStr(code), werr_msg ? werr_msg : L"no description",
                  func_name, file, line );

        wsprintf( (LPSTR)title, "OpenCV GUI Error Handler" );

        int answer = MessageBox( NULL, (LPCSTR)message, (LPCSTR)title, MB_ICONERROR|MB_ABORTRETRYIGNORE|MB_SYSTEMMODAL );

        if( answer == IDRETRY )
        {
            CV_DBG_BREAK();
        }
//++++++++++  add by Jacky ++++++++++++++++++++//
		delete [] message;
		delete [] werr_msg;
		delete [] title;
//+++++++++++++++++++++++++++++++++++++++++++++//
        return answer != IDIGNORE;
    }
    return 0;
#endif
}


CV_IMPL int cvNulDevReport( int /*code*/, const char* /*func_name*/,
    const char* /*err_msg*/, const char* /*file*/, int /*line*/, void* )
{
    return cvGetErrMode() == CV_ErrModeLeaf;
}


CV_IMPL CvErrorCallback
cvRedirectError( CvErrorCallback func, void* userdata, void** prev_userdata )
{
    CvContext* context = icvGetContext();

    CvErrorCallback old = context->error_callback;
    if( prev_userdata )
        *prev_userdata = context->userdata;
    if( func )
    {
        context->error_callback = func;
        context->userdata = userdata;
    }
    else
    {
        context->error_callback = CV_DEFAULT_ERROR_CALLBACK;
        context->userdata = 0;
    }

    return old;
}


CV_IMPL int cvGetErrInfo( const char** errorcode_desc, const char** description,
                          const char** filename, int* line )
{
    int code = cvGetErrStatus();

    if( errorcode_desc )
        *errorcode_desc = cvErrorStr( code );

    if( code >= 0 )
    {
        if( description )
            *description = 0;
        if( filename )
            *filename = 0;
        if( line )
            *line = 0;
    }
    else
    {
        CvContext* ctx = icvGetContext();

        if( description )
            *description = ctx->err_msg;
        if( filename )
            *filename = ctx->err_ctx.file;
        if( line )
            *line = ctx->err_ctx.line;
    }

    return code;
}


CV_IMPL const char* cvErrorStr( int status )
{
    static char buf[256];

    switch (status)
    {
    case CV_StsOk :        return "No Error";
    case CV_StsBackTrace : return "Backtrace";
    case CV_StsError :     return "Unspecified error";
    case CV_StsInternal :  return "Internal error";
    case CV_StsNoMem :     return "Insufficient memory";
    case CV_StsBadArg :    return "Bad argument";
    case CV_StsNoConv :    return "Iterations do not converge";
    case CV_StsAutoTrace : return "Autotrace call";
    case CV_StsBadSize :   return "Incorrect size of input array";
    case CV_StsNullPtr :   return "Null pointer";
    case CV_StsDivByZero : return "Divizion by zero occured";
    case CV_BadStep :      return "Image step is wrong";
    case CV_StsInplaceNotSupported : return "Inplace operation is not supported";
    case CV_StsObjectNotFound :      return "Requested object was not found";
    case CV_BadDepth :     return "Input image depth is not supported by function";
    case CV_StsUnmatchedFormats : return "Formats of input arguments do not match";
    case CV_StsUnmatchedSizes :  return "Sizes of input arguments do not match";
    case CV_StsOutOfRange : return "One of arguments\' values is out of range";
    case CV_StsUnsupportedFormat : return "Unsupported format or combination of formats";
    case CV_BadCOI :      return "Input COI is not supported";
    case CV_BadNumChannels : return "Bad number of channels";
    case CV_StsBadFlag :   return "Bad flag (parameter or structure field)";
    case CV_StsBadPoint :  return "Bad parameter of type CvPoint";
    case CV_StsBadMask : return "Bad type of mask argument";
    case CV_StsParseError : return "Parsing error";
    case CV_StsNotImplemented : return "The function/feature is not implemented";
    case CV_StsBadMemBlock :  return "Memory block has been corrupted";
    };

    sprintf(buf, "Unknown %s code %d", status >= 0 ? "status":"error", status);
    return buf;
}

CV_IMPL int cvGetErrMode(void)
{
    return icvGetContext()->err_mode;
}

CV_IMPL int cvSetErrMode( int mode )
{
    CvContext* context = icvGetContext();
    int prev_mode = context->err_mode;
    context->err_mode = mode;
    return prev_mode;
}

CV_IMPL int cvGetErrStatus()
{
    return icvGetContext()->err_code;
}

CV_IMPL void cvSetErrStatus( int code )
{
    icvGetContext()->err_code = code;
}


CV_IMPL void cvError( int code, const char* func_name,
                      const char* err_msg,
                      const char* file_name, int line )
{
    if( code == CV_StsOk )
        cvSetErrStatus( code );
    else
    {
        CvContext* context = icvGetContext();

        if( code != CV_StsBackTrace && code != CV_StsAutoTrace )
        {
            char* message = context->err_msg;
            context->err_code = code;

            strcpy( message, err_msg );
            context->err_ctx.file = file_name;
            context->err_ctx.line = line;
        }

        if( context->err_mode != CV_ErrModeSilent )
        {
            int terminate = context->error_callback( code, func_name, err_msg,
                                                    file_name, line, context->userdata );
            if( terminate )
            {
#if !defined WIN32 && !defined WIN64
                assert(0); // for post-mortem analysis with GDB
#endif
                exit(-abs(terminate));
            }
        }
    }
}


/******************** End of implementation of profiling stuff *********************/


/**********************DllMain********************************/

#if defined WIN32 || defined WIN64
BOOL WINAPI DllMain( HINSTANCE, DWORD  fdwReason, LPVOID )
{
    CvContext *pContext;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_TlsIndex = TlsAlloc();
        if( g_TlsIndex == TLS_OUT_OF_INDEXES ) return FALSE;
        //break;

    case DLL_THREAD_ATTACH:
        pContext = icvCreateContext();
        if( pContext == NULL)
            return FALSE;
        TlsSetValue( g_TlsIndex, (LPVOID)pContext );
        break;

    case DLL_THREAD_DETACH:
        if( g_TlsIndex != TLS_OUT_OF_INDEXES )
        {
            pContext = (CvContext*)TlsGetValue( g_TlsIndex );
            if( pContext != NULL )
                icvDestroyContext( pContext );
        }
        break;

    case DLL_PROCESS_DETACH:
        if( g_TlsIndex != TLS_OUT_OF_INDEXES )
        {
            pContext = (CvContext*)TlsGetValue( g_TlsIndex );
            if( pContext != NULL )
                icvDestroyContext( pContext );
        }
        TlsFree( g_TlsIndex );
        break;
    default:
        ;
    }
    return TRUE;
}
#else
/* POSIX pthread */

/* function - destructor of thread */
void icvPthreadDestructor(void* key_val)
{
    CvContext* context = (CvContext*) key_val;
    icvDestroyContext( context );
}

int pthrerr = pthread_key_create( &g_TlsIndex, icvPthreadDestructor );

#endif

/* function, which converts int to int */
CV_IMPL int
cvErrorFromIppStatus( int status )
{
    switch (status)
    {
    case CV_BADSIZE_ERR: return CV_StsBadSize;
    case CV_BADMEMBLOCK_ERR: return CV_StsBadMemBlock;
    case CV_NULLPTR_ERR: return CV_StsNullPtr;
    case CV_DIV_BY_ZERO_ERR: return CV_StsDivByZero;
    case CV_BADSTEP_ERR: return CV_BadStep ;
    case CV_OUTOFMEM_ERR: return CV_StsNoMem;
    case CV_BADARG_ERR: return CV_StsBadArg;
    case CV_NOTDEFINED_ERR: return CV_StsError;
    case CV_INPLACE_NOT_SUPPORTED_ERR: return CV_StsInplaceNotSupported;
    case CV_NOTFOUND_ERR: return CV_StsObjectNotFound;
    case CV_BADCONVERGENCE_ERR: return CV_StsNoConv;
    case CV_BADDEPTH_ERR: return CV_BadDepth;
    case CV_UNMATCHED_FORMATS_ERR: return CV_StsUnmatchedFormats;
    case CV_UNSUPPORTED_COI_ERR: return CV_BadCOI;
    case CV_UNSUPPORTED_CHANNELS_ERR: return CV_BadNumChannels;
    case CV_BADFLAG_ERR: return CV_StsBadFlag;
    case CV_BADRANGE_ERR: return CV_StsBadArg;
    case CV_BADCOEF_ERR: return CV_StsBadArg;
    case CV_BADFACTOR_ERR: return CV_StsBadArg;
    case CV_BADPOINT_ERR: return CV_StsBadPoint;

    default: return CV_StsError;
    }
}
/* End of file */



