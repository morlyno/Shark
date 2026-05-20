#pragma once

#include "Shark/Core/Base.h"

#include <miniaudio/miniaudio.h>

namespace Shark::Audio {

	void InitailizeVFS(ma_vfs_callbacks* vfs);

}
