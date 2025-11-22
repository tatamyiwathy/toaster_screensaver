//
//	file:bmp.h
//	date:10/14/97
//	auth:S.Ohtake
//
#ifndef __bmp_h__
#define __bmp_h__

#define		BITMAP_ID		IDB_BITMAP1

//typedef struct {
//	HBITMAP			hBitmap;
//	LPBITMAPINFO	lpBitmapInfo;
//	HPALETTE		hPalette;	
//}CELL_BITMAP;

typedef struct {
	int resource_id;
	int nCells;		//ÉZÉãñáêî
	int nCellW;
	int nCellH;
	HBITMAP			hBitmap;
	HBITMAP			hBmpMask;
	HPALETTE		hPalette;	
	LPBITMAPINFO	lpBitmapInfo;
}CELLINFO;

typedef struct {
	int nCelX,nCelY;
	CELLINFO	*pCellInfo;
}ANIME;


typedef struct {
	BYTE *pOffscrnBuff;
	int nWidth;
	int nHeight;
	HBRUSH hbr_prev;

	HDC hDC;
	HBITMAP hBMP,hBMPprev;
}OFFSCRNINFO;

int GetUsedColor( LPBITMAPINFO );
HPALETTE CreateBitmapPalette( LPBITMAPINFO );
void SetupResBmp(HWND,HINSTANCE,CELLINFO*);
void DeleteCellBmp(CELLINFO *);
void SetupAnime(ANIME*,CELLINFO *);
void SetAnimeCell( ANIME*, int);
void PlayAnime(ANIME*);
HBITMAP MakeOffScrn( HWND, OFFSCRNINFO* );
void DeleteOffscrn( OFFSCRNINFO* );
#endif	//end of __bmp_h_
