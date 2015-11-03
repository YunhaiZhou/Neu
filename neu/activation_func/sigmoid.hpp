#ifndef NEU_ACTIVATION_FUNC_SIGMOID_HPP
#define NEU_ACTIVATION_FUNC_SIGMOID_HPP
//20150528
#include <neu/assert.hpp>
#include <neu/validation.hpp>
#include <neu/activation_func/derivative.hpp>
#include <neu/range_algorithm.hpp>
namespace neu {
	BOOST_COMPUTE_FUNCTION(float, sigmoid_kernel, (float x), {
		return 1./(1.+exp(-x));
	});
	BOOST_COMPUTE_FUNCTION(float, derivative_sigmoid_kernel, (float x), {
		const float sigma = 1./(1.+exp(-x));
		return sigma*(1.-sigma);
	});
	class sigmoid {
	public:
		template<typename InputRange, typename OutputRange>
		decltype(auto) operator()(InputRange const& input, OutputRange const& output) {
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(input));
			neu::range_transform(input, output, sigmoid_kernel);
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(output));
		}
	};
	template<>
	class derivative<sigmoid> {
	public:
		template<typename InputRange, typename OutputRange>
		decltype(auto) operator()(InputRange const& input, OutputRange const& output) {
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(input));
			neu::range_transform(input, output, neu::derivative_sigmoid_kernel);
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(output));
		}
	};
}// namespace neu

#endif //NEU_ACTIVATION_FUNC_SIGMOID_HPP
