#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <map>

#define DV_DYNAMIC_LOAD 1
#include <divergence/Engine.h>
#include <divergence/DynamicLoad.h>

#include "Animator.hpp"
#include "TextScroller.hpp"
#include "Player.hpp"
#include "Level.hpp"

/*--------------------------------*/
/*-----------CONSTANTS------------*/
/*--------------------------------*/
constexpr bool debugMode = false;
constexpr int FB_WIDTH = 640;
constexpr int FB_HEIGHT = 480;
constexpr int HUD_HEIGHT = 40;
constexpr DV_KK LVL_RESET_KEY = DV_KK_R;
constexpr float RESPAWN_DELAY = 0.75f;

/*--------------------------------*/
/*-------------ENUMS--------------*/
/*--------------------------------*/
enum class GameState {
    InvalidState,
    ExitGame,
    MainMenu,
    MainMenuInfo,
    Intro,
    InGame,
    Outro
};

/*--------------------------------*/
/*-----FUNCTIONS DECLARATIONS-----*/
/*--------------------------------*/
DvVector2 relMousePos(DvGame g);
void doTransition(GameState to = GameState::InvalidState);
void loadLevel(Level& l);
void spawn(DvGame g,DvVector2 pos);

/*--------------------------------*/
/*------------STRUCTS-------------*/
/*--------------------------------*/
struct MenuButton 
{
    bool active = false;
    DvGame game;
    Animator animator;
    DvVector2 position;
    DvVector2 scale = DV_VECTOR2_ONE;
    void(*action)(MenuButton& b) = nullptr;

    int update(double deltaT)
    {
        if(!active) return 0;
        auto s = animator.getSourceVerts();
        DvVector2 siz = {s.S2.x - s.S0.x, s.S2.y - s.S0.y};
        auto mousePos = relMousePos(game);
        if(
            mousePos.x > position.x && mousePos.y > position.y &&
            mousePos.x < position.x + siz.x * scale.x &&
            mousePos.y < position.y + siz.y * scale.y
        ) {
            if(dvMouseKeyDown(game,DV_MK_M1))
            {
                animator.setAnimation("pressed");
                animator.loop = false;
                action(*this);
            } else {
                animator.setAnimation("hover");
                animator.loop = false;
            }
        } else {
            animator.setAnimation("unpressed");
        }
        animator.update(deltaT);
        return 0;
    }

    int draw()
    {
        auto s = animator.getSourceVerts();
        DvVector2 siz = {s.S2.x - s.S0.x, s.S2.y - s.S0.y};
        DV_ERR( 
            dvDrawQuad(game,animator.spriteSheet,
                position,
                {position.x + siz.x * scale.x, position.y},
                {position.x + siz.x * scale.x, position.y + siz.y * scale.y},
                {position.x, position.y + siz.y * scale.y},
                s.S0,s.S1,s.S2,s.S3,DV_COLOR_WHITE
            ) 
        );
        return 0;
    }
};

/*--------------------------------*/
/*-----------VATIABLES------------*/
/*--------------------------------*/

std::mt19937 rng;

GameState gameState = GameState::MainMenu;
DvImage renderImage;
DvImage testImg;
GameState transitionTo = GameState::InvalidState;
Animator transition;
DvImage whitePixel;
Animator digits;

//----------MENU----------
MenuButton startButton;
MenuButton infoButton;
MenuButton exitButton;
DvImage title_screen;
DvImage footer;
//----------INFO----------
DvImage info_screen;
MenuButton infoBackButton;
//----------INTRO---------
TextScroller introScroller;
//---------INGAME---------
MenuButton igResume;
MenuButton igMainMenu;
MenuButton igRestart;
MenuButton igContinue;
DvVector2 spawnPoint {100,100};
float bombTimer = 3;
Animator background;
DvImage levelName;
float levelTime;
float parTime;
bool levelCompleted = false;
bool paused;
DvImage pausedScreen;
DvImage winScreen;
bool doLevelChange = false;
float respawnTime = 0;
//--------LEVELS----------
Level* curLevel;
Level level1;
Level level2;
Level level3;
Level level4;
Level level5;
Level levelEnd;


/*--------------------------------*/
/*-----FUNCTIONS DEFINITIONS------*/
/*--------------------------------*/
DvVector2 relMousePos(DvGame g)
{
    auto mousePos = dvMousePos(g);
    if(g->apis.windowAPI == DV_WAPI_WINDOWS) mousePos.y += 40;
    auto& winSize = g->properties.windowSize;
    mousePos.x -= (winSize.x - FB_WIDTH*(winSize.y/FB_HEIGHT))/2;
    mousePos.x *= (FB_HEIGHT/winSize.y);
    mousePos.y *= (FB_HEIGHT/winSize.y);
    return mousePos;
}

void doTransition(GameState to)
{
    transition.time = 0;
    transition.loop = false;
    transition.speed = 1.5;
    transitionTo = to;
}

void disableEverything()
{
    startButton.active = false;
    infoButton.active = false;
    exitButton.active = false;
    infoBackButton.active = false;
    igMainMenu.active = false;
    igResume.active = false;
    igRestart.active = false;
    igContinue.active = false;
}

void changeGameState(GameState state)
{
    gameState = state;
    disableEverything();
    switch (gameState)
    {
    case GameState::MainMenu:
        startButton.active = true;
        infoButton.active = true;
        exitButton.active = true;
        break;
    case GameState::MainMenuInfo:
        infoBackButton.active = true;
        break;
    default:break;
    }
}

void spawn(DvGame g,DvVector2 pos)
{
    std::uniform_int_distribution<std::mt19937::result_type> dist360(0,360);
    DvColor finalCol = dvColorHSV(dist360(rng),0.5,1);
    players.push_back(Player(g,spawnPoint,{2,2},finalCol,true));
    players.back().timeTillExplode = bombTimer;
}

void loadLevel(Level& l)
{
    respawnTime = 0;
    curLevel = &l;
    paused = false;
    players.clear();
    background = l.bg;
    levelName = l.levelName;
    spawnPoint = l.spawn;
    bombTimer = l.bombTimer;
    spawn(l.game,spawnPoint);
    coliders = l.coliders;
    gameObjects = l.gameObjects;
    levelTime = 0;
    parTime = l.parTime;
    levelCompleted = false;
}

void completeLevel()
{
    levelCompleted = true;
    igContinue.active = true;
    igRestart.active = true;
    for(auto& p : players)
    {
        p.controlled = false;
        p.countdown = false;
    }
}

void resetLevel()
{
    doTransition(GameState::InvalidState);
    // loadLevel(*curLevel);
    doLevelChange = true;
}
void nextLevel()
{
    doLevelChange = true;
    if (curLevel == &level1)
        curLevel = &level2;
    else if (curLevel == &level2)
        curLevel = &level3;
    else if (curLevel == &level3)
        curLevel = &level4;
    else if (curLevel == &level4)
        curLevel = &level5;


    else
        curLevel = &levelEnd;
    doLevelChange = true;
    doTransition(GameState::InvalidState);
}

GameObject* getGOWithId(int id)
{
    auto it = std::find_if(gameObjects.begin(),gameObjects.end(),[&](auto& g){return g.id == id;});
    return (it != gameObjects.end() ? &*it : nullptr);
}

void gameButtonUpdate(GameObject& go,double deltaT)
{
    go.animator.setAnimationIfNot("unpressed");
    auto b = go.realCol();
    bool is = false;
    for(auto &p : players)
    {
        if(p.exploded) continue;
        if(b.doesColide(p.pos + p.hBoxStart,p.pos + p.hBoxEnd))
        {
            is = true;
            break;
        }
    }
    if(is)
    {
        go.animator.setAnimationIfNot("pressed");
    } else {
        go.animator.setAnimationIfNot("unpressed");
    }
}

/*--------------------------------*/
/*--------------MAIN--------------*/
/*--------------------------------*/
int main() {
    #ifdef _WIN32
    atexit([](){system("pause");});
    #endif
    DV_PRERR( dvDynamicLoad() );

    std::random_device dev;
    rng = std::mt19937(dev());

    DvGame g;
    DV_PRERR( dvCreateEngine(&g) );

    g->Initialize = [](DvGame g){
        constexpr auto winName = "Hardcoded... To Self-Destruct";
        #ifdef __STDC_LIB_EXT1__
        strcpy_s(g->properties.windowName,winName);
        #else
        strcpy(g->properties.windowName,winName);
        #endif
        
        g->properties.windowSize = {FB_WIDTH,FB_HEIGHT};
        return 0;
    };
    g->Load = [](DvGame g){
        std::vector<uint8_t> data(FB_WIDTH*FB_HEIGHT*4,0);
        DV_ERR( dvCreateImage(g,&renderImage,data.data(),data.size(),{FB_WIDTH,FB_HEIGHT},DV_IMAGE_TARGET) );
        DV_ERR( dvLoadImage(g,&testImg,"resources/test.png",0) );
        DV_ERR( dvLoadImage(g,&title_screen,"resources/title-screen.png",0) );
        DV_ERR( dvLoadImage(g,&info_screen,"resources/info-screen.png",0) );
        DV_ERR( dvLoadImage(g,&footer,"resources/footer.png",0) );
        DV_ERR( dvLoadImage(g,&pausedScreen,"resources/pause.png",0) );
        DV_ERR( dvLoadImage(g,&winScreen,"resources/beaten.png",0) );

        std::vector<uint8_t> wpdata(16*16*4,0xFF);
        DV_ERR( dvCreateImage(g,&whitePixel,wpdata.data(),wpdata.size(),{16,16},0) );

        transition = Animator(g,"resources/transition.png","resources/transition.json");
        transition.time = transition.animations[0].to;

        startButton.game = g;
        startButton.active = true;
        startButton.animator = Animator(g,"resources/start-button.png","resources/start-button.json");
        startButton.position = {200,200};
        startButton.scale = {2,2};
        startButton.action = [](MenuButton& b)
        {
            doTransition(GameState::Intro);
        };

        introScroller = TextScroller(g,"resources/intro-screen.png",whitePixel);
        introScroller.scale = {4,4};
        introScroller.speed = 10;
        // introScroller.bgColor = DV_COLOR_RED;
        introScroller.lines.push_back(TextScroller::Line{{2,3},     {63 +1,7  +1},10,15});
        introScroller.lines.push_back(TextScroller::Line{{2,10},    {152+1,14 +1},20,35});
        introScroller.lines.push_back(TextScroller::Line{{2,17},    {153+1,21 +1},35,50});
        introScroller.lines.push_back(TextScroller::Line{{56,26},   {94 +1,59 +1},55,57});
        introScroller.lines.push_back(TextScroller::Line{{2,64},    {142+1,68 +1},80,95});
        introScroller.lines.push_back(TextScroller::Line{{2,71},    {156+1,75 +1},110,125});
        introScroller.lines.push_back(TextScroller::Line{{2,78},    {156+1,82 +1},135,150});
        introScroller.lines.push_back(TextScroller::Line{{2,85},    {147+1,89 +1},150,160});
        introScroller.lines.push_back(TextScroller::Line{{2,92},    {155+1,96 +1},170,185});
        introScroller.lines.push_back(TextScroller::Line{{2,99},    {87 +1,103+1},185,195});
        introScroller.lines.push_back(TextScroller::Line{{2,106},   {67 +1,110+1},210,217});
        introScroller.lines.push_back(TextScroller::Line{{2,113},   {156+1,117+1},230,245});
        introScroller.lines.push_back(TextScroller::Line{{108,31},  {153+1,54 +1},255,255});

        infoButton.game = g;
        infoButton.active = true;
        infoButton.animator = Animator(g,"resources/info-button.png","resources/info-button.json");
        infoButton.position = {230,270};
        infoButton.scale = {1.5,1.5};
        infoButton.action = [](MenuButton& b)
        {
            doTransition(GameState::MainMenuInfo);
        };

        exitButton.game = g;
        exitButton.active = true;
        exitButton.animator = Animator(g,"resources/exit-button.png","resources/exit-button.json");
        exitButton.position = {230,322};
        exitButton.scale = {1.5,1.5};
        exitButton.action = [](MenuButton& b)
        {
            doTransition(GameState::ExitGame);
        };

        igResume.game = g;
        igResume.animator = Animator(g,"resources/resume.png","resources/resume.json");
        igResume.position = DvVector2{65,55}*4;
        igResume.scale = {4,4};
        igResume.action = [](MenuButton& b)
        {
            paused = false;
        };

        igMainMenu.game = g;
        igMainMenu.animator = Animator(g,"resources/mmenu.png","resources/mmenu.json");
        igMainMenu.position = DvVector2{58,68}*4;
        igMainMenu.scale = {4,4};
        igMainMenu.action = [](MenuButton& b)
        {
            doTransition(GameState::MainMenu);
        };

        infoBackButton.game = g;
        infoBackButton.animator = Animator(g,"resources/back-button.png","resources/back-button.json");
        infoBackButton.position = {200,390};
        infoBackButton.scale = {2,2};
        infoBackButton.action = [](MenuButton& b)
        {
            doTransition(GameState::MainMenu);
        };

        igRestart.game = g;
        igRestart.animator = Animator(g,"resources/restart.png","resources/restart.json");
        igRestart.position = DvVector2{42,71} * 4;
        igRestart.scale = {4,4};
        igRestart.action = [](MenuButton& b)
        {
            resetLevel();
        };

        igContinue.game = g;
        igContinue.animator = Animator(g,"resources/continue.png","resources/continue.json");
        igContinue.position = DvVector2{105,71} * 4;
        igContinue.scale = {4,4};
        igContinue.action = [](MenuButton& b)
        {
            nextLevel();
            b.active = false;
        };

        Player::baseAnimator = Animator(g,"resources/player.png","resources/player.json");
        Player::baseAnimator.speed = 0.85;
        Player::baseDigits = Animator(g,"resources/digits.png","resources/digits.json");
        digits = Player::baseDigits;


        //------------
        //---LEVELS---
        //------------

        GameObject BUTTs = GameObject(g,Animator(g,"resources/butts.png","resources/butts.json"),{0,0},{2,2});
        BUTTs.hBox = {{48,38},{80,91}};
        BUTTs.onColide = [](GameObject &go, Player &p)
        {
            if(!levelCompleted && p.animator.getAnimation() == "self-destruct" && !p.animator.ended())
            {
                go.animator.speed = 1.2;
                go.animator.setAnimationIfNot("explode");
                go.animator.loop = false;
                respawnTime = -99;
            }
            if(go.animator.getAnimation() == "explode" && go.animator.ended())
            {
                completeLevel();
            }
        };

        GameObject plus10 = GameObject(g,Animator(g,"resources/plus10.png","resources/plus10.json"),{0,0},{4,4});
        plus10.hBox = {{0,0},{10,10}};
        plus10.usrUpdate = [](GameObject& go,double deltaT)
        {
            go.pos.y += std::sin(levelTime * 2) * deltaT * 8;
        };
        plus10.onColide = [](GameObject &go, Player&p)
        {
            if(go.visible)
            {
                go.visible = false;
                p.timeTillExplode += 10;
            }
        };
        plus10.animator.speed = 0.8;
        plus10.animator.loop = true;

        GameObject plus5 = GameObject(g,Animator(g,"resources/plus5.png","resources/plus10.json"),{0,0},{4,4});
        plus5.hBox = {{0,0},{10,10}};
        plus5.usrUpdate = [](GameObject& go,double deltaT)
        {
            go.pos.y += std::sin(levelTime * 2) * deltaT * 8;
        };
        plus5.onColide = [](GameObject &go, Player&p)
        {
            if(go.visible)
            {
                go.visible = false;
                p.timeTillExplode += 5;
            }
        };
        plus5.animator.speed = 0.8;
        plus5.animator.loop = true;
        GameObject gameButton = GameObject(g,Animator(g,"resources/button.png","resources/button.json"),{0,0},{2,2});
        gameButton.hBox = {{7,23},{25,30}};
        if(debugMode)
            gameButton.onColide = [](GameObject &go, Player&p){};
        gameButton.animator.loop = true;

        GameObject verDoor = GameObject(g,Animator(g,"resources/door.png","resources/door.json"),{0,0},{4,4});
        verDoor.hBox = {{0,0},{8,16}};
        verDoor.usrUpdate = [](GameObject& go,double deltaT)
        {
            if(go.animator.getAnimation() == "opening" && go.animator.ended())
            {
                go.hBox = {{0,0},{8,2}};
            } else {
                go.hBox = {{0,0},{8,16}};
            }
        };
        verDoor.onColide = [](GameObject &go, Player&p)
        {
            if((p.pos.x <= go.pos.x && p.vel.x > 0) || (p.pos.x > go.pos.x && p.vel.x < 0))
            {
                p.vel.x = 0;
            }
        };
        verDoor.animator.loop = false;

        //level1
        level1.game = g;
        level1.bg = Animator(g,"resources/level1/bg1.png","resources/level1/bg1.json");
        DV_ERR( dvLoadImage(g,&level1.levelName,"resources/level1/name.png",0) );
        level1.spawn = {100,300};
        level1.parTime = 4;
        level1.bombTimer = 50;
        level1.coliders.push_back({{ 0 * 4, 110 * 4},{ 160 * 4, 120 * 4}});
        level1.coliders.push_back({{ 0 * 4, 31 * 4},{ 5 * 4, 120 * 4}});
        level1.coliders.push_back({{ 90 * 4, 104 * 4},{ 160 * 4, 120 * 4}});
        level1.coliders.push_back({{ 95 * 4, 99 * 4},{ 160 * 4, 120 * 4}});
        level1.coliders.push_back({{ 100 * 4, 94 * 4},{ 160 * 4, 120 * 4}});
        level1.coliders.push_back({{ 105 * 4, 89 * 4},{ 160 * 4, 120 * 4}});
        level1.coliders.push_back({{ 111 * 4, 72 * 4},{ 160 * 4, 120 * 4}});
        level1.coliders.push_back({{ 160 * 4, 0 * 4},{ 200 * 4, 120 * 4}});

        level1.gameObjects.push_back(GameObject(g,Animator(g,"resources/level1/tip1.png"),DvVector2{25,20}*4,{4,4}));

        level1.gameObjects.push_back(GameObject(g,Animator(g,"resources/level1/tip2.png"),DvVector2{60,36}*4,{4,4}));
        level1.gameObjects.back().usrUpdate = [](GameObject &go, double deltaT)
        {
            go.visible = levelCompleted ||
                std::find_if(players.begin(),players.end(),[](auto& pl){return pl.controlled && pl.pos.x > 240;}) != players.end();   
        };

        level1.gameObjects.push_back(GameObject(g,Animator(g,"resources/level1/tip3.png"),DvVector2{91,30}*4,{4,4}));
        level1.gameObjects.back().usrUpdate = [](GameObject &go, double deltaT)
        {
            go.visible = levelCompleted ||
                std::find_if(players.begin(),players.end(),[](auto& pl){return pl.controlled && pl.pos.x > 400;}) != players.end();   
        };

        level1.gameObjects.push_back(GameObject(g,Animator(g,"resources/level1/tip4.png"),DvVector2{126,30}*4,{4,4}));
        level1.gameObjects.back().usrUpdate = [](GameObject &go, double deltaT)
        {
            go.visible = levelCompleted ||
                std::find_if(players.begin(),players.end(),[](auto& pl){return pl.controlled && pl.pos.x > 500;}) != players.end();   
        };
        level1.gameObjects.push_back(BUTTs);
        level1.gameObjects.back().pos = DvVector2{110,27}*4;

        //level2
        level2.game = g;
        level2.bg = Animator(g,"resources/level2/bg2.png");
        DV_ERR( dvLoadImage(g,&level2.levelName,"resources/level2/name.png",0) );
        level2.spawn = {290,70};
        level2.parTime = 8;
        level2.bombTimer = 4.1;
        level2.coliders.push_back({{1,0},{261,98}}); //0
        level2.coliders.push_back({{2,62},{182,150}}); //1
        level2.coliders.push_back({{1,62},{73,251}}); //2
        level2.coliders.push_back({{73,246},{229,274}}); //3
        level2.coliders.push_back({{178,223},{332,265}}); //4
        level2.coliders.push_back({{206,202},{376,236}}); //5
        level2.coliders.push_back({{230,171},{386,226}}); //6
        level2.coliders.push_back({{382,178},{441,230}}); //7
        level2.coliders.push_back({{438,187},{510,229}}); //8
        level2.coliders.push_back({{360,17},{384,96}}); //9
        level2.coliders.push_back({{380,60},{457,105}}); //10
        level2.coliders.push_back({{435,80},{629,111}}); //11
        level2.coliders.push_back({{559,74},{637,467}}); //12
        level2.coliders.push_back({{479,334},{631,470}}); //13
        level2.coliders.push_back({{631,470},{631,470}}); //14
        level2.coliders.push_back({{345,410},{609,471}}); //15
        level2.coliders.push_back({{206,336},{259,365}}); //16
        level2.coliders.push_back({{258,344},{348,412}}); //17
        level2.coliders.push_back({{315,337},{350,457}}); //18
        level2.coliders.push_back({{229,365},{323,470}}); //19
        level2.coliders.push_back({{76,269},{157,364}}); //20
        level2.coliders.push_back({{2,322},{129,475}}); //21
        level2.coliders.push_back({{103,456},{267,473}}); //22
        // level2.coliders.push_back({});

        level2.gameObjects.push_back(BUTTs);
        level2.gameObjects.back().pos = {70,298};
        level2.gameObjects.back().scale = {1.7,1.7};

        level2.gameObjects.push_back(plus10);
        level2.gameObjects.back().pos = {90,180};
        level2.gameObjects.back().animator.loop = true;
        level2.gameObjects.back().animator.speed = 0.8;

        level2.gameObjects.push_back(GameObject(g,Animator(g,"resources/level2/lava.png","resources/level2/lava.json"),DvVector2{71,65}*4,{4,4}));
        level2.gameObjects.back().onColide = [](GameObject &go, Player& p)
        {
            if(p.drag.x != 12 || p.drag.y != 30)
            {
                p.controlled = false;
                p.countdown = false;
                p.drag = {12,30};
                p.animator.speed *= 0.6f;
            }
        };
        level2.gameObjects.back().animator.loop = true;
        level2.gameObjects.back().animator.speed = 0.25f;
        level2.gameObjects.back().animator.loop = true;
        level2.gameObjects.back().drawAfterPlayer = true;
        level2.gameObjects.back().hBox = {{16,21},{49,37}};

        //level3
        level3.game = g;
        level3.bg = Animator(g,"resources/level3/bg3.png");
        DV_ERR( dvLoadImage(g,&level3.levelName,"resources/level3/name.png",0) );
        level3.spawn = {107,180};
        level3.parTime = 10;
        level3.bombTimer = 5;

        level3.coliders.push_back({{1,40},{79,171}}); //0
        level3.coliders.push_back({{0,41},{174,118}}); //1
        level3.coliders.push_back({{1,42},{191,104}}); //2
        level3.coliders.push_back({{1,39},{319,60}}); //3
        level3.coliders.push_back({{309,55},{374,89}}); //4
        level3.coliders.push_back({{316,49},{427,107}}); //5
        level3.coliders.push_back({{417,98},{451,183}}); //6
        level3.coliders.push_back({{185,169},{295,177}}); //7
        level3.coliders.push_back({{236,49},{263,90}}); //8
        level3.coliders.push_back({{360,169},{452,182}}); //9
        level3.coliders.push_back({{289,160},{326,182}});
        level3.coliders.push_back({{323,165},{363,182}});
        level3.coliders.push_back({{0,240},{180,314}});
        level3.coliders.push_back({{176,312},{263,368}});
        level3.coliders.push_back({{253,363},{637,477}});
        level3.coliders.push_back({{321,313},{407,374}});
        level3.coliders.push_back({{399,316},{475,373}});
        level3.coliders.push_back({{464,252},{638,477}});
        level3.coliders.push_back({{233,240},{566,252}});
        level3.coliders.push_back({{449,109},{559,120}});
        level3.coliders.push_back({{547,109},{605,293}});
        level3.coliders.push_back({{-10,162},{10,259}});
        // level3.coliders.push_back({});


        level3.gameObjects.push_back(BUTTs);
        level3.gameObjects.back().pos = DvVector2{93,15}*4;

        level3.gameObjects.push_back(plus5);
        level3.gameObjects.back().pos = {195,60};
        level3.gameObjects.back().animator.loop = true;
        level3.gameObjects.back().animator.speed = 0.8;

        level3.gameObjects.push_back(plus5);
        level3.gameObjects.back().pos = {268,60};
        level3.gameObjects.back().animator.loop = true;
        level3.gameObjects.back().animator.speed = 0.8;

        level3.gameObjects.push_back(plus5);
        level3.gameObjects.back().pos = {272,314};
        level3.gameObjects.back().animator.loop = true;
        level3.gameObjects.back().animator.speed = 0.8;

        level3.gameObjects.push_back(gameButton);
        level3.gameObjects.back().usrUpdate = [](GameObject& go,double deltaT)
        {
            gameButtonUpdate(go,deltaT);
            if(go.animator.getAnimation() == "unpressed")
            {
                getGOWithId(102)->animator.setAnimationIfNot("opening");
                getGOWithId(103)->animator.setAnimationIfNot("closing");
            } else {
                getGOWithId(102)->animator.setAnimationIfNot("closing");
                getGOWithId(103)->animator.setAnimationIfNot("opening");
            }
        };
        level3.gameObjects.back().pos = {405,258};
        level3.gameObjects.back().animator.loop = true;
        level3.gameObjects.back().animator.speed = 0.8;

        level3.gameObjects.push_back(gameButton);
        level3.gameObjects.back().usrUpdate = [](GameObject& go,double deltaT)
        {
            gameButtonUpdate(go,deltaT);
            if(go.animator.getAnimation() == "unpressed")
            {
                getGOWithId(101)->animator.setAnimationIfNot("opening");
                getGOWithId(104)->animator.setAnimationIfNot("closing");
            } else {
                getGOWithId(101)->animator.setAnimationIfNot("closing");
                getGOWithId(104)->animator.setAnimationIfNot("opening");
            }
        };
        level3.gameObjects.back().pos = {358,106};
        level3.gameObjects.back().animator.loop = true;
        level3.gameObjects.back().animator.speed = 0.8;

        level3.gameObjects.push_back(verDoor);
        level3.gameObjects.back().id = 101;
        level3.gameObjects.back().pos = {331,108};
        level3.gameObjects.back().animator.end();
        level3.gameObjects.back().scale = level3.gameObjects.back().scale * 0.9;

        level3.gameObjects.push_back(verDoor);
        level3.gameObjects.back().id = 102;
        level3.gameObjects.back().pos = {373,253};
        level3.gameObjects.back().animator.end();
        level3.gameObjects.back().scale = level3.gameObjects.back().scale * 0.9;

        level3.gameObjects.push_back(verDoor);
        level3.gameObjects.back().id = 103;
        level3.gameObjects.back().pos = {420,184};
        level3.gameObjects.back().animator.end();
        level3.gameObjects.back().scale = level3.gameObjects.back().scale * 0.9;

        level3.gameObjects.push_back(verDoor);
        level3.gameObjects.back().id = 104;
        level3.gameObjects.back().pos = {378,184};
        level3.gameObjects.back().animator.end();
        level3.gameObjects.back().scale = level3.gameObjects.back().scale * 0.9;
        
        //level4
        level4.game = g;
        level4.bg = Animator(g,"resources/level4/bg4.png");
        DV_ERR( dvLoadImage(g,&level4.levelName,"resources/level4/name.png",0) );
        level4.spawn = {48,262};
        level4.parTime = 8;
        level4.bombTimer = 5;

        level4.coliders.push_back({{-30,41},{0,479}}); //0
        level4.coliders.push_back({{0,321},{92,390}}); //1
        level4.coliders.push_back({{88,372},{135,479}}); //2
        level4.coliders.push_back({{489,353},{639,364}}); //3
        level4.coliders.push_back({{547,361},{638,479}}); //4
        level4.coliders.push_back({{640,41},{670,351}}); //5
        level4.coliders.push_back({{0,480},{640,500}}); //6
        // level3.coliders.push_back({});


        level4.gameObjects.push_back(BUTTs);
        level4.gameObjects.back().pos = DvVector2{117,43}*4;

        level4.gameObjects.push_back(GameObject(g,Animator(g,"resources/level4/lava.png","resources/level4/lava.json"),DvVector2{15,80}*4,{4,4}));
        level4.gameObjects.back().onColide = [](GameObject &go, Player& p)
        {
            if(p.drag.x != 12 || p.drag.y != 30)
            {
                p.controlled = false;
                p.countdown = false;
                p.drag = {12,30};
                p.animator.speed *= 0.6f;
            }
        };
        level4.gameObjects.back().animator.loop = true;
        level4.gameObjects.back().animator.speed = 0.25f;
        level4.gameObjects.back().animator.loop = true;
        level4.gameObjects.back().drawAfterPlayer = true;
        level4.gameObjects.back().hBox = {{18,20},{121,39}};

        level4.gameObjects.push_back(plus5);
        level4.gameObjects.back().pos = {8,275};
        level4.gameObjects.back().animator.loop = true;
        level4.gameObjects.back().animator.speed = 0.8;

        level4.gameObjects.push_back(plus5);
        level4.gameObjects.back().pos = {8,235};
        level4.gameObjects.back().animator.loop = true;
        level4.gameObjects.back().animator.speed = 0.8;

        level4.gameObjects.push_back(plus5);
        level4.gameObjects.back().pos = {8,195};
        level4.gameObjects.back().animator.loop = true;
        level4.gameObjects.back().animator.speed = 0.8;


        //level5
        level5.game = g;
        level5.bg = Animator(g,"resources/level5/bg5.png");
        DV_ERR( dvLoadImage(g,&level5.levelName,"resources/level5/name.png",0) ); 
        level5.spawn = {48,170};
        level5.parTime = 15;
        level5.bombTimer = 8;

        level5.coliders.push_back({{560,129},{642,229}}); //0
        level5.coliders.push_back({{640,40},{665,186}}); //1
        level5.coliders.push_back({{588,227},{636,293}}); //2
        level5.coliders.push_back({{472,277},{660,471}}); //3
        level5.coliders.push_back({{-5,439},{647,477}}); //4
        level5.coliders.push_back({{-27,38},{0,451}}); //5
        level5.coliders.push_back({{-2,241},{151,275}}); //6
        level5.coliders.push_back({{118,241},{151,375}}); //7
        level5.coliders.push_back({{152,341},{168,375}}); //8
        level5.coliders.push_back({{150,341},{185,368}}); //9
        level5.coliders.push_back({{147,342},{192,351}}); //10
        level5.coliders.push_back({{196,101},{328,128}}); //11
        // level3.coliders.push_back({});

        level5.gameObjects.push_back(plus10);
        level5.gameObjects.back().pos = {8,195};

        level5.gameObjects.push_back(plus10);
        level5.gameObjects.back().pos = {600,42};

        level5.gameObjects.push_back(plus10);
        level5.gameObjects.back().pos = {550,230};

        level5.gameObjects.push_back(verDoor);
        level5.gameObjects.back().id = 101;
        level5.gameObjects.back().pos = {135,375};
        level5.gameObjects.back().animator.end();

        level5.gameObjects.push_back(gameButton);
        level5.gameObjects.back().usrUpdate = [](GameObject& go,double deltaT)
        {
            gameButtonUpdate(go,deltaT);
            if(go.animator.getAnimation() == "unpressed")
            {
                getGOWithId(101)->animator.setAnimationIfNot("closing");
            } else {
                getGOWithId(101)->animator.setAnimationIfNot("opening");
            }
        };
        level5.gameObjects.back().pos = {580,68};
        level5.gameObjects.back().animator.loop = true;
        level5.gameObjects.back().animator.speed = 0.8;

        level5.gameObjects.push_back(BUTTs);
        level5.gameObjects.back().pos = DvVector2{-65,260};



        //levelEnd
        levelEnd.game = g;
        levelEnd.bg = Animator(g,"resources/levelEnd/bgEnd.png");
        DV_ERR( dvLoadImage(g,&levelEnd.levelName,"resources/levelEnd/name.png",0) ); 
        levelEnd.spawn = {283,156};
        levelEnd.parTime = 999;
        levelEnd.bombTimer = 20;

        levelEnd.coliders.push_back({{133,226},{471,248}}); //0

        return 0;
    };

    g->Update = [](DvGame g,double deltaT)
    {
        if(dvKeyboardKeyDown(g,DV_KK_Q))
        {
            DV_ERR( dvExit(g) );
        }

        if(transitionTo != GameState::InvalidState && (int)transition.time >= transition.animations[0].to * 0.5)
        {
            changeGameState(transitionTo);
            transitionTo = GameState::InvalidState;
        }

        if(gameState == GameState::ExitGame && transition.time > transition.animations[0].to)
        {
            DV_ERR( dvExit(g) );
        }

        switch (gameState)
        {
        case GameState::MainMenu:{
            startButton.update(deltaT);
            infoButton.update(deltaT);
            exitButton.update(deltaT);
        }break;
        case GameState::MainMenuInfo:{
            infoBackButton.update(deltaT);
        }break;
        case GameState::Intro:{
            introScroller.update(deltaT);
            if(dvKeyboardKeyClicked(g,DV_KK_ENTER))
            {
                if(introScroller.time < introScroller.lines.back().to){
                    introScroller.time = introScroller.lines.back().to;
                } else {
                    doTransition(GameState::InGame);
                    loadLevel(level1);
                }
            }
        }break;

        case GameState::InGame:{
            if(!paused && !levelCompleted && dvKeyboardKeyClicked(g,DV_KK_ESC))
            {
                igMainMenu.active = true;
                igResume.active = true;
                paused = true;
            } else if(paused)
            {
                igMainMenu.update(deltaT);
                igResume.update(deltaT);
                if(dvKeyboardKeyClicked(g,DV_KK_ESC))
                {
                    igMainMenu.active = false;
                    igResume.active = false;
                    paused = false;
                }
                break;
            }
            if(transitionTo == GameState::InvalidState && doLevelChange && (int)transition.time >= transition.animations[0].to * 0.5)
            {
                doLevelChange = false;
                loadLevel(*curLevel);
            }
            if(!levelCompleted)
            {
                levelTime += deltaT;
            } else {
                igContinue.update(deltaT);
                igRestart.update(deltaT);
                if(dvKeyboardKeyClicked(g,DV_KK_ENTER))
                {
                    igContinue.action(igContinue);
                }
                if(dvKeyboardKeyClicked(g,LVL_RESET_KEY))
                {
                    igRestart.action(igRestart);
                }
                break;
            }
            if(std::find_if(players.begin(),players.end(),[](auto& p){return p.controlled;}) == players.end())
            {
                if(respawnTime >= RESPAWN_DELAY)
                {
                    respawnTime = 0;
                    spawn(g,spawnPoint);
                } else {
                    respawnTime += deltaT;
                }
            }
            for(auto& p : players)
            {
                p.update(deltaT);
            }
            for(auto& go : gameObjects)
            {
                go.update(deltaT);
            }
            if(dvKeyboardKeyClicked(g,LVL_RESET_KEY) && (int)transition.time >= transition.animations[0].to)
            {
                resetLevel();
            }
            if(debugMode)
            {
                static bool e = false;
                if(!e)
                {
                    if(dvMouseKeyDown(g,DV_MK_M1))
                    {
                        auto mp = relMousePos(g);
                        printf("{%i,%i}",(int)mp.x,(int)mp.y);
                        e = true;
                    }
                } else {
                    if(dvMouseKeyDown(g,DV_MK_M2))
                    {
                        auto mp = relMousePos(g);
                        printf(",{%i,%i}\n",(int)mp.x,(int)mp.y);
                        e = false;
                    }
                }
                if(dvKeyboardKeyClicked(g,DV_KK_O))
                {
                    auto mp = relMousePos(g);
                    for(int i = 0; i < coliders.size();i++)
                    {
                        if(coliders[i].doesColide(mp,mp))
                        {
                            printf("%i,",i);
                        }
                    }
                    printf("\n");
                }
                if(dvKeyboardKeyClicked(g,DV_KK_P))
                {
                    spawn(g,spawnPoint);
                }
                if(dvKeyboardKeyClicked(g,DV_KK_L))
                {
                    nextLevel();
                }
            }
        }break;
        
        default: break;
        }

        
        transition.update(deltaT);
        return 0;
    };  

    g->Draw = [](DvGame g,double deltaT){
        DV_ERR( dvClear(g,DV_COLOR_BLACK) );
        DV_ERR( dvRenderTarget(g,renderImage) );
        DV_ERR( dvClear(g,DV_COLOR_BLACK) );
        //do drawing here
        switch (gameState)
        {
        case GameState::MainMenu:{
            DV_ERR( dvDraw(g,title_screen,DV_VECTOR2_ZERO,{4,4},DV_COLOR_WHITE) );
            DV_ERR( dvDraw(g,footer,{270,450},{2,2},DV_COLOR_WHITE) );
            startButton.draw();
            infoButton.draw();
            exitButton.draw();
        }break; 
        case GameState::MainMenuInfo:{
            DV_ERR( dvDraw(g,info_screen,DV_VECTOR2_ZERO,{4,4},DV_COLOR_WHITE) );
            infoBackButton.draw();
        }break;
        case GameState::Intro:{
            introScroller.draw();
        }break;
        case GameState::InGame:{
            auto bs = background.getSourceVerts();
            DV_ERR(
                dvDrawQuad(g,background.spriteSheet,
                    {0,0},{FB_WIDTH,0},{FB_WIDTH,FB_HEIGHT},{0,FB_HEIGHT},
                    bs.S0,bs.S1,bs.S2,bs.S3,DV_COLOR_WHITE
                )
            );

            for(auto& go : gameObjects)
            {
                if(!go.drawAfterPlayer)
                go.draw(debugMode ? whitePixel : nullptr);
            }
            for(auto& p : players)
            {
                p.draw();
            }
            for(auto& go : gameObjects)
            {
                if(go.drawAfterPlayer)
                go.draw(debugMode ? whitePixel : nullptr);
            }
            if constexpr(debugMode)
            {
                for(auto& c : coliders)
                {
                    c.draw(g,whitePixel,DV_COLOR_RED);
                }
            }
            DV_ERR(
                dvDrawQuad(g,whitePixel,
                    {0,0},{FB_WIDTH,0},{FB_WIDTH,HUD_HEIGHT},{0,HUD_HEIGHT},
                    {0,0},{1,0},{1,1},{0,1},DV_COLOR_BLACK
                )
            );
            DV_ERR( dvDraw(g,levelName,{10,10},{4,4},DV_COLOR_WHITE) );

            DvColor timeCol = levelTime > parTime ? DV_COLOR_RED : DV_COLOR_GREEN;
            int ilt = (int)levelTime;
            int hlt = (ilt/100);
            int tlt = ((ilt%100)/10);
            int dlt = ((ilt%10));
            int flt = ((int)(levelTime*10)%10);

            digits.setAnimation(std::to_string(hlt).c_str());
            auto ds = digits.getSourceVerts();
            DV_ERR( dvDrawQuad(g,digits.spriteSheet,
                DvVector2{4*138,4*3},
                DvVector2{4*141,4*3},
                DvVector2{4*141,4*7},
                DvVector2{4*138,4*7},
                ds.S0,ds.S1,ds.S2,ds.S3,timeCol
            ) );

            digits.setAnimation(std::to_string(tlt).c_str());
            ds = digits.getSourceVerts();
            DV_ERR( dvDrawQuad(g,digits.spriteSheet,
                DvVector2{4*142,4*3},
                DvVector2{4*145,4*3},
                DvVector2{4*145,4*7},
                DvVector2{4*142,4*7},
                ds.S0,ds.S1,ds.S2,ds.S3,timeCol
            ) );

            digits.setAnimation(std::to_string(dlt).c_str());
            ds = digits.getSourceVerts();
            DV_ERR( dvDrawQuad(g,digits.spriteSheet,
                DvVector2{4*146,4*3},
                DvVector2{4*149,4*3},
                DvVector2{4*149,4*7},
                DvVector2{4*146,4*7},
                ds.S0,ds.S1,ds.S2,ds.S3,timeCol
            ) );

            //dot
            DV_ERR( dvDrawQuad(g,whitePixel,
                DvVector2{4*150,4*6},
                DvVector2{4*151,4*6},
                DvVector2{4*151,4*7},
                DvVector2{4*150,4*7},
                {0,0},{1,0},{1,1},{0,1},timeCol
            ) );

            digits.setAnimation(std::to_string(flt).c_str());
            ds = digits.getSourceVerts();
            DV_ERR( dvDrawQuad(g,digits.spriteSheet,
                DvVector2{4*152,4*3},
                DvVector2{4*155,4*3},
                DvVector2{4*155,4*7},
                DvVector2{4*152,4*7},
                ds.S0,ds.S1,ds.S2,ds.S3,timeCol
            ) );

            if(paused)
            {
                dvDraw(g,pausedScreen,{0,0},{4,4},DV_COLOR_WHITE);
                igMainMenu.draw();
                igResume.draw();
            } else if (levelCompleted){
                dvDraw(g,winScreen,{0,0},{4,4},DV_COLOR_WHITE);
                DV_ERR( dvDraw(g,levelName,{320 - levelName->size.x*2,150},{4,4},DV_COLOR_WHITE) );
                igContinue.draw();
                igRestart.draw();
            }

        }break;


        default:break;
        }

        auto s = transition.getSourceVerts();
        DV_ERR( 
            dvDrawQuad(
                g,transition.spriteSheet,
                {0,0},{FB_WIDTH,0},{FB_WIDTH,FB_HEIGHT},{0,FB_HEIGHT},
                s.S0,s.S1,s.S2,s.S3,DV_COLOR_WHITE
            )
        );
        auto& winSize = g->properties.windowSize;
        DV_ERR( dvRenderTarget(g,nullptr) );
        DV_ERR ( 
            dvDraw(g,renderImage,
                { (winSize.x - renderImage->size.x*(winSize.y/renderImage->size.y))/2 , 0 },
                // DV_VECTOR2_ZERO,
                { (winSize.y/renderImage->size.y) , (winSize.y/renderImage->size.y)},
                // DV_VECTOR2_ONE,
                DV_COLOR_WHITE
            ) 
        );
        // dvDraw(g,testImg,relMousePos(g),{1,1},DV_COLOR_WHITE);
        return 0;
    };

    DV_PRERR( dvRunGame(g) );
    DV_PRERR( dvCleanGame(&g) );

    DV_PRERR( dvDynamicUnload() );

    return 0;
}
