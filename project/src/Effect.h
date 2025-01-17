#pragma once
#include "Texture.h"

using namespace dae;

class Effect
{
public:
    // Constructor & Destructor
    Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
    ~Effect();
    
    // Getters
    ID3DX11EffectTechnique* GetTechnique() const;
    //ID3D11InputLayout* GetInputLayout() const;

	void SetMatrix(const Matrix& world) const;
	void SetDiffuseMap(Texture* texture) const;

    void ToggleTechnique();
    
private:
    ID3DX11Effect* m_pEffect;
    ID3DX11EffectTechnique* m_pTechnique{};
    ID3D11InputLayout* m_pInputLayout{};

    ID3DX11EffectMatrixVariable* m_pWorldMatrixVariable{};
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable{};

	int m_TechniqueIdx{ 0 };

    // private functions 
    ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
};