/*
 *  Copyright (C) 2010-2016 Fabio Cavallo (aka FHorse)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "info.h"
#include "emu.h"
#include "cpu.h"
#include "gfx.h"
#include "sdl_wid.h"
#include "overscan.h"
#include "clock.h"
#include "input.h"
#include "ppu.h"
#include "version.h"
#include "text.h"
#include "palette.h"
#include "paldef.h"
#include "opengl.h"
#include "conf.h"

#define ntsc_width(wdt, a, flag) wdt = 0

#define change_color(index, color, operation)\
	tmp = palette_RGB[index].color + operation;\
	palette_RGB[index].color = (tmp < 0 ? 0 : (tmp > 0xFF ? 0xFF : tmp))
#define rgb_modifier(red, green, blue)\
	/* prima ottengo la paletta monocromatica */\
	ntsc_set(cfg->ntsc_format, PALETTE_MONO, 0, 0, (BYTE *) palette_RGB);\
	/* quindi la modifico */\
	{\
		WORD i;\
		SWORD tmp;\
		for (i = 0; i < NUM_COLORS; i++) {\
			/* rosso */\
			change_color(i, r, red);\
			/* green */\
			change_color(i, g, green);\
			/* blue */\
			change_color(i, b, blue);\
		}\
	}\
	/* ed infine utilizzo la nuova */\
	ntsc_set(cfg->ntsc_format, FALSE, 0, (BYTE *) palette_RGB,(BYTE *) palette_RGB)

SDL_Surface *framebuffer;
uint32_t *palette_win, software_flags;
static BYTE ntsc_width_pixel[5] = {0, 0, 7, 10, 14};

BYTE gfx_init(void) {
	const SDL_VideoInfo *video_info;

	sdl_wid();

	// inizializzazione video SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		return (EXIT_ERROR);
	}

	video_info = SDL_GetVideoInfo();

	// modalita' video con profondita' di colore
	// inferiori a 32 bits non sono supportate.
	if (video_info->vfmt->BitsPerPixel < 32) {
		fprintf(stderr, "Sorry but color depth less than 32 bits are not supported\n");
		return (EXIT_ERROR);
	}

	// controllo se e' disponibile l'accelerazione hardware
	if (video_info->hw_available) {
		software_flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_ASYNCBLIT;
	} else {
		software_flags = SDL_SWSURFACE | SDL_ASYNCBLIT;
	}

	// per poter inizializzare il glew devo creare un contesto opengl prima
	if (!(surface_sdl = SDL_SetVideoMode(0, 0, 0, SDL_OPENGL))) {
		opengl.supported = FALSE;

		cfg->render = RENDER_SOFTWARE;
		gfx_set_render(cfg->render);

		fprintf(stderr, "INFO: OpenGL not supported.\n");
	} else {
		opengl.supported = TRUE;
	}

	// casi particolari provenienti dal settings_file_parse() e cmd_line_parse()
	if (cfg->fullscreen == FULLSCR) {
      cfg->fullscreen = NO_FULLSCR;
	}
	opengl_init();

	if (opengl.supported == FALSE) {
		cfg->render = RENDER_SOFTWARE;
		gfx_set_render(cfg->render);
	}

	// inizializzo l'ntsc che utilizzero' non solo
	// come filtro ma anche nel gfx_set_screen() per
	// generare la paletta dei colori.
	if (ntsc_init(0, 0, 0, 0, 0) == EXIT_ERROR) {
		return (EXIT_ERROR);
	}

	// mi alloco una zona di memoria dove conservare la
	// paletta nel formato di visualizzazione.
	if (!(palette_win = (uint32_t *) malloc(NUM_COLORS * sizeof(uint32_t)))) {
		fprintf(stderr, "Unable to allocate the palette\n");
		return (EXIT_ERROR);
	}

   gfx_set_screen(cfg->scale, cfg->filter, NO_FULLSCR, cfg->palette, FALSE, FALSE);
	if (cfg->fullscreen) {
		cfg->fullscreen = NO_FULLSCR;
		cfg->scale = gfx.scale_before_fscreen;
	} else {
		// nella versione windows (non so in quella linux), sembra che
		// il VSync (con alcune schede video) non venga settato correttamente
		// al primo gfx_set_screen. E' necessario fare un gfx_reset_video
		// e poi nuovamente un gfx_set_screen. Nella versione linux il gui_reset_video()
		// non fa assolutamente nulla.
	}

	if (!surface_sdl) {
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		return (EXIT_ERROR);
	}

	return (EXIT_OK);
}
void gfx_set_render(BYTE render) {
   gfx.opengl = FALSE;
}
void gfx_set_screen(BYTE scale, DBWORD filter, BYTE fullscreen, BYTE palette, BYTE force_scale,
        BYTE force_palette) {
	BYTE set_mode;
	WORD width, height, w_for_pr, h_for_pr;
	DBWORD old_filter = cfg->filter;

	gfx_set_screen_start:
	set_mode = FALSE;
	width = 0, height = 0;
	w_for_pr = 0, h_for_pr = 0;

	// l'ordine dei vari controlli non deve essere cambiato:
	// 0) overscan
	// 1) filtro
	// 2) fullscreen
	// 3) fattore di scala
	// 4) tipo di paletta (IMPORTANTE: dopo il SDL_SetVideoMode)

	// overscan
	{
		overscan.enabled = cfg->oscan;

		gfx.rows = SCR_ROWS;
		gfx.lines = SCR_LINES;

		if (overscan.enabled == OSCAN_DEFAULT) {
			overscan.enabled = cfg->oscan_default;
		}

		if (overscan.enabled) {
			gfx.rows -= (overscan.borders->left + overscan.borders->right);
			gfx.lines -= (overscan.borders->up + overscan.borders->down);
		}
	}

	/* filtro */
	if (filter == NO_CHANGE) {
		filter = cfg->filter;
	}
   if ((filter != cfg->filter) || info.on_cfg || force_scale)
   {
      gfx.filter = scale_surface;
      // se sto passando dal filtro ntsc ad un'altro, devo
      // ricalcolare la larghezza del video mode quindi
      // forzo il controllo del fattore di scala.
   }

	// fullscreen
	if (fullscreen == NO_CHANGE) {
		fullscreen = cfg->fullscreen;
	}
	if ((fullscreen != cfg->fullscreen) || info.on_cfg) {
		// forzo il controllo del fattore di scale
		force_scale = TRUE;
		// indico che devo cambiare il video mode
		set_mode = TRUE;
	}

	// fattore di scala
	if (scale == NO_CHANGE) {
		scale = cfg->scale;
	}
   if ((scale != cfg->scale) || info.on_cfg || force_scale)
   {
      // il fattore di scala a 1 e' possibile  solo senza filtro
      set_mode = TRUE;
      if (!width) {
         width = gfx.rows * scale;
         gfx.w[CURRENT] = width;
         gfx.w[NO_OVERSCAN] = SCR_ROWS * scale;
      }
      height = gfx.lines * scale;
      gfx.h[CURRENT] = height;
      gfx.h[NO_OVERSCAN] = SCR_LINES * scale;
   }

	// cfg->scale e cfg->filter posso aggiornarli prima
	// del set_mode, mentre cfg->fullscreen e cfg->palette
	// devo farlo necessariamente dopo.
	// salvo il nuovo fattore di scala
	cfg->scale = scale;
	// salvo ill nuovo filtro
	cfg->filter = filter;

	// devo eseguire un SDL_SetVideoMode?
	if (set_mode) {
		uint32_t flags = software_flags;

		gfx.w[VIDEO_MODE] = width;
		gfx.h[VIDEO_MODE] = height;

		gfx.pixel_aspect_ratio = 1.0f;


		// nella versione a 32 bit (GTK) dopo un gfx_reset_video,
		// se non lo faccio anche qui, crasha tutto.
		//sdl_wid();

		// inizializzo la superfice video
		surface_sdl = SDL_SetVideoMode(gfx.w[VIDEO_MODE], gfx.h[VIDEO_MODE], 0, flags);

		// in caso di errore
		if (!surface_sdl) {
			fprintf(stderr, "SDL_SetVideoMode failed : %s\n", SDL_GetError());
			return;
		}

		gfx.bit_per_pixel = surface_sdl->format->BitsPerPixel;
	}

	// paletta
	if (palette == NO_CHANGE) {
		palette = cfg->palette;
	}
	if ((palette != cfg->palette) || info.on_cfg || force_palette) {
		if (palette == PALETTE_FILE) {
			if (strlen(cfg->palette_file) != 0) {
				if (palette_load_from_file(cfg->palette_file) == EXIT_ERROR) {
					memset(cfg->palette_file, 0x00, sizeof(cfg->palette_file));
					text_add_line_info(1, "[red]error on palette file");
					if (cfg->palette != PALETTE_FILE) {
						palette = cfg->palette;
					} else if (machine.type == NTSC) {
						palette = PALETTE_NTSC;
					} else {
						palette = PALETTE_SONY;
					}
				} else {
					ntsc_set(cfg->ntsc_format, FALSE, (BYTE *) palette_base_file, 0,
							(BYTE *) palette_RGB);
				}
			}
		}

		switch (palette) {
			case PALETTE_PAL:
				ntsc_set(cfg->ntsc_format, FALSE, (BYTE *) palette_base_pal, 0,
						(BYTE *) palette_RGB);
				break;
			case PALETTE_NTSC:
				ntsc_set(cfg->ntsc_format, FALSE, 0, 0, (BYTE *) palette_RGB);
				break;
			case PALETTE_FRBX_UNSATURED:
				ntsc_set(cfg->ntsc_format, FALSE, (BYTE *) palette_firebrandx_unsaturated_v5, 0,
						(BYTE *) palette_RGB);
				break;
			case PALETTE_FRBX_YUV:
				ntsc_set(cfg->ntsc_format, FALSE, (BYTE *) palette_firebrandx_YUV_v3, 0,
						(BYTE *) palette_RGB);
				break;
			case PALETTE_GREEN:
				rgb_modifier(-0x20, 0x20, -0x20);
				break;
			case PALETTE_FILE:
				break;
			default:
				ntsc_set(cfg->ntsc_format, palette, 0, 0, (BYTE *) palette_RGB);
				break;
		}

		// inizializzo in ogni caso la tabella YUV dell'hqx
		hqx_init();

		//memorizzo i colori della paletta nel formato di visualizzazione
		{
			WORD i;

			for (i = 0; i < NUM_COLORS; i++) {
				palette_win[i] = SDL_MapRGBA(surface_sdl->format, palette_RGB[i].r,
						palette_RGB[i].g, palette_RGB[i].b, 255);
			}
		}
	}

	// salvo il nuovo stato del fullscreen
	cfg->fullscreen = fullscreen;
	// salvo il nuovo tipo di paletta
	cfg->palette = palette;

	// software rendering
	framebuffer = surface_sdl;
	flip = SDL_Flip;

	text.surface = surface_sdl;
	text_clear = gfx_text_clear;
	text_blit = gfx_text_blit;
	text.w = surface_sdl->w;
	text.h = surface_sdl->h;

	w_for_pr = gfx.w[VIDEO_MODE];
	h_for_pr = gfx.h[VIDEO_MODE];

	gfx_text_reset();

	// calcolo le proporzioni tra il disegnato a video (overscan e schermo
	// con le dimensioni per il filtro NTSC compresi) e quello che dovrebbe
	// essere (256 x 240). Mi serve per calcolarmi la posizione del puntatore
	// dello zapper.
	gfx.w_pr = ((float) w_for_pr / gfx.w[CURRENT]) * ((float) gfx.w[NO_OVERSCAN] / SCR_ROWS);
	gfx.h_pr = ((float) h_for_pr / gfx.h[CURRENT]) * ((float) gfx.h[NO_OVERSCAN] / SCR_LINES);

	if (info.on_cfg == TRUE) {
		info.on_cfg = FALSE;
	}
}

void gfx_draw_screen(BYTE forced) {
	if (!forced && (info.no_rom || info.pause)) {
		if (++info.pause_frames_drawscreen == 4) {
			info.pause_frames_drawscreen = 0;
			forced = TRUE;
		} else {
			text_rendering(FALSE);
			return;
		}
	}

	// se il frameskip me lo permette (o se forzato), disegno lo screen
	if (forced || !ppu.skip_draw) {
		// applico l'effetto desiderato
      gfx.filter(screen.data,
            screen.line,
            palette_win,
            framebuffer->format->BitsPerPixel,
            framebuffer->pitch,
            framebuffer->pixels,
            gfx.rows,
            gfx.lines,
            framebuffer->w,
            framebuffer->h,
            cfg->scale);

      text_rendering(TRUE);

		// disegno a video
		flip(framebuffer);
	}
}
void gfx_reset_video(void) {
	if (surface_sdl) {
		SDL_FreeSurface(surface_sdl);
	}

	surface_sdl = framebuffer = NULL;

	//sdl_wid();
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_InitSubSystem(SDL_INIT_VIDEO);
}

void gfx_quit(void) {
	if (palette_win) {
		free(palette_win);
	}

	if (surface_sdl) {
		SDL_FreeSurface(surface_sdl);
	}

	opengl_context_delete();
	ntsc_quit();
	text_quit();
	SDL_Quit();
}

void gfx_text_create_surface(_txt_element *ele) {
	ele->surface = gfx_create_RGB_surface(text.surface, ele->w, ele->h);
	ele->blank = gfx_create_RGB_surface(text.surface, ele->w, ele->h);
}
void gfx_text_release_surface(_txt_element *ele) {
	if (ele->surface) {
		SDL_FreeSurface(ele->surface);
		ele->surface = NULL;
	}
	if (ele->blank) {
		SDL_FreeSurface(ele->blank);
		ele->blank = NULL;
	}
}
void gfx_text_rect_fill(_txt_element *ele, _rect *rect, uint32_t color) {
	SDL_FillRect(ele->surface, rect, color);
}
void gfx_text_reset(void) {
	txt_table[TXT_NORMAL] = SDL_MapRGBA(text.surface->format, 0xFF, 0xFF, 0xFF, 0);
	txt_table[TXT_RED]    = SDL_MapRGBA(text.surface->format, 0xFF, 0x4C, 0x3E, 0);
	txt_table[TXT_YELLOW] = SDL_MapRGBA(text.surface->format, 0xFF, 0xFF, 0   , 0);
	txt_table[TXT_GREEN]  = SDL_MapRGBA(text.surface->format, 0   , 0xFF, 0   , 0);
	txt_table[TXT_CYAN]   = SDL_MapRGBA(text.surface->format, 0   , 0xFF, 0xFF, 0);
	txt_table[TXT_BROWN]  = SDL_MapRGBA(text.surface->format, 0xEB, 0x89, 0x31, 0);
	txt_table[TXT_BLUE]   = SDL_MapRGBA(text.surface->format, 0x2D, 0x8D, 0xBD, 0);
	txt_table[TXT_GRAY]   = SDL_MapRGBA(text.surface->format, 0xA0, 0xA0, 0xA0, 0);
	txt_table[TXT_BLACK]  = SDL_MapRGBA(text.surface->format, 0   , 0   , 0   , 0);
}
void gfx_text_clear(_txt_element *ele) {
	return;
}
void gfx_text_blit(_txt_element *ele, _rect *rect) {
	SDL_Rect src_rect;

	if (!cfg->txt_on_screen) {
		return;
	}

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.w = ele->w;
	src_rect.h = ele->h;

	SDL_BlitSurface(ele->surface, &src_rect, text.surface, rect);
}

SDL_Surface *gfx_create_RGB_surface(SDL_Surface *src, uint32_t width, uint32_t height) {
	SDL_Surface *new_surface, *tmp;

	tmp = SDL_CreateRGBSurface(src->flags, width, height,
			src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask,
			src->format->Bmask, src->format->Amask);

	new_surface = SDL_DisplayFormatAlpha(tmp);

	memset(new_surface->pixels, 0,
	        new_surface->w * new_surface->h * new_surface->format->BytesPerPixel);

	SDL_FreeSurface(tmp);

	return (new_surface);
}

double sdl_get_ms(void) {
	return (SDL_GetTicks());
}
