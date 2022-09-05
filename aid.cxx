// AID: Aggregate Image Data

#define _OLE32_

#include <windows.h>

#include <ppl.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <eh.h>
#include <math.h>

#include <memory>
#include <mutex>

#include <djlimagedata.hxx>
#include <djlenum.hxx>
#include <djlexcept.hxx>
#include <djl_sha256.hxx>

using namespace std;
using namespace concurrency;

#pragma comment(lib, "bcrypt.lib")

CDJLTrace tracer;

const int MetadataBufferSize = 100;

enum EnumAppMode { modeSerialNumbers, modeFocalLengths, modeModels, modeLenses, modeHasImage, modeHasGPS, modeEmbedded, modeAdobeEdits, modeRatings };

class GenericEntry
{
    private:
        size_t count;

    public:
        GenericEntry() { count = 1; }
        size_t Count() { return count; }
        void IncrementCount() { count++; }

        static int EntryCompareCount( const void * a, const void * b )
        {
            GenericEntry *pa = (GenericEntry *) a;
            GenericEntry *pb = (GenericEntry *) b;

            // sort from largest to smallest
    
            if ( pa->count > pb->count )
                return -1;
    
            if ( pa->count < pb->count )
                return 1;
    
            return 0;
        }
};

class EmbeddedImageEntry : public GenericEntry
{
    private:
        unsigned int offset;
        unsigned int length;
        char acSha256[ 65 ];
        WCHAR awcPath[ MAX_PATH ];

    public:
        EmbeddedImageEntry( char * pcSha256, unsigned int off, unsigned int len, WCHAR * path )
        {
            offset = off;
            length = len;
            strcpy( acSha256, pcSha256 );
            wcscpy( awcPath, path );
        }

        bool Same( EmbeddedImageEntry & entry )
        {
            return ( entry.length == length && !strcmp( entry.acSha256, acSha256 ) );
        }

        static int EntryCompare( const void * a, const void * b )
        {
            EmbeddedImageEntry *pa = (EmbeddedImageEntry *) a;
            EmbeddedImageEntry *pb = (EmbeddedImageEntry *) b;
    
            if ( pa->length > pb->length )
                return 1;
    
            if ( pa->length < pb->length )
                return -1;

            return ( strcmp( pa->acSha256, pb->acSha256 ) );
        } //EntryCompare

        unsigned int Offset() { return offset; };
        unsigned int Length() { return length; };
        WCHAR * Path() { return awcPath; };
    
        static void PrintHeader()
        {
            printf( "   length     count sha256\n" );
            printf( "   ------     ----- ------\n" );
        }
    
        void PrintItem()
        {
            printf( "%9u %9zu %s %ws\n", length, Count(), acSha256, awcPath );
        }
};

class FocalLengthEntry : public GenericEntry
{
    private:
        unsigned int focalLength;
    
    public:
        FocalLengthEntry( unsigned int fl )
        {
            focalLength = fl;
        }
    
        bool Same( FocalLengthEntry & entry )
        {
            return focalLength == entry.focalLength;
        }
    
        static int EntryCompare( const void * a, const void * b )
        {
            FocalLengthEntry *pa = (FocalLengthEntry *) a;
            FocalLengthEntry *pb = (FocalLengthEntry *) b;
    
            if ( pa->focalLength > pb->focalLength )
                return 1;
    
            if ( pa->focalLength < pb->focalLength )
                return -1;
    
            return 0;
        } //EntryCompare
    
        static void PrintHeader()
        {
            printf( "focal length        count\n" );
            printf( "------------        -----\n" );
        }
    
        void PrintItem()
        {
            printf( "%12u %12zu\n", focalLength, Count() );
        }
};

class RatingEntry : public GenericEntry
{
    private:
        int rating;
    
    public:
        RatingEntry( int r )
        {
            rating = r;
        }
    
        bool Same( RatingEntry & entry )
        {
            return rating == entry.rating;
        }
    
        static int EntryCompare( const void * a, const void * b )
        {
            RatingEntry *pa = (RatingEntry *) a;
            RatingEntry *pb = (RatingEntry *) b;
    
            if ( pa->rating > pb->rating )
                return 1;
    
            if ( pa->rating < pb->rating )
                return -1;
    
            return 0;
        } //EntryCompare
    
        static void PrintHeader()
        {
            printf( "rating              count\n" );
            printf( "------------        -----\n" );
        }
    
        void PrintItem()
        {
            printf( "%12d %12zu\n", rating, Count() );
        }
};

class SerialNumberEntry : public GenericEntry
{
    private:
        char acMake[ MetadataBufferSize ];
        char acModel[ MetadataBufferSize ];
        char acSerialNumber[ MetadataBufferSize ];
    
    public:
        SerialNumberEntry( char * pcMake, char * pcModel, char * pcSerialNumber )
        {
            strcpy( acMake, pcMake );
            strcpy( acModel, pcModel );
            strcpy( acSerialNumber, pcSerialNumber );
        }
    
        bool Same( SerialNumberEntry & entry )
        {
            return !stricmp( acSerialNumber, entry.acSerialNumber ) &&
                   !stricmp( acModel, entry.acModel ) &&
                   !stricmp( acMake, entry.acMake );
        }
    
        static int EntryCompare( const void * a, const void * b )
        {
            SerialNumberEntry *pa = (SerialNumberEntry *) a;
            SerialNumberEntry *pb = (SerialNumberEntry *) b;
    
            int diff = strcmp( pa->acMake, pb->acMake );
    
            if ( 0 == diff )
                diff = strcmp( pa->acModel, pb->acModel );
    
            if ( 0 == diff )
                diff = strcmp( pa->acSerialNumber, pb->acSerialNumber );
    
            return diff;
        } //EntryCompare
    
        static void PrintHeader()
        {
            printf( "make                           model                                            serial number                                             count\n" );
            printf( "----                           -----                                            -------------                                             -----\n" );
        }
    
        void PrintItem()
        {
            printf( "%-29s  %-47s  %-50s %12zu\n", acMake, acModel, acSerialNumber, Count() );
        }
};

class ModelEntry : public GenericEntry
{
    private:
        char acMake[ MetadataBufferSize ];
        char acModel[ MetadataBufferSize ];
    
    public:
        ModelEntry( char * pcMake, char * pcModel )
        {
            strcpy( acMake, pcMake );
            strcpy( acModel, pcModel );
        }
    
        bool Same( ModelEntry & entry )
        {
            //return !stricmp( acModel, entry.acModel ) && !stricmp( acMake, entry.acMake );

            return !stricmp( acModel, entry.acModel );
        }
    
        static int EntryCompare( const void * a, const void * b )
        {
            ModelEntry *pa = (ModelEntry *) a;
            ModelEntry *pb = (ModelEntry *) b;
    
            int diff = strcmp( pa->acModel, pb->acModel );

            //if ( 0 == diff )
            //    diff = strcmp( pa->acMake, pb->acMake );

            return diff;
        } //EntryCompare
    
        static void PrintHeader()
        {
            printf( "make                           model                                                   count\n" );
            printf( "----                           -----                                                   -----\n" );
        }
    
        void PrintItem()
        {
            printf( "%-29s  %-47s  %12zu\n", acMake, acModel, Count() );
        }
};

template<class T> class CEntryTracker
{
    private:
        std::mutex g_mtx;
        vector<T> entries;

        void SortEntries( bool sortOnCount )
        {
            if ( sortOnCount )
                qsort( entries.data(), entries.size(), sizeof( T ), T::EntryCompareCount );
            else
                qsort( entries.data(), entries.size(), sizeof( T ), T::EntryCompare );
        }

    public:
        CEntryTracker() {}

        size_t Count() { return entries.size(); }

        T & operator[] ( size_t i ) { return entries[ i ]; }

        void AddOrUpdate( T & item )
        {
            lock_guard<mutex> lock( g_mtx );

            // Sure, it's a linear search under a lock, but there just aren't that many unique values
            // and the hotspots are likely elsewhere. (specifically: CreateFile and ReadFile). This function: 0.13%

            for ( int i = 0; i < entries.size(); i++ )
            {
                if ( entries[i].Same( item ) )
                {
                    entries[ i ].IncrementCount();
                    return;
                }
            }

            entries.push_back( item );
        }

        void PrintEntries( const char * entryType, bool sortOnCount = false )
        {
            size_t fileCount = 0;

            for ( int i = 0; i < entries.size(); i++ )
                fileCount += entries[ i ].Count();

            printf( "found %Iu unique %s in %Iu files with that data\n", entries.size(), entryType, fileCount );
            SortEntries( sortOnCount );

            T::PrintHeader();

            for ( int i = 0; i < entries.size(); i++ )
            {
                entries[i].PrintItem();
            }
        }
};

void Usage()
{
    printf( "usage: aid [filename] /p:[rootpath] /e:[extesion] /a:X /m:[model] [/v]\n" );
    printf( "Aggregate Image Data\n" );
    printf( "       filename       Retrieves data of just one file. Can't be used with /p and /e.\n" );
    printf( "       /a:X           App Mode. Default is Serial Numbers\n" );
    printf( "                          a   Adobe Edits\n" );
    printf( "                          e   Embedded Images (flac/mp3)\n" );
    printf( "                          f   Focal Lengths\n" );
    printf( "                          g   GPS data\n" );
    printf( "                          i   Valid image embedded (e.g. flac files)\n" );
    printf( "                          l   Lens Models\n" );
    printf( "                          m   Models\n" );
    printf( "                          s   Serial Numbers\n" );
    printf( "                          r   Rating\n" );
    printf( "       /c             Used with /a:e, creates a file for each embedded image in the 'out' subdirectory.\n" );
    printf( "       /e:            Specifies the file extension to include. Default is *\n" );
    printf( "       /m:            Used with /p and /e. The model substring must be in the EquipModel case insensitive.\n" );
    printf( "       /o             Use One thread for parsing files, not parallelized. (enumeration uses many threads).\n" );
    printf( "       /p:            Specifies the root of the file system enumeration.\n" );
    printf( "       /s:X           Sort criteria. Default is App Mode setting /a\n" );
    printf( "                          c   Count of entries\n" );
    printf( "       /v             Enable verbose tracing.\n" );
    printf( "   examples:    aid c:\\pictures\\whitney.jpg\n" );
    printf( "                aid /p:c:\\pictures /e:jpg\n" );
    printf( "                aid /a:f /p:c:\\pictures /e:jpg\n" );
    printf( "                aid /p:c:\\pictures /e:dng /m:leica\n" );
    printf( "                aid /p:d:\\ /e:cr? /a:l /s:c\n" );
    printf( "                aid /p:d:\\ /e:rw2 /a:m /s:c\n" );
    printf( "   notes:       Supported extensions: JPG, TIF, RW2, RAF, ARW, .ORF, .CR2, .CR3, .NEF, .DNG, .FLAC, .MP3, etc.\n" );
    exit( 1 );
} //Usage

bool ModelInName( char * pcCameraModel, char * pcName )
{
    if ( ( NULL == pcCameraModel ) || ( 0 == *pcCameraModel ) )
        return true;

    char acLower[ MetadataBufferSize ];
    strcpy( acLower, pcCameraModel );
    strlwr( acLower );
    return ( NULL != strstr( acLower, pcName ) );
} //ModelInName

void AppendBackslashAndLowercase( WCHAR * pwc )
{
    _wcslwr( pwc );

    int i = wcslen( pwc );

    if ( ( i > 0 ) && ( L'\\' != pwc[ i - 1 ] ) )
    {
        pwc[ i++ ] = L'\\';
        pwc[ i ] = 0;
    }
} //AppendBackslash

const WCHAR * ImageExtension( unsigned long long x )
{
    if ( 0xd8ff == ( x & 0xffff ) )
        return L".jpg";

    if ( 0x5089 == ( x & 0xffff ) )
        return L".png";

    if ( 0x4d42 == ( x & 0xffff ) )
        return L".bmp";

    if ( 7079746620000000 == x )
        return L".heif";

    return L".jpg"; // wic will know what to do
} //ImageExtension

void CreateEmbeddedImages( CEntryTracker<EmbeddedImageEntry> & embeddedImages )
{
    WCHAR awc[ MAX_PATH ];
    DWORD dw = GetCurrentDirectory( _countof( awc ), awc );
    if ( 0 == dw )
    {
        printf( "can't get current directory\n" );
        return;
    }

    printf( "current directory: %ws\n", awc );
    int len = wcslen( awc );
    if ( L'\\' != awc[ len - 1 ] )
        wcscat( awc, L"\\" );

    wcscat( awc, L"out\\" );
    BOOL ok = CreateDirectory( awc, 0 );
    if ( !ok )
    {
        printf( "can't create directory %ws\n", awc );
        return;
    }

    int dirlen = wcslen( awc );

    for ( size_t i = 0; i < embeddedImages.Count(); i++ )
    {
        //printf( "o %d, l %d, file %ws\n", embeddedImages[i].Offset(), embeddedImages[i].Length(), embeddedImages[i].Path() );

        WCHAR * filename = wcsrchr( embeddedImages[i].Path(), L'\\' );

        if ( 0 != filename )
        {
            wcscpy( awc + dirlen, filename );
            WCHAR * pwcExtension = wcsrchr( awc, L'.' );
            if ( 0 != pwcExtension && embeddedImages[i].Length() > 8 )
            {
                CStream stream( embeddedImages[i].Path(), embeddedImages[i].Offset(), embeddedImages[i].Length() );
                vector<byte> vImage( embeddedImages[i].Length() );
                stream.Read( vImage.data(), embeddedImages[i].Length() );

                unsigned long long header = 0;
                memcpy( &header, vImage.data(), sizeof header );
                wcscpy( pwcExtension, ImageExtension( header ) );
    
                HANDLE h = CreateFile( awc, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
                if ( INVALID_HANDLE_VALUE != h )
                {
    
                    DWORD dwWritten = 0;
                    WriteFile( h, vImage.data(), embeddedImages[i].Length(), &dwWritten, NULL );
    
                    CloseHandle( h );
                }
            }
        }
    }
} //CreateEmbeddedImages

void ProcessFile(
    EnumAppMode appMode,
    bool verboseTracing,
    std::mutex & mtx,
    char * acCameraModel,
    LONG & hasImageCount,
    LONG & hasGPSCount,
    CStringArray & array,
    int i,
    CEntryTracker<SerialNumberEntry> & bodies,
    CEntryTracker<SerialNumberEntry> & lenses,
    CEntryTracker<FocalLengthEntry> & focalLengths,
    CEntryTracker<RatingEntry> & ratings,
    CEntryTracker<ModelEntry> & models,
    CEntryTracker<EmbeddedImageEntry> & embeddedImages,
    LONG & withAdobeEdits,
    LONG & withoutAdobeEdits )
{
    unique_ptr<CImageData> id( new CImageData() );
    char acModel[ MetadataBufferSize ]; acModel[0] = 0;

    if ( EnumAppMode::modeAdobeEdits == appMode )
    {
        bool edits = id->HoldsAdobeEditsInXMP( array[ i ] );

        if ( verboseTracing )
        {
            lock_guard<mutex> lock( mtx );
            printf( "adobe edits: %s in file %ws\n", edits ? "yes" : "no ", array[ i ] );
        }

        if ( edits )
            InterlockedIncrement( & withAdobeEdits );
        else
            InterlockedIncrement( & withoutAdobeEdits );
    }
    else if ( EnumAppMode::modeSerialNumbers == appMode )
    {
        char acMake[ MetadataBufferSize ]; acMake[0] = 0;
        char acSerialNumber[ MetadataBufferSize ]; acSerialNumber[0] = 0;
        char acLensMake[ MetadataBufferSize ]; acLensMake[0] = 0;
        char acLensModel[ MetadataBufferSize ]; acLensModel[0] = 0;
        char acLensSerialNumber[ MetadataBufferSize ]; acLensSerialNumber[0] = 0;

        bool ok = id->GetSerialNumbers( array[ i ], acMake, MetadataBufferSize, acModel, MetadataBufferSize, acSerialNumber, MetadataBufferSize,
                                        acLensMake, MetadataBufferSize, acLensModel, MetadataBufferSize, acLensSerialNumber, MetadataBufferSize );

        if ( ok && ModelInName( acModel, acCameraModel ) )
        {
            if ( verboseTracing )
            {
                lock_guard<mutex> lock( mtx );
                printf( "serial number information for %ws\n", array[ i ] );

                printf( "  make:          %s\n", acMake );
                printf( "  model:         %s\n", acModel );
                printf( "  serial #:      %s\n", acSerialNumber );

                printf( "  lens make:     %s\n", acLensMake );
                printf( "  lens model:    %s\n", acLensModel );
                printf( "  lens serial #: %s\n", acLensSerialNumber );

                if ( 0 != acSerialNumber[0] && 0 == acMake[0] && 0 == acModel[0] )
                    printf( "serial number with no camera make/model!!!!\n" );

                if ( 0 != acLensSerialNumber[0] && 0 == acLensMake[0] && 0 == acLensModel[0] )
                    printf( "serial number with no lens make/model!!!!\n" );
            }

            if ( 0 != acSerialNumber[ 0 ] )
            {
                SerialNumberEntry body( acMake, acModel, acSerialNumber );
                bodies.AddOrUpdate( body );
            }

            if ( 0 != acLensSerialNumber[ 0 ] )
            {
                SerialNumberEntry lens( acLensMake, acLensModel, acLensSerialNumber );
                lenses.AddOrUpdate( lens );
            }
        }
    }
    else if ( EnumAppMode::modeFocalLengths == appMode )
    {
        double focalLengthLens, flGuess, flComputed;
        int flIn35mmFilm;
        
        double flBestGuess = id->FindFocalLength( array[ i ], focalLengthLens, flIn35mmFilm, flGuess, flComputed, acModel, _countof( acModel ) );

        if ( 0.0 != flBestGuess && ModelInName( acModel, acCameraModel ) )
        {
            unsigned int focalLen = (unsigned int) lroundl( flBestGuess );

            FocalLengthEntry fl( focalLen );
            focalLengths.AddOrUpdate( fl );
        }
    }
    else if ( EnumAppMode::modeRatings == appMode )
    {
        int rating;

        bool found = id->GetRating( array[ i ], rating );

        if ( found )
        {
            RatingEntry re( rating );
            ratings.AddOrUpdate( re );
        }
    }
    else if ( EnumAppMode::modeModels == appMode )
    {
        char acMake[ MetadataBufferSize ] = { 0 };
        acModel[ 0 ] = 0;
        bool ok = id->GetCameraInfo( array[ i ], acMake, MetadataBufferSize, acModel, MetadataBufferSize );

        if ( ok && ModelInName( acModel, acCameraModel ) )
        {
            if ( verboseTracing )
            {
                lock_guard<mutex> lock( mtx );
                printf( "model information for %ws\n", array[ i ] );

                printf( "  make:          %s\n", acMake );
                printf( "  model:         %s\n", acModel );
            }

            if ( 0 != acModel[ 0 ] )
            {
                ModelEntry model( acMake, acModel );
                models.AddOrUpdate( model );
            }
        }
    }
    else if ( EnumAppMode::modeLenses == appMode )
    {
        char acMake[ MetadataBufferSize ]; acMake[0] = 0;
        char acSerialNumber[ MetadataBufferSize ]; acSerialNumber[0] = 0;
        char acLensMake[ MetadataBufferSize ]; acLensMake[0] = 0;
        char acLensModel[ MetadataBufferSize ]; acLensModel[0] = 0;
        char acLensSerialNumber[ MetadataBufferSize ]; acLensSerialNumber[0] = 0;

        bool ok = id->GetSerialNumbers( array[ i ], acMake, MetadataBufferSize, acModel, MetadataBufferSize, acSerialNumber, MetadataBufferSize,
                                        acLensMake, MetadataBufferSize, acLensModel, MetadataBufferSize, acLensSerialNumber, MetadataBufferSize );

        if ( ok && ModelInName( acModel, acCameraModel ) )
        {
            if ( verboseTracing )
            {
                lock_guard<mutex> lock( mtx );
                printf( "lens model information for %ws\n", array[ i ] );

                printf( "  lens make:     %s\n", acLensMake );
                printf( "  lens model:    %s\n", acLensModel );
            }

            if ( 0 != acLensModel[ 0 ] )
            {
                ModelEntry model( acLensMake, acLensModel );
                models.AddOrUpdate( model );
            }
        }
    }
    else if ( EnumAppMode::modeHasImage == appMode )
    {
        long long offset, length;
        int orientation, width, height, fullWidth, fullHeight;
        bool hasImage = id->FindEmbeddedImage( array[ i ], &offset, &length, &orientation, &width, &height, &fullWidth, &fullHeight );

        if ( hasImage )
            InterlockedIncrement( &hasImageCount );
    }
    else if ( EnumAppMode::modeHasGPS == appMode )
    {
        double lat, lon;
        bool hasGPS = id->GetGPSLocation( array[ i ], &lat, &lon );

        if ( hasGPS )
        {
            InterlockedIncrement( &hasGPSCount );
        
            if ( verboseTracing )
            {
                lock_guard<mutex> lock( mtx );

                printf( "file with GPS: %ws\n", array[ i ] );
                printf( "    https://www.google.com/maps/search/?api=1&query=%lf,%lf\n", lat, lon );
            }
        }
    }
    else if ( EnumAppMode::modeEmbedded == appMode )
    {
        long long offset, length;
        int orientation, width, height, fullWidth, fullHeight;
        bool hasImage = id->FindEmbeddedImage( array[ i ], &offset, &length, &orientation, &width, &height, &fullWidth, &fullHeight );

        if ( hasImage )
        {
            InterlockedIncrement( &hasImageCount );

            if ( verboseTracing )
            {
                lock_guard<mutex> lock( mtx );
                printf( "has image, offset %I64d, length %I64d\n", offset, length );
            }

            CStream stream( array[ i ], offset, length );
            if ( stream.Ok() )
            {
                vector<byte> vImage( length );
                stream.Read( vImage.data(), length );

                char acSha256[ 65 ];
                acSha256[ 64 ] = 0;

                CSha256 sha;
                bool ok = sha.Hash( vImage.data(), length, acSha256 );

                if ( ok )
                {
                    if ( verboseTracing )
                    {
                        lock_guard<mutex> lock( mtx );

                        printf( "added entry for %s, %ws\n", acSha256, array[ i ] );
                    }

                    EmbeddedImageEntry entry( acSha256, offset, length, array[ i ] );
                    embeddedImages.AddOrUpdate( entry );
                }
            }
            else
                printf( "can't open stream %ws\n", array[ i ] );
        }
    }
} //ProcessFile

const WCHAR * MusicExtensions[] =
{
    L"flac",
    L"mp3",
};

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
#if false
    int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    tmpFlag |= ( _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF );
    _CrtSetDbgFlag( tmpFlag );
#endif

    Scoped_SE_Translator scoped_se_translator{ SE_trans_func };

    if ( argc < 2 )
        Usage();

    tracer.Enable( false, L"aid.txt", true );

    std::mutex mtx;
    bool verboseTracing = false;
    EnumAppMode appMode = EnumAppMode::modeSerialNumbers;

    static WCHAR awcFilename[ MAX_PATH + 1 ] = { 0 };
    static WCHAR awcRootPath[ MAX_PATH + 1 ] = { 0 };
    static WCHAR awcExtension[ MAX_PATH + 1 ] = { 0 };
    static char acCameraModel[ 100 ] = { 0 };
    const WCHAR * pwcRoot = 0;
    const WCHAR * pwcFile = 0;
    bool sortOnCount = false;
    bool createEmbeddedImages = false;
    bool oneThread = false;

    int iArg = 1;
    while ( iArg < argc )
    {
        const WCHAR * pwcArg = argv[iArg];
        WCHAR a0 = pwcArg[0];

        if ( ( L'-' == a0 ) || ( L'/' == a0 ) )
        {
           WCHAR a1 = towlower( pwcArg[1] );

           if ( L'a' == a1 )
           {
               if ( L':' != pwcArg[2] )
                   Usage();

               WCHAR mode = tolower( pwcArg[3] );

               if ( L's' == mode )
                   appMode = EnumAppMode::modeSerialNumbers;
               else if ( L'a' == mode )
                   appMode = EnumAppMode::modeAdobeEdits;
               else if ( L'e' == mode )
                   appMode = EnumAppMode::modeEmbedded;
               else if ( L'f' == mode )
                   appMode = EnumAppMode::modeFocalLengths;
               else if ( L'g' == mode )
                   appMode = EnumAppMode::modeHasGPS;
               else if ( L'i' == mode )
                   appMode = EnumAppMode::modeHasImage;
               else if ( L'm' == mode )
                   appMode = EnumAppMode::modeModels;
               else if ( L'l' == mode )
                   appMode = EnumAppMode::modeLenses;
               else if ( L'r' == mode )
                   appMode = EnumAppMode::modeRatings;
               else
                   Usage();
           }
           else if ( L'c' == a1 )
               createEmbeddedImages = true;
           else if ( L's' == a1 )
           {
               if ( L':' != pwcArg[2] )
                   Usage();

               WCHAR mode = tolower( pwcArg[3] );

               if ( L'c' == mode )
                   sortOnCount = true;
               else
                   Usage();
           }
           else if ( L'v' == a1 )
               verboseTracing = TRUE;
           else if ( L'o' == a1 )
               oneThread = TRUE;
           else if ( L'p' == a1 )
           {
               if ( ( 0 != awcRootPath[ 0 ] ) ||
                    ( ( L'\\' != pwcArg[2] ) && ( L':' != pwcArg[2] ) ) )
                   Usage();

               wcscpy( awcRootPath, pwcArg + 3 );
               pwcRoot = pwcArg + 3;
           }
           else if ( L'e' == a1 )
           {
               if ( 0 != awcExtension[0] )
                   Usage();

               WCHAR const * pwcExt = pwcArg + 3;

               while ( ( *pwcExt == '.' ) || ( *pwcExt == '*' ) )
                   pwcExt++;

               wcscpy( awcExtension, pwcExt );
           }
           else if ( L'm' == a1 )
           {
               if ( 0 != acCameraModel[0] )
                   Usage();

               size_t len;
               wcstombs_s( &len, acCameraModel, _countof( acCameraModel ), pwcArg + 3, _TRUNCATE ); 
               strlwr( acCameraModel );
           }
           else
           {
               printf( "unrecognized flag %c\n", a1 );
               Usage();
           }
        }
        else
        {
            if ( 0 != awcFilename[ 0 ] )
                Usage();

            pwcFile = pwcArg;
            wcscpy( awcFilename, pwcArg );
        }

       iArg++;
    }

    if ( 0 == awcExtension[0] )
        wcscpy( awcExtension, L"*" );

    if ( ( ( 0 == awcFilename[0] ) && ( 0 == awcRootPath[0] ) ) ||
         ( ( 0 != awcFilename[0] ) && ( 0 != awcRootPath[0] ) ) )
        Usage();

    if ( ( 0 != awcRootPath[0] ) && ( 0 == awcExtension[0] ) )
        Usage();

    if ( 0 != awcFilename[0] )
    {
        if ( wcschr( awcFilename, L'*' ) )
        {
            WCHAR const * pwcExt = awcFilename;
            while ( ( *pwcExt == '.' ) || ( *pwcExt == '*' ) )
                pwcExt++;

            wcscpy( awcExtension, pwcExt );
            awcFilename[0] = 0;
            _wfullpath( awcRootPath, L".\\", _countof( awcRootPath ) );
        }
        else
        {
            struct _stat64i32 s;
            if ( _wstat( awcFilename, &s ) == 0 )
            {
                if ( s.st_mode & S_IFDIR )
                {
                    wcscpy( awcRootPath, awcFilename );
                    awcFilename[0] = 0;
                }
                else
                {
                    _wfullpath( awcFilename, pwcFile, _countof( awcFilename ) );
                }
            }
        }
    }

    if ( 0 != pwcRoot )
       _wfullpath( awcRootPath, pwcRoot, _countof( awcRootPath ) );

    //printf( "awcFilename:  %ws\n", awcFilename );
    //printf( "awcRootPath:  %ws\n", awcRootPath );
    //printf( "awcExtension: %ws\n", awcExtension );

    LONG hasImageCount = 0;
    LONG hasGPSCount = 0;

    try
    {
        if ( 0 != awcFilename[0] )
        {
            CImageData id;
            char acModel[ MetadataBufferSize ] = { 0 };

            if ( EnumAppMode::modeAdobeEdits == appMode )
            {
                bool edits = id.HoldsAdobeEditsInXMP( awcFilename );

                printf( "holds adobe edits: %s\n", edits ? "yes" : "no" );
            }
            else if ( EnumAppMode::modeSerialNumbers == appMode )
            {
                char acMake[ MetadataBufferSize ] = { 0 };
                char acSerialNumber[ MetadataBufferSize ] = { 0 };
                char acLensMake[ MetadataBufferSize ] = { 0 };
                char acLensModel[ MetadataBufferSize ] = { 0 };
                char acLensSerialNumber[ MetadataBufferSize ] = { 0 };
    
                bool ok = id.GetSerialNumbers( awcFilename, acMake, MetadataBufferSize, acModel, MetadataBufferSize, acSerialNumber, MetadataBufferSize,
                                               acLensMake, MetadataBufferSize, acLensModel, MetadataBufferSize, acLensSerialNumber, MetadataBufferSize );
    
                if ( ok )
                {
                    printf( "make:          %s\n", acMake );
                    printf( "model:         %s\n", acModel );
                    printf( "serial #:      %s\n", acSerialNumber );
    
                    printf( "lens make:     %s\n", acLensMake );
                    printf( "lens model:    %s\n", acLensModel );
                    printf( "lens serial #: %s\n", acLensSerialNumber );
                }
                else
                    printf( "neither camera or lens serial number information found\n" );
            }
            else if ( EnumAppMode::modeFocalLengths == appMode )
            {
                double focalLengthLens, flGuess, flComputed;
                int flIn35mmFilm;
                char acModel[ 100 ] = { 0 };
    
                double flBestGuess = id.FindFocalLength( awcFilename, focalLengthLens, flIn35mmFilm, flGuess, flComputed, acModel, _countof( acModel ) );
    
                if ( 0.0 != flBestGuess )
                {
                    if ( 0 != flBestGuess )
                        printf( "equivalent focal length: %.1lf\n", flBestGuess );
    
                    if ( verboseTracing )
                    {
                        if ( 0.0 != focalLengthLens )
                            printf( "  focal length of lens: %.1lf\n", focalLengthLens );
                        if ( 0 != flIn35mmFilm )
                            printf( "  in 35mmfilm: %d\n", flIn35mmFilm );
                        if ( 0.0 != flGuess )
                            printf( "  approximate equivalent: %.1lf\n", flGuess );
                        if ( 0.0 != flComputed )
                            printf( "  computed based on sensor size: %.1lf\n", flComputed );
                    }
                }
                else
                    printf( "no focal length information found\n" );
            }
            else if ( EnumAppMode::modeRatings == appMode )
            {
                int rating;
    
                bool found = id.GetRating( awcFilename, rating );
    
                if ( found )
                    printf( "rating: %d\n", rating );
                else
                    printf( "no rating information found\n" );
            }
            else if ( EnumAppMode::modeModels == appMode )
            {
                char acMake[ MetadataBufferSize ] = { 0 };
                bool ok = id.GetCameraInfo( awcFilename, acMake, MetadataBufferSize, acModel, MetadataBufferSize );
    
                if ( ok && ( 0 != acMake[0] || 0 != acModel[ 0 ] ) )
                {
                    printf( "make:          %s\n", acMake );
                    printf( "model:         %s\n", acModel );
                }
                else
                    printf( "camera model unavailable\n" );
            }
            else if ( EnumAppMode::modeLenses == appMode )
            {
                char acMake[ MetadataBufferSize ] = { 0 };
                char acSerialNumber[ MetadataBufferSize ] = { 0 };
                char acLensMake[ MetadataBufferSize ] = { 0 };
                char acLensModel[ MetadataBufferSize ] = { 0 };
                char acLensSerialNumber[ MetadataBufferSize ] = { 0 };
    
                bool ok = id.GetSerialNumbers( awcFilename, acMake, MetadataBufferSize, acModel, MetadataBufferSize, acSerialNumber, MetadataBufferSize,
                                               acLensMake, MetadataBufferSize, acLensModel, MetadataBufferSize, acLensSerialNumber, MetadataBufferSize );
    
                if ( ok )
                {
                    printf( "make:          %s\n", acMake );
                    printf( "model:         %s\n", acModel );
                    printf( "serial #:      %s\n", acSerialNumber );
    
                    printf( "lens make:     %s\n", acLensMake );
                    printf( "lens model:    %s\n", acLensModel );
                    printf( "lens serial #: %s\n", acLensSerialNumber );
                }
                else
                    printf( "neither camera or lens serial number information found\n" );
    
                if ( ok && ( 0 != acLensMake[0] || 0 != acLensModel[ 0 ] ) )
                {
                    printf( "lens make:          %s\n", acLensMake );
                    printf( "lens model:         %s\n", acLensModel );
                }
                else
                    printf( "camera model unavailable\n" );
            }
            else if ( EnumAppMode::modeHasImage == appMode )
            {
                long long offset, length;
                int orientation, width, height, fullWidth, fullHeight;
                bool hasImage = id.FindEmbeddedImage( awcFilename, &offset, &length, &orientation, &width, &height, &fullWidth, &fullHeight );

                printf( "has an embedded image: %s\n", hasImage ? "yes" : "no" );

                if ( hasImage )
                {
                    printf( "offset:        %lld\n", offset );
                    printf( "length:        %lld\n", length );
                    printf( "orientation:   %d\n", orientation );
                    printf( "width:         %d\n", width );
                    printf( "height:        %d\n", height );
                    printf( "full width:    %d\n", fullWidth );
                    printf( "full height:   %d\n", fullHeight );
                }
            }
            else if ( EnumAppMode::modeHasGPS == appMode )
            {
                double lat, lon;
                bool hasGPS = id.GetGPSLocation( awcFilename, &lat, &lon );

                printf( "has gps coordinates: %s\n", hasGPS ? "yes" : "no" );

                if ( hasGPS )
                {
                    printf( "latitude:   %lf\n", lat );
                    printf( "longitude:  %lf\n", lon );
                    printf( "https://www.google.com/maps/search/?api=1&query=%lf,%lf\n", lat, lon );
                }
            }
            else if ( EnumAppMode::modeEmbedded == appMode )
            {
                long long offset, length;
                int orientation, width, height, fullWidth, fullHeight;
                bool hasImage = id.FindEmbeddedImage( awcFilename, &offset, &length, &orientation, &width, &height, &fullWidth, &fullHeight );

                printf( "has an embedded image: %s\n", hasImage ? "yes" : "no" );

                if ( hasImage )
                {
                    printf( "offset:        %lld\n", offset );
                    printf( "length:        %lld\n", length );
                    printf( "orientation:   %d\n", orientation );
                    printf( "width:         %d\n", width );
                    printf( "height:        %d\n", height );
                    printf( "full width:    %d\n", fullWidth );
                    printf( "full height:   %d\n", fullHeight );

                    CStream stream( awcFilename, offset, length );
                    if ( ! stream.Ok() )
                    {
                        printf( "can't open the stream %ws\n", awcFilename );
                    }
                    else
                    {
                        vector<byte> vImage( length );
                        stream.Read( vImage.data(), length );

                        char acSha256[ 65 ];
                        acSha256[ 64 ] = 0;

                        CSha256 sha;
                        bool ok = sha.Hash( vImage.data(), length, acSha256 );
                        printf( "sha256 ok: %d, value %s\n", ok, acSha256 );
                    }
                }
            }
        }
        else
        {
            AppendBackslashAndLowercase( awcRootPath );

            static WCHAR awcSpec[ MAX_PATH + 1 ];
            wcscpy( awcSpec, L"*." );
            wcscat( awcSpec, awcExtension );

            WCHAR ** pExtensions = NULL;
            int cExtensions = 0;

            if ( EnumAppMode::modeEmbedded == appMode )
            {
                pExtensions = (WCHAR **) MusicExtensions;
                cExtensions = _countof( MusicExtensions );
            }

            CStringArray array;
            CEnumFolder enumerate( true, &array, pExtensions, cExtensions );
            enumerate.Enumerate( awcRootPath, awcSpec );
            array.Sort();
            printf( "found %zd files\n\n", array.Count() );

            CEntryTracker<SerialNumberEntry> bodies;
            CEntryTracker<SerialNumberEntry> lenses;
            CEntryTracker<FocalLengthEntry> focalLengths;
            CEntryTracker<RatingEntry> ratings;
            CEntryTracker<ModelEntry> models;
            CEntryTracker<EmbeddedImageEntry> embeddedImages;
            LONG withAdobeEdits = 0;
            LONG withoutAdobeEdits = 0;

            // This is ugly, but I don't know how to tell ppl to use 1 thread in an elegant way

            if ( oneThread )
            {
                for ( int i = 0; i < array.Count(); i++ )
                    ProcessFile( appMode, verboseTracing, mtx, acCameraModel, hasImageCount, hasGPSCount, array, i, bodies, lenses,
                                 focalLengths, ratings, models, embeddedImages, withAdobeEdits, withoutAdobeEdits );
            }
            else
            {
                parallel_for ( 0, (int) array.Count(), [&] ( int i  )
                {
                    ProcessFile( appMode, verboseTracing, mtx, acCameraModel, hasImageCount, hasGPSCount, array, i, bodies, lenses,
                                 focalLengths, ratings, models, embeddedImages, withAdobeEdits, withoutAdobeEdits );
                }, static_partitioner() );
            }

            if ( EnumAppMode::modeAdobeEdits == appMode )
            {
                printf( "files with    adobe edits: %d\n", withAdobeEdits );
                printf( "files without adobe edits: %d\n", withoutAdobeEdits );
            }
            else if ( EnumAppMode::modeSerialNumbers == appMode )
            {
                bodies.PrintEntries( "bodies", sortOnCount );
    
                printf( "\n" );

                lenses.PrintEntries( "lenses", sortOnCount );
            }
            else if ( EnumAppMode::modeFocalLengths == appMode )
            {
                focalLengths.PrintEntries( "focal lengths", sortOnCount );
            }
            else if ( EnumAppMode::modeRatings == appMode )
            {
                ratings.PrintEntries( "ratings", sortOnCount );
            }
            else if ( EnumAppMode::modeModels == appMode )
            {
                models.PrintEntries( "models", sortOnCount );
            }
            else if ( EnumAppMode::modeLenses == appMode )
            {
                models.PrintEntries( "lenses", sortOnCount );
            }
            else if ( EnumAppMode::modeHasImage == appMode )
            {
                printf( "files with an image: %d\n", hasImageCount );
            }
            else if ( EnumAppMode::modeHasGPS == appMode )
            {
                printf( "files with GPS coordinates: %d\n", hasGPSCount );
            }
            else if ( EnumAppMode::modeEmbedded == appMode )
            {
                embeddedImages.PrintEntries( "embedded images", sortOnCount );

                printf( "found %zd unique embedded images in %d files\n", embeddedImages.Count(), hasImageCount );

                if ( createEmbeddedImages )
                    CreateEmbeddedImages( embeddedImages );
            }
        }
    }
    catch( const SE_Exception & e )
    {
        printf( "caught an SE exception\n" );
        printf( "  caught SE exceeption error %8.8x\n", e.getSeNumber() );
    }
    catch ( exception & e )
    {
        printf( "caught an exception\n" );
        printf( "  caught exception %s\n", e.what() );
    }
    catch( ... )
    {
        printf( "caught untyped exception\n" );
    }

    tracer.Shutdown();

    //printf( "clean exit\n" );
    return 0;
} //wmain


