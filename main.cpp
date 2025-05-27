#include <iostream>
#include <Windows.h>
#include <commdlg.h> // color picker
#include <fstream>
#include <vector>

using namespace std;

struct DDALine {
    POINT start;
    POINT end;
    COLORREF color;
};

std::vector<DDALine> DDAlines;



#define ID_DRAW_DDA       101
#define ID_COLOR_PICKER   102
#define ID_CLEAR_SCREEN   103
#define ID_MENU_SAVE      104
#define ID_MENU_LOAD      105

// Global variables
COLORREF currentColor = RGB(0, 0, 0);
bool isDrawing = false;
POINT startPoint;
POINT endPoint;

// Function declarations
LRESULT WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp);
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
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

HWND hwndCanvas;

LRESULT WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp)
{
    static HWND btnDDA, btnClear, btnColor;
    static int toolSelected = 0;

    HDC hdc;
    switch (m)
    {
        case WM_CREATE: {
            HMENU hMenubar = CreateMenu();

            // Options menu
            HMENU hMenu = CreateMenu();
            AppendMenu(hMenu, MF_STRING, ID_DRAW_DDA,      "Draw DDA Line");
            AppendMenu(hMenu, MF_STRING, ID_COLOR_PICKER,  "Choose Color");
            AppendMenu(hMenu, MF_STRING, ID_CLEAR_SCREEN,  "Clear Screen");
            AppendMenu(hMenu, MF_STRING, ID_MENU_SAVE, "Save");
            AppendMenu(hMenu, MF_STRING, ID_MENU_LOAD, "Load");

            AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hMenu, "Options");
            SetMenu(hwnd, hMenubar);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_DRAW_DDA:
                    toolSelected = ID_DRAW_DDA;
                    cout << "[Tool]   DDA Line Tool selected." << endl;
                    break;

                case ID_COLOR_PICKER: {
                    // Color picker bta3t c++
                    CHOOSECOLOR cc = { sizeof(CHOOSECOLOR) };
                    static COLORREF acrCustClr[16];
                    cc.hwndOwner = hwnd;
                    cc.lpCustColors = acrCustClr;
                    cc.rgbResult = currentColor;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                    if (ChooseColor(&cc)) currentColor = cc.rgbResult;
                    cout << "[Action] Color changed: " << currentColor << endl;
                    break;
                }
                case ID_MENU_SAVE: {
                    ofstream out("shapes.dat", ios::binary);
                    size_t count = DDAlines.size();
                    out.write((char*)&count, sizeof(count));
                    for (DDALine& line : DDAlines)
                        out.write((char*)&line, sizeof(DDALine));
                    out.close();
                    cout << "\n[Action] Drawing saved\n";
                    break;
                }

                case ID_MENU_LOAD: {
                    ifstream in("shapes.dat", ios::binary);
                    if (in) {
                        DDAlines.clear();
                        size_t count;
                        in.read((char*)&count, sizeof(count));
                        for (size_t i = 0; i < count; ++i) {
                            DDALine line;
                            in.read((char*)&line, sizeof(DDALine));
                            DDAlines.push_back(line);
                        }
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    cout << "\n[Action] File loaded\n";
                    break;
                }
                case ID_CLEAR_SCREEN:
                    DDAlines.clear();
                    InvalidateRect(hwnd, NULL, TRUE);
                    cout << "\n[Action] Screen cleared\n";
                    break;
            }
            break;

        case WM_LBUTTONDOWN:
            if (toolSelected == ID_DRAW_DDA) {
                isDrawing = true;
                startPoint.x = LOWORD(lp);
                startPoint.y = HIWORD(lp);
            }

            break;

        case WM_LBUTTONUP:
            if (toolSelected == ID_DRAW_DDA && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);
                DrawLineDDA(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                DDALine newLine = { startPoint, endPoint, currentColor };
                DDAlines.push_back(newLine);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   DDA line drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }

            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            for (const DDALine& line : DDAlines) {
                DrawLineDDA(hdc, line.start.x, line.start.y, line.end.x, line.end.y, line.color);
            }
            EndPaint(hwnd, &ps);
            break;
        }
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

// dda line is missing inverted slope drawing handling
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