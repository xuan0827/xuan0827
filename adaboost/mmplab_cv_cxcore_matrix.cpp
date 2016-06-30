#include "stdafx.h"
#include "mmplab_cxcore.h"
// 
// /****************************************************************************************\
// *                           [scaled] Identity matrix initialization                      *
// \****************************************************************************************/
// 
CV_IMPL void
cvSetIdentity( CvArr* array, CvScalar value )
{
    CV_FUNCNAME( "cvSetIdentity" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)array;
    CvSize size;
    int i, k, len, step;
    int type, pix_size;
    uchar* data = 0;
    double buf[4];

    if( !CV_IS_MAT( mat ))
    {
        int coi = 0;
        CV_CALL( mat = cvGetMat( mat, &stub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    size = cvGetMatSize( mat );
    len = CV_IMIN( size.width, size.height );

    type = CV_MAT_TYPE(mat->type);
    pix_size = CV_ELEM_SIZE(type);
    size.width *= pix_size;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;
        size.height = 1;
    }

    data = mat->data.ptr;
    step = mat->step;
    if( step == 0 )
        step = CV_STUB_STEP;
    IPPI_CALL( icvSetZero_8u_C1R( data, step, size ));
    step += pix_size;

    if( type == CV_32FC1 )
    {
        float val = (float)value.val[0];
        float* _data = (float*)data;
        step /= sizeof(_data[0]);
        len *= step;

        for( i = 0; i < len; i += step )
            _data[i] = val;
    }
    else if( type == CV_64FC1 )
    {
        double val = value.val[0];
        double* _data = (double*)data;
        step /= sizeof(_data[0]);
        len *= step;

        for( i = 0; i < len; i += step )
            _data[i] = val;
    }
    else
    {
        uchar* val_ptr = (uchar*)buf;
        cvScalarToRawData( &value, buf, type, 0 );
        len *= step;

        for( i = 0; i < len; i += step )
            for( k = 0; k < pix_size; k++ )
                data[i+k] = val_ptr[k];
    }

    __END__;
}
// 
// 
// /****************************************************************************************\
// *                                    Trace of the matrix                                 *
// \****************************************************************************************/
// 
// CV_IMPL CvScalar
// cvTrace( const CvArr* array )
// {
//     CvScalar sum = {{0,0,0,0}};
//     
//     CV_FUNCNAME( "cvTrace" );
// 
//     __BEGIN__;
// 
//     CvMat stub, *mat = 0;
// 
//     if( CV_IS_MAT( array ))
//     {
//         mat = (CvMat*)array;
//         int type = CV_MAT_TYPE(mat->type);
//         int size = MIN(mat->rows,mat->cols);
//         uchar* data = mat->data.ptr;
// 
//         if( type == CV_32FC1 )
//         {
//             int step = mat->step + sizeof(float);
// 
//             for( ; size--; data += step )
//                 sum.val[0] += *(float*)data;
//             EXIT;
//         }
//         
//         if( type == CV_64FC1 )
//         {
//             int step = mat->step + sizeof(double);
// 
//             for( ; size--; data += step )
//                 sum.val[0] += *(double*)data;
//             EXIT;
//         }
//     }
// 
//     CV_CALL( mat = cvGetDiag( array, &stub ));
//     CV_CALL( sum = cvSum( mat ));
// 
//     __END__;
// 
//     return sum;
// }
// 
// 
// /****************************************************************************************\
// *                                     Matrix transpose                                   *
// \****************************************************************************************/
// 
// /////////////////// macros for inplace transposition of square matrix ////////////////////
// 
#define ICV_DEF_TRANSP_INP_CASE_C1( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    step /= sizeof(arr[0]);         \
                                    \
    while( --len )                  \
    {                               \
        arr += step, arr1++;        \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        do                          \
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
                                    \
            arr2++;                 \
            arr3 += step;           \
        }                           \
        while( arr2 != arr3  );     \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C3( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 3;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=3, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
        }                           \
    }                               \
}


#define ICV_DEF_TRANSP_INP_CASE_C4( \
    arrtype, len )                  \
{                                   \
    arrtype* arr1 = arr;            \
    int y;                          \
    step /= sizeof(arr[0]);         \
                                    \
    for( y = 1; y < len; y++ )      \
    {                               \
        arr += step, arr1 += 4;     \
        arrtype* arr2 = arr;        \
        arrtype* arr3 = arr1;       \
                                    \
        for( ; arr2!=arr3; arr2+=4, \
                        arr3+=step )\
        {                           \
            arrtype t0 = arr2[0];   \
            arrtype t1 = arr3[0];   \
            arr2[0] = t1;           \
            arr3[0] = t0;           \
            t0 = arr2[1];           \
            t1 = arr3[1];           \
            arr2[1] = t1;           \
            arr3[1] = t0;           \
            t0 = arr2[2];           \
            t1 = arr3[2];           \
            arr2[2] = t1;           \
            arr3[2] = t0;           \
            t0 = arr2[3];           \
            t1 = arr3[3];           \
            arr2[3] = t1;           \
            arr3[3] = t0;           \
        }                           \
    }                               \
}


// //////////////// macros for non-inplace transposition of rectangular matrix //////////////

#define ICV_DEF_TRANSP_CASE_C1( arrtype )       \
{                                               \
    int x, y;                                   \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( y = 0; y <= size.height - 2; y += 2,   \
                src += 2*srcstep, dst += 2 )    \
    {                                           \
        const arrtype* src1 = src + srcstep;    \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += dststep )       \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1 += dststep;                    \
                                                \
            t0 = src[x + 1];                    \
            t1 = src1[x + 1];                   \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src1[x];               \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
        }                                       \
    }                                           \
                                                \
    if( y < size.height )                       \
    {                                           \
        arrtype* dst1 = dst;                    \
        for( x = 0; x <= size.width - 2;        \
                x += 2, dst1 += 2*dststep )     \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            dst1[0] = t0;                       \
            dst1[dststep] = t1;                 \
        }                                       \
                                                \
        if( x < size.width )                    \
        {                                       \
            arrtype t0 = src[x];                \
            dst1[0] = t0;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C3( arrtype )       \
{                                               \
    size.width *= 3;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=3 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 3,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
            arrtype t2 = src[x + 2];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
            dst1[2] = t2;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_CASE_C4( arrtype )       \
{                                               \
    size.width *= 4;                            \
    srcstep /= sizeof(src[0]);                  \
    dststep /= sizeof(dst[0]);                  \
                                                \
    for( ; size.height--; src+=srcstep, dst+=4 )\
    {                                           \
        int x;                                  \
        arrtype* dst1 = dst;                    \
                                                \
        for( x = 0; x < size.width; x += 4,     \
                            dst1 += dststep )   \
        {                                       \
            arrtype t0 = src[x];                \
            arrtype t1 = src[x + 1];            \
                                                \
            dst1[0] = t0;                       \
            dst1[1] = t1;                       \
                                                \
            t0 = src[x + 2];                    \
            t1 = src[x + 3];                    \
                                                \
            dst1[2] = t0;                       \
            dst1[3] = t1;                       \
        }                                       \
    }                                           \
}


#define ICV_DEF_TRANSP_INP_FUNC( flavor, arrtype, cn )      \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( arrtype* arr, int step, CvSize size )\
{                                                           \
    assert( size.width == size.height );                    \
                                                            \
    ICV_DEF_TRANSP_INP_CASE_C##cn( arrtype, size.width )    \
    return CV_OK;                                           \
}


#define ICV_DEF_TRANSP_FUNC( flavor, arrtype, cn )          \
static CvStatus CV_STDCALL                                  \
icvTranspose_##flavor( const arrtype* src, int srcstep,     \
                    arrtype* dst, int dststep, CvSize size )\
{                                                           \
    ICV_DEF_TRANSP_CASE_C##cn( arrtype )                    \
    return CV_OK;                                           \
}


ICV_DEF_TRANSP_INP_FUNC( 8u_C1IR, uchar, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C2IR, ushort, 1 )
ICV_DEF_TRANSP_INP_FUNC( 8u_C3IR, uchar, 3 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C2IR, int, 1 )
ICV_DEF_TRANSP_INP_FUNC( 16u_C3IR, ushort, 3 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C2IR, int64, 1 )
ICV_DEF_TRANSP_INP_FUNC( 32s_C3IR, int, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C2IR, int, 4 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C3IR, int64, 3 )
ICV_DEF_TRANSP_INP_FUNC( 64s_C4IR, int64, 4 )


ICV_DEF_TRANSP_FUNC( 8u_C1R, uchar, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C2R, ushort, 1 )
ICV_DEF_TRANSP_FUNC( 8u_C3R, uchar, 3 )
ICV_DEF_TRANSP_FUNC( 16u_C2R, int, 1 )
ICV_DEF_TRANSP_FUNC( 16u_C3R, ushort, 3 )
ICV_DEF_TRANSP_FUNC( 32s_C2R, int64, 1 )
ICV_DEF_TRANSP_FUNC( 32s_C3R, int, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C2R, int, 4 )
ICV_DEF_TRANSP_FUNC( 64s_C3R, int64, 3 )
ICV_DEF_TRANSP_FUNC( 64s_C4R, int64, 4 )

CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, R )
CV_DEF_INIT_PIXSIZE_TAB_2D( Transpose, IR )
// 
CV_IMPL void
cvTranspose( const CvArr* srcarr, CvArr* dstarr )
{
    static CvBtFuncTable tab, inp_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvTranspose" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize size;
    int type, pix_size;

    if( !inittab )
    {
        icvInitTransposeIRTable( &inp_tab );
        icvInitTransposeRTable( &tab );
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    type = CV_MAT_TYPE( src->type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( src );

    if( dstarr == srcarr )
    {
        dst = src; 
    }
    else
    {
        if( !CV_IS_MAT( dst ))
        {
            int coi = 0;
            CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));

            if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
        }

        if( !CV_ARE_TYPES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( size.width != dst->height || size.height != dst->width )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }

    if( src->data.ptr == dst->data.ptr )
    {
        if( size.width == size.height )
        {
            CvFunc2D_1A func = (CvFunc2D_1A)(inp_tab.fn_2d[pix_size]);

            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            IPPI_CALL( func( src->data.ptr, src->step, size ));
        }
        else
        {
            if( size.width != 1 && size.height != 1 )
                CV_ERROR( CV_StsBadSize,
                    "Rectangular matrix can not be transposed inplace" );
            
            if( !CV_IS_MAT_CONT( src->type & dst->type ))
                CV_ERROR( CV_StsBadFlag, "In case of inplace column/row transposition "
                                       "both source and destination must be continuous" );

            if( dst == src )
            {
                int t;
                CV_SWAP( dst->width, dst->height, t );
                dst->step = dst->height == 1 ? 0 : pix_size;
            }
        }
    }
    else
    {
        CvFunc2D_2A func = (CvFunc2D_2A)(tab.fn_2d[pix_size]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step,
                         dst->data.ptr, dst->step, size ));
    }

    __END__;
}

// 
// /****************************************************************************************\
// *                              LU decomposition/back substitution                        *
// \****************************************************************************************/
// 
#define arrtype float
#define temptype double

typedef  CvStatus (CV_STDCALL * CvLUDecompFunc)( double* A, int stepA, CvSize sizeA,
                                                 void* B, int stepB, CvSize sizeB,
                                                 double* det );
// 
typedef  CvStatus (CV_STDCALL * CvLUBackFunc)( double* A, int stepA, CvSize sizeA,
                                               void* B, int stepB, CvSize sizeB );

// 
#define ICV_DEF_LU_DECOMP_FUNC( flavor, arrtype )                               \
static CvStatus CV_STDCALL                                                      \
icvLUDecomp_##flavor( double* A, int stepA, CvSize sizeA,                       \
                      arrtype* B, int stepB, CvSize sizeB, double* _det )       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = 0, i;                                                               \
    double det = 1;                                                             \
                                                                                \
    assert( sizeA.width == sizeA.height );                                      \
                                                                                \
    if( B )                                                                     \
    {                                                                           \
        assert( sizeA.height == sizeB.height );                                 \
        m = sizeB.width;                                                        \
    }                                                                           \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    for( i = 0; i < n; i++, A += stepA, B += stepB )                            \
    {                                                                           \
        int j, k = i;                                                           \
        double* tA = A;                                                         \
        arrtype* tB = 0;                                                        \
        double kval = fabs(A[i]), tval;                                         \
                                                                                \
        /* find the pivot element */                                            \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tval = fabs(tA[i]);                                                 \
                                                                                \
            if( tval > kval )                                                   \
            {                                                                   \
                kval = tval;                                                    \
                k = j;                                                          \
            }                                                                   \
        }                                                                       \
                                                                                \
        if( kval == 0 )                                                         \
        {                                                                       \
            det = 0;                                                            \
            break;                                                              \
        }                                                                       \
                                                                                \
        /* swap rows */                                                         \
        if( k != i )                                                            \
        {                                                                       \
            tA = A + stepA*(k - i);                                             \
            det = -det;                                                         \
                                                                                \
            for( j = i; j < n; j++ )                                            \
            {                                                                   \
                double t;                                                       \
                CV_SWAP( A[j], tA[j], t );                                      \
            }                                                                   \
                                                                                \
            if( m > 0 )                                                         \
            {                                                                   \
                tB = B + stepB*(k - i);                                         \
                                                                                \
                for( j = 0; j < m; j++ )                                        \
                {                                                               \
                    arrtype t = B[j];                                           \
                    CV_SWAP( B[j], tB[j], t );                                  \
                }                                                               \
            }                                                                   \
        }                                                                       \
                                                                                \
        tval = 1./A[i];                                                         \
        det *= A[i];                                                            \
        tA = A;                                                                 \
        tB = B;                                                                 \
        A[i] = tval; /* to replace division with multiplication in LUBack */    \
                                                                                \
        /* update matrix and the right side of the system */                    \
        for( j = i + 1; j < n; j++ )                                            \
        {                                                                       \
            tA += stepA;                                                        \
            tB += stepB;                                                        \
            double alpha = -tA[i]*tval;                                         \
                                                                                \
            for( k = i + 1; k < n; k++ )                                        \
                tA[k] = tA[k] + alpha*A[k];                                     \
                                                                                \
            if( m > 0 )                                                         \
                for( k = 0; k < m; k++ )                                        \
                    tB[k] = (arrtype)(tB[k] + alpha*B[k]);                      \
        }                                                                       \
    }                                                                           \
                                                                                \
    if( _det )                                                                  \
        *_det = det;                                                            \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_DECOMP_FUNC( 32f, float )
ICV_DEF_LU_DECOMP_FUNC( 64f, double )


#define ICV_DEF_LU_BACK_FUNC( flavor, arrtype )                                 \
static CvStatus CV_STDCALL                                                      \
icvLUBack_##flavor( double* A, int stepA, CvSize sizeA,                         \
                    arrtype* B, int stepB, CvSize sizeB )                       \
{                                                                               \
    int n = sizeA.width;                                                        \
    int m = sizeB.width, i;                                                     \
                                                                                \
    assert( m > 0 && sizeA.width == sizeA.height &&                             \
            sizeA.height == sizeB.height );                                     \
    stepA /= sizeof(A[0]);                                                      \
    stepB /= sizeof(B[0]);                                                      \
                                                                                \
    A += stepA*(n - 1);                                                         \
    B += stepB*(n - 1);                                                         \
                                                                                \
    for( i = n - 1; i >= 0; i--, A -= stepA )                                   \
    {                                                                           \
        int j, k;                                                               \
        for( j = 0; j < m; j++ )                                                \
        {                                                                       \
            arrtype* tB = B + j;                                                \
            double x = 0;                                                       \
                                                                                \
            for( k = n - 1; k > i; k--, tB -= stepB )                           \
                x += A[k]*tB[0];                                                \
                                                                                \
            tB[0] = (arrtype)((tB[0] - x)*A[i]);                                \
        }                                                                       \
    }                                                                           \
                                                                                \
    return CV_OK;                                                               \
}


ICV_DEF_LU_BACK_FUNC( 32f, float )
ICV_DEF_LU_BACK_FUNC( 64f, double )
// 
static CvFuncTable lu_decomp_tab, lu_back_tab;
static int lu_inittab = 0;
// 
static void icvInitLUTable( CvFuncTable* decomp_tab,
                            CvFuncTable* back_tab )
{
    decomp_tab->fn_2d[0] = (void*)icvLUDecomp_32f;
    decomp_tab->fn_2d[1] = (void*)icvLUDecomp_64f;
    back_tab->fn_2d[0] = (void*)icvLUBack_32f;
    back_tab->fn_2d[1] = (void*)icvLUBack_64f;
}
// 
// 
// 
// /****************************************************************************************\
// *                                 Determinant of the matrix                              *
// \****************************************************************************************/
// 
#define det2(m)   (m(0,0)*m(1,1) - m(0,1)*m(1,0))
#define det3(m)   (m(0,0)*(m(1,1)*m(2,2) - m(1,2)*m(2,1)) -  \
                   m(0,1)*(m(1,0)*m(2,2) - m(1,2)*m(2,0)) +  \
                   m(0,2)*(m(1,0)*m(2,1) - m(1,1)*m(2,0)))

CV_IMPL double
cvDet( const CvArr* arr )
{
    double result = 0;
    uchar* buffer = 0;
    int local_alloc = 0;
    
    CV_FUNCNAME( "cvDet" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    int type;

    if( !CV_IS_MAT( mat ))
    {
        CV_CALL( mat = cvGetMat( mat, &stub ));
    }

    type = CV_MAT_TYPE( mat->type );

    if( mat->width != mat->height )
        CV_ERROR( CV_StsBadSize, "The matrix must be square" );

    #define Mf( y, x ) ((float*)(m + y*step))[x]
    #define Md( y, x ) ((double*)(m + y*step))[x]

    if( mat->width == 2 )
    {
        uchar* m = mat->data.ptr;
        int step = mat->step;

        if( type == CV_32FC1 )
        {
            result = det2(Mf);
        }
        else if( type == CV_64FC1 )
        {
            result = det2(Md);
        }
        else
        {
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        }
    }
    else if( mat->width == 3 )
    {
        uchar* m = mat->data.ptr;
        int step = mat->step;
        
        if( type == CV_32FC1 )
        {
            result = det3(Mf);
        }
        else if( type == CV_64FC1 )
        {
            result = det3(Md);
        }
        else
        {
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        }
    }
    else if( mat->width == 1 )
    {
        if( type == CV_32FC1 )
        {
            result = mat->data.fl[0];
        }
        else if( type == CV_64FC1 )
        {
            result = mat->data.db[0];
        }
        else
        {
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        }
    }
    else
    {
        CvLUDecompFunc decomp_func;
        CvSize size = cvGetMatSize( mat );
        const int worktype = CV_64FC1;
        int buf_size = size.width*size.height*CV_ELEM_SIZE(worktype);
        CvMat tmat;

        if( !lu_inittab )
        {
            icvInitLUTable( &lu_decomp_tab, &lu_back_tab );
            lu_inittab = 1;
        }

        if( CV_MAT_CN( type ) != 1 || CV_MAT_DEPTH( type ) < CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( size.width <= CV_MAX_LOCAL_MAT_SIZE )
        {
            buffer = (uchar*)cvStackAlloc( buf_size );
            local_alloc = 1;
        }
        else
        {
            CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
        }

        CV_CALL( cvInitMatHeader( &tmat, size.height, size.width, worktype, buffer ));
        if( type == worktype )
        {
        	CV_CALL( cvCopy( mat, &tmat ));
        }
        else
            CV_CALL( cvConvert( mat, &tmat ));

        decomp_func = (CvLUDecompFunc)(lu_decomp_tab.fn_2d[CV_MAT_DEPTH(worktype)-CV_32F]);
        assert( decomp_func );

        IPPI_CALL( decomp_func( tmat.data.db, tmat.step, size, 0, 0, size, &result ));
    }

    #undef Mf
    #undef Md

    /*icvCheckVector_64f( &result, 1 );*/

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    return result;
}

// /****************************************************************************************\
// *                               3D vector cross-product                                  *
// \****************************************************************************************/
// 
CV_IMPL void
cvCrossProduct( const CvArr* srcAarr, const CvArr* srcBarr, CvArr* dstarr )
{
    CV_FUNCNAME( "cvCrossProduct" );

    __BEGIN__;

    CvMat stubA, *srcA = (CvMat*)srcAarr;
    CvMat stubB, *srcB = (CvMat*)srcBarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    int type;

    if( !CV_IS_MAT(srcA))
        CV_CALL( srcA = cvGetMat( srcA, &stubA ));

    type = CV_MAT_TYPE( srcA->type );

    if( srcA->width*srcA->height*CV_MAT_CN(type) != 3 )
        CV_ERROR( CV_StsBadArg, "All the input arrays must be continuous 3-vectors" );

    if( !srcB || !dst )
        CV_ERROR( CV_StsNullPtr, "" );

    if( (srcA->type & ~CV_MAT_CONT_FLAG) == (srcB->type & ~CV_MAT_CONT_FLAG) &&
        (srcA->type & ~CV_MAT_CONT_FLAG) == (dst->type & ~CV_MAT_CONT_FLAG) )
    {
        if( !srcB->data.ptr || !dst->data.ptr )
            CV_ERROR( CV_StsNullPtr, "" );
    }
    else
    {
        if( !CV_IS_MAT(srcB))
            CV_CALL( srcB = cvGetMat( srcB, &stubB ));

        if( !CV_IS_MAT(dst))
            CV_CALL( dst = cvGetMat( dst, &dstub ));

        if( !CV_ARE_TYPES_EQ( srcA, srcB ) ||
            !CV_ARE_TYPES_EQ( srcB, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );
    }

    if( !CV_ARE_SIZES_EQ( srcA, srcB ) || !CV_ARE_SIZES_EQ( srcB, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( CV_MAT_DEPTH(type) == CV_32F )
    {
        float* dstdata = (float*)(dst->data.ptr);
        const float* src1data = (float*)(srcA->data.ptr);
        const float* src2data = (float*)(srcB->data.ptr);

        if( CV_IS_MAT_CONT(srcA->type & srcB->type & dst->type) )
        {
            dstdata[2] = src1data[0] * src2data[1] - src1data[1] * src2data[0];
            dstdata[0] = src1data[1] * src2data[2] - src1data[2] * src2data[1];
            dstdata[1] = src1data[2] * src2data[0] - src1data[0] * src2data[2];
        }
        else
        {
            int step1 = srcA->step ? srcA->step/sizeof(src1data[0]) : 1;
            int step2 = srcB->step ? srcB->step/sizeof(src1data[0]) : 1;
            int step = dst->step ? dst->step/sizeof(src1data[0]) : 1;

            dstdata[2*step] = src1data[0] * src2data[step2] - src1data[step1] * src2data[0];
            dstdata[0] = src1data[step1] * src2data[step2*2] - src1data[step1*2] * src2data[step2];
            dstdata[step] = src1data[step1*2] * src2data[0] - src1data[0] * src2data[step2*2];
        }
    }
    else if( CV_MAT_DEPTH(type) == CV_64F )
    {
        double* dstdata = (double*)(dst->data.ptr);
        const double* src1data = (double*)(srcA->data.ptr);
        const double* src2data = (double*)(srcB->data.ptr);
        
        if( CV_IS_MAT_CONT(srcA->type & srcB->type & dst->type) )
        {
            dstdata[2] = src1data[0] * src2data[1] - src1data[1] * src2data[0];
            dstdata[0] = src1data[1] * src2data[2] - src1data[2] * src2data[1];
            dstdata[1] = src1data[2] * src2data[0] - src1data[0] * src2data[2];
        }
        else
        {
            int step1 = srcA->step ? srcA->step/sizeof(src1data[0]) : 1;
            int step2 = srcB->step ? srcB->step/sizeof(src1data[0]) : 1;
            int step = dst->step ? dst->step/sizeof(src1data[0]) : 1;

            dstdata[2*step] = src1data[0] * src2data[step2] - src1data[step1] * src2data[0];
            dstdata[0] = src1data[step1] * src2data[step2*2] - src1data[step1*2] * src2data[step2];
            dstdata[step] = src1data[step1*2] * src2data[0] - src1data[0] * src2data[step2*2];
        }
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    __END__;
}

icvBLAS_GEMM_32f_t icvBLAS_GEMM_32f_p = 0;
icvBLAS_GEMM_64f_t icvBLAS_GEMM_64f_p = 0;
icvBLAS_GEMM_32fc_t icvBLAS_GEMM_32fc_p = 0;
icvBLAS_GEMM_64fc_t icvBLAS_GEMM_64fc_p = 0;

static void
icvGEMM_CopyBlock( const uchar* src, int src_step,
                   uchar* dst, int dst_step,
                   CvSize size, int pix_size )
{
    int j;
    size.width = size.width * (pix_size / sizeof(int));

    for( ; size.height--; src += src_step, dst += dst_step )
    {
        for( j = 0; j <= size.width - 4; j += 4 )
        {
            int t0 = ((const int*)src)[j];
            int t1 = ((const int*)src)[j+1];
            ((int*)dst)[j] = t0;
            ((int*)dst)[j+1] = t1;
            t0 = ((const int*)src)[j+2];
            t1 = ((const int*)src)[j+3];
            ((int*)dst)[j+2] = t0;
            ((int*)dst)[j+3] = t1;
        }

        for( ; j < size.width; j++ )
            ((int*)dst)[j] = ((const int*)src)[j];
    }
}


static void
icvGEMM_TransposeBlock( const uchar* src, int src_step,
                        uchar* dst, int dst_step,
                        CvSize size, int pix_size )
{
    int i, j;
    for( i = 0; i < size.width; i++, dst += dst_step, src += pix_size )
    {
        const uchar* _src = src;
        switch( pix_size )
        {
        case sizeof(int):
            for( j = 0; j < size.height; j++, _src += src_step )
                ((int*)dst)[j] = ((int*)_src)[0];
            break;
        case sizeof(int)*2:
            for( j = 0; j < size.height*2; j += 2, _src += src_step )
            {
                int t0 = ((int*)_src)[0];
                int t1 = ((int*)_src)[1];
                ((int*)dst)[j] = t0;
                ((int*)dst)[j+1] = t1;
            }
            break;
        case sizeof(int)*4:
            for( j = 0; j < size.height*4; j += 4, _src += src_step )
            {
                int t0 = ((int*)_src)[0];
                int t1 = ((int*)_src)[1];
                ((int*)dst)[j] = t0;
                ((int*)dst)[j+1] = t1;
                t0 = ((int*)_src)[2];
                t1 = ((int*)_src)[3];
                ((int*)dst)[j+2] = t0;
                ((int*)dst)[j+3] = t1;
            }
            break;
        default:
            assert(0);
            return;
        }
    }
}

#define ICV_DEF_GEMM_SINGLE_MUL( flavor, arrtype, worktype )                \
static CvStatus CV_STDCALL                                                  \
icvGEMMSingleMul_##flavor( const arrtype* a_data, size_t a_step,            \
                         const arrtype* b_data, size_t b_step,              \
                         const arrtype* c_data, size_t c_step,              \
                         arrtype* d_data, size_t d_step,                    \
                         CvSize a_size, CvSize d_size,                      \
                         double alpha, double beta, int flags )             \
{                                                                           \
    int i, j, k, n = a_size.width, m = d_size.width, drows = d_size.height; \
    const arrtype *_a_data = a_data, *_b_data = b_data, *_c_data = c_data;  \
    arrtype* a_buf = 0;                                                     \
    size_t a_step0, a_step1, c_step0, c_step1, t_step;                      \
                                                                            \
    a_step /= sizeof(a_data[0]);                                            \
    b_step /= sizeof(b_data[0]);                                            \
    c_step /= sizeof(c_data[0]);                                            \
    d_step /= sizeof(d_data[0]);                                            \
    a_step0 = a_step;                                                       \
    a_step1 = 1;                                                            \
                                                                            \
    if( !c_data )                                                           \
        c_step0 = c_step1 = 0;                                              \
    else if( !(flags & CV_GEMM_C_T) )                                       \
        c_step0 = c_step, c_step1 = 1;                                      \
    else                                                                    \
        c_step0 = 1, c_step1 = c_step;                                      \
                                                                            \
    if( flags & CV_GEMM_A_T )                                               \
    {                                                                       \
        CV_SWAP( a_step0, a_step1, t_step );                                \
        n = a_size.height;                                                  \
        if( a_step > 1 && n > 1 )                                           \
            a_buf = (arrtype*)cvStackAlloc(n*sizeof(a_data[0]));            \
    }                                                                       \
                                                                            \
    if( n == 1 ) /* external product */                                     \
    {                                                                       \
        arrtype* b_buf = 0;                                                 \
                                                                            \
        if( a_step > 1 )                                                    \
        {                                                                   \
            a_buf = (arrtype*)cvStackAlloc(drows*sizeof(a_data[0]));        \
            for( k = 0; k < drows; k++ )                                    \
                a_buf[k] = a_data[a_step*k];                                \
            a_data = a_buf;                                                 \
        }                                                                   \
                                                                            \
        if( b_step > 1 )                                                    \
        {                                                                   \
            b_buf = (arrtype*)cvStackAlloc(d_size.width*sizeof(b_buf[0]) ); \
            for( j = 0; j < d_size.width; j++ )                             \
                b_buf[j] = b_data[j*b_step];                                \
            b_data = b_buf;                                                 \
        }                                                                   \
                                                                            \
        for( i = 0; i < drows; i++, _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            worktype al = worktype(a_data[i])*alpha;                        \
            c_data = _c_data;                                               \
            for( j = 0; j <= d_size.width - 2; j += 2, c_data += 2*c_step1 )\
            {                                                               \
                worktype s0 = al*b_data[j];                                 \
                worktype s1 = al*b_data[j+1];                               \
                if( !c_data )                                               \
                {                                                           \
                    d_data[j] = arrtype(s0);                                \
                    d_data[j+1] = arrtype(s1);                              \
                }                                                           \
                else                                                        \
                {                                                           \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
                    d_data[j+1] = arrtype(s1 + c_data[c_step1]*beta);       \
                }                                                           \
            }                                                               \
                                                                            \
            for( ; j < d_size.width; j++, c_data += c_step1 )               \
            {                                                               \
                worktype s0 = al*b_data[j];                                 \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else if( flags & CV_GEMM_B_T ) /* A * Bt */                             \
    {                                                                       \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                    _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            a_data = _a_data;                                               \
            b_data = _b_data;                                               \
            c_data = _c_data;                                               \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = a_data[a_step1*k];                           \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j < d_size.width; j++, b_data += b_step,            \
                                               c_data += c_step1 )          \
            {                                                               \
                worktype s0(0), s1(0), s2(0), s3(0);                        \
                                                                            \
                for( k = 0; k <= n - 4; k += 4 )                            \
                {                                                           \
                    s0 += worktype(a_data[k])*b_data[k];                    \
                    s1 += worktype(a_data[k+1])*b_data[k+1];                \
                    s2 += worktype(a_data[k+2])*b_data[k+2];                \
                    s3 += worktype(a_data[k+3])*b_data[k+3];                \
                }                                                           \
                                                                            \
                for( ; k < n; k++ )                                         \
                    s0 += worktype(a_data[k])*b_data[k];                    \
                s0 = (s0+s1+s2+s3)*alpha;                                   \
                                                                            \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else if( d_size.width*sizeof(d_data[0]) <= 1600 )                       \
    {                                                                       \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                    _c_data += c_step0,                     \
                                    d_data += d_step )                      \
        {                                                                   \
            a_data = _a_data, c_data = _c_data;                             \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = a_data[a_step1*k];                           \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j <= m - 4; j += 4, c_data += 4*c_step1 )           \
            {                                                               \
                const arrtype* b = _b_data + j;                             \
                worktype s0(0), s1(0), s2(0), s3(0);                        \
                                                                            \
                for( k = 0; k < n; k++, b += b_step )                       \
                {                                                           \
                    worktype a(a_data[k]);                                  \
                    s0 += a * b[0]; s1 += a * b[1];                         \
                    s2 += a * b[2]; s3 += a * b[3];                         \
                }                                                           \
                                                                            \
                if( !c_data )                                               \
                {                                                           \
                    d_data[j] = arrtype(s0*alpha);                          \
                    d_data[j+1] = arrtype(s1*alpha);                        \
                    d_data[j+2] = arrtype(s2*alpha);                        \
                    d_data[j+3] = arrtype(s3*alpha);                        \
                }                                                           \
                else                                                        \
                {                                                           \
                    s0 = s0*alpha; s1 = s1*alpha;                           \
                    s2 = s2*alpha; s3 = s3*alpha;                           \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
                    d_data[j+1] = arrtype(s1 + c_data[c_step1]*beta);       \
                    d_data[j+2] = arrtype(s2 + c_data[c_step1*2]*beta);     \
                    d_data[j+3] = arrtype(s3 + c_data[c_step1*3]*beta);     \
                }                                                           \
            }                                                               \
                                                                            \
            for( ; j < m; j++, c_data += c_step1 )                          \
            {                                                               \
                const arrtype* b = _b_data + j;                             \
                worktype s0(0);                                             \
                                                                            \
                for( k = 0; k < n; k++, b += b_step )                       \
                    s0 += worktype(a_data[k]) * b[0];                       \
                                                                            \
                s0 = s0*alpha;                                              \
                if( !c_data )                                               \
                    d_data[j] = arrtype(s0);                                \
                else                                                        \
                    d_data[j] = arrtype(s0 + c_data[0]*beta);               \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        worktype* d_buf = (worktype*)cvStackAlloc(m*sizeof(d_buf[0]));      \
                                                                            \
        for( i = 0; i < drows; i++, _a_data += a_step0,                     \
                                            _c_data += c_step0,             \
                                            d_data += d_step )              \
        {                                                                   \
            a_data = _a_data;                                               \
            b_data = _b_data;                                               \
            c_data = _c_data;                                               \
                                                                            \
            if( a_buf )                                                     \
            {                                                               \
                for( k = 0; k < n; k++ )                                    \
                    a_buf[k] = _a_data[a_step1*k];                          \
                a_data = a_buf;                                             \
            }                                                               \
                                                                            \
            for( j = 0; j < m; j++ )                                        \
                d_buf[j] = worktype(0);                                     \
                                                                            \
            for( k = 0; k < n; k++, b_data += b_step )                      \
            {                                                               \
                worktype al(a_data[k]);                                     \
                                                                            \
                for( j = 0; j <= m - 4; j += 4 )                            \
                {                                                           \
                    worktype t0 = d_buf[j] + b_data[j]*al;                  \
                    worktype t1 = d_buf[j+1] + b_data[j+1]*al;              \
                    d_buf[j] = t0;                                          \
                    d_buf[j+1] = t1;                                        \
                    t0 = d_buf[j+2] + b_data[j+2]*al;                       \
                    t1 = d_buf[j+3] + b_data[j+3]*al;                       \
                    d_buf[j+2] = t0;                                        \
                    d_buf[j+3] = t1;                                        \
                }                                                           \
                                                                            \
                for( ; j < m; j++ )                                         \
                    d_buf[j] += b_data[j]*al;                               \
            }                                                               \
                                                                            \
            if( !c_data )                                                   \
                for( j = 0; j < m; j++ )                                    \
                    d_data[j] = arrtype(d_buf[j]*alpha);                    \
            else                                                            \
                for( j = 0; j < m; j++, c_data += c_step1 )                 \
                {                                                           \
                    worktype t = d_buf[j]*alpha;                            \
                    d_data[j] = arrtype(t + c_data[0]*beta);                \
                }                                                           \
        }                                                                   \
    }                                                                       \
    return CV_OK;                                                           \
}
// 
// 
#define ICV_DEF_GEMM_BLOCK_MUL( flavor, arrtype, worktype )         \
static CvStatus CV_STDCALL                                          \
icvGEMMBlockMul_##flavor( const arrtype* a_data, size_t a_step,     \
                        const arrtype* b_data, size_t b_step,       \
                        worktype* d_data, size_t d_step,            \
                        CvSize a_size, CvSize d_size, int flags )   \
{                                                                   \
    int i, j, k, n = a_size.width, m = d_size.width;                \
    const arrtype *_a_data = a_data, *_b_data = b_data;             \
    arrtype* a_buf = 0;                                             \
    size_t a_step0, a_step1, t_step;                                \
    int do_acc = flags & 16;                                        \
                                                                    \
    a_step /= sizeof(a_data[0]);                                    \
    b_step /= sizeof(b_data[0]);                                    \
    d_step /= sizeof(d_data[0]);                                    \
                                                                    \
    a_step0 = a_step;                                               \
    a_step1 = 1;                                                    \
                                                                    \
    if( flags & CV_GEMM_A_T )                                       \
    {                                                               \
        CV_SWAP( a_step0, a_step1, t_step );                        \
        n = a_size.height;                                          \
        a_buf = (arrtype*)cvStackAlloc(n*sizeof(a_data[0]));        \
    }                                                               \
                                                                    \
    if( flags & CV_GEMM_B_T )                                       \
    {                                                               \
        /* second operand is transposed */                          \
        for( i = 0; i < d_size.height; i++, _a_data += a_step0,     \
                                            d_data += d_step )      \
        {                                                           \
            a_data = _a_data; b_data = _b_data;                     \
                                                                    \
            if( a_buf )                                             \
            {                                                       \
                for( k = 0; k < n; k++ )                            \
                    a_buf[k] = a_data[a_step1*k];                   \
                a_data = a_buf;                                     \
            }                                                       \
                                                                    \
            for( j = 0; j < d_size.width; j++, b_data += b_step )   \
            {                                                       \
                worktype s0 = do_acc ? d_data[j]:worktype(0), s1(0);\
                for( k = 0; k <= n - 2; k += 2 )                    \
                {                                                   \
                    s0 += worktype(a_data[k])*b_data[k];            \
                    s1 += worktype(a_data[k+1])*b_data[k+1];        \
                }                                                   \
                                                                    \
                for( ; k < n; k++ )                                 \
                    s0 += worktype(a_data[k])*b_data[k];            \
                                                                    \
                d_data[j] = s0 + s1;                                \
            }                                                       \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        for( i = 0; i < d_size.height; i++, _a_data += a_step0,     \
                                            d_data += d_step )      \
        {                                                           \
            a_data = _a_data, b_data = _b_data;                     \
                                                                    \
            if( a_buf )                                             \
            {                                                       \
                for( k = 0; k < n; k++ )                            \
                    a_buf[k] = a_data[a_step1*k];                   \
                a_data = a_buf;                                     \
            }                                                       \
                                                                    \
            for( j = 0; j <= m - 4; j += 4 )                        \
            {                                                       \
                worktype s0, s1, s2, s3;                            \
                const arrtype* b = b_data + j;                      \
                                                                    \
                if( do_acc )                                        \
                {                                                   \
                    s0 = d_data[j]; s1 = d_data[j+1];               \
                    s2 = d_data[j+2]; s3 = d_data[j+3];             \
                }                                                   \
                else                                                \
                    s0 = s1 = s2 = s3 = worktype(0);                \
                                                                    \
                for( k = 0; k < n; k++, b += b_step )               \
                {                                                   \
                    worktype a(a_data[k]);                          \
                    s0 += a * b[0]; s1 += a * b[1];                 \
                    s2 += a * b[2]; s3 += a * b[3];                 \
                }                                                   \
                                                                    \
                d_data[j] = s0; d_data[j+1] = s1;                   \
                d_data[j+2] = s2; d_data[j+3] = s3;                 \
            }                                                       \
                                                                    \
            for( ; j < m; j++ )                                     \
            {                                                       \
                const arrtype* b = b_data + j;                      \
                worktype s0 = do_acc ? d_data[j] : worktype(0);     \
                                                                    \
                for( k = 0; k < n; k++, b += b_step )               \
                    s0 += worktype(a_data[k]) * b[0];               \
                                                                    \
                d_data[j] = s0;                                     \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}
// 
// 
#define ICV_DEF_GEMM_STORE( flavor, arrtype, worktype )             \
static CvStatus CV_STDCALL                                          \
icvGEMMStore_##flavor( const arrtype* c_data, size_t c_step,        \
                       const worktype* d_buf, size_t d_buf_step,    \
                       arrtype* d_data, size_t d_step, CvSize d_size,\
                       double alpha, double beta, int flags )       \
{                                                                   \
    const arrtype* _c_data = c_data;                                \
    int j;                                                          \
    size_t c_step0, c_step1;                                        \
                                                                    \
    c_step /= sizeof(c_data[0]);                                    \
    d_buf_step /= sizeof(d_buf[0]);                                 \
    d_step /= sizeof(d_data[0]);                                    \
                                                                    \
    if( !c_data )                                                   \
        c_step0 = c_step1 = 0;                                      \
    else if( !(flags & CV_GEMM_C_T) )                               \
        c_step0 = c_step, c_step1 = 1;                              \
    else                                                            \
        c_step0 = 1, c_step1 = c_step;                              \
                                                                    \
    for( ; d_size.height--; _c_data += c_step0,                     \
                            d_buf += d_buf_step,                    \
                            d_data += d_step )                      \
    {                                                               \
        if( _c_data )                                               \
        {                                                           \
            c_data = _c_data;                                       \
            for( j = 0; j <= d_size.width - 4; j += 4, c_data += 4*c_step1 )\
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                worktype t1 = alpha*d_buf[j+1];                     \
                t0 += beta*worktype(c_data[0]);                     \
                t1 += beta*worktype(c_data[c_step1]);               \
                d_data[j] = arrtype(t0);                            \
                d_data[j+1] = arrtype(t1);                          \
                t0 = alpha*d_buf[j+2];                              \
                t1 = alpha*d_buf[j+3];                              \
                t0 += beta*worktype(c_data[c_step1*2]);             \
                t1 += beta*worktype(c_data[c_step1*3]);             \
                d_data[j+2] = arrtype(t0);                          \
                d_data[j+3] = arrtype(t1);                          \
            }                                                       \
            for( ; j < d_size.width; j++, c_data += c_step1 )       \
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                d_data[j] = arrtype(t0 + beta*c_data[0]);           \
            }                                                       \
        }                                                           \
        else                                                        \
        {                                                           \
            for( j = 0; j <= d_size.width - 4; j += 4 )             \
            {                                                       \
                worktype t0 = alpha*d_buf[j];                       \
                worktype t1 = alpha*d_buf[j+1];                     \
                d_data[j] = arrtype(t0);                            \
                d_data[j+1] = arrtype(t1);                          \
                t0 = alpha*d_buf[j+2];                              \
                t1 = alpha*d_buf[j+3];                              \
                d_data[j+2] = arrtype(t0);                          \
                d_data[j+3] = arrtype(t1);                          \
            }                                                       \
            for( ; j < d_size.width; j++ )                          \
                d_data[j] = arrtype(alpha*d_buf[j]);                \
        }                                                           \
    }                                                               \
    return CV_OK;                                                   \
}


ICV_DEF_GEMM_SINGLE_MUL( 32f_C1R, float, double)
ICV_DEF_GEMM_BLOCK_MUL( 32f_C1R, float, double)
ICV_DEF_GEMM_STORE( 32f_C1R, float, double)

ICV_DEF_GEMM_SINGLE_MUL( 64f_C1R, double, double)
ICV_DEF_GEMM_BLOCK_MUL( 64f_C1R, double, double)
ICV_DEF_GEMM_STORE( 64f_C1R, double, double)

ICV_DEF_GEMM_SINGLE_MUL( 32f_C2R, CvComplex32f, CvComplex64f)
ICV_DEF_GEMM_BLOCK_MUL( 32f_C2R, CvComplex32f, CvComplex64f)
ICV_DEF_GEMM_STORE( 32f_C2R, CvComplex32f, CvComplex64f)

ICV_DEF_GEMM_SINGLE_MUL( 64f_C2R, CvComplex64f, CvComplex64f)
ICV_DEF_GEMM_BLOCK_MUL( 64f_C2R, CvComplex64f, CvComplex64f)
ICV_DEF_GEMM_STORE( 64f_C2R, CvComplex64f, CvComplex64f)
// 
typedef CvStatus (CV_STDCALL *CvGEMMSingleMulFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, const void* src3, size_t step3,
                   void* dst, size_t dststep, CvSize srcsize, CvSize dstsize,
                   double alpha, double beta, int flags );
// 
typedef CvStatus (CV_STDCALL *CvGEMMBlockMulFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, void* dst, size_t dststep,
                   CvSize srcsize, CvSize dstsize, int flags );
// 
typedef CvStatus (CV_STDCALL *CvGEMMStoreFunc)( const void* src1, size_t step1,
                   const void* src2, size_t step2, void* dst, size_t dststep,
                   CvSize dstsize, double alpha, double beta, int flags );

// 
static void icvInitGEMMTable( CvBigFuncTable* single_mul_tab,
                              CvBigFuncTable* block_mul_tab,
                              CvBigFuncTable* store_tab )
{
    single_mul_tab->fn_2d[CV_32FC1] = (void*)icvGEMMSingleMul_32f_C1R;
    single_mul_tab->fn_2d[CV_64FC1] = (void*)icvGEMMSingleMul_64f_C1R;
    single_mul_tab->fn_2d[CV_32FC2] = (void*)icvGEMMSingleMul_32f_C2R;
    single_mul_tab->fn_2d[CV_64FC2] = (void*)icvGEMMSingleMul_64f_C2R;
    block_mul_tab->fn_2d[CV_32FC1] = (void*)icvGEMMBlockMul_32f_C1R;
    block_mul_tab->fn_2d[CV_64FC1] = (void*)icvGEMMBlockMul_64f_C1R;
    block_mul_tab->fn_2d[CV_32FC2] = (void*)icvGEMMBlockMul_32f_C2R;
    block_mul_tab->fn_2d[CV_64FC2] = (void*)icvGEMMBlockMul_64f_C2R;
    store_tab->fn_2d[CV_32FC1] = (void*)icvGEMMStore_32f_C1R;
    store_tab->fn_2d[CV_64FC1] = (void*)icvGEMMStore_64f_C1R;
    store_tab->fn_2d[CV_32FC2] = (void*)icvGEMMStore_32f_C2R;
    store_tab->fn_2d[CV_64FC2] = (void*)icvGEMMStore_64f_C2R;
}
// 
// 
CV_IMPL void
cvGEMM( const CvArr* Aarr, const CvArr* Barr, double alpha,
        const CvArr* Carr, double beta, CvArr* Darr, int flags )
{
    const int block_lin_size = 128;
    const int block_size = block_lin_size * block_lin_size;

    static CvBigFuncTable single_mul_tab, block_mul_tab, store_tab;
    static int inittab = 0;
    static double zero[] = {0,0,0,0};
    static float zerof[] = {0,0,0,0};
    
    uchar* buffer = 0;
    int local_alloc = 0;
    uchar* block_buffer = 0;

    CV_FUNCNAME( "cvGEMM" );

    __BEGIN__;

    CvMat *A = (CvMat*)Aarr;
    CvMat *B = (CvMat*)Barr;
    CvMat *C = (CvMat*)Carr;
    CvMat *D = (CvMat*)Darr;
    int len = 0;
    
    CvMat stub, stub1, stub2, stub3;
    CvSize a_size, d_size;
    int type;

    if( !CV_IS_MAT( A ))
    {
        int coi = 0;
        CV_CALL( A = cvGetMat( A, &stub1, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( B ))
    {
        int coi = 0;
        CV_CALL( B = cvGetMat( B, &stub2, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( D ))
    {
        int coi = 0;
        CV_CALL( D = cvGetMat( D, &stub, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( beta == 0 )
        C = 0;

    if( C )
    {
        if( !CV_IS_MAT( C ))
        {
            int coi = 0;
            CV_CALL( C = cvGetMat( C, &stub3, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }

        if( !CV_ARE_TYPES_EQ( C, D ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( (flags&CV_GEMM_C_T) == 0 && (C->cols != D->cols || C->rows != D->rows) ||
            (flags&CV_GEMM_C_T) != 0 && (C->rows != D->cols || C->cols != D->rows))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        if( (flags & CV_GEMM_C_T) != 0 && C->data.ptr == D->data.ptr )
        {
            cvTranspose( C, D );
            C = D;
            flags &= ~CV_GEMM_C_T;
        }
    }
    else
    {
        C = &stub3;
        C->data.ptr = 0;
        C->step = 0;
        C->type = CV_MAT_CONT_FLAG;
    }

    type = CV_MAT_TYPE(A->type);
    if( !CV_ARE_TYPES_EQ( A, B ) || !CV_ARE_TYPES_EQ( A, D ) )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    a_size.width = A->cols;
    a_size.height = A->rows;
    d_size.width = D->cols;
    d_size.height = D->rows;

    switch( flags & (CV_GEMM_A_T|CV_GEMM_B_T) )
    {
    case 0:
        len = B->rows;
        if( a_size.width != len ||
            B->cols != d_size.width ||
            a_size.height != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 1:
        len = B->rows;
        if( a_size.height != len ||
            B->cols != d_size.width ||
            a_size.width != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 2:
        len = B->cols;
        if( a_size.width != len ||
            B->rows != d_size.width ||
            a_size.height != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    case 3:
        len = B->cols;
        if( a_size.height != len ||
            B->rows != d_size.width ||
            a_size.width != d_size.height )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        break;
    }

    if( flags == 0 && 2 <= len && len <= 4 && (len == d_size.width || len == d_size.height) )
    {
        int i;
        if( type == CV_64F )
        {
            double* d = D->data.db;
            const double *a = A->data.db, *b = B->data.db, *c = C->data.db;
            size_t d_step = D->step/sizeof(d[0]),
                   a_step = A->step/sizeof(a[0]),
                   b_step = B->step/sizeof(b[0]),
                   c_step = C->step/sizeof(c[0]);

            if( !c )
                c = zero;

            switch( len )
            {
            case 2:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step];
                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                    }
                }
                else
                    break;
                EXIT;
            case 3:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1];
                        double t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                        d[2] = t2*alpha + c[2]*beta;
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] + a[a_step+2]*b[b_step*2];
                        double t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] + a[a_step*2+2]*b[b_step*2];

                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                        d[d_step*2] = t2*alpha + c[c_step*2]*beta;
                    }
                }
                else
                    break;
                EXIT;
            case 4:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        double t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1] + a[3]*b[b_step*3+1];
                        double t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2] + a[3]*b[b_step*3+2];
                        double t3 = a[0]*b[3] + a[1]*b[b_step+3] + a[2]*b[b_step*2+3] + a[3]*b[b_step*3+3];
                        d[0] = t0*alpha + c[0]*beta;
                        d[1] = t1*alpha + c[1]*beta;
                        d[2] = t2*alpha + c[2]*beta;
                        d[3] = t3*alpha + c[3]*beta;
                    }
                }
                else if( d_size.width <= 16 && a != d )
                {
                    int c_step0 = 1;
                    if( c == zero )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        double t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        double t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] +
                                    a[a_step+2]*b[b_step*2] + a[a_step+3]*b[b_step*3];
                        double t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] +
                                    a[a_step*2+2]*b[b_step*2] + a[a_step*2+3]*b[b_step*3];
                        double t3 = a[a_step*3]*b[0] + a[a_step*3+1]*b[b_step] +
                                    a[a_step*3+2]*b[b_step*2] + a[a_step*3+3]*b[b_step*3];
                        d[0] = t0*alpha + c[0]*beta;
                        d[d_step] = t1*alpha + c[c_step]*beta;
                        d[d_step*2] = t2*alpha + c[c_step*2]*beta;
                        d[d_step*3] = t3*alpha + c[c_step*3]*beta;
                    }
                }
                else
                    break;
                EXIT;
            }
        }

        if( type == CV_32F )
        {
            float* d = D->data.fl;
            const float *a = A->data.fl, *b = B->data.fl, *c = C->data.fl;
            size_t d_step = D->step/sizeof(d[0]),
                   a_step = A->step/sizeof(a[0]),
                   b_step = B->step/sizeof(b[0]),
                   c_step = C->step/sizeof(c[0]);

            if( !c )
                c = zerof;

            switch( len )
            {
            case 2:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                    }
                }
                else
                    break;
                EXIT;
            case 3:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1];
                        float t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                        d[2] = (float)(t2*alpha + c[2]*beta);
                    }
                }
                else if( a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] + a[a_step+2]*b[b_step*2];
                        float t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] + a[a_step*2+2]*b[b_step*2];

                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                        d[d_step*2] = (float)(t2*alpha + c[c_step*2]*beta);
                    }
                }
                else
                    break;
                EXIT;
            case 4:
                if( len == d_size.width && b != d )
                {
                    for( i = 0; i < d_size.height; i++, d += d_step, a += a_step, c += c_step )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        float t1 = a[0]*b[1] + a[1]*b[b_step+1] + a[2]*b[b_step*2+1] + a[3]*b[b_step*3+1];
                        float t2 = a[0]*b[2] + a[1]*b[b_step+2] + a[2]*b[b_step*2+2] + a[3]*b[b_step*3+2];
                        float t3 = a[0]*b[3] + a[1]*b[b_step+3] + a[2]*b[b_step*2+3] + a[3]*b[b_step*3+3];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[1] = (float)(t1*alpha + c[1]*beta);
                        d[2] = (float)(t2*alpha + c[2]*beta);
                        d[3] = (float)(t3*alpha + c[3]*beta);
                    }
                }
                else if( len <= 16 && a != d )
                {
                    int c_step0 = 1;
                    if( c == zerof )
                    {
                        c_step0 = 0;
                        c_step = 1;
                    }

                    for( i = 0; i < d_size.width; i++, d++, b++, c += c_step0 )
                    {
                        float t0 = a[0]*b[0] + a[1]*b[b_step] + a[2]*b[b_step*2] + a[3]*b[b_step*3];
                        float t1 = a[a_step]*b[0] + a[a_step+1]*b[b_step] +
                                   a[a_step+2]*b[b_step*2] + a[a_step+3]*b[b_step*3];
                        float t2 = a[a_step*2]*b[0] + a[a_step*2+1]*b[b_step] +
                                   a[a_step*2+2]*b[b_step*2] + a[a_step*2+3]*b[b_step*3];
                        float t3 = a[a_step*3]*b[0] + a[a_step*3+1]*b[b_step] +
                                   a[a_step*3+2]*b[b_step*2] + a[a_step*3+3]*b[b_step*3];
                        d[0] = (float)(t0*alpha + c[0]*beta);
                        d[d_step] = (float)(t1*alpha + c[c_step]*beta);
                        d[d_step*2] = (float)(t2*alpha + c[c_step*2]*beta);
                        d[d_step*3] = (float)(t3*alpha + c[c_step*3]*beta);
                    }
                }
                else
                    break;
                EXIT;
            }
        }
    }
 
    {
        int b_step = B->step;
        CvGEMMSingleMulFunc single_mul_func;
        CvMat tmat, *D0 = D;
        icvBLAS_GEMM_32f_t blas_func = 0;

        if( !inittab )
        {
            icvInitGEMMTable( &single_mul_tab, &block_mul_tab, &store_tab );
            inittab = 1;
        }

        single_mul_func = (CvGEMMSingleMulFunc)single_mul_tab.fn_2d[type];
        if( !single_mul_func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( D->data.ptr == A->data.ptr || D->data.ptr == B->data.ptr )
        {
            int buf_size = d_size.width*d_size.height*CV_ELEM_SIZE(type);
            if( d_size.width <= CV_MAX_LOCAL_MAT_SIZE )
            {
                buffer = (uchar*)cvStackAlloc( buf_size );
                local_alloc = 1;
            }
            else
                CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));

            tmat = cvMat( d_size.height, d_size.width, type, buffer );
            D = &tmat;
        }

        if( (d_size.width == 1 || len == 1) && !(flags & CV_GEMM_B_T) && CV_IS_MAT_CONT(B->type) )
        {
            b_step = d_size.width == 1 ? 0 : CV_ELEM_SIZE(type);
            flags |= CV_GEMM_B_T;
        }

        if( (d_size.width | d_size.height | len) >= 16 && icvBLAS_GEMM_32f_p != 0 )
        {
            blas_func = type == CV_32FC1 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_32f_p :
                        type == CV_64FC1 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_64f_p :
                        type == CV_32FC2 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_32fc_p :
                        type == CV_64FC2 ? (icvBLAS_GEMM_32f_t)icvBLAS_GEMM_64fc_p : 0;
        }

        if( blas_func )
        {
            const char* transa = flags & CV_GEMM_A_T ? "t" : "n";
            const char* transb = flags & CV_GEMM_B_T ? "t" : "n";
            int lda, ldb, ldd;
            
            if( C->data.ptr )
            {
                if( C->data.ptr != D->data.ptr )
                {
                    if( !(flags & CV_GEMM_C_T) )
                        cvCopy( C, D );
                    else
                        cvTranspose( C, D );
                }
            }

            if( CV_MAT_DEPTH(type) == CV_32F )
            {
                CvComplex32f _alpha, _beta;
                
                lda = A->step/sizeof(float);
                ldb = b_step/sizeof(float);
                ldd = D->step/sizeof(float);
                _alpha.re = (float)alpha;
                _alpha.im = 0;
                _beta.re = C->data.ptr ? (float)beta : 0;
                _beta.im = 0;
                if( CV_MAT_CN(type) == 2 )
                    lda /= 2, ldb /= 2, ldd /= 2;

                blas_func( transb, transa, &d_size.width, &d_size.height, &len,
                       &_alpha, B->data.ptr, &ldb, A->data.ptr, &lda,
                       &_beta, D->data.ptr, &ldd );
            }
            else
            {
                CvComplex64f _alpha, _beta;
                
                lda = A->step/sizeof(double);
                ldb = b_step/sizeof(double);
                ldd = D->step/sizeof(double);
                _alpha.re = alpha;
                _alpha.im = 0;
                _beta.re = C->data.ptr ? beta : 0;
                _beta.im = 0;
                if( CV_MAT_CN(type) == 2 )
                    lda /= 2, ldb /= 2, ldd /= 2;

                blas_func( transb, transa, &d_size.width, &d_size.height, &len,
                       &_alpha, B->data.ptr, &ldb, A->data.ptr, &lda,
                       &_beta, D->data.ptr, &ldd );
            }
        }
        else if( d_size.height <= block_lin_size/2 || d_size.width <= block_lin_size/2 || len <= 10 ||
            d_size.width <= block_lin_size && d_size.height <= block_lin_size && len <= block_lin_size )
        {
            single_mul_func( A->data.ptr, A->step, B->data.ptr, b_step,
                             C->data.ptr, C->step, D->data.ptr, D->step,
                             a_size, d_size, alpha, beta, flags );
        }
        else
        {
            int is_a_t = flags & CV_GEMM_A_T;
            int is_b_t = flags & CV_GEMM_B_T;
            int elem_size = CV_ELEM_SIZE(type);
            int dk0_1, dk0_2;
            int a_buf_size = 0, b_buf_size, d_buf_size;
            uchar* a_buf = 0;
            uchar* b_buf = 0;
            uchar* d_buf = 0;
            int i, j, k, di = 0, dj = 0, dk = 0;
            int dm0, dn0, dk0;
            int a_step0, a_step1, b_step0, b_step1, c_step0, c_step1;
            int work_elem_size = elem_size << (CV_MAT_DEPTH(type) == CV_32F ? 1 : 0);
            CvGEMMBlockMulFunc block_mul_func = (CvGEMMBlockMulFunc)block_mul_tab.fn_2d[type];
            CvGEMMStoreFunc store_func = (CvGEMMStoreFunc)store_tab.fn_2d[type];

            assert( block_mul_func && store_func );

            if( !is_a_t )
                a_step0 = A->step, a_step1 = elem_size;
            else
                a_step0 = elem_size, a_step1 = A->step;

            if( !is_b_t )
                b_step0 = b_step, b_step1 = elem_size;
            else
                b_step0 = elem_size, b_step1 = b_step;

            if( !C->data.ptr )
            {
                c_step0 = c_step1 = 0;
                flags &= ~CV_GEMM_C_T;
            }
            else if( !(flags & CV_GEMM_C_T) )
                c_step0 = C->step, c_step1 = elem_size;
            else
                c_step0 = elem_size, c_step1 = C->step;

            dm0 = MIN( block_lin_size, d_size.height );
            dn0 = MIN( block_lin_size, d_size.width );
            dk0_1 = block_size / dm0;
            dk0_2 = block_size / dn0;
            dk0 = MAX( dk0_1, dk0_2 );
            dk0 = MIN( dk0, len );
            if( dk0*dm0 > block_size )
                dm0 = block_size / dk0;
            if( dk0*dn0 > block_size )
                dn0 = block_size / dk0;

            dk0_1 = (dn0+dn0/8+2) & -2;
            b_buf_size = (dk0+dk0/8+1)*dk0_1*elem_size;
            d_buf_size = (dk0+dk0/8+1)*dk0_1*work_elem_size;
        
            if( is_a_t )
            {
                a_buf_size = (dm0+dm0/8+1)*((dk0+dk0/8+2)&-2)*elem_size;
                flags &= ~CV_GEMM_A_T;
            }

            CV_CALL( block_buffer = (uchar*)cvAlloc(a_buf_size + b_buf_size + d_buf_size));
            d_buf = block_buffer;
            b_buf = d_buf + d_buf_size;

            if( is_a_t )
                a_buf = b_buf + b_buf_size;

            for( i = 0; i < d_size.height; i += di )
            {
                di = dm0;
                if( i + di >= d_size.height || 8*(i + di) + di > 8*d_size.height )
                    di = d_size.height - i;

                for( j = 0; j < d_size.width; j += dj )
                {
                    uchar* _d = D->data.ptr + i*D->step + j*elem_size;
                    const uchar* _c = C->data.ptr + i*c_step0 + j*c_step1;
                    int _d_step = D->step;
                    dj = dn0;

                    if( j + dj >= d_size.width || 8*(j + dj) + dj > 8*d_size.width )
                        dj = d_size.width - j;

                    flags &= 15;
                    if( dk0 < len )
                    {
                        _d = d_buf;
                        _d_step = dj*work_elem_size;
                    }

                    for( k = 0; k < len; k += dk )
                    {
                        const uchar* _a = A->data.ptr + i*a_step0 + k*a_step1;
                        int _a_step = A->step;
                        const uchar* _b = B->data.ptr + k*b_step0 + j*b_step1;
                        int _b_step = b_step;
                        CvSize a_bl_size;

                        dk = dk0;
                        if( k + dk >= len || 8*(k + dk) + dk > 8*len )
                            dk = len - k;

                        if( !is_a_t )
                            a_bl_size.width = dk, a_bl_size.height = di;
                        else
                            a_bl_size.width = di, a_bl_size.height = dk;

                        if( a_buf && is_a_t )
                        {
                            int t;
                            _a_step = dk*elem_size;
                            icvGEMM_TransposeBlock( _a, A->step, a_buf, _a_step, a_bl_size, elem_size );
                            CV_SWAP( a_bl_size.width, a_bl_size.height, t );
                            _a = a_buf;
                        }
                
                        if( dj < d_size.width )
                        {
                            CvSize b_size;
                            if( !is_b_t )
                                b_size.width = dj, b_size.height = dk;
                            else
                                b_size.width = dk, b_size.height = dj;

                            _b_step = b_size.width*elem_size;
                            icvGEMM_CopyBlock( _b, b_step, b_buf, _b_step, b_size, elem_size );
                            _b = b_buf;
                        }

                        if( dk0 < len )
                            block_mul_func( _a, _a_step, _b, _b_step, _d, _d_step,
                                            a_bl_size, cvSize(dj,di), flags );
                        else
                            single_mul_func( _a, _a_step, _b, _b_step, _c, C->step, _d, _d_step,
                                             a_bl_size, cvSize(dj,di), alpha, beta, flags );
                        flags |= 16;
                    }

                    if( dk0 < len )
                        store_func( _c, C->step, _d, _d_step, D->data.ptr + i*D->step + j*elem_size,
                                    D->step, cvSize(dj,di), alpha, beta, flags );
                }
            }
        }

        if( D0 != D )
            CV_CALL( cvCopy( D, D0 ));
    }

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
    if( block_buffer )
        cvFree( &block_buffer );
}

// /****************************************************************************************\
// *                                        cvMulTransposed                                 *
// \****************************************************************************************/
// 
#define ICV_DEF_MULTRANS_R_FUNC( flavor, srctype, dsttype, load_macro )         \
static CvStatus CV_STDCALL                                                      \
icvMulTransposedR_##flavor( const srctype* src, int srcstep,                    \
                       dsttype* dst, int dststep,                               \
                       const dsttype* delta, int deltastep,                     \
                       CvSize size, int delta_cols, double scale )              \
{                                                                               \
    int i, j, k;                                                                \
    dsttype* tdst = dst;                                                        \
    dsttype* col_buf = 0;                                                       \
    dsttype* delta_buf = 0;                                                     \
    int local_alloc = 0;                                                        \
    int buf_size = size.height*sizeof(dsttype);                                 \
                                                                                \
    if( delta && delta_cols < size.width )                                      \
    {                                                                           \
        assert( delta_cols == 1 );                                              \
        buf_size += 4*buf_size;                                                 \
    }                                                                           \
                                                                                \
    if( buf_size <= CV_MAX_LOCAL_SIZE )                                         \
    {                                                                           \
        col_buf = (dsttype*)cvStackAlloc( buf_size );                           \
        local_alloc = 1;                                                        \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        col_buf = (dsttype*)cvAlloc( buf_size );                                \
        if( !col_buf )                                                          \
            return CV_OUTOFMEM_ERR;                                             \
    }                                                                           \
                                                                                \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);                       \
    deltastep /= sizeof(delta[0]);                                              \
                                                                                \
    if( delta && delta_cols < size.width )                                      \
    {                                                                           \
        delta_buf = col_buf + size.height;                                      \
        for( i = 0; i < size.height; i++ )                                      \
            delta_buf[i*4] = delta_buf[i*4+1] =                                 \
                delta_buf[i*4+2] = delta_buf[i*4+3] = delta[i*deltastep];       \
        delta = delta_buf;                                                      \
        deltastep = deltastep ? 4 : 0;                                          \
    }                                                                           \
                                                                                \
    if( !delta )                                                                \
        for( i = 0; i < size.width; i++, tdst += dststep )                      \
        {                                                                       \
            for( k = 0; k < size.height; k++ )                                  \
                col_buf[k] = src[k*srcstep+i];                                  \
                                                                                \
            for( j = i; j <= size.width - 4; j += 4 )                           \
            {                                                                   \
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;                          \
                const srctype *tsrc = src + j;                                  \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc += srcstep )             \
                {                                                               \
                    double a = col_buf[k];                                      \
                    s0 += a * load_macro(tsrc[0]);                              \
                    s1 += a * load_macro(tsrc[1]);                              \
                    s2 += a * load_macro(tsrc[2]);                              \
                    s3 += a * load_macro(tsrc[3]);                              \
                }                                                               \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
                tdst[j+1] = (dsttype)(s1*scale);                                \
                tdst[j+2] = (dsttype)(s2*scale);                                \
                tdst[j+3] = (dsttype)(s3*scale);                                \
            }                                                                   \
                                                                                \
            for( ; j < size.width; j++ )                                        \
            {                                                                   \
                double s0 = 0;                                                  \
                const srctype *tsrc = src + j;                                  \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc += srcstep )             \
                    s0 += col_buf[k] * tsrc[0];                                 \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
            }                                                                   \
        }                                                                       \
    else                                                                        \
        for( i = 0; i < size.width; i++, tdst += dststep )                      \
        {                                                                       \
            if( !delta_buf )                                                    \
                for( k = 0; k < size.height; k++ )                              \
                    col_buf[k] = load_macro(src[k*srcstep+i]) - delta[k*deltastep+i]; \
            else                                                                \
                for( k = 0; k < size.height; k++ )                              \
                    col_buf[k] = load_macro(src[k*srcstep+i]) - delta_buf[k*deltastep]; \
                                                                                \
            for( j = i; j <= size.width - 4; j += 4 )                           \
            {                                                                   \
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;                          \
                const srctype *tsrc = src + j;                                  \
                const dsttype *d = delta_buf ? delta_buf : delta + j;           \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc+=srcstep, d+=deltastep ) \
                {                                                               \
                    double a = col_buf[k];                                      \
                    s0 += a * (load_macro(tsrc[0]) - d[0]);                     \
                    s1 += a * (load_macro(tsrc[1]) - d[1]);                     \
                    s2 += a * (load_macro(tsrc[2]) - d[2]);                     \
                    s3 += a * (load_macro(tsrc[3]) - d[3]);                     \
                }                                                               \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
                tdst[j+1] = (dsttype)(s1*scale);                                \
                tdst[j+2] = (dsttype)(s2*scale);                                \
                tdst[j+3] = (dsttype)(s3*scale);                                \
            }                                                                   \
                                                                                \
            for( ; j < size.width; j++ )                                        \
            {                                                                   \
                double s0 = 0;                                                  \
                const srctype *tsrc = src + j;                                  \
                const dsttype *d = delta_buf ? delta_buf : delta + j;           \
                                                                                \
                for( k = 0; k < size.height; k++, tsrc+=srcstep, d+=deltastep ) \
                    s0 += col_buf[k] * (load_macro(tsrc[0]) - d[0]);            \
                                                                                \
                tdst[j] = (dsttype)(s0*scale);                                  \
            }                                                                   \
        }                                                                       \
                                                                                \
    /* fill the lower part of the destination matrix */                         \
    for( i = 1; i < size.width; i++ )                                           \
        for( j = 0; j < i; j++ )                                                \
            dst[dststep*i + j] = dst[dststep*j + i];                            \
                                                                                \
    if( col_buf && !local_alloc )                                               \
        cvFree( &col_buf );                                                     \
                                                                                \
    return CV_NO_ERR;                                                           \
}


#define ICV_DEF_MULTRANS_L_FUNC( flavor, srctype, dsttype, load_macro )         \
static CvStatus CV_STDCALL                                                      \
icvMulTransposedL_##flavor( const srctype* src, int srcstep,                    \
                            dsttype* dst, int dststep,                          \
                            dsttype* delta, int deltastep,                      \
                            CvSize size, int delta_cols, double scale )         \
{                                                                               \
    int i, j, k;                                                                \
    dsttype* tdst = dst;                                                        \
                                                                                \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);                       \
    deltastep /= sizeof(delta[0]);                                              \
                                                                                \
    if( !delta )                                                                \
        for( i = 0; i < size.height; i++, tdst += dststep )                     \
            for( j = i; j < size.height; j++ )                                  \
            {                                                                   \
                double s = 0;                                                   \
                const srctype *tsrc1 = src + i*srcstep;                         \
                const srctype *tsrc2 = src + j*srcstep;                         \
                                                                                \
                for( k = 0; k <= size.width - 4; k += 4 )                       \
                    s += tsrc1[k]*tsrc2[k] + tsrc1[k+1]*tsrc2[k+1] +            \
                         tsrc1[k+2]*tsrc2[k+2] + tsrc1[k+3]*tsrc2[k+3];         \
                for( ; k < size.width; k++ )                                    \
                    s += tsrc1[k] * tsrc2[k];                                   \
                tdst[j] = (dsttype)(s*scale);                                   \
            }                                                                   \
    else                                                                        \
    {                                                                           \
        dsttype* row_buf = 0;                                                   \
        int local_alloc = 0;                                                    \
        int buf_size = size.width*sizeof(dsttype);                              \
        dsttype delta_buf[4];                                                   \
        int delta_shift = delta_cols == size.width ? 4 : 0;                     \
                                                                                \
        if( buf_size <= CV_MAX_LOCAL_SIZE )                                     \
        {                                                                       \
            row_buf = (dsttype*)cvStackAlloc( buf_size );                       \
            local_alloc = 1;                                                    \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            row_buf = (dsttype*)cvAlloc( buf_size );                            \
            if( !row_buf )                                                      \
                return CV_OUTOFMEM_ERR;                                         \
        }                                                                       \
                                                                                \
        for( i = 0; i < size.height; i++, tdst += dststep )                     \
        {                                                                       \
            const srctype *tsrc1 = src + i*srcstep;                             \
            const dsttype *tdelta1 = delta + i*deltastep;                       \
                                                                                \
            if( delta_cols < size.width )                                       \
                for( k = 0; k < size.width; k++ )                               \
                    row_buf[k] = tsrc1[k] - tdelta1[0];                         \
            else                                                                \
                for( k = 0; k < size.width; k++ )                               \
                    row_buf[k] = tsrc1[k] - tdelta1[k];                         \
                                                                                \
            for( j = i; j < size.height; j++ )                                  \
            {                                                                   \
                double s = 0;                                                   \
                const srctype *tsrc2 = src + j*srcstep;                         \
                const dsttype *tdelta2 = delta + j*deltastep;                   \
                if( delta_cols < size.width )                                   \
                {                                                               \
                    delta_buf[0] = delta_buf[1] =                               \
                        delta_buf[2] = delta_buf[3] = tdelta2[0];               \
                    tdelta2 = delta_buf;                                        \
                }                                                               \
                for( k = 0; k <= size.width-4; k += 4, tdelta2 += delta_shift ) \
                    s += row_buf[k]*(load_macro(tsrc2[k]) - tdelta2[0]) +       \
                         row_buf[k+1]*(load_macro(tsrc2[k+1]) - tdelta2[1]) +   \
                         row_buf[k+2]*(load_macro(tsrc2[k+2]) - tdelta2[2]) +   \
                         row_buf[k+3]*(load_macro(tsrc2[k+3]) - tdelta2[3]);    \
                for( ; k < size.width; k++, tdelta2++ )                         \
                    s += row_buf[k]*(load_macro(tsrc2[k]) - tdelta2[0]);        \
                tdst[j] = (dsttype)(s*scale);                                   \
            }                                                                   \
        }                                                                       \
                                                                                \
        if( row_buf && !local_alloc )                                           \
            cvFree( &row_buf );                                                 \
    }                                                                           \
                                                                                \
    /* fill the lower part of the destination matrix */                         \
    for( j = 0; j < size.height - 1; j++ )                                      \
        for( i = j; i < size.height; i++ )                                      \
            dst[dststep*i + j] = dst[dststep*j + i];                            \
                                                                                \
    return CV_NO_ERR;                                                           \
}


ICV_DEF_MULTRANS_R_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_MULTRANS_R_FUNC( 8u64f, uchar, double, CV_8TO32F )
ICV_DEF_MULTRANS_R_FUNC( 32f, float, float, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 32f64f, float, double, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 64f, double, double, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16u32f, ushort, float, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16u64f, ushort, double, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16s32f, short, float, CV_NOP )
ICV_DEF_MULTRANS_R_FUNC( 16s64f, short, double, CV_NOP )

ICV_DEF_MULTRANS_L_FUNC( 8u32f, uchar, float, CV_8TO32F )
ICV_DEF_MULTRANS_L_FUNC( 8u64f, uchar, double, CV_8TO32F )
ICV_DEF_MULTRANS_L_FUNC( 32f, float, float, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 32f64f, float, double, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 64f, double, double, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16u32f, ushort, float, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16u64f, ushort, double, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16s32f, short, float, CV_NOP )
ICV_DEF_MULTRANS_L_FUNC( 16s64f, short, double, CV_NOP )

// 
typedef CvStatus (CV_STDCALL * CvMulTransposedFunc)
    ( const void* src, int srcstep,
      void* dst, int dststep, const void* delta,
      int deltastep, CvSize size, int delta_cols, double scale );

CV_IMPL void
cvMulTransposed( const CvArr* srcarr, CvArr* dstarr,
                 int order, const CvArr* deltaarr, double scale )
{
    const int gemm_level = 100; // boundary above which GEMM is faster.
    CvMat* src2 = 0;

    CV_FUNCNAME( "cvMulTransposed" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvMat deltastub, *delta = (CvMat*)deltaarr;
    int stype, dtype;

    if( !CV_IS_MAT( src ))
        CV_CALL( src = cvGetMat( src, &sstub ));

    if( !CV_IS_MAT( dst ))
        CV_CALL( dst = cvGetMat( dst, &dstub ));

    if( delta )
    {
        if( !CV_IS_MAT( delta ))
            CV_CALL( delta = cvGetMat( delta, &deltastub ));

        if( !CV_ARE_TYPES_EQ( dst, delta ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( (delta->rows != src->rows && delta->rows != 1) ||
            (delta->cols != src->cols && delta->cols != 1) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }
    else
    {
        delta = &deltastub;
        delta->data.ptr = 0;
        delta->step = 0;
        delta->rows = delta->cols = 0;
    }

    stype = CV_MAT_TYPE( src->type );
    dtype = CV_MAT_TYPE( dst->type );

    if( dst->rows != dst->cols )
        CV_ERROR( CV_StsBadSize, "The destination matrix must be square" );

    if( (order != 0 && src->cols != dst->cols) ||
        (order == 0 && src->rows != dst->rows))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( src->data.ptr == dst->data.ptr || stype == dtype &&
        (dst->cols >= gemm_level && dst->rows >= gemm_level &&
         src->cols >= gemm_level && src->rows >= gemm_level))
    {
        if( deltaarr )
        {
            CV_CALL( src2 = cvCreateMat( src->rows, src->cols, src->type ));
            cvRepeat( delta, src2 );
            cvSub( src, src2, src2 );
            src = src2;
        }
        cvGEMM( src, src, scale, 0, 0, dst, order == 0 ? CV_GEMM_B_T : CV_GEMM_A_T ); 
    }
    else
    {
        CvMulTransposedFunc func =
            stype == CV_8U && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_8u32f :
                    (CvMulTransposedFunc)icvMulTransposedL_8u32f) :
            stype == CV_8U && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_8u64f :
                    (CvMulTransposedFunc)icvMulTransposedL_8u64f) :
            stype == CV_16U && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16u32f :
                    (CvMulTransposedFunc)icvMulTransposedL_16u32f) :
            stype == CV_16U && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16u64f :
                    (CvMulTransposedFunc)icvMulTransposedL_16u64f) :
            stype == CV_16S && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16s32f :
                    (CvMulTransposedFunc)icvMulTransposedL_16s32f) :
            stype == CV_16S && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_16s64f :
                    (CvMulTransposedFunc)icvMulTransposedL_16s64f) :
            stype == CV_32F && dtype == CV_32F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_32f :
                    (CvMulTransposedFunc)icvMulTransposedL_32f) :
            stype == CV_32F && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_32f64f :
                    (CvMulTransposedFunc)icvMulTransposedL_32f64f) :
            stype == CV_64F && dtype == CV_64F ?
            (order ? (CvMulTransposedFunc)icvMulTransposedR_64f :
                    (CvMulTransposedFunc)icvMulTransposedL_64f) : 0;

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src->step, dst->data.ptr, dst->step,
                         delta->data.ptr, delta->step, cvGetMatSize( src ),
                         delta->cols, scale ));
    }

    __END__;

    if( src2 )
        cvReleaseMat( &src2 );
}
// 
// /****************************************************************************************\
// *                             Find sum of pixels in the ROI                              *
// \****************************************************************************************/
// 
#define ICV_SUM_COI_CASE( __op__, len, cn )                 \
    for( ; x <= (len) - 4*(cn); x += 4*(cn) )               \
        s0 += __op__(src[x]) + __op__(src[x+(cn)]) +        \
              __op__(src[x+(cn)*2]) + __op__(src[x+(cn)*3]);\
                                                            \
    for( ; x < (len); x += (cn) )                           \
        s0 += __op__(src[x]);


#define ICV_SUM_CASE_C1( __op__, len )                      \
    ICV_SUM_COI_CASE( __op__, len, 1 )


#define ICV_SUM_CASE_C2( __op__, len )                      \
    for( ; x <= (len) - 8; x += 8 )                         \
    {                                                       \
        s0 += __op__(src[x]) + __op__(src[x+2]) +           \
              __op__(src[x+4]) + __op__(src[x+6]);          \
        s1 += __op__(src[x+1]) + __op__(src[x+3]) +         \
              __op__(src[x+5]) + __op__(src[x+7]);          \
    }                                                       \
                                                            \
    for( ; x < (len); x += 2 )                              \
    {                                                       \
        s0 += __op__(src[x]);                               \
        s1 += __op__(src[x+1]);                             \
    }



#define ICV_SUM_CASE_C3( __op__, len )                      \
    for( ; x <= (len) - 12; x += 12 )                       \
    {                                                       \
        s0 += __op__(src[x]) + __op__(src[x+3]) +           \
              __op__(src[x+6]) + __op__(src[x+9]);          \
        s1 += __op__(src[x+1]) + __op__(src[x+4]) +         \
              __op__(src[x+7]) + __op__(src[x+10]);         \
        s2 += __op__(src[x+2]) + __op__(src[x+5]) +         \
              __op__(src[x+8]) + __op__(src[x+11]);         \
    }                                                       \
                                                            \
    for( ; x < (len); x += 3 )                              \
    {                                                       \
        s0 += __op__(src[x]);                               \
        s1 += __op__(src[x+1]);                             \
        s2 += __op__(src[x+2]);                             \
    }


#define ICV_SUM_CASE_C4( __op__, len )                      \
    for( ; x <= (len) - 16; x += 16 )                       \
    {                                                       \
        s0 += __op__(src[x]) + __op__(src[x+4]) +           \
              __op__(src[x+8]) + __op__(src[x+12]);         \
        s1 += __op__(src[x+1]) + __op__(src[x+5]) +         \
              __op__(src[x+9]) + __op__(src[x+13]);         \
        s2 += __op__(src[x+2]) + __op__(src[x+6]) +         \
              __op__(src[x+10]) + __op__(src[x+14]);        \
        s3 += __op__(src[x+3]) + __op__(src[x+7]) +         \
              __op__(src[x+11]) + __op__(src[x+15]);        \
    }                                                       \
                                                            \
    for( ; x < (len); x += 4 )                              \
    {                                                       \
        s0 += __op__(src[x]);                               \
        s1 += __op__(src[x+1]);                             \
        s2 += __op__(src[x+2]);                             \
        s3 += __op__(src[x+3]);                             \
    }


////////////////////////////////////// entry macros //////////////////////////////////////

#define ICV_SUM_ENTRY_COMMON()          \
    step /= sizeof(src[0])

#define ICV_SUM_ENTRY_C1( sumtype )     \
    sumtype s0 = 0;                     \
    ICV_SUM_ENTRY_COMMON()

#define ICV_SUM_ENTRY_C2( sumtype )     \
    sumtype s0 = 0, s1 = 0;             \
    ICV_SUM_ENTRY_COMMON()

#define ICV_SUM_ENTRY_C3( sumtype )     \
    sumtype s0 = 0, s1 = 0, s2 = 0;     \
    ICV_SUM_ENTRY_COMMON()

#define ICV_SUM_ENTRY_C4( sumtype )         \
    sumtype s0 = 0, s1 = 0, s2 = 0, s3 = 0; \
    ICV_SUM_ENTRY_COMMON()


#define ICV_SUM_ENTRY_BLOCK_COMMON( block_size )    \
    int remaining = block_size;                     \
    ICV_SUM_ENTRY_COMMON()

#define ICV_SUM_ENTRY_BLOCK_C1( sumtype, worktype, block_size ) \
    sumtype sum0 = 0;                                           \
    worktype s0 = 0;                                            \
    ICV_SUM_ENTRY_BLOCK_COMMON( block_size )

#define ICV_SUM_ENTRY_BLOCK_C2( sumtype, worktype, block_size ) \
    sumtype sum0 = 0, sum1 = 0;                                 \
    worktype s0 = 0, s1 = 0;                                    \
    ICV_SUM_ENTRY_BLOCK_COMMON( block_size )

#define ICV_SUM_ENTRY_BLOCK_C3( sumtype, worktype, block_size ) \
    sumtype sum0 = 0, sum1 = 0, sum2 = 0;                       \
    worktype s0 = 0, s1 = 0, s2 = 0;                            \
    ICV_SUM_ENTRY_BLOCK_COMMON( block_size )

#define ICV_SUM_ENTRY_BLOCK_C4( sumtype, worktype, block_size ) \
    sumtype sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;             \
    worktype s0 = 0, s1 = 0, s2 = 0, s3 = 0;                    \
    ICV_SUM_ENTRY_BLOCK_COMMON( block_size )


/////////////////////////////////////// exit macros //////////////////////////////////////

#define ICV_SUM_EXIT_C1( tmp, sumtype )     \
    sum[0] = (sumtype)tmp##0

#define ICV_SUM_EXIT_C2( tmp, sumtype )     \
    sum[0] = (sumtype)tmp##0;               \
    sum[1] = (sumtype)tmp##1;

#define ICV_SUM_EXIT_C3( tmp, sumtype )     \
    sum[0] = (sumtype)tmp##0;               \
    sum[1] = (sumtype)tmp##1;               \
    sum[2] = (sumtype)tmp##2;

#define ICV_SUM_EXIT_C4( tmp, sumtype )     \
    sum[0] = (sumtype)tmp##0;               \
    sum[1] = (sumtype)tmp##1;               \
    sum[2] = (sumtype)tmp##2;               \
    sum[3] = (sumtype)tmp##3;

#define ICV_SUM_EXIT_BLOCK_C1( sumtype )    \
    sum0 += s0;                             \
    ICV_SUM_EXIT_C1( sum, sumtype )

#define ICV_SUM_EXIT_BLOCK_C2( sumtype )    \
    sum0 += s0; sum1 += s1;                 \
    ICV_SUM_EXIT_C2( sum, sumtype )

#define ICV_SUM_EXIT_BLOCK_C3( sumtype )    \
    sum0 += s0; sum1 += s1;                 \
    sum2 += s2;                             \
    ICV_SUM_EXIT_C3( sum, sumtype )

#define ICV_SUM_EXIT_BLOCK_C4( sumtype )    \
    sum0 += s0; sum1 += s1;                 \
    sum2 += s2; sum3 += s3;                 \
    ICV_SUM_EXIT_C4( sum, sumtype )

////////////////////////////////////// update macros /////////////////////////////////////

#define ICV_SUM_UPDATE_COMMON( block_size ) \
    remaining = block_size

#define ICV_SUM_UPDATE_C1( block_size )     \
    ICV_SUM_UPDATE_COMMON( block_size );    \
    sum0 += s0;                             \
    s0 = 0

#define ICV_SUM_UPDATE_C2( block_size )     \
    ICV_SUM_UPDATE_COMMON( block_size );    \
    sum0 += s0; sum1 += s1;                 \
    s0 = s1 = 0

#define ICV_SUM_UPDATE_C3( block_size )     \
    ICV_SUM_UPDATE_COMMON( block_size );    \
    sum0 += s0; sum1 += s1; sum2 += s2;     \
    s0 = s1 = s2 = 0

#define ICV_SUM_UPDATE_C4( block_size )     \
    ICV_SUM_UPDATE_COMMON( block_size );    \
    sum0 += s0; sum1 += s1;                 \
    sum2 += s2; sum3 += s3;                 \
    s0 = s1 = s2 = s3 = 0


#define ICV_DEF_SUM_NOHINT_BLOCK_FUNC_2D( name, flavor, cn,     \
    __op__, arrtype, sumtype_final, sumtype, worktype, block_size )\
IPCVAPI_IMPL(CvStatus, icv##name##_##flavor##_C##cn##R,(        \
    const arrtype* src, int step, CvSize size,                  \
    sumtype_final* sum ), (src, step, size, sum) )              \
{                                                               \
    ICV_SUM_ENTRY_BLOCK_C##cn(sumtype,worktype,(block_size)*(cn)); \
    size.width *= cn;                                           \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_SUM_CASE_C##cn( __op__, limit );                \
            if( remaining == 0 )                                \
            {                                                   \
                ICV_SUM_UPDATE_C##cn( (block_size)*(cn) );      \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    ICV_SUM_EXIT_BLOCK_C##cn( sumtype_final );                  \
    return CV_OK;                                               \
}


#define ICV_DEF_SUM_NOHINT_FUNC_2D( name, flavor, cn,           \
    __op__, arrtype, sumtype_final, sumtype, worktype, block_size )\
IPCVAPI_IMPL(CvStatus, icv##name##_##flavor##_C##cn##R,(        \
    const arrtype* src, int step, CvSize size,                  \
    sumtype_final* sum ), (src, step, size, sum) )              \
{                                                               \
    ICV_SUM_ENTRY_C##cn( sumtype );                             \
    size.width *= cn;                                           \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_SUM_CASE_C##cn( __op__, size.width );               \
    }                                                           \
                                                                \
    ICV_SUM_EXIT_C##cn( s, sumtype_final );                     \
    return CV_OK;                                               \
}


#define ICV_DEF_SUM_HINT_FUNC_2D( name, flavor, cn,             \
    __op__, arrtype, sumtype_final, sumtype, worktype, block_size )\
IPCVAPI_IMPL(CvStatus, icv##name##_##flavor##_C##cn##R,(        \
    const arrtype* src, int step, CvSize size,                  \
    sumtype_final* sum, CvHintAlgorithm /*hint*/ ),             \
    (src, step, size, sum, cvAlgHintAccurate) )                 \
{                                                               \
    ICV_SUM_ENTRY_C##cn( sumtype );                             \
    size.width *= cn;                                           \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_SUM_CASE_C##cn( __op__, size.width );               \
    }                                                           \
                                                                \
    ICV_SUM_EXIT_C##cn( s, sumtype_final );                     \
    return CV_OK;                                               \
}


#define ICV_DEF_SUM_NOHINT_BLOCK_FUNC_2D_COI( name, flavor,     \
    __op__, arrtype, sumtype_final, sumtype, worktype, block_size )\
static CvStatus CV_STDCALL icv##name##_##flavor##_CnCR(         \
    const arrtype* src, int step, CvSize size, int cn,          \
    int coi, sumtype_final* sum )                               \
{                                                               \
    ICV_SUM_ENTRY_BLOCK_C1(sumtype,worktype,(block_size)*(cn)); \
    size.width *= cn;                                           \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_SUM_COI_CASE( __op__, limit, cn );              \
            if( remaining == 0 )                                \
            {                                                   \
                ICV_SUM_UPDATE_C1( (block_size)*(cn) );         \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    ICV_SUM_EXIT_BLOCK_C1( sumtype_final );                     \
    return CV_OK;                                               \
}


#define ICV_DEF_SUM_NOHINT_FUNC_2D_COI( name, flavor,           \
    __op__, arrtype, sumtype_final, sumtype, worktype, block_size )\
static CvStatus CV_STDCALL icv##name##_##flavor##_CnCR(         \
    const arrtype* src, int step, CvSize size, int cn,          \
    int coi, sumtype_final* sum )                               \
{                                                               \
    ICV_SUM_ENTRY_C1( sumtype );                                \
    size.width *= cn;                                           \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_SUM_COI_CASE( __op__, size.width, cn );             \
    }                                                           \
                                                                \
    ICV_SUM_EXIT_C1( s, sumtype_final );                        \
    return CV_OK;                                               \
}
 

#define ICV_DEF_SUM_ALL( name, flavor, __op__, arrtype, sumtype_final, sumtype, \
                         worktype, hintp_type, nohint_type, block_size )        \
    ICV_DEF_SUM_##hintp_type##_FUNC_2D( name, flavor, 1, __op__, arrtype,       \
                         sumtype_final, sumtype, worktype, block_size )         \
    ICV_DEF_SUM_##hintp_type##_FUNC_2D( name, flavor, 2, __op__, arrtype,       \
                         sumtype_final, sumtype, worktype, block_size )         \
    ICV_DEF_SUM_##hintp_type##_FUNC_2D( name, flavor, 3, __op__, arrtype,       \
                         sumtype_final, sumtype, worktype, block_size )         \
    ICV_DEF_SUM_##hintp_type##_FUNC_2D( name, flavor, 4, __op__, arrtype,       \
                         sumtype_final, sumtype, worktype, block_size )         \
    ICV_DEF_SUM_##nohint_type##_FUNC_2D_COI( name, flavor, __op__, arrtype,     \
                         sumtype_final, sumtype, worktype, block_size )

ICV_DEF_SUM_ALL( Sum, 8u, CV_NOP, uchar, double, int64, unsigned,
                 NOHINT_BLOCK, NOHINT_BLOCK, 1 << 24 )
ICV_DEF_SUM_ALL( Sum, 16u, CV_NOP, ushort, double, int64, unsigned,
                 NOHINT_BLOCK, NOHINT_BLOCK, 1 << 16 )
ICV_DEF_SUM_ALL( Sum, 16s, CV_NOP, short, double, int64, int,
                 NOHINT_BLOCK, NOHINT_BLOCK, 1 << 16 )
ICV_DEF_SUM_ALL( Sum, 32s, CV_NOP, int, double, double, double, NOHINT, NOHINT, 0 )
ICV_DEF_SUM_ALL( Sum, 32f, CV_NOP, float, double, double, double, HINT, NOHINT, 0 )
ICV_DEF_SUM_ALL( Sum, 64f, CV_NOP, double, double, double, double, NOHINT, NOHINT, 0 )
// 
#define icvSum_8s_C1R   0
#define icvSum_8s_C2R   0
#define icvSum_8s_C3R   0
#define icvSum_8s_C4R   0
#define icvSum_8s_CnCR  0
// 
 CV_DEF_INIT_BIG_FUNC_TAB_2D( Sum, R )
 CV_DEF_INIT_FUNC_TAB_2D( Sum, CnCR )
// 
CV_IMPL CvScalar
cvSum( const CvArr* arr )
{
    static CvBigFuncTable sum_tab;
    static CvFuncTable sumcoi_tab;
    static int inittab = 0;

    CvScalar sum = {{0,0,0,0}};

    CV_FUNCNAME("cvSum");

    __BEGIN__;

    int type, coi = 0;
    int mat_step;
    CvSize size;
    CvMat stub, *mat = (CvMat*)arr;

    if( !inittab )
    {
        icvInitSumRTable( &sum_tab );
        icvInitSumCnCRTable( &sumcoi_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(mat) )
    {
        if( CV_IS_MATND(mat) )
        {
            void* matnd = (void*)mat;
            CvMatND nstub;
            CvNArrayIterator iterator;
            int pass_hint;

            CV_CALL( cvInitNArrayIterator( 1, &matnd, 0, &nstub, &iterator ));

            type = CV_MAT_TYPE(iterator.hdr[0]->type);
            if( CV_MAT_CN(type) > 4 )
                CV_ERROR( CV_StsOutOfRange, "The input array must have at most 4 channels" );

            pass_hint = CV_MAT_DEPTH(type) == CV_32F;

            if( !pass_hint )
            {
                CvFunc2D_1A1P func = (CvFunc2D_1A1P)(sum_tab.fn_2d[type]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );
       
                do
                {
                    CvScalar temp = {{0,0,0,0}};
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.size, temp.val ));
                    sum.val[0] += temp.val[0];
                    sum.val[1] += temp.val[1];
                    sum.val[2] += temp.val[2];
                    sum.val[3] += temp.val[3];
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_1A1P1I func = (CvFunc2D_1A1P1I)(sum_tab.fn_2d[type]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );
       
                do
                {
                    CvScalar temp = {{0,0,0,0}};
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.size, temp.val, cvAlgHintAccurate ));
                    sum.val[0] += temp.val[0];
                    sum.val[1] += temp.val[1];
                    sum.val[2] += temp.val[2];
                    sum.val[3] += temp.val[3];
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));
    }

    type = CV_MAT_TYPE(mat->type);
    size = cvGetMatSize( mat );

    mat_step = mat->step;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;
        
        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
        {
            if( type == CV_32FC1 )
            {
                float* data = mat->data.fl;

                do
                {
                    sum.val[0] += data[size.width - 1];
                }
                while( --size.width );

                EXIT;
            }

            if( type == CV_64FC1 )
            {
                double* data = mat->data.db;

                do
                {
                    sum.val[0] += data[size.width - 1];
                }
                while( --size.width );

                EXIT;
            }
        }
        size.height = 1;
        mat_step = CV_STUB_STEP;
    }

    if( CV_MAT_CN(type) == 1 || coi == 0 )
    {
        int pass_hint = CV_MAT_DEPTH(type) == CV_32F;

        if( CV_MAT_CN(type) > 4 )
            CV_ERROR( CV_StsOutOfRange, "The input array must have at most 4 channels" );

        if( !pass_hint )
        {
            CvFunc2D_1A1P func = (CvFunc2D_1A1P)(sum_tab.fn_2d[type]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size, sum.val ));
        }
        else
        {
            CvFunc2D_1A1P1I func = (CvFunc2D_1A1P1I)(sum_tab.fn_2d[type]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size, sum.val, cvAlgHintAccurate ));
        }
    }
    else
    {
        CvFunc2DnC_1A1P func = (CvFunc2DnC_1A1P)(sumcoi_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

        IPPI_CALL( func( mat->data.ptr, mat_step, size,
                         CV_MAT_CN(type), coi, sum.val ));
    }

    __END__;

    return  sum;
}
// 

#define ICV_DEF_NONZERO_ALL( flavor, __op__, arrtype )              \
    ICV_DEF_SUM_NOHINT_FUNC_2D( CountNonZero, flavor, 1, __op__,    \
                                arrtype, int, int, int, 0 )         \
    ICV_DEF_SUM_NOHINT_FUNC_2D_COI( CountNonZero, flavor, __op__,   \
                                    arrtype, int, int, int, 0 )
// 
#undef  CV_NONZERO_DBL
#define CV_NONZERO_DBL(x) (((x) & CV_BIG_INT(0x7fffffffffffffff)) != 0)

ICV_DEF_NONZERO_ALL( 8u, CV_NONZERO, uchar )
ICV_DEF_NONZERO_ALL( 16s, CV_NONZERO, ushort )
ICV_DEF_NONZERO_ALL( 32s, CV_NONZERO, int )
ICV_DEF_NONZERO_ALL( 32f, CV_NONZERO_FLT, int )
ICV_DEF_NONZERO_ALL( 64f, CV_NONZERO_DBL, int64 )

#define icvCountNonZero_8s_C1R icvCountNonZero_8u_C1R
#define icvCountNonZero_8s_CnCR icvCountNonZero_8u_CnCR
#define icvCountNonZero_16u_C1R icvCountNonZero_16s_C1R
#define icvCountNonZero_16u_CnCR icvCountNonZero_16s_CnCR
// 
CV_DEF_INIT_FUNC_TAB_2D( CountNonZero, C1R )
CV_DEF_INIT_FUNC_TAB_2D( CountNonZero, CnCR )
// 
CV_IMPL int
cvCountNonZero( const CvArr* arr )
{
    static CvFuncTable nz_tab;
    static CvFuncTable nzcoi_tab;
    static int inittab = 0;

    int count = 0;

    CV_FUNCNAME("cvCountNonZero");

    __BEGIN__;

    int type, coi = 0;
    int mat_step;
    CvSize size;
    CvMat stub, *mat = (CvMat*)arr;

    if( !inittab )
    {
        icvInitCountNonZeroC1RTable( &nz_tab );
        icvInitCountNonZeroCnCRTable( &nzcoi_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(mat) )
    {
        if( CV_IS_MATND(mat) )
        {
            void* matnd = (void*)arr;
            CvMatND nstub;
            CvNArrayIterator iterator;
            CvFunc2D_1A1P func;

            CV_CALL( cvInitNArrayIterator( 1, &matnd, 0, &nstub, &iterator ));

            type = CV_MAT_TYPE(iterator.hdr[0]->type);

            if( CV_MAT_CN(type) != 1 )
                CV_ERROR( CV_BadNumChannels,
                    "Only single-channel array are supported here" );

            func = (CvFunc2D_1A1P)(nz_tab.fn_2d[CV_MAT_DEPTH(type)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );
       
            do
            {
                int temp;
                IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                 iterator.size, &temp ));
                count += temp;
            }
            while( cvNextNArraySlice( &iterator ));
            EXIT;
        }
        else
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));
    }

    type = CV_MAT_TYPE(mat->type);
    size = cvGetMatSize( mat );

    mat_step = mat->step;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;
        size.height = 1;
        mat_step = CV_STUB_STEP;
    }

    if( CV_MAT_CN(type) == 1 || coi == 0 )
    {
        CvFunc2D_1A1P func = (CvFunc2D_1A1P)(nz_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( CV_MAT_CN(type) != 1 )
            CV_ERROR( CV_BadNumChannels,
            "The function can handle only a single channel at a time (use COI)");

        if( !func )
            CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

        IPPI_CALL( func( mat->data.ptr, mat_step, size, &count ));
    }
    else
    {
        CvFunc2DnC_1A1P func = (CvFunc2DnC_1A1P)(nzcoi_tab.fn_2d[CV_MAT_DEPTH(type)]);

        if( !func )
            CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

        IPPI_CALL( func( mat->data.ptr, mat_step, size, CV_MAT_CN(type), coi, &count ));
    }

    __END__;

    return  count;
}
// 
// 
// /****************************************************************************************\
// *                                Reduce Matrix to Vector                                 *
// \****************************************************************************************/
// 
#define ICV_ACC_ROWS_FUNC( name, flavor, arrtype, acctype,      \
                           __op__, load_macro )                 \
static CvStatus CV_STDCALL                                      \
icv##name##Rows_##flavor##_C1R( const arrtype* src, int srcstep,\
                           acctype* dst, CvSize size )          \
{                                                               \
    int i, width = size.width;                                  \
    srcstep /= sizeof(src[0]);                                  \
                                                                \
    for( i = 0; i < width; i++ )                                \
        dst[i] = load_macro(src[i]);                            \
                                                                \
    for( ; --size.height;  )                                    \
    {                                                           \
        src += srcstep;                                         \
        for( i = 0; i <= width - 4; i += 4 )                    \
        {                                                       \
            acctype s0 = load_macro(src[i]);                    \
            acctype s1 = load_macro(src[i+1]);                  \
            acctype a0 = dst[i], a1 = dst[i+1];                 \
            a0 = (acctype)__op__(a0,s0); a1 = (acctype)__op__(a1,s1); \
            dst[i] = a0; dst[i+1] = a1;                         \
                                                                \
            s0 = load_macro(src[i+2]);                          \
            s1 = load_macro(src[i+3]);                          \
            a0 = dst[i+2]; a1 = dst[i+3];                       \
            a0 = (acctype)__op__(a0,s0); a1 = (acctype)__op__(a1,s1);  \
            dst[i+2] = a0; dst[i+3] = a1;                       \
        }                                                       \
                                                                \
        for( ; i < width; i++ )                                 \
        {                                                       \
            acctype s0 = load_macro(src[i]), a0 = dst[i];       \
            a0 = (acctype)__op__(a0,s0);                        \
            dst[i] = a0;                                        \
        }                                                       \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}

// 
#define ICV_ACC_COLS_FUNC_C1( name, flavor, arrtype, worktype, acctype, __op__ )\
static CvStatus CV_STDCALL                                              \
icv##name##Cols_##flavor##_C1R( const arrtype* src, int srcstep,        \
                                acctype* dst, int dststep, CvSize size )\
{                                                                       \
    int i, width = size.width;                                          \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        if( width == 1 )                                                \
            dst[0] = (acctype)src[0];                                   \
        else                                                            \
        {                                                               \
            worktype a0 = src[0], a1 = src[1];                          \
            for( i = 2; i <= width - 4; i += 4 )                        \
            {                                                           \
                worktype s0 = src[i], s1 = src[i+1];                    \
                a0 = __op__(a0, s0);                                    \
                a1 = __op__(a1, s1);                                    \
                s0 = src[i+2]; s1 = src[i+3];                           \
                a0 = __op__(a0, s0);                                    \
                a1 = __op__(a1, s1);                                    \
            }                                                           \
                                                                        \
            for( ; i < width; i++ )                                     \
            {                                                           \
                worktype s0 = src[i];                                   \
                a0 = __op__(a0, s0);                                    \
            }                                                           \
            a0 = __op__(a0, a1);                                        \
            dst[0] = (acctype)a0;                                       \
        }                                                               \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}


#define ICV_ACC_COLS_FUNC_C3( name, flavor, arrtype, worktype, acctype, __op__ ) \
static CvStatus CV_STDCALL                                              \
icv##name##Cols_##flavor##_C3R( const arrtype* src, int srcstep,        \
                                acctype* dst, int dststep, CvSize size )\
{                                                                       \
    int i, width = size.width*3;                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        worktype a0 = src[0], a1 = src[1], a2 = src[2];                 \
        for( i = 3; i < width; i += 3 )                                 \
        {                                                               \
            worktype s0 = src[i], s1 = src[i+1], s2 = src[i+2];         \
            a0 = __op__(a0, s0);                                        \
            a1 = __op__(a1, s1);                                        \
            a2 = __op__(a2, s2);                                        \
        }                                                               \
                                                                        \
        dst[0] = (acctype)a0;                                           \
        dst[1] = (acctype)a1;                                           \
        dst[2] = (acctype)a2;                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

// 
#define ICV_ACC_COLS_FUNC_C4( name, flavor, arrtype, worktype, acctype, __op__ ) \
static CvStatus CV_STDCALL                                              \
icv##name##Cols_##flavor##_C4R( const arrtype* src, int srcstep,        \
                                acctype* dst, int dststep, CvSize size )\
{                                                                       \
    int i, width = size.width*4;                                        \
    srcstep /= sizeof(src[0]);                                          \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )              \
    {                                                                   \
        worktype a0 = src[0], a1 = src[1], a2 = src[2], a3 = src[3];    \
        for( i = 4; i < width; i += 4 )                                 \
        {                                                               \
            worktype s0 = src[i], s1 = src[i+1];                        \
            a0 = __op__(a0, s0);                                        \
            a1 = __op__(a1, s1);                                        \
            s0 = src[i+2]; s1 = src[i+3];                               \
            a2 = __op__(a2, s0);                                        \
            a3 = __op__(a3, s1);                                        \
        }                                                               \
                                                                        \
        dst[0] = (acctype)a0;                                           \
        dst[1] = (acctype)a1;                                           \
        dst[2] = (acctype)a2;                                           \
        dst[3] = (acctype)a3;                                           \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}
// 
// 
ICV_ACC_ROWS_FUNC( Sum, 8u32s, uchar, int, CV_ADD, CV_NOP )
ICV_ACC_ROWS_FUNC( Sum, 8u32f, uchar, float, CV_ADD, CV_8TO32F )
ICV_ACC_ROWS_FUNC( Sum, 16u32f, ushort, float, CV_ADD, CV_NOP )
ICV_ACC_ROWS_FUNC( Sum, 16u64f, ushort, double, CV_ADD, CV_NOP )
ICV_ACC_ROWS_FUNC( Sum, 16s32f, short, float, CV_ADD, CV_NOP )
ICV_ACC_ROWS_FUNC( Sum, 16s64f, short, double, CV_ADD, CV_NOP )
ICV_ACC_ROWS_FUNC( Sum, 32f, float, float, CV_ADD, CV_NOP )
ICV_ACC_ROWS_FUNC( Sum, 32f64f, float, double, CV_ADD, CV_NOP )
ICV_ACC_ROWS_FUNC( Sum, 64f, double, double, CV_ADD, CV_NOP )

ICV_ACC_ROWS_FUNC( Max, 8u, uchar, uchar, CV_MAX_8U, CV_NOP )
ICV_ACC_ROWS_FUNC( Max, 32f, float, float, MAX, CV_NOP )
ICV_ACC_ROWS_FUNC( Max, 64f, double, double, MAX, CV_NOP )

ICV_ACC_ROWS_FUNC( Min, 8u, uchar, uchar, CV_MIN_8U, CV_NOP )
ICV_ACC_ROWS_FUNC( Min, 32f, float, float, MIN, CV_NOP )
ICV_ACC_ROWS_FUNC( Min, 64f, double, double, MIN, CV_NOP )

ICV_ACC_COLS_FUNC_C1( Sum, 8u32s, uchar, int, int, CV_ADD )
ICV_ACC_COLS_FUNC_C1( Sum, 8u32f, uchar, int, float, CV_ADD )
ICV_ACC_COLS_FUNC_C1( Sum, 16u32f, ushort, float, float, CV_ADD )
ICV_ACC_COLS_FUNC_C1( Sum, 16u64f, ushort, double, double, CV_ADD )
ICV_ACC_COLS_FUNC_C1( Sum, 16s32f, short, float, float, CV_ADD )
ICV_ACC_COLS_FUNC_C1( Sum, 16s64f, short, double, double, CV_ADD )

ICV_ACC_COLS_FUNC_C1( Sum, 32f, float, float, float, CV_ADD )
ICV_ACC_COLS_FUNC_C1( Sum, 32f64f, float, double, double, CV_ADD )
ICV_ACC_COLS_FUNC_C1( Sum, 64f, double, double, double, CV_ADD )
ICV_ACC_COLS_FUNC_C3( Sum, 8u32s, uchar, int, int, CV_ADD )
ICV_ACC_COLS_FUNC_C3( Sum, 8u32f, uchar, int, float, CV_ADD )
ICV_ACC_COLS_FUNC_C3( Sum, 32f, float, float, float, CV_ADD )
ICV_ACC_COLS_FUNC_C3( Sum, 64f, double, double, double, CV_ADD )
ICV_ACC_COLS_FUNC_C4( Sum, 8u32s, uchar, int, int, CV_ADD )
ICV_ACC_COLS_FUNC_C4( Sum, 8u32f, uchar, int, float, CV_ADD )
ICV_ACC_COLS_FUNC_C4( Sum, 32f, float, float, float, CV_ADD )
ICV_ACC_COLS_FUNC_C4( Sum, 64f, double, double, double, CV_ADD )

ICV_ACC_COLS_FUNC_C1( Max, 8u, uchar, int, uchar, CV_MAX_8U )
ICV_ACC_COLS_FUNC_C1( Max, 32f, float, float, float, MAX )
ICV_ACC_COLS_FUNC_C1( Max, 64f, double, double, double, MAX )

ICV_ACC_COLS_FUNC_C1( Min, 8u, uchar, int, uchar, CV_MIN_8U )
ICV_ACC_COLS_FUNC_C1( Min, 32f, float, float, float, MIN )
ICV_ACC_COLS_FUNC_C1( Min, 64f, double, double, double, MIN )

typedef CvStatus (CV_STDCALL * CvReduceToRowFunc)
    ( const void* src, int srcstep, void* dst, CvSize size );

typedef CvStatus (CV_STDCALL * CvReduceToColFunc)
    ( const void* src, int srcstep, void* dst, int dststep, CvSize size );

// 
CV_IMPL void
cvReduce( const CvArr* srcarr, CvArr* dstarr, int dim, int op )
{
    CvMat* temp = 0;
    
    CV_FUNCNAME( "cvReduce" );

    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr, *dst0;
    int sdepth, ddepth, cn, op0 = op;
    CvSize size;

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &sstub ));

    if( !CV_IS_MAT(dst) )
        CV_CALL( dst = cvGetMat( dst, &dstub ));

    if( !CV_ARE_CNS_EQ(src, dst) )
        CV_ERROR( CV_StsUnmatchedFormats, "Input and output arrays must have the same number of channels" );

    sdepth = CV_MAT_DEPTH(src->type);
    ddepth = CV_MAT_DEPTH(dst->type);
    cn = CV_MAT_CN(src->type);
    dst0 = dst;

    size = cvGetMatSize(src);

    if( dim < 0 )
        dim = src->rows > dst->rows ? 0 : src->cols > dst->cols ? 1 : dst->cols == 1;

    if( dim > 1 )
        CV_ERROR( CV_StsOutOfRange, "The reduced dimensionality index is out of range" );

    if( dim == 0 && (dst->cols != src->cols || dst->rows != 1) ||
        dim == 1 && (dst->rows != src->rows || dst->cols != 1) )
        CV_ERROR( CV_StsBadSize, "The output array size is incorrect" );

    if( op == CV_REDUCE_AVG )
    {
        int ttype = sdepth == CV_8U ? CV_MAKETYPE(CV_32S,cn) : dst->type;
        if( ttype != dst->type )
            CV_CALL( dst = temp = cvCreateMat( dst->rows, dst->cols, ttype ));
        op = CV_REDUCE_SUM;
        ddepth = CV_MAT_DEPTH(ttype);
    }

    if( op != CV_REDUCE_SUM && op != CV_REDUCE_MAX && op != CV_REDUCE_MIN )
        CV_ERROR( CV_StsBadArg, "Unknown reduce operation index, must be one of CV_REDUCE_*" );

    if( dim == 0 )
    {
        CvReduceToRowFunc rfunc =
            op == CV_REDUCE_SUM ?
            (sdepth == CV_8U && ddepth == CV_32S ? (CvReduceToRowFunc)icvSumRows_8u32s_C1R :
             sdepth == CV_8U && ddepth == CV_32F ? (CvReduceToRowFunc)icvSumRows_8u32f_C1R :
             sdepth == CV_16U && ddepth == CV_32F ? (CvReduceToRowFunc)icvSumRows_16u32f_C1R :
             sdepth == CV_16U && ddepth == CV_64F ? (CvReduceToRowFunc)icvSumRows_16u64f_C1R :
             sdepth == CV_16S && ddepth == CV_32F ? (CvReduceToRowFunc)icvSumRows_16s32f_C1R :
             sdepth == CV_16S && ddepth == CV_64F ? (CvReduceToRowFunc)icvSumRows_16s64f_C1R :
             sdepth == CV_32F && ddepth == CV_32F ? (CvReduceToRowFunc)icvSumRows_32f_C1R :
             sdepth == CV_32F && ddepth == CV_64F ? (CvReduceToRowFunc)icvSumRows_32f64f_C1R :        
             sdepth == CV_64F && ddepth == CV_64F ? (CvReduceToRowFunc)icvSumRows_64f_C1R : 0) :
            op == CV_REDUCE_MAX ?
            (sdepth == CV_8U && ddepth == CV_8U ? (CvReduceToRowFunc)icvMaxRows_8u_C1R :
             sdepth == CV_32F && ddepth == CV_32F ? (CvReduceToRowFunc)icvMaxRows_32f_C1R :
             sdepth == CV_64F && ddepth == CV_64F ? (CvReduceToRowFunc)icvMaxRows_64f_C1R : 0) :

            (sdepth == CV_8U && ddepth == CV_8U ? (CvReduceToRowFunc)icvMinRows_8u_C1R :
             sdepth == CV_32F && ddepth == CV_32F ? (CvReduceToRowFunc)icvMinRows_32f_C1R :
             sdepth == CV_64F && ddepth == CV_64F ? (CvReduceToRowFunc)icvMinRows_64f_C1R : 0);

        if( !rfunc )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Unsupported combination of input and output array formats" );

        size.width *= cn;
        IPPI_CALL( rfunc( src->data.ptr, src->step ? src->step : CV_STUB_STEP,
                          dst->data.ptr, size ));
    }
    else
    {
        CvReduceToColFunc cfunc =
            op == CV_REDUCE_SUM ?
            (sdepth == CV_8U && ddepth == CV_32S ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_8u32s_C1R :
                                cn == 3 ? icvSumCols_8u32s_C3R :
                                cn == 4 ? icvSumCols_8u32s_C4R : 0) :
             sdepth == CV_8U && ddepth == CV_32F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_8u32f_C1R :
                                cn == 3 ? icvSumCols_8u32f_C3R :
                                cn == 4 ? icvSumCols_8u32f_C4R : 0) :
             sdepth == CV_16U && ddepth == CV_32F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_16u32f_C1R : 0) :
             sdepth == CV_16U && ddepth == CV_64F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_16u64f_C1R : 0) :
             sdepth == CV_16S && ddepth == CV_32F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_16s32f_C1R : 0) :
             sdepth == CV_16S && ddepth == CV_64F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_16s64f_C1R : 0) :
             sdepth == CV_32F && ddepth == CV_32F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_32f_C1R :
                                cn == 3 ? icvSumCols_32f_C3R :
                                cn == 4 ? icvSumCols_32f_C4R : 0) :
             sdepth == CV_32F && ddepth == CV_64F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_32f64f_C1R : 0) :
             sdepth == CV_64F && ddepth == CV_64F ?
            (CvReduceToColFunc)(cn == 1 ? icvSumCols_64f_C1R :
                                cn == 3 ? icvSumCols_64f_C3R :
                                cn == 4 ? icvSumCols_64f_C4R : 0) : 0) :
             op == CV_REDUCE_MAX && cn == 1 ?
             (sdepth == CV_8U && ddepth == CV_8U ? (CvReduceToColFunc)icvMaxCols_8u_C1R :
              sdepth == CV_32F && ddepth == CV_32F ? (CvReduceToColFunc)icvMaxCols_32f_C1R :
              sdepth == CV_64F && ddepth == CV_64F ? (CvReduceToColFunc)icvMaxCols_64f_C1R : 0) :
             op == CV_REDUCE_MIN && cn == 1 ?
             (sdepth == CV_8U && ddepth == CV_8U ? (CvReduceToColFunc)icvMinCols_8u_C1R :
              sdepth == CV_32F && ddepth == CV_32F ? (CvReduceToColFunc)icvMinCols_32f_C1R :
              sdepth == CV_64F && ddepth == CV_64F ? (CvReduceToColFunc)icvMinCols_64f_C1R : 0) : 0;

        if( !cfunc )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Unsupported combination of input and output array formats" );

        IPPI_CALL( cfunc( src->data.ptr, src->step ? src->step : CV_STUB_STEP,
                          dst->data.ptr, dst->step ? dst->step : CV_STUB_STEP, size ));
    }

    if( op0 == CV_REDUCE_AVG )
        cvScale( dst, dst0, 1./(dim == 0 ? src->rows : src->cols) );

    __END__;

    if( temp )
        cvReleaseMat( &temp );
}
// 
// /* End of file. */