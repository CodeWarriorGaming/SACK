//#include <stdhdrs.h>
#undef _DEBUG
#ifdef _WIN32
#include <windows.h>
#endif

#define MEM_LIBRARY_SOURCE
#ifdef __LINUX__
#define Relinquish() sched_yield()
#define GetCurrentProcessId getpid
#define GetCurrentThreadId getpid
#define GetLastError() errno
#else
#define Relinquish() Sleep(0)
#endif

#include "./types.h"
#include <stddef.h>
#include <stdio.h>

//#include <logging.h>
#include "sharestruc.h"
#include "sharemem.h"


#ifdef __LINUX__
#include <fcntl.h>
#include <sys/mman.h>
#define HANDLE int
#endif

static int nMinAllocateSize;

#ifdef _DEBUG
// may define one or the other of these but NOT both
//#define STRONG_DIAGNOSTIC // logs allocs, holds, and release ordered queue...
//#define MEM_TRACKING  // logs allocs, holds, and releases with an accounthing list
int bDisableDebug;
int bLogCritical;
#endif

//TEXTCHAR byDebugOutput[256];
/*
// expression form breaks preprocessor - need to keep at least this example...
#ifdef _DEBUG
#define ODSEx(s,pFile,nLine) ( Log6( DBG_FILELINEFMT "[%03lX:%03lX] [%ld]: %s", pFile, nLine, GetCurrentProcessId(), GetCurrentThreadId(), GetLastError(), s ) )
#else
#define ODSEx(s,pFile,nLine) ( Log4( "[%03lX:%03lX] [%ld]: %s",GetCurrentProcessId(), GetCurrentThreadId(), GetLastError(), s ) )
#endif
#define ODS(s) ODSEx(s,__FILE__, __LINE__ )
*/
#ifdef _DEBUG
#define ODSEx(s,pFile,nLine) //Log6( DBG_FILELINEFMT "[%03lX:%03lX] [%ld]: %s", pFile, nLine ,GetCurrentProcessId(), GetCurrentThreadId(), GetLastError(), s )
#else
#define ODSEx(s,pFile,nLine) //Log4( "[%03lX:%03lX] [%ld]: %s",GetCurrentProcessId(), GetCurrentThreadId(), GetLastError(), s )
#endif
#define ODS(s)   //Log4( "[%03lX:%03lX] [%ld]: %s", GetCurrentProcessId(), GetCurrentThreadId(), GetLastError(), s )


#define Log1(a,b)  //fprintf( stderr, a, b )
#define Log2(a,b,c) //fprintf( stderr, a, b,c )
#define Log3(a,b,c,d) //fprintf( stderr, a, b,c,d )
#define Log4(a,b,c,d,e) //fprintf( stderr, a, b,c,d,e )
#define Log5(a,b,c,d,e,f) //fprintf( stderr, a, b,c,d,e,f )
#define Log(a) //fprintf( stderr, "%s", a )

//MEM_PROC( void DebugDumpMemEx( LOGICAL bVerbose );

// minimum allocation granularity is somewhere between
// one sector size and one memory grain(FILE_GRAN)

#ifndef _WIN32
#include <errno.h>
// threads and PIDs share same name space in *nix
#endif


//-------------------------------------------------------------------------
#ifndef HAS_ASSEMBLY
MEM_PROC( uint32_t, LockedExchange )( uint32_t* p, uint32_t val )
{
	if( p )
	{
		uint32_t prior = *p;
		*p = val;
		return prior;
	}
   return 0;
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MEM_PROC( void, MemSet )( POINTER p, uint32_t n, uint32_t sz )
{
#ifdef _MSC_VER
      _asm mov ecx, sz;
      _asm mov eax, n;
      _asm mov edi, p;
      _asm cld;
      _asm mov   edx, ecx;
      _asm shr 	ecx, 2;
      _asm rep 	stosd;
      _asm test 	edx, 2
      _asm jz 	store_one;
      _asm stosw;
store_one:
      _asm test 	edx,1
      _asm jz 	store_none;
      _asm stosb;
store_none: ;
#else
   memset( p, n, sz );
#endif
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

MEM_PROC( void, MemCpy )( POINTER pTo, CPOINTER pFrom, uint32_t sz )
{
#ifdef _MSC_VER
      _asm mov   ecx, sz;
      _asm mov   edi, pTo;
      _asm mov   esi, pFrom;
      _asm cld;
      _asm mov   edx, ecx;
      _asm shr   ecx, 2;
      _asm rep   movsd;
      _asm test  edx,1;
      _asm jz    test_2;
      _asm movsb;
test_2:
      _asm test edx, 2;
      _asm jz test_end;
      _asm movsw;
test_end: ;
#else
	memcpy( pTo, pFrom, sz );
#endif
}

MEM_PROC( int, MemCmp )( CPOINTER pOne, CPOINTER pTwo, uint32_t sz )
{
   return memcmp( pOne, pTwo, sz );
}
#else
#ifdef __STATIC__
//extern uint32_t  LockedExchange( uint32_t* p, uint32_t val );
extern void MemSet( POINTER p, uint32_t n, uint32_t sz );
extern void MemCpy( POINTER to, CPOINTER from, uint32_t sz );
extern int MemCmp( CPOINTER p1, CPOINTER p2, uint32_t sz );
#else
//MEM_PROC(  uint32_t,  LockedExchange )( uint32_t* p, uint32_t val );
MEM_PROC(  void, MemSet )( POINTER p, uint32_t n, uint32_t sz );
MEM_PROC(  void, MemCpy )( POINTER to, CPOINTER from, uint32_t sz );
MEM_PROC(  int, MemCmp )( CPOINTER to, CPOINTER from, uint32_t sz );
#endif
#endif

//-------------------------------------------------------------------------
MEM_PROC( uint32_t, LockedIncrement )( uint32_t* p)
{
    if(p)
        return (*p)++;
    return 0;
}
//-------------------------------------------------------------------------
MEM_PROC( uint32_t, LockedDecrement )( uint32_t* p )
{
    if(p)
        return (*p)--;
    return 0;
}
//-------------------------------------------------------------------------

LOGICAL EnterCriticalSecNoWaitEx( PCRITICALSECTION pcs DBG_PASS )
{
   unsigned long dwCurProc;
   dwCurProc = (GetCurrentProcessId() << 16) | (GetCurrentThreadId() & 0xffff);
#ifdef _DEBUG
   if( bLogCritical > 1 )
       Log5( "%s(%ld) [%08lx:%08lx] Attempt enter critical Section %08lx", pFile, nLine
            , GetCurrentProcessId(), GetCurrentThreadId()
            , pcs->dwLocks );
#endif
   // need to aquire lock on section...
   // otherwise our old mechanism allowed an enter in another thread
   // to falsely identify the section as its own while the real owner
   // tried to exit...
//   if( LockedExchange( &pcs->dwUpdating, 1 ) )
//       return FALSE;
   if( !pcs->dwThread || dwCurProc == pcs->dwThread )
   {
       pcs->dwLocks++;
#ifdef _DEBUG
       pcs->pFile = pFile;
       pcs->nLine = nLine;
#endif
       pcs->dwThread = dwCurProc;
       pcs->dwUpdating = 0;
       return TRUE;
   }
   else // if( pcs->dwThread ) ... and it's not me
   {
       if( !(pcs->dwLocks & SECTION_LOGGED_WAIT) )
       {
#ifdef _DEBUG
			 if( bLogCritical )
			 {
           char msg[256];
           sprintf( msg, "Waiting on critical section owned by %s(%d).", (pcs->pFile)?(pcs->pFile):"Unknown", pcs->nLine );
			  ODSEx( msg, pFile, nLine );
			 }
#endif
           pcs->dwLocks |= SECTION_LOGGED_WAIT;
       }
       pcs->dwUpdating = 0;
   }
   return FALSE;
}

LOGICAL EnterCriticalSecEx( PCRITICALSECTION pcs DBG_PASS )
{
    while( !EnterCriticalSecNoWaitEx( pcs DBG_RELAY ) )
        Relinquish();
    return TRUE;
}
//-------------------------------------------------------------------------

LOGICAL LeaveCriticalSecEx( PCRITICALSECTION pcs DBG_PASS )
{
   unsigned long dwCurProc;
   //Log( "Leave critical Sec - begin " );
#ifdef _DEBUG
	if( bLogCritical > 1 )
	{
		Log5( "%s(%ld) [%08lx:%08lx] Leave critical Section(1) %08lx", pFile, nLine
				, GetCurrentProcessId(), GetCurrentThreadId()  
				, pcs->dwLocks ); 
	}
#endif
//	while( LockedExchange( &pcs->dwUpdating, 1 ) )
//		Relinquish();

   dwCurProc = (GetCurrentProcessId() << 16) | (GetCurrentThreadId() & 0xFFFF);

   if( !pcs->dwLocks )
   {
#ifdef _DEBUG
      ODSEx( "Leaving a blank critical section", pFile, nLine );
#else
      ODS( "Leaving a blank critical section" );
#endif
      //while( 1 );
		pcs->dwUpdating = 0;
      return FALSE;
   }

   if( pcs->dwThread == dwCurProc )
   {
#ifdef _DEBUG
		pcs->pFile = pFile;
		pcs->nLine = nLine;
#endif
      pcs->dwLocks--;
      if( pcs->dwLocks & SECTION_LOGGED_WAIT )
      {
#ifdef _DEBUG
			 if( bLogCritical )
			 {
				 ODSEx( "Releasing critical section being waited for.", pFile, nLine );
			 }
#endif
          if( !( pcs->dwLocks & ~(SECTION_LOGGED_WAIT) ) )
          {
              pcs->dwLocks = 0;
              pcs->dwThread = 0;
              Relinquish();
          }
      }
      else
      {
	      if( !pcs->dwLocks )
      	   pcs->dwThread = 0;
      }
   }
   else
   {
#ifdef _DEBUG
		{
			char msg[256];
			sprintf( msg, "Sorry - you can't leave a section owned by %08lx %08lx %s(%d)..."
							, pcs->dwThread
							, pcs->dwLocks
							, (pcs->pFile)?(pcs->pFile):"Unknown", pcs->nLine );
			ODSEx( msg, pFile, nLine );
		}
#else
      ODS( "Sorry - you can't leave a section you don't own..." );
#endif
		pcs->dwUpdating = 0;
      return FALSE;
   }
   // allow other locking threads immediate access to section
	// but I know when that happens - since the waiting process
	// will flag - SECTION_LOGGED_WAIT
   //Relinquish();
	pcs->dwUpdating = 0;
   return TRUE;
}

//-------------------------------------------------------------------------

void InitializeCriticalSec( PCRITICALSECTION pcs )
{
   pcs->dwUpdating = 0;
   pcs->dwLocks = 0;
   pcs->dwThread = 0;
   return;
}

void DeleteCriticalSec( PCRITICALSECTION pcs )
{
   return;
}

//-------------------------------------------------------------------------

#ifdef _DEBUG

enum {
   LOG_ALLOC,
   LOG_RELEASE,
   LOG_GRAB,
   LOG_DROP
};


typedef struct internal_debug_log {
   int nType;
   POINTER p;
   uint32_t dwSize;
   uint32_t dwID;
   uint32_t dwTime;
   uint32_t nLine;
   TEXTSTR pFile;
} DLOG, *PDLOG;

#if defined(STRONG_DIAGNOSTIC) || defined(MEM_TRACKING)

struct debug_log {
	int  nLogPos; // next position to update;
	DLOG DebugLog[64000];
} *pDebugLog;


#ifdef MEM_TRACKING
void TRACK_ALLOC( void *pt, int sz, char *pFile, int nLine )
{
   int pos, max;
   max = pDebugLog->nLogPos;
   for( pos = 0; pos < max; pos++ )
   {
      if( !pDebugLog->DebugLog[pos].nType
         ) // some deallocated thing...
         break;
   }
   pDebugLog->DebugLog[pos].dwSize = sz;
   pDebugLog->DebugLog[pos].nLine = nLine;
   pDebugLog->DebugLog[pos].pFile = pFile;
   pDebugLog->DebugLog[pos].p = pt;
   pDebugLog->DebugLog[pos].dwID = (GetCurrentProcessId() << 16) | GetCurrentThreadId();
   pDebugLog->DebugLog[pos].nType = 1;
   pDebugLog->DebugLog[pos].dwTime = GetTickCount();
   if( pos == max )
      pDebugLog->nLogPos = pos + 1;
}
void TRACK_HOLD( void *pt, char *pFile, int nLine )
{
   int pos, max;
   max = pDebugLog->nLogPos;
   for( pos = 0; pos < max; pos++ )
   {
      if( pDebugLog->DebugLog[pos].p == pt )
         break;
   }
   if( pos == max )
   {
      ODS( "Failed to find allocated reference..." );
      DebugBreak();
   }
   else
   {
      pDebugLog->DebugLog[pos].nType++;
   }
}
void TRACK_DEALLOC( void *pt, char *pFile, int nLine )
{
   int pos, max;
   max = pDebugLog->nLogPos;
   for( pos = 0; pos < max; pos++ )
   {
      if( pDebugLog->DebugLog[pos].p == pt )
         break;
   }
   if( pos == max )
   {
      ODS( "Failed to find allocation reference..." );
      DebugBreak();
   }
   else
   {
      if( (pDebugLog->DebugLog[pos].nType) )
      {
         pDebugLog->DebugLog[pos].nType--;
         //if( !pDebugLog->DebugLog[pos].nType )
         //   pDebugLog->DebugLog[pos].p = NULL;
         pDebugLog->DebugLog[pos].nLine = nLine;
         pDebugLog->DebugLog[pos].pFile = pFile;
         pDebugLog->DebugLog[pos].dwID = (GetCurrentProcessId() << 16) | GetCurrentThreadId();
      }
      else
      {
         ODS( "This block was already deallocated..." ); // check block for prior releaser...
         DebugBreak();
      }
   }
}
uint32_t lastdumptime;
void TRACK_DUMP( void )
{
   int n, max;
   max = pDebugLog->nLogPos;
   for( n = 0; n < max; n++ )
   {
      if( pDebugLog->DebugLog[n].p &&
          pDebugLog->DebugLog[n].dwTime >= lastdumptime)
      {
         TEXTCHAR msg[256];
         sprintf( msg, "%9d[%08x]: (%08x)%d",
                     pDebugLog->DebugLog[n].dwTime,
                     pDebugLog->DebugLog[n].dwID,
                     pDebugLog->DebugLog[n].p,
                     pDebugLog->DebugLog[n].dwSize );
         ODSEx( msg, pDebugLog->DebugLog[n].pFile, pDebugLog->DebugLog[n].nLine );
         Relinquish();
      }
   }
   lastdumptime = GetTickCount();
}

#else
#define TRACK_ALLOC(pt,sz,pFile,nLine)    (0)
#define TRACK_HOLD(pt,sz,nLine)           (0)
#define TRACK_DEALLOC(pt,sz,nLine)        (0)
#define TRACK_DUMP()                      (0)
#endif

#define LOG_ALLOCATE( p, sz, pFile, nLine )
#define LOG_RELEASE( p, pFile, nLine )
#define LOG_GRAB(f,l)
#define LOG_DROP(f,l)
   void WriteDebugLog(void) {}

#endif // TRACKING || DIAGNOSTIC

#endif // _DEBUG

LOGICAL bInit;


#ifdef _WIN32
SYSTEM_INFO si;
               //(0x10000 * 0x1000) //256 megs?
#define FILE_GRAN si.dwAllocationGranularity
#else
int pagesize;
#define FILE_GRAN pagesize
#endif

#define BASE_MEMORY (POINTER)0x80000000
// golly allocating a WHOLE DOS computer to ourselves? how RUDE
uint32_t dwSystemCapacity = 0x10000 * 0x08;  // 512K ! 1 meg... or 16 :(
#define SYSTEM_CAPACITY  dwSystemCapacity


#define CHUNK_SIZE ( offsetof( CHUNK, byData ) )
#define MEM_SIZE  ( offsetof( MEM, pRoot ) )

//-----------------------------------------------------------------

#ifdef __LINUX__
#define WINAPI
typedef uint32_t HINSTANCE;
#endif
// last entry in space tracking array will ALWAYS be
// another space tracking array (if used)
// (32 bytes)
typedef struct space_tracking_structure {
   POINTER pMem;
#ifdef _WIN32
   HANDLE  hFile;
   HANDLE  hMem;
#else
   int hFile;
#endif
   uint32_t dwSmallSize;
   uint32_t dwSmallIndex;
   struct space_tracking_structure *pNext
                                 , **me;
   struct space_tracking_structure *expansion;
} SPACE, *PSPACE;

static PSPACE pSpacePool;
static PSPACE pFirst;
#ifdef _DEBUG
static uint32_t dwBlocks; // last values from getmemstats...
static uint32_t dwFreeBlocks;
static uint32_t dwAllocated;
static uint32_t dwFree;
#endif
#define MAX_PER_BLOCK 128 // 1 page blocks ...

//------------------------------------------------------------------------------------------------------
static LOGICAL bLogAllocate;

void InitSharedMemory( void )
{
    if( !bInit )
    {
#ifdef _WIN32
        GetSystemInfo( &si );
#else
        pagesize = getpagesize();
#endif
        Log2( "CHUNK: %d  MEM:%d", CHUNK_SIZE, MEM_SIZE );
        bInit = TRUE;  // onload was definatly a zero.
        {
            uint32_t dwSize = MAX_PER_BLOCK * sizeof( SPACE );
            pSpacePool = OpenSpace( NULL, NULL, &dwSize );
            if( pSpacePool )
            {
                MemSet( pSpacePool, 0, dwSize );
                Log1( "Allocated Space pool %lu", dwSize );
            }
        }
    }
    else
    {
        ODS( "already initialized?" );
     }
}

//------------------------------------------------------------------------------------------------------

#if !defined(__STATIC__) && defined( _WIN32 )
// must keep LibMain for LCC-Win32
//LIBMAIN()
//{
//   return 1;
//}
//LIBEXIT()
//{
//	return 1;
//}
//LIBMAIN_END();
#endif

//------------------------------------------------------------------------------------------------------
// private
static PSPACE AddSpace( PSPACE pSpacePool, HANDLE hFile, HANDLE hMem, POINTER pMem, uint32_t dwSize, int bLink )
{
   PSPACE ps, _ps;
   static int InAdding; // don't add our tracking to ourselves...
   int i;
   if( !pSpacePool || InAdding )
   {
       Log2( "No space pool(%p) or InAdding(%d)", pSpacePool, InAdding );
       return NULL;
   }
   InAdding = 1;
   _ps = NULL;
   ps = pSpacePool;
Retry:
   do {
      for( i = 0; i < (MAX_PER_BLOCK-1); i++ )
      {
         if( !ps[i].pMem )
         {
            ps += i;
            break;
         }
      }
      if( i == (MAX_PER_BLOCK-1) )
      {
         _ps = ps;
         ps = ps[i].expansion;
      }
      else
         break;
   } while( ps );

   if( !ps )
   {
      //DebugBreak(); // examine conditions for allocating new space block...
      dwSize = sizeof(SPACE) * MAX_PER_BLOCK;
      if( _ps )
      {
      	 _ps[(MAX_PER_BLOCK-1)].pMem = (POINTER)1;
         ps = _ps[(MAX_PER_BLOCK-1)].expansion = (PSPACE)OpenSpace( NULL, NULL, &dwSize );
      }
      MemSet( ps, 0, dwSize );
      goto Retry;
   }
   //Log7( "Managing space (s)%p (pm)%p (hf)%08lx (hm)%08lx (sz)%ld %08lx-%08lx"
   //				, ps, pMem, (uint32_t)hFile, (uint32_t)hMem, dwSize
   //            , (uint32_t)pMem, ((uint32_t)pMem + dwSize)
   //				);
   ps->pMem = pMem;
   ps->hFile = hFile;
#ifdef _WIN32
   ps->hMem = hMem;
#endif
   ps->dwSmallSize = dwSize;
   if( bLink )
   {
   	PSPACE AddAfter;
      AddAfter = pFirst;
   	while( AddAfter && AddAfter->pNext )
			AddAfter = AddAfter->pNext;
   	//Log2( "Linked into space...%p after %p ", ps, AddAfter );
		if( AddAfter )
		{
   	   ps->me = &AddAfter->pNext;
   	   AddAfter->pNext = ps;
   	}
   	else
   	{
   		ps->me = &pFirst;
   		pFirst = ps;
   	}
  	   ps->pNext = NULL;
   }
   InAdding = 0;
   return ps;
}

//------------------------------------------------------------------------------------------------------

void DumpSpaces( void )
{
	PSPACE ps = pSpacePool;
	while( ps )
	{
		Log3( "Space: %p mem: %p-%p", ps, ps->pMem, (uint8_t*)ps->pMem + ps->dwSmallSize );
		if( ps->expansion )
		{
         PSPACE pe = ps->expansion;
			while( pe )
			{
				Log4( "expanded space: %p mem: %p-%p(%d)"
					 , pe
					 , pe->pMem, (uint8_t*)pe->pMem + pe->dwSmallSize
					 , pe->dwSmallSize );
				pe = pe->expansion;
			}
		}

      ps = ps->pNext;
	}
}

//------------------------------------------------------------------------------------------------------


//#define DEBUG_FIND_SPACE

PSPACE FindSpace( POINTER pMem )
{
   PSPACE ps;
	PSPACE pes;
	ps = pFirst;
   while( pes = ps )
	{
		while( pes )
		{
			if( pes->pMem == (POINTER)1 )
			{
				// space block continuation expansion....
				Log( "UNTESTED SPACE EXTENSION!" );
				ps = ps->expansion;
				continue;
			}
			if( pes->pMem == pMem )
			{
				return pes;
			}
         pes = pes->expansion;
		}
      ps = ps->pNext;
   }
   return NULL;
}

//------------------------------------------------------------------------------------------------------

static void DoCloseSpace( PSPACE ps )
{
   if( ps )
   {
       //Log( "Closing a space..." );
#ifdef _WIN32
      UnmapViewOfFile( ps->pMem );
      CloseHandle( ps->hMem );
      CloseHandle( ps->hFile );
#else
      munmap( ps->pMem, ps->dwSmallSize );
		close( (int)ps->pMem );
		if( ps->hFile >= 0 )
         close( ps->hFile );
#endif
      if( ps->me )
      {
          *ps->me = ps->pNext;
          if( ps->pNext )
              ps->pNext->me = ps->me;
      }
      else
      {
          Log( "UNHANDLED! Unlink space which is an expansion!" );
      }
      MemSet( ps, 0, sizeof( SPACE ) );
   }
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, CloseSpace )( POINTER pMem )
{
   DoCloseSpace( FindSpace( pMem ) );
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( uint32_t, GetSpaceSize )( POINTER pMem )
{
   PSPACE ps;
   ps = FindSpace( pMem );
   if( ps )
      return ps->dwSmallSize;
   return 0;
}

#ifdef __LINUX__
uint32_t GetFileSize( int fd )
{
    uint32_t len = lseek( fd, 0, SEEK_END );
    lseek( fd, 0, SEEK_SET );
    return len;
}

#endif
//------------------------------------------------------------------------------------------------------
MEM_PROC( POINTER, OpenSpaceEx )( TEXTSTR pWhat, TEXTSTR pWhere, uint32_t address, uint32_t* dwSize )
{
   HANDLE hFile;
   HANDLE hMem;
   POINTER pMem = NULL;

   if( !bInit )
   {
       //ODS( "Doing Init" );
       InitSharedMemory();
   }
#ifdef __LINUX__
   {
       int fd = -1;
		 int exists = FALSE
			 , readonly = FALSE;
       if( !pWhere && !pWhere)
       {
           pMem = mmap( 0, *dwSize
                , PROT_READ|PROT_WRITE
                , MAP_SHARED|MAP_ANONYMOUS
                , 0, 0 );
       }

       if( !pMem )
		 {
          mode_t prior;
			 prior = umask( 0 );
			 fd = open( pWhere, O_RDWR|O_CREAT|O_EXCL, 0777 );
          umask(prior);
           if( fd == -1 )
           {
               if( GetLastError() == EEXIST )
               {
                   exists = TRUE;
                   fd = open( pWhere, O_RDWR );
               }
               if( fd == -1 )
					{
                  readonly = TRUE;
						fd = open( pWhere, O_RDONLY );
					}
					if( fd == -1 )
					{
						Log2( "Sorry - failed to open: %d %s", errno, pWhere);
                   return NULL;
               }
           }
           if( exists )
           {
               if( GetFileSize( fd ) < (uint32_t)*dwSize )
					{
                  // expands the file...
                   ftruncate( fd, *dwSize );
                   //*dwSize = ( ( *dwSize + ( FILE_GRAN - 1 ) ) / FILE_GRAN ) * FILE_GRAN;
               }
               else
					{
                  // expands the size requested to that of the file...
                   *dwSize = GetFileSize( fd );
               }
           }
           else
           {
               if( !*dwSize )
					{
                  // can't create a 0 sized file this way.
                   *dwSize = 1; // not zero.
                   close( fd );
                   unlink( pWhere );
                   return NULL;
               }
               //*dwSize = ( ( *dwSize + ( FILE_GRAN - 1 ) ) / FILE_GRAN ) * FILE_GRAN;
               ftruncate( fd, *dwSize );
           }
           pMem = mmap( 0, *dwSize
							 , PROT_READ|(readonly?(0):PROT_WRITE)
							 , MAP_SHARED|((fd<0)?MAP_ANONYMOUS:0)
                       , fd, 0 );

       }
       if( pMem )
       {
           //ODS( "Adding a space" );
           AddSpace( pSpacePool, (HANDLE)fd, 0, pMem, *dwSize, TRUE );
       }
   }
   return pMem;

#endif
#ifdef _WIN32
   if( !pWhat && !pWhere )
   {
      hMem = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, *dwSize, NULL );
      if( !hMem )
      {
      	ODS( "Failed to allocate pagefile memory?!" );
      }
   }
   else
      hMem = OpenFileMapping( FILE_MAP_READ|FILE_MAP_WRITE
                       , FALSE
                       , pWhat );

   hFile = INVALID_HANDLE_VALUE;

   if( !hMem )
   {
      hFile = CreateFile( pWhere, GENERIC_READ|GENERIC_WRITE
               ,FILE_SHARE_READ|FILE_SHARE_WRITE//|FILE_SHARE_DELETE
               ,NULL // default security
               ,OPEN_ALWAYS
               ,FILE_ATTRIBUTE_NORMAL //|FILE_ATTRIBUTE_TEMPORARY
                //| FILE_FLAG_WRITE_THROUGH
                //| FILE_FLAG_NO_BUFFERING
                  // must access on sector bournds
                  // must read complete sectors
                  //| FILE_FLAG_DELETE_ON_CLOSE
                , NULL );

      if( hFile == INVALID_HANDLE_VALUE )
      {
	      hMem = CreateFileMapping( INVALID_HANDLE_VALUE
   	                     , NULL
      	                  , PAGE_READWRITE
         	                 |SEC_COMMIT|SEC_NOCACHE
            	            , 0, *dwSize
               	         , pWhat );
			if( hMem )
				goto isokay;
         ODS( "Sorry - Nothing good can happen to day with a filename like that...");
         return NULL;
      }

      if( GetLastError() == ERROR_ALREADY_EXISTS )
      {
         // mark status for memory... dunno why?
         // in theory this is a memory image of valid memory already...
         if( GetFileSize( hFile, NULL ) < (uint32_t)*dwSize )
         {
            //*dwSize = ( ( *dwSize + ( FILE_GRAN - 1 ) ) / FILE_GRAN ) * FILE_GRAN;
            SetFilePointer( hFile, *dwSize, NULL, FILE_BEGIN );
            SetEndOfFile( hFile );
         }
         else
         {
         	*dwSize = GetFileSize( hFile, NULL );
            //SetFilePointer( hFile, 0, NULL, FILE_END );
           // SetEndOfFile( hFile );
         }
      }
      else
      {
         // allocate it at least one grain....
         // or round current value up to next grain
         //if( !*dwSize )
        	//{
        	//	*dwSize = 1; // not zero.
        //		CloseHandle( hFile );
        //		DeleteFile( pWhere );
        //		return NULL;
        	//}
         //*dwSize = ( ( *dwSize + ( FILE_GRAN - 1 ) ) / FILE_GRAN ) * FILE_GRAN;
         SetFilePointer( hFile, *dwSize, NULL, FILE_BEGIN );
         SetEndOfFile( hFile );
      }
      *dwSize = GetFileSize( hFile, NULL );

      hMem = CreateFileMapping( hFile
                        , NULL
                        , PAGE_READWRITE
                          |SEC_COMMIT|SEC_NOCACHE
                        , 0, 0
                        , pWhat );
      if( !hMem )
      {
	      TEXTCHAR byDebug[256];
   	   sprintf( byDebug, "Create of mapping failed on object specified? %p", hFile );
      	ODS( byDebug );
      	*dwSize = 1;
         CloseHandle( hFile );
         return NULL;
      }
   }
isokay:
   pMem = MapViewOfFileEx( hMem
              , FILE_MAP_READ|FILE_MAP_WRITE
              , 0, 0  // offset high, low
              , 0     // size of file to map
              , (POINTER)address ); // don't specify load location... irrelavent...
#endif
   if( !pMem )
   {
   	Log1( "Create view of file for memory access failed at %p", (POINTER)address );
#ifdef _win32
      CloseHandle( hMem );
      if( hFile != INVALID_HANDLE_VALUE )
         CloseHandle( hFile );
#endif
      return NULL;
   }

   // store information about this
   // external to the space - do NOT
   // modify content of memory opened!
   AddSpace( pSpacePool, hFile, hMem, pMem, *dwSize, TRUE );
   return pMem;
}

//------------------------------------------------------------------------------------------------------
#undef OpenSpace
MEM_PROC( POINTER, OpenSpace )( TEXTSTR pWhat, TEXTSTR pWhere, uint32_t* dwSize )
{
        return OpenSpaceEx( pWhat, pWhere, 0, dwSize );
}
//------------------------------------------------------------------------------------------------------

MEM_PROC( void, InitHeap)( PMEM pMem, uint32_t dwSize )
{
   //pMem->dwSize = *dwSize - MEM_SIZE;
   // size of the PMEM block is all inclusive (from pMem(0) to pMem(dwSize))
   // do NOT need to substract the size of the tracking header
   // otherwise we would be working from &pMem->pRoot + dwSize
	if( !FindSpace( pMem ) )
	{
		// a heap must be in the valid space pool.
		// it may not have come from a file, and will not have 
		// a file or memory handle.
	   AddSpace( pSpacePool, 0, 0, pMem, dwSize, TRUE );
	}
   pMem->dwSize = dwSize;

   pMem->pFirstFree = pMem->pRoot;
   //InitializeCriticalSec( &pMem->cs );
   pMem->pRoot[0].dwSize = dwSize - MEM_SIZE - CHUNK_SIZE;
   pMem->pRoot[0].dwOwners = 0;
        pMem->pRoot[0].pRoot  = (void*)pMem;
   pMem->pRoot[0].pPrior = NULL;
   pMem->pRoot[0].pNextFree = NULL;
   pMem->pRoot[0].pPriorFree = NULL;
#ifdef _DEBUG
   if( !bDisableDebug )
   {
       MemSet( pMem->pRoot[0].byData, 0x1BADCAFE, pMem->pRoot[0].dwSize );
       //*(int*)(pMem->pRoot[0].byData + pMem->pRoot[0].dwSize - 4) = 0x78563412;
   }
#endif
}

//------------------------------------------------------------------------------------------------------

PMEM DigSpace( TEXTSTR pWhat, TEXTSTR pWhere, uint32_t* dwSize )
{
    PMEM pMem = (PMEM)OpenSpace( pWhat, pWhere, dwSize );

    if( !pMem )
    {
        TEXTCHAR byDebug[256];
        // did reference BASE_MEMORY...
        sprintf( byDebug, "Create view of file for memory access failed at ????" );
        ODS( byDebug );
        CloseSpace( (POINTER)pMem );
        return NULL;
    }
    Log( "Go to init the heap..." );
    InitHeap( pMem, *dwSize );
    return pMem;
}

//------------------------------------------------------------------------------------------------------

int ExpandSpace( PMEM pMem, uint32_t dwAmount )
{
   PSPACE pspace = FindSpace( (POINTER)pMem );
   PSPACE pnewspace;
   PMEM   pExtend;

   pExtend = DigSpace( NULL, NULL, &dwAmount );
   pnewspace = FindSpace( (POINTER)pExtend );

   // unlink from list of spaces to search, link onto
   // this space we are expanding...
   *pnewspace->me = pnewspace->pNext;
   if( pnewspace->pNext )
   	pnewspace->pNext->me = pnewspace->me;
	pnewspace->me = NULL;
	pnewspace->pNext = NULL;

   while( pspace && pspace->expansion )
   	pspace = pspace->expansion;
   if( pspace )
   	pspace->expansion = pnewspace;

   return TRUE;
}


//------------------------------------------------------------------------------------------------------


static uint32_t bMemInstanced; // set if anybody starts to DIG.
static PMEM pMemInstance;
//static PMEM pMasterInstance;

static PMEM GrabMemEx( PMEM pMem DBG_PASS )
#define GrabMem(m) GrabMemEx( m DBG_SRC )
{
	if( !pMem )
	{
		if( !bMemInstanced )
		{
         uint32_t MinSize = SYSTEM_CAPACITY;
         // generic internal memory, unnamed, unshared, unsaved
         pMemInstance = pMem = DigSpace( NULL, NULL, &MinSize );
         if( !pMem )
         {
            bMemInstanced = FALSE;
            ODS( "Failed to allocate memory - assuming fatailty at Allocation service level." );
            return NULL;
         }
		}
		else
			return 0;
	}
   return pMem;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, HeapAllocateEx )( PMEM pHeap, uint32_t dwSize DBG_PASS )
{
   PCHUNK pc;
   PMEM pMem, pCurMem = NULL;
   PSPACE pMemSpace;

#ifdef _DEBUG
	if( !bDisableDebug )
	   GetHeapMemStatsEx(pHeap, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif
	if( !dwSize ) // no size is NO space!
	{
		return NULL;
	}
	// if memstats is used - memory could have been initialized there...
	// so wait til now to grab pMemInstance.
   if( !pHeap )
   	pHeap = pMemInstance;

   pMem = GrabMem( pHeap );
   dwSize += 3; // fix size to allocate at least _32s which
   dwSize &= 0xFFFFFFFC;

#ifdef _DEBUG
	if( !bDisableDebug )
	   dwSize += 4;  // add a uint32_t at end to mark, and check for application overflow...
#endif

	// re-search for memory should step long back...
search_for_free_memory:
	for( pc = NULL, pMemSpace = FindSpace( pMem ); !pc && pMemSpace; pMemSpace = pMemSpace->expansion )
	{
      // pMem is guaranteed not to change until Dropped
      //Log1( "Scan Block: %08x", pMemSpace );
      //DropMem( pCurMem );
	   pCurMem = (PMEM)pMemSpace->pMem;
	   GrabMem( pCurMem );
      pc = pCurMem->pFirstFree; // current pChunk(pc)
	   while( pc )
   	{
	   	//Log2( "Check %d vs %d", pc->dwSize, dwSize );
   	   if( pc->dwSize >= dwSize ) // if free block size is big enough...
      	{
	         // split block
   	      if( ( pc->dwSize - dwSize ) <= ( CHUNK_SIZE + nMinAllocateSize ) ) // must allocate it all.
      	   {
         	   if( !pc->pPriorFree )
            	{
               	if( ( pCurMem->pFirstFree = pc->pNextFree ) )
                  	pCurMem->pFirstFree->pPriorFree = NULL;

	               pc->pNextFree = NULL;
   	            pc->pPriorFree = NULL;
      	      }
         	   else
            	{
	               if( pc->pNextFree )
   	            {
      	            pc->pNextFree->pPriorFree = pc->pPriorFree;
         	      }
            	   pc->pPriorFree->pNextFree = pc->pNextFree;
	            }
   	         pc->dwOwners = 1;
      	      break; // successful allocation....
	         }
   	      else
      	   {
         	   PCHUNK pNew;  // cleared, NEW, uninitialized block...
            	PCHUNK pNext;
	            pNext = (PCHUNK)( pc->byData + pc->dwSize );
   	         pNew = (PCHUNK)(pc->byData + dwSize);
      	      pNew->dwSize = ( ( pc->dwSize - CHUNK_SIZE ) - dwSize );
         	   if( pNew->dwSize & 0x80000000 )
            	   DebugBreak();

	            pc->dwSize = dwSize; // set old size?  this can wait until we have the block.
   	         if( pc->dwSize & 0x80000000 )
      	         DebugBreak();

            	if( (uintptr_t)pNext - (uintptr_t)pCurMem < (uintptr_t)pCurMem->dwSize )  // not beyond end of memory...
	               pNext->pPrior = pNew;

      	      pNew->dwOwners = 0;
         	   pNew->pRoot = pc->pRoot;
            	pNew->pPrior = pc;
#ifdef _DEBUG
				   pNew->pFile = pFile;
			   	pNew->nLine = nLine;
#endif

	            if( ( pNew->pNextFree = pc->pNextFree ) )
   	         {
      	         pc->pNextFree->pPriorFree = pNew;
         	   }

	            if( ( pNew->pPriorFree = pc->pPriorFree ) )
   	         {
      	         pc->pPriorFree->pNextFree = pNew;
         	   }
            	else // update pFirstFree....
	            {
   	            PCHUNK pFirstFree;
      	         PCHUNK *ppFirstFree;
         	      pFirstFree=*(ppFirstFree=&(((PMEM)(pc->pRoot))->pFirstFree));

               	if( pFirstFree != pc )
	               {
   	               ODS( "This NEVER happens!");
      	            pNew->pNextFree = pFirstFree;
         	         pFirstFree->pPriorFree = pNew;
            	   }
               	*ppFirstFree = pNew;
	            }
         	   pc->dwOwners = 1;  // set owned block.
   	         break; // successful allocation....
      	   }
	      }
   	   pc = pc->pNextFree;
	   }
   }
   //DropMem( pCurMem );
   if( !pc )
   {
   	{
   		char msg[256];
   		sprintf( msg, "Failed to allocate %ld bytes", dwSize );
	   	ODSEx( msg, pFile, nLine );
	   }
      if( dwSize < SYSTEM_CAPACITY )
      {
         if( ExpandSpace( pMem, SYSTEM_CAPACITY ) )
            goto search_for_free_memory;
      }
      else
      {
         if( ExpandSpace( pMem, dwSize + CHUNK_SIZE + MEM_SIZE ) )
            goto search_for_free_memory;
      }
      ODS( "Remaining space in memory block is insufficient.  Please EXPAND block.");
      //DropMem( pMem );
		//DebugDumpMemEx( TRUE );
#ifdef DEBUG_FIND_SPACE
		DumpSpaces();
#endif
      return NULL;
   }
   if( bLogAllocate )
   {
      TEXTCHAR byMsg[256];
      sprintf( byMsg, "Allocate : %p(%p) - %ld bytes"
                    , pc->byData, pc, pc->dwSize );
      ODSEx( byMsg, pFile, nLine );
   }

#ifdef _DEBUG
	pc->pFile = pFile;
	pc->nLine = nLine;
	if( !bDisableDebug )
	{
	   MemSet( pc->byData, 0xDEADBEEF, pc->dwSize );

#ifdef STRONG_DIAGNOSTIC
		LOG_ALLOCATE( pc->byData, pc->dwSize, pFile, nLine );
#endif
	   *(int*)(pc->byData + pc->dwSize - 4) = 0x78563412;

#ifdef MEM_TRACKING
	   TRACK_ALLOC( pc, pc->dwSize, pFile, nLine );
#endif
	}
#endif
   //DropMem( pMem );
#ifdef _DEBUG
#ifdef STRONG_DIAGNOSTIC
   DiagMem();
#endif
#endif
#ifdef _DEBUG
	//if( !bDisableDebug )
	//   GetHeapMemStatsEx(pHeap, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif
#ifdef DEBUG_FIND_SPACE
	DumpSpaces();
#endif
   return pc->byData;
}

//------------------------------------------------------------------------------------------------------
#undef AllocateEx
MEM_PROC( POINTER, AllocateEx )( uint32_t dwSize DBG_PASS )
{
	return HeapAllocateEx( pMemInstance, dwSize DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, HeapReallocateEx )( PMEM pHeap, POINTER source, uint32_t size DBG_PASS )
{
	POINTER dest;
	uint32_t min;

	dest = HeapAllocateEx( pHeap, size DBG_RELAY );
	if( source )
	{
		min = SizeOfMemBlock( source );
		if( size < min )
			min = size;
		MemCpy( dest, source, min );
		if( min < size )
			MemSet( ((uint8_t*)dest) + min, size, size - min );
		ReleaseEx( source DBG_RELAY );
	}
	else
		MemSet( dest, 0, size );


	return dest;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, HeapMoveEx )( PMEM pNewHeap, POINTER source DBG_PASS )
{
	return HeapReallocateEx( pNewHeap, source, SizeOfMemBlock( source ) DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, ReallocateEx )( POINTER source, uint32_t size DBG_PASS )
{
	return HeapReallocateEx( pMemInstance, source, size DBG_RELAY );
}

//------------------------------------------------------------------------------------------------------

static void Bubble( PMEM pMem )
{
   // handle sorting free memory to be least signficant first...
   PCHUNK temp, next;
   temp = pMem->pFirstFree;
   if( !temp )
   	return;
   next = temp->pNextFree;
   while( temp && next )
   {
      if( (uintptr_t)next < (uintptr_t)temp )
      {
         next = temp->pNextFree;
         if( (temp->pNextFree = next->pNextFree) )
         {
            temp->pNextFree->pPriorFree = temp;
         }
         if( (next->pPriorFree = temp->pPriorFree ) )
         {
            next->pPriorFree->pNextFree = next;
         }
         next->pNextFree = temp;
         temp->pPriorFree = next;
      }
      if( !next->pPriorFree )
         pMem->pFirstFree = next; // this is new beginning of chain...
      temp = next;
      next = temp->pNextFree;
   }
}

//------------------------------------------------------------------------------------------------------

static void CollapsePrior( PCHUNK pThis )
{

}

//------------------------------------------------------------------------------------------------------

MEM_PROC( uint32_t, SizeOfMemBlock )( CPOINTER pData )
{
	if( pData )
	{
	   register PCHUNK pc = (PCHUNK)(((uintptr_t)pData) - offsetof( CHUNK, byData ));
#ifdef _DEBUG
		if( !bDisableDebug )
	   	return pc->dwSize - 4;
#endif
	   return pc->dwSize;
	}
	return 0;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, MemDupEx )( CPOINTER thing DBG_PASS )
{
	uint32_t size = SizeOfMemBlock( thing );
   POINTER result;
	result = AllocateEx( size DBG_RELAY );
	MemCpy( result, thing, size );
   return result;
}

#undef MemDup
MEM_PROC( POINTER, MemDup )(CPOINTER thing )
{
   return MemDupEx( thing DBG_SRC );
}
//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, ReleaseEx )( POINTER pData DBG_PASS )
{
// register PMEM pMem = (PMEM)(pData - offsetof( MEM, pRoot ));
   register PCHUNK pc = (PCHUNK)(((uintptr_t)pData) - offsetof( CHUNK, byData ));
   PMEM pMem, pCurMem;
   PSPACE pMemSpace;
	if( !pData ) // always safe to release nothing at all..
		return NULL;

	// Allow a simple release() to close a shared memory file mapping
	// this is a slight performance hit for all deallocations
	{
		PSPACE ps = FindSpace( pData );
		if( ps )
		{
			DoCloseSpace( ps );
			return NULL;
		}
	}
#ifdef _DEBUG
	if( !bDisableDebug )
	   GetHeapMemStatsEx(pc->pRoot, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif

   pMem = GrabMem( pc->pRoot );
	pMemSpace = FindSpace( pMem );

   while( pMemSpace && ( ( pCurMem = (PMEM)pMemSpace->pMem ),
   		 (   ( (uint32_t)pData < (uint32_t)pCurMem )
   		 ||  ( (uint32_t)pData > ( (uint32_t)pCurMem + pCurMem->dwSize ) ) )
   		 )
        )
	{
		Log( "ERROR: This block should have immediatly referenced it's correct heap!" );
       pMemSpace = pMemSpace->expansion;
   }
   if( !pMemSpace )
   {
#ifdef _DEBUG
      TEXTCHAR byMsg[256];
      sprintf( byMsg, "This Block is NOT within the managed heap! : %p"
                    , pData );
      ODSEx( byMsg, pFile, nLine );
#endif
  		DebugBreak();
		//DropMem( pMem );
       return NULL;
   }
   pCurMem = (PMEM)pMemSpace->pMem;
   if( bLogAllocate )
   {
      TEXTCHAR byMsg[256];
      sprintf( byMsg, "Release  : %p(%p) - %ld bytes"
                    , pc->byData, pc, pc->dwSize );
      ODSEx( byMsg, pFile, nLine );
   }
#ifdef STRONG_DIAGNOSTIC
		LOG_RELEASE( pData, pFile, nLine );
#endif
   if( pData && pc )
   {
      if( !pc->dwOwners )
      {
         TEXTCHAR byMsg[256];
#ifdef _DEBUG
         sprintf( (char*)byMsg, "Block is already Free! %p by %s(%ld)", pc,
                 pc->pFile, pc->nLine );
#else
			// CRITICAL ERROR!
         sprintf( (char*)byMsg, "Block is already Free! %p ", pc );
#endif
         ODSEx( byMsg, pFile, nLine );
         //DropMem( pMem );
        // DebugDumpMem();
         return pData;
      }
#ifdef _DEBUG
		if( !bDisableDebug )
			if( *(int*)(pc->byData + pc->dwSize - 4 ) != 0x78563412 )
			{
				Log1( "memory block: %p", pc->byData );
				ODSEx( "Application overflowed requested memory...", pc->pFile, pc->nLine );
					DebugBreak();
			}
#endif
      pc->dwOwners--;
      if( pc->dwOwners )
      {
         //ODS( "Released Block was held by other owners" );
         //DebugBreak();
         //DebugDumpMem();
#ifdef _DEBUG
			pc->nLine = nLine;  // store where it was released from
		   pc->pFile = pFile;
#endif
         //DropMem( pMem );
         return pData;
      }
      else
      {
         LOGICAL bCollapsed = FALSE;
         PCHUNK pNext, pNextNext, pPrior;
         PCHUNK *ppFirstFree;
         unsigned int nNext;
         // fill memory with a known value...
         // this will allow me to check usage after release....
#ifdef MEM_TRACKING
			if( !bDisableDebug )
			{
			   TRACK_DEALLOC( pc, pFile, nLine );
			}
#endif

#ifdef _DEBUG
			pc->nLine = nLine;  // store where it was released from
			pc->pFile = pFile;
			if( !bDisableDebug )
			{
	         MemSet( pc->byData, 0xFACEBEAD,	 pc->dwSize );
			}
#endif
         ppFirstFree = &(((PMEM)(pc->pRoot))->pFirstFree);

         pNext = (PCHUNK)(pc->byData + pc->dwSize);
         if( (nNext = (int)pNext - (int)pCurMem) >= pCurMem->dwSize )
         {
            // if next is NOT within valid memory...
            pNext = NULL;
         }

         if( ( pPrior = pc->pPrior ) ) // is not root chunk...
         {
            if( !pPrior->dwOwners ) // prior physical is free
            {
               pPrior->dwSize += CHUNK_SIZE + pc->dwSize; // add this header plus size
               if( pPrior->dwSize & 0x80000000 )
                  DebugBreak();
               pc = pPrior; // use prior block as base ....
               if( pNext )
                  pNext->pPrior = pPrior;
               bCollapsed = TRUE;
            }
         }
         // begin checking NEXT physical memory block for conglomerating
         if( pNext )
         {
            if( !pNext->dwOwners )
            {
               pc->dwSize += CHUNK_SIZE + pNext->dwSize;
               if( pc->dwSize & 0x80000000 )
                  DebugBreak();
               pNextNext = (PCHUNK)(pNext->byData + pNext->dwSize );

               if( (((int)pNextNext) - ((int)pCurMem)) < (int)pCurMem->dwSize )
               {
                  pNextNext->pPrior = pc;
               }

               if( bCollapsed ) // this free'd block became part of pPrior
               {
                  pNext->pPrior = pc; // regardless of ownership this has to point at prior
                  if( pNext->pNextFree )
                     pNext->pNextFree->pPriorFree = pNext->pPriorFree;
                  else
                     if( pNext->pPriorFree )
                        pNext->pPriorFree->pNextFree = NULL;

                  if( pNext->pPriorFree )
                     pNext->pPriorFree->pNextFree = pNext->pNextFree;
                  else
                     *ppFirstFree = pNext->pNextFree;
               }
               else
               {
                  pc->pPriorFree = pNext->pPriorFree;
                  if( pc->pPriorFree )
                     pc->pPriorFree->pNextFree = pc;
                  else  // there wasn't a prior - musta been first.
                     *ppFirstFree = pc;

                  pc->pNextFree = pNext->pNextFree; //absorbing next block - absorb pointer
                  if( pc->pNextFree )
                     pc->pNextFree->pPriorFree = pc;
                  bCollapsed = TRUE;
               }
            }
         }

         if( !bCollapsed ) // no block near this one was free...
         {
            pc->pPriorFree = NULL;
            if( ( pc->pNextFree = (*ppFirstFree) ) )
               pc->pNextFree->pPriorFree = pc;
            (*ppFirstFree) = pc; //
         }
      }
   }
   Bubble( pMem );
   //DropMem( pMem );
#ifdef _DEBUG
	if( !bDisableDebug )
	{
#ifdef STRONG_DIAGNOSTIC
	   DiagMem( pMem );
#endif
      //GetHeapMemStatsEx(pc->pRoot, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
	}
#endif
#ifdef _DEBUG
	if( !bDisableDebug )
	   GetHeapMemStatsEx(pc->pRoot, &dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif

   return NULL;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, HoldEx )( POINTER pData DBG_PASS )
{
   PCHUNK pc = (PCHUNK)((char*)pData - CHUNK_SIZE);
   PMEM pMem = GrabMem( pc->pRoot );

   if( bLogAllocate )
   {
      TEXTCHAR byMsg[256];
      sprintf( byMsg, "Hold     : %p - %ld bytes",
                           pc,pc->dwSize );
      ODSEx( byMsg, pFile, nLine );
   }
#ifdef MEM_TRACKING
	if( !bDisableDebug )
	   TRACK_HOLD( pc, pFile, nLine );
#endif
   pc->dwOwners++;
   //DropMem(pMem );
#ifdef _DEBUG
	//if( !bDisableDebug )
	//   GetHeapMemStatsEx(&dwFree,&dwAllocated,&dwBlocks,&dwFreeBlocks DBG_RELAY);
#endif
   return pData;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( POINTER, GetFirstUsedBlock )( PMEM pHeap )
{
	return pHeap->pRoot[0].byData;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, DebugDumpHeapMemEx )( PMEM pHeap, LOGICAL bVerbose )
{
   PCHUNK pc, _pc;
   int nTotalFree = 0;
   int nChunks = 0;
   int nTotalUsed = 0;
   TEXTCHAR byDebug[256];
   PSPACE pMemSpace;
   PMEM pMem = GrabMem( pHeap ), pCurMem;

#ifdef STRONG_DIAGNOSTIC
   WriteDebugLog();
#endif

   pc = pMem->pRoot;

   ODS(" ------ Memory Dump ------- " );
   {
      TEXTCHAR byDebug[256];
      sprintf( byDebug, "FirstFree : %p",
               pMem->pFirstFree );
      ODS( byDebug );
   }

	for( pc = NULL, pMemSpace = FindSpace( pMem ); pMemSpace; pMemSpace = pMemSpace->expansion )
	{
	   pCurMem = (PMEM)pMemSpace->pMem;
      pc = pCurMem->pRoot; // current pChunk(pc)

	   while( (((int)pc) - ((int)pCurMem)) < (int)pCurMem->dwSize ) // while PC not off end of memory
   	{
   		Relinquish(); // allow debug log to work...
	      nChunks++;
   	   if( !pc->dwOwners )
      	{
         	nTotalFree += pc->dwSize;
	         if( bVerbose )
   	      {
      	      sprintf( byDebug, "Free at %p size: %ld(%lx) Prior:%p NF:%p PF:%p",
         	       pc, pc->dwSize, pc->dwSize,
            	    pc->pPrior,
               	 pc->pNextFree, pc->pPriorFree );
	         }
   	   }
      	else
	      {
   	      nTotalUsed += pc->dwSize;
      	   if( bVerbose )
         	{
            	sprintf( byDebug, "Used at %p size: %ld(%lx) Prior:%p",
               	 pc, pc->dwSize, pc->dwSize,
	                pc->pPrior );
   	      }
      	}
	      if( bVerbose )
   	   {
#ifdef _DEBUG
				ODSEx( byDebug, !IsBadReadPtr( pc->pFile, 1 )?pc->pFile:"Unknown", pc->nLine );
#else
	         ODS( byDebug );
#endif
   	   }
	      _pc = pc;
   	   pc = (PCHUNK)(pc->byData + pc->dwSize );
      	if( pc == _pc )
	      {
	      	ODS( "Next block is the current block..." );
   	      DebugBreak(); // broken memory chain
      	   break;
	      }
	   }
   }
   sprintf( byDebug, "Total Free: %d  TotalUsed: %d  TotalChunks: %d TotalMemory:%d",
            nTotalFree, nTotalUsed, nChunks,
            nTotalFree + nTotalUsed + nChunks * CHUNK_SIZE );
   ODS( byDebug );
   Relinquish();
#ifdef MEM_TRACKING
	if( !bDisableDebug )
	   TRACK_DUMP();
#endif
   //DropMem( pMem );
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, DebugDumpMemEx )( LOGICAL bVerbose )
{
	DebugDumpHeapMemEx( pMemInstance, bVerbose );
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, DebugDumpHeapMemFile )( PMEM pHeap, char *pFilename )
{
	FILE *file;
	file = fopen( pFilename, "wt" );
	if( file )
	{
      PCHUNK pc, _pc;
   	PMEM pMem, pCurMem;
	   PSPACE pMemSpace;
      int nTotalFree = 0;
      int nChunks = 0;
      int nTotalUsed = 0;
      TEXTCHAR byDebug[256];

      pMem = GrabMem( pHeap );

      fprintf( file, " ------ Memory Dump ------- \n" );
      {
         TEXTCHAR byDebug[256];
         sprintf( byDebug, "FirstFree : %p",
                  pMem->pFirstFree );
         fprintf( file, "%s\n", byDebug );
      }

		for( pc = NULL, pMemSpace = FindSpace( pMem ); pMemSpace; pMemSpace = pMemSpace->expansion )
		{
		   pCurMem = (PMEM)pMemSpace->pMem;
	      pc = pCurMem->pRoot; // current pChunk(pc)

	      while( (((int)pc) - ((int)pCurMem)) < (int)pCurMem->dwSize ) // while PC not off end of memory
   	   {
      	   Relinquish(); // allow debug log to work...
         	nChunks++;
	         if( !pc->dwOwners )
   	      {
      	      nTotalFree += pc->dwSize;
         	   sprintf( byDebug, "Free at %p size: %ld(%lx) Prior:%p NF:%p PF:%p",
            	    pc, pc->dwSize, pc->dwSize,
               	 pc->pPrior,
	                pc->pNextFree, pc->pPriorFree );
   	      }
      	   else
         	{
            	nTotalUsed += pc->dwSize;
	            sprintf( byDebug, "Used at %p size: %ld(%lx) Prior:%p",
   	             pc, pc->dwSize, pc->dwSize,
      	          pc->pPrior );
         	}
   #ifdef _DEBUG
	   		fprintf( file, "%s(%ld):%s\n", pc->pFile, pc->nLine, byDebug );
   #else
   	      fprintf( file, "%s\n", byDebug );
   #endif
	         _pc = pc;
   	      pc = (PCHUNK)(pc->byData + pc->dwSize );
      	   if( pc == _pc )
         	{
	            DebugBreak(); // broken memory chain
   	         break;
      	   }
	      }
      }
      fprintf( file, "--------------- FREE MEMORY LIST --------------------\n" );

		for( pc = NULL, pMemSpace = FindSpace( pMem ); pMemSpace; pMemSpace = pMemSpace->expansion )
		{
		   pCurMem = (PMEM)pMemSpace->pMem;
	      pc = pCurMem->pFirstFree; // current pChunk(pc)

	      while( pc ) // while PC not off end of memory
   	   {
      	   sprintf( byDebug, "Free at %p size: %ld(%lx) ",
         	 		   pc, pc->dwSize, pc->dwSize );

   #ifdef _DEBUG
	   		fprintf( file, "%s(%ld):%s\n", pc->pFile, pc->nLine, byDebug );
   #else
   	      fprintf( file, "%s\n", byDebug );
   #endif
	      	pc = pc->pNextFree;
   	   }
		}
      sprintf( byDebug, "Total Free: %d  TotalUsed: %d  TotalChunks: %d TotalMemory:%d",
               nTotalFree, nTotalUsed, nChunks,
               nTotalFree + nTotalUsed + nChunks * CHUNK_SIZE );
      fprintf( file, "%s\n", byDebug );
      Relinquish();
   #ifdef MEM_TRACKING
   	if( !bDisableDebug )
   	   TRACK_DUMP();
   #endif
      //DropMem( pMem );

 		fclose( file );
	}

}

//------------------------------------------------------------------------------------------------------
MEM_PROC( void, DebugDumpMemFile )( char *pFilename )
{
	DebugDumpHeapMemFile( pMemInstance, pFilename );
}
//------------------------------------------------------------------------------------------------------

MEM_PROC( LOGICAL, Defragment )( POINTER *ppMemory ) // returns true/false, updates pointer
{
   // pass an array of allocated memory... for all memory blocks in list,
   // check to see if they can be reallocated lower, and or just moved to
   // a memory space lower than they are now.
   PCHUNK pc, pPrior;
   PMEM pMem;
   if( !ppMemory || !*ppMemory)
      return FALSE;
   pc = (PCHUNK)(((uintptr_t)(*ppMemory)) - offsetof( CHUNK, byData ));
   pMem = GrabMem( pc->pRoot );

      // check if prior block is free... if so - then...
      // move this data down, and reallocate the freeness at the end
      // this reallocation may move the free next to another free, which
      // should be collapsed into this one...
   pPrior = pc->pPrior;
   if( ( pc->dwOwners == 1 ) && // not HELD by others... no way to update their pointers
       pPrior &&
       !pPrior->dwOwners )
   {
      CHUNK Allocated, Free, *pNew;
      Allocated = *pc; // save this chunk...
      Free = *pPrior; // save free chunk...
      MemCpy( pc->pPrior->byData, pc->byData, Allocated.dwSize );
      pNew = (PCHUNK)(pPrior->byData + Allocated.dwSize);
      pNew->dwSize = Free.dwSize;
      pNew->dwOwners = 0;
      pNew->pPrior = pPrior; // now pAllocated...
      pNew->pRoot = Free.pRoot;
      if( ( pNew->pNextFree = Free.pNextFree ) )
         pNew->pNextFree->pPriorFree = pNew;
      if( ( pNew->pPriorFree = Free.pPriorFree ) )
         pNew->pPriorFree->pNextFree = pNew;
      else
      {
         (((PMEM)(Allocated.pRoot))->pFirstFree) = pNew;
      }
#ifdef _DEBUG
      pNew->pFile = Free.pFile;
      pNew->nLine = Free.nLine;
#endif
      pPrior->dwSize = Allocated.dwSize;
      pPrior->dwOwners = 1;
      pPrior->pNextFree = NULL;
      pPrior->pPriorFree = NULL;
      // update NEXT NEXT real block...
      {
         PCHUNK pNext;
         pNext = (PCHUNK)( pNew->byData + pNew->dwSize );

         if( (((int)pNext) - ((int)pMem)) < (int)pMem->dwSize )
         {
            if( !pNext->dwOwners ) // if next is free.....
            {
               // consolidate...
               if( pNext->pNextFree )
               	pNext->pNextFree->pPriorFree = pNext->pPriorFree;
               if( pNext->pPriorFree )
               	pNext->pPriorFree->pNextFree = pNext->pNextFree;
               pNew->dwSize += pNext->dwSize + CHUNK_SIZE;
               pNext = (PCHUNK)( pNew->byData + pNew->dwSize );

               if( (uint32_t)(((char *)pNext) - ((char *)pMem)) < pMem->dwSize )
               {
                  pNext->pPrior = pNew;
               }
            }
            else
               pNext->pPrior = pNew;
         }
      }
      *ppMemory = pPrior->byData;
      //DropMem( pMem );
      GetHeapMemStats( pMemInstance, NULL, NULL, NULL, NULL );
      return TRUE;
   }

   //DropMem( pMem );
   return FALSE;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, GetHeapMemStatsEx )( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks DBG_PASS )
{
    int nFree = 0, nUsed = 0, nChunks = 0, nFreeChunks = 0, nSpaces = 0;
    PCHUNK pc, _pc;
    PMEM pMem;
    PSPACE pMemSpace;
    pMem = GrabMemEx( pHeap DBG_RELAY );
    pMemSpace = FindSpace( pMem );
    while( pMemSpace )
    {
        PMEM pMem = ((PMEM)pMemSpace->pMem);
        pc = pMem->pRoot;

        while( (((int)pc) - ((int)pMem)) < (int)pMem->dwSize ) // while PC not off end of memory
        {
            nChunks++;
            if( !pc->dwOwners )
            {
                nFree += pc->dwSize;
                nFreeChunks++;
            }
            else
            {
                nUsed += pc->dwSize;
#ifdef _DEBUG
					 if( !bDisableDebug )
					 {
						 if( pc->dwSize > pMem->dwSize )
						 {
                      Log1( "Memory block %p has a corrupt size.", pc->byData );
							 DebugBreak();
						 }
						 else if( *(int*)( pc->byData + pc->dwSize - 4 ) != 0x78563412 )
						 {
							 Log1( "memory block: %p", pc->byData );
							 ODSEx( "Application overflowed allocated memory.", pc->pFile, pc->nLine );
							 DebugBreak();
						 }
					 }
#endif
            }

            _pc = pc;
            pc = (PCHUNK)(pc->byData + pc->dwSize );
            if( (((int)pc) - ((int)pMem)) < (int)pMem->dwSize  )
            {
                if( pc == _pc )
                {
                    Log( "Current block is the same as the last block we checked!" );
                    DebugBreak(); // broken memory chain
                    break;
                }
                if( pc->pPrior != _pc )
                {
                    Log4( "Block's prior is not the last block we checked! prior %p sz: %d current: %p currentprior: %p"
                    			, _pc
                    			, _pc->dwSize
                    			, pc
                    			, pc->pPrior );
                    DebugBreak();
                    break;
                }
            }
        }
        pc = pMem->pFirstFree;
        while( pc )
        {
            if( pc->dwOwners )
            {  // owned block is in free memory chain ! ?
                Log( "Owned block is in free memory chain!" );
                DebugBreak();
                break;
            }
            pc = pc->pNextFree;
        }
        nSpaces++;
        pMemSpace = pMemSpace->expansion;
    }
    //DropMem( pMem );

    if( pFree )
        *pFree = nFree;
    if( pUsed )
        *pUsed = nUsed;
    if( pChunks )
        *pChunks = nChunks;
    if( pFreeChunks )
        *pFreeChunks = nFreeChunks;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, GetMemStats )( uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks )
{
	GetHeapMemStats( pMemInstance, pFree, pUsed, pChunks, pFreeChunks );
}

//------------------------------------------------------------------------------------------------------
MEM_PROC( void, SetAllocateLogging )( LOGICAL bTrueFalse )
{
   bLogAllocate = bTrueFalse;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, SetCriticalLogging )( LOGICAL bTrueFalse )
{
#ifdef _DEBUG
   bLogCritical = bTrueFalse;
#endif
}
//------------------------------------------------------------------------------------------------------

MEM_PROC( void, SetAllocateDebug )( LOGICAL bDisable )
{
#ifdef _DEBUG
	bDisableDebug = bDisable;
#endif
}

void DisableMemoryValidate( LOGICAL disable )
{
	SetAllocateDebug(TRUE);
}


//------------------------------------------------------------------------------------------------------

MEM_PROC( void, SetMinAllocate )( int nSize )
{
	nMinAllocateSize = nSize;
}

//------------------------------------------------------------------------------------------------------

MEM_PROC( void, SetHeapUnit )( int dwSize )
{
	dwSystemCapacity = dwSize;
}

//------------------------------------------------------------------------------------------------------

// unsigned long = INDEX = uint32_t
// soon to become uint64_t - base pointers expand then also... so alignemt maintains
//------------------------------------------------------------------------------------------------------

MEM_PROC( int, SetMemoryPointer )( PSPACE pmstr, uint32_t idx )
{
   pmstr->dwSmallIndex = idx;
   return 1;
}

//------------------------------------------------------------------------------------------------------
MEM_PROC( int, CanReadMore )( PSPACE pmstr )
{
   return pmstr->dwSmallSize - pmstr->dwSmallIndex;
}

//------------------------------------------------------------------------------------------------------
MEM_PROC( int, CanReadSize )( PSPACE pmstr, uint32_t size )
{
   return ( ( pmstr->dwSmallSize - pmstr->dwSmallIndex ) > size );
}

//------------------------------------------------------------------------------------------------------
MEM_PROC( unsigned char, ReadNextCharacter )( PSPACE pmstr )
{
   unsigned char t;
   t = 0;
   if( pmstr )
   {
      if( pmstr->dwSmallIndex < (pmstr->dwSmallSize - 1) )
      {
         t = *(unsigned char *)((char*)pmstr->pMem + pmstr->dwSmallIndex++);
      }
   }
   return t;
}

//------------------------------------------------------------------------------------------------------
MEM_PROC( unsigned short, ReadNextWord )( PSPACE pmstr )
{
   unsigned short t;
   t = 0;
   if( pmstr )
   {
      if( pmstr->dwSmallIndex < (pmstr->dwSmallSize - 2) )
      {
         t = *(unsigned short *)((char*)pmstr->pMem + pmstr->dwSmallIndex);
         pmstr->dwSmallIndex += 2;
      }
   }
   return t;
}

//------------------------------------------------------------------------------------------------------
MEM_PROC( uint32_t, ReadNextLong )( PSPACE pmstr )
{
   unsigned long t;
   t = 0;
   if( pmstr )
   {
      if( pmstr->dwSmallIndex < (pmstr->dwSmallSize - 4) )
      {
         t = *(unsigned long *)((char*)pmstr->pMem + pmstr->dwSmallIndex);
         pmstr->dwSmallIndex += 4;
      }
   }
   return t;
}

//------------------------------------------------------------------------------------------------------
MEM_PROC( unsigned long, ReadNextBuffer )( PSPACE pmstr, void *p, unsigned long i )
{
   memcpy( p, (char*)pmstr->pMem + pmstr->dwSmallIndex, i );
   pmstr->dwSmallIndex += i;
   return i;
}

//------------------------------------------------------------------------------------------------------
// result in 0(equal), 1 above, or -1 below
// *r contains the position of difference
MEM_PROC( int, CmpMem8 )( void *s1, void *s2, unsigned long n, unsigned long *r )
{
   register int t1, t2;
	uint32_t pos;
   {
   	pos = 0;
      while( pos < n )
      {
         t1 = *(unsigned char*)s1;
         t2 = *(unsigned char*)s2;
         if( ( t1 ) == ( t2 ) )
         {
            (pos)++;
            s1 = (void*)(((int)s1) + 1);
            s2 = (void*)(((int)s2) + 1);
         }
         else if( t1 > t2 )
         {
         	if( r )
         		*r = pos;
            return 1;
         }
         else
         	if( r )
         		*r = pos;
            return -1;
      }
   }
   if( r )
   	*r = pos;
   return 0;
}

//------------------------------------------------------------------------------------------------------
#undef GetHeapMemStats
MEM_PROC( void, GetHeapMemStats )( PMEM pHeap, uint32_t *pFree, uint32_t *pUsed, uint32_t *pChunks, uint32_t *pFreeChunks )
{
	GetHeapMemStatsEx( pHeap, pFree, pUsed, pChunks, pFreeChunks DBG_SRC );
}

MEM_PROC( TEXTSTR, StrDupEx )( CTEXTSTR original DBG_PASS )
{
	int len = strlen( original ) + 1;
	char *result = AllocateEx( len DBG_RELAY );
	MemCpy( result, original, len );
	return result;
}

#ifdef __GNUC__
int stricmp( char *one, char *two )
{
   return strcasecmp( one, two );
}

int strnicmp( char *one, char *two, int len )
{
   return strncasecmp( one, two, len );
}
#endif


