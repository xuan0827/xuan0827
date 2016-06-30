

#ifndef _GRFMT_BASE_H_
#define _GRFMT_BASE_H_

#if _MSC_VER >= 1200
    #pragma warning( disable: 4514 )
    #pragma warning( disable: 4711 )
    #pragma warning( disable: 4611 )
#endif

#include "utils.h"
#include "mmplab_highgui.h"

#define  RBS_BAD_HEADER     -125  /* invalid image header */
#define  BAD_HEADER_ERR()   goto bad_header_exit

#ifndef _MAX_PATH
    #define _MAX_PATH    1024
#endif


///////////////////////////////// base class for readers ////////////////////////
class   GrFmtReader
{
public:
    
    GrFmtReader( const char* filename );
    virtual ~GrFmtReader();

    int   GetWidth()  { return m_width; };
    int   GetHeight() { return m_height; };
    bool  IsColor()   { return m_iscolor; };
    int   GetDepth()  { return m_bit_depth; };
    void  UseNativeDepth( bool yes ) { m_native_depth = yes; };
    bool  IsFloat()   { return m_isfloat; };

    virtual bool  ReadHeader() = 0;
    virtual bool  ReadData( uchar* data, int step, int color ) = 0;
    virtual void  Close();

protected:

    bool    m_iscolor;
    int     m_width;    // width  of the image ( filled by ReadHeader )
    int     m_height;   // height of the image ( filled by ReadHeader )
    int     m_bit_depth;// bit depth per channel (normally 8)
    char    m_filename[_MAX_PATH]; // filename
    bool    m_native_depth;// use the native bit depth of the image
    bool    m_isfloat;  // is image saved as float or double?
};


///////////////////////////// base class for writers ////////////////////////////
class   GrFmtWriter
{
public:

    GrFmtWriter( const char* filename );
    virtual ~GrFmtWriter() {};
    virtual bool  IsFormatSupported( int depth );
    virtual bool  WriteImage( const uchar* data, int step,
                              int width, int height, int depth, int channels ) = 0;
protected:
    char    m_filename[_MAX_PATH]; // filename
};


////////////////////////////// base class for filter factories //////////////////
class   GrFmtFilterFactory
{
public:

    GrFmtFilterFactory();
    virtual ~GrFmtFilterFactory() {};

    const char*  GetDescription() { return m_description; };
    int     GetSignatureLength()  { return m_sign_len; };
    virtual bool CheckSignature( const char* signature );
    virtual bool CheckExtension( const char* filename );
    virtual GrFmtReader* NewReader( const char* filename ) = 0;
    virtual GrFmtWriter* NewWriter( const char* filename ) = 0;

protected:
    const char* m_description;
           // graphic format description in form:
           // <Some textual description>( *.<extension1> [; *.<extension2> ...]).
           // the textual description can not contain symbols '(', ')'
           // and may be, some others. It is safe to use letters, digits and spaces only.
           // e.g. "Targa (*.tga)",
           // or "Portable Graphic Format (*.pbm;*.pgm;*.ppm)"

    int          m_sign_len;    // length of the signature of the format
    const char*  m_signature;   // signature of the format
};


/////////////////////////// list of graphic format filters ///////////////////////////////

typedef void* ListPosition;

class   GrFmtFactoriesList
{
public:

    GrFmtFactoriesList();
    virtual ~GrFmtFactoriesList();
    void  RemoveAll();
    bool  AddFactory( GrFmtFilterFactory* factory );
    int   FactoriesCount() { return m_curFactories; };
    ListPosition  GetFirstFactoryPos();
    GrFmtFilterFactory*  GetNextFactory( ListPosition& pos );
    virtual GrFmtReader*  FindReader( const char* filename );
    virtual GrFmtWriter*  FindWriter( const char* filename );

protected:

    GrFmtFilterFactory** m_factories;
    int  m_maxFactories;
    int  m_curFactories;
};

#endif/*_GRFMT_BASE_H_*/
/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef _GRFMTS_H_
#define _GRFMTS_H_





#endif/*_GRFMTS_H_*/

#ifndef _GRFMT_BMP_H_
#define _GRFMT_BMP_H_


enum BmpCompression
{
    BMP_RGB = 0,
		BMP_RLE8 = 1,
		BMP_RLE4 = 2,
		BMP_BITFIELDS = 3
};


// Windows Bitmap reader
class GrFmtBmpReader : public GrFmtReader
{
public:
    
    GrFmtBmpReader( const char* filename );
    ~GrFmtBmpReader();
    
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();
	
protected:
    
    RLByteStream    m_strm;
    PaletteEntry    m_palette[256];
    int             m_bpp;
    int             m_offset;
    BmpCompression  m_rle_code;
};


// ... writer
class GrFmtBmpWriter : public GrFmtWriter
{
public:
    
    GrFmtBmpWriter( const char* filename );
    ~GrFmtBmpWriter();
    
    bool  WriteImage( const uchar* data, int step,
		int width, int height, int depth, int channels );
protected:
	
    WLByteStream  m_strm;
};


// ... and filter factory
class GrFmtBmp : public GrFmtFilterFactory
{
public:
    
    GrFmtBmp();
    ~GrFmtBmp();
	
    GrFmtReader* NewReader( const char* filename );
    GrFmtWriter* NewWriter( const char* filename );

};

#endif/*_GRFMT_BMP_H_*/


#ifndef _GRFMT_EXR_H_
#define _GRFMT_EXR_H_

#ifdef HAVE_ILMIMF

#include <ImfChromaticities.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImathBox.h>


using namespace Imf;
using namespace Imath;

/* libpng version only */

class GrFmtExrReader : public GrFmtReader
{
public:
	
    GrFmtExrReader( const char* filename );
    ~GrFmtExrReader();
	
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();
	
protected:
    void  UpSample( uchar *data, int xstep, int ystep, int xsample, int ysample );
    void  UpSampleX( float *data, int xstep, int xsample );
    void  UpSampleY( uchar *data, int xstep, int ystep, int ysample );
    void  ChromaToBGR( float *data, int numlines, int step );
    void  RGBToGray( float *in, float *out );
	
    InputFile      *m_file;
    PixelType       m_type;
    Box2i           m_datawindow;
    bool            m_ischroma;
    const Channel  *m_red;
    const Channel  *m_green;
    const Channel  *m_blue;
    Chromaticities  m_chroma;
};


class GrFmtExrWriter : public GrFmtWriter
{
public:
	
    GrFmtExrWriter( const char* filename );
    ~GrFmtExrWriter();
	
    bool  IsFormatSupported( int depth );
    bool  WriteImage( const uchar* data, int step,
		int width, int height, int depth, int channels );
protected:
};


// Exr filter factory
class GrFmtExr : public GrFmtFilterFactory
{
public:
	
    GrFmtExr();
    ~GrFmtExr();
	
    GrFmtReader* NewReader( const char* filename );
    GrFmtWriter* NewWriter( const char* filename );
	//    bool CheckSignature( const char* signature );
};

#endif

#endif/*_GRFMT_EXR_H_*/
#ifndef _GRFMT_JPEG_H_
#define _GRFMT_JPEG_H_



#ifdef HAVE_JPEG

/* IJG-based version */

class GrFmtJpegReader : public GrFmtReader
{
public:
    
    GrFmtJpegReader( const char* filename );
    ~GrFmtJpegReader();

    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();

protected:

    void* m_cinfo; // pointer to IJG JPEG codec structure
    void* m_jerr; // pointer to error processing manager state
    FILE* m_f;
};


class GrFmtJpegWriter : public GrFmtWriter
{
public:
    
    GrFmtJpegWriter( const char* filename );
    ~GrFmtJpegWriter();

    bool  WriteImage( const uchar* data, int step,
                      int width, int height, int depth, int channels );
};

#else

/* hand-crafted implementation */

class RJpegBitStream : public RMBitStream
{
public:
    RMByteStream  m_low_strm;
    
    RJpegBitStream();
    ~RJpegBitStream();

    virtual bool  Open( const char* filename );
    virtual void  Close();

    void  Flush(); // flushes high-level bit stream
    void  AlignOnByte();
    int   FindMarker();

protected:
    virtual void  ReadBlock();
};


//////////////////// JPEG reader /////////////////////

class GrFmtJpegReader : public GrFmtReader
{
public:
    
    GrFmtJpegReader( const char* filename );
    ~GrFmtJpegReader();

    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();

protected:

    int   m_offset; // offset of first scan
    int   m_version; // JFIF version
    int   m_planes; // 3 (YCrCb) or 1 (Gray)
    int   m_precision; // 8 or 12-bit per sample
    int   m_type; // SOF type
    int   m_MCUs; // MCUs in restart interval
    int   m_ss, m_se, m_ah, m_al; // progressive JPEG parameters
    
    // information about each component
    struct cmp_info
    {
        char h;  // horizontal sampling factor
        char v;  // vertical   sampling factor
        char tq; // quantization table index
        char td, ta; // DC & AC huffman tables
        int  dc_pred; // DC predictor
    };
    
    cmp_info m_ci[3];

    int     m_tq[4][64];
    bool    m_is_tq[4];
    
    short*  m_td[4];
    bool    m_is_td[4];
    
    short*  m_ta[4];
    bool    m_is_ta[4];
    
    RJpegBitStream  m_strm;

protected:

    bool  LoadQuantTables( int length );
    bool  LoadHuffmanTables( int length );
    void  ProcessScan( int* idx, int ns, uchar* data, int step, int color );
    void  ResetDecoder();
    void  GetBlock( int* block, int c );
};


//////////////////// JPEG-specific output bitstream ///////////////////////

class WJpegBitStream : public WMBitStream
{
public:
    WMByteStream  m_low_strm;
    
    WJpegBitStream();
    ~WJpegBitStream();

    virtual void  Flush();
    virtual bool  Open( const char* filename );
    virtual void  Close();

protected:
    virtual void  WriteBlock();
};


//////////////////// JPEG reader /////////////////////

class GrFmtJpegWriter : public GrFmtWriter
{
public:
    
    GrFmtJpegWriter( const char* filename );
    ~GrFmtJpegWriter();

    bool  WriteImage( const uchar* data, int step,
                      int width, int height, int depth, int channels );

protected:

    WJpegBitStream  m_strm;
};

#endif /* HAVE_JPEG */


// JPEG filter factory
class GrFmtJpeg : public GrFmtFilterFactory
{
public:
    
    GrFmtJpeg();
    ~GrFmtJpeg();

    GrFmtReader* NewReader( const char* filename );
    GrFmtWriter* NewWriter( const char* filename );
};


#endif/*_GRFMT_JPEG_H_*/


#ifndef _GRFMT_PNG_H_
#define _GRFMT_PNG_H_

#ifdef HAVE_PNG



/* libpng version only */

class GrFmtPngReader : public GrFmtReader
{
public:
    
    GrFmtPngReader( const char* filename );
    ~GrFmtPngReader();
	
    bool  CheckFormat( const char* signature );
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();
	
protected:
	
    void* m_png_ptr;  // pointer to decompression structure
    void* m_info_ptr; // pointer to image information structure
    void* m_end_info; // pointer to one more image information structure
    FILE* m_f;
    int   m_color_type;
};


class GrFmtPngWriter : public GrFmtWriter
{
public:
    
    GrFmtPngWriter( const char* filename );
    ~GrFmtPngWriter();
	
    bool  IsFormatSupported( int depth );
    bool  WriteImage( const uchar* data, int step,
		int width, int height, int depth, int channels );
protected:
};


// PNG filter factory
class GrFmtPng : public GrFmtFilterFactory
{
public:
    
    GrFmtPng();
    ~GrFmtPng();
	
    GrFmtReader* NewReader( const char* filename );
    GrFmtWriter* NewWriter( const char* filename );
    bool CheckSignature( const char* signature );
};

#endif

#endif/*_GRFMT_PNG_H_*/


#ifndef _GRFMT_PxM_H_
#define _GRFMT_PxM_H_



class GrFmtPxMReader : public GrFmtReader
{
public:
    
    GrFmtPxMReader( const char* filename );
    ~GrFmtPxMReader();
    
    bool  CheckFormat( const char* signature );
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();
	
protected:
    
    RLByteStream    m_strm;
    PaletteEntry    m_palette[256];
    int             m_bpp;
    int             m_offset;
    bool            m_binary;
    int             m_maxval;
};


class GrFmtPxMWriter : public GrFmtWriter
{
public:
    
    GrFmtPxMWriter( const char* filename );
    ~GrFmtPxMWriter();
	
    bool  WriteImage( const uchar* data, int step,
		int width, int height, int depth, int channels );
protected:
	
    WLByteStream  m_strm;
};


// PxM filter factory
class GrFmtPxM : public GrFmtFilterFactory
{
public:
    
    GrFmtPxM();
    ~GrFmtPxM();
	
    GrFmtReader* NewReader( const char* filename );
    GrFmtWriter* NewWriter( const char* filename );
    bool CheckSignature( const char* signature );
};


#endif/*_GRFMT_PxM_H_*/

#ifndef _GRFMT_SUNRAS_H_
#define _GRFMT_SUNRAS_H_

#include "mmplab_grfmt.h"

enum SunRasType
{
    RAS_OLD = 0,
		RAS_STANDARD = 1,
		RAS_BYTE_ENCODED = 2, /* RLE encoded */
		RAS_FORMAT_RGB = 3    /* RGB instead of BGR */
};

enum SunRasMapType
{
    RMT_NONE = 0,       /* direct color encoding */
		RMT_EQUAL_RGB = 1   /* paletted image */
};


// Sun Raster Reader
class GrFmtSunRasterReader : public GrFmtReader
{
public:
	
    GrFmtSunRasterReader( const char* filename );
    ~GrFmtSunRasterReader();
	
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();
	
protected:
    
    RMByteStream    m_strm;
    PaletteEntry    m_palette[256];
    int             m_bpp;
    int             m_offset;
    SunRasType      m_type;
    SunRasMapType   m_maptype;
    int             m_maplength;
};


class GrFmtSunRasterWriter : public GrFmtWriter
{
public:
    
    GrFmtSunRasterWriter( const char* filename );
    ~GrFmtSunRasterWriter();
	
    bool  WriteImage( const uchar* data, int step,
		int width, int height, int depth, int channels );
protected:
	
    WMByteStream  m_strm;
};


// ... and filter factory
class GrFmtSunRaster : public GrFmtFilterFactory
{
public:
    
    GrFmtSunRaster();
    ~GrFmtSunRaster();
	
    GrFmtReader* NewReader( const char* filename );
    GrFmtWriter* NewWriter( const char* filename );
};

#endif/*_GRFMT_SUNRAS_H_*/

#ifndef _GRFMT_TIFF_H_
#define _GRFMT_TIFF_H_



// native simple TIFF codec
enum TiffCompression
{
    TIFF_UNCOMP = 1,
    TIFF_HUFFMAN = 2,
    TIFF_PACKBITS = 32773
};

enum TiffByteOrder
{
    TIFF_ORDER_II = 0x4949,
    TIFF_ORDER_MM = 0x4d4d
};


enum  TiffTag
{
    TIFF_TAG_WIDTH  = 256,
    TIFF_TAG_HEIGHT = 257,
    TIFF_TAG_BITS_PER_SAMPLE = 258,
    TIFF_TAG_COMPRESSION = 259,
    TIFF_TAG_PHOTOMETRIC = 262,
    TIFF_TAG_STRIP_OFFSETS = 273,
    TIFF_TAG_STRIP_COUNTS = 279,
    TIFF_TAG_SAMPLES_PER_PIXEL = 277,
    TIFF_TAG_ROWS_PER_STRIP = 278,
    TIFF_TAG_PLANAR_CONFIG = 284,
    TIFF_TAG_COLOR_MAP = 320
};


enum TiffFieldType
{
    TIFF_TYPE_BYTE = 1,
    TIFF_TYPE_SHORT = 3,
    TIFF_TYPE_LONG = 4
};



#ifdef HAVE_TIFF

// libtiff based TIFF codec

class GrFmtTiffReader : public GrFmtReader
{
public:
    
    GrFmtTiffReader( const char* filename );
    ~GrFmtTiffReader();

    bool  CheckFormat( const char* signature );
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();

protected:

    void* m_tif;
};


#else

class GrFmtTiffReader : public GrFmtReader
{
public:
    
    GrFmtTiffReader( const char* filename );
    ~GrFmtTiffReader();

    bool  CheckFormat( const char* signature );
    bool  ReadData( uchar* data, int step, int color );
    bool  ReadHeader();
    void  Close();

protected:
    
    RLByteStream     m_strm;
    PaletteEntry     m_palette[256];
    int              m_bpp;
    int*             m_temp_palette;
    int              m_max_pal_length;
    int*             m_offsets;
    int              m_maxoffsets;
    int              m_strips;
    int              m_rows_per_strip;
    TiffCompression  m_compression;
    TiffByteOrder    m_byteorder;

    int  GetWordEx();
    int  GetDWordEx();
    int  ReadTable( int offset, int count, TiffFieldType fieldtype,
                    int*& array, int& arraysize );
};

#endif

// ... and writer
class GrFmtTiffWriter : public GrFmtWriter
{
public:
    
    GrFmtTiffWriter( const char* filename );
    ~GrFmtTiffWriter();

    bool  WriteImage( const uchar* data, int step,
                      int width, int height, int depth, int channels );
protected:

    WLByteStream  m_strm;

    void  WriteTag( TiffTag tag, TiffFieldType fieldType,
                    int count, int value );
};


// TIFF filter factory
class GrFmtTiff : public GrFmtFilterFactory
{
public:
    
    GrFmtTiff();
    ~GrFmtTiff();

    GrFmtReader* NewReader( const char* filename );
    GrFmtWriter* NewWriter( const char* filename );
    bool CheckSignature( const char* signature );
};

#endif/*_GRFMT_TIFF_H_*/
