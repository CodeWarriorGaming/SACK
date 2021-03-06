#ifndef NO_FILEOP_ALIAS
#  define NO_FILEOP_ALIAS
#endif

#include <stdhdrs.h>
#include <pssql.h>
#include <filesys.h>
#include <sack_vfs.h>
#include <filesys.h>

#include "../vfs_internal.h"  // for BLOCK_SIZE

// dereferencing NULL pointers; the function wouldn't be called with a NULL.
#pragma warning( disable:26451)

static struct vfs_command_local
{
	struct file_system_interface *fsi;
	struct sack_vfs_volume *current_vol;
	struct file_system_mounted_interface *current_mount;
	LOGICAL verbose;

	struct sack_vfs_volume *current_vol_source;
	struct file_system_mounted_interface *current_mount_source;
} l;

#ifdef USE_VFS_OS_INTERFACE
const char *testDatas[] = {
	"Test File Data...."
};
static void testVolume_os( void ) {
	CTEXTSTR result;
	size_t resultLen;
	int n;
	for( n = 0; n < 256; n++ ) {
		sack_vfs_os_ioctl_store_rw_object( l.current_mount, testDatas[0], strlen( testDatas[0] ), &result, &resultLen );
		printf( "Received identifier? %s\n", result );
	}
}
#endif

static void testVolume( void ) {
	struct sack_vfs_file *db;
	struct sack_vfs_file *dbj;
	uint16_t buffer[2048];
	int n;
	int nj;
	db = sack_vfs_openfile( l.current_vol, "test.db" );
	if( !db ) {
		printf( " Failed to open test db in current vfs.\n" );
		return;
	}
	for( n = 0; n < 10000; n++ ) {
		int b;

		for( b = 0; b < 2048; b++ ) buffer[b] = (n & 0xFFFF);
		sack_vfs_write( db, buffer, 4096 );

		dbj = sack_vfs_openfile( l.current_vol, "test.db-journal" );
		for( nj = 0; nj < 5; nj++ ) {
			for( b = 0; b < 2048; b++ ) buffer[b] = (nj & 0xFFFF);
			sack_vfs_write( dbj, buffer, 4096 );
		}

		sack_vfs_close( dbj );
		sack_vfs_unlink_file( l.current_vol, "test.db-journal" );

		if( ( n % 1000 ) == 999 )
		{
			int check;
			for( check = 0; check < n; check++ ) {
				uint16_t val;
				sack_vfs_seek( db, check * 4096, SEEK_SET );
				sack_vfs_read( db, &val, 2 );
				if( val != (check & 0xFFFF) ) {
					printf( "BREAK" );
				}
			}
			sack_vfs_seek( db, (check + 1) * 4096, SEEK_SET );
		}
	}

	l.current_vol;
}

static void testVolume_alt( void ) {
	struct sack_vfs_file *db;
	struct sack_vfs_file *dbj;
	uint16_t buffer[2048];
	int n;
	int nj;
	int _n = 0;
	db = sack_vfs_openfile( l.current_vol, "test.db" );
	if( !db ) {
		printf( " Failed to open test db in current vfs.\n" );
		return;
	}
	for( n = 0; n < 500; ) {
		int b;
		//do {
		if( !n ) {
			for( nj = 0; nj < 512; nj++ ) {
				for( b = 0; b < 2048; b++ ) buffer[b] = (n & 0xFFFF);
				sack_vfs_write( db, buffer, 4096 );
				n++;
			}
		}
			//n++;
		//} while( (n % 2) );
		dbj = sack_vfs_openfile( l.current_vol, "test.db-journal" );
		for( nj = 0; nj < 1; nj++ ) {
			for( b = 0; b < 2048; b++ ) buffer[b] = (nj & 0xFFFF);
			sack_vfs_write( dbj, buffer, 4096 );
			//n++;
			for( b = 0; b < 2048; b++ ) buffer[b] = (n & 0xFFFF);
			sack_vfs_write( db, buffer, 4096 );
			n++;
		}

		sack_vfs_close( dbj );
		sack_vfs_unlink_file( l.current_vol, "test.db-journal" );

		/*
		for( b = 0; b < 2048; b++ ) buffer[b] = (n & 0xFFFF);
		sack_vfs_write( db, buffer, 4096 );
		n++;
		*/
		if( (n - _n ) > 100 ) {
			int check;
			_n = n;
			for( check = 0; check < n; check++ ) {
				uint16_t val;
				sack_vfs_seek( db, check * 4096, SEEK_SET );
				sack_vfs_read( db, &val, 2 );
				if( val != (check & 0xFFFF) ) {
					printf( "BREAK" );
				}
			}
			sack_vfs_seek( db, (check) * 4096, SEEK_SET );
		}
	}

	l.current_vol;
}

static void testVolume_slow( void ) {
	FILE *db;
	FILE *dbj;
	uint16_t buffer[2048];
	int n;
	int nj;
	db = sack_fopenEx( 0, "test.db", "wb", l.current_mount );
	if( !db ) {
		printf( " Failed to open test db in current vfs.\n" );
		return;
	}
	for( n = 0; n < 100000; n++ ) {
		int b;

		for( b = 0; b < 2048; b++ ) buffer[b] = (n & 0xFFFF);
		sack_fwrite( buffer, 2, 2048, db );

		dbj = sack_fopenEx( 0, "test.db-journal", "wb", l.current_mount );
		for( nj = 0; nj < 5; nj++ ) {
			for( b = 0; b < 2048; b++ ) buffer[b] = (nj & 0xFFFF);
			sack_fwrite( buffer, 2, 2048, dbj );
		}

		sack_fclose( dbj );
		sack_unlinkEx( 0, "test.db-journal", l.current_mount );

		{
			int check;
			for( check = 0; check < n; check++ ) {
				uint16_t val;
				sack_fseek( db, check * 4096, SEEK_SET );
				sack_fread( &val, 1, 2, db );
				if( val != (check & 0xFFFF) ) {
					printf( "BREAK" );
				}
			}
			sack_fseek( db, (check+1) * 4096, SEEK_SET );
		}
	}

	l.current_vol;
}

static void testVolume_db( void ) {
#if !defined( __NO_OPTIONS__ )
	//FILE* db;
	int n;
	PODBC odbc = ConnectToDatabase( "$sack@vfs$testsql.db" );
	SQLCommandf( odbc, "delete * from sqlite_master" );
	CheckODBCTable( odbc, GetFieldsInSQL( "create table test1 (a,b,c)", FALSE ), CTO_MERGE );
	CheckODBCTable( odbc, GetFieldsInSQL("create table test2 (a,b,c)", FALSE ), CTO_MERGE );

	for( n = 0; n < 100000; n++ ) {
		SQLCommandf( odbc, "insert into test1 (a,b,c) values(%d,%d,%d)", n, n * 2, n * 3 );
		SQLCommandf( odbc, "insert into test2 (a,b,c) values(%d,%d,%d)", n, n * 2, n * 3 );
		if( n % 3 )
			SQLCommandf( odbc, "delete from test2 " );

	}

	l.current_vol;
#endif
}



static void StoreFileAs( CTEXTSTR filename, CTEXTSTR asfile )
{
	FILE *in = sack_fopenEx( 0, filename, "rbn", sack_get_default_mount() );
	if( l.verbose ) printf( " Opened file %s = %p\n", filename, in );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, asfile, "wbn", l.current_mount );
		size_t size = sack_fsize( in );
		POINTER data = NewArray( uint8_t, size );
		if( l.verbose ) printf( " Opened file %s = %p\n", asfile, out );
		sack_fread( data, size, 1, in );
		if( l.verbose ) printf( " read %zd\n", size );
		sack_fwrite( data, size, 1, out );
		sack_fclose( in );
		sack_ftruncate( out );
		sack_fclose( out );
		Release( data );
	}
}

static void CPROC _StoreFile( uintptr_t psv,  CTEXTSTR filename, enum ScanFileProcessFlags flags )
{
	if( flags & SFF_DIRECTORY ) {// don't need to do anything with directories... already
      // doing subcurse option.
	} else {
		FILE *in = sack_fopenEx( 0, filename, "rbn", sack_get_default_mount() );
		if( l.verbose ) printf( " Opened file %s = %p\n", filename, in );
		if( in )
		{
			size_t size = sack_fsize( in );
			if( size == (size_t)-1 ) {
				sack_fclose( in );
				printf( "Failed to open file:%s\n", filename );
				return;
			}
			if( l.verbose ) printf( " file size (%zd)\n", size );
			{
				FILE *out = sack_fopenEx( 0, filename, "wbn", l.current_mount );
				POINTER data = NewArray( uint8_t, size );
				if( l.verbose ) printf( " Opened file %s = %p (%zd)\n", filename, out, size );
				sack_fread( data, size, 1, in );
				if( l.verbose ) printf( " read %zd\n", size );
				sack_fwrite( data, size, 1, out );
				sack_fclose( in );
				sack_ftruncate( out );
				sack_fclose( out );
				Release( data );
			}
		}else
			printf( " Failed to opened file %s\n", filename );
	}
}

static void CPROC _PatchFile( uintptr_t psv,  CTEXTSTR filename, enum ScanFileProcessFlags flags )
{
	if( flags & SFF_DIRECTORY ) {
		// don't need to do anything with directories... already
      // doing subcurse option.
	} else {
		FILE *in = sack_fopenEx( 0, filename, "rb", sack_get_default_mount() );
		FILE *in2 = sack_fopenEx( 0, filename, "rb", l.current_mount_source );
		if( l.verbose ) printf( " Opened file %s = %p\n", filename, in );
		if( in )
		{
			size_t size = sack_fsize( in );
			size_t size2 = in2?sack_fsize( in2 ):0;
			POINTER data = NewArray( uint8_t, size );
			sack_fread( data, size, 1, in );
			if( l.verbose ) printf( " file sizes (%zd) (%zd)\n", size, size2 );
			if( size == size2 )
			{
				POINTER data2 = NewArray( uint8_t, size2 );
				sack_fread( data2, size2, 1, in2 );
				if( l.verbose ) printf( "read %zd\n", size );
				if( memcmp( data, data2, size ) ) {
					size2 = -1;
					if( l.verbose ) printf( "data compared inequal; including in output\n" );
				}
				sack_fclose( in2 );
				Deallocate( POINTER, data2 );
			}
			if( ( size != size2 )
			   || (StrCaseCmp( filename, ".app.config" ) == 0)
			   || (StrCaseCmp( filename, "./.app.config" ) == 0) )
			{
				FILE *out = sack_fopenEx( 0, filename, "wb", l.current_mount );
				if( l.verbose ) printf( " Opened file %s = %p (%zd)\n", filename, out, size );
				sack_fwrite( data, size, 1, out );
				sack_ftruncate( out );
				sack_fclose( out );
			}
			sack_fclose( in );
			Release( data );
		}
	}
}

static void StoreFile( CTEXTSTR filemask )
{
	void *info = NULL;
	char * tmppath = strdup( filemask );
	char *end = (char*)pathrchr( tmppath );
	if( end ) {
		end[0] = 0; end++;
	} else {
		end = tmppath;
		tmppath = NULL;
	}
	while( ScanFilesEx( tmppath, end, &info, _StoreFile, SFF_DIRECTORIES|SFF_SUBCURSE|SFF_SUBPATHONLY, 0, FALSE, sack_get_default_mount() ) );
}

static int PatchFile( CTEXTSTR vfsName, CTEXTSTR filemask, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 )
{
	void *info = NULL;
	if( l.current_vol_source )
		sack_vfs_unload_volume( l.current_vol_source );
	if( key1 && key2 ) {
		l.current_vol_source = sack_vfs_load_crypt_volume( vfsName, version, key1, key2 );

	}else {
		l.current_vol_source = sack_vfs_load_volume( vfsName );
	}
	if( !l.current_vol_source )
	{
		printf( "Failed to load vfs: %s", vfsName );
		return 2;
	}
	l.current_mount_source = sack_mount_filesystem( "vfs2", l.fsi, 10, (uintptr_t)l.current_vol_source, 1 );

	while( ScanFilesEx( NULL, filemask, &info, _PatchFile, SFF_DIRECTORIES|SFF_SUBCURSE|SFF_SUBPATHONLY, 0, FALSE, sack_get_default_mount() ) );
	return 0;
}

/*
static void ExtractFile( CTEXTSTR filename )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", l.current_mount );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, filename, "wb", sack_get_default_mount() );
		size_t size = sack_fsize( in );
		POINTER data = NewArray( uint8_t, size );
		sack_fread( data, size, 1, in );
		sack_fwrite( data, size, 1, out );
		sack_fclose( in );
		sack_ftruncate( out );
		sack_fclose( out );
		Release( data );
	}
}
*/

static void CPROC _ExtractFile( uintptr_t psv, CTEXTSTR filename, enum ScanFileProcessFlags flags )
{
	if( flags & SFF_DIRECTORY ) {
		// don't need to do anything with directories... already
		// doing subcurse option.
	}
	else {
		FILE *in = sack_fopenEx( 0, filename, "rbn", l.current_mount );
		if( l.verbose ) printf( " Opened file %s = %p\n", filename, in );
		if( in )
		{
			size_t size = sack_fsize( in );
			CTEXTSTR filepart;
			if( l.verbose ) printf( " file size (%zd)\n", size );
			if( filepart = pathrchr( filename ) ) {
				TEXTSTR tmp = DupCStrLen( filename, filepart - filename );
				MakePath( tmp );
				Release( tmp );
			}
			{
				FILE *out = sack_fopenEx( 0, filename, "wbn", sack_get_default_mount() );
				if( out ) {
					POINTER data = NewArray( uint8_t, size );
					if( l.verbose ) printf( " Opened file %s = %p (%zd)\n", filename, out, size );
					sack_fread( data, size, 1, in );
					if( l.verbose ) printf( " read %zd\n", size );
					sack_fwrite( data, size, 1, out );
					sack_ftruncate( out );
					sack_fclose( out );
					Release( data );
				}
				sack_fclose( in );
			}
		}
	}
}

static void ExtractFile( CTEXTSTR filemask )
{
	void *info = NULL;
	while( ScanFilesEx( NULL, filemask, &info, _ExtractFile, SFF_DIRECTORIES | SFF_SUBCURSE | SFF_SUBPATHONLY, 0
				, FALSE, l.current_mount ) );
}


#define Seek(a,b) (((uintptr_t)a)+(b))

#ifdef WIN32

POINTER GetExtraData( POINTER block )
{
	//uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;

	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE ) {
			lprintf( "Basic signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE ) {
			lprintf( "Basic NT signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				lprintf( "Optional header signature is incorrect..." );
				return NULL;
			}
		}
		{
			int n;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			uintptr_t dwSize = 0;
			uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].PointerToRawData) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}
			dwSize += 0x1FFF;
			dwSize &= ~0xFFF;
			return (POINTER)Seek( source_memory, dwSize );
		}
	}
}

//#ifdef VIRUS_SCANNER_PARANOIA
void SetExtraData( POINTER block, size_t length )
{
	//uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;

	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE )
			//lprintf( "Basic signature check failed; not a library" );
			return;


		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE )
			//lprintf( "Basic NT signature check failed; not a library" );
			return;

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				//lprintf( "Optional header signature is incorrect..." );
				return;
			}
		}
		{
			int n;
			long FPISections = source_dos_header->e_lfanew
				+ sizeof( DWORD ) + sizeof( IMAGE_FILE_HEADER )
				+ source_nt_header->FileHeader.SizeOfOptionalHeader;
			PIMAGE_SECTION_HEADER source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			uintptr_t dwSize = 0;
			uintptr_t newSize;
			source_section = (PIMAGE_SECTION_HEADER)Seek( source_memory, FPISections );
			for( n = 0; n < source_nt_header->FileHeader.NumberOfSections; n++ )
			{
				newSize = (source_section[n].PointerToRawData) + source_section[n].SizeOfRawData;
				if( newSize > dwSize )
					dwSize = newSize;
			}
			dwSize += 0xFFF;
			dwSize &= ~0xFFF;
			//printf( "size is %d (%08x)\n", dwSize, dwSize );
			return;// (POINTER)Seek( source_memory, dwSize );
		}
	}
}
//#endif
#endif

static void AppendFilesAs( CTEXTSTR filename1, CTEXTSTR filename2, CTEXTSTR outputname )
{
	FILE *file1;
	size_t file1_size;
	FILE *file2;
	size_t file2_size;
	FILE *file_out;
	size_t file_out_size;

	POINTER buffer;

	file1 = sack_fopenEx( 0, filename1, "rb", sack_get_default_mount() );
	if( !file1 ) { printf( "Failed to read file to append: %s", filename1 ); return; }
	file1_size = sack_fsize( file1 );
	file2 = sack_fopenEx( 0, filename2, "rb", sack_get_default_mount() );
	if( !file2 ) { printf( "Failed to read file to append: %s", filename2 ); return; }
	file2_size = sack_fsize( file2 );
	file_out = sack_fopenEx( 0, outputname, "wb", sack_get_default_mount() );
	if( !file_out ) { printf( "Failed to read file to append to: %s", outputname ); return; }
	file_out_size = sack_fsize( file_out );

	buffer = NewArray( uint8_t, file1_size );
	sack_fread( buffer, file1_size, 1, file1 );
	sack_fwrite( buffer, file1_size, 1, file_out );
	{
#ifdef WIN32
		POINTER extra = GetExtraData( buffer );
#else
		POINTER extra = (POINTER)file1_size;
		lprintf( "linux append is probably wrong here..." );
#endif
		//printf( "output is... %zd %p %zd\n", file1_size, extra, (uintptr_t)extra - (uintptr_t)buffer );
		// there's probably a better expression...
		if( ((uintptr_t)extra - (uintptr_t)buffer) <= ( (file1_size + (2*BLOCK_SIZE-1) )& ~(BLOCK_SIZE-1) ) )
		{
			sack_fseek( file_out, ((uintptr_t)extra - (uintptr_t)buffer), SEEK_SET );
		}
		else {
			size_t fill = file1_size - ((uintptr_t)extra-(uintptr_t)buffer);
			size_t n;
			if( fill > 0 )
				for( n = 0; n < fill; n++ ) sack_fwrite( "", 1, 1, file_out );
		}

		sack_fseek( file_out, ((uintptr_t)extra - (uintptr_t)buffer)-BLOCK_SIZE, SEEK_SET );
		{
			const uint8_t *sig = sack_vfs_get_signature2( (POINTER)((uintptr_t)extra-BLOCK_SIZE), buffer );
			sack_fwrite( sig, 1, BLOCK_SIZE, file_out );
		}
		//lprintf( "Filesize raw is %d (padded %d)", file1_size, file1_size + (4096 - ( file1_size & 0xFFF )) );
		//lprintf( "extra offset is %d", (uintptr_t)extra - (uintptr_t)buffer );
	}
	//SetExtraData( buffer, file2_size );

	Release( buffer );

	buffer = NewArray( uint8_t, file2_size );
	sack_fread( buffer, file2_size, 1, file2 );
	sack_fwrite( buffer, file2_size, 1, file_out );

	sack_fclose( file1 );
	sack_fclose( file2 );
	sack_fclose( file_out );
}

static void ExtractFileAs( CTEXTSTR filename, CTEXTSTR asfile )
{
	FILE *in = sack_fopenEx( 0, filename, "rb", l.current_mount );
	if( in )
	{
		FILE *out = sack_fopenEx( 0, asfile, "wb", sack_get_default_mount() );
		size_t size = sack_fsize( in );
		POINTER data = NewArray( uint8_t, size );
		sack_fread( data, size, 1, in );
		sack_fwrite( data, size, 1, out );
		sack_fclose( in );
		sack_ftruncate( out );
		sack_fclose( out );
		Release( data );
	}
}

struct scanFileInfo {
	struct sack_vfs_volume * vol;
	POINTER *ppInfo;
};

static void CPROC ShowFile( uintptr_t psv, CTEXTSTR file, enum ScanFileProcessFlags flags )
{
	struct scanFileInfo *pInfo = (struct scanFileInfo*)psv;
	uint64_t ctime;
	uint64_t wtime;
	size_t ofs = 0;
	SACK_TIME ct, wt;
	struct find_cursor * cursor = GetScanFileCursor( pInfo->ppInfo[0] );
	ctime = l.fsi->find_get_ctime?l.fsi->find_get_ctime( cursor):0;
	wtime = l.fsi->find_get_wtime?l.fsi->find_get_wtime( cursor ):0;
	//if( !ctime )DebugBreak();
	ConvertTickToTime( ctime, &ct );
	ConvertTickToTime( wtime, &wt );
	if( file[0] == '.' && file[1] == '/' ) ofs = 2;
	size_t size = l.fsi->find_get_size( cursor );
	//printf( "%9zd %s %" PRId64 "  %" PRId64 "\n", l.fsi->size( f ), file, ctime, wtime );
	//if( !size ) DebugBreak();
	printf( "%9zd %s " "  %d-%02d-%02d %02d:%02d:%02d.%03d %d"  "  %d-%02d-%02d %02d:%02d:%02d.%03d %d"  "\n"
		, size, file
		, ct.yr, ct.mo, ct.dy, ct.hr, ct.mn, ct.sc, ct.ms, ct.zhr
		, wt.yr, wt.mo, wt.dy, wt.hr, wt.mn, wt.sc, wt.ms, wt.zhr
		);
}

static void GetDirectory( void )
{
	POINTER info = NULL;
	struct scanFileInfo sfi = { l.current_vol, &info };
	while( ScanFilesEx( NULL, "*", &info, ShowFile, SFF_SUBCURSE|SFF_SUBPATHONLY
	                  , (uintptr_t)&sfi, FALSE, l.current_mount ) );
	//l.fsi->
}

static void usage( void )
{
	printf( "arguments are processed in order... commands may be appended on the same line...\n" );
	printf( "   verbose                             : show operations; (some)debugging\n" );
	printf( "   vfs <filename>                      : specify a unencrypted VFS file to use.\n" );
	printf( "   cvfs <filename> <key1> <key2>       : specify an encrypted VFS file to use; and keys to use.\n" );
	printf( "   dir                                 : show current directory.\n" );
	printf( "   rm <filename>                       : delete file within VFS.\n" );
	printf( "   delete <filename>                   : delete file within VFS.\n" );
	printf( "   store <filemask>                    : store files that match the name in the VFS from local filesystem.\n" );
	printf( "   patch <vfsName> <filemask>          : store files that match the name and are different from those in the VFS.\n" );
	printf( "   cpatch <vfsName> <key1> <key2> <filemask> : store files that match the name and are different from those in the VFS.\n" );
	printf( "   extract <filemask>                  : extract files that match the name in the VFS to local filesystem.\n" );
	printf( "   storeas <filename> <as file>        : store file from <filename> into VFS as <as file>.\n" );
	printf( "   extractas <filename> <as file>      : extract file <filename> from VFS as <as file>.\n" );
	printf( "   append <file 1> <file 2> <to file>  : store <file 1>+<file 2> as <to file> in native file system.\n" );
	printf( "   shrink                              : remove extra space at the end of a volume.\n" );
	printf( "   encrypt <key1> <key2>               : apply encryption keys to vfs.\n" );
	printf( "   decrypt                             : remove encryption keys from vfs.\n" );
	printf( "   sign                                : get volume short signature.\n" );
	printf( "   sign-encrypt <key1>                 : get volume short signature; use key1 and signature to encrypt volume.\n" );
	printf( "   sign-to-header <filename> <varname> : get volume short signature; write a c header called filename, with a variable varname.\n" );
}

SaneWinMain( argc, argv )
{
	int arg;
	uintptr_t version = 0;
	SetSystemLog( SYSLOG_FILE, stdout );
	if( argc < 2 ) { usage(); return 0; }

	l.fsi = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME );
	if( !l.fsi ) {
		l.fsi = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME "-fs" );
		if( !l.fsi ) {
			l.fsi = sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME "-os" );
			if( !l.fsi ) {
				printf( "Failed to load file system interface.\n" );
				return 0;
			}
		}
	}
	for( arg = 1; arg < argc; arg++ )
	{
		if( StrCaseCmp( argv[arg], "dir" ) == 0 )
			GetDirectory();
		else if( StrCaseCmp( argv[arg], "verbose" ) == 0 )
		{
			l.verbose = TRUE;
			{
				int arg2;
				for( arg2 = 1; arg2 < argc; arg2++ )
					lprintf( "Arg %d = %s", arg2, argv[arg2] );
			}
		}
		else if( StrCaseCmp( argv[arg], "cvfs" ) == 0 )
		{
			if( l.current_vol )
				sack_vfs_unload_volume( l.current_vol );
			l.current_vol = sack_vfs_load_crypt_volume( argv[arg+1], version, argv[arg+2], argv[arg+3] );
			if( !l.current_vol )
			{
				printf( "Failed to load vfs: %s", argv[arg+1] );
				return 2;
			}
			l.current_mount = sack_mount_filesystem( "vfs", l.fsi, 10, (uintptr_t)l.current_vol, 1 );
			arg += 3;
		}
		else if( StrCaseCmp( argv[arg], "vfs" ) == 0 )
		{
			if( l.current_vol )
				sack_vfs_unload_volume( l.current_vol );
			l.current_vol = sack_vfs_load_volume( argv[arg+1] );
			if( !l.current_vol )
			{
				printf( "Failed to load vfs: %s", argv[arg+1] );
				return 2;
			}
			l.current_mount = sack_mount_filesystem( "vfs", l.fsi, 10, (uintptr_t)l.current_vol, 1 );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "rm" ) == 0
			|| StrCaseCmp( argv[arg], "delete" ) == 0 )
		{
			l.fsi->_unlink( (uintptr_t)l.current_vol, argv[arg+1] );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "store" ) == 0 )
		{
			StoreFile( argv[arg+1] );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "patch" ) == 0 )
		{
			if( (arg+2) <= argc )
				if( PatchFile( argv[arg+1], argv[arg+2], 0, NULL, NULL ) )
					return 2;
			arg+=2;
		}
		else if( StrCaseCmp( argv[arg], "cpatch" ) == 0 )
		{
			if( (arg+4) <= argc )
				if( PatchFile( argv[arg+1], argv[arg+4], version, argv[arg+2], argv[arg+3] ) )
					return 2;
			arg+=4;
		}
		else if( StrCaseCmp( argv[arg], "test" ) == 0 ) {
			arg++;
			switch( argv[arg][0] ) {
			default:
				arg--; // next argument isn't a test number; go back 1.
				/*fallthrough*/
			case '1':
				testVolume();
				break;
			case '2':
				testVolume_slow();
				break;
			case '3':
				testVolume_alt();
				break;
#ifdef USE_VFS_OS_INTERFACE
			case '4':
				testVolume_os();
				break;
#endif
			case '5':
				testVolume_db();
				break;
			}
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "extract" ) == 0 )
		{
			ExtractFile( argv[arg+1] );
			arg++;
		}
		else if( StrCaseCmp( argv[arg], "storeas" ) == 0 )
		{
			StoreFileAs( argv[arg+1], argv[arg+2] );
			arg += 2;
		}
		else if( StrCaseCmp( argv[arg], "extractas" ) == 0 )
		{
			ExtractFileAs( argv[arg+1], argv[arg+2] );
			arg += 2;
		}
		else if( StrCaseCmp( argv[arg], "append" ) == 0 )
		{
			AppendFilesAs( argv[arg+1], argv[arg+2], argv[arg+3] );
			arg += 3;
		}
		else if( StrCaseCmp( argv[arg], "shrink" ) == 0 )
		{
			sack_vfs_shrink_volume( l.current_vol );
			arg += 3;
		}
		else if( StrCaseCmp( argv[arg], "decrypt" ) == 0 )
		{
			if( !sack_vfs_decrypt_volume( l.current_vol ) )
				printf( "Failed to decrypt volume.\n" );
		}
		else if( StrCaseCmp( argv[arg], "encrypt" ) == 0 )
		{
			if( !sack_vfs_encrypt_volume( l.current_vol, version, argv[arg+1], argv[arg+2] ) )
				printf( "Failed to encrypt volume.\n" );
			arg += 2;
		}
		else if( StrCaseCmp( argv[arg], "sign" ) == 0 )
		{
			const char *signature = sack_vfs_get_signature( l.current_vol );
			printf( "%s\n", signature );
		}
		else if( StrCaseCmp( argv[arg], "version" ) == 0 ) {
			version = atoi( argv[arg + 1] );
			arg++;
		} else if( StrCaseCmp( argv[arg], "sign-encrypt" ) == 0 )
		{
			const char *signature = sack_vfs_get_signature( l.current_vol );
			if( !sack_vfs_encrypt_volume( l.current_vol, version, argv[arg+1], signature ) )
				printf( "Failed to encrypt volume.\n" );
			arg += 1;
		}
		else if( StrCaseCmp( argv[arg], "sign-to-header" ) == 0 )
		{
			const char *signature = sack_vfs_get_signature( l.current_vol );
			FILE *output = sack_fopenEx( 0, argv[arg+1], "wb", sack_get_default_mount() );
			if( !output )
			{
				printf( "Failed to open output header file: %s", argv[arg+1] );
				return 2;
			}
			sack_fprintf( output, "const char *%s = \"%s\";\n", argv[arg+2], signature );
			sack_fclose( output );
			arg += 2;
		}
	}
	if( l.current_vol )
		sack_vfs_unload_volume( l.current_vol );
	if( l.current_vol_source )
		sack_vfs_unload_volume( l.current_vol_source );
	return 0;
}
EndSaneWinMain()
