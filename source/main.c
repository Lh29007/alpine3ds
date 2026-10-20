/*
 * alpine3ds : mini terminal tactile pour Nintendo 3DS (homebrew .3dsx)
 *
 *   Ecran du haut  : sortie du terminal
 *   Ecran du bas   : clavier QWERTY tactile + ligne de saisie
 *
 * Commandes : help, clear, fetch, exit
 * Touche START : quitter
 */
#include <3ds.h>
#include <citro2d.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define TOP_W     400
#define BOT_W     320
#define SCR_H     240

#define KB_TOP    60          /* y du premier rang du clavier */
#define KEY_W     32
#define KEY_H     36
#define MAX_KEYS  64
#define MAX_LINES 16
#define LINE_LEN  64
#define INPUT_LEN 60

/* codes speciaux (hors ASCII imprimable) */
#define K_BACK  1
#define K_ENTER 2
#define K_SPACE 3

typedef struct {
	int x, y, w, h;
	int code;
	char lbl[12];
	C2D_Text txt;
} Key;

static Key keys[MAX_KEYS];
static int nkeys = 0;

static C3D_RenderTarget *topTarget;
static C3D_RenderTarget *botTarget;

static C2D_TextBuf keyBuf;   /* titres et touches : jamais effaces */
static C2D_TextBuf outBuf;   /* sortie du terminal : reconstruite au besoin */
static C2D_TextBuf inBuf;    /* ligne de saisie : reconstruite a chaque frappe */

static char lines[MAX_LINES][LINE_LEN];
static C2D_Text outTxt[MAX_LINES];
static int nlines = 0;
static bool outputDirty = true;

static char input[INPUT_LEN];
static int ilen = 0;
static C2D_Text inputTxt;
static bool inputDirty = true;

static C2D_Text titleTxt;
static bool running = true;

/* ------------------------------------------------------------------ */
/* Clavier                                                             */
/* ------------------------------------------------------------------ */

static void add_key(int x, int y, int w, int code, const char *label)
{
	if (nkeys >= MAX_KEYS)
		return;
	Key *k = &keys[nkeys++];
	k->x = x;
	k->y = y;
	k->w = w;
	k->h = KEY_H;
	k->code = code;
	snprintf(k->lbl, sizeof k->lbl, "%s", label);
	C2D_TextParse(&k->txt, keyBuf, k->lbl);
}

static void build_keyboard(void)
{
	static const char *rows[4] = { "1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm." };

	for (int r = 0; r < 4; r++) {
		int n = (int)strlen(rows[r]);
		int off = (10 - n) / 2;          /* centrage sur une grille de 10 colonnes */
		for (int i = 0; i < n; i++) {
			char tmp[2] = { rows[r][i], 0 };
			add_key((off + i) * KEY_W, KB_TOP + r * KEY_H, KEY_W, rows[r][i], tmp);
		}
	}

	int y = KB_TOP + 4 * KEY_H;
	add_key(0,            y, 6 * KEY_W, K_SPACE, "espace");
	add_key(6 * KEY_W,    y, 2 * KEY_W, K_BACK,  "<-");
	add_key(8 * KEY_W,    y, 2 * KEY_W, K_ENTER, "OK");
}

static const Key *hit(int x, int y)
{
	for (int i = 0; i < nkeys; i++) {
		const Key *k = &keys[i];
		if (x >= k->x && x < k->x + k->w && y >= k->y && y < k->y + k->h)
			return k;
	}
	return NULL;
}

/* ------------------------------------------------------------------ */
/* Sortie du terminal                                                  */
/* ------------------------------------------------------------------ */

static void say(const char *fmt, ...)
{
	char buf[LINE_LEN];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	if (nlines < MAX_LINES) {
		strcpy(lines[nlines++], buf);
	} else {
		memmove(lines, lines + 1, sizeof(lines) - LINE_LEN);
		strcpy(lines[MAX_LINES - 1], buf);
	}
	outputDirty = true;
}

/* ------------------------------------------------------------------ */
/* Commandes                                                           */
/* ------------------------------------------------------------------ */

static void cmd_fetch(void)
{
	static const char *models[] = {
		"Nintendo 3DS", "Nintendo 3DS XL", "New Nintendo 3DS",
		"Nintendo 2DS", "New Nintendo 3DS XL", "New Nintendo 2DS XL"
	};
	u8 model = 0;
	u8 battery = 0;

	cfguInit();
	CFGU_GetSystemModel(&model);
	cfguExit();

	ptmuInit();
	PTMU_GetBatteryLevel(&battery);
	ptmuExit();

	u32 kv = osGetKernelVersion();
	const char *m = (model < 6) ? models[model] : "inconnu";

	say("alpine3ds@3ds");
	say("  OS       : Alpine (rootfs non installee)");
	say("  Modele   : %s", m);
	say("  Kernel   : %u.%u-%u", (unsigned)((kv >> 24) & 0xff),
	    (unsigned)((kv >> 16) & 0xff), (unsigned)((kv >> 8) & 0xff));
	say("  Batterie : %u/5", (unsigned)battery);
	say("  Ecrans   : 400x240 haut, 320x240 bas tactile");
}

static void run_command(const char *cmd)
{
	say("$ %s", cmd);

	if (cmd[0] == '\0')
		return;
	if (strcmp(cmd, "help") == 0) {
		say("Commandes : help, clear, fetch, exit");
		say("START : quitter le payload");
	} else if (strcmp(cmd, "clear") == 0) {
		nlines = 0;
		outputDirty = true;
	} else if (strcmp(cmd, "fetch") == 0) {
		cmd_fetch();
	} else if (strcmp(cmd, "exit") == 0) {
		running = false;
	} else {
		say("sh: commande introuvable: %s", cmd);
	}
}

/* ------------------------------------------------------------------ */
/* Saisie                                                              */
/* ------------------------------------------------------------------ */

static void key_pressed(const Key *k)
{
	if (!k)
		return;

	switch (k->code) {
	case K_BACK:
		if (ilen > 0)
			input[--ilen] = '\0';
		break;
	case K_ENTER:
		input[ilen] = '\0';
		run_command(input);
		ilen = 0;
		input[0] = '\0';
		break;
	case K_SPACE:
		if (ilen < INPUT_LEN - 1) {
			input[ilen++] = ' ';
			input[ilen] = '\0';
		}
		break;
	default:
		if (ilen < INPUT_LEN - 1) {
			input[ilen++] = (char)k->code;
			input[ilen] = '\0';
		}
		break;
	}
	inputDirty = true;
}

/* ------------------------------------------------------------------ */
/* Rendu                                                               */
/* ------------------------------------------------------------------ */

static void rebuild_texts(void)
{
	if (inputDirty) {
		char line[INPUT_LEN + 32];
		snprintf(line, sizeof line, "root@alpine3ds:~$ %s_", input);
		C2D_TextBufClear(inBuf);
		C2D_TextParse(&inputTxt, inBuf, line);
		inputDirty = false;
	}
	if (outputDirty) {
		C2D_TextBufClear(outBuf);
		for (int i = 0; i < nlines; i++)
			C2D_TextParse(&outTxt[i], outBuf, lines[i]);
		outputDirty = false;
	}
}

static void draw_top(void)
{
	C2D_SceneBegin(topTarget);
	C2D_DrawRectSolid(0, 0, 0.0f, TOP_W, 20, C2D_Color32(0x2a, 0x4a, 0x7a, 0xff));
	C2D_DrawText(&titleTxt, 0, 8, 4, 0.5f, 0.6f, 0.6f);
	for (int i = 0; i < nlines; i++)
		C2D_DrawText(&outTxt[i], 0, 8, 28 + i * 12, 0.5f, 0.55f, 0.55f);
}

static void draw_key(const Key *k, u32 color)
{
	C2D_DrawRectSolid(k->x + 2, k->y + 2, 0.0f, k->w - 4, k->h - 4, color);

	float tw = 0.0f, th = 0.0f;
	C2D_TextGetDimensions(&k->txt, 0.6f, 0.6f, &tw, &th);
	C2D_DrawText(&k->txt, 0,
	             k->x + (k->w - tw) / 2.0f,
	             k->y + (k->h - th) / 2.0f,
	             0.5f, 0.6f, 0.6f);
}

static void draw_bottom(void)
{
	C2D_SceneBegin(botTarget);
	C2D_DrawRectSolid(0, 0, 0.0f, BOT_W, KB_TOP - 4, C2D_Color32(0x20, 0x20, 0x2c, 0xff));
	C2D_DrawText(&inputTxt, 0, 6, 18, 0.5f, 0.6f, 0.6f);

	for (int i = 0; i < nkeys; i++) {
		const Key *k = &keys[i];
		bool special = (k->code == K_BACK || k->code == K_ENTER || k->code == K_SPACE);
		draw_key(k, special ? C2D_Color32(0x2a, 0x5a, 0x8a, 0xff)
		                    : C2D_Color32(0x3a, 0x3a, 0x4c, 0xff));
	}
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	gfxInitDefault();
	hidInit();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	keyBuf = C2D_TextBufNew(1024);
	outBuf = C2D_TextBufNew(2048);
	inBuf  = C2D_TextBufNew(256);

	C2D_TextParse(&titleTxt, keyBuf, "alpine3ds - terminal");
	build_keyboard();

	say("alpine3ds pret. Tape 'help' puis OK.");

	while (aptMainLoop() && running) {
		hidScanInput();
		u32 down = hidKeysDown();

		if (down & KEY_START)
			break;

		if (down & KEY_TOUCH) {
			touchPosition tp;
			hidTouchRead(&tp);
			key_pressed(hit(tp.px, tp.py));
		}

		rebuild_texts();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(topTarget, C2D_Color32(0x0c, 0x0c, 0x12, 0xff));
		draw_top();
		C2D_TargetClear(botTarget, C2D_Color32(0x14, 0x14, 0x1c, 0xff));
		draw_bottom();
		C3D_FrameEnd(0);
	}

	C2D_TextBufDelete(keyBuf);
	C2D_TextBufDelete(outBuf);
	C2D_TextBufDelete(inBuf);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
