#define DEFINES_DEKWARE_INTERFACE
#define DO_LOGGING
#include <stdhdrs.h>
#include "plugin.h"

INDEX iTelnet;

typedef struct mydatapath_tag {
	DATAPATH common;
	struct {
		uint32_t bDoNAWS : 1;
		uint32_t bSendNAWS : 1;
		uint32_t bLastWasIAC : 1; // secondary IAC (255) received
	} flags;
// buffers needed for proc_iac
	uint8_t iac_data[256];
	int iac_count;
	PVARTEXT vt; // collects var text....
} MYDATAPATH, *PMYDATAPATH;

///*
// * Definitions for the TELNET protocol.
// */
// 
#define  IAC   (uint8_t)255    /* 0XFF interpret as command: */
#define  DONT  (uint8_t)254    /* 0XFE you are not to use option */
#define  DO    (uint8_t)253    /* 0XFD please, you use option */
#define  WONT  (uint8_t)252    /* 0XFC I won't use option */
#define  WILL  (uint8_t)251    /* 0XFB I will use option */
#define  SB    (uint8_t)250    /* 0XFA interpret as subnegotiation */
#define  GA    (uint8_t)249    /* 0XF9 you may reverse the line */
#define  EL    (uint8_t)248    /* 0XF8 erase the current line */
#define  EC    (uint8_t)247    /* 0XF7 erase the current character */
#define  AYT   (uint8_t)246    /* 0XF6 are you there */
#define  AO    (uint8_t)245    /* 0XF5 abort output--but let prog finish */
#define  IP    (uint8_t)244    /* 0XF4 interrupt process--permanently */
#define  IACBK (uint8_t)243    /* 0XF3 break */
#define  DM    (uint8_t)242    /* 0XF2 data mark--for connect. cleaning */
#define  NOP   (uint8_t)241    /* 0XF1 nop */
#define  SE    (uint8_t)240    /* 0XF0 end sub negotiation */
#define  EOR   (uint8_t)239    /* 0XEF end of record (transparent mode) */

#define DO_TB                 0  //*856  do transmit binary 
#define DO_ECHO               1  //*857
#define DO_SGA                3  //*858 supress go ahead
#define DO_STATUS             5  //*859
#define DO_TIMINGMARK         6  //*860
#define DO_BM                19  // 735 Telent Byte Macro
#define DO_DET               20  // 1043 Data Entry Terminal
#define DO_TERM_TYPE         24  //*884/930/1091
#define DO_EOR               25  // 885 end of record
#define DO_TUID              26  // 927 TACACS User Identification 
#define DO_OUTMRK            27  // 933 Output Marking 
#define DO_TTYLOC            28  // 946 Telnet Terminal Location
#define DO_3270_REGIME       29  // 1041 Telnet 3270 Regime Option
#define DO_X3_PAD            30  // 1053 
#define DO_NAWS              31  //*1073 Negotiate About Window Size
#define DO_TERMSPEED         32  //*1079 Set terminal line speed
#define DO_FLOW_CONTROL      33  //?1372   - xon/xoff
#define DO_LINEMODE          34  //?1184
#define DO_X_DISPLAY_LOCATION 35 // 1986 send back my name for other xwindows clients
#define DO_AUTHENTICATION    37  // 1409 - IS/SEND/REPLY/NAME
#define DO_SUPDUP            21   // 736
#define DO_NEW_ENVIRON       39   // 1529

#define DO_COMPRESS          85  // mud client zlib compression on stream

#define DO_EXOPL            255  //*861 do extended option list
#define DO_RANDOMLOSE       256  // 748 option to gernerate random data loss
#define DO_SUBLIMINAL       257  // 1097 

#define MAX_DO_S ( sizeof( DoText ) / sizeof( struct do_text_tag ) )

struct do_text_tag 
{
	uint8_t dovalue;
	TEXTSTR dotext;
} DoText [] = {
	{DO_TB, "TB"}
	,{DO_ECHO, "ECHO"}
	,{DO_SGA, "SGA"}
	,{DO_STATUS, "STATUS"}
	,{DO_TIMINGMARK, "TIMINGMARK"}
	,{DO_BM, "BM"}
	,{DO_DET, "DET"}     
	,{DO_TERM_TYPE, "TERM_TYPE"}         
	,{DO_EOR, "EOR"}
	,{DO_TUID, "TUID"}
	,{DO_OUTMRK, "OUTMRK"}
	,{DO_TTYLOC, "TTYLOC"}  
	,{DO_3270_REGIME, "3270_REGIME"}      
	,{DO_X3_PAD, "X3_PAD"}          
	,{DO_NAWS, "NAWS"}
	,{DO_TERMSPEED, "TERMSPEED"}
	,{DO_FLOW_CONTROL, "FLOW_CONTROL"}
	,{DO_LINEMODE, "LINEMODE"}
	,{DO_X_DISPLAY_LOCATION, "X_DISPLAY_LOCATION"}
	,{DO_AUTHENTICATION, "AUTHENTICATION"}
	,{DO_SUPDUP, "SUPDUP"}
	,{DO_NEW_ENVIRON, "NEW_ENVIRON"}

	,{DO_COMPRESS, "COMPRESS"}

	,{DO_EXOPL, "EXOPL"}
	,{(TEXTCHAR)DO_RANDOMLOSE, "RANDOMLOSE"}
	,{(TEXTCHAR)DO_SUBLIMINAL, "SUBLIMINAL"}};
	
#define TWECHO     1
#define BREAKEVNT  2
static uint32_t status;

// this is used below during processing IAC codes
// it's a relic from obsolete code... but it may
// contain some hidden meaning that was missed....
//static int wwdd_val; // unsure of this value
//---------------------------------------------------------------------------

static int proc_iac(PMYDATAPATH pmdp, uint8_t tchar)
{
	// 0 data is just incomming use it
	// 1 we used the data don't do any thing with it
	// 3 send data....
	uint8_t i;
	LOGICAL retval=0;   /* assume we will use the character */
	//lprintf( "TCHAR: %d(%c) %d", tchar, tchar, pmdp->iac_count );

	/* don't start if not telnet and not an IAC character string */
	/* also need to filter out double IAC into a single IAC */
	if( tchar == IAC )
	{
		if( !pmdp->iac_count )
		{
			// start command...
			pmdp->iac_data[0]=tchar;
			pmdp->iac_count = 1;
			pmdp->flags.bLastWasIAC = 0;
			return 1;
		}
		if( !pmdp->flags.bLastWasIAC )
		{
			// receive 1, delete it....
			pmdp->flags.bLastWasIAC = 1;
			return 1;
		}
		// second IAC - continue on and process it.
		// and is in collection of IAC code...
		pmdp->flags.bLastWasIAC = 0;
	}

	if(pmdp->iac_count > 0)
	{
		retval = 1;
		if(pmdp->iac_count>255)
			pmdp->iac_count=255;
		//Log1( "Added byte: %02x", tchar );
		pmdp->iac_data[pmdp->iac_count++]=tchar;

      switch( (uint8_t)pmdp->iac_data[1])
		{
		case IAC:
			// displayln(traceout,"double iac on %hd\n",Channel);
			pmdp->iac_count=0;
			retval=0; /* return the character is yours */
			goto done;
		case DO:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,"DO   %n ",pmdp->iac_data[2]);
			if(pmdp->iac_data[2]== DO_ECHO )      /* ECHO */
			{
				if(status&TWECHO)
				{
					// displayln(traceout,"no response %n \n",pmdp->iac_data[2]);
					goto processed;/* ECHO IS CORRECT DONT RESPOND */
				}
				status|= TWECHO;
			reply_will:
				pmdp->iac_data[1]=WILL;
				goto send_iac;
			}
			else if(pmdp->iac_data[2]== DO_SGA ) /* SGA  */
			{
				goto reply_will;
			}
			else if( pmdp->iac_data[2] == DO_NAWS )
			{
				pmdp->flags.bDoNAWS = 1;
				pmdp->flags.bSendNAWS = 1;
				goto reply_will;
			}
			else
			{
				int n;
				for( n = 0; n < MAX_DO_S; n++ )
				{
					if( pmdp->iac_data[2] == DoText[n].dovalue )
					{
						Log1( "Server request for DO %s", DoText[n].dotext );
						break;
					}
				}
				if( n == MAX_DO_S )
					Log1( "Server request for unknown DO: %02X", pmdp->iac_data[2] );
			}
			goto sendwont;

		case DONT:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,"DONT %n ",pmdp->iac_data[2]);
			if(pmdp->iac_data[2]==DO_ECHO)           /* ECHO */
			{
				if( (status&TWECHO) ==0)
				{
					// displayln(traceout,"no response %n \n",pmdp->iac_data[2]);
					goto processed;/* ECHO IS CORRECT DONT RESPOND */
				}
				status&=~TWECHO;
			}
			else if(pmdp->iac_data[2]==DO_SGA)           /* SGA  */
			{
				goto reply_will;
			}
			else
			{
				int n;
				for( n = 0; n < MAX_DO_S; n++ )
				{
					if( pmdp->iac_data[2] == DoText[n].dovalue )
					{
						Log1( "Server request for DONT %s", DoText[n].dotext );
						break;
					}
				}
				if( n == MAX_DO_S )
					Log1( "Server request for unknown DONT: %02X", pmdp->iac_data[2] );
			}
		sendwont:
			pmdp->iac_data[1]=WONT;
			goto send_iac;

		case WILL:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,"WILL %n ",pmdp->iac_data[2]);
			if( pmdp->iac_data[2] == DO_SGA )
			{
				pmdp->iac_data[1] = DO;
				goto send_iac;
			}
			else if( pmdp->iac_data[2] == DO_EOR ) {
				// accept, and knowingly reject.
				// There is nothing different to do with or without this.
				/*
					As the EOR code indicates the end of an effective data unit, Telnet
					should attempt to send the data up to and including the EOR code
					together to promote communication efficiency.
				*/
				goto senddont;
			} 
			else {
				int n;
				for( n = 0; n < MAX_DO_S; n++ ) {
					if( pmdp->iac_data[2] == DoText[n].dovalue ) {
						Log1( "Server request for WILL %s", DoText[n].dotext );
						break;
					}
				}
				if( n == MAX_DO_S )
					Log1( "Server request for unknown WILL: %02X", pmdp->iac_data[2] );
			}
			goto senddont;
 
		case WONT:
			if(pmdp->iac_count<3)break; /* get entire option request */
			// displayln(traceout,"WONT %n ",pmdp->iac_data[2]);

		senddont:
			pmdp->iac_data[1]=DONT;

		send_iac:

			//if(*((short*)(&pmdp->iac_data[1])) == wwdd_val)
			//{
			//	goto processed;  /* don't resend anything*/
			//}

			//wwdd_val=*((short*)(&pmdp->iac_data[1]));

			i=pmdp->iac_data[1];

			switch(i)
			{
			case DO:
				Log( "response DO   ");
				break;
			case DONT:
				Log( "response DONT ");
				break;
			case WILL:
				Log( "response WILL ");
				break;
			case WONT:
				Log( "response WONT ");
				break;
			}
			//Log1("command data byte... %02d",pmdp->iac_data[2]);

			retval=3; /* we used the character and we need a write */
			goto processed;

		case SB:
			if(tchar==SE)goto unknown;
			break;

		case IACBK:
			status |= BREAKEVNT;
			// displayln(traceout,"iac break\n");
			goto processed;

		case NOP:
			// displayln(traceout,"iac nop\n");
			goto processed;
 
		case GA:
		case EL:
		case EC:
		case AYT:
		case AO:
		case IP:
		case DM:
		case SE:
		case EOR:
		default:
		unknown:

			// displayln(traceout,"iac on %hd =",Channel);
			// for (i=0;i<pmdp->iac_count;i++)
			//   displayln(traceout," %hd",pmdp->iac_data[i]);
			// displayln(traceout,"\n");

		processed:
			pmdp->iac_count=0;
		}
		goto done;
	}

done:
	return(retval);
}

//---------------------------------------------------------------------------

static PTEXT CPROC TelnetHandle( PDATAPATH pdPSI_CONTROL, PTEXT pText )
{
   PMYDATAPATH pdp = (PMYDATAPATH)pdPSI_CONTROL;
   INDEX idx;
   int nState;
   TEXTCHAR *ptext;
   PTEXT save;
   save = pText;
   nState = 0;
	//Log1( "Telnet input... %s", GetText( pText ) );
   for( save = pText; pText; pText = NEXTLINE(pText) )
   {
      ptext = GetText( pText );
      {
         for( idx = 0; idx < pText->data.size && ptext[idx]; idx++ )
         {
            switch( proc_iac( pdp, ptext[idx] ) )
            {
            case 0:
					VarTextAddCharacter( pdp->vt, ptext[idx] );
               break;
            case 3:
            	{
            		PTEXT responce = SegCreate( 3 );
            		MemCpy( responce->data.data, pdp->iac_data, 3 );
						responce->flags |= TF_BINARY;
						//Log3( "Enquing a responce... %02x %02x %02x"
						//	 , pdp->iac_data[0]
						//	 , pdp->iac_data[1]
						//	 , pdp->iac_data[2]  );
            		EnqueLink( &pdp->common.Output, responce );
						pdp->iac_count = 0;

						// also send initial packets....
						if( pdp->flags.bSendNAWS )
						{
							PTEXT send = SegCreate( 9 );
							TEXTCHAR *out = GetText( send );
							PTEXT val;
							int tmp;
							send->flags |= TF_BINARY;
							out[0] = IAC;
							out[1] = SB;
							out[2] = DO_NAWS;
							val = GetVolatileVariable( pdp->common.Owner->Current, "cols" );
							if( val )
								tmp = atoi( GetText( val ) );
							else
								tmp = 80;
							out[3] = ( tmp & 0xFF00 ) >> 8;
							out[4] = tmp & 0xFF;

							val = GetVolatileVariable( pdp->common.Owner->Current, "rows" );
							if( val )
								tmp = atoi( GetText( val ) );
							else
								tmp = 25;
							out[5] = ( tmp & 0xFF00 ) >> 8;
							out[6] = tmp & 0xFF;

							out[7] = IAC;
							out[8] = SE;
							EnqueLink( &pdp->common.Output, send );
							pdp->flags.bSendNAWS = 0;
						}
	            }
					break;
            case 1:
            	// should totally snag the character...
               // ptext[idx] = ' ';
               break;
            }
         }
      }
   }
	LineRelease( save );
   return VarTextGet( pdp->vt );
}

//---------------------------------------------------------------------------

static int CPROC Read( PDATAPATH pdp )
{
	return RelayInput( pdp, TelnetHandle );
}

//---------------------------------------------------------------------------

static PTEXT CPROC ValidateEOL( PDATAPATH pdPSI_CONTROL, PTEXT line )
{
   //PMYDATAPATH pdp = (PMYDATAPATH)pdPSI_CONTROL;
	PTEXT end, check;
	// outbound return is added at the end of the line.
	// so if there is no return, don't add one...
	if( !line || ( line->flags & TF_NORETURN ) )
		return line;
	end = line;
	SetEnd( end );
	check = end;
	while( check && ( check->flags & TF_INDIRECT ) )
	{
		check = GetIndirect( check );
	}
	// if check == NULL the last segment is a indirect with 0 content...

	if( !check ||
	    GetTextSize( end ) && !(end->flags&TF_BINARY) )
	{
		//PTEXT junk = BuildLine( line );
		//Log1( "Adding a end of line before shipping to network... %s", GetText( junk ) );
		//LineRelease( junk );
		SegAppend( end, SegCreate( 0 ) );
	}	
	return line;
}

//---------------------------------------------------------------------------

static int CPROC Write( PDATAPATH pdp )
{
	return RelayOutput( pdp, ValidateEOL );
}

//---------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdPSI_CONTROL )
{
   PMYDATAPATH pdp = (PMYDATAPATH)pdPSI_CONTROL;
	VarTextDestroy( &pdp->vt );
   pdp->common.Type = 0;
   return 0;
}

//---------------------------------------------------------------------------

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PMYDATAPATH pdp = CreateDataPath( pChannel, MYDATAPATH );
	pdp->common.Type = iTelnet;
	pdp->common.Read = Read;
	pdp->common.Write = Write;
	pdp->common.Close = Close;
	pdp->vt = VarTextCreate();
	return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	if( DekwareGetCoreInterface( DekVersion ) ) {
	iTelnet = RegisterDevice( "telnet", "Processes telnet IAC sequences", Open );
	   //return DekVersion;
	}
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void )
{
	UnregisterDevice( "telnet" );
}

// $Log: telnet.c,v $
// Revision 1.18  2005/02/21 12:08:40  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.17  2005/01/28 16:02:19  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.16  2005/01/18 02:47:01  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.15  2003/11/08 00:09:41  panther
// fixes for VarText abstraction
//
// Revision 1.14  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.13  2003/04/20 01:20:13  panther
// Updates and fixes to window-like console.  History, window logic
//
// Revision 1.12  2003/04/02 06:43:27  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.11  2003/03/31 14:47:05  panther
// Minor mods to filters
//
// Revision 1.10  2003/03/25 08:59:02  panther
// Added CVS logging
//
