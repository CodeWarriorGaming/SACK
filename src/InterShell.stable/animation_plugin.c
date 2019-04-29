#if 0
#include <sack_types.h>
#include <deadstart.h>
#include "intershell_local.h"
#include "intershell_registry.h"
#include "menu_real_button.h"
#include "resource.h"
#include "fonts.h"
#include "animation.h"

#define ANIMATION_BUTTON_NAME "Animation"


typedef struct {

	TEXTSTR animation_name;

	uint32_t x;				//position and size of played animation
	uint32_t y;
	uint32_t w;
	uint32_t h;

	PMNG_ANIMATION 	animation;
	
	PMENU_BUTTON pmb;
   PLIST buttons;

} ANIMATION_INFO, *PANIMATION_INFO;


ANIMATION_INFO l;


enum {
	EDIT_ANIMATION_NAME = 1200
	, EDIT_X
	, EDIT_Y 
	, EDIT_W 
	, EDIT_H 
};

//---------------------------------------------------------------------------
PRELOAD( RegisterAnimationDialogResources )
{

	EasyRegisterResource( "intershell/animation", EDIT_ANIMATION_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_X, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_Y, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_W, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/animation", EDIT_H, EDIT_FIELD_NAME );

}

//---------------------------------------------------------------------------
OnCreateMenuButton( ANIMATION_BUTTON_NAME )( PMENU_BUTTON common )
{
   PANIMATION_INFO info = ( PANIMATION_INFO )Allocate( sizeof( *info ) );
   MemSet(info, 0, sizeof(*info));
  
	info->pmb = common;
	{
		//uint32_t w, h;
		//GetFrameSize( InterShell_GetButtonControl( common ), &w, &h );
		info->w = common->w;
		info->h = common->h;
	}
//   lprintf("OnCreateMenuButton I am here!!!");
   AddLink( &l.buttons, info );
   return (uintptr_t)info;
}


//---------------------------------------------------------------------------
static void ConfigureAnimationButton( PANIMATION_INFO info, PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "ConfigureAnimationButton.isFrame" );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		char buffer[256];

		SetCommonButtons( frame, &done, &okay );

		SetControlText( GetControl( frame, EDIT_ANIMATION_NAME ), info->animation_name );

		snprintf(buffer, sizeof(buffer), "%d", info->x);
		SetControlText( GetControl( frame, EDIT_X ), buffer );

		snprintf(buffer, sizeof(buffer), "%d", info->y);
		SetControlText( GetControl( frame, EDIT_Y ), buffer );

		snprintf(buffer, sizeof(buffer), "%d", info->w);
		SetControlText( GetControl( frame, EDIT_W ), buffer );

		snprintf(buffer, sizeof(buffer), "%d", info->h);
		SetControlText( GetControl( frame, EDIT_H ), buffer );

		SetCommonButtonControls( frame );

		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			char buffer[256];
			GetCommonButtonControls( frame );
			GetControlText( GetControl( frame, EDIT_ANIMATION_NAME ), buffer, sizeof( buffer ) );
			if( info->animation_name )
				Release( info->animation_name );
			info->animation_name = StrDup( buffer );

			GetControlText( GetControl( frame, EDIT_X), buffer, sizeof( buffer ) );
			info->x = atoi(buffer);
			GetControlText( GetControl( frame, EDIT_Y), buffer, sizeof( buffer ) );
			info->y = atoi(buffer);
			GetControlText( GetControl( frame, EDIT_W), buffer, sizeof( buffer ) );
			info->w = atoi(buffer);
			GetControlText( GetControl( frame, EDIT_H), buffer, sizeof( buffer ) );
			info->h = atoi(buffer);


		}
		DestroyFrame( &frame );
	}
}

//---------------------------------------------------------------------------
OnEditControl( ANIMATION_BUTTON_NAME )( uintptr_t psv, PSI_CONTROL parent )
{
	ConfigureAnimationButton( (PANIMATION_INFO)psv, parent );
	return psv;
}


//---------------------------------------------------------------------------
static uintptr_t CPROC SetAnimationName( uintptr_t psv, arg_list args )
{
//	PANIMATION_INFO info = (PANIMATION_INFO)psv;


	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, CTEXTSTR, name );
	//lprintf("SetAnimationName i am here %s %X", name, name);
if( info )
	info->animation_name = StrDup( name );

	return psv;
}

//---------------------------------------------------------------------------
static uintptr_t CPROC SetAnimationXValue( uintptr_t psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, int32_t, x );
if( info )
	info->x = x;

	return psv;
}

//---------------------------------------------------------------------------
static uintptr_t CPROC SetAnimationYValue( uintptr_t psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, int32_t, y );
if( info )
	info->y = y;
	return psv;
}
//---------------------------------------------------------------------------
static uintptr_t CPROC SetAnimationWValue( uintptr_t psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, int32_t, w );
if( info )
	info->w = w;
	return psv;
}
//---------------------------------------------------------------------------
static uintptr_t CPROC SetAnimationHValue( uintptr_t psv, arg_list args )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;
	PARAM( args, int32_t, h );
if( info )
	info->h = h;
	return psv;
}


//---------------------------------------------------------------------------
void WriteAnimationButton( CTEXTSTR leader, FILE *file, uintptr_t psv )
{
	PANIMATION_INFO info = (PANIMATION_INFO)psv;

	sack_fprintf( file, "Animation name =%s\n", info->animation_name?info->animation_name:"" );
	sack_fprintf( file, "Animation X =%d\n", info->x );
	sack_fprintf( file, "Animation Y =%d\n", info->y );
	sack_fprintf( file, "Animation W =%d\n", info->w );
	sack_fprintf( file, "Animation H =%d\n", info->h );

}

//---------------------------------------------------------------------------
void ReadAnimationButton( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "Animation name =%m", SetAnimationName );
	AddConfigurationMethod( pch, "Animation X =%i", SetAnimationXValue );
	AddConfigurationMethod( pch, "Animation Y =%i", SetAnimationYValue );
	AddConfigurationMethod( pch, "Animation W =%i", SetAnimationWValue );
	AddConfigurationMethod( pch, "Animation H =%i", SetAnimationHValue );
}



//---------------------------------------------------------------------------
OnSaveControl( ANIMATION_BUTTON_NAME )( FILE *file, uintptr_t psv )
{
	WriteAnimationButton( NULL, file, psv );
}


//---------------------------------------------------------------------------
OnLoadControl( ANIMATION_BUTTON_NAME )( PCONFIG_HANDLER pch, uintptr_t psv )
{

//   lprintf("OnLoadControl I am here!!!");

	ReadAnimationButton( pch, psv);
}

//---------------------------------------------------------------------------
OnGlobalPropertyEdit( ANIMATION_BUTTON_NAME )( PSI_CONTROL parent )
{

	ConfigureAnimationButton( &l, parent );
}


//---------------------------------------------------------------------------
OnSaveCommon( ANIMATION_BUTTON_NAME )( FILE *file )
{
	WriteAnimationButton( NULL, file, (uintptr_t)&l );
}

//---------------------------------------------------------------------------
OnLoadCommon( ANIMATION_BUTTON_NAME )( PCONFIG_HANDLER pch )
{
//   lprintf("OnLoadCommon I am here!!!   (sizeof(l)) is %lu", (sizeof(l)));

	MemSet( &l, 0, sizeof(l) );
	ReadAnimationButton( pch, (uintptr_t)&l);
}


//---------------------------------------------------------------------------
void PlayAnimation(PSI_CONTROL control)
/*
   Play animations 
*/
{
   //DebugBreak();
	if( l.animation )
	{
		DeInitAnimationEngine( l.animation );
		l.animation = NULL;
	}

	{
		INDEX idx;
		PANIMATION_INFO info;
		LIST_FORALL( l.buttons, idx, PANIMATION_INFO, info )
		{
			if( info->animation_name && info->animation_name[0] )
			{
				PCanvasData canvas = GetCanvas( NULL );

				info->animation = InitAnimationEngine();

				//		lprintf("PlayAnimation : %s %d %d %d %d", l.animation_name, l.x, l.y, l.w, l.h);

				if(info->animation)
					GenerateAnimation( info->animation, control, info->animation_name, PARTX(info->x), PARTY(info->y), PARTW( info->x, info->w ), PARTH( info->y, info->h ));
			}
		}
	}
}

#endif
