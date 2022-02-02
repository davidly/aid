#pragma once

#include <windows.h>
#include <stdio.h>
#include <bcrypt.h>

#define NT_SUCCESS( Status )   (((NTSTATUS)(Status)) >= 0)
#define NT_FAILED( Status )    (((NTSTATUS)(Status)) < 0)
#define STATUS_UNSUCCESSFUL    ((NTSTATUS)0xC0000001L)

class CSha256
{
    private:
        BCRYPT_ALG_HANDLE hAlg;
        BCRYPT_HASH_HANDLE hHash;
        DWORD cbData, cbHash, cbHashObject;
        PBYTE pbHashObject, pbHash;

        void CleanUp()
        {
            if ( 0 != hAlg )
            {
                BCryptCloseAlgorithmProvider( hAlg, 0 );
                hAlg = 0;
            }
        
            if ( 0 != hHash )    
            {
                BCryptDestroyHash( hHash );
                hHash = 0;
            }
        
            if ( 0 != pbHashObject )
            {
                HeapFree( GetProcessHeap(), 0, pbHashObject );
                pbHashObject = 0;
            }
        
            if ( 0 != pbHash )
            {
                HeapFree( GetProcessHeap(), 0, pbHash );
                pbHash = 0;
            }
        } //Cleanup

    public:
        CSha256() :
            hAlg( 0 ),
            hHash( 0 ),
            cbData( 0 ),
            cbHash( 0 ),
            cbHashObject( 0 ),
            pbHashObject( 0 ),
            pbHash( 0 )
        {
            NTSTATUS s = BCryptOpenAlgorithmProvider( &hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0 );
            if ( NT_FAILED( s ) )
            {
                printf( "can't open algorithm: %#x\n", s );
                return;
            }

            s = BCryptGetProperty( hAlg,  BCRYPT_OBJECT_LENGTH,  (PBYTE) &cbHashObject, sizeof DWORD, &cbData,  0 );
            if ( NT_FAILED( s ) )
            {
                printf( "can't crypt get property object length: %#x\n", s );
                CleanUp();
                return;
            }

            pbHashObject = (PBYTE) HeapAlloc( GetProcessHeap(), 0, cbHashObject );
            if ( 0 == pbHashObject)
            {
                printf( "can't allocate heap for hashobject\n" );
                CleanUp();
                return;
            }

            s = BCryptGetProperty( hAlg, BCRYPT_HASH_LENGTH, (PBYTE) &cbHash, sizeof DWORD, &cbData, 0 );
            if ( NT_FAILED( s ) )
            {
                printf( "can't crypt get property hash length: %#x\n", s );
                CleanUp();
                return;
            }

            pbHash = (PBYTE) HeapAlloc( GetProcessHeap (), 0, cbHash );
            if ( 0 == pbHash)
            {
                printf( "can't allocate heap for pbHash\n" );
                CleanUp();
                return;
            }
        } //CSha256

        ~CSha256()
        {
            CleanUp();
        }

        bool Hash( byte * pb, unsigned long long cb, char * pcHash )
        {
            // Sha256 is 256 bits, which is 32 bytes.

            byte buf[ 32 ];
            if ( !Hash( pb, cb, buf, sizeof buf ) )
                return false;

            // each byte is represented as 2 bytes in hex, so a 64 character string plus null termination

            pcHash[ 65 ] = 0;
            for ( int i = 0; i < 32; i++ )
            {
                sprintf( pcHash + ( 2 * i ), "%02x", (unsigned int) buf[ i ] );
            }

            return true;
        } //Hash

        bool Hash( byte * pb, unsigned long long cb, byte *pbOut, unsigned int cbOut )
        {
            // The hHash object must be recreated each time, since it's no longer valid after BCryptFinishHash

            if ( 0 != hHash )    
            {
                BCryptDestroyHash( hHash );
                hHash = 0;
            }
        
            NTSTATUS s = BCryptCreateHash( hAlg, &hHash, pbHashObject, cbHashObject, 0, 0, 0 );
            if ( NT_FAILED( s ) )
            {
                printf( "can't create hash: %#x\n", s );
                return false;
            }
    
            s = BCryptHashData( hHash, pb, cb, 0 );
            if ( NT_FAILED( s ) )
            {
                printf( "can't BCryptHashData: %#x\n", s );
                return false;
            }
    
            s = BCryptFinishHash( hHash, pbHash, cbHash, 0 );
            if ( NT_FAILED( s ) )
            {
                printf( "can't BCryptFinishHash: %#x\n", s );
                return false;
            }

            if ( cbOut < cbHash )
            {
                printf( "output buffer size %d isn't large enough for hash, size %d\n", cbOut, cbHash );
                return false;
            }

            memcpy( pbOut, pbHash, cbHash );
            return true;
        } //Hash
}; //CSha256

