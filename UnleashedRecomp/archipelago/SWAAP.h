#ifndef SWA_AP_H
#define SWA_AP_H

#include <queue>
#include <SWA.h>

#include <Archipelago.h>
#include <json/json.h>

#include "classmaps.h"

#define AP_MESSAGE_ACH 12407
#define AP_ITEM_MAX 512

namespace SWAAP {

struct ArchipelagoMessage {
	std::string header;
	std::string message;
	uint16_t imgID;
};

enum APMessageType {
	SENT_ITEM,
	RECV_ITEM,
	DEATH_LINK,
	TRAP_LINK,
	MEMORY_DEBUG,
	MISC
};

struct APSharedLocation {
	int64_t id;
	virtual bool IsCurrentLocation(std::string type, int stage, int index);

	static void CheckLocation(std::string type, int stage, int index);
	static bool IsLocationChecked(std::string type, int stage, int index);
};

struct InventoryLocation : APSharedLocation {
	int stage;
	int index;
	inline InventoryLocation(int64_t id, int stage, int index) {
		this->id = id;
		this->stage = stage;
		this->index = index;
	}
	bool IsCurrentLocation(std::string type, int stage, int index) override;
};

struct ShoeLocation : APSharedLocation {
	int index;
	inline ShoeLocation(int64_t id, int index) {
		this->id = id;
		this->index = index;
	}
	bool IsCurrentLocation(std::string type, int stage, int index) override;
};

struct CompletionLocation : APSharedLocation {
	int stage; // Stage ID as in the save file
	int rank; // i guess?
	inline CompletionLocation(int64_t id, int stage, int rank) {
		this->id = id;
		this->stage = stage;
		this->rank = rank;
	}
	bool IsCurrentLocation(std::string type, int stage, int index) override;
};

struct MedalLocation : APSharedLocation {
	int stage; // Stage ID as in the save file
	int index;
	bool sun;
	inline MedalLocation(int64_t id, int stage, int index, bool sun) {
		this->id = id;
		this->stage = stage;
		this->index = index;
		this->sun = sun;
	}
	bool IsCurrentLocation(std::string type, int stage, int index) override;
};

struct IntFlagLocation : APSharedLocation {
	std::string flag;
	int value;
	inline IntFlagLocation(int64_t id, std::string flag, int value) {
		this->id = id;
		this->flag = flag;
		this->value = value;
	}
	bool IsCurrentLocation(std::string type, int stage, int index) override;
};

struct BoolFlagLocation : APSharedLocation {
	std::string flag;
	inline BoolFlagLocation(int64_t id, std::string flag) {
		this->id = id;
		this->flag = flag;
	}
	bool IsCurrentLocation(std::string type, int stage, int index) override;
};

class ArchipelagoManager {
	public:
		static bool hasInit;
		static bool connectAttempt;
		
		static void ItemClearCallback();
		static void ItemRecvCallback(int64_t item, bool notify);
		static void LocationCheckCallback(int64_t location);
		
		static void SetAPVersionCallback(int version);
		static void SetProgressionModeCallback(int progression);
		static void SetActItemsCallback(int ai);
		static void SetActOrderCallback(std::map<int, int> actorder);

		static void DebugLog(std::string buf);
		static void AddRings(int amt, bool fromLink = false);
		
		static std::string ipport;
		static std::string slot;
		static std::string password;

		static std::set<std::string> tags;

		static int64_t uuid;
		
		static bool isDebug;
		static int version;
		static int progressionMode;
		static int actItems;
		static int itemState[AP_ITEM_MAX];
		static std::set<int64_t> locationState;
		static std::map<std::string, int> actLevels;
		static inline std::queue<ArchipelagoMessage> messageQueue;
		static void Update(double dt, PPCContext& ctx, uint8_t* base);
		static void DisplayMessage(APMessageType type, std::string message, uint16_t imgID = 0);
		static void ConnectToArchipelago();
		static void DisconnectFromArchipelago();

		static xpointer<CEvilSonicContext> evilContext;
};

// Traps

class BounceManager {
	private:
	// So we don't spam the server with rings
	static double elapsed;
	static int numRing;

	static bool toDeath; // Should we be expecting a death link packet?
	static int amnesty;
	static void SendLink(Json::Value root, std::string tag);
	
	public:
	static bool fromDeath; // Is the next death from death link?
	static void BounceRecvCallback(AP_Bounce bounce);
	static void SendDeathLink(std::string cause);
	static void SendTrapLink(std::string trap);
	static void SendRingLink(int amt);
	static void Update(double dt);
};

#define DEATH_TRAP 5000
struct TrapBase {
	int trapID;

	virtual bool OnReceive(PPCContext& ctx, uint8_t* base, int trapID);
	virtual bool Update(double dt, PPCContext& ctx, uint8_t* base);
};

class TrapManager {
	private:
	static std::map<int, TrapBase*> StaticTrap;
	static std::map<std::string, int> TrapNameToID;
	static double TrapTimer;

	public:
	static std::vector<TrapBase*> ActiveTraps;
	static std::queue<int> TrapQueue;
	static std::queue<int> PrioTrapQueue;

	static bool IsActive(int trapID);
	static bool IsValidTrap(int trapID);
	static void Update(double dt, PPCContext& ctx, uint8_t* base);
	static int GetTrapFromName(std::string name);
};

}

#endif
