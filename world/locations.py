from BaseClasses import Region, Location
from rule_builder.rules import True_, Has, HasAll, CanReachLocation
from .apshared import *
from .items import SWAItem

class SWALocation(Location):
	game = apshared["game"]

class LocationState:
	def __init__(self):
		self.er_regions = []
		self.sanities = ["base"]
		self.regen = False
		self.act_order = {}

	def setup_regions(self, world):
		menu_region = Region("Menu", world.player, world.multiworld)
		world.multiworld.regions.append(menu_region)

		hubs = []
		sphere_zero_stages = []
		all_stages = []
		all_stages_unshuffled = []
		unshuffled_stages = []
		for stage in apshared["stages"]:
			stage_region = Region(stage["name"], world.player, world.multiworld)
			world.multiworld.regions.append(stage_region)
			for check in stage["checks"]:
				check_name = f"{stage['name']} - {check['name']}"
				if check["sanity"] in self.sanities:
					stage_region.add_locations({check_name: location_name_to_id[check_name]}, SWALocation)
					rule = True_()
					for req in check["req"]:
						if req in item_name_to_id.keys():
							rule = rule & Has(req)
						else:
							rule = rule & Has(f"{req} Complete")
					print(rule)
					world.set_rule(world.get_location(check_name), rule)
					if check["name"] == "Completion" and len(check["req"]) == 0:
						sphere_zero_stages.append(stage["name"])
			
			if stage["type"] == "hub":
				hubs.append((stage, stage_region))
				continue
			if stage["type"] == "stage":
				all_stages.append(stage["name"])
				all_stages_unshuffled.append(stage["name"])
			else:
				unshuffled_stages.append(stage["name"])
		
		if not self.regen:
			if world.act_shuffle:
				world.random.shuffle(sphere_zero_stages)
				for i in sphere_zero_stages[:2]:
					all_stages.remove(i)
				world.random.shuffle(all_stages)
				all_stages = sphere_zero_stages[:2] + all_stages

			for stage in apshared["stages"]:
				if stage["name"] in all_stages:
					self.act_order[str(all_stages_unshuffled.index(stage["name"]))] = all_stages.index(stage["name"])
		else:
			for key in self.act_order.keys():
				all_stages[self.act_order[key]] = all_stages_unshuffled[int(key)]
		
		world.spoiler_text += f"Act Order for {world.multiworld.player_name[world.player]}:\n"
		for i in range(len(all_stages)):
			world.spoiler_text += f"\t{all_stages_unshuffled[i]} -> {all_stages[i]}\n"
		
		for stage in unshuffled_stages:
			all_stages.append(stage)
			all_stages_unshuffled.append(stage)
		
		for hub in hubs:
			for sub in hub[0]["sub"]:
				shuffled_stage = all_stages[all_stages_unshuffled.index(sub["stage"])]
				stage_region = world.get_region(shuffled_stage)
				stage_region.add_event(
					f"{sub['stage']} Complete", f"{sub['stage']} Complete",
					location_type=SWALocation, item_type=SWAItem,
					rule=CanReachLocation(f"{shuffled_stage} - Completion"), show_in_spoiler=False
				)
				hub[1].connect(stage_region)
				enter = world.get_entrance(f"{hub[1].name} -> {stage_region.name}")
				if sub["level"] > 1:
					if sub["evil"]: rule = Has(PROGRESSIVE_MOON_LEVEL, sub["level"]-1)
					else: rule = Has(PROGRESSIVE_SUN_LEVEL, sub["level"]-1)
				else:
					rule = True_()
				for req in sub["req"]:
					if req in item_name_to_id.keys():
						rule = rule & Has(req)
					else:
						rule = rule & Has(f"{req} Complete")
				print(rule)
				world.set_rule(enter, rule)
		
		if world.progression_mode == 0:
			world.create_entrance(menu_region, world.get_region("Apotos Hub"), True_())
			world.create_entrance(menu_region, world.get_region("Mazuri Hub"), Has("Windmill Isle Night Act 1 Complete"))
			world.create_entrance(menu_region, world.get_region("Spagonia Hub"), Has("Egg Beetle Complete"))
			world.create_entrance(menu_region, world.get_region("Holoska Hub"), Has("Rooftop Run Night Act 1 Complete"))
			world.create_entrance(menu_region, world.get_region("Chun-Nan Hub"), Has("Cool Edge Day Act 1 Complete"))
			world.create_entrance(menu_region, world.get_region("Shamar Hub"), Has("Dark Moray Complete"))
			world.create_entrance(menu_region, world.get_region("Empire City Hub"), Has("Arid Sands Day Act 1 Complete"))
			world.create_entrance(menu_region, world.get_region("Adabat Hub"), Has("Arid Sands Day Act 1 Complete"))
			world.create_entrance(menu_region, world.get_region("Eggmanland Hub"), Has("Egg Lancer Complete"))
		elif world.progression_mode == 1:
			world.create_entrance(menu_region, world.get_region("Apotos Hub"), Has(HUB_UNLOCK_APOTOS))
			world.create_entrance(menu_region, world.get_region("Mazuri Hub"), Has(HUB_UNLOCK_MAZURI))
			world.create_entrance(menu_region, world.get_region("Spagonia Hub"), Has(HUB_UNLOCK_SPAGONIA))
			world.create_entrance(menu_region, world.get_region("Holoska Hub"), Has(HUB_UNLOCK_HOLOSKA))
			world.create_entrance(menu_region, world.get_region("Chun-Nan Hub"), Has(HUB_UNLOCK_CHUNNAN))
			world.create_entrance(menu_region, world.get_region("Shamar Hub"), Has(HUB_UNLOCK_SHAMAR))
			world.create_entrance(menu_region, world.get_region("Empire City Hub"), Has(HUB_UNLOCK_EMPIRE_CITY))
			world.create_entrance(menu_region, world.get_region("Adabat Hub"), Has(HUB_UNLOCK_ADABAT))
			world.create_entrance(menu_region, world.get_region("Eggmanland Hub"), Has(HUB_UNLOCK_EGGMANLAND))
		
		world.set_rule(world.get_entrance("Eggmanland Hub -> Dark Gaia"),
			HasAll(BLUE_CHAOS_EMERALD, RED_CHAOS_EMERALD, PURPLE_CHAOS_EMERALD, CYAN_CHAOS_EMERALD, GREEN_CHAOS_EMERALD, YELLOW_CHAOS_EMERALD, WHITE_CHAOS_EMERALD)
		)
		world.get_location("Dark Gaia - Completion").place_locked_item(world.create_item(THE_WORLD_REPAIRED))
		menu_region.add_event(
			"Defeat Dark Gaia", THE_WORLD_REPAIRED,
			location_type=SWALocation, item_type=SWAItem,
			rule=CanReachLocation("Dark Gaia - Completion"), show_in_spoiler=False
		)
		world.set_completion_rule(Has(THE_WORLD_REPAIRED))
