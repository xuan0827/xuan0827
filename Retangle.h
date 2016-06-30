// Retangle.h: interface for the Retangle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RETANGLE_H__2891D14A_70C3_4668_BBBC_9440B35E5AA4__INCLUDED_)
#define AFX_RETANGLE_H__2891D14A_70C3_4668_BBBC_9440B35E5AA4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Retangle  
{
public:
	Retangle(int x ,int y , int ID);
	virtual ~Retangle();
	void Is_Move_Resize( int x  , int y );
	void Draw(CDC& pDC );
	void SetRB(int x  , int y);
	void Move(int x , int y , int W , int H );
	void Resize(int x  , int y);
	void PixelCopy(BYTE* pSrc , int Width ,int Height);
	void ShowSize(CDC& pDC);
	
	void SetSize(int x  , int y);
	inline void SetSelect(bool b){select = b;};
	inline void SetResize(bool b){ resize = b;};
	
	inline bool GetSelect(){return select;};
	inline bool GetResize(){return resize;};
	inline int GetSize(){return abs(left - right) * abs(top - bottom);};
	inline CxImage* GetImage(){return Image;};
	inline int GetWidth(){return abs(left - right); };
	inline int GetHeight(){return abs(top - bottom);};
	void SavePatern(CString);
	inline BYTE* GetData(){return Image->GetBits();};
	inline void ReSample(int w , int h){ Image->Resample(w,h); };
	inline void SetLock( bool s ){ Is_Lock = s; }

	int left;
	int top;
	int right;
	int bottom;

 private:
	
	bool Is_Lock;
	int diff_left;
	int diff_top;
	int diff_right;
	int diff_buttom;

	int ID;
	bool select;
	bool resize;
	CxImage* Image;

};

#endif // !defined(AFX_RETANGLE_H__2891D14A_70C3_4668_BBBC_9440B35E5AA4__INCLUDED_)
