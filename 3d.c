#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "math3d.h"
// #include "utah_teapot.h"

#define WINDOW_TITLE "3D"
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define BACKGROUND_COLOR 0x00000000
#define TARGET_FPS 60

typedef struct {
    Vector3D *vertices;
    int vertex_count;
    int (*faces)[3];
    int face_count;
} Mesh;

typedef struct {
    int face[3];
    double depth;
} FaceDepth;

Mesh load_obj(const char *path) {
    Mesh mesh = {0};

    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        return mesh;
    }

    char line[1024];

    while (fgets(line, sizeof(line), file)) {

        // Vertex
        if (strncmp(line, "v ", 2) == 0) {
            Vector3D v;

            if (sscanf(line, "v %lf %lf %lf",
                       &v.x, &v.y, &v.z) == 3) {

                Vector3D *vertices = realloc(
                    mesh.vertices,
                    (mesh.vertex_count + 1) * sizeof(Vector3D)
                );

                if (!vertices) {
                    fprintf(stderr, "Failed to allocate vertices\n");
                    free(mesh.vertices);
                    free(mesh.faces);
                    fclose(file);

                    Mesh empty = {0};
                    return empty;
                }

                mesh.vertices = vertices;
                mesh.vertices[mesh.vertex_count++] = v;
            }
        }

        // Face
        else if (strncmp(line, "f ", 2) == 0) {

            int indices[64];
            int count = 0;

            char *token = strtok(line + 2, " \t\r\n");

            while (token && count < 64) {

                // Extract vertex index from:
                //
                // 1
                // 1/2
                // 1/2/3
                // 1//3
                //
                indices[count] = atoi(token);

                // Convert OBJ index to zero-based index
                if (indices[count] > 0) {
                    indices[count]--;
                }
                else if (indices[count] < 0) {
                    indices[count] = mesh.vertex_count + indices[count];
                }

                count++;
                token = strtok(NULL, " \t\r\n");
            }

            if (count < 3)
                continue;

            /*
             * Triangulate polygon using a triangle fan:
             *
             * 1 2 3 4
             *
             * becomes:
             *
             * 1 2 3
             * 1 3 4
             */

            for (int i = 1; i < count - 1; i++) {

                int (*faces)[3] = realloc(
                    mesh.faces,
                    (mesh.face_count + 1) * sizeof(int[3])
                );

                if (!faces) {
                    fprintf(stderr, "Failed to allocate faces\n");
                    free(mesh.vertices);
                    free(mesh.faces);
                    fclose(file);

                    Mesh empty = {0};
                    return empty;
                }

                mesh.faces = faces;

                mesh.faces[mesh.face_count][0] = indices[0];
                mesh.faces[mesh.face_count][1] = indices[i];
                mesh.faces[mesh.face_count][2] = indices[i + 1];

                mesh.face_count++;
            }
        }
    }

    fclose(file);

    return mesh;
}

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

void normalize_mesh(Mesh *m) {
    Vector3D mn = m->vertices[0], mx = m->vertices[0];
    for (int i = 1; i < m->vertex_count; ++i) {
        Vector3D v = m->vertices[i];
        if (v.x < mn.x) mn.x = v.x;  if (v.x > mx.x) mx.x = v.x;
        if (v.y < mn.y) mn.y = v.y;  if (v.y > mx.y) mx.y = v.y;
        if (v.z < mn.z) mn.z = v.z;  if (v.z > mx.z) mx.z = v.z;
    }
    Vector3D c = {(mn.x + mx.x) / 2, (mn.y + mx.y) / 2, (mn.z + mx.z) / 2};
    double size = max(mx.x - mn.x, mx.y - mn.y, mx.z - mn.z);
    double s = 2.0 / size;
    for (int i = 0; i < m->vertex_count; ++i) {
        m->vertices[i].x = (m->vertices[i].x - c.x) * s;
        m->vertices[i].y = (m->vertices[i].y - c.y) * s;
        m->vertices[i].z = (m->vertices[i].z - c.z) * s;
    }
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
        (point.x / ((double)WINDOW_WIDTH/(double)WINDOW_HEIGHT)) / point.z,
        point.y / point.z
    };
    return p;
}

Vector3D rotate_x(Vector3D point, double angle) {
    Vector3D p = {
        point.x,
        point.y * cos(angle) - point.z * sin(angle),
        point.y * sin(angle) + point.z * cos(angle)
    };
    return p;
}

Vector3D rotate_y(Vector3D point, double angle) {
    Vector3D p = {
        point.x * cos(angle) - point.z * sin(angle),
        point.y,
        point.x * sin(angle) + point.z * cos(angle)
    };
    return p;
}

Vector3D transform(Vector3D p, double ax, double ay, Vector3D pos) {
    p = rotate_x(p, ax);
    p = rotate_y(p, ay);
    return (Vector3D){p.x + pos.x, p.y + pos.y, p.z + pos.z};
}

Uint32 pack_color(Uint32 rgba, double shading) {
    Uint8 r = ((rgba >> 24) & 0xFF) * shading;
    Uint8 g = ((rgba >> 16) & 0xFF) * shading;
    Uint8 b = ((rgba >> 8)  & 0xFF) * shading;
    return 0xFF000000 | (r << 16) | (g << 8) | b;
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

void fill_triangle(SDL_Renderer *renderer, Uint32 *framebuffer, float  *zbuffer, Vector2D p1, Vector2D p2, Vector2D p3, double z1, double z2, double z3, Uint32 color, double shading) {
    double min_x = clamp(min(p1.x, p2.x, p3.x), 0.0, (double)WINDOW_WIDTH - 1);
    double max_x = clamp(max(p1.x, p2.x, p3.x), 0.0, (double)WINDOW_WIDTH - 1);
    double min_y = clamp(min(p1.y, p2.y, p3.y), 0.0, (double)WINDOW_HEIGHT - 1);
    double max_y = clamp(max(p1.y, p2.y, p3.y), 0.0, (double)WINDOW_HEIGHT - 1);

    double det = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
    if (fabs(det) < 1e-9) return;           // degenerate triangle
    double inv_det = 1.0 / det;

    double iz1 = 1.0 / z1, iz2 = 1.0 / z2, iz3 = 1.0 / z3;
    Uint32 pixel = pack_color(color, shading);   // same for the whole triangle

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            double px = x + 0.5, py = y + 0.5;   // sample at pixel center
            double alpha = ((p2.y - p3.y) * (px - p3.x) + (p3.x - p2.x) * (py - p3.y)) * inv_det;
            double beta  = ((p3.y - p1.y) * (px - p3.x) + (p1.x - p3.x) * (py - p3.y)) * inv_det;
            double gamma = 1.0 - alpha - beta;

            if (alpha < 0 || beta < 0 || gamma < 0) continue;

            float inv_z = alpha * iz1 + beta * iz2 + gamma * iz3;
            int idx = y * WINDOW_WIDTH + x;

            if (inv_z > zbuffer[idx]) {          
                zbuffer[idx] = inv_z;
                framebuffer[idx] = pixel;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
        WINDOW_HEIGHT, 
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderClear(renderer);

    const char *obj_path = "assets/teapot.obj";
    if (argc > 1) {
        obj_path = argv[1];
    }
    Mesh mesh = load_obj(obj_path);
    if (!mesh.vertices || !mesh.faces) {
        fprintf(stderr, "Failed to load mesh\n");
        return 1;
    }

    normalize_mesh(&mesh);

    // light source
    Vector3D light_source = {0.0, 0.0, -1.0};

    int delay = 1000 / TARGET_FPS;
    int done = 0;
    float angle_x = 0.0f;
    float angle_y = 0.0f;
    double speed = 5.0; // units per second
    Vector3D position = {0.0, 0.0, 4.0};
    Uint32 last_ticks = SDL_GetTicks();
    Uint32 fps_timer = SDL_GetTicks();
    int frame_count = 0;

    Uint32 *framebuffer = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(Uint32));
    float  *zbuffer     = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(float));
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,       // 0xAARRGGBB per pixel
        SDL_TEXTUREACCESS_STREAMING,    // updated every frame
        WINDOW_WIDTH, WINDOW_HEIGHT
    );
    Vector3D *rotated_points = malloc(mesh.vertex_count * sizeof(Vector3D));

    while (!done) {
        SDL_Event event;
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = 1;
            } else if (event.type == SDL_MOUSEWHEEL) {
                position.z += event.wheel.y * 0.5;
            } else if (event.type == SDL_MOUSEMOTION) {
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    angle_x += event.motion.yrel * 0.01;
                    angle_y += event.motion.xrel * 0.01;
                }
            }
        }
        // double dt = 1.0 / TARGET_FPS;        
        // angle += 2 * M_PI * dt / 2;         
        // dz += 1 * dt;
        Uint32 now = SDL_GetTicks();
        double dt = (now - last_ticks) / 1000.0;
        last_ticks = now;

        if (keys[SDL_SCANCODE_A]) position.x -= speed * dt;
        if (keys[SDL_SCANCODE_D]) position.x += speed * dt;
        if (keys[SDL_SCANCODE_W]) position.y += speed * dt;
        if (keys[SDL_SCANCODE_S]) position.y -= speed * dt;

        memset(framebuffer, 0, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(Uint32));
        memset(zbuffer,     0, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(float));

        for (int i = 0; i < mesh.vertex_count; ++i) {
            rotated_points[i] = transform(mesh.vertices[i], angle_x, angle_y, position);
        }

        for (int i = 0; i < mesh.face_count; ++i) {
            int i0 = mesh.faces[i][0];
            int i1 = mesh.faces[i][1];
            int i2 = mesh.faces[i][2];
            Vector3D face_normal = cross_product(
                (Vector3D){
                    rotated_points[i1].x - rotated_points[i0].x,
                    rotated_points[i1].y - rotated_points[i0].y,
                    rotated_points[i1].z - rotated_points[i0].z
                },
                (Vector3D){
                    rotated_points[i2].x - rotated_points[i0].x,
                    rotated_points[i2].y - rotated_points[i0].y,
                    rotated_points[i2].z - rotated_points[i0].z
                }
            );

            if (rotated_points[i0].z < 0.1 ||
                rotated_points[i1].z < 0.1 ||
                rotated_points[i2].z < 0.1) {
                continue;
            }

            // backface culling
            Vector3D to_tri = rotated_points[i0];   // camera is at the origin
            if (dot_product(face_normal, to_tri) >= 0) continue; 

            double ambient = 0.15;
            double diffuse = clamp(dot_product(normalize(face_normal), light_source), 0.0, 1.0);
            double shading = ambient + (1.0 - ambient) * diffuse;
            
            Vector3D triangle[3] = {
                rotated_points[i0],
                rotated_points[i1],
                rotated_points[i2]
            };
            Vector2D projected_triangle[3];
            for (int j = 0; j < 3; ++j) {
                projected_triangle[j] = screen(project(triangle[j]));
            }
            fill_triangle(renderer, framebuffer, zbuffer, projected_triangle[0], projected_triangle[1], projected_triangle[2], triangle[0].z, triangle[1].z, triangle[2].z, 0x00FF00FF, shading);
        }

        SDL_UpdateTexture(texture, NULL, framebuffer, WINDOW_WIDTH * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        // SDL_Delay(delay);

        frame_count++;
        Uint32 t = SDL_GetTicks();
        if (t - fps_timer >= 1000) {
            char title[64];
            snprintf(title, sizeof(title), "%s - %d FPS (%.2f ms/frame)",
                    WINDOW_TITLE, frame_count, (t - fps_timer) / (double)frame_count);
            SDL_SetWindowTitle(window, title);
            frame_count = 0;
            fps_timer = t;
        }
    }

    free(rotated_points);
    free(mesh.vertices);
    free(mesh.faces);
    free(framebuffer);
    free(zbuffer);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
