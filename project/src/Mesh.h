#pragma once
#include "Vector3.h"
#include "Vector4.h"
#include "vector"

#include "Matrix.h"
#include "Effect.h"


using namespace dae;

struct Vertex_PosCol
{
	Vector3 position{};
	ColorRGB color{ colors::White };
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
	Vector3 viewDirection{};
};

struct Vertex_Out
{
	Vector4 position{};
	ColorRGB color{ colors::White };
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
	Vector3 viewDirection{};
};

enum class PrimitiveTopology
{
	TriangleList,
	TriangleStrip
};

class Mesh
{
public:
    Mesh(ID3D11Device* pDevice, std::vector<uint32_t> indices, std::vector<Vertex_PosCol> vertices);
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh& operator=(Mesh&&) noexcept = delete;

	void Render(ID3D11DeviceContext* pDeviceContext) const;
	void SetMatrix(const Matrix& wvpMatrix) const;
    void SetDiffuseMap(Texture* texture) const;

    void ToggleTechnique();
	
	std::vector<uint32_t>& GetIndices() { return m_Indices; }
	std::vector<Vertex_Out>& GetVerticesOut() { return m_Vertices_out; }
	std::vector<Vertex_PosCol>& GetVertices() { return m_Vertices; }

	PrimitiveTopology GetTopology() { return m_PrimitiveTopology; }

private:
	void CreateLayoutAndBuffers(ID3D11Device* pDevice, const std::vector<Vertex_PosCol>& vertices, const std::vector<uint32_t>& indices);

	void CreateVertexLayout(ID3D11Device* pDevice);
	void CreateVertexBuffer(ID3D11Device* pDevice, const std::vector<Vertex_PosCol>& vertices, const std::vector<uint32_t>& indices);
    
    // DirectX resources
    ID3D11Buffer* m_pVertexBuffer{ nullptr };
    ID3D11Buffer* m_pIndexBuffer{ nullptr };
    ID3D11InputLayout* m_pInputLayout{ nullptr };

    // Effect and technique
    Effect* m_pEffect{ nullptr };
    ID3DX11EffectTechnique* m_pTechnique{ nullptr };

    // Buffer sizes
    uint32_t m_NumIndices{ 0 };

	std::vector<uint32_t> m_Indices{};
	std::vector<Vertex_PosCol> m_Vertices{};
	std::vector<Vertex_Out> m_Vertices_out{};

	PrimitiveTopology m_PrimitiveTopology{ PrimitiveTopology::TriangleList };
};
