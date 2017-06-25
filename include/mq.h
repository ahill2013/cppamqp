/**
 * Florida Tech's IGVC program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 * Florida Tech's IGVC program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AMQPTRIAL_MQ_H
#define AMQPTRIAL_MQ_H
#include <amqpcpp.h>    // Copernica
#include <amqpcpp/libev.h>  // Copernica
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <thread>
#include <mutex>
#include <chrono>
#include <assert.h>

void setup_consumer(AMQP::TcpChannel* chan, std::string queue, std::string exchange, std::string key);
void setup_exchange(AmqpClient::Channel::ptr_t conn, std::string exchange, std::string type);
void setup_cop_exchange(AMQP::TcpChannel* chan, std::string exchange, std::string type);
// Send a message to an exchange with a key
void send_message(AmqpClient::Channel::ptr_t conn, std::string message, std::string header,
                  std::string exchange, std::string key);

// Tell subscribers to either unbind or to shut themselves down
void close_message(AmqpClient::Channel::ptr_t conn, std::string message,
                  std::string exchange, std::string key);

/**
 * @brief Potential message headers that should be known. Unicode strings for the sake of sending to Java programs.
 */
struct MessageHeaders {

    // UNICODE versions of headers
    std::string WGENERICNAME = u8"MESSAGE";
    std::string WCLOSE = u8"CLOSE";
    std::string WSTART = u8"START";
    std::string WNAV = u8"NAV";

    std::string WSTATUS = u8"STATUS";
    std::string WGETSTATUS = u8"GETSTATUS";

    std::string WGPSFRAME = u8"GPS";
    std::string WCOMMANDS = u8"COMMANDS";
    std::string WINTERVAL = u8"INTERVAL";
    std::string WLINES = u8"LINES";
    std::string WMBROAD = u8"MOTORBROAD";
    std::string WOBSTACLES = u8"OBSTACLES";


};

/**
 * @brief Possible exchange types, exchange names, and routing keys
 */
struct ExchKeys {
    std::string FANOUT = "fanout";
    std::string TOPIC = "topic";
    std::string DIRECT = "direct";

    std::string GPS = "GPS";
    std::string MC = "MC";
    std::string VISION = "VISION";

    std::string gps_exchange = "gps_exchange";
    std::string gps_key = "gps_key";

    std::string mc_exchange = "mc_exchange";
    std::string mc_key = "mc_key";

    std::string vision_exchange = "vision_exchange";
    std::string vision_key = "vision_key";

    std::string lidar_exchange = "lidar_exchange";
    std::string lidar_key = "lidar_key";

    std::string control_exchange = "control_exchange";
    std::string control_key = "control_key";

    std::string log_exchange = "controllog_exchange";
    std::string log_key = "controllog_key";

    std::string nav_exchange = "nav_exchange";
    std::string nav_key = "nav_key";


    std::string gps_sub = "gps_sub_queue";
    std::string mc_sub = "mc_sub_queue";
    std::string vision_sub = "vision_sub_queue";

    std::map<std::string,std::string> declared = {{gps_exchange, FANOUT}, {mc_exchange, TOPIC},
                                                  {control_exchange, TOPIC}, {nav_exchange, TOPIC},
                                                  {vision_exchange, TOPIC}, {lidar_exchange, TOPIC}};

};

/**
 * @brief Wrapper around an AMQP-CPP connection bound to a LibEv event loop
 */
class MQSub {
public:
    MQSub(struct ev_loop&, std::string& host, std::string& queue);
    ~MQSub();

    AMQP::TcpChannel* getChannel();
    AMQP::TcpConnection* getConnection();

    std::string getQueue();

private:
    AMQP::LibEvHandler* handler;
    AMQP::TcpConnection* conn;
    AMQP::TcpChannel* chan;

    std::string _host;
    std::string _queue;

};

#endif //AMQPTRIAL_MQ_H
