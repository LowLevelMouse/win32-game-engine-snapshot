# win32-demo

Minimal Win32 API demo using C. No libraries or frameworks.  
**Note**: This is a public snapshot of my private game engine repository.

## How to Run

Run the batch file named `build.bat` in the project root, then run `win32.exe`.

## Features

- Creates a basic Win32 window with callback and message pump
- Plays a sine wave with XAudio2  
  **Note**: This is muted by default. `SourceVoice->Start();` is `#if` guarded — feel free to remove the guard to enable sound.
- Robust file writing and reading  
  Currently writes `"hello!"` to a test file for demonstration.
- Initializes OpenGL on Windows and displays a brick texture with low alpha  
  - VAO, VBO, and EBO setup  
  - Shader compilation with runtime compile error checking  
  - Pre-multiplied alpha blending  
  - Texture filtering
