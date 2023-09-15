//
// Created by 35691 on 9/15/2023.
//

#ifndef OKWIFI_TOOLS_H
#define OKWIFI_TOOLS_H

#include <string>
#include <cstdint>

namespace ok_wifi {

    /**
     * convert string to uint8
     * @param pContainer
     * @param string
     */
    void stringToUint(uint8_t *pContainer, const std::string &string);

    /**
     * convert uint8 To string
     * @param pData
     * @return
     */
    std::string UintToString(const uint8_t *pData);
}

#endif //OKWIFI_TOOLS_H
