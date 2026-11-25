#include "SWAAP.h"

#include <filesystem>
#include <format>
#include <Archipelago.h>
#include <cstdio>
#include <chrono>

#include <ui/achievement_overlay.h>
#include <api/SWA.h>
#include <mod/ini_file.h>
#include <user/paths.h>
#include <ui/message_window.h>
#include <user/config.h>

#include "apshared.h"

namespace SWAAP {

int ArchipelagoManager::version{};
int ArchipelagoManager::progressionMode{};
int ArchipelagoManager::actItems{};
std::map<std::string, int> ArchipelagoManager::actLevels{};
int ArchipelagoManager::itemState[AP_ITEM_MAX]{};
std::set<int64_t> ArchipelagoManager::locationState{};
bool ArchipelagoManager::hasInit{};
bool ArchipelagoManager::connectAttempt{};
std::string ArchipelagoManager::ipport{};
std::string ArchipelagoManager::slot{};
std::string ArchipelagoManager::password{};

int64_t ArchipelagoManager::uuid = 0;
std::set<std::string> ArchipelagoManager::tags{};

static SaveData g_lastSaveData{};
static double elapsed = -1;
static auto lastFlags = std::map<const char*, uint32_t>();

static auto excludedIndices = std::set<int>();
static auto indexStat = std::filesystem::file_time_type();

bool ArchipelagoManager::isDebug = false;

xpointer<CEvilSonicContext> ArchipelagoManager::evilContext;

void ArchipelagoManager::DebugLog(std::string buf) {
	if (isDebug) {
		AP_Say(buf);
		DisplayMessage(APMessageType::MEMORY_DEBUG, buf);
	}
}

static void watchFlags(hh::vector<FlagManager::RegisteredFlag> flags) {
	for (auto cur = flags.begin(); cur < flags.end(); cur++) {
		uint32_t value = 0;
		switch (*cur->flag->name.get()) {
			case 'e': value = cur->flag->eFlag.value; break;
			case 'b': value = cur->flag->bFlag.value; break;
			case 'i': value = cur->flag->iFlag.value; break;
		}
		if (*cur->flag->name.get() == 'e') {
		//	printf("%s: %p\n", cur->flag->name.get(), cur->flag.get());
		}
		if (lastFlags.contains(cur->flag->name.get())) {
			if (value != lastFlags[cur->flag->name.get()]) {
				uint32_t oldV = lastFlags[cur->flag->name];
				uint32_t newV = value;
				char buf[256] = {0};
				snprintf(buf, 256, "(FLAG) %s: %u -> %u", cur->flag->name.get(), oldV, newV);
				SWAAP::ArchipelagoManager::DebugLog(buf);
				lastFlags[cur->flag->name.get()] = value;
			}
		} else lastFlags.emplace(cur->flag->name.get(), value);
	}
}

static void setBoolFlag(const char* name, bool value) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->bFlag.begin(); cur < fm->bFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (strcmp(flagName, name) == 0) {
			cur->flag->bFlag.value = value;
			break;
		}
	}
}

static void setIntFlag(const char* name, int value) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->iFlag.begin(); cur < fm->iFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (strcmp(flagName, name) == 0) {
			cur->flag->iFlag.value = value;
			break;
		}
	}
}

static void setListFlag(const char* name, int value) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->eFlag.begin(); cur < fm->eFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (strcmp(flagName, name) == 0) {
			cur->flag->eFlag.value = value;
			break;
		}
	}
}

static const char* getListFlag(const char* name) {
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->eFlag.begin(); cur < fm->eFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (strcmp(flagName, name) == 0) {
			return cur->flag->eFlag.named[cur->flag->eFlag.value];
		}
	}
	return NULL;
}

static void setupFlags(void) {
	setIntFlag("iFlag_TownSecNo", 270);
	setIntFlag("iFlag_ProfInfo_State", 101);
	setIntFlag("iFlag_Tails", 9);
	setIntFlag("iFlag_Rin", 2);
	setIntFlag("iFlag_Sunafkin", 50);
//	setListFlag("eFlag_ReturnPlace", 2);
	
	FlagManager* fm = FlagManager::GetInstance();
	for (auto cur = fm->bFlag.begin(); cur < fm->bFlag.end(); cur++) {
		const char* flagName = cur->flag->name.get();
		if (!strncmp(flagName, "bFlag_StgClear", 14)) {
			cur->flag->bFlag.value = true;
		}
		if (!strncmp(flagName, "bFlag_IsVisit", 13)) {
			cur->flag->bFlag.value = false;
		}
		if (!strncmp(flagName, "bFlag_Event", 11)) {
			cur->flag->bFlag.value = true;
		}
		if (!strncmp(flagName, "bFlag_IsStageOpen", 17)) {
			cur->flag->bFlag.value = true;
		}
		if (!strncmp(flagName, "bFlag_BossOpen", 14)) {
			cur->flag->bFlag.value = true;
		}
		if (!strncmp(flagName, "bFlag_IsStageRescue", 19)) {
			cur->flag->bFlag.value = false;
		}
	}
	setBoolFlag("bFlag_IsVisitWorldMap", true);

	int emeralds[] = {
		BLUE_CHAOS_EMERALD,
		RED_CHAOS_EMERALD,
		PURPLE_CHAOS_EMERALD,
		CYAN_CHAOS_EMERALD,
		GREEN_CHAOS_EMERALD,
		YELLOW_CHAOS_EMERALD,
		WHITE_CHAOS_EMERALD
	};
	int numEmerald = 0;
	for (int emerald : emeralds) {
		if (ArchipelagoManager::itemState[emerald]) numEmerald += 1;
	}
	switch (numEmerald) {
		case 7: setBoolFlag("bFlag_IsStageRescue_EggManBase", true); setBoolFlag("bFlag_IsStageRescueEvent_EggManBase", true);
		case 6: setBoolFlag("bFlag_IsStageRescue_SouthEastAsia", true); setBoolFlag("bFlag_IsStageRescueEvent_SouthEastAsia", true);
		case 5: setBoolFlag("bFlag_IsStageRescue_PetraCapital", true); setBoolFlag("bFlag_IsStageRescueEvent_PetraCapital", true);
		case 4: setBoolFlag("bFlag_IsStageRescue_Snow", true); setBoolFlag("bFlag_IsStageRescueEvent_Snow", true);
		case 3: setBoolFlag("bFlag_IsStageRescue_EuropeanCity", true); setBoolFlag("bFlag_IsStageRescueEvent_EuropeanCity", true);
		case 2: setBoolFlag("bFlag_IsStageRescue_China", true); setBoolFlag("bFlag_IsStageRescueEvent_China", true);
		case 1: setBoolFlag("bFlag_IsStageRescue_Africa", true); setBoolFlag("bFlag_IsStageRescueEvent_Africa", true);
		default: break;
	}
}

static void resetWatchers() {
	lastFlags.clear();
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
	memcpy(&g_lastSaveData, save, sizeof(SaveData));
}

void ArchipelagoManager::Update(double dt, PPCContext& ctx, uint8_t* base) {
	hasInit = AP_GetConnectionStatus() == AP_ConnectionStatus::Authenticated;
	auto dataPath = GetGamePath() / "SWAAP/indices.txt";
	elapsed += dt;
	if (elapsed > 1) {
		elapsed -= 1;
		if (std::filesystem::exists(dataPath) && hasInit) {
			isDebug = true;
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
		} else isDebug = false;
	}

	TrapManager::Update(dt, ctx, base);
	BounceManager::Update(dt);
	
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
//	printf("AppDoc @ %p\n", appdoc);
//	printf("AppDoc Member @ %p\n", appdoc->m_pMember.get());
//	printf("GameParameter @ %p\n", appdoc->m_pMember->m_spGameParameter.get());
//	printf("Save Data @ %p\n", save);
	
//	printf("Day Sonic Instance: %p\n", CPlayerSpeedContext::GetInstance());
//	printf("Night Sonic Instance: %p\n", evilContext.get());
	
	if (isDebug) {
		FlagManager* flags = FlagManager::GetInstance();
		watchFlags(flags->eFlag);
		watchFlags(flags->iFlag);
		watchFlags(flags->bFlag);
	}
	
	for (int i = 0; i < 512; i++) {
		auto lastStage = g_lastSaveData.records[i];
		auto curStage = save->records[i];
		if (lastStage.Time == 0) continue;
		if (lastStage.StageID != curStage.StageID) DebugLog(std::format<int>("New Stage: {0}", curStage.StageID));
	}

	if (isDebug) {
		for (int i = 0; i < sizeof(SaveData); i++) {
			if (excludedIndices.contains(i)) continue;
			uint8_t prev = ((uint8_t*)&g_lastSaveData)[i];
			uint8_t cur = ((uint8_t*)save)[i];
			if (prev != cur) {
				auto str = std::format("(SAVE) {0}: {1} -> {2}", i, prev, cur);
				DebugLog(str);
			}
		}
	}
	
	memcpy(&g_lastSaveData, save, sizeof(SaveData));
	if (AP_IsMessagePending()) {
		auto baseMsg = AP_GetLatestMessage();
		switch (baseMsg->type) {
			case AP_MessageType::ItemSend: {
				auto castMsg = (AP_ItemSendMessage*)baseMsg;
				char buf[1024] = {0};
				if (castMsg->recvPlayer == slot) break;
				if (castMsg->sendPlayer != slot) break;
				snprintf(buf, 1024, "%s to %s", castMsg->item.c_str(), castMsg->recvPlayer.c_str());
				DisplayMessage(SENT_ITEM, buf);
				break;
			}
			case AP_MessageType::ItemRecv: {
				auto castMsg = (AP_ItemRecvMessage*)baseMsg;
				int i = 0;
				for (i = 0; i < AP_ITEM_MAX; i++) {
					if (ITEM_TO_NAME[i] && !strcmp(ITEM_TO_NAME[i], castMsg->item.c_str())) break;
				}
				uint16_t imgID = 0;
				switch (i) {
					case BLUE_CHAOS_EMERALD: imgID = 26; break;
					case RED_CHAOS_EMERALD: imgID = 25; break;
					case PURPLE_CHAOS_EMERALD: imgID = 29; break;
					case CYAN_CHAOS_EMERALD: imgID = 27; break;
					case GREEN_CHAOS_EMERALD: imgID = 24; break;
					case YELLOW_CHAOS_EMERALD: imgID = 28; break;
					case WHITE_CHAOS_EMERALD: imgID = 31; break; // slightly inaccurate but shrug
					case SUN_MEDAL: imgID = 32; break;
					case PROGRESSIVE_SUN_LEVEL: imgID = 52; break;
					case MOON_MEDAL: imgID = 34; break;
					case PROGRESSIVE_MOON_LEVEL: imgID = 53; break;
					case STOMPING_SHOES: imgID = 41; break;
					case WALL_JUMP_SHOES: imgID = 42; break;
					case AIR_BOOST_SHOES: imgID = 43; break;
					case LIGHT_DASH_SHOES: imgID = 44; break;
					default: break;
				}
				if (castMsg->item.starts_with("Act Unlock")) imgID = 79;
				if (castMsg->item.starts_with("Hub Unlock")) imgID = 51;
				if (castMsg->item.ends_with("Currency")) imgID = 81;
				if (castMsg->sendPlayer == slot) {
					DisplayMessage(RECV_ITEM, castMsg->item, imgID);
				} else {
					char buf[1024] = {0};
					snprintf(buf, 1024, "%s from %s", castMsg->item.c_str(), castMsg->sendPlayer.c_str());
					DisplayMessage(RECV_ITEM, buf, imgID);
				}
				break;
			}
		}
		AP_ClearLatestMessage();
	}
}

void ArchipelagoManager::DisplayMessage(APMessageType type, std::string message, uint16_t imgID) {
	std::string header = "";
	switch (type) {
		case APMessageType::SENT_ITEM:
			header = "Sent Item";
			break;
		case APMessageType::RECV_ITEM:
			header = "Received Item";
			break;
		case APMessageType::DEATH_LINK:
			header = "Death Link Received";
			break;
		case APMessageType::TRAP_LINK:
			header = "Trap Link Received";
			break;
		case APMessageType::MEMORY_DEBUG:
			header = "Memory Watch Triggered";
			break;
		default:
			header = "Unknown AP Message";
			break;
	}
	messageQueue.push(ArchipelagoMessage{.header=header, .message=message, .imgID=imgID});
	AchievementOverlay::Open(AP_MESSAGE_ACH);
}

void ArchipelagoManager::ItemClearCallback() {
	for (int i = 0; i < AP_ITEM_MAX; i++) {
		itemState[i] = 0;
	}
}

void ArchipelagoManager::AddRings(int amt, bool fromLink) {
	auto appdoc = SWA::CApplicationDocument::GetInstance();
	auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
	if (!fromLink) {
		save->moneyRing = save->moneyRing + amt;
		save->totalRing = save->totalRing + amt;
	}
	auto dayInst = CPlayerSpeedContext::GetInstance();
	if (dayInst) {
		dayInst->m_Rings = dayInst->m_Rings + amt;
		if (dayInst->m_Rings < 0) dayInst->m_Rings = 0;
		if (amt > 0) dayInst->m_ChaosEnergy = dayInst->m_ChaosEnergy + 1.3 * amt;
	} else if (evilContext) {
		evilContext->m_Rings = evilContext->m_Rings + amt;
		if (evilContext->m_Rings < 0) evilContext->m_Rings = 0;
	}
}

void ArchipelagoManager::ItemRecvCallback(int64_t item, bool notify) {
	itemState[item] += 1;
	printf("Receiving Item %s (%i, #%i)\n", ITEM_TO_NAME[item], (int)item, itemState[item]);
	if (notify) {
		auto appdoc = SWA::CApplicationDocument::GetInstance();
		auto save = (SaveData*)appdoc->m_pMember->m_spGameParameter->m_pSaveData.get();
		
		if (TrapManager::IsValidTrap(item)) TrapManager::TrapQueue.push(item);
		
		switch (item) {
			case EXTRA_LIFE: save->lives = save->lives + 1; break;
			case EXPERIENCE_20: save->experience = save->experience + 20; break;
			case EXPERIENCE_50: save->experience = save->experience + 50; break;
			case EXPERIENCE_100: save->experience = save->experience + 100; break;
			case RINGS_10: AddRings(10); break;
			case RINGS_20: AddRings(20); break;
			case RINGS_50: AddRings(50); break;
		}
	}
	
	switch (item) {
		case THE_WORLD_REPAIRED: AP_StoryComplete(); break;
	}
}

void ArchipelagoManager::LocationCheckCallback(int64_t location) {
	locationState.insert(location);
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
	}
}

void ArchipelagoManager::SetActItemsCallback(int ai) {
	actItems = ai;
}

void ArchipelagoManager::DisconnectFromArchipelago() {
	AP_Shutdown();
	ItemClearCallback();
	locationState.clear();
	hasInit = false;
	connectAttempt = false;
}

void ArchipelagoManager::ConnectToArchipelago() {
	if (connectAttempt) return;
	ItemClearCallback();
	locationState.clear();
	
	auto ini = IniFile();
	auto path = GetGamePath() / "SWAAP/connection.ini";
	if (!ini.read(path)) {
		int result = -1;
		MessageWindow::Open("Connection failed: Unable to read \"connection.ini\"", &result);
		return;
	}

	ipport = ini.getString("Connect", "ipport", "");
	slot = ini.getString("Connect", "slot", "");
	password = ini.getString("Connect", "password", "");

	if (ipport.empty() || slot.empty()) {
		int result = -1;
		MessageWindow::Open("Connection failed: Cannot find connection info in ini", &result);
		return;
	}

	AP_Init(ipport.c_str(), SWA_AP_GAME, slot.c_str(), password.c_str());
	AP_SetItemClearCallback(ItemClearCallback);
	AP_SetItemRecvCallback(ItemRecvCallback);
	AP_SetLocationCheckedCallback(LocationCheckCallback);
	AP_RegisterBouncedCallback(BounceManager::BounceRecvCallback);

	AP_RegisterSlotDataIntCallback("version", SetAPVersionCallback);
	AP_RegisterSlotDataIntCallback("progression_mode", SetProgressionModeCallback);
	AP_RegisterSlotDataIntCallback("act_items", SetActItemsCallback);
	AP_RegisterSlotDataMapIntIntCallback("act_levels", SetActOrderCallback);

	uuid = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::utc_clock::now().time_since_epoch()).count();
	
	AP_Start();
	connectAttempt = true;
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
	if (isShoeItem) {
		ctx.r3.u64 = SWAAP::ArchipelagoManager::locationState.contains(32772+ctx.r4.u64);
	} else {
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
	}
	
	return;
}

// Medal Patch

static int getMedalCount(int levelItem, int medalItem) {
	int numMedal = 0;
	const int medalList[] = {0, 15, 30, 45, 60, 80, 120};
	if (SWAAP::ArchipelagoManager::itemState[levelItem] > 6) numMedal = 200;
	else numMedal = medalList[SWAAP::ArchipelagoManager::itemState[levelItem]];

	numMedal += SWAAP::ArchipelagoManager::itemState[medalItem];
	
	return numMedal;
}

void SWA_AP_MedalCountMidAsmHook(PPCRegister& r3, PPCRegister& r10, PPCRegister &r11) {
	uint8_t* base = g_memory.base;
	
	int levelItem = r10.u64 == 12 ? PROGRESSIVE_MOON_LEVEL : PROGRESSIVE_SUN_LEVEL;
	int medalItem = r10.u64 == 12 ? MOON_MEDAL : SUN_MEDAL;
	
	r3.u64 = getMedalCount(levelItem, medalItem);
	PPC_STORE_U32(r10.u32 + r11.u32, getMedalCount(levelItem, medalItem));
}

PPC_FUNC_IMPL(__imp__sub_82589F38);
PPC_FUNC(sub_82589F38) {
//	printf("r3: %08x\n", ctx.r3.u32);
	__imp__sub_82589F38(ctx, base);
}

void SWA_AP_CountryDisappearMidAsmHook(PPCRegister& f28, PPCRegister& r11) {
	uint8_t* base = g_memory.base;
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

	int country = PPC_LOAD_U32(r11.u32 + 0x24);
	if (!(country >= 0 && country < 9)) return;
	bool canAccess = SWAAP::ArchipelagoManager::itemState[countryToItem[country]];
	if (country == 8) {
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
		if (!(hasEmeralds && getMedalCount(PROGRESSIVE_SUN_LEVEL, SUN_MEDAL) >= 120 && getMedalCount(PROGRESSIVE_MOON_LEVEL, MOON_MEDAL) >= 80)) {
			canAccess = false;
		}
	}
	if (!canAccess) f28.f64 = 0;
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

	/*
		<name>eFlag_Sunafkin_Country
			<item>PetraCapital</item>
			<item>Mykonos</item>
			<item>China</item>
			<item>Snow</item>
			<item>EuropeanCity</item>
			<item>SouthEastAsia</item>
			<item>NYCity</item>
			<item>Africa</item>
			<item>EggManBase</item>
		</name>
		uS_g
	*/
	if (r30.u32 != -1) {
		switch (countryToItem[r30.u32]) {
			case HUB_UNLOCK_APOTOS: SWAAP::setListFlag("eFlag_Sunafkin_Country", 1); break;
			case HUB_UNLOCK_SPAGONIA: SWAAP::setListFlag("eFlag_Sunafkin_Country", 4); break;
			case HUB_UNLOCK_CHUNNAN: SWAAP::setListFlag("eFlag_Sunafkin_Country", 2); break;
			case HUB_UNLOCK_MAZURI: SWAAP::setListFlag("eFlag_Sunafkin_Country", 7); break;
			case HUB_UNLOCK_HOLOSKA: SWAAP::setListFlag("eFlag_Sunafkin_Country", 3); break;
			case HUB_UNLOCK_SHAMAR: SWAAP::setListFlag("eFlag_Sunafkin_Country", 0); break;
			case HUB_UNLOCK_ADABAT: SWAAP::setListFlag("eFlag_Sunafkin_Country", 5); break;
			case HUB_UNLOCK_EMPIRE_CITY: SWAAP::setListFlag("eFlag_Sunafkin_Country", 6); break;
			case HUB_UNLOCK_EGGMANLAND: SWAAP::setListFlag("eFlag_Sunafkin_Country", 8); break;
		}
	}
}

// Entrance Activation

PPC_FUNC_IMPL(__imp__sub_8273EFC0);
PPC_FUNC(sub_8273EFC0) {
	int areaType = PPC_LOAD_U32(ctx.r3.u32 + 244);
	int doorType = PPC_LOAD_U32(ctx.r3.u32 + 268);
	char* stageType = (char*)MmGetHostAddress(PPC_LOAD_U32(ctx.r3.u32 + 260));

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
			if (!strcmp(stageType, "ActD_Extra_01")) apStage = "Tornado Defense Act 1";
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
			if (!strcmp(stageType, "ActD_Extra_02")) apStage = "Tornado Defense Act 2";
			if (!strcmp(stageType, "boss_darkgaia")) {apStage = "Dark Gaia"; isBoss = 2;}
			if (!strcmp(stageType, "boss_eggmanland")) {apStage = "Egg Dragoon"; isBoss = 2;}
			if (!strcmp(stageType, "Act_EggManLand")) apStage = "Eggmanland";
			break;
	}
	if (*apStage.c_str() == '\0') {
		__imp__sub_8273EFC0(ctx, base);
		return;
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
//	printf("Entrance @ %08x\n", ctx.r3.u32);
//	printf("\tAreaType: %i\n", areaType);
//	printf("\tDoorType: %i\n", doorType);
//	printf("\tNeedMedalNum: %i\n", needMedalNum);
//	printf("\tStageType: %s\n", stageType);

	if (isBoss == 2) PPC_STORE_U32(ctx.r3.u32 + 268, 0);
	if (isBoss == 3) PPC_STORE_U32(ctx.r3.u32 + 268, 1);
	if (!stageAllowed) {
		PPC_STORE_U32(ctx.r3.u32 + 268, 2);
		return;
	}
	
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

// Get Money Ring

PPC_FUNC_IMPL(__imp__sub_82519FD0);
PPC_FUNC(sub_82519FD0) {
	const char* curHubChar = SWAAP::getListFlag("eFlag_LastTown");
	if (!curHubChar) return;
	std::string curHub = curHubChar;
	int currency = 0;
	if (curHub == "eFlag_TownState_PetraCapital") currency = SHAMAR_CURRENCY;
	if (curHub == "eFlag_TownState_Mykonos") currency = APOTOS_CURRENCY;
	if (curHub == "eFlag_TownState_China") currency = CHUNNAN_CURRENCY;
	if (curHub == "eFlag_TownState_Snow") currency = HOLOSKA_CURRENCY;
	if (curHub == "eFlag_TownState_EuropeanCity") currency = SPAGONIA_CURRENCY;
	if (curHub == "eFlag_TownState_SouthEastAsia") currency = ADABAT_CURRENCY;
	if (curHub == "eFlag_TownState_NYCity") currency = EMPIRE_CITY_CURRENCY;
	if (curHub == "eFlag_TownState_Africa") currency = MAZURI_CURRENCY;
	if (curHub == "eFlag_TownState_EggManBase") currency = EGGMANLAND_CURRENCY;

	if (SWAAP::ArchipelagoManager::itemState[currency]) __imp__sub_82519FD0(ctx, base);
	else ctx.r3.u64 = 0;
}

// Stage Return Data (size: 80)
// offset 0: vtable (820C5BDC)
// offset 28: AreaType

//PPC_FUNC_IMPL(__imp__sub_82B5B520);
//PPC_FUNC(sub_82B5B520) {
//	__imp__sub_82B5B520(ctx, base);
//	printf("Stage Return Location: %08x\n", ctx.r3.u32);
//}

// Flag Reference: sub_825042B0
