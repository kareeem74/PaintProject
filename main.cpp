#include <algorithm>
#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <cmath>
#include <stack>
#include <fstream>
#include <iostream>
using namespace std;

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

// Renamed from Rectangle to Line to avoid conflict with WinAPI Rectangle()
template <typename T>
struct Line {
    T x1, y1, x2, y2;
    Line(T _x1 = 0, T _y1 = 0, T _x2 = 0, T _y2 = 0)
        : x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}
};

// Constants for Cohen–Sutherland outcodes
const int INSIDE = 0;  // 0000
const int LEFT   = 1;  // 0001
const int RIGHT  = 2;  // 0010
const int BOTTOM = 4;  // 0100
const int TOP    = 8;  // 1000

int ComputeRegionCode(int x, int y, int xmin, int ymin, int xmax, int ymax) {
    int code = INSIDE;
    if (x < xmin)      code |= LEFT;
    else if (x > xmax) code |= RIGHT;
    if (y < ymin)      code |= BOTTOM;
    else if (y > ymax) code |= TOP;
    return code;
}

bool CohenSutherlandLineClip(Line<int>& line, int xmin, int ymin, int xmax, int ymax) {
    int x1 = line.x1, y1 = line.y1;
    int x2 = line.x2, y2 = line.y2;

    int code1 = ComputeRegionCode(x1, y1, xmin, ymin, xmax, ymax);
    int code2 = ComputeRegionCode(x2, y2, xmin, ymin, xmax, ymax);
    bool accept = false;

    while (true) {
        if ((code1 | code2) == 0) {
            // Both endpoints inside: trivially accept
            accept = true;
            break;
        } else if (code1 & code2) {
            // Both endpoints share an outside zone: trivially reject
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
                x1 = x;
                y1 = y;
                code1 = ComputeRegionCode(x1, y1, xmin, ymin, xmax, ymax);
            } else {
                x2 = x;
                y2 = y;
                code2 = ComputeRegionCode(x2, y2, xmin, ymin, xmax, ymax);
            }
        }
    }

    if (accept) {
        line.x1 = x1;
        line.y1 = y1;
        line.x2 = x2;
        line.y2 = y2;
        return true;
    }
    return false;
}

bool PointInCircle(int x, int y, int xc, int yc, int r) {
    int dx = x - xc;
    int dy = y - yc;
    return (dx * dx + dy * dy) <= (r * r);
}

vector<Point> SutherlandHodgmanPolygonClip(const vector<Point>& subject, const vector<Point>& clip) {
    auto isInside = [&](const Point& p, const Point& cp1, const Point& cp2) {
        return ((cp2.x - cp1.x) * (p.y - cp1.y) - (cp2.y - cp1.y) * (p.x - cp1.x)) >= 0;
    };

    auto computeIntersection = [&](const Point& s, const Point& p, const Point& cp1, const Point& cp2) {
        int x1 = s.x, y1 = s.y;
        int x2 = p.x, y2 = p.y;
        int x3 = cp1.x, y3 = cp1.y;
        int x4 = cp2.x, y4 = cp2.y;

        int denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (denom == 0) {
            return p;
        }
        int xi = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / denom;
        int yi = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / denom;
        return Point(xi, yi);
    };

    vector<Point> output = subject;
    int clipCount = static_cast<int>(clip.size());

    for (int i = 0; i < clipCount; i++) {
        vector<Point> input = output;
        output.clear();

        Point cp1 = clip[i];
        Point cp2 = clip[(i + 1) % clipCount];

        int inCount = static_cast<int>(input.size());
        if (inCount == 0) break;

        Point S = input[inCount - 1];
        for (int j = 0; j < inCount; j++) {
            Point P = input[j];
            if (isInside(P, cp1, cp2)) {
                if (!isInside(S, cp1, cp2)) {
                    Point I = computeIntersection(S, P, cp1, cp2);
                    output.push_back(I);
                }
                output.push_back(P);
            } else if (isInside(S, cp1, cp2)) {
                Point I = computeIntersection(S, P, cp1, cp2);
                output.push_back(I);
            }
            S = P;
        }
    }

    return output;
}

void InitConsole() {
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    SetConsoleTitleA("Paint Console");
}

// Enumerations for shape types and algorithms
enum ShapeType { LINE, CIRCLE, RECTANGLE, ELLIPSE, POLYGON, SPLINE };
enum Algorithm {
    DDA,
    MIDPOINT,
    PARAMETRIC_LINE,
    DIRECT_CIRCLE,
    ITERATIVE_POLAR_CIRCLE,
    MODIFIED_MIDPOINT_CIRCLE,
    POLAR_CIRCLE,
    PARAMETRIC_ELLIPSE,
    DIRECT_ELLIPSE,
    POLAR_ELLIPSE
};
enum FillMethod {
    NO_FILL,
    FLOOD_FILL_STACK,
    FLOOD_FILL_RECURSIVE,
    SCANLINE_POLYGON_FILL,
    FILL_CIRCLE_LINES,
    FILL_CIRCLE_CIRCLES,
    FILL_SQUARE_HERMITE,
    FILL_RECT_BEZIER
};

struct Shape {
    ShapeType type;
    COLORREF strokeColor;
    COLORREF fillColor;
    vector<Point> points;
    Algorithm algorithm;
    FillMethod fillMethod;
    int radius;
    bool filled;
};

vector<Shape> shapes;
COLORREF currentStrokeColor = RGB(0, 0, 0);
COLORREF currentFillColor   = RGB(255, 255, 255);
ShapeType currentTool       = LINE;
Algorithm currentAlgorithm  = DDA;
FillMethod currentFillMethod= NO_FILL;
bool     currentFilled      = false;
bool     isDrawing          = false;
Point    startPoint;
COLORREF customColors[16]   = {0};
vector<Point> currentPolygon;
vector<Point> currentSplinePoints;

// Forward declarations of drawing functions
void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color);
void DrawLineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color);
void DrawLineParametric(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color);

void DrawCircleDirect(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleIterativePolar(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCircleModifiedMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCirclePolar(HDC hdc, int xc, int yc, int r, COLORREF color);

void DrawEllipseDirect(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipsePolar(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipseParametric(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);

void FloodFillStack(HDC hdc, int x, int y, COLORREF fillColor);
void FloodFillRecursive(HDC hdc, int x, int y, COLORREF oldColor, COLORREF fillColor);
void ScanlineFillPolygon(HDC hdc, const vector<Point>& poly, COLORREF fillColor);
void FillCircleWithLines(HDC hdc, int xc, int yc, int r, COLORREF fillColor);
void FillCircleWithCircles(HDC hdc, int xc, int yc, int r, COLORREF fillColor);
void FillSquareWithHermite(HDC hdc, int x1, int y1, int x2, int y2, COLORREF fillColor);
void FillRectangleWithBezier(HDC hdc, int x1, int y1, int x2, int y2, COLORREF fillColor);

void DrawSplineCardinal(HDC hdc, const vector<Point>& ctrlPoints, COLORREF color);

void ShowColorPicker(HWND hwnd, bool pickStroke);
void SaveShapesToFile(HWND hwnd);
void LoadShapesFromFile(HWND hwnd);
void ClearShapes(HWND hwnd);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static bool polygonDrawing = false;
    static bool splineDrawing  = false;

    // Back‐buffer variables
    static HDC     hdcMem    = NULL;
    static HBITMAP hbmMem    = NULL;
    static HBITMAP hbmOld    = NULL;
    static int     bufWidth  = 0;
    static int     bufHeight = 0;

    switch (msg) {
    case WM_CREATE: {
        // Create menu bar (unchanged)
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)GetStockObject(WHITE_BRUSH));
        SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_CROSS));
        HMENU hMenu = CreateMenu();
        HMENU m;

        // ─── Line menu ─────────────────────────────────────────────────────────────
        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 1001, "DDA");
        AppendMenu(m, MF_STRING, 1002, "Midpoint");
        AppendMenu(m, MF_STRING, 1003, "Parametric");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m, "Line");

        // ─── Circle menu ────────────────────────────────────────────────────────────
        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 2001, "Direct");
        AppendMenu(m, MF_STRING, 2002, "Iterative Polar");
        AppendMenu(m, MF_STRING, 2003, "Midpoint");
        AppendMenu(m, MF_STRING, 2004, "Modified Midpoint");
        AppendMenu(m, MF_STRING, 2005, "Polar");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m, "Circle");

        // ─── Ellipse menu ────────────────────────────────────────────────────────────
        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 3001, "Direct");
        AppendMenu(m, MF_STRING, 3002, "Polar");
        AppendMenu(m, MF_STRING, 3003, "Parametric");
        AppendMenu(m, MF_STRING, 3004, "Midpoint");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m, "Ellipse");

        // ─── Rectangle menu ──────────────────────────────────────────────────────────
        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 4001, "Outline (DDA)");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m, "Rectangle");

        // ─── Polygon menu ────────────────────────────────────────────────────────────
        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 5001, "Draw Polygon");
        AppendMenu(m, MF_STRING, 5002, "Fill Polygon (Scanline)");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m, "Polygon");

        // ─── Spline menu ─────────────────────────────────────────────────────────────
        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 6001, "Cardinal Spline");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m, "Spline");

        // ─── Is Filled toggle ─────────────────────────────────────────────────────────
        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 9001, "Filled");
        AppendMenu(m, MF_STRING, 9002, "No Filled");
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)m, "Is Filled");

        // ─── Color / Save / Load / Clear ─────────────────────────────────────────────
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, 7001, "Choose Stroke Color...");
        AppendMenu(hMenu, MF_STRING, 7002, "Choose Fill Color...");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, 8001, "Save...");
        AppendMenu(hMenu, MF_STRING, 8002, "Load...");
        AppendMenu(hMenu, MF_STRING, 8003, "Clear Canvas");
        SetMenu(hwnd, hMenu);

        // Create memory DC and bitmap for double buffering
        {
            HDC hdcWindow = GetDC(hwnd);
            RECT rc;
            GetClientRect(hwnd, &rc);
            bufWidth  = rc.right - rc.left;
            bufHeight = rc.bottom - rc.top;
            hdcMem = CreateCompatibleDC(hdcWindow);
            hbmMem = CreateCompatibleBitmap(hdcWindow, bufWidth, bufHeight);
            hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
            // Fill background white
            HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
            FillRect(hdcMem, &rc, hbrWhite);
            ReleaseDC(hwnd, hdcWindow);
        }
        break;
    }

    case WM_SIZE: {
        // Recreate memory bitmap on resize
        int newWidth  = LOWORD(lParam);
        int newHeight = HIWORD(lParam);
        if (newWidth != bufWidth || newHeight != bufHeight) {
            bufWidth  = newWidth;
            bufHeight = newHeight;
            if (hdcMem && hbmMem) {
                SelectObject(hdcMem, hbmOld);
                DeleteObject(hbmMem);
                HDC hdcWindow = GetDC(hwnd);
                hbmMem = CreateCompatibleBitmap(hdcWindow, bufWidth, bufHeight);
                hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
                // Clear new buffer to white
                RECT rc = { 0, 0, bufWidth, bufHeight };
                HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
                FillRect(hdcMem, &rc, hbrWhite);
                ReleaseDC(hwnd, hdcWindow);
            }
        }
        break;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
            // ─── Line selections ─────────────────────────────────────────────────────
            case 1001:
                currentTool      = LINE;
                currentAlgorithm = DDA;
                break;
            case 1002:
                currentTool      = LINE;
                currentAlgorithm = MIDPOINT;
                break;
            case 1003:
                currentTool      = LINE;
                currentAlgorithm = PARAMETRIC_LINE;
                break;

            // ─── Circle selections ────────────────────────────────────────────────────
            case 2001:
                currentTool      = CIRCLE;
                currentAlgorithm = DIRECT_CIRCLE;
                break;
            case 2002:
                currentTool      = CIRCLE;
                currentAlgorithm = ITERATIVE_POLAR_CIRCLE;
                break;
            case 2003:
                currentTool      = CIRCLE;
                currentAlgorithm = MIDPOINT;
                break;
            case 2004:
                currentTool      = CIRCLE;
                currentAlgorithm = MODIFIED_MIDPOINT_CIRCLE;
                break;
            case 2005:
                currentTool      = CIRCLE;
                currentAlgorithm = POLAR_CIRCLE;
                break;

            // ─── Ellipse selections ────────────────────────────────────────────────────
            case 3001:
                currentTool      = ELLIPSE;
                currentAlgorithm = DIRECT_ELLIPSE;
                break;
            case 3002:
                currentTool      = ELLIPSE;
                currentAlgorithm = POLAR_ELLIPSE;
                break;
            case 3003:
                currentTool      = ELLIPSE;
                currentAlgorithm = PARAMETRIC_ELLIPSE;
                break;
            case 3004:
                currentTool      = ELLIPSE;
                currentAlgorithm = MIDPOINT;
                break;

            // ─── Rectangle selection ──────────────────────────────────────────────────
            case 4001:
                currentTool       = RECTANGLE;
                currentAlgorithm  = DDA;
                currentFillMethod = NO_FILL; // actual fill if currentFilled==true
                break;

            // ─── Polygon selections ────────────────────────────────────────────────────
            case 5001:
                currentTool       = POLYGON;
                currentFillMethod = NO_FILL;
                polygonDrawing    = true;
                currentPolygon.clear();
                break;
            case 5002:
                currentTool       = POLYGON;
                currentFillMethod = SCANLINE_POLYGON_FILL;
                currentFilled     = true; // force fill on polygon
                polygonDrawing    = true;
                currentPolygon.clear();
                break;

            // ─── Spline selection ──────────────────────────────────────────────────────
            case 6001:
                currentTool       = SPLINE;
                currentFillMethod = NO_FILL;
                splineDrawing     = true;
                currentSplinePoints.clear();
                break;

            // ─── Is Filled toggle ─────────────────────────────────────────────────────
            case 9001:
                currentFilled = true;
                break;
            case 9002:
                currentFilled = false;
                break;

            // ─── Color pickers / Save / Load / Clear ─────────────────────────────────
            case 7001:
                ShowColorPicker(hwnd, true);
                break;
            case 7002:
                ShowColorPicker(hwnd, false);
                break;
            case 8001:
                SaveShapesToFile(hwnd);
                break;
            case 8002:
                LoadShapesFromFile(hwnd);
                break;
            case 8003:
                // ClearShapes must clear the shapes vector
                ClearShapes(hwnd);
                // Also clear the back‐buffer to white
                if (hdcMem) {
                    RECT rc = { 0, 0, bufWidth, bufHeight };
                    HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
                    FillRect(hdcMem, &rc, hbrWhite);
                }
                break;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam), y = HIWORD(lParam);
        if (currentTool == POLYGON && polygonDrawing) {
            currentPolygon.push_back({ x, y });
        } else if (currentTool == SPLINE && splineDrawing) {
            currentSplinePoints.push_back({ x, y });
        } else {
            startPoint.x = x;
            startPoint.y = y;
            isDrawing = true;
        }
        break;
    }

    case WM_RBUTTONDOWN: {
        if (currentTool == POLYGON && polygonDrawing) {
            if (currentPolygon.size() >= 3) {
                Shape s;
                s.type        = POLYGON;
                s.strokeColor = currentStrokeColor;
                s.fillColor   = currentFillColor;
                s.points      = currentPolygon;
                s.algorithm   = DDA;
                s.fillMethod  = currentFillMethod;
                s.filled      = (currentFillMethod != NO_FILL);
                shapes.push_back(s);

                // Draw polygon directly onto back‐buffer
                vector<Point> clipPoly = {
                    {0, 0}, {bufWidth, 0}, {bufWidth, bufHeight}, {0, bufHeight}
                };
                vector<Point> clipped = SutherlandHodgmanPolygonClip(s.points, clipPoly);
                if (!clipped.empty()) {
                    int n = static_cast<int>(clipped.size());
                    for (int i = 0; i < n; i++) {
                        DrawLineDDA(
                            hdcMem,
                            clipped[i].x, clipped[i].y,
                            clipped[(i + 1) % n].x, clipped[(i + 1) % n].y,
                            s.strokeColor
                        );
                    }
                    if (s.filled && s.fillMethod == SCANLINE_POLYGON_FILL) {
                        ScanlineFillPolygon(hdcMem, clipped, s.fillColor);
                    }
                }
            }
            polygonDrawing = false;
            currentPolygon.clear();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (currentTool == SPLINE && splineDrawing) {
            if (currentSplinePoints.size() >= 4) {
                Shape s;
                s.type        = SPLINE;
                s.strokeColor = currentStrokeColor;
                s.fillColor   = currentFillColor;
                s.points      = currentSplinePoints;
                s.algorithm   = DDA;
                s.fillMethod  = NO_FILL;
                s.filled      = false;
                shapes.push_back(s);

                // Draw spline directly onto back‐buffer
                DrawSplineCardinal(hdcMem, s.points, s.strokeColor);
            }
            splineDrawing = false;
            currentSplinePoints.clear();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (!isDrawing) break;

        Point endPt = { LOWORD(lParam), HIWORD(lParam) };
        Shape s;
        s.type        = currentTool;
        s.strokeColor = currentStrokeColor;
        s.fillColor   = currentFillColor;
        s.points      = { startPoint, endPt };
        s.algorithm   = currentAlgorithm;
        s.fillMethod  = currentFillMethod;
        s.radius      = 0;

        if (s.type == CIRCLE) {
            int dx = endPt.x - startPoint.x;
            int dy = endPt.y - startPoint.y;
            s.radius = static_cast<int>(hypot(dx, dy));
        }

        // Determine fill/outline based on currentFilled
        switch (s.type) {
            case LINE:
                s.filled     = false;
                s.fillMethod = NO_FILL;
                break;

            case CIRCLE:
                if (currentFilled) {
                    s.filled     = true;
                    s.fillMethod = FLOOD_FILL_STACK;
                } else {
                    s.filled     = false;
                    s.fillMethod = NO_FILL;
                }
                break;

            case ELLIPSE:
                if (currentFilled) {
                    s.filled     = true;
                    s.fillMethod = FLOOD_FILL_STACK;
                } else {
                    s.filled     = false;
                    s.fillMethod = NO_FILL;
                }
                break;

            case RECTANGLE:
                if (currentFilled) {
                    s.filled     = true;
                    s.fillMethod = FLOOD_FILL_STACK;
                } else {
                    s.filled     = false;
                    s.fillMethod = NO_FILL;
                }
                break;

            case POLYGON:
                s.filled     = (currentFillMethod != NO_FILL);
                break;

            case SPLINE:
                s.filled     = false;
                s.fillMethod = NO_FILL;
                break;
        }

        // Save to shapes array
        shapes.push_back(s);

        // Draw shape immediately onto back‐buffer (hdcMem)
        switch (s.type) {
            case LINE: {
                Line<int> line(s.points[0].x, s.points[0].y,
                              s.points[1].x, s.points[1].y);
                DrawLineDDA(hdcMem, line.x1, line.y1, line.x2, line.y2, s.strokeColor);
                break;
            }
            case CIRCLE: {
                int xc = s.points[0].x, yc = s.points[0].y;
                int r  = s.radius;
                switch (s.algorithm) {
                    case DIRECT_CIRCLE:
                        DrawCircleDirect(hdcMem, xc, yc, r, s.strokeColor);
                        break;
                    case ITERATIVE_POLAR_CIRCLE:
                        DrawCircleIterativePolar(hdcMem, xc, yc, r, s.strokeColor);
                        break;
                    case MIDPOINT:
                        DrawCircleMidpoint(hdcMem, xc, yc, r, s.strokeColor);
                        break;
                    case MODIFIED_MIDPOINT_CIRCLE:
                        DrawCircleModifiedMidpoint(hdcMem, xc, yc, r, s.strokeColor);
                        break;
                    case POLAR_CIRCLE:
                        DrawCirclePolar(hdcMem, xc, yc, r, s.strokeColor);
                        break;
                    default:
                        break;
                }
                if (s.filled) {
                    FloodFillStack(hdcMem, xc, yc, s.fillColor);
                }
                break;
            }
            case ELLIPSE: {
                int xc = s.points[0].x, yc = s.points[0].y;
                int rx = abs(s.points[1].x - s.points[0].x);
                int ry = abs(s.points[1].y - s.points[0].y);
                switch (s.algorithm) {
                    case DIRECT_ELLIPSE:
                        DrawEllipseDirect(hdcMem, xc, yc, rx, ry, s.strokeColor);
                        break;
                    case POLAR_ELLIPSE:
                        DrawEllipsePolar(hdcMem, xc, yc, rx, ry, s.strokeColor);
                        break;
                    case PARAMETRIC_ELLIPSE:
                        DrawEllipseParametric(hdcMem, xc, yc, rx, ry, s.strokeColor);
                        break;
                    case MIDPOINT:
                        DrawEllipseMidpoint(hdcMem, xc, yc, rx, ry, s.strokeColor);
                        break;
                    default:
                        break;
                }
                if (s.filled) {
                    FloodFillStack(hdcMem, xc, yc, s.fillColor);
                }
                break;
            }
            case RECTANGLE: {
                int x1 = s.points[0].x, y1 = s.points[0].y;
                int x2 = s.points[1].x, y2 = s.points[1].y;
                DrawLineDDA(hdcMem, x1, y1, x2, y1, s.strokeColor);
                DrawLineDDA(hdcMem, x2, y1, x2, y2, s.strokeColor);
                DrawLineDDA(hdcMem, x2, y2, x1, y2, s.strokeColor);
                DrawLineDDA(hdcMem, x1, y2, x1, y1, s.strokeColor);
                if (s.filled) {
                    int mx = (x1 + x2) / 2, my = (y1 + y2) / 2;
                    FloodFillStack(hdcMem, mx, my, s.fillColor);
                }
                break;
            }
            case POLYGON:
                // (Handled in WM_RBUTTONDOWN)
                break;
            case SPLINE:
                // (Handled in WM_RBUTTONDOWN)
                break;
        }

        isDrawing = false;
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdcWindow = BeginPaint(hwnd, &ps);
        // Blit the back‐buffer to the window
        BitBlt(hdcWindow, 0, 0, bufWidth, bufHeight, hdcMem, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        // Clean up back‐buffer resources
        if (hdcMem) {
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


// ------------------------- Drawing Algorithms -------------------------

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

void DrawCircleIterativePolar(HDC hdc, int xc, int yc, int r, COLORREF color) {
    double x = r, y = 0;
    double theta = 0, dtheta = 1.0 / r;
    while (theta <= M_PI / 4) {
        int xi = static_cast<int>(round(x)), yi = static_cast<int>(round(y));
        SetPixel(hdc, xc + xi, yc + yi, color);
        SetPixel(hdc, xc - xi, yc + yi, color);
        SetPixel(hdc, xc + xi, yc - yi, color);
        SetPixel(hdc, xc - xi, yc - yi, color);
        SetPixel(hdc, xc + yi, yc + xi, color);
        SetPixel(hdc, xc - yi, yc + xi, color);
        SetPixel(hdc, xc + yi, yc - xi, color);
        SetPixel(hdc, xc - yi, yc - xi, color);

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
        SetPixel(hdc, xc + x, yc + y, color);
        SetPixel(hdc, xc - x, yc + y, color);
        SetPixel(hdc, xc + x, yc - y, color);
        SetPixel(hdc, xc - x, yc - y, color);
        SetPixel(hdc, xc + y, yc + x, color);
        SetPixel(hdc, xc - y, yc + x, color);
        SetPixel(hdc, xc + y, yc - x, color);
        SetPixel(hdc, xc - y, yc - x, color);
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
        SetPixel(hdc, xc + x, yc + y, color);
        SetPixel(hdc, xc - x, yc + y, color);
        SetPixel(hdc, xc + x, yc - y, color);
        SetPixel(hdc, xc - x, yc - y, color);
        SetPixel(hdc, xc + y, yc + x, color);
        SetPixel(hdc, xc - y, yc + x, color);
        SetPixel(hdc, xc + y, yc - x, color);
        SetPixel(hdc, xc - y, yc - x, color);
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

// 4. Flood Fill Algorithms

void FloodFillStack(HDC hdc, int x, int y, COLORREF fillColor) {
    COLORREF oldColor = GetPixel(hdc, x, y);
    if (oldColor == fillColor) return;
    stack<Point> stk;
    stk.push({ x, y });
    while (!stk.empty()) {
        Point p = stk.top(); stk.pop();
        COLORREF c = GetPixel(hdc, p.x, p.y);
        if (c != oldColor) continue;
        SetPixel(hdc, p.x, p.y, fillColor);
        stk.push({ p.x + 1, p.y });
        stk.push({ p.x - 1, p.y });
        stk.push({ p.x, p.y + 1 });
        stk.push({ p.x, p.y - 1 });
    }
}

void FloodFillRecursive(HDC hdc, int x, int y, COLORREF oldColor, COLORREF fillColor) {
    if (x < 0 || y < 0 || x >= GetDeviceCaps(hdc, HORZRES) ||
        y >= GetDeviceCaps(hdc, VERTRES)) return;
    COLORREF c = GetPixel(hdc, x, y);
    if (c != oldColor || c == fillColor) return;
    SetPixel(hdc, x, y, fillColor);
    FloodFillRecursive(hdc, x + 1, y, oldColor, fillColor);
    FloodFillRecursive(hdc, x - 1, y, oldColor, fillColor);
    FloodFillRecursive(hdc, x, y + 1, oldColor, fillColor);
    FloodFillRecursive(hdc, x, y - 1, oldColor, fillColor);
}

// 5. Polygon Scanline Fill

void ScanlineFillPolygon(HDC hdc, const vector<Point>& poly, COLORREF fillColor) {
    int n = static_cast<int>(poly.size());
    if (n < 3) return;
    int ymin = poly[0].y, ymax = poly[0].y;
    for (int i = 1; i < n; i++) {
        ymin = min(ymin, poly[i].y);
        ymax = max(ymax, poly[i].y);
    }
    for (int y = ymin; y <= ymax; y++) {
        vector<int> nodes;
        for (int i = 0, j = n - 1; i < n; j = i++) {
            if ((poly[i].y < y && poly[j].y >= y) ||
                (poly[j].y < y && poly[i].y >= y)) {
                int x = static_cast<int>(poly[i].x + (double)(y - poly[i].y) /
                              (poly[j].y - poly[i].y) *
                              (poly[j].x - poly[i].x));
                nodes.push_back(x);
            }
        }
        sort(nodes.begin(), nodes.end());
        for (size_t k = 0; k + 1 < nodes.size(); k += 2) {
            for (int x = nodes[k]; x < nodes[k + 1]; x++) {
                SetPixel(hdc, x, y, fillColor);
            }
        }
    }
}

// 6. Fill Circle with Radiating Lines

void FillCircleWithLines(HDC hdc, int xc, int yc, int r, COLORREF fillColor) {
    for (int angle = 0; angle < 360; angle++) {
        double theta = angle * M_PI / 180.0;
        int x_end = xc + static_cast<int>(round(r * cos(theta)));
        int y_end = yc + static_cast<int>(round(r * sin(theta)));
        DrawLineDDA(hdc, xc, yc, x_end, y_end, fillColor);
        COLORREF oldColor = GetPixel(hdc, xc, yc);
        if (oldColor == fillColor) return;
    }
}

// 7. Fill Circle with Concentric Circles

void FillCircleWithCircles(HDC hdc, int xc, int yc, int r, COLORREF fillColor) {
    for (int rr = r; rr >= 0; rr--) {
        DrawCircleMidpoint(hdc, xc, yc, rr, fillColor);
    }
}

// 8. Fill Square with Vertical Hermite Curves

void FillSquareWithHermite(HDC hdc, int x1, int y1, int x2, int y2, COLORREF fillColor) {
    int left = min(x1, x2), right = max(x1, x2);
    int top = min(y1, y2), bottom = max(y1, y2);
    int height = bottom - top;
    for (int x = left; x <= right; x++) {
        for (int y = top; y <= bottom; y++) {
            double t = static_cast<double>(y - top) / height;
            double h00 = 2 * t * t * t - 3 * t * t + 1;
            double h10 = t * t * t - 2 * t * t + t;
            double h01 = -2 * t * t * t + 3 * t * t;
            double h11 = t * t * t - t * t;
            double py = h00 * top + h10 * 1 + h01 * bottom + h11 * 1;
            if (static_cast<int>(round(py)) == y) {
                SetPixel(hdc, x, y, fillColor);
            }
        }
    }
}

// 9. Fill Rectangle with Horizontal Bezier Curves

void FillRectangleWithBezier(HDC hdc, int x1, int y1, int x2, int y2, COLORREF fillColor) {
    int left = min(x1, x2), right = max(x1, x2);
    int top = min(y1, y2), bottom = max(y1, y2);
    Point P0 = { left, top };
    Point P1 = { (left + right) / 2, (top + bottom) / 2 };
    Point P2 = { right, top };
    for (int y = top; y <= bottom; y++) {
        double t = static_cast<double>(y - top) / (bottom - top);
        double bx = (1 - t) * (1 - t) * P0.x + 2 * t * (1 - t) * P1.x + t * t * P2.x;
        int x_end = static_cast<int>(round(bx));
        for (int x = left; x <= x_end; x++) {
            SetPixel(hdc, x, y, fillColor);
        }
    }
}

// 10. Cardinal Spline (Catmull-Rom as special case, tension=0.5)

void DrawSplineCardinal(HDC hdc, const vector<Point>& ctrlPoints, COLORREF color) {
    int n = static_cast<int>(ctrlPoints.size());
    if (n < 4) return;
    double tension = 0.5;
    for (int i = 1; i < n - 2; i++) {
        Point P0 = ctrlPoints[i - 1];
        Point P1 = ctrlPoints[i];
        Point P2 = ctrlPoints[i + 1];
        Point P3 = ctrlPoints[i + 2];
        for (double t = 0; t <= 1; t += 0.01) {
            double t2 = t * t, t3 = t2 * t;
            double s = (1 - tension) / 2;
            double b1 = -s * t3 + 2 * s * t2 - s * t;
            double b2 = (2 - s) * t3 + (s - 3) * t2 + 1;
            double b3 = (s - 2) * t3 + (3 - 2 * s) * t2 + s * t;
            double b4 = s * t3 - s * t2;
            double x = b1 * P0.x + b2 * P1.x + b3 * P2.x + b4 * P3.x;
            double y = b1 * P0.y + b2 * P1.y + b3 * P2.y + b4 * P3.y;
            SetPixel(hdc, static_cast<int>(round(x)), static_cast<int>(round(y)), color);
        }
    }
}

// ------------------------- Utility & File I/O -------------------------

void ShowColorPicker(HWND hwnd, bool pickStroke) {
    CHOOSECOLOR cc = { sizeof(cc) };
    cc.hwndOwner    = hwnd;
    cc.lpCustColors = customColors;
    cc.rgbResult    = pickStroke ? currentStrokeColor : currentFillColor;
    cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc)) {
        if (pickStroke) currentStrokeColor = cc.rgbResult;
        else            currentFillColor   = cc.rgbResult;
    }
}

void SaveShapesToFile(HWND hwnd) {
    OPENFILENAME ofn = { sizeof(ofn) };
    char fileName[MAX_PATH] = "";
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Paint Files (*.pnt)\0*.pnt\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "pnt";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileName(&ofn)) {
        FILE* f = fopen(fileName, "wb");
        if (!f) return;
        int n = static_cast<int>(shapes.size());
        fwrite(&n, sizeof(int), 1, f);
        for (const Shape& s : shapes) {
            fwrite(&s.type, sizeof(s.type), 1, f);
            fwrite(&s.strokeColor, sizeof(s.strokeColor), 1, f);
            fwrite(&s.fillColor, sizeof(s.fillColor), 1, f);
            int count = static_cast<int>(s.points.size());
            fwrite(&count, sizeof(int), 1, f);
            fwrite(s.points.data(), sizeof(Point), count, f);
            fwrite(&s.algorithm, sizeof(s.algorithm), 1, f);
            fwrite(&s.fillMethod, sizeof(s.fillMethod), 1, f);
            fwrite(&s.radius, sizeof(s.radius), 1, f);
            fwrite(&s.filled, sizeof(s.filled), 1, f);
        }
        fclose(f);
    }
}

void LoadShapesFromFile(HWND hwnd) {
    OPENFILENAME ofn = { sizeof(ofn) };
    char fileName[MAX_PATH] = "";
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Paint Files (*.pnt)\0*.pnt\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn)) {
        FILE* f = fopen(fileName, "rb");
        if (!f) return;
        shapes.clear();
        int n;
        fread(&n, sizeof(int), 1, f);
        for (int i = 0; i < n; ++i) {
            Shape s;
            fread(&s.type, sizeof(s.type), 1, f);
            fread(&s.strokeColor, sizeof(s.strokeColor), 1, f);
            fread(&s.fillColor, sizeof(s.fillColor), 1, f);
            int count;
            fread(&count, sizeof(int), 1, f);
            s.points.resize(count);
            fread(s.points.data(), sizeof(Point), count, f);
            fread(&s.algorithm, sizeof(s.algorithm), 1, f);
            fread(&s.fillMethod, sizeof(s.fillMethod), 1, f);
            fread(&s.radius, sizeof(s.radius), 1, f);
            fread(&s.filled, sizeof(s.filled), 1, f);
            shapes.push_back(s);
        }
        fclose(f);
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

void ClearShapes(HWND hwnd) {
    shapes.clear();
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

// ----------------------- Main Entry Point -----------------------

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    InitConsole();

    WNDCLASSEX wc = {
        sizeof(wc),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0,
        0,
        hInst,
        NULL,
        LoadCursor(NULL, IDC_CROSS),
        (HBRUSH)(GetStockObject(WHITE_BRUSH)),
        NULL,
        "PaintClass",
        NULL
    };
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow("PaintClass", "Advanced Paint",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 700, NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}
