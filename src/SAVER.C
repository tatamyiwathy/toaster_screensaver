//
//	file:saver.c
//	date:10/17/97
//	auth:tatamyiwathy
//
#include	<windows.h>
#include	<winnls32.h>
#include	<stdlib.h>

#include	"saver.h"
#include	"anime.h"

#define		NUM_TOASTER	16


//STATIC変数
UINT		oldval;
HANDLE		hMutex;
HINSTANCE	ghInstance;
LPSTR		glpszCmdParam;
LPCSTR		gszAppName = "Toaster";
HWND		hWndPreview, hWndMain;
BOOL		bPreview, bSetting;
POINT		ptCurPos;
int			execute_mode;
UINT		uTimer;
RECT		rScreen;
OSVERSIONINFO	OsVersionInfo;
OFFSCRNINFO	offscrn;
HBITMAP		hBmpOffScrn;
BOOL		inDialog;

typedef struct {
	int	x, y;
	int mx, my;
	ANIME anime;
}TOASTER_SPRITE;
TOASTER_SPRITE toasters[NUM_TOASTER];
CELLINFO	cell_toaster = { IDB_BITMAP1,8,140,180,NULL,NULL,NULL };


#define		EXEC_PREVIEW		1
#define		EXEC_FULL_SCREEN	2

#define		ID_TIMER1			1

////////////////////////////////////////////////////////////////////////
//	コマンドライン（グローバル変数）
#define CmdOpMax 5
int argc = 0;
char* argv[CmdOpMax];

//プロトタイプ
LRESULT CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);


//
//	トースター更新
//
void UpdateToaster(TOASTER_SPRITE* wk, OFFSCRNINFO* o)
{
	wk->x += wk->mx;
	wk->y += wk->my;

	if (wk->x < 0 - wk->anime.pCellInfo->nCellW)
		wk->x = o->nWidth;
	if (wk->y > o->nHeight)
		wk->y = 0 - wk->anime.pCellInfo->nCellH;
}


//
//パスワードの確認
//
BOOL VerifyPassword(HWND hWnd)
{	//パスワードの確認
	HINSTANCE hPasswordCpl = LoadLibrary("PASSWORD.CPL");
	typedef BOOL(WINAPI* VERIFYSCREENSAVEPWD)(HWND hwnd);
	VERIFYSCREENSAVEPWD VerifyScreenSavePwd = (VERIFYSCREENSAVEPWD)GetProcAddress(hPasswordCpl, "VerifyScreenSavePwd");

	if (hPasswordCpl != NULL) {
		if (!VerifyScreenSavePwd(hWnd)) {
			FreeLibrary(hPasswordCpl);
			return FALSE;
		}
	}
	FreeLibrary(hPasswordCpl);
	return TRUE;
}


/*----------------------------------------------------------------------------
	ウィンドウプロシージャ
----------------------------------------------------------------------------*/
LRESULT WINAPI WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC	hDC;
	PAINTSTRUCT	ps;
	int i, j;
	HPALETTE	hOldPal;
	HDC hMemoryDC;
	HBITMAP hBmpOld;

	switch (message) {
	case WM_CREATE:
		inDialog = FALSE;
		OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&OsVersionInfo);

		GetClientRect(hWnd, &rScreen);
		int width = rScreen.right - rScreen.left;
		int height = rScreen.bottom - rScreen.top;
		SetupResBmp(hWnd, ghInstance, &cell_toaster);
		for (i = 0; i < NUM_TOASTER; i++) {
			SetupAnime(&toasters[i].anime, &cell_toaster);
			SetAnimeCell(&toasters[i].anime, rand() % cell_toaster.nCells);

			toasters[i].x = rand() % width;
			toasters[i].y = rand() % height;
			toasters[i].mx = (rand() % 4) * 2 + 2;
			toasters[i].my = toasters[i].mx / 2;
			toasters[i].mx = -toasters[i].mx;
		}
		hBmpOffScrn = MakeOffScrn(hWnd, &offscrn);

		GetCursorPos(&ptCurPos);
		uTimer = 0;
		uTimer = SetTimer(hWnd, ID_TIMER1, 100, NULL);
		if (0 == uTimer) {
			MessageBox(hWnd, "fault 'SetTimer'", "致命的なエラー", MB_OK | MB_ICONEXCLAMATION);
			DestroyWindow(hWnd);
		}
		if (execute_mode == EXEC_FULL_SCREEN) {
			WINNLSEnableIME(NULL, FALSE);
			ShowCursor(FALSE);
			if (VER_PLATFORM_WIN32_NT != OsVersionInfo.dwPlatformId)
				//ctrl-alt-tab,atl-tabを防ぐWin95のみ
				SystemParametersInfo(SPI_SCREENSAVERRUNNING, 1, &oldval, 0);
		}
		break;
	case WM_TIMER:
		if (ID_TIMER1 != wParam)
			break;
		for (i = 0; i < NUM_TOASTER; i++) {
			UpdateToaster(&toasters[i], &offscrn);
			PlayAnime(&toasters[i].anime);
		}
		InvalidateRect(hWnd, &rScreen, FALSE);
		UpdateWindow(hWnd);
		break;
	case WM_PALETTECHANGED:
		/* パレット変更に応答可能でなく、パレット表示が
		 * パレット変更を引き起こしたら書き直します．
		 */
		 // wPalam･･･システムパレットを変更させたウィンドウのハンドル。
		if ((HWND)wParam != hWnd) {
			hDC = GetDC(hWnd);
			//  オリジナルパレットに変更する。
			hOldPal = SelectPalette(hDC, cell_toaster.hPalette, 0);
			i = RealizePalette(hDC);
			if (i) {
				UpdateColors(hDC);
			}
			SelectPalette(hDC, hOldPal, 0);
			ReleaseDC(hWnd, hDC);
		}
		break;
		//  ウィンドウが間もなく入力フォーカスを受け取ることを伝える。
		//  入力フォーカスを受け取ったときに論理パレットを実現すると、
		//  そのウィンドウはTRUEを返し、そうでなければ、FALSEを返す。
	case WM_QUERYNEWPALETTE:
		/* パレット表示がパレット変更を引き起こしたら、
		* すべてを書き直します．*/
		// このブロックの関数は全てWindowsAPI。
		hDC = GetDC(hWnd);
		// 論理パレット選択（自分のパレットに）。
		hOldPal = SelectPalette(hDC, cell_toaster.hPalette, FALSE);
		// パレットを自分のディスプレイ・コンテキストにマッピング。
		i = RealizePalette(hDC);
		// 元のパレットに戻す。
		SelectPalette(hDC, hOldPal, 0);
		ReleaseDC(hWnd, hDC);
		if (i) {
			// クライアント領域全体を更新領域へ追加し、
			// バックグラウンドを消去する。
			InvalidateRect(hWnd, (LPRECT)(NULL), TRUE);
		}
		break;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		if (NULL == cell_toaster.hPalette) {
			EndPaint(hWnd, &ps);
			break;
		}
		hOldPal = SelectPalette(hDC, cell_toaster.hPalette, FALSE);
		i = RealizePalette(hDC);

		//オフスクリーンを黒く塗りつぶす
		Rectangle(offscrn.hDC, 0, 0, offscrn.nWidth, offscrn.nHeight);

		for (i = 0; i < NUM_TOASTER; i++) {
			hMemoryDC = CreateCompatibleDC(hDC);
			hBmpOld = SelectObject(hMemoryDC, cell_toaster.hBmpMask);
			j = BitBlt(offscrn.hDC,
				toasters[i].x,
				toasters[i].y,
				toasters[i].anime.pCellInfo->nCellW,
				toasters[i].anime.pCellInfo->nCellH,
				hMemoryDC,
				toasters[i].anime.nCelX,
				toasters[i].anime.nCelY,
				SRCAND);
			SelectObject(hMemoryDC, hBmpOld);

			hBmpOld = SelectObject(hMemoryDC, cell_toaster.hBitmap);
			j = BitBlt(offscrn.hDC,
				toasters[i].x,
				toasters[i].y,
				toasters[i].anime.pCellInfo->nCellW,
				toasters[i].anime.pCellInfo->nCellH,
				hMemoryDC,
				toasters[i].anime.nCelX,
				toasters[i].anime.nCelY,
				SRCPAINT);
			SelectObject(hMemoryDC, hBmpOld);

			DeleteDC(hMemoryDC);
		}
		BitBlt(hDC, 0, 0, offscrn.nWidth, offscrn.nHeight,
			offscrn.hDC, 0, 0, SRCCOPY);
		SelectPalette(hDC, hOldPal, FALSE);
		EndPaint(hWnd, &ps);
		break;
	case WM_MOUSEMOVE:
		if (execute_mode == EXEC_PREVIEW)
			break;
		if (LOWORD(lParam) != ptCurPos.x) {
			if (HIWORD(lParam) != ptCurPos.y) {
				GetCursorPos(&ptCurPos);
				SendMessage(hWnd, WM_CLOSE, 0, 0);
			}
		}
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		if (execute_mode == EXEC_PREVIEW)
			break;
		SendMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	case WM_ACTIVATE:
		//WM_ACTIVATEメッセージはウィンドウがアクティブか非アクティブになった時に送られる
		//このメッセージは最初に非アクティブになろうとしている
		//トップレベルのウィンドウのプロシージャに送られる
		//それからアクティブになったウィンドウのプロシージャに送られる
		if (LOWORD(wParam) == WA_INACTIVE) {		//プレビューの時アクティブでなくなったら終了
			if (execute_mode == EXEC_FULL_SCREEN)
				break;
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
	case WM_SYSCOMMAND:
		//WM_SYSCOMMAND SC_CLOSE or SC_SCREENSAVE 
		//スクリーンセーバーの再起動防止のため、このメッセージを return FALSE で処理する
		switch (wParam) {
		case SC_CLOSE:
			return FALSE;
		case SC_SCREENSAVE:
			return FALSE;
		}
		break;
	case WM_CLOSE:
		if (execute_mode == EXEC_PREVIEW) {
			DestroyWindow(hWnd);
			break;
		}
		if (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId) {
			DestroyWindow(hWnd);
			break;
			//WindowsNTはシステムがパスワードを管理しているので
			//以下のコードは必要ない
		}
		//レジストリ .Default\\Control Panel\\desktop の ScreenSaveUsePassword から取得します。 
		{
			DWORD Type = REG_DWORD;
			DWORD Size = sizeof(DWORD);
			HKEY hKeyResult = 0;
			int PasswordFlg = 0;

			inDialog = TRUE;
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\desktop", 0, KEY_READ, &hKeyResult)) {
				if (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId) {
#if 0
					BYTE	data[sizeof(DWORD)];
					//WindowsNT
					RegQueryValueEx(hKeyResult, "ScreenSaverIsSecure", NULL, &Type, data, &Size);
					RegCloseKey(hKeyResult);
					PasswordFlg = atoi(data);
#endif
				}
				else {
					//Windows95
					RegQueryValueEx(hKeyResult, "ScreenSaveUsePassword", 0, &Type, (LPBYTE)&PasswordFlg, &Size);
					RegCloseKey(hKeyResult);
				}
			}
			else {
				MessageBox(hWnd, "レジストリキーがなかった", "", MB_OK);
			}
			inDialog = FALSE;
			if (!PasswordFlg) {
				DestroyWindow(hWnd);		//"パスワードによる保護"をチェックしていないので終了
				break;
			}

			if (TRUE == VerifyPassword(hWnd))
				DestroyWindow(hWnd);		//
		}
		break;
	case WM_DESTROY:
		if (execute_mode = EXEC_FULL_SCREEN)
			if (VER_PLATFORM_WIN32_NT != OsVersionInfo.dwPlatformId)
				//ctrl-alt-tab,atl-tab防止設定を解除するWin95のみ
				SystemParametersInfo(SPI_SCREENSAVERRUNNING, 0, &oldval, 0);
		if (uTimer != 0)
			KillTimer(hWnd, uTimer);
		DeleteCellBmp(&cell_toaster);
		DeleteObject(hBmpOffScrn);
		DeleteOffscrn(&offscrn);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return (0L);
}

////////////////////////////////////////////////////////////////////////
// コマンドライン分解
//
void GetCmdLine(LPTSTR szCmdLine) {

	int i, j;

	for (i = 0; i < CmdOpMax; i++)
		argv[i] = "";

	i = j = 0;
	argc = 1;
	argv[0] = (char*)gszAppName;
	while (szCmdLine[0] != '\0') {
		if (szCmdLine[i] == ' ' || szCmdLine[i] == '\0') {
			argv[argc++] = szCmdLine + i - j;
			if (szCmdLine[i] == '\0') break;
			szCmdLine[i] = '\0';
			j = -1;
		}
		i++;
		j++;
	}
}
////////////////////////////////////////////////////////////////////////
// コマンドラインオプションチェック
//
BOOL MatchOption(LPTSTR lpsz, LPTSTR lpszOption) {

	lpsz[2] = '\0';						// WindowsNT対策  /c:??????? と入る為

	if (lpsz[0] == '-' || lpsz[0] == '/')
		lpsz++;
	if (lstrcmpi(lpsz, lpszOption) == 0)
		return TRUE;
	return FALSE;
}
/*----------------------------------------------------------------------------
	メイン関数
----------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	MSG			msg;
	WNDCLASS	wndclass;
	HWND		hwnd;
	RECT		rect;

	//	if( NULL != FindWindow( gszAppName, gszAppName ) )
	//		return (msg.wParam);		//２重起動チェック

	ghInstance = hInstance;
	glpszCmdParam = lpszCmdParam;

	GetCmdLine(lpszCmdParam);						// コマンドラインの解析

	execute_mode = 0;

	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_PARENTDC;
	wndclass.lpfnWndProc = (WNDPROC)WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = gszAppName;
#if 0
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = gszAppName;
#endif


	if (1 == argc || MatchOption(argv[1], "c")) {
		//
		// プロパティ設定
		//
		hwnd = GetForegroundWindow();
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, (DLGPROC)DialogProc);
		return FALSE;
	}
	else if (MatchOption(argv[1], "a")) {
		//
		// パスワードによる保護
		//
		HINSTANCE	hMprDll;
		hMprDll = LoadLibrary("MPR.DLL");
		if (hMprDll != NULL) {
			typedef VOID(WINAPI* PWDCHANGEPASSWORDA)(LPSTR, HWND, UINT, UINT);
			PWDCHANGEPASSWORDA PwdChangePasswordA = (PWDCHANGEPASSWORDA)GetProcAddress(hMprDll, "PwdChangePasswordA");
			if (PwdChangePasswordA != NULL)
				PwdChangePasswordA("SCRSAVE", GetForegroundWindow(), 0, 0);
		}
		FreeLibrary(hMprDll);
	}
	else if (MatchOption(argv[1], "s")) {
		//
		//	フルスクリーン起動
		//
		execute_mode = EXEC_FULL_SCREEN;
		if (!RegisterClass(&wndclass))
			return FALSE;
#if _DEBUG
		hWndMain = CreateWindowEx(0,	//WS_EX_DLGMODALFRAME,
			gszAppName, gszAppName,
			WS_POPUP | WS_VISIBLE,
			0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);
#else
		hWndMain = CreateWindowEx(WS_EX_TOPMOST,	//WS_EX_DLGMODALFRAME,
			gszAppName, gszAppName,
			WS_POPUP | WS_VISIBLE,
			0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);
#endif
		ShowWindow(hWndMain, SW_SHOW);
		UpdateWindow(hWndMain);
	}
	else if (MatchOption(argv[1], "p")) {
		//
		//	プレビュー起動
		//
		SECURITY_ATTRIBUTES SecurityAttr;
		SecurityAttr.nLength = sizeof(SecurityAttr);
		SecurityAttr.lpSecurityDescriptor = NULL;
		SecurityAttr.bInheritHandle = TRUE;
		if (0 != OpenMutex(MUTEX_ALL_ACCESS, FALSE, gszAppName)) {
			BringWindowToTop(hMutex);
			CloseHandle(hMutex);
			//MessageBox( NULL,"二重起動","",MB_OK|MB_ICONEXCLAMATION);
			return -1;
		}
		else
			hMutex = CreateMutex(&SecurityAttr, TRUE, gszAppName);
		execute_mode = EXEC_PREVIEW;
		hwnd = (HWND)atol(argv[2]);

		wndclass.lpfnWndProc = (WNDPROC)WndProc;
		if (!RegisterClass(&wndclass))
			return FALSE;

		GetClientRect(hwnd, &rect);
#if _DEBUG
		//		hWndMain = CreateWindowEx (WS_EX_TOPMOST,	//WS_EX_DLGMODALFRAME,
		//                			gszAppName, gszAppName,
		//                			WS_OVERLAPPEDWINDOW,
		//                			0,0,CW_USEDEFAULT,CW_USEDEFAULT,
		//							NULL, NULL, hInstance, NULL) ;
		hWndMain = CreateWindowEx(WS_EX_TOPMOST,	//WS_EX_DLGMODALFRAME,
			gszAppName, gszAppName,
			WS_CHILD | WS_VISIBLE,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
			hwnd, NULL, hInstance, NULL);
#else
		hWndMain = CreateWindowEx(WS_EX_TOPMOST,	//WS_EX_DLGMODALFRAME,
			gszAppName, gszAppName,
			WS_CHILD | WS_VISIBLE,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
			hwnd, NULL, hInstance, NULL);

#endif
		ShowWindow(hWndMain, SW_SHOW);
		UpdateWindow(hWndMain);
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (msg.wParam);
}
