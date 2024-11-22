#include <windows.h>
#include <atlstr.h>
#include "resource.h"

#define backgroundColor 0x00000000
#define wallColor 0x00808080
#define playerColor 0x0000C000
#define targetColor 0x000000C0
#define shortestPathColor 0x00004080 //Dotted
#define playerPathColor 0x00606060 //Dotted
#define timerID 13666

int X=0,Y=0,cellSize=0,vCells=10;
double score=0;
int levelsDone=0;

TCHAR ClassName[] = TEXT("Labyrinth v1.x");
HWND hWnd;

typedef struct tagWordBytes {
	WORD h;
	WORD l;
} WORDBYTES;

HDC bufferDC;
HBITMAP bufferBitmap;
HBRUSH bgBrush;

BYTE* q;
WORDBYTES* stack;
WORDBYTES* shstack; //Shortest Path Stack
WORDBYTES* playerStack;
WORD sp=0;
WORD shsp=0;
int ps=0;
int state=0;
int surrenders=0;

int hr,vr,cls;
int seconds=0;
UINT_PTR tmr;

WORD seed;
WORD x=0,y=0;
WORD px=0, py=0;
WORD tx=0, ty=0;

bool newPuzzle = true;

void clearFill(WORD fx, WORD fy);
void drawPlayer();
void drawShortestWay();
void drawPlayerPath();

WORD random(WORD max) {
	WORD tmp = seed;
	WORD thash;
	tmp^=(tmp<<7);
	tmp^=(tmp>>1);
	tmp^=(tmp<<9);
	thash = (WORD)__rdtsc();
	seed = thash+tmp;
	return (tmp%max);
}

void RedrawText() {
	TEXTMETRIC tm;
	GetTextMetrics(bufferDC, &tm);
	RECT invlRect;invlRect.left=X*cellSize+16;invlRect.top=16;invlRect.bottom=19*tm.tmHeight;invlRect.right=hr-1;
	int txtx = invlRect.left;
	int txty = invlRect.top;
	FillRect(bufferDC, &invlRect, bgBrush);
	CString data;
	data.Format(TEXT("Moves required:"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("%d"), shsp-1);TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("Moves done:"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("%d"), ps);TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("Time passed:"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	if(seconds%60<10) {
		data.Format(TEXT("%d:0%d"), seconds/60, seconds%60);
	} else {
		data.Format(TEXT("%d:%d"), seconds/60, seconds%60);
	}
	TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("Number of cells:"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("%dx%d"), vCells, vCells);TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("White flags:"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("%d"),surrenders);TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("Levels Done:"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("%d"),levelsDone);TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("Scores:"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("%.3f"),score);TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	invlRect.left = txtx;invlRect.top = 16;invlRect.right = hr-1;invlRect.bottom = txty;
	RedrawWindow(hWnd, &invlRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void StaticText() {
	TEXTMETRIC tm;
	GetTextMetrics(bufferDC, &tm);
	RECT invlRect;invlRect.left=X*cellSize+16;invlRect.top=Y*cellSize-(15*tm.tmHeight);invlRect.right=hr-1;invlRect.bottom=vr-1;
	CString data;
	int txtx = invlRect.left;
	int txty = invlRect.top;
	data.Format(TEXT("ESCAPE = Exit game;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("F4 = Surrender;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("F6 = Previous level;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("F7 = Next Level;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("LEFT ARROW = Goes left if possible;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("UP ARROW = Goes up if possible;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("RIGHT ARROW = Goes right if possible;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("DOWN ARROW = Goes down if possible;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("Red square is your target;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("Green square is you;"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;txty+=tm.tmHeight;txty+=tm.tmHeight;
	data.Format(TEXT("L A B Y R I N T H"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	data.Format(TEXT("by Masterkiller"));TextOut(bufferDC, txtx, txty, data, data.GetLength());txty+=tm.tmHeight;
	RedrawWindow(hWnd, &invlRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void InitFonts() {
	HFONT cf = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, TEXT("Fixedsys"));
	SelectObject(bufferDC, cf);
	SetBkColor(bufferDC, backgroundColor);
	SetTextColor(bufferDC, 0x0080FF80);
}

void genLab() {
	memset(q, 0x0F, X*Y*sizeof(BYTE));
	BYTE nbh,nbhc;bool hasTarget=false;int hhsp=0;
	x = random(X);
	y = random(Y);
	px = x; py = y;
	while(true) {
		q[y*X+x]|=0x10;
		nbh=0;
		if(!((x==0)||(q[y*X+x-1]&0x10))) {
			nbh|=0x01;
		}
		if(!((y==0)||(q[(y-1)*X+x]&0x10))) {
			nbh|=0x02;
		}
		if(!((x==X-1)||(q[y*X+x+1]&0x10))) {
			nbh|=0x04;
		}
		if(!((y==Y-1)||(q[(y+1)*X+x]&0x10))) {
			nbh|=0x08;
		}
		if(nbh==0) {
			if(sp==0) {
				break;
			} else {
				if(hhsp<sp) {
					hhsp=sp;
					tx = x; ty = y;
				}
				sp--;
				y = stack[sp].h;
				x = stack[sp].l;
				continue;
			}
		}
		while((nbh&(nbhc=(1<<random(4))))==0);
		stack[sp].h=y;
		stack[sp].l=x;
		sp++;
		switch(nbhc) {
		case 1:
			q[y*X+x]&=0xFE;
			q[y*X+x-1]&=0xFB;
			x--;
			break;
		case 2:
			q[y*X+x]&=0xFD;
			q[(y-1)*X+x]&=0xF7;
			y--;
			break;
		case 4:
			q[y*X+x]&=0xFB;
			q[y*X+x+1]&=0xFE;
			x++;
			break;
		case 8:
			q[y*X+x]&=0xF7;
			q[(y+1)*X+x]&=0xFD;
			y++;
			break;
		}
	}
}

void getShortest() { //Call this after setting player and target coords
	for(int iy=0; iy<Y; iy++) {
		for(int ix=0; ix<X; ix++) {
			q[iy*X+ix]&=0x0F;
		}
	}
	BYTE nbh,nbhc;
	x=px; y=py;
	while(true) {
		q[y*X+x]|=0x10;
		if(x==tx&&y==ty) {
			shstack[shsp].h=y;
			shstack[shsp].l=x;
			shsp++;
			break;
		}
		nbh=0;
		if((q[y*X+x]&0x01)==0) {
			if((q[y*X+x-1]&0x10)==0) {
				nbh|=0x01;
			}
		}
		if((q[y*X+x]&0x02)==0) {
			if((q[(y-1)*X+x]&0x10)==0) {
				nbh|=0x02;
			}
		}
		if((q[y*X+x]&0x04)==0) {
			if((q[y*X+x+1]&0x10)==0) {
				nbh|=0x04;
			}
		}
		if((q[y*X+x]&0x08)==0) {
			if((q[(y+1)*X+x]&0x10)==0) {
				nbh|=0x08;
			}
		}
		if(nbh==0) {
			if(shsp==0) {
				break;
			} else {
				shsp--;
				y = shstack[shsp].h;
				x = shstack[shsp].l;
				continue;
			}
		}
		while((nbh&(nbhc=(1<<random(4))))==0);
		shstack[shsp].h=y;
		shstack[shsp].l=x;
		shsp++;
		switch(nbhc) {
		case 1:
			x--;
			break;
		case 2:
			y--;
			break;
		case 4:
			x++;
			break;
		case 8:
			y++;
			break;
		}
	}
}

void moveDone() {
	RedrawText();
	playerStack[ps].h=py;
	playerStack[ps].l=px;
	ps++;
	if(px==tx&&py==ty) {//Winner
		KillTimer(hWnd, tmr);
		drawPlayerPath();
		drawShortestWay();
		state=2;
		levelsDone++;
		score+=((((double)shsp/(double)ps)*60.0f)/((double)seconds/60.f));
		RedrawText();
		delete[] q;
		delete[] stack;
		delete[] shstack;
		if((vr/(vCells+1))>=5) {
			vCells++;
			cellSize=vr/vCells;
			Y = vr/cellSize;
			X = Y;
			q=new BYTE[X*Y];
			stack=new WORDBYTES[X*Y];
			shstack=new WORDBYTES[X*Y];
			for(int by=0; by<vr; by++) {
				for(int bx=0; bx<hr; bx++) {
					SetPixelV(bufferDC, bx, by, backgroundColor);
				}
			}
		}
	}
}

void drawLab() {
	int cx, cy;
	for(int wx=0; wx<X*cellSize; wx++) {
		for(int wy=0; wy<Y*cellSize; wy++) {
			SetPixelV(bufferDC, wx, wy, backgroundColor);
		}
	}
	for(x=0; x<X; x++) {
		for(y=0; y<Y; y++) {
			cx = x*cellSize;
			cy = y*cellSize;
			if(q[y*X+x]&0x01) {
				//LeftWall
				for(int i=0; i<cellSize; i++) {
					SetPixelV(bufferDC, cx, cy+i, wallColor);
				}
			}
			if(q[y*X+x]&0x02) {
				//TopWall
				for(int i=0; i<cellSize; i++) {
					SetPixelV(bufferDC, cx+i, cy, wallColor);
				}
			}
			if(q[y*X+x]&0x04) {
				//RightWall
				for(int i=0; i<cellSize; i++) {
					SetPixelV(bufferDC, cx+cellSize-1, cy+i, wallColor);
				}
			}
			if(q[y*X+x]&0x08) {
				//BottomWall
				for(int i=0; i<cellSize; i++) {
					SetPixelV(bufferDC, cx+i, cy+cellSize-1, wallColor);
				}
			}
		}
	}
}

void drawPlayerPath() {
	int cx, cy;
	for(int i=0; i<ps; i++) {
		cx = (playerStack[i].l)*cellSize;
		cy = (playerStack[i].h)*cellSize;
		for(int dy=1; dy<cellSize-1; dy++) {
			for(int dx=1; dx<cellSize-1; dx++) {
				if(dy%2) {
					if(dx%2) {
						SetPixel(bufferDC, cx+dx, cy+dy, playerPathColor);
					}
				} else {
					if((dx%2)==0) {
						SetPixel(bufferDC, cx+dx, cy+dy, playerPathColor);
					}
				}
			}
		}
	}
}

void drawShortestWay() {
	int cx, cy;
	clearFill(px, py);
	clearFill(tx, ty);
	for(int i=0; i<shsp; i++) {
		cx = (shstack[i].l)*cellSize;
		cy = (shstack[i].h)*cellSize;
		for(int dy=1; dy<cellSize-1; dy++) {
			for(int dx=1; dx<cellSize-1; dx++) {
				if(dy%2) {
					if(dx%2) {
						if(i==0) {
							SetPixel(bufferDC, cx+dx, cy+dy, playerColor);
						} else {
							if(i==shsp-1) {
								SetPixel(bufferDC, cx+dx, cy+dy, targetColor);
							} else {
								SetPixel(bufferDC, cx+dx, cy+dy, shortestPathColor);
							}
						}
					}
				} else {
					if((dx%2)==0) {
						if(i==0) {
							SetPixel(bufferDC, cx+dx, cy+dy, playerColor);
						} else {
							if(i==shsp-1) {
								SetPixel(bufferDC, cx+dx, cy+dy, targetColor);
							} else {
								SetPixel(bufferDC, cx+dx, cy+dy, shortestPathColor);
							}
						}
					}
				}
			}
		}
	}
	drawPlayer();
	RECT shInvl;shInvl.left=0;shInvl.top=0;shInvl.right=X*cellSize;shInvl.bottom=Y*cellSize;
	RedrawWindow(hWnd, &shInvl, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void drawPlayer() {
	int cx, cy;
	cx = px*cellSize;
	cy = py*cellSize;
	for(int iy=cy+2; iy<cy+cellSize-2; iy++) {
		for(int ix=cx+2; ix<cx+cellSize-2; ix++) {
			SetPixelV(bufferDC, ix, iy, playerColor);
		}
	}
	RECT regn; regn.left = cx+1; regn.top = cy+1; regn.right = cx+cellSize-1; regn.bottom = cy+cellSize-1;
	RedrawWindow(hWnd, &regn, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void drawTarget() {
	int cx, cy;
	cx = tx*cellSize;
	cy = ty*cellSize;
	for(int iy=cy+2; iy<cy+cellSize-2; iy++) {
		for(int ix=cx+2; ix<cx+cellSize-2; ix++) {
			SetPixelV(bufferDC, ix, iy, targetColor);
		}
	}
	RECT regn; regn.left = cx+1; regn.top = cy+1; regn.right = cx+cellSize-1; regn.bottom = cy+cellSize-1;
	RedrawWindow(hWnd, &regn, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void clearFill(WORD fx, WORD fy) {
	int cx, cy;
	cx = fx*cellSize;
	cy = fy*cellSize;
	for(int iy=cy+1; iy<cy+cellSize-1; iy++) {
		for(int ix=cx+1; ix<cx+cellSize-1; ix++) {
			SetPixelV(bufferDC, ix, iy, backgroundColor);
		}
	}
	RECT regn; regn.left = cx+1; regn.top = cy+1; regn.right = cx+cellSize-1; regn.bottom = cy+cellSize-1;
	RedrawWindow(hWnd, &regn, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void rePaint(HWND hWnd) {
	PAINTSTRUCT ps;
	HDC drawDC = BeginPaint(hWnd, &ps);
	BitBlt(drawDC, 0, 0, hr, vr, bufferDC, 0, 0, SRCCOPY);
	EndPaint(hWnd, &ps);
}

void keyHandler(WPARAM vkKey) {
	switch(state) {
	case 1:
		switch(vkKey) {
		case VK_LEFT:
			if((q[py*X+px]&0x01)==0) {
				clearFill(px, py);
				px--;
				drawPlayer();
				moveDone();
			}
			break;
		case VK_UP:
			if((q[py*X+px]&0x02)==0) {
				clearFill(px, py);
				py--;
				drawPlayer();
				moveDone();
			}
			break;
		case VK_RIGHT:
			if((q[py*X+px]&0x04)==0) {
				clearFill(px, py);
				px++;
				drawPlayer();
				moveDone();
			}
			break;
		case VK_DOWN:
			if((q[py*X+px]&0x08)==0) {
				clearFill(px, py);
				py++;
				drawPlayer();
				moveDone();
			}
			break;
		case VK_F4:
			surrenders++;
			KillTimer(hWnd, tmr);
			drawShortestWay();
			state=2;
			break;
		case VK_F6:
			if(vCells>10) {
				vCells--;
			}
			surrenders++;
			KillTimer(hWnd, tmr);
			drawShortestWay();
			cellSize=vr/vCells;
			Y = vr/cellSize;
			X = Y;
			delete[] q;
			delete[] stack;
			delete[] shstack;
			q=new BYTE[X*Y];
			stack=new WORDBYTES[X*Y];
			shstack=new WORDBYTES[X*Y];
			for(int by=0; by<vr; by++) {
				for(int bx=0; bx<hr; bx++) {
					SetPixelV(bufferDC, bx, by, backgroundColor);
				}
			}
			state=2;
			break;
		case VK_F7:
			if(vr/(vCells+1)>=5) {
				vCells++;
			}
			surrenders++;
			KillTimer(hWnd, tmr);
			drawShortestWay();
			cellSize=vr/vCells;
			Y = vr/cellSize;
			X = Y;
			delete[] q;
			delete[] stack;
			delete[] shstack;
			q=new BYTE[X*Y];
			stack=new WORDBYTES[X*Y];
			shstack=new WORDBYTES[X*Y];
			for(int by=0; by<vr; by++) {
				for(int bx=0; bx<hr; bx++) {
					SetPixelV(bufferDC, bx, by, backgroundColor);
				}
			}
			state=2;
			break;
		case VK_ESCAPE:
			newPuzzle=false;
			PostQuitMessage(0);
			break;
		}
		break;
	case 2:
		switch(vkKey) {
		case VK_ESCAPE:
			newPuzzle=false;
			PostQuitMessage(0);
			break;
		default:
			state=1;
			PostQuitMessage(0);
			break;
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch(Msg) {
	case WM_DESTROY:
	case WM_CLOSE:
	case WM_QUERYENDSESSION:
		newPuzzle = false;
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		rePaint(hWnd);
		break;
	case WM_KEYDOWN:
		keyHandler(wParam);
		break;
	case WM_TIMER:
		if(wParam==timerID) {
			seconds++;
			RedrawText();
		}
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	seed = (WORD)__rdtsc();
	bgBrush = CreateSolidBrush(backgroundColor);
	WNDCLASS wc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = bgBrush;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = ClassName;
	wc.lpszMenuName = NULL;
	wc.style = CS_OWNDC;
	if(!RegisterClass(&wc)) {
		return 0;
	}
	HDC ScreenDC = CreateDC(TEXT("DISPLAY"), 0, 0, 0);
	hr=GetDeviceCaps(ScreenDC, HORZRES);
	vr=GetDeviceCaps(ScreenDC, VERTRES);
	cls=GetDeviceCaps(ScreenDC, BITSPIXEL);
	DeleteDC(ScreenDC);
	DEVMODE nds;
	memset(&nds, 0, sizeof(DEVMODE));
	nds.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL;
	nds.dmPelsWidth = hr;
	nds.dmPelsHeight = vr;
	nds.dmBitsPerPel = cls;
	ChangeDisplaySettings(&nds, CDS_FULLSCREEN);
	cellSize=vr/10;
	Y = vr/cellSize;
	X=Y;
	q = new BYTE[X*Y];
	stack = new WORDBYTES[X*Y];
	shstack = new WORDBYTES[X*Y];
	playerStack = new WORDBYTES[1048576];
	RECT wndRect;wndRect.top = 0;wndRect.left = 0;wndRect.right = hr;wndRect.bottom = vr;
	DWORD es=WS_EX_APPWINDOW,s=WS_POPUP;
	AdjustWindowRectEx(&wndRect, s, FALSE, es);
	if(!(hWnd=CreateWindowEx(es, ClassName, TEXT("Labyrinth"), s, hr/2-(wndRect.right-wndRect.left)/2, vr/2-(wndRect.bottom-wndRect.top)/2, wndRect.right-wndRect.left, wndRect.bottom-wndRect.top, NULL, NULL, hInstance, NULL))) {
		return 0;
	}
	HDC winDC = GetDC(hWnd);
	bufferDC = CreateCompatibleDC(winDC);
	ReleaseDC(hWnd, winDC);
	InitFonts();
	bufferBitmap = CreateBitmap(hr, vr, 1, cls, NULL);
	SelectObject(bufferDC, bufferBitmap);
	ShowWindow(hWnd, 1);
	SetForegroundWindow(hWnd);
	MSG msg;
	while(newPuzzle) {
		StaticText();
		tmr = SetTimer(hWnd, timerID, 1000, NULL);
		ps=0;
		shsp=0;
		seconds=0;
		genLab();
		getShortest();
		drawLab();
		drawPlayer();
		drawTarget();
		state=1;
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
		RedrawText();
		while(GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	ChangeDisplaySettings(NULL, 0);
	return 0;
}