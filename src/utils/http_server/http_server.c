
#include <stdhdrs.h>
#include <filesys.h>
#include <http.h>

LOGICAL CPROC FallbackHandler( uintptr_t psv, struct HttpState *state )
{
	PTEXT resource = GetHttpResource( state );
	PTEXT result;
	size_t result_size = GetSizeofFile( GetText( resource ) + 1, NULL );
	//lprintf( "Serve resource: %s", GetText( resource ) );

	if( result_size != (size_t)-1 )
	{
		FILE *input = sack_fopen( 0, GetText( resource ) + 1, "rb" );
		if( input )
		{
			size_t x;
			result = SegCreate( result_size );
			x = fread( GetText( result ), 1, result_size, input );
			SetTextSize( result, result_size );
			//SendHttpMessage( state, NULL, result );
			SendHttpResponse( state, NULL, 200, "OK", "text/html", result );
			LineRelease( result );
			fclose( input );
			return TRUE;
		}
	}
	return FALSE;
}

static void LoadPlugins( CTEXTSTR listfile )
{
	FILE *input = sack_fopen( 0, listfile, "rt" );
	if( input )
	{
		TEXTCHAR buf[256];
		while( fgets( buf, sizeof( buf ), input ) )
		{
			if( buf[StrLen(buf)-1] == '\n' )
				buf[StrLen(buf)-1] = 0;
			LoadFunction( buf, NULL );
		}
		fclose( input );
	}
}

// is main or winmain as required by compiler and target
SaneWinMain( argc, argv )
{
	CTEXTSTR site = "d3x0r.org";
	CTEXTSTR serve_interface = "0.0.0.0:6180";
	{
		while( argv++,(--argc > 0) )
		{
			if( argv[0][0] == '@' )
			{
				LoadPlugins( argv[0]+1 );
			}
			else if( argv[0][0] == '-' )
			{
				switch( argv[0][1] )
				{
				case 'i':
					if( --argc )
					{
						argv++;
						serve_interface = argv[0];
					}
					break;
				}
			}
			else
			{
				site = argv[0];
			}
		}
	}
	NetworkWait( NULL, 1024, 16 );
	{
		struct HttpServer *server = CreateHttpServerEx( serve_interface, NULL, site, FallbackHandler, 0 );
		if( server )
			while( 1 )
				WakeableSleep( 1000000 );
	}
	return 0;
}
EndSaneWinMain()



