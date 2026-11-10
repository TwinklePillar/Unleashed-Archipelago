#include "archipelago.h"

#include <filesystem>
#include <format>
#include <Archipelago.h>
#include <cstdio>

#include <ui/achievement_overlay.h>
#include <api/SWA.h>
#include <mod/ini_file.h>
#include <user/paths.h>
#include <ui/message_window.h>

#include "apshared.h"

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
int ArchipelagoManager::actItems;
std::map<std::string, int> ArchipelagoManager::actLevels;
int ArchipelagoManager::itemState[AP_ITEM_MAX];
bool ArchipelagoManager::hasInit;

static SaveData g_lastSaveData{};
static double elapsed = -1;
static auto lastFlags = std::map<const char*, uint32_t>();

static auto excludedIndices = std::set<int>();
static auto indexStat = std::filesystem::file_time_type();

static void debugMsg(std::string buf) {
	if (std::filesystem::exists(GetUserPath() / "indices.txt") && ArchipelagoManager::hasInit) {
		AP_Say(buf);
		ArchipelagoManager::DisplayMessage(APMessageType::MEMORY_DEBUG, buf);
	}
}

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
				debugMsg(buf);
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
	setIntFlag("iFlag_TownSecNo", 270);
	setIntFlag("iFlag_ProfInfo_State", 101);
	setIntFlag("iFlag_Tails", 9);
	setIntFlag("iFlag_Rin", 2);
	setListFlag("eFlag_ReturnPlace", 2);
	
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->bFlag.begin(); cur < fm->bFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (!strncmp(flagName, "bFlag_StgClear", 14)) {
			cur->flag->value = true;
		}
		if (!strncmp(flagName, "bFlag_IsVisit", 13)) {
			cur->flag->value = false;
		}
		if (!strncmp(flagName, "bFlag_Event", 11)) {
			cur->flag->value = true;
		}
		if (!strncmp(flagName, "bFlag_IsStageOpen", 17)) {
			cur->flag->value = true;
		}
		if (!strncmp(flagName, "bFlag_BossOpen", 14)) {
			cur->flag->value = true;
		}
		if (!strncmp(flagName, "bFlag_IsStageRescue", 19)) {
			cur->flag->value = false;
		}
	}
	setBoolFlag("bFlag_IsVisitWorldMap", true);
}

static void resetWatchers() {
	lastFlags.clear();
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
	memcpy(&g_lastSaveData, save, sizeof(SaveData));
}

void ArchipelagoManager::Update(double dt) {
	auto dataPath = GetUserPath() / "indices.txt";
	if (std::filesystem::exists(dataPath) && hasInit) {
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
//	printf("AppDoc @ %p\n", appdoc);
//	printf("AppDoc Member @ %p\n", appdoc->m_pMember.get());
//	printf("GameParameter @ %p\n", appdoc->m_pMember->m_spGameParameter.get());
//	printf("Save Data @ %p\n", save);
	
	if (std::filesystem::exists(dataPath) && hasInit) {
		FlagManager* flags = FlagManager::GetInstance();
		watchFlags(flags->eFlag);
		watchFlags(flags->iFlag);
		watchFlags(flags->bFlag);
	}
	
	for (int i = 0; i < 512; i++) {
		auto lastStage = g_lastSaveData.records[i];
		auto curStage = save->records[i];
		if (lastStage.Time == 0) continue;
		if (lastStage.StageID != curStage.StageID) debugMsg(std::format<int>("New Stage: {0}", curStage.StageID));
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
			debugMsg( std::format<int>("Moon Medal {0}:{1}", curStage.StageID, changedBit));
			AP_SendItem(curStage.StageID * 1000 + 20 + changedBit);
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
			debugMsg(std::format<int>("Sun Medal {0}:{1}", curStage.StageID, changedBit));
			AP_SendItem(curStage.StageID * 1000 + 60 + changedBit);
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

	if (std::filesystem::exists(dataPath) && hasInit) {
		for (int i = 0; i < sizeof(SaveData); i++) {
			if (excludedIndices.contains(i)) continue;
			uint8_t prev = ((uint8_t*)&g_lastSaveData)[i];
			uint8_t cur = ((uint8_t*)save)[i];
			if (prev != cur) {
				auto str = std::format("(SAVE) {0}: {1} -> {2}", i, prev, cur);
				debugMsg(str);
			}
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
	printf("Receiving Item %s (%i, #%i)\n", ITEM_TO_NAME[item], (int)item, itemState[item]);
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
	if (version < SWA_AP_MIN_VERSION || version > SWA_AP_VERSION) {
		int result;
		char buf[256] = {0};
		snprintf(buf, 256, "Incompatible APWorld Detected! Expected %i-%i, got %i! Disconnecting", SWA_AP_MIN_VERSION, SWA_AP_VERSION, version);
		MessageWindow::Open(buf, &result);
		DisconnectFromArchipelago();
	}
	
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
		actLevels.emplace(VANILLA_ACT_ORDER[it->first], it->second);
		printf("%s -> %i\n", VANILLA_ACT_ORDER[it->first], it->second);
	}
}

void ArchipelagoManager::SetActItemsCallback(int ai) {
	actItems = ai;
}

void ArchipelagoManager::DisconnectFromArchipelago() {
	AP_Shutdown();
	hasInit = false;
}

void ArchipelagoManager::ConnectToArchipelago() {
	if (hasInit) return;
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
	AP_RegisterSlotDataIntCallback("act_items", SetActItemsCallback);
	AP_RegisterSlotDataMapIntIntCallback("act_levels", SetActOrderCallback);

	AP_Start();
	hasInit = true;
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

// Entrance Activation

PPC_FUNC_IMPL(__imp__sub_8273EFC0);
PPC_FUNC(sub_8273EFC0) {
	int areaType = PPC_LOAD_U32(ctx.r3.u32 + 244);
	int doorType = PPC_LOAD_U32(ctx.r3.u32 + 268);
	char* stageType = (char*)MmGetHostAddress(PPC_LOAD_U32(ctx.r3.u32 + 260));

	/*
	#Town_EggManBase/CommonOBJ.set.xml:138:    <StageType>ActD_Extra_02</StageType>
	#Town_EuropeanCityETF/CommonOBJ.set.xml:1997:    <StageType>ActD_Extra_01</StageType>
	#Town_EggManBase/CommonOBJ.set.xml:160:    <StageType>boss_darkgaia</StageType>
	#Town_EggManBase/CommonOBJ.set.xml:182:    <StageType>boss_eggmanland</StageType>
	#Town_EggManBase/CommonOBJ.set.xml:204:    <StageType>Act_EggManLand</StageType>
	*/

	bool stageAllowed = true;
	std::string apStage = "";
	int isBoss = 0;
	switch (areaType) {
		case 0: // Apotos
			if (!strcmp(stageType, "day_tutorial")) apStage = "Windmill Isle Day Act 1";
			if (!strcmp(stageType, "day1")) apStage = "Windmill Isle Day Act 2";
			if (!strcmp(stageType, "ActD_SubMykonos_01")) apStage = "Windmill Isle Day Act 3";
			if (!strcmp(stageType, "night1")) apStage = "Windmill Isle Night Act 1";
			if (!strcmp(stageType, "ActN_SubMykonos_01")) apStage = "Windmill Isle Night Act 2";
			break;
		case 1: // Spagonia
			if (!strcmp(stageType, "day1")) apStage = "Rooftop Run Day Act 1";
			if (!strcmp(stageType, "ActD_SubEU_01")) apStage = "Rooftop Run Day Act 2";
			if (!strcmp(stageType, "ActD_SubEU_02")) apStage = "Rooftop Run Day Act 3";
			if (!strcmp(stageType, "night1")) apStage = "Rooftop Run Night Act 1";
			if (!strcmp(stageType, "dayBoss")) {apStage = "Egg Devil Ray"; isBoss = 2;}
			break;
		case 2: // Chun-Nan
			if (!strcmp(stageType, "day1")) apStage = "Dragon Road Day Act 1";
			if (!strcmp(stageType, "ActD_SubChina_03")) apStage = "Dragon Road Day Act 2";
			if (!strcmp(stageType, "ActD_SubChina_04")) apStage = "Dragon Road Day Act 3";
			if (!strcmp(stageType, "night1")) apStage = "Dragon Road Night Act 1";
			if (!strcmp(stageType, "ActN_SubChina_01")) apStage = "Dragon Road Night Act 2";
			if (!strcmp(stageType, "nightBoss")) {apStage = "Dark Gaia Phoenix"; isBoss = 3;}
			break;
		case 3: // Mazuri
			if (!strcmp(stageType, "day1")) apStage = "Savannah Citadel Day Act 1";
			if (!strcmp(stageType, "ActD_SubAfrica_01")) apStage = "Savannah Citadel Day Act 2";
			if (!strcmp(stageType, "ActD_SubAfrica_03")) apStage = "Savannah Citadel Day Act 3";
			if (!strcmp(stageType, "night1")) apStage = "Savannah Citadel Night Act 1";
			if (!strcmp(stageType, "ActN_SubAfrica_01")) apStage = "Savannah Citadel Night Act 2";
			if (!strcmp(stageType, "dayBoss")) {apStage = "Egg Beetle"; isBoss = 2;}
			break;
		case 4: // Holoska
			if (!strcmp(stageType, "day1")) apStage = "Cool Edge Day Act 1";
			if (!strcmp(stageType, "ActD_SubSnow_01")) apStage = "Cool Edge Day Act 2";
			if (!strcmp(stageType, "night1")) apStage = "Cool Edge Night Act 1";
			if (!strcmp(stageType, "nightBoss")) {apStage = "Dark Moray"; isBoss = 3;}
			break;
		case 5: // Shamar
			if (!strcmp(stageType, "day1")) apStage = "Arid Sands Day Act 1";
			if (!strcmp(stageType, "ActD_SubPetra_03")) apStage = "Arid Sands Day Act 2";
			if (!strcmp(stageType, "night1")) apStage = "Arid Sands Night Act 1";
			if (!strcmp(stageType, "nightBoss")) {apStage = "Dark Guardian"; isBoss = 3;}
			break;
		case 6: // Adabat
			if (!strcmp(stageType, "day1")) apStage = "Jungle Joyride Day Act 1";
			if (!strcmp(stageType, "ActD_SubBeach_02")) apStage = "Jungle Joyride Day Act 2";
			if (!strcmp(stageType, "ActD_SubBeach_04")) apStage = "Jungle Joyride Day Act 3";
			if (!strcmp(stageType, "night1")) apStage = "Jungle Joyride Night Act 1";
			if (!strcmp(stageType, "ActN_SubBeach_01")) apStage = "Jungle Joyride Night Act 2";
			if (!strcmp(stageType, "dayBoss")) {apStage = "Egg Lancer"; isBoss = 2;}
			break;
		case 7: // Empire City
			if (!strcmp(stageType, "day1")) apStage = "Skyscraper Scamper Day Act 1";
			if (!strcmp(stageType, "ActD_SubNY_01")) apStage = "Skyscraper Scamper Day Act 2";
			if (!strcmp(stageType, "night1")) apStage = "Skyscraper Scamper Night Act 1";
			break;
		case 8: // Eggmanland
			break;
	}

	if (SWAAP::ArchipelagoManager::actItems) {
		for (int i = 0; i < AP_ITEM_MAX; i++) {
			if (ITEM_TO_NAME[i] && !strcmp(ITEM_TO_NAME[i]+12, apStage.c_str())) {
				if (SWAAP::ArchipelagoManager::itemState[i] == 0) stageAllowed = false;
				break;
			}
		}
	}
	
	if (SWAAP::ArchipelagoManager::actLevels.contains(apStage)) {
		int newLevel = SWAAP::ArchipelagoManager::actLevels[apStage];
		if (newLevel != -1) PPC_STORE_U32(ctx.r3.u32 + 276, newLevel);
		else if (isBoss) PPC_STORE_U32(ctx.r3.u32 + 276, 1);
	}

	int needMedalNum = PPC_LOAD_U32(ctx.r3.u32 + 276);
	printf("r3: %08x\n", ctx.r3.u32);
	printf("\tAreaType: %i\n", areaType);
	printf("\tDoorType: %i\n", doorType);
	printf("\tNeedMedalNum: %i\n", needMedalNum);
	printf("\tStageType: %s\n", stageType);

	if (isBoss == 2) PPC_STORE_U32(ctx.r3.u32 + 268, 0);
	if (isBoss == 3) PPC_STORE_U32(ctx.r3.u32 + 268, 1);
	if (!stageAllowed) return;
	
	__imp__sub_8273EFC0(ctx, base);
}

// Saving flags I think, don't remember

PPC_FUNC_IMPL(__imp__sub_825042B0);
PPC_FUNC(sub_825042B0) {
	SWAAP::setupFlags();
	__imp__sub_825042B0(ctx, base);
}

// On new game

PPC_FUNC_IMPL(__imp__sub_82585550);
PPC_FUNC(sub_82585550) {
	__imp__sub_82585550(ctx, base);
	SWAAP::setupFlags();
	SWAAP::resetWatchers();
}

// Stage Return Data (size: 80)
// offset 0: vtable (820C5BDC)
// offset 28: AreaType

PPC_FUNC_IMPL(__imp__sub_82B5B520);
PPC_FUNC(sub_82B5B520) {
	__imp__sub_82B5B520(ctx, base);
	printf("Stage Return Location: %08x\n", ctx.r3.u32);
}

// Flag Reference: sub_825042B0
