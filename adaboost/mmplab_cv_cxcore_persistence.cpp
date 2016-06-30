// 
#include "stdafx.h"
 #include "mmplab_cxcore.h"
 #include <ctype.h>
#include <float.h>
// 
// /****************************************************************************************\
// *                            Common macros and type definitions                          *
// \****************************************************************************************/
// 
 #define cv_isprint(c)     ((signed char)(c) >= (signed char)' ')
 #define cv_isprint_or_tab(c)  ((signed char)(c) >= (signed char)' ' || (c) == '\t')
// 
static char* icv_itoa( int _val, char* buffer, int /*radix*/ )
{
    const int radix = 10;
    char* ptr=buffer + 23 /* enough even for 64-bit integers */;
    unsigned val = abs(_val);

    *ptr = '\0';
    do
    {
        unsigned r = val / radix;
        *--ptr = (char)(val - (r*radix) + '0');
        val = r;
    }
    while( val != 0 );

    if( _val < 0 )
        *--ptr = '-';

    return ptr;
}
// 
// 
typedef struct CvGenericHash
{
    CV_SET_FIELDS()
    int tab_size;
    void** table;
}
CvGenericHash;

typedef CvGenericHash CvStringHash;

typedef struct CvFileMapNode
{
    CvFileNode value;
    const CvStringHashNode* key;
    struct CvFileMapNode* next;
}
CvFileMapNode;
// 
typedef struct CvXMLStackRecord
{
    CvMemStoragePos pos;
    CvString struct_tag;
    int struct_indent;
    int struct_flags;
}
CvXMLStackRecord;
// 
#define CV_XML_OPENING_TAG 1
#define CV_XML_CLOSING_TAG 2
#define CV_XML_EMPTY_TAG 3
#define CV_XML_HEADER_TAG 4
#define CV_XML_DIRECTIVE_TAG 5
// 
// //typedef void (*CvParse)( struct CvFileStorage* fs );
 typedef void (*CvStartWriteStruct)( struct CvFileStorage* fs, const char* key,
                                    int struct_flags, const char* type_name );
 typedef void (*CvEndWriteStruct)( struct CvFileStorage* fs );
 typedef void (*CvWriteInt)( struct CvFileStorage* fs, const char* key, int value );
 typedef void (*CvWriteReal)( struct CvFileStorage* fs, const char* key, double value );
 typedef void (*CvWriteString)( struct CvFileStorage* fs, const char* key,
                               const char* value, int quote );
 typedef void (*CvWriteComment)( struct CvFileStorage* fs, const char* comment, int eol_comment );
 typedef void (*CvStartNextStream)( struct CvFileStorage* fs );
// 
typedef struct CvFileStorage
{
    int flags;
    int is_xml;
    int write_mode;
    int is_first;
    CvMemStorage* memstorage;
    CvMemStorage* dststorage;
    CvMemStorage* strstorage;
    CvStringHash* str_hash;
    CvSeq* roots;
    CvSeq* write_stack;
    int struct_indent;
    int struct_flags;
    CvString struct_tag;
    int space;
    char* filename;
    FILE* file;
    char* buffer;
    char* buffer_start;
    char* buffer_end;
    int wrap_margin;
    int lineno;
    int dummy_eof;
    const char* errmsg;
    char errmsgbuf[128];

    CvStartWriteStruct start_write_struct;
    CvEndWriteStruct end_write_struct;
    CvWriteInt write_int;
    CvWriteReal write_real;
    CvWriteString write_string;
    CvWriteComment write_comment;
    CvStartNextStream start_next_stream;
    //CvParse parse;
}
CvFileStorage;
// 

#define CV_YML_INDENT  3
#define CV_XML_INDENT  2
#define CV_YML_INDENT_FLOW  1
#define CV_FS_MAX_LEN 4096

#define CV_FILE_STORAGE ('Y' + ('A' << 8) + ('M' << 16) + ('L' << 24))
#define CV_IS_FILE_STORAGE(fs) ((fs) != 0 && (fs)->flags == CV_FILE_STORAGE)
// 
#define CV_CHECK_FILE_STORAGE(fs)                       \
{                                                       \
    if( !CV_IS_FILE_STORAGE(fs) )                       \
        CV_ERROR( (fs) ? CV_StsBadArg : CV_StsNullPtr,  \
                  "Invalid pointer to file storage" );  \
}

#define CV_CHECK_OUTPUT_FILE_STORAGE(fs)                \
{                                                       \
    CV_CHECK_FILE_STORAGE(fs);                          \
    if( !fs->write_mode )                               \
        CV_ERROR( CV_StsError, "The file storage is opened for reading" ); \
}
// 
CV_IMPL const char*
cvAttrValue( const CvAttrList* attr, const char* attr_name )
{
    while( attr && attr->attr )
    {
        int i;
        for( i = 0; attr->attr[i*2] != 0; i++ )
        {
            if( strcmp( attr_name, attr->attr[i*2] ) == 0 )
                return attr->attr[i*2+1];
        }
        attr = attr->next;
    }

    return 0;
}
// 
// 
static CvGenericHash*
cvCreateMap( int flags, int header_size, int elem_size,
             CvMemStorage* storage, int start_tab_size )
{
    CvGenericHash* map = 0;

    CV_FUNCNAME( "cvCreateMap" );

    __BEGIN__;

    if( header_size < (int)sizeof(CvGenericHash) )
        CV_ERROR( CV_StsBadSize, "Too small map header_size" );

    if( start_tab_size <= 0 )
        start_tab_size = 16;

    CV_CALL( map = (CvGenericHash*)cvCreateSet( flags, header_size, elem_size, storage ));

    map->tab_size = start_tab_size;
    start_tab_size *= sizeof(map->table[0]);
    CV_CALL( map->table = (void**)cvMemStorageAlloc( storage, start_tab_size ));
    memset( map->table, 0, start_tab_size );

    __END__;

    if( cvGetErrStatus() < 0 )
        map = 0;

    return map;
}

// 
#define CV_PARSE_ERROR( errmsg )                                    \
{                                                                   \
    icvParseError( fs, cvFuncName, (errmsg), __FILE__, __LINE__ );  \
    EXIT;                                                           \
}
// 
// 
static void
icvParseError( CvFileStorage* fs, const char* func_name,
               const char* err_msg, const char* source_file, int source_line )
{
    char buf[1<<10];
    sprintf( buf, "%s(%d): %s", fs->filename, fs->lineno, err_msg );
    cvError( CV_StsParseError, func_name, buf, source_file, source_line );
}


static void
icvFSCreateCollection( CvFileStorage* fs, int tag, CvFileNode* collection )
{
    CV_FUNCNAME( "icvFSCreateCollection" );

    __BEGIN__;
    
    if( CV_NODE_IS_MAP(tag) )
    {
        if( collection->tag != CV_NODE_NONE )
        {
            assert( fs->is_xml != 0 );
            CV_PARSE_ERROR( "Sequence element should not have name (use <_></_>)" );
        }

        CV_CALL( collection->data.map = cvCreateMap( 0, sizeof(CvFileNodeHash),
                            sizeof(CvFileMapNode), fs->memstorage, 16 ));
    }
    else
    {
        CvSeq* seq;
        CV_CALL( seq = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvFileNode), fs->memstorage ));
        
        // if <collection> contains some scalar element, add it to the newly created collection
        if( CV_NODE_TYPE(collection->tag) != CV_NODE_NONE )
            cvSeqPush( seq, collection );

        collection->data.seq = seq;
    }

    collection->tag = tag;
    cvSetSeqBlockSize( collection->data.seq, 8 );

    __END__;
}

// 

static char*
icvFSDoResize( CvFileStorage* fs, char* ptr, int len )
{
    char* new_ptr = 0;
    CV_FUNCNAME( "icvFSDoResize" );

    __BEGIN__;
    
    int written_len = (int)(ptr - fs->buffer_start);
    int new_size = (int)((fs->buffer_end - fs->buffer_start)*3/2);
    new_size = MAX( written_len + len, new_size );
    CV_CALL( new_ptr = (char*)cvAlloc( new_size + 256 ));
    fs->buffer = new_ptr + (fs->buffer - fs->buffer_start);
    if( written_len > 0 )
        memcpy( new_ptr, fs->buffer_start, written_len );
    fs->buffer_start = new_ptr;
    fs->buffer_end = fs->buffer_start + new_size;
    new_ptr += written_len;

    __END__;

    return new_ptr;
}
// 
// 
inline char* icvFSResizeWriteBuffer( CvFileStorage* fs, char* ptr, int len )
{
    return ptr + len < fs->buffer_end ? ptr : icvFSDoResize( fs, ptr, len );
}
// 
// 
static char*
icvFSFlush( CvFileStorage* fs )
{
    char* ptr = fs->buffer;
    int indent;

    if( ptr > fs->buffer_start + fs->space )
    {
        ptr[0] = '\n';
        ptr[1] = '\0';
        fputs( fs->buffer_start, fs->file );
        fs->buffer = fs->buffer_start;
    }

    indent = fs->struct_indent;

    if( fs->space != indent )
    {
        if( fs->space < indent )
            memset( fs->buffer_start + fs->space, ' ', indent - fs->space );
        fs->space = indent;
    }

    ptr = fs->buffer = fs->buffer_start + fs->space;

    return ptr;
}

// 
// /* closes file storage and deallocates buffers */
CV_IMPL  void
cvReleaseFileStorage( CvFileStorage** p_fs )
{
    CV_FUNCNAME("cvReleaseFileStorage" );

    __BEGIN__;

    if( !p_fs )
        CV_ERROR( CV_StsNullPtr, "NULL double pointer to file storage" );

    if( *p_fs )
    {
        CvFileStorage* fs = *p_fs;
        *p_fs = 0;

        if( fs->write_mode && fs->file )
        {
            if( fs->write_stack )
            {
                while( fs->write_stack->total > 0 )
                    cvEndWriteStruct(fs);
            }
            icvFSFlush(fs);
            if( fs->is_xml )
                fputs("</opencv_storage>\n", fs->file );
        }

        //icvFSReleaseCollection( fs->roots ); // delete all the user types recursively

        if( fs->file )
        {
            fclose( fs->file );
            fs->file = 0;
        }

        cvReleaseMemStorage( &fs->strstorage );

        cvFree( &fs->buffer_start );
        cvReleaseMemStorage( &fs->memstorage );

        memset( fs, 0, sizeof(*fs) );
        cvFree( &fs );
    }

    __END__;
}
// 
// 
#define CV_HASHVAL_SCALE 33
// 
CV_IMPL CvStringHashNode*
cvGetHashedKey( CvFileStorage* fs, const char* str, int len, int create_missing )
{
    CvStringHashNode* node = 0;
    CV_FUNCNAME( "cvGetHashedKey" );

    __BEGIN__;
    
    unsigned hashval = 0;
    int i, tab_size;
    CvStringHash* map = fs->str_hash;

    if( !fs )
        EXIT;

    if( len < 0 )
    {
        for( i = 0; str[i] != '\0'; i++ )
            hashval = hashval*CV_HASHVAL_SCALE + (unsigned char)str[i];
        len = i;
    }
    else for( i = 0; i < len; i++ )
        hashval = hashval*CV_HASHVAL_SCALE + (unsigned char)str[i];

    hashval &= INT_MAX;
    tab_size = map->tab_size;
    if( (tab_size & (tab_size - 1)) == 0 )
        i = (int)(hashval & (tab_size - 1));
    else
        i = (int)(hashval % tab_size);

    for( node = (CvStringHashNode*)(map->table[i]); node != 0; node = node->next )
    {
        if( node->hashval == hashval &&
            node->str.len == len &&
            memcmp( node->str.ptr, str, len ) == 0 )
            break;
    }

    if( !node && create_missing )
    {
        node = (CvStringHashNode*)cvSetNew( (CvSet*)map );
        node->hashval = hashval;
        CV_CALL( node->str = cvMemStorageAllocString( map->storage, str, len ));
        node->next = (CvStringHashNode*)(map->table[i]);
        map->table[i] = node;
    }

    __END__;

    return node;
}
// 

CV_IMPL CvFileNode*
cvGetFileNode( CvFileStorage* fs, CvFileNode* _map_node,
               const CvStringHashNode* key,
               int create_missing )
{
    CvFileNode* value = 0;
    
    CV_FUNCNAME( "cvGetFileNode" );

    __BEGIN__;

    int k = 0, attempts = 1;

    if( !fs )
        EXIT;

    CV_CHECK_FILE_STORAGE(fs);

    if( !key )
        CV_ERROR( CV_StsNullPtr, "Null key element" );

    if( _map_node )
    {
        if( !fs->roots )
            EXIT;
        attempts = fs->roots->total;
    }     

    for( k = 0; k < attempts; k++ )
    {
        int i, tab_size;
        CvFileNode* map_node = _map_node;
        CvFileMapNode* another;
        CvFileNodeHash* map;

        if( !map_node )
            map_node = (CvFileNode*)cvGetSeqElem( fs->roots, k );

        if( !CV_NODE_IS_MAP(map_node->tag) )
        {
            if( (!CV_NODE_IS_SEQ(map_node->tag) || map_node->data.seq->total != 0) &&
                CV_NODE_TYPE(map_node->tag) != CV_NODE_NONE )
                CV_ERROR( CV_StsError, "The node is neither a map nor an empty collection" );
            EXIT;
        }

        map = map_node->data.map;
        tab_size = map->tab_size;

        if( (tab_size & (tab_size - 1)) == 0 )
            i = (int)(key->hashval & (tab_size - 1));
        else
            i = (int)(key->hashval % tab_size);

        for( another = (CvFileMapNode*)(map->table[i]); another != 0; another = another->next )
            if( another->key == key )
            {
                if( !create_missing )
                {
                    value = &another->value;
                    EXIT;
                }
                CV_PARSE_ERROR( "Duplicated key" );
            }

        if( k == attempts - 1 && create_missing )
        {
            CvFileMapNode* node = (CvFileMapNode*)cvSetNew( (CvSet*)map );
            node->key = key;

            node->next = (CvFileMapNode*)(map->table[i]);
            map->table[i] = node;
            value = (CvFileNode*)node;
        }
    }

    __END__;

    return value;
}
// 
// 
CV_IMPL CvFileNode*
cvGetFileNodeByName( const CvFileStorage* fs, const CvFileNode* _map_node, const char* str )
{
    CvFileNode* value = 0;
    CV_FUNCNAME( "cvGetFileNodeByName" );

    __BEGIN__;

    int i, len, tab_size;
    unsigned hashval = 0;
    int k = 0, attempts = 1;

    if( !fs )
        EXIT;

    CV_CHECK_FILE_STORAGE(fs);

    if( !str )
        CV_ERROR( CV_StsNullPtr, "Null element name" );

    for( i = 0; str[i] != '\0'; i++ )
        hashval = hashval*CV_HASHVAL_SCALE + (unsigned char)str[i];
    hashval &= INT_MAX;
    len = i;

    if( !_map_node )
    {
        if( !fs->roots )
            EXIT;
        attempts = fs->roots->total;
    }     

    for( k = 0; k < attempts; k++ )
    {
        CvFileNodeHash* map;
        const CvFileNode* map_node = _map_node;
        CvFileMapNode* another;

        if( !map_node )
            map_node = (CvFileNode*)cvGetSeqElem( fs->roots, k );

        if( !CV_NODE_IS_MAP(map_node->tag) )
        {
            if( (!CV_NODE_IS_SEQ(map_node->tag) || map_node->data.seq->total != 0) &&
                CV_NODE_TYPE(map_node->tag) != CV_NODE_NONE )
                CV_ERROR( CV_StsError, "The node is neither a map nor an empty collection" );
            EXIT;
        }

        map = map_node->data.map;
        tab_size = map->tab_size;

        if( (tab_size & (tab_size - 1)) == 0 )
            i = (int)(hashval & (tab_size - 1));
        else
            i = (int)(hashval % tab_size);

        for( another = (CvFileMapNode*)(map->table[i]); another != 0; another = another->next )
        {
            const CvStringHashNode* key = another->key;

            if( key->hashval == hashval &&
                key->str.len == len &&
                memcmp( key->str.ptr, str, len ) == 0 )
            {
                value = &another->value;
                EXIT;
            }
        }
    }

    __END__;

    return value;
}
// 

// 
static char*
icvDoubleToString( char* buf, double value )
{
    Cv64suf val;
    unsigned ieee754_hi;

    val.f = value;
    ieee754_hi = (unsigned)(val.u >> 32);

    if( (ieee754_hi & 0x7ff00000) != 0x7ff00000 )
    {
        int ivalue = cvRound(value);
        if( ivalue == value )
            sprintf( buf, "%d.", ivalue );
        else
        {
            static const char* fmt[] = {"%.16e", "%.16f"};
            double avalue = fabs(value);
            char* ptr = buf;
            sprintf( buf, fmt[0.01 <= avalue && avalue < 1000], value );
            if( *ptr == '+' || *ptr == '-' )
                ptr++;
            for( ; isdigit(*ptr); ptr++ )
                ;
            if( *ptr == ',' )
                *ptr = '.';
        }
    }
    else
    {
        unsigned ieee754_lo = (unsigned)val.u;
        if( (ieee754_hi & 0x7fffffff) + (ieee754_lo != 0) > 0x7ff00000 )
            strcpy( buf, ".Nan" );
        else
            strcpy( buf, (int)ieee754_hi < 0 ? "-.Inf" : ".Inf" );
    }

    return buf;
}
// 
// 
// static char*
// icvFloatToString( char* buf, float value )
// {
//     Cv32suf val;
//     unsigned ieee754;
//     val.f = value;
//     ieee754 = val.u;
// 
//     if( (ieee754 & 0x7f800000) != 0x7f800000 )
//     {
//         int ivalue = cvRound(value);
//         if( ivalue == value )
//             sprintf( buf, "%d.", ivalue );
//         else
//         {
//             static const char* fmt[] = {"%.8e", "%.8f"};
//             double avalue = fabs((double)value);
//             char* ptr = buf;
//             sprintf( buf, fmt[0.01 <= avalue && avalue < 1000], value );
//             if( *ptr == '+' || *ptr == '-' )
//                 ptr++;
//             for( ; isdigit(*ptr); ptr++ )
//                 ;
//             if( *ptr == ',' )
//                 *ptr = '.';
//         }
//     }
//     else
//     {
//         if( (ieee754 & 0x7fffffff) != 0x7f800000 )
//             strcpy( buf, ".Nan" );
//         else
//             strcpy( buf, (int)ieee754 < 0 ? "-.Inf" : ".Inf" );
//     }
// 
//     return buf;
// }
// 
// 
static void
icvProcessSpecialDouble( CvFileStorage* fs, char* buf, double* value, char** endptr )
{
    CV_FUNCNAME( "icvProcessSpecialDouble" );

    __BEGIN__;
    
    char c = buf[0];
    int inf_hi = 0x7ff00000;
    
    if( c == '-' || c == '+' )
    {
        inf_hi = c == '-' ? 0xfff00000 : 0x7ff00000;
        c = *++buf;
    }
    
    if( c != '.' )
        CV_PARSE_ERROR( "Bad format of floating-point constant" );

    if( toupper(buf[1]) == 'I' && toupper(buf[2]) == 'N' && toupper(buf[3]) == 'F' )
        *(uint64*)value = ((uint64)inf_hi << 32);
    else if( toupper(buf[1]) == 'N' && toupper(buf[2]) == 'A' && toupper(buf[3]) == 'N' )
        *(uint64*)value = (uint64)-1;
    else
        CV_PARSE_ERROR( "Bad format of floating-point constant" );

    *endptr = buf + 4;

    __END__;
}
// 
// 
static double icv_strtod( CvFileStorage* fs, char* ptr, char** endptr )
{
    double fval = strtod( ptr, endptr );
    if( **endptr == '.' )
    {
        char* dot_pos = *endptr;
        *dot_pos = ',';
        double fval2 = strtod( ptr, endptr );
        *dot_pos = '.';
        if( *endptr > dot_pos )
            fval = fval2;
        else
            *endptr = dot_pos;
    }

    if( *endptr == ptr || isalpha(**endptr) )
        icvProcessSpecialDouble( fs, ptr, &fval, endptr );

    return fval;
}
// 
// 
// /****************************************************************************************\
// *                                       YAML Parser                                      *
// \****************************************************************************************/
// 
static char*
icvYMLSkipSpaces( CvFileStorage* fs, char* ptr, int min_indent, int max_comment_indent )
{
    CV_FUNCNAME( "icvYMLSkipSpaces" );

    __BEGIN__;
    
    for(;;)
    {
        while( *ptr == ' ' )
            ptr++;
        if( *ptr == '#' )
        {
            if( ptr - fs->buffer_start > max_comment_indent )
                EXIT;
            *ptr = '\0';
        }
        else if( cv_isprint(*ptr) )
        {
            if( ptr - fs->buffer_start < min_indent )
                CV_PARSE_ERROR( "Incorrect indentation" );
            break;
        }
        else if( *ptr == '\0' || *ptr == '\n' || *ptr == '\r' )
        {
            int max_size = (int)(fs->buffer_end - fs->buffer_start);
            ptr = fgets( fs->buffer_start, max_size, fs->file );
            if( !ptr )
            {
                // emulate end of stream
                ptr = fs->buffer_start;
                ptr[0] = ptr[1] = ptr[2] = '.';
                ptr[3] = '\0';
                fs->dummy_eof = 1;
                break;
            }
            else
            {
                int l = (int)strlen(ptr);
                if( ptr[l-1] != '\n' && ptr[l-1] != '\r' && !feof(fs->file) )
                    CV_PARSE_ERROR( "Too long string or a last string w/o newline" );
            }

            fs->lineno++;
        }
        else
            CV_PARSE_ERROR( *ptr == '\t' ? "Tabs are prohibited in YAML!" : "Invalid character" );    
    }

    __END__;

    return ptr;
}

// 
static char*
icvYMLParseKey( CvFileStorage* fs, char* ptr,
                CvFileNode* map_node, CvFileNode** value_placeholder )
{
    CV_FUNCNAME( "icvYMLParseKey" );
    
    __BEGIN__;

    char c;
    char *endptr = ptr - 1, *saveptr;
    CvStringHashNode* str_hash_node;

    if( *ptr == '-' )
        CV_PARSE_ERROR( "Key may not start with \'-\'" );

    do c = *++endptr;
    while( cv_isprint(c) && c != ':' );

    if( c != ':' )
        CV_PARSE_ERROR( "Missing \':\'" );

    saveptr = endptr + 1;
    do c = *--endptr;
    while( c == ' ' );

    ++endptr;
    if( endptr == ptr )
        CV_PARSE_ERROR( "An empty key" );
    
    CV_CALL( str_hash_node = cvGetHashedKey( fs, ptr, (int)(endptr - ptr), 1 ));
    CV_CALL( *value_placeholder = cvGetFileNode( fs, map_node, str_hash_node, 1 ));
    ptr = saveptr;

    __END__;

    return ptr;
}
// 
// 
static char*
icvYMLParseValue( CvFileStorage* fs, char* ptr, CvFileNode* node,
                  int parent_flags, int min_indent )
{
    CV_FUNCNAME( "icvYMLParseValue" );
    
    __BEGIN__;

    char buf[CV_FS_MAX_LEN + 1024];
    char* endptr = 0;
    char c = ptr[0], d = ptr[1];
    int is_parent_flow = CV_NODE_IS_FLOW(parent_flags);
    int value_type = CV_NODE_NONE;
    int len;

    memset( node, 0, sizeof(*node) );

    if( c == '!' ) // handle explicit type specification
    {
        if( d == '!' || d == '^' )
        {
            ptr++;
            value_type |= CV_NODE_USER;
        }

        endptr = ptr++;
        do d = *++endptr;
        while( cv_isprint(d) && d != ' ' );
        len = (int)(endptr - ptr);
        if( len == 0 )
            CV_PARSE_ERROR( "Empty type name" );
        d = *endptr;
        *endptr = '\0';
        
        if( len == 3 && !CV_NODE_IS_USER(value_type) )
        {
            if( memcmp( ptr, "str", 3 ) == 0 )
                value_type = CV_NODE_STRING;
            else if( memcmp( ptr, "int", 3 ) == 0 )
                value_type = CV_NODE_INT;
            else if( memcmp( ptr, "seq", 3 ) == 0 )
                value_type = CV_NODE_SEQ;
            else if( memcmp( ptr, "map", 3 ) == 0 )
                value_type = CV_NODE_MAP;
        }
        else if( len == 5 && !CV_NODE_IS_USER(value_type) )
        {
            if( memcmp( ptr, "float", 5 ) == 0 )
                value_type = CV_NODE_REAL;
        }
        else if( CV_NODE_IS_USER(value_type) )
        {
            CV_CALL( node->info = cvFindType( ptr ));
            if( !node->info )
                node->tag &= ~CV_NODE_USER;
        }

        *endptr = d;
        CV_CALL( ptr = icvYMLSkipSpaces( fs, endptr, min_indent, INT_MAX ));
        
        c = *ptr;

        if( !CV_NODE_IS_USER(value_type) )
        {
            if( value_type == CV_NODE_STRING && c != '\'' && c != '\"' )
                goto force_string;
            if( value_type == CV_NODE_INT )
                goto force_int;
            if( value_type == CV_NODE_REAL )
                goto force_real;
        }
    }

    if( isdigit(c) ||
        (c == '-' || c == '+') && (isdigit(d) || d == '.') ||
        c == '.' && isalnum(d)) // a number
    {
        double fval;
        int ival;
        endptr = ptr + (c == '-' || c == '+');
        while( isdigit(*endptr) )
            endptr++;
        if( *endptr == '.' || *endptr == 'e' )
        {
force_real:
            fval = icv_strtod( fs, ptr, &endptr );
            /*if( endptr == ptr || isalpha(*endptr) )
                CV_CALL( icvProcessSpecialDouble( fs, endptr, &fval, &endptr ));*/

            node->tag = CV_NODE_REAL;
            node->data.f = fval;
        }
        else
        {
force_int:
            ival = (int)strtol( ptr, &endptr, 0 );
            node->tag = CV_NODE_INT;
            node->data.i = ival;
        }

        if( !endptr || endptr == ptr )
            CV_PARSE_ERROR( "Invalid numeric value (inconsistent explicit type specification?)" );

        ptr = endptr;
    }
    else if( c == '\'' || c == '\"' ) // an explicit string
    {
        node->tag = CV_NODE_STRING;
        if( c == '\'' )
            for( len = 0; len < CV_FS_MAX_LEN; )
            {
                c = *++ptr;
                if( isalnum(c) || (c != '\'' && cv_isprint(c)))
                    buf[len++] = c;
                else if( c == '\'' )
                {
                    c = *++ptr;
                    if( c != '\'' )
                        break;
                    buf[len++] = c;
                }
                else
                    CV_PARSE_ERROR( "Invalid character" );
            }
        else
            for( len = 0; len < CV_FS_MAX_LEN; )
            {
                c = *++ptr;
                if( isalnum(c) || (c != '\\' && c != '\"' && cv_isprint(c)))
                    buf[len++] = c;
                else if( c == '\"' )
                {
                    ++ptr;
                    break;
                }
                else if( c == '\\' )
                {
                    d = *++ptr;
                    if( d == '\'' )
                        buf[len++] = d;
                    else if( d == '\"' || d == '\\' || d == '\'' )
                        buf[len++] = d;
                    else if( d == 'n' )
                        buf[len++] = '\n';
                    else if( d == 'r' )
                        buf[len++] = '\r';
                    else if( d == 't' )
                        buf[len++] = '\t';
                    else if( d == 'x' || isdigit(d) && d < '8' )
                    {
                        int val, is_hex = d == 'x';
                        c = ptr[3];
                        ptr[3] = '\0';
                        val = strtol( ptr + is_hex, &endptr, is_hex ? 8 : 16 );
                        ptr[3] = c;
                        if( endptr == ptr + is_hex )
                            buf[len++] = 'x';
                        else
                        {
                            buf[len++] = (char)val;
                            ptr = endptr;
                        }
                    }
                }
                else
                    CV_PARSE_ERROR( "Invalid character" );
            }

        if( len >= CV_FS_MAX_LEN )
            CV_PARSE_ERROR( "Too long string literal" );

        CV_CALL( node->data.str = cvMemStorageAllocString( fs->memstorage, buf, len ));
    }
    else if( c == '[' || c == '{' ) // collection as a flow
    {
        int new_min_indent = min_indent + !is_parent_flow;
        int struct_flags = CV_NODE_FLOW + (c == '{' ? CV_NODE_MAP : CV_NODE_SEQ);
        int is_simple = 1;
        
        CV_CALL( icvFSCreateCollection( fs, CV_NODE_TYPE(struct_flags) +
                                        (node->info ? CV_NODE_USER : 0), node ));

        d = c == '[' ? ']' : '}';

        for( ++ptr ;;)
        {
            CvFileNode* elem = 0;

            CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr, new_min_indent, INT_MAX ));
            if( *ptr == '}' || *ptr == ']' )
            {
                if( *ptr != d )
                    CV_PARSE_ERROR( "The wrong closing bracket" );
                ptr++;
                break;
            }

            if( node->data.seq->total != 0 )
            {
                if( *ptr != ',' )
                    CV_PARSE_ERROR( "Missing , between the elements" );
                CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr + 1, new_min_indent, INT_MAX ));
            }

            if( CV_NODE_IS_MAP(struct_flags) )
            {
                CV_CALL( ptr = icvYMLParseKey( fs, ptr, node, &elem ));
                CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr, new_min_indent, INT_MAX ));
            }
            else
            {
                if( *ptr == ']' )
                    break;
                elem = (CvFileNode*)cvSeqPush( node->data.seq, 0 );
            }
            CV_CALL( ptr = icvYMLParseValue( fs, ptr, elem, struct_flags, new_min_indent ));
            if( CV_NODE_IS_MAP(struct_flags) )
                elem->tag |= CV_NODE_NAMED;
            is_simple &= !CV_NODE_IS_COLLECTION(elem->tag);
        }
        node->data.seq->flags |= is_simple ? CV_NODE_SEQ_SIMPLE : 0;
    }
    else
    {
        int indent, struct_flags, is_simple;
        
        if( is_parent_flow || c != '-' )
        {
            // implicit (one-line) string or nested block-style collection
            if( !is_parent_flow )
            {
                if( c == '?' )
                    CV_PARSE_ERROR( "Complex keys are not supported" );
                if( c == '|' || c == '>' )
                    CV_PARSE_ERROR( "Multi-line text literals are not supported" );
            }

force_string:
            endptr = ptr - 1;

            do c = *++endptr;
            while( cv_isprint(c) &&
                   (!is_parent_flow || c != ',' && c != '}' && c != ']') &&
                   (is_parent_flow || c != ':' || value_type == CV_NODE_STRING));

            if( endptr == ptr )
                CV_PARSE_ERROR( "Invalid character" );

            if( is_parent_flow || c != ':' )
            {
                char* str_end = endptr;
                node->tag = CV_NODE_STRING;
                // strip spaces in the end of string
                do c = *--str_end;
                while( str_end > ptr && c == ' ' );
                str_end++;
                CV_CALL( node->data.str = cvMemStorageAllocString( fs->memstorage, ptr, (int)(str_end - ptr) ));
                ptr = endptr;
                EXIT;
            }
            struct_flags = CV_NODE_MAP;
        }
        else
            struct_flags = CV_NODE_SEQ;

        CV_CALL( icvFSCreateCollection( fs, struct_flags +
                    (node->info ? CV_NODE_USER : 0), node ));
        
        indent = (int)(ptr - fs->buffer_start);
        is_simple = 1;

        for(;;)
        {
            CvFileNode* elem = 0;
            
            if( CV_NODE_IS_MAP(struct_flags) )
            {
                CV_CALL( ptr = icvYMLParseKey( fs, ptr, node, &elem ));
            }
            else
            {
                c = *ptr++;
                if( c != '-' )
                    CV_PARSE_ERROR( "Block sequence elements must be preceded with \'-\'" );

                CV_CALL( elem = (CvFileNode*)cvSeqPush( node->data.seq, 0 ));
            }

            CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr, indent + 1, INT_MAX ));
            CV_CALL( ptr = icvYMLParseValue( fs, ptr, elem, struct_flags, indent + 1 ));
            if( CV_NODE_IS_MAP(struct_flags) )
                elem->tag |= CV_NODE_NAMED;
            is_simple &= !CV_NODE_IS_COLLECTION(elem->tag);

            CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr, 0, INT_MAX ));
            if( ptr - fs->buffer_start != indent )
            {
                if( ptr - fs->buffer_start < indent )
                    break;
                else
                    CV_PARSE_ERROR( "Incorrect indentation" );
            }
            if( memcmp( ptr, "...", 3 ) == 0 )
                break;
        }

        node->data.seq->flags |= is_simple ? CV_NODE_SEQ_SIMPLE : 0;
    }

    __END__;

    return ptr;
}
// 
// 
static void
icvYMLParse( CvFileStorage* fs )
{
    CV_FUNCNAME( "icvYMLParse" );

    __BEGIN__;

    char* ptr = fs->buffer_start;
    int is_first = 1;

    for(;;)
    {
        // 0. skip leading comments and directives  and ...
        // 1. reach the first item
        for(;;)
        {
            CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr, 0, INT_MAX ));
            if( !ptr )
                EXIT;

            if( *ptr == '%' )
            {
                if( memcmp( ptr, "%YAML:", 6 ) == 0 &&
                    memcmp( ptr, "%YAML:1.", 8 ) != 0 )
                    CV_PARSE_ERROR( "Unsupported YAML version (it must be 1.x)" );
                *ptr = '\0';
            }
            else if( *ptr == '-' )
            {
                if( memcmp(ptr, "---", 3) == 0 )
                {
                    ptr += 3;
                    break;
                }
                else if( is_first )
                    break;
            }
            else if( isalnum(*ptr) || *ptr=='_')
            {
                if( !is_first )
                    CV_PARSE_ERROR( "The YAML streams must start with '---', except the first one" );
                break;
            }
            else
                CV_PARSE_ERROR( "Invalid or unsupported syntax" );
        }

        CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr, 0, INT_MAX ));
        if( memcmp( ptr, "...", 3 ) != 0 )
        {
            // 2. parse the collection
            CvFileNode* root_node = (CvFileNode*)cvSeqPush( fs->roots, 0 );

            CV_CALL( ptr = icvYMLParseValue( fs, ptr, root_node, CV_NODE_NONE, 0 ));
            if( !CV_NODE_IS_COLLECTION(root_node->tag) )
                CV_PARSE_ERROR( "Only collections as YAML streams are supported by this parser" );
            
            // 3. parse until the end of file or next collection
            CV_CALL( ptr = icvYMLSkipSpaces( fs, ptr, 0, INT_MAX ));
            if( !ptr )
                EXIT;
        }

        if( fs->dummy_eof )
            break;
        ptr += 3;
        is_first = 0;
    }

    __END__;
}
// 
// 
// /****************************************************************************************\
// *                                       YAML Emitter                                     *
// \****************************************************************************************/
// 
static void
icvYMLWrite( CvFileStorage* fs, const char* key, const char* data, const char* cvFuncName )
{
    //CV_FUNCNAME( "icvYMLWrite" );

    __BEGIN__;
    
    int i, keylen = 0;
    int datalen = 0;
    int struct_flags;
    char* ptr;

    struct_flags = fs->struct_flags;

    if( key && key[0] == '\0' )
        key = 0;

    if( CV_NODE_IS_COLLECTION(struct_flags) )
    {
        if( (CV_NODE_IS_MAP(struct_flags) ^ (key != 0)) )
            CV_ERROR( CV_StsBadArg, "An attempt to add element without a key to a map, "
                                    "or add element with key to sequence" );
    }
    else
    {
        fs->is_first = 0;
        struct_flags = CV_NODE_EMPTY | (key ? CV_NODE_MAP : CV_NODE_SEQ);
    }

    if( key )
    {
        keylen = (int)strlen(key);
        if( keylen == 0 )
            CV_ERROR( CV_StsBadArg, "The key is an empty" );

        if( keylen > CV_FS_MAX_LEN )
            CV_ERROR( CV_StsBadArg, "The key is too long" );
    }

    if( data )
        datalen = (int)strlen(data);

    if( CV_NODE_IS_FLOW(struct_flags) )
    {
        int new_offset;
        ptr = fs->buffer;
        if( !CV_NODE_IS_EMPTY(struct_flags) )
            *ptr++ = ',';
        new_offset = (int)(ptr - fs->buffer_start) + keylen + datalen;
        if( new_offset > fs->wrap_margin && new_offset - fs->struct_indent > 10 )
        {
            fs->buffer = ptr;
            ptr = icvFSFlush(fs);
        }
        else
            *ptr++ = ' ';
    }
    else
    {
        ptr = icvFSFlush(fs);
        if( !CV_NODE_IS_MAP(struct_flags) )
        {
            *ptr++ = '-';
            if( data )
                *ptr++ = ' ';
        }
    }

    if( key )
    {
        if( !isalpha(key[0]) && key[0] != '_' )
            CV_ERROR( CV_StsBadArg, "Key must start with a letter or _" );
        
        ptr = icvFSResizeWriteBuffer( fs, ptr, keylen );

        for( i = 0; i < keylen; i++ )
        {
            int c = key[i];
        
            ptr[i] = (char)c;
            if( !isalnum(c) && c != '-' && c != '_' && c != ' ' )
                CV_ERROR( CV_StsBadArg, "Invalid character occurs in the key" );
        }

        ptr += keylen;
        *ptr++ = ':';
        if( !CV_NODE_IS_FLOW(struct_flags) && data )
            *ptr++ = ' ';
    }

    if( data )
    {
        ptr = icvFSResizeWriteBuffer( fs, ptr, datalen );
        memcpy( ptr, data, datalen );
        ptr += datalen;
    }

    fs->buffer = ptr;
    fs->struct_flags = struct_flags & ~CV_NODE_EMPTY;

    __END__;
}
// 
// 
static void
icvYMLStartWriteStruct( CvFileStorage* fs, const char* key, int struct_flags,
                        const char* type_name CV_DEFAULT(0))
{
    CV_FUNCNAME( "icvYMLStartWriteStruct" );

    __BEGIN__;

    int parent_flags;
    char buf[CV_FS_MAX_LEN + 1024];
    const char* data = 0;

    struct_flags = (struct_flags & (CV_NODE_TYPE_MASK|CV_NODE_FLOW)) | CV_NODE_EMPTY;
    if( !CV_NODE_IS_COLLECTION(struct_flags))
        CV_ERROR( CV_StsBadArg,
        "Some collection type - CV_NODE_SEQ or CV_NODE_MAP, must be specified" );

    if( CV_NODE_IS_FLOW(struct_flags) )
    {
        char c = CV_NODE_IS_MAP(struct_flags) ? '{' : '[';
        struct_flags |= CV_NODE_FLOW;

        if( type_name )
            sprintf( buf, "!!%s %c", type_name, c );
        else
        {
            buf[0] = c;
            buf[1] = '\0';
        }
        data = buf;
    }
    else if( type_name )
    {
        sprintf( buf, "!!%s", type_name );
        data = buf;
    }

    CV_CALL( icvYMLWrite( fs, key, data, cvFuncName ));

    parent_flags = fs->struct_flags;
    cvSeqPush( fs->write_stack, &parent_flags );
    fs->struct_flags = struct_flags;

    if( !CV_NODE_IS_FLOW(parent_flags) )
        fs->struct_indent += CV_YML_INDENT + CV_NODE_IS_FLOW(struct_flags);

    __END__;    
}


static void
icvYMLEndWriteStruct( CvFileStorage* fs )
{
    CV_FUNCNAME( "icvYMLEndWriteStruct" );

    __BEGIN__;

    int parent_flags = 0, struct_flags;
    char* ptr;

    struct_flags = fs->struct_flags;
    if( fs->write_stack->total == 0 )
        CV_ERROR( CV_StsError, "EndWriteStruct w/o matching StartWriteStruct" );

    cvSeqPop( fs->write_stack, &parent_flags );

    if( CV_NODE_IS_FLOW(struct_flags) )
    {
        ptr = fs->buffer;
        if( ptr > fs->buffer_start + fs->struct_indent && !CV_NODE_IS_EMPTY(struct_flags) )
            *ptr++ = ' ';
        *ptr++ = CV_NODE_IS_MAP(struct_flags) ? '}' : ']';
        fs->buffer = ptr;
    }
    else if( CV_NODE_IS_EMPTY(struct_flags) )
    {
        ptr = icvFSFlush(fs);
        memcpy( ptr, CV_NODE_IS_MAP(struct_flags) ? "{}" : "[]", 2 );
        fs->buffer = ptr + 2;
    }

    if( !CV_NODE_IS_FLOW(parent_flags) )
        fs->struct_indent -= CV_YML_INDENT + CV_NODE_IS_FLOW(struct_flags);
    assert( fs->struct_indent >= 0 );

    fs->struct_flags = parent_flags;

    __END__;
}
// 
// 
static void
icvYMLStartNextStream( CvFileStorage* fs )
{
    //CV_FUNCNAME( "icvYMLStartNextStream" );

    __BEGIN__;

    if( !fs->is_first )
    {
        while( fs->write_stack->total > 0 )
            icvYMLEndWriteStruct(fs);

        fs->struct_indent = 0;
        icvFSFlush(fs);
        fputs( "...\n", fs->file );
        fputs( "---\n", fs->file );
        fs->buffer = fs->buffer_start;
    }

    __END__;
}
// 
// 
static void
icvYMLWriteInt( CvFileStorage* fs, const char* key, int value )
{
    CV_FUNCNAME( "icvYMLWriteInt" );

    __BEGIN__;
    
    char buf[128];
    CV_CALL( icvYMLWrite( fs, key, icv_itoa( value, buf, 10 ), cvFuncName ));
    
    __END__;    
}
// 
// 
static void
icvYMLWriteReal( CvFileStorage* fs, const char* key, double value )
{
    CV_FUNCNAME( "icvYMLWriteReal" );

    __BEGIN__;
    
    char buf[128];
    CV_CALL( icvYMLWrite( fs, key, icvDoubleToString( buf, value ), cvFuncName ));
    
    __END__;    
}
// 
// 
static void
icvYMLWriteString( CvFileStorage* fs, const char* key,
                   const char* str, int quote CV_DEFAULT(0))
{
    CV_FUNCNAME( "icvYMLWriteString" );

    __BEGIN__;

    char buf[CV_FS_MAX_LEN*4+16];
    char* data = (char*)str;
    int i, len;

    if( !str )
        CV_ERROR( CV_StsNullPtr, "Null string pointer" );

    len = (int)strlen(str);
    if( len > CV_FS_MAX_LEN )
        CV_ERROR( CV_StsBadArg, "The written string is too long" );

    if( quote || len == 0 || str[0] != str[len-1] || str[0] != '\"' && str[0] != '\'' )
    {
        int need_quote = quote || len == 0;
        data = buf;
        *data++ = '\"';
        for( i = 0; i < len; i++ )
        {
            char c = str[i];
            
            if( !need_quote && !isalnum(c) && c != '_' && c != ' ' && c != '-' &&
                c != '(' && c != ')' && c != '/' && c != '+' && c != ';' )
                need_quote = 1;

            if( !isalnum(c) && (!cv_isprint(c) || c == '\\' || c == '\'' || c == '\"') )
            {
                *data++ = '\\';
                if( cv_isprint(c) )
                    *data++ = c;
                else if( c == '\n' )
                    *data++ = 'n';
                else if( c == '\r' )
                    *data++ = 'r';
                else if( c == '\t' )
                    *data++ = 't';
                else
                {
                    sprintf( data, "x%02x", c );
                    data += 3;
                }
            }
            else
                *data++ = c;
        }
        if( !need_quote && (isdigit(str[0]) ||
            str[0] == '+' || str[0] == '-' || str[0] == '.' ))
            need_quote = 1;

        if( need_quote )
            *data++ = '\"';
        *data++ = '\0';
        data = buf + !need_quote;
    }

    CV_CALL( icvYMLWrite( fs, key, data, cvFuncName ));

    __END__;
}

// 
static void
icvYMLWriteComment( CvFileStorage* fs, const char* comment, int eol_comment )
{
    CV_FUNCNAME( "icvYMLWriteComment" );

    __BEGIN__;

    int len; //, indent;
    int multiline;
    const char* eol;
    char* ptr;

    if( !comment )
        CV_ERROR( CV_StsNullPtr, "Null comment" );

    len = (int)strlen(comment);
    eol = strchr(comment, '\n');
    multiline = eol != 0;
    ptr = fs->buffer;

    if( !eol_comment || multiline ||
        fs->buffer_end - ptr < len || ptr == fs->buffer_start )
        ptr = icvFSFlush( fs );
    else
        *ptr++ = ' ';

    while( comment )
    {
        *ptr++ = '#';
        *ptr++ = ' ';
        if( eol )
        {
            ptr = icvFSResizeWriteBuffer( fs, ptr, (int)(eol - comment) + 1 );
            memcpy( ptr, comment, eol - comment + 1 );
            fs->buffer = ptr + (eol - comment);
            comment = eol + 1;
            eol = strchr( comment, '\n' );
        }
        else
        {
            len = (int)strlen(comment);
            ptr = icvFSResizeWriteBuffer( fs, ptr, len );
            memcpy( ptr, comment, len );
            fs->buffer = ptr + len;
            comment = 0;
        }
        ptr = icvFSFlush( fs );
    }
    
    __END__;
}
// 
// 
// /****************************************************************************************\
// *                                       XML Parser                                       *
// \****************************************************************************************/
// 
#define CV_XML_INSIDE_COMMENT 1
#define CV_XML_INSIDE_TAG 2
#define CV_XML_INSIDE_DIRECTIVE 3

static char*
icvXMLSkipSpaces( CvFileStorage* fs, char* ptr, int mode )
{
    CV_FUNCNAME( "icvXMLSkipSpaces" );

    __BEGIN__;

    int level = 0;

    for(;;)
    {
        char c;
        ptr--;
        
        if( mode == CV_XML_INSIDE_COMMENT )
        {
            do c = *++ptr;
            while( cv_isprint_or_tab(c) && (c != '-' || ptr[1] != '-' || ptr[2] != '>') );
            
            if( c == '-' )
            {
                assert( ptr[1] == '-' && ptr[2] == '>' );
                mode = 0;
                ptr += 3;
            }
        }
        else if( mode == CV_XML_INSIDE_DIRECTIVE )
        {
            // !!!NOTE!!! This is not quite correct, but should work in most cases
            do
            {
                c = *++ptr;
                level += c == '<';
                level -= c == '>';
                if( level < 0 )
                    EXIT;
            } while( cv_isprint_or_tab(c) );
        }
        else
        {
            do c = *++ptr;
            while( c == ' ' || c == '\t' );

            if( c == '<' && ptr[1] == '!' && ptr[2] == '-' && ptr[3] == '-' )
            {
                if( mode != 0 )
                    CV_PARSE_ERROR( "Comments are not allowed here" );
                mode = CV_XML_INSIDE_COMMENT;
                ptr += 4;
            }
            else if( cv_isprint(c) )
                break;
        }
        
        if( !cv_isprint(*ptr) )
        {
            int max_size = (int)(fs->buffer_end - fs->buffer_start);
            if( *ptr != '\0' && *ptr != '\n' && *ptr != '\r' )
                CV_PARSE_ERROR( "Invalid character in the stream" );
            ptr = fgets( fs->buffer_start, max_size, fs->file );
            if( !ptr )
            {
                ptr = fs->buffer_start;
                *ptr = '\0';
                fs->dummy_eof = 1;
                break;
            }
            else
            {
                int l = (int)strlen(ptr);
                if( ptr[l-1] != '\n' && ptr[l-1] != '\r' && !feof(fs->file) )
                    CV_PARSE_ERROR( "Too long string or a last string w/o newline" );
            }
            fs->lineno++;
        }
    }

    __END__;

    return ptr;
}
// 
// 
static char*
icvXMLParseTag( CvFileStorage* fs, char* ptr, CvStringHashNode** _tag,
                CvAttrList** _list, int* _tag_type );

static char*
icvXMLParseValue( CvFileStorage* fs, char* ptr, CvFileNode* node,
                  int value_type CV_DEFAULT(CV_NODE_NONE))
{
    CV_FUNCNAME( "icvXMLParseValue" );
    
    __BEGIN__;

    CvFileNode *elem = node;
    int have_space = 1, is_simple = 1;
    int is_user_type = CV_NODE_IS_USER(value_type);
    memset( node, 0, sizeof(*node) );

    value_type = CV_NODE_TYPE(value_type);
    
    for(;;)
    {
        char c = *ptr, d;
        char* endptr;

        if( isspace(c) || c == '\0' || c == '<' && ptr[1] == '!' && ptr[2] == '-' )
        {
            CV_CALL( ptr = icvXMLSkipSpaces( fs, ptr, 0 ));
            have_space = 1;
            c = *ptr;
        }

        d = ptr[1];

        if( c =='<' )
        {
            CvStringHashNode *key = 0, *key2 = 0;
            CvAttrList* list = 0;
            CvTypeInfo* info = 0;
            int tag_type = 0;
            int is_noname = 0;
            const char* type_name = 0;
            int elem_type = CV_NODE_NONE;

            if( d == '/' )
                break;

            CV_CALL( ptr = icvXMLParseTag( fs, ptr, &key, &list, &tag_type ));

            if( tag_type == CV_XML_DIRECTIVE_TAG )
                CV_PARSE_ERROR( "Directive tags are not allowed here" );
            if( tag_type == CV_XML_EMPTY_TAG )
                CV_PARSE_ERROR( "Empty tags are not supported" );

            assert( tag_type == CV_XML_OPENING_TAG );

            type_name = list ? cvAttrValue( list, "type_id" ) : 0;
            if( type_name )
            {
                if( strcmp( type_name, "str" ) == 0 )
                    elem_type = CV_NODE_STRING;
                else if( strcmp( type_name, "map" ) == 0 )
                    elem_type = CV_NODE_MAP;
                else if( strcmp( type_name, "seq" ) == 0 )
                    elem_type = CV_NODE_MAP;
                else
                {
                    CV_CALL( info = cvFindType( type_name ));
                    if( info )
                        elem_type = CV_NODE_USER;
                }
            }

            is_noname = key->str.len == 1 && key->str.ptr[0] == '_';
            if( !CV_NODE_IS_COLLECTION(node->tag) )
            {
                CV_CALL( icvFSCreateCollection( fs, is_noname ? CV_NODE_SEQ : CV_NODE_MAP, node ));
            }
            else if( is_noname ^ CV_NODE_IS_SEQ(node->tag) )
                CV_PARSE_ERROR( is_noname ? "Map element should have a name" :
                              "Sequence element should not have name (use <_></_>)" ); 

            if( is_noname )
                elem = (CvFileNode*)cvSeqPush( node->data.seq, 0 );
            else
                CV_CALL( elem = cvGetFileNode( fs, node, key, 1 ));
                
            CV_CALL( ptr = icvXMLParseValue( fs, ptr, elem, elem_type));
            if( !is_noname )
                elem->tag |= CV_NODE_NAMED;
            is_simple &= !CV_NODE_IS_COLLECTION(elem->tag);
            elem->info = info;
            CV_CALL( ptr = icvXMLParseTag( fs, ptr, &key2, &list, &tag_type ));
            if( tag_type != CV_XML_CLOSING_TAG || key2 != key )
                CV_PARSE_ERROR( "Mismatched closing tag" );
            have_space = 1;
        }
        else
        {
            if( !have_space )
                CV_PARSE_ERROR( "There should be space between literals" );
            
            elem = node;
            if( node->tag != CV_NODE_NONE )
            {
                if( !CV_NODE_IS_COLLECTION(node->tag) )
                    CV_CALL( icvFSCreateCollection( fs, CV_NODE_SEQ, node ));

                elem = (CvFileNode*)cvSeqPush( node->data.seq, 0 );
                elem->info = 0;
            }

            if( value_type != CV_NODE_STRING &&
                (isdigit(c) || (c == '-' || c == '+') &&
                (isdigit(d) || d == '.') || c == '.' && isalnum(d)) ) // a number
            {
                double fval;
                int ival;
                endptr = ptr + (c == '-' || c == '+');
                while( isdigit(*endptr) )
                    endptr++;
                if( *endptr == '.' || *endptr == 'e' )
                {
                    fval = icv_strtod( fs, ptr, &endptr );
                    /*if( endptr == ptr || isalpha(*endptr) )
                        CV_CALL( icvProcessSpecialDouble( fs, ptr, &fval, &endptr ));*/
                    elem->tag = CV_NODE_REAL;
                    elem->data.f = fval;
                }
                else
                {
                    ival = (int)strtol( ptr, &endptr, 0 );
                    elem->tag = CV_NODE_INT;
                    elem->data.i = ival;
                }

                if( endptr == ptr )
                    CV_PARSE_ERROR( "Invalid numeric value (inconsistent explicit type specification?)" );

                ptr = endptr;
            }
            else
            {
                // string
                char buf[CV_FS_MAX_LEN+16];
                int i = 0, len, is_quoted = 0;
                elem->tag = CV_NODE_STRING;
                if( c == '\"' )
                    is_quoted = 1;
                else
                    --ptr;

                for( ;; )
                {
                    c = *++ptr;
                    if( !isalnum(c) )
                    {
                        if( c == '\"' )
                        {
                            if( !is_quoted )
                                CV_PARSE_ERROR( "Literal \" is not allowed within a string. Use &quot;" );
                            ++ptr;
                            break;
                        }
                        else if( !cv_isprint(c) || c == '<' || (!is_quoted && isspace(c)))
                        {
                            if( is_quoted )
                                CV_PARSE_ERROR( "Closing \" is expected" );
                            break;
                        }
                        else if( c == '\'' || c == '>' )
                        {
                            CV_PARSE_ERROR( "Literal \' or > are not allowed. Use &apos; or &gt;" );
                        }
                        else if( c == '&' )
                        {
                            if( *ptr == '#' )
                            {
                                int val;
                                ptr++;
                                val = (int)strtol( ptr, &endptr, 0 );
                                if( (unsigned)val > (unsigned)255 ||
                                    !endptr || *endptr != ';' )
                                    CV_PARSE_ERROR( "Invalid numeric value in the string" );
                                c = (char)val;
                            }
                            else
                            {
                                endptr = ptr++;
                                do c = *++endptr;
                                while( isalnum(c) );
                                if( c != ';' )
                                    CV_PARSE_ERROR( "Invalid character in the symbol entity name" );
                                len = (int)(endptr - ptr);
                                if( len == 2 && memcmp( ptr, "lt", len ) == 0 )
                                    c = '<';
                                else if( len == 2 && memcmp( ptr, "gt", len ) == 0 )
                                    c = '>';
                                else if( len == 3 && memcmp( ptr, "amp", len ) == 0 )
                                    c = '&';
                                else if( len == 4 && memcmp( ptr, "apos", len ) == 0 )
                                    c = '\'';
                                else if( len == 4 && memcmp( ptr, "quot", len ) == 0 )
                                    c = '\"';
                                else
                                {
                                    memcpy( buf + i, ptr-1, len + 2 );
                                    i += len + 2;
                                }
                            }
                            ptr = endptr;
                        }
                    }
                    buf[i++] = c;
                    if( i >= CV_FS_MAX_LEN )
                        CV_PARSE_ERROR( "Too long string literal" );
                }
                CV_CALL( elem->data.str = cvMemStorageAllocString( fs->memstorage, buf, i ));
            }

            if( !CV_NODE_IS_COLLECTION(value_type) && value_type != CV_NODE_NONE )
                break;
            have_space = 0;
        }
    }

    if( (CV_NODE_TYPE(node->tag) == CV_NODE_NONE ||
        CV_NODE_TYPE(node->tag) != value_type &&
        !CV_NODE_IS_COLLECTION(node->tag)) &&
        CV_NODE_IS_COLLECTION(value_type) )
    {
        CV_CALL( icvFSCreateCollection( fs, CV_NODE_IS_MAP(value_type) ?
                                        CV_NODE_MAP : CV_NODE_SEQ, node ));
    }

    if( value_type != CV_NODE_NONE &&
        value_type != CV_NODE_TYPE(node->tag) )
        CV_PARSE_ERROR( "The actual type is different from the specified type" );

    if( CV_NODE_IS_COLLECTION(node->tag) && is_simple )
            node->data.seq->flags |= CV_NODE_SEQ_SIMPLE;

    node->tag |= is_user_type ? CV_NODE_USER : 0;

    __END__;

    return ptr;
}

// 
static char*
icvXMLParseTag( CvFileStorage* fs, char* ptr, CvStringHashNode** _tag,
                CvAttrList** _list, int* _tag_type )
{
    int tag_type = 0;
    CvStringHashNode* tagname = 0;
    CvAttrList *first = 0, *last = 0;
    int count = 0, max_count = 4;
    int attr_buf_size = (max_count*2 + 1)*sizeof(char*) + sizeof(CvAttrList);
    
    CV_FUNCNAME( "icvXMLParseTag" );

    __BEGIN__;

    char* endptr;
    char c;
    int have_space;

    if( *ptr != '<' )
        CV_PARSE_ERROR( "Tag should start with \'<\'" );

    ptr++;
    if( isalnum(*ptr) || *ptr == '_' )
        tag_type = CV_XML_OPENING_TAG;
    else if( *ptr == '/' )
    {
        tag_type = CV_XML_CLOSING_TAG;
        ptr++;
    }
    else if( *ptr == '?' )
    {
        tag_type = CV_XML_HEADER_TAG;
        ptr++;
    }
    else if( *ptr == '!' )
    {
        tag_type = CV_XML_DIRECTIVE_TAG;
        assert( ptr[1] != '-' || ptr[2] != '-' );
        ptr++;
    }
    else
        CV_PARSE_ERROR( "Unknown tag type" );

    for(;;)
    {
        CvStringHashNode* attrname;
        
        if( !isalpha(*ptr) && *ptr != '_' )
            CV_PARSE_ERROR( "Name should start with a letter or underscore" );

        endptr = ptr - 1;
        do c = *++endptr;
        while( isalnum(c) || c == '_' || c == '-' );

        CV_CALL( attrname = cvGetHashedKey( fs, ptr, (int)(endptr - ptr), 1 ));
        ptr = endptr;

        if( !tagname )
            tagname = attrname;
        else
        {
            if( tag_type == CV_XML_CLOSING_TAG )
                CV_PARSE_ERROR( "Closing tag should not contain any attributes" );

            if( !last || count >= max_count )
            {
                CvAttrList* chunk;
                
                CV_CALL( chunk = (CvAttrList*)cvMemStorageAlloc( fs->memstorage, attr_buf_size ));
                memset( chunk, 0, attr_buf_size );
                chunk->attr = (const char**)(chunk + 1);
                count = 0;
                if( !last )
                    first = last = chunk;
                else
                    last = last->next = chunk;
            }
            last->attr[count*2] = attrname->str.ptr;
        }

        if( last )
        {
            CvFileNode stub;
            
            if( *ptr != '=' )
            {
                CV_CALL( ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG ));
                if( *ptr != '=' )
                    CV_PARSE_ERROR( "Attribute name should be followed by \'=\'" );
            }

            c = *++ptr;
            if( c != '\"' && c != '\'' )
            {
                CV_CALL( ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG ));
                if( *ptr != '\"' && *ptr != '\'' )
                    CV_PARSE_ERROR( "Attribute value should be put into single or double quotes" );
            }

            ptr = icvXMLParseValue( fs, ptr, &stub, CV_NODE_STRING );
            assert( stub.tag == CV_NODE_STRING );
            last->attr[count*2+1] = stub.data.str.ptr;
            count++;
        }
        
        c = *ptr;
        have_space = isspace(c) || c == '\0';

        if( c != '>' )
        {
            CV_CALL( ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG ));
            c = *ptr;
        }

        if( c == '>' )
        {
            if( tag_type == CV_XML_HEADER_TAG )
                CV_PARSE_ERROR( "Invalid closing tag for <?xml ..." );
            ptr++;
            break;
        }
        else if( c == '?' && tag_type == CV_XML_HEADER_TAG )
        {
            if( ptr[1] != '>'  )
                CV_PARSE_ERROR( "Invalid closing tag for <?xml ..." );
            ptr += 2;
            break;
        }
        else if( c == '/' && ptr[1] == '>' && tag_type == CV_XML_OPENING_TAG )
        {
            tag_type = CV_XML_EMPTY_TAG;
            ptr += 2;
            break;
        }

        if( !have_space )
            CV_PARSE_ERROR( "There should be space between attributes" );
    }

    __END__;

    *_tag = tagname;
    *_tag_type = tag_type;
    *_list = first;

    return ptr;
}
// 
// 
static void
icvXMLParse( CvFileStorage* fs )
{
    CV_FUNCNAME( "icvXMLParse" );

    __BEGIN__;

    char* ptr = fs->buffer_start;
    CvStringHashNode *key = 0, *key2 = 0;
    CvAttrList* list = 0;
    int tag_type = 0;

    // CV_XML_INSIDE_TAG is used to prohibit leading comments
    CV_CALL( ptr = icvXMLSkipSpaces( fs, ptr, CV_XML_INSIDE_TAG ));

    if( memcmp( ptr, "<?xml", 5 ) != 0 )
        CV_PARSE_ERROR( "Valid XML should start with \'<?xml ...?>\'" );

    CV_CALL( ptr = icvXMLParseTag( fs, ptr, &key, &list, &tag_type ));

    /*{
        const char* version = cvAttrValue( list, "version" );
        if( version && strncmp( version, "1.", 2 ) != 0 )
            CV_ERROR( CV_StsParseError, "Unsupported version of XML" );
    }*/
    {
        const char* encoding = cvAttrValue( list, "encoding" );
        if( encoding && strcmp( encoding, "ASCII" ) != 0 )
            CV_PARSE_ERROR( "Unsupported encoding" );
    }

    while( *ptr != '\0' )
    {
        CV_CALL( ptr = icvXMLSkipSpaces( fs, ptr, 0 ));

        if( *ptr != '\0' )
        {
            CvFileNode* root_node;
            CV_CALL( ptr = icvXMLParseTag( fs, ptr, &key, &list, &tag_type ));
            if( tag_type != CV_XML_OPENING_TAG ||
                strcmp(key->str.ptr,"opencv_storage") != 0 )
                CV_PARSE_ERROR( "<opencv_storage> tag is missing" );

            root_node = (CvFileNode*)cvSeqPush( fs->roots, 0 );
            CV_CALL( ptr = icvXMLParseValue( fs, ptr, root_node, CV_NODE_NONE ));
            CV_CALL( ptr = icvXMLParseTag( fs, ptr, &key2, &list, &tag_type ));
            if( tag_type != CV_XML_CLOSING_TAG || key != key2 )
                CV_PARSE_ERROR( "</opencv_storage> tag is missing" );
            CV_CALL( ptr = icvXMLSkipSpaces( fs, ptr, 0 ));
        }
    }

    assert( fs->dummy_eof != 0 );

    __END__;
}
// 
// 
// /****************************************************************************************\
// *                                       XML Emitter                                      *
// \****************************************************************************************/
// 
#define icvXMLFlush icvFSFlush
// 
static void
icvXMLWriteTag( CvFileStorage* fs, const char* key, int tag_type, CvAttrList list )
{
    CV_FUNCNAME( "icvXMLWriteTag" );

    __BEGIN__;

    char* ptr = fs->buffer;
    int i, len = 0;
    int struct_flags = fs->struct_flags;

    if( key && key[0] == '\0' )
        key = 0;

    if( tag_type == CV_XML_OPENING_TAG || tag_type == CV_XML_EMPTY_TAG )
    {
        if( CV_NODE_IS_COLLECTION(struct_flags) )
        {
            if( CV_NODE_IS_MAP(struct_flags) ^ (key != 0) )
                CV_ERROR( CV_StsBadArg, "An attempt to add element without a key to a map, "
                                        "or add element with key to sequence" );
        }
        else
        {
            struct_flags = CV_NODE_EMPTY + (key ? CV_NODE_MAP : CV_NODE_SEQ);
            fs->is_first = 0;
        }

        if( !CV_NODE_IS_EMPTY(struct_flags) )
            ptr = icvXMLFlush(fs);
    }

    if( !key ) 
        key = "_";
    else if( key[0] == '_' && key[1] == '\0' )
        CV_ERROR( CV_StsBadArg, "A single _ is a reserved tag name" );

    len = (int)strlen( key );
    *ptr++ = '<';
    if( tag_type == CV_XML_CLOSING_TAG )
    {
        if( list.attr )
            CV_ERROR( CV_StsBadArg, "Closing tag should not include any attributes" );
        *ptr++ = '/';
    }
    
    if( !isalpha(key[0]) && key[0] != '_' )
        CV_ERROR( CV_StsBadArg, "Key should start with a letter or _" );
    
    ptr = icvFSResizeWriteBuffer( fs, ptr, len );
    for( i = 0; i < len; i++ )
    {
        char c = key[i];
        if( !isalnum(c) && c != '_' && c != '-' )
            CV_ERROR( CV_StsBadArg, "Invalid character in the key" );
        ptr[i] = c;
    }
    ptr += len;

    for(;;)
    {
        const char** attr = list.attr;
        
        for( ; attr && attr[0] != 0; attr += 2 )
        {
            int len0 = (int)strlen(attr[0]);
            int len1 = (int)strlen(attr[1]);

            ptr = icvFSResizeWriteBuffer( fs, ptr, len0 + len1 + 4 );
            *ptr++ = ' ';
            memcpy( ptr, attr[0], len0 );
            ptr += len0;
            *ptr++ = '=';
            *ptr++ = '\"';
            memcpy( ptr, attr[1], len1 );
            ptr += len1;
            *ptr++ = '\"';
        }
        if( !list.next )
            break;
        list = *list.next;
    }

    if( tag_type == CV_XML_EMPTY_TAG )
        *ptr++ = '/';
    *ptr++ = '>';
    fs->buffer = ptr;
    fs->struct_flags = struct_flags & ~CV_NODE_EMPTY;

    __END__;
}

// 
static void
icvXMLStartWriteStruct( CvFileStorage* fs, const char* key, int struct_flags,
                        const char* type_name CV_DEFAULT(0))
{
    CV_FUNCNAME( "icvXMLStartWriteStruct" );

    __BEGIN__;

    CvXMLStackRecord parent;
    const char* attr[10];
    int idx = 0;

    struct_flags = (struct_flags & (CV_NODE_TYPE_MASK|CV_NODE_FLOW)) | CV_NODE_EMPTY;
    if( !CV_NODE_IS_COLLECTION(struct_flags))
        CV_ERROR( CV_StsBadArg,
        "Some collection type: CV_NODE_SEQ or CV_NODE_MAP must be specified" );

    if( type_name )
    {
        attr[idx++] = "type_id";
        attr[idx++] = type_name;
    }
    attr[idx++] = 0;

    CV_CALL( icvXMLWriteTag( fs, key, CV_XML_OPENING_TAG, cvAttrList(attr,0) ));

    parent.struct_flags = fs->struct_flags & ~CV_NODE_EMPTY;
    parent.struct_indent = fs->struct_indent;
    parent.struct_tag = fs->struct_tag;
    cvSaveMemStoragePos( fs->strstorage, &parent.pos );
    cvSeqPush( fs->write_stack, &parent );

    fs->struct_indent += CV_XML_INDENT;
    if( !CV_NODE_IS_FLOW(struct_flags) )
        icvXMLFlush( fs );

    fs->struct_flags = struct_flags;
    if( key )
    {
        CV_CALL( fs->struct_tag = cvMemStorageAllocString( fs->strstorage, (char*)key, -1 ));
    }
    else
    {
        fs->struct_tag.ptr = 0;
        fs->struct_tag.len = 0;
    }

    __END__;    
}
// 
// 
static void
icvXMLEndWriteStruct( CvFileStorage* fs )
{
    CV_FUNCNAME( "icvXMLStartWriteStruct" );

    __BEGIN__;

    CvXMLStackRecord parent;

    if( fs->write_stack->total == 0 )
        CV_ERROR( CV_StsError, "An extra closing tag" );

    CV_CALL( icvXMLWriteTag( fs, fs->struct_tag.ptr, CV_XML_CLOSING_TAG, cvAttrList(0,0) ));
    cvSeqPop( fs->write_stack, &parent );

    fs->struct_indent = parent.struct_indent;
    fs->struct_flags = parent.struct_flags;
    fs->struct_tag = parent.struct_tag;
    cvRestoreMemStoragePos( fs->strstorage, &parent.pos );

    __END__;
}

// 
static void
icvXMLStartNextStream( CvFileStorage* fs )
{
    //CV_FUNCNAME( "icvXMLStartNextStream" );

    __BEGIN__;

    if( !fs->is_first )
    {
        while( fs->write_stack->total > 0 )
            icvXMLEndWriteStruct(fs);

        fs->struct_indent = 0;
        icvXMLFlush(fs);
        /* XML does not allow multiple top-level elements,
           so we just put a comment and continue
           the current (and the only) "stream" */
        fputs( "\n<!-- next stream -->\n", fs->file );
        /*fputs( "</opencv_storage>\n", fs->file );
        fputs( "<opencv_storage>\n", fs->file );*/
        fs->buffer = fs->buffer_start;
    }

    __END__;
}


static void
icvXMLWriteScalar( CvFileStorage* fs, const char* key, const char* data, int len )
{
    CV_FUNCNAME( "icvXMLWriteScalar" );

    __BEGIN__;

    if( CV_NODE_IS_MAP(fs->struct_flags) ||
        (!CV_NODE_IS_COLLECTION(fs->struct_flags) && key) )
    {
        icvXMLWriteTag( fs, key, CV_XML_OPENING_TAG, cvAttrList(0,0) );
        char* ptr = icvFSResizeWriteBuffer( fs, fs->buffer, len );
        memcpy( ptr, data, len );
        fs->buffer = ptr + len;
        icvXMLWriteTag( fs, key, CV_XML_CLOSING_TAG, cvAttrList(0,0) );
    }
    else
    {
        char* ptr = fs->buffer;
        int new_offset = (int)(ptr - fs->buffer_start) + len;

        if( key )
            CV_ERROR( CV_StsBadArg, "elements with keys can not be written to sequence" );

        fs->struct_flags = CV_NODE_SEQ;

        if( (new_offset > fs->wrap_margin && new_offset - fs->struct_indent > 10) ||
            (ptr > fs->buffer_start && ptr[-1] == '>' && !CV_NODE_IS_EMPTY(fs->struct_flags)) )
        {
            ptr = icvXMLFlush(fs);
        }
        else if( ptr > fs->buffer_start + fs->struct_indent && ptr[-1] != '>' )
            *ptr++ = ' ';
    
        memcpy( ptr, data, len );
        fs->buffer = ptr + len;
    }

    __END__;
}
// 
// 
static void
icvXMLWriteInt( CvFileStorage* fs, const char* key, int value )
{
    //CV_FUNCNAME( "cvXMLWriteInt" );

    __BEGIN__;
    
    char buf[128], *ptr = icv_itoa( value, buf, 10 );
    int len = (int)strlen(ptr);
    icvXMLWriteScalar( fs, key, ptr, len );
    
    __END__;    
}
// 
// 
static void
icvXMLWriteReal( CvFileStorage* fs, const char* key, double value )
{
    //CV_FUNCNAME( "cvXMLWriteReal" );

    __BEGIN__;
    
    char buf[128];
    int len = (int)strlen( icvDoubleToString( buf, value ));
    icvXMLWriteScalar( fs, key, buf, len );
    
    __END__;    
}

// 
static void
icvXMLWriteString( CvFileStorage* fs, const char* key, const char* str, int quote )
{
    CV_FUNCNAME( "cvXMLWriteString" );

    __BEGIN__;

    char buf[CV_FS_MAX_LEN*6+16];
    char* data = (char*)str;
    int i, len;

    if( !str )
        CV_ERROR( CV_StsNullPtr, "Null string pointer" );

    len = (int)strlen(str);
    if( len > CV_FS_MAX_LEN )
        CV_ERROR( CV_StsBadArg, "The written string is too long" );

    if( quote || len == 0 || str[0] != '\"' || str[0] != str[len-1] )
    {
        int need_quote = quote || len == 0;
        data = buf;
        *data++ = '\"';
        for( i = 0; i < len; i++ )
        {
            char c = str[i];
            
            if( !isalnum(c) && (!cv_isprint(c) || c == '<' || c == '>' ||
                c == '&' || c == '\'' || c == '\"') )
            {
                *data++ = '&';
                if( c == '<' )
                {
                    memcpy(data, "lt", 2);
                    data += 2;
                }
                else if( c == '>' )
                {
                    memcpy(data, "gt", 2);
                    data += 2;
                }
                else if( c == '&' )
                {
                    memcpy(data, "amp", 3);
                    data += 3;
                }
                else if( c == '\'' )
                {
                    memcpy(data, "apos", 4);
                    data += 4;
                }
                else if( c == '\"' )
                {
                    memcpy( data, "quot", 4);
                    data += 4;
                }
                else
                {
                    sprintf( data, "#x%02x", c );
                    data += 4;
                }
                *data++ = ';';
            }
            else
            {
                if( c == ' ' )
                    need_quote = 1;
                *data++ = c;
            }
        }
        if( !need_quote && (isdigit(str[0]) ||
            str[0] == '+' || str[0] == '-' || str[0] == '.' ))
            need_quote = 1;

        if( need_quote )
            *data++ = '\"';
        len = (int)(data - buf) - !need_quote;
        *data++ = '\0';
        data = buf + !need_quote;
    }

    icvXMLWriteScalar( fs, key, data, len );

    __END__;
}


static void
icvXMLWriteComment( CvFileStorage* fs, const char* comment, int eol_comment )
{
    CV_FUNCNAME( "cvXMLWriteComment" );

    __BEGIN__;

    int len;
    int multiline;
    const char* eol;
    char* ptr;

    if( !comment )
        CV_ERROR( CV_StsNullPtr, "Null comment" );

    if( strstr(comment, "--") != 0 )
        CV_ERROR( CV_StsBadArg, "Double hyphen \'--\' is not allowed in the comments" );

    len = (int)strlen(comment);
    eol = strchr(comment, '\n');
    multiline = eol != 0;
    ptr = fs->buffer;

    if( multiline || !eol_comment || fs->buffer_end - ptr < len + 5 )
        ptr = icvXMLFlush( fs );
    else if( ptr > fs->buffer_start + fs->struct_indent )
        *ptr++ = ' ';

    if( !multiline )
    {
        ptr = icvFSResizeWriteBuffer( fs, ptr, len + 9 );
        sprintf( ptr, "<!-- %s -->", comment );
        len = (int)strlen(ptr);
    }
    else
    {
        strcpy( ptr, "<!--" );
        len = 4;
    }

    fs->buffer = ptr + len;
    ptr = icvXMLFlush(fs);

    if( multiline )
    {
        while( comment )
        {
            if( eol )
            {
                ptr = icvFSResizeWriteBuffer( fs, ptr, (int)(eol - comment) + 1 );
                memcpy( ptr, comment, eol - comment + 1 );
                ptr += eol - comment;
                comment = eol + 1;
                eol = strchr( comment, '\n' );
            }
            else
            {
                len = (int)strlen(comment);
                ptr = icvFSResizeWriteBuffer( fs, ptr, len );
                memcpy( ptr, comment, len );
                ptr += len;
                comment = 0;
            }
            fs->buffer = ptr;
            ptr = icvXMLFlush( fs );
        }
        sprintf( ptr, "-->" );
        fs->buffer = ptr + 3;
        icvXMLFlush( fs );
    }
    
    __END__;
}
// 
// 
// /****************************************************************************************\
// *                              Common High-Level Functions                               *
// \****************************************************************************************/
// 
CV_IMPL CvFileStorage*
cvOpenFileStorage( const char* filename, CvMemStorage* dststorage, int flags )
{
    CvFileStorage* fs = 0;
    char* xml_buf = 0;

    CV_FUNCNAME("cvOpenFileStorage" );

    __BEGIN__;

    int default_block_size = 1 << 18;
    bool append = (flags & 3) == CV_STORAGE_APPEND;

    if( !filename )
        CV_ERROR( CV_StsNullPtr, "NULL filename" );

    CV_CALL( fs = (CvFileStorage*)cvAlloc( sizeof(*fs) ));
    memset( fs, 0, sizeof(*fs));

    CV_CALL( fs->memstorage = cvCreateMemStorage( default_block_size ));
    fs->dststorage = dststorage ? dststorage : fs->memstorage;

    CV_CALL( fs->filename = (char*)cvMemStorageAlloc( fs->memstorage, strlen(filename)+1 ));
    strcpy( fs->filename, filename );

    fs->flags = CV_FILE_STORAGE;
    fs->write_mode = (flags & 3) != 0;
    fs->file = fopen( fs->filename, !fs->write_mode ? "rt" : !append ? "wt" : "a+t" );
    if( !fs->file )
        EXIT;

    fs->roots = 0;
    fs->struct_indent = 0;
    fs->struct_flags = 0;
    fs->wrap_margin = 71;

    if( fs->write_mode )
    {
        // we use factor=6 for XML (the longest characters (' and ") are encoded with 6 bytes (&apos; and &quot;)
        // and factor=4 for YAML ( as we use 4 bytes for non ASCII characters (e.g. \xAB))
        int buf_size = CV_FS_MAX_LEN*(fs->is_xml ? 6 : 4) + 1024;

        char* dot_pos = strrchr( fs->filename, '.' );
        fs->is_xml = dot_pos && (strcmp( dot_pos, ".xml" ) == 0 ||
                      strcmp( dot_pos, ".XML" ) == 0 || strcmp( dot_pos, ".Xml" ) == 0);

        if( append )
            fseek( fs->file, 0, SEEK_END );

        fs->write_stack = cvCreateSeq( 0, sizeof(CvSeq), fs->is_xml ?
                sizeof(CvXMLStackRecord) : sizeof(int), fs->memstorage );
        fs->is_first = 1;
        fs->struct_indent = 0;
        fs->struct_flags = CV_NODE_EMPTY;
        CV_CALL( fs->buffer_start = fs->buffer = (char*)cvAlloc( buf_size + 1024 ));
        fs->buffer_end = fs->buffer_start + buf_size;
        if( fs->is_xml )
        {
            int file_size = (int)ftell( fs->file );
            CV_CALL( fs->strstorage = cvCreateChildMemStorage( fs->memstorage ));
            if( !append || file_size == 0 )
            {
                fputs( "<?xml version=\"1.0\"?>\n", fs->file );
                fputs( "<opencv_storage>\n", fs->file );
            }
            else
            {
                int xml_buf_size = 1 << 10;
                char substr[] = "</opencv_storage>";
                int last_occurence = -1;
                xml_buf_size = MIN(xml_buf_size, file_size);
                fseek( fs->file, -xml_buf_size, SEEK_END );
                CV_CALL(xml_buf = (char*)cvAlloc( xml_buf_size+2 ));
                // find the last occurence of </opencv_storage>
                for(;;)
                {
                    int line_offset = ftell( fs->file );
                    char* ptr0 = fgets( xml_buf, xml_buf_size, fs->file ), *ptr;
                    if( !ptr0 )
                        break;
                    ptr = ptr0;
                    for(;;)
                    {
                        ptr = strstr( ptr, substr );
                        if( !ptr )
                            break;
                        last_occurence = line_offset + (int)(ptr - ptr0);
                        ptr += strlen(substr);
                    }
                }
                if( last_occurence < 0 )
                    CV_ERROR( CV_StsError, "Could not find </opencv_storage> in the end of file.\n" );
                fclose( fs->file );
                fs->file = fopen( fs->filename, "r+t" );
                fseek( fs->file, last_occurence, SEEK_SET );
                // replace the last "</opencv_storage>" with " <!-- resumed -->", which has the same length
                fputs( " <!-- resumed -->", fs->file );
                fseek( fs->file, 0, SEEK_END );
                fputs( "\n", fs->file );
            }
            fs->start_write_struct = icvXMLStartWriteStruct;
            fs->end_write_struct = icvXMLEndWriteStruct;
            fs->write_int = icvXMLWriteInt;
            fs->write_real = icvXMLWriteReal;
            fs->write_string = icvXMLWriteString;
            fs->write_comment = icvXMLWriteComment;
            fs->start_next_stream = icvXMLStartNextStream;
        }
        else
        {
            if( !append )
                fputs( "%YAML:1.0\n", fs->file );
            else
                fputs( "...\n---\n", fs->file );
            fs->start_write_struct = icvYMLStartWriteStruct;
            fs->end_write_struct = icvYMLEndWriteStruct;
            fs->write_int = icvYMLWriteInt;
            fs->write_real = icvYMLWriteReal;
            fs->write_string = icvYMLWriteString;
            fs->write_comment = icvYMLWriteComment;
            fs->start_next_stream = icvYMLStartNextStream;
        }
    }
    else
    {
        int buf_size;
        const char* yaml_signature = "%YAML:";
        char buf[16];
        fgets( buf, sizeof(buf)-2, fs->file );
        fs->is_xml = strncmp( buf, yaml_signature, strlen(yaml_signature) ) != 0;
        
        fseek( fs->file, 0, SEEK_END );
        buf_size = ftell( fs->file );
        fseek( fs->file, 0, SEEK_SET );

        buf_size = MIN( buf_size, (1 << 20) );
        buf_size = MAX( buf_size, CV_FS_MAX_LEN*2 + 1024 );
        
        CV_CALL( fs->str_hash = cvCreateMap( 0, sizeof(CvStringHash),
                        sizeof(CvStringHashNode), fs->memstorage, 256 ));
        
        CV_CALL( fs->roots = cvCreateSeq( 0, sizeof(CvSeq),
                        sizeof(CvFileNode), fs->memstorage ));

        CV_CALL( fs->buffer = fs->buffer_start = (char*)cvAlloc( buf_size + 256 ));
        fs->buffer_end = fs->buffer_start + buf_size;
        fs->buffer[0] = '\n';
        fs->buffer[1] = '\0';

        //mode = cvGetErrMode();
        //cvSetErrMode( CV_ErrModeSilent );
        if( fs->is_xml )
            icvXMLParse( fs );
        else
            icvYMLParse( fs );
        //cvSetErrMode( mode );

        // release resources that we do not need anymore
        cvFree( &fs->buffer_start );
        fs->buffer = fs->buffer_end = 0;
    }

    __END__;

    if( fs )
    {
        if( cvGetErrStatus() < 0 || !fs->file )
        {
            cvReleaseFileStorage( &fs );
        }
        else if( !fs->write_mode )
        {
            fclose( fs->file );
            fs->file = 0;
        }
    }

    cvFree( &xml_buf );

    return  fs;
}
// 
// 
CV_IMPL void
cvStartWriteStruct( CvFileStorage* fs, const char* key, int struct_flags,
                    const char* type_name, CvAttrList /*attributes*/ )
{
    CV_FUNCNAME( "cvStartWriteStruct" );

    __BEGIN__;

    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    CV_CALL( fs->start_write_struct( fs, key, struct_flags, type_name ));

    __END__;
}
// 

CV_IMPL void
cvEndWriteStruct( CvFileStorage* fs )
{
    CV_FUNCNAME( "cvEndWriteStruct" );

    __BEGIN__;

    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    CV_CALL( fs->end_write_struct( fs ));

    __END__;
}
// 
// 
CV_IMPL void
cvWriteInt( CvFileStorage* fs, const char* key, int value )
{
    CV_FUNCNAME( "cvWriteInt" );

    __BEGIN__;

    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    CV_CALL( fs->write_int( fs, key, value ));

    __END__;
}
// 
// 
CV_IMPL void
cvWriteReal( CvFileStorage* fs, const char* key, double value )
{
    CV_FUNCNAME( "cvWriteReal" );

    __BEGIN__;

    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    CV_CALL( fs->write_real( fs, key, value ));

    __END__;
}
// 
// 
CV_IMPL void
cvWriteString( CvFileStorage* fs, const char* key, const char* value, int quote )
{
    CV_FUNCNAME( "cvWriteString" );

    __BEGIN__;

    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    CV_CALL( fs->write_string( fs, key, value, quote ));

    __END__;
}
// 
// 
CV_IMPL void
cvWriteComment( CvFileStorage* fs, const char* comment, int eol_comment )
{
    CV_FUNCNAME( "cvWriteComment" );

    __BEGIN__;

    CV_CHECK_OUTPUT_FILE_STORAGE(fs);
    CV_CALL( fs->write_comment( fs, comment, eol_comment ));

    __END__;
}
// 
// 

CV_IMPL const char*
cvGetFileNodeName( const CvFileNode* file_node )
{
    return file_node && CV_NODE_HAS_NAME(file_node->tag) ?
        ((CvFileMapNode*)file_node)->key->str.ptr : 0;
}

// /****************************************************************************************\
// *                                    RTTI Functions                                      *
// \****************************************************************************************/
// 
CvTypeInfo *CvType::first = 0, *CvType::last = 0;

CvType::CvType( const char* type_name,
                CvIsInstanceFunc is_instance, CvReleaseFunc release,
                CvReadFunc read, CvWriteFunc write, CvCloneFunc clone )
{
    CvTypeInfo _info;
    _info.flags = 0;
    _info.header_size = sizeof(_info);
    _info.type_name = type_name;
    _info.prev = _info.next = 0;
    _info.is_instance = is_instance;
    _info.release = release;
    _info.clone = clone;
    _info.read = read;
    _info.write = write;

    cvRegisterType( &_info );
    info = first;
}


CvType::~CvType()
{
    cvUnregisterType( info->type_name );
}
CV_IMPL  void
cvRegisterType( const CvTypeInfo* _info )
{
    CV_FUNCNAME("cvRegisterType" );

    __BEGIN__;

    CvTypeInfo* info = 0;
    int i, len;
    char c;
    
    //if( !CvType::first )
    //    icvCreateStandardTypes();

    if( !_info || _info->header_size != sizeof(CvTypeInfo) )
        CV_ERROR( CV_StsBadSize, "Invalid type info" );

    if( !_info->is_instance || !_info->release ||
        !_info->read || !_info->write )
        CV_ERROR( CV_StsNullPtr,
        "Some of required function pointers "
        "(is_instance, release, read or write) are NULL");

    c = _info->type_name[0];
    if( !isalpha(c) && c != '_' )
        CV_ERROR( CV_StsBadArg, "Type name should start with a letter or _" );

    len = (int)strlen(_info->type_name);

    for( i = 0; i < len; i++ )
    {
        c = _info->type_name[i];
        if( !isalnum(c) && c != '-' && c != '_' )
            CV_ERROR( CV_StsBadArg,
            "Type name should contain only letters, digits, - and _" );
    }

    CV_CALL( info = (CvTypeInfo*)cvAlloc( sizeof(*info) + len + 1 ));

    *info = *_info;
    info->type_name = (char*)(info + 1);
    memcpy( (char*)info->type_name, _info->type_name, len + 1 );

    info->flags = 0;
    info->next = CvType::first;
    info->prev = 0;
    if( CvType::first )
        CvType::first->prev = info;
    else
        CvType::last = info;
    CvType::first = info;

    __END__;
}

// 
CV_IMPL void
cvUnregisterType( const char* type_name )
{
    CV_FUNCNAME("cvUnregisterType" );

    __BEGIN__;

    CvTypeInfo* info;

    CV_CALL( info = cvFindType( type_name ));
    if( info )
    {
        if( info->prev )
            info->prev->next = info->next;
        else
            CvType::first = info->next;

        if( info->next )
            info->next->prev = info->prev;
        else
            CvType::last = info->prev;

        if( !CvType::first || !CvType::last )
            CvType::first = CvType::last = 0;

        cvFree( &info );
    }

    __END__;
}

// 
CV_IMPL CvTypeInfo*
cvFindType( const char* type_name )
{
    CvTypeInfo* info = 0;

    for( info = CvType::first; info != 0; info = info->next )
        if( strcmp( info->type_name, type_name ) == 0 )
            break;

    return info;
}

// 
CV_IMPL CvTypeInfo*
cvTypeOf( const void* struct_ptr )
{
    CvTypeInfo* info = 0;

    for( info = CvType::first; info != 0; info = info->next )
        if( info->is_instance( struct_ptr ))
            break;

    return info;
}
// 
// 
/* universal functions */
CV_IMPL void
cvRelease( void** struct_ptr )
{
    CV_FUNCNAME("cvRelease" );

    __BEGIN__;

    CvTypeInfo* info;

    if( !struct_ptr )
        CV_ERROR( CV_StsNullPtr, "NULL double pointer" );

    if( *struct_ptr )
    {
        CV_CALL( info = cvTypeOf( *struct_ptr ));
        if( !info )
            CV_ERROR( CV_StsError, "Unknown object type" );
        if( !info->release )
            CV_ERROR( CV_StsError, "release function pointer is NULL" );

        CV_CALL( info->release( struct_ptr ));
        *struct_ptr = 0;
    }

    __END__;
}

CV_IMPL void*
cvRead( CvFileStorage* fs, CvFileNode* node, CvAttrList* list )
{
    void* obj = 0;
    
    CV_FUNCNAME( "cvRead" );

    __BEGIN__;

    CV_CHECK_FILE_STORAGE( fs );

    if( !node )
        EXIT;

    if( !CV_NODE_IS_USER(node->tag) || !node->info )
        CV_ERROR( CV_StsError, "The node does not represent a user object (unknown type?)" );

    CV_CALL( obj = node->info->read( fs, node ));

    __END__;

    if( list )
        *list = cvAttrList(0,0);

    return obj;
}

CV_IMPL void*
cvLoad( const char* filename, CvMemStorage* memstorage,
        const char* name, const char** _real_name )
{
    void* ptr = 0;
    const char* real_name = 0;
    CvFileStorage* fs = 0;

    CV_FUNCNAME( "cvLoad" );

    __BEGIN__;

    CvFileNode* node = 0;
    CV_CALL( fs = cvOpenFileStorage( filename, memstorage, CV_STORAGE_READ ));
    
    if( !fs )
        EXIT;

    if( name )
    {
        CV_CALL( node = cvGetFileNodeByName( fs, 0, name ));
    }
    else
    {
        int i, k;
        for( k = 0; k < fs->roots->total; k++ )
        {
            CvSeq* seq;
            CvSeqReader reader;
        
            node = (CvFileNode*)cvGetSeqElem( fs->roots, k );
            if( !CV_NODE_IS_MAP( node->tag ))
                EXIT;
            seq = node->data.seq;
            node = 0;

            cvStartReadSeq( seq, &reader, 0 );

            // find the first element in the map
            for( i = 0; i < seq->total; i++ )
            {
                if( CV_IS_SET_ELEM( reader.ptr ))
                {
                    node = (CvFileNode*)reader.ptr;
                    goto stop_search;
                }
                CV_NEXT_SEQ_ELEM( seq->elem_size, reader );
            }
        }

stop_search:
        ;
    }

    if( !node )
        CV_ERROR( CV_StsObjectNotFound, "Could not find the/an object in file storage" );

    real_name = cvGetFileNodeName( node );
    CV_CALL( ptr = cvRead( fs, node, 0 ));

    // sanity check
    if( !memstorage && (CV_IS_SEQ( ptr ) || CV_IS_SET( ptr )) )
        CV_ERROR( CV_StsNullPtr,
        "NULL memory storage is passed - the loaded dynamic structure can not be stored" );

    __END__;

    cvReleaseFileStorage( &fs );
    if( cvGetErrStatus() < 0 )
    {
        cvRelease( (void**)&ptr );
        real_name = 0;
    }

    if( _real_name )
        *_real_name = real_name;

    return ptr;
}
// 
// /* End of file. */


// 
// /////////////////////////////////////////////////////////////////////////////////////////
// 
#define icvGivens_64f( n, x, y, c, s ) \
{                                      \
    int _i;                            \
    double* _x = (x);                  \
    double* _y = (y);                  \
                                       \
    for( _i = 0; _i < n; _i++ )        \
    {                                  \
        double t0 = _x[_i];            \
        double t1 = _y[_i];            \
        _x[_i] = t0*c + t1*s;          \
        _y[_i] = -t0*s + t1*c;         \
    }                                  \
}
// 
// 
// /* y[0:m,0:n] += diag(a[0:1,0:m]) * x[0:m,0:n] */
static  void
icvMatrAXPY_64f( int m, int n, const double* x, int dx,
                 const double* a, double* y, int dy )
{
    int i, j;

    for( i = 0; i < m; i++, x += dx, y += dy )
    {
        double s = a[i];

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = t0;
            y[j+1] = t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = t0;
            y[j+3] = t1;
        }

        for( ; j < n; j++ ) y[j] += s*x[j];
    }
}
// 
// 
// /* y[1:m,-1] = h*y[1:m,0:n]*x[0:1,0:n]'*x[-1]  (this is used for U&V reconstruction)
//    y[1:m,0:n] += h*y[1:m,0:n]*x[0:1,0:n]'*x[0:1,0:n] */
static void
icvMatrAXPY3_64f( int m, int n, const double* x, int l, double* y, double h )
{
    int i, j;

    for( i = 1; i < m; i++ )
    {
        double s = 0;

        y += l;

        for( j = 0; j <= n - 4; j += 4 )
            s += x[j]*y[j] + x[j+1]*y[j+1] + x[j+2]*y[j+2] + x[j+3]*y[j+3];

        for( ; j < n; j++ )  s += x[j]*y[j];

        s *= h;
        y[-1] = s*x[-1];

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = t0;
            y[j+1] = t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = t0;
            y[j+3] = t1;
        }

        for( ; j < n; j++ ) y[j] += s*x[j];
    }
}
// 
// 
#define icvGivens_32f( n, x, y, c, s ) \
{                                      \
    int _i;                            \
    float* _x = (x);                   \
    float* _y = (y);                   \
                                       \
    for( _i = 0; _i < n; _i++ )        \
    {                                  \
        double t0 = _x[_i];            \
        double t1 = _y[_i];            \
        _x[_i] = (float)(t0*c + t1*s); \
        _y[_i] = (float)(-t0*s + t1*c);\
    }                                  \
}

static  void
icvMatrAXPY_32f( int m, int n, const float* x, int dx,
                 const float* a, float* y, int dy )
{
    int i, j;

    for( i = 0; i < m; i++, x += dx, y += dy )
    {
        double s = a[i];

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = (float)t0;
            y[j+1] = (float)t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = (float)t0;
            y[j+3] = (float)t1;
        }

        for( ; j < n; j++ )
            y[j] = (float)(y[j] + s*x[j]);
    }
}


static void
icvMatrAXPY3_32f( int m, int n, const float* x, int l, float* y, double h )
{
    int i, j;

    for( i = 1; i < m; i++ )
    {
        double s = 0;
        y += l;

        for( j = 0; j <= n - 4; j += 4 )
            s += x[j]*y[j] + x[j+1]*y[j+1] + x[j+2]*y[j+2] + x[j+3]*y[j+3];

        for( ; j < n; j++ )  s += x[j]*y[j];

        s *= h;
        y[-1] = (float)(s*x[-1]);

        for( j = 0; j <= n - 4; j += 4 )
        {
            double t0 = y[j]   + s*x[j];
            double t1 = y[j+1] + s*x[j+1];
            y[j]   = (float)t0;
            y[j+1] = (float)t1;
            t0 = y[j+2] + s*x[j+2];
            t1 = y[j+3] + s*x[j+3];
            y[j+2] = (float)t0;
            y[j+3] = (float)t1;
        }

        for( ; j < n; j++ ) y[j] = (float)(y[j] + s*x[j]);
    }
}
// 
// /* accurate hypotenuse calculation */
static double
pythag( double a, double b )
{
    a = fabs( a );
    b = fabs( b );
    if( a > b )
    {
        b /= a;
        a *= sqrt( 1. + b * b );
    }
    else if( b != 0 )
    {
        a /= b;
        a = b * sqrt( 1. + a * a );
    }

    return a;
}
// 
// /****************************************************************************************/
// /****************************************************************************************/

#define MAX_ITERS  30

static void
icvSVD_64f( double* a, int lda, int m, int n,
            double* w,
            double* uT, int lduT, int nu,
            double* vT, int ldvT,
            double* buffer )
{
    double* e;
    double* temp;
    double *w1, *e1;
    double *hv;
    double ku0 = 0, kv0 = 0;
    double anorm = 0;
    double *a1, *u0 = uT, *v0 = vT;
    double scale, h;
    int i, j, k, l;
    int nm, m1, n1;
    int nv = n;
    int iters = 0;
    double* hv0 = (double*)cvStackAlloc( (m+2)*sizeof(hv0[0])) + 1; 

    e = buffer;
    w1 = w;
    e1 = e + 1;
    nm = n;
    
    temp = buffer + nm;

    memset( w, 0, nm * sizeof( w[0] ));
    memset( e, 0, nm * sizeof( e[0] ));

    m1 = m;
    n1 = n;

    /* transform a to bi-diagonal form */
    for( ;; )
    {
        int update_u;
        int update_v;
        
        if( m1 == 0 )
            break;

        scale = h = 0;
        update_u = uT && m1 > m - nu;
        hv = update_u ? uT : hv0;

        for( j = 0, a1 = a; j < m1; j++, a1 += lda )
        {
            double t = a1[0];
            scale += fabs( hv[j] = t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < m1; j++ )
            {
                double t = (hv[j] *= f);
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = f - g;
            h = 1. / (f * g - s);

            memset( temp, 0, n1 * sizeof( temp[0] ));

            /* calc temp[0:n-i] = a[i:m,i:n]'*hv[0:m-i] */
            icvMatrAXPY_64f( m1, n1 - 1, a + 1, lda, hv, temp + 1, 0 );
            for( k = 1; k < n1; k++ ) temp[k] *= h;

            /* modify a: a[i:m,i:n] = a[i:m,i:n] + hv[0:m-i]*temp[0:n-i]' */
            icvMatrAXPY_64f( m1, n1 - 1, temp + 1, 0, hv, a + 1, lda );
            *w1 = g*scale;
        }
        w1++;

        /* store -2/(hv'*hv) */
        if( update_u )
        {
            if( m1 == m )
                ku0 = h;
            else
                hv[-1] = h;
        }

        a++;
        n1--;
        if( vT )
            vT += ldvT + 1;

        if( n1 == 0 )
            break;

        scale = h = 0;
        update_v = vT && n1 > n - nv;

        hv = update_v ? vT : hv0;

        for( j = 0; j < n1; j++ )
        {
            double t = a[j];
            scale += fabs( hv[j] = t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < n1; j++ )
            {
                double t = (hv[j] *= f);
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = f - g;
            h = 1. / (f * g - s);
            hv[-1] = 0.;

            /* update a[i:m:i+1:n] = a[i:m,i+1:n] + (a[i:m,i+1:n]*hv[0:m-i])*... */
            icvMatrAXPY3_64f( m1, n1, hv, lda, a, h );

            *e1 = g*scale;
        }
        e1++;

        /* store -2/(hv'*hv) */
        if( update_v )
        {
            if( n1 == n )
                kv0 = h;
            else
                hv[-1] = h;
        }

        a += lda;
        m1--;
        if( uT )
            uT += lduT + 1;
    }

    m1 -= m1 != 0;
    n1 -= n1 != 0;

    /* accumulate left transformations */
    if( uT )
    {
        m1 = m - m1;
        uT = u0 + m1 * lduT;
        for( i = m1; i < nu; i++, uT += lduT )
        {
            memset( uT + m1, 0, (m - m1) * sizeof( uT[0] ));
            uT[i] = 1.;
        }

        for( i = m1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nu - i;

            l = m - i;

            hv = u0 + (lduT + 1) * i;
            h = i == 0 ? ku0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                uT = hv;
                icvMatrAXPY3_64f( lh, l-1, hv+1, lduT, uT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] *= s;
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * lduT] = 0;
                hv[0] = 1;
            }
        }
        uT = u0;
    }

    /* accumulate right transformations */
    if( vT )
    {
        n1 = n - n1;
        vT = v0 + n1 * ldvT;
        for( i = n1; i < nv; i++, vT += ldvT )
        {
            memset( vT + n1, 0, (n - n1) * sizeof( vT[0] ));
            vT[i] = 1.;
        }

        for( i = n1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nv - i;

            l = n - i;
            hv = v0 + (ldvT + 1) * i;
            h = i == 0 ? kv0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                vT = hv;
                icvMatrAXPY3_64f( lh, l-1, hv+1, ldvT, vT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] *= s;
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * ldvT] = 0;
                hv[0] = 1;
            }
        }
        vT = v0;
    }

    for( i = 0; i < nm; i++ )
    {
        double tnorm = fabs( w[i] );
        tnorm += fabs( e[i] );

        if( anorm < tnorm )
            anorm = tnorm;
    }

    anorm *= DBL_EPSILON;

    /* diagonalization of the bidiagonal form */
    for( k = nm - 1; k >= 0; k-- )
    {
        double z = 0;
        iters = 0;

        for( ;; )               /* do iterations */
        {
            double c, s, f, g, x, y;
            int flag = 0;

            /* test for splitting */
            for( l = k; l >= 0; l-- )
            {
                if( fabs(e[l]) <= anorm )
                {
                    flag = 1;
                    break;
                }
                assert( l > 0 );
                if( fabs(w[l - 1]) <= anorm )
                    break;
            }

            if( !flag )
            {
                c = 0;
                s = 1;

                for( i = l; i <= k; i++ )
                {
                    f = s * e[i];

                    e[i] *= c;

                    if( anorm + fabs( f ) == anorm )
                        break;

                    g = w[i];
                    h = pythag( f, g );
                    w[i] = h;
                    c = g / h;
                    s = -f / h;

                    if( uT )
                        icvGivens_64f( m, uT + lduT * (l - 1), uT + lduT * i, c, s );
                }
            }

            z = w[k];
            if( l == k || iters++ == MAX_ITERS )
                break;

            /* shift from bottom 2x2 minor */
            x = w[l];
            y = w[k - 1];
            g = e[k - 1];
            h = e[k];
            f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
            g = pythag( f, 1 );
            if( f < 0 )
                g = -g;
            f = x - (z / x) * z + (h / x) * (y / (f + g) - h);
            /* next QR transformation */
            c = s = 1;

            for( i = l + 1; i <= k; i++ )
            {
                g = e[i];
                y = w[i];
                h = s * g;
                g *= c;
                z = pythag( f, h );
                e[i - 1] = z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = -x * s + g * c;
                h = y * s;
                y *= c;

                if( vT )
                    icvGivens_64f( n, vT + ldvT * (i - 1), vT + ldvT * i, c, s );

                z = pythag( f, h );
                w[i - 1] = z;

                /* rotation can be arbitrary if z == 0 */
                if( z != 0 )
                {
                    c = f / z;
                    s = h / z;
                }
                f = c * g + s * y;
                x = -s * g + c * y;

                if( uT )
                    icvGivens_64f( m, uT + lduT * (i - 1), uT + lduT * i, c, s );
            }

            e[l] = 0;
            e[k] = f;
            w[k] = x;
        }                       /* end of iteration loop */

        if( iters > MAX_ITERS )
            break;

        if( z < 0 )
        {
            w[k] = -z;
            if( vT )
            {
                for( j = 0; j < n; j++ )
                    vT[j + k * ldvT] = -vT[j + k * ldvT];
            }
        }
    }                           /* end of diagonalization loop */

    /* sort singular values and corresponding values */
    for( i = 0; i < nm; i++ )
    {
        k = i;
        for( j = i + 1; j < nm; j++ )
            if( w[k] < w[j] )
                k = j;

        if( k != i )
        {
            double t;
            CV_SWAP( w[i], w[k], t );

            if( vT )
                for( j = 0; j < n; j++ )
                    CV_SWAP( vT[j + ldvT*k], vT[j + ldvT*i], t );

            if( uT )
                for( j = 0; j < m; j++ )
                    CV_SWAP( uT[j + lduT*k], uT[j + lduT*i], t );
        }
    }
}
// 
// 
static void
icvSVD_32f( float* a, int lda, int m, int n,
            float* w,
            float* uT, int lduT, int nu,
            float* vT, int ldvT,
            float* buffer )
{
    float* e;
    float* temp;
    float *w1, *e1;
    float *hv;
    double ku0 = 0, kv0 = 0;
    double anorm = 0;
    float *a1, *u0 = uT, *v0 = vT;
    double scale, h;
    int i, j, k, l;
    int nm, m1, n1;
    int nv = n;
    int iters = 0;
    float* hv0 = (float*)cvStackAlloc( (m+2)*sizeof(hv0[0])) + 1;

    e = buffer;

    w1 = w;
    e1 = e + 1;
    nm = n;
    
    temp = buffer + nm;

    memset( w, 0, nm * sizeof( w[0] ));
    memset( e, 0, nm * sizeof( e[0] ));

    m1 = m;
    n1 = n;

    /* transform a to bi-diagonal form */
    for( ;; )
    {
        int update_u;
        int update_v;
        
        if( m1 == 0 )
            break;

        scale = h = 0;

        update_u = uT && m1 > m - nu;
        hv = update_u ? uT : hv0;

        for( j = 0, a1 = a; j < m1; j++, a1 += lda )
        {
            double t = a1[0];
            scale += fabs( hv[j] = (float)t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < m1; j++ )
            {
                double t = (hv[j] = (float)(hv[j]*f));
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = (float)(f - g);
            h = 1. / (f * g - s);

            memset( temp, 0, n1 * sizeof( temp[0] ));

            /* calc temp[0:n-i] = a[i:m,i:n]'*hv[0:m-i] */
            icvMatrAXPY_32f( m1, n1 - 1, a + 1, lda, hv, temp + 1, 0 );

            for( k = 1; k < n1; k++ ) temp[k] = (float)(temp[k]*h);

            /* modify a: a[i:m,i:n] = a[i:m,i:n] + hv[0:m-i]*temp[0:n-i]' */
            icvMatrAXPY_32f( m1, n1 - 1, temp + 1, 0, hv, a + 1, lda );
            *w1 = (float)(g*scale);
        }
        w1++;
        
        /* store -2/(hv'*hv) */
        if( update_u )
        {
            if( m1 == m )
                ku0 = h;
            else
                hv[-1] = (float)h;
        }

        a++;
        n1--;
        if( vT )
            vT += ldvT + 1;

        if( n1 == 0 )
            break;

        scale = h = 0;
        update_v = vT && n1 > n - nv;
        hv = update_v ? vT : hv0;

        for( j = 0; j < n1; j++ )
        {
            double t = a[j];
            scale += fabs( hv[j] = (float)t );
        }

        if( scale != 0 )
        {
            double f = 1./scale, g, s = 0;

            for( j = 0; j < n1; j++ )
            {
                double t = (hv[j] = (float)(hv[j]*f));
                s += t * t;
            }

            g = sqrt( s );
            f = hv[0];
            if( f >= 0 )
                g = -g;
            hv[0] = (float)(f - g);
            h = 1. / (f * g - s);
            hv[-1] = 0.f;

            /* update a[i:m:i+1:n] = a[i:m,i+1:n] + (a[i:m,i+1:n]*hv[0:m-i])*... */
            icvMatrAXPY3_32f( m1, n1, hv, lda, a, h );

            *e1 = (float)(g*scale);
        }
        e1++;

        /* store -2/(hv'*hv) */
        if( update_v )
        {
            if( n1 == n )
                kv0 = h;
            else
                hv[-1] = (float)h;
        }

        a += lda;
        m1--;
        if( uT )
            uT += lduT + 1;
    }

    m1 -= m1 != 0;
    n1 -= n1 != 0;

    /* accumulate left transformations */
    if( uT )
    {
        m1 = m - m1;
        uT = u0 + m1 * lduT;
        for( i = m1; i < nu; i++, uT += lduT )
        {
            memset( uT + m1, 0, (m - m1) * sizeof( uT[0] ));
            uT[i] = 1.;
        }

        for( i = m1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nu - i;

            l = m - i;

            hv = u0 + (lduT + 1) * i;
            h = i == 0 ? ku0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                uT = hv;
                icvMatrAXPY3_32f( lh, l-1, hv+1, lduT, uT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] = (float)(hv[k]*s);
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * lduT] = 0;
                hv[0] = 1;
            }
        }
        uT = u0;
    }

    /* accumulate right transformations */
    if( vT )
    {
        n1 = n - n1;
        vT = v0 + n1 * ldvT;
        for( i = n1; i < nv; i++, vT += ldvT )
        {
            memset( vT + n1, 0, (n - n1) * sizeof( vT[0] ));
            vT[i] = 1.;
        }

        for( i = n1 - 1; i >= 0; i-- )
        {
            double s;
            int lh = nv - i;

            l = n - i;
            hv = v0 + (ldvT + 1) * i;
            h = i == 0 ? kv0 : hv[-1];

            assert( h <= 0 );

            if( h != 0 )
            {
                vT = hv;
                icvMatrAXPY3_32f( lh, l-1, hv+1, ldvT, vT+1, h );

                s = hv[0] * h;
                for( k = 0; k < l; k++ ) hv[k] = (float)(hv[k]*s);
                hv[0] += 1;
            }
            else
            {
                for( j = 1; j < l; j++ )
                    hv[j] = 0;
                for( j = 1; j < lh; j++ )
                    hv[j * ldvT] = 0;
                hv[0] = 1;
            }
        }
        vT = v0;
    }

    for( i = 0; i < nm; i++ )
    {
        double tnorm = fabs( w[i] );
        tnorm += fabs( e[i] );

        if( anorm < tnorm )
            anorm = tnorm;
    }

    anorm *= FLT_EPSILON;

    /* diagonalization of the bidiagonal form */
    for( k = nm - 1; k >= 0; k-- )
    {
        double z = 0;
        iters = 0;

        for( ;; )               /* do iterations */
        {
            double c, s, f, g, x, y;
            int flag = 0;

            /* test for splitting */
            for( l = k; l >= 0; l-- )
            {
                if( fabs( e[l] ) <= anorm )
                {
                    flag = 1;
                    break;
                }
                assert( l > 0 );
                if( fabs( w[l - 1] ) <= anorm )
                    break;
            }

            if( !flag )
            {
                c = 0;
                s = 1;

                for( i = l; i <= k; i++ )
                {
                    f = s * e[i];
                    e[i] = (float)(e[i]*c);

                    if( anorm + fabs( f ) == anorm )
                        break;

                    g = w[i];
                    h = pythag( f, g );
                    w[i] = (float)h;
                    c = g / h;
                    s = -f / h;

                    if( uT )
                        icvGivens_32f( m, uT + lduT * (l - 1), uT + lduT * i, c, s );
                }
            }

            z = w[k];
            if( l == k || iters++ == MAX_ITERS )
                break;

            /* shift from bottom 2x2 minor */
            x = w[l];
            y = w[k - 1];
            g = e[k - 1];
            h = e[k];
            f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
            g = pythag( f, 1 );
            if( f < 0 )
                g = -g;
            f = x - (z / x) * z + (h / x) * (y / (f + g) - h);
            /* next QR transformation */
            c = s = 1;

            for( i = l + 1; i <= k; i++ )
            {
                g = e[i];
                y = w[i];
                h = s * g;
                g *= c;
                z = pythag( f, h );
                e[i - 1] = (float)z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = -x * s + g * c;
                h = y * s;
                y *= c;

                if( vT )
                    icvGivens_32f( n, vT + ldvT * (i - 1), vT + ldvT * i, c, s );

                z = pythag( f, h );
                w[i - 1] = (float)z;

                /* rotation can be arbitrary if z == 0 */
                if( z != 0 )
                {
                    c = f / z;
                    s = h / z;
                }
                f = c * g + s * y;
                x = -s * g + c * y;

                if( uT )
                    icvGivens_32f( m, uT + lduT * (i - 1), uT + lduT * i, c, s );
            }

            e[l] = 0;
            e[k] = (float)f;
            w[k] = (float)x;
        }                       /* end of iteration loop */

        if( iters > MAX_ITERS )
            break;

        if( z < 0 )
        {
            w[k] = (float)(-z);
            if( vT )
            {
                for( j = 0; j < n; j++ )
                    vT[j + k * ldvT] = -vT[j + k * ldvT];
            }
        }
    }                           /* end of diagonalization loop */

    /* sort singular values and corresponding vectors */
    for( i = 0; i < nm; i++ )
    {
        k = i;
        for( j = i + 1; j < nm; j++ )
            if( w[k] < w[j] )
                k = j;

        if( k != i )
        {
            float t;
            CV_SWAP( w[i], w[k], t );

            if( vT )
                for( j = 0; j < n; j++ )
                    CV_SWAP( vT[j + ldvT*k], vT[j + ldvT*i], t );

            if( uT )
                for( j = 0; j < m; j++ )
                    CV_SWAP( uT[j + lduT*k], uT[j + lduT*i], t );
        }
    }
}
// 
// 
static void
icvSVBkSb_64f( int m, int n, const double* w,
               const double* uT, int lduT,
               const double* vT, int ldvT,
               const double* b, int ldb, int nb,
               double* x, int ldx, double* buffer )
{
    double threshold = 0;
    int i, j, nm = MIN( m, n );

    if( !b )
        nb = m;

    for( i = 0; i < n; i++ )
        memset( x + i*ldx, 0, nb*sizeof(x[0]));

    for( i = 0; i < nm; i++ )
        threshold += w[i];
    threshold *= 2*DBL_EPSILON;

    /* vT * inv(w) * uT * b */
    for( i = 0; i < nm; i++, uT += lduT, vT += ldvT )
    {
        double wi = w[i];

        if( wi > threshold )
        {
            wi = 1./wi;

            if( nb == 1 )
            {
                double s = 0;
                if( b )
                {
                    if( ldb == 1 )
                    {
                        for( j = 0; j <= m - 4; j += 4 )
                            s += uT[j]*b[j] + uT[j+1]*b[j+1] + uT[j+2]*b[j+2] + uT[j+3]*b[j+3];
                        for( ; j < m; j++ )
                            s += uT[j]*b[j];
                    }
                    else
                    {
                        for( j = 0; j < m; j++ )
                            s += uT[j]*b[j*ldb];
                    }
                }
                else
                    s = uT[0];
                s *= wi;
                if( ldx == 1 )
                {
                    for( j = 0; j <= n - 4; j += 4 )
                    {
                        double t0 = x[j] + s*vT[j];
                        double t1 = x[j+1] + s*vT[j+1];
                        x[j] = t0;
                        x[j+1] = t1;
                        t0 = x[j+2] + s*vT[j+2];
                        t1 = x[j+3] + s*vT[j+3];
                        x[j+2] = t0;
                        x[j+3] = t1;
                    }

                    for( ; j < n; j++ )
                        x[j] += s*vT[j];
                }
                else
                {
                    for( j = 0; j < n; j++ )
                        x[j*ldx] += s*vT[j];
                }
            }
            else
            {
                if( b )
                {
                    memset( buffer, 0, nb*sizeof(buffer[0]));
                    icvMatrAXPY_64f( m, nb, b, ldb, uT, buffer, 0 );
                    for( j = 0; j < nb; j++ )
                        buffer[j] *= wi;
                }
                else
                {
                    for( j = 0; j < nb; j++ )
                        buffer[j] = uT[j]*wi;
                }
                icvMatrAXPY_64f( n, nb, buffer, 0, vT, x, ldx );
            }
        }
    }
}
// 
// 
static void
icvSVBkSb_32f( int m, int n, const float* w,
               const float* uT, int lduT,
               const float* vT, int ldvT,
               const float* b, int ldb, int nb,
               float* x, int ldx, float* buffer )
{
    float threshold = 0.f;
    int i, j, nm = MIN( m, n );

    if( !b )
        nb = m;

    for( i = 0; i < n; i++ )
        memset( x + i*ldx, 0, nb*sizeof(x[0]));

    for( i = 0; i < nm; i++ )
        threshold += w[i];
    threshold *= 2*FLT_EPSILON;

    /* vT * inv(w) * uT * b */
    for( i = 0; i < nm; i++, uT += lduT, vT += ldvT )
    {
        double wi = w[i];
        
        if( wi > threshold )
        {
            wi = 1./wi;

            if( nb == 1 )
            {
                double s = 0;
                if( b )
                {
                    if( ldb == 1 )
                    {
                        for( j = 0; j <= m - 4; j += 4 )
                            s += uT[j]*b[j] + uT[j+1]*b[j+1] + uT[j+2]*b[j+2] + uT[j+3]*b[j+3];
                        for( ; j < m; j++ )
                            s += uT[j]*b[j];
                    }
                    else
                    {
                        for( j = 0; j < m; j++ )
                            s += uT[j]*b[j*ldb];
                    }
                }
                else
                    s = uT[0];
                s *= wi;

                if( ldx == 1 )
                {
                    for( j = 0; j <= n - 4; j += 4 )
                    {
                        double t0 = x[j] + s*vT[j];
                        double t1 = x[j+1] + s*vT[j+1];
                        x[j] = (float)t0;
                        x[j+1] = (float)t1;
                        t0 = x[j+2] + s*vT[j+2];
                        t1 = x[j+3] + s*vT[j+3];
                        x[j+2] = (float)t0;
                        x[j+3] = (float)t1;
                    }

                    for( ; j < n; j++ )
                        x[j] = (float)(x[j] + s*vT[j]);
                }
                else
                {
                    for( j = 0; j < n; j++ )
                        x[j*ldx] = (float)(x[j*ldx] + s*vT[j]);
                }
            }
            else
            {
                if( b )
                {
                    memset( buffer, 0, nb*sizeof(buffer[0]));
                    icvMatrAXPY_32f( m, nb, b, ldb, uT, buffer, 0 );
                    for( j = 0; j < nb; j++ )
                        buffer[j] = (float)(buffer[j]*wi);
                }
                else
                {
                    for( j = 0; j < nb; j++ )
                        buffer[j] = (float)(uT[j]*wi);
                }
                icvMatrAXPY_32f( n, nb, buffer, 0, vT, x, ldx );
            }
        }
    }
}
// 
// 
CV_IMPL  void
cvSVD( CvArr* aarr, CvArr* warr, CvArr* uarr, CvArr* varr, int flags )
{
    uchar* buffer = 0;
    int local_alloc = 0;

    CV_FUNCNAME( "cvSVD" );

    __BEGIN__;

    CvMat astub, *a = (CvMat*)aarr;
    CvMat wstub, *w = (CvMat*)warr;
    CvMat ustub, *u;
    CvMat vstub, *v;
    CvMat tmat;
    uchar* tw = 0;
    int type;
    int a_buf_offset = 0, u_buf_offset = 0, buf_size, pix_size;
    int temp_u = 0, /* temporary storage for U is needed */
        t_svd; /* special case: a->rows < a->cols */
    int m, n;
    int w_rows, w_cols;
    int u_rows = 0, u_cols = 0;
    int w_is_mat = 0;

    if( !CV_IS_MAT( a ))
        CV_CALL( a = cvGetMat( a, &astub ));

    if( !CV_IS_MAT( w ))
        CV_CALL( w = cvGetMat( w, &wstub ));

    if( !CV_ARE_TYPES_EQ( a, w ))
        CV_ERROR( CV_StsUnmatchedFormats, "" );

    if( a->rows >= a->cols )
    {
        m = a->rows;
        n = a->cols;
        w_rows = w->rows;
        w_cols = w->cols;
        t_svd = 0;
    }
    else
    {
        CvArr* t;
        CV_SWAP( uarr, varr, t );

        flags = (flags & CV_SVD_U_T ? CV_SVD_V_T : 0)|
                (flags & CV_SVD_V_T ? CV_SVD_U_T : 0);
        m = a->cols;
        n = a->rows;
        w_rows = w->cols;
        w_cols = w->rows;
        t_svd = 1;
    }

    u = (CvMat*)uarr;
    v = (CvMat*)varr;

    w_is_mat = w_cols > 1 && w_rows > 1;

    if( !w_is_mat && CV_IS_MAT_CONT(w->type) && w_cols + w_rows - 1 == n )
        tw = w->data.ptr;

    if( u )
    {
        if( !CV_IS_MAT( u ))
            CV_CALL( u = cvGetMat( u, &ustub ));

        if( !(flags & CV_SVD_U_T) )
        {
            u_rows = u->rows;
            u_cols = u->cols;
        }
        else
        {
            u_rows = u->cols;
            u_cols = u->rows;
        }

        if( !CV_ARE_TYPES_EQ( a, u ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( u_rows != m || (u_cols != m && u_cols != n))
            CV_ERROR( CV_StsUnmatchedSizes, !t_svd ? "U matrix has unappropriate size" :
                                                     "V matrix has unappropriate size" );
            
        temp_u = (u_rows != u_cols && !(flags & CV_SVD_U_T)) || u->data.ptr==a->data.ptr;

        if( w_is_mat && u_cols != w_rows )
            CV_ERROR( CV_StsUnmatchedSizes, !t_svd ? "U and W have incompatible sizes" :
                                                     "V and W have incompatible sizes" );
    }
    else
    {
        u = &ustub;
        u->data.ptr = 0;
        u->step = 0;
    }

    if( v )
    {
        int v_rows, v_cols;

        if( !CV_IS_MAT( v ))
            CV_CALL( v = cvGetMat( v, &vstub ));

        if( !(flags & CV_SVD_V_T) )
        {
            v_rows = v->rows;
            v_cols = v->cols;
        }
        else
        {
            v_rows = v->cols;
            v_cols = v->rows;
        }

        if( !CV_ARE_TYPES_EQ( a, v ))
            CV_ERROR( CV_StsUnmatchedFormats, "" );

        if( v_rows != n || v_cols != n )
            CV_ERROR( CV_StsUnmatchedSizes, t_svd ? "U matrix has unappropriate size" :
                                                    "V matrix has unappropriate size" );

        if( w_is_mat && w_cols != v_cols )
            CV_ERROR( CV_StsUnmatchedSizes, t_svd ? "U and W have incompatible sizes" :
                                                    "V and W have incompatible sizes" );
    }
    else
    {
        v = &vstub;
        v->data.ptr = 0;
        v->step = 0;
    }

    type = CV_MAT_TYPE( a->type );
    pix_size = CV_ELEM_SIZE(type);
    buf_size = n*2 + m;

    if( !(flags & CV_SVD_MODIFY_A) )
    {
        a_buf_offset = buf_size;
        buf_size += a->rows*a->cols;
    }

    if( temp_u )
    {
        u_buf_offset = buf_size;
        buf_size += u->rows*u->cols;
    }

    buf_size *= pix_size;

    if( buf_size <= CV_MAX_LOCAL_SIZE )
    {
        buffer = (uchar*)cvStackAlloc( buf_size );
        local_alloc = 1;
    }
    else
    {
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));
    }
    
    if( !(flags & CV_SVD_MODIFY_A) )
    {
        cvInitMatHeader( &tmat, m, n, type,
                         buffer + a_buf_offset*pix_size );
        if( !t_svd )
            cvCopy( a, &tmat );
        else
            cvT( a, &tmat );
        a = &tmat;
    }

    if( temp_u )
    {
        cvInitMatHeader( &ustub, u_cols, u_rows, type, buffer + u_buf_offset*pix_size );
        u = &ustub;
    }

    if( !tw )
        tw = buffer + (n + m)*pix_size;

    if( type == CV_32FC1 )
    {
        icvSVD_32f( a->data.fl, a->step/sizeof(float), a->rows, a->cols,
                   (float*)tw, u->data.fl, u->step/sizeof(float), u_cols,
                   v->data.fl, v->step/sizeof(float), (float*)buffer );
    }
    else if( type == CV_64FC1 )
    {
        icvSVD_64f( a->data.db, a->step/sizeof(double), a->rows, a->cols,
                    (double*)tw, u->data.db, u->step/sizeof(double), u_cols,
                    v->data.db, v->step/sizeof(double), (double*)buffer );
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    if( tw != w->data.ptr )
    {
        int shift = w->cols != 1;
        cvSetZero( w );
        if( type == CV_32FC1 )
            for( int i = 0; i < n; i++ )
                ((float*)(w->data.ptr + i*w->step))[i*shift] = ((float*)tw)[i];
        else
            for( int i = 0; i < n; i++ )
                ((double*)(w->data.ptr + i*w->step))[i*shift] = ((double*)tw)[i];
    }

    if( uarr )
    {
        if( !(flags & CV_SVD_U_T))
            cvT( u, uarr );
        else if( temp_u )
            cvCopy( u, uarr );
        /*CV_CHECK_NANS( uarr );*/
    }

    if( varr )
    {
        if( !(flags & CV_SVD_V_T))
            cvT( v, varr );
        /*CV_CHECK_NANS( varr );*/
    }

    CV_CHECK_NANS( w );

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}
// 
// 
CV_IMPL void
cvSVBkSb( const CvArr* warr, const CvArr* uarr,
          const CvArr* varr, const CvArr* barr,
          CvArr* xarr, int flags )
{
    uchar* buffer = 0;
    int local_alloc = 0;

    CV_FUNCNAME( "cvSVBkSb" );

    __BEGIN__;

    CvMat wstub, *w = (CvMat*)warr;
    CvMat bstub, *b = (CvMat*)barr;
    CvMat xstub, *x = (CvMat*)xarr;
    CvMat ustub, ustub2, *u = (CvMat*)uarr;
    CvMat vstub, vstub2, *v = (CvMat*)varr;
    uchar* tw = 0;
    int type;
    int temp_u = 0, temp_v = 0;
    int u_buf_offset = 0, v_buf_offset = 0, w_buf_offset = 0, t_buf_offset = 0;
    int buf_size = 0, pix_size;
    int m, n, nm;
    int u_rows, u_cols;
    int v_rows, v_cols;

    if( !CV_IS_MAT( w ))
        CV_CALL( w = cvGetMat( w, &wstub ));

    if( !CV_IS_MAT( u ))
        CV_CALL( u = cvGetMat( u, &ustub ));

    if( !CV_IS_MAT( v ))
        CV_CALL( v = cvGetMat( v, &vstub ));

    if( !CV_IS_MAT( x ))
        CV_CALL( x = cvGetMat( x, &xstub ));

    if( !CV_ARE_TYPES_EQ( w, u ) || !CV_ARE_TYPES_EQ( w, v ) || !CV_ARE_TYPES_EQ( w, x ))
        CV_ERROR( CV_StsUnmatchedFormats, "All matrices must have the same type" );

    type = CV_MAT_TYPE( w->type );
    pix_size = CV_ELEM_SIZE(type);

    if( !(flags & CV_SVD_U_T) )
    {
        temp_u = 1;
        u_buf_offset = buf_size;
        buf_size += u->cols*u->rows*pix_size;
        u_rows = u->rows;
        u_cols = u->cols;
    }
    else
    {
        u_rows = u->cols;
        u_cols = u->rows;
    }

    if( !(flags & CV_SVD_V_T) )
    {
        temp_v = 1;
        v_buf_offset = buf_size;
        buf_size += v->cols*v->rows*pix_size;
        v_rows = v->rows;
        v_cols = v->cols;
    }
    else
    {
        v_rows = v->cols;
        v_cols = v->rows;
    }

    m = u_rows;
    n = v_rows;
    nm = MIN(n,m);

    if( (u_rows != u_cols && v_rows != v_cols) || x->rows != v_rows )
        CV_ERROR( CV_StsBadSize, "V or U matrix must be square" );

    if( (w->rows == 1 || w->cols == 1) && w->rows + w->cols - 1 == nm )
    {
        if( CV_IS_MAT_CONT(w->type) )
            tw = w->data.ptr;
        else
        {
            w_buf_offset = buf_size;
            buf_size += nm*pix_size;
        }
    }
    else
    {
        if( w->cols != v_cols || w->rows != u_cols )
            CV_ERROR( CV_StsBadSize, "W must be 1d array of MIN(m,n) elements or "
                                    "matrix which size matches to U and V" );
        w_buf_offset = buf_size;
        buf_size += nm*pix_size;
    }

    if( b )
    {
        if( !CV_IS_MAT( b ))
            CV_CALL( b = cvGetMat( b, &bstub ));
        if( !CV_ARE_TYPES_EQ( w, b ))
            CV_ERROR( CV_StsUnmatchedFormats, "All matrices must have the same type" );
        if( b->cols != x->cols || b->rows != m )
            CV_ERROR( CV_StsUnmatchedSizes, "b matrix must have (m x x->cols) size" );
    }
    else
    {
        b = &bstub;
        memset( b, 0, sizeof(*b));
    }

    t_buf_offset = buf_size;
    buf_size += (MAX(m,n) + b->cols)*pix_size;

    if( buf_size <= CV_MAX_LOCAL_SIZE )
    {
        buffer = (uchar*)cvStackAlloc( buf_size );
        local_alloc = 1;
    }
    else
        CV_CALL( buffer = (uchar*)cvAlloc( buf_size ));

    if( temp_u )
    {
        cvInitMatHeader( &ustub2, u_cols, u_rows, type, buffer + u_buf_offset );
        cvT( u, &ustub2 );
        u = &ustub2;
    }

    if( temp_v )
    {
        cvInitMatHeader( &vstub2, v_cols, v_rows, type, buffer + v_buf_offset );
        cvT( v, &vstub2 );
        v = &vstub2;
    }

    if( !tw )
    {
        int i, shift = w->cols > 1 ? pix_size : 0;
        tw = buffer + w_buf_offset;
        for( i = 0; i < nm; i++ )
            memcpy( tw + i*pix_size, w->data.ptr + i*(w->step + shift), pix_size );
    }

    if( type == CV_32FC1 )
    {
        icvSVBkSb_32f( m, n, (float*)tw, u->data.fl, u->step/sizeof(float),
                       v->data.fl, v->step/sizeof(float),
                       b->data.fl, b->step/sizeof(float), b->cols,
                       x->data.fl, x->step/sizeof(float),
                       (float*)(buffer + t_buf_offset) );
    }
    else if( type == CV_64FC1 )
    {
        icvSVBkSb_64f( m, n, (double*)tw, u->data.db, u->step/sizeof(double),
                       v->data.db, v->step/sizeof(double),
                       b->data.db, b->step/sizeof(double), b->cols,
                       x->data.db, x->step/sizeof(double),
                       (double*)(buffer + t_buf_offset) );
    }
    else
    {
        CV_ERROR( CV_StsUnsupportedFormat, "" );
    }

    __END__;

    if( buffer && !local_alloc )
        cvFree( &buffer );
}
// 
// /* End of file. */
