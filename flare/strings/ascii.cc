
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/strings/ascii.h"

namespace flare {

const character_properties kCtrlOrSpace = character_properties::eControl | character_properties::eSpace;

const character_properties kPPG = character_properties::ePunct |
                                  character_properties::ePrint | character_properties::eGraph;

const character_properties kDHGP = character_properties::eHexDigit |
                                   character_properties::ePrint | character_properties::eGraph |
                                   character_properties::eDigit;

const character_properties kHAUGP = character_properties::eHexDigit |
                                    character_properties::eAlpha |
                                    character_properties::eUpper |
                                    character_properties::eGraph |
                                    character_properties::ePrint;

const character_properties kHALGP = character_properties::eHexDigit |
                                    character_properties::eAlpha |
                                    character_properties::eLower |
                                    character_properties::eGraph |
                                    character_properties::ePrint;

const character_properties kAUGP = character_properties::eAlpha |
                                   character_properties::eUpper |
                                   character_properties::eGraph |
                                   character_properties::ePrint;

const character_properties kALGP = character_properties::eAlpha |
                                   character_properties::eLower |
                                   character_properties::eGraph |
                                   character_properties::ePrint;

const character_properties ascii::kCharacterProperties[128] = {
        /* 00 . */ character_properties::eControl,
        /* 01 . */ character_properties::eControl,
        /* 02 . */ character_properties::eControl,
        /* 03 . */ character_properties::eControl,
        /* 04 . */ character_properties::eControl,
        /* 05 . */ character_properties::eControl,
        /* 06 . */ character_properties::eControl,
        /* 07 . */ character_properties::eControl,
        /* 08 . */ character_properties::eControl,
        /* 09 . */ kCtrlOrSpace,
        /* 0a . */ kCtrlOrSpace,
        /* 0b . */ kCtrlOrSpace,
        /* 0c . */ kCtrlOrSpace,
        /* 0d . */ kCtrlOrSpace,
        /* 0e . */ character_properties::eControl,
        /* 0f . */ character_properties::eControl,
        /* 10 . */ character_properties::eControl,
        /* 11 . */ character_properties::eControl,
        /* 12 . */ character_properties::eControl,
        /* 13 . */ character_properties::eControl,
        /* 14 . */ character_properties::eControl,
        /* 15 . */ character_properties::eControl,
        /* 16 . */ character_properties::eControl,
        /* 17 . */ character_properties::eControl,
        /* 18 . */ character_properties::eControl,
        /* 19 . */ character_properties::eControl,
        /* 1a . */ character_properties::eControl,
        /* 1b . */ character_properties::eControl,
        /* 1c . */ character_properties::eControl,
        /* 1d . */ character_properties::eControl,
        /* 1e . */ character_properties::eControl,
        /* 1f . */ character_properties::eControl,
        /* 20   */ character_properties::eSpace | character_properties::ePrint,
        /* 21 ! */ kPPG,
        /* 22 " */ kPPG,
        /* 23 # */ kPPG,
        /* 24 $ */ kPPG,
        /* 25 % */ kPPG,
        /* 26 & */ kPPG,
        /* 27 ' */ kPPG,
        /* 28 ( */ kPPG,
        /* 29 ) */ kPPG,
        /* 2a * */ kPPG,
        /* 2b + */ kPPG,
        /* 2c , */ kPPG,
        /* 2d - */ kPPG,
        /* 2e . */ kPPG,
        /* 2f / */ kPPG,
        /* 30 0 */ kDHGP,
        /* 31 1 */ kDHGP,
        /* 32 2 */ kDHGP,
        /* 33 3 */ kDHGP,
        /* 34 4 */ kDHGP,
        /* 35 5 */ kDHGP,
        /* 36 6 */ kDHGP,
        /* 37 7 */ kDHGP,
        /* 38 8 */ kDHGP,
        /* 39 9 */ kDHGP,
        /* 3a : */ kPPG,
        /* 3b ; */ kPPG,
        /* 3c < */ kPPG,
        /* 3d = */ kPPG,
        /* 3e > */ kPPG,
        /* 3f ? */ kPPG,
        /* 40 @ */ kPPG,
        /* 41 A */ kHAUGP,
        /* 42 B */ kHAUGP,
        /* 43 C */ kHAUGP,
        /* 44 D */ kHAUGP,
        /* 45 E */ kHAUGP,
        /* 46 F */ kHAUGP,
        /* 47 G */ kAUGP,
        /* 48 H */ kAUGP,
        /* 49 I */ kAUGP,
        /* 4a J */ kAUGP,
        /* 4b K */ kAUGP,
        /* 4c L */ kAUGP,
        /* 4d M */ kAUGP,
        /* 4e N */ kAUGP,
        /* 4f O */ kAUGP,
        /* 50 P */ kAUGP,
        /* 51 Q */ kAUGP,
        /* 52 R */ kAUGP,
        /* 53 S */ kAUGP,
        /* 54 T */ kAUGP,
        /* 55 U */ kAUGP,
        /* 56 V */ kAUGP,
        /* 57 W */ kAUGP,
        /* 58 X */ kAUGP,
        /* 59 Y */ kAUGP,
        /* 5a Z */ kAUGP,
        /* 5b [ */ kPPG,
        /* 5c \ */ kPPG,
        /* 5d ] */ kPPG,
        /* 5e ^ */ kPPG,
        /* 5f _ */ kPPG,
        /* 60 ` */ kPPG,
        /* 61 a */ kHALGP,
        /* 62 b */ kHALGP,
        /* 63 c */ kHALGP,
        /* 64 d */ kHALGP,
        /* 65 e */ kHALGP,
        /* 66 f */ kHALGP,
        /* 67 g */ kALGP,
        /* 68 h */ kALGP,
        /* 69 i */ kALGP,
        /* 6a j */ kALGP,
        /* 6b k */ kALGP,
        /* 6c l */ kALGP,
        /* 6d m */ kALGP,
        /* 6e n */ kALGP,
        /* 6f o */ kALGP,
        /* 70 p */ kALGP,
        /* 71 q */ kALGP,
        /* 72 r */ kALGP,
        /* 73 s */ kALGP,
        /* 74 t */ kALGP,
        /* 75 u */ kALGP,
        /* 76 v */ kALGP,
        /* 77 w */ kALGP,
        /* 78 x */ kALGP,
        /* 79 y */ kALGP,
        /* 7a z */ kALGP,
        /* 7b { */ kPPG,
        /* 7c | */ kPPG,
        /* 7d } */ kPPG,
        /* 7e ~ */ kPPG,
        /* 7f . */ character_properties::eControl

};

}  // namespace flare
