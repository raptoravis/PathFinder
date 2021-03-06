#include "Buffer.hpp"
#include "CopyRequestManager.hpp"

namespace Memory
{

    Buffer::Buffer(
        const HAL::BufferProperties& properties, 
        GPUResource::UploadStrategy uploadStrategy, 
        ResourceStateTracker* stateTracker,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyRequestManager* copyRequestManager)
        :
        GPUResource(uploadStrategy, stateTracker, resourceAllocator, descriptorAllocator, copyRequestManager),
        mRequstedStride{ properties.Stride },
        mProperties{ properties }
    {
        if (uploadStrategy == GPUResource::UploadStrategy::Automatic)
        {
            mBufferPtr = resourceAllocator->AllocateBuffer(properties);
            if (mStateTracker) mStateTracker->StartTrakingResource(mBufferPtr.get());
        }
        else
        {
            mUploadBuffers.emplace(resourceAllocator->AllocateBuffer(properties, HAL::CPUAccessibleHeapType::Upload), 0);
        }
    }

    Buffer::Buffer(
        const HAL::BufferProperties& properties, 
        ResourceStateTracker* stateTracker, 
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        PoolDescriptorAllocator* descriptorAllocator, 
        CopyRequestManager* copyRequestManager,
        const HAL::Device& device, 
        const HAL::Heap& mainResourceExplicitHeap, 
        uint64_t explicitHeapOffset)
        :
        GPUResource(UploadStrategy::Automatic, stateTracker, resourceAllocator, descriptorAllocator, copyRequestManager),
        mRequstedStride{ properties.Stride },
        mProperties{ properties }
    {
        mBufferPtr = SegregatedPoolsResourceAllocator::BufferPtr{
            new HAL::Buffer{ device, properties, mainResourceExplicitHeap, explicitHeapOffset },
            [](HAL::Buffer* buffer) { delete buffer; }
        };

        if (mStateTracker) mStateTracker->StartTrakingResource(mBufferPtr.get());
    }

    Buffer::~Buffer()
    {
        if (mStateTracker && mBufferPtr) mStateTracker->StopTrakingResource(mBufferPtr.get());
    }

    const HAL::CBDescriptor* Buffer::GetCBDescriptor() const
    {
        // Descriptor needs to be created either if it does not exist yet
        // or we're only using upload buffers (Direct Access) and no descriptors were 
        // created for upload buffer of this frame
        //
        if (!mCBDescriptor || (!mBufferPtr && mCBDescriptorRequestFrameNumber != mFrameNumber))
        {
            mCBDescriptor = mDescriptorAllocator->AllocateCBDescriptor(*HALBuffer(), mRequstedStride);
            mCBDescriptorRequestFrameNumber = mFrameNumber;
        }

        return mCBDescriptor.get();
    }

    const HAL::UADescriptor* Buffer::GetUADescriptor() const
    {
        assert_format(mUploadStrategy != GPUResource::UploadStrategy::DirectAccess,
            "Direct Access buffers cannot have Unordered Access descriptors since they're always in GenericRead state");

        if (!mUADescriptor)
        {
            mUADescriptor = mDescriptorAllocator->AllocateUADescriptor(*HALBuffer(), mRequstedStride);
        }

        return mUADescriptor.get();
    }

    const HAL::SRDescriptor* Buffer::GetSRDescriptor() const
    {
        // Descriptor needs to be created either if it does not exist yet
        // or we're only using upload buffers (Direct Access) and no descriptors were 
        // created for upload buffer of this frame
        //
        if (!mSRDescriptor || (!mBufferPtr && mSRDescriptorRequestFrameNumber != mFrameNumber))
        {
            mSRDescriptor = mDescriptorAllocator->AllocateSRDescriptor(*HALBuffer(), mRequstedStride);
            mSRDescriptorRequestFrameNumber = mFrameNumber;
        }

        return mSRDescriptor.get();
    }

    const HAL::Buffer* Buffer::HALBuffer() const
    {
        return mUploadStrategy == GPUResource::UploadStrategy::Automatic ? 
            mBufferPtr.get() : CurrentFrameUploadBuffer();
    }

    const HAL::Resource* Buffer::HALResource() const
    {
        return HALBuffer();
    }

    uint64_t Buffer::ResourceSizeInBytes() const
    {
        return mBufferPtr ? mBufferPtr->TotalMemory() : mUploadBuffers.front().first->TotalMemory();
    }

    void Buffer::ApplyDebugName()
    {
        GPUResource::ApplyDebugName();

        if (mBufferPtr)
        {
            mBufferPtr->SetDebugName(mDebugName);
        }
    }

    CopyRequestManager::CopyCommand Buffer::GetUploadCommands()
    {
        return[&](HAL::CopyCommandListBase& cmdList)
        {
            if (mUploadStrategy != GPUResource::UploadStrategy::DirectAccess)
            {
                cmdList.CopyBufferRegion(*CurrentFrameUploadBuffer(), *HALBuffer(), 0, HALBuffer()->ElementCapacity(), 0);
            }
        };
    }

    CopyRequestManager::CopyCommand Buffer::GetReadbackCommands()
    {
        return[&](HAL::CopyCommandListBase& cmdList)
        {
            cmdList.CopyBufferRegion(*HALBuffer(), *CurrentFrameReadbackBuffer(), 0, HALBuffer()->ElementCapacity(), 0);
        };
    }

}
