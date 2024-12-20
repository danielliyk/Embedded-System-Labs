#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include "SDL.h"
#include "TexturedRectangle.hpp"
#include "AnimatedSprite.hpp"
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <stdio.h>
#include <SDL2_ttf/SDL_ttf.h>
#include <SDL2_mixer/SDL_mixer.h>

//score
static int score = 0;
//Screen dimension constants
const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 600;

class LTexture
{
    public:
        //Initializes variables
        LTexture();
        //Deallocates memory
        ~LTexture();
        bool loadFromFile( std::string path );
        bool loadFromRenderedText( std::string textureText, SDL_Color textColor );
        void free();
        //Set color modulation
        void setColor( Uint8 red, Uint8 green, Uint8 blue );
        //Set blending
        void setBlendMode( SDL_BlendMode blending );
        //Set alpha modulation
        void setAlpha( Uint8 alpha );
        //Renders texture at given point
        void render( int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE );
        int getWidth();
        int getHeight();
    private:
        //The actual hardware texture
        SDL_Texture* mTexture;
        //Image dimensions
        int mWidth;
        int mHeight;
};

bool init();
bool loadMedia();
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

Mix_Music *gMusic = NULL;
Mix_Chunk *gMusic2 = NULL;
Mix_Chunk *gScratch = NULL;
Mix_Chunk *gSheep = NULL;
Mix_Chunk *gMedium = NULL;
Mix_Chunk *gLow = NULL;

TTF_Font *gFont = NULL;

LTexture gBGTexture;
LTexture gBGTextureStart;
LTexture gBGTextureEnd;
LTexture gObTexture;
LTexture gTextTexture;
LTexture gStartTexture;
LTexture gStartTextTexture;
LTexture gStartTextTexture2;

LTexture::LTexture()
{
    mTexture = NULL;
    mWidth = 0;
    mHeight = 0;
}

LTexture::~LTexture()
{
    free();
}

bool LTexture::loadFromFile( std::string path )
{
    free();
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load( path.c_str() );
    if( loadedSurface == NULL )
    {
        printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
    }
    else
    {
        SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0, 0xFF, 0xFF ) );
        newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
        if( newTexture == NULL )
        {
            printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
        }
        else
        {
            mWidth = loadedSurface->w;
            mHeight = loadedSurface->h;
        }
        SDL_FreeSurface( loadedSurface );
    }
    mTexture = newTexture;
    return mTexture != NULL;
}

bool LTexture::loadFromRenderedText( std::string textureText, SDL_Color textColor )
{
    free();

    //Render text surface
    SDL_Surface* textSurface = TTF_RenderText_Solid( gFont, textureText.c_str(), textColor );
    if( textSurface == NULL )
    {
        printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() );
    }
    else
    {
        //Create texture from surface pixels
        mTexture = SDL_CreateTextureFromSurface( gRenderer, textSurface );
        if( mTexture == NULL )
        {
            printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
        }
        else
        {
            //Get image dimensions
            mWidth = textSurface->w;
            mHeight = textSurface->h;
        }
        //Get rid of old surface
        SDL_FreeSurface( textSurface );
    }
    
    //Return success
    return mTexture != NULL;
}

void LTexture::free()
{
    //Free texture if it exists
    if( mTexture != NULL )
    {
        SDL_DestroyTexture( mTexture );
        mTexture = NULL;
        mWidth = 0;
        mHeight = 0;
    }
}

void LTexture::setColor( Uint8 red, Uint8 green, Uint8 blue )
{
    //Modulate texture rgb
    SDL_SetTextureColorMod( mTexture, red, green, blue );
}

void LTexture::setBlendMode( SDL_BlendMode blending )
{
    //Set blending function
    SDL_SetTextureBlendMode( mTexture, blending );
}

void LTexture::render( int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip )
{
    //Set rendering space and render to screen
    SDL_Rect renderQuad = { x, y, mWidth, mHeight };

    //Set clip rendering dimensions
    if( clip != NULL )
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }

    //Render to screen
    SDL_RenderCopyEx( gRenderer, mTexture, clip, &renderQuad, angle, center, flip );
}

int LTexture::getWidth()
{
    return mWidth;
}

int LTexture::getHeight()
{
    return mHeight;
}


bool init()
{
    //Initialization flag
    bool success = true;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        //Set texture filtering to linear
        if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
        {
            printf( "Warning: Linear texture filtering not enabled!" );
        }

        //Create window
        gWindow = SDL_CreateWindow( "dinosour adventure", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
        if( gWindow == NULL )
        {
            printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
            success = false;
        }
        else
        {
            //Create vsynced renderer for window
            gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
            if( gRenderer == NULL )
            {
                printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
                success = false;
            }
            else
            {
                //Initialize renderer color
                SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
                gBGTexture.loadFromFile( "src/assets/background/Background.png" );
                gBGTextureStart.loadFromFile("src/assets/background/StartBackground3.png");
                gObTexture.loadFromFile("src/assets/obstacles/Sheep1.png");
                gStartTexture.loadFromFile("src/assets/background/Background.png");
                
                //Initialize SDL_ttf
                if( TTF_Init() == -1 )
                {
                    printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
                    success = false;
                }
                
                //Initialize SDL_mixer
               if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
               {
                   printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
                   success = false;
               }
            }
        }
    }

    return success;
}

bool loadMedia()
{
    //Loading success flag
    bool success = true;
    
    //Open the font
    gFont = TTF_OpenFont( "src/assets/text/SEASRN__.ttf", 60 );
    if( gFont == NULL )
    {
        printf( "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError() );
        success = false;
    }
    else
    {
        //Render text
        SDL_Color textColor = { 0, 0, 0 };
        if( !gTextTexture.loadFromRenderedText(std::to_string(score), textColor ) )
        {
            printf( "Failed to render text texture!\n" );
            success = false;
        }
    }
    
    //Load music
    gMusic = Mix_LoadMUS( "src/assets/audio/background_music.wav" );
    if( gMusic == NULL )
    {
        printf( "Failed to load beat music! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }
    gMusic2 = Mix_LoadWAV("src/assets/audio/ending.wav");
    if( gMusic2 == NULL )
    {
        printf( "Failed to load beat music! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }
    
    //Load sound effects
    gScratch = Mix_LoadWAV( "src/assets/audio/surprise.wav" );
    if( gScratch == NULL )
    {
        printf( "Failed to load surprise sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }
    gSheep = Mix_LoadWAV( "src/assets/audio/poka.wav" );
    if( gScratch == NULL )
    {
        printf( "Failed to load surprise sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }
    
    //Load background texture
    if( !gBGTexture.loadFromFile( "src/assets/background/Background.png" ) )
    {
        printf( "Failed to load background texture!\n" );
        success = false;
    }
    if( !gObTexture.loadFromFile( "src/assets/obstacles/Sheep1.png" ) )
    {
        printf( "Failed to load sheep texture!\n" );
        success = false;
    }
    
    return success;
}

void close()
{
    //Free loaded images
    gTextTexture.free();

    //Free global font
    TTF_CloseFont( gFont );
    gFont = NULL;

    //Free the sound effects
    Mix_FreeChunk( gScratch );
    Mix_FreeChunk( gSheep );
    Mix_FreeChunk( gMedium );
    Mix_FreeChunk( gLow );
    gScratch = NULL;
    gSheep = NULL;
    gMedium = NULL;
    gLow = NULL;
    
    //Free the music
    Mix_FreeMusic( gMusic );
    gMusic = NULL;
    
    //Destroy window
    SDL_DestroyRenderer( gRenderer );
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;
    gRenderer = NULL;

    //Quit SDL subsystems
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}


bool checkCollision( SDL_Rect a, SDL_Rect b )
{
    int leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;

    rightA = a.x + 200;
    topA = a.y + 50;
    bottomA = a.y + 100;

    leftB = b.x;
    rightB = b.x + b.w;
    topB = b.y;
    bottomB = b.y + b.h;
    
    if( topA > bottomB)
        return false;
    if( bottomA < topB)
        return false;
    if( rightA < leftB || leftB < 0)
    {
        return false;
    }
    return true;
}
// game logic
int command(){
    if( !init() )
    {
        printf( "Failed to initialize!\n" );
    }
    else
    {
        if( !loadMedia() )
        {
            printf( "Failed to load media!\n" );
        }
        else
        {
            //Main loop flag
            bool quit = true;

            //Event handler
            SDL_Event e;

            int scrollingOffset = 0; //background
            int scrollingOffsetObj1 = 0; //tree
            int scrollingOffsetObj2 = 0; //sheep
            int scrollingOffsetObj3 = 0; //sheep2
            int scrollingOffsetObj4 = 0; //tree2
            int scrollingOffsetObj5 = 0; //tree3
            int scrollingOffsetObj6 = 0; //arrow
            int scrollingOffsetSS = 0;
            
            int posy = 0; // character y position
            int speed = 12;
            int random_pos_tree = 0;
            int random_pos_sheep = 0;
            int random_start_tree = 0;
            int random_start_sheep = 0;
            int random_pos_sheep1 = 120;
            int random_start_sheep1 = 100;
            
            int random_start_tree2 = 350;
            int random_pos_tree2 = 240;
            
            int random_start_tree3 = 460;
            int random_pos_tree3 = 480;
            
            int random_start_arrow = 480;
            int random_pos_arrow = 120;
            
            int cnt = 0;
            int life = 3;
            
            bool game = false;
            Mix_PlayMusic(gMusic, -1);
            
            //startup screen
            while(true) {
                //Scroll background
                scrollingOffsetSS-=speed;

                if( scrollingOffsetSS < -gBGTextureStart.getWidth() )
                {
                    scrollingOffsetSS = 0;
                }
                //Render background
                gBGTextureStart.render( scrollingOffsetSS,0 );
                gBGTextureStart.render( scrollingOffsetSS + gBGTextureStart.getWidth(), 0 );
                
                SDL_Color textColor = { 25, 20, 50 };
                gStartTextTexture.loadFromRenderedText("Dinosuar Adventure", textColor );
                gStartTextTexture.render( ( SCREEN_WIDTH - gStartTextTexture.getWidth() - 180), 250);
                
                static int frameNumber =0;

                AnimatedSprite animatedSprite(gRenderer,"src/assets/dino/Character.bmp");
                animatedSprite.Draw(290,300,300,300);
                animatedSprite.PlayFrame(-40,-30,300,250,frameNumber);
                animatedSprite.Render(gRenderer);
                frameNumber++;
                if(frameNumber>6){
                    frameNumber= 0;
                }
               
                SDL_RenderPresent(gRenderer);
                SDL_Delay(30);
                SDL_RenderClear(gRenderer);
                while( SDL_PollEvent( &e ) != 0 )
                {
                    if( e.type == SDL_KEYDOWN && e.key.repeat == 0 )
                    {
                        switch( e.key.keysym.sym )
                        {
                            case SDLK_SPACE: {
                                quit = false;
                                break;}
                        }
                    }
                    if (!quit)
                        break;
                }
                if(!quit)
                    break;
            }
            int oldcnt = cnt;
            // running
            while( !quit )
            {
                if(cnt >= 50)
                    game = true;
                srand(time(0));
                //Handle events on queue
                while( SDL_PollEvent( &e ) != 0 )
                {
                    //User requests quit
                    if( e.type == SDL_QUIT )
                    {
                        quit = true;
                    }
                    
                    if( e.type == SDL_KEYDOWN && e.key.repeat == 0 )
                    {
                        switch( e.key.keysym.sym )
                        {
                            case SDLK_UP: {
                                posy -= 120;
                                if( ( posy < 0 ))
                                {
                                    //Move back
                                    posy += 120;
                                }
                                break;}
                            case SDLK_DOWN: {
                                posy += 120;
                                if(( posy > 480 ) )
                                {
                                    //Move back
                                    posy -= 120;
                                }
                                break;}
                        }
                    }
                }
                
                int music = 0;
                if (game) {
                    Mix_HaltMusic();
                    Mix_HaltChannel(-1);
                    
                    scrollingOffset-=speed;
                    if( scrollingOffset < -gBGTexture.getWidth() )
                    {
                        scrollingOffset = 0;
                        cnt++;
                        std::cout << cnt<< std::endl;
                    }
                    //Render background
                    gBGTexture.render( scrollingOffset,0 );
                    gBGTexture.render( scrollingOffset + gBGTexture.getWidth(), 0 );
                
                    //Render current frame
                    gTextTexture.render( ( SCREEN_WIDTH - gTextTexture.getWidth() - 20), 20);
                    
                    SDL_Color textColor = { 0, 0, 0 };
                    gStartTextTexture.loadFromRenderedText("Final Score: ", textColor );
                    
                    gStartTextTexture.render( ( SCREEN_WIDTH - gStartTextTexture.getWidth() - 90), 25);

                    static int frameNumber =0;
                    AnimatedSprite animatedSprite(gRenderer,"src/assets/dino/Character.bmp");
                    animatedSprite.Draw(250,200,400,400);
                    animatedSprite.PlayFrame(-40,-30,300,250,frameNumber);
                    animatedSprite.Render(gRenderer);
                    frameNumber++;
                    if(frameNumber>6){
                        frameNumber= 0;
                    }
                    SDL_RenderPresent(gRenderer);
                    SDL_Delay(40);
                    SDL_RenderClear(gRenderer);
                    continue;
                }
 
                if (cnt % 4 == 0 && cnt != oldcnt)
                {
                    speed += 3;
                    oldcnt = cnt;
                }
                
                //Scroll background
                scrollingOffset-=speed;

                if( scrollingOffset < -gBGTexture.getWidth() )
                {
                    scrollingOffset = 0;
                    cnt++;
                    std::cout << cnt<< std::endl;
                }
                
                //scroll arrow
                scrollingOffsetObj6 -= 2*speed;
                if ( scrollingOffsetObj6 < -gBGTexture.getWidth()-random_start_arrow-140) {
                    scrollingOffsetObj6 = 0;
                    random_pos_arrow = ((rand()%5 * 120)/10)*10;
                    random_start_arrow = ((rand()%6 * 140)/10)*10;
                }
                
                //scroll tree
                scrollingOffsetObj1 -= speed;
                if ( scrollingOffsetObj1 < -gBGTexture.getWidth()-random_start_tree-140) {
                    scrollingOffsetObj1 = 0;
                    random_pos_tree = ((rand()%5 * 120)/10)*10;
                    random_start_tree = ((rand()%6 * 140)/10)*10;
                }
                
                scrollingOffsetObj4 -= speed;
                if ( scrollingOffsetObj4 < -gBGTexture.getWidth()-random_start_tree2 -140) {
                    scrollingOffsetObj4 = 0;
                    random_pos_tree2 = ((rand()%5 * 120)/10)*10;
                    random_start_tree2 = ((rand()%6 * 140)/10)*10;
                }
                scrollingOffsetObj5 -= speed;
                if ( scrollingOffsetObj5 < -gBGTexture.getWidth()-random_start_tree2 -140) {
                    scrollingOffsetObj5 = 0;
                    random_pos_tree3 = ((rand()%5 * 120)/10)*10;
                    random_start_tree3 = ((rand()%6 * 140)/10)*10;
                }
                //scroll sheep
                scrollingOffsetObj2 -= speed;
                if ( scrollingOffsetObj2 < -gBGTexture.getWidth()-random_start_sheep-140) {
                    scrollingOffsetObj2 = 0;
                    random_pos_sheep = ((rand()%5 * 120)/10)*10;
                    random_start_sheep = ((rand()%6 * 140)/10)*10;
                }
                scrollingOffsetObj3 -= speed;
                if ( scrollingOffsetObj3 < -2*gBGTexture.getWidth()) {
                    scrollingOffsetObj3 = 0;
                    random_pos_sheep1 = ((rand()%5 * 120)/10)*10;
                    random_start_sheep1 = ((rand()%6 * 140)/10)*10;
                }
                
                //Render background
                gBGTexture.render( scrollingOffset,0 );
                gBGTexture.render( scrollingOffset + gBGTexture.getWidth(), 0 );
                //Render current frame
                gTextTexture.render( ( SCREEN_WIDTH - gTextTexture.getWidth() - 20), 20);
                
                //check identical
                while ( random_start_tree == random_start_sheep) {
                    random_start_tree = ((rand()%6 * 140)/10)*10;
                }
                
                AnimatedSprite Sheep1(gRenderer, "src/assets/obstacles/Sheep1.bmp");
                AnimatedSprite Sheep2(gRenderer, "src/assets/obstacles/Sheep2.bmp");
                AnimatedSprite Tree(gRenderer, "src/assets/obstacles/Tree1.bmp");
                AnimatedSprite animatedSprite(gRenderer,"src/assets/dino/Character.bmp");
                AnimatedSprite Heart1(gRenderer, "src/assets/obstacles/heart.bmp");
                AnimatedSprite Heart2(gRenderer, "src/assets/obstacles/heart.bmp");
                AnimatedSprite Heart3(gRenderer, "src/assets/obstacles/heart.bmp");
                AnimatedSprite Tree2(gRenderer, "src/assets/obstacles/Tree2.bmp");
                AnimatedSprite Tree3(gRenderer, "src/assets/obstacles/Tree3.bmp");
                AnimatedSprite Arrow(gRenderer, "src/assets/obstacles/Arrow.bmp");
                
                //check collision
                if (checkCollision(animatedSprite.getRect(), Sheep1.getRect())) {
                    score++;
                    Mix_PlayChannel(-1, gSheep, 0);
                        SDL_Color textColor = { 0, 0, 0 };
                        gTextTexture.loadFromRenderedText(std::to_string(score), textColor );
                    random_pos_sheep = -100000;
                }
                if (checkCollision(animatedSprite.getRect(), Sheep2.getRect())) {
                    score++;
                    Mix_PlayChannel(-1, gSheep, 0);
                        SDL_Color textColor = { 0, 0, 0 };
                        gTextTexture.loadFromRenderedText(std::to_string(score), textColor );
                    random_pos_sheep1 = -100000;
                }
                if (checkCollision(animatedSprite.getRect(), Tree.getRect())) {
                    life--;
                    Mix_PlayChannel(-1, gScratch, 0);
                    random_pos_tree = -100000;
                }
                if (checkCollision(animatedSprite.getRect(), Tree2.getRect())) {
                    life--;
                    Mix_PlayChannel(-1, gScratch, 0);
                    random_pos_tree2 = -100000;
                }
                if (checkCollision(animatedSprite.getRect(), Tree3.getRect())) {
                    life--;
                    Mix_PlayChannel(-1, gScratch, 0);
                    random_pos_tree3 = -100000;
                }
                if (checkCollision(animatedSprite.getRect(), Arrow.getRect())) {
                    life--;
                    Mix_PlayChannel(-1, gScratch, 0);
                    random_pos_arrow = -100000;
                }
                
                Sheep1.Draw(random_start_sheep + scrollingOffsetObj2 + gBGTexture.getWidth(), random_pos_sheep, 140, 140);
                Sheep2.Draw(random_start_sheep1 + scrollingOffsetObj3 + gBGTexture.getWidth(), random_pos_sheep1, 140, 140);
                Tree.Draw(random_start_tree + scrollingOffsetObj1 + gBGTexture.getWidth(), random_pos_tree, 140, 140);
                Tree2.Draw(random_start_tree2 + scrollingOffsetObj4 + gBGTexture.getWidth(), random_pos_tree2, 140, 140);
                Tree3.Draw(random_start_tree3 + scrollingOffsetObj5 + gBGTexture.getWidth(), random_pos_tree3, 140, 140);
                animatedSprite.Draw(50,posy,200,200);
                Heart1.Draw(810, 510, 90, 90);
                Heart2.Draw(740, 510, 90, 90);
                Heart3.Draw(670, 510, 90, 90);
                Arrow.Draw(random_start_arrow + scrollingOffsetObj6 + gBGTexture.getWidth(), random_pos_arrow, 140, 140);

                static int frameNumber =0;

                Sheep1.PlayFrame(0, 0, 2000, 2000, 0);
                Sheep2.PlayFrame(0, 0, 3000, 3000, 0);
                Tree.PlayFrame(10, 10, 2000, 3000, 0);
                Tree2.PlayFrame(10, 10, 2000, 3000, 0);
                Tree3.PlayFrame(10, 10, 2000, 3000, 0);
                Arrow.PlayFrame(10, 10, 2000, 3000, 0);
                animatedSprite.PlayFrame(-40,-30,300,250,frameNumber);
                Heart1.PlayFrame(0, 0, 2000, 2000, 0);
                Heart2.PlayFrame(0, 0, 2000, 2000, 0);
                Heart3.PlayFrame(0, 0, 2000, 2000, 0);
                
                Sheep1.Render(gRenderer);
                Sheep2.Render(gRenderer);
                Tree.Render(gRenderer);
                Tree2.Render(gRenderer);
                Tree3.Render(gRenderer);
                Arrow.Render(gRenderer);
                animatedSprite.Render(gRenderer);
                     
                if (life == 3) {
                    Heart1.Render(gRenderer);
                    Heart2.Render(gRenderer);
                    Heart3.Render(gRenderer); }
                else if (life == 2) {
                    Heart1.Render(gRenderer);
                    Heart2.Render(gRenderer);
                }
                else if (life == 1) {
                    Heart1.Render(gRenderer);
                }
                else if (life == 0) {
                    game = true;
                }
                frameNumber++;
                if(frameNumber>6){
                    frameNumber= 0;
                }
                //show
                SDL_RenderPresent(gRenderer);
                SDL_Delay(30);
                SDL_RenderClear(gRenderer);

            }
        }
    }
    return 0;
}


int main(int argc, char* argv[]){
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        std::cout << "SDL could not be initialized: " <<
                  SDL_GetError();
    }else{
        std::cout << "SDL video system is ready to go\n";
    }
    command();
    //Free resources and close SDL
    close();
    SDL_DestroyWindow(gWindow);
    
    SDL_Quit();
    return 0;
}
