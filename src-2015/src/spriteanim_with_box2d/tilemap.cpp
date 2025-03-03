// ------------------------------------------------------------------

#include "common.h"
#include "drawimage.h"
#include "script.h"
#include "tilemap.h"
#include "entity.h"

// ------------------------------------------------------------------

#include "physics.h"
// The World (in physics.cpp)
extern b2World *g_World;
extern int    c_ScreenW;
extern int    c_ScreenH;

// ------------------------------------------------------------------

Tilemap         *g_Current = NULL;
int              tilemapW = 159;
int              tilemapH = 77;

// ------------------------------------------------------------------

void lua_tile(int color, string filename, int x, int y, int w, int h)
{
	try {
		// load image if needed
		if (g_Current->images.find(filename) == g_Current->images.end()) {
			// load!
			DrawImage *image = new DrawImage((executablePath() + "/data/tilemap/" + filename).c_str(), v3b(255, 0, 255));
			// insert
			g_Current->images[filename] = image;
		}
		// store tile definition
		Tile *tile = new Tile;
		tile->image = g_Current->images[filename];
		tile->x = x;
		tile->y = y;
		tile->w = w;
		tile->h = h;
		g_Current->tiles[v3b(color & 255, (color >> 8) & 255, color >> 16)] = tile;
	}
	catch (Fatal& f) { // error handling
		std::cerr << Console::red << f.message() << Console::gray << std::endl;
	}
}

// ------------------------------------------------------------------

void lua_tilemap(string filename, int tw, int th)
{
	try {
		g_Current->tilemap = loadImageRGBA(executablePath()  + "/data/data/" + filename);
		g_Current->tilemap->flipH();
		g_Current->tilew = tw;
		g_Current->tileh = th;
	}
	catch (Fatal& f) { // error handling
		std::cerr << Console::red << f.message() << Console::gray << std::endl;
	}
}

// ------------------------------------------------------------------

int lua_color(int r, int g, int b)
{
	return (b << 16) + (g << 8) + r;
}

// ------------------------------------------------------------------

int lua_tileat(int i, int j)
{
	try {
		v3b pix = v3b(g_Current->tilemap->pixel<Clamp>(i, j));
		return lua_color(pix[0], pix[1], pix[2]);
	}
	catch (Fatal& f) { // error handling
		std::cerr << Console::red << f.message() << Console::gray << std::endl;
	}
	return 0;
}

// ------------------------------------------------------------------

void lua_set_tileat(int i, int j, int clr)
{
	try {
		g_Current->tilemap->pixel(i, j)[0] = clr & 255;
		g_Current->tilemap->pixel(i, j)[1] = (clr >> 8) & 255;
		g_Current->tilemap->pixel(i, j)[2] = (clr >> 16) & 255;
	}
	catch (Fatal& f) { // error handling
		std::cerr << Console::red << f.message() << Console::gray << std::endl;
	}
}

extern vector<v3i> g_Ennemies;
void lua_create_ennemies(int x, int y, int mov)
{
	g_Ennemies.push_back(v3i((int)((x * 850) / 53), (int)((y * 401) / 24), mov));
}

extern vector<v3i> g_Stars;
void lua_create_stars(int x, int y, int mov)
{
	g_Stars.push_back(v3i((int)((x * 850)/53), (int)((y * 401)/24)
		, mov));

	// ------------------------------------------------------------------
}

extern vector<v3i> g_Gems;
void lua_create_gems(int x, int y, int mov){
	g_Gems.push_back(v3i((int)((x * 850) / 53), (int)((y * 401) / 24)
		, mov));
}

int lua_num_tiles_x()
{
	return g_Current->tilemap->w();
}

// ------------------------------------------------------------------

int lua_num_tiles_y()
{
	return g_Current->tilemap->h();
}

// ------------------------------------------------------------------

Tilemap *tilemap_load(string fname)
{
	Tilemap *tilemap = new Tilemap;

	Script *script = script_create();

	// install our own functions into the script
	{
		module(script->lua)
			[
				def("tile", &lua_tile),
				def("tilemap", &lua_tilemap),
				def("color", &lua_color),
				def("tileat", &lua_tileat),
				def("set_tileat", &lua_set_tileat),
				def("num_tiles_x", &lua_num_tiles_x),
				def("num_tiles_y", &lua_num_tiles_y),
				def("create_ennemies", &lua_create_ennemies),
				def("create_stars", &lua_create_stars),
				def("create_gems", &lua_create_gems)
			];
	}
	// load the script (global space gets executed)
	g_Current = tilemap;
	script_load(script, executablePath()  + "/data/scripts/" + fname);
	g_Current = NULL;
	// kill the script (no longer needed)
	script_kill(script);
	delete (script);

	return tilemap;
}

// ------------------------------------------------------------------

void tilemap_bind_to_physics(Tilemap *tmap)
{
	// define the dynamic body for the entire tilemap
	b2BodyDef bodyDef;
	bodyDef.type = b2_staticBody;
	bodyDef.position.Set(0.0f, 0.0f);
	b2Body *body = g_World->CreateBody(&bodyDef);

	ForImage(tmap->tilemap, i, j) {
		v3b pix = v3b(tmap->tilemap->pixel(i, j));
		Tile *tile = tmap->tiles[pix];
		if (tile) {
			// define a box shape.
			b2PolygonShape box;
			box.SetAsBox(
				in_meters(tile->w / 2), in_meters(tile->h / 2),  // size
				b2Vec2(in_meters(i*tmap->tilew + tile->w / 2), in_meters(j*tmap->tileh + tile->h / 2)), // center
				0.0f);
			// define the dynamic body fixture.
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &box;
			// set the box density to be zero, so it will be static.
			fixtureDef.density = 0.0f;
			// override the default friction.
			fixtureDef.friction = 0.99f;
			// how bouncy?
			fixtureDef.restitution = 0.02f;
			// user data; set to NULL to distinguish from entities
			fixtureDef.userData = (void*)NULL;
			// add the shape to the body.
			body->CreateFixture(&fixtureDef);

		}
	}

}




// ------------------------------------------------------------------
void tilemap_draw(Tilemap *tmap, v2i viewpos, int decallage)
{

	ForImage(tmap->tilemap, i, j) {
		v3b pix = v3b(tmap->tilemap->pixel(i, j));
		Tile *tile = tmap->tiles[pix];
		if (tile && (i*tmap->tilew - viewpos[0] <= c_ScreenW && i*tmap->tilew - viewpos[0] > 0 && j*tmap->tileh - viewpos[1] <= c_ScreenH && j*tmap->tileh - viewpos[1] >= 0)) {
			tile->image->drawSub(v2i(i*tmap->tilew, j*tmap->tileh) - v2i(viewpos[0] - decallage, viewpos[1]), v2i(tile->w, tile->h), v2i(tile->x, tile->y), v2i(tile->w, tile->h));
		}
	}

}

// ------------------------------------------------------------------
