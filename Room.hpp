#include <vector>

struct Object {
	int obj_type = 3;     // 0 for torch, 1 for key, 2 for bomb
	bool reached = false; // Has been reached by player
	uint8_t x;            // x position
	uint8_t y;            // y position
};

struct Room {
	bool locked = true;            // It's only a chest until the player opens it
	std::vector< Object > objects; // All the objects in this room
};