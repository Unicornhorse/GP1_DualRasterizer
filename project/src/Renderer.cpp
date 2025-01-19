#include "pch.h"
#include "Renderer.h"
#include "Utils.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		// Get aspect ratio
		m_AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);

		// Camera 
		m_Camera.Initialize(45.f, { .0f,.0f, -50.f });
		m_Camera.CalculateProjectionMatrix(m_AspectRatio);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}		

		// Mesh
		Utils::ParseOBJ("resources/vehicle.obj", m_Vertices, m_Indices);
		m_pMesh = new Mesh(m_pDevice, m_Indices, m_Vertices);

		// Textures
		//m_pTexture = Texture::LoadFromFile("resources/uv_grid_2.png", m_pDevice);
		m_pTexture = Texture::LoadFromFile("resources/vehicle_diffuse.png", m_pDevice);
		m_pMesh->SetDiffuseMap(m_pTexture);

		PrintControls();
	}

	Renderer::~Renderer()
	{
		if (m_pDeviceContext) {
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
			m_pDeviceContext = nullptr;
		}


		if (m_pDevice) {
			m_pDevice->Release();
			m_pDevice = nullptr;
		}

		if (m_pSwapChain) {
			m_pSwapChain->Release();
			m_pSwapChain = nullptr;
		}

		if (m_pDepthStencilBuffer) {
			m_pDepthStencilBuffer->Release();
			m_pDepthStencilBuffer = nullptr;
		}

		if (m_pDepthStencilView) {
			m_pDepthStencilView->Release();
			m_pDepthStencilView = nullptr;
		}

		if (m_pRenderTargetBuffer) {
			m_pRenderTargetBuffer->Release();
			m_pRenderTargetBuffer = nullptr;
		}

		if (m_pRenderTargetView) {
			m_pRenderTargetView->Release();
			m_pRenderTargetView = nullptr;
		}

		if (m_pDepthBufferPixels) {
			delete[] m_pDepthBufferPixels;
			m_pDepthBufferPixels = nullptr;
		}

		if (m_pMesh) {
			delete m_pMesh;
			m_pMesh = nullptr;
		}

		if (m_pTexture)
		{
			delete m_pTexture;
			m_pTexture = nullptr;
		}
	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_Camera.Update(pTimer);

		if (m_RotationEnabled)
		{
			m_Rotation = m_Rotationspeed * pTimer->GetElapsed();
			m_World *= Matrix::CreateRotationY(m_Rotation);
		}

		wvpMatrix = m_World * m_Camera.GetViewMatrix() * m_Camera.GetProjectionMatrix();

		m_pMesh->SetMatrix(wvpMatrix);
	}


	void Renderer::Render() const
	{
		if (!m_IsInitialized)
			return;

		if (m_Hardware)
		{
			RenderHardware();
		}
		else
		{
			RenderSoftware();
		}
	}

	HRESULT Renderer::InitializeDirectX()
	{
		// 1. Create Device and Devicecontext
		// ================================
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		HRESULT result = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			createDeviceFlags,
			&featureLevel,
			1,
			D3D11_SDK_VERSION,
			&m_pDevice,
			nullptr,
			&m_pDeviceContext
		);

		if (FAILED(result)) {
			return result;
		}

		// Create the DXGI factory
		IDXGIFactory1* pDXGIFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDXGIFactory));
		if (FAILED(result)) {
			return result;
		}

		// 2. Create Swap Chain
		// ================================
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};

		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;

		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;

		swapChainDesc.Windowed = TRUE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;
		
		// Get the handle (HWND) from SDL backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_GetVersion(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		// Create Swap Chain
		result = pDXGIFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result)) {
			return result;
		}

		// 3. Create DepthStencil (DS) & DepthStencilView (DSV)
		// Resource
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		// view
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result)) {
			return result;
		}

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result)) {
			return result;
		}

		// 4. Create RenderTarget (RT) & RenderTargetView (RTV)
		// ================================

		// Resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result)) {
			return result;
		}

		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result)) {
			return result;
		}

		// 5. Bind RTV & DSV to ouput merger stage
		// ================================
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		// 6. Set the viewport
		// ================================
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		return result;
	}

	// -------------------
	// Key input functions
	// -------------------
	void Renderer::ToggleRasterizerMode()
	{
		m_Hardware = !m_Hardware;

		if (m_Hardware)
		{
			std::cout << "\033[33m" << "**(SHARED) Hardware Rasterizer";
		}
		else
		{
			std::cout << "\033[33m" << "**(SHARED) Software Rasterizer";
		}
		std::cout << std::endl;
		std::cout << "\033[0m";
	}

	void Renderer::ToggleVehicleRotation()
	{
		m_RotationEnabled = !m_RotationEnabled;

		std::cout << "\033[33m" << "**(SHARED) Vehicle Rotation: ";

		if (m_RotationEnabled)
		{
			std::cout << "ON\n";
		}
		else
		{
			std::cout << "OFF\n";
		}
		std::cout << "\033[0m";
	}

	void Renderer::CycleCullMode()
	{
	}

	void Renderer::ToggleUniformClearColor()
	{
		m_UniformClearColor = !m_UniformClearColor;
		std::cout << "\033[33m" << "**(SHARED) Uniform ClearColor: ";
		if (m_UniformClearColor)
		{
			std::cout << "ON\n";
		}
		else
		{
			std::cout << "OFF\n";
		}
		std::cout << "\033[0m";
	}
	
	void Renderer::ToggleFireFX()
	{
		if (m_Hardware) {

			m_FireFX = !m_FireFX;

			std::cout << "\033[32m" << "**(HARDWARE) FireFX: ";

			if (m_FireFX)
			{
				std::cout << "ON\n";
			}
			else
			{
				std::cout << "OFF\n";
			}
			std::cout << "\033[0m";
		}
	}

	void Renderer::ToggleTechnique()
	{
		m_pMesh->ToggleTechnique();
	}

	void Renderer::CycleShadingMode()
	{
	}

	void Renderer::ToggleNormalMap()
	{
	}

	void Renderer::ToggleDepthBufferVisualisation()
	{
	}

	void Renderer::ToggleBoundingBoxVisualisation()
	{
	}

	void Renderer::RenderSoftware() const
	{
		//@START
		//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		//Clear BackBuffer
		if (m_UniformClearColor) {
			Uint32 clearColor = SDL_MapRGB(m_pBackBuffer->format, 30, 30, 30);
			SDL_FillRect(m_pBackBuffer, nullptr, clearColor);
		}
		else {
			Uint32 clearColor = SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100);
			SDL_FillRect(m_pBackBuffer, nullptr, clearColor);
		}

		std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

		VertexTransformationFunction(m_pMesh);
		
		//std::vector<Vector2> vertices_ScreenSpace{};
		//
		//auto& vertices_out{ mesh->GetVerticesOut() };
		//
		//for (const auto& vertex : vertices_out)
		//{
		//	vertices_ScreenSpace.push_back(
		//		{
		//			(vertex.position.x + 1) / 2.0f * m_Width,
		//			(1.0f - vertex.position.y) / 2.0f * m_Height
		//		});
		//}
		//
		//Triangle triangle{};
		//
		//auto& indices{ mesh->GetIndices() };
		//switch (mesh->GetTopology())
		//{
		//case PrimitiveTopology::TriangeList:
		//	for (int i{}; i < indices.size(); i += 3)
		//	{
		//		if (CalculateTriangle(triangle, mesh, vertices_ScreenSpace, i))
		//		{
		//			RenderTriangle(triangle);
		//		}
		//	}
		//	break;
		//case PrimitiveTopology::TriangleStrip:
		//	for (int i{}; i < indices.size() - 2; i++)
		//	{
		//		if (CalculateTriangle(triangle, mesh, vertices_ScreenSpace, i, (i % 2) == 1))
		//		{
		//			RenderTriangle(triangle);
		//		}
		//	}
		//	break;
		//default:
		//	break;
		//}
		//
		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::VertexTransformationFunction(Mesh* mesh) const
	{
		auto& vertices{ m_Vertices };
		auto& vertices_out{ mesh->GetVerticesOut() };

		vertices_out.clear();
		vertices_out.reserve(vertices.size());

		Matrix worldprojectionMatrix{ m_World * m_Camera.GetViewMatrix() * m_Camera.GetProjectionMatrix()};

		for (auto& vertex : vertices)
		{
			// Tranform the vertex using the inversed view matrix
			Vertex_Out outVertex{ 
				worldprojectionMatrix.TransformPoint({vertex.position, 1.f}),
				vertex.color,
				vertex.uv,
				m_World.TransformVector(vertex.normal).Normalized(),
				m_World.TransformVector(vertex.tangent).Normalized(),
				worldprojectionMatrix.TransformVector(vertex.viewDirection).Normalized()
			};

			outVertex.viewDirection = Vector3{ outVertex.position.x, outVertex.position.y, outVertex.position.z };
			outVertex.viewDirection.Normalize();

			outVertex.position.x /= outVertex.position.w;
			outVertex.position.y /= outVertex.position.w;
			outVertex.position.z /= outVertex.position.w;

			// Add the new vertex to the list of NDC vertices
			vertices_out.emplace_back(outVertex);
		}
	}

	void Renderer::RenderHardware() const
	{
		// 1. Clear RTV & DSV
		ColorRGB color;
		if (m_UniformClearColor) {
			color = ColorRGB{ .39f, .59f, .93f };
		}
		else {
			color = ColorRGB{ 0.14f, 0.14f, 0.14f };
		}
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &color.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		// 2. SET pipeline + invoke draw call (= render)
		m_pMesh->Render(m_pDeviceContext);


		// 3. Present backbuffer (swap)
		m_pSwapChain->Present(0, 0);
	}

	void Renderer::PrintControls() const
	{
		std::cout << "\033[33m"; // Set color to Yellow
		std::cout << "[Key Bindings - SHARED] \n";
		std::cout << "   [F1]  Toggle Rasterizer Mode (HARDWARE/SOFTWARE)\n"; // TODO
		std::cout << "   [F2]  Toggle Vehicle Rotation (ON/OFF)\n";
		std::cout << "   [F9]  Cycle CullMode (BACK/FRONT/NONE)\n"; // TODO
		std::cout << "   [F10]  Toggle Uniform ClearColor (ON/OFF)\n";
		std::cout << "   [F11]  Toggle Print FPS (ON/OFF)\n";
		std::cout << "\033[0m" << std::endl;

		std::cout << "\033[32m"; // Set color to Green
		std::cout << "[Key Bindings - HARDWARE] \n";
		std::cout << "   [F3]  Toggle FireFX (ON/OFF)\n"; // TODO
		std::cout << "   [F4]  Cycle Sampler State (ON/OFF)\n";
		std::cout << "\033[0m" << std::endl;

		std::cout << "\033[35m"; // Set color to Purple
		std::cout << "[Key Bindings - SHARED] \n";
		std::cout << "   [F5]  Cycle Shading Mode (COMBINED/OBSERVED_AREA/DIFFUSE/SPECULAR)\n"; // TODO
		std::cout << "   [F6]  Toggle NormalMap (ON/OFF)\n"; // TODO
		std::cout << "   [F7]  Toggle DepthBuffer Visualization (ON/OFF)\n"; // TODO
		std::cout << "   [F8]  Toggle BoundingBox Visualization (ON/OFF)\n"; // TODO
		std::cout << "\033[0m" << std::endl;
	}
}
