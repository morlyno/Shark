#pragma once

#include "Shark/Asset/Asset.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	// TODO(moro): think about a better way to allow a RendererResource to also be an Asset
	class RendererResource : public Asset
	{
	public:
		virtual nvrhi::ResourceHandle GetResourceHandle() const = 0;
	};

}
