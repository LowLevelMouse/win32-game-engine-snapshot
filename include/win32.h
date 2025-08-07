#ifndef ENGINE_WIN32_H
#define ENGINE_WIN32_H


#include <stdint.h>
#include <glad/gl.h>
#include <emmintrin.h>
#include <math.h>

#include "shared.h"

//do-while is to make this works with edge cases like within and if before an else
//where it binds the macros if to the else instead of the initial if you wanted
//turns multiple statements into one
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

#define Check(Expression) do { if(!(Expression)) {return -1;} } while(0)

	
#define Pi32 3.1415927f

#define Kilobytes(Value) ((Value) * 1024ULL)
#define Megabytes(Value) ((Value) * 1024ULL * 1024ULL)
#define Gigabytes(Value) ((Value) * 1024ULL * 1024ULL * 1024ULL)
#define Terabytes(Value) ((Value) * 1024ULL * 1024ULL * 1024ULL * 1024ULL)

#define ARGB32(A, R, G, B) (uint32_t)( (A << 24) | (R << 16) | (G << 8) | B )

#define VK_CODE_MAX 256


#define AlignPow2(Value, Alignment) ( ( (Value) + ((Alignment) - 1) ) & ~((Alignment) - 1) )




#define PushStruct(Arena, type) (type*)PushSize(Arena, sizeof(type))
#define PushArray(Arena, type, Count) (type*)PushSize(Arena, sizeof(type)*(Count))
#define PushStruct16(Arena, type) (type*)PushSizeAligned(Arena, sizeof(type), 16)
#define PushArray16(Arena, type, Count) (type*)PushSizeAligned(Arena, sizeof(type)*(Count), 16) 


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

struct win32_input
{
	bool IsDown[256];
	bool WasDown[256];
};



extern const char* VertexShaderSource;
extern const char* FragmentShaderSource;

#endif