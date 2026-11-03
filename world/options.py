from dataclasses import dataclass

from Options import PerGameCommonOptions, Toggle, Choice, Range

class ProgressionMode(Choice):
	"""
	How should the game progress?

	Story: You play through the story like in vanilla
	Hub Rando: Each hub is put into the item pool
	"""
	display_name = "Progression Mode"

	option_story = 0
	option_hub_rando = 1

	default = option_story

class ActShuffle(Toggle):
	"""
	Should each level entrance shuffle to a different level?
	"""
	display_name = "Act Shuffle"

class NoMedalRequirements(Toggle):
	"""
	Should medal requirements be disabled?
	"""
	display_name = "No Medal Requirements"

class ChaosEmeralds(Toggle):
	"""
	Should the 7 chaos emeralds be shuffled into the item pool?
	"""
	display_name = "Chaos Emeralds"

@dataclass
class SWAOptions(PerGameCommonOptions):
	progression_mode: ProgressionMode
	act_shuffle: ActShuffle
	no_medal_requirements: NoMedalRequirements
	chaos_emeralds: ChaosEmeralds
