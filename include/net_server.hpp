#pragma once
#include <net_common.hpp>
#ifndef NET_SERVER_H
#define NET_SERVER_H

namespace hsc {
	namespace net {
		template <typename T>
		class server_interface {
		public:
			server_interface(uint16_t port):asio_acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)){

			}

			virtual ~server_interface() {
				stop();
			}

			bool start() {
				try {
					acceptClient();
					asio_thread = std::thread([this]() {context.run(); });
				}catch (const std::exception& e) {
					std::cerr << "Exception while starting server: " << e.what() << std::endl;
					return false;
				}
				context.restart();
				std::cout << "Server started!" << std::endl;
				return true;
			}
			void stop() {
				context.stop();
				if (asio_thread.joinable()) asio_thread.join();
				std::cout << "Server stopped!" << std::endl;
			}

			//AYSNC- Wait for client connection
			void acceptClient() {
				asio_acceptor.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket socket) {
						if (!ec) {
							std::cout << "New connection: " << socket.remote_endpoint() << std::endl;
							
							std::shared_ptr<connection<T>> new_connection(new connection<T>(
								connection<T>::owner::server, context, std::move(socket), messagesIn
							));
							//std::shared_ptr<connection<T>> new_connection = std::make_shared<connection<T>>(connection<T>::owner::server,
							//	context, std::move(socket), messagesIn
							//);

							if (onClientConnect(new_connection)) {
								connections.push_back(std::move(new_connection));
								connections.back()->connectToClient(idCounter++);
								std::cout << "Connection made with ID:" << connections.back()->getID() << std::endl;
							}
							else {
								std::cout << "The new connection was denied" << std::endl;
							}
						}
						else {
							std::cerr << "There was an error while handling a new connection \n"
								<< ec.message() << std::endl;
						}
						acceptClient(); //Call self again to keep context alive
					}
				);
			}

			//Send a message to a client
			void sendMessage(std::shared_ptr<connection<T>> client, const hsc::net::packets::message<T> msg) {
				if (client && client->isConnected()) {
					client->send(msg);
				}
				else {
					onClientDisconnect(client);
					client.reset();
					connections.erase(std::remove(connections.begin(), connections.end(), client), connections.end());
				}
			}

			//Send a message to all clients, and or specify one to ignore
			void sendMessageAll(const hsc::net::packets::message<T> msg, std::shared_ptr<connection<T>> ignoredClient=nullptr) {
				bool isInvalidClient = false;
				for (auto& client : connections) {
					if (client && client->isConnected()) {
						if (client != ignoredClient) {
							client->send(msg);
						}
					}
					else {
						onClientDisconnect(client);
						client.reset();
						isInvalidClient = true;
					}
				}
				if (isInvalidClient) {
					connections.erase(std::remove(connections.begin(), connections.end(), nullptr), connections.end());
				}
			}

			void update(size_t maxMessages = -1){
				size_t message_count = 0;
				while (message_count < maxMessages && !messagesIn.empty()) {
					auto msg = messagesIn.pop_front();
					onMessage(msg.remote, msg.msg);
					message_count++;
				}
			}

		protected:
			//Called when a client joins, return true to accept them
			virtual bool onClientConnect(std::shared_ptr<hsc::net::connection<T>> client) {
				return false;
			}
			//Called when a client disconects
			virtual void onClientDisconnect(std::shared_ptr<hsc::net::connection<T>> client) {
				
			}
			//Called when a message arrives
			virtual void onMessage(std::shared_ptr<hsc::net::connection<T>> client, hsc::net::packets::message<T>& msg) {

			}

			asio::io_context context; //This is shared across all clients
			std::thread asio_thread; //This is where asio stuff will ocur
			asio::ip::tcp::acceptor asio_acceptor; //This accepts our clients

			hsc::queues::thread_safe_queue<hsc::net::packets::owned_message<T>> messagesIn; //Messages to our end
			std::deque<std::shared_ptr<hsc::net::connection<T>>> connections; //This holds all active connections

			uint32_t idCounter = 10000; //All clients will have an ID
		};
	}
}
#endif