#pragma once

#include "ResourceStorage.hpp"
#include "RenderSurface.hpp"
#include "ResourceDescriptorStorage.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/SwapChain.hpp"
#include "../HardwareAbstractionLayer/RingBufferResource.hpp"
#include "../Foundation/MemoryUtils.hpp"

#include <vector>
#include <unordered_map>
#include <functional>
#include <tuple>

namespace PathFinder
{

    class RenderPass;

    class ResourceStorage
    {
    public:
        friend class ResourceScheduler;
        friend class ResourceProvider;
        friend class RootConstantsUpdater;

        using ResourceName = Foundation::Name;
        using PassName = Foundation::Name;

        ResourceStorage(HAL::Device* device, const RenderSurface& defaultRenderSurface, uint8_t simultaneousFramesInFlight);

        void BeginFrame(uint64_t frameFenceValue);
        void EndFrame(uint64_t completedFrameFenceValue);

        HAL::RTDescriptor GetRenderTargetDescriptor(Foundation::Name resourceName) const;
        HAL::DSDescriptor GetDepthStencilDescriptor(Foundation::Name resourceName) const;
        HAL::RTDescriptor GetBackBufferDescriptor() const;
        
        void SetCurrentBackBufferIndex(uint8_t index);
        void SetCurrentPassName(PassName passName);
        void SetCurrentStateForResource(ResourceName name, HAL::ResourceState state);
        void AllocateScheduledResources();
        void UseSwapChain(HAL::SwapChain& swapChain);

        template <class RootConstants> 
        RootConstants* GetRootConstantDataForCurrentPass() const;

        HAL::BufferResource<uint8_t>* GetRootConstantBufferForCurrentPass() const;
        HAL::Resource* GetResource(ResourceName resourceName) const;

        const std::vector<ResourceName>* GetScheduledResourceNamesForCurrentPass() const;
        std::optional<HAL::ResourceState> GetResourceCurrentState(ResourceName resourceName) const;
        std::optional<HAL::ResourceState> GetResourceStateForCurrentPass(ResourceName resourceName) const;
        std::optional<HAL::ResourceFormat::Color> GetResourceShaderVisibleFormatForCurrentPass(ResourceName resourceName) const;

    private:
        const std::vector<Foundation::Name> BackBufferNames{ "BackBuffer1", "BackBuffer2", "BackBuffer3" };

    private:
        using ScheduledResourceNames = std::unordered_map<PassName, std::vector<ResourceName>>;
        using ResourceMap = std::unordered_map<ResourceName, std::unique_ptr<HAL::Resource>>;
        using ResourceStateMap = std::unordered_map<ResourceName, HAL::ResourceState>;
        using ResourcePerPassStateMap = std::unordered_map<PassName, std::unordered_map<ResourceName, HAL::ResourceState>>;
        using ResourceFormatMap = std::unordered_map<PassName, std::unordered_map<ResourceName, HAL::ResourceFormat::Color>>;
        using ResourceAllocationMap = std::unordered_map<ResourceName, std::function<void()>>;
        using BackBufferDescriptors = std::vector<HAL::RTDescriptor>;
        using BackBufferResources = std::vector<HAL::ColorTextureResource*>;
        using RootConstantBufferMap = std::unordered_map<PassName, std::unique_ptr<HAL::RingBufferResource<uint8_t>>>;

        //template <class BufferT> using TextureAllocationCallback = std::function<void(const ResourceT&)>;
        template <class TextureT> using TextureAllocationCallback = std::function<void(const TextureT&)>;
        template <class TextureT> using TextureAllocationCallbackMap = std::unordered_map<ResourceName, std::vector<TextureAllocationCallback<TextureT>>>;

        using TextureAllocationCallbacks = std::tuple<
            TextureAllocationCallbackMap<HAL::ColorTextureResource>,
            TextureAllocationCallbackMap<HAL::TypelessTextureResource>,
            TextureAllocationCallbackMap<HAL::DepthStencilTextureResource>
        >;

        template <class TextureT, class ...Args>
        void QueueTextureAllocationIfNeeded(ResourceName resourceName, const TextureAllocationCallback<TextureT>& callback, Args&&... args);

        template <class BufferDataT>
        void AllocateRootConstantBufferIfNeeded();

        void RegisterStateForResource(ResourceName resourceName, HAL::ResourceState state);
        void MarkResourceNameAsScheduled(ResourceName name);

        HAL::Device* mDevice;

        RenderSurface mDefaultRenderSurface;
        PassName mCurrentPassName;
        uint8_t mSimultaneousFramesInFlight;

        ScheduledResourceNames mScheduledResourceNames;
        ResourceMap mResources;
        ResourcePerPassStateMap mResourcePerPassStates;
        ResourceStateMap mResourceExpectedStates;
        ResourceStateMap mResourceCurrentStates;
        ResourceAllocationMap mResourceDelayedAllocations;
        ResourceFormatMap mResourceShaderVisibleFormatMap;
        TextureAllocationCallbacks mTextureAllocationCallbacks;
        ResourceDescriptorStorage mDescriptorStorage;
        BackBufferDescriptors mBackBufferDescriptors;
        RootConstantBufferMap mRootConstantBuffers;

        uint8_t mCurrentBackBufferIndex = 0;
    };

}

#include "ResourceStorage.inl"