#include "Renderer.h"
#include "Utilities.h"
#include "DXCommandQueue.h"

#include <cassert>
#include <chrono>

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[3] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },  // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },  // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },   // 2
};

static short g_Indicies[3] =
{
	0, 1, 2
};

Renderer::Renderer(std::wstring windowName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	EnableDebugLayer();

	CreateDevice();
	commandQueue = new DXCommandQueue(device);
	window = new Window(windowName, startWindowWidth, startWindowHeight, device, commandQueue->Get());

	scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)window->GetWindowWidth(), (float)window->GetWindowHeight());
	FOV = 60.0f;

	LoadContent();
}

Renderer::~Renderer()
{
	delete commandQueue;
}

void Renderer::Render()
{
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();

	commandQueue->ResetCommandList(backBufferIndex);
	ComPtr<ID3D12GraphicsCommandList2> commandList = commandQueue->GetCommandList();

	CD3DX12_RESOURCE_BARRIER renderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &renderBarrier);

	float clearColor[4] = { 0.2f, 0.45f, 0.31f, 1.0f };
	commandList->ClearRenderTargetView(window->GetCurrentBackBufferRTV(), clearColor, 0, nullptr);

	CD3DX12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &presentBarrier);

	commandQueue->ExecuteCommandList(backBufferIndex);
	window->Present();

	// calling 'GetCurrentBackBufferIndex' again, since we've presented and the
	// current back buffer has now changed.
	commandQueue->WaitForFenceValue(window->GetCurrentBackBufferIndex());
}

void Renderer::Resize()
{
	commandQueue->Flush();
	window->Resize();
}

void Renderer::EnableDebugLayer()
{
#if defined (_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

void Renderer::CreateDevice()
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	// Enables errors to be caught during device creation and while quering adapaters
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	SIZE_T maxDedicatedVideoMemory = 0;
	for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		// Is adapter not a software GPU? Can it succesfully create a Dx12 device?
		// Does it have more dedicated video memory than the other adapters? If so, store it as the most capable adapter
		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
				D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
			dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
		}
	}

	ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif
}

void Renderer::LoadContent()
{
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	commandQueue->ResetCommandList(backBufferIndex);

	// Upload vertex buffer //
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(&vertexBuffer, &intermediateVertexBuffer, _countof(g_Vertices), 
		sizeof(VertexPosColor), g_Vertices);

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(g_Vertices);
	vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	// Upload Index Buffer //
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(&indexBuffer, &intermediateIndexBuffer, _countof(g_Indicies),
		sizeof(short), g_Indicies);

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = sizeof(g_Indicies);

	// Create Depth-Stencil view heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&DSVHeap)));

	UINT compileFlags = 0;

#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\triangle.vs.hlsl", nullptr, nullptr, "main", "vs_5_1", compileFlags, 0, &vertexShaderBlob, nullptr));

	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DCompileFromFile(L"Shaders\\triangle.ps.hlsl", nullptr, nullptr, "main", "ps_5_1", compileFlags, 0, &pixelShaderBlob, nullptr));

	// 1.1 allows for driver optimization, hence why we should check if it's available or not
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// Serialize the root signature.
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	// Create the root signature.
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineDesc;

	pipelineDesc.pRootSignature = rootSignature.Get();
	pipelineDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineDesc.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineDesc
	};

	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)));

	commandQueue->ExecuteCommandList(backBufferIndex);
	commandQueue->WaitForFenceValue(backBufferIndex);
}

void Renderer::UpdateBufferResource(ID3D12Resource** destinationResource, ID3D12Resource** intermediateBuffer, size_t numberOfElements, size_t elementSize, const void* bufferData)
{
	if (!bufferData)
	{
		assert(false && "Buffer that is trying to be uploaded to GPU memory is NULL");
	}

	// Size of the entire buffer in bytes & properties
	size_t bufferSize = elementSize * numberOfElements;
	CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	CD3DX12_HEAP_PROPERTIES destinationHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_HEAP_PROPERTIES intermediateHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Create a commited resource for the GPU resource
	// This is used to create a resource and an implicit heap that is large enough to store the given resource. This `destinationResource` is also mapped to the implicit heap
	ThrowIfFailed(device->CreateCommittedResource(&destinationHeap, D3D12_HEAP_FLAG_NONE, &buffer,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(destinationResource)));

	// Though the heap is created, nothing has been uploaded yet
	// We create another resource, this time on the GPU's Upload Heap.
	// This heap is optimized to receive a CPU to GPU write-once only call
	// Then once it's on the GPU, we can transfer the data from the Upload Heap to the Default heap
	ThrowIfFailed(device->CreateCommittedResource(&intermediateHeap,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(intermediateBuffer)));

	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = bufferData;
	subresourceData.RowPitch = bufferSize;
	subresourceData.SlicePitch = bufferSize; // Double check if 1 works here as well?

	// Using the info from the subresource data struct, the data will now be copied over to the Upload heap and then the Default Heap to the destination resource.
	UpdateSubresources(commandQueue->GetCommandList().Get(), *destinationResource, *intermediateBuffer, 0, 0, 1, &subresourceData);
}