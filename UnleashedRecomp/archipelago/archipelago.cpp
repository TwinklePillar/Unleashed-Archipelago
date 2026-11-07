#include "archipelago.h"

#include <filesystem>
#include <format>
#include <Archipelago.h>
#include <iostream>
#include <cstdio>

#include <ui/achievement_overlay.h>
#include <api/SWA.h>
#include <mod/ini_file.h>
#include <user/paths.h>
#include <ui/message_window.h>

#include "SWA/System/GameDocument.h"
#include "apshared.h"
#include "kernel/memory.h"

namespace SWAAP {

struct StageRecord {
	be<uint32_t> StageID;
	be<uint32_t> MoonBits;
	be<uint32_t> SunBits;
	be<int32_t> Score;
	be<float> Time;
	be<uint32_t> Rank;
	SWA_INSERT_PADDING(40);
};

struct SaveData {
	StageRecord records[512];
	be<uint32_t> lives;
	bool stomp;
	bool walljump;
	bool airboost;
	bool lightdash;
	SWA_INSERT_PADDING(0x773);
};

class FlagManager {
	public:
	struct Flag {
		xpointer<const char> name;
		uint32_t value;
	};

	struct RegisteredFlag {
		xpointer<Flag> flag;
		SWA_INSERT_PADDING(4);
	};
	
	be<float> version;
	hh::vector<RegisteredFlag> eFlag;
	hh::vector<RegisteredFlag> iFlag;
	hh::vector<RegisteredFlag> bFlag;

	static FlagManager* GetInstance() {
		return *(xpointer<FlagManager>*)MmGetHostAddress(0x833678F0);
	}
};

int ArchipelagoManager::version;
int ArchipelagoManager::progressionMode;
std::map<const char*, const char*> ArchipelagoManager::actOrder;
int64_t ArchipelagoManager::itemState[AP_ITEM_MAX];

static SaveData g_lastSaveData{};
static double elapsed = -1;
static auto lastFlags = std::map<const char*, uint32_t>();

static auto excludedIndices = std::set<int>();
static auto indexStat = std::filesystem::file_time_type();

static void watchFlags(hh::vector<FlagManager::RegisteredFlag> flags) {
	for (auto cur = flags.begin(); cur < flags.end(); cur++) {
		if (lastFlags.contains(cur->flag->name.get())) {
			if (cur->flag->value != lastFlags[cur->flag->name.get()]) {
				uint32_t oldV = lastFlags[cur->flag->name];
				uint32_t newV = cur->flag->value;
				if (*cur->flag->name.get() != 'b') {
					oldV = be<uint32_t>::byteswap(oldV);
					newV = be<uint32_t>::byteswap(newV);
				}
				char buf[256] = {0};
				snprintf(buf, 256, "(FLAG) %s: %u -> %u", cur->flag->name.get(), oldV, newV);
				AP_Say(buf);
				ArchipelagoManager::DisplayMessage(APMessageType::MEMORY_DEBUG, buf);
				lastFlags[cur->flag->name.get()] = cur->flag->value;
			}
		} else lastFlags.emplace(cur->flag->name.get(), cur->flag->value);
	}
}

static void setBoolFlag(const char* name, bool value) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->bFlag.begin(); cur < fm->bFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (strcmp(flagName, name) == 0) {
			cur->flag->value = value;
			break;
		}
	}
}

static void setIntFlag(const char* name, int value) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->iFlag.begin(); cur < fm->iFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (strcmp(flagName, name) == 0) {
			cur->flag->value = be<uint32_t>::byteswap(value);
			break;
		}
	}
}

static void setListFlag(const char* name, bool value) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->eFlag.begin(); cur < fm->eFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (strcmp(flagName, name) == 0) {
			cur->flag->value = be<uint32_t>::byteswap(value);
			break;
		}
	}
}

static void setupFlags(void) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->bFlag.begin(); cur < fm->bFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (!strncmp(flagName, "bFlag_StgClear", 14)) {
			cur->flag->value = true;
		}
		if (!strncmp(flagName, "bFlag_IsVisit", 13)) {
			cur->flag->value = false;
		}
	}
}

void ArchipelagoManager::Update(double dt) {
	auto dataPath = GetUserPath() / "indices.txt";
	if (std::filesystem::exists(dataPath)) {
		elapsed += dt;
		if (elapsed > 1) {
			elapsed -= 1;
			auto curStat = std::filesystem::last_write_time(dataPath);
			if (curStat != indexStat) {
				indexStat = curStat;
				std::ifstream file(dataPath);
				excludedIndices.clear();
				for (int i = 0; i < 32768; i++) excludedIndices.insert(i);
				while (!file.eof()) {
					char buf[256] = {0};
					file.getline(buf, 256);
					if (*buf < '0' || *buf > '9') continue;
					excludedIndices.insert(std::stoi(buf));
				}
			}
		}
	}
	
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
	if (g_lastSaveData.records[0].StageID != 0xEABA) {
		memcpy(&g_lastSaveData, save, sizeof(SaveData));
	}
	
//	FlagManager* flags = FlagManager::GetInstance();
//	watchFlags(flags->eFlag);
//	watchFlags(flags->iFlag);
//	watchFlags(flags->bFlag);
	
	for (int i = 0; i < 512; i++) {
		auto lastStage = g_lastSaveData.records[i];
		auto curStage = save->records[i];
	//	if (lastStage.StageID != curStage.StageID) DisplayMessage(APMessageType::MEMORY_DEBUG, std::format<int>("New Stage: {0}", curStage.StageID));
		if (lastStage.MoonBits != curStage.MoonBits) {
			int changedBit = 0;
			uint32_t lastBit = lastStage.MoonBits;
			uint32_t curBit = curStage.MoonBits;
			for (int i = 0; i < 32; i++) {
				if ((lastBit & 1) != (curBit & 1)) {
					changedBit = i;
					break;
				}
				lastBit >>= 1;
				curBit >>= 1;
			}
	//		DisplayMessage(APMessageType::SENT_ITEM, std::format<int>("Moon Medal {0}:{1}", curStage.StageID, changedBit));
		}
		if (lastStage.SunBits != curStage.SunBits) {
			int changedBit = 0;
			uint32_t lastBit = lastStage.SunBits;
			uint32_t curBit = curStage.SunBits;
			for (int i = 0; i < 32; i++) {
				if ((lastBit & 1) != (curBit & 1)) {
					changedBit = i;
					break;
				}
				lastBit >>= 1;
				curBit >>= 1;
			}
	//		DisplayMessage(APMessageType::SENT_ITEM, std::format<int>("Sun Medal {0}:{1}", curStage.StageID, changedBit));
		}
		if (lastStage.Time != curStage.Time) {
			AP_SendItem(curStage.StageID * 1000);
			curStage.Time = lastStage.Time;
		}
	}

	if (save->stomp) {
		save->stomp = 0;
		AP_SendItem(32772);
	}
	if (save->walljump) {
		save->walljump = 0;
		AP_SendItem(32773);
	}
	if (save->airboost) {
		save->airboost = 0;
		AP_SendItem(32774);
	}
	if (save->lightdash) {
		save->lightdash = 0;
		AP_SendItem(32775);
	}

	for (int i = 0; i < sizeof(SaveData); i++) {
		if (excludedIndices.contains(i)) continue;
		uint8_t prev = ((uint8_t*)&g_lastSaveData)[i];
		uint8_t cur = prev;//((uint8_t*)save)[i];
		if (prev != cur) {
			auto str = std::format("(SAVE) {0}: {1} -> {2}", i, prev, cur);
			AP_Say(str);
			DisplayMessage(APMessageType::MEMORY_DEBUG, str);
		}
	}
	
	memcpy(&g_lastSaveData, save, sizeof(SaveData));
}

void ArchipelagoManager::DisplayMessage(APMessageType type, std::string message) {
	std::string header = "";
	switch (type) {
		case APMessageType::SENT_ITEM:
			header = "Sent Item";
			break;
		case APMessageType::RECV_ITEM:
			header = "Received Item";
			break;
		case APMessageType::MEMORY_DEBUG:
			header = "Memory Watch Triggered";
			break;
		default:
			header = "Unknown AP Message";
			break;
	}
	messageQueue.push(ArchipelagoMessage{.header=header, .message=message});
	AchievementOverlay::Open(AP_MESSAGE_ACH);
}

void ArchipelagoManager::ItemClearCallback() {
	for (int i = 0; i < AP_ITEM_MAX; i++) {
		itemState[i] = 0;
	}
}

void ArchipelagoManager::ItemRecvCallback(int64_t item, bool notify) {
	itemState[item] += 1;
	if (notify) {
		DisplayMessage(RECV_ITEM, ITEM_TO_NAME[item]);

		auto appdoc = SWA::CApplicationDocument::GetInstance();
		auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
		if (item == EXTRA_LIFE) {
			save->lives = save->lives + 1;
		}
	}
	
	switch (item) {
		case THE_WORLD_REPAIRED: AP_StoryComplete(); break;
	}
}

void ArchipelagoManager::LocationCheckCallback(int64_t location) {
	
}

void ArchipelagoManager::SetAPVersionCallback(int version) {
	ArchipelagoManager::version = version;
	printf("Version: %i\n", version);
}

void ArchipelagoManager::SetProgressionModeCallback(int progression) {
	progressionMode = progression;
	printf("Progression: %i\n", progression);
}

void ArchipelagoManager::SetActOrderCallback(std::map<int, int> actorder) {
	int max = 0;
	for (auto it = actorder.begin(); it != actorder.end(); it++) {
		actOrder.emplace(VANILLA_ACT_ORDER[it->second], VANILLA_ACT_ORDER[it->first]);
		printf("%s -> %s\n", VANILLA_ACT_ORDER[it->second], VANILLA_ACT_ORDER[it->first]);
	}
}

void ArchipelagoManager::DisconnectFromArchipelago() {
	AP_Shutdown();
}

void ArchipelagoManager::ConnectToArchipelago() {
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
	printf("AppDoc @ %p\n", appdoc);
	printf("AppDoc Member @ %p\n", appdoc->m_pMember.get());
	printf("GameParameter @ %p\n", appdoc->m_pMember->m_spGameParameter.get());
	printf("Save Data @ %p\n", save);
	ItemClearCallback();
	setupFlags();
	
	auto ini = IniFile();
	auto path = GetUserPath() / "connection.ini";
	if (!ini.read(path)) {
		int result = -1;
		MessageWindow::Open("Connection failed: Unable to read \"connection.ini\"", &result);
		return;
	}

	auto ipport = ini.getString("Connect", "ipport", "");
	auto slot = ini.getString("Connect", "slot", "");
	auto password = ini.getString("Connect", "password", "");

	if (ipport.empty() || slot.empty()) {
		int result = -1;
		MessageWindow::Open("Connection failed: Cannot find connection info in ini", &result);
		return;
	}

	AP_Init(ipport.c_str(), SWA_AP_GAME, slot.c_str(), password.c_str());
	AP_SetItemClearCallback(ItemClearCallback);
	AP_SetItemRecvCallback(ItemRecvCallback);
	AP_SetLocationCheckedCallback(LocationCheckCallback);

	AP_RegisterSlotDataIntCallback("version", SetAPVersionCallback);
	AP_RegisterSlotDataIntCallback("progression_mode", SetProgressionModeCallback);
	AP_RegisterSlotDataMapIntIntCallback("act_order", SetActOrderCallback);

	AP_Start();
}

}

// Shoe Patch

// yes I'm really doing this lol
static bool isShoeItem = false;
PPC_FUNC_IMPL(__imp__sub_82690D00);
PPC_FUNC(sub_82690D00) {
	isShoeItem = true;
	__imp__sub_82690D00(ctx, base);
	isShoeItem = false;
}

PPC_FUNC_IMPL(__imp__sub_82519C08);
PPC_FUNC(sub_82519C08) {
	switch (ctx.r4.u64) {
		case 0: // Stomp
			ctx.r3.u64 = SWAAP::ArchipelagoManager::itemState[STOMPING_SHOES] > 0;
			break;
		case 1: // Wall Jump
			ctx.r3.u64 = SWAAP::ArchipelagoManager::itemState[WALL_JUMP_SHOES] > 0;
			break;
		case 2: // Air Boost
			ctx.r3.u64 = SWAAP::ArchipelagoManager::itemState[AIR_BOOST_SHOES] > 0;
			break;
		case 3: // Light Dash
			ctx.r3.u64 = SWAAP::ArchipelagoManager::itemState[LIGHT_DASH_SHOES] > 0;
			break;
	}
	if (isShoeItem) ctx.r3.u64 = 0;
	
	return;
}

// Medal Patch

static int getMedalCount(int levelItem) {
	int numMedal = 0;
	const int medalList[] = {0, 15, 30, 45, 60, 80, 120};
	if (SWAAP::ArchipelagoManager::itemState[levelItem] > 6) numMedal = 200;
	else numMedal = medalList[SWAAP::ArchipelagoManager::itemState[levelItem]];

	return numMedal;
}

void SWA_AP_MedalCountMidAsmHook(PPCRegister& r3, PPCRegister& r10, PPCRegister &r11) {
	uint8_t* base = g_memory.base;
	
	int levelItem = r10.u64 == 12 ? PROGRESSIVE_MOON_LEVEL : PROGRESSIVE_SUN_LEVEL;
	
	r3.u64 = getMedalCount(levelItem);
	PPC_STORE_U32(r10.u32 + r11.u32, getMedalCount(levelItem));
}

void SWA_AP_HubRandoMidAsmHook(PPCRegister& r29, PPCRegister& r30) {
	int countryToItem[] = {
		HUB_UNLOCK_APOTOS,
		HUB_UNLOCK_SPAGONIA,
		HUB_UNLOCK_CHUNNAN,
		HUB_UNLOCK_MAZURI,
		HUB_UNLOCK_HOLOSKA,
		HUB_UNLOCK_SHAMAR,
		HUB_UNLOCK_ADABAT,
		HUB_UNLOCK_EMPIRE_CITY,
		HUB_UNLOCK_EGGMANLAND,
	};
	if (r29.s32 != -1 && SWAAP::ArchipelagoManager::itemState[countryToItem[r29.u32]] == 0) r29.s32 = -1;
	if (r30.s32 != -1 && SWAAP::ArchipelagoManager::itemState[countryToItem[r30.u32]] == 0) r30.s32 = -1;
	
	if (r29.s32 == 8 || r30.s32 == 8) {
		int chaosEmeralds[] = {
			BLUE_CHAOS_EMERALD,
			RED_CHAOS_EMERALD,
			PURPLE_CHAOS_EMERALD,
			CYAN_CHAOS_EMERALD,
			GREEN_CHAOS_EMERALD,
			YELLOW_CHAOS_EMERALD,
			WHITE_CHAOS_EMERALD,
		};

		bool hasEmeralds = true;
		for (int i = 0; i < 7; i++) if (SWAAP::ArchipelagoManager::itemState[chaosEmeralds[i]] == 0) hasEmeralds = false;
		if (!(hasEmeralds && getMedalCount(PROGRESSIVE_SUN_LEVEL) >= 120 && getMedalCount(PROGRESSIVE_MOON_LEVEL) >= 80)) {
			r29.s32 = -1;
			r30.s32 = -1;
		}
	}
}

PPC_FUNC_IMPL(__imp__sub_825042B0);
PPC_FUNC(sub_825042B0) {
	SWAAP::setupFlags();
	__imp__sub_825042B0(ctx, base);
}

// Flag Reference: sub_825042B0
