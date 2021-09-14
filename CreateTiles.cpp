// Inspired by https://github.com/lassyla/game1/blob/master/pack_tiles.cpp
// Reads PNG files in images directory as binary data and stores into tiles.bin,
// which will be read into PlayMode to create the sprites and rooms

#include <vector>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "PPU466.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "Room.hpp"

int main(int argc, char** argv) {
	std::cout << "Hey bestie... \n";
	// Initialize palette and tile tables -- will be packed in same order as path arrays,
	// i.e., sprite_paths and room_paths. Last palette and tile are reserved for background
	std::vector< PPU466::Palette > palette_table(8);
	std::vector< PPU466::Tile > tile_table(16 * 16);
	std::vector< Room > rooms;

	// PNG filenames
	int num_sprites = 7;
	std::string sprite_paths[7] = { "../images/flame.png",
								    "../images/unlit_torch.png",
									"../images/lit_torch.png",
									"../images/chest.png",
									"../images/key.png",
									"../images/explosion.png",
									"../images/door.png"}; // Update for loop count if adding sprites

	int num_rooms = 3;
	std::string room_paths[3] = { "../images/room0.png",
								  "../images/room1.png",
								  "../images/room2.png"};

	std::string bg_path = "../images/background.png";

	// Binary data to be read in from PNGs
	std::vector< glm::u8vec4 > data;

	// Size of all PNGs is 8x8 pixels
	glm::uvec2 size = glm::uvec2(8, 8); // Change if you make bigger sprites

	// Load background palette
	load_png(data_path(bg_path), &size, &data, UpperLeftOrigin);
	PPU466::Palette bg_palette = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		data[0],
		data[1],
		glm::u8vec4(0x00, 0x00, 0x00, 0x00)
	};
	palette_table[7] = bg_palette;

	PPU466::Tile bg_tile;
	for (int i = 0; i < 8; i++) {
		bg_tile.bit1[i] = 0; // All 0's
		bg_tile.bit0[i] = 255; // All 1's
	}
	tile_table[255] = bg_tile;

	// Load sprite PNGs
	for (int i = 0; i < num_sprites; i++) {
		std::cout << sprite_paths[i] << std::endl;
		load_png(data_path(sprite_paths[i]), &size, &data, UpperLeftOrigin); // TODO: Upper or LowerLeftOrigin?

		PPU466::Palette palette = {
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		};

		PPU466::Tile tile;
		tile.bit0 = { 0, 0, 0, 0, 0, 0, 0, 0 };
		tile.bit1 = { 0, 0, 0, 0, 0, 0, 0, 0 };

		// Loop through every pixel
		int colors_found = 0; // Keeps track of how many colors we've added to palette
		for (uint8_t y = 0; y < 8; y++) { // Change if you make bigger sprites
			for (uint8_t x = 0; x < 8; x++) {
				glm::u8vec4 pixel_color = data[(8 * (7 - y)) + x];

				// If this color is already in our palette, get palette index
				bool in_palette = false;
				for (uint8_t palette_idx = 0; palette_idx < colors_found; palette_idx++) {
					if (pixel_color == palette[palette_idx]) {
						in_palette = true;
						uint8_t palette_idx_bit0 = palette_idx & 0b1;
						uint8_t palette_idx_bit1 = (palette_idx >> 1) & 0b1;
						tile.bit0[y] = tile.bit0[y] | (palette_idx_bit0 << x);
						tile.bit1[y] = tile.bit1[y] | (palette_idx_bit1 << x);
					}
				}
				// If the color is not in our palette, add it to the palette
				if (!in_palette) {
					palette[colors_found] = pixel_color;
					uint8_t palette_idx_bit0 = colors_found & 0b1;
					uint8_t palette_idx_bit1 = (colors_found >> 1) & 0b1;
					tile.bit0[y] = tile.bit0[y] | (palette_idx_bit0 << x);
					tile.bit1[y] = tile.bit1[y] | (palette_idx_bit1 << x);

					colors_found++;
				}
			}
		}
		// Add palette to palette table (same order as sprite_paths)
		palette_table[i] = palette;
		std::cout << "Printing palette: \n";
		for (int j = 0; j < 4; j++) {
			std::cout << j << ": " << glm::to_string(palette[j]) << std::endl;
		}

		// Add tile to tile table (same order as sprite_paths)
		tile_table[i] = tile;
		std::cout << "Printing tile: \n";
		for (int j = 0; j < 8; j++) {
			std::cout << j << ": " << unsigned(tile.bit1[j]) << ", " << unsigned(tile.bit0[j]) << "\n";
		}
	}

	for (int i = 0; i < num_rooms; i++) {
		std::cout << room_paths[i] << std::endl;
		load_png(data_path(room_paths[i]), &size, &data, UpperLeftOrigin);

		Room new_room;

		int num_objects = 0;

		while (data[num_objects].w != 0x00) { // Alpha value -- if transparent, it's not info
			std::cout << "Unpacking a new object" << std::endl;
			std::cout << "A value: " << unsigned(data[num_objects].w) << std::endl;
			Object new_obj;
			new_obj.obj_type = data[num_objects].x; // Stored in PNG as R value
			std::cout << "R value: " << unsigned(new_obj.obj_type) << std::endl;
			new_obj.x        = data[num_objects].y; // Stored in PNG as G value
			std::cout << "G value: " << unsigned(new_obj.x) << std::endl;
			new_obj.y        = data[num_objects].z; // Stored in PNG as B value
			std::cout << "B value: " << unsigned(new_obj.y) << std::endl;

			new_room.objects.push_back(new_obj);

			num_objects++;
		}

		rooms.push_back(new_room);
		//DON'T FORGET TO WRITE_CHUNK BELOW
	}

	// Write binary to tiles.bin
	std::ofstream out(data_path("../tiles.bin"), std::ios::binary);
	write_chunk("pal0", palette_table, &out);
	write_chunk("til1", tile_table,    &out);
	//write_chunk("rom2", rooms,         &out); // For some reason, there was a problem
												// reading this in PlayMode...? I suspect
												// it has something to do with how rooms
												// is a vector of structs, each containing
												// its own vector of structs
	write_chunk("rom2", rooms[0].objects, &out);
	write_chunk("rom3", rooms[1].objects, &out);
	write_chunk("rom4", rooms[2].objects, &out);

}