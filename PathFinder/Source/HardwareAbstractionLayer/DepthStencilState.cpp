#include "DepthStencilState.hpp"

#include <d3d12.h>

namespace HAL
{

    void DepthStencilState::SetDepthTestEnabled(bool enabled)
    {
        mDesc.DepthEnable = enabled;
    }

    void DepthStencilState::SetDepthWriteEnabled(bool enabled)
    {
        mDesc.DepthWriteMask = enabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    }

    void DepthStencilState::SetComparisonFunction(ComparisonFunction function)
    {
        switch (function)
        {
        case ComparisonFunction::Never: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER; break;
        case ComparisonFunction::Less: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; break;
        case ComparisonFunction::Equal: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL; break;
        case ComparisonFunction::LessOrEqual: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; break;
        case ComparisonFunction::Greater: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER; break;
        case ComparisonFunction::NotEqual: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL; break;
        case ComparisonFunction::GreaterOrEqual: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL; break;
        case ComparisonFunction::Always: mDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS; break;
        }
    }

}