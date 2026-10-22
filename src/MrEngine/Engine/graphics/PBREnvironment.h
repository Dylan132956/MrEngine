#pragma once

#include "Texture.h"
#include "Material.h"
#include <memory>
#include <string>
#include <vector>

namespace moonriver
{
    // CPU-side IBL precompute (DiligentFX / Khronos split-sum style).
    class PBREnvironment
    {
    public:
        static PBREnvironment& Instance();

        // Build high-contrast procedural sky env + irradiance / prefiltered / BRDF LUT.
        void InitializeProcedural();

        // Load Radiance .hdr equirectangular, convert to cubemap float data, bake IBL.
        // Returns false if the file cannot be loaded.
        bool InitializeFromHDR(const std::string& path);

        // Load a KTX1 RGBA16F/RGBA32F cubemap (e.g. DiligentSamples papermill.ktx) and bake IBL from mip 0.
        bool InitializeFromKTX(const std::string& path);

        // Load a KTX cubemap, mirror its axes by (sx, sy, sz) so that new(d) = old(sx*d.x, sy*d.y, sz*d.z),
        // optionally write the reoriented cubemap to savePath (KTX1 RGBA16F, mip 0), then bake IBL.
        bool InitializeFromKTXReoriented(const std::string& path, float sx, float sy, float sz, const std::string& savePath);

        // Optional: replace env cubemap from an existing cubemap texture, then rebuild IBL.
        // Requires m_envRGBA already populated (e.g. after InitializeFromHDR / procedural).
        void InitializeFromCubemap(const std::shared_ptr<Texture>& envCubemap);

        bool IsReady() const { return m_ready; }

        const std::shared_ptr<Texture>& GetEnvironmentMap() const { return m_environment; }
        const std::shared_ptr<Texture>& GetIrradianceMap() const { return m_irradiance; }
        const std::shared_ptr<Texture>& GetPrefilteredMap() const { return m_prefiltered; }
        const std::shared_ptr<Texture>& GetBRDFLUT() const { return m_brdfLUT; }

        // Push IBL textures onto all PBR materials currently in the scene.
        void ApplyToSceneMaterials();

        // Bind IBL samplers onto one material.
        void ApplyToMaterial(const std::shared_ptr<Material>& material);

        // Release GPU textures while the engine/driver is still alive (call from Engine shutdown).
        void Shutdown();

    private:
        PBREnvironment() = default;

        // When cacheSourcePath is non-empty, baked maps are read from / written to "<cacheSourcePath>.iblcache".
        void BuildFromEnvironment(const std::string& cacheSourcePath = std::string());
        std::shared_ptr<Texture> UploadEnvCubemapRGBA16F();
        void FillProceduralEnvRGBA(int size);
        bool LoadHDREquirectToCubemap(const std::string& path, int cubeSize);
        bool LoadKTXCubemap(const std::string& path);
        bool SaveKTXCubemap(const std::string& path) const;
        void ReorientEnvironment(float sx, float sy, float sz);

        ByteBuffer BakeIrradiance(const std::vector<float>& envRGBA, int envSize, int outSize);
        std::vector<ByteBuffer> BakePrefiltered(const std::vector<float>& envRGBA, int envSize, int outSize, int mipCount);
        ByteBuffer BakeBRDFLUT(int size);
        static std::shared_ptr<Texture> UploadCubemapRGBA32F(const std::vector<ByteBuffer>& mips, int size);

        bool LoadBakeCache(const std::string& sourcePath, ByteBuffer& irradiance, std::vector<ByteBuffer>& prefiltered, ByteBuffer& lut);
        void SaveBakeCache(const std::string& sourcePath, const ByteBuffer& irradiance, const std::vector<ByteBuffer>& prefiltered, const ByteBuffer& lut);

        static void FaceUVToDir(int face, float u, float v, float& x, float& y, float& z);
        static void SampleCubemap(const std::vector<float>& envRGBA, int size, float x, float y, float z, float& r, float& g, float& b);
        static void Hammersley(uint32_t i, uint32_t n, float& u, float& v);
        static void ImportanceSampleGGX(float u1, float u2, float roughness, float Nx, float Ny, float Nz,
            float& Hx, float& Hy, float& Hz);
        static void PackRGBA32F(ByteBuffer& dst, int pixelIndex, float r, float g, float b, float a = 1.0f);

        bool m_ready = false;
        std::shared_ptr<Texture> m_environment;
        std::shared_ptr<Texture> m_irradiance;
        std::shared_ptr<Texture> m_prefiltered;
        std::shared_ptr<Texture> m_brdfLUT;
        std::vector<float> m_envRGBA;
        int m_envSize = 0;
    };
}
