import json

location_name_to_id = {}
item_name_to_id = {}
item_name_groups = {}
location_name_groups = {}

with open("apshared.json", "r") as sharedjson:
	shared = json.load(sharedjson)

hub_base = 400
act_base = 410

for stage in shared["stages"]:
	if stage["type"] == "hub":
		shared["items"].append({"name": f"Hub Unlock: {stage['name'][:-4]}", "item_classif": "progression", "group": "Hubs", "id": hub_base})
		hub_base += 1

	if stage["type"] in ("stage", "unshuffled_stage"):
		shared["items"].append({"name": f"Act Unlock: {stage['name']}", "item_classif": "progression", "group": "Acts", "id": act_base})
		act_base += 1
	
	for check in stage["checks"]:
		check_name = f"{stage['name']} - {check['name']}"
		stage_sanity = f"{stage['name']} {check['sanity']}"
		location_name_to_id[check_name] = check["id"]
		if not stage["name"] in location_name_groups:
			location_name_groups[stage["name"]] = []
		if not check["sanity"] in location_name_groups:
			location_name_groups[check["sanity"]] = []
		if not stage_sanity in location_name_groups:
			location_name_groups[stage_sanity] = []
		location_name_groups[stage["name"]].append(check_name)
		location_name_groups[check["sanity"]].append(check_name)
		location_name_groups[stage_sanity].append(check_name)
		
		if not "evil" in check:
			check["evil"] = -1

current_item_id = 1

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
		appy.write(f"\n{item['name'].replace(' ', '_').replace('.', '').replace('-', '').replace(':', '').upper()} = '{item['name']}'")

with open("UnleashedRecomp/archipelago/apshared.h", "w") as apcpp:
	apcpp.write("#ifndef SWA_AP_SHARED\n#define SWA_AP_SHARED\n\n")
	apcpp.write(f"#define SWA_AP_VERSION {shared['version']}\n")
	apcpp.write(f"#define SWA_AP_MIN_VERSION {shared['min_version']}\n")
	apcpp.write(f'#define SWA_AP_GAME "{shared['game']}"\n\n')
	for item in shared["items"]:
		apcpp.write(f"#define {item['name'].replace(' ', '_').replace('.', '').replace('-', '').replace(':', '').upper()} {item['id']}\n")
	apcpp.write("\nstatic const char* const VANILLA_ACT_ORDER[] = {\n")
	for stage in shared["stages"]:
		if stage["type"] != "hub":
			apcpp.write(f'\t"{stage['name']}",\n')
	apcpp.write("};\n")
	apcpp.write("\nstatic const char* const ITEM_TO_NAME[] = {\n")
	for item in shared["items"]:
		apcpp.write(f"\t[{item['id']}] = \"{item['name']}\",\n")
	apcpp.write("};\n")
	apcpp.write("\n#endif")
