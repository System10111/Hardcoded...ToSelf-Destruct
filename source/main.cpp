#include <iostream>
#include <cstring>
#include <vector>
#define DV_DYNAMIC_LOAD 1
#include <divergence/Engine.h>
#include <divergence/DynamicLoad.h>

#include "Animator.hpp"
#include "TextScroller.hpp"

//
#include <chrono>
#include <cstring>
#include <thread>
#include <regex>
#include <array>
//

/*--------------------------------*/
/*-----------CONSTANTS------------*/
/*--------------------------------*/
constexpr int FB_WIDTH = 640;
constexpr int FB_HEIGHT = 480;

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
            } else {
                if(animator.getAnimation() == "pressed")
                {
                    action(*this);
                }
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

GameState gameState = GameState::MainMenu;
DvImage renderImage;
DvImage testImg;
GameState transitionTo = GameState::InvalidState;
Animator transition;
DvImage whitePixel;

//----------MENU----------
MenuButton startButton;
MenuButton infoButton;
MenuButton exitButton;
DvImage title_screen;
DvImage footer;
//---------INFO-----------
DvImage info_screen;
MenuButton infoBackButton;
//--------INTRO-----------
TextScroller introScroller;

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

/*--------------------------------*/
/*--------------MAIN--------------*/
/*--------------------------------*/
int main() {
    #ifdef _WIN32
    atexit([](){system("pause");});
    #endif
    DV_PRERR( dvDynamicLoad() );

    DvGame g;
    DV_PRERR( dvCreateEngine(&g) );

    g->Initialize = [](DvGame g){
        strcpy_s(g->properties.windowName,"Hardcoded... To Self-Destruct");
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

        infoBackButton.game = g;
        infoBackButton.animator = Animator(g,"resources/back-button.png","resources/back-button.json");
        infoBackButton.position = {200,390};
        infoBackButton.scale = {2,2};
        infoBackButton.action = [](MenuButton& b)
        {
            doTransition(GameState::MainMenu);
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
