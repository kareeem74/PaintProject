#include <algorithm>
#include <iostream>
#include <math.h>
#include <Windows.h>
#include <commdlg.h> // color picker
#include <complex.h>
#include <fstream>
#include <vector>
#include <stack>
#include <list>

using namespace std;

// TODO:
// [*] Change the background of window to be white
// [*] Try to change the shape of your window mouse
// [*] User must interact with window using mouse only
// [*] Try to make combination between your console and window
// [*] Give users the option to choose shape color before drawing from menu
// [*] Implement item to clear screen from shapes
// [*] Implement save function for all data in screen
// [*] Implement load function to load data from files
// [*] Implement line algorithms [DDA, Midpoint and parametric]
// [*] Implement Circle algorithms [Direct, Polar, iterative Polar, midpoint and modified Midpoint]
// [ ] Filling Circle with lines after taking filling quarter from user
// [ ] Filling Circle with other circles after taking filling quarter from user
// [ ] Filling Square with Hermit Curve [Vertical]
// [ ] Filling Rectangle with Bezier Curve [horizontal]
// [*] Convex and Non-Convex Filling Algorithm
// [*] Recursive and Non-Recursive Flood Fill
// [ ] Cardinal Spline Curve
// [*] Ellipse Algorithms [Direct, polar and midpoint]
// [ ] Clipping algorithms using Rectangle as Clipping Window to clip [Point, Line, Polygon]
// [ ] Clipping algorithms using Square as Clipping Window to clip [Point, Line]
// BONUS
// [ ] Clipping algorithms using circle as a Clipping Window to clip [Point, Line]


// Shapes types
enum ShapeType {
    SHAPE_LINE,
    SHAPE_CIRCLE,
    SHAPE_ELLIPSE,
    SHAPE_FILL,
};

// algorithms
enum Algorithm {
    DIRECT,
    DDA,
    MIDPOINT,
    MODIFIED_MIDPOINT,
    PARAMETRIC,
    POLAR,
    ITERATIVE_POLAR,
    STACK,
    RECURSIVE,
    CONVEX,
    GENERAL,
};


// Shape struct
struct Shape {
    ShapeType type;
    int algorithm;

    union {
        struct { POINT start, end; COLORREF color; } Line;
        struct { POINT c; int radius; COLORREF color; } Circle;
        struct { POINT c, r; COLORREF color; } Ellipse;
        struct { POINT p; COLORREF color; } Fill;
        struct { POINT p[10]; int points; COLORREF color; } ScanLineFill;
    };
};

typedef POINT Point;


// Canvas info to save and load
std::vector<Shape> shapes;


// Menu Options IDs
#define ID_MENU_SAVE                        101
#define ID_MENU_LOAD                        102
#define ID_COLOR_PICKER                     103
#define ID_CLEAR_SCREEN                     104
#define ID_DRAW_DDA_LINE                    105
#define ID_DRAW_MIDPOINT_LINE               106
#define ID_DRAW_PARAMETRIC_LINE             107
#define ID_DRAW_DIRECT_CIRCLE               108
#define ID_DRAW_MIDPOINT_CIRCLE             109
#define ID_DRAW_MODIFIED_MIDPOINT_CIRCLE    110
#define ID_DRAW_POLAR_CIRCLE                111
#define ID_DRAW_ITERATIVE_POLAR_CIRCLE      112
#define ID_DRAW_DIRECT_ELLIPSE              114
#define ID_DRAW_MIDPOINT_ELLIPSE            115
#define ID_DRAW_POLAR_ELLIPSE               116
#define ID_DRAW_FILL_STACK                  117
#define ID_DRAW_FILL_RECURSIVE              118
#define ID_DRAW_CONVEX_FILL                 119
#define ID_DRAW_GENERAL_FILL                120

// Global variables
COLORREF currentColor = RGB(0, 0, 0);
bool isDrawing = false;
POINT startPoint;
POINT endPoint;
vector <POINT> controlPoints;
int scanLineSize = 0;

// Function declarations
LRESULT WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp);
void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c);
void DrawLineMidpoint(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c);
void DrawLineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color);
void DrawCircleDirect(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleModMidpoint(HDC hdc,int xc,int yc, int r,COLORREF color);
void DrawCirclePolar(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleItPolar(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawEllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void FloodFillStack(HDC hdc, int x, int y, COLORREF fillColor);
void FloodFillRec(HDC hdc,int x,int y,COLORREF fillColor);
void ConvexFill(HDC hdc,const POINT* p,int n,COLORREF color);
void GeneralFill(HDC hdc,const POINT* p,int n,COLORREF color);
POINT* ShapeDrawer(HDC hdc, COLORREF c);

int APIENTRY WinMain(HINSTANCE hi, HINSTANCE pi, LPSTR cmd, int nsh)
{
    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
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

            HMENU hLineMenu = CreateMenu();
            AppendMenu(hLineMenu, MF_STRING, ID_DRAW_DDA_LINE, "DDA");
            AppendMenu(hLineMenu, MF_STRING, ID_DRAW_MIDPOINT_LINE, "Midpoint");
            AppendMenu(hLineMenu, MF_STRING, ID_DRAW_PARAMETRIC_LINE, "Parametric");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hLineMenu, "Line");

            HMENU hCircleMenu = CreateMenu();
            AppendMenu(hCircleMenu, MF_STRING, ID_DRAW_DIRECT_CIRCLE, "Direct");
            AppendMenu(hCircleMenu, MF_STRING, ID_DRAW_MIDPOINT_CIRCLE, "Midpoint");
            AppendMenu(hCircleMenu, MF_STRING, ID_DRAW_MODIFIED_MIDPOINT_CIRCLE, "Modified Midpoint");
            AppendMenu(hCircleMenu, MF_STRING, ID_DRAW_POLAR_CIRCLE, "Polar");
            AppendMenu(hCircleMenu, MF_STRING, ID_DRAW_ITERATIVE_POLAR_CIRCLE, "Iterative Polar");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hCircleMenu, "Circle");

            HMENU hEllipseMenu = CreateMenu();
            AppendMenu(hEllipseMenu, MF_STRING, ID_DRAW_DIRECT_ELLIPSE, "Direct");
            AppendMenu(hEllipseMenu, MF_STRING, ID_DRAW_MIDPOINT_ELLIPSE, "Midpoint");
            AppendMenu(hEllipseMenu, MF_STRING, ID_DRAW_POLAR_ELLIPSE, "Polar");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hEllipseMenu, "Ellipse");

            HMENU hFillMenu = CreateMenu();
            AppendMenu(hFillMenu, MF_STRING, ID_DRAW_FILL_RECURSIVE, "Recursive");
            AppendMenu(hFillMenu, MF_STRING, ID_DRAW_FILL_STACK, "Using Stack");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hFillMenu, "Fill");

            HMENU hScanLineFillMenu = CreateMenu();
            AppendMenu(hScanLineFillMenu, MF_STRING, ID_DRAW_CONVEX_FILL, "Convex");
            AppendMenu(hScanLineFillMenu, MF_STRING, ID_DRAW_GENERAL_FILL, "General");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hScanLineFillMenu, "Scan-Line Fill");


            AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hToolMenu, "Tools");


            AppendMenu(hMenubar, MF_STRING, ID_COLOR_PICKER,  "Choose Color");

            AppendMenu(hMenubar, MF_STRING, ID_CLEAR_SCREEN,  "Clear Screen");

            SetMenu(hwnd, hMenubar);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_DRAW_DDA_LINE: {
                    toolSelected = ID_DRAW_DDA_LINE;
                    cout << "[Tool]   DDA Line Tool selected." << endl;
                    break;
                }

                case ID_DRAW_MIDPOINT_LINE: {
                    toolSelected = ID_DRAW_MIDPOINT_LINE;
                    cout << "[Tool]   Midpoint Line Tool selected." << endl;
                    break;
                }

                case ID_DRAW_PARAMETRIC_LINE: {
                    toolSelected = ID_DRAW_PARAMETRIC_LINE;
                    cout << "[Tool]   Parametric Line Tool selected." << endl;
                    break;
                }

                case ID_DRAW_DIRECT_CIRCLE: {
                    toolSelected = ID_DRAW_DIRECT_CIRCLE;
                    cout << "[Tool]   Direct Circle Tool selected." << endl;
                    break;
                }

                case ID_DRAW_MIDPOINT_CIRCLE: {
                    toolSelected = ID_DRAW_MIDPOINT_CIRCLE;
                    cout << "[Tool]   Midpoint Circle Tool selected." << endl;
                    break;
                }

                case ID_DRAW_MODIFIED_MIDPOINT_CIRCLE: {
                    toolSelected = ID_DRAW_MODIFIED_MIDPOINT_CIRCLE;
                    cout << "[Tool]   Modified Midpoint Circle Tool selected." << endl;
                    break;
                }

                case ID_DRAW_POLAR_CIRCLE: {
                    toolSelected = ID_DRAW_POLAR_CIRCLE;
                    cout << "[Tool]   Polar Circle Tool selected." << endl;
                    break;
                }

                case ID_DRAW_ITERATIVE_POLAR_CIRCLE: {
                    toolSelected = ID_DRAW_ITERATIVE_POLAR_CIRCLE;
                    cout << "[Tool]   Iterative Polar Circle Tool selected." << endl;
                    break;
                }

                case ID_DRAW_DIRECT_ELLIPSE: {
                    toolSelected = ID_DRAW_DIRECT_ELLIPSE;
                    cout << "[Tool]   Direct Ellipse Tool selected." << endl;
                    break;
                }

                case ID_DRAW_MIDPOINT_ELLIPSE: {
                    toolSelected = ID_DRAW_MIDPOINT_ELLIPSE;
                    cout << "[Tool]   Midpoint Ellipse Tool selected." << endl;
                    break;
                }

                case ID_DRAW_POLAR_ELLIPSE: {
                    toolSelected = ID_DRAW_POLAR_ELLIPSE;
                    cout << "[Tool]   Polar Ellipse Tool selected." << endl;
                    break;
                }

                case ID_DRAW_FILL_STACK: {
                    toolSelected = ID_DRAW_FILL_STACK;
                    cout << "[Tool]   Fill using stack Tool selected." << endl;
                    break;
                }

                case ID_DRAW_FILL_RECURSIVE: {
                    toolSelected = ID_DRAW_FILL_RECURSIVE;
                    cout << "[Tool]   Fill using recursive Tool selected." << endl;
                    break;
                }

                case ID_DRAW_CONVEX_FILL: {
                    toolSelected = ID_DRAW_CONVEX_FILL;
                    cout << "[Tool]   Convex Fill Tool selected." << endl;
                    cout << "Enter the number of Points: ";
                    cin >> scanLineSize;
                    break;
                }

                case ID_DRAW_GENERAL_FILL: {
                    toolSelected = ID_DRAW_GENERAL_FILL;
                    cout << "[Tool]   General Fill Tool selected." << endl;
                    cout << "Enter the number of Points: ";
                    cin >> scanLineSize;
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
                        cout << "[Action] Drawing saved";
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
                        cout << "[Action] File loaded";
                    }
                    break;
                }

                case ID_CLEAR_SCREEN: {
                    shapes.clear();
                    InvalidateRect(hwnd, NULL, TRUE);
                    cout << "[Action] Screen cleared";
                    break;
                }

            }
            break;

        case WM_LBUTTONDOWN:
            if (toolSelected == ID_DRAW_CONVEX_FILL) {
                int x = LOWORD(lp);
                int y = HIWORD(lp);
                hdc = GetDC(hwnd);
                SetPixel(hdc, x, y, currentColor);
                controlPoints.push_back(Point(x, y));

                if (controlPoints.size() == scanLineSize) {
                    ConvexFill(hdc, &controlPoints[0], controlPoints.size(), currentColor);
                    Shape newFill{};                         // zero-initialise everything
                    newFill.type      = SHAPE_FILL;
                    newFill.algorithm = CONVEX;

                    /* copy up to p[10] -- avoid overflow! */
                    newFill.ScanLineFill.points =
                        min<size_t>(controlPoints.size(), 10);

                    copy_n(controlPoints.begin(),
                                newFill.ScanLineFill.points,
                                newFill.ScanLineFill.p);

                    newFill.ScanLineFill.color = currentColor;
                    shapes.push_back(newFill);
                    controlPoints.clear();
                }
            }
            else if (toolSelected == ID_DRAW_GENERAL_FILL) {
                int x = LOWORD(lp);
                int y = HIWORD(lp);
                hdc = GetDC(hwnd);
                SetPixel(hdc, x, y, currentColor);
                controlPoints.push_back(Point(x, y));

                if (controlPoints.size() == scanLineSize) {
                    GeneralFill(hdc, &controlPoints[0], controlPoints.size(), currentColor);
                    Shape newFill{};                         // zero-initialise everything
                    newFill.type      = SHAPE_FILL;
                    newFill.algorithm = GENERAL;

                    /* copy up to p[10] -- avoid overflow! */
                    newFill.ScanLineFill.points =
                        min<size_t>(controlPoints.size(), 10);

                    copy_n(controlPoints.begin(),
                                newFill.ScanLineFill.points,
                                newFill.ScanLineFill.p);

                    newFill.ScanLineFill.color = currentColor;
                    shapes.push_back(newFill);
                    controlPoints.clear();
                }
            }
            else if (toolSelected) {
                isDrawing = true;
                startPoint.x = LOWORD(lp);
                startPoint.y = HIWORD(lp);
            }
            break;

        case WM_LBUTTONUP:
            if (toolSelected == ID_DRAW_DDA_LINE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);
                DrawLineMidpoint(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
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
            else if (toolSelected == ID_DRAW_MIDPOINT_LINE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);
                DrawLineMidpoint(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                Shape newLine = {
                    .type = SHAPE_LINE,
                    .algorithm = MIDPOINT,
                    .Line = {
                        {startPoint.x, startPoint.y},
                        {endPoint.x, endPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newLine);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Midpoint (Bresenham) line drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }
            else if (toolSelected == ID_DRAW_PARAMETRIC_LINE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);
                DrawLineParametric(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                Shape newLine = {
                    .type = SHAPE_LINE,
                    .algorithm = MIDPOINT,
                    .Line = {
                        {startPoint.x, startPoint.y},
                        {endPoint.x, endPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newLine);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Parametric line drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }
            else if (toolSelected == ID_DRAW_DIRECT_CIRCLE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                int radius = sqrt(pow(endPoint.x - startPoint.x, 2) + pow(endPoint.y - startPoint.y, 2));
                DrawCircleDirect(hdc, startPoint.x, startPoint.y, radius, currentColor);
                Shape newCircle = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = MIDPOINT,
                    .Circle = {
                        {startPoint.x, startPoint.y},
                        radius,
                        currentColor
                    }
                };
                shapes.push_back(newCircle);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Direct Circle drawn, center (" << startPoint.x << "," << startPoint.y
                << ") radius: " << radius << "." << endl;
            }
            else if (toolSelected == ID_DRAW_MIDPOINT_CIRCLE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                int radius = sqrt(pow(endPoint.x - startPoint.x, 2) + pow(endPoint.y - startPoint.y, 2));
                DrawCircleMidpoint(hdc, startPoint.x, startPoint.y, radius, currentColor);
                Shape newCircle = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = MIDPOINT,
                    .Circle = {
                        {startPoint.x, startPoint.y},
                        radius,
                        currentColor
                    }
                };
                shapes.push_back(newCircle);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Midpoint (Bresenham) circle drawn, center (" << startPoint.x << "," << startPoint.y
                << ") radius: " << radius << "." << endl;
            }
            else if (toolSelected == ID_DRAW_MODIFIED_MIDPOINT_CIRCLE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                int radius = sqrt(pow(endPoint.x - startPoint.x, 2) + pow(endPoint.y - startPoint.y, 2));
                DrawCircleModMidpoint(hdc, startPoint.x, startPoint.y, radius, currentColor);
                Shape newCircle = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = MIDPOINT,
                    .Circle = {
                        {startPoint.x, startPoint.y},
                        radius,
                        currentColor
                    }
                };
                shapes.push_back(newCircle);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Modified Midpoint (Bresenham) circle drawn, center (" << startPoint.x << "," << startPoint.y
                << ") radius: " << radius << "." << endl;
            }
            else if (toolSelected == ID_DRAW_POLAR_CIRCLE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                int radius = sqrt(pow(endPoint.x - startPoint.x, 2) + pow(endPoint.y - startPoint.y, 2));
                DrawCirclePolar(hdc, startPoint.x, startPoint.y, radius, currentColor);
                Shape newCircle = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = POLAR,
                    .Circle = {
                        {startPoint.x, startPoint.y},
                        radius,
                        currentColor
                    }
                };
                shapes.push_back(newCircle);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Polar circle drawn, center (" << startPoint.x << "," << startPoint.y
                << ") radius: " << radius << "." << endl;
            }
            else if (toolSelected == ID_DRAW_ITERATIVE_POLAR_CIRCLE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                int radius = sqrt(pow(endPoint.x - startPoint.x, 2) + pow(endPoint.y - startPoint.y, 2));
                DrawCircleItPolar(hdc, startPoint.x, startPoint.y, radius, currentColor);
                Shape newCircle = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = POLAR,
                    .Circle = {
                        {startPoint.x, startPoint.y},
                        radius,
                        currentColor
                    }
                };
                shapes.push_back(newCircle);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Iterative Polar circle drawn, center (" << startPoint.x << "," << startPoint.y
                << ") radius: " << radius << "." << endl;
            }
            else if (toolSelected == ID_DRAW_DIRECT_ELLIPSE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                DrawEllipseDirect(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                Shape newEllipse = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = POLAR,
                    .Ellipse = {
                        {startPoint.x, startPoint.y},
                        {endPoint.x, endPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newEllipse);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Parametric Ellipse drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }
            else if (toolSelected == ID_DRAW_MIDPOINT_ELLIPSE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                DrawEllipseMidpoint(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                Shape newEllipse = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = MIDPOINT,
                    .Ellipse = {
                        {startPoint.x, startPoint.y},
                        {endPoint.x, endPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newEllipse);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Midpoint Ellipse drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }
            else if (toolSelected == ID_DRAW_POLAR_ELLIPSE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                DrawEllipsePolar(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
                Shape newEllipse = {
                    .type = SHAPE_CIRCLE,
                    .algorithm = POLAR,
                    .Ellipse = {
                        {startPoint.x, startPoint.y},
                        {endPoint.x, endPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newEllipse);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   Polar Ellipse drawn from the point (" << startPoint.x << "," << startPoint.y << ") to ("  <<
                    endPoint.x << "," << endPoint.y << ")" << endl;
            }
            else if (toolSelected == ID_DRAW_FILL_RECURSIVE && isDrawing) {

                HDC hdc = GetDC(hwnd);

                FloodFillRec(hdc, startPoint.x, startPoint.y, currentColor);
                Shape newFill = {
                    .type = SHAPE_FILL,
                    .algorithm = STACK,
                    .Fill = {
                        {startPoint.x, startPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newFill);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   The Shape filled recursively from the point (" << startPoint.x << "," << startPoint.y << ")"
                << "." << endl;
            }
            else if (toolSelected == ID_DRAW_FILL_STACK && isDrawing) {

                HDC hdc = GetDC(hwnd);

                FloodFillStack(hdc, startPoint.x, startPoint.y, currentColor);
                Shape newFill = {
                    .type = SHAPE_FILL,
                    .algorithm = STACK,
                    .Fill = {
                        {startPoint.x, startPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newFill);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   The Shape filled iteratively from the point (" << startPoint.x << "," << startPoint.y << ")"
                << "." << endl;
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
                    else if (shape.algorithm == MIDPOINT) {
                        DrawLineMidpoint(hdc, shape.Line.start.x, shape.Line.start.y,
                            shape.Line.end.x, shape.Line.end.y, shape.Line.color);
                    }
                    else if (shape.algorithm == PARAMETRIC) {
                        DrawLineParametric(hdc, shape.Line.start.x, shape.Line.start.y,
                            shape.Line.end.x, shape.Line.end.y, shape.Line.color);
                    }
                }
                else if (shape.type == SHAPE_CIRCLE) {
                    if (shape.algorithm == DIRECT) {
                        DrawCircleDirect(hdc, shape.Circle.c.x, shape.Circle.c.y,
                            shape.Circle.radius, shape.Line.color);
                    }
                    else if (shape.algorithm == MIDPOINT) {
                        DrawCircleMidpoint(hdc, shape.Circle.c.x, shape.Circle.c.y,
                            shape.Circle.radius, shape.Line.color);
                    }
                    else if (shape.algorithm == MODIFIED_MIDPOINT) {
                        DrawCircleModMidpoint(hdc, shape.Circle.c.x, shape.Circle.c.y,
                            shape.Circle.radius, shape.Line.color);
                    }
                    else if (shape.algorithm == POLAR) {
                        DrawCirclePolar(hdc, shape.Circle.c.x, shape.Circle.c.y,
                            shape.Circle.radius, shape.Line.color);
                    }
                    else if (shape.algorithm == ITERATIVE_POLAR) {
                        DrawCircleItPolar(hdc, shape.Circle.c.x, shape.Circle.c.y,
                            shape.Circle.radius, shape.Line.color);
                    }
                }
                else if (shape.type == SHAPE_ELLIPSE) {
                    if (shape.algorithm == DIRECT) {
                        DrawEllipseDirect(hdc, shape.Ellipse.c.x, shape.Ellipse.c.y,
                            shape.Ellipse.r.x, shape.Ellipse.r.y, shape.Line.color);
                    }
                    else if (shape.algorithm == MIDPOINT) {
                        DrawEllipseMidpoint(hdc, shape.Ellipse.c.x, shape.Ellipse.c.y,
                            shape.Ellipse.r.x, shape.Ellipse.r.y, shape.Line.color);
                    }
                    else if (shape.algorithm == POLAR) {
                        DrawEllipsePolar(hdc, shape.Ellipse.c.x, shape.Ellipse.c.y,
                            shape.Ellipse.r.x, shape.Ellipse.r.y, shape.Line.color);
                    }
                }
                else if (shape.type == SHAPE_FILL) {
                    if (shape.algorithm == STACK) {
                        FloodFillStack(hdc, shape.Fill.p.x, shape.Fill.p.y, shape.Fill.color);
                    }
                    else if (shape.algorithm == RECURSIVE) {
                        FloodFillRec(hdc, shape.Fill.p.x, shape.Fill.p.y, shape.Fill.color);
                    }
                    else if (shape.algorithm == CONVEX) {
                        const POINT* points = shape.ScanLineFill.p;

                        ConvexFill(hdc, points, shape.ScanLineFill.points, shape.ScanLineFill.color);

                    }
                    else if (shape.algorithm == GENERAL) {
                        const POINT* points = shape.ScanLineFill.p;

                        GeneralFill(hdc, points, shape.ScanLineFill.points, shape.ScanLineFill.color);

                    }
                }
            }
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd); break;

        case WM_DESTROY:
            PostQuitMessage(0); break;

        default:return DefWindowProc(hwnd, m, wp, lp);
    }
    return 0;
}

// Helper Functions
int Round(double x)
{
    return (int)(x + 0.5);
}

//  for circle
void Draw8Points(HDC hdc,int xc,int yc, int a, int b,COLORREF color)
{
    SetPixel(hdc, xc+a, yc+b, color);
    SetPixel(hdc, xc-a, yc+b, color);
    SetPixel(hdc, xc-a, yc-b, color);
    SetPixel(hdc, xc+a, yc-b, color);
    SetPixel(hdc, xc+b, yc+a, color);
    SetPixel(hdc, xc-b, yc+a, color);
    SetPixel(hdc, xc-b, yc-a, color);
    SetPixel(hdc, xc+b, yc-a, color);
}

//  for ellipse
void Draw4Points(HDC hdc, int xc, int yc, int x, int y, COLORREF color) {
    SetPixel(hdc, xc + x, yc + y, color);
    SetPixel(hdc, xc - x, yc + y, color);
    SetPixel(hdc, xc + x, yc - y, color);
    SetPixel(hdc, xc - x, yc - y, color);
}


// Line algorithms
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

void DrawLineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF c) {
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

void DrawLineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    int dx = x2 - x1, dy = y2 - y1;
    double length = sqrt(dx * dx + dy * dy);
    double dt = 1.0 / length;
    for (double t = 0; t <= 1.0; t += dt) {
        int x = Round(x1 + dx * t);
        int y = Round(y1 + dy * t);
        SetPixel(hdc, x, y, color);
    }
}

// Circle algorithms

void DrawCircleDirect(HDC hdc, int xc, int yc, int R,COLORREF color) {
    int x=0,y=R;
    int R2=R*R;
    Draw8Points(hdc,xc,yc,x,y,color);
    while(x<y)
    {
        x++;
        y=Round(sqrt((double)(R2-x*x)));
        Draw8Points(hdc,xc,yc,x,y,color);
    }
}

void DrawCircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color) {
    int x = 0, y = r, d = 1 - r;
    while(x <= y){
        Draw8Points(hdc,xc,yc,x,y,color);
        if(d < 0) d+=2*x+3;
        else { d+=2*(x-y)+5; y--; }
        x++;
    }
}

void DrawCircleModMidpoint(HDC hdc,int xc,int yc, int r,COLORREF color)
{
    int x = 0,y = r;
    int d = 1-r;
    int d1 = 3, d2 = 5-2*r;
    Draw8Points(hdc,xc,yc,x,y,color);
    while(x<y)
    {
        if(d<0) { d+=d1; d2+=2; }
        else { d+=d2; d2+=4; y--; }
        d1+=2;
        x++;
        Draw8Points(hdc,xc,yc,x,y,color);
    }
}

void DrawCirclePolar(HDC hdc, int xc, int yc, int r, COLORREF color)
{
    int x = r, y = 0;
    double theta = 0, dtheta = 1.0 / r;
    Draw8Points(hdc,xc,yc,x,y,color);
    while(x>y)
    {
        theta += dtheta;
        x = Round(r*cos(theta));
        y = Round(r*sin(theta));
        Draw8Points(hdc,xc,yc,x,y,color);
    }
}

void DrawCircleItPolar(HDC hdc, int xc, int yc, int r, COLORREF color)
{
    double x = r,y = 0;
    double dtheta = 1.0 / r;
    double cdtheta = cos(dtheta), sdtheta = sin(dtheta);
    Draw8Points(hdc,xc,yc,r,0,color);
    while(x>y)
    {
        double x1 = x * cdtheta - y * sdtheta;
        y = x * sdtheta + y*cdtheta;
        x = x1;
        Draw8Points(hdc,xc,yc,Round(x),Round(y),color);
    }
}

// Ellipse algorithms
void DrawEllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    int a = abs(rx - xc);
    int b = abs(ry - yc);

    int x = 0;
    int y;
    double a2 = a * a;
    double b2 = b * b;

    while (x <= a) {
        y = Round(b * sqrt(1.0 - ((double)(x * x)) / a2));
        Draw4Points(hdc, xc, yc, x, y, color);
        x++;
    }

    while (y <= b) {
        x = Round(a * sqrt(1.0 - ((double)(y * y)) / b2));
        Draw4Points(hdc, xc, yc, x, y, color);
        y++;
    }

}

void DrawEllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    int a = abs(rx - xc);
    int b = abs(ry - yc);
    int x = 0;
    int y = b;

    int a2 = a * a;
    int b2 = b * b;

    int d = b2 - a2 * b + 0.25 * a2;
    int dx = 2 * b2 * x;
    int dy = 2 * a2 * y;

    while (dx < dy) {
        Draw4Points(hdc, xc, yc, x, y, color);
        x++;
        dx += 2 * b2;
        if (d < 0)
            d += dx + b2;
        else {
            y--;
            dy -= 2 * a2;
            d += dx - dy + b2;
        }
    }

    d = b2 * (x + 0.5) * (x + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
    while (y >= 0) {
        Draw4Points(hdc, xc, yc, x, y, color);
        y--;
        dy -= 2 * a2;
        if (d > 0)
            d += a2 - dy;
        else {
            x++;
            dx += 2 * b2;
            d += dx - dy + a2;
        }
    }
}

void DrawEllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color)
{
    int a = abs(rx - xc);
    int b = abs(ry - yc);

    double theta = 0;
    double dtheta = 1.0 / max(a, b);
    int x = a, y = 0;

    while (theta <= M_PI_2)
    {
        x = Round(a * cos(theta));
        y = Round(b * sin(theta));
        Draw4Points(hdc, xc, yc, x, y, color);
        theta += dtheta;
    }
}

// Fill
// Flood Fill algorithms
void FloodFillStack(HDC hdc, int x, int y, COLORREF fillColor) {
    COLORREF current = GetPixel(hdc,x,y);
    if(current != RGB(255, 255, 255) || current == fillColor) return;
    stack<Point> stk;
    stk.push({x,y});
    while(!stk.empty()){
        Point p=stk.top(); stk.pop();
        if(GetPixel(hdc,p.x,p.y) != current) continue;
        SetPixel(hdc,p.x,p.y,fillColor);
        stk.push({p.x,p.y-1});
        stk.push({p.x,p.y+1});
        stk.push({p.x-1,p.y});
        stk.push({p.x+1,p.y});
    }
}

void FloodFillRec(HDC hdc,int x,int y,COLORREF fillColor) {
    COLORREF current = GetPixel(hdc,x,y);
    if(current != RGB(255, 255, 255) || current == fillColor) return;
    SetPixel(hdc,x,y,fillColor);
    FloodFillRec(hdc,x+1,y,fillColor);
    FloodFillRec(hdc,x-1,y,fillColor);
    FloodFillRec(hdc,x,y+1,fillColor);
    FloodFillRec(hdc,x,y-1,fillColor);
}

// Convex Fill
const int MAX_ENTRIES =  800;
struct Entry
{
    int xmin,xmax;
};

void InitEntries(Entry table[])
{
    for(int i=0; i< MAX_ENTRIES ; i++)
    {

        table[i].xmin=MAXINT;
        table[i].xmax=-MAXINT;

    }
}

void ScanEdge(POINT v1,POINT v2,Entry table[])
{
    if(v1.y==v2.y)return;
    if(v1.y>v2.y)swap(v1,v2);
    double minv=(double)(v2.x-v1.x)/(v2.y-v1.y);
    double x=v1.x;
    int y=v1.y;
    while(y<v2.y)
    {
        if(x<table[y].xmin)table[y].xmin=(int)ceil(x);
        if(x>table[y].xmax)table[y].xmax=(int)floor(x);
        y++;
        x+=minv;
    }
}

void DrawSanLines(HDC hdc,Entry table[],COLORREF color)
{
    for(int y=0;y<MAX_ENTRIES;y++)
        if(table[y].xmin<table[y].xmax)
            for(int x=table[y].xmin;x<=table[y].xmax;x++)
                SetPixel(hdc,x,y,color);

}

void ConvexFill(HDC hdc,const POINT* p,int n,COLORREF color)
{
    Entry *table = new Entry[MAX_ENTRIES];
    InitEntries(table);
    POINT v1 = p[n-1];
    for(int i = 0; i < n; i++)
    {
        POINT v2 = p[i];
        ScanEdge(v1,v2,table);
        v1 = p[i];
    }
    DrawSanLines(hdc,table,color);
    delete table;
}

// General Polygon Fill
struct EdgeRec
{
    double x;
    double minv;
    int ymax;
    bool operator<(EdgeRec r)
    {
        return x<r.x;
    }
};
typedef list<EdgeRec> EdgeList;

EdgeRec InitEdgeRec(POINT& v1,POINT& v2)
{
    if(v1.y>v2.y)swap(v1,v2);
    EdgeRec rec;
    rec.x=v1.x;
    rec.ymax=v2.y;
    rec.minv=(double)(v2.x-v1.x)/(v2.y-v1.y);
    return rec;
}

void InitEdgeTable(const POINT *p,int n,EdgeList table[])
{
    POINT p1=p[n-1];
    for(int i=0;i<n;i++)
    {
        POINT p2 = p[i];
        if(p1.y==p2 .y){p1=p2 ;continue;}
        EdgeRec rec=InitEdgeRec(p1, p2 );
        table[p1.y].push_back(rec);
        p1 = p[i];
    }
}

void GeneralFill(HDC hdc,const POINT* p,int n,COLORREF c)
{
    EdgeList *table=new EdgeList [MAX_ENTRIES];
    InitEdgeTable(p,n,table);
    int y=0;
    while(y<MAX_ENTRIES && table[y].size()==0)y++;
    if(y==MAX_ENTRIES)return;
    EdgeList ActiveList=table[y];
    while (ActiveList.size()>0)
    {
        ActiveList.sort();
        for(EdgeList::iterator it=ActiveList.begin();it!=ActiveList.end();it++)
        {
            int x1=(int)ceil(it->x);
            it++;
            int x2=(int)floor(it->x);
            for(int x=x1;x<=x2;x++)SetPixel(hdc,x,y,c);
        }
        y++;
        EdgeList::iterator it=ActiveList.begin();
        while(it!=ActiveList.end())
            if(y==it->ymax) it=ActiveList.erase(it); else it++;
        for(EdgeList::iterator it=ActiveList.begin();it!=ActiveList.end();it++)
            it->x+=it->minv;
        ActiveList.insert(ActiveList.end(),table[y].begin(),table[y].end());
    }
    delete[] table;
}

POINT* ShapeDrawer(HDC hdc, COLORREF c) {
    int n;
    cout << "Enter number of Points: ";
    cin >> n;
    POINT *p = new POINT[n];
    for(int i=0;i<n;i++) {
        cout << "Point " << i+1 << " Enter x: ";
        cin >> p[i].x;
        cout << "Point " << i+1 << " Enter y: ";
        cin >> p[i].y;
    }
    POINT p1 = p[n-1];
    for(int i=0;i<n;i++)
    {
        POINT p2=p[i];
        DrawLineDDA(hdc, p1.x, p1.y , p2.x, p2.y, c);
        p1=p[i];
    }
    return p;
}