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

    // int faces[6][4] = {
    //     {0, 1, 2, 3}, 
    //     {5, 4, 7, 6}, 
    //     {4, 5, 1, 0}, 
    //     {3, 2, 6, 7}, 
    //     {4, 0, 3, 7}, 
    //     {1, 5, 6, 2}  
    // };

    int delay = 1000 / TARGET_FPS;
    int done = 0;
    float angle = 0.0f;
    double dz = 3.0f;
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
        angle += 2 * M_PI * dt / 4; 
        // dz += 1 * dt;
        // for (int i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
        //     Vector3D rotated_point = rotate(points[i], angle);
        //     rotated_point.z += dz;
        //     Vector2D projected_point = project(rotated_point);
        //     Vector2D screen_point = screen(projected_point);
        //     draw_point(renderer, screen_point.x, screen_point.y, 10, 0xFFFFFFFF, 1.0);
        // }
        for (int i = 0; i < sizeof(faces) / sizeof(faces[0]); ++i) {
            int num_faces = sizeof(faces[i]) / sizeof(faces[i][0]);
            for (int j = 0; j < num_faces; ++j) {
                int index1 = faces[i][j];
                int index2 = faces[i][(j+1) % num_faces];
                Vector3D rotated_point1 = rotate(points[index1], angle);
                rotated_point1.z += dz;
                Vector3D rotated_point2 = rotate(points[index2], angle);
                rotated_point2.z += dz;
                Vector2D projected_point1 = project(rotated_point1);
                Vector2D projected_point2 = project(rotated_point2);
                Vector2D screen_point1 = screen(projected_point1);
                Vector2D screen_point2 = screen(projected_point2);
                draw_line(renderer, screen_point1.x, screen_point1.y, screen_point2.x, screen_point2.y, 0xFFFFFFFF, 1.0);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(delay);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
