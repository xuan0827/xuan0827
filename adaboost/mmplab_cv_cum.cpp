#include "stdafx.h"
#include "mmplab_cv.h"

#define  ICV_DEF_ACC_FUNC( name, srctype, dsttype, cvtmacro )           \
IPCVAPI_IMPL( CvStatus,                                                 \
name,( const srctype *src, int srcstep, dsttype *dst,                   \
       int dststep, CvSize size ), (src, srcstep, dst, dststep, size )) \
                                                                        \
{                                                                       \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 4; x += 4 )                       \
        {                                                               \
            dsttype t0 = dst[x] + cvtmacro(src[x]);                     \
            dsttype t1 = dst[x + 1] + cvtmacro(src[x + 1]);             \
            dst[x] = t0;  dst[x + 1] = t1;                              \
                                                                        \
            t0 = dst[x + 2] + cvtmacro(src[x + 2]);                     \
            t1 = dst[x + 3] + cvtmacro(src[x + 3]);                     \
            dst[x + 2] = t0;  dst[x + 3] = t1;                          \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            dst[x] += cvtmacro(src[x]);                                 \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


ICV_DEF_ACC_FUNC( icvAdd_8u32f_C1IR, uchar, float, CV_8TO32F )
ICV_DEF_ACC_FUNC( icvAdd_32f_C1IR, float, float, CV_NOP )
ICV_DEF_ACC_FUNC( icvAddSquare_8u32f_C1IR, uchar, float, CV_8TO32F_SQR )
ICV_DEF_ACC_FUNC( icvAddSquare_32f_C1IR, float, float, CV_SQR )


#define  ICV_DEF_ACCPROD_FUNC( flavor, srctype, dsttype, cvtmacro )         \
IPCVAPI_IMPL( CvStatus, icvAddProduct_##flavor##_C1IR,                      \
( const srctype *src1, int step1, const srctype *src2, int step2,           \
  dsttype *dst, int dststep, CvSize size ),                                 \
 (src1, step1, src2, step2, dst, dststep, size) )                           \
{                                                                           \
    step1 /= sizeof(src1[0]);                                               \
    step2 /= sizeof(src2[0]);                                               \
    dststep /= sizeof(dst[0]);                                              \
                                                                            \
    for( ; size.height--; src1 += step1, src2 += step2, dst += dststep )    \
    {                                                                       \
        int x;                                                              \
        for( x = 0; x <= size.width - 4; x += 4 )                           \
        {                                                                   \
            dsttype t0 = dst[x] + cvtmacro(src1[x])*cvtmacro(src2[x]);      \
            dsttype t1 = dst[x+1] + cvtmacro(src1[x+1])*cvtmacro(src2[x+1]);\
            dst[x] = t0;  dst[x + 1] = t1;                                  \
                                                                            \
            t0 = dst[x + 2] + cvtmacro(src1[x + 2])*cvtmacro(src2[x + 2]);  \
            t1 = dst[x + 3] + cvtmacro(src1[x + 3])*cvtmacro(src2[x + 3]);  \
            dst[x + 2] = t0;  dst[x + 3] = t1;                              \
        }                                                                   \
                                                                            \
        for( ; x < size.width; x++ )                                        \
            dst[x] += cvtmacro(src1[x])*cvtmacro(src2[x]);                  \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}
// 
// 
ICV_DEF_ACCPROD_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCPROD_FUNC( 32f, float, float, CV_NOP )


#define  ICV_DEF_ACCWEIGHT_FUNC( flavor, srctype, dsttype, cvtmacro )   \
IPCVAPI_IMPL( CvStatus, icvAddWeighted_##flavor##_C1IR,                 \
( const srctype *src, int srcstep, dsttype *dst, int dststep,           \
  CvSize size, dsttype alpha ), (src, srcstep, dst, dststep, size, alpha) )\
{                                                                       \
    dsttype beta = (dsttype)(1 - alpha);                                \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 4; x += 4 )                       \
        {                                                               \
            dsttype t0 = dst[x]*beta + cvtmacro(src[x])*alpha;          \
            dsttype t1 = dst[x+1]*beta + cvtmacro(src[x+1])*alpha;      \
            dst[x] = t0; dst[x + 1] = t1;                               \
                                                                        \
            t0 = dst[x + 2]*beta + cvtmacro(src[x + 2])*alpha;          \
            t1 = dst[x + 3]*beta + cvtmacro(src[x + 3])*alpha;          \
            dst[x + 2] = t0; dst[x + 3] = t1;                           \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            dst[x] = dst[x]*beta + cvtmacro(src[x])*alpha;              \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
 }
// 
// 
ICV_DEF_ACCWEIGHT_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCWEIGHT_FUNC( 32f, float, float, CV_NOP )
// 
// 
#define  ICV_DEF_ACCMASK_FUNC_C1( name, srctype, dsttype, cvtmacro )    \
IPCVAPI_IMPL( CvStatus,                                                 \
name,( const srctype *src, int srcstep, const uchar* mask, int maskstep,\
       dsttype *dst, int dststep, CvSize size ),                        \
       (src, srcstep, mask, maskstep, dst, dststep, size ))             \
{                                                                       \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 2; x += 2 )                       \
        {                                                               \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src[x]);                             \
            if( mask[x+1] )                                             \
                dst[x+1] += cvtmacro(src[x+1]);                         \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src[x]);                             \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}
// 
// 
ICV_DEF_ACCMASK_FUNC_C1( icvAdd_8u32f_C1IMR, uchar, float, CV_8TO32F )
ICV_DEF_ACCMASK_FUNC_C1( icvAdd_32f_C1IMR, float, float, CV_NOP )
ICV_DEF_ACCMASK_FUNC_C1( icvAddSquare_8u32f_C1IMR, uchar, float, CV_8TO32F_SQR )
ICV_DEF_ACCMASK_FUNC_C1( icvAddSquare_32f_C1IMR, float, float, CV_SQR )


 #define  ICV_DEF_ACCPRODUCTMASK_FUNC_C1( flavor, srctype, dsttype, cvtmacro )  \
IPCVAPI_IMPL( CvStatus, icvAddProduct_##flavor##_C1IMR,                 \
( const srctype *src1, int step1, const srctype* src2, int step2,       \
  const uchar* mask, int maskstep, dsttype *dst, int dststep, CvSize size ),\
  (src1, step1, src2, step2, mask, maskstep, dst, dststep, size ))      \
{                                                                       \
    step1 /= sizeof(src1[0]);                                           \
    step2 /= sizeof(src2[0]);                                           \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src1 += step1, src2 += step2,                 \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 2; x += 2 )                       \
        {                                                               \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src1[x])*cvtmacro(src2[x]);          \
            if( mask[x+1] )                                             \
                dst[x+1] += cvtmacro(src1[x+1])*cvtmacro(src2[x+1]);    \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            if( mask[x] )                                               \
                dst[x] += cvtmacro(src1[x])*cvtmacro(src2[x]);          \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}
// 
// 
ICV_DEF_ACCPRODUCTMASK_FUNC_C1( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCPRODUCTMASK_FUNC_C1( 32f, float, float, CV_NOP )

#define  ICV_DEF_ACCWEIGHTMASK_FUNC_C1( flavor, srctype, dsttype, cvtmacro ) \
IPCVAPI_IMPL( CvStatus, icvAddWeighted_##flavor##_C1IMR,                \
( const srctype *src, int srcstep, const uchar* mask, int maskstep,     \
  dsttype *dst, int dststep, CvSize size, dsttype alpha ),              \
  (src, srcstep, mask, maskstep, dst, dststep, size, alpha ))           \
{                                                                       \
    dsttype beta = (dsttype)(1 - alpha);                                \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x <= size.width - 2; x += 2 )                       \
        {                                                               \
            if( mask[x] )                                               \
                dst[x] = dst[x]*beta + cvtmacro(src[x])*alpha;          \
            if( mask[x+1] )                                             \
                dst[x+1] = dst[x+1]*beta + cvtmacro(src[x+1])*alpha;    \
        }                                                               \
                                                                        \
        for( ; x < size.width; x++ )                                    \
            if( mask[x] )                                               \
                dst[x] = dst[x]*beta + cvtmacro(src[x])*alpha;          \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}
// 
ICV_DEF_ACCWEIGHTMASK_FUNC_C1( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCWEIGHTMASK_FUNC_C1( 32f, float, float, CV_NOP )

// 
 #define  ICV_DEF_ACCMASK_FUNC_C3( name, srctype, dsttype, cvtmacro )    \
IPCVAPI_IMPL( CvStatus,                                                 \
name,( const srctype *src, int srcstep, const uchar* mask, int maskstep,\
       dsttype *dst, int dststep, CvSize size ),                        \
       (src, srcstep, mask, maskstep, dst, dststep, size ))             \
{                                                                       \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x < size.width; x++ )                               \
            if( mask[x] )                                               \
            {                                                           \
                dsttype t0, t1, t2;                                     \
                t0 = dst[x*3] + cvtmacro(src[x*3]);                     \
                t1 = dst[x*3+1] + cvtmacro(src[x*3+1]);                 \
                t2 = dst[x*3+2] + cvtmacro(src[x*3+2]);                 \
                dst[x*3] = t0;                                          \
                dst[x*3+1] = t1;                                        \
                dst[x*3+2] = t2;                                        \
            }                                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}
// 
// 
ICV_DEF_ACCMASK_FUNC_C3( icvAdd_8u32f_C3IMR, uchar, float, CV_8TO32F )
ICV_DEF_ACCMASK_FUNC_C3( icvAdd_32f_C3IMR, float, float, CV_NOP )
ICV_DEF_ACCMASK_FUNC_C3( icvAddSquare_8u32f_C3IMR, uchar, float, CV_8TO32F_SQR )
ICV_DEF_ACCMASK_FUNC_C3( icvAddSquare_32f_C3IMR, float, float, CV_SQR )
// 
// 
#define  ICV_DEF_ACCPRODUCTMASK_FUNC_C3( flavor, srctype, dsttype, cvtmacro )  \
IPCVAPI_IMPL( CvStatus, icvAddProduct_##flavor##_C3IMR,                 \
( const srctype *src1, int step1, const srctype* src2, int step2,       \
  const uchar* mask, int maskstep, dsttype *dst, int dststep, CvSize size ),\
  (src1, step1, src2, step2, mask, maskstep, dst, dststep, size ))      \
{                                                                       \
    step1 /= sizeof(src1[0]);                                           \
    step2 /= sizeof(src2[0]);                                           \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src1 += step1, src2 += step2,                 \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x < size.width; x++ )                               \
            if( mask[x] )                                               \
            {                                                           \
                dsttype t0, t1, t2;                                     \
                t0 = dst[x*3]+cvtmacro(src1[x*3])*cvtmacro(src2[x*3]);  \
                t1 = dst[x*3+1]+cvtmacro(src1[x*3+1])*cvtmacro(src2[x*3+1]);\
                t2 = dst[x*3+2]+cvtmacro(src1[x*3+2])*cvtmacro(src2[x*3+2]);\
                dst[x*3] = t0;                                          \
                dst[x*3+1] = t1;                                        \
                dst[x*3+2] = t2;                                        \
            }                                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}
// 
// 
ICV_DEF_ACCPRODUCTMASK_FUNC_C3( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCPRODUCTMASK_FUNC_C3( 32f, float, float, CV_NOP )


#define  ICV_DEF_ACCWEIGHTMASK_FUNC_C3( flavor, srctype, dsttype, cvtmacro ) \
IPCVAPI_IMPL( CvStatus, icvAddWeighted_##flavor##_C3IMR,                \
( const srctype *src, int srcstep, const uchar* mask, int maskstep,     \
  dsttype *dst, int dststep, CvSize size, dsttype alpha ),              \
  (src, srcstep, mask, maskstep, dst, dststep, size, alpha ))           \
{                                                                       \
    dsttype beta = (dsttype)(1 - alpha);                                \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep,                               \
                          dst += dststep, mask += maskstep )            \
    {                                                                   \
        int x;                                                          \
        for( x = 0; x < size.width; x++ )                               \
            if( mask[x] )                                               \
            {                                                           \
                dsttype t0, t1, t2;                                     \
                t0 = dst[x*3]*beta + cvtmacro(src[x*3])*alpha;          \
                t1 = dst[x*3+1]*beta + cvtmacro(src[x*3+1])*alpha;      \
                t2 = dst[x*3+2]*beta + cvtmacro(src[x*3+2])*alpha;      \
                dst[x*3] = t0;                                          \
                dst[x*3+1] = t1;                                        \
                dst[x*3+2] = t2;                                        \
            }                                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

ICV_DEF_ACCWEIGHTMASK_FUNC_C3( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_ACCWEIGHTMASK_FUNC_C3( 32f, float, float, CV_NOP )


#define  ICV_DEF_INIT_ACC_TAB( FUNCNAME )                                           \
static  void  icvInit##FUNCNAME##Table( CvFuncTable* tab, CvBigFuncTable* masktab ) \
{                                                                                   \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u32f_C1IR;                          \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_C1IR;                           \
                                                                                    \
    masktab->fn_2d[CV_8UC1] = (void*)icv##FUNCNAME##_8u32f_C1IMR;                   \
    masktab->fn_2d[CV_32FC1] = (void*)icv##FUNCNAME##_32f_C1IMR;                    \
                                                                                    \
    masktab->fn_2d[CV_8UC3] = (void*)icv##FUNCNAME##_8u32f_C3IMR;                   \
    masktab->fn_2d[CV_32FC3] = (void*)icv##FUNCNAME##_32f_C3IMR;                    \
}


ICV_DEF_INIT_ACC_TAB( Add )
ICV_DEF_INIT_ACC_TAB( AddSquare )
ICV_DEF_INIT_ACC_TAB( AddProduct )
ICV_DEF_INIT_ACC_TAB( AddWeighted )


CV_IMPL void
cvAcc( const void* arr, void* sumarr, const void* maskarr )
{
    static CvFuncTable acc_tab;
    static CvBigFuncTable accmask_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvAcc" );

    __BEGIN__;

    int type, sumdepth;
    int mat_step, sum_step, mask_step = 0;
    CvSize size;
    CvMat stub, *mat = (CvMat*)arr;
    CvMat sumstub, *sum = (CvMat*)sumarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;

 

    if( !CV_IS_MAT( mat ) || !CV_IS_MAT( sum ))
    {
        int coi1 = 0, coi2 = 0;
        CV_CALL( mat = cvGetMat( mat, &stub, &coi1 ));
        CV_CALL( sum = cvGetMat( sum, &sumstub, &coi2 ));
        if( coi1 + coi2 != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( CV_MAT_DEPTH( sum->type ) != CV_32F )
        CV_ERROR( CV_BadDepth, "" );

    if( !CV_ARE_CNS_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    sumdepth = CV_MAT_DEPTH( sum->type );
    if( sumdepth != CV_32F && (maskarr != 0 || sumdepth != CV_64F))
        CV_ERROR( CV_BadDepth, "Bad accumulator type" );

    if( !CV_ARE_SIZES_EQ( mat, sum ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( mat );
    type = CV_MAT_TYPE( mat->type );

    mat_step = mat->step;
    sum_step = sum->step;

    if( !mask )
    {
        CvFunc2D_2A func=(CvFunc2D_2A)acc_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "Unsupported type combination" );

        size.width *= CV_MAT_CN(type);
        if( CV_IS_MAT_CONT( mat->type & sum->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step, sum->data.ptr, sum_step, size ));
    }
    else
    {
        CvFunc2D_3A func = (CvFunc2D_3A)accmask_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );            

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & sum->type & mask->type ))
        {
            size.width *= size.height;
            mat_step = sum_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr, mask_step,
                         sum->data.ptr, sum_step, size ));
    }

    __END__;
}

static void
icvAdaptiveThreshold_MeanC( const CvMat* src, CvMat* dst, int method,
                            int maxValue, int type, int size, double delta )
{
    CvMat* mean = 0;
    CV_FUNCNAME( "icvAdaptiveThreshold_MeanC" );

    __BEGIN__;

    int i, j, rows, cols;
    int idelta = type == CV_THRESH_BINARY ? cvCeil(delta) : cvFloor(delta);
    uchar tab[768];

    if( size <= 1 || (size&1) == 0 )
        CV_ERROR( CV_StsOutOfRange, "Neighborhood size must be >=3 and odd (3, 5, 7, ...)" );

    if( maxValue < 0 )
    {
        CV_CALL( cvSetZero( dst ));
        EXIT;
    }

    rows = src->rows;
    cols = src->cols;

    if( src->data.ptr != dst->data.ptr )
        mean = dst;
    else
        CV_CALL( mean = cvCreateMat( rows, cols, CV_8UC1 ));

    CV_CALL( cvSmooth( src, mean, method == CV_ADAPTIVE_THRESH_MEAN_C ?
                       CV_BLUR : CV_GAUSSIAN, size, size ));
    if( maxValue > 255 )
        maxValue = 255;

    if( type == CV_THRESH_BINARY )
        for( i = 0; i < 768; i++ )
            tab[i] = (uchar)(i - 255 > -idelta ? maxValue : 0);
    else
        for( i = 0; i < 768; i++ )
            tab[i] = (uchar)(i - 255 <= -idelta ? maxValue : 0);

    for( i = 0; i < rows; i++ )
    {
        const uchar* s = src->data.ptr + i*src->step;
        const uchar* m = mean->data.ptr + i*mean->step;
        uchar* d = dst->data.ptr + i*dst->step;

        for( j = 0; j < cols; j++ )
            d[j] = tab[s[j] - m[j] + 255];
    }

    __END__;

    if( mean != dst )
        cvReleaseMat( &mean );
}
// 
// 
CV_IMPL void
cvAdaptiveThreshold( const void *srcIm, void *dstIm, double maxValue,
                     int method, int type, int blockSize, double param1 )
{
    CvMat src_stub, dst_stub;
    CvMat *src = 0, *dst = 0;

    CV_FUNCNAME( "cvAdaptiveThreshold" );

    __BEGIN__;

    if( type != CV_THRESH_BINARY && type != CV_THRESH_BINARY_INV )
        CV_ERROR( CV_StsBadArg, "Only CV_TRESH_BINARY and CV_THRESH_BINARY_INV "
                                "threshold types are acceptable" );

    CV_CALL( src = cvGetMat( srcIm, &src_stub ));
    CV_CALL( dst = cvGetMat( dstIm, &dst_stub ));

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_TYPE(dst->type) != CV_8UC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    switch( method )
    {
    case CV_ADAPTIVE_THRESH_MEAN_C:
    case CV_ADAPTIVE_THRESH_GAUSSIAN_C:
        CV_CALL( icvAdaptiveThreshold_MeanC( src, dst, method, cvRound(maxValue),type,
                                             blockSize, param1 ));
        break;
    default:
        CV_ERROR( CV_BADCOEF_ERR, "" );
    }

    __END__;
}
// 
// /* End of file. */
typedef struct _CvPtInfo
{
    CvPoint pt;
    int k;                      /* support region */
    int s;                      /* curvature value */
    struct _CvPtInfo *next;
}
_CvPtInfo;
// 
// 
// /* curvature: 0 - 1-curvature, 1 - k-cosine curvature. */
CvStatus
icvApproximateChainTC89( CvChain*               chain,
                         int                    header_size,
                         CvMemStorage*          storage, 
                         CvSeq**                contour, 
                         int    method )
{
    static const int abs_diff[] = { 1, 2, 3, 4, 3, 2, 1, 0, 1, 2, 3, 4, 3, 2, 1 };

    char            local_buffer[1 << 16];
    char*           buffer = local_buffer;
    int             buffer_size;

    _CvPtInfo       temp;
    _CvPtInfo       *array, *first = 0, *current = 0, *prev_current = 0;
    int             i, j, i1, i2, s, len;
    int             count;

    CvChainPtReader reader;
    CvSeqWriter     writer;
    CvPoint         pt = chain->origin;
   
    assert( chain && contour && buffer );

    buffer_size = (chain->total + 8) * sizeof( _CvPtInfo );

    *contour = 0;

    if( !CV_IS_SEQ_CHAIN_CONTOUR( chain ))
        return CV_BADFLAG_ERR;

    if( header_size < (int)sizeof(CvContour) )
        return CV_BADSIZE_ERR;
    
    cvStartWriteSeq( (chain->flags & ~CV_SEQ_ELTYPE_MASK) | CV_SEQ_ELTYPE_POINT,
                     header_size, sizeof( CvPoint ), storage, &writer );
    
    if( chain->total == 0 )
    {        
        CV_WRITE_SEQ_ELEM( pt, writer );
        goto exit_function;
    }

    cvStartReadChainPoints( chain, &reader );

    if( method > CV_CHAIN_APPROX_SIMPLE && buffer_size > (int)sizeof(local_buffer))
    {
        buffer = (char *) cvAlloc( buffer_size );
        if( !buffer )
            return CV_OUTOFMEM_ERR;
    }

    array = (_CvPtInfo *) buffer;
    count = chain->total;

    temp.next = 0;
    current = &temp;

    /* Pass 0.
       Restores all the digital curve points from the chain code.
       Removes the points (from the resultant polygon)
       that have zero 1-curvature */
    for( i = 0; i < count; i++ )
    {
        int prev_code = *reader.prev_elem;

        reader.prev_elem = reader.ptr;
        CV_READ_CHAIN_POINT( pt, reader );

        /* calc 1-curvature */
        s = abs_diff[reader.code - prev_code + 7];

        if( method <= CV_CHAIN_APPROX_SIMPLE )
        {
            if( method == CV_CHAIN_APPROX_NONE || s != 0 )
            {
                CV_WRITE_SEQ_ELEM( pt, writer );
            }
        }
        else
        {
            if( s != 0 )
                current = current->next = array + i;
            array[i].s = s;
            array[i].pt = pt;
        }
    }

    //assert( pt.x == chain->origin.x && pt.y == chain->origin.y );

    if( method <= CV_CHAIN_APPROX_SIMPLE )
        goto exit_function;

    current->next = 0;

    len = i;
    current = temp.next;

    assert( current );

    /* Pass 1.
       Determines support region for all the remained points */
    do
    {
        CvPoint pt0;
        int k, l = 0, d_num = 0;

        i = (int)(current - array);
        pt0 = array[i].pt;

        /* determine support region */
        for( k = 1;; k++ )
        {
            int lk, dk_num;
            int dx, dy;
            Cv32suf d;

            assert( k <= len );

            /* calc indices */
            i1 = i - k;
            i1 += i1 < 0 ? len : 0;
            i2 = i + k;
            i2 -= i2 >= len ? len : 0;

            dx = array[i2].pt.x - array[i1].pt.x;
            dy = array[i2].pt.y - array[i1].pt.y;

            /* distance between p_(i - k) and p_(i + k) */
            lk = dx * dx + dy * dy;

            /* distance between p_i and the line (p_(i-k), p_(i+k)) */
            dk_num = (pt0.x - array[i1].pt.x) * dy - (pt0.y - array[i1].pt.y) * dx;
            d.f = (float) (((double) d_num) * lk - ((double) dk_num) * l);

            if( k > 1 && (l >= lk || (d_num > 0 && d.i <= 0 || d_num < 0 && d.i >= 0)))
                break;

            d_num = dk_num;
            l = lk;
        }

        current->k = --k;

        /* determine cosine curvature if it should be used */
        if( method == CV_CHAIN_APPROX_TC89_KCOS )
        {
            /* calc k-cosine curvature */
            for( j = k, s = 0; j > 0; j-- )
            {
                double temp_num;
                int dx1, dy1, dx2, dy2;
                Cv32suf sk;

                i1 = i - j;
                i1 += i1 < 0 ? len : 0;
                i2 = i + j;
                i2 -= i2 >= len ? len : 0;

                dx1 = array[i1].pt.x - pt0.x;
                dy1 = array[i1].pt.y - pt0.y;
                dx2 = array[i2].pt.x - pt0.x;
                dy2 = array[i2].pt.y - pt0.y;

                if( (dx1 | dy1) == 0 || (dx2 | dy2) == 0 )
                    break;

                temp_num = dx1 * dx2 + dy1 * dy2;
                temp_num =
                    (float) (temp_num /
                             sqrt( ((double)dx1 * dx1 + (double)dy1 * dy1) *
                                   ((double)dx2 * dx2 + (double)dy2 * dy2) ));
                sk.f = (float) (temp_num + 1.1);

                assert( 0 <= sk.f && sk.f <= 2.2 );
                if( j < k && sk.i <= s )
                    break;

                s = sk.i;
            }
            current->s = s;
        }
        current = current->next;
    }
    while( current != 0 );

    prev_current = &temp;
    current = temp.next;

    /* Pass 2.
       Performs non-maxima supression */
    do
    {
        int k2 = current->k >> 1;

        s = current->s;
        i = (int)(current - array);

        for( j = 1; j <= k2; j++ )
        {
            i2 = i - j;
            i2 += i2 < 0 ? len : 0;

            if( array[i2].s > s )
                break;

            i2 = i + j;
            i2 -= i2 >= len ? len : 0;

            if( array[i2].s > s )
                break;
        }

        if( j <= k2 )           /* exclude point */
        {
            prev_current->next = current->next;
            current->s = 0;     /* "clear" point */
        }
        else
            prev_current = current;
        current = current->next;
    }
    while( current != 0 );

    /* Pass 3.
       Removes non-dominant points with 1-length support region */
    current = temp.next;
    assert( current );
    prev_current = &temp;

    do
    {
        if( current->k == 1 )
        {
            s = current->s;
            i = (int)(current - array);

            i1 = i - 1;
            i1 += i1 < 0 ? len : 0;

            i2 = i + 1;
            i2 -= i2 >= len ? len : 0;

            if( s <= array[i1].s || s <= array[i2].s )
            {
                prev_current->next = current->next;
                current->s = 0;
            }
            else
                prev_current = current;
        }
        else
            prev_current = current;
        current = current->next;
    }
    while( current != 0 );

    if( method == CV_CHAIN_APPROX_TC89_KCOS )
        goto copy_vect;

    /* Pass 4.
       Cleans remained couples of points */
    assert( temp.next );

    if( array[0].s != 0 && array[len - 1].s != 0 )      /* specific case */
    {
        for( i1 = 1; i1 < len && array[i1].s != 0; i1++ )
        {
            array[i1 - 1].s = 0;
        }
        if( i1 == len )
            goto copy_vect;     /* all points survived */
        i1--;

        for( i2 = len - 2; i2 > 0 && array[i2].s != 0; i2-- )
        {
            array[i2].next = 0;
            array[i2 + 1].s = 0;
        }
        i2++;

        if( i1 == 0 && i2 == len - 1 )  /* only two points */
        {
            i1 = (int)(array[0].next - array);
            array[len] = array[0];      /* move to the end */
            array[len].next = 0;
            array[len - 1].next = array + len;
        }
        temp.next = array + i1;
    }

    current = temp.next;
    first = prev_current = &temp;
    count = 1;

    /* do last pass */
    do
    {
        if( current->next == 0 || current->next - current != 1 )
        {
            if( count >= 2 )
            {
                if( count == 2 )
                {
                    int s1 = prev_current->s;
                    int s2 = current->s;

                    if( s1 > s2 || s1 == s2 && prev_current->k <= current->k )
                        /* remove second */
                        prev_current->next = current->next;
                    else
                        /* remove first */
                        first->next = current;
                }
                else
                    first->next->next = current;
            }
            first = current;
            count = 1;
        }
        else
            count++;
        prev_current = current;
        current = current->next;
    }
    while( current != 0 );

  copy_vect:

    /* gather points */
    current = temp.next;
    assert( current );

    do
    {
        CV_WRITE_SEQ_ELEM( current->pt, writer );
        current = current->next;
    }
    while( current != 0 );

exit_function:

    *contour = cvEndWriteSeq( &writer );

    assert( writer.seq->total > 0 );

    if( buffer != local_buffer )
        cvFree( &buffer );
    return CV_OK;
}

// 
// /****************************************************************************************\
// *                               Polygonal Approximation                                  *
// \****************************************************************************************/
// 
// /* Ramer-Douglas-Peucker algorithm for polygon simplification */
// 
// /* the version for integer point coordinates */
static CvStatus
icvApproxPolyDP_32s( CvSeq* src_contour, int header_size, 
                     CvMemStorage* storage,
                     CvSeq** dst_contour, float eps )
{
    int             init_iters = 3;
    CvSlice         slice = {0, 0}, right_slice = {0, 0};
    CvSeqReader     reader, reader2;
    CvSeqWriter     writer;
    CvPoint         start_pt = {INT_MIN, INT_MIN}, end_pt = {0, 0}, pt = {0,0};
    int             i = 0, j, count = src_contour->total, new_count;
    int             is_closed = CV_IS_SEQ_CLOSED( src_contour );
    int             le_eps = 0;
    CvMemStorage*   temp_storage = 0;
    CvSeq*          stack = 0;
    
    assert( CV_SEQ_ELTYPE(src_contour) == CV_32SC2 );
    cvStartWriteSeq( src_contour->flags, header_size, sizeof(pt), storage, &writer );

    if( src_contour->total == 0  )
    {
        *dst_contour = cvEndWriteSeq( &writer );
        return CV_OK;
    }

    temp_storage = cvCreateChildMemStorage( storage );

    assert( src_contour->first != 0 );
    stack = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvSlice), temp_storage );
    eps *= eps;
    cvStartReadSeq( src_contour, &reader, 0 );

    if( !is_closed )
    {
        right_slice.start_index = count;
        end_pt = *(CvPoint*)(reader.ptr);
        start_pt = *(CvPoint*)cvGetSeqElem( src_contour, -1 );

        if( start_pt.x != end_pt.x || start_pt.y != end_pt.y )
        {
            slice.start_index = 0;
            slice.end_index = count - 1;
            cvSeqPush( stack, &slice );
        }
        else
        {
            is_closed = 1;
            init_iters = 1;
        }
    }
    
    if( is_closed )
    {
        /* 1. Find approximately two farthest points of the contour */
        right_slice.start_index = 0;

        for( i = 0; i < init_iters; i++ )
        {
            int max_dist = 0;
            cvSetSeqReaderPos( &reader, right_slice.start_index, 1 );
            CV_READ_SEQ_ELEM( start_pt, reader );   /* read the first point */

            for( j = 1; j < count; j++ )
            {
                int dx, dy, dist;

                CV_READ_SEQ_ELEM( pt, reader );
                dx = pt.x - start_pt.x;
                dy = pt.y - start_pt.y;

                dist = dx * dx + dy * dy;

                if( dist > max_dist )
                {
                    max_dist = dist;
                    right_slice.start_index = j;
                }
            }

            le_eps = max_dist <= eps;
        }

        /* 2. initialize the stack */
        if( !le_eps )
        {
            slice.start_index = cvGetSeqReaderPos( &reader );
            slice.end_index = right_slice.start_index += slice.start_index;

            right_slice.start_index -= right_slice.start_index >= count ? count : 0;
            right_slice.end_index = slice.start_index;
            if( right_slice.end_index < right_slice.start_index )
                right_slice.end_index += count;

            cvSeqPush( stack, &right_slice );
            cvSeqPush( stack, &slice );
        }
        else
            CV_WRITE_SEQ_ELEM( start_pt, writer );
    }

    /* 3. run recursive process */
    while( stack->total != 0 )
    {
        cvSeqPop( stack, &slice );

        if( slice.end_index > slice.start_index + 1 )
        {
            int dx, dy, dist, max_dist = 0;
            
            cvSetSeqReaderPos( &reader, slice.end_index );
            CV_READ_SEQ_ELEM( end_pt, reader );

            cvSetSeqReaderPos( &reader, slice.start_index );
            CV_READ_SEQ_ELEM( start_pt, reader );

            dx = end_pt.x - start_pt.x;
            dy = end_pt.y - start_pt.y;

            assert( dx != 0 || dy != 0 );

            for( i = slice.start_index + 1; i < slice.end_index; i++ )
            {
                CV_READ_SEQ_ELEM( pt, reader );
                dist = abs((pt.y - start_pt.y) * dx - (pt.x - start_pt.x) * dy);

                if( dist > max_dist )
                {
                    max_dist = dist;
                    right_slice.start_index = i;
                }
            }

            le_eps = (double)max_dist * max_dist <= eps * ((double)dx * dx + (double)dy * dy);
        }
        else
        {
            assert( slice.end_index > slice.start_index );
            le_eps = 1;
            /* read starting point */
            cvSetSeqReaderPos( &reader, slice.start_index );
            CV_READ_SEQ_ELEM( start_pt, reader );
        }

        if( le_eps )
        {
            CV_WRITE_SEQ_ELEM( start_pt, writer );
        }
        else
        {
            right_slice.end_index = slice.end_index;
            slice.end_index = right_slice.start_index;
            cvSeqPush( stack, &right_slice );
            cvSeqPush( stack, &slice );
        }
    }

    is_closed = CV_IS_SEQ_CLOSED( src_contour );
    if( !is_closed )
        CV_WRITE_SEQ_ELEM( end_pt, writer );

    *dst_contour = cvEndWriteSeq( &writer );
    
    cvStartReadSeq( *dst_contour, &reader, is_closed );
    CV_READ_SEQ_ELEM( start_pt, reader );

    reader2 = reader;
    CV_READ_SEQ_ELEM( pt, reader );

    new_count = count = (*dst_contour)->total;
    for( i = !is_closed; i < count - !is_closed && new_count > 2; i++ )
    {
        int dx, dy, dist;
        CV_READ_SEQ_ELEM( end_pt, reader );

        dx = end_pt.x - start_pt.x;
        dy = end_pt.y - start_pt.y;
        dist = abs((pt.x - start_pt.x)*dy - (pt.y - start_pt.y)*dx);
        if( (double)dist * dist <= 0.5*eps*((double)dx*dx + (double)dy*dy) && dx != 0 && dy != 0 )
        {
            new_count--;
            *((CvPoint*)reader2.ptr) = start_pt = end_pt;
            CV_NEXT_SEQ_ELEM( sizeof(pt), reader2 );
            CV_READ_SEQ_ELEM( pt, reader );
            i++;
            continue;
        }
        *((CvPoint*)reader2.ptr) = start_pt = pt;
        CV_NEXT_SEQ_ELEM( sizeof(pt), reader2 );
        pt = end_pt;
    }

    if( !is_closed )
        *((CvPoint*)reader2.ptr) = pt;

    if( new_count < count )
        cvSeqPopMulti( *dst_contour, 0, count - new_count );

    cvReleaseMemStorage( &temp_storage );

    return CV_OK;
}
// 
// 
// /* the version for integer point coordinates */
static CvStatus
icvApproxPolyDP_32f( CvSeq* src_contour, int header_size, 
                     CvMemStorage* storage,
                     CvSeq** dst_contour, float eps )
{
    int             init_iters = 3;
    CvSlice         slice = {0, 0}, right_slice = {0, 0};
    CvSeqReader     reader, reader2;
    CvSeqWriter     writer;
    CvPoint2D32f    start_pt = {-1e6f, -1e6f}, end_pt = {0, 0}, pt = {0,0};
    int             i = 0, j, count = src_contour->total, new_count;
    int             is_closed = CV_IS_SEQ_CLOSED( src_contour );
    int             le_eps = 0;
    CvMemStorage*   temp_storage = 0;
    CvSeq*          stack = 0;
    
    assert( CV_SEQ_ELTYPE(src_contour) == CV_32FC2 );
    cvStartWriteSeq( src_contour->flags, header_size, sizeof(pt), storage, &writer );

    if( src_contour->total == 0  )
    {
        *dst_contour = cvEndWriteSeq( &writer );
        return CV_OK;
    }

    temp_storage = cvCreateChildMemStorage( storage );

    assert( src_contour->first != 0 );
    stack = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvSlice), temp_storage );
    eps *= eps;
    cvStartReadSeq( src_contour, &reader, 0 );

    if( !is_closed )
    {
        right_slice.start_index = count;
        end_pt = *(CvPoint2D32f*)(reader.ptr);
        start_pt = *(CvPoint2D32f*)cvGetSeqElem( src_contour, -1 );

        if( fabs(start_pt.x - end_pt.x) > FLT_EPSILON ||
            fabs(start_pt.y - end_pt.y) > FLT_EPSILON )
        {
            slice.start_index = 0;
            slice.end_index = count - 1;
            cvSeqPush( stack, &slice );
        }
        else
        {
            is_closed = 1;
            init_iters = 1;
        }
    }
    
    if( is_closed )
    {
        /* 1. Find approximately two farthest points of the contour */
        right_slice.start_index = 0;

        for( i = 0; i < init_iters; i++ )
        {
            double max_dist = 0;
            cvSetSeqReaderPos( &reader, right_slice.start_index, 1 );
            CV_READ_SEQ_ELEM( start_pt, reader );   /* read the first point */

            for( j = 1; j < count; j++ )
            {
                double dx, dy, dist;

                CV_READ_SEQ_ELEM( pt, reader );
                dx = pt.x - start_pt.x;
                dy = pt.y - start_pt.y;

                dist = dx * dx + dy * dy;

                if( dist > max_dist )
                {
                    max_dist = dist;
                    right_slice.start_index = j;
                }
            }

            le_eps = max_dist <= eps;
        }

        /* 2. initialize the stack */
        if( !le_eps )
        {
            slice.start_index = cvGetSeqReaderPos( &reader );
            slice.end_index = right_slice.start_index += slice.start_index;

            right_slice.start_index -= right_slice.start_index >= count ? count : 0;
            right_slice.end_index = slice.start_index;
            if( right_slice.end_index < right_slice.start_index )
                right_slice.end_index += count;

            cvSeqPush( stack, &right_slice );
            cvSeqPush( stack, &slice );
        }
        else
            CV_WRITE_SEQ_ELEM( start_pt, writer );
    }

    /* 3. run recursive process */
    while( stack->total != 0 )
    {
        cvSeqPop( stack, &slice );

        if( slice.end_index > slice.start_index + 1 )
        {
            double dx, dy, dist, max_dist = 0;
            
            cvSetSeqReaderPos( &reader, slice.end_index );
            CV_READ_SEQ_ELEM( end_pt, reader );

            cvSetSeqReaderPos( &reader, slice.start_index );
            CV_READ_SEQ_ELEM( start_pt, reader );
            
            dx = end_pt.x - start_pt.x;
            dy = end_pt.y - start_pt.y;

            assert( dx != 0 || dy != 0 );

            for( i = slice.start_index + 1; i < slice.end_index; i++ )
            {
                CV_READ_SEQ_ELEM( pt, reader );
                dist = fabs((pt.y - start_pt.y) * dx - (pt.x - start_pt.x) * dy);

                if( dist > max_dist )
                {
                    max_dist = dist;
                    right_slice.start_index = i;
                }
            }

            le_eps = (double)max_dist * max_dist <= eps * ((double)dx * dx + (double)dy * dy);
        }
        else
        {
            assert( slice.end_index > slice.start_index );
            le_eps = 1;
            /* read starting point */
            cvSetSeqReaderPos( &reader, slice.start_index );
            CV_READ_SEQ_ELEM( start_pt, reader );
        }

        if( le_eps )
        {
            CV_WRITE_SEQ_ELEM( start_pt, writer );
        }
        else
        {
            right_slice.end_index = slice.end_index;
            slice.end_index = right_slice.start_index;
            cvSeqPush( stack, &right_slice );
            cvSeqPush( stack, &slice );
        }
    }

    is_closed = CV_IS_SEQ_CLOSED( src_contour );
    if( !is_closed )
        CV_WRITE_SEQ_ELEM( end_pt, writer );

    *dst_contour = cvEndWriteSeq( &writer );
    
    cvStartReadSeq( *dst_contour, &reader, is_closed );
    CV_READ_SEQ_ELEM( start_pt, reader );

    reader2 = reader;
    CV_READ_SEQ_ELEM( pt, reader );

    new_count = count = (*dst_contour)->total;
    for( i = !is_closed; i < count - !is_closed && new_count > 2; i++ )
    {
        double dx, dy, dist;
        CV_READ_SEQ_ELEM( end_pt, reader );

        dx = end_pt.x - start_pt.x;
        dy = end_pt.y - start_pt.y;
        dist = fabs((pt.x - start_pt.x)*dy - (pt.y - start_pt.y)*dx);
        if( (double)dist * dist <= 0.5*eps*((double)dx*dx + (double)dy*dy) )
        {
            new_count--;
            *((CvPoint2D32f*)reader2.ptr) = start_pt = end_pt;
            CV_NEXT_SEQ_ELEM( sizeof(pt), reader2 );
            CV_READ_SEQ_ELEM( pt, reader );
            i++;
            continue;
        }
        *((CvPoint2D32f*)reader2.ptr) = start_pt = pt;
        CV_NEXT_SEQ_ELEM( sizeof(pt), reader2 );
        pt = end_pt;
    }

    if( !is_closed )
        *((CvPoint2D32f*)reader2.ptr) = pt;

    if( new_count < count )
        cvSeqPopMulti( *dst_contour, 0, count - new_count );

    cvReleaseMemStorage( &temp_storage );

    return CV_OK;
}
// 
// 
CV_IMPL CvSeq*
cvApproxPoly( const void*  array, int  header_size,
              CvMemStorage*  storage, int  method, 
              double  parameter, int parameter2 )
{
    CvSeq* dst_seq = 0;
    CvSeq *prev_contour = 0, *parent = 0;
    CvContour contour_header;
    CvSeq* src_seq = 0;
    CvSeqBlock block;
    int recursive = 0;

    CV_FUNCNAME( "cvApproxPoly" );

    __BEGIN__;

    if( CV_IS_SEQ( array ))
    {
        src_seq = (CvSeq*)array;
        if( !CV_IS_SEQ_POLYLINE( src_seq ))
            CV_ERROR( CV_StsBadArg, "Unsupported sequence type" );

        recursive = parameter2;

        if( !storage )
            storage = src_seq->storage;
    }
    else
    {
        CV_CALL( src_seq = cvPointSeqFromMat(
            CV_SEQ_KIND_CURVE | (parameter2 ? CV_SEQ_FLAG_CLOSED : 0),
            array, &contour_header, &block ));
    }

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "NULL storage pointer " );

    if( header_size < 0 )
        CV_ERROR( CV_StsOutOfRange, "header_size is negative. "
                  "Pass 0 to make the destination header_size == input header_size" );

    if( header_size == 0 )
        header_size = src_seq->header_size;

    if( !CV_IS_SEQ_POLYLINE( src_seq ))
    {
        if( CV_IS_SEQ_CHAIN( src_seq ))
        {
            CV_ERROR( CV_StsBadArg, "Input curves are not polygonal. "
                                    "Use cvApproxChains first" );
        }
        else
        {
            CV_ERROR( CV_StsBadArg, "Input curves have uknown type" );
        }
    }

    if( header_size == 0 )
        header_size = src_seq->header_size;

    if( header_size < (int)sizeof(CvContour) )
        CV_ERROR( CV_StsBadSize, "New header size must be non-less than sizeof(CvContour)" );

    if( method != CV_POLY_APPROX_DP )
        CV_ERROR( CV_StsOutOfRange, "Unknown approximation method" );

    while( src_seq != 0 )
    {
        CvSeq *contour = 0;

        switch (method)
        {
        case CV_POLY_APPROX_DP:
            if( parameter < 0 )
                CV_ERROR( CV_StsOutOfRange, "Accuracy must be non-negative" );

            if( CV_SEQ_ELTYPE(src_seq) == CV_32SC2 )
            {
                IPPI_CALL( icvApproxPolyDP_32s( src_seq, header_size, storage,
                                                &contour, (float)parameter ));
            }
            else
            {
                IPPI_CALL( icvApproxPolyDP_32f( src_seq, header_size, storage,
                                                &contour, (float)parameter ));
            }
            break;
        default:
            assert(0);
            CV_ERROR( CV_StsBadArg, "Invalid approximation method" );
        }

        assert( contour );

        if( header_size >= (int)sizeof(CvContour))
            cvBoundingRect( contour, 1 );

        contour->v_prev = parent;
        contour->h_prev = prev_contour;

        if( prev_contour )
            prev_contour->h_next = contour;
        else if( parent )
            parent->v_next = contour;
        prev_contour = contour;
        if( !dst_seq )
            dst_seq = prev_contour;

        if( !recursive )
            break;

        if( src_seq->v_next )
        {
            assert( prev_contour != 0 );
            parent = prev_contour;
            prev_contour = 0;
            src_seq = src_seq->v_next;
        }
        else
        {
            while( src_seq->h_next == 0 )
            {
                src_seq = src_seq->v_prev;
                if( src_seq == 0 )
                    break;
                prev_contour = parent;
                if( parent )
                    parent = parent->v_prev;
            }
            if( src_seq )
                src_seq = src_seq->h_next;
        }
    }

    __END__;

    return dst_seq;
}

static void
icvGaussNewton( const CvMat* J, const CvMat* err, CvMat* delta,
                CvMat* JtJ=0, CvMat* JtErr=0, CvMat* JtJW=0, CvMat* JtJV=0 )
{
    CvMat* _temp_JtJ = 0;
    CvMat* _temp_JtErr = 0;
    CvMat* _temp_JtJW = 0;
    CvMat* _temp_JtJV = 0;
    
    CV_FUNCNAME( "icvGaussNewton" );

    __BEGIN__;

    if( !CV_IS_MAT(J) || !CV_IS_MAT(err) || !CV_IS_MAT(delta) )
        CV_ERROR( CV_StsBadArg, "Some of required arguments is not a valid matrix" );

    if( !JtJ )
    {
        CV_CALL( _temp_JtJ = cvCreateMat( J->cols, J->cols, J->type ));
        JtJ = _temp_JtJ;
    }
    else if( !CV_IS_MAT(JtJ) )
        CV_ERROR( CV_StsBadArg, "JtJ is not a valid matrix" );

    if( !JtErr )
    {
        CV_CALL( _temp_JtErr = cvCreateMat( J->cols, 1, J->type ));
        JtErr = _temp_JtErr;
    }
    else if( !CV_IS_MAT(JtErr) )
        CV_ERROR( CV_StsBadArg, "JtErr is not a valid matrix" );

    if( !JtJW )
    {
        CV_CALL( _temp_JtJW = cvCreateMat( J->cols, 1, J->type ));
        JtJW = _temp_JtJW;
    }
    else if( !CV_IS_MAT(JtJW) )
        CV_ERROR( CV_StsBadArg, "JtJW is not a valid matrix" );

    if( !JtJV )
    {
        CV_CALL( _temp_JtJV = cvCreateMat( J->cols, J->cols, J->type ));
        JtJV = _temp_JtJV;
    }
    else if( !CV_IS_MAT(JtJV) )
        CV_ERROR( CV_StsBadArg, "JtJV is not a valid matrix" );

    cvMulTransposed( J, JtJ, 1 );
    cvGEMM( J, err, 1, 0, 0, JtErr, CV_GEMM_A_T );
    cvSVD( JtJ, JtJW, 0, JtJV, CV_SVD_MODIFY_A + CV_SVD_V_T );
    cvSVBkSb( JtJW, JtJV, JtJV, JtErr, delta, CV_SVD_U_T + CV_SVD_V_T );

    __END__;

    if( _temp_JtJ || _temp_JtErr || _temp_JtJW || _temp_JtJV )
    {
        cvReleaseMat( &_temp_JtJ );
        cvReleaseMat( &_temp_JtErr );
        cvReleaseMat( &_temp_JtJW );
        cvReleaseMat( &_temp_JtJV );
    }
}

CV_IMPL void
cvFindHomography( const CvMat* object_points, const CvMat* image_points, CvMat* __H )
{
    CvMat *_m = 0, *_M = 0;
    CvMat *_L = 0;
    
    CV_FUNCNAME( "cvFindHomography" );

    __BEGIN__;

    int h_type;
    int i, k, count, count2;
    CvPoint2D64f *m, *M;
    CvPoint2D64f cm = {0,0}, sm = {0,0};
    double inv_Hnorm[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    double H[9];
    CvMat _inv_Hnorm = cvMat( 3, 3, CV_64FC1, inv_Hnorm );
    CvMat _H = cvMat( 3, 3, CV_64FC1, H );
    double LtL[9*9], LW[9], LV[9*9];
    CvMat* _Lp;
    double* L;
    CvMat _LtL = cvMat( 9, 9, CV_64FC1, LtL );
    CvMat _LW = cvMat( 9, 1, CV_64FC1, LW );
    CvMat _LV = cvMat( 9, 9, CV_64FC1, LV );
    CvMat _Hrem = cvMat( 3, 3, CV_64FC1, LV + 8*9 );

    if( !CV_IS_MAT(image_points) || !CV_IS_MAT(object_points) || !CV_IS_MAT(__H) )
        CV_ERROR( CV_StsBadArg, "one of arguments is not a valid matrix" );

    h_type = CV_MAT_TYPE(__H->type);
    if( h_type != CV_32FC1 && h_type != CV_64FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Homography matrix must have 32fC1 or 64fC1 type" );
    if( __H->rows != 3 || __H->cols != 3 )
        CV_ERROR( CV_StsBadSize, "Homography matrix must be 3x3" );

    count = MAX(image_points->cols, image_points->rows);
    count2 = MAX(object_points->cols, object_points->rows);
    if( count != count2 )
        CV_ERROR( CV_StsUnmatchedSizes, "Numbers of image and object points do not match" );

    CV_CALL( _m = cvCreateMat( 1, count, CV_64FC2 ));
    CV_CALL( cvConvertPointsHomogenious( image_points, _m ));
    m = (CvPoint2D64f*)_m->data.ptr;
    
    CV_CALL( _M = cvCreateMat( 1, count, CV_64FC2 ));
    CV_CALL( cvConvertPointsHomogenious( object_points, _M ));
    M = (CvPoint2D64f*)_M->data.ptr;

    // calculate the normalization transformation Hnorm.
    for( i = 0; i < count; i++ )
        cm.x += m[i].x, cm.y += m[i].y;
   
    cm.x /= count; cm.y /= count;

    for( i = 0; i < count; i++ )
    {
        double x = m[i].x - cm.x;
        double y = m[i].y - cm.y;
        sm.x += fabs(x); sm.y += fabs(y);
    }

    sm.x /= count; sm.y /= count;
    inv_Hnorm[0] = sm.x;
    inv_Hnorm[4] = sm.y;
    inv_Hnorm[2] = cm.x;
    inv_Hnorm[5] = cm.y;
    sm.x = 1./sm.x;
    sm.y = 1./sm.y;
    
    CV_CALL( _Lp = _L = cvCreateMat( 2*count, 9, CV_64FC1 ) );
    L = _L->data.db;

    for( i = 0; i < count; i++, L += 18 )
    {
        double x = -(m[i].x - cm.x)*sm.x, y = -(m[i].y - cm.y)*sm.y;
        L[0] = L[9 + 3] = M[i].x;
        L[1] = L[9 + 4] = M[i].y;
        L[2] = L[9 + 5] = 1;
        L[9 + 0] = L[9 + 1] = L[9 + 2] = L[3] = L[4] = L[5] = 0;
        L[6] = x*M[i].x;
        L[7] = x*M[i].y;
        L[8] = x;
        L[9 + 6] = y*M[i].x;
        L[9 + 7] = y*M[i].y;
        L[9 + 8] = y;
    }

    if( count > 4 )
    {
        cvMulTransposed( _L, &_LtL, 1 );
        _Lp = &_LtL;
    }

    _LW.rows = MIN(count*2, 9);
    cvSVD( _Lp, &_LW, 0, &_LV, CV_SVD_MODIFY_A + CV_SVD_V_T );
    cvScale( &_Hrem, &_Hrem, 1./_Hrem.data.db[8] );
    cvMatMul( &_inv_Hnorm, &_Hrem, &_H );

    if( count > 4 )
    {
        // reuse the available storage for jacobian and other vars
        CvMat _J = cvMat( 2*count, 8, CV_64FC1, _L->data.db );
        CvMat _err = cvMat( 2*count, 1, CV_64FC1, _L->data.db + 2*count*8 );
        CvMat _JtJ = cvMat( 8, 8, CV_64FC1, LtL );
        CvMat _JtErr = cvMat( 8, 1, CV_64FC1, LtL + 8*8 );
        CvMat _JtJW = cvMat( 8, 1, CV_64FC1, LW );
        CvMat _JtJV = cvMat( 8, 8, CV_64FC1, LV );
        CvMat _Hinnov = cvMat( 8, 1, CV_64FC1, LV + 8*8 );

        for( k = 0; k < 10; k++ )
        {
            double* J = _J.data.db, *err = _err.data.db;
            for( i = 0; i < count; i++, J += 16, err += 2 )
            {
                double di = 1./(H[6]*M[i].x + H[7]*M[i].y + 1.);
                double _xi = (H[0]*M[i].x + H[1]*M[i].y + H[2])*di;
                double _yi = (H[3]*M[i].x + H[4]*M[i].y + H[5])*di;
                err[0] = m[i].x - _xi;
                err[1] = m[i].y - _yi;
                J[0] = M[i].x*di;
                J[1] = M[i].y*di;
                J[2] = di;
                J[8+3] = M[i].x;
                J[8+4] = M[i].y;
                J[8+5] = di;
                J[6] = -J[0]*_xi;
                J[7] = -J[1]*_xi;
                J[8+6] = -J[8+3]*_yi;
                J[8+7] = -J[8+4]*_yi;
                J[3] = J[4] = J[5] = J[8+0] = J[8+1] = J[8+2] = 0.;
            }

            icvGaussNewton( &_J, &_err, &_Hinnov, &_JtJ, &_JtErr, &_JtJW, &_JtJV );

            for( i = 0; i < 8; i++ )
                H[i] += _Hinnov.data.db[i];
        }
    }

    cvConvert( &_H, __H );

    __END__;

    cvReleaseMat( &_m );
    cvReleaseMat( &_M );
    cvReleaseMat( &_L );
}

CV_IMPL int
cvRodrigues2( const CvMat* src, CvMat* dst, CvMat* jacobian )
{
    int result = 0;
    
    CV_FUNCNAME( "cvRogrigues2" );

    __BEGIN__;

    int depth, elem_size;
    int i, k;
    double J[27];
    CvMat _J = cvMat( 3, 9, CV_64F, J );

    if( !CV_IS_MAT(src) )
        CV_ERROR( !src ? CV_StsNullPtr : CV_StsBadArg, "Input argument is not a valid matrix" );

    if( !CV_IS_MAT(dst) )
        CV_ERROR( !dst ? CV_StsNullPtr : CV_StsBadArg,
        "The first output argument is not a valid matrix" );

    depth = CV_MAT_DEPTH(src->type);
    elem_size = CV_ELEM_SIZE(depth);

    if( depth != CV_32F && depth != CV_64F )
        CV_ERROR( CV_StsUnsupportedFormat, "The matrices must have 32f or 64f data type" );

    if( !CV_ARE_DEPTHS_EQ(src, dst) )
        CV_ERROR( CV_StsUnmatchedFormats, "All the matrices must have the same data type" );

    if( jacobian )
    {
        if( !CV_IS_MAT(jacobian) )
            CV_ERROR( CV_StsBadArg, "Jacobian is not a valid matrix" );

        if( !CV_ARE_DEPTHS_EQ(src, jacobian) || CV_MAT_CN(jacobian->type) != 1 )
            CV_ERROR( CV_StsUnmatchedFormats, "Jacobian must have 32fC1 or 64fC1 datatype" );

        if( (jacobian->rows != 9 || jacobian->cols != 3) &&
            (jacobian->rows != 3 || jacobian->cols != 9))
            CV_ERROR( CV_StsBadSize, "Jacobian must be 3x9 or 9x3" );
    }

    if( src->cols == 1 || src->rows == 1 )
    {
        double rx, ry, rz, theta;
        int step = src->rows > 1 ? src->step / elem_size : 1;

        if( src->rows + src->cols*CV_MAT_CN(src->type) - 1 != 3 )
            CV_ERROR( CV_StsBadSize, "Input matrix must be 1x3, 3x1 or 3x3" );

        if( dst->rows != 3 || dst->cols != 3 || CV_MAT_CN(dst->type) != 1 )
            CV_ERROR( CV_StsBadSize, "Output matrix must be 3x3, single-channel floating point matrix" );

        if( depth == CV_32F )
        {
            rx = src->data.fl[0];
            ry = src->data.fl[step];
            rz = src->data.fl[step*2];
        }
        else
        {
            rx = src->data.db[0];
            ry = src->data.db[step];
            rz = src->data.db[step*2];
        }
        theta = sqrt(rx*rx + ry*ry + rz*rz);

        if( theta < DBL_EPSILON )
        {
            cvSetIdentity( dst );

            if( jacobian )
            {
                memset( J, 0, sizeof(J) );
                J[5] = J[15] = J[19] = -1;
                J[7] = J[11] = J[21] = 1;
            }
        }
        else
        {
            const double I[] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
            
            double c = cos(theta);
            double s = sin(theta);
            double c1 = 1. - c;
            double itheta = theta ? 1./theta : 0.;

            rx *= itheta; ry *= itheta; rz *= itheta;

            double rrt[] = { rx*rx, rx*ry, rx*rz, rx*ry, ry*ry, ry*rz, rx*rz, ry*rz, rz*rz };
            double _r_x_[] = { 0, -rz, ry, rz, 0, -rx, -ry, rx, 0 };
            double R[9];
            CvMat _R = cvMat( 3, 3, CV_64F, R );

            // R = cos(theta)*I + (1 - cos(theta))*r*rT + sin(theta)*[r_x]
            // where [r_x] is [0 -rz ry; rz 0 -rx; -ry rx 0]
            for( k = 0; k < 9; k++ )
                R[k] = c*I[k] + c1*rrt[k] + s*_r_x_[k];

            cvConvert( &_R, dst );
            
            if( jacobian )
            {
                double drrt[] = { rx+rx, ry, rz, ry, 0, 0, rz, 0, 0,
                                  0, rx, 0, rx, ry+ry, rz, 0, rz, 0,
                                  0, 0, rx, 0, 0, ry, rx, ry, rz+rz };
                double d_r_x_[] = { 0, 0, 0, 0, 0, -1, 0, 1, 0,
                                    0, 0, 1, 0, 0, 0, -1, 0, 0,
                                    0, -1, 0, 1, 0, 0, 0, 0, 0 };
                for( i = 0; i < 3; i++ )
                {
                    double ri = i == 0 ? rx : i == 1 ? ry : rz;
                    double a0 = -s*ri, a1 = (s - 2*c1*itheta)*ri, a2 = c1*itheta;
                    double a3 = (c - s*itheta)*ri, a4 = s*itheta;
                    for( k = 0; k < 9; k++ )
                        J[i*9+k] = a0*I[k] + a1*rrt[k] + a2*drrt[i*9+k] +
                                   a3*_r_x_[k] + a4*d_r_x_[i*9+k];
                }
            }
        }
    }
    else if( src->cols == 3 && src->rows == 3 )
    {
        double R[9], U[9], V[9], W[3], rx, ry, rz;
        CvMat _R = cvMat( 3, 3, CV_64F, R );
        CvMat _U = cvMat( 3, 3, CV_64F, U );
        CvMat _V = cvMat( 3, 3, CV_64F, V );
        CvMat _W = cvMat( 3, 1, CV_64F, W );
        double theta, s, c;
        int step = dst->rows > 1 ? dst->step / elem_size : 1;
        
        if( (dst->rows != 1 || dst->cols*CV_MAT_CN(dst->type) != 3) &&
            (dst->rows != 3 || dst->cols != 1 || CV_MAT_CN(dst->type) != 1))
            CV_ERROR( CV_StsBadSize, "Output matrix must be 1x3 or 3x1" );

        cvConvert( src, &_R );
        if( !cvCheckArr( &_R, CV_CHECK_RANGE+CV_CHECK_QUIET, -100, 100 ) )
        {
            cvZero(dst);
            if( jacobian )
                cvZero(jacobian);
            EXIT;
        }
        
        cvSVD( &_R, &_W, &_U, &_V, CV_SVD_MODIFY_A + CV_SVD_U_T + CV_SVD_V_T );
        cvGEMM( &_U, &_V, 1, 0, 0, &_R, CV_GEMM_A_T );
        
        rx = R[7] - R[5];
        ry = R[2] - R[6];
        rz = R[3] - R[1];

        s = sqrt((rx*rx + ry*ry + rz*rz)*0.25);
        c = (R[0] + R[4] + R[8] - 1)*0.5;
        c = c > 1. ? 1. : c < -1. ? -1. : c;
        theta = acos(c);

        if( s < 1e-5 )
        {
            double t;

            if( c > 0 )
                rx = ry = rz = 0;
            else
            {
                t = (R[0] + 1)*0.5;
                rx = theta*sqrt(MAX(t,0.));
                t = (R[4] + 1)*0.5;
                ry = theta*sqrt(MAX(t,0.))*(R[1] < 0 ? -1. : 1.);
                t = (R[8] + 1)*0.5;
                rz = theta*sqrt(MAX(t,0.))*(R[2] < 0 ? -1. : 1.);
            }

            if( jacobian )
            {
                memset( J, 0, sizeof(J) );
                if( c > 0 )
                {
                    J[5] = J[15] = J[19] = -0.5;
                    J[7] = J[11] = J[21] = 0.5;
                }
            }
        }
        else
        {
            double vth = 1/(2*s);
            
            if( jacobian )
            {
                double t, dtheta_dtr = -1./s;
                // var1 = [vth;theta]
                // var = [om1;var1] = [om1;vth;theta]
                double dvth_dtheta = -vth*c/s;
                double d1 = 0.5*dvth_dtheta*dtheta_dtr;
                double d2 = 0.5*dtheta_dtr;
                // dvar1/dR = dvar1/dtheta*dtheta/dR = [dvth/dtheta; 1] * dtheta/dtr * dtr/dR
                double dvardR[5*9] =
                {
                    0, 0, 0, 0, 0, 1, 0, -1, 0,
                    0, 0, -1, 0, 0, 0, 1, 0, 0,
                    0, 1, 0, -1, 0, 0, 0, 0, 0,
                    d1, 0, 0, 0, d1, 0, 0, 0, d1,
                    d2, 0, 0, 0, d2, 0, 0, 0, d2
                };
                // var2 = [om;theta]
                double dvar2dvar[] =
                {
                    vth, 0, 0, rx, 0,
                    0, vth, 0, ry, 0,
                    0, 0, vth, rz, 0,
                    0, 0, 0, 0, 1
                };
                double domegadvar2[] = 
                {
                    theta, 0, 0, rx*vth,
                    0, theta, 0, ry*vth,
                    0, 0, theta, rz*vth
                };
        
                CvMat _dvardR = cvMat( 5, 9, CV_64FC1, dvardR );
                CvMat _dvar2dvar = cvMat( 4, 5, CV_64FC1, dvar2dvar );
                CvMat _domegadvar2 = cvMat( 3, 4, CV_64FC1, domegadvar2 );
                double t0[3*5];
                CvMat _t0 = cvMat( 3, 5, CV_64FC1, t0 );
        
                cvMatMul( &_domegadvar2, &_dvar2dvar, &_t0 );
                cvMatMul( &_t0, &_dvardR, &_J );

                // transpose every row of _J (treat the rows as 3x3 matrices)
                CV_SWAP(J[1], J[3], t); CV_SWAP(J[2], J[6], t); CV_SWAP(J[5], J[7], t);
                CV_SWAP(J[10], J[12], t); CV_SWAP(J[11], J[15], t); CV_SWAP(J[14], J[16], t);
                CV_SWAP(J[19], J[21], t); CV_SWAP(J[20], J[24], t); CV_SWAP(J[23], J[25], t);
            }

            vth *= theta;
            rx *= vth; ry *= vth; rz *= vth;
        }

        if( depth == CV_32F )
        {
            dst->data.fl[0] = (float)rx;
            dst->data.fl[step] = (float)ry;
            dst->data.fl[step*2] = (float)rz;
        }
        else
        {
            dst->data.db[0] = rx;
            dst->data.db[step] = ry;
            dst->data.db[step*2] = rz;
        }
    }

    if( jacobian )
    {
        if( depth == CV_32F )
        {
            if( jacobian->rows == _J.rows )
                cvConvert( &_J, jacobian );
            else
            {
                float Jf[3*9];
                CvMat _Jf = cvMat( _J.rows, _J.cols, CV_32FC1, Jf );
                cvConvert( &_J, &_Jf );
                cvTranspose( &_Jf, jacobian );
            }
        }
        else if( jacobian->rows == _J.rows )
            cvCopy( &_J, jacobian );
        else
            cvTranspose( &_J, jacobian );
    }

    result = 1;

    __END__;

    return result;
}

// 
CV_IMPL void
cvProjectPoints2( const CvMat* obj_points,
                  const CvMat* r_vec,
                  const CvMat* t_vec,
                  const CvMat* A,
                  const CvMat* dist_coeffs,
                  CvMat* img_points, CvMat* dpdr,
                  CvMat* dpdt, CvMat* dpdf,
                  CvMat* dpdc, CvMat* dpdk )
{
    CvMat *_M = 0, *_m = 0;
    CvMat *_dpdr = 0, *_dpdt = 0, *_dpdc = 0, *_dpdf = 0, *_dpdk = 0;
    
    CV_FUNCNAME( "cvProjectPoints2" );

    __BEGIN__;

    int i, j, count;
    int calc_derivatives;
    const CvPoint3D64f* M;
    CvPoint2D64f* m;
    double r[3], R[9], dRdr[27], t[3], a[9], k[4] = {0,0,0,0}, fx, fy, cx, cy;
    CvMat _r, _t, _a = cvMat( 3, 3, CV_64F, a ), _k;
    CvMat _R = cvMat( 3, 3, CV_64F, R ), _dRdr = cvMat( 3, 9, CV_64F, dRdr );
    double *dpdr_p = 0, *dpdt_p = 0, *dpdk_p = 0, *dpdf_p = 0, *dpdc_p = 0;
    int dpdr_step = 0, dpdt_step = 0, dpdk_step = 0, dpdf_step = 0, dpdc_step = 0;

    if( !CV_IS_MAT(obj_points) || !CV_IS_MAT(r_vec) ||
        !CV_IS_MAT(t_vec) || !CV_IS_MAT(A) ||
        /*!CV_IS_MAT(dist_coeffs) ||*/ !CV_IS_MAT(img_points) )
        CV_ERROR( CV_StsBadArg, "One of required arguments is not a valid matrix" );

    count = MAX(obj_points->rows, obj_points->cols);

    if( CV_IS_CONT_MAT(obj_points->type) && CV_MAT_DEPTH(obj_points->type) == CV_64F &&
        (obj_points->rows == 1 && CV_MAT_CN(obj_points->type) == 3 ||
        obj_points->rows == count && CV_MAT_CN(obj_points->type)*obj_points->cols == 3))
        _M = (CvMat*)obj_points;
    else
    {
        CV_CALL( _M = cvCreateMat( 1, count, CV_64FC3 ));
        CV_CALL( cvConvertPointsHomogenious( obj_points, _M ));
    }

    if( CV_IS_CONT_MAT(img_points->type) && CV_MAT_DEPTH(img_points->type) == CV_64F &&
        (img_points->rows == 1 && CV_MAT_CN(img_points->type) == 2 ||
        img_points->rows == count && CV_MAT_CN(img_points->type)*img_points->cols == 2))
        _m = img_points;
    else
        CV_CALL( _m = cvCreateMat( 1, count, CV_64FC2 ));

    M = (CvPoint3D64f*)_M->data.db;
    m = (CvPoint2D64f*)_m->data.db;

    if( CV_MAT_DEPTH(r_vec->type) != CV_64F && CV_MAT_DEPTH(r_vec->type) != CV_32F ||
        (r_vec->rows != 1 && r_vec->cols != 1 ||
        r_vec->rows*r_vec->cols*CV_MAT_CN(r_vec->type) != 3) &&
        (r_vec->rows != 3 && r_vec->cols != 3 || CV_MAT_CN(r_vec->type) != 1))
        CV_ERROR( CV_StsBadArg, "Rotation must be represented by 1x3 or 3x1 "
                  "floating-point rotation vector, or 3x3 rotation matrix" );

    if( r_vec->rows == 3 && r_vec->cols == 3 )
    {
        _r = cvMat( 3, 1, CV_64FC1, r );
        CV_CALL( cvRodrigues2( r_vec, &_r ));
        CV_CALL( cvRodrigues2( &_r, &_R, &_dRdr ));
        cvCopy( r_vec, &_R );
    }
    else
    {
        _r = cvMat( r_vec->rows, r_vec->cols, CV_MAKETYPE(CV_64F,CV_MAT_CN(r_vec->type)), r );
        CV_CALL( cvConvert( r_vec, &_r ));
        CV_CALL( cvRodrigues2( &_r, &_R, &_dRdr ) );
    }

    if( CV_MAT_DEPTH(t_vec->type) != CV_64F && CV_MAT_DEPTH(t_vec->type) != CV_32F ||
        t_vec->rows != 1 && t_vec->cols != 1 ||
        t_vec->rows*t_vec->cols*CV_MAT_CN(t_vec->type) != 3 )
        CV_ERROR( CV_StsBadArg,
            "Translation vector must be 1x3 or 3x1 floating-point vector" );

    _t = cvMat( t_vec->rows, t_vec->cols, CV_MAKETYPE(CV_64F,CV_MAT_CN(t_vec->type)), t );
    CV_CALL( cvConvert( t_vec, &_t ));

    if( CV_MAT_TYPE(A->type) != CV_64FC1 && CV_MAT_TYPE(A->type) != CV_32FC1 ||
        A->rows != 3 || A->cols != 3 )
        CV_ERROR( CV_StsBadArg, "Instrinsic parameters must be 3x3 floating-point matrix" );

    CV_CALL( cvConvert( A, &_a ));
    fx = a[0]; fy = a[4];
    cx = a[2]; cy = a[5];

    if( dist_coeffs )
    {
        if( !CV_IS_MAT(dist_coeffs) ||
            CV_MAT_DEPTH(dist_coeffs->type) != CV_64F &&
            CV_MAT_DEPTH(dist_coeffs->type) != CV_32F ||
            dist_coeffs->rows != 1 && dist_coeffs->cols != 1 ||
            dist_coeffs->rows*dist_coeffs->cols*CV_MAT_CN(dist_coeffs->type) != 4 )
            CV_ERROR( CV_StsBadArg,
                "Distortion coefficients must be 1x4 or 4x1 floating-point vector" );

        _k = cvMat( dist_coeffs->rows, dist_coeffs->cols,
                    CV_MAKETYPE(CV_64F,CV_MAT_CN(dist_coeffs->type)), k );
        CV_CALL( cvConvert( dist_coeffs, &_k ));
    }
    
    if( dpdr )
    {
        if( !CV_IS_MAT(dpdr) ||
            CV_MAT_TYPE(dpdr->type) != CV_32FC1 &&
            CV_MAT_TYPE(dpdr->type) != CV_64FC1 ||
            dpdr->rows != count*2 || dpdr->cols != 3 )
            CV_ERROR( CV_StsBadArg, "dp/drot must be 2Nx3 floating-point matrix" );

        if( CV_MAT_TYPE(dpdr->type) == CV_64FC1 )
            _dpdr = dpdr;
        else
            CV_CALL( _dpdr = cvCreateMat( 2*count, 3, CV_64FC1 ));
        dpdr_p = _dpdr->data.db;
        dpdr_step = _dpdr->step/sizeof(dpdr_p[0]);
    }

    if( dpdt )
    {
        if( !CV_IS_MAT(dpdt) ||
            CV_MAT_TYPE(dpdt->type) != CV_32FC1 &&
            CV_MAT_TYPE(dpdt->type) != CV_64FC1 ||
            dpdt->rows != count*2 || dpdt->cols != 3 )
            CV_ERROR( CV_StsBadArg, "dp/dT must be 2Nx3 floating-point matrix" );

        if( CV_MAT_TYPE(dpdt->type) == CV_64FC1 )
            _dpdt = dpdt;
        else
            CV_CALL( _dpdt = cvCreateMat( 2*count, 3, CV_64FC1 ));
        dpdt_p = _dpdt->data.db;
        dpdt_step = _dpdt->step/sizeof(dpdt_p[0]);
    }

    if( dpdf )
    {
        if( !CV_IS_MAT(dpdf) ||
            CV_MAT_TYPE(dpdf->type) != CV_32FC1 && CV_MAT_TYPE(dpdf->type) != CV_64FC1 ||
            dpdf->rows != count*2 || dpdf->cols != 2 )
            CV_ERROR( CV_StsBadArg, "dp/df must be 2Nx2 floating-point matrix" );

        if( CV_MAT_TYPE(dpdf->type) == CV_64FC1 )
            _dpdf = dpdf;
        else
            CV_CALL( _dpdf = cvCreateMat( 2*count, 2, CV_64FC1 ));
        dpdf_p = _dpdf->data.db;
        dpdf_step = _dpdf->step/sizeof(dpdf_p[0]);
    }

    if( dpdc )
    {
        if( !CV_IS_MAT(dpdc) ||
            CV_MAT_TYPE(dpdc->type) != CV_32FC1 && CV_MAT_TYPE(dpdc->type) != CV_64FC1 ||
            dpdc->rows != count*2 || dpdc->cols != 2 )
            CV_ERROR( CV_StsBadArg, "dp/dc must be 2Nx2 floating-point matrix" );

        if( CV_MAT_TYPE(dpdc->type) == CV_64FC1 )
            _dpdc = dpdc;
        else
            CV_CALL( _dpdc = cvCreateMat( 2*count, 2, CV_64FC1 ));
        dpdc_p = _dpdc->data.db;
        dpdc_step = _dpdc->step/sizeof(dpdc_p[0]);
    }

    if( dpdk )
    {
        if( !CV_IS_MAT(dpdk) ||
            CV_MAT_TYPE(dpdk->type) != CV_32FC1 && CV_MAT_TYPE(dpdk->type) != CV_64FC1 ||
            dpdk->rows != count*2 || (dpdk->cols != 4 && dpdk->cols != 2) )
            CV_ERROR( CV_StsBadArg, "dp/df must be 2Nx4 or 2Nx2 floating-point matrix" );

        if( !dist_coeffs )
            CV_ERROR( CV_StsNullPtr, "dist_coeffs is NULL while dpdk is not" );

        if( CV_MAT_TYPE(dpdk->type) == CV_64FC1 )
            _dpdk = dpdk;
        else
            CV_CALL( _dpdk = cvCreateMat( dpdk->rows, dpdk->cols, CV_64FC1 ));
        dpdk_p = _dpdk->data.db;
        dpdk_step = _dpdk->step/sizeof(dpdk_p[0]);
    }

    calc_derivatives = dpdr || dpdt || dpdf || dpdc || dpdk;

    for( i = 0; i < count; i++ )
    {
        double X = M[i].x, Y = M[i].y, Z = M[i].z;
        double x = R[0]*X + R[1]*Y + R[2]*Z + t[0];
        double y = R[3]*X + R[4]*Y + R[5]*Z + t[1];
        double z = R[6]*X + R[7]*Y + R[8]*Z + t[2];
        double r2, r4, a1, a2, a3, cdist;
        double xd, yd;

        z = z ? 1./z : 1;
        x *= z; y *= z;

        r2 = x*x + y*y;
        r4 = r2*r2;
        a1 = 2*x*y;
        a2 = r2 + 2*x*x;
        a3 = r2 + 2*y*y;
        cdist = 1 + k[0]*r2 + k[1]*r4;
        xd = x*cdist + k[2]*a1 + k[3]*a2;
        yd = y*cdist + k[2]*a3 + k[3]*a1;

        m[i].x = xd*fx + cx;
        m[i].y = yd*fy + cy;

        if( calc_derivatives )
        {
            if( dpdc_p )
            {
                dpdc_p[0] = 1; dpdc_p[1] = 0;
                dpdc_p[dpdc_step] = 0;
                dpdc_p[dpdc_step+1] = 1;
                dpdc_p += dpdc_step*2;
            }

            if( dpdf_p )
            {
                dpdf_p[0] = xd; dpdf_p[1] = 0;
                dpdf_p[dpdf_step] = 0;
                dpdf_p[dpdf_step+1] = yd;
                dpdf_p += dpdf_step*2;
            }

            if( dpdk_p )
            {
                dpdk_p[0] = fx*x*r2;
                dpdk_p[1] = fx*x*r4;
                dpdk_p[dpdk_step] = fy*y*r2;
                dpdk_p[dpdk_step+1] = fy*y*r4;
                if( _dpdk->cols > 2 )
                {
                    dpdk_p[2] = fx*a1;
                    dpdk_p[3] = fx*a2;
                    dpdk_p[dpdk_step+2] = fy*a3;
                    dpdk_p[dpdk_step+3] = fy*a1;
                }
                dpdk_p += dpdk_step*2;
            }

            if( dpdt_p )
            {
                double dxdt[] = { z, 0, -x*z }, dydt[] = { 0, z, -y*z };
                for( j = 0; j < 3; j++ )
                {
                    double dr2dt = 2*x*dxdt[j] + 2*y*dydt[j];
                    double dcdist_dt = k[0]*dr2dt + 2*k[1]*r2*dr2dt;
                    double da1dt = 2*(x*dydt[j] + y*dxdt[j]);
                    double dmxdt = fx*(dxdt[j]*cdist + x*dcdist_dt +
                                k[2]*da1dt + k[3]*(dr2dt + 2*x*dxdt[j]));
                    double dmydt = fy*(dydt[j]*cdist + y*dcdist_dt +
                                k[2]*(dr2dt + 2*y*dydt[j]) + k[3]*da1dt);
                    dpdt_p[j] = dmxdt;
                    dpdt_p[dpdt_step+j] = dmydt;
                }
                dpdt_p += dpdt_step*2;
            }

            if( dpdr_p )
            {
                double dx0dr[] =
                {
                    X*dRdr[0] + Y*dRdr[1] + Z*dRdr[2],
                    X*dRdr[9] + Y*dRdr[10] + Z*dRdr[11],
                    X*dRdr[18] + Y*dRdr[19] + Z*dRdr[20]
                };
                double dy0dr[] =
                {
                    X*dRdr[3] + Y*dRdr[4] + Z*dRdr[5],
                    X*dRdr[12] + Y*dRdr[13] + Z*dRdr[14],
                    X*dRdr[21] + Y*dRdr[22] + Z*dRdr[23]
                };
                double dz0dr[] =
                {
                    X*dRdr[6] + Y*dRdr[7] + Z*dRdr[8],
                    X*dRdr[15] + Y*dRdr[16] + Z*dRdr[17],
                    X*dRdr[24] + Y*dRdr[25] + Z*dRdr[26]
                };
                for( j = 0; j < 3; j++ )
                {
                    double dxdr = z*(dx0dr[j] - x*dz0dr[j]);
                    double dydr = z*(dy0dr[j] - y*dz0dr[j]);
                    double dr2dr = 2*x*dxdr + 2*y*dydr;
                    double dcdist_dr = k[0]*dr2dr + 2*k[1]*r2*dr2dr;
                    double da1dr = 2*(x*dydr + y*dxdr);
                    double dmxdr = fx*(dxdr*cdist + x*dcdist_dr +
                                k[2]*da1dr + k[3]*(dr2dr + 2*x*dxdr));
                    double dmydr = fy*(dydr*cdist + y*dcdist_dr +
                                k[2]*(dr2dr + 2*y*dydr) + k[3]*da1dr);
                    dpdr_p[j] = dmxdr;
                    dpdr_p[dpdr_step+j] = dmydr;
                }
                dpdr_p += dpdr_step*2;
            }
        }
    }

    if( _m != img_points )
        cvConvertPointsHomogenious( _m, img_points );
    if( _dpdr != dpdr )
        cvConvert( _dpdr, dpdr );
    if( _dpdt != dpdt )
        cvConvert( _dpdt, dpdt );
    if( _dpdf != dpdf )
        cvConvert( _dpdf, dpdf );
    if( _dpdc != dpdc )
        cvConvert( _dpdc, dpdc );
    if( _dpdk != dpdk )
        cvConvert( _dpdk, dpdk );

    __END__;

    if( _M != obj_points )
        cvReleaseMat( &_M );
    if( _m != img_points )
        cvReleaseMat( &_m );
    if( _dpdr != dpdr )
        cvReleaseMat( &_dpdr );
    if( _dpdt != dpdt )
        cvReleaseMat( &_dpdt );
    if( _dpdf != dpdf )
        cvReleaseMat( &_dpdf );
    if( _dpdc != dpdc )
        cvReleaseMat( &_dpdc );
    if( _dpdk != dpdk )
        cvReleaseMat( &_dpdk );
}

// 
CV_IMPL void
cvFindExtrinsicCameraParams2( const CvMat* obj_points,
                  const CvMat* img_points, const CvMat* A,
                  const CvMat* dist_coeffs,
                  CvMat* r_vec, CvMat* t_vec )
{
    const int max_iter = 20;
    CvMat *_M = 0, *_Mxy = 0, *_m = 0, *_mn = 0, *_L = 0, *_J = 0;
    
    CV_FUNCNAME( "cvFindExtrinsicCameraParams2" );

    __BEGIN__;

    int i, j, count;
    double a[9], k[4] = { 0, 0, 0, 0 }, R[9], ifx, ify, cx, cy;
    double Mc[3] = {0, 0, 0}, MM[9], U[9], V[9], W[3];
    double JtJ[6*6], JtErr[6], JtJW[6], JtJV[6*6], delta[6], param[6];
    CvPoint3D64f* M = 0;
    CvPoint2D64f *m = 0, *mn = 0;
    CvMat _a = cvMat( 3, 3, CV_64F, a );
    CvMat _R = cvMat( 3, 3, CV_64F, R );
    CvMat _r = cvMat( 3, 1, CV_64F, param );
    CvMat _t = cvMat( 3, 1, CV_64F, param + 3 );
    CvMat _Mc = cvMat( 1, 3, CV_64F, Mc );
    CvMat _MM = cvMat( 3, 3, CV_64F, MM );
    CvMat _U = cvMat( 3, 3, CV_64F, U );
    CvMat _V = cvMat( 3, 3, CV_64F, V );
    CvMat _W = cvMat( 3, 1, CV_64F, W );
    CvMat _JtJ = cvMat( 6, 6, CV_64F, JtJ );
    CvMat _JtErr = cvMat( 6, 1, CV_64F, JtErr );
    CvMat _JtJW = cvMat( 6, 1, CV_64F, JtJW );
    CvMat _JtJV = cvMat( 6, 6, CV_64F, JtJV );
    CvMat _delta = cvMat( 6, 1, CV_64F, delta );
    CvMat _param = cvMat( 6, 1, CV_64F, param );
    CvMat _dpdr, _dpdt;

    if( !CV_IS_MAT(obj_points) || !CV_IS_MAT(img_points) ||
        !CV_IS_MAT(A) || !CV_IS_MAT(r_vec) || !CV_IS_MAT(t_vec) )
        CV_ERROR( CV_StsBadArg, "One of required arguments is not a valid matrix" );

    count = MAX(obj_points->cols, obj_points->rows);
    CV_CALL( _M = cvCreateMat( 1, count, CV_64FC3 ));
    CV_CALL( _Mxy = cvCreateMat( 1, count, CV_64FC2 ));
    CV_CALL( _m = cvCreateMat( 1, count, CV_64FC2 ));
    CV_CALL( _mn = cvCreateMat( 1, count, CV_64FC2 ));
    M = (CvPoint3D64f*)_M->data.db;
    m = (CvPoint2D64f*)_m->data.db;
    mn = (CvPoint2D64f*)_mn->data.db;

    CV_CALL( cvConvertPointsHomogenious( obj_points, _M ));
    CV_CALL( cvConvertPointsHomogenious( img_points, _m ));
    CV_CALL( cvConvert( A, &_a ));

    if( dist_coeffs )
    {
        CvMat _k;
        if( !CV_IS_MAT(dist_coeffs) ||
            CV_MAT_DEPTH(dist_coeffs->type) != CV_64F &&
            CV_MAT_DEPTH(dist_coeffs->type) != CV_32F ||
            dist_coeffs->rows != 1 && dist_coeffs->cols != 1 ||
            dist_coeffs->rows*dist_coeffs->cols*CV_MAT_CN(dist_coeffs->type) != 4 )
            CV_ERROR( CV_StsBadArg,
                "Distortion coefficients must be 1x4 or 4x1 floating-point vector" );

        _k = cvMat( dist_coeffs->rows, dist_coeffs->cols,
                    CV_MAKETYPE(CV_64F,CV_MAT_CN(dist_coeffs->type)), k );
        CV_CALL( cvConvert( dist_coeffs, &_k ));
    }

    if( CV_MAT_DEPTH(r_vec->type) != CV_64F && CV_MAT_DEPTH(r_vec->type) != CV_32F ||
        r_vec->rows != 1 && r_vec->cols != 1 ||
        r_vec->rows*r_vec->cols*CV_MAT_CN(r_vec->type) != 3 )
        CV_ERROR( CV_StsBadArg, "Rotation vector must be 1x3 or 3x1 floating-point vector" );

    if( CV_MAT_DEPTH(t_vec->type) != CV_64F && CV_MAT_DEPTH(t_vec->type) != CV_32F ||
        t_vec->rows != 1 && t_vec->cols != 1 ||
        t_vec->rows*t_vec->cols*CV_MAT_CN(t_vec->type) != 3 )
        CV_ERROR( CV_StsBadArg,
            "Translation vector must be 1x3 or 3x1 floating-point vector" );

    ifx = 1./a[0]; ify = 1./a[4];
    cx = a[2]; cy = a[5];

    // normalize image points
    // (unapply the intrinsic matrix transformation and distortion)
    for( i = 0; i < count; i++ )
    {
        double x = (m[i].x - cx)*ifx, y = (m[i].y - cy)*ify, x0 = x, y0 = y;

        // compensate distortion iteratively
        if( dist_coeffs )
            for( j = 0; j < 5; j++ )
            {
                double r2 = x*x + y*y;
                double icdist = 1./(1 + k[0]*r2 + k[1]*r2*r2);
                double delta_x = 2*k[2]*x*y + k[3]*(r2 + 2*x*x);
                double delta_y = k[2]*(r2 + 2*y*y) + 2*k[3]*x*y;
                x = (x0 - delta_x)*icdist;
                y = (y0 - delta_y)*icdist;
            }
        mn[i].x = x; mn[i].y = y;

        // calc mean(M)
        Mc[0] += M[i].x;
        Mc[1] += M[i].y;
        Mc[2] += M[i].z;
    }

    Mc[0] /= count;
    Mc[1] /= count;
    Mc[2] /= count;

    cvReshape( _M, _M, 1, count );
    cvMulTransposed( _M, &_MM, 1, &_Mc );
    cvSVD( &_MM, &_W, 0, &_V, CV_SVD_MODIFY_A + CV_SVD_V_T );

    // initialize extrinsic parameters
    if( W[2]/W[1] < 1e-3 || count < 4 )
    {
        // a planar structure case (all M's lie in the same plane)
        double tt[3], h[9], h1_norm, h2_norm;
        CvMat* R_transform = &_V;
        CvMat T_transform = cvMat( 3, 1, CV_64F, tt );
        CvMat _H = cvMat( 3, 3, CV_64F, h );
        CvMat _h1, _h2, _h3;

        if( V[2]*V[2] + V[5]*V[5] < 1e-10 )
            cvSetIdentity( R_transform );

        if( cvDet(R_transform) < 0 )
            cvScale( R_transform, R_transform, -1 );

        cvGEMM( R_transform, &_Mc, -1, 0, 0, &T_transform, CV_GEMM_B_T );

        for( i = 0; i < count; i++ )
        {
            const double* Rp = R_transform->data.db;
            const double* Tp = T_transform.data.db;
            const double* src = _M->data.db + i*3;
            double* dst = _Mxy->data.db + i*2;

            dst[0] = Rp[0]*src[0] + Rp[1]*src[1] + Rp[2]*src[2] + Tp[0];
            dst[1] = Rp[3]*src[0] + Rp[4]*src[1] + Rp[5]*src[2] + Tp[1];
        }

        cvFindHomography( _Mxy, _mn, &_H );

        cvGetCol( &_H, &_h1, 0 );
        _h2 = _h1; _h2.data.db++;
        _h3 = _h2; _h3.data.db++;
        h1_norm = sqrt(h[0]*h[0] + h[3]*h[3] + h[6]*h[6]);
        h2_norm = sqrt(h[1]*h[1] + h[4]*h[4] + h[7]*h[7]);

        cvScale( &_h1, &_h1, 1./h1_norm );
        cvScale( &_h2, &_h2, 1./h2_norm );
        cvScale( &_h3, &_t, 2./(h1_norm + h2_norm));
        cvCrossProduct( &_h1, &_h2, &_h3 );

        cvRodrigues2( &_H, &_r );
        cvRodrigues2( &_r, &_H );
        cvMatMulAdd( &_H, &T_transform, &_t, &_t );
        cvMatMul( &_H, R_transform, &_R );
        cvRodrigues2( &_R, &_r );
    }
    else
    {
        // non-planar structure. Use DLT method
        double* L;
        double LL[12*12], LW[12], LV[12*12], sc;
        CvMat _LL = cvMat( 12, 12, CV_64F, LL );
        CvMat _LW = cvMat( 12, 1, CV_64F, LW );
        CvMat _LV = cvMat( 12, 12, CV_64F, LV );
        CvMat _RRt, _RR, _tt;

        CV_CALL( _L = cvCreateMat( 2*count, 12, CV_64F ));
        L = _L->data.db;

        for( i = 0; i < count; i++, L += 24 )
        {
            double x = -mn[i].x, y = -mn[i].y;
            L[0] = L[16] = M[i].x;
            L[1] = L[17] = M[i].y;
            L[2] = L[18] = M[i].z;
            L[3] = L[19] = 1.;
            L[4] = L[5] = L[6] = L[7] = 0.;
            L[12] = L[13] = L[14] = L[15] = 0.;
            L[8] = x*M[i].x;
            L[9] = x*M[i].y;
            L[10] = x*M[i].z;
            L[11] = x;
            L[20] = y*M[i].x;
            L[21] = y*M[i].y;
            L[22] = y*M[i].z;
            L[23] = y;
        }

        cvMulTransposed( _L, &_LL, 1 );
        cvSVD( &_LL, &_LW, 0, &_LV, CV_SVD_MODIFY_A + CV_SVD_V_T );
        _RRt = cvMat( 3, 4, CV_64F, LV + 11*12 );
        cvGetCols( &_RRt, &_RR, 0, 3 );
        cvGetCol( &_RRt, &_tt, 3 );
        if( cvDet(&_RR) < 0 )
            cvScale( &_RRt, &_RRt, -1 );
        sc = cvNorm(&_RR);
        cvSVD( &_RR, &_W, &_U, &_V, CV_SVD_MODIFY_A + CV_SVD_U_T + CV_SVD_V_T );
        cvGEMM( &_U, &_V, 1, 0, 0, &_R, CV_GEMM_A_T );
        cvScale( &_tt, &_t, cvNorm(&_R)/sc );
        cvRodrigues2( &_R, &_r );
        cvReleaseMat( &_L );
    }

    CV_CALL( _J = cvCreateMat( 2*count, 6, CV_64FC1 ));
    cvGetCols( _J, &_dpdr, 0, 3 );
    cvGetCols( _J, &_dpdt, 3, 6 );

    // refine extrinsic parameters using iterative algorithm
    for( i = 0; i < max_iter; i++ )
    {
        double n1, n2;
        cvReshape( _mn, _mn, 2, 1 );
        cvProjectPoints2( _M, &_r, &_t, &_a, dist_coeffs,
                          _mn, &_dpdr, &_dpdt, 0, 0, 0 );
        cvSub( _m, _mn, _mn );
        cvReshape( _mn, _mn, 1, 2*count );

        cvMulTransposed( _J, &_JtJ, 1 );
        cvGEMM( _J, _mn, 1, 0, 0, &_JtErr, CV_GEMM_A_T );
        cvSVD( &_JtJ, &_JtJW, 0, &_JtJV, CV_SVD_MODIFY_A + CV_SVD_V_T );
        if( JtJW[5]/JtJW[0] < 1e-12 )
            break;
        cvSVBkSb( &_JtJW, &_JtJV, &_JtJV, &_JtErr,
                  &_delta, CV_SVD_U_T + CV_SVD_V_T );
        cvAdd( &_delta, &_param, &_param );
        n1 = cvNorm( &_delta );
        n2 = cvNorm( &_param );
        if( n1/n2 < 1e-10 )
            break;
    }

    _r = cvMat( r_vec->rows, r_vec->cols,
        CV_MAKETYPE(CV_64F,CV_MAT_CN(r_vec->type)), param );
    _t = cvMat( t_vec->rows, t_vec->cols,
        CV_MAKETYPE(CV_64F,CV_MAT_CN(t_vec->type)), param + 3 );

    cvConvert( &_r, r_vec );
    cvConvert( &_t, t_vec );

    __END__;

    cvReleaseMat( &_M );
    cvReleaseMat( &_Mxy );
    cvReleaseMat( &_m );
    cvReleaseMat( &_mn );
    cvReleaseMat( &_L );
    cvReleaseMat( &_J );
}
// 
// 
static void
icvInitIntrinsicParams2D( const CvMat* obj_points,
                          const CvMat* img_points,
                          const CvMat* point_counts,
                          CvSize image_size,
                          CvMat* intrinsic_matrix,
                          double aspect_ratio )
{
    CvMat *_A = 0, *_b = 0;
    
    CV_FUNCNAME( "icvInitIntrinsicParams2D" );

    __BEGIN__;

    int i, j, pos, img_count;
    double a[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    double H[9], AtA[4], AtAW[2], AtAV[4], Atb[2], f[2];
    CvMat _a = cvMat( 3, 3, CV_64F, a );
    CvMat _H = cvMat( 3, 3, CV_64F, H );
    CvMat _AtA = cvMat( 2, 2, CV_64F, AtA );
    CvMat _AtAW = cvMat( 2, 1, CV_64F, AtAW );
    CvMat _AtAV = cvMat( 2, 2, CV_64F, AtAV );
    CvMat _Atb = cvMat( 2, 1, CV_64F, Atb );
    CvMat _f = cvMat( 2, 1, CV_64F, f );

    assert( CV_MAT_TYPE(point_counts->type) == CV_32SC1 &&
            CV_IS_MAT_CONT(point_counts->type) );
    img_count = point_counts->rows + point_counts->cols - 1;

    if( CV_MAT_TYPE(obj_points->type) != CV_32FC3 &&
        CV_MAT_TYPE(obj_points->type) != CV_64FC3 ||
        CV_MAT_TYPE(img_points->type) != CV_32FC2 &&
        CV_MAT_TYPE(img_points->type) != CV_64FC2 )
        CV_ERROR( CV_StsUnsupportedFormat, "Both object points and image points must be 2D" );

    if( obj_points->rows != 1 || img_points->rows != 1 )
        CV_ERROR( CV_StsBadSize, "object points and image points must be a single-row matrices" );

    CV_CALL( _A = cvCreateMat( 2*img_count, 2, CV_64F ));
    CV_CALL( _b = cvCreateMat( 2*img_count, 1, CV_64F ));
    a[2] = (image_size.width - 1)*0.5;
    a[5] = (image_size.height - 1)*0.5;

    // extract vanishing points in order to obtain initial value for the focal length
    for( i = 0, pos = 0; i < img_count; i++ )
    {
        double* Ap = _A->data.db + i*4;
        double* bp = _b->data.db + i*2;
        int count = point_counts->data.i[i];
        double h[3], v[3], d1[3], d2[3];
        double n[4] = {0,0,0,0};
        CvMat _m, _M;
        cvGetCols( obj_points, &_M, pos, pos + count );
        cvGetCols( img_points, &_m, pos, pos + count );
        pos += count;

        CV_CALL( cvFindHomography( &_M, &_m, &_H ));
        
        H[0] -= H[6]*a[2]; H[1] -= H[7]*a[2]; H[2] -= H[8]*a[2];
        H[3] -= H[6]*a[5]; H[4] -= H[7]*a[5]; H[5] -= H[8]*a[5];

        for( j = 0; j < 3; j++ )
        {
            double t0 = H[j*3], t1 = H[j*3+1];
            h[j] = t0; v[j] = t1;
            d1[j] = (t0 + t1)*0.5;
            d2[j] = (t0 - t1)*0.5;
            n[0] += t0*t0; n[1] += t1*t1;
            n[2] += d1[j]*d1[j]; n[3] += d2[j]*d2[j];
        }

        for( j = 0; j < 4; j++ )
            n[j] = 1./sqrt(n[j]);

        for( j = 0; j < 3; j++ )
        {
            h[j] *= n[0]; v[j] *= n[1];
            d1[j] *= n[2]; d2[j] *= n[3];
        }

        Ap[0] = h[0]*v[0]; Ap[1] = h[1]*v[1];
        Ap[2] = d1[0]*d2[0]; Ap[3] = d1[1]*d2[1];
        bp[0] = -h[2]*v[2]; bp[1] = -d1[2]*d2[2];
    }

    // while it is not about gradient descent search,
    // the formula is the same: f = inv(At*A)*At*b
    icvGaussNewton( _A, _b, &_f, &_AtA, &_Atb, &_AtAW, &_AtAV );
    a[0] = sqrt(fabs(1./f[0]));
    a[4] = sqrt(fabs(1./f[1]));
    if( aspect_ratio != 0 )
    {
        double tf = (a[0] + a[4])/(aspect_ratio + 1.);
        a[0] = aspect_ratio*tf;
        a[4] = tf;
    }

    cvConvert( &_a, intrinsic_matrix );

    __END__;

    cvReleaseMat( &_A );
    cvReleaseMat( &_b );
}
// 
// 
// /* finds intrinsic and extrinsic camera parameters
//    from a few views of known calibration pattern */
CV_IMPL void
cvCalibrateCamera2( const CvMat* obj_points,
                    const CvMat* img_points,
                    const CvMat* point_counts,
                    CvSize image_size,
                    CvMat* A, CvMat* dist_coeffs,
                    CvMat* r_vecs, CvMat* t_vecs,
                    int flags )
{
    double alpha_smooth = 0.4;
    
    CvMat *counts = 0, *_M = 0, *_m = 0;
    CvMat *_Ji = 0, *_Je = 0, *_JtJ = 0, *_JtErr = 0, *_JtJW = 0, *_JtJV = 0;
    CvMat *_param = 0, *_param_innov = 0, *_err = 0;
    
    CV_FUNCNAME( "cvCalibrateCamera2" );

    __BEGIN__;

    double a[9];
    CvMat _a = cvMat( 3, 3, CV_64F, a ), _k;
    CvMat _Mi, _mi, _ri, _ti, _part;
    CvMat _dpdr, _dpdt, _dpdf, _dpdc, _dpdk;
    CvMat _sr_part = cvMat( 1, 3, CV_64F ), _st_part = cvMat( 1, 3, CV_64F ), _r_part, _t_part;
    int i, j, pos, iter, img_count, count = 0, max_count = 0, total = 0, param_count;
    int r_depth = 0, t_depth = 0, r_step = 0, t_step = 0, cn, dims;
    int output_r_matrices = 0;
    double aspect_ratio = 0.;

    if( !CV_IS_MAT(obj_points) || !CV_IS_MAT(img_points) ||
        !CV_IS_MAT(point_counts) || !CV_IS_MAT(A) || !CV_IS_MAT(dist_coeffs) )
        CV_ERROR( CV_StsBadArg, "One of required vector arguments is not a valid matrix" );

    if( image_size.width <= 0 || image_size.height <= 0 )
        CV_ERROR( CV_StsOutOfRange, "image width and height must be positive" );

    if( CV_MAT_TYPE(point_counts->type) != CV_32SC1 ||
        point_counts->rows != 1 && point_counts->cols != 1 )
        CV_ERROR( CV_StsUnsupportedFormat,
            "the array of point counters must be 1-dimensional integer vector" );

    CV_CALL( counts = cvCreateMat( point_counts->rows, point_counts->width, CV_32SC1 ));
    cvCopy( point_counts, counts );

    img_count = counts->rows + counts->cols - 1;

    if( r_vecs )
    {
        r_depth = CV_MAT_DEPTH(r_vecs->type);
        r_step = r_vecs->rows == 1 ? 3*CV_ELEM_SIZE(r_depth) : r_vecs->step;
        cn = CV_MAT_CN(r_vecs->type);
        if( !CV_IS_MAT(r_vecs) || r_depth != CV_32F && r_depth != CV_64F ||
            (r_vecs->rows != img_count || r_vecs->cols*cn != 3 && r_vecs->cols*cn != 9) &&
            (r_vecs->rows != 1 || r_vecs->cols != img_count || cn != 3) )
            CV_ERROR( CV_StsBadArg, "the output array of rotation vectors must be 3-channel "
                "1xn or nx1 array or 1-channel nx3 or nx9 array, where n is the number of views" );
        output_r_matrices = r_vecs->rows == img_count && r_vecs->cols*cn == 9;
    }

    if( t_vecs )
    {
        t_depth = CV_MAT_DEPTH(t_vecs->type);
        t_step = t_vecs->rows == 1 ? 3*CV_ELEM_SIZE(t_depth) : t_vecs->step;
        cn = CV_MAT_CN(t_vecs->type);
        if( !CV_IS_MAT(t_vecs) || t_depth != CV_32F && t_depth != CV_64F ||
            (t_vecs->rows != img_count || t_vecs->cols*cn != 3) &&
            (t_vecs->rows != 1 || t_vecs->cols != img_count || cn != 3) )
            CV_ERROR( CV_StsBadArg, "the output array of translation vectors must be 3-channel "
                "1xn or nx1 array or 1-channel nx3 array, where n is the number of views" );
    }

    if( CV_MAT_TYPE(A->type) != CV_32FC1 && CV_MAT_TYPE(A->type) != CV_64FC1 ||
        A->rows != 3 || A->cols != 3 )
        CV_ERROR( CV_StsBadArg,
            "Intrinsic parameters must be 3x3 floating-point matrix" );

    if( CV_MAT_TYPE(dist_coeffs->type) != CV_32FC1 &&
        CV_MAT_TYPE(dist_coeffs->type) != CV_64FC1 ||
        (dist_coeffs->rows != 4 || dist_coeffs->cols != 1) &&
        (dist_coeffs->cols != 4 || dist_coeffs->rows != 1))
        CV_ERROR( CV_StsBadArg,
            "Distortion coefficients must be 4x1 or 1x4 floating-point matrix" );

    for( i = 0; i < img_count; i++ )
    {
        int temp_count = counts->data.i[i];
        if( temp_count < 4 )
        {
            char buf[100];
            sprintf( buf, "The number of points in the view #%d is <4", i );
            CV_ERROR( CV_StsOutOfRange, buf );
        }
        max_count = MAX( max_count, temp_count );
        total += temp_count;
    }

    dims = CV_MAT_CN(obj_points->type)*(obj_points->rows == 1 ? 1 : obj_points->cols);

    if( CV_MAT_DEPTH(obj_points->type) != CV_32F &&
        CV_MAT_DEPTH(obj_points->type) != CV_64F ||
        (obj_points->rows != total || dims != 3 && dims != 2) &&
        (obj_points->rows != 1 || obj_points->cols != total || (dims != 3 && dims != 2)) )
        CV_ERROR( CV_StsBadArg, "Object points must be 1xn or nx1, 2- or 3-channel matrix, "
                                "or nx3 or nx2 single-channel matrix" );

    cn = CV_MAT_CN(img_points->type);
    if( CV_MAT_DEPTH(img_points->type) != CV_32F &&
        CV_MAT_DEPTH(img_points->type) != CV_64F ||
        (img_points->rows != total || img_points->cols*cn != 2) &&
        (img_points->rows != 1 || img_points->cols != total || cn != 2) )
        CV_ERROR( CV_StsBadArg, "Image points must be 1xn or nx1, 2-channel matrix, "
                                "or nx2 single-channel matrix" );

    CV_CALL( _M = cvCreateMat( 1, total, CV_64FC3 ));
    CV_CALL( _m = cvCreateMat( 1, total, CV_64FC2 ));

    CV_CALL( cvConvertPointsHomogenious( obj_points, _M ));
    CV_CALL( cvConvertPointsHomogenious( img_points, _m ));

    param_count = 8 + img_count*6;
    CV_CALL( _param = cvCreateMat( param_count, 1, CV_64FC1 ));
    CV_CALL( _param_innov = cvCreateMat( param_count, 1, CV_64FC1 ));
    CV_CALL( _JtJ = cvCreateMat( param_count, param_count, CV_64FC1 ));
    CV_CALL( _JtErr = cvCreateMat( param_count, 1, CV_64FC1 ));
    CV_CALL( _JtJW = cvCreateMat( param_count, 1, CV_64FC1 ));
    CV_CALL( _JtJV = cvCreateMat( param_count, param_count, CV_64FC1 ));
    CV_CALL( _Ji = cvCreateMat( max_count*2, 8, CV_64FC1 ));
    CV_CALL( _Je = cvCreateMat( max_count*2, 6, CV_64FC1 ));
    CV_CALL( _err = cvCreateMat( max_count*2, 1, CV_64FC1 ));
    
    cvGetCols( _Je, &_dpdr, 0, 3 );
    cvGetCols( _Je, &_dpdt, 3, 6 );
    cvGetCols( _Ji, &_dpdf, 0, 2 );
    cvGetCols( _Ji, &_dpdc, 2, 4 );
    cvGetCols( _Ji, &_dpdk, 4, flags & CV_CALIB_ZERO_TANGENT_DIST ? 6 : 8 );
    cvZero( _Ji );

    // 1. initialize intrinsic parameters
    if( flags & CV_CALIB_USE_INTRINSIC_GUESS )
    {
        cvConvert( A, &_a );
        if( a[0] <= 0 || a[4] <= 0 )
            CV_ERROR( CV_StsOutOfRange, "Focal length (fx and fy) must be positive" );
        if( a[2] < 0 || a[2] >= image_size.width ||
            a[5] < 0 || a[5] >= image_size.height )
            CV_ERROR( CV_StsOutOfRange, "Principal point must be within the image" );
        if( fabs(a[1]) > 1e-5 )
            CV_ERROR( CV_StsOutOfRange, "Non-zero skew is not supported by the function" );
        if( fabs(a[3]) > 1e-5 || fabs(a[6]) > 1e-5 ||
            fabs(a[7]) > 1e-5 || fabs(a[8]-1) > 1e-5 )
            CV_ERROR( CV_StsOutOfRange,
                "The intrinsic matrix must have [fx 0 cx; 0 fy cy; 0 0 1] shape" );
        a[1] = a[3] = a[6] = a[7] = 0.;
        a[8] = 1.;

        if( flags & CV_CALIB_FIX_ASPECT_RATIO )
            aspect_ratio = a[0]/a[4];
    }
    else
    {
        if( dims == 3 )
        {
            CvScalar mean, sdv;
            cvAvgSdv( _M, &mean, &sdv );
            if( fabs(mean.val[2]) > 1e-5 && fabs(mean.val[2] - 1) > 1e-5 ||
                fabs(sdv.val[2]) > 1e-5 )
                CV_ERROR( CV_StsBadArg,
                "For non-planar calibration rigs the initial intrinsic matrix must be specified" );
        }
        for( i = 0; i < total; i++ )
            ((CvPoint3D64f*)(_M->data.db + i*3))->z = 0.;

        if( flags & CV_CALIB_FIX_ASPECT_RATIO )
        {
            aspect_ratio = cvmGet(A,0,0);
            aspect_ratio /= cvmGet(A,1,1);
            if( aspect_ratio < 0.01 || aspect_ratio > 100 )
                CV_ERROR( CV_StsOutOfRange,
                    "The specified aspect ratio (=a(0,0)/a(1,1)) is incorrect" );
        }
        icvInitIntrinsicParams2D( _M, _m, counts, image_size, &_a, aspect_ratio );
    }

    _k = cvMat( dist_coeffs->rows, dist_coeffs->cols, CV_64FC1, _param->data.db + 4 );
    cvZero( &_k );

    // 2. initialize extrinsic parameters
    for( i = 0, pos = 0; i < img_count; i++, pos += count )
    {
        count = counts->data.i[i];
        _ri = cvMat( 1, 3, CV_64FC1, _param->data.db + 8 + i*6 );
        _ti = cvMat( 1, 3, CV_64FC1, _param->data.db + 8 + i*6 + 3 );

        cvGetCols( _M, &_Mi, pos, pos + count );
        cvGetCols( _m, &_mi, pos, pos + count );
        cvFindExtrinsicCameraParams2( &_Mi, &_mi, &_a, &_k, &_ri, &_ti );
    }

    _param->data.db[0] = a[0];
    _param->data.db[1] = a[4];
    _param->data.db[2] = a[2];
    _param->data.db[3] = a[5];

    // 3. run the optimization
    for( iter = 0; iter < 30; iter++ )
    {
        double* jj = _JtJ->data.db;
        double change;

        for( i = 0, pos = 0; i < img_count; i++, pos += count )
        {
            count = counts->data.i[i];
            _ri = cvMat( 1, 3, CV_64FC1, _param->data.db + 8 + i*6);
            _ti = cvMat( 1, 3, CV_64FC1, _param->data.db + 8 + i*6 + 3);

            cvGetCols( _M, &_Mi, pos, pos + count );
            _mi = cvMat( count*2, 1, CV_64FC1, _m->data.db + pos*2 );

            _dpdr.rows = _dpdt.rows = _dpdf.rows = _dpdc.rows = _dpdk.rows = count*2;

            _err->cols = 1;
            _err->rows = count*2;
            cvReshape( _err, _err, 2, count );
            cvProjectPoints2( &_Mi, &_ri, &_ti, &_a, &_k, _err, &_dpdr, &_dpdt, &_dpdf,
                              flags & CV_CALIB_FIX_PRINCIPAL_POINT ? 0 : &_dpdc, &_dpdk );

            // alter dpdf in case if only one of the focal
            // parameters (fy) is independent variable
            if( flags & CV_CALIB_FIX_ASPECT_RATIO )
                for( j = 0; j < count; j++ )
                {
                    double* dpdf_j = (double*)(_dpdf.data.ptr + j*_dpdf.step*2);
                    dpdf_j[1] = dpdf_j[0]*aspect_ratio;
                    dpdf_j[0] = 0.;
                }

            cvReshape( _err, _err, 1, count*2 );
            cvSub( &_mi, _err, _err );
            
            _Je->rows = _Ji->rows = count*2;
            
            cvGetSubRect( _JtJ, &_part, cvRect(0,0,8,8) );
            cvGEMM( _Ji, _Ji, 1, &_part, i > 0, &_part, CV_GEMM_A_T );

            cvGetSubRect( _JtJ, &_part, cvRect(8+i*6,8+i*6,6,6) );
            cvMulTransposed( _Je, &_part, 1 );
            
            cvGetSubRect( _JtJ, &_part, cvRect(8+i*6,0,6,8) );
            cvGEMM( _Ji, _Je, 1, 0, 0, &_part, CV_GEMM_A_T );

            cvGetRows( _JtErr, &_part, 0, 8 );
            cvGEMM( _Ji, _err, 1, &_part, i > 0, &_part, CV_GEMM_A_T );

            cvGetRows( _JtErr, &_part, 8 + i*6, 8 + (i+1)*6 );
            cvGEMM( _Je, _err, 1, 0, 0, &_part, CV_GEMM_A_T );
        }

        // make the matrix JtJ exactly symmetrical and add missing zeros
        for( i = 0; i < param_count; i++ )
        {
            int mj = i < 8 ? param_count : ((i - 8)/6)*6 + 14;
            for( j = i+1; j < mj; j++ )
                jj[j*param_count + i] = jj[i*param_count + j];
            for( ; j < param_count; j++ )
                jj[j*param_count + i] = jj[i*param_count + j] = 0;
        }

        cvSVD( _JtJ, _JtJW, 0, _JtJV, CV_SVD_MODIFY_A + CV_SVD_V_T );
        cvSVBkSb( _JtJW, _JtJV, _JtJV, _JtErr, _param_innov, CV_SVD_U_T + CV_SVD_V_T );

        cvScale( _param_innov, _param_innov, 1. - pow(1. - alpha_smooth, iter + 1.) );
        cvGetRows( _param_innov, &_part, 0, 4 );
        change = cvNorm( &_part );
        cvGetRows( _param, &_part, 0, 4 );
        change /= cvNorm( &_part );

        if( flags & CV_CALIB_FIX_PRINCIPAL_POINT )
            _param_innov->data.db[2] = _param_innov->data.db[3] = 0.;

        if( flags & CV_CALIB_ZERO_TANGENT_DIST )
            _param_innov->data.db[6] = _param_innov->data.db[7] = 0.;

        cvAdd( _param, _param_innov, _param );

        if( flags & CV_CALIB_FIX_ASPECT_RATIO )
            _param->data.db[0] = _param->data.db[1]*aspect_ratio;
        
        a[0] = _param->data.db[0];
        a[4] = _param->data.db[1];
        a[2] = _param->data.db[2];
        a[5] = _param->data.db[3];

        if( change < FLT_EPSILON )
            break;
    }

    cvConvert( &_a, A );
    cvConvert( &_k, dist_coeffs );

    _r_part = cvMat( output_r_matrices ? 3 : 1, 3, r_depth );
    _t_part = cvMat( 1, 3, t_depth );
    for( i = 0; i < img_count; i++ )
    {
        if( r_vecs )
        {
            _sr_part.data.db = _param->data.db + 8 + i*6;
            _r_part.data.ptr = r_vecs->data.ptr + i*r_step;
            if( !output_r_matrices )
                cvConvert( &_sr_part, &_r_part );
            else
            {
                cvRodrigues2( &_sr_part, &_a );
                cvConvert( &_a, &_r_part );
            }
        }
        if( t_vecs )
        {
            _st_part.data.db = _param->data.db + 8 + i*6 + 3;
            _t_part.data.ptr = t_vecs->data.ptr + i*t_step;
            cvConvert( &_st_part, &_t_part );
        }
    }

    __END__;

    cvReleaseMat( &counts );
    cvReleaseMat( &_M );
    cvReleaseMat( &_m );
    cvReleaseMat( &_param );
    cvReleaseMat( &_param_innov );
    cvReleaseMat( &_JtJ );
    cvReleaseMat( &_JtErr );
    cvReleaseMat( &_JtJW );
    cvReleaseMat( &_JtJV );
    cvReleaseMat( &_Ji );
    cvReleaseMat( &_Je );
    cvReleaseMat( &_err );
}


typedef CvStatus (CV_STDCALL * CvColorCvtFunc0)(
    const void* src, int srcstep, void* dst, int dststep, CvSize size );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc1)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0 );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc2)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0, int param1 );

typedef CvStatus (CV_STDCALL * CvColorCvtFunc3)(
    const void* src, int srcstep, void* dst, int dststep,
    CvSize size, int param0, int param1, int param2 );

/****************************************************************************************\
*                 Various 3/4-channel to 3/4-channel RGB transformations                 *
\****************************************************************************************/

#define CV_IMPL_BGRX2BGR( flavor, arrtype )                             \
static CvStatus CV_STDCALL                                              \
icvBGRx2BGR_##flavor##_CnC3R( const arrtype* src, int srcstep,          \
                              arrtype* dst, int dststep,                \
                              CvSize size, int src_cn, int blue_idx )   \
{                                                                       \
    int i;                                                              \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
    srcstep -= size.width*src_cn;                                       \
    size.width *= 3;                                                    \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += 3, src += src_cn )             \
        {                                                               \
            arrtype t0=src[blue_idx], t1=src[1], t2=src[blue_idx^2];    \
            dst[i] = t0;                                                \
            dst[i+1] = t1;                                              \
            dst[i+2] = t2;                                              \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


#define CV_IMPL_BGR2BGRX( flavor, arrtype )                             \
static CvStatus CV_STDCALL                                              \
icvBGR2BGRx_##flavor##_C3C4R( const arrtype* src, int srcstep,          \
                              arrtype* dst, int dststep,                \
                              CvSize size, int blue_idx )               \
{                                                                       \
    int i;                                                              \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
    srcstep -= size.width*3;                                            \
    size.width *= 4;                                                    \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += 4, src += 3 )                  \
        {                                                               \
            arrtype t0=src[blue_idx], t1=src[1], t2=src[blue_idx^2];    \
            dst[i] = t0;                                                \
            dst[i+1] = t1;                                              \
            dst[i+2] = t2;                                              \
            dst[i+3] = 0;                                               \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


#define CV_IMPL_BGRA2RGBA( flavor, arrtype )                            \
static CvStatus CV_STDCALL                                              \
icvBGRA2RGBA_##flavor##_C4R( const arrtype* src, int srcstep,           \
                             arrtype* dst, int dststep, CvSize size )   \
{                                                                       \
    int i;                                                              \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
    size.width *= 4;                                                    \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += 4 )                            \
        {                                                               \
            arrtype t0 = src[2], t1 = src[1], t2 = src[0], t3 = src[3]; \
            dst[i] = t0;                                                \
            dst[i+1] = t1;                                              \
            dst[i+2] = t2;                                              \
            dst[i+3] = t3;                                              \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


CV_IMPL_BGRX2BGR( 8u, uchar )
CV_IMPL_BGRX2BGR( 16u, ushort )
CV_IMPL_BGRX2BGR( 32f, int )
CV_IMPL_BGR2BGRX( 8u, uchar )
CV_IMPL_BGR2BGRX( 16u, ushort )
CV_IMPL_BGR2BGRX( 32f, int )
CV_IMPL_BGRA2RGBA( 8u, uchar )
CV_IMPL_BGRA2RGBA( 16u, ushort )
CV_IMPL_BGRA2RGBA( 32f, int )


/****************************************************************************************\
*           Transforming 16-bit (565 or 555) RGB to/from 24/32-bit (888[8]) RGB          *
\****************************************************************************************/

static CvStatus CV_STDCALL
icvBGR5x52BGRx_8u_C2CnR( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int dst_cn,
                         int blue_idx, int green_bits )
{
    int i;
    assert( green_bits == 5 || green_bits == 6 );
    dststep -= size.width*dst_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++, dst += dst_cn )
            {
                unsigned t = ((const ushort*)src)[i];
                dst[blue_idx] = (uchar)(t << 3);
                dst[1] = (uchar)((t >> 3) & ~3);
                dst[blue_idx ^ 2] = (uchar)((t >> 8) & ~7);
                if( dst_cn == 4 )
                    dst[3] = 0;
            }
        else
            for( i = 0; i < size.width; i++, dst += dst_cn )
            {
                unsigned t = ((const ushort*)src)[i];
                dst[blue_idx] = (uchar)(t << 3);
                dst[1] = (uchar)((t >> 2) & ~7);
                dst[blue_idx ^ 2] = (uchar)((t >> 7) & ~7);
                if( dst_cn == 4 )
                    dst[3] = 0;
            }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2BGR5x5_8u_CnC2R( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int src_cn,
                         int blue_idx, int green_bits )
{
    int i;
    srcstep -= size.width*src_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t = (src[blue_idx] >> 3)|((src[1]&~3) << 3)|((src[blue_idx^2]&~7) << 8);
                ((ushort*)dst)[i] = (ushort)t;
            }
        else
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t = (src[blue_idx] >> 3)|((src[1]&~7) << 2)|((src[blue_idx^2]&~7) << 7);
                ((ushort*)dst)[i] = (ushort)t;
            }
    }

    return CV_OK;
}



/////////////////////////// IPP Color Conversion Functions //////////////////////////////

icvRGB2XYZ_8u_C3R_t  icvRGB2XYZ_8u_C3R_p = 0;
icvRGB2XYZ_16u_C3R_t icvRGB2XYZ_16u_C3R_p = 0;
icvRGB2XYZ_32f_C3R_t icvRGB2XYZ_32f_C3R_p = 0;
icvXYZ2RGB_8u_C3R_t  icvXYZ2RGB_8u_C3R_p = 0;
icvXYZ2RGB_16u_C3R_t icvXYZ2RGB_16u_C3R_p = 0;
icvXYZ2RGB_32f_C3R_t icvXYZ2RGB_32f_C3R_p = 0;

icvRGB2HSV_8u_C3R_t  icvRGB2HSV_8u_C3R_p = 0;
icvHSV2RGB_8u_C3R_t  icvHSV2RGB_8u_C3R_p = 0;

icvBGR2Lab_8u_C3R_t  icvBGR2Lab_8u_C3R_p = 0;
icvLab2BGR_8u_C3R_t  icvLab2BGR_8u_C3R_p = 0;

icvRGB2HLS_8u_C3R_t  icvRGB2HLS_8u_C3R_p = 0;
icvRGB2HLS_32f_C3R_t icvRGB2HLS_32f_C3R_p = 0;
icvHLS2RGB_8u_C3R_t  icvHLS2RGB_8u_C3R_p = 0;
icvHLS2RGB_32f_C3R_t icvHLS2RGB_32f_C3R_p = 0;

icvRGB2Luv_8u_C3R_t  icvRGB2Luv_8u_C3R_p = 0;
icvLuv2RGB_8u_C3R_t  icvLuv2RGB_8u_C3R_p = 0;

//icvRGB2Luv_32f_C3R_t icvRGB2Luv_32f_C3R_p = 0;
//icvLuv2RGB_32f_C3R_t icvLuv2RGB_32f_C3R_p = 0;
//icvRGB2Luv_32f_C3R_t icvRGB2Luv_32f_C3R_p = 0;
//icvLuv2RGB_32f_C3R_t icvLuv2RGB_32f_C3R_p = 0;


#define CV_IMPL_BGRx2ABC_IPP( flavor, arrtype )                         \
static CvStatus CV_STDCALL                                              \
icvBGRx2ABC_IPP_##flavor##_CnC3R( const arrtype* src, int srcstep,      \
    arrtype* dst, int dststep, CvSize size, int src_cn,                 \
    int blue_idx, CvColorCvtFunc0 ipp_func )                            \
{                                                                       \
    int block_size = MIN(1 << 14, size.width);                          \
    arrtype* buffer;                                                    \
    int i, di, k;                                                       \
    int do_copy = src_cn > 3 || blue_idx != 2 || src == dst;            \
    CvStatus status = CV_OK;                                            \
                                                                        \
    if( !do_copy )                                                      \
        return ipp_func( src, srcstep, dst, dststep, size );            \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    buffer = (arrtype*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );  \
    srcstep -= size.width*src_cn;                                       \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += block_size )                   \
        {                                                               \
            arrtype* dst1 = dst + i*3;                                  \
            di = MIN(block_size, size.width - i);                       \
                                                                        \
            for( k = 0; k < di*3; k += 3, src += src_cn )               \
            {                                                           \
                arrtype b = src[blue_idx];                              \
                arrtype g = src[1];                                     \
                arrtype r = src[blue_idx^2];                            \
                buffer[k] = r;                                          \
                buffer[k+1] = g;                                        \
                buffer[k+2] = b;                                        \
            }                                                           \
                                                                        \
            status = ipp_func( buffer, CV_STUB_STEP,                    \
                               dst1, CV_STUB_STEP, cvSize(di,1) );      \
            if( status < 0 )                                            \
                return status;                                          \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


static CvStatus CV_STDCALL
icvBGRx2ABC_IPP_8u_CnC3R( const uchar* src, int srcstep,
    uchar* dst, int dststep, CvSize size, int src_cn,
    int blue_idx, CvColorCvtFunc0 ipp_func )
{
    int block_size = MIN(1 << 14, size.width);
    uchar* buffer;
    int i, di, k;
    int do_copy = src_cn > 3 || blue_idx != 2 || src == dst;
    CvStatus status = CV_OK;

    if( !do_copy )
        return ipp_func( src, srcstep, dst, dststep, size );

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);

    buffer = (uchar*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
    srcstep -= size.width*src_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += block_size )
        {
            uchar* dst1 = dst + i*3;
            di = MIN(block_size, size.width - i);

            for( k = 0; k < di*3; k += 3, src += src_cn )
            {
                uchar b = src[blue_idx];
                uchar g = src[1];
                uchar r = src[blue_idx^2];
                buffer[k] = r;
                buffer[k+1] = g;
                buffer[k+2] = b;
            }

            status = ipp_func( buffer, CV_STUB_STEP,
                               dst1, CV_STUB_STEP, cvSize(di,1) );
            if( status < 0 )
                return status;
        }
    }

    return CV_OK;
}



//CV_IMPL_BGRx2ABC_IPP( 8u, uchar )
CV_IMPL_BGRx2ABC_IPP( 16u, ushort )
CV_IMPL_BGRx2ABC_IPP( 32f, float )

#define CV_IMPL_ABC2BGRx_IPP( flavor, arrtype )                         \
static CvStatus CV_STDCALL                                              \
icvABC2BGRx_IPP_##flavor##_C3CnR( const arrtype* src, int srcstep,      \
    arrtype* dst, int dststep, CvSize size, int dst_cn,                 \
    int blue_idx, CvColorCvtFunc0 ipp_func )                            \
{                                                                       \
    int block_size = MIN(1 << 10, size.width);                          \
    arrtype* buffer;                                                    \
    int i, di, k;                                                       \
    int do_copy = dst_cn > 3 || blue_idx != 2 || src == dst;            \
    CvStatus status = CV_OK;                                            \
                                                                        \
    if( !do_copy )                                                      \
        return ipp_func( src, srcstep, dst, dststep, size );            \
                                                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    buffer = (arrtype*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );  \
    dststep -= size.width*dst_cn;                                       \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        for( i = 0; i < size.width; i += block_size )                   \
        {                                                               \
            const arrtype* src1 = src + i*3;                            \
            di = MIN(block_size, size.width - i);                       \
                                                                        \
            status = ipp_func( src1, CV_STUB_STEP,                      \
                               buffer, CV_STUB_STEP, cvSize(di,1) );    \
            if( status < 0 )                                            \
                return status;                                          \
                                                                        \
            for( k = 0; k < di*3; k += 3, dst += dst_cn )               \
            {                                                           \
                arrtype r = buffer[k];                                  \
                arrtype g = buffer[k+1];                                \
                arrtype b = buffer[k+2];                                \
                dst[blue_idx] = b;                                      \
                dst[1] = g;                                             \
                dst[blue_idx^2] = r;                                    \
                if( dst_cn == 4 )                                       \
                    dst[3] = 0;                                         \
            }                                                           \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

CV_IMPL_ABC2BGRx_IPP( 8u, uchar )
CV_IMPL_ABC2BGRx_IPP( 16u, ushort )
CV_IMPL_ABC2BGRx_IPP( 32f, float )


/////////////////////////////////////////////////////////////////////////////////////////


/****************************************************************************************\
*                                 Color to/from Grayscale                                *
\****************************************************************************************/

#define fix(x,n)      (int)((x)*(1 << (n)) + 0.5)
#define descale       CV_DESCALE

#define cscGr_32f  0.299f
#define cscGg_32f  0.587f
#define cscGb_32f  0.114f

/* BGR/RGB -> Gray */
#define csc_shift  14
#define cscGr  fix(cscGr_32f,csc_shift) 
#define cscGg  fix(cscGg_32f,csc_shift)
#define cscGb  /*fix(cscGb_32f,csc_shift)*/ ((1 << csc_shift) - cscGr - cscGg)

#define CV_IMPL_GRAY2BGRX( flavor, arrtype )                    \
static CvStatus CV_STDCALL                                      \
icvGray2BGRx_##flavor##_C1CnR( const arrtype* src, int srcstep, \
                       arrtype* dst, int dststep, CvSize size,  \
                       int dst_cn )                             \
{                                                               \
    int i;                                                      \
    srcstep /= sizeof(src[0]);                                  \
    dststep /= sizeof(src[0]);                                  \
    dststep -= size.width*dst_cn;                               \
                                                                \
    for( ; size.height--; src += srcstep, dst += dststep )      \
    {                                                           \
        if( dst_cn == 3 )                                       \
            for( i = 0; i < size.width; i++, dst += 3 )         \
                dst[0] = dst[1] = dst[2] = src[i];              \
        else                                                    \
            for( i = 0; i < size.width; i++, dst += 4 )         \
            {                                                   \
                dst[0] = dst[1] = dst[2] = src[i];              \
                dst[3] = 0;                                     \
            }                                                   \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


CV_IMPL_GRAY2BGRX( 8u, uchar )
CV_IMPL_GRAY2BGRX( 16u, ushort )
CV_IMPL_GRAY2BGRX( 32f, float )


static CvStatus CV_STDCALL
icvBGR5x52Gray_8u_C2C1R( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int green_bits )
{
    int i;
    assert( green_bits == 5 || green_bits == 6 );

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++ )
            {
                int t = ((ushort*)src)[i];
                t = ((t << 3) & 0xf8)*cscGb + ((t >> 3) & 0xfc)*cscGg +
                    ((t >> 8) & 0xf8)*cscGr;
                dst[i] = (uchar)CV_DESCALE(t,csc_shift);
            }
        else
            for( i = 0; i < size.width; i++ )
            {
                int t = ((ushort*)src)[i];
                t = ((t << 3) & 0xf8)*cscGb + ((t >> 2) & 0xf8)*cscGg +
                    ((t >> 7) & 0xf8)*cscGr;
                dst[i] = (uchar)CV_DESCALE(t,csc_shift);
            }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvGray2BGR5x5_8u_C1C2R( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, int green_bits )
{
    int i;
    assert( green_bits == 5 || green_bits == 6 );

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        if( green_bits == 6 )
            for( i = 0; i < size.width; i++ )
            {
                int t = src[i];
                ((ushort*)dst)[i] = (ushort)((t >> 3)|((t & ~3) << 3)|((t & ~7) << 8));
            }
        else
            for( i = 0; i < size.width; i++ )
            {
                int t = src[i] >> 3;
                ((ushort*)dst)[i] = (ushort)(t|(t << 5)|(t << 10));
            }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2Gray_8u_CnC1R( const uchar* src, int srcstep,
                       uchar* dst, int dststep, CvSize size,
                       int src_cn, int blue_idx )
{
    int i;
    srcstep -= size.width*src_cn;

    if( size.width*size.height >= 1024 )
    {
        int* tab = (int*)cvStackAlloc( 256*3*sizeof(tab[0]) );
        int r = 0, g = 0, b = (1 << (csc_shift-1));
    
        for( i = 0; i < 256; i++ )
        {
            tab[i] = b;
            tab[i+256] = g;
            tab[i+512] = r;
            g += cscGg;
            if( !blue_idx )
                b += cscGb, r += cscGr;
            else
                b += cscGr, r += cscGb;
        }

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t0 = tab[src[0]] + tab[src[1] + 256] + tab[src[2] + 512];
                dst[i] = (uchar)(t0 >> csc_shift);
            }
        }
    }
    else
    {
        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i++, src += src_cn )
            {
                int t0 = src[blue_idx]*cscGb + src[1]*cscGg + src[blue_idx^2]*cscGr;
                dst[i] = (uchar)CV_DESCALE(t0, csc_shift);
            }
        }
    }
    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2Gray_16u_CnC1R( const ushort* src, int srcstep,
                        ushort* dst, int dststep, CvSize size,
                        int src_cn, int blue_idx )
{
    int i;
    int cb = cscGb, cr = cscGr;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;

    if( blue_idx )
        cb = cscGr, cr = cscGb;

    for( ; size.height--; src += srcstep, dst += dststep )
        for( i = 0; i < size.width; i++, src += src_cn )
            dst[i] = (ushort)CV_DESCALE((unsigned)(src[0]*cb +
                    src[1]*cscGg + src[2]*cr), csc_shift);

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2Gray_32f_CnC1R( const float* src, int srcstep,
                        float* dst, int dststep, CvSize size,
                        int src_cn, int blue_idx )
{
    int i;
    float cb = cscGb_32f, cr = cscGr_32f;
    if( blue_idx )
        cb = cscGr_32f, cr = cscGb_32f;

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    for( ; size.height--; src += srcstep, dst += dststep )
        for( i = 0; i < size.width; i++, src += src_cn )
            dst[i] = src[0]*cb + src[1]*cscGg_32f + src[2]*cr;

    return CV_OK;
}


/****************************************************************************************\
*                                     RGB <-> YCrCb                                      *
\****************************************************************************************/

/* BGR/RGB -> YCrCb */
#define yuvYr_32f cscGr_32f
#define yuvYg_32f cscGg_32f
#define yuvYb_32f cscGb_32f
#define yuvCr_32f 0.713f
#define yuvCb_32f 0.564f

#define yuv_shift 14
#define yuvYr  fix(yuvYr_32f,yuv_shift)
#define yuvYg  fix(yuvYg_32f,yuv_shift)
#define yuvYb  fix(yuvYb_32f,yuv_shift)
#define yuvCr  fix(yuvCr_32f,yuv_shift)
#define yuvCb  fix(yuvCb_32f,yuv_shift)

#define yuv_descale(x)  CV_DESCALE((x), yuv_shift)
#define yuv_prescale(x) ((x) << yuv_shift)

#define  yuvRCr_32f   1.403f
#define  yuvGCr_32f   (-0.714f)
#define  yuvGCb_32f   (-0.344f)
#define  yuvBCb_32f   1.773f

#define  yuvRCr   fix(yuvRCr_32f,yuv_shift)
#define  yuvGCr   (-fix(-yuvGCr_32f,yuv_shift))
#define  yuvGCb   (-fix(-yuvGCb_32f,yuv_shift))
#define  yuvBCb   fix(yuvBCb_32f,yuv_shift)

#define CV_IMPL_BGRx2YCrCb( flavor, arrtype, worktype, scale_macro, cast_macro,     \
                            YUV_YB, YUV_YG, YUV_YR, YUV_CR, YUV_CB, YUV_Cx_BIAS )   \
static CvStatus CV_STDCALL                                                  \
icvBGRx2YCrCb_##flavor##_CnC3R( const arrtype* src, int srcstep,            \
    arrtype* dst, int dststep, CvSize size, int src_cn, int blue_idx )      \
{                                                                           \
    int i;                                                                  \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(src[0]);                                              \
    srcstep -= size.width*src_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, src += src_cn )                 \
        {                                                                   \
            worktype b = src[blue_idx], r = src[2^blue_idx], y;             \
            y = scale_macro(b*YUV_YB + src[1]*YUV_YG + r*YUV_YR);           \
            r = scale_macro((r - y)*YUV_CR) + YUV_Cx_BIAS;                  \
            b = scale_macro((b - y)*YUV_CB) + YUV_Cx_BIAS;                  \
            dst[i] = cast_macro(y);                                         \
            dst[i+1] = cast_macro(r);                                       \
            dst[i+2] = cast_macro(b);                                       \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


CV_IMPL_BGRx2YCrCb( 8u, uchar, int, yuv_descale, CV_CAST_8U,
                    yuvYb, yuvYg, yuvYr, yuvCr, yuvCb, 128 )

CV_IMPL_BGRx2YCrCb( 16u, ushort, int, yuv_descale, CV_CAST_16U,
                    yuvYb, yuvYg, yuvYr, yuvCr, yuvCb, 32768 )

CV_IMPL_BGRx2YCrCb( 32f, float, float, CV_NOP, CV_NOP,
                    yuvYb_32f, yuvYg_32f, yuvYr_32f, yuvCr_32f, yuvCb_32f, 0.5f )


#define CV_IMPL_YCrCb2BGRx( flavor, arrtype, worktype, prescale_macro,      \
    scale_macro, cast_macro, YUV_BCb, YUV_GCr, YUV_GCb, YUV_RCr, YUV_Cx_BIAS)\
static CvStatus CV_STDCALL                                                  \
icvYCrCb2BGRx_##flavor##_C3CnR( const arrtype* src, int srcstep,            \
                                arrtype* dst, int dststep, CvSize size,     \
                                int dst_cn, int blue_idx )                  \
{                                                                           \
    int i;                                                                  \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(src[0]);                                              \
    dststep -= size.width*dst_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, dst += dst_cn )                 \
        {                                                                   \
            worktype Y = prescale_macro(src[i]),                            \
                     Cr = src[i+1] - YUV_Cx_BIAS,                           \
                     Cb = src[i+2] - YUV_Cx_BIAS;                           \
            worktype b, g, r;                                               \
            b = scale_macro( Y + YUV_BCb*Cb );                              \
            g = scale_macro( Y + YUV_GCr*Cr + YUV_GCb*Cb );                 \
            r = scale_macro( Y + YUV_RCr*Cr );                              \
                                                                            \
            dst[blue_idx] = cast_macro(b);                                  \
            dst[1] = cast_macro(g);                                         \
            dst[blue_idx^2] = cast_macro(r);                                \
            if( dst_cn == 4 )                                               \
                dst[3] = 0;                                                 \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


CV_IMPL_YCrCb2BGRx( 8u, uchar, int, yuv_prescale, yuv_descale, CV_CAST_8U,
                    yuvBCb, yuvGCr, yuvGCb, yuvRCr, 128 )

CV_IMPL_YCrCb2BGRx( 16u, ushort, int, yuv_prescale, yuv_descale, CV_CAST_16U,
                    yuvBCb, yuvGCr, yuvGCb, yuvRCr, 32768 )

CV_IMPL_YCrCb2BGRx( 32f, float, float, CV_NOP, CV_NOP, CV_NOP,
                    yuvBCb_32f, yuvGCr_32f, yuvGCb_32f, yuvRCr_32f, 0.5f )


/****************************************************************************************\
*                                      RGB <-> XYZ                                       *
\****************************************************************************************/

#define xyzXr_32f  0.412453f
#define xyzXg_32f  0.357580f
#define xyzXb_32f  0.180423f

#define xyzYr_32f  0.212671f
#define xyzYg_32f  0.715160f
#define xyzYb_32f  0.072169f

#define xyzZr_32f  0.019334f
#define xyzZg_32f  0.119193f
#define xyzZb_32f  0.950227f

#define xyzRx_32f  3.240479f
#define xyzRy_32f  (-1.53715f)
#define xyzRz_32f  (-0.498535f)

#define xyzGx_32f  (-0.969256f)
#define xyzGy_32f  1.875991f
#define xyzGz_32f  0.041556f

#define xyzBx_32f  0.055648f
#define xyzBy_32f  (-0.204043f)
#define xyzBz_32f  1.057311f

#define xyz_shift  10
#define xyzXr_32s  fix(xyzXr_32f, xyz_shift )
#define xyzXg_32s  fix(xyzXg_32f, xyz_shift )
#define xyzXb_32s  fix(xyzXb_32f, xyz_shift )

#define xyzYr_32s  fix(xyzYr_32f, xyz_shift )
#define xyzYg_32s  fix(xyzYg_32f, xyz_shift )
#define xyzYb_32s  fix(xyzYb_32f, xyz_shift )

#define xyzZr_32s  fix(xyzZr_32f, xyz_shift )
#define xyzZg_32s  fix(xyzZg_32f, xyz_shift )
#define xyzZb_32s  fix(xyzZb_32f, xyz_shift )

#define xyzRx_32s  fix(3.240479f, xyz_shift )
#define xyzRy_32s  -fix(1.53715f, xyz_shift )
#define xyzRz_32s  -fix(0.498535f, xyz_shift )

#define xyzGx_32s  -fix(0.969256f, xyz_shift )
#define xyzGy_32s  fix(1.875991f, xyz_shift )
#define xyzGz_32s  fix(0.041556f, xyz_shift )

#define xyzBx_32s  fix(0.055648f, xyz_shift )
#define xyzBy_32s  -fix(0.204043f, xyz_shift )
#define xyzBz_32s  fix(1.057311f, xyz_shift )

#define xyz_descale(x) CV_DESCALE((x),xyz_shift)

#define CV_IMPL_BGRx2XYZ( flavor, arrtype, worktype,                        \
                          scale_macro, cast_macro, suffix )                 \
static CvStatus CV_STDCALL                                                  \
icvBGRx2XYZ_##flavor##_CnC3R( const arrtype* src, int srcstep,              \
                              arrtype* dst, int dststep, CvSize size,       \
                              int src_cn, int blue_idx )                    \
{                                                                           \
    int i;                                                                  \
    worktype t, matrix[] =                                                  \
    {                                                                       \
        xyzXb##suffix, xyzXg##suffix, xyzXr##suffix,                        \
        xyzYb##suffix, xyzYg##suffix, xyzYr##suffix,                        \
        xyzZb##suffix, xyzZg##suffix, xyzZr##suffix                         \
    };                                                                      \
                                                                            \
    if( icvRGB2XYZ_##flavor##_C3R_p )                                       \
        return icvBGRx2ABC_IPP_##flavor##_CnC3R( src, srcstep,              \
            dst, dststep, size, src_cn, blue_idx,                           \
            icvRGB2XYZ_##flavor##_C3R_p );                                  \
                                                                            \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(dst[0]);                                              \
    srcstep -= size.width*src_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    if( blue_idx )                                                          \
    {                                                                       \
        CV_SWAP( matrix[0], matrix[2], t );                                 \
        CV_SWAP( matrix[3], matrix[5], t );                                 \
        CV_SWAP( matrix[6], matrix[8], t );                                 \
    }                                                                       \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, src += src_cn )                 \
        {                                                                   \
            worktype x = scale_macro(src[0]*matrix[0] +                     \
                    src[1]*matrix[1] + src[2]*matrix[2]);                   \
            worktype y = scale_macro(src[0]*matrix[3] +                     \
                    src[1]*matrix[4] + src[2]*matrix[5]);                   \
            worktype z = scale_macro(src[0]*matrix[6] +                     \
                    src[1]*matrix[7] + src[2]*matrix[8]);                   \
                                                                            \
            dst[i] = (arrtype)(x);                                          \
            dst[i+1] = (arrtype)(y);                                        \
            dst[i+2] = cast_macro(z); /*sum of weights for z > 1*/          \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}


CV_IMPL_BGRx2XYZ( 8u, uchar, int, xyz_descale, CV_CAST_8U, _32s )
CV_IMPL_BGRx2XYZ( 16u, ushort, int, xyz_descale, CV_CAST_16U, _32s )
CV_IMPL_BGRx2XYZ( 32f, float, float, CV_NOP, CV_NOP, _32f )


#define CV_IMPL_XYZ2BGRx( flavor, arrtype, worktype, scale_macro,           \
                          cast_macro, suffix )                              \
static CvStatus CV_STDCALL                                                  \
icvXYZ2BGRx_##flavor##_C3CnR( const arrtype* src, int srcstep,              \
                              arrtype* dst, int dststep, CvSize size,       \
                              int dst_cn, int blue_idx )                    \
{                                                                           \
    int i;                                                                  \
    worktype t, matrix[] =                                                  \
    {                                                                       \
        xyzBx##suffix, xyzBy##suffix, xyzBz##suffix,                        \
        xyzGx##suffix, xyzGy##suffix, xyzGz##suffix,                        \
        xyzRx##suffix, xyzRy##suffix, xyzRz##suffix                         \
    };                                                                      \
                                                                            \
    if( icvXYZ2RGB_##flavor##_C3R_p )                                       \
        return icvABC2BGRx_IPP_##flavor##_C3CnR( src, srcstep,              \
            dst, dststep, size, dst_cn, blue_idx,                           \
            icvXYZ2RGB_##flavor##_C3R_p );                                  \
                                                                            \
    srcstep /= sizeof(src[0]);                                              \
    dststep /= sizeof(dst[0]);                                              \
    dststep -= size.width*dst_cn;                                           \
    size.width *= 3;                                                        \
                                                                            \
    if( blue_idx )                                                          \
    {                                                                       \
        CV_SWAP( matrix[0], matrix[6], t );                                 \
        CV_SWAP( matrix[1], matrix[7], t );                                 \
        CV_SWAP( matrix[2], matrix[8], t );                                 \
    }                                                                       \
                                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )                  \
    {                                                                       \
        for( i = 0; i < size.width; i += 3, dst += dst_cn )                 \
        {                                                                   \
            worktype b = scale_macro(src[i]*matrix[0] +                     \
                    src[i+1]*matrix[1] + src[i+2]*matrix[2]);               \
            worktype g = scale_macro(src[i]*matrix[3] +                     \
                    src[i+1]*matrix[4] + src[i+2]*matrix[5]);               \
            worktype r = scale_macro(src[i]*matrix[6] +                     \
                    src[i+1]*matrix[7] + src[i+2]*matrix[8]);               \
                                                                            \
            dst[0] = cast_macro(b);                                         \
            dst[1] = cast_macro(g);                                         \
            dst[2] = cast_macro(r);                                         \
                                                                            \
            if( dst_cn == 4 )                                               \
                dst[3] = 0;                                                 \
        }                                                                   \
    }                                                                       \
                                                                            \
    return CV_OK;                                                           \
}

CV_IMPL_XYZ2BGRx( 8u, uchar, int, xyz_descale, CV_CAST_8U, _32s )
CV_IMPL_XYZ2BGRx( 16u, ushort, int, xyz_descale, CV_CAST_16U, _32s )
CV_IMPL_XYZ2BGRx( 32f, float, float, CV_NOP, CV_NOP, _32f )


/****************************************************************************************\
*                          Non-linear Color Space Transformations                        *
\****************************************************************************************/

// driver color space conversion function for 8u arrays that uses 32f function
// with appropriate pre- and post-scaling.
static CvStatus CV_STDCALL
icvABC2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx, CvColorCvtFunc2 cvtfunc_32f,
                     const float* pre_coeffs, int postscale )
{
    int block_size = MIN(1 << 8, size.width);
    float* buffer = (float*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
    int i, di, k;
    CvStatus status = CV_OK;

    dststep -= size.width*dst_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += block_size )
        {
            const uchar* src1 = src + i*3;
            di = MIN(block_size, size.width - i);
            
            for( k = 0; k < di*3; k += 3 )
            {
                float a = CV_8TO32F(src1[k])*pre_coeffs[0] + pre_coeffs[1];
                float b = CV_8TO32F(src1[k+1])*pre_coeffs[2] + pre_coeffs[3];
                float c = CV_8TO32F(src1[k+2])*pre_coeffs[4] + pre_coeffs[5];
                buffer[k] = a;
                buffer[k+1] = b;
                buffer[k+2] = c;
            }

            status = cvtfunc_32f( buffer, 0, buffer, 0, cvSize(di,1), 3, blue_idx );
            if( status < 0 )
                return status;
            
            if( postscale )
            {
                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    int b = cvRound(buffer[k]*255.);
                    int g = cvRound(buffer[k+1]*255.);
                    int r = cvRound(buffer[k+2]*255.);

                    dst[0] = CV_CAST_8U(b);
                    dst[1] = CV_CAST_8U(g);
                    dst[2] = CV_CAST_8U(r);
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
            else
            {
                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    int b = cvRound(buffer[k]);
                    int g = cvRound(buffer[k+1]);
                    int r = cvRound(buffer[k+2]);

                    dst[0] = CV_CAST_8U(b);
                    dst[1] = CV_CAST_8U(g);
                    dst[2] = CV_CAST_8U(r);
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }
    }

    return CV_OK;
}


// driver color space conversion function for 8u arrays that uses 32f function
// with appropriate pre- and post-scaling.
static CvStatus CV_STDCALL
icvBGRx2ABC_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx, CvColorCvtFunc2 cvtfunc_32f,
                      int prescale, const float* post_coeffs )
{
    int block_size = MIN(1 << 8, size.width);
    float* buffer = (float*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
    int i, di, k;
    CvStatus status = CV_OK;

    srcstep -= size.width*src_cn;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += block_size )
        {
            uchar* dst1 = dst + i*3;
            di = MIN(block_size, size.width - i);

            if( prescale )
            {
                for( k = 0; k < di*3; k += 3, src += src_cn )
                {
                    float b = CV_8TO32F(src[0])*0.0039215686274509803f;
                    float g = CV_8TO32F(src[1])*0.0039215686274509803f;
                    float r = CV_8TO32F(src[2])*0.0039215686274509803f;

                    buffer[k] = b;
                    buffer[k+1] = g;
                    buffer[k+2] = r;
                }
            }
            else
            {
                for( k = 0; k < di*3; k += 3, src += src_cn )
                {
                    float b = CV_8TO32F(src[0]);
                    float g = CV_8TO32F(src[1]);
                    float r = CV_8TO32F(src[2]);

                    buffer[k] = b;
                    buffer[k+1] = g;
                    buffer[k+2] = r;
                }
            }
            
            status = cvtfunc_32f( buffer, 0, buffer, 0, cvSize(di,1), 3, blue_idx );
            if( status < 0 )
                return status;

            for( k = 0; k < di*3; k += 3 )
            {
                int a = cvRound( buffer[k]*post_coeffs[0] + post_coeffs[1] );
                int b = cvRound( buffer[k+1]*post_coeffs[2] + post_coeffs[3] );
                int c = cvRound( buffer[k+2]*post_coeffs[4] + post_coeffs[5] );
                dst1[k] = CV_CAST_8U(a);
                dst1[k+1] = CV_CAST_8U(b);
                dst1[k+2] = CV_CAST_8U(c);
            }
        }
    }

    return CV_OK;
}


/****************************************************************************************\
*                                      RGB <-> HSV                                       *
\****************************************************************************************/

static const uchar icvHue255To180[] =
{
      0,   1,   1,   2,   3,   4,   4,   5,   6,   6,   7,   8,   8,   9,  10,  11,
     11,  12,  13,  13,  14,  15,  16,  16,  17,  18,  18,  19,  20,  20,  21,  22,
     23,  23,  24,  25,  25,  26,  27,  28,  28,  29,  30,  30,  31,  32,  32,  33,
     34,  35,  35,  36,  37,  37,  38,  39,  40,  40,  41,  42,  42,  43,  44,  44,
     45,  46,  47,  47,  48,  49,  49,  50,  51,  52,  52,  53,  54,  54,  55,  56,
     56,  57,  58,  59,  59,  60,  61,  61,  62,  63,  64,  64,  65,  66,  66,  67,
     68,  68,  69,  70,  71,  71,  72,  73,  73,  74,  75,  76,  76,  77,  78,  78,
     79,  80,  80,  81,  82,  83,  83,  84,  85,  85,  86,  87,  88,  88,  89,  90,
     90,  91,  92,  92,  93,  94,  95,  95,  96,  97,  97,  98,  99, 100, 100, 101,
    102, 102, 103, 104, 104, 105, 106, 107, 107, 108, 109, 109, 110, 111, 112, 112,
    113, 114, 114, 115, 116, 116, 117, 118, 119, 119, 120, 121, 121, 122, 123, 124,
    124, 125, 126, 126, 127, 128, 128, 129, 130, 131, 131, 132, 133, 133, 134, 135,
    136, 136, 137, 138, 138, 139, 140, 140, 141, 142, 143, 143, 144, 145, 145, 146,
    147, 148, 148, 149, 150, 150, 151, 152, 152, 153, 154, 155, 155, 156, 157, 157,
    158, 159, 160, 160, 161, 162, 162, 163, 164, 164, 165, 166, 167, 167, 168, 169,
    169, 170, 171, 172, 172, 173, 174, 174, 175, 176, 176, 177, 178, 179, 179, 180
};


static const uchar icvHue180To255[] =
{
      0,   1,   3,   4,   6,   7,   9,  10,  11,  13,  14,  16,  17,  18,  20,  21,
     23,  24,  26,  27,  28,  30,  31,  33,  34,  35,  37,  38,  40,  41,  43,  44,
     45,  47,  48,  50,  51,  52,  54,  55,  57,  58,  60,  61,  62,  64,  65,  67,
     68,  69,  71,  72,  74,  75,  77,  78,  79,  81,  82,  84,  85,  86,  88,  89,
     91,  92,  94,  95,  96,  98,  99, 101, 102, 103, 105, 106, 108, 109, 111, 112,
    113, 115, 116, 118, 119, 120, 122, 123, 125, 126, 128, 129, 130, 132, 133, 135,
    136, 137, 139, 140, 142, 143, 145, 146, 147, 149, 150, 152, 153, 154, 156, 157,
    159, 160, 162, 163, 164, 166, 167, 169, 170, 171, 173, 174, 176, 177, 179, 180,
    181, 183, 184, 186, 187, 188, 190, 191, 193, 194, 196, 197, 198, 200, 201, 203,
    204, 205, 207, 208, 210, 211, 213, 214, 215, 217, 218, 220, 221, 222, 224, 225,
    227, 228, 230, 231, 232, 234, 235, 237, 238, 239, 241, 242, 244, 245, 247, 248,
    249, 251, 252, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
};


static CvStatus CV_STDCALL
icvBGRx2HSV_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    const int hsv_shift = 12;

    static const int div_table[] = {
        0, 1044480, 522240, 348160, 261120, 208896, 174080, 149211,
        130560, 116053, 104448, 94953, 87040, 80345, 74606, 69632,
        65280, 61440, 58027, 54973, 52224, 49737, 47476, 45412,
        43520, 41779, 40172, 38684, 37303, 36017, 34816, 33693,
        32640, 31651, 30720, 29842, 29013, 28229, 27486, 26782,
        26112, 25475, 24869, 24290, 23738, 23211, 22706, 22223,
        21760, 21316, 20890, 20480, 20086, 19707, 19342, 18991,
        18651, 18324, 18008, 17703, 17408, 17123, 16846, 16579,
        16320, 16069, 15825, 15589, 15360, 15137, 14921, 14711,
        14507, 14308, 14115, 13926, 13743, 13565, 13391, 13221,
        13056, 12895, 12738, 12584, 12434, 12288, 12145, 12006,
        11869, 11736, 11605, 11478, 11353, 11231, 11111, 10995,
        10880, 10768, 10658, 10550, 10445, 10341, 10240, 10141,
        10043, 9947, 9854, 9761, 9671, 9582, 9495, 9410,
        9326, 9243, 9162, 9082, 9004, 8927, 8852, 8777,
        8704, 8632, 8561, 8492, 8423, 8356, 8290, 8224,
        8160, 8097, 8034, 7973, 7913, 7853, 7795, 7737,
        7680, 7624, 7569, 7514, 7461, 7408, 7355, 7304,
        7253, 7203, 7154, 7105, 7057, 7010, 6963, 6917,
        6872, 6827, 6782, 6739, 6695, 6653, 6611, 6569,
        6528, 6487, 6447, 6408, 6369, 6330, 6292, 6254,
        6217, 6180, 6144, 6108, 6073, 6037, 6003, 5968,
        5935, 5901, 5868, 5835, 5803, 5771, 5739, 5708,
        5677, 5646, 5615, 5585, 5556, 5526, 5497, 5468,
        5440, 5412, 5384, 5356, 5329, 5302, 5275, 5249,
        5222, 5196, 5171, 5145, 5120, 5095, 5070, 5046,
        5022, 4998, 4974, 4950, 4927, 4904, 4881, 4858,
        4836, 4813, 4791, 4769, 4748, 4726, 4705, 4684,
        4663, 4642, 4622, 4601, 4581, 4561, 4541, 4522,
        4502, 4483, 4464, 4445, 4426, 4407, 4389, 4370,
        4352, 4334, 4316, 4298, 4281, 4263, 4246, 4229,
        4212, 4195, 4178, 4161, 4145, 4128, 4112, 4096
    };

    int i;
    if( icvRGB2HSV_8u_C3R_p )
    {
        CvStatus status = icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                                src_cn, blue_idx, icvRGB2HSV_8u_C3R_p );
        if( status >= 0 )
        {
            size.width *= 3;
            for( ; size.height--; dst += dststep )
            {
                for( i = 0; i <= size.width - 12; i += 12 )
                {
                    uchar t0 = icvHue255To180[dst[i]], t1 = icvHue255To180[dst[i+3]];
                    dst[i] = t0; dst[i+3] = t1;
                    t0 = icvHue255To180[dst[i+6]]; t1 = icvHue255To180[dst[i+9]];
                    dst[i+6] = t0; dst[i+9] = t1;
                }
                for( ; i < size.width; i += 3 )
                    dst[i] = icvHue255To180[dst[i]];
            }
        }
        return status;
    }

    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            int b = (src)[blue_idx], g = (src)[1], r = (src)[2^blue_idx];
            int h, s, v = b;
            int vmin = b, diff;
            int vr, vg;

            CV_CALC_MAX_8U( v, g );
            CV_CALC_MAX_8U( v, r );
            CV_CALC_MIN_8U( vmin, g );
            CV_CALC_MIN_8U( vmin, r );

            diff = v - vmin;
            vr = v == r ? -1 : 0;
            vg = v == g ? -1 : 0;

            s = diff * div_table[v] >> hsv_shift;
            h = (vr & (g - b)) +
                (~vr & ((vg & (b - r + 2 * diff)) + ((~vg) & (r - g + 4 * diff))));
            h = ((h * div_table[diff] * 15 + (1 << (hsv_shift + 6))) >> (7 + hsv_shift))\
                + (h < 0 ? 30*6 : 0);

            dst[i] = (uchar)h;
            dst[i+1] = (uchar)s;
            dst[i+2] = (uchar)v;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2HSV_32f_CnC3R( const float* src, int srcstep,
                       float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float h, s, v;

            float vmin, diff;

            v = vmin = r;
            if( v < g ) v = g;
            if( v < b ) v = b;
            if( vmin > g ) vmin = g;
            if( vmin > b ) vmin = b;

            diff = v - vmin;
            s = diff/(float)(fabs(v) + FLT_EPSILON);
            diff = (float)(60./(diff + FLT_EPSILON));
            if( v == r )
                h = (g - b)*diff;
            else if( v == g )
                h = (b - r)*diff + 120.f;
            else
                h = (r - g)*diff + 240.f;

            if( h < 0 ) h += 360.f;

            dst[i] = h;
            dst[i+1] = s;
            dst[i+2] = v;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvHSV2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst,
                       int dststep, CvSize size, int dst_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float h = src[i], s = src[i+1], v = src[i+2];
            float b, g, r;

            if( s == 0 )
                b = g = r = v;
            else
            {
                static const int sector_data[][3]=
                    {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};
                float tab[4];
                int sector;
                h *= 0.016666666666666666f; // h /= 60;
                if( h < 0 )
                    do h += 6; while( h < 0 );
                else if( h >= 6 )
                    do h -= 6; while( h >= 6 );
                sector = cvFloor(h);
                h -= sector;

                tab[0] = v;
                tab[1] = v*(1.f - s);
                tab[2] = v*(1.f - s*h);
                tab[3] = v*(1.f - s*(1.f - h));
                
                b = tab[sector_data[sector][0]];
                g = tab[sector_data[sector][1]];
                r = tab[sector_data[sector][2]];
            }

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvHSV2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    static const float pre_coeffs[] = { 2.f, 0.f, 0.0039215686274509803f, 0.f, 1.f, 0.f };

    if( icvHSV2RGB_8u_C3R_p )
    {
        int block_size = MIN(1 << 14, size.width);
        uchar* buffer;
        int i, di, k;
        CvStatus status = CV_OK;

        buffer = (uchar*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
        dststep -= size.width*dst_cn;

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i += block_size )
            {
                const uchar* src1 = src + i*3;
                di = MIN(block_size, size.width - i);
                for( k = 0; k < di*3; k += 3 )
                {
                    uchar h = icvHue180To255[src1[k]];
                    uchar s = src1[k+1];
                    uchar v = src1[k+2];
                    buffer[k] = h;
                    buffer[k+1] = s;
                    buffer[k+2] = v;
                }

                status = icvHSV2RGB_8u_C3R_p( buffer, di*3,
                                buffer, di*3, cvSize(di,1) );
                if( status < 0 )
                    return status;

                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    uchar r = buffer[k];
                    uchar g = buffer[k+1];
                    uchar b = buffer[k+2];
                    dst[blue_idx] = b;
                    dst[1] = g;
                    dst[blue_idx^2] = r;
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }

        return CV_OK;
    }

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvHSV2BGRx_32f_C3CnR, pre_coeffs, 0 );
}


/****************************************************************************************\
*                                     RGB <-> HLS                                        *
\****************************************************************************************/

static CvStatus CV_STDCALL
icvBGRx2HLS_32f_CnC3R( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;

    if( icvRGB2HLS_32f_C3R_p )
    {
        CvStatus status = icvBGRx2ABC_IPP_32f_CnC3R( src, srcstep, dst, dststep, size,
                                                     src_cn, blue_idx, icvRGB2HLS_32f_C3R_p );
        if( status >= 0 )
        {
            size.width *= 3;
            dststep /= sizeof(dst[0]);

            for( ; size.height--; dst += dststep )
            {
                for( i = 0; i <= size.width - 12; i += 12 )
                {
                    float t0 = dst[i]*360.f, t1 = dst[i+3]*360.f;
                    dst[i] = t0; dst[i+3] = t1;
                    t0 = dst[i+6]*360.f; t1 = dst[i+9]*360.f;
                    dst[i+6] = t0; dst[i+9] = t1;
                }
                for( ; i < size.width; i += 3 )
                    dst[i] = dst[i]*360.f;
            }
        }
        return status;
    }

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float h = 0.f, s = 0.f, l;
            float vmin, vmax, diff;

            vmax = vmin = r;
            if( vmax < g ) vmax = g;
            if( vmax < b ) vmax = b;
            if( vmin > g ) vmin = g;
            if( vmin > b ) vmin = b;

            diff = vmax - vmin;
            l = (vmax + vmin)*0.5f;

            if( diff > FLT_EPSILON )
            {
                s = l < 0.5f ? diff/(vmax + vmin) : diff/(2 - vmax - vmin);
                diff = 60.f/diff;

                if( vmax == r )
                    h = (g - b)*diff;
                else if( vmax == g )
                    h = (b - r)*diff + 120.f;
                else
                    h = (r - g)*diff + 240.f;

                if( h < 0.f ) h += 360.f;
            }

            dst[i] = h;
            dst[i+1] = l;
            dst[i+2] = s;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvHLS2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int dst_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);

    if( icvHLS2RGB_32f_C3R_p )
    {
        int block_size = MIN(1 << 10, size.width);
        float* buffer;
        int di, k;
        CvStatus status = CV_OK;

        buffer = (float*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
        dststep -= size.width*dst_cn;

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i += block_size )
            {
                const float* src1 = src + i*3;
                di = MIN(block_size, size.width - i);
                for( k = 0; k < di*3; k += 3 )
                {
                    float h = src1[k]*0.0027777777777777779f; // /360.
                    float s = src1[k+1], v = src1[k+2];
                    buffer[k] = h; buffer[k+1] = s; buffer[k+2] = v;
                }

                status = icvHLS2RGB_32f_C3R_p( buffer, di*3*sizeof(dst[0]),
                                buffer, di*3*sizeof(dst[0]), cvSize(di,1) );
                if( status < 0 )
                    return status;

                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    float r = buffer[k], g = buffer[k+1], b = buffer[k+2];
                    dst[blue_idx] = b; dst[1] = g; dst[blue_idx^2] = r;
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }

        return CV_OK;
    }
    
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float h = src[i], l = src[i+1], s = src[i+2];
            float b, g, r;

            if( s == 0 )
                b = g = r = l;
            else
            {
                static const int sector_data[][3]=
                    {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};
                float tab[4];
                int sector;
                
                float p2 = l <= 0.5f ? l*(1 + s) : l + s - l*s;
                float p1 = 2*l - p2;

                h *= 0.016666666666666666f; // h /= 60;
                if( h < 0 )
                    do h += 6; while( h < 0 );
                else if( h >= 6 )
                    do h -= 6; while( h >= 6 );

                assert( 0 <= h && h < 6 );
                sector = cvFloor(h);
                h -= sector;

                tab[0] = p2;
                tab[1] = p1;
                tab[2] = p1 + (p2 - p1)*(1-h);
                tab[3] = p1 + (p2 - p1)*h;

                b = tab[sector_data[sector][0]];
                g = tab[sector_data[sector][1]];
                r = tab[sector_data[sector][2]];
            }

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0;
        }
    }

    return CV_OK;
}

static CvStatus CV_STDCALL
icvBGRx2HLS_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    static const float post_coeffs[] = { 0.5f, 0.f, 255.f, 0.f, 255.f, 0.f };

    if( icvRGB2HLS_8u_C3R_p )
    {
        CvStatus status = icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                                    src_cn, blue_idx, icvRGB2HLS_8u_C3R_p );
        if( status >= 0 )
        {
            size.width *= 3;
            for( ; size.height--; dst += dststep )
            {
                int i;
                for( i = 0; i <= size.width - 12; i += 12 )
                {
                    uchar t0 = icvHue255To180[dst[i]], t1 = icvHue255To180[dst[i+3]];
                    dst[i] = t0; dst[i+3] = t1;
                    t0 = icvHue255To180[dst[i+6]]; t1 = icvHue255To180[dst[i+9]];
                    dst[i+6] = t0; dst[i+9] = t1;
                }
                for( ; i < size.width; i += 3 )
                    dst[i] = icvHue255To180[dst[i]];
            }
        }
        return status;
    }

    return icvBGRx2ABC_8u_CnC3R( src, srcstep, dst, dststep, size, src_cn, blue_idx,
                                 (CvColorCvtFunc2)icvBGRx2HLS_32f_CnC3R, 1, post_coeffs );
}


static CvStatus CV_STDCALL
icvHLS2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    static const float pre_coeffs[] = { 2.f, 0.f, 0.0039215686274509803f, 0.f,
                                        0.0039215686274509803f, 0.f };

    if( icvHLS2RGB_8u_C3R_p )
    {
        int block_size = MIN(1 << 14, size.width);
        uchar* buffer;
        int i, di, k;
        CvStatus status = CV_OK;

        buffer = (uchar*)cvStackAlloc( block_size*3*sizeof(buffer[0]) );
        dststep -= size.width*dst_cn;

        for( ; size.height--; src += srcstep, dst += dststep )
        {
            for( i = 0; i < size.width; i += block_size )
            {
                const uchar* src1 = src + i*3;
                di = MIN(block_size, size.width - i);
                for( k = 0; k < di*3; k += 3 )
                {
                    uchar h = icvHue180To255[src1[k]];
                    uchar l = src1[k+1];
                    uchar s = src1[k+2];
                    buffer[k] = h;
                    buffer[k+1] = l;
                    buffer[k+2] = s;
                }

                status = icvHLS2RGB_8u_C3R_p( buffer, di*3,
                                buffer, di*3, cvSize(di,1) );
                if( status < 0 )
                    return status;

                for( k = 0; k < di*3; k += 3, dst += dst_cn )
                {
                    uchar r = buffer[k];
                    uchar g = buffer[k+1];
                    uchar b = buffer[k+2];
                    dst[blue_idx] = b;
                    dst[1] = g;
                    dst[blue_idx^2] = r;
                    if( dst_cn == 4 )
                        dst[3] = 0;
                }
            }
        }

        return CV_OK;
    }

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvHLS2BGRx_32f_C3CnR, pre_coeffs, 1 );
}


/****************************************************************************************\
*                                     RGB <-> L*a*b*                                     *
\****************************************************************************************/

#define  labXr_32f  0.433953f /* = xyzXr_32f / 0.950456 */
#define  labXg_32f  0.376219f /* = xyzXg_32f / 0.950456 */
#define  labXb_32f  0.189828f /* = xyzXb_32f / 0.950456 */

#define  labYr_32f  0.212671f /* = xyzYr_32f */
#define  labYg_32f  0.715160f /* = xyzYg_32f */ 
#define  labYb_32f  0.072169f /* = xyzYb_32f */ 

#define  labZr_32f  0.017758f /* = xyzZr_32f / 1.088754 */
#define  labZg_32f  0.109477f /* = xyzZg_32f / 1.088754 */
#define  labZb_32f  0.872766f /* = xyzZb_32f / 1.088754 */

#define  labRx_32f  3.0799327f  /* = xyzRx_32f * 0.950456 */
#define  labRy_32f  (-1.53715f) /* = xyzRy_32f */
#define  labRz_32f  (-0.542782f)/* = xyzRz_32f * 1.088754 */

#define  labGx_32f  (-0.921235f)/* = xyzGx_32f * 0.950456 */
#define  labGy_32f  1.875991f   /* = xyzGy_32f */ 
#define  labGz_32f  0.04524426f /* = xyzGz_32f * 1.088754 */

#define  labBx_32f  0.0528909755f /* = xyzBx_32f * 0.950456 */
#define  labBy_32f  (-0.204043f)  /* = xyzBy_32f */
#define  labBz_32f  1.15115158f   /* = xyzBz_32f * 1.088754 */

#define  labT_32f   0.008856f

#define labT   fix(labT_32f*255,lab_shift)

#undef lab_shift
#define lab_shift 10
#define labXr  fix(labXr_32f,lab_shift)
#define labXg  fix(labXg_32f,lab_shift)
#define labXb  fix(labXb_32f,lab_shift)
                            
#define labYr  fix(labYr_32f,lab_shift)
#define labYg  fix(labYg_32f,lab_shift)
#define labYb  fix(labYb_32f,lab_shift)
                            
#define labZr  fix(labZr_32f,lab_shift)
#define labZg  fix(labZg_32f,lab_shift)
#define labZb  fix(labZb_32f,lab_shift)

#define labSmallScale_32f  7.787f
#define labSmallShift_32f  0.13793103448275862f  /* 16/116 */
#define labLScale_32f      116.f
#define labLShift_32f      16.f
#define labLScale2_32f     903.3f

#define labSmallScale fix(31.27 /* labSmallScale_32f*(1<<lab_shift)/255 */,lab_shift)
#define labSmallShift fix(141.24138 /* labSmallScale_32f*(1<<lab) */,lab_shift)
#define labLScale fix(295.8 /* labLScale_32f*255/100 */,lab_shift)
#define labLShift fix(41779.2 /* labLShift_32f*1024*255/100 */,lab_shift)
#define labLScale2 fix(labLScale2_32f*0.01,lab_shift)

/* 1024*(([0..511]./255)**(1./3)) */
static ushort icvLabCubeRootTab[] = {
       0,  161,  203,  232,  256,  276,  293,  308,  322,  335,  347,  359,  369,  379,  389,  398,
     406,  415,  423,  430,  438,  445,  452,  459,  465,  472,  478,  484,  490,  496,  501,  507,
     512,  517,  523,  528,  533,  538,  542,  547,  552,  556,  561,  565,  570,  574,  578,  582,
     586,  590,  594,  598,  602,  606,  610,  614,  617,  621,  625,  628,  632,  635,  639,  642,
     645,  649,  652,  655,  659,  662,  665,  668,  671,  674,  677,  680,  684,  686,  689,  692,
     695,  698,  701,  704,  707,  710,  712,  715,  718,  720,  723,  726,  728,  731,  734,  736,
     739,  741,  744,  747,  749,  752,  754,  756,  759,  761,  764,  766,  769,  771,  773,  776,
     778,  780,  782,  785,  787,  789,  792,  794,  796,  798,  800,  803,  805,  807,  809,  811,
     813,  815,  818,  820,  822,  824,  826,  828,  830,  832,  834,  836,  838,  840,  842,  844,
     846,  848,  850,  852,  854,  856,  857,  859,  861,  863,  865,  867,  869,  871,  872,  874,
     876,  878,  880,  882,  883,  885,  887,  889,  891,  892,  894,  896,  898,  899,  901,  903,
     904,  906,  908,  910,  911,  913,  915,  916,  918,  920,  921,  923,  925,  926,  928,  929,
     931,  933,  934,  936,  938,  939,  941,  942,  944,  945,  947,  949,  950,  952,  953,  955,
     956,  958,  959,  961,  962,  964,  965,  967,  968,  970,  971,  973,  974,  976,  977,  979,
     980,  982,  983,  985,  986,  987,  989,  990,  992,  993,  995,  996,  997,  999, 1000, 1002,
    1003, 1004, 1006, 1007, 1009, 1010, 1011, 1013, 1014, 1015, 1017, 1018, 1019, 1021, 1022, 1024,
    1025, 1026, 1028, 1029, 1030, 1031, 1033, 1034, 1035, 1037, 1038, 1039, 1041, 1042, 1043, 1044,
    1046, 1047, 1048, 1050, 1051, 1052, 1053, 1055, 1056, 1057, 1058, 1060, 1061, 1062, 1063, 1065,
    1066, 1067, 1068, 1070, 1071, 1072, 1073, 1074, 1076, 1077, 1078, 1079, 1081, 1082, 1083, 1084,
    1085, 1086, 1088, 1089, 1090, 1091, 1092, 1094, 1095, 1096, 1097, 1098, 1099, 1101, 1102, 1103,
    1104, 1105, 1106, 1107, 1109, 1110, 1111, 1112, 1113, 1114, 1115, 1117, 1118, 1119, 1120, 1121,
    1122, 1123, 1124, 1125, 1127, 1128, 1129, 1130, 1131, 1132, 1133, 1134, 1135, 1136, 1138, 1139,
    1140, 1141, 1142, 1143, 1144, 1145, 1146, 1147, 1148, 1149, 1150, 1151, 1152, 1154, 1155, 1156,
    1157, 1158, 1159, 1160, 1161, 1162, 1163, 1164, 1165, 1166, 1167, 1168, 1169, 1170, 1171, 1172,
    1173, 1174, 1175, 1176, 1177, 1178, 1179, 1180, 1181, 1182, 1183, 1184, 1185, 1186, 1187, 1188,
    1189, 1190, 1191, 1192, 1193, 1194, 1195, 1196, 1197, 1198, 1199, 1200, 1201, 1202, 1203, 1204,
    1205, 1206, 1207, 1208, 1209, 1210, 1211, 1212, 1213, 1214, 1215, 1215, 1216, 1217, 1218, 1219,
    1220, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1229, 1230, 1230, 1231, 1232, 1233, 1234,
    1235, 1236, 1237, 1238, 1239, 1240, 1241, 1242, 1242, 1243, 1244, 1245, 1246, 1247, 1248, 1249,
    1250, 1251, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258, 1259, 1259, 1260, 1261, 1262, 1263,
    1264, 1265, 1266, 1266, 1267, 1268, 1269, 1270, 1271, 1272, 1273, 1273, 1274, 1275, 1276, 1277,
    1278, 1279, 1279, 1280, 1281, 1282, 1283, 1284, 1285, 1285, 1286, 1287, 1288, 1289, 1290, 1291
};


static CvStatus CV_STDCALL
icvBGRx2Lab_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    int i;

    /*if( icvBGR2Lab_8u_C3R_p )
        return icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                         src_cn, blue_idx^2, icvBGR2Lab_8u_C3R_p );*/

    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            int b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            int x, y, z, f;
            int L, a;

            x = b*labXb + g*labXg + r*labXr;
            y = b*labYb + g*labYg + r*labYr;
            z = b*labZb + g*labZg + r*labZr;

            f = x > labT;
            x = CV_DESCALE( x, lab_shift );

            if( f )
                assert( (unsigned)x < 512 ), x = icvLabCubeRootTab[x];
            else
                x = CV_DESCALE(x*labSmallScale + labSmallShift,lab_shift);

            f = z > labT;
            z = CV_DESCALE( z, lab_shift );

            if( f )
                assert( (unsigned)z < 512 ), z = icvLabCubeRootTab[z];
            else
                z = CV_DESCALE(z*labSmallScale + labSmallShift,lab_shift);

            f = y > labT;
            y = CV_DESCALE( y, lab_shift );

            if( f )
            {
                assert( (unsigned)y < 512 ), y = icvLabCubeRootTab[y];
                L = CV_DESCALE(y*labLScale - labLShift, 2*lab_shift );
            }
            else
            {
                L = CV_DESCALE(y*labLScale2,lab_shift);
                y = CV_DESCALE(y*labSmallScale + labSmallShift,lab_shift);
            }

            a = CV_DESCALE( 500*(x - y), lab_shift ) + 128;
            b = CV_DESCALE( 200*(y - z), lab_shift ) + 128;

            dst[i] = CV_CAST_8U(L);
            dst[i+1] = CV_CAST_8U(a);
            dst[i+2] = CV_CAST_8U(b);
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2Lab_32f_CnC3R( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float x, y, z;
            float L, a;

            x = b*labXb_32f + g*labXg_32f + r*labXr_32f;
            y = b*labYb_32f + g*labYg_32f + r*labYr_32f;
            z = b*labZb_32f + g*labZg_32f + r*labZr_32f;

            if( x > labT_32f )
                x = cvCbrt(x);
            else
                x = x*labSmallScale_32f + labSmallShift_32f;

            if( z > labT_32f )
                z = cvCbrt(z);
            else
                z = z*labSmallScale_32f + labSmallShift_32f;

            if( y > labT_32f )
            {
                y = cvCbrt(y);
                L = y*labLScale_32f - labLShift_32f;
            }
            else
            {
                L = y*labLScale2_32f;
                y = y*labSmallScale_32f + labSmallShift_32f;
            }

            a = 500.f*(x - y);
            b = 200.f*(y - z);

            dst[i] = L;
            dst[i+1] = a;
            dst[i+2] = b;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvLab2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int dst_cn, int blue_idx )
{
    int i;
    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float L = src[i], a = src[i+1], b = src[i+2];
            float x, y, z;
            float g, r;

            L = (L + labLShift_32f)*(1.f/labLScale_32f);
            x = (L + a*0.002f);
            z = (L - b*0.005f);
            y = L*L*L;
            x = x*x*x;
            z = z*z*z;

            b = x*labBx_32f + y*labBy_32f + z*labBz_32f;
            g = x*labGx_32f + y*labGy_32f + z*labGz_32f;
            r = x*labRx_32f + y*labRy_32f + z*labRz_32f;

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvLab2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    // L: [0..255] -> [0..100]
    // a: [0..255] -> [-128..127]
    // b: [0..255] -> [-128..127]
    static const float pre_coeffs[] = { 0.39215686274509809f, 0.f, 1.f, -128.f, 1.f, -128.f };

    if( icvLab2BGR_8u_C3R_p )
        return icvABC2BGRx_IPP_8u_C3CnR( src, srcstep, dst, dststep, size,
                                         dst_cn, blue_idx^2, icvLab2BGR_8u_C3R_p );

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvLab2BGRx_32f_C3CnR, pre_coeffs, 1 );
}


/****************************************************************************************\
*                                     RGB <-> L*u*v*                                     *
\****************************************************************************************/

#define luvUn_32f  0.19793943f 
#define luvVn_32f  0.46831096f 
#define luvYmin_32f  0.05882353f /* 15/255 */

static CvStatus CV_STDCALL
icvBGRx2Luv_32f_CnC3R( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int src_cn, int blue_idx )
{
    int i;

    /*if( icvRGB2Luv_32f_C3R_p )
        return icvBGRx2ABC_IPP_32f_CnC3R( src, srcstep, dst, dststep, size,
                                          src_cn, blue_idx, icvRGB2Luv_32f_C3R_p );*/

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    srcstep -= size.width*src_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, src += src_cn )
        {
            float b = src[blue_idx], g = src[1], r = src[2^blue_idx];
            float x, y, z;
            float L, u, v, t;

            x = b*xyzXb_32f + g*xyzXg_32f + r*xyzXr_32f;
            y = b*xyzYb_32f + g*xyzYg_32f + r*xyzYr_32f;
            z = b*xyzZb_32f + g*xyzZg_32f + r*xyzZr_32f;

            if( !x && !y && !z )
                L = u = v = 0.f;
            else
            {
                if( y > labT_32f )
                    L = labLScale_32f * cvCbrt(y) - labLShift_32f;
                else
                    L = labLScale2_32f * y;

                t = 1.f / (x + 15 * y + 3 * z);            
                u = 4.0f * x * t;
                v = 9.0f * y * t;

                u = 13*L*(u - luvUn_32f);
                v = 13*L*(v - luvVn_32f);
            }

            dst[i] = L;
            dst[i+1] = u;
            dst[i+2] = v;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvLuv2BGRx_32f_C3CnR( const float* src, int srcstep, float* dst, int dststep,
                       CvSize size, int dst_cn, int blue_idx )
{
    int i;

    /*if( icvLuv2RGB_32f_C3R_p )
        return icvABC2BGRx_IPP_32f_C3CnR( src, srcstep, dst, dststep, size,
                                          dst_cn, blue_idx, icvLuv2RGB_32f_C3R_p );*/

    srcstep /= sizeof(src[0]);
    dststep /= sizeof(dst[0]);
    dststep -= size.width*dst_cn;
    size.width *= 3;

    for( ; size.height--; src += srcstep, dst += dststep )
    {
        for( i = 0; i < size.width; i += 3, dst += dst_cn )
        {
            float L = src[i], u = src[i+1], v = src[i+2];
            float x, y, z, t, u1, v1, b, g, r;

            if( L >= 8 ) 
            {
                t = (L + labLShift_32f) * (1.f/labLScale_32f);
                y = t*t*t;
            }
            else
            {
                y = L * (1.f/labLScale2_32f);
                L = MAX( L, 0.001f );
            }

            t = 1.f/(13.f * L);
            u1 = u*t + luvUn_32f;
            v1 = v*t + luvVn_32f;
            x = 2.25f * u1 * y / v1 ;
            z = (12 - 3 * u1 - 20 * v1) * y / (4 * v1);                
                       
            b = xyzBx_32f*x + xyzBy_32f*y + xyzBz_32f*z;
            g = xyzGx_32f*x + xyzGy_32f*y + xyzGz_32f*z;
            r = xyzRx_32f*x + xyzRy_32f*y + xyzRz_32f*z;

            dst[blue_idx] = b;
            dst[1] = g;
            dst[blue_idx^2] = r;
            if( dst_cn == 4 )
                dst[3] = 0.f;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvBGRx2Luv_8u_CnC3R( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int src_cn, int blue_idx )
{
    // L: [0..100] -> [0..255]
    // u: [-134..220] -> [0..255]
    // v: [-140..122] -> [0..255]
    //static const float post_coeffs[] = { 2.55f, 0.f, 1.f, 83.f, 1.f, 140.f };
    static const float post_coeffs[] = { 2.55f, 0.f, 0.72033898305084743f, 96.525423728813564f,
                                         0.99609375f, 139.453125f };

    if( icvRGB2Luv_8u_C3R_p )
        return icvBGRx2ABC_IPP_8u_CnC3R( src, srcstep, dst, dststep, size,
                                         src_cn, blue_idx, icvRGB2Luv_8u_C3R_p );

    return icvBGRx2ABC_8u_CnC3R( src, srcstep, dst, dststep, size, src_cn, blue_idx,
                                 (CvColorCvtFunc2)icvBGRx2Luv_32f_CnC3R, 1, post_coeffs );
}


static CvStatus CV_STDCALL
icvLuv2BGRx_8u_C3CnR( const uchar* src, int srcstep, uchar* dst, int dststep,
                      CvSize size, int dst_cn, int blue_idx )
{
    // L: [0..255] -> [0..100]
    // u: [0..255] -> [-134..220]
    // v: [0..255] -> [-140..122]
    static const float pre_coeffs[] = { 0.39215686274509809f, 0.f, 1.388235294117647f, -134.f,
                                        1.003921568627451f, -140.f };

    if( icvLuv2RGB_8u_C3R_p )
        return icvABC2BGRx_IPP_8u_C3CnR( src, srcstep, dst, dststep, size,
                                         dst_cn, blue_idx, icvLuv2RGB_8u_C3R_p );

    return icvABC2BGRx_8u_C3CnR( src, srcstep, dst, dststep, size, dst_cn, blue_idx,
                                 (CvColorCvtFunc2)icvLuv2BGRx_32f_C3CnR, pre_coeffs, 1 );
}

/****************************************************************************************\
*                            Bayer Pattern -> RGB conversion                             *
\****************************************************************************************/

static CvStatus CV_STDCALL
icvBayer2BGR_8u_C1C3R( const uchar* bayer0, int bayer_step,
                       uchar *dst0, int dst_step,
                       CvSize size, int code )
{
    int blue = code == CV_BayerBG2BGR || code == CV_BayerGB2BGR ? -1 : 1;
    int start_with_green = code == CV_BayerGB2BGR || code == CV_BayerGR2BGR;

    memset( dst0, 0, size.width*3*sizeof(dst0[0]) );
    memset( dst0 + (size.height - 1)*dst_step, 0, size.width*3*sizeof(dst0[0]) );
    dst0 += dst_step + 3 + 1;
    size.height -= 2;
    size.width -= 2;

    for( ; size.height-- > 0; bayer0 += bayer_step, dst0 += dst_step )
    {
        int t0, t1;
        const uchar* bayer = bayer0;
        uchar* dst = dst0;
        const uchar* bayer_end = bayer + size.width;

        dst[-4] = dst[-3] = dst[-2] = dst[size.width*3-1] =
            dst[size.width*3] = dst[size.width*3+1] = 0;

        if( size.width <= 0 )
            continue;

        if( start_with_green )
        {
            t0 = (bayer[1] + bayer[bayer_step*2+1] + 1) >> 1;
            t1 = (bayer[bayer_step] + bayer[bayer_step+2] + 1) >> 1;
            dst[-blue] = (uchar)t0;
            dst[0] = bayer[bayer_step+1];
            dst[blue] = (uchar)t1;
            bayer++;
            dst += 3;
        }

        if( blue > 0 )
        {
            for( ; bayer <= bayer_end - 2; bayer += 2, dst += 6 )
            {
                t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                      bayer[bayer_step*2+2] + 2) >> 2;
                t1 = (bayer[1] + bayer[bayer_step] +
                      bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
                dst[-1] = (uchar)t0;
                dst[0] = (uchar)t1;
                dst[1] = bayer[bayer_step+1];

                t0 = (bayer[2] + bayer[bayer_step*2+2] + 1) >> 1;
                t1 = (bayer[bayer_step+1] + bayer[bayer_step+3] + 1) >> 1;
                dst[2] = (uchar)t0;
                dst[3] = bayer[bayer_step+2];
                dst[4] = (uchar)t1;
            }
        }
        else
        {
            for( ; bayer <= bayer_end - 2; bayer += 2, dst += 6 )
            {
                t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                      bayer[bayer_step*2+2] + 2) >> 2;
                t1 = (bayer[1] + bayer[bayer_step] +
                      bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
                dst[1] = (uchar)t0;
                dst[0] = (uchar)t1;
                dst[-1] = bayer[bayer_step+1];

                t0 = (bayer[2] + bayer[bayer_step*2+2] + 1) >> 1;
                t1 = (bayer[bayer_step+1] + bayer[bayer_step+3] + 1) >> 1;
                dst[4] = (uchar)t0;
                dst[3] = bayer[bayer_step+2];
                dst[2] = (uchar)t1;
            }
        }

        if( bayer < bayer_end )
        {
            t0 = (bayer[0] + bayer[2] + bayer[bayer_step*2] +
                  bayer[bayer_step*2+2] + 2) >> 2;
            t1 = (bayer[1] + bayer[bayer_step] +
                  bayer[bayer_step+2] + bayer[bayer_step*2+1]+2) >> 2;
            dst[-blue] = (uchar)t0;
            dst[0] = (uchar)t1;
            dst[blue] = bayer[bayer_step+1];
            bayer++;
            dst += 3;
        }

        blue = -blue;
        start_with_green = !start_with_green;
    }

    return CV_OK;
}


/****************************************************************************************\
*                                   The main function                                    *
\****************************************************************************************/

CV_IMPL void
cvCvtColor( const CvArr* srcarr, CvArr* dstarr, int code )
{
    CV_FUNCNAME( "cvCvtColor" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_step, dst_step;
    int src_cn, dst_cn, depth;
    CvColorCvtFunc0 func0 = 0;
    CvColorCvtFunc1 func1 = 0;
    CvColorCvtFunc2 func2 = 0;
    CvColorCvtFunc3 func3 = 0;
    int param[] = { 0, 0, 0, 0 };
    
    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( !CV_ARE_DEPTHS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    depth = CV_MAT_DEPTH(src->type);
    if( depth != CV_8U && depth != CV_16U && depth != CV_32F )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    src_cn = CV_MAT_CN( src->type );
    dst_cn = CV_MAT_CN( dst->type );
    size = cvGetMatSize( src );
    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT(src->type & dst->type) &&
        code != CV_BayerBG2BGR && code != CV_BayerGB2BGR &&
        code != CV_BayerRG2BGR && code != CV_BayerGR2BGR ) 
    {
        size.width *= size.height;
        size.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }

    switch( code )
    {
    case CV_BGR2BGRA:
    case CV_RGB2BGRA:
        if( src_cn != 3 || dst_cn != 4 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func1 = depth == CV_8U ? (CvColorCvtFunc1)icvBGR2BGRx_8u_C3C4R :
                depth == CV_16U ? (CvColorCvtFunc1)icvBGR2BGRx_16u_C3C4R :
                depth == CV_32F ? (CvColorCvtFunc1)icvBGR2BGRx_32f_C3C4R : 0;
        param[0] = code == CV_BGR2BGRA ? 0 : 2; // blue_idx
        break;

    case CV_BGRA2BGR:
    case CV_RGBA2BGR:
    case CV_RGB2BGR:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 3 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func2 = depth == CV_8U ? (CvColorCvtFunc2)icvBGRx2BGR_8u_CnC3R :
                depth == CV_16U ? (CvColorCvtFunc2)icvBGRx2BGR_16u_CnC3R :
                depth == CV_32F ? (CvColorCvtFunc2)icvBGRx2BGR_32f_CnC3R : 0;
        param[0] = src_cn;
        param[1] = code == CV_BGRA2BGR ? 0 : 2; // blue_idx
        break;

    case CV_BGRA2RGBA:
        if( src_cn != 4 || dst_cn != 4 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func0 = depth == CV_8U ? (CvColorCvtFunc0)icvBGRA2RGBA_8u_C4R :
                depth == CV_16U ? (CvColorCvtFunc0)icvBGRA2RGBA_16u_C4R :
                depth == CV_32F ? (CvColorCvtFunc0)icvBGRA2RGBA_32f_C4R : 0;
        break;

    case CV_BGR2BGR565:
    case CV_BGR2BGR555:
    case CV_RGB2BGR565:
    case CV_RGB2BGR555:
    case CV_BGRA2BGR565:
    case CV_BGRA2BGR555:
    case CV_RGBA2BGR565:
    case CV_RGBA2BGR555:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 2 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed RGB format "
            "is only possible for 8-bit images (8-bit grayscale, 888 BGR/RGB or 8888 BGRA/RGBA)" );

        func3 = (CvColorCvtFunc3)icvBGRx2BGR5x5_8u_CnC2R;
        param[0] = src_cn;
        param[1] = code == CV_BGR2BGR565 || code == CV_BGR2BGR555 ||
                   code == CV_BGRA2BGR565 || code == CV_BGRA2BGR555 ? 0 : 2; // blue_idx
        param[2] = code == CV_BGR2BGR565 || code == CV_RGB2BGR565 ||
                   code == CV_BGRA2BGR565 || code == CV_RGBA2BGR565 ? 6 : 5; // green_bits
        break;

    case CV_BGR5652BGR:
    case CV_BGR5552BGR:
    case CV_BGR5652RGB:
    case CV_BGR5552RGB:
    case CV_BGR5652BGRA:
    case CV_BGR5552BGRA:
    case CV_BGR5652RGBA:
    case CV_BGR5552RGBA:
        if( src_cn != 2 || (dst_cn != 3 && dst_cn != 4))
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed BGR format "
            "is only possible for 8-bit images (8-bit grayscale, 888 BGR/BGR or 8888 BGRA/BGRA)" );

        func3 = (CvColorCvtFunc3)icvBGR5x52BGRx_8u_C2CnR;
        param[0] = dst_cn;
        param[1] = code == CV_BGR5652BGR || code == CV_BGR5552BGR ||
                   code == CV_BGR5652BGRA || code == CV_BGR5552BGRA ? 0 : 2; // blue_idx
        param[2] = code == CV_BGR5652BGR || code == CV_BGR5652RGB ||
                   code == CV_BGR5652BGRA || code == CV_BGR5652RGBA ? 6 : 5; // green_bits
        break;

    case CV_BGR2GRAY:
    case CV_BGRA2GRAY:
    case CV_RGB2GRAY:
    case CV_RGBA2GRAY:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 1 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func2 = depth == CV_8U ? (CvColorCvtFunc2)icvBGRx2Gray_8u_CnC1R :
                depth == CV_16U ? (CvColorCvtFunc2)icvBGRx2Gray_16u_CnC1R :
                depth == CV_32F ? (CvColorCvtFunc2)icvBGRx2Gray_32f_CnC1R : 0;
        
        param[0] = src_cn;
        param[1] = code == CV_BGR2GRAY || code == CV_BGRA2GRAY ? 0 : 2;
        break;

    case CV_BGR5652GRAY:
    case CV_BGR5552GRAY:
        if( src_cn != 2 || dst_cn != 1 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed BGR format "
            "is only possible for 8-bit images (888 BGR/BGR or 8888 BGRA/BGRA)" );

        func2 = (CvColorCvtFunc2)icvBGR5x52Gray_8u_C2C1R;
        
        param[0] = code == CV_BGR5652GRAY ? 6 : 5; // green_bits
        break;

    case CV_GRAY2BGR:
    case CV_GRAY2BGRA:
        if( src_cn != 1 || (dst_cn != 3 && dst_cn != 4))
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        func1 = depth == CV_8U ? (CvColorCvtFunc1)icvGray2BGRx_8u_C1CnR :
                depth == CV_16U ? (CvColorCvtFunc1)icvGray2BGRx_16u_C1CnR :
                depth == CV_32F ? (CvColorCvtFunc1)icvGray2BGRx_32f_C1CnR : 0;
        
        param[0] = dst_cn;
        break;

    case CV_GRAY2BGR565:
    case CV_GRAY2BGR555:
        if( src_cn != 1 || dst_cn != 2 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Conversion to/from 16-bit packed BGR format "
            "is only possible for 8-bit images (888 BGR/BGR or 8888 BGRA/BGRA)" );

        func2 = (CvColorCvtFunc2)icvGray2BGR5x5_8u_C1C2R;
        param[0] = code == CV_GRAY2BGR565 ? 6 : 5; // green_bits
        break;

    case CV_BGR2YCrCb:
    case CV_RGB2YCrCb:
    case CV_BGR2XYZ:
    case CV_RGB2XYZ:
    case CV_BGR2HSV:
    case CV_RGB2HSV:
    case CV_BGR2Lab:
    case CV_RGB2Lab:
    case CV_BGR2Luv:
    case CV_RGB2Luv:
    case CV_BGR2HLS:
    case CV_RGB2HLS:
        if( (src_cn != 3 && src_cn != 4) || dst_cn != 3 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth == CV_8U )
            func2 = code == CV_BGR2YCrCb || code == CV_RGB2YCrCb ? (CvColorCvtFunc2)icvBGRx2YCrCb_8u_CnC3R :
                    code == CV_BGR2XYZ || code == CV_RGB2XYZ ? (CvColorCvtFunc2)icvBGRx2XYZ_8u_CnC3R :
                    code == CV_BGR2HSV || code == CV_RGB2HSV ? (CvColorCvtFunc2)icvBGRx2HSV_8u_CnC3R :
                    code == CV_BGR2Lab || code == CV_RGB2Lab ? (CvColorCvtFunc2)icvBGRx2Lab_8u_CnC3R :
                    code == CV_BGR2Luv || code == CV_RGB2Luv ? (CvColorCvtFunc2)icvBGRx2Luv_8u_CnC3R :
                    code == CV_BGR2HLS || code == CV_RGB2HLS ? (CvColorCvtFunc2)icvBGRx2HLS_8u_CnC3R : 0;
        else if( depth == CV_16U )
            func2 = code == CV_BGR2YCrCb || code == CV_RGB2YCrCb ? (CvColorCvtFunc2)icvBGRx2YCrCb_16u_CnC3R :
                    code == CV_BGR2XYZ || code == CV_RGB2XYZ ? (CvColorCvtFunc2)icvBGRx2XYZ_16u_CnC3R : 0;
        else if( depth == CV_32F )
            func2 = code == CV_BGR2YCrCb || code == CV_RGB2YCrCb ? (CvColorCvtFunc2)icvBGRx2YCrCb_32f_CnC3R :
                    code == CV_BGR2XYZ || code == CV_RGB2XYZ ? (CvColorCvtFunc2)icvBGRx2XYZ_32f_CnC3R :
                    code == CV_BGR2HSV || code == CV_RGB2HSV ? (CvColorCvtFunc2)icvBGRx2HSV_32f_CnC3R :
                    code == CV_BGR2Lab || code == CV_RGB2Lab ? (CvColorCvtFunc2)icvBGRx2Lab_32f_CnC3R :
                    code == CV_BGR2Luv || code == CV_RGB2Luv ? (CvColorCvtFunc2)icvBGRx2Luv_32f_CnC3R :
                    code == CV_BGR2HLS || code == CV_RGB2HLS ? (CvColorCvtFunc2)icvBGRx2HLS_32f_CnC3R : 0;
        
        param[0] = src_cn;
        param[1] = code == CV_BGR2XYZ || code == CV_BGR2YCrCb || code == CV_BGR2HSV ||
                   code == CV_BGR2Lab || code == CV_BGR2Luv || code == CV_BGR2HLS ? 0 : 2;
        break;

    case CV_YCrCb2BGR:
    case CV_YCrCb2RGB:
    case CV_XYZ2BGR:
    case CV_XYZ2RGB:
    case CV_HSV2BGR:
    case CV_HSV2RGB:
    case CV_Lab2BGR:
    case CV_Lab2RGB:
    case CV_Luv2BGR:
    case CV_Luv2RGB:
    case CV_HLS2BGR:
    case CV_HLS2RGB:
        if( src_cn != 3 || (dst_cn != 3 && dst_cn != 4) )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );

        if( depth == CV_8U )
            func2 = code == CV_YCrCb2BGR || code == CV_YCrCb2RGB ? (CvColorCvtFunc2)icvYCrCb2BGRx_8u_C3CnR :
                    code == CV_XYZ2BGR || code == CV_XYZ2RGB ? (CvColorCvtFunc2)icvXYZ2BGRx_8u_C3CnR :
                    code == CV_HSV2BGR || code == CV_HSV2RGB ? (CvColorCvtFunc2)icvHSV2BGRx_8u_C3CnR :
                    code == CV_HLS2BGR || code == CV_HLS2RGB ? (CvColorCvtFunc2)icvHLS2BGRx_8u_C3CnR :
                    code == CV_Lab2BGR || code == CV_Lab2RGB ? (CvColorCvtFunc2)icvLab2BGRx_8u_C3CnR :
                    code == CV_Luv2BGR || code == CV_Luv2RGB ? (CvColorCvtFunc2)icvLuv2BGRx_8u_C3CnR : 0;
        else if( depth == CV_16U )
            func2 = code == CV_YCrCb2BGR || code == CV_YCrCb2RGB ? (CvColorCvtFunc2)icvYCrCb2BGRx_16u_C3CnR :
                    code == CV_XYZ2BGR || code == CV_XYZ2RGB ? (CvColorCvtFunc2)icvXYZ2BGRx_16u_C3CnR : 0;
        else if( depth == CV_32F )
            func2 = code == CV_YCrCb2BGR || code == CV_YCrCb2RGB ? (CvColorCvtFunc2)icvYCrCb2BGRx_32f_C3CnR :
                    code == CV_XYZ2BGR || code == CV_XYZ2RGB ? (CvColorCvtFunc2)icvXYZ2BGRx_32f_C3CnR :
                    code == CV_HSV2BGR || code == CV_HSV2RGB ? (CvColorCvtFunc2)icvHSV2BGRx_32f_C3CnR :
                    code == CV_HLS2BGR || code == CV_HLS2RGB ? (CvColorCvtFunc2)icvHLS2BGRx_32f_C3CnR :
                    code == CV_Lab2BGR || code == CV_Lab2RGB ? (CvColorCvtFunc2)icvLab2BGRx_32f_C3CnR :
                    code == CV_Luv2BGR || code == CV_Luv2RGB ? (CvColorCvtFunc2)icvLuv2BGRx_32f_C3CnR : 0;
        
        param[0] = dst_cn;
        param[1] = code == CV_XYZ2BGR || code == CV_YCrCb2BGR || code == CV_HSV2BGR ||
                   code == CV_Lab2BGR || code == CV_Luv2BGR || code == CV_HLS2BGR ? 0 : 2;
        break;

    case CV_BayerBG2BGR:
    case CV_BayerGB2BGR:
    case CV_BayerRG2BGR:
    case CV_BayerGR2BGR:
        if( src_cn != 1 || dst_cn != 3 )
            CV_ERROR( CV_BadNumChannels,
            "Incorrect number of channels for this conversion code" );
        
        if( depth != CV_8U )
            CV_ERROR( CV_BadDepth,
            "Bayer pattern can be converted only to 8-bit 3-channel BGR/RGB image" );

        func1 = (CvColorCvtFunc1)icvBayer2BGR_8u_C1C3R;
        param[0] = code; // conversion code
        break;
    default:
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported color conversion code" );
    }

    if( func0 )
    {
        IPPI_CALL( func0( src->data.ptr, src_step, dst->data.ptr, dst_step, size ));
    }
    else if( func1 )
    {
        IPPI_CALL( func1( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0] ));
    }
    else if( func2 )
    {
        IPPI_CALL( func2( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0], param[1] ));
    }
    else if( func3 )
    {
        IPPI_CALL( func3( src->data.ptr, src_step,
            dst->data.ptr, dst_step, size, param[0], param[1], param[2] ));
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "The image format is not supported" );

    __END__;
}
/* initializes 8-element array for fast access to 3x3 neighborhood of a pixel */
#define  CV_INIT_3X3_DELTAS( deltas, step, nch )            \
    ((deltas)[0] =  (nch),  (deltas)[1] = -(step) + (nch),  \
     (deltas)[2] = -(step), (deltas)[3] = -(step) - (nch),  \
     (deltas)[4] = -(nch),  (deltas)[5] =  (step) - (nch),  \
     (deltas)[6] =  (step), (deltas)[7] =  (step) + (nch))
// 
 static const CvPoint icvCodeDeltas[8] =
     { {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1} };
// 
CV_IMPL void
cvStartReadChainPoints( CvChain * chain, CvChainPtReader * reader )
{
    int i;

    CV_FUNCNAME( "cvStartReadChainPoints" );

    __BEGIN__;

    if( !chain || !reader )
        CV_ERROR( CV_StsNullPtr, "" );

    if( chain->elem_size != 1 || chain->header_size < (int)sizeof(CvChain))
        CV_ERROR_FROM_STATUS( CV_BADSIZE_ERR );

    cvStartReadSeq( (CvSeq *) chain, (CvSeqReader *) reader, 0 );
    CV_CHECK();

    reader->pt = chain->origin;

    for( i = 0; i < 8; i++ )
    {
        reader->deltas[i][0] = (char) icvCodeDeltas[i].x;
        reader->deltas[i][1] = (char) icvCodeDeltas[i].y;
    }

    __END__;
}

// 
// /****************************************************************************************\
// *                         Raster->Chain Tree (Suzuki algorithms)                         *
// \****************************************************************************************/
// 
typedef struct _CvContourInfo
{
    int flags;
    struct _CvContourInfo *next;        /* next contour with the same mark value */
    struct _CvContourInfo *parent;      /* information about parent contour */
    CvSeq *contour;             /* corresponding contour (may be 0, if rejected) */
    CvRect rect;                /* bounding rectangle */
    CvPoint origin;             /* origin point (where the contour was traced from) */
    int is_hole;                /* hole flag */
}
_CvContourInfo;
// 
// 
// /*
//   Structure that is used for sequental retrieving contours from the image.
//   It supports both hierarchical and plane variants of Suzuki algorithm.
// */
typedef struct _CvContourScanner
{
    CvMemStorage *storage1;     /* contains fetched contours */
    CvMemStorage *storage2;     /* contains approximated contours
                                   (!=storage1 if approx_method2 != approx_method1) */
    CvMemStorage *cinfo_storage;        /* contains _CvContourInfo nodes */
    CvSet *cinfo_set;           /* set of _CvContourInfo nodes */
    CvMemStoragePos initial_pos;        /* starting storage pos */
    CvMemStoragePos backup_pos; /* beginning of the latest approx. contour */
    CvMemStoragePos backup_pos2;        /* ending of the latest approx. contour */
    char *img0;                 /* image origin */
    char *img;                  /* current image row */
    int img_step;               /* image step */
    CvSize img_size;            /* ROI size */
    CvPoint offset;             /* ROI offset: coordinates, added to each contour point */
    CvPoint pt;                 /* current scanner position */
    CvPoint lnbd;               /* position of the last met contour */
    int nbd;                    /* current mark val */
    _CvContourInfo *l_cinfo;    /* information about latest approx. contour */
    _CvContourInfo cinfo_temp;  /* temporary var which is used in simple modes */
    _CvContourInfo frame_info;  /* information about frame */
    CvSeq frame;                /* frame itself */
    int approx_method1;         /* approx method when tracing */
    int approx_method2;         /* final approx method */
    int mode;                   /* contour scanning mode:
                                   0 - external only
                                   1 - all the contours w/o any hierarchy
                                   2 - connected components (i.e. two-level structure -
                                   external contours and holes) */
    int subst_flag;
    int seq_type1;              /* type of fetched contours */
    int header_size1;           /* hdr size of fetched contours */
    int elem_size1;             /* elem size of fetched contours */
    int seq_type2;              /*                                       */
    int header_size2;           /*        the same for approx. contours  */
    int elem_size2;             /*                                       */
    _CvContourInfo *cinfo_table[126];
}
_CvContourScanner;
// 
#define _CV_FIND_CONTOURS_FLAGS_EXTERNAL_ONLY    1
#define _CV_FIND_CONTOURS_FLAGS_HIERARCHIC       2
// 
// /* 
//    Initializes scanner structure.
//    Prepare image for scanning ( clear borders and convert all pixels to 0-1.
// */
CV_IMPL CvContourScanner
cvStartFindContours( void* _img, CvMemStorage* storage,
                     int  header_size, int mode, 
                     int  method, CvPoint offset )
{
    int y;
    int step;
    CvSize size;
    uchar *img = 0;
    CvContourScanner scanner = 0;
    CvMat stub, *mat = (CvMat*)_img;

    CV_FUNCNAME( "cvStartFindContours" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( mat = cvGetMat( mat, &stub ));

    if( !CV_IS_MASK_ARR( mat ))
        CV_ERROR( CV_StsUnsupportedFormat, "[Start]FindContours support only 8uC1 images" );

    size = cvSize( mat->width, mat->height );
    step = mat->step;
    img = (uchar*)(mat->data.ptr);

    if( method < 0 || method > CV_CHAIN_APPROX_TC89_KCOS )
        CV_ERROR_FROM_STATUS( CV_BADRANGE_ERR );

    if( header_size < (int) (method == CV_CHAIN_CODE ? sizeof( CvChain ) : sizeof( CvContour )))
        CV_ERROR_FROM_STATUS( CV_BADSIZE_ERR );

    scanner = (CvContourScanner)cvAlloc( sizeof( *scanner ));
    if( !scanner )
        CV_ERROR_FROM_STATUS( CV_OUTOFMEM_ERR );

    memset( scanner, 0, sizeof( *scanner ));

    scanner->storage1 = scanner->storage2 = storage;
    scanner->img0 = (char *) img;
    scanner->img = (char *) (img + step);
    scanner->img_step = step;
    scanner->img_size.width = size.width - 1;   /* exclude rightest column */
    scanner->img_size.height = size.height - 1; /* exclude bottomost row */
    scanner->mode = mode;
    scanner->offset = offset;
    scanner->pt.x = scanner->pt.y = 1;
    scanner->lnbd.x = 0;
    scanner->lnbd.y = 1;
    scanner->nbd = 2;
    scanner->mode = (int) mode;
    scanner->frame_info.contour = &(scanner->frame);
    scanner->frame_info.is_hole = 1;
    scanner->frame_info.next = 0;
    scanner->frame_info.parent = 0;
    scanner->frame_info.rect = cvRect( 0, 0, size.width, size.height );
    scanner->l_cinfo = 0;
    scanner->subst_flag = 0;

    scanner->frame.flags = CV_SEQ_FLAG_HOLE;

    scanner->approx_method2 = scanner->approx_method1 = method;

    if( method == CV_CHAIN_APPROX_TC89_L1 || method == CV_CHAIN_APPROX_TC89_KCOS )
        scanner->approx_method1 = CV_CHAIN_CODE;

    if( scanner->approx_method1 == CV_CHAIN_CODE )
    {
        scanner->seq_type1 = CV_SEQ_CHAIN_CONTOUR;
        scanner->header_size1 = scanner->approx_method1 == scanner->approx_method2 ?
            header_size : sizeof( CvChain );
        scanner->elem_size1 = sizeof( char );
    }
    else
    {
        scanner->seq_type1 = CV_SEQ_POLYGON;
        scanner->header_size1 = scanner->approx_method1 == scanner->approx_method2 ?
            header_size : sizeof( CvContour );
        scanner->elem_size1 = sizeof( CvPoint );
    }

    scanner->header_size2 = header_size;

    if( scanner->approx_method2 == CV_CHAIN_CODE )
    {
        scanner->seq_type2 = scanner->seq_type1;
        scanner->elem_size2 = scanner->elem_size1;
    }
    else
    {
        scanner->seq_type2 = CV_SEQ_POLYGON;
        scanner->elem_size2 = sizeof( CvPoint );
    }

    scanner->seq_type1 = scanner->approx_method1 == CV_CHAIN_CODE ?
        CV_SEQ_CHAIN_CONTOUR : CV_SEQ_POLYGON;

    scanner->seq_type2 = scanner->approx_method2 == CV_CHAIN_CODE ?
        CV_SEQ_CHAIN_CONTOUR : CV_SEQ_POLYGON;

    cvSaveMemStoragePos( storage, &(scanner->initial_pos) );

    if( method > CV_CHAIN_APPROX_SIMPLE )
    {
        scanner->storage1 = cvCreateChildMemStorage( scanner->storage2 );
    }

    if( mode > CV_RETR_LIST )
    {
        scanner->cinfo_storage = cvCreateChildMemStorage( scanner->storage2 );
        scanner->cinfo_set = cvCreateSet( 0, sizeof( CvSet ), sizeof( _CvContourInfo ),
                                          scanner->cinfo_storage );
        if( scanner->cinfo_storage == 0 || scanner->cinfo_set == 0 )
            CV_ERROR_FROM_STATUS( CV_OUTOFMEM_ERR );
    }

    /* make zero borders */
    memset( img, 0, size.width );
    memset( img + step * (size.height - 1), 0, size.width );

    for( y = 1, img += step; y < size.height - 1; y++, img += step )
    {
        img[0] = img[size.width - 1] = 0;
    }

    /* converts all pixels to 0 or 1 */
    cvThreshold( mat, mat, 0, 1, CV_THRESH_BINARY );
    CV_CHECK();

    __END__;

    if( cvGetErrStatus() < 0 )
        cvFree( &scanner );

    return scanner;
}
// 
// /* 
//    Final stage of contour processing.
//    Three variants possible:
//       1. Contour, which was retrieved using border following, is added to
//          the contour tree. It is the case when the icvSubstituteContour function
//          was not called after retrieving the contour.
// 
//       2. New contour, assigned by icvSubstituteContour function, is added to the
//          tree. The retrieved contour itself is removed from the storage.
//          Here two cases are possible:
//             2a. If one deals with plane variant of algorithm
//                 (hierarchical strucutre is not reconstructed),
//                 the contour is removed completely.
//             2b. In hierarchical case, the header of the contour is not removed.
//                 It's marked as "link to contour" and h_next pointer of it is set to
//                 new, substituting contour.
// 
//       3. The similar to 2, but when NULL pointer was assigned by 
//          icvSubstituteContour function. In this case, the function removes
//          retrieved contour completely if plane case and 
//          leaves header if hierarchical (but doesn't mark header as "link").
//       ------------------------------------------------------------------------
//       The 1st variant can be used to retrieve and store all the contours from the image
//       (with optional convertion from chains to contours using some approximation from 
//       restriced set of methods). Some characteristics of contour can be computed in the
//       same pass.
//       
//       The usage scheme can look like:
// 
//       icvContourScanner scanner;
//       CvMemStorage*  contour_storage;
//       CvSeq*  first_contour;
//       CvStatus  result;
// 
//       ...
// 
//       icvCreateMemStorage( &contour_storage, block_size/0 );
// 
//       ...
//               
//       cvStartFindContours
//               ( img, contour_storage,
//                 header_size, approx_method,
//                 [external_only,]
//                 &scanner );
// 
//       for(;;)
//       {
//           [CvSeq* contour;]
//           result = icvFindNextContour( &scanner, &contour/0 );
// 
//           if( result != CV_OK ) break;
// 
//           // calculate some characteristics
//           ...
//       }
// 
//       if( result < 0 ) goto error_processing;
// 
//       cvEndFindContours( &scanner, &first_contour );
//       ...
// 
//       -----------------------------------------------------------------
// 
//       Second variant is more complex and can be used when someone wants store not
//       the retrieved contours but transformed ones. (e.g. approximated with some
//       non-default algorithm ).
//       
//       The scheme can be the as following:
// 
//       icvContourScanner scanner;
//       CvMemStorage*  contour_storage;
//       CvMemStorage*  temp_storage;
//       CvSeq*  first_contour;
//       CvStatus  result;
// 
//       ...
// 
//       icvCreateMemStorage( &contour_storage, block_size/0 );
//       icvCreateMemStorage( &temp_storage, block_size/0 );
// 
//       ...
//               
//       icvStartFindContours8uC1R
//               ( <img_params>, temp_storage,
//                 header_size, approx_method,
//                 [retrival_mode],
//                 &scanner );
// 
//       for(;;)
//       {
//           CvSeq* temp_contour;
//           CvSeq* new_contour;
//           result = icvFindNextContour( scanner, &temp_contour );
// 
//           if( result != CV_OK ) break;
// 
//           <approximation_function>( temp_contour, contour_storage,
//                                     &new_contour, <parameters...> );
// 
//           icvSubstituteContour( scanner, new_contour );
//           ...
//       }
// 
//       if( result < 0 ) goto error_processing;
// 
//       cvEndFindContours( &scanner, &first_contour );
//       ...
// 
//       ----------------------------------------------------------------------------
//       Third method to retrieve contours may be applied if contours are irrelevant
//       themselves but some characteristics of them are used only.
//       The usage is similar to second except slightly different internal loop
// 
//       for(;;)
//       {
//           CvSeq* temp_contour;
//           result = icvFindNextContour( &scanner, &temp_contour );
// 
//           if( result != CV_OK ) break;
// 
//           // calculate some characteristics of temp_contour
// 
//           icvSubstituteContour( scanner, 0 );
//           ...
//       }
// 
//       new_storage variable is not needed here. 
// 
//       Two notes.
//       1. Second and third method can interleave. I.e. it is possible to
//          remain contours that satisfy with some criteria and reject others.
//          In hierarchic case the resulting tree is the part of original tree with
//          some nodes absent. But in the resulting tree the contour1 is a child 
//          (may be indirect) of contour2 iff in the original tree the contour1
//          is a child (may be indirect) of contour2.
// */
static void
icvEndProcessContour( CvContourScanner scanner )
{
    _CvContourInfo *l_cinfo = scanner->l_cinfo;

    if( l_cinfo )
    {
        if( scanner->subst_flag )
        {
            CvMemStoragePos temp;

            cvSaveMemStoragePos( scanner->storage2, &temp );

            if( temp.top == scanner->backup_pos2.top &&
                temp.free_space == scanner->backup_pos2.free_space )
            {
                cvRestoreMemStoragePos( scanner->storage2, &scanner->backup_pos );
            }
            scanner->subst_flag = 0;
        }

        if( l_cinfo->contour )
        {
            cvInsertNodeIntoTree( l_cinfo->contour, l_cinfo->parent->contour,
                                  &(scanner->frame) );
        }
        scanner->l_cinfo = 0;
    }
}

// /* replaces one contour with another */
// CV_IMPL void
// cvSubstituteContour( CvContourScanner scanner, CvSeq * new_contour )
// {
//     _CvContourInfo *l_cinfo;
// 
//     CV_FUNCNAME( "cvSubstituteContour" );
// 
//     __BEGIN__;
// 
//     if( !scanner )
//         CV_ERROR( CV_StsNullPtr, "" );
// 
//     l_cinfo = scanner->l_cinfo;
//     if( l_cinfo && l_cinfo->contour && l_cinfo->contour != new_contour )
//     {
//         l_cinfo->contour = new_contour;
//         scanner->subst_flag = 1;
//     }
// 
//     __END__;
// }
// 
// 
// /* 
//     marks domain border with +/-<constant> and stores the contour into CvSeq.
//         method:
//             <0  - chain
//             ==0 - direct
//             >0  - simple approximation
// */
static CvStatus
icvFetchContour( char                   *ptr, 
                 int                    step,
                 CvPoint                pt, 
                 CvSeq*                 contour, 
                 int    _method )
{
    const char      nbd = 2;
    int             deltas[16];
    CvSeqWriter     writer;
    char            *i0 = ptr, *i1, *i3, *i4 = 0;
    int             prev_s = -1, s, s_end;
    int             method = _method - 1;

    assert( (unsigned) _method <= CV_CHAIN_APPROX_SIMPLE );

    /* initialize local state */
    CV_INIT_3X3_DELTAS( deltas, step, 1 );
    memcpy( deltas + 8, deltas, 8 * sizeof( deltas[0] ));

    /* initialize writer */
    cvStartAppendToSeq( contour, &writer );

    if( method < 0 )
        ((CvChain *) contour)->origin = pt;

    s_end = s = CV_IS_SEQ_HOLE( contour ) ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
        if( *i1 != 0 )
            break;
    }
    while( s != s_end );

    if( s == s_end )            /* single pixel domain */
    {
        *i0 = (char) (nbd | -128);
        if( method >= 0 )
        {
            CV_WRITE_SEQ_ELEM( pt, writer );
        }
    }
    else
    {
        i3 = i0;
        prev_s = s ^ 4;

        /* follow border */
        for( ;; )
        {
            s_end = s;

            for( ;; )
            {
                i4 = i3 + deltas[++s];
                if( *i4 != 0 )
                    break;
            }
            s &= 7;

            /* check "right" bound */
            if( (unsigned) (s - 1) < (unsigned) s_end )
            {
                *i3 = (char) (nbd | -128);
            }
            else if( *i3 == 1 )
            {
                *i3 = nbd;
            }

            if( method < 0 )
            {
                char _s = (char) s;

                CV_WRITE_SEQ_ELEM( _s, writer );
            }
            else
            {
                if( s != prev_s || method == 0 )
                {
                    CV_WRITE_SEQ_ELEM( pt, writer );
                    prev_s = s;
                }

                pt.x += icvCodeDeltas[s].x;
                pt.y += icvCodeDeltas[s].y;

            }

            if( i4 == i0 && i3 == i1 )
                break;

            i3 = i4;
            s = (s + 4) & 7;
        }                       /* end of border following loop */
    }

    cvEndWriteSeq( &writer );

    if( _method != CV_CHAIN_CODE )
        cvBoundingRect( contour, 1 );

    assert( writer.seq->total == 0 && writer.seq->first == 0 ||
            writer.seq->total > writer.seq->first->count ||
            (writer.seq->first->prev == writer.seq->first &&
             writer.seq->first->next == writer.seq->first) );

    return CV_OK;
}
// 
// 
// 
// /* 
//    trace contour until certain point is met.
//    returns 1 if met, 0 else.
// */
static int
icvTraceContour( char *ptr, int step, char *stop_ptr, int is_hole )
{
    int deltas[16];
    char *i0 = ptr, *i1, *i3, *i4;
    int s, s_end;

    /* initialize local state */
    CV_INIT_3X3_DELTAS( deltas, step, 1 );
    memcpy( deltas + 8, deltas, 8 * sizeof( deltas[0] ));

    assert( (*i0 & -2) != 0 );

    s_end = s = is_hole ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
        if( *i1 != 0 )
            break;
    }
    while( s != s_end );

    i3 = i0;

    /* check single pixel domain */
    if( s != s_end )
    {
        /* follow border */
        for( ;; )
        {
            s_end = s;

            for( ;; )
            {
                i4 = i3 + deltas[++s];
                if( *i4 != 0 )
                    break;
            }

            if( i3 == stop_ptr || (i4 == i0 && i3 == i1) )
                break;

            i3 = i4;
            s = (s + 4) & 7;
        }                       /* end of border following loop */
    }
    return i3 == stop_ptr;
}
// 
// 
static CvStatus
icvFetchContourEx( char*                ptr, 
                   int                  step,
                   CvPoint              pt, 
                   CvSeq*               contour,
                   int  _method, 
                   int                  nbd,
                   CvRect*              _rect )
{
    int         deltas[16];
    CvSeqWriter writer;
    char        *i0 = ptr, *i1, *i3, *i4;
    CvRect      rect;
    int         prev_s = -1, s, s_end;
    int         method = _method - 1;

    assert( (unsigned) _method <= CV_CHAIN_APPROX_SIMPLE );
    assert( 1 < nbd && nbd < 128 );

    /* initialize local state */
    CV_INIT_3X3_DELTAS( deltas, step, 1 );
    memcpy( deltas + 8, deltas, 8 * sizeof( deltas[0] ));

    /* initialize writer */
    cvStartAppendToSeq( contour, &writer );

    if( method < 0 )
        ((CvChain *)contour)->origin = pt;

    rect.x = rect.width = pt.x;
    rect.y = rect.height = pt.y;

    s_end = s = CV_IS_SEQ_HOLE( contour ) ? 0 : 4;

    do
    {
        s = (s - 1) & 7;
        i1 = i0 + deltas[s];
        if( *i1 != 0 )
            break;
    }
    while( s != s_end );

    if( s == s_end )            /* single pixel domain */
    {
        *i0 = (char) (nbd | 0x80);
        if( method >= 0 )
        {
            CV_WRITE_SEQ_ELEM( pt, writer );
        }
    }
    else
    {
        i3 = i0;

        prev_s = s ^ 4;

        /* follow border */
        for( ;; )
        {
            s_end = s;

            for( ;; )
            {
                i4 = i3 + deltas[++s];
                if( *i4 != 0 )
                    break;
            }
            s &= 7;

            /* check "right" bound */
            if( (unsigned) (s - 1) < (unsigned) s_end )
            {
                *i3 = (char) (nbd | 0x80);
            }
            else if( *i3 == 1 )
            {
                *i3 = (char) nbd;
            }

            if( method < 0 )
            {
                char _s = (char) s;
                CV_WRITE_SEQ_ELEM( _s, writer );
            }
            else if( s != prev_s || method == 0 )
            {
                CV_WRITE_SEQ_ELEM( pt, writer );
            }

            if( s != prev_s )
            {
                /* update bounds */
                if( pt.x < rect.x )
                    rect.x = pt.x;
                else if( pt.x > rect.width )
                    rect.width = pt.x;

                if( pt.y < rect.y )
                    rect.y = pt.y;
                else if( pt.y > rect.height )
                    rect.height = pt.y;
            }

            prev_s = s;
            pt.x += icvCodeDeltas[s].x;
            pt.y += icvCodeDeltas[s].y;

            if( i4 == i0 && i3 == i1 )  break;

            i3 = i4;
            s = (s + 4) & 7;
        }                       /* end of border following loop */
    }

    rect.width -= rect.x - 1;
    rect.height -= rect.y - 1;

    cvEndWriteSeq( &writer );

    if( _method != CV_CHAIN_CODE )
        ((CvContour*)contour)->rect = rect;

    assert( writer.seq->total == 0 && writer.seq->first == 0 ||
            writer.seq->total > writer.seq->first->count ||
            (writer.seq->first->prev == writer.seq->first &&
             writer.seq->first->next == writer.seq->first) );

    if( _rect )  *_rect = rect;

    return CV_OK;
}
// 
// 
CvSeq *
cvFindNextContour( CvContourScanner scanner )
{
    char *img0;
    char *img;
    int step;
    int width, height;
    int x, y;
    int prev;
    CvPoint lnbd;
    CvSeq *contour = 0;
    int nbd;
    int mode;
    CvStatus result = (CvStatus) 1;

    CV_FUNCNAME( "cvFindNextContour" );

    __BEGIN__;

    if( !scanner )
        CV_ERROR( CV_StsNullPtr, "" );
    icvEndProcessContour( scanner );

    /* initialize local state */
    img0 = scanner->img0;
    img = scanner->img;
    step = scanner->img_step;
    x = scanner->pt.x;
    y = scanner->pt.y;
    width = scanner->img_size.width;
    height = scanner->img_size.height;
    mode = scanner->mode;
    lnbd = scanner->lnbd;
    nbd = scanner->nbd;

    prev = img[x - 1];

    for( ; y < height; y++, img += step )
    {
        for( ; x < width; x++ )
        {
            int p = img[x];

            if( p != prev )
            {
                _CvContourInfo *par_info = 0;
                _CvContourInfo *l_cinfo = 0;
                CvSeq *seq = 0;
                int is_hole = 0;
                CvPoint origin;

                if( !(prev == 0 && p == 1) )    /* if not external contour */
                {
                    /* check hole */
                    if( p != 0 || prev < 1 )
                        goto resume_scan;

                    if( prev & -2 )
                    {
                        lnbd.x = x - 1;
                    }
                    is_hole = 1;
                }

                if( mode == 0 && (is_hole || img0[lnbd.y * step + lnbd.x] > 0) )
                    goto resume_scan;

                origin.y = y;
                origin.x = x - is_hole;

                /* find contour parent */
                if( mode <= 1 || (!is_hole && mode == 2) || lnbd.x <= 0 )
                {
                    par_info = &(scanner->frame_info);
                }
                else
                {
                    int lval = img0[lnbd.y * step + lnbd.x] & 0x7f;
                    _CvContourInfo *cur = scanner->cinfo_table[lval - 2];

                    assert( lval >= 2 );

                    /* find the first bounding contour */
                    while( cur )
                    {
                        if( (unsigned) (lnbd.x - cur->rect.x) < (unsigned) cur->rect.width &&
                            (unsigned) (lnbd.y - cur->rect.y) < (unsigned) cur->rect.height )
                        {
                            if( par_info )
                            {
                                if( icvTraceContour( scanner->img0 +
                                                     par_info->origin.y * step +
                                                     par_info->origin.x, step, img + lnbd.x,
                                                     par_info->is_hole ) > 0 )
                                    break;
                            }
                            par_info = cur;
                        }
                        cur = cur->next;
                    }

                    assert( par_info != 0 );

                    /* if current contour is a hole and previous contour is a hole or
                       current contour is external and previous contour is external then
                       the parent of the contour is the parent of the previous contour else
                       the parent is the previous contour itself. */
                    if( par_info->is_hole == is_hole )
                    {
                        par_info = par_info->parent;
                        /* every contour must have a parent
                           (at least, the frame of the image) */
                        if( !par_info )
                            par_info = &(scanner->frame_info);
                    }

                    /* hole flag of the parent must differ from the flag of the contour */
                    assert( par_info->is_hole != is_hole );
                    if( par_info->contour == 0 )        /* removed contour */
                        goto resume_scan;
                }

                lnbd.x = x - is_hole;

                cvSaveMemStoragePos( scanner->storage2, &(scanner->backup_pos) );

                seq = cvCreateSeq( scanner->seq_type1, scanner->header_size1,
                                   scanner->elem_size1, scanner->storage1 );
                if( !seq )
                {
                    result = CV_OUTOFMEM_ERR;
                    goto exit_func;
                }
                seq->flags |= is_hole ? CV_SEQ_FLAG_HOLE : 0;

                /* initialize header */
                if( mode <= 1 )
                {
                    l_cinfo = &(scanner->cinfo_temp);
                    result = icvFetchContour( img + x - is_hole, step,
                                              cvPoint( origin.x + scanner->offset.x,
                                                       origin.y + scanner->offset.y),
                                              seq, scanner->approx_method1 );
                    if( result < 0 )
                        goto exit_func;
                }
                else
                {
                    union { _CvContourInfo* ci; CvSetElem* se; } v;
                    v.ci = l_cinfo;
                    cvSetAdd( scanner->cinfo_set, 0, &v.se );
                    l_cinfo = v.ci;

                    result = icvFetchContourEx( img + x - is_hole, step,
                                                cvPoint( origin.x + scanner->offset.x,
                                                         origin.y + scanner->offset.y),
                                                seq, scanner->approx_method1,
                                                nbd, &(l_cinfo->rect) );
                    if( result < 0 )
                        goto exit_func;
                    l_cinfo->rect.x -= scanner->offset.x;
                    l_cinfo->rect.y -= scanner->offset.y;

                    l_cinfo->next = scanner->cinfo_table[nbd - 2];
                    scanner->cinfo_table[nbd - 2] = l_cinfo;

                    /* change nbd */
                    nbd = (nbd + 1) & 127;
                    nbd += nbd == 0 ? 3 : 0;
                }

                l_cinfo->is_hole = is_hole;
                l_cinfo->contour = seq;
                l_cinfo->origin = origin;
                l_cinfo->parent = par_info;

                if( scanner->approx_method1 != scanner->approx_method2 )
                {
                    result = icvApproximateChainTC89( (CvChain *) seq,
                                                      scanner->header_size2,
                                                      scanner->storage2,
                                                      &(l_cinfo->contour),
                                                      scanner->approx_method2 );
                    if( result < 0 )
                        goto exit_func;
                    cvClearMemStorage( scanner->storage1 );
                }

                l_cinfo->contour->v_prev = l_cinfo->parent->contour;

                if( par_info->contour == 0 )
                {
                    l_cinfo->contour = 0;
                    if( scanner->storage1 == scanner->storage2 )
                    {
                        cvRestoreMemStoragePos( scanner->storage1, &(scanner->backup_pos) );
                    }
                    else
                    {
                        cvClearMemStorage( scanner->storage1 );
                    }
                    p = img[x];
                    goto resume_scan;
                }

                cvSaveMemStoragePos( scanner->storage2, &(scanner->backup_pos2) );
                scanner->l_cinfo = l_cinfo;
                scanner->pt.x = x + 1;
                scanner->pt.y = y;
                scanner->lnbd = lnbd;
                scanner->img = (char *) img;
                scanner->nbd = nbd;
                contour = l_cinfo->contour;

                result = CV_OK;
                goto exit_func;
              resume_scan:
                prev = p;
                /* update lnbd */
                if( prev & -2 )
                {
                    lnbd.x = x;
                }
            }                   /* end of prev != p */
        }                       /* end of loop on x */

        lnbd.x = 0;
        lnbd.y = y + 1;
        x = 1;
        prev = 0;

    }                           /* end of loop on y */

  exit_func:

    if( result != 0 )
        contour = 0;
    if( result < 0 )
        CV_ERROR_FROM_STATUS( result );

    __END__;

    return contour;
}
// 
// 
// /* 
//    The function add to tree the last retrieved/substituted contour, 
//    releases temp_storage, restores state of dst_storage (if needed), and
//    returns pointer to root of the contour tree */
CV_IMPL CvSeq *
cvEndFindContours( CvContourScanner * _scanner )
{
    CvContourScanner scanner;
    CvSeq *first = 0;

    CV_FUNCNAME( "cvFindNextContour" );

    __BEGIN__;

    if( !_scanner )
        CV_ERROR( CV_StsNullPtr, "" );
    scanner = *_scanner;

    if( scanner )
    {
        icvEndProcessContour( scanner );

        if( scanner->storage1 != scanner->storage2 )
            cvReleaseMemStorage( &(scanner->storage1) );

        if( scanner->cinfo_storage )
            cvReleaseMemStorage( &(scanner->cinfo_storage) );

        first = scanner->frame.v_next;
        cvFree( _scanner );
    }

    __END__;

    return first;
}
// 
// 
#define ICV_SINGLE                  0
#define ICV_CONNECTING_ABOVE        1
#define ICV_CONNECTING_BELOW        -1
#define ICV_IS_COMPONENT_POINT(val) ((val) != 0)

#define CV_GET_WRITTEN_ELEM( writer ) ((writer).ptr - (writer).seq->elem_size)

typedef  struct CvLinkedRunPoint
{
    struct CvLinkedRunPoint* link;
    struct CvLinkedRunPoint* next;
    CvPoint pt;
}
CvLinkedRunPoint;
// 
// 
static int
icvFindContoursInInterval( const CvArr* src,
                           /*int minValue, int maxValue,*/
                           CvMemStorage* storage,
                           CvSeq** result,
                           int contourHeaderSize )
{
    int count = 0;
    CvMemStorage* storage00 = 0;
    CvMemStorage* storage01 = 0;
    CvSeq* first = 0;

    CV_FUNCNAME( "icvFindContoursInInterval" );

    __BEGIN__;

    int i, j, k, n;

    uchar*  src_data = 0;
    int  img_step = 0;
    CvSize  img_size;

    int  connect_flag;
    int  lower_total;
    int  upper_total;
    int  all_total;

    CvSeq*  runs;
    CvLinkedRunPoint  tmp;
    CvLinkedRunPoint*  tmp_prev;
    CvLinkedRunPoint*  upper_line = 0;
    CvLinkedRunPoint*  lower_line = 0;
    CvLinkedRunPoint*  last_elem;

    CvLinkedRunPoint*  upper_run = 0;
    CvLinkedRunPoint*  lower_run = 0;
    CvLinkedRunPoint*  prev_point = 0;

    CvSeqWriter  writer_ext;
    CvSeqWriter  writer_int;
    CvSeqWriter  writer;
    CvSeqReader  reader;

    CvSeq* external_contours;
    CvSeq* internal_contours;
    CvSeq* prev = 0;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "NULL storage pointer" );

    if( !result )
        CV_ERROR( CV_StsNullPtr, "NULL double CvSeq pointer" );

    if( contourHeaderSize < (int)sizeof(CvContour))
        CV_ERROR( CV_StsBadSize, "Contour header size must be >= sizeof(CvContour)" );

    CV_CALL( storage00 = cvCreateChildMemStorage(storage));
    CV_CALL( storage01 = cvCreateChildMemStorage(storage));

    {
        CvMat stub, *mat;

        CV_CALL( mat = cvGetMat( src, &stub ));
        if( !CV_IS_MASK_ARR(mat))
            CV_ERROR( CV_StsBadArg, "Input array must be 8uC1 or 8sC1" );
        src_data = mat->data.ptr;
        img_step = mat->step;
        img_size = cvGetMatSize( mat );
    }

    // Create temporary sequences
    runs = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvLinkedRunPoint), storage00 );
    cvStartAppendToSeq( runs, &writer );

    cvStartWriteSeq( 0, sizeof(CvSeq), sizeof(CvLinkedRunPoint*), storage01, &writer_ext );
    cvStartWriteSeq( 0, sizeof(CvSeq), sizeof(CvLinkedRunPoint*), storage01, &writer_int );

    tmp_prev = &(tmp);
    tmp_prev->next = 0;
    tmp_prev->link = 0;
    
    // First line. None of runs is binded
    tmp.pt.y = 0;
    i = 0;
    CV_WRITE_SEQ_ELEM( tmp, writer );
    upper_line = (CvLinkedRunPoint*)CV_GET_WRITTEN_ELEM( writer );
    
    tmp_prev = upper_line;
    for( j = 0; j < img_size.width; )
    {
        for( ; j < img_size.width && !ICV_IS_COMPONENT_POINT(src_data[j]); j++ )
            ;
        if( j == img_size.width )
            break;

        tmp.pt.x = j;
        CV_WRITE_SEQ_ELEM( tmp, writer );
        tmp_prev->next = (CvLinkedRunPoint*)CV_GET_WRITTEN_ELEM( writer );
        tmp_prev = tmp_prev->next;

        for( ; j < img_size.width && ICV_IS_COMPONENT_POINT(src_data[j]); j++ )
            ;

        tmp.pt.x = j-1;
        CV_WRITE_SEQ_ELEM( tmp, writer );
        tmp_prev->next = (CvLinkedRunPoint*)CV_GET_WRITTEN_ELEM( writer );
        tmp_prev->link = tmp_prev->next;
        // First point of contour
        CV_WRITE_SEQ_ELEM( tmp_prev, writer_ext );
        tmp_prev = tmp_prev->next;
    }
    cvFlushSeqWriter( &writer );
    upper_line = upper_line->next;
    upper_total = runs->total - 1;
    last_elem = tmp_prev;
    tmp_prev->next = 0;
    
    for( i = 1; i < img_size.height; i++ )
    {
//------// Find runs in next line
        src_data += img_step;
        tmp.pt.y = i;
        all_total = runs->total;
        for( j = 0; j < img_size.width; )
        {
            for( ; j < img_size.width && !ICV_IS_COMPONENT_POINT(src_data[j]); j++ )
                ;
            if( j == img_size.width ) break;

            tmp.pt.x = j;
            CV_WRITE_SEQ_ELEM( tmp, writer );
            tmp_prev->next = (CvLinkedRunPoint*)CV_GET_WRITTEN_ELEM( writer );
            tmp_prev = tmp_prev->next;

            for( ; j < img_size.width && ICV_IS_COMPONENT_POINT(src_data[j]); j++ )
                ;

            tmp.pt.x = j-1;
            CV_WRITE_SEQ_ELEM( tmp, writer );
            tmp_prev = tmp_prev->next = (CvLinkedRunPoint*)CV_GET_WRITTEN_ELEM( writer );
        }//j
        cvFlushSeqWriter( &writer );
        lower_line = last_elem->next;
        lower_total = runs->total - all_total;
        last_elem = tmp_prev;
        tmp_prev->next = 0;
//------//
//------// Find links between runs of lower_line and upper_line
        upper_run = upper_line;
        lower_run = lower_line;
        connect_flag = ICV_SINGLE;

        for( k = 0, n = 0; k < upper_total/2 && n < lower_total/2; )
        {
            switch( connect_flag )
            {
            case ICV_SINGLE:
                if( upper_run->next->pt.x < lower_run->next->pt.x )
                {
                    if( upper_run->next->pt.x >= lower_run->pt.x  -1 )
                    {
                        lower_run->link = upper_run;
                        connect_flag = ICV_CONNECTING_ABOVE;
                        prev_point = upper_run->next;
                    }
                    else
                        upper_run->next->link = upper_run;
                    k++;
                    upper_run = upper_run->next->next;
                }
                else
                {
                    if( upper_run->pt.x <= lower_run->next->pt.x  +1 )
                    {
                        lower_run->link = upper_run;
                        connect_flag = ICV_CONNECTING_BELOW;
                        prev_point = lower_run->next;
                    }
                    else
                    {
                        lower_run->link = lower_run->next;
                        // First point of contour
                        CV_WRITE_SEQ_ELEM( lower_run, writer_ext );
                    }
                    n++;
                    lower_run = lower_run->next->next;
                }
                break;
            case ICV_CONNECTING_ABOVE:
                if( upper_run->pt.x > lower_run->next->pt.x +1 )
                {
                    prev_point->link = lower_run->next;
                    connect_flag = ICV_SINGLE;
                    n++;
                    lower_run = lower_run->next->next;
                }
                else
                {
                    prev_point->link = upper_run;
                    if( upper_run->next->pt.x < lower_run->next->pt.x )
                    {
                        k++;
                        prev_point = upper_run->next;
                        upper_run = upper_run->next->next;
                    }
                    else
                    {
                        connect_flag = ICV_CONNECTING_BELOW;
                        prev_point = lower_run->next;
                        n++;
                        lower_run = lower_run->next->next;
                    }
                }
                break;
            case ICV_CONNECTING_BELOW:
                if( lower_run->pt.x > upper_run->next->pt.x +1 )
                {
                    upper_run->next->link = prev_point;
                    connect_flag = ICV_SINGLE;
                    k++;
                    upper_run = upper_run->next->next;
                }
                else
                {
                    // First point of contour
                    CV_WRITE_SEQ_ELEM( lower_run, writer_int );

                    lower_run->link = prev_point;
                    if( lower_run->next->pt.x < upper_run->next->pt.x )
                    {
                        n++;
                        prev_point = lower_run->next;
                        lower_run = lower_run->next->next;
                    }
                    else
                    {
                        connect_flag = ICV_CONNECTING_ABOVE;
                        k++;
                        prev_point = upper_run->next;
                        upper_run = upper_run->next->next;
                    }
                }
                break;          
            }
        }// k, n

        for( ; n < lower_total/2; n++ )
        {
            if( connect_flag != ICV_SINGLE )
            {
                prev_point->link = lower_run->next;
                connect_flag = ICV_SINGLE;
                lower_run = lower_run->next->next;
                continue;
            }
            lower_run->link = lower_run->next;

            //First point of contour
            CV_WRITE_SEQ_ELEM( lower_run, writer_ext );

            lower_run = lower_run->next->next;
        }

        for( ; k < upper_total/2; k++ )
        {
            if( connect_flag != ICV_SINGLE )
            {
                upper_run->next->link = prev_point;
                connect_flag = ICV_SINGLE;
                upper_run = upper_run->next->next;
                continue;
            }
            upper_run->next->link = upper_run;
            upper_run = upper_run->next->next;
        }
        upper_line = lower_line;
        upper_total = lower_total;
    }//i

    upper_run = upper_line;

    //the last line of image
    for( k = 0; k < upper_total/2; k++ )
    {
        upper_run->next->link = upper_run;
        upper_run = upper_run->next->next;
    }

//------//
//------//Find end read contours
    external_contours = cvEndWriteSeq( &writer_ext );
    internal_contours = cvEndWriteSeq( &writer_int );

    for( k = 0; k < 2; k++ )
    {
        CvSeq* contours = k == 0 ? external_contours : internal_contours;

        cvStartReadSeq( contours, &reader );

        for( j = 0; j < contours->total; j++, count++ )
        {
            CvLinkedRunPoint* p_temp;
            CvLinkedRunPoint* p00;
            CvLinkedRunPoint* p01;
            CvSeq* contour;

            CV_READ_SEQ_ELEM( p00, reader );
            p01 = p00;

            if( !p00->link )
                continue;

            cvStartWriteSeq( CV_SEQ_ELTYPE_POINT | CV_SEQ_POLYLINE | CV_SEQ_FLAG_CLOSED,
                             contourHeaderSize, sizeof(CvPoint), storage, &writer );
            do
            {
                CV_WRITE_SEQ_ELEM( p00->pt, writer );
                p_temp = p00;
                p00 = p00->link;
                p_temp->link = 0;
            }
            while( p00 != p01 );

            contour = cvEndWriteSeq( &writer );
            cvBoundingRect( contour, 1 );

            if( k != 0 )
                contour->flags |= CV_SEQ_FLAG_HOLE;

            if( !first )
                prev = first = contour;
            else
            {
                contour->h_prev = prev;
                prev = prev->h_next = contour;
            }
        }
    }

    __END__;

    if( !first )
        count = -1;

    if( result )
        *result = first;

    cvReleaseMemStorage(&storage00);
    cvReleaseMemStorage(&storage01);

    return count;
}
// 
// 
// 
// /*F///////////////////////////////////////////////////////////////////////////////////////
// //    Name: cvFindContours
// //    Purpose:
// //      Finds all the contours on the bi-level image.
// //    Context:
// //    Parameters:
// //      img  - source image.
// //             Non-zero pixels are considered as 1-pixels
// //             and zero pixels as 0-pixels.
// //      step - full width of source image in bytes.
// //      size - width and height of the image in pixels
// //      storage - pointer to storage where will the output contours be placed.
// //      header_size - header size of resulting contours
// //      mode - mode of contour retrieval.
// //      method - method of approximation that is applied to contours
// //      first_contour - pointer to first contour pointer
// //    Returns:
// //      CV_OK or error code
// //    Notes:
// //F*/
CV_IMPL int
cvFindContours( void*  img,  CvMemStorage*  storage,                
                CvSeq**  firstContour, int  cntHeaderSize,                 
                int  mode, 
                int  method, CvPoint offset )
{
    CvContourScanner scanner = 0;
    CvSeq *contour = 0;
    int count = -1;
    
    CV_FUNCNAME( "cvFindContours" );

    __BEGIN__;

    if( !firstContour )
        CV_ERROR( CV_StsNullPtr, "NULL double CvSeq pointer" );

    if( method == CV_LINK_RUNS )
    {
        if( offset.x != 0 || offset.y != 0 )
            CV_ERROR( CV_StsOutOfRange,
            "Nonzero offset is not supported in CV_LINK_RUNS yet" );

        CV_CALL( count = icvFindContoursInInterval( img, storage,
                                    firstContour, cntHeaderSize ));
    }
    else
    {
        CV_CALL( scanner = cvStartFindContours( img, storage,
                        cntHeaderSize, mode, method, offset ));
        assert( scanner );

        do
        {
            count++;
            contour = cvFindNextContour( scanner );
        }
        while( contour != 0 );

        *firstContour = cvEndFindContours( &scanner );    
    }

    __END__;

    return count;
}

#define CV_MATCH_CHECK( status, cvFun )                                    \
  {                                                                        \
    status = cvFun;                                                        \
    if( status != CV_OK )                                                  \
     goto M_END;                                                           \
  }

static CvStatus
icvCalcTriAttr( const CvSeq * contour, CvPoint t2, CvPoint t1, int n1,
                CvPoint t3, int n3, double *s, double *s_c,
                double *h, double *a, double *b );

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: icvCreateContourTree
//    Purpose:
//    Create binary tree representation for the contour 
//    Context:
//    Parameters:
//      contour - pointer to input contour object.
//      storage - pointer to the current storage block
//      tree   -  output pointer to the binary tree representation 
//      threshold - threshold for the binary tree building 
//
//F*/
static CvStatus
icvCreateContourTree( const CvSeq * contour, CvMemStorage * storage,
                      CvContourTree ** tree, double threshold )
{
    CvPoint *pt_p;              /*  pointer to previos points   */
    CvPoint *pt_n;              /*  pointer to next points      */
    CvPoint *pt1, *pt2;         /*  pointer to current points   */

    CvPoint t, tp1, tp2, tp3, tn1, tn2, tn3;
    int lpt, flag, i, j, i_tree, j_1, j_3, i_buf;
    double s, sp1, sp2, sn1, sn2, s_c, sp1_c, sp2_c, sn1_c, sn2_c, h, hp1, hp2, hn1, hn2,
        a, ap1, ap2, an1, an2, b, bp1, bp2, bn1, bn2;
    double a_s_c, a_sp1_c;

    _CvTrianAttr **ptr_p, **ptr_n, **ptr1, **ptr2;      /*  pointers to pointers of triangles  */
    _CvTrianAttr *cur_adr;

    int *num_p, *num_n, *num1, *num2;   /*   numbers of input contour points   */
    int nm, nmp1, nmp2, nmp3, nmn1, nmn2, nmn3;
    int seq_flags = 1, i_end, prev_null, prev2_null;
    double koef = 1.5;
    double eps = 1.e-7;
    double e;
    CvStatus status;
    int hearder_size;
    _CvTrianAttr tree_one, tree_two, *tree_end, *tree_root;

    CvSeqWriter writer;

    assert( contour != NULL && contour->total >= 4 );
    status = CV_OK;

    if( contour == NULL )
        return CV_NULLPTR_ERR;
    if( contour->total < 4 )
        return CV_BADSIZE_ERR;

    if( !CV_IS_SEQ_POLYGON( contour ))
        return CV_BADFLAG_ERR;


/*   Convert Sequence to array    */
    lpt = contour->total;
    pt_p = pt_n = NULL;
    num_p = num_n = NULL;
    ptr_p = ptr_n = ptr1 = ptr2 = NULL;
    tree_end = NULL;

    pt_p = (CvPoint *) cvAlloc( lpt * sizeof( CvPoint ));
    pt_n = (CvPoint *) cvAlloc( lpt * sizeof( CvPoint ));

    num_p = (int *) cvAlloc( lpt * sizeof( int ));
    num_n = (int *) cvAlloc( lpt * sizeof( int ));

    hearder_size = sizeof( CvContourTree );
    seq_flags = CV_SEQ_POLYGON_TREE;
    cvStartWriteSeq( seq_flags, hearder_size, sizeof( _CvTrianAttr ), storage, &writer );

    ptr_p = (_CvTrianAttr **) cvAlloc( lpt * sizeof( _CvTrianAttr * ));
    ptr_n = (_CvTrianAttr **) cvAlloc( lpt * sizeof( _CvTrianAttr * ));

    memset( ptr_p, 0, lpt * sizeof( _CvTrianAttr * ));
    memset( ptr_n, 0, lpt * sizeof( _CvTrianAttr * ));

    if( pt_p == NULL || pt_n == NULL )
        return CV_OUTOFMEM_ERR;
    if( ptr_p == NULL || ptr_n == NULL )
        return CV_OUTOFMEM_ERR;

/*     write fild for the binary tree root   */
/*  start_writer = writer;   */

    tree_one.pt.x = tree_one.pt.y = 0;
    tree_one.sign = 0;
    tree_one.area = 0;
    tree_one.r1 = tree_one.r2 = 0;
    tree_one.next_v1 = tree_one.next_v2 = tree_one.prev_v = NULL;

    CV_WRITE_SEQ_ELEM( tree_one, writer );
    tree_root = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

    if( cvCvtSeqToArray( contour, (char *) pt_p ) == (char *) contour )
        return CV_BADPOINT_ERR;

    for( i = 0; i < lpt; i++ )
        num_p[i] = i;

    i = lpt;
    flag = 0;
    i_tree = 0;
    e = 20.;                    /*  initial threshold value   */
    ptr1 = ptr_p;
    ptr2 = ptr_n;
    pt1 = pt_p;
    pt2 = pt_n;
    num1 = num_p;
    num2 = num_n;
/*  binary tree constraction    */
    while( i > 4 )
    {
        if( flag == 0 )
        {
            ptr1 = ptr_p;
            ptr2 = ptr_n;
            pt1 = pt_p;
            pt2 = pt_n;
            num1 = num_p;
            num2 = num_n;
            flag = 1;
        }
        else
        {
            ptr1 = ptr_n;
            ptr2 = ptr_p;
            pt1 = pt_n;
            pt2 = pt_p;
            num1 = num_n;
            num2 = num_p;
            flag = 0;
        }
        t = pt1[0];
        nm = num1[0];
        tp1 = pt1[i - 1];
        nmp1 = num1[i - 1];
        tp2 = pt1[i - 2];
        nmp2 = num1[i - 2];
        tp3 = pt1[i - 3];
        nmp3 = num1[i - 3];
        tn1 = pt1[1];
        nmn1 = num1[1];
        tn2 = pt1[2];
        nmn2 = num1[2];

        i_buf = 0;
        i_end = -1;
        CV_MATCH_CHECK( status,
                        icvCalcTriAttr( contour, t, tp1, nmp1, tn1, nmn1, &s, &s_c, &h, &a,
                                        &b ));
        CV_MATCH_CHECK( status,
                        icvCalcTriAttr( contour, tp1, tp2, nmp2, t, nm, &sp1, &sp1_c, &hp1,
                                        &ap1, &bp1 ));
        CV_MATCH_CHECK( status,
                        icvCalcTriAttr( contour, tp2, tp3, nmp3, tp1, nmp1, &sp2, &sp2_c, &hp2,
                                        &ap2, &bp2 ));
        CV_MATCH_CHECK( status,
                        icvCalcTriAttr( contour, tn1, t, nm, tn2, nmn2, &sn1, &sn1_c, &hn1,
                                        &an1, &bn1 ));


        j_3 = 3;
        prev_null = prev2_null = 0;
        for( j = 0; j < i; j++ )
        {
            tn3 = pt1[j_3];
            nmn3 = num1[j_3];
            if( j == 0 )
                j_1 = i - 1;
            else
                j_1 = j - 1;

            CV_MATCH_CHECK( status, icvCalcTriAttr( contour, tn2, tn1, nmn1, tn3, nmn3,
                                                    &sn2, &sn2_c, &hn2, &an2, &bn2 ));

            if( (s_c < sp1_c && s_c < sp2_c && s_c <= sn1_c && s_c <= sn2_c && s_c < e) ||
                (s_c == sp1_c && s_c <= sp2_c || s_c == sp2_c && s_c <= sp1_c) && s_c <= sn1_c
                && s_c <= sn2_c && s_c < e && j > 1 && prev2_null == 0 || (s_c < eps && j > 0
                                                                           && prev_null == 0) )

            {
                prev_null = prev2_null = 1;
                if( s_c < threshold )
                {
                    if( ptr1[j_1] == NULL && ptr1[j] == NULL )
                    {
                        if( i_buf > 0 )
                            ptr2[i_buf - 1] = NULL;
                        else
                            i_end = 0;
                    }
                    else
                    {
/*   form next vertex  */
                        tree_one.pt = t;
                        tree_one.sign = (char) (CV_SIGN( s ));
                        tree_one.r1 = h / a;
                        tree_one.r2 = b / a;
                        tree_one.area = fabs( s );
                        tree_one.next_v1 = ptr1[j_1];
                        tree_one.next_v2 = ptr1[j];

                        CV_WRITE_SEQ_ELEM( tree_one, writer );
                        cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

                        if( ptr1[j_1] != NULL )
                            ptr1[j_1]->prev_v = cur_adr;
                        if( ptr1[j] != NULL )
                            ptr1[j]->prev_v = cur_adr;

                        if( i_buf > 0 )
                            ptr2[i_buf - 1] = cur_adr;
                        else
                        {
                            tree_end = (_CvTrianAttr *) writer.ptr;
                            i_end = 1;
                        }
                        i_tree++;
                    }
                }
                else
/*   form next vertex    */
                {
                    tree_one.pt = t;
                    tree_one.sign = (char) (CV_SIGN( s ));
                    tree_one.area = fabs( s );
                    tree_one.r1 = h / a;
                    tree_one.r2 = b / a;
                    tree_one.next_v1 = ptr1[j_1];
                    tree_one.next_v2 = ptr1[j];

                    CV_WRITE_SEQ_ELEM( tree_one, writer );
                    cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

                    if( ptr1[j_1] != NULL )
                        ptr1[j_1]->prev_v = cur_adr;
                    if( ptr1[j] != NULL )
                        ptr1[j]->prev_v = cur_adr;

                    if( i_buf > 0 )
                        ptr2[i_buf - 1] = cur_adr;
                    else
                    {
                        tree_end = cur_adr;
                        i_end = 1;
                    }
                    i_tree++;
                }
            }
            else
/*   the current triangle is'not LMIAT    */
            {
                prev_null = 0;
                switch (prev2_null)
                {
                case 0:
                    break;
                case 1:
                    {
                        prev2_null = 2;
                        break;
                    }
                case 2:
                    {
                        prev2_null = 0;
                        break;
                    }
                }
                if( j != i - 1 || i_end == -1 )
                    ptr2[i_buf] = ptr1[j];
                else if( i_end == 0 )
                    ptr2[i_buf] = NULL;
                else
                    ptr2[i_buf] = tree_end;
                pt2[i_buf] = t;
                num2[i_buf] = num1[j];
                i_buf++;
            }
/*    go to next vertex    */
            tp3 = tp2;
            tp2 = tp1;
            tp1 = t;
            t = tn1;
            tn1 = tn2;
            tn2 = tn3;
            nmp3 = nmp2;
            nmp2 = nmp1;
            nmp1 = nm;
            nm = nmn1;
            nmn1 = nmn2;
            nmn2 = nmn3;

            sp2 = sp1;
            sp1 = s;
            s = sn1;
            sn1 = sn2;
            sp2_c = sp1_c;
            sp1_c = s_c;
            s_c = sn1_c;
            sn1_c = sn2_c;

            ap2 = ap1;
            ap1 = a;
            a = an1;
            an1 = an2;
            bp2 = bp1;
            bp1 = b;
            b = bn1;
            bn1 = bn2;
            hp2 = hp1;
            hp1 = h;
            h = hn1;
            hn1 = hn2;
            j_3++;
            if( j_3 >= i )
                j_3 = 0;
        }

        i = i_buf;
        e = e * koef;
    }

/*  constract tree root  */
    if( i != 4 )
        return CV_BADFACTOR_ERR;

    t = pt2[0];
    tn1 = pt2[1];
    tn2 = pt2[2];
    tp1 = pt2[3];
    nm = num2[0];
    nmn1 = num2[1];
    nmn2 = num2[2];
    nmp1 = num2[3];
/*   first pair of the triangles   */
    CV_MATCH_CHECK( status,
                    icvCalcTriAttr( contour, t, tp1, nmp1, tn1, nmn1, &s, &s_c, &h, &a, &b ));
    CV_MATCH_CHECK( status,
                    icvCalcTriAttr( contour, tn2, tn1, nmn1, tp1, nmp1, &sn2, &sn2_c, &hn2,
                                    &an2, &bn2 ));
/*   second pair of the triangles   */
    CV_MATCH_CHECK( status,
                    icvCalcTriAttr( contour, tn1, t, nm, tn2, nmn2, &sn1, &sn1_c, &hn1, &an1,
                                    &bn1 ));
    CV_MATCH_CHECK( status,
                    icvCalcTriAttr( contour, tp1, tn2, nmn2, t, nm, &sp1, &sp1_c, &hp1, &ap1,
                                    &bp1 ));

    a_s_c = fabs( s_c - sn2_c );
    a_sp1_c = fabs( sp1_c - sn1_c );

    if( a_s_c > a_sp1_c )
/*   form child vertexs for the root     */
    {
        tree_one.pt = t;
        tree_one.sign = (char) (CV_SIGN( s ));
        tree_one.area = fabs( s );
        tree_one.r1 = h / a;
        tree_one.r2 = b / a;
        tree_one.next_v1 = ptr2[3];
        tree_one.next_v2 = ptr2[0];

        tree_two.pt = tn2;
        tree_two.sign = (char) (CV_SIGN( sn2 ));
        tree_two.area = fabs( sn2 );
        tree_two.r1 = hn2 / an2;
        tree_two.r2 = bn2 / an2;
        tree_two.next_v1 = ptr2[1];
        tree_two.next_v2 = ptr2[2];

        CV_WRITE_SEQ_ELEM( tree_one, writer );
        cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

        if( s_c > sn2_c )
        {
            if( ptr2[3] != NULL )
                ptr2[3]->prev_v = cur_adr;
            if( ptr2[0] != NULL )
                ptr2[0]->prev_v = cur_adr;
            ptr1[0] = cur_adr;

            i_tree++;

            CV_WRITE_SEQ_ELEM( tree_two, writer );
            cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

            if( ptr2[1] != NULL )
                ptr2[1]->prev_v = cur_adr;
            if( ptr2[2] != NULL )
                ptr2[2]->prev_v = cur_adr;
            ptr1[1] = cur_adr;

            i_tree++;

            pt1[0] = tp1;
            pt1[1] = tn1;
        }
        else
        {
            CV_WRITE_SEQ_ELEM( tree_two, writer );
            cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

            if( ptr2[1] != NULL )
                ptr2[1]->prev_v = cur_adr;
            if( ptr2[2] != NULL )
                ptr2[2]->prev_v = cur_adr;
            ptr1[0] = cur_adr;

            i_tree++;

            CV_WRITE_SEQ_ELEM( tree_one, writer );
            cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

            if( ptr2[3] != NULL )
                ptr2[3]->prev_v = cur_adr;
            if( ptr2[0] != NULL )
                ptr2[0]->prev_v = cur_adr;
            ptr1[1] = cur_adr;

            i_tree++;

            pt1[0] = tn1;
            pt1[1] = tp1;
        }
    }
    else
    {
        tree_one.pt = tp1;
        tree_one.sign = (char) (CV_SIGN( sp1 ));
        tree_one.area = fabs( sp1 );
        tree_one.r1 = hp1 / ap1;
        tree_one.r2 = bp1 / ap1;
        tree_one.next_v1 = ptr2[2];
        tree_one.next_v2 = ptr2[3];

        tree_two.pt = tn1;
        tree_two.sign = (char) (CV_SIGN( sn1 ));
        tree_two.area = fabs( sn1 );
        tree_two.r1 = hn1 / an1;
        tree_two.r2 = bn1 / an1;
        tree_two.next_v1 = ptr2[0];
        tree_two.next_v2 = ptr2[1];

        CV_WRITE_SEQ_ELEM( tree_one, writer );
        cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

        if( sp1_c > sn1_c )
        {
            if( ptr2[2] != NULL )
                ptr2[2]->prev_v = cur_adr;
            if( ptr2[3] != NULL )
                ptr2[3]->prev_v = cur_adr;
            ptr1[0] = cur_adr;

            i_tree++;

            CV_WRITE_SEQ_ELEM( tree_two, writer );
            cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

            if( ptr2[0] != NULL )
                ptr2[0]->prev_v = cur_adr;
            if( ptr2[1] != NULL )
                ptr2[1]->prev_v = cur_adr;
            ptr1[1] = cur_adr;

            i_tree++;

            pt1[0] = tn2;
            pt1[1] = t;
        }
        else
        {
            CV_WRITE_SEQ_ELEM( tree_two, writer );
            cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

            if( ptr2[0] != NULL )
                ptr2[0]->prev_v = cur_adr;
            if( ptr2[1] != NULL )
                ptr2[1]->prev_v = cur_adr;
            ptr1[0] = cur_adr;

            i_tree++;

            CV_WRITE_SEQ_ELEM( tree_one, writer );
            cur_adr = (_CvTrianAttr *) (writer.ptr - writer.seq->elem_size);

            if( ptr2[2] != NULL )
                ptr2[2]->prev_v = cur_adr;
            if( ptr2[3] != NULL )
                ptr2[3]->prev_v = cur_adr;
            ptr1[1] = cur_adr;

            i_tree++;

            pt1[0] = t;
            pt1[1] = tn2;

        }
    }

/*    form root   */
    s = cvContourArea( contour );

    tree_root->pt = pt1[1];
    tree_root->sign = 0;
    tree_root->area = fabs( s );
    tree_root->r1 = 0;
    tree_root->r2 = 0;
    tree_root->next_v1 = ptr1[0];
    tree_root->next_v2 = ptr1[1];
    tree_root->prev_v = NULL;

    ptr1[0]->prev_v = (_CvTrianAttr *) tree_root;
    ptr1[1]->prev_v = (_CvTrianAttr *) tree_root;

/*     write binary tree root   */
/*    CV_WRITE_SEQ_ELEM (tree_one, start_writer);   */
    i_tree++;
/*  create Sequence hearder     */
    *((CvSeq **) tree) = cvEndWriteSeq( &writer );
/*   write points for the main segment into sequence header   */
    (*tree)->p1 = pt1[0];

  M_END:

    cvFree( &ptr_n );
    cvFree( &ptr_p );
    cvFree( &num_n );
    cvFree( &num_p );
    cvFree( &pt_n );
    cvFree( &pt_p );

    return status;
}

/****************************************************************************************\

 triangle attributes calculations 

\****************************************************************************************/
static CvStatus
icvCalcTriAttr( const CvSeq * contour, CvPoint t2, CvPoint t1, int n1,
                CvPoint t3, int n3, double *s, double *s_c,
                double *h, double *a, double *b )
{
    double x13, y13, x12, y12, l_base, nx, ny, qq;
    double eps = 1.e-5;

    x13 = t3.x - t1.x;
    y13 = t3.y - t1.y;
    x12 = t2.x - t1.x;
    y12 = t2.y - t1.y;
    qq = x13 * x13 + y13 * y13;
    l_base = cvSqrt( (float) (qq) );
    if( l_base > eps )
    {
        nx = y13 / l_base;
        ny = -x13 / l_base;

        *h = nx * x12 + ny * y12;

        *s = (*h) * l_base / 2.;

        *b = nx * y12 - ny * x12;

        *a = l_base;
/*   calculate interceptive area   */
        *s_c = cvContourArea( contour, cvSlice(n1, n3+1));
    }
    else
    {
        *h = 0;
        *s = 0;
        *s_c = 0;
        *b = 0;
        *a = 0;
    }

    return CV_OK;
}

/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: cvCreateContourTree
//    Purpose:
//    Create binary tree representation for the contour 
//    Context:
//    Parameters:
//      contour - pointer to input contour object.
//      storage - pointer to the current storage block
//      tree   -  output pointer to the binary tree representation 
//      threshold - threshold for the binary tree building 
//
//F*/
CV_IMPL CvContourTree*
cvCreateContourTree( const CvSeq* contour, CvMemStorage* storage, double threshold )
{
    CvContourTree* tree = 0;
    
    CV_FUNCNAME( "cvCreateContourTree" );
    __BEGIN__;

    IPPI_CALL( icvCreateContourTree( contour, storage, &tree, threshold ));

    __CLEANUP__;
    __END__;

    return tree;
}


/*F///////////////////////////////////////////////////////////////////////////////////////
//    Name: icvContourFromContourTree
//    Purpose:
//    reconstracts contour from binary tree representation  
//    Context:
//    Parameters:
//      tree   -  pointer to the input binary tree representation 
//      storage - pointer to the current storage block
//      contour - pointer to output contour object.
//      criteria - criteria for the definition threshold value
//                 for the contour reconstracting (level or precision)
//F*/
CV_IMPL CvSeq*
cvContourFromContourTree( const CvContourTree*  tree,
                          CvMemStorage*  storage,
                          CvTermCriteria  criteria )
{
    CvSeq* contour = 0;
    _CvTrianAttr **ptr_buf = 0;     /*  pointer to the pointer's buffer  */
    int *level_buf = 0;
    int i_buf;

    int lpt;
    double area_all;
    double threshold;
    int cur_level;
    int level;
    int seq_flags;
    char log_iter, log_eps;
    int out_hearder_size;
    _CvTrianAttr *tree_one = 0, tree_root;  /*  current vertex  */

    CvSeqReader reader;
    CvSeqWriter writer;

    CV_FUNCNAME("cvContourFromContourTree");

    __BEGIN__;

    if( !tree )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !CV_IS_SEQ_POLYGON_TREE( tree ))
        CV_ERROR_FROM_STATUS( CV_BADFLAG_ERR );

    criteria = cvCheckTermCriteria( criteria, 0., 100 );

    lpt = tree->total;
    ptr_buf = NULL;
    level_buf = NULL;
    i_buf = 0;
    cur_level = 0;
    log_iter = (char) (criteria.type == CV_TERMCRIT_ITER ||
                       (criteria.type == CV_TERMCRIT_ITER + CV_TERMCRIT_EPS));
    log_eps = (char) (criteria.type == CV_TERMCRIT_EPS ||
                      (criteria.type == CV_TERMCRIT_ITER + CV_TERMCRIT_EPS));

    cvStartReadSeq( (CvSeq *) tree, &reader, 0 );

    out_hearder_size = sizeof( CvContour );

    seq_flags = CV_SEQ_POLYGON;
    cvStartWriteSeq( seq_flags, out_hearder_size, sizeof( CvPoint ), storage, &writer );

    ptr_buf = (_CvTrianAttr **) cvAlloc( lpt * sizeof( _CvTrianAttr * ));
    if( ptr_buf == NULL )
        CV_ERROR_FROM_STATUS( CV_OUTOFMEM_ERR );
    if( log_iter )
    {
        level_buf = (int *) cvAlloc( lpt * (sizeof( int )));

        if( level_buf == NULL )
            CV_ERROR_FROM_STATUS( CV_OUTOFMEM_ERR );
    }

    memset( ptr_buf, 0, lpt * sizeof( _CvTrianAttr * ));

/*     write the first tree root's point as a start point of the result contour  */
    CV_WRITE_SEQ_ELEM( tree->p1, writer );
/*     write the second tree root"s point into buffer    */

/*     read the root of the tree   */
    CV_READ_SEQ_ELEM( tree_root, reader );

    tree_one = &tree_root;
    area_all = tree_one->area;

    if( log_eps )
        threshold = criteria.epsilon * area_all;
    else
        threshold = 10 * area_all;

    if( log_iter )
        level = criteria.max_iter;
    else
        level = -1;

/*  contour from binary tree constraction    */
    while( i_buf >= 0 )
    {
        if( tree_one != NULL && (cur_level <= level || tree_one->area >= threshold) )
/*   go to left sub tree for the vertex and save pointer to the right vertex   */
/*   into the buffer     */
        {
            ptr_buf[i_buf] = tree_one;
            if( log_iter )
            {
                level_buf[i_buf] = cur_level;
                cur_level++;
            }
            i_buf++;
            tree_one = tree_one->next_v1;
        }
        else
        {
            i_buf--;
            if( i_buf >= 0 )
            {
                CvPoint pt = ptr_buf[i_buf]->pt;
                CV_WRITE_SEQ_ELEM( pt, writer );
                tree_one = ptr_buf[i_buf]->next_v2;
                if( log_iter )
                {
                    cur_level = level_buf[i_buf] + 1;
                }
            }
        }
    }

    contour = cvEndWriteSeq( &writer );
    cvBoundingRect( contour, 1 );

    __CLEANUP__;
    __END__;

    cvFree( &level_buf );
    cvFree( &ptr_buf );

    return contour;
}

static int
icvSklansky_32s( CvPoint** array, int start, int end, int* stack, int nsign, int sign2 )
{
    int incr = end > start ? 1 : -1;
    /* prepare first triangle */
    int pprev = start, pcur = pprev + incr, pnext = pcur + incr;
    int stacksize = 3;

    if( start == end ||
        (array[start]->x == array[end]->x &&
         array[start]->y == array[end]->y) )
    {
        stack[0] = start;
        return 1;
    }

    stack[0] = pprev;
    stack[1] = pcur;
    stack[2] = pnext;

    end += incr; /* make end = afterend */

    while( pnext != end )
    {
        /* check the angle p1,p2,p3 */
        int cury = array[pcur]->y;
        int nexty = array[pnext]->y;
        int by = nexty - cury;

        if( CV_SIGN(by) != nsign )
        {
            int ax = array[pcur]->x - array[pprev]->x;
            int bx = array[pnext]->x - array[pcur]->x;
            int ay = cury - array[pprev]->y;
            int convexity = ay*bx - ax*by;/* if >0 then convex angle */

            if( CV_SIGN(convexity) == sign2 && (ax != 0 || ay != 0) )
            {
                pprev = pcur;
                pcur = pnext;
                pnext += incr;
                stack[stacksize] = pnext;
                stacksize++;
            }
            else
            {
                if( pprev == start )
                {
                    pcur = pnext;
                    stack[1] = pcur;
                    pnext += incr;
                    stack[2] = pnext;
                }
                else
                {
                    stack[stacksize-2] = pnext;
                    pcur = pprev;
                    pprev = stack[stacksize-4];
                    stacksize--;
                }
            }
        }
        else
        {
            pnext += incr;
            stack[stacksize-1] = pnext;
        }
    }

    return --stacksize;
}
// 
// 
static int
icvSklansky_32f( CvPoint2D32f** array, int start, int end, int* stack, int nsign, int sign2 )
{
    int incr = end > start ? 1 : -1;
    /* prepare first triangle */
    int pprev = start, pcur = pprev + incr, pnext = pcur + incr;
    int stacksize = 3;

    if( start == end ||
        (array[start]->x == array[end]->x &&
         array[start]->y == array[end]->y) )
    {
        stack[0] = start;
        return 1;
    }

    stack[0] = pprev;
    stack[1] = pcur;
    stack[2] = pnext;

    end += incr; /* make end = afterend */

    while( pnext != end )
    {
        /* check the angle p1,p2,p3 */
        float cury = array[pcur]->y;
        float nexty = array[pnext]->y;
        float by = nexty - cury;

        if( CV_SIGN( by ) != nsign )
        {
            float ax = array[pcur]->x - array[pprev]->x;
            float bx = array[pnext]->x - array[pcur]->x;
            float ay = cury - array[pprev]->y;
            float convexity = ay*bx - ax*by;/* if >0 then convex angle */

            if( CV_SIGN( convexity ) == sign2 && (ax != 0 || ay != 0) )
            {
                pprev = pcur;
                pcur = pnext;
                pnext += incr;
                stack[stacksize] = pnext;
                stacksize++;
            }
            else
            {
                if( pprev == start )
                {
                    pcur = pnext;
                    stack[1] = pcur;
                    pnext += incr;
                    stack[2] = pnext;

                }
                else
                {
                    stack[stacksize-2] = pnext;
                    pcur = pprev;
                    pprev = stack[stacksize-4];
                    stacksize--;
                }
            }
        }
        else
        {
            pnext += incr;
            stack[stacksize-1] = pnext;
        }
    }

    return --stacksize;
}
// 
typedef int (*sklansky_func)( CvPoint** points, int start, int end,
                              int* stack, int sign, int sign2 );
// 
 #define cmp_pts( pt1, pt2 )  \
    ((pt1)->x < (pt2)->x || (pt1)->x <= (pt2)->x && (pt1)->y < (pt2)->y)
static CV_IMPLEMENT_QSORT( icvSortPointsByPointers_32s, CvPoint*, cmp_pts )
static CV_IMPLEMENT_QSORT( icvSortPointsByPointers_32f, CvPoint2D32f*, cmp_pts )
// 
static void
icvCalcAndWritePtIndices( CvPoint** pointer, int* stack, int start, int end,
                          CvSeq* ptseq, CvSeqWriter* writer )
{
    CV_FUNCNAME( "icvCalcAndWritePtIndices" );

    __BEGIN__;
    
    int i, incr = start < end ? 1 : -1;
    int idx, first_idx = ptseq->first->start_index;

    for( i = start; i != end; i += incr )
    {
        CvPoint* ptr = (CvPoint*)pointer[stack[i]];
        CvSeqBlock* block = ptseq->first;
        while( (unsigned)(idx = (int)(ptr - (CvPoint*)block->data)) >= (unsigned)block->count )
        {
            block = block->next;
            if( block == ptseq->first )
                CV_ERROR( CV_StsError, "Internal error" );
        }
        idx += block->start_index - first_idx;
        CV_WRITE_SEQ_ELEM( idx, *writer );
    }

    __END__;
}
// 
// 
CV_IMPL CvSeq*
cvConvexHull2( const CvArr* array, void* hull_storage,
               int orientation, int return_points )
{
    union { CvContour* c; CvSeq* s; } hull;
    CvPoint** pointer = 0;
    CvPoint2D32f** pointerf = 0;
    int* stack = 0;
    
    CV_FUNCNAME( "cvConvexHull2" );
    
    hull.s = 0;

    __BEGIN__;

    CvMat* mat = 0;
    CvSeqReader reader;
    CvSeqWriter writer;
    CvContour contour_header;
    union { CvContour c; CvSeq s; } hull_header;
    CvSeqBlock block, hullblock;
    CvSeq* ptseq = 0;
    CvSeq* hullseq = 0;
    int is_float;
    int* t_stack;
    int t_count;
    int i, miny_ind = 0, maxy_ind = 0, total;
    int hulltype;
    int stop_idx;
    sklansky_func sklansky;

    if( CV_IS_SEQ( array ))
    {
        ptseq = (CvSeq*)array;
        if( !CV_IS_SEQ_POINT_SET( ptseq ))
            CV_ERROR( CV_StsBadArg, "Unsupported sequence type" );
        if( hull_storage == 0 )
            hull_storage = ptseq->storage;
    }
    else
    {
        CV_CALL( ptseq = cvPointSeqFromMat(
            CV_SEQ_KIND_GENERIC, array, &contour_header, &block ));
    }

    if( CV_IS_STORAGE( hull_storage ))
    {
        if( return_points )
        {
            CV_CALL( hullseq = cvCreateSeq(
                CV_SEQ_KIND_CURVE|CV_SEQ_ELTYPE(ptseq)|
                CV_SEQ_FLAG_CLOSED|CV_SEQ_FLAG_CONVEX,
                sizeof(CvContour), sizeof(CvPoint),(CvMemStorage*)hull_storage ));
        }
        else
        {
            CV_CALL( hullseq = cvCreateSeq(
                CV_SEQ_KIND_CURVE|CV_SEQ_ELTYPE_PPOINT|
                CV_SEQ_FLAG_CLOSED|CV_SEQ_FLAG_CONVEX,
                sizeof(CvContour), sizeof(CvPoint*), (CvMemStorage*)hull_storage ));
        }
    }
    else
    {
        if( !CV_IS_MAT( hull_storage ))
            CV_ERROR(CV_StsBadArg, "Destination must be valid memory storage or matrix");

        mat = (CvMat*)hull_storage;

        if( mat->cols != 1 && mat->rows != 1 || !CV_IS_MAT_CONT(mat->type))
            CV_ERROR( CV_StsBadArg,
            "The hull matrix should be continuous and have a single row or a single column" );

        if( mat->cols + mat->rows - 1 < ptseq->total )
            CV_ERROR( CV_StsBadSize, "The hull matrix size might be not enough to fit the hull" );

        if( CV_MAT_TYPE(mat->type) != CV_SEQ_ELTYPE(ptseq) &&
            CV_MAT_TYPE(mat->type) != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "The hull matrix must have the same type as input or 32sC1 (integers)" );

        CV_CALL( hullseq = cvMakeSeqHeaderForArray(
            CV_SEQ_KIND_CURVE|CV_MAT_TYPE(mat->type)|CV_SEQ_FLAG_CLOSED,
            sizeof(contour_header), CV_ELEM_SIZE(mat->type), mat->data.ptr,
            mat->cols + mat->rows - 1, &hull_header.s, &hullblock ));

        cvClearSeq( hullseq );
    }

    total = ptseq->total;
    if( total == 0 )
    {
        if( mat )
            CV_ERROR( CV_StsBadSize,
            "Point sequence can not be empty if the output is matrix" );
        EXIT;
    }

    cvStartAppendToSeq( hullseq, &writer );

    is_float = CV_SEQ_ELTYPE(ptseq) == CV_32FC2;
    hulltype = CV_SEQ_ELTYPE(hullseq);
    sklansky = !is_float ? (sklansky_func)icvSklansky_32s :
                           (sklansky_func)icvSklansky_32f;

    CV_CALL( pointer = (CvPoint**)cvAlloc( ptseq->total*sizeof(pointer[0]) ));
    CV_CALL( stack = (int*)cvAlloc( (ptseq->total + 2)*sizeof(stack[0]) ));
    pointerf = (CvPoint2D32f**)pointer;

    cvStartReadSeq( ptseq, &reader );

    for( i = 0; i < total; i++ )
    {
        pointer[i] = (CvPoint*)reader.ptr;
        CV_NEXT_SEQ_ELEM( ptseq->elem_size, reader );
    }

    // sort the point set by x-coordinate, find min and max y
    if( !is_float )
    {
        icvSortPointsByPointers_32s( pointer, total, 0 );
        for( i = 1; i < total; i++ )
        {
            int y = pointer[i]->y;
            if( pointer[miny_ind]->y > y )
                miny_ind = i;
            if( pointer[maxy_ind]->y < y )
                maxy_ind = i;
        }
    }
    else
    {
        icvSortPointsByPointers_32f( pointerf, total, 0 );
        for( i = 1; i < total; i++ )
        {
            float y = pointerf[i]->y;
            if( pointerf[miny_ind]->y > y )
                miny_ind = i;
            if( pointerf[maxy_ind]->y < y )
                maxy_ind = i;
        }
    }

    if( pointer[0]->x == pointer[total-1]->x &&
        pointer[0]->y == pointer[total-1]->y )
    {
        if( hulltype == CV_SEQ_ELTYPE_PPOINT )
        {
            CV_WRITE_SEQ_ELEM( pointer[0], writer );
        }
        else if( hulltype == CV_SEQ_ELTYPE_INDEX )
        {
            int index = 0;
            CV_WRITE_SEQ_ELEM( index, writer );
        }
        else
        {
            CvPoint pt = pointer[0][0];
            CV_WRITE_SEQ_ELEM( pt, writer );
        }
        goto finish_hull;
    }

    /*upper half */
    {
        int *tl_stack = stack;
        int tl_count = sklansky( pointer, 0, maxy_ind, tl_stack, -1, 1 );
        int *tr_stack = tl_stack + tl_count;
        int tr_count = sklansky( pointer, ptseq->total - 1, maxy_ind, tr_stack, -1, -1 );

        /* gather upper part of convex hull to output */
        if( orientation == CV_COUNTER_CLOCKWISE )
        {
            CV_SWAP( tl_stack, tr_stack, t_stack );
            CV_SWAP( tl_count, tr_count, t_count );
        }

        if( hulltype == CV_SEQ_ELTYPE_PPOINT )
        {
            for( i = 0; i < tl_count - 1; i++ )
                CV_WRITE_SEQ_ELEM( pointer[tl_stack[i]], writer );

            for( i = tr_count - 1; i > 0; i-- )
                CV_WRITE_SEQ_ELEM( pointer[tr_stack[i]], writer );
        }
        else if( hulltype == CV_SEQ_ELTYPE_INDEX )
        {
            CV_CALL( icvCalcAndWritePtIndices( pointer, tl_stack,
                                               0, tl_count-1, ptseq, &writer ));
            CV_CALL( icvCalcAndWritePtIndices( pointer, tr_stack,
                                               tr_count-1, 0, ptseq, &writer ));
        }
        else
        {
            for( i = 0; i < tl_count - 1; i++ )
                CV_WRITE_SEQ_ELEM( pointer[tl_stack[i]][0], writer );

            for( i = tr_count - 1; i > 0; i-- )
                CV_WRITE_SEQ_ELEM( pointer[tr_stack[i]][0], writer );
        }
        stop_idx = tr_count > 2 ? tr_stack[1] : tl_count > 2 ? tl_stack[tl_count - 2] : -1;
    }

    /* lower half */
    {
        int *bl_stack = stack;
        int bl_count = sklansky( pointer, 0, miny_ind, bl_stack, 1, -1 );
        int *br_stack = stack + bl_count;
        int br_count = sklansky( pointer, ptseq->total - 1, miny_ind, br_stack, 1, 1 );

        if( orientation != CV_COUNTER_CLOCKWISE )
        {
            CV_SWAP( bl_stack, br_stack, t_stack );
            CV_SWAP( bl_count, br_count, t_count );
        }

        if( stop_idx >= 0 )
        {
            int check_idx = bl_count > 2 ? bl_stack[1] :
                            bl_count + br_count > 2 ? br_stack[2-bl_count] : -1;
            if( check_idx == stop_idx || check_idx >= 0 &&
                pointer[check_idx]->x == pointer[stop_idx]->x &&
                pointer[check_idx]->y == pointer[stop_idx]->y )
            {
                /* if all the points lie on the same line, then
                   the bottom part of the convex hull is the mirrored top part
                   (except the exteme points).*/
                bl_count = MIN( bl_count, 2 );
                br_count = MIN( br_count, 2 );
            }
        }

        if( hulltype == CV_SEQ_ELTYPE_PPOINT )
        {
            for( i = 0; i < bl_count - 1; i++ )
                CV_WRITE_SEQ_ELEM( pointer[bl_stack[i]], writer );

            for( i = br_count - 1; i > 0; i-- )
                CV_WRITE_SEQ_ELEM( pointer[br_stack[i]], writer );
        }
        else if( hulltype == CV_SEQ_ELTYPE_INDEX )
        {
            CV_CALL( icvCalcAndWritePtIndices( pointer, bl_stack,
                                               0, bl_count-1, ptseq, &writer ));
            CV_CALL( icvCalcAndWritePtIndices( pointer, br_stack,
                                               br_count-1, 0, ptseq, &writer ));
        }
        else
        {
            for( i = 0; i < bl_count - 1; i++ )
                CV_WRITE_SEQ_ELEM( pointer[bl_stack[i]][0], writer );

            for( i = br_count - 1; i > 0; i-- )
                CV_WRITE_SEQ_ELEM( pointer[br_stack[i]][0], writer );
        }
    }

finish_hull:
    CV_CALL( cvEndWriteSeq( &writer ));

    if( mat )
    {
        if( mat->rows > mat->cols )
            mat->rows = hullseq->total;
        else
            mat->cols = hullseq->total;
    }
    else
    {
        hull.s = hullseq;
        hull.c->rect = cvBoundingRect( ptseq,
            ptseq->header_size < (int)sizeof(CvContour) ||
            &ptseq->flags == &contour_header.flags );
        
        /*if( ptseq != (CvSeq*)&contour_header )
            hullseq->v_prev = ptseq;*/
    }

    __END__;

    cvFree( &pointer );
    cvFree( &stack );

    return hull.s;
}
//////////////////////////////////////////////////////////////////////////
//Canny
//////////////////////////////////////////////////////////////////////////
icvCannyGetSize_t icvCannyGetSize_p = 0;
icvCanny_16s8u_C1R_t icvCanny_16s8u_C1R_p = 0;

CV_IMPL void
cvCanny( const void* srcarr, void* dstarr,
         double low_thresh, double high_thresh, int aperture_size )
{
    static const int sec_tab[] = { 1, 3, 0, 0, 2, 2, 2, 2 };
    CvMat *dx = 0, *dy = 0;
    void *buffer = 0;
    uchar **stack_top, **stack_bottom = 0;

    CV_FUNCNAME( "cvCanny" );

    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int flags = aperture_size;
    int low, high;
    int* mag_buf[3];
    uchar* map;
    int mapstep, maxsize;
    int i, j;
    CvMat mag_row;

    CV_CALL( src = cvGetMat( src, &srcstub ));
    CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( CV_MAT_TYPE( src->type ) != CV_8UC1 ||
        CV_MAT_TYPE( dst->type ) != CV_8UC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( low_thresh > high_thresh )
    {
        double t;
        CV_SWAP( low_thresh, high_thresh, t );
    }

    aperture_size &= INT_MAX;
    if( (aperture_size & 1) == 0 || aperture_size < 3 || aperture_size > 7 )
        CV_ERROR( CV_StsBadFlag, "" );

    size = cvGetMatSize( src );

    dx = cvCreateMat( size.height, size.width, CV_16SC1 );
    dy = cvCreateMat( size.height, size.width, CV_16SC1 );
    cvSobel( src, dx, 1, 0, aperture_size );
    cvSobel( src, dy, 0, 1, aperture_size );

    if( icvCannyGetSize_p && icvCanny_16s8u_C1R_p && !(flags & CV_CANNY_L2_GRADIENT) )
    {
        int buf_size=  0;
        IPPI_CALL( icvCannyGetSize_p( size, &buf_size ));
        CV_CALL( buffer = cvAlloc( buf_size ));
        IPPI_CALL( icvCanny_16s8u_C1R_p( (short*)dx->data.ptr, dx->step,
                                     (short*)dy->data.ptr, dy->step,
                                     dst->data.ptr, dst->step,
                                     size, (float)low_thresh,
                                     (float)high_thresh, buffer ));
        EXIT;
    }

    if( flags & CV_CANNY_L2_GRADIENT )
    {
        Cv32suf ul, uh;
        ul.f = (float)low_thresh;
        uh.f = (float)high_thresh;

        low = ul.i;
        high = uh.i;
    }
    else
    {
        low = cvFloor( low_thresh );
        high = cvFloor( high_thresh );
    }

    CV_CALL( buffer = cvAlloc( (size.width+2)*(size.height+2) +
                                (size.width+2)*3*sizeof(int)) );

    mag_buf[0] = (int*)buffer;
    mag_buf[1] = mag_buf[0] + size.width + 2;
    mag_buf[2] = mag_buf[1] + size.width + 2;
    map = (uchar*)(mag_buf[2] + size.width + 2);
    mapstep = size.width + 2;

    maxsize = MAX( 1 << 10, size.width*size.height/10 );
    CV_CALL( stack_top = stack_bottom = (uchar**)cvAlloc( maxsize*sizeof(stack_top[0]) ));

    memset( mag_buf[0], 0, (size.width+2)*sizeof(int) );
    memset( map, 1, mapstep );
    memset( map + mapstep*(size.height + 1), 1, mapstep );

    /* sector numbers 
       (Top-Left Origin)

        1   2   3
         *  *  * 
          * * *  
        0*******0
          * * *  
         *  *  * 
        3   2   1
    */

    #define CANNY_PUSH(d)    *(d) = (uchar)2, *stack_top++ = (d)
    #define CANNY_POP(d)     (d) = *--stack_top

    mag_row = cvMat( 1, size.width, CV_32F );

    // calculate magnitude and angle of gradient, perform non-maxima supression.
    // fill the map with one of the following values:
    //   0 - the pixel might belong to an edge
    //   1 - the pixel can not belong to an edge
    //   2 - the pixel does belong to an edge
    for( i = 0; i <= size.height; i++ )
    {
        int* _mag = mag_buf[(i > 0) + 1] + 1;
        float* _magf = (float*)_mag;
        const short* _dx = (short*)(dx->data.ptr + dx->step*i);
        const short* _dy = (short*)(dy->data.ptr + dy->step*i);
        uchar* _map;
        int x, y;
        int magstep1, magstep2;
        int prev_flag = 0;

        if( i < size.height )
        {
            _mag[-1] = _mag[size.width] = 0;

            if( !(flags & CV_CANNY_L2_GRADIENT) )
                for( j = 0; j < size.width; j++ )
                    _mag[j] = abs(_dx[j]) + abs(_dy[j]);
            else if( icvFilterSobelVert_8u16s_C1R_p != 0 ) // check for IPP
            {
                // use vectorized sqrt
                mag_row.data.fl = _magf;
                for( j = 0; j < size.width; j++ )
                {
                    x = _dx[j]; y = _dy[j];
                    _magf[j] = (float)((double)x*x + (double)y*y);
                }
                cvPow( &mag_row, &mag_row, 0.5 );
            }
            else
            {
                for( j = 0; j < size.width; j++ )
                {
                    x = _dx[j]; y = _dy[j];
                    _magf[j] = (float)sqrt((double)x*x + (double)y*y);
                }
            }
        }
        else
            memset( _mag-1, 0, (size.width + 2)*sizeof(int) );

        // at the very beginning we do not have a complete ring
        // buffer of 3 magnitude rows for non-maxima suppression
        if( i == 0 )
            continue;

        _map = map + mapstep*i + 1;
        _map[-1] = _map[size.width] = 1;
        
        _mag = mag_buf[1] + 1; // take the central row
        _dx = (short*)(dx->data.ptr + dx->step*(i-1));
        _dy = (short*)(dy->data.ptr + dy->step*(i-1));
        
        magstep1 = (int)(mag_buf[2] - mag_buf[1]);
        magstep2 = (int)(mag_buf[0] - mag_buf[1]);

        if( (stack_top - stack_bottom) + size.width > maxsize )
        {
            uchar** new_stack_bottom;
            maxsize = MAX( maxsize * 3/2, maxsize + size.width );
            CV_CALL( new_stack_bottom = (uchar**)cvAlloc( maxsize * sizeof(stack_top[0])) );
            memcpy( new_stack_bottom, stack_bottom, (stack_top - stack_bottom)*sizeof(stack_top[0]) );
            stack_top = new_stack_bottom + (stack_top - stack_bottom);
            cvFree( &stack_bottom );
            stack_bottom = new_stack_bottom;
        }

        for( j = 0; j < size.width; j++ )
        {
            #define CANNY_SHIFT 15
            #define TG22  (int)(0.4142135623730950488016887242097*(1<<CANNY_SHIFT) + 0.5)

            x = _dx[j];
            y = _dy[j];
            int s = x ^ y;
            int m = _mag[j];

            x = abs(x);
            y = abs(y);
            if( m > low )
            {
                int tg22x = x * TG22;
                int tg67x = tg22x + ((x + x) << CANNY_SHIFT);

                y <<= CANNY_SHIFT;

                if( y < tg22x )
                {
                    if( m > _mag[j-1] && m >= _mag[j+1] )
                    {
                        if( m > high && !prev_flag && _map[j-mapstep] != 2 )
                        {
                            CANNY_PUSH( _map + j );
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
                else if( y > tg67x )
                {
                    if( m > _mag[j+magstep2] && m >= _mag[j+magstep1] )
                    {
                        if( m > high && !prev_flag && _map[j-mapstep] != 2 )
                        {
                            CANNY_PUSH( _map + j );
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
                else
                {
                    s = s < 0 ? -1 : 1;
                    if( m > _mag[j+magstep2-s] && m > _mag[j+magstep1+s] )
                    {
                        if( m > high && !prev_flag && _map[j-mapstep] != 2 )
                        {
                            CANNY_PUSH( _map + j );
                            prev_flag = 1;
                        }
                        else
                            _map[j] = (uchar)0;
                        continue;
                    }
                }
            }
            prev_flag = 0;
            _map[j] = (uchar)1;
        }

        // scroll the ring buffer
        _mag = mag_buf[0];
        mag_buf[0] = mag_buf[1];
        mag_buf[1] = mag_buf[2];
        mag_buf[2] = _mag;
    }

    // now track the edges (hysteresis thresholding)
    while( stack_top > stack_bottom )
    {
        uchar* m;
        if( (stack_top - stack_bottom) + 8 > maxsize )
        {
            uchar** new_stack_bottom;
            maxsize = MAX( maxsize * 3/2, maxsize + 8 );
            CV_CALL( new_stack_bottom = (uchar**)cvAlloc( maxsize * sizeof(stack_top[0])) );
            memcpy( new_stack_bottom, stack_bottom, (stack_top - stack_bottom)*sizeof(stack_top[0]) );
            stack_top = new_stack_bottom + (stack_top - stack_bottom);
            cvFree( &stack_bottom );
            stack_bottom = new_stack_bottom;
        }

        CANNY_POP(m);
    
        if( !m[-1] )
            CANNY_PUSH( m - 1 );
        if( !m[1] )
            CANNY_PUSH( m + 1 );
        if( !m[-mapstep-1] )
            CANNY_PUSH( m - mapstep - 1 );
        if( !m[-mapstep] )
            CANNY_PUSH( m - mapstep );
        if( !m[-mapstep+1] )
            CANNY_PUSH( m - mapstep + 1 );
        if( !m[mapstep-1] )
            CANNY_PUSH( m + mapstep - 1 );
        if( !m[mapstep] )
            CANNY_PUSH( m + mapstep );
        if( !m[mapstep+1] )
            CANNY_PUSH( m + mapstep + 1 );
    }

    // the final pass, form the final image
    for( i = 0; i < size.height; i++ )
    {
        const uchar* _map = map + mapstep*(i+1) + 1;
        uchar* _dst = dst->data.ptr + dst->step*i;
        
        for( j = 0; j < size.width; j++ )
            _dst[j] = (uchar)-(_map[j] >> 1);
    }

    __END__;

    cvReleaseMat( &dx );
    cvReleaseMat( &dy );
    cvFree( &buffer );
    cvFree( &stack_bottom );
}



// /****************************************************************************************\
//                              Sobel & Scharr Derivative Filters
// \****************************************************************************************/
// 
// /////////////////////////////// Old IPP derivative filters ///////////////////////////////
// // still used in corner detectors (see cvcorner.cpp)
// 
icvFilterSobelVert_8u16s_C1R_t icvFilterSobelVert_8u16s_C1R_p = 0;
icvFilterSobelHoriz_8u16s_C1R_t icvFilterSobelHoriz_8u16s_C1R_p = 0;
icvFilterSobelVertSecond_8u16s_C1R_t icvFilterSobelVertSecond_8u16s_C1R_p = 0;
icvFilterSobelHorizSecond_8u16s_C1R_t icvFilterSobelHorizSecond_8u16s_C1R_p = 0;
icvFilterSobelCross_8u16s_C1R_t icvFilterSobelCross_8u16s_C1R_p = 0;
// 
 icvFilterSobelVert_32f_C1R_t icvFilterSobelVert_32f_C1R_p = 0;
 icvFilterSobelHoriz_32f_C1R_t icvFilterSobelHoriz_32f_C1R_p = 0;
 icvFilterSobelVertSecond_32f_C1R_t icvFilterSobelVertSecond_32f_C1R_p = 0;
icvFilterSobelHorizSecond_32f_C1R_t icvFilterSobelHorizSecond_32f_C1R_p = 0;
icvFilterSobelCross_32f_C1R_t icvFilterSobelCross_32f_C1R_p = 0;
// 
 icvFilterScharrVert_8u16s_C1R_t icvFilterScharrVert_8u16s_C1R_p = 0;
icvFilterScharrHoriz_8u16s_C1R_t icvFilterScharrHoriz_8u16s_C1R_p = 0;
icvFilterScharrVert_32f_C1R_t icvFilterScharrVert_32f_C1R_p = 0;
icvFilterScharrHoriz_32f_C1R_t icvFilterScharrHoriz_32f_C1R_p = 0;
// 
// ///////////////////////////////// New IPP derivative filters /////////////////////////////
// 
#define IPCV_FILTER_PTRS( name )                    \
icvFilter##name##GetBufSize_8u16s_C1R_t             \
    icvFilter##name##GetBufSize_8u16s_C1R_p = 0;    \
icvFilter##name##Border_8u16s_C1R_t                 \
    icvFilter##name##Border_8u16s_C1R_p = 0;        \
icvFilter##name##GetBufSize_32f_C1R_t               \
    icvFilter##name##GetBufSize_32f_C1R_p = 0;      \
icvFilter##name##Border_32f_C1R_t                   \
    icvFilter##name##Border_32f_C1R_p = 0;

IPCV_FILTER_PTRS( ScharrHoriz )
IPCV_FILTER_PTRS( ScharrVert )
IPCV_FILTER_PTRS( SobelHoriz )
IPCV_FILTER_PTRS( SobelNegVert )
IPCV_FILTER_PTRS( SobelHorizSecond )
IPCV_FILTER_PTRS( SobelVertSecond )
IPCV_FILTER_PTRS( SobelCross )
IPCV_FILTER_PTRS( Laplacian )
// 
typedef CvStatus (CV_STDCALL * CvDeriv3x3GetBufSizeIPPFunc)
    ( CvSize roi, int* bufsize );
// 
typedef CvStatus (CV_STDCALL * CvDerivGetBufSizeIPPFunc)
    ( CvSize roi, int masksize, int* bufsize );

typedef CvStatus (CV_STDCALL * CvDeriv3x3IPPFunc_8u )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int bordertype, uchar bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDeriv3x3IPPFunc_32f )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int bordertype, float bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDerivIPPFunc_8u )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int masksize, int bordertype,
      uchar bordervalue, void* buffer );

typedef CvStatus (CV_STDCALL * CvDerivIPPFunc_32f )
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, int masksize, int bordertype,
      float bordervalue, void* buffer );
// 
// //////////////////////////////////////////////////////////////////////////////////////////
// 
CV_IMPL void
cvSobel( const void* srcarr, void* dstarr, int dx, int dy, int aperture_size )
{
    CvSepFilter filter;
    void* buffer = 0;
    int local_alloc = 0;

    CV_FUNCNAME( "cvSobel" );

    __BEGIN__;

    int origin = 0;
    int src_type, dst_type;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub ));
    if( !CV_IS_MAT(dst) )
        CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( CV_IS_IMAGE_HDR( srcarr ))
        origin = ((IplImage*)srcarr)->origin;

    src_type = CV_MAT_TYPE( src->type );
    dst_type = CV_MAT_TYPE( dst->type );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsBadArg, "src and dst have different sizes" );

    if( ((aperture_size == CV_SCHARR || aperture_size == 3 || aperture_size == 5) &&
        dx <= 2 && dy <= 2 && dx + dy <= 2 && icvFilterSobelNegVertBorder_8u16s_C1R_p) &&
        (src_type == CV_8UC1 && dst_type == CV_16SC1 ||
        src_type == CV_32FC1 && dst_type == CV_32FC1) )
    {
        CvDerivGetBufSizeIPPFunc ipp_sobel_getbufsize_func = 0;
        CvDerivIPPFunc_8u ipp_sobel_func_8u = 0;
        CvDerivIPPFunc_32f ipp_sobel_func_32f = 0;

        CvDeriv3x3GetBufSizeIPPFunc ipp_scharr_getbufsize_func = 0;
        CvDeriv3x3IPPFunc_8u ipp_scharr_func_8u = 0;
        CvDeriv3x3IPPFunc_32f ipp_scharr_func_32f = 0;

        if( aperture_size == CV_SCHARR )
        {
            if( dx == 1 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_scharr_func_8u = icvFilterScharrVertBorder_8u16s_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrVertGetBufSize_8u16s_C1R_p;
                else
                    ipp_scharr_func_32f = icvFilterScharrVertBorder_32f_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrVertGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_scharr_func_8u = icvFilterScharrHorizBorder_8u16s_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrHorizGetBufSize_8u16s_C1R_p;
                else
                    ipp_scharr_func_32f = icvFilterScharrHorizBorder_32f_C1R_p,
                    ipp_scharr_getbufsize_func = icvFilterScharrHorizGetBufSize_32f_C1R_p;
            }
            else
                CV_ERROR( CV_StsBadArg, "Scharr filter can only be used to compute 1st image derivatives" );
        }
        else
        {
            if( dx == 1 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelNegVertBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelNegVertGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelNegVertBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelNegVertGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelHorizBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelHorizBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizGetBufSize_32f_C1R_p;
            }
            else if( dx == 2 && dy == 0 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelVertSecondBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelVertSecondGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelVertSecondBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelVertSecondGetBufSize_32f_C1R_p;
            }
            else if( dx == 0 && dy == 2 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelHorizSecondBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizSecondGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelHorizSecondBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelHorizSecondGetBufSize_32f_C1R_p;
            }
            else if( dx == 1 && dy == 1 )
            {
                if( src_type == CV_8U )
                    ipp_sobel_func_8u = icvFilterSobelCrossBorder_8u16s_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelCrossGetBufSize_8u16s_C1R_p;
                else
                    ipp_sobel_func_32f = icvFilterSobelCrossBorder_32f_C1R_p,
                    ipp_sobel_getbufsize_func = icvFilterSobelCrossGetBufSize_32f_C1R_p;
            }
        }

        if( (ipp_sobel_func_8u || ipp_sobel_func_32f) && ipp_sobel_getbufsize_func ||
            (ipp_scharr_func_8u || ipp_scharr_func_32f) && ipp_scharr_getbufsize_func )
        {
            int bufsize = 0, masksize = aperture_size == 3 ? 33 : 55;
            CvSize size = cvGetMatSize( src );
            uchar* src_ptr = src->data.ptr;
            uchar* dst_ptr = dst->data.ptr;
            int src_step = src->step ? src->step : CV_STUB_STEP;
            int dst_step = dst->step ? dst->step : CV_STUB_STEP;
            const int bordertype = 1; // replication border
            CvStatus status;

            status = ipp_sobel_getbufsize_func ?
                ipp_sobel_getbufsize_func( size, masksize, &bufsize ) :
                ipp_scharr_getbufsize_func( size, &bufsize );

            if( status >= 0 )
            {
                if( bufsize <= CV_MAX_LOCAL_SIZE )
                {
                    buffer = cvStackAlloc( bufsize );
                    local_alloc = 1;
                }
                else
                    CV_CALL( buffer = cvAlloc( bufsize ));
            
                status =
                    ipp_sobel_func_8u ? ipp_sobel_func_8u( src_ptr, src_step, dst_ptr, dst_step,
                                                           size, masksize, bordertype, 0, buffer ) :
                    ipp_sobel_func_32f ? ipp_sobel_func_32f( src_ptr, src_step, dst_ptr, dst_step,
                                                             size, masksize, bordertype, 0, buffer ) :
                    ipp_scharr_func_8u ? ipp_scharr_func_8u( src_ptr, src_step, dst_ptr, dst_step,
                                                             size, bordertype, 0, buffer ) :
                    ipp_scharr_func_32f ? ipp_scharr_func_32f( src_ptr, src_step, dst_ptr, dst_step,
                                                               size, bordertype, 0, buffer ) : 
                        CV_NOTDEFINED_ERR;
            }

            if( status >= 0 &&
                (dx == 0 && dy == 1 && origin || dx == 1 && dy == 1 && !origin)) // negate the output
                cvSubRS( dst, cvScalarAll(0), dst );

            if( status >= 0 )
                EXIT;
        }
    }

    CV_CALL( filter.init_deriv( src->cols, src_type, dst_type, dx, dy,
                aperture_size, origin ? CvSepFilter::FLIP_KERNEL : 0));
    CV_CALL( filter.process( src, dst ));

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}
#define ICV_WARP_MUL_ONE_8U(x)  ((x) << ICV_WARP_SHIFT)
#define ICV_WARP_DESCALE_8U(x)  CV_DESCALE((x), ICV_WARP_SHIFT*2)
#define ICV_WARP_CLIP_X(x)      ((unsigned)(x) < (unsigned)ssize.width ? \
                                (x) : (x) < 0 ? 0 : ssize.width - 1)
#define ICV_WARP_CLIP_Y(y)      ((unsigned)(y) < (unsigned)ssize.height ? \
                                (y) : (y) < 0 ? 0 : ssize.height - 1)

float icvLinearCoeffs[(ICV_LINEAR_TAB_SIZE+1)*2];

float icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE+1)*2];

void icvInitCubicCoeffTab()
{
    static int inittab = 0;
    if( !inittab )
    {
#if 0
        // classical Mitchell-Netravali filter
        const double B = 1./3;
        const double C = 1./3;
        const double p0 = (6 - 2*B)/6.;
        const double p2 = (-18 + 12*B + 6*C)/6.;
        const double p3 = (12 - 9*B - 6*C)/6.;
        const double q0 = (8*B + 24*C)/6.;
        const double q1 = (-12*B - 48*C)/6.;
        const double q2 = (6*B + 30*C)/6.;
        const double q3 = (-B - 6*C)/6.;

        #define ICV_CUBIC_1(x)  (((x)*p3 + p2)*(x)*(x) + p0)
        #define ICV_CUBIC_2(x)  ((((x)*q3 + q2)*(x) + q1)*(x) + q0)
#else
        // alternative "sharp" filter
        const double A = -0.75;
        #define ICV_CUBIC_1(x)  (((A + 2)*(x) - (A + 3))*(x)*(x) + 1)
        #define ICV_CUBIC_2(x)  (((A*(x) - 5*A)*(x) + 8*A)*(x) - 4*A)
#endif
        for( int i = 0; i <= ICV_CUBIC_TAB_SIZE; i++ )
        {
            float x = (float)i/ICV_CUBIC_TAB_SIZE;
            icvCubicCoeffs[i*2] = (float)ICV_CUBIC_1(x);
            x += 1.f;
            icvCubicCoeffs[i*2+1] = (float)ICV_CUBIC_2(x);
        }

        inittab = 1;
    }
}

// /****************************************************************************************\
// *                                         Resize                                         *
// \****************************************************************************************/
// 
static CvStatus CV_STDCALL
icvResize_NN_8u_C1R( const uchar* src, int srcstep, CvSize ssize,
                     uchar* dst, int dststep, CvSize dsize, int pix_size )
{
    int* x_ofs = (int*)cvStackAlloc( dsize.width * sizeof(x_ofs[0]) );
    int pix_size4 = pix_size / sizeof(int);
    int x, y, t;

    for( x = 0; x < dsize.width; x++ )
    {
        t = (ssize.width*x*2 + MIN(ssize.width, dsize.width) - 1)/(dsize.width*2);
        t -= t >= ssize.width;
        x_ofs[x] = t*pix_size;
    }

    for( y = 0; y < dsize.height; y++, dst += dststep )
    {
        const uchar* tsrc;
        t = (ssize.height*y*2 + MIN(ssize.height, dsize.height) - 1)/(dsize.height*2);
        t -= t >= ssize.height;
        tsrc = src + srcstep*t;

        switch( pix_size )
        {
        case 1:
            for( x = 0; x <= dsize.width - 2; x += 2 )
            {
                uchar t0 = tsrc[x_ofs[x]];
                uchar t1 = tsrc[x_ofs[x+1]];

                dst[x] = t0;
                dst[x+1] = t1;
            }

            for( ; x < dsize.width; x++ )
                dst[x] = tsrc[x_ofs[x]];
            break;
        case 2:
            for( x = 0; x < dsize.width; x++ )
                *(ushort*)(dst + x*2) = *(ushort*)(tsrc + x_ofs[x]);
            break;
        case 3:
            for( x = 0; x < dsize.width; x++ )
            {
                const uchar* _tsrc = tsrc + x_ofs[x];
                dst[x*3] = _tsrc[0]; dst[x*3+1] = _tsrc[1]; dst[x*3+2] = _tsrc[2];
            }
            break;
        case 4:
            for( x = 0; x < dsize.width; x++ )
                *(int*)(dst + x*4) = *(int*)(tsrc + x_ofs[x]);
            break;
        case 6:
            for( x = 0; x < dsize.width; x++ )
            {
                const ushort* _tsrc = (const ushort*)(tsrc + x_ofs[x]);
                ushort* _tdst = (ushort*)(dst + x*6);
                _tdst[0] = _tsrc[0]; _tdst[1] = _tsrc[1]; _tdst[2] = _tsrc[2];
            }
            break;
        default:
            for( x = 0; x < dsize.width; x++ )
                CV_MEMCPY_INT( dst + x*pix_size, tsrc + x_ofs[x], pix_size4 );
        }
    }

    return CV_OK;
}
// 
// 
typedef struct CvResizeAlpha
{
    int idx;
    union
    {
        float alpha;
        int ialpha;
    };
}
CvResizeAlpha;

// 
#define  ICV_DEF_RESIZE_BILINEAR_FUNC( flavor, arrtype, worktype, alpha_field,  \
                                       mul_one_macro, descale_macro )           \
static CvStatus CV_STDCALL                                                      \
icvResize_Bilinear_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                                   arrtype* dst, int dststep, CvSize dsize,     \
                                   int cn, int xmax,                            \
                                   const CvResizeAlpha* xofs,                   \
                                   const CvResizeAlpha* yofs,                   \
                                   worktype* buf0, worktype* buf1 )             \
{                                                                               \
    int prev_sy0 = -1, prev_sy1 = -1;                                           \
    int k, dx, dy;                                                              \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
    xmax *= cn;                                                                 \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
    {                                                                           \
        worktype fy = yofs[dy].alpha_field, *swap_t;                            \
        int sy0 = yofs[dy].idx, sy1 = sy0 + (fy > 0 && sy0 < ssize.height-1);   \
                                                                                \
        if( sy0 == prev_sy0 && sy1 == prev_sy1 )                                \
            k = 2;                                                              \
        else if( sy0 == prev_sy1 )                                              \
        {                                                                       \
            CV_SWAP( buf0, buf1, swap_t );                                      \
            k = 1;                                                              \
        }                                                                       \
        else                                                                    \
            k = 0;                                                              \
                                                                                \
        for( ; k < 2; k++ )                                                     \
        {                                                                       \
            worktype* _buf = k == 0 ? buf0 : buf1;                              \
            const arrtype* _src;                                                \
            int sy = k == 0 ? sy0 : sy1;                                        \
            if( k == 1 && sy1 == sy0 )                                          \
            {                                                                   \
                memcpy( buf1, buf0, dsize.width*sizeof(buf0[0]) );              \
                continue;                                                       \
            }                                                                   \
                                                                                \
            _src = src + sy*srcstep;                                            \
            for( dx = 0; dx < xmax; dx++ )                                      \
            {                                                                   \
                int sx = xofs[dx].idx;                                          \
                worktype fx = xofs[dx].alpha_field;                             \
                worktype t = _src[sx];                                          \
                _buf[dx] = mul_one_macro(t) + fx*(_src[sx+cn] - t);             \
            }                                                                   \
                                                                                \
            for( ; dx < dsize.width; dx++ )                                     \
                _buf[dx] = mul_one_macro(_src[xofs[dx].idx]);                   \
        }                                                                       \
                                                                                \
        prev_sy0 = sy0;                                                         \
        prev_sy1 = sy1;                                                         \
                                                                                \
        if( sy0 == sy1 )                                                        \
            for( dx = 0; dx < dsize.width; dx++ )                               \
                dst[dx] = (arrtype)descale_macro( mul_one_macro(buf0[dx]));     \
        else                                                                    \
            for( dx = 0; dx < dsize.width; dx++ )                               \
                dst[dx] = (arrtype)descale_macro( mul_one_macro(buf0[dx]) +     \
                                                  fy*(buf1[dx] - buf0[dx]));    \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}
// 

typedef struct CvDecimateAlpha
{
    int si, di;
    float alpha;
}
CvDecimateAlpha;


#define  ICV_DEF_RESIZE_AREA_FAST_FUNC( flavor, arrtype, worktype, cast_macro ) \
static CvStatus CV_STDCALL                                                      \
icvResize_AreaFast_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                              arrtype* dst, int dststep, CvSize dsize, int cn,  \
                              const int* ofs, const int* xofs )                 \
{                                                                               \
    int dy, dx, k = 0;                                                          \
    int scale_x = ssize.width/dsize.width;                                      \
    int scale_y = ssize.height/dsize.height;                                    \
    int area = scale_x*scale_y;                                                 \
    float scale = 1.f/(scale_x*scale_y);                                        \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
        for( dx = 0; dx < dsize.width; dx++ )                                   \
        {                                                                       \
            const arrtype* _src = src + dy*scale_y*srcstep + xofs[dx];          \
            worktype sum = 0;                                                   \
                                                                                \
            for( k = 0; k <= area - 4; k += 4 )                                 \
                sum += _src[ofs[k]] + _src[ofs[k+1]] +                          \
                       _src[ofs[k+2]] + _src[ofs[k+3]];                         \
                                                                                \
            for( ; k < area; k++ )                                              \
                sum += _src[ofs[k]];                                            \
                                                                                \
            dst[dx] = (arrtype)cast_macro( sum*scale );                         \
        }                                                                       \
                                                                                \
    return CV_OK;                                                               \
}

// 
#define  ICV_DEF_RESIZE_AREA_FUNC( flavor, arrtype, load_macro, cast_macro )    \
static CvStatus CV_STDCALL                                                      \
icvResize_Area_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,   \
                               arrtype* dst, int dststep, CvSize dsize,         \
                               int cn, const CvDecimateAlpha* xofs,             \
                               int xofs_count, float* buf, float* sum )         \
{                                                                               \
    int k, sy, dx, cur_dy = 0;                                                  \
    float scale_y = (float)ssize.height/dsize.height;                           \
                                                                                \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
    dsize.width *= cn;                                                          \
                                                                                \
    for( sy = 0; sy < ssize.height; sy++, src += srcstep )                      \
    {                                                                           \
        if( cn == 1 )                                                           \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                buf[dxn] = buf[dxn] + load_macro(src[xofs[k].si])*alpha;        \
            }                                                                   \
        else if( cn == 2 )                                                      \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1;                                 \
            }                                                                   \
        else if( cn == 3 )                                                      \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                float t2 = buf[dxn+2] + load_macro(src[sxn+2])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1; buf[dxn+2] = t2;                \
            }                                                                   \
        else                                                                    \
            for( k = 0; k < xofs_count; k++ )                                   \
            {                                                                   \
                int sxn = xofs[k].si;                                           \
                int dxn = xofs[k].di;                                           \
                float alpha = xofs[k].alpha;                                    \
                float t0 = buf[dxn] + load_macro(src[sxn])*alpha;               \
                float t1 = buf[dxn+1] + load_macro(src[sxn+1])*alpha;           \
                buf[dxn] = t0; buf[dxn+1] = t1;                                 \
                t0 = buf[dxn+2] + load_macro(src[sxn+2])*alpha;                 \
                t1 = buf[dxn+3] + load_macro(src[sxn+3])*alpha;                 \
                buf[dxn+2] = t0; buf[dxn+3] = t1;                               \
            }                                                                   \
                                                                                \
        if( (cur_dy + 1)*scale_y <= sy + 1 || sy == ssize.height - 1 )          \
        {                                                                       \
            float beta = sy + 1 - (cur_dy+1)*scale_y, beta1;                    \
            beta = MAX( beta, 0 );                                              \
            beta1 = 1 - beta;                                                   \
            if( fabs(beta) < 1e-3 )                                             \
                for( dx = 0; dx < dsize.width; dx++ )                           \
                {                                                               \
                    dst[dx] = (arrtype)cast_macro(sum[dx] + buf[dx]);           \
                    sum[dx] = buf[dx] = 0;                                      \
                }                                                               \
            else                                                                \
                for( dx = 0; dx < dsize.width; dx++ )                           \
                {                                                               \
                    dst[dx] = (arrtype)cast_macro(sum[dx] + buf[dx]*beta1);     \
                    sum[dx] = buf[dx]*beta;                                     \
                    buf[dx] = 0;                                                \
                }                                                               \
            dst += dststep;                                                     \
            cur_dy++;                                                           \
        }                                                                       \
        else                                                                    \
            for( dx = 0; dx < dsize.width; dx += 2 )                            \
            {                                                                   \
                float t0 = sum[dx] + buf[dx];                                   \
                float t1 = sum[dx+1] + buf[dx+1];                               \
                sum[dx] = t0; sum[dx+1] = t1;                                   \
                buf[dx] = buf[dx+1] = 0;                                        \
            }                                                                   \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


#define  ICV_DEF_RESIZE_BICUBIC_FUNC( flavor, arrtype, worktype, load_macro,    \
                                      cast_macro1, cast_macro2 )                \
static CvStatus CV_STDCALL                                                      \
icvResize_Bicubic_##flavor##_CnR( const arrtype* src, int srcstep, CvSize ssize,\
                                  arrtype* dst, int dststep, CvSize dsize,      \
                                  int cn, int xmin, int xmax,                   \
                                  const CvResizeAlpha* xofs, float** buf )      \
{                                                                               \
    float scale_y = (float)ssize.height/dsize.height;                           \
    int dx, dy, sx, sy, sy2, ify;                                               \
    int prev_sy2 = -2;                                                          \
                                                                                \
    xmin *= cn; xmax *= cn;                                                     \
    dsize.width *= cn;                                                          \
    ssize.width *= cn;                                                          \
    srcstep /= sizeof(src[0]);                                                  \
    dststep /= sizeof(dst[0]);                                                  \
                                                                                \
    for( dy = 0; dy < dsize.height; dy++, dst += dststep )                      \
    {                                                                           \
        float w0, w1, w2, w3;                                                   \
        float fy, x, sum;                                                       \
        float *row, *row0, *row1, *row2, *row3;                                 \
        int k1, k = 4;                                                          \
                                                                                \
        fy = dy*scale_y;                                                        \
        sy = cvFloor(fy);                                                       \
        fy -= sy;                                                               \
        ify = cvRound(fy*ICV_CUBIC_TAB_SIZE);                                   \
        sy2 = sy + 2;                                                           \
                                                                                \
        if( sy2 > prev_sy2 )                                                    \
        {                                                                       \
            int delta = prev_sy2 - sy + 2;                                      \
            for( k = 0; k < delta; k++ )                                        \
                CV_SWAP( buf[k], buf[k+4-delta], row );                         \
        }                                                                       \
                                                                                \
        for( sy += k - 1; k < 4; k++, sy++ )                                    \
        {                                                                       \
            const arrtype* _src = src + sy*srcstep;                             \
                                                                                \
            row = buf[k];                                                       \
            if( sy < 0 )                                                        \
                continue;                                                       \
            if( sy >= ssize.height )                                            \
            {                                                                   \
                assert( k > 0 );                                                \
                memcpy( row, buf[k-1], dsize.width*sizeof(row[0]) );            \
                continue;                                                       \
            }                                                                   \
                                                                                \
            for( dx = 0; dx < xmin; dx++ )                                      \
            {                                                                   \
                int ifx = xofs[dx].ialpha, sx0 = xofs[dx].idx;                  \
                sx = sx0 + cn*2;                                                \
                while( sx >= ssize.width )                                      \
                    sx -= cn;                                                   \
                x = load_macro(_src[sx]);                                       \
                sum = x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2 + 1];       \
                if( (unsigned)(sx = sx0 + cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2];          \
                if( (unsigned)(sx = sx0) < (unsigned)ssize.width )              \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[ifx*2];                                 \
                if( (unsigned)(sx = sx0 - cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                row[dx] = sum + x*icvCubicCoeffs[ifx*2 + 1];                    \
            }                                                                   \
                                                                                \
            for( ; dx < xmax; dx++ )                                            \
            {                                                                   \
                int ifx = xofs[dx].ialpha;                                      \
                int sx0 = xofs[dx].idx;                                         \
                row[dx] = _src[sx0 - cn]*icvCubicCoeffs[ifx*2 + 1] +            \
                    _src[sx0]*icvCubicCoeffs[ifx*2] +                           \
                    _src[sx0 + cn]*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2] + \
                    _src[sx0 + cn*2]*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1];\
            }                                                                   \
                                                                                \
            for( ; dx < dsize.width; dx++ )                                     \
            {                                                                   \
                int ifx = xofs[dx].ialpha, sx0 = xofs[dx].idx;                  \
                x = load_macro(_src[sx0 - cn]);                                 \
                sum = x*icvCubicCoeffs[ifx*2 + 1];                              \
                if( (unsigned)(sx = sx0) < (unsigned)ssize.width )              \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[ifx*2];                                 \
                if( (unsigned)(sx = sx0 + cn) < (unsigned)ssize.width )         \
                    x = load_macro(_src[sx]);                                   \
                sum += x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ifx)*2];          \
                if( (unsigned)(sx = sx0 + cn*2) < (unsigned)ssize.width )       \
                    x = load_macro(_src[sx]);                                   \
                row[dx] = sum + x*icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE-ifx)*2+1]; \
            }                                                                   \
                                                                                \
            if( sy == 0 )                                                       \
                for( k1 = 0; k1 < k; k1++ )                                     \
                    memcpy( buf[k1], row, dsize.width*sizeof(row[0]));          \
        }                                                                       \
                                                                                \
        prev_sy2 = sy2;                                                         \
                                                                                \
        row0 = buf[0]; row1 = buf[1];                                           \
        row2 = buf[2]; row3 = buf[3];                                           \
                                                                                \
        w0 = icvCubicCoeffs[ify*2+1];                                           \
        w1 = icvCubicCoeffs[ify*2];                                             \
        w2 = icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ify)*2];                      \
        w3 = icvCubicCoeffs[(ICV_CUBIC_TAB_SIZE - ify)*2 + 1];                  \
                                                                                \
        for( dx = 0; dx < dsize.width; dx++ )                                   \
        {                                                                       \
            worktype val = cast_macro1( row0[dx]*w0 + row1[dx]*w1 +             \
                                        row2[dx]*w2 + row3[dx]*w3 );            \
            dst[dx] = cast_macro2(val);                                         \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}
// 
// 
ICV_DEF_RESIZE_BILINEAR_FUNC( 8u, uchar, int, ialpha,
                              ICV_WARP_MUL_ONE_8U, ICV_WARP_DESCALE_8U )
ICV_DEF_RESIZE_BILINEAR_FUNC( 16u, ushort, float, alpha, CV_NOP, cvRound )
ICV_DEF_RESIZE_BILINEAR_FUNC( 32f, float, float, alpha, CV_NOP, CV_NOP )

ICV_DEF_RESIZE_BICUBIC_FUNC( 8u, uchar, int, CV_8TO32F, cvRound, CV_CAST_8U )
ICV_DEF_RESIZE_BICUBIC_FUNC( 16u, ushort, int, CV_NOP, cvRound, CV_CAST_16U )
ICV_DEF_RESIZE_BICUBIC_FUNC( 32f, float, float, CV_NOP, CV_NOP, CV_NOP )

ICV_DEF_RESIZE_AREA_FAST_FUNC( 8u, uchar, int, cvRound )
ICV_DEF_RESIZE_AREA_FAST_FUNC( 16u, ushort, int, cvRound )
ICV_DEF_RESIZE_AREA_FAST_FUNC( 32f, float, float, CV_NOP )

ICV_DEF_RESIZE_AREA_FUNC( 8u, uchar, CV_8TO32F, cvRound )
ICV_DEF_RESIZE_AREA_FUNC( 16u, ushort, CV_NOP, cvRound )
ICV_DEF_RESIZE_AREA_FUNC( 32f, float, CV_NOP, CV_NOP )


static void icvInitResizeTab( CvFuncTable* bilin_tab,
                              CvFuncTable* bicube_tab,
                              CvFuncTable* areafast_tab,
                              CvFuncTable* area_tab )
{
    bilin_tab->fn_2d[CV_8U] = (void*)icvResize_Bilinear_8u_CnR;
    bilin_tab->fn_2d[CV_16U] = (void*)icvResize_Bilinear_16u_CnR;
    bilin_tab->fn_2d[CV_32F] = (void*)icvResize_Bilinear_32f_CnR;

    bicube_tab->fn_2d[CV_8U] = (void*)icvResize_Bicubic_8u_CnR;
    bicube_tab->fn_2d[CV_16U] = (void*)icvResize_Bicubic_16u_CnR;
    bicube_tab->fn_2d[CV_32F] = (void*)icvResize_Bicubic_32f_CnR;

    areafast_tab->fn_2d[CV_8U] = (void*)icvResize_AreaFast_8u_CnR;
    areafast_tab->fn_2d[CV_16U] = (void*)icvResize_AreaFast_16u_CnR;
    areafast_tab->fn_2d[CV_32F] = (void*)icvResize_AreaFast_32f_CnR;

    area_tab->fn_2d[CV_8U] = (void*)icvResize_Area_8u_CnR;
    area_tab->fn_2d[CV_16U] = (void*)icvResize_Area_16u_CnR;
    area_tab->fn_2d[CV_32F] = (void*)icvResize_Area_32f_CnR;
}
// 
// 
typedef CvStatus (CV_STDCALL * CvResizeBilinearFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, int xmax, const CvResizeAlpha* xofs,
                      const CvResizeAlpha* yofs, float* buf0, float* buf1 );

typedef CvStatus (CV_STDCALL * CvResizeBicubicFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, int xmin, int xmax,
                      const CvResizeAlpha* xofs, float** buf );
// 
typedef CvStatus (CV_STDCALL * CvResizeAreaFastFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, const int* ofs, const int *xofs );
// 
typedef CvStatus (CV_STDCALL * CvResizeAreaFunc)
                    ( const void* src, int srcstep, CvSize ssize,
                      void* dst, int dststep, CvSize dsize,
                      int cn, const CvDecimateAlpha* xofs,
                      int xofs_count, float* buf, float* sum );
// 
// 
// ////////////////////////////////// IPP resize functions //////////////////////////////////

icvResize_8u_C1R_t icvResize_8u_C1R_p = 0;
icvResize_8u_C3R_t icvResize_8u_C3R_p = 0;
icvResize_8u_C4R_t icvResize_8u_C4R_p = 0;
icvResize_16u_C1R_t icvResize_16u_C1R_p = 0;
icvResize_16u_C3R_t icvResize_16u_C3R_p = 0;
icvResize_16u_C4R_t icvResize_16u_C4R_p = 0;
icvResize_32f_C1R_t icvResize_32f_C1R_p = 0;
icvResize_32f_C3R_t icvResize_32f_C3R_p = 0;
icvResize_32f_C4R_t icvResize_32f_C4R_p = 0;

typedef CvStatus (CV_STDCALL * CvResizeIPPFunc)
( const void* src, CvSize srcsize, int srcstep, CvRect srcroi,
  void* dst, int dststep, CvSize dstroi,
  double xfactor, double yfactor, int interpolation );
// 
// //////////////////////////////////////////////////////////////////////////////////////////
// 
CV_IMPL void cvResize( const CvArr* srcarr, CvArr* dstarr, int method )
{
    static CvFuncTable bilin_tab, bicube_tab, areafast_tab, area_tab;
    static int inittab = 0;
    void* temp_buf = 0;

    CV_FUNCNAME( "cvResize" );

    __BEGIN__;
    
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize ssize, dsize;
    float scale_x, scale_y;
    int k, sx, sy, dx, dy;
    int type, depth, cn;
    
    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));
    
    if( CV_ARE_SIZES_EQ( src, dst ))
        CV_CALL( cvCopy( src, dst ));

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !inittab )
    {
        icvInitResizeTab( &bilin_tab, &bicube_tab, &areafast_tab, &area_tab );
        inittab = 1;
    }

    ssize = cvGetMatSize( src );
    dsize = cvGetMatSize( dst );
    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);
    scale_x = (float)ssize.width/dsize.width;
    scale_y = (float)ssize.height/dsize.height;

    if( method == CV_INTER_CUBIC &&
        (MIN(ssize.width, dsize.width) <= 4 ||
        MIN(ssize.height, dsize.height) <= 4) )
        method = CV_INTER_LINEAR;

    if( icvResize_8u_C1R_p &&
        MIN(ssize.width, dsize.width) > 4 &&
        MIN(ssize.height, dsize.height) > 4 )
    {
        CvResizeIPPFunc ipp_func =
            type == CV_8UC1 ? icvResize_8u_C1R_p :
            type == CV_8UC3 ? icvResize_8u_C3R_p :
            type == CV_8UC4 ? icvResize_8u_C4R_p :
            type == CV_16UC1 ? icvResize_16u_C1R_p :
            type == CV_16UC3 ? icvResize_16u_C3R_p :
            type == CV_16UC4 ? icvResize_16u_C4R_p :
            type == CV_32FC1 ? icvResize_32f_C1R_p :
            type == CV_32FC3 ? icvResize_32f_C3R_p :
            type == CV_32FC4 ? icvResize_32f_C4R_p : 0;
        if( ipp_func && (CV_INTER_NN < method && method < CV_INTER_AREA))
        {
            int srcstep = src->step ? src->step : CV_STUB_STEP;
            int dststep = dst->step ? dst->step : CV_STUB_STEP;
            IPPI_CALL( ipp_func( src->data.ptr, ssize, srcstep,
                                 cvRect(0,0,ssize.width,ssize.height),
                                 dst->data.ptr, dststep, dsize,
                                 (double)dsize.width/ssize.width,
                                 (double)dsize.height/ssize.height, 1 << method ));
            EXIT;
        }
    }

    if( method == CV_INTER_NN )
    {
        IPPI_CALL( icvResize_NN_8u_C1R( src->data.ptr, src->step, ssize,
                                        dst->data.ptr, dst->step, dsize,
                                        CV_ELEM_SIZE(src->type)));
    }
    else if( method == CV_INTER_LINEAR || method == CV_INTER_AREA )
    {
        if( method == CV_INTER_AREA &&
            ssize.width >= dsize.width && ssize.height >= dsize.height )
        {
            // "area" method for (scale_x > 1 & scale_y > 1)
            int iscale_x = cvRound(scale_x);
            int iscale_y = cvRound(scale_y);

            if( fabs(scale_x - iscale_x) < DBL_EPSILON &&
                fabs(scale_y - iscale_y) < DBL_EPSILON )
            {
                int area = iscale_x*iscale_y;
                int srcstep = src->step / CV_ELEM_SIZE(depth);
                int* ofs = (int*)cvStackAlloc( (area + dsize.width*cn)*sizeof(int) );
                int* xofs = ofs + area;
                CvResizeAreaFastFunc func = (CvResizeAreaFastFunc)areafast_tab.fn_2d[depth];

                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );
                
                for( sy = 0, k = 0; sy < iscale_y; sy++ )
                    for( sx = 0; sx < iscale_x; sx++ )
                        ofs[k++] = sy*srcstep + sx*cn;

                for( dx = 0; dx < dsize.width; dx++ )
                {
                    sx = dx*iscale_x*cn;
                    for( k = 0; k < cn; k++ )
                        xofs[dx*cn + k] = sx + k;
                }

                IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                                 dst->step, dsize, cn, ofs, xofs ));
            }
            else
            {
                int buf_len = dsize.width*cn + 4, buf_size, xofs_count = 0;
                float scale = 1.f/(scale_x*scale_y);
                float *buf, *sum;
                CvDecimateAlpha* xofs;
                CvResizeAreaFunc func = (CvResizeAreaFunc)area_tab.fn_2d[depth];

                if( !func || cn > 4 )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                buf_size = buf_len*2*sizeof(float) + ssize.width*2*sizeof(CvDecimateAlpha);
                if( buf_size < CV_MAX_LOCAL_SIZE )
                    buf = (float*)cvStackAlloc(buf_size);
                else
                    CV_CALL( temp_buf = buf = (float*)cvAlloc(buf_size));
                sum = buf + buf_len;
                xofs = (CvDecimateAlpha*)(sum + buf_len);

                for( dx = 0, k = 0; dx < dsize.width; dx++ )
                {
                    float fsx1 = dx*scale_x, fsx2 = fsx1 + scale_x;
                    int sx1 = cvCeil(fsx1), sx2 = cvFloor(fsx2);

                    assert( (unsigned)sx1 < (unsigned)ssize.width );

                    if( sx1 > fsx1 )
                    {
                        assert( k < ssize.width*2 );            
                        xofs[k].di = dx*cn;
                        xofs[k].si = (sx1-1)*cn;
                        xofs[k++].alpha = (sx1 - fsx1)*scale;
                    }

                    for( sx = sx1; sx < sx2; sx++ )
                    {
                        assert( k < ssize.width*2 );
                        xofs[k].di = dx*cn;
                        xofs[k].si = sx*cn;
                        xofs[k++].alpha = scale;
                    }

                    if( fsx2 - sx2 > 1e-3 )
                    {
                        assert( k < ssize.width*2 );
                        assert((unsigned)sx2 < (unsigned)ssize.width );
                        xofs[k].di = dx*cn;
                        xofs[k].si = sx2*cn;
                        xofs[k++].alpha = (fsx2 - sx2)*scale;
                    }
                }

                xofs_count = k;
                memset( sum, 0, buf_len*sizeof(float) );
                memset( buf, 0, buf_len*sizeof(float) );

                IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                                 dst->step, dsize, cn, xofs, xofs_count, buf, sum ));
            }
        }
        else // true "area" method for the cases (scale_x > 1 & scale_y < 1) and
             // (scale_x < 1 & scale_y > 1) is not implemented.
             // instead, it is emulated via some variant of bilinear interpolation.
        {
            float inv_scale_x = (float)dsize.width/ssize.width;
            float inv_scale_y = (float)dsize.height/ssize.height;
            int xmax = dsize.width, width = dsize.width*cn, buf_size;
            float *buf0, *buf1;
            CvResizeAlpha *xofs, *yofs;
            int area_mode = method == CV_INTER_AREA;
            float fx, fy;
            CvResizeBilinearFunc func = (CvResizeBilinearFunc)bilin_tab.fn_2d[depth];

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            buf_size = width*2*sizeof(float) + (width + dsize.height)*sizeof(CvResizeAlpha);
            if( buf_size < CV_MAX_LOCAL_SIZE )
                buf0 = (float*)cvStackAlloc(buf_size);
            else
                CV_CALL( temp_buf = buf0 = (float*)cvAlloc(buf_size));
            buf1 = buf0 + width;
            xofs = (CvResizeAlpha*)(buf1 + width);
            yofs = xofs + width;

            for( dx = 0; dx < dsize.width; dx++ )
            {
                if( !area_mode )
                {
                    fx = (float)((dx+0.5)*scale_x - 0.5);
                    sx = cvFloor(fx);
                    fx -= sx;
                }
                else
                {
                    sx = cvFloor(dx*scale_x);
                    fx = (dx+1) - (sx+1)*inv_scale_x;
                    fx = fx <= 0 ? 0.f : fx - cvFloor(fx);
                }

                if( sx < 0 )
                    fx = 0, sx = 0;

                if( sx >= ssize.width-1 )
                {
                    fx = 0, sx = ssize.width-1;
                    if( xmax >= dsize.width )
                        xmax = dx;
                }

                if( depth != CV_8U )
                    for( k = 0, sx *= cn; k < cn; k++ )
                        xofs[dx*cn + k].idx = sx + k, xofs[dx*cn + k].alpha = fx;
                else
                    for( k = 0, sx *= cn; k < cn; k++ )
                        xofs[dx*cn + k].idx = sx + k,
                        xofs[dx*cn + k].ialpha = CV_FLT_TO_FIX(fx, ICV_WARP_SHIFT);
            }

            for( dy = 0; dy < dsize.height; dy++ )
            {
                if( !area_mode )
                {
                    fy = (float)((dy+0.5)*scale_y - 0.5);
                    sy = cvFloor(fy);
                    fy -= sy;
                    if( sy < 0 )
                        sy = 0, fy = 0;
                }
                else
                {
                    sy = cvFloor(dy*scale_y);
                    fy = (dy+1) - (sy+1)*inv_scale_y;
                    fy = fy <= 0 ? 0.f : fy - cvFloor(fy);
                }

                yofs[dy].idx = sy;
                if( depth != CV_8U )
                    yofs[dy].alpha = fy;
                else
                    yofs[dy].ialpha = CV_FLT_TO_FIX(fy, ICV_WARP_SHIFT);
            }

            IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                             dst->step, dsize, cn, xmax, xofs, yofs, buf0, buf1 ));
        }
    }
    else if( method == CV_INTER_CUBIC )
    {
        int width = dsize.width*cn, buf_size;
        int xmin = dsize.width, xmax = -1;
        CvResizeAlpha* xofs;
        float* buf[4];
        CvResizeBicubicFunc func = (CvResizeBicubicFunc)bicube_tab.fn_2d[depth];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        
        buf_size = width*(4*sizeof(float) + sizeof(xofs[0]));
        if( buf_size < CV_MAX_LOCAL_SIZE )
            buf[0] = (float*)cvStackAlloc(buf_size);
        else
            CV_CALL( temp_buf = buf[0] = (float*)cvAlloc(buf_size));

        for( k = 1; k < 4; k++ )
            buf[k] = buf[k-1] + width;
        xofs = (CvResizeAlpha*)(buf[3] + width);

        icvInitCubicCoeffTab();

        for( dx = 0; dx < dsize.width; dx++ )
        {
            float fx = dx*scale_x;
            sx = cvFloor(fx);
            fx -= sx;
            int ifx = cvRound(fx*ICV_CUBIC_TAB_SIZE);
            if( sx-1 >= 0 && xmin > dx )
                xmin = dx;
            if( sx+2 < ssize.width )
                xmax = dx + 1;
        
            // at least one of 4 points should be within the image - to
            // be able to set other points to the same value. see the loops
            // for( dx = 0; dx < xmin; dx++ ) ... and for( ; dx < width; dx++ ) ...
            if( sx < -2 )
                sx = -2;
            else if( sx > ssize.width )
                sx = ssize.width;

            for( k = 0; k < cn; k++ )
            {
                xofs[dx*cn + k].idx = sx*cn + k;
                xofs[dx*cn + k].ialpha = ifx;
            }
        }
    
        IPPI_CALL( func( src->data.ptr, src->step, ssize, dst->data.ptr,
                         dst->step, dsize, cn, xmin, xmax, xofs, buf ));
    }
    else
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported interpolation method" );

    __END__;

    cvFree( &temp_buf );
}

