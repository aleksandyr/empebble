#ifdef EMSCRIPTEN

#include "pebble_os.h"
#include "empebble_js.h"
#include "SDL_prims.h"

#define PI 3.14159265

#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <emscripten.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))


extern void pbl_main(void *params);

static PebbleAppHandlers _PebbleAppHandlers;
static time_t _then;

SDL_Surface *screen;

#define LOCK(X) if(SDL_MUSTLOCK(X)) SDL_LockSurface(X)
#define UNLOCK(X) if(SDL_MUSTLOCK(X)) SDL_UnlockSurface(X)

#define min(A, B) (((A) < (B) ? (A) : (B)))
#define max(A, B) (((A) > (B) ? (A) : (B)))
#define clamp(A, X, B)  min(max(A, X), B)

struct GContext {
  GColor stroke_color;
  GColor fill_color;
  GColor text_color;
  GCompOp  compositing_mode;
};
                         //AABBGGRR
const uint32_t r_white = 0xFFFFFFFF;
const uint32_t r_black = 0xFF000000;
const uint32_t r_clear = 0x00000000;

Window *current_window = NULL;

GContext current_graphics_context;

uint32_t getRawColor(uint8_t color) {
    switch(color) {
    case 0:
      return r_black;
    case 1:
      return r_white;
    default:
      return r_clear;
  }
}

SDL_Color s_white = {255, 255, 255, SDL_ALPHA_OPAQUE};
SDL_Color s_black = {0, 0, 0, SDL_ALPHA_OPAQUE};
SDL_Color s_clear = {255, 255, 255, SDL_ALPHA_TRANSPARENT};

SDL_Color getColor(uint8_t color) {
  switch(color) {
    case 0:
      return s_black;
    case 1:
      return s_white;
    default:
      return s_clear;
  }
}

// This is really stupid, but I can't think of a better way to do it.
// The problem is that there is no way to keep context with a Layer,
// so we can't write generic layer update_proc. So we here keep a list
// of all of the text_layers that have been created and iterate
// through them to find the one we're currently trying to draw.
TextLayer* text_layers[200];

int main() {
  printf("[INFO] Entering main\n");
  SDL_Init(SDL_INIT_VIDEO);
  screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
  if(screen == NULL) {
    printf("SDL_SetVideoMode failed!\n");
    return 1;
  }

  if(TTF_Init()==-1) {
    printf("TTF_Init: %s\n", TTF_GetError());
    return 2;
  }

  //SDL_Flip(screen); 

  pbl_main(NULL);
  return 0;
}

void tick() {
  time_t now = time(0);

  switch(_PebbleAppHandlers.tick_info.tick_units) {
    case SECOND_UNIT:
      if(_then == now) return;
      break;
    case MINUTE_UNIT:
      if(_then/60 == now/60) return;
      break;
    case HOUR_UNIT:
      if(_then/(60*60) == now/(60*60)) return;
      break;
    case DAY_UNIT:
      printf("Unsupported tick unit: DAY_UNIT");
      return;
    case MONTH_UNIT:
      printf("Unsupported tick unit: MONTH_UNIT");
      return;
    case YEAR_UNIT:
      printf("Unsupported tick unit: YEAR_UNIT");
      return;
    default:
      printf("Unknown tick unit: %d\n", _PebbleAppHandlers.tick_info.tick_units);
      return;
  }
  _then = now;

  (_PebbleAppHandlers.tick_info.tick_handler)(NULL, NULL);
}

void handle_buttons();

void loop() {
  //_emscripten_push_main_loop_blocker(tick, NULL, "tick handler");
  if(_PebbleAppHandlers.tick_info.tick_handler != NULL)
    tick();
  handle_buttons();
  SDL_Flip(screen);
}

//#todo #sprint2
void animation_init(struct Animation *animation);

//#todo #sprint2
void animation_set_delay(struct Animation *animation, uint32_t delay_ms);

//#todo #sprint2
void animation_set_duration(struct Animation *animation, uint32_t duration_ms);

//#todo #sprint2
void animation_set_curve(struct Animation *animation, AnimationCurve curve);

//#todo #sprint2
void animation_set_handlers(struct Animation *animation, AnimationHandlers callbacks, void *context);

//#todo #sprint2
void animation_set_implementation(struct Animation *animation, const AnimationImplementation *implementation);

//#todo #sprint2
void *animation_get_context(struct Animation *animation);

//#todo #sprint2
void animation_schedule(struct Animation *animation);

//#todo #sprint2
void animation_unschedule(struct Animation *animation);

//#todo #sprint2
void animation_unschedule_all(void);

//#todo #sprint2
bool animation_is_scheduled(struct Animation *animation);

//#todo #sprint2
AppTimerHandle app_timer_send_event(AppContextRef app_ctx, uint32_t timeout_ms, uint32_t cookie);

//#todo #sprint2
bool app_timer_cancel_event(AppContextRef app_ctx_ref, AppTimerHandle handle);

void app_event_loop(AppTaskContextRef app_task_ctx, PebbleAppHandlers *handlers) {
  printf("[DEBUG] Got app_event_loop\n");
  _PebbleAppHandlers = *handlers;
  (_PebbleAppHandlers.init_handler)(NULL);

  if(_PebbleAppHandlers.tick_info.tick_handler != NULL)
    (_PebbleAppHandlers.tick_info.tick_handler)(NULL, NULL);
  _then = time(0);

  printf("[DEBUG] Passing control to main loop\n");
  emscripten_set_main_loop(loop,30,0);
}

void app_log(uint8_t log_level, const char *src_filename, int src_line_number, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[256];
  vsnprintf(buffer, 256, fmt, args);
  va_end(args);
  char *level;
  switch((AppLogLevel)log_level) {
  case APP_LOG_LEVEL_ERROR:
    level = "ERROR";
    break;
  case APP_LOG_LEVEL_WARNING:
    level = "WARNING";
    break;
  case APP_LOG_LEVEL_INFO:
    level = "INFO";
    break;
  case APP_LOG_LEVEL_DEBUG:
    level = "DEBUG";
    break;
  case APP_LOG_LEVEL_DEBUG_VERBOSE:
    level = "VERBOSE";
    break;
  default:
    level = "UNKNOWN";
  }
  printf("[%s] %s:%d %s\n", level, src_filename, src_line_number, buffer);
}

//#todo #sprint1
bool bmp_init_container(int resource_id, BmpContainer *c);

//#todo #sprint1
void bmp_deinit_container(BmpContainer *c);

int32_t cos_lookup(int32_t angle) {
  return cos(angle*PI/180)*0xFFFF;
}

#include "emscripten.pebble_fonts.h"

GFont fonts_get_system_font(const char *font_key) {
  GFont output = NULL;
  if(strcmp(FONT_KEY_FONT_FALLBACK, font_key) == 0) {

  } else if(strcmp(FONT_KEY_GOTHIC_14, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic.ttf", 14);
  } else if(strcmp(FONT_KEY_GOTHIC_14_BOLD, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic-bold.ttf", 14);
  } else if(strcmp(FONT_KEY_GOTHIC_18, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic.ttf", 18);
  } else if(strcmp(FONT_KEY_GOTHIC_18_BOLD, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic-bold.ttf", 18);
  } else if(strcmp(FONT_KEY_GOTHIC_24, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic.ttf", 24);
  } else if(strcmp(FONT_KEY_GOTHIC_24_BOLD, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic-bold.ttf", 24);
  } else if(strcmp(FONT_KEY_GOTHIC_28, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic.ttf", 28);
  } else if(strcmp(FONT_KEY_GOTHIC_28_BOLD, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gothic-bold.ttf", 28);
  } else if(strcmp(FONT_KEY_GOTHAM_30_BLACK, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gotham-black.ttf", 30);
  } else if(strcmp(FONT_KEY_GOTHAM_42_BOLD, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gotham-bold.ttf", 42);
  } else if(strcmp(FONT_KEY_GOTHAM_42_LIGHT, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gotham-light.ttf", 42);
  } else if(strcmp(FONT_KEY_GOTHAM_42_MEDIUM_NUMBERS, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gotham-medium-numbers.ttf", 42);
  } else if(strcmp(FONT_KEY_GOTHAM_34_MEDIUM_NUMBERS, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gotham-medium-numbers.ttf", 32);
  } else if(strcmp(FONT_KEY_GOTHAM_34_LIGHT_SUBSET, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gotham-light-subset.ttf", 34);
  } else if(strcmp(FONT_KEY_GOTHAM_18_LIGHT_SUBSET, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/gotham-light-subset.ttf", 18);
  } else if(strcmp(FONT_KEY_DROID_SERIF_28_BOLD, font_key) == 0) {
    output = TTF_OpenFont("../resources/font/droid-serif-bold.ttf", 28);
  } else {
    printf("[ERROR] Invalid system font %s\n", font_key);
  }

  return output;
}

GFont fonts_load_custom_font(ResHandle resource) {
  //TODO: Fix
  /* char *font_name = resource->path; */
  /* char path[128] = "../resources/font/"; */
  /* strncat(path, font_name, 128); */
  /* uint8_t font_size = resource->font_size; */
  /* return TTF_OpenFont(path, font_size); */
  return NULL;
}

//#nosdk
void fonts_unload_custom_font(GFont font) {
  printf("[WARN] [NOOP] fonts_unload_custom_font\n");
}

//TODO: We're currently using a single graphics context.
//      This is probably accurate.
void graphics_context_set_stroke_color(GContext *ctx, GColor color) {
  ctx->stroke_color = color;
}

void graphics_context_set_fill_color(GContext *ctx, GColor color) { 
  ctx->fill_color = color;
}

void graphics_context_set_text_color(GContext *ctx, GColor color) {
  ctx->text_color = color;
}

void graphics_context_set_compositing_mode(GContext *ctx, GCompOp mode) {
  ctx->compositing_mode = mode;
}

void set_pixel(SDL_Surface *surface, int x, int y, uint8_t color)
{
    LOCK(surface);
    Uint8 *target_pixel = (Uint8 *)(surface->pixels + y * surface->pitch + x * 4);
    *(Uint32 *)target_pixel = getRawColor(color);
    UNLOCK(surface);
}

void graphics_draw_pixel(GContext *ctx, GPoint point) {
  LOCK(screen);
  SDL_DrawPixel(screen, point.x, point.y, getRawColor(ctx->stroke_color));
  UNLOCK(screen);

}

void graphics_draw_line(GContext *ctx, GPoint p0, GPoint p1) {
  printf("Drawing line with color: %x\n", getRawColor(ctx->stroke_color));
  LOCK(screen);
  SDL_DrawLine(screen, p0.x, p0.y, p1.x, p1.y, getRawColor(ctx->stroke_color));
  UNLOCK(screen);
}

void graphics_fill_rect(GContext *ctx, GRect rect, uint8_t corner_radius, GCornerMask corner_mask) {
  //TODO: corner_radius and corner_mask
  //TODO: is stroke color meaningful?
  uint32_t color = getRawColor(ctx->fill_color);
  SDL_Rect srect = {rect.origin.x, rect.origin.y, rect.size.w, rect.size.h};
  SDL_FillRect(screen, &srect, color);
}

void graphics_draw_circle(GContext *ctx, GPoint p, int radius) {
  LOCK(screen);
  SDL_DrawCircle(screen, p.x, p.y, radius, r_white);
  UNLOCK(screen);
}

void graphics_fill_circle(GContext *ctx, GPoint p, int radius) {
  LOCK(screen);
  SDL_FillCircle(screen, p.x, p.y, radius, getRawColor(ctx->fill_color));
  UNLOCK(screen);
}

//#nosdk
void graphics_draw_round_rect(GContext *ctx, GRect rect, int radius);

void get_time(PblTm *_time) {
  time_t raw;
  struct tm *now;

  time(&raw);
  now = localtime(&raw);

  _time->tm_sec = now->tm_sec;
  _time->tm_min = now->tm_min;
  _time->tm_hour = now->tm_hour;
  _time->tm_mday = now->tm_mday;
  _time->tm_mon = now->tm_mon;
  _time->tm_year = now->tm_year;
  _time->tm_wday = now->tm_wday;
  _time->tm_yday = now->tm_yday;
  _time->tm_isdst = now->tm_isdst;
}

void gpath_init(GPath *path, const GPathInfo *init) {
  path->num_points = init->num_points;
  path->points = init->points;
}

void gpath_move_to(GPath *path, GPoint point) {
  path->offset = point;
}

void gpath_rotate_to(GPath *path, int32_t angle) {
  path->rotation = angle;
}

SDL_Point *_gpath_to_sdl(GPath *path) {
  SDL_Point *points = malloc(sizeof(SDL_Point) * path->num_points);

  double s = sin(path->rotation * PI / 180);
  double c = cos(path->rotation * PI / 180);

  for(int p=0;p<path->num_points;p++) {
    double x, y, nx;
    x = path->points[p].x+0.5;
    y = path->points[p].y+0.5;
    nx = x * c - y * s;
    y = x * s + y * c;
    x = nx;
    x += path->offset.x;
    y += path->offset.y;

    points[p].x = x;
    points[p].y = y;
  }

  return points;
}

void gpath_draw_outline(GContext *ctx, GPath *path) {
  LOCK(screen);
  SDL_Point *points = _gpath_to_sdl(path);
  SDL_DrawPolygon(screen, points, path->num_points, getRawColor(ctx->stroke_color));
  free(points);
  UNLOCK(screen);
}

void gpath_draw_filled(GContext *ctx, GPath *path) {
  LOCK(screen);
  SDL_Point *points = _gpath_to_sdl(path);
  SDL_FillPolygon(screen, points, path->num_points, getRawColor(ctx->fill_color));
  free(points);
  UNLOCK(screen);
}

GPoint grect_center_point(GRect *rect) {
  GPoint output;
  output.x = rect->origin.x + (rect->size.w)/2;
  output.y = rect->origin.y + (rect->size.h)/2;

  return output;
}

void layer_mark_dirty(Layer *layer) {
  (layer->update_proc)(layer, &current_graphics_context);
}

void layer_remove_from_parent(Layer *child) {
  Layer *cursor = child->parent->first_child;

  if(cursor == child) {
    child->parent->first_child = child->next_sibling;
  } else {
    while(cursor->next_sibling != child) {
      cursor = cursor->next_sibling;
    }
    cursor->next_sibling = child->next_sibling;
  }

  child->parent = NULL;
}

void layer_add_child(Layer *parent, Layer *child) {
  if(parent->first_child == NULL) {
    parent->first_child = child;
  } else {
    parent = parent->first_child;
    while(parent->next_sibling != NULL) {
      parent = parent->next_sibling;
    }
    parent->next_sibling = child;
  }
}

//#trivial
GRect layer_get_frame(Layer *layer) {
  return layer->frame;
}

//#trivial
void layer_set_frame(Layer *layer, GRect frame) {
  layer->frame = frame;
}

//#trivial
void layer_set_hidden(Layer *layer, bool hidden) {
  layer->hidden = hidden;
}

//#verify
void layer_init(Layer *layer, GRect frame) {
  //TODO: implement?
  layer->frame = frame;
}

void light_enable(bool enable) {
  printf("[INFO] Backlight: %s\n", (enable ? "on" : "off"));
  //TODO: Emscripten callback to add a glow indicator?
}

void light_enable_interaction(void) {
  printf("[INFO] Backlight should be turned on for 2 seconds.\n");
  //TODO: Callback, timer, etc.
}

void psleep(int millis) {
  //TODO: How does this interact with JS?
  usleep((uint32_t)millis*1000);
}

//NOTE: This is inferred from the layout of a pbpack.
typedef struct {
  uint32_t index;
  uint32_t offset;
  uint32_t size;
  uint32_t crc;
} eResHandle;

uint32_t resource_count = 0;
eResHandle *resource_table = NULL;
uint32_t resources_size = 0;
char *resources;

void resource_init_current_app(ResVersionHandle version) {
  printf("[INFO] resource_init_current_app\n");
  printf("[DEBUG] Friendly version \"%s\"\n", version->friendly_version );
  printf("[DEBUG] CRC %x TIMESTAMP %x\n", version->crc, version->timestamp);

  if(resource_table != NULL) {
    printf("[ERROR] Attempt to double-init resources; aborting\n");
    return;
  }

  FILE *resFile;
  uint32_t result;
  ResBankVersion fVersion;

  resFile = fopen("/app_resources.pbpack", "rb");
  if(resFile == NULL) {
    printf("[ERROR] Unable to load app_resources.pbpack\n");
    return;
  }

  result = fread(&resource_count, sizeof(uint32_t), 1, resFile);
  if(result != 1) {
    printf("[ERROR] Unable to read resource table count from resource file\n");
    return;
  }

  result = fread(&fVersion, sizeof(ResBankVersion), 1, resFile);
  if(result != 1) {
    printf("[ERROR] Unable to read resource table header from resource file\n");
    return;
  }

  if(fVersion.crc != version->crc ||
     fVersion.timestamp != version->timestamp ||
     strcmp(fVersion.friendly_version,version->friendly_version) != 0) {
    printf("[ERROR] Resource header mismatch; Pebble will not load this resource pack\n");
    printf("[DEBUG] Friendly version \"%s\"\n", fVersion.friendly_version );
    printf("[DEBUG] CRC %x TIMESTAMP %x\n", fVersion.crc, fVersion.timestamp);
    return;
  }


  resource_table = (eResHandle*)malloc(sizeof(eResHandle)*resource_count);

  result = fread(resource_table, sizeof(eResHandle), resource_count, resFile);

  if(result != resource_count) {
    printf("[ERROR] Tried to load %u resource table entries but got %u\n", resource_count, result);
    return;
  }

  //Here, we (dangerously) assume that resources are dense-packed and that the first index is 1.
  //This is going to be *generally* true but should be eventually refactored.
  for(int i=0;i<resource_count;i++) {
    printf("[DEBUG] I:%d O:%d S:%d C:%x\n", resource_table[i].index, resource_table[i].offset, resource_table[i].size, resource_table[i].crc);
    if(resource_table[i].index != i+1) {
      printf("[ERROR] Resources were not dense and sequential.");
    }
  }

  //TODO: (Ab)use the fact that there are at most 256 resources at a given time.
  fseek(resFile, 256*sizeof(eResHandle) + sizeof(uint32_t) + sizeof(ResBankVersion), SEEK_SET);
  uint32_t resource_start = ftell(resFile);
  fseek(resFile, 0, SEEK_END);
  uint32_t resource_end = ftell(resFile);
  fseek(resFile, 256*sizeof(eResHandle) + sizeof(uint32_t) + sizeof(ResBankVersion), SEEK_SET);

  resources = (char *)malloc(sizeof(char)*(resource_end-resource_start));

  result = fread(resources, sizeof(char), resource_end-resource_start, resFile);

  resources_size = result;

  if(result != (resource_end-resource_start)) {
    printf("[ERROR] Tried to read %d bytes of resources but got %d\n", resource_end-resource_start, result);
    return;
  }

  printf("[DEBUG] Loaded %u resources (%u bytes)\n", resource_count, result);
  
}

//#todo #sprint1
ResHandle resource_get_handle(uint32_t file_id) {
  printf("[INFO] Fetching resource handle %d\n", file_id);
  if(resource_table == NULL) {
    printf("[ERROR] Attempt to read resources before resources were initialized\n");
    return NULL;
  }

  return &resource_table[file_id-1];
}

size_t resource_load(ResHandle h, uint8_t *buffer, size_t max_length) {
  return resource_load_byte_range(h, 0, buffer, max_length);
}

size_t resource_load_byte_range(ResHandle h, uint32_t start_bytes, uint8_t *data, size_t num_bytes) {
  printf("[INFO] resource_load_byte_range\n");
  size_t readBytes = 0;

  eResHandle *e = (eResHandle *)h;

  if(start_bytes >= e->size) {
    printf("[ERROR] Attempt to start reading past the end of a resource; truncating\n");
    start_bytes = e->size-1;
  }

  if(start_bytes+num_bytes > e->size) {
    printf("[ERROR] Attempt to read past the end of a resource; truncating\n");
    num_bytes = e->size - start_bytes;
  }

  start_bytes += e->offset;

  memcpy(data, resources+start_bytes, num_bytes);

  return num_bytes;
}

//#nosdk
size_t resource_size(ResHandle h);

//#todo #sprint1
void rotbmp_deinit_container(RotBmpContainer *c);

//#todo #sprint1
bool rotbmp_init_container(int resource_id, RotBmpContainer *c);

//#todo #sprint1
void rotbmp_pair_deinit_container(RotBmpPairContainer *c);

//#todo #sprint1
bool rotbmp_pair_init_container(int white_resource_id, int black_resource_id, RotBmpPairContainer *c);

//#todo #sprint1
void rotbmp_pair_layer_set_src_ic(RotBmpPairLayer *pair, GPoint ic);

//#todo #sprint1
void rotbmp_pair_layer_set_angle(RotBmpPairLayer *pair, int32_t angle);

void window_init(Window *window, const char *debug_name) {
  //TODO: Implement the rest of this.
  printf("[INFO] Loading window: %s\n", debug_name);
  window->layer.frame.origin.x = 0;
  window->layer.frame.origin.y = 0;
  window->layer.frame.size.w = 144;
  window->layer.frame.size.h = 168;
}

void window_stack_push(Window *window, bool animated) {
  //TODO: Implement
  printf("[WARN] [NOOP] window_stack_push\n");
  current_window = window;

  // update the window -- this may not be in the correct place
  if(current_window) {
    layer_mark_dirty(&window->layer);
  }

}

void window_set_click_config_provider(Window *window, ClickConfigProvider click_config_provider) {
  window->click_config_provider = click_config_provider;
}

void window_set_background_color(Window *window, GColor background_color) {
  uint32_t color = getRawColor(background_color);

  SDL_FillRect(screen, NULL, color);
}

void window_render(Window *window, GContext *ctx) {
  printf("[WARN] [NOOP] window_render\n");
}

void window_set_fullscreen(Window *window, bool enabled) {
  window->is_fullscreen = enabled;

  //TODO: Does this cause a relayout?
}

struct Layer *window_get_root_layer(Window *window) {
  return &window->layer;
}


int32_t sin_lookup(int32_t angle) {
  //TODO: Rewrite this to use 0x10000 as max_angle and not 360
  return sin(angle*PI/180)*0xFFFF;
}

void make_time(const PblTm *now, struct tm *time) {
  time->tm_sec = now->tm_sec;
  time->tm_min = now->tm_min;
  time->tm_hour = now->tm_hour;
  time->tm_mday = now->tm_mday;
  time->tm_mon = now->tm_mon;
  time->tm_year = now->tm_year;
  time->tm_wday = now->tm_wday;
  time->tm_yday = now->tm_yday;
  time->tm_isdst = now->tm_isdst;
}

void string_format_time(char *ptr, size_t maxsize, const char *format, const PblTm *timeptr) {
  struct tm tm_time;
  make_time(timeptr, &tm_time);
  mktime(&tm_time);

  size_t actual = strftime(ptr, maxsize, format, &tm_time);
  if(actual == 0)
    printf("[WARN] strftime failed - %d:%s\n", maxsize, format);

}

void draw_text(TextLayer *text_layer) {
  SDL_Surface *text_surface;
  SDL_Color bgcolor = getColor(text_layer->background_color);
  if(!(text_surface = TTF_RenderText(text_layer->font, text_layer->text,
                                     getColor(text_layer->text_color),
                                     bgcolor))) {
    printf("[ERROR] TTF_RenderText_Solid: %s\n", TTF_GetError());
  } else {
    SDL_Rect dst;

    dst.x = text_layer->layer.frame.origin.x;
    dst.y = text_layer->layer.frame.origin.y;

    // handle center and right alignment
    int w, h;

    TTF_SizeText(text_layer->font, text_layer->text, &w, &h);
    if (text_layer->text_alignment == GTextAlignmentCenter) {
      dst.x = dst.x + (text_layer->layer.frame.size.w - w) / 2;
    }
    else if(text_layer->text_alignment == GTextAlignmentRight) {
      dst.x = dst.x + text_layer->layer.frame.size.w - w;
    }

    // draw text
    if(bgcolor.unused == 255) {
      GRect bgframe = text_layer->layer.frame;
      SDL_Rect bgrect = {bgframe.origin.x, bgframe.origin.y, bgframe.size.w, bgframe.size.h};
      SDL_FillRect(screen, &bgrect, SDL_MapRGBA(
                     screen->format, bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.unused));
    }
    SDL_BlitSurface(text_surface, NULL, screen, &dst);
    SDL_FreeSurface(text_surface);
  }
}


void text_layer_update(Layer *layer, GContext *ctx) {
  for (size_t i = 0; i < sizeof(text_layers); i++) {
    if(text_layers[i] != NULL && &text_layers[i]->layer == layer) {
      draw_text(text_layers[i]);
      return;
    }
  }
  printf("[ERROR] Could not find text layer");
}

void text_layer_init(TextLayer *text_layer, GRect frame) {
  text_layer->layer.frame = frame;
  text_layer->layer.update_proc = text_layer_update;

  //Defaults taken from the SDK's documentation.
  text_layer->font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  text_layer->text_alignment = GTextAlignmentLeft;
  text_layer->text_color = 0;
  text_layer->background_color = 1;
  text_layer->overflow_mode = GTextOverflowModeWordWrap;
  text_layer->should_cache_layout = false;

  for(size_t i = 0; i < sizeof(text_layers); i++) {
    if (text_layers[i] == NULL) {
      text_layers[i] = text_layer;
      return;
    }
  }
  
  printf("[ERROR] Too many text layers (only 200 supported)");
}

const char *text_layer_get_text(TextLayer *text_layer) { 
  return text_layer->text;
}

void text_layer_set_text(TextLayer *text_layer, const char *text) {
  text_layer->text = text;
  draw_text(text_layer);
}

void text_layer_set_font(TextLayer *text_layer, GFont font) { 
  text_layer->font = font;
}

void text_layer_set_text_color(TextLayer *text_layer, GColor color) {
  text_layer->text_color = color;
}

void text_layer_set_background_color(TextLayer *text_layer, GColor color) {
  text_layer->background_color = color;
}

void vibes_double_pulse(void) {
  printf("[INFO] Vibration: double pulse\n");
  //TODO: Emscripten callback to shake screen?
}

void vibes_enqueue_custom_pattern(VibePattern pattern) {
  printf("[INFO] Vibration: ");
  const uint32_t *cursor = pattern.durations;
  for(int i=0;i < pattern.num_segments - 1;++i) {
    printf("%d, ", *(cursor++));
  }
  printf("%d\n", *cursor);
}

void vibes_long_pulse(void) {
  printf("[INFO] Vibration: long pulse\n");
  //TODO: Emscripten callback to shake screen?
}

void vibes_short_pulse(void) {
  printf("[INFO] Vibration: short pulse\n");
  //TODO: Emscripten callback to shake screen?
}

GContext *app_get_current_graphics_context(void) {
  //TODO: Accurate?
  return &current_graphics_context;
}

bool clock_is_24h_style(void) {
  //TODO: Callbacks to let this be switched on/off in Emscripten
  return true;
}

//#todo sprint2
void property_animation_init_layer_frame(struct PropertyAnimation *property_animation, struct Layer *layer, GRect *from_frame, GRect *to_frame);

//#trivial
void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment text_alignment) {
  text_layer->text_alignment = text_alignment;
}

//#verify
void graphics_draw_bitmap_in_rect(GContext *ctx, const GBitmap *bitmap, GRect rect) {
  // TODO: verify composite mode implementations
  // TODO: rework to support non-32-bit images
  // TODO: bitmap->info_flags?
  GCompOp compositing_mode = ctx->compositing_mode;
  SDL_Surface *image = bitmap->addr;

  LOCK(screen);
  LOCK(image);

  for(uint8_t x=0; x < rect.size.w; x++) {
    uint16_t dst_x = x + rect.origin.x;
    if(dst_x < 0) continue;
    else if(dst_x >= SCREEN_WIDTH) break;


    for(uint8_t y=0; y < rect.size.h; y++) {
      uint16_t dst_y = y + rect.origin.y;
      if(dst_y < 0) continue;
      else if(dst_y >= SCREEN_HEIGHT) break;

      //TODO: Does this implement tiling correctly?
      uint32_t *src_c = (uint32_t *)(image->pixels + image->pitch * (y % bitmap->bounds.size.h) + 4 * (x % bitmap->bounds.size.w));
      uint32_t *dst_c = (uint32_t *)(screen->pixels + screen->pitch * dst_y + 4 * dst_x);

      switch(compositing_mode) {
        case GCompOpAssign:
          *dst_c = *src_c;
          break;
        case GCompOpAssignInverted:
          //Might need the commented section to de-alpha.
          *dst_c = (~*src_c); //| 0xFF000000;
          break;
        case GCompOpOr:
          *dst_c = *src_c | *dst_c; 
          break;
        case GCompOpAnd:
          *dst_c = *src_c & *dst_c;
          break;
        case GCompOpClear:
          *dst_c = r_clear;
          break;
        case GCompOpSet:
          // TODO: Implement
          *dst_c = *src_c;
          break;
      }
    }
  }

  UNLOCK(image);
  UNLOCK(screen);
}

//#todo #sprint2
void graphics_text_draw(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, const GTextLayoutCacheRef layout) {
  printf("[WARN] [NOOP] graphics_text_draw\n");
}

//#trivial #nosdk
void layer_set_bounds(Layer *layer, GRect bounds) {
  layer->bounds = bounds;
}

//#trivial #nosdk
GRect layer_get_bounds(Layer *layer) {
  return layer->bounds;
}

//#trivial #nosdk
void layer_set_update_proc(Layer *layer, LayerUpdateProc update_proc) {
  layer->update_proc = update_proc;
}

//#trivial #nosdk
struct Window *layer_get_window(Layer *layer) {
  return layer->window;
}

//#trivial #nosdk
void layer_remove_child_layers(Layer *parent) { 
  Layer *cursor = parent->first_child;
  while(cursor != NULL) {
    cursor->parent = NULL;
    cursor = cursor->next_sibling;
  }
  parent->first_child = NULL;
}

//#trivial #nosdk
void layer_insert_below_sibling(Layer *layer_to_insert, Layer *below_sibling_layer) {
  layer_to_insert->next_sibling = below_sibling_layer->next_sibling;
  below_sibling_layer->next_sibling = layer_to_insert;
  layer_to_insert->parent = below_sibling_layer->parent;
}

//#trivial #nosdk
void layer_insert_above_sibling(Layer *layer_to_insert, Layer *above_sibling_layer) {
  if(above_sibling_layer->parent->first_child == above_sibling_layer) {
    above_sibling_layer->parent->first_child = layer_to_insert;
  } else {
    Layer *cursor = above_sibling_layer->parent->first_child;
    while(cursor->next_sibling != above_sibling_layer) {
      cursor = cursor->next_sibling;
    }

    cursor->next_sibling = layer_to_insert;
  }

  layer_to_insert->next_sibling = above_sibling_layer;
  layer_to_insert->parent = above_sibling_layer->parent;
}


//#trivial #nosdk
bool layer_get_hidden(Layer *layer) {
  return layer->hidden;
}

//#trivial #nosdk
void layer_set_clips(Layer *layer, bool clips) {
  layer->clips = clips;
}

//#trivial #nosdk
bool layer_get_clips(Layer *layer) {
  return layer->clips;
}

//#nosdk
GSize text_layer_get_max_used_size(GContext *ctx, TextLayer *text_layer);

//#nosdk
void text_layer_set_size(TextLayer *text_layer, const GSize max_size);

//#trivial #nosdk
void text_layer_set_overflow_mode(TextLayer *text_layer, GTextOverflowMode line_mode) {
  text_layer->overflow_mode = line_mode;
}

//#nosdk
GSize graphics_text_layout_get_max_used_size(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, GTextLayoutCacheRef layout);

//#nosdk
void inverter_layer_init(InverterLayer *inverter, GRect frame);

//#nosdk
void bitmap_layer_init(BitmapLayer *image, GRect frame);

//#nosdk
void bitmap_layer_set_bitmap(BitmapLayer *image, const GBitmap *bitmap);

//#nosdk
void bitmap_layer_set_alignment(BitmapLayer *image, GAlign alignment);

//#nosdk
void bitmap_layer_set_background_color(BitmapLayer *image, GColor color);

//#nosdk
void bitmap_layer_set_compositing_mode(BitmapLayer *image, GCompOp mode);

//#nosdk
bool heap_bitmap_init(HeapBitmap *hb, int resource_id);

//#nosdk
void heap_bitmap_deinit(HeapBitmap *hb);

//#nosdk
ButtonId click_recognizer_get_button_id(ClickRecognizerRef recognizer);

//#nosdk
uint8_t click_number_of_clicks_counted(ClickRecognizerRef recognizer);

//#nosdk
void menu_cell_basic_draw(GContext *ctx, const Layer *cell_layer, const char *title, const char *subtitle, GBitmap *icon);

//#nosdk
void menu_cell_title_draw(GContext *ctx, const Layer *cell_layer, const char *title);

//#nosdk
void menu_cell_basic_header_draw(GContext *ctx, const Layer *cell_layer, const char *title);

//#nosdk
void menu_layer_init(MenuLayer *menu_layer, GRect frame);

//#nosdk
Layer *menu_layer_get_layer(MenuLayer *menu_layer);

//#nosdk
void menu_layer_set_callbacks(MenuLayer *menu_layer, void *callback_context, MenuLayerCallbacks callbacks);

//#nosdk
void menu_layer_set_click_config_onto_window(MenuLayer *menu_layer, struct Window *window);

//#nosdk
void menu_layer_set_selected_next(MenuLayer *menu_layer, bool up, MenuRowAlign scroll_align, bool animated);

//#nosdk
void menu_layer_set_selected_index(MenuLayer *menu_layer, MenuIndex index, MenuRowAlign scroll_align, bool animated);

//#nosdk
void menu_layer_reload_data(MenuLayer *menu_layer);

//#nosdk
int16_t menu_index_compare(MenuIndex *a, MenuIndex *b);

//#nosdk
void scroll_layer_init(ScrollLayer *scroll_layer, GRect frame);

//#nosdk
void scroll_layer_add_child(ScrollLayer *scroll_layer, Layer *child);

//#nosdk
void scroll_layer_set_click_config_onto_window(ScrollLayer *scroll_layer, struct Window *window);

//#nosdk
void scroll_layer_set_callbacks(ScrollLayer *scroll_layer, ScrollLayerCallbacks callbacks);

//#nosdk
void scroll_layer_set_context(ScrollLayer *scroll_layer, void *context);

//#nosdk
void scroll_layer_set_content_offset(ScrollLayer *scroll_layer, GPoint offset, bool animated);

//#nosdk
GPoint scroll_layer_get_content_offset(ScrollLayer *scroll_layer);

//#nosdk
void scroll_layer_set_content_size(ScrollLayer *scroll_layer, GSize size);

//#nosdk
GSize scroll_layer_get_content_size(ScrollLayer *scroll_layer);

//#nosdk
void scroll_layer_set_frame(ScrollLayer *scroll_layer, GRect rect);

//#nosdk
void scroll_layer_scroll_up_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer);

//#nosdk
void scroll_layer_scroll_down_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer);

//#nosdk
void simple_menu_layer_init(SimpleMenuLayer *simple_menu, GRect frame, Window *window, const SimpleMenuSection *sections, int num_sections, void *callback_context);

//#nosdk
Layer *simple_menu_layer_get_layer(SimpleMenuLayer *simple_menu);

//#nosdk
int simple_menu_layer_get_selected_index(SimpleMenuLayer *simple_menu);

//#nosdk
void simple_menu_layer_set_selected_index(SimpleMenuLayer *simple_menu, int index, bool animated);

//#nosdk
void window_deinit(Window *window);

//#nosdk
void window_set_click_config_provider_with_context(Window *window, ClickConfigProvider click_config_provider, void *context);

//#nosdk
ClickConfigProvider window_get_click_config_provider(Window *window);

//#nosdk
void window_set_window_handlers(Window *window, WindowHandlers handlers);

//#nosdk
bool window_get_fullscreen(Window *window);

//#nosdk
void window_set_status_bar_icon(Window *window, const GBitmap *icon);

//#nosdk
bool window_is_loaded(Window *window);

/////////// APP Message stuf //////////

DictionaryIterator *outbound_di;
AppMessageResult app_message_out_get(DictionaryIterator **iter_out){
  // TODO: Use the actual app message out size
  Dictionary *dict = calloc(1, sizeof(Dictionary) + 256);

  DictionaryIterator *di = malloc(sizeof(DictionaryIterator));
  di->dictionary = dict;
  di->end = dict+sizeof(Dictionary) + 256;
  di->cursor = dict->head;
  *iter_out = di;
  outbound_di = di;
  
  return APP_MSG_OK;
}

// Note: free output of this function
char* jsonify_dict(DictionaryIterator *di) {
  char* output = malloc(sizeof(char) * 1024);
  output[0] = '[';
  int index = 1;
  Tuple *initial = di->cursor;
  Tuple *t = dict_read_first(di);
  while(t) {
    char value[400];
    switch(t->type) {
    case TUPLE_BYTE_ARRAY: {
      value[0] = '[';
      int size = 1;
      char *cursor = value+size;
      for (int i = 0; i < t->length; i++){
        int n = snprintf(cursor, sizeof(value)-size, "%d, ", t->value->data[i] & 255);
        cursor += n;
        size += n;
      }
      (cursor-2)[0] = ']';
      (cursor-1)[0] = 0;
      break;
    }
    case TUPLE_CSTRING:
      snprintf(value, 100, "\"%s\"", t->value->cstring);
      break;
    case TUPLE_UINT:
      switch(t->length) {
      case 1:
        snprintf(value, 100, "%u", t->value->uint8);
        break;
      case 2:
        snprintf(value, 100, "%u", t->value->uint16);
        break;
      case 4:
        snprintf(value, 100, "%u", t->value->uint32);
        break;
      }
      break;
    case TUPLE_INT:
      switch(t->length) {
      case 1:
        snprintf(value, 100, "%d", t->value->int8);
        break;
      case 2:
        snprintf(value, 100, "%d", t->value->int16);
        break;
      case 4:
        snprintf(value, 100, "%d", t->value->int32);
        break;
      }
      break;
    }

    char line[500];
    int len = snprintf(line, 500, "{\"key\": %d, \"value\": %s},", t->key, value);

    if (index + len > sizeof(output)) {
      realloc(output, sizeof(output) + (index+len) - sizeof(output) + 1);
    }
    memcpy(output+index, line, len);
    index += len;
    t = dict_read_next(di);
  }
  output[index-1] = ']';
  di->cursor = initial;
  return output;
}

AppMessageResult app_message_out_send(void) {
  char *json = jsonify_dict(outbound_di);
  printf("[OUTBOUND] %s\n", json);
  free(json);
  return APP_MSG_OK;
}
AppMessageResult app_message_out_release(void) {
  free(outbound_di->dictionary);
  free(outbound_di);
  return APP_MSG_OK;
}


AppMessageResult app_message_register_callbacks(AppMessageCallbacksNode *callbacks_node);
AppMessageResult app_message_deregister_callbacks(AppMessageCallbacksNode *callbacks_node);
void app_sync_init(AppSync *s, uint8_t *buffer, const uint16_t buffer_size, const Tuplet * const keys_and_initial_values, const uint8_t count, AppSyncTupleChangedCallback tuple_changed_callback, AppSyncErrorCallback error_callback, void *context);
void app_sync_deinit(AppSync *s);
AppMessageResult app_sync_set(AppSync *s, const Tuplet * const keys_and_values_to_update, const uint8_t count);
const Tuple *app_sync_get(const AppSync *s, const uint32_t key);



//////////////// JS API ///////////////
void create_dict() {
  DictionaryIterator *di;
  app_message_out_get(&di);
}

void add_bytes_to_dict(uint32_t key, uint8_t *bytes, size_t length){
  Tuplet t = TupletBytes(key, bytes, length);
  dict_write_tuplet(outbound_di, &t);
}

void add_string_to_dict(uint32_t key, char *str){
  Tuplet t = TupletCString(key, str);
  dict_write_tuplet(outbound_di, &t);
}

void add_uint8_to_dict(uint32_t key, uint8_t i) {
  Tuplet t = TupletInteger(key, i);
  dict_write_tuplet(outbound_di, &t);
}
void add_uint16_to_dict(uint32_t key, uint16_t i) {
  Tuplet t = TupletInteger(key, i);
  dict_write_tuplet(outbound_di, &t);
}
void add_uint32_to_dict(uint32_t key, uint32_t i) {
  char *x = (char*)&i;
  Tuplet t = TupletInteger(key, i);
  dict_write_tuplet(outbound_di, &t);
}
void add_int8_to_dict(uint32_t key, int8_t i) {
  Tuplet t = TupletInteger(key, i);
  dict_write_tuplet(outbound_di, &t);
}
void add_int16_to_dict(uint32_t key, int16_t i) {
  Tuplet t = TupletInteger(key, i);
  dict_write_tuplet(outbound_di, &t);
}
void add_int32_to_dict(uint32_t key, int32_t i) {
  Tuplet t = TupletInteger(key, i);
  dict_write_tuplet(outbound_di, &t);
}

void send_dict_to_pebble() {
  dict_write_end(outbound_di);
  char *msg = jsonify_dict(outbound_di);
  printf("[SENDING] %s\n", msg);
  free(msg);
  (_PebbleAppHandlers.messaging_info.default_callbacks.callbacks.in_received)
    (outbound_di, NULL);
}

// Buttons
ButtonId button_down = -1;
time_t button_down_time;

ClickConfig* get_click_config(ButtonId button_id) {
  if(current_window != NULL) {
    ClickConfig back = {};
    ClickConfig up = {};
    ClickConfig center = {};
    ClickConfig down = {};
    ClickConfig *cs[4];
    cs[0] = &back;
    cs[1] = &up;
    cs[2] = &center;
    cs[3] = &down;

    current_window->click_config_provider(cs, current_window);

    ClickConfig *r = malloc(sizeof(ClickConfig));
    memcpy(r, cs[button_id], sizeof(ClickConfig));
    return r;
  }
  return NULL;
}

void press_button(ButtonId button_id) {
  ClickConfig *c = get_click_config(button_id);
  ClickHandler h = c->click.handler;
  if (h != NULL) {
    h(NULL, current_window);
  }

  free(c);

  button_down = button_id;
  button_down_time = time(NULL);
}

void release_button(ButtonId button_id) {
  button_down = -1;
  button_down_time = 0;
}

void handle_buttons() {
  if (button_down == -1) return;

  time_t now = time(0);
  double delta = difftime(now, button_down_time);
  ClickConfig *c = get_click_config(button_down);
  
  if (delta*1000 > c->long_click.delay_ms && c->long_click.delay_ms > 0) {

    printf("Over delay, clicking %d\n", button_down);
    ClickHandler h = c->long_click.release_handler;
    if (h != NULL) {
      h(NULL, current_window);
    }
    release_button(button_down);
  }
  free(c);
}

void press_back() {
  press_button(BUTTON_ID_BACK);
}
void press_up() {
  press_button(BUTTON_ID_UP);
}
void press_select() {
  press_button(BUTTON_ID_SELECT);
}
void press_down() {
  press_button(BUTTON_ID_DOWN);
}

void release_back() {
  release_button(BUTTON_ID_BACK);
}
void release_up() {
  release_button(BUTTON_ID_UP);
}
void release_select() {
  release_button(BUTTON_ID_SELECT);
}
void release_down() {
  release_button(BUTTON_ID_DOWN);
}

//#end-of-progress

Window *window_stack_pop(bool animated);
void window_stack_pop_all(const bool animated);
bool window_stack_contains_window(Window *window);
Window *window_stack_get_top_window(void);
Window *window_stack_remove(Window *window, bool animated);
void property_animation_init(struct PropertyAnimation *property_animation, const struct PropertyAnimationImplementation *implementation, void *subject, void *from_value, void *to_value);
void property_animation_update_int16(struct PropertyAnimation *property_animation, const uint32_t time_normalized);
void property_animation_update_gpoint(struct PropertyAnimation *property_animation, const uint32_t time_normalized);
void property_animation_update_grect(struct PropertyAnimation *property_animation, const uint32_t time_normalized);
void action_bar_layer_init(ActionBarLayer *action_bar);
void action_bar_layer_set_context(ActionBarLayer *action_bar, void *context);
void action_bar_layer_set_click_config_provider(ActionBarLayer *action_bar, ClickConfigProvider click_config_provider);
void action_bar_layer_set_icon(ActionBarLayer *action_bar, ButtonId button_id, const GBitmap *icon);
void action_bar_layer_clear_icon(ActionBarLayer *action_bar, ButtonId button_id);
void action_bar_layer_add_to_window(ActionBarLayer *action_bar, struct Window *window);
void action_bar_layer_remove_from_window(ActionBarLayer *action_bar);
void action_bar_layer_set_background_color(ActionBarLayer *action_bar, GColor background_color);
void number_window_init(NumberWindow *numberwindow, const char *label, NumberWindowCallbacks callbacks, void *callback_context);
void number_window_set_label(NumberWindow *nw, const char *label);
void number_window_set_max(NumberWindow *numberwindow, int max);
void number_window_set_min(NumberWindow *numberwindow, int min);
void number_window_set_value(NumberWindow *numberwindow, int value);
void number_window_set_step_size(NumberWindow *numberwindow, int step);
int number_window_get_value(NumberWindow *numberwindow);
void clock_copy_time_string(char *buffer, uint8_t size);
#endif /* EMSCRIPTEN */
