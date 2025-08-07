#ifndef GAME_H
#define GAME_H


#include <stdint.h>
#include <glad/gl.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shared.h"

#define ENTITY_MAX 1000

struct collision
{
	float X, Y; //Left Bottom
	float Width, Height;
};

struct image
{
	GLuint TextureID;
	int Width;
	int Height;
	int Pitch;
	int Format;
	void* Data;
	const char* Filename;
};

struct entity
{
	image* Image;
	float X, Y;
	float Width, Height;
	float ScaleX, ScaleY;
	float Theta;
	collision Collision;
	
	int Index;
};

struct game_state
{
	entity Entities[ENTITY_MAX];
	int EntityCount;
	entity* PlayerEntity;
	int PlayerEntityIndex;
	bool IsInit;
	
	image Image;
	
	GLuint ProjLoc;
	GLuint BrickLoc; 
	GLuint ColourPos;
};


struct camera
{
	float X, Y; //Left Bottom
	float Width, Height;
};

struct v2
{
	float X;
	float Y;
};

#endif