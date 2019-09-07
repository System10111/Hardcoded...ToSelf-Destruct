#pragma once
#include <divergence/Framework.h>
#include <vector>
#include <memory>

#include "Colider.hpp"
#include "Player.hpp"

struct GameObject
{
    int id = 0;
    DvGame game;
    Animator animator;
    DvVector2 pos;
    DvVector2 scale;
    Colider hBox;
    bool turnedLeft = false;
    DvColor color = DV_COLOR_WHITE;
    bool visible = true;
    bool drawAfterPlayer = false;

    void (*usrUpdate)(GameObject& go,double deltaT) = nullptr;
    void (*onColide)(GameObject& go, Player& p) = nullptr;

    GameObject(){};
    GameObject(DvGame g,Animator an,DvVector2 _pos,DvVector2 _scale)
    {
        game = g;
        animator = an;
        pos = _pos;
        scale = _scale;
    }


    Colider realCol()
    {
        auto b = hBox;
        b.begin = b.begin * scale + pos;
        b.end = b.end * scale + pos;
        return b;
    }

    void draw(DvImage collisionShow = nullptr)
    {
        if(visible){
            auto s = animator.getSourceVerts();
            if(turnedLeft)
            {
                auto tmp = s.S0;
                s.S0 = s.S1;
                s.S1 = tmp;
                tmp = s.S2;
                s.S2 = s.S3;
                s.S3 = tmp;
            }
            DvVector2 size {
                std::abs(s.S0.x - s.S2.x),
                std::abs(s.S0.y - s.S2.y)
            };
            dvDrawQuad(game,animator.spriteSheet,
                {pos.x,pos.y},
                {pos.x+scale.x*size.x,pos.y},
                {pos.x+scale.x*size.x,pos.y+scale.y*size.y},
                {pos.x,pos.y+scale.y*size.y},
                s.S0,s.S1,s.S2,s.S3,color
            );
        }
        if(collisionShow && onColide)
        {
            auto b = realCol();
            b.draw(game,collisionShow, DV_COLOR_BLUE);
        }
    }

    void update(double deltaT)
    {
        animator.update(deltaT);
        if(onColide)
        {
            auto b = realCol();
            for(auto &p : players)
            {
                if(b.doesColide(p.pos + p.hBoxStart,p.pos + p.hBoxEnd))
                {
                    onColide(*this,p);
                }
            }
        }
        if(usrUpdate){usrUpdate(*this,deltaT);}
    }
};

struct Level {
    DvGame game;
    DvVector2 spawn;
    Animator bg;
    DvImage levelName;
    std::vector<Colider> coliders;
    std::vector<GameObject> gameObjects;
    float parTime = 10;
    float bombTimer = 15;
};

std::vector<GameObject> gameObjects;