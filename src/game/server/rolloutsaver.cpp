#include <sys/stat.h>

#include <engine/shared/config.h>
#include <base/system.h>
#include "rolloutsaver.h"

void CRolloutSaver::NewRollout(int BotID)
{
	mkdir(g_Config.m_TMLP_RolloutDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	char FilePath[512];
	str_format(FilePath, sizeof(FilePath), "%s/%04d-%02d", g_Config.m_TMLP_RolloutDir, g_Config.m_TMLP_ServerID, BotID);
	char *PrefixEnd = FilePath + str_length(FilePath);

	str_copy(PrefixEnd, "-state.gz", 128);
	m_StateFile = gzopen(FilePath, "wb");
	str_copy(PrefixEnd, "-action.gz", 128);
	m_ActionFile = gzopen(FilePath, "wb");
}

void CRolloutSaver::WriteState(uint8_t *RenderData)
{
	gzwrite(m_StateFile, RenderData, 90*50);
}

void CRolloutSaver::WriteActionAndValue(CModel::Action *Action, float Value)
{
	gzwrite(m_ActionFile, &Action->target_x, sizeof(CModel::Action::target_x));
	gzwrite(m_ActionFile, &Action->target_y, sizeof(CModel::Action::target_y));
	gzwrite(m_ActionFile, &Action->b_left, sizeof(CModel::Action::b_left));
	gzwrite(m_ActionFile, &Action->b_right, sizeof(CModel::Action::b_right));
	gzwrite(m_ActionFile, &Action->b_jump, sizeof(CModel::Action::b_jump));
	gzwrite(m_ActionFile, &Action->b_fire, sizeof(CModel::Action::b_fire));
	gzwrite(m_ActionFile, &Action->b_hook, sizeof(CModel::Action::b_hook));
	gzwrite(m_ActionFile, &Action->weapon, sizeof(CModel::Action::weapon));

	gzwrite(m_ActionFile, &Value, sizeof(float));
}

void CRolloutSaver::WriteReward(float Reward)
{
	gzwrite(m_ActionFile, &Reward, sizeof(float));
}

void CRolloutSaver::CloseRollout()
{
	gzclose(m_StateFile);
	gzclose(m_ActionFile);
}
