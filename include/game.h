#ifndef GAME_H
#define GAME_H


#include <stdint.h>
#include <glad/gl.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "shared.h"

#define ENTITY_MAX 1000
#define IMAGES_MAX 2

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
	//Data is uploaded gpu side and discarded cpu side void* Data;
	char Filename[256];
};

struct entity
{

	float X, Y;
	float Width, Height;
	float ScaleX, ScaleY;
	float Theta;
	collision Collision;
	
	int ImageIndex;
	int Index;
};

struct game_state
{
	image Images[IMAGES_MAX];
	int ImageCount;
	entity Entities[ENTITY_MAX];
	int EntityCount;
	int PlayerEntityIndex;
	int PlayerTextureIndex;
	bool IsInit;
	
	GLuint BackgroundTexture;
	
	GLuint ProjLoc;
	GLuint BrickLoc; 
	GLuint ColourLoc;
	
	GLuint BaseColourLoc;
	GLuint LightPosLoc;
	GLuint LightColourLoc;
	GLuint LightRadiusLoc;
	GLuint AmbientLoc;
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