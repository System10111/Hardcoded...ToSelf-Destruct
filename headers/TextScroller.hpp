#include <divergence/Framework.h>
#include <string>
#include <vector>
#include <algorithm>


struct TextScroller
{
    DvGame game;
    DvImage image;
    DvImage coverImage;
    DvVector2 position;
    DvVector2 scale;
    float speed = 1.0;
    double time = 0;
    DvColor bgColor = DV_COLOR_BLACK;
    struct Line {
        DvVector2 rectStart;
        DvVector2 rectEnd;
        float from;
        float to;
    };  
    std::vector<Line> lines;

    TextScroller(){};
    TextScroller(DvGame g, std::string imageFile,DvImage cover)
    {
        game = g;
        coverImage = cover;
        auto err = dvLoadImage(g,&image,imageFile.c_str(),0);
        if(err) {dvExit(g);return;}
    }
    void draw()
    {
        dvDraw(game,image,position,scale,DV_COLOR_WHITE);
        for(auto& l : lines)
        {
            if(l.to < time) continue;
            DvVector2 st = l.rectStart;
            if(l.from < time){
                st.x += (l.rectEnd.x - l.rectStart.x) * ((time - l.from) / (l.to - l.from));
            }
            dvDrawQuad(
                game,coverImage,
                {st.x           * scale.x,st.y * scale.y},
                {l.rectEnd.x    * scale.x,st.y * scale.y},
                {l.rectEnd.x    * scale.x,l.rectEnd.y   * scale.y},
                {st.x           * scale.x,l.rectEnd.y   * scale.y},
                {0,0},
                {coverImage->size.x,0},
                coverImage->size,
                {0,coverImage->size.y},
                bgColor
            );
        }
    }
    void update(double deltaT)
    {
        time += deltaT * speed;
    }
};