#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <fstream>

#include <engine/shared/config.h>
#include "gamecontext.h"
#include "gplogger.h"

std::string escape_json(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

CGameplayLogger::~CGameplayLogger()
{
	AbortSession();
}

// Called with player IP address
/*
		char PlayerAddr[NETADDR_MAXSTRSIZE];
		Server()->GetClientAddr(m_ClientID, PlayerAddr, NETADDR_MAXSTRSIZE);
		m_gpLogger.NewSession(m_ClientID, Server()->ClientName(m_ClientID), (const char *)PlayerAddr);
*/
void CGameplayLogger::NewSession(int ClientID, const char *PlayerName, const char *PlayerAddr)
{
	AbortSession();

	m_StartTime = time(NULL);
	m_FrameCount = 0;
	m_ClientID = ClientID;
	str_copy(m_PlayerName, PlayerName, MAX_NAME_LENGTH);
	str_copy(m_PlayerAddr, PlayerAddr, NETADDR_MAXSTRSIZE);

	// start time, domain, serverid, cid, random
	char Random[9];
	RandomStr(Random, 8);
	str_format(m_SessionID, sizeof(m_SessionID), "%x-%s-%d-%d-%s", m_StartTime, g_Config.m_TMLP_Domain, g_Config.m_TMLP_ServerID, m_ClientID, Random);
	dbg_msg("gplogger", "New session %s", m_SessionID);

	str_format(m_SessionTmpDir, sizeof(m_SessionTmpDir), "%s/%s", g_Config.m_TMLP_TmpDir, m_SessionID);
	if (mkdir(m_SessionTmpDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
		return;

	str_format(m_DataFilePath, sizeof(m_DataFilePath), "%s/%s", m_SessionTmpDir, "/data.gz");
	str_format(m_LabelsFilePath, sizeof(m_LabelsFilePath), "%s/%s", m_SessionTmpDir, "/labels.gz");
	m_DataFile = gzopen(m_DataFilePath, "wb");
	m_LabelsFile = gzopen(m_LabelsFilePath, "wb");

	m_Active = true;
}

void CGameplayLogger::AddFrame(uint8_t *RenderData, CInputData *InputData)
{
	/* 25 fps */
	if (!m_Active)
		return;

	// Data: 4500 bytes
	gzwrite(m_DataFile, RenderData, 90*50);

	// Labels: 13 bytes
	gzwrite(m_LabelsFile, &InputData->TargetX, sizeof(CInputData::TargetX));
	gzwrite(m_LabelsFile, &InputData->TargetY, sizeof(CInputData::TargetY));
	gzwrite(m_LabelsFile, &InputData->Direction, sizeof(CInputData::Direction));
	gzwrite(m_LabelsFile, &InputData->Weapon, sizeof(CInputData::Weapon));
	gzwrite(m_LabelsFile, &InputData->Jump, sizeof(CInputData::Jump));
	gzwrite(m_LabelsFile, &InputData->Fire, sizeof(CInputData::Fire));
	gzwrite(m_LabelsFile, &InputData->Hook, sizeof(CInputData::Hook));

	m_FrameCount += 1;
}

// Called with round ID
/*
	char RoundID[64];
	CGameplayLogger::GenRoundID(RoundID, sizeof(RoundID));
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->m_gpLogger.SaveSession(GameServer()->m_apPlayers[i]->m_Score, RoundID);
		}
	}
*/
void CGameplayLogger::SaveSession(int Score, const char* RoundID)
{
	if (!m_Active)
		return;

	gzclose(m_DataFile);
	gzclose(m_LabelsFile);

	char MetaFilePath[340];
	str_format(MetaFilePath, sizeof(MetaFilePath), "%s/%s", m_SessionTmpDir, "/meta.json");
	std::ofstream MetaFile;
	MetaFile.open(MetaFilePath);
	MetaFile <<   "{ \"version\": 1";
	MetaFile << "\n, \"score\": " << Score;
	MetaFile << "\n, \"start_time\": " << m_StartTime;
	MetaFile << "\n, \"end_time\": " << time(NULL);
	MetaFile << "\n, \"volume\": " << m_FrameCount;
	MetaFile << "\n, \"cid\": " << m_ClientID;
	MetaFile << "\n, \"player\": \"" << escape_json(m_PlayerName) << "\"";
	MetaFile << "\n, \"ip\": \"" << escape_json(m_PlayerAddr) << "\"";
	MetaFile << "\n, \"domain\": \"" << escape_json(g_Config.m_TMLP_Domain) << "\"";
	MetaFile << "\n, \"serverid\": " << g_Config.m_TMLP_ServerID;
	MetaFile << "\n, \"map\": \"" << escape_json(g_Config.m_SvMap) << "\"";
	MetaFile << "\n, \"gametype\": \"" << escape_json(g_Config.m_SvGametype) << "\"";
	MetaFile << "\n, \"sessionid\": \"" << escape_json(m_SessionID) << "\"";
	MetaFile << "\n, \"roundid\": \"" << escape_json(RoundID) << "\"";
	MetaFile << "\n}";
	MetaFile.close();

	char SessionDir[320];
	str_format(SessionDir, sizeof(SessionDir), "%s/%s", g_Config.m_TMLP_Dir, m_SessionID);
	rename(m_SessionTmpDir, SessionDir);

	m_Active = false;
}

void CGameplayLogger::AbortSession()
{
	if (!m_Active)
		return;

	gzclose(m_DataFile);
	gzclose(m_LabelsFile);
	unlink(m_DataFilePath);
	unlink(m_LabelsFilePath);
	rmdir(m_SessionTmpDir);

	m_Active = false;
}

void CGameplayLogger::GenRoundID(char *buffer, int len)
{
	char Random[9];
	RandomStr(Random, 8);
	str_format(buffer, len, "%x-%s-%d-%s", time(NULL), g_Config.m_TMLP_Domain, g_Config.m_TMLP_ServerID, Random);
}

void CGameplayLogger::RandomStr(char *str, int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	for (int i = 0; i < len; i++)
	{
		str[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}
	str[len] = '\0';
}
