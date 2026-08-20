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
int enable_distortion = 1;  // 1 = shifted, 0 = normal
static int circle_mode = 0;  // 0 = line mode, 1 = circle mode
static int anti_alias = 0;  // 0 = Bresenham, 1 = Xiaolin Wu
std::vector<Line> lines;
std::vector<Circle> circles;
int main() {
    struct mfb_window* window =
        mfb_open_ex("MiniGUI Platform", WIDTH, HEIGHT, MFB_WF_RESIZABLE);
    if (!window)
        return 1;

    mu_Context* ctx = (mu_Context*)malloc(sizeof(mu_Context));
    mu_init(ctx);

    // Set font callbacks for microui
    ctx->text_width = [](mu_Font font, const char* str, int len) {
        return (len < 0 ? (int)strlen(str) : len) * 8;
        };
    ctx->text_height = [](mu_Font font) { return 8; };

    UIRenderer renderer(WIDTH, HEIGHT);

    // Set up char input callback for textbox input
    mfb_set_char_input_callback(
        [](struct mfb_window* w, unsigned int c) {
            ui_bridge_char_input(w, c);
            // modes
            if (c == '1') mode = 0;
            if (c == '2') mode = 1;
            if (c == '3') mode = 2;
            //intensity
            if (c == 'r') intensity = 1.0f;
            if (c == '+') intensity += 0.1f;
            if (c == '-') intensity -= 0.1f;
            // bound intensitybetween 0.1 and 2.0
            if (intensity < 0.1f) intensity = 0.1f;
            if (intensity > 2.0f) intensity = 2.0f;
        },
        window);

    while (mfb_update_events(window) != MFB_STATE_EXIT) {
        // mouse state management
        static bool drawing = false;
        static int start_x = 0, start_y = 0;
        // 1. Input
        ui_bridge_input(ctx, window);
        // handle mouse input for line drawing
        int mx = ctx->mouse_pos.x;
        int my = ctx->mouse_pos.y;

        bool ui_blocking = (ctx->hover_root != nullptr);

        if (!ui_blocking) {
            if (ctx->mouse_pressed == MU_MOUSE_LEFT) {
                drawing = true;
                start_x = mx;
                start_y = my;
            }
        }

        if (drawing && !(ctx->mouse_down & MU_MOUSE_LEFT)) {
            drawing = false;
            if (circle_mode) {
                // Calculate radius from start to current mouse position
                int dx = mx - start_x;
                int dy = my - start_y;
                int radius = (int)sqrtf(dx * dx + dy * dy);
                circles.push_back({ start_x, start_y, radius, current_color });
            }
            else {
                lines.push_back({ start_x, start_y, mx, my, current_color });
            }
        }
    
        // Update wave animation
        static float t = 0.0f;
        t += 0.016f * speed;

        // 2. Scene Rendering (Background)
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {

                float t0 = t * speed;

                float r = 0.0f, g = 0.0f, b = 0.0f;

                if (mode == 0) {
					// RGB waves
                    float fx = x * 0.005f* wave_strength;
                    float fy = y * 0.005f* wave_strength;

                    r = sinf(fx + t0) * 0.5f + 0.5f;
                    g = sinf(fy + t0 * 1.3f) * 0.5f + 0.5f;
                    b = sinf(fx + fy + t0 * 0.7f) * 0.5f + 0.5f;
                }

                else if (mode == 1) {
					// circle waves
                    float dx = x - WIDTH * 0.5f;
                    float dy = y - HEIGHT * 0.5f;

                    float dist = sqrtf(dx * dx + dy * dy);

                    float wave = sinf(dist * 0.02f * wave_strength - t0 * 3.0f);

                    r = wave * 0.5f + 0.5f;
                    g = sinf(wave * 3.0f + t0) * 0.5f + 0.5f;
                    b = 1.0f - r;
                }

                else if (mode == 2) {
					// square waves
                    float v1 = sinf((x + t0 * 60) * 0.02f * wave_strength);
                    float v2 = sinf((y - t0 * 40) * 0.02f * wave_strength);

                    r = v1 * 0.5f + 0.5f;
                    g = v2 * 0.5f + 0.5f;
                    b = sinf(v1 + v2 + t0) * 0.5f + 0.5f;
                }

                // intensity
                r *= intensity;
                g *= intensity;
                b *= intensity;

                // clamp
                if (r > 1.0f) r = 1.0f;
                if (g > 1.0f) g = 1.0f;
                if (b > 1.0f) b = 1.0f;

                // convert
                uint8_t R = (uint8_t)(r * 255);
                uint8_t G = (uint8_t)(g * 255);
                uint8_t B = (uint8_t)(b * 255);

                // invert
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

        // --- Widgets window ---
        if (mu_begin_window(ctx, "Widgets", mu_rect(20, 20, 360, 540))) {
            int w1[] = { -1 };
            static bool show_message = false;
            // label / text
            mu_layout_row(ctx, 1, w1, 0);
            mu_label(ctx, "mu_label: plain static text");
            mu_text(ctx, "press 0 ,1 or 2 to change mode , press + to increase intensity , - to decrease it press r to restart the intensity");
			//UI shift checkbox
            mu_checkbox(ctx, "UI Shift (click here)", &enable_distortion);
			//explaing the circle mode checkbox
            mu_label(ctx, "Circle: Click-drag to set center & radius");
			//circle mode checkbox
            mu_checkbox(ctx, "Circle Mode", &circle_mode);
            //UI controls for mouse drawing
            static float r_val = 255, g_val = 255, b_val = 255;

            mu_label(ctx, "Line Color R:");
            mu_slider(ctx, &r_val, 0, 255);

            mu_label(ctx, "Line Color G:");
            mu_slider(ctx, &g_val, 0, 255);

            mu_label(ctx, "Line Color B:");
            mu_slider(ctx, &b_val, 0, 255);
            // Update line color in real-time
            current_color = MFB_RGB((uint8_t)r_val, (uint8_t)g_val, (uint8_t)b_val);
            // clear lines button
            if (mu_button(ctx, "Clear Lines")) {
                lines.clear();
                circles.clear();
            }
            // button
            mu_layout_row(ctx, 1, w1, 0);
            if (mu_button(ctx, "mu_button: click me")) {
                quit_requested = false; // just a reaction
            }
            //button
            mu_layout_row(ctx, 1, w1, 0);
            if (mu_button(ctx, "Click me")) {
                show_message = true; // show a message box when clicked
            }
            // show message box if button clicked
            if (show_message) {
                mu_layout_row(ctx, 1, w1, 0);
                mu_label(ctx, "Hello , how it is going ? ");
            }
            // checkbox
            mu_layout_row(ctx, 1, w1, 0);
            mu_checkbox(ctx, "mu_checkbox A (off)", &checkbox_a);
            mu_checkbox(ctx, "mu_checkbox B (on)", &checkbox_b);

            // textbox
            mu_layout_row(ctx, 1, w1, 0);
            mu_label(ctx, "mu_textbox:");
            mu_textbox(ctx, textbox_buf, sizeof(textbox_buf));

            // slider
            mu_layout_row(ctx, 1, w1, 0);
            mu_label(ctx, "mu_slider (0-100):");
            mu_slider(ctx, &slider_val, 0, 100);
            // custom sliders for wave strength and animation speed
            mu_label(ctx, "wave strength:");
            mu_slider(ctx, &wave_strength, 0, 5);

            mu_label(ctx, "animation speed:");
            mu_slider(ctx, &speed, 0.1f, 10);
            // invert colors checkbox
            mu_checkbox(ctx, "invert colors", &invert_colors);
            // Benchmark button
            if (mu_button(ctx, "Benchmark Lines")) {
                // Seed random number generator
                srand((unsigned int)time(NULL));

                const int NUM_LINES = 100000;
                std::vector<Line> random_lines;
                random_lines.reserve(NUM_LINES);

                // Generate 100,000 random lines
                for (int i = 0; i < NUM_LINES; i++) {
                    int x0 = rand() % WIDTH;
                    int y0 = rand() % HEIGHT;
                    int x1 = rand() % WIDTH;
                    int y1 = rand() % HEIGHT;
                    random_lines.push_back({ x0, y0, x1, y1, MFB_RGB(255, 255, 255) });
                }

                printf("\n=== Benchmark: %d Lines ===\n", NUM_LINES);

                // --- Benchmark Bresenham ---
                auto start = std::chrono::high_resolution_clock::now();
                for (const auto& l : random_lines) {
                    renderer.draw_line(l.x0, l.y0, l.x1, l.y1, l.color);
                }
                auto end = std::chrono::high_resolution_clock::now();
                auto bresenham_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                printf("Bresenham (integer): %lld ms\n", bresenham_time);

                // --- Benchmark Naive ---
                start = std::chrono::high_resolution_clock::now();
                for (const auto& l : random_lines) {
                    renderer.draw_line_naive(l.x0, l.y0, l.x1, l.y1, l.color);
                }
                end = std::chrono::high_resolution_clock::now();
                auto naive_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                printf("Naive (floating):   %lld ms\n", naive_time);

                // Calculate percentage difference
                float speedup = (float)naive_time / (float)bresenham_time;
                printf("Speedup: %.2fx faster\n\n", speedup);
            }
            //anti-alias checkbox
            mu_checkbox(ctx, "Anti-alias (smooth lines)", &anti_alias);
            // number
            mu_layout_row(ctx, 1, w1, 0);
            mu_label(ctx, "mu_number (step 0.1):");
            mu_number(ctx, &number_val, 0.1f);

            // header (collapsible section)
            if (mu_header(ctx, "mu_header: collapsible section")) {
                mu_layout_row(ctx, 1, w1, 0);
                mu_label(ctx, "Content inside the header.");
            }

            // treenode
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

            // quit button
            mu_layout_row(ctx, 1, w1, 0);
            if (mu_button(ctx, "Quit")) {
                quit_requested = true;
            }

            mu_end_window(ctx);
        }

        // --- Panel window ---
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

        // --- Popup demo window ---
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
            if (mu_begin_window_ex(ctx, "my popup", mu_rect(0, 0, 260, 84),
                popup_opt)) {
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

        // draw stored lines 
        for (const auto& l : lines) {
            if (anti_alias) {
                renderer.draw_line_wu(l.x0, l.y0, l.x1, l.y1, l.color);
            }
            else {
                renderer.draw_line(l.x0, l.y0, l.x1, l.y1, l.color);
            }
        }
        // preview line while drawing
        if (drawing && !circle_mode) {
            if (anti_alias) {
                renderer.draw_line_wu(start_x, start_y, mx, my, current_color);
            }
            else {
                renderer.draw_line(start_x, start_y, mx, my, current_color);
            }
        }
        // preview circle while drawing
        if (drawing && circle_mode) {
            int dx = mx - start_x;
            int dy = my - start_y;
            int radius = (int)sqrtf(dx * dx + dy * dy);
            renderer.draw_circle(start_x, start_y, radius, current_color);
        }
        // draw stored circles
        for (const auto& c : circles) {
            renderer.draw_circle(c.xc, c.yc, c.radius, c.color);
        }

        // 5. Display
        mfb_update_state state = mfb_update_ex(window, g_buffer, WIDTH, HEIGHT);
        if (state < 0)
            break;

        // Cap FPS (optional, minifb has built-in sync)
        mfb_wait_sync(window);
    }

    mfb_close(window);
    free(ctx);
    return 0;
}
