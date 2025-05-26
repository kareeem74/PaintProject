#include <Windows.h>

using namespace std;

LRESULT WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp);

// Helper Function
int Round(double x);
void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c);



int APIENTRY WinMain(HINSTANCE hi, HINSTANCE pi, LPSTR cmd, int nsh)
{
    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.lpszClassName = "PaintClass";
    wc.lpszMenuName = NULL;
    wc.lpfnWndProc = WndProc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = hi;
    RegisterClass(&wc);
    HWND hwnd = CreateWindow("PaintCLass", "Paint", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hi, 0);
    ShowWindow(hwnd, nsh);
    UpdateWindow(hwnd);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}


LRESULT WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp)
{
    HDC hdc;
    switch (m)
    {
        case WM_LBUTTONDOWN:
            hdc = GetDC(hwnd);
        ReleaseDC(hwnd, hdc);
        break;
        case WM_LBUTTONUP:
            hdc = GetDC(hwnd);
        ReleaseDC(hwnd, hdc);
        break;
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_CROSS));  // Change to crosshair
        return TRUE;
        case WM_CLOSE:
            DestroyWindow(hwnd); break;
        case WM_DESTROY:
            PostQuitMessage(0); break;
        default:return DefWindowProc(hwnd, m, wp, lp);
    }
    return 0;
}


int Round(double x)
{
    return (int)(x + 0.5);
}


void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c)
{
    int dx = x2 - x1, dy = y2 - y1;
    SetPixel(hdc, x1, y1, c);
    if (abs(dx) >= abs(dy))
    {
        double m = (double)dy / dx;
        int x = x1;
        double y = y1;
        while (x < x2)
        {
            x++;
            y += m;
            SetPixel(hdc, x, Round(y), c);
        }
    }
    else {
        double mi = (double)dx / dy;
        int y = y1;
        double x = x1;
        while (y < y2)
        {
            y++;
            x += mi;
            SetPixel(hdc, Round(x),y, c);
        }

    }
}