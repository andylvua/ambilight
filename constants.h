//
// Created by andrew on 1/3/24.
//

#ifndef AMBILIGHT_CONSTANTS_H
#define AMBILIGHT_CONSTANTS_H

namespace Constants {
    namespace Capturer {
        constexpr int BLACK_BAR_DETECTION_COLOR_THRESHOLD = 60;
        constexpr int BLACK_BAR_DETECTION_HEIGHT = 20;
        constexpr int BLACK_BAR_DETECTION_INTERVAL = 1000;

        constexpr int INTERPOLATION_TOLERANCE = 3;
    }

    namespace Sender {
        constexpr int TIMEOUT_SLOTS = 3;
    }

    namespace Serial {
        constexpr int BAUD_RATE = 250000;

        constexpr int INIT_TIMEOUT = 5000;
        constexpr int READ_TIMEOUT = 100;

        constexpr int CHECKSUM_MAGIC = 0x55;
        constexpr auto INIT_MESSAGE = "al_ready";
        constexpr auto RECEIVE_MESSAGE = "rcv";
    }
}


#endif //AMBILIGHT_CONSTANTS_H
