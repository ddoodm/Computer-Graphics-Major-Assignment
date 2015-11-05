/* ================================================	*\
 *	    UTS: Introduction to Computer Graphics		*
 *             Major Assignment | Part C			*
 *			         AUTUMN 2014					*
 * ================================================	*
 *													*
 *   AUTHORS.......: DEINYON DAVIES   (11688025)	*
 * 	                 YIANNIS CHAMBERS (11699156)	*
 *													*
 *	 REVISION......: 19 / MAY / 2014				*
 *													*
 *	 COMPILER......: MSVC++ PROFESSIONAL 2012		*
 *													*
 *	 INCLUDED LIBS.: OPENGL EXTENSION WRANGLER		*
\*													*/

#include <stdio.h>
#define _USE_MATH_DEFINES

#include <math.h>		// Mathematics constants and functions
#include <algorithm>	// std::copy()
#include <Windows.h>	// File dialogue
#include "glew.h"		// Extension wrangler library
#include <glut.h>		// OpenGL with GLUT

#include <vec3.h>		// Custom geometric vector library
#include <bmpLoader.h>	// Custom BMP image loader
#include <objLoader.h>	// Custom OBJ model loader
#include <programSelectMenu.h>

//////// VERTEX SHADER SOURCE ////////
static const char* vs_src = 
	"#version 330 core\n"

	"in vec4 vVertex;"
	"in vec4 vNormal;"
	"in vec2 tc;"
	"uniform mat4 mv_matrix;"
	"uniform mat4 proj_matrix;"

	"out VS_OUT { vec3 N; vec4 P; vec2 TC; } vs_out;"

	"void main(void) "
	"{"
	// Calculate view-space coordinate of the current vertex
	"  vs_out.P = mv_matrix * vVertex;"

	// Transform vertex normal into view-space
	"  vs_out.N = mat3(mv_matrix) * vec3(vNormal);"

	// Texture coordinate
	"  vs_out.TC = tc;"

	// Final coordinate is the clip space coordinate of P
	"  gl_Position = proj_matrix * vs_out.P;"
	"}";

//////// FRAGMENT SHADER SOURCE ////////
static const char* fs_src =
	"#version 330 core\n"
	"out vec4 color;"
	"uniform vec4 vColor;"
	"uniform bool multNormals;"
	"uniform sampler2D s;"

	// -- In block from Vertex Shader --
	"in VS_OUT { vec3 N; vec4 P; vec2 TC; } vs_in;"

	// -- Modelview Matrix --
	"uniform mat4 mv_matrix;"

	// -- Light and Material properties --
	"uniform vec3 light_pos_1 = vec3(100.0, 100.0, 100.0);"		// Light 1 position
	"uniform vec3 diffuse_albedo_1 = vec3(0.12, 0.30, 0.62);"	// Light 1 diffuse albedo
	"uniform vec3 specular_albedo_1 = vec3(0.7);"				// Light 1 specular albedo

	"uniform vec3 light_pos_2 = vec3(-100.0, 100.0, 0.0);"		// Light 2 position
	"uniform vec3 diffuse_albedo_2 = vec3(0.85, 0.25, 0.05);"	// Light 2 diffuse albedo
	"uniform vec3 specular_albedo_2 = vec3(0.7,0.3,0.1);"		// Light 2 specular albedo

	"uniform vec3 rim_albedo = vec3(0.6);"						// Ambient rim albedo
	"uniform float specular_power = 128.0;"						// Material specular sharpness (power)
	"uniform float rim_power = 1.25;"							// Material rim sharpness (power)
	"uniform vec3 ambient = vec3(0.1, 0.1, 0.1);"				// Global ambient colour

	"void main(void) "
	"{"
	// Interpolated vertex coordinate (already in view-space) - swizzle for vec3
	"  vec3 P = vs_in.P.xyz;"

	// Interpolated normal (already in view-space)
	"  vec3 N = normalize(vs_in.N);"

	// Transform light vector to view-space (P is interpolated view-space coordinate)
	"  vec3 L1 = normalize(light_pos_1 - P);"
	"  vec3 L2 = normalize(light_pos_2 - P);"

	// Viewing vector (negative of the view-space position in this case)
	"  vec3 V = normalize(-P);"

	// Calculate the reflection vector by reflecting -L around the plane defined by N
	"  vec3 R1 = reflect(-L1, N);"
	"  vec3 R2 = reflect(-L2, N);"

	// -- Calculate diffuse and specular contributions --
	// Traditional Phong diffuse texture contribution is determined by surface normal * light vector.
	"  vec3 diffuse1 = max(0.0, dot(N, L1) ) * diffuse_albedo_1 * texture(s, vs_in.TC).rgb;"
	"  vec3 diffuse2 = max(0.0, dot(N, L2) ) * diffuse_albedo_2 * texture(s, vs_in.TC).rgb;"

	// Traditional Phong specular contribution ( no Blinn-Phong optimisation )
	"  vec3 specular1 = pow( max(0.0,dot(R1, V)), specular_power) * specular_albedo_1;"
	"  vec3 specular2 = pow( max(0.0,dot(R2, V)), specular_power) * specular_albedo_2;"

	// Artificial rim colour contributes when normal is at a graziong angle to the view
	"  vec3 rim = pow( smoothstep(0.0, 1.0, 1.0 - dot(N, V) ), rim_power) * rim_albedo;"

	// Final colour is the sum of contributions
	"  color = vec4( (ambient + diffuse1+diffuse2 + specular1+specular2) + rim, 1.0);"
	"}";
////////               ////////

int cWidth = 640, cHeight = 480;
int mouseX, mouseY;

string lpszModelName;

vec3f rot;				// View rotation
vec3f keyRot;			// Keyboard rotation offset

GLfloat
	mv_matrix[4*4],		// Modelview Matrix
	proj_matrix[4*4],	// Projection Matrix
	zoffset=0.0f;		// Z offset

GLuint
	rendering_program,	// GLSL program
	vao,				// Vertex Array Object
	pos_buffer,			// Vertex coordinate buffer
	texture[2],			// Array of texture names
	vertCount;			// Vertex count

static void check_args (int argc, char** argv);
static GLuint compile_shaders (void);
static void init_texture (char* path, int index);
static void init_menu (void);
static void calculate_mv_matrix (float cTime);
static void calculate_proj_matrix (float fov, float aspect, float n, float f);
static void print_shader_log(GLuint shader);

void setup			(void);
void reshape		(int, int);
void render			(void);
void keyFunc		(unsigned char,int,int);
void specialKeyFunc	(int, int,int);
void mouseFunc		(int, int);
void menuFunc		(int);
void shutdown		(void);

int main (int argc, char* argv[])
{
	fprintf(stdout, "\t== OBJ Model Loader & GLSL Demo | UTS CG, AUT 2014 ==\n\n");

	check_args(argc, argv);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
	glutInitWindowSize(cWidth, cHeight);
	glutInitWindowPosition(-1,-1);			// Let OS decide
//	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glutCreateWindow("Part III : OBJ Model Loader");

	glutReshapeFunc(reshape);
	glutDisplayFunc(render);
	glutKeyboardFunc( keyFunc );
	glutSpecialFunc(specialKeyFunc); 
	glutMotionFunc(mouseFunc);

	init_menu();

	// == Initialize GLEW (extension loader) ==
	GLenum glewResult = glewInit();
	if(glewResult != GLEW_OK)
	{
		fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(glewResult));
		getchar();
		return 1;
	}

	setup();
	glutMainLoop();
	shutdown();

	return 0;
}

static void check_args (int argc, char** argv)
{
	lpszModelName = "Models\\dbunny.obj";
	if(argc > 1)
		lpszModelName = string(argv[1]);
}

static void init_menu (void)
{
	// Create right-click menu:
	GLuint optmen = glutCreateMenu(menuFunc);
	glutAddMenuEntry(" === Multisampling === ", 0);
	glutAddMenuEntry("Enable", 1);
	glutAddMenuEntry("Disable", 2);
	glutAddMenuEntry(" === Texture === ", 0);
	glutAddMenuEntry("Disable", 3);
	glutAddMenuEntry("Tiles", 4);
	glutAddMenuEntry("Sand", 5);
	glutAddMenuEntry(" === Rendering === ", 0);
	glutAddMenuEntry("Wireframe", 6);
	glutAddMenuEntry("Fill", 7);

	// Program select menu
	GLuint psmen = init_program_select_menu();

	glutSetMenu(psmen);
	glutAddSubMenu("Display Options ...", optmen);
}

void setup (void)
{
	fprintf(stdout, "Running GLEW v%s\n", glewGetString(GLEW_VERSION));

	rendering_program = compile_shaders();

	// Create and bind VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	ObjModel obj;
	obj.load(lpszModelName.c_str());
	vertCount = obj.getSize();
	float* verts = new float[vertCount];
	obj.getVertices(verts);

	// Create a vertex array buffer for the VAO, to store vertex coordinates
	glGenBuffers(1, &pos_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pos_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertCount*sizeof(float), verts, GL_STATIC_DRAW);

	// Map array buffer to shader location 0
	GLuint vVertexLocation = glGetAttribLocation(rendering_program, "vVertex");
	glVertexAttribPointer(
		vVertexLocation,	// Location of the vVertex attribute (should be 0)
		3,					// Three component vector (X,Y,Z)
		GL_FLOAT,			// Float data
		GL_FALSE,			// Do not normalize
		sizeof(float)*8,	// Stride
		(void*)0);			// Offset = 0
	glEnableVertexAttribArray(vVertexLocation);

	// Set up the Texture Coordinate attribute
	GLuint tcLocation = glGetAttribLocation(rendering_program, "tc");
	glVertexAttribPointer(
		tcLocation,					// Location of tc attribute
		2,							// Two-component vector (U,V)
		GL_FLOAT,					// Float coordinates
		GL_FALSE,					// Do not normalize
		sizeof(float)*8,			// Stride
		(void*)(sizeof(float)*3));	// Skip over first 3 floats for tc
	glEnableVertexAttribArray(tcLocation);

	// Set up the normal attribute
	GLuint normLocation = glGetAttribLocation(rendering_program, "vNormal");
	glVertexAttribPointer(
		normLocation,				// Location of vNormal attribute
		3,							// Three-component vector (X,Y,Z)
		GL_FLOAT,					// Float coordinates
		GL_FALSE,					// Do not normalize (already is)
		sizeof(float)*8,			// Stride
		(void*)(sizeof(float)*5));	// Skip vertex coord and TC
	glEnableVertexAttribArray(normLocation);

	// Sets colour to use when clearning the draw buffer
	glClearColor(1., 1., 1., 1.0);

	// == Init. two textures ==
	init_texture("Textures\\tiles.bmp", 1);
	init_texture("Textures\\sand.bmp", 0);

	glEnable(GL_CULL_FACE);		// Backface Cull
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);	// Z-buffer
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_MULTISAMPLE);
}

static GLuint compile_shaders (void)
{
	fprintf(stdout, "Status: Compiling shaders...\n");
	GLuint vs, fs, program;

	// Compile vertex shader
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_src, NULL);
	glCompileShader(vs);
	print_shader_log(vs);

	// Compile fragment shader
	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_src, NULL);
	glCompileShader(fs);
	print_shader_log(fs);

	// Link shaders into executable GPU program
	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	// Return program name:
	return program;
}

void init_texture (char* path, int index)
{
	fprintf(stdout, "Status: Initializing texture <%s>\n", path);

	// Load BMP using the library
	BmpImage bmp;
	bmp.load(path);

	glGenTextures(1, &texture[index]);				// Generate a texture object
	glBindTexture(GL_TEXTURE_2D, texture[index]);	// Bind the (first) texture to the Texture 2D target

	// == Allocate memory for texture ==
	glTexStorage2D(GL_TEXTURE_2D,			// Binding point
				   1,						// Mipmap levels
				   GL_RGBA32F,				// Internal format
				   bmp.data.width,			// Dimensions
				   bmp.data.height);		// ( Try to keep as powers of 2 )

	// == Substitute texture image into target (copy memory) ==
	glTexSubImage2D(GL_TEXTURE_2D,			// Binding point
					0,						// Mipmap level
					0,0,					// Start at 0,0
					bmp.data.width,			// Copy all pixels
					bmp.data.height,
					GL_RGBA,				// Pixel format
					GL_FLOAT,				// Float data
					bmp.data.rgba);			// Pointer to pixels
}

void render (void)
{
	static float cTime = 0.0;
	cTime += 0.01f;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(rendering_program);

	GLfloat triCol[] = {0,0,0,1};
	GLuint colLocation = glGetUniformLocation(rendering_program, "vColor");
	glUniform4fv(colLocation, 1, triCol);

	GLuint mvLocation = glGetUniformLocation(rendering_program, "mv_matrix");
	GLuint projLocation = glGetUniformLocation(rendering_program, "proj_matrix");
	glUniformMatrix4fv(projLocation, 1, GL_TRUE, proj_matrix);

	calculate_mv_matrix(cTime);
	glUniformMatrix4fv(mvLocation, 1, GL_TRUE, mv_matrix);

	// == Draw model ==
	glDrawArrays(GL_TRIANGLES, 0, vertCount);

	glutSwapBuffers();
	glutPostRedisplay();
	Sleep(1);	// Frame limiter
}

void reshape (int w, int h)
{
	cWidth = w, cHeight = h;
	glViewport(0,0,w,h);
	calculate_proj_matrix(
		60.0f,				// FOV
		(float)w/(float)h,	// Aspect
		0.01f,				// N
		10.0f);				// F
}

// 4x4 Matrix Product
static void mult_mat4 (float ma[16], float mb[16], float res[16])
{
	for(int h=0; h<4; h++)
		for(int v=0; v<4; v++)
		{
			float sum=0;
			for(int k=0; k<4; k++)
				sum += ma[k*4 + v] * mb[h*4 + k];
			res[h*4 + v] = sum;
		}
}

static void calculate_mv_matrix (float cTime)
{
	static vec3f prot;
	float xrads = (float)mouseX/(float)cWidth*M_PI*4.f		+ keyRot.x/180.0*M_PI;
	float yrads = (float)mouseY/(float)cHeight*M_PI*2.f		+ keyRot.y/180.0*M_PI;;
	rot.x += (xrads - prot.x) * 0.05f;
	rot.y += (yrads - prot.y) * 0.05f;
	prot = rot;

	float sz = sin(rot.y), cz = cos(rot.y);
	GLfloat rotZ[] = 
	{
		cz,		sz,		0.0,	0.0,
		-sz,	cz,		0.0,	0.0,
		0.0,	0.0,	1.0,	0.0,
		0.0,	0.0,	0.0,	1.0
	};

	float sx = sin(rot.x), cx = cos(rot.x);
	GLfloat rotY[] =
	{
		cx,		.0,		sx,		.0,
		.0,		1.,		.0,		.0,
		-sx,	.0,		cx,		.0,
		.0,		.0,		.0,		1.
	};

	vec3f t = vec3f(0,0,-3 + zoffset);
	GLfloat trans[] =
	{
		1.,	0.,	0.,	t.x,
		0.,	1.,	0.,	t.y,
		0.,	0.,	1.,	t.z,
		0.,	0.,	0.,	1.
	};

	float rot[16];
	mult_mat4(rotZ, rotY, rot);

	float rottrans[16];
	mult_mat4(rot, trans, rottrans);

	std::copy(rottrans, rottrans+16, mv_matrix);
}

static void calculate_proj_matrix (float fov, float aspect, float n, float f)
{
	// === Perspective Projection Matrix Implementation === //

	float fovrads = fov/180.0f*M_PI;
	float cotfov = 1.f / tan( fovrads * 0.5f );
	float zfn = (f + n) / (f - n);
	float wfn = (2.f * f * n) / (f - n);

	GLfloat mat[] = 
	{
		cotfov/aspect,	0.0,		0.0,		0.0,
		0.0,			cotfov,		0.0,		0.0,
		0.0,			0.0,		-zfn,		-wfn,
		0.0,			0.0,		-1.0,		0.0
	};

	std::copy(mat, mat+16, proj_matrix);
}

void keyFunc (unsigned char key,int,int)
{
	switch( toupper(key) )
	{
	case 'X':	keyRot.x += 30;	break;
	case 'Y':	keyRot.y += 30;	break;

	// Reset
	case 'R': case 'I':
		keyRot = vec3f();
		rot = vec3f();
		mouseX = mouseY = 0;
		break;
	}
}

void specialKeyFunc (int k, int,int)
{
	switch(k){
	case GLUT_KEY_UP:	zoffset+=0.02f;	break;
	case GLUT_KEY_DOWN:	zoffset-=0.02f;	break;
	}
}

void mouseFunc (int x, int y)
{
	mouseX = x; mouseY = y;
}

void menuFunc (int val)
{
	switch(val)
	{
	case 1:	glEnable(GL_MULTISAMPLE);					break;
	case 2:	glDisable(GL_MULTISAMPLE);					break;
	case 3: glBindTexture(GL_TEXTURE_2D, 0);			break;
	case 4:	glBindTexture(GL_TEXTURE_2D, texture[1]);	break;
	case 5:	glBindTexture(GL_TEXTURE_2D, texture[0]);	break;
	case 6: glPolygonMode(GL_FRONT, GL_LINE);			break;
	case 7: glPolygonMode(GL_FRONT, GL_FILL);			break;
	}
}

void shutdown (void)
{
	glDeleteVertexArrays(1, &vao);
	glDeleteTextures(2, texture);
	glDeleteProgram(rendering_program);
	glDeleteBuffers(1, &pos_buffer);

	exit(0);
}


#include <string>
static void print_shader_log(GLuint shader)
{
    std::string str;
    GLint len;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 1)
    {
        str.resize(len);
        glGetShaderInfoLog(shader, len, NULL, &str[0]);
		fprintf(stdout, "\n  ==== SHADER LOG ==== \n%s  ====            ====\n\n", str.c_str());
    }
}