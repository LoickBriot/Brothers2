// Sylvain.Lefebvre@inria.fr  2015-03-10
// ------------------------------------------------------------------

#include "common.h"
#include "drawimage.h"

LIBSL_WIN32_FIX;

// ------------------------------------------------------------------

#include "tilemap.h"
#include "entity.h"

// ------------------------------------------------------------------

// Constants
const int    c_ScreenW = 800;
const int    c_ScreenH = 600;

// ------------------------------------------------------------------

time_t          g_LastFrame = 0;

bool            g_Keys[256];

Tilemap        *g_Tilemap = NULL;

vector<Entity*> g_Entities;

// ------------------------------------------------------------------

// 'mainKeyPressed' is called everytime a key is pressed
void mainKeyPressed(uchar key)
{
	g_Keys[key] = true;
}

// ------------------------------------------------------------------

// 'mainKeyUnpressed' is called everytime a key is released
void mainKeyUnpressed(uchar key)
{
	g_Keys[key] = false;
}

// ------------------------------------------------------------------

// 'mainRender' is called everytime the screen is drawn
void mainRender()
{

	//// Physics

	// -> compute elapsed time
	time_t now = milliseconds();
	time_t el = now - g_LastFrame;
	if (el > 50) {
		g_LastFrame = now;
	}
	// -> check for contact between entities
	for (int a = 0; a < (int)g_Entities.size(); a++) {
		for (int b = a + 1; b < (int)g_Entities.size(); b++) {
			if (!g_Entities[a]->killed && !g_Entities[b]->killed) {
				if (entity_bbox(g_Entities[a]).intersect(entity_bbox(g_Entities[b]))) {
					entity_contact(g_Entities[a], g_Entities[b]);
					entity_contact(g_Entities[b], g_Entities[a]);
				}
			}
		}
	}

	//// Logic

	// -> step all entities
	for (int a = 0; a < (int)g_Entities.size(); a++) {
		entity_step(g_Entities[a], el);
	}

	//// Display

	clearScreen();
	// -> draw tilemap
	tilemap_draw(g_Tilemap);
	// -> draw all entities
	for (int a = 0; a < (int)g_Entities.size(); a++) {
		entity_draw(g_Entities[a]);
	}

}

// ------------------------------------------------------------------

// 'main' is the starting point of the application
int main(int argc, const char **argv)
{
	try { // error handling

		// opens a window
		SimpleUI::init(c_ScreenW, c_ScreenH, "Tilemap");
		// set the render function to be 'mainRender' defined above
		SimpleUI::onRender = mainRender;
		// set the keyboard function
		SimpleUI::onKeyPressed = mainKeyPressed;
		SimpleUI::onKeyUnpressed = mainKeyUnpressed;

		// init drawimage library
		drawimage_init(c_ScreenW, c_ScreenH);
		// keys
		for (int i = 0; i < 256; i++) {
			g_Keys[i] = false;
		}

		///// Level creation

		// load a tilemap
		g_Tilemap = tilemap_load("level.lua");

		// load a simple entity
		{
			Entity *c = entity_create("coin0", "coin.lua");
			c->pos = v2f(32, 32);
			g_Entities.push_back(c);
		} {
			Entity *c = entity_create("coin1", "coin.lua");
			c->pos = v2f(96, 32);
			g_Entities.push_back(c);
		} {
			Entity *c = entity_create("coin2", "coin.lua");
			c->pos = v2f(128, 32);
			g_Entities.push_back(c);
		} {
			Entity *c = entity_create("player1", "player.lua");
			c->pos = v2f(196, 16);
			g_Entities.push_back(c);
		} {
			Entity *c = entity_create("player2", "ennemy.lua");
			c->pos = v2f(16, 16);
			g_Entities.push_back(c);
		}

		g_LastFrame = milliseconds();

		// enter the main loop
		SimpleUI::loop();

		drawimage_terminate();

		// close the window
		SimpleUI::shutdown();

	}
	catch (Fatal& f) { // error handling
		std::cerr << Console::red << f.message() << Console::gray << std::endl;
	}

	return 0;
}

// ------------------------------------------------------------------
