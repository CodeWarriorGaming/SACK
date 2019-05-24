#ifndef IMAGE_SHADER_EXTENSION_INCLUDED
#define IMAGE_SHADER_EXTENSION_INCLUDED

#define __need___va_list
#include <stdarg.h>

#if defined( USE_GLES2 ) || defined( __EMSCRIPTEN__ )
#include <GLES2/gl2.h>
#endif

#ifdef _MSC_VER
#include <GL/glew.h>
#include <GL/GLU.h>
#define CheckErr()  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
						lprintf( "err=%d (%" _cstring_f ")",err, gluErrorString( err ) ); \
				}                               
#define CheckErrf(f,...)  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
					lprintf( "err=%d " f,err,##__VA_ARGS__ ); \
				}                               
#else
#define CheckErr()  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
						lprintf( "err=%d ",err ); \
				}                               
#define CheckErrf(f,...)  				{    \
					GLenum err = glGetError();  \
					if( err )                   \
					lprintf( "err=%d "f,err,##__VA_ARGS__ ); \
				}                               
#endif

#ifdef __ANDROID__
#define va_list __va_list
#else
#define va_list va_list
#endif

#include <image3d.h>


IMAGE_NAMESPACE
typedef struct image_shader_tracker ImageShaderTracker;

struct image_shader_tracker
{
	struct image_shader_flags
	{
		BIT_FIELD set_matrix : 1; // flag indicating we have set the matrix; this is cleared at the beginning of each enable of a context
		BIT_FIELD failed : 1; // shader compilation failed, abort enable; and don't reinitialize
		BIT_FIELD set_modelview : 1;
	} flags;
	CTEXTSTR name;
	int glProgramId;
	int glVertexProgramId;
	int glFragProgramId;

	int eye_point;
	int position_attrib;
	int color_attrib;
	int projection;
	int worldview;
	int modelview;
	// initial setup for private data
	uintptr_t (CPROC*Setup)( uintptr_t );
	// Init is called to re-load the program within a GL context.
	void (CPROC*Init)( uintptr_t,PImageShaderTracker );
	uintptr_t psvSetup;
	uintptr_t psvInit;
	void (CPROC*Enable)( PImageShaderTracker,uintptr_t,va_list);
	void (CPROC*AppendTristrip )( struct image_shader_op *,int triangles,uintptr_t,va_list);
	void (CPROC*Output)( PImageShaderTracker tracker, uintptr_t, uintptr_t, int from, int to );
	void (CPROC*Reset)( PImageShaderTracker tracker, uintptr_t, uintptr_t );

	uintptr_t (CPROC*InitShaderOp)( PImageShaderTracker tracker, uintptr_t psvShader, int *existing_verts, va_list args );
};

struct image_shader_op
{
	struct image_shader_tracker *tracker;
	uintptr_t psvKey;
	int from;
	int to;
	//int depth_enabled;
};

struct image_shader_image_buffer
{
	GLboolean depth;
	struct image_shader_tracker *tracker;
	Image target;
	PLIST output;
	struct image_shader_op *last_op;
};

struct image_shader_image_buffer_op
{
	struct image_shader_image_buffer *image_shader_op;
	struct image_shader_op *last_op;
	uintptr_t psvKey;
};

PImageShaderTracker CPROC GetShaderInit( CTEXTSTR name, uintptr_t (CPROC*)(uintptr_t), void(CPROC*)(uintptr_t,PImageShaderTracker), uintptr_t );
#define GetShader(name) GetShaderInit( name, NULL, NULL, 0 )
void CPROC SetShaderModelView( PImageShaderTracker tracker, RCOORD *matrix );

int CPROC CompileShaderEx( PImageShaderTracker shader, char const *const *vertex_code, int vert_blocks, char const*const*frag_code, int frag_blocks, struct image_shader_attribute_order *, int nAttribs );
int CPROC CompileShader( PImageShaderTracker shader, char const *const*vertex_code, int vert_blocks, char const*const*frag_code, int frag_blocks );
void CPROC ClearShaders( void );

void CPROC EnableShader( PImageShaderTracker shader, ... );


// verts and a single color
uintptr_t CPROC SetupSuperSimpleShader( uintptr_t );
void CPROC InitSuperSimpleShader( uintptr_t psv, PImageShaderTracker tracker );

// verts, texture verts and a single texture
uintptr_t CPROC SetupSimpleTextureShader( uintptr_t );
void CPROC InitSimpleTextureShader( uintptr_t psv, PImageShaderTracker tracker );

// verts, texture_verts, texture and a single texture
uintptr_t CPROC SetupSimpleShadedTextureShader( uintptr_t );
void CPROC InitSimpleShadedTextureShader( uintptr_t psv, PImageShaderTracker tracker );

//
uintptr_t CPROC SetupSimpleMultiShadedTextureShader( uintptr_t );
void CPROC InitSimpleMultiShadedTextureShader( uintptr_t psv, PImageShaderTracker tracker );

void DumpAttribs( PImageShaderTracker tracker, int program );

void CloseShaders( struct glSurfaceData *glSurface );
void FlushShaders( struct glSurfaceData *glSurface );

struct shader_buffer *CPROC CreateShaderBuffer( int dimensions, int start_size, int expand_by );
void CPROC AppendShaderData( struct image_shader_op *op, struct shader_buffer *buffer, float *data );

void CPROC  SetShaderEnable( PImageShaderTracker tracker, void (CPROC*EnableShader)( PImageShaderTracker tracker, uintptr_t, va_list args ), uintptr_t psv );
void CPROC  SetShaderOpInit( PImageShaderTracker tracker, uintptr_t (CPROC*InitOp)( PImageShaderTracker tracker, uintptr_t, int *existing_verts, va_list args ) );
//void CPROC  SetShaderFlush( PImageShaderTracker tracker, void (CPROC*FlushShader)( PImageShaderTracker tracker, uintptr_t, uintptr_t, int from, int to ) );
void CPROC  SetShaderOutput( PImageShaderTracker tracker, void (CPROC*FlushShader)( PImageShaderTracker tracker, uintptr_t, uintptr_t, int, int  ) );
void CPROC  SetShaderReset( PImageShaderTracker tracker, void (CPROC*FlushShader)( PImageShaderTracker tracker, uintptr_t, uintptr_t  ) );
void CPROC  SetShaderAppendTristrip( PImageShaderTracker tracker, void (CPROC*AppendTriStrip)(  struct image_shader_op *op, int triangles, uintptr_t,va_list args ) );
void CPROC  AppendShaderTristripQuad( struct image_shader_op * op, ... );
void CPROC  AppendShaderTristrip( struct image_shader_op * op, int triangles, ... );

size_t CPROC AppendShaderBufferData( struct shader_buffer *buffer, float *data );

struct image_shader_op * CPROC BeginShaderOp(PImageShaderTracker tracker, ... );

struct image_shader_op * BeginImageShaderOp(PImageShaderTracker tracker, Image target, ... );
void AppendImageShaderOpTristrip( struct image_shader_op *op, int triangles, ... );

void SetShaderDepth( Image pImage, LOGICAL enable );
int GetShaderUniformLocation( PImageShaderTracker shader, const char *uniformName );
void SetUniform4f( int uniformId, float v1, float v2, float v3, float v4 );
void SetUniform4fv( int uniformId, int n, RCOORD *v1 );
void SetUniform3fv( int uniformId, int n, RCOORD *v1 );
void SetUniform1f( int uniformId, RCOORD v1 );
void SetUniformMatrix4fv( int uniformId, int n, int sign, RCOORD *v1 );


IMAGE_NAMESPACE_END
#endif
