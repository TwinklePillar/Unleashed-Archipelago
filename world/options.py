from dataclasses import dataclass

from Options import PerGameCommonOptions, Toggle, DefaultOnToggle, Choice, Range

class ProgressionMode(Choice):
	"""
	How should the game progress?

	Hub Rando: Each hub is put into the item pool
	"""
	display_name = "Progression Mode"

	option_hub_rando = 1

	default = option_hub_rando

class GoalStage(Choice):
	"""
	Which stage should hold the goal item?
	"""
	display_name = "Goal Stage"

	option_dark_gaia = 0
	option_egg_dragoon = 1
	option_eggmanland = 2

	default = option_dark_gaia

class ActShuffle(Toggle):
	"""
	Should each level entrance shuffle to a different level?
	"""
	display_name = "Act Shuffle"

class ActItems(Toggle):
	"""
	Should each act be locked by an item? You will still need enough sun/moon levels and the hub to access it
	"""
	display_name = "Act Items"

class IncludeDay(DefaultOnToggle):
	"""
	Should the Day Sonic locations be included?
	Regardless of this setting, you may be required to transform for:
	- Eggmanland
	- Entrance Fields
	- Missions
	"""
	display_name = "Include Day Locations"

class IncludeNight(DefaultOnToggle):
	"""
	Should the Night Sonic locations be included?
	Regardless of this setting, you may be required to transform for:
	- Eggmanland
	- Entrance Fields
	- Missions
	"""
	display_name = "Include Night Locations"

class RandomizeMedalRequirements(Choice):
	"""
	What should be the medal requirements for acts?
	"""
	display_name = "Medal Requirements"

	option_vanilla = 0
	option_randomize = 1
	option_no_requirements = 2

	default = option_vanilla
	
class MedalItems(Toggle):
	"""
	Should there be 200 sun and 200 moon medals in the pool instead of 6 progressive levels each?
	"""
	display_name = "Medal Items"

class ChaosEmeralds(DefaultOnToggle):
	"""
	Should the 7 chaos emeralds be shuffled into the item pool?
	"""
	display_name = "Chaos Emeralds"

class EnableShop(Choice):
	"""
	Should the shop items be locations?
	"""
	display_name = "ShopSanity"

	option_disabled = 0
	option_other_sanities_only = 1
#	option_all = 2

	default = option_disabled

class ShuffleCurrencies(DefaultOnToggle):
	"""
	Should the currencies for each country be shuffled into the item pool?
	"""
	display_name = "Shuffle Currencies"

class EnableMissions(Toggle):
	"""
	Should locations from missions be counted for sanities?
	"""
	display_name = "Enable Missions"

class MedalSanity(Toggle):
	"""
	Should the sun and moon medals be locations?
	"""
	display_name = "MedalSanity"

class MediaSanity(Toggle):
	"""
	Should the records, tapes, and books be locations?
	"""
	display_name = "MediaSanity"

class TrapChance(Range):
	"""
	What should be the chance of a filler item being replaced with a random trap?
	"""
	display_name = "Trap Chance"

	range_start = 0
	range_end = 100

	default = 10

@dataclass
class SWAOptions(PerGameCommonOptions):
	progression_mode: ProgressionMode
	goal_stage: GoalStage
	act_items: ActItems
	include_day: IncludeDay
	include_night: IncludeNight
#	act_shuffle: ActShuffle
	random_act_levels: RandomizeMedalRequirements
	medal_items: MedalItems
	chaos_emeralds: ChaosEmeralds
	shuffle_currencies: ShuffleCurrencies
	shopsanity: EnableShop
	enable_missions: EnableMissions
	medalsanity: MedalSanity
	mediasanity: MediaSanity
	trap_chance: TrapChance
