#include "SWAAP.h"
#include "apshared.h"
#include <Archipelago.h>

#include <cstdio>

namespace SWAAP {
	static int inferStageID() {
		std::string town = FlagManager::GetListFlag("eFlag_LastTown");
		if (town == "eFlag_TownState_PetraCapital") return 80000;
		if (town == "eFlag_TownState_Mykonos") return 60000;
		if (town == "eFlag_TownState_China") return 30000;
		if (town == "eFlag_TownState_Snow") return 90000;
		if (town == "eFlag_TownState_EuropeanCity") return 50000;
		if (town == "eFlag_TownState_SouthEastAsia") return 20000;
		if (town == "eFlag_TownState_NYCity") return 70000;
		if (town == "eFlag_TownState_Africa") return 10000;
		if (town == "eFlag_TownState_EggManBase") return 40000;
		return 0;
	}
	
	bool APSharedLocation::IsCurrentLocation(std::string type, int stage, int index) {
		return false;
	}

	void APSharedLocation::CheckLocation(std::string type, int stage, int index) {
		if (stage == 0) stage = inferStageID();
		for (auto location : locations) {
			if (location->IsCurrentLocation(type, stage, index)) AP_SendItem(location->id);
		}
	}
	
	bool APSharedLocation::IsLocationChecked(std::string type, int stage, int index) {
		if (stage == 0) stage = inferStageID();
		for (auto location : locations) {
			if (location->IsCurrentLocation(type, stage, index)) return ArchipelagoManager::locationState.contains(location->id);
		}
		return false;
	}

	bool InventoryLocation::IsCurrentLocation(std::string type, int stage, int index) {
		if (type != "inventory") return false;
		if (this->stage != 0 && stage != this->stage) return false;
		if (index != this->index) return false;
		return true;
	}
	
	bool ShoeLocation::IsCurrentLocation(std::string type, int stage, int index) {
		if (type != "shoe") return false;
		if (index != this->index) return false;
		return true;
	}
	
	bool CompletionLocation::IsCurrentLocation(std::string type, int stage, int index) {
		if (type != "complete") return false;
		if (this->stage != 0 && stage != this->stage) return false;
		if (index > rank) return false;
		return true;
	}
	
	bool MedalLocation::IsCurrentLocation(std::string type, int stage, int index) {
		if (!((type == "sun" && this->sun) || (type == "moon" && !this->sun))) return false;
		if (this->stage != 0 && stage != this->stage) return false;
		if (index != this->index) return false;
		return true;
	}
	
	bool IntFlagLocation::IsCurrentLocation(std::string type, int stage, int index) {
		if (!(type == flag)) return false;
		if (index != value) return false;
		return true;
	}
	
	bool BoolFlagLocation::IsCurrentLocation(std::string type, int stage, int index) {
		if (!(type == flag)) return false;
		return true;
	}
	
}

void SWA_AP_StageCompleteMidAsmHook(PPCRegister& r18) {
	// r18: something related to stage ID?
	auto base = (uint8_t*)MmGetHostAddress(0);
	uint32_t temp = PPC_LOAD_U32(r18.u32 + 152);
	uint32_t stageID = PPC_LOAD_U32(temp + 24);
//	printf("r18: %08x, temp: %08x, temp2: %i\n", r18.u32, temp, stageID);
	SWAAP::APSharedLocation::CheckLocation("complete", stageID, 5);

	char buf[256] = {0};
	snprintf(buf, 256, "Sending %i Completion", stageID);
	SWAAP::ArchipelagoManager::DebugLog(buf);
}

void SWA_AP_MedalLocationMidAsmHook(PPCRegister& r3, PPCRegister& r29, PPCRegister& r31) {
	// r29: index
	// r31: moon/sun
	// r3: record
	auto base = (uint8_t*)MmGetHostAddress(0);
//	printf("r3: %08x, r29: %08x, r31: %08x\n", r3.u32, r29.u32, r31.u32);
	int stageID = PPC_LOAD_U32(r3.u32);
	const char* medalType = "";
	char buf[256] = {0};
	if (r31.u32) { // Sun
		medalType = "Sun";
		SWAAP::APSharedLocation::CheckLocation("sun", stageID, r29.u32);
	} else { // Moon
		medalType = "Moon";
		SWAAP::APSharedLocation::CheckLocation("moon", stageID, r29.u32);
	}

	snprintf(buf, 256, "%s Medal %i:%i", medalType, stageID, r29.s32);
	SWAAP::ArchipelagoManager::DebugLog(buf);
}

void SWA_AP_InventoryLocationMidAsmHook(PPCRegister& r4, PPCRegister& r5) {
	// r4: inventory index
	// r5: count
	if (r5.s32 > 0) {
		char buf[256] = {0};
		snprintf(buf, 256, "Sending Inventory #%i\n", r4.u32 + 1);
		SWAAP::ArchipelagoManager::DebugLog(buf);
		SWAAP::APSharedLocation::CheckLocation("inventory", 0, r4.u32 + 1);
	}
}

// Shoe Location
PPC_FUNC_IMPL(__imp__sub_82519C20);
PPC_FUNC(sub_82519C20) {
	SWAAP::APSharedLocation::CheckLocation("shoe", 0, ctx.r4.u64);
	SWAAP::ArchipelagoManager::DebugLog("Sending Shoe");
}
/*
// something involving shops
PPC_FUNC_IMPL(__imp__sub_82B280A0);
PPC_FUNC(sub_82B280A0) {
	printf("r3: %08x, r4: %08x\n", ctx.r3.u32, ctx.r4.u32);
	__imp__sub_82B280A0(ctx, base);
	printf("\tret: %08x\n", ctx.r3.u32);
}

// shop update
// 55462

PPC_FUNC_IMPL(__imp__sub_82B2E698);
PPC_FUNC(sub_82B2E698) {
	__imp__sub_82B2E698(ctx, base);
}

PPC_FUNC_IMPL(__imp__sub_82519C60);
PPC_FUNC(sub_82519C60) {
//	__imp__sub_82519C60(ctx, base);
//	printf("r3: %08x\n", ctx.r3.u32);
	ctx.r3.u64 = 8;
}
*/
