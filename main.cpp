#include <iostream>
#include <Windows.h>
#include <commdlg.h> // color picker
#include <fstream>
#include <vector>

using namespace std;

// Shapes types
enum ShapeType {
    SHAPE_LINE,
    SHAPE_CIRCLE,
};

// Line algorithms
enum LineAlgorithm {
    DDA,
    BRESENHAM,
    PARAMETRIC,
};

// Shape struct
struct Shape {
    ShapeType type;
    int algorithm;

    union {
        struct { POINT start, end; COLORREF color; } Line;
    };
};

// Canvas info to save and load
std::vector<Shape> shapes;

// Menu Options IDs
#define ID_MENU_SAVE      101
#define ID_MENU_LOAD      102
#define ID_COLOR_PICKER   103
#define ID_CLEAR_SCREEN   104
#define ID_DRAW_DDA       105
#define ID_DRAW_BRESENHAM 106

// Global variables
COLORREF currentColor = RGB(0, 0, 0);
bool isDrawing = false;
POINT startPoint;
POINT endPoint;

// Function declarations
LRESULT WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp);
void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c);
void DrawLineBresenham(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c);

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

            HMENU hFileMenu = CreateMenu();
            AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFileMenu, "File");
            AppendMenu(hFileMenu, MF_STRING, ID_MENU_SAVE, "Save");
            AppendMenu(hFileMenu, MF_STRING, ID_MENU_LOAD, "Load");

            HMENU hToolMenu = CreateMenu();
            AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hToolMenu, "Tools");
            AppendMenu(hToolMenu, MF_STRING, ID_DRAW_DDA, "Draw DDA Line");
            AppendMenu(hToolMenu, MF_STRING, ID_DRAW_BRESENHAM, "Draw Bresenham Line");


            AppendMenu(hMenubar, MF_STRING, ID_COLOR_PICKER,  "Choose Color");
            AppendMenu(hMenubar, MF_STRING, ID_CLEAR_SCREEN,  "Clear Screen");

            SetMenu(hwnd, hMenubar);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_DRAW_DDA: {
                    toolSelected = ID_DRAW_DDA;
                    cout << "[Tool]   DDA Line Tool selected." << endl;
                    break;
                }

                case ID_DRAW_BRESENHAM: {
                    toolSelected = ID_DRAW_BRESENHAM;
                    cout << "[Tool]   Bresenham Line Tool selected." << endl;
                    break;
                }

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
                    OPENFILENAME ofn = { sizeof(ofn) };
                    char fileName[MAX_PATH] = "";
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = "Paint Files (*.dat)\0*.dat\0All Files\0*.*\0";
                    ofn.lpstrFile = fileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrDefExt = "pnt";
                    ofn.Flags = OFN_OVERWRITEPROMPT;

                    if (GetSaveFileName(&ofn)) {
                        FILE *f = fopen(fileName, "wb");
                        if (!f) {
                            MessageBox(hwnd, "Failed to open file for saving.", "Error", MB_OK | MB_ICONERROR);
                        }
                        ofstream out(fileName, ios::binary);
                        size_t count = shapes.size();
                        out.write((char*)&count, sizeof(count));
                        for (Shape& shape : shapes)
                            out.write((char*)&shape, sizeof(Shape));
                        out.close();
                        cout << "\n[Action] Drawing saved\n";
                    }
                    break;
                }

                case ID_MENU_LOAD: {OPENFILENAME ofn = { sizeof(ofn) };
                    char fileName[MAX_PATH] = "";
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = "Paint Files (*.dat)\0*.dat\0All Files\0*.*\0";
                    ofn.lpstrFile = fileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST;
                    if (GetOpenFileName(&ofn)) {
                        ifstream in(fileName, ios::binary);
                        if (in) {
                            shapes.clear();
                            size_t count;
                            in.read((char*)&count, sizeof(count));
                            for (size_t i = 0; i < count; ++i) {
                                Shape shape;
                                in.read((char*)&shape, sizeof(Shape));
                                shapes.push_back(shape);
                            }
                            InvalidateRect(hwnd, NULL, TRUE);
                        }
                        cout << "\n[Action] File loaded\n";
                    }
                    break;
                }

                case ID_CLEAR_SCREEN: {
                    shapes.clear();
                    InvalidateRect(hwnd, NULL, TRUE);
                    cout << "\n[Action] Screen cleared\n";
                    break;
                }

            }
            break;

        case WM_LBUTTONDOWN:
            if (toolSelected == ID_DRAW_DDA || toolSelected == ID_DRAW_BRESENHAM) {
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
                DrawLineBresenham(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                Shape newLine = {
                    .type = SHAPE_LINE,
                    .algorithm = DDA,
                    .Line = {
                        {startPoint.x, startPoint.y},
                        {endPoint.x, endPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newLine);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   DDA line drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }
            else if (toolSelected == ID_DRAW_BRESENHAM && isDrawing) {
                cout << toolSelected;
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);
                DrawLineBresenham(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                Shape newLine = {
                    .type = SHAPE_LINE,
                    .algorithm = BRESENHAM,
                    .Line = {
                        {startPoint.x, startPoint.y},
                        {endPoint.x, endPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newLine);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Bresenham line drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            for (const Shape& shape : shapes) {
                if (shape.type == SHAPE_LINE) {
                    if (shape.algorithm == DDA) {
                        DrawLineDDA(hdc, shape.Line.start.x, shape.Line.start.y,
                            shape.Line.end.x, shape.Line.end.y, shape.Line.color);
                    }
                    else if (shape.algorithm == BRESENHAM) {
                        DrawLineBresenham(hdc, shape.Line.start.x, shape.Line.start.y,
                            shape.Line.end.x, shape.Line.end.y, shape.Line.color);
                    }
                }
            }
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_CROSS));  // Change to cursor
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

void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c)
{
    int dx = x2 - x1, dy = y2 - y1;

    SetPixel(hdc, x1, y1, c);
    if (abs(dx) >= abs(dy)) {
        int x = x1, step = (dx > 0) ? 1 : -1;
        double y = y1, m = (double)dy / dx;
        m *= step;
        while (x != x2) {
            x += step;
            y += m;
            SetPixel(hdc, x, Round(y), c);
        }
    }
    else {
        int y = y1, step = (dy > 0) ? 1 : -1;
        double x = x1, m = (double)dx / dy;
        m *= step;
        while (y != y2) {
            y += step;
            x += m;
            SetPixel(hdc, Round(x), y, c);
        }
    }
}

void DrawLineBresenham(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x2 > x1) ? 1 : -1;
    int sy = (y2 > y1) ? 1 : -1;

    int x = x1, y = y1;

    SetPixel(hdc, x, y, c);

    if (dx > dy) {
        int d = 2 * dy - dx;
        int d1 = 2 * (dy - dx);
        int d2 = 2 * dy;

        while (x != x2) {
            x += sx;
            if (d < 0) {
                d += d2;
            } else {
                y += sy;
                d += d1;
            }
            SetPixel(hdc, x, y, c);
        }
    } else {
        int d = 2 * dx - dy;
        int d1 = 2 * (dx - dy);
        int d2 = 2 * dx;

        while (y != y2) {
            y += sy;
            if (d < 0) {
                d += d2;
            } else {
                x += sx;
                d += d1;
            }
            SetPixel(hdc, x, y, c);
        }
    }
}

