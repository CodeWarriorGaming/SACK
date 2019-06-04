#define DEFINES_DEKWARE_INTERFACE

#include <stdhdrs.h>
#include "plugin.h"

#include <controls.h>

#define MAIN_SOURCE
#include "global.h"

FunctionProto AddControl
				  , Result
				  , DoHideCommon
				  , DoShowCommon
				  , DoEditCommon
				  , DoSaveCommon
	;

/*
command_entry dialog_commands[] = { { DEFTEXT( "display" ), 4, 7, DEFTEXT( "Show the frame" ), DoShowCommon }
											 , { DEFTEXT( "hide" ), 4, 4, DEFTEXT( "Hide the frame" ), DoHideCommon }
											 , { DEFTEXT( "edit" ), 4, 4, DEFTEXT( "Edit the frame" ), DoEditCommon }
											 , { DEFTEXT( "save" ), 4, 4, DEFTEXT( "save the frame as a file" ), DoSaveCommon }
};
*/

static int ObjectMethod( "psi_control", "save", "Save Control (frame)" )
//int DoSaveCommon
( PSENTIENT ps, PENTITY pe, PTEXT params )
{
	PTEXT filename = GetFileName( ps, &params );
	if( filename )
	{
		PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
		PSI_CONTROL pc = pct->control.pc;
		if( pc )
		{
			if( SaveXMLFrame( pc, GetText( filename ) ) )
			{
				if( ps->CurrentMacro )
					ps->CurrentMacro->state.flags.bSuccess = 1;
			}
			else
				if( !ps->CurrentMacro )
				{
					S_MSG( ps, "Failed to save control." );
				}
		}
	}
	else
	{
		if( !ps->CurrentMacro )
		{
			S_MSG( ps, "Must supply a filename to write control to." );
		}
	}
	return 0;
}
//--------------------------------------------------------------------------

static int ObjectMethod( "psi_control", "edit", "Edit Control (frame)" )
//int DoEditCommon
( PSENTIENT ps, PENTITY pe, PTEXT params )
{
	PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PSI_CONTROL pc = pct->control.pc;
	if( pc )
	{
		TEXTCHAR *option = GetText( GetParam( ps, &params ) );
		if( !option )
			option = "0";
		EditFrame( GetFrame( pc ), atoi( option ) );
	}
	return 1;
}

//--------------------------------------------------------------------------
static int ObjectMethod( "psi_control", "hide", "Hide Control (frame)" )
//int DoHideCommon
	( PSENTIENT ps, PENTITY pe, PTEXT params )
{
	PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PSI_CONTROL pc = pct->control.pc;
	if( pc )
	{
		HideControl( pc );
	}
	return 1;
}

//--------------------------------------------------------------------------

static int ObjectMethod( "psi_control", "show", "Show Control (frame)" )
//int DoShowCommon
( PSENTIENT ps, PENTITY pe, PTEXT params )
{
	PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)GetLink( &ps->Current->pPlugin, g.iCommon );
	PSI_CONTROL pc = pct->control.pc;
	if( pc )
	{
		DisplayFrame( pc );
	}
	return 1;
}

//--------------------------------------------------------------------------

void DestroyAControl( PENTITY pe )
{
	//PSI_CONTROL pc = GetLink( &pe->pPlugin, g.iCommon );
	PSI_CONTROL_TRACKER pComTrack = (PSI_CONTROL_TRACKER)GetLink( &pe->pPlugin, g.iCommon );
	if( pComTrack )
	{
		DeleteLink( &g.pMyFrames, pe );
		SetLink( &pe->pPlugin, g.iCommon, NULL );
		if( pComTrack->flags.created_internally )
		{
			if( !pComTrack->flags.menu )
				DestroyCommon( &pComTrack->control.pc );
			else
				DestroyPopup( pComTrack->control.menu );
		}
		Release( pComTrack );
	}
}

//--------------------------------------------------------------------------

static int CPROC CreatePopupThing( PSENTIENT ps, PENTITY peNew, PTEXT params )
{
	PMENU menu = CreatePopup();
	PSI_CONTROL_TRACKER pComTrack = New( COMMON_TRACKER );
	pComTrack->control.menu = menu;
	pComTrack->flags.created_internally = 1;
	pComTrack->flags.menu = 1;
	SetLink( &ps->Current->pPlugin, g.iCommon, (POINTER)pComTrack );
	// add some methods to add menu items, and to
	// issue the popup dialog....
	// also these popup objects need to be able to be given
	// to things like listboxes for contect menus on items.
	UnlockAwareness( CreateAwareness( peNew ) );

	return 0;
}

//--------------------------------------------------------------------------

int IsOneOfMyFrames( PENTITY pe )
{
	PENTITY peCheck;
	INDEX idx;
	LIST_FORALL( g.pMyFrames, idx, PENTITY, peCheck )
	{
		if( pe == peCheck )
         return TRUE;
	}
	return FALSE;
}

static PTEXT CPROC SetCaption( uintptr_t psv
							  , struct entity_tag *pe
							  , PTEXT newvalue )
{
	{
		PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)psv;//GetLink( &pe->pPlugin, g.iCommon );
      PSI_CONTROL pc = pct->control.pc;
		if( pc )
		{
			PTEXT line = BuildLine( newvalue );
			SetControlText( pc, GetText( line ) );
         LineRelease( line );
		}
	}
   return NULL;
}

static void DumpMacro( PVARTEXT vt, PMACRO pm, TEXTCHAR *type )
{
	PTEXT pt;
   INDEX idx;
	vtprintf( vt, "/%s \"%s\"", type, GetText( GetName(pm) ) );
	{
		PTEXT param;
		param = pm->pArgs;
		while( param )
		{
			vtprintf( vt, " %s", GetText( param ) );
			param = NEXTLINE( param );
		}
	}
   vtprintf( vt, ";" );
	LIST_FORALL( pm->pCommands, idx, PTEXT, pt )
	{
		PTEXT x;
		x = BuildLine( pt );
		vtprintf( vt, "%s;", GetText( x ) );
		LineRelease( x );
	}
}

//--------------------------------------------------------------------------

static void DumpMacros( PVARTEXT vt, PENTITY pe )
{
	INDEX idx;
	PMACRO pm;
	PMACRO behavior;
	LIST_FORALL( pe->pGlobalBehaviors, idx, PMACRO, behavior )
	{
		DumpMacro( vt, behavior, "on" );
	}
	LIST_FORALL( pe->pBehaviors, idx, PMACRO, behavior )
	{
		DumpMacro( vt, behavior, "on" );
	}
	LIST_FORALL( pe->pMacros, idx, PMACRO, pm )
		DumpMacro( vt, pm, "macro" );
}

//--------------------------------------------------------------------------

 PENTITY GetOneOfMyFrames( PSI_CONTROL pc )
{
	PENTITY peCheck;
	INDEX idx;
	LIST_FORALL( g.pMyFrames, idx, PENTITY, peCheck )
	{
		// this can accidentally create a list which will create a memory leak.
		if( peCheck->pPlugin )
		{
			PSI_CONTROL_TRACKER pctCheck = (PSI_CONTROL_TRACKER)GetLink( &peCheck->pPlugin, g.iCommon );
			// and 099.99% of the time it won't be... fuhk
			//lprintf( "this list may be orphaned... %p", pctCheck );
			if( pctCheck )
			{
				PSI_CONTROL pcCheck = pctCheck->control.pc;
				if( pc == pcCheck )
				{
					return peCheck;
				}
			}
		}
	}
   lprintf( "Failed to find control... entity this time..." );
   return NULL;
}

//--------------------------------------------------------------------------

int CPROC SaveCommonMacroData( PSI_CONTROL pc, PVARTEXT pvt )
{
	PSI_CONTROL pcFrame = GetFrame( pc );
	PENTITY peFrame = (PENTITY)GetOneOfMyFrames( pcFrame );
	if( peFrame && IsOneOfMyFrames( peFrame ) )
	{
		// gather the script components attached on behaviors
		DumpMacros( pvt, (PENTITY)GetOneOfMyFrames( pc ) );
      return 1;
	}
   return 0;
}

#if 0
static volatile_variable_entry common_vars[] = { { DEFTEXT("caption")
																 , NULL
																 , SetCaption }
															  , { DEFTEXT( "x" )
																 , NULL, NULL }
															  , { DEFTEXT( "y" )
																 , NULL, NULL }
															  , { DEFTEXT( "width" )
																 , NULL, NULL }
															  , { DEFTEXT( "height" )
																 , NULL, NULL }
};
#endif

//--------------------------------------------------------------------------
PENTITY CommonInitControl( PSI_CONTROL pc )
{
	PENTITY peNew = NULL;
	EnterCriticalSec( &g.csCreating );
	{
		PSI_CONTROL_TRACKER pComTrack = New( COMMON_TRACKER );
		PSI_CONTROL pcFrame = GetParentControl( pc );
		pComTrack->control.pc = pc;
		pComTrack->flags.created_internally = 0;
		pComTrack->flags.menu = 0;
		if( g.peCreating )
		{
			pComTrack->flags.created_internally = 1;
			peNew = g.peCreating;
			g.peCreating = NULL;
			SetLink( &peNew->pPlugin, g.iCommon, (POINTER)pComTrack );
		}
		else
		{
			PENTITY peFrame = GetOneOfMyFrames( pc );
			if( !peFrame )
			{
				// need to validate that this IS a PE frame and not
				// something like the pointer to the object which handles
				// locking for common buttons...
				PTEXT name = SegCreateFromText( GetControlTypeName( pc ) );
				while( pcFrame && !( peFrame = (PENTITY)GetOneOfMyFrames( pcFrame ) ) )
				{
					pcFrame = GetParentControl( pcFrame );
				}
				peNew = CreateEntityIn( peFrame, name );
				if( peNew )
				{
					SetLink( &peNew->pPlugin, g.iCommon, (POINTER)pComTrack );
					AddLink( &peNew->pDestroy, DestroyAControl );

					// AddMethod( peNew, dialog_commands );
					// AddMethod( peNew, dialog_commands+1 );
					// AddMethod( peNew, dialog_commands+2 );

					AddLink( &g.pMyFrames, peNew );
				}
				else
               lprintf( "Failed to create entity for control..." );
			}
			ScanRegisteredObjects( peNew, "psi_control" );
			// this needs to be done so that buttons may perform 'on' commands.
			//UnlockAwareness( CreateAwareness( peNew ) );
			LeaveCriticalSec( &g.csCreating );
			return NULL;
		}
	}
	LeaveCriticalSec( &g.csCreating );
	return peNew;
}


static int OnCreateObject( "psi_control", "generic control..." )
//static int CPROC AddAControl
( PSENTIENT ps, PENTITY peNew, PTEXT params )
{
	PENTITY peFrame = FindContainer( peNew );
	PSI_CONTROL_TRACKER pct = (PSI_CONTROL_TRACKER)GetLink( &peFrame->pPlugin, g.iCommon );
	PSI_CONTROL pc;
	TEXTCHAR *control_type;
	if( pct )
	{
		pc = pct->control.pc;
	}
	else
		pc = NULL;
	//if( pc )
	//{
		//lprintf( "hmm... create a child of the frame... but within or without?" );
      // abort!
   //   return 0;
	//}
	//else
	if( !pc )
	{
		control_type = GetText( GetParam( ps, &params ) );
		if( !control_type )
		{
			int first = 1;
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			PVARTEXT pvt = VarTextCreate();
			vtprintf( pvt, "Must specify type of control to make: " );
			for( name = GetFirstRegisteredName( "psi/control", &data );
				 name;
				  name = GetNextRegisteredName( &data ) )
			{
				int n;
				for( n = 0; name[n]; n++ )
					if( name[n] < '0' || name[n] > '9' )
						break;
				if( name[n] )
				{
					vtprintf( pvt, "%s%s", first?"":", ", name );
               first = 0;
				}
			}
			EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
			VarTextDestroy( &pvt );
			return 1;
		}

	}
	if( pc == NULL )
	{
		TEXTCHAR *caption = GetText( GetParam( ps, &params ) );
		if( caption )
		{
			const TEXTCHAR *px = GetText( GetParam( ps, &params ) )
		, *py = GetText( GetParam( ps, &params ) )
		, *pw = GetText( GetParam( ps, &params ) )
		, *ph = GetText( GetParam( ps, &params ) );
			if( !px ) px = "120";
			if( !py ) py = "50";
			if( !pw ) pw = "320";
			if( !ph ) ph = "250";
			{
				PSI_CONTROL pNewControl;
				EnterCriticalSec( &g.csCreating );
				g.peCreating = peNew;
				pNewControl = MakeNamedCaptionedControl( pc, control_type
																 , atoi( px ), atoi( py )
																 , atoi( pw ), atoi( ph )
																 , 0
																 , caption
																	);
				// named control have invoked "extra init" method, which
				// if all goes well we should be able to discover our parent
				//SetCommonUserData( pNewControl, (uintptr_t)peNew );
				AddLink( &g.pMyFrames, peNew );
				if( g.peCreating )
				{
					lprintf( "An extra init function did not clear g.peCreating.!" );
					DebugBreak();
					g.peCreating = NULL;
				}
				LeaveCriticalSec( &g.csCreating );
#if 0
				AddVolatileVariable( peNew, common_vars, (uintptr_t)pNewControl );
				AddVolatileVariable( peNew, common_vars+1, (uintptr_t)pNewControl );
				AddVolatileVariable( peNew, common_vars+2, (uintptr_t)pNewControl );
				AddVolatileVariable( peNew, common_vars+3, (uintptr_t)pNewControl );
				AddVolatileVariable( peNew, common_vars+4, (uintptr_t)pNewControl );
				AddMethod( peNew, dialog_commands );
				AddMethod( peNew, dialog_commands+1 );
				AddMethod( peNew, dialog_commands+2 );
				if( !pc )
					AddMethod( peNew, dialog_commands+3 );
#endif
				AddLink( &peNew->pDestroy, DestroyAControl );
				// add custom methods here?
				// add a init method?

				// these don't really need awareness?
				// they do need a method of invoking ON events...
				// maybe I can summon a parent awareness to do it's bidding...
				UnlockAwareness( CreateAwareness( peNew ) );
				// allow control to specify /on methods
				// or query the control registration tree for possibilities...
				// on click...
				return 0;
			}
		}
		else
		{
			DECLTEXT( msg, "Must supply a caption for the frame." );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return 1; // abort creation for now.
}

int CPROC CustomFrameInit( PSI_CONTROL pc )
{
	CommonInitControl( pc );
	return 1;
}

//--------------------------------------------------------------------------
static int CPROC CustomDefaultInit( PSI_CONTROL pc )
{
	CommonInitControl( pc );
	return 1;
}

//--------------------------------------------------------------------------
static int CPROC CustomDefaultDestroy( PSI_CONTROL pc )
{
	// registered as destroy of control...
   // predates On___ behaviors.
	PENTITY pe = GetOneOfMyFrames( pc );
	if( pe )
	{
		// in the course of destruction, DestroyCallbacks dispatched
		// which removes this from my list of things now.
		// since multiple paths of destroyentity might happen...
		DestroyEntity( pe );
	}
	return 1;
}

//--------------------------------------------------------------------------


PRELOAD( RegisterRoutines ) // PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	InitializeCriticalSec( &g.csCreating );
	if( DekwareGetCoreInterface( DekVersion ) ) {
	g.iCommon = RegisterExtension( "PSI Control" );
	////RegisterObject( "Frame", "Allows interface to Panther's Slick Interface dialogs", InitFrame );
	//RegisterObject( "Control", "Allows interface to Panther's Slick Interface dialogs", AddAControl );
	RegisterObject( "Menu", "A popup menu selector", CreatePopupThing );

	// this registers a default, if the control itself does not specify...
	SimpleRegisterMethod( "psi/control/rtti/extra init"
							  , CustomDefaultInit, "int", "dekware common init", "(PSI_CONTROL)" );
	SimpleRegisterMethod( "psi/control/rtti/extra destroy"
							  , CustomDefaultDestroy, "int", "dekware common destroy", "(PSI_CONTROL)" );

	SimpleRegisterMethod( "psi/control/" CONTROL_FRAME_NAME  "/rtti/extra init"
							  , CustomFrameInit, "int", "extra init", "(PSI_CONTROL)" );
	//return DekVersion;
	}
}

//--------------------------------------------------------------------------

PUBLIC( void, EditControlBehaviors)( PSI_CONTROL pc )
{
   // interesting wonder what this does...
}

//--------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterObject( "Control" );
}

//--------------------------------------------------------------------------
// $Log: dialog.c,v $
// Revision 1.6  2005/02/22 12:28:31  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.5  2005/01/17 09:01:13  d3x0r
// checkpoint ...
//
// Revision 1.4  2004/12/16 10:01:10  d3x0r
// Continue work on PSI plugin module for dekware, which is now within realm of grasp... need a couple enum methods
//
// Revision 1.3  2004/09/27 16:06:47  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.2  2003/01/13 08:47:27  panther
// *** empty log message ***
//
// Revision 1.1  2002/08/04 01:34:43  panther
// Initial commit.
//
//
