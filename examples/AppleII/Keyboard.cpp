#include "Keyboard.h"
#include <App.h>

namespace qe::Examples::AppleII
{

void Keyboard::RunModule(Context ctx)
{
    ctx_ = ctx;
    CreateKeys();
}

void Keyboard::DestroyModule()
{

}

void Keyboard::UpdateModule()
{
    ImGuiIO& io = ImGui::GetIO();
    auto& map = (io.KeyMods == ImGuiMod_Shift) ?    shiftMode_ :
                    (io.KeyMods == ImGuiMod_Ctrl) ?     ctrlMode_ :
                    normalMode_;

    for (auto p : map)
    {
        int hostKey = p.first;
        uint8_t appleKey = p.second;
        if (ImGui::IsKeyPressed( ImGuiKey(hostKey)))
        {
            key_.store(appleKey);
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        breakKey_.store(true);
    }
    return;
}

uint8_t Keyboard::GetKey()
{
    uint8_t newKey = key_.exchange(0);
    return newKey;
}

bool Keyboard::HasBreakKey()
{
    bool hasBreak = breakKey_.exchange(false);
    return hasBreak;
}

void Keyboard::CreateKeys()
{
    {
        // NORMAL MODE
        ////////////////////////////////////////////////////////////////

        for (int key = ImGuiKey_A; key <= ImGuiKey_Z; ++key)
        {
            uint8_t code = (key - ImGuiKey_A) + 65;
            normalMode_[key] = ( code | 0x80 );
        }
        for (int key = ImGuiKey_0; key <= ImGuiKey_9; ++key)
        {
            uint8_t code = (key - ImGuiKey_0) + 48;
            normalMode_[key] = (code | 0x80);
        }

        normalMode_[ImGuiKey_Backspace] =       (  8 | 0x80);
        normalMode_[ImGuiKey_LeftArrow] =       (  8 | 0x80);
        normalMode_[ImGuiKey_RightArrow] =      ( 21 | 0x80);
        normalMode_[ImGuiKey_Enter] =           ( 13 | 0x80);
        normalMode_[ImGuiKey_Space] =           ( 32 | 0x80);
        normalMode_[ImGuiKey_Apostrophe] =      ( 32 | 0x80);
        normalMode_[ImGuiKey_Escape] =          ( 27 | 0x80);
        normalMode_[ImGuiKey_Comma] =           ( 44 | 0x80);
        normalMode_[ImGuiKey_Equal] =           ( 61 | 0x80);
        normalMode_[ImGuiKey_Period] =          ( 46 | 0x80);
        normalMode_[ImGuiKey_Minus] =           ( 45 | 0x80);
        normalMode_[ImGuiKey_LeftBracket] =     ( 91 | 0x80);
        normalMode_[ImGuiKey_RightBracket] =    ( 93 | 0x80);
        normalMode_[ImGuiKey_Slash] =           ( 47 | 0x80);
        normalMode_[ImGuiKey_Backslash] =       ( 92 | 0x80);
        normalMode_[ImGuiKey_Semicolon] =       ( 59 | 0x80);
        normalMode_[ImGuiKey_RightArrow] =      ( 21 | 0x80);
    }

    {
        // SFIFT MODE
        ////////////////////////////////////////////////////////////////

        shiftMode_[ImGuiKey_Equal] =            ( 43 | 0x80);
        shiftMode_[ImGuiKey_Apostrophe] =       ( 34 | 0x80);
        shiftMode_[ImGuiKey_1] =                ( 33 | 0x80);
        shiftMode_[ImGuiKey_2] =                ( 64 | 0x80);
        shiftMode_[ImGuiKey_3] =                ( 35 | 0x80);
        shiftMode_[ImGuiKey_4] =                ( 36 | 0x80);
        shiftMode_[ImGuiKey_5] =                ( 37 | 0x80);
        shiftMode_[ImGuiKey_6] =                ( 94 | 0x80);
        shiftMode_[ImGuiKey_7] =                ( 38 | 0x80);
        shiftMode_[ImGuiKey_8] =                ( 42 | 0x80);
        shiftMode_[ImGuiKey_9] =                ( 40 | 0x80);
        shiftMode_[ImGuiKey_0] =                ( 41 | 0x80);
        shiftMode_[ImGuiKey_Backslash] =        (124 | 0x80);
        shiftMode_[ImGuiKey_Semicolon] =        ( 58 | 0x80);
        shiftMode_[ImGuiKey_Comma] =            ( 60 | 0x80);
        shiftMode_[ImGuiKey_Period] =           ( 62 | 0x80);


    }

    {
        // CTRL MODE
        ////////////////////////////////////////////////////////////////

        for (int key = ImGuiKey_A; key <= ImGuiKey_Z; ++key)
        {
            uint8_t code = (key - ImGuiKey_A) + 1;
            ctrlMode_[key] = ( code | 0x80 );
        }
    }
}

}
