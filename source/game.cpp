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
image LoadImage(const char* Filename, image_option Option)
{
	image Image = {};
	int Channels;
	int ChanneslsForce = 4;
	stbi_set_flip_vertically_on_load(1);
	//Texture pixels are not stored CPU side
	unsigned char* Data = stbi_load(Filename, &Image.Width, &Image.Height, &Channels, ChanneslsForce);
	if(Data)
	{
		//NOTE:The image struct is not fully filled yet here
		if(Option == Image_Option_Premultiply)
		{
#if 0
			PremultiplyAlpha(&Image, Data); 
#else
			PremultiplyAlpha_4x(&Image, Data);
#endif
		}
		
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

int AddEntity(entity* Entities, int* EntityCount, float X, float Y, float Width, float Height, entity_type Type)
{
	int EntityIndex = *EntityCount;
	
	entity Entity = {};
	Entity.X = X;
	Entity.Y = Y;
	Entity.Width = Width;
	Entity.Height = Height;
	Entity.Collision = StandardCollision(Entity);
	Entity.Type = Type;
	Entity.Index = EntityIndex;
	
	Entities[(*EntityCount)++] = Entity;
	
	return EntityIndex;
}

int AddNpc(game_state* GameState, entity* Entities, int* EntityCount, float X, float Y, float Width, float Height, entity_type Type)
{
	int NpcIndex = AddEntity(Entities, EntityCount, X, Y, Width, Height, Type);
	entity* Npc = &GameState->Entities[NpcIndex];
	Npc->PathingTimerMax = 3.0f;
	Npc->PathingTimer = Npc->PathingTimerMax;
	Npc->Velocity = {1.0f, 1.0f};
	GameState->NpcCount++;
	return NpcIndex;
}

int AddImage(image* Images, int* ImageCount, char* Filename, image_option Options)
{
	int ImageIndex = *ImageCount;
	
	image Image = LoadImage(Filename, Options);
	
	Images[(*ImageCount)++] = Image;
	
	return ImageIndex;
}

void PopulateWorldCornerTest(entity* Entities, int* EntityCount)
{
	float Width = 400;
	float Height = 200;
	//float PadX = 300;
	//float PadY = 150;
	
	float XStart =  Width + 100;
	float X = XStart;
	float Y = 0;
	
	for(int EntityY = 0; EntityY < 10; EntityY++)
	{
		X = XStart;
		for(int EntityX = 0; EntityX < 10; EntityX++)
		{
			if (EntityY == 0 || EntityX == 0) 
			AddEntity(Entities, EntityCount, X, Y, Width, Height, Entity_Type_Other);
			
			X+= Width;// + PadX;
		}
		
		Y+= Height;// + PadY;
	}
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
			AddEntity(Entities, EntityCount, X, Y, Width, Height, Entity_Type_Other);
			
			X+= Width + PadX;
		}
		
		Y+= Height + PadY;
	}
}


void InitGameState(memory* Memory, GLuint* ShaderProgram)
{
	Memory->GameState = PushStruct(&Memory->PermanentMemory, game_state);
	game_state* GameState = Memory->GameState;
	
	float PlayerWidth = 640 - 2*150;
	float PlayerHeight = 480 - 2*150;
	GameState->PlayerEntityIndex = AddEntity(GameState->Entities, &GameState->EntityCount, 0, 0, PlayerWidth, PlayerHeight, Entity_Type_Player); //340 Width 180 Height //400 GenWidth 200 GenHeight 300 GenPadX 150 GenPadY
	entity* PlayerEntity = &GameState->Entities[GameState->PlayerEntityIndex];

	GameState->PlayerTextureIndex = AddImage(GameState->Images, &GameState->ImageCount, "../brick_test.png", Image_Option_Premultiply);
	PlayerEntity->ImageIndex = GameState->PlayerTextureIndex;
	
	GameState->NpcEntityIndex[0]= AddNpc(GameState, GameState->Entities, &GameState->EntityCount, -PlayerWidth - 20.0f, 0, PlayerWidth, PlayerHeight, Entity_Type_Player);
	//GameState->NpcEntityIndex[1]= AddNpc(GameState,GameState->Entities, &GameState->EntityCount, 0, -PlayerHeight - 20.0f, PlayerWidth, PlayerHeight, Entity_Type_Player);
	//GameState->NpcEntityIndex[2]= AddNpc(GameState, GameState->Entities, &GameState->EntityCount, -PlayerWidth - 20.0f, -PlayerHeight - 20.0f, PlayerWidth, PlayerHeight, Entity_Type_Player);
	
	GameState->ParticleTextureIndex = AddImage(GameState->Images, &GameState->ImageCount, "../particle_high_res.png", Image_Option_None);
	
	PopulateWorld(GameState->Entities, &GameState->EntityCount);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // For premultiplied alpha

	GameState->ProjLoc = glGetUniformLocation(*ShaderProgram, "ProjMatrix");
	GameState->BrickLoc = glGetUniformLocation(*ShaderProgram, "BrickTexture");
	GameState->ColourLoc = glGetUniformLocation(*ShaderProgram, "Colour");
	GameState->LightPosLoc = glGetUniformLocation(*ShaderProgram, "LightPos");
	GameState->LightColourLoc = glGetUniformLocation(*ShaderProgram, "LightColour");
	GameState->LightRadiusLoc = glGetUniformLocation(*ShaderProgram, "LightRadius");
	GameState->AmbientLoc = glGetUniformLocation(*ShaderProgram, "Ambient");
	
	//Background Texture
	glGenTextures(1, &GameState->BackgroundTexture);
	glBindTexture(GL_TEXTURE_2D, GameState->BackgroundTexture);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	uint8_t Alpha = 255;
	float R = 50.0f / 255.0f;
	float G = 50.0f / 255.0f;
	float B = 50.0f / 255.0f;
	float A = Alpha / 255.0f;
	
	R*= A;
	G*= A;
	B*= A;
	
	//Background Texture
	uint8_t BGPixel[4] = {(uint8_t)(R * 255.0f + 0.5f), (uint8_t)(G * 255.0f + 0.5f), (uint8_t)(B * 255.0f + 0.5f), Alpha};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, BGPixel);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	//In case we sample outside of bounds by accident 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	Memory->IsGameStateInit = true;
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

float RotateTowards(float Current, float Target, float MaxAngleDiff)
{
	float AngleDiff = Target - Current;
	
	//Make sure we use the shortest way there, AngleDiff can exceed Pi radians, we need to correct that
	if(AngleDiff >  Pi32) 
		AngleDiff -= 2.0f * Pi32;
	if(AngleDiff <= -Pi32) // Open interval (-Pi32, Pi32]
		AngleDiff += 2.0f * Pi32;
	
	//Make sure we don't exceed the amount we set per frame to rotate
	if(AngleDiff >  MaxAngleDiff) 
		AngleDiff =  MaxAngleDiff;
	if(AngleDiff < -MaxAngleDiff) 
		AngleDiff = -MaxAngleDiff;
	
	//Current + AngleDiff can also be > 180 degrees or <= -180 causing issues, so we wrap
	//Example AngleDiff -170 - 160 = -330 -> Wraps to + 30
	//Say Current = 160 NewAngle = 160 + 30 = 190 -> Problem!
	float NewAngle = Current + AngleDiff;
	if(NewAngle >  Pi32) 
		NewAngle -= 2.0f * Pi32;
	if(NewAngle < -Pi32) 
		NewAngle += 2.0f * Pi32;
	
	return NewAngle;
	
}

void ResolveCollision(game_state* GameState, entity* CollideEntity, v2 InitialP, float Epsilon, float GlideSpeed)
{
	(void)GlideSpeed;
	(void)InitialP;
	
	float Adjustment[2] = {-1, -1};
	int OverlapMinIndex[2] = {-1, -1};
	bool CantBeX = false;
	bool CantBeY = false;
	for(int Pass = 0; Pass < 3; Pass++)
	{
		bool CollisonsFound = false;
		for(int EntityIndex = 0; EntityIndex < GameState->EntityCount; EntityIndex++)
		{
			entity* TestEntity = &GameState->Entities[EntityIndex];
			if(EntityIndex != CollideEntity->Index)
			{
				bool Collided = CollisionCheckPair(Epsilon, CollideEntity->X, CollideEntity->Y, CollideEntity->Width, CollideEntity->Height, TestEntity->X, TestEntity->Y, TestEntity->Width, TestEntity->Height);
				
				if(Collided)
				{
					if(Pass == 2)
					{
						#if 0
						for(int OverlapIndex = 0; OverlapIndex < 2; OverlapIndex++)
						{
							switch(OverlapMinIndex[OverlapIndex])
							{
								case 0: CollideEntity->X += Adjustment[OverlapIndex]; break;
								case 1: CollideEntity->X -= Adjustment[OverlapIndex]; break;
								case 2: CollideEntity->Y -= Adjustment[OverlapIndex]; break;
								case 3: CollideEntity->Y += Adjustment[OverlapIndex]; break;
							}
						}
						//Assert((InitialP.X == CollideEntity->X) && (InitialP.Y == CollideEntity->Y));
						#else
						CollideEntity->X = InitialP.X;
						CollideEntity->Y = InitialP.Y;
						#endif
						break;
					}
					
					if(Pass > 0 && OverlapMinIndex[Pass - 1] >= 0)
					{
						switch(OverlapMinIndex[Pass - 1])
						{
							#if 0
							case 0: CollideEntity->X -= Adjustment[Pass - 1];  break;
							case 1: CollideEntity->X += Adjustment[Pass - 1];  break;
							case 2: CollideEntity->Y += Adjustment[Pass - 1];  break;
							case 3: CollideEntity->Y -= Adjustment[Pass - 1];  break;
							#else
							case 0: CantBeX = true; CollideEntity->X -= Adjustment[Pass - 1]; break;
							case 1: CantBeX = true; CollideEntity->X += Adjustment[Pass - 1]; break;
							case 2: CantBeY = true; CollideEntity->Y += Adjustment[Pass - 1]; break;
							case 3: CantBeY = true; CollideEntity->Y -= Adjustment[Pass - 1]; break;
							#endif
						}
					}
					float OverlapRight = ((TestEntity->X + TestEntity->Width) - (CollideEntity->X - Epsilon));//Right Side Hit
					float OverlapLeft = ((CollideEntity->X + CollideEntity->Width + Epsilon) - TestEntity->X);//Left Side Hit
					float OverlapBottom = ((CollideEntity->Y + CollideEntity->Height + Epsilon) - TestEntity->Y); //Bottom Side Hit
					float OverlapTop = ((TestEntity->Y + TestEntity->Height) - (CollideEntity->Y  - Epsilon));//TopSide Hit
					float OverlapAmounts[MAX_OVERLAPS] = {OverlapRight, OverlapLeft, OverlapBottom, OverlapTop};
					float OverlapMin = OverlapAmounts[0];
					
					#if 0
					if(CantBeX)
					{
						OverlapMin = OverlapBottom;
						OverlapMinIndex[Pass] = 2;
						if(OverlapTop < OverlapBottom)
						{
							OverlapMin = OverlapTop;
							OverlapMinIndex[Pass] = 3;
						}
					}
					else if(CantBeY)
					{
						OverlapMin = OverlapRight;
						OverlapMinIndex[Pass] = 0;
						if(OverlapLeft < OverlapRight) 
						{
							OverlapMin = OverlapLeft;
							OverlapMinIndex[Pass] = 1;
						}
					}
					else
					#endif
					{
						OverlapMinIndex[Pass] = 0;
						for(int OverlapIndex = 1; OverlapIndex < MAX_OVERLAPS; OverlapIndex++)
						{
							if(OverlapAmounts[OverlapIndex] < OverlapMin)
							{
								OverlapMin = OverlapAmounts[OverlapIndex];
								OverlapMinIndex[Pass] = OverlapIndex;
							}
						}
					}
					
					Adjustment[Pass] = (OverlapMin);
					//v2* Velocity = &CollideEntity->Velocity;
					switch(OverlapMinIndex[Pass])
					{
						case 0: 
						{
							CollideEntity->X += Adjustment[Pass]; 
							//Velocity->X = 0;
							//if(Velocity->Y > GlideSpeed)  Velocity->Y = GlideSpeed;
							//if(Velocity->Y < -GlideSpeed) Velocity->Y = -GlideSpeed;
						}						
						break;
						case 1: 
						{
							CollideEntity->X -= Adjustment[Pass]; 
							//Velocity->X = 0;
							//if(Velocity->Y > GlideSpeed)  Velocity->Y = GlideSpeed;
							//if(Velocity->Y < -GlideSpeed) Velocity->Y = -GlideSpeed;
						}
						break;
						case 2: 
						{
							CollideEntity->Y -= Adjustment[Pass]; 
							//Velocity->Y = 0;
							//if(Velocity->X > GlideSpeed)  Velocity->X = GlideSpeed;
							//if(Velocity->X < -GlideSpeed) Velocity->X = -GlideSpeed;
						}						
						break;
						case 3:
						{
							CollideEntity->Y += Adjustment[Pass]; 
							//Velocity->Y = 0;
							//if(Velocity->X > GlideSpeed)  Velocity->X = GlideSpeed;
							//if(Velocity->X < -GlideSpeed) Velocity->X = -GlideSpeed;
						}
						break;
						
					}
					
					CollisonsFound = true;
					break;
				}
				
			}
		}
		
		if(!CollisonsFound) break;
	}
	
}
void MoveAndResolveCollision(game_state* GameState, entity* CollideEntity, float Epsilon, float GlideSpeed, float DT)
{
	v2* Velocity = &CollideEntity->Velocity;
	
	v2 InitialP = {CollideEntity->X, CollideEntity->Y};
	CollideEntity->X += Velocity->X * DT;
	//if(IsPlayer) Camera->X += Velocity->X * DT;
	CollideEntity->Y += Velocity->Y * DT;
	//if(IsPlayer) Camera->Y += Velocity->Y * DT;
	
	ResolveCollision(GameState, CollideEntity, InitialP, Epsilon, GlideSpeed);
}

void MoveAndCollisionCheckGlobal(game_state* GameState, input* Input, entity* CollideEntity, float DT)
{
	//Unify input check and appropriate vector in one go
	v2 AccelInput = {(float)Input->IsDown[Button_Right] - (float)Input->IsDown[Button_Left],
					 (float)Input->IsDown[Button_Up]    - (float)Input->IsDown[Button_Down]};
	
	//If we go diagonal we need to normalize so it's not faster
	if((AccelInput.X != 0) && (AccelInput.Y != 0))
	{
		float Magnitude = sqrtf(AccelInput.X * AccelInput.X + AccelInput.Y * AccelInput.Y);
		AccelInput.X /= Magnitude;
		AccelInput.Y /= Magnitude;
		
	}
	
	//If this frames input is not the same as last frames
	if(CollideEntity->LastAccelInput.X != AccelInput.X || CollideEntity->LastAccelInput.Y != AccelInput.Y)
	{
		//Start timer
		CollideEntity->HoldTime = 0;
		CollideEntity->LastAccelInput.X = AccelInput.X
;		CollideEntity->LastAccelInput.Y = AccelInput.Y;
	}
	else
	{
		CollideEntity->HoldTime += DT;
	}
;	float AcceptThreshold = 0.1f;

	//If we aren't moving we need to 0 out LastAccel since our velcoity is based off that
	if((AccelInput.X == 0) && (AccelInput.Y == 0)) 
	{
		v2 Accel = {};
		CollideEntity->LastAccel = Accel;
	}
	//If we are at or above the threshold and are still trying to move we can finally let the angle change
	//and apply the new acceleration
	else if((CollideEntity->HoldTime >= AcceptThreshold))
	{
		CollideEntity->LastAccel = AccelInput;
		float TargetAngle = atan2f(AccelInput.Y, AccelInput.X);
		float TurnSpeed = 4.0f;
		CollideEntity->Angle = RotateTowards(CollideEntity->Angle, TargetAngle, TurnSpeed * DT);
	}

	v2* Velocity = &CollideEntity->Velocity;
		
	float AccelRate = 128.0f;	
	Velocity->X += CollideEntity->LastAccel.X * AccelRate * DT;
	Velocity->Y += CollideEntity->LastAccel.Y * AccelRate * DT;
	
	float Drag = 0.99f;
	Velocity->X *= Drag;
	Velocity->Y *= Drag;
	
	if(Velocity->X > 200.0f)  Velocity->X = 200.0f;
	if(Velocity->X < -200.0f) Velocity->X = -200.0f;
	if(Velocity->Y > 200.0f)  Velocity->Y = 200.0f;
	if(Velocity->Y < -200.0f) Velocity->Y = -200.0f;
	
#if 0
	if(Velocity->X > 60.0f)  Velocity->X = 60.0f;
	if(Velocity->X < -60.0f) Velocity->X = -60.0f;
	if(Velocity->Y > 60.0f)  Velocity->Y = 60.0f;
	if(Velocity->Y < -60.0f) Velocity->Y = -60.0f;
#endif

	float Speed = 128.0f;
	Velocity->X = AccelInput.X * Speed;
	Velocity->Y = AccelInput.Y * Speed;
	
	float Epsilon = 10.0f;
	float GlideSpeed = 100.0f;
	MoveAndResolveCollision(GameState, CollideEntity, Epsilon, GlideSpeed, DT);
}

void NpcMoveAndCollisionCheck(game_state* GameState, float DT)
{
	float Speed = 60.0f;
	for(int NpcIndexIndex = 0; NpcIndexIndex < GameState->NpcCount; NpcIndexIndex++)
	{
		int NpcIndex = GameState->NpcEntityIndex[NpcIndexIndex];
		entity* Npc = &GameState->Entities[NpcIndex];
		if(Npc->PathingTimer <= 0.0f)
		{
			Npc->Velocity = { (float)((rand() % 3) - 1), (float)((rand() % 3) - 1) };
			//Npc->Velocity = {1, 0};
			Npc->PathingTimer = Npc->PathingTimerMax;
		}
		else
		{
			Npc->X += Npc->Velocity.X * Speed * DT;
			Npc->Y += Npc->Velocity.Y * Speed * DT;
			Npc->PathingTimer -= DT;
		}
		
		float Epsilon = 10.0f;
		float GlideSpeed = 100.0f;
		MoveAndResolveCollision(GameState, Npc, Epsilon, GlideSpeed, DT);
	}
}

void EmitParticle(particle_system* ParticleSystem, float X, float Y)
{
	for(int ParticleIndex = 0; ParticleIndex < MAX_PARTICLES; ParticleIndex++)
	{
		particle* Particle = &ParticleSystem->Particles[ParticleIndex];
		if(!Particle->Active)
		{
			Particle->X = X;
			Particle->Y = Y;
			Particle->VX = ((rand() % 200) - 100) / 100.0f; //-1 -> 1
			Particle->VY = ((rand() % 200) - 100) / 100.0f; //-1 -> 1
			Particle->Size = 16.0f;
			Particle->MaxLife = Particle->Life = 2.0f;
			Particle->R = 1.0f;
			Particle->G = 0.5f;
			Particle->B = 0.0f;
			Particle->A = 1.0f;
			Particle->Active = true;
			break;
		}
	}
}

void UpdateParticles(particle_system* ParticleSystem, float DT)
{
	for(int ParticleIndex = 0; ParticleIndex < MAX_PARTICLES; ParticleIndex++)
	{
		particle* Particle = &ParticleSystem->Particles[ParticleIndex];
		if(Particle->Active)
		{
			float Speed = 60.0f;

			Particle->X += Particle->VX * DT * Speed; 
			Particle->Y += Particle->VY * DT * Speed;
			Particle->Life -= DT;
			Particle->A = Particle->Life / Particle->MaxLife;
			Particle->Size *= 0.98f;
			if(Particle->Life <= 0.0f)
			{
				Particle->Active = false;
			}
		}
	}
}

v2 SquareEmission(float Diameter)
{
	v2 Offset = {};
	Offset.X = (rand() / (float)RAND_MAX - 0.5f) * Diameter / 2.0f;
	Offset.Y = (rand() / (float)RAND_MAX - 0.5f) * Diameter / 2.0f;
	
	return Offset;
}

v2 RadialEmission(float MaxRadius)
{
	v2 Offset = {};
	
	float Angle = (float)rand() / RAND_MAX * 2.0f * Pi32;
	float Radius = sqrtf((float)rand() / RAND_MAX) * MaxRadius; //Need to make # of particles proportional to area
	Offset.X = cosf(Angle)*Radius;
	Offset.Y = sinf(Angle)*Radius;
	
	return Offset;
}

triangle SetAndUploadRotatedTraingleVertices(entity* CurrEntity)
{
	//Rotate Around Center of Triangle instead of origin
	float PivotX = CurrEntity->X + CurrEntity->Width * 0.5f; 
	float PivotY = CurrEntity->Y + CurrEntity->Height* 0.5f;

	//Local coordinates, center is the middle of the triangle
	v2 Bottom = {-0.5f*CurrEntity->Width, -0.5f*CurrEntity->Height};
	v2 Right = {0.5f*CurrEntity->Width, 0.0f};
	v2 Top = {-0.5f*CurrEntity->Width, 0.5f*CurrEntity->Height};

	float CosAngle = cosf(CurrEntity->Angle);
	float SinAngle = sinf(CurrEntity->Angle);

	v2 BottomRotated = {PivotX + (Bottom.X * CosAngle - Bottom.Y * SinAngle), PivotY + (Bottom.X * SinAngle + Bottom.Y * CosAngle)};
	v2 RightRotated = {PivotX + (Right.X * CosAngle - Right.Y * SinAngle), PivotY + (Right.X * SinAngle + Right.Y * CosAngle)};
	v2 TopRotated = {PivotX + (Top.X * CosAngle - Top.Y * SinAngle), PivotY + (Top.X * SinAngle + Top.Y * CosAngle)};

	float Vertices[] = 
	{
		BottomRotated.X,   BottomRotated.Y,  0.0f, 0.0f,
		RightRotated.X,    RightRotated.Y,   1.0f, 0.0f,
		TopRotated.X,      TopRotated.Y,     0.0f, 1.0f,
	};
	int VerticesSize = sizeof(Vertices);

	glBufferSubData(GL_ARRAY_BUFFER, 0, VerticesSize, Vertices);
	
	triangle Triangle = {RightRotated, BottomRotated, TopRotated};
	return Triangle;
}

void SetAndUploadQuadVertices(entity* CurrEntity)
{
	float Vertices[] = 
	{
	   CurrEntity->X + CurrEntity->Width,  CurrEntity->Y + CurrEntity->Height, 1.0f, 1.0f,//Top right
	   CurrEntity->X,                 	   CurrEntity->Y + CurrEntity->Height, 0.0f, 1.0f,//Top left
	   CurrEntity->X,                      CurrEntity->Y,                      0.0f, 0.0f,//Bottom left
	   CurrEntity->X + CurrEntity->Width,  CurrEntity->Y,                      1.0f, 0.0f,//Bottom right

	};
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
	
}

//UPDATE IMAGE LOAD FAILED CASE
///NEED TO MAKE TIMING ROBUST!!!
void UpdateAndRender(memory* Memory, input* Input, GLuint* VAO, GLuint* VBO, GLuint* ShaderProgram)
{
	
	if(!Memory->IsGameStateInit)
	{
		InitGameState(Memory, ShaderProgram);
	}
	
	float DT = 1.0f / 60.0f;
	
	game_state* GameState = Memory->GameState;
	entity* PlayerEntity = &GameState->Entities[GameState->PlayerEntityIndex];
	image* PlayerImage = &GameState->Images[PlayerEntity->ImageIndex];
	image* ParticleImage = &GameState->Images[GameState->ParticleTextureIndex];
	
	
	MoveAndCollisionCheckGlobal(GameState, Input, PlayerEntity, DT);
	NpcMoveAndCollisionCheck(GameState, DT);
	
	float CameraWidth = 640 * 1.5f;
	float CameraHeight = 480 * 1.5f;
	
	float HalfW = CameraWidth / 2.0f;
	float HalfH = CameraHeight / 2.0f;
	
	float CenterCameraX = PlayerEntity->X + PlayerEntity->Width / 2.0f;
	float CenterCameraY = PlayerEntity->Y + PlayerEntity->Height / 2.0f;
	
	camera Camera = {CenterCameraX - HalfW, CenterCameraY - HalfH, CameraWidth, CameraHeight};
	
	float ProjMatrix[16];
	OrthographicProjectionMatrix(ProjMatrix, Camera.X, Camera.Y, Camera.X + Camera.Width, Camera.Y + Camera.Height);
	
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); 
	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	glUseProgram(*ShaderProgram);
	glUniformMatrix4fv(GameState->ProjLoc, 1, GL_FALSE, ProjMatrix);
	
	float LightX = 400.0f;
	float LightY = 300.0f;
	glUniform2f(GameState->LightPosLoc, LightX, LightY);
	glUniform3f(GameState->LightColourLoc, 3.0f, 3.0f, 3.0f);
	glUniform1f(GameState->LightRadiusLoc, 125.0f);
	glUniform1f(GameState->AmbientLoc, 0.2f);
	
	//Background quad render
	//glUniform4f(GameState->ColourLoc, 0.2f*Alpha, 0.3f*Alpha, 0.3f*Alpha, Alpha);
	glUniform4f(GameState->ColourLoc, 1.0f, 1.0f, 1.0f, 1.0f);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GameState->BackgroundTexture);
	
	float BackgroundVertices[] =
	{
		Camera.X,                Camera.Y,                 0.0f, 0.0f,
		Camera.X + Camera.Width, Camera.Y,                 1.0f, 0.0f,
		Camera.X + Camera.Width, Camera.Y + Camera.Height, 1.0f, 1.0f,
		Camera.X,                Camera.Y + Camera.Height, 0.0f, 1.0f,
	};
	
	glBindVertexArray(*VAO);
	glBindBuffer(GL_ARRAY_BUFFER, *VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(BackgroundVertices), BackgroundVertices);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	
	//Entity Render
	float TextureModAlpha = 1.0f;
	glUniform4f(GameState->ColourLoc, 1.0f * TextureModAlpha, 0, 0, TextureModAlpha);
	glUniform1i(GameState->BrickLoc, 0);
	glBindTexture(GL_TEXTURE_2D, PlayerImage->TextureID);
	for(int EntityIndex = 0; EntityIndex < GameState->EntityCount; EntityIndex++)
	{
		entity* CurrEntity = &GameState->Entities[EntityIndex];
		
		if(CurrEntity->Type == Entity_Type_Player)
		{
			SetAndUploadQuadVertices(CurrEntity);
			//triangle Triangle = SetAndUploadRotatedTraingleVertices(CurrEntity);
			//glDrawArrays(GL_TRIANGLES, 0, 3);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			
			v2 Offset = RadialEmission(2.5f);
		   // EmitParticle(&GameState->ParticleSystem, Triangle.CCW.X + Offset.X , Triangle.CCW.Y + Offset.Y);
			//EmitParticle(&GameState->ParticleSystem, Triangle.CW.X + Offset.X , Triangle.CW.Y + Offset.Y);

		}
		else if (CurrEntity->Type == Entity_Type_Other)
		{
			SetAndUploadQuadVertices(CurrEntity);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
	}
	
	//Particle Render
	glBlendFunc(GL_ONE, GL_ONE);
	glBindTexture(GL_TEXTURE_2D, ParticleImage->TextureID);
	for(int ParticleIndex = 0; ParticleIndex < MAX_PARTICLES; ParticleIndex++)
	{
		particle* Particle = &GameState->ParticleSystem.Particles[ParticleIndex];
		if(Particle->Active)
		{
			float Size = Particle->Size;
			float HalfSize = 0.5f * Size;
			float Vertices[] = 
			{
				Particle->X + HalfSize, Particle->Y + HalfSize, 1.0f, 1.0f,//Top right
				Particle->X - HalfSize, Particle->Y + HalfSize, 0.0f, 1.0f,//Top left
				Particle->X - HalfSize, Particle->Y - HalfSize, 0.0f, 0.0f,//Bottom left
				Particle->X + HalfSize, Particle->Y - HalfSize, 1.0f, 0.0f,//Bottom right
			};
			int VerticesSize = sizeof(Vertices);
			
			glBufferSubData(GL_ARRAY_BUFFER, 0, VerticesSize, Vertices);
			glUniform4f(GameState->ColourLoc, Particle->R, Particle->G , Particle->B, Particle->A);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			
		}
		
	}	
	
	UpdateParticles(&GameState->ParticleSystem, DT);
}