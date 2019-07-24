#pragma once

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    namespace ResourceNames
    {
        inline Foundation::Name PlaygroundRenderTarget{ "Resource_PlaygroundRT" };
    }

    namespace PSONames
    {
        inline Foundation::Name DepthOnly{ "PSO_DepthOnly" };
        inline Foundation::Name GBuffer{ "PSO_GBuffer" };
        inline Foundation::Name PostProcessing{ "PSO_PostProcessing" };
    }

}