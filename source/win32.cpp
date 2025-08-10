#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> //Contains exit macros
#include <xaudio2.h>

#include <glad/wgl.h>

#include "win32.h"


static GLuint gFboScene = 0;
static GLuint gTexScen = 0;
static GLuint gSceneW = 0, gSceenH = 0;

void UpdateAndRender(memory* Memory, input* Input, GLuint* VAO, GLuint* VBO, GLuint* ShaderProgram);

extern "C" 
{
    // NVIDIA
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    // AMD
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

void InitializeArena(memory_arena* Arena, size_t Size, uint8_t* Base)
{
	Arena->Base = Base;
	Arena->Size = Size;
	Arena->Used = 0;
}

void* PushSize(memory_arena* Arena, size_t Size)
{
	Assert(Arena->Used + Size <= Arena->Size);
	
	void* Result = Arena->Base + Arena->Used;
	Arena->Used += Size;
	
	return Result;
}

void* PushSizeAligned(memory_arena* Arena, size_t Size, size_t Alignment)
{
	Assert(Alignment != 0);
	Assert( (Alignment & (Alignment - 1)) == 0);
	uintptr_t PointerValue = (uintptr_t)(Arena->Base + Arena->Used);
	uintptr_t AlignedPointerValue = AlignPow2(PointerValue, Alignment);
	size_t AlignedUsed = AlignedPointerValue - (uintptr_t)Arena->Base;
	
	Assert(AlignedUsed + Size <= Arena->Size);
	
	void* Result = (void*)AlignedPointerValue;
	Arena->Used += AlignedUsed + Size;
	
	return Result;
}

LRESULT CALLBACK WindowProc(HWND WindowHandle, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;
	switch(Msg)
	{
		case WM_SIZE:
		{
			int Width = LOWORD(LParam);
			int Height = HIWORD(LParam);
			glViewport(0, 0, Width, Height);
			
			Result = 0;
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT Ps;
			BeginPaint(WindowHandle, &Ps);
			EndPaint(WindowHandle, &Ps);
			
			Result = 0;
		}
		break;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			Result = 0;
		}
		break;
		
		default: 
			Result = DefWindowProc(WindowHandle, Msg, WParam, LParam);
	}
	
	return Result;
	
}
	
HWND WindowInitialization(HINSTANCE hInstance)
{
	WNDCLASS WindowClass = {};
	WindowClass.lpfnWndProc = WindowProc;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "MyWindowClass";
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	//WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1); If we weren't handling WM_PAINT and not using OpenGL
	
	ATOM ClassID = RegisterClass(&WindowClass);
	if(ClassID == 0)
	{
		Dialog("Could not register class\nReturn %hu", ClassID); 
		return 0;
	}
	
	HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "Win32", WS_OVERLAPPEDWINDOW, 
									   CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL);
	if(WindowHandle == 0)
	{
		Dialog("Could not create the window \nReturn %p", WindowHandle); 
		return 0;
	}
	
	
	return WindowHandle;
}

HGLRC InitializeOpenGL(HDC WindowDC)
{
	PIXELFORMATDESCRIPTOR Pfd = {};
	Pfd.nSize = sizeof(Pfd);
	Pfd.nVersion = 1;
	Pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	Pfd.iPixelType = PFD_TYPE_RGBA;
	Pfd.cColorBits = 24;
	Pfd.cAlphaBits = 8;
	//Pfd.cDepthBits = 24;
	//Pfd.cStencilBits = 8;
	Pfd.iLayerType = PFD_MAIN_PLANE;
	
	int PixelFormat = ChoosePixelFormat(WindowDC, &Pfd);
	DescribePixelFormat(WindowDC, PixelFormat, sizeof(Pfd), &Pfd); //Used for debugging
	if(!PixelFormat)
	{
		Dialog("Could not get the pixel format\nError: %d", GetLastError());
		return 0;
	}
	BOOL Success = SetPixelFormat(WindowDC, PixelFormat, &Pfd);
	if(!Success)
	{
		Dialog("Could not set the pixel format\nError %d", GetLastError());
		return 0;
	}
	
	HGLRC DummyGLRC;
	DummyGLRC = wglCreateContext(WindowDC);
	if(!DummyGLRC)
	{
		Dialog("Could not create DummyGLRC\nError %d", GetLastError());
		return 0;
	}
	
	Success = wglMakeCurrent(WindowDC, DummyGLRC);
	if(!Success)
	{
		Dialog("Could not make DummyGLRC current\nError %d", GetLastError());
		wglDeleteContext(DummyGLRC);
		return 0;
	}
	
	int SuccessGlad = gladLoadWGL(WindowDC, (GLADloadfunc)wglGetProcAddress);
	if(!SuccessGlad)
	{
		Dialog("Could complete gladLoadWGL\nError %d", SuccessGlad);
		wglDeleteContext(DummyGLRC);
		return 0;
	}
	
	if(!wglCreateContextAttribsARB)
	{
		Dialog("Could not find wglCreateContextAttribsARB\nCan't proceed");
		wglDeleteContext(DummyGLRC);
		return 0;
	}
	
	const int Attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3, 
						   WGL_CONTEXT_MINOR_VERSION_ARB, 3, 
						   WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
						   // WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
						   0};
						   
	HGLRC NewGLContext = wglCreateContextAttribsARB(WindowDC, 0 , Attribs);
	if(!NewGLContext)
	{
		Dialog("Could not create the Core 3.3 OpenGL Context\nCan't proceed");
		wglDeleteContext(DummyGLRC);
		return 0;
	}
	
	wglMakeCurrent(0, 0);
	wglDeleteContext(DummyGLRC);
	
	Success = wglMakeCurrent(WindowDC, NewGLContext);
	if(!Success)
	{
		Dialog("Could not bind the Core 3.3 OpenGL context\nError: %d", Success);
		wglDeleteContext(NewGLContext);
		return 0;
	}
	
	SuccessGlad = gladLoaderLoadGL();
	if(!SuccessGlad)
	{
		Dialog("Could not complete gladLoaderLoadGL\nError: %d", SuccessGlad);
		wglDeleteContext(NewGLContext);
		return 0;
	}
	
	wglSwapIntervalEXT(1);
	
	return NewGLContext;
	
}


void PrepFrame(GLuint* VAO, GLuint* VBO, GLuint* ShaderProgram, float* Vertices, size_t VerticesSize)
{
	unsigned int QuadIndices[] =
	{
		0, 1, 2,
		2, 3, 0
	};
	
	GLuint EBO;
	glGenVertexArrays(1, VAO);
	glBindVertexArray(*VAO);
	
	glGenBuffers(1, VBO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	
	//glBufferData(GL_ARRAY_BUFFER, VerticesSize, Vertices, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, VerticesSize, Vertices, GL_STREAM_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QuadIndices), QuadIndices, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	
	GLint GLSuccess;
	GLchar InfoLog[512];
	
	GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(VertexShader, 1, &VertexShaderSource, 0);
	glCompileShader(VertexShader);
	
	glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &GLSuccess);
	if(!GLSuccess)
	{
		glGetShaderInfoLog(VertexShader, 512, 0, InfoLog);
		Dialog("ERROR: Vertex shader compilation failed\nLog: %s", InfoLog);
	}
	
	GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShader, 1, &FragmentShaderSource, 0);
	glCompileShader(FragmentShader);
	
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &GLSuccess);
	if(!GLSuccess)
	{
		glGetShaderInfoLog(FragmentShader, 512, 0, InfoLog);
		Dialog("ERROR: Fragment shader compilation failed\nLog: %s", InfoLog);
	}
	
	*ShaderProgram = glCreateProgram();
	glAttachShader(*ShaderProgram, VertexShader);
	glAttachShader(*ShaderProgram, FragmentShader);
	glLinkProgram(*ShaderProgram);
	
	glGetProgramiv(*ShaderProgram, GL_LINK_STATUS, &GLSuccess);
	if(!GLSuccess)
	{
		glGetShaderInfoLog(*ShaderProgram, 512, 0, InfoLog);
		Dialog("ERROR: Shader program linking failed\nLog: %s", InfoLog);
	}
	
	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);
	
	glViewport(0, 0, 640, 480);
}


IXAudio2* InitalizeXAudio2(HWND WindowHandle)
{
	CoInitializeEx(0, COINIT_MULTITHREADED); //Need to init COM
	
	IXAudio2* XAudio;
	HRESULT Result = XAudio2Create(&XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	XACheck(Result, "XAudio2Create Failed with %s", 0);

	
	IXAudio2MasteringVoice* MasterVoice;
	Result = XAudio->CreateMasteringVoice(&MasterVoice);
	XACheck(Result, "CreateMasteringVoice Failed with %s", 0);

	return XAudio;
}

int CreateXAudio2Buffer(HWND WindowHandle, IXAudio2* XAudio2, xaudio2_buffer* XAudio2Buffer)
{
	XAudio2Buffer->Format.wFormatTag = WAVE_FORMAT_PCM;
	XAudio2Buffer->Format.nChannels = 2;
	XAudio2Buffer->Format.nSamplesPerSec = 48000;
	XAudio2Buffer->Format.wBitsPerSample = 16;
	XAudio2Buffer->Format.nBlockAlign = (XAudio2Buffer->Format.nChannels * XAudio2Buffer->Format.wBitsPerSample) / 8;
	XAudio2Buffer->Format.nAvgBytesPerSec = XAudio2Buffer->Format.nSamplesPerSec * XAudio2Buffer->Format.nBlockAlign;
	
	HRESULT Result = XAudio2->CreateSourceVoice(&XAudio2Buffer->SourceVoice, &XAudio2Buffer->Format);
	XACheck(Result, "CreateSourceVoice Failed with %s", 0);
	
	return 1;
}

int FillXAudio2Buffer(xaudio2_buffer* XAudio2Buffer)
{	
	int16_t* SampleDest = (int16_t*)XAudio2Buffer->Samples;
	for(int i = 0; i < XAudio2Buffer->SampleCount; i++)
	{
		float Sine = sinf(XAudio2Buffer->Theta);
		int16_t Sample = (int16_t)(Sine * XAudio2Buffer->Volume);
		*SampleDest++ = Sample;
		*SampleDest++ = Sample;
		
		XAudio2Buffer->Theta += XAudio2Buffer->DeltaTheta;
		
		if(XAudio2Buffer->Theta >= 2.0f * Pi32)
		{
			XAudio2Buffer->Theta -= 2.0f * Pi32;
		}
	}
	
	//Relate data to the SourceBuffer and start playing
	XAUDIO2_BUFFER AudioBuffer = {};
	AudioBuffer.AudioBytes = XAudio2Buffer->SampleCount * sizeof(int16_t) * 2;
	AudioBuffer.pAudioData = (BYTE*)XAudio2Buffer->Samples;
	AudioBuffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	AudioBuffer.LoopBegin = 0;
	AudioBuffer.LoopLength = XAudio2Buffer->SampleCount;
	
	XAudio2Buffer->SourceVoice->SubmitSourceBuffer(&AudioBuffer);
	
	return 1;
}

int Win32WriteFile(const LPCWSTR Path, const char* Message)
{
	HANDLE FileWrite = CreateFileW(Path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(FileWrite == INVALID_HANDLE_VALUE)
	{
		DWORD Error = GetLastError(); 
		Dialog("CreateFileW Write failed\nError %d", Error); 
		return 0;
	}
	
	DWORD WriteSize = (DWORD)strlen(Message);
	DWORD Written = 0;
	DWORD TotalWritten = 0;
	
	BOOL Success = false;
	while(TotalWritten < WriteSize)
	{
		Success = WriteFile(FileWrite, Message + TotalWritten, WriteSize - TotalWritten, &Written, 0);
		if(!Success)
		{
			DWORD Error = GetLastError(); 
			Dialog("WriteFile failed\nError %d", Error);
			return 0;
		}
		
		TotalWritten += Written;
	}
	
	if(TotalWritten == WriteSize)
	{
		BOOL Flushed = FlushFileBuffers(FileWrite);
		if(!Flushed)
		{
			DWORD Error = GetLastError(); 
			Dialog("File buffer flush failed\nError %d", Error);
			return 0;
		}
	}
	
	//SetEndOfFile(FileWrite); // If we didn't use CREATE_ALWAYS this would be necessary, and 
	CloseHandle(FileWrite);
		
	return 1;
}

int Win32ReadFile(const LPCWSTR Path)
{
	//Read Test
	HANDLE FileRead = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if(FileRead == INVALID_HANDLE_VALUE)
	{
		DWORD Error = GetLastError(); 
		Dialog("CreateFileW Read failed\nError %d", Error); 
		return 0;
	}

	LARGE_INTEGER LargeInteger;
	BOOL Success = GetFileSizeEx(FileRead, &LargeInteger);
	LONGLONG FileSize = LargeInteger.QuadPart;
	if(!Success)
	{
		DWORD Error = GetLastError(); 
		Dialog("GetFileSizeEx failed\nError %d", Error); 
		CloseHandle(FileRead); 
		return 0;
	}

	//Mainly for 32bit system guard if we want to read the full file into memory at once
	if(FileSize >= SIZE_MAX)
	{
		Dialog("File too large to read into memory");
		CloseHandle(FileRead);
		return 0;
	}
	
	char* Buffer = (char*)malloc((size_t)FileSize + 1); //+ 1 for null terminator
	if (!Buffer)
	{
		Dialog("Malloc failed");
		CloseHandle(FileRead);
		return 0;
	}
	
	LONGLONG TotalRead = 0;
	LONGLONG Remaining = FileSize;
	while(Remaining > 0)
	{
		DWORD BytesRead = 0;
		DWORD BytesRequested = (Remaining >= MAXDWORD) ? MAXDWORD : (DWORD)Remaining;
		Success = ReadFile(FileRead, Buffer + TotalRead, BytesRequested, &BytesRead, 0);
		if(!Success)
		{
			DWORD Error = GetLastError();
			Dialog("ReadFile failed\nError: %d", Error);
			break;
		}
		if(BytesRead == 0)
		{
			Dialog("ReadFile didn't read any bytes");
			break;
		}
		TotalRead += BytesRead;
		Remaining -= BytesRead;
	}
	
	//For Robustness
	if(TotalRead != FileSize)
	{
		Dialog("Read incomplete: %llu bytes of %llu bytes", TotalRead, FileSize);
		free(Buffer);
		CloseHandle(FileRead);
		return 0;
	}
	
	Buffer[TotalRead] = '\0';
	free(Buffer);
	CloseHandle(FileRead);
	
	return 1;
	
}


void PollInput(win32_input* Win32Input)
{
	for(int Key = 0; Key < VK_CODE_MAX; Key++)
	{
		bool IsDown = (GetAsyncKeyState(Key) & 0x8000) != 0;
		Win32Input->IsDown[Key] = IsDown;
	}
}

void MapWin32InputToGame(input* Input, win32_input* Win32Input)
{
	/* enum buttons
	{
		Button_Up,
		Button_Down,
		Button_Left,
		Button_Right,
	};
	*/
	const int KeyMap[Button_Count] = 
	{
		VK_UP,
		VK_DOWN,
		VK_LEFT,
		VK_RIGHT,
	};
	
	for(int Button = 0; Button < Button_Count; Button++)
	{
		Input->IsDown[Button] = Win32Input->IsDown[KeyMap[Button]]; 
	}
}

void StoreInputState(input* Input)
{
	for(int Button = 0; Button < Button_Count; Button++)
	{	
		Input->WasDown[Button] = Input->IsDown[Button];
	}
}

//This needs to be more descriptive and potentially take in dynamic paramaters so we can account for a different layout
void SetQuadVertices(float* Vertices, float X, float Y, float Width, float Height)
{
	//Modify X of each vertex
	Vertices[0] = (X + Width);
	Vertices[4] = X;
	Vertices[8] = X;
	Vertices[12] = (X + Width);

	//Modify Y of each vertex
	Vertices[1] = (Y + Height);
	Vertices[5] = (Y + Height);
	Vertices[9] = Y;
	Vertices[13] = Y;
}

memory InitializeMemory()
{
	memory Memory = {};
	
	uint64_t TotalMemorySize = Megabytes(500);
	uint64_t PermanentMemorySize = Megabytes(250);
	uint64_t TemporaryMemorySize = TotalMemorySize - PermanentMemorySize;
	Assert(PermanentMemorySize + TemporaryMemorySize <= TotalMemorySize);
	
	uint8_t* Base = (uint8_t*)VirtualAlloc(0, TotalMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if(Base)
	{
		InitializeArena(&Memory.PermanentMemory, PermanentMemorySize, Base);
		InitializeArena(&Memory.TemporaryMemory, TemporaryMemorySize, Base + PermanentMemorySize);
		
		Memory.IsInit = true;
	}
	
	return Memory;
}

///
//NEED TO MAKE TIMING ROBUST!!!
///
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;
	
	memory Memory = InitializeMemory();
	if(!Memory.IsInit)
	{
		return EXIT_FAILURE;
	}

	//Window Initialization
	HWND WindowHandle = WindowInitialization(hInstance);
	if(!WindowHandle)
	{
		return EXIT_FAILURE;
	}
	//Init OpenGL
	HDC WindowDC = GetDC(WindowHandle);
	HGLRC NewGLContext = InitializeOpenGL(WindowDC);
	if(!NewGLContext)
	{
		return EXIT_FAILURE;
	}
	
	//Initalize XAudio2 and fill a buffer
	IXAudio2* XAudio2 = InitalizeXAudio2(WindowHandle);
	if(!XAudio2)
	{
		return EXIT_FAILURE;
	}
	
	xaudio2_buffer XAudio2Buffer = {};
	
	int XAudio2Result = CreateXAudio2Buffer(WindowHandle, XAudio2, &XAudio2Buffer);
	if(!XAudio2Result)
	{
		return EXIT_FAILURE;
	}
	
	XAudio2Buffer.Seconds = 1;
	XAudio2Buffer.SampleCount = 48000;
	XAudio2Buffer.Samples = PushSizeAligned(&Memory.PermanentMemory, sizeof(int16_t) * 2 * XAudio2Buffer.SampleCount * XAudio2Buffer.Seconds, 16);
	XAudio2Buffer.Frequency = 320.0f;
	XAudio2Buffer.Volume = 2500;
	XAudio2Buffer.Theta = 0;
	XAudio2Buffer.DeltaTheta = (2.0f * Pi32 * XAudio2Buffer.Frequency) / XAudio2Buffer.Format.nSamplesPerSec;
	
	XAudio2Result = FillXAudio2Buffer(&XAudio2Buffer);
	if(!XAudio2Result)
	{
		return EXIT_FAILURE;
	}
	
#if AUDIO_PLAYING
	XAudio2Buffer.SourceVoice->Start();
#endif

	//Step 4 Write and Read File I/O
	const char* Message = "hello!";
	const LPCWSTR Path = L"test.txt";
	int WriteResult =  Win32WriteFile(Path, Message);
	if(!WriteResult)
	{
		return EXIT_FAILURE;
	}
	int ReadResult = Win32ReadFile(Path);
	if(!ReadResult)
	{
		return EXIT_FAILURE;
	}
	
	
	//Setup OpenGL state
	GLuint VAO, VBO, ShaderProgram;
	float DummyVerticies[] = 
	{
	//  x, y, u, v
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
	};
	int DummyVerticesSize = sizeof(DummyVerticies);
	PrepFrame(&VAO, &VBO, &ShaderProgram, DummyVerticies, DummyVerticesSize);
	
	//Make the window visible and force a repaint
	ShowWindow(WindowHandle, nCmdShow);
	UpdateWindow(WindowHandle);
	
	LARGE_INTEGER PerfFreq = {};
	QueryPerformanceFrequency(&PerfFreq);
	
	double SecondsPerTick = 1.0 / (double)PerfFreq.QuadPart;
	
	win32_input Win32Input = {};
	input Input = {};
	
	LARGE_INTEGER LastCounter = {};
	QueryPerformanceCounter(&LastCounter);
	
	bool Running = true;
	MSG Msg = {};
	while(Running)
	{	
		while(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			if(Msg.message == WM_QUIT) { Running = false; break; }
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		
		PollInput(&Win32Input);
		MapWin32InputToGame(&Input, &Win32Input);
		
		UpdateAndRender(&Memory, &Input, &VAO, &VBO, &ShaderProgram);
		
		StoreInputState(&Input);
		
		SwapBuffers(WindowDC);
	
		//We are skipping the time spent processing messages
		LARGE_INTEGER CurrentCounter = {};
		QueryPerformanceCounter(&CurrentCounter);

		double SecondsElapsed = (double)(CurrentCounter.QuadPart - LastCounter.QuadPart) * SecondsPerTick;
		LastCounter = CurrentCounter;
		
		char Buffer[256];
		_snprintf_s(Buffer, sizeof(Buffer), _TRUNCATE, "Frame Time %.4f s (%.1f FPS)\n", SecondsElapsed, 1.0 / SecondsElapsed);
		OutputDebugStringA(Buffer);
	}
	
	//Cleanup
	XAudio2->Release();
	CoUninitialize();
	
	ReleaseDC(WindowHandle, WindowDC);
	return (int)Msg.wParam;
		
}

const char* VertexShaderSource =
R"(
	#version 330 core
	
	layout (location = 0) in vec2 aPos;
	layout (location = 1) in vec2 aTexCoord;
	
	out vec2 TexCoord;
	out vec2 WorldPos;
	
	uniform mat4 ProjMatrix;
	

	void main()
	{
		WorldPos = aPos;
		gl_Position = ProjMatrix * vec4(aPos, 0.0f, 1.0f);
		TexCoord = aTexCoord;
	}
)";
const char* FragmentShaderSource =
R"(
	#version 330 core
	in vec2 TexCoord;
	in vec2 WorldPos;
	
	out vec4 FragColour;
	
	uniform sampler2D BrickTexture;
	
	uniform vec4 Colour; //For Modulation
	uniform vec2 LightPos;
	uniform vec3 LightColour;
	uniform float LightRadius;
	uniform float Ambient;
	
	void main()
	{
		vec4 TexColour = texture(BrickTexture, TexCoord);
				
		float Distance = distance(WorldPos, LightPos);
		float Attenuation = clamp(1.0 - (Distance / LightRadius), 0.0, 1.0);
		Attenuation *= Attenuation;
		float Brightness = Ambient * (1 - Attenuation) + 1.0f * Attenuation;
		
		vec3 RGB = TexColour.rgb * Colour.rgb * Brightness * LightColour;

		float A = TexColour.a * Colour.a;

		FragColour = vec4(RGB, A);
		//FragColour = Colour;
	}
)";

