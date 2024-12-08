<p align="center">
  <img src="https://github.com/user-attachments/assets/bb316e6f-a514-49a1-8fad-d40fde33013c" />
</p>

# CodeSoul

![Console Adventures](https://img.shields.io/badge/Console%20Adventures-green?logo=gnometerminal&logoColor=000000
)  ![WinAPI](https://img.shields.io/badge/WinAPI-blue
)  ![Graphics](https://img.shields.io/badge/Graphics-darkorange?logo=blender&logoColor=white)

An attempt to mimic an OpenGL-like graphics engine using only the WinAPI `BitBlt` function. This project extends `Drawing.cpp` from [Kalmichkov](https://github.com/Milikovv18/Kalmichkov), adding features such as matrix operations, a physics engine, and rudimentary lighting. Rendering and physics computations are performed entirely on the CPU.

---

## ✨ Features

### Rendering
- 2D engine enhanced with perspective matrix generation for 3D rendering.
- Support for custom fragment shaders to implement textures and lighting effects.
- Efficient triangle rasterization with a back-face culling mechanism.

### Physics
- Embedded physics engine with collision detection and response.
- Inertia tensors support for simulating realistic rotations.
- Supports gravitational and spring-like behaviors for objects.

### Interaction
- **WASD**: Move forward/left/right/backward.
- **Space**: Move upward.
- **Shift**: Lower the camera.
- **Mouse Movement**: Look around.

---

## 🗂️ Project Structure

### Camera.cpp
- **`processInput`**: Handles user input for camera movement.
- **`lookAt`**: Generates a view matrix to orient the camera.
- **`perspective`**: Creates a perspective projection matrix.
- **`timeSinceStart`**: Returns the elapsed time since program launch.
- **Key tracking**: Functions like `addTrackingKey`, `removeTrackingKey`, and `setCustomKeysCallback` allow user-defined input handling.

### Canvas.cpp
- Class for managing drawing operations.
- Key functions:
  - **`addFigure`**: Handles geometric figure culling to avoid drawing behind the camera.
  - **`setPixel`**: Primary drawing function for updating the canvas.
  - **`setAlignment`**: Aligns the canvas within the console window.

### Figure.h
- Defines the `IFigure` interface and `Triangle` class.
- Implements barycentric interpolation for color and texture mapping.
- **`setFragmentShader`**: Allows custom shaders for advanced texture rendering.

### Physics.cpp
- Core physics engine functions:
  - **`updatePhysics`**: Main loop for physics computations.
  - **`computeForcesAndTorques`**: Calculates forces and torques.
  - **`checkCollisions`** & **`resolveCollisions`**: Manage collision detection and response.
  - **`star`**: Generates the star operator matrix:
    - ### The Star Operator
      The star operator transforms a vector ![](https://latex.codecogs.com/svg.latex?\vec{v}%20=%20[v_x,%20v_y,%20v_z]^T)
      into a skew-symmetric matrix:
      
      $$ [\vec{v}]_\times = 
      \begin{bmatrix}
      0 & -v_z & v_y \\
      v_z & 0 & -v_x \\
      -v_y & v_x & 0
      \end{bmatrix}. $$
      
      This matrix enables cross-product computation as a matrix-vector multiplication: ![](https://latex.codecogs.com/svg.latex?[\vec{v}]_\times%20\vec{w}%20=%20\vec{v}%20\times%20\vec{w}).

### Texture.h
- Loads bitmaps and retrieves pixel data for rendering.

### Vecd.h
- Implements 2D-4D vector operations including addition, normalization, dot product, and reflection.

### Main.cpp
- Main rendering loop showcasing two triangles:
  - A fixed floor triangle.
  - A draggable triangle attached to a spring.
- Demonstrates camera and physics interactions with gravity and collision mechanics.

### Logger.cpp
- Logs physics computations to `Physics_Log.txt`.


## 🎥 Visual Demo
https://github.com/user-attachments/assets/1872fef4-765a-442e-9461-8014cec9593e

## Example Shaders

### Vertex Shader
```cpp
void vertex_shader(const Vert& in, Vecd<4>& gl_Position, Varying& out) {
    out.texcoord = in.texcoord;
    out.color = in.color;
    gl_Position = in.position;
}
```

### Floor Fragment Shader
```cpp
Texture grassTex(LR"(grass.bmp)");
void floorFrag(const Vecd<4>& pos, const Vecd<4>& texture, Vecd<4>& col) {
    col = grassTex.getPixel(texture.x(), texture.y());
    col *= 0.3; // Ambient light
}
```

### Draggable Triangle Fragment Shader
```cpp
Vecd<4> triagRotatedNormal;
Texture goldTex(LR"(gold.bmp)");
void triagFrag(const Vecd<4>& pos, const Vecd<4>& texture, Vecd<4>& col)
{
    col = goldTex.getPixel(texture.x(), texture.y());
    Vecd<3> norm = normalize(triagRotatedNormal);
    Vecd<3> lightDir = normalize(lightPos - pos);

    // Ambient
    Vecd<4> ambient = 0.1 * lightCol;

    // Diffuse
    double diff = abs(max(dot(norm, lightDir), 0.0));
    Vecd<4> diffuse = 0.6 * diff * lightCol;

    // Specular
    Vecd<3> viewDir = normalize(cam.getPos() - pos);
    Vecd<3> reflectDir = reflect(-lightDir, norm);

    double spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    Vecd<4> specular = 0.5 * spec * lightCol;

    // Result
    col = (ambient + diffuse) * col + specular;
}
```


## ⚙️ Build Instructions

To build CodeMelody, use the **MSVC x64 compiler**:

1. Open the project file `CodeMelody.vcxproj` in Visual Studio.
2. Compile the project in the desired configuration (Debug or Release).
3. Run the resulting executable.


## 📜 Acknowledgments

- Code partially taken from [Kalmichkov](https://github.com/Milikovv18/Kalmichkov)’s `Drawing.cpp`.
- Draggable triangle inertia tensor sourced from online references (its computation in code goes far out of project scope).
