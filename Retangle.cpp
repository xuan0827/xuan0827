// Retangle.cpp: implementation of the Retangle class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Retangle.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Retangle::Retangle(int x ,int y , int ID)
{
	left = x ;
	top = y;
	right = x + 2;
	bottom = y + 2;
	Image = NULL;
	
	this->ID = ID; 
	select  = false;
	resize  = false;
	Is_Lock = false;

}

Retangle::~Retangle()
{
	if(Image != NULL)
		Image->~CxImage();
	delete Image;
}

void Retangle::Is_Move_Resize(int x  , int y)
{
	
	if( x > left && x < right -4 && y > top &&  y < bottom -4 )
		select = true;
	

	if( abs(x-right) < 5 && abs(y-bottom) < 5 ) 
	{
		resize = true;
	}
}

void Retangle::ShowSize(CDC& pDC)
{
	CPen pen(PS_SOLID,2,RGB(255,0,0));
	CPen *oldpen = pDC.SelectObject(&pen);
	pDC.SelectStockObject(NULL_BRUSH);	
	
	char* strButtom = new char[10];
	itoa(abs(bottom - top), strButtom , 10);
	char* strRight = new char[10];
	itoa(abs(right-left), strRight , 10);
	
	pDC.TextOut( right , top , strButtom);
	pDC.TextOut( right  , top + 18, strRight);
	
	delete []strButtom;
	delete []strRight;
	
}

void Retangle::Draw(CDC& pDC )
{

	CPen pen(PS_SOLID,3,RGB(255,0,0));
	CPen *oldpen = pDC.SelectObject(&pen);
	pDC.SelectStockObject(NULL_BRUSH);
	

	CRect rect( CPoint(  left , top ), CPoint( right , bottom ));
	
//	CRect rect1( CPoint(  left - 2, top - 2), CPoint( left + 1 , top + 1  ));
//	CRect rect2( CPoint(  right - 1, buttom - 1), CPoint( right + 2 , buttom + 2  ));
	
	pDC.Rectangle(rect);
//	pDC.Rectangle(rect1);
//	pDC.Rectangle(rect2);
}

void Retangle::SetRB(int x  , int y)
{
	right = x ;
	bottom = y ;
}

void Retangle::SetSize(int x  , int y)
{
	right = left + x ;
	bottom = top + y ;
}

void Retangle::Move(int x  , int y , int W , int H )
{	
	if( !Is_Lock ) {
		diff_left   = x - left;
		diff_top    = y - top;
		diff_right  = x - right;
		diff_buttom = y - bottom;

		Is_Lock = true;
	}

	int tmp_top    = (y - diff_top) ;
	int tmp_buttom = (y - diff_buttom);
	int tmp_left   = (x - diff_left);
	int tmp_right  = (x - diff_right);
		
	if( tmp_top >0  &&  tmp_buttom < H-1  &&  tmp_left >0  &&  tmp_right < W-1 ) {
		top    = tmp_top;
		bottom = tmp_buttom;
		left   = tmp_left;
		right  = tmp_right;
	}	

}

void Retangle::Resize(int x  , int y)
{
	right += (x - right);
	bottom += (y - bottom);
}

void Retangle::PixelCopy(BYTE* pSrc , int Width ,int Height)
{
	BYTE* data = new BYTE[abs(right - left)*abs(top - bottom)*3];
	
	//設定24位元
	pSrc += ((Height-bottom) * Width + left*3 );
	//pSrc += ((Height-buttom) * Width + left );
	
	BYTE *temp = data;
	
	for(int i = 0 ; i < abs(top - bottom) ; i++)
		for(int j = 0 ; j < abs(right - left) ; j++)	
		{
			temp [0] = pSrc[ (i * Width + j*3)];
			temp [1] = pSrc[ (i * Width + j*3) + 1];
			temp [2] = pSrc[ (i * Width + j*3) + 2];
			temp  += 3;
			
		}
		
		Image = new CxImage(1);
		//	image->CreateFromArray( data , GetWidth() , GetHeight()	, 8, GetWidth()*3 , false);
		Image->CreateFromArray( data , GetWidth() , GetHeight()	, 24, GetWidth()*3 , false);
		
		delete []data;
		
		
}

void Retangle::SavePatern(CString s)
{
	Image->Save(s,1);
}