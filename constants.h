#if !defined(CONSTANTS)

#define INITIAL_SCREEN_WIDTH	800
#define INITIAL_SCREEN_HEIGHT	600

#define FRAMES_PER_SECOND 60

#define UP		1
#define DOWN	2
#define LEFT	4
#define RIGHT	8

#define PLAYER_SPEED 1

#define GRID_FLOOR  0
#define GRID_WALL   1
#define GRID_EXIT   2

#define NUM_MODELS	10
#define NUM_MATS	9

#define MODE_PLAY				0
#define MODE_FREE_CAMERA		1
#define MODE_LEVEL_EDIT			2
#define MODE_REPLAY				3
#define MODE_PAUSED				4

#define EDITOR_MODE_ENEMY		0
#define EDITOR_MODE_WALL		1

#define MAX_LEVELS			  50
#define MAX_BULLETS			  250
#define MAX_ENEMIES			  20
#define MAX_PARTICLE_EMITTERS 30
#define MAX_LIGHTS			  500
#define MAX_WALLS			  500
#define MAX_GRID_ONE_DIM	  500
#define MAX_INPUTS			  10000

// grid constants in world state -- don't use anything but 0 and 1 right now, but with level gen, this will come in handy for entity placement
#define NOTHING		0
#define WALL		1
#define PLAYER		2
#define ENEMY_1		3
#define ENEMY_2		4

#define CONSTANTS
#endif // !CONSTANTS
