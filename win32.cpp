#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <xaudio2.h>
#include <math.h>

//do while is to make this works with edge cases like within and if before an else
//where it binds the macros if to the else instead of the initial if you wanted
//turns multiple statements into one
#define Assert(Expression) do { if (!(Expression)) { *(volatile int*)0 = 0; } } while(0)
#define Check(Expression) do { if(!(Expression)) {return -1;} } while(0)
#define XACheck(Expression, Message) \
	do \
	{ \
		if(FAILED(Expression)) \
		{ \
			char Buffer[256]; \
			snprintf(Buffer, sizeof(Buffer), "%s\nHRESULT: 0x%08X", Message, Result); \
			MessageBoxA(WindowHandle, Buffer, "XAudio2 Error", MB_OK | MB_ICONERROR); \
			return -1;\
		}\
	}while(0) \
	
#define Pi32 3.1415927f

static void* BitmapMemory;
static HBITMAP BitmapHandle;
static HDC BitmapDC;
static BITMAPINFO BitmapInfo;
static int Width = 640;
static int Height = 480;

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
			
			if(NewWidth > 0 && NewHeight > 0)
			{
				Width = NewWidth;
				Height = NewHeight;
				
				if(BitmapHandle)
				{
					DeleteObject(BitmapHandle);
					BitmapInfo.bmiHeader.biWidth = Width;
					BitmapInfo.bmiHeader.biHeight = -Height;
					BitmapHandle = 0;
					BitmapMemory = 0;
					
					BitmapHandle = CreateDIBSection(BitmapDC, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
					Assert(BitmapMemory);
					SelectObject(BitmapDC, BitmapHandle);
				}
				
				InvalidateRect(WindowHandle, NULL, FALSE);
			}
#else
			InvalidateRect(WindowHandle, NULL, FALSE);
#endif
			Result = 0;
		}
		break;
		case WM_PAINT:
		{
			if(BitmapMemory)
			{
				PAINTSTRUCT Paint;
				HDC Hdc = BeginPaint(WindowHandle, &Paint);
				
				uint32_t* Pixels = (uint32_t*)BitmapMemory;
				for(int Y = 0; Y < Height; Y++)
				{
					for(int X = 0; X < Width; X++)
					{
						uint8_t R = (uint8_t)(X % 256);
						uint8_t G = (uint8_t)(Y % 256);
						uint8_t B = 0;
						Pixels[Y * Width + X] = (R << 16) | (G << 8) | B;
					}
				}
				
#if USE_STRETCHDIBITS
				RECT ClientRect;
				GetClientRect(WindowHandle, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				
				StretchDIBits(Hdc, 0, 0, WindowWidth, WindowHeight,
								   0, 0, Width, Height,
								   BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
#else
				BitBlt(Hdc, 0, 0, Width, Height, BitmapDC, 0, 0, SRCCOPY);
#endif
				
				EndPaint(WindowHandle, &Paint);
				
			}
			Result = 0;
		}
		break;
		case WM_DESTROY:
		{
			if(BitmapHandle) { DeleteObject(BitmapHandle); BitmapHandle = 0; }
			if(BitmapDC) 	 { DeleteDC(BitmapDC); BitmapDC = 0; }
			PostQuitMessage(0);
			Result = 0;
		}
		break;
		
		default: 
			Result = DefWindowProc(WindowHandle, Msg, WParam, LParam);
	}
	
	return Result;
	
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;
	
	//Step 1 Window Initialization
	WNDCLASS WindowClass = {};
	WindowClass.lpfnWndProc = WindowProc;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "MyWindowClass";
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	
	ATOM ClassID = RegisterClass(&WindowClass);
	Check(ClassID);
	
	HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "Win32", WS_OVERLAPPEDWINDOW, 
									   CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL);
	Check(WindowHandle);
	
	//Step 2 BitBlt/StretchDIBits preamble
	HDC ScreenDC = GetDC(WindowHandle);
	BitmapDC = CreateCompatibleDC(ScreenDC);
	
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = -Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	
	BitmapHandle = CreateDIBSection(BitmapDC, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
	Assert(BitmapMemory);
	SelectObject(BitmapDC, BitmapHandle);
	
	ReleaseDC(WindowHandle, ScreenDC);
	
	//Step 3 Initalize XAudio2
	CoInitializeEx(0, COINIT_MULTITHREADED); //Need to init COM
	
	IXAudio2* XAudio;
	HRESULT Result = XAudio2Create(&XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	XACheck(Result, "XAudio2Create Failed with %s");
	
	IXAudio2MasteringVoice* MasterVoice;
	Result = XAudio->CreateMasteringVoice(&MasterVoice);
	XACheck(Result, "CreateMasteringVoice Failed with %s");
	
	
	WAVEFORMATEX Format = {};
	Format.wFormatTag = WAVE_FORMAT_PCM;
	Format.nChannels = 2;
	Format.nSamplesPerSec = 48000;
	Format.wBitsPerSample = 16;
	Format.nBlockAlign = (Format.nChannels * Format.wBitsPerSample) / 8;
	Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
	
	IXAudio2SourceVoice* SourceVoice;
	Result = XAudio->CreateSourceVoice(&SourceVoice, &Format);
	XACheck(Result, "CreateSourceVoice Failed with %s");
	
	//Fill a buffer of sound
	const int Samples = 48000;
	int16_t Data[Samples * 2];
	float Frequency = 320.0f;
	float Volume = 2500;
	float Theta = 0;
	float DeltaTheta = (2.0f * Pi32 * Frequency) / Format.nSamplesPerSec;
	
	int16_t* SampleDest = (int16_t*)Data;
	for(int i = 0; i < Samples; i++)
	{
		float Sine = sinf(Theta);
		int16_t Sample = (int16_t)(Sine * Volume);
		*SampleDest++ = Sample;
		*SampleDest++ = Sample;
		
		Theta += DeltaTheta;
		
		if(Theta >= 2.0f * Pi32)
		{
			Theta -= 2.0f * Pi32;
		}
	}
	
	//Relate data to the SourceBuffer and start playing
	XAUDIO2_BUFFER Buffer = {};
	Buffer.AudioBytes = Samples * sizeof(int16_t) * 2;
	Buffer.pAudioData = (BYTE*)Data;
	Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	Buffer.LoopBegin = 0;
	Buffer.LoopLength = Samples;
	
	SourceVoice->SubmitSourceBuffer(&Buffer);
	SourceVoice->Start();
	
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
	XAudio->Release();
	CoUninitialize();
	if(BitmapHandle) { DeleteObject(BitmapHandle); BitmapHandle = 0; }
	if(BitmapDC) 	 { DeleteDC(BitmapDC); BitmapDC = 0; }
	return (int)Msg.wParam;
		
}