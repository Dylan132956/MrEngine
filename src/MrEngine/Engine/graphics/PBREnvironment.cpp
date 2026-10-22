#include "PBREnvironment.h"
#include "Renderer.h"
#include "math/Mathf.h"
#include "memory/ByteBuffer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_HDR
#include "stb_image.h"

namespace moonriver
{
    static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
    static inline float Saturate(float v) { return std::max(0.0f, std::min(1.0f, v)); }

    PBREnvironment& PBREnvironment::Instance()
    {
        static PBREnvironment inst;
        return inst;
    }

    void PBREnvironment::Shutdown()
    {
        m_environment.reset();
        m_irradiance.reset();
        m_prefiltered.reset();
        m_brdfLUT.reset();
        m_envRGBA.clear();
        m_envSize = 0;
        m_ready = false;
    }

    void PBREnvironment::PackRGBA32F(ByteBuffer& dst, int pixelIndex, float r, float g, float b, float a)
    {
        int o = pixelIndex * 16;
        float rgba[4] = { r, g, b, a };
        std::memcpy(&dst[o], rgba, sizeof(rgba));
    }

    void PBREnvironment::FaceUVToDir(int face, float u, float v, float& x, float& y, float& z)
    {
        switch (face)
        {
        case 0: x = 1;  y = -v; z = -u; break;
        case 1: x = -1; y = -v; z = u;  break;
        case 2: x = u;  y = 1;  z = v;  break;
        case 3: x = u;  y = -1; z = -v; break;
        case 4: x = u;  y = -v; z = 1;  break;
        default:x = -u; y = -v; z = -1; break;
        }
        float len = std::sqrt(x * x + y * y + z * z);
        x /= len; y /= len; z /= len;
    }

    void PBREnvironment::SampleCubemap(const std::vector<float>& envRGBA, int size, float x, float y, float z,
        float& r, float& g, float& b)
    {
        float ax = std::abs(x), ay = std::abs(y), az = std::abs(z);
        int face;
        float uc, vc;
        if (ax >= ay && ax >= az)
        {
            face = x > 0 ? 0 : 1;
            float ma = ax;
            uc = x > 0 ? -z : z;
            vc = -y;
            uc /= ma; vc /= ma;
        }
        else if (ay >= ax && ay >= az)
        {
            face = y > 0 ? 2 : 3;
            float ma = ay;
            uc = x;
            vc = y > 0 ? z : -z;
            uc /= ma; vc /= ma;
        }
        else
        {
            face = z > 0 ? 4 : 5;
            float ma = az;
            uc = z > 0 ? x : -x;
            vc = -y;
            uc /= ma; vc /= ma;
        }

        float u = (uc * 0.5f + 0.5f) * (size - 1);
        float v = (vc * 0.5f + 0.5f) * (size - 1);
        int x0 = (int)u;
        int y0 = (int)v;
        int x1 = std::min(x0 + 1, size - 1);
        int y1 = std::min(y0 + 1, size - 1);
        float tx = u - x0;
        float ty = v - y0;

        auto fetch = [&](int px, int py, float& fr, float& fg, float& fb)
        {
            size_t idx = ((size_t)face * size * size + (size_t)py * size + (size_t)px) * 4;
            fr = envRGBA[idx + 0];
            fg = envRGBA[idx + 1];
            fb = envRGBA[idx + 2];
        };

        float r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
        fetch(x0, y0, r00, g00, b00);
        fetch(x1, y0, r10, g10, b10);
        fetch(x0, y1, r01, g01, b01);
        fetch(x1, y1, r11, g11, b11);

        float r0 = Lerp(r00, r10, tx);
        float g0 = Lerp(g00, g10, tx);
        float b0 = Lerp(b00, b10, tx);
        float r1 = Lerp(r01, r11, tx);
        float g1 = Lerp(g01, g11, tx);
        float b1 = Lerp(b01, b11, tx);
        r = Lerp(r0, r1, ty);
        g = Lerp(g0, g1, ty);
        b = Lerp(b0, b1, ty);
    }

    void PBREnvironment::Hammersley(uint32_t i, uint32_t n, float& u, float& v)
    {
        uint32_t bits = i;
        bits = (bits << 16) | (bits >> 16);
        bits = ((bits & 0x55555555u) << 1) | ((bits & 0xAAAAAAAAu) >> 1);
        bits = ((bits & 0x33333333u) << 2) | ((bits & 0xCCCCCCCCu) >> 2);
        bits = ((bits & 0x0F0F0F0Fu) << 4) | ((bits & 0xF0F0F0F0u) >> 4);
        bits = ((bits & 0x00FF00FFu) << 8) | ((bits & 0xFF00FF00u) >> 8);
        u = (float)i / (float)n;
        v = (float)bits * 2.3283064365386963e-10f;
    }

    void PBREnvironment::ImportanceSampleGGX(float u1, float u2, float roughness,
        float Nx, float Ny, float Nz, float& Hx, float& Hy, float& Hz)
    {
        float a = roughness * roughness;
        float phi = 2.0f * 3.14159265f * u1;
        float cosTheta = std::sqrt((1.0f - u2) / (1.0f + (a * a - 1.0f) * u2));
        float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));

        float Hx_t = std::cos(phi) * sinTheta;
        float Hy_t = std::sin(phi) * sinTheta;
        float Hz_t = cosTheta;

        float upx = 0, upy = 0, upz = 0;
        if (std::abs(Nz) < 0.999f) { upz = 1; }
        else { upx = 1; }

        float Tx = upy * Nz - upz * Ny;
        float Ty = upz * Nx - upx * Nz;
        float Tz = upx * Ny - upy * Nx;
        float tlen = std::sqrt(Tx * Tx + Ty * Ty + Tz * Tz);
        Tx /= tlen; Ty /= tlen; Tz /= tlen;
        float Bx = Ny * Tz - Nz * Ty;
        float By = Nz * Tx - Nx * Tz;
        float Bz = Nx * Ty - Ny * Tx;

        Hx = Tx * Hx_t + Bx * Hy_t + Nx * Hz_t;
        Hy = Ty * Hx_t + By * Hy_t + Ny * Hz_t;
        Hz = Tz * Hx_t + Bz * Hy_t + Nz * Hz_t;
        float hlen = std::sqrt(Hx * Hx + Hy * Hy + Hz * Hz);
        Hx /= hlen; Hy /= hlen; Hz /= hlen;
    }

    void PBREnvironment::FillProceduralEnvRGBA(int size)
    {
        m_envSize = size;
        m_envRGBA.assign((size_t)size * size * 6 * 4, 0.0f);

        for (int face = 0; face < 6; ++face)
        {
            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    float u = (x + 0.5f) / size * 2.0f - 1.0f;
                    float v = (y + 0.5f) / size * 2.0f - 1.0f;
                    float dx, dy, dz;
                    FaceUVToDir(face, u, v, dx, dy, dz);

                    // High-contrast studio: dark walls + bright window + warm floor.
                    float wall = 0.04f;
                    float r = wall, g = wall, b = wall * 1.05f;

                    // Softbox / window on +Z hemisphere
                    float win = Saturate(dz * 0.85f + 0.15f);
                    win = std::pow(win, 4.0f);
                    r += win * 8.0f;
                    g += win * 7.6f;
                    b += win * 7.0f;

                    // Ceiling bounce
                    float ceil = Saturate(dy);
                    r += ceil * 0.35f;
                    g += ceil * 0.38f;
                    b += ceil * 0.42f;

                    // Warm ground
                    float ground = Saturate(-dy);
                    r += ground * 0.55f;
                    g += ground * 0.35f;
                    b += ground * 0.18f;

                    // Hot key light lobe
                    float sunDx = -0.25f, sunDy = 0.55f, sunDz = 0.75f;
                    float sl = std::sqrt(sunDx * sunDx + sunDy * sunDy + sunDz * sunDz);
                    sunDx /= sl; sunDy /= sl; sunDz /= sl;
                    float nd = std::max(0.0f, dx * sunDx + dy * sunDy + dz * sunDz);
                    float sun = std::pow(nd, 128.0f) * 25.0f;
                    r += sun; g += sun * 0.95f; b += sun * 0.85f;

                    size_t idx = ((size_t)face * size * size + (size_t)y * size + (size_t)x) * 4;
                    m_envRGBA[idx + 0] = r;
                    m_envRGBA[idx + 1] = g;
                    m_envRGBA[idx + 2] = b;
                    m_envRGBA[idx + 3] = 1.0f;
                }
            }
        }
    }

    std::shared_ptr<Texture> PBREnvironment::UploadEnvCubemapRGBA16F()
    {
        const int size = m_envSize;
        const int facePixels = size * size;
        ByteBuffer pixels(facePixels * 6 * 16);
        for (int i = 0; i < facePixels * 6; ++i)
        {
            PackRGBA32F(pixels, i,
                m_envRGBA[(size_t)i * 4 + 0],
                m_envRGBA[(size_t)i * 4 + 1],
                m_envRGBA[(size_t)i * 4 + 2]);
        }
        auto tex = Texture::CreateCubemap(size, TextureFormat::R32G32B32A32F, FilterMode::Linear, SamplerAddressMode::ClampToEdge, false);
        int faceBytes = facePixels * 16;
        tex->UpdateCubemap(pixels, 0, { 0, faceBytes, faceBytes * 2, faceBytes * 3, faceBytes * 4, faceBytes * 5 });
        return tex;
    }

    bool PBREnvironment::LoadHDREquirectToCubemap(const std::string& path, int cubeSize)
    {
        int w = 0, h = 0, comp = 0;
        float* data = stbi_loadf(path.c_str(), &w, &h, &comp, 3);
        if (!data || w <= 0 || h <= 0)
        {
            if (data) stbi_image_free(data);
            printf("PBREnvironment: failed to load HDR '%s'\n", path.c_str());
            return false;
        }

        m_envSize = cubeSize;
        m_envRGBA.assign((size_t)cubeSize * cubeSize * 6 * 4, 0.0f);

        auto sampleEquirect = [&](float dx, float dy, float dz, float& r, float& g, float& b)
        {
            float len = std::sqrt(dx * dx + dy * dy + dz * dz);
            dx /= len; dy /= len; dz /= len;
            float phi = std::atan2(dz, dx);
            float theta = std::asin(std::max(-1.0f, std::min(1.0f, dy)));
            float u = phi / (2.0f * 3.14159265f) + 0.5f;
            float v = 0.5f - theta / 3.14159265f;

            float fx = u * (w - 1);
            float fy = v * (h - 1);
            int x0 = (int)fx;
            int y0 = (int)fy;
            int x1 = std::min(x0 + 1, w - 1);
            int y1 = std::min(y0 + 1, h - 1);
            float tx = fx - x0;
            float ty = fy - y0;

            auto fetch = [&](int px, int py, float& fr, float& fg, float& fb)
            {
                int i = (py * w + px) * 3;
                fr = data[i + 0];
                fg = data[i + 1];
                fb = data[i + 2];
            };
            float r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
            fetch(x0, y0, r00, g00, b00);
            fetch(x1, y0, r10, g10, b10);
            fetch(x0, y1, r01, g01, b01);
            fetch(x1, y1, r11, g11, b11);
            r = Lerp(Lerp(r00, r10, tx), Lerp(r01, r11, tx), ty);
            g = Lerp(Lerp(g00, g10, tx), Lerp(g01, g11, tx), ty);
            b = Lerp(Lerp(b00, b10, tx), Lerp(b01, b11, tx), ty);
        };

        for (int face = 0; face < 6; ++face)
        {
            for (int y = 0; y < cubeSize; ++y)
            {
                for (int x = 0; x < cubeSize; ++x)
                {
                    float uu = (x + 0.5f) / cubeSize * 2.0f - 1.0f;
                    float vv = (y + 0.5f) / cubeSize * 2.0f - 1.0f;
                    float dx, dy, dz;
                    FaceUVToDir(face, uu, vv, dx, dy, dz);
                    float r, g, b;
                    sampleEquirect(dx, dy, dz, r, g, b);
                    size_t idx = ((size_t)face * cubeSize * cubeSize + (size_t)y * cubeSize + (size_t)x) * 4;
                    m_envRGBA[idx + 0] = r;
                    m_envRGBA[idx + 1] = g;
                    m_envRGBA[idx + 2] = b;
                    m_envRGBA[idx + 3] = 1.0f;
                }
            }
        }

        stbi_image_free(data);
        printf("PBREnvironment: loaded HDR '%s' (%dx%d) -> cube %d\n", path.c_str(), w, h, cubeSize);
        return true;
    }

    std::shared_ptr<Texture> PBREnvironment::UploadCubemapRGBA32F(const std::vector<ByteBuffer>& mips, int size)
    {
        auto tex = Texture::CreateCubemap(size, TextureFormat::R32G32B32A32F, FilterMode::Linear, SamplerAddressMode::ClampToEdge, mips.size() > 1);
        for (int mip = 0; mip < (int)mips.size(); ++mip)
        {
            int mipSize = std::max(1, size >> mip);
            int faceBytes = mipSize * mipSize * 16;
            tex->UpdateCubemap(mips[mip], mip, { 0, faceBytes, faceBytes * 2, faceBytes * 3, faceBytes * 4, faceBytes * 5 });
        }
        return tex;
    }

    ByteBuffer PBREnvironment::BakeIrradiance(const std::vector<float>& envRGBA, int envSize, int outSize)
    {
        const int sampleCount = 256;
        ByteBuffer pixels(outSize * outSize * 6 * 16);

        for (int face = 0; face < 6; ++face)
        {
            for (int y = 0; y < outSize; ++y)
            {
                for (int x = 0; x < outSize; ++x)
                {
                    float u = (x + 0.5f) / outSize * 2.0f - 1.0f;
                    float v = (y + 0.5f) / outSize * 2.0f - 1.0f;
                    float Nx, Ny, Nz;
                    FaceUVToDir(face, u, v, Nx, Ny, Nz);

                    float upx = 0, upy = 0, upz = 0;
                    if (std::abs(Nz) < 0.999f) { upz = 1; }
                    else { upx = 1; }
                    float Tx = upy * Nz - upz * Ny;
                    float Ty = upz * Nx - upx * Nz;
                    float Tz = upx * Ny - upy * Nx;
                    float tlen = std::sqrt(Tx * Tx + Ty * Ty + Tz * Tz);
                    Tx /= tlen; Ty /= tlen; Tz /= tlen;
                    float Bx = Ny * Tz - Nz * Ty;
                    float By = Nz * Tx - Nx * Tz;
                    float Bz = Nx * Ty - Ny * Tx;

                    // Cosine-weighted hemisphere average => E/pi (matches irradiance * diffuseColor).
                    float ir = 0, ig = 0, ib = 0;
                    for (int i = 0; i < sampleCount; ++i)
                    {
                        float xi1, xi2;
                        Hammersley((uint32_t)i, (uint32_t)sampleCount, xi1, xi2);
                        float phi = 2.0f * 3.14159265f * xi1;
                        float cosTheta = std::sqrt(1.0f - xi2);
                        float sinTheta = std::sqrt(xi2);
                        float lx = std::cos(phi) * sinTheta;
                        float ly = std::sin(phi) * sinTheta;
                        float lz = cosTheta;
                        float Lx = Tx * lx + Bx * ly + Nx * lz;
                        float Ly = Ty * lx + By * ly + Ny * lz;
                        float Lz = Tz * lx + Bz * ly + Nz * lz;
                        float sr, sg, sb;
                        SampleCubemap(envRGBA, envSize, Lx, Ly, Lz, sr, sg, sb);
                        ir += sr; ig += sg; ib += sb;
                    }
                    ir /= (float)sampleCount;
                    ig /= (float)sampleCount;
                    ib /= (float)sampleCount;

                    int dst = face * outSize * outSize + y * outSize + x;
                    PackRGBA32F(pixels, dst, ir, ig, ib);
                }
            }
        }
        return pixels;
    }

    std::vector<ByteBuffer> PBREnvironment::BakePrefiltered(const std::vector<float>& envRGBA, int envSize, int outSize, int mipCount)
    {
        std::vector<ByteBuffer> mips;
        const int sampleCount = 256;

        for (int mip = 0; mip < mipCount; ++mip)
        {
            int mipSize = std::max(1, outSize >> mip);
            float roughness = (float)mip / (float)std::max(1, mipCount - 1);
            ByteBuffer pixels(mipSize * mipSize * 6 * 16);

            for (int face = 0; face < 6; ++face)
            {
                for (int y = 0; y < mipSize; ++y)
                {
                    for (int x = 0; x < mipSize; ++x)
                    {
                        float u = (x + 0.5f) / mipSize * 2.0f - 1.0f;
                        float v = (y + 0.5f) / mipSize * 2.0f - 1.0f;
                        float Nx, Ny, Nz;
                        FaceUVToDir(face, u, v, Nx, Ny, Nz);
                        float Vx = Nx, Vy = Ny, Vz = Nz;

                        float pr = 0, pg = 0, pb = 0;
                        float weight = 0;
                        for (int i = 0; i < sampleCount; ++i)
                        {
                            float xi1, xi2;
                            Hammersley((uint32_t)i, (uint32_t)sampleCount, xi1, xi2);
                            float Hx, Hy, Hz;
                            ImportanceSampleGGX(xi1, xi2, std::max(roughness, 0.001f), Nx, Ny, Nz, Hx, Hy, Hz);
                            float Lx = 2.0f * (Vx * Hx + Vy * Hy + Vz * Hz) * Hx - Vx;
                            float Ly = 2.0f * (Vx * Hx + Vy * Hy + Vz * Hz) * Hy - Vy;
                            float Lz = 2.0f * (Vx * Hx + Vy * Hy + Vz * Hz) * Hz - Vz;
                            float NoL = std::max(0.0f, Nx * Lx + Ny * Ly + Nz * Lz);
                            if (NoL > 0.0f)
                            {
                                // Approximate mip-filtered source sample by averaging nearby env texels via bilinear cubemap.
                                float sr, sg, sb;
                                SampleCubemap(envRGBA, envSize, Lx, Ly, Lz, sr, sg, sb);
                                pr += sr * NoL;
                                pg += sg * NoL;
                                pb += sb * NoL;
                                weight += NoL;
                            }
                        }
                        if (weight > 0) { pr /= weight; pg /= weight; pb /= weight; }

                        int dst = face * mipSize * mipSize + y * mipSize + x;
                        PackRGBA32F(pixels, dst, pr, pg, pb);
                    }
                }
            }

            mips.push_back(pixels);
        }
        return mips;
    }

    ByteBuffer PBREnvironment::BakeBRDFLUT(int size)
    {
        ByteBuffer pixels(size * size * 4);
        const uint32_t sampleCount = 256;

        auto GeometrySchlickGGX = [](float NdotV, float roughness) {
            float a = roughness;
            float k = (a * a) / 2.0f;
            return NdotV / (NdotV * (1.0f - k) + k);
        };
        auto GeometrySmith = [&](float NdotV, float NdotL, float roughness) {
            return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
        };

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                float NdotV = (x + 0.5f) / size;
                float roughness = (y + 0.5f) / size;
                NdotV = std::max(NdotV, 0.001f);

                float Vx = std::sqrt(1.0f - NdotV * NdotV);
                float Vy = 0.0f;
                float Vz = NdotV;
                float Nx = 0, Ny = 0, Nz = 1;

                float A = 0, B = 0;
                for (uint32_t i = 0; i < sampleCount; ++i)
                {
                    float xi1, xi2;
                    Hammersley(i, sampleCount, xi1, xi2);
                    float Hx, Hy, Hz;
                    ImportanceSampleGGX(xi1, xi2, roughness, Nx, Ny, Nz, Hx, Hy, Hz);
                    float VdotH = std::max(0.0f, Vx * Hx + Vy * Hy + Vz * Hz);
                    float Lx = 2.0f * VdotH * Hx - Vx;
                    float Ly = 2.0f * VdotH * Hy - Vy;
                    float Lz = 2.0f * VdotH * Hz - Vz;
                    float NdotL = std::max(Lz, 0.0f);
                    float NdotH = std::max(Hz, 0.0f);
                    if (NdotL > 0.0f)
                    {
                        float G = GeometrySmith(NdotV, NdotL, roughness);
                        float G_Vis = (G * VdotH) / std::max(NdotH * NdotV, 1e-4f);
                        float Fc = std::pow(1.0f - VdotH, 5.0f);
                        A += (1.0f - Fc) * G_Vis;
                        B += Fc * G_Vis;
                    }
                }
                A /= (float)sampleCount;
                B /= (float)sampleCount;
                int dst = (y * size + x) * 4;
                pixels[dst + 0] = (uint8_t)(Saturate(A) * 255.0f + 0.5f);
                pixels[dst + 1] = (uint8_t)(Saturate(B) * 255.0f + 0.5f);
                pixels[dst + 2] = 0;
                pixels[dst + 3] = 255;
            }
        }
        return pixels;
    }

    namespace
    {
        const int kIrradianceSize = 32;
        const int kPrefilterSize = 256;
        const int kPrefilterMips = 7;
        const int kLutSize = 256;

        // Bump when bake math/layout changes so stale caches are rebuilt.
        const uint32_t kCacheVersion = 1;
        const char kCacheMagic[8] = { 'M', 'R', 'I', 'B', 'L', 'C', 'A', '1' };

        struct IBLCacheHeader
        {
            char magic[8];
            uint32_t version;
            uint32_t irradianceSize;
            uint32_t prefilterSize;
            uint32_t prefilterMips;
            uint32_t lutSize;
            uint32_t reserved;
            uint64_t sourceSize;
            int64_t sourceTime;
        };

        bool GetSourceStamp(const std::string& path, uint64_t& size, int64_t& time)
        {
#if defined(_WIN32)
            struct _stat64 st;
            if (_stat64(path.c_str(), &st) != 0) return false;
#else
            struct stat st;
            if (stat(path.c_str(), &st) != 0) return false;
#endif
            size = (uint64_t)st.st_size;
            time = (int64_t)st.st_mtime;
            return true;
        }

        IBLCacheHeader MakeHeader(uint64_t sourceSize, int64_t sourceTime)
        {
            IBLCacheHeader h = {};
            std::memcpy(h.magic, kCacheMagic, sizeof(kCacheMagic));
            h.version = kCacheVersion;
            h.irradianceSize = kIrradianceSize;
            h.prefilterSize = kPrefilterSize;
            h.prefilterMips = kPrefilterMips;
            h.lutSize = kLutSize;
            h.sourceSize = sourceSize;
            h.sourceTime = sourceTime;
            return h;
        }

        int CubeBytes(int size) { return size * size * 6 * 16; }
    }

    bool PBREnvironment::LoadBakeCache(const std::string& sourcePath, ByteBuffer& irradiance, std::vector<ByteBuffer>& prefiltered, ByteBuffer& lut)
    {
        uint64_t srcSize = 0;
        int64_t srcTime = 0;
        if (!GetSourceStamp(sourcePath, srcSize, srcTime))
        {
            return false;
        }

        const std::string cachePath = sourcePath + ".iblcache";
        FILE* fp = fopen(cachePath.c_str(), "rb");
        if (!fp)
        {
            return false;
        }

        IBLCacheHeader expected = MakeHeader(srcSize, srcTime);
        IBLCacheHeader header = {};
        bool ok = fread(&header, sizeof(header), 1, fp) == 1 && std::memcmp(&header, &expected, sizeof(header)) == 0;

        auto readBlock = [&](ByteBuffer& buf, int bytes) {
            if (!ok) return;
            buf = ByteBuffer(bytes);
            ok = fread(buf.Bytes(), 1, (size_t)bytes, fp) == (size_t)bytes;
        };

        readBlock(irradiance, CubeBytes(kIrradianceSize));
        prefiltered.clear();
        for (int mip = 0; ok && mip < kPrefilterMips; ++mip)
        {
            ByteBuffer mipPixels;
            readBlock(mipPixels, CubeBytes(std::max(1, kPrefilterSize >> mip)));
            prefiltered.push_back(mipPixels);
        }
        readBlock(lut, kLutSize * kLutSize * 4);
        fclose(fp);

        if (!ok)
        {
            printf("PBREnvironment: IBL cache '%s' is stale or invalid, rebaking\n", cachePath.c_str());
        }
        return ok;
    }

    void PBREnvironment::SaveBakeCache(const std::string& sourcePath, const ByteBuffer& irradiance, const std::vector<ByteBuffer>& prefiltered, const ByteBuffer& lut)
    {
        uint64_t srcSize = 0;
        int64_t srcTime = 0;
        if (!GetSourceStamp(sourcePath, srcSize, srcTime))
        {
            return;
        }

        const std::string cachePath = sourcePath + ".iblcache";
        FILE* fp = fopen(cachePath.c_str(), "wb");
        if (!fp)
        {
            printf("PBREnvironment: failed to write IBL cache '%s'\n", cachePath.c_str());
            return;
        }
        IBLCacheHeader header = MakeHeader(srcSize, srcTime);
        fwrite(&header, sizeof(header), 1, fp);
        fwrite(irradiance.Bytes(), 1, (size_t)irradiance.Size(), fp);
        for (const auto& mip : prefiltered)
        {
            fwrite(mip.Bytes(), 1, (size_t)mip.Size(), fp);
        }
        fwrite(lut.Bytes(), 1, (size_t)lut.Size(), fp);
        fclose(fp);
        printf("PBREnvironment: wrote IBL cache '%s'\n", cachePath.c_str());
    }

    void PBREnvironment::BuildFromEnvironment(const std::string& cacheSourcePath)
    {
        if (!m_environment)
        {
            m_environment = UploadEnvCubemapRGBA16F();
        }

        ByteBuffer irradiance;
        std::vector<ByteBuffer> prefiltered;
        ByteBuffer lut;
        if (!cacheSourcePath.empty() && LoadBakeCache(cacheSourcePath, irradiance, prefiltered, lut))
        {
            printf("PBREnvironment: loaded IBL cache for '%s'\n", cacheSourcePath.c_str()); fflush(stdout);
        }
        else
        {
            printf("PBREnvironment: baking irradiance %d...\n", kIrradianceSize); fflush(stdout);
            irradiance = BakeIrradiance(m_envRGBA, m_envSize, kIrradianceSize);
            printf("PBREnvironment: baking prefiltered %d x %d mips...\n", kPrefilterSize, kPrefilterMips); fflush(stdout);
            prefiltered = BakePrefiltered(m_envRGBA, m_envSize, kPrefilterSize, kPrefilterMips);
            printf("PBREnvironment: baking BRDF LUT %d...\n", kLutSize); fflush(stdout);
            lut = BakeBRDFLUT(kLutSize);
            if (!cacheSourcePath.empty())
            {
                SaveBakeCache(cacheSourcePath, irradiance, prefiltered, lut);
            }
        }

        m_irradiance = UploadCubemapRGBA32F({ irradiance }, kIrradianceSize);
        m_prefiltered = UploadCubemapRGBA32F(prefiltered, kPrefilterSize);
        m_brdfLUT = Texture::CreateTexture2DFromMemory(
            lut, kLutSize, kLutSize, TextureFormat::R8G8B8A8,
            FilterMode::Linear, SamplerAddressMode::ClampToEdge, false);
        m_ready = true;
        printf("PBREnvironment: IBL ready\n"); fflush(stdout);
    }

    void PBREnvironment::InitializeProcedural()
    {
        FillProceduralEnvRGBA(256);
        m_environment = UploadEnvCubemapRGBA16F();
        BuildFromEnvironment();
        ApplyToSceneMaterials();
    }

    static float HalfToFloat(uint16_t h)
    {
        uint32_t sign = (uint32_t)(h & 0x8000u) << 16;
        uint32_t exp = (h >> 10) & 0x1Fu;
        uint32_t mant = h & 0x3FFu;
        uint32_t bits;
        if (exp == 0)
        {
            if (mant == 0)
            {
                bits = sign;
            }
            else
            {
                exp = 127 - 15 + 1;
                while ((mant & 0x400u) == 0) { mant <<= 1; --exp; }
                mant &= 0x3FFu;
                bits = sign | (exp << 23) | (mant << 13);
            }
        }
        else if (exp == 31)
        {
            bits = sign | 0x7F800000u | (mant << 13);
        }
        else
        {
            bits = sign | ((exp + 127 - 15) << 23) | (mant << 13);
        }
        float f;
        std::memcpy(&f, &bits, 4);
        return f;
    }

    bool PBREnvironment::LoadKTXCubemap(const std::string& path)
    {
        FILE* fp = fopen(path.c_str(), "rb");
        if (!fp)
        {
            printf("PBREnvironment: failed to open KTX '%s'\n", path.c_str());
            return false;
        }
        std::vector<uint8_t> file;
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        file.resize((size_t)size);
        size_t read = fread(file.data(), 1, file.size(), fp);
        fclose(fp);

        static const uint8_t kKtx1Id[12] = { 0xAB, 'K', 'T', 'X', ' ', '1', '1', 0xBB, '\r', '\n', 0x1A, '\n' };
        if (read < 64 || std::memcmp(file.data(), kKtx1Id, 12) != 0)
        {
            printf("PBREnvironment: '%s' is not a KTX1 file\n", path.c_str());
            return false;
        }

        auto u32 = [&](size_t offset) { uint32_t v; std::memcpy(&v, &file[offset], 4); return v; };
        const uint32_t glType = u32(16);
        const uint32_t glFormat = u32(24);
        const uint32_t width = u32(36);
        const uint32_t height = u32(40);
        const uint32_t faces = u32(52);
        const uint32_t kvBytes = u32(60);
        const uint32_t GL_HALF_FLOAT = 0x140B;
        const uint32_t GL_FLOAT = 0x1406;
        const uint32_t GL_RGBA = 0x1908;
        if (faces != 6 || width != height || glFormat != GL_RGBA || (glType != GL_HALF_FLOAT && glType != GL_FLOAT))
        {
            printf("PBREnvironment: unsupported KTX layout in '%s' (faces=%u fmt=0x%X type=0x%X)\n",
                path.c_str(), faces, glFormat, glType);
            return false;
        }

        const size_t componentSize = glType == GL_HALF_FLOAT ? 2 : 4;
        const size_t faceBytes = (size_t)width * height * 4 * componentSize;
        size_t offset = 64 + kvBytes;
        const uint32_t imageSize = u32(offset);
        offset += 4;
        if (imageSize < faceBytes || offset + faceBytes * 6 > file.size())
        {
            printf("PBREnvironment: truncated KTX '%s'\n", path.c_str());
            return false;
        }

        m_envSize = (int)width;
        m_envRGBA.assign((size_t)width * height * 6 * 4, 0.0f);
        for (int face = 0; face < 6; ++face)
        {
            const uint8_t* src = &file[offset + (size_t)face * ((imageSize + 3) & ~3u)];
            float* dst = &m_envRGBA[(size_t)face * width * height * 4];
            for (size_t i = 0; i < (size_t)width * height * 4; ++i)
            {
                if (componentSize == 2)
                {
                    uint16_t h;
                    std::memcpy(&h, src + i * 2, 2);
                    dst[i] = HalfToFloat(h);
                }
                else
                {
                    std::memcpy(&dst[i], src + i * 4, 4);
                }
            }
        }

        printf("PBREnvironment: loaded KTX cubemap '%s' (%ux%u)\n", path.c_str(), width, height);
        return true;
    }

    static uint16_t FloatToHalf(float f)
    {
        uint32_t bits;
        std::memcpy(&bits, &f, 4);
        uint32_t sign = (bits >> 16) & 0x8000u;
        int32_t exp = (int32_t)((bits >> 23) & 0xFFu) - 127 + 15;
        uint32_t mant = bits & 0x7FFFFFu;
        if (((bits >> 23) & 0xFFu) == 0xFFu)
        {
            return (uint16_t)(sign | 0x7C00u | (mant ? 0x200u : 0u));
        }
        if (exp >= 31)
        {
            return (uint16_t)(sign | 0x7C00u);
        }
        if (exp <= 0)
        {
            if (exp < -10)
            {
                return (uint16_t)sign;
            }
            mant |= 0x800000u;
            uint32_t shift = (uint32_t)(14 - exp);
            uint32_t half = mant >> shift;
            if ((mant >> (shift - 1)) & 1u)
            {
                ++half;
            }
            return (uint16_t)(sign | half);
        }
        uint32_t half = sign | ((uint32_t)exp << 10) | (mant >> 13);
        if (mant & 0x1000u)
        {
            ++half;
        }
        return (uint16_t)half;
    }

    void PBREnvironment::ReorientEnvironment(float sx, float sy, float sz)
    {
        const int size = m_envSize;
        std::vector<float> src = m_envRGBA;
        for (int face = 0; face < 6; ++face)
        {
            for (int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    float u = (x + 0.5f) / size * 2.0f - 1.0f;
                    float v = (y + 0.5f) / size * 2.0f - 1.0f;
                    float dx, dy, dz;
                    FaceUVToDir(face, u, v, dx, dy, dz);
                    dx *= sx; dy *= sy; dz *= sz;

                    float ax = std::abs(dx), ay = std::abs(dy), az = std::abs(dz);
                    int srcFace;
                    float uc, vc;
                    if (ax >= ay && ax >= az)
                    {
                        srcFace = dx > 0 ? 0 : 1;
                        uc = (dx > 0 ? -dz : dz) / ax;
                        vc = -dy / ax;
                    }
                    else if (ay >= ax && ay >= az)
                    {
                        srcFace = dy > 0 ? 2 : 3;
                        uc = dx / ay;
                        vc = (dy > 0 ? dz : -dz) / ay;
                    }
                    else
                    {
                        srcFace = dz > 0 ? 4 : 5;
                        uc = (dz > 0 ? dx : -dx) / az;
                        vc = -dy / az;
                    }

                    int px = (int)std::floor((uc * 0.5f + 0.5f) * size);
                    int py = (int)std::floor((vc * 0.5f + 0.5f) * size);
                    px = std::max(0, std::min(size - 1, px));
                    py = std::max(0, std::min(size - 1, py));

                    size_t s = (((size_t)srcFace * size + py) * size + px) * 4;
                    size_t d = (((size_t)face * size + y) * size + x) * 4;
                    std::memcpy(&m_envRGBA[d], &src[s], sizeof(float) * 4);
                }
            }
        }
    }

    bool PBREnvironment::SaveKTXCubemap(const std::string& path) const
    {
        FILE* fp = fopen(path.c_str(), "wb");
        if (!fp)
        {
            printf("PBREnvironment: failed to write KTX '%s'\n", path.c_str());
            return false;
        }

        static const uint8_t kKtx1Id[12] = { 0xAB, 'K', 'T', 'X', ' ', '1', '1', 0xBB, '\r', '\n', 0x1A, '\n' };
        const uint32_t header[13] = {
            0x04030201u,        // endianness
            0x140Bu,            // glType: GL_HALF_FLOAT
            2u,                 // glTypeSize
            0x1908u,            // glFormat: GL_RGBA
            0x881Au,            // glInternalFormat: GL_RGBA16F
            0x1908u,            // glBaseInternalFormat: GL_RGBA
            (uint32_t)m_envSize,
            (uint32_t)m_envSize,
            0u,                 // pixelDepth
            0u,                 // numberOfArrayElements
            6u,                 // numberOfFaces
            1u,                 // numberOfMipmapLevels
            0u,                 // bytesOfKeyValueData
        };
        fwrite(kKtx1Id, 1, sizeof(kKtx1Id), fp);
        fwrite(header, 4, 13, fp);

        const size_t faceTexels = (size_t)m_envSize * m_envSize * 4;
        const uint32_t imageSize = (uint32_t)(faceTexels * sizeof(uint16_t));
        fwrite(&imageSize, 4, 1, fp);

        std::vector<uint16_t> halfs(faceTexels);
        for (int face = 0; face < 6; ++face)
        {
            const float* src = &m_envRGBA[(size_t)face * faceTexels];
            for (size_t i = 0; i < faceTexels; ++i)
            {
                halfs[i] = FloatToHalf(src[i]);
            }
            fwrite(halfs.data(), sizeof(uint16_t), halfs.size(), fp);
        }
        fclose(fp);
        printf("PBREnvironment: wrote reoriented KTX cubemap '%s'\n", path.c_str());
        return true;
    }

    bool PBREnvironment::InitializeFromKTXReoriented(const std::string& path, float sx, float sy, float sz, const std::string& savePath)
    {
        if (!LoadKTXCubemap(path))
        {
            return false;
        }
        ReorientEnvironment(sx, sy, sz);
        bool saved = !savePath.empty() && SaveKTXCubemap(savePath);
        m_environment = UploadEnvCubemapRGBA16F();
        // Cache is keyed to the reoriented file, never to the unflipped source.
        BuildFromEnvironment(saved ? savePath : std::string());
        ApplyToSceneMaterials();
        return true;
    }

    bool PBREnvironment::InitializeFromKTX(const std::string& path)
    {
        if (!LoadKTXCubemap(path))
        {
            return false;
        }
        m_environment = UploadEnvCubemapRGBA16F();
        BuildFromEnvironment(path);
        ApplyToSceneMaterials();
        return true;
    }

    bool PBREnvironment::InitializeFromHDR(const std::string& path)
    {
        // 256 cube is enough for loft HDR; Diligent often uses 512 — raise if needed.
        if (!LoadHDREquirectToCubemap(path, 256))
        {
            return false;
        }
        m_environment = UploadEnvCubemapRGBA16F();
        BuildFromEnvironment(path);
        ApplyToSceneMaterials();
        return true;
    }

    void PBREnvironment::InitializeFromCubemap(const std::shared_ptr<Texture>& envCubemap)
    {
        if (m_envRGBA.empty())
        {
            FillProceduralEnvRGBA(256);
        }
        if (!m_environment)
        {
            m_environment = UploadEnvCubemapRGBA16F();
        }
        BuildFromEnvironment();
        if (envCubemap)
        {
            m_environment = envCubemap;
        }
        ApplyToSceneMaterials();
    }

    void PBREnvironment::ApplyToMaterial(const std::shared_ptr<Material>& material)
    {
        if (!material || !m_ready) return;
        if (material->GetShaderName() != "standard") return;

        material->SetTexture("SPIRV_Cross_Combinedg_IrradianceMapg_IrradianceSampler", m_irradiance);
        material->SetTexture("SPIRV_Cross_Combinedg_PrefilteredMapg_PrefilteredSampler", m_prefiltered);
        material->SetTexture("SPIRV_Cross_Combinedg_BRDFLUTg_BRDFLUTSampler", m_brdfLUT);
    }

    void PBREnvironment::ApplyToSceneMaterials()
    {
        if (!m_ready) return;
        for (auto* renderer : Renderer::GetRenderers())
        {
            const auto& materials = renderer->GetMaterials();
            for (const auto& mat : materials)
            {
                ApplyToMaterial(mat);
            }
        }
    }
}
