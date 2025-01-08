
#include "D3D12Driver.h"
#include "D3D12DriverFactory.h"
#include "CommandStreamDispatcher.h"
#include "D3D12Context.h"
#include "D3D12Handles.h"
#include "D3D12DescriptorCache.h"
#include <utils/unwindows.h>

namespace filament
{
	namespace backend
	{
		Driver* D3D12DriverFactory::create(backend::D3D12Platform* platform)
		{
			return D3D12Driver::create(platform);
		}

		Driver* D3D12Driver::create(backend::D3D12Platform* platform)
		{
			assert(platform);
			return new D3D12Driver(platform);
		}

		D3D12Driver::D3D12Driver(backend::D3D12Platform* platform) noexcept:
			DriverBase(new ConcreteDispatcher<D3D12Driver>()),
			m_platform(*platform),
			m_context(new D3D12Context())
		{
		
		}

		D3D12Driver::~D3D12Driver() noexcept
		{
			delete m_context;
		}

		ShaderModel D3D12Driver::getShaderModel() const noexcept
		{
			return ShaderModel::GL_CORE_41;
		}

		void D3D12Driver::setPresentationTime(int64_t monotonic_clock_ns)
		{

		}

		void D3D12Driver::beginFrame(int64_t monotonic_clock_ns, uint32_t frame_id)
		{
			//Indicate a state transition on the resource usage.
			SwapChainBuffer& scb = m_context->current_swap_chain->m_backbuffers[m_context->m_frameIndex];
			m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scb.buffer.Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		}

		void D3D12Driver::endFrame(uint32_t frame_id)
		{
			// Clean memory allocations
			m_context->GetUploadBufferAllocator()->CleanUpAllocations();

			m_context->GetDefaultBufferAllocator()->CleanUpAllocations();

			m_context->GetTextureResourceAllocator()->CleanUpAllocations();

			m_context->GetDescriptorCache()->Reset();
		}

		void D3D12Driver::flush(int dummy)
		{

		}

		void D3D12Driver::createVertexBufferR(
			Handle<HwVertexBuffer> vbh,
			uint8_t buffer_count,
			uint8_t attribute_count,
			uint32_t vertex_count,
			AttributeArray attributes,
			BufferUsage usage)
		{
			construct_handle<D3D12VertexBuffer>(
				m_handle_map,
				vbh,
				m_context,
				buffer_count,
				attribute_count,
				vertex_count,
				attributes,
				usage);
		}

		void D3D12Driver::createIndexBufferR(
			Handle<HwIndexBuffer> ibh,
			ElementType element_type,
			uint32_t index_count,
			BufferUsage usage)
		{
			construct_handle<D3D12IndexBuffer>(
				m_handle_map,
				ibh,
				m_context,
				element_type,
				index_count,
				usage);
		}

		void D3D12Driver::createTextureR(
			Handle<HwTexture> th,
			SamplerType target,
			uint8_t levels,
			TextureFormat format,
			uint8_t samples,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			TextureUsage usage)
		{
			construct_handle<D3D12Texture>(m_handle_map, th, m_context, target, levels, format, samples, width, height, depth, usage);
		}

		void D3D12Driver::createSamplerGroupR(Handle<HwSamplerGroup> sgh, size_t size)
		{
			construct_handle<D3D12SamplerGroup>(m_handle_map, sgh, m_context, size);
		}

		void D3D12Driver::createUniformBufferR(Handle<HwUniformBuffer> ubh, size_t size, BufferUsage usage)
		{
			construct_handle<D3D12UniformBuffer>(m_handle_map, ubh, m_context, size, usage);
		}

		void D3D12Driver::createRenderPrimitiveR(Handle<HwRenderPrimitive> rph, int dummy)
		{
			construct_handle<D3D12RenderPrimitive>(m_handle_map, rph, m_context);
		}

		void D3D12Driver::createProgramR(Handle<HwProgram> ph, Program&& program)
		{
			construct_handle<D3D12Program>(m_handle_map, ph, m_context, std::move(program));
		}

		void D3D12Driver::createDefaultRenderTargetR(Handle<HwRenderTarget> rth, int dummy)
		{
			construct_handle<D3D12RenderTarget>(m_handle_map, rth, m_context);
		}

		void D3D12Driver::createRenderTargetR(
			Handle<HwRenderTarget> rth,
			TargetBufferFlags flags,
			uint32_t width,
			uint32_t height,
			uint8_t samples,
			TargetBufferInfo color,
			TargetBufferInfo depth,
			TargetBufferInfo stencil)
		{
			auto render_target = construct_handle<D3D12RenderTarget>(
				m_handle_map,
				rth,
				m_context,
				flags,
				width,
				height,
				samples,
				color,
				depth,
				stencil);

			//assert(samples == 1);

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.SampleDesc.Count = samples;

			if (flags & TargetBufferFlags::COLOR)
			{
				auto color_texture = handle_cast<D3D12Texture>(m_handle_map, color.handle);
				desc.Format = m_context->GetTextureViewFormat(color_texture->format);
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

				const float optimizedClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				if (FAILED(m_context->m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					&CD3DX12_CLEAR_VALUE{ desc.Format, optimizedClearColor },
					IID_PPV_ARGS(&render_target->colorTexture))))
				{
					throw std::runtime_error("Failed to create FrameBuffer color texture");
				}

				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = desc.Format;
				rtvDesc.ViewDimension = (samples > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;

				//render_target->rtv = m_context->m_descHeapRTV.alloc();
				//m_context->m_device->CreateRenderTargetView(render_target->colorTexture.Get(), &rtvDesc, render_target->rtv.cpuHandle);

				if (samples <= 1) {
					D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
					srvDesc.Format = desc.Format;
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					srvDesc.Texture2D.MostDetailedMip = 0;
					srvDesc.Texture2D.MipLevels = 1;

					//render_target->srv = m_context->m_descHeapCBV_SRV_UAV.alloc();
					//m_context->m_device->CreateShaderResourceView(render_target->colorTexture.Get(), &srvDesc, render_target->srv.cpuHandle);
				}
			}
			if ((flags & TargetBufferFlags::DEPTH) ||
				(flags & TargetBufferFlags::STENCIL))
			{
				D3D12Texture* texture = nullptr;
				UINT level = 0;

				if (flags & TargetBufferFlags::DEPTH)
				{
					texture = handle_cast<D3D12Texture>(m_handle_map, depth.handle);
					level = depth.level;
				}
				else if (flags & TargetBufferFlags::STENCIL)
				{
					texture = handle_cast<D3D12Texture>(m_handle_map, stencil.handle);
					level = stencil.level;
				}

				D3D12_DEPTH_STENCIL_VIEW_DESC depth_desc = { };
				depth_desc.Format = m_context->GetDepthViewFormat(texture->format);
				depth_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				depth_desc.Texture2D.MipSlice = level;

				desc.Format = depth_desc.Format;
				desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

				if (FAILED(m_context->m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_DEPTH_WRITE,
					&CD3DX12_CLEAR_VALUE{desc.Format, 1.0f, 0 },
					IID_PPV_ARGS(&render_target->depthStencilTexture))))
				{
					throw std::runtime_error("Failed to create RenderTarget depth-stencil texture");
				}

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = desc.Format;
				dsvDesc.ViewDimension = (samples > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;

				//render_target->dsv = m_context->m_descHeapDSV.alloc();
				//m_context->m_device->CreateDepthStencilView(render_target->depthStencilTexture.Get(), &dsvDesc, render_target->dsv.cpuHandle);
			}
		}

		void D3D12Driver::createFenceR(Handle<HwFence> fh, int dummy)
		{

		}

		void D3D12Driver::createSwapChainR(Handle<HwSwapChain> sch, void* native_window, uint64_t flags)
		{
			construct_handle<D3D12SwapChain>(m_handle_map, sch, m_context, native_window);
		}

		void D3D12Driver::createStreamFromTextureIdR(
			Handle<HwStream> stream,
			intptr_t externalTextureId,
			uint32_t width,
			uint32_t height)
		{

		}

		Handle<HwVertexBuffer> D3D12Driver::createVertexBufferS() noexcept
		{
			return alloc_handle<D3D12VertexBuffer, HwVertexBuffer>();
		}

		Handle<HwIndexBuffer> D3D12Driver::createIndexBufferS() noexcept
		{
			return alloc_handle<D3D12IndexBuffer, HwIndexBuffer>();
		}

		Handle<HwTexture> D3D12Driver::createTextureS() noexcept
		{
			return alloc_handle<D3D12Texture, HwTexture>();
		}

		Handle<HwSamplerGroup> D3D12Driver::createSamplerGroupS() noexcept
		{
			return alloc_handle<D3D12SamplerGroup, HwSamplerGroup>();
		}

		Handle<HwUniformBuffer> D3D12Driver::createUniformBufferS() noexcept
		{
			return alloc_handle<D3D12UniformBuffer, HwUniformBuffer>();
		}

		Handle<HwRenderPrimitive> D3D12Driver::createRenderPrimitiveS() noexcept
		{
			return alloc_handle<D3D12RenderPrimitive, HwRenderPrimitive>();
		}

		Handle<HwProgram> D3D12Driver::createProgramS() noexcept
		{
			return alloc_handle<D3D12Program, HwProgram>();
		}

		Handle<HwRenderTarget> D3D12Driver::createDefaultRenderTargetS() noexcept
		{
			return alloc_handle<D3D12RenderTarget, HwRenderTarget>();
		}

		Handle<HwRenderTarget> D3D12Driver::createRenderTargetS() noexcept
		{
			return alloc_handle<D3D12RenderTarget, HwRenderTarget>();
		}

		Handle<HwFence> D3D12Driver::createFenceS() noexcept
		{
			return Handle<HwFence>();
		}

		Handle<HwSwapChain> D3D12Driver::createSwapChainS() noexcept
		{
			return alloc_handle<D3D12SwapChain, HwSwapChain>();
		}

		Handle<HwStream> D3D12Driver::createStreamFromTextureIdS() noexcept
		{
			return Handle<HwStream>();
		}

		void D3D12Driver::destroyVertexBuffer(Handle<HwVertexBuffer> vbh)
		{
			m_context->waitForGPU();
			destruct_handle<D3D12VertexBuffer>(m_handle_map, vbh);
		}

		void D3D12Driver::destroyIndexBuffer(Handle<HwIndexBuffer> ibh)
		{
			m_context->waitForGPU();
			destruct_handle<D3D12IndexBuffer>(m_handle_map, ibh);
		}

		void D3D12Driver::destroyRenderPrimitive(Handle<HwRenderPrimitive> rph)
		{
			m_context->waitForGPU();
			destruct_handle<D3D12RenderPrimitive>(m_handle_map, rph);
		}

		void D3D12Driver::destroyProgram(Handle<HwProgram> ph)
		{
			destruct_handle<D3D12Program>(m_handle_map, ph);
		}

		void D3D12Driver::destroySamplerGroup(Handle<HwSamplerGroup> sgh)
		{
			destruct_handle<D3D12SamplerGroup>(m_handle_map, sgh);
		}

		void D3D12Driver::destroyUniformBuffer(Handle<HwUniformBuffer> ubh)
		{
			destruct_handle<D3D12UniformBuffer>(m_handle_map, ubh);
		}

		void D3D12Driver::destroyTexture(Handle<HwTexture> th)
		{
			destruct_handle<D3D12Texture>(m_handle_map, th);
		}

		void D3D12Driver::destroyRenderTarget(Handle<HwRenderTarget> rth)
		{
			m_context->waitForGPU();
			destruct_handle<D3D12RenderTarget>(m_handle_map, rth);
		}

		void D3D12Driver::destroySwapChain(Handle<HwSwapChain> sch)
		{
			m_context->waitForGPU();
			destruct_handle<D3D12SwapChain>(m_handle_map, sch);
		}

		void D3D12Driver::destroyStream(Handle<HwStream> sh)
		{

		}

		void D3D12Driver::terminate()
		{

		}

		Handle<HwStream> D3D12Driver::createStream(void* stream)
		{
			return Handle<HwStream>();
		}

		void D3D12Driver::setStreamDimensions(Handle<HwStream> stream, uint32_t width, uint32_t height)
		{

		}

		int64_t D3D12Driver::getStreamTimestamp(Handle<HwStream> stream)
		{
			return 0;
		}

		void D3D12Driver::updateStreams(backend::DriverApi* driver)
		{

		}

		void D3D12Driver::destroyFence(Handle<HwFence> fh)
		{

		}

		FenceStatus D3D12Driver::wait(Handle<HwFence> fh, uint64_t timeout)
		{
			return FenceStatus::ERROR;
		}

		bool D3D12Driver::isTextureFormatSupported(TextureFormat format)
		{
			UINT support = 0;
			D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = { m_context->GetTextureFormat(format) };
			if (SUCCEEDED(m_context->m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo))))
			{
				if (formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D)
				{
					formatInfo = { m_context->GetTextureViewFormat(format) };
					if (SUCCEEDED(m_context->m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo))))
					{
						if (formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
							return true;
					}
				}
			}
			return false;
		}

		bool D3D12Driver::isRenderTargetFormatSupported(TextureFormat format)
		{
			return this->isTextureFormatSupported(format);
		}

		bool D3D12Driver::isFrameTimeSupported()
		{
			return false;
		}

		void D3D12Driver::updateVertexBuffer(
			Handle<HwVertexBuffer> vbh,
			size_t index,
			BufferDescriptor&& data,
			uint32_t offset)
		{
			auto buffer = handle_cast<D3D12VertexBuffer>(m_handle_map, vbh);
			buffer->Update(m_context, index, data, offset);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateIndexBuffer(
			Handle<HwIndexBuffer> ibh,
			BufferDescriptor&& data,
			uint32_t offset)
		{
			auto buffer = handle_cast<D3D12IndexBuffer>(m_handle_map, ibh);
			buffer->Update(m_context, data, offset);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateTexture(
			Handle<HwTexture> th,
			int layer, int level,
			int x, int y,
			int w, int h,
			PixelBufferDescriptor&& data)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->UpdateTexture(
				m_context,
				layer, level,
				x, y,
				w, h,
				data);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::update2DImage(
			Handle<HwTexture> th,
			uint32_t level,
			uint32_t x, uint32_t y,
			uint32_t width, uint32_t height,
			PixelBufferDescriptor&& data)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->UpdateTexture(
				m_context,
				0, level,
				x, y,
				width, height,
				data);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateCubeImage(
			Handle<HwTexture> th,
			uint32_t level,
			PixelBufferDescriptor&& data,
			FaceOffsets face_offsets)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			for (int i = 0; i < 6; ++i)
			{
				auto buffer = static_cast<uint8_t*>(data.buffer) + face_offsets[i];
				if (data.type == PixelDataType::COMPRESSED)
				{
					texture->UpdateTexture(
						m_context,
						i, level,
						0, 0,
						texture->width >> level, texture->height >> level,
						PixelBufferDescriptor(buffer, data.size / 6, data.compressedFormat, data.imageSize, nullptr));
				}
				else
				{
					texture->UpdateTexture(
						m_context,
						i, level,
						0, 0,
						texture->width >> level, texture->height >> level,
						PixelBufferDescriptor(buffer, data.size / 6, data.format, data.type));
				}
			}
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::copyTexture(
			Handle<HwTexture> th_dst, int dst_layer, int dst_level,
			Offset3D dst_offset,
			Offset3D dst_extent,
			Handle<HwTexture> th_src, int src_layer, int src_level,
			Offset3D src_offset,
			Offset3D src_extent,
			SamplerMagFilter blit_filter)
		{
			auto dst = handle_cast<D3D12Texture>(m_handle_map, th_dst);
			auto src = handle_cast<D3D12Texture>(m_handle_map, th_src);
			dst->CopyTexture(
				m_context,
				dst_layer, dst_level,
				dst_offset, dst_extent,
				src,
				src_layer, src_level,
				src_offset, src_extent);
		}

		void D3D12Driver::copyTextureToMemory(
			Handle<HwTexture> th,
			int layer, int level,
			Offset3D offset,
			Offset3D extent,
			PixelBufferDescriptor&& p)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->CopyTextureToMemory(
				m_context,
				layer, level,
				offset, extent,
				p);
			this->scheduleDestroy(std::move(p));
		}

		void D3D12Driver::setupExternalImage(void* image)
		{

		}

		void D3D12Driver::cancelExternalImage(void* image)
		{

		}

		void D3D12Driver::setExternalImage(Handle<HwTexture> th, void* image)
		{

		}

		void D3D12Driver::setExternalStream(Handle<HwTexture> th, Handle<HwStream> sh)
		{

		}

		void D3D12Driver::generateMipmaps(Handle<HwTexture> th)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->GenerateMipmaps(m_context);
		}

		bool D3D12Driver::canGenerateMipmaps()
		{
			return true;
		}

		void D3D12Driver::loadUniformBuffer(Handle<HwUniformBuffer> ubh, BufferDescriptor&& data)
		{
			auto uniform_buffer = handle_cast<D3D12UniformBuffer>(m_handle_map, ubh);
			uniform_buffer->Load(m_context, data);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateSamplerGroup(Handle<HwSamplerGroup> sgh, SamplerGroup&& sg)
		{
			auto sampler_group = handle_cast<D3D12SamplerGroup>(m_handle_map, sgh);
			sampler_group->Update(m_context, std::move(sg));
		}

		void D3D12Driver::beginRenderPass(
			Handle<HwRenderTarget> rth,
			const RenderPassParams& params)
		{
			auto render_target = handle_cast<D3D12RenderTarget>(m_handle_map, rth);
			m_context->current_render_target = render_target;
			m_context->current_render_pass_flags = params.flags;

			FrameBuffer fb;

			uint32_t target_height = 0;

			// set render target
			if (render_target->default_render_target)
			{
				UINT frameIndex = m_context->m_frameIndex;
				SwapChainBuffer& scb = m_context->current_swap_chain->m_backbuffers[frameIndex];
				fb.colorTexture = scb.buffer;
				fb.rtv.cpuHandle = scb.RTV->GetDescriptorHandle();
				
				if (render_target->depthStencilTexture == nullptr)
				{
					DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = { };
					m_context->current_swap_chain->m_swapChain->GetDesc1(&swap_chain_desc);

					render_target->CreateDepth(
						m_context,
						DXGI_FORMAT_D24_UNORM_S8_UINT, 
						1,
						swap_chain_desc.Width,
						swap_chain_desc.Height);
					render_target->height = swap_chain_desc.Height;
				}

				fb.depthStencilTexture = render_target->depthStencilTexture;
				fb.dsv.cpuHandle = render_target->DSV->GetDescriptorHandle();
				target_height = render_target->height;
				// Indicate a state transition on the resource usage.
			}
			else
			{
				fb.colorTexture = render_target->colorTexture;
				fb.rtv.cpuHandle = render_target->SRV->GetDescriptorHandle();
				fb.depthStencilTexture = render_target->depthStencilTexture;
				fb.dsv.cpuHandle = render_target->DSV->GetDescriptorHandle();
				target_height = render_target->height;
				m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(fb.colorTexture.Get(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
			}

			if (fb.colorTexture)
			{
				m_context->m_commandList->OMSetRenderTargets(1, &fb.rtv.cpuHandle, false, &fb.dsv.cpuHandle);
			}
			else
			{
				m_context->m_commandList->OMSetRenderTargets(0, nullptr, false, &fb.dsv.cpuHandle);
			}

			// set viewport
			D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(
				(float) params.viewport.left,
				(float) (target_height - (params.viewport.bottom + params.viewport.height)),
				(float) params.viewport.width,
				(float) params.viewport.height
			);
			m_context->m_commandList->RSSetViewports(1, &viewport);

			// clear
			if (params.flags.clear & filament::backend::TargetBufferFlags::COLOR)
			{
				if (fb.colorTexture)
				{
					m_context->m_commandList->ClearRenderTargetView(fb.rtv.cpuHandle, (float*) &params.clearColor, 0, nullptr);
				}
			}
			if (params.flags.clear & filament::backend::TargetBufferFlags::DEPTH ||
				params.flags.clear & filament::backend::TargetBufferFlags::STENCIL)
			{
				D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;
				if (params.flags.clear & filament::backend::TargetBufferFlags::DEPTH)
				{
					flags |= D3D12_CLEAR_FLAG_DEPTH;
				}
				if (params.flags.clear & filament::backend::TargetBufferFlags::STENCIL)
				{
					flags |= D3D12_CLEAR_FLAG_STENCIL;
				}

				if (fb.depthStencilTexture)
				{
					m_context->m_commandList->ClearDepthStencilView(fb.dsv.cpuHandle, flags, 1.0f, 0, 0, nullptr);
				}
			}
			m_context->m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		void D3D12Driver::endRenderPass(int dummy)
		{
			m_context->current_render_target = nullptr;
			m_context->current_render_pass_flags = { };

			for (size_t i = 0; i < m_context->uniform_buffer_bindings.size(); ++i)
			{
				m_context->uniform_buffer_bindings[i] = { };
			}

			for (size_t i = 0; i < m_context->sampler_group_binding.size(); ++i)
			{
				m_context->sampler_group_binding[i].sampler_group = SamplerGroupHandle();
			}
		}

		void D3D12Driver::discardSubRenderTargetBuffers(
			Handle<HwRenderTarget> rth,
			TargetBufferFlags targetBufferFlags,
			uint32_t left,
			uint32_t bottom,
			uint32_t width,
			uint32_t height)
		{

		}

		void D3D12Driver::setRenderPrimitiveBuffer(
			Handle<HwRenderPrimitive> rph,
			Handle<HwVertexBuffer> vbh,
			Handle<HwIndexBuffer> ibh,
			uint32_t enabled_attributes)
		{
			auto primitive = handle_cast<D3D12RenderPrimitive>(m_handle_map, rph);
			auto vertex_buffer = handle_cast<D3D12VertexBuffer>(m_handle_map, vbh);
			primitive->SetBuffer(m_context, vbh, ibh, enabled_attributes, vertex_buffer->vertexCount);
		}

		void D3D12Driver::setRenderPrimitiveRange(
			Handle<HwRenderPrimitive> rph,
			PrimitiveType pt,
			uint32_t offset,
			uint32_t min_index,
			uint32_t max_index,
			uint32_t count)
		{
			auto primitive = handle_cast<D3D12RenderPrimitive>(m_handle_map, rph);
			primitive->SetRange(m_context, pt, offset, min_index, max_index, count);
		}

		void D3D12Driver::setViewportScissor(
			int32_t left,
			int32_t bottom,
			uint32_t width,
			uint32_t height)
		{
			CD3DX12_RECT rect(
				(LONG)left,
				(LONG)(m_context->current_render_target->height - (bottom + height)),
				(LONG)left + width,
				(LONG)bottom + height);
			m_context->m_commandList->RSSetScissorRects(1, &rect);
		}

		void D3D12Driver::makeCurrent(Handle<HwSwapChain> sch_draw, Handle<HwSwapChain> sch_read)
		{
			auto swap_chain = handle_cast<D3D12SwapChain>(m_handle_map, sch_draw);
			if (m_context->current_swap_chain != swap_chain)
			{
				if (m_context->current_swap_chain == nullptr) {
					m_context->executeCommandList(false);
					m_context->waitForGPU();
				}
				m_context->current_swap_chain = swap_chain;
			}

			ID3D12CommandAllocator* commandAllocator = m_context->m_commandAllocators[m_context->m_frameIndex].Get();
			commandAllocator->Reset();
			m_context->m_commandList->Reset(commandAllocator, nullptr);
		}

		void D3D12Driver::commit(Handle<HwSwapChain> sch)
		{
			//Indicate a state transition on the resource usage.
			SwapChainBuffer& scb = m_context->current_swap_chain->m_backbuffers[m_context->m_frameIndex];
			m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scb.buffer.Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

			m_context->executeCommandList(false);
			auto swap_chain = handle_cast<D3D12SwapChain>(m_handle_map, sch);
			swap_chain->m_swapChain->Present(1, 0);

			const UINT64 prevFrameFenceValue = m_context->m_fenceValues[m_context->m_frameIndex];
			m_context->m_commandQueue->Signal(m_context->m_fence.Get(), prevFrameFenceValue);
			m_context->m_frameIndex = swap_chain->m_swapChain->GetCurrentBackBufferIndex();
			UINT64& currentFrameFenceValue = m_context->m_fenceValues[m_context->m_frameIndex];

			if (m_context->m_fence->GetCompletedValue() < currentFrameFenceValue) {
				m_context->m_fence->SetEventOnCompletion(currentFrameFenceValue, m_context->m_fenceCompletionEvent);
				WaitForSingleObjectEx(m_context->m_fenceCompletionEvent, INFINITE, FALSE);
			}
			currentFrameFenceValue = prevFrameFenceValue + 1;
		}

		void D3D12Driver::bindUniformBuffer(size_t index, Handle<HwUniformBuffer> ubh)
		{
			auto uniform_buffer = handle_cast<D3D12UniformBuffer>(m_handle_map, ubh);

			assert(index < m_context->uniform_buffer_bindings.size());
			
			m_context->uniform_buffer_bindings[index].ConstantBufferRef = uniform_buffer->cb;
			//m_context->uniform_buffer_bindings[index].cbv = uniform_buffer->cbv;
			//m_context->uniform_buffer_bindings[index].updata = uniform_buffer->updata;
			m_context->uniform_buffer_bindings[index].offset = 0;
			m_context->uniform_buffer_bindings[index].size = uniform_buffer->size;
		}

		void D3D12Driver::bindUniformBufferRange(
			size_t index,
			Handle<HwUniformBuffer> ubh,
			size_t offset,
			size_t size)
		{
			auto uniform_buffer = handle_cast<D3D12UniformBuffer>(m_handle_map, ubh);

			assert(index < m_context->uniform_buffer_bindings.size());
			assert(offset < uniform_buffer->size);
			assert(offset + size <= uniform_buffer->size);

			m_context->uniform_buffer_bindings[index].offset = offset;
			m_context->uniform_buffer_bindings[index].size = size;
		}

		void D3D12Driver::setUniformVector(
			backend::ProgramHandle ph,
			std::string name,
			size_t count,
			backend::BufferDescriptor&& data)
		{

		}

		void D3D12Driver::setUniformMatrix(
			backend::ProgramHandle ph,
			std::string name,
			size_t count,
			backend::BufferDescriptor&& data)
		{

		}

		void D3D12Driver::bindSamplers(size_t index, Handle<HwSamplerGroup> sgh)
		{
			auto sampler_group = handle_cast<D3D12SamplerGroup>(m_handle_map, sgh);

			assert(index < m_context->sampler_group_binding.size());

			m_context->sampler_group_binding[index].sampler_group = sgh;
		}

		void D3D12Driver::insertEventMarker(const char* string, size_t len)
		{

		}

		void D3D12Driver::pushGroupMarker(const char* string, size_t len)
		{

		}

		void D3D12Driver::popGroupMarker(int dummy)
		{

		}

		void D3D12Driver::readPixels(
			Handle<HwRenderTarget> src,
			uint32_t x,
			uint32_t y,
			uint32_t width,
			uint32_t height,
			PixelBufferDescriptor&& data)
		{

		}

		void D3D12Driver::readStreamPixels(
			Handle<HwStream> sh,
			uint32_t x,
			uint32_t y,
			uint32_t width,
			uint32_t height,
			PixelBufferDescriptor&& data)
		{

		}

		void D3D12Driver::blit(
			TargetBufferFlags buffers,
			Handle<HwRenderTarget> dst,
			backend::Viewport dstRect,
			Handle<HwRenderTarget> src,
			backend::Viewport srcRect,
			SamplerMagFilter filter)
		{
			
		}

		void D3D12Driver::draw(backend::PipelineState ps, Handle<HwRenderPrimitive> rph)
		{
			auto program = handle_cast<D3D12Program>(m_handle_map, ps.program);
			auto primitive = handle_cast<D3D12RenderPrimitive>(m_handle_map, rph);
			auto vertex_buffer = handle_cast<D3D12VertexBuffer>(m_handle_map, primitive->vertex_buffer);
			auto index_buffer = handle_cast<D3D12IndexBuffer>(m_handle_map, primitive->index_buffer);

			auto DescriptorCache = m_context->GetDescriptorCache();
			if (program->meshInputLayout.size() == 0)
			{
				auto get_format = [](ElementType type) {
					switch (type)
					{
					case ElementType::FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
					case ElementType::FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
					case ElementType::FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
					case ElementType::UBYTE4: return DXGI_FORMAT_R8G8B8A8_UINT;
					default: assert(false); return DXGI_FORMAT_UNKNOWN;
					}
				};
				for (size_t i = 0; i < vertex_buffer->attributes.size(); ++i)
				{
					if (primitive->enabled_attributes & (1 << i))
					{
						const auto& attribute = vertex_buffer->attributes[i];
						D3D12_INPUT_ELEMENT_DESC desc = { };
						desc.SemanticName = Semantics[attribute.Semantic].SemanticName;
						desc.SemanticIndex = Semantics[attribute.Semantic].SemanticIndex;
						desc.Format = get_format(attribute.type);
						desc.InputSlot = (UINT)0;
						desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
						desc.AlignedByteOffset = attribute.offset;
						program->meshInputLayout.push_back(desc);
					}
				}
			}
			m_context->m_commandList->SetPipelineState(program->GetPSO(m_context, ps).Get());
			m_context->m_commandList->SetGraphicsRootSignature(program->m_RootSignature.Get());
			UINT indexTable = 0;
			//ID3D12DescriptorHeap* descriptorHeaps[] = {
			//	m_context->m_descHeapCBV_SRV_UAV.heap.Get()
			//};
			//m_context->m_commandList->SetDescriptorHeaps(1, descriptorHeaps);
			auto CacheCbvSrvUavDescriptorHeap = m_context->GetDescriptorCache()->GetCacheCbvSrvUavDescriptorHeap();
			ID3D12DescriptorHeap* DescriptorHeaps[] = { CacheCbvSrvUavDescriptorHeap.Get() };
			m_context->m_commandList->SetDescriptorHeaps(1, DescriptorHeaps);

			if (program->vertex_binary)
			{
				for (size_t i = 0; i < program->vert_cbv.size(); ++i)
				{
					int bindpoint = program->vert_cbv[i].BindPoint;
					//m_context->m_commandList->SetGraphicsRootDescriptorTable(program->vert_cbv_map[bindpoint], m_context->uniform_buffer_bindings[bindpoint].cbv.gpuHandle);
					m_context->m_commandList->SetGraphicsRootConstantBufferView(program->vert_cbv_map[bindpoint], m_context->uniform_buffer_bindings[bindpoint].ConstantBufferRef->ResourceLocation.GPUVirtualAddress);
				}

                const auto& samplers = program->info.getSamplerGroupInfo();
                for (size_t i = 0; i < samplers.size(); ++i)
                {
                    if (i == 2 || i == 3) // vs
                    {
                        if (m_context->sampler_group_binding[i].sampler_group)
                        {
                            auto sampler_group = handle_cast<D3D12SamplerGroup>(m_handle_map, m_context->sampler_group_binding[i].sampler_group);

                            for (int j = 0; j < samplers[i].size(); ++j)
                            {
                                auto& s = sampler_group->sb->getSamplers()[j];

                                if (s.t)
                                {
                                    auto texture = handle_const_cast<D3D12Texture>(m_handle_map, s.t);
									std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SrcDescriptors;
									SrcDescriptors.push_back(texture->SRV->GetDescriptorHandle());
									auto GpuDescriptorHandle = DescriptorCache->AppendCbvSrvUavDescriptors(SrcDescriptors);
									//m_context->m_commandList->SetGraphicsRootDescriptorTable(program->vert_srv_map[samplers[i][j].binding], GpuDescriptorHandle);
                                }
                            }
                        }
                    }
                }
			}
			
			if (program->pixel_binary)
			{
				for (size_t i = 0; i < program->frag_cbv.size(); ++i)
				{
					int bindpoint = program->frag_cbv[i].BindPoint;
					//m_context->m_commandList->SetGraphicsRootDescriptorTable(program->frag_cbv_map[bindpoint], m_context->uniform_buffer_bindings[bindpoint].cbv.gpuHandle);
					m_context->m_commandList->SetGraphicsRootConstantBufferView(program->frag_cbv_map[bindpoint], m_context->uniform_buffer_bindings[bindpoint].ConstantBufferRef->ResourceLocation.GPUVirtualAddress);
				}

				const auto& samplers = program->info.getSamplerGroupInfo();
				for (size_t i = 0; i < samplers.size(); ++i)
				{
                    if (i == 4 || i == 6) // ps
                    {
                        if (m_context->sampler_group_binding[i].sampler_group)
                        {
                            auto sampler_group = handle_cast<D3D12SamplerGroup>(m_handle_map, m_context->sampler_group_binding[i].sampler_group);

                            for (int j = 0; j < samplers[i].size(); ++j)
                            {
                                auto& s = sampler_group->sb->getSamplers()[j];

                                if (s.t)
                                {
                                    auto texture = handle_const_cast<D3D12Texture>(m_handle_map, s.t);
									std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SrcDescriptors;
									SrcDescriptors.push_back(texture->SRV->GetDescriptorHandle());
									auto GpuDescriptorHandle = DescriptorCache->AppendCbvSrvUavDescriptors(SrcDescriptors);
									m_context->m_commandList->SetGraphicsRootDescriptorTable(program->frag_srv_map[samplers[i][j].binding], GpuDescriptorHandle);
									//m_context->m_commandList->SetGraphicsRootDescriptorTable(program->frag_srv_map[samplers[i][j].binding], texture->srv.gpuHandle);
                                }
                            }
                        }
                    }
				}
			}

			m_context->SetVertexBuffer(vertex_buffer->VertexBufferRefArray[0], 0, vertex_buffer->stride, vertex_buffer->size);
			m_context->SetIndexBuffer(index_buffer->IndexBufferRef, 0, index_buffer->indexFormat, index_buffer->indexDataSize);
			//m_context->m_commandList->IASetVertexBuffers(0, vertex_buffer->vbvs.size(), &vertex_buffer->vbvs[0]);
			//m_context->m_commandList->IASetIndexBuffer(&index_buffer->ibv);

			m_context->m_commandList->DrawIndexedInstanced(primitive->count, 1, 0, 0, 0);
		}
	}
}
