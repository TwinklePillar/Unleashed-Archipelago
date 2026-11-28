import json

location_name_to_id = {}
item_name_to_id = {}
item_name_groups = {}
location_name_groups = {}

with open("apshared.json", "r") as sharedjson:
	shared = json.load(sharedjson)

hub_base = 400
act_base = 430

day_group = "Day Locations"
night_group = "Night Locations"

sanities = {}
sanity_priority = [
	"Completions", "Shoes",
	"Sun Medals", "Mission Sun Medals",
	"Moon Medals", "Mission Moon Medals",
	"Records", "Shop Records",
	"Books", "Shop Books", "Mission Books",
	"Tapes", "Shop Tapes", "Mission Tapes"
]
unimpl_sanities = []

current_item_id = 1
current_location_id = 1

def get_stage_groups(stage):
	if not "type" in stage: return []
	stage_groups = [stage["type"]]
	old_len = 0
	new_len = len(stage_groups)
	while old_len != new_len:
		for group in stage_groups.copy():
			if group in shared["stagegroups"]:
				for meta in shared["stagegroups"][group]:
					if not meta in stage_groups:
						stage_groups.append(meta)
		old_len = new_len
		new_len = len(stage_groups)
	return stage_groups

def add_location_groups(stage, check, check_name):
	time_group = ""
	if "evil" in stage:
		if stage["evil"] == 0: time_group = day_group
		if stage["evil"] == 1: time_group = night_group
	if "evil" in check:
		if check["evil"] == 0: time_group = day_group
		if check["evil"] == 1: time_group = night_group
	check_groups = [stage["name"], check["sanity"], f"{stage['name']} {check['sanity']}"]
	if len(time_group):
		check_groups.append(time_group)
	stage_groups = get_stage_groups(stage)
	for stage_group in stage_groups:
		check_groups.append(stage_group)
		check_groups.append(f"{stage_group} {check['sanity']}")
	if "shop" in check and check["shop"]:
		check_groups += ["Shops"]

	old_len = 0
	new_len = len(check_groups)
	while old_len != new_len:
		for group in check_groups.copy():
			if group in shared["metagroups"]:
				for meta in shared["metagroups"][group]:
					if not meta in check_groups:
						check_groups.append(meta)
						check_groups.append(f"{stage['name']} {meta}")
						for stage_group in stage_groups:
							check_groups.append(f"{stage_group} {meta}")
		old_len = new_len
		new_len = len(check_groups)
	
	for group in check_groups:
		if not group in location_name_groups:
			location_name_groups[group] = []
		location_name_groups[group].append(check_name)

for stage in shared["stages"]:
	if stage["type"] == "Hub" and stage["name"] != "World Map":
		shared["items"].append({"name": f"Hub Unlock: {stage['name'][:-4]}", "item_classif": "progression", "group": "Hubs", "id": hub_base})
		shared["items"].append({"name": f"{stage['name'][:-4]} Dictionary", "item_classif": "progression", "group": "Dictionaries", "id": hub_base+1})
		shared["items"].append({"name": f"{stage['name'][:-4]} Currency", "item_classif": "progression", "group": "Currencies", "id": hub_base+2})
		hub_base += 3

	if "Stages" in get_stage_groups(stage):
		shared["items"].append({"name": f"Act Unlock: {stage['name']}", "item_classif": "progression", "group": "Acts", "id": act_base})
		act_base += 1
	
	for check in stage["checks"].copy():
		check_name = f"{stage['name']} - {check['name']}"
		if check["sanity"] in sanity_priority:
			if not check["sanity"] in sanities:
				sanities[check["sanity"]] = []
			sanities[check["sanity"]].append((check_name, check))
		else:
			stage["checks"].remove(check)
			if not check["sanity"] in unimpl_sanities: unimpl_sanities.append(check["sanity"])
			continue
		add_location_groups(stage, check, check_name)
		
		if not "evil" in check:
			check["evil"] = -1

for souvenir in shared["souvenirs"].values():
	check_name = f"Souvenir - {souvenir['name']}"
	if souvenir["sanity"] in sanity_priority:
		if not souvenir["sanity"] in sanities:
			sanities[souvenir["sanity"]] = []
		sanities[souvenir["sanity"]].append((check_name, souvenir))
	else:
		if not souvenir["sanity"] in unimpl_sanities: unimpl_sanities.append(souvenir["sanity"])
#		stage["checks"].remove(check)
		continue
	add_location_groups({"name": "Souvenir"}, souvenir, check_name)

for sanity in sanity_priority:
	for check in sanities[sanity]:
		check[1]["id"] = current_location_id
		location_name_to_id[check[0]] = current_location_id
		current_location_id += 1

print(unimpl_sanities)

for item in shared["items"]:
	if not "id" in item:
		item["id"] = current_item_id
		current_item_id += 1
	item_name_to_id[item["name"]] = item["id"]
	if "group" in item:
		if not item["group"] in item_name_groups:
			item_name_groups[item["group"]] = []
		item_name_groups[item["group"]].append(item["name"])

with open("world/apshared.py", "w") as appy:
	appy.write("from BaseClasses import ItemClassification\n")
	appy.write("apshared = ")
	shared_str = json.dumps(shared)
	index = shared_str.find("item_classif")
	while True:
		index = shared_str.find("item_classif")
		if index == -1: break
		part = shared_str.partition('"item_classif": "')
		shared_str = "".join([part[0], '"classif": ItemClassification.'])
		part = part[2].partition('"')
		shared_str = "".join([shared_str, part[0], part[2]])
	appy.write(shared_str)
	appy.write(f"\nlocation_name_to_id = ")
	appy.write(json.dumps(location_name_to_id))
	appy.write("\nitem_name_to_id = ")
	appy.write(json.dumps(item_name_to_id))
	appy.write(f"\nlocation_name_groups = ")
	appy.write(json.dumps(location_name_groups))
	appy.write("\nitem_name_groups = ")
	appy.write(json.dumps(item_name_groups))
	for item in shared["items"]:
		if "const_name" in item:
			item_const = item["const_name"]
		else:
			item_const = item['name'].replace(' ', '_').replace('.', '').replace('-', '').replace(':', '').upper()
		appy.write(f"\n{item_const} = '{item['name']}'")

with open("UnleashedRecomp/archipelago/apshared.h", "w") as apcpp:
	apcpp.write("#ifndef SWA_AP_SHARED\n#define SWA_AP_SHARED\n\n")
	apcpp.write("#include \"SWAAP.h\"\n")
	apcpp.write(f"#define SWA_AP_VERSION {shared['version']}\n")
	apcpp.write(f"#define SWA_AP_MIN_VERSION {shared['min_version']}\n")
	apcpp.write(f'#define SWA_AP_GAME "{shared['game']}"\n\n')
	apcpp.write("namespace SWAAP {\n");
	apcpp.write("static APSharedLocation* locations[] = {\n");
	for stage in shared["stages"]:
		if "stage" in stage: stage_id = stage["stage"]
		else: stage_id = 0
		for check in stage["checks"]:
			if "stage" in check: check_stage = check["stage"]
			else: check_stage = stage_id

			if "rank" in check:
				apcpp.write(f"\tnew CompletionLocation({check['id']}, {check_stage}, {check['rank']}),\n")
			if "inventory" in check:
				apcpp.write(f"\tnew InventoryLocation({check['id']}, {check_stage}, {check['inventory']}),\n")
			if "sun" in check:
				apcpp.write(f"\tnew MedalLocation({check['id']}, {check_stage}, {check['sun']}, true),\n")
			if "moon" in check:
				apcpp.write(f"\tnew MedalLocation({check['id']}, {check_stage}, {check['moon']}, false),\n")
			if "shoe" in check:
				apcpp.write(f"\tnew ShoeLocation({check['id']}, {check['shoe']}),\n")
			if "bFlag" in check:
				apcpp.write(f"\tnew BoolFlagLocation({check['id']}, \"{check['bFlag']}\"),\n")
			for key in check.keys():
				if key.startswith("iFlag"):
					apcpp.write(f"\tnew IntFlagLocation({check['id']}, \"{key}\", {check[key]}),\n")
	apcpp.write("};\n\n");
	for item in shared["items"]:
		if "const_name" in item:
			item_const = item["const_name"]
		else:
			item_const = item['name'].replace(' ', '_').replace('.', '').replace('-', '').replace(':', '').replace('+', '').upper()
		apcpp.write(f"#define {item_const} {item['id']}\n")
	apcpp.write("\nstatic const char* const VANILLA_ACT_ORDER[] = {\n")
	for stage in shared["stages"]:
		if stage["type"] != "Hub":
			apcpp.write(f'\t"{stage['name']}",\n')
	apcpp.write("};\n")
	apcpp.write("\nstatic const char* const ITEM_TO_NAME[] = {\n")
	for item in shared["items"]:
		apcpp.write(f"\t[{item['id']}] = \"{item['name']}\",\n")
	apcpp.write("\t[511] = \"__IDS_END__\"\n};\n")
	apcpp.write("} // namespace SWAAP\n");
	apcpp.write("\n#endif")
