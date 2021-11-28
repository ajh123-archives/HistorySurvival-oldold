#pragma once

#ifndef NET_COMMOM_H
#define NET_COMMON_H

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>

#ifndef NET_COMMOM_H_DEFS
#define NET_COMMON_H_DEFS

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOUSER
#define NOGDI
#endif
#define ASIO_STANDALONE
#endif

#include "raylib.h"
#include "raymath.h"
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


enum class CustomMsgTypes : uint32_t
{
	Server_GetStatus,
	Server_GetPing,
	Client_Accepted,
	Client_SetID,
	Client_Register,
	Client_Unregister,
	Game_AddPlayer,
	Game_RemovePlayer,
	Game_UpdatePlayer,
	Plugin_Message
};
struct player {
	uint32_t ID = 0;
	uint32_t selectedEntity = 0;
	Vector3 pos = {0, 0, 0};
	void setID(uint32_t id) {
		ID = id;
	};
};

namespace hsc {
	namespace queues {
		template <typename T>
		class thread_safe_queue{
		public:
			thread_safe_queue() = default;
			thread_safe_queue(const thread_safe_queue<T>&) = delete;
			virtual ~thread_safe_queue() {clear();}

		public:
			const T& front(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.front();
			}

			const T& back(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.back();
			}

			void push_back(const T& item){
				std::scoped_lock lock(muxQueue);
				deqQueue.emplace_back(std::move(item));
				std::unique_lock<std::mutex> ul(muxBlocking);
				cvBlocking.notify_one();
			}

			void push_front(const T& item){
				std::scoped_lock lock(muxQueue);
				deqQueue.emplace_front(std::move(item));
				std::unique_lock<std::mutex> ul(muxBlocking);
				cvBlocking.notify_one();
			}

			bool empty(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.empty();
			}

			size_t count(){
				std::scoped_lock lock(muxQueue);
				return deqQueue.size();
			}

			void clear(){
				std::scoped_lock lock(muxQueue);
				deqQueue.clear();
			}

			void wait()
			{
				while (empty())
				{
					std::unique_lock<std::mutex> ul(muxBlocking);
					cvBlocking.wait(ul);
				}
			}

			T pop_front(){
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.front());
				deqQueue.pop_front();
				return t;
			}

			T pop_back(){
				std::scoped_lock lock(muxQueue);
				auto t = std::move(deqQueue.back());
				deqQueue.pop_back();
				return t;
			}

		protected:
			std::mutex muxQueue;
			std::deque<T> deqQueue;
			std::condition_variable cvBlocking;
			std::mutex muxBlocking;
		};
	}
}



//Foward Declare the server interface
namespace hsc {
	namespace net {
		template <typename T>
		class server_interface;
	}
}
//Foward Declare the server interface



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

				if (owner_type == owner::server) {
					handshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
					handshakeCheck = scramble(handshakeOut);
				}
				else {
					handshakeIn = 0;
					handshakeOut = 0;
				}
			}
			virtual ~connection() {}

			uint32_t getID() const {
				return id;
			}

		public:
			void connectToClient(hsc::net::server_interface<T>* server, uint32_t uid) {
				if (owner_type == owner::server) {
					if (my_socket.is_open()) {
						id = uid;
						connectionEstablished = true;
						writeValidation();
						readValidation(server);
					}
				}
			}

			void connectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
				if (owner_type == owner::client) {
					asio::async_connect(my_socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
							if (!ec) {
								connectionEstablished = true;
								readValidation();
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
				return this->shared_from_this();
			}

			bool isValidated() {
				return validHandshake;
			}

		private:
			//AYSNC- Write Validation
			void writeValidation() {
				asio::async_write(my_socket, asio::buffer(&handshakeOut, sizeof(uint64_t)),
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec) {
							if (owner_type == owner::client) {
								readHeader();
							}
						}
						else {
							std::cout << ec.message() << "Error while writing validation to " << id << std::endl;
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
							connectionEstablished = false;
							std::cout << ec.message() << "Error while writing packet header to " << id << std::endl;
							my_socket.close();
						}
					});
			}

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

			//ASYNC- Read validation
			void readValidation(hsc::net::server_interface<T>* server = nullptr)
			{
				asio::async_read(my_socket, asio::buffer(&handshakeIn, sizeof(uint64_t)),
					[=](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							if (owner_type == owner::server)
							{
								std::shared_ptr<hsc::net::connection<T>> connection = this->getConnectionPtr();
								if (handshakeIn == handshakeCheck)
								{
									std::cout << "Client Validated" << std::endl;
									validHandshake = true;
									server->onClientValidates(connection);
									readHeader();
								}
								else
								{
									std::cout << "Client " << id << "'s validation failed" << std::endl;
									validHandshake = false;
									my_socket.close();
								}
							}
							else
							{
								handshakeOut = scramble(handshakeIn);
								writeValidation();
							}
						}
						else
						{
							std::cout << "Error while reading validation from " << id << std::endl;
							my_socket.close();
						}
					});
			}

			// "Encrypt" data
			uint64_t scramble(uint64_t input)
			{
				uint64_t out = input ^ 0xDEADBEEFC0DECAFE;
				out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
				return out ^ 0xC0DEFACE12345678;
			}


			//Once a full message is received, add it to the incoming queue
			void addMessageToQueue() {
				try {
					if (owner_type == owner::server) {
						messagesIn.push_back({ this->getConnectionPtr(), msgIn });
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

			// Handshake Validation			
			uint64_t handshakeOut = 0;
			uint64_t handshakeIn = 0;
			uint64_t handshakeCheck = 0;

			bool validHandshake = false;
			bool connectionEstablished = false;

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



namespace hsc {
	namespace net {
		template <typename T>
		class client_interface {
		public:
			client_interface() {}
			virtual ~client_interface() {
				disconnect();
			}

		public:
			//Connects to a specified server
			bool connect(const std::string& host, const uint16_t port) {
				try {
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

				}
				catch (std::exception& e) {
					std::cerr << "Exception while connecting to server: " << e.what() << std::endl;
					return false;
				}
				return true;
			}

			//Disconects from the server
			void disconnect() {
				if (isConnected()) {
					connection->disconnect();
				}
				context.stop();
				if (asio_thread.joinable()) {
					asio_thread.join();
				}
				connection.release();
			}

			bool isConnected() {
				if (connection) {
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
			hsc::queues::thread_safe_queue<hsc::net::packets::owned_message<T>>& messagesToUs() {
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



namespace hsc {
	namespace net {
		template <typename T>
		class server_interface {
		public:
			server_interface(uint16_t port, const char* address) :asio_acceptor(context, asio::ip::tcp::endpoint(asio::ip::address::from_string(address), port)) {

			}

			virtual ~server_interface() {
				stop();
			}

			bool start() {
				try {
					acceptClient();
					asio_thread = std::thread([this]() {context.run(); });
				}
				catch (const std::exception& e) {
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

							std::shared_ptr<connection<T>> new_connection =
								std::make_shared<connection<T>>(
									connection<T>::owner::server,
									context,
									std::move(socket),
									messagesIn
									);
							if (onClientConnect(new_connection)) {
								connections.push_back(std::move(new_connection));
								connections.back()->connectToClient(this, idCounter++);
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
			void sendMessage(std::shared_ptr<hsc::net::connection<T>> client, const hsc::net::packets::message<T> msg) {
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
			void sendMessageAll(const hsc::net::packets::message<T> msg, std::shared_ptr<hsc::net::connection<T>> ignoredClient = nullptr) {
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

			void update(size_t maxMessages = -1, bool wait = false) {
				if (wait) messagesIn.wait();
				size_t message_count = 0;
				while (message_count < maxMessages && !messagesIn.empty()) {
					auto msg = messagesIn.pop_front();
					onMessage(msg.remote, msg.msg);
					message_count++;
				}
			}

		public:
			//Called when a client gets validated
			virtual void onClientValidates(std::shared_ptr<hsc::net::connection<T>> client) {

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