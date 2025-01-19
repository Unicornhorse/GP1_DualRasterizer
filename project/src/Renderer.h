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

		// Toggle Both
		void ToggleRasterizerMode();
		void ToggleVehicleRotation();
		void CycleCullMode();
		void ToggleUniformClearColor();

		// Toggle Hardware
		void ToggleFireFX();
		void ToggleTechnique();

		// Toggle Software
		void CycleShadingMode();
		void ToggleNormalMap();
		void ToggleDepthBufferVisualisation();
		void ToggleBoundingBoxVisualisation();

	private:
		// Window Variables
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };
	
		// Software Variables
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		float* m_pDepthBufferPixels{};

		// Hardware variables
		HRESULT InitializeDirectX();

		ID3D11Device* m_pDevice{};
		ID3D11DeviceContext* m_pDeviceContext{};

		IDXGISwapChain* m_pSwapChain{};
		ID3D11Texture2D* m_pDepthStencilBuffer{};
		ID3D11DepthStencilView* m_pDepthStencilView{};

		ID3D11Texture2D* m_pRenderTargetBuffer{};
		ID3D11RenderTargetView* m_pRenderTargetView{};

		// Standard Variables
		Mesh* m_pMesh{};
		Camera m_Camera{};

		std::vector<Vertex_PosCol> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		float m_Rotationspeed{ 0.9f };
		float m_Rotation{};

		Matrix m_World{};
		Matrix wvpMatrix{};

		Texture* m_pTexture{};
		float m_AspectRatio;
		
		// render modes
		void RenderSoftware() const;
		void VertexTransformationFunction(Mesh* mesh) const;
		void RenderSoftwareMesh(Mesh* mesh) const;
		float Remap(float value, float low1, float high1, float low2, float high2) const;
		void RenderHardware() const;

		// Toggles and cycles
		void PrintControls() const;
		void DrawBoundingBox(int minX, int minY, int maxX, int maxY, uint32_t* framebuffer, int width, int height, uint32_t color) const;

		bool m_Hardware{ true };
		bool m_RotationEnabled{ true };
		bool m_UniformClearColor{ true };

		bool m_FireFX{ false };

		bool m_DisplayDepthBuffer{ false };
		bool m_DisplayBoundingBox{ false };
	};
}
