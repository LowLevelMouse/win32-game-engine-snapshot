#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h> //Contains exit macros
#include <xaudio2.h>
#include <math.h>
#include <emmintrin.h>

//do while is to make this works with edge cases like within and if before an else
//where it binds the macros if to the else instead of the initial if you wanted
//turns multiple statements into one
#define Assert(Expression) do { if (!(Expression)) { *(volatile int*)0 = 0; } } while(0)
#define Check(Expression) do { if(!(Expression)) {return -1;} } while(0)
#define Dialog(...) \
	do\
	{\
		char _Buffer[256]; \
		snprintf(_Buffer, sizeof(_Buffer), __VA_ARGS__); \
		MessageBoxA(0, _Buffer, "Error", MB_OK | MB_ICONERROR); \
	}while(0)
	
#define XACheck(Expression, Message, ReturnCode) \
	do \
	{ \
		if(FAILED(Expression)) \
		{ \
			char _Buffer[256]; \
			snprintf(_Buffer, sizeof(_Buffer), "%s\nHRESULT: 0x%08X", Message, Expression); \
			MessageBoxA(WindowHandle, _Buffer, "XAudio2 Error", MB_OK | MB_ICONERROR); \
			return ReturnCode; \
		}\
	}while(0) \
	
#define Pi32 3.1415927f

#define Kilobytes(Value) ((Value) * 1024ULL)
#define Megabytes(Value) ((Value) * 1024ULL * 1024ULL)
#define Gigabytes(Value) ((Value) * 1024ULL * 1024ULL * 1024ULL)
#define Terabytes(Value) ((Value) * 1024ULL * 1024ULL * 1024ULL * 1024ULL)



#define PushStruct(Arena, type) (type*)PushSize(Arena, sizeof(type))
#define PushArray(Arena, type, Count) (type*)PushSize(Arena, sizeof(type)*(Count))

#define PushStruct16(Arena, type) (type*)PushSizeAligned(Arena, sizeof(type), 16)
#define PushArray16(Arena, type, Count) (type*)PushSizeAligned(Arena, sizeof(type)*(Count), 16)

#define AlignPow2(Value, Alignment) ( ( (Value) + ((Alignment) - 1) ) & ~((Alignment) - 1) ) 

struct memory_arena
{
	size_t Size;
	size_t Used;
	uint8_t* Base;
};

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
	

struct backbuffer
{
	void* BitmapMemory;
	HBITMAP BitmapHandle;
	HDC BitmapDC;
	BITMAPINFO BitmapInfo;
	int Width;
	int Height;
	int Pitch;
};

struct xaudio2
{
	IXAudio2* Engine;
	IXAudio2MasteringVoice* MasterVoice;
};

struct xaudio2_buffer
{
	IXAudio2SourceVoice* SourceVoice;
	WAVEFORMATEX Format;
	
	int Seconds;
	int SampleCount;
	void* Samples;
	float Frequency;
	float Volume;
	float Theta;
	float DeltaTheta;
};

static backbuffer Backbuffer = {}; //Init to 0 since its a global anyway but just for clarity

int RecreateBackbuffer(int NewWidth, int NewHeight)
{
	//Trying to stop WM_PAINT ASAP
	Backbuffer.BitmapMemory = 0;
	
	if(Backbuffer.BitmapHandle)
	{
		DeleteObject(Backbuffer.BitmapHandle);
		Backbuffer.BitmapHandle = 0;
	}
	
	Backbuffer.Width = NewWidth;
	Backbuffer.Height = NewHeight;
	Backbuffer.Pitch = Backbuffer.Width * (Backbuffer.BitmapInfo.bmiHeader.biBitCount / 8);
	Backbuffer.BitmapInfo.bmiHeader.biWidth = Backbuffer.Width;
	Backbuffer.BitmapInfo.bmiHeader.biHeight = -Backbuffer.Height;
	
	Backbuffer.BitmapHandle = CreateDIBSection(Backbuffer.BitmapDC, &Backbuffer.BitmapInfo, DIB_RGB_COLORS, &Backbuffer.BitmapMemory, 0, 0);
	if(!Backbuffer.BitmapMemory || !Backbuffer.BitmapHandle)
	{
		Backbuffer.BitmapMemory = 0;
		Dialog("Could not CreateDIBSection for backbuffer");
		if(Backbuffer.BitmapHandle)
		{
			DeleteObject(Backbuffer.BitmapHandle);
			Backbuffer.BitmapHandle = 0;
		}
		return 0;
	}
	Assert(Backbuffer.BitmapMemory);
	
	HGDIOBJ Result = SelectObject(Backbuffer.BitmapDC, Backbuffer.BitmapHandle);
	if(!Result || Result == HGDI_ERROR)
	{
		DeleteObject(Backbuffer.BitmapHandle);
		Backbuffer.BitmapMemory = 0;
		Backbuffer.BitmapHandle = 0;
		return 0;
	}
	
	return 1;
				
}


LRESULT CALLBACK WindowProc(HWND WindowHandle, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;
	switch(Msg)
	{
		case WM_SIZE:
		{
#ifndef USE_STRETCHDIBITS
			RECT Rect;
			GetClientRect(WindowHandle, &Rect);
			int NewWidth = Rect.right - Rect.left;
			int NewHeight = Rect.bottom - Rect.top;
			
			if((NewWidth > 0 && NewHeight > 0) || !Backbuffer.BitmapHandle)
			{
				int BackbufferResult = RecreateBackbuffer(NewWidth, NewHeight);
				if(!BackbufferResult)
				{
					Dialog("WM_SIZE RecreateBackbuffer failed");
				}
				else
				{
					InvalidateRect(WindowHandle, NULL, FALSE);
				}
			}
#else
			InvalidateRect(WindowHandle, NULL, FALSE);
#endif
			Result = 0;
		}
		break;
		case WM_PAINT:
		{
			if(Backbuffer.BitmapMemory)
			{
				PAINTSTRUCT Paint;
				HDC Hdc = BeginPaint(WindowHandle, &Paint);
				
				//BI_RGB + 32bpp Memory layout wants BGRA so register wants ARGB

#if 0
				uint32_t Colour = 0xFFFF0000;
				__m128i FillColour = _mm_set1_epi32(Colour);
#endif

#if 1
				uint8_t* Pixels = (uint8_t*)(Backbuffer.BitmapMemory);

				
				for(int Y = 0; Y < Backbuffer.Height; Y++)
				{
					uint32_t* Row = (uint32_t*)Pixels;
					int X = 0;
					
					uint8_t R = (uint8_t)(X % 256);
					uint8_t G = (uint8_t)(Y % 256);
					uint8_t B = 0;
					uint32_t Colour = (0xFF << 24) | (R << 16) | (G << 8) | B;
					
					for(; X <= Backbuffer.Width - 4; X += 4)
					{
						uint32_t Colour1 = (0xFF << 24) | (R << 16) | (G << 8) | B;
						R = (uint8_t)((X + 1) % 256);
						uint32_t Colour2 = (0xFF << 24) | (R << 16) | (G << 8) | B;
						R = (uint8_t)((X + 2) % 256);
						uint32_t Colour3 = (0xFF << 24) | (R << 16) | (G << 8) | B;
						R = (uint8_t)((X + 3) % 256);
						uint32_t Colour4 = (0xFF << 24) | (R << 16) | (G << 8) | B;
						__m128i FillColour = _mm_set_epi32(Colour4, Colour3, Colour2, Colour1);
						_mm_storeu_si128((__m128i*)(Row + X), FillColour);
					}
					
					for(; X < Backbuffer.Width; X++)
					{
						R = (uint8_t)(X % 256);
						G = (uint8_t)(Y % 256);
						B = 0;
						Colour = (0xFF << 24) | (R << 16) | (G << 8) | B;
						
						Row[X] = Colour;
					}
					
					Pixels += Backbuffer.Pitch;
				}
#else
				uint32_t* Pixels = (uint32_t*)Backbuffer.BitmapMemory;
				for(int Y = 0; Y < Backbuffer.Height; Y++)
				{
					for(int X = 0; X < Backbuffer.Width; X++)
					{
						uint8_t R = (uint8_t)(X % 256);
						uint8_t G = (uint8_t)(Y % 256);
						uint8_t B = 0;
						Pixels[Y * Backbuffer.Width + X] = (R << 16) | (G << 8) | B;
					}
				}
#endif
				
#if USE_STRETCHDIBITS
				RECT ClientRect;
				GetClientRect(WindowHandle, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				
				StretchDIBits(Hdc, 0, 0, WindowWidth, WindowHeight,
								   0, 0, Backbuffer.Width, Backbuffer.Height,
								   Backbuffer.BitmapMemory, &Backbuffer.BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
#else
				BitBlt(Hdc, 0, 0, Backbuffer.Width, Backbuffer.Height, Backbuffer.BitmapDC, 0, 0, SRCCOPY);
#endif
				
				EndPaint(WindowHandle, &Paint);

				
			}
			Result = 0;
		}
		break;
		case WM_DESTROY:
		{
			if(Backbuffer.BitmapHandle) { DeleteObject(Backbuffer.BitmapHandle); Backbuffer.BitmapHandle = 0; }
			if(Backbuffer.BitmapDC) 	 { DeleteDC(Backbuffer.BitmapDC); Backbuffer.BitmapDC = 0; }
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

int BackbufferInitialization(HWND WindowHandle)
{
	Backbuffer.Width = 647;
	Backbuffer.Height = 480;
	
	Backbuffer.BitmapInfo.bmiHeader.biSize = sizeof(Backbuffer.BitmapInfo.bmiHeader);
	Backbuffer.BitmapInfo.bmiHeader.biWidth = Backbuffer.Width;
	Backbuffer.BitmapInfo.bmiHeader.biHeight = -Backbuffer.Height;
	Backbuffer.BitmapInfo.bmiHeader.biPlanes = 1;
	Backbuffer.BitmapInfo.bmiHeader.biBitCount = 32;
	Backbuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	Backbuffer.Pitch = Backbuffer.Width * (Backbuffer.BitmapInfo.bmiHeader.biBitCount / 8);
	//Assert((Backbuffer.Pitch & 16) == 0);
	
#ifndef USE_STRETCHDIBITS
	
	HDC ScreenDC = GetDC(WindowHandle);
	Backbuffer.BitmapDC = CreateCompatibleDC(ScreenDC);
	if(!Backbuffer.BitmapDC)
	{
		Dialog("Could not CreateCompatibleDC for backbuffer");
		ReleaseDC(WindowHandle, ScreenDC);
		return 0;
	}
	
	Backbuffer.BitmapHandle = CreateDIBSection(Backbuffer.BitmapDC, &Backbuffer.BitmapInfo, DIB_RGB_COLORS, &Backbuffer.BitmapMemory, 0, 0);
	if(!Backbuffer.BitmapMemory || !Backbuffer.BitmapHandle)
	{
		Dialog("Could not CreateDIBSection for backbuffer");
		if(Backbuffer.BitmapHandle)
		{
			DeleteObject(Backbuffer.BitmapHandle);
		}
		DeleteDC(Backbuffer.BitmapDC);
		ReleaseDC(WindowHandle, ScreenDC);
		return 0;
	}
	Assert(Backbuffer.BitmapMemory);
	
	HGDIOBJ Result = SelectObject(Backbuffer.BitmapDC, Backbuffer.BitmapHandle); //Not working with a region so no HGDI_ERROR check
	if(!Result)
	{
		Dialog("Could not SelectObject for the backbuffer");
		DeleteObject(Backbuffer.BitmapHandle);
		DeleteDC(Backbuffer.BitmapDC);
		ReleaseDC(WindowHandle, ScreenDC);
		return 0;
	}
	
	ReleaseDC(WindowHandle, ScreenDC);
#else
	(void)WindowHandle;

	int Size = Backbuffer.Height * Backbuffer.Pitch;
	Backbuffer.BitmapMemory = (uint8_t*)VirtualAlloc(0, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	Assert(((uintptr_t)Backbuffer.BitmapMemory % 16) == 0 );
	Assert(Backbuffer.BitmapMemory);
#endif
	
	return 1;
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
	int BackbufferResult = BackbufferInitialization(WindowHandle);
	if(!BackbufferResult)
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
	
#if 1
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
		
	}
	
	//Cleanup
	XAudio2->Release();
	CoUninitialize();
	if(Backbuffer.BitmapHandle) 
	{ 
		DeleteObject(Backbuffer.BitmapHandle); 
		Backbuffer.BitmapHandle = 0; 
	}
	if(Backbuffer.BitmapDC)
	{ 
		DeleteDC(Backbuffer.BitmapDC); 
		Backbuffer.BitmapDC = 0; 
	}
	return (int)Msg.wParam;
		
}