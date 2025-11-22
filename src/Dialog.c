#include <windows.h>
#include "saver.h"
#include "anime.h"

static void error( HWND hWnd, int err )
{
	switch( err ){
	case 0:
		MessageBox( hWnd, "パレットの作成に失敗しちゃった","設定ダイアログ",MB_OK|MB_ICONEXCLAMATION );
		break;
	case 1:
		MessageBox( hWnd, "LoadBitmapに失敗しちゃった","設定ダイアログ",MB_OK|MB_ICONEXCLAMATION );
		break;
	}
}

LRESULT CALLBACK DialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;

	switch( msg ){
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if( wParam == ID_OK ){
			EndDialog( hDlg, 0 );
			return TRUE;
		}
	case WM_DESTROY:
		return TRUE;
	case WM_PAINT:
		BeginPaint( hDlg, &ps );
		EndPaint( hDlg, &ps );
		return TRUE;
	}
	return FALSE;
}