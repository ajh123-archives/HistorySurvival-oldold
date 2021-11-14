#pragma once
#include <net_common.hpp>

namespace hsc{
	namespace net{
		template <typename T>
		class client_interface{
			client_interface(): socket(context){

			}
			virtual ~client_interface(){
				disconnect();
			}

			public:
				//Connects to a specified server
				bool connect(const std::string& host, const uint16_t port){
					try{
						connection = std::make_unique<hsc::net::packets::connection<T>>();
						
					}catch (std::exception& e){
						std::cerr << "Exception while connecting to server: "<<e.what()<<std::endl;
						return false;
					}
					return true;
				}

				//Disconects from the server
				void disconnect(){

				}

				bool isConnected(){
					if(connection){
						return connection->isConnected();
					}
					return false;
				}

				//Retrive the mesage input queue
				hsc::queues::thread_safe_queue<hsc::net::packets::message<T>>& messagesIn(){
					return messagesIn;
				}

			protected:
				asio::io_context context; //This will be pased to our connection
				std::thread asio_thread; //This is where asio stuff will ocur
				asio::ip::tcp::socket socket; //This will be used to make our connection
				std::unique_ptr<hsc::net::packets::connection<T>> connection; //Our connection

			private:
				hsc::queues::thread_safe_queue<hsc::net::packets::message<T>> messagesIn; //Messages to our end
		};
	}

}