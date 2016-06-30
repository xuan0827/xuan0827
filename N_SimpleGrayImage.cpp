/** @file SimpleGrayImage.cc
 *  Image Processing Lecture, WS 07/08, Uni Koblenz.
 *  Simple image class to read and write PGM Images.
 *
 *  @author     Detlev Droege
 *  @author     Frank Schmitt
 *  @created    November 2007
 */
#include "stdafx.h"
#include <cassert>
#include <cctype>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>

#include "N_SimpleGrayImage.h"


void N_SimpleGrayImage::alloc_mem(int wid, int hig)
{
	if (pixels) delete[] pixels; //delete old pixel data
	if (rows) delete[] rows; //delete old row pointers
	pixels = new byte[wid * hig];	// get memory for pixels
	rows = new byte*[h];		// get memory for row pointers
	byte *pp = pixels;			// let pp point to 1. pixel row
	for (int i = 0; i < h; i++) {	// for every row i
		rows[i] = pp;			// make rows[i] point to it
		pp += w;			// advance pp to next row
	}
}

void N_SimpleGrayImage::init_attributes()
{
	w = h = 0;
	pixels = NULL;
	rows = NULL;
}

N_SimpleGrayImage::N_SimpleGrayImage()
{
	init_attributes();
}

N_SimpleGrayImage::~N_SimpleGrayImage()
{
	delete [] pixels;
	delete [] rows;
	pixels = NULL;
	rows = NULL;
}

N_SimpleGrayImage::N_SimpleGrayImage(int wid, int hig)
{
	init_attributes();
	resize(wid, hig);
}

void N_SimpleGrayImage::resize(int wid, int hig)
{
	assert ((wid > 0) && (hig > 0));
	w = wid;
	h = hig;
	alloc_mem(wid, hig);
}
N_SimpleGrayImage::N_SimpleGrayImage( byte  *in_image, int nWidth, int nHeight )
{
	init_attributes();
	w = nWidth;
	h = nHeight;
	alloc_mem(w, h);
	memcpy( pixels, in_image, sizeof(byte)*w*h);
		
}

