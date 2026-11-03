from worlds.AutoWorld import World
from Options import OptionError

from .items import ItemState
from .locations import LocationState
from .options import SWAOptions

from .apshared import apshared
from .apshared import location_name_to_id as lname_to_id
from .apshared import item_name_to_id as iname_to_id

class SWAWorld(World):
	ut_can_gen_without_yaml = True
	
	game = apshared["game"]
	location_name_to_id = lname_to_id
	item_name_to_id = iname_to_id

	options_dataclass = SWAOptions
	options: SWAOptions

	def __init__(self, multiworld, player):
		super().__init__(multiworld, player)
		self.location_state = LocationState()
		self.item_state = ItemState()
	
	def generate_early(self):
		self.spoiler_text = ""
		self.progression_mode = self.options.progression_mode.value
		self.act_shuffle = self.options.act_shuffle.value
		self.item_state.no_medal_requirements = self.options.no_medal_requirements.value
		self.item_state.chaos_emeralds = self.options.chaos_emeralds.value
		
		self.location_state.regen = False
		re_gen_passthrough = getattr(self.multiworld, "re_gen_passthrough", {})
		if re_gen_passthrough and self.game in re_gen_passthrough:
			slot_data = re_gen_passthrough[self.game]
			self.progression_mode = slot_data["progression_mode"]
			self.location_state.sanities = slot_data["sanities"]
			self.location_state.act_order = slot_data["act_order"]
			self.location_state.regen = True
			return

	def create_regions(self):
		self.location_state.setup_regions(self)

	def create_items(self):
		self.item_state.setup_items(self)

	def create_item(self, name):
		return self.item_state.create_item(self, name)

	def get_filler_item_name(self):
		return self.item_state.get_filler_item(self)

	def write_spoiler(self, spoiler_handle):
		spoiler_handle.write(self.spoiler_text)

	def fill_slot_data(self):
		return {
			"version": apshared["version"],
			"sanities": self.location_state.sanities,
			"progression_mode": self.progression_mode,
			"act_order": self.location_state.act_order
		}

	@staticmethod
	def interpret_slot_data(slot_data):
		return slot_data
