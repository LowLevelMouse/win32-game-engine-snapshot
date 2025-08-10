#include "game.h"

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

void PremultiplyAlpha(image* Image, void* RawData)
{
	uint8_t* Data = (uint8_t*)RawData;
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

void PremultiplyAlpha_4x(image* Image, void* RawData)
{
	__m128i I255_4x = _mm_set1_epi32(255);
	__m128i I0_4x =  _mm_set1_epi32(0);
	__m128i MaskFF = _mm_set1_epi32(0xFF);
			
	uint8_t* Data = (uint8_t*)RawData;
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
		//NOTE:The image struct is not fully filled yet here
#if 0
		PremultiplyAlpha(&Image, Data); 
#else
		PremultiplyAlpha_4x(&Image, Data);
#endif
	
		//Image.Data = Data;
		snprintf(Image.Filename, sizeof(Image.Filename), "%s", Filename);
		Image.Format = GL_RGBA;
		Image.Pitch = Image.Width * ChanneslsForce;
		
		glGenTextures(1, &Image.TextureID);
		glBindTexture(GL_TEXTURE_2D, Image.TextureID);
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//OpenGL prefers adding explicity the bpp like with "8" here
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Image.Width, Image.Height, 0, Image.Format, GL_UNSIGNED_BYTE, Data);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glGenerateMipmap(GL_TEXTURE_2D);
		
		stbi_image_free(Data);
	}
	return Image;
}

collision StandardCollision(entity Entity)
{
	collision Collision = {};
	
	Collision.X = Entity.X;
	Collision.Y = Entity.Y;
	Collision.Width = Entity.Width;
	Collision.Height = Entity.Height;
	
	return Collision;
}

int AddEntity(entity* Entities, int* EntityCount, float X, float Y, float Width, float Height)
{
	int EntityIndex = *EntityCount;
	
	entity Entity = {};
	Entity.X = X;
	Entity.Y = Y;
	Entity.Width = Width;
	Entity.Height = Height;
	Entity.Collision = StandardCollision(Entity);
	Entity.Index = EntityIndex;
	
	Entities[(*EntityCount)++] = Entity;
	
	return EntityIndex;
}

int AddImage(image* Images, int* ImageCount, char* Filename)
{
	int ImageIndex = *ImageCount;
	
	image Image = LoadImage(Filename);
	
	Images[(*ImageCount)++] = Image;
	
	return ImageIndex;
}

void PopulateWorld(entity* Entities, int* EntityCount)
{
	float Width = 400;
	float Height = 200;
	float PadX = 300;
	float PadY = 150;
	
	float XStart =  Width + 100;
	float X = XStart;
	float Y = 0;
	
	for(int EntityY = 0; EntityY < 10; EntityY++)
	{
		X = XStart;
		for(int EntityX = 0; EntityX < 10; EntityX++)
		{
			AddEntity(Entities, EntityCount, X, Y, Width, Height);
			
			X+= Width + PadX;
		}
		
		Y+= Height + PadY;
	}
}

bool CollisionCheckPair(float Epsilon, float AX, float AY, float AWidth, float AHeight,
					float BX, float BY, float BWidth, float BHeight)
{
	bool Collided = false;
	//Pos + Dim will be 1 greater than the pos so we use strictly < or > no >= or <= since that would be overlapping
	if( (AX < BX + BWidth + Epsilon) && (AX + AWidth > BX - Epsilon) && (AY < BY + BHeight + Epsilon) && (AY + AHeight > BY - Epsilon) )
	{
		Collided = true;
	}
	
	return Collided;
}

void OrthographicProjectionMatrix(float* ProjMatrix, float Left, float Bottom, float Right, float Top)
{
	float XScale = 2.0f / (Right - Left);
	float YScale = 2.0f / (Top - Bottom);
	float XTrans = -(Right + Left) / (float)(Right - Left);
	float YTrans = -(Top + Bottom) / (float)(Top - Bottom);
	
	ProjMatrix[0] = XScale; ProjMatrix[4] = 0; 		 ProjMatrix[8] = 0;       ProjMatrix[12] = XTrans;
	ProjMatrix[1] = 0; 		ProjMatrix[5] = YScale;  ProjMatrix[9] = 0;       ProjMatrix[13] = YTrans;
	ProjMatrix[2] = 0; 		ProjMatrix[6] = 0; 		 ProjMatrix[10] = -1.0f;  ProjMatrix[14] = 0;
	ProjMatrix[3] = 0; 		ProjMatrix[7] = 0; 		 ProjMatrix[11] = 0;      ProjMatrix[15] = 1.0f;
						  
}

void MoveAndCollisionCheckGlobal(game_state* GameState, camera* Camera, input* Input, entity* CollideEntity, int CollideEntityIndex)
{
	float DX = 0.0f;
	float DY = 0.0f;
	
	bool Left = Input->WasDown[Button_Left];
	bool Up = Input->WasDown[Button_Up];
	bool Right = Input->WasDown[Button_Right];
	bool Down = Input->WasDown[Button_Down];
	
	float Speed = 4.0f;
	if(Right) DX = 1.0f;
	if(Left) DX = -1.0f;
	if(Up) DY = 1.0f;
	if(Down) DY = -1.0f;
	

	float Magnitude = sqrtf(DX * DX + DY * DY);
	if(Magnitude > 0.0f)
	{
		DX /= Magnitude;
		DY /= Magnitude;
	}
	
	CollideEntity->X += DX * Speed;
	Camera->X += DX * Speed;
	
	const float Epsilon = 10.0f;
	for(int EntityIndex = 0; EntityIndex < GameState->EntityCount; EntityIndex++)
	{
		entity* TestEntity = &GameState->Entities[EntityIndex];
		if(EntityIndex != CollideEntityIndex)
		{
			bool Collided = CollisionCheckPair(Epsilon, CollideEntity->X, CollideEntity->Y, CollideEntity->Width, CollideEntity->Height, TestEntity->X, TestEntity->Y, TestEntity->Width, TestEntity->Height);
			
			if(Collided)
			{
				float OverlapX = 0;
				if(DX > 0)
				{
					OverlapX = ((CollideEntity->X + CollideEntity->Width) - TestEntity->X);//Right Side Hit
					CollideEntity->X -= (OverlapX + Epsilon);
					Camera->X -= (OverlapX + Epsilon);
				}
				else if (DX < 0)
				{
					OverlapX = ((TestEntity->X + TestEntity->Width) - CollideEntity->X);//Left Side Hit
					CollideEntity->X += (OverlapX + Epsilon);
					Camera->X += (OverlapX + Epsilon);
				}
			}
			
		}
	}
	
	CollideEntity->Y += DY * Speed;
	Camera->Y += DY * Speed;
	
	for(int EntityIndex = 0; EntityIndex < GameState->EntityCount; EntityIndex++)
	{
		entity* TestEntity = &GameState->Entities[EntityIndex];
		if(EntityIndex != CollideEntityIndex)
		{

					
			bool Collided = CollisionCheckPair(Epsilon, CollideEntity->X, CollideEntity->Y, CollideEntity->Width, CollideEntity->Height, TestEntity->X, TestEntity->Y, TestEntity->Width, TestEntity->Height);
			
			if(Collided)
			{
				float OverlapY = 0;
				if(DY > 0)
				{
					OverlapY = ((CollideEntity->Y + CollideEntity->Height) - TestEntity->Y); //Right Side Hit
					CollideEntity->Y -= (OverlapY + Epsilon);
					Camera->Y -= (OverlapY + Epsilon);
				}
				else if (DY < 0)
				{
					OverlapY = ((TestEntity->Y + TestEntity->Height) - CollideEntity->Y);//Left Side Hit
					CollideEntity->Y += (OverlapY + Epsilon);
					Camera->Y += (OverlapY + Epsilon);
				}
			}
			
		}
	}
}

//UPDATE IMAGE LOAD FAILED CASE

void UpdateAndRender(memory* Memory, input* Input, GLuint* VAO, GLuint* VBO, GLuint* ShaderProgram)
{
	
	if(!Memory->IsGameStateInit)
	{
		//gladLoaderLoadGL();
		
		Memory->GameState = PushStruct(&Memory->PermanentMemory, game_state);
		game_state* GameState = Memory->GameState;
		
		GameState->PlayerEntityIndex = AddEntity(GameState->Entities, &GameState->EntityCount, 0, 0, 640 - 2*150, 480 - 2*150); //340 Width 180 Height //400 GenWidth 200 GenHeight 300 GenPadX 150 GenPadY
		entity* PlayerEntity = &GameState->Entities[GameState->PlayerEntityIndex];

		GameState->PlayerTextureIndex = AddImage(GameState->Images, &GameState->ImageCount, "../brick_test.png");
		PlayerEntity->ImageIndex = GameState->PlayerTextureIndex;
		
		PopulateWorld(GameState->Entities, &GameState->EntityCount);
	
		//GLuint TexLoc = glGetUniformLocation(ShaderProgram, "BrickTexture");
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // For premultiplied alpha
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		GameState->ProjLoc = glGetUniformLocation(*ShaderProgram, "ProjMatrix");
		GameState->BrickLoc = glGetUniformLocation(*ShaderProgram, "BrickTexture");
		GameState->ColourLoc = glGetUniformLocation(*ShaderProgram, "Colour");
		GameState->LightPosLoc = glGetUniformLocation(*ShaderProgram, "LightPos");
		GameState->LightColourLoc = glGetUniformLocation(*ShaderProgram, "LightColour");
		GameState->LightRadiusLoc = glGetUniformLocation(*ShaderProgram, "LightRadius");
		GameState->AmbientLoc = glGetUniformLocation(*ShaderProgram, "Ambient");
		
		
		Memory->IsGameStateInit = true;
	}
	
	game_state* GameState = Memory->GameState;
	entity* PlayerEntity = &GameState->Entities[GameState->PlayerEntityIndex];
	image* PlayerImage = &GameState->Images[PlayerEntity->ImageIndex];
	
	float CameraWidth = 640 * 1.5f;
	float CameraHeight = 480 * 1.5f;
	
	float HalfW = CameraWidth / 2.0f;
	float HalfH = CameraHeight / 2.0f;
	
	float CenterCameraX = PlayerEntity->X + PlayerEntity->Width / 2.0f;
	float CenterCameraY = PlayerEntity->Y + PlayerEntity->Height / 2.0f;
	
	
	camera Camera = {CenterCameraX - HalfW, CenterCameraY - HalfH, CameraWidth, CameraHeight};
		
	float ProjMatrix[16];
	//OrthographicProjectionMatrix(ProjMatrix, Camera.X, Camera.Y, Camera.X + Camera.Width, Camera.Y + Camera.Height);
	
	MoveAndCollisionCheckGlobal(GameState, &Camera, Input, PlayerEntity, GameState->PlayerEntityIndex);
	
	OrthographicProjectionMatrix(ProjMatrix, Camera.X, Camera.Y, Camera.X + Camera.Width, Camera.Y + Camera.Height);
	
	float Alpha = 1.0f;
	glClearColor(0.2f * Alpha, 0.3f * Alpha, 0.3f * Alpha, Alpha);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glUseProgram(*ShaderProgram);
	
	float FloatTestAlpha = 0.5f;
	glUniform4f(GameState->ColourLoc, 1.0f * FloatTestAlpha, 0, 0, FloatTestAlpha);
	glUniformMatrix4fv(GameState->ProjLoc, 1, GL_FALSE, ProjMatrix);
	glUniform1i(GameState->BrickLoc, 0);
	
	float LightX = 400.0f;
	float LightY = 300.0f;
	
	glUniform2f(GameState->LightPosLoc, LightX, LightY);
	glUniform3f(GameState->LightColourLoc, 1.0f, 1.0f, 1.0f);
	glUniform1f(GameState->LightRadiusLoc, 250.0f);
	glUniform1f(GameState->AmbientLoc, 0.2f);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, PlayerImage->TextureID);
	
	glBindVertexArray(*VAO);
	//glDrawArrays(GL_TRIANGLES, 0, 6);
	
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	for(int EntityIndex = 0; EntityIndex < GameState->EntityCount; EntityIndex++)
	{
		entity CurrEntity = GameState->Entities[EntityIndex];
		float Vertices[] = 
		{
		   CurrEntity.X + CurrEntity.Width,  CurrEntity.Y + CurrEntity.Height, 1.0f, 1.0f,//Top right
		   CurrEntity.X,                 	 CurrEntity.Y + CurrEntity.Height, 0.0f, 1.0f,//Top left
		   CurrEntity.X,                     CurrEntity.Y,                     0.0f, 0.0f,//Bottom left
		   CurrEntity.X + CurrEntity.Width,  CurrEntity.Y,                     1.0f, 0.0f,//Bottom right

		};
		int VerticesSize = sizeof(Vertices);
		
		glBufferSubData(GL_ARRAY_BUFFER, 0, VerticesSize, Vertices);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		glBufferSubData(GL_ARRAY_BUFFER, 0, VerticesSize, Vertices);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
}