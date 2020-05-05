#pragma once

#include "../Foundation/Name.hpp"
#include "../Scene/Scene.hpp"

#include "GPUCommandRecorder.hpp"
#include "RootConstantsUpdater.hpp"
#include "ResourceProvider.hpp"
#include "RenderPassUtilityProvider.hpp"
#include "UIGPUStorage.hpp"
#include "SceneGPUStorage.hpp"

namespace PathFinder
{

    template <class ContentMediator>
    class RenderContext
    {
    public:
        RenderContext(
            GPUCommandRecorder* graphicCommandRecorder, 
            RootConstantsUpdater* rootConstantsUpdater, 
            ResourceProvider* resourceProvider,
            RenderPassUtilityProvider* utilityProvider)
            :
            mGrapicCommandRecorder{ graphicCommandRecorder },
            mRootConstantsUpdater{ rootConstantsUpdater },
            mResourceProvider{ resourceProvider },
            mUtilityProvider{ utilityProvider } {}

        void SetContent(ContentMediator* content) { mContent = content; }

    private:
        GPUCommandRecorder* mGrapicCommandRecorder;
        RootConstantsUpdater* mRootConstantsUpdater;
        ResourceProvider* mResourceProvider;
        RenderPassUtilityProvider* mUtilityProvider;
        ContentMediator* mContent = nullptr;

    public:
        inline GPUCommandRecorder* GetCommandRecorder() const { return mGrapicCommandRecorder; }
        inline RootConstantsUpdater* GetConstantsUpdater() const { return mRootConstantsUpdater; }
        inline ResourceProvider* GetResourceProvider() const { return mResourceProvider; }
        inline const ContentMediator* GetContent() const { return mContent; }
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mUtilityProvider->DefaultRenderSurfaceDescription; }
        inline auto FrameNumber() const { return mUtilityProvider->FrameNumber; }
    };

}
