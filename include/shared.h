#ifndef SHARED_H
#define SHARED_H

struct game_state;

#define Pi32 3.1415927f
#define Assert(Expression) do { if (!(Expression)) { *(volatile int*)0 = 0; } } while(0)
	
struct memory_arena
{
	size_t Size;
	size_t Used;
	uint8_t* Base;
};

struct memory
{
	memory_arena PermanentMemory;
	memory_arena TemporaryMemory;
	game_state* GameState;
	bool IsInit;
	bool IsGameStateInit;
};


void* PushSize(memory_arena* Arena, size_t Size);
void* PushSizeAligned(memory_arena* Arena, size_t Size, size_t Alignment);

#define AlignPow2(Value, Alignment) ( ( (Value) + ((Alignment) - 1) ) & ~((Alignment) - 1) )

#define PushStruct(Arena, type) (type*)PushSize(Arena, sizeof(type))
#define PushArray(Arena, type, Count) (type*)PushSize(Arena, sizeof(type)*(Count))
#define PushStruct16(Arena, type) (type*)PushSizeAligned(Arena, sizeof(type), 16)
#define PushArray16(Arena, type, Count) (type*)PushSizeAligned(Arena, sizeof(type)*(Count), 16) 

enum button
{
	Button_Up,
	Button_Down,
	Button_Left,
	Button_Right,
	Button_Space,
	
	Button_Count
};

struct input
{
	bool IsDown[Button_Count];
	bool WasDown[Button_Count];
};

struct v2
{
	float X;
	float Y;
};


void UpdateAndRender(memory* Memory, input* Input, GLuint* VAO, GLuint* VBO, GLuint* ShaderProgram, float DT);

#endif