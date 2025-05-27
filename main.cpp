#include <iostream>
#include <Windows.h>
#include <commdlg.h>
#include <vector>
#include <cmath>
#include <stack>
#include <fstream>
using namespace std;

typedef POINT Point;
void InitConsole() {
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
}

enum ShapeType { LINE, CIRCLE, RECTANGLE, ELLIPSE };
enum Algorithm { DDA, MIDPOINT, PARAMETRIC, POLAR };

struct Shape {
    ShapeType type;
    COLORREF strokeColor;
    COLORREF fillColor;
    vector<Point> points;
    Algorithm algorithm;
    int radius;
    bool filled;
};

vector<Shape> shapes;
COLORREF currentStrokeColor = RGB(0,0,0);
COLORREF currentFillColor   = RGB(255,255,255);
ShapeType currentTool       = LINE;
Algorithm currentAlgorithm  = DDA;
bool     currentFilled      = false;
bool     isDrawing          = false;
Point    startPoint;
COLORREF customColors[16] = {0};

void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color);
void DrawLineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color);
void DrawCircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawCirclePolar(HDC hdc, int xc, int yc, int r, COLORREF color);
void DrawEllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void DrawEllipseParametric(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void FloodFillStack(HDC hdc, int x, int y, COLORREF fillColor);
void ScanlineFillEllipse(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color);
void ShowColorPicker(HWND hwnd, bool pickStroke);
void SaveShapesToFile(HWND hwnd);
void LoadShapesFromFile(HWND hwnd);
void ClearShapes(HWND hwnd);

void DrawLineDDA(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    int dx = x2 - x1, dy = y2 - y1;
    double steps = max(abs(dx), abs(dy));
    double xi = dx / steps, yi = dy / steps;
    double x = x1, y = y1;
    for(int i=0; i<=steps; i++){
        SetPixel(hdc, (int)round(x), (int)round(y), color);
        x+=xi; y+=yi;
    }
}

void DrawLineMidpoint(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    int dx = abs(x2-x1), dy = abs(y2-y1);
    int sx = x1<x2 ? 1 : -1, sy = y1<y2 ? 1 : -1;
    int err = dx - dy;
    while(true){
        SetPixel(hdc, x1, y1, color);
        if(x1==x2 && y1==y2) break;
        int e2 = 2*err;
        if(e2 > -dy) { err -= dy; x1 += sx; }
        if(e2 < dx)  { err += dx; y1 += sy; }
    }
}

void DrawCircleMidpoint(HDC hdc, int xc, int yc, int r, COLORREF color) {
    int x=0, y=r, d=1-r;
    while(x<=y){
        POINT pts[8] = {
            {xc+x, yc+y},{xc-x, yc+y},
            {xc+x, yc-y},{xc-x, yc-y},
            {xc+y, yc+x},{xc-y, yc+x},
            {xc+y, yc-x},{xc-y, yc-x}
        };
        for(auto&p:pts) SetPixel(hdc,p.x,p.y,color);
        if(d<0) d+=2*x+3;
        else    { d+=2*(x-y)+5; y--; }
        x++;
    }
}

void DrawCirclePolar(HDC hdc, int xc, int yc, int r, COLORREF color) {
    double theta=0, dtheta=1.0/r;
    while(theta<=2*M_PI){
        int x=xc+(int)(r*cos(theta));
        int y=yc+(int)(r*sin(theta));
        SetPixel(hdc,x,y,color);
        theta+=dtheta;
    }
}

void DrawEllipseParametric(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    double t=0, dt=1.0/max(rx,ry);
    while(t<=2*M_PI){
        int x=xc+(int)(rx*cos(t));
        int y=yc+(int)(ry*sin(t));
        SetPixel(hdc,x,y,color);
        t+=dt;
    }
}

void DrawEllipseMidpoint(HDC hdc, int xc, int yc, int rx, int ry, COLORREF color) {
    double dx, dy, d1, d2, x=0, y=ry;
    d1 = ry*ry - rx*rx*ry + 0.25*rx*rx;
    dx = 2*ry*ry*x; dy = 2*rx*rx*y;
    while(dx<dy){
        SetPixel(hdc, xc+x, yc+y, color);
        SetPixel(hdc, xc-x, yc+y, color);
        SetPixel(hdc, xc+x, yc-y, color);
        SetPixel(hdc, xc-x, yc-y, color);
        if(d1<0) { x++; dx+=2*ry*ry; d1+=dx+ry*ry; }
        else     { x++; y--; dx+=2*ry*ry; dy-=2*rx*rx; d1+=dx-dy+ry*ry; }
    }
    d2 = ry*ry*(x+0.5)*(x+0.5) + rx*rx*(y-1)*(y-1) - rx*rx*ry*ry;
    while(y>0){
        SetPixel(hdc, xc+x, yc+y, color);
        SetPixel(hdc, xc-x, yc+y, color);
        SetPixel(hdc, xc+x, yc-y, color);
        SetPixel(hdc, xc-x, yc-y, color);
        if(d2>0) { y--; dy-=2*rx*rx; d2+=rx*rx-dy; }
        else     { y--; x++; dx+=2*ry*ry; dy-=2*rx*rx; d2+=dx-dy+rx*rx; }
    }
}

void FloodFillStack(HDC hdc, int x, int y, COLORREF fillColor) {
    COLORREF oldColor = GetPixel(hdc,x,y);
    if(oldColor==fillColor) return;
    stack<Point> stk;
    stk.push({x,y});
    while(!stk.empty()){
        Point p=stk.top(); stk.pop();
        if(GetPixel(hdc,p.x,p.y)!=oldColor) continue;
        SetPixel(hdc,p.x,p.y,fillColor);
        stk.push({p.x+1,p.y});
        stk.push({p.x-1,p.y});
        stk.push({p.x,p.y+1});
        stk.push({p.x,p.y-1});
    }
}

void ScanlineFillEllipse(HDC hdc, int xc,int yc,int rx,int ry, COLORREF color) {
    for(int y=yc-ry; y<=yc+ry; y++){
        double dy = (y - yc);
        double v = 1.0 - dy*dy/(double)(ry*ry);
        if(v < 0) continue;
        int dx = (int)(rx * sqrt(v));
        for(int x=xc-dx; x<=xc+dx; x++){
            SetPixel(hdc,x,y,color);
        }
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
        FILE *f = fopen(fileName, "wb");
        if (!f) return;
        int n = (int)shapes.size();
        fwrite(&n, sizeof(int), 1, f);
        for (const Shape &s : shapes) {
            fwrite(&s.type, sizeof(s.type), 1, f);
            fwrite(&s.strokeColor, sizeof(s.strokeColor), 1, f);
            fwrite(&s.fillColor, sizeof(s.fillColor), 1, f);
            int count = (int)s.points.size();
            fwrite(&count, sizeof(int), 1, f);
            fwrite(s.points.data(), sizeof(Point), count, f);
            fwrite(&s.algorithm, sizeof(s.algorithm), 1, f);
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
        FILE *f = fopen(fileName, "rb");
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

void ShowColorPicker(HWND hwnd, bool pickStroke) {
    CHOOSECOLOR cc = { sizeof(cc) };
    cc.hwndOwner    = hwnd;
    cc.lpCustColors = customColors;
    cc.rgbResult    = pickStroke ? currentStrokeColor : currentFillColor;
    cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
    if(ChooseColor(&cc)) {
        if(pickStroke) currentStrokeColor = cc.rgbResult;
        else           currentFillColor   = cc.rgbResult;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    switch(msg) {
    case WM_CREATE: {
        HMENU hMenu = CreateMenu();
        HMENU m;

        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING, 1001,"DDA");
        AppendMenu(m, MF_STRING, 1002,"Midpoint");
        AppendMenu(hMenu, MF_POPUP,(UINT_PTR)m,"Line");

        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING,2001,"Outline Midpoint");
        AppendMenu(m, MF_STRING,2002,"Outline Polar");
        AppendMenu(m, MF_STRING,2003,"Filled Midpoint");
        AppendMenu(m, MF_STRING,2004,"Filled Polar");
        AppendMenu(hMenu, MF_POPUP,(UINT_PTR)m,"Circle");

        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING,3001,"Outline (DDA)");
        AppendMenu(m, MF_STRING,3002,"Filled");
        AppendMenu(hMenu, MF_POPUP,(UINT_PTR)m,"Rectangle");

        m = CreatePopupMenu();
        AppendMenu(m, MF_STRING,4001,"Outline Midpoint");
        AppendMenu(m, MF_STRING,4002,"Outline Parametric");
        AppendMenu(m, MF_STRING,4003,"Filled Midpoint");
        AppendMenu(m, MF_STRING,4004,"Filled Parametric");
        AppendMenu(hMenu, MF_POPUP,(UINT_PTR)m,"Ellipse");

        AppendMenu(hMenu, MF_SEPARATOR,0,NULL);
        AppendMenu(hMenu, MF_STRING,5001,"Choose Stroke Color...");
        AppendMenu(hMenu, MF_STRING,5002,"Choose Fill Color...");

        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, 6001, "Save...");
        AppendMenu(hMenu, MF_STRING, 6002, "Load...");
        AppendMenu(hMenu, MF_STRING, 6003, "Clear Canvas");

        SetMenu(hwnd,hMenu);
        break;
    }
    case WM_COMMAND: {
        switch(LOWORD(wParam)) {
        case 1001: currentTool=LINE; currentAlgorithm=DDA; currentFilled=false; break;
        case 1002: currentTool=LINE; currentAlgorithm=MIDPOINT; currentFilled=false; break;
        case 2001: currentTool=CIRCLE; currentAlgorithm=MIDPOINT; currentFilled=false; break;
        case 2002: currentTool=CIRCLE; currentAlgorithm=POLAR; currentFilled=false; break;
        case 2003: currentTool=CIRCLE; currentAlgorithm=MIDPOINT; currentFilled=true; break;
        case 2004: currentTool=CIRCLE; currentAlgorithm=POLAR; currentFilled=true; break;
        case 3001: currentTool=RECTANGLE; currentAlgorithm=DDA; currentFilled=false; break;
        case 3002: currentTool=RECTANGLE; currentAlgorithm=DDA; currentFilled=true; break;
        case 4001: currentTool=ELLIPSE; currentAlgorithm=MIDPOINT; currentFilled=false; break;
        case 4002: currentTool=ELLIPSE; currentAlgorithm=PARAMETRIC; currentFilled=false; break;
        case 4003: currentTool=ELLIPSE; currentAlgorithm=MIDPOINT; currentFilled=true; break;
        case 4004: currentTool=ELLIPSE; currentAlgorithm=PARAMETRIC; currentFilled=true; break;
        case 5001: ShowColorPicker(hwnd, true); break;
        case 5002: ShowColorPicker(hwnd, false); break;
        case 6001: SaveShapesToFile(hwnd); break;
        case 6002: LoadShapesFromFile(hwnd); break;
        case 6003: ClearShapes(hwnd); break;
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        startPoint.x = LOWORD(lParam);
        startPoint.y = HIWORD(lParam);
        isDrawing = true;
        break;
    }
    case WM_LBUTTONUP: {
        if(!isDrawing) break;
        Point endPt = { LOWORD(lParam), HIWORD(lParam) };
        Shape s;
        s.type        = currentTool;
        s.strokeColor = currentStrokeColor;
        s.fillColor   = currentFillColor;
        s.points      = { startPoint, endPt };
        s.algorithm   = currentAlgorithm;
        s.filled      = currentFilled;
        s.radius      = 0;
        if(s.type==CIRCLE) {
            int dx=endPt.x-startPoint.x, dy=endPt.y-startPoint.y;
            s.radius = (int)hypot(dx,dy);
        }
        shapes.push_back(s);
        InvalidateRect(hwnd,NULL,TRUE);
        isDrawing = false;
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        for (const auto &sh : shapes) {
            switch (sh.type) {
            case LINE:
                if (sh.algorithm == DDA)
                    DrawLineDDA(hdc, sh.points[0].x, sh.points[0].y,
                                sh.points[1].x, sh.points[1].y, sh.strokeColor);
                else
                    DrawLineMidpoint(hdc, sh.points[0].x, sh.points[0].y,
                                    sh.points[1].x, sh.points[1].y, sh.strokeColor);
                break;

            case CIRCLE:
                if (sh.algorithm == MIDPOINT)
                    DrawCircleMidpoint(hdc, sh.points[0].x, sh.points[0].y,
                                       sh.radius, sh.strokeColor);
                else
                    DrawCirclePolar(hdc, sh.points[0].x, sh.points[0].y,
                                    sh.radius, sh.strokeColor);
                if (sh.filled)
                    ScanlineFillEllipse(hdc, sh.points[0].x, sh.points[0].y,
                                        sh.radius, sh.radius, sh.fillColor);
                break;

            case RECTANGLE: {
                int x1 = sh.points[0].x, y1 = sh.points[0].y;
                int x2 = sh.points[1].x, y2 = sh.points[1].y;
                DrawLineDDA(hdc, x1, y1, x2, y1, sh.strokeColor);
                DrawLineDDA(hdc, x2, y1, x2, y2, sh.strokeColor);
                DrawLineDDA(hdc, x2, y2, x1, y2, sh.strokeColor);
                DrawLineDDA(hdc, x1, y2, x1, y1, sh.strokeColor);
                if (sh.filled)
                    FloodFillStack(hdc, (x1 + x2) / 2, (y1 + y2) / 2, sh.fillColor);
                break;
            }

            case ELLIPSE: {
                int rx = abs(sh.points[1].x - sh.points[0].x);
                int ry = abs(sh.points[1].y - sh.points[0].y);
                if (sh.algorithm == PARAMETRIC)
                    DrawEllipseParametric(hdc, sh.points[0].x, sh.points[0].y,
                                          rx, ry, sh.strokeColor);
                else
                    DrawEllipseMidpoint(hdc, sh.points[0].x, sh.points[0].y,
                                        rx, ry, sh.strokeColor);
                if (sh.filled)
                    ScanlineFillEllipse(hdc, sh.points[0].x, sh.points[0].y,
                                        rx, ry, sh.fillColor);
                break;
            }
            }
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}

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
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW+1),
        NULL,
        "PaintClass",
        NULL
    };
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindow("PaintClass","Advanced Paint",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,CW_USEDEFAULT,
                             800,600,NULL,NULL,hInst,NULL);
    ShowWindow(hwnd,nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg,NULL,0,0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}