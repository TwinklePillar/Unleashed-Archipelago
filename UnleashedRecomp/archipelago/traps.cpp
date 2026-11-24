#include "SWAAP.h"
#include "apshared.h"

#include "user/config.h"

namespace SWAAP {

bool TrapBase::OnReceive(PPCContext& ctx, uint8_t* base, int trapID) {return true;}
bool TrapBase::Update(double dt, PPCContext& ctx, uint8_t* base) {return false;}

struct TimedTrap : TrapBase {
	double timer;

	bool TickTimer(double dt) {
		timer -= dt;
		return timer > 0;
	}

	bool OnReceive(PPCContext& ctx, uint8_t* base, int trapID) {
		auto newTrap = new TimedTrap;
		newTrap->trapID = trapID;
		newTrap->timer = 20;
		TrapManager::ActiveTraps.push_back(newTrap);
		return true;
	}
	
	bool Update(double dt, PPCContext& ctx, uint8_t* base) {
		return TickTimer(dt);
	}
};

struct DeathTrap : TrapBase {
	bool OnReceive(PPCContext& ctx, uint8_t* base, int trapID) {
		auto newTrap = new DeathTrap;
		newTrap->trapID = DEATH_TRAP;
		TrapManager::ActiveTraps.push_back(newTrap);

		BounceManager::fromDeath = true;
		if (ArchipelagoManager::evilContext) ArchipelagoManager::evilContext->m_Health = 0;
		
		return true;
	}
	bool Update(double dt, PPCContext& ctx, uint8_t* base) {
		return false;
	}
};

std::map<int, TrapBase*> TrapManager::StaticTrap = {
	{REVERSE_TRAP, new TimedTrap{}},
	{SLOW_TRAP, new TimedTrap{}},
	{FAST_TRAP, new TimedTrap{}},
	{DEATH_TRAP, new DeathTrap{}}
};

std::map<std::string, int> TrapManager::TrapNameToID = {
	{"Reverse Trap", REVERSE_TRAP},
	{"Slow Trap", SLOW_TRAP},
	{"Fast Trap", FAST_TRAP}
};

double TrapManager::TrapTimer{};

std::vector<TrapBase*> TrapManager::ActiveTraps{};
std::queue<int> TrapManager::TrapQueue{};
std::queue<int> TrapManager::PrioTrapQueue{};

bool TrapManager::IsValidTrap(int trapID) {
	return StaticTrap.contains(trapID);
}

bool TrapManager::IsActive(int trapID) {
	for (auto trap : ActiveTraps) {
		if (trap->trapID == trapID) return true;
	}
	return false;
}

void TrapManager::Update(double dt, PPCContext& ctx, uint8_t* base) {
	for (int i = ActiveTraps.size()-1; i >= 0; i--) {
		if (!ActiveTraps[i]->Update(dt, ctx, base)) {
			delete ActiveTraps[i];
			ActiveTraps.erase(ActiveTraps.begin()+i);
		}
	}
	
	TrapTimer -= dt;
	if (TrapTimer < 0) {
		int nextTrap = 0;
		bool isPrio = false;
		if (!PrioTrapQueue.empty()) {
			isPrio = true;
			nextTrap = PrioTrapQueue.front();
			PrioTrapQueue.pop();
		}
		if (!nextTrap && !TrapQueue.empty()) {
			nextTrap = TrapQueue.front();
			TrapQueue.pop();
		}
		if (StaticTrap.contains(nextTrap)) {
			if (StaticTrap[nextTrap]->OnReceive(ctx, base, nextTrap)) {
				TrapTimer = Config::TrapTimer;
				if (!isPrio && nextTrap < AP_ITEM_MAX) BounceManager::SendTrapLink(ITEM_TO_NAME[nextTrap]);
			} else if (!isPrio) {
				TrapQueue.push(nextTrap);
			}
		}
	}
}

int TrapManager::GetTrapFromName(std::string name) {
	if (TrapNameToID.contains(name)) return TrapNameToID[name];
	return 0;
}

}

// Day Sonic - Get Death Floor Height

PPC_FUNC_IMPL(__imp__sub_82316480);
PPC_FUNC(sub_82316480) {
	if (SWAAP::TrapManager::IsActive(DEATH_TRAP)) ctx.f1.f64 = 100000000;
	else __imp__sub_82316480(ctx, base);
}
