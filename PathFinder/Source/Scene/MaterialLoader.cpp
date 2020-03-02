#include "MaterialLoader.hpp"

#include <glm/gtc/type_precision.hpp>

namespace PathFinder
{

    MaterialLoader::MaterialLoader(const std::filesystem::path& executableFolder, PreprocessableAssetStorage* assetStorage, Memory::GPUResourceProducer* resourceProducer)
        : mAssetStorage{ assetStorage }, mResourceLoader{ executableFolder, resourceProducer }, mResourceProducer{ resourceProducer }
    {
        CreateDefaultTextures();
        LoadLTCLookupTables();
    }

    Material MaterialLoader::LoadMaterial(
        const std::string& albedoMapRelativePath,
        const std::string& normalMapRelativePath,
        std::optional<std::string> roughnessMapRelativePath,
        std::optional<std::string> metalnessMapRelativePath,
        std::optional<std::string> displacementMapRelativePath,
        std::optional<std::string> distanceFieldRelativePath,
        std::optional<std::string> AOMapRelativePath)
    {
        Material material{};

        material.AlbedoMap = GetOrAllocateTexture(albedoMapRelativePath);
        material.NormalMap = GetOrAllocateTexture(normalMapRelativePath);

        if (roughnessMapRelativePath) material.RoughnessMap = GetOrAllocateTexture(*roughnessMapRelativePath);
        if (metalnessMapRelativePath) material.MetalnessMap = GetOrAllocateTexture(*metalnessMapRelativePath);
        if (displacementMapRelativePath) material.DisplacementMap = GetOrAllocateTexture(*displacementMapRelativePath);
        if (AOMapRelativePath) material.AOMap = GetOrAllocateTexture(*AOMapRelativePath);

        if (material.DisplacementMap && distanceFieldRelativePath)
        {
            material.DistanceField = GetOrAllocateTexture(*distanceFieldRelativePath);

            if (!material.DistanceField)
            {
                HAL::Texture::Properties distFieldProperties{
                    HAL::ColorFormat::RGBA32_Unsigned, HAL::TextureKind::Texture3D,
                    DistanceFieldTextureSize, HAL::ResourceState::UnorderedAccess, HAL::ResourceState::AnyShaderAccess };

                material.DistanceField = AllocateAndStoreTexture(distFieldProperties, *distanceFieldRelativePath);

                mAssetStorage->PreprocessAsset(material.DistanceField, [this, distanceFieldRelativePath](Memory::GPUResource* distanceField)
                {
                    mResourceLoader.StoreResource(*distanceField, *distanceFieldRelativePath);
                });
            }
        }

        if (!material.AlbedoMap) material.AlbedoMap = m1x1White2DTexture.get();
        if (!material.NormalMap) material.NormalMap = m1x1White2DTexture.get();
        if (!material.RoughnessMap) material.RoughnessMap = m1x1White2DTexture.get();
        if (!material.MetalnessMap) material.MetalnessMap = m1x1Black2DTexture.get();
        if (!material.DisplacementMap) material.DisplacementMap = m1x1Black2DTexture.get();
        if (!material.AOMap) material.AOMap = m1x1White2DTexture.get();
        if (!material.DistanceField) material.DistanceField = m1x1Black3DTexture.get();

        material.LTC_LUT_0_Specular = mLTC_LUT_0_GGXHeightCorrelated.get();
        material.LTC_LUT_1_Specular = mLTC_LUT_1_GGXHeightCorrelated.get();

        material.LTC_LUT_0_Diffuse = mLTC_LUT_0_DisneyDiffuseNormalized.get();
        material.LTC_LUT_1_Diffuse = mLTC_LUT_1_DisneyDiffuseNormalized.get();

        return material;
    }

    Memory::Texture* MaterialLoader::GetOrAllocateTexture(const std::string& relativePath)
    {
        auto textureIt = mMaterialTextures.find(relativePath);

        if (textureIt != mMaterialTextures.end())
        {
            return textureIt->second.get();
        }
        else
        {
            auto [iter, success] = mMaterialTextures.emplace(relativePath, mResourceLoader.LoadTexture(relativePath));
            return iter->second.get();
        }
    }

    Memory::Texture* MaterialLoader::AllocateAndStoreTexture(const HAL::Texture::Properties& properties, const std::string& relativePath)
    {
        auto [iter, success] = mMaterialTextures.emplace(relativePath, mResourceProducer->NewTexture(properties));
        return iter->second.get();
    }

    void MaterialLoader::CreateDefaultTextures()
    {
        HAL::Texture::Properties dummy2DTextureProperties{
            HAL::ColorFormat::RGBA8_Usigned_Norm, HAL::TextureKind::Texture2D,
            Geometry::Dimensions{1, 1}, HAL::ResourceState::AnyShaderAccess };

        HAL::Texture::Properties dummy3DTextureProperties{
            HAL::ColorFormat::RGBA8_Usigned_Norm, HAL::TextureKind::Texture3D,
            Geometry::Dimensions{1, 1}, HAL::ResourceState::AnyShaderAccess };

        m1x1Black2DTexture = mResourceProducer->NewTexture(dummy2DTextureProperties);
        m1x1White2DTexture = mResourceProducer->NewTexture(dummy2DTextureProperties);
        m1x1Black3DTexture = mResourceProducer->NewTexture(dummy3DTextureProperties);

        m1x1Black2DTexture->RequestWrite();
        m1x1White2DTexture->RequestWrite();
        m1x1Black3DTexture->RequestWrite();

        glm::u8vec4 black{ 0, 0, 0, 0 };
        glm::u8vec4 white{ 1, 1, 1, 1 };

        m1x1Black2DTexture->Write(&black, 0, 1);
        m1x1White2DTexture->Write(&white, 0, 1);
        m1x1Black3DTexture->Write(&black, 0, 1);
    }

    void MaterialLoader::LoadLTCLookupTables()
    {
        mLTC_LUT_0_GGXHeightCorrelated = mResourceLoader.LoadTexture("/Precompiled/LTC_LUT_GGXHeightCorrelated_0.dds");
        mLTC_LUT_1_GGXHeightCorrelated = mResourceLoader.LoadTexture("/Precompiled/LTC_LUT_GGXHeightCorrelated_1.dds");
        mLTC_LUT_0_DisneyDiffuseNormalized = mResourceLoader.LoadTexture("/Precompiled/LTC_LUT_Disney_Diffuse_Normalized_0.dds");
        mLTC_LUT_1_DisneyDiffuseNormalized = mResourceLoader.LoadTexture("/Precompiled/LTC_LUT_Disney_Diffuse_Normalized_1.dds");
    }

}
