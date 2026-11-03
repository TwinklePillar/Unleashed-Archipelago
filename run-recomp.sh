clear
python3 process_apshared.py
cmake --build ./out/build/linux-relwithdebinfo --target UnleashedRecomp
cp "out/build/linux-relwithdebinfo/UnleashedRecomp/UnleashedRecomp" "build"
./build/UnleashedRecomp
