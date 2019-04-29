#include <stdhdrs.h>
#include <stdio.h>
#include <network.h>

PCLIENT pcListen;
uint32_t tick, _tick;

void CPROC LogRead( PCLIENT pc, POINTER buffer, size_t nSize, SOCKADDR *sa )
{
	tick = GetTickCount();
	if( !buffer )
	{
		buffer = Allocate( 4096 );
		_tick = tick;
	}
	else
	{
		TEXTCHAR msgtime[12];
		fwrite( msgtime, snprintf( msgtime, 12, "%04" _32f "|", tick - _tick ), 1, stdout );
		_tick = tick;
		fwrite( buffer, nSize, 1, stdout );
		if( nSize < 4096 )
			fwrite( "\n", 1, 1, stdout );
		fflush( stdout );
	}
	ReadUDP( pc, buffer, 4096 );
}

// command line option
//  time
//  output file...
SaneWinMain( argc, argv )
{
	NetworkStart();
	pcListen = ServeUDP( "0.0.0.0", 514, LogRead, NULL );
	while( pcListen ) WakeableSleep( SLEEP_FOREVER );
	printf( "Failed to start." );
	return 1;
}
EndSaneWinMain()
