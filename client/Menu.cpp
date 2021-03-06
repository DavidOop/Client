#include "Menu.h"
#include <memory>

Button::~Button() = default;
//========================================================================================
//			Menu functions
//========================================================================================
Menu::Menu() {

	setMenuVector();
	setMenuRec();
}
//========================================================================================
void Menu::setMenuRec() {
	//set menu box
	setFillColor(sf::Color(0, 0, 0, 150));
	setSize(sf::Vector2f{ (*this)[1]->getGlobalBounds().width + float(LEFT_PADDING * 2),float(SCREEN_HEIGHT) });
	dynamic_cast<Help*>((*this)[2].get())->setWidth(getGlobalBounds().width);

}
//========================================================================================

void Menu::setMenuVector() {

	//puts Menus for opnning screen in a vector
	push_back(std::make_unique<Start>());
	push_back(std::make_unique<Settings>(std::ref(dynamic_cast<Start*>(begin()->get())->settings())));
	push_back(std::make_unique<Help>());
	push_back(std::make_unique<Close>());

	m_itToPressed = (--end());

	unsigned topPadding = TOP_PADDING;
	for (auto& it = begin(); it != end(); ++it) {
		(*it)->setPosition(float(LEFT_PADDING), float(topPadding));
		topPadding = (TOP_PADDING + unsigned((*it)->getPosition().y) + FONT_SIZE);
	}
	(*m_itToPressed)->setPosition(float(LEFT_PADDING), float(SCREEN_HEIGHT - (FONT_SIZE + DOWN_PADDING)));
}
//=====================================================================================
// the mouse event: true- mousePressed. false- mouseMoved.
/*
		this loop updates iterator if the use pressed.
*/
void Menu::mouseEventButton(const sf::Vector2f& location, bool event) {
	std::vector<std::unique_ptr<Button>>::iterator button;
	for (button = begin(); button != end(); ++button)
		(event) ? (m_itToPressed = (*button)->pressed(location) ? button : m_itToPressed) : (*button)->onBotton(location);

	//update close class what is displaying
	(*(--button))->setPressed(button != m_itToPressed);
}
//=====================================================================================
//								Start functions
//=====================================================================================

//=====================================================================================
//			`						Close functions
//=====================================================================================
void Close::display(sf::RenderWindow& window)
{
	setString((!m_pressed) ? "Close" : "Back");
	setFillColor((m_switch) ? display2 : display1); window.draw(*this);
}
//=====================================================================================
bool Close::pressed(const sf::Vector2f& location) {
	auto pre = check(location);
	if (pre) {
		if (!m_pressed)//if close
			exit(0);
		else               //if back
			m_pressed = false;
	}
	return pre;
}
