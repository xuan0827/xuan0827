#include "stdafx.h"
#include "mmplab_cxcore.h"
// 
// /****************************************************************************************\
// *                                         N o r m                                        *
// \****************************************************************************************/

#define ICV_NORM_CASE( _op_,                \
    _update_op_, worktype, len )            \
                                            \
    for( ; x <= (len) - 4; x += 4 )         \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x+1];           \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
                                            \
        t0 = (src)[x+2];                    \
        t1 = (src)[x+3];                    \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
    }                                       \
                                            \
    for( ; x < (len); x++ )                 \
    {                                       \
        worktype t0 = (src)[x];             \
        t0 = (worktype)_op_(t0);            \
        norm = _update_op_( norm, t0 );     \
    }


#define ICV_NORM_COI_CASE( _op_,            \
    _update_op_, worktype, len, cn )        \
                                            \
    for( ; x < (len); x++ )                 \
    {                                       \
        worktype t0 = (src)[x*(cn)];        \
        t0 = (worktype)_op_(t0);            \
        norm = _update_op_( norm, t0 );     \
    }


#define ICV_NORM_DIFF_CASE( _op_,           \
    _update_op_, worktype, len )            \
                                            \
    for( ; x <= (len) - 4; x += 4 )         \
    {                                       \
        worktype t0 = (src1)[x] - (src2)[x];\
        worktype t1 = (src1)[x+1]-(src2)[x+1];\
                                            \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
                                            \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
                                            \
        t0 = (src1)[x+2] - (src2)[x+2];     \
        t1 = (src1)[x+3] - (src2)[x+3];     \
                                            \
        t0 = _op_(t0);                      \
        t1 = _op_(t1);                      \
                                            \
        norm = _update_op_( norm, t0 );     \
        norm = _update_op_( norm, t1 );     \
    }                                       \
                                            \
    for( ; x < (len); x++ )                 \
    {                                       \
        worktype t0 = (src1)[x] - (src2)[x];\
        t0 = (worktype)_op_(t0);            \
        norm = _update_op_( norm, t0 );     \
    }


#define ICV_NORM_DIFF_COI_CASE( _op_, _update_op_, worktype, len, cn ) \
    for( ; x < (len); x++ )                                     \
    {                                                           \
        worktype t0 = (src1)[x*(cn)] - (src2)[x*(cn)];          \
        t0 = (worktype)_op_(t0);                                \
        norm = _update_op_( norm, t0 );                         \
    }


/*
 	The algorithm and its multiple variations below
    below accumulates the norm by blocks of size "block_size".
    Each block may span across multiple lines and it is
    not necessary aligned by row boundaries. Within a block
    the norm is accumulated to intermediate light-weight
    type (worktype). It really makes sense for 8u, 16s, 16u types
    and L1 & L2 norms, where worktype==int and normtype==int64.
    In other cases a simpler algorithm is used
*/
#define  ICV_DEF_NORM_NOHINT_BLOCK_FUNC_2D( name, _op_, _update_op_, \
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    CvSize size, double* _norm ), (src, step, size, _norm) )    \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_CASE( _op_, _update_op_, worktype, limit );\
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_NOHINT_FUNC_2D( name, _op_, _update_op_,  \
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    CvSize size, double* _norm ), (src, step, size, _norm) )    \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_CASE(_op_, _update_op_, worktype, size.width); \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


/*
   In IPP only 32f flavors of norm functions are with hint.
   For float worktype==normtype==double, thus the block algorithm,
   described above, is not necessary.
 */
#define  ICV_DEF_NORM_HINT_FUNC_2D( name, _op_, _update_op_,    \
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    CvSize size, double* _norm, CvHintAlgorithm /*hint*/ ),     \
    (src, step, size, _norm, cvAlgHintAccurate) )               \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_CASE(_op_, _update_op_, worktype, size.width); \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_NOHINT_BLOCK_FUNC_2D_COI( name, _op_,     \
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
                CvSize size, int cn, int coi, double* _norm )   \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
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
            ICV_NORM_COI_CASE( _op_, _update_op_,               \
                               worktype, limit, cn );           \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_NOHINT_FUNC_2D_COI( name, _op_,           \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
                CvSize size, int cn, int coi, double* _norm )   \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step )                         \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_COI_CASE( _op_, _update_op_,                   \
                           worktype, size.width, cn );          \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}
// 

#define  ICV_DEF_NORM_DIFF_NOHINT_BLOCK_FUNC_2D( name, _op_,    \
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, CvSize size, double* _norm),\
   (src1, step1, src2, step2, size, _norm))                     \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_CASE( _op_, _update_op_,              \
                                worktype, limit );              \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D( name, _op_,          \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, CvSize size, double* _norm),\
    ( src1, step1, src2, step2, size, _norm ))                  \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_CASE( _op_, _update_op_,                  \
                            worktype, size.width );             \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_HINT_FUNC_2D( name, _op_,            \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, CvSize size, double* _norm, \
    CvHintAlgorithm /*hint*/ ),                                 \
    (src1, step1, src2, step2, size, _norm, cvAlgHintAccurate ))\
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_CASE( _op_, _update_op_,                  \
                            worktype, size.width );             \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_NOHINT_BLOCK_FUNC_2D_COI( name, _op_,\
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, CvSize size,                \
    int cn, int coi, double* _norm )                            \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_COI_CASE( _op_, _update_op_,          \
                                    worktype, limit, cn );      \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D_COI( name, _op_,      \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, CvSize size,                \
    int cn, int coi, double* _norm )                            \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2 )        \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_COI_CASE( _op_, _update_op_,              \
                                worktype, size.width, cn );     \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}
// 
// 
// /****************************************************************************************\
// *                             N o r m   with    M A S K                                  *
// \****************************************************************************************/

#define ICV_NORM_MASK_CASE( _op_,               \
        _update_op_, worktype, len )            \
{                                               \
    for( ; x <= (len) - 2; x += 2 )             \
    {                                           \
        worktype t0;                            \
        if( mask[x] )                           \
        {                                       \
            t0 = (src)[x];                      \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
        if( mask[x+1] )                         \
        {                                       \
            t0 = (src)[x+1];                    \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
    }                                           \
                                                \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src)[x];             \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define ICV_NORM_DIFF_MASK_CASE( _op_, _update_op_, worktype, len ) \
{                                               \
    for( ; x <= (len) - 2; x += 2 )             \
    {                                           \
        worktype t0;                            \
        if( mask[x] )                           \
        {                                       \
            t0 = (src1)[x] - (src2)[x];         \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
        if( mask[x+1] )                         \
        {                                       \
            t0 = (src1)[x+1] - (src2)[x+1];     \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
    }                                           \
                                                \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src1)[x] - (src2)[x];\
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define ICV_NORM_MASK_COI_CASE( _op_, _update_op_, worktype, len, cn ) \
{                                               \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src)[x*(cn)];        \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define ICV_NORM_DIFF_MASK_COI_CASE( _op_, _update_op_, worktype, len, cn )\
{                                               \
    for( ; x < (len); x++ )                     \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = (src1)[x*(cn)] - (src2)[x*(cn)];  \
            t0 = _op_(t0);                      \
            norm = _update_op_( norm, t0 );     \
        }                                       \
}


#define  ICV_DEF_NORM_MASK_NOHINT_BLOCK_FUNC_2D( name, _op_,    \
    _update_op_, post_func, arrtype,                            \
    normtype, worktype, block_size )                            \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    const uchar* mask, int maskstep, CvSize size, double* _norm ),\
    (src, step, mask, maskstep, size, _norm) )                  \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_MASK_CASE( _op_, _update_op_,              \
                                worktype, limit );              \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_MASK_NOHINT_FUNC_2D( name, _op_, _update_op_,\
    post_func, arrtype, normtype, worktype, block_size )        \
IPCVAPI_IMPL( CvStatus, name, ( const arrtype* src, int step,   \
    const uchar* mask, int maskstep, CvSize size, double* _norm ),\
    (src, step, mask, maskstep, size, _norm) )                  \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_MASK_CASE( _op_, _update_op_,                  \
                            worktype, size.width );             \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_MASK_NOHINT_BLOCK_FUNC_2D_COI( name, _op_,\
                _update_op_, post_func, arrtype,                \
                normtype, worktype, block_size )                \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
    const uchar* mask, int maskstep, CvSize size,               \
    int cn, int coi, double* _norm )                            \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_MASK_COI_CASE( _op_, _update_op_,          \
                                    worktype, limit, cn );      \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_MASK_NOHINT_FUNC_2D_COI( name, _op_,      \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src, int step,  \
    const uchar* mask, int maskstep, CvSize size,               \
    int cn, int coi, double* _norm )                            \
{                                                               \
    normtype norm = 0;                                          \
    step /= sizeof(src[0]);                                     \
    src += coi - 1;                                             \
                                                                \
    for( ; size.height--; src += step, mask += maskstep )       \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_MASK_COI_CASE( _op_, _update_op_,              \
                                worktype, size.width, cn );     \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}



#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_BLOCK_FUNC_2D( name,     \
    _op_, _update_op_, post_func, arrtype,                      \
    normtype, worktype, block_size )                            \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, double* _norm ),                 \
    (src1, step1, src2, step2, mask, maskstep, size, _norm ))   \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_MASK_CASE( _op_, _update_op_,         \
                                     worktype, limit );         \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D( name, _op_,     \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
IPCVAPI_IMPL( CvStatus, name,( const arrtype* src1, int step1,  \
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, double* _norm ),                 \
    (src1, step1, src2, step2, mask, maskstep, size, _norm ))   \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_MASK_CASE( _op_, _update_op_,             \
                                 worktype, size.width );        \
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_BLOCK_FUNC_2D_COI( name, \
    _op_, _update_op_, post_func, arrtype,                      \
    normtype, worktype, block_size )                            \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, int cn, int coi, double* _norm ) \
{                                                               \
    int remaining = block_size;                                 \
    normtype total_norm = 0;                                    \
    worktype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        while( x < size.width )                                 \
        {                                                       \
            int limit = MIN( remaining, size.width - x );       \
            remaining -= limit;                                 \
            limit += x;                                         \
            ICV_NORM_DIFF_MASK_COI_CASE( _op_, _update_op_,     \
                                    worktype, limit, cn );      \
            if( remaining == 0 )                                \
            {                                                   \
                remaining = block_size;                         \
                total_norm += (normtype)norm;                   \
                norm = 0;                                       \
            }                                                   \
        }                                                       \
    }                                                           \
                                                                \
    total_norm += (normtype)norm;                               \
    *_norm = post_func((double)total_norm);                     \
    return CV_OK;                                               \
}


#define  ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D_COI( name, _op_, \
    _update_op_, post_func,                                     \
    arrtype, normtype, worktype, block_size )                   \
static CvStatus CV_STDCALL name( const arrtype* src1, int step1,\
    const arrtype* src2, int step2, const uchar* mask,          \
    int maskstep, CvSize size, int cn, int coi, double* _norm ) \
{                                                               \
    normtype norm = 0;                                          \
    step1 /= sizeof(src1[0]);                                   \
    step2 /= sizeof(src2[0]);                                   \
    src1 += coi - 1;                                            \
    src2 += coi - 1;                                            \
                                                                \
    for( ; size.height--; src1 += step1, src2 += step2,         \
                          mask += maskstep )                    \
    {                                                           \
        int x = 0;                                              \
        ICV_NORM_DIFF_MASK_COI_CASE( _op_, _update_op_,         \
                                     worktype, size.width, cn );\
    }                                                           \
                                                                \
    *_norm = post_func((double)norm);                           \
    return CV_OK;                                               \
}


// //////////////////////////////////// The macros expanded /////////////////////////////////
// 
// 
#define ICV_DEF_NORM_FUNC_ALL_C(flavor, _abs_, _abs_diff_, arrtype, worktype)\
                                                                            \
ICV_DEF_NORM_NOHINT_FUNC_2D( icvNorm_Inf_##flavor##_C1R,                    \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_NOHINT_FUNC_2D_COI( icvNorm_Inf_##flavor##_CnCR,               \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D( icvNormDiff_Inf_##flavor##_C1R,           \
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )               \
                                                                            \
ICV_DEF_NORM_DIFF_NOHINT_FUNC_2D_COI( icvNormDiff_Inf_##flavor##_CnCR,      \
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )               \
                                                                            \
ICV_DEF_NORM_MASK_NOHINT_FUNC_2D( icvNorm_Inf_##flavor##_C1MR,              \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_MASK_NOHINT_FUNC_2D_COI( icvNorm_Inf_##flavor##_CnCMR,         \
    _abs_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )                    \
                                                                            \
ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D( icvNormDiff_Inf_##flavor##_C1MR,     \
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )               \
                                                                            \
ICV_DEF_NORM_DIFF_MASK_NOHINT_FUNC_2D_COI( icvNormDiff_Inf_##flavor##_CnCMR,\
    _abs_diff_, MAX, CV_NOP, arrtype, worktype, worktype, 0 )


ICV_DEF_NORM_FUNC_ALL_C( 8u, CV_NOP, CV_IABS, uchar, int )
ICV_DEF_NORM_FUNC_ALL_C( 16u, CV_NOP, CV_IABS, ushort, int )
ICV_DEF_NORM_FUNC_ALL_C( 16s, CV_IABS, CV_IABS, short, int )
// there is no protection from overflow
// (otherwise we had to do everything in int64's or double's)
ICV_DEF_NORM_FUNC_ALL_C( 32s, CV_IABS, CV_IABS, int, int )
ICV_DEF_NORM_FUNC_ALL_C( 32f, fabs, fabs, float, double )
ICV_DEF_NORM_FUNC_ALL_C( 64f, fabs, fabs, double, double )

#define ICV_DEF_NORM_FUNC_ALL_L1( flavor, _abs_, _abs_diff_, hintp_func, nohint_func,\
                                  arrtype, normtype, worktype, block_size )         \
                                                                                    \
ICV_DEF_NORM_##hintp_func##_FUNC_2D( icvNorm_L1_##flavor##_C1R,                     \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_##nohint_func##_FUNC_2D_COI( icvNorm_L1_##flavor##_CnCR,               \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_DIFF_##hintp_func##_FUNC_2D( icvNormDiff_L1_##flavor##_C1R,            \
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )           \
                                                                                    \
ICV_DEF_NORM_DIFF_##nohint_func##_FUNC_2D_COI( icvNormDiff_L1_##flavor##_CnCR,      \
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )           \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D( icvNorm_L1_##flavor##_C1MR,              \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D_COI( icvNorm_L1_##flavor##_CnCMR,         \
    _abs_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )                \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D( icvNormDiff_L1_##flavor##_C1MR,     \
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )           \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D_COI( icvNormDiff_L1_##flavor##_CnCMR,\
    _abs_diff_, CV_ADD, CV_NOP, arrtype, normtype, worktype, block_size )


ICV_DEF_NORM_FUNC_ALL_L1( 8u, CV_NOP, CV_IABS, NOHINT_BLOCK, NOHINT_BLOCK,
                          uchar, int64, int, 1 << 23 )
ICV_DEF_NORM_FUNC_ALL_L1( 16u, CV_NOP, CV_IABS, NOHINT_BLOCK, NOHINT_BLOCK,
                          ushort, int64, int, 1 << 15 )
ICV_DEF_NORM_FUNC_ALL_L1( 16s, CV_IABS, CV_IABS, NOHINT_BLOCK, NOHINT_BLOCK,
                          short, int64, int, 1 << 15 )
// there is no protection from overflow on abs() stage.
// (otherwise we had to do everything in int64's or double's)
ICV_DEF_NORM_FUNC_ALL_L1( 32s, fabs, fabs, NOHINT, NOHINT,
                          int, double, double, INT_MAX )
ICV_DEF_NORM_FUNC_ALL_L1( 32f, fabs, fabs, HINT, NOHINT,
                          float, double, double, INT_MAX )
ICV_DEF_NORM_FUNC_ALL_L1( 64f, fabs, fabs, NOHINT, NOHINT,
                          double, double, double, INT_MAX )


#define ICV_DEF_NORM_FUNC_ALL_L2( flavor, hintp_func, nohint_func, arrtype,         \
                                  normtype, worktype, block_size, sqr_macro )       \
                                                                                    \
ICV_DEF_NORM_##hintp_func##_FUNC_2D( icvNorm_L2_##flavor##_C1R,                     \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_##nohint_func##_FUNC_2D_COI( icvNorm_L2_##flavor##_CnCR,               \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_##hintp_func##_FUNC_2D( icvNormDiff_L2_##flavor##_C1R,            \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_##nohint_func##_FUNC_2D_COI( icvNormDiff_L2_##flavor##_CnCR,      \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D( icvNorm_L2_##flavor##_C1MR,              \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_MASK_##nohint_func##_FUNC_2D_COI( icvNorm_L2_##flavor##_CnCMR,         \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D( icvNormDiff_L2_##flavor##_C1MR,     \
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )              \
                                                                                    \
ICV_DEF_NORM_DIFF_MASK_##nohint_func##_FUNC_2D_COI( icvNormDiff_L2_##flavor##_CnCMR,\
    sqr_macro, CV_ADD, sqrt, arrtype, normtype, worktype, block_size )


ICV_DEF_NORM_FUNC_ALL_L2( 8u, NOHINT_BLOCK, NOHINT_BLOCK, uchar,
                          int64, int, 1 << 15, CV_SQR_8U )
ICV_DEF_NORM_FUNC_ALL_L2( 16u, NOHINT, NOHINT, ushort,
                          double, double, INT_MAX, CV_SQR )
ICV_DEF_NORM_FUNC_ALL_L2( 16s, NOHINT, NOHINT, short,
                          double, double, INT_MAX, CV_SQR )
// there is no protection from overflow on abs() stage.
// (otherwise we had to do everything in int64's or double's)
ICV_DEF_NORM_FUNC_ALL_L2( 32s, NOHINT, NOHINT, int,
                          double, double, INT_MAX, CV_SQR )
ICV_DEF_NORM_FUNC_ALL_L2( 32f, HINT, NOHINT, float,
                          double, double, INT_MAX, CV_SQR )
ICV_DEF_NORM_FUNC_ALL_L2( 64f, NOHINT, NOHINT, double,
                          double, double, INT_MAX, CV_SQR )


#define ICV_DEF_INIT_NORM_TAB_2D( FUNCNAME, FLAG )              \
static void icvInit##FUNCNAME##FLAG##Table( CvFuncTable* tab )  \
{                                                               \
    tab->fn_2d[CV_8U] = (void*)icv##FUNCNAME##_8u_##FLAG;       \
    tab->fn_2d[CV_8S] = 0;                                      \
    tab->fn_2d[CV_16U] = (void*)icv##FUNCNAME##_16u_##FLAG;     \
    tab->fn_2d[CV_16S] = (void*)icv##FUNCNAME##_16s_##FLAG;     \
    tab->fn_2d[CV_32S] = (void*)icv##FUNCNAME##_32s_##FLAG;     \
    tab->fn_2d[CV_32F] = (void*)icv##FUNCNAME##_32f_##FLAG;     \
    tab->fn_2d[CV_64F] = (void*)icv##FUNCNAME##_64f_##FLAG;     \
}

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, C1R )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, C1R )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, C1R )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, C1R )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, C1R )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, C1R )

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, CnCR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, CnCR )

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, C1MR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, C1MR )

ICV_DEF_INIT_NORM_TAB_2D( Norm_Inf, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L1, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( Norm_L2, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_Inf, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L1, CnCMR )
ICV_DEF_INIT_NORM_TAB_2D( NormDiff_L2, CnCMR )


static void icvInitNormTabs( CvFuncTable* norm_tab, CvFuncTable* normmask_tab )
{
    icvInitNorm_InfC1RTable( &norm_tab[0] );
    icvInitNorm_L1C1RTable( &norm_tab[1] );
    icvInitNorm_L2C1RTable( &norm_tab[2] );
    icvInitNormDiff_InfC1RTable( &norm_tab[3] );
    icvInitNormDiff_L1C1RTable( &norm_tab[4] );
    icvInitNormDiff_L2C1RTable( &norm_tab[5] );

    icvInitNorm_InfCnCRTable( &norm_tab[6] );
    icvInitNorm_L1CnCRTable( &norm_tab[7] );
    icvInitNorm_L2CnCRTable( &norm_tab[8] );
    icvInitNormDiff_InfCnCRTable( &norm_tab[9] );
    icvInitNormDiff_L1CnCRTable( &norm_tab[10] );
    icvInitNormDiff_L2CnCRTable( &norm_tab[11] );

    icvInitNorm_InfC1MRTable( &normmask_tab[0] );
    icvInitNorm_L1C1MRTable( &normmask_tab[1] );
    icvInitNorm_L2C1MRTable( &normmask_tab[2] );
    icvInitNormDiff_InfC1MRTable( &normmask_tab[3] );
    icvInitNormDiff_L1C1MRTable( &normmask_tab[4] );
    icvInitNormDiff_L2C1MRTable( &normmask_tab[5] );

    icvInitNorm_InfCnCMRTable( &normmask_tab[6] );
    icvInitNorm_L1CnCMRTable( &normmask_tab[7] );
    icvInitNorm_L2CnCMRTable( &normmask_tab[8] );
    icvInitNormDiff_InfCnCMRTable( &normmask_tab[9] );
    icvInitNormDiff_L1CnCMRTable( &normmask_tab[10] );
    icvInitNormDiff_L2CnCMRTable( &normmask_tab[11] );
}
// 
// 
CV_IMPL  double
cvNorm( const void* imgA, const void* imgB, int normType, const void* mask )
{
    static CvFuncTable norm_tab[12];
    static CvFuncTable normmask_tab[12];
    static int inittab = 0;

    double  norm = 0, norm_diff = 0;

    CV_FUNCNAME("cvNorm");

    __BEGIN__;

    int type, depth, cn, is_relative;
    CvSize size;
    CvMat stub1, *mat1 = (CvMat*)imgB;
    CvMat stub2, *mat2 = (CvMat*)imgA;
    int mat2_flag = CV_MAT_CONT_FLAG;
    int mat1_step, mat2_step, mask_step = 0;
    int coi = 0, coi2 = 0;

    if( !mat1 )
    {
        mat1 = mat2;
        mat2 = 0;
    }

    is_relative = mat2 && (normType & CV_RELATIVE);
    normType &= ~CV_RELATIVE;

    switch( normType )
    {
    case CV_C:
    case CV_L1:
    case CV_L2:
    case CV_DIFF_C:
    case CV_DIFF_L1:
    case CV_DIFF_L2:
        normType = (normType & 7) >> 1;
        break;
    default:
        CV_ERROR( CV_StsBadFlag, "" );
    }

    /* light variant */
    if( CV_IS_MAT(mat1) && (!mat2 || CV_IS_MAT(mat2)) && !mask )
    {
        if( mat2 )
        {
            if( !CV_ARE_TYPES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );

            if( !CV_ARE_SIZES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );

            mat2_flag = mat2->type;
        }

        size = cvGetMatSize( mat1 );
        type = CV_MAT_TYPE(mat1->type);
        depth = CV_MAT_DEPTH(type);
        cn = CV_MAT_CN(type);

        if( CV_IS_MAT_CONT( mat1->type & mat2_flag ))
        {
            size.width *= size.height;

            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE && normType == 2 /* CV_L2 */ )
            {
                if( depth == CV_32F )
                {
                    const float* src1data = mat1->data.fl;
                    int size0 = size.width *= cn;
                
                    if( !mat2 || is_relative )
                    {
                        do
                        {
                            double t = src1data[size.width-1];
                            norm += t*t;
                        }
                        while( --size.width );
                    }

                    if( mat2 )
                    {
                        const float* src2data = mat2->data.fl;
                        size.width = size0;

                        do
                        {
                            double t = src1data[size.width-1] - src2data[size.width-1];
                            norm_diff += t*t;
                        }
                        while( --size.width );

                        if( is_relative )
                            norm = norm_diff/(norm + DBL_EPSILON);
                        else
                            norm = norm_diff;
                    }
                    norm = sqrt(norm);
                    EXIT;
                }

                if( depth == CV_64F )
                {
                    const double* src1data = mat1->data.db;
                    int size0 = size.width *= cn;

                    if( !mat2 || is_relative )
                    {
                        do
                        {
                            double t = src1data[size.width-1];
                            norm += t*t;
                        }
                        while( --size.width );
                    }

                    if( mat2 )
                    {
                        const double* src2data = mat2->data.db;
                        size.width = size0;

                        do
                        {
                            double t = src1data[size.width-1] - src2data[size.width-1];
                            norm_diff += t*t;
                        }
                        while( --size.width );

                        if( is_relative )
                            norm = norm_diff/(norm + DBL_EPSILON);
                        else
                            norm = norm_diff;
                    }
                    norm = sqrt(norm);
                    EXIT;
                }
            }
            size.height = 1;
            mat1_step = mat2_step = CV_STUB_STEP;
        }
        else
        {
            mat1_step = mat1->step;
            mat2_step = mat2 ? mat2->step : 0;
        }
    }
    else if( !CV_IS_MATND(mat1) && !CV_IS_MATND(mat2) )
    {
        CV_CALL( mat1 = cvGetMat( mat1, &stub1, &coi ));
        
        if( mat2 )
        {
            CV_CALL( mat2 = cvGetMat( mat2, &stub2, &coi2 ));

            if( !CV_ARE_TYPES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );

            if( !CV_ARE_SIZES_EQ( mat1, mat2 ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );

            if( coi != coi2 && CV_MAT_CN( mat1->type ) > 1 )
                CV_ERROR( CV_BadCOI, "" );

            mat2_flag = mat2->type;
        }

        size = cvGetMatSize( mat1 );
        type = CV_MAT_TYPE(mat1->type);
        depth = CV_MAT_DEPTH(type);
        cn = CV_MAT_CN(type);
        mat1_step = mat1->step;
        mat2_step = mat2 ? mat2->step : 0;

        if( !mask && CV_IS_MAT_CONT( mat1->type & mat2_flag ))
        {
            size.width *= size.height;
            size.height = 1;
            mat1_step = mat2_step = CV_STUB_STEP;
        }
    }
    else
    {
        CvArr* arrs[] = { mat1, mat2 };
        CvMatND stubs[2];
        CvNArrayIterator iterator;
        int pass_hint;

        if( !inittab )
        {
            icvInitNormTabs( norm_tab, normmask_tab );
            inittab = 1;
        }

        if( mask )
            CV_ERROR( CV_StsBadMask,
            "This operation on multi-dimensional arrays does not support mask" );

        CV_CALL( cvInitNArrayIterator( 1 + (mat2 != 0), arrs, 0, stubs, &iterator ));

        type = CV_MAT_TYPE(iterator.hdr[0]->type);
        depth = CV_MAT_DEPTH(type);
        iterator.size.width *= CV_MAT_CN(type);

        pass_hint = normType != 0 && (depth == CV_32F); 

        if( !mat2 || is_relative )
        {
            if( !pass_hint )
            {
                CvFunc2D_1A1P func;
        
                CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P)norm_tab[normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.size, &temp ));
                    norm += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_1A1P1I func;

                CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P1I)norm_tab[normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.size, &temp, cvAlgHintAccurate ));
                    norm += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }
        }

        if( mat2 )
        {
            if( !pass_hint )
            {
                CvFunc2D_2A1P func;
                CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P)norm_tab[3 + normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.size, &temp ));
                    norm_diff += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_2A1P1I func;
                CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P1I)norm_tab[3 + normType].fn_2d[depth]);

                do
                {
                    double temp = 0;
                    IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                     iterator.ptr[1], CV_STUB_STEP,
                                     iterator.size, &temp, cvAlgHintAccurate ));
                    norm_diff += temp;
                }
                while( cvNextNArraySlice( &iterator ));
            }

            if( is_relative )
                norm = norm_diff/(norm + DBL_EPSILON);
            else
                norm = norm_diff;
        }
        EXIT;
    }

    if( !inittab )
    {
        icvInitNormTabs( norm_tab, normmask_tab );
        inittab = 1;
    }

    if( !mask )
    {
        if( cn == 1 || coi == 0 )
        {
            int pass_hint = depth == CV_32F && normType != 0;
            size.width *= cn;

            if( !mat2 || is_relative )
            {
                if( !pass_hint )
                {
                    CvFunc2D_1A1P func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P)norm_tab[normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, size, &norm ));
                }
                else
                {
                    CvFunc2D_1A1P1I func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_1A1P1I)norm_tab[normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, size, &norm, cvAlgHintAccurate ));
                }
            }
        
            if( mat2 )
            {
                if( !pass_hint )
                {
                    CvFunc2D_2A1P func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P)norm_tab[3 + normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                     size, &norm_diff ));
                }
                else
                {
                    CvFunc2D_2A1P1I func;
                    CV_GET_FUNC_PTR( func, (CvFunc2D_2A1P1I)norm_tab[3 + normType].fn_2d[depth]);

                    IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                     size, &norm_diff, cvAlgHintAccurate ));
                }

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
        else
        {
            if( !mat2 || is_relative )
            {
                CvFunc2DnC_1A1P func;
                CV_GET_FUNC_PTR( func, (CvFunc2DnC_1A1P)norm_tab[6 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step, size, cn, coi, &norm ));
            }
        
            if( mat2 )
            {
                CvFunc2DnC_2A1P func;
                CV_GET_FUNC_PTR( func, (CvFunc2DnC_2A1P)norm_tab[9 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                 size, cn, coi, &norm_diff ));

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
    }
    else
    {
        CvMat maskstub, *matmask = (CvMat*)mask;

        if( CV_MAT_CN(type) > 1 && coi == 0 )
            CV_ERROR( CV_StsBadArg, "" );

        CV_CALL( matmask = cvGetMat( matmask, &maskstub ));

        if( !CV_IS_MASK_ARR( matmask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat1, matmask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        
        mask_step = matmask->step;

        if( CV_IS_MAT_CONT( mat1->type & mat2_flag & matmask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            mat1_step = mat2_step = mask_step = CV_STUB_STEP;
        }

        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            if( !mat2 || is_relative )
            {
                CvFunc2D_2A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2D_2A1P)normmask_tab[normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step,
                                 matmask->data.ptr, mask_step, size, &norm ));
            }
        
            if( mat2 )
            {
                CvFunc2D_3A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2D_3A1P)normmask_tab[3 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step, mat2->data.ptr, mat2_step,
                                 matmask->data.ptr, mask_step, size, &norm_diff ));

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
        else
        {
            if( !mat2 || is_relative )
            {
                CvFunc2DnC_2A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2DnC_2A1P)normmask_tab[6 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step,
                                 matmask->data.ptr, mask_step,
                                 size, cn, coi, &norm ));
            }
        
            if( mat2 )
            {
                CvFunc2DnC_3A1P func;
                CV_GET_FUNC_PTR( func,
                    (CvFunc2DnC_3A1P)normmask_tab[9 + normType].fn_2d[depth]);

                IPPI_CALL( func( mat1->data.ptr, mat1_step,
                                 mat2->data.ptr, mat2_step,
                                 matmask->data.ptr, mask_step,
                                 size, cn, coi, &norm_diff ));

                if( is_relative )
                    norm = norm_diff/(norm + DBL_EPSILON);
                else
                    norm = norm_diff;
            }
        }
    }

    __END__;

    return norm;
}
// 
// /* End of file. */

/****************************************************************************************\
*                             Mean and StdDev calculation                                *
\****************************************************************************************/

#define ICV_MEAN_SDV_COI_CASE( worktype, sqsumtype, \
                               sqr_macro, len, cn ) \
    for( ; x <= (len) - 4*(cn); x += 4*(cn))\
    {                                       \
        worktype t0 = src[x];               \
        worktype t1 = src[x + (cn)];        \
                                            \
        s0  += t0 + t1;                     \
        sq0 += (sqsumtype)(sqr_macro(t0)) + \
               (sqsumtype)(sqr_macro(t1));  \
                                            \
        t0 = src[x + 2*(cn)];               \
        t1 = src[x + 3*(cn)];               \
                                            \
        s0  += t0 + t1;                     \
        sq0 += (sqsumtype)(sqr_macro(t0)) + \
               (sqsumtype)(sqr_macro(t1));  \
    }                                       \
                                            \
    for( ; x < (len); x += (cn) )           \
    {                                       \
        worktype t0 = src[x];               \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
    }


#define ICV_MEAN_SDV_CASE_C1( worktype, sqsumtype, sqr_macro, len ) \
    ICV_MEAN_SDV_COI_CASE( worktype, sqsumtype, sqr_macro, len, 1 )


#define ICV_MEAN_SDV_CASE_C2( worktype, sqsumtype, \
                              sqr_macro, len ) \
    for( ; x < (len); x += 2 )              \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x + 1];         \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
        s1 += t1;                           \
        sq1 += (sqsumtype)(sqr_macro(t1));  \
    }


#define ICV_MEAN_SDV_CASE_C3( worktype, sqsumtype, \
                              sqr_macro, len ) \
    for( ; x < (len); x += 3 )              \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x + 1];         \
        worktype t2 = (src)[x + 2];         \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
        s1 += t1;                           \
        sq1 += (sqsumtype)(sqr_macro(t1));  \
        s2 += t2;                           \
        sq2 += (sqsumtype)(sqr_macro(t2));  \
    }


#define ICV_MEAN_SDV_CASE_C4( worktype, sqsumtype, \
                              sqr_macro, len ) \
    for( ; x < (len); x += 4 )              \
    {                                       \
        worktype t0 = (src)[x];             \
        worktype t1 = (src)[x + 1];         \
                                            \
        s0 += t0;                           \
        sq0 += (sqsumtype)(sqr_macro(t0));  \
        s1 += t1;                           \
        sq1 += (sqsumtype)(sqr_macro(t1));  \
                                            \
        t0 = (src)[x + 2];                  \
        t1 = (src)[x + 3];                  \
                                            \
        s2 += t0;                           \
        sq2 += (sqsumtype)(sqr_macro(t0));  \
        s3 += t1;                           \
        sq3 += (sqsumtype)(sqr_macro(t1));  \
    }


#define ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqsumtype, \
                                    sqr_macro, len, cn ) \
    for( ; x <= (len) - 4; x += 4 )             \
    {                                           \
        worktype t0;                            \
        if( mask[x] )                           \
        {                                       \
            t0 = src[x*(cn)]; pix++;            \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
                                                \
        if( mask[x+1] )                         \
        {                                       \
            t0 = src[(x+1)*(cn)]; pix++;        \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
                                                \
        if( mask[x+2] )                         \
        {                                       \
            t0 = src[(x+2)*(cn)]; pix++;        \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
                                                \
        if( mask[x+3] )                         \
        {                                       \
            t0 = src[(x+3)*(cn)]; pix++;        \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
    }                                           \
                                                \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*(cn)]; pix++;   \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
        }                                       \
    }


#define ICV_MEAN_SDV_MASK_CASE_C1( worktype, sqsumtype, sqr_macro, len )    \
    ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqsumtype, sqr_macro, len, 1 )


#define ICV_MEAN_SDV_MASK_CASE_C2( worktype, sqsumtype,\
                                   sqr_macro, len )    \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*2];             \
            worktype t1 = src[x*2+1];           \
            pix++;                              \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
            s1 += t1;                           \
            sq1 += sqsumtype(sqr_macro(t1));    \
        }                                       \
    }


#define ICV_MEAN_SDV_MASK_CASE_C3( worktype, sqsumtype,\
                                   sqr_macro, len )    \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*3];             \
            worktype t1 = src[x*3+1];           \
            worktype t2 = src[x*3+2];           \
            pix++;                              \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
            s1 += t1;                           \
            sq1 += sqsumtype(sqr_macro(t1));    \
            s2 += t2;                           \
            sq2 += sqsumtype(sqr_macro(t2));    \
        }                                       \
    }


#define ICV_MEAN_SDV_MASK_CASE_C4( worktype, sqsumtype,\
                                   sqr_macro, len )    \
    for( ; x < (len); x++ )                     \
    {                                           \
        if( mask[x] )                           \
        {                                       \
            worktype t0 = src[x*4];             \
            worktype t1 = src[x*4+1];           \
            pix++;                              \
            s0 += t0;                           \
            sq0 += sqsumtype(sqr_macro(t0));    \
            s1 += t1;                           \
            sq1 += sqsumtype(sqr_macro(t1));    \
            t0 = src[x*4+2];                    \
            t1 = src[x*4+3];                    \
            s2 += t0;                           \
            sq2 += sqsumtype(sqr_macro(t0));    \
            s3 += t1;                           \
            sq3 += sqsumtype(sqr_macro(t1));    \
        }                                       \
    }


////////////////////////////////////// entry macros //////////////////////////////////////

#define ICV_MEAN_SDV_ENTRY_COMMON()                 \
    int pix;                                        \
    double scale, tmp;                              \
    step /= sizeof(src[0])

#define ICV_MEAN_SDV_ENTRY_C1( sumtype, sqsumtype ) \
    sumtype s0 = 0;                                 \
    sqsumtype sq0 = 0;                              \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_C2( sumtype, sqsumtype ) \
    sumtype s0 = 0, s1 = 0;                         \
    sqsumtype sq0 = 0, sq1 = 0;                     \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_C3( sumtype, sqsumtype ) \
    sumtype s0 = 0, s1 = 0, s2 = 0;                 \
    sqsumtype sq0 = 0, sq1 = 0, sq2 = 0;            \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_C4( sumtype, sqsumtype ) \
    sumtype s0 = 0, s1 = 0, s2 = 0, s3 = 0;         \
    sqsumtype sq0 = 0, sq1 = 0, sq2 = 0, sq3 = 0;   \
    ICV_MEAN_SDV_ENTRY_COMMON()


#define ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )   \
    int remaining = block_size;                         \
    ICV_MEAN_SDV_ENTRY_COMMON()

#define ICV_MEAN_SDV_ENTRY_BLOCK_C1( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0;                                           \
    sqsumtype sqsum0 = 0;                                       \
    worktype s0 = 0;                                            \
    sqworktype sq0 = 0;                                         \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_SDV_ENTRY_BLOCK_C2( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0, sum1 = 0;                                 \
    sqsumtype sqsum0 = 0, sqsum1 = 0;                           \
    worktype s0 = 0, s1 = 0;                                    \
    sqworktype sq0 = 0, sq1 = 0;                                \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_SDV_ENTRY_BLOCK_C3( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0, sum1 = 0, sum2 = 0;                       \
    sqsumtype sqsum0 = 0, sqsum1 = 0, sqsum2 = 0;               \
    worktype s0 = 0, s1 = 0, s2 = 0;                            \
    sqworktype sq0 = 0, sq1 = 0, sq2 = 0;                       \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )

#define ICV_MEAN_SDV_ENTRY_BLOCK_C4( sumtype, sqsumtype,        \
                        worktype, sqworktype, block_size )      \
    sumtype sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;             \
    sqsumtype sqsum0 = 0, sqsum1 = 0, sqsum2 = 0, sqsum3 = 0;   \
    worktype s0 = 0, s1 = 0, s2 = 0, s3 = 0;                    \
    sqworktype sq0 = 0, sq1 = 0, sq2 = 0, sq3 = 0;              \
    ICV_MEAN_SDV_ENTRY_BLOCK_COMMON( block_size )


/////////////////////////////////////// exit macros //////////////////////////////////////

#define ICV_MEAN_SDV_EXIT_COMMON()              \
    scale = pix ? 1./pix : 0

#define ICV_MEAN_SDV_EXIT_CN( total, sqtotal, idx ) \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    mean[idx] = tmp = scale*(double)total##idx; \
    tmp = scale*(double)sqtotal##idx - tmp*tmp; \
    sdv[idx] = sqrt(MAX(tmp,0.))

#define ICV_MEAN_SDV_EXIT_C1( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 )

#define ICV_MEAN_SDV_EXIT_C2( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 1 )

#define ICV_MEAN_SDV_EXIT_C3( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 1 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 2 )

#define ICV_MEAN_SDV_EXIT_C4( total, sqtotal )  \
    ICV_MEAN_SDV_EXIT_COMMON();                 \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 0 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 1 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 2 );  \
    ICV_MEAN_SDV_EXIT_CN( total, sqtotal, 3 )

////////////////////////////////////// update macros /////////////////////////////////////

#define ICV_MEAN_SDV_UPDATE_COMMON( block_size )\
    remaining = block_size

#define ICV_MEAN_SDV_UPDATE_C1( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    s0 = 0; sq0 = 0

#define ICV_MEAN_SDV_UPDATE_C2( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    sum1 += s1; sqsum1 += sq1;                  \
    s0 = s1 = 0; sq0 = sq1 = 0

#define ICV_MEAN_SDV_UPDATE_C3( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    sum1 += s1; sqsum1 += sq1;                  \
    sum2 += s2; sqsum2 += sq2;                  \
    s0 = s1 = s2 = 0; sq0 = sq1 = sq2 = 0

#define ICV_MEAN_SDV_UPDATE_C4( block_size )    \
    ICV_MEAN_SDV_UPDATE_COMMON( block_size );   \
    sum0 += s0; sqsum0 += sq0;                  \
    sum1 += s1; sqsum1 += sq1;                  \
    sum2 += s2; sqsum2 += sq2;                  \
    sum3 += s3; sqsum3 += sq3;                  \
    s0 = s1 = s2 = s3 = 0; sq0 = sq1 = sq2 = sq3 = 0



#define ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, cn, arrtype,        \
                                sumtype, sqsumtype, worktype,       \
                                sqworktype, block_size, sqr_macro ) \
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##R,         \
                        ( const arrtype* src, int step,             \
                          CvSize size, double* mean, double* sdv ), \
                          (src, step, size, mean, sdv) )            \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C##cn( sumtype, sqsumtype,             \
                worktype, sqworktype, (block_size)*(cn) );          \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_CASE_C##cn( worktype, sqworktype,          \
                                     sqr_macro, limit );            \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C##cn( (block_size)*(cn) );     \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C##cn(0);                                   \
    ICV_MEAN_SDV_EXIT_C##cn( sum, sqsum );                          \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_FUNC_2D( flavor, cn, arrtype,              \
                                  sumtype, sqsumtype, worktype )    \
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##R,         \
                        ( const arrtype* src, int step,             \
                          CvSize size, double* mean, double* sdv ), \
                          (src, step, size, mean, sdv) )            \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C##cn( sumtype, sqsumtype );                 \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_CASE_C##cn( worktype, sqsumtype,               \
                                 CV_SQR, size.width );              \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C##cn( s, sq );                               \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D_COI( flavor, arrtype,        \
                                sumtype, sqsumtype, worktype,       \
                                sqworktype, block_size, sqr_macro ) \
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCR           \
                        ( const arrtype* src, int step,             \
                          CvSize size, int cn, int coi,             \
                          double* mean, double* sdv )               \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C1( sumtype, sqsumtype,                \
                worktype, sqworktype, (block_size)*(cn) );          \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_COI_CASE( worktype, sqworktype,            \
                                   sqr_macro, limit, cn);           \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C1( (block_size)*(cn) );        \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C1(0);                                      \
    ICV_MEAN_SDV_EXIT_C1( sum, sqsum );                             \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_FUNC_2D_COI( flavor, arrtype,              \
                                      sumtype, sqsumtype, worktype )\
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCR           \
                        ( const arrtype* src, int step, CvSize size,\
                        int cn, int coi, double* mean, double* sdv )\
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C1( sumtype, sqsumtype );                    \
    pix = size.width * size.height;                                 \
    size.width *= (cn);                                             \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step )                             \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_COI_CASE( worktype, sqsumtype,                 \
                               CV_SQR, size.width, cn );            \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C1( s, sq );                                  \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, cn,            \
                        arrtype, sumtype, sqsumtype, worktype,      \
                        sqworktype, block_size, sqr_macro )         \
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##MR,        \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, double* mean, double* sdv ), \
                       (src, step, mask, maskstep, size, mean, sdv))\
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C##cn( sumtype, sqsumtype,             \
                    worktype, sqworktype, block_size );             \
    pix = 0;                                                        \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_MASK_CASE_C##cn( worktype, sqworktype,     \
                                          sqr_macro, limit );       \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C##cn( block_size );            \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C##cn(0);                                   \
    ICV_MEAN_SDV_EXIT_C##cn( sum, sqsum );                          \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_FUNC_2D( flavor, cn, arrtype,         \
                                       sumtype, sqsumtype, worktype)\
IPCVAPI_IMPL( CvStatus, icvMean_StdDev_##flavor##_C##cn##MR,        \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, double* mean, double* sdv ), \
                       (src, step, mask, maskstep, size, mean, sdv))\
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C##cn( sumtype, sqsumtype );                 \
    pix = 0;                                                        \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_MASK_CASE_C##cn( worktype, sqsumtype,          \
                                      CV_SQR, size.width );         \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C##cn( s, sq );                               \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D_COI( flavor,            \
                            arrtype, sumtype, sqsumtype, worktype,  \
                            sqworktype, block_size, sqr_macro )     \
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCMR          \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, int cn, int coi,             \
                          double* mean, double* sdv )               \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_BLOCK_C1( sumtype, sqsumtype,                \
                    worktype, sqworktype, block_size );             \
    pix = 0;                                                        \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        while( x < size.width )                                     \
        {                                                           \
            int limit = MIN( remaining, size.width - x );           \
            remaining -= limit;                                     \
            limit += x;                                             \
            ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqworktype,       \
                                        sqr_macro, limit, cn );     \
            if( remaining == 0 )                                    \
            {                                                       \
                ICV_MEAN_SDV_UPDATE_C1( block_size );               \
            }                                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_UPDATE_C1(0);                                      \
    ICV_MEAN_SDV_EXIT_C1( sum, sqsum );                             \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_MASK_FUNC_2D_COI( flavor, arrtype,         \
                                    sumtype, sqsumtype, worktype )  \
static CvStatus CV_STDCALL icvMean_StdDev_##flavor##_CnCMR          \
                        ( const arrtype* src, int step,             \
                          const uchar* mask, int maskstep,          \
                          CvSize size, int cn, int coi,             \
                          double* mean, double* sdv )               \
{                                                                   \
    ICV_MEAN_SDV_ENTRY_C1( sumtype, sqsumtype );                    \
    pix = 0;                                                        \
    src += coi - 1;                                                 \
                                                                    \
    for( ; size.height--; src += step, mask += maskstep )           \
    {                                                               \
        int x = 0;                                                  \
        ICV_MEAN_SDV_MASK_COI_CASE( worktype, sqsumtype,            \
                                    CV_SQR, size.width, cn );       \
    }                                                               \
                                                                    \
    ICV_MEAN_SDV_EXIT_C1( s, sq );                                  \
    return CV_OK;                                                   \
}


#define ICV_DEF_MEAN_SDV_BLOCK_ALL( flavor, arrtype, sumtype, sqsumtype,\
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 1, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 2, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 3, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D( flavor, 4, arrtype, sumtype, sqsumtype, \
                            worktype, sqworktype, block_size, sqr_macro)\
ICV_DEF_MEAN_SDV_BLOCK_FUNC_2D_COI( flavor, arrtype, sumtype, sqsumtype,\
                            worktype, sqworktype, block_size, sqr_macro)\
                                                                        \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 1, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 2, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 3, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D( flavor, 4, arrtype, sumtype,       \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )    \
ICV_DEF_MEAN_SDV_MASK_BLOCK_FUNC_2D_COI( flavor, arrtype, sumtype,      \
            sqsumtype, worktype, sqworktype, block_size, sqr_macro )

#define ICV_DEF_MEAN_SDV_ALL( flavor, arrtype, sumtype, sqsumtype, worktype )   \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 1, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 2, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 3, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D( flavor, 4, arrtype, sumtype, sqsumtype, worktype )    \
ICV_DEF_MEAN_SDV_FUNC_2D_COI( flavor, arrtype, sumtype, sqsumtype, worktype )   \
                                                                                \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 1, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 2, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 3, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D(flavor, 4, arrtype, sumtype, sqsumtype, worktype) \
ICV_DEF_MEAN_SDV_MASK_FUNC_2D_COI( flavor, arrtype, sumtype, sqsumtype, worktype )


ICV_DEF_MEAN_SDV_BLOCK_ALL( 8u, uchar, int64, int64, unsigned, unsigned, 1 << 16, CV_SQR_8U )
ICV_DEF_MEAN_SDV_BLOCK_ALL( 16u, ushort, int64, int64, unsigned, int64, 1 << 16, CV_SQR )
ICV_DEF_MEAN_SDV_BLOCK_ALL( 16s, short, int64, int64, int, int64, 1 << 16, CV_SQR )

ICV_DEF_MEAN_SDV_ALL( 32s, int, double, double, double )
ICV_DEF_MEAN_SDV_ALL( 32f, float, double, double, double )
ICV_DEF_MEAN_SDV_ALL( 64f, double, double, double, double )

#define icvMean_StdDev_8s_C1R  0
#define icvMean_StdDev_8s_C2R  0
#define icvMean_StdDev_8s_C3R  0
#define icvMean_StdDev_8s_C4R  0
#define icvMean_StdDev_8s_CnCR 0

#define icvMean_StdDev_8s_C1MR  0
#define icvMean_StdDev_8s_C2MR  0
#define icvMean_StdDev_8s_C3MR  0
#define icvMean_StdDev_8s_C4MR  0
#define icvMean_StdDev_8s_CnCMR 0

CV_DEF_INIT_BIG_FUNC_TAB_2D( Mean_StdDev, R )
CV_DEF_INIT_FUNC_TAB_2D( Mean_StdDev, CnCR )
CV_DEF_INIT_BIG_FUNC_TAB_2D( Mean_StdDev, MR )
CV_DEF_INIT_FUNC_TAB_2D( Mean_StdDev, CnCMR )

CV_IMPL  void
cvAvgSdv( const CvArr* img, CvScalar* _mean, CvScalar* _sdv, const void* mask )
{
    CvScalar mean = {{0,0,0,0}};
    CvScalar sdv = {{0,0,0,0}};

    static CvBigFuncTable meansdv_tab;
    static CvFuncTable meansdvcoi_tab;
    static CvBigFuncTable meansdvmask_tab;
    static CvFuncTable meansdvmaskcoi_tab;
    static int inittab = 0;

    CV_FUNCNAME("cvMean_StdDev");

    __BEGIN__;

    int type, coi = 0;
    int mat_step, mask_step = 0;
    CvSize size;
    CvMat stub, maskstub, *mat = (CvMat*)img, *matmask = (CvMat*)mask;

    if( !inittab )
    {
        icvInitMean_StdDevRTable( &meansdv_tab );
        icvInitMean_StdDevCnCRTable( &meansdvcoi_tab );
        icvInitMean_StdDevMRTable( &meansdvmask_tab );
        icvInitMean_StdDevCnCMRTable( &meansdvmaskcoi_tab );
        inittab = 1;
    }

    if( !CV_IS_MAT(mat) )
        CV_CALL( mat = cvGetMat( mat, &stub, &coi ));

    type = CV_MAT_TYPE( mat->type );

    if( CV_MAT_CN(type) > 4 && coi == 0 )
        CV_ERROR( CV_StsOutOfRange, "The input array must have at most 4 channels unless COI is set" );

    size = cvGetMatSize( mat );
    mat_step = mat->step;

    if( !mask )
    {
        if( CV_IS_MAT_CONT( mat->type ))
        {
            size.width *= size.height;
            size.height = 1;
            mat_step = CV_STUB_STEP;
        }

        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            CvFunc2D_1A2P func = (CvFunc2D_1A2P)(meansdv_tab.fn_2d[type]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size, mean.val, sdv.val ));
        }
        else
        {
            CvFunc2DnC_1A2P func = (CvFunc2DnC_1A2P)
                (meansdvcoi_tab.fn_2d[CV_MAT_DEPTH(type)]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, size,
                             CV_MAT_CN(type), coi, mean.val, sdv.val ));
        }
    }
    else
    {
        CV_CALL( matmask = cvGetMat( matmask, &maskstub ));

        mask_step = matmask->step;

        if( !CV_IS_MASK_ARR( matmask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mat, matmask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        if( CV_IS_MAT_CONT( mat->type & matmask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            mat_step = mask_step = CV_STUB_STEP;
        }

        if( CV_MAT_CN(type) == 1 || coi == 0 )
        {
            CvFunc2D_2A2P func = (CvFunc2D_2A2P)(meansdvmask_tab.fn_2d[type]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step, matmask->data.ptr,
                             mask_step, size, mean.val, sdv.val ));
        }
        else
        {
            CvFunc2DnC_2A2P func = (CvFunc2DnC_2A2P)
                (meansdvmaskcoi_tab.fn_2d[CV_MAT_DEPTH(type)]);

            if( !func )
                CV_ERROR( CV_StsBadArg, cvUnsupportedFormat );

            IPPI_CALL( func( mat->data.ptr, mat_step,
                             matmask->data.ptr, mask_step,
                             size, CV_MAT_CN(type), coi, mean.val, sdv.val ));
        }
    }

    __END__;

    if( _mean )
        *_mean = mean;

    if( _sdv )
        *_sdv = sdv;
}

