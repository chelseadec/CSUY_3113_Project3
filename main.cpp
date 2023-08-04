/**
* Author: Chelsea DeCambre
* Assignment: Lunar Lander
* Date due: 2023-08-03, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 8

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <list>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
};

// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480,
          FONTBANK_SIZE = 16;

const float BG_RED     = 0.012f,
            BG_GREEN   = 0.059f,
            BG_BLUE    = 0.09f,
            
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl",
           FONT_FILEPATH[] = "/Users/chelsea/Desktop/Lunar Lander/SDLProject/assets/font1.png";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char SPRITESHEET_FILEPATH[] = "/Users/chelsea/Desktop/Lunar Lander/SDLProject/assets/spaceman.png",
           PLATFORM_FILEPATH[]    = "/Users/chelsea/Desktop/Lunar Lander/SDLProject/assets/LayeredRock.png",
           PORTAL_FILEPATH_0[]    = "/Users/chelsea/Desktop/Lunar Lander/SDLProject/assets/key.png",
           PORTAL_FILEPATH_1[]    = "/Users/chelsea/Desktop/Lunar Lander/SDLProject/assets/portalsSpriteSheet1.png",
           PORTAL_FILEPATH_2[]    = "/Users/chelsea/Desktop/Lunar Lander/SDLProject/assets/portalsSpriteSheet2.png",
           PORTAL_FILEPATH_3[]    = "/Users/chelsea/Desktop/Lunar Lander/SDLProject/assets/portalsSpriteSheet3.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;
//     g_asteriod_collided  = false,
//     g_player_landed = false;
int g_frame_counter;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int) text[i];  // ascii value of character
        float offset = (screen_size + spacing) * i;
        
        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->SetModelMatrix(model_matrix);
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello, Physics (again)!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.SetProjectionMatrix(g_projection_matrix);
    g_program.SetViewMatrix(g_view_matrix);
    
    glUseProgram(g_program.programID);
    
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
    
    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id   = load_texture(PLATFORM_FILEPATH),
           portal_texture_id_0   = load_texture(PORTAL_FILEPATH_0);
            
            
    g_state.platforms = new Entity[PLATFORM_COUNT];
    
    // Set the type of every platform entity to PLATFORM
    for (int i = 0; i < PLATFORM_COUNT - 3; i++)
    {
        
        if (i == 0) {
            g_state.platforms[i].set_position(glm::vec3(0.0f, 0.0f, 0.0f));
        }
        else if (i == 1) {
            g_state.platforms[i].set_position(glm::vec3(-3.5f, -0.80f, 0.0f));
        }
        else if (i == 2) {
            g_state.platforms[i].set_position(glm::vec3(-2.5f, 1.35f, 0.0f));
        }
        else if (i == 3) {
            g_state.platforms[i].set_position(glm::vec3(1.0f, 2.35f, 0.0f));
        }
        else if (i == 4) {
            g_state.platforms[i].set_position(glm::vec3(2.35f, -2.35f, 0.0f));
        }
        else g_state.platforms[i].set_position(glm::vec3(-3.5f, 2.35f, 0.0f));

        g_state.platforms[i].m_texture_id = platform_texture_id;
        g_state.platforms[i].set_width(0.4f);
        g_state.platforms[i].set_entity_type(PLATFORM);
        g_state.platforms[i].update(0.0f, NULL, 0);
        
    }
    
    g_state.platforms[PLATFORM_COUNT - 1].m_texture_id = platform_texture_id;
    g_state.platforms[PLATFORM_COUNT - 1].set_position(glm::vec3(-1.5f, -1.35f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 1].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 1].set_entity_type(PLATFORM);
    g_state.platforms[PLATFORM_COUNT - 1].update(0.0f, NULL, 0);

    g_state.platforms[PLATFORM_COUNT - 2].m_texture_id = platform_texture_id;
    g_state.platforms[PLATFORM_COUNT - 2].set_position(glm::vec3(2.5f, 0.5f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 2].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 2].set_entity_type(PLATFORM);
    g_state.platforms[PLATFORM_COUNT - 2].update(0.0f, NULL, 0);
    
    g_state.platforms[PLATFORM_COUNT - 3].m_texture_id = portal_texture_id_0;
    g_state.platforms[PLATFORM_COUNT - 3].set_position(glm::vec3(3.5f, 2.25f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 3].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 3].set_entity_type(TRAP);
    g_state.platforms[PLATFORM_COUNT - 3].update(0.0f, NULL, 0);
    
    // ––––– PLAYER (GEORGE) ––––– //
    // Existing
    g_state.player = new Entity();
    g_state.player->set_position(glm::vec3(-3.5f, 2.35f, 0.0f));
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->m_speed = 1.0f;
    g_state.player->set_acceleration(glm::vec3(0.2f, -0.2f, 0.0f)); // y-position is the effect of gravity on the object
    g_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
     // Walking
    g_state.player->m_walking[g_state.player->LEFT]  = new int[4] { 13, 14, 15, 16 };
    g_state.player->m_walking[g_state.player->RIGHT] = new int[4] { 9, 10, 11, 12  };
    g_state.player->m_walking[g_state.player->UP]    = new int[4] { 1, 2, 3, 4     };
    g_state.player->m_walking[g_state.player->DOWN]  = new int[4] { 5, 6, 7,  8    };

    g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];  // start George looking right
    g_state.player->m_animation_frames = 4;
    g_state.player->m_animation_index  = 0;
    g_state.player->m_animation_time   = 0.0f;
    g_state.player->m_animation_cols   = 4;
    g_state.player->m_animation_rows   = 4;
    g_state.player->set_height(0.9f);
    g_state.player->set_width(0.9f);
    
    // Jumpipiping
    g_state.player->m_jumping_power = 0.5f;
    
    // And set a trap by making one of the platforms a TRAP type
//    g_state.platforms[rand() % PLATFORM_COUNT].set_entity_type(TRAP);
    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    g_frame_counter = 0;
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->set_acceleration(glm::vec3(0.2f, -0.2f, 0.0f)); // y-position is the effect of gravity on the object
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        // Jump
                        if (g_state.player->m_collided_bottom) g_state.player->m_is_jumping = true;
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    float y_value = g_state.player->get_acceleration().y,
          x_value = g_state.player->get_acceleration().x;

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->m_movement.x = -0.5f;
        g_state.player->set_acceleration(glm::vec3(x_value - 1.0f, y_value, 0.0f));
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->LEFT];
        
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->m_movement.x = 0.5f;
//        float old_speed = g_state.player->get_speed();
//        g_state.player->set_speed(old_speed + 0.01f);
        g_state.player->set_acceleration(glm::vec3(x_value + 1.0f, 0.0f, 0.0f));
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];
    }
    else if (key_state[SDL_SCANCODE_UP])
    {
        g_state.player->m_movement.y = 0.5f;
        g_state.player->set_acceleration(glm::vec3(0.0f, y_value + 1.0f, 0.0f));
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        g_state.player->m_movement.y = -0.5f;
        g_state.player->set_acceleration(glm::vec3(0.0f, y_value - 1.0f, 0.0f));
    }

    
    if (glm::length(g_state.player->m_movement) > 1.0f)
    {
        g_state.player->m_movement = glm::normalize(g_state.player->m_movement);
    }
}

bool check_lst(int value, const std::list<int>& list) {
    for (auto i = list.begin(); i != list.end(); ++i) {
        if (*i == value) return true;
    }
    return false;
}


void update()
{
//    GLuint  portal_texture_id_0   = load_texture(PORTAL_FILEPATH_0),
//            portal_texture_id_1   = load_texture(PORTAL_FILEPATH_1),
//            portal_texture_id_2   = load_texture(PORTAL_FILEPATH_2),
//            portal_texture_id_3   = load_texture(PORTAL_FILEPATH_3);
    LOG("POSSIIIITIOOONNNNN");
    LOG(g_state.player->get_position().x);
    LOG(g_state.player->get_position().y);
    
    // keep the player in game bounds
    if (g_state.player->get_position().x > 4.80f) g_state.player->set_position(glm::vec3(4.80f,g_state.player->get_position().y, 0.0f));
    else if (g_state.player->get_position().x < - 4.80f) g_state.player->set_position(glm::vec3(-4.80f,g_state.player->get_position().y, 0.0f));
    else if (g_state.player->get_position().y >   3.35) g_state.player->set_position(glm::vec3(g_state.player->get_position().x, 3.35f, 0.0f));
    else if (g_state.player->get_position().y < - 3.35) g_state.player->set_position(glm::vec3(g_state.player->get_position().x, -3.35f, 0.0f));
    
    g_frame_counter++;
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

//    for (int i = 0; i < PLATFORM_COUNT; ++i) {
//        if (i != PLATFORM_COUNT - 3) {
//            g_state.player->check_collision(&(g_state.platforms[i])); // check if the player has collided with any asteriods
//        }
//    }
//    if (g_state.player->m_collided_bottom || g_state.player->m_collided_top || g_state.player->m_collided_left || g_state.player->m_collided_right) g_asteriod_collided = true;
//
    

//    g_state.player->check_collision(&(g_state.platforms[PLATFORM_COUNT - 3])); // check if player has reached the portal
//
//    if (!g_asteriod_collided && (g_state.player->m_collided_bottom || g_state.player->m_collided_top || g_state.player->m_collided_left || g_state.player->m_collided_right)) {g_player_landed = true;}
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, g_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }
    const std::list<int>    range0 = {0,1,2,3,4},
                            range1 = {5,6,7,8,9},
                            range2 = {10,11,12,13,14},
                            range3 = {15, 16, 17, 18, 19};
    
    int mod_val = g_frame_counter % 20;
    
//    if (check_lst(mod_val, range0)) { g_state.platforms[PLATFORM_COUNT - 3].m_texture_id = portal_texture_id_0;}
//    else if (check_lst(mod_val, range1)) {g_state.platforms[PLATFORM_COUNT - 3].m_texture_id = portal_texture_id_1;}
//    else if (check_lst(mod_val, range2)) {g_state.platforms[PLATFORM_COUNT - 3].m_texture_id = portal_texture_id_2;}
//    else if (check_lst(mod_val, range3)) {g_state.platforms[PLATFORM_COUNT - 3].m_texture_id = portal_texture_id_3;}
//

    g_accumulator = delta_time;
}

void render()
{
    GLuint font_texture_id = load_texture(FONT_FILEPATH);
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_state.player->render(&g_program);
    
    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
 
    std::list<int> range = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    for (auto i = range.begin(); i != range.end(); ++i ) {
        if ((g_frame_counter % 55) == *i){
            if (g_state.player->m_asteriod_collided) {
                draw_text(&g_program, font_texture_id, "MISSION FAILED", 0.43f, 0.00f, glm::vec3(-2.75f, 0.75f, 0.0f));}
            else if (g_state.player->m_player_landed) {draw_text(&g_program, font_texture_id, "MISSION ACCOMPLISHED", 0.43f, 0.00f, glm::vec3(-4.0f, 0.75f, 0.0f));
            }
        }
        
    }
    SDL_GL_SwapWindow(g_display_window);
    
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
