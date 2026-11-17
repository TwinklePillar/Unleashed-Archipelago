#include "kernel/memory.h"
#include <SWA.h>

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
		SWA_INSERT_PADDING(0x5A0);
		be<uint32_t> moneyRing;
		SWA_INSERT_PADDING(0x30);
		be<uint32_t> experience;
		SWA_INSERT_PADDING(4);
		be<uint32_t> totalRing;
		SWA_INSERT_PADDING(0x19C);
	};
	
	class FlagManager {
		public:
		union Flag {
			xpointer<const char> name;
			struct {
				xpointer<const char> name;
				bool value;
			} bFlag;
			struct {
				xpointer<const char> name;
				be<uint32_t> value;
			} iFlag;
			struct {
				xpointer<const char> name;
				hh::vector<xpointer<char>> named;
				be<uint32_t> value;
			} eFlag;
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

	class ShopObject {
		public:
		struct ShopData {
			xpointer<char> id;
			SWA_INSERT_PADDING(4);
			be<int32_t> price;
		};
		struct ShopItem {
			xpointer<ShopData> shopData;
			be<uint32_t> shopObj; // some sort of object
		};
		SWA_INSERT_PADDING(540);
		xpointer<hh::vector<ShopItem>> items;
		SWA_INSERT_PADDING(4);
		be<int32_t> index;
	};
	
	class CPlayerSpeedContext {
		public:
		SWA_INSERT_PADDING(0x538);
		be<int32_t> m_Rings;
		be<float> m_ringEnergy;

		static CPlayerSpeedContext* GetInstance() {
			return *(xpointer<CPlayerSpeedContext>*)MmGetHostAddress(0x83364724);
		}
	};
	
	class CEvilSonicContext {
		public:
		SWA_INSERT_PADDING(0x660);
		be<int32_t> m_Rings;
		SWA_INSERT_PADDING(0x24);
        be<float> m_DarkGaiaEnergy;
        SWA_INSERT_PADDING(0x138);
        be<uint32_t> m_AnimationID;
        SWA_INSERT_PADDING(0x38);
        be<float> m_UnkHudGuideF32;
        be<uint32_t> m_UnkHudGuideU32;
        SWA_INSERT_PADDING(0x18);
        be<SWA::Player::EGuideType> m_GuideType;
        SWA_INSERT_PADDING(0xA8);
        be<uint32_t> m_OutOfControlCount;
	};
}
