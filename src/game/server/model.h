#ifndef GAME_SERVER_MODEL_H
#define GAME_SERVER_MODEL_H

#include <vector>
#include <random>

namespace tensorflow {
	class SavedModelBundle;
	class Tensor;
}

class CModel
{
public:
	CModel();
	~CModel();

	struct Action
	{
		float target_x;
		float target_y;
		bool b_left;
		bool b_right;
		bool b_jump;
		bool b_fire;
		bool b_hook;
		uint8_t weapon;
	};

	void LoadModel();

	void PrepareFrame(int NumBots);
	void FeedFrame(uint8_t *Frame, float *RNNStateIn);
	void ForwardPass();
	void FetchActionAndValue(Action *Act, float *RNNStateOut, float *Value);

private:
	bool SampleBinary(float Value);
	int SampleClass(float *Logits, int N);
	float SampleNormal(float Mu, float Var);

	std::mt19937 m_RandGen;
	std::uniform_real_distribution<float> m_RandF;
	tensorflow::SavedModelBundle *m_Bundle;
	tensorflow::Tensor *m_Input = NULL;
	tensorflow::Tensor *m_RNNStateIn = NULL;
	std::vector<tensorflow::Tensor> m_Outputs;
	int m_CurrentNumBots = -1;
	int m_BotIndex;
};

#endif
