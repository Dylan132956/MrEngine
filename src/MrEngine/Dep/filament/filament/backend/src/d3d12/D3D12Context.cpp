
#include "D3D12Context.h"
#include <assert.h>
#include <atlcomcli.h>
#include <assert.h>
#include "utils.hpp"
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;

//#include "d3dx12.h"
//#include "DDSTextureLoader.h"
//#include "MathHelper.h"

namespace filament
{
	namespace backend
	{
		D3D12Context::D3D12Context()
		{
			UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
			{
				ComPtr<ID3D12Debug> debugController;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
					debugController->EnableDebugLayer();
					dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
				}
			}
#endif

			if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)))) {
				throw std::runtime_error("Failed to create DXGI factory");
			}

			// Find D3D12 compatible adapter and create the device object.
			std::string dxgiAdapterDescription;
			{
				ComPtr<IDXGIAdapter1> adapter = getAdapter(m_dxgiFactory);
				if (adapter) {
					DXGI_ADAPTER_DESC adapterDesc;
					adapter->GetDesc(&adapterDesc);
					dxgiAdapterDescription = Utility::convertToUTF8(adapterDesc.Description);
				}
				else {
					throw std::runtime_error("No suitable video adapter supporting D3D12 found");
				}

				if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
					throw std::runtime_error("Failed to create D3D12 device");
				}

			}

			// Create default direct command queue.
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
				throw std::runtime_error("Failed to create command queue");
			}
			// Determine supported root signature version.
			{
				D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureFeature = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
				m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSignatureFeature, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE));
				m_rootSignatureVersion = rootSignatureFeature.HighestVersion;
			}

			// Create descriptor heaps.
			m_descHeapRTV = createDescriptorHeap({ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 16, D3D12_DESCRIPTOR_HEAP_FLAG_NONE });
			m_descHeapDSV = createDescriptorHeap({ D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 16, D3D12_DESCRIPTOR_HEAP_FLAG_NONE });
			m_descHeapCBV_SRV_UAV = createDescriptorHeap({ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE });

			// Create frame synchronization objects.
			{
				if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
					throw std::runtime_error("Failed to create fence object");
				}
				m_fenceCompletionEvent = CreateEvent(nullptr, false, false, nullptr);
			}

			std::printf("Direct3D 12 Renderer [%s]\n", dxgiAdapterDescription.c_str());

			// Determine maximum supported MSAA level.
			UINT samples;
			for (samples = maxSamples; samples > 1; samples /= 2) {
				D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS mqlColor = { DXGI_FORMAT_R16G16B16A16_FLOAT, samples };
				D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS mqlDepthStencil = { DXGI_FORMAT_D24_UNORM_S8_UINT, samples };
				m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &mqlColor, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
				m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &mqlDepthStencil, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
				if (mqlColor.NumQualityLevels > 0 && mqlDepthStencil.NumQualityLevels > 0) {
					break;
				}
			}

			for (UINT frameIndex = 0; frameIndex < NumFrames; ++frameIndex) {

				if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[frameIndex])))) {
					throw std::runtime_error("Failed to create command allocator");
				}
			}

			// Create default command list.
			if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList)))) {
				throw std::runtime_error("Failed to create direct command list");
			}

			// Create 64kB host-mapped buffer in the upload heap for shader constants.
			m_constantBuffer = createUploadBuffer(64 * 1024);

		}

		UploadBufferRegion D3D12Context::allocFromUploadBuffer(UploadBuffer& buffer, UINT size, int align) const
		{
			const UINT alignedSize = Utility::roundToPowerOfTwo(size, align);
			if (buffer.cursor + alignedSize > buffer.capacity) {
				throw std::overflow_error("Out of upload buffer capacity while allocating memory");
			}

			UploadBufferRegion region;
			region.cpuAddress = reinterpret_cast<void*>(buffer.cpuAddress + buffer.cursor);
			region.gpuAddress = buffer.gpuAddress + buffer.cursor;
			region.size = alignedSize;
			buffer.cursor += alignedSize;
			return region;
		}

		UploadBuffer D3D12Context::createUploadBuffer(UINT capacity) const
		{
			UploadBuffer buffer;
			buffer.cursor = 0;
			buffer.capacity = capacity;

			if (FAILED(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(capacity),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&buffer.buffer))))
			{
				throw std::runtime_error("Failed to create GPU upload buffer");
			}
			if (FAILED(buffer.buffer->Map(0, &CD3DX12_RANGE{ 0, 0 }, reinterpret_cast<void**>(&buffer.cpuAddress)))) {
				throw std::runtime_error("Failed to map GPU upload buffer to host address space");
			}
			buffer.gpuAddress = buffer.buffer->GetGPUVirtualAddress();
			return buffer;
		}

		ComPtr<ID3DBlob> D3D12Context::compileShader(const std::string& filename, const std::string& entryPoint, const std::string& profile)
		{
			UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
			flags |= D3DCOMPILE_DEBUG;
			flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

			ComPtr<ID3DBlob> shader;
			ComPtr<ID3DBlob> errorBlob;

			std::printf("Compiling HLSL shader: %s [%s]\n", filename.c_str(), entryPoint.c_str());

			if (FAILED(D3DCompileFromFile(Utility::convertToUTF16(filename).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), profile.c_str(), flags, 0, &shader, &errorBlob))) {
				std::string errorMsg = "Shader compilation failed: " + filename;
				if (errorBlob) {
					errorMsg += std::string("\n") + static_cast<const char*>(errorBlob->GetBufferPointer());
				}
				throw std::runtime_error(errorMsg);
			}
			return shader;
		}

		ComPtr<ID3D12RootSignature> D3D12Context::createRootSignature(D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) const
		{
			const D3D12_ROOT_SIGNATURE_FLAGS standardFlags =
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

			switch (desc.Version) {
			case D3D_ROOT_SIGNATURE_VERSION_1_0:
				desc.Desc_1_0.Flags |= standardFlags;
				break;
			case D3D_ROOT_SIGNATURE_VERSION_1_1:
				desc.Desc_1_1.Flags |= standardFlags;
				break;
			}

			ComPtr<ID3D12RootSignature> rootSignature;
			ComPtr<ID3DBlob> signatureBlob, errorBlob;
			if (FAILED(D3DX12SerializeVersionedRootSignature(&desc, m_rootSignatureVersion, &signatureBlob, &errorBlob))) {
				throw std::runtime_error("Failed to serialize root signature");
			}
			if (FAILED(m_device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) {
				throw std::runtime_error("Failed to create root signature");
			}
			return rootSignature;
		}

		void D3D12Context::executeCommandList(bool reset) const
		{
			if (FAILED(m_commandList->Close())) {
				throw std::runtime_error("Failed close command list (validation error or not in recording state)");
			}

			ID3D12CommandList* lists[] = { m_commandList.Get() };
			m_commandQueue->ExecuteCommandLists(1, lists);

			if (reset) {
				m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);
			}
		}

		void D3D12Context::waitForGPU() const
		{
			UINT64& fenceValue = m_fenceValues[m_frameIndex];
			m_commandQueue->Signal(m_fence.Get(), fenceValue);
			m_fence->SetEventOnCompletion(fenceValue, m_fenceCompletionEvent);
			WaitForSingleObjectEx(m_fenceCompletionEvent, INFINITE, FALSE);
			++fenceValue;

		}

		ComPtr<IDXGIAdapter1> D3D12Context::getAdapter(const ComPtr<IDXGIFactory4>& factory)
		{
			ComPtr<IDXGIAdapter1> adapter;
			for (UINT index = 0; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index) {
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
					continue;
				}
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
					return adapter;
				}
			}
			return nullptr;
		}

		DescriptorHeap D3D12Context::createDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC& desc) const
		{
			DescriptorHeap heap;
			if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.heap)))) {
				throw std::runtime_error("Failed to create descriptor heap");
			}
			heap.numDescriptorsAllocated = 0;
			heap.numDescriptorsInHeap = desc.NumDescriptors;
			heap.descriptorSize = m_device->GetDescriptorHandleIncrementSize(desc.Type);
			return heap;
		}

		FrameBuffer D3D12Context::createFrameBuffer(UINT width, UINT height, UINT samples, DXGI_FORMAT colorFormat, DXGI_FORMAT depthstencilFormat)
		{
			FrameBuffer fb = {};
			fb.width = width;
			fb.height = height;
			fb.samples = samples;

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.SampleDesc.Count = samples;

			if (colorFormat != DXGI_FORMAT_UNKNOWN) {
				desc.Format = colorFormat;
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

				const float optimizedClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				if (FAILED(m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					&CD3DX12_CLEAR_VALUE{ colorFormat, optimizedClearColor },
					IID_PPV_ARGS(&fb.colorTexture))))
				{
					throw std::runtime_error("Failed to create FrameBuffer color texture");
				}

				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = desc.Format;
				rtvDesc.ViewDimension = (samples > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;

				fb.rtv = m_descHeapRTV.alloc();
				m_device->CreateRenderTargetView(fb.colorTexture.Get(), &rtvDesc, fb.rtv.cpuHandle);

				if (samples <= 1) {
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.Format = desc.Format;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.Texture2D.MostDetailedMip = 0;
					srvDesc.Texture2D.MipLevels = 1;

					fb.srv = m_descHeapCBV_SRV_UAV.alloc();
					m_device->CreateShaderResourceView(fb.colorTexture.Get(), &srvDesc, fb.srv.cpuHandle);
				}
			}

			if (depthstencilFormat != DXGI_FORMAT_UNKNOWN) {
				desc.Format = depthstencilFormat;
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

				if (FAILED(m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					&CD3DX12_CLEAR_VALUE{ depthstencilFormat, 1.0f, 0 },
					IID_PPV_ARGS(&fb.depthStencilTexture))))
				{
					throw std::runtime_error("Failed to create FrameBuffer depth-stencil texture");
				}

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = desc.Format;
				dsvDesc.ViewDimension = (samples > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

				fb.dsv = m_descHeapDSV.alloc();
				m_device->CreateDepthStencilView(fb.depthStencilTexture.Get(), &dsvDesc, fb.dsv.cpuHandle);
			}
			return fb;
		}

		D3D12Context::~D3D12Context()
		{

		}

		void D3D12Context::SetState(const backend::PipelineState& ps)
		{

		}

		DXGI_FORMAT D3D12Context::GetTextureFormat(TextureFormat format)
		{
			switch (format)
			{
				case TextureFormat::R8: return DXGI_FORMAT_R8_UNORM;
				case TextureFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::R16F: return DXGI_FORMAT_R16_FLOAT;
                case TextureFormat::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                case TextureFormat::R32F: return DXGI_FORMAT_R32_FLOAT;
                case TextureFormat::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case TextureFormat::DEPTH16: return DXGI_FORMAT_R16_TYPELESS;
				case TextureFormat::DEPTH24: return DXGI_FORMAT_UNKNOWN;
				case TextureFormat::DEPTH24_STENCIL8: return DXGI_FORMAT_R24G8_TYPELESS;
				case TextureFormat::DEPTH32F: return DXGI_FORMAT_R32_TYPELESS;
				case TextureFormat::DEPTH32F_STENCIL8: return DXGI_FORMAT_R32G8X24_TYPELESS;
				case TextureFormat::DXT1_RGB: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT1_RGBA: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT3_RGBA: return DXGI_FORMAT_BC2_UNORM;
				case TextureFormat::DXT5_RGBA: return DXGI_FORMAT_BC3_UNORM;
				default: assert(false); break;
			}
			return DXGI_FORMAT_UNKNOWN;
		}

		DXGI_FORMAT D3D12Context::GetTextureViewFormat(TextureFormat format)
		{
			switch (format)
			{
				case TextureFormat::R8: return DXGI_FORMAT_R8_UNORM;
				case TextureFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::R16F: return DXGI_FORMAT_R16_FLOAT;
                case TextureFormat::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                case TextureFormat::R32F: return DXGI_FORMAT_R32_FLOAT;
                case TextureFormat::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case TextureFormat::DEPTH16: return DXGI_FORMAT_R16_UNORM;
				case TextureFormat::DEPTH24: return DXGI_FORMAT_UNKNOWN;
				case TextureFormat::DEPTH24_STENCIL8: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				case TextureFormat::DEPTH32F: return DXGI_FORMAT_R32_FLOAT;
				case TextureFormat::DEPTH32F_STENCIL8: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				case TextureFormat::DXT1_RGB: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT1_RGBA: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT3_RGBA: return DXGI_FORMAT_BC2_UNORM;
				case TextureFormat::DXT5_RGBA: return DXGI_FORMAT_BC3_UNORM;
				default: assert(false); break;
			}
			return DXGI_FORMAT_UNKNOWN;
		}

		DXGI_FORMAT D3D12Context::GetDepthViewFormat(TextureFormat format)
		{
			switch (format)
			{
				case TextureFormat::DEPTH16: return DXGI_FORMAT_D16_UNORM;
				case TextureFormat::DEPTH24: return DXGI_FORMAT_UNKNOWN;
				case TextureFormat::DEPTH24_STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
				case TextureFormat::DEPTH32F: return DXGI_FORMAT_D32_FLOAT;
				case TextureFormat::DEPTH32F_STENCIL8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				default: assert(false); break;
			}
			return DXGI_FORMAT_UNKNOWN;
		}
	}
}
