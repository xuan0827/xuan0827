
// 
#include "stdafx.h"
#include "mmplab_highgui.h"
#include "mmplab_grfmt.h"

// 
// 
GrFmtReader::GrFmtReader( const char* filename )
{
    strncpy( m_filename, filename, sizeof(m_filename) - 1 );
    m_filename[sizeof(m_filename)-1] = '\0';
    m_width = m_height = 0;
    m_iscolor = false;
    m_bit_depth = 8;
    m_native_depth = false;
    m_isfloat = false;
}
// 
// 
GrFmtReader::~GrFmtReader()
{
    Close();
}
// 
// 
void GrFmtReader::Close()
{
    m_width = m_height = 0;
    m_iscolor = false;
}
// 
// 
GrFmtWriter::GrFmtWriter( const char* filename )
{
    strncpy( m_filename, filename, sizeof(m_filename) - 1 );
    m_filename[sizeof(m_filename)-1] = '\0';
}
// 
// 
bool  GrFmtWriter::IsFormatSupported( int depth )
{
    return depth == IPL_DEPTH_8U;
}
// 
// 
GrFmtFilterFactory::GrFmtFilterFactory()
{
    m_description = m_signature = 0;
    m_sign_len = 0;
}


bool GrFmtFilterFactory::CheckSignature( const char* signature )
{
    return m_sign_len > 0 && signature != 0 &&
           memcmp( signature, m_signature, m_sign_len ) == 0;
}


static int GetExtensionLength( const char* buffer )
{
    int len = 0;

    if( buffer )
    {
        const char* ext = strchr( buffer, '.');
        if( ext++ )
            while( isalnum(ext[len]) && len < _MAX_PATH )
                len++;
    }

    return len;
}
// 
// 
bool GrFmtFilterFactory::CheckExtension( const char* format )
{
    const char* descr = 0;
    int len = 0;
        
    if( !format || !m_description )
        return false;

    // find the right-most extension of the passed format string
    for(;;)
    {
        const char* ext = strchr( format + 1, '.' );
        if( !ext ) break;
        format = ext;
    }

    len = GetExtensionLength( format );

    if( format[0] != '.' || len == 0 )
        return false;

    descr = strchr( m_description, '(' );

    while( descr )
    {
        descr = strchr( descr + 1, '.' );
        int i, len2 = GetExtensionLength( descr ); 

        if( len2 == 0 )
            break;

        if( len2 == len )
        {
            for( i = 0; i < len; i++ )
            {
                int c1 = tolower(format[i+1]);
                int c2 = tolower(descr[i+1]);

                if( c1 != c2 )
                    break;
            }
            if( i == len )
                return true;
        }
    }

    return false;
}

// 
// 
// ///////////////////// GrFmtFilterList //////////////////////////
// 
GrFmtFactoriesList::GrFmtFactoriesList()
{
    m_factories = 0;
    RemoveAll();
}
// 

GrFmtFactoriesList::~GrFmtFactoriesList()
{
    RemoveAll();
}


void  GrFmtFactoriesList::RemoveAll()
{
    if( m_factories )
    {
        for( int i = 0; i < m_curFactories; i++ ) delete m_factories[i];
        delete[] m_factories;
    }
    m_factories = 0;
    m_maxFactories = m_curFactories = 0;
}
// 
// 
bool  GrFmtFactoriesList::AddFactory( GrFmtFilterFactory* factory )
{
    assert( factory != 0 );
    if( m_curFactories == m_maxFactories )
    {
        // reallocate the factorys pointers storage
        int newMaxFactories = 2*m_maxFactories;
        if( newMaxFactories < 16 ) newMaxFactories = 16;

        GrFmtFilterFactory** newFactories = new GrFmtFilterFactory*[newMaxFactories];

        for( int i = 0; i < m_curFactories; i++ ) newFactories[i] = m_factories[i];

        delete[] m_factories;
        m_factories = newFactories;
        m_maxFactories = newMaxFactories;
    }

    m_factories[m_curFactories++] = factory;
    return true;
}
// 
// 
ListPosition  GrFmtFactoriesList::GetFirstFactoryPos()
{
    return (ListPosition)m_factories;
}
// 
// 
GrFmtFilterFactory* GrFmtFactoriesList::GetNextFactory( ListPosition& pos )
{
    GrFmtFilterFactory* factory = 0;
    GrFmtFilterFactory** temp = (GrFmtFilterFactory**)pos;

    assert( temp == 0 || (m_factories <= temp && temp < m_factories + m_curFactories));
    if( temp )
    {
        factory = *temp++;
        pos = (ListPosition)(temp < m_factories + m_curFactories ? temp : 0);
    }
    return factory;
}
// 
// 
GrFmtReader* GrFmtFactoriesList::FindReader( const char* filename )
{
    FILE*  f = 0;
    char   signature[4096];
    int    sign_len = 0;
    GrFmtReader* reader = 0;
    ListPosition pos = GetFirstFactoryPos();

    if( !filename ) return 0;

    while( pos )
    {
        GrFmtFilterFactory* tempFactory = GetNextFactory(pos);
        int cur_sign_len = tempFactory->GetSignatureLength();
        if( sign_len < cur_sign_len ) sign_len = cur_sign_len;
    }

    assert( sign_len <= (int)sizeof(signature) );
    f = fopen( filename, "rb" );

    if( f )
    {
        sign_len = (int)fread( signature, 1, sign_len, f );
        fclose(f);

        pos = GetFirstFactoryPos();
        while( pos )
        {
            GrFmtFilterFactory* tempFactory = GetNextFactory(pos);
            int cur_sign_len = tempFactory->GetSignatureLength();
            if( cur_sign_len <= sign_len && tempFactory->CheckSignature(signature))
            {
                reader = tempFactory->NewReader( filename );
                break;
            }
        }
    }

    return reader;
}
// 
// 
GrFmtWriter* GrFmtFactoriesList::FindWriter( const char* filename )
{
    GrFmtWriter* writer = 0;
    ListPosition pos = GetFirstFactoryPos();

    if( !filename ) return 0;

    while( pos )
    {
        GrFmtFilterFactory* tempFactory = GetNextFactory(pos);
        if( tempFactory->CheckExtension( filename ))
        {
            writer = tempFactory->NewWriter( filename );
            break;
        }
    }

    return writer;
}
// 
// /* End of file. */
// 


// 
static const char* fmtSignBmp = "BM";

GrFmtBmp::GrFmtBmp()
{
    m_sign_len = 2;
    m_signature = fmtSignBmp;
    m_description = "Windows bitmap (*.bmp;*.dib)";
}

// 
GrFmtBmp::~GrFmtBmp()
{
}
// 
// 
GrFmtReader* GrFmtBmp::NewReader( const char* filename )
{
    return new GrFmtBmpReader( filename );
}

// 
GrFmtWriter* GrFmtBmp::NewWriter( const char* filename )
{
    return new GrFmtBmpWriter( filename );
}
// 
// 
// /************************ BMP reader *****************************/
// 
GrFmtBmpReader::GrFmtBmpReader( const char* filename ) : GrFmtReader( filename )
{
    m_offset = -1;
}


GrFmtBmpReader::~GrFmtBmpReader()
{
}
// 
// 
void  GrFmtBmpReader::Close()
{
    m_strm.Close();
    GrFmtReader::Close();
}
// 
// 
bool  GrFmtBmpReader::ReadHeader()
{
    bool result = false;
    
    assert( strlen(m_filename) != 0 );
    if( !m_strm.Open( m_filename )) return false;

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_strm.Skip( 10 );
        m_offset = m_strm.GetDWord();

        int  size = m_strm.GetDWord();

        if( size >= 36 )
        {
            m_width  = m_strm.GetDWord();
            m_height = m_strm.GetDWord();
            m_bpp    = m_strm.GetDWord() >> 16;
            m_rle_code = (BmpCompression)m_strm.GetDWord();
            m_strm.Skip(12);
            int clrused = m_strm.GetDWord();
            m_strm.Skip( size - 36 );

            if( m_width > 0 && m_height > 0 &&
             (((m_bpp == 1 || m_bpp == 4 || m_bpp == 8 ||
                m_bpp == 24 || m_bpp == 32 ) && m_rle_code == BMP_RGB) ||
               (m_bpp == 16 && (m_rle_code == BMP_RGB || m_rle_code == BMP_BITFIELDS)) ||
               (m_bpp == 4 && m_rle_code == BMP_RLE4) ||
               (m_bpp == 8 && m_rle_code == BMP_RLE8))) 
            {
                m_iscolor = true;
                result = true;

                if( m_bpp <= 8 )
                {
                    memset( m_palette, 0, sizeof(m_palette));
                    m_strm.GetBytes( m_palette, (clrused == 0? 1<<m_bpp : clrused)*4 );
                    m_iscolor = IsColorPalette( m_palette, m_bpp );
                }
                else if( m_bpp == 16 && m_rle_code == BMP_BITFIELDS )
                {
                    int redmask = m_strm.GetDWord();
                    int greenmask = m_strm.GetDWord();
                    int bluemask = m_strm.GetDWord();

                    if( bluemask == 0x1f && greenmask == 0x3e0 && redmask == 0x7c00 )
                        m_bpp = 15;
                    else if( bluemask == 0x1f && greenmask == 0x7e0 && redmask == 0xf800 )
                        ;
                    else
                        result = false;
                }
                else if( m_bpp == 16 && m_rle_code == BMP_RGB )
                    m_bpp = 15;
            }
        }
        else if( size == 12 )
        {
            m_width  = m_strm.GetWord();
            m_height = m_strm.GetWord();
            m_bpp    = m_strm.GetDWord() >> 16;
            m_rle_code = BMP_RGB;

            if( m_width > 0 && m_height > 0 &&
               (m_bpp == 1 || m_bpp == 4 || m_bpp == 8 ||
                m_bpp == 24 || m_bpp == 32 )) 
            {
                if( m_bpp <= 8 )
                {
                    uchar buffer[256*3];
                    int j, clrused = 1 << m_bpp;
                    m_strm.GetBytes( buffer, clrused*3 );
                    for( j = 0; j < clrused; j++ )
                    {
                        m_palette[j].b = buffer[3*j+0];
                        m_palette[j].g = buffer[3*j+1];
                        m_palette[j].r = buffer[3*j+2];
                    }
                }
                result = true;
            }
        }
    }

    if( !result )
    {
        m_offset = -1;
        m_width = m_height = -1;
        m_strm.Close();
    }
    return result;
}
// 
// 
bool  GrFmtBmpReader::ReadData( uchar* data, int step, int color )
{
    const  int buffer_size = 1 << 12;
    uchar  buffer[buffer_size];
    uchar  bgr_buffer[buffer_size];
    uchar  gray_palette[256];
    bool   result = false;
    uchar* src = buffer;
    uchar* bgr = bgr_buffer;
    int  src_pitch = ((m_width*(m_bpp != 15 ? m_bpp : 16) + 7)/8 + 3) & -4;
    int  nch = color ? 3 : 1;
    int  width3 = m_width*nch;
    int  y;

    if( m_offset < 0 || !m_strm.IsOpened())
        return false;
    
    data += (m_height - 1)*step;
    step = -step;

    if( (m_bpp != 24 || !color) && src_pitch+32 > buffer_size )
        src = new uchar[src_pitch+32];

    if( !color )
    {
        if( m_bpp <= 8 )
        {
            CvtPaletteToGray( m_palette, gray_palette, 1 << m_bpp );
        }
        if( m_width*3 + 32 > buffer_size ) bgr = new uchar[m_width*3 + 32];
    }
    
    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_strm.SetPos( m_offset );
        
        switch( m_bpp )
        {
        /************************* 1 BPP ************************/
        case 1:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( src, src_pitch );
                FillColorRow1( color ? data : bgr, src, m_width, m_palette );
                if( !color )
                    icvCvt_BGR2Gray_8u_C3C1R( bgr, 0, data, 0, cvSize(m_width,1) );
            }
            result = true;
            break;
        
        /************************* 4 BPP ************************/
        case 4:
            if( m_rle_code == BMP_RGB )
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    if( color )
                        FillColorRow4( data, src, m_width, m_palette );
                    else
                        FillGrayRow4( data, src, m_width, gray_palette );
                }
                result = true;
            }
            else if( m_rle_code == BMP_RLE4 ) // rle4 compression
            {
                uchar* line_end = data + width3;
                y = 0;

                for(;;)
                {
                    int code = m_strm.GetWord();
                    int len = code & 255;
                    code >>= 8;
                    if( len != 0 ) // encoded mode
                    {
                        PaletteEntry clr[2];
                        uchar gray_clr[2];
                        int t = 0;
                        
                        clr[0] = m_palette[code >> 4];
                        clr[1] = m_palette[code & 15];
                        gray_clr[0] = gray_palette[code >> 4];
                        gray_clr[1] = gray_palette[code & 15];

                        uchar* end = data + len*nch;
                        if( end > line_end ) goto decode_rle4_bad;
                        do
                        {
                            if( color )
                                WRITE_PIX( data, clr[t] );
                            else
                                *data = gray_clr[t];
                            t ^= 1;
                        }
                        while( (data += nch) < end );
                    }
                    else if( code > 2 ) // absolute mode
                    {
                        if( data + code*nch > line_end ) goto decode_rle4_bad;
                        m_strm.GetBytes( src, (((code + 1)>>1) + 1) & -2 );
                        if( color )
                            data = FillColorRow4( data, src, code, m_palette );
                        else
                            data = FillGrayRow4( data, src, code, gray_palette );
                    }
                    else
                    {
                        int x_shift3 = (int)(line_end - data);
                        int y_shift = m_height - y;

                        if( code == 2 )
                        {
                            x_shift3 = m_strm.GetByte()*nch;
                            y_shift = m_strm.GetByte();
                        }

                        len = x_shift3 + (y_shift * width3) & ((code == 0) - 1);

                        if( color )
                            data = FillUniColor( data, line_end, step, width3, 
                                                 y, m_height, x_shift3,
                                                 m_palette[0] );
                        else
                            data = FillUniGray( data, line_end, step, width3, 
                                                y, m_height, x_shift3,
                                                gray_palette[0] );

                        if( y >= m_height )
                            break;
                    }
                }

                result = true;
decode_rle4_bad: ;
            }
            break;

        /************************* 8 BPP ************************/
        case 8:
            if( m_rle_code == BMP_RGB )
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    if( color )
                        FillColorRow8( data, src, m_width, m_palette );
                    else
                        FillGrayRow8( data, src, m_width, gray_palette );
                }
                result = true;
            }
            else if( m_rle_code == BMP_RLE8 ) // rle8 compression
            {
                uchar* line_end = data + width3;
                int line_end_flag = 0;
                y = 0;

                for(;;)
                {
                    int code = m_strm.GetWord();
                    int len = code & 255;
                    code >>= 8;
                    if( len != 0 ) // encoded mode
                    {
                        int prev_y = y;
                        len *= nch;
                        
                        if( data + len > line_end )
                            goto decode_rle8_bad;

                        if( color )
                            data = FillUniColor( data, line_end, step, width3, 
                                                 y, m_height, len,
                                                 m_palette[code] );
                        else
                            data = FillUniGray( data, line_end, step, width3, 
                                                y, m_height, len,
                                                gray_palette[code] );

                        line_end_flag = y - prev_y;
                    }
                    else if( code > 2 ) // absolute mode
                    {
                        int prev_y = y;
                        int code3 = code*nch;
                        
                        if( data + code3 > line_end )
                            goto decode_rle8_bad;
                        m_strm.GetBytes( src, (code + 1) & -2 );
                        if( color )
                            data = FillColorRow8( data, src, code, m_palette );
                        else
                            data = FillGrayRow8( data, src, code, gray_palette );

                        line_end_flag = y - prev_y;
                    }
                    else
                    {
                        int x_shift3 = (int)(line_end - data);
                        int y_shift = m_height - y;
                        
                        if( code || !line_end_flag || x_shift3 < width3 )
                        {
                            if( code == 2 )
                            {
                                x_shift3 = m_strm.GetByte()*nch;
                                y_shift = m_strm.GetByte();
                            }

                            x_shift3 += (y_shift * width3) & ((code == 0) - 1);

                            if( y >= m_height )
                                break;

                            if( color )
                                data = FillUniColor( data, line_end, step, width3, 
                                                     y, m_height, x_shift3,
                                                     m_palette[0] );
                            else
                                data = FillUniGray( data, line_end, step, width3, 
                                                    y, m_height, x_shift3,
                                                    gray_palette[0] );

                            if( y >= m_height )
                                break;
                        }

                        line_end_flag = 0;
                    }
                }

                result = true;
decode_rle8_bad: ;
            }
            break;
        /************************* 15 BPP ************************/
        case 15:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( src, src_pitch );
                if( !color )
                    icvCvt_BGR5552Gray_8u_C2C1R( src, 0, data, 0, cvSize(m_width,1) );
                else
                    icvCvt_BGR5552BGR_8u_C2C3R( src, 0, data, 0, cvSize(m_width,1) );
            }
            result = true;
            break;
        /************************* 16 BPP ************************/
        case 16:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( src, src_pitch );
                if( !color )
                    icvCvt_BGR5652Gray_8u_C2C1R( src, 0, data, 0, cvSize(m_width,1) );
                else
                    icvCvt_BGR5652BGR_8u_C2C3R( src, 0, data, 0, cvSize(m_width,1) );
            }
            result = true;
            break;
        /************************* 24 BPP ************************/
        case 24:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( color ? data : src, src_pitch );
                if( !color )
                    icvCvt_BGR2Gray_8u_C3C1R( src, 0, data, 0, cvSize(m_width,1) );
            }
            result = true;
            break;
        /************************* 32 BPP ************************/
        case 32:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( src, src_pitch );

                if( !color )
                    icvCvt_BGRA2Gray_8u_C4C1R( src, 0, data, 0, cvSize(m_width,1) );
                else
                    icvCvt_BGRA2BGR_8u_C4C3R( src, 0, data, 0, cvSize(m_width,1) );
            }
            result = true;
            break;
        default:
            assert(0);
        }
    }

    if( src != buffer ) delete[] src;
    if( bgr != bgr_buffer ) delete[] bgr;
    return result;
}
// 
// 
// //////////////////////////////////////////////////////////////////////////////////////////
// 
GrFmtBmpWriter::GrFmtBmpWriter( const char* filename ) : GrFmtWriter( filename )
{
}
// 
// 
GrFmtBmpWriter::~GrFmtBmpWriter()
{
}
// 
// 
bool  GrFmtBmpWriter::WriteImage( const uchar* data, int step,
                                  int width, int height, int /*depth*/, int channels )
{
    bool result = false;
    int fileStep = (width*channels + 3) & -4;
    uchar zeropad[] = "\0\0\0\0";

    assert( data && width > 0 && height > 0 && step >= fileStep );

    if( m_strm.Open( m_filename ) )
    {
        int  bitmapHeaderSize = 40;
        int  paletteSize = channels > 1 ? 0 : 1024;
        int  headerSize = 14 /* fileheader */ + bitmapHeaderSize + paletteSize;
        PaletteEntry palette[256];
        
        // write signature 'BM'
        m_strm.PutBytes( fmtSignBmp, (int)strlen(fmtSignBmp) );

        // write file header
        m_strm.PutDWord( fileStep*height + headerSize ); // file size
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( headerSize );

        // write bitmap header
        m_strm.PutDWord( bitmapHeaderSize );
        m_strm.PutDWord( width );
        m_strm.PutDWord( height );
        m_strm.PutWord( 1 );
        m_strm.PutWord( channels << 3 );
        m_strm.PutDWord( BMP_RGB );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );
        m_strm.PutDWord( 0 );

        if( channels == 1 )
        {
            FillGrayPalette( palette, 8 );
            m_strm.PutBytes( palette, sizeof(palette));
        }

        width *= channels;
        data += step*(height - 1);
        for( ; height--; data -= step )
        {
            m_strm.PutBytes( data, width );
            if( fileStep > width )
                m_strm.PutBytes( zeropad, fileStep - width );
        }

        m_strm.Close();
        result = true;
    }
    return result;
}
// 
// 
// P?M filter factory

GrFmtPxM::GrFmtPxM()
{
    m_sign_len = 3;
    m_signature = "";
    m_description = "Portable image format (*.pbm;*.pgm;*.ppm)";
}


GrFmtPxM::~GrFmtPxM()
{
}


bool GrFmtPxM::CheckSignature( const char* signature )
{
    return signature[0] == 'P' &&
           '1' <= signature[1] && signature[1] <= '6' &&
           isspace(signature[2]);
}


GrFmtReader* GrFmtPxM::NewReader( const char* filename )
{
    return new GrFmtPxMReader( filename );
}


GrFmtWriter* GrFmtPxM::NewWriter( const char* filename )
{
    return new GrFmtPxMWriter( filename );
}


///////////////////////// P?M reader //////////////////////////////

static int ReadNumber( RLByteStream& strm, int maxdigits )
{
    int code;
    int val = 0;
    int digits = 0;

    code = strm.GetByte();

    if( !isdigit(code))
    {
        do
        {
            if( code == '#' )
            {
                do
                {
                    code = strm.GetByte();
                }
                while( code != '\n' && code != '\r' );
            }
            
            code = strm.GetByte();

            while( isspace(code))
                code = strm.GetByte();
        }
        while( !isdigit( code ));
    }

    do
    {
        val = val*10 + code - '0';
        if( ++digits >= maxdigits ) break;
        code = strm.GetByte();
    }
    while( isdigit(code));

    return val;
}


GrFmtPxMReader::GrFmtPxMReader( const char* filename ) : GrFmtReader( filename )
{
    m_offset = -1;
}


GrFmtPxMReader::~GrFmtPxMReader()
{
}


void  GrFmtPxMReader::Close()
{
    m_strm.Close();
}


bool  GrFmtPxMReader::ReadHeader()
{
    bool result = false;
    
    assert( strlen(m_filename) != 0 );
    if( !m_strm.Open( m_filename )) return false;

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        int code = m_strm.GetByte();
        if( code != 'P' )
            BAD_HEADER_ERR();

        code = m_strm.GetByte();
        switch( code )
        {
        case '1': case '4': m_bpp = 1; break;
        case '2': case '5': m_bpp = 8; break;
        case '3': case '6': m_bpp = 24; break;
        default: BAD_HEADER_ERR();
        }
        
        m_binary = code >= '4';
        m_iscolor = m_bpp > 8;

        m_width = ReadNumber( m_strm, INT_MAX );
        m_height = ReadNumber( m_strm, INT_MAX );
        
        m_maxval = m_bpp == 1 ? 1 : ReadNumber( m_strm, INT_MAX );

        if( m_maxval > 255 ) m_binary = false;

        if( m_width > 0 && m_height > 0 && m_maxval > 0 && m_maxval < (1 << 16)) 
        {
            m_offset = m_strm.GetPos();
            result = true;
        }
bad_header_exit:
        ;
    }

    if( !result )
    {
        m_offset = -1;
        m_width = m_height = -1;
        m_strm.Close();
    }
    return result;
}


bool  GrFmtPxMReader::ReadData( uchar* data, int step, int color )
{
    const  int buffer_size = 1 << 12;
    uchar  buffer[buffer_size];
    uchar  pal_buffer[buffer_size];
    uchar  bgr_buffer[buffer_size];
    PaletteEntry palette[256];
    bool   result = false;
    uchar* src = buffer;
    uchar* gray_palette = pal_buffer;
    uchar* bgr = bgr_buffer;
    int  src_pitch = (m_width*m_bpp + 7)/8;
    int  nch = m_iscolor ? 3 : 1;
    int  width3 = m_width*nch;
    int  i, x, y;

    if( m_offset < 0 || !m_strm.IsOpened())
        return false;
    
    if( src_pitch+32 > buffer_size )
        src = new uchar[width3 + 32];

    if( m_maxval + 1 > buffer_size )
        gray_palette = new uchar[m_maxval + 1];

    if( m_width*3 + 32 > buffer_size )
        bgr = new uchar[m_width*3 + 32];

    // create LUT for converting colors
    for( i = 0; i <= m_maxval; i++ )
    {
        gray_palette[i] = (uchar)((i*255/m_maxval)^(m_bpp == 1 ? 255 : 0));
    }

    FillGrayPalette( palette, m_bpp==1 ? 1 : 8 , m_bpp == 1 );
    
    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_strm.SetPos( m_offset );
        
        switch( m_bpp )
        {
        ////////////////////////// 1 BPP /////////////////////////
        case 1:
            if( !m_binary )
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    for( x = 0; x < m_width; x++ )
                        src[x] = ReadNumber( m_strm, 1 ) != 0;

                    if( color )
                        FillColorRow8( data, src, m_width, palette );
                    else
                        FillGrayRow8( data, src, m_width, gray_palette );
                }
            }
            else
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    
                    if( color )
                        FillColorRow1( data, src, m_width, palette );
                    else
                        FillGrayRow1( data, src, m_width, gray_palette );
                }
            }
            result = true;
            break;

        ////////////////////////// 8 BPP /////////////////////////
        case 8:
        case 24:
            for( y = 0; y < m_height; y++, data += step )
            {
                if( !m_binary )
                {
                    for( x = 0; x < width3; x++ )
                    {
                        int code = ReadNumber( m_strm, INT_MAX );
                        if( (unsigned)code > (unsigned)m_maxval ) code = m_maxval;
                        src[x] = gray_palette[code];
                    }
                }
                else
                {
                    m_strm.GetBytes( src, src_pitch );
                }

                if( m_bpp == 8 )
                {
                    if( color )
                        FillColorRow8( data, src, m_width, palette );
                    else
                        memcpy( data, src, m_width );
                }
                else
                {
                    if( color )
                        icvCvt_RGB2BGR_8u_C3R( src, 0, data, 0, cvSize(m_width,1) );
                    else
                        icvCvt_BGR2Gray_8u_C3C1R( src, 0, data, 0, cvSize(m_width,1), 2 );
                }
            }
            result = true;
            break;
        default:
            assert(0);
        }
    }

    if( src != buffer )
        delete[] src; 

    if( bgr != bgr_buffer )
        delete[] bgr;

    if( gray_palette != pal_buffer )
        delete[] gray_palette;

    return result;
}


//////////////////////////////////////////////////////////////////////////////////////////

GrFmtPxMWriter::GrFmtPxMWriter( const char* filename ) : GrFmtWriter( filename )
{
}


GrFmtPxMWriter::~GrFmtPxMWriter()
{
}


static char PxMLUT[256][5];
static bool isPxMLUTInitialized = false;

bool  GrFmtPxMWriter::WriteImage( const uchar* data, int step,
                                  int width, int height, int /*depth*/, int _channels )
{
    bool isBinary = false;
    bool result = false;

    int  channels = _channels > 1 ? 3 : 1;
    int  fileStep = width*channels;
    int  x, y;

    assert( data && width > 0 && height > 0 && step >= fileStep );
    
    if( m_strm.Open( m_filename ) )
    {
        int  lineLength = ((isBinary ? 1 : 4)*channels +
                           (channels > 1 ? 2 : 0))*width + 32;
        int  bufferSize = 128; // buffer that should fit a header
        char* buffer = 0;
                
        if( bufferSize < lineLength )
            bufferSize = lineLength;

        buffer = new char[bufferSize];
        if( !buffer )
        {
            m_strm.Close();
            return false;
        }

        if( !isPxMLUTInitialized )
        {
            for( int i = 0; i < 256; i++ )
                sprintf( PxMLUT[i], "%4d", i );

            isPxMLUTInitialized = 1;
        }

        // write header;
        sprintf( buffer, "P%c\n%d %d\n255\n",
                 '2' + (channels > 1 ? 1 : 0) + (isBinary ? 3 : 0),
                 width, height );

        m_strm.PutBytes( buffer, (int)strlen(buffer) );

        for( y = 0; y < height; y++, data += step )
        {
            if( isBinary )
            {
                if( _channels == 3 )
                    icvCvt_BGR2RGB_8u_C3R( (uchar*)data, 0,
                        (uchar*)buffer, 0, cvSize(width,1) );
                else if( _channels == 4 )
                    icvCvt_BGRA2BGR_8u_C4C3R( (uchar*)data, 0,
                        (uchar*)buffer, 0, cvSize(width,1), 1 );
                m_strm.PutBytes( channels > 1 ? buffer : (char*)data, fileStep );
            }
            else
            {
                char* ptr = buffer;
                
                if( channels > 1 )
                    for( x = 0; x < width*channels; x += channels )
                    {
                        strcpy( ptr, PxMLUT[data[x+2]] );
                        ptr += 4;
                        strcpy( ptr, PxMLUT[data[x+1]] );
                        ptr += 4;
                        strcpy( ptr, PxMLUT[data[x]] );
                        ptr += 4;
                        *ptr++ = ' ';
                        *ptr++ = ' ';
                    }
                else
                    for( x = 0; x < width; x++ )
                    {
                        strcpy( ptr, PxMLUT[data[x]] );
                        ptr += 4;
                    }

                *ptr++ = '\n';

                m_strm.PutBytes( buffer, (int)(ptr - buffer) );
            }
        }
        delete[] buffer;
        m_strm.Close();
        result = true;
    }
    
    return result;
}

static const char* fmtSignSunRas = "\x59\xA6\x6A\x95";

// Sun Raster filter factory

GrFmtSunRaster::GrFmtSunRaster()
{
    m_sign_len = 4;
    m_signature = fmtSignSunRas;
    m_description = "Sun raster files (*.sr;*.ras)";
}


GrFmtSunRaster::~GrFmtSunRaster()
{
}


GrFmtReader* GrFmtSunRaster::NewReader( const char* filename )
{
    return new GrFmtSunRasterReader( filename );
}


GrFmtWriter* GrFmtSunRaster::NewWriter( const char* filename )
{
    return new GrFmtSunRasterWriter( filename );
}


/************************ Sun Raster reader *****************************/

GrFmtSunRasterReader::GrFmtSunRasterReader( const char* filename ) : GrFmtReader( filename )
{
    m_offset = -1;
}


GrFmtSunRasterReader::~GrFmtSunRasterReader()
{
}


void  GrFmtSunRasterReader::Close()
{
    m_strm.Close();
}


bool  GrFmtSunRasterReader::ReadHeader()
{
    bool result = false;
    
    assert( strlen(m_filename) != 0 );
    if( !m_strm.Open( m_filename )) return false;

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_strm.Skip( 4 );
        m_width  = m_strm.GetDWord();
        m_height = m_strm.GetDWord();
        m_bpp    = m_strm.GetDWord();
        int palSize = 3*(1 << m_bpp);

        m_strm.Skip( 4 );
        m_type   = (SunRasType)m_strm.GetDWord();
        m_maptype = (SunRasMapType)m_strm.GetDWord();
        m_maplength = m_strm.GetDWord();

        if( m_width > 0 && m_height > 0 &&
            (m_bpp == 1 || m_bpp == 8 || m_bpp == 24 || m_bpp == 32) &&
            (m_type == RAS_OLD || m_type == RAS_STANDARD ||
             (m_type == RAS_BYTE_ENCODED && m_bpp == 8) || m_type == RAS_FORMAT_RGB) &&
            (m_maptype == RMT_NONE && m_maplength == 0 ||
             m_maptype == RMT_EQUAL_RGB && m_maplength <= palSize && m_bpp <= 8))
        {
            memset( m_palette, 0, sizeof(m_palette));
            
            if( m_maplength != 0 )
            {
                int readed;
                uchar buffer[256*3];

                m_strm.GetBytes( buffer, m_maplength, &readed );
                if( readed == m_maplength )
                {
                    int i;
                    palSize = m_maplength/3;

                    for( i = 0; i < palSize; i++ )
                    {
                        m_palette[i].b = buffer[i + 2*palSize];
                        m_palette[i].g = buffer[i + palSize];
                        m_palette[i].r = buffer[i];
                        m_palette[i].a = 0;
                    }

                    m_iscolor = IsColorPalette( m_palette, m_bpp );
                    m_offset = m_strm.GetPos();

                    assert( m_offset == 32 + m_maplength );
                    result = true;
                }
            }
            else
            {
                m_iscolor = m_bpp > 8;
                
                if( !m_iscolor )
                    FillGrayPalette( m_palette, m_bpp );

                m_offset = m_strm.GetPos();

                assert( m_offset == 32 + m_maplength );
                result = true;
            }
        }
    }

    if( !result )
    {
        m_offset = -1;
        m_width = m_height = -1;
        m_strm.Close();
    }
    return result;
}


bool  GrFmtSunRasterReader::ReadData( uchar* data, int step, int color )
{
    const  int buffer_size = 1 << 12;
    uchar  buffer[buffer_size];
    uchar  bgr_buffer[buffer_size];
    uchar  gray_palette[256];
    bool   result = false;
    uchar* src = buffer;
    uchar* bgr = bgr_buffer;
    int  src_pitch = ((m_width*m_bpp + 7)/8 + 1) & -2;
    int  nch = color ? 3 : 1;
    int  width3 = m_width*nch;
    int  y;

    if( m_offset < 0 || !m_strm.IsOpened())
        return false;
    
    if( src_pitch+32 > buffer_size )
        src = new uchar[src_pitch+32];

    if( m_width*3 + 32 > buffer_size )
        bgr = new uchar[m_width*3 + 32];

    if( !color && m_maptype == RMT_EQUAL_RGB )
        CvtPaletteToGray( m_palette, gray_palette, 1 << m_bpp );

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_strm.SetPos( m_offset );
        
        switch( m_bpp )
        {
        /************************* 1 BPP ************************/
        case 1:
            if( m_type != RAS_BYTE_ENCODED )
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    if( color )
                        FillColorRow1( data, src, m_width, m_palette );
                    else
                        FillGrayRow1( data, src, m_width, gray_palette );
                }
                result = true;
            }
            else
            {
                uchar* line_end = src + (m_width*m_bpp + 7)/8;
                uchar* tsrc = src;
                y = 0;
               
                for(;;)
                {
                    int max_count = (int)(line_end - tsrc);
                    int code = 0, len = 0, len1 = 0;

                    do
                    {
                        code = m_strm.GetByte();
                        if( code == 0x80 )
                        {
                            len = m_strm.GetByte();
                            if( len != 0 ) break;
                        }
                        tsrc[len1] = (uchar)code;
                    }
                    while( ++len1 < max_count );

                    tsrc += len1;

                    if( len > 0 ) // encoded mode
                    {
                        ++len;
                        code = m_strm.GetByte();
                        if( len > line_end - tsrc )
                        {
                            assert(0);
                            goto bad_decoding_1bpp;
                        }

                        memset( tsrc, code, len );
                        tsrc += len;
                    }

                    if( tsrc >= line_end )
                    {
                        tsrc = src;
                        if( color )
                            FillColorRow1( data, src, m_width, m_palette );
                        else
                            FillGrayRow1( data, src, m_width, gray_palette );
                        data += step;
                        if( ++y >= m_height ) break;
                    }
                }
                result = true;
bad_decoding_1bpp:
                ;
            }
            break;
        /************************* 8 BPP ************************/
        case 8:
            if( m_type != RAS_BYTE_ENCODED )
            {
                for( y = 0; y < m_height; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    if( color )
                        FillColorRow8( data, src, m_width, m_palette );
                    else
                        FillGrayRow8( data, src, m_width, gray_palette );
                }
                result = true;
            }
            else // RLE-encoded
            {
                uchar* line_end = data + width3;
                y = 0;

                for(;;)
                {
                    int max_count = (int)(line_end - data);
                    int code = 0, len = 0, len1;
                    uchar* tsrc = src;

                    do
                    {
                        code = m_strm.GetByte();
                        if( code == 0x80 )
                        {
                            len = m_strm.GetByte();
                            if( len != 0 ) break;
                        }
                        *tsrc++ = (uchar)code;
                    }
                    while( (max_count -= nch) > 0 );

                    len1 = (int)(tsrc - src);

                    if( len1 > 0 )
                    {
                        if( color )
                            FillColorRow8( data, src, len1, m_palette );
                        else
                            FillGrayRow8( data, src, len1, gray_palette );
                        data += len1*nch;
                    }

                    if( len > 0 ) // encoded mode
                    {
                        len = (len + 1)*nch;
                        code = m_strm.GetByte();

                        if( color )
                            data = FillUniColor( data, line_end, step, width3, 
                                                 y, m_height, len,
                                                 m_palette[code] );
                        else
                            data = FillUniGray( data, line_end, step, width3, 
                                                y, m_height, len,
                                                gray_palette[code] );
                        if( y >= m_height )
                            break;
                    }

                    if( data == line_end )
                    {
                        if( m_strm.GetByte() != 0 )
                            goto bad_decoding_end;
                        line_end += step;
                        data = line_end - width3;
                        if( ++y >= m_height ) break;
                    }
                }

                result = true;
bad_decoding_end:
                ;
            }
            break;
        /************************* 24 BPP ************************/
        case 24:
            for( y = 0; y < m_height; y++, data += step )
            {
                m_strm.GetBytes( color ? data : bgr, src_pitch );

                if( color )
                {
                    if( m_type == RAS_FORMAT_RGB )
                        icvCvt_RGB2BGR_8u_C3R( data, 0, data, 0, cvSize(m_width,1) );
                }
                else
                {
                    icvCvt_BGR2Gray_8u_C3C1R( bgr, 0, data, 0, cvSize(m_width,1),
                                              m_type == RAS_FORMAT_RGB ? 2 : 0 );
                }
            }
            result = true;
            break;
        /************************* 32 BPP ************************/
        case 32:
            for( y = 0; y < m_height; y++, data += step )
            {
                /* hack: a0 b0 g0 r0 a1 b1 g1 r1 ... are written to src + 3,
                   so when we look at src + 4, we see b0 g0 r0 x b1 g1 g1 x ... */
                m_strm.GetBytes( src + 3, src_pitch );
                
                if( color )
                    icvCvt_BGRA2BGR_8u_C4C3R( src + 4, 0, data, 0, cvSize(m_width,1),
                                              m_type == RAS_FORMAT_RGB ? 2 : 0 );
                else
                    icvCvt_BGRA2Gray_8u_C4C1R( src + 4, 0, data, 0, cvSize(m_width,1),
                                               m_type == RAS_FORMAT_RGB ? 2 : 0 );
            }
            result = true;
            break;
        default:
            assert(0);
        }
    }

    if( src != buffer ) delete[] src; 
    if( bgr != bgr_buffer ) delete[] bgr;

    return result;
}


//////////////////////////////////////////////////////////////////////////////////////////

GrFmtSunRasterWriter::GrFmtSunRasterWriter( const char* filename ) : GrFmtWriter( filename )
{
}


GrFmtSunRasterWriter::~GrFmtSunRasterWriter()
{
}


bool  GrFmtSunRasterWriter::WriteImage( const uchar* data, int step,
                                        int width, int height, int /*depth*/, int channels )
{
    bool result = false;
    int  fileStep = (width*channels + 1) & -2;
    int  y;

    assert( data && width > 0 && height > 0 && step >= fileStep);
    
    if( m_strm.Open( m_filename ) )
    {
        m_strm.PutBytes( fmtSignSunRas, (int)strlen(fmtSignSunRas) );
        m_strm.PutDWord( width );
        m_strm.PutDWord( height );
        m_strm.PutDWord( channels*8 );
        m_strm.PutDWord( fileStep*height );
        m_strm.PutDWord( RAS_STANDARD );
        m_strm.PutDWord( RMT_NONE );
        m_strm.PutDWord( 0 );

        for( y = 0; y < height; y++, data += step )
            m_strm.PutBytes( data, fileStep );
        
        m_strm.Close();
        result = true;
    }
    return result;
}

static const char fmtSignTiffII[] = "II\x2a\x00";
static const char fmtSignTiffMM[] = "MM\x00\x2a";

GrFmtTiff::GrFmtTiff()
{
    m_sign_len = 4;
    m_signature = "";
    m_description = "TIFF Files (*.tiff;*.tif)";
}

GrFmtTiff::~GrFmtTiff()
{
}

bool GrFmtTiff::CheckSignature( const char* signature )
{
    return memcmp( signature, fmtSignTiffII, 4 ) == 0 ||
           memcmp( signature, fmtSignTiffMM, 4 ) == 0;
}


GrFmtReader* GrFmtTiff::NewReader( const char* filename )
{
    return new GrFmtTiffReader( filename );
}


GrFmtWriter* GrFmtTiff::NewWriter( const char* filename )
{
    return new GrFmtTiffWriter( filename );
}


#ifdef HAVE_TIFF

#include "tiff.h"
#include "tiffio.h"

static int grfmt_tiff_err_handler_init = 0;

static void GrFmtSilentTIFFErrorHandler( const char*, const char*, va_list ) {}

GrFmtTiffReader::GrFmtTiffReader( const char* filename ) : GrFmtReader( filename )
{
    m_tif = 0;

    if( !grfmt_tiff_err_handler_init )
    {
        grfmt_tiff_err_handler_init = 1;

        TIFFSetErrorHandler( GrFmtSilentTIFFErrorHandler );
        TIFFSetWarningHandler( GrFmtSilentTIFFErrorHandler );
    }
}


GrFmtTiffReader::~GrFmtTiffReader()
{
}


void  GrFmtTiffReader::Close()
{
    if( m_tif )
    {
        TIFF* tif = (TIFF*)m_tif;
        TIFFClose( tif );
        m_tif = 0;
    }
}


bool  GrFmtTiffReader::CheckFormat( const char* signature )
{
    return memcmp( signature, fmtSignTiffII, 4 ) == 0 ||
           memcmp( signature, fmtSignTiffMM, 4 ) == 0;
}


bool  GrFmtTiffReader::ReadHeader()
{
    char errmsg[1024];
    bool result = false;

    Close();
    TIFF* tif = TIFFOpen( m_filename, "r" );

    if( tif )
    {
        int width = 0, height = 0, photometric = 0, compression = 0;
        m_tif = tif;

        if( TIFFRGBAImageOK( tif, errmsg ) &&
            TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &width ) &&
            TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &height ) &&
            TIFFGetField( tif, TIFFTAG_PHOTOMETRIC, &photometric ) && 
            (!TIFFGetField( tif, TIFFTAG_COMPRESSION, &compression ) ||
            (compression != COMPRESSION_LZW &&
             compression != COMPRESSION_OJPEG)))
        {
            m_width = width;
            m_height = height;
            m_iscolor = photometric > 1;
            
            result = true;
        }
    }

    if( !result )
        Close();

    return result;
}


bool  GrFmtTiffReader::ReadData( uchar* data, int step, int color )
{
    bool result = false;
    uchar* buffer = 0;

    color = color > 0 || (color < 0 && m_iscolor);

    if( m_tif && m_width && m_height )
    {
        TIFF* tif = (TIFF*)m_tif;
        int tile_width0 = m_width, tile_height0 = 0;
        int x, y, i;
        int is_tiled = TIFFIsTiled(tif);
        
        if( !is_tiled &&
            TIFFGetField( tif, TIFFTAG_ROWSPERSTRIP, &tile_height0 ) ||
            is_tiled &&
            TIFFGetField( tif, TIFFTAG_TILEWIDTH, &tile_width0 ) &&
            TIFFGetField( tif, TIFFTAG_TILELENGTH, &tile_height0 ))
        {
            if( tile_width0 <= 0 )
                tile_width0 = m_width;

            if( tile_height0 <= 0 )
                tile_height0 = m_height;
            
            buffer = new uchar[tile_height0*tile_width0*4];

            for( y = 0; y < m_height; y += tile_height0, data += step*tile_height0 )
            {
                int tile_height = tile_height0;

                if( y + tile_height > m_height )
                    tile_height = m_height - y;

                for( x = 0; x < m_width; x += tile_width0 )
                {
                    int tile_width = tile_width0, ok;

                    if( x + tile_width > m_width )
                        tile_width = m_width - x;

                    if( !is_tiled )
                        ok = TIFFReadRGBAStrip( tif, y, (uint32*)buffer );
                    else
                        ok = TIFFReadRGBATile( tif, x, y, (uint32*)buffer );

                    if( !ok )
                        goto exit_func;

                    for( i = 0; i < tile_height; i++ )
                        if( color )
                            icvCvt_BGRA2BGR_8u_C4C3R( buffer + i*tile_width*4, 0,
                                          data + x*3 + step*(tile_height - i - 1), 0,
                                          cvSize(tile_width,1), 2 );
                        else
                            icvCvt_BGRA2Gray_8u_C4C1R( buffer + i*tile_width*4, 0,
                                           data + x + step*(tile_height - i - 1), 0,
                                           cvSize(tile_width,1), 2 );
                }
            }

            result = true;
        }
    }

exit_func:

    Close();
    delete[] buffer;

    return result;
}

#else

static const int  tiffMask[] = { 0xff, 0xff, 0xffffffff, 0xffff, 0xffffffff };

/************************ TIFF reader *****************************/

GrFmtTiffReader::GrFmtTiffReader( const char* filename ) : GrFmtReader( filename )
{
    m_offsets = 0;
    m_maxoffsets = 0;
    m_strips = -1;
    m_max_pal_length = 0;
    m_temp_palette = 0;
}


GrFmtTiffReader::~GrFmtTiffReader()
{
    Close();

    delete[] m_offsets;
    delete[] m_temp_palette;
}

void  GrFmtTiffReader::Close()
{
    m_strm.Close();
}


bool  GrFmtTiffReader::CheckFormat( const char* signature )
{
    return memcmp( signature, fmtSignTiffII, 4 ) == 0 ||
           memcmp( signature, fmtSignTiffMM, 4 ) == 0;
}


int   GrFmtTiffReader::GetWordEx()
{
    int val = m_strm.GetWord();
    if( m_byteorder == TIFF_ORDER_MM )
        val = ((val)>>8)|(((val)&0xff)<<8);
    return val;
}


int   GrFmtTiffReader::GetDWordEx()
{
    int val = m_strm.GetDWord();
    if( m_byteorder == TIFF_ORDER_MM )
        val = BSWAP( val );
    return val;
}


int  GrFmtTiffReader::ReadTable( int offset, int count,
                                 TiffFieldType fieldType,
                                 int*& array, int& arraysize )
{
    int i;
    
    if( count < 0 )
        return RBS_BAD_HEADER;
    
    if( fieldType != TIFF_TYPE_SHORT &&
        fieldType != TIFF_TYPE_LONG &&
        fieldType != TIFF_TYPE_BYTE )
        return RBS_BAD_HEADER;

    if( count > arraysize )
    {
        delete[] array;
        arraysize = arraysize*3/2;
        if( arraysize < count )
            arraysize = count;
        array = new int[arraysize];
    }

    if( count > 1 )
    {
        int pos = m_strm.GetPos();
        m_strm.SetPos( offset );

        if( fieldType == TIFF_TYPE_LONG )
        {
            if( m_byteorder == TIFF_ORDER_MM )
                for( i = 0; i < count; i++ )
                    array[i] = ((RMByteStream&)m_strm).GetDWord();
            else
                for( i = 0; i < count; i++ )
                    array[i] = ((RLByteStream&)m_strm).GetDWord();
        }
        else if( fieldType == TIFF_TYPE_SHORT )
        {
            if( m_byteorder == TIFF_ORDER_MM )
                for( i = 0; i < count; i++ )
                    array[i] = ((RMByteStream&)m_strm).GetWord();
            else
                for( i = 0; i < count; i++ )
                    array[i] = ((RLByteStream&)m_strm).GetWord();
        }
        else // fieldType == TIFF_TYPE_BYTE
            for( i = 0; i < count; i++ )
                array[i] = m_strm.GetByte();

        m_strm.SetPos(pos);
    }
    else
    {
        assert( (offset & ~tiffMask[fieldType]) == 0 );
        array[0] = offset;
    }

    return 0;
}


bool  GrFmtTiffReader::ReadHeader()
{
    bool result = false;
    int  photometric = -1;
    int  channels = 1;
    int  pal_length = -1;

    const int MAX_CHANNELS = 4;
    int  bpp_arr[MAX_CHANNELS];

    assert( strlen(m_filename) != 0 );
    if( !m_strm.Open( m_filename )) return false;

    m_width = -1;
    m_height = -1;
    m_strips = -1;
    m_bpp = 1;
    m_compression = TIFF_UNCOMP;
    m_rows_per_strip = -1;
    m_iscolor = false;

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        m_byteorder = (TiffByteOrder)m_strm.GetWord();
        m_strm.Skip( 2 );
        int header_offset = GetDWordEx();

        m_strm.SetPos( header_offset );

        // read the first tag directory
        int i, j, count = GetWordEx();

        for( i = 0; i < count; i++ )
        {
            // read tag
            TiffTag tag = (TiffTag)GetWordEx();
            TiffFieldType fieldType = (TiffFieldType)GetWordEx();
            int count = GetDWordEx();
            int value = GetDWordEx();
            if( count == 1 )
            {
                if( m_byteorder == TIFF_ORDER_MM )
                {
                    if( fieldType == TIFF_TYPE_SHORT )
                        value = (unsigned)value >> 16;
                    else if( fieldType == TIFF_TYPE_BYTE )
                        value = (unsigned)value >> 24;
                }

                value &= tiffMask[fieldType];
            }

            switch( tag )
            {
            case  TIFF_TAG_WIDTH:
                m_width = value;
                break;

            case  TIFF_TAG_HEIGHT:
                m_height = value;
                break;

            case  TIFF_TAG_BITS_PER_SAMPLE:
                {
                    int* bpp_arr_ref = bpp_arr;

                    if( count > MAX_CHANNELS )
                        BAD_HEADER_ERR();

                    if( ReadTable( value, count, fieldType, bpp_arr_ref, count ) < 0 )
                        BAD_HEADER_ERR();
                
                    for( j = 1; j < count; j++ )
                    {
                        if( bpp_arr[j] != bpp_arr[0] )
                            BAD_HEADER_ERR();
                    }

                    m_bpp = bpp_arr[0];
                }

                break;

            case  TIFF_TAG_COMPRESSION:
                m_compression = (TiffCompression)value;
                if( m_compression != TIFF_UNCOMP &&
                    m_compression != TIFF_HUFFMAN &&
                    m_compression != TIFF_PACKBITS )
                    BAD_HEADER_ERR();
                break;

            case  TIFF_TAG_PHOTOMETRIC:
                photometric = value;
                if( (unsigned)photometric > 3 )
                    BAD_HEADER_ERR();
                break;

            case  TIFF_TAG_STRIP_OFFSETS:
                m_strips = count;
                if( ReadTable( value, count, fieldType, m_offsets, m_maxoffsets ) < 0 )
                    BAD_HEADER_ERR();
                break;

            case  TIFF_TAG_SAMPLES_PER_PIXEL:
                channels = value;
                if( channels != 1 && channels != 3 && channels != 4 )
                    BAD_HEADER_ERR();
                break;

            case  TIFF_TAG_ROWS_PER_STRIP:
                m_rows_per_strip = value;
                break;

            case  TIFF_TAG_PLANAR_CONFIG:
                {
                int planar_config = value;
                if( planar_config != 1 )
                    BAD_HEADER_ERR();
                }
                break;

            case  TIFF_TAG_COLOR_MAP:
                if( fieldType != TIFF_TYPE_SHORT || count < 2 )
                    BAD_HEADER_ERR();
                if( ReadTable( value, count, fieldType,
                               m_temp_palette, m_max_pal_length ) < 0 )
                    BAD_HEADER_ERR();
                pal_length = count / 3;
                if( pal_length > 256 )
                    BAD_HEADER_ERR();
                for( i = 0; i < pal_length; i++ )
                {
                    m_palette[i].r = (uchar)(m_temp_palette[i] >> 8);
                    m_palette[i].g = (uchar)(m_temp_palette[i + pal_length] >> 8);
                    m_palette[i].b = (uchar)(m_temp_palette[i + pal_length*2] >> 8);
                }
                break;
            case  TIFF_TAG_STRIP_COUNTS:
                break;
            }
        }

        if( m_strips == 1 && m_rows_per_strip == -1 )
            m_rows_per_strip = m_height;

        if( m_width > 0 && m_height > 0 && m_strips > 0 &&
            (m_height + m_rows_per_strip - 1)/m_rows_per_strip == m_strips )
        {
            switch( m_bpp )
            {
            case 1:
                if( photometric == 0 || photometric == 1 && channels == 1 )
                {
                    FillGrayPalette( m_palette, m_bpp, photometric == 0 );
                    result = true;
                    m_iscolor = false;
                }
                break;
            case 4:
            case 8:
                if( (photometric == 0 || photometric == 1 ||
                     photometric == 3 && pal_length == (1 << m_bpp)) &&
                    m_compression != TIFF_HUFFMAN && channels == 1 )
                {
                    if( pal_length < 0 )
                    {
                        FillGrayPalette( m_palette, m_bpp, photometric == 0 );
                        m_iscolor = false;
                    }
                    else
                    {
                        m_iscolor = IsColorPalette( m_palette, m_bpp );
                    }
                    result = true;
                }
                else if( photometric == 2 && pal_length < 0 &&
                         (channels == 3 || channels == 4) &&
                         m_compression == TIFF_UNCOMP )
                {
                    m_bpp = 8*channels;
                    m_iscolor = true;
                    result = true;
                }
                break;
            default:
                BAD_HEADER_ERR();
            }
        }
bad_header_exit:
        ;
    }

    if( !result )
    {
        m_strips = -1;
        m_width = m_height = -1;
        m_strm.Close();
    }

    return result;
}


bool  GrFmtTiffReader::ReadData( uchar* data, int step, int color )
{
    const  int buffer_size = 1 << 12;
    uchar  buffer[buffer_size];
    uchar  gray_palette[256];
    bool   result = false;
    uchar* src = buffer;
    int    src_pitch = (m_width*m_bpp + 7)/8;
    int    y = 0;

    if( m_strips < 0 || !m_strm.IsOpened())
        return false;
    
    if( src_pitch+32 > buffer_size )
        src = new uchar[src_pitch+32];

    if( !color )
        if( m_bpp <= 8 )
        {
            CvtPaletteToGray( m_palette, gray_palette, 1 << m_bpp );
        }

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        for( int s = 0; s < m_strips; s++ )
        {
            int y_limit = m_rows_per_strip;

            y_limit += y;
            if( y_limit > m_height ) y_limit = m_height;

            m_strm.SetPos( m_offsets[s] );

            if( m_compression == TIFF_UNCOMP )
            {
                for( ; y < y_limit; y++, data += step )
                {
                    m_strm.GetBytes( src, src_pitch );
                    if( color )
                        switch( m_bpp )
                        {
                        case 1:
                            FillColorRow1( data, src, m_width, m_palette );
                            break;
                        case 4:
                            FillColorRow4( data, src, m_width, m_palette );
                            break;
                        case 8:
                            FillColorRow8( data, src, m_width, m_palette );
                            break;
                        case 24:
                            icvCvt_RGB2BGR_8u_C3R( src, 0, data, 0, cvSize(m_width,1) );
                            break;
                        case 32:
                            icvCvt_BGRA2BGR_8u_C4C3R( src, 0, data, 0, cvSize(m_width,1), 2 );
                            break;
                        default:
                            assert(0);
                            goto bad_decoding_end;
                        }
                    else
                        switch( m_bpp )
                        {
                        case 1:
                            FillGrayRow1( data, src, m_width, gray_palette );
                            break;
                        case 4:
                            FillGrayRow4( data, src, m_width, gray_palette );
                            break;
                        case 8:
                            FillGrayRow8( data, src, m_width, gray_palette );
                            break;
                        case 24:
                            icvCvt_BGR2Gray_8u_C3C1R( src, 0, data, 0, cvSize(m_width,1), 2 );
                            break;
                        case 32:
                            icvCvt_BGRA2Gray_8u_C4C1R( src, 0, data, 0, cvSize(m_width,1), 2 );
                            break;
                        default:
                            assert(0);
                            goto bad_decoding_end;
                        }
                }
            }
            else
            {
            }

            result = true;

bad_decoding_end:

            ;
        }
    }

    if( src != buffer ) delete[] src; 
    return result;
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////

GrFmtTiffWriter::GrFmtTiffWriter( const char* filename ) : GrFmtWriter( filename )
{
}

GrFmtTiffWriter::~GrFmtTiffWriter()
{
}

void  GrFmtTiffWriter::WriteTag( TiffTag tag, TiffFieldType fieldType,
                                 int count, int value )
{
    m_strm.PutWord( tag );
    m_strm.PutWord( fieldType );
    m_strm.PutDWord( count );
    m_strm.PutDWord( value );
}


bool  GrFmtTiffWriter::WriteImage( const uchar* data, int step,
                                   int width, int height, int /*depth*/, int channels )
{
    bool result = false;
    int fileStep = width*channels;

    assert( data && width > 0 && height > 0 && step >= fileStep);

    if( m_strm.Open( m_filename ) )
    {
        int rowsPerStrip = (1 << 13)/fileStep;

        if( rowsPerStrip < 1 )
            rowsPerStrip = 1;

        if( rowsPerStrip > height )
            rowsPerStrip = height;

        int i, stripCount = (height + rowsPerStrip - 1) / rowsPerStrip;
/*#if defined _DEBUG || !defined WIN32
        int uncompressedRowSize = rowsPerStrip * fileStep;
#endif*/
        int directoryOffset = 0;

        int* stripOffsets = new int[stripCount];
        short* stripCounts = new short[stripCount];
        uchar* buffer = new uchar[fileStep + 32];
        int  stripOffsetsOffset = 0;
        int  stripCountsOffset = 0;
        int  bitsPerSample = 8; // TODO support 16 bit
        int  y = 0;

        m_strm.PutBytes( fmtSignTiffII, 4 );
        m_strm.PutDWord( directoryOffset );

        // write an image data first (the most reasonable way
        // for compressed images)
        for( i = 0; i < stripCount; i++ )
        {
            int limit = y + rowsPerStrip;

            if( limit > height )
                limit = height;

            stripOffsets[i] = m_strm.GetPos();

            for( ; y < limit; y++, data += step )
            {
                if( channels == 3 )
                    icvCvt_BGR2RGB_8u_C3R( data, 0, buffer, 0, cvSize(width,1) );
                else if( channels == 4 )
                    icvCvt_BGRA2RGBA_8u_C4R( data, 0, buffer, 0, cvSize(width,1) );

                m_strm.PutBytes( channels > 1 ? buffer : data, fileStep );
            }

            stripCounts[i] = (short)(m_strm.GetPos() - stripOffsets[i]);
            /*assert( stripCounts[i] == uncompressedRowSize ||
                    stripCounts[i] < uncompressedRowSize &&
                    i == stripCount - 1);*/
        }

        if( stripCount > 2 )
        {
            stripOffsetsOffset = m_strm.GetPos();
            for( i = 0; i < stripCount; i++ )
                m_strm.PutDWord( stripOffsets[i] );

            stripCountsOffset = m_strm.GetPos();
            for( i = 0; i < stripCount; i++ )
                m_strm.PutWord( stripCounts[i] );
        }
        else if(stripCount == 2)
        {
            stripOffsetsOffset = m_strm.GetPos();
            for (i = 0; i < stripCount; i++)
            {
                m_strm.PutDWord (stripOffsets [i]);
            }
            stripCountsOffset = stripCounts [0] + (stripCounts [1] << 16);
        }
        else
        {
            stripOffsetsOffset = stripOffsets[0];
            stripCountsOffset = stripCounts[0];
        }

        if( channels > 1 )
        {
            bitsPerSample = m_strm.GetPos();
            m_strm.PutWord(8);
            m_strm.PutWord(8);
            m_strm.PutWord(8);
            if( channels == 4 )
                m_strm.PutWord(8);
        }

        directoryOffset = m_strm.GetPos();

        // write header
        m_strm.PutWord( 9 );

        /* warning: specification 5.0 of Tiff want to have tags in
           ascending order. This is a non-fatal error, but this cause
           warning with some tools. So, keep this in ascending order */

        WriteTag( TIFF_TAG_WIDTH, TIFF_TYPE_LONG, 1, width );
        WriteTag( TIFF_TAG_HEIGHT, TIFF_TYPE_LONG, 1, height );
        WriteTag( TIFF_TAG_BITS_PER_SAMPLE,
                  TIFF_TYPE_SHORT, channels, bitsPerSample );
        WriteTag( TIFF_TAG_COMPRESSION, TIFF_TYPE_LONG, 1, TIFF_UNCOMP );
        WriteTag( TIFF_TAG_PHOTOMETRIC, TIFF_TYPE_SHORT, 1, channels > 1 ? 2 : 1 );

        WriteTag( TIFF_TAG_STRIP_OFFSETS, TIFF_TYPE_LONG,
                  stripCount, stripOffsetsOffset );

        WriteTag( TIFF_TAG_SAMPLES_PER_PIXEL, TIFF_TYPE_SHORT, 1, channels );
        WriteTag( TIFF_TAG_ROWS_PER_STRIP, TIFF_TYPE_LONG, 1, rowsPerStrip );
        
        WriteTag( TIFF_TAG_STRIP_COUNTS,
                  stripCount > 1 ? TIFF_TYPE_SHORT : TIFF_TYPE_LONG,
                  stripCount, stripCountsOffset );

        m_strm.PutDWord(0);
        m_strm.Close();

        // write directory offset
        FILE* f = fopen( m_filename, "r+b" );
        buffer[0] = (uchar)directoryOffset;
        buffer[1] = (uchar)(directoryOffset >> 8);
        buffer[2] = (uchar)(directoryOffset >> 16);
        buffer[3] = (uchar)(directoryOffset >> 24);

        fseek( f, 4, SEEK_SET );
        fwrite( buffer, 1, 4, f );
        fclose(f);

        delete[]  stripOffsets;
        delete[]  stripCounts;
        delete[] buffer;

        result = true;
    }
    return result;
}

// JPEG filter factory

GrFmtJpeg::GrFmtJpeg()
{
    m_sign_len = 3;
    m_signature = "\xFF\xD8\xFF";
    m_description = "JPEG files (*.jpeg;*.jpg;*.jpe)";
}


GrFmtJpeg::~GrFmtJpeg()
{
}


GrFmtReader* GrFmtJpeg::NewReader( const char* filename )
{
    return new GrFmtJpegReader( filename );
}


GrFmtWriter* GrFmtJpeg::NewWriter( const char* filename )
{
    return new GrFmtJpegWriter( filename );
}


#ifdef HAVE_JPEG

/****************************************************************************************\
    This part of the file implements JPEG codec on base of IJG libjpeg library,
    in particular, this is the modified example.doc from libjpeg package.
    See otherlibs/_graphics/readme.txt for copyright notice.
\****************************************************************************************/

#include <stdio.h>
#include <setjmp.h>

#ifdef WIN32

#define XMD_H // prevent redefinition of INT32
#undef FAR  // prevent FAR redefinition

#endif

#if defined WIN32 && defined __GNUC__
typedef unsigned char boolean;
#endif

extern "C" {
#include "jpeglib.h"
}

/////////////////////// Error processing /////////////////////

typedef struct GrFmtJpegErrorMgr
{
    struct jpeg_error_mgr pub;    /* "parent" structure */
    jmp_buf setjmp_buffer;        /* jump label */
}
GrFmtJpegErrorMgr;


METHODDEF(void)
error_exit( j_common_ptr cinfo )
{
    GrFmtJpegErrorMgr* err_mgr = (GrFmtJpegErrorMgr*)(cinfo->err);
    
    /* Return control to the setjmp point */
    longjmp( err_mgr->setjmp_buffer, 1 );
}


/////////////////////// GrFmtJpegReader ///////////////////


GrFmtJpegReader::GrFmtJpegReader( const char* filename ) : GrFmtReader( filename )
{
    m_cinfo = 0;
    m_f = 0;
}


GrFmtJpegReader::~GrFmtJpegReader()
{
}


void  GrFmtJpegReader::Close()
{
    if( m_f )
    {
        fclose( m_f );
        m_f = 0;
    }

    if( m_cinfo )
    {
        jpeg_decompress_struct* cinfo = (jpeg_decompress_struct*)m_cinfo;
        GrFmtJpegErrorMgr* jerr = (GrFmtJpegErrorMgr*)m_jerr;

        jpeg_destroy_decompress( cinfo );
        delete cinfo;
        delete jerr;
        m_cinfo = 0;
        m_jerr = 0;
    }
    GrFmtReader::Close();
}


bool  GrFmtJpegReader::ReadHeader()
{
    bool result = false;
    Close();

    jpeg_decompress_struct* cinfo = new jpeg_decompress_struct;
    GrFmtJpegErrorMgr* jerr = new GrFmtJpegErrorMgr;

    cinfo->err = jpeg_std_error(&jerr->pub);
    jerr->pub.error_exit = error_exit;

    m_cinfo = cinfo;
    m_jerr = jerr;

    if( setjmp( jerr->setjmp_buffer ) == 0 )
    {
        jpeg_create_decompress( cinfo );

        m_f = fopen( m_filename, "rb" );
        if( m_f )
        {
            jpeg_stdio_src( cinfo, m_f );
            jpeg_read_header( cinfo, TRUE );

            m_width = cinfo->image_width;
            m_height = cinfo->image_height;
            m_iscolor = cinfo->num_components > 1;

            result = true;
        }
    }

    if( !result )
        Close();

    return result;
}

/***************************************************************************
 * following code is for supporting MJPEG image files
 * based on a message of Laurent Pinchart on the video4linux mailing list
 ***************************************************************************/

/* JPEG DHT Segment for YCrCb omitted from MJPEG data */
static
unsigned char my_jpeg_odml_dht[0x1a4] = {
    0xff, 0xc4, 0x01, 0xa2,

    0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,

    0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,

    0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04,
    0x04, 0x00, 0x00, 0x01, 0x7d,
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
    0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1,
    0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a,
    0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65,
    0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
    0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
    0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
    0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa,

    0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04,
    0x04, 0x00, 0x01, 0x02, 0x77,
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
    0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09,
    0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17,
    0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83,
    0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
    0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
    0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6,
    0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

/*
 * Parse the DHT table.
 * This code comes from jpeg6b (jdmarker.c).
 */
static
int my_jpeg_load_dht (struct jpeg_decompress_struct *info, unsigned char *dht,
              JHUFF_TBL *ac_tables[], JHUFF_TBL *dc_tables[])
{
    unsigned int length = (dht[2] << 8) + dht[3] - 2;
    unsigned int pos = 4;
    unsigned int count, i;
    int index;

    JHUFF_TBL **hufftbl;
    unsigned char bits[17];
    unsigned char huffval[256];

    while (length > 16)
    {
       bits[0] = 0;
       index = dht[pos++];
       count = 0;
       for (i = 1; i <= 16; ++i)
       {
           bits[i] = dht[pos++];
           count += bits[i];
       }
       length -= 17;

       if (count > 256 || count > length)
           return -1;
    
       for (i = 0; i < count; ++i)
           huffval[i] = dht[pos++];
       length -= count;

       if (index & 0x10)
       {
           index -= 0x10;
           hufftbl = &ac_tables[index];
       }
       else
           hufftbl = &dc_tables[index];
    
       if (index < 0 || index >= NUM_HUFF_TBLS)
           return -1;
    
       if (*hufftbl == NULL)
           *hufftbl = jpeg_alloc_huff_table ((j_common_ptr)info);
       if (*hufftbl == NULL)
           return -1;

       memcpy ((*hufftbl)->bits, bits, sizeof (*hufftbl)->bits);
       memcpy ((*hufftbl)->huffval, huffval, sizeof (*hufftbl)->huffval);
    }
    
    if (length != 0)
       return -1;
    
    return 0;
}

/***************************************************************************
 * end of code for supportting MJPEG image files
 * based on a message of Laurent Pinchart on the video4linux mailing list
 ***************************************************************************/

bool  GrFmtJpegReader::ReadData( uchar* data, int step, int color )
{
    bool result = false;

    color = color > 0 || (m_iscolor && color < 0);
    
    if( m_cinfo && m_jerr && m_width && m_height )
    {
        jpeg_decompress_struct* cinfo = (jpeg_decompress_struct*)m_cinfo;
        GrFmtJpegErrorMgr* jerr = (GrFmtJpegErrorMgr*)m_jerr;
        JSAMPARRAY buffer = 0;
        
        if( setjmp( jerr->setjmp_buffer ) == 0 )
        {
            /* check if this is a mjpeg image format */
            if ( cinfo->ac_huff_tbl_ptrs[0] == NULL &&
                cinfo->ac_huff_tbl_ptrs[1] == NULL &&
                cinfo->dc_huff_tbl_ptrs[0] == NULL &&
                cinfo->dc_huff_tbl_ptrs[1] == NULL )
            {
                /* yes, this is a mjpeg image format, so load the correct
                huffman table */
                my_jpeg_load_dht( cinfo,
                    my_jpeg_odml_dht,
                    cinfo->ac_huff_tbl_ptrs,
                    cinfo->dc_huff_tbl_ptrs );
            }

            if( color > 0 || (m_iscolor && color < 0) )
            {
                color = 1;
                if( cinfo->num_components != 4 )
                {
                    cinfo->out_color_space = JCS_RGB;
                    cinfo->out_color_components = 3;
                }
                else
                {
                    cinfo->out_color_space = JCS_CMYK;
                    cinfo->out_color_components = 4;
                }
            }
            else
            {
                color = 0;
                if( cinfo->num_components != 4 )
                {
                    cinfo->out_color_space = JCS_GRAYSCALE;
                    cinfo->out_color_components = 1;
                }
                else
                {
                    cinfo->out_color_space = JCS_CMYK;
                    cinfo->out_color_components = 4;
                }
            }

            jpeg_start_decompress( cinfo );

            buffer = (*cinfo->mem->alloc_sarray)((j_common_ptr)cinfo,
                                              JPOOL_IMAGE, m_width*4, 1 );

            for( ; m_height--; data += step )
            {
                jpeg_read_scanlines( cinfo, buffer, 1 );
                if( color )
                {
                    if( cinfo->out_color_components == 3 )
                        icvCvt_RGB2BGR_8u_C3R( buffer[0], 0, data, 0, cvSize(m_width,1) );
                    else
                        icvCvt_CMYK2BGR_8u_C4C3R( buffer[0], 0, data, 0, cvSize(m_width,1) );
                }
                else
                {
                    if( cinfo->out_color_components == 1 )
                        memcpy( data, buffer[0], m_width );
                    else
                        icvCvt_CMYK2Gray_8u_C4C1R( buffer[0], 0, data, 0, cvSize(m_width,1) );
                }
            }
            result = true;
            jpeg_finish_decompress( cinfo );
        }
    }

    Close();
    return result;
}


/////////////////////// GrFmtJpegWriter ///////////////////

GrFmtJpegWriter::GrFmtJpegWriter( const char* filename ) : GrFmtWriter( filename )
{
}


GrFmtJpegWriter::~GrFmtJpegWriter()
{
}


bool  GrFmtJpegWriter::WriteImage( const uchar* data, int step,
                                   int width, int height, int /*depth*/, int _channels )
{
    const int default_quality = 95;
    struct jpeg_compress_struct cinfo;
    GrFmtJpegErrorMgr jerr;

    bool result = false;
    FILE* f = 0;
    int channels = _channels > 1 ? 3 : 1;
    uchar* buffer = 0; // temporary buffer for row flipping
    
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = error_exit;

    if( setjmp( jerr.setjmp_buffer ) == 0 )
    {
        jpeg_create_compress(&cinfo);
        f = fopen( m_filename, "wb" );

        if( f )
        {
            jpeg_stdio_dest( &cinfo, f );

            cinfo.image_width = width;
            cinfo.image_height = height;
            cinfo.input_components = channels;
            cinfo.in_color_space = channels > 1 ? JCS_RGB : JCS_GRAYSCALE;

            jpeg_set_defaults( &cinfo );
            jpeg_set_quality( &cinfo, default_quality,
                              TRUE /* limit to baseline-JPEG values */ );
            jpeg_start_compress( &cinfo, TRUE );

            if( channels > 1 )
                buffer = new uchar[width*channels];

            for( ; height--; data += step )
            {
                uchar* ptr = (uchar*)data;
            
                if( _channels == 3 )
                {
                    icvCvt_BGR2RGB_8u_C3R( data, 0, buffer, 0, cvSize(width,1) );
                    ptr = buffer;
                }
                else if( _channels == 4 )
                {
                    icvCvt_BGRA2BGR_8u_C4C3R( data, 0, buffer, 0, cvSize(width,1), 2 );
                    ptr = buffer;
                }

                jpeg_write_scanlines( &cinfo, &ptr, 1 );
            }

            jpeg_finish_compress( &cinfo );
            result = true;
        }
    }

    if(f) fclose(f);
    jpeg_destroy_compress( &cinfo );

    delete[] buffer;
    return result;
}

#else

//////////////////////  JPEG-oriented two-level bitstream ////////////////////////

RJpegBitStream::RJpegBitStream()
{
}

RJpegBitStream::~RJpegBitStream()
{
    Close();
}


bool  RJpegBitStream::Open( const char* filename )
{
    Close();
    Allocate();
    
    m_is_opened = m_low_strm.Open( filename );
    if( m_is_opened ) SetPos(0);
    return m_is_opened;
}


void  RJpegBitStream::Close()
{
    m_low_strm.Close();
    m_is_opened = false;
}


void  RJpegBitStream::ReadBlock()
{
    uchar* end = m_start + m_block_size;
    uchar* current = m_start;

    if( setjmp( m_low_strm.JmpBuf()) == 0 )
    {
        int sz = m_unGetsize;
        memmove( current - sz, m_end - sz, sz );
        while( current < end )
        {
            int val = m_low_strm.GetByte();
            if( val != 0xff )
            {
                *current++ = (uchar)val;
            }
            else
            {
                val = m_low_strm.GetByte();
                if( val == 0 )
                    *current++ = 0xFF;
                else if( !(0xD0 <= val && val <= 0xD7) )
                {
                    m_low_strm.SetPos( m_low_strm.GetPos() - 2 );
                    goto fetch_end;
                }
            }
        }
fetch_end: ;
    }
    else
    {
        if( current == m_start && m_jmp_set )
            longjmp( m_jmp_buf, RBS_THROW_EOS );
    }
    m_current = m_start;
    m_end = m_start + (((current - m_start) + 3) & -4);
    if( !bsIsBigEndian() )
        bsBSwapBlock( m_start, m_end );
}


void  RJpegBitStream::Flush()
{
    m_end = m_start + m_block_size;
    m_current = m_end - 4;
    m_bit_idx = 0;
}

void  RJpegBitStream::AlignOnByte()
{
    m_bit_idx &= -8;
}

int  RJpegBitStream::FindMarker()
{
    int code = m_low_strm.GetWord();
    while( (code & 0xFF00) != 0xFF00 || (code == 0xFFFF || code == 0xFF00 ))
    {
        code = ((code&255) << 8) | m_low_strm.GetByte();
    }
    return code;
}


/****************************** JPEG (JFIF) reader ***************************/

// zigzag & IDCT prescaling (AAN algorithm) tables
static const uchar zigzag[] =
{
  0,  8,  1,  2,  9, 16, 24, 17, 10,  3,  4, 11, 18, 25, 32, 40,
 33, 26, 19, 12,  5,  6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35,
 28, 21, 14,  7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30,
 23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63,
 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63
};


static const int idct_prescale[] =
{
    16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
    22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
    21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
    19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
    16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
    12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
     8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
     4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
};


#define fixb         14
#define fix(x, n)    (int)((x)*(1 << (n)) + .5)
#define fix1(x, n)   (x)
#define fixmul(x)    (x)

#define C0_707     fix( 0.707106781f, fixb )
#define C0_924     fix( 0.923879533f, fixb )
#define C0_541     fix( 0.541196100f, fixb )
#define C0_382     fix( 0.382683432f, fixb )
#define C1_306     fix( 1.306562965f, fixb )

#define C1_082     fix( 1.082392200f, fixb )
#define C1_414     fix( 1.414213562f, fixb )
#define C1_847     fix( 1.847759065f, fixb )
#define C2_613     fix( 2.613125930f, fixb )

#define fixc       12
#define b_cb       fix( 1.772, fixc )
#define g_cb      -fix( 0.34414, fixc )
#define g_cr      -fix( 0.71414, fixc )
#define r_cr       fix( 1.402, fixc )

#define y_r        fix( 0.299, fixc )
#define y_g        fix( 0.587, fixc )
#define y_b        fix( 0.114, fixc )

#define cb_r      -fix( 0.1687, fixc )
#define cb_g      -fix( 0.3313, fixc )
#define cb_b       fix( 0.5,    fixc )

#define cr_r       fix( 0.5,    fixc )
#define cr_g      -fix( 0.4187, fixc )
#define cr_b      -fix( 0.0813, fixc )


// IDCT without prescaling
static void aan_idct8x8( int *src, int *dst, int step )
{
    int   workspace[64], *work = workspace;
    int   i;

    /* Pass 1: process rows */
    for( i = 8; i > 0; i--, src += 8, work += 8 )
    {
        /* Odd part */
        int  x0 = src[5], x1 = src[3];
        int  x2 = src[1], x3 = src[7];

        int  x4 = x0 + x1; x0 -= x1;

        x1 = x2 + x3; x2 -= x3;
        x3 = x1 + x4; x1 -= x4;

        x4 = (x0 + x2)*C1_847;
        x0 = descale( x4 - x0*C2_613, fixb);
        x2 = descale( x2*C1_082 - x4, fixb);
        x1 = descale( x1*C1_414, fixb);

        x0 -= x3;
        x1 -= x0;
        x2 += x1;

        work[7] = x3; work[6] = x0;
        work[5] = x1; work[4] = x2;

        /* Even part */
        x2 = src[2]; x3 = src[6];
        x0 = src[0]; x1 = src[4];

        x4 = x2 + x3;
        x2 = descale((x2-x3)*C1_414, fixb) - x4;

        x3 = x0 + x1; x0 -= x1;
        x1 = x3 + x4; x3 -= x4;
        x4 = x0 + x2; x0 -= x2;

        x2 = work[7];
        x1 -= x2; x2 = 2*x2 + x1;
        work[7] = x1; work[0] = x2;

        x2 = work[6];
        x1 = x4 + x2; x4 -= x2;
        work[1] = x1; work[6] = x4;

        x1 = work[5]; x2 = work[4];
        x4 = x0 + x1; x0 -= x1;
        x1 = x3 + x2; x3 -= x2;

        work[2] = x4; work[5] = x0;
        work[3] = x3; work[4] = x1;
    }

    /* Pass 2: process columns */
    work = workspace;
    for( i = 8; i > 0; i--, dst += step, work++ )
    {
        /* Odd part */
        int  x0 = work[8*5], x1 = work[8*3];
        int  x2 = work[8*1], x3 = work[8*7];

        int  x4 = x0 + x1; x0 -= x1;
        x1 = x2 + x3; x2 -= x3;
        x3 = x1 + x4; x1 -= x4;

        x4 = (x0 + x2)*C1_847;
        x0 = descale( x4 - x0*C2_613, fixb);
        x2 = descale( x2*C1_082 - x4, fixb);
        x1 = descale( x1*C1_414, fixb);

        x0 -= x3;
        x1 -= x0;
        x2 += x1;

        dst[7] = x3; dst[6] = x0;
        dst[5] = x1; dst[4] = x2;

        /* Even part */
        x2 = work[8*2]; x3 = work[8*6];
        x0 = work[8*0]; x1 = work[8*4];

        x4 = x2 + x3;
        x2 = descale((x2-x3)*C1_414, fixb) - x4;

        x3 = x0 + x1; x0 -= x1;
        x1 = x3 + x4; x3 -= x4;
        x4 = x0 + x2; x0 -= x2;

        x2 = dst[7];
        x1 -= x2; x2 = 2*x2 + x1;
        x1 = descale(x1,3);
        x2 = descale(x2,3);

        dst[7] = x1; dst[0] = x2;

        x2 = dst[6];
        x1 = descale(x4 + x2,3);
        x4 = descale(x4 - x2,3);
        dst[1] = x1; dst[6] = x4;

        x1 = dst[5]; x2 = dst[4];

        x4 = descale(x0 + x1,3);
        x0 = descale(x0 - x1,3);
        x1 = descale(x3 + x2,3);
        x3 = descale(x3 - x2,3);
       
        dst[2] = x4; dst[5] = x0;
        dst[3] = x3; dst[4] = x1;
    }
}


static const int max_dec_htable_size = 1 << 12;
static const int first_table_bits = 9;

GrFmtJpegReader::GrFmtJpegReader( const char* filename ) : GrFmtReader( filename )
{
    m_planes= -1;
    m_offset= -1;

    int i;
    for( i = 0; i < 4; i++ )
    {
        m_td[i] = new short[max_dec_htable_size];
        m_ta[i] = new short[max_dec_htable_size];
    }
}


GrFmtJpegReader::~GrFmtJpegReader()
{
    for( int i = 0; i < 4; i++ )
    {
        delete[] m_td[i];
        m_td[i] = 0;
        delete[] m_ta[i];
        m_ta[i] = 0;
    }
}


void  GrFmtJpegReader::Close()
{
    m_strm.Close();
    GrFmtReader::Close();
}


bool GrFmtJpegReader::ReadHeader()
{
    char buffer[16];
    int  i;
    bool result = false, is_sof = false, 
         is_qt = false, is_ht = false, is_sos = false;
    
    assert( strlen(m_filename) != 0 );
    if( !m_strm.Open( m_filename )) return false;

    memset( m_is_tq, 0, sizeof(m_is_tq));
    memset( m_is_td, 0, sizeof(m_is_td));
    memset( m_is_ta, 0, sizeof(m_is_ta));
    m_MCUs = 0;

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        RMByteStream& lstrm = m_strm.m_low_strm;
        
        lstrm.Skip( 2 ); // skip SOI marker
        
        for(;;)
        {
            int marker = m_strm.FindMarker() & 255;

            // check for standalone markers
            if( marker != 0xD8 /* SOI */ && marker != 0xD9 /* EOI */ &&
                marker != 0x01 /* TEM */ && !( 0xD0 <= marker && marker <= 0xD7 ))
            {
                int pos    = lstrm.GetPos();
                int length = lstrm.GetWord();
            
                switch( marker )
                {
                case 0xE0: // APP0
                    lstrm.GetBytes( buffer, 5 );
                    if( strcmp(buffer, "JFIF") == 0 ) // JFIF identification
                    {
                        m_version = lstrm.GetWord();
                        //is_jfif = true;
                    }
                    break;

                case 0xC0: // SOF0
                    m_precision = lstrm.GetByte();
                    m_height = lstrm.GetWord();
                    m_width = lstrm.GetWord();
                    m_planes = lstrm.GetByte();

                    if( m_width == 0 || m_height == 0 || // DNL not supported
                       (m_planes != 1 && m_planes != 3)) goto parsing_end;

                    m_iscolor = m_planes == 3;
                
                    memset( m_ci, -1, sizeof(m_ci));

                    for( i = 0; i < m_planes; i++ )
                    {
                        int idx = lstrm.GetByte();

                        if( idx < 1 || idx > m_planes ) // wrong index
                        {
                            idx = i+1; // hack
                        }
                        cmp_info& ci = m_ci[idx-1];
                        
                        if( ci.tq > 0 /* duplicated description */) goto parsing_end;

                        ci.h = (char)lstrm.GetByte();
                        ci.v = (char)(ci.h & 15);
                        ci.h >>= 4;
                        ci.tq = (char)lstrm.GetByte();
                        if( !((ci.h == 1 || ci.h == 2 || ci.h == 4) &&
                              (ci.v == 1 || ci.v == 2 || ci.v == 4) &&
                              ci.tq < 3) ||
                            // chroma mcu-parts should have equal sizes and
                            // be non greater then luma sizes
                            !( i != 2 || (ci.h == m_ci[1].h && ci.v == m_ci[1].v &&
                                          ci.h <= m_ci[0].h && ci.v <= m_ci[0].v)))
                            goto parsing_end;
                    }
                    is_sof = true;
                    m_type = marker - 0xC0;
                    break;

                case 0xDB: // DQT
                    if( !LoadQuantTables( length )) goto parsing_end;
                    is_qt = true;
                    break;

                case 0xC4: // DHT
                    if( !LoadHuffmanTables( length )) goto parsing_end;
                    is_ht = true;
                    break;

                case 0xDA: // SOS
                    is_sos = true;
                    m_offset = pos - 2;
                    goto parsing_end;

                case 0xDD: // DRI
                    m_MCUs = lstrm.GetWord();
                    break;
                }
                lstrm.SetPos( pos + length );
            }
        }
parsing_end: ;
    }

    result = /*is_jfif &&*/ is_sof && is_qt && is_ht && is_sos;
    if( !result )
    {
        m_width = m_height = -1;
        m_offset = -1;
        m_strm.Close();
    }
    return result;
}


bool GrFmtJpegReader::LoadQuantTables( int length )
{
    uchar buffer[128];
    int  i, tq_size;
    
    RMByteStream& lstrm = m_strm.m_low_strm;
    length -= 2;

    while( length > 0 )
    {
        int tq = lstrm.GetByte();
        int size = tq >> 4;
        tq &= 15;

        tq_size = (64<<size) + 1; 
        if( tq > 3 || size > 1 || length < tq_size ) return false;
        length -= tq_size;

        lstrm.GetBytes( buffer, tq_size - 1 );
        
        if( size == 0 ) // 8 bit quant factors
        {
            for( i = 0; i < 64; i++ )
            {
                int idx = zigzag[i];
                m_tq[tq][idx] = buffer[i] * 16 * idct_prescale[idx];
            }
        }
        else // 16 bit quant factors
        {
            for( i = 0; i < 64; i++ )
            {
                int idx = zigzag[i];
                m_tq[tq][idx] = ((unsigned short*)buffer)[i] * idct_prescale[idx];
            }
        }
        m_is_tq[tq] = true;
    }

    return true;
}


bool GrFmtJpegReader::LoadHuffmanTables( int length )
{
    const int max_bits = 16;
    uchar buffer[1024];
    int  buffer2[1024];

    int  i, ht_size;
    RMByteStream& lstrm = m_strm.m_low_strm;
    length -= 2;

    while( length > 0 )
    {
        int t = lstrm.GetByte();
        int hclass = t >> 4;
        t &= 15;

        if( t > 3 || hclass > 1 || length < 17 ) return false;
        length -= 17;

        lstrm.GetBytes( buffer, max_bits );
        for( i = 0, ht_size = 0; i < max_bits; i++ ) ht_size += buffer[i];

        if( length < ht_size ) return false;
        length -= ht_size;

        lstrm.GetBytes( buffer + max_bits, ht_size );
        
        if( !::bsCreateDecodeHuffmanTable( 
                  ::bsCreateSourceHuffmanTable( 
                        buffer, buffer2, max_bits, first_table_bits ),
                        hclass == 0 ? m_td[t] : m_ta[t], 
                        max_dec_htable_size )) return false;
        if( hclass == 0 )
            m_is_td[t] = true;
        else
            m_is_ta[t] = true;
    }
    return true;
}


bool GrFmtJpegReader::ReadData( uchar* data, int step, int color )
{
    if( m_offset < 0 || !m_strm.IsOpened())
        return false;

    if( setjmp( m_strm.JmpBuf()) == 0 )
    {
        RMByteStream& lstrm = m_strm.m_low_strm;
        lstrm.SetPos( m_offset );

        for(;;)
        {
            int marker = m_strm.FindMarker() & 255;

            if( marker == 0xD8 /* SOI */ || marker == 0xD9 /* EOI */ )
                goto decoding_end;

            // check for standalone markers
            if( marker != 0x01 /* TEM */ && !( 0xD0 <= marker && marker <= 0xD7 ))
            {
                int pos    = lstrm.GetPos();
                int length = lstrm.GetWord();
            
                switch( marker )
                {
                case 0xC4: // DHT
                    if( !LoadHuffmanTables( length )) goto decoding_end;
                    break;

                case 0xDA: // SOS
                    // read scan header
                    {
                        int idx[3] = { -1, -1, -1 };
                        int i, ns = lstrm.GetByte();
                        int sum = 0, a; // spectral selection & approximation

                        if( ns != m_planes ) goto decoding_end;
                        for( i = 0; i < ns; i++ )
                        {
                            int td, ta, c = lstrm.GetByte() - 1;
                            if( c < 0 || m_planes <= c )
                            {
                                c = i; // hack
                            }
                            
                            if( idx[c] != -1 ) goto decoding_end;
                            idx[i] = c;
                            td = lstrm.GetByte();
                            ta = td & 15;
                            td >>= 4;
                            if( !(ta <= 3 && m_is_ta[ta] && 
                                  td <= 3 && m_is_td[td] &&
                                  m_is_tq[m_ci[c].tq]) )
                                goto decoding_end;

                            m_ci[c].td = (char)td;
                            m_ci[c].ta = (char)ta;

                            sum += m_ci[c].h*m_ci[c].v;
                        }

                        if( sum > 10 ) goto decoding_end;

                        m_ss = lstrm.GetByte();
                        m_se = lstrm.GetByte();

                        a = lstrm.GetByte();
                        m_al = a & 15;
                        m_ah = a >> 4;

                        ProcessScan( idx, ns, data, step, color );
                        goto decoding_end; // only single scan case is supported now
                    }

                    //m_offset = pos - 2;
                    //break;

                case 0xDD: // DRI
                    m_MCUs = lstrm.GetWord();
                    break;
                }
                
                if( marker != 0xDA ) lstrm.SetPos( pos + length );
            }
        }
decoding_end: ;
    }

    return true;
}


void  GrFmtJpegReader::ResetDecoder()
{
    m_ci[0].dc_pred = m_ci[1].dc_pred = m_ci[2].dc_pred = 0; 
}

void  GrFmtJpegReader::ProcessScan( int* idx, int ns, uchar* data, int step, int color )
{
    int   i, s = 0, mcu, x1 = 0, y1 = 0;
    int   temp[64];
    int   blocks[10][64];
    int   pos[3], h[3], v[3];
    int   x_shift = 0, y_shift = 0;
    int   nch = color ? 3 : 1;

    assert( ns == m_planes && m_ss == 0 && m_se == 63 &&
            m_al == 0 && m_ah == 0 ); // sequental & single scan

    assert( idx[0] == 0 && (ns ==1 || (idx[1] == 1 && idx[2] == 2)));

    for( i = 0; i < ns; i++ )
    {
        int c = idx[i];
        h[c] = m_ci[c].h*8;
        v[c] = m_ci[c].v*8;
        pos[c] = s >> 6;
        s += h[c]*v[c];
    }

    if( ns == 3 )
    {
        x_shift = h[0]/(h[1]*2);
        y_shift = v[0]/(v[1]*2);
    }

    m_strm.Flush();
    ResetDecoder();

    for( mcu = 0;; mcu++ )
    {
        int  x2, y2, x, y, xc;
        int* cmp;
        uchar* data1;
        
        if( mcu == m_MCUs && m_MCUs != 0 )
        {
            ResetDecoder();
            m_strm.AlignOnByte();
            mcu = 0;
        }

        // Get mcu
        for( i = 0; i < ns; i++ )
        {
            int  c = idx[i];
            cmp = blocks[pos[c]];
            for( y = 0; y < v[c]; y += 8, cmp += h[c]*8 )
                for( x = 0; x < h[c]; x += 8 )
                {
                    GetBlock( temp, c );
                    if( i < (color ? 3 : 1))
                    {
                        aan_idct8x8( temp, cmp + x, h[c] );
                    }
                }
        }

        y2 = v[0];
        x2 = h[0];

        if( y1 + y2 > m_height ) y2 = m_height - y1;
        if( x1 + x2 > m_width ) x2 = m_width - x1;

        cmp = blocks[0];
        data1 = data + x1*nch;

        if( ns == 1 )
            for( y = 0; y < y2; y++, data1 += step, cmp += h[0] )
            {
                if( color )
                {
                    for( x = 0; x < x2; x++ )
                    {
                        int val = descale( cmp[x] + 128*4, 2 );
                        data1[x*3] = data1[x*3 + 1] = data1[x*3 + 2] = saturate( val );
                    }
                }
                else
                {
                    for( x = 0; x < x2; x++ )
                    {
                        int val = descale( cmp[x] + 128*4, 2 );
                        data1[x] = saturate( val );
                    }
                }
            }
        else
        {
            for( y = 0; y < y2; y++, data1 += step, cmp += h[0] )
            {
                if( color )
                {
                    int  shift = h[1]*(y >> y_shift);
                    int* cmpCb = blocks[pos[1]] + shift; 
                    int* cmpCr = blocks[pos[2]] + shift;
                    x = 0;
                    if( x_shift == 0 )
                    {
                        for( ; x < x2; x++ )
                        {
                            int Y  = (cmp[x] + 128*4) << fixc;
                            int Cb = cmpCb[x];
                            int Cr = cmpCr[x];
                            int t = (Y + Cb*b_cb) >> (fixc + 2);
                            data1[x*3] = saturate(t);
                            t = (Y + Cb*g_cb + Cr*g_cr) >> (fixc + 2);
                            data1[x*3 + 1] = saturate(t);
                            t = (Y + Cr*r_cr) >> (fixc + 2);
                            data1[x*3 + 2] = saturate(t);
                        }
                    }
                    else if( x_shift == 1 )
                    {
                        for( xc = 0; x <= x2 - 2; x += 2, xc++ )
                        {
                            int Y  = (cmp[x] + 128*4) << fixc;
                            int Cb = cmpCb[xc];
                            int Cr = cmpCr[xc];
                            int t = (Y + Cb*b_cb) >> (fixc + 2);
                            data1[x*3] = saturate(t);
                            t = (Y + Cb*g_cb + Cr*g_cr) >> (fixc + 2);
                            data1[x*3 + 1] = saturate(t);
                            t = (Y + Cr*r_cr) >> (fixc + 2);
                            data1[x*3 + 2] = saturate(t);
                            Y = (cmp[x+1] + 128*4) << fixc;
                            t = (Y + Cb*b_cb) >> (fixc + 2);
                            data1[x*3 + 3] = saturate(t);
                            t = (Y + Cb*g_cb + Cr*g_cr) >> (fixc + 2);
                            data1[x*3 + 4] = saturate(t);
                            t = (Y + Cr*r_cr) >> (fixc + 2);
                            data1[x*3 + 5] = saturate(t);
                        }
                    }
                    for( ; x < x2; x++ )
                    {
                        int Y  = (cmp[x] + 128*4) << fixc;
                        int Cb = cmpCb[x >> x_shift];
                        int Cr = cmpCr[x >> x_shift];
                        int t = (Y + Cb*b_cb) >> (fixc + 2);
                        data1[x*3] = saturate(t);
                        t = (Y + Cb*g_cb + Cr*g_cr) >> (fixc + 2);
                        data1[x*3 + 1] = saturate(t);
                        t = (Y + Cr*r_cr) >> (fixc + 2);
                        data1[x*3 + 2] = saturate(t);
                    }
                }
                else
                {
                    for( x = 0; x < x2; x++ )
                    {
                        int val = descale( cmp[x] + 128*4, 2 );
                        data1[x] = saturate(val);
                    }
                }
            }
        }

        x1 += h[0];
        if( x1 >= m_width )
        {
            x1 = 0;
            y1 += v[0];
            data += v[0]*step;
            if( y1 >= m_height ) break;
        }
    }
}


void  GrFmtJpegReader::GetBlock( int* block, int c )
{
    memset( block, 0, 64*sizeof(block[0]) );

    assert( 0 <= c && c < 3 );
    const short* td = m_td[m_ci[c].td];
    const short* ta = m_ta[m_ci[c].ta];
    const int* tq = m_tq[m_ci[c].tq];

    // Get DC coefficient
    int i = 0, cat  = m_strm.GetHuff( td );
    int mask = bs_bit_mask[cat];
    int val  = m_strm.Get( cat );

    val -= (val*2 <= mask ? mask : 0);
    m_ci[c].dc_pred = val += m_ci[c].dc_pred;
    
    block[0] = descale(val * tq[0],16);

    // Get AC coeffs
    for(;;)
    {
        cat = m_strm.GetHuff( ta );
        if( cat == 0 ) break; // end of block

        i += (cat >> 4) + 1;
        cat &= 15;
        mask = bs_bit_mask[cat];
        val  = m_strm.Get( cat );
        cat  = zigzag[i];
        val -= (val*2 <= mask ? mask : 0);
        block[cat] = descale(val * tq[cat], 16);
        assert( i <= 63 );
        if( i >= 63 ) break;
    }
}

////////////////////// WJpegStream ///////////////////////

WJpegBitStream::WJpegBitStream()
{
}


WJpegBitStream::~WJpegBitStream()
{
    Close();
    m_is_opened = false;
}



bool  WJpegBitStream::Open( const char* filename )
{
    Close();
    Allocate();
    
    m_is_opened = m_low_strm.Open( filename );
    if( m_is_opened )
    {
        m_block_pos = 0;
        ResetBuffer();
    }
    return m_is_opened;
}


void  WJpegBitStream::Close()
{
    if( m_is_opened )
    {
        Flush();
        m_low_strm.Close();
        m_is_opened = false;
    }
}


void  WJpegBitStream::Flush()
{
    Put( -1, m_bit_idx & 31 );
    *((ulong*&)m_current)++ = m_val;
    WriteBlock();
    ResetBuffer();
}


void  WJpegBitStream::WriteBlock()
{
    uchar* ptr = m_start;
    if( !bsIsBigEndian() )
        bsBSwapBlock( m_start, m_current );

    while( ptr < m_current )
    {
        int val = *ptr++;
        m_low_strm.PutByte( val );
        if( val == 0xff )
        {
            m_low_strm.PutByte( 0 );
        }
    }
    
    m_current = m_start;
}


/////////////////////// GrFmtJpegWriter ///////////////////

GrFmtJpegWriter::GrFmtJpegWriter( const char* filename ) : GrFmtWriter( filename )
{
}

GrFmtJpegWriter::~GrFmtJpegWriter()
{
}

//  Standard JPEG quantization tables
static const uchar jpegTableK1_T[] =
{
    16, 12, 14, 14,  18,  24,  49,  72,
    11, 12, 13, 17,  22,  35,  64,  92,
    10, 14, 16, 22,  37,  55,  78,  95,
    16, 19, 24, 29,  56,  64,  87,  98,
    24, 26, 40, 51,  68,  81, 103, 112,
    40, 58, 57, 87, 109, 104, 121, 100,
    51, 60, 69, 80, 103, 113, 120, 103,
    61, 55, 56, 62,  77,  92, 101,  99
};


static const uchar jpegTableK2_T[] =
{
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};


// Standard Huffman tables

// ... for luma DCs.
static const uchar jpegTableK3[] =
{
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};


// ... for chroma DCs.
static const uchar jpegTableK4[] =
{
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};


// ... for luma ACs.
static const uchar jpegTableK5[] =
{
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125,
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
    0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

// ... for chroma ACs  
static const uchar jpegTableK6[] =
{
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119,
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};


static const char jpegHeader[] = 
    "\xFF\xD8"  // SOI  - start of image
    "\xFF\xE0"  // APP0 - jfif extention
    "\x00\x10"  // 2 bytes: length of APP0 segment
    "JFIF\x00"  // JFIF signature
    "\x01\x02"  // version of JFIF
    "\x00"      // units = pixels ( 1 - inch, 2 - cm )
    "\x00\x01\x00\x01" // 2 2-bytes values: x density & y density
    "\x00\x00"; // width & height of thumbnail: ( 0x0 means no thumbnail)

#define postshift 14

// FDCT with postscaling
static void aan_fdct8x8( int *src, int *dst,
                         int step, const int *postscale )
{
    int  workspace[64], *work = workspace;
    int  i;

    // Pass 1: process rows
    for( i = 8; i > 0; i--, src += step, work += 8 )
    {
        int x0 = src[0], x1 = src[7];
        int x2 = src[3], x3 = src[4];

        int x4 = x0 + x1; x0 -= x1;
        x1 = x2 + x3; x2 -= x3;
    
        work[7] = x0; work[1] = x2;
        x2 = x4 + x1; x4 -= x1;

        x0 = src[1]; x3 = src[6];
        x1 = x0 + x3; x0 -= x3;
        work[5] = x0;

        x0 = src[2]; x3 = src[5];
        work[3] = x0 - x3; x0 += x3;

        x3 = x0 + x1; x0 -= x1;
        x1 = x2 + x3; x2 -= x3;

        work[0] = x1; work[4] = x2;

        x0 = descale((x0 - x4)*C0_707, fixb);
        x1 = x4 + x0; x4 -= x0;
        work[2] = x4; work[6] = x1;

        x0 = work[1]; x1 = work[3];
        x2 = work[5]; x3 = work[7];

        x0 += x1; x1 += x2; x2 += x3;
        x1 = descale(x1*C0_707, fixb);

        x4 = x1 + x3; x3 -= x1;
        x1 = (x0 - x2)*C0_382;
        x0 = descale(x0*C0_541 + x1, fixb);
        x2 = descale(x2*C1_306 + x1, fixb);

        x1 = x0 + x3; x3 -= x0;
        x0 = x4 + x2; x4 -= x2;

        work[5] = x1; work[1] = x0;
        work[7] = x4; work[3] = x3;
    }

    work = workspace;
    // pass 2: process columns
    for( i = 8; i > 0; i--, work++, postscale += 8, dst += 8 )
    {
        int  x0 = work[8*0], x1 = work[8*7];
        int  x2 = work[8*3], x3 = work[8*4];

        int  x4 = x0 + x1; x0 -= x1;
        x1 = x2 + x3; x2 -= x3;
    
        work[8*7] = x0; work[8*0] = x2;
        x2 = x4 + x1; x4 -= x1;

        x0 = work[8*1]; x3 = work[8*6];
        x1 = x0 + x3; x0 -= x3;
        work[8*4] = x0;

        x0 = work[8*2]; x3 = work[8*5];
        work[8*3] = x0 - x3; x0 += x3;

        x3 = x0 + x1; x0 -= x1;
        x1 = x2 + x3; x2 -= x3;

        dst[0] = descale(x1*postscale[0], postshift);
        dst[4] = descale(x2*postscale[4], postshift);

        x0 = descale((x0 - x4)*C0_707, fixb);
        x1 = x4 + x0; x4 -= x0;

        dst[2] = descale(x4*postscale[2], postshift);
        dst[6] = descale(x1*postscale[6], postshift);

        x0 = work[8*0]; x1 = work[8*3];
        x2 = work[8*4]; x3 = work[8*7];

        x0 += x1; x1 += x2; x2 += x3;
        x1 = descale(x1*C0_707, fixb);

        x4 = x1 + x3; x3 -= x1;
        x1 = (x0 - x2)*C0_382;
        x0 = descale(x0*C0_541 + x1, fixb);
        x2 = descale(x2*C1_306 + x1, fixb);

        x1 = x0 + x3; x3 -= x0;
        x0 = x4 + x2; x4 -= x2;

        dst[5] = descale(x1*postscale[5], postshift);
        dst[1] = descale(x0*postscale[1], postshift);
        dst[7] = descale(x4*postscale[7], postshift);
        dst[3] = descale(x3*postscale[3], postshift);
    }
}


bool  GrFmtJpegWriter::WriteImage( const uchar* data, int step,
                                   int width, int height, int /*depth*/, int _channels )
{
    assert( data && width > 0 && height > 0 );
    
    if( !m_strm.Open( m_filename ) ) return false;

    // encode the header and tables
    // for each mcu:
    //   convert rgb to yuv with downsampling (if color).
    //   for every block:
    //     calc dct and quantize
    //     encode block.
    int x, y;
    int i, j;
    const int max_quality = 12;
    int   quality = max_quality;
    WMByteStream& lowstrm = m_strm.m_low_strm;
    int   fdct_qtab[2][64];
    ulong huff_dc_tab[2][16];
    ulong huff_ac_tab[2][256];
    int  channels = _channels > 1 ? 3 : 1;
    int  x_scale = channels > 1 ? 2 : 1, y_scale = x_scale;
    int  dc_pred[] = { 0, 0, 0 };
    int  x_step = x_scale * 8;
    int  y_step = y_scale * 8;
    int  block[6][64];
    int  buffer[1024];
    int  luma_count = x_scale*y_scale;
    int  block_count = luma_count + channels - 1;
    int  Y_step = x_scale*8;
    const int UV_step = 16;
    double inv_quality;

    if( quality < 3 ) quality = 3;
    if( quality > max_quality ) quality = max_quality;

    inv_quality = 1./quality;

    // Encode header
    lowstrm.PutBytes( jpegHeader, sizeof(jpegHeader) - 1 );
    
    // Encode quantization tables
    for( i = 0; i < (channels > 1 ? 2 : 1); i++ )
    {
        const uchar* qtable = i == 0 ? jpegTableK1_T : jpegTableK2_T;
        int chroma_scale = i > 0 ? luma_count : 1;
        
        lowstrm.PutWord( 0xffdb );   // DQT marker
        lowstrm.PutWord( 2 + 65*1 ); // put single qtable
        lowstrm.PutByte( 0*16 + i ); // 8-bit table

        // put coefficients
        for( j = 0; j < 64; j++ )
        {
            int idx = zigzag[j];
            int qval = cvRound(qtable[idx]*inv_quality);
            if( qval < 1 )
                qval = 1;
            if( qval > 255 )
                qval = 255;
            fdct_qtab[i][idx] = cvRound((1 << (postshift + 9))/
                                      (qval*chroma_scale*idct_prescale[idx]));
            lowstrm.PutByte( qval );
        }
    }

    // Encode huffman tables
    for( i = 0; i < (channels > 1 ? 4 : 2); i++ )
    {
        const uchar* htable = i == 0 ? jpegTableK3 : i == 1 ? jpegTableK5 :
                              i == 2 ? jpegTableK4 : jpegTableK6;
        int is_ac_tab = i & 1;
        int idx = i >= 2;
        int tableSize = 16 + (is_ac_tab ? 162 : 12);

        lowstrm.PutWord( 0xFFC4   );      // DHT marker
        lowstrm.PutWord( 3 + tableSize ); // define one huffman table
        lowstrm.PutByte( is_ac_tab*16 + idx ); // put DC/AC flag and table index
        lowstrm.PutBytes( htable, tableSize ); // put table

        bsCreateEncodeHuffmanTable( bsCreateSourceHuffmanTable(
            htable, buffer, 16, 9 ), is_ac_tab ? huff_ac_tab[idx] :
            huff_dc_tab[idx], is_ac_tab ? 256 : 16 );
    }

    // put frame header
    lowstrm.PutWord( 0xFFC0 );          // SOF0 marker
    lowstrm.PutWord( 8 + 3*channels );  // length of frame header
    lowstrm.PutByte( 8 );               // sample precision
    lowstrm.PutWord( height );
    lowstrm.PutWord( width );
    lowstrm.PutByte( channels );        // number of components

    for( i = 0; i < channels; i++ )
    {
        lowstrm.PutByte( i + 1 );  // (i+1)-th component id (Y,U or V)
        if( i == 0 )
            lowstrm.PutByte(x_scale*16 + y_scale); // chroma scale factors
        else
            lowstrm.PutByte(1*16 + 1);
        lowstrm.PutByte( i > 0 ); // quantization table idx
    }

    // put scan header
    lowstrm.PutWord( 0xFFDA );          // SOS marker
    lowstrm.PutWord( 6 + 2*channels );  // length of scan header
    lowstrm.PutByte( channels );        // number of components in the scan

    for( i = 0; i < channels; i++ )
    {
        lowstrm.PutByte( i+1 );             // component id
        lowstrm.PutByte( (i>0)*16 + (i>0) );// selection of DC & AC tables
    }

    lowstrm.PutWord(0*256 + 63);// start and end of spectral selection - for
                                // sequental DCT start is 0 and end is 63

    lowstrm.PutByte( 0 );  // successive approximation bit position 
                           // high & low - (0,0) for sequental DCT  

    // encode data
    for( y = 0; y < height; y += y_step, data += y_step*step )
    {
        for( x = 0; x < width; x += x_step )
        {
            int x_limit = x_step;
            int y_limit = y_step;
            const uchar* rgb_data = data + x*_channels;
            int* Y_data = block[0];

            if( x + x_limit > width ) x_limit = width - x;
            if( y + y_limit > height ) y_limit = height - y;

            memset( block, 0, block_count*64*sizeof(block[0][0]));
            
            if( channels > 1 )
            {
                int* UV_data = block[luma_count];

                for( i = 0; i < y_limit; i++, rgb_data += step, Y_data += Y_step )
                {
                    for( j = 0; j < x_limit; j++, rgb_data += _channels )
                    {
                        int r = rgb_data[2];
                        int g = rgb_data[1];
                        int b = rgb_data[0];

                        int Y = descale( r*y_r + g*y_g + b*y_b, fixc - 2) - 128*4;
                        int U = descale( r*cb_r + g*cb_g + b*cb_b, fixc - 2 );
                        int V = descale( r*cr_r + g*cr_g + b*cr_b, fixc - 2 );
                        int j2 = j >> (x_scale - 1); 

                        Y_data[j] = Y;
                        UV_data[j2] += U;
                        UV_data[j2 + 8] += V;
                    }

                    rgb_data -= x_limit*_channels;
                    if( ((i+1) & (y_scale - 1)) == 0 )
                    {
                        UV_data += UV_step;
                    }
                }
            }
            else
            {
                for( i = 0; i < y_limit; i++, rgb_data += step, Y_data += Y_step )
                {
                    for( j = 0; j < x_limit; j++ )
                        Y_data[j] = rgb_data[j]*4 - 128*4;
                }
            }

            for( i = 0; i < block_count; i++ )
            {
                int is_chroma = i >= luma_count;
                int src_step = x_scale * 8;
                int run = 0, val;
                int* src_ptr = block[i & -2] + (i & 1)*8;
                const ulong* htable = huff_ac_tab[is_chroma];

                aan_fdct8x8( src_ptr, buffer, src_step, fdct_qtab[is_chroma] );

                j = is_chroma + (i > luma_count);
                val = buffer[0] - dc_pred[j];
                dc_pred[j] = buffer[0];

                {
                float a = (float)val;
                int cat = (((int&)a >> 23) & 255) - (126 & (val ? -1 : 0));

                assert( cat <= 11 );
                m_strm.PutHuff( cat, huff_dc_tab[is_chroma] );
                m_strm.Put( val - (val < 0 ? 1 : 0), cat );
                }

                for( j = 1; j < 64; j++ )
                {
                    val = buffer[zigzag[j]];

                    if( val == 0 )
                    {
                        run++;
                    }
                    else
                    {
                        while( run >= 16 )
                        {
                            m_strm.PutHuff( 0xF0, htable ); // encode 16 zeros
                            run -= 16;
                        }

                        {
                        float a = (float)val;
                        int cat = (((int&)a >> 23) & 255) - (126 & (val ? -1 : 0));

                        assert( cat <= 10 );
                        m_strm.PutHuff( cat + run*16, htable );
                        m_strm.Put( val - (val < 0 ? 1 : 0), cat );
                        }

                        run = 0;
                    }
                }

                if( run )
                {
                    m_strm.PutHuff( 0x00, htable ); // encode EOB
                }
            }
        }
    }

    // Flush 
    m_strm.Flush();

    lowstrm.PutWord( 0xFFD9 ); // EOI marker
    m_strm.Close();

    return true;
}

#endif
