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
    //keeps the monster on the floor level
    tf[3][1] = 0.0f;
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



Monster& Monster::restrict_monster_within(float xmin, float xmax, float zmin, float zmax) {
    min_x = xmin;
    max_x = xmax;
    max_z = zmax;
    min_z = zmin;
    return *this;
}

Monster& Monster::seconds_for_coinflip(float secs) {
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

    elapsed_time         = elapsed_time + dt;
    chasing_elapsed_time = chasing_elapsed_time + dt;
    if (chasing_elapsed_time > 15.0f) {
        float rand = generate_random_number();
        if (rand > 0.35f) {
            start_chasing_player();
        } else {
            stop_chasing_player();
        }
        chasing_elapsed_time = 0.0f;
    }
    if (monster_chasing_player) {
        glm::mat4 tf          = model_ref->get_local_transform();
        glm::vec3 monster_pos = glm::vec3(tf[3]);

        glm::vec3 dir = glm::normalize(player_position - monster_pos);
        //can get slightly more than the camera's to ensure a proper chase
        auto speed = chase_speed + uniform_rand(el) *(14.0f - chase_speed);
        move_towards(dir, speed, dt);
    } else {
        if (!scripts.empty()) {
            auto current_script = &scripts.front();
            if (current_script->elapsed_secs < current_script->duration_secs) {
                // PRINT_VEC4(monster_model()->get_local_transform()[3]);
                move_towards(current_script->direction, current_script->speed, dt);
                current_script->elapsed_secs = current_script->elapsed_secs + dt;
            } else {
                scripts.pop();
            }
        } else {

            glm::vec3 pos = glm::vec3(monster_model()->get_local_transform()[3]);

            // pick a random target inside [min_x..max_x]×[min_z..max_z]
            float     u1 = uniform_rand(el);
            float     u2 = uniform_rand(el);
            float     tx = min_x + u1 * (max_x - min_x);
            float     tz = min_z + u2 * (max_z - min_z);
            glm::vec3 target{tx, pos.y, tz};

            glm::vec3 dir   = glm::normalize(target - pos);
            //speed can get slightly higher than camera for difficulty
            float     speed = 5.0f + uniform_rand(el) * (14.0f-5.0f);

            float distance = glm::distance(target, pos);
            float duration = distance / speed;

            add_scripted_movement(dir, speed, duration);
        }
    }
    // std::cout << "Elapsed time is: " << elapsed_time << "\n";
    if (elapsed_time > seconds_per_coinflip) {
        // std::cout << "10 seconds have passed at the monster";
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
        // after coinflip restart time
        elapsed_time = 0.0f;
    }

    if (monster_model()->is_active()) {
        on_active();
    } else {
        on_disabled();
    }
}
