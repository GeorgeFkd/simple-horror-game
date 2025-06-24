#pragma once

#include "Camera.h"
#include "Model.h"
#include <chrono>
#include <queue>
#include <random>
#include <vector>

class Monster {
  public:
    Monster(Models::Model* monster_model);
    Monster();
    ~Monster();
    inline void set_model(Models::Model* monster_model){
        this->model_ref = monster_model;
    }

    void move_towards(const glm::vec3& direction, float speed, float dt);

    Monster& add_scripted_movement(const glm::vec3& direction, float speed,
                                   float duration_secs);
    void     clear_scripted_movements();
    bool     no_scripts_left();

    Monster& disappear_probability(float pr);
    Monster& seconds_for_coinflip(float s);
    Monster& restrict_monster_within(float xmin,float xmax,float zmin,float zmax);
    
    void teleport_at(const glm::vec3& world_position);
    void disappear();
    void appear_at(const glm::vec3& world_position);

    void update(float dt,const glm::vec3& player_view_direction,const glm::vec3& player_position);
    
    
    inline void on_monster_active(std::function<void()> fn){
        on_active = fn;
    }
    inline void on_monster_not_active(std::function<void()> fn){
        on_disabled = fn;
    }
    inline void start_chasing_player(){
        monster_chasing_player = true;
    }

    inline void stop_chasing_player() {
        monster_chasing_player = false;
    }

    inline void set_chasing_speed(float sp){
        chase_speed = sp;
    }

    Models::Model* monster_model();

  private:
    struct Scripted {
        glm::vec3            direction;
        float                speed;
        float duration_secs;
        float elapsed_secs;
    };
    float max_x,min_x,max_z,min_z;

    bool monster_chasing_player = false;
    float distance_from_player                          = 10.0f;
    float time_looking_at_it                       = 0.0f;
    float time_not_looking_at_it                   = 0.0f;
    int   seconds_per_coinflip                     = 20;
    float random_disappear_probability             = 0.55f;
    float disappear_probability_when_looking_at_it = 0.75f;
    float seconds_looking_at_it_for_coinflip       = 5.0f;
    float seconds_looking_at_it_for_death          = 7.0f;
    float seconds_not_looking_at_it_for_death      = 10.0f;
    float elapsed_time = 0.0f;
    float chasing_elapsed_time = 0.0f;
    //depending on the camera's speed the user can be caught or not
    float chase_speed = 5.0f;
    Models::Model*                        model_ref;
    bool                                  is_scripted            = false;
    std::queue<Scripted>                 scripts;
    std::random_device               rand_dev;
    std::default_random_engine       el;
    std::uniform_real_distribution<> uniform_rand;
    std::function<void()> on_active;
    std::function<void()> on_disabled;
    std::function<void()> on_looking_death;
    std::function<void()> on_not_looking_death;
    float generate_random_number();
};
