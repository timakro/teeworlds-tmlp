#include "tensorflow/cc/saved_model/loader.h"
#include "tensorflow/cc/saved_model/tag_constants.h"
#include "tensorflow/cc/framework/ops.h"

#include "gamecontext.h"
#include "model.h"

namespace tf = tensorflow;

CModel::CModel()
{
	m_Bundle = new tf::SavedModelBundle();
	m_State = new tf::Tensor();

	tf::SavedModelBundle bundle;
	tf::LoadSavedModel(tf::SessionOptions(), tf::RunOptions(), "../testsave", {tf::kSavedModelTagServe}, m_Bundle);

	/*uint8_t raw_input[90*50] = {0};
	tf::Tensor input(tf::DT_UINT8, tf::TensorShape({1, 1, 50, 90}));
	mem_copy(input.flat<uint8_t>().data(), raw_input, 90*50);
	//dbg_msg("stuff", "%d %d %d %d %d", input.tensor<uint8_t,4>()(0,0,4,4), input.tensor<uint8_t,4>()(0,0,5,4), input.tensor<uint8_t,4>()(0,0,3,4), input.tensor<uint8_t,4>()(0,0,4,5), input.tensor<uint8_t,4>()(0,0,4,3));
	std::vector<tf::Tensor> outputs;

	{
	tf::Status run_status = bundle.session->Run({{"input", input}}, {"target_mu/Tanh", "binary/Sigmoid", "weapon/truediv", "state_out"}, {}, &outputs);
	if (!run_status.ok()) {
		std::cout << "Running model failed: " << run_status << std::endl;
	}

	tf::TTypes<float_t>::Flat target = outputs[0].flat<float_t>();
	tf::TTypes<float_t>::Flat binary = outputs[1].flat<float_t>();
	tf::TTypes<float_t>::Flat weapon = outputs[2].flat<float_t>();
	dbg_msg("stuff", "%f %f %f %f %f %f", weapon(0), weapon(1), weapon(2), weapon(3), weapon(4), weapon(5));
	}

	tf::Tensor out;

	{
	tf::Status run_status = bundle.session->Run({{"input", input}, {"state_in", out}}, {"target_mu/Tanh", "binary/Sigmoid", "weapon/truediv", "state_out"}, {}, &outputs);
	if (!run_status.ok()) {
		std::cout << "Running model failed: " << run_status << std::endl;
	}

	tf::TTypes<float_t>::Flat target = outputs[0].flat<float_t>();
	tf::TTypes<float_t>::Flat binary = outputs[1].flat<float_t>();
	tf::TTypes<float_t>::Flat weapon = outputs[2].flat<float_t>();
	dbg_msg("stuff", "%f %f %f %f %f %f", weapon(0), weapon(1), weapon(2), weapon(3), weapon(4), weapon(5));
	}*/
}

CModel::~CModel()
{
	delete m_Bundle;
	m_Bundle = NULL;
	delete m_State;
	m_State = NULL;
}

void CModel::ForwardPass(uint8_t *frame, float *target_mu, float *target_var, float *binary, float *weapon)
{
	tf::Tensor input(tf::DT_UINT8, tf::TensorShape({1, 1, 50, 90}));
	mem_copy(input.flat<uint8_t>().data(), frame, 90*50);

	std::vector<tf::Tensor> outputs;
	tf::Status run_status;
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	if(!m_HasState)
		run_status = m_Bundle->session->Run({{"input", input}},
			{"target_mu/Tanh", "target_var/Softplus", "binary/Sigmoid", "weapon/truediv", "state_out"}, {}, &outputs);
	else
		run_status = m_Bundle->session->Run({{"input", input}, {"state_in", *m_State}},
			{"target_mu/Tanh", "target_var/Softplus", "binary/Sigmoid", "weapon/truediv", "state_out"}, {}, &outputs);

	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> time_span = t2 - t1;
	std::cout << "It took me " << time_span.count() << " milliseconds." << std::endl;

	if (!run_status.ok()) {
		std::cout << "Running model failed: " << run_status << std::endl;
		return;
	}

	mem_copy(target_mu, outputs[0].flat<float_t>().data(), 2*sizeof(float));
	mem_copy(target_var, outputs[1].flat<float_t>().data(), 2*sizeof(float));
	mem_copy(binary, outputs[2].flat<float_t>().data(), 5*sizeof(float));
	mem_copy(weapon, outputs[3].flat<float_t>().data(), 5*sizeof(float));

	*m_State = outputs[4];
	m_HasState = true;
}

void CModel::ReloadModel()
{
}
