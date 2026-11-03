import json

location_name_to_id = {}
item_name_to_id = {}

with open("apshared.json", "r") as sharedjson:
	shared = json.load(sharedjson)

current_item_id = 1

for item in shared["items"]:
    item["id"] = current_item_id
    item_name_to_id[item["name"]] = current_item_id
    current_item_id += 1

for stage in shared["stages"]:
    for check in stage["checks"]:
        location_name_to_id[f"{stage['name']} - {check['name']}"] = check["id"]

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
