#pragma once
#include <divergence/Framework.h>
#include <vector>

struct Colider
{
    DvVector2 begin;
    DvVector2 end;
    int draw(DvGame g,DvImage img,DvColor col)
    {
        dvDrawQuad(g,img,
            begin,{begin.x+1,begin.y},{begin.x+1,end.y},{begin.x,end.y},
            {0,0},{1,0},{1,1},{0,1},col
        );
        dvDrawQuad(g,img,
            begin,{end.x,begin.y},{end.x,begin.y+1},{begin.x,begin.y+1},
            {0,0},{1,0},{1,1},{0,1},col
        );
        dvDrawQuad(g,img,
            {begin.x,end.y},end,{end.x,end.y-1},{begin.x,end.y-1},
            {0,0},{1,0},{1,1},{0,1},col
        );
        dvDrawQuad(g,img,
            {end.x,begin.y},end,{end.x-1,end.y},{end.x-1,begin.y},
            {0,0},{1,0},{1,1},{0,1},col
        );
        return 0;
    }

    bool doesColide(DvVector2 oBegin,DvVector2 oEnd)
    {
        return oBegin.x < end.x &&
            oEnd.x > begin.x &&
            oBegin.y < end.y &&
            oEnd.y > begin.y;
    }
};

std::vector<Colider> coliders;