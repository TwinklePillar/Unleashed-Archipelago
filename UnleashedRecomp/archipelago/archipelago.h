#ifndef SWA_AP_H
#define SWA_AP_H

#include <queue>
#include <SWA.h>

#include <Archipelago.h>

#include "classmaps.h"

#define AP_MESSAGE_ACH 12407
#define AP_ITEM_MAX 512

namespace SWAAP {

struct ArchipelagoMessage {
	std::string header;
	std::string message;
	uint16_t imgID;
};

struct ActiveTrap {
	int trapID;
	double timer;
};

enum APMessageType {
	SENT_ITEM,
	RECV_ITEM,
	MEMORY_DEBUG,
	MISC
};

class ArchipelagoManager {
	public:
		static bool hasInit;
		
		static void ItemClearCallback();
		static void ItemRecvCallback(int64_t item, bool notify);
		static void LocationCheckCallback(int64_t location);
		
		static void SetAPVersionCallback(int version);
		static void SetProgressionModeCallback(int progression);
		static void SetActItemsCallback(int ai);
		static void SetActOrderCallback(std::map<int, int> actorder);

		static void DebugLog(std::string buf);
		
		static std::string ipport;
		static std::string slot;
		static std::string password;
		
		static std::queue<int> trapQueue;
		static std::queue<int> prioTrapQueue;
		static double trapTimer;
		
		static bool isDebug;
		static int version;
		static int progressionMode;
		static int actItems;
		static int itemState[AP_ITEM_MAX];
		static std::map<std::string, int> actLevels;
		static inline std::queue<ArchipelagoMessage> messageQueue;
		static void Update(double dt, PPCContext& ctx, uint8_t* base);
		static void DisplayMessage(APMessageType type, std::string message, uint16_t imgID = 0);
		static void ConnectToArchipelago();
		static void DisconnectFromArchipelago();

		static xpointer<CEvilSonicContext> evilContext;

		static std::vector<ActiveTrap> activeTraps;
		static bool isTrapActive(int trapID);
};

}

#endif
