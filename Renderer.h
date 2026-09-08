#pragma once
#pragma comment(lib, "dwrite")

#include <d2d1.h>
#include <wincodec.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "GameData.h"

class Renderer {
    public:
        ID2D1Factory *pFactory = nullptr;
        ID2D1HwndRenderTarget *pRenderTarget = nullptr;
        ID2D1Bitmap *playerBitmap = nullptr;
        ID2D1Bitmap *enemyBitmap = nullptr;
        ID2D1Bitmap *portalBitmap = nullptr;
        ID2D1Bitmap *bossBitmap = nullptr;
        IWICBitmapDecoder *decoder = nullptr;
        IWICImagingFactory *wicFactory = nullptr;
        IWICBitmapFrameDecode *frame = nullptr;
        IWICFormatConverter *converter = nullptr;
        IDWriteFactory *pWriteFactory = nullptr;
        IDWriteTextFormat *pTextFormat = nullptr;
        ID2D1SolidColorBrush* brush = nullptr;
        Colors colors;
        std::vector<float> blood_angles;
        int blood_length;

        void init() {
            // Factories
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

            // Blood angles and length
            blood_angles.reserve(20);
            for (int i = 0; i < 20; i++) {
                // 2 * pi * i / 20
                blood_angles.push_back(6.28319f * i / 20);
            }
            blood_length = 15;
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
            
            // Create brush
            pRenderTarget->CreateSolidColorBrush(
                colors.background,
                &brush
            );

            CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&wicFactory)
            );

            // Load PNG
            load_image(L"assets/main_character.png", &playerBitmap);
            load_image(L"assets/enemy1.png", &enemyBitmap);
            load_image(L"assets/portal.png", &portalBitmap);
            load_image(L"assets/boss.png", &bossBitmap);

            // Initialize game state
            D2D1_SIZE_F size = pRenderTarget->GetSize();
            D2D1_SIZE_F imageSize = playerBitmap->GetSize();

            int y = (size.width - imageSize.width) / 2;
            int x = (size.height - imageSize.height) / 2;

            wicFactory->Release();

            return {(int)x, (int)y, (int)(x + imageSize.height), (int)(y + imageSize.width), (int)size.height / 2, (int)size.width / 2};

        }

        void render_begin() {
            pRenderTarget->BeginDraw();

            // Background
            pRenderTarget->Clear(colors.background);
        }

        void render_walls(
            const std::vector<std::vector<int>>& walls,
            const RenderStats* render_stats,
            const int x,
            const int y,
            const int current_chunk_i,
            const int current_chunk_j,
            const int chunk_size
        ) {
            // Draw not walls
            brush->SetColor(colors.ground);

            for (
                int i = (std::max)(0, current_chunk_i - render_stats->render_distance);
                i < (std::min)((int)walls.size(), current_chunk_i + render_stats->render_distance);
                i++
            ) {
                for (
                    int j = (std::max)(0, current_chunk_j - render_stats->render_distance);
                    j < (std::min)((int)walls[i].size(), current_chunk_j + render_stats->render_distance);
                    j++
                ) {
                    int wall = walls[i][j];
                    // ground
                    if (wall == 0 || wall == 2) {
                        pRenderTarget->FillRectangle(
                            D2D1::RectF(
                                chunk_size * j + render_stats->shift[1] - y,  // ymin
                                chunk_size * i + render_stats->shift[0] - x,  // xmin
                                chunk_size * (j + 1) + render_stats->shift[1] - y,   // ymax
                                chunk_size * (i + 1) + render_stats->shift[0] - x  // xmax
                            ),
                            brush
                        );
                    }
                    // portal
                    else if (wall == 3) {
                        pRenderTarget->DrawBitmap(
                            portalBitmap,
                            D2D1::RectF(
                                chunk_size * j + render_stats->shift[1] - y,  // ymin
                                chunk_size * i + render_stats->shift[0] - x,  // xmin
                                chunk_size * (j + 1) + render_stats->shift[1] - y,   // ymax
                                chunk_size * (i + 1) + render_stats->shift[0] - x  // xmax
                            )
                        );
                    }
                    // enemy
                    else if (wall == 4) {
                        pRenderTarget->DrawBitmap(
                            enemyBitmap,
                            D2D1::RectF(
                                chunk_size * j + render_stats->shift[1] - y,  // ymin
                                chunk_size * i + render_stats->shift[0] - x,  // xmin
                                chunk_size * (j + 1) + render_stats->shift[1] - y,   // ymax
                                chunk_size * (i + 1) + render_stats->shift[0] - x  // xmax
                            )
                        );
                    }
                }
            }
        }

        void render_player(
            const RenderStats* render_stats
        ) {
            pRenderTarget->DrawBitmap(
                playerBitmap,
                D2D1::RectF(
                    render_stats->player_render[1],
                    render_stats->player_render[0],
                    render_stats->player_render[3],
                    render_stats->player_render[2]
                )
            );
        }

        void render_boss(
            const RenderStats* render_stats,
            const int x,
            const int y,
            const Boss *boss
        ) {
            if (!boss) return;
            
            // Draw boss
            pRenderTarget->DrawBitmap(
                bossBitmap,
                D2D1::RectF(
                    boss->y - boss->half_width + render_stats->shift[1] - y,
                    boss->x - boss->half_height + render_stats->shift[0] - x,
                    boss->y + boss->half_width + render_stats->shift[1] - y,
                    boss->x + boss->half_height + render_stats->shift[0] - x
                )
            );

            // Draw dialogue
            if (boss->dialogue.size()) {
                brush->SetColor(colors.background);

                D2D1_RECT_F textRect = D2D1::RectF(
                    boss->y + render_stats->shift[1] - y,
                    boss->x - 20.0f - boss->half_height + render_stats->shift[0] - x,
                    boss->y + 200.0f + render_stats->shift[1] - y,
                    boss->x - boss->half_height + render_stats->shift[0] - x
                );

                pRenderTarget->DrawText(
                    boss->dialogue.c_str(),
                    static_cast<UINT32>(boss->dialogue.size()),
                    pTextFormat,
                    textRect,
                    brush
                );
            }
        }

        void render_projectiles(
            const RenderStats* render_stats,
            const int x,
            const int y,
            std::unordered_map<int, std::vector<Projectile*>>& projectiles,
            const int current_chunk_i,
            const int current_chunk_j,
            const int max_chunks_i,
            const int max_chunks_j
        ) {
            for (
                int i = (std::max)(0, current_chunk_i - render_stats->render_distance);
                i < (std::min)(max_chunks_i, current_chunk_i + render_stats->render_distance);
                i++
            ) {
                for (
                    int j = (std::max)(0, current_chunk_j - render_stats->render_distance);
                    j < (std::min)(max_chunks_j, current_chunk_j + render_stats->render_distance);
                    j++
                ) {
                    const int key = i * max_chunks_j + j;
                    if (projectiles.find(key) == projectiles.end()) continue;
                    for (Projectile *proj : projectiles[key]) {

                        if (proj->damage_type == 'F') brush->SetColor(colors.fire);
                        if (proj->damage_type == 'C') brush->SetColor(colors.cold);
                        if (proj->damage_type == 'P') brush->SetColor(colors.physical);
                        pRenderTarget->FillEllipse(
                            D2D1::Ellipse(
                                D2D1::Point2F((int)proj->y + render_stats->shift[1] - y, (int)proj->x + render_stats->shift[0] - x),  // center
                                proj->radius,
                                proj->radius
                            ),
                            brush
                        );
                    }
                }
            }
        }

        void render_attack(
            const RenderStats* render_stats,
            const Attack* attack
        ) {

            // Draw attack
            if (attack->attacking) {
                brush->SetColor(colors.physical);
                pRenderTarget->DrawLine(
                    D2D1::Point2F(render_stats->center_y + attack->ybeg, render_stats->center_x + attack->xbeg),
                    D2D1::Point2F(render_stats->center_y + attack->yend, render_stats->center_x + attack->xend),
                    brush,
                    5.0f
                );
            }
        }

        void render_taking_damage(
            const RenderStats* render_stats,
            const int invulnerability_frame
        ) {
            if (invulnerability_frame == -1) return;
            // draw blood at dist from center
            brush->SetColor(colors.blood);
            int dist = 20 + 2 * invulnerability_frame;
            for (float angle : blood_angles) {
                int xbeg = render_stats->center_x + dist * std::sin(angle);
                int ybeg = render_stats->center_y + dist * std::cos(angle);
                int xend = render_stats->center_x + (dist + blood_length) * std::sin(angle);
                int yend = render_stats->center_y + (dist + blood_length) * std::cos(angle);
                pRenderTarget->DrawLine(
                    D2D1::Point2F(ybeg, xbeg),
                    D2D1::Point2F(yend, xend),
                    brush,
                    3.0f
                );
            }
        }

        void render_boss_life(
            const RenderStats* render_stats,
            const Boss *boss
        ) {
            if (!boss) return;

            // outline
            brush->SetColor(colors.boss_life_outline);
            pRenderTarget->DrawRectangle(
                D2D1::RectF(
                    render_stats->boss_life_bar_ymin,
                    20.0f,
                    render_stats->boss_life_bar_ymax,
                    40.0f
                ),
                brush,
                3.0f
            );

            // fill
            brush->SetColor(colors.boss_life_fill);
            float life_ratio =  boss->life * 1.0f / boss->max_life;
            int ymax = render_stats->boss_life_bar_ymin + life_ratio *
                (render_stats->boss_life_bar_ymax - render_stats->boss_life_bar_ymin);
            pRenderTarget->FillRectangle(
                D2D1::RectF(
                    render_stats->boss_life_bar_ymin + 2,
                    22.0f,
                    ymax - 2,
                    38.0f
                ),
                brush
            );

        }

        void render_stats(
            const int life,
            const int max_life
        ) {
            // Show stats
            brush->SetColor(colors.stats);

            std::wstring text =
                L"\nlife: " + std::to_wstring(life) + L"/ " + std::to_wstring(max_life);

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
        }

        void render_dead(
            const RenderStats* render_stats
        ) {
            brush->SetColor(colors.stats);
            std::wstring text = L"DEAD";

            D2D1_RECT_F textRect = D2D1::RectF(
                render_stats->center_y - 30.0f, render_stats->center_x - 5.0f,
                render_stats->center_y + 30.0f, render_stats->center_x + 5.0f
            );

            pRenderTarget->DrawText(
                text.c_str(),
                static_cast<UINT32>(text.size()),
                pTextFormat,
                textRect,
                brush
            );
        }

        void render_end() {
            pRenderTarget->EndDraw();
        }

        void on_wm_destroy() {
            if (playerBitmap) playerBitmap->Release();
            if (enemyBitmap) enemyBitmap->Release();
            if (pRenderTarget) pRenderTarget->Release();
            if (pFactory) pFactory->Release();
            if (pTextFormat) pTextFormat->Release();
            if (pWriteFactory) pWriteFactory->Release();
            if (brush) brush->Release();
        }
};

extern Renderer renderer;