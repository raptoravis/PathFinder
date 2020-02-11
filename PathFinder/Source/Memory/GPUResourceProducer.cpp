#include "GPUResourceProducer.hpp"

namespace Memory
{

    GPUResourceProducer::GPUResourceProducer(
        const HAL::Device* device,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        ResourceStateTracker* stateTracker, 
        PoolDescriptorAllocator* descriptorAllocator)
        : 
        mDevice{ device },
        mResourceAllocator{ resourceAllocator }, 
        mStateTracker{ stateTracker }, 
        mDescriptorAllocator{ descriptorAllocator } {}

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(const HAL::Texture::Properties& properties)
    {
        Texture* texture = new Texture{ properties, mStateTracker, mResourceAllocator, mDescriptorAllocator, mCommandList };
        auto [iter, success] = mAllocatedResources.insert(texture);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(const HAL::Texture::Properties& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset)
    {
        Texture* texture = new Texture{
            properties, mStateTracker, mResourceAllocator, mDescriptorAllocator, 
            mCommandList, *mDevice, explicitHeap, heapOffset 
        };

        auto [iter, success] = mAllocatedResources.insert(texture);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    void GPUResourceProducer::SetCommandList(HAL::CopyCommandListBase* commandList)
    {
        mCommandList = commandList;

        for (GPUResource* resource : mAllocatedResources)
        {
            resource->SetCommandList(mCommandList);
        }
    }

    void GPUResourceProducer::BeginFrame(uint64_t frameNumber)
    {
        for (GPUResource* resource : mAllocatedResources)
        {
            resource->BeginFrame(frameNumber);
        }
    }

    void GPUResourceProducer::EndFrame(uint64_t frameNumber)
    {
        for (GPUResource* resource : mAllocatedResources)
        {
            resource->EndFrame(frameNumber);
        }
    }

}
