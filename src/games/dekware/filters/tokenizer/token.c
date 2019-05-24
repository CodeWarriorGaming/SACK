// provides typical token burst on a data stream.

#include <stdhdrs.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"


static int myTypeID;

typedef struct mydatapath_tag {
	DATAPATH common;

	struct {
		uint32_t newline : 1;
		uint32_t outbound : 1;
	} flags;
	PTEXT tokens;
} MYDATAPATH, *PMYDATAPATH;

//--------------------------------------------------------------------------

static PTEXT CPROC Tokenize( PMYDATAPATH pdp, PTEXT text )
{
	PTEXT out, result;
	if( text )
	{
		pdp->tokens = SegAppend( pdp->tokens, burst( text ) );
		if( pdp->tokens && !pdp->flags.newline )
			pdp->tokens->flags|= TF_NORETURN;
		LineRelease( text );
	}
	// nothing indirect will be present... 
	// now - should I include the newline in the burst?
	result = out = pdp->tokens;
	if( out )
	{
		while( out && GetTextSize( out ) ) 
			out = NEXTLINE( out );
		if( out )
		{
			pdp->flags.newline = 1;
			out = NEXTLINE( out ); // break AFTER the newline
			SegBreak( out );
		}
		else
			pdp->flags.newline = 0;
		pdp->tokens = out;
	}
	return result;
}

//--------------------------------------------------------------------------

static int CPROC Read( PMYDATAPATH pdp )
{
	if( pdp->flags.outbound )
		return RelayInput( (PDATAPATH)pdp, NULL );
	return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))Tokenize );
}

//--------------------------------------------------------------------------

static int CPROC Write( PMYDATAPATH pdp )
{
	if( pdp->flags.outbound )
		return RelayOutput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))Tokenize );
	return RelayOutput( (PDATAPATH)pdp, NULL );
}

//--------------------------------------------------------------------------

static int CPROC Close( PMYDATAPATH pdp )
{
	pdp->common.Type = 0;
	return 0;
}

//--------------------------------------------------------------------------

#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp = NULL;
	PTEXT option;
	// parameters
	//    none
	pdp = CreateDataPath( pChannel, MYDATAPATH );
	while( option = GetParam( ps, &parameters ) )
	{
	   if( OptionLike( option, "inbound" ) )
	   {
	   	pdp->flags.outbound = 0;
	   }
	   else if( OptionLike( option, "outbound" ) )
	   {
	   	pdp->flags.outbound = 1;
	   }
	   else
	   {
	   	//DECLTEXT( msg, "Unknown option for token filter. Allowed are 'inbound' and 'outbound'." );
	   }
	}
	pdp->tokens = NULL;
	pdp->common.Type = myTypeID;
	pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
	pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
	pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
	return (PDATAPATH)pdp;
}

//--------------------------------------------------------------------------

PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
	myTypeID = RegisterDevice( "token", "Tokenizes the stream going through it...", Open );
	//   return DekVersion;
	}
}

//--------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( "token" );
}
