#pragma once
#include <net_common.hpp>
#ifndef NET_CLIENT_H
#define NET_CLIENT_H

namespace hsc{
	namespace net{
		template <typename T>
		class client_interface{
		public:
			client_interface(){}
			virtual ~client_interface(){
				disconnect();
			}

		public:
			//Connects to a specified server
			bool connect(const std::string& host, const uint16_t port){
				try{					
					//Resolve a hostname/ip/domain to allow us to connect
					asio::ip::tcp::resolver resolver(context);
					asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

					connection = std::make_unique<hsc::net::connection<T>>(
						hsc::net::connection<T>::owner::client,
						context,
						asio::ip::tcp::socket(context),
						messagesIn
					);

					//Actually connect
					connection->connectToServer(endpoints);
					asio_thread = std::thread([this]() {context.run(); });

				}catch (std::exception& e){
					std::cerr << "Exception while connecting to server: "<<e.what()<<std::endl;
					return false;
				}
				return true;
			}

			//Disconects from the server
			void disconnect(){
				if (isConnected()) {
					connection->disconnect();
				}
				context.stop();
				if (asio_thread.joinable()) {
					asio_thread.join();
				}
				connection.release();
			}

			bool isConnected(){
				if (connection){
					return connection->isConnected();
				}
				return false;
			}

			// Send message to server
			void send(const hsc::net::packets::message<T>& msg)
			{
				if (isConnected())
					connection->send(msg);
			}

			//Retrive the mesage input queue
			hsc::queues::thread_safe_queue<hsc::net::packets::owned_message<T>>& messagesToUs(){
				return messagesIn;
			}

		protected:
			asio::io_context context; //This will be pased to our connection
			std::thread asio_thread; //This is where asio stuff will ocur
			std::unique_ptr<hsc::net::connection<T>> connection; //Our connection

		private:
			hsc::queues::thread_safe_queue<hsc::net::packets::owned_message<T>> messagesIn; //Messages to our end
		};
	}

}
#endif