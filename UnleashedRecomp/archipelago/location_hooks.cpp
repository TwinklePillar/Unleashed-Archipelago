#include "archipelago.h"
#include <Archipelago.h>

#include <cstdio>

void SWA_AP_StageCompleteMidAsmHook(PPCRegister& r18) {
	// r18: something related to stage ID?
	auto base = (uint8_t*)MmGetHostAddress(0);
	uint32_t temp = PPC_LOAD_U32(r18.u32 + 152);
	uint32_t stageID = PPC_LOAD_U32(temp + 24);
	printf("r18: %08x, temp: %08x, temp2: %i\n", r18.u32, temp, stageID);
	AP_SendItem(stageID * 1000);

	char buf[256] = {0};
	snprintf(buf, 256, "Sending %i Completion", stageID);
	SWAAP::ArchipelagoManager::DebugLog(buf);
}

void SWA_AP_MedalLocationMidAsmHook(PPCRegister& r3, PPCRegister& r29, PPCRegister& r31) {
	// r29: index
	// r31: moon/sun
	// r3: record
	auto base = (uint8_t*)MmGetHostAddress(0);
	printf("r3: %08x, r29: %08x, r31: %08x\n", r3.u32, r29.u32, r31.u32);
	int stageID = PPC_LOAD_U32(r3.u32);
	const char* medalType = "";
	char buf[256] = {0};
	if (r31.u32) { // Sun
		medalType = "Sun";
		AP_SendItem(stageID * 1000 + 60 + r29.u32);
	} else { // Moon
		medalType = "Moon";
		AP_SendItem(stageID * 1000 + 20 + r29.u32);
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
		AP_SendItem(r4.u32 + 1);
	}
}

// Shoe Location
PPC_FUNC_IMPL(__imp__sub_82519C20);
PPC_FUNC(sub_82519C20) {
	AP_SendItem(32772 + ctx.r4.u64);
	SWAAP::ArchipelagoManager::DebugLog("Sending Shoe");
}

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
