#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif
#define WIDTH 60
#define HEIGHT 20
#define MAX_SHAPES 100
// Character grid structure representing the canvas
char canvas[HEIGHT][WIDTH];
int canvas_color[HEIGHT][WIDTH]; // Holds color codes for each pixel
// Types of shapes supported
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_TRIANGLE,
    SHAPE_CIRCLE
} ShapeType;
// Parameters for each shape type
typedef struct {
    int x1, y1;
    int x2, y2;
} LineParams;
typedef struct {
    int x, y; // Top-left corner
    int w, h; // Width and height
} RectParams;
typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} TriParams;
typedef struct {
    int cx, cy;
    int r;
} CircleParams;
// Shape definition
typedef struct {
    int id;
    ShapeType type;
    int color_code; // ANSI color index (1-6)
    union {
        LineParams line;
        RectParams rect;
        TriParams tri;
        CircleParams circle;
    } p;
} Shape;
// List of active shapes in the editor
Shape shapes[MAX_SHAPES];
int shape_count = 0;
int next_shape_id = 1;
// Returns the ANSI escape code string for a given color index
const char* get_color_escape(int code) {
    switch (code) {
        case 1: return "\033[1;31m"; // Bold Red
        case 2: return "\033[1;32m"; // Bold Green
        case 3: return "\033[1;33m"; // Bold Yellow
        case 4: return "\033[1;34m"; // Bold Blue
        case 5: return "\033[1;35m"; // Bold Magenta
        case 6: return "\033[1;36m"; // Bold Cyan
        case 7: return "\033[0;37m"; // Normal White
        default: return "\033[0m";    // Reset
    }
}
// Clears the character canvas to background underscores ('_')
void clear_canvas() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            canvas[y][x] = '_';
            canvas_color[y][x] = 0; // Default/Reset color
        }
    }
}
// Plot a pixel on the canvas if it lies within the viewport limits
void plot_pixel(int x, int y, char ch, int color_code) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        canvas[y][x] = ch;
        canvas_color[y][x] = color_code;
    }
}
// Draws a line using Bresenham's Line Algorithm
void draw_line(int x1, int y1, int x2, int y2, int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        plot_pixel(x1, y1, '*', color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}
// Draws the outline of a rectangle
void draw_rect(int x, int y, int w, int h, int color) {
    int x_end = x + w - 1;
    int y_end = y + h - 1;
    // Top and bottom horizontal edges
    for (int col = x; col <= x_end; col++) {
        plot_pixel(col, y, '*', color);
        plot_pixel(col, y_end, '*', color);
    }
    // Left and right vertical edges
    for (int row = y; row <= y_end; row++) {
        plot_pixel(x, row, '*', color);
        plot_pixel(x_end, row, '*', color);
    }
}
// Draws a triangle by connecting its three vertices with lines
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) {
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x3, y3, color);
    draw_line(x3, y3, x1, y1, color);
}
// Draws a circle outline using Bresenham's Midpoint Circle Algorithm
void draw_circle(int cx, int cy, int r, int color) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    while (x <= y) {
        plot_pixel(cx + x, cy + y, '*', color);
        plot_pixel(cx - x, cy + y, '*', color);
        plot_pixel(cx + x, cy - y, '*', color);
        plot_pixel(cx - x, cy - y, '*', color);
        plot_pixel(cx + y, cy + x, '*', color);
        plot_pixel(cx - y, cy + x, '*', color);
        plot_pixel(cx + y, cy - x, '*', color);
        plot_pixel(cx - y, cy - x, '*', color);
        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}
// Clears and regenerates the canvas layout from the active shapes list
void redraw_canvas() {
    clear_canvas();
    for (int i = 0; i < shape_count; i++) {
        Shape s = shapes[i];
        switch (s.type) {
            case SHAPE_LINE:
                draw_line(s.p.line.x1, s.p.line.y1, s.p.line.x2, s.p.line.y2, s.color_code);
                break;
            case SHAPE_RECTANGLE:
                draw_rect(s.p.rect.x, s.p.rect.y, s.p.rect.w, s.p.rect.h, s.color_code);
                break;
            case SHAPE_TRIANGLE:
                draw_triangle(s.p.tri.x1, s.p.tri.y1, s.p.tri.x2, s.p.tri.y2, s.p.tri.x3, s.p.tri.y3, s.color_code);
                break;
            case SHAPE_CIRCLE:
                draw_circle(s.p.circle.cx, s.p.circle.cy, s.p.circle.r, s.color_code);
                break;
        }
    }
}
// Adds a shape to the active shape collection
void add_shape(Shape s) {
    if (shape_count < MAX_SHAPES) {
        s.id = next_shape_id++;
        shapes[shape_count++] = s;
    } else {
        printf("\033[1;31mError: Canvas maximum shape limit (%d) reached!\033[0m\n", MAX_SHAPES);
    }
}
// Deletes a shape from the list by its ID
int delete_shape(int id) {
    for (int i = 0; i < shape_count; i++) {
        if (shapes[i].id == id) {
            for (int j = i; j < shape_count - 1; j++) {
                shapes[j] = shapes[j + 1];
            }
            shape_count--;
            return 1; // Success
        }
    }
    return 0; // Not found
}
// Prints the 2D grid canvas to the console with coordinates and colors
void display_canvas() {
    // Column header tens digit
    printf("   ");
    for (int x = 0; x < WIDTH; x++) {
        if (x % 10 == 0) printf("%d", x / 10);
        else printf(" ");
    }
    printf("\n   ");
    // Column header units digit
    for (int x = 0; x < WIDTH; x++) {
        printf("%d", x % 10);
    }
    printf("\n");
    // Top horizontal border
    printf("  +");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+\n");
    // Print rows
    for (int y = 0; y < HEIGHT; y++) {
        printf("%2d|", y); // Row index label
        for (int x = 0; x < WIDTH; x++) {
            char ch = canvas[y][x];
            int color = canvas_color[y][x];
            if (ch == '*') {
                printf("%s%c\033[0m", get_color_escape(color), ch);
            } else {
                // Dim down background characters for premium look
                printf("\033[90m%c\033[0m", ch);
            }
        }
        printf("|\n");
    }
    // Bottom horizontal border
    printf("  +");
    for (int x = 0; x < WIDTH; x++) printf("-");
    printf("+\n");
}
// Print the list of active shapes with their details
void print_active_shapes() {
    printf("\n\033[1;33mActive Objects:\033[0m\n");
    if (shape_count == 0) {
        printf("  (None - canvas is empty)\n");
        return;
    }
    for (int i = 0; i < shape_count; i++) {
        Shape s = shapes[i];
        const char* type_name = "";
        char color_name[20];
        switch (s.color_code) {
            case 1: strcpy(color_name, "Red"); break;
            case 2: strcpy(color_name, "Green"); break;
            case 3: strcpy(color_name, "Yellow"); break;
            case 4: strcpy(color_name, "Blue"); break;
            case 5: strcpy(color_name, "Magenta"); break;
            case 6: strcpy(color_name, "Cyan"); break;
            default: strcpy(color_name, "White"); break;
        }
        switch (s.type) {
            case SHAPE_LINE:
                printf("  [%d] Line (%s): (%d, %d) -> (%d, %d)\n",
                       s.id, color_name, s.p.line.x1, s.p.line.y1, s.p.line.x2, s.p.line.y2);
                break;
            case SHAPE_RECTANGLE:
                printf("  [%d] Rectangle (%s): Top-left (%d, %d), Size %dx%d\n",
                       s.id, color_name, s.p.rect.x, s.p.rect.y, s.p.rect.w, s.p.rect.h);
                break;
            case SHAPE_TRIANGLE:
                printf("  [%d] Triangle (%s): P1(%d, %d), P2(%d, %d), P3(%d, %d)\n",
                       s.id, color_name, s.p.tri.x1, s.p.tri.y1, s.p.tri.x2, s.p.tri.y2, s.p.tri.x3, s.p.tri.y3);
                break;
            case SHAPE_CIRCLE:
                printf("  [%d] Circle (%s): Center (%d, %d), Radius %d\n",
                       s.id, color_name, s.p.circle.cx, s.p.circle.cy, s.p.circle.r);
                break;
        }
    }
}
// Clean and clear keyboard input buffer to avoid skipping scans
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
// Display the command menu
void print_menu() {
    printf("\n");
    printf("\033[1;36m========================================================\033[0m\n");
    printf("\033[1;36m            2D GRAPHICS CONSOLE EDITOR                  \033[0m\n");
    printf("\033[1;36m========================================================\033[0m\n");
    printf(" 1. Add Line               5. Delete Object\n");
    printf(" 2. Add Rectangle          6. Clear Canvas (All)\n");
    printf(" 3. Add Triangle           7. Save Canvas to File\n");
    printf(" 4. Add Circle             8. Exit\n");
    printf("\033[1;36m--------------------------------------------------------\033[0m\n");
}
// Clear console viewport using ANSI escape sequences
void clear_screen() {
    printf("\033[H\033[2J");
    fflush(stdout);
}
// Prompt user to enter a point coord, showing coordinates safety warning if outside viewport
void prompt_coord(const char* label, int* x, int* y) {
    while (1) {
        printf("%s (x y): ", label);
        if (scanf("%d %d", x, y) == 2) {
            if (*x < 0 || *x >= WIDTH || *y < 0 || *y >= HEIGHT) {
                printf("\033[0;33mWarning: Coordinates are outside visible grid [0..%d, 0..%d]. It will be clipped.\033[0m\n", WIDTH - 1, HEIGHT - 1);
            }
            break;
        } else {
            printf("\033[1;31mInvalid coordinate. Please enter two integers separated by space (e.g., 10 5).\033[0m\n");
            clear_input_buffer();
        }
    }
}
// Prompt user for color choice
int prompt_color() {
    int choice = 6; // Default to Cyan
    printf("Choose Color (1:Red, 2:Green, 3:Yellow, 4:Blue, 5:Magenta, 6:Cyan): ");
    if (scanf("%d", &choice) != 1) {
        clear_input_buffer();
        return 6;
    }
    if (choice < 1 || choice > 6) choice = 6;
    return choice;
}
// Save ASCII representation of the canvas to a file
void save_to_file() {
    char filename[100];
    printf("Enter filename to save (e.g., drawing.txt): ");
    if (scanf("%99s", filename) != 1) {
        printf("\033[1;31mError reading filename.\033[0m\n");
        return;
    }
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("\033[1;31mError: Could not open file '%s' for writing.\033[0m\n", filename);
        return;
    }
    // Write character grid
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fputc(canvas[y][x], f);
        }
        fputc('\n', f);
    }
    fclose(f);
    printf("\033[1;32mCanvas successfully saved to '%s'!\033[0m\n", filename);
}
int main() {
    // Enable ANSI virtual terminal colors on Windows console
    #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    #endif
    int choice;
    clear_canvas();
    // Set up standard starting demo objects (a decorative box and center circle)
    Shape initial_rect = {
        .id = 0,
        .type = SHAPE_RECTANGLE,
        .color_code = 6, // Cyan
        .p.rect = { .x = 4, .y = 2, .w = 52, .h = 16 }
    };
    add_shape(initial_rect);
    Shape initial_circle = {
        .id = 0,
        .type = SHAPE_CIRCLE,
        .color_code = 3, // Yellow
        .p.circle = { .cx = 30, .cy = 10, .r = 5 }
    };
    add_shape(initial_circle);
    redraw_canvas();
    while (1) {
        clear_screen();
        display_canvas();
        print_active_shapes();
        print_menu();
        printf("Enter your choice (1-8): ");
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();
        if (choice == 8) {
            printf("\nExiting 2D Graphics Editor. Thank you!\n");
            break;
        }
        switch (choice) {
            case 1: { // Add Line
                int x1, y1, x2, y2;
                printf("\n--- Add Line ---\n");
                prompt_coord("Enter start coordinate", &x1, &y1);
                prompt_coord("Enter end coordinate", &x2, &y2);
                int color = prompt_color();
                clear_input_buffer();
                Shape s = {
                    .type = SHAPE_LINE,
                    .color_code = color,
                    .p.line = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2 }
                };
                add_shape(s);
                redraw_canvas();
                break;
            }
            case 2: { // Add Rectangle
                int x, y, w, h;
                printf("\n--- Add Rectangle ---\n");
                prompt_coord("Enter top-left coordinate", &x, &y);
                while (1) {
                    printf("Enter width and height (w h): ");
                    if (scanf("%d %d", &w, &h) == 2) {
                        if (w <= 0 || h <= 0) {
                            printf("\033[1;31mWidth and height must be positive integers.\033[0m\n");
                            clear_input_buffer();
                        } else {
                            break;
                        }
                    } else {
                        printf("\033[1;31mInvalid dimensions. Enter two integers.\033[0m\n");
                        clear_input_buffer();
                    }
                }
                int color = prompt_color();
                clear_input_buffer();
                Shape s = {
                    .type = SHAPE_RECTANGLE,
                    .color_code = color,
                    .p.rect = { .x = x, .y = y, .w = w, .h = h }
                };
                add_shape(s);
                redraw_canvas();
                break;
            }
            case 3: { // Add Triangle
                int x1, y1, x2, y2, x3, y3;
                printf("\n--- Add Triangle ---\n");
                prompt_coord("Enter first vertex", &x1, &y1);
                prompt_coord("Enter second vertex", &x2, &y2);
                prompt_coord("Enter third vertex", &x3, &y3);
                int color = prompt_color();
                clear_input_buffer();
                Shape s = {
                    .type = SHAPE_TRIANGLE,
                    .color_code = color,
                    .p.tri = { .x1 = x1, .y1 = y1, .x2 = x2, .y2 = y2, .x3 = x3, .y3 = y3 }
                };
                add_shape(s);
                redraw_canvas();
                break;
            }
            case 4: { // Add Circle
                int cx, cy, r;
                printf("\n--- Add Circle ---\n");
                prompt_coord("Enter center coordinate", &cx, &cy);
                while (1) {
                    printf("Enter radius: ");
                    if (scanf("%d", &r) == 1) {
                        if (r <= 0) {
                            printf("\033[1;31mRadius must be a positive integer.\033[0m\n");
                            clear_input_buffer();
                        } else {
                            break;
                        }
                    } else {
                        printf("\033[1;31mInvalid radius. Enter an integer.\033[0m\n");
                        clear_input_buffer();
                    }
                }
                int color = prompt_color();
                clear_input_buffer();
                Shape s = {
                    .type = SHAPE_CIRCLE,
                    .color_code = color,
                    .p.circle = { .cx = cx, .cy = cy, .r = r }
                };
                add_shape(s);
                redraw_canvas();
                break;
            }
            case 5: { // Delete Object
                if (shape_count == 0) {
                    printf("\nNo objects to delete! Press Enter to continue...");
                    getchar();
                    break;
                }
                int id;
                printf("\nEnter the ID of the object to delete: ");
                if (scanf("%d", &id) == 1) {
                    clear_input_buffer();
                    if (delete_shape(id)) {
                        printf("\033[1;32mObject [%d] deleted successfully!\033[0m\n", id);
                        redraw_canvas();
                    } else {
                        printf("\033[1;31mObject with ID %d not found.\033[0m\n", id);
                    }
                } else {
                    clear_input_buffer();
                    printf("\033[1;31mInvalid ID.\033[0m\n");
                }
                printf("Press Enter to continue...");
                getchar();
                break;
            }
            case 6: { // Clear Canvas
                shape_count = 0;
                redraw_canvas();
                printf("\n\033[1;32mCanvas cleared successfully!\033[0m\n");
                printf("Press Enter to continue...");
                getchar();
                break;
            }
            case 7: { // Save to File
                save_to_file();
                clear_input_buffer();
                printf("Press Enter to continue...");
                getchar();
                break;
            }
            default:
                printf("\n\033[1;31mInvalid selection! Please choose 1-8.\033[0m\n");
                printf("Press Enter to continue...");
                getchar();
                break;
        }
    }
    return 0;
}
