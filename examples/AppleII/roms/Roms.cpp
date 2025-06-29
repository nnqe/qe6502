#include "Roms.h"

namespace qe::Examples::AppleII::Roms
{
    static std::vector<uint8_t> apple_ii_plus_disk_ii_card_5 =
        #include "apple_ii_plus_disk_ii_card_5.hex"
        ;
    static std::vector<uint8_t> apple_ii_plus_video_rom =
        #include "apple_ii_plus_video_rom.hex"
        ;
    ///////////////////////////////////////////////////////////////
    std::vector<uint8_t> Get_AppleII_Plus_DiskII_Card5()
    {
        return apple_ii_plus_disk_ii_card_5;
    }

    std::vector<uint8_t> Get_AppleII_Plus_VideoRom()
    {
        return apple_ii_plus_video_rom;
    }

}
