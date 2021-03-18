#pragma once

namespace Shark {

	struct TagComponent
	{
		TagComponent() = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
		~TagComponent() = default;

		std::string Tag;
	};

}
