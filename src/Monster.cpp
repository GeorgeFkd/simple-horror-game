#include "Monster.h"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Monster::Monster(Models::Model* monster_model)
    : model_ref(monster_model), el(rand_dev()), uniform_rand(0.0f, 1.0f) {}

float Monster::generate_random_number() {
    return uniform_rand(el);
}

Monster::Monster() {}

Monster::~Monster() {}

Models::Model* Monster::monster_model() {
    return model_ref;
}

bool Monster::no_scripts_left() {
    return scripts.empty();
}

Monster& Monster::add_scripted_movement(const glm::vec3& direction, float speed, float seconds) {
    is_scripted = true;
    scripts.push({direction, speed, seconds, 0});
    return *this;
}

Monster& Monster::should_look_at_it_every(float s){
    seconds_looking_at_it_for_death = s;
    return *this;
}

Monster& Monster::should_not_look_at_it_more_than(float s) {
    seconds_not_looking_at_it_for_death = s;
    return *this;
}

void Monster::clear_scripted_movements() {
    while (!scripts.empty()) {
        scripts.pop();
    }
}

void Monster::move_towards(const glm::vec3& direction, float speed, float dt) {
    glm::mat4       tf          = model_ref->get_local_transform();
    const auto      current_pos = glm::vec3(tf[3]);
    const glm::vec3 target_pos  = current_pos + direction * speed;
    const float     mix_factor  = glm::clamp(dt, 0.0f, 1.0f);
    const glm::vec3 new_pos     = glm::mix(current_pos, target_pos, mix_factor);

    // the extra work here is for the monster to rotate based on where he is going
    const glm::vec3 flat_dir = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
    const float     yaw      = std::atan2(flat_dir.x, flat_dir.z);
    tf                       = glm::translate(glm::mat4(1.0f), new_pos);
    tf                       = glm::rotate(tf, yaw, glm::vec3(0.0f, 1.0f, 0.0f));

    tf[3] = glm::vec4(new_pos, 1.0f);

    std::cout << "Monster moving (via mix)\n";
    model_ref->set_local_transform(tf);
}

void Monster::teleport_at(const glm::vec3& world_position) {
    glm::mat4 tf = glm::translate(glm::mat4(1.0f), world_position);
    model_ref->set_local_transform(tf);
}

void Monster::disappear() {
    model_ref->set_interactivity(false);
    model_ref->disable();
}

void Monster::appear_at(const glm::vec3& world_position) {
    teleport_at(world_position);
    model_ref->enable();
}

Monster& Monster::follow_distance(float dist) {
    distance_from_player = dist;
    return *this;
}

Monster& Monster::seconds_for_coinflip(float secs){
    seconds_per_coinflip = secs;
    return *this;
}

Monster& Monster::disappear_probability(float pr) {
    random_disappear_probability = pr;
    return *this;
}
// Print any glm::vec4
#define PRINT_VEC4(v)                                                                              \
    std::cout << #v " = (" << (v).x << ", " << (v).y << ", " << (v).z << ", " << (v).w << ")"      \
              << std::endl

void Monster::update(float dt, const glm::vec3& player_view_direction,
                     const glm::vec3& player_position) {
    if (!scripts.empty()) {
        auto current_script = &scripts.front();
        if (current_script->elapsed_secs < current_script->duration_secs) {
            PRINT_VEC4(monster_model()->get_local_transform()[3]);
            move_towards(current_script->direction, current_script->speed, dt);
            current_script->elapsed_secs = current_script->elapsed_secs + dt;
        } else {
            scripts.pop();
        }
    }

    elapsed_time = elapsed_time + dt;
    std::cout << "Elapsed time is: " << elapsed_time << "\n";
    if (elapsed_time > seconds_per_coinflip) {
        std::cout << "10 seconds have passed at the monster";
        float randNum = generate_random_number();
        float probs   = 0.0f;
        if (model_ref->is_active()) {
            probs = random_disappear_probability;
        } else {
            probs = 1 - random_disappear_probability;
        }
        if (randNum < probs) {
            // it might be better to do the toggle explicitly
            model_ref->toggle_active();
            if (!model_ref->is_active()) {
                time_looking_at_it     = 0.0f;
                time_not_looking_at_it = 0.0f;
            }
        }
        //after coinflip restart time
        elapsed_time = 0.0f;
    }

    auto monster_pos      = glm::vec3(monster_model()->get_local_transform()[3]);
    auto towards_monster  = glm::normalize(monster_pos - player_position);
    auto monster_view_dir = glm::dot(player_view_direction, towards_monster);
    if (monster_view_dir > 0) {
        std::cout << "You are looking at the monster\n";
        time_looking_at_it += dt;
        time_not_looking_at_it = 0;
    } else {
        std::cout << "You are not looking at the monster\n";
        time_not_looking_at_it += dt;
        time_looking_at_it = 0;
    }
    float rand_num_for_death = generate_random_number();
    if (time_looking_at_it > seconds_looking_at_it_for_death) {
        if (rand_num_for_death < 0.65f)
            return;
        on_looking_death();
        std::cout << "You died because you looked at it too much\n";
        return;
    }

    if (time_not_looking_at_it > seconds_not_looking_at_it_for_death) {
        if (rand_num_for_death < 0.65f)
            return;
        on_not_looking_death();
        std::cout << "You died because you did not keep it in check\n";
        return;
    }
    if (monster_model()->is_active()) {
        on_active();
    } else {
        on_disabled();
    }

    //TODO when looking at the monster, make it disappear with a low probability
}
