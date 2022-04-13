#include <cstdint>
#include <sys/types.h>
#include <unistd.h>
#include <devctl.h>

namespace bbs {


    struct BBSParams {
        std::uint32_t seed;
        std::uint32_t p;
        std::uint32_t q;
    };


}


#define SET_GEN_PARAMS __DIOT(_DCMD_MISC, 1, std::uint32_t) // Pass information to the device
#define GET_ELEMENT __DIOF(_DCMD_MISC, 2 ,bbs::BBSParams) // Get information from the device
