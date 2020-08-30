#if !defined(MODEL)

#include "constants.h"
#include "console.h"

struct Material {
	std::string name;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;

	Material() {};

	Material(std::string name, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) {
		this->name = name;
		
		this->ambient = ambient;
		this->diffuse = diffuse;
		this->specular = specular;
		this->shininess = shininess;
	}
};

struct Materials {
	
	union {
		Material mats[NUM_MATS];

		struct {
			Material light;
			Material emerald;
			Material chrome;
			Material silver;
			Material gold;
			Material blackRubber;
			Material ruby;
			Material grey;
		};
	};

    Materials() {
        // pulled from http://devernay.free.fr/cours/opengl/materials.html
        emerald = Material("emerald", glm::vec3(0.0215f, 0.1745f, 0.0215f),
            glm::vec3(0.07568f, 0.61424f, 0.07568f),
            glm::vec3(0.633f, 0.727811f, 0.633f),
            0.6f);

        chrome = Material("chrome", glm::vec3(0.25f, 0.25f, 0.25f),
            glm::vec3(0.4f, 0.4f, 0.4f),
            glm::vec3(0.774597f, 0.774597f, 0.774597f),
            0.6f);

        silver = Material("silver", glm::vec3(0.19225, 0.19225, 0.19225),
            glm::vec3(0.50754, 0.50754, 0.50754),
            glm::vec3(0.508273, 0.508273, 0.508273),
            0.4f);

        gold = Material("gold", glm::vec3(0.24725, 0.1995, 0.0745),
            glm::vec3(0.75164, 0.60648, 0.22648),
            glm::vec3(0.628281, 0.555802, 0.366065),
            0.4f);

        blackRubber = Material("blackRubber", glm::vec3(0.02, 0.02, 0.02),
            glm::vec3(0.01, 0.01, 0.01),
            glm::vec3(0.4, 0.4, 0.4),
            .078125f);

        ruby = Material("ruby", glm::vec3(0.1745, 0.01175, 0.01175),
            glm::vec3(0.61424, 0.04136, 0.04136),
            glm::vec3(0.727811, 0.626959, 0.626959),
            0.6f);

        light = Material("light", glm::vec3(1.0f),
            glm::vec3(1.0f),
            glm::vec3(1.0f),
            1.0f);

		grey = Material("grey", glm::vec3(0.23f),
			glm::vec3(0.23f),
			glm::vec3(0.23f),
			1.0f);
    }

	~Materials() {};
};

Material* getMaterial(std::string name, Materials *materials) {
	for (int i = 0; i < NUM_MATS; i++) {
		if (materials->mats[i].name == name) {
			return &materials->mats[i];
		}
	}

	return NULL;
}

struct Mesh {

	std::vector<float> vertices;
	std::vector<float> texCoords;
	std::vector<float> normals;
	std::vector<unsigned int> indices;
	std::vector<unsigned int> outlineIndices;

	bool drawOutline = false;

	float scaleFactor;

	Material *material;

	unsigned int VAO_ID;
	unsigned int verticesVBO_ID;
	unsigned int normalsVBO_ID;
	unsigned int EBO_ID;
	unsigned int outline_EBO_ID;

	unsigned int shaderProgramID;

	Mesh() {};

	void draw(glm::vec3 worldOffset, float outlineFactor) {
		glUseProgram(shaderProgramID);
		glBindVertexArray(VAO_ID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
		glDepthFunc(GL_LESS);

		glm::mat4 current_model = glm::translate(glm::mat4(1.0f), worldOffset);
		
		setUniformMat4(shaderProgramID, "model", current_model);
		
		setUniform3f(shaderProgramID, "materialAmbient", material->ambient);
		setUniform3f(shaderProgramID, "materialDiffuse", material->diffuse);
		setUniform3f(shaderProgramID, "materialSpecular", material->specular);
		setUniform1f(shaderProgramID, "materialShininess", material->shininess);

		glDrawElements(GL_TRIANGLES, (GLsizei) indices.size(), GL_UNSIGNED_INT, 0);

		if (drawOutline) {
			setUniform3f(shaderProgramID, "materialAmbient", material->ambient * outlineFactor);
			setUniform3f(shaderProgramID, "materialDiffuse", material->diffuse * outlineFactor);
			setUniform3f(shaderProgramID, "materialSpecular", material->specular * outlineFactor);
			setUniform1f(shaderProgramID, "materialShininess", 1.0f);

			glDepthFunc(GL_LEQUAL);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outline_EBO_ID);
			glDrawElements(GL_LINE_LOOP, (GLsizei)outlineIndices.size(), GL_UNSIGNED_INT, 0);
		}
	}

	void scale(glm::vec3 scale) {
		// NOTE: stride of 6 accounts for normals
		for (int i = 0; i < vertices.size(); i += 6) {
			vertices[i]	  *= scale.x;
			vertices[i+1] *= scale.y;
			vertices[i+2] *= scale.z;
		}

		// Probably a better way to make this transformation in place with openGL.
		reloadToVBOs();
	}

	void reloadToVBOs() {
		glBindBuffer(GL_ARRAY_BUFFER, verticesVBO_ID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(this->vertices[0]) * this->vertices.size(), &this->vertices[0]);
	}

	void setupVAO() {
		if (vertices.size() == 0) {
			std::cout << "ERROR: Mesh VAO setup invoked without vertices" << std::endl;
			return;
		}
		
		if (vertices.size() == 0) {
			std::cout << "ERROR: Mesh VAO setup invoked without indices" << std::endl;
			return;
		}

		glGenVertexArrays(1, &this->VAO_ID);
		glBindVertexArray(this->VAO_ID);

		glGenBuffers(1, &verticesVBO_ID);
		glBindBuffer(GL_ARRAY_BUFFER, verticesVBO_ID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(this->vertices[0]) * this->vertices.size(), &this->vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // points
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // normals
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glGenBuffers(1, &EBO_ID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(this->indices[0]) * this->indices.size(), &this->indices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &outline_EBO_ID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outline_EBO_ID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(this->outlineIndices[0]) * this->outlineIndices.size(), &this->outlineIndices[0], GL_STATIC_DRAW);
	}
};

// www.youtube.com/watch?v=BTGo-JHuCGc
struct Model {
	std::string name;

	std::vector<Mesh> meshes;

	// TODO: Visual Studio likes to think this line isn't here.
	glm::vec3 scaleFactor = glm::vec3(1.0f);

	Model() {
		scaleFactor = glm::vec3(1.0f);		
	}
	Model(std::string name) {
		this->name = name;
		scaleFactor = glm::vec3(1.0f);
	}

	void draw(glm::vec3 worldOffset, float outlineFactor) {
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i].draw(worldOffset, outlineFactor);
		}
	}

	void draw(glm::vec3 worldOffset) {
		draw(worldOffset, 1.0f);
	}

	void scale(glm::vec3 scale) {	
		if(scaleFactor.x != 0.0f) scaleFactor.x *= scale.x;
		else					  scaleFactor.x  = scale.x;

		if (scaleFactor.y != 0.0f) scaleFactor.y *= scale.y;
		else					   scaleFactor.y  = scale.y;

		if (scaleFactor.z != 0.0f) scaleFactor.z *= scale.z;
		else					   scaleFactor.z  = scale.z;
		
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i].scale(scale);
		}
	}
};

struct Models {

	Models() {};
	~Models() {};

	union {
		Model mods[NUM_MODELS];

		struct {
			Model player;
			Model enemy;
			
            Model bullet;
			Model enemyBullet;
			Model bulletPart;

			Model floorModel;
			Model wallTopModel;
			Model wallLeftModel;

			Model lightCube;

			Model guidingGrid;
		};
	};
};

Model* getModel(std::string name, Models *models) {
	for (int i = 0; i < NUM_MODELS; i++) {
		if (models->mods[i].name == name) {
			return &models->mods[i];
		}
	}

	return NULL;
}

void setMaterial(std::string modelName, std::string matName, Materials *materials, Models *models, Console *console) {
	Material *mat = getMaterial(matName, materials);
	Model *model = getModel(modelName, models);
	bool fail = false;

	if (mat == NULL) {
		addTextToBox("Material not found: " + matName, &console->historyTextbox);
		fail = true;
	}
	if (model == NULL) {
		addTextToBox("Model not found: " + modelName, &console->historyTextbox);
		fail = true;
	}
	if (fail) return;

	for (int i = 0; i < model->meshes.size(); i++) {
		model->meshes[i].material = mat;
	}
}

#define MODEL
#endif