#include "utilities/shannon_entropy.h"

#include <cmath>

namespace utilities
{

float shannon_entropy::finalize() const noexcept
{
	float entropy{};
	const float length_over_1 = 1.f / total_length_;
	for (auto count : byte_count_)
	{
		float probability = count * length_over_1;
		if (probability > 0.f)
			entropy += std::abs(probability * std::log2f(probability));
	}

	return entropy;
}

} //namespace utilities
