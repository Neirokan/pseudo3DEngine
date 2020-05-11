//
// Created by Neirokan on 09.05.2020
//

#include "ResourceManager.h"
#include <map>
#include <memory>
#include <iostream>
namespace ResourceManager
{
	namespace
	{
		std::map<std::string, std::shared_ptr<sf::Texture>> _textures;
		std::map<std::string, std::shared_ptr<sf::SoundBuffer>> _soundBuffers;
	}

	void unloadTextures()
	{
		for (auto it = _textures.begin(); it != _textures.end(); it++)
			it->second.reset();
		_textures.clear();
	}

	void unloadSoundBuffers()
	{
		for (auto it = _soundBuffers.begin(); it != _soundBuffers.end(); it++)
			it->second.reset();
		_soundBuffers.clear();
	}

	void unloadAllResources()
	{
		unloadTextures();
		unloadSoundBuffers();
	}

	sf::Texture* loadTexture(const std::string& filename)
	{
		// If texture is already loaded - return pointer to it
		auto it = _textures.find(filename);
		if (it != _textures.end())
			return it->second.get();

		// Otherwise - try to load it. If failure - return zero
		std::shared_ptr<sf::Texture> texture(new sf::Texture);
		if (!texture->loadFromFile(filename))
			return nullptr;

		// If success - remember and return texture pointer
		texture->setRepeated(true);
		_textures.emplace(filename, texture);

		return texture.get();
	}

	sf::SoundBuffer* loadSoundBuffer(const std::string& filename)
	{
		// If texture is already loaded - return pointer to it
		auto it = _soundBuffers.find(filename);
		if (it != _soundBuffers.end())
			return it->second.get();

		// Otherwise - try to load it. If failure - return zero
		std::shared_ptr<sf::SoundBuffer> soundBuffer(new sf::SoundBuffer);
		if (!soundBuffer->loadFromFile(filename))
			return nullptr;

		// If success - remember and return texture pointer
		_soundBuffers.emplace(filename, soundBuffer);

		return soundBuffer.get();
	}
}
