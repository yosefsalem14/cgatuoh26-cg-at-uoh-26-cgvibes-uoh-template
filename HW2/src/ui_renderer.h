#pragma once
extern "C" {
#include "microui.h"
#include <string>        
#include <vector>       
#include <cstdint>       
}
#include "MiniFB.h"
#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
struct Vertex3D {
    glm::vec3 position;
};
struct Face3D {
    int v0, v1, v2;
};
class UIRenderer {
public:
    UIRenderer(int width, int height);
    void render(mu_Context* ctx, uint32_t* buffer);
    void draw_line(int x0, int y0, int x1, int y1, uint32_t c);
    void draw_pixel(int x, int y, uint32_t c);
    void draw_circle(int xc, int yc, int radius, uint32_t color);
    void draw_line_naive(int x0, int y0, int x1, int y1, uint32_t color);
    void draw_line_wu(int x0, int y0, int x1, int y1, uint32_t color);
    uint32_t blend_pixel(uint32_t bg, uint32_t fg, float alpha);
    void drawWireframe(const std::vector<Vertex3D>& vertices,
        const std::vector<Face3D>& faces,
        const glm::mat4& transform,
        uint32_t color);
    bool loadOBJ(const std::string& filename,
        std::vector<Vertex3D>& vertices,
        std::vector<Face3D>& faces);

private:
    void draw_rect(mu_Rect rect, mu_Color color);
    void draw_text(const char* text, mu_Vec2 pos, mu_Color color);
    void draw_icon(int id, mu_Rect rect, mu_Color color);
    void set_clip_rect(mu_Rect rect);

    uint32_t* m_buffer;
    int m_width;
    int m_height;
    mu_Rect m_clip_rect;

    uint32_t to_uint32(mu_Color c) {
        return MFB_ARGB(c.a, c.r, c.g, c.b);
    }
};
