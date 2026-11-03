#include "archipelago.h"

#include <format>
#include <Archipelago.h>
#include <iostream>

#include <ui/achievement_overlay.h>
#include <api/SWA.h>
#include <mod/ini_file.h>
#include <user/paths.h>
#include <ui/message_window.h>

#include "apshared.h"

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

int ArchipelagoManager::version;
int ArchipelagoManager::progressionMode;
std::map<const char*, const char*> ArchipelagoManager::actOrder;
int64_t ArchipelagoManager::itemState[AP_ITEM_MAX];

static SaveData g_lastSaveData{};
static double elapsed = -1;

void ArchipelagoManager::Update(double dt) {
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
	if (g_lastSaveData.records[0].StageID != 0xEABA) {
		memcpy(&g_lastSaveData, save, sizeof(SaveData));
	}
	
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

		for (int j = 0; j < 40; j++) {
	//		if (lastStage.pad21[j] != curStage.pad21[j]) DisplayMessage(APMessageType::MEMORY_DEBUG, std::format<int>("(Stage {0}) {1}: {2} -> {3}", curStage.StageID, j+24, lastStage.pad21[j], curStage.pad21[j]));
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

	for (int i = 0; i < 0x773; i++) {
	//	if (g_lastSaveData.pad31[i] != save->pad31[i]) DisplayMessage(APMessageType::MEMORY_DEBUG, std::format("(UnkSec) {0}: {1} -> {2}", i, g_lastSaveData.pad31[i], save->pad31[i]));
	}
	
	
	memcpy(&g_lastSaveData, save, sizeof(SaveData));
	elapsed += dt;
	if (elapsed > 5) {
		elapsed -= 5;
		//	DisplayMessage(APMessageType::MISC, "5 Second Timer");
	}
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
		if (item == THE_WORLD_REPAIRED) AP_StoryComplete();
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


void ArchipelagoManager::ConnectToArchipelago() {
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
	printf("Save Data @ %p\n", save);
	ItemClearCallback();
	
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
			ctx.r3.u64 = ArchipelagoManager::itemState[STOMPING_SHOES] > 0;
			break;
		case 1: // Wall Jump
			ctx.r3.u64 = ArchipelagoManager::itemState[WALL_JUMP_SHOES] > 0;
			break;
		case 2: // Air Boost
			ctx.r3.u64 = ArchipelagoManager::itemState[AIR_BOOST_SHOES] > 0;
			break;
		case 3: // Light Dash
			ctx.r3.u64 = ArchipelagoManager::itemState[LIGHT_DASH_SHOES] > 0;
			break;
	}
	if (isShoeItem) ctx.r3.u64 = 0;
	
	return;
}

// Medal Patch

void SWA_AP_MedalCountMidAsmHook(PPCRegister& r3, PPCRegister& r10, PPCRegister &r11) {
	uint8_t* base = g_memory.base;
	
	int item = r10.u64 == 12 ? PROGRESSIVE_MOON_LEVEL : PROGRESSIVE_SUN_LEVEL;
	int numMedal = 0;

	const int medalList[] = {0, 15, 30, 45, 60, 80, 120};
	if (ArchipelagoManager::itemState[item] > 6) numMedal = 200;
	else numMedal = medalList[ArchipelagoManager::itemState[item]];
	
	r3.u64 = numMedal;
	PPC_STORE_U32(r10.u32 + r11.u32, numMedal);
}
