namespace PathFinder
{

    template <class T>
    void CopyDevice::QueueBufferToTextureCopy(std::shared_ptr<HAL::BufferResource<T>> buffer, std::shared_ptr<HAL::TextureResource> texture, const HAL::ResourceFootprint& footprint)
    {
        for (const HAL::SubresourceFootprint& subresourceFootprint : footprint.SubresourceFootprints())
        {
            mCommandList.CopyBufferToTexture(*buffer, *texture, subresourceFootprint);
        }

        mResourcesToCopy.push_back(buffer);
        mResourcesToCopy.push_back(texture);
    }

    template <class T>
    std::shared_ptr<HAL::BufferResource<T>> CopyDevice::QueueResourceCopyToSystemMemory(std::shared_ptr<HAL::BufferResource<T>> buffer)
    {
        auto emptyClone = std::make_shared<HAL::BufferResource<T>>(
            *mDevice, buffer->Capacity(), buffer->PerElementAlignment(), HAL::ResourceState::CopyDestination, buffer->ExpectedStates());

        mCommandList.CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy.push_back(buffer);
        mResourcesToCopy.push_back(emptyClone);

        return emptyClone;
    }


    template <class T>
    std::shared_ptr<HAL::BufferResource<T>> CopyDevice::QueueResourceCopyToReadbackMemory(std::shared_ptr<HAL::BufferResource<T>> buffer)
    {
        auto emptyClone = std::make_shared<HAL::BufferResource<T>>(
            *mDevice, buffer->Capacity(), buffer->PerElementAlignment(), HAL::CPUAccessibleHeapType::Readback);

        mCommandList.CopyResource(*buffer, *emptyClone);

        // Hold in RAM until actual copy is performed
        mResourcesToCopy.push_back(buffer);
        mResourcesToCopy.push_back(emptyClone);

        return emptyClone;
    }

}

