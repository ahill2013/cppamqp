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

#include "../include/mq.h"

// Possible message headers
MessageHeaders messageHeaders;

/**
 * @brief Using a Copernica channel bind a consumer to a queue and bind that queue to an exchange key combination
 * @param chan Copernica channel created from a TCPConnection class
 * @param queue name of the queue that RabbitMQ should be bound to
 * @param exchange name of the exchange which should forward messages to the queue
 * @param key routing key to further specify that only messages with this key from this exchange should be forwarded
 */
void setup_consumer(AMQP::TcpChannel* chan, std::string queue, std::string exchange, std::string key) {
    chan->declareQueue(queue);  // Declare a queue
    chan->bindQueue(exchange, queue, key);  // Bind a queue to an exchange so that it gets messages posted on exchange
}

/**
 * @brief Using a SimpleAmqpClient channel make sure that an exchange has been declared of the expected type
 * @param conn smart pointer to the SimpleAmqpClient channel
 * @param exchange name of the exchange to declare
 * @param type type of the exchange to declare (broadcast, topic, etc.)
 */
void setup_exchange(AmqpClient::Channel::ptr_t conn, std::string exchange, std::string type) {
    conn->DeclareExchange(exchange, type, false, false, false);
}

/**
 * @brief Using a Copernica channel make sure that an exchange has been declared
 * @param chan name of the channel to use
 * @param exchange name of the exchange to declare
 * @param type type of the exchange to declare (broadcast, topic, etc.)
 */
void setup_cop_exchange(AMQP::TcpChannel* chan, std::string exchange, std::string type) {

    if (type == "fanout") {
        chan->declareExchange(exchange, AMQP::fanout);  // Make sure exchange exists
    } else if (type == "topic") {
        chan->declareExchange(exchange, AMQP::topic);
    }
}

/**
 * @brief Send a message using a SimpleAmqpClient that the queue is closing
 * @deprecated
 * @param conn SimpleAmqpClient channel
 * @param message representation of message as string
 * @param exchange name of the exchange
 * @param key name of the key
 */
void close_message(AmqpClient::Channel::ptr_t conn, std::string message,
                       std::string exchange, std::string key) {
    AmqpClient::BasicMessage::ptr_t b_message = AmqpClient::BasicMessage::Create(message);

    std::map<std::string,AmqpClient::TableValue> header_table = {{messageHeaders.WGENERICNAME, AmqpClient::TableValue(messageHeaders.WCLOSE)}};
    b_message->HeaderTable(header_table);

    conn->BasicPublish(exchange, key, b_message, false, false);
}

/**
 * @brief Using a SimpleAmqpClient channel send a message with an exchange and routing key
 * @deprecated
 * @param conn SimpleAmqpClient channel
 * @param message message as a string
 * @param header header naming the type of the message
 * @param exchange name of the exchange to send the message to
 * @param key routing key so that only people listening on the exchange with this key will be used
 */
void send_message(AmqpClient::Channel::ptr_t conn, std::string message, std::string header,
                  std::string exchange, std::string key) {

    // Option 2 without headers I want
    AmqpClient::BasicMessage::ptr_t b_message = AmqpClient::BasicMessage::Create(message);

    std::map<std::string,AmqpClient::TableValue> header_table = {{messageHeaders.WGENERICNAME, AmqpClient::TableValue(header)}};
    b_message->HeaderTable(header_table);

    conn->BasicPublish(exchange, key, b_message, false, false);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/**
 * @brief Create a wrapper around a Copernica AMQP connection
 * @param loop event loop to bind TcpConnection to
 * @param host name of the host to look for RabbitMQ-Server at
 * @param queue name of the queue to be saved
 */
MQSub::MQSub(struct ev_loop& loop, std::string& host, std::string &queue) { // : handler(&loop) {
    handler = new AMQP::LibEvHandler(&loop);
    conn = new AMQP::TcpConnection(handler, AMQP::Address(host));
    chan = new AMQP::TcpChannel(conn);

    _host = host;
    _queue = queue;

}


/**
 * Clean up the connection by closing the channel and connection then deleting objects
 */
MQSub::~MQSub() {
    chan->close();
    conn->close();
    delete chan;
    delete conn;
    delete handler;
}

AMQP::TcpChannel* MQSub::getChannel() {
    return this->chan;
}

AMQP::TcpConnection* MQSub::getConnection() {
    return this->conn;
}

std::string MQSub::getQueue() {
    return _queue;
}