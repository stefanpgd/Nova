#include "Renderer.h"
#include "DXUtilities.h"
#include "DXDevice.h"
#include "DXCommands.h"
#include "DXAccess.h"

#include <cassert>
#include <chrono>

using namespace RendererInternal;

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static short g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

Renderer::Renderer(std::wstring windowName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	EnableDebugLayer();

	device = new DXDevice();
	commands = new DXCommands();
	window = new Window(windowName, startWindowWidth, startWindowHeight, device, commandQueue->Get());

	FOV = 60.0f; // Move to a camera class //

	LoadContent();
}

Renderer::~Renderer()
{
	commands->Flush(window->GetCurrentBackBufferIndex());
	delete commands;
}

void Renderer::Render()
{
	float angle = static_cast<float>(frameCount * 4.15f);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	model = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle)) * XMMatrixTranslation(cos(frameCount * 0.015) * 8.0f, sin(frameCount * 0.005) * 5.0f, 0.0f);

	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	view = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	float aspectRatio = (float)window->GetWindowWidth() / static_cast<float>(window->GetWindowHeight());
	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOV), aspectRatio, 0.1f, 1000.0f);

	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();

	commands->ResetCommandList(backBufferIndex);
	ComPtr<ID3D12GraphicsCommandList2> commandList = commands->GetList();

	auto RTV = window->GetCurrentBackBufferRTV();
	auto DSV = DSVHeap->GetCPUDescriptorHandleForHeapStart();

	CD3DX12_RESOURCE_BARRIER renderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &renderBarrier);

	float clearColor[4] = { 0.2f, 0.45f, 0.31f, 1.0f };
	commandList->ClearRenderTargetView(RTV, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(DSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->SetPipelineState(pipelineState.Get());
	// Even though the Root Signature was used to create the PSO, we have to explicitly bind it to the command list as well
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	// Input-Assembler Settings //
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);

	// Rasterizer Settings //
	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());

	// Output-Merger settings //
	commandList->OMSetRenderTargets(1, &RTV, FALSE, &DSV);

	// Update MVP //
	XMMATRIX mvp = XMMatrixMultiply(model, view);
	mvp = XMMatrixMultiply(mvp, projection);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp, 0);

	commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	CD3DX12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &presentBarrier);

	commands->ExecuteCommandList(backBufferIndex);
	window->Present();

	// calling 'GetCurrentBackBufferIndex' again, since we've presented and the
	// current back buffer has now changed.
	commands->WaitForFenceValue(window->GetCurrentBackBufferIndex());

	frameCount++;
}

void Renderer::Resize()
{
	commands->Flush(window->GetCurrentBackBufferIndex());
	window->Resize();
	ResizeDepthBuffer();
}

void Renderer::EnableDebugLayer()
{
#if defined (_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

void Renderer::LoadContent()
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	commands->ResetCommandList(backBufferIndex);

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

	commands->ExecuteCommandList(backBufferIndex);
	commands->WaitForFenceValue(backBufferIndex);

	ResizeDepthBuffer();
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
	UpdateSubresources(commands->GetCommandList().Get(), *destinationResource, *intermediateBuffer, 0, 0, 1, &subresourceData);
}

void Renderer::ResizeDepthBuffer()
{
	commandQueue->Flush(window->GetCurrentBackBufferIndex());

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);;
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, window->GetWindowWidth(), window->GetWindowHeight(),
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearValue, IID_PPV_ARGS(&depthBuffer)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &dsv,
		DSVHeap->GetCPUDescriptorHandleForHeapStart());
}

DXDevice* DXAccess::GetDevice()
{
	if (RendererInternal::device)
	{
		return RendererInternal::device;
	}

	assert(false && "DXDevice hasn't been initialized");
}

DXCommands* DXAccess::GetCommands()
{
	if (RendererInternal::commands)
	{
		return RendererInternal::commands;
	}

	assert(false && "DXCommands hasn't been initialized");
}