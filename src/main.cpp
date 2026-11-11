// bad_item_id_fix: Kyubu port of coremaze's Cube-World-Bad-Item-ID-Crash-Fix.
//
// A dropped item with an unknown id makes the game look up a null sprite and crash. At 0xB229B the game does
//     mov ecx, eax / mov [esp+0x14], eax
// with EAX = the sprite. The stub tests EAX: if it is null it jumps to the game's own "invalid" path (0xB2C50),
// otherwise it runs the two original instructions and continues at 0xB22A1.
// The stub must be able to *choose where execution continues*, which a Kyubu mid hook cannot do, so the original
// jmp-stub is kept and installed through Kyubu's patch_bytes (conflicts are detected, unloading restores the bytes).
#include "../../common.hpp"

extern "C" {
unsigned int invalid_JMP_back;
unsigned int valid_JMP_back;
}

void __declspec(naked) __declspec(noinline) CheckNullASM() {
    asm("test eax, eax");   // is the sprite a null pointer?
    asm("jz 0f");

    asm("mov ecx, eax");    // original code
    asm("mov [esp+0x14], eax");
    asm("jmp [_valid_JMP_back]");

    asm("0:");              // a conditional jump cannot reach the target through memory: use a jmp
    asm("jmp [_invalid_JMP_back]");
}

struct BadItemIdFix {
    explicit BadItemIdFix(kyubu::Mod& m) {
        const uintptr_t site = classic::at(m, 0xB229B);
        static const uint8_t want[] = {0x8B, 0xC8, 0x89, 0x44, 0x24, 0x14, 0xE8};
        if (!classic::expect(m, site, want, sizeof want, "bad_item_id_fix")) return;
        invalid_JMP_back = static_cast<unsigned>(classic::at(m, 0xB2C50));
        valid_JMP_back = static_cast<unsigned>(site + 6);
        if (!classic::write_jmp(m, site, reinterpret_cast<const void*>(&CheckNullASM), "bad_item_id_fix")) return;
        m.info("bad_item_id_fix: null sprite check installed");
    }
};

KYUBU_MOD("bad_item_id_fix", KY_SIDE_CLIENT, BadItemIdFix)
