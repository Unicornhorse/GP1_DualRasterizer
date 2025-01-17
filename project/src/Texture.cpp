#include "Texture.h"
#include <SDL_image.h>

dae::Texture::Texture(SDL_Surface* pSurface, ID3D11Device* pDevice):
	m_pSurface{ pSurface }, 
	m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
{
	Load(pSurface, pDevice);
}

dae::Texture::~Texture()
{
	if (m_pResource)
	{
		m_pResource->Release();
		m_pResource = nullptr;
	}
	if (m_pSRV)
	{
		m_pSRV->Release();
		m_pSRV = nullptr;
	}
}

dae::Texture* dae::Texture::LoadFromFile(const std::string& path, ID3D11Device* pDevice)
{
	return new Texture{ IMG_Load(path.c_str()), pDevice };
}


void dae::Texture::Load(const SDL_Surface* pSurface, ID3D11Device* pDevice)
{
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = pSurface->w;
	desc.Height = pSurface->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;	
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = pSurface->pixels;
	initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
	initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

	HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pSRV);
}