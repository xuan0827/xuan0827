//cvsumpixels
#include "stdafx.h"
#include "mmplab_cv.h"
#include <limits.h>
 #include <stdio.h>
// 
#define ICV_DEF_INTEGRAL_OP_C1( flavor, arrtype, sumtype, sqsumtype, worktype,  \
                                cast_macro, cast_sqr_macro )    \
static CvStatus CV_STDCALL                                      \
icvIntegralImage_##flavor##_C1R( const arrtype* src, int srcstep,\
                                 sumtype* sum, int sumstep,     \
                                 sqsumtype* sqsum, int sqsumstep,\
                                 sumtype* tilted, int tiltedstep,\
                                 CvSize size )                  \
{                                                               \
    int x, y;                                                   \
    sumtype s;                                                  \
    sqsumtype sq;                                               \
    sumtype* buf = 0;                                           \
                                                                \
    srcstep /= sizeof(src[0]);                                  \
                                                                \
    memset( sum, 0, (size.width+1)*sizeof(sum[0]));             \
    sumstep /= sizeof(sum[0]);                                  \
    sum += sumstep + 1;                                         \
                                                                \
    if( sqsum )                                                 \
    {                                                           \
        memset( sqsum, 0, (size.width+1)*sizeof(sqsum[0]));     \
        sqsumstep /= sizeof(sqsum[0]);                          \
        sqsum += sqsumstep + 1;                                 \
    }                                                           \
                                                                \
    if( tilted )                                                \
    {                                                           \
        memset( tilted, 0, (size.width+1)*sizeof(tilted[0]));   \
        tiltedstep /= sizeof(tilted[0]);                        \
        tilted += tiltedstep + 1;                               \
    }                                                           \
                                                                \
    if( sqsum == 0 && tilted == 0 )                             \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                                          sum += sumstep )      \
        {                                                       \
            sum[-1] = 0;                                        \
            for( x = 0, s = 0; x < size.width; x++ )            \
            {                                                   \
                sumtype t = cast_macro(src[x]);                 \
                s += t;                                         \
                sum[x] = sum[x - sumstep] + s;                  \
            }                                                   \
        }                                                       \
    }                                                           \
    else if( tilted == 0 )                                      \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                        sum += sumstep, sqsum += sqsumstep )    \
        {                                                       \
            sum[-1] = 0;                                        \
            sqsum[-1] = 0;                                      \
                                                                \
            for( x = 0, s = 0, sq = 0; x < size.width; x++ )    \
            {                                                   \
                worktype it = src[x];                           \
                sumtype t = cast_macro(it);                     \
                sqsumtype tq = cast_sqr_macro(it);              \
                s += t;                                         \
                sq += tq;                                       \
                t = sum[x - sumstep] + s;                       \
                tq = sqsum[x - sqsumstep] + sq;                 \
                sum[x] = t;                                     \
                sqsum[x] = tq;                                  \
            }                                                   \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        if( sqsum == 0 )                                        \
        {                                                       \
            assert(0);                                          \
            return CV_NULLPTR_ERR;                              \
        }                                                       \
                                                                \
        buf = (sumtype*)cvStackAlloc((size.width + 1 )* sizeof(buf[0]));\
        sum[-1] = tilted[-1] = 0;                               \
        sqsum[-1] = 0;                                          \
                                                                \
        for( x = 0, s = 0, sq = 0; x < size.width; x++ )        \
        {                                                       \
            worktype it = src[x];                               \
            sumtype t = cast_macro(it);                         \
            sqsumtype tq = cast_sqr_macro(it);                  \
            buf[x] = tilted[x] = t;                             \
            s += t;                                             \
            sq += tq;                                           \
            sum[x] = s;                                         \
            sqsum[x] = sq;                                      \
        }                                                       \
                                                                \
        if( size.width == 1 )                                   \
            buf[1] = 0;                                         \
                                                                \
        for( y = 1; y < size.height; y++ )                      \
        {                                                       \
            worktype it;                                        \
            sumtype t0;                                         \
            sqsumtype tq0;                                      \
                                                                \
            src += srcstep;                                     \
            sum += sumstep;                                     \
            sqsum += sqsumstep;                                 \
            tilted += tiltedstep;                               \
                                                                \
            it = src[0/*x*/];                                   \
            s = t0 = cast_macro(it);                            \
            sq = tq0 = cast_sqr_macro(it);                      \
                                                                \
            sum[-1] = 0;                                        \
            sqsum[-1] = 0;                                      \
            /*tilted[-1] = buf[0];*/                            \
            tilted[-1] = tilted[-tiltedstep];                   \
                                                                \
            sum[0] = sum[-sumstep] + t0;                        \
            sqsum[0] = sqsum[-sqsumstep] + tq0;                 \
            tilted[0] = tilted[-tiltedstep] + t0 + buf[1];      \
                                                                \
            for( x = 1; x < size.width - 1; x++ )               \
            {                                                   \
                sumtype t1 = buf[x];                            \
                buf[x-1] = t1 + t0;                             \
                it = src[x];                                    \
                t0 = cast_macro(it);                            \
                tq0 = cast_sqr_macro(it);                       \
                s += t0;                                        \
                sq += tq0;                                      \
                sum[x] = sum[x - sumstep] + s;                  \
                sqsum[x] = sqsum[x - sqsumstep] + sq;           \
                t1 += buf[x+1] + t0 + tilted[x - tiltedstep - 1];\
                tilted[x] = t1;                                 \
            }                                                   \
                                                                \
            if( size.width > 1 )                                \
            {                                                   \
                sumtype t1 = buf[x];                            \
                buf[x-1] = t1 + t0;                             \
                it = src[x];    /*+*/                           \
                t0 = cast_macro(it);                            \
                tq0 = cast_sqr_macro(it);                       \
                s += t0;                                        \
                sq += tq0;                                      \
                sum[x] = sum[x - sumstep] + s;                  \
                sqsum[x] = sqsum[x - sqsumstep] + sq;           \
                tilted[x] = t0 + t1 + tilted[x - tiltedstep - 1];\
                buf[x] = t0;                                    \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


ICV_DEF_INTEGRAL_OP_C1( 8u32s, uchar, int, double, int, CV_NOP, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_C1( 8u64f, uchar, double, double, int, CV_8TO32F, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_C1( 32f64f, float, double, double, double, CV_NOP, CV_SQR )
ICV_DEF_INTEGRAL_OP_C1( 64f, double, double, double, double, CV_NOP, CV_SQR )


#define ICV_DEF_INTEGRAL_OP_CN( flavor, arrtype, sumtype, sqsumtype,    \
                                worktype, cast_macro, cast_sqr_macro )  \
static CvStatus CV_STDCALL                                      \
icvIntegralImage_##flavor##_CnR( const arrtype* src, int srcstep,\
                                 sumtype* sum, int sumstep,     \
                                 sqsumtype* sqsum, int sqsumstep,\
                                 CvSize size, int cn )          \
{                                                               \
    int x, y;                                                   \
    srcstep /= sizeof(src[0]);                                  \
                                                                \
    memset( sum, 0, (size.width+1)*cn*sizeof(sum[0]));          \
    sumstep /= sizeof(sum[0]);                                  \
    sum += sumstep + cn;                                        \
                                                                \
    if( sqsum )                                                 \
    {                                                           \
        memset( sqsum, 0, (size.width+1)*cn*sizeof(sqsum[0]));  \
        sqsumstep /= sizeof(sqsum[0]);                          \
        sqsum += sqsumstep + cn;                                \
    }                                                           \
                                                                \
    size.width *= cn;                                           \
                                                                \
    if( sqsum == 0 )                                            \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                                          sum += sumstep )      \
        {                                                       \
            for( x = -cn; x < 0; x++ )                          \
                sum[x] = 0;                                     \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
                sum[x] = cast_macro(src[x]) + sum[x - cn];      \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
                sum[x] = sum[x] + sum[x - sumstep];             \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        for( y = 0; y < size.height; y++, src += srcstep,       \
                        sum += sumstep, sqsum += sqsumstep )    \
        {                                                       \
            for( x = -cn; x < 0; x++ )                          \
            {                                                   \
                sum[x] = 0;                                     \
                sqsum[x] = 0;                                   \
            }                                                   \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
            {                                                   \
                worktype it = src[x];                           \
                sumtype t = cast_macro(it) + sum[x-cn];         \
                sqsumtype tq = cast_sqr_macro(it) + sqsum[x-cn];\
                sum[x] = t;                                     \
                sqsum[x] = tq;                                  \
            }                                                   \
                                                                \
            for( x = 0; x < size.width; x++ )                   \
            {                                                   \
                sumtype t = sum[x] + sum[x - sumstep];          \
                sqsumtype tq = sqsum[x] + sqsum[x - sqsumstep]; \
                sum[x] = t;                                     \
                sqsum[x] = tq;                                  \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


ICV_DEF_INTEGRAL_OP_CN( 8u32s, uchar, int, double, int, CV_NOP, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_CN( 8u64f, uchar, double, double, int, CV_8TO32F, CV_8TO32F_SQR )
ICV_DEF_INTEGRAL_OP_CN( 32f64f, float, double, double, double, CV_NOP, CV_SQR )
ICV_DEF_INTEGRAL_OP_CN( 64f, double, double, double, double, CV_NOP, CV_SQR )

// 
static void icvInitIntegralImageTable( CvFuncTable* table_c1, CvFuncTable* table_cn )
{
    table_c1->fn_2d[CV_8U] = (void*)icvIntegralImage_8u64f_C1R;
    table_c1->fn_2d[CV_32F] = (void*)icvIntegralImage_32f64f_C1R;
    table_c1->fn_2d[CV_64F] = (void*)icvIntegralImage_64f_C1R;

    table_cn->fn_2d[CV_8U] = (void*)icvIntegralImage_8u64f_CnR;
    table_cn->fn_2d[CV_32F] = (void*)icvIntegralImage_32f64f_CnR;
    table_cn->fn_2d[CV_64F] = (void*)icvIntegralImage_64f_CnR;
}
// 
// 
typedef CvStatus (CV_STDCALL * CvIntegralImageFuncC1)(
    const void* src, int srcstep, void* sum, int sumstep,
    void* sqsum, int sqsumstep, void* tilted, int tiltedstep,
    CvSize size );

typedef CvStatus (CV_STDCALL * CvIntegralImageFuncCn)(
    const void* src, int srcstep, void* sum, int sumstep,
    void* sqsum, int sqsumstep, CvSize size, int cn );
// 
icvIntegral_8u32s_C1R_t icvIntegral_8u32s_C1R_p = 0;
icvSqrIntegral_8u32s64f_C1R_t icvSqrIntegral_8u32s64f_C1R_p = 0;
// 
CV_IMPL void
cvIntegral( const CvArr* image, CvArr* sumImage,
            CvArr* sumSqImage, CvArr* tiltedSumImage )
{
    static CvFuncTable tab_c1, tab_cn;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvIntegralImage" );

    __BEGIN__;

    CvMat src_stub, *src = (CvMat*)image;
    CvMat sum_stub, *sum = (CvMat*)sumImage;
    CvMat sqsum_stub, *sqsum = (CvMat*)sumSqImage;
    CvMat tilted_stub, *tilted = (CvMat*)tiltedSumImage;
    int coi0 = 0, coi1 = 0, coi2 = 0, coi3 = 0;
    int depth, cn;
    int src_step, sum_step, sqsum_step, tilted_step;
    CvIntegralImageFuncC1 func_c1 = 0;
    CvIntegralImageFuncCn func_cn = 0;
    CvSize size;

    if( !inittab )
    {
        icvInitIntegralImageTable( &tab_c1, &tab_cn );
        inittab = 1;
    }

    CV_CALL( src = cvGetMat( src, &src_stub, &coi0 ));
    CV_CALL( sum = cvGetMat( sum, &sum_stub, &coi1 ));
    
    if( sum->width != src->width + 1 ||
        sum->height != src->height + 1 )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( CV_MAT_DEPTH( sum->type ) != CV_64F &&
        (CV_MAT_DEPTH( src->type ) != CV_8U ||
         CV_MAT_DEPTH( sum->type ) != CV_32S ) ||
        !CV_ARE_CNS_EQ( src, sum ))
        CV_ERROR( CV_StsUnsupportedFormat,
        "Sum array must have 64f type (or 32s type in case of 8u source array) "
        "and the same number of channels as the source array" );

    if( sqsum )
    {
        CV_CALL( sqsum = cvGetMat( sqsum, &sqsum_stub, &coi2 ));
        if( !CV_ARE_SIZES_EQ( sum, sqsum ) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        if( CV_MAT_DEPTH( sqsum->type ) != CV_64F || !CV_ARE_CNS_EQ( src, sqsum ))
            CV_ERROR( CV_StsUnsupportedFormat,
                      "Squares sum array must be 64f "
                      "and the same number of channels as the source array" );
    }

    if( tilted )
    {
        if( !sqsum )
            CV_ERROR( CV_StsNullPtr,
            "Squared sum array must be passed if tilted sum array is passed" );

        CV_CALL( tilted = cvGetMat( tilted, &tilted_stub, &coi3 ));
        if( !CV_ARE_SIZES_EQ( sum, tilted ) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        if( !CV_ARE_TYPES_EQ( sum, tilted ) )
            CV_ERROR( CV_StsUnmatchedFormats,
                      "Sum and tilted sum must have the same types" );
        if( CV_MAT_CN(tilted->type) != 1 )
            CV_ERROR( CV_StsNotImplemented,
                      "Tilted sum can not be computed for multi-channel arrays" );
    }

    if( coi0 || coi1 || coi2 || coi3 )
        CV_ERROR( CV_BadCOI, "COI is not supported by the function" );

    depth = CV_MAT_DEPTH(src->type);
    cn = CV_MAT_CN(src->type);

    if( CV_MAT_DEPTH( sum->type ) == CV_32S )
    {
        func_c1 = (CvIntegralImageFuncC1)icvIntegralImage_8u32s_C1R;
        func_cn = (CvIntegralImageFuncCn)icvIntegralImage_8u32s_CnR;
    }
    else
    {
        func_c1 = (CvIntegralImageFuncC1)tab_c1.fn_2d[depth];
        func_cn = (CvIntegralImageFuncCn)tab_cn.fn_2d[depth];
        if( !func_c1 && !func_cn )
            CV_ERROR( CV_StsUnsupportedFormat, "This source image format is unsupported" );
    }

    size = cvGetMatSize(src);
    src_step = src->step ? src->step : CV_STUB_STEP;
    sum_step = sum->step ? sum->step : CV_STUB_STEP;
    sqsum_step = !sqsum ? 0 : sqsum->step ? sqsum->step : CV_STUB_STEP;
    tilted_step = !tilted ? 0 : tilted->step ? tilted->step : CV_STUB_STEP;

    if( cn == 1 )
    {
        if( depth == CV_8U && !tilted && CV_MAT_DEPTH(sum->type) == CV_32S )
        {
            if( !sqsum && icvIntegral_8u32s_C1R_p &&
                icvIntegral_8u32s_C1R_p( src->data.ptr, src_step,
                            sum->data.i, sum_step, size, 0 ) >= 0 )
                EXIT;
            
            if( sqsum && icvSqrIntegral_8u32s64f_C1R_p &&
                icvSqrIntegral_8u32s64f_C1R_p( src->data.ptr, src_step, sum->data.i,
                            sum_step, sqsum->data.db, sqsum_step, size, 0, 0 ) >= 0 )
                EXIT;
        }

        IPPI_CALL( func_c1( src->data.ptr, src_step, sum->data.ptr, sum_step,
                        sqsum ? sqsum->data.ptr : 0, sqsum_step,
                        tilted ? tilted->data.ptr : 0, tilted_step, size ));
    }
    else
    {
        IPPI_CALL( func_cn( src->data.ptr, src_step, sum->data.ptr, sum_step,
                        sqsum ? sqsum->data.ptr : 0, sqsum_step, size, cn ));
    }

    __END__;
}
// 
// 
// /* End of file. */
//  CvMat helper tables
//
// */


const float icv8x32fTab_cv[] =
{
    -256.f, -255.f, -254.f, -253.f, -252.f, -251.f, -250.f, -249.f,
    -248.f, -247.f, -246.f, -245.f, -244.f, -243.f, -242.f, -241.f,
    -240.f, -239.f, -238.f, -237.f, -236.f, -235.f, -234.f, -233.f,
    -232.f, -231.f, -230.f, -229.f, -228.f, -227.f, -226.f, -225.f,
    -224.f, -223.f, -222.f, -221.f, -220.f, -219.f, -218.f, -217.f,
    -216.f, -215.f, -214.f, -213.f, -212.f, -211.f, -210.f, -209.f,
    -208.f, -207.f, -206.f, -205.f, -204.f, -203.f, -202.f, -201.f,
    -200.f, -199.f, -198.f, -197.f, -196.f, -195.f, -194.f, -193.f,
    -192.f, -191.f, -190.f, -189.f, -188.f, -187.f, -186.f, -185.f,
    -184.f, -183.f, -182.f, -181.f, -180.f, -179.f, -178.f, -177.f,
    -176.f, -175.f, -174.f, -173.f, -172.f, -171.f, -170.f, -169.f,
    -168.f, -167.f, -166.f, -165.f, -164.f, -163.f, -162.f, -161.f,
    -160.f, -159.f, -158.f, -157.f, -156.f, -155.f, -154.f, -153.f,
    -152.f, -151.f, -150.f, -149.f, -148.f, -147.f, -146.f, -145.f,
    -144.f, -143.f, -142.f, -141.f, -140.f, -139.f, -138.f, -137.f,
    -136.f, -135.f, -134.f, -133.f, -132.f, -131.f, -130.f, -129.f,
    -128.f, -127.f, -126.f, -125.f, -124.f, -123.f, -122.f, -121.f,
    -120.f, -119.f, -118.f, -117.f, -116.f, -115.f, -114.f, -113.f,
    -112.f, -111.f, -110.f, -109.f, -108.f, -107.f, -106.f, -105.f,
    -104.f, -103.f, -102.f, -101.f, -100.f,  -99.f,  -98.f,  -97.f,
     -96.f,  -95.f,  -94.f,  -93.f,  -92.f,  -91.f,  -90.f,  -89.f,
     -88.f,  -87.f,  -86.f,  -85.f,  -84.f,  -83.f,  -82.f,  -81.f,
     -80.f,  -79.f,  -78.f,  -77.f,  -76.f,  -75.f,  -74.f,  -73.f,
     -72.f,  -71.f,  -70.f,  -69.f,  -68.f,  -67.f,  -66.f,  -65.f,
     -64.f,  -63.f,  -62.f,  -61.f,  -60.f,  -59.f,  -58.f,  -57.f,
     -56.f,  -55.f,  -54.f,  -53.f,  -52.f,  -51.f,  -50.f,  -49.f,
     -48.f,  -47.f,  -46.f,  -45.f,  -44.f,  -43.f,  -42.f,  -41.f,
     -40.f,  -39.f,  -38.f,  -37.f,  -36.f,  -35.f,  -34.f,  -33.f,
     -32.f,  -31.f,  -30.f,  -29.f,  -28.f,  -27.f,  -26.f,  -25.f,
     -24.f,  -23.f,  -22.f,  -21.f,  -20.f,  -19.f,  -18.f,  -17.f,
     -16.f,  -15.f,  -14.f,  -13.f,  -12.f,  -11.f,  -10.f,   -9.f,
      -8.f,   -7.f,   -6.f,   -5.f,   -4.f,   -3.f,   -2.f,   -1.f,
       0.f,    1.f,    2.f,    3.f,    4.f,    5.f,    6.f,    7.f,
       8.f,    9.f,   10.f,   11.f,   12.f,   13.f,   14.f,   15.f,
      16.f,   17.f,   18.f,   19.f,   20.f,   21.f,   22.f,   23.f,
      24.f,   25.f,   26.f,   27.f,   28.f,   29.f,   30.f,   31.f,
      32.f,   33.f,   34.f,   35.f,   36.f,   37.f,   38.f,   39.f,
      40.f,   41.f,   42.f,   43.f,   44.f,   45.f,   46.f,   47.f,
      48.f,   49.f,   50.f,   51.f,   52.f,   53.f,   54.f,   55.f,
      56.f,   57.f,   58.f,   59.f,   60.f,   61.f,   62.f,   63.f,
      64.f,   65.f,   66.f,   67.f,   68.f,   69.f,   70.f,   71.f,
      72.f,   73.f,   74.f,   75.f,   76.f,   77.f,   78.f,   79.f,
      80.f,   81.f,   82.f,   83.f,   84.f,   85.f,   86.f,   87.f,
      88.f,   89.f,   90.f,   91.f,   92.f,   93.f,   94.f,   95.f,
      96.f,   97.f,   98.f,   99.f,  100.f,  101.f,  102.f,  103.f,
     104.f,  105.f,  106.f,  107.f,  108.f,  109.f,  110.f,  111.f,
     112.f,  113.f,  114.f,  115.f,  116.f,  117.f,  118.f,  119.f,
     120.f,  121.f,  122.f,  123.f,  124.f,  125.f,  126.f,  127.f,
     128.f,  129.f,  130.f,  131.f,  132.f,  133.f,  134.f,  135.f,
     136.f,  137.f,  138.f,  139.f,  140.f,  141.f,  142.f,  143.f,
     144.f,  145.f,  146.f,  147.f,  148.f,  149.f,  150.f,  151.f,
     152.f,  153.f,  154.f,  155.f,  156.f,  157.f,  158.f,  159.f,
     160.f,  161.f,  162.f,  163.f,  164.f,  165.f,  166.f,  167.f,
     168.f,  169.f,  170.f,  171.f,  172.f,  173.f,  174.f,  175.f,
     176.f,  177.f,  178.f,  179.f,  180.f,  181.f,  182.f,  183.f,
     184.f,  185.f,  186.f,  187.f,  188.f,  189.f,  190.f,  191.f,
     192.f,  193.f,  194.f,  195.f,  196.f,  197.f,  198.f,  199.f,
     200.f,  201.f,  202.f,  203.f,  204.f,  205.f,  206.f,  207.f,
     208.f,  209.f,  210.f,  211.f,  212.f,  213.f,  214.f,  215.f,
     216.f,  217.f,  218.f,  219.f,  220.f,  221.f,  222.f,  223.f,
     224.f,  225.f,  226.f,  227.f,  228.f,  229.f,  230.f,  231.f,
     232.f,  233.f,  234.f,  235.f,  236.f,  237.f,  238.f,  239.f,
     240.f,  241.f,  242.f,  243.f,  244.f,  245.f,  246.f,  247.f,
     248.f,  249.f,  250.f,  251.f,  252.f,  253.f,  254.f,  255.f,
     256.f,  257.f,  258.f,  259.f,  260.f,  261.f,  262.f,  263.f,
     264.f,  265.f,  266.f,  267.f,  268.f,  269.f,  270.f,  271.f,
     272.f,  273.f,  274.f,  275.f,  276.f,  277.f,  278.f,  279.f,
     280.f,  281.f,  282.f,  283.f,  284.f,  285.f,  286.f,  287.f,
     288.f,  289.f,  290.f,  291.f,  292.f,  293.f,  294.f,  295.f,
     296.f,  297.f,  298.f,  299.f,  300.f,  301.f,  302.f,  303.f,
     304.f,  305.f,  306.f,  307.f,  308.f,  309.f,  310.f,  311.f,
     312.f,  313.f,  314.f,  315.f,  316.f,  317.f,  318.f,  319.f,
     320.f,  321.f,  322.f,  323.f,  324.f,  325.f,  326.f,  327.f,
     328.f,  329.f,  330.f,  331.f,  332.f,  333.f,  334.f,  335.f,
     336.f,  337.f,  338.f,  339.f,  340.f,  341.f,  342.f,  343.f,
     344.f,  345.f,  346.f,  347.f,  348.f,  349.f,  350.f,  351.f,
     352.f,  353.f,  354.f,  355.f,  356.f,  357.f,  358.f,  359.f,
     360.f,  361.f,  362.f,  363.f,  364.f,  365.f,  366.f,  367.f,
     368.f,  369.f,  370.f,  371.f,  372.f,  373.f,  374.f,  375.f,
     376.f,  377.f,  378.f,  379.f,  380.f,  381.f,  382.f,  383.f,
     384.f,  385.f,  386.f,  387.f,  388.f,  389.f,  390.f,  391.f,
     392.f,  393.f,  394.f,  395.f,  396.f,  397.f,  398.f,  399.f,
     400.f,  401.f,  402.f,  403.f,  404.f,  405.f,  406.f,  407.f,
     408.f,  409.f,  410.f,  411.f,  412.f,  413.f,  414.f,  415.f,
     416.f,  417.f,  418.f,  419.f,  420.f,  421.f,  422.f,  423.f,
     424.f,  425.f,  426.f,  427.f,  428.f,  429.f,  430.f,  431.f,
     432.f,  433.f,  434.f,  435.f,  436.f,  437.f,  438.f,  439.f,
     440.f,  441.f,  442.f,  443.f,  444.f,  445.f,  446.f,  447.f,
     448.f,  449.f,  450.f,  451.f,  452.f,  453.f,  454.f,  455.f,
     456.f,  457.f,  458.f,  459.f,  460.f,  461.f,  462.f,  463.f,
     464.f,  465.f,  466.f,  467.f,  468.f,  469.f,  470.f,  471.f,
     472.f,  473.f,  474.f,  475.f,  476.f,  477.f,  478.f,  479.f,
     480.f,  481.f,  482.f,  483.f,  484.f,  485.f,  486.f,  487.f,
     488.f,  489.f,  490.f,  491.f,  492.f,  493.f,  494.f,  495.f,
     496.f,  497.f,  498.f,  499.f,  500.f,  501.f,  502.f,  503.f,
     504.f,  505.f,  506.f,  507.f,  508.f,  509.f,  510.f,  511.f,
};

const float icv8x32fSqrTab[] =
{
 16384.f,  16129.f,  15876.f,  15625.f,  15376.f,  15129.f,  14884.f,  14641.f,
 14400.f,  14161.f,  13924.f,  13689.f,  13456.f,  13225.f,  12996.f,  12769.f,
 12544.f,  12321.f,  12100.f,  11881.f,  11664.f,  11449.f,  11236.f,  11025.f,
 10816.f,  10609.f,  10404.f,  10201.f,  10000.f,   9801.f,   9604.f,   9409.f,
  9216.f,   9025.f,   8836.f,   8649.f,   8464.f,   8281.f,   8100.f,   7921.f,
  7744.f,   7569.f,   7396.f,   7225.f,   7056.f,   6889.f,   6724.f,   6561.f,
  6400.f,   6241.f,   6084.f,   5929.f,   5776.f,   5625.f,   5476.f,   5329.f,
  5184.f,   5041.f,   4900.f,   4761.f,   4624.f,   4489.f,   4356.f,   4225.f,
  4096.f,   3969.f,   3844.f,   3721.f,   3600.f,   3481.f,   3364.f,   3249.f,
  3136.f,   3025.f,   2916.f,   2809.f,   2704.f,   2601.f,   2500.f,   2401.f,
  2304.f,   2209.f,   2116.f,   2025.f,   1936.f,   1849.f,   1764.f,   1681.f,
  1600.f,   1521.f,   1444.f,   1369.f,   1296.f,   1225.f,   1156.f,   1089.f,
  1024.f,    961.f,    900.f,    841.f,    784.f,    729.f,    676.f,    625.f,
   576.f,    529.f,    484.f,    441.f,    400.f,    361.f,    324.f,    289.f,
   256.f,    225.f,    196.f,    169.f,    144.f,    121.f,    100.f,     81.f,
    64.f,     49.f,     36.f,     25.f,     16.f,      9.f,      4.f,      1.f,
     0.f,      1.f,      4.f,      9.f,     16.f,     25.f,     36.f,     49.f,
    64.f,     81.f,    100.f,    121.f,    144.f,    169.f,    196.f,    225.f,
   256.f,    289.f,    324.f,    361.f,    400.f,    441.f,    484.f,    529.f,
   576.f,    625.f,    676.f,    729.f,    784.f,    841.f,    900.f,    961.f,
  1024.f,   1089.f,   1156.f,   1225.f,   1296.f,   1369.f,   1444.f,   1521.f,
  1600.f,   1681.f,   1764.f,   1849.f,   1936.f,   2025.f,   2116.f,   2209.f,
  2304.f,   2401.f,   2500.f,   2601.f,   2704.f,   2809.f,   2916.f,   3025.f,
  3136.f,   3249.f,   3364.f,   3481.f,   3600.f,   3721.f,   3844.f,   3969.f,
  4096.f,   4225.f,   4356.f,   4489.f,   4624.f,   4761.f,   4900.f,   5041.f,
  5184.f,   5329.f,   5476.f,   5625.f,   5776.f,   5929.f,   6084.f,   6241.f,
  6400.f,   6561.f,   6724.f,   6889.f,   7056.f,   7225.f,   7396.f,   7569.f,
  7744.f,   7921.f,   8100.f,   8281.f,   8464.f,   8649.f,   8836.f,   9025.f,
  9216.f,   9409.f,   9604.f,   9801.f,  10000.f,  10201.f,  10404.f,  10609.f,
 10816.f,  11025.f,  11236.f,  11449.f,  11664.f,  11881.f,  12100.f,  12321.f,
 12544.f,  12769.f,  12996.f,  13225.f,  13456.f,  13689.f,  13924.f,  14161.f,
 14400.f,  14641.f,  14884.f,  15129.f,  15376.f,  15625.f,  15876.f,  16129.f,
 16384.f,  16641.f,  16900.f,  17161.f,  17424.f,  17689.f,  17956.f,  18225.f,
 18496.f,  18769.f,  19044.f,  19321.f,  19600.f,  19881.f,  20164.f,  20449.f,
 20736.f,  21025.f,  21316.f,  21609.f,  21904.f,  22201.f,  22500.f,  22801.f,
 23104.f,  23409.f,  23716.f,  24025.f,  24336.f,  24649.f,  24964.f,  25281.f,
 25600.f,  25921.f,  26244.f,  26569.f,  26896.f,  27225.f,  27556.f,  27889.f,
 28224.f,  28561.f,  28900.f,  29241.f,  29584.f,  29929.f,  30276.f,  30625.f,
 30976.f,  31329.f,  31684.f,  32041.f,  32400.f,  32761.f,  33124.f,  33489.f,
 33856.f,  34225.f,  34596.f,  34969.f,  35344.f,  35721.f,  36100.f,  36481.f,
 36864.f,  37249.f,  37636.f,  38025.f,  38416.f,  38809.f,  39204.f,  39601.f,
 40000.f,  40401.f,  40804.f,  41209.f,  41616.f,  42025.f,  42436.f,  42849.f,
 43264.f,  43681.f,  44100.f,  44521.f,  44944.f,  45369.f,  45796.f,  46225.f,
 46656.f,  47089.f,  47524.f,  47961.f,  48400.f,  48841.f,  49284.f,  49729.f,
 50176.f,  50625.f,  51076.f,  51529.f,  51984.f,  52441.f,  52900.f,  53361.f,
 53824.f,  54289.f,  54756.f,  55225.f,  55696.f,  56169.f,  56644.f,  57121.f,
 57600.f,  58081.f,  58564.f,  59049.f,  59536.f,  60025.f,  60516.f,  61009.f,
 61504.f,  62001.f,  62500.f,  63001.f,  63504.f,  64009.f,  64516.f,  65025.f
};

const uchar icvSaturate8u_cv[] = 
{
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255
};

/* End of file. */

//thresh 
// 
static CvStatus CV_STDCALL
icvThresh_8u_C1R( const uchar* src, int src_step, uchar* dst, int dst_step,
                  CvSize roi, uchar thresh, uchar maxval, int type )
{
    int i, j;
    uchar tab[256];

    switch( type )
    {
    case CV_THRESH_BINARY:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = maxval;
        break;
    case CV_THRESH_BINARY_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = maxval;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    case CV_THRESH_TRUNC:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = thresh;
        break;
    case CV_THRESH_TOZERO:
        for( i = 0; i <= thresh; i++ )
            tab[i] = 0;
        for( ; i < 256; i++ )
            tab[i] = (uchar)i;
        break;
    case CV_THRESH_TOZERO_INV:
        for( i = 0; i <= thresh; i++ )
            tab[i] = (uchar)i;
        for( ; i < 256; i++ )
            tab[i] = 0;
        break;
    default:
        return CV_BADFLAG_ERR;
    }

    for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
    {
        for( j = 0; j <= roi.width - 4; j += 4 )
        {
            uchar t0 = tab[src[j]];
            uchar t1 = tab[src[j+1]];

            dst[j] = t0;
            dst[j+1] = t1;

            t0 = tab[src[j+2]];
            t1 = tab[src[j+3]];

            dst[j+2] = t0;
            dst[j+3] = t1;
        }

        for( ; j < roi.width; j++ )
            dst[j] = tab[src[j]];
    }

    return CV_NO_ERR;
}
// 
// 
static CvStatus CV_STDCALL
icvThresh_32f_C1R( const float *src, int src_step, float *dst, int dst_step,
                   CvSize roi, float thresh, float maxval, int type )
{
    int i, j;
    const int* isrc = (const int*)src;
    int* idst = (int*)dst;
    Cv32suf v;
    int iThresh, iMax;

    v.f = thresh; iThresh = CV_TOGGLE_FLT(v.i);
    v.f = maxval; iMax = v.i;

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    switch( type )
    {
    case CV_THRESH_BINARY:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT(temp) <= iThresh) - 1) & iMax;
            }
        }
        break;

    case CV_THRESH_BINARY_INV:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT(temp) > iThresh) - 1) & iMax;
            }
        }
        break;

    case CV_THRESH_TRUNC:
        for( i = 0; i < roi.height; i++, src += src_step, dst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                float temp = src[j];

                if( temp > thresh )
                    temp = thresh;
                dst[j] = temp;
            }
        }
        break;

    case CV_THRESH_TOZERO:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT( temp ) <= iThresh) - 1) & temp;
            }
        }
        break;

    case CV_THRESH_TOZERO_INV:
        for( i = 0; i < roi.height; i++, isrc += src_step, idst += dst_step )
        {
            for( j = 0; j < roi.width; j++ )
            {
                int temp = isrc[j];
                idst[j] = ((CV_TOGGLE_FLT( temp ) > iThresh) - 1) & temp;
            }
        }
        break;

    default:
        return CV_BADFLAG_ERR;
    }

    return CV_OK;
}
// 
// 
static double
icvGetThreshVal_Otsu( const CvHistogram* hist )
{
    double max_val = 0;
    
    CV_FUNCNAME( "icvGetThreshVal_Otsu" );

    __BEGIN__;

    int i, count;
    const float* h;
    double sum = 0, mu = 0;
    bool uniform = false;
    double low = 0, high = 0, delta = 0;
    float* nu_thresh = 0;
    double mu1 = 0, q1 = 0;
    double max_sigma = 0;

    if( !CV_IS_HIST(hist) || CV_IS_SPARSE_HIST(hist) || hist->mat.dims != 1 )
        CV_ERROR( CV_StsBadArg,
        "The histogram in Otsu method must be a valid dense 1D histogram" );

    count = hist->mat.dim[0].size;
    h = (float*)cvPtr1D( hist->bins, 0 );

    if( !CV_HIST_HAS_RANGES(hist) || CV_IS_UNIFORM_HIST(hist) )
    {
        if( CV_HIST_HAS_RANGES(hist) )
        {
            low = hist->thresh[0][0];
            high = hist->thresh[0][1];
        }
        else
        {
            low = 0;
            high = count;
        }

        delta = (high-low)/count;
        low += delta*0.5;
        uniform = true;
    }
    else
        nu_thresh = hist->thresh2[0];

    for( i = 0; i < count; i++ )
    {
        sum += h[i];
        if( uniform )
            mu += (i*delta + low)*h[i];
        else
            mu += (nu_thresh[i*2] + nu_thresh[i*2+1])*0.5*h[i];
    }
    
    sum = fabs(sum) > FLT_EPSILON ? 1./sum : 0;
    mu *= sum;

    mu1 = 0;
    q1 = 0;

    for( i = 0; i < count; i++ )
    {
        double p_i, q2, mu2, val_i, sigma;

        p_i = h[i]*sum;
        mu1 *= q1;
        q1 += p_i;
        q2 = 1. - q1;

        if( MIN(q1,q2) < FLT_EPSILON || MAX(q1,q2) > 1. - FLT_EPSILON )
            continue;

        if( uniform )
            val_i = i*delta + low;
        else
            val_i = (nu_thresh[i*2] + nu_thresh[i*2+1])*0.5;

        mu1 = (mu1 + val_i*p_i)/q1;
        mu2 = (mu - q1*mu1)/q2;
        sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
        if( sigma > max_sigma )
        {
            max_sigma = sigma;
            max_val = val_i;
        }
    }

    __END__;

    return max_val;
}
// 
// 
icvAndC_8u_C1R_t icvAndC_8u_C1R_p = 0;
icvCompareC_8u_C1R_cv_t icvCompareC_8u_C1R_cv_p = 0;
icvThreshold_GTVal_8u_C1R_t icvThreshold_GTVal_8u_C1R_p = 0;
icvThreshold_GTVal_32f_C1R_t icvThreshold_GTVal_32f_C1R_p = 0;
icvThreshold_LTVal_8u_C1R_t icvThreshold_LTVal_8u_C1R_p = 0;
icvThreshold_LTVal_32f_C1R_t icvThreshold_LTVal_32f_C1R_p = 0;
// 
CV_IMPL void
cvThreshold( const void* srcarr, void* dstarr, double thresh, double maxval, int type )
{
    CvHistogram* hist = 0;
    
    CV_FUNCNAME( "cvThreshold" );

    __BEGIN__;

    CvSize roi;
    int src_step, dst_step;
    CvMat src_stub, *src = (CvMat*)srcarr;
    CvMat dst_stub, *dst = (CvMat*)dstarr;
    CvMat src0, dst0;
    int coi1 = 0, coi2 = 0;
    int ithresh, imaxval, cn;
    bool use_otsu;

    CV_CALL( src = cvGetMat( src, &src_stub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dst_stub, &coi2 ));

    if( coi1 + coi2 )
        CV_ERROR( CV_BadCOI, "COI is not supported by the function" );

    if( !CV_ARE_CNS_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedFormats, "Both arrays must have equal number of channels" );

    cn = CV_MAT_CN(src->type);
    if( cn > 1 )
    {
        src = cvReshape( src, &src0, 1 );
        dst = cvReshape( dst, &dst0, 1 );
    }

    use_otsu = (type & ~CV_THRESH_MASK) == CV_THRESH_OTSU;
    type &= CV_THRESH_MASK;

    if( use_otsu )
    {
        float _ranges[] = { 0, 256 };
        float* ranges = _ranges;
        int hist_size = 256;
        void* srcarr0 = src;

        if( CV_MAT_TYPE(src->type) != CV_8UC1 )
            CV_ERROR( CV_StsNotImplemented, "Otsu method can only be used with 8uC1 images" );

        CV_CALL( hist = cvCreateHist( 1, &hist_size, CV_HIST_ARRAY, &ranges ));
        cvCalcArrHist( &srcarr0, hist );
        thresh = cvFloor(icvGetThreshVal_Otsu( hist ));
    }

    if( !CV_ARE_DEPTHS_EQ( src, dst ) )
    {
        if( CV_MAT_TYPE(dst->type) != CV_8UC1 )
            CV_ERROR( CV_StsUnsupportedFormat, "In case of different types destination should be 8uC1" );

        if( type != CV_THRESH_BINARY && type != CV_THRESH_BINARY_INV )
            CV_ERROR( CV_StsBadArg,
            "In case of different types only CV_THRESH_BINARY "
            "and CV_THRESH_BINARY_INV thresholding types are supported" );

        if( maxval < 0 )
        {
            CV_CALL( cvSetZero( dst ));
        }
        else
        {
            CV_CALL( cvCmpS( src, thresh, dst, type == CV_THRESH_BINARY ? CV_CMP_GT : CV_CMP_LE ));
            if( maxval < 255 )
                CV_CALL( cvAndS( dst, cvScalarAll( maxval ), dst ));
        }
        EXIT;
    }

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    roi = cvGetMatSize( src );
    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        roi.width *= roi.height;
        roi.height = 1;
        src_step = dst_step = CV_STUB_STEP;
    }
    else
    {
        src_step = src->step;
        dst_step = dst->step;
    }

    switch( CV_MAT_DEPTH(src->type) )
    {
    case CV_8U:
        
        ithresh = cvFloor(thresh);
        imaxval = cvRound(maxval);
        if( type == CV_THRESH_TRUNC )
            imaxval = ithresh;
        imaxval = CV_CAST_8U(imaxval);

        if( ithresh < 0 || ithresh >= 255 )
        {
            if( type == CV_THRESH_BINARY || type == CV_THRESH_BINARY_INV ||
                (type == CV_THRESH_TRUNC || type == CV_THRESH_TOZERO_INV) && ithresh < 0 ||
                type == CV_THRESH_TOZERO && ithresh >= 255 )
            {
                int v = type == CV_THRESH_BINARY ? (ithresh >= 255 ? 0 : imaxval) :
                        type == CV_THRESH_BINARY_INV ? (ithresh >= 255 ? imaxval : 0) :
                        type == CV_THRESH_TRUNC ? imaxval : 0;

                cvSet( dst, cvScalarAll(v) );
                EXIT;
            }
            else
            {
                cvCopy( src, dst );
                EXIT;
            }
        }

        if( type == CV_THRESH_BINARY || type == CV_THRESH_BINARY_INV )
        {
            if( icvCompareC_8u_C1R_cv_p && icvAndC_8u_C1R_p )
            {
                IPPI_CALL( icvCompareC_8u_C1R_cv_p( src->data.ptr, src_step,
                    (uchar)ithresh, dst->data.ptr, dst_step, roi,
                    type == CV_THRESH_BINARY ? cvCmpGreater : cvCmpLessEq ));

                if( imaxval < 255 )
                    IPPI_CALL( icvAndC_8u_C1R_p( dst->data.ptr, dst_step,
                    (uchar)imaxval, dst->data.ptr, dst_step, roi ));
                EXIT;
            }
        }
        else if( type == CV_THRESH_TRUNC || type == CV_THRESH_TOZERO_INV )
        {
            if( icvThreshold_GTVal_8u_C1R_p )
            {
                IPPI_CALL( icvThreshold_GTVal_8u_C1R_p( src->data.ptr, src_step,
                    dst->data.ptr, dst_step, roi, (uchar)ithresh,
                    (uchar)(type == CV_THRESH_TRUNC ? ithresh : 0) ));
                EXIT;
            }
        }
        else
        {
            assert( type == CV_THRESH_TOZERO );
            if( icvThreshold_LTVal_8u_C1R_p )
            {
                ithresh = cvFloor(thresh+1.);
                ithresh = CV_CAST_8U(ithresh);
                IPPI_CALL( icvThreshold_LTVal_8u_C1R_p( src->data.ptr, src_step,
                    dst->data.ptr, dst_step, roi, (uchar)ithresh, 0 ));
                EXIT;
            }
        }

        icvThresh_8u_C1R( src->data.ptr, src_step,
                          dst->data.ptr, dst_step, roi,
                          (uchar)ithresh, (uchar)imaxval, type );
        break;
    case CV_32F:

        if( type == CV_THRESH_TRUNC || type == CV_THRESH_TOZERO_INV )
        {
            if( icvThreshold_GTVal_32f_C1R_p )
            {
                IPPI_CALL( icvThreshold_GTVal_32f_C1R_p( src->data.fl, src_step,
                    dst->data.fl, dst_step, roi, (float)thresh,
                    type == CV_THRESH_TRUNC ? (float)thresh : 0 ));
                EXIT;
            }
        }
        else if( type == CV_THRESH_TOZERO )
        {
            if( icvThreshold_LTVal_32f_C1R_p )
            {
                IPPI_CALL( icvThreshold_LTVal_32f_C1R_p( src->data.fl, src_step,
                    dst->data.fl, dst_step, roi, (float)(thresh*(1 + FLT_EPSILON)), 0 ));
                EXIT;
            }
        }

        icvThresh_32f_C1R( src->data.fl, src_step, dst->data.fl, dst_step, roi,
                           (float)thresh, (float)maxval, type );
        break;
    default:
        CV_ERROR( CV_BadDepth, cvUnsupportedFormat );
    }

    __END__;

    if( hist )
        cvReleaseHist( &hist );
}
// 
// /* End of file. */
CV_IMPL CvSeq* cvPointSeqFromMat( int seq_kind, const CvArr* arr,
                                  CvContour* contour_header, CvSeqBlock* block )
{
    CvSeq* contour = 0;

    CV_FUNCNAME( "cvPointSeqFromMat" );

    assert( arr != 0 && contour_header != 0 && block != 0 );

    __BEGIN__;
    
    int eltype;
    CvMat* mat = (CvMat*)arr;
    
    if( !CV_IS_MAT( mat ))
        CV_ERROR( CV_StsBadArg, "Input array is not a valid matrix" ); 

    eltype = CV_MAT_TYPE( mat->type );
    if( eltype != CV_32SC2 && eltype != CV_32FC2 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "The matrix can not be converted to point sequence because of "
        "inappropriate element type" );

    if( mat->width != 1 && mat->height != 1 || !CV_IS_MAT_CONT(mat->type))
        CV_ERROR( CV_StsBadArg,
        "The matrix converted to point sequence must be "
        "1-dimensional and continuous" );

    CV_CALL( cvMakeSeqHeaderForArray(
            (seq_kind & (CV_SEQ_KIND_MASK|CV_SEQ_FLAG_CLOSED)) | eltype,
            sizeof(CvContour), CV_ELEM_SIZE(eltype), mat->data.ptr,
            mat->width*mat->height, (CvSeq*)contour_header, block ));

    contour = (CvSeq*)contour_header;

    __END__;

    return contour;
}

// 
typedef CvStatus (CV_STDCALL * CvCopyNonConstBorderFunc)(
    const void*, int, CvSize, void*, int, CvSize, int, int );

typedef CvStatus (CV_STDCALL * CvCopyNonConstBorderFuncI)(
    const void*, int, CvSize, CvSize, int, int );

icvCopyReplicateBorder_8u_C1R_t icvCopyReplicateBorder_8u_C1R_p = 0;
icvCopyReplicateBorder_16s_C1R_t icvCopyReplicateBorder_16s_C1R_p = 0;
icvCopyReplicateBorder_8u_C3R_t icvCopyReplicateBorder_8u_C3R_p = 0;
icvCopyReplicateBorder_32s_C1R_t icvCopyReplicateBorder_32s_C1R_p = 0;
icvCopyReplicateBorder_16s_C3R_t icvCopyReplicateBorder_16s_C3R_p = 0;
icvCopyReplicateBorder_16s_C4R_t icvCopyReplicateBorder_16s_C4R_p = 0;
icvCopyReplicateBorder_32s_C3R_t icvCopyReplicateBorder_32s_C3R_p = 0;
icvCopyReplicateBorder_32s_C4R_t icvCopyReplicateBorder_32s_C4R_p = 0;

icvCopyReplicateBorder_8u_C1IR_t icvCopyReplicateBorder_8u_C1IR_p = 0;
icvCopyReplicateBorder_16s_C1IR_t icvCopyReplicateBorder_16s_C1IR_p = 0;
icvCopyReplicateBorder_8u_C3IR_t icvCopyReplicateBorder_8u_C3IR_p = 0;
icvCopyReplicateBorder_32s_C1IR_t icvCopyReplicateBorder_32s_C1IR_p = 0;
icvCopyReplicateBorder_16s_C3IR_t icvCopyReplicateBorder_16s_C3IR_p = 0;
icvCopyReplicateBorder_16s_C4IR_t icvCopyReplicateBorder_16s_C4IR_p = 0;
icvCopyReplicateBorder_32s_C3IR_t icvCopyReplicateBorder_32s_C3IR_p = 0;
icvCopyReplicateBorder_32s_C4IR_t icvCopyReplicateBorder_32s_C4IR_p = 0;
// 
// 
CvStatus CV_STDCALL
icvCopyReplicateBorder_8u( const uchar* src, int srcstep, CvSize srcroi,
                           uchar* dst, int dststep, CvSize dstroi,
                           int top, int left, int cn, const uchar* )
{
    const int isz = (int)sizeof(int);
    int i, j;

    if( srcstep == dststep && dst + dststep*top + left*cn == src &&
        icvCopyReplicateBorder_8u_C1IR_p )
    {
        CvCopyNonConstBorderFuncI ifunc =
               cn == 1 ? icvCopyReplicateBorder_8u_C1IR_p :
               cn == 2 ? icvCopyReplicateBorder_16s_C1IR_p :
               cn == 3 ? icvCopyReplicateBorder_8u_C3IR_p :
               cn == 4 ? icvCopyReplicateBorder_32s_C1IR_p :
               cn == 6 ? icvCopyReplicateBorder_16s_C3IR_p :
               cn == 8 ? icvCopyReplicateBorder_16s_C4IR_p :
               cn == 12 ? icvCopyReplicateBorder_32s_C3IR_p :
               cn == 16 ? icvCopyReplicateBorder_32s_C4IR_p : 0;

        if( ifunc )
            return ifunc( src, srcstep, srcroi, dstroi, top, left );
    }
    else if( icvCopyReplicateBorder_8u_C1R_p )
    {
        CvCopyNonConstBorderFunc func =
               cn == 1 ? icvCopyReplicateBorder_8u_C1R_p :
               cn == 2 ? icvCopyReplicateBorder_16s_C1R_p :
               cn == 3 ? icvCopyReplicateBorder_8u_C3R_p :
               cn == 4 ? icvCopyReplicateBorder_32s_C1R_p :
               cn == 6 ? icvCopyReplicateBorder_16s_C3R_p :
               cn == 8 ? icvCopyReplicateBorder_16s_C4R_p :
               cn == 12 ? icvCopyReplicateBorder_32s_C3R_p :
               cn == 16 ? icvCopyReplicateBorder_32s_C4R_p : 0;

        if( func )
            return func( src, srcstep, srcroi, dst, dststep, dstroi, top, left );
    }

    if( (cn | srcstep | dststep | (size_t)src | (size_t)dst) % isz == 0 )
    {
        const int* isrc = (const int*)src;
        int* idst = (int*)dst;
        
        cn /= isz;
        srcstep /= isz;
        dststep /= isz;

        srcroi.width *= cn;
        dstroi.width *= cn;
        left *= cn;

        for( i = 0; i < dstroi.height; i++, idst += dststep )
        {
            if( idst + left != isrc )
                for( j = 0; j < srcroi.width; j++ )
                    idst[j + left] = isrc[j];
            for( j = left - 1; j >= 0; j-- )
                idst[j] = idst[j + cn];
            for( j = left+srcroi.width; j < dstroi.width; j++ )
                idst[j] = idst[j - cn];
            if( i >= top && i < top + srcroi.height - 1 )
                isrc += srcstep;
        }
    }
    else
    {
        srcroi.width *= cn;
        dstroi.width *= cn;
        left *= cn;

        for( i = 0; i < dstroi.height; i++, dst += dststep )
        {
            if( dst + left != src )
                for( j = 0; j < srcroi.width; j++ )
                    dst[j + left] = src[j];
            for( j = left - 1; j >= 0; j-- )
                dst[j] = dst[j + cn];
            for( j = left+srcroi.width; j < dstroi.width; j++ )
                dst[j] = dst[j - cn];
            if( i >= top && i < top + srcroi.height - 1 )
                src += srcstep;
        }
    }

    return CV_OK;
}
// 
// 
static CvStatus CV_STDCALL
icvCopyReflect101Border_8u( const uchar* src, int srcstep, CvSize srcroi,
                            uchar* dst, int dststep, CvSize dstroi,
                            int top, int left, int cn )
{
    const int isz = (int)sizeof(int);
    int i, j, k, t, dj, tab_size, int_mode = 0;
    const int* isrc = (const int*)src;
    int* idst = (int*)dst, *tab;

    if( (cn | srcstep | dststep | (size_t)src | (size_t)dst) % isz == 0 )
    {
        cn /= isz;
        srcstep /= isz;
        dststep /= isz;

        int_mode = 1;
    }

    srcroi.width *= cn;
    dstroi.width *= cn;
    left *= cn;

    tab_size = dstroi.width - srcroi.width;
    tab = (int*)cvStackAlloc( tab_size*sizeof(tab[0]) );

    if( srcroi.width == 1 )
    {
        for( k = 0; k < cn; k++ )
            for( i = 0; i < tab_size; i += cn )
                tab[i + k] = k + left;
    }
    else
    {
        j = dj = cn;
        for( i = left - cn; i >= 0; i -= cn )
        {
            for( k = 0; k < cn; k++ )
                tab[i + k] = j + k + left;
            if( (unsigned)(j += dj) >= (unsigned)srcroi.width )
                j -= 2*dj, dj = -dj;
        }
        
        j = srcroi.width - cn*2;
        dj = -cn;
        for( i = left; i < tab_size; j += cn )
        {
            for( k = 0; k < cn; k++ )
                tab[i + k] = j + k + left;
            if( (unsigned)(j += dj) >= (unsigned)srcroi.width )
                j -= 2*dj, dj = -dj;
        }
    }

    if( int_mode )
    {
        idst += top*dststep;
        for( i = 0; i < srcroi.height; i++, isrc += srcstep, idst += dststep )
        {
            if( idst + left != isrc )
                for( j = 0; j < srcroi.width; j++ )
                    idst[j + left] = isrc[j];
            for( j = 0; i < left; j++ )
            {
                k = tab[j]; 
                idst[j] = idst[k];
            }
            for( ; j < tab_size; j++ )
            {
                k = tab[j];
                idst[j + srcroi.width] = idst[k];
            }
        }
        isrc -= srcroi.height*srcstep;
        idst -= (top - srcroi.height)*dststep;
    }
    else
    {
        dst += top*dststep;
        for( i = 0; i < srcroi.height; i++, src += srcstep, dst += dststep )
        {
            if( dst + left != src )
                for( j = 0; j < srcroi.width; j++ )
                    dst[j + left] = src[j];
            for( j = 0; i < left; j++ )
            {
                k = tab[j]; 
                dst[j] = dst[k];
            }
            for( ; j < tab_size; j++ )
            {
                k = tab[j];
                dst[j + srcroi.width] = dst[k];
            }
        }
        src -= srcroi.height*srcstep;
        dst -= (top - srcroi.height)*dststep;
    }

    for( t = 0; t < 2; t++ )
    {
        int i1, i2, di;
        if( t == 0 )
            i1 = top-1, i2 = 0, di = -1, j = 1, dj = 1;
        else
            i1 = top+srcroi.height, i2=dstroi.height, di = 1, j = srcroi.height-2, dj = -1;
        
        for( i = i1; i != i2; i += di )
        {
            if( int_mode )
            {
                const int* s = idst + i*dststep;
                int* d = idst + (j+top)*dststep;
                for( k = 0; k < dstroi.width; k++ )
                    d[k] = s[k];
            }
            else
            {
                const uchar* s = dst + i*dststep;
                uchar* d = dst + (j+top)*dststep;
                for( k = 0; k < dstroi.width; k++ )
                    d[k] = s[k];
            }

            if( (unsigned)(j += dj) >= (unsigned)srcroi.height )
                j -= 2*dj, dj = -dj;
        }
    }

    return CV_OK;
}
// 
// 
static CvStatus CV_STDCALL
icvCopyConstBorder_8u( const uchar* src, int srcstep, CvSize srcroi,
                       uchar* dst, int dststep, CvSize dstroi,
                       int top, int left, int cn, const uchar* value )
{
    const int isz = (int)sizeof(int);
    int i, j, k;
    if( (cn | srcstep | dststep | (size_t)src | (size_t)dst | (size_t)value) % isz == 0 )
    {
        const int* isrc = (const int*)src;
        int* idst = (int*)dst;
        const int* ivalue = (const int*)value;
        int v0 = ivalue[0];
        
        cn /= isz;
        srcstep /= isz;
        dststep /= isz;

        srcroi.width *= cn;
        dstroi.width *= cn;
        left *= cn;

        for( j = 1; j < cn; j++ )
            if( ivalue[j] != ivalue[0] )
                break;

        if( j == cn )
            cn = 1;

        if( dstroi.width <= 0 )
            return CV_OK;

        for( i = 0; i < dstroi.height; i++, idst += dststep )
        {
            if( i < top || i >= top + srcroi.height )
            {
                if( cn == 1 )
                {
                    for( j = 0; j < dstroi.width; j++ )
                        idst[j] = v0;
                }
                else
                {
                    for( j = 0; j < cn; j++ )
                        idst[j] = ivalue[j];
                    for( ; j < dstroi.width; j++ )
                        idst[j] = idst[j - cn];
                }
                continue;
            }

            if( cn == 1 )
            {
                for( j = 0; j < left; j++ )
                    idst[j] = v0;
                for( j = srcroi.width + left; j < dstroi.width; j++ )
                    idst[j] = v0;
            }
            else
            {
                for( k = 0; k < cn; k++ )
                {
                    for( j = 0; j < left; j += cn )
                        idst[j+k] = ivalue[k];
                    for( j = srcroi.width + left; j < dstroi.width; j += cn )
                        idst[j+k] = ivalue[k];
                }
            }

            if( idst + left != isrc )
                for( j = 0; j < srcroi.width; j++ )
                    idst[j + left] = isrc[j];
            isrc += srcstep;
        }
    }
    else
    {
        uchar v0 = value[0];
        
        srcroi.width *= cn;
        dstroi.width *= cn;
        left *= cn;

        for( j = 1; j < cn; j++ )
            if( value[j] != value[0] )
                break;

        if( j == cn )
            cn = 1;

        if( dstroi.width <= 0 )
            return CV_OK;

        for( i = 0; i < dstroi.height; i++, dst += dststep )
        {
            if( i < top || i >= top + srcroi.height )
            {
                if( cn == 1 )
                {
                    for( j = 0; j < dstroi.width; j++ )
                        dst[j] = v0;
                }
                else
                {
                    for( j = 0; j < cn; j++ )
                        dst[j] = value[j];
                    for( ; j < dstroi.width; j++ )
                        dst[j] = dst[j - cn];
                }
                continue;
            }

            if( cn == 1 )
            {
                for( j = 0; j < left; j++ )
                    dst[j] = v0;
                for( j = srcroi.width + left; j < dstroi.width; j++ )
                    dst[j] = v0;
            }
            else
            {
                for( k = 0; k < cn; k++ )
                {
                    for( j = 0; j < left; j += cn )
                        dst[j+k] = value[k];
                    for( j = srcroi.width + left; j < dstroi.width; j += cn )
                        dst[j+k] = value[k];
                }
            }

            if( dst + left != src )
                for( j = 0; j < srcroi.width; j++ )
                    dst[j + left] = src[j];
            src += srcstep;
        }
    }

    return CV_OK;
}
// 
// 
CV_IMPL void
cvCopyMakeBorder( const CvArr* srcarr, CvArr* dstarr, CvPoint offset,
                  int bordertype, CvScalar value )
{
    CV_FUNCNAME( "cvCopyMakeBorder" );

    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize srcsize, dstsize;
    int srcstep, dststep;
    int pix_size, type;

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub ));
    
    if( !CV_IS_MAT(dst) )    
        CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( offset.x < 0 || offset.y < 0 )
        CV_ERROR( CV_StsOutOfRange, "Offset (left/top border width) is negative" );

    if( src->rows + offset.y > dst->rows || src->cols + offset.x > dst->cols )
        CV_ERROR( CV_StsBadSize, "Source array is too big or destination array is too small" );

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    type = CV_MAT_TYPE(src->type);
    pix_size = CV_ELEM_SIZE(type);
    srcsize = cvGetMatSize(src);
    dstsize = cvGetMatSize(dst);
    srcstep = src->step;
    dststep = dst->step;
    if( srcstep == 0 )
        srcstep = CV_STUB_STEP;
    if( dststep == 0 )
        dststep = CV_STUB_STEP;

    if( bordertype == IPL_BORDER_REPLICATE )
    {
        icvCopyReplicateBorder_8u( src->data.ptr, srcstep, srcsize,
                                   dst->data.ptr, dststep, dstsize,
                                   offset.y, offset.x, pix_size );
    }
    else if( bordertype == IPL_BORDER_REFLECT_101 )
    {
        icvCopyReflect101Border_8u( src->data.ptr, srcstep, srcsize,
                                   dst->data.ptr, dststep, dstsize,
                                   offset.y, offset.x, pix_size );
    }
    else if( bordertype == IPL_BORDER_CONSTANT )
    {
        double buf[4];
        cvScalarToRawData( &value, buf, src->type, 0 );
        icvCopyConstBorder_8u( src->data.ptr, srcstep, srcsize,
                               dst->data.ptr, dststep, dstsize,
                               offset.y, offset.x, pix_size, (uchar*)buf );
    }
    else
        CV_ERROR( CV_StsBadFlag, "Unknown/unsupported border type" );
    
    __END__;
}

void
icvCrossCorr( const CvArr* _img, const CvArr* _templ, CvArr* _corr, CvPoint anchor )
{
    const double block_scale = 4.5;
    const int min_block_size = 256;
    CvMat* dft_img = 0;
    CvMat* dft_templ = 0;
    void* buf = 0;
    
    CV_FUNCNAME( "icvCrossCorr" );
    
    __BEGIN__;

    CvMat istub, *img = (CvMat*)_img;
    CvMat tstub, *templ = (CvMat*)_templ;
    CvMat cstub, *corr = (CvMat*)_corr;
    CvMat sstub, dstub, *src, *dst, temp;
    CvSize dftsize, blocksize;
    CvMat* planes[] = { 0, 0, 0, 0 };
    int x, y, i, yofs, buf_size = 0;
    int depth, templ_depth, corr_depth, max_depth = CV_32F, cn, templ_cn, corr_cn;

    CV_CALL( img = cvGetMat( img, &istub ));
    CV_CALL( templ = cvGetMat( templ, &tstub ));
    CV_CALL( corr = cvGetMat( corr, &cstub ));

    if( CV_MAT_DEPTH( img->type ) != CV_8U &&
        CV_MAT_DEPTH( img->type ) != CV_16U &&
        CV_MAT_DEPTH( img->type ) != CV_32F )
        CV_ERROR( CV_StsUnsupportedFormat,
        "The function supports only 8u, 16u and 32f data types" );

    if( !CV_ARE_DEPTHS_EQ( img, templ ) && CV_MAT_DEPTH( templ->type ) != CV_32F )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Template (kernel) must be of the same depth as the input image, or be 32f" );
    
    if( !CV_ARE_DEPTHS_EQ( img, corr ) && CV_MAT_DEPTH( corr->type ) != CV_32F &&
        CV_MAT_DEPTH( corr->type ) != CV_64F )
        CV_ERROR( CV_StsUnsupportedFormat,
        "The output image must have the same depth as the input image, or be 32f/64f" );

    if( (!CV_ARE_CNS_EQ( img, corr ) || CV_MAT_CN(templ->type) > 1) &&
        (CV_MAT_CN( corr->type ) > 1 || !CV_ARE_CNS_EQ( img, templ)) )
        CV_ERROR( CV_StsUnsupportedFormat,
        "The output must have the same number of channels as the input (when the template has 1 channel), "
        "or the output must have 1 channel when the input and the template have the same number of channels" );

    depth = CV_MAT_DEPTH(img->type);
    cn = CV_MAT_CN(img->type);
    templ_depth = CV_MAT_DEPTH(templ->type);
    templ_cn = CV_MAT_CN(templ->type);
    corr_depth = CV_MAT_DEPTH(corr->type);
    corr_cn = CV_MAT_CN(corr->type);
    max_depth = MAX( max_depth, templ_depth );
    max_depth = MAX( max_depth, depth );
    max_depth = MAX( max_depth, corr_depth );
    if( depth > CV_8U )
        max_depth = CV_64F;

    if( img->cols < templ->cols || img->rows < templ->rows )
        CV_ERROR( CV_StsUnmatchedSizes,
        "Such a combination of image and template/filter size is not supported" );

    if( corr->rows > img->rows + templ->rows - 1 ||
        corr->cols > img->cols + templ->cols - 1 )
        CV_ERROR( CV_StsUnmatchedSizes,
        "output image should not be greater than (W + w - 1)x(H + h - 1)" );

    blocksize.width = cvRound(templ->cols*block_scale);
    blocksize.width = MAX( blocksize.width, min_block_size - templ->cols + 1 );
    blocksize.width = MIN( blocksize.width, corr->cols );
    blocksize.height = cvRound(templ->rows*block_scale);
    blocksize.height = MAX( blocksize.height, min_block_size - templ->rows + 1 );
    blocksize.height = MIN( blocksize.height, corr->rows );

    dftsize.width = cvGetOptimalDFTSize(blocksize.width + templ->cols - 1);
    if( dftsize.width == 1 )
        dftsize.width = 2;
    dftsize.height = cvGetOptimalDFTSize(blocksize.height + templ->rows - 1);
    if( dftsize.width <= 0 || dftsize.height <= 0 )
        CV_ERROR( CV_StsOutOfRange, "the input arrays are too big" );

    // recompute block size
    blocksize.width = dftsize.width - templ->cols + 1;
    blocksize.width = MIN( blocksize.width, corr->cols );
    blocksize.height = dftsize.height - templ->rows + 1;
    blocksize.height = MIN( blocksize.height, corr->rows );

    CV_CALL( dft_img = cvCreateMat( dftsize.height, dftsize.width, max_depth ));
    CV_CALL( dft_templ = cvCreateMat( dftsize.height*templ_cn, dftsize.width, max_depth ));

    if( templ_cn > 1 && templ_depth != max_depth )
        buf_size = templ->cols*templ->rows*CV_ELEM_SIZE(templ_depth);

    if( cn > 1 && depth != max_depth )
        buf_size = MAX( buf_size, (blocksize.width + templ->cols - 1)*
            (blocksize.height + templ->rows - 1)*CV_ELEM_SIZE(depth));

    if( (corr_cn > 1 || cn > 1) && corr_depth != max_depth )
        buf_size = MAX( buf_size, blocksize.width*blocksize.height*CV_ELEM_SIZE(corr_depth));

    if( buf_size > 0 )
        CV_CALL( buf = cvAlloc(buf_size) );

    // compute DFT of each template plane
    for( i = 0; i < templ_cn; i++ )
    {
        yofs = i*dftsize.height;

        src = templ;
        dst = cvGetSubRect( dft_templ, &dstub, cvRect(0,yofs,templ->cols,templ->rows));
    
        if( templ_cn > 1 )
        {
            planes[i] = templ_depth == max_depth ? dst :
                cvInitMatHeader( &temp, templ->rows, templ->cols, templ_depth, buf );
            cvSplit( templ, planes[0], planes[1], planes[2], planes[3] );
            src = planes[i];
            planes[i] = 0;
        }

        if( dst != src )
            cvConvert( src, dst );

        if( dft_templ->cols > templ->cols )
        {
            cvGetSubRect( dft_templ, dst, cvRect(templ->cols, yofs,
                          dft_templ->cols - templ->cols, templ->rows) );
            cvZero( dst );
        }
        cvGetSubRect( dft_templ, dst, cvRect(0,yofs,dftsize.width,dftsize.height) );
        cvDFT( dst, dst, CV_DXT_FORWARD + CV_DXT_SCALE, templ->rows );
    }

    // calculate correlation by blocks
    for( y = 0; y < corr->rows; y += blocksize.height )
    {
        for( x = 0; x < corr->cols; x += blocksize.width )
        {
            CvSize csz = { blocksize.width, blocksize.height }, isz;
            int x0 = x - anchor.x, y0 = y - anchor.y;
            int x1 = MAX( 0, x0 ), y1 = MAX( 0, y0 ), x2, y2;
            csz.width = MIN( csz.width, corr->cols - x );
            csz.height = MIN( csz.height, corr->rows - y );
            isz.width = csz.width + templ->cols - 1;
            isz.height = csz.height + templ->rows - 1;
            x2 = MIN( img->cols, x0 + isz.width );
            y2 = MIN( img->rows, y0 + isz.height );
            
            for( i = 0; i < cn; i++ )
            {
                CvMat dstub1, *dst1;
                yofs = i*dftsize.height;

                src = cvGetSubRect( img, &sstub, cvRect(x1,y1,x2-x1,y2-y1) );
                dst = cvGetSubRect( dft_img, &dstub, cvRect(0,0,isz.width,isz.height) );
                dst1 = dst;
                
                if( x2 - x1 < isz.width || y2 - y1 < isz.height )
                    dst1 = cvGetSubRect( dft_img, &dstub1,
                        cvRect( x1 - x0, y1 - y0, x2 - x1, y2 - y1 ));

                if( cn > 1 )
                {
                    planes[i] = dst1;
                    if( depth != max_depth )
                        planes[i] = cvInitMatHeader( &temp, y2 - y1, x2 - x1, depth, buf );
                    cvSplit( src, planes[0], planes[1], planes[2], planes[3] );
                    src = planes[i];
                    planes[i] = 0;
                }

                if( dst1 != src )
                    cvConvert( src, dst1 );

                if( dst != dst1 )
                    cvCopyMakeBorder( dst1, dst, cvPoint(x1 - x0, y1 - y0), IPL_BORDER_REPLICATE );

                if( dftsize.width > isz.width )
                {
                    cvGetSubRect( dft_img, dst, cvRect(isz.width, 0,
                          dftsize.width - isz.width,dftsize.height) );
                    cvZero( dst );
                }

                cvDFT( dft_img, dft_img, CV_DXT_FORWARD, isz.height );
                cvGetSubRect( dft_templ, dst,
                    cvRect(0,(templ_cn>1?yofs:0),dftsize.width,dftsize.height) );

                cvMulSpectrums( dft_img, dst, dft_img, CV_DXT_MUL_CONJ );
                cvDFT( dft_img, dft_img, CV_DXT_INVERSE, csz.height );

                src = cvGetSubRect( dft_img, &sstub, cvRect(0,0,csz.width,csz.height) );
                dst = cvGetSubRect( corr, &dstub, cvRect(x,y,csz.width,csz.height) );

                if( corr_cn > 1 )
                {
                    planes[i] = src;
                    if( corr_depth != max_depth )
                    {
                        planes[i] = cvInitMatHeader( &temp, csz.height, csz.width, corr_depth, buf );
                        cvConvert( src, planes[i] );
                    }
                    cvMerge( planes[0], planes[1], planes[2], planes[3], dst );
                    planes[i] = 0;                    
                }
                else
                {
                    if( i == 0 )
                        cvConvert( src, dst );
                    else
                    {
                        if( max_depth > corr_depth )
                        {
                            cvInitMatHeader( &temp, csz.height, csz.width, corr_depth, buf );
                            cvConvert( src, &temp );
                            src = &temp;
                        }
                        cvAcc( src, dst );
                    }
                }
            }
        }
    }

    __END__;

    cvReleaseMat( &dft_img );
    cvReleaseMat( &dft_templ );
    cvFree( &buf );
}


// 
#define IPCV_MORPHOLOGY_PTRS( morphtype, flavor )               \
    icv##morphtype##Rect_##flavor##_C1R_t                       \
        icv##morphtype##Rect_##flavor##_C1R_p = 0;              \
    icv##morphtype##Rect_GetBufSize_##flavor##_C1R_t            \
        icv##morphtype##Rect_GetBufSize_##flavor##_C1R_p = 0;   \
    icv##morphtype##Rect_##flavor##_C3R_t                       \
        icv##morphtype##Rect_##flavor##_C3R_p = 0;              \
    icv##morphtype##Rect_GetBufSize_##flavor##_C3R_t            \
        icv##morphtype##Rect_GetBufSize_##flavor##_C3R_p = 0;   \
    icv##morphtype##Rect_##flavor##_C4R_t                       \
        icv##morphtype##Rect_##flavor##_C4R_p = 0;              \
    icv##morphtype##Rect_GetBufSize_##flavor##_C4R_t            \
        icv##morphtype##Rect_GetBufSize_##flavor##_C4R_p = 0;   \
                                                                \
    icv##morphtype##_##flavor##_C1R_t                           \
        icv##morphtype##_##flavor##_C1R_p = 0;                  \
    icv##morphtype##_##flavor##_C3R_t                           \
        icv##morphtype##_##flavor##_C3R_p = 0;                  \
    icv##morphtype##_##flavor##_C4R_t                           \
        icv##morphtype##_##flavor##_C4R_p = 0;

#define IPCV_MORPHOLOGY_INITALLOC_PTRS( flavor )                \
    icvMorphInitAlloc_##flavor##_C1R_t                          \
        icvMorphInitAlloc_##flavor##_C1R_p = 0;                 \
    icvMorphInitAlloc_##flavor##_C3R_t                          \
        icvMorphInitAlloc_##flavor##_C3R_p = 0;                 \
    icvMorphInitAlloc_##flavor##_C4R_t                          \
        icvMorphInitAlloc_##flavor##_C4R_p = 0;

IPCV_MORPHOLOGY_PTRS( Erode, 8u )
IPCV_MORPHOLOGY_PTRS( Erode, 32f )
IPCV_MORPHOLOGY_PTRS( Dilate, 8u )
IPCV_MORPHOLOGY_PTRS( Dilate, 32f )
IPCV_MORPHOLOGY_INITALLOC_PTRS( 8u )
IPCV_MORPHOLOGY_INITALLOC_PTRS( 32f )

 icvMorphFree_t icvMorphFree_p = 0;
// 
// /****************************************************************************************\
//                      Basic Morphological Operations: Erosion & Dilation
// \****************************************************************************************/
// 
static void icvErodeRectRow_8u( const uchar* src, uchar* dst, void* params );
static void icvErodeRectRow_32f( const int* src, int* dst, void* params );
static void icvDilateRectRow_8u( const uchar* src, uchar* dst, void* params );
static void icvDilateRectRow_32f( const int* src, int* dst, void* params );

static void icvErodeRectCol_8u( const uchar** src, uchar* dst, int dst_step,
                                int count, void* params );
static void icvErodeRectCol_32f( const int** src, int* dst, int dst_step,
                                 int count, void* params );
static void icvDilateRectCol_8u( const uchar** src, uchar* dst, int dst_step,
                                 int count, void* params );
static void icvDilateRectCol_32f( const int** src, int* dst, int dst_step,
                                  int count, void* params );

static void icvErodeAny_8u( const uchar** src, uchar* dst, int dst_step,
                            int count, void* params );
static void icvErodeAny_32f( const int** src, int* dst, int dst_step,
                             int count, void* params );
static void icvDilateAny_8u( const uchar** src, uchar* dst, int dst_step,
                             int count, void* params );
static void icvDilateAny_32f( const int** src, int* dst, int dst_step,
                              int count, void* params );
// 
CvMorphology::CvMorphology()
{
    element = 0;
    el_sparse = 0;
}

CvMorphology::CvMorphology( int _operation, int _max_width, int _src_dst_type,
                            int _element_shape, CvMat* _element,
                            CvSize _ksize, CvPoint _anchor,
                            int _border_mode, CvScalar _border_value )
{
    element = 0;
    el_sparse = 0;
    init( _operation, _max_width, _src_dst_type,
          _element_shape, _element, _ksize, _anchor,
          _border_mode, _border_value );
}


void CvMorphology::clear()
{
    cvReleaseMat( &element );
    cvFree( &el_sparse );
    CvBaseImageFilter::clear();
}


CvMorphology::~CvMorphology()
{
    clear();
}
// 
// 
void CvMorphology::init( int _operation, int _max_width, int _src_dst_type,
                         int _element_shape, CvMat* _element,
                         CvSize _ksize, CvPoint _anchor,
                         int _border_mode, CvScalar _border_value )
{
    CV_FUNCNAME( "CvMorphology::init" );

    __BEGIN__;

    int depth = CV_MAT_DEPTH(_src_dst_type);
    int el_type = 0, nz = -1;
    
    if( _operation != ERODE && _operation != DILATE )
        CV_ERROR( CV_StsBadArg, "Unknown/unsupported morphological operation" );

    if( _element_shape == CUSTOM )
    {
        if( !CV_IS_MAT(_element) )
            CV_ERROR( CV_StsBadArg,
            "structuring element should be valid matrix if CUSTOM element shape is specified" );

        el_type = CV_MAT_TYPE(_element->type);
        if( el_type != CV_8UC1 && el_type != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat, "the structuring element must have 8uC1 or 32sC1 type" );

        _ksize = cvGetMatSize(_element);
        CV_CALL( nz = cvCountNonZero(_element));
        if( nz == _ksize.width*_ksize.height )
            _element_shape = RECT;
    }

    operation = _operation;
    el_shape = _element_shape;

    CV_CALL( CvBaseImageFilter::init( _max_width, _src_dst_type, _src_dst_type,
        _element_shape == RECT, _ksize, _anchor, _border_mode, _border_value ));

    if( el_shape == RECT )
    {
        if( operation == ERODE )
        {
            if( depth == CV_8U )
                x_func = (CvRowFilterFunc)icvErodeRectRow_8u,
                y_func = (CvColumnFilterFunc)icvErodeRectCol_8u;
            else if( depth == CV_32F )
                x_func = (CvRowFilterFunc)icvErodeRectRow_32f,
                y_func = (CvColumnFilterFunc)icvErodeRectCol_32f;
        }
        else
        {
            assert( operation == DILATE );
            if( depth == CV_8U )
                x_func = (CvRowFilterFunc)icvDilateRectRow_8u,
                y_func = (CvColumnFilterFunc)icvDilateRectCol_8u;
            else if( depth == CV_32F )
                x_func = (CvRowFilterFunc)icvDilateRectRow_32f,
                y_func = (CvColumnFilterFunc)icvDilateRectCol_32f;
        }
    }
    else
    {
        int i, j, k = 0;
        int cn = CV_MAT_CN(src_type);
        CvPoint* nz_loc;

        if( !(element && el_sparse &&
            _ksize.width == element->cols && _ksize.height == element->rows) )
        {
            cvReleaseMat( &element );
            cvFree( &el_sparse );
            CV_CALL( element = cvCreateMat( _ksize.height, _ksize.width, CV_8UC1 ));
            CV_CALL( el_sparse = (uchar*)cvAlloc(
                ksize.width*ksize.height*(2*sizeof(int) + sizeof(uchar*))));
        }

        if( el_shape == CUSTOM )
        {
            CV_CALL( cvConvert( _element, element ));
        }
        else
        {
            CV_CALL( init_binary_element( element, el_shape, anchor ));
        }

        if( operation == ERODE )
        {
            if( depth == CV_8U )
                y_func = (CvColumnFilterFunc)icvErodeAny_8u;
            else if( depth == CV_32F )
                y_func = (CvColumnFilterFunc)icvErodeAny_32f;
        }
        else
        {
            assert( operation == DILATE );
            if( depth == CV_8U )
                y_func = (CvColumnFilterFunc)icvDilateAny_8u;
            else if( depth == CV_32F )
                y_func = (CvColumnFilterFunc)icvDilateAny_32f;
        }
        
        nz_loc = (CvPoint*)el_sparse;

        for( i = 0; i < ksize.height; i++ )
            for( j = 0; j < ksize.width; j++ )
            {
                if( element->data.ptr[i*element->step+j] )
                    nz_loc[k++] = cvPoint(j*cn,i);
            }
        if( k == 0 )
            nz_loc[k++] = cvPoint(anchor.x*cn,anchor.y);
        el_sparse_count = k;
    }

    if( depth == CV_32F && border_mode == IPL_BORDER_CONSTANT )
    {
        int i, cn = CV_MAT_CN(src_type);
        int* bt = (int*)border_tab;
        for( i = 0; i < cn; i++ )
            bt[i] = CV_TOGGLE_FLT(bt[i]);
    }

    __END__;
}
// 
// 
void CvMorphology::init( int _max_width, int _src_type, int _dst_type,
                         bool _is_separable, CvSize _ksize,
                         CvPoint _anchor, int _border_mode,
                         CvScalar _border_value )
{
    CvBaseImageFilter::init( _max_width, _src_type, _dst_type, _is_separable,
                             _ksize, _anchor, _border_mode, _border_value );
}
// 

void CvMorphology::start_process( CvSlice x_range, int width )
{
    CvBaseImageFilter::start_process( x_range, width );
    if( el_shape == RECT )
    {
        // cut the cyclic buffer off by 1 line if need, to make
        // the vertical part of separable morphological filter
        // always process 2 rows at once (except, may be,
        // for the last one in a stripe).
        int t = buf_max_count - max_ky*2;
        if( t > 1 && t % 2 != 0 )
        {
            buf_max_count--;
            buf_end -= buf_step;
        }
    }
}
// 
// 
int CvMorphology::fill_cyclic_buffer( const uchar* src, int src_step,
                                      int y0, int y1, int y2 )
{
    int i, y = y0, bsz1 = border_tab_sz1, bsz = border_tab_sz;
    int pix_size = CV_ELEM_SIZE(src_type);
    int width_n = (prev_x_range.end_index - prev_x_range.start_index)*pix_size;

    if( CV_MAT_DEPTH(src_type) != CV_32F )
        return CvBaseImageFilter::fill_cyclic_buffer( src, src_step, y0, y1, y2 );

    // fill the cyclic buffer
    for( ; buf_count < buf_max_count && y < y2; buf_count++, y++, src += src_step )
    {
        uchar* trow = is_separable ? buf_end : buf_tail;

        for( i = 0; i < width_n; i += sizeof(int) )
        {
            int t = *(int*)(src + i);
            *(int*)(trow + i + bsz1) = CV_TOGGLE_FLT(t);
        }

        if( border_mode != IPL_BORDER_CONSTANT )
        {
            for( i = 0; i < bsz1; i++ )
            {
                int j = border_tab[i];
                trow[i] = trow[j];
            }
            for( ; i < bsz; i++ )
            {
                int j = border_tab[i];
                trow[i + width_n] = trow[j];
            }
        }
        else
        {
            const uchar *bt = (uchar*)border_tab; 
            for( i = 0; i < bsz1; i++ )
                trow[i] = bt[i];

            for( ; i < bsz; i++ )
                trow[i + width_n] = bt[i];
        }

        if( is_separable )
            x_func( trow, buf_tail, this );

        buf_tail += buf_step;
        if( buf_tail >= buf_end )
            buf_tail = buf_start;
    }

    return y - y0;
}
// 
// 
void CvMorphology::init_binary_element( CvMat* element, int element_shape, CvPoint anchor )
{
    CV_FUNCNAME( "CvMorphology::init_binary_element" );

    __BEGIN__;

    int type;
    int i, j, cols, rows;
    int r = 0, c = 0;
    double inv_r2 = 0;

    if( !CV_IS_MAT(element) )
        CV_ERROR( CV_StsBadArg, "element must be valid matrix" );

    type = CV_MAT_TYPE(element->type);
    if( type != CV_8UC1 && type != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "element must have 8uC1 or 32sC1 type" );

    if( anchor.x == -1 )
        anchor.x = element->cols/2;

    if( anchor.y == -1 )
        anchor.y = element->rows/2;

    if( (unsigned)anchor.x >= (unsigned)element->cols ||
        (unsigned)anchor.y >= (unsigned)element->rows )
        CV_ERROR( CV_StsOutOfRange, "anchor is outside of element" );

    if( element_shape != RECT && element_shape != CROSS && element_shape != ELLIPSE )
        CV_ERROR( CV_StsBadArg, "Unknown/unsupported element shape" );

    rows = element->rows;
    cols = element->cols;

    if( rows == 1 || cols == 1 )
        element_shape = RECT;

    if( element_shape == ELLIPSE )
    {
        r = rows/2;
        c = cols/2;
        inv_r2 = r ? 1./((double)r*r) : 0;
    }

    for( i = 0; i < rows; i++ )
    {
        uchar* ptr = element->data.ptr + i*element->step;
        int j1 = 0, j2 = 0, jx, t = 0;

        if( element_shape == RECT || element_shape == CROSS && i == anchor.y )
            j2 = cols;
        else if( element_shape == CROSS )
            j1 = anchor.x, j2 = j1 + 1;
        else
        {
            int dy = i - r;
            if( abs(dy) <= r )
            {
                int dx = cvRound(c*sqrt(((double)r*r - dy*dy)*inv_r2));
                j1 = MAX( c - dx, 0 );
                j2 = MIN( c + dx + 1, cols );
            }
        }

        for( j = 0, jx = j1; j < cols; )
        {
            for( ; j < jx; j++ )
            {
                if( type == CV_8UC1 )
                    ptr[j] = (uchar)t;
                else
                    ((int*)ptr)[j] = t;
            }
            if( jx == j2 )
                jx = cols, t = 0;
            else
                jx = j2, t = 1;
        }
    }

    __END__;
}
// 

#define ICV_MORPH_RECT_ROW( name, flavor, arrtype,          \
                            worktype, update_extr_macro )   \
static void                                                 \
icv##name##RectRow_##flavor( const arrtype* src,            \
                             arrtype* dst, void* params )   \
{                                                           \
    const CvMorphology* state = (const CvMorphology*)params;\
    int ksize = state->get_kernel_size().width;             \
    int width = state->get_width();                         \
    int cn = CV_MAT_CN(state->get_src_type());              \
    int i, j, k;                                            \
                                                            \
    width *= cn; ksize *= cn;                               \
                                                            \
    if( ksize == cn )                                       \
    {                                                       \
        for( i = 0; i < width; i++ )                        \
            dst[i] = src[i];                                \
        return;                                             \
    }                                                       \
                                                            \
    for( k = 0; k < cn; k++, src++, dst++ )                 \
    {                                                       \
        for( i = 0; i <= width - cn*2; i += cn*2 )          \
        {                                                   \
            const arrtype* s = src + i;                     \
            worktype m = s[cn], t;                          \
            for( j = cn*2; j < ksize; j += cn )             \
            {                                               \
                t = s[j]; update_extr_macro(m,t);           \
            }                                               \
            t = s[0]; update_extr_macro(t,m);               \
            dst[i] = (arrtype)t;                            \
            t = s[j]; update_extr_macro(t,m);               \
            dst[i+cn] = (arrtype)t;                         \
        }                                                   \
                                                            \
        for( ; i < width; i += cn )                         \
        {                                                   \
            const arrtype* s = src + i;                     \
            worktype m = s[0], t;                           \
            for( j = cn; j < ksize; j += cn )               \
            {                                               \
                t = s[j]; update_extr_macro(m,t);           \
            }                                               \
            dst[i] = (arrtype)m;                            \
        }                                                   \
    }                                                       \
}


ICV_MORPH_RECT_ROW( Erode, 8u, uchar, int, CV_CALC_MIN_8U )
ICV_MORPH_RECT_ROW( Dilate, 8u, uchar, int, CV_CALC_MAX_8U )
ICV_MORPH_RECT_ROW( Erode, 32f, int, int, CV_CALC_MIN )
ICV_MORPH_RECT_ROW( Dilate, 32f, int, int, CV_CALC_MAX )


#define ICV_MORPH_RECT_COL( name, flavor, arrtype,          \
        worktype, update_extr_macro, toggle_macro )         \
static void                                                 \
icv##name##RectCol_##flavor( const arrtype** src,           \
    arrtype* dst, int dst_step, int count, void* params )   \
{                                                           \
    const CvMorphology* state = (const CvMorphology*)params;\
    int ksize = state->get_kernel_size().height;            \
    int width = state->get_width();                         \
    int cn = CV_MAT_CN(state->get_src_type());              \
    int i, k;                                               \
                                                            \
    width *= cn;                                            \
    dst_step /= sizeof(dst[0]);                             \
                                                            \
    for( ; ksize > 1 && count > 1; count -= 2,              \
                        dst += dst_step*2, src += 2 )       \
    {                                                       \
        for( i = 0; i <= width - 4; i += 4 )                \
        {                                                   \
            const arrtype* sptr = src[1] + i;               \
            worktype s0 = sptr[0], s1 = sptr[1],            \
                s2 = sptr[2], s3 = sptr[3], t0, t1;         \
                                                            \
            for( k = 2; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i;                          \
                t0 = sptr[0]; t1 = sptr[1];                 \
                update_extr_macro(s0,t0);                   \
                update_extr_macro(s1,t1);                   \
                t0 = sptr[2]; t1 = sptr[3];                 \
                update_extr_macro(s2,t0);                   \
                update_extr_macro(s3,t1);                   \
            }                                               \
                                                            \
            sptr = src[0] + i;                              \
            t0 = sptr[0]; t1 = sptr[1];                     \
            update_extr_macro(t0,s0);                       \
            update_extr_macro(t1,s1);                       \
            dst[i] = (arrtype)toggle_macro(t0);             \
            dst[i+1] = (arrtype)toggle_macro(t1);           \
            t0 = sptr[2]; t1 = sptr[3];                     \
            update_extr_macro(t0,s2);                       \
            update_extr_macro(t1,s3);                       \
            dst[i+2] = (arrtype)toggle_macro(t0);           \
            dst[i+3] = (arrtype)toggle_macro(t1);           \
                                                            \
            sptr = src[k] + i;                              \
            t0 = sptr[0]; t1 = sptr[1];                     \
            update_extr_macro(t0,s0);                       \
            update_extr_macro(t1,s1);                       \
            dst[i+dst_step] = (arrtype)toggle_macro(t0);    \
            dst[i+dst_step+1] = (arrtype)toggle_macro(t1);  \
            t0 = sptr[2]; t1 = sptr[3];                     \
            update_extr_macro(t0,s2);                       \
            update_extr_macro(t1,s3);                       \
            dst[i+dst_step+2] = (arrtype)toggle_macro(t0);  \
            dst[i+dst_step+3] = (arrtype)toggle_macro(t1);  \
        }                                                   \
                                                            \
        for( ; i < width; i++ )                             \
        {                                                   \
            const arrtype* sptr = src[1] + i;               \
            worktype s0 = sptr[0], t0;                      \
                                                            \
            for( k = 2; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i; t0 = sptr[0];            \
                update_extr_macro(s0,t0);                   \
            }                                               \
                                                            \
            sptr = src[0] + i; t0 = sptr[0];                \
            update_extr_macro(t0,s0);                       \
            dst[i] = (arrtype)toggle_macro(t0);             \
                                                            \
            sptr = src[k] + i; t0 = sptr[0];                \
            update_extr_macro(t0,s0);                       \
            dst[i+dst_step] = (arrtype)toggle_macro(t0);    \
        }                                                   \
    }                                                       \
                                                            \
    for( ; count > 0; count--, dst += dst_step, src++ )     \
    {                                                       \
        for( i = 0; i <= width - 4; i += 4 )                \
        {                                                   \
            const arrtype* sptr = src[0] + i;               \
            worktype s0 = sptr[0], s1 = sptr[1],            \
                s2 = sptr[2], s3 = sptr[3], t0, t1;         \
                                                            \
            for( k = 1; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i;                          \
                t0 = sptr[0]; t1 = sptr[1];                 \
                update_extr_macro(s0,t0);                   \
                update_extr_macro(s1,t1);                   \
                t0 = sptr[2]; t1 = sptr[3];                 \
                update_extr_macro(s2,t0);                   \
                update_extr_macro(s3,t1);                   \
            }                                               \
            dst[i] = (arrtype)toggle_macro(s0);             \
            dst[i+1] = (arrtype)toggle_macro(s1);           \
            dst[i+2] = (arrtype)toggle_macro(s2);           \
            dst[i+3] = (arrtype)toggle_macro(s3);           \
        }                                                   \
                                                            \
        for( ; i < width; i++ )                             \
        {                                                   \
            const arrtype* sptr = src[0] + i;               \
            worktype s0 = sptr[0], t0;                      \
                                                            \
            for( k = 1; k < ksize; k++ )                    \
            {                                               \
                sptr = src[k] + i; t0 = sptr[0];            \
                update_extr_macro(s0,t0);                   \
            }                                               \
            dst[i] = (arrtype)toggle_macro(s0);             \
        }                                                   \
    }                                                       \
}


ICV_MORPH_RECT_COL( Erode, 8u, uchar, int, CV_CALC_MIN_8U, CV_NOP )
ICV_MORPH_RECT_COL( Dilate, 8u, uchar, int, CV_CALC_MAX_8U, CV_NOP )
ICV_MORPH_RECT_COL( Erode, 32f, int, int, CV_CALC_MIN, CV_TOGGLE_FLT )
ICV_MORPH_RECT_COL( Dilate, 32f, int, int, CV_CALC_MAX, CV_TOGGLE_FLT )


#define ICV_MORPH_ANY( name, flavor, arrtype, worktype,     \
                       update_extr_macro, toggle_macro )    \
static void                                                 \
icv##name##Any_##flavor( const arrtype** src, arrtype* dst, \
                    int dst_step, int count, void* params ) \
{                                                           \
    CvMorphology* state = (CvMorphology*)params;            \
    int width = state->get_width();                         \
    int cn = CV_MAT_CN(state->get_src_type());              \
    int i, k;                                               \
    CvPoint* el_sparse = (CvPoint*)state->get_element_sparse_buf();\
    int el_count = state->get_element_sparse_count();       \
    const arrtype** el_ptr = (const arrtype**)(el_sparse + el_count);\
    const arrtype** el_end = el_ptr + el_count;             \
                                                            \
    width *= cn;                                            \
    dst_step /= sizeof(dst[0]);                             \
                                                            \
    for( ; count > 0; count--, dst += dst_step, src++ )     \
    {                                                       \
        for( k = 0; k < el_count; k++ )                     \
            el_ptr[k] = src[el_sparse[k].y]+el_sparse[k].x; \
                                                            \
        for( i = 0; i <= width - 4; i += 4 )                \
        {                                                   \
            const arrtype** psptr = el_ptr;                 \
            const arrtype* sptr = *psptr++;                 \
            worktype s0 = sptr[i], s1 = sptr[i+1],          \
                     s2 = sptr[i+2], s3 = sptr[i+3], t;     \
                                                            \
            while( psptr != el_end )                        \
            {                                               \
                sptr = *psptr++;                            \
                t = sptr[i];                                \
                update_extr_macro(s0,t);                    \
                t = sptr[i+1];                              \
                update_extr_macro(s1,t);                    \
                t = sptr[i+2];                              \
                update_extr_macro(s2,t);                    \
                t = sptr[i+3];                              \
                update_extr_macro(s3,t);                    \
            }                                               \
                                                            \
            dst[i] = (arrtype)toggle_macro(s0);             \
            dst[i+1] = (arrtype)toggle_macro(s1);           \
            dst[i+2] = (arrtype)toggle_macro(s2);           \
            dst[i+3] = (arrtype)toggle_macro(s3);           \
        }                                                   \
                                                            \
        for( ; i < width; i++ )                             \
        {                                                   \
            const arrtype* sptr = el_ptr[0] + i;            \
            worktype s0 = sptr[0], t0;                      \
                                                            \
            for( k = 1; k < el_count; k++ )                 \
            {                                               \
                sptr = el_ptr[k] + i;                       \
                t0 = sptr[0];                               \
                update_extr_macro(s0,t0);                   \
            }                                               \
                                                            \
            dst[i] = (arrtype)toggle_macro(s0);             \
        }                                                   \
    }                                                       \
}

ICV_MORPH_ANY( Erode, 8u, uchar, int, CV_CALC_MIN, CV_NOP )
ICV_MORPH_ANY( Dilate, 8u, uchar, int, CV_CALC_MAX, CV_NOP )
ICV_MORPH_ANY( Erode, 32f, int, int, CV_CALC_MIN, CV_TOGGLE_FLT )
ICV_MORPH_ANY( Dilate, 32f, int, int, CV_CALC_MAX, CV_TOGGLE_FLT )

/////////////////////////////////// External Interface /////////////////////////////////////


CV_IMPL IplConvKernel *
cvCreateStructuringElementEx( int cols, int rows,
                              int anchorX, int anchorY,
                              int shape, int *values )
{
    IplConvKernel *element = 0;
    int i, size = rows * cols;
    int element_size = sizeof(*element) + size*sizeof(element->values[0]);

    CV_FUNCNAME( "cvCreateStructuringElementEx" );

    __BEGIN__;

    if( !values && shape == CV_SHAPE_CUSTOM )
        CV_ERROR_FROM_STATUS( CV_NULLPTR_ERR );

    if( cols <= 0 || rows <= 0 ||
        (unsigned) anchorX >= (unsigned) cols ||
        (unsigned) anchorY >= (unsigned) rows )
        CV_ERROR_FROM_STATUS( CV_BADSIZE_ERR );

    CV_CALL( element = (IplConvKernel *)cvAlloc(element_size + 32));
    if( !element )
        CV_ERROR_FROM_STATUS( CV_OUTOFMEM_ERR );

    element->nCols = cols;
    element->nRows = rows;
    element->anchorX = anchorX;
    element->anchorY = anchorY;
    element->nShiftR = shape < CV_SHAPE_ELLIPSE ? shape : CV_SHAPE_CUSTOM;
    element->values = (int*)(element + 1);

    if( shape == CV_SHAPE_CUSTOM )
    {
        if( !values )
            CV_ERROR( CV_StsNullPtr, "Null pointer to the custom element mask" );
        for( i = 0; i < size; i++ )
            element->values[i] = values[i];
    }
    else
    {
        CvMat el_hdr = cvMat( rows, cols, CV_32SC1, element->values );
        CV_CALL( CvMorphology::init_binary_element(&el_hdr,
                        shape, cvPoint(anchorX,anchorY)));
    }

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseStructuringElement( &element );

    return element;
}
// 
// 
CV_IMPL void
cvReleaseStructuringElement( IplConvKernel ** element )
{
    CV_FUNCNAME( "cvReleaseStructuringElement" );

    __BEGIN__;

    if( !element )
        CV_ERROR( CV_StsNullPtr, "" );
    cvFree( element );

    __END__;
}
// 
// 
typedef CvStatus (CV_STDCALL * CvMorphRectGetBufSizeFunc_IPP)
    ( int width, CvSize el_size, int* bufsize );
// 
typedef CvStatus (CV_STDCALL * CvMorphRectFunc_IPP)
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize roi, CvSize el_size, CvPoint el_anchor, void* buffer );
// 
typedef CvStatus (CV_STDCALL * CvMorphCustomInitAllocFunc_IPP)
    ( int width, const uchar* element, CvSize el_size,
      CvPoint el_anchor, void** morphstate );

typedef CvStatus (CV_STDCALL * CvMorphCustomFunc_IPP)
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize roi, int bordertype, void* morphstate );
// 
static void
icvMorphOp( const void* srcarr, void* dstarr, IplConvKernel* element,
            int iterations, int mop )
{
    CvMorphology morphology;
    void* buffer = 0;
    int local_alloc = 0;
    void* morphstate = 0;
    CvMat* temp = 0;

    CV_FUNCNAME( "icvMorphOp" );

    __BEGIN__;

    int i, coi1 = 0, coi2 = 0;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat el_hdr, *el = 0;
    CvSize size, el_size;
    CvPoint el_anchor;
    int el_shape;
    int type;
    bool inplace;

    if( !CV_IS_MAT(src) )
        CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    
    if( src != &srcstub )
    {
        srcstub = *src;
        src = &srcstub;
    }

    if( dstarr == srcarr )
        dst = src;
    else
    {
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

        if( !CV_ARE_TYPES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( !CV_ARE_SIZES_EQ( src, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );
    }

    if( dst != &dststub )
    {
        dststub = *dst;
        dst = &dststub;
    }

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    type = CV_MAT_TYPE( src->type );
    size = cvGetMatSize( src );
    inplace = src->data.ptr == dst->data.ptr;

    if( iterations == 0 || (element && element->nCols == 1 && element->nRows == 1))
    {
        if( src->data.ptr != dst->data.ptr )
            cvCopy( src, dst );
        EXIT;
    }

    if( element )
    {
        el_size = cvSize( element->nCols, element->nRows );
        el_anchor = cvPoint( element->anchorX, element->anchorY );
        el_shape = (int)(element->nShiftR);
        el_shape = el_shape < CV_SHAPE_CUSTOM ? el_shape : CV_SHAPE_CUSTOM;
    }
    else
    {
        el_size = cvSize(3,3);
        el_anchor = cvPoint(1,1);
        el_shape = CV_SHAPE_RECT;
    }

    if( el_shape == CV_SHAPE_RECT && iterations > 1 )
    {
        el_size.width += (el_size.width-1)*iterations;
        el_size.height += (el_size.height-1)*iterations;
        el_anchor.x *= iterations;
        el_anchor.y *= iterations;
        iterations = 1;
    }

    if( el_shape == CV_SHAPE_RECT && icvErodeRect_GetBufSize_8u_C1R_p )
    {
        CvMorphRectFunc_IPP rect_func = 0;
        CvMorphRectGetBufSizeFunc_IPP rect_getbufsize_func = 0;

        if( mop == 0 )
        {
            if( type == CV_8UC1 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_8u_C1R_p,
                rect_func = icvErodeRect_8u_C1R_p;
            else if( type == CV_8UC3 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_8u_C3R_p,
                rect_func = icvErodeRect_8u_C3R_p;
            else if( type == CV_8UC4 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_8u_C4R_p,
                rect_func = icvErodeRect_8u_C4R_p;
            else if( type == CV_32FC1 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_32f_C1R_p,
                rect_func = icvErodeRect_32f_C1R_p;
            else if( type == CV_32FC3 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_32f_C3R_p,
                rect_func = icvErodeRect_32f_C3R_p;
            else if( type == CV_32FC4 )
                rect_getbufsize_func = icvErodeRect_GetBufSize_32f_C4R_p,
                rect_func = icvErodeRect_32f_C4R_p;
        }
        else
        {
            if( type == CV_8UC1 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_8u_C1R_p,
                rect_func = icvDilateRect_8u_C1R_p;
            else if( type == CV_8UC3 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_8u_C3R_p,
                rect_func = icvDilateRect_8u_C3R_p;
            else if( type == CV_8UC4 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_8u_C4R_p,
                rect_func = icvDilateRect_8u_C4R_p;
            else if( type == CV_32FC1 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_32f_C1R_p,
                rect_func = icvDilateRect_32f_C1R_p;
            else if( type == CV_32FC3 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_32f_C3R_p,
                rect_func = icvDilateRect_32f_C3R_p;
            else if( type == CV_32FC4 )
                rect_getbufsize_func = icvDilateRect_GetBufSize_32f_C4R_p,
                rect_func = icvDilateRect_32f_C4R_p;
        }

        if( rect_getbufsize_func && rect_func )
        {
            int bufsize = 0;

            CvStatus status = rect_getbufsize_func( size.width, el_size, &bufsize );
            if( status >= 0 && bufsize > 0 )
            {
                if( bufsize < CV_MAX_LOCAL_SIZE )
                {
                    buffer = cvStackAlloc( bufsize );
                    local_alloc = 1;
                }
                else
                    CV_CALL( buffer = cvAlloc( bufsize ));
            }

            if( status >= 0 )
            {
                int src_step, dst_step = dst->step ? dst->step : CV_STUB_STEP;

                if( inplace )
                {
                    CV_CALL( temp = cvCloneMat( dst ));
                    src = temp;
                }
                src_step = src->step ? src->step : CV_STUB_STEP;

                status = rect_func( src->data.ptr, src_step, dst->data.ptr,
                                    dst_step, size, el_size, el_anchor, buffer );
            }
            
            if( status >= 0 )
                EXIT;
        }
    }
    else if( el_shape == CV_SHAPE_CUSTOM && icvMorphInitAlloc_8u_C1R_p && icvMorphFree_p &&
             src->data.ptr != dst->data.ptr )
    {
        CvMorphCustomFunc_IPP custom_func = 0;
        CvMorphCustomInitAllocFunc_IPP custom_initalloc_func = 0;
        const int bordertype = 1; // replication border

        if( type == CV_8UC1 )
            custom_initalloc_func = icvMorphInitAlloc_8u_C1R_p,
            custom_func = mop == 0 ? icvErode_8u_C1R_p : icvDilate_8u_C1R_p;
        else if( type == CV_8UC3 )
            custom_initalloc_func = icvMorphInitAlloc_8u_C3R_p,
            custom_func = mop == 0 ? icvErode_8u_C3R_p : icvDilate_8u_C3R_p;
        else if( type == CV_8UC4 )
            custom_initalloc_func = icvMorphInitAlloc_8u_C4R_p,
            custom_func = mop == 0 ? icvErode_8u_C4R_p : icvDilate_8u_C4R_p;
        else if( type == CV_32FC1 )
            custom_initalloc_func = icvMorphInitAlloc_32f_C1R_p,
            custom_func = mop == 0 ? icvErode_32f_C1R_p : icvDilate_32f_C1R_p;
        else if( type == CV_32FC3 )
            custom_initalloc_func = icvMorphInitAlloc_32f_C3R_p,
            custom_func = mop == 0 ? icvErode_32f_C3R_p : icvDilate_32f_C3R_p;
        else if( type == CV_32FC4 )
            custom_initalloc_func = icvMorphInitAlloc_32f_C4R_p,
            custom_func = mop == 0 ? icvErode_32f_C4R_p : icvDilate_32f_C4R_p;

        if( custom_initalloc_func && custom_func )
        {
            uchar *src_ptr, *dst_ptr = dst->data.ptr;
            int src_step, dst_step = dst->step ? dst->step : CV_STUB_STEP;
            int el_len = el_size.width*el_size.height;
            uchar* el_mask = (uchar*)cvStackAlloc( el_len );
            CvStatus status;

            for( i = 0; i < el_len; i++ )
                el_mask[i] = (uchar)(element->values[i] != 0);

            status = custom_initalloc_func( size.width, el_mask, el_size,
                                            el_anchor, &morphstate );

            if( status >= 0 && (inplace || iterations > 1) )
            {
                CV_CALL( temp = cvCloneMat( dst ));
                src = temp;
            }

            src_ptr = src->data.ptr;
            src_step = src->step ? src->step : CV_STUB_STEP;

            for( i = 0; i < iterations && status >= 0 && morphstate; i++ )
            {
                uchar* t_ptr;
                int t_step;
                status = custom_func( src_ptr, src_step, dst_ptr, dst_step,
                                      size, bordertype, morphstate );
                CV_SWAP( src_ptr, dst_ptr, t_ptr );
                CV_SWAP( src_step, dst_step, t_step );
                if( i == 0 && temp )
                {
                    dst_ptr = temp->data.ptr;
                    dst_step = temp->step ? temp->step : CV_STUB_STEP;
                }
            }

            if( status >= 0 )
            {
                if( iterations % 2 == 0 )
                    cvCopy( temp, dst );
                EXIT;
            }
        }
    }

    if( el_shape != CV_SHAPE_RECT )
    {
        el_hdr = cvMat( element->nRows, element->nCols, CV_32SC1, element->values );
        el = &el_hdr;
        el_shape = CV_SHAPE_CUSTOM;
    }

    CV_CALL( morphology.init( mop, src->cols, src->type,
                    el_shape, el, el_size, el_anchor ));

    for( i = 0; i < iterations; i++ )
    {
        CV_CALL( morphology.process( src, dst ));
        src = dst;
    }

    __END__;

    if( !local_alloc )
        cvFree( &buffer );
    if( morphstate )
        icvMorphFree_p( morphstate );
    cvReleaseMat( &temp );
}
// 
// 
CV_IMPL void
cvErode( const void* src, void* dst, IplConvKernel* element, int iterations )
{
    icvMorphOp( src, dst, element, iterations, 0 );
}
// 
// 
CV_IMPL void
cvDilate( const void* src, void* dst, IplConvKernel* element, int iterations )
{
    icvMorphOp( src, dst, element, iterations, 1 );
}
// 
// /****************************************************************************************\
//                                     Base Image Filter
// \****************************************************************************************/
// 
static void default_x_filter_func( const uchar*, uchar*, void* )
{
}

static void default_y_filter_func( uchar**, uchar*, int, int, void* )
{
}
// 
CvBaseImageFilter::CvBaseImageFilter()
{
    min_depth = CV_8U;
    buffer = 0;
    rows = 0;
    max_width = 0;
    x_func = default_x_filter_func;
    y_func = default_y_filter_func;
}
// 

void CvBaseImageFilter::clear()
{
    cvFree( &buffer );
    rows = 0;
}
// 
// 
CvBaseImageFilter::~CvBaseImageFilter()
{
    clear();
}
// 
// 
void CvBaseImageFilter::get_work_params()
{
    int min_rows = max_ky*2 + 3, rows = MAX(min_rows,10), row_sz;
    int width = max_width, trow_sz = 0;

    if( is_separable )
    {
        int max_depth = MAX(CV_MAT_DEPTH(src_type), CV_MAT_DEPTH(dst_type));
        int max_cn = MAX(CV_MAT_CN(src_type), CV_MAT_CN(dst_type));
        max_depth = MAX( max_depth, min_depth );
        work_type = CV_MAKETYPE( max_depth, max_cn );
        trow_sz = cvAlign( (max_width + ksize.width - 1)*CV_ELEM_SIZE(src_type), ALIGN );
    }
    else
    {
        work_type = src_type;
        width += ksize.width - 1;
    }
    row_sz = cvAlign( width*CV_ELEM_SIZE(work_type), ALIGN );
    buf_size = rows*row_sz;
    buf_size = MIN( buf_size, 1 << 16 );
    buf_size = MAX( buf_size, min_rows*row_sz );
    max_rows = (buf_size/row_sz)*3 + max_ky*2 + 8;
    buf_size += trow_sz;
}
// 
// 
void CvBaseImageFilter::init( int _max_width, int _src_type, int _dst_type,
                              bool _is_separable, CvSize _ksize, CvPoint _anchor,
                              int _border_mode, CvScalar _border_value )
{
    CV_FUNCNAME( "CvBaseImageFilter::init" );

    __BEGIN__;

    int total_buf_sz, src_pix_sz, row_tab_sz, bsz;
    uchar* ptr;

    if( !(buffer && _max_width <= max_width && _src_type == src_type &&
        _dst_type == dst_type && _is_separable == is_separable &&
        _ksize.width == ksize.width && _ksize.height == ksize.height &&
        _anchor.x == anchor.x && _anchor.y == anchor.y) )
        clear();

    is_separable = _is_separable != 0;
    max_width = _max_width; //MAX(_max_width,_ksize.width);
    src_type = CV_MAT_TYPE(_src_type);
    dst_type = CV_MAT_TYPE(_dst_type);
    ksize = _ksize;
    anchor = _anchor;

    if( anchor.x == -1 )
        anchor.x = ksize.width / 2;
    if( anchor.y == -1 )
        anchor.y = ksize.height / 2;

    max_ky = MAX( anchor.y, ksize.height - anchor.y - 1 ); 
    border_mode = _border_mode;
    border_value = _border_value;

    if( ksize.width <= 0 || ksize.height <= 0 ||
        (unsigned)anchor.x >= (unsigned)ksize.width ||
        (unsigned)anchor.y >= (unsigned)ksize.height )
        CV_ERROR( CV_StsOutOfRange, "invalid kernel size and/or anchor position" );

    if( border_mode != IPL_BORDER_CONSTANT && border_mode != IPL_BORDER_REPLICATE &&
        border_mode != IPL_BORDER_REFLECT && border_mode != IPL_BORDER_REFLECT_101 )
        CV_ERROR( CV_StsBadArg, "Invalid/unsupported border mode" );

    get_work_params();

    prev_width = 0;
    prev_x_range = cvSlice(0,0);

    buf_size = cvAlign( buf_size, ALIGN );

    src_pix_sz = CV_ELEM_SIZE(src_type);
    border_tab_sz1 = anchor.x*src_pix_sz;
    border_tab_sz = (ksize.width-1)*src_pix_sz;
    bsz = cvAlign( border_tab_sz*sizeof(int), ALIGN );

    assert( max_rows > max_ky*2 );
    row_tab_sz = cvAlign( max_rows*sizeof(uchar*), ALIGN );
    total_buf_sz = buf_size + row_tab_sz + bsz;
    
    CV_CALL( ptr = buffer = (uchar*)cvAlloc( total_buf_sz ));
    
    rows = (uchar**)ptr;
    ptr += row_tab_sz;
    border_tab = (int*)ptr;
    ptr += bsz;

    buf_start = ptr;
    const_row = 0;

    if( border_mode == IPL_BORDER_CONSTANT )
        cvScalarToRawData( &border_value, border_tab, src_type, 0 );

    __END__;
}
// 
// 
void CvBaseImageFilter::start_process( CvSlice x_range, int width )
{
    int mode = border_mode;
    int pix_sz = CV_ELEM_SIZE(src_type), work_pix_sz = CV_ELEM_SIZE(work_type);
    int bsz = buf_size, bw = x_range.end_index - x_range.start_index, bw1 = bw + ksize.width - 1;
    int tr_step = cvAlign(bw1*pix_sz, ALIGN );
    int i, j, k, ofs;
    
    if( x_range.start_index == prev_x_range.start_index &&
        x_range.end_index == prev_x_range.end_index &&
        width == prev_width )
        return;

    prev_x_range = x_range;
    prev_width = width;

    if( !is_separable )
        bw = bw1;
    else
        bsz -= tr_step;

    buf_step = cvAlign(bw*work_pix_sz, ALIGN);

    if( mode == IPL_BORDER_CONSTANT )
        bsz -= buf_step;
    buf_max_count = bsz/buf_step;
    buf_max_count = MIN( buf_max_count, max_rows - max_ky*2 );
    buf_end = buf_start + buf_max_count*buf_step;

    if( mode == IPL_BORDER_CONSTANT )
    {
        int i, tab_len = ksize.width*pix_sz;
        uchar* bt = (uchar*)border_tab;
        uchar* trow = buf_end;
        const_row = buf_end + (is_separable ? 1 : 0)*tr_step;

        for( i = pix_sz; i < tab_len; i++ )
            bt[i] = bt[i - pix_sz];
        for( i = 0; i < pix_sz; i++ )
            trow[i] = bt[i];
        for( i = pix_sz; i < tr_step; i++ )
            trow[i] = trow[i - pix_sz];
        if( is_separable )
            x_func( trow, const_row, this );
        return;
    }

    if( x_range.end_index - x_range.start_index <= 1 )
        mode = IPL_BORDER_REPLICATE;

    width = (width - 1)*pix_sz;
    ofs = (anchor.x-x_range.start_index)*pix_sz;

    for( k = 0; k < 2; k++ )
    {
        int idx, delta;
        int i1, i2, di;

        if( k == 0 )
        {
            idx = (x_range.start_index - 1)*pix_sz;
            delta = di = -pix_sz;
            i1 = border_tab_sz1 - pix_sz;
            i2 = -pix_sz;
        }
        else
        {
            idx = x_range.end_index*pix_sz;
            delta = di = pix_sz;
            i1 = border_tab_sz1;
            i2 = border_tab_sz;
        }

        if( (unsigned)idx > (unsigned)width )
        {
            int shift = mode == IPL_BORDER_REFLECT_101 ? pix_sz : 0;
            idx = k == 0 ? shift : width - shift;
            delta = -delta;
        }

        for( i = i1; i != i2; i += di )
        {
            for( j = 0; j < pix_sz; j++ )
                border_tab[i + j] = idx + ofs + j;
            if( mode != IPL_BORDER_REPLICATE )
            {
                if( delta > 0 && idx == width ||
                    delta < 0 && idx == 0 )
                {
                    if( mode == IPL_BORDER_REFLECT_101 )
                        idx -= delta*2;
                    delta = -delta;
                }
                else
                    idx += delta;
            }
        }
    }
}
// 
// 
void CvBaseImageFilter::make_y_border( int row_count, int top_rows, int bottom_rows )
{
    int i;
    
    if( border_mode == IPL_BORDER_CONSTANT ||
        border_mode == IPL_BORDER_REPLICATE )
    {
        uchar* row1 = border_mode == IPL_BORDER_CONSTANT ? const_row : rows[max_ky];
        
        for( i = 0; i < top_rows && rows[i] == 0; i++ )
            rows[i] = row1;

        row1 = border_mode == IPL_BORDER_CONSTANT ? const_row : rows[row_count-1];
        for( i = 0; i < bottom_rows; i++ )
            rows[i + row_count] = row1;
    }
    else
    {
        int j, dj = 1, shift = border_mode == IPL_BORDER_REFLECT_101;

        for( i = top_rows-1, j = top_rows+shift; i >= 0; i-- )
        {
            if( rows[i] == 0 )
                rows[i] = rows[j];
            j += dj;
            if( dj > 0 && j >= row_count )
            {
                if( !bottom_rows )
                    break;
                j -= 1 + shift;
                dj = -dj;
            }
        }

        for( i = 0, j = row_count-1-shift; i < bottom_rows; i++, j-- )
            rows[i + row_count] = rows[j];
    }
}
// 
// 
int CvBaseImageFilter::fill_cyclic_buffer( const uchar* src, int src_step,
                                           int y0, int y1, int y2 )
{
    int i, y = y0, bsz1 = border_tab_sz1, bsz = border_tab_sz;
    int pix_size = CV_ELEM_SIZE(src_type);
    int width = prev_x_range.end_index - prev_x_range.start_index, width_n = width*pix_size;
    bool can_use_src_as_trow = is_separable && width >= ksize.width;

    // fill the cyclic buffer
    for( ; buf_count < buf_max_count && y < y2; buf_count++, y++, src += src_step )
    {
        uchar* trow = is_separable ? buf_end : buf_tail;
        uchar* bptr = can_use_src_as_trow && y1 < y && y+1 < y2 ? (uchar*)(src - bsz1) : trow;

        if( bptr != trow )
        {
            for( i = 0; i < bsz1; i++ )
                trow[i] = bptr[i];
            for( ; i < bsz; i++ )
                trow[i] = bptr[i + width_n];
        }
        else if( !(((size_t)(bptr + bsz1)|(size_t)src|width_n) & (sizeof(int)-1)) )
            for( i = 0; i < width_n; i += sizeof(int) )
                *(int*)(bptr + i + bsz1) = *(int*)(src + i);
        else
            for( i = 0; i < width_n; i++ )
                bptr[i + bsz1] = src[i];

        if( border_mode != IPL_BORDER_CONSTANT )
        {
            for( i = 0; i < bsz1; i++ )
            {
                int j = border_tab[i];
                bptr[i] = bptr[j];
            }
            for( ; i < bsz; i++ )
            {
                int j = border_tab[i];
                bptr[i + width_n] = bptr[j];
            }
        }
        else
        {
            const uchar *bt = (uchar*)border_tab; 
            for( i = 0; i < bsz1; i++ )
                bptr[i] = bt[i];

            for( ; i < bsz; i++ )
                bptr[i + width_n] = bt[i];
        }

        if( is_separable )
        {
            x_func( bptr, buf_tail, this );
            if( bptr != trow )
            {
                for( i = 0; i < bsz1; i++ )
                    bptr[i] = trow[i];
                for( ; i < bsz; i++ )
                    bptr[i + width_n] = trow[i];
            }
        }

        buf_tail += buf_step;
        if( buf_tail >= buf_end )
            buf_tail = buf_start;
    }

    return y - y0;
}

int CvBaseImageFilter::process( const CvMat* src, CvMat* dst,
                                CvRect src_roi, CvPoint dst_origin, int flags )
{
    int rows_processed = 0;

    /*
        check_parameters
        initialize_horizontal_border_reloc_tab_if_not_initialized_yet
        
        for_each_source_row: src starts from src_roi.y, buf starts with the first available row
            1) if separable,
                   1a.1) copy source row to temporary buffer, form a border using border reloc tab.
                   1a.2) apply row-wise filter (symmetric, asymmetric or generic)
               else
                   1b.1) copy source row to the buffer, form a border
            2) if the buffer is full, or it is the last source row:
                 2.1) if stage != middle, form the pointers to other "virtual" rows.
                 if separable
                    2a.2) apply column-wise filter, store the results.
                 else
                    2b.2) form a sparse (offset,weight) tab
                    2b.3) apply generic non-separable filter, store the results
            3) update row pointers etc.
    */

    CV_FUNCNAME( "CvBaseImageFilter::process" );

    __BEGIN__;

    int i, width, _src_y1, _src_y2;
    int src_x, src_y, src_y1, src_y2, dst_y;
    int pix_size = CV_ELEM_SIZE(src_type);
    uchar *sptr = 0, *dptr;
    int phase = flags & (CV_START|CV_END|CV_MIDDLE);
    bool isolated_roi = (flags & CV_ISOLATED_ROI) != 0;

    if( !CV_IS_MAT(src) )
        CV_ERROR( CV_StsBadArg, "" );

    if( CV_MAT_TYPE(src->type) != src_type )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    width = src->cols;

    if( src_roi.width == -1 && src_roi.x == 0 )
        src_roi.width = width;

    if( src_roi.height == -1 && src_roi.y == 0 )
    {
        src_roi.y = 0;
        src_roi.height = src->rows;
    }

    if( src_roi.width > max_width ||
        src_roi.x < 0 || src_roi.width < 0 ||
        src_roi.y < 0 || src_roi.height < 0 ||
        src_roi.x + src_roi.width > width ||
        src_roi.y + src_roi.height > src->rows )
        CV_ERROR( CV_StsOutOfRange, "Too large source image or its ROI" );

    src_x = src_roi.x;
    _src_y1 = 0;
    _src_y2 = src->rows;

    if( isolated_roi )
    {
        src_roi.x = 0;
        width = src_roi.width;
        _src_y1 = src_roi.y;
        _src_y2 = src_roi.y + src_roi.height;
    }

    if( !CV_IS_MAT(dst) )
        CV_ERROR( CV_StsBadArg, "" );

    if( CV_MAT_TYPE(dst->type) != dst_type )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( dst_origin.x < 0 || dst_origin.y < 0 )
        CV_ERROR( CV_StsOutOfRange, "Incorrect destination ROI origin" );

    if( phase == CV_WHOLE )
        phase = CV_START | CV_END;
    phase &= CV_START | CV_END | CV_MIDDLE;

    // initialize horizontal border relocation tab if it is not initialized yet
    if( phase & CV_START )
        start_process( cvSlice(src_roi.x, src_roi.x + src_roi.width), width );
    else if( prev_width != width || prev_x_range.start_index != src_roi.x ||
        prev_x_range.end_index != src_roi.x + src_roi.width )
        CV_ERROR( CV_StsBadArg,
            "In a middle or at the end the horizontal placement of the stripe can not be changed" );

    dst_y = dst_origin.y;
    src_y1 = src_roi.y;
    src_y2 = src_roi.y + src_roi.height;

    if( phase & CV_START )
    {
        for( i = 0; i <= max_ky*2; i++ )
            rows[i] = 0;
        
        src_y1 -= max_ky;
        top_rows = bottom_rows = 0;

        if( src_y1 < _src_y1 )
        {
            top_rows = _src_y1 - src_y1;
            src_y1 = _src_y1;
        }

        buf_head = buf_tail = buf_start;
        buf_count = 0;
    }

    if( phase & CV_END )
    {
        src_y2 += max_ky;

        if( src_y2 > _src_y2 )
        {
            bottom_rows = src_y2 - _src_y2;
            src_y2 = _src_y2;
        }
    }
    
    dptr = dst->data.ptr + dst_origin.y*dst->step + dst_origin.x*CV_ELEM_SIZE(dst_type);
    sptr = src->data.ptr + src_y1*src->step + src_x*pix_size;
        
    for( src_y = src_y1; src_y < src_y2; )
    {
        uchar* bptr;
        int row_count, delta;

        delta = fill_cyclic_buffer( sptr, src->step, src_y, src_y1, src_y2 );

        src_y += delta;
        sptr += src->step*delta;

        // initialize the cyclic buffer row pointers
        bptr = buf_head;
        for( i = 0; i < buf_count; i++ )
        {
            rows[i+top_rows] = bptr;
            bptr += buf_step;
            if( bptr >= buf_end )
                bptr = buf_start;
        }

        row_count = top_rows + buf_count;
        
        if( !rows[0] || (phase & CV_END) && src_y == src_y2 )
        {
            int br = (phase & CV_END) && src_y == src_y2 ? bottom_rows : 0;
            make_y_border( row_count, top_rows, br );
            row_count += br;
        }

        if( rows[0] && row_count > max_ky*2 )
        {
            int count = row_count - max_ky*2;
            if( dst_y + count > dst->rows )
                CV_ERROR( CV_StsOutOfRange, "The destination image can not fit the result" );

            assert( count >= 0 );
            y_func( rows + max_ky - anchor.y, dptr, dst->step, count, this );
            row_count -= count;
            dst_y += count;
            dptr += dst->step*count;
            for( bptr = row_count > 0 ?rows[count] : 0; buf_head != bptr && buf_count > 0; buf_count-- )
            {
                buf_head += buf_step;
                if( buf_head >= buf_end )
                    buf_head = buf_start;
            }
            rows_processed += count;
            top_rows = MAX(top_rows - count, 0);
        }
    }

    __END__;

    return rows_processed;
}
// 
// 
// /****************************************************************************************\
//                                     Separable Linear Filter
// \****************************************************************************************/
// 
static void icvFilterRowSymm_8u32s( const uchar* src, int* dst, void* params );
static void icvFilterColSymm_32s8u( const int** src, uchar* dst, int dst_step,
                                    int count, void* params );
static void icvFilterColSymm_32s16s( const int** src, short* dst, int dst_step,
                                     int count, void* params );
static void icvFilterRowSymm_8u32f( const uchar* src, float* dst, void* params );
static void icvFilterRow_8u32f( const uchar* src, float* dst, void* params );
static void icvFilterRowSymm_16s32f( const short* src, float* dst, void* params );
static void icvFilterRow_16s32f( const short* src, float* dst, void* params );
static void icvFilterRowSymm_16u32f( const ushort* src, float* dst, void* params );
static void icvFilterRow_16u32f( const ushort* src, float* dst, void* params );
static void icvFilterRowSymm_32f( const float* src, float* dst, void* params );
static void icvFilterRow_32f( const float* src, float* dst, void* params );

static void icvFilterColSymm_32f8u( const float** src, uchar* dst, int dst_step,
                                    int count, void* params );
static void icvFilterCol_32f8u( const float** src, uchar* dst, int dst_step,
                                int count, void* params );
static void icvFilterColSymm_32f16s( const float** src, short* dst, int dst_step,
                                     int count, void* params );
static void icvFilterCol_32f16s( const float** src, short* dst, int dst_step,
                                 int count, void* params );
static void icvFilterColSymm_32f16u( const float** src, ushort* dst, int dst_step,
                                     int count, void* params );
static void icvFilterCol_32f16u( const float** src, ushort* dst, int dst_step,
                                 int count, void* params );
static void icvFilterColSymm_32f( const float** src, float* dst, int dst_step,
                                  int count, void* params );
static void icvFilterCol_32f( const float** src, float* dst, int dst_step,
                              int count, void* params );

CvSepFilter::CvSepFilter()
{
    min_depth = CV_32F;
    kx = ky = 0;
    kx_flags = ky_flags = 0;
}
// 

CvSepFilter::CvSepFilter( int _max_width, int _src_type, int _dst_type,
                          const CvMat* _kx, const CvMat* _ky,
                          CvPoint _anchor, int _border_mode,
                          CvScalar _border_value )
{
    min_depth = CV_32F;
    kx = ky = 0;
    init( _max_width, _src_type, _dst_type, _kx, _ky, _anchor, _border_mode, _border_value );
}


void CvSepFilter::clear()
{
    cvReleaseMat( &kx );
    cvReleaseMat( &ky );
    CvBaseImageFilter::clear();
}
// 
// 
CvSepFilter::~CvSepFilter()
{
    clear();
}


#undef FILTER_BITS
#define FILTER_BITS 8

void CvSepFilter::init( int _max_width, int _src_type, int _dst_type,
                        const CvMat* _kx, const CvMat* _ky,
                        CvPoint _anchor, int _border_mode,
                        CvScalar _border_value )
{
    CV_FUNCNAME( "CvSepFilter::init" );

    __BEGIN__;

    CvSize _ksize;
    int filter_type;
    int i, xsz, ysz;
    int convert_filters = 0;
    double xsum = 0, ysum = 0;
    const float eps = FLT_EPSILON*100.f;

    if( !CV_IS_MAT(_kx) || !CV_IS_MAT(_ky) ||
        _kx->cols != 1 && _kx->rows != 1 ||
        _ky->cols != 1 && _ky->rows != 1 ||
        CV_MAT_CN(_kx->type) != 1 || CV_MAT_CN(_ky->type) != 1 ||
        !CV_ARE_TYPES_EQ(_kx,_ky) )
        CV_ERROR( CV_StsBadArg,
        "Both kernels must be valid 1d single-channel vectors of the same types" );

    if( CV_MAT_CN(_src_type) != CV_MAT_CN(_dst_type) )
        CV_ERROR( CV_StsUnmatchedFormats, "Input and output must have the same number of channels" );

    filter_type = MAX( CV_32F, CV_MAT_DEPTH(_kx->type) );

    _ksize.width = _kx->rows + _kx->cols - 1;
    _ksize.height = _ky->rows + _ky->cols - 1;

    CV_CALL( CvBaseImageFilter::init( _max_width, _src_type, _dst_type, 1, _ksize,
                                      _anchor, _border_mode, _border_value ));

    if( !(kx && CV_ARE_SIZES_EQ(kx,_kx)) )
    {
        cvReleaseMat( &kx );
        CV_CALL( kx = cvCreateMat( _kx->rows, _kx->cols, filter_type ));
    }

    if( !(ky && CV_ARE_SIZES_EQ(ky,_ky)) )
    {
        cvReleaseMat( &ky );
        CV_CALL( ky = cvCreateMat( _ky->rows, _ky->cols, filter_type ));
    }

    CV_CALL( cvConvert( _kx, kx ));
    CV_CALL( cvConvert( _ky, ky ));

    xsz = kx->rows + kx->cols - 1;
    ysz = ky->rows + ky->cols - 1;
    kx_flags = ky_flags = ASYMMETRICAL + SYMMETRICAL + POSITIVE + SUM_TO_1 + INTEGER;
    
    if( !(xsz & 1) )
        kx_flags &= ~(ASYMMETRICAL + SYMMETRICAL);
    if( !(ysz & 1) )
        ky_flags &= ~(ASYMMETRICAL + SYMMETRICAL);

    for( i = 0; i < xsz; i++ )
    {
        float v = kx->data.fl[i];
        xsum += v;
        if( v < 0 )
            kx_flags &= ~POSITIVE;
        if( fabs(v - cvRound(v)) > eps )
            kx_flags &= ~INTEGER;
        if( fabs(v - kx->data.fl[xsz - i - 1]) > eps )
            kx_flags &= ~SYMMETRICAL;
        if( fabs(v + kx->data.fl[xsz - i - 1]) > eps )
            kx_flags &= ~ASYMMETRICAL;
    }

    if( fabs(xsum - 1.) > eps )
        kx_flags &= ~SUM_TO_1;
    
    for( i = 0; i < ysz; i++ )
    {
        float v = ky->data.fl[i];
        ysum += v;
        if( v < 0 )
            ky_flags &= ~POSITIVE;
        if( fabs(v - cvRound(v)) > eps )
            ky_flags &= ~INTEGER;
        if( fabs(v - ky->data.fl[ysz - i - 1]) > eps )
            ky_flags &= ~SYMMETRICAL;
        if( fabs(v + ky->data.fl[ysz - i - 1]) > eps )
            ky_flags &= ~ASYMMETRICAL;
    }

    if( fabs(ysum - 1.) > eps )
        ky_flags &= ~SUM_TO_1;

    x_func = 0;
    y_func = 0;

    if( CV_MAT_DEPTH(src_type) == CV_8U )
    {
        if( CV_MAT_DEPTH(dst_type) == CV_8U &&
            ((kx_flags&ky_flags) & (SYMMETRICAL + POSITIVE + SUM_TO_1)) == SYMMETRICAL + POSITIVE + SUM_TO_1 )
        {
            x_func = (CvRowFilterFunc)icvFilterRowSymm_8u32s;
            y_func = (CvColumnFilterFunc)icvFilterColSymm_32s8u;
            kx_flags &= ~INTEGER;
            ky_flags &= ~INTEGER;
            convert_filters = 1;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_16S &&
            (kx_flags & (SYMMETRICAL + ASYMMETRICAL)) && (kx_flags & INTEGER) &&
            (ky_flags & (SYMMETRICAL + ASYMMETRICAL)) && (ky_flags & INTEGER) )
        {
            x_func = (CvRowFilterFunc)icvFilterRowSymm_8u32s;
            y_func = (CvColumnFilterFunc)icvFilterColSymm_32s16s;
            convert_filters = 1;
        }
        else
        {
            if( CV_MAT_DEPTH(dst_type) > CV_32F )
                CV_ERROR( CV_StsUnsupportedFormat, "8u->64f separable filtering is not supported" );

            if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
                x_func = (CvRowFilterFunc)icvFilterRowSymm_8u32f;
            else
                x_func = (CvRowFilterFunc)icvFilterRow_8u32f;
        }
    }
    else if( CV_MAT_DEPTH(src_type) == CV_16U )
    {
        if( CV_MAT_DEPTH(dst_type) > CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "16u->64f separable filtering is not supported" );

        if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
            x_func = (CvRowFilterFunc)icvFilterRowSymm_16u32f;
        else
            x_func = (CvRowFilterFunc)icvFilterRow_16u32f;
    }
    else if( CV_MAT_DEPTH(src_type) == CV_16S )
    {
        if( CV_MAT_DEPTH(dst_type) > CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "16s->64f separable filtering is not supported" );

        if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
            x_func = (CvRowFilterFunc)icvFilterRowSymm_16s32f;
        else
            x_func = (CvRowFilterFunc)icvFilterRow_16s32f;
    }
    else if( CV_MAT_DEPTH(src_type) == CV_32F )
    {
        if( CV_MAT_DEPTH(dst_type) != CV_32F )
            CV_ERROR( CV_StsUnsupportedFormat, "When the input has 32f data type, the output must also have 32f type" );

        if( kx_flags & (SYMMETRICAL + ASYMMETRICAL) )
            x_func = (CvRowFilterFunc)icvFilterRowSymm_32f;
        else
            x_func = (CvRowFilterFunc)icvFilterRow_32f;
    }
    else
        CV_ERROR( CV_StsUnsupportedFormat, "Unknown or unsupported input data type" );

    if( !y_func )
    {
        if( CV_MAT_DEPTH(dst_type) == CV_8U )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f8u;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f8u;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_16U )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f16u;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f16u;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_16S )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f16s;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f16s;
        }
        else if( CV_MAT_DEPTH(dst_type) == CV_32F )
        {
            if( ky_flags & (SYMMETRICAL + ASYMMETRICAL) )
                y_func = (CvColumnFilterFunc)icvFilterColSymm_32f;
            else
                y_func = (CvColumnFilterFunc)icvFilterCol_32f;
        }
        else
            CV_ERROR( CV_StsUnsupportedFormat, "Unknown or unsupported input data type" );
    }

    if( convert_filters )
    {
        int scale = kx_flags & ky_flags & INTEGER ? 1 : (1 << FILTER_BITS);
        int sum;
        
        for( i = sum = 0; i < xsz; i++ )
        {
            int t = cvRound(kx->data.fl[i]*scale);
            kx->data.i[i] = t;
            sum += t;
        }
        if( scale > 1 )
            kx->data.i[xsz/2] += scale - sum;

        for( i = sum = 0; i < ysz; i++ )
        {
            int t = cvRound(ky->data.fl[i]*scale);
            ky->data.i[i] = t;
            sum += t;
        }
        if( scale > 1 )
            ky->data.i[ysz/2] += scale - sum;
        kx->type = (kx->type & ~CV_MAT_DEPTH_MASK) | CV_32S;
        ky->type = (ky->type & ~CV_MAT_DEPTH_MASK) | CV_32S;
    }

    __END__;
}
// 
// 
void CvSepFilter::init( int _max_width, int _src_type, int _dst_type,
                        bool _is_separable, CvSize _ksize,
                        CvPoint _anchor, int _border_mode,
                        CvScalar _border_value )
{
    CvBaseImageFilter::init( _max_width, _src_type, _dst_type, _is_separable,
                             _ksize, _anchor, _border_mode, _border_value );
}
// 
// 
static void
icvFilterRowSymm_8u32s( const uchar* src, int* dst, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _kx = state->get_x_kernel();
    const int* kx = _kx->data.i;
    int ksize = _kx->cols + _kx->rows - 1;
    int i = 0, j, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int ksize2 = ksize/2, ksize2n = ksize2*cn;
    int is_symm = state->get_x_kernel_flags() & CvSepFilter::SYMMETRICAL;
    const uchar* s = src + ksize2n;

    kx += ksize2;
    width *= cn;

    if( is_symm )
    {
        if( ksize == 1 && kx[0] == 1 )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = s[i], s1 = s[i+1];
                dst[i] = s0; dst[i+1] = s1;
            }
            s += i;
        }
        else if( ksize == 3 )
        {
            if( kx[0] == 2 && kx[1] == 1 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[-cn] + s[0]*2 + s[cn], s1 = s[1-cn] + s[1]*2 + s[1+cn];
                    dst[i] = s0; dst[i+1] = s1;
                }
            else if( kx[0] == 10 && kx[1] == 3 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[0]*10 + (s[-cn] + s[cn])*3, s1 = s[1]*10 + (s[1-cn] + s[1+cn])*3;
                    dst[i] = s0; dst[i+1] = s1;
                }
            else if( kx[0] == 2*64 && kx[1] == 1*64 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = (s[0]*2 + s[-cn] + s[cn]) << 6;
                    int s1 = (s[1]*2 + s[1-cn] + s[1+cn]) << 6;
                    dst[i] = s0; dst[i+1] = s1;
                }
            else
            {
                int k0 = kx[0], k1 = kx[1];
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[0]*k0 + (s[-cn] + s[cn])*k1, s1 = s[1]*k0 + (s[1-cn] + s[1+cn])*k1;
                    dst[i] = s0; dst[i+1] = s1;
                }
            }
        }
        else if( ksize == 5 )
        {
            int k0 = kx[0], k1 = kx[1], k2 = kx[2];
            if( k0 == 6*16 && k1 == 4*16 && k2 == 1*16 )
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = (s[0]*6 + (s[-cn] + s[cn])*4 + (s[-cn*2] + s[cn*2])*1) << 4;
                    int s1 = (s[1]*6 + (s[1-cn] + s[1+cn])*4 + (s[1-cn*2] + s[1+cn*2])*1) << 4;
                    dst[i] = s0; dst[i+1] = s1;
                }
            else
                for( ; i <= width - 2; i += 2, s += 2 )
                {
                    int s0 = s[0]*k0 + (s[-cn] + s[cn])*k1 + (s[-cn*2] + s[cn*2])*k2;
                    int s1 = s[1]*k0 + (s[1-cn] + s[1+cn])*k1 + (s[1-cn*2] + s[1+cn*2])*k2;
                    dst[i] = s0; dst[i+1] = s1;
                }
        }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                int f = kx[0];
                int s0 = f*s[0], s1 = f*s[1], s2 = f*s[2], s3 = f*s[3];
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    f = kx[k];
                    s0 += f*(s[j] + s[-j]); s1 += f*(s[j+1] + s[-j+1]);
                    s2 += f*(s[j+2] + s[-j+2]); s3 += f*(s[j+3] + s[-j+3]);
                }

                dst[i] = s0; dst[i+1] = s1;
                dst[i+2] = s2; dst[i+3] = s3;
            }

        for( ; i < width; i++, s++ )
        {
            int s0 = kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += kx[k]*(s[j] + s[-j]);
            dst[i] = s0;
        }
    }
    else
    {
        if( ksize == 3 && kx[0] == 0 && kx[1] == 1 )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                int s0 = s[cn] - s[-cn], s1 = s[1+cn] - s[1-cn];
                dst[i] = s0; dst[i+1] = s1;
            }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    int f = kx[k];
                    s0 += f*(s[j] - s[-j]); s1 += f*(s[j+1] - s[-j+1]);
                    s2 += f*(s[j+2] - s[-j+2]); s3 += f*(s[j+3] - s[-j+3]);
                }

                dst[i] = s0; dst[i+1] = s1;
                dst[i+2] = s2; dst[i+3] = s3;
            }

        for( ; i < width; i++, s++ )
        {
            int s0 = kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += kx[k]*(s[j] - s[-j]);
            dst[i] = s0;
        }
    }
}
// 
// 
#define ICV_FILTER_ROW( flavor, srctype, dsttype, load_macro )      \
static void                                                         \
icvFilterRow_##flavor(const srctype* src, dsttype* dst, void*params)\
{                                                                   \
    const CvSepFilter* state = (const CvSepFilter*)params;          \
    const CvMat* _kx = state->get_x_kernel();                       \
    const dsttype* kx = (const dsttype*)(_kx->data.ptr);            \
    int ksize = _kx->cols + _kx->rows - 1;                          \
    int i = 0, k, width = state->get_width();                       \
    int cn = CV_MAT_CN(state->get_src_type());                      \
    const srctype* s;                                               \
                                                                    \
    width *= cn;                                                    \
                                                                    \
    for( ; i <= width - 4; i += 4 )                                 \
    {                                                               \
        double f = kx[0];                                           \
        double s0=f*load_macro(src[i]), s1=f*load_macro(src[i+1]),  \
                s2=f*load_macro(src[i+2]), s3=f*load_macro(src[i+3]);\
        for( k = 1, s = src + i + cn; k < ksize; k++, s += cn )     \
        {                                                           \
            f = kx[k];                                              \
            s0 += f*load_macro(s[0]);                               \
            s1 += f*load_macro(s[1]);                               \
            s2 += f*load_macro(s[2]);                               \
            s3 += f*load_macro(s[3]);                               \
        }                                                           \
        dst[i] = (dsttype)s0; dst[i+1] = (dsttype)s1;               \
        dst[i+2] = (dsttype)s2; dst[i+3] = (dsttype)s3;             \
    }                                                               \
                                                                    \
    for( ; i < width; i++ )                                         \
    {                                                               \
        double s0 = (double)kx[0]*load_macro(src[i]);               \
        for( k = 1, s = src + i + cn; k < ksize; k++, s += cn )     \
            s0 += (double)kx[k]*load_macro(s[0]);                   \
        dst[i] = (dsttype)s0;                                       \
    }                                                               \
}


ICV_FILTER_ROW( 8u32f, uchar, float, CV_8TO32F )
ICV_FILTER_ROW( 16s32f, short, float, CV_NOP )
ICV_FILTER_ROW( 16u32f, ushort, float, CV_NOP )
ICV_FILTER_ROW( 32f, float, float, CV_NOP )

// 
#define ICV_FILTER_ROW_SYMM( flavor, srctype, dsttype, load_macro ) \
static void                                                         \
icvFilterRowSymm_##flavor( const srctype* src,                      \
                           dsttype* dst, void* params )             \
{                                                                   \
    const CvSepFilter* state = (const CvSepFilter*)params;          \
    const CvMat* _kx = state->get_x_kernel();                       \
    const dsttype* kx = (const dsttype*)(_kx->data.ptr);            \
    int ksize = _kx->cols + _kx->rows - 1;                          \
    int i = 0, j, k, width = state->get_width();                    \
    int cn = CV_MAT_CN(state->get_src_type());                      \
    int is_symm=state->get_x_kernel_flags()&CvSepFilter::SYMMETRICAL;\
    int ksize2 = ksize/2, ksize2n = ksize2*cn;                      \
    const srctype* s = src + ksize2n;                               \
                                                                    \
    kx += ksize2;                                                   \
    width *= cn;                                                    \
                                                                    \
    if( is_symm )                                                   \
    {                                                               \
        for( ; i <= width - 4; i += 4, s += 4 )                     \
        {                                                           \
            double f = kx[0];                                       \
            double s0=f*load_macro(s[0]), s1=f*load_macro(s[1]),    \
                   s2=f*load_macro(s[2]), s3=f*load_macro(s[3]);    \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
            {                                                       \
                f = kx[k];                                          \
                s0 += f*load_macro(s[j] + s[-j]);                   \
                s1 += f*load_macro(s[j+1] + s[-j+1]);               \
                s2 += f*load_macro(s[j+2] + s[-j+2]);               \
                s3 += f*load_macro(s[j+3] + s[-j+3]);               \
            }                                                       \
                                                                    \
            dst[i] = (dsttype)s0; dst[i+1] = (dsttype)s1;           \
            dst[i+2] = (dsttype)s2; dst[i+3] = (dsttype)s3;         \
        }                                                           \
                                                                    \
        for( ; i < width; i++, s++ )                                \
        {                                                           \
            double s0 = (double)kx[0]*load_macro(s[0]);             \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
                s0 += (double)kx[k]*load_macro(s[j] + s[-j]);       \
            dst[i] = (dsttype)s0;                                   \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        for( ; i <= width - 4; i += 4, s += 4 )                     \
        {                                                           \
            double s0 = 0, s1 = 0, s2 = 0, s3 = 0;                  \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
            {                                                       \
                double f = kx[k];                                   \
                s0 += f*load_macro(s[j] - s[-j]);                   \
                s1 += f*load_macro(s[j+1] - s[-j+1]);               \
                s2 += f*load_macro(s[j+2] - s[-j+2]);               \
                s3 += f*load_macro(s[j+3] - s[-j+3]);               \
            }                                                       \
                                                                    \
            dst[i] = (dsttype)s0; dst[i+1] = (dsttype)s1;           \
            dst[i+2] = (dsttype)s2; dst[i+3] = (dsttype)s3;         \
        }                                                           \
                                                                    \
        for( ; i < width; i++, s++ )                                \
        {                                                           \
            double s0 = 0;                                          \
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )         \
                s0 += (double)kx[k]*load_macro(s[j] - s[-j]);       \
            dst[i] = (dsttype)s0;                                   \
        }                                                           \
    }                                                               \
}


ICV_FILTER_ROW_SYMM( 8u32f, uchar, float, CV_8TO32F )
ICV_FILTER_ROW_SYMM( 16s32f, short, float, CV_NOP )
ICV_FILTER_ROW_SYMM( 16u32f, ushort, float, CV_NOP )

static void
icvFilterRowSymm_32f( const float* src, float* dst, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _kx = state->get_x_kernel();
    const float* kx = _kx->data.fl;
    int ksize = _kx->cols + _kx->rows - 1;
    int i = 0, j, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int ksize2 = ksize/2, ksize2n = ksize2*cn;
    int is_symm = state->get_x_kernel_flags() & CvSepFilter::SYMMETRICAL;
    const float* s = src + ksize2n;

    kx += ksize2;
    width *= cn;

    if( is_symm )
    {
        if( ksize == 3 && fabs(kx[0]-2.) <= FLT_EPSILON && fabs(kx[1]-1.) <= FLT_EPSILON )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                float s0 = s[-cn] + s[0]*2 + s[cn], s1 = s[1-cn] + s[1]*2 + s[1+cn];
                dst[i] = s0; dst[i+1] = s1;
            }
        else if( ksize == 3 && fabs(kx[0]-10.) <= FLT_EPSILON && fabs(kx[1]-3.) <= FLT_EPSILON )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                float s0 = s[0]*10 + (s[-cn] + s[cn])*3, s1 = s[1]*10 + (s[1-cn] + s[1+cn])*3;
                dst[i] = s0; dst[i+1] = s1;
            }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                double f = kx[0];
                double s0 = f*s[0], s1 = f*s[1], s2 = f*s[2], s3 = f*s[3];
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    f = kx[k];
                    s0 += f*(s[j] + s[-j]); s1 += f*(s[j+1] + s[-j+1]);
                    s2 += f*(s[j+2] + s[-j+2]); s3 += f*(s[j+3] + s[-j+3]);
                }

                dst[i] = (float)s0; dst[i+1] = (float)s1;
                dst[i+2] = (float)s2; dst[i+3] = (float)s3;
            }

        for( ; i < width; i++, s++ )
        {
            double s0 = (double)kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += (double)kx[k]*(s[j] + s[-j]);
            dst[i] = (float)s0;
        }
    }
    else
    {
        if( ksize == 3 && fabs(kx[0]) <= FLT_EPSILON && fabs(kx[1]-1.) <= FLT_EPSILON )
            for( ; i <= width - 2; i += 2, s += 2 )
            {
                float s0 = s[cn] - s[-cn], s1 = s[1+cn] - s[1-cn];
                dst[i] = s0; dst[i+1] = s1;
            }
        else
            for( ; i <= width - 4; i += 4, s += 4 )
            {
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                {
                    double f = kx[k];
                    s0 += f*(s[j] - s[-j]); s1 += f*(s[j+1] - s[-j+1]);
                    s2 += f*(s[j+2] - s[-j+2]); s3 += f*(s[j+3] - s[-j+3]);
                }

                dst[i] = (float)s0; dst[i+1] = (float)s1;
                dst[i+2] = (float)s2; dst[i+3] = (float)s3;
            }

        for( ; i < width; i++, s++ )
        {
            double s0 = (double)kx[0]*s[0];
            for( k = 1, j = cn; k <= ksize2; k++, j += cn )
                s0 += (double)kx[k]*(s[j] - s[-j]);
            dst[i] = (float)s0;
        }
    }
}


static void
icvFilterColSymm_32s8u( const int** src, uchar* dst, int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const int* ky = _ky->data.i;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;
    int i, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());

    width *= cn;
    src += ksize2;
    ky += ksize2;

    for( ; count--; dst += dst_step, src++ )
    {
        if( ksize == 3 )
        {
            const int* sptr0 = src[-1], *sptr1 = src[0], *sptr2 = src[1];
            int k0 = ky[0], k1 = ky[1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sptr1[i]*k0 + (sptr0[i] + sptr2[i])*k1;
                int s1 = sptr1[i+1]*k0 + (sptr0[i+1] + sptr2[i+1])*k1;
                s0 = CV_DESCALE(s0, FILTER_BITS*2);
                s1 = CV_DESCALE(s1, FILTER_BITS*2);
                dst[i] = (uchar)s0; dst[i+1] = (uchar)s1;
            }
        }
        else if( ksize == 5 )
        {
            const int* sptr0 = src[-2], *sptr1 = src[-1],
                *sptr2 = src[0], *sptr3 = src[1], *sptr4 = src[2];
            int k0 = ky[0], k1 = ky[1], k2 = ky[2];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sptr2[i]*k0 + (sptr1[i] + sptr3[i])*k1 + (sptr0[i] + sptr4[i])*k2;
                int s1 = sptr2[i+1]*k0 + (sptr1[i+1] + sptr3[i+1])*k1 + (sptr0[i+1] + sptr4[i+1])*k2;
                s0 = CV_DESCALE(s0, FILTER_BITS*2);
                s1 = CV_DESCALE(s1, FILTER_BITS*2);
                dst[i] = (uchar)s0; dst[i+1] = (uchar)s1;
            }
        }
        else
            for( i = 0; i <= width - 4; i += 4 )
            {
                int f = ky[0];
                const int* sptr = src[0] + i, *sptr2;
                int s0 = f*sptr[0], s1 = f*sptr[1], s2 = f*sptr[2], s3 = f*sptr[3];
                for( k = 1; k <= ksize2; k++ )
                {
                    sptr = src[k] + i;
                    sptr2 = src[-k] + i;
                    f = ky[k];
                    s0 += f*(sptr[0] + sptr2[0]);
                    s1 += f*(sptr[1] + sptr2[1]);
                    s2 += f*(sptr[2] + sptr2[2]);
                    s3 += f*(sptr[3] + sptr2[3]);
                }

                s0 = CV_DESCALE(s0, FILTER_BITS*2);
                s1 = CV_DESCALE(s1, FILTER_BITS*2);
                dst[i] = (uchar)s0; dst[i+1] = (uchar)s1;
                s2 = CV_DESCALE(s2, FILTER_BITS*2);
                s3 = CV_DESCALE(s3, FILTER_BITS*2);
                dst[i+2] = (uchar)s2; dst[i+3] = (uchar)s3;
            }

        for( ; i < width; i++ )
        {
            int s0 = ky[0]*src[0][i];
            for( k = 1; k <= ksize2; k++ )
                s0 += ky[k]*(src[k][i] + src[-k][i]);

            s0 = CV_DESCALE(s0, FILTER_BITS*2);
            dst[i] = (uchar)s0;
        }
    }
}


static void
icvFilterColSymm_32s16s( const int** src, short* dst,
                         int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const int* ky = (const int*)_ky->data.ptr;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;
    int i = 0, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int is_symm = state->get_y_kernel_flags() & CvSepFilter::SYMMETRICAL;
    int is_1_2_1 = is_symm && ksize == 3 && ky[1] == 2 && ky[2] == 1;
    int is_3_10_3 = is_symm && ksize == 3 && ky[1] == 10 && ky[2] == 3;
    int is_m1_0_1 = !is_symm && ksize == 3 && ky[1] == 0 &&
        ky[2]*ky[2] == 1 ? (ky[2] > 0 ? 1 : -1) : 0;

    width *= cn;
    src += ksize2;
    ky += ksize2;
    dst_step /= sizeof(dst[0]);

    if( is_symm )
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_1_2_1 )
            {
                const int *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src0[i] + src1[i]*2 + src2[i],
                        s1 = src0[i+1] + src1[i+1]*2 + src2[i+1];

                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            }
            else if( is_3_10_3 )
            {
                const int *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src1[i]*10 + (src0[i] + src2[i])*3,
                        s1 = src1[i+1]*10 + (src0[i+1] + src2[i+1])*3;

                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    int f = ky[0];
                    const int* sptr = src[0] + i, *sptr2;
                    int s0 = f*sptr[0], s1 = f*sptr[1],
                        s2 = f*sptr[2], s3 = f*sptr[3];
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] + sptr2[0]); s1 += f*(sptr[1] + sptr2[1]);
                        s2 += f*(sptr[2] + sptr2[2]); s3 += f*(sptr[3] + sptr2[3]);
                    }

                    dst[i] = CV_CAST_16S(s0); dst[i+1] = CV_CAST_16S(s1);
                    dst[i+2] = CV_CAST_16S(s2); dst[i+3] = CV_CAST_16S(s3);
                }

            for( ; i < width; i++ )
            {
                int s0 = ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += ky[k]*(src[k][i] + src[-k][i]);
                dst[i] = CV_CAST_16S(s0);
            }
        }
    }
    else
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_m1_0_1 )
            {
                const int *src0 = src[-is_m1_0_1], *src2 = src[is_m1_0_1];

                for( i = 0; i <= width - 2; i += 2 )
                {
                    int s0 = src2[i] - src0[i], s1 = src2[i+1] - src0[i+1];
                    dst[i] = (short)s0; dst[i+1] = (short)s1;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    int f = ky[0];
                    const int* sptr = src[0] + i, *sptr2;
                    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] - sptr2[0]); s1 += f*(sptr[1] - sptr2[1]);
                        s2 += f*(sptr[2] - sptr2[2]); s3 += f*(sptr[3] - sptr2[3]);
                    }

                    dst[i] = CV_CAST_16S(s0); dst[i+1] = CV_CAST_16S(s1);
                    dst[i+2] = CV_CAST_16S(s2); dst[i+3] = CV_CAST_16S(s3);
                }

            for( ; i < width; i++ )
            {
                int s0 = ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += ky[k]*(src[k][i] - src[-k][i]);
                dst[i] = CV_CAST_16S(s0);
            }
        }
    }
}


#define ICV_FILTER_COL( flavor, srctype, dsttype, worktype,     \
                        cast_macro1, cast_macro2 )              \
static void                                                     \
icvFilterCol_##flavor( const srctype** src, dsttype* dst,       \
                       int dst_step, int count, void* params )  \
{                                                               \
    const CvSepFilter* state = (const CvSepFilter*)params;      \
    const CvMat* _ky = state->get_y_kernel();                   \
    const srctype* ky = (const srctype*)_ky->data.ptr;          \
    int ksize = _ky->cols + _ky->rows - 1;                      \
    int i, k, width = state->get_width();                       \
    int cn = CV_MAT_CN(state->get_src_type());                  \
                                                                \
    width *= cn;                                                \
    dst_step /= sizeof(dst[0]);                                 \
                                                                \
    for( ; count--; dst += dst_step, src++ )                    \
    {                                                           \
        for( i = 0; i <= width - 4; i += 4 )                    \
        {                                                       \
            double f = ky[0];                                   \
            const srctype* sptr = src[0] + i;                   \
            double s0 = f*sptr[0], s1 = f*sptr[1],              \
                   s2 = f*sptr[2], s3 = f*sptr[3];              \
            worktype t0, t1;                                    \
            for( k = 1; k < ksize; k++ )                        \
            {                                                   \
                sptr = src[k] + i; f = ky[k];                   \
                s0 += f*sptr[0]; s1 += f*sptr[1];               \
                s2 += f*sptr[2]; s3 += f*sptr[3];               \
            }                                                   \
                                                                \
            t0 = cast_macro1(s0); t1 = cast_macro1(s1);         \
            dst[i]=cast_macro2(t0); dst[i+1]=cast_macro2(t1);   \
            t0 = cast_macro1(s2); t1 = cast_macro1(s3);         \
            dst[i+2]=cast_macro2(t0); dst[i+3]=cast_macro2(t1); \
        }                                                       \
                                                                \
        for( ; i < width; i++ )                                 \
        {                                                       \
            double s0 = (double)ky[0]*src[0][i];                \
            worktype t0;                                        \
            for( k = 1; k < ksize; k++ )                        \
                s0 += (double)ky[k]*src[k][i];                  \
            t0 = cast_macro1(s0);                               \
            dst[i] = cast_macro2(t0);                           \
        }                                                       \
    }                                                           \
}


ICV_FILTER_COL( 32f8u, float, uchar, int, cvRound, CV_CAST_8U )
ICV_FILTER_COL( 32f16s, float, short, int, cvRound, CV_CAST_16S )
ICV_FILTER_COL( 32f16u, float, ushort, int, cvRound, CV_CAST_16U )

#define ICV_FILTER_COL_SYMM( flavor, srctype, dsttype, worktype,    \
                             cast_macro1, cast_macro2 )             \
static void                                                         \
icvFilterColSymm_##flavor( const srctype** src, dsttype* dst,       \
                           int dst_step, int count, void* params )  \
{                                                                   \
    const CvSepFilter* state = (const CvSepFilter*)params;          \
    const CvMat* _ky = state->get_y_kernel();                       \
    const srctype* ky = (const srctype*)_ky->data.ptr;              \
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;        \
    int i, k, width = state->get_width();                           \
    int cn = CV_MAT_CN(state->get_src_type());                      \
    int is_symm = state->get_y_kernel_flags() & CvSepFilter::SYMMETRICAL;\
                                                                    \
    width *= cn;                                                    \
    src += ksize2;                                                  \
    ky += ksize2;                                                   \
    dst_step /= sizeof(dst[0]);                                     \
                                                                    \
    if( is_symm )                                                   \
    {                                                               \
        for( ; count--; dst += dst_step, src++ )                    \
        {                                                           \
            for( i = 0; i <= width - 4; i += 4 )                    \
            {                                                       \
                double f = ky[0];                                   \
                const srctype* sptr = src[0] + i, *sptr2;           \
                double s0 = f*sptr[0], s1 = f*sptr[1],              \
                       s2 = f*sptr[2], s3 = f*sptr[3];              \
                worktype t0, t1;                                    \
                for( k = 1; k <= ksize2; k++ )                      \
                {                                                   \
                    sptr = src[k] + i;                              \
                    sptr2 = src[-k] + i;                            \
                    f = ky[k];                                      \
                    s0 += f*(sptr[0] + sptr2[0]);                   \
                    s1 += f*(sptr[1] + sptr2[1]);                   \
                    s2 += f*(sptr[2] + sptr2[2]);                   \
                    s3 += f*(sptr[3] + sptr2[3]);                   \
                }                                                   \
                                                                    \
                t0 = cast_macro1(s0); t1 = cast_macro1(s1);         \
                dst[i]=cast_macro2(t0); dst[i+1]=cast_macro2(t1);   \
                t0 = cast_macro1(s2); t1 = cast_macro1(s3);         \
                dst[i+2]=cast_macro2(t0); dst[i+3]=cast_macro2(t1); \
            }                                                       \
                                                                    \
            for( ; i < width; i++ )                                 \
            {                                                       \
                double s0 = (double)ky[0]*src[0][i];                \
                worktype t0;                                        \
                for( k = 1; k <= ksize2; k++ )                      \
                    s0 += (double)ky[k]*(src[k][i] + src[-k][i]);   \
                t0 = cast_macro1(s0);                               \
                dst[i] = cast_macro2(t0);                           \
            }                                                       \
        }                                                           \
    }                                                               \
    else                                                            \
    {                                                               \
        for( ; count--; dst += dst_step, src++ )                    \
        {                                                           \
            for( i = 0; i <= width - 4; i += 4 )                    \
            {                                                       \
                double f = ky[0];                                   \
                const srctype* sptr = src[0] + i, *sptr2;           \
                double s0 = 0, s1 = 0, s2 = 0, s3 = 0;              \
                worktype t0, t1;                                    \
                for( k = 1; k <= ksize2; k++ )                      \
                {                                                   \
                    sptr = src[k] + i;                              \
                    sptr2 = src[-k] + i;                            \
                    f = ky[k];                                      \
                    s0 += f*(sptr[0] - sptr2[0]);                   \
                    s1 += f*(sptr[1] - sptr2[1]);                   \
                    s2 += f*(sptr[2] - sptr2[2]);                   \
                    s3 += f*(sptr[3] - sptr2[3]);                   \
                }                                                   \
                                                                    \
                t0 = cast_macro1(s0); t1 = cast_macro1(s1);         \
                dst[i]=cast_macro2(t0); dst[i+1]=cast_macro2(t1);   \
                t0 = cast_macro1(s2); t1 = cast_macro1(s3);         \
                dst[i+2]=cast_macro2(t0); dst[i+3]=cast_macro2(t1); \
            }                                                       \
                                                                    \
            for( ; i < width; i++ )                                 \
            {                                                       \
                double s0 = (double)ky[0]*src[0][i];                \
                worktype t0;                                        \
                for( k = 1; k <= ksize2; k++ )                      \
                    s0 += (double)ky[k]*(src[k][i] - src[-k][i]);   \
                t0 = cast_macro1(s0);                               \
                dst[i] = cast_macro2(t0);                           \
            }                                                       \
        }                                                           \
    }                                                               \
}


ICV_FILTER_COL_SYMM( 32f8u, float, uchar, int, cvRound, CV_CAST_8U )
ICV_FILTER_COL_SYMM( 32f16s, float, short, int, cvRound, CV_CAST_16S )
ICV_FILTER_COL_SYMM( 32f16u, float, ushort, int, cvRound, CV_CAST_16U )


static void
icvFilterCol_32f( const float** src, float* dst,
                  int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const float* ky = (const float*)_ky->data.ptr;
    int ksize = _ky->cols + _ky->rows - 1;
    int i, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());

    width *= cn;
    dst_step /= sizeof(dst[0]);

    for( ; count--; dst += dst_step, src++ )
    {
        for( i = 0; i <= width - 4; i += 4 )
        {
            double f = ky[0];
            const float* sptr = src[0] + i;
            double s0 = f*sptr[0], s1 = f*sptr[1],
                   s2 = f*sptr[2], s3 = f*sptr[3];
            for( k = 1; k < ksize; k++ )
            {
                sptr = src[k] + i; f = ky[k];
                s0 += f*sptr[0]; s1 += f*sptr[1];
                s2 += f*sptr[2]; s3 += f*sptr[3];
            }

            dst[i] = (float)s0; dst[i+1] = (float)s1;
            dst[i+2] = (float)s2; dst[i+3] = (float)s3;
        }

        for( ; i < width; i++ )
        {
            double s0 = (double)ky[0]*src[0][i];
            for( k = 1; k < ksize; k++ )
                s0 += (double)ky[k]*src[k][i];
            dst[i] = (float)s0;
        }
    }
}


static void
icvFilterColSymm_32f( const float** src, float* dst,
                      int dst_step, int count, void* params )
{
    const CvSepFilter* state = (const CvSepFilter*)params;
    const CvMat* _ky = state->get_y_kernel();
    const float* ky = (const float*)_ky->data.ptr;
    int ksize = _ky->cols + _ky->rows - 1, ksize2 = ksize/2;
    int i = 0, k, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int is_symm = state->get_y_kernel_flags() & CvSepFilter::SYMMETRICAL;
    int is_1_2_1 = is_symm && ksize == 3 &&
        fabs(ky[1] - 2.) <= FLT_EPSILON && fabs(ky[2] - 1.) <= FLT_EPSILON;
    int is_3_10_3 = is_symm && ksize == 3 &&
            fabs(ky[1] - 10.) <= FLT_EPSILON && fabs(ky[2] - 3.) <= FLT_EPSILON;
    int is_m1_0_1 = !is_symm && ksize == 3 &&
            fabs(ky[1]) <= FLT_EPSILON && fabs(ky[2]*ky[2] - 1.) <= FLT_EPSILON ?
            (ky[2] > 0 ? 1 : -1) : 0;

    width *= cn;
    src += ksize2;
    ky += ksize2;
    dst_step /= sizeof(dst[0]);

    if( is_symm )
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_1_2_1 )
            {
                const float *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 4; i += 4 )
                {
                    float s0 = src0[i] + src1[i]*2 + src2[i],
                          s1 = src0[i+1] + src1[i+1]*2 + src2[i+1],
                          s2 = src0[i+2] + src1[i+2]*2 + src2[i+2],
                          s3 = src0[i+3] + src1[i+3]*2 + src2[i+3];

                    dst[i] = s0; dst[i+1] = s1;
                    dst[i+2] = s2; dst[i+3] = s3;
                }
            }
            else if( is_3_10_3 )
            {
                const float *src0 = src[-1], *src1 = src[0], *src2 = src[1];

                for( i = 0; i <= width - 4; i += 4 )
                {
                    float s0 = src1[i]*10 + (src0[i] + src2[i])*3,
                          s1 = src1[i+1]*10 + (src0[i+1] + src2[i+1])*3,
                          s2 = src1[i+2]*10 + (src0[i+2] + src2[i+2])*3,
                          s3 = src1[i+3]*10 + (src0[i+3] + src2[i+3])*3;

                    dst[i] = s0; dst[i+1] = s1;
                    dst[i+2] = s2; dst[i+3] = s3;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    double f = ky[0];
                    const float* sptr = src[0] + i, *sptr2;
                    double s0 = f*sptr[0], s1 = f*sptr[1],
                           s2 = f*sptr[2], s3 = f*sptr[3];
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] + sptr2[0]); s1 += f*(sptr[1] + sptr2[1]);
                        s2 += f*(sptr[2] + sptr2[2]); s3 += f*(sptr[3] + sptr2[3]);
                    }

                    dst[i] = (float)s0; dst[i+1] = (float)s1;
                    dst[i+2] = (float)s2; dst[i+3] = (float)s3;
                }

            for( ; i < width; i++ )
            {
                double s0 = (double)ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += (double)ky[k]*(src[k][i] + src[-k][i]);
                dst[i] = (float)s0;
            }
        }
    }
    else
    {
        for( ; count--; dst += dst_step, src++ )
        {
            if( is_m1_0_1 )
            {
                const float *src0 = src[-is_m1_0_1], *src2 = src[is_m1_0_1];

                for( i = 0; i <= width - 4; i += 4 )
                {
                    float s0 = src2[i] - src0[i], s1 = src2[i+1] - src0[i+1],
                          s2 = src2[i+2] - src0[i+2], s3 = src2[i+3] - src0[i+3];
                    dst[i] = s0; dst[i+1] = s1;
                    dst[i+2] = s2; dst[i+3] = s3;
                }
            }
            else
                for( i = 0; i <= width - 4; i += 4 )
                {
                    double f = ky[0];
                    const float* sptr = src[0] + i, *sptr2;
                    double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                    for( k = 1; k <= ksize2; k++ )
                    {
                        sptr = src[k] + i; sptr2 = src[-k] + i; f = ky[k];
                        s0 += f*(sptr[0] - sptr2[0]); s1 += f*(sptr[1] - sptr2[1]);
                        s2 += f*(sptr[2] - sptr2[2]); s3 += f*(sptr[3] - sptr2[3]);
                    }

                    dst[i] = (float)s0; dst[i+1] = (float)s1;
                    dst[i+2] = (float)s2; dst[i+3] = (float)s3;
                }

            for( ; i < width; i++ )
            {
                double s0 = (double)ky[0]*src[0][i];
                for( k = 1; k <= ksize2; k++ )
                    s0 += (double)ky[k]*(src[k][i] - src[-k][i]);
                dst[i] = (float)s0;
            }
        }
    }
}


#define SMALL_GAUSSIAN_SIZE  7

void CvSepFilter::init_gaussian_kernel( CvMat* kernel, double sigma )
{
    static const float small_gaussian_tab[][SMALL_GAUSSIAN_SIZE/2+1] =
    {
        {1.f},
        {0.5f, 0.25f},
        {0.375f, 0.25f, 0.0625f},
        {0.28125f, 0.21875f, 0.109375f, 0.03125f}
    };

    CV_FUNCNAME( "CvSepFilter::init_gaussian_kernel" );

    __BEGIN__;

    int type, i, n, step;
    const float* fixed_kernel = 0;
    double sigmaX, scale2X, sum;
    float* cf;
    double* cd;

    if( !CV_IS_MAT(kernel) )
        CV_ERROR( CV_StsBadArg, "kernel is not a valid matrix" );

    type = CV_MAT_TYPE(kernel->type);
    
    if( kernel->cols != 1 && kernel->rows != 1 ||
        (kernel->cols + kernel->rows - 1) % 2 == 0 ||
        type != CV_32FC1 && type != CV_64FC1 )
        CV_ERROR( CV_StsBadSize, "kernel should be 1D floating-point vector of odd (2*k+1) size" );

    n = kernel->cols + kernel->rows - 1;

    if( n <= SMALL_GAUSSIAN_SIZE && sigma <= 0 )
        fixed_kernel = small_gaussian_tab[n>>1];

    sigmaX = sigma > 0 ? sigma : (n/2 - 1)*0.3 + 0.8;
    scale2X = -0.5/(sigmaX*sigmaX);
    step = kernel->rows == 1 ? 1 : kernel->step/CV_ELEM_SIZE1(type);
    cf = kernel->data.fl;
    cd = kernel->data.db;

    sum = fixed_kernel ? -fixed_kernel[0] : -1.;

    for( i = 0; i <= n/2; i++ )
    {
        double t = fixed_kernel ? (double)fixed_kernel[i] : exp(scale2X*i*i);
        if( type == CV_32FC1 )
        {
            cf[(n/2+i)*step] = (float)t;
            sum += cf[(n/2+i)*step]*2;
        }
        else
        {
            cd[(n/2+i)*step] = t;
            sum += cd[(n/2+i)*step]*2;
        }
    }

    sum = 1./sum;
    for( i = 0; i <= n/2; i++ )
    {
        if( type == CV_32FC1 )
            cf[(n/2+i)*step] = cf[(n/2-i)*step] = (float)(cf[(n/2+i)*step]*sum);
        else
            cd[(n/2+i)*step] = cd[(n/2-i)*step] = cd[(n/2+i)*step]*sum;
    }

    __END__;
}
// 
// 
void CvSepFilter::init_sobel_kernel( CvMat* _kx, CvMat* _ky, int dx, int dy, int flags )
{
    CV_FUNCNAME( "CvSepFilter::init_sobel_kernel" );

    __BEGIN__;

    int i, j, k, msz;
    int* kerI;
    bool normalize = (flags & NORMALIZE_KERNEL) != 0;
    bool flip = (flags & FLIP_KERNEL) != 0;

    if( !CV_IS_MAT(_kx) || !CV_IS_MAT(_ky) )
        CV_ERROR( CV_StsBadArg, "One of the kernel matrices is not valid" );

    msz = MAX( _kx->cols + _kx->rows, _ky->cols + _ky->rows );
    if( msz > 32 )
        CV_ERROR( CV_StsOutOfRange, "Too large kernel size" );

    kerI = (int*)cvStackAlloc( msz*sizeof(kerI[0]) );

    if( dx < 0 || dy < 0 || dx+dy <= 0 )
        CV_ERROR( CV_StsOutOfRange,
            "Both derivative orders (dx and dy) must be non-negative "
            "and at least one of them must be positive." );

    for( k = 0; k < 2; k++ )
    {
        CvMat* kernel = k == 0 ? _kx : _ky;
        int order = k == 0 ? dx : dy;
        int n = kernel->cols + kernel->rows - 1, step;
        int type = CV_MAT_TYPE(kernel->type);
        double scale = !normalize ? 1. : 1./(1 << (n-order-1));
        int iscale = 1;

        if( kernel->cols != 1 && kernel->rows != 1 ||
            (kernel->cols + kernel->rows - 1) % 2 == 0 ||
            type != CV_32FC1 && type != CV_64FC1 && type != CV_32SC1 )
            CV_ERROR( CV_StsOutOfRange,
            "Both kernels must be 1D floating-point or integer vectors of odd (2*k+1) size." );

        if( normalize && n > 1 && type == CV_32SC1 )
            CV_ERROR( CV_StsBadArg, "Integer kernel can not be normalized" );

        if( n <= order )
            CV_ERROR( CV_StsOutOfRange,
            "Derivative order must be smaller than the corresponding kernel size" );

        if( n == 1 )
            kerI[0] = 1;
        else if( n == 3 )
        {
            if( order == 0 )
                kerI[0] = 1, kerI[1] = 2, kerI[2] = 1;
            else if( order == 1 )
                kerI[0] = -1, kerI[1] = 0, kerI[2] = 1;
            else
                kerI[0] = 1, kerI[1] = -2, kerI[2] = 1;
        }
        else
        {
            int oldval, newval;
            kerI[0] = 1;
            for( i = 0; i < n; i++ )
                kerI[i+1] = 0;

            for( i = 0; i < n - order - 1; i++ )
            {
                oldval = kerI[0];
                for( j = 1; j <= n; j++ )
                {
                    newval = kerI[j]+kerI[j-1];
                    kerI[j-1] = oldval;
                    oldval = newval;
                }
            }

            for( i = 0; i < order; i++ )
            {
                oldval = -kerI[0];
                for( j = 1; j <= n; j++ )
                {
                    newval = kerI[j-1] - kerI[j];
                    kerI[j-1] = oldval;
                    oldval = newval;
                }
            }
        }

        step = kernel->rows == 1 ? 1 : kernel->step/CV_ELEM_SIZE1(type);
        if( flip && (order & 1) && k )
            iscale = -iscale, scale = -scale;

        for( i = 0; i < n; i++ )
        {
            if( type == CV_32SC1 )
                kernel->data.i[i*step] = kerI[i]*iscale;
            else if( type == CV_32FC1 )
                kernel->data.fl[i*step] = (float)(kerI[i]*scale);
            else
                kernel->data.db[i*step] = kerI[i]*scale;
        }
    }

    __END__;
}
// 
// 
void CvSepFilter::init_scharr_kernel( CvMat* _kx, CvMat* _ky, int dx, int dy, int flags )
{
    CV_FUNCNAME( "CvSepFilter::init_scharr_kernel" );

    __BEGIN__;

    int i, k;
    int kerI[3];
    bool normalize = (flags & NORMALIZE_KERNEL) != 0;
    bool flip = (flags & FLIP_KERNEL) != 0;

    if( !CV_IS_MAT(_kx) || !CV_IS_MAT(_ky) )
        CV_ERROR( CV_StsBadArg, "One of the kernel matrices is not valid" );

    if( ((dx|dy)&~1) || dx+dy != 1 )
        CV_ERROR( CV_StsOutOfRange,
            "Scharr kernel can only be used for 1st order derivatives" );

    for( k = 0; k < 2; k++ )
    {
        CvMat* kernel = k == 0 ? _kx : _ky;
        int order = k == 0 ? dx : dy;
        int n = kernel->cols + kernel->rows - 1, step;
        int type = CV_MAT_TYPE(kernel->type);
        double scale = !normalize ? 1. : order == 0 ? 1./16 : 1./2;
        int iscale = 1;

        if( kernel->cols != 1 && kernel->rows != 1 ||
            kernel->cols + kernel->rows - 1 != 3 ||
            type != CV_32FC1 && type != CV_64FC1 && type != CV_32SC1 )
            CV_ERROR( CV_StsOutOfRange,
            "Both kernels must be 1D floating-point or integer vectors containing 3 elements each." );

        if( normalize && type == CV_32SC1 )
            CV_ERROR( CV_StsBadArg, "Integer kernel can not be normalized" );

        if( order == 0 )
            kerI[0] = 3, kerI[1] = 10, kerI[2] = 3;
        else
            kerI[0] = -1, kerI[1] = 0, kerI[2] = 1;

        step = kernel->rows == 1 ? 1 : kernel->step/CV_ELEM_SIZE1(type);
        if( flip && (order & 1) && k )
            iscale = -iscale, scale = -scale;

        for( i = 0; i < n; i++ )
        {
            if( type == CV_32SC1 )
                kernel->data.i[i*step] = kerI[i]*iscale;
            else if( type == CV_32FC1 )
                kernel->data.fl[i*step] = (float)(kerI[i]*scale);
            else
                kernel->data.db[i*step] = kerI[i]*scale;
        }
    }

    __END__;
}
// 
// 
void CvSepFilter::init_deriv( int _max_width, int _src_type, int _dst_type,
                              int dx, int dy, int aperture_size, int flags )
{
    CV_FUNCNAME( "CvSepFilter::init_deriv" );

    __BEGIN__;

    int kx_size = aperture_size == CV_SCHARR ? 3 : aperture_size, ky_size = kx_size;
    float kx_data[CV_MAX_SOBEL_KSIZE], ky_data[CV_MAX_SOBEL_KSIZE];
    CvMat _kx, _ky;

    if( kx_size <= 0 || ky_size > CV_MAX_SOBEL_KSIZE )
        CV_ERROR( CV_StsOutOfRange, "Incorrect aperture_size" );

    if( kx_size == 1 && dx )
        kx_size = 3;
    if( ky_size == 1 && dy )
        ky_size = 3;

    _kx = cvMat( 1, kx_size, CV_32FC1, kx_data );
    _ky = cvMat( 1, ky_size, CV_32FC1, ky_data );

    if( aperture_size == CV_SCHARR )
    {
        CV_CALL( init_scharr_kernel( &_kx, &_ky, dx, dy, flags ));
    }
    else
    {
        CV_CALL( init_sobel_kernel( &_kx, &_ky, dx, dy, flags ));
    }

    CV_CALL( init( _max_width, _src_type, _dst_type, &_kx, &_ky ));

    __END__;
}
// 
// 
void CvSepFilter::init_gaussian( int _max_width, int _src_type, int _dst_type,
                                 int gaussian_size, double sigma )
{
    float* kdata = 0;
    
    CV_FUNCNAME( "CvSepFilter::init_gaussian" );

    __BEGIN__;

    CvMat _kernel;

    if( gaussian_size <= 0 || gaussian_size > 1024 )
        CV_ERROR( CV_StsBadSize, "Incorrect size of gaussian kernel" );

    kdata = (float*)cvStackAlloc(gaussian_size*sizeof(kdata[0]));
    _kernel = cvMat( 1, gaussian_size, CV_32F, kdata );

    CV_CALL( init_gaussian_kernel( &_kernel, sigma ));
    CV_CALL( init( _max_width, _src_type, _dst_type, &_kernel, &_kernel )); 

    __END__;
}

CvMat* icvIPPFilterInit( const CvMat* src, int stripe_size, CvSize ksize )
{
    CvSize temp_size;
    int pix_size = CV_ELEM_SIZE(src->type);
    temp_size.width = cvAlign(src->cols + ksize.width - 1,8/CV_ELEM_SIZE(src->type & CV_MAT_DEPTH_MASK));
    //temp_size.width = src->cols + ksize.width - 1;
    temp_size.height = (stripe_size*2 + temp_size.width*pix_size) / (temp_size.width*pix_size*2);
    temp_size.height = MAX( temp_size.height, ksize.height );
    temp_size.height = MIN( temp_size.height, src->rows + ksize.height - 1 );
    
    return cvCreateMat( temp_size.height, temp_size.width, src->type );
}
// 
// 
int icvIPPFilterNextStripe( const CvMat* src, CvMat* temp, int y,
                            CvSize ksize, CvPoint anchor )
{
    int pix_size = CV_ELEM_SIZE(src->type);
    int src_step = src->step ? src->step : CV_STUB_STEP;
    int temp_step = temp->step ? temp->step : CV_STUB_STEP;
    int i, dy, src_y1 = 0, src_y2;
    int temp_rows;
    uchar* temp_ptr = temp->data.ptr;
    CvSize stripe_size, temp_size;

    dy = MIN( temp->rows - ksize.height + 1, src->rows - y );
    if( y > 0 )
    {
        int temp_ready = ksize.height - 1;
        
        for( i = 0; i < temp_ready; i++ )
            memcpy( temp_ptr + temp_step*i, temp_ptr +
                    temp_step*(temp->rows - temp_ready + i), temp_step );

        temp_ptr += temp_ready*temp_step;
        temp_rows = dy;
        src_y1 = y + temp_ready - anchor.y;
        src_y2 = src_y1 + dy;
        if( src_y1 >= src->rows )
        {
            src_y1 = src->rows - 1;
            src_y2 = src->rows;
        }
    }
    else
    {
        temp_rows = dy + ksize.height - 1;
        src_y2 = temp_rows - anchor.y;
    }

    src_y2 = MIN( src_y2, src->rows );

    stripe_size = cvSize(src->cols, src_y2 - src_y1);
    temp_size = cvSize(temp->cols, temp_rows);
    icvCopyReplicateBorder_8u( src->data.ptr + src_y1*src_step, src_step,
                      stripe_size, temp_ptr, temp_step, temp_size,
                      (y == 0 ? anchor.y : 0), anchor.x, pix_size );
    return dy;
}
// 
// 
// /////////////////////////////// IPP separable filter functions ///////////////////////////

icvFilterRow_8u_C1R_t icvFilterRow_8u_C1R_p = 0;
icvFilterRow_8u_C3R_t icvFilterRow_8u_C3R_p = 0;
icvFilterRow_8u_C4R_t icvFilterRow_8u_C4R_p = 0;
icvFilterRow_16s_C1R_t icvFilterRow_16s_C1R_p = 0;
icvFilterRow_16s_C3R_t icvFilterRow_16s_C3R_p = 0;
icvFilterRow_16s_C4R_t icvFilterRow_16s_C4R_p = 0;
icvFilterRow_32f_C1R_t icvFilterRow_32f_C1R_p = 0;
icvFilterRow_32f_C3R_t icvFilterRow_32f_C3R_p = 0;
icvFilterRow_32f_C4R_t icvFilterRow_32f_C4R_p = 0;
// 
icvFilterColumn_8u_C1R_t icvFilterColumn_8u_C1R_p = 0;
icvFilterColumn_8u_C3R_t icvFilterColumn_8u_C3R_p = 0;
icvFilterColumn_8u_C4R_t icvFilterColumn_8u_C4R_p = 0;
icvFilterColumn_16s_C1R_t icvFilterColumn_16s_C1R_p = 0;
icvFilterColumn_16s_C3R_t icvFilterColumn_16s_C3R_p = 0;
icvFilterColumn_16s_C4R_t icvFilterColumn_16s_C4R_p = 0;
icvFilterColumn_32f_C1R_t icvFilterColumn_32f_C1R_p = 0;
icvFilterColumn_32f_C3R_t icvFilterColumn_32f_C3R_p = 0;
icvFilterColumn_32f_C4R_t icvFilterColumn_32f_C4R_p = 0;
// 
// //////////////////////////////////////////////////////////////////////////////////////////
// 
typedef CvStatus (CV_STDCALL * CvIPPSepFilterFunc)
    ( const void* src, int srcstep, void* dst, int dststep,
      CvSize size, const float* kernel, int ksize, int anchor );

int icvIPPSepFilter( const CvMat* src, CvMat* dst, const CvMat* kernelX,
                     const CvMat* kernelY, CvPoint anchor )
{
    int result = 0;
    
    CvMat* top_bottom = 0;
    CvMat* vout_hin = 0;
    CvMat* dst_buf = 0;
    
    CV_FUNCNAME( "icvIPPSepFilter" );

    __BEGIN__;

    CvSize ksize;
    CvPoint el_anchor;
    CvSize size;
    int type, depth, pix_size;
    int i, x, y, dy = 0, prev_dy = 0, max_dy;
    CvMat vout;
    CvIPPSepFilterFunc x_func = 0, y_func = 0;
    int src_step, top_bottom_step;
    float *kx, *ky;
    int align, stripe_size;

    if( !icvFilterRow_8u_C1R_p )
        EXIT;

    if( !CV_ARE_TYPES_EQ( src, dst ) || !CV_ARE_SIZES_EQ( src, dst ) ||
        !CV_IS_MAT_CONT(kernelX->type & kernelY->type) ||
        CV_MAT_TYPE(kernelX->type) != CV_32FC1 ||
        CV_MAT_TYPE(kernelY->type) != CV_32FC1 ||
        kernelX->cols != 1 && kernelX->rows != 1 ||
        kernelY->cols != 1 && kernelY->rows != 1 ||
        (unsigned)anchor.x >= (unsigned)(kernelX->cols + kernelX->rows - 1) ||
        (unsigned)anchor.y >= (unsigned)(kernelY->cols + kernelY->rows - 1) )
        CV_ERROR( CV_StsError, "Internal Error: incorrect parameters" );

    ksize.width = kernelX->cols + kernelX->rows - 1;
    ksize.height = kernelY->cols + kernelY->rows - 1;

    /*if( ksize.width <= 5 && ksize.height <= 5 )
    {
        float* ker = (float*)cvStackAlloc( ksize.width*ksize.height*sizeof(ker[0]));
        CvMat kernel = cvMat( ksize.height, ksize.width, CV_32F, ker );
        for( y = 0, i = 0; y < ksize.height; y++ )
            for( x = 0; x < ksize.width; x++, i++ )
                ker[i] = kernelY->data.fl[y]*kernelX->data.fl[x];

        CV_CALL( cvFilter2D( src, dst, &kernel, anchor ));
        EXIT;
    }*/

    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    pix_size = CV_ELEM_SIZE(type);

    if( type == CV_8UC1 )
        x_func = icvFilterRow_8u_C1R_p, y_func = icvFilterColumn_8u_C1R_p;
    else if( type == CV_8UC3 )
        x_func = icvFilterRow_8u_C3R_p, y_func = icvFilterColumn_8u_C3R_p;
    else if( type == CV_8UC4 )
        x_func = icvFilterRow_8u_C4R_p, y_func = icvFilterColumn_8u_C4R_p;
    else if( type == CV_16SC1 )
        x_func = icvFilterRow_16s_C1R_p, y_func = icvFilterColumn_16s_C1R_p;
    else if( type == CV_16SC3 )
        x_func = icvFilterRow_16s_C3R_p, y_func = icvFilterColumn_16s_C3R_p;
    else if( type == CV_16SC4 )
        x_func = icvFilterRow_16s_C4R_p, y_func = icvFilterColumn_16s_C4R_p;
    else if( type == CV_32FC1 )
        x_func = icvFilterRow_32f_C1R_p, y_func = icvFilterColumn_32f_C1R_p;
    else if( type == CV_32FC3 )
        x_func = icvFilterRow_32f_C3R_p, y_func = icvFilterColumn_32f_C3R_p;
    else if( type == CV_32FC4 )
        x_func = icvFilterRow_32f_C4R_p, y_func = icvFilterColumn_32f_C4R_p;
    else
        EXIT;

    size = cvGetMatSize(src);
    stripe_size = src->data.ptr == dst->data.ptr ? 1 << 15 : 1 << 16;
    max_dy = MAX( ksize.height - 1, stripe_size/(size.width + ksize.width - 1));
    max_dy = MIN( max_dy, size.height + ksize.height - 1 );
    
    align = 8/CV_ELEM_SIZE(depth);

    CV_CALL( top_bottom = cvCreateMat( ksize.height*2, cvAlign(size.width,align), type ));

    CV_CALL( vout_hin = cvCreateMat( max_dy + ksize.height,
        cvAlign(size.width + ksize.width - 1, align), type ));
    
    if( src->data.ptr == dst->data.ptr && size.height )
        CV_CALL( dst_buf = cvCreateMat( max_dy + ksize.height,
            cvAlign(size.width, align), type ));

    kx = (float*)cvStackAlloc( ksize.width*sizeof(kx[0]) );
    ky = (float*)cvStackAlloc( ksize.height*sizeof(ky[0]) );

    // mirror the kernels
    for( i = 0; i < ksize.width; i++ )
        kx[i] = kernelX->data.fl[ksize.width - i - 1];

    for( i = 0; i < ksize.height; i++ )
        ky[i] = kernelY->data.fl[ksize.height - i - 1];

    el_anchor = cvPoint( ksize.width - anchor.x - 1, ksize.height - anchor.y - 1 );

    cvGetCols( vout_hin, &vout, anchor.x, anchor.x + size.width );

    src_step = src->step ? src->step : CV_STUB_STEP;
    top_bottom_step = top_bottom->step ? top_bottom->step : CV_STUB_STEP;
    vout.step = vout.step ? vout.step : CV_STUB_STEP;

    for( y = 0; y < size.height; y += dy )
    {
        const CvMat *vin = src, *hout = dst;
        int src_y = y, dst_y = y;
        dy = MIN( max_dy, size.height - (ksize.height - anchor.y - 1) - y );

        if( y < anchor.y || dy < anchor.y )
        {
            int ay = anchor.y;
            CvSize src_stripe_size = size;
            
            if( y < anchor.y )
            {
                src_y = 0;
                dy = MIN( anchor.y, size.height );
                src_stripe_size.height = MIN( dy + ksize.height - anchor.y - 1, size.height );
            }
            else
            {
                src_y = MAX( y - anchor.y, 0 );
                dy = size.height - y;
                src_stripe_size.height = MIN( dy + anchor.y, size.height );
                ay = MAX( anchor.y - y, 0 );
            }

            icvCopyReplicateBorder_8u( src->data.ptr + src_y*src_step, src_step,
                src_stripe_size, top_bottom->data.ptr, top_bottom_step,
                cvSize(size.width, dy + ksize.height - 1), ay, 0, pix_size );
            vin = top_bottom;
            src_y = anchor.y;            
        }

        // do vertical convolution
        IPPI_CALL( y_func( vin->data.ptr + src_y*vin->step, vin->step ? vin->step : CV_STUB_STEP,
                           vout.data.ptr, vout.step, cvSize(size.width, dy),
                           ky, ksize.height, el_anchor.y ));

        // now it's time to copy the previously processed stripe to the input/output image
        if( src->data.ptr == dst->data.ptr )
        {
            for( i = 0; i < prev_dy; i++ )
                memcpy( dst->data.ptr + (y - prev_dy + i)*dst->step,
                        dst_buf->data.ptr + i*dst_buf->step, size.width*pix_size );
            if( y + dy < size.height )
            {
                hout = dst_buf;
                dst_y = 0;
            }
        }

        // create a border for every line by replicating the left-most/right-most elements
        for( i = 0; i < dy; i++ )
        {
            uchar* ptr = vout.data.ptr + i*vout.step;
            for( x = -1; x >= -anchor.x*pix_size; x-- )
                ptr[x] = ptr[x + pix_size];
            for( x = size.width*pix_size; x < (size.width+ksize.width-anchor.x-1)*pix_size; x++ )
                ptr[x] = ptr[x - pix_size];
        }

        // do horizontal convolution
        IPPI_CALL( x_func( vout.data.ptr, vout.step, hout->data.ptr + dst_y*hout->step,
                           hout->step ? hout->step : CV_STUB_STEP,
                           cvSize(size.width, dy), kx, ksize.width, el_anchor.x ));
        prev_dy = dy;
    }

    result = 1;

    __END__;

    cvReleaseMat( &vout_hin );
    cvReleaseMat( &dst_buf );
    cvReleaseMat( &top_bottom );

    return result;
}
