from dataclasses import dataclass

from Options import PerGameCommonOptions, Toggle, DefaultOnToggle, Choice, Range

class ProgressionMode(Choice):
	"""
	How should the game progress?

	Hub Rando: Each hub is put into the item pool
	Act Rando: Each hub AND act is put into the item pool
	"""
	display_name = "Progression Mode"

	option_hub_rando = 1
	option_act_rando = 2

	default = option_hub_rando

class ActShuffle(Toggle):
	"""
	Should each level entrance shuffle to a different level?
	"""
	display_name = "Act Shuffle"

class RandomizeMedalRequirements(Toggle):
	"""
	Should medal requirement for each act be randomized?
	"""
	display_name = "Randomize Medal Requirements"

class ChaosEmeralds(DefaultOnToggle):
	"""
	Should the 7 chaos emeralds be shuffled into the item pool?
	"""
	display_name = "Chaos Emeralds"

@dataclass
class SWAOptions(PerGameCommonOptions):
	progression_mode: ProgressionMode
#	act_shuffle: ActShuffle
	random_act_levels: RandomizeMedalRequirements
	chaos_emeralds: ChaosEmeralds
