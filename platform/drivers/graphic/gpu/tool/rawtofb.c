#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <png.h>
#include <stdlib.h>

//raw file format:
//header:
	//width  = read_long(fp);
	//height = read_long(fp);
	//stride = read_long(fp);
	//format = read_long(fp);

// Read a 32-bit signed integer.
static int read_long(FILE * fp)
{
	unsigned char b0, b1, b2, b3;	/* Bytes from file */

	b0 = getc(fp);
	b1 = getc(fp);
	b2 = getc(fp);
	b3 = getc(fp);

	return ((int)(((((b3 << 8) | b2) << 8) | b1) << 8) | b0);
}

/**
*/
int writeImage(char *filename, int stride, int height, int bpp ,char *fbbuf, int scale_rate, char *title)
{
	int code = 0;
	FILE *fp = NULL;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep row = NULL;
	char *fbd = (char *)malloc(stride * height / scale_rate / scale_rate);
	// Open file for writing (binary mode)
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s for writing\n", filename);
		code = 1;
		return code;
	}
	char *cl = fbbuf;
	char *cr = fbd;
	for (int i = 0; i < height / scale_rate; i++) {
		cl = fbbuf + stride * i * scale_rate;
		for (int c = 0; c < stride/bpp/scale_rate; c++) {
			memcpy(cr ,(cl + c * bpp * scale_rate),bpp);
			cr += bpp;
		}
	}

	fwrite(fbd, stride * height  / scale_rate / scale_rate, 1, fp);

	if (fp != NULL)
		fclose(fp);
	return code;
}

int main(int argc, char *argv[])
{
    // Make sure that the output filename argument has been provided
	char *fbbuf = NULL;

	int fbbufsize = 0;

	int32_t raw_width;	/*! Width of the buffer in pixels. */
	int32_t raw_height;	/*! Height of the buffer in pixels. */
	int32_t raw_stride;
	int32_t format;
	int scale_rate = 1;
	FILE *fp = NULL;
	if (argc != 4) {
		fprintf(stderr, "usage: \r\n   raw2fb fbfilename rawfile  scale_rate\n");
		return 1;
	}
	// Specify an output image size

	scale_rate = atoi(argv[3]);
	fp = fopen(argv[2], "rb");

	raw_width = read_long(fp);
	raw_height = read_long(fp);
	raw_stride = read_long(fp);
	format = read_long(fp);
	printf("raw_width:%d", raw_width);
	printf("raw_height:%d", raw_height);
	printf("raw_stride:%d", raw_stride);
	printf("format:%d", format);
	fbbufsize = raw_stride * raw_height;

	fbbuf = (char *)malloc(fbbufsize);
	fread(fbbuf, fbbufsize, 1, fp);
	fclose(fp);

	printf("Saving fbfile\n");
	int result = writeImage(argv[1], raw_stride, raw_height,raw_stride/raw_width ,fbbuf, scale_rate, "This is my test image");
	if (result) {
		printf("Saving err\n");
	}

	return result;
}
