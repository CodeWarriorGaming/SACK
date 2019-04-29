
#define DEFINE_DEFAULT_RENDER_INTERFACE
#define USE_IMAGE_INTERFACE GetImageInterface()
#include <stdhdrs.h>
#include <image.h>
#include <sharemem.h>
#include <network.h>
#include <controls.h>
#include <timers.h>
#include <configscript.h>
#ifdef __LINUX__
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev.h>
#include <xvid.h>
#endif


#include "channels.h"
#include "streamstruct.h"
#include "compress.h"
#include "decompress.h"
#include "filters.h"

#ifdef __LINUX__
typedef int HANDLE;
#endif
typedef struct {
	PCAPTURE_DEVICE pDev[2];

	int32_t display_x, display_y;
	uint32_t display_width, display_height;
   SOCKADDR *saBroadcast;
} GLOBAL;

GLOBAL g;

#ifdef __LINUX__
#define INVALID_HANDLE_VALUE -1
#endif

int EnqueFrameEx( SIMPLE_QUEUE *que, uintptr_t frame, char *qname, int line )
#define EnqueFrame(q,f) EnqueFrameEx(q,f,#q, __LINE__ )
{
	INDEX next_head = (que->head + 1)%que->size;
   //lprintf( "Enque %d into %s (%d)", frame, qname, line );
	if( next_head != que->tail )
	{
      que->frames[que->head] = frame;
		que->head = next_head;
      return 1;
	}
   return 0;
}

uintptr_t DequeFrameEx( SIMPLE_QUEUE *que, char *quename, int line )
#define DequeFrame(q) DequeFrameEx(q,#q, __LINE__ )
{
	if( que->head != que->tail )
	{
		INDEX r = que->frames[que->tail];
      //lprintf( "%s(%d) results as %d", quename, line, r );
		que->tail++;
		que->tail %= que->size;
      return r;
	}
   return INVALID_INDEX;
}

INDEX FramesQueued( SIMPLE_QUEUE *que )
{
	int len = que->head - que->tail;
	if( len < 0 )
      len += que->size;
   return len;
}

#ifdef __LINUX__
typedef struct v4l_data_tag
{
	HANDLE handle;
	struct {
		BIT_FIELD bFirstRead : 1;
		BIT_FIELD bNotMemMapped : 1;
	} flags;
	struct video_capability caps;
	struct video_mbuf mbuf;
	struct video_mmap *vmap;
	struct video_tuner tuner;
	struct video_audio audio;
	struct video_channel chan;
	struct video_picture picture;
	struct video_unit    unit;
   // for caputre directly to the framebuffer
	struct video_buffer  vbuf;
   // capture size parameters
   struct video_window vwin;
	INDEX curr_channel;
	uint32_t channels[256]; // array of valid channels?
	uint32_t freq;
	uint8_t* map; // pointer to mapped memory..
	Image *pFrames;
   SIMPLE_QUEUE done;
   SIMPLE_QUEUE ready;
	SIMPLE_QUEUE capture;
	PTHREAD pThread;
// nLastCycledFrame...
   // held until next copy..
   INDEX nFrame;
// an array of frames for count of frames...

} *PV4L_DATA, *PDEVICE_DATA;

//--------------------------------------------------------------------

int myioctlEx( HANDLE h, int op, char *pOp, uintptr_t data DBG_PASS )
{
	int ret = ioctl(h,op,(POINTER)data);
	int err;
	lprintf( "args : h:%d op:%s(%d), %" PTRSZVALf
           , h, pOp, op, data );
	if( ret == -1 )
	{
      err = errno;
		lprintf( DBG_FILELINEFMT "Failed to ioctl(%s): %s" DBG_RELAY
				 , pOp
				 , strerror(err)
				 );
		errno = err;
	}
	return ret;
}

#define ioctl(h,op,data) myioctlEx((h),(op),#op,(uintptr_t)(data) DBG_SRC)

// return last captured frame
void CycleReadV4L( PDEVICE_DATA pDevice )
{
	if( pDevice->flags.bNotMemMapped )
	{
		int n;
		if( ( n = DequeFrame( &pDevice->done ) ) != INVALID_INDEX )
		{
         int result;
			result = read( pDevice->handle
							 , GetImageSurface( pDevice->pFrames[n] )
							 , pDevice->vwin.width*pDevice->vwin.height*(pDevice->picture.depth/8) );
			lprintf( "read resulted: %d", result );

			EnqueFrame( &pDevice->ready, n );
		}
	}
	else
	{
		if( pDevice->flags.bFirstRead )
		{
			int n;
			lprintf( "..." );
			while( ( n = DequeFrame( &pDevice->done ) ) != INVALID_INDEX )
			{
			// preserve the order of frames queued for capture...
				lprintf( "initial start queuing %d which is %d,%d"
						 , n
						 , pDevice->caps.maxwidth
						 , pDevice->caps.maxheight
						 );
				pDevice->vmap[n].format = VIDEO_PALETTE_RGB24;
				pDevice->vmap[n].frame  = n;
				pDevice->vmap[n].width  = pDevice->vwin.width;
				pDevice->vmap[n].height = pDevice->vwin.height;
											// mark all frames pending...
												// therefore none are ready.
				if( ioctl( pDevice->handle, VIDIOCMCAPTURE, pDevice->vmap + n) == -1 )
				{
					lprintf( "Failed to start capture first read." );
				}
				else
				{
					lprintf( "Mark %d as to capture...", n );
					EnqueFrame( &pDevice->capture, n );
				}
				pDevice->flags.bFirstRead = 0;
			}
		}
		else
		{
			INDEX nFrame;
			do
			{
				nFrame = DequeFrame( &pDevice->done );
						 // there's only one frame, have to wait for it to be done...
				if( nFrame == INVALID_INDEX && pDevice->mbuf.frames == 1 )
					goto check_capture_done;

				if( nFrame == INVALID_INDEX && ( FramesQueued( &pDevice->capture ) < 3 ) )
				{
					lprintf( "Dropping a frame..." );
					lprintf( "done %d %d", pDevice->done.head, pDevice->done.tail );
					lprintf( "capture %d %d", pDevice->capture.head, pDevice->capture.tail );
					lprintf( "ready %d %d", pDevice->ready.head, pDevice->ready.tail );
				// dropping a frame...
				// no done frames, snag the next ready and grab it.
					nFrame = DequeFrame( &pDevice->ready );
					if( nFrame == INVALID_INDEX )
					{
						lprintf( "Fatal error - all frames are outstanding in process..." );
						lprintf( "done %d %d", pDevice->done.head, pDevice->done.tail );
						lprintf( "capture %d %d", pDevice->capture.head, pDevice->capture.tail );
						lprintf( "ready %d %d", pDevice->ready.head, pDevice->ready.tail );
					}
				}
				if( nFrame != INVALID_INDEX )
				{
				// locked should only ever be one frame...
				// and that one we can skip - and claim a dropped frame.

				// begin queuing another frame to be filled...
				// the next frame will ahve already been queued...
				// and we will fall out and wait for ti - because
				// the very next one is the oldest one queued to wait.
				//lprintf( "queue wait on frame %d", nFrame );
					if( ioctl(pDevice->handle
								, VIDIOCMCAPTURE
								, &pDevice->vmap[nFrame]) == -1 )
					{
					// already pending...
						lprintf( "Failed to capture next frame pending..." );
					}
					EnqueFrame( &pDevice->capture, nFrame );
				}
			} while( nFrame != INVALID_INDEX );
		}
	check_capture_done:
		{
			int retry = 1;
			int nFrame = DequeFrame( &pDevice->capture );
			while (retry ){
							  //lprintf( "Monitor frame %d wait.", nFrame );
				if( ioctl(pDevice->handle, VIDIOCSYNC, &nFrame ) == -1 )
				{
					switch( errno )
					{
					// this is an okay thing to have happen?
					// the read is lost? or what?

					case EINTR:
						lprintf( "interrupt woke ioctl..." );
						retry = 1;
						break;
					default:
						lprintf( "Fatal error... didn't wait for capture." );
						retry = 0;
						return;
								//return NULL;
					}
				}
				else
					retry = 0;
			}
			EnqueFrame( &pDevice->ready, nFrame );
							//lprintf( "issuing wake to thread..." );
							//WakeThread( pDevice->pThread );
							//lprintf( "Monitor frame %d ready.", nFrame );
		}
	}
}

int CPROC GetCapturedFrame( uintptr_t psv, PCAPTURE_DEVICE pDevice )
{
	PDEVICE_DATA pDevData = (PDEVICE_DATA)psv;
	if( pDevData->nFrame != INVALID_INDEX )
		EnqueFrame( &pDevData->done, pDevData->nFrame );
   pDevData->nFrame = DequeFrame( &pDevData->ready );
	if( pDevData->nFrame != INVALID_INDEX )
	{
	//lprintf( "Process frame %d to compress...", nFrame );
	//lprintf( "done %d %d", pDeviceData->done.head, pDeviceData->done.tail );
	//lprintf( "capture %d %d", pDeviceData->capture.head, pDeviceData->capture.tail );
	//lprintf( "ready %d %d", pDeviceData->ready.head, pDeviceData->ready.tail );
		SetDeviceData( pDevice
						 , pDevData->pFrames[pDevData->nFrame]
						 , 768*480 );
      return 1;
	}
   return 0;
}
#endif

uintptr_t CPROC CycleCallbacks( PTHREAD pThread )
{
	uintptr_t psvDev = pThread->param;
	PCAPTURE_DEVICE pDevice = (PCAPTURE_DEVICE)psvDev;
   pDevice->pCallbackThread = pThread;
   while( 1 )
	{
		PCAPTURE_CALLBACK callback;
		for( callback = pDevice->callbacks; callback; callback = NextLink( callback ) )
		{
			//lprintf( "Process callback: %p", callback );
			//lprintf( "Process callback: %p (%p)", callback, callback->callback );
			if( !callback->callback( callback->psv, pDevice ) )
            break;
			//lprintf( "Processed callback: %p (%p)", callback, callback->callback );
		}
		Relinquish();
	}
   return 0;
}

PCAPTURE_DEVICE CreateCaptureStream( void )
{
	PCAPTURE_DEVICE pcd = Allocate( sizeof( CAPTURE_DEVICE) );
	MemSet( pcd, 0, sizeof( CAPTURE_DEVICE ) );
	ThreadTo( CycleCallbacks, (uintptr_t)pcd );
   return pcd;
}

#ifdef __LINUX__
uintptr_t CPROC CycleReadThread( PTHREAD pThread )
{
	uintptr_t psvDev = pThread->param;
   PDEVICE_DATA pDevice = (PDEVICE_DATA)psvDev;
	while(1)
	{
		CycleReadV4L( pDevice );
	}
   return 0;
}
#endif

//--------------------------------------------------------------------

#ifdef __LINUX__
void CloseV4L( PDEVICE_DATA *ppDevice )
{
	if( ppDevice )
	{
		PDEVICE_DATA pDevice = *ppDevice;
		if( pDevice->pFrames )
		{
			int n;
			for( n = 0; n < pDevice->mbuf.frames; n++ )
				UnmakeImageFile( pDevice->pFrames[n] );
			Release( pDevice->pFrames );
		}
		if( pDevice->vmap )
			Release( pDevice->vmap );
		if( pDevice->map != MAP_FAILED && pDevice->map )
			munmap( pDevice->map, pDevice->mbuf.size );
		close( pDevice->handle );
		(*ppDevice) = NULL;
	}
}
#endif

//--------------------------------------------------------------------

#ifdef __LINUX__
void SetChannelV4L( PDEVICE_DATA pDevice, int chan )
{
	if(chan == -1)
		pDevice->chan.channel = 1;  //set to 1 for video line input
   else
		pDevice->chan.channel = 0;
	ioctl(pDevice->handle, VIDIOCSCHAN, &pDevice->chan);

	if( chan >= 0 )
	{
		int freq = freqs[chan];
		pDevice->tuner.mode = VIDEO_MODE_NTSC;
      //pDevice->tuner.flags=VIDEO_TUNER_NORM;
		ioctl(pDevice->handle, VIDIOCSTUNER, &pDevice->tuner);
		ioctl(pDevice->handle, VIDIOCSFREQ, &freq);
	}
}
#endif
//--------------------------------------------------------------------

//--------------------------------------------------------------------

#ifdef __LINUX__
int GetCaptureSizeV4L( PDEVICE_DATA pDevice, uint32_t* width, uint32_t* height )
{
	if( !pDevice )
		return FALSE;
	if( width )
		(*width) = pDevice->vwin.width;
	if( height )
		(*height) = pDevice->vwin.height;
	return TRUE;
}
#endif
//--------------------------------------------------------------------

#ifdef __LINUX__
uintptr_t OpenV4L( char *name )
{
	PDEVICE_DATA pDevice;
	if( !name )
	{
		char default_device[256];
		int n;
		for( n = 0; n < 16; n++ )
		{
		// woo fun with recursion!
			sprintf( default_device, "/devices/video%d", n );
			pDevice = (PDEVICE_DATA)OpenV4L( default_device );
			if( !pDevice )
			{
				sprintf( default_device, "/dev/video%d", n );
				pDevice = (PDEVICE_DATA)OpenV4L( default_device );
			}
			if( pDevice )
            break;
		}
		if( !pDevice )
			lprintf( "Failed to open default devices enumerated from 0 to 16" );
		return (uintptr_t)pDevice;
	}

	{
		int handle = open( name, O_RDONLY );
		if( handle == -1 )
		{
         lprintf( "attempted open of %s failed", name );
			return (uintptr_t)NULL;
		}
		lprintf( "attempted open of %s success!", name );
		pDevice = Allocate( sizeof( *pDevice ) );
		MemSet( pDevice, 0, sizeof( *pDevice ) );
		pDevice->flags.bFirstRead = 1;
		pDevice->handle = handle;

		ioctl(handle, VIDIOCGCAP, &pDevice->caps );
		lprintf( "Some interesting ranges: (%d,%d) (%d,%d)"
				 , pDevice->caps.minwidth
				 , pDevice->caps.minheight
				 , pDevice->caps.maxwidth
				 , pDevice->caps.maxheight
				 );

		ioctl(pDevice->handle, VIDIOCGCHAN, &pDevice->chan);
		if(pDevice->channels[pDevice->curr_channel])
			pDevice->chan.channel = 0;
		else
			pDevice->chan.channel = 1;  //set to 1 for video line input
		ioctl(pDevice->handle, VIDIOCSCHAN, &pDevice->chan);

		pDevice->tuner.tuner = 0;
		ioctl(pDevice->handle, VIDIOCGTUNER, &pDevice->tuner);
		pDevice->tuner.mode = VIDEO_MODE_NTSC;
		ioctl(pDevice->handle, VIDIOCSTUNER, &pDevice->tuner);

		//ioctl(pDevice->handle, VIDIOCSFREQ, &pDevice->freq);
		ioctl(pDevice->handle, VIDIOCGFREQ, &pDevice->freq);

		ioctl(pDevice->handle, VIDIOCGAUDIO, &pDevice->audio);
		pDevice->audio.flags &= ~VIDEO_AUDIO_MUTE;
		ioctl(pDevice->handle, VIDIOCSAUDIO, &pDevice->audio);

		ioctl( pDevice->handle, VIDIOCGUNIT, &pDevice->unit );
		lprintf( "related units: %d %d %d %d %d"
				 , pDevice->unit.video
				 , pDevice->unit.vbi
				 , pDevice->unit.radio
				 , pDevice->unit.audio
				 , pDevice->unit.teletext );

		ioctl( pDevice->handle, VIDIOCGWIN, &pDevice->vwin );
		lprintf( "Format parameters %d %d %d %d %d %d"
				 , pDevice->vwin.x
				 , pDevice->vwin.y
				 , pDevice->vwin.width
				 , pDevice->vwin.height
				 , pDevice->vwin.chromakey
				 , pDevice->vwin.flags );


		if( ioctl(pDevice->handle, VIDIOCGMBUF, &pDevice->mbuf) == -1 )
		{
			CloseV4L( &pDevice );
			return (uintptr_t)NULL;
		}
      lprintf( "Driver claims it has %d frames", pDevice->mbuf.frames );
		MemSet( ( pDevice->pFrames = Allocate( sizeof( pDevice->pFrames[0] ) * pDevice->mbuf.frames ) )
  				, 0
 				, sizeof( pDevice->pFrames[0] ) * pDevice->mbuf.frames );
		pDevice->vmap = Allocate( sizeof( pDevice->vmap[0] )
										* pDevice->mbuf.frames );
      pDevice->ready.size = pDevice->mbuf.frames + 1;
      pDevice->done.size = pDevice->mbuf.frames + 1;
      pDevice->capture.size = pDevice->mbuf.frames + 1;
      pDevice->ready.frames = Allocate( sizeof( *pDevice->ready.frames ) * (pDevice->mbuf.frames + 1) );
      pDevice->done.frames = Allocate( sizeof( *pDevice->done.frames ) * (pDevice->mbuf.frames + 1) );
		pDevice->capture.frames = Allocate( sizeof( *pDevice->capture.frames ) * (pDevice->mbuf.frames + 1) );

    	 //pDevice->mbuf.frames = 2;
		ioctl( pDevice->handle, VIDIOCGPICT, &pDevice->picture );
		lprintf( "picture stats: b: %d h: %d col: %d con: %d wht: %d dep: %d pal: %d"
				 , pDevice->picture.brightness
				 , pDevice->picture.hue
				 , pDevice->picture.colour
				 , pDevice->picture.contrast
				 , pDevice->picture.whiteness
				 , pDevice->picture.depth
				 , pDevice->picture.palette );
      // must set depth for palette 32
      pDevice->picture.depth = 32;
		pDevice->picture.palette = VIDEO_PALETTE_RGB32;
		ioctl( pDevice->handle, VIDIOCSPICT, &pDevice->picture );

		{
			int n;
			lprintf( " about to mmap with %u handle of %d"
							 , pDevice->mbuf.size
							 , pDevice->handle

                 );

			pDevice->map = (uint8_t*)mmap(0
									 , pDevice->mbuf.size//4096 //pDevice->mbuf.size/
									 , PROT_READ
									 , MAP_SHARED
									 , pDevice->handle
									 , 0
									 );
			if( pDevice->map == MAP_FAILED )
			{
            pDevice->flags.bNotMemMapped = TRUE;
            //lprintf( "Failed to mmap... %d %d %d(%d)", pDevice->mbuf.size, pDevice->handle, getpagesize(), pDevice->mbuf.size%getpagesize() );
				//perror( "Failed to mmap..." );
				//CloseV4L( &pDevice );
				//return (uintptr_t)NULL;
				for( n = 0; n < pDevice->mbuf.frames; n++ )
				{
					pDevice->pFrames[n] = NULL;
					pDevice->pFrames[n] = MakeImageFile( pDevice->vwin.width
																  , pDevice->vwin.height );
				}
			}
			else
			{
				for( n = 0; n < pDevice->mbuf.frames; n++ )
				{
					pDevice->pFrames[n] = NULL;
					pDevice->pFrames[n] = RemakeImage( pDevice->pFrames[n]
																, (PCOLOR)(pDevice->map + pDevice->mbuf.offsets[n])
																, pDevice->vwin.width
																, pDevice->vwin.height );
				}
			}
		}
 		{
			int n;
			for( n = 0; n < pDevice->mbuf.frames; n++ )
			{
            lprintf( "put %d in done", n );
            EnqueFrame( &pDevice->done, n );
			}
		}
      lprintf( "cycle read..." );
		ThreadTo( CycleReadThread, (uintptr_t)pDevice );
		return (uintptr_t)pDevice;
	}
}
#endif

//--------------------------------------------------------------------

void AddCaptureCallback( PCAPTURE_DEVICE pDevice, int (CPROC *callback)(uintptr_t psv, PCAPTURE_DEVICE pDev ), uintptr_t psv )
{
	PCAPTURE_CALLBACK pcc = Allocate( sizeof( *pcc ) );
	pcc->callback = callback;
	pcc->psv = psv;
	pcc->next = NULL;
   pcc->me = NULL;
   lprintf( "Added callback... %p (%p)", pcc, callback );
   LinkLast( pDevice->callbacks, PCAPTURE_CALLBACK, pcc );
}

//--------------------------------------------------------------------

int CPROC DisplayAFrame( uintptr_t psv, PCAPTURE_DEVICE pDevice )
{
	static uint32_t tick;
	uint32_t newtick = GetTickCount();
   static uint32_t lastloss;
   static uint32_t frames; frames++;
	if( tick )
	{
		if( newtick - tick > 50 )
		{
			//lprintf( "Lost something? %d (%d?) %d %d", newtick - tick, (newtick-tick)/33, frames, frames-lastloss );
			lastloss = frames;
		}
	}
   tick = newtick;
		  //PFRAME pFrame = (PFRAME)psv;
	{
		PRENDERER pRender = (PRENDERER)psv;
		Image image;
      		GetDeviceData( pDevice, (POINTER*)&image, NULL );
		if( image )
			BlotImage( GetDisplayImage( pRender ), image, 0, 0 );
		lprintf( "Displayed frame..." );
								// if I blotcolor over the region of the overlay, then I can draw it
								// zero alpha and zero color auto full transparent under overlay.
								//BlotImage( GetDisplayImage( pRender ), pDevice->pResultFrame, 0, 0 );

							// restore second device's frame...
							// need to perhaps open clipping regions for smart drawing
							// through the display layer...
		if(g.pDev[1] && g.pDev[1]->data)
			BlotScaledImageSizedTo( GetDisplayImage( pRender )
									 , (Image)g.pDev[1]->data
									 , 0, 0
									 , 240, 180 );
	UpdateDisplay( pRender );
								//UpdateFrame( pFrame, 0, 0, -1, -1 );
	}
   return 1;
}

//--------------------------------------------------------------------

int CPROC DisplayASmallFrame( uintptr_t psv, PCAPTURE_DEVICE pDevice )
{
	static uint32_t tick;
	uint32_t newtick = GetTickCount();
   static uint32_t lastloss;
   static uint32_t frames; frames++;
	if( tick )
	{
		if( newtick - tick > 50 )
		{
			//lprintf( "Lost something? %d (%d?) %d %d", newtick - tick, (newtick-tick)/33, frames, frames-lastloss );
			lastloss = frames;
		}
	}
   tick = newtick;
	//PFRAME pFrame = (PFRAME)psv;
	{
		PRENDERER pRender = (PRENDERER)psv;
		//BlotImage( GetFrameSurface( pFrame ), pDevice->pCurrentFrame, 0, 0 );
		//lprintf( "Display frame..." );
		BlotScaledImageSizedTo( GetDisplayImage( pRender )
									 , (Image)pDevice->data
									 , 0, 0
									 , 240, 180 );
		UpdateDisplayPortion( pRender, 0, 0,  240, 180 );
		//UpdateFrame( pFrame, 0, 0, -1, -1 );
	}
   return 0;
}

//---------------------------------------------------------------------------

typedef struct codec_tag {
	PCAPTURE_DEVICE pDevice;
	PCOMPRESS pCompress;
   // data is the data frame shared between compress and decompress gadgets
   POINTER data;
	PDECOMPRESS pDecompress;
	uint64_t bytes;
   uint32_t start;
} *PCODEC;


void GetDeviceData( PCAPTURE_DEVICE pDevice, POINTER *data, INDEX *length )
{
	(*data) = pDevice->data;
	if( length )
		(*length) = pDevice->length;
}

void SetDeviceDataEx( PCAPTURE_DEVICE pDevice, POINTER data, INDEX length
						  , ReleaseBitStreamData release,uintptr_t psv)
{
	if( pDevice->release )
      pDevice->release( pDevice->psv, data, length );
	pDevice->data = data;
	pDevice->length = length;
	pDevice->release = release;
   pDevice->psv = psv;
}

uintptr_t CPROC SetNetworkBroadcast( uintptr_t psv, arg_list args )
{
	PARAM( args, char*, addr );
   lprintf( "~~~~~~~" );
   g.saBroadcast = CreateSockAddress( addr, 0 );
   return psv;
}

uintptr_t CPROC SetDisplayFilterSize( uintptr_t psv, arg_list args )
{
   PARAM( args, int64_t, x );
   PARAM( args, int64_t, y );
   PARAM( args, int64_t, width );
	PARAM( args, int64_t, height );
   g.display_x = x;
   g.display_y = y;
	g.display_width = width;
	g.display_height = height;

   return psv;
}

void ReadConfig( char *name )
{
	PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
	AddConfigurationMethod( pch, "display at (%i,%i) %i by %i", SetDisplayFilterSize );
	AddConfigurationMethod( pch, "broadcast %m", SetNetworkBroadcast );
   ProcessConfigurationFile( pch, name, 0 );

}


#ifndef __LIBRARY__
int main( int argc, char **argv )
{
	uint32_t width, height;
	//SetSystemLog( SYSLOG_FILE, stdout );
	//SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
	SetBlotMethod( BLOT_C );
   SetAllocateLogging( TRUE );
   NetworkWait(NULL,16, 16 );
	ReadConfig( argc < 2 ? "stream.conf":argv[1] );
   SetAllocateLogging( TRUE );
	{
		PRENDERER pRender;
      g.pDev[0] = CreateCaptureStream();
#ifdef __LINUX__
		{
			uintptr_t psvCap;
			psvCap = OpenV4L( NULL );
			//psvCap = OpenV4L( NULL );
			if( !psvCap )
			{
				return 0;
			}
			AddCaptureCallback( g.pDev[0], GetCapturedFrame, psvCap );
			GetCaptureSizeV4L( (PDEVICE_DATA)psvCap, &width, &height );
			//SetChannelV4L( (PDEVICE_DATA)psvCap, 45 );
		}
#else
		{
			uintptr_t psvCap;
         psvCap = OpenV4W( NULL );
		}
//  		AddCaptureCallback( g.pDev[0], GetNetworkCapturedFrame, OpenNetworkCapture( NULL ) );
		width = 768;
      height = 480;
//  		AddCaptureCallback( g.pDev[0], DecompressFrame
//  								, OpenDecompressor( g.pDev[0]
//  														, width, height ) );
#endif
		pRender = OpenDisplaySizedAt( 0, g.display_width, g.display_height, g.display_x, g.display_y );
      UpdateDisplay( pRender );
		AddCaptureCallback( g.pDev[0], DisplayAFrame, (uintptr_t)pRender );
#ifdef __LINUX__
		//g.pDev[1] = OpenV4L( NULL );
		if( !g.pDev[1] )
		{
         lprintf( "Failed to open second capture device" );
		}
		else
		{
			AddCaptureCallback( g.pDev[1], DisplayASmallFrame, (uintptr_t)pRender );
		}
#endif
		//if(0)
#ifdef __LINUX__
      // apply capture-compressor
		{
			int n;
         for( n = 0; n < 2; n++ )
			{
#ifndef __64__
				if( g.pDev[n] )
				{
					PCODEC codec = Allocate( sizeof( *codec ) );
					codec->pDevice = g.pDev[n];
					codec->data = NULL;
					codec->bytes = 0;
               codec->start = GetTickCount();
//  					codec->pCompress = OpenCompressor( (PCAPTURE_DEVICE)g.pDev[n]
//  																, width, height );
//  					codec->pDecompress = OpenDecompressor( (PCAPTURE_DEVICE)g.pDev[n]
//  																	 , width, height );
//					AddCaptureCallback( g.pDev[n], CompressFrame, (uintptr_t)codec->pCompress );
				}
#endif
			}
		}
#ifndef __64__
		{
//  			AddCaptureCallback( g.pDev[0], RenderNetworkFrame, OpenNetworkRender( NULL ) );
		}
#endif
#endif

   SetAllocateLogging( TRUE );
		{
			char whatever[2];
			do{lprintf( "Entering wait..." ); fgets( whatever, 2, stdin ); } while( whatever[0] != 'q' );
		}
      // these should call all the callback destruction calls...
		//CloseCapture( g.pDev[0] );
		//CloseCapture( g.pDev[1] );
		CloseDisplay( pRender );
	}
	return 0;
}

#endif

