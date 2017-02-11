//
// Created by armin1215 on 2/11/17.
//

#include "tclap/CmdLine.h"
#include <iostream>

TCLAP::CmdLine cmd("Testing command line", ' ', "0.0");
TCLAP::ValueArg<std::string> ipArg("i","ip", "IP RabbitMQ-Server lives on", false, "amqp://localhost", "string");
TCLAP::SwitchArg motorsArg("m", "motors", "myrio connected", cmd, false);
TCLAP::SwitchArg motionArg("n", "motion", "motion planner reporting", cmd, false);


int main(int argc, char** argv) {

    cmd.add(ipArg);

    cmd.parse(argc, argv);

    std::string ip = ipArg.getValue();
    std::cout << ip << std::endl;

    bool motors = motorsArg.getValue();
    std::cout << motors << std::endl;
}