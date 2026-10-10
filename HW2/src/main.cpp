#include "MiniFB.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <cmath>
extern "C" {
#include "microui.h"
}
#include "ui_bridge.h"
#include "ui_renderer.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>   
#include <cfloat>

#define WIDTH 1600
#define HEIGHT 1200

static uint32_t current_color = MFB_RGB(255, 255, 255);
static uint32_t g_buffer[WIDTH * HEIGHT];
static int mode = 0;
static float intensity = 1.0f;
static float wave_strength = 1.0f;
static float speed = 1.0f;
static int invert_colors = 0;

struct Line {
    int x0, y0, x1, y1;
    uint32_t color;
};
struct Circle {
    int xc, yc, radius;
    uint32_t color;
};

// 3D Model Container
struct Model3D {
    std::string name;
    std::vector<Vertex3D> vertices;
    std::vector<Face3D> faces;
    bool loaded = false;

    glm::vec3 localTranslation = glm::vec3(0.0f);
    glm::vec3 localRotation = glm::vec3(0.0f);
    glm::vec3 localScale = glm::vec3(1.0f);
    glm::vec3 worldTranslation = glm::vec3(0.0f);
    glm::vec3 worldRotation = glm::vec3(0.0f);
    glm::vec3 worldScale = glm::vec3(1.0f);
};

int enable_distortion = 0;              // 1 = shifted, 0 = normal
static int circle_mode = 0;             // 0 = line mode, 1 = circle mode
static int anti_alias = 0;              // 0 = Bresenham, 1 = Xiaolin Wu
static int show_2d_drawings = 0;        // 1 = show lines/circles, 0 = hide

std::vector<Line> lines;                // 2D drawing storage
std::vector<Circle> circles;            // 2D drawing storage
std::vector<Model3D> models;            // 3D Variables
int activeModelIndex = -1;              // -1 means no model loaded

int main() {
    struct mfb_window* window =
        mfb_open_ex("MiniGUI Platform", WIDTH, HEIGHT, MFB_WF_RESIZABLE);
    if (!window)
        return 1;

    mu_Context* ctx = (mu_Context*)malloc(sizeof(mu_Context));
    mu_init(ctx);

    ctx->text_width = [](mu_Font font, const char* str, int len) {
        return (len < 0 ? (int)strlen(str) : len) * 8;
        };
    ctx->text_height = [](mu_Font font) { return 8; };

    UIRenderer renderer(WIDTH, HEIGHT);

    // Test GLM
    glm::vec3 testVec(1.0f, 2.0f, 3.0f);
    glm::mat4 testMat = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 20.0f, 30.0f));
    glm::vec4 result = testMat * glm::vec4(testVec, 1.0f);
    printf("GLM Test: (%.1f, %.1f, %.1f, %.1f)\n", result.x, result.y, result.z, result.w);

    auto loadModel = [&](const std::string& filename, const std::string& name) {
        Model3D model;
        model.name = name;
        if (renderer.loadOBJ(filename, model.vertices, model.faces)) {
            glm::vec3 min(FLT_MAX), max(-FLT_MAX);
            for (const auto& v : model.vertices) {
                min.x = std::min(min.x, v.position.x);
                min.y = std::min(min.y, v.position.y);
                min.z = std::min(min.z, v.position.z);
                max.x = std::max(max.x, v.position.x);
                max.y = std::max(max.y, v.position.y);
                max.z = std::max(max.z, v.position.z);
            }
            glm::vec3 center = (min + max) * 0.5f;
            float size = std::max({ max.x - min.x, max.y - min.y, max.z - min.z });
            float scale = 400.0f / size;
            glm::mat4 normalizeMat = glm::translate(glm::mat4(1.0f), -center);
            normalizeMat = glm::scale(normalizeMat, glm::vec3(scale));
            for (auto& v : model.vertices) {
                glm::vec4 pos = normalizeMat * glm::vec4(v.position, 1.0f);
                v.position = glm::vec3(pos);
            }
            model.loaded = true;
            models.push_back(model);
            if (activeModelIndex == -1) activeModelIndex = 0;
            printf("Loaded '%s': %zu vertices, %zu faces\n", name.c_str(), model.vertices.size(), model.faces.size());
        }
        else {
            printf("Failed to load '%s'\n", filename.c_str());
        }
        };

    // Load models
    loadModel("cube.obj", "Cube");
    loadModel("cube2.obj", "Cube2");

    static bool mouseDragging = false;
    static int prevMouseX = 0;
    static int prevMouseY = 0;
    static bool rightDragging = false;
    static int prevRightX = 0;
    static int prevRightY = 0;
    static bool middleDragging = false;
    static int prevMiddleY = 0;
    // --- Part 6: Keyboard Controls for 3D Transformations ---
    mfb_set_char_input_callback(
        [](struct mfb_window* w, unsigned int c) {
            ui_bridge_char_input(w, c);
            if (c == '1') mode = 0;
            if (c == '2') mode = 1;
            if (c == '3') mode = 2;
            if (c == 'r') intensity = 1.0f;
            if (c == '+') intensity += 0.1f;
            if (c == '-') intensity -= 0.1f;
            if (intensity < 0.1f) intensity = 0.1f;
            if (intensity > 2.0f) intensity = 2.0f;

            if (activeModelIndex >= 0) {
                Model3D& model = models[activeModelIndex];
                if (c == 's' || c == 'S') model.localTranslation.y += 5.0f;
                if (c == 'w' || c == 'W') model.localTranslation.y -= 5.0f;
                if (c == 'a' || c == 'A') model.localTranslation.x -= 5.0f;
                if (c == 'd' || c == 'D') model.localTranslation.x += 5.0f;
                if (c == 'q' || c == 'Q') model.localTranslation.z += 5.0f;
                if (c == 'e' || c == 'E') model.localTranslation.z -= 5.0f;
                if (c == 'z' || c == 'Z') model.localRotation.x += 5.0f;
                if (c == 'x' || c == 'X') model.localRotation.x -= 5.0f;
                if (c == 'c' || c == 'C') model.localRotation.y += 5.0f;
                if (c == 'v' || c == 'V') model.localRotation.y -= 5.0f;
                if (c == 'b' || c == 'B') model.localRotation.z += 5.0f;
                if (c == 'n' || c == 'N') model.localRotation.z -= 5.0f;
                if (c == 'r' || c == 'R') {
                    model.localTranslation = glm::vec3(0.0f);
                    model.localRotation = glm::vec3(0.0f);
                    model.localScale = glm::vec3(1.0f);
                    model.worldTranslation = glm::vec3(0.0f);
                    model.worldRotation = glm::vec3(0.0f);
                    model.worldScale = glm::vec3(1.0f);
                    printf("Reset: %s\n", model.name.c_str());
                }
            }
        },
        window);

    while (mfb_update_events(window) != MFB_STATE_EXIT) {
        static bool drawing = false;
        static int start_x = 0, start_y = 0;

        // 1. Input
        ui_bridge_input(ctx, window);


        int mx = ctx->mouse_pos.x;
        int my = ctx->mouse_pos.y;
        bool ui_blocking = (ctx->hover_root != nullptr);
        // --- Middle Mouse Drag for Scaling ---
        if (!ui_blocking) {
            if (activeModelIndex >= 0 && models[activeModelIndex].loaded) {
                Model3D& model = models[activeModelIndex];

                if (ctx->mouse_down & MU_MOUSE_MIDDLE) {
                    if (!middleDragging) {
                        middleDragging = true;
                        prevMiddleY = my;
                    }
                    else {
                        int dy = my - prevMiddleY;
                        // Scale based on vertical mouse movement
                        float scaleFactor = 1.0f + dy * 0.005f;
                        model.localScale *= scaleFactor;
                        // Clamp
                        if (model.localScale.x < 0.1f) model.localScale.x = 0.1f;
                        if (model.localScale.y < 0.1f) model.localScale.y = 0.1f;
                        if (model.localScale.z < 0.1f) model.localScale.z = 0.1f;
                        if (model.localScale.x > 10.0f) model.localScale.x = 10.0f;
                        if (model.localScale.y > 10.0f) model.localScale.y = 10.0f;
                        if (model.localScale.z > 10.0f) model.localScale.z = 10.0f;
                        prevMiddleY = my;
                    }
                }
                else {
                    middleDragging = false;
                }
            }
        }
        // Right-click drag translation
        if (!ui_blocking) {
            if (activeModelIndex >= 0 && models[activeModelIndex].loaded) {
                Model3D& model = models[activeModelIndex];
                if (ctx->mouse_down & MU_MOUSE_RIGHT) {
                    if (!rightDragging) {
                        rightDragging = true;
                        prevRightX = mx;
                        prevRightY = my;
                    }
                    else {
                        int dx = mx - prevRightX;
                        int dy = my - prevRightY;
                        model.localTranslation.x += dx * 1.0f;
                        model.localTranslation.y += dy * 1.0f;
                        prevRightX = mx;
                        prevRightY = my;
                    }
                }
                else {
                    rightDragging = false;
                }
            }
        }

        // Left-click line/circle drawing
        if (!ui_blocking && show_2d_drawings) {
            if (ctx->mouse_pressed == MU_MOUSE_LEFT) {
                drawing = true;
                start_x = mx;
                start_y = my;
            }
        }
        if (drawing && !(ctx->mouse_down & MU_MOUSE_LEFT)) {
            drawing = false;
            if (show_2d_drawings) {
                if (circle_mode) {
                    int dx = mx - start_x;
                    int dy = my - start_y;
                    int radius = (int)sqrtf(dx * dx + dy * dy);
                    circles.push_back({ start_x, start_y, radius, current_color });
                }
                else {
                    lines.push_back({ start_x, start_y, mx, my, current_color });
                }
            }
        }

        // Left-click drag rotation (only when 2D drawings are OFF)
        if (!ui_blocking && !show_2d_drawings) {
            if (activeModelIndex >= 0 && models[activeModelIndex].loaded) {
                Model3D& model = models[activeModelIndex];
                if (ctx->mouse_down & MU_MOUSE_LEFT) {
                    if (!mouseDragging) {
                        mouseDragging = true;
                        prevMouseX = mx;
                        prevMouseY = my;
                    }
                    else {
                        int dx = mx - prevMouseX;
                        int dy = my - prevMouseY;
                        model.localRotation.y += dx * 0.5f;
                        model.localRotation.x += dy * 0.5f;
                        prevMouseX = mx;
                        prevMouseY = my;
                    }
                }
                else {
                    mouseDragging = false;
                }
            }
        }

        // Background animation
        static float t = 0.0f;
        t += 0.016f * speed;

        // 2. Scene Rendering (Background)
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                float t0 = t * speed;
                float r = 0.0f, g = 0.0f, b = 0.0f;

                if (mode == 0) {
                    float fx = x * 0.005f * wave_strength;
                    float fy = y * 0.005f * wave_strength;
                    r = sinf(fx + t0) * 0.5f + 0.5f;
                    g = sinf(fy + t0 * 1.3f) * 0.5f + 0.5f;
                    b = sinf(fx + fy + t0 * 0.7f) * 0.5f + 0.5f;
                }
                else if (mode == 1) {
                    float dx = x - WIDTH * 0.5f;
                    float dy = y - HEIGHT * 0.5f;
                    float dist = sqrtf(dx * dx + dy * dy);
                    float wave = sinf(dist * 0.02f * wave_strength - t0 * 3.0f);
                    r = wave * 0.5f + 0.5f;
                    g = sinf(wave * 3.0f + t0) * 0.5f + 0.5f;
                    b = 1.0f - r;
                }
                else if (mode == 2) {
                    float v1 = sinf((x + t0 * 60) * 0.02f * wave_strength);
                    float v2 = sinf((y - t0 * 40) * 0.02f * wave_strength);
                    r = v1 * 0.5f + 0.5f;
                    g = v2 * 0.5f + 0.5f;
                    b = sinf(v1 + v2 + t0) * 0.5f + 0.5f;
                }

                r *= intensity;
                g *= intensity;
                b *= intensity;
                if (r > 1.0f) r = 1.0f;
                if (g > 1.0f) g = 1.0f;
                if (b > 1.0f) b = 1.0f;

                uint8_t R = (uint8_t)(r * 255);
                uint8_t G = (uint8_t)(g * 255);
                uint8_t B = (uint8_t)(b * 255);

                if (invert_colors) {
                    R = 255 - R;
                    G = 255 - G;
                    B = 255 - B;
                }

                g_buffer[y * WIDTH + x] = MFB_RGB(R, G, B);
            }
        }

        // 3. UI Logic
        static float slider_val = 50.0f;
        static float number_val = 3.14f;
        static int checkbox_a = 0;
        static int checkbox_b = 1;
        static char textbox_buf[128] = "edit me";
        static bool quit_requested = false;

        mu_begin(ctx);

        if (mu_begin_window(ctx, "Widgets", mu_rect(20, 20, 360, 540))) {
            int w1[] = { -1 };
            static bool show_message = false;

            mu_layout_row(ctx, 1, w1, 0);
            mu_label(ctx, "mu_label: plain static text");
            mu_text(ctx, "press 1, 2 or 3 to change mode, + to increase, - to decrease, r to reset");

            mu_checkbox(ctx, "UI Shift (click here)", &enable_distortion);
            mu_label(ctx, "Circle: Click-drag to set center & radius");
            mu_checkbox(ctx, "Circle Mode", &circle_mode);

            static float r_val = 255, g_val = 255, b_val = 255;
            mu_label(ctx, "Line Color R:");
            mu_slider(ctx, &r_val, 0, 255);
            mu_label(ctx, "Line Color G:");
            mu_slider(ctx, &g_val, 0, 255);
            mu_label(ctx, "Line Color B:");
            mu_slider(ctx, &b_val, 0, 255);
            current_color = MFB_RGB((uint8_t)r_val, (uint8_t)g_val, (uint8_t)b_val);

            if (mu_button(ctx, "Clear Lines")) {
                lines.clear();
                circles.clear();
            }
            if (mu_button(ctx, "mu_button: click me")) {
                quit_requested = false;
            }
            if (mu_button(ctx, "Click me")) {
                show_message = true;
            }
            if (show_message) {
                mu_layout_row(ctx, 1, w1, 0);
                mu_label(ctx, "Hello, how is it going?");
            }
            mu_checkbox(ctx, "mu_checkbox A (off)", &checkbox_a);
            mu_checkbox(ctx, "mu_checkbox B (on)", &checkbox_b);
            mu_label(ctx, "mu_textbox:");
            mu_textbox(ctx, textbox_buf, sizeof(textbox_buf));
            mu_label(ctx, "mu_slider (0-100):");
            mu_slider(ctx, &slider_val, 0, 100);
            mu_label(ctx, "wave strength:");
            mu_slider(ctx, &wave_strength, 0, 5);
            mu_label(ctx, "animation speed:");
            mu_slider(ctx, &speed, 0.1f, 10);
            mu_checkbox(ctx, "invert colors", &invert_colors);

            if (mu_button(ctx, "Benchmark Lines")) {
                srand((unsigned int)time(NULL));
                const int NUM_LINES = 100000;
                std::vector<Line> random_lines;
                random_lines.reserve(NUM_LINES);
                for (int i = 0; i < NUM_LINES; i++) {
                    int x0 = rand() % WIDTH;
                    int y0 = rand() % HEIGHT;
                    int x1 = rand() % WIDTH;
                    int y1 = rand() % HEIGHT;
                    random_lines.push_back({ x0, y0, x1, y1, MFB_RGB(255, 255, 255) });
                }
                printf("\n=== Benchmark: %d Lines ===\n", NUM_LINES);
                auto start = std::chrono::high_resolution_clock::now();
                for (const auto& l : random_lines) {
                    renderer.draw_line(l.x0, l.y0, l.x1, l.y1, l.color);
                }
                auto end = std::chrono::high_resolution_clock::now();
                auto bresenham_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                printf("Bresenham (integer): %lld ms\n", bresenham_time);
                start = std::chrono::high_resolution_clock::now();
                for (const auto& l : random_lines) {
                    renderer.draw_line_naive(l.x0, l.y0, l.x1, l.y1, l.color);
                }
                end = std::chrono::high_resolution_clock::now();
                auto naive_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                printf("Naive (floating):   %lld ms\n", naive_time);
                float speedup = (float)naive_time / (float)bresenham_time;
                printf("Speedup: %.2fx faster\n\n", speedup);
            }

            mu_checkbox(ctx, "Anti-alias (smooth lines)", &anti_alias);
            mu_checkbox(ctx, "Show 2D Drawings", &show_2d_drawings);

            mu_label(ctx, "--- Models ---");
            for (int i = 0; i < (int)models.size(); i++) {
                char label[64];
                snprintf(label, sizeof(label), "%s %s", models[i].name.c_str(),
                    (i == activeModelIndex) ? "✓" : "");
                if (mu_button(ctx, label)) {
                    activeModelIndex = i;
                    printf("Switched to: %s\n", models[i].name.c_str());
                }
            }

            mu_label(ctx, "--- 3D Transformations ---");
            if (activeModelIndex >= 0 && models[activeModelIndex].loaded) {
                Model3D& model = models[activeModelIndex];
                mu_label(ctx, "Local Translation:");
                mu_slider(ctx, &model.localTranslation.x, -200, 200);
                mu_slider(ctx, &model.localTranslation.y, -200, 200);
                mu_slider(ctx, &model.localTranslation.z, -200, 200);
                mu_label(ctx, "Local Rotation (degrees):");
                mu_slider(ctx, &model.localRotation.x, -180, 180);
                mu_slider(ctx, &model.localRotation.y, -180, 180);
                mu_slider(ctx, &model.localRotation.z, -180, 180);
                mu_label(ctx, "Local Scale:");
                mu_slider(ctx, &model.localScale.x, 0.1f, 5.0f);
                mu_slider(ctx, &model.localScale.y, 0.1f, 5.0f);
                mu_slider(ctx, &model.localScale.z, 0.1f, 5.0f);
                mu_label(ctx, "World Translation:");
                mu_slider(ctx, &model.worldTranslation.x, -200, 200);
                mu_slider(ctx, &model.worldTranslation.y, -200, 200);
                mu_slider(ctx, &model.worldTranslation.z, -200, 200);
                mu_label(ctx, "World Rotation (degrees):");
                mu_slider(ctx, &model.worldRotation.x, -180, 180);
                mu_slider(ctx, &model.worldRotation.y, -180, 180);
                mu_slider(ctx, &model.worldRotation.z, -180, 180);
                mu_label(ctx, "World Scale:");
                mu_slider(ctx, &model.worldScale.x, 0.1f, 5.0f);
                mu_slider(ctx, &model.worldScale.y, 0.1f, 5.0f);
                mu_slider(ctx, &model.worldScale.z, 0.1f, 5.0f);
                char info[128];
                snprintf(info, sizeof(info), "Active: %s (%zu vertices, %zu faces)",
                    model.name.c_str(), model.vertices.size(), model.faces.size());
                mu_label(ctx, info);
            }
            else {
                mu_label(ctx, "No model loaded!");
            }

            mu_layout_row(ctx, 1, w1, 0);
            mu_label(ctx, "mu_number (step 0.1):");
            mu_number(ctx, &number_val, 0.1f);

            if (mu_header(ctx, "mu_header: collapsible section")) {
                mu_layout_row(ctx, 1, w1, 0);
                mu_label(ctx, "Content inside the header.");
            }
            if (mu_begin_treenode(ctx, "mu_treenode: root")) {
                mu_layout_row(ctx, 1, w1, 0);
                mu_label(ctx, "child item A");
                if (mu_begin_treenode(ctx, "nested node")) {
                    mu_layout_row(ctx, 1, w1, 0);
                    mu_label(ctx, "deeply nested item");
                    mu_end_treenode(ctx);
                }
                mu_end_treenode(ctx);
            }
            if (mu_button(ctx, "Quit")) {
                quit_requested = true;
            }
            mu_end_window(ctx);
        }

        if (mu_begin_window(ctx, "Panel Demo", mu_rect(395, 20, 380, 200))) {
            int w2[] = { -1 };
            mu_layout_row(ctx, 1, w2, 120);
            mu_begin_panel(ctx, "scrollable panel");
            int wp[] = { -1 };
            for (int i = 1; i <= 12; i++) {
                mu_layout_row(ctx, 1, wp, 0);
                char line[32];
                snprintf(line, sizeof(line), "Panel row %d", i);
                mu_label(ctx, line);
            }
            mu_end_panel(ctx);
            mu_end_window(ctx);
        }

        if (mu_begin_window(ctx, "Popup Demo", mu_rect(395, 235, 380, 80))) {
            int w3[] = { -1 };
            mu_layout_row(ctx, 1, w3, 0);
            if (mu_button(ctx, "Open popup")) {
                mu_Container* popup = mu_get_container(ctx, "my popup");
                popup->rect = mu_rect(ctx->mouse_pos.x, ctx->mouse_pos.y, 260, 84);
                popup->open = 1;
                ctx->hover_root = ctx->next_hover_root = popup;
                mu_bring_to_front(ctx, popup);
            }
            int popup_opt = MU_OPT_POPUP | MU_OPT_NORESIZE | MU_OPT_NOSCROLL |
                MU_OPT_NOTITLE | MU_OPT_CLOSED;
            if (mu_begin_window_ex(ctx, "my popup", mu_rect(0, 0, 260, 84), popup_opt)) {
                int wp[] = { -1 };
                mu_layout_row(ctx, 1, wp, 0);
                mu_label(ctx, "mu_popup: click outside to close");
                if (mu_button(ctx, "Close")) {
                    mu_get_current_container(ctx)->open = 0;
                }
                mu_end_window(ctx);
            }
            mu_end_window(ctx);
        }

        mu_end(ctx);

        if (quit_requested) {
            mfb_close(window);
            break;
        }

        // 4. UI Rendering
        renderer.render(ctx, g_buffer);

        // Draw ALL 3D Wireframes
        for (int i = 0; i < (int)models.size(); i++) {
            if (models[i].loaded) {
                Model3D& model = models[i];
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, model.localTranslation);
                modelMatrix = glm::rotate(modelMatrix, glm::radians(model.localRotation.x), glm::vec3(1, 0, 0));
                modelMatrix = glm::rotate(modelMatrix, glm::radians(model.localRotation.y), glm::vec3(0, 1, 0));
                modelMatrix = glm::rotate(modelMatrix, glm::radians(model.localRotation.z), glm::vec3(0, 0, 1));
                modelMatrix = glm::scale(modelMatrix, model.localScale);

                glm::mat4 worldMatrix = glm::mat4(1.0f);
                worldMatrix = glm::translate(worldMatrix, model.worldTranslation);
                worldMatrix = glm::rotate(worldMatrix, glm::radians(model.worldRotation.x), glm::vec3(1, 0, 0));
                worldMatrix = glm::rotate(worldMatrix, glm::radians(model.worldRotation.y), glm::vec3(0, 1, 0));
                worldMatrix = glm::rotate(worldMatrix, glm::radians(model.worldRotation.z), glm::vec3(0, 0, 1));
                worldMatrix = glm::scale(worldMatrix, model.worldScale);

                glm::mat4 finalTransform = worldMatrix * modelMatrix;

                uint32_t wireColor = (i == activeModelIndex) ? MFB_RGB(0, 0, 0) : MFB_RGB(100, 100, 100);
                renderer.drawWireframe(model.vertices, model.faces, finalTransform, wireColor);
            }
        }

        if (show_2d_drawings) {
            for (const auto& l : lines) { // draw stored lines 
                if (anti_alias) {
                    renderer.draw_line_wu(l.x0, l.y0, l.x1, l.y1, l.color);
                }
                else {
                    renderer.draw_line(l.x0, l.y0, l.x1, l.y1, l.color);
                }
            }
            if (drawing && !circle_mode) { // preview line while drawing
                if (anti_alias) {
                    renderer.draw_line_wu(start_x, start_y, mx, my, current_color);
                }
                else {
                    renderer.draw_line(start_x, start_y, mx, my, current_color);
                }
            }
            if (drawing && circle_mode) { // preview circle while drawing
                int dx = mx - start_x;
                int dy = my - start_y;
                int radius = (int)sqrtf(dx * dx + dy * dy);
                renderer.draw_circle(start_x, start_y, radius, current_color);
            }
            for (const auto& c : circles) {  // draw stored circles
                renderer.draw_circle(c.xc, c.yc, c.radius, c.color);
            }
        }

        // 5. Display (Always update)
        mfb_update_state state = mfb_update_ex(window, g_buffer, WIDTH, HEIGHT);
        if (state < 0)
            break;

        mfb_wait_sync(window);
    }

    mfb_close(window);
    free(ctx);
    return 0;
}