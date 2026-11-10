from BaseClasses import Region, Location
from rule_builder.rules import True_, Has, HasAll, CanReachLocation
from .apshared import *
from .items import SWAItem

class SWALocation(Location):
	game = apshared["game"]

class LocationState:
	def __init__(self):
		self.er_regions = []
		self.sanities = ["complete", "shoe"]
		self.regen = False
		self.act_levels = {}
		
		self.include_day = 1
		self.include_night = 1

	def setup_regions(self, world):
		menu_region = Region("Menu", world.player, world.multiworld)
		world.multiworld.regions.append(menu_region)

		hubs = []
		free_hub = ()
		sphere_zero_stages = []
		all_stages = []
		all_stages_unshuffled = []
		unshuffled_stages = []
		for stage in apshared["stages"]:
			if stage["type"] == "stage":
				all_stages.append(stage["name"])
				all_stages_unshuffled.append(stage["name"])
			elif stage["type"] == "unshuffled_stage":
				unshuffled_stages.append(stage["name"])
			
			if stage["evil"] == 0 and not self.include_day: continue
			if stage["evil"] == 1 and not self.include_night: continue
			stage_region = Region(stage["name"], world.player, world.multiworld)
			world.multiworld.regions.append(stage_region)
			for check in stage["checks"]:
				if check["evil"] == 0 and not self.include_day: continue
				if check["evil"] == 1 and not self.include_night: continue
				check_name = f"{stage['name']} - {check['name']}"
				if check["sanity"] in self.sanities:
					stage_region.add_locations({check_name: location_name_to_id[check_name]}, SWALocation)
					rule = True_()
					for req in check["req"]:
						if req in item_name_to_id.keys():
							rule = rule & Has(req)
						else:
							rule = rule & Has(f"{req} Complete")
					world.set_rule(world.get_location(check_name), rule)
					if check["name"] == "Completion" and len(check["req"]) == 0:
						sphere_zero_stages.append(stage["name"])
			
			if stage["type"] == "hub":
				hubs.append((stage, stage_region))
				if stage["name"] == "Apotos Hub" and world.free_hub == HUB_UNLOCK_APOTOS: free_hub = (stage, stage_region)
				if stage["name"] == "Mazuri Hub" and world.free_hub == HUB_UNLOCK_MAZURI: free_hub = (stage, stage_region)
				if stage["name"] == "Spagonia Hub" and world.free_hub == HUB_UNLOCK_SPAGONIA: free_hub = (stage, stage_region)
				if stage["name"] == "Chun-Nan Hub" and world.free_hub == HUB_UNLOCK_CHUNNAN: free_hub = (stage, stage_region)
				if stage["name"] == "Holoska Hub" and world.free_hub == HUB_UNLOCK_HOLOSKA: free_hub = (stage, stage_region)
				if stage["name"] == "Shamar Hub" and world.free_hub == HUB_UNLOCK_SHAMAR: free_hub = (stage, stage_region)
				if stage["name"] == "Empire City Hub" and world.free_hub == HUB_UNLOCK_EMPIRE_CITY: free_hub = (stage, stage_region)
				if stage["name"] == "Adabat Hub" and world.free_hub == HUB_UNLOCK_ADABAT: free_hub = (stage, stage_region)
				continue

		level_weights = [0, 4, 4, 3, 3, 2, 2, 1]
		levels = []
		for i in range(len(level_weights)):
			for j in range(level_weights[i]):
				levels.append(i)
		if not self.regen:
			if world.random_act_levels == 1:
				free_stages = []
				for sub in free_hub[0]["sub"]:
					if sub["evil"] == 0 and not self.include_day: continue
					if sub["evil"] == 1 and not self.include_night: continue
					if len(sub["req"]): continue
					free_stages.append(sub["stage"])
				
				world.random.shuffle(free_stages)
				free_stages = free_stages[:2 if len(free_stages) > 2 else len(free_stages)]
				for i in range(len(all_stages_unshuffled)):
					if all_stages_unshuffled[i] in free_stages:
						self.act_levels[str(i)] = 1
					else:
						self.act_levels[str(i)] = world.random.choice(levels)
			elif world.random_act_levels == 2:
				for i in range(len(all_stages_unshuffled)):
					self.act_levels[str(i)] = 1
			else:
				for i in range(len(all_stages_unshuffled)):
					self.act_levels[str(i)] = -1
		
		world.spoiler_text += f"Act Levels for {world.multiworld.player_name[world.player]}:\n"
		for i in range(len(all_stages)):
			world.spoiler_text += f"\t{all_stages_unshuffled[i]} -> {self.act_levels[str(i)]} ({'Moon' if 'Night' in all_stages_unshuffled[i] else 'Sun'})\n"
		
		for stage in unshuffled_stages:
			all_stages.append(stage)
			all_stages_unshuffled.append(stage)
		
		for hub in hubs:
			for sub in hub[0]["sub"]:
				if sub["evil"] == 0 and not self.include_day: continue
				if sub["evil"] == 1 and not self.include_night: continue
				shuffled_stage = all_stages[all_stages_unshuffled.index(sub["stage"])]
				stage_region = world.get_region(shuffled_stage)
				stage_region.add_event(
					f"{sub['stage']} Complete", f"{sub['stage']} Complete",
					location_type=SWALocation, item_type=SWAItem,
					rule=CanReachLocation(f"{shuffled_stage} - Completion"), show_in_spoiler=False
				)
				hub[1].connect(stage_region)
				enter = world.get_entrance(f"{hub[1].name} -> {stage_region.name}")
				index = str(all_stages_unshuffled.index(sub["stage"]))
				if index in self.act_levels:
					level = self.act_levels[index]
				else:
					level = -1
				if (level == -1): level = sub["level"]
				if level > 1:
					if sub["evil"]: rule = Has(PROGRESSIVE_MOON_LEVEL, level-1)
					else: rule = Has(PROGRESSIVE_SUN_LEVEL, level-1)
				if world.act_items:
					rule = rule & Has(f"Act Unlock: {sub['stage']}")
				else:
					rule = True_()
				for req in sub["req"]:
					if req in item_name_to_id.keys():
						rule = rule & Has(req)
					else:
						rule = rule & Has(f"{req} Complete")
				print(rule)
				world.set_rule(enter, rule)
		
		if world.progression_mode == 1:
			world.create_entrance(menu_region, world.get_region("Apotos Hub"), Has(HUB_UNLOCK_APOTOS))
			world.create_entrance(menu_region, world.get_region("Mazuri Hub"), Has(HUB_UNLOCK_MAZURI))
			world.create_entrance(menu_region, world.get_region("Spagonia Hub"), Has(HUB_UNLOCK_SPAGONIA))
			world.create_entrance(menu_region, world.get_region("Holoska Hub"), Has(HUB_UNLOCK_HOLOSKA))
			world.create_entrance(menu_region, world.get_region("Chun-Nan Hub"), Has(HUB_UNLOCK_CHUNNAN))
			world.create_entrance(menu_region, world.get_region("Shamar Hub"), Has(HUB_UNLOCK_SHAMAR))
			world.create_entrance(menu_region, world.get_region("Empire City Hub"), Has(HUB_UNLOCK_EMPIRE_CITY))
			world.create_entrance(menu_region, world.get_region("Adabat Hub"), Has(HUB_UNLOCK_ADABAT))

			egg_req = Has(HUB_UNLOCK_EGGMANLAND) & Has(PROGRESSIVE_SUN_LEVEL, 6) & Has(PROGRESSIVE_MOON_LEVEL, 5)
			for i in item_name_groups["Emeralds"]: egg_req = egg_req & Has(i)
			world.create_entrance(menu_region, world.get_region("Eggmanland Hub"), egg_req)
		world.get_location("Dark Gaia - Completion").place_locked_item(world.create_item(THE_WORLD_REPAIRED))
		menu_region.add_event(
			"Defeat Dark Gaia", THE_WORLD_REPAIRED,
			location_type=SWALocation, item_type=SWAItem,
			rule=CanReachLocation("Dark Gaia - Completion"), show_in_spoiler=False
		)
		world.set_completion_rule(Has(THE_WORLD_REPAIRED))
