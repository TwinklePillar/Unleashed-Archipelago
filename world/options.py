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
	You may be required to transform into the Hedgehog in entrance fields regardless of this setting
	"""
	display_name = "Include Day Locations"

class IncludeNight(DefaultOnToggle):
	"""
	Should the Night Sonic locations be included?
	You may be required to transform into the Werehog in entrance fields regardless of this setting
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
	

class ChaosEmeralds(DefaultOnToggle):
	"""
	Should the 7 chaos emeralds be shuffled into the item pool?
	"""
	display_name = "Chaos Emeralds"

class MedalSanity(Toggle):
	"""
	Should the sun and moon medals be locations?
	"""
	display_name = "MedalSanity"

@dataclass
class SWAOptions(PerGameCommonOptions):
	progression_mode: ProgressionMode
	act_items: ActItems
	include_day: IncludeDay
	include_night: IncludeNight
#	act_shuffle: ActShuffle
	random_act_levels: RandomizeMedalRequirements
	chaos_emeralds: ChaosEmeralds
	medalsanity: MedalSanity
