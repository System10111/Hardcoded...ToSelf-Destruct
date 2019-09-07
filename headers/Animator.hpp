#pragma once
#include <divergence/Framework.h>
#include <string>
#include <vector>
#include <algorithm>

/*Z tfuvu kyzj grik nyzcv yzxy. Yv yv.
  Zk'j gifsrscp kyv dfjk drxztrc tfuv pflmv vmvi ivru.
  Z ufe'k vmve befn, nyp z'd vetfuzex kyzj ze r tvrjvi tzgyvi, slk vyyy.
*/

//The animator struct
struct Animator 
{
    DvGame game;
    DvImage spriteSheet;
    float speed = 1.0;
    int curAnimation = 0;
    double time = 0.0;
    bool loop = false;

    struct Animation {
        std::string name;
        int from;
        int to;
    };
    std::vector<Animation> animations;
    struct Frame {
        int x,y,w,h;
    };
    std::vector<Frame> frames;

    
    Animator(){};
    Animator(DvGame& g, std::string sheetFile,std::string animationFile)
    {
        game = g;
        auto err = dvLoadImage(g,&spriteSheet,sheetFile.c_str(),0);
        if(err) {dvExit(g);return;}

        FILE* af = 0;
        #ifdef __STDC_LIB_EXT1__
        fopen_s(&af,animationFile.c_str(),"r");
        #else
        af = fopen(animationFile.c_str(),"rb");
        #endif
        if(af == NULL)
        {
            {dvExit(g);return;}
        }
        fseek(af,0,SEEK_END);
        long fileSize = ftell(af);
        fseek(af,0,SEEK_SET);
        std::vector<char> jsondat(fileSize);
        fread(jsondat.data(),fileSize,1,af);
        fclose(af);

        std::string json(jsondat.begin(),jsondat.end());

        auto framesIt = json.find("\"frames\"");
        if(framesIt == std::string::npos) {dvExit(g);return;}
        framesIt += sizeof("\"frames\"") - 1;
        json = json.substr(framesIt);
        std::string framess = json.substr(
            json.find('['),json.find(']') - 1
        );
        framess = framess.substr(framess.find_first_of('{'),framess.find_last_of('}')-framess.find_first_of('{'));
        size_t loc = 0;
        do
        {
            Frame fr;

            framess = framess.substr(loc+ sizeof("\"frame\""));
            
            std::string xstr = framess.substr(framess.find("\"x\"")+4,framess.find(',')-framess.find("\"x\"")-4);
            framess = framess.substr(framess.find(',')+2);
            fr.x = std::stoi(xstr);
            std::string ystr = framess.substr(framess.find("\"y\"")+4,framess.find(',')-framess.find("\"y\"")-4);
            framess = framess.substr(framess.find(',')+2);
            fr.y = std::stoi(ystr);
            std::string wstr = framess.substr(framess.find("\"w\"")+4,framess.find(',')-framess.find("\"w\"")-4);
            framess = framess.substr(framess.find(',')+2);
            fr.w = std::stoi(wstr);
            std::string hstr = framess.substr(framess.find("\"h\"")+4,framess.find(',')-framess.find("\"h\"")-5);
            framess = framess.substr(framess.find(',')+2);
            fr.h = std::stoi(hstr);
            loc = framess.find("\"frame\"");

            frames.push_back(fr);
        } while(loc != std::string::npos);

        auto frameTagsIt = json.find("\"frameTags\"");
        if(frameTagsIt == std::string::npos) {dvExit(g);return;}
        frameTagsIt += sizeof("\"frameTags\"") - 1;
        json = json.substr(frameTagsIt);
        std::string frametagss = json.substr(
            json.find('['),json.find(']') - 1
        );
        frametagss = frametagss.substr(frametagss.find_first_of('{'),frametagss.find_last_of('}')-frametagss.find_first_of('{'));
        loc = 0;
        do
        {
            Animation an;

            frametagss = frametagss.substr(loc+ sizeof("\"name\"") + 1);

            frametagss = frametagss.substr(frametagss.find('\"')+1);
            std::string namestr = frametagss.substr(0,frametagss.find('\"'));
            an.name = namestr;

            frametagss = frametagss.substr(frametagss.find(',')+1);
            std::string fromstr = frametagss.substr(frametagss.find("\"from\"")+7,frametagss.find(',')-frametagss.find("\"from\"")-7);
            frametagss = frametagss.substr(frametagss.find(',')+2);
            an.from = std::stoi(fromstr);
            std::string tostr = frametagss.substr(frametagss.find("\"to\"")+5,frametagss.find(',')-frametagss.find("\"to\"")-5);
            frametagss = frametagss.substr(frametagss.find(',')+2);
            an.to = std::stoi(tostr);
            loc = frametagss.find("\"name\"");

            animations.push_back(an);
        } while(loc != std::string::npos);
    }

    Animator(DvGame&g, std::string sheetFile)
    {
        game = g;
        auto err = dvLoadImage(g,&spriteSheet,sheetFile.c_str(),0);
        if(err) {dvExit(g);return;}
        frames.push_back(Frame{0,0,(int)spriteSheet->size.x,(int)spriteSheet->size.y});
        animations.push_back(Animation{"default",0,0});
    }

    void update(double deltaT)
    {
        if((int)time >= animations[curAnimation].to)
        {
            if(!loop) return;
            time -= animations[curAnimation].to - animations[curAnimation].from;
        }
        time += deltaT * speed * 10;
    }

    void setAnimation(const char* anim)
    {
        auto anIt = std::find_if(animations.begin(),animations.end(),[&](auto& an){return an.name == anim;});
        if(anIt == animations.end()) return;
        curAnimation = std::distance(animations.begin(), anIt);
        time = anIt->from;
    }
    void setAnimationIfNot(const char* anim)
    {
        auto anIt = std::find_if(animations.begin(),animations.end(),[&](auto& an){return an.name == anim;});
        if(anIt == animations.end()) return;
        if(curAnimation == std::distance(animations.begin(), anIt)) return;
        curAnimation = std::distance(animations.begin(), anIt);
        time = anIt->from;
    }

    bool ended()
    {
        return time >= animations[curAnimation].to;
    }

    void end()
    {
        time = animations[curAnimation].to;
    }

    std::string getAnimation()
    {
        return animations[curAnimation].name;
    }

    auto getSourceVerts()
    {
        struct {
            DvVector2 S0,S1,S2,S3;
        } res;
        if(animations.size() == 0 || frames.size() == 0) return res;
        auto t = time >=  frames.size() ? frames.size()-1 : (int)time;
        res.S0 = {(float)frames[t].x,(float)frames[t].y};
        res.S1 = {res.S0.x + frames[t].w,res.S0.y};
        res.S2 = {res.S1.x,res.S1.y + frames[t].h};
        res.S3 = {res.S0.x,res.S2.y};
        return res;
    }
};