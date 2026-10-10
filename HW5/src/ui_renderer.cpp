#include "ui_renderer.h"
// microui.h included via ui_renderer.h
#include <string.h>
#include <algorithm>
#include <cmath>
#include <cstdio>   
#include <cfloat> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
extern int enable_distortion;
// Minimal 8x8 bitmap font for ASCII characters 32-126
// Each byte represents a row of 8 pixels
static const uint8_t font_8x8[95][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x18, 0x3c, 0x3c, 0x18, 0x18, 0x00, 0x18, 0x00}, // !
    {0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    {0x6c, 0x6c, 0xfe, 0x6c, 0xfe, 0x6c, 0x6c, 0x00}, // #
    {0x18, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x18, 0x00}, // $
    {0x00, 0x6c, 0xda, 0x30, 0x58, 0xb6, 0x36, 0x00}, // %
    {0x38, 0x6c, 0x6c, 0x38, 0x76, 0x6c, 0x3b, 0x00}, // &
    {0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    {0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00}, // (
    {0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00}, // )
    {0x00, 0x6c, 0x38, 0xfe, 0x38, 0x6c, 0x00, 0x00}, // *
    {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00}, // +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30}, // ,
    {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00}, // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // .
    {0x02, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x40, 0x00}, // /
    {0x3c, 0x66, 0x6e, 0x76, 0x66, 0x66, 0x3c, 0x00}, // 0
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7e, 0x00}, // 1
    {0x3c, 0x66, 0x06, 0x0c, 0x18, 0x30, 0x7e, 0x00}, // 2
    {0x3c, 0x66, 0x06, 0x1c, 0x06, 0x66, 0x3c, 0x00}, // 3
    {0x1c, 0x3c, 0x6c, 0xcc, 0xfe, 0x0c, 0x1e, 0x00}, // 4
    {0x7e, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00}, // 5
    {0x3c, 0x66, 0x60, 0x7c, 0x66, 0x66, 0x3c, 0x00}, // 6
    {0x7e, 0x66, 0x0c, 0x18, 0x18, 0x18, 0x18, 0x00}, // 7
    {0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x3c, 0x00}, // 8
    {0x3c, 0x66, 0x66, 0x3e, 0x06, 0x66, 0x3c, 0x00}, // 9
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, // :
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30, 0x00}, // ;
    {0x0c, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0c, 0x00}, // <
    {0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00}, // =
    {0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x30, 0x00}, // >
    {0x3c, 0x66, 0x06, 0x0c, 0x18, 0x00, 0x18, 0x00}, // ?
    {0x3c, 0x66, 0x6e, 0x6e, 0x60, 0x62, 0x3c, 0x00}, // @
    {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00}, // A
    {0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x7c, 0x00}, // B
    {0x3c, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00}, // C
    {0x78, 0x6c, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00}, // D
    {0x7e, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7e, 0x00}, // E
    {0x7e, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00}, // F
    {0x3c, 0x66, 0x60, 0x6e, 0x66, 0x66, 0x3c, 0x00}, // G
    {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00}, // H
    {0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // I
    {0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0xcc, 0x78, 0x00}, // J
    {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00}, // K
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00}, // L
    {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00}, // M
    {0x66, 0x66, 0x76, 0x7e, 0x3e, 0x66, 0x66, 0x00}, // N
    {0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}, // O
    {0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00}, // P
    {0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x0e, 0x00}, // Q
    {0x7c, 0x66, 0x66, 0x7c, 0x78, 0x6c, 0x66, 0x00}, // R
    {0x3c, 0x66, 0x30, 0x18, 0x0c, 0x66, 0x3c, 0x00}, // S
    {0x7e, 0x5a, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}, // U
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // V
    {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}, // W
    {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00}, // X
    {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00}, // Y
    {0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00}, // Z
    {0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00}, // [
    {0x40, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x02, 0x00}, // backslash
    {0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00}, // ]
    {0x10, 0x38, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00}, // ^
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff}, // _
    {0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // `
    {0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3e, 0x00}, // a
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x00}, // b
    {0x00, 0x00, 0x3c, 0x60, 0x60, 0x66, 0x3c, 0x00}, // c
    {0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00}, // d
    {0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00}, // e
    {0x1c, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x00}, // f
    {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x7c}, // g
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // h
    {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00}, // i
    {0x06, 0x00, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00}, // j
    {0x60, 0x60, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0x00}, // k
    {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // l
    {0x00, 0x00, 0x66, 0xff, 0xdb, 0xdb, 0xdb, 0x00}, // m
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // n
    {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00}, // o
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60}, // p
    {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x06}, // q
    {0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x00}, // r
    {0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00}, // s
    {0x30, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x1c, 0x00}, // t
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00}, // u
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // v
    {0x00, 0x00, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}, // w
    {0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00}, // x
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x7c}, // y
    {0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x7e, 0x00}, // z
    {0x0c, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0c, 0x00}, // {
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // |
    {0x30, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x30, 0x00}, // }
    {0x00, 0x00, 0x00, 0x76, 0xdc, 0x00, 0x00, 0x00}, // ~
};

UIRenderer::UIRenderer(int width, int height) : m_width(width), m_height(height), m_buffer(nullptr) {
    m_clip_rect = { 0, 0, width, height };
}

void UIRenderer::render(mu_Context* ctx, uint32_t* buffer) {
    m_buffer = buffer;
    mu_Command* cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
        switch (cmd->type) {
        case MU_COMMAND_RECT: draw_rect(cmd->rect.rect, cmd->rect.color); break;
        case MU_COMMAND_TEXT: draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_ICON: draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
        case MU_COMMAND_CLIP: set_clip_rect(cmd->clip.rect); break;
        }
    }
}

void UIRenderer::draw_rect(mu_Rect rect, mu_Color color) {
    uint32_t c = to_uint32(color);

    //moves entire UI
    int offset_x = enable_distortion ? 50 : 0;
    int offset_y = enable_distortion ? 30 : 0;

    int x1 = std::max({ rect.x + offset_x, m_clip_rect.x, 0 });
    int y1 = std::max({ rect.y + offset_y, m_clip_rect.y, 0 });
    int x2 = std::min({ rect.x + offset_x + rect.w, m_clip_rect.x + m_clip_rect.w, m_width });
    int y2 = std::min({ rect.y + offset_y + rect.h, m_clip_rect.y + m_clip_rect.h, m_height });

    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            m_buffer[y * m_width + x] = c;
        }
    }
}

void UIRenderer::draw_text(const char* text, mu_Vec2 pos, mu_Color color) {
    uint32_t c = to_uint32(color);

    //Same translation on text
    int offset_x = enable_distortion ? 50 : 0;
    int offset_y = enable_distortion ? 30 : 0;

    int x = pos.x + offset_x;
    int y = pos.y + offset_y;

    for (const char* p = text; *p; p++) {
        if (*p < 32 || *p > 126) continue;
        const uint8_t* glyph = font_8x8[*p - 32];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (glyph[row] & (0x80 >> col)) {
                    int px = x + col;
                    int py = y + row;
                    if (px >= m_clip_rect.x && px < m_clip_rect.x + m_clip_rect.w &&
                        py >= m_clip_rect.y && py < m_clip_rect.y + m_clip_rect.h &&
                        px >= 0 && px < m_width && py >= 0 && py < m_height) {
                        m_buffer[py * m_width + px] = c;
                    }
                }
            }
        }
        x += 8;
    }
}

void UIRenderer::draw_pixel(int x, int y, uint32_t c) {
    if (x < m_clip_rect.x || x >= m_clip_rect.x + m_clip_rect.w) return;
    if (y < m_clip_rect.y || y >= m_clip_rect.y + m_clip_rect.h) return;
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    m_buffer[y * m_width + x] = c;
}

void UIRenderer::draw_line(int x0, int y0, int x1, int y1, uint32_t c) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        draw_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void UIRenderer::draw_circle(int xc, int yc, int radius, uint32_t color) {
    if (radius <= 0) return;

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;  // Decision parameter

    while (y >= x) {
        // Draw eight symmetric pixels
        draw_pixel(xc + x, yc + y, color);
        draw_pixel(xc - x, yc + y, color);
        draw_pixel(xc + x, yc - y, color);
        draw_pixel(xc - x, yc - y, color);
        draw_pixel(xc + y, yc + x, color);
        draw_pixel(xc - y, yc + x, color);
        draw_pixel(xc + y, yc - x, color);
        draw_pixel(xc - y, yc - x, color);

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else {
            d = d + 4 * x + 6;
        }
    }
}
void UIRenderer::draw_line_naive(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    // Handle degenerate case (single point)
    if (dx == 0 && dy == 0) {
        draw_pixel(x0, y0, color);
        return;
    }

    // Vertical line
    if (dx == 0) {
        int y_start = (y0 < y1) ? y0 : y1;
        int y_end = (y0 < y1) ? y1 : y0;
        for (int y = y_start; y <= y_end; y++) {
            draw_pixel(x0, y, color);
        }
        return;
    }

    // Horizontal line
    if (dy == 0) {
        int x_start = (x0 < x1) ? x0 : x1;
        int x_end = (x0 < x1) ? x1 : x0;
        for (int x = x_start; x <= x_end; x++) {
            draw_pixel(x, y0, color);
        }
        return;
    }

    float m = (float)dy / (float)dx;
    float b = y0 - m * x0;

    if (abs(dx) >= abs(dy)) {
        // Shallow slope: iterate over x
        int x_start = (x0 < x1) ? x0 : x1;
        int x_end = (x0 < x1) ? x1 : x0;
        for (int x = x_start; x <= x_end; x++) {
            int y = (int)(m * x + b);
            draw_pixel(x, y, color);
        }
    }
    else {
        // Steep slope: iterate over y
        int y_start = (y0 < y1) ? y0 : y1;
        int y_end = (y0 < y1) ? y1 : y0;
        for (int y = y_start; y <= y_end; y++) {
            int x = (int)((y - b) / m);
            draw_pixel(x, y, color);
        }
    }
}
uint32_t UIRenderer::blend_pixel(uint32_t bg, uint32_t fg, float alpha) {
    if (alpha <= 0.0f) return bg;
    if (alpha >= 1.0f) return fg;

    uint8_t br = (bg >> 16) & 0xFF;
    uint8_t bg_ = (bg >> 8) & 0xFF;
    uint8_t bb = bg & 0xFF;

    uint8_t fr = (fg >> 16) & 0xFF;
    uint8_t fg_ = (fg >> 8) & 0xFF;
    uint8_t fb = fg & 0xFF;

    uint8_t r = (uint8_t)(br * (1.0f - alpha) + fr * alpha);
    uint8_t g = (uint8_t)(bg_ * (1.0f - alpha) + fg_ * alpha);
    uint8_t b = (uint8_t)(bb * (1.0f - alpha) + fb * alpha);

    return MFB_RGB(r, g, b);
}
void UIRenderer::draw_line_wu(int x0, int y0, int x1, int y1, uint32_t color) {
    // Handle vertical line (no anti-aliasing needed)
    if (x0 == x1) {
        int y_start = (y0 < y1) ? y0 : y1;
        int y_end = (y0 < y1) ? y1 : y0;
        for (int y = y_start; y <= y_end; y++) {
            draw_pixel(x0, y, color);
        }
        return;
    }

    // Handle horizontal line (no anti-aliasing needed)
    if (y0 == y1) {
        int x_start = (x0 < x1) ? x0 : x1;
        int x_end = (x0 < x1) ? x1 : x0;
        for (int x = x_start; x <= x_end; x++) {
            draw_pixel(x, y0, color);
        }
        return;
    }

    bool steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    float gradient = (float)dy / (float)dx;

    // First endpoint
    float y = y0 + gradient;
    float ipart;
    float fpart = modff(y, &ipart);
    int y_int = (int)y;

    // Draw endpoint pixels (two pixels with brightness)
    auto draw_pixel_alpha = [&](int x, int y, float alpha) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
        if (alpha <= 0.0f) return;

        uint32_t bg = m_buffer[y * m_width + x];
        // If background is black (0), blend with color directly
        if (bg == 0) {
            uint32_t blended = 0;
            uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * alpha);
            uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * alpha);
            uint8_t b = (uint8_t)((color & 0xFF) * alpha);
            m_buffer[y * m_width + x] = MFB_RGB(r, g, b);
        }
        else {
            m_buffer[y * m_width + x] = blend_pixel(bg, color, alpha);
        }
        };

    // Draw first endpoint
    if (steep) {
        draw_pixel_alpha(y_int, x0, 1.0f - fpart);
        draw_pixel_alpha(y_int + 1, x0, fpart);
    }
    else {
        draw_pixel_alpha(x0, y_int, 1.0f - fpart);
        draw_pixel_alpha(x0, y_int + 1, fpart);
    }

    // Main loop
    for (int x = x0 + 1; x < x1; x++) {
        fpart = modff(y, &ipart);
        y_int = (int)y;

        if (steep) {
            draw_pixel_alpha(y_int, x, 1.0f - fpart);
            draw_pixel_alpha(y_int + 1, x, fpart);
        }
        else {
            draw_pixel_alpha(x, y_int, 1.0f - fpart);
            draw_pixel_alpha(x, y_int + 1, fpart);
        }
        y += gradient;
    }

    // Last endpoint
    fpart = modff(y, &ipart);
    y_int = (int)y;

    if (steep) {
        draw_pixel_alpha(y_int, x1, 1.0f - fpart);
        draw_pixel_alpha(y_int + 1, x1, fpart);
    }
    else {
        draw_pixel_alpha(x1, y_int, 1.0f - fpart);
        draw_pixel_alpha(x1, y_int + 1, fpart);
    }
}
void UIRenderer::drawWireframe(const std::vector<Vertex3D>& vertices,
    const std::vector<Face3D>& faces,
    const glm::mat4& transform,
    uint32_t color) {
    for (const auto& face : faces) {
        // Get the three vertices
        glm::vec4 v0 = transform * glm::vec4(vertices[face.v0].position, 1.0f);
        glm::vec4 v1 = transform * glm::vec4(vertices[face.v1].position, 1.0f);
        glm::vec4 v2 = transform * glm::vec4(vertices[face.v2].position, 1.0f);

        // Perspective divide (if w != 1)
        v0 = v0 / v0.w;
        v1 = v1 / v1.w;
        v2 = v2 / v2.w;

        // Project to 2D with Y flipped
        int x0 = (int)(v0.x * (m_width / 2) + (m_width / 2));
        int y0 = (int)(m_height / 2 - v0.y * (m_height / 2));
        int x1 = (int)(v1.x * (m_width / 2) + (m_width / 2));
        int y1 = (int)(m_height / 2 - v1.y * (m_height / 2));
        int x2 = (int)(v2.x * (m_width / 2) + (m_width / 2));
        int y2 = (int)(m_height / 2 - v2.y * (m_height / 2));

        // Draw three edges
        draw_line(x0, y0, x1, y1, color);
        draw_line(x1, y1, x2, y2, color);
        draw_line(x2, y2, x0, y0, color);
    }
}
void UIRenderer::drawAxes(const glm::mat4& transform, float length, uint32_t colorX, uint32_t colorY, uint32_t colorZ) {
    glm::vec4 origin = transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 xEnd = transform * glm::vec4(length, 0.0f, 0.0f, 1.0f);
    glm::vec4 yEnd = transform * glm::vec4(0.0f, length, 0.0f, 1.0f);
    glm::vec4 zEnd = transform * glm::vec4(0.0f, 0.0f, length, 1.0f);

    // Perspective divide
    origin = origin / origin.w;
    xEnd = xEnd / xEnd.w;
    yEnd = yEnd / yEnd.w;
    zEnd = zEnd / zEnd.w;

    int ox = (int)(origin.x * (m_width / 2) + (m_width / 2));
    int oy = (int)(m_height / 2 - origin.y * (m_height / 2));
    int xEx = (int)(xEnd.x * (m_width / 2) + (m_width / 2));
    int xEy = (int)(m_height / 2 - xEnd.y * (m_height / 2));
    int yEx = (int)(yEnd.x * (m_width / 2) + (m_width / 2));
    int yEy = (int)(m_height / 2 - yEnd.y * (m_height / 2));
    int zEx = (int)(zEnd.x * (m_width / 2) + (m_width / 2));
    int zEy = (int)(m_height / 2 - zEnd.y * (m_height / 2));

    draw_line(ox, oy, xEx, xEy, colorX);
    draw_line(ox, oy, yEx, yEy, colorY);
    draw_line(ox, oy, zEx, zEy, colorZ);
}
void UIRenderer::drawBoundingBox(const glm::vec3& min, const glm::vec3& max, const glm::mat4& transform, uint32_t color) {
    glm::vec3 corners[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z},
        {max.x, min.y, max.z}, {min.x, min.y, max.z},
        {min.x, max.y, min.z}, {max.x, max.y, min.z},
        {max.x, max.y, max.z}, {min.x, max.y, max.z}
    };

    glm::vec4 screen[8];
    for (int i = 0; i < 8; i++) {
        glm::vec4 c = transform * glm::vec4(corners[i], 1.0f);
        // Perspective divide
        c = c / c.w;
        screen[i] = glm::vec4(
            c.x * (m_width / 2) + (m_width / 2),
            m_height / 2 - c.y * (m_height / 2),
            c.z,
            c.w
        );
    }

    // Draw 12 edges
    draw_line((int)screen[0].x, (int)screen[0].y, (int)screen[1].x, (int)screen[1].y, color);
    draw_line((int)screen[1].x, (int)screen[1].y, (int)screen[2].x, (int)screen[2].y, color);
    draw_line((int)screen[2].x, (int)screen[2].y, (int)screen[3].x, (int)screen[3].y, color);
    draw_line((int)screen[3].x, (int)screen[3].y, (int)screen[0].x, (int)screen[0].y, color);
    draw_line((int)screen[4].x, (int)screen[4].y, (int)screen[5].x, (int)screen[5].y, color);
    draw_line((int)screen[5].x, (int)screen[5].y, (int)screen[6].x, (int)screen[6].y, color);
    draw_line((int)screen[6].x, (int)screen[6].y, (int)screen[7].x, (int)screen[7].y, color);
    draw_line((int)screen[7].x, (int)screen[7].y, (int)screen[4].x, (int)screen[4].y, color);
    draw_line((int)screen[0].x, (int)screen[0].y, (int)screen[4].x, (int)screen[4].y, color);
    draw_line((int)screen[1].x, (int)screen[1].y, (int)screen[5].x, (int)screen[5].y, color);
    draw_line((int)screen[2].x, (int)screen[2].y, (int)screen[6].x, (int)screen[6].y, color);
    draw_line((int)screen[3].x, (int)screen[3].y, (int)screen[7].x, (int)screen[7].y, color);
}
void UIRenderer::drawFaceNormals(const std::vector<Vertex3D>& vertices,
    const std::vector<Face3D>& faces,
    const std::vector<glm::vec3>& normals,
    const glm::mat4& transform,
    float length,
    uint32_t color) {
    if (vertices.empty() || faces.empty() || normals.empty()) return;
    if (faces.size() != normals.size()) return;

    for (int i = 0; i < (int)faces.size(); i++) {
        const auto& face = faces[i];

        // Calculate face center
        glm::vec3 center = (vertices[face.v0].position +
            vertices[face.v1].position +
            vertices[face.v2].position) / 3.0f;

        // Transform center to world space
        glm::vec4 centerWorld = transform * glm::vec4(center, 1.0f);
        centerWorld = centerWorld / centerWorld.w;

        // Transform normal direction (using inverse transpose for correct rotation)
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(transform));
        glm::vec4 normalWorld = normalMatrix * glm::vec4(normals[i], 0.0f);
        normalWorld = glm::normalize(normalWorld);

        // End point of normal line
        glm::vec4 endWorld = centerWorld + normalWorld * length;

        // Project to screen
        int cx = (int)(centerWorld.x * (m_width / 2) + (m_width / 2));
        int cy = (int)(m_height / 2 - centerWorld.y * (m_height / 2));
        int ex = (int)(endWorld.x * (m_width / 2) + (m_width / 2));
        int ey = (int)(m_height / 2 - endWorld.y * (m_height / 2));

        draw_line(cx, cy, ex, ey, color);
    }
}
void UIRenderer::drawVertexNormals(const std::vector<Vertex3D>& vertices,
    const std::vector<glm::vec3>& normals,
    const glm::mat4& transform,
    float length,
    uint32_t color) {
    if (vertices.empty() || normals.empty()) return;
    if (vertices.size() != normals.size()) return;

    for (int i = 0; i < (int)vertices.size(); i++) {
        // Transform vertex to world space
        glm::vec4 posWorld = transform * glm::vec4(vertices[i].position, 1.0f);
        posWorld = posWorld / posWorld.w;

        // Transform normal direction
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(transform));
        glm::vec4 normalWorld = normalMatrix * glm::vec4(normals[i], 0.0f);
        normalWorld = glm::normalize(normalWorld);

        // End point of normal line
        glm::vec4 endWorld = posWorld + normalWorld * length;

        // Project to screen
        int sx = (int)(posWorld.x * (m_width / 2) + (m_width / 2));
        int sy = (int)(m_height / 2 - posWorld.y * (m_height / 2));
        int ex = (int)(endWorld.x * (m_width / 2) + (m_width / 2));
        int ey = (int)(m_height / 2 - endWorld.y * (m_height / 2));

        draw_line(sx, sy, ex, ey, color);
    }
}
bool UIRenderer::loadOBJ(const std::string& filename,
    std::vector<Vertex3D>& vertices,
    std::vector<Face3D>& faces) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) return false;

    vertices.clear();
    faces.clear();

    std::vector<glm::vec2> texCoords;   // temporary storage for vt

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            sscanf(line, "v %f %f %f", &x, &y, &z);
            vertices.push_back({ glm::vec3(x, y, z), glm::vec2(0,0) });
        }
        else if (line[0] == 'v' && line[1] == 't') {
            float u, v;
            sscanf(line, "vt %f %f", &u, &v);
            texCoords.push_back(glm::vec2(u, 1.0f - v)); // OBJ v is bottom-up, flip for OpenGL convention
        }
        else if (line[0] == 'f') {
            int v0, v1, v2;
            int t0 = -1, t1 = -1, t2 = -1;
            int n0, n1, n2;

            // Try parsing with texture and normal indices: f v/vt/vn
            int matches = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
                &v0, &t0, &n0, &v1, &t1, &n1, &v2, &t2, &n2);
            if (matches == 9) {
                // all good
            }
            else {
                // Try parsing with only texture: f v/vt
                matches = sscanf(line, "f %d/%d %d/%d %d/%d",
                    &v0, &t0, &v1, &t1, &v2, &t2);
                if (matches != 6) {
                    // Try simple vertices only: f v v v
                    matches = sscanf(line, "f %d %d %d", &v0, &v1, &v2);
                    if (matches == 3) {
                        faces.push_back({ v0 - 1, v1 - 1, v2 - 1 });
                        continue;
                    }
                }
            }

            // If we have texture indices, assign UVs to vertices (assuming 1‑based indexing)
            if (t0 > 0 && t1 > 0 && t2 > 0) {
                if (t0 - 1 < (int)texCoords.size()) vertices[v0 - 1].uv = texCoords[t0 - 1];
                if (t1 - 1 < (int)texCoords.size()) vertices[v1 - 1].uv = texCoords[t1 - 1];
                if (t2 - 1 < (int)texCoords.size()) vertices[v2 - 1].uv = texCoords[t2 - 1];
            }

            faces.push_back({ v0 - 1, v1 - 1, v2 - 1 });
        }
    }
    fclose(file);
    return true;
}
// Simple BMP loader (24‑bit uncompressed)
bool loadBMP(const char* filename, uint32_t*& outData, int& outWidth, int& outHeight) {
    FILE* file = fopen(filename, "rb");
    if (!file) return false;

    // BMP header (54 bytes)
    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) {
        fclose(file);
        return false;
    }

    // Check "BM" signature
    if (header[0] != 'B' || header[1] != 'M') {
        fclose(file);
        return false;
    }

    int width = *(int*)&header[18];
    int height = *(int*)&header[22];
    int bitsPerPixel = *(short*)&header[28];

    if (bitsPerPixel != 24) {
        fclose(file);
        return false;
    }

    // Row size is padded to multiple of 4 bytes
    int rowSize = (width * 3 + 3) & ~3;

    // Allocate pixel data (RGBA)
    outData = new uint32_t[width * height];
    outWidth = width;
    outHeight = height;

    unsigned char* rowData = new unsigned char[rowSize];
    for (int y = 0; y < height; y++) {
        fread(rowData, 1, rowSize, file);
        for (int x = 0; x < width; x++) {
            int i = y * width + x;
            int j = x * 3;
            uint8_t b = rowData[j];
            uint8_t g = rowData[j + 1];
            uint8_t r = rowData[j + 2];
            outData[i] = MFB_RGB(r, g, b); // store as 24‑bit
        }
    }

    delete[] rowData;
    fclose(file);
    return true;
}
void UIRenderer::draw_icon(int id, mu_Rect rect, mu_Color color) {
    uint32_t c = to_uint32(color);
    // Centre a square drawing area within the rect (max 14x14)
    int sz = std::min({ rect.w, rect.h, 14 });
    int cx = rect.x + rect.w / 2;
    int cy = rect.y + rect.h / 2;
    int h = sz / 2;

    switch (id) {
    case MU_ICON_CLOSE: {
        // X: two diagonal lines with 2px thickness
        for (int t = -1; t <= 1; t++) {
            draw_line(cx - h, cy - h + t, cx + h, cy + h + t, c);
            draw_line(cx + h, cy - h + t, cx - h, cy + h + t, c);
        }
        break;
    }
    case MU_ICON_CHECK: {
        // Checkmark: short upstroke then long upstroke
        int lx = cx - h + 1;
        int ly = cy + 1;
        for (int t = -1; t <= 1; t++) {
            draw_line(lx, ly + t, lx + h / 2, cy + h / 2 + t, c);
            draw_line(lx + h / 2, cy + h / 2 + t, cx + h, cy - h + t, c);
        }
        break;
    }
    case MU_ICON_COLLAPSED: {
        // Right-pointing filled triangle: base on left, tip on right
        for (int dx = -h; dx <= h; dx++) {
            int hh = (h - dx) / 2;
            draw_line(cx + dx, cy - hh, cx + dx, cy + hh, c);
        }
        break;
    }
    case MU_ICON_EXPANDED: {
        // Down-pointing filled triangle: base on top, tip on bottom
        for (int dy = -h; dy <= h; dy++) {
            int hw = (h - dy) / 2;
            draw_line(cx - hw, cy + dy, cx + hw, cy + dy, c);
        }
        break;
    }
    default:
        draw_rect(rect, color);
        break;
    }
}

void UIRenderer::set_clip_rect(mu_Rect rect) {
    m_clip_rect = rect;
}
//Compute barycentric coordinates for a point (x, y) inside triangle (v0, v1, v2)
glm::vec3 computeBarycentric(float x, float y, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2) {
    // Calculate area of the full triangle
    float denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);

    // Edge case: degenerate triangle (area is zero)
    if (fabs(denom) < 0.0001f) {
        return glm::vec3(1.0f, 0.0f, 0.0f);  // Default to first vertex
    }

    // Compute barycentric weights
    float alpha = ((v1.y - v2.y) * (x - v2.x) + (v2.x - v1.x) * (y - v2.y)) / denom;
    float beta = ((v2.y - v0.y) * (x - v2.x) + (v0.x - v2.x) * (y - v2.y)) / denom;
    float gamma = 1.0f - alpha - beta;

    return glm::vec3(alpha, beta, gamma);
}
void UIRenderer::drawFilledTriangle(const std::vector<Vertex3D>& vertices,
    const std::vector<Face3D>& faces,
    const glm::mat4& transform,
    uint32_t color,
    float* zBuffer,
    bool cullBackfaces,
    bool fillRules) {
    if (zBuffer == nullptr) return;

    for (const auto& face : faces) {
        // Get three vertices and transform them
        glm::vec4 v0 = transform * glm::vec4(vertices[face.v0].position, 1.0f);
        glm::vec4 v1 = transform * glm::vec4(vertices[face.v1].position, 1.0f);
        glm::vec4 v2 = transform * glm::vec4(vertices[face.v2].position, 1.0f);

        // --- Backface Culling ---
        if (cullBackfaces) {
            glm::vec3 edge1 = glm::vec3(v1) - glm::vec3(v0);
            glm::vec3 edge2 = glm::vec3(v2) - glm::vec3(v0);
            glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
            glm::vec3 viewDir = glm::normalize(-glm::vec3(v0));
            if (glm::dot(normal, viewDir) < 0.0f) {
                continue;  // Skip back-facing triangle
            }
        }

        // Store original Z values before perspective divide (for depth interpolation)
        float z0 = v0.z;
        float z1 = v1.z;
        float z2 = v2.z;

        // Perspective divide
        v0 = v0 / v0.w;
        v1 = v1 / v1.w;
        v2 = v2 / v2.w;

        // Project to screen coordinates
        float x0 = v0.x * (m_width / 2) + (m_width / 2);
        float y0 = m_height / 2 - v0.y * (m_height / 2);
        float x1 = v1.x * (m_width / 2) + (m_width / 2);
        float y1 = m_height / 2 - v1.y * (m_height / 2);
        float x2 = v2.x * (m_width / 2) + (m_width / 2);
        float y2 = m_height / 2 - v2.y * (m_height / 2);
        // Compute the 2D area of the triangle in screen space
        float area = fabs((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0));
        if (area < 0.5f) continue;   // skip nearly degenerate triangles
        // Find bounding box
        int minX = (int)std::floor(std::min({ x0, x1, x2 }));
        int maxX = (int)std::ceil(std::max({ x0, x1, x2 }));
        int minY = (int)std::floor(std::min({ y0, y1, y2 }));
        int maxY = (int)std::ceil(std::max({ y0, y1, y2 }));

        // Clamp to screen
        minX = std::max(minX, 0);
        maxX = std::min(maxX, m_width - 1);
        minY = std::max(minY, 0);
        maxY = std::min(maxY, m_height - 1);

        // Barycentric vertices (in 2D screen space)
        glm::vec2 sv0(x0, y0);
        glm::vec2 sv1(x1, y1);
        glm::vec2 sv2(x2, y2);

        // --- Top-Left Fill Rule 
        // These values are used to determine if an edge is a top or left edge
        // Top edge: horizontal edge at the top (smallest y)
        // Left edge: edge going from top to bottom on the left side

        // Loop over every pixel in the bounding box
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // Compute barycentric weights for this pixel
                glm::vec3 bary = computeBarycentric((float)x, (float)y, sv0, sv1, sv2);

                // Epsilon for floating-point precision
                const float eps = 0.001f;
                // Check if pixel is inside the triangle (allow edge pixels)
                if (bary.x >= -eps && bary.y >= -eps && bary.z >= -eps) {
                    bool drawPixel = true;

                    // --- Top-Left Fill Rule 
                    if (fillRules) {
                        // Check if the pixel is on an edge of the triangle
                        bool onEdge0 = (fabs(bary.x) < eps);
                        bool onEdge1 = (fabs(bary.y) < eps);
                        bool onEdge2 = (fabs(bary.z) < eps);

                        if (onEdge0 || onEdge1 || onEdge2) {
                            // Determine which edge the pixel is on
                            // Edge 0 is opposite vertex 0 (between vertices 1 and 2)
                            if (onEdge0) {
                                // Edge 0: check if it's a top or left edge
                                // Top edge: horizontal (y1 == y2) and y1 < y0 (above vertex 0)
                                bool isHorizontal = (fabs(sv1.y - sv2.y) < eps);
                                bool isTopEdge = false;
                                bool isLeftEdge = false;

                                if (isHorizontal) {
                                    // Horizontal edge: top if y1 < y0 (lower y is top)
                                    isTopEdge = (sv1.y < sv0.y);
                                }
                                else {
                                    // Non-horizontal: left if x1 < x2 (going from top to bottom)
                                    // The edge is on the left if x decreases as y increases
                                    isLeftEdge = (sv1.x > sv2.x);
                                }

                                // Only draw if it's a top or left edge
                                if (!isTopEdge && !isLeftEdge) {
                                    drawPixel = false;
                                }
                            }
                            // Edge 1 is opposite vertex 1 (between vertices 2 and 0)
                            else if (onEdge1) {
                                bool isHorizontal = (fabs(sv2.y - sv0.y) < eps);
                                bool isTopEdge = false;
                                bool isLeftEdge = false;

                                if (isHorizontal) {
                                    isTopEdge = (sv2.y < sv1.y);
                                }
                                else {
                                    isLeftEdge = (sv2.x < sv0.x);
                                }

                                if (!isTopEdge && !isLeftEdge) {
                                    drawPixel = false;
                                }
                            }
                            // Edge 2 is opposite vertex 2 (between vertices 0 and 1)
                            else if (onEdge2) {
                                bool isHorizontal = (fabs(sv0.y - sv1.y) < eps);
                                bool isTopEdge = false;
                                bool isLeftEdge = false;

                                if (isHorizontal) {
                                    isTopEdge = (sv0.y < sv2.y);
                                }
                                else {
                                    isLeftEdge = (sv0.x < sv1.x);
                                }

                                if (!isTopEdge && !isLeftEdge) {
                                    drawPixel = false;
                                }
                            }
                        }
                    }

                    if (drawPixel) {
                        // --- Z-Buffer Depth Test ---
                        // Interpolate depth using barycentric coordinates
                        float depth = bary.x * z0 + bary.y * z1 + bary.z * z2;

                        int idx = y * m_width + x;
                        if (depth < zBuffer[idx]) {
                            // Pixel is closer! Update Z-buffer and color
                            zBuffer[idx] = depth;
                            m_buffer[idx] = color;
                        }
                    }
                }
            }
        }
    }
}
void UIRenderer::drawZBuffer(float* zBuffer, float minDepth, float maxDepth) {
    if (zBuffer == nullptr) return;

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            int idx = y * m_width + x;
            float depth = zBuffer[idx];

            // Normalize depth to 0-1 range
            float normalized = (depth - minDepth) / (maxDepth - minDepth);
            normalized = std::max(0.0f, std::min(1.0f, normalized));

            // Convert to grayscale (lighter = closer, darker = farther)
            // Or invert for better visualization: darker = closer
            uint8_t value = (uint8_t)(normalized * 255);
            // Invert so closer = darker (white = far, black = near)
            value = 255 - value;

            m_buffer[idx] = MFB_RGB(value, value, value);
        }
    }
}
void UIRenderer::drawTriangleBBox(const std::vector<Vertex3D>& vertices,
    const std::vector<Face3D>& faces,
    const glm::mat4& transform,
    uint32_t color) {
    for (const auto& face : faces) {
        // Get three vertices and transform them
        glm::vec4 v0 = transform * glm::vec4(vertices[face.v0].position, 1.0f);
        glm::vec4 v1 = transform * glm::vec4(vertices[face.v1].position, 1.0f);
        glm::vec4 v2 = transform * glm::vec4(vertices[face.v2].position, 1.0f);

        // Perspective divide
        v0 = v0 / v0.w;
        v1 = v1 / v1.w;
        v2 = v2 / v2.w;

        // Project to screen coordinates
        int x0 = (int)(v0.x * (m_width / 2) + (m_width / 2));
        int y0 = (int)(m_height / 2 - v0.y * (m_height / 2));
        int x1 = (int)(v1.x * (m_width / 2) + (m_width / 2));
        int y1 = (int)(m_height / 2 - v1.y * (m_height / 2));
        int x2 = (int)(v2.x * (m_width / 2) + (m_width / 2));
        int y2 = (int)(m_height / 2 - v2.y * (m_height / 2));

        // Find bounding box
        int minX = std::min({ x0, x1, x2 });
        int maxX = std::max({ x0, x1, x2 });
        int minY = std::min({ y0, y1, y2 });
        int maxY = std::max({ y0, y1, y2 });

        // Clamp to screen
        minX = std::max(minX, 0);
        maxX = std::min(maxX, m_width - 1);
        minY = std::max(minY, 0);
        maxY = std::min(maxY, m_height - 1);

        // Fill bounding box with color
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                m_buffer[y * m_width + x] = color;
            }
        }
    }
}