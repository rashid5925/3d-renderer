#include <stdio.h>
#include <SDL.h>
#include <math.h>

#include "math3d.h"
#include "utah_teapot.h"

#define WINDOW_TITLE "3D"
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define BACKGROUND_COLOR 0x00000000
#define TARGET_FPS 60

double deg_to_rad(double degrees) {
    return degrees * M_PI / 180.0;
}

double clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

double min(double a, double b, double c) {
    return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

double max(double a, double b, double c) {
    return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c);
}

Vector3D normalize(Vector3D v) {
    double length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    Vector3D normalized = {v.x / length, v.y / length, v.z / length};
    return normalized;
}

double dot_product(Vector3D a, Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3D cross_product(Vector3D a, Vector3D b) {
    Vector3D result = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return result;
}

void sort_faces_by_depth(int faces[][3], int num_faces, Vector3D points[]) {
    for (int i = 0; i < num_faces - 1; ++i) {
        for (int j = 0; j < num_faces - i - 1; ++j) {
            double depth1 = (points[faces[j][0]].z + points[faces[j][1]].z + points[faces[j][2]].z) / 3.0;
            double depth2 = (points[faces[j + 1][0]].z + points[faces[j + 1][1]].z + points[faces[j + 1][2]].z) / 3.0;
            if (depth1 < depth2) {
                int temp[3];
                for (int k = 0; k < 3; ++k) {
                    temp[k] = faces[j][k];
                    faces[j][k] = faces[j + 1][k];
                    faces[j + 1][k] = temp[k];
                }
            }
        }
    }
}

Vector2D screen(Vector2D point) {
    // -1 .. 1 => 0 .. W/H 
    // 0 .. 2 , /2 0 .. 1, * W/h 0 .. W/h
    Vector2D p = {
        (point.x + 1) / 2 * WINDOW_WIDTH,
        (1 - ((point.y + 1) / 2)) * WINDOW_HEIGHT
    };
    return p;
}

Vector2D project(Vector3D point) {
    Vector2D p = {
        point.x / point.z,
        point.y / point.z
    };
    return p;
}

Vector3D rotate(Vector3D point, double angle) {
    Vector3D p = {
        point.x * cos(angle) - point.z * sin(angle),
        point.y,
        point.x * sin(angle) + point.z * cos(angle)
    };
    return p;
}

void draw_line(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, Uint32 color, double shading) {
    Uint8 r = (color >> 24) & 0xFF;
    Uint8 g = (color >> 16) & 0xFF;
    Uint8 b = (color >> 8) & 0xFF;
    Uint8 a = color & 0xFF;
    SDL_SetRenderDrawColor(renderer, r * shading, g * shading, b * shading, a);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void draw_point(SDL_Renderer *renderer, int x, int y, int size, Uint32 color, double shading) {
    Uint8 r = (color >> 24) & 0xFF;
    Uint8 g = (color >> 16) & 0xFF;
    Uint8 b = (color >> 8) & 0xFF;
    Uint8 a = color & 0xFF;
    SDL_SetRenderDrawColor(renderer, r * shading, g * shading, b * shading, a);
    SDL_Rect rect = {x - size / 2, y - size / 2, size, size};
    SDL_RenderFillRect(renderer, &rect);
}

void fill_triangle(SDL_Renderer *renderer, Vector2D p1, Vector2D p2, Vector2D p3, Uint32 color, double shading) {
    double min_x = clamp(min(p1.x, p2.x, p3.x), 0.0, (double)WINDOW_WIDTH);
    double max_x = clamp(max(p1.x, p2.x, p3.x), 0.0, (double)WINDOW_WIDTH);
    double min_y = clamp(min(p1.y, p2.y, p3.y), 0.0, (double)WINDOW_HEIGHT);
    double max_y = clamp(max(p1.y, p2.y, p3.y), 0.0, (double)WINDOW_HEIGHT);

    for (int y = (int)min_y; y <= (int)max_y; ++y) {
        for (int x = (int)min_x; x <= (int)max_x; ++x) {
            double alpha = ((p2.y - p3.y) * (x - p3.x) + (p3.x - p2.x) * (y - p3.y)) /
                           ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
            double beta = ((p3.y - p1.y) * (x - p3.x) + (p1.x - p3.x) * (y - p3.y)) /
                          ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
            double gamma = 1.0 - alpha - beta;

            if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                draw_point(renderer, x, y, 1, color, shading);
            }
        }
    }
}

int main() {
    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
        WINDOW_HEIGHT, 
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderClear(renderer);

    // Vector3D points[] = {
    //     {0.25, 0.25, 0.25},
    //     {-0.25, 0.25, 0.25},
    //     {-0.25, -0.25, 0.25},
    //     {0.25, -0.25, 0.25},

    //     {0.25, 0.25, -0.25},
    //     {-0.25, 0.25, -0.25},
    //     {-0.25, -0.25, -0.25},
    //     {0.25, -0.25, -0.25}
    // };

    // int faces[12][3] = {
    //     {0, 1, 2}, {0, 2, 3},   // top    (z+) 
    //     {4, 6, 5}, {4, 7, 6},   // bottom (z-)
    //     {0, 5, 1}, {0, 4, 5},   // side   (y+)
    //     {1, 6, 2}, {1, 5, 6},   // side   (x-)
    //     {2, 7, 3}, {2, 6, 7},   // side   (y-)
    //     {3, 4, 0}, {3, 7, 4}    // side   (x+)
    // };

    // Vector3D points[12] = {
    //     {-0.262866,  0.425325,  0.000000},
    //     { 0.262866,  0.425325,  0.000000},
    //     {-0.262866, -0.425325,  0.000000},
    //     { 0.262866, -0.425325,  0.000000},
    //     { 0.000000, -0.262866,  0.425325},
    //     { 0.000000,  0.262866,  0.425325},
    //     { 0.000000, -0.262866, -0.425325},
    //     { 0.000000,  0.262866, -0.425325},
    //     { 0.425325,  0.000000, -0.262866},
    //     { 0.425325,  0.000000,  0.262866},
    //     {-0.425325,  0.000000, -0.262866},
    //     {-0.425325,  0.000000,  0.262866}
    // };

    // // 20 Triangular Faces (Counter-Clockwise Winding)
    // int faces[20][3] = {
    //     {0, 11, 5},  {0, 5, 1},   {0, 1, 7},   {0, 7, 10},  {0, 10, 11},
    //     {1, 5, 9},   {5, 11, 4},  {11, 10, 2}, {10, 7, 6},  {7, 1, 8},
    //     {3, 9, 4},   {3, 4, 2},   {3, 2, 6},   {3, 6, 8},   {3, 8, 9},
    //     {4, 9, 5},   {2, 4, 11},  {6, 2, 10},  {8, 6, 7},   {9, 8, 1}
    // };

    // light source
    Vector3D light_source = {0.0, 0.0, -1.0};

    int delay = 1000 / TARGET_FPS;
    int done = 0;
    float angle = 0.0f;
    double dz = 7.0f;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = 1;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
        SDL_RenderClear(renderer);
        double dt = 1.0 / TARGET_FPS;
        angle += 2 * M_PI * dt / 2; 
        // dz += 1 * dt;

        Vector3D rotated_points[sizeof(points) / sizeof(points[0])];
        for (int i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
            rotated_points[i] = rotate(points[i], angle);
            rotated_points[i].z += dz;
        }
        sort_faces_by_depth(faces, sizeof(faces) / sizeof(faces[0]), rotated_points);

        for (int i = 0; i < sizeof(faces) / sizeof(faces[0]); ++i) {
            Vector3D rotated_point1 = rotate(points[faces[i][0]], angle);
            rotated_point1.z += dz;
            Vector3D rotated_point2 = rotate(points[faces[i][1]], angle);
            rotated_point2.z += dz;
            Vector3D rotated_point3 = rotate(points[faces[i][2]], angle);
            rotated_point3.z += dz;

            Vector3D face_normal = cross_product(
                (Vector3D){
                    rotated_point2.x - rotated_point1.x,
                    rotated_point2.y - rotated_point1.y,
                    rotated_point2.z - rotated_point1.z
                },
                (Vector3D){
                    rotated_point3.x - rotated_point1.x,
                    rotated_point3.y - rotated_point1.y,
                    rotated_point3.z - rotated_point1.z
                }
            );
            double ambient = 0.15;
            double diffuse = clamp(dot_product(normalize(face_normal), light_source), 0.0, 1.0);
            double shading = ambient + (1.0 - ambient) * diffuse;
            
            Vector3D triangle[3] = {rotated_point1, rotated_point2, rotated_point3};
            Vector2D projected_triangle[3];
            for (int j = 0; j < 3; ++j) {
                Vector2D projected_point1 = project(triangle[j]);
                Vector2D projected_point2 = project(triangle[(j + 1) % 3]);
                Vector2D screen_point1 = screen(projected_point1);
                Vector2D screen_point2 = screen(projected_point2);
                projected_triangle[j] = screen(projected_point1);
            }
            fill_triangle(renderer, projected_triangle[0], projected_triangle[1], projected_triangle[2], 0x00FF00FF, shading);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(delay);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
