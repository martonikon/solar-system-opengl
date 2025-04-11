#include <GL\glew.h>
#include <chrono>
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <gl\GLU.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <iostream>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include "GeometryNode.h"
#include "GroupNode.h"
#include "TransformNode.h"


bool init();
bool initGL();
void render();
GLuint DrawCube(float, GLuint&, GLuint&);
void DrawCube(GLuint id);
void close();
TransformNode* trEarth = nullptr;
void CreateScene();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

Shader gShader;
Model gModel;
Model gModel2;

GroupNode* gRoot;

GLuint gVAO, gVBO, gEBO;

// camera
Camera camera(glm::vec3(0.5f, 0.5f, 20.0f));
float lastX = -1;
float lastY = -1;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Define a steady clock type for measuring time
using Clock = std::chrono::steady_clock;

// Variables to store the start time
auto startTime = Clock::now();
auto prevTime = startTime;

// lighting
glm::vec3 lightPos(0.5f, 0.5f, 0.5f);

//statics
unsigned int Node::genID;
glm::mat4 TransformNode::transformMatrix = glm::mat4(1.0f);

TransformNode* trMercury;
TransformNode* trVenus;
TransformNode* trEart;
TransformNode* trEarthMoon;
TransformNode* trMars;
TransformNode* trPhobos;
TransformNode* trDeimos;
TransformNode* trJupiter;
TransformNode* trSaturn;
TransformNode* trUranus;
TransformNode* trNeptune;

//event handlers
void HandleKeyDown(const SDL_KeyboardEvent& key);
void HandleMouseMotion(const SDL_MouseMotionEvent& motion);
void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);

glm::vec3 ScreenToWorldRay(int mouseX, int mouseY, glm::mat4 view, glm::mat4 proj)
{
	float x = (2.0f * mouseX) / 1920 - 1.0f;
	float y = 1.0f - (2.0f * mouseY) / 1080;
	glm::vec4 rayClip(x, y, -1.0f, 1.0f);
	glm::vec4 rayEye = glm::inverse(proj) * rayClip;
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
	glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
	return glm::normalize(rayWorld);
}

bool RayIntersectsSphere(glm::vec3 origin, glm::vec3 dir, glm::vec3 center, float radius)
{
	glm::vec3 oc = origin - center;
	float b = glm::dot(oc, dir);
	float c = glm::dot(oc, oc) - radius * radius;
	float h = b * b - c;
	return h >= 0.0f;
}

int main(int argc, char* args[])
{
	init();

	CreateScene();

	SDL_Event e;
	//While application is running
	bool quit = false;
	while (!quit)
	{
		// per-frame time logic
		// --------------------
		float currentFrame = SDL_GetTicks() / 1000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else
				{
					HandleKeyDown(e.key);
				}
				break;

			case SDL_MOUSEMOTION:
				HandleMouseMotion(e.motion);
				break;

			case SDL_MOUSEBUTTONDOWN:
				if (e.button.button == SDL_BUTTON_LEFT)
				{
					glm::mat4 view = camera.GetViewMatrix();
					glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 1920.0f / 1080.0f, 0.1f, 1000.0f);
					glm::vec3 rayDir = ScreenToWorldRay(e.button.x, e.button.y, view, proj);
					glm::vec3 rayOrigin = camera.Position;

					struct PlanetHitTest {
						TransformNode* planet;
						float radius;
					};

					PlanetHitTest planets[] = {
						{trMercury, 2.0f}, {trVenus, 2.0f}, {trEarth, 2.5f}, {trMars, 2.0f},
						{trJupiter, 4.0f}, {trSaturn, 4.0f}, {trUranus, 3.5f}, {trNeptune, 3.5f}

					};

					for (int i = 0; i < sizeof(planets) / sizeof(PlanetHitTest); ++i)
					{
						glm::vec3 pos = planets[i].planet->GetWorldTranslation();
						if (RayIntersectsSphere(rayOrigin, rayDir, pos, planets[i].radius))
						{
							camera.SetFollowTarget(planets[i].planet);
							break;
						}
					}
				}
				break;
		
			case SDL_MOUSEWHEEL:
				HandleMouseWheel(e.wheel);
				break;
			}
		}

		//Render
		render();

		//Update screen
		SDL_GL_SwapWindow(gWindow);
	}

	close();

	return 0;
}

void HandleKeyDown(const SDL_KeyboardEvent& key)
{
	switch (key.keysym.sym)
	{
	case SDLK_w:
		camera.ProcessKeyboard(FORWARD, deltaTime);
		break;
	case SDLK_s:
		camera.ProcessKeyboard(BACKWARD, deltaTime);
		break;
	case SDLK_a:
		camera.ProcessKeyboard(LEFT, deltaTime);
		break;
	case SDLK_d:
		camera.ProcessKeyboard(RIGHT, deltaTime);
		break;

	case SDLK_r:
		std::cout << "Resetting camera follow...\n";
		camera.ClearFollowTarget();
		camera.SetPosition(glm::vec3(0.5f, 0.5f, 20.0f));
		camera.SetFront(glm::vec3(0.0f, 0.0f, -1.0f));
		break;
	}
}

void HandleMouseMotion(const SDL_MouseMotionEvent& motion)
{
	if (firstMouse)
	{
		lastX = motion.x;
		lastY = motion.y;
		firstMouse = false;
	}
	else
	{
		camera.ProcessMouseMovement(motion.x - lastX, lastY - motion.y);
		lastX = motion.x;
		lastY = motion.y;
	}
}

void HandleMouseWheel(const SDL_MouseWheelEvent& wheel)
{
	camera.ProcessMouseScroll(wheel.y);
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);


		//Create window
		gWindow = SDL_CreateWindow("Solar System", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext(gWindow);
			if (gContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Use Vsync
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				//Initialize OpenGL
				if (!initGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}
			}
		}
	}

	return success;
}

bool initGL()
{
	bool success = true;
	GLenum error = GL_NO_ERROR;
	glewExperimental = true;
	glewInit();

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		success = false;
		printf("Error initializing OpenGL! %s\n", gluErrorString(error));
	}

	glClearColor(0.05f, 0.0f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gShader.Load("./shaders/vertex.vert", "./shaders/fragment.frag");

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //other modes GL_FILL, GL_POINT

	return success;
}

void CreateScene()
{
	gRoot = new GroupNode("root");

	TransformNode* trSun = new TransformNode("Sun Transform");
	trSun->SetScale(glm::vec3(0.015f, 0.015f, 0.015f));
	trSun->SetTranslation(glm::vec3(0.5f, 0.5f, 0.5f));
	GeometryNode* sun = new GeometryNode("sun");
	sun->LoadFromFile("models/Sun/13913_Sun_v2_l3.obj");
	sun->SetShader(&gShader);
	trSun->AddChild(sun);
	gRoot->AddChild(trSun);


	trMercury = new TransformNode("Mercury Transform");
	trMercury->SetScale(glm::vec3(0.001f, 0.001f, 0.001f));
	trMercury->SetTranslation(glm::vec3(10.0f, 0.5f, 0.5f));
	trMercury->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	GeometryNode* mercury = new GeometryNode("mercury");
	mercury->LoadFromFile("models/Mercury/13900_Mercury_v1_l3.obj");
	mercury->SetShader(&gShader);
	mercury->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	trMercury->AddChild(mercury);
	gRoot->AddChild(trMercury);

	trVenus = new TransformNode("Venus Transform");
	trVenus->SetScale(glm::vec3(0.002f, 0.002f, 0.002f));
	trVenus->SetTranslation(glm::vec3(20.7f, 0.5f, 0.5f));
	trVenus->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	GeometryNode* venus = new GeometryNode("Venus");
	venus->LoadFromFile("models/Venus/13901_Venus_v1_l3.obj");
	venus->SetShader(&gShader);
	venus->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	trVenus->AddChild(venus);
	gRoot->AddChild(trVenus);


	trEarth = new TransformNode("Earth Transform");
	trEarth->SetScale(glm::vec3(0.0035f, 0.0035f, 0.0035f));
	trEarth->SetTranslation(glm::vec3(30.0f, 0.5f, 0.0f));
	GeometryNode* earth = new GeometryNode("earth");
	earth->LoadFromFile("models/Earth/13902_Earth_v1_l3.obj");
	earth->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	earth->SetShader(&gShader);
	earth->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	trEarth->AddChild(earth);
	gRoot->AddChild(trEarth);

	trEarthMoon = new TransformNode("Earth Moon Transform");
	trEarthMoon->SetScale(glm::vec3(.025f, .025f, .025f));
	trEarthMoon->SetTranslation(glm::vec3(650.0f, 0.5f, 0.0f));
	GeometryNode* earthMoon = new GeometryNode("earthMoon");
	earthMoon->LoadFromFile("models/Deimos/10464_Asteroid_v1_Iterations-2.obj");
	earthMoon->SetShader(&gShader);
	trEarthMoon->AddChild(earthMoon);
	trEarth->AddChild(trEarthMoon);
	

	trMars = new TransformNode("Mars Transform");
	trMars->SetScale(glm::vec3(0.0045f, 0.0045f, 0.0045f));
	trMars->SetTranslation(glm::vec3(40.0f, 0.5f, 0.0f));
	GeometryNode* mars = new GeometryNode("mars");
	mars->LoadFromFile("models/Mars/13903_Mars_v1_l3.obj");
	mars->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	mars->SetShader(&gShader);
	mars->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	trMars->AddChild(mars);
	gRoot->AddChild(trMars);


	trPhobos = new TransformNode("Phobos Transform");
	trPhobos->SetScale(glm::vec3(.05f, .05f, 0.05f));
	trPhobos->SetTranslation(glm::vec3(650.0f, 0.5f, 0.0f));
	GeometryNode* phobos = new GeometryNode("phobos");
	phobos->LoadFromFile("models/Deimos/10464_Asteroid_v1_Iterations-2.obj");
	phobos->SetShader(&gShader);
	trPhobos->AddChild(phobos);
	trMars->AddChild(trPhobos);


	trDeimos = new TransformNode("Deimos Transform");
	trDeimos->SetScale(glm::vec3(0.05f, 0.05f, 0.05f));
	trDeimos->SetTranslation(glm::vec3(-650.0f, 1.5f, 0.0f));
	GeometryNode* deimos = new GeometryNode("deimos");
	deimos->LoadFromFile("models/Deimos/10464_Asteroid_v1_Iterations-2.obj");
	deimos->SetShader(&gShader);
	trDeimos->AddChild(deimos);
	trMars->AddChild(trDeimos);


	trJupiter = new TransformNode("Jupiter Transform");
	trJupiter->SetScale(glm::vec3(0.007f, 0.007f, 0.007f));
	trJupiter->SetTranslation(glm::vec3(50.0f, 0.5f, 0.0f));
	GeometryNode* jupiter = new GeometryNode("jupiter");
	jupiter->LoadFromFile("models/Jupiter/13905_Jupiter_v1_l3.obj");
	jupiter->SetShader(&gShader);
	jupiter->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	trJupiter->AddChild(jupiter);
	gRoot->AddChild(trJupiter);


	trSaturn = new TransformNode("Saturn Transform");
	trSaturn->SetScale(glm::vec3(0.006f, 0.006f, 0.006f));
	trSaturn->SetTranslation(glm::vec3(60.0f, 0.5f, 0.5f));
	GeometryNode* saturn = new GeometryNode("saturn");
	saturn->LoadFromFile("models/Saturn/13906_Saturn_v1_l3.obj");
	saturn->SetShader(&gShader);
	saturn->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	trSaturn->AddChild(saturn);
	gRoot->AddChild(trSaturn);


	trUranus = new TransformNode("Uranus Transform");
	trUranus->SetScale(glm::vec3(0.005f, 0.005f, 0.005f));
	trUranus->SetTranslation(glm::vec3(70.2f, 0.5f, 0.5f));
	GeometryNode* uranus = new GeometryNode("uranus");
	uranus->LoadFromFile("models/Uranus/13907_Uranus_v2_l3.obj");
	uranus->SetShader(&gShader);
	uranus->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	trUranus->AddChild(uranus);
	gRoot->AddChild(trUranus);


	trNeptune = new TransformNode("Neptune Transform");
	trNeptune->SetScale(glm::vec3(0.0048f, 0.0048f, 0.0058f));
	trNeptune->SetTranslation(glm::vec3(80.0f, 0.5f, 0.5f));
	GeometryNode* neptune = new GeometryNode("neptune");
	neptune->LoadFromFile("models/Neptune/13908_Neptune_V2_l3.obj");
	neptune->SetModelRotation(glm::vec3(90.0f, 0.0f, 0.0f));
	neptune->SetShader(&gShader);
	trNeptune->AddChild(neptune);
	gRoot->AddChild(trNeptune);


}
void close()
{
	//delete GL programs, buffers and objects
	glDeleteProgram(gShader.ID);
	glDeleteVertexArrays(1, &gVAO);
	glDeleteBuffers(1, &gVBO);

	//Delete OGL context
	SDL_GL_DeleteContext(gContext);
	//Destroy window	
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}


void render() {
	// Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//newly added camera update
	camera.UpdateFollow(deltaTime);

	// Set up view and projection matrices
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 1920.0f / 1080.0f, 0.1f, 1000.0f);

	// Use the shader program
	glUseProgram(gShader.ID);
	gShader.setMat4("view", view);
	gShader.setMat4("proj", proj);

	// Set light position in view space
	glm::vec4 lightPosView = view * glm::vec4(lightPos, 0.5);
	gShader.setVec3("light.position", glm::vec3(lightPosView));

	// Define orbit radius and orbit speed

	//radius

	float orbitRadiusMercury = 10.0f;
	float orbitRadiusVenus = 15.0f;
	float orbitRadiusEarth = 25.0f;
	float orbitRadiusEarthMoon = 1.0f;
	float orbitRadiusMars = 30.0f;
	float orbitRadiusJupiter = 35.0f;
	float orbitRadiusSaturn = 40.0f;
	float orbitRadiusUranus = 45.0f;
	float orbitRadiusNeptune = 50.0f;

	//speed
	float orbitspeedMerc = 0.055f;
	float orbitspeedVen = 0.05f;
	float orbitspeedEarth = 0.045f;
	float orbitspeedEarthMoon = 0.21f;
	float orbitspeedMars = 0.04f;
	float orbitspeedJup = 0.035f;
	float orbitspeedSat = 0.03f;
	float orbitspeedUr = 0.01f;
	float orbitspeedNep = 0.01f;


	// Calculate current time and orbit angle
	float elapsedTime = SDL_GetTicks() / 1000.0f;
	elapsedTime += deltaTime;

	float orbitAngleMerc = glm::radians((elapsedTime * orbitspeedMerc) * 360.0f);
	float orbitAngleVen = glm::radians((elapsedTime * orbitspeedVen) * 360.0f);
	float orbitAngleEarth = glm::radians((elapsedTime * orbitspeedEarth) * 360.0f);
	float orbitAngleEarthMoon = glm::radians((elapsedTime * orbitspeedEarthMoon) * 360.0f);
	float orbitAngleMars = glm::radians((elapsedTime * orbitspeedMars) * 360.0f);
	float orbitAngleJup = glm::radians((elapsedTime * orbitspeedJup) * 360.0f);
	float orbitAngleSat = glm::radians((elapsedTime * orbitspeedSat) * 360.0f);
	float orbitAngleUr = glm::radians((elapsedTime * orbitspeedUr) * 360.0f);
	float orbitAngleNep = glm::radians((elapsedTime * orbitspeedNep) * 360.0f);


	// Calculate positions of the planets
	trMercury->SetTranslation(glm::vec3(orbitRadiusMercury * glm::cos(orbitAngleMerc), 0.0f, orbitRadiusMercury * glm::sin(orbitAngleMerc)));
	trVenus->SetTranslation(glm::vec3(orbitRadiusVenus * glm::cos(orbitAngleVen), 0.0f, orbitRadiusVenus * glm::sin(orbitAngleVen)));
	trEarth->SetTranslation(glm::vec3(orbitRadiusEarth * glm::cos(orbitAngleEarth), 0.0f, orbitRadiusEarth * glm::sin(orbitAngleEarth)));
	trEarthMoon->SetTranslation(glm::vec3(orbitRadiusEarthMoon * glm::cos(orbitAngleEarthMoon), 0.0f, orbitRadiusEarthMoon * glm::sin(orbitAngleEarthMoon))*650.0f);
	trMars->SetTranslation(glm::vec3(orbitRadiusMars * glm::cos(orbitAngleMars), 0.0f, orbitRadiusMars * glm::sin(orbitAngleMars)));
	trPhobos->SetTranslation(glm::vec3(orbitRadiusEarthMoon * glm::cos(orbitAngleEarthMoon), 0.0f, orbitRadiusEarthMoon * glm::sin(orbitAngleEarthMoon)) * 650.0f);
	trDeimos->SetTranslation(glm::vec3(orbitRadiusEarthMoon * glm::cos(orbitAngleEarthMoon), 0.0f, orbitRadiusEarthMoon * (glm::sin(orbitAngleEarthMoon))) * -650.0f);
	trJupiter->SetTranslation(glm::vec3(orbitRadiusJupiter * glm::cos(orbitAngleJup), 0.0f, orbitRadiusJupiter * glm::sin(orbitAngleJup)));
	trSaturn->SetTranslation(glm::vec3(orbitRadiusSaturn * glm::cos(orbitAngleSat), 0.0f, orbitRadiusSaturn * glm::sin(orbitAngleSat)));
	trUranus->SetTranslation(glm::vec3(orbitRadiusUranus * glm::cos(orbitAngleUr), 0.0f, orbitRadiusUranus * glm::sin(orbitAngleUr)));
	trNeptune->SetTranslation(glm::vec3(orbitRadiusNeptune * glm::cos(orbitAngleNep), 0.0f, orbitRadiusNeptune * glm::sin(orbitAngleNep)));
	


	gRoot->Traverse();
}


GLuint CreateCube(float width, GLuint& VBO)
{
	//each side of the cube with its own vertices to use different normals
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); //the data comes from the currently bound GL_ARRAY_BUFFER
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	return VAO;
}

void DrawCube(GLuint id)
{
	const float width = 1.0f;
	glUseProgram(gShader.ID);
	glBindVertexArray(gVAO);

	// Set model matrix
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 scale = glm::scale(model, glm::vec3(width, width, width));
	gShader.setMat4("model", model);

	// Draw cube
	glDrawArrays(GL_TRIANGLES, 0, 36);
}
