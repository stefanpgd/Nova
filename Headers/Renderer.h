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
class DXPipeline;

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
	void LoadContent();
	void ResizeDepthBuffer();

private:
	DXPipeline* pipeline;

	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	ComPtr<ID3D12Resource> depthBuffer;

	float FOV;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;

	unsigned int startWindowWidth = 1280;
	unsigned int startWindowHeight = 720;

	unsigned int frameCount = 0;
};