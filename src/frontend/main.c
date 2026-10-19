//
// Created by Victor Gomez Ponce on 02/10/2026.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "common/types.h"
#include "core/nes.h"
#include "core/bus.h"
#include "core/cpu6502.h"
#include "core/ppu.h"
#include "core/apu.h"
#include "core/cartridge.h"

#define SCALE 3
#define PANEL_W 320 // Right space to pattern visors
#define FRAME_TIME (1.0 / 60.0)
#define MAX_PENDING_FRAMES 4
#define AUDIO_RATE 48000
#define AUDIO_GAIN 2.0f
#define AUDIO_QUEUE_FRAMES 3

static nes_t nes;
static cartridge_t *cart;
static SDL_AudioDeviceID audio;
static bool audio_muted = false;
static float audio_buffer[APU_BUFFER_SIZE];

static void blit_screen(SDL_Texture *tex) {
    u8 pixels[PPU_SCREEN_H][PPU_SCREEN_W][3];

    for (int y = 0; y < PPU_SCREEN_H; y++) {
        for (int x = 0; x < PPU_SCREEN_W; x++) {
            const u8 *c = ppu_colors[nes.ppu.screen[y][x] & 0x3F];
            pixels[y][x][0] = c[0];
            pixels[y][x][1] = c[1];
            pixels[y][x][2] = c[2];
        }
    }

    SDL_UpdateTexture(tex, NULL, pixels, PPU_SCREEN_W * 3);
}

static void blit_pattern(SDL_Texture *tex, u8 index) {
    u8 pixels[128][128][3];

    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            const u8 *c = ppu_colors[nes.ppu.pattern_screen[index][y][x] & 0x3F];
            pixels[y][x][0] = c[0];
            pixels[y][x][1] = c[1];
            pixels[y][x][2] = c[2];
        }
    }

    SDL_UpdateTexture(tex, NULL, pixels, 128 * 3);
}

// Emulation steps

static void step_instruction(void) {
    do { nes_clock(&nes); } while (!cpu_complete(&nes));
    // CPU Clock is 3x slower, drain leftovers
    do { nes_clock(&nes); } while (cpu_complete(&nes));
}

static void step_frame(void) {
    do { nes_clock(&nes); } while (!nes.ppu.frame_complete);
    do { nes_clock(&nes); } while (!cpu_complete(&nes));
    nes.ppu.frame_complete = false;
}

static void run_frame(void) {
    do { nes_clock(&nes); } while (!nes.ppu.frame_complete);
    nes.ppu.frame_complete = false;
}

static void open_audio(void) {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = AUDIO_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 512;
    audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio) {
        fprintf(stderr, "SDL audio: %s\n", SDL_GetError());
        return;
    }
    apu_set_sample_rate(&nes, (u32)have.freq);
    SDL_PauseAudioDevice(audio, 0);
}

static void queue_audio(void) {
    const u32 n = apu_take_samples(&nes, audio_buffer, APU_BUFFER_SIZE);
    if (!audio) return;
    for (u32 i = 0; i < n; i++) audio_buffer[i] = audio_muted ? 0.0f : audio_buffer[i] * AUDIO_GAIN;
    SDL_QueueAudio(audio, audio_buffer, n * (u32)sizeof(float));
}

static void drain_audio(void) {
    apu_take_samples(&nes, audio_buffer, APU_BUFFER_SIZE);
    if (audio) SDL_ClearQueuedAudio(audio);
}

static void dump_oam(void) {
    for (int i = 0; i < 64; i++) {
        const oam_entry_t *s = &nes.ppu.oam[i];
        printf("%02X: (%3u, %3u) ID: %02X AT: %02X\n", i, s->x, s->y, s->id, s->attribute);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    const char *rom = (argc > 1) ? argv[1] : "roms/Donkey-Kong.nes";

    cart = cartridge_load(rom);
    if (!cart) {
        fprintf(stderr, "No se pudo cargar la ROM: %s\n", rom);
        return 1;
    }
    printf("ROM: %s\n", rom);
    printf("  mapper %u, PRG %u x 16KB, CHR %u x 8KB, mirror %s\n",
           cart->mapper_id, cart->prg_banks, cart->chr_banks,
           cart->mirror == MIRROR_VERTICAL ? "vertical" : "horizontal");

    nes_insert_cartridge(&nes, cart);
    nes_reset(&nes);


    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        cartridge_free(cart);
        return 1;
    }

    const int win_w = PPU_SCREEN_W * SCALE + PANEL_W;
    const int win_h = PPU_SCREEN_H * SCALE;

    SDL_Window *win = SDL_CreateWindow("Nes_Emu",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       win_w, win_h, 0);
    SDL_Renderer *ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (!ren) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        if (win) SDL_DestroyWindow(win);
        SDL_Quit();
        cartridge_free(cart);
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    open_audio();

    SDL_Texture *screen_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                PPU_SCREEN_W, PPU_SCREEN_H);
    SDL_Texture *pat_tex[2];
    pat_tex[0] = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                   SDL_TEXTUREACCESS_STREAMING, 128, 128);
    pat_tex[1] = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24,
                                   SDL_TEXTUREACCESS_STREAMING, 128, 128);

    bool running = true;
    bool emulating = false;
    u8 selected_palette = 0;

    u64 last = SDL_GetPerformanceCounter();
    double residual = 0.0;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_SPACE: emulating = !emulating; break;
                    case SDLK_r: nes_reset(&nes); break;
                    case SDLK_c: if (!emulating) step_instruction(); break;
                    case SDLK_f: if (!emulating) step_frame(); break;
                    case SDLK_p: selected_palette = (selected_palette + 1) & 0x07; break;
                    case SDLK_o: dump_oam(); break;
                    case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5:
                        nes.apu.mute ^= (u8)(1 << (e.key.keysym.sym - SDLK_1));
                        break;
                    case SDLK_m: audio_muted = !audio_muted; break;
                    default: break;
                }
            }
        }

        // Controller 1: X=A, Z=B, A=Select, S=Start, arrows
        const u8 *keys = SDL_GetKeyboardState(NULL);
        nes.controller[0] = 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_X] ? 0x80 : 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_Z] ? 0x40 : 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_A] ? 0x20 : 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_S] ? 0x10 : 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_UP] ? 0x08 : 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_DOWN] ? 0x04 : 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_LEFT] ? 0x02 : 0x00;
        nes.controller[0] |= keys[SDL_SCANCODE_RIGHT] ? 0x01 : 0x00;

        const u64 now = SDL_GetPerformanceCounter();
        const double elapsed = (double)(now - last) / (double)SDL_GetPerformanceFrequency();
        last = now;

        if (emulating && audio) {
            const Uint32 target = (Uint32)(AUDIO_RATE / 60 * AUDIO_QUEUE_FRAMES * sizeof(float));
            for (int i = 0; i < MAX_PENDING_FRAMES && SDL_GetQueuedAudioSize(audio) < target; i++) {
                run_frame();
                queue_audio();
            }
        } else if (emulating) {
            residual += elapsed;
            if (residual > MAX_PENDING_FRAMES * FRAME_TIME) residual = MAX_PENDING_FRAMES * FRAME_TIME;

            while (residual >= FRAME_TIME) {
                residual -= FRAME_TIME;
                run_frame();
                queue_audio();
            }
        } else {
            residual = 0.0;
            drain_audio();
        }

        ppu_render_pattern_table(&nes, 0, selected_palette);
        ppu_render_pattern_table(&nes, 1, selected_palette);

        blit_screen(screen_tex);
        blit_pattern(pat_tex[0], 0);
        blit_pattern(pat_tex[1], 1);

        SDL_SetRenderDrawColor(ren, 0, 0, 40, 255);
        SDL_RenderClear(ren);

        SDL_Rect dst = { 0, 0, PPU_SCREEN_W * SCALE, PPU_SCREEN_H * SCALE };
        SDL_RenderCopy(ren, screen_tex, NULL, &dst);

        const int px = PPU_SCREEN_W * SCALE + 16;
        SDL_Rect p0 = { px, 16, 128, 128 };
        SDL_Rect p1 = { px, 160, 128, 128 };
        SDL_RenderCopy(ren, pat_tex[0], NULL, &p0);
        SDL_RenderCopy(ren, pat_tex[1], NULL, &p1);

        SDL_RenderPresent(ren);

        char channels[6];
        for (int i = 0; i < 5; i++) channels[i] = (nes.apu.mute & (1 << i)) ? '-' : (char)('1' + i);
        channels[5] = '\0';

        char title[192];
        snprintf(title, sizeof title,
                 "Nes_Emu  |  %s  |  PC:%04X A:%02X X:%02X Y:%02X SP:%02X  pal:%u  snd:%s%s",
                 emulating ? "RUN" : "PAUSE",
                 nes.cpu.pc, nes.cpu.a, nes.cpu.x, nes.cpu.y,
                 nes.cpu.stkp, selected_palette, channels, audio_muted ? " M" : "");
        SDL_SetWindowTitle(win, title);
    }

    if (audio) SDL_CloseAudioDevice(audio);
    SDL_DestroyTexture(pat_tex[0]);
    SDL_DestroyTexture(pat_tex[1]);
    SDL_DestroyTexture(screen_tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    cartridge_free(cart);
    return 0;
}
