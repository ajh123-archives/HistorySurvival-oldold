#include <server_main.hpp>
#include <net_server.hpp>

enum class CustomMsgTypes : uint32_t
{
	InvalidMessage,
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

class CustomServer : public hsc::net::server_interface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t port, const char* address) : hsc::net::server_interface<CustomMsgTypes>(port, address)
	{

	}
protected:
protected:
	virtual bool onClientConnect(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client)
	{
		hsc::net::packets::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		client->send(msg);
		return true;
	}

	// Called when a client appears to have disconnected
	virtual void onClientDisconnect(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client)
	{
		std::cout << "Removing client [" << client->getID() << "]\n";
	}

	// Called when a message arrives
	virtual void onMessage(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client, hsc::net::packets::message<CustomMsgTypes>& msg)
	{
		switch (msg.header.id)
		{
		case CustomMsgTypes::ServerPing:
		{
			std::cout << "[" << client->getID() << "]: Server Ping\n";

			// Simply bounce message back to client
			client->send(msg);
		}
		break;

		case CustomMsgTypes::MessageAll:
		{
			std::cout << "[" << client->getID() << "]: Message All\n";

			// Construct a new message and send it to all clients
			hsc::net::packets::message<CustomMsgTypes> msg;
			msg.header.id = CustomMsgTypes::ServerMessage;
			msg << client->getID();
			sendMessageAll(msg, client);

		}
		break;
		}
	}
};

int server_main(std::string bind_to) {
	CustomServer server(36676, bind_to.c_str());
	server.start();

	while (1)
	{
		server.update(-1, true);
	}
	return 0;
}