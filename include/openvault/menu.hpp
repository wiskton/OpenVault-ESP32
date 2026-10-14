#pragma once
#include <vector>
#include <cstddef>
namespace openvault {
// Action ids remain stable; availability depends on the persistent cofre, not just RAM.
inline std::vector<std::size_t> walletMenu(bool saved,bool unlocked){
    if(!saved) return {0,1};
    if(!unlocked) return {2};
    return {3,4,5,6,9,7,8};
}
}
