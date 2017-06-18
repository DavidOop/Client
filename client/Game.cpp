#pragma once
#include "Game.h"
#include <iostream>
#include <vector>
#include <Windows.h>

//====================================================================================
//================================ CONSTRUCTOR =======================================
//====================================================================================
Game::Game(const Images &images, const Fonts &fonts, Uint32 image_id, sf::View& view,const sf::String &name)
	:m_me(std::make_unique<MyPlayer>()),
	m_background(images.getImage(BACKGROUND)),
	m_view(view)
{
	//if (m_socket.connect(sf::IpAddress::LocalHost, 5555) != sf::TcpSocket::Done)
	if (m_socket.connect("10.2.16.95", 5555) != sf::TcpSocket::Done)
		std::cout << "no connecting\n";

	sf::Packet packet;
	packet << image_id << name; //����� ���� �� ������ ���
	if (m_socket.send(packet) != sf::TcpSocket::Done)
		std::cout << "no sending image\n";

	receive(images, fonts);//����� ���� �����
	m_me->editText(fonts[SETTINGS], name);
}
//--------------------------------------------------------------------------
void Game::receive(const Images &images, const Fonts &fonts)
{
	sf::Packet packet;
	std::pair <Uint32, sf::Vector2f> temp;
	Uint32 image;
	sf::String name, s = "asd";
	float radius;

	m_socket.setBlocking(true);//**********************************
	Sleep(100);//*********************************

	auto status = m_socket.receive(packet);

	if (status == sf::TcpSocket::Done)
	{
		while (!packet.endOfPacket())//����� �� �� ������ ��� ����
		{
			packet >> temp;

			if (temp.first >= FOOD_LOWER && temp.first <= BOMBS_UPPER)
				m_objectsOnBoard.insert(temp, images);

			else if (temp.first >= PLAYER_LOWER && temp.first <= PLAYER_UPPER)//
			{
				packet >> radius >> image>> name;
				m_players.emplace(temp.first, std::make_unique<OtherPlayers>(temp.first, images[image], fonts[SETTINGS], radius, temp.second, name));
			}
		}
	}

	m_players.erase(temp.first); //����� ������ ��� �������� ������

	m_me->setId(temp.first);//����� ������ ���
	m_me->setTexture(images[image]);
	m_me->setPosition(temp.second);
	m_me->setCenter(temp.second + Vector2f{ NEW_PLAYER,NEW_PLAYER });

	std::cout << std::string(name) << '\n';
}

//====================================================================================
//================================     PLAY     ======================================
//====================================================================================
unsigned Game::play(sf::RenderWindow &w, const Images &images, const Fonts &fonts)
{
	m_socket.setBlocking(false);

	sf::Packet packet;
	sf::Event event;

	while (m_me->getLive())
	{
		w.pollEvent(event);
		auto speed = TimeClass::instance().RestartClock();

		//����� �� �����
		if (m_receive) // �� ��� ��� �� ������ ������ ���
			if (event.type == sf::Event::EventType::KeyPressed)
				updateMove(speed);

		//���� ���� �����
		receiveChanges(images, fonts);

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
void Game::updateMove(float speed)
{
	sf::Packet packet;
	packet.clear();

	if (m_me->legalMove(speed))
	{
		std::vector<Uint32> deleted;
		m_me->collision(deleted, m_objectsOnBoard, m_players, m_me.get());

		if (!m_me->getLive())
			deleted.push_back(m_me->getId()); // �� ����

		//����� ������� ����
		packet << m_me->getId() << m_me->getRadius() << m_me->getPosition() << deleted;

		if (m_socket.send(packet) != sf::TcpSocket::Done)
			std::cout << "no sending data\n";

		if (!m_me->getLive())
			Sleep(100);

		m_receive = false;
	}
}

//====================================================================================
//===========================      RECEIVE DATA      =================================
//====================================================================================
//����� ��� �� ����
void Game::receiveChanges(const Images &images, const Fonts &fonts)
{
	sf::Packet packet;

	m_socket.receive(packet);
	while (!packet.endOfPacket())
	{
		std::pair<Uint32, sf::Vector2f> temp;
		if (!(packet >> temp))
			continue;
		std::vector<Uint32> del;

		if (temp.first >= FOOD_LOWER && temp.first <= BOMBS_UPPER) // ���� �� ����� �����
			m_objectsOnBoard.insert(temp, images);

		else if (temp.first >= PLAYER_LOWER && temp.first <= PLAYER_UPPER)// ����
		{
			if (temp.first == m_me->getId())// ����� ���
				m_receive = true;

			else if (m_players.find(temp.first) != m_players.end())// ����� �� ���� (���� ����..)�
			{
				m_players[temp.first]->setPosition(temp.second);
				m_players[temp.first]->setCenter();
				m_players[temp.first]->collision(del, m_objectsOnBoard, m_players, m_me.get());
			}

			else // ���� ���
				addPlayer(temp, packet, images, fonts);
		}
	}

	deleteDeadPlayer(m_players);
}
//------------------------------------------------------------------------------------
void Game::addPlayer(const std::pair<Uint32, sf::Vector2f> &temp, sf::Packet &packet, const Images &images, const Fonts &fonts)
{
	Uint32 image;
	sf::String name;
	packet >> image>>name;
	m_players.emplace(temp.first, std::make_unique<OtherPlayers>(temp.first, images[image], fonts[SETTINGS], NEW_PLAYER, temp.second,name));
}
//------------------------------------------------------------------------------------
void deleteDeadPlayer(std::unordered_map<Uint32, std::unique_ptr<OtherPlayers>>& players)
{
	for (auto it = players.begin(); it != players.end();)
		(it->second->getLive()) ? it++ : it = players.erase(it);
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
	w.clear();
	//-------------------- ��� ---------------------
	setView(w);

	w.draw(m_background);

	for (auto &x : m_objectsOnBoard)
		w.draw(*x.second.get());

	for (auto &x : m_players)
	{
		w.draw(*x.second.get());
		w.draw(x.second->getName());
	}

	w.draw(*m_me.get());
	w.draw(m_me->getName());
	////------------------- ����� --------------------
	//sf::View scoreView;
	//scoreView.reset(sf::FloatRect{ 0.f, float(SCREEN_HEIGHT / 5 * 4), float(SCREEN_WIDTH), float(SCREEN_HEIGHT) });


	w.display();

}


//*************************************************************************************
//****************************    PLAYER FUNCTION   ***********************************
//*************************************************************************************
// ����� ��� �� ����
void Player::collision(std::vector<Uint32> &deleted, Maps &objectsOnBoard, std::unordered_map<Uint32, std::unique_ptr<OtherPlayers>>& players, Player *me)
{
	checkPlayers(deleted, players, me);
	checkFoodAndBomb(deleted, objectsOnBoard);
}
//--------------------------------------------------------------------------
void Player::checkPlayers(std::vector<Uint32> &deleted, std::unordered_map<Uint32, std::unique_ptr<OtherPlayers>>& players, Player *me)
{
	for (auto &player : players)
	{
		if (player.second->getId() == getId())
			continue;

		if (circlesCollide(player.second.get()))
			if (getRadius() > player.second->getRadius()) //�� ���� ������� ��� ����� ����� ����� ��
			{
				newRadius(player.second.get());
				player.second->setLive(false); //�����, ����� ���� ���� ��
				deleted.push_back(player.first);
			}
			else
				m_live = false; // �� ����� ���� �������� ��
	}


	if (dynamic_cast<OtherPlayers*>(this)) //����� �� ���� ����� ��� ����� ���
	{
		if (circlesCollide(me))
			if (getRadius() > me->getRadius())
			{
				me->setLive(false);
				newRadius(me);
			}
			else
			{
				m_live = false; //����� ����� ��
				me->newRadius(this);
			}
	}

	deleteDeadPlayer(players);
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

	if (getRadius() < NEW_PLAYER)
		m_live = false;
}