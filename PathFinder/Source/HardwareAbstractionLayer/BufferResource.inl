#pragma once

namespace HAL
{
    template <class T>
    BufferResource<T>::BufferResource(const Device& device, uint64_t capacity, uint64_t perElementAlignment, CPUAccessibleHeapType heapType)
        : Resource(
            device,
            ResourceFormat{
                device, std::nullopt, ResourceFormat::BufferKind::Buffer,
                Geometry::Dimensions{ PaddedElementSize(perElementAlignment) * capacity }
            },
            heapType
        ),
        mNonPaddedElementSize{ sizeof(T) },
        mPaddedElementSize{ PaddedElementSize(perElementAlignment) },
        mCapacity{ capacity },
        mPerElementAlignment{ perElementAlignment }
    {
        ThrowIfFailed(mResource->Map(0, nullptr, (void**)& mMappedMemory));
    }

    template <class T>
    BufferResource<T>::BufferResource(const Device& device, uint64_t capacity, uint64_t perElementAlignment, ResourceState initialState, ResourceState expectedStates)
        : Resource(
            device,
            ResourceFormat{
                device, std::nullopt, ResourceFormat::BufferKind::Buffer,
                Geometry::Dimensions{ PaddedElementSize(perElementAlignment) * capacity }
            },
            initialState,
            expectedStates
        ),
        mNonPaddedElementSize{ sizeof(T) },
        mPaddedElementSize{ PaddedElementSize(perElementAlignment) },
        mCapacity{ capacity },
        mPerElementAlignment{ perElementAlignment }
    { }

    template <class T>
    BufferResource<T>::~BufferResource()
    {
        if (mMappedMemory)
        {
            mResource->Unmap(0, nullptr);
            mMappedMemory = nullptr;
        }
    }

    template <class T>
    void HAL::BufferResource<T>::ValidateMappedMemory() const
    {
        if (!mMappedMemory)
        {
            throw std::runtime_error("Buffer resource is not readable by CPU");
        }
    }

    template <class T>
    void HAL::BufferResource<T>::ValidateIndex(uint64_t index) const
    {
        if (index >= mCapacity) {
            throw std::invalid_argument("Index is out of bounds");
        }
    }

    template <class T>
    uint64_t BufferResource<T>::PaddedElementSize(uint64_t alignment)
    {
        return (sizeof(T) + alignment - 1) & ~(alignment - 1);
    }

    template <class T>
    void BufferResource<T>::Write(uint64_t startIndex, const T* data, uint64_t dataLength)
    {
        ValidateMappedMemory();
        ValidateIndex(startIndex);

        if (mPaddedElementSize > sizeof(T) && dataLength > 1)
        {
            throw std::invalid_argument(
                "Writing several objects into buffer that requires per object memory padding."
                "Instead of writing a continuous chunk of memory, write objects one by one in a loop."
            );
        }

        memcpy(mMappedMemory + startIndex * mPaddedElementSize, data, sizeof(T) * dataLength);
    }

    template <class T>
    T* HAL::BufferResource<T>::At(uint64_t index)
    {
        ValidateMappedMemory();
        ValidateIndex(index);

        return reinterpret_cast<T*>(mMappedMemory + index * mPaddedElementSize);
    }

    template <class T>
    bool BufferResource<T>::CanImplicitlyPromoteFromCommonStateToState(HAL::ResourceState state) const
    {
        // Buffers can be promoted from Common to all states except depth ones
        return !EnumMaskBitSet(state, ResourceState::DepthRead) && !EnumMaskBitSet(state, ResourceState::DepthWrite);
    }

    template <class T>
    bool BufferResource<T>::CanImplicitlyDecayToCommonStateFromState(HAL::ResourceState state) const
    {
        // Buffers will decay to Common from any state on any command queue 
        return true;
    }

    template <class T>
    uint32_t BufferResource<T>::SubresourceCount() const
    {
        return 1;
    }

}

