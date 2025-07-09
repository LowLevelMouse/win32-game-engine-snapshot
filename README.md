# win32-demo

Minimal Win32 API demo written in C — no libraries or frameworks.  
**Note**: This is a public snapshot of my private game engine repository.

---

## How to Run

Run the `build.bat` file in the project root, then execute `win32.exe`.

---

## Features

- Creates a basic Win32 window with a message pump and callback handler
- Custom memory allocator using an arena built on top of `VirtualAlloc`
  - Supports memory alignment to an arbitrary byte boundary
- Plays a sine wave using XAudio2  
  **Note**: This is muted by default. `SourceVoice->Start();` is `#if`-guarded — remove the guard to enable audio.
- File I/O system  
  - Currently writes `"hello!"` to a test file as a demonstration
- Initializes OpenGL on Windows and renders a brick texture with transparency  
  - VAO, VBO, and EBO setup  
  - Runtime shader compilation with error checking  
  - Pre-multiplied alpha blending  
  - Nearest-neighbor texture filtering
