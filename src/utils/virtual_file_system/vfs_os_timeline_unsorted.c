//#define DEBUG_TEST_LOCKS

//#define DEBUG_VALIDATE_TREE_ADD
//#define DEBUG_LOG_LOCKS

//#define INVERSE_TEST
//#define DEBUG_DELETE_BALANCE


//#define DEBUG_AVL_DETAIL

int nodes;

struct storageTimelineCache {
	BLOCKINDEX timelineSector;
	FPI dirEntry[BLOCK_SIZE / sizeof( FPI )];
	struct dirent_cache caches[BLOCK_SIZE / sizeof( FPI )];
	//	struct dirent_cache caches[BLOCK_SIZE / sizeof( FPI )];
};

#define timelineBlockIndexNull 0


typedef union timelineBlockType {
	// 0 is invalid; indexes must subtract 1 to get
	// real timeline index.
	uint64_t raw;
	struct timelineBlockReference {
		uint64_t index;
	} ref;
} TIMELINE_BLOCK_TYPE;

// this is milliseconds since 1970 (unix epoc) * 256 + timezoneOffset /15 in the low byte
typedef struct timelineTimeType {
	int64_t tzOfs : 8;
	uint64_t tick : 56;
} TIMELINE_TIME_TYPE;

PREFIX_PACKED struct timelineHeader {
	TIMELINE_BLOCK_TYPE first_free_entry;
	TIMELINE_BLOCK_TYPE crootNode_deleted;
	TIMELINE_BLOCK_TYPE srootNode;
	uint64_t unused[5];
	//uint64_t unused2[8];
} PACKED;

// current size is 64 bytes.
// me_fpi is the physical FPI in the timeline file of the TIMELINE_BLOCK_TYPE that references 'this' block.
// structure defines little endian structure for storage.

PREFIX_PACKED struct storageTimelineNode {
	// if dirent_fpi == 0; it's free; and priorData will point at another free node
	uint64_t dirent_fpi;

	uint32_t filler32_1;
	uint16_t priorDataPad;
	uint8_t  filler8_1; // how much of the last block in the file is not used

	uint8_t  timeTz; // lesser least significant byte of time... sometimes can read time including timezone offset with time - 1 byte

	uint64_t time;

	uint64_t me_fpi; // it is know by  ( me_fpi & 0x3f ) == 32 or == 36 whether this is slesser or sgreater, (me_fpi & ~3f) = parent_fpi
	uint64_t priorData; // if not 0, references a start block version of data.
} PACKED;

struct memoryTimelineNode {
	// if dirent_fpi == 0; it's free.
	FPI this_fpi;
	uint64_t index;
	// the end of this is the same as storage timeline.

	struct storageTimelineNode* disk;
	enum block_cache_entries diskCache;
};


struct storageTimelineCursor {
	PDATASTACK parentNodes;  // save stack of parents in cursor
	struct storageTimelineCache dirents; // temp; needs work.
};

#define NUM_ROOT_TIMELINE_NODES (TIME_BLOCK_SIZE - sizeof( struct timelineHeader )) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimeline {
	struct timelineHeader header;
	struct storageTimelineNode entries[NUM_ROOT_TIMELINE_NODES];
} PACKED;

#define NUM_TIMELINE_NODES (TIME_BLOCK_SIZE) / sizeof( struct storageTimelineNode )
PREFIX_PACKED struct storageTimelineBlock {
	struct storageTimelineNode entries[(TIME_BLOCK_SIZE) / sizeof( struct storageTimelineNode )];
} PACKED;

#ifdef DEBUG_VALIDATE_TREE
#define VTReadOnly  , TRUE
#define VTReadWrite  , FALSE

#else
#define VTReadOnly
#define VTReadWrite
#endif


#ifdef _DEBUG
#define GRTENoLog ,0
#define GRTELog ,1
#else
#define GRTENoLog 
#define GRTELog 
#endif

#define convertMeToParentFPI(n) ((n)&~0x3f)
#define convertMeToParentIndex(n) (((n)>sizeof(struct timelineHeader))?( ( convertMeToParentFPI((n)&~0x3f)- sizeof( struct timelineHeader ) ) / sizeof( struct storageTimelineNode ) + 1 ):0)


struct storageTimelineNode* getRawTimeEntry( struct sack_vfs_os_volume* vol, uint64_t timeEntry, enum block_cache_entries *cache
#if _DEBUG
	, int log
#endif
	 DBG_PASS )
{
	int locks;
	cache[0] = BC( TIMELINE );
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, cache, TIME_BLOCK_SIZE DBG_SRC );

	locks = GETMASK_( vol->seglock, seglock, cache[0] );
#ifdef DEBUG_TEST_LOCKS
#  ifdef DEBUG_LOG_LOCKS
#    ifdef _DEBUG
	if( log )
#    endif
		_lprintf(DBG_RELAY)( "Lock %d %d %d", (int)timeEntry, cache[0], locks );
#  endif
	if( locks > 9 ) {
		lprintf( "Lock OVERFLOW" );
		DebugBreak();
	}
#endif
	locks++;
	
	SETMASK_( vol->seglock, seglock, cache[0], locks );

	return node;
}

TIMELINE_BLOCK_TYPE* getRawTimePointer( struct sack_vfs_os_volume* vol, uint64_t fpi, enum block_cache_entries *cache ) {
	cache[0] = BC( TIMELINE );
	return (TIMELINE_BLOCK_TYPE*)vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, fpi, cache, TIME_BLOCK_SIZE DBG_SRC );
}

void dropRawTimeEntry( struct sack_vfs_os_volume* vol, enum block_cache_entries cache
#if _DEBUG
	, int log
#endif
	 DBG_PASS ) {
	int locks;
	locks = GETMASK_( vol->seglock, seglock, cache );
#ifdef DEBUG_TEST_LOCKS
#  ifdef DEBUG_LOG_LOCKS
#    ifdef _DEBUG
	if( log )
#    endif
	_lprintf(DBG_RELAY)( "UnLock %d %d", cache, locks );
#  endif
	if( !locks ) {
		lprintf( "Lock UNDERFLOW" );
		DebugBreak();
	}
#endif
	locks--;
	SETMASK_( vol->seglock, seglock, cache, locks );
}

void reloadTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, uint64_t timeEntry
#ifdef DEBUG_VALIDATE_TREE
	, LOGICAL readOnly
#endif
#if _DEBUG
	, int log
#endif
	 DBG_PASS )
{
	enum block_cache_entries cache =
#ifdef DEBUG_VALIDATE_TREE
		readOnly ?BC(TIMELINE_RO):
#endif
		BC( TIMELINE );
	//uintptr_t vfs_os_FSEEK( struct sack_vfs_os_volume *vol, BLOCKINDEX firstblock, FPI offset, enum block_cache_entries *cache_index DBG_SRC ) {
	//if( timeEntry > 62 )DebugBreak();
	int locks;
	FPI pos = sane_offsetof( struct storageTimeline, entries[timeEntry - 1] );
	struct storageTimelineNode* node = ( struct storageTimelineNode* )vfs_os_FSEEK( vol, vol->timeline_file, 0/*no block*/, pos, &cache, TIME_BLOCK_SIZE DBG_RELAY );
	locks = GETMASK_( vol->seglock, seglock, cache );
#ifdef DEBUG_TEST_LOCKS
#ifdef DEBUG_LOG_LOCKS
#ifdef _DEBUG
	if( log )
#endif
		_lprintf(DBG_RELAY)( "Lock %d %d %d", (int)timeEntry, cache, locks );
#endif
	if( locks > 12 ) {
		lprintf( "Lock OVERFLOW" );
		DebugBreak();
	}
#endif
	locks++;
	SETMASK_( vol->seglock, seglock, cache, locks );

	time->disk = node;
	time->diskCache = cache;

	time->index = timeEntry;
	time->this_fpi = pos;

}


//-----------------------------------------------------------------------------------
// Timeline Support Functions
//-----------------------------------------------------------------------------------
void updateTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL drop DBG_PASS ) {
	SMUDGECACHE( vol, time->diskCache );
	if( drop ) {
		int locks;
		locks = GETMASK_( vol->seglock, seglock, time->diskCache );
#ifdef DEBUG_TEST_LOCKS
#ifdef DEBUG_LOG_LOCKS
		lprintf( "Unlock %d %d", time->diskCache, locks );
#endif
		if( !locks ) {
			lprintf( "Lock UNDERFLOW" );
			DebugBreak();
		}
#endif
		locks--;
		SETMASK_( vol->seglock, seglock, time->diskCache, locks );
	}
}

//---------------------------------------------------------------------------

void reloadDirectoryEntry( struct sack_vfs_os_volume* vol, struct memoryTimelineNode* time, struct sack_vfs_os_find_info* decoded_dirent DBG_PASS ) {
	enum block_cache_entries cache = BC( DIRECTORY );
	struct directory_entry* dirent;// , * entkey;
	struct directory_hash_lookup_block* dirblock;
	//struct directory_hash_lookup_block* dirblockkey;
	PDATASTACK pdsChars = CreateDataStack( 1 );
	BLOCKINDEX this_dir_block = (time->disk->dirent_fpi >> BLOCK_BYTE_SHIFT);
	BLOCKINDEX next_block;
	dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, this_dir_block, DIR_BLOCK_SIZE, cache );
	//dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[cache];
	dirent = (struct directory_entry*)(((uintptr_t)dirblock) + (time->disk->dirent_fpi & BLOCK_SIZE));
	//entkey = (struct directory_entry*)(((uintptr_t)dirblockkey) + (time->dirent_fpi & BLOCK_SIZE));

	decoded_dirent->vol = vol;

	// all of this regards the current state of a find cursor...
	decoded_dirent->base = NULL;
	decoded_dirent->base_len = 0;
	decoded_dirent->mask = NULL;
	decoded_dirent->pds_directories = NULL;

	decoded_dirent->filesize = (size_t)( dirent->filesize );
	if( time->disk->priorData ) {
		enum block_cache_entries cache;
		struct storageTimelineNode* prior = getRawTimeEntry( vol, time->disk->priorData, &cache GRTENoLog DBG_SRC );
		while( prior->priorData ) {
			dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
			prior = getRawTimeEntry( vol, prior->priorData, &cache GRTENoLog DBG_RELAY );
		}
		decoded_dirent->ctime = prior->time;
		dropRawTimeEntry( vol, cache GRTENoLog DBG_RELAY );
	}
	else
		decoded_dirent->ctime = time->disk->time;
	decoded_dirent->wtime = time->disk->time;

	while( (next_block = dirblock->next_block[DIRNAME_CHAR_PARENT]) ) {
		enum block_cache_entries back_cache = BC( DIRECTORY );
		struct directory_hash_lookup_block* back_dirblock;
		back_dirblock = BTSEEK( struct directory_hash_lookup_block*, vol, next_block, DIR_BLOCK_SIZE, back_cache );
		//back_dirblockkey = (struct directory_hash_lookup_block*)vol->usekey[back_cache];
		int i;
		for( i = 0; i < DIRNAME_CHAR_PARENT; i++ ) {
			if( (back_dirblock->next_block[i]) == this_dir_block ) {
				PushData( &pdsChars, &i );
				break;
			}
		}
		if( i == DIRNAME_CHAR_PARENT ) {
			// directory didn't have a forward link to it?
			DebugBreak();
		}
		this_dir_block = next_block;
		dirblock = back_dirblock;
	}

	char* c;
	int n = 0;
	// could fill leadin....
	decoded_dirent->leadin[0] = 0;
	decoded_dirent->leadinDepth = 0;
	while( c = (char*)PopData( &pdsChars ) )
		decoded_dirent->filename[n++] = c[0];
	DeleteDataStack( &pdsChars );

	{
		BLOCKINDEX nameBlock;
		nameBlock = dirblock->names_first_block;
		FPI name_offset = (dirent[n].name_offset ) & DIRENT_NAME_OFFSET_OFFSET;

		enum block_cache_entries cache = BC( NAMES );
		const char* dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset, &cache, NAME_BLOCK_SIZE DBG_SRC );
		const char* dirname_ = dirname;
		//const char* dirkey = (const char*)(vol->usekey[cache]) + (name_offset & BLOCK_MASK);
		const char* prior_dirname = dirname;
		int c;
		do {
			while( (((unsigned char)(c = (dirname[0] )) != UTF8_EOT))
				&& ((((uintptr_t)prior_dirname) & ~BLOCK_MASK) == (((uintptr_t)dirname) & ~BLOCK_MASK))
				) {
				decoded_dirent->filename[n++] = c;
				dirname++;
				//dirkey++;
			}
			if( ((((uintptr_t)prior_dirname) & ~BLOCK_MASK) != (((uintptr_t)dirname) & ~BLOCK_MASK)) ) {
				int partial = (int)(dirname - dirname_);
				cache = BC( NAMES );
				dirname = (const char*)vfs_os_FSEEK( vol, NULL, nameBlock, name_offset + partial, &cache, NAME_BLOCK_SIZE DBG_SRC );
				//dirkey = (const char*)(vol->usekey[cache]) + ((name_offset + partial) & BLOCK_MASK);
				dirname_ = dirname - partial;
				prior_dirname = dirname;
				continue;
			}
			// didn't stop because it exceeded a sector boundary
			break;
		} while( 1 );
	}
	decoded_dirent->filename[n] = 0;
	decoded_dirent->filenamelen = n;
	//time->dirent_fpi

}

//---------------------------------------------------------------------------

static void deleteTimelineIndex( struct sack_vfs_os_volume* vol, BLOCKINDEX index ) {
	BLOCKINDEX next;
	do {
		struct storageTimelineNode* time;
		enum block_cache_entries cache = BC( TIMELINE );

		//lprintf( "Delete start... %d", index );
		time = getRawTimeEntry( vol, index, &cache GRTELog DBG_SRC );
		next = (BLOCKINDEX)time->priorData; // this type is larger than index in some configurations
		nodes--;

		{
			struct storageTimeline* timeline = vol->timeline;
			time->priorData = timeline->header.first_free_entry.ref.index;
			timeline->header.first_free_entry.ref.index = index;
			SMUDGECACHE( vol, vol->timelineCache );
			SMUDGECACHE( vol, cache );
		}

		dropRawTimeEntry( vol, cache GRTELog DBG_SRC );
#ifdef DEBUG_VALIDATE_TREE
		//ValidateTimelineTree( vol DBG_SRC );
#endif
		//lprintf( "Delete done... %d", index );
	} while( index = next );
#ifdef DEBUG_DELETE_LAST
	checkRoot( vol );
#endif
	if( !nodes && vol->timeline->header.srootNode.ref.index ) {
		lprintf( "No more nodes, but the root points at something." );
		DebugBreak();
	}
	//lprintf( "Root is now %d %d", nodes, vol->timeline->header.srootNode.ref.index );
}

BLOCKINDEX getTimeEntry( struct memoryTimelineNode* time, struct sack_vfs_os_volume* vol, LOGICAL unused, void(*init)(uintptr_t, struct memoryTimelineNode*), uintptr_t psv DBG_PASS ) {
	enum block_cache_entries cache = BC( TIMELINE );
	enum block_cache_entries cache_last = BC( TIMELINE );
	enum block_cache_entries cache_free = BC( TIMELINE );
	enum block_cache_entries cache_new = BC( TIMELINE );
	struct storageTimeline* timeline = vol->timeline;
	TIMELINE_BLOCK_TYPE freeIndex;
	BLOCKINDEX index;
	BLOCKINDEX priorIndex = (BLOCKINDEX)time->index; // ref.index type is larger than index in some configurations; but won't exceed those bounds

	freeIndex.ref.index = timeline->header.first_free_entry.ref.index;

	// update next free.
	reloadTimeEntry( time, vol, index = (BLOCKINDEX)freeIndex.ref.index VTReadWrite GRTELog DBG_RELAY ); // ref.index type is larger than index in some configurations; but won't exceed those bounds

	timeline->header.first_free_entry.ref.index = timeline->header.first_free_entry.ref.index + 1;

	SMUDGECACHE( vol, vol->timelineCache );

	// make sure the new entry is emptied.
	time->disk->me_fpi = 0;
	time->disk->dirent_fpi = 0;
	time->disk->priorData = 0;
	time->disk->time = timeGetTime64ns();

	{
		int tz = GetTimeZone();
		if( tz < 0 )
			tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
		else
			tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
		time->disk->time += (int64_t)tz * 900 * (int64_t)1000000000;
		time->disk->timeTz = tz;
	}

	if( init ) init( psv, time );
	nodes++;
	//lprintf( "Add start... %d", freeIndex.ref.index );
#if defined( DEBUG_TIMELINE_DIR_TRACKING) || defined( DEBUG_TIMELINE_AVL )
	LoG( "Return time entry:%d", time->index );
#endif
	updateTimeEntry( time, vol, FALSE DBG_RELAY ); // don't drop; returning this one.

	return index;
}

BLOCKINDEX updateTimeEntryTime( struct memoryTimelineNode* time
			, struct sack_vfs_os_volume *vol, uint64_t index
			, LOGICAL allocateNew
			, void( *init )( uintptr_t, struct memoryTimelineNode* ), uintptr_t psv DBG_PASS ) {
	if( allocateNew ) {
		if( time ) {
			uint64_t inputIndex = time ? time->index : index;
			// gets a new timestamp.
			enum block_cache_entries inputCache = time ? time->diskCache : BC( ZERO );
			BLOCKINDEX newIndex = getTimeEntry( time, vol, TRUE, init, psv DBG_RELAY );
			time->disk->priorData = inputIndex;
			updateTimeEntry( time, vol, FALSE DBG_RELAY );
			dropRawTimeEntry( vol, inputCache GRTELog DBG_RELAY );
			return newIndex;
		}
		else {
			struct memoryTimelineNode time_;
			struct storageTimelineNode* timeold;
			uint64_t inputIndex = index;
			enum block_cache_entries inputCache;
			FPI dirent_fpi;
			timeold = getRawTimeEntry( vol, index, &inputCache GRTELog DBG_RELAY );
			dirent_fpi = (FPI)timeold->dirent_fpi; // ref.index type is larger than index in some configurations; but won't exceed those bounds
			dropRawTimeEntry( vol, inputCache GRTELog DBG_RELAY );

			// gets a new timestamp.
			time_.index = index;
			BLOCKINDEX newIndex = getTimeEntry( &time_, vol, TRUE, init, psv DBG_RELAY );
			time_.disk->priorData = inputIndex;
			time_.disk->dirent_fpi = dirent_fpi;
			updateTimeEntry( &time_, vol, TRUE DBG_RELAY );
			return newIndex;
		}
	}
	else {
		struct memoryTimelineNode time_;
		if( !time ) time = &time_;
		reloadTimeEntry( time, vol, index VTReadWrite GRTENoLog DBG_RELAY );
		time->disk->time = timeGetTime64ns();
		{
			int tz = GetTimeZone();
			if( tz < 0 )
				tz = -( ( ( -tz / 100 ) * 60 ) + ( -tz % 100 ) ) / 15; // -840/15 = -56
			else
				tz = ( ( ( tz / 100 ) * 60 ) + ( tz % 100 ) ) / 15; // -840/15 = -56  720/15 = 48
			time->disk->time += (int64_t)tz * 900 * (int64_t)1000000000;
			time->disk->timeTz = tz;
		}
		updateTimeEntry( time, vol, FALSE DBG_RELAY );
		return (BLOCKINDEX)index; // index type is larger than index in some configurations; but won't exceed those bounds
	}
}
