/* ================================================	*\
 *	    UTS: Introduction to Computer Graphics		*
 *             Major Assignment | Part B			*
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

#include <iostream>
#include <math.h>
#include <glut.h>

#include <vec3.h>		// Custom geometric vector library
#include <bmpLoader.h>	// Custom BMP parser
#include <programSelectMenu.h>

using namespace std;

enum TextureList
{
	CYLINDER_WALL,
	CYLINDER_CAP,
	BUILD_RIGHT,
	BUILD_FRONT,
	CONCRETE,
	SKYBOX
};

// Global texture name table
GLuint textures [6];

/*************************************\
	Load BMP texture from file
\*************************************/
void init_texture (char* path, int index, GLuint* textures)
{
	fprintf(stdout, "Status: Parsing Texture <%s>...\n", path);
	glEnable( GL_TEXTURE_2D );

	BmpImage bmp;
	bmp.load(path);

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

/*************************************\
  Computes the normal vector of the
  triangle defined by (p1, p2, p3).
\*************************************/
vec3f d_triNorm (const vec3f &p1, const vec3f &p2, const vec3f &p3)
{
	// Edge deltas
	vec3f a = p2 - p1;
	vec3f b = p3 - p1;

	// Normal vector (unnormalized)
	vec3f n = a * b;

	// Normalize:
	n = n.normalize();

	return n;
}

/*************************************\
  Cylinder with a sloped top facet
\*************************************/
class Cylinder
{
	vec3f**	verts;		// 2D array of vertex coordinates
	vec3f** tube_tc;	// 2D array of texture coordinates for the tube
	vec3f*  cap_tc;		// 1D array of texture coordinates for the caps
	vec3f*  norms;		// 1D array of vertex normals
	vec3f	origin;		// Position
	GLuint	slices;		// Resolution
	GLfloat
			radius,		// Radius of the cylinder
			height,		// Distance from the origin to the midpoint of the sloped facet
			slope;		// Amplitude of the slope
	GLuint	texA, texB;

	void draw_tube (void)
	{
		// ==== Join upper with lower ellipsoid ==== //
		glBegin(GL_QUADS);
		for(int i=0;i<slices;i++)
		{
			int ni = (i + 1) % slices;
			int nni = (i + 1) % (slices + 1);

			glNormal3fv( &norms[i].x );
			glTexCoord2fv( &tube_tc[0][i].x );		glVertex3fv( &verts[0][i].x );
			glTexCoord2fv( &tube_tc[1][i].x );		glVertex3fv( &verts[1][i].x );

			glNormal3fv( &norms[ni].x );
			glTexCoord2fv( &tube_tc[1][nni].x );	glVertex3fv( &verts[1][ni].x );
			glTexCoord2fv( &tube_tc[0][nni].x );	glVertex3fv( &verts[0][ni].x );
		}
		glEnd();
	}

	void draw_caps (void)
	{
		// ==== Cap Holes ==== //
		glBegin(GL_TRIANGLE_FAN);
		glNormal3fv( &norms[0].x );
		for(int i=0;i<slices;i++)
		{
			glTexCoord2fv( &cap_tc[i].x );
			glVertex3fv( &verts[0][i].x );
		}
		glEnd();

		glBegin(GL_TRIANGLE_FAN);
		glNormal3fv( &norms[0].x );
		for(int i=slices-1;i>=0;i--)
		{
			glTexCoord2fv( &cap_tc[i].x );
			glVertex3fv( &verts[1][i].x );
		}
		glEnd();
	}

public:
	Cylinder (void)
		: origin(vec3f(0,0,0)),
		  slices(32),
		  radius(50),
		  height(100),
		  slope(50)
	{  }

	Cylinder (vec3f origin, int slices, float radius, float height, float slope)
		: origin(origin), slices(slices), radius(radius), height(height), slope(slope)
	{  }

	~Cylinder (void)
	{
		// Free 2D arrays
		for(int i=0;i<2;i++)
		{
			delete [] verts[i];
			delete [] tube_tc[i];
		}

		delete [] verts;
		delete [] tube_tc;
		delete [] cap_tc;
	}

	void initialize (GLuint texA, GLuint texB)
	{
		this->texA = texA, this->texB = texB;

		// Initialize 2D arrays (2 sides)
		verts	= new vec3f* [2];
		tube_tc	= new vec3f* [2];
		for(int i=0;i<2;i++)
		{
			verts[i]	= new vec3f[slices];
			tube_tc[i]	= new vec3f[slices+1];
		}

		cap_tc	= new vec3f [slices+1];

		// ==== Cylinder Base (constant Y) ==== //
		for(int i=0;i<slices;i++)
		{
			float ui = (float)i/(float)slices;
			float theta = ui * 2.0f * M_PI;

			// Position
			float x = origin.x + radius * cos(theta);
			float y = origin.y;
			float z = origin.z + radius * sin(theta);
			verts[0][i] = vec3f(x,y,z);

			// Tube texture
			float tu = ui;
			float tv = 0.0;
			tube_tc[0][i] = vec3f(tu,tv,0);

			// Cap texture
			float cu = 0.5 + 0.5*cos(theta);
			float cv = 0.5 + 0.5*sin(theta);
			cap_tc[i] = vec3f(cu, cv, 0);
		}
		tube_tc[0][slices] = vec3f(1.0,0,0);

		// ==== Cylinder Slope (varying Y) ==== //
		for(int i=0;i<slices;i++)
		{
			float ui = (float)i/(float)slices;
			float theta = ui * 2.0 * M_PI;

			float x = verts[0][i].x;
			float y = verts[0][i].y + height + slope * cos(theta);
			float z = verts[0][i].z;
			verts[1][i] = vec3f(x,y,z);

			float tu = ui;
			float tv = 1.0 - (height + slope + origin.y - y) / (height + slope + origin.y);
			tube_tc[1][i] = vec3f(tu,tv,0);
		}
		tube_tc[1][slices] = vec3f(1.0,1.0,0);

		// ==== Vertex Normals ==== //
		vec3f *flatNorms = new vec3f [slices];
		for(int i=0;i<slices;i++)
		{
			int ni = (i+1)%slices;
			vec3f N = d_triNorm(
			//	verts[0][i], verts[0][ni], verts[1][ni] );
				verts[1][ni], verts[0][ni], verts[0][i] ); // Reversed winding order
			flatNorms[i] = N;
		}

		// ==== Average Normals ==== //
		norms = new vec3f [slices];
		for(int i=0;i<slices;i++)
		{
			int ni = (i+1)%slices;
			int nni = (i+2)%slices;

			vec3f N = ( flatNorms[i] + flatNorms[ni] + flatNorms[nni] ) / 3.0;

			norms[ni] = N;
		}

		delete [] flatNorms;
	}
	
	void draw (void)
	{
		static const GLfloat matSpecular []= {0.5,0.5,0.5,1.0};
		static const GLfloat specPower = 24.0;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &specPower);

		glBindTexture(GL_TEXTURE_2D, textures[texA]);
		draw_tube();

		glBindTexture(GL_TEXTURE_2D, textures[texB]);
		draw_caps();

		static const GLfloat zero[] = {0,0,0,1};
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, zero);
	}
};

/*************************************\
  Base-pivot Cuboid
\*************************************/
class Cuboid
{
	vec3f	verts[2][4];	// Vertex coordinates
	vec3f	cap_tc[4];		// Texture coordinates for caps
	vec3f	ring_tc[2][5];	// Texture coordinates for ring
	vec3f	size;			// Width, height and depth
	GLuint	texA, texB;

	void draw_caps (void)
	{
		vec3f N;	// Surface normal

		glBegin(GL_QUADS);
		N = d_triNorm( verts[0][0], verts[0][1], verts[0][2] );
		glNormal3fv( &N.x );
		for(int slice=3;slice>=0;slice--)
		{
			glTexCoord2fv( &cap_tc[slice].x );
			glVertex3fv( &verts[0][slice].x );
		}
		
		N = d_triNorm( verts[1][0], verts[1][1], verts[1][2] );
		glNormal3fv( &N.x );
		for(int slice=0;slice<4;slice++)
		{
			glTexCoord2fv( &cap_tc[slice].x );
			glVertex3fv( &verts[1][slice].x );
		}
		glEnd();
	}

	void draw_ring (void)
	{
		vec3f N;	// Surface normal

		glBegin(GL_QUADS);
		for(int slice=0;slice<4;slice++)
		{
			int ns = (slice+1)%4;

			N = d_triNorm( verts[0][slice], verts[0][ns], verts[1][ns] );
			glNormal3fv( &N.x );

			glTexCoord2fv( &ring_tc[0][slice].x );	glVertex3fv( &verts[0][slice].x );
			glTexCoord2fv( &ring_tc[0][ns].x );		glVertex3fv( &verts[0][ns].x );
			glTexCoord2fv( &ring_tc[1][ns].x );		glVertex3fv( &verts[1][ns].x );
			glTexCoord2fv( &ring_tc[1][slice].x );	glVertex3fv( &verts[1][slice].x );
		}
		glEnd();
	}

public:
	Cuboid (vec3f size)
		: size(size)
	{ }

	void initialize (GLuint texA, GLuint texB)
	{
		this->texA = texA, this->texB = texB;

		static const float r = sqrt( 3.0 / 2.0 );
		static const float phi = asin( sqrt( 2.0 / 3.0 ) );

		for(int slice=0; slice<4; slice++)
		{
			float ui = (float)slice / 4.0f;
			float theta = ui * 2.0 * M_PI + 0.25*M_PI;

			float x = size.x * sin(phi) * cos(theta);
			float y = size.y * cos(phi);
			float z = size.z * sin(phi) * sin(theta);
			verts[0][slice] = vec3f(x,y,z);

			// Other side is similar, with negated Y:
			verts[1][slice] = vec3f(x, -y, z);

			// Cap texture coordinates
			cap_tc[slice] = vec3f( 0.5+0.5*cos(theta), 0.5+0.5*sin(theta), 0.0 );

			// Ring texture coordinates
			ring_tc[0][slice] = vec3f( ui, 1.0, 0.0 );
			ring_tc[1][slice] = vec3f( ui, 0.0, 0.0 );
		}
	}

	void draw (void)
	{
		glBindTexture(GL_TEXTURE_2D, textures[texA]);
		draw_ring();

		glBindTexture(GL_TEXTURE_2D, textures[texB]);
		draw_caps();
	}
};

/*************************************\
  Water
\*************************************/
class Water
{
	const GLuint HVRES;	// H and V resolution
	const vec3f SIZE;	// Drawable size
	GLfloat time;		// Time
	vec3f **verts;		// 2D array of vertices
	vec3f **norms;		// 2D array of precomputed normals

	/*************************************\
	  Constructs a triangle with texture
	  coordinates.
	\*************************************/
	void d_triangle (const vec3f &a, const vec3f &b, const vec3f &c) const
	{
		glTexCoord2d(0,0);		glVertex3fv( &a.x );
		glTexCoord2d(0.5,1);	glVertex3fv( &b.x );
		glTexCoord2d(1,0);		glVertex3fv( &c.x );
	}

	/*************************************\
	  Constructs a quadrangle from two
	  triangles for triangulated line
	  drawing.
	\*************************************/
	void d_triQuad ( const vec3f &a, const vec3f &b, const vec3f &c, const vec3f &d ) const
	{
		d_triangle(a,b,c);
		d_triangle(c,d,a);
	}

public:
	Water(GLuint hvres, vec3f &size) : HVRES(hvres), SIZE(size), time(0.0) { }

	void build (void)
	{
		// Init. 2D array
		// Z is up
		verts = new vec3f* [HVRES];
		for(int y=0;y<HVRES;y++)
		{
			verts[y] = new vec3f[HVRES];
			for(int x=0;x<HVRES;x++)
			{
				float vx = (float)x / (float)HVRES * SIZE.x - SIZE.x/2.0;
				float vy = (float)y / (float)HVRES * SIZE.y - SIZE.y/2.0;

				float waveFunc = sin( vec3f(0.75,0.25,0.0).dot(vec3f(vx,vy,0.0)) * 0.08 + time )
					+ 0.4*sin( vec3f(0.90,0.10,0.0).dot(vec3f(vx,vy,0.0)) * 0.24 + time*0.74 );
				waveFunc /= 2.0; // Sum of sines results in -2.0 <= W <= 2.0, so it is mapped to -1...1

				float vz = SIZE.z * waveFunc;

				verts[y][x] = vec3f(vx,vz,vy);
			}
		}

		// === Compute Facit Normals === //
		//vec3f** flatNorms = new vec3f* [HVRES];
		norms = new vec3f* [HVRES];
		for(int y=0;y<HVRES;y++)
		{
			int ny = (y+1)%HVRES;
			//flatNorms[y] = new vec3f[HVRES];
			norms[y] = new vec3f[HVRES];
			for(int x=0;x<HVRES;x++)
			{
				int nx = (x+1)%HVRES;
				vec3f
					a = verts[y][x],
					b = verts[y][nx],
					c = verts[ny][nx];
				norms[y][x] = d_triNorm(a, b, c );
			}
		}

		// === Averagen Facit Normals === //
		/*
		for(int y=0;y<HVRES-2;y++)
			for(int x=0;x<HVRES-2;x++)
			{
				vec3f N = ( 
					flatNorms[x][y+0] + flatNorms[x+1][y+0] + flatNorms[x+2][y+0] +
					flatNorms[x][y+1] + flatNorms[x+1][y+1] + flatNorms[x+2][y+1] +
					flatNorms[x][y+2] + flatNorms[x+1][y+2] + flatNorms[x+2][y+2] ) / (3.0*3.0);

				norms[x+1][y+1] = N;
			}
			*/
	}

	void update (GLfloat cTime)
	{
		time = cTime;
		build();
	}

	void draw (void) const
	{
		// No texture
		glDisable(GL_TEXTURE_2D);

		// Configure fragment blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f( 0.8,0.9,1.0,0.8 );

		// Shiny water material
		GLfloat matSpecular[] = {0.65, 0.67, 0.69}; 
		GLfloat matSpecPower = 64.0;
		glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular );
		glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, &matSpecPower );

		glBegin(GL_TRIANGLES);
		for(int y=0;y<HVRES-1;y++)
			for(int x=0;x<HVRES-1;x++)
			{
				// Plane Normal:
				vec3f n = d_triNorm(verts[y][x],verts[y+1][x],verts[y+1][x+1]);
				glNormal3fv( &n.x );

				d_triQuad(	verts[y][x],
							verts[y+1][x],
							verts[y+1][x+1],
							verts[y][x+1]);
			}
		glEnd();

		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

		// Reset material
		GLfloat zero[] = {0,0,0,0}; 
		glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, zero );
		glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, zero );
	}
};

/*************************************\
  Skybox
\*************************************/
class Skybox
{
	GLuint texture;

	void draw_caps (void)
	{
		glBegin(GL_QUADS);
		for(int side=0;side<2;side++)
			for(int slice=0;slice<4;slice++)
			{
				glTexCoord2fv( &cap_tc[side][slice].x );
				glVertex3fv( &verts[side][slice].x );
			}
		glEnd();
	}

	void draw_ring (void)
	{
		glBegin(GL_QUADS);
		for(int slice=3;slice>=0;slice--)
		{
			int ns = (slice+1)%4;
			int tcns = (slice+1)%5;
			glTexCoord2fv( &ring_tc[0][slice].x );	glVertex3fv( &verts[0][slice].x );
			glTexCoord2fv( &ring_tc[0][tcns].x );	glVertex3fv( &verts[0][ns].x );
			glTexCoord2fv( &ring_tc[1][tcns].x );	glVertex3fv( &verts[1][ns].x );
			glTexCoord2fv( &ring_tc[1][slice].x );	glVertex3fv( &verts[1][slice].x );
		}
		glEnd();
	}

public:
	Skybox (void) { }

	vec3f	verts[2][4];	// Vertex coordinates
	vec3f	cap_tc[2][4];	// Texture coordinates for caps
	vec3f	ring_tc[2][5];	// Texture coordinates for ring

	void initialize (GLuint texID)
	{
		texture = texID;

		static const float s = 1000.0;
		static const float r = s * sqrt( 3.0 / 2.0 );
		static const float phi = asin( sqrt( 2.0 / 3.0 ) );

		for(int slice=0; slice<4; slice++)
		{
			float ui = (float)slice / 4.0f;
			float theta = ui * 2.0 * M_PI + 0.25*M_PI;

			float x = r * sin(phi) * cos(theta);
			float y = r * cos(phi);
			float z = r * sin(phi) * sin(theta);
			verts[0][slice] = vec3f(x,y,z);

			// Other side is similar, with negated Y:
			verts[1][slice] = vec3f(x, -y, z);

			// Cap texture coordinates
			theta += 0.5*M_PI;
			cap_tc[0][slice] = vec3f( 3./8. + 1.4/8. * cos(theta), 5./6. + 1.4/6. * sin(theta), 0.0 );
			cap_tc[1][slice] = vec3f( 3./8. + 1.4/8. * cos(theta), 1./6. + 1.4/6. * sin(theta), 0.0 );

			// Ring texture coordinates
			ring_tc[0][slice] = vec3f( ui, 2./3., 0.0 );
			ring_tc[1][slice] = vec3f( ui, 1./3., 0.0 );
		}

		// Ring texture coordinates
		ring_tc[0][4] = vec3f( 1.0, 2./3., 0.0 );
		ring_tc[1][4] = vec3f( 1.0, 1./3., 0.0 );
	}

	void draw (void)
	{
		glDisable(GL_CULL_FACE);
		glBindTexture(GL_TEXTURE_2D, textures[texture]);
		glDisable(GL_LIGHTING);

		// Full, unbiased radiosity
		glColor4f(1,1,1,1);

		draw_ring();
		draw_caps();

		glEnable(GL_LIGHTING);
		glEnable(GL_CULL_FACE);
	}
};

/*************************************\
  Camera
\*************************************/
class Camera
{
	GLfloat distance;
	vec3f origin;
	vec3f viewRot;
	vec3f orbit;

public:
	vec3f keyRot;

	Camera(void) 
		: distance(200.0), origin(vec3f(0, -10.0, 0)), keyRot(vec3f()), viewRot(vec3f()), orbit(vec3f())
	{ }
	Camera(GLfloat distance) 
		: distance(distance), origin(vec3f(0, -10.0, 0)), keyRot(vec3f()), viewRot(vec3f()), orbit(vec3f())
	{ }

	void update (const GLuint mouseX, const GLuint mouseY)
	{
		viewRot = vec3f( mouseY, mouseX, 0.0 );
	}

	void transform (Skybox sky)
	{
		// Ease keyRot
		static vec3f porbit;
		orbit.x += (keyRot.x - porbit.x) * 0.05f;
		orbit.y += (keyRot.y - porbit.y) * 0.05f;
		porbit = orbit;

		// Skybox is not affected by camera translation
		glPushMatrix();
		glRotatef(viewRot.x, -1.0, 0.0, 0.0);
		glRotatef(viewRot.y, 0.0, 1.0, 0.0);
		glRotatef(orbit.x, 1.0, 0.0, 0.0);
		glRotatef(orbit.y, 0.0, 1.0, 0.0);
		glRotatef(orbit.z, 0.0, 0.0, 1.0);
		sky.draw();
		glPopMatrix();

		// Rotate entire coordinate system (view rotation)
		glRotatef(viewRot.x, -1.0, 0.0, 0.0);
		glRotatef(viewRot.y, 0.0, 1.0, 0.0);

		// Move viewer into distance, starting from origin
		glTranslatef(0, 0, -distance);
		glTranslatef(origin.x, origin.y, origin.z);

		// Orbit about keyboard rotation angle
		glRotatef(orbit.x, 1.0, 0.0, 0.0);
		glRotatef(orbit.y, 0.0, 1.0, 0.0);
		glRotatef(orbit.z, 0.0, 0.0, 1.0);
		glRotatef(90, 0.0, 1.0, 0.0);
	}
};

/*************************************\
  Simple Directional Light (sun)
\*************************************/
class Sun
{
public:
	void init (void)
	{
		static const GLfloat lightAmbient[]	= {0.4,0.4,0.4,1.0};
		static const GLfloat lightDiffuse[]	= {0.6,0.4,0.2,1.0};
		static const GLfloat lightSpecular[]= {1.0,1.0,1.0,1.0};

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
		static const GLfloat lightPos[]	= {-40.0, 40.0, 40.0, 1.0};
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	}
} sun;

/*************************************\
  Spot Light
\*************************************/
class SpLight
{
public:
	void init (void)
	{
		static const GLfloat lightAmbient[]	= {0.0,0.0,0.0,1.0};
		static const GLfloat lightDiffuse[]	= {0.5,0.0,0.0,1.0};
		static const GLfloat lightSpecular[]= {1.0,0.2,0.1,1.0};

		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT1);
		glLightfv(GL_LIGHT1, GL_AMBIENT, lightAmbient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiffuse);
		glLightfv(GL_LIGHT1, GL_SPECULAR, lightSpecular);
		glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

		// Spotlight Attenuation //
		glLightf(GL_LIGHT1,GL_SPOT_CUTOFF,120.0f);	// Angle
		glLightf(GL_LIGHT1,GL_SPOT_EXPONENT,80.0f);	// Power
	}

	void transform (void)
	{
		// Light visualizer
		/*glPushMatrix();
		glTranslatef(-60,0,-60);
		glColor3f(1.0,0.6,0.4);
		glutSolidSphere(10.0,8,8);
		glColor3f(1.0,1.0,1.0);
		glPopMatrix();*/

		// Light points up from ground
		// W = 1 = positional
		static const GLfloat lightPos[]	= {-60, 0, -60, 1.0};
		glLightfv(GL_LIGHT1, GL_POSITION, lightPos);

		// Spotlight Direction //
		static const GLfloat lightDirection[]= {0.0,100.0,0, 0.0};
		glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION, lightDirection);
	}

} spLight;


/*************************************\
  Globals
\*************************************/
GLuint
	cWidth = 640,
	cHeight = 480,
	mouseX, mouseY;

bool
	bDrawGrid = 0,
	bWireframe = 0,
	bRendered = 1;

// World camera (250 units from origin)
Camera		camera		(250.0);

// Cylindrical building
Cylinder	cylinder	( vec3f(0,10,0), 32, 50, 100, 50 );
Cylinder	step1		( vec3f(0,5,0), 32, 65, 5, 0 );
Cylinder	step2		( vec3f(), 32, 75, 5, 0 );

// Cuboid buildings
Cuboid		build_right	( vec3f(100, 50, 150) );
Cuboid		build_front	( vec3f(60, 30, 100) );
Cuboid		foundation	( vec3f(160, 25, 500) );

// Water (Z-up)
Water		water		( 64, vec3f(150,500,5) );

// Skybox
Skybox		sky;

/*************************************\
  Function Prototypes
\*************************************/
static void setup		(void);
static void draw		(void);
static void reshape		(int,int);
static void mouseFunc	(int,int);
static void menu_func	(int);
static void keyFunc		(unsigned char,int,int);
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
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(cWidth,cHeight);
	glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH );
	glutCreateWindow("Major Assignment | Tycho Brahe");

	glutDisplayFunc( draw );
	glutReshapeFunc( reshape );
	glutMotionFunc( mouseFunc );
	glutKeyboardFunc( keyFunc );
	glutIdleFunc( idle );

	init_menu();

	setup();
	glutMainLoop();

	return 0;
}

static void init_menu(void)
{
	// Program select menu
	GLuint psmen = init_program_select_menu();

	// Create right-click menu:
	GLuint optmen = glutCreateMenu(menu_func);
	glutAddMenuEntry("Display Wireframe", 1);
	glutAddMenuEntry("Display Rendered Without Texture", 2);
	glutAddMenuEntry("Display Rendered With Texture", 3);

	glutSetMenu(psmen);
	glutAddSubMenu("Display Options ...", optmen);
}

static void setup (void)
{
	glClearColor( 0.8,0.8,0.8,1.0 );

	// Load textures
	init_texture("Textures\\cylinderWall.bmp",	CYLINDER_WALL, textures);
	init_texture("Textures\\cylinderCap.bmp",	CYLINDER_CAP, textures);
	init_texture("Textures\\Front.bmp",			BUILD_FRONT, textures);
	init_texture("Textures\\BuildRight.bmp",	BUILD_RIGHT, textures);
	init_texture("Textures\\Concrete.bmp",		CONCRETE, textures);
	init_texture("Textures\\Skybox.bmp",		SKYBOX, textures);

	// Cylinder building
	cylinder.initialize(CYLINDER_WALL, CYLINDER_CAP);
	step1.initialize(CONCRETE, CYLINDER_CAP);
	step2.initialize(CONCRETE, CYLINDER_CAP);

	// Rectangle buildings
	build_front.initialize(BUILD_FRONT, BUILD_FRONT);
	build_right.initialize(BUILD_RIGHT, CONCRETE);
	foundation.initialize(CONCRETE, CONCRETE);

	// Water
	water.build();

	// Sky
	sky.initialize(SKYBOX);

	// Directional light
	sun.init();

	// Spot light
	spLight.init();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void draw		(void)
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if(bRendered)	glEnable(GL_TEXTURE_2D);
	else			glDisable(GL_TEXTURE_2D);

	if(bWireframe)
	{
		glLineWidth( 2.0 );
		glPolygonMode(GL_FRONT, GL_LINE);
	}
	else
		glPolygonMode(GL_FRONT, GL_FILL);

	glRotatef(5, 1,0,0);
	camera.transform(sky);
	sun.transform();

	if(bDrawGrid) draw_axis();
	
	glColor3f(0.9,0.9,0.9);

	// Cylinder building
	glPushMatrix();
	glTranslatef(0, 0, -100);
	glRotatef(40, 0, 1, 0);
	spLight.transform();	// Update spot light
	cylinder.draw();
	step1.draw();
	step2.draw();
	glPopMatrix();

	// Right cuboid building
	glPushMatrix();
	glTranslatef(100, 25, 25);
	build_right.draw();
	glTranslatef(-75, 0, 150);
	glRotatef( 90, 0, 1, 0 );
	build_right.draw();
	glPopMatrix();

	// Front timber building
	glPushMatrix();
	glTranslatef(20, 15, 0);
	build_front.draw();
	glPopMatrix();

	// Foundations
	glPushMatrix();
	glTranslatef(0, -15, 0);
	foundation.draw();
	glTranslatef(-325, 0, 0);
	foundation.draw();
	glPopMatrix();

	// Water
	glPushMatrix();
	glTranslatef(-160,-10,0);
	water.draw();
	glPopMatrix();

	glPopMatrix();
	glutSwapBuffers();
	Sleep(1);	// Frame limiter
}

static void reshape		(int w, int h)
{
	cWidth = w, cHeight = h;
	glViewport(0,0,cWidth,cHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective( 70.0, (float)cWidth/(float)cHeight, 0.1f, 5000.0f );
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
		mouseX = mouseY = 0;
		break;

	// Draw grid
	case 'G': bDrawGrid =! bDrawGrid;	break;

	// Wireframe
	case 'W': bWireframe =! bWireframe;	break;

	//Exit
	case 27: exit(0);
	}
}

static void idle		(void)
{
	static float cTime = 0.0;

	camera.update(mouseX, mouseY);
	water.update(cTime);

	glutPostRedisplay();
	cTime += 0.02f;
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

static void menu_func(int k)
{
	switch (k)
	{
	case 1:	bWireframe = 1; bRendered = 0; break;
	case 2:	bWireframe = 0; bRendered = 0; break;
	case 3: bWireframe = 0; bRendered = 1; break;
	}
}