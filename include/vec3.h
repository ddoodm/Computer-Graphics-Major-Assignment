
/*************************************\
  3-Dimensional Geometric Vector
\*************************************/
template <typename T> struct vec3
{
	T x,y,z;

	vec3 (void) : x(0), y(0), z(0) {}
	vec3 (T x, T y, T z) : x(x), y(y), z(z) {}

	inline T magnitude ( void ) const
	{
		return magnitude(*this);
	}

	T magnitude ( const vec3& rhs ) const
	{
		return sqrt( rhs.x*rhs.x + rhs.y*rhs.y + rhs.z*rhs.z );
	}

	inline vec3& normalize ( void ) const
	{
		return normalize(*this);
	}

	vec3& normalize ( const vec3& rhs ) const
	{
		T mag = magnitude( rhs );
		return vec3(
			rhs.x / mag,
			rhs.y / mag,
			rhs.z / mag);
	}

	vec3& operator- ( const vec3& rhs ) const
	{
		return vec3(x - rhs.x, y-rhs.y, z-rhs.z);
	}

	vec3& operator+ ( const vec3& rhs ) const
	{
		return vec3(x + rhs.x, y+rhs.y, z+rhs.z);
	}

	vec3& operator+= ( const vec3& rhs )
	{
		x += rhs.x, y += rhs.y, z += rhs.z;
		return *this;
	}

	// Cross product
	vec3& operator* ( const vec3& rhs ) const
	{
		T cx = y*rhs.z - z*rhs.y;
		T cy = z*rhs.x - x*rhs.z;
		T cz = x*rhs.y - y*rhs.x;
		return vec3( cx, cy, cz );
	}

	// Multiply vector by scalar
	vec3& operator* ( const T rhs ) const
	{
		return vec3( x*rhs, y*rhs, z*rhs );
	}

	// Divide vector by scalar
	vec3& operator/ ( const T rhs ) const
	{
		return vec3( x/rhs, y/rhs, z/rhs );
	}

	// Dot product
	T dot (const vec3& rhs) const
	{
		return rhs.x*x + rhs.y*y + rhs.z*z;
	}
};

typedef vec3<GLfloat> vec3f;
