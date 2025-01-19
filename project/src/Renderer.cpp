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
		if (!m_Hardware) {
			m_DisplayDepthBuffer = !m_DisplayDepthBuffer;

			std::cout << "\033[35m" << "**(SOFTWARE) Depth Buffer Visualisation: ";

			if (m_DisplayDepthBuffer)
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

	void Renderer::ToggleBoundingBoxVisualisation()
	{
		if (!m_Hardware) {
			m_DisplayBoundingBox = !m_DisplayBoundingBox;
			std::cout << "\033[35m" << "**(SOFTWARE) Bounding Box Visualisation: ";
			if (m_DisplayBoundingBox)
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

	void Renderer::DrawBoundingBox(int minX, int minY, int maxX, int maxY, uint32_t* framebuffer, int width, int height, uint32_t color) const
	{
		// Top
		for (int x = minX; x < maxX; ++x)
		{
			framebuffer[x + minY * width] = color;
		}
		// Bottom
		for (int x = minX; x < maxX; ++x)
		{
			framebuffer[x + maxY * width] = color;
		}
		// Left
		for (int y = minY; y < maxY; ++y)
		{
			framebuffer[minX + y * width] = color;
		}
		// Right
		for (int y = minY; y < maxY; ++y)
		{
			framebuffer[maxX + y * width] = color;
		}
	}

	void Renderer::RenderSoftware() const
	{
		//@START
		//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		//Clear BackBuffer
		if (m_UniformClearColor) {
			Uint32 clearColor = SDL_MapRGB(m_pBackBuffer->format, 39, 39, 39);
			SDL_FillRect(m_pBackBuffer, nullptr, clearColor);
		}
		else {
			Uint32 clearColor = SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100);
			SDL_FillRect(m_pBackBuffer, nullptr, clearColor);
		}

		// Set world space coordinates to NDC
		VertexTransformationFunction(m_pMesh);

		// Render Mesh
		RenderSoftwareMesh(m_pMesh);
		
		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::VertexTransformationFunction(Mesh* mesh) const
	{
		std::vector<Vertex_PosCol>& vertices{ mesh->GetVertices() };
		std::vector<Vertex_Out>&	vertices_out{ mesh->GetVerticesOut() };

		vertices_out.clear();
		vertices_out.reserve(vertices.size());

		for (auto& vertex : vertices)
		{
			Vertex_Out outVertex{ 
				wvpMatrix.TransformPoint({vertex.position, 1.f}),
				vertex.color,
				vertex.uv,
				m_World.TransformVector(vertex.normal),
				m_World.TransformVector(vertex.tangent),
				m_World.TransformVector(vertex.viewDirection)
			};

			// World Space
			Vector4 worldSpace{
				vertex.position.x,
				vertex.position.y,
				vertex.position.z,
				1.f
			};

			// View Space
			Vector4 viewSpace{ m_Camera.GetViewMatrix().TransformPoint(m_World.TransformPoint(worldSpace))};

			// Projection Space
			Vector4 projected{ m_Camera.GetProjectionMatrix().TransformPoint(viewSpace)};

			// Perspective Division
			outVertex.position.x /= projected.w;
			outVertex.position.y /= projected.w;
			outVertex.position.z /= projected.w;

			// NDC Space
			vertices_out.emplace_back(outVertex);
		}
	}

	void Renderer::RenderSoftwareMesh(Mesh* mesh) const
	{
		std::vector<uint32_t>&		indices{ mesh->GetIndices() };
		std::vector<Vertex_Out>&	vertices_NDC{ mesh->GetVerticesOut() };

		PrimitiveTopology topology{ mesh->GetTopology() };

		for (size_t i = 0; i < m_Width * m_Height; ++i) {
			m_pDepthBufferPixels[i] = std::numeric_limits<float>::max();
		}

		// Rasterization
		uint16_t size = indices.size() - (topology == PrimitiveTopology::TriangleList ? 0 : 2);

		for (size_t i = 0; i < size; i += (topology == PrimitiveTopology::TriangleList ? 3 : 1))
		{
			// Vertices
			Vector2 v0{ vertices_NDC[indices[i]].position.x,	 vertices_NDC[indices[i]].position.y };
			Vector2 v1{};
			Vector2 v2{};

			// z positions
			float z0 = vertices_NDC[indices[i]].position.z;
			float z1{};
			float z2{};

			// w positions
			float zw0 = vertices_NDC[indices[i]].position.w;
			float zw1{};
			float zw2{};

			if (topology == PrimitiveTopology::TriangleStrip && i % 2 != 0) {
				v1 = { vertices_NDC[indices[i + 2]].position.x, vertices_NDC[indices[i + 2]].position.y };
				v2 = { vertices_NDC[indices[i + 1]].position.x, vertices_NDC[indices[i + 1]].position.y };

				z1 = vertices_NDC[indices[i + 2]].position.z;
				z2 = vertices_NDC[indices[i + 1]].position.z;

				zw1 = vertices_NDC[indices[i + 2]].position.w;
				zw2 = vertices_NDC[indices[i + 1]].position.w;
			}
			else {
				v1 = { vertices_NDC[indices[i + 1]].position.x, vertices_NDC[indices[i + 1]].position.y };
				v2 = { vertices_NDC[indices[i + 2]].position.x, vertices_NDC[indices[i + 2]].position.y };

				z1 = vertices_NDC[indices[i + 1]].position.z;
				z2 = vertices_NDC[indices[i + 2]].position.z;

				zw1 = vertices_NDC[indices[i + 1]].position.w;
				zw2 = vertices_NDC[indices[i + 2]].position.w;
			}

			// NDC Coordinates
			Vector2 A{ ((v0.x + 1) / 2) * m_Width, ((1 - v0.y) / 2) * m_Height };
			Vector2 B{ ((v1.x + 1) / 2) * m_Width, ((1 - v1.y) / 2) * m_Height };
			Vector2 C{ ((v2.x + 1) / 2) * m_Width, ((1 - v2.y) / 2) * m_Height };

			// Edges
			Vector2 edge0 = B - A;
			Vector2 edge1 = C - B;
			Vector2 edge2 = A - C;

			// Bounding box
			int minX = std::max(0, static_cast<int>(std::floor(std::min({ A.x, B.x, C.x }))));
			int minY = std::max(0, static_cast<int>(std::floor(std::min({ A.y, B.y, C.y }))));

			int maxX = std::min(m_Width - 1, static_cast<int>(std::ceil(std::max({ A.x, B.x, C.x }))));
			int maxY = std::min(m_Height - 1, static_cast<int>(std::ceil(std::max({ A.y, B.y, C.y }))));

			if (m_DisplayBoundingBox)
			{
				Uint32 boundingColor = SDL_MapRGB(m_pBackBuffer->format, 100, 000, 000);
				DrawBoundingBox(minX, minY, maxX, maxY, m_pBackBufferPixels, m_Width, m_Height, boundingColor);
			}

			for (int px{ minX }; px < maxX; ++px)
			{
				for (int py{ minY }; py < maxY; ++py)
				{
					ColorRGB finalColor{ colors::Black };
					Vector2 P{ px + 0.5f, py + 0.5f };

					// Direction from NDC to P(ixel Point)
					Vector2 AP = P - A;
					Vector2 BP = P - B;
					Vector2 CP = P - C;

					// Barycentric weights
					float w0 = (Vector2::Cross(edge1, BP));
					float w1 = (Vector2::Cross(edge2, CP));
					float w2 = (Vector2::Cross(edge0, AP));

					// Total Triangle Area 
					float total = w0 + w1 + w2;

					w0 /= total;
					w1 /= total;
					w2 /= total;

					// Check if point is inside the triangle
					if (w0 >= 0 && w1 >= 0 && w2 >= 0)
					{
						// zBuffer
						float zBufferValue = 1 / ((w0 / z0) +
							(w1 / z1) +
							(w2 / z2));

						// Interpolated Depth -> using correct depth interpolation with w value
						float interpolatedDepth = 1 / ((w0 / zw0) +
							(w1 / zw1) +
							(w2 / zw2));


						int pixelIndex = py * m_Width + px;

						// Depth check
						if (zBufferValue > 0 && zBufferValue < 1) {
							if (zBufferValue < m_pDepthBufferPixels[pixelIndex])
							{
								if (!m_DisplayDepthBuffer) {
									// Texture
									Vector2 textureColor{};
									if (topology == PrimitiveTopology::TriangleStrip && i % 2 != 0) {
										textureColor = (((vertices_NDC[indices[i]].uv / zw0) * w0) +
											((vertices_NDC[indices[i + 2]].uv / zw1) * w1) +
											((vertices_NDC[indices[i + 1]].uv / zw2) * w2)) * interpolatedDepth;
									}
									else {
										textureColor = (((vertices_NDC[indices[i]].uv / zw0) * w0) +
											((vertices_NDC[indices[i + 1]].uv / zw1) * w1) +
											((vertices_NDC[indices[i + 2]].uv / zw2) * w2)) * interpolatedDepth;
									}

									finalColor += m_pTexture->Sample(textureColor);
								}
								else {
									float depth = Remap(zBufferValue, 0.985f, 1.f, 0.f, 1.f);
									ColorRGB remappedDepth{ depth, depth, depth };
									finalColor += remappedDepth;
								}

								// Depth write
								m_pDepthBufferPixels[pixelIndex] = zBufferValue;

								//Update Color in Buffer
								finalColor.MaxToOne();

								m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
									static_cast<uint8_t>(finalColor.r * 255),
									static_cast<uint8_t>(finalColor.g * 255),
									static_cast<uint8_t>(finalColor.b * 255));
							}
						}
					}
				}
			}
		}
	}

	float Renderer::Remap(float value, float low1, float high1, float low2, float high2) const {
		return (value - low1) / (high1 - low1) * (high2 - low2) + low2;
	}

	void Renderer::RenderHardware() const
	{
		// 1. Clear RTV & DSV
		ColorRGB color;
		if (m_UniformClearColor) {
			color = ColorRGB{ .39f, .59f, .93f };
		}
		else {
			color = ColorRGB{ 0.1f, 0.1f, 0.1f };
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
		std::cout << "   [F7]  Toggle DepthBuffer Visualization (ON/OFF)\n";
		std::cout << "   [F8]  Toggle BoundingBox Visualization (ON/OFF)\n";
		std::cout << "\033[0m" << std::endl;
	}
}
