#pragma once

#include <Windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>

using namespace std;

class ObjModel
{
public:
	vector< array<float,3> > vertCoords;	
	vector< array<float,2> > texCoords;
	vector< array<float,3> > normals;
	vector< array<float,8> > vertices;

	inline ObjModel ()
	{  }
	inline ~ObjModel ()
	{
		vertCoords.clear();
		texCoords.clear();
		vertices.clear();
	}

	inline int load (const char* path)
	{
		fprintf(stdout, "Status: Loading OBJ file <%s>...\n", path);
		read_lines(path);
		print_info(path);

		return 1;
	}

	inline int getSize (void)
	{
		return vertices.size() * 8;
	}

	inline int getComponentCount (void)
	{
		return 8;
	}

	inline void getVertices (float buf[])
	{
		for(int i=0; i<vertices.size(); i++)
		{
			array<float,8> vert = vertices[i];
			copy(vert.begin(), vert.end(), buf + i*8);
		}
	}

private:
	int read_lines (const char* path)
	{
		ifstream ifs(path);
		if(!ifs)
			return exit_nullfile(path);

		int linec=1;
		while(ifs.good())
		{
			string str;
			getline(ifs, str);
			process_line(str, linec);
			linec++;
		}
		fprintf(stdout, "Status: Read %d lines of file <%s>.\n", linec, path);

		return 0;
	}

	void process_line(string str, int linec)
	{
		// == Prefix strings ==
		string prefix_comment("#");
		string prefix_vertex("v ");
		string prefix_texcoord("vt");
		string prefix_normal("vn");
		string prefix_face("f ");

		// Comment
		if (!str.compare(0, prefix_comment.size(), prefix_comment))
			process_comment(str, prefix_comment, linec);

		// Vertex
		else if (!str.compare(0, prefix_vertex.size(), prefix_vertex))
			process_vertex(str.substr(prefix_vertex.size()));

		// Texture Coordinate
		else if (!str.compare(0, prefix_texcoord.size(), prefix_texcoord))
			process_texcoord(str.substr(prefix_texcoord.size()));

		// Vertex normal
		else if (!str.compare(0, prefix_normal.size(), prefix_normal))
			process_normal(str.substr(prefix_normal.size()));

		// Face
		else if (!str.compare(0, prefix_face.size(), prefix_face))
			process_face(str.substr(prefix_face.size()));
	}

	void process_comment(string str, string prefix, int linec)
	{
//		fprintf(stdout, "OBJ Comment #%d:  %s \n", linec, str.substr(prefix.size()).c_str());
		return;
	}

	void process_vertex(string triplet)
	{
		string s = triplet.substr( triplet.find_first_not_of(" "), string::npos );
		istringstream iss(s);
		array<float,3> coord = {0,0,0};

		for(int i=0; iss && i<3; i++)
		{
			string sub;
			iss >> sub;
			coord[i] = ::atof(sub.c_str());
		}

		vertCoords.push_back( coord );
	}

	void process_texcoord(string doublet)
	{
		string s = doublet.substr( doublet.find_first_not_of(" "), string::npos );
		istringstream iss(s);
		array<float,2> coord = {0,0};

		for(int i=0; iss && i<2; i++)
		{
			string sub;
			iss >> sub;
			coord[i] = ::atof(sub.c_str());
		}

		texCoords.push_back( coord );
	}

	void process_normal(string triplet)
	{
		string s = triplet.substr( triplet.find_first_not_of(" "), string::npos );
		istringstream iss(s);
		array<float,3> coord = {0,0,0};

		for(int i=0; iss && i<3; i++)
		{
			string sub;
			iss >> sub;
			coord[i] = ::atof(sub.c_str());
		}

		normals.push_back( coord );
	}

	void process_face(string str)
	{
		// 'str' is in the form v/vt v/vt v/vt, so I split it up first with an istringstream
		string s = str.substr( str.find_first_not_of(" "), string::npos );
		istringstream iss(s);

		// Three components (must be triangulated)
		for(int i=0; i<3; i++)
		{
			string sub;
			iss >> sub;

			// Sub is now in the form v/vt for vertex [i]
			int iVert = atoi(strtok((char*)sub.c_str(), "/")) - 1; // NOT ZERO NUMBERED!!!!
			int iTex = atoi(strtok(NULL, "/")) - 1;
			int iNorm = atoi(strtok(NULL, "/")) - 1;

			// Now that we have indices, we can add them into an array
			array<float,3> vert = vertCoords[iVert];
			array<float,2> tex = texCoords[iTex];
			array<float,3> norm = normals[iNorm];
			array<float,8> composite;
			for(int j=0; j<3; j++)	composite[j] = vert[j];
			for(int j=3; j<5; j++)	composite[j] = tex[j-3];
			for(int j=5; j<8; j++)	composite[j] = norm[j-5];
			vertices.push_back(composite);
		}
	}

	void print_info (const char* path)
	{
		fprintf(stdout, " * OBJ Model <%s> parsed with %d vertex coordinates and %d texture coordinates. After index mapping, the model is %d vertices total.\n",
			path, vertCoords.size(), texCoords.size(), vertices.size());
	}

	int exit_nullfile (const char* path)
	{
		fprintf(stderr, "\n  OBJ ERROR: Could not locate OBJ model file <%s>.\n", path);
		mbox_error(path);
		return 1;
	}

	void mbox_error (const char* path)
	{
		char message[255];
		sprintf(message,
			"A fatal error occurred while parsing OBJ model <%s>.\n"
			"Please check the console for details.", path);
		MessageBox(NULL, message, "Fatal OBJ Loader Error", MB_OK | MB_ICONEXCLAMATION );
	}
};