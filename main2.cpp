#include "SDL2/SDL.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

#define TILE_SIZE 32

// Struktura przechowująca mapę gry
struct game_map_t {
    int width, height;
    std::vector<int> map;

    int get(int x, int y) const {
        if (x < 0) return 1;
        if (x >= width) return 1;
        if (y < 0) return 1;
        if (y >= height) return 1;
        return map[y * width + x];
    }
};

// Przykładowa mapa gry
game_map_t game_map = {
        20, 15, {
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                1,1,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,1,1
        }
};

// Struktura przechowująca wektor dwuwymiarowy
union vect_t {
    struct { double x; double y; } v;
    double p[2];
};

// Przeciążenie operatora + dla wektora
vect_t operator+(const vect_t a, const vect_t b) {
    vect_t ret = a;
    ret.v.x += b.v.x;
    ret.v.y += b.v.y;
    return ret;
}

// Przeciążenie operatora * dla wektora i liczby
vect_t operator*(const vect_t a, const double b) {
    vect_t ret = a;
    ret.v.x *= b;
    ret.v.y *= b;
    return ret;
}

// Struktura przechowująca informacje o graczu
struct player_t {
    vect_t p; // pozycja
    vect_t v; // prędkość
    vect_t a; // przyspieszenie
};

// Funkcja sprawdzająca, czy dany punkt jest w kolizji z mapą gry
bool is_in_collision(vect_t pos, const game_map_t &map) {
    return map.get((int)pos.v.x, (int)pos.v.y) > 0;
}

// Funkcja sprawdzająca, czy gracz stoi na ziemi (jest na podłożu)
bool is_on_the_ground(player_t player, const game_map_t &map) {
    return map.get((int)player.p.v.x, (int)(player.p.v.y + 0.01)) > 0;
}

// Funkcja aktualizująca stan gracza na podstawie fizyki gry
player_t update_player(player_t player_old, const game_map_t &map, double dt) {
    player_t player = player_old;

    // Ustawienie przyspieszenia ziemskiego, jeśli gracz nie stoi na ziemi
    if (!is_on_the_ground(player_old, map)) {
        player_old.a.v.y = 10; // przyspieszenie ziemskie (w powietrzu)
    }

    // Obliczenie nowej pozycji gracza
    player.p = player_old.p + (player_old.v * dt) + (player_old.a * dt * dt) * 0.5;
    // Obliczenie nowej prędkości gracza
    player.v = player_old.v + (player_old.a * dt);
    player.v = player.v * 0.99; // Zmniejszenie prędkości w wyniku tarcia

    // Obsługa kolizji z blokami
    std::vector<vect_t> collision_points = {
            { { -0.4, 0.0 } },
            { { 0.4, 0.0 } }
    };

    std::vector<vect_t> collision_mods = {
            { { 0.0, -1.0 } },
            { { 0.0, -1.0 } }
    };

    for (int i = 0; i < collision_points.size(); i++) {
        auto test_point = player.p + collision_points[i];

        if (is_in_collision(test_point, map)) {
            if (collision_mods[i].v.y < 0) {
                // Jeśli kolizja z blokiem z dołu, zatrzymaj pionową prędkość
                player.v.v.y = 0;
                player.p.v.y = player_old.p.v.y;
            } else {
                // Jeśli kolizja z blokiem z innej strony, zatrzymaj cały ruch
                player.v.v.x = 0;
                player.v.v.y = 0;
                player.p = player_old.p;
            }
        }
    }

    // Sprawdzenie, czy gracz spadł poniżej mapy
    if (player.p.v.y < 0) {
        // Zresetowanie pozycji gracza na początkową (można dostosować według potrzeb)
        player.p.v.x = 0; // Ustawienie X na lewej krawędzi
        player.p.v.y = 0; // Ustawienie Y na dolnej krawędzi
        player.v.v.x = 0; // Zatrzymanie prędkości poziomej
        player.v.v.y = 0; // Zatrzymanie prędkości pionowej
    }

    return player;
}

// Funkcja rysująca mapę gry
void draw_map(SDL_Renderer *renderer, game_map_t &map, std::shared_ptr<SDL_Texture> tex) {
    for (int y = map.height - 1; y >= 0; y--)
        for (int x = 0; x < map.width; x++) {
            SDL_Rect dst = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
            if (map.get(x, y) > 0) {
                SDL_Rect src = { 128 * (map.get(x, y) - 1), 0, TILE_SIZE, TILE_SIZE };
                SDL_RenderCopy(renderer, tex.get(), &src, &dst);
            }
        }
}

// Funkcja ładowania tekstury z pliku BMP
std::shared_ptr<SDL_Texture> load_image(SDL_Renderer *renderer, const char *path) {
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface from image: %s", SDL_GetError());
        throw std::invalid_argument(SDL_GetError());
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
        SDL_FreeSurface(surface);
        throw std::invalid_argument(SDL_GetError());
    }
    std::shared_ptr<SDL_Texture> tex(texture, SDL_DestroyTexture);
    SDL_FreeSurface(surface);
    return tex;
}

// Funkcja ładowania obu tekstur gracza
std::vector<std::shared_ptr<SDL_Texture>> load_player_textures(SDL_Renderer *renderer) {
    std::vector<std::shared_ptr<SDL_Texture>> textures;

    SDL_Surface *surface1 = SDL_LoadBMP("image/player.bmp");
    if (!surface1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface from image: %s", SDL_GetError());
        throw std::invalid_argument(SDL_GetError());
    }
    SDL_SetColorKey(surface1, SDL_TRUE, SDL_MapRGB(surface1->format, 0, 255, 255));
    SDL_Texture *texture1 = SDL_CreateTextureFromSurface(renderer, surface1);
    if (!texture1) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
        SDL_FreeSurface(surface1);
        throw std::invalid_argument(SDL_GetError());
    }
    textures.emplace_back(texture1, SDL_DestroyTexture);
    SDL_FreeSurface(surface1);

    SDL_Surface *surface2 = SDL_LoadBMP("image/player2.bmp");
    if (!surface2) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create surface from image: %s", SDL_GetError());
        throw std::invalid_argument(SDL_GetError());
    }
    SDL_SetColorKey(surface2, SDL_TRUE, SDL_MapRGB(surface2->format, 0, 255, 255));
    SDL_Texture *texture2 = SDL_CreateTextureFromSurface(renderer, surface2);
    if (!texture2) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
        SDL_FreeSurface(surface2);
        throw std::invalid_argument(SDL_GetError());
    }
    textures.emplace_back(texture2, SDL_DestroyTexture);
    SDL_FreeSurface(surface2);

    return textures;
}

// Główna funkcja programu
int main(int argc, char *argv[]) {
    using namespace std::chrono_literals;
    using namespace std::chrono;
    using namespace std;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return 3;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_CreateWindowAndRenderer(800, 600, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
        return 3;
    }

    std::vector<std::shared_ptr<SDL_Texture>> player_textures = load_player_textures(renderer);
    auto tiles_texture = load_image(renderer, "image/block.bmp");
    auto background_texture = load_image(renderer, "image/background.bmp");

    bool still_playing = true;
    player_t player = { { 1, 1 }, { 0, 0 }, { 0, 0 } };

    double dt = 1.0 / 60.0;
    double game_time = 0.0;
    steady_clock::time_point current_time = steady_clock::now();

    bool is_player_texture1 = true; // Flaga do przełączania tekstur gracza
    int player_frame_counter = 0;

    while (still_playing) {
        // Obsługa zdarzeń
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    still_playing = false;
                    break;
                case SDL_KEYDOWN:
                    if (is_on_the_ground(player, game_map)) {
                        if (event.key.keysym.scancode == SDL_SCANCODE_UP) player.a.v.y = -500;
                        if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                            player.a.v.x = -2;
                            is_player_texture1 = false;
                        }
                        if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                            player.a.v.x = 2;
                            is_player_texture1 = true;
                        }
                    }
                    break;
                case SDL_KEYUP:
                    if (event.key.keysym.scancode == SDL_SCANCODE_Q) still_playing = false;
                    if (event.key.keysym.scancode == SDL_SCANCODE_UP) player.a.v.y = 0;
                    if (event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                        player.a.v.x = 0;
                        player_frame_counter = 0;
                    }
                    break;
            }
        }

        // Aktualizacja fizyki gry
        game_time += dt;
        player = update_player(player, game_map, dt);

        // Renderowanie
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, background_texture.get(), NULL, NULL);
        draw_map(renderer, game_map, tiles_texture);

        // Określenie, którą teksturę gracza należy renderować na podstawie kierunku ruchu
        SDL_Rect player_rect = { (int)(player.p.v.x * TILE_SIZE - (TILE_SIZE / 2)), (int)(player.p.v.y * TILE_SIZE - TILE_SIZE), TILE_SIZE, TILE_SIZE * 2 };
        SDL_Texture *current_player_texture = is_player_texture1 ? player_textures[0].get() : player_textures[1].get();
        SDL_RenderCopyEx(renderer, current_player_texture, NULL, &player_rect, 0, NULL, SDL_FLIP_NONE);

        SDL_RenderPresent(renderer);

        // Opóźnienie w celu ograniczenia liczby klatek na sekundę
        current_time = current_time + microseconds((long long int)(dt * 1000000.0));
        std::this_thread::sleep_until(current_time);

        // Licznik klatek dla animacji gracza
        if (player_frame_counter >= 30) {
            is_player_texture1 = !is_player_texture1; // Przełącz tekstury co 10 klatek
            player_frame_counter = 0;
        }
        player_frame_counter++;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
