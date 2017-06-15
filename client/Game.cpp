#pragma once
#include "Game.h"
#include <iostream>
#include <vector>
#include <Windows.h>

//====================================================================================
//================================ CONSTRUCTOR =======================================
//====================================================================================
Game::Game(const Images &images, Uint32 image_id, sf::View& view)
	:m_me(std::make_unique<MyPlayer>()),
	m_background(images.getImage(BACKGROUND)),
	m_view(view)
{
	//if (m_socket.connect(sf::IpAddress::LocalHost, 5555) != sf::TcpSocket::Done)
	if (m_socket.connect("10.2.15.207", 5555) != sf::TcpSocket::Done)
		std::cout << "no connecting\n";

	sf::Packet packet;
	packet << image_id; //����� ���� �� ������ ���
	if (m_socket.send(packet) != sf::TcpSocket::Done)
		std::cout << "no sending image\n";

	receive(images);//����� ���� �����
}
//--------------------------------------------------------------------------
void Game::receive(const Images &images)
{
	sf::Packet packet;
	std::pair <Uint32, sf::Vector2f> temp;
	Uint32 image;
	float radius;

	m_socket.setBlocking(true);//**********************************
	Sleep(100);//*********************************

	auto status = m_socket.receive(packet);
	static int a = 0;

	if (status == sf::TcpSocket::Done)
	{
		while (!packet.endOfPacket())//����� �� �� ������ ��� ����
		{
			packet >> temp;

			if (temp.first >= FOOD_LOWER && temp.first <= BOMBS_UPPER) {
				m_objectsOnBoard.insert(temp, images);
				++a;
				std::cout << a << '\n';
			}

			else if (temp.first >= PLAYER_LOWER && temp.first <= PLAYER_UPPER)//
			{
				packet >> radius >> image;
				m_players.emplace(temp.first, std::make_unique<OtherPlayers>(temp.first, images[image], radius, temp.second));
			}
		}
	}

	m_players.erase(temp.first); //����� ������ ��� �������� ������

	m_me->setId(temp.first);//����� ������ ���
	m_me->setTexture(images[image]);
	m_me->setPosition(temp.second);
	m_me->setCenter(temp.second + Vector2f{ NEW_PLAYER,NEW_PLAYER });
}

//====================================================================================
//================================     PLAY     ======================================
//====================================================================================
unsigned Game::play(sf::RenderWindow &w, const Images &images)
{
	m_socket.setBlocking(false);

	sf::Packet packet;
	sf::Event event;
	while (true)
	{
		w.pollEvent(event);
		auto speed = TimeClass::instance().RestartClock();

		//����� �� �����
		if (m_receive) // �� ��� ��� �� ������ ������ ���
			if (event.type == sf::Event::EventType::KeyPressed)
				if (!updateMove(speed))
					return m_me->getScore();


		//���� ���� �����
		if (!receiveChanges(event, images))
			return m_me->getScore();

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
			return m_me->getScore();

		draw(w);
	}

	return m_me->getScore();
}

//====================================================================================
//===========================      UPDATE MOVE       =================================
//====================================================================================
//����� ��� �� ����
bool Game::updateMove(float speed)
{
	sf::Packet packet;
	packet.clear();
	bool temp = true;

	static int send = 0;

	//������� ����
	std::unique_ptr<MyPlayer> tempMe = std::make_unique<MyPlayer>(*m_me.get());
	if (tempMe->legalMove(speed))
	{
		std::vector<Uint32> deleted;

		temp = tempMe->collision(deleted, m_objectsOnBoard, m_players, tempMe.get());
		if (tempMe->getRadius() < NEW_PLAYER)  // �� ���� �����
			temp = false;

		if (!temp)
			deleted.push_back(tempMe->getId()); // �� ����

		//����� ������� ����
		packet << tempMe->getId() << tempMe->getRadius() << tempMe->getPosition() << deleted;
		//std::cout <<"temp id "<<  tempMe->getId() <<"\n";
		//std::cout << "send " << send << '\n';
		//send++;

		if (m_socket.send(packet) != sf::TcpSocket::Done)
			std::cout << "no sending data\n";

		if (!temp)
			Sleep(100);

		m_me->setRadius(tempMe->getRadius());
		m_me->setPosition(tempMe->getPosition());
		m_me->setCenter();

		m_receive = false;
	}

	return temp;
}

//====================================================================================
//===========================      RECEIVE DATA      =================================
//====================================================================================
//����� ��� �� ����
bool Game::receiveChanges(const sf::Event &event, const Images &images)
{
	sf::Packet packet;

	static int receiv = 0;
	//std::cout << "receive\n";

	if (m_socket.receive(packet) == sf::TcpSocket::Done)
	{
		/*std::cout << "receive " << receive << '\n';
		receive++;*/
	}

	while (!packet.endOfPacket())
	{
		std::pair<Uint32, sf::Vector2f> temp;
		if (!(packet >> temp))
			continue;
		std::vector<Uint32> del;

	//	std::cout << temp.first << std::endl;

		if (temp.first >= FOOD_LOWER && temp.first <= BOMBS_UPPER) // ���� �� ����� �����
		{
			m_objectsOnBoard.insert(temp, images);
		//	std::cout << "receive " << receiv <<" "<<temp.first<< '\n';
				//std::cout << temp.first << std::endl;
			receiv++;
		}
		else if (temp.first >= PLAYER_LOWER && temp.first <= PLAYER_UPPER)// ����
		{
			if (temp.first == m_me->getId())// ����� ���
			{
				m_receive = true;
			}

			else if (m_players.find(temp.first) != m_players.end())// ����� �� ���� (���� ����..)�
			{
				m_players[temp.first]->setPosition(temp.second);
				//m_players[temp.first]->setCenter(m_players[temp.first]->getPosition() + Vector2f{ m_players[temp.first]->getRadius(),m_players[temp.first]->getRadius() });
				m_players[temp.first]->setCenter();
				if (!m_players[temp.first]->collision(del, m_objectsOnBoard, m_players, m_me.get()))
					return false; //�� ����� ��� ����
			}
			else // ���� ���
			{
				addPlayer(temp, packet, images);
				/*Uint32 image;
				packet >> image;
				m_players.emplace(temp.first, std::make_unique<OtherPlayers>(temp.first, images[image], NEW_PLAYER, temp.second));*/
			}
		}
	}
	return true;
}
//------------------------------------------------------------------------------------
void Game::addPlayer(const std::pair<Uint32, sf::Vector2f> &temp, sf::Packet &packet, const Images &images)
{
	Uint32 image;
	packet >> image;
	m_players.emplace(temp.first, std::make_unique<OtherPlayers>(temp.first, images[image], NEW_PLAYER, temp.second));
}
//====================================================================================
//===========================          PRINT         =================================
//====================================================================================
void Game::setView(sf::RenderWindow &w) const
{
	sf::Vector2f pos{ float(SCREEN_WIDTH) / 2 , float(SCREEN_HEIGHT) / 2 };

	if (m_me->getCenter().x > SCREEN_WIDTH / 2)
		if (BOARD_SIZE.x - m_me->getCenter().x < SCREEN_WIDTH / 2)
			pos.x = BOARD_SIZE.x - SCREEN_WIDTH / 2;
		else
			pos.x = m_me->getCenter().x;

	if (m_me->getCenter().y > SCREEN_HEIGHT / 2)
		if (BOARD_SIZE.y - m_me->getCenter().y < SCREEN_HEIGHT / 2)
			pos.y = BOARD_SIZE.y - SCREEN_HEIGHT / 2;
		else
			pos.y = m_me->getCenter().y;

	m_view.setCenter(pos);
	w.setView(m_view);
}
//--------------------------------------------------------------------------
void Game::draw(sf::RenderWindow &w) const
{
	setView(w);

	w.clear();
	w.draw(m_background);

	for (auto &x : m_objectsOnBoard)
		w.draw(*x.second.get());

	for (auto &x : m_players) {
		//std::cout << "players\n";
		//std::cout << x.second->getId() << '\n';
		w.draw(*x.second.get());
	}

	w.draw(*m_me.get());

	w.display();

}


//*************************************************************************************
//****************************    PLAYER FUNCTION   ***********************************
//*************************************************************************************
// ����� ��� �� ����
bool Player::collision(std::vector<Uint32> &deleted, Maps &objectsOnBoard, std::unordered_map<Uint32, std::unique_ptr<OtherPlayers>>& players, Player *me)
{
	checkFoodAndBomb(deleted, objectsOnBoard);
	return checkPlayers(deleted, players, me);
}
//--------------------------------------------------------------------------
bool Player::checkPlayers(std::vector<Uint32> &deleted, std::unordered_map<Uint32, std::unique_ptr<OtherPlayers>>& players, Player *me)
{
	bool temp = true;
	std::vector<Uint32> del;
	for (auto &player : players)
	{
		if (player.second->getId() == getId())
			continue;
		if (circlesCollide(player.second.get()))
			if (getRadius() > player.second->getRadius()) //�� ���� ������� ��� ����� ����� ����� ��
			{
				setScore(Uint32(player.second->getRadius()));
				newRadius(player.second.get());
				del.push_back(player.first); // ����� ����
				deleted.push_back(player.first); // ����� ��� ����
			}
			else
				temp = (dynamic_cast<MyPlayer*>(this)) ? false : true; //�� ������ �� (�� ����� ���, ����� �����)�
	}

	//if (getId() != me->getId()) //����� �� ���� ����� ��� ����� ���
	if (dynamic_cast<OtherPlayers*>(this))
	{
		if (circlesCollide(me))
			if (getRadius() > me->getRadius())
				temp = false;
			else
			{
				del.push_back(getId()); //����� ����
				me->setScore(Uint32(getRadius()));
				me->newRadius(this);
				//deleted.push_back(getId()); // ����� ��� ����
			}
	}

	for (auto pl : del)
		players.erase(pl);

	return temp;
}
//--------------------------------------------------------------------------
void Player::checkFoodAndBomb(std::vector<Uint32> &deleted, Maps &objectsOnBoard)
{
	std::set<Uint32> check = objectsOnBoard.colliding(getCenter(), getRadius());

	for (auto it : check) //����� �� ���� ������ ������ ������
		if (circlesCollide(objectsOnBoard[it].get()))
		{
			newRadius(objectsOnBoard[it].get());
			objectsOnBoard.eraseFromData(it);
			deleted.push_back(it);
		}
}