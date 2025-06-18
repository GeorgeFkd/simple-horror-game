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

    Monster& follow_distance(float dist);
    // Monster& follow_speed(float speed);
    Monster& disappear_probability(float pr);
    // Monster& seconds_per_coinflip(float s);

    void teleport_at(const glm::vec3& world_position);
    void disappear();
    void appear_at(const glm::vec3& world_position);

    void update(float dt,const glm::vec3& player_view_direction,const glm::vec3& player_position);

    void on_death_by_not_looking(std::function<void()> fn);
    void on_death_by_looking(std::function<void()> fn);
    
    inline void on_monster_active(std::function<void()> fn){
        on_active = fn;
    }
    inline void on_monster_not_active(std::function<void()> fn){
        on_disabled = fn;
    }

    Models::Model* monster_model();

  private:
    struct Scripted {
        glm::vec3            direction;
        float                speed;
        float duration_secs;
        float elapsed_secs;
    };

    float distance_from_player                          = 10.0f;
    // those will be inside the class 
    float time_looking_at_it                       = 0.0f;
    float time_not_looking_at_it                   = 0.0f;
    int   seconds_per_coinflip                     = 20;
    float random_dissapear_probability             = 0.55f;
    float dissapear_probability_when_looking_at_it = 0.75f;
    float seconds_to_look_at_it_for_coinflip       = 5.0f;
    float seconds_looking_at_it_for_death          = 7.0f;
    float seconds_not_looking_at_it_for_death      = 10.0f;
    float elapsed_time = 0.0f;
    Models::Model*                        model_ref;
    bool                                  is_scripted            = false;
    std::queue<Scripted>                 scripts;
    std::random_device               rand_dev;
    std::default_random_engine       el;
    std::uniform_real_distribution<> uniform_rand;
    std::function<void()> on_active;
    std::function<void()> on_disabled;
    float generate_random_number();
};
