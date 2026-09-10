#ifndef UNICODE
#define UNICODE
#endif 

#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <wincodec.h>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "GameState.h"
#include "Renderer.h"

#pragma comment(lib, "d2d1")
#pragma comment(lib, "windowscodecs")
#pragma comment(lib, "ole32")
#pragma comment(lib, "user32")

const double frameTime = 1.0 / 60.0;
GameState gameState;
Renderer renderer;
HCURSOR cursor = LoadCursorFromFile(L"assets/cursor.cur");

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    
    // WIC factory
    CoInitialize(nullptr);

    // Register the window class.
    const wchar_t CLASS_NAME[]  = L"The game main";
    
    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Initialize renderer
    renderer.init();

    // Create the window.

    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"The game",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        0, 0, width, height,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg = { };
    double accumulator = 0.0;

    auto previous = std::chrono::steady_clock::now();

    while (true)
    {
        // Process Windows messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                return 0;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Measure elapsed real time
        auto current = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(current - previous).count();
        previous = current;

        accumulator += dt;

        // Update at a fixed 60 FPS
        while (accumulator >= frameTime)
        {
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(hwnd, &p);

            gameState.run_frame(p.y, p.x);

            accumulator -= frameTime;
        }

        // Draw
        InvalidateRect(hwnd, nullptr, FALSE);

        // Avoid using 100% CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {

    case WM_CREATE:
        {

            std::vector<int> data = renderer.on_wm_create(hwnd);

            gameState.init({data[0], data[1], data[2], data[3]}, {data[4], data[5]});

            return 0;
        }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        renderer.render(&gameState);

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_KEYDOWN:
        gameState.key_down(wParam);
        return 0;

    case WM_KEYUP:
        gameState.key_up(wParam);
        return 0;

    case WM_LBUTTONDOWN: {
        gameState.on_lbuttondown();
        return 0;
    }

    case WM_LBUTTONUP: {
        gameState.on_lbuttonup();
        return 0;
    }

    case WM_SETCURSOR: {
        SetCursor(cursor);
        return 0;
    }

    case WM_DESTROY:
        renderer.on_wm_destroy();
        gameState.end_game();
        CoUninitialize();
        
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}