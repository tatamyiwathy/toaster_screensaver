//
//	file:anime.c
//	date:10/14/94
//	auth:tatamyiwathy
//
#include <windows.h>
#include <stdlib.h>
#include "..\Assets\resource.h"
#include "anime.h"

//
//	使用されている色数の取得
//
int GetUsedColor( LPBITMAPINFO lpBitmapInfo )
{
	int num_color;
	//
	//	色数カウント
	//
	num_color = lpBitmapInfo->bmiHeader.biClrUsed;
	if( 0==num_color ){
		if( 8 == lpBitmapInfo->bmiHeader.biBitCount ){
			num_color = 256;
		}else if( 4 == lpBitmapInfo->bmiHeader.biBitCount ){
			num_color = 16;
		}else{
			return 0;
		}
	}
	return num_color;
}
//
//	ビットマップから論理パレットを作る
//
HPALETTE CreateBitmapPalette( LPBITMAPINFO lpBitmapInfo )
{
	int i,num_color;
	LPLOGPALETTE	lpLogPalette;
	HPALETTE		hPal;
	
	
	num_color = GetUsedColor( lpBitmapInfo );
//	if( 256 < num_color )
//		return NULL;
	//
	//	パレット作成
	//
	lpLogPalette = (LPLOGPALETTE)malloc( num_color*sizeof( LOGPALETTE ) );
	if( NULL == lpLogPalette ){
		return NULL;
	}
	lpLogPalette->palVersion = 0x300;
	lpLogPalette->palNumEntries = num_color;
	for( i=0; i<num_color; i++ ){
		lpLogPalette->palPalEntry[i].peRed   = lpBitmapInfo->bmiColors[i].rgbRed;
		lpLogPalette->palPalEntry[i].peGreen = lpBitmapInfo->bmiColors[i].rgbGreen;
		lpLogPalette->palPalEntry[i].peBlue  = lpBitmapInfo->bmiColors[i].rgbBlue;
		lpLogPalette->palPalEntry[i].peFlags  = 0;
	}
	hPal = CreatePalette( lpLogPalette );
	free( lpLogPalette );
	return hPal;
}

//
//	セル用ビットマップの設定
//
void SetupResBmp(HWND hWnd,HINSTANCE hInstance, CELLINFO *p_cell)
{
	HGLOBAL	hRes;
	LPBITMAPINFO	lpBitmapInfo;
	HDC	hDC;
	HPALETTE hPalOld;
	int n;

	hRes = LoadResource( hInstance, FindResource( hInstance, MAKEINTRESOURCE(p_cell->resource_id),RT_BITMAP));
	lpBitmapInfo = (LPBITMAPINFO)LockResource(hRes);
	p_cell->lpBitmapInfo = lpBitmapInfo;

	p_cell->hPalette = CreateBitmapPalette( lpBitmapInfo );

	//
	//	ビットマップ作成
	//
	n = GetUsedColor( lpBitmapInfo );
	hDC = GetDC( hWnd );
	hPalOld = SelectPalette( hDC, p_cell->hPalette,FALSE );
	RealizePalette(hDC);
	p_cell->hBitmap = CreateDIBitmap(	hDC,
										&lpBitmapInfo->bmiHeader,
										CBM_INIT,
										(BYTE*)( (BYTE*)lpBitmapInfo + sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*n),
										lpBitmapInfo,
										DIB_RGB_COLORS );
	p_cell->hBmpMask = LoadBitmap( hInstance, MAKEINTRESOURCE(IDB_BITMAP3) );
	SelectPalette( hDC, hPalOld,FALSE);
	ReleaseDC( hWnd, hDC );
}
//
//	セルの後始末
//	
//
void DeleteCellBmp(CELLINFO *p_cell)
{
	DeleteObject( p_cell->hBitmap );
	DeleteObject( p_cell->hBmpMask );
	DeleteObject( p_cell->hPalette );
}
//
//	アニメーションワーク初期化
//
void SetupAnime(ANIME *anime, CELLINFO *p_cell)
{
	anime->nCelX = 0;
	anime->nCelY = 0;
	anime->pCellInfo = p_cell;
}
//
//	カレントセル番号の設定
//
void SetAnimeCell( ANIME *anime, int c )
{
	anime->nCelX = c * anime->pCellInfo->nCellW;
}
//
//	アニメプレイ
//
void PlayAnime(ANIME *anime)
{
	anime->nCelX += anime->pCellInfo->nCellW;
	if( anime->nCelX >= (anime->pCellInfo->nCellW * anime->pCellInfo->nCells)  )
		anime->nCelX = 0;
}

//
//	オフスクリーンビットマップの作成
//	グローバル変数'pOffscrnBuff'にオフスクリーンのピクセルデータの配列への
//	アドレスが入る
//
//	アプリケーションを終了させる時は
//	明示的にメモリブロックを開放しなければならない
//
HBITMAP MakeOffScrn( HWND hWnd, OFFSCRNINFO *p_offscrn )
{
	RECT r;
	HDC hDC;
#if 0
	HBITMAP hBitmap;
	int iScrnSize;
	UINT uBitsPixel;

	hDC = GetDC( hWnd );
	uBitsPixel = GetDeviceCaps(hDC,BITSPIXEL);

	GetClientRect( hWnd,&r );
	p_offscrn->nHeight = r.bottom - r.top;
	p_offscrn->nWidth = r.right - r.left;

	iScrnSize = (r.right-r.left)*(r.bottom-r.top);
	p_offscrn->pOffscrnBuff = calloc( iScrnSize,uBitsPixel/8 );
	if( NULL == p_offscrn->pOffscrnBuff ){
		MessageBox( hWnd, "メモリの確保の失敗","",MB_OK);
		SendMessage( hWnd, WM_CLOSE,0,0 );
		return 0;
	}
//	memset( pOffscrnBuff, 0x20, iScrnSize );
	hBitmap = CreateBitmap( 
				r.right-r.left,		//nWidth
				r.bottom-r.top,		//nHeight
				1,					//cPlanes
				uBitsPixel,			//cBitPerPel
				p_offscrn->pOffscrnBuff );
	ReleaseDC( hWnd, hDC );
	return hBitmap;
#else
	GetClientRect( hWnd,&r );
	p_offscrn->nHeight = r.bottom - r.top;
	p_offscrn->nWidth = r.right - r.left;
	hDC = GetDC( hWnd );
	p_offscrn->hDC = CreateCompatibleDC( hDC );
	p_offscrn->hBMP = CreateCompatibleBitmap( hDC,r.right-r.left,r.bottom-r.top);
	p_offscrn->hBMPprev = SelectObject( p_offscrn->hDC, p_offscrn->hBMP );
	p_offscrn->hbr_prev = SelectObject( p_offscrn->hDC, GetStockObject( BLACK_BRUSH ) );
	ReleaseDC( hWnd, hDC );
	return p_offscrn->hBMP;
#endif
}
//
//	オフスクリーン削除
//
void DeleteOffscrn( OFFSCRNINFO *p_offscrn )
{
	SelectObject( p_offscrn->hDC,p_offscrn->hbr_prev );
	SelectObject( p_offscrn->hDC,p_offscrn->hBMPprev);
	DeleteDC( p_offscrn->hDC );
	p_offscrn->hDC = NULL;
	DeleteObject( p_offscrn->hBMP );
	p_offscrn->hBMP = NULL;
	p_offscrn->hBMPprev = NULL;
}
