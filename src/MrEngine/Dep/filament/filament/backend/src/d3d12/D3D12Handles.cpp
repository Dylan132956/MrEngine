
#define INITGUID
#include "D3D12Handles.h"
#include <d3dcompiler.h>
#include <assert.h>
#include "utils.hpp"

namespace filament
{
	namespace backend
	{
		D3D12SwapChain::D3D12SwapChain(D3D12Context* context, void* native_window)
		{
			context->waitForGPU();
			// Release resources that are tied to the swap chain and update fence values.
			for (UINT n = 0; n < context->NumFrames; n++)
			{
				context->m_fenceValues[n] = context->m_fenceValues[context->m_frameIndex];
			}

			int window_width = 0;
			int window_height = 0;

			DXGI_SWAP_CHAIN_DESC1 desc = { };

			HWND window = reinterpret_cast<HWND>(native_window);
			RECT rect;
			GetClientRect(window, &rect);
			window_width = rect.right - rect.left;
			window_height = rect.bottom - rect.top;
			// Create per-frame resources.
			//context->m_descHeapRTV.numDescriptorsAllocated = 0;
			//context->m_descHeapDSV.numDescriptorsAllocated = 0;
			//context->m_descHeapCBV_SRV_UAV.numDescriptorsAllocated = 0;
			{
				DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
				swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				swapChainDesc.SampleDesc.Count = 1;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferCount = context->NumFrames;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

				ComPtr<IDXGISwapChain1> swapChain;
				if (FAILED(context->m_dxgiFactory->CreateSwapChainForHwnd(context->m_commandQueue.Get(), window, &swapChainDesc, nullptr, nullptr, &swapChain))) {
					throw std::runtime_error("Failed to create swap chain");
				}
				swapChain.As(&m_swapChain);
			}
			context->m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
			context->m_dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
			// Create window swap chain.
			// Create per-frame resources.
			for (UINT frameIndex = 0; frameIndex < context->NumFrames; ++frameIndex) {
				if (FAILED(m_swapChain->GetBuffer(frameIndex, IID_PPV_ARGS(&m_backbuffers[frameIndex].buffer)))) {
					throw std::runtime_error("Failed to retrieve swap chain back buffer");
				}

				//m_backbuffers[frameIndex].rtv = context->m_descHeapRTV.alloc();
				D3D12_RENDER_TARGET_VIEW_DESC RtvDesc = {};
				RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				RtvDesc.Texture2D.MipSlice = 0;
				RtvDesc.Texture2D.PlaneSlice = 0;
				RtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				m_backbuffers[frameIndex].RTV = std::make_unique<MD3D12RenderTargetView>(context, RtvDesc, m_backbuffers[frameIndex].buffer.Get());
				//context->m_device->CreateRenderTargetView(m_backbuffers[frameIndex].buffer.Get(), nullptr, m_backbuffers[frameIndex].rtv.cpuHandle);

				//context->m_framebuffers[frameIndex] = context->createFrameBuffer(window_width, window_height, samples, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT);
				//if (samples > 1) {
				//	context->m_resolveFramebuffers[frameIndex] = context->createFrameBuffer(window_width, window_height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, (DXGI_FORMAT)0);
				//}
				//else {
				//	context->m_resolveFramebuffers[frameIndex] = context->m_framebuffers[frameIndex];
				//}
			}
		}

		D3D12SwapChain::~D3D12SwapChain()
		{

		}

		D3D12RenderTarget::D3D12RenderTarget(
			D3D12Context* context,
			TargetBufferFlags flags,
			uint32_t width,
			uint32_t height,
			uint8_t samples,
			TargetBufferInfo color,
			TargetBufferInfo depth,
			TargetBufferInfo stencil):
			HwRenderTarget(width, height)
		{
			
		}

		D3D12RenderTarget::D3D12RenderTarget(D3D12Context* context):
			HwRenderTarget(0, 0)
		{
			default_render_target = true;
		}

		D3D12RenderTarget::~D3D12RenderTarget()
		{

		}

		void D3D12RenderTarget::CreateDepth(D3D12Context* context, DXGI_FORMAT format, uint32_t samples, uint32_t width, uint32_t height)
		{
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.SampleDesc.Count = samples;
			if (format != DXGI_FORMAT_UNKNOWN) {
				desc.Format = format;
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

				if (FAILED(context->m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					&CD3DX12_CLEAR_VALUE{ format, 1.0f, 0 },
					IID_PPV_ARGS(&depthStencilTexture))))
				{
					throw std::runtime_error("Failed to create RenderTarget depth-stencil texture");
				}

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = desc.Format;
				dsvDesc.ViewDimension = (samples > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

				DSV = std::make_unique<MD3D12DepthStencilView>(context, dsvDesc, depthStencilTexture.Get());
				//dsv = context->m_descHeapDSV.alloc();
				//context->m_device->CreateDepthStencilView(depthStencilTexture.Get(), &dsvDesc, dsv.cpuHandle);
			}
		}

		D3D12Program::D3D12Program(D3D12Context* context, Program&& program):
			HwProgram(program.getName())
		{
			//program.withFragmentShader(NULL, 0);
			const auto& sources = program.getShadersSource();
			for (size_t i = 0; i < sources.size(); ++i)
			{
				std::string src((const char*) &sources[i][0], sources[i].size());
				auto type = (Program::Shader) i;

				const char* target = nullptr;
                std::string sEntrypoint;
				if (type == Program::Shader::VERTEX)
				{
					//target = "vs_4_0_level_9_3";
					target = "vs_5_0";
                    sEntrypoint = "vert";
				}
				else if (type == Program::Shader::FRAGMENT)
				{
					//target = "ps_4_0_level_9_3";
					target = "ps_4_0";
                    sEntrypoint = "frag";
				}

				ID3DBlob* binary = nullptr;
				ID3DBlob* error = nullptr;

				HRESULT hr = D3DCompile(
					&src[0],
					src.size(),
					program.getName().c_str(),
					nullptr,
                    D3D_COMPILE_STANDARD_FILE_INCLUDE,
                    sEntrypoint.c_str(),
					target,
					0,
					0,
					&binary,
					&error);

                if (error)
                {
                    std::string message = reinterpret_cast<const char*>(error->GetBufferPointer());
                    assert(!error);
                }

				if (type == Program::Shader::VERTEX)
				{
					vertex_binary = binary;
					//reflection
					ID3D12ShaderReflection* pReflector = NULL;
					D3DReflect(binary->GetBufferPointer(), binary->GetBufferSize(),
						IID_ID3D12ShaderReflection, (void**)&pReflector);

					D3D12_SHADER_DESC shaderDesc;
					pReflector->GetDesc(&shaderDesc);
					///
					D3D12_SIGNATURE_PARAMETER_DESC signature_param_desc{};
					std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_descs(shaderDesc.InputParameters);
					for (int i = 0; i < shaderDesc.InputParameters; i++)
					{
						pReflector->GetInputParameterDesc(i, &signature_param_desc);
						input_element_descs[i].SemanticName = signature_param_desc.SemanticName;
						input_element_descs[i].SemanticIndex = signature_param_desc.SemanticIndex;
						input_element_descs[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
						input_element_descs[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						input_element_descs[i].InputSlot = 0;

						if (signature_param_desc.Mask == 1)
						{
							if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)        input_element_descs[i].Format = DXGI_FORMAT_R32_UINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  input_element_descs[i].Format = DXGI_FORMAT_R32_SINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element_descs[i].Format = DXGI_FORMAT_R32_FLOAT;
						}
						else if (signature_param_desc.Mask <= 3)
						{
							if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)        input_element_descs[i].Format = DXGI_FORMAT_R32G32_UINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  input_element_descs[i].Format = DXGI_FORMAT_R32G32_SINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element_descs[i].Format = DXGI_FORMAT_R32G32_FLOAT;
						}
						else if (signature_param_desc.Mask <= 7)
						{
							if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)        input_element_descs[i].Format = DXGI_FORMAT_R32G32B32_UINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  input_element_descs[i].Format = DXGI_FORMAT_R32G32B32_SINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element_descs[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
						}
						else if (signature_param_desc.Mask <= 15)
						{
							if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)        input_element_descs[i].Format = DXGI_FORMAT_R32G32B32A32_UINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  input_element_descs[i].Format = DXGI_FORMAT_R32G32B32A32_SINT;
							else if (signature_param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_element_descs[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
						}
					}
					//vert_cbv
					for (int i = 0; i != shaderDesc.ConstantBuffers; ++i)
					{
						ID3D12ShaderReflectionConstantBuffer* cb = pReflector->GetConstantBufferByIndex(i);
						D3D12_SHADER_BUFFER_DESC cbDesc;
						cb->GetDesc(&cbDesc);    
						for (int j = 0; j != shaderDesc.BoundResources; ++j)
						{
							D3D12_SHADER_INPUT_BIND_DESC bindDesc;
							pReflector->GetResourceBindingDesc(j, &bindDesc);
							if (strcmp(cbDesc.Name, bindDesc.Name) == 0)
							{
								vert_cbv.push_back(bindDesc);
								std::cout << cbDesc.Name << " " << bindDesc.BindPoint << std::endl;
								break;
							}
						}
						for (UINT j = 0; j < cbDesc.Variables; j++)
						{
							ID3D12ShaderReflectionVariable* pVariable = cb->GetVariableByIndex(j);
							D3D12_SHADER_VARIABLE_DESC var_desc;
							pVariable->GetDesc(&var_desc);
							std::cout << " Name: " << var_desc.Name;
							std::cout << " Size: " << var_desc.Size;
							std::cout << " Offset: " << var_desc.StartOffset << "\n";
						}
					}
					//vert_srv
					for (int i = 0; i != shaderDesc.BoundResources; ++i)
					{
						D3D12_SHADER_INPUT_BIND_DESC bindDesc;
						pReflector->GetResourceBindingDesc(i, &bindDesc);
						if (bindDesc.Type == D3D_SIT_TEXTURE)
						{
							vert_srv.push_back(bindDesc);
						}
						if (bindDesc.Type == D3D_SIT_SAMPLER)
						{
							vert_sam.push_back(bindDesc);
						}
					}
				}
				else if (type == Program::Shader::FRAGMENT)
				{
					pixel_binary = binary;
					//reflection
					ID3D12ShaderReflection* pReflector = NULL;
					D3DReflect(binary->GetBufferPointer(), binary->GetBufferSize(),
						IID_ID3D12ShaderReflection, (void**)&pReflector);

					D3D12_SHADER_DESC shaderDesc;
					pReflector->GetDesc(&shaderDesc);
					//frag_cbv
					for (int i = 0; i != shaderDesc.ConstantBuffers; ++i)
					{
						ID3D12ShaderReflectionConstantBuffer* cb = pReflector->GetConstantBufferByIndex(i);
						D3D12_SHADER_BUFFER_DESC cbDesc;
						cb->GetDesc(&cbDesc);
						for (int j = 0; j != shaderDesc.BoundResources; ++j)
						{
							D3D12_SHADER_INPUT_BIND_DESC bindDesc;
							pReflector->GetResourceBindingDesc(j, &bindDesc);
							if (strcmp(cbDesc.Name, bindDesc.Name) == 0)
							{
								frag_cbv.push_back(bindDesc);
								std::cout << cbDesc.Name << " " << bindDesc.BindPoint << std::endl;
								break;
							}
						}
						for (UINT j = 0; j < cbDesc.Variables; j++)
						{
							ID3D12ShaderReflectionVariable* pVariable = cb->GetVariableByIndex(j);
							D3D12_SHADER_VARIABLE_DESC var_desc;
							pVariable->GetDesc(&var_desc);
							std::cout << " Name: " << var_desc.Name;
							std::cout << " Size: " << var_desc.Size;
							std::cout << " Offset: " << var_desc.StartOffset << "\n";
						}
					}
					//frag_srv
					for (int i = 0; i != shaderDesc.BoundResources; ++i)
					{
						D3D12_SHADER_INPUT_BIND_DESC bindDesc;
						pReflector->GetResourceBindingDesc(i, &bindDesc);
						if (bindDesc.Type == D3D_SIT_TEXTURE)
						{
							frag_srv.push_back(bindDesc);
						}
						if (bindDesc.Type == D3D_SIT_SAMPLER)
						{
							frag_sam.push_back(bindDesc);
						}
					}
				}

				SAFE_RELEASE(error);
			}

			info = std::move(program);
		}

		ComPtr<ID3D12PipelineState> D3D12Program::GetPSO(D3D12Context* context, const backend::PipelineState& ps)
		{
			if (m_PipelineState == nullptr) {
				UINT total_size = vert_cbv.size() + vert_srv.size() + frag_cbv.size() + frag_srv.size();
				std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
				std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptorRanges;
				rootParameters.resize(total_size);
				descriptorRanges.resize(total_size);

				UINT index = 0;
				for (size_t i = 0; i < vert_cbv.size(); ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC bind_desc = vert_cbv[i];
					//CD3DX12_DESCRIPTOR_RANGE1 descriptor = { D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, bind_desc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC };
					//descriptorRanges[index] = descriptor;
					//rootParameters[index].InitAsDescriptorTable(1, &descriptorRanges[index], D3D12_SHADER_VISIBILITY_VERTEX);
					rootParameters[index].InitAsConstantBufferView(bind_desc.BindPoint, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
					vert_cbv_map[bind_desc.BindPoint] = index;
					index++;
				}
				for (size_t i = 0; i < vert_srv.size(); ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC bind_desc = vert_srv[i];
					const CD3DX12_DESCRIPTOR_RANGE1 descriptor = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bind_desc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE };
					descriptorRanges[index] = descriptor;
					rootParameters[index].InitAsDescriptorTable(1, &descriptorRanges[index], D3D12_SHADER_VISIBILITY_VERTEX);
					vert_srv_map[bind_desc.BindPoint] = index;
					index++;
				}
				//
				for (size_t i = 0; i < frag_cbv.size(); ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC bind_desc = frag_cbv[i];
					//const CD3DX12_DESCRIPTOR_RANGE1 descriptor = { D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, bind_desc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC };
					//descriptorRanges[index] = descriptor;
					//rootParameters[index].InitAsDescriptorTable(1, &descriptorRanges[index], D3D12_SHADER_VISIBILITY_PIXEL);
					rootParameters[index].InitAsConstantBufferView(bind_desc.BindPoint, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_PIXEL);
					frag_cbv_map[bind_desc.BindPoint] = index;
					index++;
				}
				for (size_t i = 0; i < frag_srv.size(); ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC bind_desc = frag_srv[i];
					const CD3DX12_DESCRIPTOR_RANGE1 descriptor = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bind_desc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE };
					descriptorRanges[index] = descriptor;
					rootParameters[index].InitAsDescriptorTable(1, &descriptorRanges[index], D3D12_SHADER_VISIBILITY_PIXEL);
					frag_srv_map[bind_desc.BindPoint] = index;
					index++;
				}

				std::vector<CD3DX12_STATIC_SAMPLER_DESC> defaultSamplerDescs;
				UINT sam_size = vert_sam.size() + frag_sam.size();
				defaultSamplerDescs.resize(sam_size);
				UINT sam_index = 0;
				for (size_t i = 0; i < vert_sam.size(); ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC bind_desc = vert_sam[i];
					CD3DX12_STATIC_SAMPLER_DESC defaultSamplerDesc{ bind_desc.BindPoint, D3D12_FILTER_ANISOTROPIC };
					defaultSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
					defaultSamplerDescs[sam_index] = defaultSamplerDesc;
					sam_index++;
				}

				for (size_t i = 0; i < frag_sam.size(); ++i)
				{
					D3D12_SHADER_INPUT_BIND_DESC bind_desc = frag_sam[i];
					CD3DX12_STATIC_SAMPLER_DESC defaultSamplerDesc{ bind_desc.BindPoint, D3D12_FILTER_ANISOTROPIC };
					defaultSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
					defaultSamplerDescs[sam_index] = defaultSamplerDesc;
					sam_index++;
				}

				CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC signatureDesc;
				signatureDesc.Init_1_1(rootParameters.size(), rootParameters.data(), defaultSamplerDescs.size(), defaultSamplerDescs.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
				m_RootSignature = context->createRootSignature(signatureDesc);

				D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
				psoDesc.pRootSignature = m_RootSignature.Get();
				psoDesc.InputLayout = { meshInputLayout.data(), (UINT)meshInputLayout.size() };
				psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertex_binary);
				psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixel_binary);
				context->SetState(ps, psoDesc);
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				psoDesc.NumRenderTargets = 1;
				psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
				psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
				psoDesc.SampleDesc.Count = 1;
				psoDesc.SampleMask = UINT_MAX;

				if (FAILED(context->m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)))) {
					throw std::runtime_error("Failed to create graphics pipeline state");
				}
			}
			return m_PipelineState;
		}

		D3D12Program::~D3D12Program()
		{
			SAFE_RELEASE(vertex_binary);
			SAFE_RELEASE(pixel_binary);
		}

		D3D12UniformBuffer::D3D12UniformBuffer(D3D12Context* context, size_t size, BufferUsage usage):
			size(size),
			usage(usage)
		{
			cb = std::make_shared<MD3D12ConstantBuffer>();
			auto UploadBufferAllocator = context->GetUploadBufferAllocator();
			UploadBufferAllocator->AllocUploadResource(size, UPLOAD_RESOURCE_ALIGNMENT, cb->ResourceLocation);
		}

		D3D12UniformBuffer::~D3D12UniformBuffer()
		{

		}

		void D3D12UniformBuffer::Load(D3D12Context* context, const BufferDescriptor& data)
		{
			if (data.buffer) {
				std::memcpy(cb->ResourceLocation.MappedAddress, data.buffer, data.size);
			}
		}

		D3D12SamplerGroup::D3D12SamplerGroup(D3D12Context* context, size_t size):
			HwSamplerGroup(size)
		{
		
		}

		D3D12SamplerGroup::~D3D12SamplerGroup()
		{
			
		}

		void D3D12SamplerGroup::Update(D3D12Context* context, SamplerGroup&& sg)
		{
			*sb = std::move(sg);
		}

		StagingBuffer createStagingBuffer(D3D12Context* context, const ComPtr<ID3D12Resource>& resource, UINT firstSubresource, UINT numSubresources, const D3D12_SUBRESOURCE_DATA* data)
		{
			StagingBuffer stagingBuffer;
			stagingBuffer.firstSubresource = firstSubresource;
			stagingBuffer.numSubresources = numSubresources;
			stagingBuffer.layouts.resize(numSubresources);

			const D3D12_RESOURCE_DESC resourceDesc = resource->GetDesc();

			UINT64 numBytesTotal;
			std::vector<UINT> numRows{ numSubresources };
			std::vector<UINT64> rowBytes{ numSubresources };
			context->m_device->GetCopyableFootprints(&resourceDesc, firstSubresource, numSubresources, 0, stagingBuffer.layouts.data(), numRows.data(), rowBytes.data(), &numBytesTotal);

			if (FAILED(context->m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(numBytesTotal),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&stagingBuffer.buffer))))
			{
				throw std::runtime_error("Failed to create GPU staging buffer");
			}

			if (data) {
				assert(resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D);
				void* bufferMemory;
				if (FAILED(stagingBuffer.buffer->Map(0, &CD3DX12_RANGE{ 0, 0 }, &bufferMemory))) {
					throw std::runtime_error("Failed to map GPU staging buffer to host address space");
				}

				for (UINT subresource = 0; subresource < numSubresources; ++subresource) {
					uint8_t* subresourceMemory = reinterpret_cast<uint8_t*>(bufferMemory) + stagingBuffer.layouts[subresource].Offset;
					if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
						// Generic buffer: Copy everything in one go.
						std::memcpy(subresourceMemory, data->pData, numBytesTotal);
					}
					else {
						// Texture: Copy data line by line.
						for (UINT row = 0; row < numRows[subresource]; ++row) {
							const uint8_t* srcRowPtr = reinterpret_cast<const uint8_t*>(data[subresource].pData) + row * data[subresource].RowPitch;
							uint8_t* destRowPtr = subresourceMemory + row * stagingBuffer.layouts[subresource].Footprint.RowPitch;
							std::memcpy(destRowPtr, srcRowPtr, rowBytes[subresource]);
						}
					}
				}
				stagingBuffer.buffer->Unmap(0, nullptr);
			}
			return stagingBuffer;
		}

		void D3D12Texture::createTextureSRV(D3D12Context* context, D3D12_SRV_DIMENSION dimension, UINT mostDetailedMip, UINT mipLevels)
		{
			const D3D12_RESOURCE_DESC desc = texture->GetDesc();
			const UINT effectiveMipLevels = (mipLevels > 0) ? mipLevels : (desc.MipLevels - mostDetailedMip);
			assert(!(desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = desc.Format;
			srvDesc.ViewDimension = dimension;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			switch (dimension) {
			case D3D12_SRV_DIMENSION_TEXTURE2D:
				srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
				srvDesc.Texture2D.MipLevels = effectiveMipLevels;
				break;
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
				srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
				srvDesc.Texture2DArray.MipLevels = effectiveMipLevels;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
				break;
			case D3D12_SRV_DIMENSION_TEXTURECUBE:
				assert(desc.DepthOrArraySize == 6);
				srvDesc.TextureCube.MostDetailedMip = mostDetailedMip;
				srvDesc.TextureCube.MipLevels = effectiveMipLevels;
				break;
			default:
				assert(0);
			}
			SRV = std::make_unique<MD3D12ShaderResourceView>(context, srvDesc, texture.Get());
		}

		void D3D12Texture::createTextureUAV(D3D12Context* context, UINT mipSlice)
		{
			const D3D12_RESOURCE_DESC desc = texture->GetDesc();
			assert(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = desc.Format;
			if (desc.DepthOrArraySize > 1) {
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.MipSlice = mipSlice;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
			}
			else {
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = mipSlice;
			}
			UAV = std::make_unique<MD3D12UnorderedAccessView>(context, uavDesc, texture.Get());
		}

		void D3D12Texture::CreateDepth(D3D12Context* context, DXGI_FORMAT format, uint32_t samples, uint32_t width, uint32_t height)
		{
			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.SampleDesc.Count = samples;
			if (format != DXGI_FORMAT_UNKNOWN) {
				desc.Format = format;
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

				if (FAILED(context->m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					&CD3DX12_CLEAR_VALUE{ format, 1.0f, 0 },
					IID_PPV_ARGS(&texture))))
				{
					throw std::runtime_error("Failed to create RenderTarget depth-stencil texture");
				}

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = desc.Format;
				dsvDesc.ViewDimension = (samples > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
				
				DSV = std::make_unique<MD3D12DepthStencilView>(context, dsvDesc, texture.Get());
			}
		}

		D3D12Texture::D3D12Texture(
			D3D12Context* context,
			backend::SamplerType target,
			uint8_t levels,
			TextureFormat format,
			uint8_t samples,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			TextureUsage usage):
			HwTexture(target, levels, samples, width, height, depth, format, usage)
		{
			UINT mip_levels = levels;
			UINT array_size = depth;
			UINT bind_flags = 0;
			UINT misc_flags = 0;

			assert(depth == 1 || depth == 6);

			levels = (levels > 0) ? levels : Utility::numMipmapLevels(width, height);

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = depth;
			desc.MipLevels = levels;
			desc.SampleDesc.Count = 1;
			if (format == TextureFormat::DEPTH24_STENCIL8)
			{
				desc.Format = context->GetDepthViewFormat(format);
				CreateDepth(context, desc.Format, 1, width, height);
			}
			else
			{
				desc.Format = context->GetTextureFormat(format);;
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				if (FAILED(context->m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&texture))))
				{
					throw std::runtime_error("Failed to create 2D texture");
				}

				D3D12_SRV_DIMENSION srvDim;
				switch (depth) {
				case 1:  srvDim = D3D12_SRV_DIMENSION_TEXTURE2D; break;
				case 6:  srvDim = D3D12_SRV_DIMENSION_TEXTURECUBE; break;
				default: srvDim = D3D12_SRV_DIMENSION_TEXTURE2DARRAY; break;
				}
				createTextureSRV(context, srvDim);
			}
		}

		D3D12Texture::~D3D12Texture()
		{

		}

		void D3D12Texture::UpdateTexture(
			D3D12Context* context,
			int layer, int level,
			int x, int y,
			int w, int h,
			const PixelBufferDescriptor& data)
		{
			UINT row_pitch = 0;

			int bc_block_bytes = 0;
			switch (format)
			{
			case TextureFormat::DXT1_RGB:
			case TextureFormat::DXT1_RGBA:
				bc_block_bytes = 8;
				break;
			case TextureFormat::DXT3_RGBA:
			case TextureFormat::DXT5_RGBA:
				bc_block_bytes = 16;
				break;
			default:
				row_pitch = (UINT)getTextureFormatSize(format) * w;
				break;
			}
			if (bc_block_bytes > 0)
			{
				int block_wide = std::max<int>(1, (w + 3) / 4);
				row_pitch = block_wide * bc_block_bytes;
			}


			StagingBuffer textureStagingBuffer;
			{
				const D3D12_SUBRESOURCE_DATA sdata{ data.buffer, row_pitch };
				textureStagingBuffer = createStagingBuffer(context, texture, 0, 1, &sdata);
			}

			{
				const CD3DX12_TEXTURE_COPY_LOCATION destCopyLocation{ texture.Get(), 0 };
				const CD3DX12_TEXTURE_COPY_LOCATION srcCopyLocation{ textureStagingBuffer.buffer.Get(), textureStagingBuffer.layouts[0] };

				context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, 0));
				context->m_commandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
				context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON, 0));
			}

			context->executeCommandList();
			context->waitForGPU();
		}

		void D3D12Texture::CopyTexture(
			D3D12Context* context,
			int dst_layer, int dst_level,
			const backend::Offset3D& dst_offset,
			const backend::Offset3D& dst_extent,
			D3D12Texture* src,
			int src_layer, int src_level,
			const backend::Offset3D& src_offset,
			const backend::Offset3D& src_extent)
		{
			assert(dst_extent.x == src_extent.x);
			assert(dst_extent.y == src_extent.y);
			assert(dst_extent.z == src_extent.z);
		}

		void D3D12Texture::CopyTextureToMemory(
			D3D12Context* context,
			int layer, int level,
			const Offset3D& offset,
			const Offset3D& extent,
			PixelBufferDescriptor& data)
		{
		}

		void D3D12Texture::GenerateMipmaps(D3D12Context* context)
		{
			//assert(width == height);
			//assert(Utility::isPowerOfTwo(width));

			//if (!context->m_mipmapGeneration.rootSignature) {
			//	const CD3DX12_DESCRIPTOR_RANGE1 descriptorRanges[] = {
			//		{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE},
			//		{D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE},
			//	};
			//	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
			//	rootParameters[0].InitAsDescriptorTable(1, &descriptorRanges[0]);
			//	rootParameters[1].InitAsDescriptorTable(1, &descriptorRanges[1]);

			//	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
			//	rootSignatureDesc.Init_1_1(2, rootParameters);
			//	context->m_mipmapGeneration.rootSignature = context->createRootSignature(rootSignatureDesc);
			//}

			//ID3D12PipelineState* pipelineState = nullptr;

			//const D3D12_RESOURCE_DESC desc = texture->GetDesc();
			//if (desc.DepthOrArraySize == 1 && desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
			//	if (!context->m_mipmapGeneration.gammaTexturePipelineState) {
			//		ComPtr<ID3DBlob> computeShader = context->compileShader("shaders/hlsl/downsample.hlsl", "downsample_gamma", "cs_5_0");
			//		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
			//		psoDesc.pRootSignature = context->m_mipmapGeneration.rootSignature.Get();
			//		psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader.Get());
			//		if (FAILED(context->m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&context->m_mipmapGeneration.gammaTexturePipelineState)))) {
			//			throw std::runtime_error("Failed to create compute pipeline state (gamma correct downsample filter)");
			//		}
			//	}
			//	pipelineState = context->m_mipmapGeneration.gammaTexturePipelineState.Get();
			//}
			//else if (desc.DepthOrArraySize > 1 && desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
			//	if (!context->m_mipmapGeneration.arrayTexturePipelineState) {
			//		ComPtr<ID3DBlob> computeShader = context->compileShader("shaders/hlsl/downsample_array.hlsl", "downsample_linear", "cs_5_0");
			//		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
			//		psoDesc.pRootSignature = context->m_mipmapGeneration.rootSignature.Get();
			//		psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader.Get());
			//		if (FAILED(context->m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&context->m_mipmapGeneration.arrayTexturePipelineState)))) {
			//			throw std::runtime_error("Failed to create compute pipeline state (array downsample filter)");
			//		}
			//	}
			//	pipelineState = context->m_mipmapGeneration.arrayTexturePipelineState.Get();
			//}
			//else {
			//	assert(desc.DepthOrArraySize == 1);
			//	if (!context->m_mipmapGeneration.linearTexturePipelineState) {
			//		ComPtr<ID3DBlob> computeShader = context->compileShader("shaders/hlsl/downsample.hlsl", "downsample_linear", "cs_5_0");
			//		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
			//		psoDesc.pRootSignature = context->m_mipmapGeneration.rootSignature.Get();
			//		psoDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader.Get());
			//		if (FAILED(context->m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&context->m_mipmapGeneration.linearTexturePipelineState)))) {
			//			throw std::runtime_error("Failed to create compute pipeline state (linear downsample filter)");
			//		}
			//	}
			//	pipelineState = context->m_mipmapGeneration.linearTexturePipelineState.Get();
			//}

			//DescriptorHeapMark mark(context->m_descHeapCBV_SRV_UAV);

			//D3D12Texture linearTexture = *this;
			//if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
			//	pipelineState = context->m_mipmapGeneration.gammaTexturePipelineState.Get();
			//	D3D12Texture lt(context, filament::backend::SamplerType::SAMPLER_2D, 1, filament::backend::TextureFormat::RGBA8, levels, width, height, 1, (filament::backend::TextureUsage)0);
			//	linearTexture = lt;

			//	context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(linearTexture.texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
			//	context->m_commandList->CopyResource(linearTexture.texture.Get(), texture.Get());
			//	context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(linearTexture.texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));
			//}

			//ID3D12DescriptorHeap* descriptorHeaps[] = {
			//	context->m_descHeapCBV_SRV_UAV.heap.Get()
			//};

			//context->m_commandList->SetComputeRootSignature(context->m_mipmapGeneration.rootSignature.Get());
			//context->m_commandList->SetDescriptorHeaps(1, descriptorHeaps);
			//context->m_commandList->SetPipelineState(pipelineState);


			//std::vector<CD3DX12_RESOURCE_BARRIER> preDispatchBarriers{ desc.DepthOrArraySize };
			//std::vector<CD3DX12_RESOURCE_BARRIER> postDispatchBarriers{ desc.DepthOrArraySize };
			//for (UINT level = 1, levelWidth = width / 2, levelHeight = height / 2; level < levels; ++level, levelWidth /= 2, levelHeight /= 2) {
			//	linearTexture.createTextureSRV(context, desc.DepthOrArraySize > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D, level - 1, 1);
			//	linearTexture.createTextureUAV(context, level);

			//	for (UINT arraySlice = 0; arraySlice < desc.DepthOrArraySize; ++arraySlice) {
			//		const UINT subresourceIndex = D3D12CalcSubresource(level, arraySlice, 0, levels, desc.DepthOrArraySize);
			//		preDispatchBarriers[arraySlice] = CD3DX12_RESOURCE_BARRIER::Transition(linearTexture.texture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, subresourceIndex);
			//		postDispatchBarriers[arraySlice] = CD3DX12_RESOURCE_BARRIER::Transition(linearTexture.texture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, subresourceIndex);
			//	}

			//	context->m_commandList->ResourceBarrier(desc.DepthOrArraySize, preDispatchBarriers.data());
			//	//context->m_commandList->SetComputeRootDescriptorTable(0, linearTexture.srv.gpuHandle);
			//	//context->m_commandList->SetComputeRootDescriptorTable(1, linearTexture.uav.gpuHandle);
			//	context->m_commandList->Dispatch((std::max)(UINT(1), levelWidth / 8), (std::max)(UINT(1), levelHeight / 8), desc.DepthOrArraySize);
			//	context->m_commandList->ResourceBarrier(desc.DepthOrArraySize, postDispatchBarriers.data());
			//}

			//if (texture == linearTexture.texture) {
			//	context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON));
			//}
			//else {
			//	context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
			//	context->m_commandList->CopyResource(texture.Get(), linearTexture.texture.Get());
			//	context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));
			//}
		}

		D3D12VertexBuffer::D3D12VertexBuffer(
			D3D12Context* context,
			uint8_t buffer_count,
			uint8_t attribute_count,
			uint32_t vertex_count,
			AttributeArray attributes,
			BufferUsage usage):
			HwVertexBuffer(buffer_count, attribute_count, vertex_count, attributes),
			VertexBufferRefArray(buffer_count, nullptr),
			usage(usage)
		{
			assert(attribute_count > 0);
			stride = attributes[0].stride;
		}

		D3D12VertexBuffer::~D3D12VertexBuffer()
		{
		}

		void D3D12VertexBuffer::Update(
			D3D12Context* context,
			size_t index,
			const BufferDescriptor& data,
			uint32_t offset)
		{
			assert(index < VertexBufferRefArray.size());
			if (VertexBufferRefArray[index] == nullptr) 
			{
				VertexBufferRefArray[index] = std::make_shared<MD3D12VertexBuffer>();
				D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(data.size, D3D12_RESOURCE_FLAG_NONE);
				auto DefaultBufferAllocator = context->GetDefaultBufferAllocator();
				DefaultBufferAllocator->AllocDefaultResource(ResourceDesc, UPLOAD_RESOURCE_ALIGNMENT, VertexBufferRefArray[index]->ResourceLocation);
				size = offset + data.size;
			}
			//Create upload resource 
			MD3D12ResourceLocation UploadResourceLocation;
			auto UploadBufferAllocator = context->GetUploadBufferAllocator();
			void* MappedData = UploadBufferAllocator->AllocUploadResource(data.size, UPLOAD_RESOURCE_ALIGNMENT, UploadResourceLocation);

			//Copy contents to upload resource
			memcpy(MappedData, data.buffer, data.size);

			//Copy data from upload resource to default resource
			MD3D12Resource* DefaultBuffer = VertexBufferRefArray[index]->ResourceLocation.UnderlyingResource;
			MD3D12Resource* UploadBuffer = UploadResourceLocation.UnderlyingResource;
			// Enqueue upload to the GPU.

			context->TransitionResource(DefaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
			context->CopyBufferRegion(DefaultBuffer, VertexBufferRefArray[index]->ResourceLocation.OffsetFromBaseOfResource, UploadBuffer, UploadResourceLocation.OffsetFromBaseOfResource, data.size);
		}

		D3D12IndexBuffer::D3D12IndexBuffer(
			D3D12Context* context,
			ElementType element_type,
			uint32_t index_count,
			BufferUsage usage):
			HwIndexBuffer((uint8_t) D3D12Driver::getElementTypeSize(element_type), index_count),
			usage(usage)
		{
			indexDataSize = elementSize * index_count;
			indexElement_type = element_type;
		}

		D3D12IndexBuffer::~D3D12IndexBuffer()
		{

		}

		void D3D12IndexBuffer::Update(
			D3D12Context* context,
			const BufferDescriptor& data,
			uint32_t offset)
		{
			// Create GPU resources & initialize view structures.
			if (IndexBufferRef == nullptr) 
			{
				IndexBufferRef = std::make_shared<MD3D12IndexBuffer>();
				//Create default resource
				D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(data.size, D3D12_RESOURCE_FLAG_NONE);
				auto DefaultBufferAllocator = context->GetDefaultBufferAllocator();
				DefaultBufferAllocator->AllocDefaultResource(ResourceDesc, UPLOAD_RESOURCE_ALIGNMENT, IndexBufferRef->ResourceLocation);
				indexDataSize = data.size;
				if (indexElement_type == ElementType::USHORT) {
					indexFormat = DXGI_FORMAT_R16_UINT;
				}
				else
				{
					indexFormat = DXGI_FORMAT_R32_UINT;
				}
			}

			//Create upload resource 
			MD3D12ResourceLocation UploadResourceLocation;
			auto UploadBufferAllocator = context->GetUploadBufferAllocator();
			void* MappedData = UploadBufferAllocator->AllocUploadResource(data.size, UPLOAD_RESOURCE_ALIGNMENT, UploadResourceLocation);

			//Copy contents to upload resource
			memcpy(MappedData, data.buffer, data.size);

			//Copy data from upload resource to default resource
			MD3D12Resource* DefaultBuffer = IndexBufferRef->ResourceLocation.UnderlyingResource;
			MD3D12Resource* UploadBuffer = UploadResourceLocation.UnderlyingResource;
			// Enqueue upload to the GPU.

			context->TransitionResource(DefaultBuffer, D3D12_RESOURCE_STATE_COPY_DEST);
			context->CopyBufferRegion(DefaultBuffer, IndexBufferRef->ResourceLocation.OffsetFromBaseOfResource, UploadBuffer, UploadResourceLocation.OffsetFromBaseOfResource, data.size);
		}

		D3D12RenderPrimitive::D3D12RenderPrimitive(D3D12Context* context)
		{
		
		}

		D3D12RenderPrimitive::~D3D12RenderPrimitive()
		{
			
		}

		void D3D12RenderPrimitive::SetBuffer(
			D3D12Context* context,
			Handle<HwVertexBuffer> vbh,
			Handle<HwIndexBuffer> ibh,
			uint32_t enabled_attributes,
			uint32_t vertex_count)
		{
			this->vertex_buffer = vbh;
			this->index_buffer = ibh;
			this->enabled_attributes = enabled_attributes;
			this->maxVertexCount = vertex_count;
		}

		void D3D12RenderPrimitive::SetRange(
			D3D12Context* context,
			PrimitiveType pt,
			uint32_t offset,
			uint32_t min_index,
			uint32_t max_index,
			uint32_t count)
		{
			this->offset = offset;
			this->minIndex = min_index;
			this->maxIndex = max_index;
			this->count = count;
			this->type = pt;
		}
	}
}