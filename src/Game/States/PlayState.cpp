#include "Game/States/PlayState.hpp"

#include "Collision/Collision.hpp"
#include "Engine/Input.hpp"
#include "Engine/G.hpp"
#include "Game/States/LevelSelectState.hpp"
#include "Game/Barkanoid.hpp"
#include "Util/Random.hpp"
#include "Util/VectorUtil.hpp"

PlayState::PlayState(std::string levelName)
	: State(), lives(2), levelName(levelName), ballSpeedIncrement(1.0f), ballSpeedBounds(300.0f, 800.0f),
	  ballSplitAngle((float)PI / 4.0f)
{
}

PlayState::~PlayState()
{
	SetMouseCaptured(false);

	if (level != nullptr)
	{
		delete level;
		level = nullptr;
	}
	if (ui != nullptr)
	{
		delete ui;
		ui = nullptr;
	}
}

void PlayState::Init()
{
	SetMouseCaptured(true);
	sf::Vector2u size = GetGame()->GetSize();
	G::GetAudioManager()->PlayMusic("Funk-Game-Loop.ogg", false);

	std::string levelData = G::GetAssetManager()->GetLevel(levelName);
	if (levelData == "")
	{
		LOG_ERROR("Trying to load level with no data");
		return;
	}
	level = new Level(levelData);

	paddle = new Paddle(sf::Vector2f((float)size.x / 2.0f, (float)size.y - 46.0f));
	gameArea = new GameArea(sf::Vector2f(0.0f, 64.0f), sf::Vector2f((float)size.x, (float)size.y - 64.0f));

	stage = new Stage(sf::Vector2f(16.0f, 104.0f), sf::Vector2f((float)size.x - gameArea->GetWallThickness() * 2.0f, (float)size.y));

	ui = new UI(size);

	Add(paddle);
	Add(gameArea);
	Add(stage);

	paddle->SetHorizontalRange(sf::Vector2f(16.0f, size.x - 16.0f));

	ResetLevel();
}

void PlayState::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	State::draw(target, states);
	target.draw(*ui, states);
}

void PlayState::Update(float delta)
{
	State::Update(delta);

	// Update stage time
	stageTime += sf::milliseconds((sf::Int32)(delta * 1000.0f));

	/*
	Collisions go here
	*/

	// Collide balls
	for (Ball* ball : balls)
	{
		if (ball->IsMoving())
		{
			if (Collide(ball, paddle))		continue;
			if (Collide(ball, gameArea))	continue;
			if (Collide(ball, stage))		continue;
		}
	}

	// Collide all treats and remove ones that have gone offscreen
	Treat* treat = nullptr;
	for (size_t i = 0; i < treats.size();)
	{
		treat = treats[i];
		Collide(treat, gameArea);
		bool hitPaddle = false;
		if (Collide(treat, paddle))
		{
			treat->AddAward(this);
			hitPaddle = true;
		}
		if (hitPaddle || treat->GetPosition().y - 50.0f > GetGame()->GetSize().y)
		{
			Remove(treat);
			delete treat;
		}
		else i++;
	}

	// Check for balls that have gone offscreen
	Ball* ball = nullptr;
	for (size_t i = 0; i < balls.size(); )
	{
		ball = balls[i];
		if (ball->GetPosition().y > GetGame()->GetSize().y + ball->GetCollider()->GetBounds().height)
		{
			Remove(ball);
			delete ball;
		}
		else i++;
	}

	// Increase ball speed
	int stageClockSeconds = (int)stageTime.asSeconds();
	float speedToAdd = 0.0f;
	while (stageClockSeconds > ballSpeedTimer)
	{
		ballSpeedTimer++;
		speedToAdd += ballSpeedIncrement;
	}
	if (speedToAdd > 0.0f)
		AddBallSpeed(speedToAdd);

	// Set UI timer
	ui->SetTime(stageTime);

	// Remove dead GameObjects
	stage->RemoveDead();

	// Check if there are no balls left -> lose a life
	if (balls.size() == 0)
	{
		lives--;
		if (lives < 0)
		{
			GetGame()->AddSubstate(new GameOverSubstate(GetGame()->GetSize(),
				std::bind(&PlayState::BackToLevelSelect, this),
				std::bind(&PlayState::GameOverResetLevel, this)));
			SetMouseCaptured(false);
		}
		else
		{
			G::GetAudioManager()->PlaySound("death.wav");
			ResetLife();
		}
	}

	// Check if the stage has been completed
	if (stage->GetBrickCount() <= 0)
	{
		// Check if a new highscore for the stage was set
		dynamic_cast<Barkanoid*>(G::GetGame())->GetScoreIO().TrySetStageScore(level->GetLevelName(), level->GetStageName(), stagePoints);

		GetGame()->AddSubstate(new StageCompleteSubstate(GetGame()->GetSize(), level->GetStageName(),
			std::bind(&PlayState::StageCompleteClicked, this)));
		SetMouseCaptured(false);
	}

	//Check for pause keypress
	if (Input::JustPressed(sf::Keyboard::Escape) || Input::JustPressed(sf::Keyboard::P))
	{
		GetGame()->AddSubstate(new PauseSubstate(GetGame()->GetSize(),
			std::bind(&PlayState::Unpaused, this),
			std::bind(&PlayState::BackToLevelSelect, this),
			std::bind(&PlayState::GameOverResetLevel, this)));
		SetMouseCaptured(false);
	}

	// Check for skip stage press
	if (Input::JustPressed(sf::Keyboard::N))
		NextStage();
}

void PlayState::Add(GameObject* object)
{
	if (object == nullptr)
	{
		LOG_WARNING("Trying to add nullptr to PlayState");
		return;
	}

	State::Add(object);

	if (dynamic_cast<Treat*>(object))
		treats.push_back(reinterpret_cast<Treat*>(object));
	if (dynamic_cast<Ball*>(object))
		balls.push_back(reinterpret_cast<Ball*>(object));
}

void PlayState::Remove(GameObject* object)
{
	if (object == nullptr)
	{
		LOG_WARNING("Trying to remove nullptr from PlayState");
		return;
	}

	State::Remove(object);

	if (dynamic_cast<Treat*>(object))
		VectorUtil::TryRemove(treats, reinterpret_cast<Treat*>(object));
	if (dynamic_cast<Ball*>(object))
		VectorUtil::TryRemove(balls, reinterpret_cast<Ball*>(object));
}

void PlayState::AddPoints(int points)
{
	stagePoints += points;
	totalPoints += points;
	ui->SetPoints(totalPoints);
}

void PlayState::SetPoints(int points)
{
	stagePoints = points;
	ui->SetPoints(totalPoints);
}

void PlayState::NextStage()
{
	std::string stageName;
	std::vector<std::string> stageData;
	do
	{
		stageName = level->GetNextStage();
		// Return to level select if there are no stages left in the level
		if (stageName == "")
		{
			// Check if a new highscore for the level was set
			dynamic_cast<Barkanoid*>(G::GetGame())->GetScoreIO().TrySetLevelScore(level->GetLevelName(), totalPoints);

			BackToLevelSelect();
			return;
		}

		// Get the stage data
		stageData = G::GetAssetManager()->GetStage(levelName, stageName);
		if (stageData.empty())
			LOG_WARNING("No stage found with name \"" + stageName + "\" or no data found for it");
	}
	// Keep iterating through stage names until a valid one is found
	while (stageData.empty());

	stage->Load(stageData);
	bgColour = stage->GetBGColour();
	SetMouseCaptured(true);

	ClearTreats();
	ClearBalls();
	SetPoints(0);
	stageTime = sf::Time();
	ballSpeedTimer = 0;
	ResetLife();
	ResetBallSpeed();
}

void PlayState::ResetLevel()
{
	lives = 2;
	totalPoints = 0;
	level->Reset();
	paddle->Reset();
	NextStage();
}

void PlayState::ResetLife()
{
	ui->SetLives(lives);

	Ball* ball = new Ball(sf::Vector2f(100.0f, 300.0f));
	ball->SetPosition(paddle->GetPosition().x + (Random::Float(paddle->GetSize().x / 2.0f) - paddle->GetSize().x / 4.0f), 0.0f);
	ball->SetVelocity(ballSpeed);
	Add(ball);

	paddle->Reset();
	paddle->SetMagnetic(false);
	paddle->AttachBall(ball);
}

void PlayState::ClearTreats()
{
	for (size_t i = 0; i < treats.size();)
	{
		if (treats[i] != nullptr)
		{
			Treat* treat = treats[i];
			Remove(treat);
			delete treat;
		}
	}
	treats.clear();
}

void PlayState::ClearBalls()
{
	for (size_t i = 0; i < balls.size();)
	{
		if (balls[i] != nullptr)
		{
			Ball* ball = balls[i];
			Remove(ball);
			delete ball;
		}
	}
	balls.clear();
}

void PlayState::Unpaused()
{
	SetMouseCaptured(true);
}

void PlayState::StageCompleteClicked()
{
	NextStage();
}

void PlayState::BackToLevelSelect()
{
	LevelSelectState* state = new LevelSelectState();
	GetGame()->SwitchState(state);
}

void PlayState::GameOverResetLevel()
{
	ResetLevel();
}

void PlayState::SetMouseCaptured(bool captured)
{
	GetGame()->GetWindow()->setMouseCursorGrabbed(captured);
	GetGame()->GetWindow()->setMouseCursorVisible(!captured);
	Input::SetMouseLocked(captured);
}

void PlayState::AddBallSpeed(float amount)
{
	ballSpeed += amount;
	ballSpeed = Math::clamp(ballSpeed, ballSpeedBounds.x, ballSpeedBounds.y);
	UpdateBallSpeed();
}

void PlayState::UpdateBallSpeed()
{
	for (Ball* ball : balls)
	{
		ball->SetVelocity(ballSpeed);
	}
}

void PlayState::ResetBallSpeed()
{
	ballSpeed = 400.0f;
	UpdateBallSpeed();
}

void PlayState::SplitBalls()
{
	size_t currentBallCount = balls.size();
	for (size_t i = 0; i < currentBallCount; i++)
	{
		Ball* ball = balls[i];
		float ballAngle = ball->GetAngle();
		ball->SetAngle(ballAngle - ballSplitAngle / 2.0f);

		Ball* newBall = new Ball(ball->GetPosition());
		newBall->SetVelocity(ballSpeed);
		newBall->SetAngle(ballAngle + ballSplitAngle / 2.0f);
		Add(newBall);
	}
}
