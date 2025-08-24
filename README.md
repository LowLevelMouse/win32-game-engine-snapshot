# Game Engine for Windows

---

## How to Run

Run the `build.bat` file in the project root, then execute `win32.exe`

---

## How to Play
Use the arrow keys to move the triangle player around

Npc triangles will spawn every so often and follow the player

---

## Features (this list is frequently out of date)

- Creates a basic Win32 window with a message pump and callback handler
- Custom memory allocator using an arena built on top of `VirtualAlloc`
  - Supports memory alignment to an arbitrary byte boundary
- Plays a sine wave using XAudio2  
  **Note**: Audio is muted by default. To enable it, remove the `#if` guard on `SourceVoice->Start();` or add `/DAUDIO_PLAYING` to the build batch file.
- File I/O system  
  - Currently writes `"hello!"` to a test file as a demonstration
- Initializes OpenGL on Windows
  - VAO, VBO, and EBO setup  
  - Runtime shader compilation with error checking  
  - Pre-multiplied alpha blending  
  - Nearest-neighbor texture filtering
- Polling user input using `GetAsyncKeyState`
- AABB collision detection with one pass per axis
- Minimal entity system
- Seperation between platform and game code for future cross platform integration
