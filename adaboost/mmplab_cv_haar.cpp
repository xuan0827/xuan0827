#include "stdafx.h"
#include "mmplab_cv.h"
#include <stdio.h>

/* these settings affect the quality of detection: change with care */
#define CV_ADJUST_FEATURES 1
#define CV_ADJUST_WEIGHTS  0

typedef int sumtype;
typedef double sqsumtype;

typedef struct CvHidHaarFeature
{
    struct
    {
        sumtype *p0, *p1, *p2, *p3;
        float weight;
    }
    rect[CV_HAAR_FEATURE_MAX];
}
CvHidHaarFeature;


typedef struct CvHidHaarTreeNode
{
    CvHidHaarFeature feature;
    float threshold;
    int left;
    int right;
}
CvHidHaarTreeNode;


typedef struct CvHidHaarClassifier
{
    int count;
    //CvHaarFeature* orig_feature;
    CvHidHaarTreeNode* node;
    float* alpha;
}
CvHidHaarClassifier;


typedef struct CvHidHaarStageClassifier
{
    int  count;
    float threshold;
    CvHidHaarClassifier* classifier;
    int two_rects;
    
    struct CvHidHaarStageClassifier* next;
    struct CvHidHaarStageClassifier* child;
    struct CvHidHaarStageClassifier* parent;
}
CvHidHaarStageClassifier;


struct CvHidHaarClassifierCascade
{
    int  count;
    int  is_stump_based;
    int  has_tilted_features;
    int  is_tree;
    double inv_window_area;
    CvMat sum, sqsum, tilted;
    CvHidHaarStageClassifier* stage_classifier;
    sqsumtype *pq0, *pq1, *pq2, *pq3;
    sumtype *p0, *p1, *p2, *p3;

    void** ipp_stages;
};


/* IPP functions for object detection */
icvHaarClassifierInitAlloc_32f_t icvHaarClassifierInitAlloc_32f_p = 0;
icvHaarClassifierFree_32f_t icvHaarClassifierFree_32f_p = 0;
icvApplyHaarClassifier_32s32f_C1R_t icvApplyHaarClassifier_32s32f_C1R_p = 0;
icvRectStdDev_32s32f_C1R_t icvRectStdDev_32s32f_C1R_p = 0;

const int icv_object_win_border = 1;
const float icv_stage_threshold_bias = 0.0001f;

static CvHaarClassifierCascade*
icvCreateHaarClassifierCascade( int stage_count )
{
    CvHaarClassifierCascade* cascade = 0;
    
    CV_FUNCNAME( "icvCreateHaarClassifierCascade" );

    __BEGIN__;

    int block_size = sizeof(*cascade) + stage_count*sizeof(*cascade->stage_classifier);

    if( stage_count <= 0 )
        CV_ERROR( CV_StsOutOfRange, "Number of stages should be positive" );

    CV_CALL( cascade = (CvHaarClassifierCascade*)cvAlloc( block_size ));
    memset( cascade, 0, block_size );

    cascade->stage_classifier = (CvHaarStageClassifier*)(cascade + 1);
    cascade->flags = CV_HAAR_MAGIC_VAL;
    cascade->count = stage_count;

    __END__;

    return cascade;
}

static void
icvReleaseHidHaarClassifierCascade( CvHidHaarClassifierCascade** _cascade )
{
    if( _cascade && *_cascade )
    {
        CvHidHaarClassifierCascade* cascade = *_cascade;
        if( cascade->ipp_stages && icvHaarClassifierFree_32f_p )
        {
            int i;
            for( i = 0; i < cascade->count; i++ )
            {
                if( cascade->ipp_stages[i] )
                    icvHaarClassifierFree_32f_p( cascade->ipp_stages[i] );
            }
        }
        cvFree( &cascade->ipp_stages );
        cvFree( _cascade );
    }
}

/* create more efficient internal representation of haar classifier cascade */
static CvHidHaarClassifierCascade*
icvCreateHidHaarClassifierCascade( CvHaarClassifierCascade* cascade )
{
    CvRect* ipp_features = 0;
    float *ipp_weights = 0, *ipp_thresholds = 0, *ipp_val1 = 0, *ipp_val2 = 0;
    int* ipp_counts = 0;

    CvHidHaarClassifierCascade* out = 0;

    CV_FUNCNAME( "icvCreateHidHaarClassifierCascade" );

    __BEGIN__;

    int i, j, k, l;
    int datasize;
    int total_classifiers = 0;
    int total_nodes = 0;
    char errorstr[100];
    CvHidHaarClassifier* haar_classifier_ptr;
    CvHidHaarTreeNode* haar_node_ptr;
    CvSize orig_window_size;
    int has_tilted_features = 0;
    int max_count = 0;

    if( !CV_IS_HAAR_CLASSIFIER(cascade) )
        CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier pointer" );

    if( cascade->hid_cascade )
        CV_ERROR( CV_StsError, "hid_cascade has been already created" );

    if( !cascade->stage_classifier )
        CV_ERROR( CV_StsNullPtr, "" );

    if( cascade->count <= 0 )
        CV_ERROR( CV_StsOutOfRange, "Negative number of cascade stages" );

    orig_window_size = cascade->orig_window_size;
    
    /* check input structure correctness and calculate total memory size needed for
       internal representation of the classifier cascade */
    for( i = 0; i < cascade->count; i++ )
    {
        CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;

        if( !stage_classifier->classifier ||
            stage_classifier->count <= 0 )
        {
            sprintf( errorstr, "header of the stage classifier #%d is invalid "
                     "(has null pointers or non-positive classfier count)", i );
            CV_ERROR( CV_StsError, errorstr );
        }

        max_count = MAX( max_count, stage_classifier->count );
        total_classifiers += stage_classifier->count;

        for( j = 0; j < stage_classifier->count; j++ )
        {
            CvHaarClassifier* classifier = stage_classifier->classifier + j;

            total_nodes += classifier->count;
            for( l = 0; l < classifier->count; l++ )
            {
                for( k = 0; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    if( classifier->haar_feature[l].rect[k].r.width )
                    {
                        CvRect r = classifier->haar_feature[l].rect[k].r;
                        int tilted = classifier->haar_feature[l].tilted;
                        has_tilted_features |= tilted != 0;
                        if( r.width < 0 || r.height < 0 || r.y < 0 ||
                            r.x + r.width > orig_window_size.width
                            ||
                            (!tilted &&
                            (r.x < 0 || r.y + r.height > orig_window_size.height))
                            ||
                            (tilted && (r.x - r.height < 0 ||
                            r.y + r.width + r.height > orig_window_size.height)))
                        {
                            sprintf( errorstr, "rectangle #%d of the classifier #%d of "
                                     "the stage classifier #%d is not inside "
                                     "the reference (original) cascade window", k, j, i );
                            CV_ERROR( CV_StsNullPtr, errorstr );
                        }
                    }
                }
            }
        }
    }

    // this is an upper boundary for the whole hidden cascade size
    datasize = sizeof(CvHidHaarClassifierCascade) +
               sizeof(CvHidHaarStageClassifier)*cascade->count +
               sizeof(CvHidHaarClassifier) * total_classifiers +
               sizeof(CvHidHaarTreeNode) * total_nodes +
               sizeof(void*)*(total_nodes + total_classifiers);

    CV_CALL( out = (CvHidHaarClassifierCascade*)cvAlloc( datasize ));
    memset( out, 0, sizeof(*out) );

    /* init header */
    out->count = cascade->count;
    out->stage_classifier = (CvHidHaarStageClassifier*)(out + 1);
    haar_classifier_ptr = (CvHidHaarClassifier*)(out->stage_classifier + cascade->count);
    haar_node_ptr = (CvHidHaarTreeNode*)(haar_classifier_ptr + total_classifiers);

    out->is_stump_based = 1;
    out->has_tilted_features = has_tilted_features;
    out->is_tree = 0;

    /* initialize internal representation */
    for( i = 0; i < cascade->count; i++ )
    {
        CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;
        CvHidHaarStageClassifier* hid_stage_classifier = out->stage_classifier + i;

        hid_stage_classifier->count = stage_classifier->count;
        hid_stage_classifier->threshold = stage_classifier->threshold - icv_stage_threshold_bias;
        hid_stage_classifier->classifier = haar_classifier_ptr;
        hid_stage_classifier->two_rects = 1;
        haar_classifier_ptr += stage_classifier->count;

        hid_stage_classifier->parent = (stage_classifier->parent == -1)
            ? NULL : out->stage_classifier + stage_classifier->parent;
        hid_stage_classifier->next = (stage_classifier->next == -1)
            ? NULL : out->stage_classifier + stage_classifier->next;
        hid_stage_classifier->child = (stage_classifier->child == -1)
            ? NULL : out->stage_classifier + stage_classifier->child;
        
        out->is_tree |= hid_stage_classifier->next != NULL;

        for( j = 0; j < stage_classifier->count; j++ )
        {
            CvHaarClassifier* classifier = stage_classifier->classifier + j;
            CvHidHaarClassifier* hid_classifier = hid_stage_classifier->classifier + j;
            int node_count = classifier->count;
            float* alpha_ptr = (float*)(haar_node_ptr + node_count);

            hid_classifier->count = node_count;
            hid_classifier->node = haar_node_ptr;
            hid_classifier->alpha = alpha_ptr;
            
            for( l = 0; l < node_count; l++ )
            {
                CvHidHaarTreeNode* node = hid_classifier->node + l;
                CvHaarFeature* feature = classifier->haar_feature + l;
                memset( node, -1, sizeof(*node) );
                node->threshold = classifier->threshold[l];
                node->left = classifier->left[l];
                node->right = classifier->right[l];

                if( fabs(feature->rect[2].weight) < DBL_EPSILON ||
                    feature->rect[2].r.width == 0 ||
                    feature->rect[2].r.height == 0 )
                    memset( &(node->feature.rect[2]), 0, sizeof(node->feature.rect[2]) );
                else
                    hid_stage_classifier->two_rects = 0;
            }

            memcpy( alpha_ptr, classifier->alpha, (node_count+1)*sizeof(alpha_ptr[0]));
            haar_node_ptr =
                (CvHidHaarTreeNode*)cvAlignPtr(alpha_ptr+node_count+1, sizeof(void*));

            out->is_stump_based &= node_count == 1;
        }
    }

    //
    // NOTE: Currently, OpenMP is implemented and IPP modes are incompatible.
    // 
#ifndef _OPENMP
    {
    int can_use_ipp = icvHaarClassifierInitAlloc_32f_p != 0 &&
        icvHaarClassifierFree_32f_p != 0 &&
                      icvApplyHaarClassifier_32s32f_C1R_p != 0 &&
                      icvRectStdDev_32s32f_C1R_p != 0 &&
                      !out->has_tilted_features && !out->is_tree && out->is_stump_based;

    if( can_use_ipp )
    {
        int ipp_datasize = cascade->count*sizeof(out->ipp_stages[0]);
        float ipp_weight_scale=(float)(1./((orig_window_size.width-icv_object_win_border*2)*
            (orig_window_size.height-icv_object_win_border*2)));

        CV_CALL( out->ipp_stages = (void**)cvAlloc( ipp_datasize ));
        memset( out->ipp_stages, 0, ipp_datasize );

        CV_CALL( ipp_features = (CvRect*)cvAlloc( max_count*3*sizeof(ipp_features[0]) ));
        CV_CALL( ipp_weights = (float*)cvAlloc( max_count*3*sizeof(ipp_weights[0]) ));
        CV_CALL( ipp_thresholds = (float*)cvAlloc( max_count*sizeof(ipp_thresholds[0]) ));
        CV_CALL( ipp_val1 = (float*)cvAlloc( max_count*sizeof(ipp_val1[0]) ));
        CV_CALL( ipp_val2 = (float*)cvAlloc( max_count*sizeof(ipp_val2[0]) ));
        CV_CALL( ipp_counts = (int*)cvAlloc( max_count*sizeof(ipp_counts[0]) ));

        for( i = 0; i < cascade->count; i++ )
        {
            CvHaarStageClassifier* stage_classifier = cascade->stage_classifier + i;
            for( j = 0, k = 0; j < stage_classifier->count; j++ )
            {
                CvHaarClassifier* classifier = stage_classifier->classifier + j;
                int rect_count = 2 + (classifier->haar_feature->rect[2].r.width != 0);

                ipp_thresholds[j] = classifier->threshold[0];
                ipp_val1[j] = classifier->alpha[0];
                ipp_val2[j] = classifier->alpha[1];
                ipp_counts[j] = rect_count;
                
                for( l = 0; l < rect_count; l++, k++ )
                {
                    ipp_features[k] = classifier->haar_feature->rect[l].r;
                    //ipp_features[k].y = orig_window_size.height - ipp_features[k].y - ipp_features[k].height;
                    ipp_weights[k] = classifier->haar_feature->rect[l].weight*ipp_weight_scale;
                }
            }
            
            if( icvHaarClassifierInitAlloc_32f_p( &out->ipp_stages[i],
                ipp_features, ipp_weights, ipp_thresholds,
                ipp_val1, ipp_val2, ipp_counts, stage_classifier->count ) < 0 )
                break;
        }

        if( i < cascade->count )
        {
            for( j = 0; j < i; j++ )
                if( icvHaarClassifierFree_32f_p && out->ipp_stages[i] )
                    icvHaarClassifierFree_32f_p( out->ipp_stages[i] );
            cvFree( &out->ipp_stages );
        }
    }
    }
#endif

    cascade->hid_cascade = out;
    assert( (char*)haar_node_ptr - (char*)out <= datasize );

    __END__;

    if( cvGetErrStatus() < 0 )
        icvReleaseHidHaarClassifierCascade( &out );

    cvFree( &ipp_features );
    cvFree( &ipp_weights );
    cvFree( &ipp_thresholds );
    cvFree( &ipp_val1 );
    cvFree( &ipp_val2 );
    cvFree( &ipp_counts );

    return out;
}


#define sum_elem_ptr(sum,row,col)  \
    ((sumtype*)CV_MAT_ELEM_PTR_FAST((sum),(row),(col),sizeof(sumtype)))

#define sqsum_elem_ptr(sqsum,row,col)  \
    ((sqsumtype*)CV_MAT_ELEM_PTR_FAST((sqsum),(row),(col),sizeof(sqsumtype)))

#define calc_sum(rect,offset) \
    ((rect).p0[offset] - (rect).p1[offset] - (rect).p2[offset] + (rect).p3[offset])

CV_IMPL void
mycvSetImagesForHaarClassifierCascade( CvHaarClassifierCascade* _cascade,
                                     const CvArr* _sum,
                                     const CvArr* _sqsum,
                                     const CvArr* _tilted_sum,
                                     double scale,	//factor = windows scale
									 CvPoint LB,
									 CvPoint RU)
{
    CV_FUNCNAME("mycvSetImagesForHaarClassifierCascade");

    __BEGIN__;

    CvMat sum_stub, *sum = (CvMat*)_sum;
    CvMat sqsum_stub, *sqsum = (CvMat*)_sqsum;
    CvMat tilted_stub, *tilted = (CvMat*)_tilted_sum;
    CvHidHaarClassifierCascade* cascade;
    int coi0 = 0, coi1 = 0;
    int i;
	int _width = abs( LB.x - RU.x );
	int _height = abs( LB.y - RU.y);
    CvRect equ_rect;
    double weight_scale;

    if( !CV_IS_HAAR_CLASSIFIER(_cascade) )
        CV_ERROR( !_cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier pointer" );

    if( scale <= 0 )
        CV_ERROR( CV_StsOutOfRange, "Scale must be positive" );

    CV_CALL( sum = cvGetMat( sum, &sum_stub, &coi0 ));
    CV_CALL( sqsum = cvGetMat( sqsum, &sqsum_stub, &coi1 ));

    if( coi0 || coi1 )
        CV_ERROR( CV_BadCOI, "COI is not supported" );

    if( !CV_ARE_SIZES_EQ( sum, sqsum ))
        CV_ERROR( CV_StsUnmatchedSizes, "All integral images must have the same size" );

    if( CV_MAT_TYPE(sqsum->type) != CV_64FC1 ||
        CV_MAT_TYPE(sum->type) != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Only (32s, 64f, 32s) combination of (sum,sqsum,tilted_sum) formats is allowed" );

    if( !_cascade->hid_cascade )
        CV_CALL( icvCreateHidHaarClassifierCascade(_cascade) );

    cascade = _cascade->hid_cascade;

    if( cascade->has_tilted_features )
    {
        CV_CALL( tilted = cvGetMat( tilted, &tilted_stub, &coi1 ));

        if( CV_MAT_TYPE(tilted->type) != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Only (32s, 64f, 32s) combination of (sum,sqsum,tilted_sum) formats is allowed" );

        if( sum->step != tilted->step )
            CV_ERROR( CV_StsUnmatchedSizes,
            "Sum and tilted_sum must have the same stride (step, widthStep)" );

        if( !CV_ARE_SIZES_EQ( sum, tilted ))
            CV_ERROR( CV_StsUnmatchedSizes, "All integral images must have the same size" );
        cascade->tilted = *tilted;
    }
    //_cascade->orig_window_size.width
	//_cascade->orig_window_size.height     train原本的寬高ex 24,16
    _cascade->scale = scale;
    _cascade->real_window_size.width = cvRound( _cascade->orig_window_size.width * scale );
    _cascade->real_window_size.height = cvRound( _cascade->orig_window_size.height * scale );
// 	_cascade->real_window_size.width = _width;
// 	_cascade->real_window_size.height = _height;


	//Integral image
    cascade->sum = *sum;
    cascade->sqsum = *sqsum;
    
//  equ_rect.x = equ_rect.y = cvRound(scale);
	//window的起始座標LU
	equ_rect.x = LB.x;
	equ_rect.y = RU.y;
	//_cascade->orig_window_size乘上換算scale
	equ_rect.width = cvRound((_cascade->orig_window_size.width-2)*scale);
	equ_rect.height = cvRound((_cascade->orig_window_size.height-2)*scale);
	//手框大小
// 	equ_rect.width = _width;
// 	equ_rect.height = _height;
	weight_scale = 1./(equ_rect.width*equ_rect.height);
    cascade->inv_window_area = weight_scale;

	cascade->p0 = sum_elem_ptr(*sum, equ_rect.y, equ_rect.x);
	cascade->p1 = sum_elem_ptr(*sum, equ_rect.y, equ_rect.x + equ_rect.width );//
    cascade->p2 = sum_elem_ptr(*sum, equ_rect.y + equ_rect.height, equ_rect.x );
    cascade->p3 = sum_elem_ptr(*sum, equ_rect.y + equ_rect.height,
                                     equ_rect.x + equ_rect.width );

    cascade->pq0 = sqsum_elem_ptr(*sqsum, equ_rect.y, equ_rect.x);
    cascade->pq1 = sqsum_elem_ptr(*sqsum, equ_rect.y, equ_rect.x + equ_rect.width );
    cascade->pq2 = sqsum_elem_ptr(*sqsum, equ_rect.y + equ_rect.height, equ_rect.x );
    cascade->pq3 = sqsum_elem_ptr(*sqsum, equ_rect.y + equ_rect.height,
                                          equ_rect.x + equ_rect.width );

    /* init pointers in haar features according to real window size and
       given image pointers */
    {
#ifdef _OPENMP
    int max_threads = cvGetNumThreads();
    #pragma omp parallel for num_threads(max_threads), schedule(dynamic) 
#endif // _OPENMP
    for( i = 0; i < _cascade->count; i++ )
    {
        int j, k, l;
        for( j = 0; j < cascade->stage_classifier[i].count; j++ )
        {
            for( l = 0; l < cascade->stage_classifier[i].classifier[j].count; l++ )
            {
                CvHaarFeature* feature = 
                    &_cascade->stage_classifier[i].classifier[j].haar_feature[l];
                /* CvHidHaarClassifier* classifier =
                    cascade->stage_classifier[i].classifier + j; */
                CvHidHaarFeature* hidfeature = 
                    &cascade->stage_classifier[i].classifier[j].node[l].feature;
                double sum0 = 0, area0 = 0;
                CvRect r[3];
#if CV_ADJUST_FEATURES
                int base_w = -1, base_h = -1;
                int new_base_w = 0, new_base_h = 0;
                int kx, ky;
                int flagx = 0, flagy = 0;
                int x0 = 0, y0 = 0;
#endif
                int nr;

                /* align blocks */
                for( k = 0; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    if( !hidfeature->rect[k].p0 )
                        break;
#if CV_ADJUST_FEATURES
                    r[k] = feature->rect[k].r;
                    base_w = (int)CV_IMIN( (unsigned)base_w, (unsigned)(r[k].width-1) );
                    base_w = (int)CV_IMIN( (unsigned)base_w, (unsigned)(r[k].x - r[0].x-1) );
                    base_h = (int)CV_IMIN( (unsigned)base_h, (unsigned)(r[k].height-1) );
                    base_h = (int)CV_IMIN( (unsigned)base_h, (unsigned)(r[k].y - r[0].y-1) );
#endif
                }

                nr = k;

#if CV_ADJUST_FEATURES
                base_w += 1;
                base_h += 1;
                kx = r[0].width / base_w;
                ky = r[0].height / base_h;

                if( kx <= 0 )
                {
                    flagx = 1;
                    new_base_w = cvRound( r[0].width * scale ) / kx;
                    x0 = cvRound( r[0].x * scale );
                }

                if( ky <= 0 )
                {
                    flagy = 1;
                    new_base_h = cvRound( r[0].height * scale ) / ky;
                    y0 = cvRound( r[0].y * scale );
                }
#endif
        
                for( k = 0; k < nr; k++ )
                {
                    CvRect tr;
                    double correction_ratio;
            
#if CV_ADJUST_FEATURES
                    if( flagx )
                    {
                        tr.x = (r[k].x - r[0].x) * new_base_w / base_w + x0;
                        tr.width = r[k].width * new_base_w / base_w;
                    }
                    else
#endif
                    {
                        tr.x = cvRound( r[k].x * scale );
                        tr.width = cvRound( r[k].width * scale );
                    }

#if CV_ADJUST_FEATURES
                    if( flagy )
                    {
                        tr.y = (r[k].y - r[0].y) * new_base_h / base_h + y0;
                        tr.height = r[k].height * new_base_h / base_h;
                    }
                    else
#endif
                    {
                        tr.y = cvRound( r[k].y * scale );
                        tr.height = cvRound( r[k].height * scale );
                    }

#if CV_ADJUST_WEIGHTS
                    {
                    // RAINER START
                    const float orig_feature_size =  (float)(feature->rect[k].r.width)*feature->rect[k].r.height; 
                    const float orig_norm_size = (float)(_cascade->orig_window_size.width)*(_cascade->orig_window_size.height);
                    const float feature_size = float(tr.width*tr.height);
                    //const float normSize    = float(equ_rect.width*equ_rect.height);
                    float target_ratio = orig_feature_size / orig_norm_size;
                    //float isRatio = featureSize / normSize;
                    //correctionRatio = targetRatio / isRatio / normSize;
                    correction_ratio = target_ratio / feature_size;
                    // RAINER END
                    }
#else
                    correction_ratio = weight_scale * (!feature->tilted ? 1 : 0.5);
#endif

                    if( !feature->tilted )
                    {
                        hidfeature->rect[k].p0 = sum_elem_ptr(*sum, tr.y, tr.x);
                        hidfeature->rect[k].p1 = sum_elem_ptr(*sum, tr.y, tr.x + tr.width);
                        hidfeature->rect[k].p2 = sum_elem_ptr(*sum, tr.y + tr.height, tr.x);
                        hidfeature->rect[k].p3 = sum_elem_ptr(*sum, tr.y + tr.height, tr.x + tr.width);
                    }
                    else
                    {
                        hidfeature->rect[k].p2 = sum_elem_ptr(*tilted, tr.y + tr.width, tr.x + tr.width);
                        hidfeature->rect[k].p3 = sum_elem_ptr(*tilted, tr.y + tr.width + tr.height,
                                                              tr.x + tr.width - tr.height);
                        hidfeature->rect[k].p0 = sum_elem_ptr(*tilted, tr.y, tr.x);
                        hidfeature->rect[k].p1 = sum_elem_ptr(*tilted, tr.y + tr.height, tr.x - tr.height);
                    }

                    hidfeature->rect[k].weight = (float)(feature->rect[k].weight * correction_ratio);

                    if( k == 0 )
                        area0 = tr.width * tr.height;
                    else
                        sum0 += hidfeature->rect[k].weight * tr.width * tr.height;
                }

                hidfeature->rect[0].weight = (float)(-sum0/area0);
            } /* l */
        } /* j */
    }
    }

    __END__;
}


CV_IMPL void
cvSetImagesForHaarClassifierCascade( CvHaarClassifierCascade* _cascade,
                                     const CvArr* _sum,
                                     const CvArr* _sqsum,
                                     const CvArr* _tilted_sum,
                                     double scale )
{
    CV_FUNCNAME("cvSetImagesForHaarClassifierCascade");

    __BEGIN__;

    CvMat sum_stub, *sum = (CvMat*)_sum;
    CvMat sqsum_stub, *sqsum = (CvMat*)_sqsum;
    CvMat tilted_stub, *tilted = (CvMat*)_tilted_sum;
    CvHidHaarClassifierCascade* cascade;
    int coi0 = 0, coi1 = 0;
    int i;
    CvRect equ_rect;
    double weight_scale;

    if( !CV_IS_HAAR_CLASSIFIER(_cascade) )
        CV_ERROR( !_cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier pointer" );

    if( scale <= 0 )
        CV_ERROR( CV_StsOutOfRange, "Scale must be positive" );

    CV_CALL( sum = cvGetMat( sum, &sum_stub, &coi0 ));
    CV_CALL( sqsum = cvGetMat( sqsum, &sqsum_stub, &coi1 ));

    if( coi0 || coi1 )
        CV_ERROR( CV_BadCOI, "COI is not supported" );

    if( !CV_ARE_SIZES_EQ( sum, sqsum ))
        CV_ERROR( CV_StsUnmatchedSizes, "All integral images must have the same size" );

    if( CV_MAT_TYPE(sqsum->type) != CV_64FC1 ||
        CV_MAT_TYPE(sum->type) != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Only (32s, 64f, 32s) combination of (sum,sqsum,tilted_sum) formats is allowed" );

    if( !_cascade->hid_cascade )
        CV_CALL( icvCreateHidHaarClassifierCascade(_cascade) );

    cascade = _cascade->hid_cascade;

    if( cascade->has_tilted_features )
    {
        CV_CALL( tilted = cvGetMat( tilted, &tilted_stub, &coi1 ));

        if( CV_MAT_TYPE(tilted->type) != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Only (32s, 64f, 32s) combination of (sum,sqsum,tilted_sum) formats is allowed" );

        if( sum->step != tilted->step )
            CV_ERROR( CV_StsUnmatchedSizes,
            "Sum and tilted_sum must have the same stride (step, widthStep)" );

        if( !CV_ARE_SIZES_EQ( sum, tilted ))
            CV_ERROR( CV_StsUnmatchedSizes, "All integral images must have the same size" );
        cascade->tilted = *tilted;
    }
    
    _cascade->scale = scale;
    _cascade->real_window_size.width = cvRound( _cascade->orig_window_size.width * scale );
    _cascade->real_window_size.height = cvRound( _cascade->orig_window_size.height * scale );

    cascade->sum = *sum;
    cascade->sqsum = *sqsum;
    
    equ_rect.x = equ_rect.y = cvRound(scale);
    equ_rect.width = cvRound((_cascade->orig_window_size.width-2)*scale);
    equ_rect.height = cvRound((_cascade->orig_window_size.height-2)*scale);
    weight_scale = 1./(equ_rect.width*equ_rect.height);
    cascade->inv_window_area = weight_scale;

	cascade->p0 = sum_elem_ptr(*sum, equ_rect.y, equ_rect.x);
	cascade->p1 = sum_elem_ptr(*sum, equ_rect.y, equ_rect.x + equ_rect.width );//
    cascade->p2 = sum_elem_ptr(*sum, equ_rect.y + equ_rect.height, equ_rect.x );
    cascade->p3 = sum_elem_ptr(*sum, equ_rect.y + equ_rect.height,
                                     equ_rect.x + equ_rect.width );

    cascade->pq0 = sqsum_elem_ptr(*sqsum, equ_rect.y, equ_rect.x);
    cascade->pq1 = sqsum_elem_ptr(*sqsum, equ_rect.y, equ_rect.x + equ_rect.width );
    cascade->pq2 = sqsum_elem_ptr(*sqsum, equ_rect.y + equ_rect.height, equ_rect.x );
    cascade->pq3 = sqsum_elem_ptr(*sqsum, equ_rect.y + equ_rect.height,
                                          equ_rect.x + equ_rect.width );

    /* init pointers in haar features according to real window size and
       given image pointers */
    {
#ifdef _OPENMP
    int max_threads = cvGetNumThreads();
    #pragma omp parallel for num_threads(max_threads), schedule(dynamic) 
#endif // _OPENMP
    for( i = 0; i < _cascade->count; i++ )
    {
        int j, k, l;
        for( j = 0; j < cascade->stage_classifier[i].count; j++ )
        {
            for( l = 0; l < cascade->stage_classifier[i].classifier[j].count; l++ )
            {
                CvHaarFeature* feature = 
                    &_cascade->stage_classifier[i].classifier[j].haar_feature[l];
                /* CvHidHaarClassifier* classifier =
                    cascade->stage_classifier[i].classifier + j; */
                CvHidHaarFeature* hidfeature = 
                    &cascade->stage_classifier[i].classifier[j].node[l].feature;
                double sum0 = 0, area0 = 0;
                CvRect r[3];
#if CV_ADJUST_FEATURES
                int base_w = -1, base_h = -1;
                int new_base_w = 0, new_base_h = 0;
                int kx, ky;
                int flagx = 0, flagy = 0;
                int x0 = 0, y0 = 0;
#endif
                int nr;

                /* align blocks */
                for( k = 0; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    if( !hidfeature->rect[k].p0 )
                        break;
#if CV_ADJUST_FEATURES
                    r[k] = feature->rect[k].r;
                    base_w = (int)CV_IMIN( (unsigned)base_w, (unsigned)(r[k].width-1) );
                    base_w = (int)CV_IMIN( (unsigned)base_w, (unsigned)(r[k].x - r[0].x-1) );
                    base_h = (int)CV_IMIN( (unsigned)base_h, (unsigned)(r[k].height-1) );
                    base_h = (int)CV_IMIN( (unsigned)base_h, (unsigned)(r[k].y - r[0].y-1) );
#endif
                }

                nr = k;

#if CV_ADJUST_FEATURES
                base_w += 1;
                base_h += 1;
                kx = r[0].width / base_w;
                ky = r[0].height / base_h;

                if( kx <= 0 )
                {
                    flagx = 1;
                    new_base_w = cvRound( r[0].width * scale ) / kx;
                    x0 = cvRound( r[0].x * scale );
                }

                if( ky <= 0 )
                {
                    flagy = 1;
                    new_base_h = cvRound( r[0].height * scale ) / ky;
                    y0 = cvRound( r[0].y * scale );
                }
#endif
        
                for( k = 0; k < nr; k++ )
                {
                    CvRect tr;
                    double correction_ratio;
            
#if CV_ADJUST_FEATURES
                    if( flagx )
                    {
                        tr.x = (r[k].x - r[0].x) * new_base_w / base_w + x0;
                        tr.width = r[k].width * new_base_w / base_w;
                    }
                    else
#endif
                    {
                        tr.x = cvRound( r[k].x * scale );
                        tr.width = cvRound( r[k].width * scale );
                    }

#if CV_ADJUST_FEATURES
                    if( flagy )
                    {
                        tr.y = (r[k].y - r[0].y) * new_base_h / base_h + y0;
                        tr.height = r[k].height * new_base_h / base_h;
                    }
                    else
#endif
                    {
                        tr.y = cvRound( r[k].y * scale );
                        tr.height = cvRound( r[k].height * scale );
                    }

#if CV_ADJUST_WEIGHTS
                    {
                    // RAINER START
                    const float orig_feature_size =  (float)(feature->rect[k].r.width)*feature->rect[k].r.height; 
                    const float orig_norm_size = (float)(_cascade->orig_window_size.width)*(_cascade->orig_window_size.height);
                    const float feature_size = float(tr.width*tr.height);
                    //const float normSize    = float(equ_rect.width*equ_rect.height);
                    float target_ratio = orig_feature_size / orig_norm_size;
                    //float isRatio = featureSize / normSize;
                    //correctionRatio = targetRatio / isRatio / normSize;
                    correction_ratio = target_ratio / feature_size;
                    // RAINER END
                    }
#else
                    correction_ratio = weight_scale * (!feature->tilted ? 1 : 0.5);
#endif

                    if( !feature->tilted )
                    {
                        hidfeature->rect[k].p0 = sum_elem_ptr(*sum, tr.y, tr.x);
                        hidfeature->rect[k].p1 = sum_elem_ptr(*sum, tr.y, tr.x + tr.width);
                        hidfeature->rect[k].p2 = sum_elem_ptr(*sum, tr.y + tr.height, tr.x);
                        hidfeature->rect[k].p3 = sum_elem_ptr(*sum, tr.y + tr.height, tr.x + tr.width);
                    }
                    else
                    {
                        hidfeature->rect[k].p2 = sum_elem_ptr(*tilted, tr.y + tr.width, tr.x + tr.width);
                        hidfeature->rect[k].p3 = sum_elem_ptr(*tilted, tr.y + tr.width + tr.height,
                                                              tr.x + tr.width - tr.height);
                        hidfeature->rect[k].p0 = sum_elem_ptr(*tilted, tr.y, tr.x);
                        hidfeature->rect[k].p1 = sum_elem_ptr(*tilted, tr.y + tr.height, tr.x - tr.height);
                    }

                    hidfeature->rect[k].weight = (float)(feature->rect[k].weight * correction_ratio);

                    if( k == 0 )
                        area0 = tr.width * tr.height;
                    else
                        sum0 += hidfeature->rect[k].weight * tr.width * tr.height;
                }

                hidfeature->rect[0].weight = (float)(-sum0/area0);
            } /* l */
        } /* j */
    }
    }

    __END__;
}


CV_INLINE
double icvEvalHidHaarClassifier( CvHidHaarClassifier* classifier,
                                 double variance_norm_factor,
                                 size_t p_offset )
{
    int idx = 0;
    do 
    {
        CvHidHaarTreeNode* node = classifier->node + idx;
        double t = node->threshold * variance_norm_factor;

        double sum = calc_sum(node->feature.rect[0],p_offset) * node->feature.rect[0].weight;
        sum += calc_sum(node->feature.rect[1],p_offset) * node->feature.rect[1].weight;

        if( node->feature.rect[2].p0 )
            sum += calc_sum(node->feature.rect[2],p_offset) * node->feature.rect[2].weight;

        idx = sum < t ? node->left : node->right;
    }
    while( idx > 0 );
    return classifier->alpha[-idx];
}


CV_IMPL int
cvRunHaarClassifierCascade( CvHaarClassifierCascade* _cascade,
                            CvPoint pt, int start_stage )
{
    int result = -1;
    CV_FUNCNAME("cvRunHaarClassifierCascade");

    __BEGIN__;

    int p_offset, pq_offset;
    int i, j;
    double mean, variance_norm_factor;
    CvHidHaarClassifierCascade* cascade;

    if( !CV_IS_HAAR_CLASSIFIER(_cascade) )
        CV_ERROR( !_cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid cascade pointer" );

    cascade = _cascade->hid_cascade;
    if( !cascade )
        CV_ERROR( CV_StsNullPtr, "Hidden cascade has not been created.\n"
            "Use cvSetImagesForHaarClassifierCascade" );

    if( pt.x < 0 || pt.y < 0 ||
        pt.x + _cascade->real_window_size.width >= cascade->sum.width-2 ||
        pt.y + _cascade->real_window_size.height >= cascade->sum.height-2 )
        EXIT;

    p_offset = pt.y * (cascade->sum.step/sizeof(sumtype)) + pt.x;
    pq_offset = pt.y * (cascade->sqsum.step/sizeof(sqsumtype)) + pt.x;
    mean = calc_sum(*cascade,p_offset)*cascade->inv_window_area;
    variance_norm_factor = cascade->pq0[pq_offset] - cascade->pq1[pq_offset] -
                           cascade->pq2[pq_offset] + cascade->pq3[pq_offset];
    variance_norm_factor = variance_norm_factor*cascade->inv_window_area - mean*mean;
    if( variance_norm_factor >= 0. )
        variance_norm_factor = sqrt(variance_norm_factor);
    else
        variance_norm_factor = 1.;

    if( cascade->is_tree )
    {
        CvHidHaarStageClassifier* ptr;
        assert( start_stage == 0 );

        result = 1;
        ptr = cascade->stage_classifier;

        while( ptr )
        {
            double stage_sum = 0;

            for( j = 0; j < ptr->count; j++ )
            {
                stage_sum += icvEvalHidHaarClassifier( ptr->classifier + j, variance_norm_factor, p_offset );
            }

            if( stage_sum >= ptr->threshold )
            {
                ptr = ptr->child;
            }
            else
            {
                while( ptr && ptr->next == NULL ) ptr = ptr->parent;
                if( ptr == NULL )
                {
                    result = 0;
                    EXIT;
                }
                ptr = ptr->next;
            }
        }
    }
    else if( cascade->is_stump_based )
    {
        for( i = start_stage; i < cascade->count; i++ )
        {
            double stage_sum = 0;

            if( cascade->stage_classifier[i].two_rects )
            {
                for( j = 0; j < cascade->stage_classifier[i].count; j++ )
                {
                    CvHidHaarClassifier* classifier = cascade->stage_classifier[i].classifier + j;
                    CvHidHaarTreeNode* node = classifier->node;
                    double sum, t = node->threshold*variance_norm_factor, a, b;

                    sum = calc_sum(node->feature.rect[0],p_offset) * node->feature.rect[0].weight;
                    sum += calc_sum(node->feature.rect[1],p_offset) * node->feature.rect[1].weight;

                    a = classifier->alpha[0];
                    b = classifier->alpha[1];
                    stage_sum += sum < t ? a : b;
                }
            }
            else
            {
                for( j = 0; j < cascade->stage_classifier[i].count; j++ )
                {
                    CvHidHaarClassifier* classifier = cascade->stage_classifier[i].classifier + j;
                    CvHidHaarTreeNode* node = classifier->node;
                    double sum, t = node->threshold*variance_norm_factor, a, b;

                    sum = calc_sum(node->feature.rect[0],p_offset) * node->feature.rect[0].weight;
                    sum += calc_sum(node->feature.rect[1],p_offset) * node->feature.rect[1].weight;

                    if( node->feature.rect[2].p0 )
                        sum += calc_sum(node->feature.rect[2],p_offset) * node->feature.rect[2].weight;

                    a = classifier->alpha[0];
                    b = classifier->alpha[1];
                    stage_sum += sum < t ? a : b;
                }
            }

            if( stage_sum < cascade->stage_classifier[i].threshold )
            {
                result = -i;
                EXIT;
            }
        }
    }
    else
    {
        for( i = start_stage; i < cascade->count; i++ )
        {
            double stage_sum = 0;

            for( j = 0; j < cascade->stage_classifier[i].count; j++ )
            {
                stage_sum += icvEvalHidHaarClassifier(
                    cascade->stage_classifier[i].classifier + j,
                    variance_norm_factor, p_offset );
            }

            if( stage_sum < cascade->stage_classifier[i].threshold )
            {
                result = -i;
                EXIT;
            }
        }
    }

    result = 1;

    __END__;

    return result;
}


static int is_equal( const void* _r1, const void* _r2, void* )
{
    const CvRect* r1 = (const CvRect*)_r1;
    const CvRect* r2 = (const CvRect*)_r2;
    int distance = cvRound(r1->width*0.2);

    return r2->x <= r1->x + distance &&
           r2->x >= r1->x - distance &&
           r2->y <= r1->y + distance &&
           r2->y >= r1->y - distance &&
           r2->width <= cvRound( r1->width * 1.2 ) &&
           cvRound( r2->width * 1.2 ) >= r1->width;
}

CV_IMPL CvSeq*
	mycvHaarDetectObjects( const CvArr* _img,
	CvHaarClassifierCascade* cascade,
	CvMemStorage* storage, double scale_factor,
	int min_neighbors, int flags, CvSize min_size, CvPoint LB, CvPoint RU )
{
	int split_stage = 2;

	CvMat stub, *img = (CvMat*)_img;
	CvMat *temp = 0, *sum = 0, *tilted = 0, *sqsum = 0, *norm_img = 0, *sumcanny = 0, *img_small = 0;
	CvSeq* seq = 0;
	CvSeq* seq2 = 0;
	CvSeq* idx_seq = 0;
	CvSeq* result_seq = 0;
	CvMemStorage* temp_storage = 0;
	CvAvgComp* comps = 0;
	int i;
	int _width = abs(LB.x-RU.x);
	int _height = abs(LB.y-RU.y);
#ifdef _OPENMP
	CvSeq* seq_thread[CV_MAX_THREADS] = {0};
	int max_threads = 0;
#endif

	CV_FUNCNAME( "mycvHaarDetectObjects" );

	__BEGIN__;

	double factor;
	int npass = 2, coi;
	int do_canny_pruning = flags & CV_HAAR_DO_CANNY_PRUNING;

	if( !CV_IS_HAAR_CLASSIFIER(cascade) )
		CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

	if( !storage )
		CV_ERROR( CV_StsNullPtr, "Null storage pointer" );

	CV_CALL( img = cvGetMat( img, &stub, &coi ));
	if( coi )
		CV_ERROR( CV_BadCOI, "COI is not supported" );

	if( CV_MAT_DEPTH(img->type) != CV_8U )
		CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

	CV_CALL( temp = cvCreateMat( img->rows, img->cols, CV_8UC1 ));
	CV_CALL( sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 ));
	CV_CALL( sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 ));
	CV_CALL( temp_storage = cvCreateChildMemStorage( storage ));

#ifdef _OPENMP
	max_threads = cvGetNumThreads();
	for( i = 0; i < max_threads; i++ )
	{
		CvMemStorage* temp_storage_thread;
		CV_CALL( temp_storage_thread = cvCreateMemStorage(0));
		CV_CALL( seq_thread[i] = cvCreateSeq( 0, sizeof(CvSeq),
			sizeof(CvRect), temp_storage_thread ));
	}
#endif

	if( !cascade->hid_cascade )
		CV_CALL( icvCreateHidHaarClassifierCascade(cascade) );

	if( cascade->hid_cascade->has_tilted_features )
		tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

	seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
	seq2 = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), temp_storage );
	result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

	if( min_neighbors == 0 )
		seq = result_seq;

	if( CV_MAT_CN(img->type) > 1 )
	{
		cvCvtColor( img, temp, CV_BGR2GRAY );
		img = temp;
	}

	if( flags & CV_HAAR_SCALE_IMAGE )
	{
// 		CvSize win_size0 = cascade->orig_window_size;
// 		int use_ipp = cascade->hid_cascade->ipp_stages != 0 &&
// 			icvApplyHaarClassifier_32s32f_C1R_p != 0;
// 
// 		if( use_ipp )
// 			CV_CALL( norm_img = cvCreateMat( img->rows, img->cols, CV_32FC1 ));
// 		CV_CALL( img_small = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 ));
// 
// 		for( factor = 1; ; factor *= scale_factor )
// 		{
// 			int positive = 0;
// 			int x, y;
// 			CvSize win_size = { cvRound(win_size0.width*factor),
// 				cvRound(win_size0.height*factor) };
// 			CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
// 			CvSize sz1 = { sz.width - win_size0.width, sz.height - win_size0.height };
// 			CvRect rect1 = { icv_object_win_border, icv_object_win_border,
// 				win_size0.width - icv_object_win_border*2,
// 				win_size0.height - icv_object_win_border*2 };
// 			CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
// 			CvMat* _tilted = 0;
// 
// 			if( sz1.width <= 0 || sz1.height <= 0 )
// 				break;
// 			if( win_size.width < min_size.width || win_size.height < min_size.height )
// 				continue;
// 
// 			img1 = cvMat( sz.height, sz.width, CV_8UC1, img_small->data.ptr );
// 			sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
// 			sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
// 			if( tilted )
// 			{
// 				tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
// 				_tilted = &tilted1;
// 			}
// 			norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, norm_img ? norm_img->data.ptr : 0 );
// 			mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );
// 
// 			cvResize( img, &img1, CV_INTER_LINEAR );
// 			cvIntegral( &img1, &sum1, &sqsum1, _tilted );
// 
// 			if( use_ipp && icvRectStdDev_32s32f_C1R_p( sum1.data.i, sum1.step,
// 				sqsum1.data.db, sqsum1.step, norm1.data.fl, norm1.step, sz1, rect1 ) < 0 )
// 				use_ipp = 0;
// 
// 			if( use_ipp )
// 			{
// 				positive = mask1.cols*mask1.rows;
// 				cvSet( &mask1, cvScalarAll(255) );
// 				for( i = 0; i < cascade->count; i++ )
// 				{
// 					if( icvApplyHaarClassifier_32s32f_C1R_p(sum1.data.i, sum1.step,
// 						norm1.data.fl, norm1.step, mask1.data.ptr, mask1.step,
// 						sz1, &positive, cascade->hid_cascade->stage_classifier[i].threshold,
// 						cascade->hid_cascade->ipp_stages[i]) < 0 )
// 					{
// 						use_ipp = 0;
// 						break;
// 					}
// 					if( positive <= 0 )
// 						break;
// 				}
// 			}
// 
// 			if( !use_ipp )
// 			{
// 				cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, 0, 1. );
// 				for( y = 0, positive = 0; y < sz1.height; y++ )
// 					for( x = 0; x < sz1.width; x++ )
// 					{
// 						mask1.data.ptr[mask1.step*y + x] =
// 							cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0;
// 						positive += mask1.data.ptr[mask1.step*y + x];
// 					}
// 			}
// 
// 			if( positive > 0 )
// 			{
// 				for( y = 0; y < sz1.height; y++ )
// 					for( x = 0; x < sz1.width; x++ )
// 						if( mask1.data.ptr[mask1.step*y + x] != 0 )
// 						{
// 							CvRect obj_rect = { cvRound(y*factor), cvRound(x*factor),
// 								win_size.width, win_size.height };
// 							cvSeqPush( seq, &obj_rect );
// 						}
// 			}
// 		}
	}
	else
	{
		cvIntegral( img, sum, sqsum, tilted );

		if( do_canny_pruning )
		{
			sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
			cvCanny( img, temp, 0, 50, 3 );
			cvIntegral( temp, sumcanny );
		}

		if( (unsigned)split_stage >= (unsigned)cascade->count ||
			cascade->hid_cascade->is_tree )
		{
			split_stage = cascade->count;
			npass = 1;
		}
		factor = (double)_height/cascade->orig_window_size.height;
		for( /*factor = 1*/; factor*cascade->orig_window_size.height <= _height &&
			factor*cascade->orig_window_size.width < img->cols - 10 &&
			factor*cascade->orig_window_size.height < img->rows - 10;
			factor *= scale_factor )
		{//loop of scan window

			const double ystep = MAX( 2, factor );
			CvSize win_size = { cvRound( cascade->orig_window_size.width * factor ),
				cvRound( cascade->orig_window_size.height * factor )};
			//			CvSize win_size = { img->width ,img->height};
			CvRect equ_rect = { 0, 0, 0, 0 };
			int *p0 = 0, *p1 = 0, *p2 = 0, *p3 = 0;
			int *pq0 = 0, *pq1 = 0, *pq2 = 0, *pq3 = 0;
			int pass, stage_offset = 0;
			int stop_height = cvRound((img->rows - win_size.height) / ystep);

			if( win_size.width < min_size.width || win_size.height < min_size.height )
				continue;

			cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
			cvZero( temp );	

			if( do_canny_pruning )
			{
				equ_rect.x = cvRound(win_size.width*0.15);
				equ_rect.y = cvRound(win_size.height*0.15);
				equ_rect.width = cvRound(win_size.width*0.7);
				equ_rect.height = cvRound(win_size.height*0.7);

				p0 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step) + equ_rect.x;
				p1 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step)
					+ equ_rect.x + equ_rect.width;
				p2 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step) + equ_rect.x;
				p3 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step)
					+ equ_rect.x + equ_rect.width;

				pq0 = (int*)(sum->data.ptr + equ_rect.y*sum->step) + equ_rect.x;
				pq1 = (int*)(sum->data.ptr + equ_rect.y*sum->step)
					+ equ_rect.x + equ_rect.width;
				pq2 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step) + equ_rect.x;
				pq3 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step)
					+ equ_rect.x + equ_rect.width;
			}

			cascade->hid_cascade->count = split_stage;

			for( pass = 0; pass < npass; pass++ )
			{
#ifdef _OPENMP
#pragma omp parallel for num_threads(max_threads), schedule(dynamic)
#endif
				//for( int _iy = 0; _iy < stop_height; _iy++ )
				//stop_height = RU.y / ystep + 1;
				//for( int _iy = (RU.y)/ystep - 1 , stop_height = _iy + abs(RU.y - LB.y) / ystep ; _iy < stop_height; _iy++ )
				for( int _iy = (RU.y)/ystep - 1 , stop_height = _iy + cvCeil(win_size.height / ystep) ; _iy < stop_height; _iy++ )
				//int _iy = RU.y/ystep;
				{
					int iy = cvRound(_iy*ystep);
					//int iy = RU.y;
					int _ix, _xstep = 1;
					int stop_width = cvRound((img->cols - win_size.width) / ystep);
					uchar* mask_row = temp->data.ptr + temp->step * iy;

					//for( _ix = 0; _ix < stop_width; _ix += _xstep )
					//stop_width = LB.x/ ystep + 1;
					//for( _ix = (LB.x) / ystep - 3, stop_width = _ix + cascade->real_window_size.width / ystep ; _ix < stop_width; _ix += _xstep )
					for( _ix = (LB.x) / ystep - 3, stop_width = _ix + cvCeil(win_size.width / ystep) ; _ix < stop_width; _ix += _xstep )
					// _ix = LB.x/ ystep;
					{
						int ix = cvRound(_ix*ystep); // it really should be ystep
						//int ix = LB.x;

						if( pass == 0 )
						{
							int result;
							_xstep = 2;

							if( do_canny_pruning )
							{
								int offset;
								int s, sq;

								offset = iy*(sum->step/sizeof(p0[0])) + ix;
								s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
								sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
								if( s < 100 || sq < 20 )
									continue;
							}

							result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy), 0 );
							if( result > 0 )
							{
								if( pass < npass - 1 )
									mask_row[ix] = 1;
								else
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							if( result < 0 )
								_xstep = 1;
						}
						else if( mask_row[ix] )
						{
							int result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy),
								stage_offset );
							if( result > 0 )
							{
								if( pass == npass - 1 )
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							else
								mask_row[ix] = 0;
						}
					}
				}
				stage_offset = cascade->hid_cascade->count;
				cascade->hid_cascade->count = cascade->count;
			}
		}
	}

#ifdef _OPENMP
	// gather the results
	for( i = 0; i < max_threads; i++ )
	{
		CvSeq* s = seq_thread[i];
		int j, total = s->total;
		CvSeqBlock* b = s->first;
		for( j = 0; j < total; j += b->count, b = b->next )
			cvSeqPushMulti( seq, b->data, b->count );
	}
#endif

	if( min_neighbors != 0 )
	{
		// group retrieved rectangles in order to filter out noise 
		int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
		CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
		memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

		// count number of neighbors
		for( i = 0; i < seq->total; i++ )
		{
			CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
			int idx = *(int*)cvGetSeqElem( idx_seq, i );
			assert( (unsigned)idx < (unsigned)ncomp );

			comps[idx].neighbors++;

			comps[idx].rect.x += r1.x;
			comps[idx].rect.y += r1.y;
			comps[idx].rect.width += r1.width;
			comps[idx].rect.height += r1.height;
		}

		// calculate average bounding box
		for( i = 0; i < ncomp; i++ )
		{
			int n = comps[i].neighbors;
			if( n >= min_neighbors )
			{
				CvAvgComp comp;
				comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
				comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
				comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
				comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
				comp.neighbors = comps[i].neighbors;

				cvSeqPush( seq2, &comp );
			}
		}

		// filter out small face rectangles inside large face rectangles
		for( i = 0; i < seq2->total; i++ )
		{
			CvAvgComp r1 = *(CvAvgComp*)cvGetSeqElem( seq2, i );
			int j, flag = 1;

			for( j = 0; j < seq2->total; j++ )
			{
				CvAvgComp r2 = *(CvAvgComp*)cvGetSeqElem( seq2, j );
				int distance = cvRound( r2.rect.width * 0.2 );

				if( i != j &&
					r1.rect.x >= r2.rect.x - distance &&
					r1.rect.y >= r2.rect.y - distance &&
					r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
					r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
					(r2.neighbors > MAX( 3, r1.neighbors ) || r1.neighbors < 3) )
				{
					flag = 0;
					break;
				}
			}

			if( flag )
			{
				cvSeqPush( result_seq, &r1 );
				/* cvSeqPush( result_seq, &r1.rect ); */
			}
		}
	}

	__END__;

#ifdef _OPENMP
	for( i = 0; i < max_threads; i++ )
	{
		if( seq_thread[i] )
			cvReleaseMemStorage( &seq_thread[i]->storage );
	}
#endif

	cvReleaseMemStorage( &temp_storage );
	cvReleaseMat( &sum );
	cvReleaseMat( &sqsum );
	cvReleaseMat( &tilted );
	cvReleaseMat( &temp );
	cvReleaseMat( &sumcanny );
	cvReleaseMat( &norm_img );
	cvReleaseMat( &img_small );
	cvFree( &comps );

	return result_seq;
}

CV_IMPL CvSeq*
	mycvHaarDetectObjects_v2( const CvArr* _img,
	CvHaarClassifierCascade* cascade,
	CvMemStorage* storage, double scale_factor,
	int min_neighbors, int flags, CvSize min_size, int upperbound, int lowerbound )
{
	int split_stage = 2;

	CvMat stub, *img = (CvMat*)_img;
	CvMat *temp = 0, *sum = 0, *tilted = 0, *sqsum = 0, *norm_img = 0, *sumcanny = 0, *img_small = 0;
	CvSeq* seq = 0;
	CvSeq* seq2 = 0;
	CvSeq* idx_seq = 0;
	CvSeq* result_seq = 0;
	CvMemStorage* temp_storage = 0;
	CvAvgComp* comps = 0;
	int i;

#ifdef _OPENMP
	CvSeq* seq_thread[CV_MAX_THREADS] = {0};
	int max_threads = 0;
#endif

	CV_FUNCNAME( "mycvHaarDetectObjects_v2" );

	__BEGIN__;

	double factor;
	int npass = 2, coi;
	int do_canny_pruning = flags & CV_HAAR_DO_CANNY_PRUNING;

	if( !CV_IS_HAAR_CLASSIFIER(cascade) )
		CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

	if( !storage )
		CV_ERROR( CV_StsNullPtr, "Null storage pointer" );

	CV_CALL( img = cvGetMat( img, &stub, &coi ));
	if( coi )
		CV_ERROR( CV_BadCOI, "COI is not supported" );

	if( CV_MAT_DEPTH(img->type) != CV_8U )
		CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

	CV_CALL( temp = cvCreateMat( img->rows, img->cols, CV_8UC1 ));
	CV_CALL( sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 ));
	CV_CALL( sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 ));
	CV_CALL( temp_storage = cvCreateChildMemStorage( storage ));

#ifdef _OPENMP
	max_threads = cvGetNumThreads();
	for( i = 0; i < max_threads; i++ )
	{
		CvMemStorage* temp_storage_thread;
		CV_CALL( temp_storage_thread = cvCreateMemStorage(0));
		CV_CALL( seq_thread[i] = cvCreateSeq( 0, sizeof(CvSeq),
			sizeof(CvRect), temp_storage_thread ));
	}
#endif

	if( !cascade->hid_cascade )
		CV_CALL( icvCreateHidHaarClassifierCascade(cascade) );

	if( cascade->hid_cascade->has_tilted_features )
		tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

	seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
	seq2 = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), temp_storage );
	result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

	if( min_neighbors == 0 )
		seq = result_seq;

	if( CV_MAT_CN(img->type) > 1 )
	{
		cvCvtColor( img, temp, CV_BGR2GRAY );
		img = temp;
	}

	if( flags & CV_HAAR_SCALE_IMAGE )
	{
		CvSize win_size0 = cascade->orig_window_size;
		int use_ipp = cascade->hid_cascade->ipp_stages != 0 &&
			icvApplyHaarClassifier_32s32f_C1R_p != 0;

		if( use_ipp )
			CV_CALL( norm_img = cvCreateMat( img->rows, img->cols, CV_32FC1 ));
		CV_CALL( img_small = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 ));

		for( factor = 1; ; factor *= scale_factor )
		{
			int positive = 0;
			int x, y;
			CvSize win_size = { cvRound(win_size0.width*factor),
				cvRound(win_size0.height*factor) };
			CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
			CvSize sz1 = { sz.width - win_size0.width, sz.height - win_size0.height };
			CvRect rect1 = { icv_object_win_border, icv_object_win_border,
				win_size0.width - icv_object_win_border*2,
				win_size0.height - icv_object_win_border*2 };
			CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
			CvMat* _tilted = 0;

			if( sz1.width <= 0 || sz1.height <= 0 )
				break;
			if( win_size.width < min_size.width || win_size.height < min_size.height )
				continue;

			img1 = cvMat( sz.height, sz.width, CV_8UC1, img_small->data.ptr );
			sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
			sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
			if( tilted )
			{
				tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
				_tilted = &tilted1;
			}
			norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, norm_img ? norm_img->data.ptr : 0 );
			mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );

			cvResize( img, &img1, CV_INTER_LINEAR );
			cvIntegral( &img1, &sum1, &sqsum1, _tilted );

			if( use_ipp && icvRectStdDev_32s32f_C1R_p( sum1.data.i, sum1.step,
				sqsum1.data.db, sqsum1.step, norm1.data.fl, norm1.step, sz1, rect1 ) < 0 )
				use_ipp = 0;

			if( use_ipp )
			{
				positive = mask1.cols*mask1.rows;
				cvSet( &mask1, cvScalarAll(255) );
				for( i = 0; i < cascade->count; i++ )
				{
					if( icvApplyHaarClassifier_32s32f_C1R_p(sum1.data.i, sum1.step,
						norm1.data.fl, norm1.step, mask1.data.ptr, mask1.step,
						sz1, &positive, cascade->hid_cascade->stage_classifier[i].threshold,
						cascade->hid_cascade->ipp_stages[i]) < 0 )
					{
						use_ipp = 0;
						break;
					}
					if( positive <= 0 )
						break;
				}
			}

			if( !use_ipp )
			{
				cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, 0, 1. );
				for( y = 0, positive = 0; y < sz1.height; y++ )
					for( x = 0; x < sz1.width; x++ )
					{
						mask1.data.ptr[mask1.step*y + x] =
							cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0;
						positive += mask1.data.ptr[mask1.step*y + x];
					}
			}

			if( positive > 0 )
			{
				for( y = 0; y < sz1.height; y++ )
					for( x = 0; x < sz1.width; x++ )
						if( mask1.data.ptr[mask1.step*y + x] != 0 )
						{
							CvRect obj_rect = { cvRound(y*factor), cvRound(x*factor),
								win_size.width, win_size.height };
							cvSeqPush( seq, &obj_rect );
						}
			}
		}
	}
	else
	{
		cvIntegral( img, sum, sqsum, tilted );

		if( do_canny_pruning )
		{
			sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
			cvCanny( img, temp, 0, 50, 3 );
			cvIntegral( temp, sumcanny );
		}

		if( (unsigned)split_stage >= (unsigned)cascade->count ||
			cascade->hid_cascade->is_tree )
		{
			split_stage = cascade->count;
			npass = 1;
		}
		factor = (double)lowerbound / cascade->orig_window_size.height;
		for( /*factor = 1*/; factor*cascade->orig_window_size.height < upperbound &&
			factor*cascade->orig_window_size.width < img->cols - 10 &&
			factor*cascade->orig_window_size.height < img->rows - 10;
			factor *= scale_factor )
		{//loop of scan window

			const double ystep = MAX( 2, factor );
			CvSize win_size = { cvRound( cascade->orig_window_size.width * factor ),
				cvRound( cascade->orig_window_size.height * factor )};
			//			CvSize win_size = { img->width ,img->height};
			CvRect equ_rect = { 0, 0, 0, 0 };
			int *p0 = 0, *p1 = 0, *p2 = 0, *p3 = 0;
			int *pq0 = 0, *pq1 = 0, *pq2 = 0, *pq3 = 0;
			int pass, stage_offset = 0;
			int stop_height = cvRound((img->rows - win_size.height) / ystep);

			if( win_size.width < min_size.width || win_size.height < min_size.height )
				continue;

			cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
			cvZero( temp );	

			if( do_canny_pruning )
			{
				equ_rect.x = cvRound(win_size.width*0.15);
				equ_rect.y = cvRound(win_size.height*0.15);
				equ_rect.width = cvRound(win_size.width*0.7);
				equ_rect.height = cvRound(win_size.height*0.7);

				p0 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step) + equ_rect.x;
				p1 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step)
					+ equ_rect.x + equ_rect.width;
				p2 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step) + equ_rect.x;
				p3 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step)
					+ equ_rect.x + equ_rect.width;

				pq0 = (int*)(sum->data.ptr + equ_rect.y*sum->step) + equ_rect.x;
				pq1 = (int*)(sum->data.ptr + equ_rect.y*sum->step)
					+ equ_rect.x + equ_rect.width;
				pq2 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step) + equ_rect.x;
				pq3 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step)
					+ equ_rect.x + equ_rect.width;
			}

			cascade->hid_cascade->count = split_stage;

			for( pass = 0; pass < npass; pass++ )
			{
#ifdef _OPENMP
#pragma omp parallel for num_threads(max_threads), schedule(dynamic)
#endif
				for( int _iy = 0; _iy < stop_height; _iy++ )
				{
					int iy = cvRound(_iy*ystep);
					int _ix, _xstep = 1;
					int stop_width = cvRound((img->cols - win_size.width) / ystep);
					uchar* mask_row = temp->data.ptr + temp->step * iy;

					for( _ix = 0; _ix < stop_width; _ix += _xstep )
					{
						int ix = cvRound(_ix*ystep); // it really should be ystep

						if( pass == 0 )
						{
							int result;
							_xstep = 2;

							if( do_canny_pruning )
							{
								int offset;
								int s, sq;

								offset = iy*(sum->step/sizeof(p0[0])) + ix;
								s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
								sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
								if( s < 100 || sq < 20 )
									continue;
							}

							result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy), 0 );
							if( result > 0 )
							{
								if( pass < npass - 1 )
									mask_row[ix] = 1;
								else
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							if( result < 0 )
								_xstep = 1;
						}
						else if( mask_row[ix] )
						{
							int result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy),
								stage_offset );
							if( result > 0 )
							{
								if( pass == npass - 1 )
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							else
								mask_row[ix] = 0;
						}
					}
				}
				stage_offset = cascade->hid_cascade->count;
				cascade->hid_cascade->count = cascade->count;
			}
		}
	}

#ifdef _OPENMP
	// gather the results
	for( i = 0; i < max_threads; i++ )
	{
		CvSeq* s = seq_thread[i];
		int j, total = s->total;
		CvSeqBlock* b = s->first;
		for( j = 0; j < total; j += b->count, b = b->next )
			cvSeqPushMulti( seq, b->data, b->count );
	}
#endif

	if( min_neighbors != 0 )
	{
		// group retrieved rectangles in order to filter out noise 
		int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
		CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
		memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

		// count number of neighbors
		for( i = 0; i < seq->total; i++ )
		{
			CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
			int idx = *(int*)cvGetSeqElem( idx_seq, i );
			assert( (unsigned)idx < (unsigned)ncomp );

			comps[idx].neighbors++;

			comps[idx].rect.x += r1.x;
			comps[idx].rect.y += r1.y;
			comps[idx].rect.width += r1.width;
			comps[idx].rect.height += r1.height;
		}

		// calculate average bounding box
		for( i = 0; i < ncomp; i++ )
		{
			int n = comps[i].neighbors;
			if( n >= min_neighbors )
			{
				CvAvgComp comp;
				comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
				comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
				comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
				comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
				comp.neighbors = comps[i].neighbors;

				cvSeqPush( seq2, &comp );
			}
		}

		// filter out small face rectangles inside large face rectangles
		for( i = 0; i < seq2->total; i++ )
		{
			CvAvgComp r1 = *(CvAvgComp*)cvGetSeqElem( seq2, i );
			int j, flag = 1;

			for( j = 0; j < seq2->total; j++ )
			{
				CvAvgComp r2 = *(CvAvgComp*)cvGetSeqElem( seq2, j );
				int distance = cvRound( r2.rect.width * 0.2 );

				if( i != j &&
					r1.rect.x >= r2.rect.x - distance &&
					r1.rect.y >= r2.rect.y - distance &&
					r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
					r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
					(r2.neighbors > MAX( 3, r1.neighbors ) || r1.neighbors < 3) )
				{
					flag = 0;
					break;
				}
			}

			if( flag )
			{
				cvSeqPush( result_seq, &r1 );
				/* cvSeqPush( result_seq, &r1.rect ); */
			}
		}
	}

	__END__;

#ifdef _OPENMP
	for( i = 0; i < max_threads; i++ )
	{
		if( seq_thread[i] )
			cvReleaseMemStorage( &seq_thread[i]->storage );
	}
#endif

	cvReleaseMemStorage( &temp_storage );
	cvReleaseMat( &sum );
	cvReleaseMat( &sqsum );
	cvReleaseMat( &tilted );
	cvReleaseMat( &temp );
	cvReleaseMat( &sumcanny );
	cvReleaseMat( &norm_img );
	cvReleaseMat( &img_small );
	cvFree( &comps );

	return result_seq;
}

CV_IMPL CvSeq*
	mycvHaarDetectObjects_v3( const CvArr* _img,
	CvHaarClassifierCascade* cascade,
	CvMemStorage* storage, double scale_factor,
	int min_neighbors, int flags, CvSize min_size, int upperbound, int lowerbound, CvPoint LB, CvPoint RU )
{
	int split_stage = 2;

	CvMat stub, *img = (CvMat*)_img;
	CvMat *temp = 0, *sum = 0, *tilted = 0, *sqsum = 0, *norm_img = 0, *sumcanny = 0, *img_small = 0;
	CvSeq* seq = 0;
	CvSeq* seq2 = 0;
	CvSeq* idx_seq = 0;
	CvSeq* result_seq = 0;
	CvMemStorage* temp_storage = 0;
	CvAvgComp* comps = 0;
	int i;

#ifdef _OPENMP
	CvSeq* seq_thread[CV_MAX_THREADS] = {0};
	int max_threads = 0;
#endif

	CV_FUNCNAME( "mycvHaarDetectObjects_v3" );

	__BEGIN__;

	double factor;
	int npass = 2, coi;
	int do_canny_pruning = flags & CV_HAAR_DO_CANNY_PRUNING;

	if( !CV_IS_HAAR_CLASSIFIER(cascade) )
		CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

	if( !storage )
		CV_ERROR( CV_StsNullPtr, "Null storage pointer" );

	CV_CALL( img = cvGetMat( img, &stub, &coi ));
	if( coi )
		CV_ERROR( CV_BadCOI, "COI is not supported" );

	if( CV_MAT_DEPTH(img->type) != CV_8U )
		CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

	CV_CALL( temp = cvCreateMat( img->rows, img->cols, CV_8UC1 ));
	CV_CALL( sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 ));
	CV_CALL( sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 ));
	CV_CALL( temp_storage = cvCreateChildMemStorage( storage ));

#ifdef _OPENMP
	max_threads = cvGetNumThreads();
	for( i = 0; i < max_threads; i++ )
	{
		CvMemStorage* temp_storage_thread;
		CV_CALL( temp_storage_thread = cvCreateMemStorage(0));
		CV_CALL( seq_thread[i] = cvCreateSeq( 0, sizeof(CvSeq),
			sizeof(CvRect), temp_storage_thread ));
	}
#endif

	if( !cascade->hid_cascade )
		CV_CALL( icvCreateHidHaarClassifierCascade(cascade) );

	if( cascade->hid_cascade->has_tilted_features )
		tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

	seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
	seq2 = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), temp_storage );
	result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

	if( min_neighbors == 0 )
		seq = result_seq;

	if( CV_MAT_CN(img->type) > 1 )
	{
		cvCvtColor( img, temp, CV_BGR2GRAY );
		img = temp;
	}

	if( flags & CV_HAAR_SCALE_IMAGE )
	{
		CvSize win_size0 = cascade->orig_window_size;
		int use_ipp = cascade->hid_cascade->ipp_stages != 0 &&
			icvApplyHaarClassifier_32s32f_C1R_p != 0;

		if( use_ipp )
			CV_CALL( norm_img = cvCreateMat( img->rows, img->cols, CV_32FC1 ));
		CV_CALL( img_small = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 ));

		for( factor = 1; ; factor *= scale_factor )
		{
			int positive = 0;
			int x, y;
			CvSize win_size = { cvRound(win_size0.width*factor),
				cvRound(win_size0.height*factor) };
			CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
			CvSize sz1 = { sz.width - win_size0.width, sz.height - win_size0.height };
			CvRect rect1 = { icv_object_win_border, icv_object_win_border,
				win_size0.width - icv_object_win_border*2,
				win_size0.height - icv_object_win_border*2 };
			CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
			CvMat* _tilted = 0;

			if( sz1.width <= 0 || sz1.height <= 0 )
				break;
			if( win_size.width < min_size.width || win_size.height < min_size.height )
				continue;

			img1 = cvMat( sz.height, sz.width, CV_8UC1, img_small->data.ptr );
			sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
			sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
			if( tilted )
			{
				tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
				_tilted = &tilted1;
			}
			norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, norm_img ? norm_img->data.ptr : 0 );
			mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );

			cvResize( img, &img1, CV_INTER_LINEAR );
			cvIntegral( &img1, &sum1, &sqsum1, _tilted );

			if( use_ipp && icvRectStdDev_32s32f_C1R_p( sum1.data.i, sum1.step,
				sqsum1.data.db, sqsum1.step, norm1.data.fl, norm1.step, sz1, rect1 ) < 0 )
				use_ipp = 0;

			if( use_ipp )
			{
				positive = mask1.cols*mask1.rows;
				cvSet( &mask1, cvScalarAll(255) );
				for( i = 0; i < cascade->count; i++ )
				{
					if( icvApplyHaarClassifier_32s32f_C1R_p(sum1.data.i, sum1.step,
						norm1.data.fl, norm1.step, mask1.data.ptr, mask1.step,
						sz1, &positive, cascade->hid_cascade->stage_classifier[i].threshold,
						cascade->hid_cascade->ipp_stages[i]) < 0 )
					{
						use_ipp = 0;
						break;
					}
					if( positive <= 0 )
						break;
				}
			}

			if( !use_ipp )
			{
				cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, 0, 1. );
				for( y = 0, positive = 0; y < sz1.height; y++ )
					for( x = 0; x < sz1.width; x++ )
					{
						mask1.data.ptr[mask1.step*y + x] =
							cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0;
						positive += mask1.data.ptr[mask1.step*y + x];
					}
			}

			if( positive > 0 )
			{
				for( y = 0; y < sz1.height; y++ )
					for( x = 0; x < sz1.width; x++ )
						if( mask1.data.ptr[mask1.step*y + x] != 0 )
						{
							CvRect obj_rect = { cvRound(y*factor), cvRound(x*factor),
								win_size.width, win_size.height };
							cvSeqPush( seq, &obj_rect );
						}
			}
		}
	}
	else
	{
		cvIntegral( img, sum, sqsum, tilted );

		if( do_canny_pruning )
		{
			sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
			cvCanny( img, temp, 0, 50, 3 );
			cvIntegral( temp, sumcanny );
		}

		if( (unsigned)split_stage >= (unsigned)cascade->count ||
			cascade->hid_cascade->is_tree )
		{
			split_stage = cascade->count;
			npass = 1;
		}
		factor = (double)lowerbound / cascade->orig_window_size.height;
		for( /*factor = 1*/; factor*cascade->orig_window_size.height <= upperbound &&
			factor*cascade->orig_window_size.width < img->cols - 10 &&
			factor*cascade->orig_window_size.height < img->rows - 10;
		factor *= scale_factor )
		{//loop of scan window

			const double ystep = MAX( 2, factor );
			CvSize win_size = { cvRound( cascade->orig_window_size.width * factor ),
				cvRound( cascade->orig_window_size.height * factor )};
			//			CvSize win_size = { img->width ,img->height};
			CvRect equ_rect = { 0, 0, 0, 0 };
			int *p0 = 0, *p1 = 0, *p2 = 0, *p3 = 0;
			int *pq0 = 0, *pq1 = 0, *pq2 = 0, *pq3 = 0;
			int pass, stage_offset = 0;
			int stop_height = cvRound((img->rows - win_size.height) / ystep);

			if( win_size.width < min_size.width || win_size.height < min_size.height )
				continue;

			cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
			cvZero( temp );	

			if( do_canny_pruning )
			{
				equ_rect.x = cvRound(win_size.width*0.15);
				equ_rect.y = cvRound(win_size.height*0.15);
				equ_rect.width = cvRound(win_size.width*0.7);
				equ_rect.height = cvRound(win_size.height*0.7);

				p0 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step) + equ_rect.x;
				p1 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step)
					+ equ_rect.x + equ_rect.width;
				p2 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step) + equ_rect.x;
				p3 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step)
					+ equ_rect.x + equ_rect.width;

				pq0 = (int*)(sum->data.ptr + equ_rect.y*sum->step) + equ_rect.x;
				pq1 = (int*)(sum->data.ptr + equ_rect.y*sum->step)
					+ equ_rect.x + equ_rect.width;
				pq2 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step) + equ_rect.x;
				pq3 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step)
					+ equ_rect.x + equ_rect.width;
			}

			cascade->hid_cascade->count = split_stage;

			for( pass = 0; pass < npass; pass++ )
			{
#ifdef _OPENMP
#pragma omp parallel for num_threads(max_threads), schedule(dynamic)
#endif
				//for( int _iy = 0; _iy < stop_height; _iy++ )
				//for( int _iy = (RU.y)/ystep , stop_height = _iy + abs(RU.y - LB.y) / ystep ; _iy < stop_height; _iy++ )
				for(int _iy = (RU.y)/ystep , stop_height = _iy + abs(abs(RU.y - LB.y) - win_size.height )/ystep ; _iy < stop_height ; _iy++)
				{
					int iy = cvRound(_iy*ystep);
					int _ix, _xstep = 1;
					int stop_width = cvRound((img->cols - win_size.width) / ystep);
					uchar* mask_row = temp->data.ptr + temp->step * iy;

					//for( _ix = 0; _ix < stop_width; _ix += _xstep )
					//for( _ix = (LB.x) / ystep, stop_width = _ix + abs(LB.x - RU.x) /ystep ; _ix < stop_width; _ix += _xstep )
					for(_ix = (LB.x)/ystep, stop_width = _ix + abs(abs(LB.x - RU.x) - win_size.width ) / ystep ; _ix < stop_width ; _ix += _xstep)
					{
						int ix = cvRound(_ix*ystep); // it really should be ystep

						if( pass == 0 )
						{
							int result;
							_xstep = 2;

							if( do_canny_pruning )
							{
								int offset;
								int s, sq;

								offset = iy*(sum->step/sizeof(p0[0])) + ix;
								s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
								sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
								if( s < 100 || sq < 20 )
									continue;
							}

							result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy), 0 );
							if( result > 0 )
							{
								if( pass < npass - 1 )
									mask_row[ix] = 1;
								else
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							if( result < 0 )
								_xstep = 1;
						}
						else if( mask_row[ix] )
						{
							int result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy),
								stage_offset );
							if( result > 0 )
							{
								if( pass == npass - 1 )
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							else
								mask_row[ix] = 0;
						}
					}
				}
				stage_offset = cascade->hid_cascade->count;
				cascade->hid_cascade->count = cascade->count;
			}
		}
	}

#ifdef _OPENMP
	// gather the results
	for( i = 0; i < max_threads; i++ )
	{
		CvSeq* s = seq_thread[i];
		int j, total = s->total;
		CvSeqBlock* b = s->first;
		for( j = 0; j < total; j += b->count, b = b->next )
			cvSeqPushMulti( seq, b->data, b->count );
	}
#endif

	if( min_neighbors != 0 )
	{
		// group retrieved rectangles in order to filter out noise 
		int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
		CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
		memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

		// count number of neighbors
		for( i = 0; i < seq->total; i++ )
		{
			CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
			int idx = *(int*)cvGetSeqElem( idx_seq, i );
			assert( (unsigned)idx < (unsigned)ncomp );

			comps[idx].neighbors++;

			comps[idx].rect.x += r1.x;
			comps[idx].rect.y += r1.y;
			comps[idx].rect.width += r1.width;
			comps[idx].rect.height += r1.height;
		}

		// calculate average bounding box
		for( i = 0; i < ncomp; i++ )
		{
			int n = comps[i].neighbors;
			if( n >= min_neighbors )
			{
				CvAvgComp comp;
				comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
				comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
				comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
				comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
				comp.neighbors = comps[i].neighbors;

				cvSeqPush( seq2, &comp );
			}
		}

		// filter out small face rectangles inside large face rectangles
		for( i = 0; i < seq2->total; i++ )
		{
			CvAvgComp r1 = *(CvAvgComp*)cvGetSeqElem( seq2, i );
			int j, flag = 1;

			for( j = 0; j < seq2->total; j++ )
			{
				CvAvgComp r2 = *(CvAvgComp*)cvGetSeqElem( seq2, j );
				int distance = cvRound( r2.rect.width * 0.2 );

				if( i != j &&
					r1.rect.x >= r2.rect.x - distance &&
					r1.rect.y >= r2.rect.y - distance &&
					r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
					r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
					(r2.neighbors > MAX( 3, r1.neighbors ) || r1.neighbors < 3) )
				{
					flag = 0;
					break;
				}
			}

			if( flag )
			{
				cvSeqPush( result_seq, &r1 );
				/* cvSeqPush( result_seq, &r1.rect ); */
			}
		}
	}

	__END__;

#ifdef _OPENMP
	for( i = 0; i < max_threads; i++ )
	{
		if( seq_thread[i] )
			cvReleaseMemStorage( &seq_thread[i]->storage );
	}
#endif

	cvReleaseMemStorage( &temp_storage );
	cvReleaseMat( &sum );
	cvReleaseMat( &sqsum );
	cvReleaseMat( &tilted );
	cvReleaseMat( &temp );
	cvReleaseMat( &sumcanny );
	cvReleaseMat( &norm_img );
	cvReleaseMat( &img_small );
	cvFree( &comps );

	return result_seq;
}

CV_IMPL CvSeq*
	mycvHaarDetectObjects_v4( const CvArr* _img,
	CvHaarClassifierCascade* cascade,
	CvMemStorage* storage, double scale_factor,
	int min_neighbors, int flags, CvSize min_size, CvPoint LB, CvPoint RU )
{
	int split_stage = 2;
	//cvIntegral( img, sum, sqsum, tilted )
	CvMat stub, *img = (CvMat*)_img;
	CvMat *temp = 0, *sum = 0, *tilted = 0, *sqsum = 0, *norm_img = 0, *sumcanny = 0, *img_small = 0;
	CvSeq* seq = 0;
	CvSeq* seq2 = 0;
	CvSeq* idx_seq = 0;
	CvSeq* result_seq = 0;
	CvMemStorage* temp_storage = 0;
	CvAvgComp* comps = 0;
	int i;
	int _width = abs(LB.x-RU.x);
	int _height = abs(LB.y-RU.y);
#ifdef _OPENMP
	CvSeq* seq_thread[CV_MAX_THREADS] = {0};
	int max_threads = 0;
#endif

	CV_FUNCNAME( "mycvHaarDetectObjects_v4" );

	__BEGIN__;

	double factor;
	int npass = 2, coi;
	int do_canny_pruning = flags & CV_HAAR_DO_CANNY_PRUNING;

	if( !CV_IS_HAAR_CLASSIFIER(cascade) )
		CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

	if( !storage )
		CV_ERROR( CV_StsNullPtr, "Null storage pointer" );

	CV_CALL( img = cvGetMat( img, &stub, &coi ));
	if( coi )
		CV_ERROR( CV_BadCOI, "COI is not supported" );

	if( CV_MAT_DEPTH(img->type) != CV_8U )
		CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

	CV_CALL( temp = cvCreateMat( img->rows, img->cols, CV_8UC1 ));
	CV_CALL( sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 ));
	CV_CALL( sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 ));
	CV_CALL( temp_storage = cvCreateChildMemStorage( storage ));

#ifdef _OPENMP
	max_threads = cvGetNumThreads();
	for( i = 0; i < max_threads; i++ )
	{
		CvMemStorage* temp_storage_thread;
		CV_CALL( temp_storage_thread = cvCreateMemStorage(0));
		CV_CALL( seq_thread[i] = cvCreateSeq( 0, sizeof(CvSeq),
			sizeof(CvRect), temp_storage_thread ));
	}
#endif

	if( !cascade->hid_cascade )
		CV_CALL( icvCreateHidHaarClassifierCascade(cascade) );

	if( cascade->hid_cascade->has_tilted_features )
		tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

	seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
	seq2 = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), temp_storage );
	result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

	if( min_neighbors == 0 )
		seq = result_seq;

	if( CV_MAT_CN(img->type) > 1 )
	{
		cvCvtColor( img, temp, CV_BGR2GRAY );
		img = temp;
	}

	if( flags & CV_HAAR_SCALE_IMAGE )
	{
		// 		CvSize win_size0 = cascade->orig_window_size;
		// 		int use_ipp = cascade->hid_cascade->ipp_stages != 0 &&
		// 			icvApplyHaarClassifier_32s32f_C1R_p != 0;
		// 
		// 		if( use_ipp )
		// 			CV_CALL( norm_img = cvCreateMat( img->rows, img->cols, CV_32FC1 ));
		// 		CV_CALL( img_small = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 ));
		// 
		// 		for( factor = 1; ; factor *= scale_factor )
		// 		{
		// 			int positive = 0;
		// 			int x, y;
		// 			CvSize win_size = { cvRound(win_size0.width*factor),
		// 				cvRound(win_size0.height*factor) };
		// 			CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
		// 			CvSize sz1 = { sz.width - win_size0.width, sz.height - win_size0.height };
		// 			CvRect rect1 = { icv_object_win_border, icv_object_win_border,
		// 				win_size0.width - icv_object_win_border*2,
		// 				win_size0.height - icv_object_win_border*2 };
		// 			CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
		// 			CvMat* _tilted = 0;
		// 
		// 			if( sz1.width <= 0 || sz1.height <= 0 )
		// 				break;
		// 			if( win_size.width < min_size.width || win_size.height < min_size.height )
		// 				continue;
		// 
		// 			img1 = cvMat( sz.height, sz.width, CV_8UC1, img_small->data.ptr );
		// 			sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
		// 			sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
		// 			if( tilted )
		// 			{
		// 				tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
		// 				_tilted = &tilted1;
		// 			}
		// 			norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, norm_img ? norm_img->data.ptr : 0 );
		// 			mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );
		// 
		// 			cvResize( img, &img1, CV_INTER_LINEAR );
		// 			cvIntegral( &img1, &sum1, &sqsum1, _tilted );
		// 
		// 			if( use_ipp && icvRectStdDev_32s32f_C1R_p( sum1.data.i, sum1.step,
		// 				sqsum1.data.db, sqsum1.step, norm1.data.fl, norm1.step, sz1, rect1 ) < 0 )
		// 				use_ipp = 0;
		// 
		// 			if( use_ipp )
		// 			{
		// 				positive = mask1.cols*mask1.rows;
		// 				cvSet( &mask1, cvScalarAll(255) );
		// 				for( i = 0; i < cascade->count; i++ )
		// 				{
		// 					if( icvApplyHaarClassifier_32s32f_C1R_p(sum1.data.i, sum1.step,
		// 						norm1.data.fl, norm1.step, mask1.data.ptr, mask1.step,
		// 						sz1, &positive, cascade->hid_cascade->stage_classifier[i].threshold,
		// 						cascade->hid_cascade->ipp_stages[i]) < 0 )
		// 					{
		// 						use_ipp = 0;
		// 						break;
		// 					}
		// 					if( positive <= 0 )
		// 						break;
		// 				}
		// 			}
		// 
		// 			if( !use_ipp )
		// 			{
		// 				cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, 0, 1. );
		// 				for( y = 0, positive = 0; y < sz1.height; y++ )
		// 					for( x = 0; x < sz1.width; x++ )
		// 					{
		// 						mask1.data.ptr[mask1.step*y + x] =
		// 							cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0;
		// 						positive += mask1.data.ptr[mask1.step*y + x];
		// 					}
		// 			}
		// 
		// 			if( positive > 0 )
		// 			{
		// 				for( y = 0; y < sz1.height; y++ )
		// 					for( x = 0; x < sz1.width; x++ )
		// 						if( mask1.data.ptr[mask1.step*y + x] != 0 )
		// 						{
		// 							CvRect obj_rect = { cvRound(y*factor), cvRound(x*factor),
		// 								win_size.width, win_size.height };
		// 							cvSeqPush( seq, &obj_rect );
		// 						}
		// 			}
		// 		}
	}
	else
	{
		cvIntegral( img, sum, sqsum, tilted );//function外先做

		if( do_canny_pruning )
		{
			sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
			cvCanny( img, temp, 0, 50, 3 );
			cvIntegral( temp, sumcanny );
		}

		if( (unsigned)split_stage >= (unsigned)cascade->count ||
			cascade->hid_cascade->is_tree )
		{
			split_stage = cascade->count;
			npass = 1;
		}
		factor = (double)_height/cascade->orig_window_size.height;
// 		for( /*factor = 1*/; factor*cascade->orig_window_size.height <= _height &&
// 			factor*cascade->orig_window_size.width < img->cols - 10 &&
// 			factor*cascade->orig_window_size.height < img->rows - 10;
// 		factor *= scale_factor )
		{//loop of scan window

			const double ystep = MAX( 2, factor );
			CvSize win_size = { cvRound( cascade->orig_window_size.width * factor ), cvRound( cascade->orig_window_size.height * factor )};
				
			win_size.width = _width;
			win_size.height = _height;

			CvRect equ_rect = { 0, 0, 0, 0 };
			int *p0 = 0, *p1 = 0, *p2 = 0, *p3 = 0;
			int *pq0 = 0, *pq1 = 0, *pq2 = 0, *pq3 = 0;
			int pass, stage_offset = 0;
			int stop_height;// = cvRound((img->rows - win_size.height) / ystep);

// 			if( win_size.width < min_size.width || win_size.height < min_size.height )
// 				continue;

			cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
			cvZero( temp );	

			if( do_canny_pruning )
			{
				equ_rect.x = cvRound(win_size.width*0.15);
				equ_rect.y = cvRound(win_size.height*0.15);
				equ_rect.width = cvRound(win_size.width*0.7);
				equ_rect.height = cvRound(win_size.height*0.7);

				p0 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step) + equ_rect.x;
				p1 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step)
					+ equ_rect.x + equ_rect.width;
				p2 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step) + equ_rect.x;
				p3 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step)
					+ equ_rect.x + equ_rect.width;

				pq0 = (int*)(sum->data.ptr + equ_rect.y*sum->step) + equ_rect.x;
				pq1 = (int*)(sum->data.ptr + equ_rect.y*sum->step)
					+ equ_rect.x + equ_rect.width;
				pq2 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step) + equ_rect.x;
				pq3 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step)
					+ equ_rect.x + equ_rect.width;
			}

			cascade->hid_cascade->count = split_stage;

			for( pass = 0; pass < npass; pass++ )
			{
#ifdef _OPENMP
#pragma omp parallel for num_threads(max_threads), schedule(dynamic)
#endif
				//for( int _iy = 0; _iy < stop_height; _iy++ )
				//stop_height = RU.y / ystep + 1;
				//for( int _iy = (RU.y)/ystep - 1 , stop_height = _iy + abs(RU.y - LB.y) / ystep ; _iy < stop_height; _iy++ )
				//for( int _iy = (RU.y)/ystep - 1 , stop_height = _iy + cvCeil(win_size.height / ystep) ; _iy < stop_height; _iy++ )
				int _iy = (RU.y)/ystep ;
				int tmp = (abs(RU.y - LB.y) - win_size.height );
				if(tmp <= 0)
					tmp = 1;
				stop_height = _iy + tmp / ystep + 1;
				for(  ; _iy <= stop_height; _iy++ )
					//int _iy = RU.y/ystep;
				{
					int iy = cvRound(_iy*ystep);
					//int iy = RU.y;
					int _ix, _xstep = 1;
					int stop_width ;//= cvRound((img->cols - win_size.width) / ystep);
					uchar* mask_row = temp->data.ptr + temp->step * iy;

					//for( _ix = 0; _ix < stop_width; _ix += _xstep )
					//stop_width = LB.x/ ystep + 1;
					//for( _ix = (LB.x) / ystep - 3, stop_width = _ix + cascade->real_window_size.width / ystep ; _ix < stop_width; _ix += _xstep )
					//for( _ix = (LB.x) / ystep - 3, stop_width = _ix + cvCeil(win_size.width / ystep) ; _ix < stop_width; _ix += _xstep )
					_ix = (LB.x) / ystep;
					tmp = (abs(RU.x - LB.x) - win_size.width );
					if(tmp <= 0)
						tmp = 1;
					stop_width = _ix + tmp / ystep + 3;
					for(; _ix <= stop_width ; _ix += _xstep )
						// _ix = LB.x/ ystep;
					{
						int ix = cvRound(_ix*ystep); // it really should be ystep
						//int ix = LB.x;

						if( pass == 0 )
						{
							int result;
							_xstep = 2;

							if( do_canny_pruning )
							{
								int offset;
								int s, sq;

								offset = iy*(sum->step/sizeof(p0[0])) + ix;
								s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
								sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
								if( s < 100 || sq < 20 )
									continue;
							}

							result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy), 0 );
							if( result > 0 )
							{
								if( pass < npass - 1 )
									mask_row[ix] = 1;
								else
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							if( result < 0 )
								_xstep = 1;
						}
						else if( mask_row[ix] )
						{
							int result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy),
								stage_offset );
							if( result > 0 )
							{
								if( pass == npass - 1 )
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							else
								mask_row[ix] = 0;
						}
					}
				}
				stage_offset = cascade->hid_cascade->count;
				cascade->hid_cascade->count = cascade->count;
			}
		}
	}

#ifdef _OPENMP
	// gather the results
	for( i = 0; i < max_threads; i++ )
	{
		CvSeq* s = seq_thread[i];
		int j, total = s->total;
		CvSeqBlock* b = s->first;
		for( j = 0; j < total; j += b->count, b = b->next )
			cvSeqPushMulti( seq, b->data, b->count );
	}
#endif

	if( min_neighbors != 0 )
	{
		// group retrieved rectangles in order to filter out noise 
		int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
		CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
		memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

		// count number of neighbors
		for( i = 0; i < seq->total; i++ )
		{
			CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
			int idx = *(int*)cvGetSeqElem( idx_seq, i );
			assert( (unsigned)idx < (unsigned)ncomp );

			comps[idx].neighbors++;

			comps[idx].rect.x += r1.x;
			comps[idx].rect.y += r1.y;
			comps[idx].rect.width += r1.width;
			comps[idx].rect.height += r1.height;
		}

		// calculate average bounding box
		for( i = 0; i < ncomp; i++ )
		{
			int n = comps[i].neighbors;
			if( n >= min_neighbors )
			{
				CvAvgComp comp;
				comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
				comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
				comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
				comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
				comp.neighbors = comps[i].neighbors;

				cvSeqPush( seq2, &comp );
			}
		}

		// filter out small face rectangles inside large face rectangles
		for( i = 0; i < seq2->total; i++ )
		{
			CvAvgComp r1 = *(CvAvgComp*)cvGetSeqElem( seq2, i );
			int j, flag = 1;

			for( j = 0; j < seq2->total; j++ )
			{
				CvAvgComp r2 = *(CvAvgComp*)cvGetSeqElem( seq2, j );
				int distance = cvRound( r2.rect.width * 0.2 );

				if( i != j &&
					r1.rect.x >= r2.rect.x - distance &&
					r1.rect.y >= r2.rect.y - distance &&
					r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
					r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
					(r2.neighbors > MAX( 3, r1.neighbors ) || r1.neighbors < 3) )
				{
					flag = 0;
					break;
				}
			}

			if( flag )
			{
				cvSeqPush( result_seq, &r1 );
				/* cvSeqPush( result_seq, &r1.rect ); */
			}
		}
	}

	__END__;

#ifdef _OPENMP
	for( i = 0; i < max_threads; i++ )
	{
		if( seq_thread[i] )
			cvReleaseMemStorage( &seq_thread[i]->storage );
	}
#endif

	cvReleaseMemStorage( &temp_storage );
	cvReleaseMat( &sum );
	cvReleaseMat( &sqsum );
	cvReleaseMat( &tilted );
	cvReleaseMat( &temp );
	cvReleaseMat( &sumcanny );
	cvReleaseMat( &norm_img );
	cvReleaseMat( &img_small );
	cvFree( &comps );

	return result_seq;
}

CV_IMPL CvSeq*
	mycvHaarDetectObjects_v5( const CvArr* _img,
	CvHaarClassifierCascade* cascade,
	CvMemStorage* storage, double scale_factor,
	int min_neighbors, int flags, CvSize min_size, CvPoint LB, CvPoint RU )
{
	int split_stage = 2;
	//cvIntegral( img, sum, sqsum, tilted )
	CvMat stub, *img = (CvMat*)_img;
	CvMat *temp = 0, *sum = 0, *tilted = 0, *sqsum = 0, *norm_img = 0, *sumcanny = 0, *img_small = 0;
	CvSeq* seq = 0;
	CvSeq* seq2 = 0;
	CvSeq* idx_seq = 0;
	CvSeq* result_seq = 0;
	CvMemStorage* temp_storage = 0;
	CvAvgComp* comps = 0;
	int i;
	int _width = abs(LB.x-RU.x);
	int _height = abs(LB.y-RU.y);
#ifdef _OPENMP
	CvSeq* seq_thread[CV_MAX_THREADS] = {0};
	int max_threads = 0;
#endif

	CV_FUNCNAME( "mycvHaarDetectObjects_v5" );

	__BEGIN__;

	double factor;
	int npass = 2, coi;
	int do_canny_pruning = flags & CV_HAAR_DO_CANNY_PRUNING;

	if( !CV_IS_HAAR_CLASSIFIER(cascade) )
		CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

	if( !storage )
		CV_ERROR( CV_StsNullPtr, "Null storage pointer" );

	CV_CALL( img = cvGetMat( img, &stub, &coi ));
	if( coi )
		CV_ERROR( CV_BadCOI, "COI is not supported" );

	if( CV_MAT_DEPTH(img->type) != CV_8U )
		CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

	CV_CALL( temp = cvCreateMat( img->rows, img->cols, CV_8UC1 ));
	CV_CALL( sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 ));
	CV_CALL( sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 ));
	CV_CALL( temp_storage = cvCreateChildMemStorage( storage ));

#ifdef _OPENMP
	max_threads = cvGetNumThreads();
	for( i = 0; i < max_threads; i++ )
	{
		CvMemStorage* temp_storage_thread;
		CV_CALL( temp_storage_thread = cvCreateMemStorage(0));
		CV_CALL( seq_thread[i] = cvCreateSeq( 0, sizeof(CvSeq),
			sizeof(CvRect), temp_storage_thread ));
	}
#endif

	if( !cascade->hid_cascade )
		CV_CALL( icvCreateHidHaarClassifierCascade(cascade) );

	if( cascade->hid_cascade->has_tilted_features )
		tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

	seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
	seq2 = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), temp_storage );
	result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

	if( min_neighbors == 0 )
		seq = result_seq;

	if( CV_MAT_CN(img->type) > 1 )
	{
		cvCvtColor( img, temp, CV_BGR2GRAY );
		img = temp;
	}

	if( flags & CV_HAAR_SCALE_IMAGE )
	{
		// 		CvSize win_size0 = cascade->orig_window_size;
		// 		int use_ipp = cascade->hid_cascade->ipp_stages != 0 &&
		// 			icvApplyHaarClassifier_32s32f_C1R_p != 0;
		// 
		// 		if( use_ipp )
		// 			CV_CALL( norm_img = cvCreateMat( img->rows, img->cols, CV_32FC1 ));
		// 		CV_CALL( img_small = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 ));
		// 
		// 		for( factor = 1; ; factor *= scale_factor )
		// 		{
		// 			int positive = 0;
		// 			int x, y;
		// 			CvSize win_size = { cvRound(win_size0.width*factor),
		// 				cvRound(win_size0.height*factor) };
		// 			CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
		// 			CvSize sz1 = { sz.width - win_size0.width, sz.height - win_size0.height };
		// 			CvRect rect1 = { icv_object_win_border, icv_object_win_border,
		// 				win_size0.width - icv_object_win_border*2,
		// 				win_size0.height - icv_object_win_border*2 };
		// 			CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
		// 			CvMat* _tilted = 0;
		// 
		// 			if( sz1.width <= 0 || sz1.height <= 0 )
		// 				break;
		// 			if( win_size.width < min_size.width || win_size.height < min_size.height )
		// 				continue;
		// 
		// 			img1 = cvMat( sz.height, sz.width, CV_8UC1, img_small->data.ptr );
		// 			sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
		// 			sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
		// 			if( tilted )
		// 			{
		// 				tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
		// 				_tilted = &tilted1;
		// 			}
		// 			norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, norm_img ? norm_img->data.ptr : 0 );
		// 			mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );
		// 
		// 			cvResize( img, &img1, CV_INTER_LINEAR );
		// 			cvIntegral( &img1, &sum1, &sqsum1, _tilted );
		// 
		// 			if( use_ipp && icvRectStdDev_32s32f_C1R_p( sum1.data.i, sum1.step,
		// 				sqsum1.data.db, sqsum1.step, norm1.data.fl, norm1.step, sz1, rect1 ) < 0 )
		// 				use_ipp = 0;
		// 
		// 			if( use_ipp )
		// 			{
		// 				positive = mask1.cols*mask1.rows;
		// 				cvSet( &mask1, cvScalarAll(255) );
		// 				for( i = 0; i < cascade->count; i++ )
		// 				{
		// 					if( icvApplyHaarClassifier_32s32f_C1R_p(sum1.data.i, sum1.step,
		// 						norm1.data.fl, norm1.step, mask1.data.ptr, mask1.step,
		// 						sz1, &positive, cascade->hid_cascade->stage_classifier[i].threshold,
		// 						cascade->hid_cascade->ipp_stages[i]) < 0 )
		// 					{
		// 						use_ipp = 0;
		// 						break;
		// 					}
		// 					if( positive <= 0 )
		// 						break;
		// 				}
		// 			}
		// 
		// 			if( !use_ipp )
		// 			{
		// 				cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, 0, 1. );
		// 				for( y = 0, positive = 0; y < sz1.height; y++ )
		// 					for( x = 0; x < sz1.width; x++ )
		// 					{
		// 						mask1.data.ptr[mask1.step*y + x] =
		// 							cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0;
		// 						positive += mask1.data.ptr[mask1.step*y + x];
		// 					}
		// 			}
		// 
		// 			if( positive > 0 )
		// 			{
		// 				for( y = 0; y < sz1.height; y++ )
		// 					for( x = 0; x < sz1.width; x++ )
		// 						if( mask1.data.ptr[mask1.step*y + x] != 0 )
		// 						{
		// 							CvRect obj_rect = { cvRound(y*factor), cvRound(x*factor),
		// 								win_size.width, win_size.height };
		// 							cvSeqPush( seq, &obj_rect );
		// 						}
		// 			}
		// 		}
	}
	else
	{
		cvIntegral( img, sum, sqsum, tilted );//function外先做

		if( do_canny_pruning )
		{
			sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
			cvCanny( img, temp, 0, 50, 3 );
			cvIntegral( temp, sumcanny );
		}

		if( (unsigned)split_stage >= (unsigned)cascade->count ||
			cascade->hid_cascade->is_tree )
		{
			split_stage = cascade->count;
			npass = 1;
		}
		factor = (double)_height/cascade->orig_window_size.height;
		// 		for( /*factor = 1*/; factor*cascade->orig_window_size.height <= _height &&
		// 			factor*cascade->orig_window_size.width < img->cols - 10 &&
		// 			factor*cascade->orig_window_size.height < img->rows - 10;
		// 		factor *= scale_factor )
		{//loop of scan window

			const double ystep = MAX( 2, factor );
			CvSize win_size = { cvRound( cascade->orig_window_size.width * factor ), cvRound( cascade->orig_window_size.height * factor )};

			win_size.width = _width;
			win_size.height = _height;

			CvRect equ_rect = { 0, 0, 0, 0 };
			int *p0 = 0, *p1 = 0, *p2 = 0, *p3 = 0;
			int *pq0 = 0, *pq1 = 0, *pq2 = 0, *pq3 = 0;
			int pass, stage_offset = 0;
			int stop_height;// = cvRound((img->rows - win_size.height) / ystep);

			// 			if( win_size.width < min_size.width || win_size.height < min_size.height )
			// 				continue;

			cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
			cvZero( temp );	

			if( do_canny_pruning )
			{
				equ_rect.x = cvRound(win_size.width*0.15);
				equ_rect.y = cvRound(win_size.height*0.15);
				equ_rect.width = cvRound(win_size.width*0.7);
				equ_rect.height = cvRound(win_size.height*0.7);

				p0 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step) + equ_rect.x;
				p1 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step)
					+ equ_rect.x + equ_rect.width;
				p2 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step) + equ_rect.x;
				p3 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step)
					+ equ_rect.x + equ_rect.width;

				pq0 = (int*)(sum->data.ptr + equ_rect.y*sum->step) + equ_rect.x;
				pq1 = (int*)(sum->data.ptr + equ_rect.y*sum->step)
					+ equ_rect.x + equ_rect.width;
				pq2 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step) + equ_rect.x;
				pq3 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step)
					+ equ_rect.x + equ_rect.width;
			}

			cascade->hid_cascade->count = split_stage;

			for( pass = 0; pass < npass; pass++ )
			{
#ifdef _OPENMP
#pragma omp parallel for num_threads(max_threads), schedule(dynamic)
#endif
				//for( int _iy = 0; _iy < stop_height; _iy++ )
				//stop_height = RU.y / ystep + 1;
				//for( int _iy = (RU.y)/ystep - 1 , stop_height = _iy + abs(RU.y - LB.y) / ystep ; _iy < stop_height; _iy++ )
				//for( int _iy = (RU.y)/ystep - 1 , stop_height = _iy + cvCeil(win_size.height / ystep) ; _iy < stop_height; _iy++ )
				int _iy = (RU.y)/ystep ;
				int tmp = (abs(RU.y - LB.y) - win_size.height );
				if(tmp <= 0)
					tmp = 1;
				stop_height = _iy + tmp / ystep + 1;
				for(  ; _iy <= stop_height; _iy++ )
					//int _iy = RU.y/ystep;
				{
					int iy = cvRound(_iy*ystep);
					//int iy = RU.y;
					int _ix, _xstep = 1;
					int stop_width ;//= cvRound((img->cols - win_size.width) / ystep);
					uchar* mask_row = temp->data.ptr + temp->step * iy;

					//for( _ix = 0; _ix < stop_width; _ix += _xstep )
					//stop_width = LB.x/ ystep + 1;
					//for( _ix = (LB.x) / ystep - 3, stop_width = _ix + cascade->real_window_size.width / ystep ; _ix < stop_width; _ix += _xstep )
					//for( _ix = (LB.x) / ystep - 3, stop_width = _ix + cvCeil(win_size.width / ystep) ; _ix < stop_width; _ix += _xstep )
					_ix = (LB.x) / ystep;
					tmp = (abs(RU.x - LB.x) - win_size.width );
					if(tmp <= 0)
						tmp = 1;
					stop_width = _ix + tmp / ystep + 3;
					for(; _ix <= stop_width ; _ix += _xstep )
						// _ix = LB.x/ ystep;
					{
						int ix = cvRound(_ix*ystep); // it really should be ystep
						//int ix = LB.x;

						if( pass == 0 )
						{
							int result;
							_xstep = 2;

							if( do_canny_pruning )
							{
								int offset;
								int s, sq;

								offset = iy*(sum->step/sizeof(p0[0])) + ix;
								s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
								sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
								if( s < 100 || sq < 20 )
									continue;
							}

							result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy), 0 );
							if( result > 0 )
							{
								if( pass < npass - 1 )
									mask_row[ix] = 1;
								else
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							if( result < 0 )
								_xstep = 1;
						}
						else if( mask_row[ix] )
						{
							int result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy),
								stage_offset );
							if( result > 0 )
							{
								if( pass == npass - 1 )
								{
									CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
									cvSeqPush( seq, &rect );
#else
									cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
								}
							}
							else
								mask_row[ix] = 0;
						}
					}
				}
				stage_offset = cascade->hid_cascade->count;
				cascade->hid_cascade->count = cascade->count;
			}
		}
	}

#ifdef _OPENMP
	// gather the results
	for( i = 0; i < max_threads; i++ )
	{
		CvSeq* s = seq_thread[i];
		int j, total = s->total;
		CvSeqBlock* b = s->first;
		for( j = 0; j < total; j += b->count, b = b->next )
			cvSeqPushMulti( seq, b->data, b->count );
	}
#endif

	if( min_neighbors != 0 )
	{
		// group retrieved rectangles in order to filter out noise 
		int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
		CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
		memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

		// count number of neighbors
		for( i = 0; i < seq->total; i++ )
		{
			CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
			int idx = *(int*)cvGetSeqElem( idx_seq, i );
			assert( (unsigned)idx < (unsigned)ncomp );

			comps[idx].neighbors++;

			comps[idx].rect.x += r1.x;
			comps[idx].rect.y += r1.y;
			comps[idx].rect.width += r1.width;
			comps[idx].rect.height += r1.height;
		}

		// calculate average bounding box
		for( i = 0; i < ncomp; i++ )
		{
			int n = comps[i].neighbors;
			if( n >= min_neighbors )
			{
				CvAvgComp comp;
				comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
				comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
				comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
				comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
				comp.neighbors = comps[i].neighbors;

				cvSeqPush( seq2, &comp );
			}
		}

		// filter out small face rectangles inside large face rectangles
		for( i = 0; i < seq2->total; i++ )
		{
			CvAvgComp r1 = *(CvAvgComp*)cvGetSeqElem( seq2, i );
			int j, flag = 1;

			for( j = 0; j < seq2->total; j++ )
			{
				CvAvgComp r2 = *(CvAvgComp*)cvGetSeqElem( seq2, j );
				int distance = cvRound( r2.rect.width * 0.2 );

				if( i != j &&
					r1.rect.x >= r2.rect.x - distance &&
					r1.rect.y >= r2.rect.y - distance &&
					r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
					r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
					(r2.neighbors > MAX( 3, r1.neighbors ) || r1.neighbors < 3) )
				{
					flag = 0;
					break;
				}
			}

			if( flag )
			{
				cvSeqPush( result_seq, &r1 );
				/* cvSeqPush( result_seq, &r1.rect ); */
			}
		}
	}

	__END__;

#ifdef _OPENMP
	for( i = 0; i < max_threads; i++ )
	{
		if( seq_thread[i] )
			cvReleaseMemStorage( &seq_thread[i]->storage );
	}
#endif

	cvReleaseMemStorage( &temp_storage );
	cvReleaseMat( &sum );
	cvReleaseMat( &sqsum );
	cvReleaseMat( &tilted );
	cvReleaseMat( &temp );
	cvReleaseMat( &sumcanny );
	cvReleaseMat( &norm_img );
	cvReleaseMat( &img_small );
	cvFree( &comps );

	return result_seq;
}


CV_IMPL CvSeq*
cvHaarDetectObjects( const CvArr* _img,
                     CvHaarClassifierCascade* cascade,
                     CvMemStorage* storage, double scale_factor,
                     int min_neighbors, int flags, CvSize min_size )
{
    int split_stage = 2;

    CvMat stub, *img = (CvMat*)_img;
    CvMat *temp = 0, *sum = 0, *tilted = 0, *sqsum = 0, *norm_img = 0, *sumcanny = 0, *img_small = 0;
    CvSeq* seq = 0;
    CvSeq* seq2 = 0;
    CvSeq* idx_seq = 0;
    CvSeq* result_seq = 0;
    CvMemStorage* temp_storage = 0;
    CvAvgComp* comps = 0;
    int i;
    
#ifdef _OPENMP
    CvSeq* seq_thread[CV_MAX_THREADS] = {0};
    int max_threads = 0;
#endif
    
    CV_FUNCNAME( "cvHaarDetectObjects" );

    __BEGIN__;

    double factor;
    int npass = 2, coi;
    int do_canny_pruning = flags & CV_HAAR_DO_CANNY_PRUNING;

    if( !CV_IS_HAAR_CLASSIFIER(cascade) )
        CV_ERROR( !cascade ? CV_StsNullPtr : CV_StsBadArg, "Invalid classifier cascade" );

    if( !storage )
        CV_ERROR( CV_StsNullPtr, "Null storage pointer" );

    CV_CALL( img = cvGetMat( img, &stub, &coi ));
    if( coi )
        CV_ERROR( CV_BadCOI, "COI is not supported" );

    if( CV_MAT_DEPTH(img->type) != CV_8U )
        CV_ERROR( CV_StsUnsupportedFormat, "Only 8-bit images are supported" );

    CV_CALL( temp = cvCreateMat( img->rows, img->cols, CV_8UC1 ));
    CV_CALL( sum = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 ));
    CV_CALL( sqsum = cvCreateMat( img->rows + 1, img->cols + 1, CV_64FC1 ));
    CV_CALL( temp_storage = cvCreateChildMemStorage( storage ));

#ifdef _OPENMP
    max_threads = cvGetNumThreads();
    for( i = 0; i < max_threads; i++ )
    {
        CvMemStorage* temp_storage_thread;
        CV_CALL( temp_storage_thread = cvCreateMemStorage(0));
        CV_CALL( seq_thread[i] = cvCreateSeq( 0, sizeof(CvSeq),
                        sizeof(CvRect), temp_storage_thread ));
    }
#endif

    if( !cascade->hid_cascade )
        CV_CALL( icvCreateHidHaarClassifierCascade(cascade) );

    if( cascade->hid_cascade->has_tilted_features )
        tilted = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );

    seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvRect), temp_storage );
    seq2 = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), temp_storage );
    result_seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvAvgComp), storage );

    if( min_neighbors == 0 )
        seq = result_seq;

    if( CV_MAT_CN(img->type) > 1 )
    {
        cvCvtColor( img, temp, CV_BGR2GRAY );
        img = temp;
    }
    
    if( flags & CV_HAAR_SCALE_IMAGE )
    {
        CvSize win_size0 = cascade->orig_window_size;
        int use_ipp = cascade->hid_cascade->ipp_stages != 0 &&
                    icvApplyHaarClassifier_32s32f_C1R_p != 0;

        if( use_ipp )
            CV_CALL( norm_img = cvCreateMat( img->rows, img->cols, CV_32FC1 ));
        CV_CALL( img_small = cvCreateMat( img->rows + 1, img->cols + 1, CV_8UC1 ));

        for( factor = 1; ; factor *= scale_factor )
        {
            int positive = 0;
            int x, y;
            CvSize win_size = { cvRound(win_size0.width*factor),
                                cvRound(win_size0.height*factor) };
            CvSize sz = { cvRound( img->cols/factor ), cvRound( img->rows/factor ) };
            CvSize sz1 = { sz.width - win_size0.width, sz.height - win_size0.height };
            CvRect rect1 = { icv_object_win_border, icv_object_win_border,
                win_size0.width - icv_object_win_border*2,
                win_size0.height - icv_object_win_border*2 };
            CvMat img1, sum1, sqsum1, norm1, tilted1, mask1;
            CvMat* _tilted = 0;

            if( sz1.width <= 0 || sz1.height <= 0 )
                break;
            if( win_size.width < min_size.width || win_size.height < min_size.height )
                continue;

            img1 = cvMat( sz.height, sz.width, CV_8UC1, img_small->data.ptr );
            sum1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, sum->data.ptr );
            sqsum1 = cvMat( sz.height+1, sz.width+1, CV_64FC1, sqsum->data.ptr );
            if( tilted )
            {
                tilted1 = cvMat( sz.height+1, sz.width+1, CV_32SC1, tilted->data.ptr );
                _tilted = &tilted1;
            }
            norm1 = cvMat( sz1.height, sz1.width, CV_32FC1, norm_img ? norm_img->data.ptr : 0 );
            mask1 = cvMat( sz1.height, sz1.width, CV_8UC1, temp->data.ptr );

            cvResize( img, &img1, CV_INTER_LINEAR );
            cvIntegral( &img1, &sum1, &sqsum1, _tilted );

            if( use_ipp && icvRectStdDev_32s32f_C1R_p( sum1.data.i, sum1.step,
                sqsum1.data.db, sqsum1.step, norm1.data.fl, norm1.step, sz1, rect1 ) < 0 )
                use_ipp = 0;

            if( use_ipp )
            {
                positive = mask1.cols*mask1.rows;
                cvSet( &mask1, cvScalarAll(255) );
                for( i = 0; i < cascade->count; i++ )
                {
                    if( icvApplyHaarClassifier_32s32f_C1R_p(sum1.data.i, sum1.step,
                        norm1.data.fl, norm1.step, mask1.data.ptr, mask1.step,
                        sz1, &positive, cascade->hid_cascade->stage_classifier[i].threshold,
                        cascade->hid_cascade->ipp_stages[i]) < 0 )
                    {
                        use_ipp = 0;
                        break;
                    }
                    if( positive <= 0 )
                        break;
                }
            }
            
            if( !use_ipp )
            {
                cvSetImagesForHaarClassifierCascade( cascade, &sum1, &sqsum1, 0, 1. );
                for( y = 0, positive = 0; y < sz1.height; y++ )
                    for( x = 0; x < sz1.width; x++ )
                    {
                        mask1.data.ptr[mask1.step*y + x] =
                            cvRunHaarClassifierCascade( cascade, cvPoint(x,y), 0 ) > 0;
                        positive += mask1.data.ptr[mask1.step*y + x];
                    }
            }

            if( positive > 0 )
            {
                for( y = 0; y < sz1.height; y++ )
                    for( x = 0; x < sz1.width; x++ )
                        if( mask1.data.ptr[mask1.step*y + x] != 0 )
                        {
                            CvRect obj_rect = { cvRound(y*factor), cvRound(x*factor),
                                                win_size.width, win_size.height };
                            cvSeqPush( seq, &obj_rect );
                        }
            }
        }
    }
    else
    {
		cvIntegral( img, sum, sqsum, tilted );

		if( do_canny_pruning )
        {
            sumcanny = cvCreateMat( img->rows + 1, img->cols + 1, CV_32SC1 );
            cvCanny( img, temp, 0, 50, 3 );
            cvIntegral( temp, sumcanny );
        }
    
        if( (unsigned)split_stage >= (unsigned)cascade->count ||
            cascade->hid_cascade->is_tree )
        {
            split_stage = cascade->count;
            npass = 1;
        }

        for( factor = 1; factor*cascade->orig_window_size.width < img->cols - 10 &&
                         factor*cascade->orig_window_size.height < img->rows - 10;
             factor *= scale_factor )
        {//loop of scan window

            const double ystep = MAX( 2, factor );
            CvSize win_size = { cvRound( cascade->orig_window_size.width * factor ),
                                cvRound( cascade->orig_window_size.height * factor )};
//			CvSize win_size = { img->width ,img->height};
            CvRect equ_rect = { 0, 0, 0, 0 };
            int *p0 = 0, *p1 = 0, *p2 = 0, *p3 = 0;
            int *pq0 = 0, *pq1 = 0, *pq2 = 0, *pq3 = 0;
            int pass, stage_offset = 0;
            int stop_height = cvRound((img->rows - win_size.height) / ystep);

            if( win_size.width < min_size.width || win_size.height < min_size.height )
                continue;

            cvSetImagesForHaarClassifierCascade( cascade, sum, sqsum, tilted, factor );
            cvZero( temp );	

            if( do_canny_pruning )
            {
                equ_rect.x = cvRound(win_size.width*0.15);
                equ_rect.y = cvRound(win_size.height*0.15);
                equ_rect.width = cvRound(win_size.width*0.7);
                equ_rect.height = cvRound(win_size.height*0.7);

                p0 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step) + equ_rect.x;
                p1 = (int*)(sumcanny->data.ptr + equ_rect.y*sumcanny->step)
                            + equ_rect.x + equ_rect.width;
                p2 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step) + equ_rect.x;
                p3 = (int*)(sumcanny->data.ptr + (equ_rect.y + equ_rect.height)*sumcanny->step)
                            + equ_rect.x + equ_rect.width;

                pq0 = (int*)(sum->data.ptr + equ_rect.y*sum->step) + equ_rect.x;
                pq1 = (int*)(sum->data.ptr + equ_rect.y*sum->step)
                            + equ_rect.x + equ_rect.width;
                pq2 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step) + equ_rect.x;
                pq3 = (int*)(sum->data.ptr + (equ_rect.y + equ_rect.height)*sum->step)
                            + equ_rect.x + equ_rect.width;
            }

            cascade->hid_cascade->count = split_stage;

            for( pass = 0; pass < npass; pass++ )
            {
#ifdef _OPENMP
    #pragma omp parallel for num_threads(max_threads), schedule(dynamic)
#endif
                for( int _iy = 0; _iy < stop_height; _iy++ )
                {
                    int iy = cvRound(_iy*ystep);
                    int _ix, _xstep = 1;
                    int stop_width = cvRound((img->cols - win_size.width) / ystep);
                    uchar* mask_row = temp->data.ptr + temp->step * iy;

                    for( _ix = 0; _ix < stop_width; _ix += _xstep )
                    {
                        int ix = cvRound(_ix*ystep); // it really should be ystep
                    
                        if( pass == 0 )
                        {
                            int result;
                            _xstep = 2;

                            if( do_canny_pruning )
                            {
                                int offset;
                                int s, sq;
                        
                                offset = iy*(sum->step/sizeof(p0[0])) + ix;
                                s = p0[offset] - p1[offset] - p2[offset] + p3[offset];
                                sq = pq0[offset] - pq1[offset] - pq2[offset] + pq3[offset];
                                if( s < 100 || sq < 20 )
                                    continue;
                            }

                            result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy), 0 );
                            if( result > 0 )
                            {
                                if( pass < npass - 1 )
                                    mask_row[ix] = 1;
                                else
                                {
                                    CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
                                    cvSeqPush( seq, &rect );
#else
                                    cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
                                }
                            }
                            if( result < 0 )
                                _xstep = 1;
                        }
                        else if( mask_row[ix] )
                        {
                            int result = cvRunHaarClassifierCascade( cascade, cvPoint(ix,iy),
                                                                     stage_offset );
                            if( result > 0 )
                            {
                                if( pass == npass - 1 )
                                {
                                    CvRect rect = cvRect(ix,iy,win_size.width,win_size.height);
#ifndef _OPENMP
                                    cvSeqPush( seq, &rect );
#else
                                    cvSeqPush( seq_thread[omp_get_thread_num()], &rect );
#endif
                                }
                            }
                            else
                                mask_row[ix] = 0;
                        }
                    }
                }
                stage_offset = cascade->hid_cascade->count;
                cascade->hid_cascade->count = cascade->count;
            }
        }
    }

#ifdef _OPENMP
	// gather the results
	for( i = 0; i < max_threads; i++ )
	{
		CvSeq* s = seq_thread[i];
        int j, total = s->total;
        CvSeqBlock* b = s->first;
        for( j = 0; j < total; j += b->count, b = b->next )
            cvSeqPushMulti( seq, b->data, b->count );
	}
#endif

    if( min_neighbors != 0 )
    {
        // group retrieved rectangles in order to filter out noise 
        int ncomp = cvSeqPartition( seq, 0, &idx_seq, is_equal, 0 );
        CV_CALL( comps = (CvAvgComp*)cvAlloc( (ncomp+1)*sizeof(comps[0])));
        memset( comps, 0, (ncomp+1)*sizeof(comps[0]));

        // count number of neighbors
        for( i = 0; i < seq->total; i++ )
        {
            CvRect r1 = *(CvRect*)cvGetSeqElem( seq, i );
            int idx = *(int*)cvGetSeqElem( idx_seq, i );
            assert( (unsigned)idx < (unsigned)ncomp );

            comps[idx].neighbors++;
             
            comps[idx].rect.x += r1.x;
            comps[idx].rect.y += r1.y;
            comps[idx].rect.width += r1.width;
            comps[idx].rect.height += r1.height;
        }

        // calculate average bounding box
        for( i = 0; i < ncomp; i++ )
        {
            int n = comps[i].neighbors;
            if( n >= min_neighbors )
            {
                CvAvgComp comp;
                comp.rect.x = (comps[i].rect.x*2 + n)/(2*n);
                comp.rect.y = (comps[i].rect.y*2 + n)/(2*n);
                comp.rect.width = (comps[i].rect.width*2 + n)/(2*n);
                comp.rect.height = (comps[i].rect.height*2 + n)/(2*n);
                comp.neighbors = comps[i].neighbors;

                cvSeqPush( seq2, &comp );
            }
        }

        // filter out small face rectangles inside large face rectangles
        for( i = 0; i < seq2->total; i++ )
        {
            CvAvgComp r1 = *(CvAvgComp*)cvGetSeqElem( seq2, i );
            int j, flag = 1;

            for( j = 0; j < seq2->total; j++ )
            {
                CvAvgComp r2 = *(CvAvgComp*)cvGetSeqElem( seq2, j );
                int distance = cvRound( r2.rect.width * 0.2 );
            
                if( i != j &&
                    r1.rect.x >= r2.rect.x - distance &&
                    r1.rect.y >= r2.rect.y - distance &&
                    r1.rect.x + r1.rect.width <= r2.rect.x + r2.rect.width + distance &&
                    r1.rect.y + r1.rect.height <= r2.rect.y + r2.rect.height + distance &&
                    (r2.neighbors > MAX( 3, r1.neighbors ) || r1.neighbors < 3) )
                {
                    flag = 0;
                    break;
                }
            }

            if( flag )
            {
                cvSeqPush( result_seq, &r1 );
                /* cvSeqPush( result_seq, &r1.rect ); */
            }
        }
    }

    __END__;

#ifdef _OPENMP
	for( i = 0; i < max_threads; i++ )
	{
		if( seq_thread[i] )
            cvReleaseMemStorage( &seq_thread[i]->storage );
	}
#endif

    cvReleaseMemStorage( &temp_storage );
    cvReleaseMat( &sum );
    cvReleaseMat( &sqsum );
    cvReleaseMat( &tilted );
    cvReleaseMat( &temp );
    cvReleaseMat( &sumcanny );
    cvReleaseMat( &norm_img );
    cvReleaseMat( &img_small );
    cvFree( &comps );

    return result_seq;
}


static CvHaarClassifierCascade*
icvLoadCascadeCART( const char** input_cascade, int n, CvSize orig_window_size )
{
    int i;
    CvHaarClassifierCascade* cascade = icvCreateHaarClassifierCascade(n);
    cascade->orig_window_size = orig_window_size;

    for( i = 0; i < n; i++ )
    {
        int j, count, l;
        float threshold = 0;
        const char* stage = input_cascade[i];
        int dl = 0;

        /* tree links */
        int parent = -1;
        int next = -1;

        sscanf( stage, "%d%n", &count, &dl );
        stage += dl;
        
        assert( count > 0 );
        cascade->stage_classifier[i].count = count;
        cascade->stage_classifier[i].classifier =
            (CvHaarClassifier*)cvAlloc( count*sizeof(cascade->stage_classifier[i].classifier[0]));

        for( j = 0; j < count; j++ )
        {
            CvHaarClassifier* classifier = cascade->stage_classifier[i].classifier + j;
            int k, rects = 0;
            char str[100];
            
            sscanf( stage, "%d%n", &classifier->count, &dl );
            stage += dl;

            classifier->haar_feature = (CvHaarFeature*) cvAlloc( 
                classifier->count * ( sizeof( *classifier->haar_feature ) +
                                      sizeof( *classifier->threshold ) +
                                      sizeof( *classifier->left ) +
                                      sizeof( *classifier->right ) ) +
                (classifier->count + 1) * sizeof( *classifier->alpha ) );
            classifier->threshold = (float*) (classifier->haar_feature+classifier->count);
            classifier->left = (int*) (classifier->threshold + classifier->count);
            classifier->right = (int*) (classifier->left + classifier->count);
            classifier->alpha = (float*) (classifier->right + classifier->count);
            
            for( l = 0; l < classifier->count; l++ )
            {
                sscanf( stage, "%d%n", &rects, &dl );
                stage += dl;

                assert( rects >= 2 && rects <= CV_HAAR_FEATURE_MAX );

                for( k = 0; k < rects; k++ )
                {
                    CvRect r;
                    int band = 0;
                    sscanf( stage, "%d%d%d%d%d%f%n",
                            &r.x, &r.y, &r.width, &r.height, &band,
                            &(classifier->haar_feature[l].rect[k].weight), &dl );
                    stage += dl;
                    classifier->haar_feature[l].rect[k].r = r;
                }
                sscanf( stage, "%s%n", str, &dl );
                stage += dl;
            
                classifier->haar_feature[l].tilted = strncmp( str, "tilted", 6 ) == 0;
            
                for( k = rects; k < CV_HAAR_FEATURE_MAX; k++ )
                {
                    memset( classifier->haar_feature[l].rect + k, 0,
                            sizeof(classifier->haar_feature[l].rect[k]) );
                }
                
                sscanf( stage, "%f%d%d%n", &(classifier->threshold[l]), 
                                       &(classifier->left[l]),
                                       &(classifier->right[l]), &dl );
                stage += dl;
            }
            for( l = 0; l <= classifier->count; l++ )
            {
                sscanf( stage, "%f%n", &(classifier->alpha[l]), &dl );
                stage += dl;
            }
        }
       
        sscanf( stage, "%f%n", &threshold, &dl );
        stage += dl;

        cascade->stage_classifier[i].threshold = threshold;

        /* load tree links */
        if( sscanf( stage, "%d%d%n", &parent, &next, &dl ) != 2 )
        {
            parent = i - 1;
            next = -1;
        }
        stage += dl;

        cascade->stage_classifier[i].parent = parent;
        cascade->stage_classifier[i].next = next;
        cascade->stage_classifier[i].child = -1;

        if( parent != -1 && cascade->stage_classifier[parent].child == -1 )
        {
            cascade->stage_classifier[parent].child = i;
        }
    }

    return cascade;
}

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif

CV_IMPL CvHaarClassifierCascade*
cvLoadHaarClassifierCascade( const char* directory, CvSize orig_window_size )
{
    const char** input_cascade = 0; 
    CvHaarClassifierCascade *cascade = 0;

    CV_FUNCNAME( "cvLoadHaarClassifierCascade" );

    __BEGIN__;

    int i, n;
    const char* slash;
    char name[_MAX_PATH];
    int size = 0;
    char* ptr = 0;

    if( !directory )
        CV_ERROR( CV_StsNullPtr, "Null path is passed" );

    n = (int)strlen(directory)-1;
    slash = directory[n] == '\\' || directory[n] == '/' ? "" : "/";

    /* try to read the classifier from directory */
    for( n = 0; ; n++ )
    {
        sprintf( name, "%s%s%d/AdaBoostCARTHaarClassifier.txt", directory, slash, n );
        FILE* f = fopen( name, "rb" );
        if( !f )
            break;
        fseek( f, 0, SEEK_END );
        size += ftell( f ) + 1;
        fclose(f);
    }

    if( n == 0 && slash[0] )
    {
        CV_CALL( cascade = (CvHaarClassifierCascade*)cvLoad( directory ));
        EXIT;
    }
    else if( n == 0 )
        CV_ERROR( CV_StsBadArg, "Invalid path" );
    
    size += (n+1)*sizeof(char*);
    CV_CALL( input_cascade = (const char**)cvAlloc( size ));
    ptr = (char*)(input_cascade + n + 1);

    for( i = 0; i < n; i++ )
    {
        sprintf( name, "%s/%d/AdaBoostCARTHaarClassifier.txt", directory, i );
        FILE* f = fopen( name, "rb" );
        if( !f )
            CV_ERROR( CV_StsError, "" );
        fseek( f, 0, SEEK_END );
        size = ftell( f );
        fseek( f, 0, SEEK_SET );
        fread( ptr, 1, size, f );
        fclose(f);
        input_cascade[i] = ptr;
        ptr += size;
        *ptr++ = '\0';
    }

    input_cascade[n] = 0;
    cascade = icvLoadCascadeCART( input_cascade, n, orig_window_size );

    __END__;

    if( input_cascade )
        cvFree( &input_cascade );

    if( cvGetErrStatus() < 0 )
        cvReleaseHaarClassifierCascade( &cascade );

    return cascade;
}


CV_IMPL void
cvReleaseHaarClassifierCascade( CvHaarClassifierCascade** _cascade )
{
    if( _cascade && *_cascade )
    {
        int i, j;
        CvHaarClassifierCascade* cascade = *_cascade;

        for( i = 0; i < cascade->count; i++ )
        {
            for( j = 0; j < cascade->stage_classifier[i].count; j++ )
                cvFree( &cascade->stage_classifier[i].classifier[j].haar_feature );
            cvFree( &cascade->stage_classifier[i].classifier );
        }
        icvReleaseHidHaarClassifierCascade( &cascade->hid_cascade );
        cvFree( _cascade );
    }
}


/****************************************************************************************\
*                                  Persistence functions                                 *
\****************************************************************************************/

/* field names */

#define ICV_HAAR_SIZE_NAME            "size"
#define ICV_HAAR_STAGES_NAME          "stages"
#define ICV_HAAR_TREES_NAME             "trees"
#define ICV_HAAR_FEATURE_NAME             "feature"
#define ICV_HAAR_RECTS_NAME                 "rects"
#define ICV_HAAR_TILTED_NAME                "tilted"
#define ICV_HAAR_THRESHOLD_NAME           "threshold"
#define ICV_HAAR_LEFT_NODE_NAME           "left_node"
#define ICV_HAAR_LEFT_VAL_NAME            "left_val"
#define ICV_HAAR_RIGHT_NODE_NAME          "right_node"
#define ICV_HAAR_RIGHT_VAL_NAME           "right_val"
#define ICV_HAAR_STAGE_THRESHOLD_NAME   "stage_threshold"
#define ICV_HAAR_PARENT_NAME            "parent"
#define ICV_HAAR_NEXT_NAME              "next"

static int
icvIsHaarClassifier( const void* struct_ptr )
{
    return CV_IS_HAAR_CLASSIFIER( struct_ptr );
}

static void*
icvReadHaarClassifier( CvFileStorage* fs, CvFileNode* node )
{
    CvHaarClassifierCascade* cascade = NULL;

    CV_FUNCNAME( "cvReadHaarClassifier" );

    __BEGIN__;

    char buf[256];
    CvFileNode* seq_fn = NULL; /* sequence */
    CvFileNode* fn = NULL;
    CvFileNode* stages_fn = NULL;
    CvSeqReader stages_reader;
    int n;
    int i, j, k, l;
    int parent, next;

    CV_CALL( stages_fn = cvGetFileNodeByName( fs, node, ICV_HAAR_STAGES_NAME ) );
    if( !stages_fn || !CV_NODE_IS_SEQ( stages_fn->tag) )
        CV_ERROR( CV_StsError, "Invalid stages node" );

    n = stages_fn->data.seq->total;
    CV_CALL( cascade = icvCreateHaarClassifierCascade(n) );

    /* read size */
    CV_CALL( seq_fn = cvGetFileNodeByName( fs, node, ICV_HAAR_SIZE_NAME ) );
    if( !seq_fn || !CV_NODE_IS_SEQ( seq_fn->tag ) || seq_fn->data.seq->total != 2 )
        CV_ERROR( CV_StsError, "size node is not a valid sequence." );
    CV_CALL( fn = (CvFileNode*) cvGetSeqElem( seq_fn->data.seq, 0 ) );
    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0 )
        CV_ERROR( CV_StsError, "Invalid size node: width must be positive integer" );
    cascade->orig_window_size.width = fn->data.i;
    CV_CALL( fn = (CvFileNode*) cvGetSeqElem( seq_fn->data.seq, 1 ) );
    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0 )
        CV_ERROR( CV_StsError, "Invalid size node: height must be positive integer" );
    cascade->orig_window_size.height = fn->data.i;

    CV_CALL( cvStartReadSeq( stages_fn->data.seq, &stages_reader ) );
    for( i = 0; i < n; ++i )
    {
        CvFileNode* stage_fn;
        CvFileNode* trees_fn;
        CvSeqReader trees_reader;

        stage_fn = (CvFileNode*) stages_reader.ptr;
        if( !CV_NODE_IS_MAP( stage_fn->tag ) )
        {
            sprintf( buf, "Invalid stage %d", i );
            CV_ERROR( CV_StsError, buf );
        }

        CV_CALL( trees_fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_TREES_NAME ) );
        if( !trees_fn || !CV_NODE_IS_SEQ( trees_fn->tag )
            || trees_fn->data.seq->total <= 0 )
        {
            sprintf( buf, "Trees node is not a valid sequence. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }

        CV_CALL( cascade->stage_classifier[i].classifier =
            (CvHaarClassifier*) cvAlloc( trees_fn->data.seq->total
                * sizeof( cascade->stage_classifier[i].classifier[0] ) ) );
        for( j = 0; j < trees_fn->data.seq->total; ++j )
        {
            cascade->stage_classifier[i].classifier[j].haar_feature = NULL;
        }
        cascade->stage_classifier[i].count = trees_fn->data.seq->total;

        CV_CALL( cvStartReadSeq( trees_fn->data.seq, &trees_reader ) );
        for( j = 0; j < trees_fn->data.seq->total; ++j )
        {
            CvFileNode* tree_fn;
            CvSeqReader tree_reader;
            CvHaarClassifier* classifier;
            int last_idx;

            classifier = &cascade->stage_classifier[i].classifier[j];
            tree_fn = (CvFileNode*) trees_reader.ptr;
            if( !CV_NODE_IS_SEQ( tree_fn->tag ) || tree_fn->data.seq->total <= 0 )
            {
                sprintf( buf, "Tree node is not a valid sequence."
                         " (stage %d, tree %d)", i, j );
                CV_ERROR( CV_StsError, buf );
            }

            classifier->count = tree_fn->data.seq->total;
            CV_CALL( classifier->haar_feature = (CvHaarFeature*) cvAlloc( 
                classifier->count * ( sizeof( *classifier->haar_feature ) +
                                      sizeof( *classifier->threshold ) +
                                      sizeof( *classifier->left ) +
                                      sizeof( *classifier->right ) ) +
                (classifier->count + 1) * sizeof( *classifier->alpha ) ) );
            classifier->threshold = (float*) (classifier->haar_feature+classifier->count);
            classifier->left = (int*) (classifier->threshold + classifier->count);
            classifier->right = (int*) (classifier->left + classifier->count);
            classifier->alpha = (float*) (classifier->right + classifier->count);

            CV_CALL( cvStartReadSeq( tree_fn->data.seq, &tree_reader ) );
            for( k = 0, last_idx = 0; k < tree_fn->data.seq->total; ++k )
            {
                CvFileNode* node_fn;
                CvFileNode* feature_fn;
                CvFileNode* rects_fn;
                CvSeqReader rects_reader;

                node_fn = (CvFileNode*) tree_reader.ptr;
                if( !CV_NODE_IS_MAP( node_fn->tag ) )
                {
                    sprintf( buf, "Tree node %d is not a valid map. (stage %d, tree %d)",
                             k, i, j );
                    CV_ERROR( CV_StsError, buf );
                }
                CV_CALL( feature_fn = cvGetFileNodeByName( fs, node_fn,
                    ICV_HAAR_FEATURE_NAME ) );
                if( !feature_fn || !CV_NODE_IS_MAP( feature_fn->tag ) )
                {
                    sprintf( buf, "Feature node is not a valid map. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                CV_CALL( rects_fn = cvGetFileNodeByName( fs, feature_fn,
                    ICV_HAAR_RECTS_NAME ) );
                if( !rects_fn || !CV_NODE_IS_SEQ( rects_fn->tag )
                    || rects_fn->data.seq->total < 1
                    || rects_fn->data.seq->total > CV_HAAR_FEATURE_MAX )
                {
                    sprintf( buf, "Rects node is not a valid sequence. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                CV_CALL( cvStartReadSeq( rects_fn->data.seq, &rects_reader ) );
                for( l = 0; l < rects_fn->data.seq->total; ++l )
                {
                    CvFileNode* rect_fn;
                    CvRect r;

                    rect_fn = (CvFileNode*) rects_reader.ptr;
                    if( !CV_NODE_IS_SEQ( rect_fn->tag ) || rect_fn->data.seq->total != 5 )
                    {
                        sprintf( buf, "Rect %d is not a valid sequence. "
                                 "(stage %d, tree %d, node %d)", l, i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 0 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i < 0 )
                    {
                        sprintf( buf, "x coordinate must be non-negative integer. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.x = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 1 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i < 0 )
                    {
                        sprintf( buf, "y coordinate must be non-negative integer. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.y = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 2 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0
                        || r.x + fn->data.i > cascade->orig_window_size.width )
                    {
                        sprintf( buf, "width must be positive integer and "
                                 "(x + width) must not exceed window width. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.width = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 3 );
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= 0
                        || r.y + fn->data.i > cascade->orig_window_size.height )
                    {
                        sprintf( buf, "height must be positive integer and "
                                 "(y + height) must not exceed window height. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }
                    r.height = fn->data.i;
                    fn = CV_SEQ_ELEM( rect_fn->data.seq, CvFileNode, 4 );
                    if( !CV_NODE_IS_REAL( fn->tag ) )
                    {
                        sprintf( buf, "weight must be real number. "
                                 "(stage %d, tree %d, node %d, rect %d)", i, j, k, l );
                        CV_ERROR( CV_StsError, buf );
                    }

                    classifier->haar_feature[k].rect[l].weight = (float) fn->data.f;
                    classifier->haar_feature[k].rect[l].r = r;

                    CV_NEXT_SEQ_ELEM( sizeof( *rect_fn ), rects_reader );
                } /* for each rect */
                for( l = rects_fn->data.seq->total; l < CV_HAAR_FEATURE_MAX; ++l )
                {
                    classifier->haar_feature[k].rect[l].weight = 0;
                    classifier->haar_feature[k].rect[l].r = cvRect( 0, 0, 0, 0 );
                }

                CV_CALL( fn = cvGetFileNodeByName( fs, feature_fn, ICV_HAAR_TILTED_NAME));
                if( !fn || !CV_NODE_IS_INT( fn->tag ) )
                {
                    sprintf( buf, "tilted must be 0 or 1. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                classifier->haar_feature[k].tilted = ( fn->data.i != 0 );
                CV_CALL( fn = cvGetFileNodeByName( fs, node_fn, ICV_HAAR_THRESHOLD_NAME));
                if( !fn || !CV_NODE_IS_REAL( fn->tag ) )
                {
                    sprintf( buf, "threshold must be real number. "
                             "(stage %d, tree %d, node %d)", i, j, k );
                    CV_ERROR( CV_StsError, buf );
                }
                classifier->threshold[k] = (float) fn->data.f;
                CV_CALL( fn = cvGetFileNodeByName( fs, node_fn, ICV_HAAR_LEFT_NODE_NAME));
                if( fn )
                {
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= k
                        || fn->data.i >= tree_fn->data.seq->total )
                    {
                        sprintf( buf, "left node must be valid node number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* left node */
                    classifier->left[k] = fn->data.i;
                }
                else
                {
                    CV_CALL( fn = cvGetFileNodeByName( fs, node_fn,
                        ICV_HAAR_LEFT_VAL_NAME ) );
                    if( !fn )
                    {
                        sprintf( buf, "left node or left value must be specified. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    if( !CV_NODE_IS_REAL( fn->tag ) )
                    {
                        sprintf( buf, "left value must be real number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* left value */
                    if( last_idx >= classifier->count + 1 )
                    {
                        sprintf( buf, "Tree structure is broken: too many values. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    classifier->left[k] = -last_idx;
                    classifier->alpha[last_idx++] = (float) fn->data.f;
                }
                CV_CALL( fn = cvGetFileNodeByName( fs, node_fn,ICV_HAAR_RIGHT_NODE_NAME));
                if( fn )
                {
                    if( !CV_NODE_IS_INT( fn->tag ) || fn->data.i <= k
                        || fn->data.i >= tree_fn->data.seq->total )
                    {
                        sprintf( buf, "right node must be valid node number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* right node */
                    classifier->right[k] = fn->data.i;
                }
                else
                {
                    CV_CALL( fn = cvGetFileNodeByName( fs, node_fn,
                        ICV_HAAR_RIGHT_VAL_NAME ) );
                    if( !fn )
                    {
                        sprintf( buf, "right node or right value must be specified. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    if( !CV_NODE_IS_REAL( fn->tag ) )
                    {
                        sprintf( buf, "right value must be real number. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    /* right value */
                    if( last_idx >= classifier->count + 1 )
                    {
                        sprintf( buf, "Tree structure is broken: too many values. "
                                 "(stage %d, tree %d, node %d)", i, j, k );
                        CV_ERROR( CV_StsError, buf );
                    }
                    classifier->right[k] = -last_idx;
                    classifier->alpha[last_idx++] = (float) fn->data.f;
                }

                CV_NEXT_SEQ_ELEM( sizeof( *node_fn ), tree_reader );
            } /* for each node */
            if( last_idx != classifier->count + 1 )
            {
                sprintf( buf, "Tree structure is broken: too few values. "
                         "(stage %d, tree %d)", i, j );
                CV_ERROR( CV_StsError, buf );
            }

            CV_NEXT_SEQ_ELEM( sizeof( *tree_fn ), trees_reader );
        } /* for each tree */

        CV_CALL( fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_STAGE_THRESHOLD_NAME));
        if( !fn || !CV_NODE_IS_REAL( fn->tag ) )
        {
            sprintf( buf, "stage threshold must be real number. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }
        cascade->stage_classifier[i].threshold = (float) fn->data.f;

        parent = i - 1;
        next = -1;

        CV_CALL( fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_PARENT_NAME ) );
        if( !fn || !CV_NODE_IS_INT( fn->tag )
            || fn->data.i < -1 || fn->data.i >= cascade->count )
        {
            sprintf( buf, "parent must be integer number. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }
        parent = fn->data.i;
        CV_CALL( fn = cvGetFileNodeByName( fs, stage_fn, ICV_HAAR_NEXT_NAME ) );
        if( !fn || !CV_NODE_IS_INT( fn->tag )
            || fn->data.i < -1 || fn->data.i >= cascade->count )
        {
            sprintf( buf, "next must be integer number. (stage %d)", i );
            CV_ERROR( CV_StsError, buf );
        }
        next = fn->data.i;

        cascade->stage_classifier[i].parent = parent;
        cascade->stage_classifier[i].next = next;
        cascade->stage_classifier[i].child = -1;

        if( parent != -1 && cascade->stage_classifier[parent].child == -1 )
        {
            cascade->stage_classifier[parent].child = i;
        }
        
        CV_NEXT_SEQ_ELEM( sizeof( *stage_fn ), stages_reader );
    } /* for each stage */

    __END__;

    if( cvGetErrStatus() < 0 )
    {
        cvReleaseHaarClassifierCascade( &cascade );
        cascade = NULL;
    }

    return cascade;
}

static void
icvWriteHaarClassifier( CvFileStorage* fs, const char* name, const void* struct_ptr,
                        CvAttrList attributes )
{
    CV_FUNCNAME( "cvWriteHaarClassifier" );

    __BEGIN__;

    int i, j, k, l;
    char buf[256];
    const CvHaarClassifierCascade* cascade = (const CvHaarClassifierCascade*) struct_ptr;

    /* TODO: parameters check */

    CV_CALL( cvStartWriteStruct( fs, name, CV_NODE_MAP, CV_TYPE_NAME_HAAR, attributes ) );
    
    CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_SIZE_NAME, CV_NODE_SEQ | CV_NODE_FLOW ) );
    CV_CALL( cvWriteInt( fs, NULL, cascade->orig_window_size.width ) );
    CV_CALL( cvWriteInt( fs, NULL, cascade->orig_window_size.height ) );
    CV_CALL( cvEndWriteStruct( fs ) ); /* size */
    
    CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_STAGES_NAME, CV_NODE_SEQ ) );    
    for( i = 0; i < cascade->count; ++i )
    {
        CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_MAP ) );
        sprintf( buf, "stage %d", i );
        CV_CALL( cvWriteComment( fs, buf, 1 ) );
        
        CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_TREES_NAME, CV_NODE_SEQ ) );

        for( j = 0; j < cascade->stage_classifier[i].count; ++j )
        {
            CvHaarClassifier* tree = &cascade->stage_classifier[i].classifier[j];

            CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_SEQ ) );
            sprintf( buf, "tree %d", j );
            CV_CALL( cvWriteComment( fs, buf, 1 ) );

            for( k = 0; k < tree->count; ++k )
            {
                CvHaarFeature* feature = &tree->haar_feature[k];

                CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_MAP ) );
                if( k )
                {
                    sprintf( buf, "node %d", k );
                }
                else
                {
                    sprintf( buf, "root node" );
                }
                CV_CALL( cvWriteComment( fs, buf, 1 ) );

                CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_FEATURE_NAME, CV_NODE_MAP ) );
                
                CV_CALL( cvStartWriteStruct( fs, ICV_HAAR_RECTS_NAME, CV_NODE_SEQ ) );
                for( l = 0; l < CV_HAAR_FEATURE_MAX && feature->rect[l].r.width != 0; ++l )
                {
                    CV_CALL( cvStartWriteStruct( fs, NULL, CV_NODE_SEQ | CV_NODE_FLOW ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.x ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.y ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.width ) );
                    CV_CALL( cvWriteInt(  fs, NULL, feature->rect[l].r.height ) );
                    CV_CALL( cvWriteReal( fs, NULL, feature->rect[l].weight ) );
                    CV_CALL( cvEndWriteStruct( fs ) ); /* rect */
                }
                CV_CALL( cvEndWriteStruct( fs ) ); /* rects */
                CV_CALL( cvWriteInt( fs, ICV_HAAR_TILTED_NAME, feature->tilted ) );
                CV_CALL( cvEndWriteStruct( fs ) ); /* feature */
                
                CV_CALL( cvWriteReal( fs, ICV_HAAR_THRESHOLD_NAME, tree->threshold[k]) );

                if( tree->left[k] > 0 )
                {
                    CV_CALL( cvWriteInt( fs, ICV_HAAR_LEFT_NODE_NAME, tree->left[k] ) );
                }
                else
                {
                    CV_CALL( cvWriteReal( fs, ICV_HAAR_LEFT_VAL_NAME,
                        tree->alpha[-tree->left[k]] ) );
                }

                if( tree->right[k] > 0 )
                {
                    CV_CALL( cvWriteInt( fs, ICV_HAAR_RIGHT_NODE_NAME, tree->right[k] ) );
                }
                else
                {
                    CV_CALL( cvWriteReal( fs, ICV_HAAR_RIGHT_VAL_NAME,
                        tree->alpha[-tree->right[k]] ) );
                }

                CV_CALL( cvEndWriteStruct( fs ) ); /* split */
            }

            CV_CALL( cvEndWriteStruct( fs ) ); /* tree */
        }

        CV_CALL( cvEndWriteStruct( fs ) ); /* trees */

        CV_CALL( cvWriteReal( fs, ICV_HAAR_STAGE_THRESHOLD_NAME,
                              cascade->stage_classifier[i].threshold) );

        CV_CALL( cvWriteInt( fs, ICV_HAAR_PARENT_NAME,
                              cascade->stage_classifier[i].parent ) );
        CV_CALL( cvWriteInt( fs, ICV_HAAR_NEXT_NAME,
                              cascade->stage_classifier[i].next ) );

        CV_CALL( cvEndWriteStruct( fs ) ); /* stage */
    } /* for each stage */
    
    CV_CALL( cvEndWriteStruct( fs ) ); /* stages */
    CV_CALL( cvEndWriteStruct( fs ) ); /* root */

    __END__;
}

static void*
icvCloneHaarClassifier( const void* struct_ptr )
{
    CvHaarClassifierCascade* cascade = NULL;

    CV_FUNCNAME( "cvCloneHaarClassifier" );

    __BEGIN__;

    int i, j, k, n;
    const CvHaarClassifierCascade* cascade_src =
        (const CvHaarClassifierCascade*) struct_ptr;

    n = cascade_src->count;
    CV_CALL( cascade = icvCreateHaarClassifierCascade(n) );
    cascade->orig_window_size = cascade_src->orig_window_size;

    for( i = 0; i < n; ++i )
    {
        cascade->stage_classifier[i].parent = cascade_src->stage_classifier[i].parent;
        cascade->stage_classifier[i].next = cascade_src->stage_classifier[i].next;
        cascade->stage_classifier[i].child = cascade_src->stage_classifier[i].child;
        cascade->stage_classifier[i].threshold = cascade_src->stage_classifier[i].threshold;

        cascade->stage_classifier[i].count = 0;
        CV_CALL( cascade->stage_classifier[i].classifier =
            (CvHaarClassifier*) cvAlloc( cascade_src->stage_classifier[i].count
                * sizeof( cascade->stage_classifier[i].classifier[0] ) ) );
        
        cascade->stage_classifier[i].count = cascade_src->stage_classifier[i].count;

        for( j = 0; j < cascade->stage_classifier[i].count; ++j )
        {
            cascade->stage_classifier[i].classifier[j].haar_feature = NULL;
        }

        for( j = 0; j < cascade->stage_classifier[i].count; ++j )
        {
            const CvHaarClassifier* classifier_src = 
                &cascade_src->stage_classifier[i].classifier[j];
            CvHaarClassifier* classifier = 
                &cascade->stage_classifier[i].classifier[j];

            classifier->count = classifier_src->count;
            CV_CALL( classifier->haar_feature = (CvHaarFeature*) cvAlloc( 
                classifier->count * ( sizeof( *classifier->haar_feature ) +
                                      sizeof( *classifier->threshold ) +
                                      sizeof( *classifier->left ) +
                                      sizeof( *classifier->right ) ) +
                (classifier->count + 1) * sizeof( *classifier->alpha ) ) );
            classifier->threshold = (float*) (classifier->haar_feature+classifier->count);
            classifier->left = (int*) (classifier->threshold + classifier->count);
            classifier->right = (int*) (classifier->left + classifier->count);
            classifier->alpha = (float*) (classifier->right + classifier->count);
            for( k = 0; k < classifier->count; ++k )
            {
                classifier->haar_feature[k] = classifier_src->haar_feature[k];
                classifier->threshold[k] = classifier_src->threshold[k];
                classifier->left[k] = classifier_src->left[k];
                classifier->right[k] = classifier_src->right[k];
                classifier->alpha[k] = classifier_src->alpha[k];
            }
            classifier->alpha[classifier->count] = 
                classifier_src->alpha[classifier->count];
        }
    }

    __END__;

    return cascade;
}


/*CvType haar_type( CV_TYPE_NAME_HAAR, icvIsHaarClassifier,
                  (CvReleaseFunc)cvReleaseHaarClassifierCascade,
                  icvReadHaarClassifier, icvWriteHaarClassifier,
                  icvCloneHaarClassifier ); */

/* End of file. */

static void
icvCalcMinEigenVal( const float* cov, int cov_step, float* dst,
                    int dst_step, CvSize size, CvMat* buffer )
{
    int j;
    float* buf = buffer->data.fl;
    cov_step /= sizeof(cov[0]);
    dst_step /= sizeof(dst[0]);
    buffer->rows = 1;

    for( ; size.height--; cov += cov_step, dst += dst_step )
    {
        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3]*0.5;
            double b = cov[j*3+1];
            double c = cov[j*3+2]*0.5;
            
            buf[j + size.width] = (float)(a + c);
            buf[j] = (float)((a - c)*(a - c) + b*b);
        }

        cvPow( buffer, buffer, 0.5 );

        for( j = 0; j < size.width ; j++ )
            dst[j] = (float)(buf[j + size.width] - buf[j]);
    }
}


static void
icvCalcHarris( const float* cov, int cov_step, float* dst,
               int dst_step, CvSize size, CvMat* /*buffer*/, double k )
{
    int j;
    cov_step /= sizeof(cov[0]);
    dst_step /= sizeof(dst[0]);

    for( ; size.height--; cov += cov_step, dst += dst_step )
    {
        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3];
            double b = cov[j*3+1];
            double c = cov[j*3+2];
            dst[j] = (float)(a*c - b*b - k*(a + c)*(a + c));
        }
    }
}


static void
icvCalcEigenValsVecs( const float* cov, int cov_step, float* dst,
                      int dst_step, CvSize size, CvMat* buffer )
{
    static int y0 = 0;
    
    int j;
    float* buf = buffer->data.fl;
    cov_step /= sizeof(cov[0]);
    dst_step /= sizeof(dst[0]);

    for( ; size.height--; cov += cov_step, dst += dst_step )
    {
        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3]*0.5;
            double b = cov[j*3+1];
            double c = cov[j*3+2]*0.5;

            buf[j + size.width] = (float)(a + c);
            buf[j] = (float)((a - c)*(a - c) + b*b);
        }

        buffer->rows = 1;
        cvPow( buffer, buffer, 0.5 );

        for( j = 0; j < size.width; j++ )
        {
            double a = cov[j*3];
            double b = cov[j*3+1];
            double c = cov[j*3+2];
            
            double l1 = buf[j + size.width] + buf[j];
            double l2 = buf[j + size.width] - buf[j];

            double x = b;
            double y = l1 - a;
            double e = fabs(x);

            if( e + fabs(y) < 1e-4 )
            {
                y = b;
                x = l1 - c;
                e = fabs(x);
                if( e + fabs(y) < 1e-4 )
                {
                    e = 1./(e + fabs(y) + FLT_EPSILON);
                    x *= e, y *= e;
                }
            }

            buf[j] = (float)(x*x + y*y + DBL_EPSILON);
            dst[6*j] = (float)l1;
            dst[6*j + 2] = (float)x;
            dst[6*j + 3] = (float)y;

            x = b;
            y = l2 - a;
            e = fabs(x);

            if( e + fabs(y) < 1e-4 )
            {
                y = b;
                x = l2 - c;
                e = fabs(x);
                if( e + fabs(y) < 1e-4 )
                {
                    e = 1./(e + fabs(y) + FLT_EPSILON);
                    x *= e, y *= e;
                }
            }

            buf[j + size.width] = (float)(x*x + y*y + DBL_EPSILON);
            dst[6*j + 1] = (float)l2;
            dst[6*j + 4] = (float)x;
            dst[6*j + 5] = (float)y;
        }

        buffer->rows = 2;
        cvPow( buffer, buffer, -0.5 );

        for( j = 0; j < size.width; j++ )
        {
            double t0 = buf[j]*dst[6*j + 2];
            double t1 = buf[j]*dst[6*j + 3];

            dst[6*j + 2] = (float)t0;
            dst[6*j + 3] = (float)t1;

            t0 = buf[j + size.width]*dst[6*j + 4];
            t1 = buf[j + size.width]*dst[6*j + 5];

            dst[6*j + 4] = (float)t0;
            dst[6*j + 5] = (float)t1;
        }

        y0++;
    }
}


#define ICV_MINEIGENVAL     0
#define ICV_HARRIS          1
#define ICV_EIGENVALSVECS   2

static void
icvCornerEigenValsVecs( const CvMat* src, CvMat* eigenv, int block_size,
                        int aperture_size, int op_type, double k=0. )
{
    CvSepFilter dx_filter, dy_filter;
    CvBoxFilter blur_filter;
    CvMat *tempsrc = 0;
    CvMat *Dx = 0, *Dy = 0, *cov = 0;
    CvMat *sqrt_buf = 0;

    int buf_size = 1 << 12;
    
    CV_FUNCNAME( "icvCornerEigenValsVecs" );

    __BEGIN__;

    int i, j, y, dst_y = 0, max_dy, delta = 0;
    int aperture_size0 = aperture_size;
    int temp_step = 0, d_step;
    uchar* shifted_ptr = 0;
    int depth, d_depth;
    int stage = CV_START;
    CvSobelFixedIPPFunc ipp_sobel_vert = 0, ipp_sobel_horiz = 0;
    CvFilterFixedIPPFunc ipp_scharr_vert = 0, ipp_scharr_horiz = 0;
    CvSize el_size, size, stripe_size;
    int aligned_width;
    CvPoint el_anchor;
    double factorx, factory;
    bool use_ipp = false;

    if( block_size < 3 || !(block_size & 1) )
        CV_ERROR( CV_StsOutOfRange, "averaging window size must be an odd number >= 3" );
        
    if( aperture_size < 3 && aperture_size != CV_SCHARR || !(aperture_size & 1) )
        CV_ERROR( CV_StsOutOfRange,
        "Derivative filter aperture size must be a positive odd number >=3 or CV_SCHARR" );
    
    depth = CV_MAT_DEPTH(src->type);
    d_depth = depth == CV_8U ? CV_16S : CV_32F;

    size = cvGetMatSize(src);
    aligned_width = cvAlign(size.width, 4);

    aperture_size = aperture_size == CV_SCHARR ? 3 : aperture_size;
    el_size = cvSize( aperture_size, aperture_size );
    el_anchor = cvPoint( aperture_size/2, aperture_size/2 );

    if( aperture_size <= 5 && icvFilterSobelVert_8u16s_C1R_p )
    {
        if( depth == CV_8U && aperture_size0 == CV_SCHARR )
        {
            ipp_scharr_vert = icvFilterScharrVert_8u16s_C1R_p;
            ipp_scharr_horiz = icvFilterScharrHoriz_8u16s_C1R_p;
        }
        else if( depth == CV_32F && aperture_size0 == CV_SCHARR )
        {
            ipp_scharr_vert = icvFilterScharrVert_32f_C1R_p;
            ipp_scharr_horiz = icvFilterScharrHoriz_32f_C1R_p;
        }
        else if( depth == CV_8U )
        {
            ipp_sobel_vert = icvFilterSobelVert_8u16s_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_8u16s_C1R_p;
        }
        else if( depth == CV_32F )
        {
            ipp_sobel_vert = icvFilterSobelVert_32f_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_32f_C1R_p;
        }
    }
    
    if( ipp_sobel_vert && ipp_sobel_horiz ||
        ipp_scharr_vert && ipp_scharr_horiz )
    {
        CV_CALL( tempsrc = icvIPPFilterInit( src, buf_size,
            cvSize(el_size.width,el_size.height + block_size)));
        shifted_ptr = tempsrc->data.ptr + el_anchor.y*tempsrc->step +
                      el_anchor.x*CV_ELEM_SIZE(depth);
        temp_step = tempsrc->step ? tempsrc->step : CV_STUB_STEP;
        max_dy = tempsrc->rows - aperture_size + 1;
        use_ipp = true;
    }
    else
    {
        ipp_sobel_vert = ipp_sobel_horiz = 0;
        ipp_scharr_vert = ipp_scharr_horiz = 0;

        CV_CALL( dx_filter.init_deriv( size.width, depth, d_depth, 1, 0, aperture_size0 ));
        CV_CALL( dy_filter.init_deriv( size.width, depth, d_depth, 0, 1, aperture_size0 ));
        max_dy = buf_size / src->cols;
        max_dy = MAX( max_dy, aperture_size + block_size );
    }

    CV_CALL( Dx = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( Dy = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( cov = cvCreateMat( max_dy + block_size + 1, size.width, CV_32FC3 ));
    CV_CALL( sqrt_buf = cvCreateMat( 2, size.width, CV_32F ));
    Dx->cols = Dy->cols = size.width;

    if( !use_ipp )
        max_dy -= aperture_size - 1;
    d_step = Dx->step ? Dx->step : CV_STUB_STEP;

    CV_CALL(blur_filter.init(size.width, CV_32FC3, CV_32FC3, 0, cvSize(block_size,block_size)));
    stripe_size = size;

    factorx = (double)(1 << (aperture_size - 1)) * block_size;
    if( aperture_size0 == CV_SCHARR )
        factorx *= 2;
    if( depth == CV_8U )
        factorx *= 255.;
    factory = factorx = 1./factorx;
    if( ipp_sobel_vert )
        factory = -factory;

    for( y = 0; y < size.height; y += delta )
    {
        if( !use_ipp )
        {
            delta = MIN( size.height - y, max_dy );
            if( y + delta == size.height )
                stage = stage & CV_START ? CV_START + CV_END : CV_END;
            dx_filter.process( src, Dx, cvRect(0,y,-1,delta), cvPoint(0,0), stage );
            stripe_size.height = dy_filter.process( src, Dy, cvRect(0,y,-1,delta),
                                                    cvPoint(0,0), stage );
        }
        else
        {
            delta = icvIPPFilterNextStripe( src, tempsrc, y, el_size, el_anchor );
            stripe_size.height = delta;

            if( ipp_sobel_vert )
            {
                IPPI_CALL( ipp_sobel_vert( shifted_ptr, temp_step,
                        Dx->data.ptr, d_step, stripe_size,
                        aperture_size*10 + aperture_size ));
                IPPI_CALL( ipp_sobel_horiz( shifted_ptr, temp_step,
                        Dy->data.ptr, d_step, stripe_size,
                        aperture_size*10 + aperture_size ));
            }
            else /*if( ipp_scharr_vert )*/
            {
                IPPI_CALL( ipp_scharr_vert( shifted_ptr, temp_step,
                        Dx->data.ptr, d_step, stripe_size ));
                IPPI_CALL( ipp_scharr_horiz( shifted_ptr, temp_step,
                        Dy->data.ptr, d_step, stripe_size ));
            }
        }

        for( i = 0; i < stripe_size.height; i++ )
        {
            float* cov_data = (float*)(cov->data.ptr + i*cov->step);
            if( d_depth == CV_16S )
            {
                const short* dxdata = (const short*)(Dx->data.ptr + i*Dx->step);
                const short* dydata = (const short*)(Dy->data.ptr + i*Dy->step);

                for( j = 0; j < size.width; j++ )
                {
                    double dx = dxdata[j]*factorx;
                    double dy = dydata[j]*factory;

                    cov_data[j*3] = (float)(dx*dx);
                    cov_data[j*3+1] = (float)(dx*dy);
                    cov_data[j*3+2] = (float)(dy*dy);
                }
            }
            else
            {
                const float* dxdata = (const float*)(Dx->data.ptr + i*Dx->step);
                const float* dydata = (const float*)(Dy->data.ptr + i*Dy->step);

                for( j = 0; j < size.width; j++ )
                {
                    double dx = dxdata[j]*factorx;
                    double dy = dydata[j]*factory;

                    cov_data[j*3] = (float)(dx*dx);
                    cov_data[j*3+1] = (float)(dx*dy);
                    cov_data[j*3+2] = (float)(dy*dy);
                }
            }
        }

        if( y + stripe_size.height >= size.height )
            stage = stage & CV_START ? CV_START + CV_END : CV_END;

        stripe_size.height = blur_filter.process(cov,cov,
            cvRect(0,0,-1,stripe_size.height),cvPoint(0,0),stage+CV_ISOLATED_ROI);

        if( op_type == ICV_MINEIGENVAL )
            icvCalcMinEigenVal( cov->data.fl, cov->step,
                (float*)(eigenv->data.ptr + dst_y*eigenv->step), eigenv->step,
                stripe_size, sqrt_buf );
        else if( op_type == ICV_HARRIS )
            icvCalcHarris( cov->data.fl, cov->step,
                (float*)(eigenv->data.ptr + dst_y*eigenv->step), eigenv->step,
                stripe_size, sqrt_buf, k );
        else if( op_type == ICV_EIGENVALSVECS )
            icvCalcEigenValsVecs( cov->data.fl, cov->step,
                (float*)(eigenv->data.ptr + dst_y*eigenv->step), eigenv->step,
                stripe_size, sqrt_buf );

        dst_y += stripe_size.height;
        stage = CV_MIDDLE;
    }

    __END__;

    cvReleaseMat( &Dx );
    cvReleaseMat( &Dy );
    cvReleaseMat( &cov );
    cvReleaseMat( &sqrt_buf );
    cvReleaseMat( &tempsrc );
}


CV_IMPL void
cvCornerMinEigenVal( const void* srcarr, void* eigenvarr,
                     int block_size, int aperture_size )
{
    CV_FUNCNAME( "cvCornerMinEigenVal" );

    __BEGIN__;

    CvMat stub, *src = (CvMat*)srcarr;
    CvMat eigstub, *eigenv = (CvMat*)eigenvarr;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( eigenv = cvGetMat( eigenv, &eigstub ));

    if( CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1 ||
        CV_MAT_TYPE(eigenv->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    if( !CV_ARE_SIZES_EQ( src, eigenv ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    CV_CALL( icvCornerEigenValsVecs( src, eigenv, block_size, aperture_size, ICV_MINEIGENVAL ));

    __END__;
}


CV_IMPL void
cvCornerHarris( const CvArr* srcarr, CvArr* harris_responce,
                int block_size, int aperture_size, double k )
{
    CV_FUNCNAME( "cvCornerHarris" );

    __BEGIN__;

    CvMat stub, *src = (CvMat*)srcarr;
    CvMat eigstub, *eigenv = (CvMat*)harris_responce;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( eigenv = cvGetMat( eigenv, &eigstub ));

    if( CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1 ||
        CV_MAT_TYPE(eigenv->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    if( !CV_ARE_SIZES_EQ( src, eigenv ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    CV_CALL( icvCornerEigenValsVecs( src, eigenv, block_size, aperture_size, ICV_HARRIS, k ));

    __END__;
}


CV_IMPL void
cvCornerEigenValsAndVecs( const void* srcarr, void* eigenvarr,
                          int block_size, int aperture_size )
{
    CV_FUNCNAME( "cvCornerEigenValsAndVecs" );

    __BEGIN__;

    CvMat stub, *src = (CvMat*)srcarr;
    CvMat eigstub, *eigenv = (CvMat*)eigenvarr;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( eigenv = cvGetMat( eigenv, &eigstub ));

    if( CV_MAT_CN(eigenv->type)*eigenv->cols != src->cols*6 ||
        eigenv->rows != src->rows )
        CV_ERROR( CV_StsUnmatchedSizes, "Output array should be 6 times "
            "wider than the input array and they should have the same height");

    if( CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1 ||
        CV_MAT_TYPE(eigenv->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    CV_CALL( icvCornerEigenValsVecs( src, eigenv, block_size, aperture_size, ICV_EIGENVALSVECS ));

    __END__;
}


CV_IMPL void
cvPreCornerDetect( const void* srcarr, void* dstarr, int aperture_size )
{
    CvSepFilter dx_filter, dy_filter, d2x_filter, d2y_filter, dxy_filter;
    CvMat *Dx = 0, *Dy = 0, *D2x = 0, *D2y = 0, *Dxy = 0;
    CvMat *tempsrc = 0;
    
    int buf_size = 1 << 12;

    CV_FUNCNAME( "cvPreCornerDetect" );

    __BEGIN__;

    int i, j, y, dst_y = 0, max_dy, delta = 0;
    int temp_step = 0, d_step;
    uchar* shifted_ptr = 0;
    int depth, d_depth;
    int stage = CV_START;
    CvSobelFixedIPPFunc ipp_sobel_vert = 0, ipp_sobel_horiz = 0,
                        ipp_sobel_vert_second = 0, ipp_sobel_horiz_second = 0,
                        ipp_sobel_cross = 0;
    CvSize el_size, size, stripe_size;
    int aligned_width;
    CvPoint el_anchor;
    double factor;
    CvMat stub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    bool use_ipp = false;

    CV_CALL( src = cvGetMat( srcarr, &stub ));
    CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( CV_MAT_TYPE(src->type) != CV_8UC1 && CV_MAT_TYPE(src->type) != CV_32FC1 ||
        CV_MAT_TYPE(dst->type) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Input must be 8uC1 or 32fC1, output must be 32fC1" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( aperture_size == CV_SCHARR )
        CV_ERROR( CV_StsOutOfRange, "CV_SCHARR is not supported by this function" );

    if( aperture_size < 3 || aperture_size > 7 || !(aperture_size & 1) )
        CV_ERROR( CV_StsOutOfRange,
        "Derivative filter aperture size must be 3, 5 or 7" );
    
    depth = CV_MAT_DEPTH(src->type);
    d_depth = depth == CV_8U ? CV_16S : CV_32F;

    size = cvGetMatSize(src);
    aligned_width = cvAlign(size.width, 4);

    el_size = cvSize( aperture_size, aperture_size );
    el_anchor = cvPoint( aperture_size/2, aperture_size/2 );

    if( aperture_size <= 5 && icvFilterSobelVert_8u16s_C1R_p )
    {
        if( depth == CV_8U )
        {
            ipp_sobel_vert = icvFilterSobelVert_8u16s_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_8u16s_C1R_p;
            ipp_sobel_vert_second = icvFilterSobelVertSecond_8u16s_C1R_p;
            ipp_sobel_horiz_second = icvFilterSobelHorizSecond_8u16s_C1R_p;
            ipp_sobel_cross = icvFilterSobelCross_8u16s_C1R_p;
        }
        else if( depth == CV_32F )
        {
            ipp_sobel_vert = icvFilterSobelVert_32f_C1R_p;
            ipp_sobel_horiz = icvFilterSobelHoriz_32f_C1R_p;
            ipp_sobel_vert_second = icvFilterSobelVertSecond_32f_C1R_p;
            ipp_sobel_horiz_second = icvFilterSobelHorizSecond_32f_C1R_p;
            ipp_sobel_cross = icvFilterSobelCross_32f_C1R_p;
        }
    }
    
    if( ipp_sobel_vert && ipp_sobel_horiz && ipp_sobel_vert_second &&
        ipp_sobel_horiz_second && ipp_sobel_cross )
    {
        CV_CALL( tempsrc = icvIPPFilterInit( src, buf_size, el_size ));
        shifted_ptr = tempsrc->data.ptr + el_anchor.y*tempsrc->step +
                      el_anchor.x*CV_ELEM_SIZE(depth);
        temp_step = tempsrc->step ? tempsrc->step : CV_STUB_STEP;
        max_dy = tempsrc->rows - aperture_size + 1;
        use_ipp = true;
    }
    else
    {
        ipp_sobel_vert = ipp_sobel_horiz = 0;
        ipp_sobel_vert_second = ipp_sobel_horiz_second = ipp_sobel_cross = 0;
        dx_filter.init_deriv( size.width, depth, d_depth, 1, 0, aperture_size );
        dy_filter.init_deriv( size.width, depth, d_depth, 0, 1, aperture_size );
        d2x_filter.init_deriv( size.width, depth, d_depth, 2, 0, aperture_size );
        d2y_filter.init_deriv( size.width, depth, d_depth, 0, 2, aperture_size );
        dxy_filter.init_deriv( size.width, depth, d_depth, 1, 1, aperture_size );
        max_dy = buf_size / src->cols;
        max_dy = MAX( max_dy, aperture_size );
    }

    CV_CALL( Dx = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( Dy = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( D2x = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( D2y = cvCreateMat( max_dy, aligned_width, d_depth ));
    CV_CALL( Dxy = cvCreateMat( max_dy, aligned_width, d_depth ));
    Dx->cols = Dy->cols = D2x->cols = D2y->cols = Dxy->cols = size.width;

    if( !use_ipp )
        max_dy -= aperture_size - 1;
    d_step = Dx->step ? Dx->step : CV_STUB_STEP;

    stripe_size = size;

    factor = 1 << (aperture_size - 1);
    if( depth == CV_8U )
        factor *= 255;
    factor = 1./(factor * factor * factor);

    aperture_size = aperture_size * 10 + aperture_size;

    for( y = 0; y < size.height; y += delta )
    {
        if( !use_ipp )
        {
            delta = MIN( size.height - y, max_dy );
            CvRect roi = cvRect(0,y,size.width,delta);
            CvPoint origin=cvPoint(0,0);

            if( y + delta == size.height )
                stage = stage & CV_START ? CV_START + CV_END : CV_END;
            
            dx_filter.process(src,Dx,roi,origin,stage);
            dy_filter.process(src,Dy,roi,origin,stage);
            d2x_filter.process(src,D2x,roi,origin,stage);
            d2y_filter.process(src,D2y,roi,origin,stage);
            stripe_size.height = dxy_filter.process(src,Dxy,roi,origin,stage);
        }
        else
        {
            delta = icvIPPFilterNextStripe( src, tempsrc, y, el_size, el_anchor );
            stripe_size.height = delta;

            IPPI_CALL( ipp_sobel_vert( shifted_ptr, temp_step,
                Dx->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_horiz( shifted_ptr, temp_step,
                Dy->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_vert_second( shifted_ptr, temp_step,
                D2x->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_horiz_second( shifted_ptr, temp_step,
                D2y->data.ptr, d_step, stripe_size, aperture_size ));
            IPPI_CALL( ipp_sobel_cross( shifted_ptr, temp_step,
                Dxy->data.ptr, d_step, stripe_size, aperture_size ));
        }

        for( i = 0; i < stripe_size.height; i++, dst_y++ )
        {
            float* dstdata = (float*)(dst->data.ptr + dst_y*dst->step);
            
            if( d_depth == CV_16S )
            {
                const short* dxdata = (const short*)(Dx->data.ptr + i*Dx->step);
                const short* dydata = (const short*)(Dy->data.ptr + i*Dy->step);
                const short* d2xdata = (const short*)(D2x->data.ptr + i*D2x->step);
                const short* d2ydata = (const short*)(D2y->data.ptr + i*D2y->step);
                const short* dxydata = (const short*)(Dxy->data.ptr + i*Dxy->step);
                
                for( j = 0; j < stripe_size.width; j++ )
                {
                    double dx = dxdata[j];
                    double dx2 = dx * dx;
                    double dy = dydata[j];
                    double dy2 = dy * dy;

                    dstdata[j] = (float)(factor*(dx2*d2ydata[j] + dy2*d2xdata[j] - 2*dx*dy*dxydata[j]));
                }
            }
            else
            {
                const float* dxdata = (const float*)(Dx->data.ptr + i*Dx->step);
                const float* dydata = (const float*)(Dy->data.ptr + i*Dy->step);
                const float* d2xdata = (const float*)(D2x->data.ptr + i*D2x->step);
                const float* d2ydata = (const float*)(D2y->data.ptr + i*D2y->step);
                const float* dxydata = (const float*)(Dxy->data.ptr + i*Dxy->step);
                
                for( j = 0; j < stripe_size.width; j++ )
                {
                    double dx = dxdata[j];
                    double dy = dydata[j];
                    dstdata[j] = (float)(factor*(dx*dx*d2ydata[j] + dy*dy*d2xdata[j] - 2*dx*dy*dxydata[j]));
                }
            }
        }

        stage = CV_MIDDLE;
    }

    __END__;

    cvReleaseMat( &Dx );
    cvReleaseMat( &Dy );
    cvReleaseMat( &D2x );
    cvReleaseMat( &D2y );
    cvReleaseMat( &Dxy );
    cvReleaseMat( &tempsrc );
}
// /* Creates new histogram */
CvHistogram *
cvCreateHist( int dims, int *sizes, CvHistType type, float** ranges, int uniform )
{
    CvHistogram *hist = 0;

    CV_FUNCNAME( "cvCreateHist" );
    __BEGIN__;

    if( (unsigned)dims > CV_MAX_DIM )
        CV_ERROR( CV_BadOrder, "Number of dimensions is out of range" );

    if( !sizes )
        CV_ERROR( CV_HeaderIsNull, "Null <sizes> pointer" );

    CV_CALL( hist = (CvHistogram *)cvAlloc( sizeof( CvHistogram )));

    hist->type = CV_HIST_MAGIC_VAL;
    hist->thresh2 = 0;
    hist->bins = 0;
    if( type == CV_HIST_ARRAY )
    {
        CV_CALL( hist->bins = cvInitMatNDHeader( &hist->mat, dims, sizes,
                                                 CV_HIST_DEFAULT_TYPE ));
        CV_CALL( cvCreateData( hist->bins ));
    }
    else if( type == CV_HIST_SPARSE )
    {
        CV_CALL( hist->bins = cvCreateSparseMat( dims, sizes, CV_HIST_DEFAULT_TYPE ));
    }
    else
    {
        CV_ERROR( CV_StsBadArg, "Invalid histogram type" );
    }

    if( ranges )
        CV_CALL( cvSetHistBinRanges( hist, ranges, uniform ));

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseHist( &hist );

    return hist;
}

CV_IMPL void
cvReleaseHist( CvHistogram **hist )
{
    CV_FUNCNAME( "cvReleaseHist" );
    
    __BEGIN__;

    if( !hist )
        CV_ERROR( CV_StsNullPtr, "" );

    if( *hist )
    {
        CvHistogram* temp = *hist;

        if( !CV_IS_HIST(temp))
            CV_ERROR( CV_StsBadArg, "Invalid histogram header" );

        *hist = 0;

        if( CV_IS_SPARSE_HIST( temp ))
            cvRelease( &temp->bins );
        else
        {
            cvReleaseData( temp->bins );
            temp->bins = 0;
        }
        
        if( temp->thresh2 )
            cvFree( &temp->thresh2 );

        cvFree( &temp );
    }

    __END__;
}

// // Retrieves histogram global min, max and their positions
CV_IMPL void
cvGetMinMaxHistValue( const CvHistogram* hist,
                      float *value_min, float* value_max,
                      int* idx_min, int* idx_max )
{
    double minVal, maxVal;

    CV_FUNCNAME( "cvGetMinMaxHistValue" );

    __BEGIN__;

    int i, dims, size[CV_MAX_DIM];

    if( !CV_IS_HIST(hist) )
        CV_ERROR( CV_StsBadArg, "Invalid histogram header" );

    dims = cvGetDims( hist->bins, size );

    if( !CV_IS_SPARSE_HIST(hist) )
    {
        CvMat mat;
        CvPoint minPt, maxPt;

        CV_CALL( cvGetMat( hist->bins, &mat, 0, 1 ));
        CV_CALL( cvMinMaxLoc( &mat, &minVal, &maxVal, &minPt, &maxPt ));

        if( dims == 1 )
        {
            if( idx_min )
                *idx_min = minPt.y + minPt.x;
            if( idx_max )
                *idx_max = maxPt.y + maxPt.x;
        }
        else if( dims == 2 )
        {
            if( idx_min )
                idx_min[0] = minPt.y, idx_min[1] = minPt.x;
            if( idx_max )
                idx_max[0] = maxPt.y, idx_max[1] = maxPt.x;
        }
        else if( idx_min || idx_max )
        {
            int imin = minPt.y*mat.cols + minPt.x;
            int imax = maxPt.y*mat.cols + maxPt.x;
            int i;
           
            for( i = dims - 1; i >= 0; i-- )
            {
                if( idx_min )
                {
                    int t = imin / size[i];
                    idx_min[i] = imin - t*size[i];
                    imin = t;
                }

                if( idx_max )
                {
                    int t = imax / size[i];
                    idx_max[i] = imax - t*size[i];
                    imax = t;
                }
            }
        }
    }
    else
    {
        CvSparseMat* mat = (CvSparseMat*)hist->bins;
        CvSparseMatIterator iterator;
        CvSparseNode *node;
        int minv = INT_MAX;
        int maxv = INT_MIN;
        CvSparseNode* minNode = 0;
        CvSparseNode* maxNode = 0;
        const int *_idx_min = 0, *_idx_max = 0;
        Cv32suf m;

        for( node = cvInitSparseMatIterator( mat, &iterator );
             node != 0; node = cvGetNextSparseNode( &iterator ))
        {
            int value = *(int*)CV_NODE_VAL(mat,node);
            value = CV_TOGGLE_FLT(value);
            if( value < minv )
            {
                minv = value;
                minNode = node;
            }

            if( value > maxv )
            {
                maxv = value;
                maxNode = node;
            }
        }

        if( minNode )
        {
            _idx_min = CV_NODE_IDX(mat,minNode);
            _idx_max = CV_NODE_IDX(mat,maxNode);
            m.i = CV_TOGGLE_FLT(minv); minVal = m.f;
            m.i = CV_TOGGLE_FLT(maxv); maxVal = m.f;
        }
        else
        {
            minVal = maxVal = 0;
        }

        for( i = 0; i < dims; i++ )
        {
            if( idx_min )
                idx_min[i] = _idx_min ? _idx_min[i] : -1;
            if( idx_max )
                idx_max[i] = _idx_max ? _idx_max[i] : -1;
        }
    }

    if( value_min )
        *value_min = (float)minVal;

    if( value_max )
        *value_max = (float)maxVal;

    __END__;
}
// 

// 
// // Sets a value range for every histogram bin
CV_IMPL void
cvSetHistBinRanges( CvHistogram* hist, float** ranges, int uniform )
{
    CV_FUNCNAME( "cvSetHistBinRanges" );
    
    __BEGIN__;

    int dims, size[CV_MAX_DIM], total = 0;
    int i, j;
    
    if( !ranges )
        CV_ERROR( CV_StsNullPtr, "NULL ranges pointer" );

    if( !CV_IS_HIST(hist) )
        CV_ERROR( CV_StsBadArg, "Invalid histogram header" );

    CV_CALL( dims = cvGetDims( hist->bins, size ));
    for( i = 0; i < dims; i++ )
        total += size[i]+1;
    
    if( uniform )
    {
        for( i = 0; i < dims; i++ )
        {
            if( !ranges[i] )
                CV_ERROR( CV_StsNullPtr, "One of <ranges> elements is NULL" );
            hist->thresh[i][0] = ranges[i][0];
            hist->thresh[i][1] = ranges[i][1];
        }

        hist->type |= CV_HIST_UNIFORM_FLAG + CV_HIST_RANGES_FLAG;
    }
    else
    {
        float* dim_ranges;

        if( !hist->thresh2 )
        {
            CV_CALL( hist->thresh2 = (float**)cvAlloc(
                        dims*sizeof(hist->thresh2[0])+
                        total*sizeof(hist->thresh2[0][0])));
        }
        dim_ranges = (float*)(hist->thresh2 + dims);

        for( i = 0; i < dims; i++ )
        {
            float val0 = -FLT_MAX;

            if( !ranges[i] )
                CV_ERROR( CV_StsNullPtr, "One of <ranges> elements is NULL" );
            
            for( j = 0; j <= size[i]; j++ )
            {
                float val = ranges[i][j];
                if( val <= val0 )
                    CV_ERROR(CV_StsOutOfRange, "Bin ranges should go in ascenting order");
                val0 = dim_ranges[j] = val;
            }

            hist->thresh2[i] = dim_ranges;
            dim_ranges += size[i] + 1;
        }

        hist->type |= CV_HIST_RANGES_FLAG;
        hist->type &= ~CV_HIST_UNIFORM_FLAG;
    }

    __END__;
}
// 
// 
#define  ICV_HIST_DUMMY_IDX  (INT_MIN/3)

static CvStatus
icvCalcHistLookupTables8u( const CvHistogram* hist, int dims, int* size, int* tab )
{
    const int lo = 0, hi = 256;
    int is_sparse = CV_IS_SPARSE_HIST( hist );
    int have_range = CV_HIST_HAS_RANGES(hist);
    int i, j;
    
    if( !have_range || CV_IS_UNIFORM_HIST(hist))
    {
        for( i = 0; i < dims; i++ )
        {
            double a = have_range ? hist->thresh[i][0] : 0;
            double b = have_range ? hist->thresh[i][1] : 256;
            int sz = size[i];
            double scale = sz/(b - a);
            int step = 1;

            if( !is_sparse )
                step = ((CvMatND*)(hist->bins))->dim[i].step/sizeof(float);

            for( j = lo; j < hi; j++ )
            {
                int idx = cvFloor((j - a)*scale);
                if( (unsigned)idx < (unsigned)sz )
                    idx *= step;
                else
                    idx = ICV_HIST_DUMMY_IDX;

                tab[i*(hi - lo) + j - lo] = idx;
            }
        }
    }
    else
    {
        for( i = 0; i < dims; i++ )
        {
            double limit = hist->thresh2[i][0];
            int idx = -1, write_idx = ICV_HIST_DUMMY_IDX, sz = size[i];
            int step = 1;

            if( !is_sparse )
                step = ((CvMatND*)(hist->bins))->dim[i].step/sizeof(float);

            if( limit > hi )
                limit = hi;
            
            j = lo;
            for(;;)
            {
                for( ; j < limit; j++ )
                    tab[i*(hi - lo) + j - lo] = write_idx;

                if( (unsigned)(++idx) < (unsigned)sz )
                {
                    limit = hist->thresh2[i][idx+1];
                    if( limit > hi )
                        limit = hi;
                    write_idx = idx*step;
                }
                else
                {
                    for( ; j < hi; j++ )
                        tab[i*(hi - lo) + j - lo] = ICV_HIST_DUMMY_IDX;
                    break;
                }
            }
        }
    }

    return CV_OK;
}
// 
// 
// /***************************** C A L C   H I S T O G R A M *************************/
// 
// // Calculates histogram for one or more 8u arrays
static CvStatus CV_STDCALL
    icvCalcHist_8u_C1R( uchar** img, int step, uchar* mask, int maskStep,
                        CvSize size, CvHistogram* hist )
{
    int* tab;
    int is_sparse = CV_IS_SPARSE_HIST(hist);
    int dims, histsize[CV_MAX_DIM];
    int i, x;
    CvStatus status;

    dims = cvGetDims( hist->bins, histsize );

    tab = (int*)cvStackAlloc( dims*256*sizeof(int));
    status = icvCalcHistLookupTables8u( hist, dims, histsize, tab );

    if( status < 0 )
        return status;

    if( !is_sparse )
    {
        int total = 1;
        int* bins = ((CvMatND*)(hist->bins))->data.i;

        for( i = 0; i < dims; i++ )
            total *= histsize[i];

        if( dims <= 3 && total >= -ICV_HIST_DUMMY_IDX )
            return CV_BADSIZE_ERR; // too big histogram

        switch( dims )
        {
        case 1:
            {
            int tab1d[256];
            memset( tab1d, 0, sizeof(tab1d));

            for( ; size.height--; img[0] += step )
            {
                uchar* ptr = img[0];
                if( !mask )
                {
                    for( x = 0; x <= size.width - 4; x += 4 )
                    {
                        int v0 = ptr[x];
                        int v1 = ptr[x+1];

                        tab1d[v0]++;
                        tab1d[v1]++;

                        v0 = ptr[x+2];
                        v1 = ptr[x+3];

                        tab1d[v0]++;
                        tab1d[v1]++;
                    }

                    for( ; x < size.width; x++ )
                        tab1d[ptr[x]]++;
                }
                else
                {
                    for( x = 0; x < size.width; x++ )
                        if( mask[x] )
                            tab1d[ptr[x]]++;
                    mask += maskStep;
                }
            }

            for( i = 0; i < 256; i++ )
            {
                int idx = tab[i];
                if( idx >= 0 )
                    bins[idx] += tab1d[i];
            }
            }
            break;
        case 2:
            for( ; size.height--; img[0] += step, img[1] += step )
            {
                uchar* ptr0 = img[0];
                uchar* ptr1 = img[1];
                if( !mask )
                {
                    for( x = 0; x < size.width; x++ )
                    {
                        int v0 = ptr0[x];
                        int v1 = ptr1[x];
                        int idx = tab[v0] + tab[256+v1];

                        if( idx >= 0 )
                            bins[idx]++;
                    }
                }
                else
                {
                    for( x = 0; x < size.width; x++ )
                    {
                        if( mask[x] )
                        {
                            int v0 = ptr0[x];
                            int v1 = ptr1[x];

                            int idx = tab[v0] + tab[256+v1];

                            if( idx >= 0 )
                                bins[idx]++;
                        }
                    }
                    mask += maskStep;
                }
            }
            break;
        case 3:
            for( ; size.height--; img[0] += step, img[1] += step, img[2] += step )
            {
                uchar* ptr0 = img[0];
                uchar* ptr1 = img[1];
                uchar* ptr2 = img[2];
                if( !mask )
                {
                    for( x = 0; x < size.width; x++ )
                    {
                        int v0 = ptr0[x];
                        int v1 = ptr1[x];
                        int v2 = ptr2[x];
                        int idx = tab[v0] + tab[256+v1] + tab[512+v2];

                        if( idx >= 0 )
                            bins[idx]++;
                    }
                }
                else
                {
                    for( x = 0; x < size.width; x++ )
                    {
                        if( mask[x] )
                        {
                            int v0 = ptr0[x];
                            int v1 = ptr1[x];
                            int v2 = ptr2[x];
                            int idx = tab[v0] + tab[256+v1] + tab[512+v2];

                            if( idx >= 0 )
                                bins[idx]++;
                        }
                    }
                    mask += maskStep;
                }
            }
            break;
        default:
            for( ; size.height--; )
            {
                if( !mask )
                {
                    for( x = 0; x < size.width; x++ )
                    {
                        int* binptr = bins;
                        for( i = 0; i < dims; i++ )
                        {
                            int idx = tab[i*256 + img[i][x]];
                            if( idx < 0 )
                                break;
                            binptr += idx;
                        }
                        if( i == dims )
                            binptr[0]++;
                    }
                }
                else
                {
                    for( x = 0; x < size.width; x++ )
                    {
                        if( mask[x] )
                        {
                            int* binptr = bins;
                            for( i = 0; i < dims; i++ )
                            {
                                int idx = tab[i*256 + img[i][x]];
                                if( idx < 0 )
                                    break;
                                binptr += idx;
                            }
                            if( i == dims )
                                binptr[0]++;
                        }
                    }
                    mask += maskStep;
                }

                for( i = 0; i < dims; i++ )
                    img[i] += step;
            }
        }
    }
    else
    {
        CvSparseMat* mat = (CvSparseMat*)(hist->bins);
        int node_idx[CV_MAX_DIM];

        for( ; size.height--; )
        {
            if( !mask )
            {
                for( x = 0; x < size.width; x++ )
                {
                    for( i = 0; i < dims; i++ )
                    {
                        int idx = tab[i*256 + img[i][x]];
                        if( idx < 0 )
                            break;
                        node_idx[i] = idx;
                    }
                    if( i == dims )
                    {
                        int* bin = (int*)cvPtrND( mat, node_idx, 0, 1 );
                        bin[0]++;
                    }
                }
            }
            else
            {
                for( x = 0; x < size.width; x++ )
                {
                    if( mask[x] )
                    {
                        for( i = 0; i < dims; i++ )
                        {
                            int idx = tab[i*256 + img[i][x]];
                            if( idx < 0 )
                                break;
                            node_idx[i] = idx;
                        }
                        if( i == dims )
                        {
                            int* bin = (int*)cvPtrND( mat, node_idx, 0, 1, 0 );
                            bin[0]++;
                        }
                    }
                }
                mask += maskStep;
            }

            for( i = 0; i < dims; i++ )
                img[i] += step;
        }
    }

    return CV_OK;
}
// 
// 
// // Calculates histogram for one or more 32f arrays
static CvStatus CV_STDCALL
    icvCalcHist_32f_C1R( float** img, int step, uchar* mask, int maskStep,
                         CvSize size, CvHistogram* hist )
{
    int is_sparse = CV_IS_SPARSE_HIST(hist);
    int uniform = CV_IS_UNIFORM_HIST(hist);
    int dims, histsize[CV_MAX_DIM];
    double uni_range[CV_MAX_DIM][2];
    int i, x;

    dims = cvGetDims( hist->bins, histsize );
    step /= sizeof(img[0][0]);

    if( uniform )
    {
        for( i = 0; i < dims; i++ )
        {
            double t = histsize[i]/((double)hist->thresh[i][1] - hist->thresh[i][0]);
            uni_range[i][0] = t;
            uni_range[i][1] = -t*hist->thresh[i][0];
        }
    }

    if( !is_sparse )
    {
        CvMatND* mat = (CvMatND*)(hist->bins);
        int* bins = mat->data.i;

        if( uniform )
        {
            switch( dims )
            {
            case 1:
                {
                double a = uni_range[0][0], b = uni_range[0][1];
                int sz = histsize[0];

                for( ; size.height--; img[0] += step )
                {
                    float* ptr = img[0];

                    if( !mask )
                    {
                        for( x = 0; x <= size.width - 4; x += 4 )
                        {
                            int v0 = cvFloor(ptr[x]*a + b);
                            int v1 = cvFloor(ptr[x+1]*a + b);

                            if( (unsigned)v0 < (unsigned)sz )
                                bins[v0]++;
                            if( (unsigned)v1 < (unsigned)sz )
                                bins[v1]++;

                            v0 = cvFloor(ptr[x+2]*a + b);
                            v1 = cvFloor(ptr[x+3]*a + b);

                            if( (unsigned)v0 < (unsigned)sz )
                                bins[v0]++;
                            if( (unsigned)v1 < (unsigned)sz )
                                bins[v1]++;
                        }

                        for( ; x < size.width; x++ )
                        {
                            int v0 = cvFloor(ptr[x]*a + b);
                            if( (unsigned)v0 < (unsigned)sz )
                                bins[v0]++;
                        }
                    }
                    else
                    {
                        for( x = 0; x < size.width; x++ )
                            if( mask[x] )
                            {
                                int v0 = cvFloor(ptr[x]*a + b);
                                if( (unsigned)v0 < (unsigned)sz )
                                    bins[v0]++;
                            }
                        mask += maskStep;
                    }
                }
                }
                break;
            case 2:
                {
                double  a0 = uni_range[0][0], b0 = uni_range[0][1];
                double  a1 = uni_range[1][0], b1 = uni_range[1][1];
                int sz0 = histsize[0], sz1 = histsize[1];
                int step0 = ((CvMatND*)(hist->bins))->dim[0].step/sizeof(float);

                for( ; size.height--; img[0] += step, img[1] += step )
                {
                    float* ptr0 = img[0];
                    float* ptr1 = img[1];

                    if( !mask )
                    {
                        for( x = 0; x < size.width; x++ )
                        {
                            int v0 = cvFloor( ptr0[x]*a0 + b0 );
                            int v1 = cvFloor( ptr1[x]*a1 + b1 );

                            if( (unsigned)v0 < (unsigned)sz0 &&
                                (unsigned)v1 < (unsigned)sz1 )
                                bins[v0*step0 + v1]++;
                        }
                    }
                    else
                    {
                        for( x = 0; x < size.width; x++ )
                        {
                            if( mask[x] )
                            {
                                int v0 = cvFloor( ptr0[x]*a0 + b0 );
                                int v1 = cvFloor( ptr1[x]*a1 + b1 );

                                if( (unsigned)v0 < (unsigned)sz0 &&
                                    (unsigned)v1 < (unsigned)sz1 )
                                    bins[v0*step0 + v1]++;
                            }
                        }
                        mask += maskStep;
                    }
                }
                }
                break;
            default:
                for( ; size.height--; )
                {
                    if( !mask )
                    {
                        for( x = 0; x < size.width; x++ )
                        {
                            int* binptr = bins;
                            for( i = 0; i < dims; i++ )
                            {
                                int idx = cvFloor((double)img[i][x]*uni_range[i][0]
                                                 + uni_range[i][1]);
                                if( (unsigned)idx >= (unsigned)histsize[i] )
                                    break;
                                binptr += idx*(mat->dim[i].step/sizeof(float));
                            }
                            if( i == dims )
                                binptr[0]++;
                        }
                    }
                    else
                    {
                        for( x = 0; x < size.width; x++ )
                        {
                            if( mask[x] )
                            {
                                int* binptr = bins;
                                for( i = 0; i < dims; i++ )
                                {
                                    int idx = cvFloor((double)img[i][x]*uni_range[i][0]
                                                     + uni_range[i][1]);
                                    if( (unsigned)idx >= (unsigned)histsize[i] )
                                        break;
                                    binptr += idx*(mat->dim[i].step/sizeof(float));
                                }
                                if( i == dims )
                                    binptr[0]++;
                            }
                        }
                        mask += maskStep;
                    }

                    for( i = 0; i < dims; i++ )
                        img[i] += step;
                }
            }
        }
        else
        {
            for( ; size.height--; )
            {
                for( x = 0; x < size.width; x++ )
                {
                    if( !mask || mask[x] )
                    {
                        int* binptr = bins;
                        for( i = 0; i < dims; i++ )
                        {
                            float v = img[i][x];
                            float* thresh = hist->thresh2[i];
                            int idx = -1, sz = histsize[i];

                            while( v >= thresh[idx+1] && ++idx < sz )
                                /* nop */;

                            if( (unsigned)idx >= (unsigned)sz )
                                break;

                            binptr += idx*(mat->dim[i].step/sizeof(float));
                        }
                        if( i == dims )
                            binptr[0]++;
                    }
                }

                for( i = 0; i < dims; i++ )
                    img[i] += step;
                if( mask )
                    mask += maskStep;
            }
        }
    }
    else
    {
        CvSparseMat* mat = (CvSparseMat*)(hist->bins);
        int node_idx[CV_MAX_DIM];

        for( ; size.height--; )
        {
            if( uniform )
            {
                for( x = 0; x < size.width; x++ )
                {
                    if( !mask || mask[x] )
                    {
                        for( i = 0; i < dims; i++ )
                        {
                            int idx = cvFloor(img[i][x]*uni_range[i][0]
                                             + uni_range[i][1]);
                            if( (unsigned)idx >= (unsigned)histsize[i] )
                                break;
                            node_idx[i] = idx;
                        }
                        if( i == dims )
                        {
                            int* bin = (int*)cvPtrND( mat, node_idx, 0, 1, 0 );
                            bin[0]++;
                        }
                    }
                }
            }
            else
            {
                for( x = 0; x < size.width; x++ )
                {
                    if( !mask || mask[x] )
                    {
                        for( i = 0; i < dims; i++ )
                        {
                            float v = img[i][x];
                            float* thresh = hist->thresh2[i];
                            int idx = -1, sz = histsize[i];

                            while( v >= thresh[idx+1] && ++idx < sz )
                                /* nop */;

                            if( (unsigned)idx >= (unsigned)sz )
                                break;

                            node_idx[i] = idx;
                        }
                        if( i == dims )
                        {
                            int* bin = (int*)cvPtrND( mat, node_idx, 0, 1, 0 );
                            bin[0]++;
                        }
                    }
                }
            }

            for( i = 0; i < dims; i++ )
                img[i] += step;

            if( mask )
                mask += maskStep;
        }
    }

    return CV_OK;
}
// 
// 
CV_IMPL void
cvCalcArrHist( CvArr** img, CvHistogram* hist,
               int do_not_clear, const CvArr* mask )
{
    CV_FUNCNAME( "cvCalcHist" );

    __BEGIN__;

    uchar* ptr[CV_MAX_DIM];
    uchar* maskptr = 0;
    int maskstep = 0, step = 0;
    int i, dims;
    int cont_flag = -1;
    CvMat stub0, *mat0 = 0;
    CvMatND dense;
    CvSize size;

    if( !CV_IS_HIST(hist))
        CV_ERROR( CV_StsBadArg, "Bad histogram pointer" );

    if( !img )
        CV_ERROR( CV_StsNullPtr, "Null double array pointer" );

    CV_CALL( dims = cvGetDims( hist->bins ));
    
    for( i = 0; i < dims; i++ )
    {
        CvMat stub, *mat = (CvMat*)img[i];
        CV_CALL( mat = cvGetMat( mat, i == 0 ? &stub0 : &stub, 0, 1 ));

        if( CV_MAT_CN( mat->type ) != 1 )
            CV_ERROR( CV_BadNumChannels, "Only 1-channel arrays are allowed here" );

        if( i == 0 )
        {
            mat0 = mat;
            step = mat0->step;
        }
        else
        {
            if( !CV_ARE_SIZES_EQ( mat0, mat ))
                CV_ERROR( CV_StsUnmatchedSizes, "Not all the planes have equal sizes" );

            if( mat0->step != mat->step )
                CV_ERROR( CV_StsUnmatchedSizes, "Not all the planes have equal steps" );

            if( !CV_ARE_TYPES_EQ( mat0, mat ))
                CV_ERROR( CV_StsUnmatchedFormats, "Not all the planes have equal types" );
        }

        cont_flag &= mat->type;
        ptr[i] = mat->data.ptr;
    }

    if( mask )
    {
        CvMat stub, *mat = (CvMat*)mask;
        CV_CALL( mat = cvGetMat( mat, &stub, 0, 1 ));

        if( !CV_IS_MASK_ARR(mat))
            CV_ERROR( CV_StsBadMask, "Bad mask array" );

        if( !CV_ARE_SIZES_EQ( mat0, mat ))
            CV_ERROR( CV_StsUnmatchedSizes,
                "Mask size does not match to other arrays\' size" );
        maskptr = mat->data.ptr;
        maskstep = mat->step;
        cont_flag &= mat->type;
    }

    size = cvGetMatSize(mat0);
    if( CV_IS_MAT_CONT( cont_flag ))
    {
        size.width *= size.height;
        size.height = 1;
        maskstep = step = CV_STUB_STEP;
    }

    if( !CV_IS_SPARSE_HIST(hist))
    {
        dense = *(CvMatND*)hist->bins;
        dense.type = (dense.type & ~CV_MAT_TYPE_MASK) | CV_32SC1;
    }

    if( !do_not_clear )
    {
        CV_CALL( cvZero( hist->bins ));
    }
    else if( !CV_IS_SPARSE_HIST(hist))
    {
        CV_CALL( cvConvert( (CvMatND*)hist->bins, &dense ));
    }
    else
    {
        CvSparseMat* mat = (CvSparseMat*)(hist->bins);
        CvSparseMatIterator iterator;
        CvSparseNode* node;

        for( node = cvInitSparseMatIterator( mat, &iterator );
             node != 0; node = cvGetNextSparseNode( &iterator ))
        {
            Cv32suf* val = (Cv32suf*)CV_NODE_VAL( mat, node );
            val->i = cvRound( val->f );
        }
    }

    if( CV_MAT_DEPTH(mat0->type) > CV_8S && !CV_HIST_HAS_RANGES(hist))
        CV_ERROR( CV_StsBadArg, "histogram ranges must be set (via cvSetHistBinRanges) "
                                "before calling the function" );

    switch( CV_MAT_DEPTH(mat0->type) )
    {
    case CV_8U:
        IPPI_CALL( icvCalcHist_8u_C1R( ptr, step, maskptr, maskstep, size, hist ));
	    break;
    case CV_32F:
        {
        union { uchar** ptr; float** fl; } v;
        v.ptr = ptr;
	    IPPI_CALL( icvCalcHist_32f_C1R( v.fl, step, maskptr, maskstep, size, hist ));
        }
	    break;
    default:
        CV_ERROR( CV_StsUnsupportedFormat, "Unsupported array type" );
    }

    if( !CV_IS_SPARSE_HIST(hist))
    {
        CV_CALL( cvConvert( &dense, (CvMatND*)hist->bins ));
    }
    else
    {
        CvSparseMat* mat = (CvSparseMat*)(hist->bins);
        CvSparseMatIterator iterator;
        CvSparseNode* node;

        for( node = cvInitSparseMatIterator( mat, &iterator );
             node != 0; node = cvGetNextSparseNode( &iterator ))
        {
            Cv32suf* val = (Cv32suf*)CV_NODE_VAL( mat, node );
            val->f = (float)val->i;
        }
    }
    
    __END__;
}

CV_IMPL void cvEqualizeHist( const CvArr* src, CvArr* dst )
{
    CvHistogram* hist = 0;
    CvMat* lut = 0;
    
    CV_FUNCNAME( "cvEqualizeHist" );

    __BEGIN__;

    int i, hist_sz = 256;
    CvSize img_sz;
    float scale;
    float* h;
    int sum = 0;
    int type;
    
    CV_CALL( type = cvGetElemType( src ));
    if( type != CV_8UC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Only 8uC1 images are supported" );

    CV_CALL( hist = cvCreateHist( 1, &hist_sz, CV_HIST_ARRAY ));
    CV_CALL( lut = cvCreateMat( 1, 256, CV_8UC1 ));
    CV_CALL( cvCalcArrHist( (CvArr**)&src, hist ));
    CV_CALL( img_sz = cvGetSize( src ));
    scale = 255.f/(img_sz.width*img_sz.height);
    h = (float*)cvPtr1D( hist->bins, 0 );

    for( i = 0; i < hist_sz; i++ )
    {
        sum += cvRound(h[i]);
        lut->data.ptr[i] = (uchar)cvRound(sum*scale);
    }

    lut->data.ptr[0] = 0;
    CV_CALL( cvLUT( src, dst, lut ));

    __END__;

    cvReleaseHist(&hist);
    cvReleaseMat(&lut);
}
// /****************************************************************************************\
//SMOOTH                                          
//Box Filter
// \****************************************************************************************/
// 
static void icvSumRow_8u32s( const uchar* src0, int* dst, void* params );
static void icvSumRow_32f64f( const float* src0, double* dst, void* params );
static void icvSumCol_32s8u( const int** src, uchar* dst, int dst_step,
                             int count, void* params );
static void icvSumCol_32s16s( const int** src, short* dst, int dst_step,
                             int count, void* params );
static void icvSumCol_32s32s( const int** src, int* dst, int dst_step,
                             int count, void* params );
static void icvSumCol_64f32f( const double** src, float* dst, int dst_step,
                              int count, void* params );
// 
CvBoxFilter::CvBoxFilter()
{
    min_depth = CV_32S;
    sum = 0;
    sum_count = 0;
    normalized = false;
}

// 
// CvBoxFilter::CvBoxFilter( int _max_width, int _src_type, int _dst_type,
//                           bool _normalized, CvSize _ksize,
//                           CvPoint _anchor, int _border_mode,
//                           CvScalar _border_value )
// {
//     min_depth = CV_32S;
//     sum = 0;
//     sum_count = 0;
//     normalized = false;
//     init( _max_width, _src_type, _dst_type, _normalized,
//           _ksize, _anchor, _border_mode, _border_value );
// }
// 
// 
CvBoxFilter::~CvBoxFilter()
{
    clear();
}
// 
// 
void CvBoxFilter::init( int _max_width, int _src_type, int _dst_type,
                        bool _normalized, CvSize _ksize,
                        CvPoint _anchor, int _border_mode,
                        CvScalar _border_value )
{
    CV_FUNCNAME( "CvBoxFilter::init" );

    __BEGIN__;
    
    sum = 0;
    normalized = _normalized;

    if( normalized && CV_MAT_TYPE(_src_type) != CV_MAT_TYPE(_dst_type) ||
        !normalized && CV_MAT_CN(_src_type) != CV_MAT_CN(_dst_type))
        CV_ERROR( CV_StsUnmatchedFormats,
        "In case of normalized box filter input and output must have the same type.\n"
        "In case of unnormalized box filter the number of input and output channels must be the same" );

    min_depth = CV_MAT_DEPTH(_src_type) == CV_8U ? CV_32S : CV_64F;

    CvBaseImageFilter::init( _max_width, _src_type, _dst_type, 1, _ksize,
                             _anchor, _border_mode, _border_value );
    
    scale = normalized ? 1./(ksize.width*ksize.height) : 1;

    if( CV_MAT_DEPTH(src_type) == CV_8U )
        x_func = (CvRowFilterFunc)icvSumRow_8u32s;
    else if( CV_MAT_DEPTH(src_type) == CV_32F )
        x_func = (CvRowFilterFunc)icvSumRow_32f64f;
    else
        CV_ERROR( CV_StsUnsupportedFormat, "Unknown/unsupported input image format" );

    if( CV_MAT_DEPTH(dst_type) == CV_8U )
    {
        if( !normalized )
            CV_ERROR( CV_StsBadArg, "Only normalized box filter can be used for 8u->8u transformation" );
        y_func = (CvColumnFilterFunc)icvSumCol_32s8u;
    }
    else if( CV_MAT_DEPTH(dst_type) == CV_16S )
    {
        if( normalized || CV_MAT_DEPTH(src_type) != CV_8U )
            CV_ERROR( CV_StsBadArg, "Only 8u->16s unnormalized box filter is supported in case of 16s output" );
        y_func = (CvColumnFilterFunc)icvSumCol_32s16s;
    }
	else if( CV_MAT_DEPTH(dst_type) == CV_32S )
	{
		if( normalized || CV_MAT_DEPTH(src_type) != CV_8U )
			CV_ERROR( CV_StsBadArg, "Only 8u->32s unnormalized box filter is supported in case of 32s output");

		y_func = (CvColumnFilterFunc)icvSumCol_32s32s;
	}
    else if( CV_MAT_DEPTH(dst_type) == CV_32F )
    {
        if( CV_MAT_DEPTH(src_type) != CV_32F )
            CV_ERROR( CV_StsBadArg, "Only 32f->32f box filter (normalized or not) is supported in case of 32f output" );
        y_func = (CvColumnFilterFunc)icvSumCol_64f32f;
    }
	else{
		CV_ERROR( CV_StsBadArg, "Unknown/unsupported destination image format" );
	}

    __END__;
}


void CvBoxFilter::start_process( CvSlice x_range, int width )
{
    CvBaseImageFilter::start_process( x_range, width );
    int i, psz = CV_ELEM_SIZE(work_type);
    uchar* s;
    buf_end -= buf_step;
    buf_max_count--;
    assert( buf_max_count >= max_ky*2 + 1 );
    s = sum = buf_end + cvAlign((width + ksize.width - 1)*CV_ELEM_SIZE(src_type), ALIGN);
    sum_count = 0;

    width *= psz;
    for( i = 0; i < width; i++ )
        s[i] = (uchar)0;
}
// 
// 
static void
icvSumRow_8u32s( const uchar* src, int* dst, void* params )
{
    const CvBoxFilter* state = (const CvBoxFilter*)params;
    int ksize = state->get_kernel_size().width;
    int width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int i, k;

    width = (width - 1)*cn; ksize *= cn;

    for( k = 0; k < cn; k++, src++, dst++ )
    {
        int s = 0;
        for( i = 0; i < ksize; i += cn )
            s += src[i];
        dst[0] = s;
        for( i = 0; i < width; i += cn )
        {
            s += src[i+ksize] - src[i];
            dst[i+cn] = s;
        }
    }
}

// 
static void
icvSumRow_32f64f( const float* src, double* dst, void* params )
{
    const CvBoxFilter* state = (const CvBoxFilter*)params;
    int ksize = state->get_kernel_size().width;
    int width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int i, k;

    width = (width - 1)*cn; ksize *= cn;

    for( k = 0; k < cn; k++, src++, dst++ )
    {
        double s = 0;
        for( i = 0; i < ksize; i += cn )
            s += src[i];
        dst[0] = s;
        for( i = 0; i < width; i += cn )
        {
            s += (double)src[i+ksize] - src[i];
            dst[i+cn] = s;
        }
    }
}
// 
// 
static void
icvSumCol_32s8u( const int** src, uchar* dst,
                 int dst_step, int count, void* params )
{
#define BLUR_SHIFT 24
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    double scale = state->get_scale();
    int iscale = cvFloor(scale*(1 << BLUR_SHIFT));
    int* sum = (int*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const int* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                int t0 = CV_DESCALE(s0*iscale, BLUR_SHIFT), t1 = CV_DESCALE(s1*iscale, BLUR_SHIFT);
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
                dst[i] = (uchar)t0; dst[i+1] = (uchar)t1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i], t0 = CV_DESCALE(s0*iscale, BLUR_SHIFT);
                sum[i] = s0 - sm[i]; dst[i] = (uchar)t0;
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
#undef BLUR_SHIFT
}
// 
// 
static void
icvSumCol_32s16s( const int** src, short* dst,
                  int dst_step, int count, void* params )
{
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int ktotal = ksize*state->get_kernel_size().width;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int* sum = (int*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    dst_step /= sizeof(dst[0]);
    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const int* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else if( ktotal < 128 )
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                dst[i] = (short)s0; dst[i+1] = (short)s1;
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i];
                dst[i] = (short)s0;
                sum[i] = s0 - sm[i];
            }
            dst += dst_step;
        }
        else
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                dst[i] = CV_CAST_16S(s0); dst[i+1] = CV_CAST_16S(s1);
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i];
                dst[i] = CV_CAST_16S(s0);
                sum[i] = s0 - sm[i];
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
}

static void
icvSumCol_32s32s( const int** src, int * dst,
                  int dst_step, int count, void* params )
{
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    int* sum = (int*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    dst_step /= sizeof(dst[0]);
    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const int* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else
        {
            const int* sm = src[-ksize+1];
            for( i = 0; i <= width - 2; i += 2 )
            {
                int s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                dst[i] = s0; dst[i+1] = s1;
                s0 -= sm[i]; s1 -= sm[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
            {
                int s0 = sum[i] + sp[i];
                dst[i] = s0;
                sum[i] = s0 - sm[i];
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
}
// 
// 
static void
icvSumCol_64f32f( const double** src, float* dst,
                  int dst_step, int count, void* params )
{
    CvBoxFilter* state = (CvBoxFilter*)params;
    int ksize = state->get_kernel_size().height;
    int i, width = state->get_width();
    int cn = CV_MAT_CN(state->get_src_type());
    double scale = state->get_scale();
    bool normalized = state->is_normalized();
    double* sum = (double*)state->get_sum_buf();
    int* _sum_count = state->get_sum_count_ptr();
    int sum_count = *_sum_count;

    dst_step /= sizeof(dst[0]);
    width *= cn;
    src += sum_count;
    count += ksize - 1 - sum_count;

    for( ; count--; src++ )
    {
        const double* sp = src[0];
        if( sum_count+1 < ksize )
        {
            for( i = 0; i <= width - 2; i += 2 )
            {
                double s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                sum[i] = s0; sum[i+1] = s1;
            }

            for( ; i < width; i++ )
                sum[i] += sp[i];

            sum_count++;
        }
        else
        {
            const double* sm = src[-ksize+1];
            if( normalized )
                for( i = 0; i <= width - 2; i += 2 )
                {
                    double s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                    double t0 = s0*scale, t1 = s1*scale;
                    s0 -= sm[i]; s1 -= sm[i+1];
                    dst[i] = (float)t0; dst[i+1] = (float)t1;
                    sum[i] = s0; sum[i+1] = s1;
                }
            else
                for( i = 0; i <= width - 2; i += 2 )
                {
                    double s0 = sum[i] + sp[i], s1 = sum[i+1] + sp[i+1];
                    dst[i] = (float)s0; dst[i+1] = (float)s1;
                    s0 -= sm[i]; s1 -= sm[i+1];
                    sum[i] = s0; sum[i+1] = s1;
                }

            for( ; i < width; i++ )
            {
                double s0 = sum[i] + sp[i], t0 = s0*scale;
                sum[i] = s0 - sm[i]; dst[i] = (float)t0;
            }
            dst += dst_step;
        }
    }

    *_sum_count = sum_count;
}
// 
// 
// /****************************************************************************************\
//                                       Median Filter
// \****************************************************************************************/
// 
#define CV_MINMAX_8U(a,b) \
    (t = CV_FAST_CAST_8U((a) - (b)), (b) += t, a -= t)

static CvStatus CV_STDCALL
icvMedianBlur_8u_CnR( uchar* src, int src_step, uchar* dst, int dst_step,
                      CvSize size, int m, int cn )
{
    #define N  16
    int     zone0[4][N];
    int     zone1[4][N*N];
    int     x, y;
    int     n2 = m*m/2;
    int     nx = (m + 1)/2 - 1;
    uchar*  src_max = src + size.height*src_step;
    uchar*  src_right = src + size.width*cn;

    #define UPDATE_ACC01( pix, cn, op ) \
    {                                   \
        int p = (pix);                  \
        zone1[cn][p] op;                \
        zone0[cn][p >> 4] op;           \
    }

    if( size.height < nx || size.width < nx )
        return CV_BADSIZE_ERR;

    if( m == 3 )
    {
        size.width *= cn;

        for( y = 0; y < size.height; y++, dst += dst_step )
        {
            const uchar* src0 = src + src_step*(y-1);
            const uchar* src1 = src0 + src_step;
            const uchar* src2 = src1 + src_step;
            if( y == 0 )
                src0 = src1;
            else if( y == size.height - 1 )
                src2 = src1;

            for( x = 0; x < 2*cn; x++ )
            {
                int x0 = x < cn ? x : size.width - 3*cn + x;
                int x2 = x < cn ? x + cn : size.width - 2*cn + x;
                int x1 = x < cn ? x0 : x2, t;

                int p0 = src0[x0], p1 = src0[x1], p2 = src0[x2];
                int p3 = src1[x0], p4 = src1[x1], p5 = src1[x2];
                int p6 = src2[x0], p7 = src2[x1], p8 = src2[x2];

                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p1);
                CV_MINMAX_8U(p3, p4); CV_MINMAX_8U(p6, p7);
                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p3);
                CV_MINMAX_8U(p5, p8); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p3, p6); CV_MINMAX_8U(p1, p4);
                CV_MINMAX_8U(p2, p5); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p4, p2); CV_MINMAX_8U(p6, p4);
                CV_MINMAX_8U(p4, p2);
                dst[x1] = (uchar)p4;
            }

            for( x = cn; x < size.width - cn; x++ )
            {
                int p0 = src0[x-cn], p1 = src0[x], p2 = src0[x+cn];
                int p3 = src1[x-cn], p4 = src1[x], p5 = src1[x+cn];
                int p6 = src2[x-cn], p7 = src2[x], p8 = src2[x+cn];
                int t;

                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p1);
                CV_MINMAX_8U(p3, p4); CV_MINMAX_8U(p6, p7);
                CV_MINMAX_8U(p1, p2); CV_MINMAX_8U(p4, p5);
                CV_MINMAX_8U(p7, p8); CV_MINMAX_8U(p0, p3);
                CV_MINMAX_8U(p5, p8); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p3, p6); CV_MINMAX_8U(p1, p4);
                CV_MINMAX_8U(p2, p5); CV_MINMAX_8U(p4, p7);
                CV_MINMAX_8U(p4, p2); CV_MINMAX_8U(p6, p4);
                CV_MINMAX_8U(p4, p2);

                dst[x] = (uchar)p4;
            }
        }

        return CV_OK;
    }

    for( x = 0; x < size.width; x++, dst += cn )
    {
        uchar* dst_cur = dst;
        uchar* src_top = src;
        uchar* src_bottom = src;
        int    k, c;
        int    x0 = -1;

        if( x <= m/2 )
            nx++;

        if( nx < m )
            x0 = x < m/2 ? 0 : (nx-1)*cn;

        // init accumulator
        memset( zone0, 0, sizeof(zone0[0])*cn );
        memset( zone1, 0, sizeof(zone1[0])*cn );

        for( y = -m/2; y < m/2; y++ )
        {
            for( c = 0; c < cn; c++ )
            {
                if( x0 >= 0 )
                    UPDATE_ACC01( src_bottom[x0+c], c, += (m - nx) );
                for( k = 0; k < nx*cn; k += cn )
                    UPDATE_ACC01( src_bottom[k+c], c, ++ );
            }

            if( (unsigned)y < (unsigned)(size.height-1) )
                src_bottom += src_step;
        }

        for( y = 0; y < size.height; y++, dst_cur += dst_step )
        {
            if( cn == 1 )
            {
                for( k = 0; k < nx; k++ )
                    UPDATE_ACC01( src_bottom[k], 0, ++ );
            }
            else if( cn == 3 )
            {
                for( k = 0; k < nx*3; k += 3 )
                {
                    UPDATE_ACC01( src_bottom[k], 0, ++ );
                    UPDATE_ACC01( src_bottom[k+1], 1, ++ );
                    UPDATE_ACC01( src_bottom[k+2], 2, ++ );
                }
            }
            else
            {
                assert( cn == 4 );
                for( k = 0; k < nx*4; k += 4 )
                {
                    UPDATE_ACC01( src_bottom[k], 0, ++ );
                    UPDATE_ACC01( src_bottom[k+1], 1, ++ );
                    UPDATE_ACC01( src_bottom[k+2], 2, ++ );
                    UPDATE_ACC01( src_bottom[k+3], 3, ++ );
                }
            }

            if( x0 >= 0 )
            {
                for( c = 0; c < cn; c++ )
                    UPDATE_ACC01( src_bottom[x0+c], c, += (m - nx) );
            }

            if( src_bottom + src_step < src_max )
                src_bottom += src_step;

            // find median
            for( c = 0; c < cn; c++ )
            {
                int s = 0;
                for( k = 0; ; k++ )
                {
                    int t = s + zone0[c][k];
                    if( t > n2 ) break;
                    s = t;
                }

                for( k *= N; ;k++ )
                {
                    s += zone1[c][k];
                    if( s > n2 ) break;
                }

                dst_cur[c] = (uchar)k;
            }

            if( cn == 1 )
            {
                for( k = 0; k < nx; k++ )
                    UPDATE_ACC01( src_top[k], 0, -- );
            }
            else if( cn == 3 )
            {
                for( k = 0; k < nx*3; k += 3 )
                {
                    UPDATE_ACC01( src_top[k], 0, -- );
                    UPDATE_ACC01( src_top[k+1], 1, -- );
                    UPDATE_ACC01( src_top[k+2], 2, -- );
                }
            }
            else
            {
                assert( cn == 4 );
                for( k = 0; k < nx*4; k += 4 )
                {
                    UPDATE_ACC01( src_top[k], 0, -- );
                    UPDATE_ACC01( src_top[k+1], 1, -- );
                    UPDATE_ACC01( src_top[k+2], 2, -- );
                    UPDATE_ACC01( src_top[k+3], 3, -- );
                }
            }

            if( x0 >= 0 )
            {
                for( c = 0; c < cn; c++ )
                    UPDATE_ACC01( src_top[x0+c], c, -= (m - nx) );
            }

            if( y >= m/2 )
                src_top += src_step;
        }

        if( x >= m/2 )
            src += cn;
        if( src + nx*cn > src_right ) nx--;
    }
#undef N
#undef UPDATE_ACC
    return CV_OK;
}
// 
// 
// /****************************************************************************************\
//                                    Bilateral Filtering
// \****************************************************************************************/
// 
static CvStatus CV_STDCALL
icvBilateralFiltering_8u_CnR( uchar* src, int srcStep,
                              uchar* dst, int dstStep,
                              CvSize size, double sigma_color,
                              double sigma_space, int channels )
{
    double i2sigma_color = 1./(sigma_color*sigma_color);
    double i2sigma_space = 1./(sigma_space*sigma_space);

    double mean1[3];
    double mean0;
    double w;
    int deltas[8];
    double weight_tab[8];

    int i, j;

#define INIT_C1\
            color = src[0]; \
            mean0 = 1; mean1[0] = color;

#define COLOR_DISTANCE_C1(c1, c2)\
            (c1 - c2)*(c1 - c2)
#define KERNEL_ELEMENT_C1(k)\
            temp_color = src[deltas[k]];\
            w = weight_tab[k] + COLOR_DISTANCE_C1(color, temp_color)*i2sigma_color;\
            w = 1./(w*w + 1); \
            mean0 += w;\
            mean1[0] += temp_color*w;

#define INIT_C3\
            mean0 = 1; mean1[0] = src[0];mean1[1] = src[1];mean1[2] = src[2];

#define UPDATE_OUTPUT_C1                   \
            dst[i] = (uchar)cvRound(mean1[0]/mean0);

#define COLOR_DISTANCE_C3(c1, c2)\
            ((c1[0] - c2[0])*(c1[0] - c2[0]) + \
             (c1[1] - c2[1])*(c1[1] - c2[1]) + \
             (c1[2] - c2[2])*(c1[2] - c2[2]))
#define KERNEL_ELEMENT_C3(k)\
            temp_color = src + deltas[k];\
            w = weight_tab[k] + COLOR_DISTANCE_C3(src, temp_color)*i2sigma_color;\
            w = 1./(w*w + 1); \
            mean0 += w;\
            mean1[0] += temp_color[0]*w; \
            mean1[1] += temp_color[1]*w; \
            mean1[2] += temp_color[2]*w;

#define UPDATE_OUTPUT_C3\
            mean0 = 1./mean0;\
            dst[i*3 + 0] = (uchar)cvRound(mean1[0]*mean0); \
            dst[i*3 + 1] = (uchar)cvRound(mean1[1]*mean0); \
            dst[i*3 + 2] = (uchar)cvRound(mean1[2]*mean0);

    CV_INIT_3X3_DELTAS( deltas, srcStep, channels );

    weight_tab[0] = weight_tab[2] = weight_tab[4] = weight_tab[6] = i2sigma_space;
    weight_tab[1] = weight_tab[3] = weight_tab[5] = weight_tab[7] = i2sigma_space*2;

    if( channels == 1 )
    {
        int color, temp_color;

        for( i = 0; i < size.width; i++, src++ )
        {
            INIT_C1;
            KERNEL_ELEMENT_C1(6);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C1(5);
                KERNEL_ELEMENT_C1(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C1(7);
                KERNEL_ELEMENT_C1(0);
            }
            UPDATE_OUTPUT_C1;
        }

        src += srcStep - size.width;
        dst += dstStep;

        for( j = 1; j < size.height - 1; j++, dst += dstStep )
        {
            i = 0;
            INIT_C1;
            KERNEL_ELEMENT_C1(0);
            KERNEL_ELEMENT_C1(1);
            KERNEL_ELEMENT_C1(2);
            KERNEL_ELEMENT_C1(6);
            KERNEL_ELEMENT_C1(7);
            UPDATE_OUTPUT_C1;

            for( i = 1, src++; i < size.width - 1; i++, src++ )
            {
                INIT_C1;
                KERNEL_ELEMENT_C1(0);
                KERNEL_ELEMENT_C1(1);
                KERNEL_ELEMENT_C1(2);
                KERNEL_ELEMENT_C1(3);
                KERNEL_ELEMENT_C1(4);
                KERNEL_ELEMENT_C1(5);
                KERNEL_ELEMENT_C1(6);
                KERNEL_ELEMENT_C1(7);
                UPDATE_OUTPUT_C1;
            }

            INIT_C1;
            KERNEL_ELEMENT_C1(2);
            KERNEL_ELEMENT_C1(3);
            KERNEL_ELEMENT_C1(4);
            KERNEL_ELEMENT_C1(5);
            KERNEL_ELEMENT_C1(6);
            UPDATE_OUTPUT_C1;

            src += srcStep + 1 - size.width;
        }

        for( i = 0; i < size.width; i++, src++ )
        {
            INIT_C1;
            KERNEL_ELEMENT_C1(2);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C1(3);
                KERNEL_ELEMENT_C1(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C1(1);
                KERNEL_ELEMENT_C1(0);
            }
            UPDATE_OUTPUT_C1;
        }
    }
    else
    {
        uchar* temp_color;

        if( channels != 3 )
            return CV_UNSUPPORTED_CHANNELS_ERR;

        for( i = 0; i < size.width; i++, src += 3 )
        {
            INIT_C3;
            KERNEL_ELEMENT_C3(6);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C3(5);
                KERNEL_ELEMENT_C3(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C3(7);
                KERNEL_ELEMENT_C3(0);
            }
            UPDATE_OUTPUT_C3;
        }

        src += srcStep - size.width*3;
        dst += dstStep;

        for( j = 1; j < size.height - 1; j++, dst += dstStep )
        {
            i = 0;
            INIT_C3;
            KERNEL_ELEMENT_C3(0);
            KERNEL_ELEMENT_C3(1);
            KERNEL_ELEMENT_C3(2);
            KERNEL_ELEMENT_C3(6);
            KERNEL_ELEMENT_C3(7);
            UPDATE_OUTPUT_C3;

            for( i = 1, src += 3; i < size.width - 1; i++, src += 3 )
            {
                INIT_C3;
                KERNEL_ELEMENT_C3(0);
                KERNEL_ELEMENT_C3(1);
                KERNEL_ELEMENT_C3(2);
                KERNEL_ELEMENT_C3(3);
                KERNEL_ELEMENT_C3(4);
                KERNEL_ELEMENT_C3(5);
                KERNEL_ELEMENT_C3(6);
                KERNEL_ELEMENT_C3(7);
                UPDATE_OUTPUT_C3;
            }

            INIT_C3;
            KERNEL_ELEMENT_C3(2);
            KERNEL_ELEMENT_C3(3);
            KERNEL_ELEMENT_C3(4);
            KERNEL_ELEMENT_C3(5);
            KERNEL_ELEMENT_C3(6);
            UPDATE_OUTPUT_C3;

            src += srcStep + 3 - size.width*3;
        }

        for( i = 0; i < size.width; i++, src += 3 )
        {
            INIT_C3;
            KERNEL_ELEMENT_C3(2);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C3(3);
                KERNEL_ELEMENT_C3(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C3(1);
                KERNEL_ELEMENT_C3(0);
            }
            UPDATE_OUTPUT_C3;
        }
    }

    return CV_OK;
#undef INIT_C1
#undef KERNEL_ELEMENT_C1
#undef UPDATE_OUTPUT_C1
#undef INIT_C3
#undef KERNEL_ELEMENT_C3
#undef UPDATE_OUTPUT_C3
#undef COLOR_DISTANCE_C3
}
// 
// //////////////////////////////// IPP smoothing functions /////////////////////////////////
// 
icvFilterMedian_8u_C1R_t icvFilterMedian_8u_C1R_p = 0;
icvFilterMedian_8u_C3R_t icvFilterMedian_8u_C3R_p = 0;
icvFilterMedian_8u_C4R_t icvFilterMedian_8u_C4R_p = 0;

icvFilterBox_8u_C1R_t icvFilterBox_8u_C1R_p = 0;
icvFilterBox_8u_C3R_t icvFilterBox_8u_C3R_p = 0;
icvFilterBox_8u_C4R_t icvFilterBox_8u_C4R_p = 0;
icvFilterBox_32f_C1R_t icvFilterBox_32f_C1R_p = 0;
icvFilterBox_32f_C3R_t icvFilterBox_32f_C3R_p = 0;
icvFilterBox_32f_C4R_t icvFilterBox_32f_C4R_p = 0;

typedef CvStatus (CV_STDCALL * CvSmoothFixedIPPFunc)
( const void* src, int srcstep, void* dst, int dststep,
  CvSize size, CvSize ksize, CvPoint anchor );
// 
// //////////////////////////////////////////////////////////////////////////////////////////
// 
CV_IMPL void
cvSmooth( const void* srcarr, void* dstarr, int smooth_type,
          int param1, int param2, double param3, double param4 )
{
    CvBoxFilter box_filter;
    CvSepFilter gaussian_filter;

    CvMat* temp = 0;

    CV_FUNCNAME( "cvSmooth" );

    __BEGIN__;

    int coi1 = 0, coi2 = 0;
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvSize size;
    int src_type, dst_type, depth, cn;
    double sigma1 = 0, sigma2 = 0;
    bool have_ipp = icvFilterMedian_8u_C1R_p != 0;

    CV_CALL( src = cvGetMat( src, &srcstub, &coi1 ));
    CV_CALL( dst = cvGetMat( dst, &dststub, &coi2 ));

    if( coi1 != 0 || coi2 != 0 )
        CV_ERROR( CV_BadCOI, "" );

    src_type = CV_MAT_TYPE( src->type );
    dst_type = CV_MAT_TYPE( dst->type );
    depth = CV_MAT_DEPTH(src_type);
    cn = CV_MAT_CN(src_type);
    size = cvGetMatSize(src);

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "" );

    if( smooth_type != CV_BLUR_NO_SCALE && !CV_ARE_TYPES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedFormats,
        "The specified smoothing algorithm requires input and ouput arrays be of the same type" );

    if( smooth_type == CV_BLUR || smooth_type == CV_BLUR_NO_SCALE ||
        smooth_type == CV_GAUSSIAN || smooth_type == CV_MEDIAN )
    {
        // automatic detection of kernel size from sigma
        if( smooth_type == CV_GAUSSIAN )
        {
            sigma1 = param3;
            sigma2 = param4 ? param4 : param3;

            if( param1 == 0 && sigma1 > 0 )
                param1 = cvRound(sigma1*(depth == CV_8U ? 3 : 4)*2 + 1)|1;
            if( param2 == 0 && sigma2 > 0 )
                param2 = cvRound(sigma2*(depth == CV_8U ? 3 : 4)*2 + 1)|1;
        }

        if( param2 == 0 )
            param2 = size.height == 1 ? 1 : param1;
        if( param1 < 1 || (param1 & 1) == 0 || param2 < 1 || (param2 & 1) == 0 )
            CV_ERROR( CV_StsOutOfRange,
                "Both mask width and height must be >=1 and odd" );

        if( param1 == 1 && param2 == 1 )
        {
            cvConvert( src, dst );
            EXIT;
        }
    }

    if( have_ipp && (smooth_type == CV_BLUR || smooth_type == CV_MEDIAN) &&
        size.width >= param1 && size.height >= param2 && param1 > 1 && param2 > 1 )
    {
        CvSmoothFixedIPPFunc ipp_median_box_func = 0;

        if( smooth_type == CV_BLUR )
        {
            ipp_median_box_func =
                src_type == CV_8UC1 ? icvFilterBox_8u_C1R_p :
                src_type == CV_8UC3 ? icvFilterBox_8u_C3R_p :
                src_type == CV_8UC4 ? icvFilterBox_8u_C4R_p :
                src_type == CV_32FC1 ? icvFilterBox_32f_C1R_p :
                src_type == CV_32FC3 ? icvFilterBox_32f_C3R_p :
                src_type == CV_32FC4 ? icvFilterBox_32f_C4R_p : 0;
        }
        else if( smooth_type == CV_MEDIAN )
        {
            ipp_median_box_func =
                src_type == CV_8UC1 ? icvFilterMedian_8u_C1R_p :
                src_type == CV_8UC3 ? icvFilterMedian_8u_C3R_p :
                src_type == CV_8UC4 ? icvFilterMedian_8u_C4R_p : 0;
        }

        if( ipp_median_box_func )
        {
            CvSize el_size = { param1, param2 };
            CvPoint el_anchor = { param1/2, param2/2 };
            int stripe_size = 1 << 14; // the optimal value may depend on CPU cache,
                                       // overhead of the current IPP code etc.
            const uchar* shifted_ptr;
            int y, dy = 0;
            int temp_step, dst_step = dst->step;

            CV_CALL( temp = icvIPPFilterInit( src, stripe_size, el_size ));

            shifted_ptr = temp->data.ptr +
                el_anchor.y*temp->step + el_anchor.x*CV_ELEM_SIZE(src_type);
            temp_step = temp->step ? temp->step : CV_STUB_STEP;

            for( y = 0; y < src->rows; y += dy )
            {
                dy = icvIPPFilterNextStripe( src, temp, y, el_size, el_anchor );
                IPPI_CALL( ipp_median_box_func( shifted_ptr, temp_step,
                    dst->data.ptr + y*dst_step, dst_step, cvSize(src->cols, dy),
                    el_size, el_anchor ));
            }
            EXIT;
        }
    }

    if( smooth_type == CV_BLUR || smooth_type == CV_BLUR_NO_SCALE )
    {
        CV_CALL( box_filter.init( src->cols, src_type, dst_type,
            smooth_type == CV_BLUR, cvSize(param1, param2) ));
        CV_CALL( box_filter.process( src, dst ));
    }
    else if( smooth_type == CV_MEDIAN )
    {
        if( depth != CV_8U || cn != 1 && cn != 3 && cn != 4 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Median filter only supports 8uC1, 8uC3 and 8uC4 images" );

        IPPI_CALL( icvMedianBlur_8u_CnR( src->data.ptr, src->step,
            dst->data.ptr, dst->step, size, param1, cn ));
    }
    else if( smooth_type == CV_GAUSSIAN )
    {
        CvSize ksize = { param1, param2 };
        float* kx = (float*)cvStackAlloc( ksize.width*sizeof(kx[0]) );
        float* ky = (float*)cvStackAlloc( ksize.height*sizeof(ky[0]) );
        CvMat KX = cvMat( 1, ksize.width, CV_32F, kx );
        CvMat KY = cvMat( 1, ksize.height, CV_32F, ky );
        
        CvSepFilter::init_gaussian_kernel( &KX, sigma1 );
        if( ksize.width != ksize.height || fabs(sigma1 - sigma2) > FLT_EPSILON )
            CvSepFilter::init_gaussian_kernel( &KY, sigma2 );
        else
            KY.data.fl = kx;
        
        if( have_ipp && size.width >= param1*3 &&
            size.height >= param2 && param1 > 1 && param2 > 1 )
        {
            int done;
            CV_CALL( done = icvIPPSepFilter( src, dst, &KX, &KY,
                        cvPoint(ksize.width/2,ksize.height/2)));
            if( done )
                EXIT;
        }

        CV_CALL( gaussian_filter.init( src->cols, src_type, dst_type, &KX, &KY ));
        CV_CALL( gaussian_filter.process( src, dst ));
    }
    else if( smooth_type == CV_BILATERAL )
    {
        if( param1 < 0 || param2 < 0 )
            CV_ERROR( CV_StsOutOfRange,
            "Thresholds in bilaral filtering should not bee negative" );
        param1 += param1 == 0;
        param2 += param2 == 0;

        if( depth != CV_8U || cn != 1 && cn != 3 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Bilateral filter only supports 8uC1 and 8uC3 images" );

        IPPI_CALL( icvBilateralFiltering_8u_CnR( src->data.ptr, src->step,
            dst->data.ptr, dst->step, size, param1, param2, cn ));
    }

    __END__;

    cvReleaseMat( &temp );
}
// 
// /* End of file. */
typedef struct CvFFillSegment
{
    ushort y;
    ushort l;
    ushort r;
    ushort prevl;
    ushort prevr;
    short dir;
}
CvFFillSegment;
// 
#define UP 1
#define DOWN -1             
// 
#define ICV_PUSH( Y, L, R, PREV_L, PREV_R, DIR )\
{                                               \
    tail->y = (ushort)(Y);                      \
    tail->l = (ushort)(L);                      \
    tail->r = (ushort)(R);                      \
    tail->prevl = (ushort)(PREV_L);             \
    tail->prevr = (ushort)(PREV_R);             \
    tail->dir = (short)(DIR);                   \
    if( ++tail >= buffer_end )                  \
        tail = buffer;                          \
}


#define ICV_POP( Y, L, R, PREV_L, PREV_R, DIR ) \
{                                               \
    Y = head->y;                                \
    L = head->l;                                \
    R = head->r;                                \
    PREV_L = head->prevl;                       \
    PREV_R = head->prevr;                       \
    DIR = head->dir;                            \
    if( ++head >= buffer_end )                  \
        head = buffer;                          \
}
// 
// 
#define ICV_EQ_C3( p1, p2 ) \
    ((p1)[0] == (p2)[0] && (p1)[1] == (p2)[1] && (p1)[2] == (p2)[2])

#define ICV_SET_C3( p, q ) \
    ((p)[0] = (q)[0], (p)[1] = (q)[1], (p)[2] = (q)[2])
// 
// /****************************************************************************************\
// *              Simple Floodfill (repainting single-color connected component)            *
// \****************************************************************************************/
// 
static CvStatus
icvFloodFill_8u_CnIR( uchar* pImage, int step, CvSize roi, CvPoint seed,
                      uchar* _newVal, CvConnectedComp* region, int flags,
                      CvFFillSegment* buffer, int buffer_size, int cn )
{
    uchar* img = pImage + step * seed.y;
    int i, L, R; 
    int area = 0;
    int val0[] = {0,0,0};
    uchar newVal[] = {0,0,0};
    int XMin, XMax, YMin = seed.y, YMax = seed.y;
    int _8_connectivity = (flags & 255) == 8;
    CvFFillSegment* buffer_end = buffer + buffer_size, *head = buffer, *tail = buffer;

    L = R = XMin = XMax = seed.x;

    if( cn == 1 )
    {
        val0[0] = img[L];
        newVal[0] = _newVal[0];

        img[L] = newVal[0];

        while( ++R < roi.width && img[R] == val0[0] )
            img[R] = newVal[0];

        while( --L >= 0 && img[L] == val0[0] )
            img[L] = newVal[0];
    }
    else
    {
        assert( cn == 3 );
        ICV_SET_C3( val0, img + L*3 );
        ICV_SET_C3( newVal, _newVal );
        
        ICV_SET_C3( img + L*3, newVal );
    
        while( --L >= 0 && ICV_EQ_C3( img + L*3, val0 ))
            ICV_SET_C3( img + L*3, newVal );
    
        while( ++R < roi.width && ICV_EQ_C3( img + R*3, val0 ))
            ICV_SET_C3( img + R*3, newVal );
    }

    XMax = --R;
    XMin = ++L;
    ICV_PUSH( seed.y, L, R, R + 1, R, UP );

    while( head != tail )
    {
        int k, YC, PL, PR, dir;
        ICV_POP( YC, L, R, PL, PR, dir );

        int data[][3] =
        {
            {-dir, L - _8_connectivity, R + _8_connectivity},
            {dir, L - _8_connectivity, PL - 1},
            {dir, PR + 1, R + _8_connectivity}
        };

        if( region )
        {
            area += R - L + 1;

            if( XMax < R ) XMax = R;
            if( XMin > L ) XMin = L;
            if( YMax < YC ) YMax = YC;
            if( YMin > YC ) YMin = YC;
        }

        for( k = 0/*(unsigned)(YC - dir) >= (unsigned)roi.height*/; k < 3; k++ )
        {
            dir = data[k][0];
            img = pImage + (YC + dir) * step;
            int left = data[k][1];
            int right = data[k][2];

            if( (unsigned)(YC + dir) >= (unsigned)roi.height )
                continue;

            if( cn == 1 )
                for( i = left; i <= right; i++ )
                {
                    if( (unsigned)i < (unsigned)roi.width && img[i] == val0[0] )
                    {
                        int j = i;
                        img[i] = newVal[0];
                        while( --j >= 0 && img[j] == val0[0] )
                            img[j] = newVal[0];

                        while( ++i < roi.width && img[i] == val0[0] )
                            img[i] = newVal[0];

                        ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                    }
                }
            else
                for( i = left; i <= right; i++ )
                {
                    if( (unsigned)i < (unsigned)roi.width && ICV_EQ_C3( img + i*3, val0 ))
                    {
                        int j = i;
                        ICV_SET_C3( img + i*3, newVal );
                        while( --j >= 0 && ICV_EQ_C3( img + j*3, val0 ))
                            ICV_SET_C3( img + j*3, newVal );

                        while( ++i < roi.width && ICV_EQ_C3( img + i*3, val0 ))
                            ICV_SET_C3( img + i*3, newVal );

                        ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                    }
                }
        }
    }

    if( region )
    {
        region->area = area;
        region->rect.x = XMin;
        region->rect.y = YMin;
        region->rect.width = XMax - XMin + 1;
        region->rect.height = YMax - YMin + 1;
        region->value = cvScalar(newVal[0], newVal[1], newVal[2], 0);
    }

    return CV_NO_ERR;
}
// 
// 
// /* because all the operations on floats that are done during non-gradient floodfill
//    are just copying and comparison on equality,
//    we can do the whole op on 32-bit integers instead */
static CvStatus
icvFloodFill_32f_CnIR( int* pImage, int step, CvSize roi, CvPoint seed,
                       int* _newVal, CvConnectedComp* region, int flags,
                       CvFFillSegment* buffer, int buffer_size, int cn )
{
    int* img = pImage + (step /= sizeof(pImage[0])) * seed.y;
    int i, L, R; 
    int area = 0;
    int val0[] = {0,0,0};
    int newVal[] = {0,0,0};
    int XMin, XMax, YMin = seed.y, YMax = seed.y;
    int _8_connectivity = (flags & 255) == 8;
    CvFFillSegment* buffer_end = buffer + buffer_size, *head = buffer, *tail = buffer;

    L = R = XMin = XMax = seed.x;

    if( cn == 1 )
    {
        val0[0] = img[L];
        newVal[0] = _newVal[0];

        img[L] = newVal[0];

        while( ++R < roi.width && img[R] == val0[0] )
            img[R] = newVal[0];

        while( --L >= 0 && img[L] == val0[0] )
            img[L] = newVal[0];
    }
    else
    {
        assert( cn == 3 );
        ICV_SET_C3( val0, img + L*3 );
        ICV_SET_C3( newVal, _newVal );
        
        ICV_SET_C3( img + L*3, newVal );
    
        while( --L >= 0 && ICV_EQ_C3( img + L*3, val0 ))
            ICV_SET_C3( img + L*3, newVal );
    
        while( ++R < roi.width && ICV_EQ_C3( img + R*3, val0 ))
            ICV_SET_C3( img + R*3, newVal );
    }

    XMax = --R;
    XMin = ++L;
    ICV_PUSH( seed.y, L, R, R + 1, R, UP );

    while( head != tail )
    {
        int k, YC, PL, PR, dir;
        ICV_POP( YC, L, R, PL, PR, dir );

        int data[][3] =
        {
            {-dir, L - _8_connectivity, R + _8_connectivity},
            {dir, L - _8_connectivity, PL - 1},
            {dir, PR + 1, R + _8_connectivity}
        };

        if( region )
        {
            area += R - L + 1;

            if( XMax < R ) XMax = R;
            if( XMin > L ) XMin = L;
            if( YMax < YC ) YMax = YC;
            if( YMin > YC ) YMin = YC;
        }

        for( k = 0/*(unsigned)(YC - dir) >= (unsigned)roi.height*/; k < 3; k++ )
        {
            dir = data[k][0];
            img = pImage + (YC + dir) * step;
            int left = data[k][1];
            int right = data[k][2];

            if( (unsigned)(YC + dir) >= (unsigned)roi.height )
                continue;

            if( cn == 1 )
                for( i = left; i <= right; i++ )
                {
                    if( (unsigned)i < (unsigned)roi.width && img[i] == val0[0] )
                    {
                        int j = i;
                        img[i] = newVal[0];
                        while( --j >= 0 && img[j] == val0[0] )
                            img[j] = newVal[0];

                        while( ++i < roi.width && img[i] == val0[0] )
                            img[i] = newVal[0];

                        ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                    }
                }
            else
                for( i = left; i <= right; i++ )
                {
                    if( (unsigned)i < (unsigned)roi.width && ICV_EQ_C3( img + i*3, val0 ))
                    {
                        int j = i;
                        ICV_SET_C3( img + i*3, newVal );
                        while( --j >= 0 && ICV_EQ_C3( img + j*3, val0 ))
                            ICV_SET_C3( img + j*3, newVal );

                        while( ++i < roi.width && ICV_EQ_C3( img + i*3, val0 ))
                            ICV_SET_C3( img + i*3, newVal );

                        ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                    }
                }
        }
    }

    if( region )
    {
        Cv32suf v0, v1, v2;
        region->area = area;
        region->rect.x = XMin;
        region->rect.y = YMin;
        region->rect.width = XMax - XMin + 1;
        region->rect.height = YMax - YMin + 1;
        v0.i = newVal[0]; v1.i = newVal[1]; v2.i = newVal[2];
        region->value = cvScalar( v0.f, v1.f, v2.f );
    }

    return CV_NO_ERR;
}
// 
// /****************************************************************************************\
// *                                   Gradient Floodfill                                   *
// \****************************************************************************************/
// 
 #define DIFF_INT_C1(p1,p2) ((unsigned)((p1)[0] - (p2)[0] + d_lw[0]) <= interval[0])
// 
 #define DIFF_INT_C3(p1,p2) ((unsigned)((p1)[0] - (p2)[0] + d_lw[0])<= interval[0] && \
                             (unsigned)((p1)[1] - (p2)[1] + d_lw[1])<= interval[1] && \
                             (unsigned)((p1)[2] - (p2)[2] + d_lw[2])<= interval[2])
// 
 #define DIFF_FLT_C1(p1,p2) (fabs((p1)[0] - (p2)[0] + d_lw[0]) <= interval[0])
// 
 #define DIFF_FLT_C3(p1,p2) (fabs((p1)[0] - (p2)[0] + d_lw[0]) <= interval[0] && \
                             fabs((p1)[1] - (p2)[1] + d_lw[1]) <= interval[1] && \
                             fabs((p1)[2] - (p2)[2] + d_lw[2]) <= interval[2])
// 
static CvStatus
icvFloodFill_Grad_8u_CnIR( uchar* pImage, int step, uchar* pMask, int maskStep,
                           CvSize /*roi*/, CvPoint seed, uchar* _newVal, uchar* _d_lw,
                           uchar* _d_up, CvConnectedComp* region, int flags,
                           CvFFillSegment* buffer, int buffer_size, int cn )
{
    uchar* img = pImage + step*seed.y;
    uchar* mask = (pMask += maskStep + 1) + maskStep*seed.y;
    int i, L, R;
    int area = 0;
    int sum[] = {0,0,0}, val0[] = {0,0,0};
    uchar newVal[] = {0,0,0};
    int d_lw[] = {0,0,0};
    unsigned interval[] = {0,0,0};
    int XMin, XMax, YMin = seed.y, YMax = seed.y;
    int _8_connectivity = (flags & 255) == 8;
    int fixedRange = flags & CV_FLOODFILL_FIXED_RANGE;
    int fillImage = (flags & CV_FLOODFILL_MASK_ONLY) == 0;
    uchar newMaskVal = (uchar)(flags & 0xff00 ? flags >> 8 : 1);
    CvFFillSegment* buffer_end = buffer + buffer_size, *head = buffer, *tail = buffer;

    L = R = seed.x;
    if( mask[L] )
        return CV_OK;

    mask[L] = newMaskVal;

    for( i = 0; i < cn; i++ )
    {
        newVal[i] = _newVal[i];
        d_lw[i] = _d_lw[i];
        interval[i] = (unsigned)(_d_up[i] + _d_lw[i]);
        if( fixedRange )
            val0[i] = img[L*cn+i];
    }

    if( cn == 1 )
    {
        if( fixedRange )
        {
            while( !mask[R + 1] && DIFF_INT_C1( img + (R+1), val0 ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_INT_C1( img + (L-1), val0 ))
                mask[--L] = newMaskVal;
        }
        else
        {
            while( !mask[R + 1] && DIFF_INT_C1( img + (R+1), img + R ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_INT_C1( img + (L-1), img + L ))
                mask[--L] = newMaskVal;
        }
    }
    else
    {
        if( fixedRange )
        {
            while( !mask[R + 1] && DIFF_INT_C3( img + (R+1)*3, val0 ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_INT_C3( img + (L-1)*3, val0 ))
                mask[--L] = newMaskVal;
        }
        else
        {
            while( !mask[R + 1] && DIFF_INT_C3( img + (R+1)*3, img + R*3 ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_INT_C3( img + (L-1)*3, img + L*3 ))
                mask[--L] = newMaskVal;
        }
    }

    XMax = R;
    XMin = L;
    ICV_PUSH( seed.y, L, R, R + 1, R, UP );

    while( head != tail )
    {
        int k, YC, PL, PR, dir, curstep;
        ICV_POP( YC, L, R, PL, PR, dir );

        int data[][3] =
        {
            {-dir, L - _8_connectivity, R + _8_connectivity},
            {dir, L - _8_connectivity, PL - 1},
            {dir, PR + 1, R + _8_connectivity}
        };

        unsigned length = (unsigned)(R-L);

        if( region )
        {
            area += (int)length + 1;

            if( XMax < R ) XMax = R;
            if( XMin > L ) XMin = L;
            if( YMax < YC ) YMax = YC;
            if( YMin > YC ) YMin = YC;
        }

        if( cn == 1 )
        {
            for( k = 0; k < 3; k++ )
            {
                dir = data[k][0];
                curstep = dir * step;
                img = pImage + (YC + dir) * step;
                mask = pMask + (YC + dir) * maskStep;
                int left = data[k][1];
                int right = data[k][2];

                if( fixedRange )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_INT_C1( img + i, val0 ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_INT_C1( img + j, val0 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] && DIFF_INT_C1( img + i, val0 ))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else if( !_8_connectivity )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_INT_C1( img + i, img - curstep + i ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_INT_C1( img + j, img + (j+1) ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   (DIFF_INT_C1( img + i, img + (i-1) ) ||
                                   (DIFF_INT_C1( img + i, img + i - curstep) && i <= R)))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else
                    for( i = left; i <= right; i++ )
                    {
                        int idx, val[1];
                
                        if( !mask[i] &&
                            ((val[0] = img[i],
                            (unsigned)(idx = i-L-1) <= length) &&
                            DIFF_INT_C1( val, img - curstep + (i-1) ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_INT_C1( val, img - curstep + i ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_INT_C1( val, img - curstep + (i+1) )))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_INT_C1( img + j, img + (j+1) ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   ((val[0] = img[i],
                                   DIFF_INT_C1( val, img + (i-1) )) ||
                                   ((unsigned)(idx = i-L-1) <= length &&
                                   DIFF_INT_C1( val, img - curstep + (i-1) )) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_INT_C1( val, img - curstep + i ) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_INT_C1( val, img - curstep + (i+1) )))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
            }

            img = pImage + YC * step;
            if( fillImage )
                for( i = L; i <= R; i++ )
                    img[i] = newVal[0];
            else if( region )
                for( i = L; i <= R; i++ )
                    sum[0] += img[i];
        }
        else
        {
            for( k = 0; k < 3; k++ )
            {
                dir = data[k][0];
                curstep = dir * step;
                img = pImage + (YC + dir) * step;
                mask = pMask + (YC + dir) * maskStep;
                int left = data[k][1];
                int right = data[k][2];

                if( fixedRange )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_INT_C3( img + i*3, val0 ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_INT_C3( img + j*3, val0 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] && DIFF_INT_C3( img + i*3, val0 ))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else if( !_8_connectivity )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_INT_C3( img + i*3, img - curstep + i*3 ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_INT_C3( img + j*3, img + (j+1)*3 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   (DIFF_INT_C3( img + i*3, img + (i-1)*3 ) ||
                                   (DIFF_INT_C3( img + i*3, img + i*3 - curstep) && i <= R)))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else
                    for( i = left; i <= right; i++ )
                    {
                        int idx, val[3];
                
                        if( !mask[i] &&
                            ((ICV_SET_C3( val, img+i*3 ),
                            (unsigned)(idx = i-L-1) <= length) &&
                            DIFF_INT_C3( val, img - curstep + (i-1)*3 ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_INT_C3( val, img - curstep + i*3 ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_INT_C3( val, img - curstep + (i+1)*3 )))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_INT_C3( img + j*3, img + (j+1)*3 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   ((ICV_SET_C3( val, img + i*3 ),
                                   DIFF_INT_C3( val, img + (i-1)*3 )) ||
                                   ((unsigned)(idx = i-L-1) <= length &&
                                   DIFF_INT_C3( val, img - curstep + (i-1)*3 )) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_INT_C3( val, img - curstep + i*3 ) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_INT_C3( val, img - curstep + (i+1)*3 )))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
            }

            img = pImage + YC * step;
            if( fillImage )
                for( i = L; i <= R; i++ )
                    ICV_SET_C3( img + i*3, newVal );
            else if( region )
                for( i = L; i <= R; i++ )
                {
                    sum[0] += img[i*3];
                    sum[1] += img[i*3+1];
                    sum[2] += img[i*3+2];
                }
        }
    }
    
    if( region )
    {
        region->area = area;
        region->rect.x = XMin;
        region->rect.y = YMin;
        region->rect.width = XMax - XMin + 1;
        region->rect.height = YMax - YMin + 1;
    
        if( fillImage )
            region->value = cvScalar(newVal[0], newVal[1], newVal[2]);
        else
        {
            double iarea = area ? 1./area : 0;
            region->value = cvScalar(sum[0]*iarea, sum[1]*iarea, sum[2]*iarea);
        }
    }

    return CV_NO_ERR;
}
// 
// 
static CvStatus
icvFloodFill_Grad_32f_CnIR( float* pImage, int step, uchar* pMask, int maskStep,
                           CvSize /*roi*/, CvPoint seed, float* _newVal, float* _d_lw,
                           float* _d_up, CvConnectedComp* region, int flags,
                           CvFFillSegment* buffer, int buffer_size, int cn )
{
    float* img = pImage + (step /= sizeof(float))*seed.y;
    uchar* mask = (pMask += maskStep + 1) + maskStep*seed.y;
    int i, L, R;
    int area = 0;
    double sum[] = {0,0,0}, val0[] = {0,0,0};
    float newVal[] = {0,0,0};
    float d_lw[] = {0,0,0};
    float interval[] = {0,0,0};
    int XMin, XMax, YMin = seed.y, YMax = seed.y;
    int _8_connectivity = (flags & 255) == 8;
    int fixedRange = flags & CV_FLOODFILL_FIXED_RANGE;
    int fillImage = (flags & CV_FLOODFILL_MASK_ONLY) == 0;
    uchar newMaskVal = (uchar)(flags & 0xff00 ? flags >> 8 : 1);
    CvFFillSegment* buffer_end = buffer + buffer_size, *head = buffer, *tail = buffer;

    L = R = seed.x;
    if( mask[L] )
        return CV_OK;

    mask[L] = newMaskVal;

    for( i = 0; i < cn; i++ )
    {
        newVal[i] = _newVal[i];
        d_lw[i] = 0.5f*(_d_lw[i] - _d_up[i]);
        interval[i] = 0.5f*(_d_lw[i] + _d_up[i]);
        if( fixedRange )
            val0[i] = img[L*cn+i];
    }

    if( cn == 1 )
    {
        if( fixedRange )
        {
            while( !mask[R + 1] && DIFF_FLT_C1( img + (R+1), val0 ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_FLT_C1( img + (L-1), val0 ))
                mask[--L] = newMaskVal;
        }
        else
        {
            while( !mask[R + 1] && DIFF_FLT_C1( img + (R+1), img + R ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_FLT_C1( img + (L-1), img + L ))
                mask[--L] = newMaskVal;
        }
    }
    else
    {
        if( fixedRange )
        {
            while( !mask[R + 1] && DIFF_FLT_C3( img + (R+1)*3, val0 ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_FLT_C3( img + (L-1)*3, val0 ))
                mask[--L] = newMaskVal;
        }
        else
        {
            while( !mask[R + 1] && DIFF_FLT_C3( img + (R+1)*3, img + R*3 ))
                mask[++R] = newMaskVal;

            while( !mask[L - 1] && DIFF_FLT_C3( img + (L-1)*3, img + L*3 ))
                mask[--L] = newMaskVal;
        }
    }

    XMax = R;
    XMin = L;
    ICV_PUSH( seed.y, L, R, R + 1, R, UP );

    while( head != tail )
    {
        int k, YC, PL, PR, dir, curstep;
        ICV_POP( YC, L, R, PL, PR, dir );

        int data[][3] =
        {
            {-dir, L - _8_connectivity, R + _8_connectivity},
            {dir, L - _8_connectivity, PL - 1},
            {dir, PR + 1, R + _8_connectivity}
        };

        unsigned length = (unsigned)(R-L);

        if( region )
        {
            area += (int)length + 1;

            if( XMax < R ) XMax = R;
            if( XMin > L ) XMin = L;
            if( YMax < YC ) YMax = YC;
            if( YMin > YC ) YMin = YC;
        }

        if( cn == 1 )
        {
            for( k = 0; k < 3; k++ )
            {
                dir = data[k][0];
                curstep = dir * step;
                img = pImage + (YC + dir) * step;
                mask = pMask + (YC + dir) * maskStep;
                int left = data[k][1];
                int right = data[k][2];

                if( fixedRange )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_FLT_C1( img + i, val0 ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_FLT_C1( img + j, val0 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] && DIFF_FLT_C1( img + i, val0 ))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else if( !_8_connectivity )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_FLT_C1( img + i, img - curstep + i ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_FLT_C1( img + j, img + (j+1) ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   (DIFF_FLT_C1( img + i, img + (i-1) ) ||
                                   (DIFF_FLT_C1( img + i, img + i - curstep) && i <= R)))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else
                    for( i = left; i <= right; i++ )
                    {
                        int idx;
                        float val[1];
                
                        if( !mask[i] &&
                            ((val[0] = img[i],
                            (unsigned)(idx = i-L-1) <= length) &&
                            DIFF_FLT_C1( val, img - curstep + (i-1) ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_FLT_C1( val, img - curstep + i ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_FLT_C1( val, img - curstep + (i+1) )))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_FLT_C1( img + j, img + (j+1) ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   ((val[0] = img[i],
                                   DIFF_FLT_C1( val, img + (i-1) )) ||
                                   ((unsigned)(idx = i-L-1) <= length &&
                                   DIFF_FLT_C1( val, img - curstep + (i-1) )) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_FLT_C1( val, img - curstep + i ) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_FLT_C1( val, img - curstep + (i+1) )))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
            }

            img = pImage + YC * step;
            if( fillImage )
                for( i = L; i <= R; i++ )
                    img[i] = newVal[0];
            else if( region )
                for( i = L; i <= R; i++ )
                    sum[0] += img[i];
        }
        else
        {
            for( k = 0; k < 3; k++ )
            {
                dir = data[k][0];
                curstep = dir * step;
                img = pImage + (YC + dir) * step;
                mask = pMask + (YC + dir) * maskStep;
                int left = data[k][1];
                int right = data[k][2];

                if( fixedRange )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_FLT_C3( img + i*3, val0 ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_FLT_C3( img + j*3, val0 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] && DIFF_FLT_C3( img + i*3, val0 ))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else if( !_8_connectivity )
                    for( i = left; i <= right; i++ )
                    {
                        if( !mask[i] && DIFF_FLT_C3( img + i*3, img - curstep + i*3 ))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_FLT_C3( img + j*3, img + (j+1)*3 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   (DIFF_FLT_C3( img + i*3, img + (i-1)*3 ) ||
                                   (DIFF_FLT_C3( img + i*3, img + i*3 - curstep) && i <= R)))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
                else
                    for( i = left; i <= right; i++ )
                    {
                        int idx;
                        float val[3];
                
                        if( !mask[i] &&
                            ((ICV_SET_C3( val, img+i*3 ),
                            (unsigned)(idx = i-L-1) <= length) &&
                            DIFF_FLT_C3( val, img - curstep + (i-1)*3 ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_FLT_C3( val, img - curstep + i*3 ) ||
                            (unsigned)(++idx) <= length &&
                            DIFF_FLT_C3( val, img - curstep + (i+1)*3 )))
                        {
                            int j = i;
                            mask[i] = newMaskVal;
                            while( !mask[--j] && DIFF_FLT_C3( img + j*3, img + (j+1)*3 ))
                                mask[j] = newMaskVal;

                            while( !mask[++i] &&
                                   ((ICV_SET_C3( val, img + i*3 ),
                                   DIFF_FLT_C3( val, img + (i-1)*3 )) ||
                                   ((unsigned)(idx = i-L-1) <= length &&
                                   DIFF_FLT_C3( val, img - curstep + (i-1)*3 )) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_FLT_C3( val, img - curstep + i*3 ) ||
                                   (unsigned)(++idx) <= length &&
                                   DIFF_FLT_C3( val, img - curstep + (i+1)*3 )))
                                mask[i] = newMaskVal;

                            ICV_PUSH( YC + dir, j+1, i-1, L, R, -dir );
                        }
                    }
            }

            img = pImage + YC * step;
            if( fillImage )
                for( i = L; i <= R; i++ )
                    ICV_SET_C3( img + i*3, newVal );
            else if( region )
                for( i = L; i <= R; i++ )
                {
                    sum[0] += img[i*3];
                    sum[1] += img[i*3+1];
                    sum[2] += img[i*3+2];
                }
        }
    }
    
    if( region )
    {
        region->area = area;
        region->rect.x = XMin;
        region->rect.y = YMin;
        region->rect.width = XMax - XMin + 1;
        region->rect.height = YMax - YMin + 1;
    
        if( fillImage )
            region->value = cvScalar(newVal[0], newVal[1], newVal[2]);
        else
        {
            double iarea = area ? 1./area : 0;
            region->value = cvScalar(sum[0]*iarea, sum[1]*iarea, sum[2]*iarea);
        }
    }

    return CV_NO_ERR;
}
// 
// 
// /****************************************************************************************\
// *                                    External Functions                                  *
// \****************************************************************************************/
// 
typedef  CvStatus (CV_CDECL* CvFloodFillFunc)(
               void* img, int step, CvSize size, CvPoint seed, void* newval,
               CvConnectedComp* comp, int flags, void* buffer, int buffer_size, int cn );
// 
typedef  CvStatus (CV_CDECL* CvFloodFillGradFunc)(
               void* img, int step, uchar* mask, int maskStep, CvSize size,
               CvPoint seed, void* newval, void* d_lw, void* d_up, void* ccomp,
               int flags, void* buffer, int buffer_size, int cn );
// 
static  void  icvInitFloodFill( void** ffill_tab,
                                void** ffillgrad_tab )
{
    ffill_tab[0] = (void*)icvFloodFill_8u_CnIR;
    ffill_tab[1] = (void*)icvFloodFill_32f_CnIR;

    ffillgrad_tab[0] = (void*)icvFloodFill_Grad_8u_CnIR;
    ffillgrad_tab[1] = (void*)icvFloodFill_Grad_32f_CnIR;
}
// 
// 
CV_IMPL void
cvFloodFill( CvArr* arr, CvPoint seed_point,
             CvScalar newVal, CvScalar lo_diff, CvScalar up_diff,
             CvConnectedComp* comp, int flags, CvArr* maskarr )
{
    static void* ffill_tab[4];
    static void* ffillgrad_tab[4];
    static int inittab = 0;

    CvMat* tempMask = 0;
    CvFFillSegment* buffer = 0;
    CV_FUNCNAME( "cvFloodFill" );

    if( comp )
        memset( comp, 0, sizeof(*comp) );

    __BEGIN__;

    int i, type, depth, cn, is_simple, idx;
    int buffer_size, connectivity = flags & 255;
    double nv_buf[4] = {0,0,0,0};
    union { uchar b[4]; float f[4]; } ld_buf, ud_buf;
    CvMat stub, *img = (CvMat*)arr;
    CvMat maskstub, *mask = (CvMat*)maskarr;
    CvSize size;

    if( !inittab )
    {
        icvInitFloodFill( ffill_tab, ffillgrad_tab );
        inittab = 1;
    }

    CV_CALL( img = cvGetMat( img, &stub ));
    type = CV_MAT_TYPE( img->type );
    depth = CV_MAT_DEPTH(type);
    cn = CV_MAT_CN(type);

    idx = type == CV_8UC1 || type == CV_8UC3 ? 0 :
          type == CV_32FC1 || type == CV_32FC3 ? 1 : -1;

    if( idx < 0 )
        CV_ERROR( CV_StsUnsupportedFormat, "" );

    if( connectivity == 0 )
        connectivity = 4;
    else if( connectivity != 4 && connectivity != 8 )
        CV_ERROR( CV_StsBadFlag, "Connectivity must be 4, 0(=4) or 8" );

    is_simple = mask == 0 && (flags & CV_FLOODFILL_MASK_ONLY) == 0;

    for( i = 0; i < cn; i++ )
    {
        if( lo_diff.val[i] < 0 || up_diff.val[i] < 0 )
            CV_ERROR( CV_StsBadArg, "lo_diff and up_diff must be non-negative" );
        is_simple &= fabs(lo_diff.val[i]) < DBL_EPSILON && fabs(up_diff.val[i]) < DBL_EPSILON;
    }

    size = cvGetMatSize( img );

    if( (unsigned)seed_point.x >= (unsigned)size.width ||
        (unsigned)seed_point.y >= (unsigned)size.height )
        CV_ERROR( CV_StsOutOfRange, "Seed point is outside of image" );

    cvScalarToRawData( &newVal, &nv_buf, type, 0 );
    buffer_size = MAX( size.width, size.height )*2;
    CV_CALL( buffer = (CvFFillSegment*)cvAlloc( buffer_size*sizeof(buffer[0])));

    if( is_simple )
    {
        CvFloodFillFunc func = (CvFloodFillFunc)ffill_tab[idx];
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        
        IPPI_CALL( func( img->data.ptr, img->step, size,
                         seed_point, &nv_buf, comp, flags,
                         buffer, buffer_size, cn ));
    }
    else
    {
        CvFloodFillGradFunc func = (CvFloodFillGradFunc)ffillgrad_tab[idx];
        if( !func )
            CV_ERROR( CV_StsUnsupportedFormat, "" );
        
        if( !mask )
        {
            /* created mask will be 8-byte aligned */
            tempMask = cvCreateMat( size.height + 2, (size.width + 9) & -8, CV_8UC1 );
            mask = tempMask;
        }
        else
        {
            CV_CALL( mask = cvGetMat( mask, &maskstub ));
            if( !CV_IS_MASK_ARR( mask ))
                CV_ERROR( CV_StsBadMask, "" );

            if( mask->width != size.width + 2 || mask->height != size.height + 2 )
                CV_ERROR( CV_StsUnmatchedSizes, "mask must be 2 pixel wider "
                                       "and 2 pixel taller than filled image" );
        }

        {
            int width = tempMask ? mask->step : size.width + 2;
            uchar* mask_row = mask->data.ptr + mask->step;
            memset( mask_row - mask->step, 1, width );

            for( i = 1; i <= size.height; i++, mask_row += mask->step )
            {
                if( tempMask )
                    memset( mask_row, 0, width );
                mask_row[0] = mask_row[size.width+1] = (uchar)1;
            }
            memset( mask_row, 1, width );
        }

        if( depth == CV_8U )
            for( i = 0; i < cn; i++ )
            {
                int t = cvFloor(lo_diff.val[i]);
                ld_buf.b[i] = CV_CAST_8U(t);
                t = cvFloor(up_diff.val[i]);
                ud_buf.b[i] = CV_CAST_8U(t);
            }
        else
            for( i = 0; i < cn; i++ )
            {
                ld_buf.f[i] = (float)lo_diff.val[i];
                ud_buf.f[i] = (float)up_diff.val[i];
            }

        IPPI_CALL( func( img->data.ptr, img->step, mask->data.ptr, mask->step,
                         size, seed_point, &nv_buf, ld_buf.f, ud_buf.f,
                         comp, flags, buffer, buffer_size, cn ));
    }

    __END__;

    cvFree( &buffer );
    cvReleaseMat( &tempMask );
}

CV_IMPL void
cvConvertPointsHomogenious( const CvMat* src, CvMat* dst )
{
    CvMat* temp = 0;
    CvMat* denom = 0;

    CV_FUNCNAME( "cvConvertPointsHomogenious" );

    __BEGIN__;

    int i, s_count, s_dims, d_count, d_dims;
    CvMat _src, _dst, _ones;
    CvMat* ones = 0;

    if( !CV_IS_MAT(src) )
        CV_ERROR( !src ? CV_StsNullPtr : CV_StsBadArg,
        "The input parameter is not a valid matrix" );

    if( !CV_IS_MAT(dst) )
        CV_ERROR( !dst ? CV_StsNullPtr : CV_StsBadArg,
        "The output parameter is not a valid matrix" );

    if( src == dst || src->data.ptr == dst->data.ptr )
    {
        if( src != dst && (!CV_ARE_TYPES_EQ(src, dst) || !CV_ARE_SIZES_EQ(src,dst)) )
            CV_ERROR( CV_StsBadArg, "Invalid inplace operation" );
        EXIT;
    }

    if( src->rows > src->cols )
    {
        if( !((src->cols > 1) ^ (CV_MAT_CN(src->type) > 1)) )
            CV_ERROR( CV_StsBadSize, "Either the number of channels or columns or rows must be =1" );

        s_dims = CV_MAT_CN(src->type)*src->cols;
        s_count = src->rows;
    }
    else
    {
        if( !((src->rows > 1) ^ (CV_MAT_CN(src->type) > 1)) )
            CV_ERROR( CV_StsBadSize, "Either the number of channels or columns or rows must be =1" );

        s_dims = CV_MAT_CN(src->type)*src->rows;
        s_count = src->cols;
    }

    if( src->rows == 1 || src->cols == 1 )
        src = cvReshape( src, &_src, 1, s_count );

    if( dst->rows > dst->cols )
    {
        if( !((dst->cols > 1) ^ (CV_MAT_CN(dst->type) > 1)) )
            CV_ERROR( CV_StsBadSize,
            "Either the number of channels or columns or rows in the input matrix must be =1" );

        d_dims = CV_MAT_CN(dst->type)*dst->cols;
        d_count = dst->rows;
    }
    else
    {
        if( !((dst->rows > 1) ^ (CV_MAT_CN(dst->type) > 1)) )
            CV_ERROR( CV_StsBadSize,
            "Either the number of channels or columns or rows in the output matrix must be =1" );

        d_dims = CV_MAT_CN(dst->type)*dst->rows;
        d_count = dst->cols;
    }

    if( dst->rows == 1 || dst->cols == 1 )
        dst = cvReshape( dst, &_dst, 1, d_count );

    if( s_count != d_count )
        CV_ERROR( CV_StsUnmatchedSizes, "Both matrices must have the same number of points" );

    if( CV_MAT_DEPTH(src->type) < CV_32F || CV_MAT_DEPTH(dst->type) < CV_32F )
        CV_ERROR( CV_StsUnsupportedFormat,
        "Both matrices must be floating-point (single or double precision)" );

    if( s_dims < 2 || s_dims > 4 || d_dims < 2 || d_dims > 4 )
        CV_ERROR( CV_StsOutOfRange,
        "Both input and output point dimensionality must be 2, 3 or 4" );

    if( s_dims < d_dims - 1 || s_dims > d_dims + 1 )
        CV_ERROR( CV_StsUnmatchedSizes,
        "The dimensionalities of input and output point sets differ too much" );

    if( s_dims == d_dims - 1 )
    {
        if( d_count == dst->rows )
        {
            ones = cvGetSubRect( dst, &_ones, cvRect( s_dims, 0, 1, d_count ));
            dst = cvGetSubRect( dst, &_dst, cvRect( 0, 0, s_dims, d_count ));
        }
        else
        {
            ones = cvGetSubRect( dst, &_ones, cvRect( 0, s_dims, d_count, 1 ));
            dst = cvGetSubRect( dst, &_dst, cvRect( 0, 0, d_count, s_dims ));
        }
    }

    if( s_dims <= d_dims )
    {
        if( src->rows == dst->rows && src->cols == dst->cols )
        {
            if( CV_ARE_TYPES_EQ( src, dst ) )
                cvCopy( src, dst );
            else
                cvConvert( src, dst );
        }
        else
        {
            if( !CV_ARE_TYPES_EQ( src, dst ))
            {
                CV_CALL( temp = cvCreateMat( src->rows, src->cols, dst->type ));
                cvConvert( src, temp );
                src = temp;
            }
            cvTranspose( src, dst );
        }

        if( ones )
            cvSet( ones, cvRealScalar(1.) );
    }
    else
    {
        int s_plane_stride, s_stride, d_plane_stride, d_stride, elem_size;

        if( !CV_ARE_TYPES_EQ( src, dst ))
        {
            CV_CALL( temp = cvCreateMat( src->rows, src->cols, dst->type ));
            cvConvert( src, temp );
            src = temp;
        }

        elem_size = CV_ELEM_SIZE(src->type);

        if( s_count == src->cols )
            s_plane_stride = src->step / elem_size, s_stride = 1;
        else
            s_stride = src->step / elem_size, s_plane_stride = 1;

        if( d_count == dst->cols )
            d_plane_stride = dst->step / elem_size, d_stride = 1;
        else
            d_stride = dst->step / elem_size, d_plane_stride = 1;

        CV_CALL( denom = cvCreateMat( 1, d_count, dst->type ));

        if( CV_MAT_DEPTH(dst->type) == CV_32F )
        {
            const float* xs = src->data.fl;
            const float* ys = xs + s_plane_stride;
            const float* zs = 0;
            const float* ws = xs + (s_dims - 1)*s_plane_stride;

            float* iw = denom->data.fl;

            float* xd = dst->data.fl;
            float* yd = xd + d_plane_stride;
            float* zd = 0;

            if( d_dims == 3 )
            {
                zs = ys + s_plane_stride;
                zd = yd + d_plane_stride;
            }

            for( i = 0; i < d_count; i++, ws += s_stride )
            {
                float t = *ws;
                iw[i] = t ? t : 1.f;
            }

            cvDiv( 0, denom, denom );

            if( d_dims == 3 )
                for( i = 0; i < d_count; i++ )
                {
                    float w = iw[i];
                    float x = *xs * w, y = *ys * w, z = *zs * w;
                    xs += s_stride; ys += s_stride; zs += s_stride;
                    *xd = x; *yd = y; *zd = z;
                    xd += d_stride; yd += d_stride; zd += d_stride;
                }
            else
                for( i = 0; i < d_count; i++ )
                {
                    float w = iw[i];
                    float x = *xs * w, y = *ys * w;
                    xs += s_stride; ys += s_stride;
                    *xd = x; *yd = y;
                    xd += d_stride; yd += d_stride;
                }
        }
        else
        {
            const double* xs = src->data.db;
            const double* ys = xs + s_plane_stride;
            const double* zs = 0;
            const double* ws = xs + (s_dims - 1)*s_plane_stride;

            double* iw = denom->data.db;

            double* xd = dst->data.db;
            double* yd = xd + d_plane_stride;
            double* zd = 0;

            if( d_dims == 3 )
            {
                zs = ys + s_plane_stride;
                zd = yd + d_plane_stride;
            }

            for( i = 0; i < d_count; i++, ws += s_stride )
            {
                double t = *ws;
                iw[i] = t ? t : 1.;
            }

            cvDiv( 0, denom, denom );

            if( d_dims == 3 )
                for( i = 0; i < d_count; i++ )
                {
                    double w = iw[i];
                    double x = *xs * w, y = *ys * w, z = *zs * w;
                    xs += s_stride; ys += s_stride; zs += s_stride;
                    *xd = x; *yd = y; *zd = z;
                    xd += d_stride; yd += d_stride; zd += d_stride;
                }
            else
                for( i = 0; i < d_count; i++ )
                {
                    double w = iw[i];
                    double x = *xs * w, y = *ys * w;
                    xs += s_stride; ys += s_stride;
                    *xd = x; *yd = y;
                    xd += d_stride; yd += d_stride;
                }
        }
    }

    __END__;

    cvReleaseMat( &denom );
    cvReleaseMat( &temp );
}
// 
// /* End of file. */
int
icvIntersectLines( double x1, double dx1, double y1, double dy1,
				  double x2, double dx2, double y2, double dy2, double *t2 )
{
    double d = dx1 * dy2 - dx2 * dy1;
    int result = -1;
	
    if( d != 0 )
    {
        *t2 = ((x2 - x1) * dy1 - (y2 - y1) * dx1) / d;
        result = 0;
    }
    return result;
}
static CvStatus
icvContourArea( const CvSeq* contour, double *area )
{
    if( contour->total )
    {
        CvSeqReader reader;
        int lpt = contour->total;
        double a00 = 0, xi_1, yi_1;
        int is_float = CV_SEQ_ELTYPE(contour) == CV_32FC2;

        cvStartReadSeq( contour, &reader, 0 );

        if( !is_float )
        {
            xi_1 = ((CvPoint*)(reader.ptr))->x;
            yi_1 = ((CvPoint*)(reader.ptr))->y;
        }
        else
        {
            xi_1 = ((CvPoint2D32f*)(reader.ptr))->x;
            yi_1 = ((CvPoint2D32f*)(reader.ptr))->y;
        }
        CV_NEXT_SEQ_ELEM( contour->elem_size, reader );
        
        while( lpt-- > 0 )
        {
            double dxy, xi, yi;

            if( !is_float )
            {
                xi = ((CvPoint*)(reader.ptr))->x;
                yi = ((CvPoint*)(reader.ptr))->y;
            }
            else
            {
                xi = ((CvPoint2D32f*)(reader.ptr))->x;
                yi = ((CvPoint2D32f*)(reader.ptr))->y;
            }
            CV_NEXT_SEQ_ELEM( contour->elem_size, reader );

            dxy = xi_1 * yi - xi * yi_1;
            a00 += dxy;
            xi_1 = xi;
            yi_1 = yi;
        }

        *area = a00 * 0.5;
    }
    else
        *area = 0;

    return CV_OK;
}
// 
// 
// /****************************************************************************************\
// 
//  copy data from one buffer to other buffer 
// 
// \****************************************************************************************/
// 
static CvStatus
icvMemCopy( double **buf1, double **buf2, double **buf3, int *b_max )
{
    int bb;

    if( *buf1 == NULL && *buf2 == NULL || *buf3 == NULL )
        return CV_NULLPTR_ERR;

    bb = *b_max;
    if( *buf2 == NULL )
    {
        *b_max = 2 * (*b_max);
        *buf2 = (double *)cvAlloc( (*b_max) * sizeof( double ));

        if( *buf2 == NULL )
            return CV_OUTOFMEM_ERR;

        memcpy( *buf2, *buf3, bb * sizeof( double ));

        *buf3 = *buf2;
        cvFree( buf1 );
        *buf1 = NULL;
    }
    else
    {
        *b_max = 2 * (*b_max);
        *buf1 = (double *) cvAlloc( (*b_max) * sizeof( double ));

        if( *buf1 == NULL )
            return CV_OUTOFMEM_ERR;

        memcpy( *buf1, *buf3, bb * sizeof( double ));

        *buf3 = *buf1;
        cvFree( buf2 );
        *buf2 = NULL;
    }
    return CV_OK;
}
// 
// 
// /* area of a contour sector */
static CvStatus icvContourSecArea( CvSeq * contour, CvSlice slice, double *area )
{
    CvPoint pt;                 /*  pointer to points   */
    CvPoint pt_s, pt_e;         /*  first and last points  */
    CvSeqReader reader;         /*  points reader of contour   */

    int p_max = 2, p_ind;
    int lpt, flag, i;
    double a00;                 /* unnormalized moments m00    */
    double xi, yi, xi_1, yi_1, x0, y0, dxy, sk, sk1, t;
    double x_s, y_s, nx, ny, dx, dy, du, dv;
    double eps = 1.e-5;
    double *p_are1, *p_are2, *p_are;

    assert( contour != NULL );

    if( contour == NULL )
        return CV_NULLPTR_ERR;

    if( !CV_IS_SEQ_POLYGON( contour ))
        return CV_BADFLAG_ERR;

    lpt = cvSliceLength( slice, contour );
    /*if( n2 >= n1 )
        lpt = n2 - n1 + 1;
    else
        lpt = contour->total - n1 + n2 + 1;*/

    if( contour->total && lpt > 2 )
    {
        a00 = x0 = y0 = xi_1 = yi_1 = 0;
        sk1 = 0;
        flag = 0;
        dxy = 0;
        p_are1 = (double *) cvAlloc( p_max * sizeof( double ));

        if( p_are1 == NULL )
            return CV_OUTOFMEM_ERR;

        p_are = p_are1;
        p_are2 = NULL;

        cvStartReadSeq( contour, &reader, 0 );
        cvSetSeqReaderPos( &reader, slice.start_index );
        CV_READ_SEQ_ELEM( pt_s, reader );
        p_ind = 0;
        cvSetSeqReaderPos( &reader, slice.end_index );
        CV_READ_SEQ_ELEM( pt_e, reader );

/*    normal coefficients    */
        nx = pt_s.y - pt_e.y;
        ny = pt_e.x - pt_s.x;
        cvSetSeqReaderPos( &reader, slice.start_index );

        while( lpt-- > 0 )
        {
            CV_READ_SEQ_ELEM( pt, reader );

            if( flag == 0 )
            {
                xi_1 = (double) pt.x;
                yi_1 = (double) pt.y;
                x0 = xi_1;
                y0 = yi_1;
                sk1 = 0;
                flag = 1;
            }
            else
            {
                xi = (double) pt.x;
                yi = (double) pt.y;

/****************   edges intersection examination   **************************/
                sk = nx * (xi - pt_s.x) + ny * (yi - pt_s.y);
                if( fabs( sk ) < eps && lpt > 0 || sk * sk1 < -eps )
                {
                    if( fabs( sk ) < eps )
                    {
                        dxy = xi_1 * yi - xi * yi_1;
                        a00 = a00 + dxy;
                        dxy = xi * y0 - x0 * yi;
                        a00 = a00 + dxy;

                        if( p_ind >= p_max )
                            icvMemCopy( &p_are1, &p_are2, &p_are, &p_max );

                        p_are[p_ind] = a00 / 2.;
                        p_ind++;
                        a00 = 0;
                        sk1 = 0;
                        x0 = xi;
                        y0 = yi;
                        dxy = 0;
                    }
                    else
                    {
/*  define intersection point    */
                        dv = yi - yi_1;
                        du = xi - xi_1;
                        dx = ny;
                        dy = -nx;
                        if( fabs( du ) > eps )
                            t = ((yi_1 - pt_s.y) * du + dv * (pt_s.x - xi_1)) /
                                (du * dy - dx * dv);
                        else
                            t = (xi_1 - pt_s.x) / dx;
                        if( t > eps && t < 1 - eps )
                        {
                            x_s = pt_s.x + t * dx;
                            y_s = pt_s.y + t * dy;
                            dxy = xi_1 * y_s - x_s * yi_1;
                            a00 += dxy;
                            dxy = x_s * y0 - x0 * y_s;
                            a00 += dxy;
                            if( p_ind >= p_max )
                                icvMemCopy( &p_are1, &p_are2, &p_are, &p_max );

                            p_are[p_ind] = a00 / 2.;
                            p_ind++;

                            a00 = 0;
                            sk1 = 0;
                            x0 = x_s;
                            y0 = y_s;
                            dxy = x_s * yi - xi * y_s;
                        }
                    }
                }
                else
                    dxy = xi_1 * yi - xi * yi_1;

                a00 += dxy;
                xi_1 = xi;
                yi_1 = yi;
                sk1 = sk;

            }
        }

        xi = x0;
        yi = y0;
        dxy = xi_1 * yi - xi * yi_1;

        a00 += dxy;

        if( p_ind >= p_max )
            icvMemCopy( &p_are1, &p_are2, &p_are, &p_max );

        p_are[p_ind] = a00 / 2.;
        p_ind++;

/*     common area calculation    */
        *area = 0;
        for( i = 0; i < p_ind; i++ )
            (*area) += fabs( p_are[i] );

        if( p_are1 != NULL )
            cvFree( &p_are1 );
        else if( p_are2 != NULL )
            cvFree( &p_are2 );

        return CV_OK;
    }
    else
        return CV_BADSIZE_ERR;
}
// 
// 
// /* external contour area function */
CV_IMPL double
cvContourArea( const void *array, CvSlice slice )
{
    double area = 0;

    CV_FUNCNAME( "cvContourArea" );

    __BEGIN__;

    CvContour contour_header;
    CvSeq* contour = 0;
    CvSeqBlock block;

    if( CV_IS_SEQ( array ))
    {
        contour = (CvSeq*)array;
        if( !CV_IS_SEQ_POLYLINE( contour ))
            CV_ERROR( CV_StsBadArg, "Unsupported sequence type" );
    }
    else
    {
        CV_CALL( contour = cvPointSeqFromMat(
            CV_SEQ_KIND_CURVE, array, &contour_header, &block ));
    }

    if( cvSliceLength( slice, contour ) == contour->total )
    {
        IPPI_CALL( icvContourArea( contour, &area ));
    }
    else
    {
        if( CV_SEQ_ELTYPE( contour ) != CV_32SC2 )
            CV_ERROR( CV_StsUnsupportedFormat,
            "Only curves with integer coordinates are supported in case of contour slice" );
        IPPI_CALL( icvContourSecArea( contour, slice, &area ));
    }

    __END__;

    return area;
}

// /* Calculates bounding rectagnle of a point set or retrieves already calculated */
CV_IMPL  CvRect
cvBoundingRect( CvArr* array, int update )
{
    CvSeqReader reader;
    CvRect  rect = { 0, 0, 0, 0 };
    CvContour contour_header;
    CvSeq* ptseq = 0;
    CvSeqBlock block;

    CV_FUNCNAME( "cvBoundingRect" );

    __BEGIN__;

    CvMat stub, *mat = 0;
    int  xmin = 0, ymin = 0, xmax = -1, ymax = -1, i, j, k;
    int calculate = update;

    if( CV_IS_SEQ( array ))
    {
        ptseq = (CvSeq*)array;
        if( !CV_IS_SEQ_POINT_SET( ptseq ))
            CV_ERROR( CV_StsBadArg, "Unsupported sequence type" );

        if( ptseq->header_size < (int)sizeof(CvContour))
        {
            /*if( update == 1 )
                CV_ERROR( CV_StsBadArg, "The header is too small to fit the rectangle, "
                                        "so it could not be updated" );*/
            update = 0;
            calculate = 1;
        }
    }
    else
    {
        CV_CALL( mat = cvGetMat( array, &stub ));
        if( CV_MAT_TYPE(mat->type) == CV_32SC1 ||
            CV_MAT_TYPE(mat->type) == CV_32FC1 )
        {
            CV_CALL( ptseq = cvPointSeqFromMat(
                CV_SEQ_KIND_GENERIC, mat, &contour_header, &block ));
            mat = 0;
        }
        else if( CV_MAT_TYPE(mat->type) != CV_8UC1 &&
                CV_MAT_TYPE(mat->type) != CV_8SC1 )
            CV_ERROR( CV_StsUnsupportedFormat,
                "The image/matrix format is not supported by the function" );
        update = 0;
        calculate = 1;
    }

    if( !calculate )
    {
        rect = ((CvContour*)ptseq)->rect;
        EXIT;
    }

    if( mat )
    {
        CvSize size = cvGetMatSize(mat);
        xmin = size.width;
        ymin = -1;

        for( i = 0; i < size.height; i++ )
        {
            uchar* _ptr = mat->data.ptr + i*mat->step;
            uchar* ptr = (uchar*)cvAlignPtr(_ptr, 4);
            int have_nz = 0, k_min, offset = (int)(ptr - _ptr);
            j = 0;
            offset = MIN(offset, size.width);
            for( ; j < offset; j++ )
                if( _ptr[j] )
                {
                    have_nz = 1;
                    break;
                }
            if( j < offset )
            {
                if( j < xmin )
                    xmin = j;
                if( j > xmax )
                    xmax = j;
            }
            if( offset < size.width )
            {
                xmin -= offset;
                xmax -= offset;
                size.width -= offset;
                j = 0;
                for( ; j <= xmin - 4; j += 4 )
                    if( *((int*)(ptr+j)) )
                        break;
                for( ; j < xmin; j++ )
                    if( ptr[j] )
                    {
                        xmin = j;
                        if( j > xmax )
                            xmax = j;
                        have_nz = 1;
                        break;
                    }
                k_min = MAX(j-1, xmax);
                k = size.width - 1;
                for( ; k > k_min && (k&3) != 3; k-- )
                    if( ptr[k] )
                        break;
                if( k > k_min && (k&3) == 3 )
                {
                    for( ; k > k_min+3; k -= 4 )
                        if( *((int*)(ptr+k-3)) )
                            break;
                }
                for( ; k > k_min; k-- )
                    if( ptr[k] )
                    {
                        xmax = k;
                        have_nz = 1;
                        break;
                    }
                if( !have_nz )
                {
                    j &= ~3;
                    for( ; j <= k - 3; j += 4 )
                        if( *((int*)(ptr+j)) )
                            break;
                    for( ; j <= k; j++ )
                        if( ptr[j] )
                        {
                            have_nz = 1;
                            break;
                        }
                }
                xmin += offset;
                xmax += offset;
                size.width += offset;
            }
            if( have_nz )
            {
                if( ymin < 0 )
                    ymin = i;
                ymax = i;
            }
        }

        if( xmin >= size.width )
            xmin = ymin = 0;
    }
    else if( ptseq->total )
    {   
        int  is_float = CV_SEQ_ELTYPE(ptseq) == CV_32FC2;
        cvStartReadSeq( ptseq, &reader, 0 );

        if( !is_float )
        {
            CvPoint pt;
            /* init values */
            CV_READ_SEQ_ELEM( pt, reader );
            xmin = xmax = pt.x;
            ymin = ymax = pt.y;

            for( i = 1; i < ptseq->total; i++ )
            {            
                CV_READ_SEQ_ELEM( pt, reader );
        
                if( xmin > pt.x )
                    xmin = pt.x;
        
                if( xmax < pt.x )
                    xmax = pt.x;

                if( ymin > pt.y )
                    ymin = pt.y;

                if( ymax < pt.y )
                    ymax = pt.y;
            }
        }
        else
        {
            CvPoint pt;
            Cv32suf v;
            /* init values */
            CV_READ_SEQ_ELEM( pt, reader );
            xmin = xmax = CV_TOGGLE_FLT(pt.x);
            ymin = ymax = CV_TOGGLE_FLT(pt.y);

            for( i = 1; i < ptseq->total; i++ )
            {            
                CV_READ_SEQ_ELEM( pt, reader );
                pt.x = CV_TOGGLE_FLT(pt.x);
                pt.y = CV_TOGGLE_FLT(pt.y);
        
                if( xmin > pt.x )
                    xmin = pt.x;
        
                if( xmax < pt.x )
                    xmax = pt.x;

                if( ymin > pt.y )
                    ymin = pt.y;

                if( ymax < pt.y )
                    ymax = pt.y;
            }

            v.i = CV_TOGGLE_FLT(xmin); xmin = cvFloor(v.f);
            v.i = CV_TOGGLE_FLT(ymin); ymin = cvFloor(v.f);
            /* because right and bottom sides of
               the bounding rectangle are not inclusive
               (note +1 in width and height calculation below),
               cvFloor is used here instead of cvCeil */
            v.i = CV_TOGGLE_FLT(xmax); xmax = cvFloor(v.f);
            v.i = CV_TOGGLE_FLT(ymax); ymax = cvFloor(v.f);
        }
    }

    rect.x = xmin;
    rect.y = ymin;
    rect.width = xmax - xmin + 1;
    rect.height = ymax - ymin + 1;

    if( update )
        ((CvContour*)ptseq)->rect = rect;

    __END__;

    return rect;
}
