#include "coach.h"
#include "core/constants.h"
#include "entities/ball.h"
#include "entities/team.h"
#include "game/scene.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// Set to false to let the other team use their own logic (if you implement it)
// Set to true to test your logic on both teams
bool coach_both_teams = true;

/* -------------------------------------------------------------------------
 * Logic Functions
 *  TODO 1: You must implement the following functions in Phase 2.
 *        Each player in each team has its own functions.
 *        You can add new functions, but are NOT ALLOWED to remove
 *        the existing functions or change their structure.
 * ------------------------------------------------------------------------- 
 * ⚠️ STUDENT RULES FOR PHASE 2:
 * You are restricted to modifying ONLY specific variables in each function:
 *
 * 1. MOVEMENT FUNCTIONS (movement_logic_X_Y):
 * Allowed: player->velocity
 * Goal:    Determine the direction and speed of movement.
 *
 * 2. SHOOTING FUNCTIONS (shooting_logic_X_Y):
 * Allowed: ball->velocity
 * Goal:    Determine the direction and power of the kick/pass.
 *
 * 3. CHANGE STATE FUNCTIONS (change_state_logic_X_Y):
 * Allowed: player->state
 * Goal:    Switch between IDLE, MOVING, SHOOTING, or INTERCEPTING.
 *
 * NOTE: Directly modifying any other attributes will be flagged as a violation.
 * Thank you for your attention to this matter!
 * ------------------------------------------------------------------------- */

//*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_**_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*
//توابع کومکی اضافه:

    //مسافت بین دو نقطه
double DISTANCE(double x1, double y1, double x2, double y2){
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

    // حرکت به سمت ۱ نقطه
void move_to_target(struct Player *self, double target_x, double target_y) {
    // محاسبه فاصله بین مکان فعلی بازیکن و هدف
    double dx = target_x - self->position.x;
    double dy = target_y - self->position.y;
    double dist = DISTANCE(self->position.x, self->position.y, target_x, target_y);
    
    // حداکثر سرعت مجاز بر اساس agility
    float max_speed = ((float)self->talents.agility / 10.0f) * MAX_PLAYER_VELOCITY;
    
    if (dist > 0.5f) {
        float current_speed = max_speed;
        // ترمز نرم: اگر فاصله از سرعت کمتر بود، سرعت را معادل فاصله قرار بده تا دقیقاً روی هدف بایستد و نلرزد
        if (dist < max_speed) {
            current_speed = dist; 
        }
        // نرمال‌سازی بردار و ضرب در سرعت
        self->velocity.x = (dx / dist) * max_speed;
        self->velocity.y = (dy / dist) * max_speed;
    } else { // بایستد
        self->velocity.x = 0;
        self->velocity.y = 0;
    }
}

    // آینه کردن بازیکنان
static double get_dynamic_home_x(struct Scene *scene, int team_id, double relative_x){
    if (team_id == 1) {
        // تیم 1 از سمت چپ (X=0) شروع می‌کند
        return scene->field.width * relative_x;
    } else {
        // تیم 2 از سمت راست (X=width) شروع می‌کند، پس باید قرینه شود
        return scene->field.width * (1.0 - relative_x);
    }
}
//*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_**_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*

//*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_**_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*
static void movement_general_helper(struct Player *self, struct Scene *scene, double home_x, double home_y) {
        
    // اگه توپ بیرون رفت برگردندارایش تیمی را حفظ میکنیم
    if (scene->ball->position.x < 0.0 || scene->ball->position.x > scene->field.width || 
        scene->ball->position.y < 0.0 || scene->ball->position.y > scene->field.height) {
        move_to_target(self, home_x, home_y);
        return;
    }

    // اگر بازیکن در حالت توقف است یا در حال شوت زدن/پرتاب اوت است، نباید حرکت کند
    if (self->state == IDLE || self->state == SHOOTING) {
        self->velocity.x = 0.0f;
        self->velocity.y = 0.0f;
        return;
    }
     double target_x = home_x;
     double target_y = home_y;
     bool teammate_has_ball = (scene->ball->possessor != NULL && scene->ball->possessor->team == self->team);
    // بازیکنی که توپ داره چطور به سمت دروازه حریف حرکت کنه
    if (scene->ball->possessor == self) {
    target_x = (self->team == 1) ? scene->field.width : 0.0;
    target_y = self->position.y;

        // اوت نشدن از بالا و پایین زمین
        double margin_y = 20.0; 
        if (target_y < margin_y) target_y = margin_y   ;
        if (target_y > scene->field.height - margin_y  ) target_y = scene->field.height - margin_y   ;

        double dist_to_end = fabs(self->position.x - target_x); //فاصله بازیکن تا خط دروازه
        double final_fifth = scene->field.width * 0.20;  // 20 درصد پایانی زمین(نزدیک محوطه جریمه)

        //اگر بازیکن نزدیک انتهای زمینه مسیر حرکتشو مورب بکنه
        if (dist_to_end < final_fifth) {
            double mix = dist_to_end / final_fifth; 
            double center_y = scene->field.height / 2.0;
            target_y = (target_y * mix) + (center_y * (1.0 - mix));
        }

        // جلوگیری از ورود بازیکن به داخل تور دروازه
        double goal_margin_x = 15.0; // ۱۵ فاصله از خط دروازه
        if (target_x > scene->field.width - goal_margin_x) target_x = scene->field.width - goal_margin_x;
        if (target_x < goal_margin_x) target_x = goal_margin_x;

        //move_to_target(self, target_x, target_y);
        //return;
    }

    //*****************************
    // دروازه‌بان (بازیکن شماره 5) 
    else if (self->kit == 5) {
        float center_y = scene->field.height / 2.0f;
        float goal_half_width = 20.0f; 
        float safe_dist_gk = 50.0f; // کمی جلوتر از خط بایستد

        float base_x = (self->team == 1) ? safe_dist_gk : scene->field.width - safe_dist_gk;
        
        bool in_own_half = (self->team == 1 && scene->ball->position.x <= scene->field.width / 2.0f) ||
                           (self->team != 1 && scene->ball->position.x >= scene->field.width / 2.0f);
        
        float dist_to_ball = DISTANCE(self->position.x, self->position.y, scene->ball->position.x, scene->ball->position.y);

        //خروج دروازه‌بان: اگر توپ در محوطه نزدیک (150) است، به سمت توپ میرود
        if (in_own_half && dist_to_ball < 100.0f) {
            target_x = scene->ball->position.x;
            target_y = scene->ball->position.y;

         //اگر توپ خیلی نزدیک شد (کمتر از 50)، حتما شوت/دفع کند
            if (dist_to_ball < 80.0f) {
                self->state = SHOOTING;
            }
        } else {
            // ماندن روی خط دروازه
            target_x = base_x;
            target_y = scene->ball->position.y;

            // محدود کردن حرکت روی خط (نزدیک مرکز دروازه بماند)
            if (target_y < center_y - goal_half_width) target_y = center_y - goal_half_width;
            if (target_y > center_y + goal_half_width) target_y = center_y + goal_half_width;
        }
        //move_to_target(self, target_x, target_y);
        //return;
    }
    //*****************************

    //*****************************
    //مدافع ها (بازیکن شماره 3 و 4)
    else if (self->kit == 3 || self->kit == 4) {
        // مختصات دروازه خودی
        double own_goal_x = (self->team == 1) ? 0.0 : scene->field.width;
        double center_y = scene->field.height / 2.0;
        double half_line = scene->field.width / 2.0;
        
        double dist_to_ball = DISTANCE(self->position.x, self->position.y, scene->ball->position.x, scene->ball->position.y);

        // بررسی اینکه آیا توپ در نیمه خودی است
        bool ball_in_own_half = (self->team == 1 && scene->ball->position.x < half_line) || 
                                (self->team == 2 && scene->ball->position.x > half_line);

        if (ball_in_own_half && !teammate_has_ball) {
            // اگر توپ در نیمه خودی است و دست حریف یا آزاد است، به سمت آن برود
            if (dist_to_ball < 500.0) {
                target_x = scene->ball->position.x;
                target_y = scene->ball->position.y;
            } else {
                // اگر دور است، خط دفاعی تشکیل دهد (کمی جلوتر از دروازه‌بان)
                // قرارگیری بین توپ و دروازه با وزن بیشتر به سمت توپ
                target_x = (scene->ball->position.x* 2.0) + (own_goal_x * 1.0) / 3.0; 
                target_y = scene->ball->position.y; // هم‌تراز با توپ در محور Y
            }
        } else {
            // اگر توپ در نیمه حریف است یا دست هم‌تیمی است، در موقعیت پوششی بماند
            target_x = (self->team == 1) ? half_line - 300.0 : half_line + 300.0;
            if (self->kit == 3){
                target_y = home_y + 100;
            }else if (self->kit == 4){
                target_y = home_y - 100;
            }
        }

        // جلوگیری از خروج از نیمه و ورود به محوطه دروازه‌بان
        if (self->team == 1 && target_x > half_line - 10.0) target_x = half_line - 10.0; 
        if (self->team == 2 && target_x < half_line + 10.0) target_x = half_line + 10.0;

        float def_safe_dist = 20.0f; // دورتر از دروازه‌بان بایستد تا تداخل نکنند
        if (self->team == 1 && target_x < def_safe_dist) target_x = def_safe_dist;
        if (self->team == 2 && target_x > scene->field.width - def_safe_dist) target_x = scene->field.width - def_safe_dist;

        //move_to_target(self, target_x, target_y);
        //return; 
    } else {
    //*****************************

    //*****************************
    //مهاجمین و هافبک (بازیکنان شماره 0 و 1 و 2)
    // منطق حرکت بدون توپ
    double dist_to_ball = DISTANCE(self->position.x, self->position.y, scene->ball->position.x, scene->ball->position.y);
    double engage_radius = scene->field.width * 0.25;

     // آیا حریف توپ را در اختیار دارد؟ (تشخیص لو رفتن توپ)
    bool opponent_has_ball = (scene->ball->possessor != NULL && scene->ball->possessor->team != self->team);

    // 1. ترمز اضطراری برای جلوگیری از ادامه دادن مسیر بدون توپ
    if (opponent_has_ball) {
        // اگر بازیکن تیم ۱ است و با سرعت به سمت راست (دروازه حریف) می‌رود
        if (self->team == 1 && self->velocity.x > 0.5) {
            self->velocity.x *= 0.8; // کاهش سرعت
        } 
        // اگر بازیکن تیم ۲ است و با سرعت به سمت چپ می‌رود
        else if (self->team == 2 && self->velocity.x < -0.5) {
            self->velocity.x *= 0.8; // ترمز شدید
        }
    }

    // 2. تصمیم‌گیری برای حرکت
    if (dist_to_ball < engage_radius && !teammate_has_ball) {
        // اگر توپ نزدیک است (مثلاً تازه تکل خورده)، سریعاً بیفت دنبال توپ
        target_x = scene->ball->position.x;
        target_y = scene->ball->position.y;
    } 
    else {
        // توپ دور شده است. تعیین موقعیت برای جایگیری
        target_x = home_x;
        target_y = home_y;

        // ۳. اگر حریف توپ را گرفت، به جای رفتن به حمله، سریع برگرد عقب
        if (opponent_has_ball) {
            double shift_back = scene->field.width * 0.25; // برگشت به عقب
            
            if (self->team == 1) {
                target_x -= shift_back; // تیم ۱ باید به سمت چپ برگردد
            } else {
                target_x += shift_back; // تیم ۲ باید به سمت راست برگردد
            }
        }
    }
}
    //*****************************

    //*****************************
     // جلوگیری از خروج بازیکن از خطوط با استفاده از ابعاد داینامیک زمین
    double margin = 50.0; // حاشیه امن از خطوط
    if (target_x < margin) target_x = margin;
    if (target_x > scene->field.width - margin) target_x = scene->field.width - margin;
    if (target_y < margin) target_y = margin;
    if (target_y > scene->field.height - margin) target_y = scene->field.height - margin;

    // جلوگیری از ورود بازیکنان به داخل دروازه
    if (self->kit != 5) { // اگر دروازه بان نیست
        float min_safe_x = 50.0f; // فاصله امن از خط دروازه چپ
        float max_safe_x = scene->field.width - 50.0f; // فاصله امن از خط دروازه راست
        
        if (target_x < min_safe_x) target_x = min_safe_x;
        if (target_x > max_safe_x) target_x = max_safe_x;
    }

    move_to_target(self, target_x, target_y);

// جلوگیری از خروج از زمین (کنترل velocity واقعی)
if (self->position.x <= margin && self->velocity.x < 0)
    self->velocity.x = 0;

if (self->position.x >= scene->field.width - margin && self->velocity.x > 0)
    self->velocity.x = 0;

if (self->position.y <= margin && self->velocity.y < 0)
    self->velocity.y = 0;

if (self->position.y >= scene->field.height - margin && self->velocity.y > 0)
    self->velocity.y = 0;
}
/* Team 1 movement logic */
void movement_logic_1_0(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 1, 0.70); // 70% فاصله از دروازه خودی
    double home_y = scene->field.height * 0.50;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_1_1(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 1, 0.50); // 50% فاصله (وسط زمین)
    double home_y = scene->field.height * 0.25;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_1_2(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 1, 0.50);
    double home_y = scene->field.height * 0.75;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_1_3(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 1, 0.25); // 25% فاصله (دفاع)
    double home_y = scene->field.height * 0.30;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_1_4(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 1, 0.25);
    double home_y = scene->field.height * 0.70;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_1_5(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 1, 0.05); // 5% فاصله (دروازه‌بان)
    double home_y = scene->field.height * 0.50;
    movement_general_helper(self, scene, home_x, home_y);
}

/* Team 2 movement logic */
void movement_logic_2_0(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 2, 0.70); // 70% فاصله از دروازه خودی
    double home_y = scene->field.height * 0.50;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_2_1(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 2, 0.50);
    double home_y = scene->field.height * 0.25;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_2_2(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 2, 0.50);
    double home_y = scene->field.height * 0.75;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_2_3(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 2, 0.25);
    double home_y = scene->field.height * 0.30;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_2_4(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 2, 0.25);
    double home_y = scene->field.height * 0.70;
    movement_general_helper(self, scene, home_x, home_y);
}
void movement_logic_2_5(struct Player *self, struct Scene *scene) { 
    double home_x = get_dynamic_home_x(scene, 2, 0.05);
    double home_y = scene->field.height * 0.50;
    movement_general_helper(self, scene, home_x, home_y);
}
//*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_**_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*

//*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_**_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*
/* Team 1 shooting logic */
static void shooting_general_helper(struct Player *self, struct Scene *scene) {
    struct Ball *ball = scene->ball;
    float center_y = scene->field.height / 2.0f;

    // بررسی اینکه آیا توپ کاملاً ثابت است (شروع بازی، اوت، کرنر)
    bool is_stationary = (ball->velocity.x == 0.0f && ball->velocity.y == 0.0f);
    // سرعت مجاز توپ
    float max_shoot = ((float)self->talents.shooting / 10.0f) * MAX_BALL_VELOCITY; 

    struct Team* my_team_shoot = (self->team == 1) ? scene->first_team : scene->second_team;

    //*****************************
    // اگر بازیکنی که توپ را گرفته دروازه‌بان است
    if (self == my_team_shoot->players[5]) { 
    // محکم شوت بزن رو به جلو (دفع توپ)
    float direction = (self->team == 1) ? 1.0f : -1.0f;
    
    ball->velocity.x = direction * max_shoot;
    
    // یک زاویه تصادفی کوچیک روی محور Y بهش می‌دیم که توپ مستقیم نره تو پای حریف
    //float random_y_angle = ((rand() % 11) - 5) * 0.1f; // عددی بین 0.5- تا 0.5
    //ball->velocity.y = random_y_angle * max_shoot;
    return; 
    // خروج از تابع تا بقیه منطق پاس‌کاری اجرا نشود
    }
    //*****************************

    //*****************************
    // تشخیص تیم خودی
    struct Team* my_team = (self->team == 1) ? scene->first_team : scene->second_team;

    if (is_stationary) {
        // کیک اف **
           if (is_stationary) {
                struct Player* candidates[3];
        int count = 0;
        
        // پیدا کردن مهاجم و هافبک‌ها (شماره 0، 1 و 2) برای دریافت پاس شروع
        for (int i = 0; i <= 2; i++) {
            struct Player* p = my_team->players[i];
            // خود بازیکنی که توپ دستشه رو فیلتر می‌کنیم
            if (p != NULL && p != self) {
                candidates[count++] = p;
            }
        }
        
        // اگر بازیکنی برای پاس دادن پیدا شد، به طور تصادفی یکی رو انتخاب کن
        if (count > 0) {
            int r = rand() % count; // انتخاب یک عدد تصادفی
            struct Player* target = candidates[r];
            
            float dx = target->position.x - ball->position.x;
            float dy = target->position.y - ball->position.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist > 0.001f) {
                float pass_power = max_shoot * 0.6f; // قدرت پاس
                ball->velocity.x = (dx / dist) * pass_power;
                ball->velocity.y = (dy / dist) * pass_power;
                ball->possessor = NULL; 
                return; // پاس داده شد، خروج از تابع
            }
        }
        
        // اگر به هر دلیلی پاس نشد (ضربه ملایم رو به جلو)
        float direction = (self->team == 1) ? 1.0f : -1.0f;
        ball->velocity.x = direction * max_shoot * 0.3f;
        ball->velocity.y = 0.0f;
        ball->possessor = NULL;
        return;
    }


        //*****************************
        // جریان عادی بازی
        //*****************************
        //پاسکاری
    } else {
        struct Player* best_target = NULL;
        float min_dist_to_target = 9999.0f; // برای پیدا کردن نزدیک‌ترین هم‌تیمی آزاد
        struct Team* opp_team = (self->team == 1) ? scene->second_team : scene->first_team;
        
        // بررسی تمام بازیکنان تیم خودی برای یافتن بهترین گزینه پاس
        for (int i = 0; i < PLAYER_COUNT; i++) {
            struct Player* teammate = my_team->players[i];
            
            // به خودش و دروازه‌بان (کیت 5) پاس رو به عقب ندهد
            if (teammate != NULL && teammate != self && teammate->kit != 5) {
                
                // آیا هم‌تیمی به دروازه حریف نزدیک‌تر است؟ (پاس رو به جلو)
                bool is_closer_to_goal = false;
                if (self->team == 1) {
                    is_closer_to_goal = (teammate->position.x > self->position.x + 20.0f); // حداقل کمی جلوتر باشد
                } else {
                    is_closer_to_goal = (teammate->position.x < self->position.x - 20.0f);
                }
                
                if (is_closer_to_goal) {
                    // محاسبه فاصله تا هم‌تیمی
                    float t_dx = teammate->position.x - ball->position.x;
                    float t_dy = teammate->position.y - ball->position.y;
                    float dist_to_teammate = sqrtf(t_dx * t_dx + t_dy * t_dy);

                    // پاس‌های خیلی کوتاه (کمتر از 40) یا خیلی بلند را ایگنور کن
                    if (dist_to_teammate < 40.0f || dist_to_teammate > 400.0f) continue;

                    // بررسی مسیر پاس (آیا حریفی در مسیر هست؟)
                    bool path_is_clear = true;
                    float intercept_radius = 100.0f; // شعاع قطع توپ را کمی بیشتر کردیم تا امن‌تر پاس دهد
                    
                    for (int j = 0; j < PLAYER_COUNT; j++) {
                        struct Player* opp = opp_team->players[j];
                        if (opp == NULL) continue;
                        
                        float px = opp->position.x;
                        float py = opp->position.y;
                        float x1 = self->position.x;
                        float y1 = self->position.y;
                        float x2 = teammate->position.x;
                        float y2 = teammate->position.y;
                        
                        float l2 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
                        if (l2 > 0.0f) {
                            float t = ((px - x1) * (x2 - x1) + (py - y1) * (y2 - y1)) / l2;
                            t = fmaxf(0.0f, fminf(1.0f, t));
                            
                            float proj_x = x1 + t * (x2 - x1);
                            float proj_y = y1 + t * (y2 - y1);
                            
                            float dist_to_path = sqrtf((px - proj_x) * (px - proj_x) + (py - proj_y) * (py - proj_y));
                            
                            if (dist_to_path < intercept_radius) {
                                path_is_clear = false;
                                break;
                            }
                        }
                    }
                    
                    // به جای اولین نفر، نزدیک‌ترین هم‌تیمی آزاد و رو به جلو را پیدا می‌کنیم
                    if (path_is_clear && dist_to_teammate < min_dist_to_target) {
                        min_dist_to_target = dist_to_teammate;
                        best_target = teammate;
                    }
                }
            }
        }
        
        // اگر بهترین هدف پیدا شد، با قدرت مناسب پاس بده
        if (best_target != NULL) {
            // پاس را کمی جلوتر از پای هم‌تیمی می‌فرستیم
            float target_x = best_target->position.x + (best_target->velocity.x * 0.3f);
            float target_y = best_target->position.y + (best_target->velocity.y * 0.3f);

            float dx = target_x - ball->position.x;
            float dy = target_y - ball->position.y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            if (dist > 0.001f) {
                // قدرت پاس متناسب با فاصله تنظیم می‌شود! (جلوگیری از شوت شدن توپ به فضای خالی)
                float pass_power = dist * 1.2f; 
                if (pass_power > max_shoot * 0.9f) pass_power = max_shoot * 0.9f; // سقف قدرت پاس
                if (pass_power < max_shoot * 0.3f) pass_power = max_shoot * 0.3f; // حداقل قدرت پاس

                ball->velocity.x = (dx / dist) * pass_power;
                ball->velocity.y = (dy / dist) * pass_power;
                return; // پایان تابع
            }
        }
    //*****************************

        // در صورتی که پاس نداد، شوت می‌زند
        float goal_x = (self->team == 1) ? scene->field.width : 0.0f; 
        float goal_y = center_y;
        float goal_half_width = 25.0f; 
        
        if (ball->position.y < center_y) {
            goal_y = center_y - goal_half_width; 
        } else {
            goal_y = center_y + goal_half_width; 
        }

        float dx = goal_x - ball->position.x;
        float dy = goal_y - ball->position.y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 0.001f) {
            ball->velocity.x = (dx / dist) * max_shoot;
            ball->velocity.y = (dy / dist) * max_shoot;
        }

    //*****************************

    //شوت به دروازه وقتی نزیدکه
        float field_width = scene->field.width;
        float field_height = scene->field.height;
        float target_goal_x = (self->team == 1) ? field_width : 0.0f;
        float dist_to_goal = sqrtf(powf(self->position.x - target_goal_x, 2) + powf(self->position.y - center_y, 2));

        if (dist_to_goal < field_width * 0.25f) {
            
            // 1. آیا یاری در موقعیت بهتر (دهانه دروازه) برای پاس گل وجود دارد؟
            if (best_target != NULL && best_target->position.x > self->position.x + 10.0f) {
                 // اگر یاری جلوتر و در موقعیت خوب بود، به جای شوت به او پاس بده
                float target_x = best_target->position.x + (best_target->velocity.x * 0.3f);
                float target_y = best_target->position.y + (best_target->velocity.y * 0.3f);

                float dx = target_x - ball->position.x;
                float dy = target_y - ball->position.y;
                float dist = sqrtf(dx * dx + dy * dy);
                
                if (dist > 0.001f) {
                    float pass_power = dist * 1.2f; 
                    if (pass_power > max_shoot * 0.9f) pass_power = max_shoot * 0.9f; 
                    if (pass_power < max_shoot * 0.3f) pass_power = max_shoot * 0.3f; 

                    ball->velocity.x = (dx / dist) * pass_power;
                    ball->velocity.y = (dy / dist) * pass_power;
                    return; 
                }
            } else {
                // 2. اگر یاری نبود، شوت به گوشه‌های دروازه
                float goal_target_y = center_y;
                float goal_half_width = 45.0f; // عرض تقریبی نصف دروازه 

                // اگر بازیکن در نیمه بالای زمین است، به گوشه پایین شوت بزند و برعکس
                if (self->position.y < center_y) {
                    goal_target_y = center_y + goal_half_width - 10.0f; // گوشه پایین
                } else {
                    goal_target_y = center_y - goal_half_width + 10.0f; // گوشه بالا
                }

                float dx = target_goal_x - self->position.x;
                float dy = goal_target_y - self->position.y;
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist > 0.001f) {
                    ball->velocity.x = (dx / dist) * max_shoot; // شوت محکم
                    ball->velocity.y = (dy / dist) * max_shoot;
                    return;
                }
            }
        } /*else {
             // 3. اگر نه پاس داد و نه نزدیک دروازه بود (شوت/سانتر معمولی رو به جلو)
            float dx = target_goal_x - ball->position.x;
            float dy = center_y - ball->position.y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > 0.001f) {
                ball->velocity.x = (dx / dist) * max_shoot;
                ball->velocity.y = (dy / dist) * max_shoot;
            }
        }*/
    }
}
//*****************************


/* Team 1 shooting logic */
void shooting_logic_1_0(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_1_1(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_1_2(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_1_3(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_1_4(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_1_5(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }

/* Team 2 shooting logic */
void shooting_logic_2_0(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_2_1(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_2_2(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_2_3(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_2_4(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
void shooting_logic_2_5(struct Player *self, struct Scene *scene) { shooting_general_helper(self, scene); }
//*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_**_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*


void change_state_general(struct Player *self, struct Scene *scene) {
    //فاصله بازیکن تا توپ
    double dist_to_ball = DISTANCE(self->position.x, self->position.y, scene->ball->position.x, scene->ball->position.y);
   //ابعاد زمین
    double field_width = scene->field.width;
    double field_height = scene->field.height;

    //بررسی خروج توپ از مرزا
    int is_ball_out = (scene->ball->position.x < 0.0 || scene->ball->position.x > field_width || 
                       scene->ball->position.y < 0.0 || scene->ball->position.y > field_height);
    // اگر توپ اوت شد، بازیکنان به حالت حرکت در می‌آیند تا به پست خود برگردند
    if (is_ball_out) {
        self->state = MOVING;
        return;
    }

   // صاحب توپ (دریبل یاپاس؟)
    if (scene->ball->possessor == self) {
        bool is_stationary = (scene->ball->velocity.x == 0.0f && scene->ball->velocity.y == 0.0f);
        double target_goal_x = (self->team == 1) ? field_width : 0.0;
        double dist_to_goal = DISTANCE(self->position.x, self->position.y, target_goal_x, field_height / 2.0);
        
        // 1. دروازه‌بان و مدافعان (کیت 5 4 3) همیشه اولویتشون پاس دادن یا دفع توپه
        if (self->kit == 5 || self->kit == 4 || self->kit == 3) {
            self->state = SHOOTING;
            return;
        }

        // 2. مهاجمان و هافبک‌ها: اگر توپ کاشته است، یا به دروازه رسیده‌اند -> شوت
        if (is_stationary || dist_to_goal < field_width * 0.25) {
            self->state = SHOOTING;
        } 
        else {
            // 3. هافبک‌ها و مهاجمان در میانه زمین:
            // اگر حریفی در شعاع ۵۰ پیکسلی نزدیکشان شد، یا با احتمال ۱۵ درصد، دنبال پاس می‌گردند
            bool opponent_is_close = false;
            struct Team* opp_team = (self->team == 1) ? scene->second_team : scene->first_team;
            
            for (int i = 0; i < 5; i++) {
                if (opp_team->players[i] != NULL) {
                    float dx = opp_team->players[i]->position.x - self->position.x;
                    float dy = opp_team->players[i]->position.y - self->position.y;
                    if ((dx*dx + dy*dy) < 2500.0f) {
                        opponent_is_close = true;
                        break;
                    }
                }
            }

            if (opponent_is_close || rand() % 100 < 15) {
                self->state = SHOOTING; // رفتن به فاز پاس‌کاری
            } else {
                self->state = MOVING;   // ادامه دریبل
            }
        }
        return;
    }

    // منطق بازیکنان بدون توپ
    // بررسی اینکه آیا هم‌تیمی توپ را دارد؟
    bool teammate_has_ball = (scene->ball->possessor != NULL && scene->ball->possessor->team == self->team);

    if (teammate_has_ball) {
        // اگر توپ دست هم‌تیمی است، برای گرفتن توپ تلاش نکن! برو فضا سازی کن
        self->state = MOVING;
    } 
    else {
    if (dist_to_ball < 180.0) {
        self->state = INTERCEPTING; // تلاش برای گرفتن توپ
    } else {
        // همیشه در حالت MOVING باشند تا بتوانند به سر پست‌های خود (Home) برگردند!
        self->state = MOVING;
    }
}
}


/* Team 1 change_state logic */
void change_state_logic_1_0(struct Player *self, struct Scene *scene) {
change_state_general(self, scene);
}
void change_state_logic_1_1(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_1_2(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_1_3(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_1_4(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_1_5(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}

/* Team 2 change_state logic */
void change_state_logic_2_0(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_2_1(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_2_2(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_2_3(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_2_4(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
void change_state_logic_2_5(struct Player *self, struct Scene *scene) {
    change_state_general(self, scene);
}
/*-------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------
 * Lookup tables for factory
 * ------------------------------------------------------------------------- */
static PlayerLogicFn team1_movement[6] = {
    movement_logic_1_0, movement_logic_1_1, movement_logic_1_2,
    movement_logic_1_3, movement_logic_1_4, movement_logic_1_5
};

static PlayerLogicFn team2_movement[6] = {
    movement_logic_2_0, movement_logic_2_1, movement_logic_2_2,
    movement_logic_2_3, movement_logic_2_4, movement_logic_2_5
};

static PlayerLogicFn team1_shooting[6] = {
    shooting_logic_1_0, shooting_logic_1_1, shooting_logic_1_2,
    shooting_logic_1_3, shooting_logic_1_4, shooting_logic_1_5
};

static PlayerLogicFn team2_shooting[6] = {
    shooting_logic_2_0, shooting_logic_2_1, shooting_logic_2_2,
    shooting_logic_2_3, shooting_logic_2_4, shooting_logic_2_5
};

static PlayerLogicFn team1_change_state[6] = {
    change_state_logic_1_0, change_state_logic_1_1, change_state_logic_1_2,
    change_state_logic_1_3, change_state_logic_1_4, change_state_logic_1_5
};

static PlayerLogicFn team2_change_state[6] = {
    change_state_logic_2_0, change_state_logic_2_1, change_state_logic_2_2,
    change_state_logic_2_3, change_state_logic_2_4, change_state_logic_2_5
};

/* -------------------------------------------------------------------------
 * Factory functions
 * ------------------------------------------------------------------------- */
PlayerLogicFn get_movement_logic(int team, int kit) {
    if (coach_both_teams) return team1_movement[kit];
    return (team == 1) ? team1_movement[kit] : team2_movement[kit];
}

PlayerLogicFn get_shooting_logic(int team, int kit) {
    if (coach_both_teams) return team1_shooting[kit];
    return (team == 1) ? team1_shooting[kit] : team2_shooting[kit];
}

PlayerLogicFn get_change_state_logic(int team, int kit) {
    if (coach_both_teams) return team1_change_state[kit];
    return (team == 1) ? team1_change_state[kit] : team2_change_state[kit];
}

/* -------------------------------------------------------------------------
 * TALENTS
 *  TODO 2: Replace these default values with your desired skill points.
 * ------------------------------------------------------------------------- */
/* Team 1 */
static struct Talents team1_talents[6] = {
    {5, 6, 2, 7}, // [0] مهاجم: شوت و دریبل بالا، دفاع ضعیف
    {4, 6, 5, 5}, // [1] هافبک: متعادل و بازی‌ساز
    {5, 7, 5, 3}, // [2] هافبک: متعادل و بازی‌ساز
    {5, 6, 5, 4}, // [3]مدافع 
    {4, 7, 5, 4}, // [4] مدافع 
    {3, 9, 3, 5}  // [5] دروازه‌بان
};

/* Team 2 */
static struct Talents team2_talents[6] = {
    {4, 7, 4, 5}, // [0] مهاجم: شوت و دریبل بالا، دفاع ضعیف
    {4, 7, 5, 4}, // [1] هافبک: متعادل و بازی‌ساز
    {4, 6, 5, 5}, // [2] هافبک: متعادل و بازی‌ساز
    {5, 8, 5, 2}, // [3]مدافع 
    {3, 7, 7, 3}, // [4] مدافع
    {3, 7, 4, 6}  // [5] دروازه‌بان: سرعت واکنش و دفاع بسیار بالا
};

struct Talents get_talents(int team, int kit) {
    if (coach_both_teams) return team1_talents[kit];
    return (team == 1) ? team1_talents[kit] : team2_talents[kit];
}


/* -------------------------------------------------------------------------
 * Positioning
 *  TODO 3: Decide players positions at kick-off.
 *        Players must stay on their half, outside the center circle.
 *        Keep in mind that the kick-off team's first player will automatically
 *             be placed at the center of the pitch.
 * ------------------------------------------------------------------------- */
/* Team 1 */
static struct Vec2 team1_positions[6] = {
    {CENTER_X - 50,  CENTER_Y},       // [0] مهاجم: نزدیک دایره وسط
    {CENTER_X - 150, CENTER_Y - 100}, // [1] هافبک راست
    {CENTER_X - 150, CENTER_Y + 100}, // [2] هافبک چپ
    {CENTER_X - 250, CENTER_Y - 100}, // [3] مدافع راست
    {CENTER_X - 250, CENTER_Y + 100}, // [4] مدافع چپ
    {50,             CENTER_Y}        // [5] دروازه‌بان: چسبیده به دروازه خودی
};

/* Team 2 */
static struct Vec2 team2_positions[6] = {
    {CENTER_X + 50,  CENTER_Y},       // [0] مهاجم: نزدیک دایره وسط
    {CENTER_X + 150, CENTER_Y - 100}, // [1] هافبک چپ (قرینه)
    {CENTER_X + 150, CENTER_Y + 100}, // [2] هافبک راست (قرینه)
    {CENTER_X + 250, CENTER_Y - 100}, // [3] مدافع چپ (قرینه)
    {CENTER_X + 250, CENTER_Y + 100}, // [4] مدافع راست (قرینه)
    {PITCH_W - 50,   CENTER_Y}        // [5] دروازه‌بان: چسبیده به دروازه خودی
};

struct Vec2 get_positions(int team, int kit) {
    return (team == 1) ? team1_positions[kit] : team2_positions[kit];
}
//404106282 