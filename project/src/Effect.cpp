#pragma once
#include "pch.h"
#include "Effect.h"
#include "iostream"

Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile) :
	m_pEffect{ LoadEffect(pDevice, assetFile) },
	m_pTechnique{ nullptr },
	m_pWorldMatrixVariable{ nullptr }
{
	//m_pTechnique = m_pEffect->GetTechniqueByIndex(0);
    m_pTechnique = m_pEffect->GetTechniqueByName("PointTechnique");
	if (!m_pTechnique) {
		std::wcout << L"Technique not valid\n";
	}

	m_pWorldMatrixVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (!m_pWorldMatrixVariable->IsValid())
	{
		std::wcout << L"gWorldViewProj variable not valid!\n";
	}

	m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	if (!m_pDiffuseMapVariable->IsValid())
	{
		std::wcout << L"gDiffuseMap variable not valid!\n";
	}
}

Effect::~Effect()
{
	

	if (m_pEffect) {
		m_pEffect->Release();
		m_pEffect = nullptr;
	}

	if (m_pTechnique) {

		m_pTechnique = nullptr;
	}

	if (m_pWorldMatrixVariable)
	{
		m_pWorldMatrixVariable = nullptr;
	}
	//if (m_pInputLayout) {
	//	m_pInputLayout->Release();
	//	m_pInputLayout = nullptr;
	//}

	if (m_pDiffuseMapVariable)
	{
		m_pDiffuseMapVariable = nullptr;
	}
}

ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{

	HRESULT result;
	ID3D10Blob* pErrorBlob{nullptr};
	ID3DX11Effect* pEffect;

	DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	result = D3DX11CompileEffectFromFile(assetFile.c_str(), 
		nullptr, 
		nullptr, 
		shaderFlags, 
		0, 
		pDevice, 
		&pEffect, 
		&pErrorBlob);


	if (FAILED(result))
	{
		if (pErrorBlob != nullptr)
		{

			const char* pErrors = static_cast<char*>(pErrorBlob->GetBufferPointer());

			std::wstringstream ss;
			for (unsigned int i = 0; i < pErrorBlob->GetBufferSize(); i++)
				ss << pErrors[i];

			OutputDebugStringW(ss.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << ss.str() << std::endl;
		}
		else
		{
			std::wstringstream ss;
			ss << "EffectLoader: Failed to create effect from file: " << assetFile;
			std::wcout << ss.str() << std::endl;
			return nullptr;
		}
	}
	return pEffect;
}

ID3DX11EffectTechnique* Effect::GetTechnique() const
{
	return m_pTechnique;
}

void Effect::SetMatrix(const Matrix& wvpMatrix) const
{
	m_pWorldMatrixVariable->SetMatrix(reinterpret_cast<const float*>(&wvpMatrix));
}

void Effect::SetDiffuseMap(Texture* texture) const
{
	if (texture)
	{
		m_pDiffuseMapVariable->SetResource(texture->GetShaderResourceView());
	}
}

void Effect::ToggleTechnique()
{
	// 3 techniques in the effect file
	// ---------------------------------
	// 0 -> PointTechnique
	// 1 -> LinearTechnique
	// 2 -> AnisotropicTechnique

	std::string techniqueName;
	m_TechniqueIdx = (m_TechniqueIdx + 1) % 3;
	if (m_TechniqueIdx == 0) {
		techniqueName = "PointTechnique";
	}
	else if (m_TechniqueIdx == 1) {
		techniqueName = "LinearTechnique";
	}
	else {
		techniqueName = "AnisotropicTechnique";
	}
	std::cout << "Technique: " << techniqueName << std::endl;
	m_pTechnique = m_pEffect->GetTechniqueByIndex(m_TechniqueIdx);
	if (!m_pTechnique) {
		std::wcout << L"Technique not valid\n";
	}
}

//D3D11InputLayout* Effect::GetInputLayout() const
//
//	return m_pInputLayout;
//


