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

int main(int argc, char** argv) {
	std::cout << "Heyyy bestie \n";
	// Initialize palette and tile tables -- will be packed in same order as path arrays,
	// i.e., sprite_paths and room_paths. Last palette is reserved for background
	std::vector< PPU466::Palette > palette_table(8);
	std::vector< PPU466::Tile > tile_table(16 * 16);

	// PNG filenames (../images/flame.png)
	int num_sprites = 6;
	std::string sprite_paths[6] = { "../images/flame.png",
								    "../images/unlit_torch.png",
									"../images/lit_torch.png",
									"../images/chest.png",
									"../images/key.png",
									"../images/explosion.png"}; // Update for loop count if adding sprites

	int num_rooms = 1;
	std::string room_paths[1] = { "../images/room0.png" };

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

	// Load sprite PNGs
	for (int i = 0; i < num_sprites; i++) {
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
		load_png(data_path(room_paths[i]), &size, &data, UpperLeftOrigin); // TODO: Upper or LowerLeftOrigin?

		PPU466::Palette palette = {
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		};

		PPU466::Tile tile;
		tile.bit0 = { 0, 0, 0, 0, 0, 0, 0, 0 };
		tile.bit1 = { 0, 0, 0, 0, 0, 0, 0, 0 };

		int num_objects = 0;
		while (data[num_objects].z != 0x00) {
			num_objects++;

		}

		tile_table[num_sprites + i] = tile;
	}

	// Write binary to tiles.bin
	std::ofstream out(data_path("../tiles.bin"), std::ios::binary);
	write_chunk("pal0", palette_table, &out);
	write_chunk("til1", tile_table, &out);

}