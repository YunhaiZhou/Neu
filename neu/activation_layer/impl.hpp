#ifndef NEU_ACTIVATION_LAYER_IMPL_HPP
#define NEU_ACTIVATION_LAYER_IMPL_HPP
//20151025
#include <neu/assert.hpp>
#include <neu/basic_type.hpp>
#include <neu/validation.hpp>
#include <neu/kernel.hpp>
namespace neu {
	template<typename ActivationFunc, typename DiffActivationFunc>
	class activation_layer {
	public:
		activation_layer(
			std::size_t input_dim, std::size_t output_dim, std::size_t batch_size,
			ActivationFunc const& activation_func,
			DiffActivationFunc const& diff_activation_func)
		: input_dim_(input_dim), output_dim_(output_dim), batch_size_(batch_size),
		activation_func_(activation_func), diff_activation_func_(diff_activation_func),
		input_(input_dim*batch_size), next_input_(output_dim*batch_size),
		prev_delta_(input_dim*batch_size), df_(input_dim*batch_size) {}

		decltype(auto) forward(gpu_vector const& input) {
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(input));
			input_ = input;
			next_input_ = activation_func_(input);
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(next_input_));
		}
		decltype(auto) get_next_input() const { return (next_input_); }

		decltype(auto) backward(gpu_vector const& delta) {
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(delta));
			df_ = diff_activation_func_(input_);
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(df_));
			boost::compute::transform(df_.begin(), df_.end(), delta.begin(),
				prev_delta_.begin(), boost::compute::multiplies<scalar>());
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(prev_delta_));
		}
		decltype(auto) get_prev_delta() const { return (prev_delta_); }

	private:
		std::size_t input_dim_, output_dim_, batch_size_;
		ActivationFunc activation_func_;
		DiffActivationFunc diff_activation_func_;
		gpu_vector input_, next_input_, prev_delta_, df_;
	};
	template<typename ActivationFunc>
	decltype(auto) make_activation_layer(
			std::size_t input_dim, std::size_t output_dim, std::size_t batch_size,
			ActivationFunc const& activation_func) {
		return activation_layer<ActivationFunc, derivative<ActivationFunc>>(
			input_dim, output_dim, batch_size,
			activation_func, derivative<ActivationFunc>());
	}
	template<typename ActivationFunc>
	decltype(auto) make_activation_layer(
			std::size_t input_dim, std::size_t batch_size) {
		return activation_layer<ActivationFunc, derivative<ActivationFunc>>(
			input_dim, input_dim, batch_size,
			ActivationFunc(input_dim, batch_size),
			derivative<ActivationFunc>(input_dim, batch_size));
	}
}// namespace neu

#endif //NEU_ACTIVATION_LAYER_IMPL_HPP
