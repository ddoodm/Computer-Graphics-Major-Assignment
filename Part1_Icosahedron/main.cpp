/* ================================================	*\
 *	    UTS: Introduction to Computer Graphics		*
 *             Major Assignment | Part A			*
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

#define _USE_MATH_DEFINES

#include <Windows.h>
#include <math.h>
#include <glut.h>

#include <vec3.h>			// Custom geometric vector library
#include <bmpLoader.h>		// Custom bitmap texture parser
#include <programSelectMenu.h>

/*************************************\
  Camera
\*************************************/
class Camera
{
	GLfloat distance;
	vec3f rot;

public:
	vec3f keyRot;

	Camera(void) : distance(150.0) { }
	Camera(GLfloat distance) : distance(distance) { }

	void update (const GLuint mouseX, const GLuint mouseY)
	{
		rot = vec3f( mouseY, mouseX, 0.0 );
	}

	void transform (void)
	{
		glTranslatef(0,0, -distance);

		glRotatef(keyRot.x, 1.0, 0.0, 0.0);
		glRotatef(keyRot.y, 0.0, 1.0, 0.0);
		glRotatef(keyRot.z, 0.0, 0.0, 1.0);

		glRotatef(rot.x, -1.0, 0.0, 0.0);
		glRotatef(rot.y, 0.0, 1.0, 0.0);
	}
};

/*************************************\
  Simple directional light
\*************************************/
class DLight
{
public:
	void init (void)
	{
		static const GLfloat lightAmbient[]	= {0.4,0.4,0.4};
		static const GLfloat lightDiffuse[]	= {0.6,0.4,0.2};
		static const GLfloat lightSpecular[]= {0.7,0.7,0.7};

		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
		glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
	}

	void transform (void)
	{
		glPushMatrix();
		glTranslatef(200,200,180.0);
		glutSolidCube(10.0);
		glPopMatrix();

		// W = 0.0 = Directional light source
		static const GLfloat lightPos[]	= {200.0, 200.0, 180.0, 0.0};
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	}

} dLight;

/*************************************\
  Spot Light
\*************************************/
class SpLight
{
public:
	void init (void)
	{
		static const GLfloat lightAmbient[]	= {0.0,0.0,0.0,1.0};
		static const GLfloat lightDiffuse[]	= {0.2,0.3,0.5,1.0};
		static const GLfloat lightSpecular[]= {0.0,0.2,0.9,1.0};

		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT1);
		glLightfv(GL_LIGHT1, GL_AMBIENT, lightAmbient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiffuse);
		glLightfv(GL_LIGHT1, GL_SPECULAR, lightSpecular);
		glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

		// Spotlight Attenuation //
		glLightf(GL_LIGHT1,GL_SPOT_CUTOFF,130.0);
		glLightf(GL_LIGHT1,GL_SPOT_EXPONENT,32.0);
	}

	void transform (void)
	{
		glPushMatrix();
		glTranslatef(-160,40,-40);
		glutSolidCube(10.0);
		glPopMatrix();

		static const GLfloat lightPos[]	= {-160.0, 80.0, -40.0, 1.0};
		glLightfv(GL_LIGHT1, GL_POSITION, lightPos);

		// Spotlight Direction //
		static const GLfloat lightDirection[]= {0.0,0.0,20.0, 1.0};
		glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION, lightDirection);
	}

} spLight;

/*************************************\
  Icosahedron
\*************************************/
class Icosahedron
{
	static const float N_POINTS;
	vec3f v_coord[2][6];			// Vertex Coordinates : [side][geodesic top and border coordinates]
	GLfloat s;						// Radius
	GLuint textures[1];				// Texture names
	GLuint texW, texH;				// Texture dimensions
	static const GLuint				// Number of texture tiles H and V
		TILES_H, 
		TILES_V;		

	/*************************************\
	  Compute the surface normal of the
	  triangle defined by (p1,p2,p3).
	\*************************************/
	vec3f d_triNorm (const vec3f &p1, const vec3f &p2, const vec3f &p3) const
	{
		// Edge deltas
		vec3f a = p2 - p1;
		vec3f b = p3 - p1;

		// Orthogonal vector (unnormalized)
		vec3f n = a * b;

		// Normalize:
		n = n.normalize();

		return n;
	}

	vec3f get_uv( const int ti, const float hsz, const float vsz ) const
	{
		float u = (ti % TILES_H) / (float)TILES_H;	// Texture is TILE_H tiles wide
		float v = (ti / TILES_H) / (float)TILES_V;
		return vec3f(u, 1.0-vsz-v, 0.0);			// Trap: V is negated (-1 is up)
	}

	/*************************************\
	  Constructs a triangle with texture
	  coordinates. 'ti' is "Texture Index"
	\*************************************/
	void d_triangle (const vec3f a, const vec3f b, const vec3f c, const int ti) const
	{
		// Face Normal:
		//WARNING: Triangle is reversed to CCW winding order.
		vec3f n = d_triNorm(c,b,a);
		glNormal3fv( &n.x );

		// Get texture coordinates from texture index:
		float hsz = 1.0/(float)TILES_H;	// Tile size
		float vsz = 1.0/(float)TILES_V;
		vec3f uv = get_uv(ti,hsz,vsz);

		glTexCoord2f( uv.x,			uv.y);			glVertex3fv( &a.x );
		glTexCoord2f( uv.x+hsz/2.0,	uv.y+vsz);		glVertex3fv( &b.x );
		glTexCoord2f( uv.x+hsz,		uv.y);			glVertex3fv( &c.x );
	}

	/*************************************\
	  Constructs a quadrangle from two
	  triangles for triangulated line
	  drawing.
	\*************************************/
	void d_triQuad ( const vec3f a, const vec3f b, const vec3f c, const vec3f d, const int ti ) const
	{
		d_triangle(a,b,c,ti);
		d_triangle(c,d,a,ti+1);
	}

	void init_texture (char* path, int index)
	{
		fprintf(stdout, "Status: Parsing Texture <%s>...\n", path);
		glEnable( GL_TEXTURE_2D );

		BmpImage bmp;
		bmp.load(path);
		texW = bmp.bmpInfo.iWidth, texH = bmp.bmpInfo.iHeight;

		glGenTextures(1, &textures[index]);				// Generate a texture ID (name)
		glBindTexture(GL_TEXTURE_2D, textures[index]);	// Bind TEXTURE_2D to the new texture name

		// Texture mapping parameters MUST be set in legacy GL
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// == Declare TEXTURE_2D Data Parameters ==
		glTexImage2D(
			GL_TEXTURE_2D,		// Binding point
			0,					// Mipmap level
			GL_RGBA,			// Internal format
			bmp.data.width,		// 
			bmp.data.height,	//
			0,					// Border
			GL_RGBA,			// Format
			GL_FLOAT,			// Type
			bmp.data.rgba);		// Pointer to texels
	}

	void set_material (void) const
	{
		static const GLfloat matSpecular []= {0.99,0.92,0.94,1.0};
		static const GLfloat specPower = 32.0;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &specPower);
	}

public:
	Icosahedron (void)		: s(60.0)	{ }
	Icosahedron (float r)	: s(s)		{ }
	~Icosahedron (void)
	{
		glDeleteTextures(1, textures);
	}

	void build (void)
	{
		// Load and bind textures
		init_texture("Textures\\TileSheet.bmp", 0);

		// Radius for pentagon edges:
		static const float r = s * sin(2.0*M_PI/N_POINTS);

		// Phi is constant, and is the geodesic midpoint between Y +s and Y 0.
		static const float phi = 2.0*asin(1.0/(2.0*sin(2.0*M_PI/N_POINTS)));

		// Loop around icosahedron radius for both sides
		for(int side=0; side<2; side++)
		{
			// First vertex is +/-s on Z axis
			v_coord[side][0] = vec3f(0.0, 0.0, s*(side*2.0-1.0));
			for(int vert=1; vert<N_POINTS+1; vert++)
			{
				float theta = (float)vert / N_POINTS * 2.0*M_PI;
				float ofst = (float)side*M_PI;
				v_coord[side][vert].x = r * sin(phi) * cos(ofst+theta);
				v_coord[side][vert].y = r * sin(phi) * sin(ofst+theta);
				v_coord[side][vert].z = r * cos(phi) * (side*2.0-1.0);
			}
		}
	}

	void draw (void) const
	{
		// Upper and Lower pentagons
		/*
		for(int side=0; side<2; side++)
		{
			glBegin(GL_TRIANGLE_FAN);
			glVertex3fv( &v_coord[side][0].x );	// Open (top)
			for(int i=1; i<N_POINTS+1; i++)
				glVertex3fv( &v_coord[side][side? ((int)N_POINTS+1)-i : i].x );
			glVertex3fv( &v_coord[side][side? (int)N_POINTS : 1].x );	// Close
			glEnd();
		}
		*/

		set_material();

		static const int NP = (int)N_POINTS;

		for(int side=0; side<2; side++)
		{
			glBegin(GL_TRIANGLES);
			int i;
			for(i=1; i<N_POINTS; i++)
				d_triangle(	v_coord[side][0],							// A
							v_coord[side][side? (NP+1)-i : i],			// B
							v_coord[side][side? (NP+1)-(i+1) : i+1],	// C
							side*NP + i-1);								// Texture Index
			d_triangle(	v_coord[side][0],
						v_coord[side][side? 1 : NP],
						v_coord[side][side? NP : 1],
						side*NP + i-1);
			glEnd();
		}

		// Join upper and lower pentagons:
		glBegin(GL_TRIANGLES);
		for(int i=0;i<N_POINTS+1;i++)
		{
			d_triQuad(	v_coord[0][ (i+0+0)%NP+1 ],	// A
						v_coord[1][ (i+0+2)%NP+1 ],	// B
						v_coord[1][ (i+1+2)%NP+1 ],	// C
						v_coord[0][ (i+1+0)%NP+1 ],	// D
						N_POINTS*2 + (i-1)*2);		// Texture Index
		}
		glEnd();
	}
};
const float Icosahedron::N_POINTS = 5;
const GLuint Icosahedron::TILES_H = 5;
const GLuint Icosahedron::TILES_V = 4;

/*************************************\
  Globals
\*************************************/
GLuint
	cWidth = 640,
	cHeight = 480,
	mouseX, mouseY;

Camera		camera;				// World camera
Icosahedron icos;				// World icosahedron

bool	bWireframe=1;			// Draw wireframe
bool	bDrawGrid=1;			// Draw axis and grid
bool	bTextured=1;			// Enable TEXTURE_2D
GLuint	uiPolyMode = GL_FILL;	// Global polygon mode


/*************************************\
  Function Prototypes
\*************************************/
static void setup		(void);
static void draw		(void);
static void reshape		(int,int);
static void mouseFunc	(int,int);
static void keyFunc		(unsigned char key,int,int);
static void idle		(void);

static void init_menu	(void);
static void draw_axis	(void);
static void menu_func	(int);


/*************************************\
  Entry Point
\*************************************/
int main (int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(cWidth,cHeight);
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
	glutCreateWindow("Major Assignment | Icosahedron");

	init_menu();

	glutDisplayFunc( draw );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyFunc );
	glutMotionFunc( mouseFunc );
	glutIdleFunc( idle );

	setup();
	glutMainLoop();

	return 0;
}

static void init_menu (void)
{
	// Create right-click menu:
	GLuint optmen = glutCreateMenu(menu_func);
	glutAddMenuEntry("=== Wireframe ===", 0);
	glutAddMenuEntry("Enable", 1);
	glutAddMenuEntry("Disable", 2);
	glutAddMenuEntry("=== Texture ===", 0);
	glutAddMenuEntry("Enable", 3);
	glutAddMenuEntry("Disable", 4);
	glutAddMenuEntry("=== Polygon Mode ===", 0);
	glutAddMenuEntry("Fill", 5);
	glutAddMenuEntry("Wireframe", 6);

	// Program select menu
	GLuint psmen = init_program_select_menu();

	glutSetMenu(psmen);
	glutAddSubMenu("Display Options ...", optmen);
}

static void setup (void)
{
	glClearColor( 0.1,0.1,0.1,1.0 );

	icos.build();
	dLight.init();
	spLight.init();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
}

static void draw		(void)
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	camera.transform();
	dLight.transform();
	spLight.transform();

	if(bDrawGrid)
		draw_axis();

	if(bTextured)	glEnable( GL_TEXTURE_2D );
	else			glDisable( GL_TEXTURE_2D );

	if(uiPolyMode == GL_FILL)
	{
		glPolygonMode(GL_FRONT, GL_FILL);
		glColor3f( 0.9,0.9,0.9 );
		icos.draw();
	}

	if(bWireframe)
	{
		glPolygonMode(GL_FRONT, GL_LINE);
		glScalef(1.01,1.01,1.01);		// Avoid Z-fighting
		glLineWidth(4.0);
		if(uiPolyMode == GL_LINE)	glColor3f(0.9,0.9,0.9);
		else						glColor3f(0.0,0.0,0.0);
		icos.draw();
	}

	glutSwapBuffers();
}

static void reshape		(int w, int h)
{
	cWidth = w, cHeight = h;
	glViewport(0,0,cWidth,cHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective( 90.0, (float)cWidth/(float)cHeight, 0.1f, 1500.0f );
}

static void mouseFunc	(int x, int y)
{
	mouseX = x, mouseY = y;
}

static void keyFunc		(unsigned char key,int,int)
{
	switch( toupper(key) )
	{
	case 'X':	camera.keyRot.x += 30.0;	break;
	case 'Y':	camera.keyRot.y += 30.0;	break;
	case 'Z':	camera.keyRot.z += 30.0;	break;

	// Reset
	case 'R': case 'I':
		camera.keyRot = vec3f();
		mouseX = 0, mouseY = 0;
		break;

	// Draw grid
	case 'G': bDrawGrid =! bDrawGrid;	break;

	// Wireframe
	case 'W': bWireframe =! bWireframe;	break;
	}
}

static void idle		(void)
{
	camera.update(mouseX, mouseY);

	glutPostRedisplay();
}

static void draw_grid(void)
{
	glColor3f(0.4,0.4,0.4);
	glPushMatrix();
	for(int r=0;r<2;r++)
	{
		glRotatef(90,0,1,0);
		glBegin(GL_LINES);
		for(int x=-1000; x<=1000; x+=50)
		{
			int y = 0, z=1000;
			glVertex3f(x,y,z);
			glVertex3f(x,-y,-z);
		}
		glEnd();
	}
	glPopMatrix();
}

static void draw_axis	(void)
{
	static const float SZ = 500;
	glLineWidth(1.0);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	draw_grid();

	glBegin(GL_LINES);
	{
		glColor3f(1.0,0.0,0.0);
		glVertex3f(-SZ,0,0);
		glVertex3f(SZ,0,0);
		glColor3f(0.0,1.0,0.0);
		glVertex3f(0,-SZ,0);
		glVertex3f(0,SZ,0);
		glColor3f(0.0,0.0,1.0);
		glVertex3f(0,0,-SZ);
		glVertex3f(0,0,SZ);
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
}

static void menu_func (int k)
{
	switch(k)
	{
		// === Wireframe === //
	case 1:	bWireframe = 1;	break;
	case 2:	bWireframe = 0;	break;

		// === Texture === //
	case 3: bTextured = 1; break;
	case 4: bTextured = 0; break;

		// === Polygon Mode === //
	case 5: uiPolyMode = GL_FILL; break;
	case 6: uiPolyMode = GL_LINE; break;
	}
}