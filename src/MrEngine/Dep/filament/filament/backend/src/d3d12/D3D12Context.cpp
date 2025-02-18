
#include "D3D12Context.h"
#include <assert.h>
#include <atlcomcli.h>
#include <assert.h>
#include "utils.hpp"
#include <d3dcompiler.h>
#include "D3D12DescriptorCache.h"

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
			//m_constantBuffer = createUploadBuffer(640 * 1024);

			//Create memory allocator
			UploadBufferAllocator = std::make_unique<MD3D12UploadBufferAllocator>(m_device.Get());

			DefaultBufferAllocator = std::make_unique<MD3D12DefaultBufferAllocator>(m_device.Get());

			TextureResourceAllocator = std::make_unique<MD3D3TextureResourceAllocator>(m_device.Get());

			//Create heapSlot allocator
			RTVHeapSlotAllocator = std::make_unique<MD3D12HeapSlotAllocator>(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 200);

			DSVHeapSlotAllocator = std::make_unique<MD3D12HeapSlotAllocator>(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 200);

			SRVHeapSlotAllocator = std::make_unique<MD3D12HeapSlotAllocator>(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 200);

			DescriptorCache = std::make_unique<MD3D12DescriptorCache>(this);

		}

		//UploadBufferRegion D3D12Context::allocFromUploadBuffer(UploadBuffer& buffer, UINT size, int align) const
		//{
		//	const UINT alignedSize = Utility::roundToPowerOfTwo(size, align);
		//	if (buffer.cursor + alignedSize > buffer.capacity) {
		//		throw std::overflow_error("Out of upload buffer capacity while allocating memory");
		//	}

		//	UploadBufferRegion region;
		//	region.cpuAddress = reinterpret_cast<void*>(buffer.cpuAddress + buffer.cursor);
		//	region.gpuAddress = buffer.gpuAddress + buffer.cursor;
		//	region.size = alignedSize;
		//	buffer.cursor += alignedSize;
		//	return region;
		//}

		MD3D12HeapSlotAllocator* D3D12Context::GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
		{
			switch (HeapType)
			{
			case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
				return SRVHeapSlotAllocator.get();
				break;

				//case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
				//	break;

			case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
				return RTVHeapSlotAllocator.get();
				break;

			case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
				return DSVHeapSlotAllocator.get();
				break;

			default:
				return nullptr;
			}
		}

		//UploadBuffer D3D12Context::createUploadBuffer(UINT capacity) const
		//{
		//	UploadBuffer buffer;
		//	buffer.cursor = 0;
		//	buffer.capacity = capacity;

		//	if (FAILED(m_device->CreateCommittedResource(
		//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		//		D3D12_HEAP_FLAG_NONE,
		//		&CD3DX12_RESOURCE_DESC::Buffer(capacity),
		//		D3D12_RESOURCE_STATE_GENERIC_READ,
		//		nullptr,
		//		IID_PPV_ARGS(&buffer.buffer))))
		//	{
		//		throw std::runtime_error("Failed to create GPU upload buffer");
		//	}
		//	if (FAILED(buffer.buffer->Map(0, &CD3DX12_RANGE{ 0, 0 }, reinterpret_cast<void**>(&buffer.cpuAddress)))) {
		//		throw std::runtime_error("Failed to map GPU upload buffer to host address space");
		//	}
		//	buffer.gpuAddress = buffer.buffer->GetGPUVirtualAddress();
		//	return buffer;
		//}

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

		//FrameBuffer D3D12Context::createFrameBuffer(UINT width, UINT height, UINT samples, DXGI_FORMAT colorFormat, DXGI_FORMAT depthstencilFormat)
		//{
		//	FrameBuffer fb = {};
		//	fb.width = width;
		//	fb.height = height;
		//	fb.samples = samples;

		//	D3D12_RESOURCE_DESC desc = {};
		//	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		//	desc.Width = width;
		//	desc.Height = height;
		//	desc.DepthOrArraySize = 1;
		//	desc.MipLevels = 1;
		//	desc.SampleDesc.Count = samples;

		//	if (colorFormat != DXGI_FORMAT_UNKNOWN) {
		//		desc.Format = colorFormat;
		//		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		//		const float optimizedClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		//		if (FAILED(m_device->CreateCommittedResource(
		//			&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
		//			D3D12_HEAP_FLAG_NONE,
		//			&desc,
		//			D3D12_RESOURCE_STATE_RENDER_TARGET,
		//			&CD3DX12_CLEAR_VALUE{ colorFormat, optimizedClearColor },
		//			IID_PPV_ARGS(&fb.colorTexture))))
		//		{
		//			throw std::runtime_error("Failed to create FrameBuffer color texture");
		//		}

		//		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		//		rtvDesc.Format = desc.Format;
		//		rtvDesc.ViewDimension = (samples > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;

		//		fb.rtv = m_descHeapRTV.alloc();
		//		m_device->CreateRenderTargetView(fb.colorTexture.Get(), &rtvDesc, fb.rtv.cpuHandle);

		//		if (samples <= 1) {
		//			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		//			srvDesc.Format = desc.Format;
		//			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		//			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//			srvDesc.Texture2D.MostDetailedMip = 0;
		//			srvDesc.Texture2D.MipLevels = 1;

		//			fb.srv = m_descHeapCBV_SRV_UAV.alloc();
		//			m_device->CreateShaderResourceView(fb.colorTexture.Get(), &srvDesc, fb.srv.cpuHandle);
		//		}
		//	}

		//	if (depthstencilFormat != DXGI_FORMAT_UNKNOWN) {
		//		desc.Format = depthstencilFormat;
		//		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		//		if (FAILED(m_device->CreateCommittedResource(
		//			&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
		//			D3D12_HEAP_FLAG_NONE,
		//			&desc,
		//			D3D12_RESOURCE_STATE_DEPTH_WRITE,
		//			&CD3DX12_CLEAR_VALUE{ depthstencilFormat, 1.0f, 0 },
		//			IID_PPV_ARGS(&fb.depthStencilTexture))))
		//		{
		//			throw std::runtime_error("Failed to create FrameBuffer depth-stencil texture");
		//		}

		//		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		//		dsvDesc.Format = desc.Format;
		//		dsvDesc.ViewDimension = (samples > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

		//		fb.dsv = m_descHeapDSV.alloc();
		//		m_device->CreateDepthStencilView(fb.depthStencilTexture.Get(), &dsvDesc, fb.dsv.cpuHandle);
		//	}
		//	return fb;
		//}

		void D3D12Context::TransitionResource(MD3D12Resource* Resource, D3D12_RESOURCE_STATES StateAfter)
		{
			D3D12_RESOURCE_STATES StateBefore = Resource->CurrentState;

			if (StateBefore != StateAfter)
			{
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(Resource->D3DResource.Get(), StateBefore, StateAfter));

				Resource->CurrentState = StateAfter;
			}
		}

		void D3D12Context::CopyResource(MD3D12Resource* DstResource, MD3D12Resource* SrcResource)
		{
			m_commandList->CopyResource(DstResource->D3DResource.Get(), SrcResource->D3DResource.Get());
		}

		void D3D12Context::CopyBufferRegion(MD3D12Resource* DstResource, UINT64 DstOffset, MD3D12Resource* SrcResource, UINT64 SrcOffset, UINT64 Size)
		{
			m_commandList->CopyBufferRegion(DstResource->D3DResource.Get(), DstOffset, SrcResource->D3DResource.Get(), SrcOffset, Size);
		}

		void D3D12Context::CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION* Dst, UINT DstX, UINT DstY, UINT DstZ, const D3D12_TEXTURE_COPY_LOCATION* Src, const D3D12_BOX* SrcBox)
		{
			m_commandList->CopyTextureRegion(Dst, DstX, DstY, DstZ, Src, SrcBox);
		}

		void D3D12Context::SetVertexBuffer(const MD3D12VertexBufferRef& VertexBuffer, UINT Offset, UINT Stride, UINT Size)
		{
			// Transition resource state
			const MD3D12ResourceLocation& ResourceLocation = VertexBuffer->ResourceLocation;
			MD3D12Resource* Resource = ResourceLocation.UnderlyingResource;
			TransitionResource(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

			// Set vertex buffer
			D3D12_VERTEX_BUFFER_VIEW VBV;
			VBV.BufferLocation = ResourceLocation.GPUVirtualAddress + Offset;
			VBV.StrideInBytes = Stride;
			VBV.SizeInBytes = Size;
			m_commandList->IASetVertexBuffers(0, 1, &VBV);
		}

		void D3D12Context::SetIndexBuffer(const MD3D12IndexBufferRef& IndexBuffer, UINT Offset, DXGI_FORMAT Format, UINT Size)
		{
			// Transition resource state
			const MD3D12ResourceLocation& ResourceLocation = IndexBuffer->ResourceLocation;
			MD3D12Resource* Resource = ResourceLocation.UnderlyingResource;
			TransitionResource(Resource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER);

			// Set vertex buffer
			D3D12_INDEX_BUFFER_VIEW IBV;
			IBV.BufferLocation = ResourceLocation.GPUVirtualAddress + Offset;
			IBV.Format = Format;
			IBV.SizeInBytes = Size;
			m_commandList->IASetIndexBuffer(&IBV);
		}

		D3D12Context::~D3D12Context()
		{

		}

		void D3D12Context::SetState(const backend::PipelineState& ps, D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
		{
			auto rs = rasterizer_states.find(ps.rasterState.u);
			if (rs == rasterizer_states.end())
			{
				D3D12_RASTERIZER_DESC raster_desc = { };
				raster_desc.FillMode = D3D12_FILL_MODE_SOLID;
				raster_desc.FrontCounterClockwise = !ps.rasterState.inverseFrontFaces;
				raster_desc.DepthClipEnable = TRUE;
				//raster_desc.ScissorEnable = TRUE;

				switch (ps.rasterState.culling)
				{
				case CullingMode::NONE:
					raster_desc.CullMode = D3D12_CULL_MODE_NONE;
					break;
				case CullingMode::FRONT:
					raster_desc.CullMode = D3D12_CULL_MODE_FRONT;
					break;
				case CullingMode::BACK:
					raster_desc.CullMode = D3D12_CULL_MODE_BACK;
					break;
				default:
					assert(false);
					break;
				}

				//ID3D11RasterizerState* raster = nullptr;
				//HRESULT hr = device->CreateRasterizerState(&raster_desc, &raster);
				//assert(SUCCEEDED(hr));

				auto get_blend_func = [](BlendFunction func) {
					switch (func)
					{
					case BlendFunction::ZERO: return D3D12_BLEND_ZERO;
					case BlendFunction::ONE: return D3D12_BLEND_ONE;
					case BlendFunction::SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
					case BlendFunction::ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
					case BlendFunction::DST_COLOR: return D3D12_BLEND_DEST_COLOR;
					case BlendFunction::ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
					case BlendFunction::SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
					case BlendFunction::ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
					case BlendFunction::DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
					case BlendFunction::ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
					case BlendFunction::SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
					default: assert(false);
					}
					return D3D12_BLEND_ZERO;
				};
				auto get_blend_op = [](BlendEquation op) {
					switch (op)
					{
					case BlendEquation::ADD: return D3D12_BLEND_OP_ADD;
					case BlendEquation::SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
					case BlendEquation::REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
					case BlendEquation::MIN: return D3D12_BLEND_OP_MIN;
					case BlendEquation::MAX: return D3D12_BLEND_OP_MAX;
					default: assert(false);
					}
					return D3D12_BLEND_OP_ADD;
				};

				D3D12_BLEND_DESC blend_desc = { };
				blend_desc.AlphaToCoverageEnable = ps.rasterState.alphaToCoverage;
				for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				{
					auto& target = blend_desc.RenderTarget[i];
					target.BlendEnable = ps.rasterState.hasBlending();
					target.SrcBlend = get_blend_func(ps.rasterState.blendFunctionSrcRGB);
					target.DestBlend = get_blend_func(ps.rasterState.blendFunctionDstRGB);
					target.SrcBlendAlpha = get_blend_func(ps.rasterState.blendFunctionSrcAlpha);
					target.DestBlendAlpha = get_blend_func(ps.rasterState.blendFunctionDstAlpha);
					target.BlendOp = get_blend_op(ps.rasterState.blendEquationRGB);
					target.BlendOpAlpha = get_blend_op(ps.rasterState.blendEquationAlpha);
					target.RenderTargetWriteMask = ps.rasterState.colorWrite ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
				}

				//ID3D11BlendState* blend = nullptr;
				//hr = device->CreateBlendState(&blend_desc, &blend);
				//assert(SUCCEEDED(hr));

				auto get_depth_func = [](SamplerCompareFunc func) {
					switch (func)
					{
					case SamplerCompareFunc::LE: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
					case SamplerCompareFunc::GE: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
					case SamplerCompareFunc::L: return D3D12_COMPARISON_FUNC_LESS;
					case SamplerCompareFunc::G: return D3D12_COMPARISON_FUNC_GREATER;
					case SamplerCompareFunc::E: return D3D12_COMPARISON_FUNC_EQUAL;
					case SamplerCompareFunc::NE: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
					case SamplerCompareFunc::A: return D3D12_COMPARISON_FUNC_ALWAYS;
					case SamplerCompareFunc::N: return D3D12_COMPARISON_FUNC_NEVER;
					default: assert(false);
					}
					return D3D12_COMPARISON_FUNC_LESS_EQUAL;
				};

				D3D12_DEPTH_STENCIL_DESC depth_desc = { };
				depth_desc.DepthEnable = TRUE;
				depth_desc.DepthWriteMask = ps.rasterState.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
				depth_desc.DepthFunc = get_depth_func(ps.rasterState.depthFunc);

				//ID3D11DepthStencilState* depth = nullptr;
				//hr = device->CreateDepthStencilState(&depth_desc, &depth);
				//assert(SUCCEEDED(hr));

				D3D12Context::RenderState state;
				state.raster = raster_desc;
				state.blend = blend_desc;
				state.depth = depth_desc;
				rasterizer_states.insert({ ps.rasterState.u, state });

				rs = rasterizer_states.find(ps.rasterState.u);
			}
			psoDesc.RasterizerState = rs->second.raster;
			psoDesc.BlendState = rs->second.blend;
			psoDesc.DepthStencilState = rs->second.depth;

			//context->RSSetState(rs->second.raster);
			//context->OMSetBlendState(rs->second.blend, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			//context->OMSetDepthStencilState(rs->second.depth, D3D11_DEFAULT_STENCIL_REFERENCE);
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
