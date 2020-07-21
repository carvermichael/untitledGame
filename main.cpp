#include <glad/glad.h>
#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define UP_SIDE		0
#define DOWN_SIDE	1
#define LEFT_SIDE	2
#define RIGHT_SIDE	3

#define WORLD_MAP_SIZE_X 5
#define WORLD_MAP_SIZE_Y 5

#define PLAYER_WORLD_START_X 2
#define PLAYER_WORLD_START_Y 2

#define PLAYER_GRID_START_X 4
#define PLAYER_GRID_START_Y 4

#define ACTION_STATE_SEEKING	0
#define ACTION_STATE_AVOIDANT	1

float gridTopLeftX = -0.8f;
float gridTopLeftY = 0.8f;

float sizeOfSide = 0.2f;

unsigned int w_prevState = GLFW_RELEASE;
unsigned int a_prevState = GLFW_RELEASE;
unsigned int s_prevState = GLFW_RELEASE;
unsigned int d_prevState = GLFW_RELEASE;
unsigned int c_prevState = GLFW_RELEASE;
unsigned int l_prevState = GLFW_RELEASE;
unsigned int spacebar_prevState = GLFW_RELEASE;

glm::vec3 cameraPos = glm::vec3(1.95f, -6.5f, 3.9f); // TODO: calculate this based on the center of the grid
glm::vec3 cameraFront = glm::vec3(0.0f, 0.77f, -0.62f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

float lastCursorX = 400;
float lastCursorY = 300;

float yaw = -90.0f;
float pitch = 48.0f;

const int numRows = 8;
const int numColumns = 8;

unsigned int cube_VAO_ID;
unsigned int player_VAO_ID;

bool firstMouse = true;
bool freeCamera = false;

struct map {
	bool initialized = false;
	unsigned int grid[numColumns][numRows];
};

// This setup will result in a sparse world map. Not a big deal for now, but there is a risk for memory explosion if the size of the possible map expands. (carver - 7-20-20)
map allMaps[WORLD_MAP_SIZE_X][WORLD_MAP_SIZE_Y] = {};

struct character {
	int worldCoordX;
	int worldCoordY;
	int gridCoordX;
	int gridCoordY;

	int actionState;
};

character player;
character theOther;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

bool isTheOtherHere(int worldX, int worldY, int gridX, int gridY) {
	return	theOther.worldCoordX == worldX &&
			theOther.worldCoordY == worldY &&
			theOther.gridCoordX  == gridX  &&
			theOther.gridCoordY  == gridY;
}

bool isMapSpaceEmpty(int worldX, int worldY, int gridX, int gridY) {
	return allMaps[player.worldCoordX][player.worldCoordY].grid[gridY][gridX] == 0;
}

void moveNotPlayer() {
	if (theOther.worldCoordX != player.worldCoordX ||
		theOther.worldCoordY != player.worldCoordY) return;

	int diffToPlayerX = theOther.gridCoordX - player.gridCoordX;
	int diffToPlayerY = theOther.gridCoordY - player.gridCoordY;

	int prospectiveGridCoordX = theOther.gridCoordX;
	int prospectiveGridCoordY = theOther.gridCoordY;

	if (theOther.actionState == ACTION_STATE_SEEKING) {
		
		if (diffToPlayerX == 0 && diffToPlayerY == 0) return;
		if (diffToPlayerX == 0 && abs(diffToPlayerY) == 1) return;
		if (abs(diffToPlayerX) == 1 && diffToPlayerY == 0) return;

		if (rand() % 2 == 0 && diffToPlayerX != 0) {
			if (diffToPlayerX > 0)		prospectiveGridCoordX = theOther.gridCoordX - 1;
			else if (diffToPlayerX < 0) prospectiveGridCoordX = theOther.gridCoordX + 1;
		}
		else {
			if (diffToPlayerY == 0) {
				if (diffToPlayerX > 0)		prospectiveGridCoordX = theOther.gridCoordX - 1;
				else if (diffToPlayerX < 0) prospectiveGridCoordX = theOther.gridCoordX + 1;
			}

			if (diffToPlayerY > 0)		prospectiveGridCoordY = theOther.gridCoordY - 1;
			else if (diffToPlayerY < 0) prospectiveGridCoordY = theOther.gridCoordY + 1;
		}
	}
	// TODO: todo, or not..
	else if (theOther.actionState == ACTION_STATE_AVOIDANT) {
		if (diffToPlayerX > diffToPlayerY) {
			if		(diffToPlayerX > 0)	prospectiveGridCoordX = theOther.gridCoordX + 1;
			else if (diffToPlayerX < 0) prospectiveGridCoordX = theOther.gridCoordX - 1;

			
		}	else {
			if (diffToPlayerY > 0)		prospectiveGridCoordY = theOther.gridCoordY + 1;
			else if (diffToPlayerY < 0) prospectiveGridCoordY = theOther.gridCoordY - 1;
		}
	}

	if (isMapSpaceEmpty(player.worldCoordX, player.worldCoordY, prospectiveGridCoordX, prospectiveGridCoordY)) {
		theOther.gridCoordX = prospectiveGridCoordX;
		theOther.gridCoordY = prospectiveGridCoordY;
	}
}

void processKeyboardInput(GLFWwindow *window) {
	
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	int c_currentState = glfwGetKey(window, GLFW_KEY_C);
	if (c_currentState == GLFW_PRESS && c_prevState == GLFW_RELEASE) {
		freeCamera = !freeCamera;		
	}
	c_prevState = c_currentState;
	
	if (freeCamera) {
		const float cameraSpeed = 5.0f * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cameraPos += cameraSpeed * cameraFront;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cameraPos -= cameraSpeed * cameraFront;
		}
		// strafe movement
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		}
	}
	else {
		// NOTE: This strategy is not nearly robust enough. It relies on polling the keyboard events. 
		//		 Definite possibility of missing a press or release event here. And frame timing has not
		//		 yet been considered. Probably more reading is required. Still, good enough for exploratory work.
		int w_currentState = glfwGetKey(window, GLFW_KEY_W);
		if (w_currentState == GLFW_PRESS && w_prevState == GLFW_RELEASE) {
			moveNotPlayer();
			int prospectiveYCoord = player.gridCoordY - 1;
			if (prospectiveYCoord >= 0 && allMaps[player.worldCoordX][player.worldCoordY].grid[prospectiveYCoord][player.gridCoordX] == 0 &&
				!isTheOtherHere(player.worldCoordX, player.worldCoordY, player.gridCoordX, prospectiveYCoord)) {
				player.gridCoordY = prospectiveYCoord;
			}
			else if (prospectiveYCoord < 0) {
				player.worldCoordY++;
				player.gridCoordY = numRows - 1;
			}
		}
		w_prevState = w_currentState;

		int a_currentState = glfwGetKey(window, GLFW_KEY_A);
		if (a_currentState == GLFW_PRESS && a_prevState == GLFW_RELEASE) {
			moveNotPlayer();
			int prospectiveXCoord = player.gridCoordX - 1;
			if (prospectiveXCoord >= 0 && allMaps[player.worldCoordX][player.worldCoordY].grid[player.gridCoordY][prospectiveXCoord] == 0 &&
				!isTheOtherHere(player.worldCoordX, player.worldCoordY, prospectiveXCoord, player.gridCoordY)) {
				player.gridCoordX = prospectiveXCoord;
			} else if (prospectiveXCoord < 0) {
				player.worldCoordX--;
				player.gridCoordX = numColumns - 1;				
			}

		}
		a_prevState = a_currentState;

		int s_currentState = glfwGetKey(window, GLFW_KEY_S);
		if (s_currentState == GLFW_PRESS && s_prevState == GLFW_RELEASE) {
			moveNotPlayer();
			int prospectiveYCoord = player.gridCoordY + 1;
			if (prospectiveYCoord < numColumns && allMaps[player.worldCoordX][player.worldCoordY].grid[prospectiveYCoord][player.gridCoordX] == 0
				&& !isTheOtherHere(player.worldCoordX, player.worldCoordY, player.gridCoordX, prospectiveYCoord)) {
				player.gridCoordY = prospectiveYCoord;
			} else if (prospectiveYCoord == numRows) {
				player.worldCoordY--;
				player.gridCoordY = 0;
			}

		}
		s_prevState = s_currentState;

		int d_currentState = glfwGetKey(window, GLFW_KEY_D);
		if (d_currentState == GLFW_PRESS && d_prevState == GLFW_RELEASE) {
			moveNotPlayer();
			int prospectiveXCoord = player.gridCoordX + 1;
			if (prospectiveXCoord < numRows && allMaps[player.worldCoordX][player.worldCoordY].grid[player.gridCoordY][prospectiveXCoord] == 0 &&
				!isTheOtherHere(player.worldCoordX, player.worldCoordY, prospectiveXCoord, player.gridCoordY)) {
				player.gridCoordX = prospectiveXCoord;
			} else if (prospectiveXCoord == numColumns) {
				player.worldCoordX++;
				player.gridCoordX = 0;
			}
		}
		d_prevState = d_currentState;

		int spacebar_currentState = glfwGetKey(window, GLFW_KEY_SPACE);
		if (spacebar_currentState == GLFW_PRESS && spacebar_prevState == GLFW_RELEASE) {
			moveNotPlayer();
		}
		spacebar_prevState = spacebar_currentState;

		int l_currentState = glfwGetKey(window, GLFW_KEY_L);
		if (l_currentState == GLFW_PRESS && l_prevState == GLFW_RELEASE) {
			if		(theOther.actionState == ACTION_STATE_AVOIDANT)	theOther.actionState = ACTION_STATE_SEEKING;
			else if (theOther.actionState == ACTION_STATE_SEEKING)	theOther.actionState = ACTION_STATE_AVOIDANT;
		}
		l_prevState = l_currentState;
	}	
}

void mouseInputCallback(GLFWwindow* window, double xPos, double yPos) {
	if (freeCamera) {
		// what is the unit for offsets here? is it really degrees?
		if (firstMouse)
		{
			lastCursorX = (float)xPos;
			lastCursorY = (float)yPos;
			firstMouse = false;
		}

		float xOffset = (float)(xPos - lastCursorX);
		float yOffset = (float)(lastCursorY - yPos);

		lastCursorX = (float)xPos;
		lastCursorY = (float)yPos;

		const float sensitivity = 0.1f;
		xOffset *= sensitivity;
		yOffset *= sensitivity;

		yaw += xOffset;
		pitch += yOffset;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}
}

void createCubeVertices() {
	// NOTE: These coords are in local space
	float cubeVertices[] = {
		0.5, 0.5, 0.5,
		0.0, 0.5, 0.5,
		0.5, 0.0, 0.5,
		0.0, 0.0, 0.5,

		0.5, 0.5, 0.0,
		0.0, 0.5, 0.0,
		0.5, 0.0, 0.0,
		0.0, 0.0, 0.0
	};

	unsigned int cubeIndices[] = {
		0,	1,	2,
		1,	3,	2,

		4,	5,	6,
		5,	7,	6,

		0,	1,	4,
		1,	5,	4,

		2,	3,	7,
		2,	7,	6,

		0,	2,	6,
		0,	6,	4,

		1,	3,	7,
		1,	7,	5
	};
	
	glGenVertexArrays(1, &cube_VAO_ID);

	glBindVertexArray(cube_VAO_ID);

	unsigned int cube_VBO_ID;
	glGenBuffers(1, &cube_VBO_ID);
	glBindBuffer(GL_ARRAY_BUFFER, cube_VBO_ID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	unsigned int cube_EBO_ID;
	glGenBuffers(1, &cube_EBO_ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_EBO_ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
}

void createPlayerVertices() {
	// NOTE: These coords are in local space
	float playerVertices[] = {
		0.5, 0.5, 0.25,
		0.0, 0.5, 0.25,
		0.5, 0.0, 0.25,
		0.0, 0.0, 0.25,

		0.5, 0.5, 0.0,
		0.0, 0.5, 0.0,
		0.5, 0.0, 0.0,
		0.0, 0.0, 0.0
	};

	unsigned int playerIndices[] = {
		0,	1,	2,
		1,	3,	2,

		4,	5,	6,
		5,	7,	6,

		0,	1,	4,
		1,	5,	4,

		2,	3,	7,
		2,	7,	6,

		0,	2,	6,
		0,	6,	4,

		1,	3,	7,
		1,	7,	5
	};


	// TODO: think about pulling the these out into dedicated GPU loading function
	glGenVertexArrays(1, &player_VAO_ID);

	glBindVertexArray(player_VAO_ID);

	unsigned int player_VBO_ID;
	glGenBuffers(1, &player_VBO_ID);
	glBindBuffer(GL_ARRAY_BUFFER, player_VBO_ID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(playerVertices), playerVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	unsigned int player_EBO_ID;
	glGenBuffers(1, &player_EBO_ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, player_EBO_ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(playerIndices), playerIndices, GL_STATIC_DRAW);
}

void createAdjacentMaps(int attachedWorldMapX, int attachedWorldMapY, int directionToGetHere) {
	// TODO: fix this directionToGetHere garbage, way too confusing

	// create walled map
	unsigned int newGrid[numRows][numColumns] = {
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1
	};

	int newGridX;
	int newGridY;	

	// NOTE: hardcoding paths to other maps based on 8x8 grid structure
	switch (directionToGetHere) {
		case (UP_SIDE):
			newGridX = attachedWorldMapX;
			newGridY = attachedWorldMapY + 1;
			newGrid[7][3] = 0;
			newGrid[7][4] = 0;
			break;
		case (DOWN_SIDE):
			newGridX = attachedWorldMapX;
			newGridY = attachedWorldMapY - 1;
			newGrid[0][3] = 0;
			newGrid[0][4] = 0;
			break;
		case (LEFT_SIDE):
			newGridX = attachedWorldMapX - 1;
			newGridY = attachedWorldMapY;
			newGrid[3][7] = 0;
			newGrid[4][7] = 0;
			break;
		case (RIGHT_SIDE):
			newGridX = attachedWorldMapX + 1;
			newGridY = attachedWorldMapY;
			newGrid[3][0] = 0;
			newGrid[4][0] = 0;
			break;
	}

	allMaps[newGridX][newGridY].initialized = true;
		
	// generate down
	if (directionToGetHere != UP_SIDE) {
		if (rand() % 4 == 2 && newGridY > 0 && !allMaps[newGridX][newGridY-1].initialized) {
			newGrid[7][3] = 0;
			newGrid[7][4] = 0;
			createAdjacentMaps(newGridX, newGridY, DOWN_SIDE);
		}
	}

	// generate up
	if (directionToGetHere != DOWN_SIDE) {
		if (rand() % 4 == 2 && newGridY <= WORLD_MAP_SIZE_Y && !allMaps[newGridX][newGridY+1].initialized) {
			newGrid[0][3] = 0;
			newGrid[0][4] = 0;
			createAdjacentMaps(newGridX, newGridY, UP_SIDE);
		}
	}

	// generate right
	if (directionToGetHere != LEFT_SIDE) {
		if (rand() % 4 == 2 && newGridX <= WORLD_MAP_SIZE_X && !allMaps[newGridX+1][newGridY].initialized) {
			newGrid[3][7] = 0;
			newGrid[4][7] = 0;
			createAdjacentMaps(newGridX, newGridY, RIGHT_SIDE);
		}
	}

	// generate left
	if (directionToGetHere != RIGHT_SIDE) {
		if (rand() % 4 == 2 && newGridX > 0 && !allMaps[newGridX-1][newGridY].initialized) {
			newGrid[3][0] = 0;
			newGrid[4][0] = 0;
			createAdjacentMaps(newGridX, newGridY, LEFT_SIDE);
		}
	}

	for (int row = 0; row < numRows; row++) {
		for (int column = 0; column < numColumns; column++) {
			allMaps[newGridX][newGridY].grid[row][column] = newGrid[row][column];
		}
	}
}

int main() {
	// ------------ INIT STUFF -------------

	// initialization of glfw and glad libraries, window creation
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // __APPLE__	

	GLFWwindow* window = glfwCreateWindow(800, 600, "GridGame1", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// setting up Vertex Array Object (VAO)
	unsigned int grid_VAO_ID;
	glGenVertexArrays(1, &grid_VAO_ID);

	glBindVertexArray(grid_VAO_ID);

	// ------------- SHADERS -------------

	// setting up vertex shader
	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	// loading vertex shader file for compilation
	std::string vertexCode;
	std::ifstream vShaderFile;
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	vShaderFile.open("vertexShader.vert");
	std::stringstream vShaderStream;
	vShaderStream << vShaderFile.rdbuf();
	vShaderFile.close();
	vertexCode = vShaderStream.str();
	const char* vShaderCode = vertexCode.c_str();

	glShaderSource(vertexShader, 1, &vShaderCode, NULL);
	glCompileShader(vertexShader);

	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// setting up the fragment shaders
	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	std::string fragmentCode;
	std::ifstream fShaderFile;
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.open("fragmentShader.frag");
	std::stringstream fShaderStream;
	fShaderStream << fShaderFile.rdbuf();
	fShaderFile.close();
	fragmentCode = fShaderStream.str();
	const char* fShaderCode = fragmentCode.c_str();

	glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
	glCompileShader(fragmentShader);

	success = 0;
	memset(infoLog, 0, sizeof(infoLog));
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	// initializing shader program/pipeline, attaching compiled shaders, initialization, linking, and shader cleanup
	unsigned int shaderProgram_ID;
	shaderProgram_ID = glCreateProgram();

	glAttachShader(shaderProgram_ID, vertexShader);
	glAttachShader(shaderProgram_ID, fragmentShader);
	glLinkProgram(shaderProgram_ID);

	success = 0;
	memset(infoLog, 0, sizeof(infoLog));
	glGetProgramiv(shaderProgram_ID, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(shaderProgram_ID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	glUseProgram(shaderProgram_ID);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// start of 3D stuffs
	// creating a model matrix
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 identityModel = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	unsigned int modelLoc = glGetUniformLocation(shaderProgram_ID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// creating a view matrix with camera
	// setting up camera

	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
	
	glm::vec3 cameraRight = glm::normalize(glm::cross(cameraUp, cameraDirection)); // remember: cross product gives you orthongonal vector to both input vectors

	//glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

	glm::mat4 view;
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	
	unsigned int viewLoc = glGetUniformLocation(shaderProgram_ID, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// aaaaand the projection matrix
	glm::mat4 projection;
	projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
	unsigned int projectionLoc = glGetUniformLocation(shaderProgram_ID, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// initializing viewport and setting callback for window resizing
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouseInputCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glViewport(0, 0, 800, 600);

	glEnable(GL_DEPTH_TEST);

	createCubeVertices();
	createPlayerVertices();
	
	glm::vec3 color1 = glm::vec3(0.4f, 1.0f, 1.0f);
	glm::vec3 color2 = glm::vec3(1.0f, 0.5f, 0.5f);

	unsigned int colorUniformLocation = glGetUniformLocation(shaderProgram_ID, "colorIn");
	
	glm::vec3 currentColor = color1;

	bool color = true;

	// MAP GENERATION
	unsigned int startingGrid[numRows][numColumns] = {
		1, 1, 1, 0, 0, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 0, 0, 1, 1, 1
	};

	allMaps[PLAYER_WORLD_START_X][PLAYER_WORLD_START_Y].initialized = true;

	for (int row = 0; row < numRows; row++) {
		for (int column = 0; column < numColumns; column++) {
			allMaps[PLAYER_WORLD_START_X][PLAYER_WORLD_START_Y].grid[row][column] = startingGrid[row][column];
		}
	}

	srand((unsigned int)(glfwGetTime() * 10));
	createAdjacentMaps(PLAYER_WORLD_START_X, PLAYER_WORLD_START_Y, UP_SIDE);
	createAdjacentMaps(PLAYER_WORLD_START_X, PLAYER_WORLD_START_Y, DOWN_SIDE);
	createAdjacentMaps(PLAYER_WORLD_START_X, PLAYER_WORLD_START_Y, LEFT_SIDE);
	createAdjacentMaps(PLAYER_WORLD_START_X, PLAYER_WORLD_START_Y, RIGHT_SIDE);

	player.worldCoordX = PLAYER_WORLD_START_X;
	player.worldCoordY = PLAYER_WORLD_START_Y;
	player.gridCoordX = PLAYER_GRID_START_X;
	player.gridCoordY = PLAYER_GRID_START_Y;

	theOther.worldCoordX = PLAYER_WORLD_START_X;
	theOther.worldCoordY = PLAYER_WORLD_START_Y;
	theOther.gridCoordX = 1;
	theOther.gridCoordY = 2;

	character* characters[2] = { &player, &theOther };
	
	lastFrameTime = (float)glfwGetTime();

	glm::mat4 model2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.0f, 0.0f));

	// game loop
	while (!glfwWindowShouldClose(window)) {
		processKeyboardInput(window);
		
		glfwPollEvents();		

		// Move camera
		cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraDirection.y = sin(glm::radians(pitch));
		cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(cameraDirection);

		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		// Clear color and "z-buffer"
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.7f, 0.3f, 1.0f);	

		// DRAW CURRENT GRID (where the player currently is)
		glBindVertexArray(cube_VAO_ID);
		for (int row = 0; row < numRows; row++) {
			float yOffset = -0.5f * row;
			
			for (int column = 0; column < numColumns; column++) {
				float xOffset = 0.5f * column;

				// position
				glm::mat4 current_model = glm::translate(glm::mat4(1.0f), glm::vec3(xOffset, yOffset, 0.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(current_model));
				
				// color
				if (allMaps[player.worldCoordX][player.worldCoordY].grid[row][column] == 0) {
					glUniform3f(colorUniformLocation, color1.r, color1.g, color1.b);
				}
				else {
					glUniform3f(colorUniformLocation, color2.r, color2.g, color2.b);
				}
				
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			}
		}

		// DRAW CHARACTERS
		int numCharacters = sizeof(characters) / sizeof(character*);
		for (int i = 0; i < numCharacters; i++) {
			character currentCharacter = *characters[i];

			if (currentCharacter.worldCoordX != player.worldCoordX || currentCharacter.worldCoordY != player.worldCoordY) continue;

			float yOffset = -0.5f * currentCharacter.gridCoordY;
			float xOffset = 0.5f * currentCharacter.gridCoordX;

			glBindVertexArray(player_VAO_ID);

			if (i == 0) {
				glUniform3f(colorUniformLocation, 0.0f, 0.45f, 0.03f);
			}
			else {
				glUniform3f(colorUniformLocation, 0.3f, 1.0f, 0.03f);
			}
			
			glm::mat4 current_model = glm::translate(glm::mat4(1.0f), glm::vec3(xOffset, yOffset, 0.5f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(current_model));
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}

		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrameTime;
		lastFrameTime = currentFrame;

		glfwSwapBuffers(window);
	}

	glfwTerminate();

	return 0;	   	 
}