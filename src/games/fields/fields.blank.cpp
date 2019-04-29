
// Source mostly from

#define MAKE_RCOORD_SINGLE
#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE

// define local instance.
#define TERRAIN_MAIN_SOURCE  
#include <vectlib.h>
#include <render.h>
#include <render3d.h>

#ifndef SOMETHING_GOES_HERE
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include "local.h"




static void OnDraw3d( "Field Interaction 1" )( uintptr_t psvView )
//static int OnDrawCommon( "Terrain View" )( PSI_CONTROL pc )
}

static void OnBeginDraw3d( "Field Interaction 1" )( uintptr_t psv,PTRANSFORM camera )
{
}

static void OnFirstDraw3d( "Field Interaction 1" )( uintptr_t psvInit )
{
	// and really if initshader fails, it sets up in local flags and 
	// states to make sure we just fall back to the old way.
	// so should load the classic image along with any new images.
	if (GLEW_OK != glewInit() )
	{
		// okay let's just init glew.
		return;
	}

	//InitPerspective();
	//InitShader();

}

static uintptr_t OnInit3d( "Field Interaction 1" )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	// keep the camera as a 
	return (uintptr_t)camera;
}

