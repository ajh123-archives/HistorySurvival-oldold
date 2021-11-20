#pragma once
#include <net_common.hpp>

#ifndef NET_MESSAGE_H
#define NET_MESSAGE_H

namespace hsc {
	namespace net {
		namespace packets {
			//Message header this is sent at the start of every message. The  
			//templates allows us to use `enum class` to make sure all
			//messages are valid at compile time.
			template <typename T>
			struct message_header
			{
				T id{};
				uint32_t size = 0;
			};

			//Messages contain a body made of bytes and a header at the
			//start, they can be serialised and deserialsised. by using
			//special functions.
			template <typename T>
			struct message
			{
				hsc::net::packets::message_header<T> header;
				std::vector<uint8_t> body;

				//Return the size of the whole packet in bytes
				size_t size()  const {
					return sizeof(hsc::net::packets::message_header<T>) + body.size();
				}

				//Overide for bit sift left `<<`, allows us to use `std::cout`
				//for message debuging - as an example.
				friend std::ostream& operator << (std::ostream& os, const hsc::net::packets::message<T>& msg) {
					os << "id:" << int(msg.header.id) << "\nsize:" << msg.header.size;
					return os;
				}

				//Overide for bit sift left `<<`, allows us to use our
				//message to push on to, like a stack.
				template <typename DataType>
				friend message<T>& operator << (hsc::net::packets::message<T>& msg, const DataType& data) {
					//Check that data is 'copyable'
					static_assert(std::is_standard_layout<DataType>::value, "Data is to complex to push!");

					//Cache the current size
					size_t size = msg.body.size();
					//Resize the vector so data can be pushed
					msg.body.resize(msg.body.size() + sizeof(DataType));
					//Copy the new data into the free space
					std::memcpy(msg.body.data() + size, &data, sizeof(DataType));
					//Update the size in message's header
					msg.header.size = msg.size();

					//Return the message so we can chain pushes
					return msg;
				}

				//Overide for bit sift left `>>`, allows us to use our
				//message to pop off, like a stack.
				template <typename DataType>
				friend hsc::net::packets::message<T>& operator >> (hsc::net::packets::message<T>& msg, DataType& data) {
					//Check that data is 'copyable'
					static_assert(std::is_standard_layout<DataType>::value, "Data is to complex to poped!");

					//Cache the current size, to where data will be poped from
					size_t size = msg.body.size() - sizeof(DataType);
					//Copy the poped data into the data
					std::memcpy(&data, msg.body.data() + size, sizeof(DataType));
					//Update the size in message's header
					msg.body.resize(size);
					msg.header.size = msg.size();

					//Return the message so we can chain pushes
					return msg;
				}
			};
			//Just the same as a normal message but contains a pointer
			//to the remote connection.
			template <typename T>
			struct owned_message;
			//This is a forward decleare
		}

		//Used to represent a connection to a client or server
		template <typename T>
		class connection : public std::enable_shared_from_this<connection<T>> {
		public:
			//A connection is "owned" by either a server or a client, and its
			//behaviour is slightly different bewteen the two.
			enum class owner
			{
				server,
				client
			};

			friend std::ostream& operator << (std::ostream& os, const owner& my_owner) {
				if (my_owner == owner::server) {
					os << "owner_type:[server]";
				}
				if (my_owner == owner::client) {
					os << "owner_type:[client]";
				}
				return os;
			}

			connection(owner parent, asio::io_context& context, asio::ip::tcp::socket sock, hsc::queues::thread_safe_queue<hsc::net::packets::owned_message<T>>& messages_in) :
				asioContext(context), my_socket(std::move(sock)), messagesIn(messages_in) {
				owner_type = parent;
			}
			virtual ~connection() {}

			uint32_t getID() const {
				return id;
			}

		public:
			void connectToClient(uint32_t uid = 0) {
				if (owner_type == owner::server) {
					if (my_socket.is_open()) {
						id = uid;
						readHeader();
					}
				}
			}

			void connectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
				if (owner_type == owner::client) {
					asio::async_connect(my_socket, endpoints, 
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
							if (!ec) {
								readHeader();
							}
						});
				}
			}

			void disconnect() {
				if (isConnected()) {
					asio::post(asioContext, [this]() {my_socket.close(); });
				}
			}
			bool isConnected() const {
				return my_socket.is_open();
			}
			void send(const hsc::net::packets::message<T>& msg) {
				asio::post(asioContext,
					[this, msg]()
					{
						bool writingMessages = !messagesOut.empty();
						messagesOut.push_back(msg);
						if (!writingMessages) {
							writeHeader();
						}
					});

			}

			std::shared_ptr<connection<T>> getConnectionPtr() {
				return shared_from_this();
			}

		private:
			//AYSNC- Read message headers
			void readHeader() {
				asio::async_read(my_socket, asio::buffer(&msgIn.header, sizeof(hsc::net::packets::message_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec) {
							if (msgIn.header.size > 0) {
								msgIn.body.resize(msgIn.header.size);
								readBody();
							}
							else {
								addMessageToQueue();
							}
						}
						else {
							std::cerr << ec.message() << "Error while reading packet header from " << id << std::endl;
							my_socket.close();
						}
					});
			}
			//AYSNC- Read message bodys
			void readBody() {
				asio::async_read(my_socket, asio::buffer(msgIn.body.data(), msgIn.body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec) {
							addMessageToQueue();
						}
						else {
							std::cerr << ec.message() << "Error while reading packet body from " << id << std::endl;
							my_socket.close();
						}
					});
			}


			//AYSNC- Write message headers
			void writeHeader() {
				asio::async_write(my_socket, asio::buffer(&messagesOut.front().header, sizeof(hsc::net::packets::message_header<T>)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec) {
							if (messagesOut.front().body.size() > 0) {
								writeBody();
							}
							else {
								messagesOut.pop_front();

								if (!messagesOut.empty()) {
									writeHeader();
								}
							}
						}
						else {
							std::cout << ec.message() << "Error while writing packet header to " << id << std::endl;
							my_socket.close();
						}
					});
			}
			//AYSNC- Write message bodys
			void writeBody() {
				asio::async_write(my_socket, asio::buffer(messagesOut.front().body.data(), messagesOut.front().body.size()),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec) {
							messagesOut.pop_front();

							if (!messagesOut.empty()) {
								writeHeader();
							}
						}
						else {
							std::cout << ec.message() << "Error while writing packet header to " << id << std::endl;
							my_socket.close();
						}
					});
			}

			//Once a full message is received, add it to the incoming queue
			void addMessageToQueue() {
				try {
					if (owner_type == owner::server) {
						messagesIn.push_back({ this->getConnectionPtr(), msgIn});
					}
					else {
						messagesIn.push_back({ nullptr, msgIn });
					}
					readHeader();
				}
				catch (std::exception e) {
					std::cerr << e.what() << std::endl;
				}
			}
		protected:
			asio::ip::tcp::socket my_socket; //This socket points to the remote end
			asio::io_context& asioContext; //There should be one shared one.
			hsc::queues::thread_safe_queue<hsc::net::packets::message<T>> messagesOut; //Messages to remote end
			hsc::queues::thread_safe_queue<hsc::net::packets::owned_message<T>>& messagesIn; //Messages to our end
			hsc::net::packets::message<T> msgIn; //Temporary message holder 
			owner owner_type = owner::server; //The "owner" decides how the connection behaves
			uint32_t id = 0; //Or ID
		};

		namespace packets {
			//Just the same as a normal message but contains a pointer
			//to the remote connection.
			template <typename T>
			struct owned_message {
				std::shared_ptr<hsc::net::connection<T>> remote = nullptr;
				message<T> msg;

				//Overide for bit sift left `<<`, allows us to use `std::cout`
				//for message debuging - as an example.
				friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg) {
					os << msg.msg;
					return os;
				}
			};
		}
	}
}

#endif