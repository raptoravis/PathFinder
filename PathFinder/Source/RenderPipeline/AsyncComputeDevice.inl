#include "../Foundation/StringUtils.hpp"

namespace PathFinder
{

    template <class CommandListT, class CommandQueueT>
    AsyncComputeDevice<CommandListT, CommandQueueT>::AsyncComputeDevice(
        const HAL::Device& device,
        const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
        Memory::PoolCommandListAllocator* commandListAllocator,
        Memory::ResourceStateTracker* resourceStateTracker,
        PipelineResourceStorage* resourceStorage, 
        PipelineStateManager* pipelineStateManager,
        const RenderSurfaceDescription& defaultRenderSurface)
        :
        mCommandQueue{ device },
        mUniversalGPUDescriptorHeap{ universalGPUDescriptorHeap },
        mCommandListAllocator{ commandListAllocator },
        mResourceStateTracker{ resourceStateTracker },
        mResourceStorage{ resourceStorage },
        mPipelineStateManager{ pipelineStateManager },
        mDefaultRenderSurface{ defaultRenderSurface }
    {
        mCommandQueue.SetDebugName("Async Compute Device Command Queue");
        AllocateNewCommandList(); 
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::BindBuffer(
        Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        Memory::Buffer* resource = mResourceStorage->GetBufferResource(resourceName);
        assert_format(resource, "Buffer ' ", resourceName.ToString(), "' doesn't exist");

        BindExternalBuffer(*resource, shaderRegister, registerSpace, registerType);
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::BindExternalBuffer(
        const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        if (mAppliedComputeState || mAppliedRayTracingState)
        {
            const HAL::RootSignature* signature = mAppliedComputeState ?
                mAppliedComputeState->GetRootSignature() : mAppliedRayTracingState->GetGlobalRootSignature();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist. It either wasn't created or register/space/type aren't correctly specified.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ConstantBuffer: 
                mCommandList->SetComputeRootConstantBuffer(*buffer.HALBuffer(), index->IndexInSignature);
                break;
            case HAL::ShaderRegister::ShaderResource: 
                mCommandList->SetComputeRootDescriptorTable(buffer.GetSRDescriptor()->GPUAddress(), index->IndexInSignature); 
                break;
            case HAL::ShaderRegister::UnorderedAccess:
                mCommandList->SetComputeRootDescriptorTable(buffer.GetUADescriptor()->GPUAddress(), index->IndexInSignature);
                break;
            case HAL::ShaderRegister::Sampler:
                assert_format(false, "Incompatible register type");
            }

            return;
        }
        else {
            assert_format(false, "No pipeline state applied");
        }
    }

    template <class CommandListT, class CommandQueueT>
    template <class T>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::SetRootConstants(
        const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        const HAL::RootSignature* signature = mAppliedComputeState ?
            mAppliedComputeState->GetRootSignature() : mAppliedRayTracingState->GetGlobalRootSignature();

        auto index = signature->GetParameterIndex({shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
        assert_format(index, "Root signature parameter doesn't exist");
        mCommandList->SetComputeRootConstants(constants, index->IndexInSignature);
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::ApplyCommonComputeResourceBindingsIfNeeded()
    {
        auto commonParametersIndexOffset = mAppliedComputeRootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        if (mRebindingAfterSignatureChangeRequired)
        {
            // Look at PipelineStateManager for base root signature parameter ordering
            HAL::DescriptorAddress SRRangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::ShaderResource);
            HAL::DescriptorAddress UARangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::UnorderedAccess);

            // Alias different registers to one GPU address
            mCommandList->SetComputeRootDescriptorTable(SRRangeAddress, 3 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(SRRangeAddress, 4 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(SRRangeAddress, 5 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(SRRangeAddress, 6 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(SRRangeAddress, 7 + commonParametersIndexOffset);

            mCommandList->SetComputeRootDescriptorTable(UARangeAddress, 8 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(UARangeAddress, 9 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(UARangeAddress, 10 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(UARangeAddress, 11 + commonParametersIndexOffset);
            mCommandList->SetComputeRootDescriptorTable(UARangeAddress, 12 + commonParametersIndexOffset);
        }

        if (auto buffer = mResourceStorage->GlobalRootConstantsBuffer();
            buffer && (buffer->HALBuffer() != mBoundGlobalConstantBufferCompute || mRebindingAfterSignatureChangeRequired))
        {
            mBoundGlobalConstantBufferCompute = buffer->HALBuffer();
            mCommandList->SetComputeRootConstantBuffer(*mBoundGlobalConstantBufferCompute, 0 + commonParametersIndexOffset);
        }

        if (auto buffer = mResourceStorage->PerFrameRootConstantsBuffer();
            buffer && (buffer->HALBuffer() != mBoundFrameConstantBufferCompute || mRebindingAfterSignatureChangeRequired))
        {
            mBoundFrameConstantBufferCompute = buffer->HALBuffer();
            mCommandList->SetComputeRootConstantBuffer(*mBoundFrameConstantBufferCompute, 1 + commonParametersIndexOffset);
        }

        if (auto address = mResourceStorage->RootConstantsBufferAddressForCurrentPass();
            address != mBoundFrameConstantBufferAddressCompute || mRebindingAfterSignatureChangeRequired)
        {
            mBoundFrameConstantBufferAddressCompute = address;
            mCommandList->SetComputeRootConstantBuffer(address, 2 + commonParametersIndexOffset);
        }

        if (auto buffer = mResourceStorage->DebugBufferForCurrentPass();
            buffer && (buffer->HALBuffer() != mBoundPassDebugBufferCompute || mRebindingAfterSignatureChangeRequired))
        {
            mBoundPassDebugBufferCompute = buffer->HALBuffer();
            mCommandList->SetComputeRootUnorderedAccessResource(*mBoundPassDebugBufferCompute, 13 + commonParametersIndexOffset);
        }

        mRebindingAfterSignatureChangeRequired = false;
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::AllocateNewCommandList()
    {
        mCommandList = mCommandListAllocator->AllocateCommandList<CommandListT>();
        mCommandList->SetDescriptorHeap(*mUniversalGPUDescriptorHeap);
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::ExecuteCommands(const HAL::Fence* fenceToWaitFor, const HAL::Fence* fenceToSignal)
    {
        if (fenceToWaitFor)
        {
            mCommandQueue.WaitFence(*fenceToWaitFor);
        }

        mCommandList->Close();
        mCommandQueue.ExecuteCommandList(*mCommandList);

        if (fenceToSignal)
        {
            mCommandQueue.SignalFence(*fenceToSignal);
        }   

        AllocateNewCommandList();
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        ApplyCommonComputeResourceBindingsIfNeeded();
        InsertResourceTransitionsIfNeeded();
        
        mCommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
        mResourceStorage->AllowCurrentPassConstantBufferSingleOffsetAdvancement();
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::DispatchRays(uint32_t width, uint32_t height, uint32_t depth)
    {
        assert_format(mAppliedRayTracingState && mAppliedRayTracingDispatchInfo, "No Ray Tracing state / dispatch info were applied before Ray Dispatch");

        HAL::RayDispatchInfo dispatchInfo = *mAppliedRayTracingDispatchInfo;
        dispatchInfo.SetWidth(width);
        dispatchInfo.SetHeight(height);
        dispatchInfo.SetDepth(depth);

        ApplyCommonComputeResourceBindingsIfNeeded();
        InsertResourceTransitionsIfNeeded();

        mCommandList->DispatchRays(dispatchInfo);
        mResourceStorage->AllowCurrentPassConstantBufferSingleOffsetAdvancement();
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::ApplyPipelineState(Foundation::Name psoName)
    {
        std::optional<PipelineStateManager::PipelineStateVariant> state = mPipelineStateManager->GetPipelineState(psoName);
        assert_format(state, "Pipeline state doesn't exist");
        assert_format(!state->GraphicPSO, "Trying to apply graphics pipeline state to compute device");

        if (state->ComputePSO) ApplyStateIfNeeded(state->ComputePSO);
        else if (state->RayTracingPSO) ApplyStateIfNeeded(state->RayTracingPSO, state->BaseRayDispatchInfo);
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::ApplyStateIfNeeded(const HAL::ComputePipelineState* state)
    {
        bool computeStateApplied = mAppliedComputeState != nullptr;

        // State is already applied
        //
        if (computeStateApplied && mAppliedComputeState == state) return;

        mCommandList->SetPipelineState(*state);

        mRebindingAfterSignatureChangeRequired = state->GetRootSignature() != mAppliedComputeRootSignature;

        if (mRebindingAfterSignatureChangeRequired)
        {
            mCommandList->SetComputeRootSignature(*state->GetRootSignature());
        }

        mAppliedComputeRootSignature = state->GetRootSignature();
        mAppliedComputeState = state;
        mAppliedRayTracingState = nullptr;
        mAppliedRayTracingDispatchInfo = nullptr;
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state, const HAL::RayDispatchInfo* dispatchInfo)
    {
        bool rayTracingStateApplied = mAppliedRayTracingState != nullptr;

        // State is already applied
        //
        if (rayTracingStateApplied && mAppliedRayTracingState == state) return;

        // Set RT state
        mCommandList->SetPipelineState(*state);

        mRebindingAfterSignatureChangeRequired = state->GetGlobalRootSignature() != mAppliedComputeRootSignature;

        if (mRebindingAfterSignatureChangeRequired)
        {
            mCommandList->SetComputeRootSignature(*state->GetGlobalRootSignature());
        }

        mAppliedComputeRootSignature = state->GetGlobalRootSignature();
        mAppliedRayTracingState = state;
        mAppliedRayTracingDispatchInfo = dispatchInfo;
        mAppliedComputeState = nullptr;
    }

    template <class CommandListT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandQueueT>::InsertResourceTransitionsIfNeeded()
    {
        auto& passNode = mResourceStorage->CurrentPassGraphNode();
        bool passChanged = !(mLastDetectedPassName == passNode.PassMetadata.Name);
        
        HAL::ResourceBarrierCollection barriers{};

        auto passName = mResourceStorage->CurrentPassGraphNode().PassMetadata.Name.ToString();

        if (passChanged)
        {
            // In case pass had changed, we gather resource transitions to be applied for new pass
            mResourceStorage->RequestResourceTransitionsToCurrentPassStates();
            barriers.AddBarriers(mResourceStorage->AliasingBarriersForCurrentPass());
        }

        // Add any transitions that may have been requested for external resources (assets)
        // for example copy dest. to srv or cbv 
        bool isFirstPass = passNode.ContextualExecutionIndex == 0;
        barriers.AddBarriers(mResourceStateTracker->ApplyRequestedTransitions(isFirstPass));
        barriers.AddBarriers(mUABarriersToApply);
        mCommandList->InsertBarriers(barriers);

        mLastDetectedPassName = passNode.PassMetadata.Name;

        // We only use UA barriers of current pass for second draw/dispatch and onwards.
        // First draw/dispatch uses UA barriers from the previous render pass
        // so that transition and UA barriers could be merged correctly into one graphics API call
        mUABarriersToApply = mResourceStorage->UnorderedAccessBarriersForCurrentPass();
    }

}

