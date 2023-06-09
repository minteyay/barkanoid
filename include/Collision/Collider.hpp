#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>

enum ColliderType
{
	AABB,
	List
};

class GameObject;

class Collider
{
public:
	Collider(GameObject* gameObject)
		: gameObject(gameObject) {};
	virtual ~Collider() = default;
	virtual void Draw(sf::RenderTarget& target, sf::RenderStates states) const {}

	virtual ColliderType GetType() { return ColliderType::AABB; }
	/// <summary>
	/// <para>Gets the bounding box for this collider, in world coordinates.</para>
	/// </summary>
	virtual sf::FloatRect GetBounds();
	GameObject* GetGameObject() const { return gameObject; }
	sf::Vector2f GetPosition() const;
	sf::Vector2f GetOffset() const { return offset; }

	void SetOffset(sf::Vector2f offset) { this->offset = offset; }
	
private:
	GameObject* gameObject;
	sf::Vector2f offset;
};