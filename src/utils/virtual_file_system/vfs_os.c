#if !defined( SACK_AMALGAMATE ) || defined( __cplusplus )

/*

	FILE Data has extra fields stored with the data.
	
	   File Data - Directory entry filesize
	   references - a reference to a blockchain that contains the references to this object.
	        In the reference data block is FPI which is the directory entry ( converted directories? )  

	   Sealant - length stored in NAME_OFFSET field of directory entry
	   patches - a sealed object has the ability to be modified with other signed and sealed patches.
			 A reference to the patch FileData is stored for each patch object.
			 (The patch object has a unique object identifier?  Or does it only exist for this object?)


*/



/*
 BLOCKINDEX BAT[BLOCKS_PER_BAT] // link of next blocks; 0 if free, FFFFFFFF if end of file block
 uint8_t  block_data[BLOCKS_PER_BAT][BLOCK_SIZE];

 // (1+BLOCKS_PER_BAT) * BLOCK_SIZE total...
 BAT[0] = first directory cluster; array of struct directory_entry
 BAT[1] = name space; directory offsets land in a block referenced by this chain
 */
#define SACK_VFS_SOURCE
#define SACK_VFS_OS_SOURCE
//#define USE_STDIO
#if 1
#  include <stdhdrs.h>
#  include <ctype.h> // tolower on linux
#ifndef USE_STDIO
#  include <filesys.h>
#endif
#  include <sack_vfs.h>
#  include <procreg.h>
#  include <salty_generator.h>
#  include <sqlgetoption.h>
#  include <jsox_parser.h>
#else
#  include <sack.h>
#  include <ctype.h> // tolower on linux
//#include <filesys.h>
//#include <procreg.h>
//#include <salty_generator.h>
//#include <sack_vfs.h>
//#include <sqlgetoption.h>
#endif

// integer partial expresions summed into 64 bit.
#pragma warning( disable: 26451 )

#ifdef USE_STDIO
#define sack_fopen(a,b,c)     fopen(b,c)
#define sack_fseek(a,b,c)     fseek(a,(long)b,c)
#define sack_fclose(a)        fclose(a)
#define sack_fread(a,b,c,d)   fread(a,b,c,d)
#define sack_fwrite(a,b,c,d)  fwrite(a,b,c,d)
#define sack_ftell(a)         ftell(a)
#undef StrDup
#define StrDup(a)             strdup(a)
#define free(a)               Deallocate( POINTER, a )

#ifdef __cplusplus
namespace sack {
	namespace filesys {
#endif
		// pathops.c
		extern LOGICAL  CPROC  IsPath( CTEXTSTR path );
		extern  int CPROC  MakePath( CTEXTSTR path );
		extern CTEXTSTR CPROC pathrchr( CTEXTSTR path );
		extern CTEXTSTR CPROC pathchr( CTEXTSTR path );
#ifdef __cplusplus
	}
}
using namespace sack::filesys;
#endif

#else
#define free(a)               Deallocate( POINTER, a )

#endif

SACK_VFS_NAMESPACE

#define FILE_BASED_VFS
#define VIRTUAL_OBJECT_STORE
#include "vfs_internal.h"
#define vfs_SEEK vfs_os_SEEK
#define vfs_BSEEK vfs_os_BSEEK

struct memoryTimelineNode;

#ifdef __cplusplus
namespace objStore {
#endif
//#define PARANOID_INIT

//#define DEBUG_TRACE_LOG
//#define DEBUG_FILE_OPS
//#define DEBUG_DISK_IO
//#define DEBUG_DIRECTORIESa
//#define DEBUG_BLOCK_INIT
//#define DEBUG_TIMELINE_AVL
//#define DEBUG_TIMELINE_DIR_TRACKING
//#define DEBUG_FILE_SCAN
//#define DEBUG_FILE_OPEN

#ifdef DEBUG_TRACE_LOG
#define LoG( a,... ) lprintf( a,##__VA_ARGS__ )
#define LoG_( a,... ) _lprintf(DBG_RELAY)( a,##__VA_ARGS__ )
#else
#define LoG( a,... )
#define LoG_( a,... )
#endif
//#if !defined __GNUC__ and defined( __CPLUSPLUS )
#define sane_offsetof(type,member) ((size_t)&(((type*)0)->member))
//#endif



#define l vfs_os_local
static struct {
	struct directory_entry zero_entkey;
	uint8_t zerokey[BLOCK_SIZE];
	uint16_t index[256][256];
	char leadin[256];
	int leadinDepth;
	PLINKQUEUE plqCrypters;
} l;

#define EOFBLOCK  (~(BLOCKINDEX)0)
#define EOBBLOCK  ((BLOCKINDEX)1)
#define EODMARK   (1)
#undef GFB_INIT_NONE
#undef GFB_INIT_DIRENT
#undef GFB_INIT_NAMES

enum getFreeBlockInit {
	GFB_INIT_NONE       ,
	GFB_INIT_DIRENT     ,
	GFB_INIT_NAMES      ,
	GFB_INIT_PATCHBLOCK ,
	GFB_INIT_TIMELINE   ,
	GFB_INIT_TIMELINE_MORE,
};
// End Of Text Block
#define UTF8_EOTB 0xFF
// End Of Text
#define UTF8_EOT 0xFE

#define FIRST_DIR_BLOCK      0
//#define FIRST_NAMES_BLOCK    1
#define FIRST_TIMELINE_BLOCK 2


// use this character in hash as parent directory (block & char)
#define DIRNAME_CHAR_PARENT 0xFF

struct dirent_cache {
	BLOCKINDEX entry_fpi;
	struct directory_entry entry;  // has file size within
	struct directory_entry entry_key;  // has file size within

	struct dirent_cache *patches;
	int usedPatches;
	int availPatches;
} dirCache;


struct hashnode {
	char leadin[256];
	int leadinDepth;
	BLOCKINDEX this_dir_block;
	size_t thisent;
};

struct sack_vfs_os_find_info {
	char filename[BLOCK_SIZE];
	struct sack_vfs_os_volume *vol;
	CTEXTSTR base;
	size_t base_len;
	size_t filenamelen;
	size_t filesize;
	CTEXTSTR mask;
#ifdef VIRTUAL_OBJECT_STORE
	char leadin[256];
	int leadinDepth;
	PDATASTACK pds_directories;
	uint64_t ctime;
	uint64_t wtime;
#else
	BLOCKINDEX this_dir_block;
	size_t thisent;
#endif
};

static struct sack_vfs_os_file* _os_createFile( struct sack_vfs_os_volume* vol, BLOCKINDEX first_block );


static BLOCKINDEX _os_GetFreeBlock_( struct sack_vfs_os_volume *vol, enum getFreeBlockInit init DBG_PASS );
#define _os_GetFreeBlock(v,i) _os_GetFreeBlock_(v,i DBG_SRC )

LOGICAL _os_ScanDirectory_( struct sack_vfs_os_volume *vol, const char * filename
	, BLOCKINDEX dirBlockSeg
	, BLOCKINDEX *nameBlockStart
	, struct sack_vfs_os_file *file
	, int path_match
	, char *leadin
	, int *leadinDepth
);
#define _os_ScanDirectory(v,f,db,nb,file,pm) ((l.leadinDepth = 0), _os_ScanDirectory_(v,f,db,nb,file,pm, l.leadin, &l.leadinDepth ))

// This getNextBlock is optional allocate new one; it uses _os_getFreeBlock_
static BLOCKINDEX vfs_os_GetNextBlock( struct sack_vfs_os_volume *vol, BLOCKINDEX block, enum getFreeBlockInit init, LOGICAL expand );

static LOGICAL _os_ExpandVolume( struct sack_vfs_os_volume *vol );
static void reloadTimeEntry( struct memoryTimelineNode *time, struct sack_vfs_os_volume *vol, uint64_t timeEntry );

#define vfs_os_BSEEK(v,b,c) vfs_os_BSEEK_(v,b,c DBG_SRC )
uintptr_t vfs_os_BSEEK_( struct sack_vfs_os_volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index DBG_PASS );
uintptr_t vfs_os_DSEEK_( struct sack_vfs_os_volume* vol, FPI dataFPI, enum block_cache_entries* cache_index, POINTER* key DBG_PASS );
#define vfs_os_DSEEK(v,b,c,pp) vfs_os_DSEEK_(v,b,c,pp DBG_SRC )

uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume* vol
	, struct sack_vfs_os_file* file
	, BLOCKINDEX firstblock
	, FPI offset
	, enum block_cache_entries* cache_index );

static size_t CPROC sack_vfs_os_seek_internal( struct sack_vfs_os_file* file, size_t pos, int whence );
static size_t CPROC sack_vfs_os_write_internal( struct sack_vfs_os_file* file, const void* data_, size_t length
	, POINTER writeState );
static size_t CPROC sack_vfs_os_read_internal( struct sack_vfs_os_file* file, void* data_, size_t length );




PREFIX_PACKED struct directory_hash_lookup_block
{
	BLOCKINDEX next_block[256];
	struct directory_entry entries[VFS_DIRECTORY_ENTRIES];
	BLOCKINDEX names_first_block;
	uint8_t used_names;
} PACKED;


PREFIX_PACKED struct directory_patch_block
{
	union direction_patch_block_entry_union {
		struct direction_patch_block_entry {
			BIT_FIELD index : 8;
			BIT_FIELD hash_block : 24;
		} dirIndex;
		FPI raw;
	}entries[(BLOCK_SIZE-sizeof(BLOCKINDEX))/sizeof(uint32_t)];
	uint8_t usedEntries;
	BLOCKINDEX morePatches;
} PACKED;


PREFIX_PACKED struct directory_patch_ref_block
{
	PREFIX_PACKED struct directory_patch_ref_entry {
		BLOCKINDEX patchBlockStart;
		BLOCKINDEX dirBlock; // first patch block
		uint16_t patchNum;
		uint8_t dirEntry; // which directory entry this patches
	} entries[(BLOCK_SIZE)/sizeof( struct directory_patch_ref_entry )] PACKED;
} PACKED;


enum sack_vfs_os_seal_states {
	SACK_VFS_OS_SEAL_NONE = 0,
	SACK_VFS_OS_SEAL_LOAD,
	SACK_VFS_OS_SEAL_VALID,
	SACK_VFS_OS_SEAL_STORE,
	SACK_VFS_OS_SEAL_INVALID,  // validate failed (read whole file check)
	SACK_VFS_OS_SEAL_CLEARED,  // stored patch is writeable
	SACK_VFS_OS_SEAL_STORE_PATCH,  // stored patch new sealant (after read valid, new write)
};

struct file_block_definition {
	uint32_t avail;
	uint32_t used;
};
struct file_block_small_definition {
	uint16_t avail;
	uint16_t used;
};
struct file_block_large_definition {
	uint64_t avail;
	uint64_t used;
};

struct file_header {
	struct file_block_small_definition sealant;
	struct file_block_definition references;
	struct file_block_large_definition fileData;
	struct file_block_small_definition indexes;
	struct file_block_definition referencedBy;
};

static void flushFileSuffix( struct sack_vfs_os_file* file );
static void WriteIntoBlock( struct sack_vfs_os_file* file, int blockType, FPI pos, CPOINTER data, FPI length );


#include "vfs_os_timeline.c"


struct sack_vfs_os_file
{
	struct sack_vfs_os_volume* vol; // which volume this is in
	struct directory_entry dirent_key;
	FPI fpi;
	BLOCKINDEX _first_block;
	BLOCKINDEX block; // this should be in-sync with current FPI always; plz
	LOGICAL delete_on_close;  // someone already deleted this...
	BLOCKINDEX* blockChain;
	unsigned int blockChainAvail;
	unsigned int blockChainLength;

#  ifdef FILE_BASED_VFS
	FPI entry_fpi;  // where to write the directory entry update to
#    ifdef VIRTUAL_OBJECT_STORE
	struct file_header diskHeader; 
	struct file_header header;  // in-memory size, so we can just do generic move op

	struct memoryTimelineNode timeline;
	uint8_t* seal;
	uint8_t* sealant;
	uint8_t* readKey;
	uint16_t readKeyLen;
	//uint8_t sealantLen;
	uint8_t sealed; // boolean, on read, validates seal.  Defaults to FALSE.
	char* filename;
#    endif
	struct directory_entry _entry;  // has file size within
	struct directory_entry* entry;  // has file size within
	enum block_cache_entries cache; 
#  else
	struct directory_entry* entry;  // has file size within
#  endif

};

static void _os_UpdateFileBlocks( struct sack_vfs_os_file* file );

static uint32_t _os_AddSmallBlockUsage( struct file_block_small_definition* block, uint32_t more );


#include "vfs_os_index.c"

static void _os_SetSmallBlockUsage( struct file_block_small_definition* block, int more ) {
	block->used = more;
	while( block->avail < block->used )
		block->avail += 128;
}

 uint32_t _os_AddSmallBlockUsage( struct file_block_small_definition* block, uint32_t more ) {
	uint32_t oldval = block->used;
	_os_SetSmallBlockUsage( block, block->used + more );
	return oldval;
}

static void _os_SetFileBlockUsage( struct file_block_small_definition* block, uint32_t more ) {
	block->used = more;
	while( block->avail < block->used )
		block->avail += 256;
}

#define FILE_BLOCK_SEALANT 0
#define FILE_BLOCK_REFERENCES 1
#define FILE_BLOCK_DATA 2
#define FILE_BLOCK_INDEXES 3
#define FILE_BLOCK_REFERENCED_BY 4

static FPI GetBlockStart( struct sack_vfs_os_file* file, int blockType ) {
	FPI blockStart = sizeof( struct file_header );

	switch( blockType ) {
		//case 5:
		//	blockStart += file->header.fileData.avail; // end of file.
	case FILE_BLOCK_REFERENCED_BY:
		blockStart += file->header.indexes.avail;
	case FILE_BLOCK_INDEXES:
		blockStart += (FPI)file->header.fileData.avail;
	case FILE_BLOCK_DATA:
		blockStart += file->header.references.avail;
	case FILE_BLOCK_REFERENCES:
		blockStart += file->header.sealant.avail;
	case FILE_BLOCK_SEALANT:
		// starts at position 0.
		break;
	}
	return blockStart;
}
void WriteIntoBlock( struct sack_vfs_os_file* file, int blockType, FPI pos, CPOINTER data, FPI length ) {
	FPI blockStart = GetBlockStart( file, blockType );

	sack_vfs_os_seek_internal( file, (size_t)blockStart, SEEK_SET );
	sack_vfs_os_write_internal( file, data, (size_t)length, NULL );
}


static void _os_SetLargeBlockUsage( struct file_block_large_definition* block, uint64_t more ) {
	block->used = more;
	while( block->avail < block->used )
		block->avail = ( block->used + BLOCK_SIZE ) & BLOCK_MASK;
}

static void _os_UpdateFileBlocks( struct sack_vfs_os_file* file ) {
	FPI deltas[5]; // num header fields
	FPI oldStarts[6]; // num header fields
	FPI starts[6]; // num header fields
	oldStarts[0] = 0;
	starts[0] = 0;
	deltas[0] = ( file->header.sealant.avail ) - file->diskHeader.sealant.avail;
	starts[1] = starts[0] + file->header.sealant.avail;
	oldStarts[1] = oldStarts[0] + file->diskHeader.sealant.avail;

	deltas[1] = ( deltas[0] + file->header.references.avail ) - file->diskHeader.references.avail;
	starts[2] = starts[1] + file->header.references.avail;
	oldStarts[2] = oldStarts[1] + file->diskHeader.references.avail;

	deltas[2] = (FPI)(( deltas[1] + file->header.fileData.avail) - file->diskHeader.fileData.avail );
	starts[3] = (FPI)( starts[2] + file->header.fileData.avail );
	oldStarts[3] = (FPI)( oldStarts[2] + file->diskHeader.fileData.avail );

	deltas[3] = ( deltas[2] + file->header.indexes.avail) - file->diskHeader.indexes.avail;
	starts[4] = starts[3] + file->header.indexes.avail;
	oldStarts[4] = oldStarts[3] + file->diskHeader.indexes.avail;

	deltas[4] = ( deltas[3] + file->header.referencedBy.avail ) - file->diskHeader.referencedBy.avail;
	starts[5] = starts[4] + file->header.referencedBy.avail;
	oldStarts[5] = oldStarts[4] + file->diskHeader.referencedBy.avail;

	if( deltas[4] ) {
		static uint8_t block[BLOCK_SIZE];
		int n;
		for( n = 5; n > 0; n-- ) {
			size_t tailOffset = (size_t)starts[n];
			size_t oldTailOffset = (size_t)oldStarts[n];
			size_t dataLength = (size_t)(oldTailOffset - oldStarts[n-1]);

			// only going to copy for old length, so back up the difference
			// between new length and old length
			tailOffset -= (size_t)((starts[n] - starts[n-1] ) - dataLength);
			while( dataLength > 0 ) {
				if( dataLength > BLOCK_SIZE ) {
					oldTailOffset -= BLOCK_SIZE;
					tailOffset -= BLOCK_SIZE;
					sack_vfs_os_seek_internal( file, oldTailOffset, SEEK_SET );
					sack_vfs_os_read_internal( file, block, BLOCK_SIZE );
					sack_vfs_os_seek_internal( file, tailOffset, SEEK_SET );
					sack_vfs_os_write_internal( file, block, BLOCK_SIZE, NULL );
					dataLength -= BLOCK_SIZE;
					continue;
				}
				oldTailOffset -= dataLength;
				tailOffset -= dataLength;
				sack_vfs_os_seek_internal( file, oldTailOffset, SEEK_SET );
				sack_vfs_os_read_internal( file, block, dataLength );
				sack_vfs_os_seek_internal( file, tailOffset, SEEK_SET );
				sack_vfs_os_write_internal( file, block, dataLength, NULL );
				dataLength -= dataLength;
			}
		}
	}

	file->diskHeader = file->header;
}

static void _os_ExtendBlockChain( struct sack_vfs_os_file *file ) {
	int newSize = (file->blockChainAvail) * 2 + 1;
	file->blockChain = (BLOCKINDEX*)Reallocate( file->blockChain, newSize * sizeof( BLOCKINDEX ) );
#ifdef _DEBUG
	// debug
	memset( file->blockChain + file->blockChainAvail, 0, (newSize - file->blockChainAvail) * sizeof( BLOCKINDEX ) );
#endif
	file->blockChainAvail = newSize;

}

static void _os_SetBlockChain( struct sack_vfs_os_file *file, FPI fpi, BLOCKINDEX newBlock ) {
	FPI fileBlock = fpi >> BLOCK_SIZE_BITS;
#ifdef _DEBUG
	if( !newBlock ) DebugBreak();
#endif
	while( (fileBlock) >= file->blockChainAvail ) {
		_os_ExtendBlockChain( file );
	}
	if( fileBlock >= file->blockChainLength )
		file->blockChainLength = (unsigned int)(fileBlock + 1);
	//_lprintf(DBG_SRC)( "setting file at %d  to  %d to %d", (int)file->_first_block, (int)fileBlock, (int)newBlock );
	if( file->blockChain[fileBlock] ) {
		if( file->blockChain[fileBlock] == newBlock ) {
			return;
		}
		else {
			lprintf( "Re-setting chain to a new block... %d was %d and wants to be %d", (int)fpi, (int)file->blockChain[fileBlock], (int)newBlock );
			DebugBreak();
		}
	}
	file->blockChain[fileBlock] = newBlock;
}


// seek by byte position from a starting block; as file; result with an offset into a block.
uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume *vol
	, struct sack_vfs_os_file *file
	, BLOCKINDEX firstblock
	, FPI offset
	, enum block_cache_entries *cache_index ) 
{
	uint8_t *data;
	FPI pos = 0;
	if( file ) {
		if( (offset >> BLOCK_BYTE_SHIFT) < file->blockChainLength ) {
			firstblock = file->blockChain[offset >> BLOCK_BYTE_SHIFT];
			pos = offset & ~BLOCK_MASK;
			offset = offset & BLOCK_MASK;
		}
		else {
			if( file->blockChainLength ) {
				// go to the last known block
				firstblock = file->blockChain[pos = file->blockChainLength - 1];
				pos <<= BLOCK_BYTE_SHIFT; 
				offset -= pos;
			} 
			// else there is no known blocks... continue as usual
		}
	}
	while( firstblock != EOFBLOCK && offset >= BLOCK_SIZE ) {
		offset -= BLOCK_SIZE;
		pos += BLOCK_SIZE;
		//LoG( "Skipping a whole block of 'file' %d %d", firstblock, offset );
		firstblock = vfs_os_GetNextBlock( vol, firstblock, file?file->filename?GFB_INIT_NONE:GFB_INIT_TIMELINE_MORE:GFB_INIT_NAMES, 1 );
		if( file ) {
			//if( !file->filename ) LoG( "Set timeline block chain at %d to %d", (int)pos, (int)firstblock );
			_os_SetBlockChain( file, pos, firstblock );
		}
	}
	data = (uint8_t*)vfs_os_BSEEK_( vol, firstblock, cache_index DBG_NULL );
	return (uintptr_t)(data + (offset));
}

#define tolower_(c) (c)

static int  _os_PathCaseCmpEx ( CTEXTSTR s1, CTEXTSTR s2, size_t maxlen )
{
	if( !s1 )
		if( s2 )
			return -1;
		else
			return 0;
	else
		if( !s2 )
			return 1;
	if( s1 == s2 )
		return 0; // ==0 is success.
	for( ;s1[0] && ((unsigned char)s2[0] != UTF8_EOT) && (s1[0] == s2[0]) && maxlen;
		  s1++, s2++, maxlen-- );
	if( maxlen )
		return tolower_(s1[0]) - (((unsigned char)s2[0] == UTF8_EOT)?0:tolower_(s2[0]));
	return 0;
}

// read the byte from namespace at offset; decrypt byte in-register
// compare against the filename bytes.
static int _os_MaskStrCmp( struct sack_vfs_os_volume *vol, const char * filename, BLOCKINDEX nameBlock, FPI name_offset, int path_match ) {
	enum block_cache_entries cache = BC(NAMES);
	const char *dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset, &cache );
	const char *dirkey;
	const char *prior_dirname = dirname;
	if( !dirname ) return 1;
	dirkey = (const char*)(vol->usekey[cache]) + (name_offset & BLOCK_MASK );
	if( vol->key ) {
		int c;
		do {
			while( ((unsigned char)(c = (dirname[0] ^ dirkey[0])) != UTF8_EOT)
				&& ((((uintptr_t)prior_dirname) & BLOCK_MASK) == (((uintptr_t)dirname) & BLOCK_MASK))
				&& filename[0] ) {
				int del = tolower_( filename[0] ) - tolower_( c );
				if( del ) return del;
				filename++;
				dirname++;
				name_offset++;
				dirkey++;
				if( path_match && !filename[0] ) {
					c = (dirname[0] ^ dirkey[0]);
					if( c == '/' || c == '\\' ) return 0;
				}
			}
			if( ((((uintptr_t)prior_dirname) & BLOCK_MASK) != (((uintptr_t)dirname) & BLOCK_MASK)) ) {
				dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset, &cache );
				prior_dirname = dirname;
				continue;
			}
			// didn't stop because it exceeded a sector boundary
			break;
		} while( 1 );
		// c will be 0 or filename will be 0...
		if( path_match ) return 1;
		return filename[0] - c;
	} else {
		//LoG( "doesn't volume always have a key?" );
		if( path_match ) {
			size_t l;
			int r = _os_PathCaseCmpEx( filename, dirname, l = strlen( filename ) );
			if( !r )
				if( (dirname)[l] == '/' || (dirname)[l] == '\\' )
					return 0;
				else
					return 1;
			return r;
		}
		else
			return _os_PathCaseCmpEx( filename, dirname, strlen(filename) );
	}
}

#ifdef DEBUG_TRACE_LOG
static void _os_MaskStrCpy( char *output, size_t outlen, struct sack_vfs_os_volume *vol, FPI name_offset ) {
	if( vol->key ) {
		int c;
		FPI name_start = name_offset;
		while( UTF8_EOT != (unsigned char)( c = ( vol->usekey_buffer[BC(NAMES)][name_offset&BLOCK_MASK] ^ vol->usekey[BC(NAMES)][name_offset&BLOCK_MASK] ) ) ) {
			if( ( name_offset - name_start ) < outlen )
				output[name_offset-name_start] = c;
			name_offset++;
		}
		if( ( name_offset - name_start ) < outlen )
			output[name_offset-name_start] = 0;
		else
			output[outlen-1] = 0;
	} else {
		//LoG( "doesn't volume always have a key?" );
		StrCpyEx( output, (const char *)(vol->usekey[BC(NAMES)] + (name_offset & BLOCK_MASK )), outlen );
	}
}
#endif

#ifdef DEBUG_DIRECTORIES
static int _os_dumpDirectories( struct sack_vfs_os_volume *vol, BLOCKINDEX start, LOGICAL init ) {
	struct directory_hash_lookup_block *dirBlock;
	struct directory_hash_lookup_block *dirBlockKey;
	struct directory_entry *next_entries;
	static char leadin[256];
	static int leadinDepth;
	char outfilename[256];
	int outfilenamelen;
	size_t n;
	if( init )
		leadinDepth = 0;
	{
		enum block_cache_entries cache = BC( DIRECTORY );
		enum block_cache_entries name_cache = BC( NAMES );

		dirBlock = BTSEEK( struct directory_hash_lookup_block *, vol, start, cache );
		dirBlockKey = (struct directory_hash_lookup_block *)vol->usekey[cache];

		lprintf( "leadin : %*.*s %d %d", leadinDepth, leadinDepth, leadin, leadinDepth, dirBlock->used_names ^ dirBlockKey->used_names );
		next_entries = dirBlock->entries;

		for( n = 0; n < dirBlock->used_names ^ dirBlockKey->used_names; n++ ) {
			struct directory_entry *entkey = (vol->key) ? ((struct directory_hash_lookup_block *)vol->usekey[cache])->entries + n : &l.zero_entkey;
			FPI name_ofs = next_entries[n].name_offset ^ entkey->name_offset;
			const char *filename;
			int l;

			// if file is deleted; don't check it's name.
			if( (name_ofs) > vol->dwSize ) {
				LoG( "corrupted volume." );
				return 0;
			}

			name_cache = BC( NAMES );
			filename = (const char *)vfs_os_FSEEK( vol, NULL, dirBlock->names_first_block ^ dirBlockKey->names_first_block, name_ofs, &name_cache );
			if( !filename ) return;
			outfilenamelen = 0;
			for( l = 0; l < leadinDepth; l++ ) outfilename[outfilenamelen++] = leadin[l];

			if( vol->key ) {
				int c;
				while( (c = (((uint8_t*)filename)[0] ^ vol->usekey[name_cache][name_ofs&BLOCK_MASK])) ) {
					outfilename[outfilenamelen++] = c;
					filename++;
					name_ofs++;
				}
				outfilename[outfilenamelen] = c;
			}
			else {
				StrCpy( outfilename + outfilenamelen, filename );
			}
			//if( strlen( outfilename ) < 40 ) DebugBreak();
			lprintf( "%3d filename: %5d %s", n, name_ofs, outfilename );
		}

		for( n = 0; n < 255; n++ ) {
			BLOCKINDEX block = dirBlock->next_block[n] ^ dirBlockKey->next_block[n];
			if( block ) {
				lprintf( "Found directory with char '%c'", n );
				leadin[leadinDepth] = n;
				leadinDepth = leadinDepth + 1;
				_os_dumpDirectories( vol, block, 0 );
				leadinDepth = leadinDepth - 1;
			}
		}
	};
	return 0;
}
#endif

// this is about nLeast not being initialized.
// it will be set if it's used, if it's not 
// intiailzied, it won't be used.
#pragma warning( disable: 6001 ) 

#define _os_updateCacheAge(v,c,s,a,l) _os_updateCacheAge_(v,c,s,a,l DBG_SRC )
static void _os_updateCacheAge_( struct sack_vfs_os_volume *vol, enum block_cache_entries *cache_idx, BLOCKINDEX segment, uint8_t *age, int ageLength DBG_PASS ) {
	int n, m;
	int least;
	int nLeast;
	enum block_cache_entries cacheRoot = cache_idx[0];
	BLOCKINDEX *test_segment = vol->segment + cacheRoot;
	least = ageLength + 1;
#ifdef DEBUG_CACHE_AGING
	lprintf( "age start:" );
	LogBinary( age, ageLength );
	{
		int z;
		char buf[256];
		int ofs = 0;
		for( z = 0; z < ageLength; z++ ) ofs += snprintf( buf + ofs, 256 - ofs, "%x ", vol->bufferFPI[z+ cacheRoot] );
		lprintf( "%s", buf );
	}
#endif
	for( n = 0; n < (ageLength); n++,test_segment++ ) {
		if( test_segment[0] == segment ) {
			//if( pFile ) LoG_( "Cache found existing segment already. %d at %d(%d)", (int)segment, (cache_idx[0]+n), (int)n );
			cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + n);
			for( m = 0; m < (ageLength); m++ ) {
				if( !age[m] ) break;
				if( age[m] > age[n] )
					age[m]--;
			}
			age[n] = m;
#ifdef DEBUG_CACHE_AGING
			lprintf( "age end:" );
			LogBinary( age, ageLength );
#endif
			return;
			//break;
		}
		if( !age[n] ) { // end of list, empty entry.
			//LoG_( "Cache found unused segment already. %d at %d(%d)", (int)segment, (cache_idx[0] + n), (int)n );
			cache_idx[0] = (enum block_cache_entries)((cache_idx[0]) + n);
			for( m = 0; m < (ageLength); m++ ) { // age entries up to this one.
				if( !age[m] ) break;
				if( age[m] > ( n + 1 ) )
					age[m]--;
			}
			vol->segment[cache_idx[0]] = segment;
			age[n] = n + 1; // make this one newest
			break;
		}
		if( (age[n] < least) && !TESTFLAG( vol->seglock, cache_idx[0] + n) ) {
			least = age[n];
			nLeast = n; // this one will be oldest, unlocked candidate
		}
	}
	if( n == (ageLength) ) {
		int useCache = cacheRoot + nLeast;
#ifdef _DEBUG
		if( least > ageLength ) DebugBreak();
#endif
		for( n = 0; n < (ageLength); n++ ) {  // age evernthing.
			if( age[n] > least )
				age[n]--;
		}
		cache_idx[0] = (enum block_cache_entries)useCache;
		age[nLeast] = (ageLength); // make this one the newest, and return it.
		vol->segment[useCache] = segment;

		if( TESTFLAG( vol->dirty, useCache ) || TESTFLAG( vol->_dirty, useCache ) ) {
#ifdef DEBUG_DISK_IO
			LoG_( "MUST CLAIM SEGMENT Flush dirty segment: %d %x %d", nLeast, vol->bufferFPI[useCache], vol->segment[useCache] );
#endif
			uint8_t *crypt;
			size_t cryptlen;
			sack_fseek( vol->file, (size_t)vol->bufferFPI[useCache], SEEK_SET );
			if( vol->userkey ) {
				SRG_XSWS_encryptData( vol->usekey_buffer[useCache], BLOCK_SIZE
					, vol->segment[useCache], (const uint8_t*)vol->userkey, strlen( vol->userkey )
					, &crypt, &cryptlen );
				sack_fwrite( crypt, 1, BLOCK_SIZE, vol->file );
				Deallocate( uint8_t*, crypt );
			}else
				sack_fwrite( vol->usekey_buffer[useCache], 1, BLOCK_SIZE, vol->file );
			RESETFLAG( vol->dirty, useCache );
			RESETFLAG( vol->_dirty, useCache );
		}
	}
	{
		// read new buffer for new segment
		sack_fseek( vol->file, (size_t)(vol->bufferFPI[cache_idx[0]] = (size_t)((segment-1)*BLOCK_SIZE)), SEEK_SET );
#ifdef DEBUG_DISK_IO
		LoG_( "Read into block: fpi:%x cache:%d n:%d  seg:%d", (int)vol->bufferFPI[cache_idx[0]], (int)cache_idx[0] , (int)n, (int)segment );
#endif
		if( !sack_fread( vol->usekey_buffer[cache_idx[0]], 1, BLOCK_SIZE, vol->file ) )
			memset( vol->usekey_buffer[cache_idx[0]], 0, BLOCK_SIZE );
		else {
			if( vol->userkey )
				SRG_XSWS_decryptData( vol->usekey_buffer[cache_idx[0]], BLOCK_SIZE
					, vol->segment[cache_idx[0]], (const uint8_t*)vol->userkey, strlen( vol->userkey )
					, NULL, NULL );
		}
	}
#ifdef DEBUG_CACHE_AGING
	LoG( "age end2:" );
	LogBinary( age, ageLength );
#endif
}

#define _os_UpdateSegmentKey(v,c,s) _os_UpdateSegmentKey_(v,c,s DBG_SRC )

static enum block_cache_entries _os_UpdateSegmentKey_( struct sack_vfs_os_volume *vol, enum block_cache_entries cache_idx, BLOCKINDEX segment DBG_PASS )
{
	//LoG( "UPDATE OS SEGKEY %d %d", cache_idx, segment );
	if( cache_idx == BC(FILE) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->fileCacheAge, (BC(FILE_LAST) - BC(FILE)) DBG_RELAY );
	}
	else if( cache_idx == BC(NAMES) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->nameCacheAge, (BC(NAMES_LAST) - BC(NAMES)) DBG_RELAY );
	}
#ifdef VIRTUAL_OBJECT_STORE
	else if( cache_idx == BC(DIRECTORY) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->dirHashCacheAge, (BC(DIRECTORY_LAST) - BC(DIRECTORY)) DBG_RELAY );
	}
	else if( cache_idx == BC( TIMELINE ) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->timelineCacheAge, (BC( TIMELINE_LAST ) - BC( TIMELINE )) DBG_RELAY );
	}
	else if( cache_idx == BC( BAT ) ) {
		_os_updateCacheAge_( vol, &cache_idx, segment, vol->batHashCacheAge, (BC(BAT_LAST) - BC(BAT)) DBG_RELAY );
	}
#endif
	else {
		if( vol->segment[cache_idx] != segment ) {
			if( TESTFLAG( vol->dirty, cache_idx ) || TESTFLAG( vol->_dirty, cache_idx ) ) {
#ifdef DEBUG_DISK_IO
				LoG_( "MUST CLAIM SEGEMNT Flush dirty segment: %x %d", vol->bufferFPI[cache_idx], vol->segment[cache_idx] );
#endif
				sack_fseek( vol->file, (size_t)vol->bufferFPI[cache_idx], SEEK_SET );
				uint8_t *crypt;
				size_t cryptlen;
				if( vol->userkey ) {
					SRG_XSWS_encryptData( vol->usekey_buffer[cache_idx], BLOCK_SIZE
						, vol->segment[cache_idx], (const uint8_t*)vol->userkey, strlen( vol->userkey )
						, &crypt, &cryptlen );
					sack_fwrite( crypt, 1, BLOCK_SIZE, vol->file );
					Deallocate( uint8_t*, crypt );
				}
				else
					sack_fwrite( vol->usekey_buffer[cache_idx], 1, BLOCK_SIZE, vol->file );

				RESETFLAG( vol->dirty, cache_idx );
				RESETFLAG( vol->_dirty, cache_idx );
#ifdef DEBUG_DISK_IO
				LoG( "Flush dirty sector: %d", cache_idx, vol->bufferFPI[cache_idx]/BLOCK_SIZE );
#endif
			}

			// read new buffer for new segment
			sack_fseek( vol->file, (size_t)(vol->bufferFPI[cache_idx]=(segment - 1)*BLOCK_SIZE), SEEK_SET);
#ifdef DEBUG_DISK_IO
			LoG( "OS VFS read old sector: fpi:%d %d %d", (int)vol->bufferFPI[cache_idx], cache_idx, segment );
#endif
			if( !sack_fread( vol->usekey_buffer[cache_idx], 1, BLOCK_SIZE, vol->file ) ) {
				//lprintf( "Cleared BLock on failed read." );
				memset( vol->usekey_buffer[cache_idx], 0, BLOCK_SIZE );
			}
			else {
				if( vol->userkey )
					SRG_XSWS_decryptData( vol->usekey_buffer[cache_idx], BLOCK_SIZE
						, vol->segment[cache_idx], (const uint8_t*)vol->userkey, strlen( vol->userkey )
						, NULL, NULL );
			}
		}
		vol->segment[cache_idx] = segment;
	}

	if( !vol->key ) {
		//lprintf( "Resulting stored segment in %d", cache_idx );
		return cache_idx;
	}
	while( ((segment)*BLOCK_SIZE) > vol->dwSize ) {
		if( !_os_ExpandVolume( vol ) ) {
			DebugBreak();
		}
	}
	//vol->segment[cache_idx] = segment;
	if( vol->segment[cache_idx] == vol->_segment[cache_idx] ) {
		//lprintf( "Resulting stored segment in %d", cache_idx );
		return cache_idx;
	}
	SRG_ResetEntropy( vol->entropy );
	vol->_segment[cache_idx] = vol->segment[cache_idx];
	vol->curseg = cache_idx;  // so we know which 'segment[idx]' to use.
	SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->segkey, SHORTKEY_LENGTH * 8 );
	{
		int n;
#ifdef __64__
		uint64_t* usekey = (uint64_t*)vol->usekey[cache_idx];
		uint64_t* volkey = (uint64_t*)vol->key;
		uint64_t* segkey = (uint64_t*)vol->segkey;
		for( n = 0; n < (BLOCK_SIZE / SHORTKEY_LENGTH); n++ ) {
			usekey[0] = volkey[0] ^ (segkey[0]);
			usekey[1] = volkey[1] ^ (segkey[1]);
			usekey += 2;
			volkey += 2;
		}
#else
		uint32_t* usekey = (uint32_t*)vol->usekey[cache_idx];
		uint32_t* volkey = (uint32_t*)vol->key;
		uint32_t* segkey = (uint32_t*)vol->segkey;
		for( n = 0; n < (BLOCK_SIZE / SHORTKEY_LENGTH); n++ ) {
			usekey[0] = volkey[0] ^ (segkey[0]);
			usekey[1] = volkey[1] ^ (segkey[1]);
			usekey[2] = volkey[2] ^ (segkey[2]);
			usekey[3] = volkey[3] ^ (segkey[3]);
			usekey += 4;
			volkey += 4;
		}
#endif
	}
	//LoG( "Resulting stored segment in %d", cache_idx );
	return cache_idx;
}

#pragma warning( default: 6001 ) 

//---------------------------------------------------------------------------


struct sack_vfs_os_file * _os_createFile( struct sack_vfs_os_volume *vol, BLOCKINDEX first_block )
{
	struct sack_vfs_os_file * file = New( struct sack_vfs_os_file );
	MemSet( file, 0, sizeof( struct sack_vfs_os_file ) );
	_os_SetBlockChain( file, 0, first_block );
	file->_first_block = first_block;
	file->block = first_block;
	file->vol = vol;
	return file;
}


//---------------------------------------------------------------------------


static LOGICAL _os_ValidateBAT( struct sack_vfs_os_volume *vol ) {
	BLOCKINDEX first_slab = 0;
	BLOCKINDEX slab = vol->dwSize / ( BLOCK_SIZE );
	BLOCKINDEX last_block = ( slab * BLOCKS_PER_BAT ) / BLOCKS_PER_SECTOR;
	BLOCKINDEX n;
	enum block_cache_entries cache = BC(BAT);
	if( vol->key ) {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT;
			BLOCKINDEX *blockKey;
			BAT = TSEEK( BLOCKINDEX*, vol, n * BLOCK_SIZE, cache );
			//sack_fseek( vol->file, (size_t)n * BLOCK_SIZE, SEEK_SET );
			//if( !sack_fread( vol->usekey_buffer[BC(BAT)], 1, BLOCK_SIZE, vol->file ) ) return FALSE;
			//BAT = (BLOCKINDEX*)vol->usekey_buffer[BC(BAT)];
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
			_os_UpdateSegmentKey( vol, cache, n + 1 );

			for( m = 0; m < BLOCKS_PER_BAT; m++ )
			{
				BLOCKINDEX block = BAT[0] ^ blockKey[0];
				BAT++; blockKey++;
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) {
					vol->lastBatBlock = n + m;
					break;
				}
				if( block >= last_block ) return FALSE;
				if( block == 0 ) {
					vol->lastBatBlock = n + m; // use as a temp variable....
					AddDataItem( &vol->pdlFreeBlocks, &vol->lastBatBlock );
				}
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	} else {
		for( n = first_slab; n < slab; n += BLOCKS_PER_SECTOR  ) {
			size_t m;
			BLOCKINDEX *BAT = TSEEK( BLOCKINDEX*, vol, n * BLOCK_SIZE, cache );
			//BAT = (BLOCKINDEX*)vol->usekey_buffer[cache];
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				BLOCKINDEX block = BAT[m];
				if( block == EOFBLOCK ) continue;
				if( block == EOBBLOCK ) {
					vol->lastBatBlock = n + m;
					break;
				}
				if( block >= last_block ) return FALSE;
				if( block == 0 ) {
					vol->lastBatBlock = n + m; // use as a temp variable....
					AddDataItem( &vol->pdlFreeBlocks, &vol->lastBatBlock );
				}
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	}


	vol->timeline_cache = New( struct storageTimelineCursor );
	vol->timeline_cache->parentNodes = CreateDataStack( sizeof( struct storageTimelineNode ) );
	vol->timeline_file = _os_createFile( vol, FIRST_TIMELINE_BLOCK );
	{
		enum block_cache_entries cache = BC( TIMELINE );
		vol->timeline = (struct storageTimeline *)vfs_os_BSEEK( vol, FIRST_TIMELINE_BLOCK, &cache );
		SETFLAG( vol->seglock, cache );
		vol->timelineKey = (struct storageTimeline *)(vol->usekey[cache]);
	}

	if( !_os_ScanDirectory( vol, NULL, FIRST_DIR_BLOCK, NULL, NULL, 0 ) ) return FALSE;

	//DumpTimelineTree( vol, 0 );
	//DumpTimelineTree( vol, 1 );

	return TRUE;
}

//-------------------------------------------------------
// function to process a currently loaded program to get the
// data offset at the end of the executable.


static POINTER _os_GetExtraData( POINTER block )
{
#ifdef WIN32
#  define Seek(a,b) (((uintptr_t)a)+(b))
	//uintptr_t source_memory_length = block_len;
	POINTER source_memory = block;

	{
		PIMAGE_DOS_HEADER source_dos_header = (PIMAGE_DOS_HEADER)source_memory;
		PIMAGE_NT_HEADERS source_nt_header = (PIMAGE_NT_HEADERS)Seek( source_memory, source_dos_header->e_lfanew );
		if( source_dos_header->e_magic != IMAGE_DOS_SIGNATURE ) {
			LoG( "Basic signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->Signature != IMAGE_NT_SIGNATURE ) {
			LoG( "Basic NT signature check failed; not a library" );
			return NULL;
		}

		if( source_nt_header->FileHeader.SizeOfOptionalHeader )
		{
			if( source_nt_header->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC )
			{
				LoG( "Optional header signature is incorrect..." );
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
			dwSize += (BLOCK_SIZE*2)-1; // pad 1 full block, plus all but 1 byte of a full block(round up)
			dwSize &= ~(BLOCK_SIZE-1); // mask off the low bits; floor result to block boundary
			return (POINTER)Seek( source_memory, dwSize );
		}
	}
#  undef Seek
#else
	// need to get elf size...
	return 0;
#endif
}

static void _os_AddSalt2( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	struct datatype { void* start; size_t length; } *data = (struct datatype*)psv;
	
	(*salt_size) = data->length;
	(*salt) = (POINTER)data->start;
	// only need to make one pass of it....
	data->length = 0;
	data->start = NULL;
}
const uint8_t *sack_vfs_os_get_signature2( POINTER disk, POINTER diskReal ) {
	if( disk != diskReal ) {
		static uint8_t usekey[BLOCK_SIZE];
		static struct random_context *entropy;
		static struct datatype { void* start; size_t length; } data;
		data.start = diskReal;
		data.length = ((uintptr_t)disk - (uintptr_t)diskReal) - BLOCK_SIZE;
		if( !entropy ) entropy = SRG_CreateEntropy2( _os_AddSalt2, (uintptr_t)&data );
		SRG_ResetEntropy( entropy );
		SRG_GetEntropyBuffer( entropy, (uint32_t*)usekey, BLOCK_SIZE*CHAR_BIT );
		return usekey;
	}
	return NULL;
}


// add some space to the volume....
LOGICAL _os_ExpandVolume( struct sack_vfs_os_volume *vol ) {
	LOGICAL created = FALSE;
	LOGICAL path_checked = FALSE;
	size_t oldsize = vol->dwSize;
	if( vol->file && vol->read_only ) return TRUE;
	if( !vol->file ) {
		char *fname;
		char *iface;
		char *tmp;
		{
			char *tmp = StrDup( vol->volname );
			char *dir = (char*)pathrchr( tmp );
			if( dir ) {
				dir[0] = 0;
				if( !IsPath( tmp ) ) MakePath( tmp );
			}
			free( tmp );
			//Deallocate( char*, tmp );
		}
#ifndef USE_STDIO
		if( tmp =(char*)StrChr( vol->volname, '@' ) ) {
			tmp[0] = 0;
			iface = (char*)vol->volname;
			fname = tmp + 1;
			struct file_system_mounted_interface *mount = sack_get_mounted_filesystem( iface );
			//struct file_system_interface *iface = sack_get_filesystem_interface( iface );
			if( !sack_exists( fname ) ) {
				vol->file = sack_fopenEx( 0, fname, "rb+", mount );
				if( !vol->file )
					vol->file = sack_fopenEx( 0, fname, "wb+", mount );
				created = TRUE;
			}
			else
				vol->file = sack_fopenEx( 0, fname, "wb+", mount );
			tmp[0] = '@';
		}
		else {
			vol->file = sack_fopenEx( 0, vol->volname, "rb+", sack_get_default_mount() );
			if( !vol->file ) {
				created = TRUE;
				vol->file = sack_fopenEx( 0, vol->volname, "wb+", sack_get_default_mount() );
			}
		}
#else
		vol->file = sack_fopen( 0, vol->volname, "rb+" );
		if( !vol->file ) {
			created = TRUE;
			vol->file = sack_fopen( 0, vol->volname, "wb+" );
		}
#endif
		sack_fseek( vol->file, 0, SEEK_END );
		vol->dwSize = sack_ftell( vol->file );
		if( vol->dwSize == 0 )
			created = TRUE;
		sack_fseek( vol->file, 0, SEEK_SET );
	}

	//vol->dwSize += ((uintptr_t)vol->disk - (uintptr_t)vol->diskReal);
	// a BAT plus the sectors it references... ( BLOCKS_PER_BAT + 1 ) * BLOCK_SIZE
	vol->dwSize += BLOCKS_PER_SECTOR*BLOCK_SIZE;
	LoG( "created expanded volume: %p from %p size:%" _size_f, vol->file, BLOCKS_PER_SECTOR*BLOCK_SIZE, vol->dwSize );

	// can't recover dirents and nameents dynamically; so just assume
	// use the _os_GetFreeBlock because it will update encypted
	//vol->disk->BAT[0] = EOFBLOCK;  // allocate 1 directory entry block
	//vol->disk->BAT[1] = EOFBLOCK;  // allocate 1 name block
	if( created ) {
		_os_UpdateSegmentKey( vol, BC(BAT), 1 );
		((BLOCKINDEX*)vol->usekey_buffer[BC(BAT)])[0]
			= EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BC(BAT)])[0];
		SETFLAG( vol->dirty, BC(BAT) );
		vol->bufferFPI[BC( BAT )] = 0;
		{
			BLOCKINDEX dirblock = _os_GetFreeBlock( vol, GFB_INIT_DIRENT );
			BLOCKINDEX timeblock = _os_GetFreeBlock( vol, GFB_INIT_TIMELINE );
		}
	}

	return TRUE;
}


// shared with fuse module
// seek by byte position; result with an offset into a block.
// this is used to access BAT information; and should be otherwise avoided.
uintptr_t vfs_os_SEEK( struct sack_vfs_os_volume *vol, FPI offset, enum block_cache_entries *cache_index ) {
	while( offset >= vol->dwSize ) if( !_os_ExpandVolume( vol ) ) return 0;
	{
		BLOCKINDEX seg = (offset / BLOCK_SIZE) + 1;
		cache_index[0] = _os_UpdateSegmentKey( vol, cache_index[0], seg );
		//LoG( "RETURNING SEEK CACHED %p %d  0x%x   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)offset, (int)seg );
		return ((uintptr_t)vol->usekey_buffer[cache_index[0]]) + (offset&BLOCK_MASK);
	}
}

// shared with fuse module
// seek by block, outside of BAT.  block 0 = first block after first BAT.
uintptr_t vfs_os_BSEEK_( struct sack_vfs_os_volume *vol, BLOCKINDEX block, enum block_cache_entries *cache_index DBG_PASS ) {
	BLOCKINDEX b = ( 1 + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR) + ( block & (BLOCKS_PER_BAT-1) ) ) * BLOCK_SIZE;
	while( b >= vol->dwSize ) if( !_os_ExpandVolume( vol ) ) return 0;
	{
		BLOCKINDEX seg = ( b / BLOCK_SIZE ) + 1;
		cache_index[0] = _os_UpdateSegmentKey_( vol, cache_index[0], seg DBG_RELAY );
		//LoG( "RETURNING BSEEK CACHED %p  %d %d %d  0x%x  %d   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)(block>>BLOCK_SHIFT), (int)(BLOCKS_PER_BAT-1), (int)b, (int)block, (int)seg );
		return ((uintptr_t)vol->usekey_buffer[cache_index[0]])/* + (b&BLOCK_MASK) always 0 */;
	}
}

// shared with fuse module
// seek by block, outside of BAT.  block 0 = first block after first BAT.
uintptr_t vfs_os_DSEEK_( struct sack_vfs_os_volume *vol, FPI dataFPI, enum block_cache_entries *cache_index, POINTER*key DBG_PASS ) {
	BLOCKINDEX block = dataFPI / BLOCK_SIZE;
	size_t offset = dataFPI & BLOCK_MASK;
	BLOCKINDEX b = block * BLOCK_SIZE;
	while( b >= vol->dwSize ) if( !_os_ExpandVolume( vol ) ) return 0;
	{
		BLOCKINDEX seg = (b / BLOCK_SIZE) + 1;
		cache_index[0] = _os_UpdateSegmentKey_( vol, cache_index[0], seg DBG_RELAY );
		//LoG( "RETURNING BSEEK CACHED %p  %d %d %d  0x%x  %d   %d", vol->usekey_buffer[cache_index[0]], cache_index[0], (int)(block>>BLOCK_SHIFT), (int)(BLOCKS_PER_BAT-1), (int)b, (int)block, (int)seg );
		if( key )
			key[0] = vol->usekey[cache_index[0]] + offset;
		return ((uintptr_t)vol->usekey_buffer[cache_index[0]]) + offset;
	}
}

static BLOCKINDEX _os_GetFreeBlock_( struct sack_vfs_os_volume *vol, enum getFreeBlockInit init DBG_PASS )
{
	size_t n;
	unsigned int b = 0;
	enum block_cache_entries cache = BC( BAT );
	BLOCKINDEX *current_BAT;// = TSEEK( BLOCKINDEX*, vol, 0, cache );
	BLOCKINDEX *blockKey;
	BLOCKINDEX check_val;
	enum block_cache_entries newcache;

	if( vol->pdlFreeBlocks->Cnt ) {
		BLOCKINDEX newblock = ((BLOCKINDEX*)GetDataItem( &vol->pdlFreeBlocks, vol->pdlFreeBlocks->Cnt - 1 ))[0];
		check_val = 0;
		b = (unsigned int)(newblock / BLOCKS_PER_BAT);
		n = newblock % BLOCKS_PER_BAT;
		vol->pdlFreeBlocks->Cnt--;
	}
	else {
		check_val = EOBBLOCK;
		b = (unsigned int)(vol->lastBatBlock / BLOCKS_PER_BAT);
		n = vol->lastBatBlock % BLOCKS_PER_BAT;
	}
	//lprintf( "check, start, b, n %d %d %d %d", (int)check_val, (int) vol->lastBatBlock, (int)b, (int)n );
	current_BAT = TSEEK( BLOCKINDEX*, vol, b*BLOCKS_PER_SECTOR*BLOCK_SIZE, cache ) + n;
	blockKey = ((BLOCKINDEX*)vol->usekey[cache]) + n;

	if( !current_BAT ) return 0;

	current_BAT[0] = EOFBLOCK ^ blockKey[0];

	if( (check_val == EOBBLOCK) ) {
		if( n < (BLOCKS_PER_BAT - 1) ) {
			current_BAT[1] = EOBBLOCK ^ blockKey[1];
			vol->lastBatBlock++;
		}
		else {
			cache = BC( BAT );
			current_BAT = TSEEK( BLOCKINDEX*, vol, (b + 1) * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
			current_BAT[0] = EOBBLOCK ^ blockKey[0];
			vol->lastBatBlock = (b + 1) * BLOCKS_PER_BAT;
			//lprintf( "Set last block....%d", (int)vol->lastBatBlock );
		}
		SETFLAG( vol->dirty, cache );
	}

	switch( init ) {
	case GFB_INIT_DIRENT: {
			struct directory_hash_lookup_block *dir;
			struct directory_hash_lookup_block *dirkey;
#ifdef DEBUG_BLOCK_INIT
			LoG( "Create new directory: result %d", (int)(b * BLOCKS_PER_BAT + n) );
#endif
			newcache = _os_UpdateSegmentKey_( vol, BC( DIRECTORY ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			memset( vol->usekey_buffer[newcache], 0, BLOCK_SIZE );

			dir = (struct directory_hash_lookup_block *)vol->usekey_buffer[newcache];
			dirkey = (struct directory_hash_lookup_block *)vol->usekey[newcache];
			dir->names_first_block = _os_GetFreeBlock( vol, GFB_INIT_NAMES ) ^ dirkey->names_first_block;
			dir->used_names = 0 ^ dirkey->used_names;
			//((struct directory_hash_lookup_block*)(vol->usekey_buffer[newcache]))->entries[0].first_block = EODMARK ^ ((struct directory_hash_lookup_block*)vol->usekey[cache])->entries[0].first_block;
			break;
		}
	case GFB_INIT_TIMELINE: {
			struct storageTimeline *tl;
			struct storageTimeline *tlkey;
#ifdef DEBUG_BLOCK_INIT
			LoG( "new block, init as root timeline" );
#endif
			newcache = _os_UpdateSegmentKey_( vol, BC( TIMELINE ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			tl = (struct storageTimeline *)vol->usekey_buffer[newcache];
			tlkey = (struct storageTimeline *)vol->usekey[newcache];
			//tl->header.timeline_length  = 0 ^ tlkey->header.timeline_length;
			tl->header.crootNode.raw = 0 ^ tlkey->header.crootNode.raw;
			tl->header.srootNode.raw = 0 ^ tlkey->header.srootNode.raw;
			tl->header.first_free_entry.ref.index = 1 ^ tlkey->header.first_free_entry.ref.index;
			tl->header.first_free_entry.ref.depth = 0 ^ tlkey->header.first_free_entry.ref.depth;
			break;
		}
	case GFB_INIT_TIMELINE_MORE: {
#ifdef DEBUG_BLOCK_INIT
		LoG( "new block, init timeline more " );
#endif
		newcache = _os_UpdateSegmentKey_( vol, BC( TIMELINE ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
		memset( vol->usekey_buffer[newcache], 0, BLOCK_SIZE );
			break;
		}
	case GFB_INIT_NAMES: {
#ifdef DEBUG_BLOCK_INIT
		LoG( "new BLock, init names" );
#endif
		newcache = _os_UpdateSegmentKey_( vol, BC( NAMES ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			memset( vol->usekey_buffer[newcache], 0, BLOCK_SIZE );
			((char*)(vol->usekey_buffer[newcache]))[0] = (char)UTF8_EOTB ^ ((char*)vol->usekey[newcache])[0];
			//LoG( "New Name Buffer: %x %p", vol->segment[newcache], vol->usekey_buffer[newcache] );
			break;
		}
	default: {
#ifdef DEBUG_BLOCK_INIT
		LoG( "Default or NO init..." );
#endif
			//	memcpy( ((uint8_t*)vol->disk) + (vol->segment[newcache]-1) * BLOCK_SIZE, vol->usekey[newcache], BLOCK_SIZE );
			newcache = _os_UpdateSegmentKey_( vol, BC( FILE ), b * (BLOCKS_PER_SECTOR)+n + 1 + 1 DBG_RELAY );
			//memset( vol->usekey_buffer[newcache], 0, BLOCK_SIZE );
	}
	}
	SETFLAG( vol->dirty, newcache );
	//LoG( "Return Free block:%d   %d  %d", (int)(b*BLOCKS_PER_BAT + n), (int)b, (int)n );
	return b * BLOCKS_PER_BAT + n;
}

static BLOCKINDEX vfs_os_GetNextBlock( struct sack_vfs_os_volume *vol, BLOCKINDEX block, enum getFreeBlockInit init, LOGICAL expand ) {
	BLOCKINDEX sector = block / BLOCKS_PER_BAT;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX *, vol, sector * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
	BLOCKINDEX check_val;
	if( !this_BAT ) return 0; // if this passes, later ones will also.
#ifdef _DEBUG
	if( !block ) DebugBreak();
#endif

	check_val = (this_BAT[block & (BLOCKS_PER_BAT - 1)]) ^ ((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT-1)];
#ifdef _DEBUG
	if( !check_val ) {
		lprintf( "STOP: %p  %d  %d  %d", this_BAT, (int)check_val, (int)(block), (int)sector );
		DebugBreak();
	}
#endif
	if( check_val == EOBBLOCK ) {
		(this_BAT[block & (BLOCKS_PER_BAT-1)]) = EOFBLOCK^((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT-1)];
		if( block < (BLOCKS_PER_BAT - 1) )
			(this_BAT[1 + block & (BLOCKS_PER_BAT - 1)]) = EOBBLOCK ^ ((BLOCKINDEX*)vol->usekey[BC( BAT )])[1 + block & (BLOCKS_PER_BAT - 1)];
		//else
		//	lprintf( "THIS NEEDS A NEW BAT BLOCK TO MOVE THE MARKER" );//
	}
	if( check_val == EOFBLOCK || check_val == EOBBLOCK ) {
		if( expand ) {
			BLOCKINDEX key = vol->key?((BLOCKINDEX*)vol->usekey[cache])[block & (BLOCKS_PER_BAT-1)]:0;
			check_val = _os_GetFreeBlock( vol, init );
#ifdef _DEBUG
			if( !check_val )DebugBreak();
#endif
			// free block might have expanded...
			this_BAT = TSEEK( BLOCKINDEX*, vol, sector * ( BLOCKS_PER_SECTOR*BLOCK_SIZE ), cache );
			if( !this_BAT ) return 0;
			// segment could already be set from the _os_GetFreeBlock...
			this_BAT[block & (BLOCKS_PER_BAT-1)] = check_val ^ key;
			SETFLAG( vol->dirty, cache );
		}
	}
#ifdef _DEBUG
	if( !check_val )DebugBreak();
#endif
	//LoG( "return next block:%d %d", (int)block, (int)check_val );
	return check_val;
}



static void _os_AddSalt( uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)psv;
	if( vol->sigsalt ) {
		(*salt_size) = vol->sigkeyLength;
		(*salt) = (POINTER)vol->sigsalt;
		vol->sigsalt = NULL;
	}
	else if( vol->datakey ) {
		(*salt_size) = BLOCK_SIZE;
		(*salt) = (POINTER)vol->datakey;
		vol->datakey = NULL;
	}
	else if( vol->userkey ) {
		(*salt_size) = StrLen( vol->userkey );
		(*salt) = (POINTER)vol->userkey;
		vol->userkey = NULL;
	}
	else if( vol->devkey ) {
		(*salt_size) = StrLen( vol->devkey );
		(*salt) = (POINTER)vol->devkey;
		vol->devkey = NULL;
	}
	else if( vol->segment[vol->curseg] ) {
		BLOCKINDEX sector = vol->segment[vol->curseg];
		switch( vol->clusterKeyVersion ) {
		case 0:
			( *salt_size ) = sizeof( vol->segment[vol->curseg] );
			( *salt ) = &vol->segment[vol->curseg];
			break;
		case 1:
			memcpy( vol->tmpSalt, vol->key, 16 );
			vol->tmpSalt[sector & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[0];
			vol->tmpSalt[( sector >> 4 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[1];
			vol->tmpSalt[( sector >> 8 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[2];
			vol->tmpSalt[( sector >> 12 ) & 0xF] ^= ( (uint8_t*)( &vol->segment[vol->curseg] ) )[3];

			( (BLOCKINDEX*)vol->tmpSalt )[0] ^= sector;
			( (BLOCKINDEX*)vol->tmpSalt )[1] ^= sector;
			( *salt_size ) = 12;// sizeof( vol->segment[vol->curseg] );
			( *salt ) = vol->tmpSalt;
			break;
		}
	}
	else
		(*salt_size) = 0;
}

static void _os_AssignKey( struct sack_vfs_os_volume *vol, const char *key1, const char *key2 )
{
	uintptr_t size = BLOCK_SIZE + BLOCK_SIZE * BC(COUNT) + BLOCK_SIZE + SHORTKEY_LENGTH;
	if( !vol->key_buffer ) {
		int n;
		vol->key_buffer = (uint8_t*)HeapAllocateAligned( NULL, size, 4096 );// NewArray( uint8_t, size );
		memset( vol->key_buffer, 0, size );
		for( n = 0; n < BC(COUNT); n++ ) {
			vol->usekey_buffer[n] = vol->key_buffer + (n + 1) * BLOCK_SIZE;
		}
		for( n = 0; n < BC( COUNT ); n++ ) {
			vol->segment[n] = ~0;
			vol->_segment[n] = ~0;
			RESETFLAG( vol->dirty, n );
			RESETFLAG( vol->_dirty, n );
		}
	}

	vol->userkey = key1;
	vol->devkey = key2;
	if( key1 || key2 )
	{
		int n;
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( _os_AddSalt, (uintptr_t)vol );
		else
			SRG_ResetEntropy( vol->entropy );

		vol->key = (uint8_t*)HeapAllocateAligned( NULL, size, 4096 ); //NewArray( uint8_t, size );

		for( n = 0; n < BC(COUNT); n++ ) {
			vol->usekey[n] = vol->key + (n + 1) * BLOCK_SIZE;
		}
		vol->segkey = vol->key + BLOCK_SIZE * (BC(COUNT) + 1);
		vol->sigkey = vol->key + BLOCK_SIZE * (BC(COUNT) + 1) + SHORTKEY_LENGTH;
		vol->curseg = BC(DIRECTORY);
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)vol->key, BLOCK_SIZE * 8 );
	}
	else {
		int n;
		for( n = 0; n < BC(COUNT); n++ )
			vol->usekey[n] = l.zerokey;
		vol->segkey = l.zerokey;
		vol->sigkey = l.zerokey;
		vol->key = NULL;
	}
}

void sack_vfs_os_flush_volume( struct sack_vfs_os_volume * vol, LOGICAL unload ) {
	{
		INDEX idx;
		for( idx = 0; idx < BC( COUNT ); idx++ ) {
			if( unload ) {
				RESETFLAG( vol->seglock, idx );
			}
				if( TESTFLAG( vol->dirty, idx ) || TESTFLAG( vol->_dirty, idx ) ) {
					LoG( "Flush dirty segment: %d   %zx %d", (int)idx, vol->bufferFPI[idx], vol->segment[idx] );
					sack_fseek( vol->file, (size_t)vol->bufferFPI[idx], SEEK_SET );
					if( vol->userkey )
						SRG_XSWS_encryptData( vol->usekey_buffer[idx], BLOCK_SIZE
							, vol->segment[idx], (const uint8_t*)vol->userkey, strlen( vol->userkey )
							, NULL, NULL );
					sack_fwrite( vol->usekey_buffer[idx], 1, BLOCK_SIZE, vol->file );
					if( !TESTFLAG( vol->seglock, idx ) ) {
						vol->segment[idx] = ~0;
						if( vol->userkey )
							SRG_XSWS_decryptData( vol->usekey_buffer[idx], BLOCK_SIZE
								, vol->segment[idx], (const uint8_t*)vol->userkey, strlen( vol->userkey )
								, NULL, NULL );
					}
					RESETFLAG( vol->dirty, idx );
					RESETFLAG( vol->_dirty, idx );
				}
		}
	}
}

static uintptr_t volume_flusher( PTHREAD thread ) {
	struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)GetThreadParam( thread );
	while( 1 ) {

		while( 1 ) {
			int updated;
			INDEX idx;
			updated = 0;
			if( !LockedExchange( &vol->lock, 1 ) ) {
				// this could be 'faster' testing the whole
				// flag type size data.
				for( idx = 0; idx < BC( COUNT ); idx++ )
					if( TESTFLAG( vol->dirty, idx ) ) {
						updated = 1;
						SETFLAG( vol->_dirty, idx );
						RESETFLAG( vol->dirty, idx );
					}

				if( updated ) {
					vol->lock = 0; // data changed, don't flush.
					WakeableSleep( 256 );
				}
				else
					break; // have lock, break; flush dirty sectors(if any)
			}
			else // didn't get lock, wait.
				Relinquish();
		}

		{
			INDEX idx;
			for( idx = 0; idx < BC(COUNT); idx++ )
				if( TESTFLAG( vol->_dirty, idx )  // last pass marked this dirty
					&& !TESTFLAG( vol->dirty, idx ) ) { // hasn't been re-marked as dirty, so it's been idle...
					sack_fseek( vol->file, (size_t)vol->bufferFPI[idx], SEEK_SET );
					//uint8_t *crypt;
					//size_t cryptlen;
					if( vol->userkey )
						SRG_XSWS_encryptData( vol->usekey_buffer[idx], BLOCK_SIZE
							, vol->segment[idx], (const uint8_t*)vol->userkey, strlen( vol->userkey )
							, NULL, NULL );
					sack_fwrite( vol->usekey_buffer[idx], 1, BLOCK_SIZE, vol->file );
					RESETFLAG( vol->_dirty, idx );
					vol->segment[idx] = 0;
				}
		}

		vol->lock = 0;
		// for all dirty
		WakeableSleep( SLEEP_FOREVER );

	}
	return 0;
}

void sack_vfs_os_polish_volume( struct sack_vfs_os_volume* vol ) {
	static PTHREAD flusher;
	if( !flusher )
		flusher = ThreadTo( volume_flusher, (uintptr_t)vol );
	else
		WakeThread( flusher );
}


struct sack_vfs_os_volume *sack_vfs_os_load_volume( const char * filepath )
{
	struct sack_vfs_os_volume *vol = New( struct sack_vfs_os_volume );
	memset( vol, 0, sizeof( struct sack_vfs_os_volume ) );
	// since time is morely forward going; keeping the stack for the avl 
	// balancer can reduce forward-scanning insertion time
	vol->pdsCTimeStack = CreateDataStack( sizeof( struct memoryTimelineNode ) );
	vol->pdsWTimeStack = CreateDataStack( sizeof( struct memoryTimelineNode ) );

	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->volname = StrDup( filepath );
	_os_AssignKey( vol, NULL, NULL );
	if( !_os_ExpandVolume( vol ) || !_os_ValidateBAT( vol ) ) { Deallocate( struct sack_vfs_os_volume*, vol ); return NULL; }
#ifdef DEBUG_DIRECTORIES
	_os_dumpDirectories( vol, 0, 1 );
#endif
	return vol;
}

void sack_vfs_os_unload_volume( struct sack_vfs_os_volume * vol );

struct sack_vfs_os_volume *sack_vfs_os_load_crypt_volume( const char * filepath, uintptr_t version, const char * userkey, const char * devkey ) {
	struct sack_vfs_os_volume *vol = New( struct sack_vfs_os_volume );
	MemSet( vol, 0, sizeof( struct sack_vfs_os_volume ) );
	if( !version ) version = 2;
	vol->pdsCTimeStack = CreateDataStack( sizeof( struct memoryTimelineNode ) );
	vol->pdsWTimeStack = CreateDataStack( sizeof( struct memoryTimelineNode ) );
	vol->pdlFreeBlocks = CreateDataList( sizeof( BLOCKINDEX ) );
	vol->clusterKeyVersion = version - 1;
	vol->volname = StrDup( filepath );
	vol->userkey = userkey;
	vol->devkey = devkey;
	_os_AssignKey( vol, userkey, devkey );
	if( !_os_ExpandVolume( vol ) || !_os_ValidateBAT( vol ) ) { sack_vfs_os_unload_volume( vol ); return NULL; }
	return vol;
}

void sack_vfs_os_unload_volume( struct sack_vfs_os_volume * vol ) {
	INDEX idx;
	struct sack_vfs_file *file;
	LIST_FORALL( vol->files, idx, struct sack_vfs_file *, file )
		break;
	if( file ) {
		vol->closed = TRUE;
		return;
	}
	sack_vfs_os_flush_volume( vol, TRUE );
	strdup_free( (char*)vol->volname );
	DeleteListEx( &vol->files DBG_SRC );
	sack_fclose( vol->file );
	//if( !vol->external_memory )	CloseSpace( vol->diskReal );
	if( vol->key ) {
		Deallocate( uint8_t*, vol->key );
		SRG_DestroyEntropy( &vol->entropy );
	}
	Deallocate( uint8_t*, vol->key_buffer );
	Deallocate( struct sack_vfs_os_volume*, vol );
}

void sack_vfs_os_shrink_volume( struct sack_vfs_os_volume * vol ) {
	size_t n;
	unsigned int b = 0;
	//int found_free; // this block has free data; should be last BAT?
	BLOCKINDEX last_block = 0;
	unsigned int last_bat = 0;
	enum block_cache_entries cache = BC(BAT);
	BLOCKINDEX *current_BAT = TSEEK( BLOCKINDEX*, vol, 0, cache );
	if( !current_BAT ) return; // expand failed, tseek failed in response, so don't do anything
	do {
		BLOCKINDEX check_val;
		BLOCKINDEX *blockKey;
		blockKey = (BLOCKINDEX*)vol->usekey[cache];
		for( n = 0; n < BLOCKS_PER_BAT; n++ ) {
			check_val = *(current_BAT++);
			if( vol->key )	check_val ^= *(blockKey++);
			if( check_val ) {
				last_bat = b;
				last_block = n;
			}
		}
		b++;
		if( b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) < vol->dwSize ) {
			current_BAT = TSEEK( BLOCKINDEX*, vol, b * ( BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
		} else
			break;
	}while( 1 );
	sack_fclose( vol->file );
	vol->file = NULL;
	/*
	SetFileLength( vol->volname,
			last_bat * BLOCKS_PER_SECTOR * BLOCK_SIZE + ( last_block + 1 + 1 )* BLOCK_SIZE );
	*/
	// setting 0 size will cause expand to do an initial open instead of expanding
	vol->dwSize = 0;
}

static void _os_mask_block( struct sack_vfs_os_volume *vol, size_t n ) {
	BLOCKINDEX b = ( 1 + (n >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR) + (n & (BLOCKS_PER_BAT - 1)));
	_os_UpdateSegmentKey( vol, BC(DATAKEY), b + 1 );
	{
#ifdef __64__
		uint64_t* usekey = (uint64_t*)vol->usekey[BC(DATAKEY)];
		uint64_t* block = (uint64_t*)vol->usekey_buffer[BC( DATAKEY )];
		for( n = 0; n < (BLOCK_SIZE / 16); n++ ) {
			block[0] = block[0] ^ usekey[0];
			block[1] = block[1] ^ usekey[1];
			block += 2; usekey += 2;
		}
#else
		uint32_t* usekey = (uint32_t*)vol->usekey[BC(DATAKEY)];
		uint32_t* block = (uint32_t*)vol->usekey_buffer[BC(DATAKEY)];
		for( n = 0; n < (BLOCK_SIZE / 16); n++ ) {
			block[0] = block[0] ^ usekey[0];
			block[1] = block[1] ^ usekey[1];
			block[2] = block[2] ^ usekey[2];
			block[3] = block[3] ^ usekey[3];
			block += 4; usekey += 4;
		}
#endif
	}
}

LOGICAL sack_vfs_os_decrypt_volume( struct sack_vfs_os_volume *vol )
{
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( !vol->key ) { vol->lock = 0; return FALSE; } // volume is already decrypted, cannot remove key
	{
		enum block_cache_entries cache = BC(BAT);
		size_t n;
		BLOCKINDEX slab = vol->dwSize / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey;
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				block[0] ^= blockKey[0];
				if( block[0] == EOBBLOCK ) break;
				else if( block[0] ) _os_mask_block( vol, (n*BLOCKS_PER_BAT) + m );
				block++;
				blockKey++;
			}
			if( m < BLOCKS_PER_BAT ) break;
		}
	}
	_os_AssignKey( vol, NULL, NULL );
	vol->lock = 0;
	return TRUE;
}

LOGICAL sack_vfs_os_encrypt_volume( struct sack_vfs_os_volume *vol, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 ) {
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( vol->key ) { vol->lock = 0; return FALSE; } // volume already has a key, cannot apply new key
	if( !version ) version = 2;
	vol->clusterKeyVersion = version-1;
	_os_AssignKey( vol, key1, key2 );
	{
		int done;
		size_t n;
		enum block_cache_entries cache = BC(BAT);
		BLOCKINDEX slab = (vol->dwSize + (BLOCKS_PER_SECTOR*BLOCK_SIZE-1)) / ( BLOCKS_PER_SECTOR * BLOCK_SIZE );
		done = 0;
		for( n = 0; n < slab; n++  ) {
			size_t m;
			BLOCKINDEX *blockKey;
			BLOCKINDEX *block;// = (BLOCKINDEX*)(((uint8_t*)vol->disk) + n * (BLOCKS_PER_SECTOR * BLOCK_SIZE));
			block = TSEEK( BLOCKINDEX*, vol, n * (BLOCKS_PER_SECTOR*BLOCK_SIZE), cache );
			blockKey = ((BLOCKINDEX*)vol->usekey[cache]);

			//vol->segment[BC(BAT)] = n + 1;
			for( m = 0; m < BLOCKS_PER_BAT; m++ ) {
				if( block[0] == EOBBLOCK ) done = TRUE;
				else if( block[0] ) _os_mask_block( vol, (n*BLOCKS_PER_BAT) + m );
				block[0] ^= blockKey[0];
				if( done ) break;
				block++;
				blockKey++;
			}
			if( done ) break;
		}
	}
	vol->lock = 0;
	return TRUE;
}

const char *sack_vfs_os_get_signature( struct sack_vfs_os_volume *vol ) {
	static char signature[257];
	static const char *output = "0123456789ABCDEF";
	if( !vol )
		return NULL;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	{
		static BLOCKINDEX datakey[BLOCKS_PER_BAT];
		uint8_t* usekey = vol->key?vol->usekey[BC(DATAKEY)]:l.zerokey;
		signature[256] = 0;
		memset( datakey, 0, sizeof( datakey ) );
		{
			{
				size_t n;
				BLOCKINDEX this_dir_block = 0;
				BLOCKINDEX next_dir_block;
				BLOCKINDEX *next_entries;
				do {
					enum block_cache_entries cache = BC(DATAKEY);
					next_entries = BTSEEK( BLOCKINDEX *, vol, this_dir_block, cache );
					for( n = 0; n < BLOCKS_PER_BAT; n++ )
						datakey[n] ^= next_entries[n] ^ ((BLOCKINDEX*)(((uint8_t*)usekey)))[n];
					
					next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, GFB_INIT_DIRENT, FALSE );
#ifdef _DEBUG
					if( this_dir_block == next_dir_block )
						DebugBreak();
					if( next_dir_block == 0 )
						DebugBreak();
#endif
					this_dir_block = next_dir_block;
				}
				while( next_dir_block != EOFBLOCK );
			}
		}
		if( !vol->entropy )
			vol->entropy = SRG_CreateEntropy2( _os_AddSalt, (uintptr_t)vol );
		SRG_ResetEntropy( vol->entropy );
		vol->curseg = BC(DIRECTORY);
		vol->segment[vol->curseg] = 0;
		vol->datakey = (const char *)datakey;
		SRG_GetEntropyBuffer( vol->entropy, (uint32_t*)usekey, 128 * 8 );
		{
			int n;
			for( n = 0; n < 128; n++ ) {
				signature[n*2] = output[( usekey[n] >> 4 ) & 0xF];
				signature[n*2+1] = output[usekey[n] & 0xF];
			}
		}
	}
	vol->lock = 0;
	return signature;
}


//-----------------------------------------------------------------------------------
// Director Support Functions
//-----------------------------------------------------------------------------------

LOGICAL _os_ScanDirectory_( struct sack_vfs_os_volume *vol, const char * filename
	, BLOCKINDEX dirBlockSeg
	, BLOCKINDEX *nameBlockStart
	, struct sack_vfs_os_file *file
	, int path_match
	, char *leadin
	, int *leadinDepth
) {
	size_t n;
	int ofs = 0;
	BLOCKINDEX this_dir_block = dirBlockSeg;
	int usedNames;
	int minName;
	int curName;
	struct directory_hash_lookup_block *dirblock;
	struct directory_hash_lookup_block *dirblockkey;
	struct directory_entry *next_entries;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		BLOCKINDEX nameBlock;
		if( this_dir_block > 10000 ) DebugBreak();
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
		dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
		nameBlock = dirblock->names_first_block;
		if( filename )
		{
			BLOCKINDEX nextblock = dirblock->next_block[filename[ofs]] ^ dirblockkey->next_block[filename[ofs]];
			if( nextblock ) {
				leadin[(*leadinDepth)++] = filename[ofs];
				ofs += 1;
				this_dir_block = nextblock;
				continue;
			}
		}
		else {
			uint8_t charIndex;
			for( charIndex = 0; charIndex < 255; charIndex++ ) {
				BLOCKINDEX nextblock = dirblock->next_block[charIndex] ^ dirblockkey->next_block[charIndex];
				if( nextblock ) {
					LOGICAL r;
					leadin[(*leadinDepth)++] = (char)charIndex;
					r = _os_ScanDirectory_( vol, NULL, nextblock, nameBlockStart, file, path_match, leadin, leadinDepth );
					(*leadinDepth)--;
					if( r )
						return r;
				}
			}
		}
		usedNames = dirblock->used_names;
		minName = 0;
		curName = (usedNames) >> 1;
		{
			next_entries = dirblock->entries;
			while( minName <= usedNames && ( curName < usedNames ) && ( curName >= 0 ) )
			//for( n = 0; n < VFS_DIRECTORY_ENTRIES; n++ )
			{
				BLOCKINDEX bi;
				enum block_cache_entries name_cache = BC(NAMES);
				struct directory_entry *entkey = dirblockkey->entries + (n=curName);
				struct directory_entry *entry = dirblock->entries + n;
				//const char * testname;
				FPI name_ofs = ( next_entries[n].name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;

#ifdef DEBUG_TIMELINE_DIR_TRACKING
				{
					// make sure timeline and file entries reference each other.
					struct memoryTimelineNode time;
					reloadTimeEntry( &time, vol, entry->timelineEntry^entkey->timelineEntry );
					FPI entry_fpi = vol->bufferFPI[cache] + sane_offsetof( struct directory_hash_lookup_block, entries[n] );
					if( entry_fpi != time.dirent_fpi ) DebugBreak();
				}
#endif
				//if( filename && !name_ofs )	return FALSE; // done.
				if( 0 ) {
					LoG( "%d name_ofs = %" _size_f "(%" _size_f ") block = %d  vs %s"
						, n, name_ofs
						, next_entries[n].name_offset ^ entkey->name_offset
						, next_entries[n].first_block ^ entkey->first_block
						, filename + ofs );
				}
				if( USS_LT( n, size_t, usedNames, int ) ) {
					bi = next_entries[n].first_block ^ entkey->first_block;
					// if file is deleted; don't check it's name.
					if( !bi ) continue;
					// if file is end of directory, done sanning.
					if( bi == EODMARK ) return filename ? FALSE : (2); // done.
					if( name_ofs > vol->dwSize ) { return FALSE; }
				}
				//testname =
				if( filename ) {
					int d;
					//LoG( "this name: %s", names );
					if( ( d = _os_MaskStrCmp( vol, filename+ofs, nameBlock, name_ofs, path_match ) ) == 0 ) {
						if( file )
						{
							file->dirent_key = (*entkey);
							file->cache = cache;
							file->entry_fpi = vol->segment[BC(DIRECTORY)] * BLOCK_SIZE + ((uintptr_t)(((struct directory_hash_lookup_block *)0)->entries + n));
							file->entry = entry;
						}
						LoG( "return found entry: %p (%" _size_f ":%" _size_f ") %*.*s%s"
							, next_entries + n, name_ofs, next_entries[n].first_block ^ entkey->first_block
							, *leadinDepth, *leadinDepth, leadin
							, filename+ofs );
						if( nameBlockStart ) nameBlockStart[0] = dirblock->names_first_block ^ dirblockkey->names_first_block;
						return TRUE;
					}
					if( d > 0 ) {
						minName = curName + 1;
					} else {
						usedNames = curName - 1;
					}
					curName = (minName + usedNames) >> 1;
				}
				else
					minName++;
			}
			return filename ? FALSE : (2); // done.;
		}
		// unreachable, and broken code.
#if 0
		BLOCKINDEX next_dir_block;
		next_dir_block = vfs_os_GetNextBlock( vol, this_dir_block, GFB_INIT_TIMELINE_MORE, TRUE );
#ifdef _DEBUG
		if( this_dir_block == next_dir_block ) DebugBreak();
		if( next_dir_block == 0 ) { DebugBreak(); return FALSE; }  // should have a last-entry before no more blocks....
#endif
		this_dir_block = next_dir_block;
#endif
	}
	while( 1 );
}

// this results in an absolute disk position
static FPI _os_SaveFileName( struct sack_vfs_os_volume *vol, BLOCKINDEX firstNameBlock, const char * filename, size_t namelen ) {
	size_t n;
	int blocks = 0;
	LOGICAL scanToEnd = FALSE;
	BLOCKINDEX this_name_block = firstNameBlock;
#ifdef _DEBUG
	if( !firstNameBlock ) DebugBreak();
#endif
	while( 1 ) {
		enum block_cache_entries cache = BC(NAMES);
		unsigned char *names = BTSEEK( unsigned char *, vol, this_name_block, cache );
		unsigned char *name = (unsigned char*)names;
		if( scanToEnd ) {
			while(
				(UTF8_EOT != (name[0] ^ vol->usekey[cache][name - (unsigned char*)names]))
				&& (name - names < BLOCK_SIZE)
				) name++;
			if( ( name - names ) >= BLOCK_SIZE ) {
				// wow, that is a really LONG name.
				this_name_block = vfs_os_GetNextBlock( vol, this_name_block, GFB_INIT_NAMES, TRUE );
				blocks++;
				continue;
			}
		}
		scanToEnd = FALSE;
		while( name < ( (unsigned char*)names + BLOCK_SIZE ) ) {
			int c = name[0];
			if( vol->key ) c = c ^ vol->usekey[cache][name-names];
			if( (unsigned char)c == UTF8_EOTB ) {
				if( namelen < (size_t)( ( (unsigned char*)names + BLOCK_SIZE ) - name ) ) {
					//LoG( "using unused entry for new file...%" _size_f " %d(%d)  %" _size_f " %s", this_name_block, cache, cache - BC(NAMES), name - names, filename );
					if( vol->key ) {						
						for( n = 0; n < namelen; n++ )
							name[n] = filename[n] ^ vol->usekey[cache][n + (name-(unsigned char*)names)];
					} else
						memcpy( name, filename, namelen );

					name[namelen+0] = UTF8_EOT ^ vol->usekey[cache][name - names + namelen+0];
					name[namelen+1] = UTF8_EOTB ^ vol->usekey[cache][name - names + namelen+1];
					SETFLAG( vol->dirty, cache );
					//lprintf( "OFFSET:%d %d", (name) - (names), +blocks * BLOCK_SIZE );
					return (name) - (names) + blocks * BLOCK_SIZE;
				}
			}
			else
				if( _os_MaskStrCmp( vol, filename, firstNameBlock, (name - names)+blocks*BLOCK_SIZE, 0 ) == 0 ) {
					LoG( "using existing entry for new file...%s", filename );
					return (name) - (names) + blocks * BLOCK_SIZE;
				}
			while( 
				( UTF8_EOT != ( name[0] ^ vol->usekey[cache][name-(unsigned char*)names] ) )
				&& ( name-names < BLOCK_SIZE )
			) name++;
			if( name - names <= BLOCK_SIZE ) {
				if( name - names < BLOCK_SIZE ) {
					name++;
				}
				else {
					// next seek will be on right character...
				}
			}
			else 
				scanToEnd = TRUE; // still looking for EOT
			//LoG( "new position is %" _size_f "  %" _size_f, this_name_block, name - names );
		}
		this_name_block = vfs_os_GetNextBlock( vol, this_name_block, GFB_INIT_NAMES, TRUE );
		blocks++;
		//LoG( "Need a new name block.... %d", this_name_block );
	}
}

static void ConvertDirectory( struct sack_vfs_os_volume *vol, const char *leadin, int leadinLength, BLOCKINDEX this_dir_block, struct directory_hash_lookup_block *orig_dirblock, enum block_cache_entries *newCache ) {
	size_t n;
	int ofs = 0;
#ifdef DEBUG_FILE_OPEN
	LoG( "------------ BEGIN CONVERT DIRECTORY ----------------------" );
#endif
	do {
		enum block_cache_entries cache = BC(DIRECTORY);
		FPI nameoffset = 0;
		BLOCKINDEX new_dir_block;
		struct directory_hash_lookup_block *dirblock;
		struct directory_hash_lookup_block *dirblockkey;
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
		dirblockkey = (struct directory_hash_lookup_block *)vol->usekey[cache];
		{
			static int counters[256];
			static uint8_t namebuffer[18*4096];
			uint8_t *nameblock;
			uint8_t *namekey;
			int maxc = 0;
			int imax = 0;
			int f;
			enum block_cache_entries name_cache;
			BLOCKINDEX name_block = dirblock->names_first_block ^ dirblockkey->names_first_block;
			// read name block chain into a single array
			do {
				uint8_t *out = namebuffer + nameoffset;
				name_cache = BC( NAMES );
				nameblock = BTSEEK( uint8_t *, vol, name_block, name_cache );
				namekey = (uint8_t*)vol->usekey[name_cache];
				if( vol->key )
					for( n = 0; n < 4096; n++ )
						(*out++) = (*nameblock++) ^ (*namekey++);
				else
					for( n = 0; n < 4096; n++ )
						(*out++) = (*nameblock++);
				name_block = vfs_os_GetNextBlock( vol, name_block, GFB_INIT_NONE, 0 );
				nameoffset += 4096;
			} while( name_block != EOFBLOCK );


			memset( counters, 0, sizeof( counters ) );
			// 257/85
			for( f = 0; f < VFS_DIRECTORY_ENTRIES; f++ ) {
				BLOCKINDEX first = dirblock->entries[f].first_block ^ dirblockkey->entries[f].first_block;
				FPI name;
				int count;
				if( first == EODMARK ) break;
				name = dirblock->entries[f].name_offset ^ dirblockkey->entries[f].name_offset;
				count = (++counters[namebuffer[name]]);
				if( count > maxc ) {
					imax = namebuffer[name];
					maxc = count;
				}
			}
			// after finding most used first byte; get a new block, and point
			// hash entry to that.
			dirblock->next_block[imax]
				= ( new_dir_block
				  = _os_GetFreeBlock( vol, GFB_INIT_DIRENT ) ) ^ dirblockkey->next_block[imax];
			if( new_dir_block == 48 || new_dir_block == 36 )
				DebugBreak();

			SETFLAG( vol->dirty, cache );
			{
				struct directory_hash_lookup_block *newDirblock;
				struct directory_hash_lookup_block *newDirblockkey;
				enum block_cache_entries newdir_cache;
				BLOCKINDEX newFirstNameBlock;
				int usedNames = dirblock->used_names ^ dirblockkey->used_names;
				int _usedNames = usedNames;
				int nf = 0;
				int firstNameOffset = -1;
				int finalNameOffset = 0;;
				int movedEntry = 0;
				int offset;
				newdir_cache = BC(DIRECTORY);
				newDirblock = BTSEEK( struct directory_hash_lookup_block *, vol, new_dir_block, newdir_cache );

#ifdef DEBUG_FILE_OPEN
				LoG( "new dir block cache is  %d   %d", newdir_cache, (int)new_dir_block );
#endif
				newDirblockkey = (struct directory_hash_lookup_block *)vol->usekey[newdir_cache];
				newFirstNameBlock = newDirblock->names_first_block ^ newDirblockkey->names_first_block;
#ifdef _DEBUG
				if( !newDirblock->names_first_block )
					DebugBreak();
#endif
				newDirblock->next_block[DIRNAME_CHAR_PARENT] = (this_dir_block << 8) | imax;

				//SETFLAG( vol->dirty, newdir_cache ); // this will be dirty because it was init above.

				for( f = 0; f < usedNames; f++ ) {
					BLOCKINDEX first = dirblock->entries[f].first_block ^ dirblockkey->entries[f].first_block;
					struct directory_entry *entry;
					struct directory_entry *entkey;
					struct directory_entry *newEntry;
					struct directory_entry *newEntkey;
					FPI name;
					FPI name_ofs;
					entry = dirblock->entries + (f);
					entkey = dirblockkey->entries + (f);
					name = ( entry->name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
					if( namebuffer[name] == imax ) {
						int namelen;
						if( !movedEntry ) movedEntry = f+1;
						newEntry = newDirblock->entries + (nf);
						newEntkey = newDirblockkey->entries + (nf);
						//LoG( "Saving existing name %d %s", name, namebuffer + name );
						//LogBinary( namebuffer, 32 );
						namelen = 0;
						while( namebuffer[name + namelen] != UTF8_EOT )namelen++;
						name_ofs = _os_SaveFileName( vol, newFirstNameBlock, (char*)(namebuffer + name + 1), namelen -1 ) ^ newEntkey->name_offset;
						{
							INDEX idx;
							struct sack_vfs_os_file  * file;
							LIST_FORALL( vol->files, idx, struct sack_vfs_file  *, file ) {
								if( file->entry == entry ) {
									file->entry_fpi = 0; // new entry_fpi.
								}
							}
						}
						dirblock->used_names = ((dirblock->used_names ^ dirblockkey->used_names) - 1) ^ dirblockkey->used_names;
						newEntry->filesize = (entry->filesize ^ entkey->filesize) ^ newEntkey->filesize;
						{
							struct memoryTimelineNode time;
							FPI oldFPI;
							enum block_cache_entries  timeCache = BC( TIMELINE );
							reloadTimeEntry( &time, vol, (entry->timelineEntry     ^ entkey->timelineEntry) );
							oldFPI = time.dirent_fpi;
							// new entry is still the same timeline entry as the old entry.
							newEntry->timelineEntry = (entry->timelineEntry     ^ entkey->timelineEntry)     ^ newEntkey->timelineEntry;
							// timeline points at new entry.
							time.dirent_fpi = vol->bufferFPI[newdir_cache] + sane_offsetof( struct directory_hash_lookup_block , entries[nf]);
#ifdef DEBUG_TIMELINE_DIR_TRACKING
							lprintf( "Set timeline %d to %d", (int)time.index, (int)time.dirent_fpi );
#endif
							updateTimeEntry( &time, vol );

#ifdef DEBUG_TIMELINE_DIR_TRACKING
							lprintf( "direntry at %d  %d is time %d", (int)new_dir_block, (int)nf, (int)newEntry->timelineEntry );
#endif

							{
								INDEX idx;
								struct sack_vfs_file  * file;
								LIST_FORALL( vol->files, idx, struct sack_vfs_file  *, file ) {
									if( file->entry_fpi == oldFPI ) {
										file->entry_fpi = time.dirent_fpi; // new entry_fpi.
									}
								}
							}
						}

						newEntry->name_offset = name_ofs;
						newEntry->first_block = (entry->first_block ^ entkey->first_block) ^ newEntkey->first_block;
						SETFLAG( vol->dirty, cache );
						nf++;

						newDirblock->used_names = ((newDirblock->used_names ^ newDirblockkey->used_names) + 1) ^ newDirblockkey->used_names;
						//usedNames--;
					}
					else {
						if( movedEntry ) {
							break;
						}
					}
				}

				// move all others down 1.
				movedEntry = movedEntry - 1;
				offset = (f - movedEntry);
				usedNames -= (f-movedEntry);
				//for( ; f < usedNames; f++ )
				{
					int m;
					for( m = movedEntry; m < usedNames; m++ ) {
						dirblock->entries[m].first_block = (dirblock->entries[m + offset].first_block
							^ dirblockkey->entries[m + offset].first_block)
							^ dirblockkey->entries[m].first_block;
						dirblock->entries[m].name_offset = (dirblock->entries[m + offset].name_offset
							^ dirblockkey->entries[m + offset].name_offset)
							^ dirblockkey->entries[m].name_offset;
						dirblock->entries[m].filesize = (dirblock->entries[m + offset].filesize
							^ dirblockkey->entries[m + offset].filesize)
							^ dirblockkey->entries[m].filesize;
						dirblock->entries[m].timelineEntry = (dirblock->entries[m + offset].timelineEntry
							^ dirblockkey->entries[m + offset].timelineEntry)
							^ dirblockkey->entries[m].timelineEntry;
#ifdef DEBUG_TIMELINE_DIR_TRACKING
						lprintf( "direntry at %d  %d is time %d", (int)this_dir_block, (int)m, (int)dirblock->entries[m].timelineEntry );
#endif
						{
							struct memoryTimelineNode time;
							enum block_cache_entries  timeCache = BC( TIMELINE );
							reloadTimeEntry( &time, vol, (dirblock->entries[m + offset].timelineEntry ^ dirblockkey->entries[m + offset].timelineEntry) );
							time.dirent_fpi = vol->bufferFPI[cache] + sane_offsetof( struct directory_hash_lookup_block, entries[m] );
#ifdef DEBUG_TIMELINE_DIR_TRACKING
							lprintf( "Set timeline %d to %d", (int)time.index, (int)time.dirent_fpi );
#endif
							updateTimeEntry( &time, vol );
						}
#ifdef _DEBUG
						if( !dirblock->names_first_block ) DebugBreak();
#endif
					}

					for( m = usedNames; m < VFS_DIRECTORY_ENTRIES; m++ ) {
						dirblock->entries[m].first_block = (0)
							^ dirblockkey->entries[m].first_block;
						dirblock->entries[m].name_offset = (0)
							^ dirblockkey->entries[m].name_offset;
						dirblock->entries[m].filesize = (0)
							^ dirblockkey->entries[m].filesize;
						dirblock->entries[m].timelineEntry = (0)
							^ dirblockkey->entries[m].timelineEntry;
#ifdef _DEBUG
						if( !dirblock->names_first_block ) DebugBreak();
#endif
					}
				}


				if( usedNames ) {
					static uint8_t newnamebuffer[18 * 4096];
					int newout = 0;
					int min_name = BLOCK_SIZE + 1;
					int _min_name = -1; // min found has to be after this one.
					//lprintf( "%d names remained.", usedNames );
					for( f = 0; f < usedNames; f++ ) {
						struct directory_entry *entry;
						struct directory_entry *entkey;
						FPI name;
						entry = dirblock->entries + (f);
						entkey = dirblockkey->entries + (f);
						name = ( entry->name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
						entry->name_offset = ( newout ^ entkey->name_offset )
							| ( (entry->name_offset ^ entkey->name_offset)
								& ~DIRENT_NAME_OFFSET_OFFSET );
						while( namebuffer[name] != UTF8_EOT )
							newnamebuffer[newout++] = namebuffer[name++];
						newnamebuffer[newout++] = namebuffer[name++];
					}
					newnamebuffer[newout++] = UTF8_EOTB;
					memcpy( namebuffer, newnamebuffer, newout );
					memset( newnamebuffer, 0, newout );
				}
				else {
					namebuffer[0] = UTF8_EOTB;
				}
				{
					name_block = dirblock->names_first_block ^ dirblockkey->names_first_block;
					nameoffset = 0;
					do {
						uint8_t *out;
						nameblock = namebuffer + nameoffset;
						name_cache = BC( NAMES );
						out = BTSEEK( uint8_t *, vol, name_block, name_cache );
						namekey = (uint8_t*)vol->usekey[name_cache];
						if( vol->key )
							for( n = 0; n < 4096; n++ )
								(*out++) = (*nameblock++) ^ (*namekey++);
						else
							for( n = 0; n < 4096; n++ )
								(*out++) = (*nameblock++);
						SETFLAG( vol->dirty, cache );
						name_block = vfs_os_GetNextBlock( vol, name_block, GFB_INIT_NONE, 0 );
						nameoffset += 4096;
					} while( name_block != EOFBLOCK );
				}
			}
			break;  // a set of names has been moved out of this block.
			// has block.
		}
	} while( 0 );

	// unlink here
	// unlink dirblock->names_first_block
}

static struct directory_entry * _os_GetNewDirectory( struct sack_vfs_os_volume *vol, 
#if defined( _MSC_VER )
	_In_
#endif
	const char * filename
		, struct sack_vfs_os_file *file ) {
	size_t n;
	const char *_filename = filename;
	static char leadin[256];
	static int leadinDepth = 0;
	BLOCKINDEX this_dir_block = FIRST_DIR_BLOCK;
	struct directory_entry *next_entries;
	LOGICAL moveMark = FALSE;
	if( filename && filename[0] == '.' && ( filename[1] == '/' || filename[1] == '\\' ) ) filename += 2;
	leadinDepth = 0;
	do {
		enum block_cache_entries cache;
		FPI dirblockFPI;
		int usedNames;
		struct directory_hash_lookup_block *dirblock;
		struct directory_hash_lookup_block *dirblockkey;
		BLOCKINDEX firstNameBlock;
		cache = BC( DIRECTORY );
		dirblock = BTSEEK( struct directory_hash_lookup_block *, vol, this_dir_block, cache );
#ifdef _DEBUG
		if( !dirblock->names_first_block ) DebugBreak();
#endif

		dirblockFPI = vol->bufferFPI[cache];
		dirblockkey = (struct directory_hash_lookup_block *)vol->usekey[cache];
		firstNameBlock = dirblock->names_first_block^dirblockkey->names_first_block;
		{
			BLOCKINDEX nextblock = dirblock->next_block[filename[0]] ^ dirblockkey->next_block[filename[0]];
			if( nextblock ) {
				leadin[leadinDepth++] = filename[0];
				filename++;
				this_dir_block = nextblock;
				continue; // retry;
			}
		}
		usedNames = dirblock->used_names ^ dirblockkey->used_names;
		//lprintf( " --------------- THIS DIR BLOCK ---------------" );
		//_os_dumpDirectories( vol, this_dir_block, 1 );
		if( usedNames == VFS_DIRECTORY_ENTRIES ) {
			ConvertDirectory( vol, leadin, leadinDepth, this_dir_block, dirblock, &cache );
			continue; // retry;
		}
		{
			struct directory_entry *entkey;
			struct directory_entry *ent;
			FPI name_ofs;
			BLOCKINDEX first_blk;

			next_entries = dirblock->entries;
			entkey = dirblockkey->entries;
			ent = dirblock->entries;
			for( n = 0; USS_LT( n, size_t, usedNames, int ); n++ ) {
				ent = dirblock->entries + (n);
				entkey = dirblockkey->entries + (n);
				name_ofs = ( ent->name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
				first_blk = ent->first_block ^ entkey->first_block;
				// not name_offset (end of list) or not first_block(free entry) use this entry
				//if( name_ofs && (first_blk > 1) )  continue;

				if( _os_MaskStrCmp( vol, filename, firstNameBlock, name_ofs, 0 ) < 0 ) {
					int m;
#ifdef DEBUG_FILE_OPEN
					LoG( "Insert new directory" );
#endif
					for( m = dirblock->used_names; SUS_GT( m, int, n, size_t ); m-- ) {
						struct memoryTimelineNode node;
						dirblock->entries[m].filesize      = dirblock->entries[m - 1].filesize      ^ dirblockkey->entries[m - 1].filesize;
						dirblock->entries[m].first_block   = dirblock->entries[m - 1].first_block   ^ dirblockkey->entries[m - 1].first_block;
						dirblock->entries[m].name_offset   = dirblock->entries[m - 1].name_offset   ^ dirblockkey->entries[m - 1].name_offset;
						reloadTimeEntry( &node, vol, dirblock->entries[m - 1].timelineEntry ^ dirblockkey->entries[m - 1].timelineEntry );
						dirblock->entries[m].timelineEntry = dirblock->entries[m - 1].timelineEntry ^ dirblockkey->entries[m - 1].timelineEntry;
#ifdef DEBUG_TIMELINE_DIR_TRACKING
						lprintf( "direntry at %d  %d is time %d", (int)this_dir_block, (int)m, (int)dirblock->entries[m].timelineEntry );
#endif
						node.dirent_fpi = dirblockFPI + sane_offsetof( struct directory_hash_lookup_block, entries[m] );
#ifdef DEBUG_TIMELINE_DIR_TRACKING
						lprintf( "Set timeline %d to %d", (int)node.index, (int)node.dirent_fpi );
#endif
						updateTimeEntry( &node, vol );
					}
					dirblock->used_names++;
					break;
				}
			}
			ent = dirblock->entries + (n);
			if( n == usedNames ) {
				dirblock->used_names = (uint8_t)((n + 1) ^ dirblockkey->used_names);
			}
			//LoG( "Get New Directory save naem:%s", filename );
			name_ofs = _os_SaveFileName( vol, firstNameBlock, filename, StrLen( filename ) ) ^ entkey->name_offset;
			// have to allocate a block for the file, otherwise it would be deleted.
			first_blk = _os_GetFreeBlock( vol, GFB_INIT_NONE) ^ entkey->first_block;
		
			ent->filesize = entkey->filesize;
			ent->name_offset = name_ofs;
			ent->first_block = first_blk;
			{
				struct memoryTimelineNode time_;
				struct memoryTimelineNode *time = &time_;
				if( file )
					time = &file->timeline;
				else
					time_.ctime.raw = GetTimeOfDay();
				time->dirent_fpi = dirblockFPI + sane_offsetof( struct directory_hash_lookup_block, entries[n] );;
				// associate a time entry with this directory entry, and vice-versa.
				getTimeEntry( time, vol, 1, NULL, 0 );
#ifdef DEBUG_TIMELINE_DIR_TRACKING
				lprintf( "Set timeline %d to %d", (int)time->index, (int)time->dirent_fpi );
#endif
				ent->timelineEntry = time->index ^ entkey->timelineEntry;
#ifdef DEBUG_TIMELINE_DIR_TRACKING
				lprintf( "direntry at %d  %d is time %d", (int)this_dir_block, (int)n, (int)dirblock->entries[n].timelineEntry );
#endif
				//updateTimeEntry( time, vol );
			}
			if( file ) {
				SETFLAG( vol->seglock, cache );
				file->entry_fpi = dirblockFPI + sane_offsetof( struct directory_hash_lookup_block, entries[n] );;
				file->entry = ent;
				file->dirent_key = entkey[n];
			}
			SETFLAG( vol->dirty, cache );
			return ent;
		}
	}
	while( 1 );

}

struct sack_vfs_os_file * CPROC sack_vfs_os_openfile( struct sack_vfs_os_volume *vol, const char * filename ) {
	struct sack_vfs_os_file *file = New( struct sack_vfs_os_file );
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	MemSet( file, 0, sizeof( struct sack_vfs_os_file ) );
	file->vol = vol;
	file->entry = &file->_entry;
	file->sealant = NULL;

	if( filename[0] == '.' && ( filename[1] == '\\' || filename[1] == '/' ) ) filename += 2;

#ifdef DEBUG_FILE_OPEN
	LoG( "sack_vfs open %s = %p on %s", filename, file, vol->volname );
#endif
	if( !_os_ScanDirectory( vol, filename, FIRST_DIR_BLOCK, NULL, file, 0 ) ) {
		if( vol->read_only ) { LoG( "Fail open: readonly" ); vol->lock = 0; Deallocate( struct sack_vfs_os_file*, file ); return NULL; }
		else _os_GetNewDirectory( vol, filename, file );
	}
	file->_first_block = file->block = file->entry->first_block;
	
	sack_vfs_os_read_internal( file, &file->diskHeader, sizeof( file->diskHeader ) );
	file->header = file->diskHeader;
	file->fpi = file->header.sealant.avail + file->header.references.avail;

	{
		BLOCKINDEX offset = (file->entry->name_offset ^ file->dirent_key.name_offset);
		uint32_t sealLen = (offset & DIRENT_NAME_OFFSET_FLAG_SEALANT) >> DIRENT_NAME_OFFSET_FLAG_SEALANT_SHIFT;
		if( sealLen ) {
			file->seal = NewArray( uint8_t, sealLen );
			//file->sealantLen = sealLen;
			file->sealed = SACK_VFS_OS_SEAL_LOAD;
		}
		else {
			file->seal = NULL;
			//file->sealantLen = 0;
			file->sealed = SACK_VFS_OS_SEAL_NONE;
		}
		file->filename = StrDup( filename );
	}
	AddLink( &vol->files, file );
	vol->lock = 0;
	return file;
}

static struct sack_vfs_os_file * CPROC sack_vfs_os_open( uintptr_t psvInstance, const char * filename, const char *opts ) {
	return sack_vfs_os_openfile( (struct sack_vfs_os_volume*)psvInstance, filename );
}



static char * getFilename( const char *objBuf, size_t objBufLen
	, char *sealBuf, size_t sealBufLen, LOGICAL owner
	, char **idBuf, size_t *idBufLen ) {

	if( sealBuf ) {
		struct random_context *signEntropy = (struct random_context *)DequeLink( &l.plqCrypters );
		char *fileKey;
		size_t keyLen;
		uint8_t outbuf[32];

		if( !signEntropy )
			signEntropy = SRG_CreateEntropy4( NULL, (uintptr_t)0 );

		if( owner ) {
			char *metakey = SRG_ID_Generator3();
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)metakey, 44 );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)sealBuf, sealBufLen );
			SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

		}
		else {
			SRG_ResetEntropy( signEntropy );
			SRG_FeedEntropy( signEntropy, (const uint8_t*)sealBuf, sealBufLen );
			SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
		}

		SRG_ResetEntropy( signEntropy );
		SRG_FeedEntropy( signEntropy, (const uint8_t*)objBuf, objBufLen );
		SRG_FeedEntropy( signEntropy, (const uint8_t*)outbuf, 32 );
		fileKey = EncodeBase64Ex( outbuf, 32, &keyLen, (const char*)1 );

		SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

		SRG_DestroyEntropy( &signEntropy );

		idBuf[0] = EncodeBase64Ex( outbuf, 256 / 8, idBufLen, (const char *)1 );
		
		EnqueLink( &l.plqCrypters, signEntropy );
		return fileKey;
	}
	else {
		idBuf[0] = SRG_ID_Generator3();
		idBufLen[0] = 42;
		return idBuf[0];
	}
}


int CPROC sack_vfs_os_exists( struct sack_vfs_os_volume *vol, const char * file ) {
	LOGICAL result;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	if( file[0] == '.' && file[1] == '/' ) file += 2;
	result = _os_ScanDirectory( vol, file, FIRST_DIR_BLOCK, NULL, NULL, 0 );
	vol->lock = 0;
	return result;
}

size_t CPROC sack_vfs_os_tell( struct sack_vfs_os_file *file ) { return (size_t)file->fpi; }

size_t CPROC sack_vfs_os_size( struct sack_vfs_os_file *file ) {	return (size_t)(file->entry->filesize ^ file->dirent_key.filesize); }

size_t CPROC sack_vfs_os_seek_internal( struct sack_vfs_os_file *file, size_t pos, int whence )
{
	FPI old_fpi = file->fpi;
	if( whence == SEEK_SET ) file->fpi = pos;
	if( whence == SEEK_CUR ) file->fpi += pos;
	if( whence == SEEK_END ) file->fpi = ( file->entry->filesize  ^ file->dirent_key.filesize ) + pos;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	{
		if( ( file->fpi & ( ~BLOCK_MASK ) ) >= ( old_fpi & ( ~BLOCK_MASK ) ) ) {
			do {
				if( ( file->fpi & ( ~BLOCK_MASK ) ) == ( old_fpi & ( ~BLOCK_MASK ) ) ) {
					file->vol->lock = 0;
					return (size_t)file->fpi;
				}
				file->block = vfs_os_GetNextBlock( file->vol, file->block, GFB_INIT_NONE, TRUE );
				old_fpi += BLOCK_SIZE;
			} while( 1 );
		}
	}
	{
		size_t n = 0;
		BLOCKINDEX b = file->_first_block;
		while( n * BLOCK_SIZE < ( pos & ~BLOCK_MASK ) ) {
			b = vfs_os_GetNextBlock( file->vol, b, GFB_INIT_NONE, TRUE );
			n++;
		}
		file->block = b;
	}
	file->vol->lock = 0;
	return (size_t)file->fpi;
}
size_t CPROC sack_vfs_os_seek( struct sack_vfs_os_file* file, size_t pos, int whence ) {
	return sack_vfs_os_seek_internal( (struct sack_vfs_os_file*) file, pos, whence );
}


static void _os_MaskBlock( struct sack_vfs_os_volume *vol, uint8_t* usekey, uint8_t* block, BLOCKINDEX block_ofs, size_t ofs, const char *data, size_t length ) {
	size_t n;
	block += block_ofs;
	usekey += ofs;
	if( vol->key )
		for( n = 0; n < length; n++ ) (*block++) = (*data++) ^ (*usekey++);
	else
		memcpy( block, data, length );
}

#define IS_OWNED(file)  ( (file->entry->name_offset^file->dirent_key.name_offset) & DIRENT_NAME_OFFSET_FLAG_OWNED )

size_t CPROC sack_vfs_os_write_internal( struct sack_vfs_os_file* file, const void* data_, size_t length
		, POINTER writeState ) {
	const char* data = (const char*)data_;
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	LOGICAL updated = FALSE;
	uint8_t* cdata;
	size_t cdataLen;
	if( file->readKey && !file->fpi ) {
		SRG_XSWS_encryptData( (uint8_t*)data, length, file->timeline.ctime.raw
			, (const uint8_t*)file->readKey, file->readKeyLen
			, &cdata, &cdataLen );
		data = (const char*)cdata;
		length = cdataLen;
	}
	else {
		cdata = NULL;
	}
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	if( (file->entry->name_offset ^ file->dirent_key.name_offset) & DIRENT_NAME_OFFSET_FLAG_SEALANT ) {
		char* filename;
		size_t filenameLen = 64;
		// read-only data block.
		lprintf( "INCOMPLETE - TODO WRITE PATCH" );
		char* sealer = getFilename( data, length, (char*)file->sealant, file->header.sealant.used, IS_OWNED( file ), &filename, &filenameLen );

		struct sack_vfs_os_file* pFile = (struct sack_vfs_os_file*)sack_vfs_os_openfile( file->vol, filename );
		pFile->sealant = (uint8_t*)sealer;
		if( cdata ) Release( cdata );
		return sack_vfs_os_write_internal( pFile, data, length, (POINTER)1 );
	}
#ifdef DEBUG_FILE_OPS
	LoG( "Write to file %p %" _size_f "  @%" _size_f, file, length, ofs );
#endif
	if( ofs ) {
		enum block_cache_entries cache = BC( FILE );
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( length >= (BLOCK_SIZE - (ofs)) ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, ofs, ofs, data, BLOCK_SIZE - ofs );
			SETFLAG( file->vol->dirty, cache );
			data += BLOCK_SIZE - ofs;
			written += BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			if( file->fpi > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
				updated = TRUE;
			}

			file->block = vfs_os_GetNextBlock( file->vol, file->block, GFB_INIT_NONE, TRUE );
			length -= BLOCK_SIZE - ofs;
		}
		else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, ofs, ofs, data, length );
			SETFLAG( file->vol->dirty, cache );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
				updated = TRUE;
			}
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length ) {
		enum block_cache_entries cache = BC( FILE );
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
#ifdef _DEBUG
		if( file->block < 2 ) DebugBreak();
#endif
		if( length >= BLOCK_SIZE ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, 0, 0, data, BLOCK_SIZE );
			SETFLAG( file->vol->dirty, cache );
			data += BLOCK_SIZE;
			written += BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			if( file->fpi > ( file->entry->filesize ^ file->dirent_key.filesize ) ) {
				updated = TRUE;
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			}
			file->block = vfs_os_GetNextBlock( file->vol, file->block, GFB_INIT_NONE, TRUE );
			length -= BLOCK_SIZE;
		}
		else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], block, 0, 0, data, length );
			SETFLAG( file->vol->dirty, cache );
			data += length;
			written += length;
			file->fpi += length;
			if( file->fpi > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				updated = TRUE;
				file->entry->filesize = file->fpi ^ file->dirent_key.filesize;
			}
			length = 0;
		}
	}

#if 0
	if( !writeState && file->sealant && (void*)file->sealant != (void*)data ) {
		flushFileSuffix( file );

		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX saveFpi = file->fpi;
		sack_vfs_os_write_internal( file, (char*)file->sealant, file->header.sealant.used, (POINTER)1 );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
	}
#endif
	if( updated ) {
		SETFLAG( file->vol->dirty, file->cache ); // directory cache block (locked)
	}
	if( cdata ) Release( cdata );
	//if( !writeState )
	file->vol->lock = 0;
	return written;
}

size_t CPROC sack_vfs_os_write( struct sack_vfs_os_file *file, const void * data_, size_t length ) {
	return sack_vfs_os_write_internal( (struct sack_vfs_os_file* )file, data_, length, NULL );
}

static enum sack_vfs_os_seal_states ValidateSeal( struct sack_vfs_os_file *file, char *data, size_t length ) {
	BLOCKINDEX offset = (file->entry->name_offset ^ file->dirent_key.name_offset);
	uint32_t sealLen = (offset & DIRENT_NAME_OFFSET_FLAG_SEALANT) >> DIRENT_NAME_OFFSET_FLAG_SEALANT_SHIFT;
	struct random_context *signEntropy;// = (struct random_context *)DequeLink( &signingEntropies );
	uint8_t outbuf[32];

	signEntropy = SRG_CreateEntropy4( NULL, (uintptr_t)0 );

	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)file->sealant, file->header.sealant.used );
	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );
	if( (file->header.sealant.used != 32) || MemCmp( outbuf, file->sealant, 32 ) )
		return SACK_VFS_OS_SEAL_INVALID;

	SRG_ResetEntropy( signEntropy );
	SRG_FeedEntropy( signEntropy, (const uint8_t*)data, length );

	// DO NOT DOUBLE_PROCESS THIS DATA
	SRG_FeedEntropy( signEntropy, (const uint8_t*)file->sealant, file->header.sealant.used );

	SRG_GetEntropyBuffer( signEntropy, (uint32_t*)outbuf, 256 );

	SRG_DestroyEntropy( &signEntropy );
	{
		enum sack_vfs_os_seal_states success = SACK_VFS_OS_SEAL_INVALID;
		size_t len;
		char *rid = EncodeBase64Ex( outbuf, 256 / 8, &len, (const char *)1 );
		if( StrCmp( file->filename, rid ) == 0 )
			success = SACK_VFS_OS_SEAL_VALID;
		Deallocate( char *, rid );
		return success;
	}
}

size_t CPROC sack_vfs_os_read_internal( struct sack_vfs_os_file *file, void * data_, size_t length ) {
	char* data = (char*)data_;
	size_t written = 0;
	size_t ofs = file->fpi & BLOCK_MASK;
	if( (file->entry->name_offset ^ file->dirent_key.name_offset) & DIRENT_NAME_OFFSET_FLAG_READ_KEYED ) {
		if( !file->readKey ) return 0;
	}
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < ( file->fpi + length ) ) {
		if( ( file->entry->filesize  ^ file->dirent_key.filesize ) < file->fpi )
			length = 0;
		else
			length = (size_t)(( file->entry->filesize  ^ file->dirent_key.filesize ) - file->fpi);
	}
	if( !length ) {  file->vol->lock = 0; return 0; }

	if( ofs ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( length >= ( BLOCK_SIZE - ( ofs ) ) ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), BLOCK_SIZE - ofs );
			written += BLOCK_SIZE - ofs;
			data += BLOCK_SIZE - ofs;
			length -= BLOCK_SIZE - ofs;
			file->fpi += BLOCK_SIZE - ofs;
			file->block = vfs_os_GetNextBlock( file->vol, file->block, GFB_INIT_NONE, TRUE );
		} else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, ofs, (const char*)(block+ofs), length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}
	// if there's still length here, FPI is now on the start of blocks
	while( length ) {
		enum block_cache_entries cache = BC(FILE);
		uint8_t* block = (uint8_t*)vfs_os_BSEEK( file->vol, file->block, &cache );
		if( length >= BLOCK_SIZE ) {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, 0, (const char*)block, BLOCK_SIZE - ofs );
			written += BLOCK_SIZE;
			data += BLOCK_SIZE;
			length -= BLOCK_SIZE;
			file->fpi += BLOCK_SIZE;
			file->block = vfs_os_GetNextBlock( file->vol, file->block, GFB_INIT_NONE, TRUE );
		} else {
			_os_MaskBlock( file->vol, file->vol->usekey[cache], (uint8_t*)data, 0, 0, (const char*)block, length );
			written += length;
			file->fpi += length;
			length = 0;
		}
	}

	if( file->readKey
	   && ( file->fpi == ( file->entry->filesize ^ file->dirent_key.filesize ) )
	   && ( (file->entry->name_offset ^ file->dirent_key.name_offset)
	      & DIRENT_NAME_OFFSET_FLAG_READ_KEYED) )
	{
		uint8_t *outbuf;
		size_t outlen;
		SRG_XSWS_decryptData( (uint8_t*)data, written, file->timeline.ctime.raw
		                    , (const uint8_t*)file->readKey, file->readKeyLen
		                    , &outbuf, &outlen );
		memcpy( data, outbuf, outlen );
		Release( outbuf );
		written = outlen;
	}

	if( file->sealant
		&& (void*)file->sealant != (void*)data
		&& length == ( file->entry->filesize ^ file->dirent_key.filesize ) ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX saveFpi = file->fpi;
		file->entry->filesize = ((file->entry->filesize
			^ file->dirent_key.filesize) + file->header.sealant.used + sizeof( BLOCKINDEX ))
			^ file->dirent_key.filesize;
		sack_vfs_os_read_internal( file, (char*)file->sealant, file->header.sealant.used );
		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
		file->sealed = ValidateSeal( file, data, length );
	}
	file->vol->lock = 0;
	return written;
}

size_t CPROC sack_vfs_os_read( struct sack_vfs_os_file* file, void* data_, size_t length ) {
	return sack_vfs_os_read_internal( (struct sack_vfs_os_file*)file, data_, length );
}

static BLOCKINDEX sack_vfs_os_read_patches( struct sack_vfs_os_file *file ) {
	size_t written = 0;
	BLOCKINDEX saveFpi = file->fpi;
	size_t length;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();

	length = (size_t)(file->entry->filesize  ^ file->dirent_key.filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek_internal( file, length, SEEK_SET );

#if 0
	if( file->sealant ) {
		BLOCKINDEX saveSize = file->entry->filesize;
		BLOCKINDEX patches;

		//WriteIntoBlock( file, 0, 0, file->sealant, file->header.sealant.used );

		file->entry->filesize = saveSize;
		file->fpi = saveFpi;
		file->sealed = SACK_VFS_OS_SEAL_LOAD;
		return patches;
	}
#endif
	file->vol->lock = 0;
	return written;
}

static size_t sack_vfs_os_set_patch_block( struct sack_vfs_os_file *file, BLOCKINDEX patchBlock ) {
	size_t written = 0;
	size_t length;
	BLOCKINDEX saveFpi = file->fpi;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	length = (size_t)(file->entry->filesize  ^ file->dirent_key.filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek_internal( file, length, SEEK_SET );

	if( file->header.sealant.avail ) {
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	} else {
		BLOCKINDEX saveSize = file->entry->filesize;
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	}
	file->vol->lock = 0;
	return written;
}

static size_t sack_vfs_os_set_reference_block( struct sack_vfs_os_file *file, BLOCKINDEX patchBlock ) {
	size_t written = 0;
	size_t length;
	BLOCKINDEX saveFpi = file->fpi;
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
	length = (size_t)(file->entry->filesize  ^ file->dirent_key.filesize);

	if( !length ) { file->vol->lock = 0; return 0; }

	sack_vfs_os_seek_internal( file, length, SEEK_SET );

	if( file->sealant ) {
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	}
	else {
		sack_vfs_os_seek_internal( file, file->header.sealant.used, SEEK_CUR );
		sack_vfs_os_write_internal( file, (char*)&patchBlock, sizeof( BLOCKINDEX ), NULL );
		file->fpi = saveFpi;
	}
	file->vol->lock = 0;
	return written;
}

static void sack_vfs_os_unlink_file_entry( struct sack_vfs_os_volume *vol, struct sack_vfs_os_file *dirinfo, BLOCKINDEX first_block, LOGICAL deleted ) {
	//FPI entFPI, struct directory_entry *entry, struct directory_entry *entkey
	BLOCKINDEX block, _block;
	struct sack_vfs_os_file *file_found = NULL;
	struct sack_vfs_os_file *file;
	INDEX idx;
	LIST_FORALL( vol->files, idx, struct sack_vfs_os_file *, file ) {
		if( file->_first_block == first_block ) {
			file_found = file;
			file->delete_on_close = TRUE;
		}
	}
	if( !deleted ) {
		// delete the file entry now; this disk entry may be reused immediately.
		dirinfo->entry->first_block = dirinfo->dirent_key.first_block;
		SETFLAG( vol->dirty, dirinfo->cache );
	}

	if( !file_found ) {
		_block = block = first_block;// entry->first_block ^ entkey->first_block;
		LoG( "(marking physical deleted (again?)) entry starts at %d", block );
		// wipe out file chain BAT
		do {
			enum block_cache_entries cache = BC(BAT);
			enum block_cache_entries fileCache = BC(DATAKEY);
			BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), cache );
			BLOCKINDEX _thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[cache])[_block & (BLOCKS_PER_BAT-1)]:0;
			//BLOCKINDEX b = BLOCK_SIZE + (block >> BLOCK_SHIFT) * (BLOCKS_PER_SECTOR*BLOCK_SIZE) + (block & (BLOCKS_PER_BAT - 1)) * BLOCK_SIZE;
			uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( vol, block, &fileCache );
			//LoG( "Clearing file datablock...%p", (uintptr_t)blockData - (uintptr_t)vol->disk );
			memset( blockData, 0, BLOCK_SIZE );
			// after seek, block was read, and file position updated.
			SETFLAG( vol->dirty, cache );

			block = vfs_os_GetNextBlock( vol, block, GFB_INIT_NONE, FALSE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
			AddDataItem( &vol->pdlFreeBlocks, &_block );

			_block = block;
		} while( block != EOFBLOCK );
	}
}

static void _os_shrinkBAT( struct sack_vfs_os_file *file ) {
	struct sack_vfs_os_volume *vol = file->vol;
	BLOCKINDEX block, _block;
	size_t bsize = 0;

	_block = block = file->entry->first_block ^ file->dirent_key.first_block;
	do {
		enum block_cache_entries cache = BC(BAT);
		enum block_cache_entries data_cache = BC(DATAKEY);
		BLOCKINDEX *this_BAT = TSEEK( BLOCKINDEX*, vol, ( ( block >> BLOCK_SHIFT ) * ( BLOCKS_PER_SECTOR*BLOCK_SIZE) ), cache );
		BLOCKINDEX _thiskey;
		_thiskey = ( vol->key )?((BLOCKINDEX*)vol->usekey[cache])[_block & (BLOCKS_PER_BAT-1)]:0;
		block = vfs_os_GetNextBlock( vol, block, GFB_INIT_NONE, FALSE );
		if( bsize > (file->entry->filesize ^ file->dirent_key.filesize) ) {
			uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( file->vol, _block, &data_cache );
			//LoG( "clearing a datablock after a file..." );
			memset( blockData, 0, BLOCK_SIZE );
			this_BAT[_block & (BLOCKS_PER_BAT-1)] = _thiskey;
		} else {
			bsize++;
			if( bsize > (file->entry->filesize ^ file->dirent_key.filesize) ) {
				uint8_t* blockData = (uint8_t*)vfs_os_BSEEK( file->vol, _block, &data_cache );
				//LoG( "clearing a partial datablock after a file..., %d, %d", BLOCK_SIZE-(file->entry->filesize & (BLOCK_SIZE-1)), ( file->entry->filesize & (BLOCK_SIZE-1)) );
				memset( blockData + (file->entry->filesize & (BLOCK_SIZE-1)), 0, BLOCK_SIZE-(file->entry->filesize & (BLOCK_SIZE-1)) );
				this_BAT[_block & (BLOCKS_PER_BAT-1)] = ~_thiskey;
			}
		}
		_block = block;
	} while( block != EOFBLOCK );	
}

size_t CPROC sack_vfs_os_truncate_internal( struct sack_vfs_os_file *file ) { file->entry->filesize = file->fpi ^ file->dirent_key.filesize; _os_shrinkBAT( file ); return (size_t)file->fpi; }

size_t CPROC sack_vfs_os_truncate( struct sack_vfs_os_file* file ) { return sack_vfs_os_truncate_internal( (struct sack_vfs_os_file*)file ); }

int CPROC sack_vfs_os_close_internal( struct sack_vfs_os_file *file ) {
	while( LockedExchange( &file->vol->lock, 1 ) ) Relinquish();
#ifdef DEBUG_TRACE_LOG
	{
		enum block_cache_entries cache = BC(NAMES);
		static char fname[256];
		FPI name_ofs = file->entry->name_offset ^ file->dirent_key.name_offset;
		TSEEK( const char *, file->vol, name_ofs, cache ); // have to do the seek to the name block otherwise it might not be loaded.
		_os_MaskStrCpy( fname, sizeof( fname ), file->vol, name_ofs );
#ifdef DEBUG_FILE_OPS
		LoG( "close file:%s(%p)", fname, file );
#endif
	}
#endif
	DeleteLink( &file->vol->files, file );
	if( file->delete_on_close ) sack_vfs_os_unlink_file_entry( file->vol, file, file->_first_block, TRUE );
	{
		INDEX idx;
		struct sack_vfs_file * testFile;
		LIST_FORALL( file->vol->files, idx, struct sack_vfs_file *, testFile ) {
			if( testFile->cache == file->cache )
				break;
		}
		if( !testFile )
			RESETFLAG( file->vol->seglock, file->cache );
	}
	Deallocate( char *, file->filename );
	if( file->sealant )
		Deallocate( uint8_t*, file->sealant );
	file->vol->lock = 0;
	if( file->vol->closed ) sack_vfs_os_unload_volume( file->vol );
	Deallocate( struct sack_vfs_os_file *, file );
	return 0;
}
int CPROC sack_vfs_os_close( struct sack_vfs_os_file* file ) {
	return sack_vfs_os_close_internal( (struct sack_vfs_os_file*) file );
}

int CPROC sack_vfs_os_unlink_file( struct sack_vfs_os_volume *vol, const char * filename ) {
	int result = 0;
	struct sack_vfs_os_file tmp_dirinfo;
	if( !vol ) return 0;
	while( LockedExchange( &vol->lock, 1 ) ) Relinquish();
	LoG( "unlink file:%s", filename );
	if( _os_ScanDirectory( vol, filename, FIRST_DIR_BLOCK, NULL, &tmp_dirinfo, 0 ) ) {
		sack_vfs_os_unlink_file_entry( vol, &tmp_dirinfo, tmp_dirinfo.entry->first_block ^ tmp_dirinfo.dirent_key.first_block, FALSE );
		result = 1;
	}
	vol->lock = 0;
	return result;
}

int CPROC sack_vfs_os_flush( struct sack_vfs_os_file *file ) {	/* noop */	return 0; }

static LOGICAL CPROC sack_vfs_os_need_copy_write( void ) {	return FALSE; }


struct sack_vfs_os_find_info * CPROC sack_vfs_os_find_create_cursor(uintptr_t psvInst,const char *base,const char *mask )
{
	struct sack_vfs_os_find_info *info = New( struct sack_vfs_os_find_info );
	info->pds_directories = CreateDataStack( sizeof( struct hashnode ) );
	info->base = base;
	info->base_len = StrLen( base );
	info->mask = mask;
	info->vol = (struct sack_vfs_os_volume *)psvInst;
	info->leadinDepth = 0;
	return (struct sack_vfs_os_find_info*)info;
}


static int _os_iterate_find( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	struct directory_hash_lookup_block *dirBlock;
	struct directory_hash_lookup_block *dirBlockKey;
	struct directory_entry *next_entries;
	int n;
	do
	{
		enum block_cache_entries cache = BC(DIRECTORY);
		enum block_cache_entries name_cache = BC(NAMES);
		struct hashnode node = ((struct hashnode *)PopData( &info->pds_directories ))[0];
		dirBlock = BTSEEK( struct directory_hash_lookup_block *, info->vol, node.this_dir_block, cache );
		dirBlockKey = (struct directory_hash_lookup_block *)info->vol->usekey[cache];

		if( !node.thisent ) {
			struct hashnode subnode;
			subnode.thisent = 0;
			for( n = 254; n >= 0; n-- ) {
				BLOCKINDEX block = dirBlock->next_block[n] ^ dirBlockKey->next_block[n];
				if( block ) {
					memcpy( subnode.leadin, node.leadin, node.leadinDepth );
					subnode.leadin[node.leadinDepth] = (char)n;
					subnode.leadinDepth = node.leadinDepth + 1;
					subnode.leadin[subnode.leadinDepth] = 0;
					subnode.this_dir_block = block;
					if( subnode.this_dir_block > 5000 ) DebugBreak();
					PushData( &info->pds_directories, &subnode );
				}
			}
		}
		//lprintf( "%p ledin : %*.*s %d", node, node.leadinDepth, node.leadinDepth, node.leadin, node.leadinDepth );
		next_entries = dirBlock->entries;
		for( n = (int)node.thisent; n < (dirBlock->used_names ^ dirBlockKey->used_names); n++ ) {
			struct directory_entry *entkey = ( info->vol->key)?((struct directory_hash_lookup_block *)info->vol->usekey[cache])->entries+n:&l.zero_entkey;
			FPI name_ofs = ( next_entries[n].name_offset ^ entkey->name_offset ) & DIRENT_NAME_OFFSET_OFFSET;
			FPI name_ofs_ = name_ofs;
			const char *filename, *filename_;
			int l;

			struct memoryTimelineNode time;
			enum block_cache_entries  timeCache = BC( TIMELINE );
			reloadTimeEntry( &time, info->vol, (next_entries[n].timelineEntry ^ entkey->timelineEntry) );
			if( !time.ctime.raw ) DebugBreak();
			info->ctime = time.ctime.raw;
			info->wtime = time.stime.raw;
			// if file is deleted; don't check it's name.
			info->filesize = (size_t)(next_entries[n].filesize ^ entkey->filesize);
			if( (name_ofs) > info->vol->dwSize ) {
				lprintf( "corrupted volume." );
				return 0;
			}

			name_cache = BC( NAMES );
			filename = (const char *)vfs_os_FSEEK( info->vol, NULL, dirBlock->names_first_block ^ dirBlockKey->names_first_block, name_ofs, &name_cache );
			filename_ = filename;
			info->filenamelen = 0;
			for( l = 0; l < node.leadinDepth; l++ ) info->filename[info->filenamelen++] = node.leadin[l];

			if( info->vol->key ) {
				int c;
				do {
					while( ((name_ofs & ~BLOCK_MASK) == (name_ofs_ & ~BLOCK_MASK))
						&& ((c = (((uint8_t*)filename)[0] ^ info->vol->usekey[name_cache][name_ofs&BLOCK_MASK])) != UTF8_EOT)
						) {
						info->filename[info->filenamelen++] = c;
						filename++;
						name_ofs++;
					}
					if( ((name_ofs & ~BLOCK_MASK) == (name_ofs_ & ~BLOCK_MASK)) ) {
						name_cache = BC( NAMES );
						filename = (const char *)vfs_os_FSEEK( info->vol, NULL, dirBlock->names_first_block ^ dirBlockKey->names_first_block, name_ofs, &name_cache );
						name_ofs_ = name_ofs;
						continue;
					}
					break;
				} while( 1 );
				info->filename[info->filenamelen]	 = 0;
				//LoG( "Scan return filename: %s", info->filename );
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			} else {
				int c;
				do {
					while(
						((name_ofs&~BLOCK_MASK) == (name_ofs_ & ~BLOCK_MASK))
						&& ((c = (((uint8_t*)filename)[0])) != UTF8_EOT)
						) {
						info->filename[info->filenamelen++] = c;
						filename_ = filename;
						filename++;
						name_ofs++;
					}
					if( ((((uintptr_t)filename)&~BLOCK_MASK) != (((uintptr_t)filename_)&~BLOCK_MASK)) ) {
						name_cache = BC( NAMES );
						filename = (const char*)vfs_os_FSEEK( info->vol, NULL, dirBlock->names_first_block ^ dirBlockKey->names_first_block, name_ofs, &name_cache );
						//filenamekey = (const char*)(vol->usekey[cache]) + (name_offset & BLOCK_MASK);
						name_ofs_ = name_ofs;
						continue;
					}
					break;
				}
				while( 1 );
				info->filename[info->filenamelen] = 0;
#ifdef DEBUG_FILE_SCAN
				LoG( "Scan return filename: %s", info->filename );
#endif
				if( info->base
				    && ( info->base[0] != '.' && info->base_len != 1 )
				    && StrCaseCmpEx( info->base, info->filename, info->base_len ) )
					continue;
			}
			node.thisent = n + 1;
			if( node.this_dir_block > 5000 ) DebugBreak();
			PushData( &info->pds_directories, &node );
			return 1;
		}
	}
	while( info->pds_directories->Top );
	return 0;
}

int CPROC sack_vfs_os_find_first( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	struct hashnode root;
	root.this_dir_block = 0;
	root.leadinDepth = 0;
	root.thisent = 0;
	PushData( &info->pds_directories, &root );
	//info->thisent = 0;
	return _os_iterate_find( _info );
}

int CPROC sack_vfs_os_find_close( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	Deallocate( struct sack_vfs_os_find_info*, info ); return 0; }
int CPROC sack_vfs_os_find_next( struct sack_vfs_os_find_info *_info ) { return _os_iterate_find( _info ); }
char * CPROC sack_vfs_os_find_get_name( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	return info->filename; }
size_t CPROC sack_vfs_os_find_get_size( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	return info->filesize; }
LOGICAL CPROC sack_vfs_os_find_is_directory( struct sack_vfs_os_find_info *cursor ) { return FALSE; }
LOGICAL CPROC sack_vfs_os_is_directory( uintptr_t psvInstance, const char *path ) {
	if( path[0] == '.' && path[1] == 0 ) return TRUE;
	{
		struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)psvInstance;
		if( _os_ScanDirectory( vol, path, FIRST_DIR_BLOCK, NULL, NULL, 1 ) ) {
			return TRUE;
		}
	}
	return FALSE;
}
uint64_t CPROC sack_vfs_os_find_get_ctime( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	if( info ) return info->ctime;
	return 0;
}
uint64_t CPROC sack_vfs_os_find_get_wtime( struct sack_vfs_os_find_info *_info ) {
	struct sack_vfs_os_find_info *info = (struct sack_vfs_os_find_info *)_info;
	if( info ) return info->wtime;
	return 0;
}

LOGICAL CPROC sack_vfs_os_rename( uintptr_t psvInstance, const char *original, const char *newname ) {
	struct sack_vfs_os_volume *vol = (struct sack_vfs_os_volume *)psvInstance;
	lprintf( "RENAME IS NOT SUPPORTED IN OBJECT STORAGE(OR NEEDS TO BE FIXED)" );
	// fail if the names are the same.
	return TRUE;
}


uintptr_t CPROC sack_vfs_os_file_ioctl_internal( struct sack_vfs_os_file* file, uintptr_t opCode, va_list args ) {
	//va_list args;
	//va_start( args, opCode );
	switch( opCode ) {
	default:
		// unhandled/ignored opcode
		return FALSE;
		break;
	case SOSFSFIO_DESTROY_INDEX:
	{
		const char* indexname = va_arg( args, const char* );

		break;
	}
	case SOSFSFIO_CREATE_INDEX:
	{
		const char* indexname = va_arg( args, const char* );
		size_t indexnameLen = va_arg( args, size_t );
		//enum jsox_value_types type = va_arg( args, enum jsox_value_types );
		//int typeExtra = va_arg( args, int );
		struct memoryStorageIndex* index = allocateIndex( file, indexname, indexnameLen );
		//file->
		return (uintptr_t)index;
		break;
	}
	case SOSFSFIO_ADD_INDEX_ITEM:
	{
		struct memoryStorageIndex* index = (struct memoryStorageIndex*)va_arg( args, uintptr_t );
		struct sack_vfs_os_file *reference = va_arg( args, struct sack_vfs_os_file* );		
		struct jsox_value_container * value = va_arg( args, struct jsox_value_container* );

		//file->
		break;
	}
	case SOSFSFIO_REMOVE_INDEX_ITEM:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_ADD_REFERENCE:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_REMOVE_REFERENCE:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_ADD_REFERENCE_BY:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_REMOVE_REFERENCE_BY:
	{
		const char* indexname = va_arg( args, const char* );
		//file->
		break;
	}
	case SOSFSFIO_TAMPERED:
	{
		//struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		int *result = va_arg( args, int* );

		if( file->sealant ) {
			switch( file->sealed ) {
			case SACK_VFS_OS_SEAL_STORE:
			case SACK_VFS_OS_SEAL_VALID:
				(*result) = 1;
            break;
			default:
				(*result) = 0;
			}
		}
		else
			(*result) = 1;
	}
	break;
	case SOSFSFIO_PROVIDE_SEALANT:
	{
		const char *sealant = va_arg( args, const char * );
		size_t sealantLen = va_arg( args, size_t );
		//struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		{
			size_t len;
			if( file->sealant )
				Release( file->sealant );
			file->sealant = (uint8_t*)DecodeBase64Ex( sealant, sealantLen, &len, (const char*)1 );
			_os_SetSmallBlockUsage( &file->header.sealant, (uint8_t)len );
			_os_UpdateFileBlocks( file );
			if( file->sealed == SACK_VFS_OS_SEAL_NONE )
				file->sealed = SACK_VFS_OS_SEAL_STORE;
			else if( file->sealed == SACK_VFS_OS_SEAL_VALID || file->sealed == SACK_VFS_OS_SEAL_LOAD )
				file->sealed = SACK_VFS_OS_SEAL_STORE_PATCH;
			else
				lprintf( "Unhandled SEAL state." );
			//file->sealant = sealant;
			//file->sealantLen = sealantLen;

			// set the sealant length in the name offset.
			file->entry->name_offset = (((file->entry->name_offset ^ file->dirent_key.name_offset)
				| ((len >> 2) << 17)) ^ file->dirent_key.name_offset);
		}
	}
	break;
	case SOSFSFIO_PROVIDE_READKEY:
	{
		const char *sealant = va_arg( args, const char * );
		size_t sealantLen = va_arg( args, size_t );
		//struct sack_vfs_file *file = (struct sack_vfs_file *)psvInstance;
		{
			size_t len;
			if( file->readKey )
				Release( file->readKey );
			file->readKey = (uint8_t*)DecodeBase64Ex( sealant, sealantLen, &len, (const char*)1 );
			file->readKeyLen = (uint16_t)len;

			// set the sealant length in the name offset.
			file->entry->name_offset = (((file->entry->name_offset ^ file->dirent_key.name_offset)
				| DIRENT_NAME_OFFSET_FLAG_READ_KEYED) ^ file->dirent_key.name_offset);
		}
	}
	break;
	}
	return TRUE;
}

uintptr_t CPROC sack_vfs_os_file_ioctl_interface( uintptr_t psvInstance, uintptr_t opCode, va_list args ) {
	return sack_vfs_os_file_ioctl_internal( (struct sack_vfs_os_file*)psvInstance, opCode, args );
}
uintptr_t CPROC sack_vfs_os_file_ioctl( struct sack_vfs_os_file *psvInstance, uintptr_t opCode, ... ) {
	va_list args;
	va_start( args, opCode );
	return sack_vfs_os_file_ioctl_internal( (struct sack_vfs_os_file*)psvInstance, opCode, args );
}


uintptr_t CPROC sack_vfs_os_system_ioctl_internal( struct sack_vfs_os_volume *vol, uintptr_t opCode, va_list args ) {
	//va_list args;
	//va_start( args, opCode );
	switch( opCode ) {
	default:
		// unhandled/ignored opcode
		return FALSE;

	case SOSFSSIO_LOAD_OBJECT:
		return FALSE;

	case SOSFSSIO_PATCH_OBJECT:
		{
		LOGICAL owner = va_arg( args, LOGICAL );  // seal input is a constant, generate random meta key

		char *objIdBuf = va_arg( args, char * );
		size_t objIdBufLen = va_arg( args, size_t );

		char *patchAuth = va_arg( args, char * );
		size_t patchAuthLen = va_arg( args, size_t );

		char *objBuf = va_arg( args, char * );
		size_t objBufLen = va_arg( args, size_t );

		char *sealBuf = va_arg( args, char * );
		size_t sealBufLen = va_arg( args, size_t );

		char *keyBuf = va_arg( args, char * );
		size_t keyBufLen = va_arg( args, size_t );

		char *idBuf = va_arg( args, char * );
		size_t idBufLen = va_arg( args, size_t );

		if( sack_vfs_os_exists( vol, objIdBuf ) ) {
			struct sack_vfs_os_file* file = (struct sack_vfs_os_file*)sack_vfs_os_openfile( vol, objIdBuf );
			BLOCKINDEX patchBlock = sack_vfs_os_read_patches( file );
			if( !patchBlock ) {
				patchBlock = _os_GetFreeBlock( vol, GFB_INIT_PATCHBLOCK );
			}
			{

				enum block_cache_entries cache;
				struct directory_patch_block *newPatchblock;
				struct directory_patch_block *newPatchblockkey;

				cache = BC(FILE);
				newPatchblock = BTSEEK( struct directory_patch_block *, vol, patchBlock, cache );
				newPatchblockkey = (struct directory_patch_block *)vol->usekey[cache];

				while( 1 ) {
					//char objId[45];
					//size_t objIdLen;
					char *seal = getFilename( objBuf, objBufLen, sealBuf, sealBufLen, FALSE, &idBuf, &idBufLen );

					if( sack_vfs_os_exists( vol, idBuf ) ) {
						if( !sealBuf ) { // accidental key collision.
							continue; // try again.
						}
						else {
							// deliberate key collision; and record already exists.
							return TRUE;
						}
					}
					else {
						struct sack_vfs_os_file* file = (struct sack_vfs_os_file*)sack_vfs_os_openfile( vol, idBuf );

						//  file->entry_fpi
						newPatchblock->entries[newPatchblock->usedEntries].raw
							= file->entry_fpi ^ newPatchblockkey->entries[newPatchblock->usedEntries].raw;
						newPatchblock->usedEntries = (newPatchblock->usedEntries + 1) ^ newPatchblockkey->usedEntries;
						SETFLAG( vol->dirty, cache );

						file->sealant = (uint8_t*)seal;
						_os_SetSmallBlockUsage( &file->header.sealant, (uint8_t)strlen( seal ) );
						_os_UpdateFileBlocks( file );
						sack_vfs_os_seek_internal( file, (size_t)GetBlockStart( file, FILE_BLOCK_DATA ), SEEK_SET );
						sack_vfs_os_write_internal( file, objBuf, objBufLen, 0 );
						sack_vfs_os_close_internal( file );
					}
					return TRUE;
				}
			}
		}
		return FALSE; // object to patch was not found.
	}
	break;
	case SOSFSSIO_STORE_OBJECT:
	{
		LOGICAL owner = va_arg( args, LOGICAL );  // seal input is a constant, generate random meta key
		char *objBuf = va_arg( args, char * );
		size_t objBufLen = va_arg( args, size_t );
		char *objIdBuf = va_arg( args, char * );  // provided for re-write; provided also for private named objects
		size_t objIdBufLen = va_arg( args, size_t );
		char *sealBuf = va_arg( args, char * );  // user provided sealant if any
		size_t sealBufLen = va_arg( args, size_t );
		char *keyBuf = va_arg( args, char * );  // encryption key
		size_t keyBufLen = va_arg( args, size_t );
		char **idBuf = va_arg( args, char ** );  // output buffer
		size_t *idBufLen = va_arg( args, size_t* );
		while( 1 ) {
			char *seal = getFilename( objBuf, objBufLen, sealBuf, sealBufLen, owner, idBuf, idBufLen );
			if( sack_vfs_os_exists( vol, idBuf[0] ) ) {
				if( !sealBuf ) { // accidental key collision.
					continue; // try again.
				}
				else {
					// deliberate key collision; and record already exists.
					return TRUE;
				}
			}
			else {
				struct sack_vfs_os_file* file = (struct sack_vfs_os_file*)sack_vfs_os_openfile( vol, idBuf[0] );
				if( sealBuf ) {
					file->sealant = (uint8_t*)seal;
					_os_SetSmallBlockUsage( &file->header.sealant, (uint8_t)strlen( seal ) );
					WriteIntoBlock( file, 0, 0, seal, strlen( seal ) );
					//file->sealantLen = (uint8_t)strlen( seal );
				} else {
					file->sealant = NULL;
					_os_SetSmallBlockUsage( &file->header.sealant, 0 );
					//file->sealantLen = 0;
				}
				sack_vfs_os_write_internal( file, objBuf, objBufLen, NULL );
				sack_vfs_os_close_internal( file );
			}
			return TRUE;
		}

	}
	break;
	}
}


uintptr_t CPROC sack_vfs_os_system_ioctl_interface( uintptr_t psvInstance, uintptr_t opCode, va_list args ) {
	return sack_vfs_os_system_ioctl_internal( (struct sack_vfs_os_volume*)psvInstance, opCode, args );
}
uintptr_t CPROC sack_vfs_os_system_ioctl( struct sack_vfs_os_volume* vol, uintptr_t opCode, ... ) {
	va_list args;
	va_start( args, opCode );
	return sack_vfs_os_system_ioctl_internal( vol, opCode, args );
}

#ifndef USE_STDIO
static struct file_system_interface sack_vfs_os_fsi = {
                                                     (void*(CPROC*)(uintptr_t,const char *, const char*))sack_vfs_os_open
                                                   , (int(CPROC*)(void*))sack_vfs_os_close
                                                   , (size_t(CPROC*)(void*,void*,size_t))sack_vfs_os_read
                                                   , (size_t(CPROC*)(void*,const void*,size_t))sack_vfs_os_write
                                                   , (size_t(CPROC*)(void*,size_t,int))sack_vfs_os_seek
                                                   , (void(CPROC*)(void*))sack_vfs_os_truncate
                                                   , (int(CPROC*)(uintptr_t,const char*))sack_vfs_os_unlink_file
                                                   , (size_t(CPROC*)(void*))sack_vfs_os_size
                                                   , (size_t(CPROC*)(void*))sack_vfs_os_tell
                                                   , (int(CPROC*)(void*))sack_vfs_os_flush
                                                   , (int(CPROC*)(uintptr_t,const char*))sack_vfs_os_exists
                                                   , sack_vfs_os_need_copy_write
                                                   , (struct find_cursor*(CPROC*)(uintptr_t,const char *,const char *))             sack_vfs_os_find_create_cursor
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_os_find_first
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_os_find_close
                                                   , (int(CPROC*)(struct find_cursor*))             sack_vfs_os_find_next
                                                   , (char*(CPROC*)(struct find_cursor*))           sack_vfs_os_find_get_name
                                                   , (size_t(CPROC*)(struct find_cursor*))          sack_vfs_os_find_get_size
                                                   , (LOGICAL(CPROC*)(struct find_cursor*))         sack_vfs_os_find_is_directory
                                                   , (LOGICAL(CPROC*)(uintptr_t, const char*))      sack_vfs_os_is_directory
                                                   , (LOGICAL(CPROC*)(uintptr_t, const char*, const char*))sack_vfs_os_rename
                                                   , (uintptr_t(CPROC*)(uintptr_t, uintptr_t, va_list))sack_vfs_os_file_ioctl_interface
												   , (uintptr_t(CPROC*)(uintptr_t, uintptr_t, va_list))sack_vfs_os_system_ioctl_interface
	, (uint64_t( CPROC*)(struct find_cursor* cursor)) sack_vfs_os_find_get_ctime
	, (uint64_t( CPROC* )(struct find_cursor* cursor)) sack_vfs_os_find_get_wtime
};

PRIORITY_PRELOAD( Sack_VFS_OS_Register, CONFIG_SCRIPT_PRELOAD_PRIORITY - 2 )
{
#undef DEFAULT_VFS_NAME
#ifdef ALT_VFS_NAME
#   define DEFAULT_VFS_NAME SACK_VFS_FILESYSTEM_NAME "-os.runner"
#else
#   define DEFAULT_VFS_NAME SACK_VFS_FILESYSTEM_NAME "-os"
#endif
	sack_register_filesystem_interface( DEFAULT_VFS_NAME, &sack_vfs_os_fsi );
}

PRIORITY_PRELOAD( Sack_VFS_OS_RegisterDefaultFilesystem, SQL_PRELOAD_PRIORITY + 1 ) {
	if( SACK_GetProfileInt( GetProgramName(), "SACK/VFS/Mount FS VFS", 0 ) ) {
		struct sack_vfs_os_volume *vol;
		TEXTCHAR volfile[256];
		TEXTSTR tmp;
		SACK_GetProfileString( GetProgramName(), "SACK/VFS/OS File", "*/../assets.os", volfile, 256 );
		tmp = ExpandPath( volfile );
		vol = sack_vfs_os_load_volume( tmp );
		Deallocate( TEXTSTR, tmp );
		sack_mount_filesystem( "sack_shmem-os", sack_get_filesystem_interface( DEFAULT_VFS_NAME )
		                     , 900, (uintptr_t)vol, TRUE );
	}
}

#endif

#ifdef __cplusplus
}
#endif

#ifndef USE_STDIO
#  undef sack_fopen
#  undef sack_fseek
#  undef sack_fclose
#  undef sack_fread
#  undef sack_fwrite
#  undef sack_ftell
#  undef free
#  undef StrDup
#  define StrDup(o) StrDupEx( (o) DBG_SRC )
#endif

#undef free

SACK_VFS_NAMESPACE_END

#undef l
#endif

#undef SACK_VFS_SOURCE
#undef SACK_VFS_OS_SOURCE
