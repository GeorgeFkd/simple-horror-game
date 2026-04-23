#include "AudioPlayer.h"
#include <iostream>
void AudioPlayer::init() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 4, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << "\n";
    }
}

AudioPlayer::~AudioPlayer() {
    Mix_FreeChunk(footsteps_sound);
    Mix_FreeMusic(horror_music);
}

void AudioPlayer::debugAudio() {
    std::cout << "Checking audio support \n";
    int numDecoders = Mix_GetNumChunkDecoders();
    for (int i = 0; i < numDecoders; i++) {
        std::cout << "Chunk decoder: " << Mix_GetChunkDecoder(i) << "\n";
    }
    int numMusicDecoders = Mix_GetNumMusicDecoders();
    for (int i = 0; i < numMusicDecoders; i++) {
        std::cout << "Music decoder: " << Mix_GetMusicDecoder(i) << "\n";
    }
}

bool AudioPlayer::hasError() {
    return false;
}

bool AudioPlayer::isPlayingMusic() {
    return Mix_PlayingMusic() == 1;
}

AudioPlayer& AudioPlayer::withBackgroundMusic(std::string filepath) {
    horror_music = Mix_LoadMUS(std::move(filepath).c_str());
    if (!horror_music) {
        std::cerr << "Failed to load background music: " << Mix_GetError() << "\n";
    }
    return *this;
}

AudioPlayer& AudioPlayer::withFootstepsSound(std::string filepath) {
    footsteps_sound         = Mix_LoadWAV(std::move(filepath).c_str());
    footsteps_sound_channel = 2;
    if (!footsteps_sound) {
        std::cerr << "Failed to load music for footsteps: " << Mix_GetError() << "\n";
    }
    return *this;
}

void AudioPlayer::setMusicVolume(int volume) {
    Mix_VolumeMusic(volume);
}

void AudioPlayer::startBackgroundMusic() {
    Mix_PlayMusic(horror_music, -1);
}

void AudioPlayer::stopBackgroundMusic() {
    Mix_HaltMusic();
}

void AudioPlayer::startFootstepsSound() {
    Mix_PlayChannel(footsteps_sound_channel, footsteps_sound, -1);
}

void AudioPlayer::stopFootstepsSound() {
    Mix_HaltChannel(footsteps_sound_channel);
}
