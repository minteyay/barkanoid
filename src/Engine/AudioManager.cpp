#include "Engine/AudioManager.hpp"

#include <dirent.h>

#include "Util/MacroDefinitions.hpp"

const std::string AudioManager::musicPath = "res/music/";
const std::string AudioManager::soundPath = "res/sound/";
const size_t AudioManager::maxSoundsPerFile = 8;
const float AudioManager::musicVolume = 40.0f;

AudioManager::AudioManager()
	: mute(false)
{
	music.setLoop(true);
	music.setVolume(musicVolume);

	PreloadSounds();
}

AudioManager::~AudioManager()
{
	for (auto& soundBuffer : soundBuffers)
	{
		if (soundBuffer.second != nullptr)
		{
			delete soundBuffer.second;
			soundBuffer.second = nullptr;
		}
	}
	soundBuffers.clear();

	for (auto& soundPair : sounds)
	{
		for (auto& sound : soundPair.second)
		{
			if (sound != nullptr)
			{
				delete sound;
				sound = nullptr;
			}
		}
		soundPair.second.clear();
	}
}

void AudioManager::PreloadSounds()
{
	DIR* dir = opendir(soundPath.c_str());
	if (dir == NULL)
	{
		LOG_ERROR("Couldn't open " + soundPath);
		return;
	}

	struct dirent* entry = readdir(dir);
	while (entry != NULL)
	{
		if (entry->d_type == DT_REG)
		{
			std::string fileName = entry->d_name;
			if (fileName.find(".wav") != std::string::npos)
			{
				sf::SoundBuffer* soundBuffer = new sf::SoundBuffer();
				if (!soundBuffer->loadFromFile(soundPath + fileName))
				{
					LOG_ERROR("Couldn't load " + fileName + " into a SoundBuffer");
					delete soundBuffer;
					continue;
				}
				soundBuffers.insert(std::make_pair(fileName, soundBuffer));
			}
		}
		entry = readdir(dir);
	}
	closedir(dir);
}

void AudioManager::PlayMusic(const std::string& fileName, bool reset)
{
	if (fileName != currentMusicFileName && !music.openFromFile(musicPath + fileName))
	{
		LOG_ERROR("Couldn't open " + fileName + " for music playback");
		return;
	}
	if (reset || music.getStatus() != sf::Music::Status::Playing)
	{
		music.play();
		currentMusicFileName = fileName;
	}
}

void AudioManager::PlaySound(const std::string& fileName)
{
	if (mute) return;

	auto soundBufferIt = soundBuffers.find(fileName);
	if (soundBufferIt == soundBuffers.end())
	{
		LOG_ERROR("No sound loaded with filename " + fileName);
		return;
	}

	auto soundIt = sounds.find(fileName);
	if (soundIt == sounds.end())
	{
		sf::Sound* sound = new sf::Sound(*(*soundBufferIt).second);
		sound->play();
		auto newSoundPair = std::make_pair(fileName, std::vector<sf::Sound*>());
		newSoundPair.second.push_back(sound);
		sounds.insert(newSoundPair);
	}
	else
	{
		for (auto& sound : (*soundIt).second)
		{
			if (sound->getStatus() != sf::Sound::Status::Playing)
			{
				sound->play();
				return;
			}
		}
		if ((*soundIt).second.size() < maxSoundsPerFile)
		{
			sf::Sound* sound = new sf::Sound(*(*soundBufferIt).second);
			sound->play();
			(*soundIt).second.push_back(sound);
		}
		else
		{
			(*soundIt).second[0]->play();
		}
	}
}

void AudioManager::SetMute(bool mute)
{
	this->mute = mute;

	if (mute)
	{
		music.setVolume(0.0f);
		for (auto& soundPair : sounds)
		{
			for (auto& sound : soundPair.second)
			{
				sound->stop();
			}
		}
	}
	else
	{
		music.setVolume(musicVolume);
	}
}

void AudioManager::ToggleMute()
{
	SetMute(!mute);
}
