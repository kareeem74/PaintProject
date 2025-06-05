#include <algorithm>
#include <iostream>
#include <math.h>
#include <Windows.h>
#include <commdlg.h> // color picker
#include <complex.h>
#include <fstream>
#include <list>
#include <vector>
#include <stack>

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
// [*] Cardinal Spline Curve
// [*] Ellipse Algorithms [Direct, polar and midpoint]
// [*] Clipping algorithms using Rectangle as Clipping Window to clip [Point, Line, Polygon]
// [*] Clipping algorithms using Square as Clipping Window to clip [Point, Line]
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
    PARAMETRIC,
    POLAR,
    STACK,
    RECURSIVE,
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
    };
};

//typedef POINT Point;
struct Point {
    int x, y;
    Point(int _x=0, int _y=0) : x(_x), y(_y) {}
};
void InitConsole() {
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}

vector<Shape> shapes;

// Menu Options IDs
#define ID_MENU_SAVE                101
#define ID_MENU_LOAD                102
#define ID_COLOR_PICKER             103
#define ID_CLEAR_SCREEN             104
#define ID_DRAW_DDA_LINE            105
#define ID_DRAW_MIDPOINT_LINE       106
#define ID_DRAW_MIDPOINT_CIRCLE     107
#define ID_DRAW_POLAR_CIRCLE        108
#define ID_DRAW_PARAMETRIC_ELLIPSE  109
#define ID_DRAW_MIDPOINT_ELLIPSE    110
#define ID_DRAW_FILL_STACK          111
#define ID_DRAW_FILL_RECURSIVE      112

// Global variables
COLORREF currentColor = RGB(0, 0, 0);
bool isDrawing = false;
POINT startPoint;
POINT endPoint;

// Function declarations
LRESULT WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp);
void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c);
void DrawLineMidpoint(HDC hdc, int x1, int y1, int x2, int y2,COLORREF c);
void DrawLineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color);


void DrawCircleDirect(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleIterativePolar(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleModifiedMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCirclePolar(HDC hdc, int xc, int yc, int r, COLORREF color);

void DrawEllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipseParametric(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);

void FloodFillStack(HDC hdc, int x, int y, COLORREF fillColor);
void FloodFillRec(HDC hdc,int x,int y,COLORREF fillColor);
void DrawCardinalSpline(HDC hdc, const std::vector<POINT>& pts, float tension, COLORREF color);


bool CohenSutherlandClip(int &x1, int &y1,int &x2, int &y2,int xmin, int ymin,int xmax, int ymax);
vector<Point> SutherlandHodgmanPolygonClip(const vector<Point> &subject,const vector<Point> &clipWindow);


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
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hLineMenu, "Line");

            HMENU hCircleMenu = CreateMenu();
            AppendMenu(hCircleMenu, MF_STRING, ID_DRAW_MIDPOINT_CIRCLE, "Midpoint");
            AppendMenu(hCircleMenu, MF_STRING, ID_DRAW_POLAR_CIRCLE, "Polar");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hCircleMenu, "Circle");

            HMENU hEllipseMenu = CreateMenu();
            AppendMenu(hEllipseMenu, MF_STRING, ID_DRAW_MIDPOINT_ELLIPSE, "Parametric");
            AppendMenu(hEllipseMenu, MF_STRING, ID_DRAW_PARAMETRIC_ELLIPSE, "Midpoint");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hEllipseMenu, "Ellipse");

            HMENU hFillMenu = CreateMenu();
            AppendMenu(hFillMenu, MF_STRING, ID_DRAW_FILL_RECURSIVE, "Recursive");
            AppendMenu(hFillMenu, MF_STRING, ID_DRAW_FILL_STACK, "Using Stack");
            AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hFillMenu, "Fill");


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

                case ID_DRAW_MIDPOINT_CIRCLE: {
                    toolSelected = ID_DRAW_MIDPOINT_CIRCLE;
                    cout << "[Tool]   Midpoint Circle Tool selected." << endl;
                    break;
                }

                case ID_DRAW_POLAR_CIRCLE: {
                    toolSelected = ID_DRAW_POLAR_CIRCLE;
                    cout << "[Tool]   Polar Circle Tool selected." << endl;
                    break;
                }

                case ID_DRAW_PARAMETRIC_ELLIPSE: {
                    toolSelected = ID_DRAW_PARAMETRIC_ELLIPSE;
                    cout << "[Tool]   Parametric Ellipse Tool selected." << endl;
                    break;
                }

                case ID_DRAW_MIDPOINT_ELLIPSE: {
                    toolSelected = ID_DRAW_MIDPOINT_ELLIPSE;
                    cout << "[Tool]   Midpoint Ellipse Tool selected." << endl;
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
        if (toolSelected == ID_DRAW_DDA_LINE || toolSelected == ID_DRAW_MIDPOINT_LINE ||
            toolSelected == ID_DRAW_MIDPOINT_CIRCLE || toolSelected == ID_DRAW_POLAR_CIRCLE ||
            toolSelected == ID_DRAW_PARAMETRIC_ELLIPSE || toolSelected == ID_DRAW_MIDPOINT_ELLIPSE ||
            toolSelected == ID_DRAW_FILL_STACK || toolSelected == ID_DRAW_FILL_RECURSIVE) {
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
            else if (toolSelected == ID_DRAW_PARAMETRIC_ELLIPSE && isDrawing) {
                endPoint.x = LOWORD(lp);
                endPoint.y = HIWORD(lp);

                HDC hdc = GetDC(hwnd);

                DrawEllipseParametric(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y, currentColor);
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
            else if (toolSelected == ID_DRAW_FILL_RECURSIVE && isDrawing) {

                HDC hdc = GetDC(hwnd);

                FloodFillRec(hdc, startPoint.x, startPoint.y, currentColor);
                Shape newEllipse = {
                    .type = SHAPE_FILL,
                    .algorithm = STACK,
                    .Fill = {
                        {startPoint.x, startPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newEllipse);
                ReleaseDC(hwnd, hdc);
                isDrawing = false;
                cout << "[Draw]   The Shape filled recursively from the point (" << startPoint.x << "," << startPoint.y << ")"
                << "." << endl;
            }
            else if (toolSelected == ID_DRAW_FILL_STACK && isDrawing) {

                HDC hdc = GetDC(hwnd);

                FloodFillStack(hdc, startPoint.x, startPoint.y, currentColor);
                Shape newEllipse = {
                    .type = SHAPE_FILL,
                    .algorithm = STACK,
                    .Fill = {
                        {startPoint.x, startPoint.y},
                        currentColor
                    }
                };
                shapes.push_back(newEllipse);
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
                }
                else if (shape.type == SHAPE_CIRCLE) {
                    if (shape.algorithm == MIDPOINT) {
                        DrawCircleMidpoint(hdc, shape.Circle.c.x, shape.Circle.c.y,
                            shape.Circle.radius, shape.Line.color);
                    }
                    else if (shape.algorithm == POLAR) {
                        DrawCirclePolar(hdc, shape.Circle.c.x, shape.Circle.c.y,
                            shape.Circle.radius, shape.Line.color);
                    }
                }
                else if (shape.type == SHAPE_ELLIPSE) {
                    if (shape.algorithm == PARAMETRIC) {
                        DrawEllipseParametric(hdc, shape.Ellipse.c.x, shape.Ellipse.c.y,
                            shape.Ellipse.r.x, shape.Ellipse.r.y, shape.Line.color);
                    }
                    else if (shape.algorithm == MIDPOINT) {
                        DrawEllipseParametric(hdc, shape.Ellipse.c.x, shape.Ellipse.c.y,
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

int Round(double x)
{
    return (int)(x + 0.5);
}
void swap(int& x,int& y)
{
    int tmp=x;
    x=y;
    y=tmp;
}

void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    int dx = x2 - x1, dy = y2 - y1;
    double steps = max(abs(dx), abs(dy));
    double xi = static_cast<double>(dx) / steps, yi = static_cast<double>(dy) / steps;
    double x = x1, y = y1;
    for (int i = 0; i <= steps; i++) {
        SetPixel(hdc, static_cast<int>(round(x)), static_cast<int>(round(y)), color);
        x += xi;
        y += yi;
    }
}

void DrawLineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2 ? 1 : -1), sy = (y1 < y2 ? 1 : -1);
    int err = dx - dy;
    while (true) {
        SetPixel(hdc, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void DrawLineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    int dx = x2 - x1, dy = y2 - y1;
    double length = sqrt(dx * dx + dy * dy);
    double dt = 1.0 / length;
    for (double t = 0; t <= 1.0; t += dt) {
        int x = static_cast<int>(round(x1 + dx * t));
        int y = static_cast<int>(round(y1 + dy * t));
        SetPixel(hdc, x, y, color);
    }
}

// 2. Circle Drawing

void Draw8Points(HDC hdc, int xc, int yc, int xi, int yi, COLORREF color) {
    SetPixel(hdc, xc + xi, yc + yi, color);
    SetPixel(hdc, xc - xi, yc + yi, color);
    SetPixel(hdc, xc + xi, yc - yi, color);
    SetPixel(hdc, xc - xi, yc - yi, color);
    SetPixel(hdc, xc + yi, yc + xi, color);
    SetPixel(hdc, xc - yi, yc + xi, color);
    SetPixel(hdc, xc + yi, yc - xi, color);
    SetPixel(hdc, xc - yi, yc - xi, color);
}

void DrawCircleDirect(HDC hdc,int xc,int yc, int R,COLORREF color)
{
    int x=0,y=R;
    int R2=R*R;
    Draw8Points(hdc,xc,yc,x,y,color);
    while(x<y)
    {
        x++;
        y=ceil(sqrt((double)(R2-x*x)));
        Draw8Points(hdc,xc,yc,x,y,color);
    }
}

void DrawCircleIterativePolar(HDC hdc,int xc,int yc, int R,COLORREF color)
{
    double x=R,y=0;
    double dtheta=1.0/R;
    double cdtheta=cos(dtheta),sdtheta=sin(dtheta);
    Draw8Points(hdc,xc,yc,R,0,color);
    while(x>y)
    {
        double x1=x*cdtheta-y*sdtheta;
        y=x*sdtheta+y*cdtheta;
        x=x1;
        Draw8Points(hdc,xc,yc,round(x),round(y),color);
    }
}

void DrawCircleMidPoint(HDC hdc,int xc,int yc, int R,COLORREF color)
{
    int x=0,y=R;
    int d=1-R;
    Draw8Points(hdc,xc,yc,x,y,color);
    while(x<y)
    {
        if(d<0)
            d+=2*x+2;
        else
        {

            d+=2*(x-y)+5;
            y--;
        }
        x++;
        Draw8Points(hdc,xc,yc,x,y,color);
    }
}

void DrawCircleModifiedMidpoint(HDC hdc,int xc,int yc, int R,COLORREF color)
{
    int x=0,y=R;
    int d=1-R;
    int c1=3, c2=5-2*R;
    Draw8Points(hdc,xc,yc,x,y,color);
    while(x<y)
    {
        if(d<0)
        {
            d+=c1;
            c2+=2;
        }
        else
        {

            d+=c2;
            c2+=4;
            y--;
        }
        c1+=2;
        x++;
        Draw8Points(hdc,xc,yc,x,y,color);
    }
}

void DrawCirclePolar(HDC hdc,int xc,int yc, int R,COLORREF color)
{
    int x=R,y=0;
    double theta=0,dtheta=1.0/R;
    Draw8Points(hdc,xc,yc,x,y,color);
    while(x>y)
    {
        theta+=dtheta;
        x=round(R*cos(theta));
        y=round(R*sin(theta));
        Draw8Points(hdc,xc,yc,x,y,color);
    }
}

// 3. Ellipse Drawing

void DrawEllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    for (int x = -rx; x <= rx; x++) {
        int y = static_cast<int>(round(ry * sqrt(1.0 - (x * x) / (double)(rx * rx))));
        SetPixel(hdc, xc + x, yc + y, color);
        SetPixel(hdc, xc + x, yc - y, color);
    }
}

void DrawEllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    double theta = 0, dtheta = 1.0 / max(rx, ry);
    while (theta <= 2 * M_PI) {
        int x = xc + static_cast<int>(round(rx * cos(theta)));
        int y = yc + static_cast<int>(round(ry * sin(theta)));
        SetPixel(hdc, x, y, color);
        theta += dtheta;
    }
}

void DrawEllipseParametric(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    double t = 0, dt = 1.0 / max(rx, ry);
    while (t <= 2 * M_PI) {
        int x = xc + static_cast<int>(round(rx * cos(t)));
        int y = yc + static_cast<int>(round(ry * sin(t)));
        SetPixel(hdc, x, y, color);
        t += dt;
    }
}

void DrawEllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    double dx, dy, d1, d2, x = 0, y = ry;
    d1 = ry * ry - rx * rx * ry + 0.25 * rx * rx;
    dx = 2 * ry * ry * x;
    dy = 2 * rx * rx * y;
    while (dx < dy) {
        SetPixel(hdc, xc + static_cast<int>(x), yc + static_cast<int>(y), color);
        SetPixel(hdc, xc - static_cast<int>(x), yc + static_cast<int>(y), color);
        SetPixel(hdc, xc + static_cast<int>(x), yc - static_cast<int>(y), color);
        SetPixel(hdc, xc - static_cast<int>(x), yc - static_cast<int>(y), color);
        if (d1 < 0) {
            x++;
            dx += 2 * ry * ry;
            d1 += dx + ry * ry;
        } else {
            x++;
            y--;
            dx += 2 * ry * ry;
            dy -= 2 * rx * rx;
            d1 += dx - dy + ry * ry;
        }
    }
    d2 = ry * ry * (x + 0.5) * (x + 0.5) + rx * rx * (y - 1) * (y - 1) - rx * rx * ry * ry;
    while (y > 0) {
        SetPixel(hdc, xc + static_cast<int>(x), yc + static_cast<int>(y), color);
        SetPixel(hdc, xc - static_cast<int>(x), yc + static_cast<int>(y), color);
        SetPixel(hdc, xc + static_cast<int>(x), yc - static_cast<int>(y), color);
        SetPixel(hdc, xc - static_cast<int>(x), yc - static_cast<int>(y), color);
        if (d2 > 0) {
            y--;
            dy -= 2 * rx * rx;
            d2 += rx * rx - dy;
        } else {
            y--;
            x++;
            dx += 2 * ry * ry;
            dy -= 2 * rx * rx;
            d2 += dx - dy + rx * rx;
        }
    }
}


// ---------Filling Algorithms-----------
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
const int MAX_ENTRIES =  600;
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

void ConvexFill(HDC hdc,POINT p[],int n,COLORREF color)
{
    Entry *table=new Entry[MAX_ENTRIES];
    InitEntries(table);
    POINT v1=p[n-1];
    for(int i=0;i<n;i++)
    {
        POINT v2=p[i];
        ScanEdge(v1,v2,table);
        v1=p[i];
    }
    DrawSanLines(hdc,table,color);
    delete table;
}

// General Polygon Fill
const int MAXENTRIES = 600;
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

void InitEdgeTable(POINT *polygon,int n,EdgeList table[])
{
    POINT v1=polygon[n-1];
    for(int i=0;i<n;i++)
    {
        POINT v2=polygon[i];
        if(v1.y==v2.y){v1=v2;continue;}
        EdgeRec rec=InitEdgeRec(v1, v2);
        table[v1.y].push_back(rec);
        v1=polygon[i];
    }
}

void GeneralPolygonFill(HDC hdc,POINT *polygon,int n,COLORREF c)
{
    EdgeList *table=new EdgeList [MAXENTRIES];
    InitEdgeTable(polygon,n,table);
    int y=0;
    while(y<MAXENTRIES && table[y].size()==0)y++;
    if(y==MAXENTRIES)return;
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

// -----------------------------------------------------------------------------------------

// Cardinal Spline
struct Vector2
{
    double x,y;
    Vector2(double a=0,double b=0)
    {
        x=a; y=b;
    }
};
class Vector4
{
    double v[4];
public:
    Vector4(double a=0,double b=0,double c=0,double d=0)
    {
        v[0]=a; v[1]=b; v[2]=c; v[3]=d;
    }

    Vector4(double a[])
    {
        memcpy(v,a,4*sizeof(double));
    }
    double& operator[](int i)
    {
        return v[i];
    }
};

class Matrix4
{
    Vector4 M[4];
public:
    Matrix4(double A[])
    {
        memcpy(M,A,16*sizeof(double));
    }
    Vector4& operator[](int i)
    {
        return M[i];
    }
};

Vector4 operator*(Matrix4 M,Vector4& b)
{
    Vector4 res;
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
            res[i]+=M[i][j]*b[j];

    return res;
}
double DotProduct(Vector4& a,Vector4& b)
{
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3];
}

Vector4 GetHermiteCoeff(double x0,double s0,double x1,double s1)
{
    static double H[16]={2,1,-2,1,-3,-2,3,-1,0,1,0,0,1,0,0,0};
    static Matrix4 basis(H);
    Vector4 v(x0,s0,x1,s1);
    return basis*v;
}

void DrawHermiteCurve (HDC hdc,Vector2& P0,Vector2& T0,Vector2& P1,Vector2& T1 ,int
numpoints)
{
    Vector4 xcoeff=GetHermiteCoeff(P0.x,T0.x,P1.x,T1.x);
    Vector4 ycoeff=GetHermiteCoeff(P0.y,T0.y,P1.y,T1.y);
    if(numpoints<2)return;
    double dt=1.0/(numpoints-1);
    for(double t=0;t<=1;t+=dt)
    {
        Vector4 vt;
        vt[3]=1;
        for(int i=2;i>=0;i--)vt[i]=vt[i+1]*t;
        int x=round(DotProduct(xcoeff,vt));
        int y=round(DotProduct(ycoeff,vt));
        if(t==0)MoveToEx(hdc,x,y,NULL);else LineTo(hdc,x,y);
    }
}
void DrawHermiteCurve(HDC hdc,Vector2& P0,Vector2& T0,Vector2& P1,Vector2& T1 ,int
numpoints, COLORREF color)
{
    Vector4 xcoeff=GetHermiteCoeff(P0.x,T0.x,P1.x,T1.x);
    Vector4 ycoeff=GetHermiteCoeff(P0.y,T0.y,P1.y,T1.y);
    if(numpoints<2)return;
    double dt=1.0/(numpoints-1);
    for(double t=0;t<=1;t+=dt)
    {
        Vector4 vt;
        vt[3]=1;



        for(int i=2;i>=0;i--)vt[i]=vt[i+1]*t;
        int x=round(DotProduct(xcoeff,vt));
        int y=round(DotProduct(xcoeff,vt));
        SetPixel(hdc,x,y,color);
    }
}

void DrawCardinalSpline(HDC hdc,Vector2 P[],int n,double c,int numpix)
{
    double c1=1-c;
    Vector2 T0(c1*(P[2].x-P[0].x),c1*(P[2].y-P[0].y));
    for(int i=2;i<n-1;i++)
    {
        Vector2 T1(c1*(P[i+1].x-P[i-1].x),c1*(P[i+1].y-P[i-1].y));
        DrawHermiteCurve(hdc,P[i-1],T0,P[i],T1,numpix);
        T0=T1;
    }
}


// ---------Clipping Algorithms-----------
// Line Clipping
union OutCode
{
    unsigned All:4;
    struct{unsigned left:1,top:1,right:1,bottom:1;};
};

OutCode GetOutCode(double x,double y,int xleft,int ytop,int xright,int ybottom)
{
    OutCode out;
    out.All=0;
    if(x<xleft)out.left=1;else if(x>xright)out.right=1;
    if(y<ytop)out.top=1;else if(y>ybottom)out.bottom=1;
    return out;
}

void VIntersect(double xs,double ys,double xe,double ye,int x,double *xi,double *yi)
{
    *xi=x;
    *yi=ys+(x-xs)*(ye-ys)/(xe-xs);
}
void HIntersect(double xs,double ys,double xe,double ye,int y,double *xi,double *yi)
{
    *yi=y;
    *xi=xs+(y-ys)*(xe-xs)/(ye-ys);
}

void CohenSuth(HDC hdc,int xs,int ys,int xe,int ye,int xleft,int ytop,int xright,int ybottom)
{
    double x1=xs,y1=ys,x2=xe,y2=ye;
    OutCode out1=GetOutCode(x1,y1,xleft,ytop,xright,ybottom);
    OutCode out2=GetOutCode(x2,y2,xleft,ytop,xright,ybottom);
    while( (out1.All || out2.All) && !(out1.All & out2.All))
    {
        double xi,yi;
        if(out1.All)
        {
            if(out1.left)VIntersect(x1,y1,x2,y2,xleft,&xi,&yi);
            else if(out1.top)HIntersect(x1,y1,x2,y2,ytop,&xi,&yi);
            else if(out1.right)VIntersect(x1,y1,x2,y2,xright,&xi,&yi);
            else HIntersect(x1,y1,x2,y2,ybottom,&xi,&yi);
            x1=xi;
            y1=yi;
            out1=GetOutCode(x1,y1,xleft,ytop,xright,ybottom);
        } else
        {
            if(out2.left)VIntersect(x1,y1,x2,y2,xleft,&xi,&yi);
            else if(out2.top)HIntersect(x1,y1,x2,y2,ytop,&xi,&yi);
            else if(out2.right)VIntersect(x1,y1,x2,y2,xright,&xi,&yi);
            else HIntersect(x1,y1,x2,y2,ybottom,&xi,&yi);
            x2=xi;
            y2=yi;
            out2=GetOutCode(x2,y2,xleft,ytop,xright,ybottom);
        }
    }
    if(!out1.All && !out2.All)
    {
        MoveToEx(hdc,Round(x1),Round(y1),NULL);
        LineTo(hdc,Round(x2),Round(y2));
    }
}

// Polygon Clipping
struct Vertex
{
    double x,y;
    Vertex(int x1=0,int y1=0)
    {
        x=x1;
        y=y1;
    }
};
typedef vector<Vertex> VertexList;
typedef bool (*IsInFunc)(Vertex& v,int edge);
typedef Vertex (*IntersectFunc)(Vertex& v1,Vertex& v2,int edge);

VertexList ClipWithEdge(VertexList p,int edge,IsInFunc In,IntersectFunc Intersect)
{
    VertexList OutList;
    Vertex v1=p[p.size()-1];
    bool v1_in=In(v1,edge);
    for(int i=0;i<(int)p.size();i++)
    {
        Vertex v2=p[i];
        bool v2_in=In(v2,edge);
        if(!v1_in && v2_in)
        {

            OutList.push_back(Intersect(v1,v2,edge));
            OutList.push_back(v2);
        }else if(v1_in && v2_in) OutList.push_back(v2);
        else if(v1_in) OutList.push_back(Intersect(v1,v2,edge));
        v1=v2;
        v1_in=v2_in;
    }
    return OutList;
}

bool InLeft(Vertex& v,int edge)
{
    return v.x>=edge;
}
bool InRight(Vertex& v,int edge)
{
    return v.x<=edge;
}
bool InTop(Vertex& v,int edge)
{
    return v.y>=edge;
}
bool InBottom(Vertex& v,int edge)
{
    return v.y<=edge;
}

Vertex VIntersect(Vertex& v1,Vertex& v2,int xedge)
{
    Vertex res;
    res.x=xedge;
    res.y=v1.y+(xedge-v1.x)*(v2.y-v1.y)/(v2.x-v1.x);
    return res;
}
Vertex HIntersect(Vertex& v1,Vertex& v2,int yedge)
{
    Vertex res;
    res.y=yedge;
    res.x=v1.x+(yedge-v1.y)*(v2.x-v1.x)/(v2.y-v1.y);
    return res;
}

void PolygonClip(HDC hdc,POINT *p,int n,int xleft,int ytop,int xright,int ybottom)
{
    VertexList vlist;
    for(int i=0;i<n;i++)vlist.push_back(Vertex(p[i].x,p[i].y));
    vlist=ClipWithEdge(vlist,xleft,InLeft,VIntersect);
    vlist=ClipWithEdge(vlist,ytop,InTop,HIntersect);
    vlist=ClipWithEdge(vlist,xright,InRight,VIntersect);
    vlist=ClipWithEdge(vlist,ybottom,InBottom,HIntersect);
    Vertex v1=vlist[vlist.size()-1];
    for(int i=0;i<(int)vlist.size();i++)
    {
        Vertex v2=vlist[i];
        MoveToEx(hdc,Round(v1.x),Round(v1.y),NULL);
        LineTo(hdc,Round(v2.x),Round(v2.y));
        v1=v2;
    }
}

//--------------------------------------------------------------------------------
