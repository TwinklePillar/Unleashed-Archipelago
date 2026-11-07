// Enter your pattern code here and click on the Play button
// below to execute your code. The results will then be visible
// in the Pattern Data view.
// 
// More information can be found in the documentation.
// 
// 
// Simple example:
// 
// import std.io;
// 
// struct Pattern {
//     u32 int;
//     float floating_point;
// };
// 
// Pattern my_pattern @ 0x00;
// std::print("0x{:08X}", my_pattern.int);

struct StageRecord {
    be u32 stageID;
    be u32 moonBits [[color("007FFF")]];
    be u32 sunBits [[color("FF7F00")]];
    be s32 score;
    be float time;
    be u32 rank;
    padding[40];
};

struct Collectable {
    u8 a;
    u8 b;
};

struct Stat {
    be u32 level;
    be u32 progress;
};

struct SaveData {
    StageRecord stages[512];
    be u32 lives;
    bool stomp;
    bool walljump;
    bool airboost;
    bool lightdash;
    padding[0x773];
    
    be u32 chaosEnergy @ saveBegin + 34268;
    be u32 dayEnemiesKilled @ saveBegin + 34280;
    be u32 moneyRing @ saveBegin + 34216;
    be u32 totalRing @ saveBegin + 34276;
    Stat combat @ saveBegin + 34228;
    Stat strength @ saveBegin + 34236;
    Stat life @ saveBegin + 34244;
    Stat unleash @ saveBegin + 34252;
    Stat shield @ saveBegin + 34260;
    Stat speed @ saveBegin + 34288;
    Stat ringEnergy @ saveBegin + 34296;
    be u32 nightEnemiesKilled @ saveBegin + 34272;
        
    Collectable records[62] @ saveBegin + 33108 [[color("7F0000")]];
    Collectable tapes[59] @ saveBegin + 33374 [[color("7F7F7F")]];
    Collectable books[71] @ saveBegin + 33232 [[color("7F3F00")]];
};

be s32 listFlags[5] @ 0x04;
be s32 intFlags[60] @ 0x18;
be s32 boolFlags[547] @ 0x108;

u32 saveBegin = 0x994;
SaveData save @ saveBegin;