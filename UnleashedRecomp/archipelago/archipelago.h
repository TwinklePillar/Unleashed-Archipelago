#include <queue>

#define AP_MESSAGE_ACH 12407
#define AP_ITEM_MAX 512

namespace SWAAP {

struct ArchipelagoMessage {
    std::string header;
    std::string message;
};

enum APMessageType {
    SENT_ITEM,
    RECV_ITEM,
    MEMORY_DEBUG,
    MISC
};

class ArchipelagoManager {
	public:
		static bool hasInit;
		
		static void ItemClearCallback();
		static void ItemRecvCallback(int64_t item, bool notify);
		static void LocationCheckCallback(int64_t location);
		
		static void SetAPVersionCallback(int version);
		static void SetProgressionModeCallback(int progression);
		static void SetActItemsCallback(int ai);
		static void SetActOrderCallback(std::map<int, int> actorder);

		static int version;
		static int progressionMode;
		static int actItems;
		static int itemState[AP_ITEM_MAX];
		static std::map<std::string, int> actLevels;
        static inline std::queue<ArchipelagoMessage> messageQueue;
        static void Update(double dt);
        static void DisplayMessage(APMessageType type, std::string message);
        static void ConnectToArchipelago();
        static void DisconnectFromArchipelago();
};

}
