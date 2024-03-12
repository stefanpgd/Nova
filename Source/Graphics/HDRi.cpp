#include "Graphics/HDRI.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXPipeline.h"
#include <stb_image.h>

#include <imgui.h>

HDRI::HDRI(const std::string& filePath)
{
	int width;
	int height;
	int channels;

	stbi_set_flip_vertically_on_load(true);

	// Instead of 8 bits, we store 32 bits per channel with HDRIs
	float* buffer = stbi_loadf(filePath.c_str(), &width, &height, &channels, 4);
	stbi_set_flip_vertically_on_load(false);

	if(buffer == NULL)
	{
		LOG(Log::MessageType::Error, "Unsuccesful with loading: " + filePath);
		assert(false);
	}

	// 1. Upload Environment map
	UploadBuffer(buffer, width, height, hdriResource, hdriIndex);

	// 2. Allocate resource with matching dimensions to read/write to
	float* buff = new float[width * height * 4];
	UploadBuffer(buff, width, height, irradianceResource, irradianceIndex);

	// 3. Prepare pipeline //
	CreatePipeline();

	// 4. Convolute the HDRI //
	ConvoluteHDRI();

	stbi_image_free(buffer);
}

int HDRI::GetSRVIndex()
{
	return hdriIndex;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE HDRI::GetSRV()
{
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return SRVHeap->GetGPUHandleAt(hdriIndex);
}

D3D12_GPU_VIRTUAL_ADDRESS HDRI::GetGPULocation()
{
	return hdriResource->GetGPUVirtualAddress();
}

ComPtr<ID3D12Resource> HDRI::GetResource()
{
	return hdriResource;
}

void HDRI::HDRIDebugWindow()
{
	ImGui::Begin("HDRI View");

	ImGui::Text("HDRI - Regular");
	CD3DX12_GPU_DESCRIPTOR_HANDLE hdriHandle = GetSRV();
	ImGui::Image((ImTextureID)hdriHandle.ptr, ImVec2(512, 256));

	ImGui::Separator();

	ImGui::Text("HDRI - Irradiance Capture");
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE irradianceHandle = SRVHeap->GetGPUHandleAt(irradianceIndex);
	ImGui::Image((ImTextureID)irradianceHandle.ptr, ImVec2(512, 256));

	ImGui::End();
}

void HDRI::UploadBuffer(float* data, int width, int height, ComPtr<ID3D12Resource>& resource, int& index)
{
	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, width, height);
	description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_SUBRESOURCE_DATA subresource;
	subresource.pData = data;
	subresource.RowPitch = width * (sizeof(float) * 4);

	ComPtr<ID3D12Resource> intermediate;
	UploadPixelShaderResource(resource, intermediate, description, subresource);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	index = SRVHeap->GetNextAvailableIndex();

	DXAccess::GetDevice()->CreateShaderResourceView(resource.Get(), &srvDesc, SRVHeap->GetCPUHandleAt(index));
}

void HDRI::CreatePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 hdriRange[1];
	hdriRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // HDRI 

	CD3DX12_ROOT_PARAMETER1 skydomeRootParameters[1];
	skydomeRootParameters[0].InitAsDescriptorTable(1, &hdriRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // Skydome Texture

	rootSignature = new DXRootSignature(skydomeRootParameters, _countof(skydomeRootParameters),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	pipeline = new DXPipeline("Source/Shaders/hdriConvolution.vertex.hlsl", 
		"Source/Shaders/hdriConvolution.pixel.hlsl", rootSignature);
}

void HDRI::ConvoluteHDRI()
{
	DXCommands* commands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT);
	commands->Flush();

	//// 0. Grab all relevant variables //
	//ComPtr<ID3D12GraphicsCommandList2> commandList = commands->GetGraphicsCommandList();
	//ID3D12DescriptorHeap* heaps[] = { DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetAddress()};
	//CD3DX12_CPU_DESCRIPTOR_HANDLE depthView = DXAccess::GetWindow()->GetDepthDSV();
	//
	//
	//// 1. Clear Light DepthBuffer //
	//commandList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//
	//// 2. Bind pipeline & root // 
	//commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	//commandList->SetPipelineState(pipeline->GetAddress());
	//
	//// Bind viewport & rect, followed by the depth buffer //
	//commandList->RSSetViewports(1, &DXAccess::GetWindow()->GetViewport());
	//commandList->RSSetScissorRects(1, &DXAccess::GetWindow()->GetScissorRect());
	//commandList->OMSetRenderTargets(0, nullptr, FALSE, &depthView);
	//
	//// 3. Use Render Target as Texture //
	//commandList->SetGraphicsRootDescriptorTable(0, renderTexture);
	//
	//// 4. Bind & Render Screen Pass //
	//commandList->IASetVertexBuffers(0, 1, &screenMesh->GetVertexBufferView());
	//commandList->IASetIndexBuffer(&screenMesh->GetIndexBufferView());
	//commandList->DrawIndexedInstanced(screenMesh->GetIndicesCount(), 1, 0, 0, 0);
	//
	//// 5. Prepare screen buffer to be presented, since this is the last stage //
	//TransitionResource(screenBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}