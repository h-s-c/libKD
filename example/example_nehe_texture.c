/*
 * This code was created by Jeff Molofee '99
 * (ported to Linux by Ti Leggett '01)
 * (ported to i.mx51, i.mx31 and x11 by Freescale '10)
 * If you've found this code useful, please let him know.
 *
 * Visit Jeff at http://nehe.gamedev.net/
 *
 */
 
#define EXAMPLE_COMMON_IMPLEMENTATION
#include "example_common.h"

GLuint       g_hShaderProgram      = 0;
GLuint       g_hModelViewMatrixLoc = 0;
GLuint       g_hProjMatrixLoc      = 0;
GLuint       g_hVertexLoc          = 0;
GLuint       g_hVertexTexLoc       = 2;
GLuint       g_hColorLoc           = 1;
GLuint       g_hVertexBuf          = 0;
GLuint       g_hVertexTexBuf       = 0;
GLuint       g_hColorBuf           = 0;

//--------------------------------------------------------------------------------------
// Name: g_strVertexShader / g_strFragmentShader
// Desc: The vertex and fragment shader programs
//--------------------------------------------------------------------------------------
const KDchar* g_strVertexShader = 
"uniform   mat4 g_matModelView;				\n"
"uniform   mat4 g_matProj;					\n"
"								\n"
"attribute vec4 g_vPosition;				\n"
"attribute vec3 g_vColor;					\n"
"attribute vec2 g_vTexCoord;				\n"
"								\n"
"varying   vec3 g_vVSColor;					\n"
"varying   vec2 g_vVSTexCoord;				\n"
"								\n"
"void main()						\n"
"{								\n"
"    vec4 vPositionES = g_matModelView * g_vPosition;	\n"
"    gl_Position  = g_matProj * vPositionES;		\n"
"    g_vVSColor = g_vColor;					\n"
"    g_vVSTexCoord = g_vTexCoord;				\n"
"}								\n";


const KDchar* g_strFragmentShader = 
	"#ifdef GL_FRAGMENT_PRECISION_HIGH						\n"
	"   precision highp float;								\n"
	"#else													\n"
	"   precision mediump float;							\n"
	"#endif													\n"
	"														\n"
	"uniform sampler2D s_texture;							\n"
	"varying   vec3      g_vVSColor;						\n"
	"varying   vec2 g_vVSTexCoord;							\n"
	"														\n"
	"void main()											\n"
	"{														\n"
	"    gl_FragColor = texture2D(s_texture,g_vVSTexCoord);	\n"
	"}														\n";

KDfloat32 VertexPositions[]={
	/* Draw A Quad */
	/* Top Right Of The Quad (Top) */
	1.0f, 1.0f,-1.0f,
	/* Top Left Of The Quad (Top) */
	-1.0f, 1.0f, -1.0f,
	/* Bottom Right Of The Quad (Top) */
	1.0f,1.0f,1.0f,
	/* Bottom Left Of The Quad (Top) */
	-1.0f,1.0f,1.0f,
	/* Top Right Of The Quad (Bottom) */
	1.0f,-1.0f,1.0f,
	/* Top Left Of The Quad (Bottom) */
	-1.0f, -1.0f,1.0f,
	/* Bottom Right Of The Quad (Bottom) */
	1.0f,-1.0f,-1.0f,
	/* Bottom Left Of The Quad (Bottom) */
	-1.0f,-1.0f,-1.0f,
	/* Top Right Of The Quad (Front) */
	1.0f,1.0f,1.0f,
	/* Top Left Of The Quad (Front) */
	-1.0f,1.0f,1.0f,
	/* Bottom Right Of The Quad (Front) */
	1.0f,-1.0f,1.0f,
	/* Bottom Left Of The Quad (Front) */
	-1.0f,-1.0f,1.0f,
	/* Top Right Of The Quad (Back) */
	1.0f,-1.0f,-1.0f,
	/* Top Left Of The Quad (Back) */
	-1.0f,-1.0f,-1.0f,
	/* Bottom Right Of The Quad (Back) */
	1.0f, 1.0f, -1.0f,
	/* Bottom Left Of The Quad (Back) */
	-1.0f,1.0f,-1.0f,
	/* Top Right Of The Quad (Left) */
	-1.0f,1.0f,1.0f,
	/* Top Left Of The Quad (Left) */
	-1.0f,1.0f, -1.0f,
	/* Bottom Right Of The Quad (Left) */
	-1.0f,-1.0f,1.0f,
	/* Bottom Left Of The Quad (Left) */
	-1.0f,-1.0f,-1.0f,
	/* Top Right Of The Quad (Right) */
	1.0f,1.0f,-1.0f,
	/* Top Left Of The Quad (Right) */
	1.0f,1.0f,1.0f,
	/* Bottom Right Of The Quad (Right) */
	1.0f,-1.0f,-1.0f,
	/* Bottom Left Of The Quad (Right) */
	1.0f,-1.0f,1.0f
};

KDfloat32 VertexTexCoords[] = {
	/* Top Face */
	0.0f,0.0f,
	1.0f,0.0f,
	0.0f,1.0f,
	1.0f,1.0f,
	/* Bottom Face */
	1.0f,1.0f,
	0.0f,1.0f,
	0.0f,0.0f,
	1.0f,0.0f,
	/* Front Face */
	0.0f,0.0f,
	1.0f,0.0f,
	0.0f,1.0f,
	1.0f,1.0f,
	/* Back Face */
	1.0f,1.0f,
	0.0f,1.0f,
	1.0f,0.0f,
	0.0f,0.0f,
	/*left face*/
	0.0f,0.0f,
	1.0f,0.0f,
	0.0f,1.0f,
	1.0f,1.0f,
	/* Right face */
	0.0f,0.0f,
	1.0f,0.0f,
	0.0f,1.0f,
	1.0f,1.0f,
}; 

KDfloat32 VertexColors[] ={
	/* Red */
	1.0f, 0.0f, 0.0f, 1.0f,
	/* Red */
	1.0f, 0.0f, 0.0f, 1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f, 1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f, 1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f,1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f,1.0f,
	/* Red */
	1.0f, 0.0, 0.0f, 1.0f,
	/* Red */
	1.0f, 0.0, 0.0f, 1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f, 1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f, 1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f, 1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f, 1.0f,
	/* Red */
	1.0f, 0.0f, 0.0f,1.0f,
	/* Red */
	1.0f, 0.0f, 0.0f,1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f,1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f,1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f, 1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f, 1.0f,
	/* Red */
	1.0f, 0.0f, 0.0f, 1.0f,
	/* Red */
	1.0f, 0.0f, 0.0f, 1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f,1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f, 1.0f,
	/* Blue */
	0.0f, 0.0f, 1.0f,1.0f,
	/* Green */
	0.0f, 1.0f, 0.0f, 1.0f
};

GLuint texture[1]; /* Storage For One Texture ( NEW ) */

void render(KDint w, KDint h)
{
	static KDfloat32 fAngle = 0.0f;
	fAngle += 0.01f;

	// Rotate and translate the model view matrix
	KDfloat32 matModelView[16] = {0};
	matModelView[ 0] = +kdCosf( fAngle );
	matModelView[ 2] = +kdSinf( fAngle );
	matModelView[ 5] = 1.0f;
	matModelView[ 8] = -kdSinf( fAngle );
	matModelView[10] = +kdCosf( fAngle );
	matModelView[12] = 0.0f; //X
	matModelView[14] = -6.0f; //z
	matModelView[15] = 1.0f;

	// Build a perspective projection matrix
	KDfloat32 matProj[16] = {0};
	matProj[ 0] = kdCosf( 0.5f ) / kdSinf( 0.5f );
	matProj[ 5] = matProj[0] * (w/h) ;
	matProj[10] = -( 10.0f ) / ( 9.0f );
	matProj[11] = -1.0f;
	matProj[14] = -( 10.0f ) / ( 9.0f );

	// Clear the colorbuffer and depth-buffer
	glClearColor( 0.0f, 0.0f, 0.5f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Set some state
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	// Set the shader program
	glUseProgram( g_hShaderProgram );
	glUniformMatrix4fv( g_hModelViewMatrixLoc, 1, 0, matModelView );
	glUniformMatrix4fv( g_hProjMatrixLoc,      1, 0, matProj );

	// Bind the vertex attributes
	glBindBuffer(GL_ARRAY_BUFFER, g_hVertexBuf);
	glVertexAttribPointer( g_hVertexLoc, 3, GL_FLOAT, 0, 0, KD_NULL );
	glEnableVertexAttribArray( g_hVertexLoc );

	glBindBuffer(GL_ARRAY_BUFFER, g_hColorBuf);
	glVertexAttribPointer( g_hColorLoc, 4, GL_FLOAT, 0, 0, KD_NULL );
	glEnableVertexAttribArray( g_hColorLoc );

	glBindBuffer(GL_ARRAY_BUFFER, g_hVertexTexBuf);
	glVertexAttribPointer( g_hVertexTexLoc, 2, GL_FLOAT, 0, 0, KD_NULL );
	glEnableVertexAttribArray( g_hVertexTexLoc );

	/* Select Our Texture */
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	/* Drawing Using Triangle strips, draw triangle strips using 4 vertices */
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 20, 4);

	// Cleanup
	glDisableVertexAttribArray( g_hVertexLoc );
	glDisableVertexAttribArray( g_hColorLoc );
	glDisableVertexAttribArray( g_hVertexTexLoc );
}

KDint init(void)
{
	g_hShaderProgram = exampleCreateProgram(g_strVertexShader, g_strFragmentShader, KD_FALSE);

	// Init attributes BEFORE linking
	glBindAttribLocation( g_hShaderProgram, g_hVertexLoc,   "g_vPosition" );
	glBindAttribLocation( g_hShaderProgram, g_hColorLoc,    "g_vColor" );
	glBindAttribLocation( g_hShaderProgram, g_hVertexTexLoc,   "g_vTexCoord" );

	// Link the vertex shader and fragment shader together
	glLinkProgram( g_hShaderProgram );

	glGenBuffers(1, &g_hVertexBuf);
	glBindBuffer(GL_ARRAY_BUFFER, g_hVertexBuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPositions), VertexPositions, GL_STATIC_DRAW);

	glGenBuffers(1, &g_hColorBuf);
	glBindBuffer(GL_ARRAY_BUFFER, g_hColorBuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexColors), VertexColors, GL_STATIC_DRAW);

	glGenBuffers(1, &g_hVertexTexBuf);
	glBindBuffer(GL_ARRAY_BUFFER, g_hVertexTexBuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexTexCoords), VertexTexCoords, GL_STATIC_DRAW);

	// Get uniform locations
	g_hModelViewMatrixLoc = glGetUniformLocation( g_hShaderProgram, "g_matModelView" );
	g_hProjMatrixLoc      = glGetUniformLocation( g_hShaderProgram, "g_matProj" );

	//gen textures
	texture[0] = exampleLoadTexture("/data/lenna.png");
	return 1;
}

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    Example *example = exampleInit();
    init();

    // Main Loop
    while(example->run)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            switch(event->type)
            {
                case(KD_EVENT_QUIT):
                case(KD_EVENT_WINDOW_CLOSE):
                {
                    example->run = KD_FALSE;
                    break;
                }
                default:
                {
                    kdDefaultEvent(event);
                    break;
                }
            }
        }

        // Draw frame
	    EGLint width = 0; 
	    EGLint height = 0;
	    eglQuerySurface(example->egl.display, example->egl.surface, EGL_WIDTH, &width);
	    eglQuerySurface(example->egl.display, example->egl.surface, EGL_HEIGHT, &height);
        render(width, height);
        exampleRun(example);
    }

    return exampleDestroy(example);
}

