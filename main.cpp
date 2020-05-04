// ----------------------------------------------------------------
// LICENSE
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non - commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain.We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors.We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
// ----------------------------------------------------------------

#include <stdio.h>
#include "SDL\include\SDL.h"
#include "SDL_image\include\SDL_image.h"
#include "SDL_mixer\include\SDL_mixer.h"

/* ----------------------------------------------------------------
SDL_Init(SDL_INIT_EVERYTHING); // init all SDl subsystems
SDL_CreateWindow(); // creates a window
SDL_CreateRenderer(); // Creates a renderer inside a window, use SDL_RENDERER_PRESENTVSYNC
SDL_DestroyRenderer();
SDL_DestroyWindow();
SDL_Quit();

IMG_Init(IMG_INIT_PNG); // we only need png
IMG_Quit();
SDL_CreateTextureFromSurface(renderer, IMG_Load("ship.png")); // returns a pointer to the texture we can later draw
SDL_DestroyTexture(tex);
SDL_QueryTexture(tex, nullptr, nullptr, &w, &h); // query width and height of a loaded texture
SDL_RenderCopy(renderer, tex, &section, &destination); // section of the texture and destination on the screen
SDL_RenderPresent(renderer); // swap buffers, stall if using vsync

Mix_Init(MIX_INIT_OGG); // we only need ogg
Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048); // standard values
music = Mix_LoadMUS("music.ogg"); // returns pointer to music
Mix_PlayMusic(music, -1); // play music
fx = Mix_LoadWAV("laser.wav"); // load short audio fx
Mix_PlayChannel(-1, fx, 0); // play audio fx
SDL_GetTicks(); // for timers, returns ms since app start

Mix_FreeMusic(music);
Mix_FreeChunk(fx_shoot);
Mix_CloseAudio();
Mix_Quit();

SDL_PollEvent(&event); // query all input events, google possible contents of "event"
SDL_HasIntersection(&react_a, &react_b); // checks for quad overlap
*/

// ----------------------------------------------------------------
// UTILS

#define MY_RAND_MAX = 2147483647
static unsigned long rand_state = 1;

void srand(unsigned long seed)
{
	rand_state = seed;
}

long rand()
{
	rand_state = (rand_state * 1103515245 + 12345) % 2147483648;
	return rand_state;
}

#define MAX(x, y) x > y ? x : y
#define MIN(x, y) x > y ? y : x

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 479
#define WINDOW_BAR_SIZE 30

// GLOBALS
enum GameState
{
	REGULAR_LOOP,
	PLAYER_DYING,
	END
};

GameState game_state = REGULAR_LOOP;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

Mix_Music* music = NULL;
Mix_Chunk* laser_fx = NULL;

float start_time = 0;
float frame_start_time = 0;
float delta_time = 0;
#define APP_CURRENT_TIME ((SDL_GetTicks() - start_time)/1000.f)

// ENTITIES
// Backgrounds
#define NUM_BACKGROUNDS 4
SDL_Texture* background_textures[4];
SDL_Rect background_rects[4];

// Player
SDL_Texture* player_ship_texture = NULL;
SDL_Rect player_rect;

int player_speed[2]{ 0, 0 };
int player_speed_value = 10;

// Shots
#define MAX_SHOOTS 10
SDL_Texture* shot_texture = NULL;
SDL_Rect shot_rects[MAX_SHOOTS];
int shoots_alive[MAX_SHOOTS];

int shoot_speed = 10;

// Enemies
#define MAX_ENEMIES 5
SDL_Texture* enemy_ship_texture = NULL;
SDL_Rect enemy_rects[MAX_ENEMIES];
int enemies_alive[MAX_ENEMIES];

#define ENEMY_SPEED 5

#define ENEMY_WAVE_SPAWN_TIME 5.f
float time_since_last_wave = ENEMY_WAVE_SPAWN_TIME;
#define ENEMY_SPAWN_TIME 0.25
float time_since_last_enemy = ENEMY_SPAWN_TIME;
int current_wave_num_enemies_spawned = -1;
int spawn_position = 0;

// FUNCTION DECLARATIONS
void InitDimensions(SDL_Texture* texture, SDL_Rect* rect)
{
	int width, height;
	SDL_QueryTexture(texture, NULL, NULL, &width, &height);
	rect->w = width;
	rect->h = height;
}

void Init();
void Cleanup();

void CaptureInput();
void Update();
void Render();

void SpawnShoot();
void DestroyShoot(unsigned int index);

void DestroyEnemy(unsigned int index);

int IntersectedEnemy(const SDL_Rect* shoot_rect);

void DestroyPlayer();

int main(int argc, char* args[])
{
	Init();
	
	while (game_state != END)
	{
		float current_time = APP_CURRENT_TIME;
		delta_time = current_time - frame_start_time;
		frame_start_time = current_time;

		CaptureInput();
		Update();
		Render();
	}

	Cleanup();

	return(0); // EXIT_SUCCESS
}

void Init()
{
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, NULL);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	start_time = SDL_GetTicks() / 1000.f;

	// AUDIO
	Mix_Init(MIX_INIT_OGG);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	music = Mix_LoadMUS("assets/music.ogg"); // returns pointer to music
	laser_fx = Mix_LoadWAV("assets/laser.wav"); // load short audio fx
	Mix_PlayMusic(music, -1); // play music

	// RANDOM GENERATOR
	srand(SDL_GetTicks());

	// IMG
	IMG_Init(IMG_INIT_PNG);

	// ENTITIES
	int width, height;
	
	// Backgrounds
	for (unsigned int i = 0; i < NUM_BACKGROUNDS; ++i)
	{
		char aux[30];
		sprintf_s(aux, "assets/bg%u.png", i);
		background_textures[i] = SDL_CreateTextureFromSurface(renderer, IMG_Load(aux));
		InitDimensions(background_textures[i], &background_rects[i]);
		background_rects[i].y = WINDOW_HEIGHT - background_rects[i].h;

	}

	player_ship_texture = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/ship.png"));
	InitDimensions(player_ship_texture, &player_rect);
	player_rect.x = 0;
	player_rect.y = 0;
	player_rect.w *= 2;
	player_rect.h *= 2;
	
	// Shots
	shot_texture = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/shot.png"));
	for (unsigned int i = 0; i < MAX_SHOOTS; ++i)
	{
		InitDimensions(shot_texture, &shot_rects[i]);
		shot_rects[i].w *= 2;
		shot_rects[i].h *= 2;
		DestroyShoot(i);
	}

	// Enemies
	enemy_ship_texture = SDL_CreateTextureFromSurface(renderer, IMG_Load("assets/enemy.png"));
	for (unsigned int i = 0; i < MAX_ENEMIES; ++i)
	{
		InitDimensions(enemy_ship_texture, &enemy_rects[i]);
		enemy_rects[i].w *= 2;
		enemy_rects[i].h *= 2;
		DestroyEnemy(i);
	}

}

void Cleanup()
{
	for (unsigned int i = 0; i < NUM_BACKGROUNDS; ++i)
	{
		SDL_DestroyTexture(background_textures[i]);
	}

	SDL_DestroyTexture(player_ship_texture);
	SDL_DestroyTexture(enemy_ship_texture);
	SDL_DestroyTexture(shot_texture);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	IMG_Quit();
	SDL_Quit();
}

void CaptureInput()
{
	SDL_Event sdl_event;
	while (SDL_PollEvent(&sdl_event))
	{
		switch (sdl_event.type)
		{
		case SDL_KEYDOWN:
			switch (sdl_event.key.keysym.sym)
			{
			case SDLK_w:
				player_speed[1] = -player_speed_value;
				break;
			case SDLK_a:
				player_speed[0] = -player_speed_value;
				break;
			case SDLK_s:
				player_speed[1] = player_speed_value;
				break;
			case SDLK_d:
				player_speed[0] = player_speed_value;
				break;
			case SDLK_SPACE:
				SpawnShoot();
				break;
			}

			break;

		case SDL_KEYUP:
			switch (sdl_event.key.keysym.sym)
			{
			case SDLK_s:
			case SDLK_w:
				player_speed[1] = 0;
				break;

			case SDLK_a:
			case SDLK_d:
				player_speed[0] = 0;
				break;
			}

			break;

		case SDL_QUIT:
			game_state = END;
			break;
		}
	}
}

void Update()
{
	// BACKGROUNDS
	for (unsigned int i = 1; i < NUM_BACKGROUNDS; ++i)
	{
		background_rects[i].x -= i * 5;
		if (background_rects[i].x < -WINDOW_WIDTH)
		{
			background_rects[i].x = 0;
		}
	}

	// PLAYER
	player_rect.x += player_speed[0];
	player_rect.x = MAX(0, player_rect.x);
	player_rect.x = MIN(WINDOW_WIDTH, player_rect.x);

	player_rect.y += player_speed[1];
	player_rect.y = MAX(0, player_rect.y);
	player_rect.y = MIN(WINDOW_HEIGHT - WINDOW_BAR_SIZE, player_rect.y);

	// SHOTS
	for (unsigned int i = 0; i < MAX_SHOOTS; ++i)
	{
		if (shoots_alive[i])
		{
			shot_rects[i].x += shoot_speed;
			if (shot_rects[i].x > WINDOW_WIDTH)
			{
				DestroyShoot(i);
			}

			unsigned int intersected_enemy = IntersectedEnemy(&shot_rects[i]);
			if (intersected_enemy != -1)
			{
				DestroyEnemy(intersected_enemy);
				DestroyShoot(i);
			}
		}
	}

	// ENEMIES
	for (unsigned int i = 0; i < MAX_ENEMIES; ++i)
	{
		if (enemies_alive[i])
		{
			enemy_rects[i].x -= ENEMY_SPEED;
			if (enemy_rects[i].x < 0)
			{
				DestroyEnemy(i);
			}

			if (SDL_HasIntersection(&player_rect, &enemy_rects[i]))
			{
				DestroyEnemy(i);
				DestroyPlayer();
			}
		}
	}
	time_since_last_wave += delta_time;
	if (time_since_last_wave >= ENEMY_WAVE_SPAWN_TIME)
	{
		time_since_last_wave = 0;
		time_since_last_enemy = ENEMY_SPAWN_TIME;

		current_wave_num_enemies_spawned = 0;
		spawn_position = MIN(rand() % WINDOW_HEIGHT, WINDOW_HEIGHT - WINDOW_BAR_SIZE);
	}

	if (current_wave_num_enemies_spawned != -1)
	{
		time_since_last_enemy += delta_time;
		if (time_since_last_enemy >= ENEMY_SPAWN_TIME)
		{
			time_since_last_enemy = 0;

			enemies_alive[current_wave_num_enemies_spawned] = 1;
			enemy_rects[current_wave_num_enemies_spawned].x = WINDOW_WIDTH - enemy_rects[current_wave_num_enemies_spawned].w;
			enemy_rects[current_wave_num_enemies_spawned].y = spawn_position;
			if (++current_wave_num_enemies_spawned == MAX_ENEMIES)
			{
				current_wave_num_enemies_spawned = -1;
			}
		}
	}
}

void Render()
{
	for (unsigned int i = 0; i < NUM_BACKGROUNDS; ++i)
	{
		SDL_RenderCopy(renderer, background_textures[i], NULL, &background_rects[i]);
		if (i != 0)
		{
			SDL_Rect aux_rect = background_rects[i];
			aux_rect.x += aux_rect.w;
			SDL_RenderCopy(renderer, background_textures[i], NULL, &aux_rect);
		}
	}

	SDL_RenderCopy(renderer, player_ship_texture, NULL, &player_rect); 

	for (unsigned int i = 0; i < MAX_SHOOTS; ++i)
	{
		if (shoots_alive[i])
		{
			SDL_RenderCopy(renderer, shot_texture, NULL, &shot_rects[i]);
		}
	}

	for (unsigned int i = 0; i < MAX_ENEMIES; ++i)
	{
		if (enemies_alive[i])
		{
			SDL_RenderCopy(renderer, enemy_ship_texture, NULL, &enemy_rects[i]);
		}
	}

	SDL_RenderPresent(renderer);
}

void SpawnShoot()
{
	unsigned int i = 0;
	while (i < MAX_SHOOTS)
	{
		if (!shoots_alive[i])
		{
			shoots_alive[i] = 1;
			Mix_PlayChannel(-1, laser_fx, 0);
			shot_rects[i].x = player_rect.x + shot_rects[i].w;
			shot_rects[i].y = player_rect.y;
			return;
		}
		++i;
	}
}

void DestroyShoot(unsigned int index)
{
	shoots_alive[index] = 0;
	shot_rects[index].x = -100;
	shot_rects[index].y = -100;
}

void DestroyEnemy(unsigned int index)
{
	enemies_alive[index] = 0;
	enemy_rects[index].x = -100;
	enemy_rects[index].y = -100;
}

int IntersectedEnemy(const SDL_Rect* shoot_rect)
{
	unsigned int i = 0;
	while (i < MAX_ENEMIES)
	{
		if (enemies_alive[i] && SDL_HasIntersection(shoot_rect, &enemy_rects[i]))
		{
			return i;
		}
		++i;
	}

	return -1;
}

void DestroyPlayer()
{
	game_state = PLAYER_DYING;
}