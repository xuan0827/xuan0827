#include "stdafx.h"
#include "mmplab_cxcore.h"
//////////////////////////////////////////////////////////////////////////
//CVARRAY
//////////////////////////////////////////////////////////////////////////
static struct
{
    Cv_iplCreateImageHeader  createHeader;
    Cv_iplAllocateImageData  allocateData;
    Cv_iplDeallocate  deallocate;
    Cv_iplCreateROI  createROI;
    Cv_iplCloneImage  cloneImage;
}
CvIPL;

// /****************************************************************************************\
// *                               CvMat creation and basic operations                      *
// \****************************************************************************************/
// 
// // Creates CvMat and underlying data
CV_IMPL CvMat*
cvCreateMat( int height, int width, int type )
{
    CvMat* arr = 0;

    CV_FUNCNAME( "cvCreateMat" );
    
    __BEGIN__;

    CV_CALL( arr = cvCreateMatHeader( height, width, type ));
    CV_CALL( cvCreateData( arr ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMat( &arr );

    return arr;
}
// 
// 
static void icvCheckHuge( CvMat* arr )
{
    if( (int64)arr->step*arr->rows > INT_MAX )
        arr->type &= ~CV_MAT_CONT_FLAG;
}
// 
// // Creates CvMat header only
CV_IMPL CvMat*
cvCreateMatHeader( int rows, int cols, int type )
{
    CvMat* arr = 0;
    
    CV_FUNCNAME( "cvCreateMatHeader" );

    __BEGIN__;

    int min_step;
    type = CV_MAT_TYPE(type);

    if( rows <= 0 || cols <= 0 )
        CV_ERROR( CV_StsBadSize, "Non-positive width or height" );

    min_step = CV_ELEM_SIZE(type)*cols;
    if( min_step <= 0 )
        CV_ERROR( CV_StsUnsupportedFormat, "Invalid matrix type" );

    CV_CALL( arr = (CvMat*)cvAlloc( sizeof(*arr)));

    arr->step = rows == 1 ? 0 : cvAlign(min_step, CV_DEFAULT_MAT_ROW_ALIGN);
    arr->type = CV_MAT_MAGIC_VAL | type |
                (arr->step == 0 || arr->step == min_step ? CV_MAT_CONT_FLAG : 0);
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = 0;
    arr->refcount = 0;
    arr->hdr_refcount = 1;

    icvCheckHuge( arr );

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMat( &arr );

    return arr;
}
// 
// 
// // Initializes CvMat header, allocated by the user
CV_IMPL CvMat*
cvInitMatHeader( CvMat* arr, int rows, int cols,
                 int type, void* data, int step )
{
    CV_FUNCNAME( "cvInitMatHeader" );
    
    __BEGIN__;

    int mask, pix_size, min_step;

    if( !arr )
        CV_ERROR_FROM_CODE( CV_StsNullPtr );

    if( (unsigned)CV_MAT_DEPTH(type) > CV_DEPTH_MAX )
        CV_ERROR_FROM_CODE( CV_BadNumChannels );

    if( rows <= 0 || cols <= 0 )
        CV_ERROR( CV_StsBadSize, "Non-positive cols or rows" );
 
    type = CV_MAT_TYPE( type );
    arr->type = type | CV_MAT_MAGIC_VAL;
    arr->rows = rows;
    arr->cols = cols;
    arr->data.ptr = (uchar*)data;
    arr->refcount = 0;
    arr->hdr_refcount = 0;

    mask = (arr->rows <= 1) - 1;
    pix_size = CV_ELEM_SIZE(type);
    min_step = arr->cols*pix_size & mask;

    if( step != CV_AUTOSTEP && step != 0 )
    {
        if( step < min_step )
            CV_ERROR_FROM_CODE( CV_BadStep );
        arr->step = step & mask;
    }
    else
    {
        arr->step = min_step;
    }

    arr->type = CV_MAT_MAGIC_VAL | type |
                (arr->step == min_step ? CV_MAT_CONT_FLAG : 0);

    icvCheckHuge( arr );

    __END__;

    return arr;
}
// 
// 
// // Deallocates the CvMat structure and underlying data
CV_IMPL void
cvReleaseMat( CvMat** array )
{
    CV_FUNCNAME( "cvReleaseMat" );
    
    __BEGIN__;

    if( !array )
        CV_ERROR_FROM_CODE( CV_HeaderIsNull );

    if( *array )
    {
        CvMat* arr = *array;
        
        if( !CV_IS_MAT_HDR(arr) && !CV_IS_MATND_HDR(arr) )
            CV_ERROR_FROM_CODE( CV_StsBadFlag );

        *array = 0;

        cvDecRefData( arr );
        cvFree( &arr );
    }

    __END__;
}

// 
// // Creates a copy of matrix
CV_IMPL CvMat*
cvCloneMat( const CvMat* src )
{
    CvMat* dst = 0;
    CV_FUNCNAME( "cvCloneMat" );

    __BEGIN__;

    if( !CV_IS_MAT_HDR( src ))
        CV_ERROR( CV_StsBadArg, "Bad CvMat header" );

    CV_CALL( dst = cvCreateMatHeader( src->rows, src->cols, src->type ));

    if( src->data.ptr )
    {
        CV_CALL( cvCreateData( dst ));
        CV_CALL( cvCopy( src, dst ));
    }

    __END__;

    return dst;
}
// 
// 
// /****************************************************************************************\
// *                               CvMatND creation and basic operations                    *
// \****************************************************************************************/
// 
CV_IMPL CvMatND*
cvInitMatNDHeader( CvMatND* mat, int dims, const int* sizes,
                    int type, void* data )
{
    CvMatND* result = 0;

    CV_FUNCNAME( "cvInitMatNDHeader" );

    __BEGIN__;

    type = CV_MAT_TYPE(type);
    int i;
    int64 step = CV_ELEM_SIZE(type);

    if( !mat )
        CV_ERROR( CV_StsNullPtr, "NULL matrix header pointer" );

    if( step == 0 )
        CV_ERROR( CV_StsUnsupportedFormat, "invalid array data type" );

    if( !sizes )
        CV_ERROR( CV_StsNullPtr, "NULL <sizes> pointer" );

    if( dims <= 0 || dims > CV_MAX_DIM )
        CV_ERROR( CV_StsOutOfRange,
        "non-positive or too large number of dimensions" );

    for( i = dims - 1; i >= 0; i-- )
    {
        if( sizes[i] <= 0 )
            CV_ERROR( CV_StsBadSize, "one of dimesion sizes is non-positive" );
        mat->dim[i].size = sizes[i];
        if( step > INT_MAX )
            CV_ERROR( CV_StsOutOfRange, "The array is too big" );
        mat->dim[i].step = (int)step;
        step *= sizes[i];
    }

    mat->type = CV_MATND_MAGIC_VAL | (step <= INT_MAX ? CV_MAT_CONT_FLAG : 0) | type;
    mat->dims = dims;
    mat->data.ptr = (uchar*)data;
    mat->refcount = 0;
    mat->hdr_refcount = 0;
    result = mat;

    __END__;

    if( cvGetErrStatus() < 0 && mat )
    {
        mat->type = 0;
        mat->data.ptr = 0;
    }

    return result;
}

// 
// // Creates CvMatND and underlying data
CV_IMPL CvMatND*
cvCreateMatND( int dims, const int* sizes, int type )
{
    CvMatND* arr = 0;

    CV_FUNCNAME( "cvCreateMatND" );
    
    __BEGIN__;

    CV_CALL( arr = cvCreateMatNDHeader( dims, sizes, type ));
    CV_CALL( cvCreateData( arr ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMatND( &arr );

    return arr;
}
// 
// 
// // Creates CvMatND header only
CV_IMPL CvMatND*
cvCreateMatNDHeader( int dims, const int* sizes, int type )
{
    CvMatND* arr = 0;
    
    CV_FUNCNAME( "cvCreateMatNDHeader" );

    __BEGIN__;

    if( dims <= 0 || dims > CV_MAX_DIM )
        CV_ERROR( CV_StsOutOfRange,
        "non-positive or too large number of dimensions" );

    CV_CALL( arr = (CvMatND*)cvAlloc( sizeof(*arr) ));
    
    CV_CALL( cvInitMatNDHeader( arr, dims, sizes, type, 0 ));
    arr->hdr_refcount = 1;

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMatND( &arr );

    return arr;
 }
// 
// 
// // Creates a copy of nD array
CV_IMPL CvMatND*
cvCloneMatND( const CvMatND* src )
{
    CvMatND* dst = 0;
    CV_FUNCNAME( "cvCloneMatND" );

    __BEGIN__;

    int i, *sizes;

    if( !CV_IS_MATND_HDR( src ))
        CV_ERROR( CV_StsBadArg, "Bad CvMatND header" );

    sizes = (int*)alloca( src->dims*sizeof(sizes[0]) );

    for( i = 0; i < src->dims; i++ )
        sizes[i] = src->dim[i].size;

    CV_CALL( dst = cvCreateMatNDHeader( src->dims, sizes, src->type ));

    if( src->data.ptr )
    {
        CV_CALL( cvCreateData( dst ));
        CV_CALL( cvCopy( src, dst ));
    }

    __END__;

    return dst;
}
// 
// 
static CvMatND*
cvGetMatND( const CvArr* arr, CvMatND* matnd, int* coi )
{
    CvMatND* result = 0;
    
    CV_FUNCNAME( "cvGetMatND" );

    __BEGIN__;

    if( coi )
        *coi = 0;

    if( !matnd || !arr )
        CV_ERROR( CV_StsNullPtr, "NULL array pointer is passed" );

    if( CV_IS_MATND_HDR(arr))
    {
        if( !((CvMatND*)arr)->data.ptr )
            CV_ERROR( CV_StsNullPtr, "The matrix has NULL data pointer" );
        
        result = (CvMatND*)arr;
    }
    else
    {
        CvMat stub, *mat = (CvMat*)arr;
        
        if( CV_IS_IMAGE_HDR( mat ))
            CV_CALL( mat = cvGetMat( mat, &stub, coi ));

        if( !CV_IS_MAT_HDR( mat ))
            CV_ERROR( CV_StsBadArg, "Unrecognized or unsupported array type" );
        
        if( !mat->data.ptr )
            CV_ERROR( CV_StsNullPtr, "Input array has NULL data pointer" );

        matnd->data.ptr = mat->data.ptr;
        matnd->refcount = 0;
        matnd->hdr_refcount = 0;
        matnd->type = mat->type;
        matnd->dims = 2;
        matnd->dim[0].size = mat->rows;
        matnd->dim[0].step = mat->step;
        matnd->dim[1].size = mat->cols;
        matnd->dim[1].step = CV_ELEM_SIZE(mat->type);
        result = matnd;
    }

    __END__;

    return result;
}
// 
// 
// // returns number of dimensions to iterate.
// /*
// Checks whether <count> arrays have equal type, sizes (mask is optional array
// that needs to have the same size, but 8uC1 or 8sC1 type).
// Returns number of dimensions to iterate through:
// 0 means that all arrays are continuous,
// 1 means that all arrays are vectors of continuous arrays etc.
// and the size of largest common continuous part of the arrays 
// */
CV_IMPL int
cvInitNArrayIterator( int count, CvArr** arrs,
                      const CvArr* mask, CvMatND* stubs,
                      CvNArrayIterator* iterator, int flags )
{
    int dims = -1;

    CV_FUNCNAME( "cvInitArrayOp" );
    
    __BEGIN__;

    int i, j, size, dim0 = -1;
    int64 step;
    CvMatND* hdr0 = 0;

    if( count < 1 || count > CV_MAX_ARR )
        CV_ERROR( CV_StsOutOfRange, "Incorrect number of arrays" );

    if( !arrs || !stubs )
        CV_ERROR( CV_StsNullPtr, "Some of required array pointers is NULL" );

    if( !iterator )
        CV_ERROR( CV_StsNullPtr, "Iterator pointer is NULL" );

    for( i = 0; i <= count; i++ )
    {
        const CvArr* arr = i < count ? arrs[i] : mask;
        CvMatND* hdr;
        
        if( !arr )
        {
            if( i < count )
                CV_ERROR( CV_StsNullPtr, "Some of required array pointers is NULL" );
            break;
        }

        if( CV_IS_MATND( arr ))
            hdr = (CvMatND*)arr;
        else
        {
            int coi = 0;
            CV_CALL( hdr = cvGetMatND( arr, stubs + i, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "COI set is not allowed here" );
        }

        iterator->hdr[i] = hdr;

        if( i > 0 )
        {
            if( hdr->dims != hdr0->dims )
                CV_ERROR( CV_StsUnmatchedSizes,
                          "Number of dimensions is the same for all arrays" );
            
            if( i < count )
            {
                switch( flags & (CV_NO_DEPTH_CHECK|CV_NO_CN_CHECK))
                {
                case 0:
                    if( !CV_ARE_TYPES_EQ( hdr, hdr0 ))
                        CV_ERROR( CV_StsUnmatchedFormats,
                                  "Data type is not the same for all arrays" );
                    break;
                case CV_NO_DEPTH_CHECK:
                    if( !CV_ARE_CNS_EQ( hdr, hdr0 ))
                        CV_ERROR( CV_StsUnmatchedFormats,
                                  "Number of channels is not the same for all arrays" );
                    break;
                case CV_NO_CN_CHECK:
                    if( !CV_ARE_CNS_EQ( hdr, hdr0 ))
                        CV_ERROR( CV_StsUnmatchedFormats,
                                  "Depth is not the same for all arrays" );
                    break;
                }
            }
            else
            {
                if( !CV_IS_MASK_ARR( hdr ))
                    CV_ERROR( CV_StsBadMask, "Mask should have 8uC1 or 8sC1 data type" );
            }

            if( !(flags & CV_NO_SIZE_CHECK) )
            {
                for( j = 0; j < hdr->dims; j++ )
                    if( hdr->dim[j].size != hdr0->dim[j].size )
                        CV_ERROR( CV_StsUnmatchedSizes,
                                  "Dimension sizes are the same for all arrays" );
            }
        }
        else
            hdr0 = hdr;

        step = CV_ELEM_SIZE(hdr->type);
        for( j = hdr->dims - 1; j > dim0; j-- )
        {
            if( step != hdr->dim[j].step )
                break;
            step *= hdr->dim[j].size;
        }

        if( j == dim0 && step > INT_MAX )
            j++;

        if( j > dim0 )
            dim0 = j;

        iterator->hdr[i] = (CvMatND*)hdr;
        iterator->ptr[i] = (uchar*)hdr->data.ptr;
    }

    size = 1;
    for( j = hdr0->dims - 1; j > dim0; j-- )
        size *= hdr0->dim[j].size;

    dims = dim0 + 1;
    iterator->dims = dims;
    iterator->count = count;
    iterator->size = cvSize(size,1);

    for( i = 0; i < dims; i++ )
        iterator->stack[i] = hdr0->dim[i].size;

    __END__;

    return dims;
}
// 
// 
// // returns zero value if iteration is finished, non-zero otherwise
CV_IMPL  int  cvNextNArraySlice( CvNArrayIterator* iterator )
{
    assert( iterator != 0 );
    int i, dims, size = 0;

    for( dims = iterator->dims; dims > 0; dims-- )
    {
        for( i = 0; i < iterator->count; i++ )
            iterator->ptr[i] += iterator->hdr[i]->dim[dims-1].step;

        if( --iterator->stack[dims-1] > 0 )
            break;

        size = iterator->hdr[0]->dim[dims-1].size;

        for( i = 0; i < iterator->count; i++ )
            iterator->ptr[i] -= (size_t)size*iterator->hdr[i]->dim[dims-1].step;

        iterator->stack[dims-1] = size;
    }

    return dims > 0;
}
// 
// 
// /****************************************************************************************\
// *                            CvSparseMat creation and basic operations                   *
// \****************************************************************************************/
// 
// 
// // Creates CvMatND and underlying data
CV_IMPL CvSparseMat*
cvCreateSparseMat( int dims, const int* sizes, int type )
{
    CvSparseMat* arr = 0;

    CV_FUNCNAME( "cvCreateSparseMat" );
    
    __BEGIN__;

    type = CV_MAT_TYPE( type );
    int pix_size1 = CV_ELEM_SIZE1(type);
    int pix_size = pix_size1*CV_MAT_CN(type);
    int i, size;
    CvMemStorage* storage;

    if( pix_size == 0 )
        CV_ERROR( CV_StsUnsupportedFormat, "invalid array data type" );

    if( dims <= 0 || dims > CV_MAX_DIM_HEAP )
        CV_ERROR( CV_StsOutOfRange, "bad number of dimensions" );

    if( !sizes )
        CV_ERROR( CV_StsNullPtr, "NULL <sizes> pointer" );

    for( i = 0; i < dims; i++ )
    {
        if( sizes[i] <= 0 )
            CV_ERROR( CV_StsBadSize, "one of dimesion sizes is non-positive" );
    }

    CV_CALL( arr = (CvSparseMat*)cvAlloc(sizeof(*arr)+MAX(0,dims-CV_MAX_DIM)*sizeof(arr->size[0])));

    arr->type = CV_SPARSE_MAT_MAGIC_VAL | type;
    arr->dims = dims;
    arr->refcount = 0;
    arr->hdr_refcount = 1;
    memcpy( arr->size, sizes, dims*sizeof(sizes[0]));

    arr->valoffset = (int)cvAlign(sizeof(CvSparseNode), pix_size1);
    arr->idxoffset = (int)cvAlign(arr->valoffset + pix_size, sizeof(int));
    size = (int)cvAlign(arr->idxoffset + dims*sizeof(int), sizeof(CvSetElem));

    CV_CALL( storage = cvCreateMemStorage( CV_SPARSE_MAT_BLOCK ));
    CV_CALL( arr->heap = cvCreateSet( 0, sizeof(CvSet), size, storage ));

    arr->hashsize = CV_SPARSE_HASH_SIZE0;
    size = arr->hashsize*sizeof(arr->hashtable[0]);
    
    CV_CALL( arr->hashtable = (void**)cvAlloc( size ));
    memset( arr->hashtable, 0, size );

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseSparseMat( &arr );

    return arr;
}
// 
// 
// // Creates CvMatND and underlying data
CV_IMPL void
cvReleaseSparseMat( CvSparseMat** array )
{
    CV_FUNCNAME( "cvReleaseSparseMat" );
    
    __BEGIN__;

    if( !array )
        CV_ERROR_FROM_CODE( CV_HeaderIsNull );

    if( *array )
    {
        CvSparseMat* arr = *array;
        
        if( !CV_IS_SPARSE_MAT_HDR(arr) )
            CV_ERROR_FROM_CODE( CV_StsBadFlag );

        *array = 0;

        cvReleaseMemStorage( &arr->heap->storage );
        cvFree( &arr->hashtable );
        cvFree( &arr );
    }

    __END__;
}
// 
// 
// // Creates CvMatND and underlying data
CV_IMPL CvSparseMat*
cvCloneSparseMat( const CvSparseMat* src )
{
    CvSparseMat* dst = 0;
    
    CV_FUNCNAME( "cvCloneSparseMat" );
    
    __BEGIN__;

    if( !CV_IS_SPARSE_MAT_HDR(src) )
        CV_ERROR( CV_StsBadArg, "Invalid sparse array header" );

    CV_CALL( dst = cvCreateSparseMat( src->dims, src->size, src->type ));
    CV_CALL( cvCopy( src, dst )); 

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseSparseMat( &dst );
    
    return dst;
}
// 
// 
CvSparseNode*
cvInitSparseMatIterator( const CvSparseMat* mat, CvSparseMatIterator* iterator )
{
    CvSparseNode* node = 0;
    
    CV_FUNCNAME( "cvInitSparseMatIterator" );

    __BEGIN__;

    int idx;

    if( !CV_IS_SPARSE_MAT( mat ))
        CV_ERROR( CV_StsBadArg, "Invalid sparse matrix header" );

    if( !iterator )
        CV_ERROR( CV_StsNullPtr, "NULL iterator pointer" );

    iterator->mat = (CvSparseMat*)mat;
    iterator->node = 0;

    for( idx = 0; idx < mat->hashsize; idx++ )
        if( mat->hashtable[idx] )
        {
            node = iterator->node = (CvSparseNode*)mat->hashtable[idx];
            break;
        }

    iterator->curidx = idx;

    __END__;

    return node;
}

#define ICV_SPARSE_MAT_HASH_MULTIPLIER  33

static uchar*
icvGetNodePtr( CvSparseMat* mat, const int* idx, int* _type,
               int create_node, unsigned* precalc_hashval )
{
    uchar* ptr = 0;
    
    CV_FUNCNAME( "icvGetNodePtr" );

    __BEGIN__;

    int i, tabidx;
    unsigned hashval = 0;
    CvSparseNode *node;
    assert( CV_IS_SPARSE_MAT( mat ));

    if( !precalc_hashval )
    {
        for( i = 0; i < mat->dims; i++ )
        {
            int t = idx[i];
            if( (unsigned)t >= (unsigned)mat->size[i] )
                CV_ERROR( CV_StsOutOfRange, "One of indices is out of range" );
            hashval = hashval*ICV_SPARSE_MAT_HASH_MULTIPLIER + t;
        }
    }
    else
    {
        hashval = *precalc_hashval;
    }

    tabidx = hashval & (mat->hashsize - 1);
    hashval &= INT_MAX;

    for( node = (CvSparseNode*)mat->hashtable[tabidx];
         node != 0; node = node->next )
    {
        if( node->hashval == hashval )
        {
            int* nodeidx = CV_NODE_IDX(mat,node);
            for( i = 0; i < mat->dims; i++ )
                if( idx[i] != nodeidx[i] )
                    break;
            if( i == mat->dims )
            {
                ptr = (uchar*)CV_NODE_VAL(mat,node);
                break;
            }
        }
    }

    if( !ptr && create_node )
    {
        if( mat->heap->active_count >= mat->hashsize*CV_SPARSE_HASH_RATIO )
        {
            void** newtable;
            int newsize = MAX( mat->hashsize*2, CV_SPARSE_HASH_SIZE0);
            int newrawsize = newsize*sizeof(newtable[0]);
            
            CvSparseMatIterator iterator;
            assert( (newsize & (newsize - 1)) == 0 );

            // resize hash table
            CV_CALL( newtable = (void**)cvAlloc( newrawsize ));
            memset( newtable, 0, newrawsize );

            node = cvInitSparseMatIterator( mat, &iterator );
            while( node )
            {
                CvSparseNode* next = cvGetNextSparseNode( &iterator );
                int newidx = node->hashval & (newsize - 1);
                node->next = (CvSparseNode*)newtable[newidx];
                newtable[newidx] = node;
                node = next;
            }

            cvFree( &mat->hashtable );
            mat->hashtable = newtable;
            mat->hashsize = newsize;
            tabidx = hashval & (newsize - 1);
        }

        node = (CvSparseNode*)cvSetNew( mat->heap );
        node->hashval = hashval;
        node->next = (CvSparseNode*)mat->hashtable[tabidx];
        mat->hashtable[tabidx] = node;
        CV_MEMCPY_INT( CV_NODE_IDX(mat,node), idx, mat->dims );
        ptr = (uchar*)CV_NODE_VAL(mat,node);
        if( create_node > 0 )
            CV_ZERO_CHAR( ptr, CV_ELEM_SIZE(mat->type));
    }

    if( _type )
        *_type = CV_MAT_TYPE(mat->type);

    __END__;

    return ptr;
}
// 
// 
// static void
// icvDeleteNode( CvSparseMat* mat, const int* idx, unsigned* precalc_hashval )
// {
//     CV_FUNCNAME( "icvDeleteNode" );
// 
//     __BEGIN__;
// 
//     int i, tabidx;
//     unsigned hashval = 0;
//     CvSparseNode *node, *prev = 0;
//     assert( CV_IS_SPARSE_MAT( mat ));
// 
//     if( !precalc_hashval )
//     {
//         for( i = 0; i < mat->dims; i++ )
//         {
//             int t = idx[i];
//             if( (unsigned)t >= (unsigned)mat->size[i] )
//                 CV_ERROR( CV_StsOutOfRange, "One of indices is out of range" );
//             hashval = hashval*ICV_SPARSE_MAT_HASH_MULTIPLIER + t;
//         }
//     }
//     else
//     {
//         hashval = *precalc_hashval;
//     }
// 
//     tabidx = hashval & (mat->hashsize - 1);
//     hashval &= INT_MAX;
// 
//     for( node = (CvSparseNode*)mat->hashtable[tabidx];
//          node != 0; prev = node, node = node->next )
//     {
//         if( node->hashval == hashval )
//         {
//             int* nodeidx = CV_NODE_IDX(mat,node);
//             for( i = 0; i < mat->dims; i++ )
//                 if( idx[i] != nodeidx[i] )
//                     break;
//             if( i == mat->dims )
//                 break;
//         }
//     }
// 
//     if( node )
//     {
//         if( prev )
//             prev->next = node->next;
//         else
//             mat->hashtable[tabidx] = node->next;
//         cvSetRemoveByPtr( mat->heap, node );
//     }
// 
//     __END__;
// }
// 
// 
// 
// /****************************************************************************************\
// *                          Common for multiple array types operations                    *
// \****************************************************************************************/
// 
// // Allocates underlying array data
CV_IMPL void
cvCreateData( CvArr* arr )
{
    CV_FUNCNAME( "cvCreateData" );
    
    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ))
    {
        size_t step, total_size;
        CvMat* mat = (CvMat*)arr;
        step = mat->step;

        if( mat->data.ptr != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( step == 0 )
            step = CV_ELEM_SIZE(mat->type)*mat->cols;

        total_size = step*mat->rows + sizeof(int) + CV_MALLOC_ALIGN;
        CV_CALL( mat->refcount = (int*)cvAlloc( (size_t)total_size ));
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else if( CV_IS_IMAGE_HDR(arr))
    {
        IplImage* img = (IplImage*)arr;

        if( img->imageData != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( !CvIPL.allocateData )
        {
            CV_CALL( img->imageData = img->imageDataOrigin = 
                        (char*)cvAlloc( (size_t)img->imageSize ));
        }
        else
        {
            int depth = img->depth;
            int width = img->width;

            if( img->depth == IPL_DEPTH_32F || img->nChannels == 64 )
            {
                img->width *= img->depth == IPL_DEPTH_32F ? sizeof(float) : sizeof(double);
                img->depth = IPL_DEPTH_8U;
            }

            CvIPL.allocateData( img, 0, 0 );

            img->width = width;
            img->depth = depth;
        }
    }
    else if( CV_IS_MATND_HDR( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        int i;
        size_t total_size = CV_ELEM_SIZE(mat->type);

        if( mat->data.ptr != 0 )
            CV_ERROR( CV_StsError, "Data is already allocated" );

        if( CV_IS_MAT_CONT( mat->type ))
        {
            total_size = (size_t)mat->dim[0].size*(mat->dim[0].step != 0 ?
                         mat->dim[0].step : total_size);
        }
        else
        {
            for( i = mat->dims - 1; i >= 0; i-- )
            {
                size_t size = (size_t)mat->dim[i].step*mat->dim[i].size;

                if( total_size < size )
                    total_size = size;
            }
        }
        
        CV_CALL( mat->refcount = (int*)cvAlloc( total_size +
                                        sizeof(int) + CV_MALLOC_ALIGN ));
        mat->data.ptr = (uchar*)cvAlignPtr( mat->refcount + 1, CV_MALLOC_ALIGN );
        *mat->refcount = 1;
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;
}
// 
// Assigns external data to array
CV_IMPL void
	cvSetData( CvArr* arr, void* data, int step )
{
	CV_FUNCNAME( "cvSetData" );

	__BEGIN__;

	int pix_size, min_step;
	extern const int icvPixSize[];
	if( CV_IS_MAT_HDR(arr) || CV_IS_MATND_HDR(arr) )
		cvReleaseData( arr );

	if( data )
	{
		if( CV_IS_MAT_HDR( arr ))
		{
			CvMat* mat = (CvMat*)arr;

			int type = CV_MAT_TYPE(mat->type);
			pix_size = icvPixSize[type];
			min_step = mat->cols*pix_size & ((mat->rows <= 1) - 1);

			if( step != CV_AUTOSTEP )
			{
				if( step < min_step && data != 0 )
					CV_ERROR_FROM_CODE( CV_BadStep );
				mat->step = step & ((mat->rows <= 1) - 1);
			}
			else
			{
				mat->step = min_step;
			}

			mat->data.ptr = (uchar*)data;
			mat->type = CV_MAT_MAGIC_VAL | type |
				(mat->step==min_step ? CV_MAT_CONT_FLAG : 0);
			icvCheckHuge( mat );
		}
		else if( CV_IS_IMAGE_HDR( arr ))
		{
			IplImage* img = (IplImage*)arr;

			pix_size = ((img->depth & 255) >> 3)*img->nChannels;
			min_step = img->width*pix_size;

			if( step != CV_AUTOSTEP && img->height > 1 )
			{
				if( step < min_step && data != 0 )
					CV_ERROR_FROM_CODE( CV_BadStep );
				img->widthStep = step;
			}
			else
			{
				img->widthStep = min_step;
			}

			img->imageSize = img->widthStep * img->height;
			img->imageData = img->imageDataOrigin = (char*)data;

			if( (((int)(size_t)data | step) & 7) == 0 &&
				cvAlign(img->width * pix_size, 8) == step )
			{
				img->align = 8;
			}
			else
			{
				img->align = 4;
			}
		}
		else if( CV_IS_MATND_HDR( arr ))
		{
			CvMatND* mat = (CvMatND*)arr;
			int i;
			int64 cur_step;

			if( step != CV_AUTOSTEP )
				CV_ERROR( CV_BadStep,
				"For multidimensional array only CV_AUTOSTEP is allowed here" );

			mat->data.ptr = (uchar*)data;
			cur_step = icvPixSize[CV_MAT_TYPE(mat->type)];

			for( i = mat->dims - 1; i >= 0; i-- )
			{
				if( cur_step > INT_MAX )
					CV_ERROR( CV_StsOutOfRange, "The array is too big" );
				mat->dim[i].step = (int)cur_step;
				cur_step *= mat->dim[i].size;
			}
		}
		else
		{
			CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
		}
	}

	__END__;
}
//

CV_IMPL void
cvReleaseData( CvArr* arr )
{
    CV_FUNCNAME( "cvReleaseData" );
    
    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ) || CV_IS_MATND_HDR( arr ))
    {
        CvMat* mat = (CvMat*)arr;
        cvDecRefData( mat );
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

        if( !CvIPL.deallocate )
        {
            char* ptr = img->imageDataOrigin;
            img->imageData = img->imageDataOrigin = 0;
            cvFree( &ptr );
        }
        else
        {
            CvIPL.deallocate( img, IPL_IMAGE_DATA );
        }
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;
}

// 
CV_IMPL int
cvGetElemType( const CvArr* arr )
{
    int type = -1;

    CV_FUNCNAME( "cvGetElemType" );

    __BEGIN__;

    if( CV_IS_MAT_HDR(arr) || CV_IS_MATND_HDR(arr) || CV_IS_SPARSE_MAT_HDR(arr))
    {
        type = CV_MAT_TYPE( ((CvMat*)arr)->type );
    }
    else if( CV_IS_IMAGE(arr))
    {
        IplImage* img = (IplImage*)arr;
        type = CV_MAKETYPE( icvIplToCvDepth(img->depth), img->nChannels );
    }
    else
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );

    __END__;

    return type;
}
// 
// 
// // Returns a number of array dimensions
CV_IMPL int
cvGetDims( const CvArr* arr, int* sizes )
{
    int dims = -1;
    CV_FUNCNAME( "cvGetDims" );

    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ))
    {
        CvMat* mat = (CvMat*)arr;
        
        dims = 2;
        if( sizes )
        {
            sizes[0] = mat->rows;
            sizes[1] = mat->cols;
        }
    }
    else if( CV_IS_IMAGE( arr ))
    {
        IplImage* img = (IplImage*)arr;
        dims = 2;

        if( sizes )
        {
            sizes[0] = img->height;
            sizes[1] = img->width;
        }
    }
    else if( CV_IS_MATND_HDR( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        dims = mat->dims;
        
        if( sizes )
        {
            int i;
            for( i = 0; i < dims; i++ )
                sizes[i] = mat->dim[i].size;
        }
    }
    else if( CV_IS_SPARSE_MAT_HDR( arr ))
    {
        CvSparseMat* mat = (CvSparseMat*)arr;
        dims = mat->dims;
        
        if( sizes )
            memcpy( sizes, mat->size, dims*sizeof(sizes[0]));
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;

    return dims;
}

// 
// // Returns the size of CvMat or IplImage
CV_IMPL CvSize
cvGetSize( const CvArr* arr )
{
    CvSize size = { 0, 0 };

    CV_FUNCNAME( "cvGetSize" );

    __BEGIN__;

    if( CV_IS_MAT_HDR( arr ))
    {
        CvMat *mat = (CvMat*)arr;

        size.width = mat->cols;
        size.height = mat->rows;
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;

        if( img->roi )
        {
            size.width = img->roi->width;
            size.height = img->roi->height;
        }
        else
        {
            size.width = img->width;
            size.height = img->height;
        }
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "Array should be CvMat or IplImage" );
    }

    __END__;

    return size;
}
// 
// 
// // Selects sub-array (no data is copied)
CV_IMPL  CvMat*
cvGetSubRect( const CvArr* arr, CvMat* submat, CvRect rect )
{
    CvMat* res = 0;
    
    CV_FUNCNAME( "cvGetRect" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;

    if( !CV_IS_MAT( mat ))
        CV_CALL( mat = cvGetMat( mat, &stub ));

    if( !submat )
        CV_ERROR( CV_StsNullPtr, "" );

    if( (rect.x|rect.y|rect.width|rect.height) < 0 )
        CV_ERROR( CV_StsBadSize, "" );

    if( rect.x + rect.width > mat->cols ||
        rect.y + rect.height > mat->rows )
        CV_ERROR( CV_StsBadSize, "" );

    {
    /*
    int* refcount = mat->refcount;

    if( refcount )
        ++*refcount;

    cvDecRefData( submat );
    */
    submat->data.ptr = mat->data.ptr + (size_t)rect.y*mat->step +
                       rect.x*CV_ELEM_SIZE(mat->type);
    submat->step = mat->step & (rect.height > 1 ? -1 : 0);
    submat->type = (mat->type & (rect.width < mat->cols ? ~CV_MAT_CONT_FLAG : -1)) |
                   (submat->step == 0 ? CV_MAT_CONT_FLAG : 0);
    submat->rows = rect.height;
    submat->cols = rect.width;
    submat->refcount = 0;
    res = submat;
    }
    
    __END__;

    return res;
}
// 
// 
// // Selects array's row span.
CV_IMPL  CvMat*
cvGetRows( const CvArr* arr, CvMat* submat,
           int start_row, int end_row, int delta_row )
{
    CvMat* res = 0;
    
    CV_FUNCNAME( "cvGetRows" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;

    if( !CV_IS_MAT( mat ))
        CV_CALL( mat = cvGetMat( mat, &stub ));

    if( !submat )
        CV_ERROR( CV_StsNullPtr, "" );

    if( (unsigned)start_row >= (unsigned)mat->rows ||
        (unsigned)end_row > (unsigned)mat->rows || delta_row <= 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    {
    /*
    int* refcount = mat->refcount;

    if( refcount )
        ++*refcount;

    cvDecRefData( submat );
    */
    if( delta_row == 1 )
    {
        submat->rows = end_row - start_row;
        submat->step = mat->step & (submat->rows > 1 ? -1 : 0);
    }
    else
    {
        submat->rows = (end_row - start_row + delta_row - 1)/delta_row;
        submat->step = mat->step * delta_row;
    }

    submat->cols = mat->cols;
    submat->step &= submat->rows > 1 ? -1 : 0;
    submat->data.ptr = mat->data.ptr + (size_t)start_row*mat->step;
    submat->type = (mat->type | (submat->step == 0 ? CV_MAT_CONT_FLAG : 0)) &
                   (delta_row != 1 ? ~CV_MAT_CONT_FLAG : -1);
    submat->refcount = 0;
    submat->hdr_refcount = 0;
    res = submat;
    }
    
    __END__;

    return res;
}
// 
// 
// // Selects array's column span.
CV_IMPL  CvMat*
cvGetCols( const CvArr* arr, CvMat* submat, int start_col, int end_col )
{
    CvMat* res = 0;
    
    CV_FUNCNAME( "cvGetCols" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;

    if( !CV_IS_MAT( mat ))
        CV_CALL( mat = cvGetMat( mat, &stub ));

    if( !submat )
        CV_ERROR( CV_StsNullPtr, "" );

    if( (unsigned)start_col >= (unsigned)mat->cols ||
        (unsigned)end_col > (unsigned)mat->cols )
        CV_ERROR( CV_StsOutOfRange, "" );

    {
    /*
    int* refcount = mat->refcount;

    if( refcount )
        ++*refcount;

    cvDecRefData( submat );
    */
    submat->rows = mat->rows;
    submat->cols = end_col - start_col;
    submat->step = mat->step & (submat->rows > 1 ? -1 : 0);
    submat->data.ptr = mat->data.ptr + (size_t)start_col*CV_ELEM_SIZE(mat->type);
    submat->type = mat->type & (submat->step && submat->cols < mat->cols ?
                                ~CV_MAT_CONT_FLAG : -1);
    submat->refcount = 0;
    submat->hdr_refcount = 0;
    res = submat;
    }
    
    __END__;

    return res;
}

// 
// // Selects array diagonal
CV_IMPL  CvMat*
cvGetDiag( const CvArr* arr, CvMat* submat, int diag )
{
    CvMat* res = 0;
    
    CV_FUNCNAME( "cvGetDiag" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    int len, pix_size; 

    if( !CV_IS_MAT( mat ))
        CV_CALL( mat = cvGetMat( mat, &stub ));

    if( !submat )
        CV_ERROR( CV_StsNullPtr, "" );

    pix_size = CV_ELEM_SIZE(mat->type);

    /*{
    int* refcount = mat->refcount;

    if( refcount )
        ++*refcount;

    cvDecRefData( submat );
    }*/

    if( diag >= 0 )
    {
        len = mat->cols - diag;
        
        if( len <= 0 )
            CV_ERROR( CV_StsOutOfRange, "" );

        len = CV_IMIN( len, mat->rows );
        submat->data.ptr = mat->data.ptr + diag*pix_size;
    }
    else
    {
        len = mat->rows + diag;
        
        if( len <= 0 )
            CV_ERROR( CV_StsOutOfRange, "" );

        len = CV_IMIN( len, mat->cols );
        submat->data.ptr = mat->data.ptr - diag*mat->step;
    }

    submat->rows = len;
    submat->cols = 1;
    submat->step = (mat->step + pix_size) & (submat->rows > 1 ? -1 : 0);
    submat->type = mat->type;
    if( submat->step )
        submat->type &= ~CV_MAT_CONT_FLAG;
    else
        submat->type |= CV_MAT_CONT_FLAG;
    submat->refcount = 0;
    submat->hdr_refcount = 0;
    res = submat;
    
    __END__;

    return res;
}
// 
// 
// /****************************************************************************************\
// *                      Operations on CvScalar and accessing array elements               *
// \****************************************************************************************/
// 
// // Converts CvScalar to specified type
CV_IMPL void
cvScalarToRawData( const CvScalar* scalar, void* data, int type, int extend_to_12 )
{
    CV_FUNCNAME( "cvScalarToRawData" );

    type = CV_MAT_TYPE(type);
    
    __BEGIN__;

    int cn = CV_MAT_CN( type );
    int depth = type & CV_MAT_DEPTH_MASK;

    assert( scalar && data );
    if( (unsigned)(cn - 1) >= 4 )
        CV_ERROR( CV_StsOutOfRange, "The number of channels must be 1, 2, 3 or 4" );

    switch( depth )
    {
    case CV_8UC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((uchar*)data)[cn] = CV_CAST_8U(t);
        }
        break;
    case CV_8SC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((char*)data)[cn] = CV_CAST_8S(t);
        }
        break;
    case CV_16UC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((ushort*)data)[cn] = CV_CAST_16U(t);
        }
        break;
    case CV_16SC1:
        while( cn-- )
        {
            int t = cvRound( scalar->val[cn] );
            ((short*)data)[cn] = CV_CAST_16S(t);
        }
        break;
    case CV_32SC1:
        while( cn-- )
            ((int*)data)[cn] = cvRound( scalar->val[cn] );
        break;
    case CV_32FC1:
        while( cn-- )
            ((float*)data)[cn] = (float)(scalar->val[cn]);
        break;
    case CV_64FC1:
        while( cn-- )
            ((double*)data)[cn] = (double)(scalar->val[cn]);
        break;
    default:
        assert(0);
        CV_ERROR_FROM_CODE( CV_BadDepth );
    }

    if( extend_to_12 )
    {
        int pix_size = CV_ELEM_SIZE(type);
        int offset = CV_ELEM_SIZE1(depth)*12;

        do
        {
            offset -= pix_size;
            CV_MEMCPY_AUTO( (char*)data + offset, data, pix_size );
        }
        while( offset > pix_size );
    }

    __END__;
}

CV_IMPL  uchar*
cvPtr1D( const CvArr* arr, int idx, int* _type )
{
    uchar* ptr = 0;
    
    CV_FUNCNAME( "cvPtr1D" );

    __BEGIN__;

    if( CV_IS_MAT( arr ))
    {
        CvMat* mat = (CvMat*)arr;

        int type = CV_MAT_TYPE(mat->type);
        int pix_size = CV_ELEM_SIZE(type);

        if( _type )
            *_type = type;
        
        // the first part is mul-free sufficient check
        // that the index is within the matrix
        if( (unsigned)idx >= (unsigned)(mat->rows + mat->cols - 1) &&
            (unsigned)idx >= (unsigned)(mat->rows*mat->cols))
            CV_ERROR( CV_StsOutOfRange, "index is out of range" );

        if( CV_IS_MAT_CONT(mat->type))
        {
            ptr = mat->data.ptr + (size_t)idx*pix_size;
        }
        else
        {
            int row, col;
            if( mat->cols == 1 )
                row = idx, col = 0;
            else
                row = idx/mat->cols, col = idx - row*mat->cols;
            ptr = mat->data.ptr + (size_t)row*mat->step + col*pix_size;
        }
    }
    else if( CV_IS_IMAGE_HDR( arr ))
    {
        IplImage* img = (IplImage*)arr;
        int width = !img->roi ? img->width : img->roi->width;
        int y = idx/width, x = idx - y*width;

        ptr = cvPtr2D( arr, y, x, _type );
    }
    else if( CV_IS_MATND( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        int j, type = CV_MAT_TYPE(mat->type);
        size_t size = mat->dim[0].size;

        if( _type )
            *_type = type;

        for( j = 1; j < mat->dims; j++ )
            size *= mat->dim[j].size;

        if((unsigned)idx >= (unsigned)size )
            CV_ERROR( CV_StsOutOfRange, "index is out of range" );

        if( CV_IS_MAT_CONT(mat->type))
        {
            int pix_size = CV_ELEM_SIZE(type);
            ptr = mat->data.ptr + (size_t)idx*pix_size;
        }
        else
        {
            ptr = mat->data.ptr;
            for( j = mat->dims - 1; j >= 0; j-- )
            {
                int sz = mat->dim[j].size;
                if( sz )
                {
                    int t = idx/sz;
                    ptr += (idx - t*sz)*mat->dim[j].step;
                    idx = t;
                }
            }
        }
    }
    else if( CV_IS_SPARSE_MAT( arr ))
    {
        CvSparseMat* m = (CvSparseMat*)arr;
        if( m->dims == 1 )
            ptr = icvGetNodePtr( (CvSparseMat*)arr, &idx, _type, 1, 0 );
        else
        {
            int i, n = m->dims;
            int* _idx = (int*)cvStackAlloc(n*sizeof(_idx[0]));
            
            for( i = n - 1; i >= 0; i-- )
            {
                int t = idx / m->size[i];
                _idx[i] = idx - t*m->size[i];
                idx = t;
            }
            ptr = icvGetNodePtr( (CvSparseMat*)arr, _idx, _type, 1, 0 );
        }
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;

    return ptr;
}
// 
// 
// // Returns pointer to specified element of 2d array
CV_IMPL  uchar*
cvPtr2D( const CvArr* arr, int y, int x, int* _type )
{
    uchar* ptr = 0;
    
    CV_FUNCNAME( "cvPtr2D" );

    __BEGIN__;

    if( CV_IS_MAT( arr ))
    {
        CvMat* mat = (CvMat*)arr;
        int type;

        if( (unsigned)y >= (unsigned)(mat->rows) ||
            (unsigned)x >= (unsigned)(mat->cols) )
            CV_ERROR( CV_StsOutOfRange, "index is out of range" );

        type = CV_MAT_TYPE(mat->type);
        if( _type )
            *_type = type;

        ptr = mat->data.ptr + (size_t)y*mat->step + x*CV_ELEM_SIZE(type);
    }
    else if( CV_IS_IMAGE( arr ))
    {
        IplImage* img = (IplImage*)arr;
        int pix_size = (img->depth & 255) >> 3;
        int width, height;
        ptr = (uchar*)img->imageData;

        if( img->dataOrder == 0 )
            pix_size *= img->nChannels;

        if( img->roi )
        {
            width = img->roi->width;
            height = img->roi->height;

            ptr += img->roi->yOffset*img->widthStep +
                   img->roi->xOffset*pix_size;

            if( img->dataOrder )
            {
                int coi = img->roi->coi;
                if( !coi )
                    CV_ERROR( CV_BadCOI,
                        "COI must be non-null in case of planar images" );
                ptr += (coi - 1)*img->imageSize;
            }
        }
        else
        {
            width = img->width;
            height = img->height;
        }

        if( (unsigned)y >= (unsigned)height ||
            (unsigned)x >= (unsigned)width )
            CV_ERROR( CV_StsOutOfRange, "index is out of range" );

        ptr += y*img->widthStep + x*pix_size;

        if( _type )
        {
            int type = icvIplToCvDepth(img->depth);
            if( type < 0 || (unsigned)(img->nChannels - 1) > 3 )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            *_type = CV_MAKETYPE( type, img->nChannels );
        }
    }
    else if( CV_IS_MATND( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;

        if( mat->dims != 2 || 
            (unsigned)y >= (unsigned)(mat->dim[0].size) ||
            (unsigned)x >= (unsigned)(mat->dim[1].size) )
            CV_ERROR( CV_StsOutOfRange, "index is out of range" );

        ptr = mat->data.ptr + (size_t)y*mat->dim[0].step + x*mat->dim[1].step;
        if( _type )
            *_type = CV_MAT_TYPE(mat->type);
    }
    else if( CV_IS_SPARSE_MAT( arr ))
    {
        int idx[] = { y, x };
        ptr = icvGetNodePtr( (CvSparseMat*)arr, idx, _type, 1, 0 );
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );
    }

    __END__;

    return ptr;
}

// // Returns pointer to specified element of n-d array
CV_IMPL  uchar*
cvPtrND( const CvArr* arr, const int* idx, int* _type,
         int create_node, unsigned* precalc_hashval )
{
    uchar* ptr = 0;
    CV_FUNCNAME( "cvPtrND" );

    __BEGIN__;

    if( !idx )
        CV_ERROR( CV_StsNullPtr, "NULL pointer to indices" );

    if( CV_IS_SPARSE_MAT( arr ))
        ptr = icvGetNodePtr( (CvSparseMat*)arr, idx, 
                             _type, create_node, precalc_hashval );
    else if( CV_IS_MATND( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        int i;
        ptr = mat->data.ptr;

        for( i = 0; i < mat->dims; i++ )
        {
            if( (unsigned)idx[i] >= (unsigned)(mat->dim[i].size) )
                CV_ERROR( CV_StsOutOfRange, "index is out of range" );
            ptr += (size_t)idx[i]*mat->dim[i].step;
        }

        if( _type )
            *_type = CV_MAT_TYPE(mat->type);
    }
    else if( CV_IS_MAT_HDR(arr) || CV_IS_IMAGE_HDR(arr) )
        ptr = cvPtr2D( arr, idx[0], idx[1], _type );
    else
        CV_ERROR( CV_StsBadArg, "unrecognized or unsupported array type" );

    __END__;

    return ptr;
}
// 

// /****************************************************************************************\
// *                             Conversion to CvMat or IplImage                            *
// \****************************************************************************************/
// 
// // convert array (CvMat or IplImage) to CvMat
CV_IMPL CvMat*
cvGetMat( const CvArr* array, CvMat* mat,
          int* pCOI, int allowND )
{
    CvMat* result = 0;
    CvMat* src = (CvMat*)array;
    int coi = 0;
    
    CV_FUNCNAME( "cvGetMat" );

    __BEGIN__;

    if( !mat || !src )
        CV_ERROR( CV_StsNullPtr, "NULL array pointer is passed" );

    if( CV_IS_MAT_HDR(src))
    {
        if( !src->data.ptr )
            CV_ERROR( CV_StsNullPtr, "The matrix has NULL data pointer" );
        
        result = (CvMat*)src;
    }
    else if( CV_IS_IMAGE_HDR(src) )
    {
        const IplImage* img = (const IplImage*)src;
        int depth, order;

        if( img->imageData == 0 )
            CV_ERROR( CV_StsNullPtr, "The image has NULL data pointer" );

        depth = icvIplToCvDepth( img->depth );
        if( depth < 0 )
            CV_ERROR_FROM_CODE( CV_BadDepth );

        order = img->dataOrder & (img->nChannels > 1 ? -1 : 0);

        if( img->roi )
        {
            if( order == IPL_DATA_ORDER_PLANE )
            {
                int type = depth;

                if( img->roi->coi == 0 )
                    CV_ERROR( CV_StsBadFlag,
                    "Images with planar data layout should be used with COI selected" );

                CV_CALL( cvInitMatHeader( mat, img->roi->height,
                                   img->roi->width, type,
                                   img->imageData + (img->roi->coi-1)*img->imageSize +
                                   img->roi->yOffset*img->widthStep +
                                   img->roi->xOffset*CV_ELEM_SIZE(type),
                                   img->widthStep ));
            }
            else /* pixel order */
            {
                int type = CV_MAKETYPE( depth, img->nChannels );
                coi = img->roi->coi;

                if( img->nChannels > CV_CN_MAX )
                    CV_ERROR( CV_BadNumChannels,
                        "The image is interleaved and has over CV_CN_MAX channels" );

                CV_CALL( cvInitMatHeader( mat, img->roi->height, img->roi->width,
                                          type, img->imageData +
                                          img->roi->yOffset*img->widthStep +
                                          img->roi->xOffset*CV_ELEM_SIZE(type),
                                          img->widthStep ));
            }
        }
        else
        {
            int type = CV_MAKETYPE( depth, img->nChannels );

            if( order != IPL_DATA_ORDER_PIXEL )
                CV_ERROR( CV_StsBadFlag, "Pixel order should be used with coi == 0" );

            CV_CALL( cvInitMatHeader( mat, img->height, img->width, type,
                                      img->imageData, img->widthStep ));
        }

        result = mat;
    }
    else if( allowND && CV_IS_MATND_HDR(src) )
    {
        CvMatND* matnd = (CvMatND*)src;
        int i;
        int size1 = matnd->dim[0].size, size2 = 1;
        
        if( !src->data.ptr )
            CV_ERROR( CV_StsNullPtr, "Input array has NULL data pointer" );

        if( !CV_IS_MAT_CONT( matnd->type ))
            CV_ERROR( CV_StsBadArg, "Only continuous nD arrays are supported here" );

        if( matnd->dims > 2 )
            for( i = 1; i < matnd->dims; i++ )
                size2 *= matnd->dim[i].size;
        else
            size2 = matnd->dims == 1 ? 1 : matnd->dim[1].size;

        mat->refcount = 0;
        mat->hdr_refcount = 0;
        mat->data.ptr = matnd->data.ptr;
        mat->rows = size1;
        mat->cols = size2;
        mat->type = CV_MAT_TYPE(matnd->type) | CV_MAT_MAGIC_VAL | CV_MAT_CONT_FLAG;
        mat->step = size2*CV_ELEM_SIZE(matnd->type);
        mat->step &= size1 > 1 ? -1 : 0;

        icvCheckHuge( mat );
        result = mat;
    }
    else
    {
        CV_ERROR( CV_StsBadFlag, "Unrecognized or unsupported array type" );
    }

    __END__;

    if( pCOI )
        *pCOI = coi;

    return result;
}
// 

CV_IMPL CvMat*
cvReshape( const CvArr* array, CvMat* header,
           int new_cn, int new_rows )
{
    CvMat* result = 0;
    CV_FUNCNAME( "cvReshape" );

    __BEGIN__;

    CvMat *mat = (CvMat*)array;
    int total_width, new_width;

    if( !header )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !CV_IS_MAT( mat ))
    {
        int coi = 0;
        CV_CALL( mat = cvGetMat( mat, header, &coi, 1 ));
        if( coi )
            CV_ERROR( CV_BadCOI, "COI is not supported" );
    }

    if( new_cn == 0 )
        new_cn = CV_MAT_CN(mat->type);
    else if( (unsigned)(new_cn - 1) > 3 )
        CV_ERROR( CV_BadNumChannels, "" );

    if( mat != header )
    {
        *header = *mat;
        header->refcount = 0;
        header->hdr_refcount = 0;
    }

    total_width = mat->cols * CV_MAT_CN( mat->type );

    if( (new_cn > total_width || total_width % new_cn != 0) && new_rows == 0 )
        new_rows = mat->rows * total_width / new_cn;

    if( new_rows == 0 || new_rows == mat->rows )
    {
        header->rows = mat->rows;
        header->step = mat->step;
    }
    else
    {
        int total_size = total_width * mat->rows;
        if( !CV_IS_MAT_CONT( mat->type ))
            CV_ERROR( CV_BadStep,
            "The matrix is not continuous, thus its number of rows can not be changed" );

        if( (unsigned)new_rows > (unsigned)total_size )
            CV_ERROR( CV_StsOutOfRange, "Bad new number of rows" );

        total_width = total_size / new_rows;

        if( total_width * new_rows != total_size )
            CV_ERROR( CV_StsBadArg, "The total number of matrix elements "
                                    "is not divisible by the new number of rows" );

        header->rows = new_rows;
        header->step = total_width * CV_ELEM_SIZE1(mat->type);
    }

    new_width = total_width / new_cn;

    if( new_width * new_cn != total_width )
        CV_ERROR( CV_BadNumChannels,
        "The total width is not divisible by the new number of channels" );

    header->cols = new_width;
    header->type = CV_MAKETYPE( mat->type & ~CV_MAT_CN_MASK, new_cn );

    result = header;

    __END__;

    return  result;
}
// convert array (CvMat or IplImage) to IplImage
CV_IMPL IplImage*
	cvGetImage( const CvArr* array, IplImage* img )
{
	IplImage* result = 0;
	const IplImage* src = (const IplImage*)array;

	CV_FUNCNAME( "cvGetImage" );

	__BEGIN__;

	int depth;

	if( !img )
		CV_ERROR_FROM_CODE( CV_StsNullPtr );

	if( !CV_IS_IMAGE_HDR(src) )
	{
		const CvMat* mat = (const CvMat*)src;

		if( !CV_IS_MAT_HDR(mat))
			CV_ERROR_FROM_CODE( CV_StsBadFlag );

		if( mat->data.ptr == 0 )
			CV_ERROR_FROM_CODE( CV_StsNullPtr );

		depth = cvCvToIplDepth(mat->type);

		cvInitImageHeader( img, cvSize(mat->cols, mat->rows),
			depth, CV_MAT_CN(mat->type) );
		cvSetData( img, mat->data.ptr, mat->step );

		result = img;
	}
	else
	{
		result = (IplImage*)src;
	}

	__END__;

	return result;
}
// 

// /****************************************************************************************\
// *                               IplImage-specific functions                              *
// \****************************************************************************************/
// 
static IplROI* icvCreateROI( int coi, int xOffset, int yOffset, int width, int height )
{
    IplROI *roi = 0;

    CV_FUNCNAME( "icvCreateROI" );

    __BEGIN__;

    if( !CvIPL.createROI )
    {
        CV_CALL( roi = (IplROI*)cvAlloc( sizeof(*roi)));

        roi->coi = coi;
        roi->xOffset = xOffset;
        roi->yOffset = yOffset;
        roi->width = width;
        roi->height = height;
    }
    else
    {
        roi = CvIPL.createROI( coi, xOffset, yOffset, width, height );
    }

    __END__;

    return roi;
}

static  void
icvGetColorModel( int nchannels, char** colorModel, char** channelSeq )
{
    static char* tab[][2] =
    {
        {"GRAY", "GRAY"},
        {"",""},
        {"RGB","BGR"},
        {"RGB","BGRA"}
    };

    nchannels--;
    *colorModel = *channelSeq = "";

    if( (unsigned)nchannels <= 3 )
    {
        *colorModel = tab[nchannels][0];
        *channelSeq = tab[nchannels][1];
    }
}
// 
// 
// // create IplImage header
CV_IMPL IplImage *
cvCreateImageHeader( CvSize size, int depth, int channels )
{
    IplImage *img = 0;

    CV_FUNCNAME( "cvCreateImageHeader" );

    __BEGIN__;

    if( !CvIPL.createHeader )
    {
        CV_CALL( img = (IplImage *)cvAlloc( sizeof( *img )));
        CV_CALL( cvInitImageHeader( img, size, depth, channels, IPL_ORIGIN_TL,
                                    CV_DEFAULT_IMAGE_ROW_ALIGN ));
    }
    else
    {
        char *colorModel;
        char *channelSeq;

        icvGetColorModel( channels, &colorModel, &channelSeq );

        img = CvIPL.createHeader( channels, 0, depth, colorModel, channelSeq,
                                  IPL_DATA_ORDER_PIXEL, IPL_ORIGIN_TL,
                                  CV_DEFAULT_IMAGE_ROW_ALIGN,
                                  size.width, size.height, 0, 0, 0, 0 );
    }

    __END__;

    if( cvGetErrStatus() < 0 && img )
        cvReleaseImageHeader( &img );

    return img;
}
// 
// 
// // create IplImage header and allocate underlying data
CV_IMPL IplImage *
cvCreateImage( CvSize size, int depth, int channels )
{
    IplImage *img = 0;

    CV_FUNCNAME( "cvCreateImage" );

    __BEGIN__;

    CV_CALL( img = cvCreateImageHeader( size, depth, channels ));
    assert( img );
    CV_CALL( cvCreateData( img ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseImage( &img );

    return img;
}
// 
// 
// // initalize IplImage header, allocated by the user
CV_IMPL IplImage*
cvInitImageHeader( IplImage * image, CvSize size, int depth,
                   int channels, int origin, int align )
{
    IplImage* result = 0;

    CV_FUNCNAME( "cvInitImageHeader" );

    __BEGIN__;

    char *colorModel, *channelSeq;

    if( !image )
        CV_ERROR( CV_HeaderIsNull, "null pointer to header" );

    memset( image, 0, sizeof( *image ));
    image->nSize = sizeof( *image );

    CV_CALL( icvGetColorModel( channels, &colorModel, &channelSeq ));
    strncpy( image->colorModel, colorModel, 4 );
    strncpy( image->channelSeq, channelSeq, 4 );

    if( size.width < 0 || size.height < 0 )
        CV_ERROR( CV_BadROISize, "Bad input roi" );

    if( (depth != (int)IPL_DEPTH_1U && depth != (int)IPL_DEPTH_8U &&
         depth != (int)IPL_DEPTH_8S && depth != (int)IPL_DEPTH_16U &&
         depth != (int)IPL_DEPTH_16S && depth != (int)IPL_DEPTH_32S &&
         depth != (int)IPL_DEPTH_32F && depth != (int)IPL_DEPTH_64F) ||
         channels < 0 )
        CV_ERROR( CV_BadDepth, "Unsupported format" );
    if( origin != CV_ORIGIN_BL && origin != CV_ORIGIN_TL )
        CV_ERROR( CV_BadOrigin, "Bad input origin" );

    if( align != 4 && align != 8 )
        CV_ERROR( CV_BadAlign, "Bad input align" );

    image->width = size.width;
    image->height = size.height;

    if( image->roi )
    {
        image->roi->coi = 0;
        image->roi->xOffset = image->roi->yOffset = 0;
        image->roi->width = size.width;
        image->roi->height = size.height;
    }

    image->nChannels = MAX( channels, 1 );
    image->depth = depth;
    image->align = align;
    image->widthStep = (((image->width * image->nChannels *
         (image->depth & ~IPL_DEPTH_SIGN) + 7)/8)+ align - 1) & (~(align - 1));
    image->origin = origin;
    image->imageSize = image->widthStep * image->height;

    result = image;

    __END__;

    return result;
}
// 
// 
CV_IMPL void
cvReleaseImageHeader( IplImage** image )
{
    CV_FUNCNAME( "cvReleaseImageHeader" );

    __BEGIN__;

    if( !image )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;
        
        if( !CvIPL.deallocate )
        {
            cvFree( &img->roi );
            cvFree( &img );
        }
        else
        {
            CvIPL.deallocate( img, IPL_IMAGE_HEADER | IPL_IMAGE_ROI );
        }
    }
    __END__;
}
// 
// 
CV_IMPL void
cvReleaseImage( IplImage ** image )
{
    CV_FUNCNAME( "cvReleaseImage" );

    __BEGIN__

    if( !image )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *image )
    {
        IplImage* img = *image;
        *image = 0;
        
        cvReleaseData( img );
        cvReleaseImageHeader( &img );
    }

    __END__;
}
// 
// 
CV_IMPL void
cvSetImageROI( IplImage* image, CvRect rect )
{
    CV_FUNCNAME( "cvSetImageROI" );

    __BEGIN__;

    if( !image )
        CV_ERROR( CV_HeaderIsNull, "" );

    if( rect.x > image->width || rect.y > image->height )
        CV_ERROR( CV_BadROISize, "" );

    if( rect.x + rect.width < 0 || rect.y + rect.height < 0 )
        CV_ERROR( CV_BadROISize, "" );

    if( rect.x < 0 )
    {
        rect.width += rect.x;
        rect.x = 0;
    }

    if( rect.y < 0 )
    {
        rect.height += rect.y;
        rect.y = 0;
    }

    if( rect.x + rect.width > image->width )
        rect.width = image->width - rect.x;

    if( rect.y + rect.height > image->height )
        rect.height = image->height - rect.y;

    if( image->roi )
    {
        image->roi->xOffset = rect.x;
        image->roi->yOffset = rect.y;
        image->roi->width = rect.width;
        image->roi->height = rect.height;
    }
    else
    {
        CV_CALL( image->roi = icvCreateROI( 0, rect.x, rect.y, rect.width, rect.height ));
    }

    __END__;
}
// 

CV_IMPL void
cvSetImageCOI( IplImage* image, int coi )
{
    CV_FUNCNAME( "cvSetImageCOI" );

    __BEGIN__;

    if( !image )
        CV_ERROR( CV_HeaderIsNull, "" );

    if( (unsigned)coi > (unsigned)(image->nChannels) )
        CV_ERROR( CV_BadCOI, "" );

    if( image->roi || coi != 0 )
    {
        if( image->roi )
        {
            image->roi->coi = coi;
        }
        else
        {
            CV_CALL( image->roi = icvCreateROI( coi, 0, 0, image->width, image->height ));
        }
    }

    __END__;
}

CV_IMPL IplImage*
cvCloneImage( const IplImage* src )
{
    IplImage* dst = 0;
    CV_FUNCNAME( "cvCloneImage" );

    __BEGIN__;

    if( !CV_IS_IMAGE_HDR( src ))
        CV_ERROR( CV_StsBadArg, "Bad image header" );

    if( !CvIPL.cloneImage )
    {
        CV_CALL( dst = (IplImage*)cvAlloc( sizeof(*dst)));

        memcpy( dst, src, sizeof(*src));
        dst->imageData = dst->imageDataOrigin = 0;
        dst->roi = 0;

        if( src->roi )
        {
            dst->roi = icvCreateROI( src->roi->coi, src->roi->xOffset,
                          src->roi->yOffset, src->roi->width, src->roi->height );
        }

        if( src->imageData )
        {
            int size = src->imageSize;
            cvCreateData( dst );
            memcpy( dst->imageData, src->imageData, size );
        }
    }
    else
    {
        dst = CvIPL.cloneImage( src );
    }

    __END__;

    return dst;
}
// 
// 
// /****************************************************************************************\
// *                            Additional operations on CvTermCriteria                     *
// \****************************************************************************************/
// 
CV_IMPL CvTermCriteria
cvCheckTermCriteria( CvTermCriteria criteria, double default_eps,
                     int default_max_iters )
{
    CV_FUNCNAME( "cvCheckTermCriteria" );

    CvTermCriteria crit;

    crit.type = CV_TERMCRIT_ITER|CV_TERMCRIT_EPS;
    crit.max_iter = default_max_iters;
    crit.epsilon = (float)default_eps;
    
    __BEGIN__;

    if( (criteria.type & ~(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER)) != 0 )
        CV_ERROR( CV_StsBadArg,
                  "Unknown type of term criteria" );

    if( (criteria.type & CV_TERMCRIT_ITER) != 0 )
    {
        if( criteria.max_iter <= 0 )
            CV_ERROR( CV_StsBadArg,
                  "Iterations flag is set and maximum number of iterations is <= 0" );
        crit.max_iter = criteria.max_iter;
    }
    
    if( (criteria.type & CV_TERMCRIT_EPS) != 0 )
    {
        if( criteria.epsilon < 0 )
            CV_ERROR( CV_StsBadArg, "Accuracy flag is set and epsilon is < 0" );

        crit.epsilon = criteria.epsilon;
    }

    if( (criteria.type & (CV_TERMCRIT_EPS | CV_TERMCRIT_ITER)) == 0 )
        CV_ERROR( CV_StsBadArg,
                  "Neither accuracy nor maximum iterations "
                  "number flags are set in criteria type" );

    __END__;

    crit.epsilon = (float)MAX( 0, crit.epsilon );
    crit.max_iter = MAX( 1, crit.max_iter );

    return crit;
}

// 
// /* End of file. */
//////////////////////////////////////////////////////////////////////////
//CVCOPY
//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                  L/L COPY & SET FUNCTIONS                           //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


IPCVAPI_IMPL( CvStatus, icvCopy_8u_C1R, ( const uchar* src, int srcstep,
                                          uchar* dst, int dststep, CvSize size ),
                                          (src, srcstep, dst, dststep, size) )
{
    for( ; size.height--; src += srcstep, dst += dststep )
        memcpy( dst, src, size.width );

    return  CV_OK;
}


static CvStatus CV_STDCALL
icvSet_8u_C1R( uchar* dst, int dst_step, CvSize size,
               const void* scalar, int pix_size )
{
    int copy_len = 12*pix_size;
    uchar* dst_limit = dst + size.width;
    
    if( size.height-- )
    {
        while( dst + copy_len <= dst_limit )
        {
            memcpy( dst, scalar, copy_len );
            dst += copy_len;
        }

        memcpy( dst, scalar, dst_limit - dst );
    }

    if( size.height )
    {
        dst = dst_limit - size.width + dst_step;

        for( ; size.height--; dst += dst_step )
            memcpy( dst, dst - dst_step, size.width );
    }

    return CV_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                L/L COPY WITH MASK FUNCTIONS                         //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


#define ICV_DEF_COPY_MASK_C1_CASE( type )   \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
        if( mask[i+1] )                     \
            dst[i+1] = src[i+1];            \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = src[i];                \
    }

#define ICV_DEF_COPY_MASK_C3_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*3];             \
            type t1 = src[i*3+1];           \
            type t2 = src[i*3+2];           \
                                            \
            dst[i*3] = t0;                  \
            dst[i*3+1] = t1;                \
            dst[i*3+2] = t2;                \
        }



#define ICV_DEF_COPY_MASK_C4_CASE( type )   \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            type t0 = src[i*4];             \
            type t1 = src[i*4+1];           \
            dst[i*4] = t0;                  \
            dst[i*4+1] = t1;                \
                                            \
            t0 = src[i*4+2];                \
            t1 = src[i*4+3];                \
            dst[i*4+2] = t0;                \
            dst[i*4+3] = t1;                \
        }


#define ICV_DEF_COPY_MASK_2D( name, type, cn )              \
IPCVAPI_IMPL( CvStatus,                                     \
name,( const type* src, int srcstep, type* dst, int dststep,\
       CvSize size, const uchar* mask, int maskstep ),      \
       (src, srcstep, dst, dststep, size, mask, maskstep))  \
{                                                           \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);   \
    for( ; size.height--; src += srcstep,                   \
            dst += dststep, mask += maskstep )              \
    {                                                       \
        int i;                                              \
        ICV_DEF_COPY_MASK_C##cn##_CASE( type )              \
    }                                                       \
                                                            \
    return  CV_OK;                                          \
}


#define ICV_DEF_SET_MASK_C1_CASE( type )    \
    for( i = 0; i <= size.width-2; i += 2 ) \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
        if( mask[i+1] )                     \
            dst[i+1] = s0;                  \
    }                                       \
                                            \
    for( ; i < size.width; i++ )            \
    {                                       \
        if( mask[i] )                       \
            dst[i] = s0;                    \
    }


#define ICV_DEF_SET_MASK_C3_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*3] = s0;                  \
            dst[i*3+1] = s1;                \
            dst[i*3+2] = s2;                \
        }

#define ICV_DEF_SET_MASK_C4_CASE( type )    \
    for( i = 0; i < size.width; i++ )       \
        if( mask[i] )                       \
        {                                   \
            dst[i*4] = s0;                  \
            dst[i*4+1] = s1;                \
            dst[i*4+2] = s2;                \
            dst[i*4+3] = s3;                \
        }

#define ICV_DEF_SET_MASK_2D( name, type, cn )       \
IPCVAPI_IMPL( CvStatus,                             \
name,( type* dst, int dststep,                      \
       const uchar* mask, int maskstep,             \
       CvSize size, const type* scalar ),           \
       (dst, dststep, mask, maskstep, size, scalar))\
{                                                   \
    CV_UN_ENTRY_C##cn( type );                      \
    dststep /= sizeof(dst[0]);                      \
                                                    \
    for( ; size.height--; mask += maskstep,         \
                          dst += dststep )          \
    {                                               \
        int i;                                      \
        ICV_DEF_SET_MASK_C##cn##_CASE( type )       \
    }                                               \
                                                    \
    return CV_OK;                                   \
}


ICV_DEF_SET_MASK_2D( icvSet_8u_C1MR, uchar, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C1MR, ushort, 1 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C3MR, uchar, 3 )
ICV_DEF_SET_MASK_2D( icvSet_8u_C4MR, int, 1 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C3MR, ushort, 3 )
ICV_DEF_SET_MASK_2D( icvSet_16s_C4MR, int64, 1 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C3MR, int, 3 )
ICV_DEF_SET_MASK_2D( icvSet_32f_C4MR, int, 4 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C3MR, int64, 3 )
ICV_DEF_SET_MASK_2D( icvSet_64s_C4MR, int64, 4 )

ICV_DEF_COPY_MASK_2D( icvCopy_8u_C1MR, uchar, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C1MR, ushort, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C3MR, uchar, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_8u_C4MR, int, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C3MR, ushort, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_16s_C4MR, int64, 1 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C3MR, int, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_32f_C4MR, int, 4 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C3MR, int64, 3 )
ICV_DEF_COPY_MASK_2D( icvCopy_64s_C4MR, int64, 4 )

#define CV_DEF_INIT_COPYSET_TAB_2D( FUNCNAME, FLAG )                \
static void icvInit##FUNCNAME##FLAG##Table( CvBtFuncTable* table )  \
{                                                                   \
    table->fn_2d[1]  = (void*)icv##FUNCNAME##_8u_C1##FLAG;          \
    table->fn_2d[2]  = (void*)icv##FUNCNAME##_16s_C1##FLAG;         \
    table->fn_2d[3]  = (void*)icv##FUNCNAME##_8u_C3##FLAG;          \
    table->fn_2d[4]  = (void*)icv##FUNCNAME##_8u_C4##FLAG;          \
    table->fn_2d[6]  = (void*)icv##FUNCNAME##_16s_C3##FLAG;         \
    table->fn_2d[8]  = (void*)icv##FUNCNAME##_16s_C4##FLAG;         \
    table->fn_2d[12] = (void*)icv##FUNCNAME##_32f_C3##FLAG;         \
    table->fn_2d[16] = (void*)icv##FUNCNAME##_32f_C4##FLAG;         \
    table->fn_2d[24] = (void*)icv##FUNCNAME##_64s_C3##FLAG;         \
    table->fn_2d[32] = (void*)icv##FUNCNAME##_64s_C4##FLAG;         \
}

CV_DEF_INIT_COPYSET_TAB_2D( Set, MR )
CV_DEF_INIT_COPYSET_TAB_2D( Copy, MR )

/////////////////////////////////////////////////////////////////////////////////////////
//                                                                                     //
//                                H/L COPY & SET FUNCTIONS                             //
//                                                                                     //
/////////////////////////////////////////////////////////////////////////////////////////


CvCopyMaskFunc
icvGetCopyMaskFunc( int elem_size )
{
    static CvBtFuncTable copym_tab;
    static int inittab = 0;

    if( !inittab )
    {
        icvInitCopyMRTable( &copym_tab );
        inittab = 1;
    }
    return (CvCopyMaskFunc)copym_tab.fn_2d[elem_size];
}


/* dst = src */
CV_IMPL void
cvCopy( const void* srcarr, void* dstarr, const void* maskarr )
{
    CV_FUNCNAME( "cvCopy" );
    
    __BEGIN__;

    int pix_size;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;

    if( !CV_IS_MAT(src) || !CV_IS_MAT(dst) )
    {
        if( CV_IS_SPARSE_MAT(src) && CV_IS_SPARSE_MAT(dst))
        {
            CvSparseMat* src1 = (CvSparseMat*)src;
            CvSparseMat* dst1 = (CvSparseMat*)dst;
            CvSparseMatIterator iterator;
            CvSparseNode* node;

            dst1->dims = src1->dims;
            memcpy( dst1->size, src1->size, src1->dims*sizeof(src1->size[0]));
            dst1->valoffset = src1->valoffset;
            dst1->idxoffset = src1->idxoffset;
            cvClearSet( dst1->heap );

            if( src1->heap->active_count >= dst1->hashsize*CV_SPARSE_HASH_RATIO )
            {
                CV_CALL( cvFree( &dst1->hashtable ));
                dst1->hashsize = src1->hashsize;
                CV_CALL( dst1->hashtable =
                    (void**)cvAlloc( dst1->hashsize*sizeof(dst1->hashtable[0])));
            }

            memset( dst1->hashtable, 0, dst1->hashsize*sizeof(dst1->hashtable[0]));

            for( node = cvInitSparseMatIterator( src1, &iterator );
                 node != 0; node = cvGetNextSparseNode( &iterator ))
            {
                CvSparseNode* node_copy = (CvSparseNode*)cvSetNew( dst1->heap );
                int tabidx = node->hashval & (dst1->hashsize - 1);
                CV_MEMCPY_AUTO( node_copy, node, dst1->heap->elem_size );
                node_copy->next = (CvSparseNode*)dst1->hashtable[tabidx];
                dst1->hashtable[tabidx] = node_copy;
            }
            EXIT;
        }
        else if( CV_IS_MATND(src) || CV_IS_MATND(dst) )
        {
            CvArr* arrs[] = { src, dst };
            CvMatND stubs[3];
            CvNArrayIterator iterator;

            CV_CALL( cvInitNArrayIterator( 2, arrs, maskarr, stubs, &iterator ));
            pix_size = CV_ELEM_SIZE(iterator.hdr[0]->type);

            if( !maskarr )
            {
                iterator.size.width *= pix_size;
                if( iterator.size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double))
                {
                    do
                    {
                        memcpy( iterator.ptr[1], iterator.ptr[0], iterator.size.width );
                    }
                    while( cvNextNArraySlice( &iterator ));
                }
                else
                {
                    do
                    {
                        icvCopy_8u_C1R( iterator.ptr[0], CV_STUB_STEP,
                                        iterator.ptr[1], CV_STUB_STEP, iterator.size );
                    }
                    while( cvNextNArraySlice( &iterator ));
                }
            }
            else
            {
                CvCopyMaskFunc func = icvGetCopyMaskFunc( pix_size );
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    func( iterator.ptr[0], CV_STUB_STEP,
                          iterator.ptr[1], CV_STUB_STEP,
                          iterator.size,
                          iterator.ptr[2], CV_STUB_STEP );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }
        else
        {
            int coi1 = 0, coi2 = 0;
            CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

            if( coi1 )
            {
                CvArr* planes[] = { 0, 0, 0, 0 };

                if( maskarr )
                    CV_ERROR( CV_StsBadArg, "COI + mask are not supported" );

                planes[coi1-1] = dst;
                CV_CALL( cvSplit( src, planes[0], planes[1], planes[2], planes[3] ));
                EXIT;
            }
            else if( coi2 )
            {
                CvArr* planes[] = { 0, 0, 0, 0 };
            
                if( maskarr )
                    CV_ERROR( CV_StsBadArg, "COI + mask are not supported" );

                planes[coi2-1] = src;
                CV_CALL( cvMerge( planes[0], planes[1], planes[2], planes[3], dst ));
                EXIT;
            }
        }
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src );
    pix_size = CV_ELEM_SIZE(src->type);

    if( !maskarr )
    {
        int src_step = src->step, dst_step = dst->step;
        size.width *= pix_size;
        if( CV_IS_MAT_CONT( src->type & dst->type ) && (src_step == dst_step) && (src_step == src->width * pix_size))
        {
            size.width *= size.height;

            if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE*
                              CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double))
            {
                memcpy( dst->data.ptr, src->data.ptr, size.width );
                EXIT;
            }

            size.height = 1;
            src_step = dst_step = CV_STUB_STEP;
        }

        icvCopy_8u_C1R( src->data.ptr, src_step,
                        dst->data.ptr, dst_step, size );
    }
    else
    {
        CvCopyMaskFunc func = icvGetCopyMaskFunc(pix_size);
        CvMat maskstub, *mask = (CvMat*)maskarr;
        int src_step = src->step;
        int dst_step = dst->step;
        int mask_step;

        if( !CV_IS_MAT( mask ))
            CV_CALL( mask = cvGetMat( mask, &maskstub ));
        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( src, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;
        
        if( CV_IS_MAT_CONT( src->type & dst->type & mask->type ))
        {
            size.width *= size.height;
            size.height = 1;
            src_step = dst_step = mask_step = CV_STUB_STEP;
        }

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step, dst->data.ptr, dst_step,
                         size, mask->data.ptr, mask_step ));
    }

    __END__;
}


/* dst(idx) = value */
CV_IMPL void
cvSet( void* arr, CvScalar value, const void* maskarr )
{
    static CvBtFuncTable setm_tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvSet" );

    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    int pix_size, type;
    double buf[12];
    int mat_step;
    CvSize size;

    if( !value.val[0] && !value.val[1] &&
        !value.val[2] && !value.val[3] && !maskarr )
    {
        cvZero( arr );
        EXIT;
    }

    if( !CV_IS_MAT(mat))
    {
        if( CV_IS_MATND(mat))
        {
            CvMatND nstub;
            CvNArrayIterator iterator;
            int pix_size1;
            
            CV_CALL( cvInitNArrayIterator( 1, &arr, maskarr, &nstub, &iterator ));

            type = CV_MAT_TYPE(iterator.hdr[0]->type);
            pix_size1 = CV_ELEM_SIZE1(type);
            pix_size = pix_size1*CV_MAT_CN(type);

            CV_CALL( cvScalarToRawData( &value, buf, type, maskarr == 0 ));

            if( !maskarr )
            {
                iterator.size.width *= pix_size;
                do
                {
                    icvSet_8u_C1R( iterator.ptr[0], CV_STUB_STEP,
                                   iterator.size, buf, pix_size1 );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                CvFunc2D_2A1P func = (CvFunc2D_2A1P)(setm_tab.fn_2d[pix_size]);
                if( !func )
                    CV_ERROR( CV_StsUnsupportedFormat, "" );

                do
                {
                    func( iterator.ptr[0], CV_STUB_STEP,
                          iterator.ptr[1], CV_STUB_STEP,
                          iterator.size, buf );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }    
        else
        {
            int coi = 0;
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));

            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "" );
        }
    }

    type = CV_MAT_TYPE( mat->type );
    pix_size = CV_ELEM_SIZE(type);
    size = cvGetMatSize( mat );
    mat_step = mat->step;

    if( !maskarr )
    {
        if( CV_IS_MAT_CONT( mat->type ))
        {
            size.width *= size.height;
        
            if( size.width <= (int)(CV_MAX_INLINE_MAT_OP_SIZE*sizeof(double)))
            {
                if( type == CV_32FC1 )
                {
                    float* dstdata = (float*)(mat->data.ptr);
                    float val = (float)value.val[0];

                    do
                    {
                        dstdata[size.width-1] = val;
                    }
                    while( --size.width );

                    EXIT;
                }

                if( type == CV_64FC1 )
                {
                    double* dstdata = (double*)(mat->data.ptr);
                    double val = value.val[0];

                    do
                    {
                        dstdata[size.width-1] = val;
                    }
                    while( --size.width );

                    EXIT;
                }
            }

            mat_step = CV_STUB_STEP;
            size.height = 1;
        }
        
        size.width *= pix_size;
        CV_CALL( cvScalarToRawData( &value, buf, type, 1 ));

        IPPI_CALL( icvSet_8u_C1R( mat->data.ptr, mat_step, size, buf,
                                  CV_ELEM_SIZE1(type)));
    }
    else
    {
        CvFunc2D_2A1P func;
        CvMat maskstub, *mask = (CvMat*)maskarr;
        int mask_step;

        CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR( mask ))
            CV_ERROR( CV_StsBadMask, "" );

        if( !inittab )
        {
            icvInitSetMRTable( &setm_tab );
            inittab = 1;
        }

        if( !CV_ARE_SIZES_EQ( mat, mask ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        mask_step = mask->step;

        if( CV_IS_MAT_CONT( mat->type & mask->type ))
        {
            size.width *= size.height;
            mat_step = mask_step = CV_STUB_STEP;
            size.height = 1;
        }

        func = (CvFunc2D_2A1P)(setm_tab.fn_2d[pix_size]);
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        CV_CALL( cvScalarToRawData( &value, buf, type, 0 ));

        IPPI_CALL( func( mat->data.ptr, mat_step, mask->data.ptr,
                         mask_step, size, buf ));
    }

    __END__;
}


/****************************************************************************************\
*                                          Clearing                                      *
\****************************************************************************************/

icvSetByte_8u_C1R_t icvSetByte_8u_C1R_p = 0;

CvStatus CV_STDCALL
icvSetZero_8u_C1R( uchar* dst, int dststep, CvSize size )
{
    if( size.width + size.height > 256 && icvSetByte_8u_C1R_p )
        return icvSetByte_8u_C1R_p( 0, dst, dststep, size );

    for( ; size.height--; dst += dststep )
        memset( dst, 0, size.width );

    return CV_OK;
}

CV_IMPL void
cvSetZero( CvArr* arr )
{
    CV_FUNCNAME( "cvSetZero" );
    
    __BEGIN__;

    CvMat stub, *mat = (CvMat*)arr;
    CvSize size;
    int mat_step;

    if( !CV_IS_MAT( mat ))
    {
        if( CV_IS_MATND(mat))
        {
            CvMatND nstub;
            CvNArrayIterator iterator;
            
            CV_CALL( cvInitNArrayIterator( 1, &arr, 0, &nstub, &iterator ));
            iterator.size.width *= CV_ELEM_SIZE(iterator.hdr[0]->type);

            if( iterator.size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double) )
            {
                do
                {
                    memset( iterator.ptr[0], 0, iterator.size.width );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            else
            {
                do
                {
                    icvSetZero_8u_C1R( iterator.ptr[0], CV_STUB_STEP, iterator.size );
                }
                while( cvNextNArraySlice( &iterator ));
            }
            EXIT;
        }    
        else if( CV_IS_SPARSE_MAT(mat))
        {
            CvSparseMat* mat1 = (CvSparseMat*)mat;
            cvClearSet( mat1->heap );
            if( mat1->hashtable )
                memset( mat1->hashtable, 0, mat1->hashsize*sizeof(mat1->hashtable[0]));
            EXIT;
        }
        else
        {
            int coi = 0;
            CV_CALL( mat = cvGetMat( mat, &stub, &coi ));
            if( coi != 0 )
                CV_ERROR( CV_BadCOI, "coi is not supported" );
        }
    }

    size = cvGetMatSize( mat );
    size.width *= CV_ELEM_SIZE(mat->type);
    mat_step = mat->step;

    if( CV_IS_MAT_CONT( mat->type ))
    {
        size.width *= size.height;

        if( size.width <= CV_MAX_INLINE_MAT_OP_SIZE*(int)sizeof(double) )
        {
            memset( mat->data.ptr, 0, size.width );
            EXIT;
        }

        mat_step = CV_STUB_STEP;
        size.height = 1;
    }

    IPPI_CALL( icvSetZero_8u_C1R( mat->data.ptr, mat_step, size ));

    __END__;
}


/****************************************************************************************\
*                                          Flipping                                      *
\****************************************************************************************/

#define ICV_DEF_FLIP_HZ_CASE_C1( type ) \
    for( i = 0; i < (len+1)/2; i++ )    \
    {                                   \
        type t0 = src[i];               \
        type t1 = src[len - i - 1];     \
        dst[i] = t1;                    \
        dst[len - i - 1] = t0;          \
    }


#define ICV_DEF_FLIP_HZ_CASE_C3( type ) \
    for( i = 0; i < (len+1)/2; i++ )    \
    {                                   \
        type t0 = src[i*3];             \
        type t1 = src[(len - i)*3 - 3]; \
        dst[i*3] = t1;                  \
        dst[(len - i)*3 - 3] = t0;      \
        t0 = src[i*3 + 1];              \
        t1 = src[(len - i)*3 - 2];      \
        dst[i*3 + 1] = t1;              \
        dst[(len - i)*3 - 2] = t0;      \
        t0 = src[i*3 + 2];              \
        t1 = src[(len - i)*3 - 1];      \
        dst[i*3 + 2] = t1;              \
        dst[(len - i)*3 - 1] = t0;      \
    }


#define ICV_DEF_FLIP_HZ_CASE_C4( type ) \
    for( i = 0; i < (len+1)/2; i++ )    \
    {                                   \
        type t0 = src[i*4];             \
        type t1 = src[(len - i)*4 - 4]; \
        dst[i*4] = t1;                  \
        dst[(len - i)*4 - 4] = t0;      \
        t0 = src[i*4 + 1];              \
        t1 = src[(len - i)*4 - 3];      \
        dst[i*4 + 1] = t1;              \
        dst[(len - i)*4 - 3] = t0;      \
        t0 = src[i*4 + 2];              \
        t1 = src[(len - i)*4 - 2];      \
        dst[i*4 + 2] = t1;              \
        dst[(len - i)*4 - 2] = t0;      \
        t0 = src[i*4 + 3];              \
        t1 = src[(len - i)*4 - 1];      \
        dst[i*4 + 3] = t1;              \
        dst[(len - i)*4 - 1] = t0;      \
    }


#define ICV_DEF_FLIP_HZ_FUNC( flavor, arrtype, cn )                 \
static CvStatus CV_STDCALL                                          \
icvFlipHorz_##flavor( const arrtype* src, int srcstep,              \
                      arrtype* dst, int dststep, CvSize size )      \
{                                                                   \
    int i, len = size.width;                                        \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);           \
                                                                    \
    for( ; size.height--; src += srcstep, dst += dststep )          \
    {                                                               \
        ICV_DEF_FLIP_HZ_CASE_C##cn( arrtype )                       \
    }                                                               \
                                                                    \
    return CV_OK;                                                   \
}


ICV_DEF_FLIP_HZ_FUNC( 8u_C1R, uchar, 1 )
ICV_DEF_FLIP_HZ_FUNC( 8u_C2R, ushort, 1 )
ICV_DEF_FLIP_HZ_FUNC( 8u_C3R, uchar, 3 )
ICV_DEF_FLIP_HZ_FUNC( 16u_C2R, int, 1 )
ICV_DEF_FLIP_HZ_FUNC( 16u_C3R, ushort, 3 )
ICV_DEF_FLIP_HZ_FUNC( 32s_C2R, int64, 1 )
ICV_DEF_FLIP_HZ_FUNC( 32s_C3R, int, 3 )
ICV_DEF_FLIP_HZ_FUNC( 64s_C2R, int, 4 )
ICV_DEF_FLIP_HZ_FUNC( 64s_C3R, int64, 3 )
ICV_DEF_FLIP_HZ_FUNC( 64s_C4R, int64, 4 )

CV_DEF_INIT_PIXSIZE_TAB_2D( FlipHorz, R )


static CvStatus
icvFlipVert_8u_C1R( const uchar* src, int srcstep,
                    uchar* dst, int dststep, CvSize size )
{
    int y, i;
    const uchar* src1 = src + (size.height - 1)*srcstep;
    uchar* dst1 = dst + (size.height - 1)*dststep;

    for( y = 0; y < (size.height + 1)/2; y++, src += srcstep, src1 -= srcstep,
                                              dst += dststep, dst1 -= dststep )
    {
        i = 0;
        if( ((size_t)(src)|(size_t)(dst)|(size_t)src1|(size_t)dst1) % sizeof(int) == 0 )
        {
            for( ; i <= size.width - 16; i += 16 )
            {
                int t0 = ((int*)(src + i))[0];
                int t1 = ((int*)(src1 + i))[0];

                ((int*)(dst + i))[0] = t1;
                ((int*)(dst1 + i))[0] = t0;

                t0 = ((int*)(src + i))[1];
                t1 = ((int*)(src1 + i))[1];

                ((int*)(dst + i))[1] = t1;
                ((int*)(dst1 + i))[1] = t0;

                t0 = ((int*)(src + i))[2];
                t1 = ((int*)(src1 + i))[2];

                ((int*)(dst + i))[2] = t1;
                ((int*)(dst1 + i))[2] = t0;

                t0 = ((int*)(src + i))[3];
                t1 = ((int*)(src1 + i))[3];

                ((int*)(dst + i))[3] = t1;
                ((int*)(dst1 + i))[3] = t0;
            }

            for( ; i <= size.width - 4; i += 4 )
            {
                int t0 = ((int*)(src + i))[0];
                int t1 = ((int*)(src1 + i))[0];

                ((int*)(dst + i))[0] = t1;
                ((int*)(dst1 + i))[0] = t0;
            }
        }

        for( ; i < size.width; i++ )
        {
            uchar t0 = src[i];
            uchar t1 = src1[i];

            dst[i] = t1;
            dst1[i] = t0;
        }
    }

    return CV_OK;
}


CV_IMPL void
cvFlip( const CvArr* srcarr, CvArr* dstarr, int flip_mode )
{
    static CvBtFuncTable tab;
    static int inittab = 0;
    
    CV_FUNCNAME( "cvFlip" );
    
    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize size;
    CvFunc2D_2A func = 0;
    int pix_size;

    if( !inittab )
    {
        icvInitFlipHorzRTable( &tab );
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !dst )
        dst = src;
    else if( !CV_IS_MAT( dst ))
    {
        int coi = 0;
        CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( src );
    pix_size = CV_ELEM_SIZE( src->type );

    if( flip_mode == 0 )
    {
        size.width *= pix_size;
        
        IPPI_CALL( icvFlipVert_8u_C1R( src->data.ptr, src->step,
                                       dst->data.ptr, dst->step, size ));
    }
    else
    {
        int inplace = src->data.ptr == dst->data.ptr;
        uchar* dst_data = dst->data.ptr;
        int dst_step = dst->step;

        func = (CvFunc2D_2A)(tab.fn_2d[pix_size]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        if( flip_mode < 0 && !inplace )
        {
            dst_data += dst_step * (dst->height - 1);
            dst_step = -dst_step;
        }

        IPPI_CALL( func( src->data.ptr, src->step, dst_data, dst_step, size ));
        
        if( flip_mode < 0 && inplace )
        {
            size.width *= pix_size;
            IPPI_CALL( icvFlipVert_8u_C1R( dst->data.ptr, dst->step,
                                           dst->data.ptr, dst->step, size ));
        }
    }

    __END__;
}


CV_IMPL void
cvRepeat( const CvArr* srcarr, CvArr* dstarr )
{
    CV_FUNCNAME( "cvRepeat" );
    
    __BEGIN__;

    CvMat sstub, *src = (CvMat*)srcarr;
    CvMat dstub, *dst = (CvMat*)dstarr;
    CvSize srcsize, dstsize;
    int pix_size;
    int x, y, k, l;

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &sstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !CV_IS_MAT( dst ))
    {
        int coi = 0;
        CV_CALL( dst = cvGetMat( dst, &dstub, &coi ));
        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "coi is not supported" );
    }

    if( !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    srcsize = cvGetMatSize( src );
    dstsize = cvGetMatSize( dst );
    pix_size = CV_ELEM_SIZE( src->type );

    for( y = 0, k = 0; y < dstsize.height; y++ )
    {
        for( x = 0; x < dstsize.width; x += srcsize.width )
        {
            l = srcsize.width;
            if( l > dstsize.width - x )
                l = dstsize.width - x;
            memcpy( dst->data.ptr + y*dst->step + x*pix_size,
                    src->data.ptr + k*src->step, l*pix_size );
        }
        if( ++k == srcsize.height )
            k = 0;
    }

    __END__;
}

/* End of file. */

//////////////////////////////////////////////////////////////////////////
// CVconvert
//////////////////////////////////////////////////////////////////////////
/****************************************************************************************\
*                            Splitting/extracting array channels                         *
\****************************************************************************************/

#define  ICV_DEF_PX2PL2PX_ENTRY_C2( arrtype_ptr, ptr )  \
    arrtype_ptr plane0 = ptr[0];                        \
    arrtype_ptr plane1 = ptr[1];

#define  ICV_DEF_PX2PL2PX_ENTRY_C3( arrtype_ptr, ptr )  \
    arrtype_ptr plane0 = ptr[0];                        \
    arrtype_ptr plane1 = ptr[1];                        \
    arrtype_ptr plane2 = ptr[2];

#define  ICV_DEF_PX2PL2PX_ENTRY_C4( arrtype_ptr, ptr )  \
    arrtype_ptr plane0 = ptr[0];                        \
    arrtype_ptr plane1 = ptr[1];                        \
    arrtype_ptr plane2 = ptr[2];                        \
    arrtype_ptr plane3 = ptr[3];


#define  ICV_DEF_PX2PL_C2( arrtype, len )           \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j < (len); j++, (src) += 2 )        \
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[1];                      \
                                                    \
        plane0[j] = t0;                             \
        plane1[j] = t1;                             \
    }                                               \
    plane0 += dststep;                              \
    plane1 += dststep;                              \
}


#define  ICV_DEF_PX2PL_C3( arrtype, len )           \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j < (len); j++, (src) += 3 )        \
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[1];                      \
        arrtype t2 = (src)[2];                      \
                                                    \
        plane0[j] = t0;                             \
        plane1[j] = t1;                             \
        plane2[j] = t2;                             \
    }                                               \
    plane0 += dststep;                              \
    plane1 += dststep;                              \
    plane2 += dststep;                              \
}


#define  ICV_DEF_PX2PL_C4( arrtype, len )           \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j < (len); j++, (src) += 4 )        \
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[1];                      \
                                                    \
        plane0[j] = t0;                             \
        plane1[j] = t1;                             \
                                                    \
        t0 = (src)[2];                              \
        t1 = (src)[3];                              \
                                                    \
        plane2[j] = t0;                             \
        plane3[j] = t1;                             \
    }                                               \
    plane0 += dststep;                              \
    plane1 += dststep;                              \
    plane2 += dststep;                              \
    plane3 += dststep;                              \
}


#define  ICV_DEF_PX2PL_COI( arrtype, len, cn )      \
{                                                   \
    int j;                                          \
                                                    \
    for( j = 0; j <= (len) - 4; j += 4, (src) += 4*(cn))\
    {                                               \
        arrtype t0 = (src)[0];                      \
        arrtype t1 = (src)[(cn)];                   \
                                                    \
        (dst)[j] = t0;                              \
        (dst)[j+1] = t1;                            \
                                                    \
        t0 = (src)[(cn)*2];                         \
        t1 = (src)[(cn)*3];                         \
                                                    \
        (dst)[j+2] = t0;                            \
        (dst)[j+3] = t1;                            \
    }                                               \
                                                    \
    for( ; j < (len); j++, (src) += (cn))           \
    {                                               \
        (dst)[j] = (src)[0];                        \
    }                                               \
}


#define  ICV_DEF_COPY_PX2PL_FUNC_2D( arrtype, flavor,   \
                                     cn, entry_macro )  \
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_C##cn##P##cn##R,\
( const arrtype* src, int srcstep,                      \
  arrtype** dst, int dststep, CvSize size ),            \
  (src, srcstep, dst, dststep, size))                   \
{                                                       \
    entry_macro(arrtype*, dst);                         \
    srcstep /= sizeof(src[0]);                          \
    dststep /= sizeof(dst[0][0]);                       \
                                                        \
    for( ; size.height--; src += srcstep )              \
    {                                                   \
        ICV_DEF_PX2PL_C##cn( arrtype, size.width );     \
        src -= size.width*(cn);                         \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


#define  ICV_DEF_COPY_PX2PL_FUNC_2D_COI( arrtype, flavor )\
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_CnC1CR,      \
( const arrtype* src, int srcstep, arrtype* dst, int dststep,\
  CvSize size, int cn, int coi ),                       \
  (src, srcstep, dst, dststep, size, cn, coi))          \
{                                                       \
    src += coi - 1;                                     \
    srcstep /= sizeof(src[0]);                          \
    dststep /= sizeof(dst[0]);                          \
                                                        \
    for( ; size.height--; src += srcstep, dst += dststep )\
    {                                                   \
        ICV_DEF_PX2PL_COI( arrtype, size.width, cn );   \
        src -= size.width*(cn);                         \
    }                                                   \
                                                        \
    return CV_OK;                                       \
}


ICV_DEF_COPY_PX2PL_FUNC_2D( uchar, 8u, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( uchar, 8u, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( uchar, 8u, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PX2PL_FUNC_2D( ushort, 16s, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( ushort, 16s, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( ushort, 16s, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int, 32f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int, 32f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int, 32f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int64, 64f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int64, 64f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PX2PL_FUNC_2D( int64, 64f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )


ICV_DEF_COPY_PX2PL_FUNC_2D_COI( uchar, 8u )
ICV_DEF_COPY_PX2PL_FUNC_2D_COI( ushort, 16s )
ICV_DEF_COPY_PX2PL_FUNC_2D_COI( int, 32f )
ICV_DEF_COPY_PX2PL_FUNC_2D_COI( int64, 64f )


/****************************************************************************************\
*                            Merging/inserting array channels                            *
\****************************************************************************************/


#define  ICV_DEF_PL2PX_C2( arrtype, len )   \
{                                           \
    int j;                                  \
                                            \
    for( j = 0; j < (len); j++, (dst) += 2 )\
    {                                       \
        arrtype t0 = plane0[j];             \
        arrtype t1 = plane1[j];             \
                                            \
        dst[0] = t0;                        \
        dst[1] = t1;                        \
    }                                       \
    plane0 += srcstep;                      \
    plane1 += srcstep;                      \
}


#define  ICV_DEF_PL2PX_C3( arrtype, len )   \
{                                           \
    int j;                                  \
                                            \
    for( j = 0; j < (len); j++, (dst) += 3 )\
    {                                       \
        arrtype t0 = plane0[j];             \
        arrtype t1 = plane1[j];             \
        arrtype t2 = plane2[j];             \
                                            \
        dst[0] = t0;                        \
        dst[1] = t1;                        \
        dst[2] = t2;                        \
    }                                       \
    plane0 += srcstep;                      \
    plane1 += srcstep;                      \
    plane2 += srcstep;                      \
}


#define  ICV_DEF_PL2PX_C4( arrtype, len )   \
{                                           \
    int j;                                  \
                                            \
    for( j = 0; j < (len); j++, (dst) += 4 )\
    {                                       \
        arrtype t0 = plane0[j];             \
        arrtype t1 = plane1[j];             \
                                            \
        dst[0] = t0;                        \
        dst[1] = t1;                        \
                                            \
        t0 = plane2[j];                     \
        t1 = plane3[j];                     \
                                            \
        dst[2] = t0;                        \
        dst[3] = t1;                        \
    }                                       \
    plane0 += srcstep;                      \
    plane1 += srcstep;                      \
    plane2 += srcstep;                      \
    plane3 += srcstep;                      \
}


#define  ICV_DEF_PL2PX_COI( arrtype, len, cn )          \
{                                                       \
    int j;                                              \
                                                        \
    for( j = 0; j <= (len) - 4; j += 4, (dst) += 4*(cn))\
    {                                                   \
        arrtype t0 = (src)[j];                          \
        arrtype t1 = (src)[j+1];                        \
                                                        \
        (dst)[0] = t0;                                  \
        (dst)[(cn)] = t1;                               \
                                                        \
        t0 = (src)[j+2];                                \
        t1 = (src)[j+3];                                \
                                                        \
        (dst)[(cn)*2] = t0;                             \
        (dst)[(cn)*3] = t1;                             \
    }                                                   \
                                                        \
    for( ; j < (len); j++, (dst) += (cn))               \
    {                                                   \
        (dst)[0] = (src)[j];                            \
    }                                                   \
}


#define  ICV_DEF_COPY_PL2PX_FUNC_2D( arrtype, flavor, cn, entry_macro ) \
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_P##cn##C##cn##R, \
( const arrtype** src, int srcstep,                         \
  arrtype* dst, int dststep, CvSize size ),                 \
  (src, srcstep, dst, dststep, size))                       \
{                                                           \
    entry_macro(const arrtype*, src);                       \
    srcstep /= sizeof(src[0][0]);                           \
    dststep /= sizeof(dst[0]);                              \
                                                            \
    for( ; size.height--; dst += dststep )                  \
    {                                                       \
        ICV_DEF_PL2PX_C##cn( arrtype, size.width );         \
        dst -= size.width*(cn);                             \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


#define  ICV_DEF_COPY_PL2PX_FUNC_2D_COI( arrtype, flavor )  \
IPCVAPI_IMPL( CvStatus, icvCopy_##flavor##_C1CnCR,          \
( const arrtype* src, int srcstep,                          \
  arrtype* dst, int dststep,                                \
  CvSize size, int cn, int coi ),                           \
  (src, srcstep, dst, dststep, size, cn, coi))              \
{                                                           \
    dst += coi - 1;                                         \
    srcstep /= sizeof(src[0]); dststep /= sizeof(dst[0]);   \
                                                            \
    for( ; size.height--; src += srcstep, dst += dststep )  \
    {                                                       \
        ICV_DEF_PL2PX_COI( arrtype, size.width, cn );       \
        dst -= size.width*(cn);                             \
    }                                                       \
                                                            \
    return CV_OK;                                           \
}


ICV_DEF_COPY_PL2PX_FUNC_2D( uchar, 8u, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( uchar, 8u, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( uchar, 8u, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PL2PX_FUNC_2D( ushort, 16s, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( ushort, 16s, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( ushort, 16s, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int, 32f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int, 32f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int, 32f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int64, 64f, 2, ICV_DEF_PX2PL2PX_ENTRY_C2 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int64, 64f, 3, ICV_DEF_PX2PL2PX_ENTRY_C3 )
ICV_DEF_COPY_PL2PX_FUNC_2D( int64, 64f, 4, ICV_DEF_PX2PL2PX_ENTRY_C4 )

ICV_DEF_COPY_PL2PX_FUNC_2D_COI( uchar, 8u )
ICV_DEF_COPY_PL2PX_FUNC_2D_COI( ushort, 16s )
ICV_DEF_COPY_PL2PX_FUNC_2D_COI( int, 32f )
ICV_DEF_COPY_PL2PX_FUNC_2D_COI( int64, 64f )


#define  ICV_DEF_PXPLPX_TAB( name, FROM, TO )                           \
static void                                                             \
name( CvBigFuncTable* tab )                                             \
{                                                                       \
    tab->fn_2d[CV_8UC2] = (void*)icvCopy##_8u_##FROM##2##TO##2R;        \
    tab->fn_2d[CV_8UC3] = (void*)icvCopy##_8u_##FROM##3##TO##3R;        \
    tab->fn_2d[CV_8UC4] = (void*)icvCopy##_8u_##FROM##4##TO##4R;        \
                                                                        \
    tab->fn_2d[CV_8SC2] = (void*)icvCopy##_8u_##FROM##2##TO##2R;        \
    tab->fn_2d[CV_8SC3] = (void*)icvCopy##_8u_##FROM##3##TO##3R;        \
    tab->fn_2d[CV_8SC4] = (void*)icvCopy##_8u_##FROM##4##TO##4R;        \
                                                                        \
    tab->fn_2d[CV_16UC2] = (void*)icvCopy##_16s_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_16UC3] = (void*)icvCopy##_16s_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_16UC4] = (void*)icvCopy##_16s_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_16SC2] = (void*)icvCopy##_16s_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_16SC3] = (void*)icvCopy##_16s_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_16SC4] = (void*)icvCopy##_16s_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_32SC2] = (void*)icvCopy##_32f_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_32SC3] = (void*)icvCopy##_32f_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_32SC4] = (void*)icvCopy##_32f_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_32FC2] = (void*)icvCopy##_32f_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_32FC3] = (void*)icvCopy##_32f_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_32FC4] = (void*)icvCopy##_32f_##FROM##4##TO##4R;      \
                                                                        \
    tab->fn_2d[CV_64FC2] = (void*)icvCopy##_64f_##FROM##2##TO##2R;      \
    tab->fn_2d[CV_64FC3] = (void*)icvCopy##_64f_##FROM##3##TO##3R;      \
    tab->fn_2d[CV_64FC4] = (void*)icvCopy##_64f_##FROM##4##TO##4R;      \
}



#define  ICV_DEF_PXPLCOI_TAB( name, FROM, TO )                          \
static void                                                             \
name( CvFuncTable* tab )                                                \
{                                                                       \
    tab->fn_2d[CV_8U] = (void*)icvCopy##_8u_##FROM##TO##CR;             \
    tab->fn_2d[CV_8S] = (void*)icvCopy##_8u_##FROM##TO##CR;             \
    tab->fn_2d[CV_16U] = (void*)icvCopy##_16s_##FROM##TO##CR;           \
    tab->fn_2d[CV_16S] = (void*)icvCopy##_16s_##FROM##TO##CR;           \
    tab->fn_2d[CV_32S] = (void*)icvCopy##_32f_##FROM##TO##CR;           \
    tab->fn_2d[CV_32F] = (void*)icvCopy##_32f_##FROM##TO##CR;           \
    tab->fn_2d[CV_64F] = (void*)icvCopy##_64f_##FROM##TO##CR;           \
}


ICV_DEF_PXPLPX_TAB( icvInitSplitRTable, C, P )
ICV_DEF_PXPLCOI_TAB( icvInitSplitRCoiTable, Cn, C1 )
ICV_DEF_PXPLPX_TAB( icvInitCvtPlaneToPixRTable, P, C )
ICV_DEF_PXPLCOI_TAB( icvInitCvtPlaneToPixRCoiTable, C1, Cn )

typedef CvStatus (CV_STDCALL *CvSplitFunc)( const void* src, int srcstep,
                                                    void** dst, int dststep, CvSize size);

typedef CvStatus (CV_STDCALL *CvExtractPlaneFunc)( const void* src, int srcstep,
                                                   void* dst, int dststep,
                                                   CvSize size, int cn, int coi );

typedef CvStatus (CV_STDCALL *CvMergeFunc)( const void** src, int srcstep,
                                                    void* dst, int dststep, CvSize size);

typedef CvStatus (CV_STDCALL *CvInsertPlaneFunc)( const void* src, int srcstep,
                                                  void* dst, int dststep,
                                                  CvSize size, int cn, int coi );

CV_IMPL void
cvSplit( const void* srcarr, void* dstarr0, void* dstarr1, void* dstarr2, void* dstarr3 )
{
    static CvBigFuncTable  pxpl_tab;
    static CvFuncTable  pxplcoi_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvSplit" );

    __BEGIN__;

    CvMat stub[5], *dst[4], *src = (CvMat*)srcarr;
    CvSize size;
    void* dstptr[4] = { 0, 0, 0, 0 };
    int type, cn, coi = 0;
    int i, nzplanes = 0, nzidx = -1;
    int cont_flag;
    int src_step, dst_step = 0;

    if( !inittab )
    {
        icvInitSplitRTable( &pxpl_tab );
        icvInitSplitRCoiTable( &pxplcoi_tab );
        inittab = 1;
    }

    dst[0] = (CvMat*)dstarr0;
    dst[1] = (CvMat*)dstarr1;
    dst[2] = (CvMat*)dstarr2;
    dst[3] = (CvMat*)dstarr3;

    CV_CALL( src = cvGetMat( src, stub + 4, &coi ));

    //if( coi != 0 )
    //    CV_ERROR( CV_BadCOI, "" );

    type = CV_MAT_TYPE( src->type );
    cn = CV_MAT_CN( type );

    cont_flag = src->type;

    if( cn == 1 )
        CV_ERROR( CV_BadNumChannels, "" );

    for( i = 0; i < 4; i++ )
    {
        if( dst[i] )
        {
            nzplanes++;
            nzidx = i;
            CV_CALL( dst[i] = cvGetMat( dst[i], stub + i ));
            if( CV_MAT_CN( dst[i]->type ) != 1 )
                CV_ERROR( CV_BadNumChannels, "" );
            if( !CV_ARE_DEPTHS_EQ( dst[i], src ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );
            if( !CV_ARE_SIZES_EQ( dst[i], src ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );
            if( nzplanes > i && i > 0 && dst[i]->step != dst[i-1]->step )
                CV_ERROR( CV_BadStep, "" );
            dst_step = dst[i]->step;
            dstptr[nzplanes-1] = dst[i]->data.ptr;

            cont_flag &= dst[i]->type;
        }
    }

    src_step = src->step;
    size = cvGetMatSize( src );

    if( CV_IS_MAT_CONT( cont_flag ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;

        size.height = 1;
    }

    if( nzplanes == cn )
    {
        CvSplitFunc func = (CvSplitFunc)pxpl_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step, dstptr, dst_step, size ));
    }
    else if( nzplanes == 1 )
    {
        CvExtractPlaneFunc func = (CvExtractPlaneFunc)pxplcoi_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                         dst[nzidx]->data.ptr, dst_step,
                         size, cn, nzidx + 1 ));
    }
    else
    {
        CV_ERROR( CV_StsBadArg,
            "Either all output planes or only one output plane should be non zero" );
    }

    __END__;
}



CV_IMPL void
cvMerge( const void* srcarr0, const void* srcarr1, const void* srcarr2,
         const void* srcarr3, void* dstarr )
{
    static CvBigFuncTable plpx_tab;
    static CvFuncTable plpxcoi_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvMerge" );

    __BEGIN__;

    int src_step = 0, dst_step;
    CvMat stub[5], *src[4], *dst = (CvMat*)dstarr;
    CvSize size;
    const void* srcptr[4] = { 0, 0, 0, 0 };
    int type, cn, coi = 0;
    int i, nzplanes = 0, nzidx = -1;
    int cont_flag;

    if( !inittab )
    {
        icvInitCvtPlaneToPixRTable( &plpx_tab );
        icvInitCvtPlaneToPixRCoiTable( &plpxcoi_tab );
        inittab = 1;
    }

    src[0] = (CvMat*)srcarr0;
    src[1] = (CvMat*)srcarr1;
    src[2] = (CvMat*)srcarr2;
    src[3] = (CvMat*)srcarr3;

    CV_CALL( dst = cvGetMat( dst, stub + 4, &coi ));

    type = CV_MAT_TYPE( dst->type );
    cn = CV_MAT_CN( type );

    cont_flag = dst->type;

    if( cn == 1 )
        CV_ERROR( CV_BadNumChannels, "" );

    for( i = 0; i < 4; i++ )
    {
        if( src[i] )
        {
            nzplanes++;
            nzidx = i;
            CV_CALL( src[i] = cvGetMat( src[i], stub + i ));
            if( CV_MAT_CN( src[i]->type ) != 1 )
                CV_ERROR( CV_BadNumChannels, "" );
            if( !CV_ARE_DEPTHS_EQ( src[i], dst ))
                CV_ERROR( CV_StsUnmatchedFormats, "" );
            if( !CV_ARE_SIZES_EQ( src[i], dst ))
                CV_ERROR( CV_StsUnmatchedSizes, "" );
            if( nzplanes > i && i > 0 && src[i]->step != src[i-1]->step )
                CV_ERROR( CV_BadStep, "" );
            src_step = src[i]->step;
            srcptr[nzplanes-1] = (const void*)(src[i]->data.ptr);

            cont_flag &= src[i]->type;
        }
    }

    size = cvGetMatSize( dst );
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( cont_flag ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }

    if( nzplanes == cn )
    {
        CvMergeFunc func = (CvMergeFunc)plpx_tab.fn_2d[type];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( srcptr, src_step, dst->data.ptr, dst_step, size ));
    }
    else if( nzplanes == 1 )
    {
        CvInsertPlaneFunc func = (CvInsertPlaneFunc)plpxcoi_tab.fn_2d[CV_MAT_DEPTH(type)];

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src[nzidx]->data.ptr, src_step,
                         dst->data.ptr, dst_step,
                         size, cn, nzidx + 1 ));
    }
    else
    {
        CV_ERROR( CV_StsBadArg,
            "Either all input planes or only one input plane should be non zero" );
    }

    __END__;
}


/****************************************************************************************\
*                       Generalized split/merge: mixing channels                         *
\****************************************************************************************/

#define  ICV_DEF_MIX_CH_FUNC_2D( arrtype, flavor )              \
static CvStatus CV_STDCALL                                      \
icvMixChannels_##flavor( const arrtype** src, int* sdelta0,     \
                         int* sdelta1, arrtype** dst,           \
                         int* ddelta0, int* ddelta1,            \
                         int n, CvSize size )                   \
{                                                               \
    int i, k;                                                   \
    int block_size0 = n == 1 ? size.width : 1024;               \
                                                                \
    for( ; size.height--; )                                     \
    {                                                           \
        int remaining = size.width;                             \
        for( ; remaining > 0; )                                 \
        {                                                       \
            int block_size = MIN( remaining, block_size0 );     \
            for( k = 0; k < n; k++ )                            \
            {                                                   \
                const arrtype* s = src[k];                      \
                arrtype* d = dst[k];                            \
                int ds = sdelta1[k], dd = ddelta1[k];           \
                if( s )                                         \
                {                                               \
                    for( i = 0; i <= block_size - 2; i += 2,    \
                                        s += ds*2, d += dd*2 )  \
                    {                                           \
                        arrtype t0 = s[0], t1 = s[ds];          \
                        d[0] = t0; d[dd] = t1;                  \
                    }                                           \
                    if( i < block_size )                        \
                        d[0] = s[0], s += ds, d += dd;          \
                    src[k] = s;                                 \
                }                                               \
                else                                            \
                {                                               \
                    for( i=0; i <= block_size-2; i+=2, d+=dd*2 )\
                        d[0] = d[dd] = 0;                       \
                    if( i < block_size )                        \
                        d[0] = 0, d += dd;                      \
                }                                               \
                dst[k] = d;                                     \
            }                                                   \
            remaining -= block_size;                            \
        }                                                       \
        for( k = 0; k < n; k++ )                                \
            src[k] += sdelta0[k], dst[k] += ddelta0[k];         \
    }                                                           \
                                                                \
    return CV_OK;                                               \
}


ICV_DEF_MIX_CH_FUNC_2D( uchar, 8u )
ICV_DEF_MIX_CH_FUNC_2D( ushort, 16u )
ICV_DEF_MIX_CH_FUNC_2D( int, 32s )
ICV_DEF_MIX_CH_FUNC_2D( int64, 64s )

static void
icvInitMixChannelsTab( CvFuncTable* tab )
{
    tab->fn_2d[CV_8U] = (void*)icvMixChannels_8u;
    tab->fn_2d[CV_8S] = (void*)icvMixChannels_8u;
    tab->fn_2d[CV_16U] = (void*)icvMixChannels_16u;
    tab->fn_2d[CV_16S] = (void*)icvMixChannels_16u;
    tab->fn_2d[CV_32S] = (void*)icvMixChannels_32s;
    tab->fn_2d[CV_32F] = (void*)icvMixChannels_32s;
    tab->fn_2d[CV_64F] = (void*)icvMixChannels_64s;
}

typedef CvStatus (CV_STDCALL * CvMixChannelsFunc)( const void** src, int* sdelta0,
        int* sdelta1, void** dst, int* ddelta0, int* ddelta1, int n, CvSize size );

CV_IMPL void
cvMixChannels( const CvArr** src, int src_count,
               CvArr** dst, int dst_count,
               const int* from_to, int pair_count )
{
    static CvFuncTable mixcn_tab;
    static int inittab = 0;
    uchar* buffer = 0;
    int heap_alloc = 0;
    
    CV_FUNCNAME( "cvMixChannels" );

    __BEGIN__;
    
    CvSize size = {0,0};
    int depth = -1, elem_size = 1;
    int *sdelta0 = 0, *sdelta1 = 0, *ddelta0 = 0, *ddelta1 = 0;
    uchar **sptr = 0, **dptr = 0;
    uchar **src0 = 0, **dst0 = 0;
    int* src_cn = 0, *dst_cn = 0;
    int* src_step = 0, *dst_step = 0;
    int buf_size, i, k;
    int cont_flag = CV_MAT_CONT_FLAG;
    CvMixChannelsFunc func;

    if( !inittab )
    {
        icvInitMixChannelsTab( &mixcn_tab );
        inittab = 1;
    }

    src_count = MAX( src_count, 0 );

    if( !src && src_count > 0 )
        CV_ERROR( CV_StsNullPtr, "The input array of arrays is NULL" );

    if( !dst )
        CV_ERROR( CV_StsNullPtr, "The output array of arrays is NULL" );

    if( dst_count <= 0 || pair_count <= 0 )
        CV_ERROR( CV_StsOutOfRange,
        "The number of output arrays and the number of copied channels must be positive" );

    if( !from_to )
        CV_ERROR( CV_StsNullPtr, "The array of copied channel indices is NULL" );

    buf_size = (src_count + dst_count + 2)*
        (sizeof(src0[0]) + sizeof(src_cn[0]) + sizeof(src_step[0])) + 
        pair_count*2*(sizeof(sptr[0]) + sizeof(sdelta0[0]) + sizeof(sdelta1[0]));

    if( buf_size > CV_MAX_LOCAL_SIZE )
    {
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ) );
        heap_alloc = 1;
    }
    else
        buffer = (uchar*)cvStackAlloc( buf_size );

    src0 = (uchar**)buffer;
    dst0 = src0 + src_count;
    src_cn = (int*)(dst0 + dst_count);
    dst_cn = src_cn + src_count + 1;
    src_step = dst_cn + dst_count + 1;
    dst_step = src_step + src_count;

    sptr = (uchar**)cvAlignPtr( dst_step + dst_count, (int)sizeof(void*) );
    dptr = sptr + pair_count;
    sdelta0 = (int*)(dptr + pair_count);
    sdelta1 = sdelta0 + pair_count;
    ddelta0 = sdelta1 + pair_count;
    ddelta1 = ddelta0 + pair_count;

    src_cn[0] = dst_cn[0] = 0;

    for( k = 0; k < 2; k++ )
    {
        for( i = 0; i < (k == 0 ? src_count : dst_count); i++ )
        {
            CvMat stub, *mat = (CvMat*)(k == 0 ? src[i] : dst[i]);
            int cn;
        
            if( !CV_IS_MAT(mat) )
                CV_CALL( mat = cvGetMat( mat, &stub ));
        
            if( depth < 0 )
            {
                depth = CV_MAT_DEPTH(mat->type);
                elem_size = CV_ELEM_SIZE1(depth);
                size = cvGetMatSize(mat);
            }

            if( CV_MAT_DEPTH(mat->type) != depth )
                CV_ERROR( CV_StsUnmatchedFormats, "All the arrays must have the same bit depth" );

            if( mat->cols != size.width || mat->rows != size.height )
                CV_ERROR( CV_StsUnmatchedSizes, "All the arrays must have the same size" );

            if( k == 0 )
            {
                src0[i] = mat->data.ptr;
                cn = CV_MAT_CN(mat->type);
                src_cn[i+1] = src_cn[i] + cn;
                src_step[i] = mat->step / elem_size - size.width * cn;
            }
            else
            {
                dst0[i] = mat->data.ptr;
                cn = CV_MAT_CN(mat->type);
                dst_cn[i+1] = dst_cn[i] + cn;
                dst_step[i] = mat->step / elem_size - size.width * cn;
            }

            cont_flag &= mat->type;
        }
    }

    if( cont_flag )
    {
        size.width *= size.height;
        size.height = 1;
    }

    for( i = 0; i < pair_count; i++ )
    {
        for( k = 0; k < 2; k++ )
        {
            int cn = from_to[i*2 + k];
            const int* cn_arr = k == 0 ? src_cn : dst_cn;
            int a = 0, b = k == 0 ? src_count-1 : dst_count-1;

            if( cn < 0 || cn >= cn_arr[b+1] )
            {
                if( k == 0 && cn < 0 )
                {
                    sptr[i] = 0;
                    sdelta0[i] = sdelta1[i] = 0;
                    continue;
                }
                else
                {
                    char err_str[100];
                    sprintf( err_str, "channel index #%d in the array of pairs is negative "
                        "or exceeds the total number of channels in all the %s arrays", i*2+k,
                        k == 0 ? "input" : "output" );
                    CV_ERROR( CV_StsOutOfRange, err_str );
                }
            }

            for( ; cn >= cn_arr[a+1]; a++ )
                ;
            
            if( k == 0 )
            {
                sptr[i] = src0[a] + (cn - cn_arr[a])*elem_size;
                sdelta1[i] = cn_arr[a+1] - cn_arr[a];
                sdelta0[i] = src_step[a];
            }
            else
            {
                dptr[i] = dst0[a] + (cn - cn_arr[a])*elem_size;
                ddelta1[i] = cn_arr[a+1] - cn_arr[a];
                ddelta0[i] = dst_step[a];
            }
        }
    }

    func = (CvMixChannelsFunc)mixcn_tab.fn_2d[depth];
    if( !func )
        CV_ERROR( CV_StsUnsupportedFormat, "The data type is not supported by the function" );

    IPPI_CALL( func( (const void**)sptr, sdelta0, sdelta1, (void**)dptr,
                     ddelta0, ddelta1, pair_count, size )); 

    __END__;

    if( buffer && heap_alloc )
        cvFree( &buffer );
}


/****************************************************************************************\
*                                   cvConvertScaleAbs                                    *
\****************************************************************************************/

#define ICV_DEF_CVT_SCALE_ABS_CASE( srctype, worktype,                  \
            scale_macro, abs_macro, cast_macro, a, b )                  \
                                                                        \
{                                                                       \
    const srctype* _src = (const srctype*)src;                          \
    srcstep /= sizeof(_src[0]); /*dststep /= sizeof(_dst[0]);*/         \
                                                                        \
    for( ; size.height--; _src += srcstep, dst += dststep )             \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for( i = 0; i <= size.width - 4; i += 4 )                       \
        {                                                               \
            worktype t0 = scale_macro((a)*_src[i] + (b));               \
            worktype t1 = scale_macro((a)*_src[i+1] + (b));             \
                                                                        \
            t0 = (worktype)abs_macro(t0);                               \
            t1 = (worktype)abs_macro(t1);                               \
                                                                        \
            dst[i] = cast_macro(t0);                                    \
            dst[i+1] = cast_macro(t1);                                  \
                                                                        \
            t0 = scale_macro((a)*_src[i+2] + (b));                      \
            t1 = scale_macro((a)*_src[i+3] + (b));                      \
                                                                        \
            t0 = (worktype)abs_macro(t0);                               \
            t1 = (worktype)abs_macro(t1);                               \
                                                                        \
            dst[i+2] = cast_macro(t0);                                  \
            dst[i+3] = cast_macro(t1);                                  \
        }                                                               \
                                                                        \
        for( ; i < size.width; i++ )                                    \
        {                                                               \
            worktype t0 = scale_macro((a)*_src[i] + (b));               \
            t0 = (worktype)abs_macro(t0);                               \
            dst[i] = cast_macro(t0);                                    \
        }                                                               \
    }                                                                   \
}


#define ICV_FIX_SHIFT  15
#define ICV_SCALE(x)   (((x) + (1 << (ICV_FIX_SHIFT-1))) >> ICV_FIX_SHIFT)

static CvStatus CV_STDCALL
icvCvtScaleAbsTo_8u_C1R( const uchar* src, int srcstep,
                         uchar* dst, int dststep,
                         CvSize size, double scale, double shift,
                         int param )
{
    int srctype = param;
    int srcdepth = CV_MAT_DEPTH(srctype);

    size.width *= CV_MAT_CN(srctype);

    switch( srcdepth )
    {
    case  CV_8S:
    case  CV_8U:
        {
        uchar lut[256];
        int i;
        double val = shift;
        
        for( i = 0; i < 128; i++, val += scale )
        {
            int t = cvRound(fabs(val));
            lut[i] = CV_CAST_8U(t);
        }
        
        if( srcdepth == CV_8S )
            val = -val;
        
        for( ; i < 256; i++, val += scale )
        {
            int t = cvRound(fabs(val));
            lut[i] = CV_CAST_8U(t);
        }

        icvLUT_Transform8u_8u_C1R( src, srcstep, dst,
                                   dststep, size, lut );
        }
        break;
    case  CV_16U:
        if( fabs( scale ) <= 1. && fabs(shift) < DBL_EPSILON )
        {
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));

            if( iscale == ICV_FIX_SHIFT )
            {
                ICV_DEF_CVT_SCALE_ABS_CASE( ushort, int, CV_NOP, CV_IABS,
                                            CV_CAST_8U, 1, 0 );
            }
            else
            {
                ICV_DEF_CVT_SCALE_ABS_CASE( ushort, int, ICV_SCALE, CV_IABS,
                                            CV_CAST_8U, iscale, 0 );
            }
        }
        else
        {
            ICV_DEF_CVT_SCALE_ABS_CASE( ushort, int, cvRound, CV_IABS,
                                        CV_CAST_8U, scale, shift );
        }
        break;
    case  CV_16S:
        if( fabs( scale ) <= 1. &&
            fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))
        {
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));

            if( iscale == ICV_FIX_SHIFT && ishift == 0 )
            {
                ICV_DEF_CVT_SCALE_ABS_CASE( short, int, CV_NOP, CV_IABS,
                                            CV_CAST_8U, 1, 0 );
            }
            else
            {
                ICV_DEF_CVT_SCALE_ABS_CASE( short, int, ICV_SCALE, CV_IABS,
                                            CV_CAST_8U, iscale, ishift );
            }
        }
        else
        {
            ICV_DEF_CVT_SCALE_ABS_CASE( short, int, cvRound, CV_IABS,
                                        CV_CAST_8U, scale, shift );
        }
        break;
    case  CV_32S:
        ICV_DEF_CVT_SCALE_ABS_CASE( int, int, cvRound, CV_IABS,
                                    CV_CAST_8U, scale, shift );
        break;
    case  CV_32F:
        ICV_DEF_CVT_SCALE_ABS_CASE( float, int, cvRound, CV_IABS,
                                    CV_CAST_8U, scale, shift );
        break;
    case  CV_64F:
        ICV_DEF_CVT_SCALE_ABS_CASE( double, int, cvRound, CV_IABS,
                                    CV_CAST_8U, scale, shift );
        break;
    default:
        assert(0);
        return CV_BADFLAG_ERR;
    }

    return  CV_OK;
}


CV_IMPL void
cvConvertScaleAbs( const void* srcarr, void* dstarr,
                   double scale, double shift )
{
    CV_FUNCNAME( "cvConvertScaleAbs" );

    __BEGIN__;

    int coi1 = 0, coi2 = 0;
    CvMat  srcstub, *src = (CvMat*)srcarr;
    CvMat  dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_step, dst_step;

    CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( CV_MAT_DEPTH( dst->type ) != CV_8U )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    size = cvGetMatSize( src );
    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }

    IPPI_CALL( icvCvtScaleAbsTo_8u_C1R( src->data.ptr, src_step,
                             (uchar*)(dst->data.ptr), dst_step,
                             size, scale, shift, CV_MAT_TYPE(src->type)));
    __END__;
}

/****************************************************************************************\
*                                      cvConvertScale                                    *
\****************************************************************************************/

#define ICV_DEF_CVT_SCALE_CASE( srctype, worktype,          \
                            scale_macro, cast_macro, a, b ) \
                                                            \
{                                                           \
    const srctype* _src = (const srctype*)src;              \
    srcstep /= sizeof(_src[0]);                             \
                                                            \
    for( ; size.height--; _src += srcstep, dst += dststep ) \
    {                                                       \
        for( i = 0; i <= size.width - 4; i += 4 )           \
        {                                                   \
            worktype t0 = scale_macro((a)*_src[i]+(b));     \
            worktype t1 = scale_macro((a)*_src[i+1]+(b));   \
                                                            \
            dst[i] = cast_macro(t0);                        \
            dst[i+1] = cast_macro(t1);                      \
                                                            \
            t0 = scale_macro((a)*_src[i+2] + (b));          \
            t1 = scale_macro((a)*_src[i+3] + (b));          \
                                                            \
            dst[i+2] = cast_macro(t0);                      \
            dst[i+3] = cast_macro(t1);                      \
        }                                                   \
                                                            \
        for( ; i < size.width; i++ )                        \
        {                                                   \
            worktype t0 = scale_macro((a)*_src[i] + (b));   \
            dst[i] = cast_macro(t0);                        \
        }                                                   \
    }                                                       \
}


#define  ICV_DEF_CVT_SCALE_FUNC_INT( flavor, dsttype, cast_macro )      \
static  CvStatus  CV_STDCALL                                            \
icvCvtScaleTo_##flavor##_C1R( const uchar* src, int srcstep,            \
                              dsttype* dst, int dststep, CvSize size,   \
                              double scale, double shift, int param )   \
{                                                                       \
    int i, srctype = param;                                             \
    dsttype lut[256];                                                   \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case  CV_8U:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            double val = shift;                                         \
            for( i = 0; i < 256; i++, val += scale )                    \
            {                                                           \
                int t = cvRound(val);                                   \
                lut[i] = cast_macro(t);                                 \
            }                                                           \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else if( fabs( scale ) <= 128. &&                               \
                 fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))   \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( uchar, int, ICV_SCALE,              \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( uchar, int, cvRound,                \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_8S:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            for( i = 0; i < 256; i++ )                                  \
            {                                                           \
                int t = cvRound( (char)i*scale + shift );               \
                lut[i] = cast_macro(t);                                 \
            }                                                           \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else if( fabs( scale ) <= 128. &&                               \
                 fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))   \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( char, int, ICV_SCALE,               \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( char, int, cvRound,                 \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16U:                                                       \
        if( fabs( scale ) <= 1. && fabs(shift) < DBL_EPSILON )          \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( ushort, int, ICV_SCALE,             \
                                    cast_macro, iscale, 0 );            \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( ushort, int, cvRound,               \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16S:                                                       \
        if( fabs( scale ) <= 1. &&                                      \
            fabs( shift ) <= (INT_MAX*0.5)/(1 << ICV_FIX_SHIFT))        \
        {                                                               \
            int iscale = cvRound(scale*(1 << ICV_FIX_SHIFT));           \
            int ishift = cvRound(shift*(1 << ICV_FIX_SHIFT));           \
                                                                        \
            ICV_DEF_CVT_SCALE_CASE( short, int, ICV_SCALE,              \
                                    cast_macro, iscale, ishift );       \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( short, int, cvRound,                \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_32S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( int, int, cvRound,                      \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( float, int, cvRound,                    \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_64F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( double, int, cvRound,                   \
                                cast_macro, scale, shift );             \
        break;                                                          \
    default:                                                            \
        assert(0);                                                      \
        return CV_BADFLAG_ERR;                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


#define  ICV_DEF_CVT_SCALE_FUNC_FLT( flavor, dsttype, cast_macro )      \
static  CvStatus  CV_STDCALL                                            \
icvCvtScaleTo_##flavor##_C1R( const uchar* src, int srcstep,            \
                              dsttype* dst, int dststep, CvSize size,   \
                              double scale, double shift, int param )   \
{                                                                       \
    int i, srctype = param;                                             \
    dsttype lut[256];                                                   \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case  CV_8U:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            double val = shift;                                         \
            for( i = 0; i < 256; i++, val += scale )                    \
                lut[i] = (dsttype)val;                                  \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( uchar, double, CV_NOP,              \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_8S:                                                        \
        if( size.width*size.height >= 256 )                             \
        {                                                               \
            for( i = 0; i < 256; i++ )                                  \
                lut[i] = (dsttype)((char)i*scale + shift);              \
                                                                        \
            icvLUT_Transform8u_##flavor##_C1R( src, srcstep, dst,       \
                                dststep*sizeof(dst[0]), size, lut );    \
        }                                                               \
        else                                                            \
        {                                                               \
            ICV_DEF_CVT_SCALE_CASE( char, double, CV_NOP,               \
                                    cast_macro, scale, shift );         \
        }                                                               \
        break;                                                          \
    case  CV_16U:                                                       \
        ICV_DEF_CVT_SCALE_CASE( ushort, double, CV_NOP,                 \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_16S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( short, double, CV_NOP,                  \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32S:                                                       \
        ICV_DEF_CVT_SCALE_CASE( int, double, CV_NOP,                    \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_32F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( float, double, CV_NOP,                  \
                                cast_macro, scale, shift );             \
        break;                                                          \
    case  CV_64F:                                                       \
        ICV_DEF_CVT_SCALE_CASE( double, double, CV_NOP,                 \
                                cast_macro, scale, shift );             \
        break;                                                          \
    default:                                                            \
        assert(0);                                                      \
        return CV_BADFLAG_ERR;                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


ICV_DEF_CVT_SCALE_FUNC_INT( 8u, uchar, CV_CAST_8U )
ICV_DEF_CVT_SCALE_FUNC_INT( 8s, char, CV_CAST_8S )
ICV_DEF_CVT_SCALE_FUNC_INT( 16s, short, CV_CAST_16S )
ICV_DEF_CVT_SCALE_FUNC_INT( 16u, ushort, CV_CAST_16U )
ICV_DEF_CVT_SCALE_FUNC_INT( 32s, int, CV_CAST_32S )

ICV_DEF_CVT_SCALE_FUNC_FLT( 32f, float, CV_CAST_32F )
ICV_DEF_CVT_SCALE_FUNC_FLT( 64f, double, CV_CAST_64F )

CV_DEF_INIT_FUNC_TAB_2D( CvtScaleTo, C1R )


/****************************************************************************************\
*                             Conversion w/o scaling macros                              *
\****************************************************************************************/

#define ICV_DEF_CVT_CASE_2D( srctype, worktype,             \
                             cast_macro1, cast_macro2 )     \
{                                                           \
    const srctype* _src = (const srctype*)src;              \
    srcstep /= sizeof(_src[0]);                             \
                                                            \
    for( ; size.height--; _src += srcstep, dst += dststep ) \
    {                                                       \
        int i;                                              \
                                                            \
        for( i = 0; i <= size.width - 4; i += 4 )           \
        {                                                   \
            worktype t0 = cast_macro1(_src[i]);             \
            worktype t1 = cast_macro1(_src[i+1]);           \
                                                            \
            dst[i] = cast_macro2(t0);                       \
            dst[i+1] = cast_macro2(t1);                     \
                                                            \
            t0 = cast_macro1(_src[i+2]);                    \
            t1 = cast_macro1(_src[i+3]);                    \
                                                            \
            dst[i+2] = cast_macro2(t0);                     \
            dst[i+3] = cast_macro2(t1);                     \
        }                                                   \
                                                            \
        for( ; i < size.width; i++ )                        \
        {                                                   \
            worktype t0 = cast_macro1(_src[i]);             \
            dst[i] = cast_macro2(t0);                       \
        }                                                   \
    }                                                       \
}


#define ICV_DEF_CVT_FUNC_2D( flavor, dsttype, worktype, cast_macro2,    \
                             srcdepth1, srctype1, cast_macro11,         \
                             srcdepth2, srctype2, cast_macro12,         \
                             srcdepth3, srctype3, cast_macro13,         \
                             srcdepth4, srctype4, cast_macro14,         \
                             srcdepth5, srctype5, cast_macro15,         \
                             srcdepth6, srctype6, cast_macro16 )        \
static CvStatus CV_STDCALL                                              \
icvCvtTo_##flavor##_C1R( const uchar* src, int srcstep,                 \
                         dsttype* dst, int dststep,                     \
                         CvSize size, int param )                       \
{                                                                       \
    int srctype = param;                                                \
    dststep /= sizeof(dst[0]);                                          \
                                                                        \
    switch( CV_MAT_DEPTH(srctype) )                                     \
    {                                                                   \
    case srcdepth1:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype1, worktype,                        \
                             cast_macro11, cast_macro2 );               \
        break;                                                          \
    case srcdepth2:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype2, worktype,                        \
                             cast_macro12, cast_macro2 );               \
        break;                                                          \
    case srcdepth3:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype3, worktype,                        \
                             cast_macro13, cast_macro2 );               \
        break;                                                          \
    case srcdepth4:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype4, worktype,                        \
                             cast_macro14, cast_macro2 );               \
        break;                                                          \
    case srcdepth5:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype5, worktype,                        \
                             cast_macro15, cast_macro2 );               \
        break;                                                          \
    case srcdepth6:                                                     \
        ICV_DEF_CVT_CASE_2D( srctype6, worktype,                        \
                             cast_macro16, cast_macro2 );               \
        break;                                                          \
    }                                                                   \
                                                                        \
    return  CV_OK;                                                      \
}


ICV_DEF_CVT_FUNC_2D( 8u, uchar, int, CV_CAST_8U,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 8s, char, int, CV_CAST_8S,
                     CV_8U,  uchar,  CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 16u, ushort, int, CV_CAST_16U,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 16s, short, int, CV_CAST_16S,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 32s, int, int, CV_NOP,
                     CV_8U,  uchar,  CV_NOP,
                     CV_8S,  char,   CV_NOP,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32F, float,  cvRound,
                     CV_64F, double, cvRound )

ICV_DEF_CVT_FUNC_2D( 32f, float, float, CV_NOP,
                     CV_8U,  uchar,  CV_8TO32F,
                     CV_8S,  char,   CV_8TO32F,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_CAST_32F,
                     CV_64F, double, CV_CAST_32F )

ICV_DEF_CVT_FUNC_2D( 64f, double, double, CV_NOP,
                     CV_8U,  uchar,  CV_8TO32F,
                     CV_8S,  char,   CV_8TO32F,
                     CV_16U, ushort, CV_NOP,
                     CV_16S, short,  CV_NOP,
                     CV_32S, int,    CV_NOP,
                     CV_32F, float,  CV_NOP )

CV_DEF_INIT_FUNC_TAB_2D( CvtTo, C1R )


typedef  CvStatus (CV_STDCALL *CvCvtFunc)( const void* src, int srcstep,
                                           void* dst, int dststep, CvSize size,
                                           int param );

typedef  CvStatus (CV_STDCALL *CvCvtScaleFunc)( const void* src, int srcstep,
                                             void* dst, int dststep, CvSize size,
                                             double scale, double shift,
                                             int param );

CV_IMPL void
cvConvertScale( const void* srcarr, void* dstarr,
                double scale, double shift )
{
    static CvFuncTable cvt_tab, cvtscale_tab;
    static int inittab = 0;

    CV_FUNCNAME( "cvConvertScale" );

    __BEGIN__;

    int type;
    int is_nd = 0;
    CvMat  srcstub, *src = (CvMat*)srcarr;
    CvMat  dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_step, dst_step;
    int no_scale = scale == 1 && shift == 0;

    if( !CV_IS_MAT(src) )
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
        {
            int coi = 0;
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
            int coi = 0;
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
        int dsttype;

        CV_CALL( cvInitNArrayIterator( 2, arrs, 0, stubs, &iterator, CV_NO_DEPTH_CHECK ));

        type = iterator.hdr[0]->type;
        dsttype = iterator.hdr[1]->type;
        iterator.size.width *= CV_MAT_CN(type);

        if( !inittab )
        {
            icvInitCvtToC1RTable( &cvt_tab );
            icvInitCvtScaleToC1RTable( &cvtscale_tab );
            inittab = 1;
        }

        if( no_scale )
        {
            CvCvtFunc func = (CvCvtFunc)(cvt_tab.fn_2d[CV_MAT_DEPTH(dsttype)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.size, type ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        else
        {
            CvCvtScaleFunc func =
                (CvCvtScaleFunc)(cvtscale_tab.fn_2d[CV_MAT_DEPTH(dsttype)]);
            if( !func )
                CV_ERROR( CV_StsUnsupportedFormat, "" );

            do
            {
                IPPI_CALL( func( iterator.ptr[0], CV_STUB_STEP,
                                 iterator.ptr[1], CV_STUB_STEP,
                                 iterator.size, scale, shift, type ));
            }
            while( cvNextNArraySlice( &iterator ));
        }
        EXIT;
    }

    if( no_scale && CV_ARE_TYPES_EQ( src, dst ) )
    {
        cvCopy( src, dst );
        EXIT;
    }

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    size = cvGetMatSize( src );
    type = CV_MAT_TYPE(src->type);
    src_step = src->step;
    dst_step = dst->step;

    if( CV_IS_MAT_CONT( src->type & dst->type ))
    {
        size.width *= size.height;
        src_step = dst_step = CV_STUB_STEP;
        size.height = 1;
    }

    size.width *= CV_MAT_CN( type );

    if( CV_ARE_TYPES_EQ( src, dst ) && size.height == 1 &&
        size.width <= CV_MAX_INLINE_MAT_OP_SIZE )
    {
        if( CV_MAT_DEPTH(type) == CV_32F )
        {
            const float* srcdata = (const float*)(src->data.ptr);
            float* dstdata = (float*)(dst->data.ptr);

            do
            {
                dstdata[size.width - 1] = (float)(srcdata[size.width-1]*scale + shift);
            }
            while( --size.width );

            EXIT;
        }

        if( CV_MAT_DEPTH(type) == CV_64F )
        {
            const double* srcdata = (const double*)(src->data.ptr);
            double* dstdata = (double*)(dst->data.ptr);

            do
            {
                dstdata[size.width - 1] = srcdata[size.width-1]*scale + shift;
            }
            while( --size.width );

            EXIT;
        }
    }

    if( !inittab )
    {
        icvInitCvtToC1RTable( &cvt_tab );
        icvInitCvtScaleToC1RTable( &cvtscale_tab );
        inittab = 1;
    }

    if( !CV_ARE_CNS_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( no_scale )
    {
        CvCvtFunc func = (CvCvtFunc)(cvt_tab.fn_2d[CV_MAT_DEPTH(dst->type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                   dst->data.ptr, dst_step, size, type ));
    }
    else
    {
        CvCvtScaleFunc func = (CvCvtScaleFunc)
            (cvtscale_tab.fn_2d[CV_MAT_DEPTH(dst->type)]);

        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );

        IPPI_CALL( func( src->data.ptr, src_step,
                   dst->data.ptr, dst_step, size,
                   scale, shift, type ));
    }

    __END__;
}

/********************* helper functions for converting 32f<->64f ************************/

IPCVAPI_IMPL( CvStatus, icvCvt_32f64f,
    ( const float* src, double* dst, int len ), (src, dst, len) )
{
    int i;
    for( i = 0; i <= len - 4; i += 4 )
    {
        double t0 = src[i];
        double t1 = src[i+1];

        dst[i] = t0;
        dst[i+1] = t1;

        t0 = src[i+2];
        t1 = src[i+3];

        dst[i+2] = t0;
        dst[i+3] = t1;
    }

    for( ; i < len; i++ )
        dst[i] = src[i];

    return CV_OK;
}


IPCVAPI_IMPL( CvStatus, icvCvt_64f32f,
    ( const double* src, float* dst, int len ), (src, dst, len) )
{
    int i = 0;
    for( ; i <= len - 4; i += 4 )
    {
        double t0 = src[i];
        double t1 = src[i+1];

        dst[i] = (float)t0;
        dst[i+1] = (float)t1;

        t0 = src[i+2];
        t1 = src[i+3];

        dst[i+2] = (float)t0;
        dst[i+3] = (float)t1;
    }

    for( ; i < len; i++ )
        dst[i] = (float)src[i];

    return CV_OK;
}


CvStatus CV_STDCALL icvScale_32f( const float* src, float* dst, int len, float a, float b )
{
    int i;
    for( i = 0; i <= len - 4; i += 4 )
    {
        double t0 = src[i]*a + b;
        double t1 = src[i+1]*a + b;

        dst[i] = (float)t0;
        dst[i+1] = (float)t1;

        t0 = src[i+2]*a + b;
        t1 = src[i+3]*a + b;

        dst[i+2] = (float)t0;
        dst[i+3] = (float)t1;
    }

    for( ; i < len; i++ )
        dst[i] = (float)(src[i]*a + b);

    return CV_OK;
}


CvStatus CV_STDCALL icvScale_64f( const double* src, double* dst, int len, double a, double b )
{
    int i;
    for( i = 0; i <= len - 4; i += 4 )
    {
        double t0 = src[i]*a + b;
        double t1 = src[i+1]*a + b;

        dst[i] = t0;
        dst[i+1] = t1;

        t0 = src[i+2]*a + b;
        t1 = src[i+3]*a + b;

        dst[i+2] = t0;
        dst[i+3] = t1;
    }

    for( ; i < len; i++ )
        dst[i] = src[i]*a + b;

    return CV_OK;
}

/* End of file. */
///////////////////////////////////////////////////////////////////////////
//CVDATASTRUCT
//////////////////////////////////////////////////////////////////////////

 #define ICV_FREE_PTR(storage)  \
     ((char*)(storage)->top + (storage)->block_size - (storage)->free_space)
// 
 #define ICV_ALIGNED_SEQ_BLOCK_SIZE  \
     (int)cvAlign(sizeof(CvSeqBlock), CV_STRUCT_ALIGN)
// 
CV_INLINE int
cvAlignLeft( int size, int align )
{
    return size & -align;
}
// 
#define CV_GET_LAST_ELEM( seq, block ) \
    ((block)->data + ((block)->count - 1)*((seq)->elem_size))

#define CV_SWAP_ELEMS(a,b,elem_size)  \
{                                     \
    int k;                            \
    for( k = 0; k < elem_size; k++ )  \
    {                                 \
        char t0 = (a)[k];             \
        char t1 = (b)[k];             \
        (a)[k] = t1;                  \
        (b)[k] = t0;                  \
    }                                 \
}
// 
 #define ICV_SHIFT_TAB_MAX 32
static const char icvPower2ShiftTab[] =
{
    0, 1, -1, 2, -1, -1, -1, 3, -1, -1, -1, -1, -1, -1, -1, 4,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5
};
// 
// /****************************************************************************************\
// *            Functions for manipulating memory storage - list of memory blocks           *
// \****************************************************************************************/
// 
// /* initializes allocated storage */
static void
icvInitMemStorage( CvMemStorage* storage, int block_size )
{
    CV_FUNCNAME( "icvInitMemStorage " );
    
    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( block_size <= 0 )
        block_size = CV_STORAGE_BLOCK_SIZE;

    block_size = cvAlign( block_size, CV_STRUCT_ALIGN );
    assert( sizeof(CvMemBlock) % CV_STRUCT_ALIGN == 0 );

    memset( storage, 0, sizeof( *storage ));
    storage->signature = CV_STORAGE_MAGIC_VAL;
    storage->block_size = block_size;

    __END__;
}
// 
// 
// /* creates root memory storage */
CV_IMPL CvMemStorage*
cvCreateMemStorage( int block_size )
{
    CvMemStorage *storage = 0;

    CV_FUNCNAME( "cvCreateMemStorage" );

    __BEGIN__;

    CV_CALL( storage = (CvMemStorage *)cvAlloc( sizeof( CvMemStorage )));
    CV_CALL( icvInitMemStorage( storage, block_size ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvFree( &storage );

    return storage;
}
// 
// 
// /* creates child memory storage */
CV_IMPL CvMemStorage *
cvCreateChildMemStorage( CvMemStorage * parent )
{
    CvMemStorage *storage = 0;
    CV_FUNCNAME( "cvCreateChildMemStorage" );

    __BEGIN__;

    if( !parent )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( storage = cvCreateMemStorage(parent->block_size));
    storage->parent = parent;

    __END__;

    if( cvGetErrStatus() < 0 )
        cvFree( &storage );

    return storage;
}
// 
// 
// /* releases all blocks of the storage (or returns them to parent if any) */
static void
icvDestroyMemStorage( CvMemStorage* storage )
{
    CV_FUNCNAME( "icvDestroyMemStorage" );

    __BEGIN__;

    int k = 0;

    CvMemBlock *block;
    CvMemBlock *dst_top = 0;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( storage->parent )
        dst_top = storage->parent->top;

    for( block = storage->bottom; block != 0; k++ )
    {
        CvMemBlock *temp = block;

        block = block->next;
        if( storage->parent )
        {
            if( dst_top )
            {
                temp->prev = dst_top;
                temp->next = dst_top->next;
                if( temp->next )
                    temp->next->prev = temp;
                dst_top = dst_top->next = temp;
            }
            else
            {
                dst_top = storage->parent->bottom = storage->parent->top = temp;
                temp->prev = temp->next = 0;
                storage->free_space = storage->block_size - sizeof( *temp );
            }
        }
        else
        {
            cvFree( &temp );
        }
    }

    storage->top = storage->bottom = 0;
    storage->free_space = 0;

    __END__;
}
// 
// 
// /* releases memory storage */
CV_IMPL void
cvReleaseMemStorage( CvMemStorage** storage )
{
    CvMemStorage *st;
    CV_FUNCNAME( "cvReleaseMemStorage" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    st = *storage;
    *storage = 0;

    if( st )
    {
        CV_CALL( icvDestroyMemStorage( st ));
        cvFree( &st );
    }

    __END__;
}


/* clears memory storage (returns blocks to the parent if any) */
CV_IMPL void
cvClearMemStorage( CvMemStorage * storage )
{
    CV_FUNCNAME( "cvClearMemStorage" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( storage->parent )
    {
        icvDestroyMemStorage( storage );
    }
    else
    {
        storage->top = storage->bottom;
        storage->free_space = storage->bottom ? storage->block_size - sizeof(CvMemBlock) : 0;
    }

    __END__;
}
// 
// 
// /* moves stack pointer to next block.
//    If no blocks, allocate new one and link it to the storage */
static void
icvGoNextMemBlock( CvMemStorage * storage )
{
    CV_FUNCNAME( "icvGoNextMemBlock" );
    
    __BEGIN__;
    
    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !storage->top || !storage->top->next )
    {
        CvMemBlock *block;

        if( !(storage->parent) )
        {
            CV_CALL( block = (CvMemBlock *)cvAlloc( storage->block_size ));
        }
        else
        {
            CvMemStorage *parent = storage->parent;
            CvMemStoragePos parent_pos;

            cvSaveMemStoragePos( parent, &parent_pos );
            CV_CALL( icvGoNextMemBlock( parent ));

            block = parent->top;
            cvRestoreMemStoragePos( parent, &parent_pos );

            if( block == parent->top )  /* the single allocated block */
            {
                assert( parent->bottom == block );
                parent->top = parent->bottom = 0;
                parent->free_space = 0;
            }
            else
            {
                /* cut the block from the parent's list of blocks */
                parent->top->next = block->next;
                if( block->next )
                    block->next->prev = parent->top;
            }
        }

        /* link block */
        block->next = 0;
        block->prev = storage->top;

        if( storage->top )
            storage->top->next = block;
        else
            storage->top = storage->bottom = block;
    }

    if( storage->top->next )
        storage->top = storage->top->next;
    storage->free_space = storage->block_size - sizeof(CvMemBlock);
    assert( storage->free_space % CV_STRUCT_ALIGN == 0 );

    __END__;
}
// 
// 
// /* remembers memory storage position */
CV_IMPL void
cvSaveMemStoragePos( const CvMemStorage * storage, CvMemStoragePos * pos )
{
    CV_FUNCNAME( "cvSaveMemStoragePos" );

    __BEGIN__;

    if( !storage || !pos )
        CV_ERROR( CV_StsNullPtr, "" );

    pos->top = storage->top;
    pos->free_space = storage->free_space;

    __END__;
}

// 
// 
// /* restores memory storage position */
CV_IMPL void
cvRestoreMemStoragePos( CvMemStorage * storage, CvMemStoragePos * pos )
{
    CV_FUNCNAME( "cvRestoreMemStoragePos" );

    __BEGIN__;

    if( !storage || !pos )
        CV_ERROR( CV_StsNullPtr, "" );
    if( pos->free_space > storage->block_size )
        CV_ERROR( CV_StsBadSize, "" );

    /*
    // this breaks icvGoNextMemBlock, so comment it off for now
    if( storage->parent && (!pos->top || pos->top->next) )
    {
        CvMemBlock* save_bottom;
        if( !pos->top )
            save_bottom = 0;
        else
        {
            save_bottom = storage->bottom;
            storage->bottom = pos->top->next;
            pos->top->next = 0;
            storage->bottom->prev = 0;
        }
        icvDestroyMemStorage( storage );
        storage->bottom = save_bottom;
    }*/

    storage->top = pos->top;
    storage->free_space = pos->free_space;

    if( !storage->top )
    {
        storage->top = storage->bottom;
        storage->free_space = storage->top ? storage->block_size - sizeof(CvMemBlock) : 0;
    }

    __END__;
}
// 
// 
// /* Allocates continuous buffer of the specified size in the storage */
CV_IMPL void*
cvMemStorageAlloc( CvMemStorage* storage, size_t size )
{
    char *ptr = 0;
    
    CV_FUNCNAME( "cvMemStorageAlloc" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "NULL storage pointer" );

    if( size > INT_MAX )
        CV_ERROR( CV_StsOutOfRange, "Too large memory block is requested" );

    assert( storage->free_space % CV_STRUCT_ALIGN == 0 );

    if( (size_t)storage->free_space < size )
    {
        size_t max_free_space = cvAlignLeft(storage->block_size - sizeof(CvMemBlock), CV_STRUCT_ALIGN);
        if( max_free_space < size )
            CV_ERROR( CV_StsOutOfRange, "requested size is negative or too big" );

        CV_CALL( icvGoNextMemBlock( storage ));
    }

    ptr = ICV_FREE_PTR(storage);
    assert( (size_t)ptr % CV_STRUCT_ALIGN == 0 );
    storage->free_space = cvAlignLeft(storage->free_space - (int)size, CV_STRUCT_ALIGN );

    __END__;

    return ptr;
}


CV_IMPL CvString
cvMemStorageAllocString( CvMemStorage* storage, const char* ptr, int len )
{
    CvString str;
    CV_FUNCNAME( "cvMemStorageAllocString" );

    __BEGIN__;

    str.len = len >= 0 ? len : (int)strlen(ptr);
    CV_CALL( str.ptr = (char*)cvMemStorageAlloc( storage, str.len + 1 ));
    memcpy( str.ptr, ptr, str.len );
    str.ptr[str.len] = '\0';

    __END__;

    return str;
}

// 
// /****************************************************************************************\
// *                               Sequence implementation                                  *
// \****************************************************************************************/
// 
// /* creates empty sequence */
CV_IMPL CvSeq *
cvCreateSeq( int seq_flags, int header_size, int elem_size, CvMemStorage * storage )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvCreateSeq" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( header_size < (int)sizeof( CvSeq ) || elem_size <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    /* allocate sequence header */
    CV_CALL( seq = (CvSeq*)cvMemStorageAlloc( storage, header_size ));
    memset( seq, 0, header_size );

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size )
            CV_ERROR( CV_StsBadSize,
            "Specified element size doesn't match to the size of the specified element type "
            "(try to use 0 for element type)" );
    }
    seq->elem_size = elem_size;
    seq->storage = storage;

    CV_CALL( cvSetSeqBlockSize( seq, (1 << 10)/elem_size ));

    __END__;

    return seq;
}
// 
// 
// /* adjusts <delta_elems> field of sequence. It determines how much the sequence
//    grows if there are no free space inside the sequence buffers */
CV_IMPL void
cvSetSeqBlockSize( CvSeq *seq, int delta_elements )
{
    int elem_size;
    int useful_block_size;

    CV_FUNCNAME( "cvSetSeqBlockSize" );

    __BEGIN__;

    if( !seq || !seq->storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( delta_elements < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    useful_block_size = cvAlignLeft(seq->storage->block_size - sizeof(CvMemBlock) -
                                    sizeof(CvSeqBlock), CV_STRUCT_ALIGN);
    elem_size = seq->elem_size;

    if( delta_elements == 0 )
    {
        delta_elements = (1 << 10) / elem_size;
        delta_elements = MAX( delta_elements, 1 );
    }
    if( delta_elements * elem_size > useful_block_size )
    {
        delta_elements = useful_block_size / elem_size;
        if( delta_elements == 0 )
            CV_ERROR( CV_StsOutOfRange, "Storage block size is too small "
                                        "to fit the sequence elements" );
    }

    seq->delta_elems = delta_elements;

    __END__;
}
// 
// 
/* finds sequence element by its index */
CV_IMPL char*
cvGetSeqElem( const CvSeq *seq, int index )
{
    CvSeqBlock *block;
    int count, total = seq->total;

    if( (unsigned)index >= (unsigned)total )
    {
        index += index < 0 ? total : 0;
        index -= index >= total ? total : 0;
        if( (unsigned)index >= (unsigned)total )
            return 0;
    }

    block = seq->first;
    if( index + index <= total )
    {
        while( index >= (count = block->count) )
        {
            block = block->next;
            index -= count;
        }
    }
    else
    {
        do
        {
            block = block->prev;
            total -= block->count;
        }
        while( index < total );
        index -= total;
    }

    return block->data + index * seq->elem_size;
}

// 
/* calculates index of sequence element */
CV_IMPL int
cvSeqElemIdx( const CvSeq* seq, const void* _element, CvSeqBlock** _block )
{
    const char *element = (const char *)_element;
    int elem_size;
    int id = -1;
    CvSeqBlock *first_block;
    CvSeqBlock *block;

    CV_FUNCNAME( "cvSeqElemIdx" );

    __BEGIN__;

    if( !seq || !element )
        CV_ERROR( CV_StsNullPtr, "" );

    block = first_block = seq->first;
    elem_size = seq->elem_size;

    for( ;; )
    {
        if( (unsigned)(element - block->data) < (unsigned) (block->count * elem_size) )
        {
            if( _block )
                *_block = block;
            if( elem_size <= ICV_SHIFT_TAB_MAX && (id = icvPower2ShiftTab[elem_size - 1]) >= 0 )
                id = (int)((size_t)(element - block->data) >> id);
            else
                id = (int)((size_t)(element - block->data) / elem_size);
            id += block->start_index - seq->first->start_index;
            break;
        }
        block = block->next;
        if( block == first_block )
            break;
    }

    __END__;

    return id;
}


CV_IMPL int
cvSliceLength( CvSlice slice, const CvSeq* seq )
{
    int total = seq->total;
    int length = slice.end_index - slice.start_index;
    
    if( length != 0 )
    {
        if( slice.start_index < 0 )
            slice.start_index += total;
        if( slice.end_index <= 0 )
            slice.end_index += total;

        length = slice.end_index - slice.start_index;
    }

    if( length < 0 )
    {
        length += total;
        /*if( length < 0 )
            length += total;*/
    }
    else if( length > total )
        length = total;

    return length;
}


/* copies all the sequence elements into single continuous array */
CV_IMPL void*
cvCvtSeqToArray( const CvSeq *seq, void *array, CvSlice slice )
{
    CV_FUNCNAME( "cvCvtSeqToArray" );

    __BEGIN__;

    int elem_size, total;
    CvSeqReader reader;
    char *dst = (char*)array;

    if( !seq || !array )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    total = cvSliceLength( slice, seq )*elem_size;
    
    if( total == 0 )
        EXIT;
    
    cvStartReadSeq( seq, &reader, 0 );
    CV_CALL( cvSetSeqReaderPos( &reader, slice.start_index, 0 ));

    do
    {
        int count = (int)(reader.block_max - reader.ptr);
        if( count > total )
            count = total;

        memcpy( dst, reader.ptr, count );
        dst += count;
        reader.block = reader.block->next;
        reader.ptr = reader.block->data;
        reader.block_max = reader.ptr + reader.block->count*elem_size;
        total -= count;
    }
    while( total > 0 );

    __END__;

    return array;
}


/* constructs sequence from array without copying any data.
   the resultant sequence can't grow above its initial size */
CV_IMPL CvSeq*
cvMakeSeqHeaderForArray( int seq_flags, int header_size, int elem_size,
                         void *array, int total, CvSeq *seq, CvSeqBlock * block )
{
    CvSeq* result = 0;
    
    CV_FUNCNAME( "cvMakeSeqHeaderForArray" );

    __BEGIN__;

    if( elem_size <= 0 || header_size < (int)sizeof( CvSeq ) || total < 0 )
        CV_ERROR( CV_StsBadSize, "" );

    if( !seq || ((!array || !block) && total > 0) )
        CV_ERROR( CV_StsNullPtr, "" );

    memset( seq, 0, header_size );

    seq->header_size = header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if( elemtype != CV_SEQ_ELTYPE_GENERIC &&
            typesize != 0 && typesize != elem_size )
            CV_ERROR( CV_StsBadSize,
            "Element size doesn't match to the size of predefined element type "
            "(try to use 0 for sequence element type)" );
    }
    seq->elem_size = elem_size;
    seq->total = total;
    seq->block_max = seq->ptr = (char *) array + total * elem_size;

    if( total > 0 )
    {
        seq->first = block;
        block->prev = block->next = block;
        block->start_index = 0;
        block->count = total;
        block->data = (char *) array;
    }

    result = seq;

    __END__;

    return result;
}

// 
// /* the function allocates space for at least one more sequence element.
//    if there are free sequence blocks (seq->free_blocks != 0),
//    they are reused, otherwise the space is allocated in the storage */
static void
icvGrowSeq( CvSeq *seq, int in_front_of )
{
    CV_FUNCNAME( "icvGrowSeq" );

    __BEGIN__;

    CvSeqBlock *block;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    block = seq->free_blocks;

    if( !block )
    {
        int elem_size = seq->elem_size;
        int delta_elems = seq->delta_elems;
        CvMemStorage *storage = seq->storage;

        if( seq->total >= delta_elems*4 )
            cvSetSeqBlockSize( seq, delta_elems*2 );

        if( !storage )
            CV_ERROR( CV_StsNullPtr, "The sequence has NULL storage pointer" );

        /* if there is a free space just after last allocated block
           and it's big enough then enlarge the last block
           (this can happen only if the new block is added to the end of sequence */
        if( (unsigned)(ICV_FREE_PTR(storage) - seq->block_max) < CV_STRUCT_ALIGN &&
            storage->free_space >= seq->elem_size && !in_front_of )
        {
            int delta = storage->free_space / elem_size;

            delta = MIN( delta, delta_elems ) * elem_size;
            seq->block_max += delta;
            storage->free_space = cvAlignLeft((int)(((char*)storage->top + storage->block_size) -
                                              seq->block_max), CV_STRUCT_ALIGN );
            EXIT;
        }
        else
        {
            int delta = elem_size * delta_elems + ICV_ALIGNED_SEQ_BLOCK_SIZE;

            /* try to allocate <delta_elements> elements */
            if( storage->free_space < delta )
            {
                int small_block_size = MAX(1, delta_elems/3)*elem_size +
                                       ICV_ALIGNED_SEQ_BLOCK_SIZE;
                /* try to allocate smaller part */
                if( storage->free_space >= small_block_size + CV_STRUCT_ALIGN )
                {
                    delta = (storage->free_space - ICV_ALIGNED_SEQ_BLOCK_SIZE)/seq->elem_size;
                    delta = delta*seq->elem_size + ICV_ALIGNED_SEQ_BLOCK_SIZE;
                }
                else
                {
                    CV_CALL( icvGoNextMemBlock( storage ));
                    assert( storage->free_space >= delta );
                }
            }

            CV_CALL( block = (CvSeqBlock*)cvMemStorageAlloc( storage, delta ));
            block->data = (char*)cvAlignPtr( block + 1, CV_STRUCT_ALIGN );
            block->count = delta - ICV_ALIGNED_SEQ_BLOCK_SIZE;
            block->prev = block->next = 0;
        }
    }
    else
    {
        seq->free_blocks = block->next;
    }

    if( !(seq->first) )
    {
        seq->first = block;
        block->prev = block->next = block;
    }
    else
    {
        block->prev = seq->first->prev;
        block->next = seq->first;
        block->prev->next = block->next->prev = block;
    }

    /* for free blocks the <count> field means total number of bytes in the block.
       And for used blocks it means a current number of sequence
       elements in the block */
    assert( block->count % seq->elem_size == 0 && block->count > 0 );

    if( !in_front_of )
    {
        seq->ptr = block->data;
        seq->block_max = block->data + block->count;
        block->start_index = block == block->prev ? 0 :
            block->prev->start_index + block->prev->count;
    }
    else
    {
        int delta = block->count / seq->elem_size;
        block->data += block->count;

        if( block != block->prev )
        {
            assert( seq->first->start_index == 0 );
            seq->first = block;
        }
        else
        {
            seq->block_max = seq->ptr = block->data;
        }

        block->start_index = 0;

        for( ;; )
        {
            block->start_index += delta;
            block = block->next;
            if( block == seq->first )
                break;
        }
    }

    block->count = 0;

    __END__;
}
// 
// /* recycles a sequence block for the further use */
static void
icvFreeSeqBlock( CvSeq *seq, int in_front_of )
{
    /*CV_FUNCNAME( "icvFreeSeqBlock" );*/

    __BEGIN__;

    CvSeqBlock *block = seq->first;

    assert( (in_front_of ? block : block->prev)->count == 0 );

    if( block == block->prev )  /* single block case */
    {
        block->count = (int)(seq->block_max - block->data) + block->start_index * seq->elem_size;
        block->data = seq->block_max - block->count;
        seq->first = 0;
        seq->ptr = seq->block_max = 0;
        seq->total = 0;
    }
    else
    {
        if( !in_front_of )
        {
            block = block->prev;
            assert( seq->ptr == block->data );

            block->count = (int)(seq->block_max - seq->ptr);
            seq->block_max = seq->ptr = block->prev->data +
                block->prev->count * seq->elem_size;
        }
        else
        {
            int delta = block->start_index;

            block->count = delta * seq->elem_size;
            block->data -= block->count;

            /* update start indices of sequence blocks */
            for( ;; )
            {
                block->start_index -= delta;
                block = block->next;
                if( block == seq->first )
                    break;
            }

            seq->first = block->next;
        }

        block->prev->next = block->next;
        block->next->prev = block->prev;
    }

    assert( block->count > 0 && block->count % seq->elem_size == 0 );
    block->next = seq->free_blocks;
    seq->free_blocks = block;

    __END__;
}
// 
// 
// /****************************************************************************************\
// *                             Sequence Writer implementation                             *
// \****************************************************************************************/
// 
// /* initializes sequence writer */
CV_IMPL void
cvStartAppendToSeq( CvSeq *seq, CvSeqWriter * writer )
{
    CV_FUNCNAME( "cvStartAppendToSeq" );

    __BEGIN__;

    if( !seq || !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    memset( writer, 0, sizeof( *writer ));
    writer->header_size = sizeof( CvSeqWriter );

    writer->seq = seq;
    writer->block = seq->first ? seq->first->prev : 0;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}
// 
// 
// /* initializes sequence writer */
CV_IMPL void
cvStartWriteSeq( int seq_flags, int header_size,
                 int elem_size, CvMemStorage * storage, CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvStartWriteSeq" );

    __BEGIN__;

    if( !storage || !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( seq = cvCreateSeq( seq_flags, header_size, elem_size, storage ));
    cvStartAppendToSeq( seq, writer );

    __END__;
}
// 
// 
// /* updates sequence header */
CV_IMPL void
cvFlushSeqWriter( CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvFlushSeqWriter" );

    __BEGIN__;

    if( !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    seq = writer->seq;
    seq->ptr = writer->ptr;

    if( writer->block )
    {
        int total = 0;
        CvSeqBlock *first_block = writer->seq->first;
        CvSeqBlock *block = first_block;

        writer->block->count = (int)((writer->ptr - writer->block->data) / seq->elem_size);
        assert( writer->block->count > 0 );

        do
        {
            total += block->count;
            block = block->next;
        }
        while( block != first_block );

        writer->seq->total = total;
    }

    __END__;
}
// 
// 
// /* calls icvFlushSeqWriter and finishes writing process */
CV_IMPL CvSeq *
cvEndWriteSeq( CvSeqWriter * writer )
{
    CvSeq *seq = 0;

    CV_FUNCNAME( "cvEndWriteSeq" );

    __BEGIN__;

    if( !writer )
        CV_ERROR( CV_StsNullPtr, "" );

    CV_CALL( cvFlushSeqWriter( writer ));
    seq = writer->seq;

    /* truncate the last block */
    if( writer->block && writer->seq->storage )
    {
        CvMemStorage *storage = seq->storage;
        char *storage_block_max = (char *) storage->top + storage->block_size;

        assert( writer->block->count > 0 );

        if( (unsigned)((storage_block_max - storage->free_space)
            - seq->block_max) < CV_STRUCT_ALIGN )
        {
            storage->free_space = cvAlignLeft((int)(storage_block_max - seq->ptr), CV_STRUCT_ALIGN);
            seq->block_max = seq->ptr;
        }
    }

    writer->ptr = 0;

    __END__;

    return seq;
}
// 
// 
/* creates new sequence block */
CV_IMPL void
cvCreateSeqBlock( CvSeqWriter * writer )
{
    CV_FUNCNAME( "cvCreateSeqBlock" );

    __BEGIN__;

    CvSeq *seq;

    if( !writer || !writer->seq )
        CV_ERROR( CV_StsNullPtr, "" );

    seq = writer->seq;

    cvFlushSeqWriter( writer );

    CV_CALL( icvGrowSeq( seq, 0 ));

    writer->block = seq->first->prev;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;

    __END__;
}
// 
// 
// /****************************************************************************************\
// *                               Sequence Reader implementation                           *
// \****************************************************************************************/

/* initializes sequence reader */
CV_IMPL void
cvStartReadSeq( const CvSeq *seq, CvSeqReader * reader, int reverse )
{
    CvSeqBlock *first_block;
    CvSeqBlock *last_block;

    CV_FUNCNAME( "cvStartReadSeq" );

    if( reader )
    {
        reader->seq = 0;
        reader->block = 0;
        reader->ptr = reader->block_max = reader->block_min = 0;
    }

    __BEGIN__;

    if( !seq || !reader )
        CV_ERROR( CV_StsNullPtr, "" );

    reader->header_size = sizeof( CvSeqReader );
    reader->seq = (CvSeq*)seq;

    first_block = seq->first;

    if( first_block )
    {
        last_block = first_block->prev;
        reader->ptr = first_block->data;
        reader->prev_elem = CV_GET_LAST_ELEM( seq, last_block );
        reader->delta_index = seq->first->start_index;

        if( reverse )
        {
            char *temp = reader->ptr;

            reader->ptr = reader->prev_elem;
            reader->prev_elem = temp;

            reader->block = last_block;
        }
        else
        {
            reader->block = first_block;
        }

        reader->block_min = reader->block->data;
        reader->block_max = reader->block_min + reader->block->count * seq->elem_size;
    }
    else
    {
        reader->delta_index = 0;
        reader->block = 0;

        reader->ptr = reader->prev_elem = reader->block_min = reader->block_max = 0;
    }

    __END__;
}


/* changes the current reading block to the previous or to the next */
CV_IMPL void
cvChangeSeqBlock( void* _reader, int direction )
{
    CV_FUNCNAME( "cvChangeSeqBlock" );

    __BEGIN__;

    CvSeqReader* reader = (CvSeqReader*)_reader;
    
    if( !reader )
        CV_ERROR( CV_StsNullPtr, "" );

    if( direction > 0 )
    {
        reader->block = reader->block->next;
        reader->ptr = reader->block->data;
    }
    else
    {
        reader->block = reader->block->prev;
        reader->ptr = CV_GET_LAST_ELEM( reader->seq, reader->block );
    }
    reader->block_min = reader->block->data;
    reader->block_max = reader->block_min + reader->block->count * reader->seq->elem_size;

    __END__;
}
// 
// 
// /* returns the current reader position */
CV_IMPL int
cvGetSeqReaderPos( CvSeqReader* reader )
{
    int elem_size;
    int index = -1;

    CV_FUNCNAME( "cvGetSeqReaderPos" );

    __BEGIN__;

    if( !reader || !reader->ptr )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = reader->seq->elem_size;
    if( elem_size <= ICV_SHIFT_TAB_MAX && (index = icvPower2ShiftTab[elem_size - 1]) >= 0 )
        index = (int)((reader->ptr - reader->block_min) >> index);
    else
        index = (int)((reader->ptr - reader->block_min) / elem_size);

    index += reader->block->start_index - reader->delta_index;

    __END__;

    return index;
}


/* sets reader position to given absolute or relative
   (relatively to the current one) position */
CV_IMPL void
cvSetSeqReaderPos( CvSeqReader* reader, int index, int is_relative )
{
    CV_FUNCNAME( "cvSetSeqReaderPos" );

    __BEGIN__;

    CvSeqBlock *block;
    int elem_size, count, total;

    if( !reader || !reader->seq )
        CV_ERROR( CV_StsNullPtr, "" );

    total = reader->seq->total;
    elem_size = reader->seq->elem_size;

    if( !is_relative )
    {
        if( index < 0 )
        {
            if( index < -total )
                CV_ERROR( CV_StsOutOfRange, "" );
            index += total;
        }
        else if( index >= total )
        {
            index -= total;
            if( index >= total )
                CV_ERROR( CV_StsOutOfRange, "" );
        }

        block = reader->seq->first;
        if( index >= (count = block->count) )
        {
            if( index + index <= total )
            {
                do
                {
                    block = block->next;
                    index -= count;
                }
                while( index >= (count = block->count) );
            }
            else
            {
                do
                {
                    block = block->prev;
                    total -= block->count;
                }
                while( index < total );
                index -= total;
            }
        }
        reader->ptr = block->data + index * elem_size;
        if( reader->block != block )
        {
            reader->block = block;
            reader->block_min = block->data;
            reader->block_max = block->data + block->count * elem_size;
        }
    }
    else
    {
        char* ptr = reader->ptr;
        index *= elem_size;
        block = reader->block;

        if( index > 0 )
        {
            while( ptr + index >= reader->block_max )
            {
                int delta = (int)(reader->block_max - ptr);
                index -= delta;
                reader->block = block = block->next;
                reader->block_min = ptr = block->data;
                reader->block_max = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
        else
        {
            while( ptr + index < reader->block_min )
            {
                int delta = (int)(ptr - reader->block_min);
                index += delta;
                reader->block = block = block->prev;
                reader->block_min = block->data;
                reader->block_max = ptr = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
    }

    __END__;
}


/* pushes element to the sequence */
CV_IMPL char*
cvSeqPush( CvSeq *seq, void *element )
{
    char *ptr = 0;
    size_t elem_size;

    CV_FUNCNAME( "cvSeqPush" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    ptr = seq->ptr;

    if( ptr >= seq->block_max )
    {
        CV_CALL( icvGrowSeq( seq, 0 ));

        ptr = seq->ptr;
        assert( ptr + elem_size <= seq->block_max /*&& ptr == seq->block_min */  );
    }

    if( element )
        CV_MEMCPY_AUTO( ptr, element, elem_size );
    seq->first->prev->count++;
    seq->total++;
    seq->ptr = ptr + elem_size;

    __END__;

    return ptr;
}


/* pops the last element out of the sequence */
CV_IMPL void
cvSeqPop( CvSeq *seq, void *element )
{
    char *ptr;
    int elem_size;

    CV_FUNCNAME( "cvSeqPop" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    if( seq->total <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    elem_size = seq->elem_size;
    seq->ptr = ptr = seq->ptr - elem_size;

    if( element )
        CV_MEMCPY_AUTO( element, ptr, elem_size );
    seq->ptr = ptr;
    seq->total--;

    if( --(seq->first->prev->count) == 0 )
    {
        icvFreeSeqBlock( seq, 0 );
        assert( seq->ptr == seq->block_max );
    }

    __END__;
}


/* pushes element to the front of the sequence */
CV_IMPL char*
cvSeqPushFront( CvSeq *seq, void *element )
{
    char* ptr = 0;
    int elem_size;
    CvSeqBlock *block;

    CV_FUNCNAME( "cvSeqPushFront" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );

    elem_size = seq->elem_size;
    block = seq->first;

    if( !block || block->start_index == 0 )
    {
        CV_CALL( icvGrowSeq( seq, 1 ));

        block = seq->first;
        assert( block->start_index > 0 );
    }

    ptr = block->data -= elem_size;

    if( element )
        CV_MEMCPY_AUTO( ptr, element, elem_size );
    block->count++;
    block->start_index--;
    seq->total++;

    __END__;

    return ptr;
}


/* pulls out the first element of the sequence */
CV_IMPL void
cvSeqPopFront( CvSeq *seq, void *element )
{
    int elem_size;
    CvSeqBlock *block;

    CV_FUNCNAME( "cvSeqPopFront" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    if( seq->total <= 0 )
        CV_ERROR( CV_StsBadSize, "" );

    elem_size = seq->elem_size;
    block = seq->first;

    if( element )
        CV_MEMCPY_AUTO( element, block->data, elem_size );
    block->data += elem_size;
    block->start_index++;
    seq->total--;

    if( --(block->count) == 0 )
    {
        icvFreeSeqBlock( seq, 1 );
    }

    __END__;
}
// 

// 
// /* adds several elements to the end or in the beginning of sequence */
CV_IMPL void
cvSeqPushMulti( CvSeq *seq, void *_elements, int count, int front )
{
    char *elements = (char *) _elements;

    CV_FUNCNAME( "cvSeqPushMulti" );

    __BEGIN__;
    int elem_size;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "NULL sequence pointer" );
    if( count < 0 )
        CV_ERROR( CV_StsBadSize, "number of removed elements is negative" );

    elem_size = seq->elem_size;

    if( !front )
    {
        while( count > 0 )
        {
            int delta = (int)((seq->block_max - seq->ptr) / elem_size);

            delta = MIN( delta, count );
            if( delta > 0 )
            {
                seq->first->prev->count += delta;
                seq->total += delta;
                count -= delta;
                delta *= elem_size;
                if( elements )
                {
                    memcpy( seq->ptr, elements, delta );
                    elements += delta;
                }
                seq->ptr += delta;
            }

            if( count > 0 )
                CV_CALL( icvGrowSeq( seq, 0 ));
        }
    }
    else
    {
        CvSeqBlock* block = seq->first;
        
        while( count > 0 )
        {
            int delta;
            
            if( !block || block->start_index == 0 )
            {
                CV_CALL( icvGrowSeq( seq, 1 ));

                block = seq->first;
                assert( block->start_index > 0 );
            }

            delta = MIN( block->start_index, count );
            count -= delta;
            block->start_index -= delta;
            block->count += delta;
            seq->total += delta;
            delta *= elem_size;
            block->data -= delta;

            if( elements )
                memcpy( block->data, elements + count*elem_size, delta );
        }
    }

    __END__;
}
// 
// 
// /* removes several elements from the end of sequence */
CV_IMPL void
cvSeqPopMulti( CvSeq *seq, void *_elements, int count, int front )
{
    char *elements = (char *) _elements;

    CV_FUNCNAME( "cvSeqPopMulti" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "NULL sequence pointer" );
    if( count < 0 )
        CV_ERROR( CV_StsBadSize, "number of removed elements is negative" );

    count = MIN( count, seq->total );

    if( !front )
    {
        if( elements )
            elements += count * seq->elem_size;

        while( count > 0 )
        {
            int delta = seq->first->prev->count;

            delta = MIN( delta, count );
            assert( delta > 0 );

            seq->first->prev->count -= delta;
            seq->total -= delta;
            count -= delta;
            delta *= seq->elem_size;
            seq->ptr -= delta;

            if( elements )
            {
                elements -= delta;
                memcpy( elements, seq->ptr, delta );
            }

            if( seq->first->prev->count == 0 )
                icvFreeSeqBlock( seq, 0 );
        }
    }
    else
    {
        while( count > 0 )
        {
            int delta = seq->first->count;

            delta = MIN( delta, count );
            assert( delta > 0 );

            seq->first->count -= delta;
            seq->total -= delta;
            count -= delta;
            seq->first->start_index += delta;
            delta *= seq->elem_size;

            if( elements )
            {
                memcpy( elements, seq->first->data, delta );
                elements += delta;
            }

            seq->first->data += delta;
            if( seq->first->count == 0 )
                icvFreeSeqBlock( seq, 1 );
        }
    }

    __END__;
}
// 
// 
// /* removes all elements from the sequence */
CV_IMPL void
cvClearSeq( CvSeq *seq )
{
    CV_FUNCNAME( "cvClearSeq" );

    __BEGIN__;

    if( !seq )
        CV_ERROR( CV_StsNullPtr, "" );
    cvSeqPopMulti( seq, 0, seq->total );

    __END__;
}


CV_IMPL CvSeq*
cvSeqSlice( const CvSeq* seq, CvSlice slice, CvMemStorage* storage, int copy_data )
{
    CvSeq* subseq = 0;
    
    CV_FUNCNAME("cvSeqSlice");

    __BEGIN__;
    
    int elem_size, count, length;
    CvSeqReader reader;
    CvSeqBlock *block, *first_block = 0, *last_block = 0;
    
    if( !CV_IS_SEQ(seq) )
        CV_ERROR( CV_StsBadArg, "Invalid sequence header" );

    if( !storage )
    {
        storage = seq->storage;
        if( !storage )
            CV_ERROR( CV_StsNullPtr, "NULL storage pointer" );
    }

    elem_size = seq->elem_size;
    length = cvSliceLength( slice, seq );
    if( slice.start_index < 0 )
        slice.start_index += seq->total;
    else if( slice.start_index >= seq->total )
        slice.start_index -= seq->total;
    if( (unsigned)length > (unsigned)seq->total ||
        ((unsigned)slice.start_index >= (unsigned)seq->total && length != 0) )
        CV_ERROR( CV_StsOutOfRange, "Bad sequence slice" );

    CV_CALL( subseq = cvCreateSeq( seq->flags, seq->header_size, elem_size, storage ));

    if( length > 0 )
    {
        cvStartReadSeq( seq, &reader, 0 );
        cvSetSeqReaderPos( &reader, slice.start_index, 0 );
        count = (int)((reader.block_max - reader.ptr)/elem_size);

        do
        {
            int bl = MIN( count, length );
            
            if( !copy_data )
            {
                block = (CvSeqBlock*)cvMemStorageAlloc( storage, sizeof(*block) );
                if( !first_block )
                {
                    first_block = subseq->first = block->prev = block->next = block;
                    block->start_index = 0;
                }
                else
                {
                    block->prev = last_block;
                    block->next = first_block;
                    last_block->next = first_block->prev = block;
                    block->start_index = last_block->start_index + last_block->count;
                }
                last_block = block;
                block->data = reader.ptr;
                block->count = bl;
                subseq->total += bl;
            }
            else
                cvSeqPushMulti( subseq, reader.ptr, bl, 0 );
            length -= bl;
            reader.block = reader.block->next;
            reader.ptr = reader.block->data;
            count = reader.block->count;
        }
        while( length > 0 );
    }
    
    __END__;

    return subseq;
}
// 

typedef struct CvSeqReaderPos
{
    CvSeqBlock* block;
    char* ptr;
    char* block_min;
    char* block_max;
}
CvSeqReaderPos;
// 
#define CV_SAVE_READER_POS( reader, pos )   \
{                                           \
    (pos).block = (reader).block;           \
    (pos).ptr = (reader).ptr;               \
    (pos).block_min = (reader).block_min;   \
    (pos).block_max = (reader).block_max;   \
}
// 
#define CV_RESTORE_READER_POS( reader, pos )\
{                                           \
    (reader).block = (pos).block;           \
    (reader).ptr = (pos).ptr;               \
    (reader).block_min = (pos).block_min;   \
    (reader).block_max = (pos).block_max;   \
}
// 
inline char*
icvMed3( char* a, char* b, char* c, CvCmpFunc cmp_func, void* aux )
{
    return cmp_func(a, b, aux) < 0 ?
      (cmp_func(b, c, aux) < 0 ? b : cmp_func(a, c, aux) < 0 ? c : a)
     :(cmp_func(b, c, aux) > 0 ? b : cmp_func(a, c, aux) < 0 ? a : c);
}
// 
CV_IMPL void
cvSeqSort( CvSeq* seq, CvCmpFunc cmp_func, void* aux )
{         
    int elem_size;
    int isort_thresh = 7;
    CvSeqReader left, right;
    int sp = 0;

    struct
    {
        CvSeqReaderPos lb;
        CvSeqReaderPos ub;
    }
    stack[48];

    CV_FUNCNAME( "cvSeqSort" );

    __BEGIN__;
    
    if( !CV_IS_SEQ(seq) )
        CV_ERROR( !seq ? CV_StsNullPtr : CV_StsBadArg, "Bad input sequence" );

    if( !cmp_func )
        CV_ERROR( CV_StsNullPtr, "Null compare function" );

    if( seq->total <= 1 )
        EXIT;

    elem_size = seq->elem_size;
    isort_thresh *= elem_size;

    cvStartReadSeq( seq, &left, 0 );
    right = left;
    CV_SAVE_READER_POS( left, stack[0].lb );
    CV_PREV_SEQ_ELEM( elem_size, right );
    CV_SAVE_READER_POS( right, stack[0].ub );

    while( sp >= 0 )
    {
        CV_RESTORE_READER_POS( left, stack[sp].lb );
        CV_RESTORE_READER_POS( right, stack[sp].ub );
        sp--;

        for(;;)
        {
            int i, n, m;
            CvSeqReader ptr, ptr2;

            if( left.block == right.block )
                n = (int)(right.ptr - left.ptr) + elem_size;
            else
            {
                n = cvGetSeqReaderPos( &right );
                n = (n - cvGetSeqReaderPos( &left ) + 1)*elem_size;
            }

            if( n <= isort_thresh )
            {
            insert_sort:
                ptr = ptr2 = left;
                CV_NEXT_SEQ_ELEM( elem_size, ptr );
                CV_NEXT_SEQ_ELEM( elem_size, right );
                while( ptr.ptr != right.ptr )
                {
                    ptr2.ptr = ptr.ptr;
                    if( ptr2.block != ptr.block )
                    {
                        ptr2.block = ptr.block;
                        ptr2.block_min = ptr.block_min;
                        ptr2.block_max = ptr.block_max;
                    }
                    while( ptr2.ptr != left.ptr )
                    {
                        char* cur = ptr2.ptr;
                        CV_PREV_SEQ_ELEM( elem_size, ptr2 );
                        if( cmp_func( ptr2.ptr, cur, aux ) <= 0 )
                            break;
                        CV_SWAP_ELEMS( ptr2.ptr, cur, elem_size );
                    }
                    CV_NEXT_SEQ_ELEM( elem_size, ptr );
                }
                break;
            }
            else
            {
                CvSeqReader left0, left1, right0, right1;
                CvSeqReader tmp0, tmp1;
                char *m1, *m2, *m3, *pivot;
                int swap_cnt = 0;
                int l, l0, l1, r, r0, r1;

                left0 = tmp0 = left;
                right0 = right1 = right;
                n /= elem_size;
                
                if( n > 40 )
                {
                    int d = n / 8;
                    char *p1, *p2, *p3;
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m1 = icvMed3( p1, p2, p3, cmp_func, aux );
                    cvSetSeqReaderPos( &tmp0, (n/2) - d*3, 1 );
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m2 = icvMed3( p1, p2, p3, cmp_func, aux );
                    cvSetSeqReaderPos( &tmp0, n - 1 - d*3 - n/2, 1 );
                    p1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, d, 1 );
                    p3 = tmp0.ptr;
                    m3 = icvMed3( p1, p2, p3, cmp_func, aux );
                }
                else
                {
                    m1 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, n/2, 1 );
                    m2 = tmp0.ptr;
                    cvSetSeqReaderPos( &tmp0, n - 1 - n/2, 1 );
                    m3 = tmp0.ptr;
                }

                pivot = icvMed3( m1, m2, m3, cmp_func, aux );
                left = left0;
                if( pivot != left.ptr )
                {
                    CV_SWAP_ELEMS( pivot, left.ptr, elem_size );
                    pivot = left.ptr;
                }
                CV_NEXT_SEQ_ELEM( elem_size, left );
                left1 = left;

                for(;;)
                {
                    while( left.ptr != right.ptr && (r = cmp_func(left.ptr, pivot, aux)) <= 0 )
                    {
                        if( r == 0 )
                        {
                            if( left1.ptr != left.ptr )
                                CV_SWAP_ELEMS( left1.ptr, left.ptr, elem_size );
                            swap_cnt = 1;
                            CV_NEXT_SEQ_ELEM( elem_size, left1 );
                        }
                        CV_NEXT_SEQ_ELEM( elem_size, left );
                    }

                    while( left.ptr != right.ptr && (r = cmp_func(right.ptr,pivot, aux)) >= 0 )
                    {
                        if( r == 0 )
                        {
                            if( right1.ptr != right.ptr )
                                CV_SWAP_ELEMS( right1.ptr, right.ptr, elem_size );
                            swap_cnt = 1;
                            CV_PREV_SEQ_ELEM( elem_size, right1 );
                        }
                        CV_PREV_SEQ_ELEM( elem_size, right );
                    }

                    if( left.ptr == right.ptr )
                    {
                        r = cmp_func(left.ptr, pivot, aux);
                        if( r == 0 )
                        {
                            if( left1.ptr != left.ptr )
                                CV_SWAP_ELEMS( left1.ptr, left.ptr, elem_size );
                            swap_cnt = 1;
                            CV_NEXT_SEQ_ELEM( elem_size, left1 );
                        }
                        if( r <= 0 )
                        {
                            CV_NEXT_SEQ_ELEM( elem_size, left );
                        }
                        else
                        {
                            CV_PREV_SEQ_ELEM( elem_size, right );
                        }
                        break;
                    }

                    CV_SWAP_ELEMS( left.ptr, right.ptr, elem_size );
                    CV_NEXT_SEQ_ELEM( elem_size, left );
                    r = left.ptr == right.ptr;
                    CV_PREV_SEQ_ELEM( elem_size, right );
                    swap_cnt = 1;
                    if( r )
                        break;
                }

                if( swap_cnt == 0 )
                {
                    left = left0, right = right0;
                    goto insert_sort;
                }

                l = cvGetSeqReaderPos( &left );
                if( l == 0 )
                    l = seq->total;
                l0 = cvGetSeqReaderPos( &left0 );
                l1 = cvGetSeqReaderPos( &left1 );
                if( l1 == 0 )
                    l1 = seq->total;
                
                n = MIN( l - l1, l1 - l0 );
                if( n > 0 )
                {
                    tmp0 = left0;
                    tmp1 = left;
                    cvSetSeqReaderPos( &tmp1, 0-n, 1 );
                    for( i = 0; i < n; i++ )
                    {
                        CV_SWAP_ELEMS( tmp0.ptr, tmp1.ptr, elem_size );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp0 );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp1 );
                    }
                }

                r = cvGetSeqReaderPos( &right );
                r0 = cvGetSeqReaderPos( &right0 );
                r1 = cvGetSeqReaderPos( &right1 );
                m = MIN( r0 - r1, r1 - r );
                if( m > 0 )
                {
                    tmp0 = left;
                    tmp1 = right0;
                    cvSetSeqReaderPos( &tmp1, 1-m, 1 );
                    for( i = 0; i < m; i++ )
                    {
                        CV_SWAP_ELEMS( tmp0.ptr, tmp1.ptr, elem_size );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp0 );
                        CV_NEXT_SEQ_ELEM( elem_size, tmp1 );
                    }
                }

                n = l - l1;
                m = r1 - r;
                if( n > 1 )
                {
                    if( m > 1 )
                    {
                        if( n > m )
                        {
                            sp++;
                            CV_SAVE_READER_POS( left0, stack[sp].lb );
                            cvSetSeqReaderPos( &left0, n - 1, 1 );
                            CV_SAVE_READER_POS( left0, stack[sp].ub );
                            left = right = right0;
                            cvSetSeqReaderPos( &left, 1 - m, 1 );
                        }
                        else
                        {
                            sp++;
                            CV_SAVE_READER_POS( right0, stack[sp].ub );
                            cvSetSeqReaderPos( &right0, 1 - m, 1 );
                            CV_SAVE_READER_POS( right0, stack[sp].lb );
                            left = right = left0;
                            cvSetSeqReaderPos( &right, n - 1, 1 );
                        }
                    }
                    else
                    {
                        left = right = left0;
                        cvSetSeqReaderPos( &right, n - 1, 1 );
                    }
                }
                else if( m > 1 )
                {
                    left = right = right0;
                    cvSetSeqReaderPos( &left, 1 - m, 1 );
                }
                else
                    break;
            }
        }
    }

    __END__;
}

typedef struct CvPTreeNode
{
    struct CvPTreeNode* parent;
    char* element;
    int rank;
}
CvPTreeNode;
// 
// 
// // the function splits the input sequence or set into one or more equivalence classes.
// // is_equal(a,b,...) returns non-zero if the two sequence elements
// // belong to the same class. the function returns sequence of integers -
// // 0-based class indexes for each element.
// //
// // The algorithm is described in "Introduction to Algorithms"
// // by Cormen, Leiserson and Rivest, chapter "Data structures for disjoint sets"
CV_IMPL  int
cvSeqPartition( const CvSeq* seq, CvMemStorage* storage, CvSeq** labels,
                CvCmpFunc is_equal, void* userdata )
{
    CvSeq* result = 0;
    CvMemStorage* temp_storage = 0;
    int class_idx = 0;
    
    CV_FUNCNAME( "cvSeqPartition" );

    __BEGIN__;

    CvSeqWriter writer;
    CvSeqReader reader, reader0;
    CvSeq* nodes;
    int i, j;
    int is_set; 

    if( !labels )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !seq || !is_equal )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !storage )
        storage = seq->storage;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );

    is_set = CV_IS_SET(seq);

    temp_storage = cvCreateChildMemStorage( storage );

    nodes = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvPTreeNode), temp_storage );

    cvStartReadSeq( seq, &reader );
    memset( &writer, 0, sizeof(writer));
    cvStartAppendToSeq( nodes, &writer ); 

    // Initial O(N) pass. Make a forest of single-vertex trees.
    for( i = 0; i < seq->total; i++ )
    {
        CvPTreeNode node = { 0, 0, 0 };
        if( !is_set || CV_IS_SET_ELEM( reader.ptr ))
            node.element = reader.ptr;
        CV_WRITE_SEQ_ELEM( node, writer );
        CV_NEXT_SEQ_ELEM( seq->elem_size, reader );
    }

    cvEndWriteSeq( &writer );

    // because in the next loop we will iterate
    // through all the sequence nodes each time,
    // we do not need to initialize reader every time
    cvStartReadSeq( nodes, &reader );
    cvStartReadSeq( nodes, &reader0 );

    // The main O(N^2) pass. Merge connected components.
    for( i = 0; i < nodes->total; i++ )
    {
        CvPTreeNode* node = (CvPTreeNode*)(reader0.ptr);
        CvPTreeNode* root = node;
        CV_NEXT_SEQ_ELEM( nodes->elem_size, reader0 );

        if( !node->element )
            continue;

        // find root
        while( root->parent )
            root = root->parent;

        for( j = 0; j < nodes->total; j++ )
        {
            CvPTreeNode* node2 = (CvPTreeNode*)reader.ptr;
            
            if( node2->element && node2 != node &&
                is_equal( node->element, node2->element, userdata ))
            {
                CvPTreeNode* root2 = node2;
                
                // unite both trees
                while( root2->parent )
                    root2 = root2->parent;

                if( root2 != root )
                {
                    if( root->rank > root2->rank )
                        root2->parent = root;
                    else
                    {
                        root->parent = root2;
                        root2->rank += root->rank == root2->rank;
                        root = root2;
                    }
                    assert( root->parent == 0 );

                    // compress path from node2 to the root
                    while( node2->parent )
                    {
                        CvPTreeNode* temp = node2;
                        node2 = node2->parent;
                        temp->parent = root;
                    }

                    // compress path from node to the root
                    node2 = node;
                    while( node2->parent )
                    {
                        CvPTreeNode* temp = node2;
                        node2 = node2->parent;
                        temp->parent = root;
                    }
                }
            }

            CV_NEXT_SEQ_ELEM( sizeof(*node), reader );
        }
    }

    // Final O(N) pass (Enumerate classes)
    // Reuse reader one more time
    result = cvCreateSeq( 0, sizeof(CvSeq), sizeof(int), storage );
    cvStartAppendToSeq( result, &writer );

    for( i = 0; i < nodes->total; i++ )
    {
        CvPTreeNode* node = (CvPTreeNode*)reader.ptr;
        int idx = -1;
        
        if( node->element )
        {
            while( node->parent )
                node = node->parent;
            if( node->rank >= 0 )
                node->rank = ~class_idx++;
            idx = ~node->rank;
        }

        CV_NEXT_SEQ_ELEM( sizeof(*node), reader );
        CV_WRITE_SEQ_ELEM( idx, writer );
    }

    cvEndWriteSeq( &writer );

    __END__;

    if( labels )
        *labels = result;

    cvReleaseMemStorage( &temp_storage );
    return class_idx;
}
// 
// 
// /****************************************************************************************\
// *                                      Set implementation                                *
// \****************************************************************************************/
// 
// /* creates empty set */
CV_IMPL CvSet*
cvCreateSet( int set_flags, int header_size, int elem_size, CvMemStorage * storage )
{
    CvSet *set = 0;

    CV_FUNCNAME( "cvCreateSet" );

    __BEGIN__;

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "" );
    if( header_size < (int)sizeof( CvSet ) ||
        elem_size < (int)sizeof(void*)*2 ||
        (elem_size & (sizeof(void*)-1)) != 0 )
        CV_ERROR( CV_StsBadSize, "" );

    set = (CvSet*) cvCreateSeq( set_flags, header_size, elem_size, storage );
    set->flags = (set->flags & ~CV_MAGIC_MASK) | CV_SET_MAGIC_VAL;

    __END__;

    return set;
}
// 
// 
// /* adds new element to the set */
CV_IMPL int
cvSetAdd( CvSet* set, CvSetElem* element, CvSetElem** inserted_element )
{
    int id = -1;

    CV_FUNCNAME( "cvSetAdd" );

    __BEGIN__;

    CvSetElem *free_elem;

    if( !set )
        CV_ERROR( CV_StsNullPtr, "" );

    if( !(set->free_elems) )
    {
        int count = set->total;
        int elem_size = set->elem_size;
        char *ptr;
        CV_CALL( icvGrowSeq( (CvSeq *) set, 0 ));

        set->free_elems = (CvSetElem*) (ptr = set->ptr);
        for( ; ptr + elem_size <= set->block_max; ptr += elem_size, count++ )
        {
            ((CvSetElem*)ptr)->flags = count | CV_SET_ELEM_FREE_FLAG;
            ((CvSetElem*)ptr)->next_free = (CvSetElem*)(ptr + elem_size);
        }
        assert( count <= CV_SET_ELEM_IDX_MASK+1 );
        ((CvSetElem*)(ptr - elem_size))->next_free = 0;
        set->first->prev->count += count - set->total;
        set->total = count;
        set->ptr = set->block_max;
    }

    free_elem = set->free_elems;
    set->free_elems = free_elem->next_free;

    id = free_elem->flags & CV_SET_ELEM_IDX_MASK;
    if( element )
        CV_MEMCPY_INT( free_elem, element, (size_t)set->elem_size/sizeof(int) );

    free_elem->flags = id;
    set->active_count++;

    if( inserted_element )
        *inserted_element = free_elem;

    __END__;

    return id;
}

// 

// 
// /* removes all elements from the set */
CV_IMPL void
cvClearSet( CvSet* set )
{
    CV_FUNCNAME( "cvClearSet" );

    __BEGIN__;

    CV_CALL( cvClearSeq( (CvSeq*)set ));
    set->free_elems = 0;
    set->active_count = 0;

    __END__;
}

// 
typedef struct CvTreeNode
{
    int       flags;         /* micsellaneous flags */         
    int       header_size;   /* size of sequence header */     
    struct    CvTreeNode* h_prev; /* previous sequence */      
    struct    CvTreeNode* h_next; /* next sequence */          
    struct    CvTreeNode* v_prev; /* 2nd previous sequence */  
    struct    CvTreeNode* v_next; /* 2nd next sequence */
}
CvTreeNode;
// 
// 
// 
// // Insert contour into tree given certain parent sequence.
// // If parent is equal to frame (the most external contour),
// // then added contour will have null pointer to parent.
CV_IMPL void
cvInsertNodeIntoTree( void* _node, void* _parent, void* _frame )
{
    CV_FUNCNAME( "cvInsertNodeIntoTree" );

    __BEGIN__;

    CvTreeNode* node = (CvTreeNode*)_node;
    CvTreeNode* parent = (CvTreeNode*)_parent;

    if( !node || !parent )
        CV_ERROR( CV_StsNullPtr, "" );

    node->v_prev = _parent != _frame ? parent : 0;
    node->h_next = parent->v_next;

    assert( parent->v_next != node );

    if( parent->v_next )
        parent->v_next->h_prev = node;
    parent->v_next = node;

    __END__;
}


// 
CV_IMPL void
cvInitTreeNodeIterator( CvTreeNodeIterator* treeIterator,
                        const void* first, int max_level )
{
    CV_FUNCNAME("icvInitTreeNodeIterator");

    __BEGIN__;
    
    if( !treeIterator || !first )
        CV_ERROR( CV_StsNullPtr, "" );

    if( max_level < 0 )
        CV_ERROR( CV_StsOutOfRange, "" );

    treeIterator->node = (void*)first;
    treeIterator->level = 0;
    treeIterator->max_level = max_level;

    __END__;
}
// 
// 
CV_IMPL void*
cvNextTreeNode( CvTreeNodeIterator* treeIterator )
{
    CvTreeNode* prevNode = 0;
    
    CV_FUNCNAME("cvNextTreeNode");

    __BEGIN__;
    
    CvTreeNode* node;
    int level;

    if( !treeIterator )
        CV_ERROR( CV_StsNullPtr, "NULL iterator pointer" );

    prevNode = node = (CvTreeNode*)treeIterator->node;
    level = treeIterator->level;

    if( node )
    {
        if( node->v_next && level+1 < treeIterator->max_level )
        {
            node = node->v_next;
            level++;
        }
        else
        {
            while( node->h_next == 0 )
            {
                node = node->v_prev;
                if( --level < 0 )
                {
                    node = 0;
                    break;
                }
            }
            node = node && treeIterator->max_level != 0 ? node->h_next : 0;
        }
    }

    treeIterator->node = node;
    treeIterator->level = level;

    __END__;

    return prevNode;
}

// 
// // On Win64 (IA64) optimized versions of DFT and DCT fail the tests
#if defined WIN64 && !defined EM64T
#pragma optimize("", off)
#endif

icvDFTInitAlloc_C_32fc_t icvDFTInitAlloc_C_32fc_p = 0;
icvDFTFree_C_32fc_t icvDFTFree_C_32fc_p = 0;
icvDFTGetBufSize_C_32fc_t icvDFTGetBufSize_C_32fc_p = 0;
icvDFTFwd_CToC_32fc_t icvDFTFwd_CToC_32fc_p = 0;
icvDFTInv_CToC_32fc_t icvDFTInv_CToC_32fc_p = 0;

icvDFTInitAlloc_C_64fc_t icvDFTInitAlloc_C_64fc_p = 0;
icvDFTFree_C_64fc_t icvDFTFree_C_64fc_p = 0;
icvDFTGetBufSize_C_64fc_t icvDFTGetBufSize_C_64fc_p = 0;
icvDFTFwd_CToC_64fc_t icvDFTFwd_CToC_64fc_p = 0;
icvDFTInv_CToC_64fc_t icvDFTInv_CToC_64fc_p = 0;

icvDFTInitAlloc_R_32f_t icvDFTInitAlloc_R_32f_p = 0;
icvDFTFree_R_32f_t icvDFTFree_R_32f_p = 0;
icvDFTGetBufSize_R_32f_t icvDFTGetBufSize_R_32f_p = 0;
icvDFTFwd_RToPack_32f_t icvDFTFwd_RToPack_32f_p = 0;
icvDFTInv_PackToR_32f_t icvDFTInv_PackToR_32f_p = 0;

icvDFTInitAlloc_R_64f_t icvDFTInitAlloc_R_64f_p = 0;
icvDFTFree_R_64f_t icvDFTFree_R_64f_p = 0;
icvDFTGetBufSize_R_64f_t icvDFTGetBufSize_R_64f_p = 0;
icvDFTFwd_RToPack_64f_t icvDFTFwd_RToPack_64f_p = 0;
icvDFTInv_PackToR_64f_t icvDFTInv_PackToR_64f_p = 0;

/*icvDCTFwdInitAlloc_32f_t icvDCTFwdInitAlloc_32f_p = 0;
icvDCTFwdFree_32f_t icvDCTFwdFree_32f_p = 0;
icvDCTFwdGetBufSize_32f_t icvDCTFwdGetBufSize_32f_p = 0;
icvDCTFwd_32f_t icvDCTFwd_32f_p = 0;

icvDCTInvInitAlloc_32f_t icvDCTInvInitAlloc_32f_p = 0;
icvDCTInvFree_32f_t icvDCTInvFree_32f_p = 0;
icvDCTInvGetBufSize_32f_t icvDCTInvGetBufSize_32f_p = 0;
icvDCTInv_32f_t icvDCTInv_32f_p = 0;

icvDCTFwdInitAlloc_64f_t icvDCTFwdInitAlloc_64f_p = 0;
icvDCTFwdFree_64f_t icvDCTFwdFree_64f_p = 0;
icvDCTFwdGetBufSize_64f_t icvDCTFwdGetBufSize_64f_p = 0;
icvDCTFwd_64f_t icvDCTFwd_64f_p = 0;

icvDCTInvInitAlloc_64f_t icvDCTInvInitAlloc_64f_p = 0;
icvDCTInvFree_64f_t icvDCTInvFree_64f_p = 0;
icvDCTInvGetBufSize_64f_t icvDCTInvGetBufSize_64f_p = 0;
icvDCTInv_64f_t icvDCTInv_64f_p = 0;*/

// /****************************************************************************************\
//                                Discrete Fourier Transform
// \****************************************************************************************/
// 
#define CV_MAX_LOCAL_DFT_SIZE  (1 << 15)

static const uchar log2tab[] = { 0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0 };
static int icvlog2( int n )
{
    int m = 0;
    int f = (n >= (1 << 16))*16;
    n >>= f;
    m += f;
    f = (n >= (1 << 8))*8;
    n >>= f;
    m += f;
    f = (n >= (1 << 4))*4;
    n >>= f;
    return m + f + log2tab[n];
}

static unsigned char icvRevTable[] =
{
  0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
  0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
  0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
  0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
  0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
  0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
  0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
  0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
  0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
  0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
  0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
  0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
  0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
  0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
  0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
  0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff
};

static const double icvDxtTab[][2] =
{
{ 1.00000000000000000, 0.00000000000000000 },
{-1.00000000000000000, 0.00000000000000000 },
{ 0.00000000000000000, 1.00000000000000000 },
{ 0.70710678118654757, 0.70710678118654746 },
{ 0.92387953251128674, 0.38268343236508978 },
{ 0.98078528040323043, 0.19509032201612825 },
{ 0.99518472667219693, 0.09801714032956060 },
{ 0.99879545620517241, 0.04906767432741802 },
{ 0.99969881869620425, 0.02454122852291229 },
{ 0.99992470183914450, 0.01227153828571993 },
{ 0.99998117528260111, 0.00613588464915448 },
{ 0.99999529380957619, 0.00306795676296598 },
{ 0.99999882345170188, 0.00153398018628477 },
{ 0.99999970586288223, 0.00076699031874270 },
{ 0.99999992646571789, 0.00038349518757140 },
{ 0.99999998161642933, 0.00019174759731070 },
{ 0.99999999540410733, 0.00009587379909598 },
{ 0.99999999885102686, 0.00004793689960307 },
{ 0.99999999971275666, 0.00002396844980842 },
{ 0.99999999992818922, 0.00001198422490507 },
{ 0.99999999998204725, 0.00000599211245264 },
{ 0.99999999999551181, 0.00000299605622633 },
{ 0.99999999999887801, 0.00000149802811317 },
{ 0.99999999999971945, 0.00000074901405658 },
{ 0.99999999999992983, 0.00000037450702829 },
{ 0.99999999999998246, 0.00000018725351415 },
{ 0.99999999999999567, 0.00000009362675707 },
{ 0.99999999999999889, 0.00000004681337854 },
{ 0.99999999999999978, 0.00000002340668927 },
{ 0.99999999999999989, 0.00000001170334463 },
{ 1.00000000000000000, 0.00000000585167232 },
{ 1.00000000000000000, 0.00000000292583616 }
};
// 
#define icvBitRev(i,shift) \
   ((int)((((unsigned)icvRevTable[(i)&255] << 24)+ \
           ((unsigned)icvRevTable[((i)>> 8)&255] << 16)+ \
           ((unsigned)icvRevTable[((i)>>16)&255] <<  8)+ \
           ((unsigned)icvRevTable[((i)>>24)])) >> (shift)))

static int
icvDFTFactorize( int n, int* factors )
{
    int nf = 0, f, i, j;

    if( n <= 5 )
    {
        factors[0] = n;
        return 1;
    }
    
    f = (((n - 1)^n)+1) >> 1;
    if( f > 1 )
    {
        factors[nf++] = f;
        n = f == n ? 1 : n/f;
    }

    for( f = 3; n > 1; )
    {
        int d = n/f;
        if( d*f == n )
        {
            factors[nf++] = f;
            n = d;
        }
        else
        {
            f += 2;
            if( f*f > n )
                break;
        }
    }

    if( n > 1 )
        factors[nf++] = n;

    f = (factors[0] & 1) == 0;
    for( i = f; i < (nf+f)/2; i++ )
        CV_SWAP( factors[i], factors[nf-i-1+f], j );

    return nf;
}
// 
static void
icvDFTInit( int n0, int nf, int* factors, int* itab, int elem_size, void* _wave, int inv_itab )
{
    int digits[34], radix[34];
    int n = factors[0], m = 0;
    int* itab0 = itab;
    int i, j, k;
    CvComplex64f w, w1;
    double t;

    if( n0 <= 5 )
    {
        itab[0] = 0;
        itab[n0-1] = n0-1;
        
        if( n0 != 4 )
        {
            for( i = 1; i < n0-1; i++ )
                itab[i] = i;
        }
        else
        {
            itab[1] = 2;
            itab[2] = 1;
        }
        if( n0 == 5 )
        {
            if( elem_size == sizeof(CvComplex64f) )
                ((CvComplex64f*)_wave)[0] = CvComplex64f(1.,0.);
            else
                ((CvComplex32f*)_wave)[0] = CvComplex32f(1.f,0.f);
        }
        if( n0 != 4 )
            return;
        m = 2;
    }
    else
    {
        radix[nf] = 1;
        digits[nf] = 0;
        for( i = 0; i < nf; i++ )
        {
            digits[i] = 0;
            radix[nf-i-1] = radix[nf-i]*factors[nf-i-1];
        }

        if( inv_itab && factors[0] != factors[nf-1] )
            itab = (int*)_wave;

        if( (n & 1) == 0 )
        {
            int a = radix[1], na2 = n*a>>1, na4 = na2 >> 1;
            m = icvlog2(n);
        
            if( n <= 2  )
            {
                itab[0] = 0;
                itab[1] = na2;
            }
            else if( n <= 256 )
            {
                int shift = 10 - m;
                for( i = 0; i <= n - 4; i += 4 )
                {
                    j = (icvRevTable[i>>2]>>shift)*a;
                    itab[i] = j;
                    itab[i+1] = j + na2;
                    itab[i+2] = j + na4;
                    itab[i+3] = j + na2 + na4;
                }
            }
            else
            {
                int shift = 34 - m;
                for( i = 0; i < n; i += 4 )
                {
                    int i4 = i >> 2;
                    j = icvBitRev(i4,shift)*a;
                    itab[i] = j;
                    itab[i+1] = j + na2;
                    itab[i+2] = j + na4;
                    itab[i+3] = j + na2 + na4;
                }
            }

            digits[1]++;
            for( i = n, j = radix[2]; i < n0; )
            {
                for( k = 0; k < n; k++ )
                    itab[i+k] = itab[k] + j;
                if( (i += n) >= n0 )
                    break;
                j += radix[2];
                for( k = 1; ++digits[k] >= factors[k]; k++ )
                {
                    digits[k] = 0;
                    j += radix[k+2] - radix[k];
                }
            }
        }
        else
        {
            for( i = 0, j = 0;; )
            {
                itab[i] = j;
                if( ++i >= n0 )
                    break;
                j += radix[1];
                for( k = 0; ++digits[k] >= factors[k]; k++ )
                {
                    digits[k] = 0;
                    j += radix[k+2] - radix[k];
                }
            }
        }

        if( itab != itab0 )
        {
            itab0[0] = 0;
            for( i = n0 & 1; i < n0; i += 2 )
            {
                int k0 = itab[i];
                int k1 = itab[i+1];
                itab0[k0] = i;
                itab0[k1] = i+1;
            }
        }
    }

    if( (n0 & (n0-1)) == 0 )
    {
        w.re = w1.re = icvDxtTab[m][0];
        w.im = w1.im = -icvDxtTab[m][1];
    }
    else
    {
        t = -CV_PI*2/n0;
        w.im = w1.im = sin(t);
        w.re = w1.re = sqrt(1. - w1.im*w1.im);
    }
    n = (n0+1)/2;

    if( elem_size == sizeof(CvComplex64f) )
    {
        CvComplex64f* wave = (CvComplex64f*)_wave;

        wave[0].re = 1.;
        wave[0].im = 0.;

        if( (n0 & 1) == 0 )
        {
            wave[n].re = -1.;
            wave[n].im = 0;
        }

        for( i = 1; i < n; i++ )
        {
            wave[i] = w;
            wave[n0-i].re = w.re;
            wave[n0-i].im = -w.im;

            t = w.re*w1.re - w.im*w1.im;
            w.im = w.re*w1.im + w.im*w1.re;
            w.re = t;
        }
    }
    else
    {
        CvComplex32f* wave = (CvComplex32f*)_wave;
        assert( elem_size == sizeof(CvComplex32f) );
        
        wave[0].re = 1.f;
        wave[0].im = 0.f;

        if( (n0 & 1) == 0 )
        {
            wave[n].re = -1.f;
            wave[n].im = 0.f;
        }

        for( i = 1; i < n; i++ )
        {
            wave[i].re = (float)w.re;
            wave[i].im = (float)w.im;
            wave[n0-i].re = (float)w.re;
            wave[n0-i].im = (float)-w.im;

            t = w.re*w1.re - w.im*w1.im;
            w.im = w.re*w1.im + w.im*w1.re;
            w.re = t;
        }
    }
}
// 
// 
static const double icv_sin_120 = 0.86602540378443864676372317075294;
static const double icv_sin_45 = 0.70710678118654752440084436210485;
static const double icv_fft5_2 = 0.559016994374947424102293417182819;
static const double icv_fft5_3 = -0.951056516295153572116439333379382;
static const double icv_fft5_4 = -1.538841768587626701285145288018455;
static const double icv_fft5_5 = 0.363271264002680442947733378740309;
// 
 #define ICV_DFT_NO_PERMUTE 2
 #define ICV_DFT_COMPLEX_INPUT_OR_OUTPUT 4
// 
// // mixed-radix complex discrete Fourier transform: double-precision version
static CvStatus CV_STDCALL
icvDFT_64fc( const CvComplex64f* src, CvComplex64f* dst, int n,
             int nf, int* factors, const int* itab,
             const CvComplex64f* wave, int tab_size,
             const void* spec, CvComplex64f* buf,
             int flags, double scale )
{
    int n0 = n, f_idx, nx;
    int inv = flags & CV_DXT_INVERSE;
    int dw0 = tab_size, dw;
    int i, j, k;
    CvComplex64f t;
    int tab_step;

    if( spec )
    {
        assert( icvDFTFwd_CToC_64fc_p != 0 && icvDFTInv_CToC_64fc_p != 0 );
        return !inv ?
            icvDFTFwd_CToC_64fc_p( src, dst, spec, buf ):
            icvDFTInv_CToC_64fc_p( src, dst, spec, buf );
    }

    tab_step = tab_size == n ? 1 : tab_size == n*2 ? 2 : tab_size/n;

    // 0. shuffle data
    if( dst != src )
    {
        assert( (flags & ICV_DFT_NO_PERMUTE) == 0 );
        if( !inv )
        {
            for( i = 0; i <= n - 2; i += 2, itab += 2*tab_step )
            {
                int k0 = itab[0], k1 = itab[tab_step];
                assert( (unsigned)k0 < (unsigned)n && (unsigned)k1 < (unsigned)n );
                dst[i] = src[k0]; dst[i+1] = src[k1];
            }

            if( i < n )
                dst[n-1] = src[n-1];
        }
        else
        {
            for( i = 0; i <= n - 2; i += 2, itab += 2*tab_step )
            {
                int k0 = itab[0], k1 = itab[tab_step];
                assert( (unsigned)k0 < (unsigned)n && (unsigned)k1 < (unsigned)n );
                t.re = src[k0].re; t.im = -src[k0].im;
                dst[i] = t;
                t.re = src[k1].re; t.im = -src[k1].im;
                dst[i+1] = t;
            }

            if( i < n )
            {
                t.re = src[n-1].re; t.im = -src[n-1].im;
                dst[i] = t;
            }
        }
    }
    else
    {
        if( (flags & ICV_DFT_NO_PERMUTE) == 0 )
        {
            if( factors[0] != factors[nf-1] )
                return CV_INPLACE_NOT_SUPPORTED_ERR;
            if( nf == 1 )
            {
                if( (n & 3) == 0 )
                {
                    int n2 = n/2;
                    CvComplex64f* dsth = dst + n2;
                
                    for( i = 0; i < n2; i += 2, itab += tab_step*2 )
                    {
                        j = itab[0];
                        assert( (unsigned)j < (unsigned)n2 );

                        CV_SWAP(dst[i+1], dsth[j], t);
                        if( j > i )
                        {
                            CV_SWAP(dst[i], dst[j], t);
                            CV_SWAP(dsth[i+1], dsth[j+1], t);
                        }
                    }
                }
                // else do nothing
            }
            else
            {
                for( i = 0; i < n; i++, itab += tab_step )
                {
                    j = itab[0];
                    assert( (unsigned)j < (unsigned)n );
                    if( j > i )
                        CV_SWAP(dst[i], dst[j], t);
                }
            }
        }

        if( inv )
        {
            for( i = 0; i <= n - 2; i += 2 )
            {
                double t0 = -dst[i].im;
                double t1 = -dst[i+1].im;
                dst[i].im = t0; dst[i+1].im = t1;
            }

            if( i < n )
                dst[n-1].im = -dst[n-1].im;
        }
    }

    n = 1;
    // 1. power-2 transforms
    if( (factors[0] & 1) == 0 )
    {
        // radix-4 transform
        for( ; n*4 <= factors[0]; )
        {
            nx = n;
            n *= 4;
            dw0 /= 4;

            for( i = 0; i < n0; i += n )
            {
                CvComplex64f* v0;
                CvComplex64f* v1;
                double r0, i0, r1, i1, r2, i2, r3, i3, r4, i4;

                v0 = dst + i;
                v1 = v0 + nx*2;

                r2 = v0[0].re; i2 = v0[0].im;
                r1 = v0[nx].re; i1 = v0[nx].im;
                
                r0 = r1 + r2; i0 = i1 + i2;
                r2 -= r1; i2 -= i1;

                i3 = v1[nx].re; r3 = v1[nx].im;
                i4 = v1[0].re; r4 = v1[0].im;

                r1 = i4 + i3; i1 = r4 + r3;
                r3 = r4 - r3; i3 = i3 - i4;

                v0[0].re = r0 + r1; v0[0].im = i0 + i1;
                v1[0].re = r0 - r1; v1[0].im = i0 - i1;
                v0[nx].re = r2 + r3; v0[nx].im = i2 + i3;
                v1[nx].re = r2 - r3; v1[nx].im = i2 - i3;

                for( j = 1, dw = dw0; j < nx; j++, dw += dw0 )
                {
                    v0 = dst + i + j;
                    v1 = v0 + nx*2;

                    r2 = v0[nx].re*wave[dw*2].re - v0[nx].im*wave[dw*2].im;
                    i2 = v0[nx].re*wave[dw*2].im + v0[nx].im*wave[dw*2].re;
                    r0 = v1[0].re*wave[dw].im + v1[0].im*wave[dw].re;
                    i0 = v1[0].re*wave[dw].re - v1[0].im*wave[dw].im;
                    r3 = v1[nx].re*wave[dw*3].im + v1[nx].im*wave[dw*3].re;
                    i3 = v1[nx].re*wave[dw*3].re - v1[nx].im*wave[dw*3].im;

                    r1 = i0 + i3; i1 = r0 + r3;
                    r3 = r0 - r3; i3 = i3 - i0;
                    r4 = v0[0].re; i4 = v0[0].im;

                    r0 = r4 + r2; i0 = i4 + i2;
                    r2 = r4 - r2; i2 = i4 - i2;

                    v0[0].re = r0 + r1; v0[0].im = i0 + i1;
                    v1[0].re = r0 - r1; v1[0].im = i0 - i1;
                    v0[nx].re = r2 + r3; v0[nx].im = i2 + i3;
                    v1[nx].re = r2 - r3; v1[nx].im = i2 - i3;
                }
            }
        }

        for( ; n < factors[0]; )
        {
            // do the remaining radix-2 transform
            nx = n;
            n *= 2;
            dw0 /= 2;

            for( i = 0; i < n0; i += n )
            {
                CvComplex64f* v = dst + i;
                double r0 = v[0].re + v[nx].re;
                double i0 = v[0].im + v[nx].im;
                double r1 = v[0].re - v[nx].re;
                double i1 = v[0].im - v[nx].im;
                v[0].re = r0; v[0].im = i0;
                v[nx].re = r1; v[nx].im = i1;

                for( j = 1, dw = dw0; j < nx; j++, dw += dw0 )
                {
                    v = dst + i + j;
                    r1 = v[nx].re*wave[dw].re - v[nx].im*wave[dw].im;
                    i1 = v[nx].im*wave[dw].re + v[nx].re*wave[dw].im;
                    r0 = v[0].re; i0 = v[0].im;

                    v[0].re = r0 + r1; v[0].im = i0 + i1;
                    v[nx].re = r0 - r1; v[nx].im = i0 - i1;
                }
            }
        }
    }

    // 2. all the other transforms
    for( f_idx = (factors[0]&1) ? 0 : 1; f_idx < nf; f_idx++ )
    {
        int factor = factors[f_idx];
        nx = n;
        n *= factor;
        dw0 /= factor;

        if( factor == 3 )
        {
            // radix-3
            for( i = 0; i < n0; i += n )
            {
                CvComplex64f* v = dst + i;

                double r1 = v[nx].re + v[nx*2].re;
                double i1 = v[nx].im + v[nx*2].im;
                double r0 = v[0].re;
                double i0 = v[0].im;
                double r2 = icv_sin_120*(v[nx].im - v[nx*2].im);
                double i2 = icv_sin_120*(v[nx*2].re - v[nx].re);
                v[0].re = r0 + r1; v[0].im = i0 + i1;
                r0 -= 0.5*r1; i0 -= 0.5*i1;
                v[nx].re = r0 + r2; v[nx].im = i0 + i2;
                v[nx*2].re = r0 - r2; v[nx*2].im = i0 - i2;

                for( j = 1, dw = dw0; j < nx; j++, dw += dw0 )
                {
                    v = dst + i + j;
                    r0 = v[nx].re*wave[dw].re - v[nx].im*wave[dw].im;
                    i0 = v[nx].re*wave[dw].im + v[nx].im*wave[dw].re;
                    i2 = v[nx*2].re*wave[dw*2].re - v[nx*2].im*wave[dw*2].im;
                    r2 = v[nx*2].re*wave[dw*2].im + v[nx*2].im*wave[dw*2].re;
                    r1 = r0 + i2; i1 = i0 + r2;
                    
                    r2 = icv_sin_120*(i0 - r2); i2 = icv_sin_120*(i2 - r0);
                    r0 = v[0].re; i0 = v[0].im;
                    v[0].re = r0 + r1; v[0].im = i0 + i1;
                    r0 -= 0.5*r1; i0 -= 0.5*i1;
                    v[nx].re = r0 + r2; v[nx].im = i0 + i2;
                    v[nx*2].re = r0 - r2; v[nx*2].im = i0 - i2;
                }
            }
        }
        else if( factor == 5 )
        {
            // radix-5
            for( i = 0; i < n0; i += n )
            {
                for( j = 0, dw = 0; j < nx; j++, dw += dw0 )
                {
                    CvComplex64f* v0 = dst + i + j;
                    CvComplex64f* v1 = v0 + nx*2;
                    CvComplex64f* v2 = v1 + nx*2;

                    double r0, i0, r1, i1, r2, i2, r3, i3, r4, i4, r5, i5;

                    r3 = v0[nx].re*wave[dw].re - v0[nx].im*wave[dw].im;
                    i3 = v0[nx].re*wave[dw].im + v0[nx].im*wave[dw].re;
                    r2 = v2[0].re*wave[dw*4].re - v2[0].im*wave[dw*4].im;
                    i2 = v2[0].re*wave[dw*4].im + v2[0].im*wave[dw*4].re;

                    r1 = r3 + r2; i1 = i3 + i2;
                    r3 -= r2; i3 -= i2;

                    r4 = v1[nx].re*wave[dw*3].re - v1[nx].im*wave[dw*3].im;
                    i4 = v1[nx].re*wave[dw*3].im + v1[nx].im*wave[dw*3].re;
                    r0 = v1[0].re*wave[dw*2].re - v1[0].im*wave[dw*2].im;
                    i0 = v1[0].re*wave[dw*2].im + v1[0].im*wave[dw*2].re;

                    r2 = r4 + r0; i2 = i4 + i0;
                    r4 -= r0; i4 -= i0;

                    r0 = v0[0].re; i0 = v0[0].im;
                    r5 = r1 + r2; i5 = i1 + i2;

                    v0[0].re = r0 + r5; v0[0].im = i0 + i5;

                    r0 -= 0.25*r5; i0 -= 0.25*i5;
                    r1 = icv_fft5_2*(r1 - r2); i1 = icv_fft5_2*(i1 - i2);
                    r2 = -icv_fft5_3*(i3 + i4); i2 = icv_fft5_3*(r3 + r4);

                    i3 *= -icv_fft5_5; r3 *= icv_fft5_5;
                    i4 *= -icv_fft5_4; r4 *= icv_fft5_4;

                    r5 = r2 + i3; i5 = i2 + r3;
                    r2 -= i4; i2 -= r4;
                    
                    r3 = r0 + r1; i3 = i0 + i1;
                    r0 -= r1; i0 -= i1;

                    v0[nx].re = r3 + r2; v0[nx].im = i3 + i2;
                    v2[0].re = r3 - r2; v2[0].im = i3 - i2;

                    v1[0].re = r0 + r5; v1[0].im = i0 + i5;
                    v1[nx].re = r0 - r5; v1[nx].im = i0 - i5;
                }
            }
        }
        else
        {
            // radix-"factor" - an odd number
            int p, q, factor2 = (factor - 1)/2;
            int d, dd, dw_f = tab_size/factor;
            CvComplex64f* a = buf;
            CvComplex64f* b = buf + factor2;

            for( i = 0; i < n0; i += n )
            {
                for( j = 0, dw = 0; j < nx; j++, dw += dw0 )
                {
                    CvComplex64f* v = dst + i + j;
                    CvComplex64f v_0 = v[0];
                    CvComplex64f vn_0 = v_0;

                    if( j == 0 )
                    {
                        for( p = 1, k = nx; p <= factor2; p++, k += nx )
                        {
                            double r0 = v[k].re + v[n-k].re;
                            double i0 = v[k].im - v[n-k].im;
                            double r1 = v[k].re - v[n-k].re;
                            double i1 = v[k].im + v[n-k].im;

                            vn_0.re += r0; vn_0.im += i1;
                            a[p-1].re = r0; a[p-1].im = i0;
                            b[p-1].re = r1; b[p-1].im = i1;
                        }
                    }
                    else
                    {
                        const CvComplex64f* wave_ = wave + dw*factor;
                        d = dw;

                        for( p = 1, k = nx; p <= factor2; p++, k += nx, d += dw )
                        {
                            double r2 = v[k].re*wave[d].re - v[k].im*wave[d].im;
                            double i2 = v[k].re*wave[d].im + v[k].im*wave[d].re;

                            double r1 = v[n-k].re*wave_[-d].re - v[n-k].im*wave_[-d].im;
                            double i1 = v[n-k].re*wave_[-d].im + v[n-k].im*wave_[-d].re;
                    
                            double r0 = r2 + r1;
                            double i0 = i2 - i1;
                            r1 = r2 - r1;
                            i1 = i2 + i1;

                            vn_0.re += r0; vn_0.im += i1;
                            a[p-1].re = r0; a[p-1].im = i0;
                            b[p-1].re = r1; b[p-1].im = i1;
                        }
                    }

                    v[0] = vn_0;

                    for( p = 1, k = nx; p <= factor2; p++, k += nx )
                    {
                        CvComplex64f s0 = v_0, s1 = v_0;
                        d = dd = dw_f*p;

                        for( q = 0; q < factor2; q++ )
                        {
                            double r0 = wave[d].re * a[q].re;
                            double i0 = wave[d].im * a[q].im;
                            double r1 = wave[d].re * b[q].im;
                            double i1 = wave[d].im * b[q].re;
            
                            s1.re += r0 + i0; s0.re += r0 - i0;
                            s1.im += r1 - i1; s0.im += r1 + i1;

                            d += dd;
                            d -= -(d >= tab_size) & tab_size;
                        }

                        v[k] = s0;
                        v[n-k] = s1;
                    }
                }
            }
        }
    }

    if( fabs(scale - 1.) > DBL_EPSILON )
    {
        double re_scale = scale, im_scale = scale;
        if( inv )
            im_scale = -im_scale;

        for( i = 0; i < n0; i++ )
        {
            double t0 = dst[i].re*re_scale;
            double t1 = dst[i].im*im_scale;
            dst[i].re = t0;
            dst[i].im = t1;
        }
    }
    else if( inv )
    {
        for( i = 0; i <= n0 - 2; i += 2 )
        {
            double t0 = -dst[i].im;
            double t1 = -dst[i+1].im;
            dst[i].im = t0;
            dst[i+1].im = t1;
        }

        if( i < n0 )
            dst[n0-1].im = -dst[n0-1].im;
    }

    return CV_OK;
}
// 
// 
// mixed-radix complex discrete Fourier transform: single-precision version
static CvStatus CV_STDCALL
icvDFT_32fc( const CvComplex32f* src, CvComplex32f* dst, int n,
             int nf, int* factors, const int* itab,
             const CvComplex32f* wave, int tab_size,
             const void* spec, CvComplex32f* buf,
             int flags, double scale )
{
    int n0 = n, f_idx, nx;
    int inv = flags & CV_DXT_INVERSE;
    int dw0 = tab_size, dw;
    int i, j, k;
    CvComplex32f t;
    int tab_step = tab_size == n ? 1 : tab_size == n*2 ? 2 : tab_size/n;

    if( spec )
    {
        assert( icvDFTFwd_CToC_32fc_p != 0 && icvDFTInv_CToC_32fc_p != 0 );
        return !inv ?
            icvDFTFwd_CToC_32fc_p( src, dst, spec, buf ):
            icvDFTInv_CToC_32fc_p( src, dst, spec, buf );
    }

    // 0. shuffle data
    if( dst != src )
    {
        assert( (flags & ICV_DFT_NO_PERMUTE) == 0 );
        if( !inv )
        {
            for( i = 0; i <= n - 2; i += 2, itab += 2*tab_step )
            {
                int k0 = itab[0], k1 = itab[tab_step];
                assert( (unsigned)k0 < (unsigned)n && (unsigned)k1 < (unsigned)n );
                dst[i] = src[k0]; dst[i+1] = src[k1];
            }

            if( i < n )
                dst[n-1] = src[n-1];
        }
        else
        {
            for( i = 0; i <= n - 2; i += 2, itab += 2*tab_step )
            {
                int k0 = itab[0], k1 = itab[tab_step];
                assert( (unsigned)k0 < (unsigned)n && (unsigned)k1 < (unsigned)n );
                t.re = src[k0].re; t.im = -src[k0].im;
                dst[i] = t;
                t.re = src[k1].re; t.im = -src[k1].im;
                dst[i+1] = t;
            }

            if( i < n )
            {
                t.re = src[n-1].re; t.im = -src[n-1].im;
                dst[i] = t;
            }
        }
    }
    else
    {
        if( (flags & ICV_DFT_NO_PERMUTE) == 0 )
        {
            if( factors[0] != factors[nf-1] )
                return CV_INPLACE_NOT_SUPPORTED_ERR;
            if( nf == 1 )
            {
                if( (n & 3) == 0 )
                {
                    int n2 = n/2;
                    CvComplex32f* dsth = dst + n2;
                
                    for( i = 0; i < n2; i += 2, itab += tab_step*2 )
                    {
                        j = itab[0];
                        assert( (unsigned)j < (unsigned)n2 );

                        CV_SWAP(dst[i+1], dsth[j], t);
                        if( j > i )
                        {
                            CV_SWAP(dst[i], dst[j], t);
                            CV_SWAP(dsth[i+1], dsth[j+1], t);
                        }
                    }
                }
                // else do nothing
            }
            else
            {
                for( i = 0; i < n; i++, itab += tab_step )
                {
                    j = itab[0];
                    assert( (unsigned)j < (unsigned)n );
                    if( j > i )
                        CV_SWAP(dst[i], dst[j], t);
                }
            }
        }

        if( inv )
        {
            // conjugate the vector - i.e. invert sign of the imaginary part
            int* idst = (int*)dst;
            for( i = 0; i <= n - 2; i += 2 )
            {
                int t0 = idst[i*2+1] ^ 0x80000000;
                int t1 = idst[i*2+3] ^ 0x80000000;
                idst[i*2+1] = t0; idst[i*2+3] = t1;
            }

            if( i < n )
                idst[2*i+1] ^= 0x80000000;
        }
    }

    n = 1;
    // 1. power-2 transforms
    if( (factors[0] & 1) == 0 )
    {
        // radix-4 transform
        for( ; n*4 <= factors[0]; )
        {
            nx = n;
            n *= 4;
            dw0 /= 4;

            for( i = 0; i < n0; i += n )
            {
                CvComplex32f* v0;
                CvComplex32f* v1;
                double r0, i0, r1, i1, r2, i2, r3, i3, r4, i4;

                v0 = dst + i;
                v1 = v0 + nx*2;

                r2 = v0[0].re; i2 = v0[0].im;
                r1 = v0[nx].re; i1 = v0[nx].im;
                
                r0 = r1 + r2; i0 = i1 + i2;
                r2 -= r1; i2 -= i1;

                i3 = v1[nx].re; r3 = v1[nx].im;
                i4 = v1[0].re; r4 = v1[0].im;

                r1 = i4 + i3; i1 = r4 + r3;
                r3 = r4 - r3; i3 = i3 - i4;

                v0[0].re = (float)(r0 + r1); v0[0].im = (float)(i0 + i1);
                v1[0].re = (float)(r0 - r1); v1[0].im = (float)(i0 - i1);
                v0[nx].re = (float)(r2 + r3); v0[nx].im = (float)(i2 + i3);
                v1[nx].re = (float)(r2 - r3); v1[nx].im = (float)(i2 - i3);

                for( j = 1, dw = dw0; j < nx; j++, dw += dw0 )
                {
                    v0 = dst + i + j;
                    v1 = v0 + nx*2;

                    r2 = v0[nx].re*wave[dw*2].re - v0[nx].im*wave[dw*2].im;
                    i2 = v0[nx].re*wave[dw*2].im + v0[nx].im*wave[dw*2].re;
                    r0 = v1[0].re*wave[dw].im + v1[0].im*wave[dw].re;
                    i0 = v1[0].re*wave[dw].re - v1[0].im*wave[dw].im;
                    r3 = v1[nx].re*wave[dw*3].im + v1[nx].im*wave[dw*3].re;
                    i3 = v1[nx].re*wave[dw*3].re - v1[nx].im*wave[dw*3].im;

                    r1 = i0 + i3; i1 = r0 + r3;
                    r3 = r0 - r3; i3 = i3 - i0;
                    r4 = v0[0].re; i4 = v0[0].im;

                    r0 = r4 + r2; i0 = i4 + i2;
                    r2 = r4 - r2; i2 = i4 - i2;

                    v0[0].re = (float)(r0 + r1); v0[0].im = (float)(i0 + i1);
                    v1[0].re = (float)(r0 - r1); v1[0].im = (float)(i0 - i1);
                    v0[nx].re = (float)(r2 + r3); v0[nx].im = (float)(i2 + i3);
                    v1[nx].re = (float)(r2 - r3); v1[nx].im = (float)(i2 - i3);
                }
            }
        }

        for( ; n < factors[0]; )
        {
            // do the remaining radix-2 transform
            nx = n;
            n *= 2;
            dw0 /= 2;

            for( i = 0; i < n0; i += n )
            {
                CvComplex32f* v = dst + i;
                double r0 = v[0].re + v[nx].re;
                double i0 = v[0].im + v[nx].im;
                double r1 = v[0].re - v[nx].re;
                double i1 = v[0].im - v[nx].im;
                v[0].re = (float)r0; v[0].im = (float)i0;
                v[nx].re = (float)r1; v[nx].im = (float)i1;

                for( j = 1, dw = dw0; j < nx; j++, dw += dw0 )
                {
                    v = dst + i + j;
                    r1 = v[nx].re*wave[dw].re - v[nx].im*wave[dw].im;
                    i1 = v[nx].im*wave[dw].re + v[nx].re*wave[dw].im;
                    r0 = v[0].re; i0 = v[0].im;

                    v[0].re = (float)(r0 + r1); v[0].im = (float)(i0 + i1);
                    v[nx].re = (float)(r0 - r1); v[nx].im = (float)(i0 - i1);
                }
            }
        }
    }

    // 2. all the other transforms
    for( f_idx = (factors[0]&1) ? 0 : 1; f_idx < nf; f_idx++ )
    {
        int factor = factors[f_idx];
        nx = n;
        n *= factor;
        dw0 /= factor;

        if( factor == 3 )
        {
            // radix-3
            for( i = 0; i < n0; i += n )
            {
                CvComplex32f* v = dst + i;

                double r1 = v[nx].re + v[nx*2].re;
                double i1 = v[nx].im + v[nx*2].im;
                double r0 = v[0].re;
                double i0 = v[0].im;
                double r2 = icv_sin_120*(v[nx].im - v[nx*2].im);
                double i2 = icv_sin_120*(v[nx*2].re - v[nx].re);
                v[0].re = (float)(r0 + r1); v[0].im = (float)(i0 + i1);
                r0 -= 0.5*r1; i0 -= 0.5*i1;
                v[nx].re = (float)(r0 + r2); v[nx].im = (float)(i0 + i2);
                v[nx*2].re = (float)(r0 - r2); v[nx*2].im = (float)(i0 - i2);

                for( j = 1, dw = dw0; j < nx; j++, dw += dw0 )
                {
                    v = dst + i + j;
                    r0 = v[nx].re*wave[dw].re - v[nx].im*wave[dw].im;
                    i0 = v[nx].re*wave[dw].im + v[nx].im*wave[dw].re;
                    i2 = v[nx*2].re*wave[dw*2].re - v[nx*2].im*wave[dw*2].im;
                    r2 = v[nx*2].re*wave[dw*2].im + v[nx*2].im*wave[dw*2].re;
                    r1 = r0 + i2; i1 = i0 + r2;
                    
                    r2 = icv_sin_120*(i0 - r2); i2 = icv_sin_120*(i2 - r0);
                    r0 = v[0].re; i0 = v[0].im;
                    v[0].re = (float)(r0 + r1); v[0].im = (float)(i0 + i1);
                    r0 -= 0.5*r1; i0 -= 0.5*i1;
                    v[nx].re = (float)(r0 + r2); v[nx].im = (float)(i0 + i2);
                    v[nx*2].re = (float)(r0 - r2); v[nx*2].im = (float)(i0 - i2);
                }
            }
        }
        else if( factor == 5 )
        {
            // radix-5
            for( i = 0; i < n0; i += n )
            {
                for( j = 0, dw = 0; j < nx; j++, dw += dw0 )
                {
                    CvComplex32f* v0 = dst + i + j;
                    CvComplex32f* v1 = v0 + nx*2;
                    CvComplex32f* v2 = v1 + nx*2;

                    double r0, i0, r1, i1, r2, i2, r3, i3, r4, i4, r5, i5;

                    r3 = v0[nx].re*wave[dw].re - v0[nx].im*wave[dw].im;
                    i3 = v0[nx].re*wave[dw].im + v0[nx].im*wave[dw].re;
                    r2 = v2[0].re*wave[dw*4].re - v2[0].im*wave[dw*4].im;
                    i2 = v2[0].re*wave[dw*4].im + v2[0].im*wave[dw*4].re;

                    r1 = r3 + r2; i1 = i3 + i2;
                    r3 -= r2; i3 -= i2;

                    r4 = v1[nx].re*wave[dw*3].re - v1[nx].im*wave[dw*3].im;
                    i4 = v1[nx].re*wave[dw*3].im + v1[nx].im*wave[dw*3].re;
                    r0 = v1[0].re*wave[dw*2].re - v1[0].im*wave[dw*2].im;
                    i0 = v1[0].re*wave[dw*2].im + v1[0].im*wave[dw*2].re;

                    r2 = r4 + r0; i2 = i4 + i0;
                    r4 -= r0; i4 -= i0;

                    r0 = v0[0].re; i0 = v0[0].im;
                    r5 = r1 + r2; i5 = i1 + i2;

                    v0[0].re = (float)(r0 + r5); v0[0].im = (float)(i0 + i5);

                    r0 -= 0.25*r5; i0 -= 0.25*i5;
                    r1 = icv_fft5_2*(r1 - r2); i1 = icv_fft5_2*(i1 - i2);
                    r2 = -icv_fft5_3*(i3 + i4); i2 = icv_fft5_3*(r3 + r4);

                    i3 *= -icv_fft5_5; r3 *= icv_fft5_5;
                    i4 *= -icv_fft5_4; r4 *= icv_fft5_4;

                    r5 = r2 + i3; i5 = i2 + r3;
                    r2 -= i4; i2 -= r4;
                    
                    r3 = r0 + r1; i3 = i0 + i1;
                    r0 -= r1; i0 -= i1;

                    v0[nx].re = (float)(r3 + r2); v0[nx].im = (float)(i3 + i2);
                    v2[0].re = (float)(r3 - r2); v2[0].im = (float)(i3 - i2);

                    v1[0].re = (float)(r0 + r5); v1[0].im = (float)(i0 + i5);
                    v1[nx].re = (float)(r0 - r5); v1[nx].im = (float)(i0 - i5);
                }
            }
        }
        else
        {
            // radix-"factor" - an odd number
            int p, q, factor2 = (factor - 1)/2;
            int d, dd, dw_f = tab_size/factor;
            CvComplex32f* a = buf;
            CvComplex32f* b = buf + factor2;

            for( i = 0; i < n0; i += n )
            {
                for( j = 0, dw = 0; j < nx; j++, dw += dw0 )
                {
                    CvComplex32f* v = dst + i + j;
                    CvComplex32f v_0 = v[0];
                    CvComplex64f vn_0(v_0);

                    if( j == 0 )
                    {
                        for( p = 1, k = nx; p <= factor2; p++, k += nx )
                        {
                            double r0 = v[k].re + v[n-k].re;
                            double i0 = v[k].im - v[n-k].im;
                            double r1 = v[k].re - v[n-k].re;
                            double i1 = v[k].im + v[n-k].im;

                            vn_0.re += r0; vn_0.im += i1;
                            a[p-1].re = (float)r0; a[p-1].im = (float)i0;
                            b[p-1].re = (float)r1; b[p-1].im = (float)i1;
                        }
                    }
                    else
                    {
                        const CvComplex32f* wave_ = wave + dw*factor;
                        d = dw;

                        for( p = 1, k = nx; p <= factor2; p++, k += nx, d += dw )
                        {
                            double r2 = v[k].re*wave[d].re - v[k].im*wave[d].im;
                            double i2 = v[k].re*wave[d].im + v[k].im*wave[d].re;

                            double r1 = v[n-k].re*wave_[-d].re - v[n-k].im*wave_[-d].im;
                            double i1 = v[n-k].re*wave_[-d].im + v[n-k].im*wave_[-d].re;
                    
                            double r0 = r2 + r1;
                            double i0 = i2 - i1;
                            r1 = r2 - r1;
                            i1 = i2 + i1;

                            vn_0.re += r0; vn_0.im += i1;
                            a[p-1].re = (float)r0; a[p-1].im = (float)i0;
                            b[p-1].re = (float)r1; b[p-1].im = (float)i1;
                        }
                    }

                    v[0].re = (float)vn_0.re;
                    v[0].im = (float)vn_0.im;

                    for( p = 1, k = nx; p <= factor2; p++, k += nx )
                    {
                        CvComplex64f s0(v_0), s1 = s0;
                        d = dd = dw_f*p;

                        for( q = 0; q < factor2; q++ )
                        {
                            double r0 = wave[d].re * a[q].re;
                            double i0 = wave[d].im * a[q].im;
                            double r1 = wave[d].re * b[q].im;
                            double i1 = wave[d].im * b[q].re;
            
                            s1.re += r0 + i0; s0.re += r0 - i0;
                            s1.im += r1 - i1; s0.im += r1 + i1;

                            d += dd;
                            d -= -(d >= tab_size) & tab_size;
                        }

                        v[k].re = (float)s0.re;
                        v[k].im = (float)s0.im;
                        v[n-k].re = (float)s1.re;
                        v[n-k].im = (float)s1.im;
                    }
                }
            }
        }
    }

    if( fabs(scale - 1.) > DBL_EPSILON )
    {
        double re_scale = scale, im_scale = scale;
        if( inv )
            im_scale = -im_scale;

        for( i = 0; i < n0; i++ )
        {
            double t0 = dst[i].re*re_scale;
            double t1 = dst[i].im*im_scale;
            dst[i].re = (float)t0;
            dst[i].im = (float)t1;
        }
    }
    else if( inv )
    {
        for( i = 0; i <= n0 - 2; i += 2 )
        {
            double t0 = -dst[i].im;
            double t1 = -dst[i+1].im;
            dst[i].im = (float)t0;
            dst[i+1].im = (float)t1;
        }

        if( i < n0 )
            dst[n0-1].im = -dst[n0-1].im;
    }

    return CV_OK;
}
// 
// 
// /* FFT of real vector
//    output vector format:
//      re(0), re(1), im(1), ... , re(n/2-1), im((n+1)/2-1) [, re((n+1)/2)] OR ...
//      re(0), 0, re(1), im(1), ..., re(n/2-1), im((n+1)/2-1) [, re((n+1)/2), 0] */
#define ICV_REAL_DFT( flavor, datatype )                                \
static CvStatus CV_STDCALL                                              \
icvRealDFT_##flavor( const datatype* src, datatype* dst,                \
                     int n, int nf, int* factors, const int* itab,      \
                     const CvComplex##flavor* wave, int tab_size,       \
                     const void* spec, CvComplex##flavor* buf,          \
                     int flags, double scale )                          \
{                                                                       \
    int complex_output = (flags & ICV_DFT_COMPLEX_INPUT_OR_OUTPUT) != 0;\
    int j, n2 = n >> 1;                                                 \
    dst += complex_output;                                              \
                                                                        \
    if( spec )                                                          \
    {                                                                   \
        icvDFTFwd_RToPack_##flavor##_p( src, dst, spec, buf );          \
        goto finalize;                                                  \
    }                                                                   \
                                                                        \
    assert( tab_size == n );                                            \
                                                                        \
    if( n == 1 )                                                        \
    {                                                                   \
        dst[0] = (datatype)(src[0]*scale);                              \
    }                                                                   \
    else if( n == 2 )                                                   \
    {                                                                   \
        double t = (src[0] + src[1])*scale;                             \
        dst[1] = (datatype)((src[0] - src[1])*scale);                   \
        dst[0] = (datatype)t;                                           \
    }                                                                   \
    else if( n & 1 )                                                    \
    {                                                                   \
        dst -= complex_output;                                          \
        CvComplex##flavor* _dst = (CvComplex##flavor*)dst;              \
        _dst[0].re = (datatype)(src[0]*scale);                          \
        _dst[0].im = 0;                                                 \
        for( j = 1; j < n; j += 2 )                                     \
        {                                                               \
            double t0 = src[itab[j]]*scale;                             \
            double t1 = src[itab[j+1]]*scale;                           \
            _dst[j].re = (datatype)t0;                                  \
            _dst[j].im = 0;                                             \
            _dst[j+1].re = (datatype)t1;                                \
            _dst[j+1].im = 0;                                           \
        }                                                               \
        icvDFT_##flavor##c( _dst, _dst, n, nf, factors, itab, wave,     \
                            tab_size, 0, buf, ICV_DFT_NO_PERMUTE, 1. ); \
        if( !complex_output )                                           \
            dst[1] = dst[0];                                            \
        return CV_OK;                                                   \
    }                                                                   \
    else                                                                \
    {                                                                   \
        double t0, t;                                                   \
        double h1_re, h1_im, h2_re, h2_im;                              \
        double scale2 = scale*0.5;                                      \
        factors[0] >>= 1;                                               \
                                                                        \
        icvDFT_##flavor##c( (CvComplex##flavor*)src,                    \
                            (CvComplex##flavor*)dst, n2,                \
                            nf - (factors[0] == 1),                     \
                            factors + (factors[0] == 1),                \
                            itab, wave, tab_size, 0, buf, 0, 1. );      \
        factors[0] <<= 1;                                               \
                                                                        \
        t = dst[0] - dst[1];                                            \
        dst[0] = (datatype)((dst[0] + dst[1])*scale);                   \
        dst[1] = (datatype)(t*scale);                                   \
                                                                        \
        t0 = dst[n2];                                                   \
        t = dst[n-1];                                                   \
        dst[n-1] = dst[1];                                              \
                                                                        \
        for( j = 2, wave++; j < n2; j += 2, wave++ )                    \
        {                                                               \
            /* calc odd */                                              \
            h2_re = scale2*(dst[j+1] + t);                              \
            h2_im = scale2*(dst[n-j] - dst[j]);                         \
                                                                        \
            /* calc even */                                             \
            h1_re = scale2*(dst[j] + dst[n-j]);                         \
            h1_im = scale2*(dst[j+1] - t);                              \
                                                                        \
            /* rotate */                                                \
            t = h2_re*wave->re - h2_im*wave->im;                        \
            h2_im = h2_re*wave->im + h2_im*wave->re;                    \
            h2_re = t;                                                  \
            t = dst[n-j-1];                                             \
                                                                        \
            dst[j-1] = (datatype)(h1_re + h2_re);                       \
            dst[n-j-1] = (datatype)(h1_re - h2_re);                     \
            dst[j] = (datatype)(h1_im + h2_im);                         \
            dst[n-j] = (datatype)(h2_im - h1_im);                       \
        }                                                               \
                                                                        \
        if( j <= n2 )                                                   \
        {                                                               \
            dst[n2-1] = (datatype)(t0*scale);                           \
            dst[n2] = (datatype)(-t*scale);                             \
        }                                                               \
    }                                                                   \
                                                                        \
finalize:                                                               \
    if( complex_output )                                                \
    {                                                                   \
        dst[-1] = dst[0];                                               \
        dst[0] = 0;                                                     \
        if( (n & 1) == 0 )                                              \
            dst[n] = 0;                                                 \
    }                                                                   \
                                                                        \
    return CV_OK;                                                       \
}

// 
// /* Inverse FFT of complex conjugate-symmetric vector
//    input vector format:
//       re[0], re[1], im[1], ... , re[n/2-1], im[n/2-1], re[n/2] OR
//       re(0), 0, re(1), im(1), ..., re(n/2-1), im((n+1)/2-1) [, re((n+1)/2), 0] */
#define ICV_CCS_IDFT( flavor, datatype )                                \
static CvStatus CV_STDCALL                                              \
icvCCSIDFT_##flavor( const datatype* src, datatype* dst,                \
                     int n, int nf, int* factors, const int* itab,      \
                     const CvComplex##flavor* wave, int tab_size,       \
                     const void* spec, CvComplex##flavor* buf,          \
                     int flags, double scale )                          \
{                                                                       \
    int complex_input = (flags & ICV_DFT_COMPLEX_INPUT_OR_OUTPUT) != 0; \
    int j, k, n2 = (n+1) >> 1;                                          \
    double save_s1 = 0.;                                                \
    double t0, t1, t2, t3, t;                                           \
                                                                        \
    assert( tab_size == n );                                            \
                                                                        \
    if( complex_input )                                                 \
    {                                                                   \
        assert( src != dst );                                           \
        save_s1 = src[1];                                               \
        ((datatype*)src)[1] = src[0];                                   \
        src++;                                                          \
    }                                                                   \
                                                                        \
    if( spec )                                                          \
    {                                                                   \
        icvDFTInv_PackToR_##flavor##_p( src, dst, spec, buf );          \
        goto finalize;                                                  \
    }                                                                   \
                                                                        \
    if( n == 1 )                                                        \
    {                                                                   \
        dst[0] = (datatype)(src[0]*scale);                              \
    }                                                                   \
    else if( n == 2 )                                                   \
    {                                                                   \
        t = (src[0] + src[1])*scale;                                    \
        dst[1] = (datatype)((src[0] - src[1])*scale);                   \
        dst[0] = (datatype)t;                                           \
    }                                                                   \
    else if( n & 1 )                                                    \
    {                                                                   \
        CvComplex##flavor* _src = (CvComplex##flavor*)(src-1);          \
        CvComplex##flavor* _dst = (CvComplex##flavor*)dst;              \
                                                                        \
        _dst[0].re = src[0];                                            \
        _dst[0].im = 0;                                                 \
        for( j = 1; j < n2; j++ )                                       \
        {                                                               \
            int k0 = itab[j], k1 = itab[n-j];                           \
            t0 = _src[j].re; t1 = _src[j].im;                           \
            _dst[k0].re = (datatype)t0; _dst[k0].im = (datatype)-t1;    \
            _dst[k1].re = (datatype)t0; _dst[k1].im = (datatype)t1;     \
        }                                                               \
                                                                        \
        icvDFT_##flavor##c( _dst, _dst, n, nf, factors, itab, wave,     \
                            tab_size, 0, buf, ICV_DFT_NO_PERMUTE, 1. ); \
        dst[0] = (datatype)(dst[0]*scale);                              \
        for( j = 1; j < n; j += 2 )                                     \
        {                                                               \
            t0 = dst[j*2]*scale;                                        \
            t1 = dst[j*2+2]*scale;                                      \
            dst[j] = (datatype)t0;                                      \
            dst[j+1] = (datatype)t1;                                    \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        int inplace = src == dst;                                       \
        const CvComplex##flavor* w = wave;                              \
                                                                        \
        t = src[1];                                                     \
        t0 = (src[0] + src[n-1]);                                       \
        t1 = (src[n-1] - src[0]);                                       \
        dst[0] = (datatype)t0;                                          \
        dst[1] = (datatype)t1;                                          \
                                                                        \
        for( j = 2, w++; j < n2; j += 2, w++ )                          \
        {                                                               \
            double h1_re, h1_im, h2_re, h2_im;                          \
                                                                        \
            h1_re = (t + src[n-j-1]);                                   \
            h1_im = (src[j] - src[n-j]);                                \
                                                                        \
            h2_re = (t - src[n-j-1]);                                   \
            h2_im = (src[j] + src[n-j]);                                \
                                                                        \
            t = h2_re*w->re + h2_im*w->im;                              \
            h2_im = h2_im*w->re - h2_re*w->im;                          \
            h2_re = t;                                                  \
                                                                        \
            t = src[j+1];                                               \
            t0 = h1_re - h2_im;                                         \
            t1 = -h1_im - h2_re;                                        \
            t2 = h1_re + h2_im;                                         \
            t3 = h1_im - h2_re;                                         \
                                                                        \
            if( inplace )                                               \
            {                                                           \
                dst[j] = (datatype)t0;                                  \
                dst[j+1] = (datatype)t1;                                \
                dst[n-j] = (datatype)t2;                                \
                dst[n-j+1]= (datatype)t3;                               \
            }                                                           \
            else                                                        \
            {                                                           \
                int j2 = j >> 1;                                        \
                k = itab[j2];                                           \
                dst[k] = (datatype)t0;                                  \
                dst[k+1] = (datatype)t1;                                \
                k = itab[n2-j2];                                        \
                dst[k] = (datatype)t2;                                  \
                dst[k+1]= (datatype)t3;                                 \
            }                                                           \
        }                                                               \
                                                                        \
        if( j <= n2 )                                                   \
        {                                                               \
            t0 = t*2;                                                   \
            t1 = src[n2]*2;                                             \
                                                                        \
            if( inplace )                                               \
            {                                                           \
                dst[n2] = (datatype)t0;                                 \
                dst[n2+1] = (datatype)t1;                               \
            }                                                           \
            else                                                        \
            {                                                           \
                k = itab[n2];                                           \
                dst[k*2] = (datatype)t0;                                \
                dst[k*2+1] = (datatype)t1;                              \
            }                                                           \
        }                                                               \
                                                                        \
        factors[0] >>= 1;                                               \
        icvDFT_##flavor##c( (CvComplex##flavor*)dst,                    \
                            (CvComplex##flavor*)dst, n2,                \
                            nf - (factors[0] == 1),                     \
                            factors + (factors[0] == 1), itab,          \
                            wave, tab_size, 0, buf,                     \
                            inplace ? 0 : ICV_DFT_NO_PERMUTE, 1. );     \
        factors[0] <<= 1;                                               \
                                                                        \
        for( j = 0; j < n; j += 2 )                                     \
        {                                                               \
            t0 = dst[j]*scale;                                          \
            t1 = dst[j+1]*(-scale);                                     \
            dst[j] = (datatype)t0;                                      \
            dst[j+1] = (datatype)t1;                                    \
        }                                                               \
    }                                                                   \
                                                                        \
finalize:                                                               \
    if( complex_input )                                                 \
        ((datatype*)src)[0] = (datatype)save_s1;                        \
                                                                        \
    return CV_OK;                                                       \
}


ICV_REAL_DFT( 64f, double )
ICV_CCS_IDFT( 64f, double )
ICV_REAL_DFT( 32f, float )
ICV_CCS_IDFT( 32f, float )
// 
// 
static void
icvCopyColumn( const uchar* _src, int src_step,
               uchar* _dst, int dst_step,
               int len, int elem_size )
{
    int i, t0, t1;
    const int* src = (const int*)_src;
    int* dst = (int*)_dst;
    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    if( elem_size == sizeof(int) )
    {
        for( i = 0; i < len; i++, src += src_step, dst += dst_step )
            dst[0] = src[0];
    }
    else if( elem_size == sizeof(int)*2 )
    {
        for( i = 0; i < len; i++, src += src_step, dst += dst_step )
        {
            t0 = src[0]; t1 = src[1];
            dst[0] = t0; dst[1] = t1;
        }
    }
    else if( elem_size == sizeof(int)*4 )
    {
        for( i = 0; i < len; i++, src += src_step, dst += dst_step )
        {
            t0 = src[0]; t1 = src[1];
            dst[0] = t0; dst[1] = t1;
            t0 = src[2]; t1 = src[3];
            dst[2] = t0; dst[3] = t1;
        }
    }
}

// 
static void
icvCopyFrom2Columns( const uchar* _src, int src_step,
                     uchar* _dst0, uchar* _dst1,
                     int len, int elem_size )
{
    int i, t0, t1;
    const int* src = (const int*)_src;
    int* dst0 = (int*)_dst0;
    int* dst1 = (int*)_dst1;
    src_step /= sizeof(src[0]);

    if( elem_size == sizeof(int) )
    {
        for( i = 0; i < len; i++, src += src_step )
        {
            t0 = src[0]; t1 = src[1];
            dst0[i] = t0; dst1[i] = t1;
        }
    }
    else if( elem_size == sizeof(int)*2 )
    {
        for( i = 0; i < len*2; i += 2, src += src_step )
        {
            t0 = src[0]; t1 = src[1];
            dst0[i] = t0; dst0[i+1] = t1;
            t0 = src[2]; t1 = src[3];
            dst1[i] = t0; dst1[i+1] = t1;
        }
    }
    else if( elem_size == sizeof(int)*4 )
    {
        for( i = 0; i < len*4; i += 4, src += src_step )
        {
            t0 = src[0]; t1 = src[1];
            dst0[i] = t0; dst0[i+1] = t1;
            t0 = src[2]; t1 = src[3];
            dst0[i+2] = t0; dst0[i+3] = t1;
            t0 = src[4]; t1 = src[5];
            dst1[i] = t0; dst1[i+1] = t1;
            t0 = src[6]; t1 = src[7];
            dst1[i+2] = t0; dst1[i+3] = t1;
        }
    }
}


static void
icvCopyTo2Columns( const uchar* _src0, const uchar* _src1,
                   uchar* _dst, int dst_step,
                   int len, int elem_size )
{
    int i, t0, t1;
    const int* src0 = (const int*)_src0;
    const int* src1 = (const int*)_src1;
    int* dst = (int*)_dst;
    dst_step /= sizeof(dst[0]);

    if( elem_size == sizeof(int) )
    {
        for( i = 0; i < len; i++, dst += dst_step )
        {
            t0 = src0[i]; t1 = src1[i];
            dst[0] = t0; dst[1] = t1;
        }
    }
    else if( elem_size == sizeof(int)*2 )
    {
        for( i = 0; i < len*2; i += 2, dst += dst_step )
        {
            t0 = src0[i]; t1 = src0[i+1];
            dst[0] = t0; dst[1] = t1;
            t0 = src1[i]; t1 = src1[i+1];
            dst[2] = t0; dst[3] = t1;
        }
    }
    else if( elem_size == sizeof(int)*4 )
    {
        for( i = 0; i < len*4; i += 4, dst += dst_step )
        {
            t0 = src0[i]; t1 = src0[i+1];
            dst[0] = t0; dst[1] = t1;
            t0 = src0[i+2]; t1 = src0[i+3];
            dst[2] = t0; dst[3] = t1;
            t0 = src1[i]; t1 = src1[i+1];
            dst[4] = t0; dst[5] = t1;
            t0 = src1[i+2]; t1 = src1[i+3];
            dst[6] = t0; dst[7] = t1;
        }
    }
}

// 
static void
icvExpandCCS( uchar* _ptr, int len, int elem_size )
{
    int i;
    _ptr -= elem_size;
    memcpy( _ptr, _ptr + elem_size, elem_size );
    memset( _ptr + elem_size, 0, elem_size );
    if( (len & 1) == 0 )
        memset( _ptr + (len+1)*elem_size, 0, elem_size );

    if( elem_size == sizeof(float) )
    {
        CvComplex32f* ptr = (CvComplex32f*)_ptr;

        for( i = 1; i < (len+1)/2; i++ )
        {
            CvComplex32f t;
            t.re = ptr[i].re;
            t.im = -ptr[i].im;
            ptr[len-i] = t;
        }
    }
    else
    {
        CvComplex64f* ptr = (CvComplex64f*)_ptr;

        for( i = 1; i < (len+1)/2; i++ )
        {
            CvComplex64f t;
            t.re = ptr[i].re;
            t.im = -ptr[i].im;
            ptr[len-i] = t;
        }
    }
}
// 
// 
typedef CvStatus (CV_STDCALL *CvDFTFunc)(
     const void* src, void* dst, int n, int nf, int* factors,
     const int* itab, const void* wave, int tab_size,
     const void* spec, void* buf, int inv, double scale );

CV_IMPL void
cvDFT( const CvArr* srcarr, CvArr* dstarr, int flags, int nonzero_rows )
{
    static CvDFTFunc dft_tbl[6];
    static int inittab = 0;
    
    void* buffer = 0;
    int local_alloc = 1;
    int depth = -1;
    void *spec_c = 0, *spec_r = 0, *spec = 0;
    
    CV_FUNCNAME( "cvDFT" );

    __BEGIN__;

    int prev_len = 0, buf_size = 0, stage = 0;
    int nf = 0, inv = (flags & CV_DXT_INVERSE) != 0;
    int real_transform = 0;
    CvMat *src = (CvMat*)srcarr, *dst = (CvMat*)dstarr;
    CvMat srcstub, dststub, *src0;
    int complex_elem_size, elem_size;
    int factors[34], inplace_transform = 0;
    int ipp_norm_flag = 0;

    if( !inittab )
    {
        dft_tbl[0] = (CvDFTFunc)icvDFT_32fc;
        dft_tbl[1] = (CvDFTFunc)icvRealDFT_32f;
        dft_tbl[2] = (CvDFTFunc)icvCCSIDFT_32f;
        dft_tbl[3] = (CvDFTFunc)icvDFT_64fc;
        dft_tbl[4] = (CvDFTFunc)icvRealDFT_64f;
        dft_tbl[5] = (CvDFTFunc)icvCCSIDFT_64f;
        inittab = 1;
    }

    if( !CV_IS_MAT( src ))
    {
        int coi = 0;
        CV_CALL( src = cvGetMat( src, &srcstub, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    if( !CV_IS_MAT( dst ))
    {
        int coi = 0;
        CV_CALL( dst = cvGetMat( dst, &dststub, &coi ));

        if( coi != 0 )
            CV_ERROR( CV_BadCOI, "" );
    }

    src0 = src;
    elem_size = CV_ELEM_SIZE1(src->type);
    complex_elem_size = elem_size*2;

    // check types and sizes
    if( !CV_ARE_DEPTHS_EQ(src, dst) )
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    depth = CV_MAT_DEPTH(src->type);
    if( depth < CV_32F )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Only 32fC1, 32fC2, 64fC1 and 64fC2 formats are supported" );

    if( CV_ARE_CNS_EQ(src, dst) )
    {
        if( CV_MAT_CN(src->type) > 2 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Only 32fC1, 32fC2, 64fC1 and 64fC2 formats are supported" );

        if( !CV_ARE_SIZES_EQ(src, dst) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        real_transform = CV_MAT_CN(src->type) == 1;
        if( !real_transform )
            elem_size = complex_elem_size;
    }
    else if( !inv && CV_MAT_CN(src->type) == 1 && CV_MAT_CN(dst->type) == 2 )
    {
        if( (src->cols != 1 || dst->cols != 1 ||
            src->rows/2+1 != dst->rows && src->rows != dst->rows) &&
            (src->cols/2+1 != dst->cols || src->rows != dst->rows) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        real_transform = 1;
    }
    else if( inv && CV_MAT_CN(src->type) == 2 && CV_MAT_CN(dst->type) == 1 )
    {
        if( (src->cols != 1 || dst->cols != 1 ||
            dst->rows/2+1 != src->rows && src->rows != dst->rows) &&
            (dst->cols/2+1 != src->cols || src->rows != dst->rows) )
            CV_ERROR( CV_StsUnmatchedSizes, "" );
        real_transform = 1;
    }
    else
        CV_ERROR( CV_StsUnmatchedFormats,
        "Incorrect or unsupported combination of input & output formats" );

    if( src->cols == 1 && nonzero_rows > 0 )
        CV_ERROR( CV_StsNotImplemented,
        "This mode (using nonzero_rows with a single-column matrix) breaks the function logic, so it is prohibited.\n"
        "For fast convolution/correlation use 2-column matrix or single-row matrix instead" );

    // determine, which transform to do first - row-wise
    // (stage 0) or column-wise (stage 1) transform
    if( !(flags & CV_DXT_ROWS) && src->rows > 1 &&
        (src->cols == 1 && !CV_IS_MAT_CONT(src->type & dst->type) ||
        src->cols > 1 && inv && real_transform) )
        stage = 1;

    ipp_norm_flag = !(flags & CV_DXT_SCALE) ? 8 : (flags & CV_DXT_INVERSE) ? 2 : 1;

    for(;;)
    {
        double scale = 1;
        uchar* wave = 0;
        int* itab = 0;
        uchar* ptr;
        int i, len, count, sz = 0;
        int use_buf = 0, odd_real = 0;
        CvDFTFunc dft_func;

        if( stage == 0 ) // row-wise transform
        {
            len = !inv ? src->cols : dst->cols;
            count = src->rows;
            if( len == 1 && !(flags & CV_DXT_ROWS) )
            {
                len = !inv ? src->rows : dst->rows;
                count = 1;
            }
            odd_real = real_transform && (len & 1);
        }
        else
        {
            len = dst->rows;
            count = !inv ? src0->cols : dst->cols;
            sz = 2*len*complex_elem_size;
        }

        spec = 0;
        if( len*count >= 64 && icvDFTInitAlloc_R_32f_p != 0 ) // use IPP DFT if available
        {
            int ipp_sz = 0;
            
            if( real_transform && stage == 0 )
            {
                if( depth == CV_32F )
                {
                    if( spec_r )
                        IPPI_CALL( icvDFTFree_R_32f_p( spec_r ));
                    IPPI_CALL( icvDFTInitAlloc_R_32f_p(
                        &spec_r, len, ipp_norm_flag, cvAlgHintNone ));
                    IPPI_CALL( icvDFTGetBufSize_R_32f_p( spec_r, &ipp_sz ));
                }
                else
                {
                    if( spec_r )
                        IPPI_CALL( icvDFTFree_R_64f_p( spec_r ));
                    IPPI_CALL( icvDFTInitAlloc_R_64f_p(
                        &spec_r, len, ipp_norm_flag, cvAlgHintNone ));
                    IPPI_CALL( icvDFTGetBufSize_R_64f_p( spec_r, &ipp_sz ));
                }
                spec = spec_r;
            }
            else
            {
                if( depth == CV_32F )
                {
                    if( spec_c )
                        IPPI_CALL( icvDFTFree_C_32fc_p( spec_c ));
                    IPPI_CALL( icvDFTInitAlloc_C_32fc_p(
                        &spec_c, len, ipp_norm_flag, cvAlgHintNone ));
                    IPPI_CALL( icvDFTGetBufSize_C_32fc_p( spec_c, &ipp_sz ));
                }
                else
                {
                    if( spec_c )
                        IPPI_CALL( icvDFTFree_C_64fc_p( spec_c ));
                    IPPI_CALL( icvDFTInitAlloc_C_64fc_p(
                        &spec_c, len, ipp_norm_flag, cvAlgHintNone ));
                    IPPI_CALL( icvDFTGetBufSize_C_64fc_p( spec_c, &ipp_sz ));
                }
                spec = spec_c;
            }

            sz += ipp_sz;
        }
        else
        {
            if( len != prev_len )
                nf = icvDFTFactorize( len, factors );

            inplace_transform = factors[0] == factors[nf-1];
            sz += len*(complex_elem_size + sizeof(int));
            i = nf > 1 && (factors[0] & 1) == 0;
            if( (factors[i] & 1) != 0 && factors[i] > 5 )
                sz += (factors[i]+1)*complex_elem_size;

            if( stage == 0 && (src->data.ptr == dst->data.ptr && !inplace_transform || odd_real) ||
                stage == 1 && !inplace_transform )
            {
                use_buf = 1;
                sz += len*complex_elem_size;
            }
        }

        if( sz > buf_size )
        {
            prev_len = 0; // because we release the buffer, 
                          // force recalculation of
                          // twiddle factors and permutation table
            if( !local_alloc && buffer )
                cvFree( &buffer );
            if( sz <= CV_MAX_LOCAL_DFT_SIZE )
            {
                buf_size = sz = CV_MAX_LOCAL_DFT_SIZE;
                buffer = cvStackAlloc(sz + 32);
                local_alloc = 1;
            }
            else
            {
                CV_CALL( buffer = cvAlloc(sz + 32) );
                buf_size = sz;
                local_alloc = 0;
            }
        }

        ptr = (uchar*)buffer;
        if( !spec )
        {
            wave = ptr;
            ptr += len*complex_elem_size;
            itab = (int*)ptr;
            ptr = (uchar*)cvAlignPtr( ptr + len*sizeof(int), 16 );

            if( len != prev_len || (!inplace_transform && inv && real_transform))
                icvDFTInit( len, nf, factors, itab, complex_elem_size,
                            wave, stage == 0 && inv && real_transform );
            // otherwise reuse the tables calculated on the previous stage
        }

        if( stage == 0 )
        {
            uchar* tmp_buf = 0;
            int dptr_offset = 0;
            int dst_full_len = len*elem_size;
            int _flags = inv + (CV_MAT_CN(src->type) != CV_MAT_CN(dst->type) ?
                         ICV_DFT_COMPLEX_INPUT_OR_OUTPUT : 0);
            if( use_buf )
            {
                tmp_buf = ptr;
                ptr += len*complex_elem_size;
                if( odd_real && !inv && len > 1 &&
                    !(_flags & ICV_DFT_COMPLEX_INPUT_OR_OUTPUT))
                    dptr_offset = elem_size;
            }

            if( !inv && (_flags & ICV_DFT_COMPLEX_INPUT_OR_OUTPUT) )
                dst_full_len += (len & 1) ? elem_size : complex_elem_size;

            dft_func = dft_tbl[(!real_transform ? 0 : !inv ? 1 : 2) + (depth == CV_64F)*3];

            if( count > 1 && !(flags & CV_DXT_ROWS) && (!inv || !real_transform) )
                stage = 1;
            else if( flags & CV_DXT_SCALE )
                scale = 1./(len * (flags & CV_DXT_ROWS ? 1 : count));

            if( nonzero_rows <= 0 || nonzero_rows > count )
                nonzero_rows = count;

            for( i = 0; i < nonzero_rows; i++ )
            {
                uchar* sptr = src->data.ptr + i*src->step;
                uchar* dptr0 = dst->data.ptr + i*dst->step;
                uchar* dptr = dptr0;

                if( tmp_buf )
                    dptr = tmp_buf;
                
                dft_func( sptr, dptr, len, nf, factors, itab, wave, len, spec, ptr, _flags, scale );
                if( dptr != dptr0 )
                    memcpy( dptr0, dptr + dptr_offset, dst_full_len );
            }

            for( ; i < count; i++ )
            {
                uchar* dptr0 = dst->data.ptr + i*dst->step;
                memset( dptr0, 0, dst_full_len );
            }

            if( stage != 1 )
                break;
            src = dst;
        }
        else
        {
            int a = 0, b = count;
            uchar *buf0, *buf1, *dbuf0, *dbuf1;
            uchar* sptr0 = src->data.ptr;
            uchar* dptr0 = dst->data.ptr;
            buf0 = ptr;
            ptr += len*complex_elem_size;
            buf1 = ptr;
            ptr += len*complex_elem_size;
            dbuf0 = buf0, dbuf1 = buf1;
            
            if( use_buf )
            {
                dbuf1 = ptr;
                dbuf0 = buf1;
                ptr += len*complex_elem_size;
            }

            dft_func = dft_tbl[(depth == CV_64F)*3];

            if( real_transform && inv && src->cols > 1 )
                stage = 0;
            else if( flags & CV_DXT_SCALE )
                scale = 1./(len * count);

            if( real_transform )
            {
                int even;
                a = 1;
                even = (count & 1) == 0;
                b = (count+1)/2;
                if( !inv )
                {
                    memset( buf0, 0, len*complex_elem_size );
                    icvCopyColumn( sptr0, src->step, buf0, complex_elem_size, len, elem_size );
                    sptr0 += CV_MAT_CN(dst->type)*elem_size;
                    if( even )
                    {
                        memset( buf1, 0, len*complex_elem_size );
                        icvCopyColumn( sptr0 + (count-2)*elem_size, src->step,
                                       buf1, complex_elem_size, len, elem_size );
                    }
                }
                else if( CV_MAT_CN(src->type) == 1 )
                {
                    icvCopyColumn( sptr0, src->step, buf0 + elem_size, elem_size, len, elem_size );
                    icvExpandCCS( buf0 + elem_size, len, elem_size );
                    if( even )
                    {
                        icvCopyColumn( sptr0 + (count-1)*elem_size, src->step,
                                       buf1 + elem_size, elem_size, len, elem_size );
                        icvExpandCCS( buf1 + elem_size, len, elem_size );
                    }
                    sptr0 += elem_size;
                }
                else
                {
                    icvCopyColumn( sptr0, src->step, buf0, complex_elem_size, len, complex_elem_size );
                    //memcpy( buf0 + elem_size, buf0, elem_size );
                    //icvExpandCCS( buf0 + elem_size, len, elem_size );
                    if( even )
                    {
                        icvCopyColumn( sptr0 + b*complex_elem_size, src->step,
                                       buf1, complex_elem_size, len, complex_elem_size );
                        //memcpy( buf0 + elem_size, buf0, elem_size );
                        //icvExpandCCS( buf0 + elem_size, len, elem_size );
                    }
                    sptr0 += complex_elem_size;
                }
                
                if( even )
                    IPPI_CALL( dft_func( buf1, dbuf1, len, nf, factors, itab,
                                         wave, len, spec, ptr, inv, scale ));
                IPPI_CALL( dft_func( buf0, dbuf0, len, nf, factors, itab,
                                     wave, len, spec, ptr, inv, scale ));

                if( CV_MAT_CN(dst->type) == 1 )
                {
                    if( !inv )
                    {
                        // copy the half of output vector to the first/last column.
                        // before doing that, defgragment the vector
                        memcpy( dbuf0 + elem_size, dbuf0, elem_size );
                        icvCopyColumn( dbuf0 + elem_size, elem_size, dptr0,
                                       dst->step, len, elem_size );
                        if( even )
                        {
                            memcpy( dbuf1 + elem_size, dbuf1, elem_size );
                            icvCopyColumn( dbuf1 + elem_size, elem_size,
                                           dptr0 + (count-1)*elem_size,
                                           dst->step, len, elem_size );
                        }
                        dptr0 += elem_size;
                    }
                    else
                    {
                        // copy the real part of the complex vector to the first/last column
                        icvCopyColumn( dbuf0, complex_elem_size, dptr0, dst->step, len, elem_size );
                        if( even )
                            icvCopyColumn( dbuf1, complex_elem_size, dptr0 + (count-1)*elem_size,
                                           dst->step, len, elem_size );
                        dptr0 += elem_size;
                    }
                }
                else
                {
                    assert( !inv );
                    icvCopyColumn( dbuf0, complex_elem_size, dptr0,
                                   dst->step, len, complex_elem_size );
                    if( even )
                        icvCopyColumn( dbuf1, complex_elem_size,
                                       dptr0 + b*complex_elem_size,
                                       dst->step, len, complex_elem_size );
                    dptr0 += complex_elem_size;
                }
            }

            for( i = a; i < b; i += 2 )
            {
                if( i+1 < b )
                {
                    icvCopyFrom2Columns( sptr0, src->step, buf0, buf1, len, complex_elem_size );
                    IPPI_CALL( dft_func( buf1, dbuf1, len, nf, factors, itab,
                                         wave, len, spec, ptr, inv, scale ));
                }
                else
                    icvCopyColumn( sptr0, src->step, buf0, complex_elem_size, len, complex_elem_size );

                IPPI_CALL( dft_func( buf0, dbuf0, len, nf, factors, itab,
                                     wave, len, spec, ptr, inv, scale ));

                if( i+1 < b )
                    icvCopyTo2Columns( dbuf0, dbuf1, dptr0, dst->step, len, complex_elem_size );
                else
                    icvCopyColumn( dbuf0, complex_elem_size, dptr0, dst->step, len, complex_elem_size );
                sptr0 += 2*complex_elem_size;
                dptr0 += 2*complex_elem_size;
            }

            if( stage != 0 )
                break;
            src = dst;
        }
    }

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );

    if( spec_c )
    {
        if( depth == CV_32F )
            icvDFTFree_C_32fc_p( spec_c );
        else
            icvDFTFree_C_64fc_p( spec_c );
    }

    if( spec_r )
    {
        if( depth == CV_32F )
            icvDFTFree_R_32f_p( spec_r );
        else
            icvDFTFree_R_64f_p( spec_r );
    }
}
// 
// 
CV_IMPL void
cvMulSpectrums( const CvArr* srcAarr, const CvArr* srcBarr,
                CvArr* dstarr, int flags )
{
    CV_FUNCNAME( "cvMulSpectrums" );

    __BEGIN__;

    CvMat stubA, *srcA = (CvMat*)srcAarr;
    CvMat stubB, *srcB = (CvMat*)srcBarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    int stepA, stepB, stepC;
    int type, cn, is_1d;
    int j, j0, j1, k, rows, cols, ncols;

    if( !CV_IS_MAT(srcA))
        CV_CALL( srcA = cvGetMat( srcA, &stubA, 0 ));

    if( !CV_IS_MAT(srcB))
        CV_CALL( srcB = cvGetMat( srcB, &stubB, 0 ));

    if( !CV_IS_MAT(dst))
        CV_CALL( dst = cvGetMat( dst, &dststub, 0 ));

    if( !CV_ARE_TYPES_EQ( srcA, srcB ) || !CV_ARE_TYPES_EQ( srcA, dst ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( !CV_ARE_SIZES_EQ( srcA, srcB ) || !CV_ARE_SIZES_EQ( srcA, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    type = CV_MAT_TYPE( dst->type );
    cn = CV_MAT_CN(type);
    rows = srcA->rows;
    cols = srcA->cols;
    is_1d = (flags & CV_DXT_ROWS) ||
            (rows == 1 || cols == 1 &&
             CV_IS_MAT_CONT( srcA->type & srcB->type & dst->type ));

    if( is_1d && !(flags & CV_DXT_ROWS) )
        cols = cols + rows - 1, rows = 1;
    ncols = cols*cn;
    j0 = cn == 1;
    j1 = ncols - (cols % 2 == 0 && cn == 1);

    if( CV_MAT_DEPTH(type) == CV_32F )
    {
        float* dataA = srcA->data.fl;
        float* dataB = srcB->data.fl;
        float* dataC = dst->data.fl;

        stepA = srcA->step/sizeof(dataA[0]);
        stepB = srcB->step/sizeof(dataB[0]);
        stepC = dst->step/sizeof(dataC[0]);

        if( !is_1d && cn == 1 )
        {
            for( k = 0; k < (cols % 2 ? 1 : 2); k++ )
            {
                if( k == 1 )
                    dataA += cols - 1, dataB += cols - 1, dataC += cols - 1;
                dataC[0] = dataA[0]*dataB[0];
                if( rows % 2 == 0 )
                    dataC[(rows-1)*stepC] = dataA[(rows-1)*stepA]*dataB[(rows-1)*stepB];
                if( !(flags & CV_DXT_MUL_CONJ) )
                    for( j = 1; j <= rows - 2; j += 2 )
                    {
                        double re = (double)dataA[j*stepA]*dataB[j*stepB] -
                                    (double)dataA[(j+1)*stepA]*dataB[(j+1)*stepB];
                        double im = (double)dataA[j*stepA]*dataB[(j+1)*stepB] +
                                    (double)dataA[(j+1)*stepA]*dataB[j*stepB];
                        dataC[j*stepC] = (float)re; dataC[(j+1)*stepC] = (float)im;
                    }
                else
                    for( j = 1; j <= rows - 2; j += 2 )
                    {
                        double re = (double)dataA[j*stepA]*dataB[j*stepB] +
                                    (double)dataA[(j+1)*stepA]*dataB[(j+1)*stepB];
                        double im = (double)dataA[(j+1)*stepA]*dataB[j*stepB] -
                                    (double)dataA[j*stepA]*dataB[(j+1)*stepB];
                        dataC[j*stepC] = (float)re; dataC[(j+1)*stepC] = (float)im;
                    }
                if( k == 1 )
                    dataA -= cols - 1, dataB -= cols - 1, dataC -= cols - 1;
            }
        }

        for( ; rows--; dataA += stepA, dataB += stepB, dataC += stepC )
        {
            if( is_1d && cn == 1 )
            {
                dataC[0] = dataA[0]*dataB[0];
                if( cols % 2 == 0 )
                    dataC[j1] = dataA[j1]*dataB[j1];
            }

            if( !(flags & CV_DXT_MUL_CONJ) )
                for( j = j0; j < j1; j += 2 )
                {
                    double re = (double)dataA[j]*dataB[j] - (double)dataA[j+1]*dataB[j+1];
                    double im = (double)dataA[j+1]*dataB[j] + (double)dataA[j]*dataB[j+1];
                    dataC[j] = (float)re; dataC[j+1] = (float)im;
                }
            else
                for( j = j0; j < j1; j += 2 )
                {
                    double re = (double)dataA[j]*dataB[j] + (double)dataA[j+1]*dataB[j+1];
                    double im = (double)dataA[j+1]*dataB[j] - (double)dataA[j]*dataB[j+1];
                    dataC[j] = (float)re; dataC[j+1] = (float)im;
                }
        }
    }
    else if( CV_MAT_DEPTH(type) == CV_64F )
    {
        double* dataA = srcA->data.db;
        double* dataB = srcB->data.db;
        double* dataC = dst->data.db;

        stepA = srcA->step/sizeof(dataA[0]);
        stepB = srcB->step/sizeof(dataB[0]);
        stepC = dst->step/sizeof(dataC[0]);

        if( !is_1d && cn == 1 )
        {
            for( k = 0; k < (cols % 2 ? 1 : 2); k++ )
            {
                if( k == 1 )
                    dataA += cols - 1, dataB += cols - 1, dataC += cols - 1;
                dataC[0] = dataA[0]*dataB[0];
                if( rows % 2 == 0 )
                    dataC[(rows-1)*stepC] = dataA[(rows-1)*stepA]*dataB[(rows-1)*stepB];
                if( !(flags & CV_DXT_MUL_CONJ) )
                    for( j = 1; j <= rows - 2; j += 2 )
                    {
                        double re = dataA[j*stepA]*dataB[j*stepB] -
                                    dataA[(j+1)*stepA]*dataB[(j+1)*stepB];
                        double im = dataA[j*stepA]*dataB[(j+1)*stepB] +
                                    dataA[(j+1)*stepA]*dataB[j*stepB];
                        dataC[j*stepC] = re; dataC[(j+1)*stepC] = im;
                    }
                else
                    for( j = 1; j <= rows - 2; j += 2 )
                    {
                        double re = dataA[j*stepA]*dataB[j*stepB] +
                                    dataA[(j+1)*stepA]*dataB[(j+1)*stepB];
                        double im = dataA[(j+1)*stepA]*dataB[j*stepB] -
                                    dataA[j*stepA]*dataB[(j+1)*stepB];
                        dataC[j*stepC] = re; dataC[(j+1)*stepC] = im;
                    }
                if( k == 1 )
                    dataA -= cols - 1, dataB -= cols - 1, dataC -= cols - 1;
            }
        }

        for( ; rows--; dataA += stepA, dataB += stepB, dataC += stepC )
        {
            if( is_1d && cn == 1 )
            {
                dataC[0] = dataA[0]*dataB[0];
                if( cols % 2 == 0 )
                    dataC[j1] = dataA[j1]*dataB[j1];
            }

            if( !(flags & CV_DXT_MUL_CONJ) )
                for( j = j0; j < j1; j += 2 )
                {
                    double re = dataA[j]*dataB[j] - dataA[j+1]*dataB[j+1];
                    double im = dataA[j+1]*dataB[j] + dataA[j]*dataB[j+1];
                    dataC[j] = re; dataC[j+1] = im;
                }
            else
                for( j = j0; j < j1; j += 2 )
                {
                    double re = dataA[j]*dataB[j] + dataA[j+1]*dataB[j+1];
                    double im = dataA[j+1]*dataB[j] - dataA[j]*dataB[j+1];
                    dataC[j] = re; dataC[j+1] = im;
                }
        }
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "Only 32f and 64f types are supported" );
    }

    __END__;
}

// 
static const int icvOptimalDFTSize[] = {
1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 25, 27, 30, 32, 36, 40, 45, 48, 
50, 54, 60, 64, 72, 75, 80, 81, 90, 96, 100, 108, 120, 125, 128, 135, 144, 150, 160, 
162, 180, 192, 200, 216, 225, 240, 243, 250, 256, 270, 288, 300, 320, 324, 360, 375, 
384, 400, 405, 432, 450, 480, 486, 500, 512, 540, 576, 600, 625, 640, 648, 675, 720, 
729, 750, 768, 800, 810, 864, 900, 960, 972, 1000, 1024, 1080, 1125, 1152, 1200, 
1215, 1250, 1280, 1296, 1350, 1440, 1458, 1500, 1536, 1600, 1620, 1728, 1800, 1875, 
1920, 1944, 2000, 2025, 2048, 2160, 2187, 2250, 2304, 2400, 2430, 2500, 2560, 2592, 
2700, 2880, 2916, 3000, 3072, 3125, 3200, 3240, 3375, 3456, 3600, 3645, 3750, 3840, 
3888, 4000, 4050, 4096, 4320, 4374, 4500, 4608, 4800, 4860, 5000, 5120, 5184, 5400, 
5625, 5760, 5832, 6000, 6075, 6144, 6250, 6400, 6480, 6561, 6750, 6912, 7200, 7290, 
7500, 7680, 7776, 8000, 8100, 8192, 8640, 8748, 9000, 9216, 9375, 9600, 9720, 10000, 
10125, 10240, 10368, 10800, 10935, 11250, 11520, 11664, 12000, 12150, 12288, 12500, 
12800, 12960, 13122, 13500, 13824, 14400, 14580, 15000, 15360, 15552, 15625, 16000, 
16200, 16384, 16875, 17280, 17496, 18000, 18225, 18432, 18750, 19200, 19440, 19683, 
20000, 20250, 20480, 20736, 21600, 21870, 22500, 23040, 23328, 24000, 24300, 24576, 
25000, 25600, 25920, 26244, 27000, 27648, 28125, 28800, 29160, 30000, 30375, 30720, 
31104, 31250, 32000, 32400, 32768, 32805, 33750, 34560, 34992, 36000, 36450, 36864, 
37500, 38400, 38880, 39366, 40000, 40500, 40960, 41472, 43200, 43740, 45000, 46080, 
46656, 46875, 48000, 48600, 49152, 50000, 50625, 51200, 51840, 52488, 54000, 54675, 
55296, 56250, 57600, 58320, 59049, 60000, 60750, 61440, 62208, 62500, 64000, 64800, 
65536, 65610, 67500, 69120, 69984, 72000, 72900, 73728, 75000, 76800, 77760, 78125, 
78732, 80000, 81000, 81920, 82944, 84375, 86400, 87480, 90000, 91125, 92160, 93312, 
93750, 96000, 97200, 98304, 98415, 100000, 101250, 102400, 103680, 104976, 108000, 
109350, 110592, 112500, 115200, 116640, 118098, 120000, 121500, 122880, 124416, 125000, 
128000, 129600, 131072, 131220, 135000, 138240, 139968, 140625, 144000, 145800, 147456, 
150000, 151875, 153600, 155520, 156250, 157464, 160000, 162000, 163840, 164025, 165888, 
168750, 172800, 174960, 177147, 180000, 182250, 184320, 186624, 187500, 192000, 194400, 
196608, 196830, 200000, 202500, 204800, 207360, 209952, 216000, 218700, 221184, 225000, 
230400, 233280, 234375, 236196, 240000, 243000, 245760, 248832, 250000, 253125, 256000, 
259200, 262144, 262440, 270000, 273375, 276480, 279936, 281250, 288000, 291600, 294912, 
295245, 300000, 303750, 307200, 311040, 312500, 314928, 320000, 324000, 327680, 328050, 
331776, 337500, 345600, 349920, 354294, 360000, 364500, 368640, 373248, 375000, 384000, 
388800, 390625, 393216, 393660, 400000, 405000, 409600, 414720, 419904, 421875, 432000, 
437400, 442368, 450000, 455625, 460800, 466560, 468750, 472392, 480000, 486000, 491520, 
492075, 497664, 500000, 506250, 512000, 518400, 524288, 524880, 531441, 540000, 546750, 
552960, 559872, 562500, 576000, 583200, 589824, 590490, 600000, 607500, 614400, 622080, 
625000, 629856, 640000, 648000, 655360, 656100, 663552, 675000, 691200, 699840, 703125, 
708588, 720000, 729000, 737280, 746496, 750000, 759375, 768000, 777600, 781250, 786432, 
787320, 800000, 810000, 819200, 820125, 829440, 839808, 843750, 864000, 874800, 884736, 
885735, 900000, 911250, 921600, 933120, 937500, 944784, 960000, 972000, 983040, 984150, 
995328, 1000000, 1012500, 1024000, 1036800, 1048576, 1049760, 1062882, 1080000, 1093500, 
1105920, 1119744, 1125000, 1152000, 1166400, 1171875, 1179648, 1180980, 1200000, 
1215000, 1228800, 1244160, 1250000, 1259712, 1265625, 1280000, 1296000, 1310720, 
1312200, 1327104, 1350000, 1366875, 1382400, 1399680, 1406250, 1417176, 1440000, 
1458000, 1474560, 1476225, 1492992, 1500000, 1518750, 1536000, 1555200, 1562500, 
1572864, 1574640, 1594323, 1600000, 1620000, 1638400, 1640250, 1658880, 1679616, 
1687500, 1728000, 1749600, 1769472, 1771470, 1800000, 1822500, 1843200, 1866240, 
1875000, 1889568, 1920000, 1944000, 1953125, 1966080, 1968300, 1990656, 2000000, 
2025000, 2048000, 2073600, 2097152, 2099520, 2109375, 2125764, 2160000, 2187000, 
2211840, 2239488, 2250000, 2278125, 2304000, 2332800, 2343750, 2359296, 2361960, 
2400000, 2430000, 2457600, 2460375, 2488320, 2500000, 2519424, 2531250, 2560000, 
2592000, 2621440, 2624400, 2654208, 2657205, 2700000, 2733750, 2764800, 2799360, 
2812500, 2834352, 2880000, 2916000, 2949120, 2952450, 2985984, 3000000, 3037500, 
3072000, 3110400, 3125000, 3145728, 3149280, 3188646, 3200000, 3240000, 3276800, 
3280500, 3317760, 3359232, 3375000, 3456000, 3499200, 3515625, 3538944, 3542940, 
3600000, 3645000, 3686400, 3732480, 3750000, 3779136, 3796875, 3840000, 3888000, 
3906250, 3932160, 3936600, 3981312, 4000000, 4050000, 4096000, 4100625, 4147200, 
4194304, 4199040, 4218750, 4251528, 4320000, 4374000, 4423680, 4428675, 4478976, 
4500000, 4556250, 4608000, 4665600, 4687500, 4718592, 4723920, 4782969, 4800000, 
4860000, 4915200, 4920750, 4976640, 5000000, 5038848, 5062500, 5120000, 5184000, 
5242880, 5248800, 5308416, 5314410, 5400000, 5467500, 5529600, 5598720, 5625000, 
5668704, 5760000, 5832000, 5859375, 5898240, 5904900, 5971968, 6000000, 6075000, 
6144000, 6220800, 6250000, 6291456, 6298560, 6328125, 6377292, 6400000, 6480000, 
6553600, 6561000, 6635520, 6718464, 6750000, 6834375, 6912000, 6998400, 7031250, 
7077888, 7085880, 7200000, 7290000, 7372800, 7381125, 7464960, 7500000, 7558272, 
7593750, 7680000, 7776000, 7812500, 7864320, 7873200, 7962624, 7971615, 8000000, 
8100000, 8192000, 8201250, 8294400, 8388608, 8398080, 8437500, 8503056, 8640000, 
8748000, 8847360, 8857350, 8957952, 9000000, 9112500, 9216000, 9331200, 9375000, 
9437184, 9447840, 9565938, 9600000, 9720000, 9765625, 9830400, 9841500, 9953280, 
10000000, 10077696, 10125000, 10240000, 10368000, 10485760, 10497600, 10546875, 10616832, 
10628820, 10800000, 10935000, 11059200, 11197440, 11250000, 11337408, 11390625, 11520000, 
11664000, 11718750, 11796480, 11809800, 11943936, 12000000, 12150000, 12288000, 12301875, 
12441600, 12500000, 12582912, 12597120, 12656250, 12754584, 12800000, 12960000, 13107200, 
13122000, 13271040, 13286025, 13436928, 13500000, 13668750, 13824000, 13996800, 14062500, 
14155776, 14171760, 14400000, 14580000, 14745600, 14762250, 14929920, 15000000, 15116544, 
15187500, 15360000, 15552000, 15625000, 15728640, 15746400, 15925248, 15943230, 16000000, 
16200000, 16384000, 16402500, 16588800, 16777216, 16796160, 16875000, 17006112, 17280000, 
17496000, 17578125, 17694720, 17714700, 17915904, 18000000, 18225000, 18432000, 18662400, 
18750000, 18874368, 18895680, 18984375, 19131876, 19200000, 19440000, 19531250, 19660800, 
19683000, 19906560, 20000000, 20155392, 20250000, 20480000, 20503125, 20736000, 20971520, 
20995200, 21093750, 21233664, 21257640, 21600000, 21870000, 22118400, 22143375, 22394880, 
22500000, 22674816, 22781250, 23040000, 23328000, 23437500, 23592960, 23619600, 23887872, 
23914845, 24000000, 24300000, 24576000, 24603750, 24883200, 25000000, 25165824, 25194240, 
25312500, 25509168, 25600000, 25920000, 26214400, 26244000, 26542080, 26572050, 26873856, 
27000000, 27337500, 27648000, 27993600, 28125000, 28311552, 28343520, 28800000, 29160000, 
29296875, 29491200, 29524500, 29859840, 30000000, 30233088, 30375000, 30720000, 31104000, 
31250000, 31457280, 31492800, 31640625, 31850496, 31886460, 32000000, 32400000, 32768000, 
32805000, 33177600, 33554432, 33592320, 33750000, 34012224, 34171875, 34560000, 34992000, 
35156250, 35389440, 35429400, 35831808, 36000000, 36450000, 36864000, 36905625, 37324800, 
37500000, 37748736, 37791360, 37968750, 38263752, 38400000, 38880000, 39062500, 39321600, 
39366000, 39813120, 39858075, 40000000, 40310784, 40500000, 40960000, 41006250, 41472000, 
41943040, 41990400, 42187500, 42467328, 42515280, 43200000, 43740000, 44236800, 44286750, 
44789760, 45000000, 45349632, 45562500, 46080000, 46656000, 46875000, 47185920, 47239200, 
47775744, 47829690, 48000000, 48600000, 48828125, 49152000, 49207500, 49766400, 50000000, 
50331648, 50388480, 50625000, 51018336, 51200000, 51840000, 52428800, 52488000, 52734375, 
53084160, 53144100, 53747712, 54000000, 54675000, 55296000, 55987200, 56250000, 56623104, 
56687040, 56953125, 57600000, 58320000, 58593750, 58982400, 59049000, 59719680, 60000000, 
60466176, 60750000, 61440000, 61509375, 62208000, 62500000, 62914560, 62985600, 63281250, 
63700992, 63772920, 64000000, 64800000, 65536000, 65610000, 66355200, 66430125, 67108864, 
67184640, 67500000, 68024448, 68343750, 69120000, 69984000, 70312500, 70778880, 70858800, 
71663616, 72000000, 72900000, 73728000, 73811250, 74649600, 75000000, 75497472, 75582720, 
75937500, 76527504, 76800000, 77760000, 78125000, 78643200, 78732000, 79626240, 79716150, 
80000000, 80621568, 81000000, 81920000, 82012500, 82944000, 83886080, 83980800, 84375000, 
84934656, 85030560, 86400000, 87480000, 87890625, 88473600, 88573500, 89579520, 90000000, 
90699264, 91125000, 92160000, 93312000, 93750000, 94371840, 94478400, 94921875, 95551488, 
95659380, 96000000, 97200000, 97656250, 98304000, 98415000, 99532800, 100000000, 
100663296, 100776960, 101250000, 102036672, 102400000, 102515625, 103680000, 104857600, 
104976000, 105468750, 106168320, 106288200, 107495424, 108000000, 109350000, 110592000, 
110716875, 111974400, 112500000, 113246208, 113374080, 113906250, 115200000, 116640000, 
117187500, 117964800, 118098000, 119439360, 119574225, 120000000, 120932352, 121500000, 
122880000, 123018750, 124416000, 125000000, 125829120, 125971200, 126562500, 127401984, 
127545840, 128000000, 129600000, 131072000, 131220000, 132710400, 132860250, 134217728, 
134369280, 135000000, 136048896, 136687500, 138240000, 139968000, 140625000, 141557760, 
141717600, 143327232, 144000000, 145800000, 146484375, 147456000, 147622500, 149299200, 
150000000, 150994944, 151165440, 151875000, 153055008, 153600000, 155520000, 156250000, 
157286400, 157464000, 158203125, 159252480, 159432300, 160000000, 161243136, 162000000, 
163840000, 164025000, 165888000, 167772160, 167961600, 168750000, 169869312, 170061120, 
170859375, 172800000, 174960000, 175781250, 176947200, 177147000, 179159040, 180000000, 
181398528, 182250000, 184320000, 184528125, 186624000, 187500000, 188743680, 188956800, 
189843750, 191102976, 191318760, 192000000, 194400000, 195312500, 196608000, 196830000, 
199065600, 199290375, 200000000, 201326592, 201553920, 202500000, 204073344, 204800000, 
205031250, 207360000, 209715200, 209952000, 210937500, 212336640, 212576400, 214990848, 
216000000, 218700000, 221184000, 221433750, 223948800, 225000000, 226492416, 226748160, 
227812500, 230400000, 233280000, 234375000, 235929600, 236196000, 238878720, 239148450, 
240000000, 241864704, 243000000, 244140625, 245760000, 246037500, 248832000, 250000000, 
251658240, 251942400, 253125000, 254803968, 255091680, 256000000, 259200000, 262144000, 
262440000, 263671875, 265420800, 265720500, 268435456, 268738560, 270000000, 272097792, 
273375000, 276480000, 279936000, 281250000, 283115520, 283435200, 284765625, 286654464, 
288000000, 291600000, 292968750, 294912000, 295245000, 298598400, 300000000, 301989888, 
302330880, 303750000, 306110016, 307200000, 307546875, 311040000, 312500000, 314572800, 
314928000, 316406250, 318504960, 318864600, 320000000, 322486272, 324000000, 327680000, 
328050000, 331776000, 332150625, 335544320, 335923200, 337500000, 339738624, 340122240, 
341718750, 345600000, 349920000, 351562500, 353894400, 354294000, 358318080, 360000000, 
362797056, 364500000, 368640000, 369056250, 373248000, 375000000, 377487360, 377913600, 
379687500, 382205952, 382637520, 384000000, 388800000, 390625000, 393216000, 393660000, 
398131200, 398580750, 400000000, 402653184, 403107840, 405000000, 408146688, 409600000, 
410062500, 414720000, 419430400, 419904000, 421875000, 424673280, 425152800, 429981696, 
432000000, 437400000, 439453125, 442368000, 442867500, 447897600, 450000000, 452984832, 
453496320, 455625000, 460800000, 466560000, 468750000, 471859200, 472392000, 474609375, 
477757440, 478296900, 480000000, 483729408, 486000000, 488281250, 491520000, 492075000, 
497664000, 500000000, 503316480, 503884800, 506250000, 509607936, 510183360, 512000000, 
512578125, 518400000, 524288000, 524880000, 527343750, 530841600, 531441000, 536870912, 
537477120, 540000000, 544195584, 546750000, 552960000, 553584375, 559872000, 562500000, 
566231040, 566870400, 569531250, 573308928, 576000000, 583200000, 585937500, 589824000, 
590490000, 597196800, 597871125, 600000000, 603979776, 604661760, 607500000, 612220032, 
614400000, 615093750, 622080000, 625000000, 629145600, 629856000, 632812500, 637009920, 
637729200, 640000000, 644972544, 648000000, 655360000, 656100000, 663552000, 664301250, 
671088640, 671846400, 675000000, 679477248, 680244480, 683437500, 691200000, 699840000, 
703125000, 707788800, 708588000, 716636160, 720000000, 725594112, 729000000, 732421875, 
737280000, 738112500, 746496000, 750000000, 754974720, 755827200, 759375000, 764411904, 
765275040, 768000000, 777600000, 781250000, 786432000, 787320000, 791015625, 796262400, 
797161500, 800000000, 805306368, 806215680, 810000000, 816293376, 819200000, 820125000, 
829440000, 838860800, 839808000, 843750000, 849346560, 850305600, 854296875, 859963392, 
864000000, 874800000, 878906250, 884736000, 885735000, 895795200, 900000000, 905969664, 
906992640, 911250000, 921600000, 922640625, 933120000, 937500000, 943718400, 944784000, 
949218750, 955514880, 956593800, 960000000, 967458816, 972000000, 976562500, 983040000, 
984150000, 995328000, 996451875, 1000000000, 1006632960, 1007769600, 1012500000, 
1019215872, 1020366720, 1024000000, 1025156250, 1036800000, 1048576000, 1049760000, 
1054687500, 1061683200, 1062882000, 1073741824, 1074954240, 1080000000, 1088391168, 
1093500000, 1105920000, 1107168750, 1119744000, 1125000000, 1132462080, 1133740800, 
1139062500, 1146617856, 1152000000, 1166400000, 1171875000, 1179648000, 1180980000, 
1194393600, 1195742250, 1200000000, 1207959552, 1209323520, 1215000000, 1220703125, 
1224440064, 1228800000, 1230187500, 1244160000, 1250000000, 1258291200, 1259712000, 
1265625000, 1274019840, 1275458400, 1280000000, 1289945088, 1296000000, 1310720000, 
1312200000, 1318359375, 1327104000, 1328602500, 1342177280, 1343692800, 1350000000, 
1358954496, 1360488960, 1366875000, 1382400000, 1399680000, 1406250000, 1415577600, 
1417176000, 1423828125, 1433272320, 1440000000, 1451188224, 1458000000, 1464843750, 
1474560000, 1476225000, 1492992000, 1500000000, 1509949440, 1511654400, 1518750000, 
1528823808, 1530550080, 1536000000, 1537734375, 1555200000, 1562500000, 1572864000, 
1574640000, 1582031250, 1592524800, 1594323000, 1600000000, 1610612736, 1612431360, 
1620000000, 1632586752, 1638400000, 1640250000, 1658880000, 1660753125, 1677721600, 
1679616000, 1687500000, 1698693120, 1700611200, 1708593750, 1719926784, 1728000000, 
1749600000, 1757812500, 1769472000, 1771470000, 1791590400, 1800000000, 1811939328, 
1813985280, 1822500000, 1843200000, 1845281250, 1866240000, 1875000000, 1887436800, 
1889568000, 1898437500, 1911029760, 1913187600, 1920000000, 1934917632, 1944000000, 
1953125000, 1966080000, 1968300000, 1990656000, 1992903750, 2000000000, 2013265920, 
2015539200, 2025000000, 2038431744, 2040733440, 2048000000, 2050312500, 2073600000, 
2097152000, 2099520000, 2109375000, 2123366400, 2125764000
};
// 

CV_IMPL int
cvGetOptimalDFTSize( int size0 )
{
    int a = 0, b = sizeof(icvOptimalDFTSize)/sizeof(icvOptimalDFTSize[0]) - 1;
    if( (unsigned)size0 >= (unsigned)icvOptimalDFTSize[b] )
        return -1;

    while( a < b )
    {
        int c = (a + b) >> 1;
        if( size0 <= icvOptimalDFTSize[c] )
            b = c;
        else
            a = c+1;
    }

    return icvOptimalDFTSize[b];
}
// 
// /* End of file. */
#define ICV_DEF_UN_LOG_OP_2D( __op__, name )                                            \
static CvStatus CV_STDCALL icv##name##_8u_CnR                                           \
( const uchar* src0, int step1, uchar* dst0, int step, CvSize size,                     \
  const uchar* scalar, int pix_size )                                                   \
{                                                                                       \
    int delta = 12*pix_size;                                                            \
                                                                                        \
    for( ; size.height--; src0 += step1, dst0 += step )                                 \
    {                                                                                   \
        const uchar* src = (const uchar*)src0;                                          \
        uchar* dst = dst0;                                                              \
        int i, len = size.width;                                                        \
                                                                                        \
        if( (((size_t)src|(size_t)dst) & 3) == 0 )                                      \
        {                                                                               \
            while( (len -= delta) >= 0 )                                                \
            {                                                                           \
                for( i = 0; i < (delta); i += 12 )                                      \
                {                                                                       \
                    int t0 = __op__(((const int*)(src+i))[0], ((const int*)(scalar+i))[0]); \
                    int t1 = __op__(((const int*)(src+i))[1], ((const int*)(scalar+i))[1]); \
                    ((int*)(dst+i))[0] = t0;                                            \
                    ((int*)(dst+i))[1] = t1;                                            \
                                                                                        \
                    t0 = __op__(((const int*)(src+i))[2], ((const int*)(scalar+i))[2]); \
                    ((int*)(dst+i))[2] = t0;                                            \
                }                                                                       \
                src += delta;                                                           \
                dst += delta;                                                           \
            }                                                                           \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            while( (len -= delta) >= 0 )                                                \
            {                                                                           \
                for( i = 0; i < (delta); i += 4 )                                       \
                {                                                                       \
                    int t0 = __op__(src[i], scalar[i]);                                 \
                    int t1 = __op__(src[i+1], scalar[i+1]);                             \
                    dst[i] = (uchar)t0;                                                 \
                    dst[i+1] = (uchar)t1;                                               \
                                                                                        \
                    t0 = __op__(src[i+2], scalar[i+2]);                                 \
                    t1 = __op__(src[i+3], scalar[i+3]);                                 \
                    dst[i+2] = (uchar)t0;                                               \
                    dst[i+3] = (uchar)t1;                                               \
                }                                                                       \
                src += delta;                                                           \
                dst += delta;                                                           \
            }                                                                           \
        }                                                                               \
                                                                                        \
        for( len += delta, i = 0; i < len; i++ )                                        \
        {                                                                               \
            int t = __op__(src[i],scalar[i]);                                           \
            dst[i] = (uchar)t;                                                          \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    return CV_OK;                                                                       \
}
// 
// /////////////////////////////////////////////////////////////////////////////////////////
// //                                                                                     //
// //                                LOGIC OPERATIONS                                     //
// //                                                                                     //
// /////////////////////////////////////////////////////////////////////////////////////////
// 
static void
icvLogicS( const void* srcarr, CvScalar* scalar, void* dstarr,
           const void* maskarr, CvFunc2D_2A1P1I fn_2d )
{
    uchar* buffer = 0;
    int local_alloc = 1;
    
    CV_FUNCNAME( "icvLogicS" );
    
    __BEGIN__;

    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvMat dstbuf, *tdst;
    CvCopyMaskFunc copym_func = 0;
    
    int y, dy;
    int coi1 = 0, coi2 = 0;
    int is_nd = 0, cont_flag = 0;
    int elem_size, elem_size1, type, depth;
    double buf[12];
    CvSize size, tsize;
    int src_step, dst_step, tdst_step, mask_step;

    if( !CV_IS_MAT(src))
    {
        if( CV_IS_MATND(src) )
            is_nd = 1;
        else
            CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    }

    if( !CV_IS_MAT(dst))
    {
        if( CV_IS_MATND(dst) )
            is_nd = 1;
        else
            CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));
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

        type = CV_MAT_TYPE(iterator.hdr[0]->type);
        depth = CV_MAT_DEPTH(type);
        iterator.size.width *= CV_ELEM_SIZE(type);
        elem_size1 = CV_ELEM_SIZE1(depth);

        CV_CALL( cvScalarToRawData( scalar, buf, type, 1 ));

        do
        {
            IPPI_CALL( fn_2d( iterator.ptr[0], CV_STUB_STEP,
                              iterator.ptr[1], CV_STUB_STEP,
                              iterator.size, buf, elem_size1 ));
        }
        while( cvNextNArraySlice( &iterator ));
        EXIT;
    }

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    if( !CV_ARE_TYPES_EQ( src, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedFormats );

    if( !CV_ARE_SIZES_EQ( src, dst ) )
        CV_ERROR_FROM_CODE( CV_StsUnmatchedSizes );

    size = cvGetMatSize( src );
    type = CV_MAT_TYPE(src->type);
    depth = CV_MAT_DEPTH(type);
    elem_size = CV_ELEM_SIZE(type);
    elem_size1 = CV_ELEM_SIZE1(depth);

    if( !mask )
    {
        cont_flag = CV_IS_MAT_CONT( src->type & dst->type );
        dy = size.height;
        tdst = dst;
    }
    else
    {
        int buf_size;
        
        if( !CV_IS_MAT(mask) )
            CV_CALL( mask = cvGetMat( mask, &maskstub ));

        if( !CV_IS_MASK_ARR(mask))
            CV_ERROR( CV_StsBadMask, "" );

        if( !CV_ARE_SIZES_EQ( mask, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "" );

        cont_flag = CV_IS_MAT_CONT( src->type & dst->type & mask->type );
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

    src_step = src->step;
    dst_step = dst->step;
    tdst_step = tdst->step;
    mask_step = mask ? mask->step : 0;
    CV_CALL( cvScalarToRawData( scalar, buf, type, 1 ));

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
        IPPI_CALL( fn_2d( src->data.ptr + y*src->step, src_step, tdst->data.ptr, tdst_step,
                          cvSize(tsize.width*elem_size, tsize.height), buf, elem_size1 ));
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

ICV_DEF_UN_LOG_OP_2D( CV_AND, AndC )
 
CV_IMPL void
cvAndS( const void* src, CvScalar scalar, void* dst, const void* mask )
{
    icvLogicS( src, &scalar, dst, mask, (CvFunc2D_2A1P1I)icvAndC_8u_CnR );
}

IPCVAPI_IMPL( CvStatus, icvNot_8u_C1R,
( const uchar* src1, int step1, uchar* dst, int step, CvSize size ),
  (src1, step1, dst, step, size) )
{
    for( ; size.height--; src1 += step1, dst += step )
    {
        int i = 0;

        if( (((size_t)src1 | (size_t)dst) & 3) == 0 )
        {
            for( ; i <= size.width - 16; i += 16 )
            {
                int t0 = ~((const int*)(src1+i))[0];
                int t1 = ~((const int*)(src1+i))[1];

                ((int*)(dst+i))[0] = t0;
                ((int*)(dst+i))[1] = t1;

                t0 = ~((const int*)(src1+i))[2];
                t1 = ~((const int*)(src1+i))[3];

                ((int*)(dst+i))[2] = t0;
                ((int*)(dst+i))[3] = t1;
            }

            for( ; i <= size.width - 4; i += 4 )
            {
                int t = ~*(const int*)(src1+i);
                *(int*)(dst+i) = t;
            }
        }

        for( ; i < size.width; i++ )
        {
            int t = ~((const uchar*)src1)[i];
            dst[i] = (uchar)t;
        }
    }

    return  CV_OK;
}

