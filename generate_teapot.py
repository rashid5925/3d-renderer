import re

INPUT_FILE = "assets/teapot.obj"
OUTPUT_FILE = "utah_teapot2.h"

vertices = []
faces = []


with open(INPUT_FILE, "r") as f:
    for line in f:
        line = line.strip()

        if not line or line.startswith("#"):
            continue

        parts = line.split()

        # Vertex
        if parts[0] == "v":
            x = float(parts[1])
            y = float(parts[2])
            z = float(parts[3])

            vertices.append((x, y, z))

        # Face
        elif parts[0] == "f":
            indices = []

            for part in parts[1:]:
                # Handles:
                # 1
                # 1/2
                # 1/2/3
                # 1//3
                index = int(part.split("/")[0])

                # OBJ uses 1-based indexing
                if index > 0:
                    index -= 1
                else:
                    index = len(vertices) + index

                indices.append(index)

            # Triangulate polygons using a fan
            for i in range(1, len(indices) - 1):
                faces.append((
                    indices[0],
                    indices[i],
                    indices[i + 1]
                ))


with open(OUTPUT_FILE, "w") as f:

    f.write("#ifndef UTAH_TEAPOT_H\n")
    f.write("#define UTAH_TEAPOT_H\n\n")

    f.write("#define TEAPOT_NUM_POINTS ")
    f.write(str(len(vertices)))
    f.write("\n")

    f.write("#define TEAPOT_NUM_FACES ")
    f.write(str(len(faces)))
    f.write("\n\n")

    f.write("Vector3D teapot_points[TEAPOT_NUM_POINTS] = {\n")

    for x, y, z in vertices:
        f.write(f"    {{{x:.6f}, {y:.6f}, {z:.6f}}},\n")

    f.write("};\n\n")

    f.write("int teapot_faces[TEAPOT_NUM_FACES][3] = {\n")

    for a, b, c in faces:
        f.write(f"    {{{a}, {b}, {c}}},\n")

    f.write("};\n\n")

    f.write("#endif\n")


print(f"Generated: {OUTPUT_FILE}")
print(f"Vertices:  {len(vertices)}")
print(f"Triangles: {len(faces)}")