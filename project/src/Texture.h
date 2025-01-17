#pragma once
#include <SDL_surface.h>
#include <d3d11.h>
#include <string>
//#include "ColorRGB.h"

namespace dae {
	struct Vector2;

	class Texture
	{
    public:
        ~Texture();

        static Texture* LoadFromFile(const std::string& path, ID3D11Device* pDevice);

        ID3D11ShaderResourceView* GetShaderResourceView() const { return m_pSRV; }

    private:
        SDL_Surface* m_pSurface{};
        uint32_t* m_pSurfacePixels{};
        
        ID3D11Texture2D* m_pResource; 
        ID3D11ShaderResourceView* m_pSRV;

        // private functions
        Texture(SDL_Surface* pSurface, ID3D11Device* pDevice);
        void Load(const SDL_Surface* pSurface, ID3D11Device* pDevice);
	};
};
