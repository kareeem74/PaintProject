#include <iostream>
#include <math.h>
#include <Windows.h>
#include <commdlg.h> // color picker
#include <complex.h>
#include <fstream>
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
// [ ] Convex and Non-Convex Filling Algorithm
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

typedef POINT Point;
// struct Point {
//     int x, y;
//     Point(int _x=0, int _y=0) : x(_x), y(_y) {}
// };
void InitConsole() {
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}

// Canvas info to save and load
std::vector<Shape> shapes;

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

void DrawCircleDirect(HDC hdc, int xc, int yc, int r, COLORREF color) {
    for (int x = -r; x <= r; x++) {
        int y = static_cast<int>(round(sqrt(r * r - x * x)));
        SetPixel(hdc, xc + x, yc + y, color);
        SetPixel(hdc, xc + x, yc - y, color);
    }
}

void draw8Points(HDC hdc, int xc, int yc, int xi, int yi, COLORREF color) {
    SetPixel(hdc, xc + xi, yc + yi, color);
    SetPixel(hdc, xc - xi, yc + yi, color);
    SetPixel(hdc, xc + xi, yc - yi, color);
    SetPixel(hdc, xc - xi, yc - yi, color);
    SetPixel(hdc, xc + yi, yc + xi, color);
    SetPixel(hdc, xc - yi, yc + xi, color);
    SetPixel(hdc, xc + yi, yc - xi, color);
    SetPixel(hdc, xc - yi, yc - xi, color);
}
void DrawCircleIterativePolar(HDC hdc, int xc, int yc, int r, COLORREF color) {
    double x = r, y = 0;
    double theta = 0, dtheta = 1.0 / r;
    while (theta <= M_PI / 4) {
        int xi = static_cast<int>(round(x)), yi = static_cast<int>(round(y));
        draw8Points(hdc,xc,yc,xi,yi, color);

        theta += dtheta;
        double x_new = r * cos(theta);
        double y_new = r * sin(theta);
        x = x_new;
        y = y_new;
    }
}

void DrawCircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color) {
    int x = 0, y = r;
    int d = 1 - r;
    while (x <= y) {
        draw8Points(hdc,xc,yc,x,y, color);
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void DrawCircleModifiedMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color) {
    int x = 0, y = r;
    int d = 5 - 4 * r;
    while (x <= y) {
        draw8Points(hdc,xc,yc,x,y, color);
        if (d < 0) {
            d += 8 * x + 12;
        } else {
            d += 8 * (x - y) + 20;
            y--;
        }
        x++;
    }
}

void DrawCirclePolar(HDC hdc, int xc, int yc, int r, COLORREF color) {
    double theta = 0, dtheta = 1.0 / r;
    while (theta <= 2 * M_PI) {
        int x = xc + static_cast<int>(round(r * cos(theta)));
        int y = yc + static_cast<int>(round(r * sin(theta)));
        SetPixel(hdc, x, y, color);
        theta += dtheta;
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


// Cardinal Spline
void DrawCardinalSpline(HDC hdc, const std::vector<POINT>& pts, float tension, COLORREF color) {
    size_t n = pts.size();
    if (n < 2) return;

    // Compute derivatives D[i] for each control point:
    // For interior points: D[i] = (1 - tension) * (P[i+1] - P[i-1]) / 2
    // For endpoints:    D[0]     = (1 - tension) * (P[1]   - P[0])
    //                    D[n-1]   = (1 - tension) * (P[n-1] - P[n-2])
    std::vector<POINT> D(n);
    float coef = (1.0f - tension);
    // Endpoint derivative at i=0:
    D[0].x = Round(coef * float(pts[1].x - pts[0].x));
    D[0].y = Round(coef * float(pts[1].y - pts[0].y));
    // Interior points:
    for (size_t i = 1; i + 1 < n; ++i) {
        float dx = float(pts[i+1].x - pts[i-1].x) * 0.5f;
        float dy = float(pts[i+1].y - pts[i-1].y) * 0.5f;
        D[i].x = Round(coef * dx);
        D[i].y = Round(coef * dy);
    }
    // Endpoint derivative at i=n-1:
    D[n-1].x = Round(coef * float(pts[n-1].x - pts[n-2].x));
    D[n-1].y = Round(coef * float(pts[n-1].y - pts[n-2].y));

    // For each segment [P[i], P[i+1]], draw Hermite curve using P[i], P[i+1], D[i], D[i+1]:
    const int STEPS = 100;                      // Increase for smoother curve
    const double dt = 1.0 / STEPS;
    for (size_t i = 0; i + 1 < n; ++i) {
        const POINT& P1 = pts[i];
        const POINT& P2 = pts[i+1];
        const POINT& T1 = D[i];
        const POINT& T2 = D[i+1];

        for (int step = 0; step <= STEPS; ++step) {
            double t  = step * dt;
            double t2 = t * t;
            double t3 = t2 * t;

            // Hermite basis functions
            double h1 =  2.0 * t3 - 3.0 * t2 + 1.0;
            double h2 = -2.0 * t3 + 3.0 * t2;
            double h3 =       t3 - 2.0 * t2 + t;
            double h4 =       t3 -     t2;

            double xf = h1 * P1.x + h2 * P2.x + h3 * T1.x + h4 * T2.x;
            double yf = h1 * P1.y + h2 * P2.y + h3 * T1.y + h4 * T2.y;

            SetPixel(hdc, Round(xf), Round(yf), color);
        }
    }
}



// Region‐code bits for Cohen–Sutherland
const int INSIDE = 0;  // 0000
const int LEFT   = 1;  // 0001
const int RIGHT  = 2;  // 0010
const int BOTTOM = 4;  // 0100
const int TOP    = 8;  // 1000


bool ClipPointRect(const Point &p, int xmin, int ymin, int xmax, int ymax) {
    return (p.x >= xmin && p.x <= xmax && p.y >= ymin && p.y <= ymax);
}

static int ComputeRegionCodeRect(int x, int y, int xmin, int ymin, int xmax, int ymax) {
    int code = INSIDE;
    if (x < xmin)      code |= LEFT;
    else if (x > xmax) code |= RIGHT;
    if (y < ymin)      code |= BOTTOM;
    else if (y > ymax) code |= TOP;
    return code;
}


bool ClipLineRect(Point &p1, Point &p2, int xmin, int ymin, int xmax, int ymax) {
    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;
    int code1 = ComputeRegionCodeRect(x1, y1, xmin, ymin, xmax, ymax);
    int code2 = ComputeRegionCodeRect(x2, y2, xmin, ymin, xmax, ymax);
    bool accept = false;

    while (true) {
        if ((code1 | code2) == 0) {
            // Both endpoints inside
            accept = true;
            break;
        } else if (code1 & code2) {
            // Both share an outside zone → trivially reject
            break;
        } else {
            // At least one endpoint is outside; pick it
            int codeOut = (code1 != 0) ? code1 : code2;
            int x, y;

            if (codeOut & TOP) {
                x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
                y = ymax;
            } else if (codeOut & BOTTOM) {
                x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
                y = ymin;
            } else if (codeOut & RIGHT) {
                y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
                x = xmax;
            } else { // LEFT
                y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
                x = xmin;
            }

            if (codeOut == code1) {
                x1 = x;
                y1 = y;
                code1 = ComputeRegionCodeRect(x1, y1, xmin, ymin, xmax, ymax);
            } else {
                x2 = x;
                y2 = y;
                code2 = ComputeRegionCodeRect(x2, y2, xmin, ymin, xmax, ymax);
            }
        }
    }

    if (accept) {
        p1.x = x1; p1.y = y1;
        p2.x = x2; p2.y = y2;
        return true;
    }
    return false;
}

static bool isInsideRect(const Point &P, const Point &A, const Point &B) {
    int cross = (B.x - A.x) * (P.y - A.y) - (B.y - A.y) * (P.x - A.x);
    return (cross >= 0);
}

static Point computeIntersectionRect(
    const Point &S, const Point &P,
    const Point &A, const Point &B
) {
    int x1 = S.x, y1 = S.y;
    int x2 = P.x, y2 = P.y;
    int x3 = A.x, y3 = A.y;
    int x4 = B.x, y4 = B.y;

    int denom = (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4);
    if (denom == 0) {
        // Parallel lines; fallback to P
        return P;
    }
    int numX = (x1*y2 - y1*x2)*(x3 - x4) - (x1 - x2)*(x3*y4 - y3*x4);
    int numY = (x1*y2 - y1*x2)*(y3 - y4) - (y1 - y2)*(x3*y4 - y3*x4);
    int xi = numX / denom;
    int yi = numY / denom;
    return Point(xi, yi);
}

vector<Point> ClipPolygonRect(
    const vector<Point> &subject,
    int xmin, int ymin, int xmax, int ymax
) {
    // Define rectangle clip vertices in CCW:
    vector<Point> clipWindow = {
        { xmin, ymin }, { xmax, ymin },
        { xmax, ymax }, { xmin, ymax }
    };

    vector<Point> output = subject;
    int clipCount = static_cast<int>(clipWindow.size());

    for (int i = 0; i < clipCount; ++i) {
        Point A = clipWindow[i];
        Point B = clipWindow[(i + 1) % clipCount];
        vector<Point> input = move(output);
        output.clear();
        if (input.empty()) break;

        Point S = input.back();
        bool S_inside = isInsideRect(S, A, B);

        for (const Point &P : input) {
            bool P_inside = isInsideRect(P, A, B);
            if (P_inside) {
                if (!S_inside) {
                    // S outside, P inside → add intersection, then P
                    Point i = computeIntersectionRect(S, P, A, B);
                    output.push_back(i);
                }
                // Always add P when inside
                output.push_back(P);
            }
            else if (S_inside) {
                // S inside, P outside → add intersection only
                Point i = computeIntersectionRect(S, P, A, B);
                output.push_back(i);
            }
            // else both outside → add nothing
            S = P;
            S_inside = P_inside;
        }
    }
    return output;
}


// ------------------------------------------------------------
// 2) CLIPPING AGAINST A SQUARE [Point, Line]
// ------------------------------------------------------------

// For simplicity, define a square by its center (cx,cy) and half‐side length h.
// The square’s boundaries: [cx−h, cy−h] to [cx+h, cy+h].

// 2.1) Point-in-square test
bool ClipPointSquare(const Point &p, int cx, int cy, int halfSide) {
    return (p.x >= cx - halfSide && p.x <= cx + halfSide &&
            p.y >= cy - halfSide && p.y <= cy + halfSide);
}

// 2.2) Compute region code for Cohen–Sutherland specialized to square
static int ComputeRegionCodeSquare(int x, int y, int cx, int cy, int halfSide) {
    int code = INSIDE;
    int xmin = cx - halfSide;
    int xmax = cx + halfSide;
    int ymin = cy - halfSide;
    int ymax = cy + halfSide;
    if (x < xmin)      code |= LEFT;
    else if (x > xmax) code |= RIGHT;
    if (y < ymin)      code |= BOTTOM;
    else if (y > ymax) code |= TOP;
    return code;
}


bool ClipLineSquare(Point &p1, Point &p2, int cx, int cy, int halfSide) {
    int xmin = cx - halfSide;
    int xmax = cx + halfSide;
    int ymin = cy - halfSide;
    int ymax = cy + halfSide;

    int x1 = p1.x, y1 = p1.y;
    int x2 = p2.x, y2 = p2.y;
    int code1 = ComputeRegionCodeSquare(x1, y1, cx, cy, halfSide);
    int code2 = ComputeRegionCodeSquare(x2, y2, cx, cy, halfSide);
    bool accept = false;

    while (true) {
        if ((code1 | code2) == 0) {
            // Both endpoints inside
            accept = true;
            break;
        } else if (code1 & code2) {
            // Both share an outside region → reject
            break;
        } else {
            int codeOut = (code1 != 0) ? code1 : code2;
            int x, y;

            if (codeOut & TOP) {
                x = x1 + (x2 - x1) * (ymax - y1) / (y2 - y1);
                y = ymax;
            } else if (codeOut & BOTTOM) {
                x = x1 + (x2 - x1) * (ymin - y1) / (y2 - y1);
                y = ymin;
            } else if (codeOut & RIGHT) {
                y = y1 + (y2 - y1) * (xmax - x1) / (x2 - x1);
                x = xmax;
            } else { // LEFT
                y = y1 + (y2 - y1) * (xmin - x1) / (x2 - x1);
                x = xmin;
            }

            if (codeOut == code1) {
                x1 = x; y1 = y;
                code1 = ComputeRegionCodeSquare(x1, y1, cx, cy, halfSide);
            } else {
                x2 = x; y2 = y;
                code2 = ComputeRegionCodeSquare(x2, y2, cx, cy, halfSide);
            }
        }
    }

    if (accept) {
        p1.x = x1; p1.y = y1;
        p2.x = x2; p2.y = y2;
        return true;
    }
    return false;
}
