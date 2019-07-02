#include "tensorflow/cc/saved_model/loader.h"
#include "tensorflow/cc/saved_model/tag_constants.h"
#include "tensorflow/cc/framework/ops.h"

#include <engine/shared/config.h>
#include "gamecontext.h"
#include "model.h"

namespace tf = tensorflow;

CModel::CModel()
: m_RandGen(std::random_device()())
{
	m_Bundle = new tf::SavedModelBundle();
}

CModel::~CModel()
{
	delete m_Bundle;
	m_Bundle = NULL;
	delete m_Input;
	m_Input = NULL;
	delete m_StateIn;
	m_StateIn = NULL;
}

void CModel::LoadModel()
{
	tf::Status run_status = tf::LoadSavedModel(tf::SessionOptions(), tf::RunOptions(), g_Config.m_TMLP_ModelPath, {tf::kSavedModelTagServe}, m_Bundle);
	if(!run_status.ok())
		std::cout << "Loading model failed: " << run_status << std::endl;
}

bool CModel::SampleBinary(float Value)
{
	if(g_Config.m_TMLP_TrainingMode)
		return Value > m_RandF(m_RandGen);
	else
		return Value > 0.5;
}

int CModel::SampleClass(float *Logits, int N)
{
	if(g_Config.m_TMLP_TrainingMode)
	{
		float Rand = m_RandF(m_RandGen);
		float Sum = 0;
		for(int i = 0; i < N-1; i++)
		{
			Sum += Logits[i];
			if(Rand < Sum)
				return i;
		}
		return N-1;
	}
	else
	{
		int Class;
		float Max = -1;
		for(int i = 0; i < N; i++)
		{
			if(Logits[i] > Max)
			{
				Max = Logits[i];
				Class = i;
			}
		}
		return Class;
	}
}

float CModel::SampleNormal(float Mu, float Var)
{
	if(g_Config.m_TMLP_TrainingMode)
	{
		std::normal_distribution<float> NormalDist(Mu, sqrt(Var));
		return clamp(NormalDist(m_RandGen), 0.0f, 1.0f);
	}
	else
	{
		return Mu;
	}
}

void CModel::PrepareFrame(int NumBots)
{
	m_BotIndex = 0;

	if(NumBots == m_CurrentNumBots)
		return;

	delete m_Input;
	m_Input = new tf::Tensor(tf::DT_UINT8, tf::TensorShape({NumBots, 1, 50, 90}));
	delete m_StateIn;
	m_StateIn = new tf::Tensor(tf::DT_FLOAT, tf::TensorShape({NumBots, g_Config.m_TMLP_LSTMUnits*2}));

	m_CurrentNumBots = NumBots;
}

void CModel::FeedFrame(uint8_t *Frame, float *State)
{
	mem_copy(m_Input->flat<uint8_t>().data() + m_BotIndex*90*50, Frame, 90*50);
	mem_copy(m_StateIn->flat<float>().data() + m_BotIndex*g_Config.m_TMLP_LSTMUnits*2, State, g_Config.m_TMLP_LSTMUnits*2*sizeof(float));

	m_BotIndex++;
}

void CModel::ForwardPass()
{
	//std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

	m_BotIndex = 0;
	tf::Status run_status = m_Bundle->session->Run({{"input", *m_Input}, {"state_in", *m_StateIn}},
		{"target_mu/Tanh", "target_var/Softplus", "binary/Sigmoid", "weapon/truediv", "state_out"}, {}, &m_Outputs);

	if(!run_status.ok())
		std::cout << "Running model failed: " << run_status << std::endl;

	/*std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> time_span = t2 - t1;
	std::cout << "It took me " << time_span.count() << " milliseconds." << std::endl;*/
}

void CModel::FetchAction(Action *Act, float *State)
{
	float *target_mu =  m_Outputs[0].flat<float>().data() + m_BotIndex*2;
	float *target_var =  m_Outputs[1].flat<float>().data() + m_BotIndex*2;
	float *binary =  m_Outputs[2].flat<float>().data() + m_BotIndex*5;
	float *weapon =  m_Outputs[3].flat<float>().data() + m_BotIndex*5;

	Act->target_x = SampleNormal(target_mu[0], target_var[0]);
	Act->target_y = SampleNormal(target_mu[1], target_var[1]);
	Act->b_left = SampleBinary(binary[0]);
	Act->b_right = SampleBinary(binary[1]);
	Act->b_jump = SampleBinary(binary[2]);
	Act->b_fire = SampleBinary(binary[3]);
	Act->b_hook = SampleBinary(binary[4]);
	Act->weapon = SampleClass(weapon, 5);

	mem_copy(State, m_Outputs[4].flat<float>().data() + m_BotIndex*g_Config.m_TMLP_LSTMUnits*2, g_Config.m_TMLP_LSTMUnits*2*sizeof(float));

	m_BotIndex++;
}
