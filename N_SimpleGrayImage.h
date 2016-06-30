#ifndef N_SIMPLEGRAYIMAGE_H_
#define N_SIMPLEGRAYIMAGE_H_

#include <string>
#include <stdexcept>
#include <cassert>              // useful

typedef unsigned char byte;     // usual type for (gray) pixels

class N_SimpleGrayImage
{
	int	w;		///< width of the image
	int	h;		///< height of the image
	byte	*pixels;	///< pointer to the pixel memory
	byte	**rows;		///< pointer array to rows

	void alloc_mem (int wid, int hig);	// allocate and init memory (internal)
	void init_attributes();		// initialize (internal)

public:
	/// Default constructor: creates empty image.
	/// not normally used.
	N_SimpleGrayImage();

	/// Constructor: create image of given size.
	/// The pixels are not initialized.
	/// @param wid  width of the desired image
	/// @param hid  heigth of the desired image
	N_SimpleGrayImage(int wid, int hig);

	N_SimpleGrayImage( byte  *in_image, int nWidth, int nHeight );

	/// Destructor: releases all data.
	/// Free all allocated memory.
	virtual ~N_SimpleGrayImage ();		// destructor

	/// return the width of the image
	int width() const  { return w; }

	/// return the height of the image
	int height() const { return h; }

	/// return pointer to array of row pointers in byte**p = (byte**)img;
	operator byte ** () { return rows; }

	/// Resize the image to the given size.
	/// The pixels are not initialized.
	/// @param wid  new desired width
	/// @param hid  new desired heigth
	void resize(int wid, int hig);

};

#endif /*SIMPLEGRAYIMAGE_H_*/
