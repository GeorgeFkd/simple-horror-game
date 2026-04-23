#include <SDL2/SDL_mixer.h>
#include <string>
class AudioPlayer {
  public:
    ~AudioPlayer();
    void         init();
    void         debugAudio();
    bool         hasError();
    bool         isPlayingMusic();
    AudioPlayer& withBackgroundMusic(std::string filepath);
    AudioPlayer& withFootstepsSound(std::string filepath);
    void         setMusicVolume(int volume);
    void         startBackgroundMusic();
    void         stopBackgroundMusic();
    void         startFootstepsSound();
    void         stopFootstepsSound();

  private:
    Mix_Music* horror_music            = nullptr;
    Mix_Chunk* footsteps_sound         = nullptr;
    int        footsteps_sound_channel = -1;
};
