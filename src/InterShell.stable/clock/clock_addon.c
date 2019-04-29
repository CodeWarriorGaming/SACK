#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#include <stdhdrs.h> //DebugBreak()
#include "resource.h"

#include <controls.h>
#include <psi/clock.h>

#include "intershell_export.h"
#include "intershell_registry.h"

//---------------------------------------------------------------------------
USE_PSI_CLOCK_NAMESPACE

enum {
	CHECKBOX_ANALOG = 4000
	  , CHECKBOX_DATE
	  , CHECKBOX_SINGLE_LINE
	  , CHECKBOX_DAY_OF_WEEK
	  , EDIT_BACKGROUND_IMAGE
	  , EDIT_ANALOG_IMAGE
	  , CHECKBOX_AM_PM
};

typedef struct clock_info_tag
{
	struct {
		BIT_FIELD bAnalog : 1;  // we want to set analog, first show will enable
		BIT_FIELD bSetAnalog : 1; // did we already set analog?
		BIT_FIELD bDate : 1;
		BIT_FIELD bDayOfWeek : 1;
		BIT_FIELD bSingleLine : 1;
		BIT_FIELD bAmPm : 1;
	} flags;

	CDATA backcolor, color;

	//SFTFont *font;

	/* this control may be destroyed and recreated based on other options */
	PSI_CONTROL control;
	TEXTSTR image_name;
	Image image;
	TEXTSTR analog_image_name;
	struct clock_image_thing image_desc;
} CLOCK_INFO, *PCLOCK_INFO;

PRELOAD( RegisterExtraClockConfig )
{
	EasyRegisterResource( "InterShell/" "Clock", CHECKBOX_DATE, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/" "Clock", CHECKBOX_DAY_OF_WEEK, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/" "Clock", CHECKBOX_ANALOG, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/" "Clock", CHECKBOX_DATE, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/" "Clock", EDIT_BACKGROUND_IMAGE, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/" "Clock", EDIT_ANALOG_IMAGE, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/" "Clock", CHECKBOX_AM_PM, RADIO_BUTTON_NAME );
}

static uintptr_t OnCreateControl("Clock")
/*uintptr_t CPROC CreateClock*/( PSI_CONTROL frame, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PCLOCK_INFO info = New( CLOCK_INFO );
	MemSet( info, 0, sizeof( *info ) );
	info->color = BASE_COLOR_WHITE;

	info->control = MakeNamedControl( frame
											  , "Basic Clock Widget"
											  , x
											  , y
											  , w
											  , h
											  , -1
											  );

	// none of these are accurate values, they are just default WHITE and nothing.
	InterShell_SetButtonColors( NULL, info->color, info->backcolor, 0, 0 );
	SetClockColor( info->control, info->color );
	SetClockBackColor( info->control, info->backcolor );
	// need to supply extra information about the image, location of hands and face in image
	// and the spots...
	//MakeClockAnalog( info->control );
	//info->font = InterShell_GetCurrentButtonFont();
	//if( info->font )
	//	SetCommonFont( info->control, (*info->font ) );
	// the result of this will be hidden...
	return (uintptr_t)info;
}

static void OnShowControl( "Clock" )( uintptr_t psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	SetClockAmPm( info->control, info->flags.bAmPm );
	SetClockDate( info->control, info->flags.bDate );
	SetClockDayOfWeek( info->control, info->flags.bDayOfWeek );
	SetClockSingleLine( info->control, info->flags.bSingleLine );
	StartClock( info->control );
}

static uintptr_t OnConfigureControl( "Clock" )( uintptr_t psv, PSI_CONTROL parent_frame )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	{
		PSI_CONTROL frame = NULL; 
		int okay = 0;
		int done = 0;
		if( !frame )
		{
			frame = LoadXMLFrameOver( parent_frame, "Clock_Properties.isFrame" );
			//frame = CreateFrame( "Clock Properties", 0, 0, 420, 250, 0, NULL );
			if( frame )
			{
				//MakeTextControl( frame, 5, 15, 120, 18, TXT_STATIC, "Text Color", 0 );
				//EnableColorWellPick( MakeColorWell( frame, 130, 15, 18, 18, CLR_TEXT_COLOR, info->color ), TRUE );
				SetCommonButtonControls( frame );
				SetCheckState( GetControl( frame, CHECKBOX_DATE ), info->flags.bDate );
				SetCheckState( GetControl( frame, CHECKBOX_SINGLE_LINE ), info->flags.bSingleLine );
				SetCheckState( GetControl( frame, CHECKBOX_DAY_OF_WEEK ), info->flags.bDayOfWeek );
				SetCheckState( GetControl( frame, CHECKBOX_ANALOG ), info->flags.bAnalog );
				SetCheckState( GetControl( frame, CHECKBOX_AM_PM ), info->flags.bAmPm );
				SetControlText( GetControl( frame, EDIT_ANALOG_IMAGE ), info->analog_image_name );
				SetControlText( GetControl( frame, EDIT_BACKGROUND_IMAGE ), info->image_name );
				//EnableColorWellPick( SetColorWell( GetControl( frame, CLR_TEXT_COLOR), page_label->color ), TRUE );
				//SetButtonPushMethod( GetControl( frame, CHECKBOX_ANALOG ), ChangeClockStyle );
				SetCommonButtons( frame, &done, &okay );
				DisplayFrameOver( frame, parent_frame );
				CommonWait( frame );
				if( okay )
				{
							TEXTCHAR buffer[256];
					GetCommonButtonControls( frame );
					//info->font = InterShell_GetCurrentButtonFont();
					//if( info->font )
									//SetCommonFont( info->control, (*info->font ) );
					info->color = GetColorFromWell( GetControl( frame, CLR_TEXT_COLOR ) );
					info->backcolor = GetColorFromWell( GetControl( frame, CLR_BACKGROUND ) );
					{
						GetControlText( GetControl( frame, EDIT_BACKGROUND_IMAGE ), buffer, sizeof( buffer ) );
						if( info->image_name )
							Release( info->image_name );
						info->image_name = StrDup( buffer );
					}
					{
						GetControlText( GetControl( frame, EDIT_ANALOG_IMAGE ), buffer, sizeof( buffer ) );
						if( info->analog_image_name )
							Release( info->analog_image_name );
						info->analog_image_name = StrDup( buffer );
					}
					SetClockColor( info->control, info->color );
					SetClockBackColor( info->control, info->backcolor );
					info->flags.bAmPm = GetCheckState( GetControl( frame, CHECKBOX_AM_PM ) );
					info->flags.bAnalog = GetCheckState( GetControl( frame, CHECKBOX_ANALOG ) );
					info->flags.bDate = GetCheckState( GetControl( frame, CHECKBOX_DATE ) );
					info->flags.bSingleLine = GetCheckState( GetControl( frame, CHECKBOX_SINGLE_LINE ) );
					info->flags.bDayOfWeek = GetCheckState( GetControl( frame, CHECKBOX_DAY_OF_WEEK ) );
					if( info->image )
						SetClockBackImage( info->control, info->image );
				}
				DestroyFrame( &frame );
			}
		}
	}
	return psv;
}


static void OnSaveControl( "Clock" )( FILE *file,uintptr_t psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	sack_fprintf( file, "%sClock color=%s\n"
			 , InterShell_GetSaveIndent()
	       , FormatColor( info->color )
			 );
	sack_fprintf( file, "%sClock back color=%s\n"
			 , InterShell_GetSaveIndent()
	       , FormatColor( info->backcolor )
			 );
	sack_fprintf( file, "%sClock background image=%s\n", InterShell_GetSaveIndent(), info->image_name?info->image_name:"" );
	sack_fprintf( file, "%sClock is analog?%s\n", InterShell_GetSaveIndent(), info->flags.bAnalog?"Yes":"No" );
	sack_fprintf( file, "%sClock is military time?%s\n", InterShell_GetSaveIndent(), (!info->flags.bAmPm)?"Yes":"No" );
	sack_fprintf( file, "%sClock show date?%s\n", InterShell_GetSaveIndent(), info->flags.bDate?"Yes":"No" );
	sack_fprintf( file, "%sClock is single line?%s\n", InterShell_GetSaveIndent(), info->flags.bSingleLine?"Yes":"No" );
	sack_fprintf( file, "%sClock show day of week?%s\n", InterShell_GetSaveIndent(), info->flags.bDayOfWeek?"Yes":"No" );

	sack_fprintf( file, "%sClock analog image=%s\n", InterShell_GetSaveIndent(), info->analog_image_name?info->analog_image_name:"images/Clock.png" );
	{
		TEXTSTR out;
		EncodeBinaryConfig( &out, &info->image_desc, sizeof( info->image_desc ) );
		sack_fprintf( file, "%sClock analog description=%s\n", InterShell_GetSaveIndent(), out );
		Release( out );
	}

	InterShell_SaveCommonButtonParameters( file );

}


static uintptr_t CPROC ReloadClockColor( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CDATA, color );
	info->color = color;
	return psv;
}

static uintptr_t CPROC ReloadClockBackColor( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CDATA, color );
	info->backcolor = color;
	return psv;
}

static uintptr_t CPROC SetClockAnalog( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, bAnalog );
	info->flags.bAnalog = bAnalog;
	return psv;
}
static uintptr_t CPROC ConfigSetClockAmPm( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bAmPm = !yes_no;
	return psv;
}
static uintptr_t CPROC ConfigSetClockDate( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bDate = yes_no;
	return psv;
}
static uintptr_t CPROC ConfigSetClockDayOfWeek( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bDayOfWeek = yes_no;
	return psv;
}
static uintptr_t CPROC ConfigSetClockSingleLine( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, LOGICAL, yes_no );
	info->flags.bSingleLine = yes_no;
	return psv;
}

static uintptr_t CPROC SetClockAnalogImage( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CTEXTSTR, name );
	info->analog_image_name = StrDup( name );

	return psv;
}
static uintptr_t CPROC SetClockBackgroundImage( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, CTEXTSTR, name );
	info->image_name = StrDup( name );

	return psv;
}
static uintptr_t CPROC SetClockAnalogImageDesc( uintptr_t psv, arg_list args )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	PARAM( args, size_t, size );
	PARAM( args, struct clock_image_thing *, desc );
	if( size == sizeof( struct clock_image_thing ) )
	{
		info->image_desc = (*desc);
	}
	else
	{
				lprintf( "size of struct was %d not %d", size, sizeof( struct clock_image_thing ) );
	}
	return psv;
}

static void OnLoadControl( "Clock" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	InterShell_AddCommonButtonConfig( pch );

	AddConfigurationMethod( pch, "Clock color=%c", ReloadClockColor );
	AddConfigurationMethod( pch, "Clock back color=%c", ReloadClockBackColor );
	AddConfigurationMethod( pch, "Clock is analog?%b", SetClockAnalog );
	AddConfigurationMethod( pch, "Clock is military time?%b", ConfigSetClockAmPm );
	AddConfigurationMethod( pch, "Clock show day of week?%b", ConfigSetClockDayOfWeek );
	AddConfigurationMethod( pch, "Clock show date?%b", ConfigSetClockDate );
	AddConfigurationMethod( pch, "Clock is single line?%b", ConfigSetClockSingleLine );
	AddConfigurationMethod( pch, "Clock analog image=%m", SetClockAnalogImage );
	AddConfigurationMethod( pch, "Clock background image=%m", SetClockBackgroundImage );
	AddConfigurationMethod( pch, "Clock analog description=%B", SetClockAnalogImageDesc );
}

static void OnFixupControl( "Clock" )(  uintptr_t psv )
//void CPROC FixupClock(  uintptr_t psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	if( info )
	{
		SetClockColor( info->control, info->color );
		SetClockBackColor( info->control, info->backcolor );
		InterShell_SetButtonColors( NULL, info->color, info->backcolor, 0, 0 );
	}
}

static PSI_CONTROL OnGetControl( "Clock" )( uintptr_t psv )
//PSI_CONTROL CPROC GetClockControl( uintptr_t psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	return info->control;
}

static void OnEditEnd( "Clock" )(uintptr_t psv )
//void CPROC ResumeClock( uintptr_t psv)
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	lprintf( "Break." );
	StartClock( info->control );
}

static void OnEditBegin( "Clock" )( uintptr_t psv )
//void CPROC PauseClock( uintptr_t psv)
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	//HideCommon( info->control );
	StopClock( info->control );
}

static LOGICAL OnQueryShowControl( "Clock" )( uintptr_t psv )
{
	PCLOCK_INFO info = (PCLOCK_INFO)psv;
	if( info->flags.bAnalog )
	{
		MakeClockAnalogEx( info->control, info->analog_image_name, &info->image_desc );
	}
	else
		MakeClockAnalogEx( info->control, NULL, NULL ); // hrm this should turn off the analog feature...
	return TRUE;
}

//---------------------------------------------------------------------------
#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
