#include "stdafx.h"
#include "mmplab_highgui.h"
#include "mmplab_grfmt.h"

class  CvImageFilters
 {
public:

    CvImageFilters();
    ~CvImageFilters();

    GrFmtReader* FindReader( const char* filename ) const;
    GrFmtWriter* FindWriter( const char* filename ) const;

protected:

    GrFmtFactoriesList*  m_factories;
};

CvImageFilters::CvImageFilters()
{
    m_factories = new GrFmtFactoriesList;

    m_factories->AddFactory( new GrFmtBmp() );
    m_factories->AddFactory( new GrFmtJpeg() );
    m_factories->AddFactory( new GrFmtSunRaster() );
    m_factories->AddFactory( new GrFmtPxM() );
    m_factories->AddFactory( new GrFmtTiff() );
#ifdef HAVE_PNG
    m_factories->AddFactory( new GrFmtPng() );
#endif
#ifdef HAVE_JASPER
    m_factories->AddFactory( new GrFmtJpeg2000() );
#endif
#ifdef HAVE_ILMIMF
    m_factories->AddFactory( new GrFmtExr() );
#endif
}


CvImageFilters::~CvImageFilters()
{
    delete m_factories;
}

GrFmtReader* CvImageFilters::FindReader( const char* filename ) const
{
    return m_factories->FindReader( filename );
}
 
GrFmtWriter* CvImageFilters::FindWriter( const char* filename ) const
{
    return m_factories->FindWriter( filename );
}
 static CvImageFilters  g_Filters;

static void*
icvLoadImage( const char* filename, int flags, bool load_as_matrix )
{
    GrFmtReader* reader = 0;
    IplImage* image = 0;
    CvMat hdr, *matrix = 0;
    int depth = 8;

    CV_FUNCNAME( "cvLoadImage" );

    __BEGIN__;

    CvSize size;
    int iscolor;
    int cn;

    if( !filename || strlen(filename) == 0 )
        CV_ERROR( CV_StsNullPtr, "null filename" );

    reader = g_Filters.FindReader( filename );
    if( !reader )
        EXIT;

    if( !reader->ReadHeader() )
        EXIT;

    size.width = reader->GetWidth();
    size.height = reader->GetHeight();

    if( flags == -1 )
        iscolor = reader->IsColor();
    else
    {
        if( (flags & CV_LOAD_IMAGE_COLOR) != 0 ||
           ((flags & CV_LOAD_IMAGE_ANYCOLOR) != 0 && reader->IsColor()) )
            iscolor = 1;
        else
            iscolor = 0;

        if( (flags & CV_LOAD_IMAGE_ANYDEPTH) != 0 )
        {
            reader->UseNativeDepth(true);
            depth = reader->GetDepth();
        }
    }

    cn = iscolor ? 3 : 1;

    if( load_as_matrix )
    {
        int type;
        if(reader->IsFloat() && depth != 8)
            type = CV_32F;
        else
            type = ( depth <= 8 ) ? CV_8U : ( depth <= 16 ) ? CV_16U : CV_32S;
        CV_CALL( matrix = cvCreateMat( size.height, size.width, CV_MAKETYPE(type, cn) ));
    }
    else
    {
        int type;
        if(reader->IsFloat() && depth != 8)
            type = IPL_DEPTH_32F;
        else
            type = ( depth <= 8 ) ? IPL_DEPTH_8U : ( depth <= 16 ) ? IPL_DEPTH_16U : IPL_DEPTH_32S;
        CV_CALL( image = cvCreateImage( size, type, cn ));
        matrix = cvGetMat( image, &hdr );
    }

    if( !reader->ReadData( matrix->data.ptr, matrix->step, iscolor ))
    {
        if( load_as_matrix )
            cvReleaseMat( &matrix );
        else
            cvReleaseImage( &image );
        EXIT;
    }

    __END__;

    delete reader;

    if( cvGetErrStatus() < 0 )
    {
        if( load_as_matrix )
            cvReleaseMat( &matrix );
        else
            cvReleaseImage( &image );
    }

    return load_as_matrix ? (void*)matrix : (void*)image;
}
 
CV_IMPL IplImage*
 cvLoadImage( const char* filename, int iscolor )
{
    return (IplImage*)icvLoadImage( filename, iscolor, false );
}

CV_IMPL int
cvSaveImage( const char* filename, const CvArr* arr )
{
    int origin = 0;
    GrFmtWriter* writer = 0;
    CvMat *temp = 0, *temp2 = 0;

    CV_FUNCNAME( "cvSaveImage" );

    __BEGIN__;

    CvMat stub, *image;
    int channels, ipl_depth;

    if( !filename || strlen(filename) == 0 )
        CV_ERROR( CV_StsNullPtr, "null filename" );

    CV_CALL( image = cvGetMat( arr, &stub ));

    if( CV_IS_IMAGE( arr ))
        origin = ((IplImage*)arr)->origin;

    channels = CV_MAT_CN( image->type );
    if( channels != 1 && channels != 3 && channels != 4 )
        CV_ERROR( CV_BadNumChannels, "" );

    writer = g_Filters.FindWriter( filename );
    if( !writer )
        CV_ERROR( CV_StsError, "could not find a filter for the specified extension" );

    if( origin )
    {
        CV_CALL( temp = cvCreateMat(image->rows, image->cols, image->type) );
        CV_CALL( cvFlip( image, temp, 0 ));
        image = temp;
    }

    ipl_depth = cvCvToIplDepth(image->type);

    if( !writer->IsFormatSupported(ipl_depth) )
    {
        assert( writer->IsFormatSupported(IPL_DEPTH_8U) );
        CV_CALL( temp2 = cvCreateMat(image->rows,
            image->cols, CV_MAKETYPE(CV_8U,channels)) );
        CV_CALL( cvConvertImage( image, temp2 ));
        image = temp2;
        ipl_depth = IPL_DEPTH_8U;
    }

    if( !writer->WriteImage( image->data.ptr, image->step, image->width,
                             image->height, ipl_depth, channels ))
        CV_ERROR( CV_StsError, "could not save the image" );

    __END__;

    delete writer;
    cvReleaseMat( &temp );
    cvReleaseMat( &temp2 );

    return cvGetErrStatus() >= 0;
}

#include "utils.h"

#define  SCALE  14
#define  cR  (int)(0.299*(1 << SCALE) + 0.5)
#define  cG  (int)(0.587*(1 << SCALE) + 0.5)
#define  cB  ((1 << SCALE) - cR - cG)


void icvCvt_BGR2Gray_8u_C3C1R( const uchar* rgb, int rgb_step,
                               uchar* gray, int gray_step,
                               CvSize size, int _swap_rb )
{
    int i;
    int swap_rb = _swap_rb ? 2 : 0;
    for( ; size.height--; gray += gray_step )
    {
        for( i = 0; i < size.width; i++, rgb += 3 )
        {
            int t = descale( rgb[swap_rb]*cB + rgb[1]*cG + rgb[swap_rb^2]*cR, SCALE );
            gray[i] = (uchar)t;
        }

        rgb += rgb_step - size.width*3;
    }
}


void icvCvt_BGRA2Gray_8u_C4C1R( const uchar* rgba, int rgba_step,
                                uchar* gray, int gray_step,
                                CvSize size, int _swap_rb )
{
    int i;
    int swap_rb = _swap_rb ? 2 : 0;
    for( ; size.height--; gray += gray_step )
    {
        for( i = 0; i < size.width; i++, rgba += 4 )
        {
            int t = descale( rgba[swap_rb]*cB + rgba[1]*cG + rgba[swap_rb^2]*cR, SCALE );
            gray[i] = (uchar)t;
        }

        rgba += rgba_step - size.width*4;
    }
}


void icvCvt_Gray2BGR_8u_C1C3R( const uchar* gray, int gray_step,
                               uchar* bgr, int bgr_step, CvSize size )
{
    int i;
    for( ; size.height--; gray += gray_step )
    {
        for( i = 0; i < size.width; i++, bgr += 3 )
        {
            bgr[0] = bgr[1] = bgr[2] = gray[i];
        }
        bgr += bgr_step - size.width*3;
    }
}


void icvCvt_BGRA2BGR_8u_C4C3R( const uchar* bgra, int bgra_step,
                               uchar* bgr, int bgr_step,
                               CvSize size, int _swap_rb )
{
    int i;
    int swap_rb = _swap_rb ? 2 : 0;
    for( ; size.height--; )
    {
        for( i = 0; i < size.width; i++, bgr += 3, bgra += 4 )
        {
            uchar t0 = bgra[swap_rb], t1 = bgra[1];
            bgr[0] = t0; bgr[1] = t1;
            t0 = bgra[swap_rb^2]; bgr[2] = t0;
        }
        bgr += bgr_step - size.width*3;
        bgra += bgra_step - size.width*4;
    }
}


void icvCvt_BGRA2RGBA_8u_C4R( const uchar* bgra, int bgra_step,
                              uchar* rgba, int rgba_step, CvSize size )
{
    int i;
    for( ; size.height--; )
    {
        for( i = 0; i < size.width; i++, bgra += 4, rgba += 4 )
        {
            uchar t0 = bgra[0], t1 = bgra[1];
            uchar t2 = bgra[2], t3 = bgra[3];
            rgba[0] = t2; rgba[1] = t1;
            rgba[2] = t0; rgba[3] = t3;
        }
        bgra += bgra_step - size.width*4;
        rgba += rgba_step - size.width*4;
    }
}


void icvCvt_BGR2RGB_8u_C3R( const uchar* bgr, int bgr_step,
                            uchar* rgb, int rgb_step, CvSize size )
{
    int i;
    for( ; size.height--; )
    {
        for( i = 0; i < size.width; i++, bgr += 3, rgb += 3 )
        {
            uchar t0 = bgr[0], t1 = bgr[1], t2 = bgr[2];
            rgb[2] = t0; rgb[1] = t1; rgb[0] = t2;
        }
        bgr += bgr_step - size.width*3;
        rgb += rgb_step - size.width*3;
    }
}


typedef unsigned short ushort;

void icvCvt_BGR5552Gray_8u_C2C1R( const uchar* bgr555, int bgr555_step,
                                  uchar* gray, int gray_step, CvSize size )
{
    int i;
    for( ; size.height--; gray += gray_step, bgr555 += bgr555_step )
    {
        for( i = 0; i < size.width; i++ )
        {
            int t = descale( ((((ushort*)bgr555)[i] << 3) & 0xf8)*cB +
                             ((((ushort*)bgr555)[i] >> 2) & 0xf8)*cG +
                             ((((ushort*)bgr555)[i] >> 7) & 0xf8)*cR, SCALE );
            gray[i] = (uchar)t;
        }
    }
}


void icvCvt_BGR5652Gray_8u_C2C1R( const uchar* bgr565, int bgr565_step,
                                  uchar* gray, int gray_step, CvSize size )
{
    int i;
    for( ; size.height--; gray += gray_step, bgr565 += bgr565_step )
    {
        for( i = 0; i < size.width; i++ )
        {
            int t = descale( ((((ushort*)bgr565)[i] << 3) & 0xf8)*cB +
                             ((((ushort*)bgr565)[i] >> 3) & 0xfc)*cG +
                             ((((ushort*)bgr565)[i] >> 8) & 0xf8)*cR, SCALE );
            gray[i] = (uchar)t;
        }
    }
}


void icvCvt_BGR5552BGR_8u_C2C3R( const uchar* bgr555, int bgr555_step,
                                 uchar* bgr, int bgr_step, CvSize size )
{
    int i;
    for( ; size.height--; bgr555 += bgr555_step )
    {
        for( i = 0; i < size.width; i++, bgr += 3 )
        {
            int t0 = (((ushort*)bgr555)[i] << 3) & 0xf8;
            int t1 = (((ushort*)bgr555)[i] >> 2) & 0xf8;
            int t2 = (((ushort*)bgr555)[i] >> 7) & 0xf8;
            bgr[0] = (uchar)t0; bgr[1] = (uchar)t1; bgr[2] = (uchar)t2;
        }
        bgr += bgr_step - size.width*3;
    }
}


void icvCvt_BGR5652BGR_8u_C2C3R( const uchar* bgr565, int bgr565_step,
                                 uchar* bgr, int bgr_step, CvSize size )
{
    int i;
    for( ; size.height--; bgr565 += bgr565_step )
    {
        for( i = 0; i < size.width; i++, bgr += 3 )
        {
            int t0 = (((ushort*)bgr565)[i] << 3) & 0xf8;
            int t1 = (((ushort*)bgr565)[i] >> 3) & 0xfc;
            int t2 = (((ushort*)bgr565)[i] >> 8) & 0xf8;
            bgr[0] = (uchar)t0; bgr[1] = (uchar)t1; bgr[2] = (uchar)t2;
        }
        bgr += bgr_step - size.width*3;
    }
}


void icvCvt_CMYK2BGR_8u_C4C3R( const uchar* cmyk, int cmyk_step,
                               uchar* bgr, int bgr_step, CvSize size )
{
    int i;
    for( ; size.height--; )
    {
        for( i = 0; i < size.width; i++, bgr += 3, cmyk += 4 )
        {
            int c = cmyk[0], m = cmyk[1], y = cmyk[2], k = cmyk[3];
            c = k - ((255 - c)*k>>8);
            m = k - ((255 - m)*k>>8);
            y = k - ((255 - y)*k>>8);
            bgr[2] = (uchar)c; bgr[1] = (uchar)m; bgr[0] = (uchar)y;
        }
        bgr += bgr_step - size.width*3;
        cmyk += cmyk_step - size.width*4;
    }
}


void icvCvt_CMYK2Gray_8u_C4C1R( const uchar* cmyk, int cmyk_step,
                                uchar* gray, int gray_step, CvSize size )
{
    int i;
    for( ; size.height--; )
    {
        for( i = 0; i < size.width; i++, cmyk += 4 )
        {
            int c = cmyk[0], m = cmyk[1], y = cmyk[2], k = cmyk[3];
            c = k - ((255 - c)*k>>8);
            m = k - ((255 - m)*k>>8);
            y = k - ((255 - y)*k>>8);
            int t = descale( y*cB + m*cG + c*cR, SCALE );
            gray[i] = (uchar)t;
        }
        gray += gray_step;
        cmyk += cmyk_step - size.width*4;
    }
}


void CvtPaletteToGray( const PaletteEntry* palette, uchar* grayPalette, int entries )
{
    int i;
    for( i = 0; i < entries; i++ )
    {
        icvCvt_BGR2Gray_8u_C3C1R( (uchar*)(palette + i), 0, grayPalette + i, 0, cvSize(1,1) );
    }
}


void  FillGrayPalette( PaletteEntry* palette, int bpp, bool negative )
{
    int i, length = 1 << bpp;
    int xor_mask = negative ? 255 : 0;

    for( i = 0; i < length; i++ )
    {
        int val = (i * 255/(length - 1)) ^ xor_mask;
        palette[i].b = palette[i].g = palette[i].r = (uchar)val;
        palette[i].a = 0;
    }
}


bool  IsColorPalette( PaletteEntry* palette, int bpp )
{
    int i, length = 1 << bpp;

    for( i = 0; i < length; i++ )
    {
        if( palette[i].b != palette[i].g ||
            palette[i].b != palette[i].r )
            return true;
    }

    return false;
}


uchar* FillUniColor( uchar* data, uchar*& line_end,
                     int step, int width3,
                     int& y, int height,
                     int count3, PaletteEntry clr )
{
    do
    {
        uchar* end = data + count3;

        if( end > line_end )
            end = line_end;

        count3 -= (int)(end - data);
        
        for( ; data < end; data += 3 )
        {
            WRITE_PIX( data, clr );
        }

        if( data >= line_end )
        {
            line_end += step;
            data = line_end - width3;
            if( ++y >= height  ) break;
        }
    }
    while( count3 > 0 );

    return data;
}


uchar* FillUniGray( uchar* data, uchar*& line_end,
                    int step, int width,
                    int& y, int height,
                    int count, uchar clr )
{
    do
    {
        uchar* end = data + count;

        if( end > line_end )
            end = line_end;

        count -= (int)(end - data);
        
        for( ; data < end; data++ )
        {
            *data = clr;
        }

        if( data >= line_end )
        {
            line_end += step;
            data = line_end - width;
            if( ++y >= height  ) break;
        }
    }
    while( count > 0 );

    return data;
}


uchar* FillColorRow8( uchar* data, uchar* indices, int len, PaletteEntry* palette )
{
    uchar* end = data + len*3;
    while( (data += 3) < end )
    {
        *((PaletteEntry*)(data-3)) = palette[*indices++];
    }
    PaletteEntry clr = palette[indices[0]];
    WRITE_PIX( data - 3, clr );
    return data;
}
                       

uchar* FillGrayRow8( uchar* data, uchar* indices, int len, uchar* palette )
{
    int i;
    for( i = 0; i < len; i++ )
    {
        data[i] = palette[indices[i]];
    }
    return data + len;
}


uchar* FillColorRow4( uchar* data, uchar* indices, int len, PaletteEntry* palette )
{
    uchar* end = data + len*3;

    while( (data += 6) < end )
    {
        int idx = *indices++;
        *((PaletteEntry*)(data-6)) = palette[idx >> 4];
        *((PaletteEntry*)(data-3)) = palette[idx & 15];
    }

    int idx = indices[0];
    PaletteEntry clr = palette[idx >> 4];
    WRITE_PIX( data - 6, clr );

    if( data == end )
    {
        clr = palette[idx & 15];
        WRITE_PIX( data - 3, clr );
    }
    return end;
}


uchar* FillGrayRow4( uchar* data, uchar* indices, int len, uchar* palette )
{
    uchar* end = data + len;
    while( (data += 2) < end )
    {
        int idx = *indices++;
        data[-2] = palette[idx >> 4];
        data[-1] = palette[idx & 15];
    }

    int idx = indices[0];
    uchar clr = palette[idx >> 4];
    data[-2] = clr;

    if( data == end )
    {
        clr = palette[idx & 15];
        data[-1] = clr;
    }
    return end;
}


uchar* FillColorRow1( uchar* data, uchar* indices, int len, PaletteEntry* palette )
{
    uchar* end = data + len*3;

    while( (data += 24) < end )
    {
        int idx = *indices++;
        *((PaletteEntry*)(data - 24)) = palette[(idx & 128) != 0];
        *((PaletteEntry*)(data - 21)) = palette[(idx & 64) != 0];
        *((PaletteEntry*)(data - 18)) = palette[(idx & 32) != 0];
        *((PaletteEntry*)(data - 15)) = palette[(idx & 16) != 0];
        *((PaletteEntry*)(data - 12)) = palette[(idx & 8) != 0];
        *((PaletteEntry*)(data - 9)) = palette[(idx & 4) != 0];
        *((PaletteEntry*)(data - 6)) = palette[(idx & 2) != 0];
        *((PaletteEntry*)(data - 3)) = palette[(idx & 1) != 0];
    }
    
    int idx = indices[0] << 24;
    for( data -= 24; data < end; data += 3, idx += idx )
    {
        PaletteEntry clr = palette[idx < 0];
        WRITE_PIX( data, clr );
    }

    return data;
}


uchar* FillGrayRow1( uchar* data, uchar* indices, int len, uchar* palette )
{
    uchar* end = data + len;

    while( (data += 8) < end )
    {
        int idx = *indices++;
        *((uchar*)(data - 8)) = palette[(idx & 128) != 0];
        *((uchar*)(data - 7)) = palette[(idx & 64) != 0];
        *((uchar*)(data - 6)) = palette[(idx & 32) != 0];
        *((uchar*)(data - 5)) = palette[(idx & 16) != 0];
        *((uchar*)(data - 4)) = palette[(idx & 8) != 0];
        *((uchar*)(data - 3)) = palette[(idx & 4) != 0];
        *((uchar*)(data - 2)) = palette[(idx & 2) != 0];
        *((uchar*)(data - 1)) = palette[(idx & 1) != 0];
    }
    
    int idx = indices[0] << 24;
    for( data -= 8; data < end; data++, idx += idx )
    {
        data[0] = palette[idx < 0];
    }

    return data;
}


CV_IMPL void
cvConvertImage( const CvArr* srcarr, CvArr* dstarr, int flags )
{
    CvMat* temp = 0;
    
    CV_FUNCNAME( "cvConvertImage" );
    
    __BEGIN__;

    CvMat srcstub, *src;
    CvMat dststub, *dst;
    int src_cn, swap_rb = flags & CV_CVTIMG_SWAP_RB;

    CV_CALL( src = cvGetMat( srcarr, &srcstub ));
    CV_CALL( dst = cvGetMat( dstarr, &dststub ));

    src_cn = CV_MAT_CN( src->type );

    if( src_cn != 1 && src_cn != 3 && src_cn != 4 )
        CV_ERROR( CV_BadNumChannels, "Source image must have 1, 3 or 4 channels" );

    if( CV_MAT_DEPTH( dst->type ) != CV_8U )
        CV_ERROR( CV_BadDepth, "Destination image must be 8u" );

    if( CV_MAT_CN(dst->type) != 1 && CV_MAT_CN(dst->type) != 3 )
        CV_ERROR( CV_BadNumChannels, "Destination image must have 1 or 3 channels" );
    
    if( !CV_ARE_DEPTHS_EQ( src, dst ))
    {
        double scale = 0, shift = 0;
        int src_depth = CV_MAT_DEPTH(src->type);
        temp = cvCreateMat( src->height, src->width,
                            (src->type & CV_MAT_CN_MASK)|(dst->type & CV_MAT_DEPTH_MASK));
        scale = src_depth <= CV_8S ? 1 : src_depth <= CV_32S ? 1./256 : 255;
        shift = src_depth == CV_8S || src_depth == CV_16S ? 128 : 0;
        cvConvertScale( src, temp, scale, shift );
        src = temp;
    }

    {
        uchar *s = src->data.ptr, *d = dst->data.ptr;
        int s_step = src->step, d_step = dst->step;
        int code = src_cn*10 + CV_MAT_CN(dst->type);
        CvSize size = { src->cols, src->rows };

        if( CV_IS_MAT_CONT(src->type & dst->type) )
        {
            size.width *= size.height;
            size.height = 1;
            s_step = d_step = CV_STUB_STEP;
        }
        
        switch( code )
        {
        case 13:
            icvCvt_Gray2BGR_8u_C1C3R( s, s_step, d, d_step, size );
            break;
        case 31:
            icvCvt_BGR2Gray_8u_C3C1R( s, s_step, d, d_step, size, swap_rb );
            break;
        case 33:
            if( swap_rb )
                icvCvt_RGB2BGR_8u_C3R( s, s_step, d, d_step, size );
            break;
        case 41:
            icvCvt_BGRA2Gray_8u_C4C1R( s, s_step, d, d_step, size, swap_rb );
            break;
        case 43:
            icvCvt_BGRA2BGR_8u_C4C3R( s, s_step, d, d_step, size, swap_rb );
            break;
        default:
            CV_ERROR( CV_StsUnsupportedFormat, "Unsupported combination of input/output formats" );
        }

        if( code != 33 || swap_rb )
            src = dst;
    }

    if( flags & CV_CVTIMG_FLIP )
    {
        CV_CALL( cvFlip( src, dst, 0 ));
    }
    else if( src != dst )
    {
        CV_CALL( cvCopy( src, dst ));
    }

    __END__;

    cvReleaseMat( &temp );
}
