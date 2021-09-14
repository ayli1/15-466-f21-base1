#include "PlayMode.hpp"
#include "read_write_chunk.hpp"
#include "data_path.hpp"
#include "Load.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>

PlayMode::PlayMode() {
	std::vector< PPU466::Palette > palette_table(8);
	std::vector< PPU466::Tile > tile_table(16 * 16);

	// Read in the sprite and room info
	std::ifstream in(data_path("../tiles.bin"), std::ios::binary);
	read_chunk(in, "pal0", &palette_table);
	read_chunk(in, "til1", &tile_table);
	read_chunk(in, "rom2", &room0);
	read_chunk(in, "rom3", &room1);
	read_chunk(in, "rom4", &room2);

	room = room0; // Initialize current room to be room0

	// Transfer to PPU palette and tile table
	for (int i = 0; i < 8; i++) {
		ppu.palette_table[i] = palette_table[i];
	}

	for (int i = 0; i < 256; i++) {
		ppu.tile_table[i] = tile_table[i];
	}

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 50.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	// No wrapping >:(
	if (player_at.x > (PPU466::ScreenWidth - 8)) {
		player_at.x = PPU466::ScreenWidth - 8;
	}
	if (player_at.x < 0) {
		player_at.x = 0;
	}

	if (player_at.y > (PPU466::ScreenHeight - 8)) {
		player_at.y = PPU466::ScreenHeight - 8;
	}
	if (player_at.y < 0) {
		player_at.y = 0;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	ppu.background_color = ppu.palette_table[7][1];

	//tilemap gets recomputed every frame as some weird plasma thing:
	//NOTE: don't do this in your game! actually make a map or something :-)
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			ppu.background[x + PPU466::BackgroundWidth * y] = (7 << 8) + 255; // tile 255 and palette 7
		}
	}

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

	// Collision check
	// Referenced from https://github.com/15-466/15-466-f21-base0/blob/main/PongMode.cpp
	bool explode = false;
	for (int i = 0; i < room.size(); i++) {
		Object *obj = &room[i];
		if (obj->reached) {
			continue;
		}
		glm::vec2 obj_pos = glm::vec2(obj->x, obj->y);
		glm::vec2 min = glm::max(player_at, obj_pos);
		glm::vec2 max = glm::min(player_at + glm::vec2(8, 8), obj_pos + glm::vec2(8, 8));

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y) continue;

		if (max.x - min.x > max.y - min.y) {
			obj->reached = true;
			if (obj->obj_type == 2) { // If bomb, explode
				explode = true;
			}
		}
	}

	int sprite_idx = 0;
	//player sprite (flame):
	ppu.sprites[sprite_idx].x = int32_t(player_at.x);
	ppu.sprites[sprite_idx].y = int32_t(player_at.y);
	ppu.sprites[sprite_idx].index = 0;
	ppu.sprites[sprite_idx].attributes = 0;

	sprite_idx++;

	bool found_key = false;
	for (int i = 0; i < room.size(); i++) {
		Object *obj = &room[i];
		if (obj->obj_type == 0) { // Torch
			if (obj->reached) {   // Lit torch
				ppu.sprites[sprite_idx].index      = 2;
				ppu.sprites[sprite_idx].attributes = 2;
			}
			else {                // Unlit torch
				ppu.sprites[sprite_idx].index      = 1;
				ppu.sprites[sprite_idx].attributes = 1;
				// If player not close enough, draw behind background (not "illuminated")
				if (glm::distance(glm::vec2(obj->x, obj->y), player_at) > 50) {
					ppu.sprites[sprite_idx].attributes = ppu.sprites[sprite_idx].attributes | (1 << 7);
				}
			}
		}
		else if (obj->obj_type == 1) { // Key
			if (obj->reached) {        // Show as key
				ppu.sprites[sprite_idx].index      = 4;
				ppu.sprites[sprite_idx].attributes = 4;
				found_key = true;
			}
			else {                     // Show as chest
				ppu.sprites[sprite_idx].index      = 3;
				ppu.sprites[sprite_idx].attributes = 3;
				// If player not close enough, draw behind background (not "illuminated")
				if (glm::distance(glm::vec2(obj->x, obj->y), player_at) > 50) {
					ppu.sprites[sprite_idx].attributes = ppu.sprites[sprite_idx].attributes | (1 << 7);
				}
			}
		}
		else if (obj->obj_type == 2) { // Bomb
			if (obj->reached) {        // Show explosion
				ppu.sprites[sprite_idx].index      = 5;
				ppu.sprites[sprite_idx].attributes = 5;
			}
			else {                     // Show as chest
				ppu.sprites[sprite_idx].index      = 3;
				ppu.sprites[sprite_idx].attributes = 3;
				// If player not close enough, draw behind background (not "illuminated")
				if (glm::distance(glm::vec2(obj->x, obj->y), player_at) > 50) {
					ppu.sprites[sprite_idx].attributes = ppu.sprites[sprite_idx].attributes | (1 << 7);
				}
			}
		}
		ppu.sprites[sprite_idx].x = obj->x;
		ppu.sprites[sprite_idx].y = obj->y;
		sprite_idx++;
	}

	// Check if room is complete, i.e. either the key was found or all torches have been lit
	bool all_lit = true;
	if (!found_key) {
		for (int i = 0; i < room.size(); i++) {
			Object* obj = &room[i];
			if ((obj->obj_type == 0) && !(obj->reached)) {
				all_lit = false;
			}
		}
	}

	if (found_key || all_lit) { // Room is complete
		// Show door to next level
		ppu.sprites[sprite_idx].x = 248;
		ppu.sprites[sprite_idx].y = 232;
		ppu.sprites[sprite_idx].index = 6;
		ppu.sprites[sprite_idx].attributes = 6;

		// Once player reaches door (and it's not the last room), go to next room
		if ((room_num != 2) && glm::distance(glm::vec2(248, 232), player_at) < 5) {
			room_num++;
			if (room_num == 1) {
				room = room1;
			}
			else if (room_num == 2) {
				room = room2;
			}
			player_at = glm::vec2(0.0f);
			std::cout << "To the next room!" << std::endl;
		}
	}

	if (explode) {
		for (int i = 0; i < room.size(); i++) {
			Object* obj = &room[i];
			// If it's not a bomb, reset this object (keep showing explosion after reset
			// as a kindness to the player)
			if (obj->obj_type != 2) {
				obj->reached = false;
			}
		}
		// Put player back at starting position
		player_at = glm::vec2(0.0f);
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
