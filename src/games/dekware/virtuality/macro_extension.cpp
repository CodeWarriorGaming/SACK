

#include "local.h"

struct macro_info_struct
{
	PMACROSTATE pms; // is NULL if not running
	PMACRO macro;
	PENTITY pe_running;
	LOGICAL running;  // have to clear running before it will run again..
	struct virtuality_object *vobj;
	LOGICAL stopping; // make sure we have to stop-stopping before running new.
};

// result to brain if this is running...
static NATIVE GetMacroRunning( uintptr_t psv )
{
	struct macro_info_struct *mis = (struct macro_info_struct *)psv;
	return (NATIVE)(( mis->pms != NULL ) || mis->running);
}

// Event callback from the InvokeMacroEx call
static void CPROC MacroEnded( uintptr_t psv, PMACROSTATE pms_ending )
{
	struct macro_info_struct *mis = (struct macro_info_struct *)psv;
	mis->pms = NULL;
}

static void StartRunMacro( uintptr_t psv, NATIVE value )
{
	if( value > 0 )
	{
		struct macro_info_struct *mis = (struct macro_info_struct *)psv;
		if( !mis->running && !mis->stopping )
		{
			mis->running = 1;
			mis->pms = InvokeMacroEx( mis->vobj->ps, mis->macro, NULL, MacroEnded, (uintptr_t)mis );
		}
	}
	else
	{
		struct macro_info_struct *mis = (struct macro_info_struct *)psv;
		mis->running = 0;
	}
}

static void StopRunMacro( uintptr_t psv, NATIVE value )
{
	{
		if( value > 0 )
		{
			struct macro_info_struct *mis = (struct macro_info_struct *)psv;
			if( mis->pms && !mis->stopping )
			{
				TerminateMacro( mis->pms );
			}
			mis->stopping = 1;
		}
		else
		{
			struct macro_info_struct *mis = (struct macro_info_struct *)psv;
			if( mis )
				mis->stopping = 0;
		}
	}
}

static void ObjectMacroCreated( "Point Label", "Brain Interface", "Event on Add a macro" )(PENTITY pe_object, PMACRO macro)
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_object->pPlugin, l.extension );
	if( vobj )
	{
		PVARTEXT pvt = VarTextCreate();
		vtprintf( pvt, "Run Macro %s", GetText( GetName( macro ) ) );

		PBRAIN_STEM pbs = new BRAIN_STEM( GetText( VarTextPeek( pvt ) ) );
		vobj->brain->AddBrainStem( pbs );

		{
			struct macro_info_struct *mis = New( struct macro_info_struct );
			mis->pms = NULL;
			mis->macro = macro;
			mis->pe_running = pe_object;
			mis->running = 0;
			mis->stopping = 0;
			mis->vobj = vobj;
			pbs->AddInput( new value(GetMacroRunning, (uintptr_t)mis ), "Is Running" );
			pbs->AddOutput( new value(StartRunMacro, (uintptr_t)mis ), "Start" );
			pbs->AddOutput( new value(StopRunMacro, (uintptr_t)mis ), "Stop" );
		}
	}
}


static void ObjectMacroDestroyed( "Point Label", "Brain Interface", "Event on deletion of a macro" )(PENTITY pe_object, PMACRO macro)
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_object->pPlugin, l.extension );
	if( vobj )
	{
		PVARTEXT pvt = VarTextCreate();
		vtprintf( pvt, "Run Macro %s", GetText( GetName( macro ) ) );
		PBRAIN_STEM pbs = vobj->brain->GetBrainStem( GetText( VarTextPeek( pvt ) ) );
		vobj->brain->RemoveBrainStem( pbs );
		delete( pbs );
	}
}
