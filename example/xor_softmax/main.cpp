//#define NEU_DISABLE_ASSERTION
#include <iostream>
#include <boost/timer.hpp>
#include <boost/progress.hpp>
#include <neu/vector_io.hpp>
#include <neu/kernel.hpp>
#include <neu/layer/activation/rectifier.hpp>
#include <neu/layer/activation/leaky_rectifier.hpp>
#include <neu/layer/activation/sigmoid.hpp>
#include <neu/layer/activation/softmax_loss.hpp>
#include <neu/optimizer/momentum.hpp>
#include <neu/layer/inner_product.hpp>
#include <neu/layer/bias.hpp>
#include <neu/layer/any_layer.hpp>
#include <neu/layer/any_layer_vector.hpp>
#include <neu/layer/io.hpp>

int main(int argc, char** argv) {
	std::cout << "hello world" << std::endl;
	auto& queue = boost::compute::system::default_queue();
	auto context = boost::compute::system::default_context();

	std::random_device device; std::mt19937 rand(device());
	//std::mt19937 rand(0);

	auto input_dim = 2u;
	auto output_dim = 2u;
	auto batch_size = 4u;

	std::vector<neu::cpu_vector> cpu_input = {
		{0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}
	};
	std::vector<neu::cpu_vector> cpu_teach = {
		{1.f, 0.f}, {0.f, 1.f}, {0.f, 1.f}, {1.f, 0.f}
	};

	const auto input = neu::to_gpu_vector(neu::flatten(cpu_input), queue);
	const auto teach = neu::to_gpu_vector(neu::flatten(cpu_teach), queue);

	auto fc12_g = [&rand, dist=std::normal_distribution<>(0.f, 1.f)]
		() mutable { return dist(rand); };
	auto constant_g = [](){ return 0.f; };

	constexpr neu::scalar base_lr = 0.1f;
	constexpr neu::scalar momentum = 0.9f;
	constexpr neu::scalar weight_decay = 0.f;
	constexpr std::size_t hidden_node_num = 10u;

	std::vector<neu::layer::any_layer> nn;
	nn.push_back(neu::layer::make_inner_product(
		input_dim, hidden_node_num, batch_size, fc12_g,
		neu::optimizer::momentum(base_lr, momentum, weight_decay, input_dim*hidden_node_num, queue),
		queue));
	nn.push_back(neu::layer::make_bias(neu::layer::output_dim(nn), batch_size, constant_g,
		neu::optimizer::momentum(base_lr, momentum, weight_decay,
		neu::layer::output_dim(nn), queue), queue));
	nn.push_back(neu::layer::make_rectifier(neu::layer::output_dim(nn), batch_size));
	//nn.push_back(neu::layer::make_leaky_rectifier(neu::layer::output_dim(nn), batch_size, 0.01));
	//nn.push_back(neu::layer::make_sigmoid(neu::layer::output_dim(nn), batch_size));
	nn.push_back(neu::layer::make_inner_product(
		neu::layer::output_dim(nn), output_dim, batch_size, fc12_g,
		neu::optimizer::momentum(base_lr, momentum, weight_decay,
			neu::layer::output_dim(nn)*output_dim, queue), queue));
	nn.push_back(neu::layer::make_bias(neu::layer::output_dim(nn), batch_size, constant_g,
		neu::optimizer::momentum(base_lr, momentum, weight_decay,
			neu::layer::output_dim(nn), queue), queue));
	nn.push_back(neu::layer::make_softmax_loss(
		neu::layer::output_dim(nn), batch_size, context));

	neu::gpu_vector output(neu::layer::whole_output_size(nn), context);
	neu::gpu_vector prev_delta(neu::layer::whole_input_size(nn), context);

	std::ofstream cel_error_log("cel_error.txt");
	std::ofstream output_log("output.txt");
	std::ofstream del_weight_log("del_weight.txt");

	const auto iteration_limit = 100u;
	boost::progress_display progress(iteration_limit);
	boost::timer timer;
	for(auto i = 0u; i < iteration_limit; ++i) {
		neu::layer::forward(nn, input, output, queue);
		{
			neu::print(output_log, output, output_dim);
			auto cel = neu::range::cross_entropy_loss(output, teach, queue);
			cel_error_log << i << " " << cel << std::endl;
		}
		neu::gpu_vector error(output.size(), context);
		neu::range::calc_last_layer_delta(output, teach, error, queue);
		neu::layer::backward(nn, error, prev_delta, queue); //or neu::layer::backward_top(nn, error, queue);
		neu::layer::update(nn, queue);
		++progress;
	}
	std::cout << timer.elapsed() << " sec" << std::endl;
	neu::print(std::cout, output, output_dim);
	boost::compute::system::finish();
}
