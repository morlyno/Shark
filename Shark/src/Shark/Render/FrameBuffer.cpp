#include "skpch.h"
#include "FrameBuffer.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

    Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecification& specs)
    {
        return RendererCommand::CreateFrameBuffer(specs);
    }

}
