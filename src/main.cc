#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <list>
#include <cmath>
#include <vector>
#include <regex>

#define SCREEN_WIDTH  256
#define SCREEN_HEIGHT 150
#define SCREEN_FPS     60

#define PI 3.14159265359

using namespace std;
int render_scale = 5;

#include "loader.h"
#include "saveload.h"
#include "utility.h"
#include "drawable.h"
#include "clickable.h"
#include "cardpanel.h"

#include "text.h"
#include "scene.h"
#include "scene3d.h"
#include "particle.h"
#include "button.h"
#include "card.h"

// Particle effects
#include "snow.h"

// Scenes
#include "scenes/intro.h"
#include "scenes/living.h"
#include "scenes/jeep.h"
#include "scenes/garage.h"
#include "scenes/forest.h"
#include "scenes/cards.h"
#include "scenes/test3d.h"

int main(int argc, char **argv){
#include "assetblob"

	SDL_Event event;

    if(SDL_Init(SDL_INIT_VIDEO)){
		cout << "Failed to init SDL: " << SDL_GetError() << endl;
		return -1;
	}
	SDL_ShowCursor(SDL_DISABLE);

	// Enable audio
	if(Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096)){
		cout << "Failed to initialize audio." << endl;
		return -2;
	}

	// Automatically set default value based on desktop resolution.
	{
		SDL_DisplayMode mode;
		if(!SDL_GetDesktopDisplayMode(0, &mode)){
			int width = mode.w / SCREEN_WIDTH;
			int height = mode.h / SCREEN_HEIGHT;

			render_scale = ((width > height) ? height : width) - 1;
		}
	}

	SDL_Window *win = SDL_CreateWindow("picogamo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * render_scale, SCREEN_HEIGHT * render_scale, SDL_WINDOW_SHOWN);

	// Create a renderer for the window we'll draw everything to.
	SDL_Renderer *rend = SDL_CreateRenderer(win, -1, 0);
	SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
	SDL_RenderSetScale(rend, render_scale, render_scale);

	SDL_Texture *mouse_tx_1 = textureFromBmp(rend, "mouse/cursor.bmp", true);
	SDL_Texture *mouse_tx_2 = textureFromBmp(rend, "mouse/cursor2.bmp", true);
	SDL_Texture *mouse_tx = mouse_tx_1;

	map<int, bool> *keys = new map<int, bool>();

	// Create pointers to scene constructors by name.
	Scene::reg("intro", scene_create<IntroSplashScene>);
	Scene::reg("living", scene_create<LivingRoomScene>);
	Scene::reg("jeep", scene_create<JeepScene>);
	Scene::reg("garage", scene_create<GarageScene>);
	Scene::reg("forest", scene_create<ForestScene>);
	Scene::reg("cards", scene_create<CardsScene>);
	Scene::reg("test3d", scene_create<TestScene3D>);

	Scene::Controller *ctrl = new Scene::Controller(rend, keys);
	ctrl->set_scene(Scene::create(ctrl, "intro"));

	// Intro splash
	/*{
		int ticks_start = SDL_GetTicks();
		int ticks_prev = ticks_start;
		bool skip = false;

		while(!skip){
			int ticks_now = SDL_GetTicks();
			int ticks = (ticks_now - ticks_prev);
			ticks_prev = ticks_now;

			ctrl->draw(ticks);

			SDL_RenderPresent(rend);

			if((ticks_now - ticks_start) > 3000){
				while(SDL_PollEvent(&event)){
					if(event.type == SDL_KEYDOWN){
						skip = true;
						break;
					}
				}
			}

			SDL_Delay(1000 / 60);
		}
	}*/

	// Simple rectangle representing the mouse cursor.
	SDL_Rect mouse_cursor = { SCREEN_WIDTH, SCREEN_HEIGHT, 14, 14 };

	// Load the first scene.
	ctrl->set_scene(Scene::create(ctrl, "forest"));


	// Frame timer for FPS display
	int frame_counter = 0;
	list<int> frame_times;
	PicoText *frame_text = new PicoText(rend, (SDL_Rect){
		SCREEN_WIDTH - 25, 5,
		20, 10
	}, "");


	int ticks_last = SDL_GetTicks();
	while(1){
		int ticks_now = SDL_GetTicks();
		const int ticks = ticks_now - ticks_last;

		// Check for an event without waiting.
		while(SDL_PollEvent(&event)){
			switch(event.type){
				case SDL_QUIT:
					goto quit;

				// Map out keystates
				case SDL_KEYUP:
					(*keys)[event.key.keysym.sym] = false;
					break;
				case SDL_KEYDOWN:
					(*keys)[event.key.keysym.sym] = true;
					break;

				case SDL_MOUSEMOTION:
					// Move cursor to point position.
					mouse_cursor.x = event.motion.x / render_scale;
					mouse_cursor.y = event.motion.y / render_scale;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					ctrl->check_mouse(event);
					break;
			}
		}

		// Test multiple mouse cursors.
		if((*keys)[SDLK_2])
			mouse_tx = mouse_tx_2;
		if((*keys)[SDLK_3])
			mouse_tx = mouse_tx_1;

		// Draw the current scene.
		ctrl->draw(ticks);

		// Draw the mouse cursor
		if(SDL_GetRelativeMouseMode() != SDL_TRUE)
			SDL_RenderCopy(rend, mouse_tx, NULL, &mouse_cursor);

		frame_text->draw(ticks);

		// Flip the display buffer.
		SDL_RenderPresent(rend);

		// Delay to limit to approximately SCREEN_FPS.
		{
			int frame_time = SDL_GetTicks() - ticks_now;
			int delta = (1000 / SCREEN_FPS) - frame_time;

			if(delta >= 0)
				SDL_Delay(delta);

			frame_times.push_back(frame_time);
			if(frame_times.size() > 10)
				frame_times.pop_front();

			float avg = 0;

			for(int t : frame_times)
				avg += t;

			avg /= frame_times.size();
			if(avg == 0.0)
				avg = 0.1;

			if(frame_counter++ >= 10){
				frame_text->set_message(to_string((int)(1000.0 / avg)));
				frame_counter = 0;
			}
		}

		// Update tick counter.
		ticks_last = ticks_now;
	}

quit:
	// Clean up and close SDL library.
	Mix_CloseAudio();
	SDL_Quit();

	return 0;
}
