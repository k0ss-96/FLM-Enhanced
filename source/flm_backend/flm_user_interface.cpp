//=============================================================================
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file flm_user_interface.h
/// @brief  FLM UI Options used by console application
//=============================================================================
#include "flm_user_interface.h"
#include <windows.h>
#include <stdio.h>
#include <string>

// ============================================================================
// 1. ГЛОБАЛЬНІ ЗМІННІ ТА ОГОЛОШЕННЯ
// ============================================================================
#define WM_USER_UPDATE_LATENCY (WM_USER + 1)
HWND   g_hWndOverlay = NULL;       
double g_LatestLatencyMs = 0.0;    

// Попереднє оголошення функцій, щоб код нижче їх "бачив"
LRESULT CALLBACK WndProcOverlay(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);
void ShowOverlay(HINSTANCE hInst);

FLM_UI_SETTINGS g_ui;
FLM_USER_INTERFACE g_user_interface;

static WNDPROC InputWndProcOriginal = NULL;

// ============================================================================
// 2. СТАНДАРТНІ ФУНКЦІЇ ІНТЕРФЕЙСУ AMD
// ============================================================================
LRESULT CALLBACK InputPositiveFloatWndProc(HWND hwnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CHAR)
    {
        if (!((wParam >= '0' && wParam <= '9') || wParam == '.' || wParam == VK_RETURN || wParam == VK_DELETE || wParam == VK_BACK))
        {
            return 0;
        }
    }
    return CallWindowProc(InputWndProcOriginal, hwnd, msg, wParam, lParam);
}

extern FLM_USER_INTERFACE g_user_interface;

LRESULT CALLBACK WndProcUI(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
    static bool editFloatBiasOk        = false;
    static bool editFloatThresholdOk   = false;
    LRESULT     result                 = 0;

    switch (msg)
    {
    case WM_KEYDOWN: if( wParam == VK_ESCAPE )
                        g_user_interface.HideUI();
                     break;
    case WM_COMMAND:
    {
        if (g_ui.runtimeOptions)
        {
            unsigned short idc_buttons = LOWORD(wParam);
            switch (idc_buttons)
            {
            case IDC_RADIO_DISPLAY_RUN:
                g_ui.runtimeOptions->printLevel = FLM_PRINT_LEVEL::RUN;
                break;
            case IDC_RADIO_DISPLAY_AVERAGE:
                g_ui.runtimeOptions->printLevel = FLM_PRINT_LEVEL::ACCUMULATED;
                break;
            case IDC_RADIO_DISPLAY_OPERATIONAL:
                g_ui.runtimeOptions->printLevel = FLM_PRINT_LEVEL::OPERATIONAL;
                break;
            case IDC_RADIO_MOUSE_MOVE:
            case IDC_RADIO_MOUSE_CLICK:
            {
                if (idc_buttons == IDC_RADIO_MOUSE_MOVE)
                    g_ui.runtimeOptions->mouseEventType = FLM_MOUSE_EVENT_TYPE::MOUSE_MOVE;
                else
                    g_ui.runtimeOptions->mouseEventType = FLM_MOUSE_EVENT_TYPE::MOUSE_CLICK;
                float       threshold               = g_ui.runtimeOptions->thresholdCoefficient[g_ui.runtimeOptions->mouseEventType];
                std::string str                     = FlmFormatStr("%3.1f", threshold);
                SetWindowText(g_ui.hWndEditThreshold, str.c_str());
            }
            break;
            case IDC_CHECK_AUTO_BIAS:
                g_ui.runtimeOptions->autoBias = !g_ui.runtimeOptions->autoBias;
                if (g_ui.hWndAutoBias)
                    SendMessage(g_ui.hWndAutoBias, BM_SETCHECK, g_ui.runtimeOptions->autoBias ? TRUE : FALSE, NULL);
                if (g_ui.hWndEditBias)
                    SendMessage(g_ui.hWndEditBias, EM_SETREADONLY, g_ui.runtimeOptions->autoBias ? TRUE : FALSE, NULL);
                break;
            case IDC_CHECK_MINIMIZE_APP:
                g_ui.runtimeOptions->minimizeApp = !g_ui.runtimeOptions->minimizeApp;
                if (g_ui.hWndMinimizeApp)
                    SendMessage(g_ui.hWndMinimizeApp, BM_SETCHECK, g_ui.runtimeOptions->minimizeApp ? TRUE : FALSE, NULL);
                break;
            case IDC_CHECK_FRAME_GEN:
                g_ui.runtimeOptions->gameUsesFrameGeneration = !g_ui.runtimeOptions->gameUsesFrameGeneration;
                if (g_ui.hWndGameHasFG)
                    SendMessage(g_ui.hWndGameHasFG, BM_SETCHECK, g_ui.runtimeOptions->gameUsesFrameGeneration ? TRUE : FALSE, NULL);
                break;
            case IDC_CAPTURE_REGION:
            {
                if (g_ui.capture_region_showing)
                {
                    g_ui.processWindowRegion = false;
                    ShowWindow(g_ui.hWndRegion, SW_HIDE);
                }
                else
                {
                    SetWindowPos(g_ui.hWndRegion,
                                 HWND_TOPMOST,
                                 g_ui.runtimeOptions->iCaptureX,
                                 g_ui.runtimeOptions->iCaptureY,
                                 g_ui.runtimeOptions->iCaptureWidth,
                                 g_ui.runtimeOptions->iCaptureHeight,
                                 SWP_DRAWFRAME);
                    g_ui.processWindowRegion = true;
                    ShowWindow(g_ui.hWndRegion, SW_SHOW);
                }
                g_ui.capture_region_showing = !g_ui.capture_region_showing;
            }
            break;
            case IDC_EDIT_BIAS:
            {
                int len = GetWindowTextLengthW(g_ui.hWndEditBias) + 1;
                if ((len > 1) && (len < MAX_EDIT_TXT))
                {
                    GetWindowTextA(g_ui.hWndEditBias, g_ui.editTextBias, len);
                    if (g_ui.runtimeOptions)
                    {
                        std::string str = g_ui.editTextBias;
                        editFloatBiasOk = FlmIsFloatNumber(str);
                        if (editFloatBiasOk)
                        {
                            g_ui.runtimeOptions->biasOffset = std::stof(str);
                        }
                    }
                }
            }
            break;
            case IDC_EDIT_THRESHOLD:
            {
                int len = GetWindowTextLengthW(g_ui.hWndEditThreshold) + 1;
                if ((len > 1) && (len < MAX_EDIT_TXT))
                {
                    GetWindowTextA(g_ui.hWndEditThreshold, g_ui.editTextThreshold, len);
                    if (g_ui.runtimeOptions)
                    {
                        std::string str = g_ui.editTextThreshold;
                        editFloatThresholdOk = FlmIsFloatNumber(str);
                        if (editFloatThresholdOk)
                        {
                            g_ui.runtimeOptions->thresholdCoefficient[g_ui.runtimeOptions->mouseEventType] = std::stof(str);
                        }

                    }
                }
            }
            break;
            case IDC_BUTTON_CLOSE:
                g_user_interface.HideUI();
                break;
            case IDC_BUTTON_SAVE:
                g_ui.runtimeOptions->saveUserSettings = true;
                break;
            };
        }
    }
    break;

    case WM_CREATE:
    {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", " FLM Latency Display Mode ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 10, FLM_OPTIONS_WINDOW_WIDTH - 20, 100, hWnd, (HMENU)IDC_GRPBUTTONS, hInst, NULL);
        {  
            g_ui.hWndLatencyDisplay[0] = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Run latency measurements using small samples", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 20, 35, FLM_OPTIONS_WINDOW_WIDTH - 40, 20, hWnd, (HMENU)IDC_RADIO_DISPLAY_RUN, hInst, NULL);
            g_ui.hWndLatencyDisplay[1] = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Continuously accumulated measurement", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 20, 60, FLM_OPTIONS_WINDOW_WIDTH - 40, 20, hWnd, (HMENU)IDC_RADIO_DISPLAY_AVERAGE, hInst, NULL);
            g_ui.hWndLatencyDisplay[2] = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Show all measurements per line", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 20, 85, FLM_OPTIONS_WINDOW_WIDTH - 40, 20, hWnd, (HMENU)IDC_RADIO_DISPLAY_OPERATIONAL, hInst, NULL);
        }

        CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", " Measurement Using ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 120, FLM_OPTIONS_WINDOW_WIDTH - 20, 64, hWnd, (HMENU)IDC_GRPBUTTONS, hInst, NULL);
        {  
            g_ui.hWndMouseClick[0] = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Mouse Move", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 20, 140, 125, 20, hWnd, (HMENU)IDC_RADIO_MOUSE_MOVE, hInst, NULL);
            g_ui.hWndMouseClick[1] = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Mouse Click", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 20, 160, 120, 20, hWnd, (HMENU)IDC_RADIO_MOUSE_CLICK, hInst, NULL);
        }

        g_ui.hWndEditBias = CreateWindowEx(WS_EX_WINDOWEDGE, "EDIT", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT, 160, 160, 50, 20, hWnd, (HMENU)IDC_EDIT_BIAS, hInst, NULL);
        InputWndProcOriginal = (WNDPROC)SetWindowLongPtr(g_ui.hWndEditBias, GWLP_WNDPROC, (LONG_PTR)InputPositiveFloatWndProc);

        g_ui.hWndAutoBias = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Auto", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 300, 160, 65, 20, hWnd, (HMENU)IDC_CHECK_AUTO_BIAS, hInst, NULL);

        CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", " Frame Capture Setting ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 190, FLM_OPTIONS_WINDOW_WIDTH - 20, 50, hWnd, (HMENU)IDC_GRPBUTTONS, hInst, NULL);
        {  
            g_ui.hWndButton = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Set Capture Region", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 210, 165, 25, hWnd, (HMENU)IDC_CAPTURE_REGION, hInst, NULL);
            g_ui.hWndEditThreshold = CreateWindowEx(WS_EX_WINDOWEDGE, "EDIT", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT, 200, 210, 50, 20, hWnd, (HMENU)IDC_EDIT_THRESHOLD, hInst, NULL);
            SetWindowLongPtr(g_ui.hWndEditThreshold, GWLP_WNDPROC, (LONG_PTR)InputPositiveFloatWndProc);
        }

        #if !defined(HIDE_MINIMIZE_APP_TOGGLE)
            g_ui.hWndMinimizeApp = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Minimize window during measurements", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, FLM_OPTIONS_WINDOW_HEIGHT - 55, FLM_OPTIONS_WINDOW_WIDTH-60, 20, hWnd, (HMENU)IDC_CHECK_MINIMIZE_APP, hInst, NULL);
        #endif

        g_ui.hWndGameHasFG = CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Game uses Frame Generation", WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 10, FLM_OPTIONS_WINDOW_HEIGHT - 75, FLM_OPTIONS_WINDOW_WIDTH-60, 20, hWnd, (HMENU)IDC_CHECK_FRAME_GEN, hInst, NULL);

        CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Save settings to INI", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, FLM_OPTIONS_WINDOW_HEIGHT - 30, 165, 25, hWnd, (HMENU)IDC_BUTTON_SAVE, hInst, NULL);
        CreateWindowEx(WS_EX_WINDOWEDGE, "BUTTON", "Close", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, FLM_OPTIONS_WINDOW_WIDTH  - 65, FLM_OPTIONS_WINDOW_HEIGHT - 30, 55, 25, hWnd, (HMENU)IDC_BUTTON_CLOSE, hInst, NULL);
    }
    break;
    case WM_PAINT:  
    {
        HDC         hdc;
        PAINTSTRUCT ps;
        hdc = BeginPaint(hWnd, &ps);
        if (hdc)
        {
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, RGB(50, 50, 50));
            std::string str = "bias (ms) ";
            TextOut(hdc, 220, 160, str.c_str(), (INT)str.length());
            str = "Threshold";
            TextOut(hdc, 260, 210, str.c_str(), (INT)str.length());
        }
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_CTLCOLOREDIT:
    {
        HDC  hdcEdit = (HDC)wParam;
        HWND WNDEdit = WindowFromDC(hdcEdit);
        if (WNDEdit == g_ui.hWndEditBias)
        {
            if (editFloatBiasOk) SetTextColor(hdcEdit, RGB(0, 0, 0));
            else SetTextColor(hdcEdit, RGB(255, 0, 0));
        }
        if (WNDEdit == g_ui.hWndEditThreshold)
        {
            if (editFloatThresholdOk) SetTextColor(hdcEdit, RGB(0, 0, 0));
            else SetTextColor(hdcEdit, RGB(255, 0, 0));
        }
    }
    break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(255, 255, 255));
        SetBkColor(hdcStatic, RGB(50, 50, 50));
        if (g_ui.hbrBkgnd == NULL) g_ui.hbrBkgnd = CreateSolidBrush(RGB(50, 50, 50));
        return (INT_PTR)g_ui.hbrBkgnd;
    }
    break;
    case WM_NCHITTEST:
    {
        LRESULT position = DefWindowProc(hWnd, msg, wParam, lParam);
        return position == HTCLIENT ? HTCAPTION : position;
    }
    break;
    case WM_QUIT:
    case WM_DESTROY:
        if (g_ui.hbrBkgnd != NULL)
        {
            DeleteObject(g_ui.hbrBkgnd);
            g_ui.hbrBkgnd = NULL;
        }
        break;
    default:
        result = DefWindowProc(hWnd, msg, wParam, lParam);
        break;
    }

    return result;
}

#define BORDERWIDTH 10

LRESULT CALLBACK WndProcRegion(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (msg)
    {
    case WM_MOVE:
    {
        if (g_ui.processWindowRegion) 
        {
            INT xPos = (int)(short)LOWORD(lParam);
            INT yPos = (int)(short)HIWORD(lParam);

            if (xPos >= 8) xPos = (xPos / 8) * 8;  
            else xPos = 0;

            if (yPos < 0) yPos = 0;

            if (g_ui.runtimeOptions)
            {
                g_ui.runtimeOptions->iCaptureX             = xPos;
                g_ui.runtimeOptions->iCaptureY             = yPos;
                g_ui.runtimeOptions->captureRegionChanged = true;

                SetWindowPos(g_ui.hWndRegion, HWND_TOPMOST, g_ui.runtimeOptions->iCaptureX, g_ui.runtimeOptions->iCaptureY, g_ui.runtimeOptions->iCaptureWidth, g_ui.runtimeOptions->iCaptureHeight, SWP_DRAWFRAME);
            }
        }
    }
    break;
    case WM_SIZE:
    {
        if (g_ui.processWindowRegion) 
        {
            INT width  = LOWORD(lParam);
            INT height = HIWORD(lParam);

            if (width < 64) width = 64;
            else width = (width / 8) * 8; 

            if (height < 64) height = 64;

            if (g_ui.runtimeOptions)
            {
                if ((width + g_ui.runtimeOptions->iCaptureX) < g_ui.maxWidth)
                    g_ui.runtimeOptions->iCaptureWidth = width;

                if ((height + g_ui.runtimeOptions->iCaptureY) < g_ui.maxHeight)
                    g_ui.runtimeOptions->iCaptureHeight = height;

                g_ui.runtimeOptions->captureRegionChanged = true;
                SetWindowPos(g_ui.hWndRegion, HWND_TOPMOST, g_ui.runtimeOptions->iCaptureX, g_ui.runtimeOptions->iCaptureY, g_ui.runtimeOptions->iCaptureWidth, g_ui.runtimeOptions->iCaptureHeight, SWP_DRAWFRAME);
            }
        }
    }
    break;
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(255, 255, 255));
        SetBkColor(hdcStatic, RGB(50, 50, 50));
        if (g_ui.hbrBkgndCaptureRegion == NULL)
        {
            g_ui.hbrBkgndCaptureRegion = CreateSolidBrush(RGB(50, 50, 50));
        }
        return (INT_PTR)g_ui.hbrBkgndCaptureRegion;
    }
    break;
    case WM_DESTROY:
        if (g_ui.hbrBkgndCaptureRegion != NULL)
        {
            DeleteObject(g_ui.hbrBkgndCaptureRegion);
            g_ui.hbrBkgndCaptureRegion = NULL;
        }
        break;
    case WM_NCHITTEST:
    {
        POINT pt;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);

        RECT rc;
        GetClientRect(hWnd, &rc);
        ScreenToClient(hWnd, &pt);

        if (pt.y < BORDERWIDTH)
        {
            if (pt.x < BORDERWIDTH) return HTTOPLEFT;
            else if (pt.x > (rc.right - BORDERWIDTH)) return HTTOPRIGHT;
            return HTTOP;
        }
        if (pt.y > (rc.bottom - BORDERWIDTH))
        {
            if (pt.x < BORDERWIDTH) return HTBOTTOMLEFT;
            else if (pt.x > (rc.right - BORDERWIDTH)) return HTBOTTOMRIGHT;
            return HTBOTTOM;
        }
        if (pt.x < BORDERWIDTH) return HTLEFT;
        if (pt.x > (rc.right - BORDERWIDTH)) return HTRIGHT;

        return HTCAPTION;
    }
    break;
    default:
        result = DefWindowProc(hWnd, msg, wParam, lParam);
        break;
    }
    return result;
}

void FLM_USER_INTERFACE::CloseUI()
{
    if (m_hUserInterfaceThread == NULL) return;

    if (g_ui.hWndUser)
    {
        PostMessage(g_ui.hWndUser, WM_QUIT, 0, 0);
        g_ui.hWndUser = NULL;
    }

    WaitForSingleObject(m_hUserInterfaceThread, INFINITE);
    CloseHandle(m_hUserInterfaceThread);
    m_hUserInterfaceThread = NULL;
}

void FLM_USER_INTERFACE::UpdateRunTimeOptions()
{
    if (g_ui.runtimeOptions == NULL) return;

    int printOption = std::clamp((int)g_ui.runtimeOptions->printLevel, 0, (int)FLM_PRINT_LEVEL::PRINT_LEVEL_COUNT - 1);

    SendMessage(g_ui.hWndLatencyDisplay[(int)(printOption)], BM_SETCHECK, TRUE, NULL);

    if (g_ui.runtimeOptions->mouseEventType == FLM_MOUSE_EVENT_TYPE::MOUSE_MOVE)
        SendMessage(g_ui.hWndMouseClick[0], BM_SETCHECK, TRUE, NULL);
    else
        SendMessage(g_ui.hWndMouseClick[1], BM_SETCHECK, TRUE, NULL);

    if (g_ui.hWndEditBias)
    {
        std::string str = FlmFormatStr("%3.1f", g_ui.runtimeOptions->biasOffset);
        SetWindowText(g_ui.hWndEditBias, str.c_str());
        SendMessage(g_ui.hWndEditBias, EM_SETREADONLY, g_ui.runtimeOptions->autoBias ? TRUE : FALSE, NULL);
    }

    if (g_ui.hWndEditThreshold)
    {
        float       threshold = g_ui.runtimeOptions->thresholdCoefficient[g_ui.runtimeOptions->mouseEventType];
        std::string str       = FlmFormatStr("%3.1f", threshold);
        SetWindowText(g_ui.hWndEditThreshold, str.c_str());
    }

    if (g_ui.hWndAutoBias)
        SendMessage(g_ui.hWndAutoBias, BM_SETCHECK, g_ui.runtimeOptions->autoBias ? TRUE : FALSE, NULL);

    if (g_ui.hWndMinimizeApp)
        SendMessage(g_ui.hWndMinimizeApp, BM_SETCHECK, g_ui.runtimeOptions->minimizeApp ? TRUE : FALSE, NULL);

    if (g_ui.hWndGameHasFG)
        SendMessage(g_ui.hWndGameHasFG, BM_SETCHECK, g_ui.runtimeOptions->gameUsesFrameGeneration ? TRUE : FALSE, NULL);
}

void FLM_USER_INTERFACE::Init(FLM_RUNTIME_OPTIONS* displayOption, HANDLE hUserInterfaceThread)
{
    if (!g_ui.hWndUser) return;
    m_hUserInterfaceThread = hUserInterfaceThread;
    g_ui.runtimeOptions = displayOption;
    UpdateRunTimeOptions();
    g_ui.runtimeOptions->captureRegionChanged = false;
}

void FLM_USER_INTERFACE::HideUI()
{
    if (!g_ui.hWndUser) return;
    if (g_ui.runtimeOptions) g_ui.runtimeOptions->showOptions = false;
    ShowWindow(g_ui.hWndUser, SW_HIDE);
    g_ui.capture_region_showing = false;
    ShowWindow(g_ui.hWndRegion, SW_HIDE);
    g_ui.ui_showing = false;
}

void FLM_USER_INTERFACE::ShowUI(int x, int y)
{
    if (!g_ui.hWndUser) return;
    SetWindowPos(g_ui.hWndUser, HWND_TOPMOST, x, y, FLM_OPTIONS_WINDOW_WIDTH, FLM_OPTIONS_WINDOW_HEIGHT, SWP_DRAWFRAME | SWP_SHOWWINDOW);
    SetForegroundWindow(g_ui.hWndUser);
    if (g_ui.runtimeOptions)
    {
        UpdateRunTimeOptions();
        g_ui.runtimeOptions->showOptions = true;
    }
    g_ui.ui_showing = true;
}

// ============================================================================
// 3. КАСТОМНИЙ КОД ДЛЯ ОВЕРЛЕЮ ТА ТОЧКА ВХОДУ (ThreadMain)
// ============================================================================

LRESULT CALLBACK WndProcOverlay(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_USER_UPDATE_LATENCY:
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        // Малюємо чорний фон, який стане прозорим завдяки LWA_COLORKEY
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
        return 1;
    }
case WM_PAINT:
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    if (hdc)
    {
        SetBkMode(hdc, TRANSPARENT);
        
        // --- ШРИФТ ---
        // 28 - розмір, Consolas - назва шрифту (можна змінити на Arial або Segoe UI)
        HFONT hFont = CreateFontA(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                                  OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                                  VARIABLE_PITCH, "Consolas");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        char latencyStr[128];
        snprintf(latencyStr, sizeof(latencyStr), "Latency: %.1f ms", g_LatestLatencyMs);
        
        std::string fgStatus = (g_ui.runtimeOptions && g_ui.runtimeOptions->gameUsesFrameGeneration) ? " [FG: ON]" : " [FG: OFF]";
        std::string finalText = std::string(latencyStr) + fgStatus;

        // --- ЕФЕКТ ТІНІ (малюємо чорний текст зі зміщенням) ---
        SetTextColor(hdc, RGB(0, 0, 0)); // Чорний колір для тіні
        TextOutA(hdc, 12, 12, finalText.c_str(), (int)finalText.length());

        // --- ОСНОВНИЙ КОЛІР ТЕКСТУ ---
        // Виберіть свій колір тут:
        // RGB(0, 255, 0)   - Яскраво-зелений
        // RGB(255, 255, 0) - Жовтий
        // RGB(0, 200, 255) - Неоново-блакитний
        SetTextColor(hdc, RGB(64, 224, 208)); 
        TextOutA(hdc, 52, 27, finalText.c_str(), (int)finalText.length());
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
    EndPaint(hWnd, &ps);
    break;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowOverlay(HINSTANCE hInst)
{
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProcOverlay;
    wc.hInstance = hInst;
    wc.lpszClassName = "FLM_Overlay_Class";
    RegisterClassExA(&wc);

    g_hWndOverlay = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        "FLM_Overlay_Class", "FLM Overlay",
        WS_POPUP,
        50, 50, 450, 80, 
        NULL, NULL, hInst, NULL
    );

    if (g_hWndOverlay)
    {
        // ПАРАМЕТРИ:
        // RGB(0, 0, 0) - колір, який стане ПОВНІСТЮ прозорим (фоновий чорний)
        // 200 - загальна прозорість всього вікна (від 0 до 255). 
        //       255 - зовсім непрозорий, 128 - напівпрозорий.
        // LWA_COLORKEY | LWA_ALPHA - прапорці, що вмикають обидва режими
        
        BYTE opacity = 180; // Зробимо його злегка напівпрозорим для стилю
        SetLayeredWindowAttributes(g_hWndOverlay, RGB(255, 255, 0), opacity, LWA_COLORKEY | LWA_ALPHA);
        
        SetWindowPos(g_hWndOverlay, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        ShowWindow(g_hWndOverlay, SW_SHOW);
    }
}

int WINAPI FLM_USER_INTERFACE::ThreadMain()
{
    TCHAR szClassName[] = _T("FLM User Interface");
    MSG   msg           = {};
    HINSTANCE hIns      = GetModuleHandle(NULL);

    // Реєстрація та створення головного вікна налаштувань (UI)
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpszClassName = szClassName;
    wc.lpfnWndProc   = WndProcUI;
    wc.style         = CS_DROPSHADOW;
    wc.hInstance     = hIns;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
    RegisterClassEx(&wc);

    g_ui.hWndUser = CreateWindowEx(0, szClassName, szClassName, WS_POPUP | WS_BORDER, 0, 0, FLM_OPTIONS_WINDOW_WIDTH, FLM_OPTIONS_WINDOW_HEIGHT, NULL, 0, hIns, 0);
    
    // Створення вікна вибору регіону захоплення
    TCHAR szClassNameRegion[] = _T("FLM Capture Region");
    WNDCLASSEX wcR = {0};
    wcR.cbSize = sizeof(WNDCLASSEX);
    wcR.lpszClassName = szClassNameRegion;
    wcR.lpfnWndProc   = WndProcRegion;
    wcR.hInstance     = hIns;
    wcR.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(50, 50, 50));
    RegisterClassEx(&wcR);
    g_ui.hWndRegion = CreateWindowEx(0, szClassNameRegion, szClassNameRegion, WS_POPUP, 0, 0, 0, 0, NULL, 0, hIns, 0);

    // ЗАПУСК ОВЕРЛЕЮ
    ShowOverlay(hIns);

    // Цикл повідомлень
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}