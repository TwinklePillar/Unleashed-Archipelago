from worlds.AutoWorld import World
from Options import OptionError

from .items import ItemState
from .locations import LocationState
from .options import SWAOptions

from .apshared import (
	apshared,
	HUB_UNLOCK_APOTOS,
	HUB_UNLOCK_MAZURI,
	HUB_UNLOCK_SPAGONIA,
	HUB_UNLOCK_HOLOSKA,
	HUB_UNLOCK_CHUNNAN,
	HUB_UNLOCK_SHAMAR,
	HUB_UNLOCK_EMPIRE_CITY,
	HUB_UNLOCK_ADABAT
)
from .apshared import location_name_to_id as lname_to_id
from .apshared import item_name_to_id as iname_to_id
from .apshared import location_name_groups as lname_groups
from .apshared import item_name_groups as iname_groups

class SWAWorld(World):
	ut_can_gen_without_yaml = True
	
	game = apshared["game"]
	location_name_to_id = lname_to_id
	item_name_to_id = iname_to_id
	location_name_groups = lname_groups
	item_name_groups = iname_groups

	options_dataclass = SWAOptions
	options: SWAOptions

	def __init__(self, multiworld, player):
		super().__init__(multiworld, player)
		self.location_state = LocationState()
		self.item_state = ItemState()
	
	def generate_early(self):
		self.spoiler_text = ""
		self.progression_mode = self.options.progression_mode.value
		self.random_act_levels = self.options.random_act_levels.value
#		self.item_state.no_medal_requirements = self.options.no_medal_requirements.value
		self.item_state.chaos_emeralds = self.options.chaos_emeralds.value
		self.act_items = self.options.act_items.value
		
		self.location_state.include_day = self.options.include_day.value
		self.location_state.include_night = self.options.include_night.value
		
		if self.options.medalsanity:
			self.location_state.sanities += ["sun", "moon"]
		
		self.location_state.regen = False
		re_gen_passthrough = getattr(self.multiworld, "re_gen_passthrough", {})
		if re_gen_passthrough and self.game in re_gen_passthrough:
			slot_data = re_gen_passthrough[self.game]
			self.progression_mode = slot_data["progression_mode"]
			self.act_items = slot_data["act_items"]
			self.location_state.sanities = slot_data["sanities"]
			self.location_state.act_levels = slot_data["act_levels"]
			self.location_state.include_day = 1
			self.location_state.include_night = 1
			self.location_state.regen = True
			return

		self.free_hub = self.random.choice((
			HUB_UNLOCK_APOTOS,
			HUB_UNLOCK_MAZURI,
			HUB_UNLOCK_SPAGONIA,
			HUB_UNLOCK_HOLOSKA,
			HUB_UNLOCK_CHUNNAN,
			HUB_UNLOCK_SHAMAR,
			HUB_UNLOCK_ADABAT
		))

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
			"act_levels": self.location_state.act_levels,
			"act_items": self.act_items
		}

	@staticmethod
	def interpret_slot_data(slot_data):
		return slot_data
