from BaseClasses import Item, ItemClassification
from enum import Enum

class ItemType(Enum):
	progression = 0
	filler = 1
	car = 2
	evidence = 3
	trap = 4

from .apshared import *

class SWAItem(Item):
	game = apshared["game"]

class ItemState:
	def __init__(self):
		self.item_class = {}
		self.no_medal_requirements = 0
		self.chaos_emeralds = 0
		for item in apshared["items"]:
			self.item_class[item["name"]] = item["classif"]
	
	def create_item(self, world, item):
		return SWAItem(item, self.item_class[item], item_name_to_id[item], world.player)

	def get_filler_item(self, world):
		return world.random.choice(item_name_groups["Junk"])
	
	def setup_items(self, world):
		itempool = []
		precollect = []

		if world.location_state.include_day:
			for i in item_name_groups["Shoes"]:
				itempool.append(world.create_item(i))
		
		for i in range(6):
			if self.no_medal_requirements:
				precollect.append(PROGRESSIVE_SUN_LEVEL)
				precollect.append(PROGRESSIVE_MOON_LEVEL)
			else:
				itempool.append(world.create_item(PROGRESSIVE_SUN_LEVEL))
				itempool.append(world.create_item(PROGRESSIVE_MOON_LEVEL))

		for i in item_name_groups["Emeralds"]:
			if self.chaos_emeralds:
				itempool.append(world.create_item(i))
			else:
				precollect.append(i)
		
		if world.progression_mode == 1:
			for i in item_name_groups["Hubs"]:
				if i == world.free_hub: precollect.append(i)
				else: itempool.append(world.create_item(i))
		
		if world.act_items:
			for i in item_name_groups["Acts"]:
				itempool.append(world.create_item(i))
		
		num_unfilled = len(world.multiworld.get_unfilled_locations(world.player))
		num_filler = num_unfilled - len(itempool)
		for i in range(num_filler):
			itempool.append(world.create_filler())
		world.multiworld.itempool += itempool
		for i in precollect:
			world.push_precollected(world.create_item(i))
