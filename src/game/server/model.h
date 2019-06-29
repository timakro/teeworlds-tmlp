#ifndef GAME_SERVER_MODEL_H
#define GAME_SERVER_MODEL_H

namespace tensorflow {
	class SavedModelBundle;
	class Tensor;
}

class CModel
{
public:
	CModel();
	~CModel();

	void ForwardPass(uint8_t *frame, float *target_mu, float *target_var, float *binary, float *weapon);

	void ReloadModel();
	void ResetState() { m_HasState = false; }

private:
	tensorflow::SavedModelBundle *m_Bundle;
	tensorflow::Tensor *m_State;
	bool m_HasState = false;
};

#endif
