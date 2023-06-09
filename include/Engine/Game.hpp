#pragma once

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

#include "IUpdatable.hpp"
#include "GameObject.hpp"
#include "AssetManager.hpp"
#include "AudioManager.hpp"
class State;

class Game : public sf::Drawable, IUpdatable
{
public:
	Game(sf::RenderWindow& window, sf::Vector2u size, State* initialState, const float delta);
	~Game();

	virtual void Update(float delta);
	void Add(GameObject* object);

	void HandleEvent(sf::Event evt);

	/// <summary>
	/// <para>Sets a state to switch to at the end of the next update loop.</para>
	/// <para>Doesn't switch the state immediately, as it causes all kinds of problems.</para>
	/// </summary>
	/// <param name="newState">State to switch to.</param>
	void SwitchState(State* newState);

	void AddSubstate(State* newSubstate);
	void PopSubstate();
	void ClearSubstates();

	State* GetState() { return state; }
	sf::Vector2u GetSize() { return size; }
	sf::RenderWindow* GetWindow() { return window; }
	bool IsPaused() { return paused; }
	bool IsShowColliders() { return showColliders; }
	bool IsAllowStep() { return allowStep; }

protected:
	virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const;

	void ActuallySwitchState();

	sf::RenderWindow* window;
	const sf::Vector2u size;

	bool paused = false;
	bool showColliders = false;
	bool allowStep = false;

	AssetManager asset;
	AudioManager audio;

private:
	State* state = nullptr;
	std::vector<State*> substates;
	State* newState = nullptr;
};