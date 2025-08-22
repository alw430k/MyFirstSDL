#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <cmath>    
#include <cstdlib>

#define SDL_WINDOW_WIDTH           1920
#define SDL_WINDOW_HEIGHT          1080
/* We will use this renderer to draw into this window every frame. */
// static SDL_Window *window = NULL;
// static SDL_Renderer *renderer = NULL;
#define NUM_ORBS 100

float playerboundsx[2] = {SDL_WINDOW_WIDTH/10, 9*SDL_WINDOW_WIDTH/10};
float playerboundsy[2] = {SDL_WINDOW_HEIGHT/10, 9*SDL_WINDOW_HEIGHT/10};
float globalx = 0;
float globaly = 0;
float globalxv = 0;
float globalyv = 0;
float mousex;
float mousey;
float mousexv;
float mouseyv;
bool mousepressed = false;
bool circleselected = false;
int circleselectedi = -1;
Uint64 last_mouse_motion_time;
Uint64 MOUSE_STOP_THRESHOLD_MS = 10;
bool mouse_is_moving = false;
int score = 0;
std::string scorestr = "Score: ";
bool uniquecolors[256][256][256];
bool combined = false;



typedef struct
{
//  unsigned char cells[(SNAKE_MATRIX_SIZE * SNAKE_CELL_MAX_BITS) / 8U];
    float xpos;
    float ypos;
    float width;
    float height;
    float xvelocity;
    float yvelocity;
    bool wasd[4];
} PlayerContext;

typedef struct
{
//  unsigned char cells[(SNAKE_MATRIX_SIZE * SNAKE_CELL_MAX_BITS) / 8U];
    float xpos;
    float ypos;
    float radius;
    float xvelocity;
    float yvelocity;
    bool mousepressed[1];
    int rgb[3];
} OrbContext;

OrbContext* currselectedcircle; 



  typedef struct
 {
     SDL_Window *window;
     SDL_Renderer *renderer;
     PlayerContext player_ctx;
     OrbContext* orb_ctx[NUM_ORBS];
     Uint64 last_step;
 } AppState;
void player_initialize(PlayerContext *ctx)
{
    SDL_zeroa(ctx->wasd);
    ctx->xpos = SDL_WINDOW_WIDTH / 2;
    ctx->ypos = SDL_WINDOW_HEIGHT / 2;
    ctx->width = 100;
    ctx->height = 100;
    ctx->xvelocity = 0;
    ctx->yvelocity = 0;
    for (int i=0; i<4; i++){
        ctx->wasd[i] = false;
    }
}


float orbspeedlimit = 0.05;

void reset_uniquecolors(){
    for(int ui = 0; ui<256; ui++){
        for(int uj = 0; uj<256; uj++){
            for(int uk = 0; uk<256; uk++){
                uniquecolors[ui][uj][uj] = false;
            }
        }
    }
}

void orb_initialize(OrbContext *octx, int xpos, int ypos)
{
    
    SDL_zeroa(octx->mousepressed);
    octx->xpos = xpos;
    octx->ypos = ypos;
    octx->radius = 20;
    octx->xvelocity = SDL_randf()* (2*orbspeedlimit - orbspeedlimit);
    octx->yvelocity = SDL_randf()* (2*orbspeedlimit - orbspeedlimit);
    for (int i=0; i<1; i++){
        octx->mousepressed[i] = false;
    }
    for (int i=0; i<3; i++){
        octx->rgb[i] = SDL_randf()*255;
    }
}

void orbs_initialize(OrbContext* octx[NUM_ORBS])
{
    // std::cout << octx << " " << &octx[0].radius;
    // for(int i=0; i<NUM_ORBS; i++){
    //     orb_initialize(&octx[i], SDL_randf()*((float)SDL_WINDOW_WIDTH), SDL_randf()*((float)SDL_WINDOW_HEIGHT));
    // }
    // OrbContext* octx[NUM_ORBS];
    for (int i=0; i<NUM_ORBS; i++){
        octx[i] = new OrbContext;
        orb_initialize(octx[i], SDL_randf()*((float)SDL_WINDOW_WIDTH), SDL_randf()*((float)SDL_WINDOW_HEIGHT));
        // octx[i] = as->orb_ctx[i];
    }
}


float accelerationfactor = float(0.01);
float resistancefactor = float(0.995);
void player_accRvelocity(PlayerContext *ctx)
{
    ctx->xvelocity += accelerationfactor;
}
void player_accLvelocity(PlayerContext *ctx)
{
    ctx->xvelocity -= accelerationfactor;
}
void player_accUvelocity(PlayerContext *ctx)
{
    ctx->yvelocity -= accelerationfactor;
}
void player_accDvelocity(PlayerContext *ctx)
{
    ctx->yvelocity += accelerationfactor;
}
void player_movementresistance(PlayerContext *ctx)
{
    
    ctx->yvelocity *= resistancefactor;
    ctx->xvelocity *= resistancefactor;
    // if(ctx->yvelocity > 0 && ctx->yvelocity >= resistancefactor){ // if y negative
    //     ctx->yvelocity -= resistancefactor;
    // }
    // else if(ctx->yvelocity < 0 && ctx->yvelocity <= (float(-1)*resistancefactor)){
    //     ctx->yvelocity += resistancefactor;
    // }
    // else{
    //     ctx->yvelocity = float(0);
    // }

    // if(ctx->xvelocity > 0 && ctx->xvelocity >= resistancefactor){ // if y negative
    //     ctx->xvelocity -= resistancefactor;
    // }
    // else if(ctx->xvelocity < 0 && ctx->xvelocity <= (float(-1)*resistancefactor)){
    //     ctx->xvelocity += resistancefactor;
    // }
    // else{
    //     ctx->xvelocity = float(0);
    // }
    
}

void update(PlayerContext *ctx, OrbContext* octx[NUM_ORBS])
{
    if(ctx->wasd[0]){
        player_accUvelocity(ctx);
    }
    if(ctx->wasd[1]){
        player_accLvelocity(ctx);
    }
    if(ctx->wasd[2]){
        player_accDvelocity(ctx);
    }
    if(ctx->wasd[3]){
        player_accRvelocity(ctx);
    }
    player_movementresistance(ctx);
    
    // Map Traversal
    if (ctx->xpos+ctx->xvelocity<playerboundsx[0]){ // hits left bounds
        ctx->xpos=playerboundsx[0];
        globalxv=ctx->xvelocity;
    }else if(ctx->xpos+ctx->width+ctx->xvelocity>playerboundsx[1]){
        ctx->xpos=playerboundsx[1]-ctx->width;
        globalxv=ctx->xvelocity;
    }else{
        ctx->xpos=ctx->xpos+ctx->xvelocity;
        globalxv=0;
    }

    if (ctx->ypos+ctx->yvelocity<playerboundsy[0]){
        ctx->ypos=playerboundsy[0];
        globalyv=ctx->yvelocity;
    }else if(ctx->ypos+ctx->height+ctx->yvelocity>playerboundsy[1]){
        ctx->ypos=playerboundsy[1]-ctx->height;
        globalyv=ctx->yvelocity;
    }else{
        ctx->ypos=ctx->ypos+ctx->yvelocity;
        globalyv=0;
    }
    globalx = globalx + globalxv;
    globaly = globaly + globalyv;

    // check collisions:
    for (int i=0; i<NUM_ORBS; i++){
        
        

        //compare radius length against 4 vertices

        //compare radius length against 4 sides
        // if circle will be directly above/directly below square, will collide
        if ((octx[i]->xpos+octx[i]->xvelocity-globalxv)>(ctx->xpos+ctx->xvelocity) && (octx[i]->xpos+octx[i]->xvelocity-globalxv)<=(ctx->xpos+ctx->width+ctx->xvelocity) ) 
        {
            //Furthemore, if circle will collide from above
            if((octx[i]->ypos+octx[i]->radius+octx[i]->yvelocity-globalyv)>=(ctx->ypos+ctx->yvelocity) && (octx[i]->ypos+octx[i]->radius+octx[i]->yvelocity-globalyv)<(ctx->ypos+ctx->height+ctx->yvelocity))
            {
                // handle y collision from above
                if(uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] && !octx[i]->mousepressed[0]){
                    octx[i]->ypos = ctx->ypos+ctx->yvelocity-(octx[i]->radius+2)-globalyv; //reset yposition to not
                    octx[i]->yvelocity = ctx->yvelocity; // set yvelovity to match square y velocity
                }
                else if (!octx[i]->mousepressed[0]){
                    score+=1;
                    uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] = true;
                    orb_initialize(octx[i], SDL_randf()*((float)SDL_WINDOW_WIDTH), SDL_randf()*((float)SDL_WINDOW_HEIGHT));
                }
                
            }
            //Furthemore, if circle will collide from above
            else if((octx[i]->ypos-octx[i]->radius+octx[i]->yvelocity-globalyv)<=(ctx->ypos+ctx->height+ctx->yvelocity) && (octx[i]->ypos+octx[i]->radius+octx[i]->yvelocity-globalyv)>(ctx->ypos+ctx->yvelocity))
            {
                // handle y collision from below
                if(uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] && !octx[i]->mousepressed[0]){
                    octx[i]->ypos = ctx->ypos+ctx->height+ctx->yvelocity+(octx[i]->radius+2)-globalyv; //reset yposition to not
                    octx[i]->yvelocity = ctx->yvelocity; // set yvelovity to match square y velocity
                }else if (!octx[i]->mousepressed[0]){
                    score+=1;
                    uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] = true;
                    orb_initialize(octx[i], SDL_randf()*((float)SDL_WINDOW_WIDTH), SDL_randf()*((float)SDL_WINDOW_HEIGHT));
                }
            }
        }
        // if circle will be directly above/directly below square, will collide
        if ((octx[i]->ypos+octx[i]->yvelocity-globalyv)>(ctx->ypos+ctx->yvelocity) && (octx[i]->ypos+octx[i]->yvelocity-globalxv)<=(ctx->ypos+ctx->height+ctx->yvelocity) ) 
        {
            //Furthemore, if circle will collide from left
            if((octx[i]->xpos+octx[i]->radius+octx[i]->xvelocity-globalxv)>=(ctx->xpos+ctx->xvelocity) && (octx[i]->xpos+octx[i]->radius+octx[i]->xvelocity-globalxv)<(ctx->xpos+ctx->width+ctx->xvelocity))
            {
                // handle y collision left
                if(uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] && !octx[i]->mousepressed[0]){
                    octx[i]->xpos = ctx->xpos+ctx->xvelocity-(octx[i]->radius+2)-globalxv; //reset yposition to not
                    octx[i]->xvelocity = ctx->xvelocity; // set yvelovity to match square y velocity
                }else if (!octx[i]->mousepressed[0]){
                    score+=1;
                    uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] = true;
                    orb_initialize(octx[i], SDL_randf()*((float)SDL_WINDOW_WIDTH), SDL_randf()*((float)SDL_WINDOW_HEIGHT));
                }
            }
            //Furthemore, if circle will collide from right
            else if((octx[i]->xpos-octx[i]->radius+octx[i]->xvelocity-globalxv)<=(ctx->xpos+ctx->width+ctx->xvelocity) && (octx[i]->xpos+octx[i]->radius+octx[i]->xvelocity-globalxv)>(ctx->xpos+ctx->xvelocity))
            {
                // handle y collision from right
                if(uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] && !octx[i]->mousepressed[0]){
                    octx[i]->xpos = ctx->xpos+ctx->height+ctx->xvelocity+(octx[i]->radius+2)-globalxv; //reset yposition to not
                    octx[i]->xvelocity = ctx->xvelocity; // set yvelovity to match square y velocity
                }else if (!octx[i]->mousepressed[0]){
                    score+=1;
                    uniquecolors[octx[i]->rgb[0]][octx[i]->rgb[1]][octx[i]->rgb[2]] = true;
                    orb_initialize(octx[i], SDL_randf()*((float)SDL_WINDOW_WIDTH), SDL_randf()*((float)SDL_WINDOW_HEIGHT));
                }
            }
        }
        
    }
    
    

}
void updateorb(OrbContext *octx)
{
    if(octx->mousepressed[0]){
        octx->xpos = mousex;
        octx->ypos = mousey;
        octx->xvelocity = 0;
        octx->yvelocity = 0;
    }else{
        octx->xpos=octx->xpos+octx->xvelocity-globalxv;
        octx->ypos=octx->ypos+octx->yvelocity-globalyv;
    }

}

int SDL_RenderFillCircle(SDL_Renderer * renderer, int x, int y, int radius)
{
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius -1;
    status = 0;

    while (offsety >= offsetx) {

        status += SDL_RenderLine(renderer, x - offsety, y + offsetx,
                                     x + offsety, y + offsetx);
        status += SDL_RenderLine(renderer, x - offsetx, y + offsety,
                                     x + offsetx, y + offsety);
        status += SDL_RenderLine(renderer, x - offsetx, y - offsety,
                                     x + offsetx, y - offsety);
        status += SDL_RenderLine(renderer, x - offsety, y - offsetx,
                                     x + offsety, y - offsetx);

        if (status < 0) {
            status = -1;
            break;
        }

        if (d >= 2*offsetx) {
            d -= 2*offsetx + 1;
            offsetx +=1;
        }
        else if (d < 2 * (radius - offsety)) {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("ColorPicker", "1.0", "com.game.colorpicker");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }


    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
     if (!as) {
         return SDL_APP_FAILURE;
     }
 
     *appstate = as;
 
     if (!SDL_CreateWindowAndRenderer("ColorPicker", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0, &as->window, &as->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError()); 
        return SDL_APP_FAILURE;
     }
     player_initialize(&as->player_ctx);
     orbs_initialize(as->orb_ctx);
     reset_uniquecolors();
 
     as->last_step = SDL_GetTicks();

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

static SDL_AppResult handle_keydown_event_(PlayerContext *ctx, SDL_Scancode key_code)
{

    SDL_Event sdlEvent;
    switch (key_code) {
    /* Quit. */
    case SDL_SCANCODE_ESCAPE:
    case SDL_SCANCODE_Q:
        return SDL_APP_SUCCESS;
    default:
        break;
    }
    
    if (key_code == SDL_SCANCODE_UP || key_code == SDL_SCANCODE_W){
        ctx->wasd[0] = true;
        std::cout << "UP Pressed\n";
    }   
    if (key_code == SDL_SCANCODE_LEFT || key_code == SDL_SCANCODE_A){
        ctx->wasd[1] = true;
        std::cout << "LEFT Pressed\n";
    }
    if(key_code == SDL_SCANCODE_DOWN || key_code == SDL_SCANCODE_S){
        ctx->wasd[2] = true;
        std::cout << "DOWN Pressed\n";
    }
    if (key_code == SDL_SCANCODE_RIGHT || key_code == SDL_SCANCODE_D){
        ctx->wasd[3] = true;
        std::cout << "RIGHT Pressed\n";
    }
    return SDL_APP_CONTINUE;
}
static SDL_AppResult handle_keyup_event_(PlayerContext *ctx, SDL_Scancode key_code)
{

    if (key_code == SDL_SCANCODE_UP || key_code == SDL_SCANCODE_W){
        ctx->wasd[0] = false;
        std::cout << "UP UNPressed\n";
    }   
    if (key_code == SDL_SCANCODE_LEFT || key_code == SDL_SCANCODE_A){
        ctx->wasd[1] = false;
        std::cout << "LEFT UNPressed\n";
    }
    if(key_code == SDL_SCANCODE_DOWN || key_code == SDL_SCANCODE_S){
        ctx->wasd[2] = false;
        std::cout << "DOWN UNPressed\n";
    }
    if (key_code == SDL_SCANCODE_RIGHT || key_code == SDL_SCANCODE_D){
        ctx->wasd[3] = false;
        std::cout << "RIGHT UNPressed\n";
    }
    return SDL_APP_CONTINUE;
}

static SDL_AppResult handle_mousedown_event_(OrbContext* octx[NUM_ORBS]){
    for (int i=NUM_ORBS-1; i>=0; i--){
        if(mousex >= octx[i]->xpos-octx[i]->radius && mousex <= octx[i]->xpos+octx[i]->radius && mousey >= octx[i]->ypos-octx[i]->radius && mousey <= octx[i]->ypos+octx[i]->radius){
            octx[i]->mousepressed[0] = true;
            circleselected = true;
            circleselectedi = i;
            currselectedcircle = octx[i];
            break;
            // octx[i]->xvelocity = 0;
            // octx[i]->yvelocity = 0;
        }
    }
    return SDL_APP_CONTINUE;
}
static SDL_AppResult handle_mouseup_event_(OrbContext* octx[NUM_ORBS]){
    if(currselectedcircle->mousepressed[0]){
        currselectedcircle->mousepressed[0] = false;
        circleselected = false;
        for (int i=NUM_ORBS-1; i>=0; i--){
            if(circleselectedi!=i && mousex >= octx[i]->xpos-octx[i]->radius && mousex <= octx[i]->xpos+octx[i]->radius && mousey >= octx[i]->ypos-octx[i]->radius && mousey <= octx[i]->ypos+octx[i]->radius){
                // std::cout << "mouse in circle!";
                if(octx[i]->rgb[0] + currselectedcircle->rgb[0]>255){octx[i]->rgb[0]=255;}
                else{octx[i]->rgb[0] = octx[i]->rgb[0] + currselectedcircle->rgb[0];}
                if(octx[i]->rgb[1] + currselectedcircle->rgb[1]>255){octx[i]->rgb[1]=255;}
                else{octx[i]->rgb[1] = octx[i]->rgb[1] + currselectedcircle->rgb[1];}
                if(octx[i]->rgb[2] + currselectedcircle->rgb[2]>255){octx[i]->rgb[2]=255;}
                else{octx[i]->rgb[2] = octx[i]->rgb[2] + currselectedcircle->rgb[2];}
                combined=true;
                orb_initialize(currselectedcircle, SDL_randf()*((float)SDL_WINDOW_WIDTH), SDL_randf()*((float)SDL_WINDOW_HEIGHT));
                
                break;
            }
        }
        if(!combined){
            currselectedcircle->xvelocity = mousexv/3 + globalxv;
            currselectedcircle->yvelocity = mouseyv/3 + globalyv;
        }
        circleselectedi = -1;
        combined=false;
        
    }
    
    
    
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    PlayerContext *ctx = &((AppState *)appstate)->player_ctx;
    OrbContext* octx[NUM_ORBS];
    for (int i=0; i<NUM_ORBS; i++){
        octx[i] = new OrbContext;
        octx[i] = ((AppState *)appstate)->orb_ctx[i];
        // octx[i] = as->orb_ctx[i];
    }   
    switch (event->type) {
    case SDL_EVENT_QUIT: 
        return SDL_APP_SUCCESS; /* end the pxrogram, reporting success to the OS. */
    // case SDL_EVENT_KEY_DOWN:
    //     return handle_key_event_(ctx, event->key.scancode);
    case SDL_EVENT_MOUSE_MOTION: {
            SDL_MouseID id = event->motion.which;
            last_mouse_motion_time = SDL_GetTicks(); // Or increment a frame counter
            mouse_is_moving = true;
            mousex = event->button.x;
            mousey = event->button.y;
            mousexv = event->motion.xrel;
            mouseyv = event->motion.yrel;
            if(mousepressed){
                std::cout << event->motion.xrel<< ' '<< event->motion.yrel <<' '<< event->button.x <<' '<< event->button.y << '\n';
            }
            
            
            break;
        }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        SDL_MouseID id = event->button.which;
        
        if (event->button.which==0){
            mousepressed = true;
            std::cout << event->button.x<< ' '<<event->button.y << '\n';
            handle_mousedown_event_(octx);
        }
        
        // int index = whoseMouse(id, players, player_count);
        // if (index >= 0) {
        //     shoot(index, players, player_count);
        // }
        break;
    }
    
    case SDL_EVENT_MOUSE_BUTTON_UP:{
        SDL_MouseID id = event->button.which;
        if (!mouse_is_moving) {
            mousexv = 0;
            mouseyv = 0;
        }
        mousex = event->button.x;
        mousey = event->button.y;
        handle_mouseup_event_(octx);
        if (event->button.which==0){
            mousepressed = false;
            std::cout << event->button.x<< ' '<<event->button.y << '\n';
        }
        break;
    }
    case SDL_EVENT_KEY_DOWN: {
        SDL_Keycode sym = event->key.key;
        SDL_KeyboardID id = event->key.which;
        handle_keydown_event_(ctx, event->key.scancode);
        
        // if (sym == SDLK_W){ctx[0]->wasd=true};
        // if (sym == SDLK_A) ctx[1]->wasd |= 2;
        // if (sym == SDLK_S) ctx[2]->wasd |= 4;
        // if (sym == SDLK_D) ctx[4]->wasd |= 8;
        
        break;
        }
    case SDL_EVENT_KEY_UP: {
        SDL_Keycode sym = event->key.key;
        SDL_KeyboardID id = event->key.which;
        if (sym == SDLK_ESCAPE) return SDL_APP_SUCCESS;
        handle_keyup_event_(ctx, event->key.scancode);
        
        // if (sym == SDLK_W) ctx->wasd &= 30;
        // if (sym == SDLK_A) ctx->wasd &= 29;
        // if (sym == SDLK_S) ctx->wasd &= 27;
        // if (sym == SDLK_D) ctx->wasd &= 23;
        // if (sym == SDLK_SPACE) ctx->wasd &= 15;
        
        break;
    }
    default:
        break;
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
bool doonce = true;
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *as = (AppState *)appstate;
    PlayerContext *ctx = &as->player_ctx;
    // OrbContext *octx = &as->orb_ctx;
    OrbContext* octx[NUM_ORBS];
    for (int i=0; i<NUM_ORBS; i++){
        octx[i] = new OrbContext;
        octx[i] = as->orb_ctx[i];
        // octx[i] = as->orb_ctx[i];
    }
    
    static Uint64 accu = 0;
    static Uint64 last = 0;
    static Uint64 past = 0;
    Uint64 now = SDL_GetTicksNS();
    Uint64 dt_ns = now - past;
    SDL_FRect r;
    unsigned i;
    unsigned j;
    int ct;
    // const double now = ((double)SDL_GetTicks()) / 1000.0;  /* convert from milliseconds to seconds. */
    // /* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
    // const float red = (float) (0.5 + 0.5 * SDL_sin(now));
    // const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    // const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    // DRAW BACKGROUND
    SDL_SetRenderDrawColor(as->renderer, 30, 30, 35, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
    /* clear the window to the draw color. */
    SDL_RenderClear(as->renderer);

    

    // DRAW THE ORBS
    if(doonce){
        std::cout << int(octx[0]->xpos) <<  ' ' << int(octx[0]->xpos) <<  ' '  << octx[0]->radius <<  ' ' << octx[0]->rgb[0] <<  ' ' << octx[0]->rgb[1] <<  ' ' << octx[0]->rgb[2] <<  ' ';
        doonce = false;
    }
    // SDL_SetRenderDrawColor(as->renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    // SDL_RenderFillCircle(as->renderer, 0, 0, 300);
    for (int j=0; j<NUM_ORBS; j++){
        updateorb(octx[j]);
        SDL_SetRenderDrawColor(as->renderer, int(octx[j]->rgb[0]), int(octx[j]->rgb[1]), int(octx[j]->rgb[2]), SDL_ALPHA_OPAQUE);
        SDL_RenderFillCircle(as->renderer, int(octx[j]->xpos), int(octx[j]->ypos), int(octx[j]->radius));
    }


    // DRAW THE PLAYER
    SDL_SetRenderDrawColor(as->renderer, 250, 250, 255, SDL_ALPHA_OPAQUE);  /* blue, full alpha */
    update(ctx, octx);
    
    SDL_FRect rect;
    rect.x = ctx->xpos;
    rect.y = ctx->ypos;
    rect.w = ctx->width;
    rect.h = ctx->height;
    SDL_RenderFillRect(as->renderer, &rect);

    SDL_SetRenderDrawColor(as->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */
    SDL_SetRenderScale(as->renderer, 2.0f, 2.0f);
    SDL_RenderDebugText(as->renderer, 10, 10, (scorestr + std::to_string(score)).c_str());
    SDL_SetRenderScale(as->renderer, 1.0f, 1.0f);

    if (mousepressed and circleselected){
        // SDL_SetRenderDrawColor(as->renderer, currselectedcircle->rgb[0], currselectedcircle->rgb[1], currselectedcircle->rgb[2], SDL_ALPHA_OPAQUE);  /* white, full alpha */
        SDL_SetRenderScale(as->renderer, 1.0f, 1.0f);
        SDL_SetRenderDrawColor(as->renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);  /* white, full alpha */
        SDL_RenderDebugText(as->renderer, mousex-37, mousey-currselectedcircle->radius-10, (std::to_string(currselectedcircle->rgb[0])).c_str());
        SDL_SetRenderDrawColor(as->renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);  /* white, full alpha */
        SDL_RenderDebugText(as->renderer, mousex-12, mousey-currselectedcircle->radius-10, (std::to_string(currselectedcircle->rgb[1])).c_str());
        SDL_SetRenderDrawColor(as->renderer, 0, 0, 255, SDL_ALPHA_OPAQUE);  /* white, full alpha */
        SDL_RenderDebugText(as->renderer, mousex+13, mousey-currselectedcircle->radius-10, (std::to_string(currselectedcircle->rgb[2])).c_str());
        SDL_SetRenderScale(as->renderer, 1.0f, 1.0f);
    }
    

    

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(as->renderer);

    if (now - last > 999999999) {
        last = now;
        // SDL_snprintf(debug_string, sizeof(debug_string), "%" SDL_PRIu64 " fps", accu);
        accu = 0;
    }
    past = now;
    accu += 1;
    Uint64 elapsed = SDL_GetTicksNS() - now;
    if (elapsed < 999999) {
        SDL_DelayNS(999999 - elapsed);
    }
    if (mouse_is_moving && (SDL_GetTicks() - last_mouse_motion_time > MOUSE_STOP_THRESHOLD_MS)) {
        // Mouse has stopped moving
        mouse_is_moving = false;
        // Perform actions when mouse stops, e.g., hide a UI element
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}