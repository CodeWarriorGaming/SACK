
#include "widgets/include/banner.h"
#include "intershell_local.h"
#include "intershell_export.h"
#include "intershell_registry.h"

INTERSHELL_NAMESPACE

typedef struct banner_button BANNER_BUTTON, *PBANNER_BUTTON;

struct banner_button {
	struct {
		BIT_FIELD bTopmost : 1;
		BIT_FIELD forced_delay : 1; // also known as 'ignore clicks'
		BIT_FIELD allow_continue : 1; // also known as 'ignore clicks'
		BIT_FIELD yes_no : 1; // allow yesno on banner
		BIT_FIELD okay_cancel : 1; // allow yesno on banner
		BIT_FIELD explorer : 1; // banner only over where the explorer bar is...
		BIT_FIELD skip_if_lit : 1;
		BIT_FIELD skip_if_unlit : 1;
	} flags;
	uint32_t delay; // amount of time forced wait.
	CTEXTSTR text;
	CTEXTSTR imagename;
	Image image; // may add an image to the banner?
	PMENU_BUTTON button;
	PBANNER banner; // this sort of thing is a banner.
};

enum {
	EDIT_BANNER_TEXT = 3000
		, EDIT_BANNER_DELAY
		, CHECKBOX_TOPMOST
		, CHECKBOX_CONTINUE
		, CHECKBOX_YESNO
		, CHECKBOX_NOCLICK
		, CHECKBOX_OKAYCANCEL
		, EDIT_CONTROL_TEXT // used for macro element text
		, CHECKBOX_EXPLORER
		, CHECKBOX_SKIP_IF_LIT
		, CHECKBOX_SKIP_IF_UNLIT
};
static struct {
	PLIST banners; // active banner list.
} l;

PRELOAD( RegisterResources )
{
	EasyRegisterResource( "InterShell/banner", CHECKBOX_TOPMOST				, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/banner", CHECKBOX_CONTINUE				, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/banner", CHECKBOX_YESNO				, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/banner", CHECKBOX_OKAYCANCEL				, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/banner", CHECKBOX_NOCLICK			, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/banner", CHECKBOX_EXPLORER			, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/banner", CHECKBOX_SKIP_IF_LIT			, RADIO_BUTTON_NAME );
	EasyRegisterResource( "InterShell/banner", CHECKBOX_SKIP_IF_UNLIT		 , RADIO_BUTTON_NAME );

	EasyRegisterResource( "InterShell/banner", EDIT_BANNER_DELAY			  , EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/banner", EDIT_BANNER_TEXT			  , EDIT_FIELD_NAME );

	EasyRegisterResource( "InterShell/banner", EDIT_CONTROL_TEXT			  , EDIT_FIELD_NAME );
}

#define MAKE_BANNER_MESSAGE "Banner Message"
static uintptr_t OnCreateMenuButton( MAKE_BANNER_MESSAGE )( PMENU_BUTTON button )
{
	PBANNER_BUTTON banner = New( BANNER_BUTTON );
	MemSet( banner, 0, sizeof( *banner ) );
	banner->button = button;
	return (uintptr_t)banner;
}

static void OnKeyPressEvent( MAKE_BANNER_MESSAGE )( uintptr_t psvBanner )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	TEXTCHAR buffer[256];
	int yes_no;
	uint32_t timeout = 0;
	uint32_t delay = 0;
	if( banner->flags.skip_if_lit || banner->flags.skip_if_unlit )
	{
		if( InterShell_GetButtonHighlight( banner->button ) )
		{
			// it is lit
			if( banner->flags.skip_if_lit )
			{
				// if we ran to here, no need to change macro result.
				return;
			}
		}
		else
		{
			// it is not lit
			if( banner->flags.skip_if_unlit )
			{
				// if we ran to here, no need to change macro result.
				return;
			}
		}
	}
	if((banner->flags.explorer || !banner->flags.allow_continue)&&banner->delay)
		timeout = BANNER_TIMEOUT;
	if( !banner->flags.forced_delay && !banner->flags.yes_no && !banner->flags.okay_cancel && !banner->flags.allow_continue )
		timeout = BANNER_TIMEOUT;
	if( timeout && !banner->delay )
		delay = 2000;
	else
		delay = banner->delay;
	yes_no = CreateBanner2Ex( NULL, &banner->banner
								  , InterShell_TranslateLabelTextEx( banner->button, NULL, buffer, sizeof( buffer ), banner->text )
								  , (banner->flags.bTopmost?BANNER_TOP:0)
									| ((banner->flags.allow_continue&&(!banner->flags.yes_no))?BANNER_NOWAIT:0)
									| (banner->flags.forced_delay?BANNER_DEAD:BANNER_CLICK)
									| (banner->flags.explorer?BANNER_EXPLORER:0)
									| (banner->flags.yes_no?BANNER_OPTION_YESNO:0)
									| (banner->flags.okay_cancel?BANNER_OPTION_OKAYCANCEL:0)
									| (((banner->flags.explorer || !banner->flags.allow_continue)&&banner->delay)?BANNER_TIMEOUT:0)
								  , timeout?delay:0
									//((banner->flags.allow_continue&&!banner->flags.explorer)?((!banner->flags.forced_delay)?2000:0):banner->delay)
								  );

	if( !banner->flags.allow_continue )
	{
		if( banner->flags.yes_no || banner->flags.okay_cancel )
			SetMacroResult( yes_no );// set allow_continue to yes_no
		if( banner->banner )
			RemoveBanner2Ex( &banner->banner DBG_SRC );
	}
	else
	{
		// can have a whole slew of banners each with their own removes ?
		AddLink( &l.banners, banner );
	}
	//BannerMessage( "Yo, whatcha want!?" );
}

#define REMOVE_BANNER_MESSAGE "Banner Message Remove"
static uintptr_t OnCreateMenuButton( REMOVE_BANNER_MESSAGE )( PMENU_BUTTON button )
{
	// this button should only exist as an invisible/macro button....
	PBANNER_BUTTON banner = New( BANNER_BUTTON );
	MemSet( banner, 0, sizeof( *banner ) );
	banner->button = button;
	return (uintptr_t)banner;
}

static void OnKeyPressEvent( REMOVE_BANNER_MESSAGE )( uintptr_t psvBanner )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	INDEX idx;
	LIST_FORALL( l.banners, idx, PBANNER_BUTTON, banner )
	{
		RemoveBanner2( banner->banner );
		SetLink( &l.banners, idx, NULL );
	}
	//BannerMessage( "Yo, whatcha want!?" );
}

static uintptr_t OnConfigureControl( MAKE_BANNER_MESSAGE )( uintptr_t psvBanner, PSI_CONTROL parent )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "EditBannerMessage.isFrame" );
	if( frame )
	{
		TEXTCHAR buffer[256];
		int okay = 0;
		int done = 0;
		// some sort of check button for topmost
		// delay edit field
		// yesno
		// color?
		// image name?
		// if delay is set, make sure that the banner is unclickable.
		snprintf( buffer, sizeof( buffer ), "%d", banner->delay );
		SetControlText( GetControl( frame, EDIT_BANNER_DELAY ), buffer );
		ExpandConfigString( buffer, banner->text );
		SetCheckState( GetControl( frame, CHECKBOX_NOCLICK ), banner->flags.forced_delay );
		SetCheckState( GetControl( frame, CHECKBOX_SKIP_IF_LIT ), banner->flags.skip_if_lit );
		SetCheckState( GetControl( frame, CHECKBOX_SKIP_IF_UNLIT ), banner->flags.skip_if_unlit );
		SetCheckState( GetControl( frame, CHECKBOX_EXPLORER ), banner->flags.explorer );
		SetCheckState( GetControl( frame, CHECKBOX_TOPMOST ), banner->flags.bTopmost );
		SetCheckState( GetControl( frame, CHECKBOX_YESNO ), banner->flags.yes_no );
		SetCheckState( GetControl( frame, CHECKBOX_OKAYCANCEL ), banner->flags.okay_cancel );
		SetCheckState( GetControl( frame, CHECKBOX_CONTINUE ), banner->flags.allow_continue );
		SetControlText( GetControl( frame, EDIT_BANNER_TEXT ), buffer );
		{
			TEXTCHAR buffer[256];
			InterShell_GetButtonText( banner->button, buffer, sizeof( buffer ) );
			SetControlText( GetControl( frame, EDIT_CONTROL_TEXT ), buffer );
		}


		SetCommonButtons( frame, &done, &okay );
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			TEXTCHAR buffer[256];
			TEXTCHAR buffer2[256];
			GetControlText( GetControl( frame, EDIT_BANNER_DELAY ), buffer, sizeof( buffer ) );
			banner->delay = atoi( buffer );
			banner->flags.allow_continue = GetCheckState( GetControl( frame, CHECKBOX_CONTINUE ) );
			banner->flags.bTopmost = GetCheckState( GetControl( frame, CHECKBOX_TOPMOST ) );
			banner->flags.explorer = GetCheckState( GetControl( frame, CHECKBOX_EXPLORER ) );
			banner->flags.skip_if_lit = GetCheckState( GetControl( frame, CHECKBOX_SKIP_IF_LIT ) );
			banner->flags.skip_if_unlit = GetCheckState( GetControl( frame, CHECKBOX_SKIP_IF_UNLIT ) );
			banner->flags.yes_no = GetCheckState( GetControl( frame, CHECKBOX_YESNO ) );
			banner->flags.okay_cancel = GetCheckState( GetControl( frame, CHECKBOX_OKAYCANCEL ) );
			GetControlText( GetControl( frame, EDIT_BANNER_TEXT ), buffer, sizeof( buffer ) );
			banner->flags.forced_delay = GetCheckState( GetControl( frame, CHECKBOX_NOCLICK ) );
			StripConfigString( buffer2, buffer );
			banner->text = StrDup( buffer2 );

			GetControlText( GetControl( frame, EDIT_CONTROL_TEXT ), buffer, sizeof( buffer ) );
			InterShell_SetButtonText( banner->button, buffer );
		}
		DestroyFrame( &frame );

	}
	return psvBanner;
}



static void OnSaveControl( MAKE_BANNER_MESSAGE )( FILE *file, uintptr_t psvBanner )
{
	TEXTCHAR buffer[256];
	TEXTCHAR buffer2[256];
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	ExpandConfigString( buffer, banner->text );
	ExpandConfigString( buffer2, buffer );
	fprintf( file, "%sbanner text=%s\n", InterShell_GetSaveIndent(), buffer2 );
	fprintf( file, "%sbanner timeout=%d\n", InterShell_GetSaveIndent(), banner->delay );
	fprintf( file, "%sbanner continue=%s\n", InterShell_GetSaveIndent(), banner->flags.allow_continue?"yes":"no" );
	fprintf( file, "%sbanner force delay=%s\n", InterShell_GetSaveIndent(), banner->flags.forced_delay?"yes":"no" );
	fprintf( file, "%sbanner topmost=%s\n", InterShell_GetSaveIndent(), banner->flags.bTopmost?"yes":"no" );
	fprintf( file, "%sbanner yes or no=%s\n", InterShell_GetSaveIndent(), banner->flags.yes_no?"yes":"no" );
	fprintf( file, "%sbanner explorer=%s\n", InterShell_GetSaveIndent(), banner->flags.explorer?"yes":"no" );
	fprintf( file, "%sbanner okay or cancel=%s\n", InterShell_GetSaveIndent(), banner->flags.okay_cancel?"yes":"no" );
	fprintf( file, "%sbanner skip if button highlight=%s\n", InterShell_GetSaveIndent(), banner->flags.skip_if_lit?"yes":"no" );
	fprintf( file, "%sbanner skip if button normal=%s\n", InterShell_GetSaveIndent(), banner->flags.skip_if_unlit?"yes":"no" );
}


static uintptr_t CPROC ConfigSetBannerText( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, CTEXTSTR, text );
	TEXTCHAR buffer[256];
	StripConfigString( buffer, text );
	if( banner->text )
		Release( (POINTER)banner->text );
	banner->text = StrDup( buffer );
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerTimeout( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, int64_t, delay );
	if( delay < 0 )
		banner->delay = 0;
	else
		banner->delay = (uint32_t)delay;

	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerContinue( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, contin );
	banner->flags.allow_continue = contin;
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerForced( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, forced );
	banner->flags.forced_delay = forced;
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerTopmost( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, topmost );
	banner->flags.bTopmost = topmost;
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerYesNo( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, yesno );
	banner->flags.yes_no = yesno;
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerSkipLit( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, yesno );
	banner->flags.skip_if_lit = yesno;
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerSkipUnlit( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, yesno );
	banner->flags.skip_if_unlit = yesno;
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerExplorer( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, yesno );
	banner->flags.explorer = yesno;
	return psvBanner;
}

static uintptr_t CPROC ConfigSetBannerOkayCancel( uintptr_t psvBanner, arg_list args )
{
	PBANNER_BUTTON banner = (PBANNER_BUTTON)psvBanner;
	PARAM( args, LOGICAL, okaycancel );
	banner->flags.okay_cancel = okaycancel;
	return psvBanner;
}

static void OnLoadControl( MAKE_BANNER_MESSAGE )( PCONFIG_HANDLER pch, uintptr_t psvBanner )
{
	AddConfigurationMethod( pch, "banner text=%m", ConfigSetBannerText );
	AddConfigurationMethod( pch, "banner timeout=%i", ConfigSetBannerTimeout );
	AddConfigurationMethod( pch, "banner continue=%b", ConfigSetBannerContinue );
	AddConfigurationMethod( pch, "banner force delay=%b", ConfigSetBannerForced );
	AddConfigurationMethod( pch, "banner topmost=%b", ConfigSetBannerTopmost );
	AddConfigurationMethod( pch, "banner yes or no=%b", ConfigSetBannerYesNo );
	AddConfigurationMethod( pch, "banner explorer=%b", ConfigSetBannerExplorer );
	AddConfigurationMethod( pch, "banner okay or cancel=%b", ConfigSetBannerOkayCancel );
	AddConfigurationMethod( pch, "banner skip if button highlight=%b", ConfigSetBannerSkipLit );
	AddConfigurationMethod( pch, "banner skip if button normal=%b", ConfigSetBannerSkipUnlit );
}

INTERSHELL_NAMESPACE_END
