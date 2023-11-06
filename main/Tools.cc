//
// Created by 35691 on 9/15/2023.
//

#include "Tools.h"
#include <cstring>

namespace ok_wifi {
    void stringToUint(uint8_t *pContainer, const std::string &string) {
        auto result = const_cast<uint8_t *>(
                reinterpret_cast<const uint8_t *>(string.c_str()));
        memmove(pContainer, result, string.size());
    }

    std::string UintToString(const uint8_t *pData) {
        const char *string = reinterpret_cast<const char *>(pData);
        std::string result = std::string(string);
        return result;
    }

}
