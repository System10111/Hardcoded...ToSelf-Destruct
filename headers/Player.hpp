#pragma once
#include <divergence/Framework.h>
#include "Animator.hpp"
#include "Colider.hpp"

DvVector2 operator +(DvVector2 l, DvVector2 r){
    return {l.x+r.x,l.y+r.y};
}
DvVector2 operator -(DvVector2 l, DvVector2 r){
    return {l.x-r.x,l.y-r.y};
}
DvVector2 operator *(DvVector2 l, float r){
    return {l.x*r,l.y*r};
}
DvVector2 operator *(DvVector2 l, DvVector2 r){
    return {l.x*r.x,l.y*r.y};
}

constexpr DV_KK JUMP_KEY = DV_KK_SPACE;
constexpr DV_KK LEFT_KEY = DV_KK_LEFT;
constexpr DV_KK RIGHT_KEY = DV_KK_RIGHT;
constexpr DV_KK LEFT_KEY2 = DV_KK_A;
constexpr DV_KK RIGHT_KEY2 = DV_KK_D;

struct Player
{
    DvGame game;
    DvVector2 pos;
    DvVector2 vel = {0,0};
    DvVector2 drag = {8,0.1};
    float gravity = 600;
    float jumpStr = 200;
    float speed = 150;
    DvVector2 hBoxStart = {9,6};
    DvVector2 hBoxEnd = DvVector2{23,28};
    DvVector2 scale = {1,1};
    DvColor color;
    bool controlled;
    Animator animator;
    static Animator baseAnimator;
    Animator digits;
    static Animator baseDigits; 
    DvColor digitsColor = {0.75,0,0,1};
    bool landed;
    bool turnedLeft = false;
    float timeTillExplode = 15;
    bool exploded = false;

    float timeSinceJump = 0;

    Player(){};
    Player(DvGame g,DvVector2 _pos,DvVector2 _scale,DvColor _col = DV_COLOR_WHITE,bool _controlled = false)
    {
        color = _col;
        controlled = _controlled;
        animator.speed = baseAnimator.speed;
        animator.spriteSheet = baseAnimator.spriteSheet;
        animator.game = baseAnimator.game;
        animator.frames = baseAnimator.frames;
        animator.animations = baseAnimator.animations;
        animator.loop = true;
        digits.spriteSheet = baseDigits.spriteSheet;
        digits.game = baseDigits.game;
        digits.frames = baseDigits.frames;
        digits.animations = baseDigits.animations;
        game = g;
        pos = _pos;
        scale = _scale;
        hBoxStart = {hBoxStart.x*scale.x,hBoxStart.y*scale.y};
        hBoxEnd = {hBoxEnd.x*scale.x,hBoxEnd.y*scale.y};
    }

    int draw()
    {
        auto s = animator.getSourceVerts();
        int ttei = (int)timeTillExplode;
        int httei = (ttei/100);
        int tttei = ((ttei%100)/10);
        int dttei = ((ttei%10));
        if(turnedLeft)
        {
            auto tmp = s.S0;
            s.S0 = s.S1;
            s.S1 = tmp;
            tmp = s.S2;
            s.S2 = s.S3;
            s.S3 = tmp;
        }
        DV_ERR( dvDrawQuad(game,animator.spriteSheet,
            {pos.x,pos.y},
            {pos.x+scale.x*32,pos.y},
            {pos.x+scale.x*32,pos.y+scale.y*32},
            {pos.x,pos.y+scale.y*32},
            s.S0,s.S1,s.S2,s.S3,color
        ) )

        if(!turnedLeft && !exploded && (timeTillExplode > 10 || ((int)(timeTillExplode * 4))%3 > 0))
        {
            digits.setAnimation(std::to_string(httei).c_str());
            auto ds = digits.getSourceVerts();
            DvVector2 ex = {0,0};
            if((int)animator.time == 3) {
                ex = {0,1};
            } else if((int)animator.time == 5){
                ex = {0,2};
            } else if((int)animator.time == 10){
                ex = {0,4};
            }
            DV_ERR( dvDrawQuad(game,digits.spriteSheet,
                DvVector2{pos.x+scale.x*1,pos.y+scale.y*2} + ex * scale,
                DvVector2{pos.x+scale.x*4,pos.y+scale.y*2} + ex * scale,
                DvVector2{pos.x+scale.x*4,pos.y+scale.y*6} + ex * scale,
                DvVector2{pos.x+scale.x*1,pos.y+scale.y*6} + ex * scale,
                ds.S0,ds.S1,ds.S2,ds.S3,digitsColor
            ) );
            digits.setAnimation(std::to_string(tttei).c_str());
            ds = digits.getSourceVerts();
            DV_ERR( dvDrawQuad(game,digits.spriteSheet,
                DvVector2{pos.x+scale.x*5,pos.y+scale.y*2} + ex * scale,
                DvVector2{pos.x+scale.x*8,pos.y+scale.y*2} + ex * scale,
                DvVector2{pos.x+scale.x*8,pos.y+scale.y*6} + ex * scale,
                DvVector2{pos.x+scale.x*5,pos.y+scale.y*6} + ex * scale,
                ds.S0,ds.S1,ds.S2,ds.S3,digitsColor
            ) );
            digits.setAnimation(std::to_string(dttei).c_str());
            ds = digits.getSourceVerts();
            DV_ERR( dvDrawQuad(game,digits.spriteSheet,
                DvVector2{pos.x+scale.x*9,pos.y+scale.y*2} + ex * scale,
                DvVector2{pos.x+scale.x*12,pos.y+scale.y*2}+ ex * scale,
                DvVector2{pos.x+scale.x*12,pos.y+scale.y*6}+ ex * scale,
                DvVector2{pos.x+scale.x*9,pos.y+scale.y*6} + ex * scale,
                ds.S0,ds.S1,ds.S2,ds.S3,digitsColor
            ) );
        }
        return 0;
    }

    int update(double deltaT)
    {
        // printf("vel-y:%f ,tsj:%f\n",vel.y,timeSinceJump);
        if(timeTillExplode > 0)
        {
            timeTillExplode -= deltaT;
        } else {
            if(!exploded) explode();
        }
        timeSinceJump += deltaT;
        pos = pos + vel * deltaT;
        vel.x -= vel.x*drag.x * deltaT;
        vel.y -= vel.y*drag.y * deltaT;

        constexpr auto dMin = [](auto l,auto r)
        {
            return l < r ? l : r;
        };
        constexpr auto dMax = [](auto l,auto r)
        {
            return l > r ? l : r;
        };

        if(controlled && dvKeyboardKeyDown(game,JUMP_KEY)){
            vel.y += (gravity - (dMax(0,1.3-timeSinceJump) * jumpStr*2)) * deltaT;
        } else {
            vel.y += gravity * deltaT;
        }


        //-------------
        //--CONTROLS---
        //-------------
        if(controlled)
        {
            if(landed && dvKeyboardKeyClicked(game,JUMP_KEY))
            {
                timeSinceJump = 0;
                vel.y -= jumpStr;
                animator.setAnimation("jump");
                landed = false;
                animator.loop = false;
            }
            if(dvKeyboardKeyDown(game,RIGHT_KEY) || dvKeyboardKeyDown(game,RIGHT_KEY2))
            {
                if(landed)
                {
                    animator.setAnimationIfNot("walk");
                    animator.loop = true;
                }
                turnedLeft = false;
                vel.x = speed;
            }
            if(dvKeyboardKeyDown(game,LEFT_KEY) || dvKeyboardKeyDown(game,LEFT_KEY2))
            {
                if(landed && animator.getAnimation() != "land")
                {
                    animator.setAnimationIfNot("walk");
                    animator.loop = true;
                }
                turnedLeft = true;
                vel.x = -speed;
            }
            if(std::abs(vel.x) < 5 && landed && animator.getAnimation() != "land")
            {
                animator.setAnimationIfNot("stand");
                animator.loop = true;
            }
        }
        //-------------
        //-ANIMATIONS--
        //-------------
        if(!exploded && animator.getAnimation() == "land" && animator.ended())
        {
            animator.setAnimation("stand");
            animator.loop = true;
        }

        //-------------
        //--COLLISION--
        //-------------

        DvVector2 aVelX = {(float)(pos.x+vel.x * deltaT),pos.y};
        DvVector2 aVelY = {pos.x,(float)(pos.y+vel.y* deltaT)};
        DvVector2 aVelXY = {(float)(pos.x+vel.x* deltaT),(float)(pos.y+vel.y* deltaT)};
        bool colidedX = false,colidedY = false;
        for(auto& c : coliders)
        {
            if(colidedX && colidedY) break;
            if(!colidedX && c.doesColide(aVelX + hBoxStart,aVelX+hBoxEnd))
            {
                colidedX = true;
                vel.x = 0;
            }
            if(!colidedY && c.doesColide(aVelY + hBoxStart,aVelY+hBoxEnd))
            {
                colidedY = true;
                if(vel.y > 0)
                {
                    landed = true;
                    if(vel.y > 20 && !exploded)
                    {
                        animator.setAnimationIfNot("land");
                        animator.loop = false;
                    }
                } else {
                    landed = false;
                }
                vel.y = 0;
            }
            if(!colidedX && !colidedY && c.doesColide(aVelXY + hBoxStart,aVelXY+hBoxEnd))
            {
                colidedX = true;
                colidedY = true;
                if(vel.y > 0)
                {
                    landed = true;
                    if(vel.y > 20 && !exploded)
                    {
                        animator.setAnimationIfNot("land");
                        animator.loop = false;
                    }
                } else {
                    landed = false;
                }
                vel = {0,0};
            }
        }
        if(!colidedY && vel.y > 0) landed = false;

        animator.update(deltaT);
        return 0;
    }

    void explode()
    {
        exploded = true;
        controlled = false;
        animator.setAnimationIfNot("self-destruct");
        animator.loop = false;
        animator.speed *= 1.5;
    }
};

Animator Player::baseAnimator;
Animator Player::baseDigits;