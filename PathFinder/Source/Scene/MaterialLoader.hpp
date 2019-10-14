#pragma once

#include "Material.hpp"
#include "TextureLoader.hpp"

#include "../RenderPipeline/AssetResourceStorage.hpp"

#include <filesystem>
#include <string>
#include <optional>

namespace PathFinder 
{

    class MaterialLoader
    {
    public:
        MaterialLoader(const std::filesystem::path& fileRoot, const HAL::Device* device, AssetResourceStorage* assetStorage, CopyDevice* copyDevice);

        Material LoadMaterial(
            const std::string& albedoMapRelativePath,
            const std::string& normalMapRelativePath,
            const std::string& roughnessMapRelativePath,
            const std::string& metalnessMapRelativePath,
            std::optional<std::string> AOMapRelativePath = std::nullopt);

    private:
        AssetResourceStorage* mAssetStorage;
        TextureLoader mTextureLoader;
    };

}