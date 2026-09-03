#pragma once
#pragma comment(lib, "dwrite")

#include <d2d1.h>
#include <wincodec.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cmath>

class Renderer {
    public:
        ID2D1Factory *pFactory = nullptr;
        ID2D1HwndRenderTarget *pRenderTarget = nullptr;
        ID2D1Bitmap *pBitmap = nullptr;
        ID2D1Bitmap *eBitmap = nullptr;
        IWICBitmapDecoder *decoder = nullptr;
        IWICImagingFactory *wicFactory = nullptr;
        IWICBitmapFrameDecode *frame = nullptr;
        IWICFormatConverter *converter = nullptr;
        IDWriteFactory *pWriteFactory = nullptr;
        IDWriteTextFormat *pTextFormat = nullptr;

        void init() {
            // Direct2D factory
            D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                &pFactory
            );
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&pWriteFactory)
            );

            pWriteFactory->CreateTextFormat(
                L"Consolas",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                20.0f,
                L"",
                &pTextFormat
            );
        }

        void load_image(const wchar_t* filename, ID2D1Bitmap** bitmap) {

            wicFactory->CreateDecoderFromFilename(
                filename, nullptr, GENERIC_READ,
                WICDecodeMetadataCacheOnLoad, &decoder
            );

            HRESULT hr = decoder->GetFrame(0, &frame);

            if (FAILED(hr)) {
                std::cout << "GetFrame failed: 0x"
                        << std::hex << hr << std::dec << "\n";
            }

            wicFactory->CreateFormatConverter(&converter);

            converter->Initialize(
                frame,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr, 0.0,
                WICBitmapPaletteTypeMedianCut
            );

            pRenderTarget->CreateBitmapFromWicBitmap(
                converter, nullptr, bitmap
            );

            converter->Release();
            frame->Release();
            decoder->Release();
            converter = nullptr;
            frame = nullptr;
            decoder = nullptr;
        }

        std::vector<int> on_wm_create(HWND hwnd) {
            // Create Direct2D render target
            RECT rect;
            GetClientRect(hwnd, &rect);

            pFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(
                    hwnd,
                    D2D1::SizeU(rect.right, rect.bottom)
                ),
                &pRenderTarget
            );

            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory)
            );

            // Load PNG
            load_image(L"assets/main_character.png", &pBitmap);
            load_image(L"assets/enemy1.png", &eBitmap);

            // Initialize game state
            D2D1_SIZE_F size = pRenderTarget->GetSize();
            D2D1_SIZE_F imageSize = pBitmap->GetSize();

            int y = (size.width - imageSize.width) / 2;
            int x = (size.height - imageSize.height) / 2;

            wicFactory->Release();

            return {(int)x, (int)y, (int)(x + imageSize.height), (int)(y + imageSize.width), (int)size.height / 2, (int)size.width / 2};

        }

        void on_wm_paint(
            const std::vector<int>& player_render,
            const std::vector<std::vector<int>>& walls,
            const std::vector<int>& shift,
            const int x,
            const int y,
            const int render_distance,
            const std::vector<int> current_chunk,
            const int center_x,
            const int center_y,
            const int attack_time,
            const int current_attack_frame,
            const float attack_angle,
            const int range,
            const float cone_angle
        ) {
            pRenderTarget->BeginDraw();

            // Background
            pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

            // Draw not walls
            ID2D1SolidColorBrush* brush;
            pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                &brush
            );

            for (
                int i = (std::max)(0, current_chunk[0]-render_distance);
                i < (std::min)((int)walls.size(), current_chunk[0]+render_distance);
                i++
            ) {
                for (
                    int j = (std::max)(0, current_chunk[1]-render_distance);
                    j < (std::min)((int)walls[i].size(), current_chunk[1]+render_distance);
                    j++
                ) {
                    int wall = walls[i][j];
                    // ground
                    if (wall == 0 || wall == 2 || wall == 3) {
                        pRenderTarget->FillRectangle(
                            D2D1::RectF(
                                100 * j + shift[1] - y,  // ymin
                                100 * i + shift[0] - x,  // xmin
                                100 * (j + 1) + shift[1] - y,   // ymax
                                100 * (i + 1) + shift[0] - x  // xmax
                            ),
                            brush
                        );
                    }
                    // enemy
                    else if (wall == 4) {
                        pRenderTarget->DrawBitmap(
                            eBitmap,
                            D2D1::RectF(
                                100 * j + shift[1] - y,  // ymin
                                100 * i + shift[0] - x,  // xmin
                                100 * (j + 1) + shift[1] - y,   // ymax
                                100 * (i + 1) + shift[0] - x  // xmax
                            )
                        );
                    }
                }
            }

            // Draw player
            pRenderTarget->DrawBitmap(
                pBitmap,
                D2D1::RectF(
                    player_render[1],
                    player_render[0],
                    player_render[3],
                    player_render[2]
                )
            );

            // Draw attack
            if (current_attack_frame >= 0 && current_attack_frame <= attack_time / 3) {
                brush->SetColor(D2D1::ColorF(D2D1::ColorF::Green));

                int attack_percentage = current_attack_frame * 300 / attack_time;
                float angle = attack_angle + cone_angle * (attack_percentage - 50) / 100;
                int xbeg = center_x + 60 * std::sin(angle);
                int ybeg = center_y + 60 * std::cos(angle);
                int xend = center_x + range * std::sin(angle);
                int yend = center_y + range * std::cos(angle);
                //std::cout << "Renderer draw attack at angle " << angle << '\n';
                pRenderTarget->DrawLine(
                    D2D1::Point2F(ybeg, xbeg),
                    D2D1::Point2F(yend, xend),
                    brush,
                    5.0f
                );
            }

            // Show stats
            brush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));

            std::wstring text =
                L"x, y: " + std::to_wstring(x) + L", " + std::to_wstring(y) +
                L"\nattack: " + std::to_wstring(current_attack_frame) + L" / " + std::to_wstring(attack_time);

            D2D1_RECT_F textRect = D2D1::RectF(
                10.0f, 10.0f,
                400.0f, 200.0f
            );

            pRenderTarget->DrawText(
                text.c_str(),
                static_cast<UINT32>(text.size()),
                pTextFormat,
                textRect,
                brush
            );

            brush->Release();

            pRenderTarget->EndDraw();
        }

        void on_wm_destroy() {
            if (pBitmap) pBitmap->Release();
            if (eBitmap) eBitmap->Release();
            if (pRenderTarget) pRenderTarget->Release();
            if (pFactory) pFactory->Release();
            if (pTextFormat) pTextFormat->Release();
            if (pWriteFactory) pWriteFactory->Release();
        }
};

extern Renderer renderer;