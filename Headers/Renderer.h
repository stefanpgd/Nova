#pragma once
#include <string>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>

class Window;
class DXDescriptorHeap;

using namespace Microsoft::WRL;
using namespace DirectX;

class Renderer
{
public:
	Renderer(std::wstring windowName);
	~Renderer();

	void Render();
	void Resize();

private:
	void EnableDebugLayer();

	void LoadContent();
	void ClearRTV();
	void ClearDepth();
	void ResizeDepthBuffer();

private:
	Window* window;
	DXDescriptorHeap* DSVHeap;

	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	ComPtr<ID3D12Resource> depthBuffer;

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;

	float FOV;

	DirectX::XMMATRIX model;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;

	unsigned int startWindowWidth = 1280;
	unsigned int startWindowHeight = 720;

	unsigned int frameCount = 0;
};