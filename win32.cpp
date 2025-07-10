#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h> //Contains exit macros
#include <xaudio2.h>
#include <math.h>
#include <emmintrin.h>

#include <glad/wgl.h>
#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "win32.h"

extern "C" 
{
    // NVIDIA
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    // AMD
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

void InitalizeArena(memory_arena* Arena, size_t Size)
{
	Arena->Base = (uint8_t*)VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
	//WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1); If we weren't handling WM_PAINT
	
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
	Pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	Pfd.nVersion = 1;
	Pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	Pfd.iPixelType = PFD_TYPE_RGBA;
	Pfd.cColorBits = 24;
	Pfd.cAlphaBits = 8;
	//Pfd.cDepthBits = 24;
	//Pfd.cStencilBits = 8;
	Pfd.iLayerType = PFD_MAIN_PLANE;
	
	int PixelFormat = ChoosePixelFormat(WindowDC, &Pfd);
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
	
	//wglSwapIntervalEXT(1)
	
	return NewGLContext;
	
}

void PrepFrame(GLuint* VAO, GLuint* ShaderProgram)
{
	float Vertices[] = 
	{
		0.5f,  0.5f, 1.0f, 1.0f,//Top right
	   -0.5f,  0.5f, 0.0f, 1.0f,//Top left
	   -0.5f, -0.5f, 0.0f, 0.0f,//Bottom left
	    0.5f, -0.5f, 1.0f, 0.0f,//Bottom right

	};
	
	unsigned int Indices[] =
	{
		0, 1, 2,
		2, 3, 0
	};
	
	GLuint VBO, EBO;
	glGenVertexArrays(1, VAO);
	glBindVertexArray(*VAO);
	
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
	
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

//Data should not have padding as far as I'm aware
image LoadImage(const char* Filename)
{
	image Image = {};
	int Channels;
	int ChanneslsForce = 4;
	stbi_set_flip_vertically_on_load(1);
	unsigned char* Data = stbi_load(Filename, &Image.Width, &Image.Height, &Channels, ChanneslsForce);
	if(Data)
	{
		Image.Data = Data;
		Image.Filename = Filename;
		Image.Format = GL_RGBA;
		Image.Pitch = Image.Width * ChanneslsForce;
	}
	return Image;
}

float UInt255To1(uint32_t Value)
{
	float Result = Value / 255.0f;
	return Result;
}

uint8_t RoundFloatToUInt8(float Value)
{
	uint8_t Result = (uint8_t)(Value + 0.5f);
	return Result;
}

uint8_t MinUInt8(uint8_t A, uint8_t B)
{
	uint8_t Result = A;
	if(A > B)
	{
		Result = B;
	}
	
	return Result;
}

uint8_t MaxUint8(uint8_t A, uint8_t B)
{
	uint8_t Result = A;
	if(A < B)
	{
		Result = B;
	}
	
	return Result;
}

void PremultiplyAlpha(image* Image)
{
	uint8_t* Data = (uint8_t*)Image->Data;
	for(int Y = 0; Y < Image->Height; Y++)
	{
		uint32_t* Row = (uint32_t*)Data;
		for(int X = 0; X < Image->Width; X++)
		{
			uint32_t* Pixel = Row + X;
			uint8_t R255 = (uint8_t)((*Pixel) >> 0);
			uint8_t G255 = (uint8_t)((*Pixel) >> 8);
			uint8_t B255 = (uint8_t)((*Pixel) >> 16);
			uint8_t A255 = (uint8_t)((*Pixel) >> 24);
			float A1 = UInt255To1(A255);
			
			R255 = MaxUint8(MinUInt8(RoundFloatToUInt8(R255 * A1), 255), 0);
			G255 = MaxUint8(MinUInt8(RoundFloatToUInt8(G255 * A1), 255), 0);
			B255 = MaxUint8(MinUInt8(RoundFloatToUInt8(B255 * A1), 255), 0);

			uint32_t NewPixel = (A255 << 24) | (B255 << 16) | (G255 << 8) | (R255 << 0);
			*Pixel = NewPixel;
		}
		
		Data += Image->Pitch;
	}
	
}

void PremultiplyAlpha_4x(image* Image)
{
	__m128i I255_4x = _mm_set1_epi32(255);
	__m128i I0_4x =  _mm_set1_epi32(0);
	__m128i MaskFF = _mm_set1_epi32(0xFF);
			
	uint8_t* Data = (uint8_t*)Image->Data;
	for(int Y = 0; Y < Image->Height; Y++)
	{
		uint32_t* Row = (uint32_t*)Data;
		int X = 0;
		for(; X <= Image->Width - 4; X+= 4)
		{
			uint32_t* Pixel = Row + X;
			__m128i Pixel_4x = _mm_loadu_si128((__m128i*)Pixel);
			
			__m128i Red_4x = _mm_and_si128(Pixel_4x, MaskFF);
			__m128i Green_4x = _mm_and_si128(_mm_srli_epi32(Pixel_4x, 8), MaskFF);
			__m128i Blue_4x = _mm_and_si128(_mm_srli_epi32(Pixel_4x, 16), MaskFF);
			__m128i Alpha_4x = _mm_and_si128(_mm_srli_epi32(Pixel_4x, 24), MaskFF);

			__m128 A1_4x = _mm_mul_ps(_mm_cvtepi32_ps(Alpha_4x), _mm_set1_ps(1.0f / 255.0f));
			
			__m128 Red255_4x_Premultiply = _mm_mul_ps(_mm_cvtepi32_ps(Red_4x), A1_4x);
			__m128i Red255_4x_Rounded = _mm_cvtps_epi32(Red255_4x_Premultiply);
			__m128i Red255_4x_Clamped = _mm_max_epi32(_mm_min_epi32(Red255_4x_Rounded, I255_4x), I0_4x);
			
			__m128 Green255_4x_Premultiply = _mm_mul_ps(_mm_cvtepi32_ps(Green_4x), A1_4x);
			__m128i Green255_4x_Rounded = _mm_cvtps_epi32(Green255_4x_Premultiply);
			__m128i Green255_4x_Clamped = _mm_max_epi32(_mm_min_epi32(Green255_4x_Rounded, I255_4x), I0_4x);
			__m128i Green255_4x_Shifted = _mm_slli_epi32(Green255_4x_Clamped, 8);
			
			__m128 Blue255_4x_Premultiply = _mm_mul_ps(_mm_cvtepi32_ps(Blue_4x), A1_4x);
			__m128i Blue255_4x_Rounded = _mm_cvtps_epi32(Blue255_4x_Premultiply);
			__m128i Blue255_4x_Clamped =_mm_max_epi32(_mm_min_epi32(Blue255_4x_Rounded, I255_4x), I0_4x);
			__m128i Blue255_4x_Shifted = _mm_slli_epi32(Blue255_4x_Clamped, 16);
			
			__m128i Alpha255_4x_Shifted = _mm_slli_epi32(Alpha_4x, 24);
			
			__m128i Packed = _mm_or_si128(Alpha255_4x_Shifted, _mm_or_si128(Blue255_4x_Shifted, _mm_or_si128(Green255_4x_Shifted, Red255_4x_Clamped)));
			_mm_storeu_si128((__m128i*)Pixel, Packed);

		}
		
		for(; X < Image->Width; X++)
		{
			uint32_t* Pixel = Row + X;
			uint8_t R255 = (uint8_t)((*Pixel) >> 0);
			uint8_t G255 = (uint8_t)((*Pixel) >> 8);
			uint8_t B255 = (uint8_t)((*Pixel) >> 16);
			uint8_t A255 = (uint8_t)((*Pixel) >> 24);
			float A1 = UInt255To1(A255);
			
			R255 = MaxUint8(MinUInt8(RoundFloatToUInt8(R255 * A1), 255), 0);
			G255 = MaxUint8(MinUInt8(RoundFloatToUInt8(G255 * A1), 255), 0);
			B255 = MaxUint8(MinUInt8(RoundFloatToUInt8(B255 * A1), 255), 0);

			uint32_t NewPixel = (A255 << 24) | (B255 << 16) | (G255 << 8) | (R255 << 0);
			*Pixel = NewPixel;
		}

		Data += Image->Pitch;
	}
	
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
	
	//Write could be partial, but in almost all cases it won't be, no need for loop
	DWORD WriteSize = (DWORD)strlen(Message);
	DWORD Written;
	BOOL Success = WriteFile(FileWrite, Message, WriteSize, &Written, 0);
	if(!Success || Written != WriteSize)
	{
		DWORD Error = GetLastError(); 
		Dialog("WriteFile failed\nError %d", Error);
	}
	else 
	{
		FlushFileBuffers(FileWrite); 
		//SetEndOfFile(FileWrite); // If we didn't use CREATE_ALWAYS this would be necessary, and SetFilePointer 
	}
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

void OrthographicProjectionMatrix(float* ProjMatrix, int Left, int Bottom, int Right, int Top)
{
	float XScale = 2.0f / (Right - Left);
	float YScale = 2.0f / (Top - Bottom);
	float XTrans = -(Right + Left) / (float)(Right - Left);
	float YTrans = -(Top + Bottom) / (float)(Top - Bottom);
	
	ProjMatrix[0] = XScale; ProjMatrix[1] = 0; 		 ProjMatrix[2] = XTrans;
	ProjMatrix[3] = 0; 		ProjMatrix[4] = YScale;  ProjMatrix[5] = YTrans;
	ProjMatrix[6] = 0; 		ProjMatrix[7] = 0; 		 ProjMatrix[8] = 0;
						  
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;
	
	memory_arena MemoryArena = {};
	InitalizeArena(&MemoryArena, Megabytes(500));

	//Step 1 Window Initialization
	HWND WindowHandle = WindowInitialization(hInstance);
	if(!WindowHandle)
	{
		return EXIT_FAILURE;
	}
	//Step 2 BitBlt/StretchDIBits preamble
	HDC WindowDC = GetDC(WindowHandle);
	HGLRC NewGLContext = InitializeOpenGL(WindowDC);
	if(!NewGLContext)
	{
		return EXIT_FAILURE;
	}
	
	//Step 3 Initalize XAudio2 and fill a buffer
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
	XAudio2Buffer.Samples = PushSizeAligned(&MemoryArena, sizeof(int16_t) * 2 * XAudio2Buffer.SampleCount * XAudio2Buffer.Seconds, 16);
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
	
	//Make the window visible and force a repaint
	ShowWindow(WindowHandle, nCmdShow);
	UpdateWindow(WindowHandle);
	
	GLuint VAO, ShaderProgram;
	PrepFrame(&VAO, &ShaderProgram);
	image Image = LoadImage("brick_test.png");
	if(!Image.Data)
	{
		Dialog("Could not load image");
		return EXIT_FAILURE;
	}
	
#if 0
	PremultiplyAlpha(&Image);
#else
	PremultiplyAlpha_4x(&Image);
#endif
	
	glGenTextures(1, &Image.TextureID);
	glBindTexture(GL_TEXTURE_2D, Image.TextureID);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Image.Width, Image.Height, 0, Image.Format, GL_UNSIGNED_BYTE, Image.Data);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // For premultiplied alpha
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
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
		
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(ShaderProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Image.TextureID);
		
		glUniform1i(glGetUniformLocation(ShaderProgram, "BrickTexture"), 0);
		glBindVertexArray(VAO);
		//glDrawArrays(GL_TRIANGLES, 0, 6);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		SwapBuffers(WindowDC);
		
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
	void main()
	{
		gl_Position = vec4(aPos, 0.0f, 1.0f);
		TexCoord = aTexCoord;
	}
)";
const char* FragmentShaderSource =
R"(
	#version 330 core
	
	out vec4 FragColour;
	in vec2 TexCoord;
	uniform sampler2D BrickTexture;
	void main()
	{
		float AlphaScale = 0.2f;
		//FragColour = vec4(TexCoord, 0.0f, 1.0f);
		FragColour = texture(BrickTexture, TexCoord);
		FragColour.rgb *= AlphaScale;
		FragColour.a *= AlphaScale;
	}
)";