#ifndef GAME_H
#define GAME_H


#include <stdint.h>
#include <glad/gl.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shared.h"

#define MAX_OVERLAPS 4
#define ENTITY_MAX 1000
#define NPC_MAX 100
#define IMAGES_MAX 2

#define MAX_PARTICLES 2048

struct particle
{
	float X, Y;
	float VX, VY;
	float Life;
	float MaxLife;
	float Size;
	float R, G, B, A;
	bool Active;
};

struct particle_system
{
	particle Particles[MAX_PARTICLES];
	int ParticleCount;
};


struct collision
{
	float X, Y; //Left Bottom
	float Width, Height;
};

enum image_option
{
	Image_Option_None,
	Image_Option_Premultiply,
};

enum entity_type
{
	Entity_Type_None,
	Entity_Type_Player,
	Entity_Type_Npc,
	Entity_Type_Projectile,
	Entity_Type_Other,
};

enum is_player
{
	Is_Player_No,
	Is_Player_Yes,
};

struct image
{
	GLuint TextureID;
	int Width;
	int Height;
	int Pitch;
	int Format;
	//Data is uploaded gpu side and discarded cpu side void* Data;
	char Filename[256];
};

struct entity
{
	v2 LastAccel;
	v2 LastAccelInput;
	float HoldTime;
	
	float X, Y;
	float Width, Height;
	v2 Velocity;
	float ScaleX, ScaleY;
	float Angle;
	collision Collision;
	
	int ImageIndex;
	int Index;
	
	entity_type Type;
	
	float PathingTimer;
	float PathingTimerMax;
	
	float FlashTimer;
	float FlashTimerMax; //This is really a "half timer" how long from 0 = 1 and then we go back 1 = 0
	
	float TurnSpeed;
	float Speed;
};

struct game_state
{
	image Images[IMAGES_MAX];
	int ImageCount;
	entity Entities[ENTITY_MAX];
	int EntityCount;
	int PlayerEntityIndex;
	int PlayerTextureIndex;
	int ParticleTextureIndex;
	bool IsInit;
	
	
	int NpcEntityIndex[NPC_MAX];
	int NpcCount;
	float NpcSpawnTimer;
	float NpcSpawnInterval;
	
	GLuint BackgroundTexture;
	GLuint ParticleTexture;
	
	GLuint ProjLoc;
	GLuint BrickLoc; 
	GLuint ColourLoc;
	
	GLuint BaseColourLoc;
	GLuint LightPosLoc;
	GLuint LightColourLoc;
	GLuint LightRadiusLoc;
	GLuint AmbientLoc;
	
	GLuint FlashColourLoc;
	GLuint FlashAmountLoc;
	
	particle_system ParticleSystem;
	
	
};

struct camera
{
	float X, Y; //Left Bottom
	float Width, Height;
};

struct triangle
{
	v2 Top;
	v2 CW;
	v2 CCW;	
};


#endif