#include <server_main.hpp>
#include <net_common.hpp>
#include <unordered_map>

class CustomServer : public hsc::net::server_interface<CustomMsgTypes>
{
private:
	std::unordered_map<uint32_t, player> players;
public:
	CustomServer(uint16_t port, const char* address) : hsc::net::server_interface<CustomMsgTypes>(port, address)
	{

	}
protected:
	bool onClientConnect(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client) override
	{
		return true;
	}

	void onClientValidates(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client) override
	{
		hsc::net::packets::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::Client_Accepted;
		client->send(msg);
	}

	// Called when a client appears to have disconnected
	void onClientDisconnect(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client) override
	{
		std::cout << "Removing client " << client->getID() << std::endl;
	}

	// Called when a message arrives
	void onMessage(std::shared_ptr<hsc::net::connection<CustomMsgTypes>> client, hsc::net::packets::message<CustomMsgTypes>& msg) override
	{
		switch (msg.header.id)
		{
		case CustomMsgTypes::Client_Register:
		{
			//The client has gave us their player
			player clientPlayer;
			msg >> clientPlayer;
			clientPlayer.setID(client->getID());
			players.insert_or_assign(clientPlayer.ID, clientPlayer);
			std::cout << "Player " << clientPlayer.ID << " is registering" << std::endl;

			//Send the client their ID back
			hsc::net::packets::message<CustomMsgTypes> msg2;
			msg2.header.id = CustomMsgTypes::Client_SetID;
			msg2 << clientPlayer.ID;
			client->send(msg2);
			std::cout << "Send ID to Player " << clientPlayer.ID << std::endl;


			//Send this player to all the other players
			hsc::net::packets::message<CustomMsgTypes> msg3;
			msg3.header.id = CustomMsgTypes::Game_AddPlayer;
			msg3 << clientPlayer;
			sendMessageAll(msg3);
			std::cout << "Player " << clientPlayer.ID << " has been sent to all others" << std::endl;


			//Send this player all the other players
			for (const auto& player : players) {
				hsc::net::packets::message<CustomMsgTypes> msg4;
				msg4.header.id = CustomMsgTypes::Game_AddPlayer;
				msg4 << player.second;
				sendMessage(client, msg4);
			}

			break;
		}
		case CustomMsgTypes::Client_Unregister:
		{
			break;
		}

		case CustomMsgTypes::Game_UpdatePlayer:
		{
			//Send the updated player to all others, apart from where it came
			sendMessageAll(msg, client);
			break;
		}
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