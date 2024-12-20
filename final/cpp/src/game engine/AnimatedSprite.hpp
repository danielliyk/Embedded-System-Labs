#ifndef ANIMATED_SPRITE_HPP
#define ANIMATED_SPRITE_HPP
#include <string>
#include <SDL2/SDL.h>

class AnimatedSprite{
    public:
        AnimatedSprite(SDL_Renderer*& renderer, std::string filepath);
        ~AnimatedSprite();
        void Draw(int x, int y, int w, int h);
        void PlayFrame(int x, int y,int w, int h, int frame);
        void Update();
        void Render(SDL_Renderer*& renderer);
        SDL_Rect getRect() { return m_dst; }

    private:
        SDL_Rect m_src;
        SDL_Rect m_dst;
        SDL_Texture* m_texture;
};

#endif
