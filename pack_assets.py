import os
from shutil import copytree, rmtree

def pack_directory(d):
	for f in os.scandir(path=d):
		if f.is_dir():
			if f.name[0] == "+":
				print(f"Packing {d}/{f.name}")
				os.system(f"HedgeArcPack -T=ar -P {d}/{f.name}")
			else:
				print(f"Searching {d}/{f.name}")
				pack_directory(f"{d}/{f.name}")

def zip_directory(d):
	for f in os.scandir(path=d):
		if f.is_dir():
			if f.name[0] != "+":
				zip_directory(f"{d}/{f.name}")
			else:
				rmtree(f"{d}/{f.name}")
		if f.is_file():
			if not (f.name[0] == "+" or f.name == "mod.ini" or f.name[-4:] == ".sfd"):
				os.remove(f"{d}/{f.name}")

pack_directory("mod")
copytree("mod", "build/SWAAP/Assets", dirs_exist_ok=True)
zip_directory("build/SWAAP/Assets")
