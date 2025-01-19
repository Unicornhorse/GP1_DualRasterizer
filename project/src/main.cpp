#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"

using namespace dae;

void TogglePrintFPS(bool& printFPS);

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"DirectX - Roy Cohen - 2DAE20",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;

	bool PrintFPS = false;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				switch (e.key.keysym.scancode)
				{
				case SDL_SCANCODE_F1:
					// Toggle Rasterizer Mode				(SHARED)
					pRenderer->ToggleRasterizerMode();
					break;
				case SDL_SCANCODE_F2:
					// Toggle Vehicle Rotation				(SHARED)
					pRenderer->ToggleVehicleRotation();
					break;
				case SDL_SCANCODE_F3:
					// Toggle FireFX						(HARDWARE)		
					pRenderer->ToggleFireFX();
					break;
				case SDL_SCANCODE_F4:
					// Cycle Sampler State					(HARDWARE)	
					pRenderer->ToggleTechnique();
					break;
				case SDL_SCANCODE_F5:
					// Cycle Shading Mode 					(SOFTWARE)
					pRenderer->CycleShadingMode();
					break;
				case SDL_SCANCODE_F6:
					// Toggle NormalMap						(SOFTWARE)
					pRenderer->ToggleNormalMap();
					break;
				case SDL_SCANCODE_F7:
					// Toggle DepthBuffer Visualization		(SOFTWARE)
					pRenderer->ToggleDepthBufferVisualisation();
					break;
				case SDL_SCANCODE_F8:
					// Toggle BoundingBox Visualization		(SOFTWARE)
					pRenderer->ToggleBoundingBoxVisualisation();
					break;
				case SDL_SCANCODE_F9:
					// Cycle Cull modes						(SHARED)
					pRenderer->CycleCullMode();
					break;
				case SDL_SCANCODE_F10:
					// Toggle Uniform ClearColor			(SHARED)
					pRenderer->ToggleUniformClearColor();
					break;
				case SDL_SCANCODE_F11:
					// Toggle Print FPS						(SHARED)
					TogglePrintFPS(PrintFPS);
					break;
				default:
					break;
				}
			default:;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f && PrintFPS)
		{
			printTimer = 0.f;
			std::cout << "\033[90m" << "dFPS: " << pTimer->GetdFPS() << std::endl;
			std::cout << "\033[0m";
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}

void TogglePrintFPS(bool& printFPS)
{
	printFPS = !printFPS;

	std::cout << "\033[33m" << "**(SHARED) Print FPS: ";

	if (printFPS)
	{
		std::cout << "ON \n";
	}
	else
	{
		std::cout << "OFF \n";
	}
	std::cout << "\033[0m";
}