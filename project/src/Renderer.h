#pragma once
#include "Mesh.h"
#include "Camera.h"
#include "Matrix.h"
#include "Texture.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

		void ToggleTechnique();

	private:
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		//DIRECTX
		HRESULT InitializeDirectX();

		ID3D11Device* m_pDevice{};
		ID3D11DeviceContext* m_pDeviceContext{};

		IDXGISwapChain* m_pSwapChain{};
		ID3D11Texture2D* m_pDepthStencilBuffer{};
		ID3D11DepthStencilView* m_pDepthStencilView{};

		ID3D11Texture2D* m_pRenderTargetBuffer{};
		ID3D11RenderTargetView* m_pRenderTargetView{};

		Mesh* m_pMesh{};
		Camera m_Camera{};

		std::vector<Vertex_PosCol> vertices{
			//{{.0f, 3.f, 2.f}, {1.f, 0.f, 0.f}},
			//{{3.f, -3.f, 2.f}, {0.f, 0.f, 1.f}},
			//{{-3.f, -3.f, 2.f}, {0.f, 1.f, 0.f}}

			//{ { -3.f,	 3.f,	-2.f }, {colors::White}, {	0,	0	} },
			//{ { 0.f,	 3.f,	-2.f }, {colors::White}, { .5,	0	} },
			//{ { 3.f,	 3.f,	-2.f }, {colors::White}, {	1,	0	} },
			//{ { -3.f,	 0.f,	-2.f }, {colors::White}, {	0, .5	} },
			//{ { 0.f,	 0.f,	-2.f }, {colors::White}, { .5, .5	} },
			//{ { 3.f,	 0.f,	-2.f }, {colors::White}, {	1, .5	} },
			//{ { -3.f,	-3.f,	-2.f }, {colors::White}, {	0,	1	} },
			//{ { 0.f,	-3.f,	-2.f }, {colors::White}, { .5,	1	} },
			//{ { 3.f,	-3.f,	-2.f }, {colors::White}, {	1,	1	} }

			//{ { -3.f,  3.f, -2.f }, { colors::White }, { 0.f, 0.f } },
			//{ {  3.f,  3.f, -2.f }, { colors::White }, { 1.f, 0.f } },
			//{ { -3.f, -3.f, -2.f }, { colors::White }, { 0.f, 1.f } },
			//{ {  3.f, -3.f, -2.f }, { colors::White }, { 1.f, 1.f } },
		};

		std::vector<uint32_t> indices{ 
			//0, 1, 2 
		
			//3, 0, 1,	1, 4, 3,	4, 1, 2,
			//2, 5, 4,	6, 3, 4,	4, 7, 6,
			//7, 4, 5,	5, 8, 7

			//0, 1, 2,
			//2, 1, 3
		};

		float m_Rotationspeed{ 0.9 };
		float m_Rotation{};

		Texture* m_pTexture{};
		float m_AspectRatio;
		
	};
}
