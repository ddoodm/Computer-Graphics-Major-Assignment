/* ================================================	*\
 *	    UTS: Introduction to Computer Graphics		*
 *         Major Assignment | Bitmap Parser			*
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
 *					  SIMPLE OPENGL IMAGE LIBRARY	*
\*													*/

#include <stdio.h>
#include <Windows.h>		// MessageBox

typedef unsigned char uchar;
typedef unsigned int uint;

// No auto-padding
#pragma pack(1)

struct BmpFileHeader
{
	uchar	magic[2];		// 2 b	-	B,M
	int		bfSize;			// 4 b	-	File Size
	short	reserved[2];	// 4 b	-	
	int		bfOffBits;		// 4 b	-	Offset to pixel array
};

struct BmpInfo
{
	int		biSize;			// 4 b	-	Bitmap Info Size
	int		iWidth;			// 4 b	-	Image width
	int		iHeight;		// 4 b	-	Image height
	short	planes;			// 2 b	-	# of planes
	short	bpp;			// 2 b	-	Bits Per Pixel
	int		compression;	// 4 b	-	BI_COMPRESSION (BI_RGB = 0)
	int		biSizeImage;	// 4 b	-	Size of pixel array
	int		biXPixPerM;		// 4 b	-	X pixels per meter
	int		biYPixPerM;		// 4 b	-	Y pixels per meter
	int		biClrUsed;		// 4 b
	int		biClrImportant;	// 4 b
};

#pragma pack()

struct StImage
{
	uint	width;
	uint	height;
	float*	rgba;

	StImage(void) : width(0), height(0), rgba(0) { }
};

class BmpImage
{
public:
	BmpFileHeader	fileHeader;
	BmpInfo			bmpInfo;
	StImage			data;

	BmpImage (void)
	{  }
	~BmpImage (void)
	{ if(data.rgba) delete[] data.rgba; } // Safe delete ( may not have initialized )


	int load (char* path)
	{
		FILE* fp = fopen(path, "rb");
		if(!fp)
			return exit_nullfile(path);

		fread(&fileHeader, 1, sizeof(BmpFileHeader), fp);
		fread(&bmpInfo, 1, sizeof(BmpInfo), fp);

		// Bitmap is not 24-bit pixel mode, so we can't continue
		if(bmpInfo.bpp != 24)	return exit_colourMode(path);

		// Seek to the pixel array at the offset read from the file header
		fseek(fp, fileHeader.bfOffBits, SEEK_SET);

		// Set W and H
		data.width = bmpInfo.iWidth;
		data.height = bmpInfo.iHeight;

		// Allocate memory on the heap for width * height pixels, 4 components each
		data.rgba = new float[ data.width * data.height * 4 ];
		if(data.rgba == NULL)	return exit_memalloc(path);

		// Padding, in case the width is not divisible by 8
		int iPadding = (data.width * 3) % 4;
		
		for(uint y=0; y<data.height; y++)
		{
			for(uint x=0; x<data.width; x++)
			{
				// fread in the RGB (in reversed order, mind you)
				uchar r,g,b;
				fread(&b, 1, 1, fp);
				fread(&g, 1, 1, fp);
				fread(&r, 1, 1, fp);

				// And pack
				int i = (y*data.width + x) * 4;
				data.rgba[i + 0] = (float)r/255.0f;	// R
				data.rgba[i + 1] = (float)g/255.0f;	// G
				data.rgba[i + 2] = (float)b/255.0f;	// B
				data.rgba[i + 3] = 1.0f;			// A
			}

			// Stride if there is padding because of a bad width:
			if(iPadding != 0)
				fread(NULL, 1, 4 - iPadding, fp);

		}

		// Should be done
		fclose(fp);
		//fprintf(stdout, "Status: Parsed BMP <%s> successfully.\n", path);
		return 0;
	}

private:

	void mbox_alert(char* path)
	{
		char message[255];
		sprintf(message, "A fatal error occurred when parsing BMP <%s>.\nPlease check the console for details.", path);
		MessageBox(NULL, message, "Fatal BMP Parser Error", MB_OK | MB_ICONEXCLAMATION);
	}

	int exit_nullfile(char* path)
	{
		fprintf(stderr, "\n  BMP ERROR: Texture <%s> cannot be found.\n",
			path);
		mbox_alert(path);
		return 3;
	}

	int exit_colourMode(char* path)
	{
		fprintf(stderr, "\n  BMP ERROR: <%s> cannot be decoded. It is a %d-bit image file; this program only reads 24-bit images.\n",
			path, bmpInfo.bpp);
		mbox_alert(path);
		return 1;
	}

	int exit_memalloc(char* path)
	{
		fprintf(stderr, "\n  BMP ERROR: Could not allocate memory for <%s>. The BMP is %d x %d pixels in size.\n",
			path, data.width, data.height);
		mbox_alert(path);
		return 2;
	}
};