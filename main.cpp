#include <iostream>
#include <SDL3/SDL.h>
#include <geGL/geGL.h>
#include <geGL/StaticCalls.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <glm/gtc/constants.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//stb_image.h

using namespace ge::gl;

struct SphereVertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

void createSphere(std::vector<SphereVertex>& vertices, std::vector<uint32_t>& indices,
	int stacks = 64, int slices = 128, float radius = 1.f) {
	vertices.clear();
	indices.clear();
	for (int i = 0; i <= stacks; ++i) {
		float v = (float)i / stacks;
		float lat = glm::half_pi<float>() - v * glm::pi<float>();
		for (int j = 0; j <= slices; ++j) {
			float u = (float)j / slices;
			float lon = (u - 0.5f) * glm::two_pi<float>();
			glm::vec3 n(cos(lat) * cos(lon), sin(lat), -cos(lat) * sin(lon)); //n is the vector that points to the surface of the sphere
			vertices.push_back({ 
				n * radius, //position
				n, //normal
				{ u, v } }); //coords uv
		}
	}
	// indices of the two trianges of each 
	for (int i = 0; i < stacks; ++i)
		for (int j = 0; j < slices; ++j) {
			uint32_t a = i * (slices + 1) + j;
			uint32_t b = a + slices + 1;
			indices.insert(indices.end(), { a, b, a + 1,  a + 1, b, b + 1 });
		}
}

GLuint createShader(GLenum type, char const* src) {
	auto vs = glCreateShader(type);
	glShaderSource(vs, 1, &src, nullptr);
	glCompileShader(vs);
	GLint status;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[1000];
		glGetShaderInfoLog(vs, 1000, nullptr, buffer);
		if (type == GL_VERTEX_SHADER) std::cerr << "Vertex ";
		if (type == GL_FRAGMENT_SHADER) std::cerr << "Fragment ";
		std::cerr << "Shader Compilation Failed: " << buffer << std::endl;
	}
	return vs;
}

GLuint createProgram(std::vector<GLuint>const& shs) {
	auto prg = glCreateProgram();
	for (auto const& s : shs)
		glAttachShader(prg, s);
	glLinkProgram(prg);
	GLint status;
	glGetProgramiv(prg, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[1000];
		glGetProgramInfoLog(prg, 1000, nullptr, buffer);
		std::cerr << "Program Linking Failed: " << buffer << std::endl;
	}
	return prg;
}

int main(int argc, char* argv[]) {
	std::cerr << "Hello PGRE!" << std::endl;
	int width = 1024;
	int height = 768;

	auto window = SDL_CreateWindow("PGRE", width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	auto ctx = SDL_GL_CreateContext(window);
	ge::gl::init();

	auto vsSrc = R".(
  #version 450
  out vec4 vColor;
  uniform float iTime;
  uniform mat4 modelMatrix = mat4(1);
  uniform mat4 viewMatrix = mat4(1);
  uniform mat4 projMatrix = mat4(1);

  layout(location=0)in vec3 position;
  layout(location=1)in vec3 normal;
  layout(location=2)in vec2 texCoord;
  out vec2 uv;

  void main(){
    mat4 mpv = projMatrix * viewMatrix * modelMatrix;
    gl_Position = mpv*vec4(position,1);
    vColor = vec4(normal,1);
    uv = texCoord;
  }).";

	auto fsSrc = R".(
  #version 450
  in vec4 vColor;
  
  out vec4 fColor;

  in vec2 uv;
  layout(binding=0)uniform sampler2D tex;

  void main(){
    fColor = texture(tex,uv);
  }).";

	int imgWidth, imgHeight, imgChannels;
	unsigned char* pixels = stbi_load("earth_daymap.jpg", &imgWidth, &imgHeight, &imgChannels, 4);

	GLuint tex;
	glCreateTextures(GL_TEXTURE_2D, 1, &tex);
	glTextureStorage2D(tex, 1, GL_RGBA8, imgWidth, imgHeight);
	glTextureSubImage2D(tex, 0, 0, 0, imgWidth, imgHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	stbi_image_free(pixels);

	glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTextureUnit(0, tex);


	auto vs = createShader(GL_VERTEX_SHADER, vsSrc);
	auto fs = createShader(GL_FRAGMENT_SHADER, fsSrc);
	auto prg = createProgram({ vs,fs });

	float iTime = 0;
	auto iTimeL = glGetUniformLocation(prg, "iTime");
	auto modelMatrixL = glGetUniformLocation(prg, "modelMatrix");
	auto viewMatrixL = glGetUniformLocation(prg, "viewMatrix");
	auto projMatrixL = glGetUniformLocation(prg, "projMatrix");


	std::vector<SphereVertex> sphereVertices;
	std::vector<uint32_t> sphereIndices;
	createSphere(sphereVertices, sphereIndices);

	GLuint vbo;
	glCreateBuffers(1, &vbo);
	glNamedBufferData(vbo, sphereVertices.size() * sizeof(SphereVertex), sphereVertices.data(), GL_DYNAMIC_COPY);

	GLuint ebo;
	glCreateBuffers(1, &ebo);
	glNamedBufferData(ebo, sphereIndices.size() * sizeof(uint32_t), sphereIndices.data(), GL_DYNAMIC_COPY);

	GLuint vao;
	glCreateVertexArrays(1, &vao);

	glEnableVertexArrayAttrib(vao, 0);
	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayVertexBuffer(vao, 0, vbo, sizeof(float) * 0, sizeof(float) * 8);

	glEnableVertexArrayAttrib(vao, 1);
	glVertexArrayAttribBinding(vao, 1, 1);
	glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayVertexBuffer(vao, 1, vbo, sizeof(float) * 3, sizeof(float) * 8);

	glEnableVertexArrayAttrib(vao, 2);
	glVertexArrayAttribBinding(vao, 2, 2);
	glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayVertexBuffer(vao, 2, vbo, sizeof(float) * 6, sizeof(float) * 8);

	glVertexArrayElementBuffer(vao, ebo);

	glEnable(GL_DEPTH_TEST);

	bool running = true;
	float aspectRatio = (float)width / (float)height;
	float cameraAngleX = 0.f;
	float cameraAngleY = 0.f;
	float sensitivity = 0.01f;
	float cameraSpeed = 0.01f;
	auto cameraPosition = glm::vec3(0.f, 0.f, 3.f);
	std::map<int, bool>keys;
	while (running) { //main loop
		SDL_Event event;
		while (SDL_PollEvent(&event)) { // event loop
			if (event.type == SDL_EVENT_QUIT)
				running = false;
			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
				glViewport(0, 0, event.window.data1, event.window.data2);
				aspectRatio = (float)event.window.data1 / (float)event.window.data2;
			}
			if (event.type == SDL_EVENT_MOUSE_MOTION) {
				if (event.motion.state & SDL_BUTTON_LMASK) {
					cameraAngleY += event.motion.xrel * sensitivity;
					cameraAngleX += event.motion.yrel * sensitivity;
				}
			}
			if (event.type == SDL_EVENT_KEY_DOWN)keys[event.key.key] = true;
			if (event.type == SDL_EVENT_KEY_UP)keys[event.key.key] = false;
		}
		auto cameraRotX = glm::rotate(glm::mat4(1.f), cameraAngleX, glm::vec3(1.f, 0.f, 0.f));
		auto cameraRotY = glm::rotate(glm::mat4(1.f), cameraAngleY, glm::vec3(0.f, 1.f, 0.f));
		auto viewRotation = cameraRotX * cameraRotY;
		auto viewTranslation = glm::translate(glm::mat4(1.f), -cameraPosition);

		auto keyVector = glm::vec4(keys[SDLK_D] - keys[SDLK_A], keys[SDLK_SPACE] - keys[SDLK_LSHIFT], keys[SDLK_S] - keys[SDLK_W], 0.f);
		cameraPosition += glm::vec3(keyVector * viewRotation) * cameraSpeed;

		auto modelMatrix = glm::rotate(glm::mat4(1.f), iTime * 0.1f, glm::vec3(0.f, 1.f, 0.f));
		auto viewMatrix = viewRotation * viewTranslation;
		auto projMatrix = glm::perspective(glm::half_pi<float>(), aspectRatio, 0.1f, 1000.f);

		glClearColor(0.1, 0.1, 0.1, 1); //sets clear color                         
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear color buffer
		glUseProgram(prg);
		glUniform1f(iTimeL, iTime);
		glUniformMatrix4fv(modelMatrixL, 1, GL_FALSE, (float*)&modelMatrix);
		glUniformMatrix4fv(viewMatrixL, 1, GL_FALSE, (float*)&viewMatrix);
		glUniformMatrix4fv(projMatrixL, 1, GL_FALSE, (float*)&projMatrix);
		iTime += 0.01;
		glPointSize(10);
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, nullptr);


		SDL_GL_SwapWindow(window); // swap front & back buffer
	}

	SDL_GL_DestroyContext(ctx);
	SDL_DestroyWindow(window);
	return 0;
}