#include "Archipelago.h"
#include "SWAAP.h"
#include "user/config.h"
#include "json/json.h"

namespace SWAAP {

int BounceManager::amnesty = 0;
double BounceManager::elapsed = 0;
int BounceManager::numRing = 0;
bool BounceManager::toDeath = false;
bool BounceManager::fromDeath = false;

void BounceManager::BounceRecvCallback(AP_Bounce bounce) {
	std::set<std::string> tags{};
	Json::Value root{};
	Json::Reader reader{};
	reader.parse(bounce.data, root);
	if (bounce.tags) for (auto tag : *bounce.tags) {
		tags.emplace(tag);
	}

	if (tags.contains("DeathLink")) {
		if (!Config::DeathLinkEnabled) return;
		if (toDeath) {
			toDeath = false;
			return;
		}
		std::string cause = root["cause"].asString() == "" ? root["source"].asString() : root["cause"].asString();
		ArchipelagoManager::DisplayMessage(APMessageType::DEATH_LINK, cause, 77);
		TrapManager::PrioTrapQueue.push(DEATH_TRAP);
	}
	
	if (tags.contains("TrapLink")) {
		if (!Config::TrapLinkEnabled) return;
		if (root["source"].asString() == ArchipelagoManager::slot) return;
		int linkedTrap = TrapManager::GetTrapFromName(root["trap_name"].asString());
		if (linkedTrap) {
			TrapManager::PrioTrapQueue.push(linkedTrap);
			char buf[256] = {0};
			snprintf(buf, 256, "%s from %s", root["trap_name"].asString().c_str(), root["source"].asString().c_str());
			ArchipelagoManager::DisplayMessage(APMessageType::TRAP_LINK, buf, 82);
		}
	}
	
	if (tags.contains("RingLink")) {
		if (!Config::RingLinkEnabled) return;
		if (root["source"].asInt64() == ArchipelagoManager::uuid) return;
		auto amt = root["amount"].asInt();
		ArchipelagoManager::AddRings(amt, true);
	}
}

void BounceManager::SendLink(Json::Value root, std::string tag) {
	Json::FastWriter writer{};
	AP_Bounce bounce{};
	
	std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
	root["time"] = (int64_t)std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count();
	bounce.data = writer.write(root);
	bounce.games = nullptr;
	std::vector<std::string> tags = {tag};
	bounce.tags = &tags;
	AP_SendBounce(bounce);
}

void BounceManager::SendDeathLink(std::string cause) {
	if (!Config::DeathLinkEnabled) return;
	if (fromDeath) {
		fromDeath = false;
		return;
	}
	Json::Value root{};
	amnesty += 1;
	if (amnesty >= Config::DeathLinkAmnesty) {
		amnesty = 0;
	} else return;
 	root["source"] = ArchipelagoManager::slot;
	if (!cause.empty()) {
		root["cause"] = std::format("{0} {1}", ArchipelagoManager::slot, cause);
	}
	toDeath = true;
	SendLink(root, "DeathLink");
}

void BounceManager::SendTrapLink(std::string trap) {
	if (!Config::TrapLinkEnabled) return;
	Json::Value root{};
	root["source"] = ArchipelagoManager::slot;
	root["trap_name"] = trap;
	SendLink(root, "TrapLink");
}

void BounceManager::SendRingLink(int amt) {
	if (!Config::RingLinkEnabled) return;
	numRing += amt;
}

void BounceManager::Update(double dt) {
	if (ArchipelagoManager::hasInit) {
		elapsed += dt;
		if (elapsed > 0.25) {
			elapsed = 0;
			
			std::set<std::string> newTags{};
			if (Config::DeathLinkEnabled) newTags.emplace("DeathLink");
			if (Config::TrapLinkEnabled) newTags.emplace("TrapLink");
			if (Config::RingLinkEnabled) newTags.emplace("RingLink");
			if (newTags != ArchipelagoManager::tags) {
				ArchipelagoManager::tags = newTags;
				std::vector<std::string> tagVec{};
				for (std::string tag : newTags) {
					tagVec.push_back(tag);
				}
				AP_UpdateTags(tagVec);
			}
			
			if (numRing != 0) {
				Json::Value root{};
				root["source"] = ArchipelagoManager::uuid;
				root["amount"] = numRing;
				SendLink(root, "RingLink");
				numRing = 0;
			}
		}
	}
}

}

void SWA_AP_PosRingLinkMidAsmHook(PPCRegister& r4) {
	SWAAP::BounceManager::SendRingLink(r4.u32);
}

void SWA_AP_NegRingLinkMidAsmHook(PPCRegister& r4) {
	SWAAP::BounceManager::SendRingLink(-r4.u32);
}

// some sorta generic life setting function?

PPC_FUNC_IMPL(__imp__sub_82308ED8);
PPC_FUNC(sub_82308ED8) {
	if (ctx.r4.s32 < 0) SWAAP::BounceManager::SendDeathLink(""); // could try to be more specific but ehh
	__imp__sub_82308ED8(ctx, base);
}
