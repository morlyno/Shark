#include "skpch.h"
#include "Utils.h"

namespace Shark {

	std::string Utils::BytesToString(uint64_t bytes)
	{
		static constexpr uint64_t TB = 1024ull * 1024 * 1024 * 1024;
		static constexpr uint64_t GB = 1024 * 1024 * 1024;
		static constexpr uint64_t MB = 1024 * 1024;
		static constexpr uint64_t KB = 1024;

		const auto sizes = {
			std::pair{ TB, "TB"sv },
			std::pair{ GB, "GB"sv },
			std::pair{ MB, "MB"sv },
			std::pair{ KB, "KB"sv }
		};

		for (const auto& [size, suffix] : sizes)
		{
			if (bytes >= size)
			{
				return fmt::format("{0:.2f} {1}", (float)bytes / size, suffix);
			}
		}
		return fmt::format("{0} bytes", bytes);

#if 0
		if (bytes > (1024 * 1024 * 1024))
			return fmt::format("{0:.2f} GB", (float)bytes / (1024 * 1024 * 1024));

		if (bytes > (1024 * 1024))
			return fmt::format("{0:.2f} MB", (float)bytes / (1024 * 1024));

		if (bytes > (1024))
			return fmt::format("{0:.2f} KB", (float)bytes / (1024));

		return fmt::format("{0} bytes", bytes);
#endif
	}

}
