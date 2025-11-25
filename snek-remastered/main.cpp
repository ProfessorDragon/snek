#include "GameEngine.h"
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

// Snek Remastered
// Made by Professor Dragon

// this is horrible coding...

#define WINDOWWIDTH 256
#define WINDOWHEIGHT 144
#define ZOOMMAX 6
#define FPSTARGET 30
#define TILEW 16
#define XMAX WINDOWWIDTH/TILEW
#define YMAX WINDOWHEIGHT/TILEW

#define SNAKELENMAX XMAX*YMAX
#define SNAKELENINIT 3
#define FOODS 16
#define SKINS 8
#define MODCOUNT 13
#define SOUNDS 6

#define STARTBTN SDL_SCANCODE_RETURN
#define MUTEBTN SDL_SCANCODE_M
#define RANDOMISEBTN SDL_SCANCODE_R
#define HIDEMENUBTN SDL_SCANCODE_H
#define STICKUP SDL_SCANCODE_UP
#define STICKUP2 SDL_SCANCODE_W
#define STICKDOWN SDL_SCANCODE_DOWN
#define STICKDOWN2 SDL_SCANCODE_S
#define STICKLEFT SDL_SCANCODE_LEFT
#define STICKLEFT2 SDL_SCANCODE_A
#define STICKRIGHT SDL_SCANCODE_RIGHT
#define STICKRIGHT2 SDL_SCANCODE_D
#define BOOSTBTN SDL_SCANCODE_I
#define BOOSTBTN2 SDL_SCANCODE_Z
#define SPECIALBTN SDL_SCANCODE_O
#define SPECIALBTN2 SDL_SCANCODE_X
#define ZOOMUPBTN SDL_SCANCODE_F11
#define ZOOMDOWNBTN SDL_SCANCODE_F10

SDL_Texture* extra;
SDL_Texture* food;
SDL_Texture* icon[5];
SDL_Texture* mode;
SDL_Texture* snake;
SDL_Texture* text[3];
SDL_Texture* font[10];
Mix_Chunk* sound[SOUNDS];

// 0 - death
// 1 - food
// 2 - randomise
// 3 - shoot
// 4 - turn
// 5 - boost

byte windowscale = 3;
const Uint16 colors[3] = {0x0000, 0x000f, 0xeeef};
char snakebody[SNAKELENMAX], snakebody2[SNAKELENMAX], bricks[SNAKELENMAX], frenzy[SNAKELENMAX];
char snakex, snakey, snakeprevx, snakeprevy, snakedir, snaketurn, snakex2, snakey2, snakeprevx2, snakeprevy2, snakedir2, snaketurn2;
char foodx, foody, fooddir, portalx, portaly, boost, crash;
byte snakelen, snakelen2, brickcount, frenzystage, enemystun, guncooldown, guncooldown2;
bool foodcollect, foodcollect2, p1c2, enemytarget1;
char foodtype = 1;
const char modsmax[MODCOUNT] = {SKINS*2, FOODS+1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3};
char mods[MODCOUNT] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

Uint8* frozeninput() {
	Uint8* k = ge_input();
	if (k[ZOOMUPBTN] && windowscale < ZOOMMAX) {
		windowscale += 1;
		SDL_SetWindowSize(window, WINDOWWIDTH*windowscale, WINDOWHEIGHT*windowscale);
		while (ge_input()[ZOOMUPBTN]);
	} else if (k[ZOOMDOWNBTN] && windowscale > 1) {
		windowscale -= 1;
		SDL_SetWindowSize(window, WINDOWWIDTH*windowscale, WINDOWHEIGHT*windowscale);
		while (ge_input()[ZOOMDOWNBTN]);
	} else if (k[MUTEBTN]) {
		mute = !mute;
		while (ge_input()[MUTEBTN]);
	}
	return k;
}

string catfn(const string filename, int num) {
	return "assets/" + filename + "." + to_string(num) + ".png";
}

void rectclear(int x, int y) {
	SDL_Rect rect = {x, y, TILEW, TILEW};
	SDL_RenderFillRect(renderer, &rect);
}

void setbgandclear(bool effects) {
	byte lightness = 239;
	if (effects) lightness -= (max(boost, 0) * 12);
	SDL_SetRenderDrawColor(renderer, lightness, lightness, lightness, 255);
	SDL_RenderClear(renderer);
}

void quicksnek(int x, int y, int part, int dir, bool p2) {
	double rot = 0;
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	switch (dir) {
		case 0: rot = 90; break;
		case 1:
			if (part == 0) flip = SDL_FLIP_HORIZONTAL;
			else rot = 180;
			break;
		case 2: rot = 270; break;
	}
	ge_drawcroprottexture(snake, x, y, TILEW, TILEW, part*TILEW, (p2?ge_mod(mods[0]+SKINS, SKINS*2):mods[0])*TILEW, rot, flip);
}

void drawnumerals(unsigned int num, int x, int y) {
	for (int i = 0; i < to_string(num).length(); i++) {
		ge_drawtexture(font[to_string(num)[i]-'0'], x, y, 8, 14);
		x += 10;
	}
}

void loadtextures(SDL_Texture** arr, string name, byte max) {
	for (int i = 0; i < max; i++) {
		arr[i] = IMG_LoadTexture(renderer, catfn(name, i).c_str());
	}
}

void destroytextures(SDL_Texture** arr, byte max) {
	for (int i = 0; i < max; i++) {
		SDL_DestroyTexture(arr[i]);
	}
}

void snakelenincrease(int len, int dir, bool p2) {
	for (int i = 0; i < len; i++) {
		if (p2) {
			snakelen2 += 1;
			snakebody2[snakelen2-2] = dir;
		} else {
			snakelen += 1;
			snakebody[snakelen-2] = dir;
		}
	}
}

void drawscore(bool middle) {
	if (ge_input()[HIDEMENUBTN]) return;
	byte score = mods[9]?frenzystage:((mods[MODCOUNT-1] == 1)?(snakelen+snakelen2-SNAKELENINIT*2):(snakelen-SNAKELENINIT));
	if (middle) drawnumerals(score, (XMAX*TILEW-76)/2+((mods[MODCOUNT-1] == 2)?26:49), (mods[MODCOUNT-1] == 2)?41:40);
	else drawnumerals(score, 1, (snakex < 4 && snakey < 4)?(YMAX*TILEW-15):1);
	if (mods[MODCOUNT - 1] == 2) {
		score = snakelen2-SNAKELENINIT;
		if (middle) drawnumerals(score, (XMAX*TILEW-76)/2+26, 57);
		else drawnumerals(score, XMAX*TILEW-9, (snakex2 > XMAX-4 && snakey2 < 4)?(YMAX*TILEW-15):1);
	}
}

byte draw(bool crashcheck) {
	int x = snakex*TILEW;
	int y = snakey*TILEW;
	bool dcrash = snakex < 0 || snakex > XMAX-1 || snakey < 0 || snakey > YMAX-1;
	bool dcrash2 = mods[MODCOUNT - 1];
	int* bodycrash = new int[snakelen*2];
	int* bodycrash2 = new int[snakelen2*2];
	if (mods[MODCOUNT - 2]) {
		dcrash = false; crashcheck = false;
	} else if (dcrash2) {
		dcrash2 = snakex2 < 0 || snakex2 > XMAX - 1 || snakey2 < 0 || snakey2 > YMAX - 1;
	}
	for (int i = 0; i < snakelen; i++) {
		if (!mods[4] || i == 0 || (x/TILEW) % 2 != (y / TILEW) % 2) {
			bodycrash[i * 2] = x;
			bodycrash[i * 2 + 1] = y;
			if (i == 0) quicksnek(x, y, 0, snakebody[0], false);
			else if (i == snakelen - 1) quicksnek(x, y, 3, snakebody[i-1], false);
			else if (snakebody[i - 1] == snakebody[i]) quicksnek(x, y, 1, snakebody[i], false);
			else if (snakebody[i - 1] == 0 && snakebody[i] == 3) quicksnek(x, y, 2, 0, false);
			else if (snakebody[i - 1] == 3 && snakebody[i] == 0) quicksnek(x, y, 2, 2, false);
			else if (snakebody[i - 1] > snakebody[i]) quicksnek(x, y, 2, snakebody[i-1], false);
			else if (snakebody[i - 1] < snakebody[i]) quicksnek(x, y, 2, ge_mod(snakebody[i-1]-1, 4), false);
		}
		switch (snakebody[i]) {
			case 0: y += TILEW; break;
			case 1: x -= TILEW; break;
			case 2: y -= TILEW; break;
			case 3: x += TILEW; break;
		}
		if (mods[MODCOUNT-2]) {
			x = ge_mod(x, XMAX*TILEW); y = ge_mod(y, YMAX*TILEW);
		}
	}
	if (mods[MODCOUNT-1]) {
		x = snakex2*TILEW;
		y = snakey2*TILEW;
		for (int i = 0; i < snakelen2; i++) {
			if (!mods[4] || i == 0 || (x / TILEW) % 2 != (y / TILEW) % 2) {
				bodycrash2[i*2] = x;
				bodycrash2[i*2+1] = y;
				if (i == 0) quicksnek(x, y, 0, snakebody2[0], true);
				else if (i == snakelen2 - 1) quicksnek(x, y, 3, snakebody2[i-1], true);
				else if (snakebody2[i-1] == snakebody2[i]) quicksnek(x, y, 1, snakebody2[i], true);
				else if (snakebody2[i-1] == 0 && snakebody2[i] == 3) quicksnek(x, y, 2, 0, true);
				else if (snakebody2[i-1] == 3 && snakebody2[i] == 0) quicksnek(x, y, 2, 2, true);
				else if (snakebody2[i-1] > snakebody2[i]) quicksnek(x, y, 2, snakebody2[i-1], true);
				else if (snakebody2[i-1] < snakebody2[i]) quicksnek(x, y, 2, ge_mod(snakebody2[i-1]-1, 4), true);
			}
			switch (snakebody2[i]) {
				case 0: y += TILEW; break;
				case 1: x -= TILEW; break;
				case 2: y -= TILEW; break;
				case 3: x += TILEW; break;
			}
			if (mods[MODCOUNT-2]) {
				x = ge_mod(x, XMAX*TILEW); y = ge_mod(y, YMAX*TILEW);
			}
		}
	}
	if (crashcheck) {
		for (int i = 1; i < snakelen; i++) {
			while (bodycrash[i*2] == foodx*TILEW && bodycrash[i*2+1] == foody*TILEW) {
				foodx = rand() % XMAX;
				foody = rand() % YMAX;
			}
			if (mods[7]) {
				while (bodycrash[i*2] == portalx*TILEW && bodycrash[i*2+1] == portaly*TILEW) {
					portalx = rand() % XMAX;
					portaly = rand() % YMAX;
				}
			}
			if (bodycrash[i*2] == snakex*TILEW && bodycrash[i*2+1] == snakey*TILEW) {
				dcrash = true;
				break;
			}
			if (mods[MODCOUNT - 1] == 2 && bodycrash[i * 2] == snakex2 * TILEW && bodycrash[i * 2 + 1] == snakey2 * TILEW) {
				dcrash2 = true;
				break;
			}
		}
		if (mods[MODCOUNT-1]) {
			for (int i = 1; i < snakelen2; i++) {
				while (bodycrash2[i * 2] == foodx * TILEW && bodycrash2[i * 2 + 1] == foody * TILEW) {
					foodx = rand() % XMAX;
					foody = rand() % YMAX;
				}
				if (mods[7]) {
					while (bodycrash2[i * 2] == portalx * TILEW && bodycrash2[i * 2 + 1] == portaly * TILEW) {
						portalx = rand() % XMAX;
						portaly = rand() % YMAX;
					}
				}
				if (bodycrash2[i * 2] == snakex2 * TILEW && bodycrash2[i * 2 + 1] == snakey2 * TILEW) {
					dcrash2 = true;
					break;
				}
				if (mods[MODCOUNT - 1] == 2 && bodycrash2[i * 2] == snakex * TILEW && bodycrash2[i * 2 + 1] == snakey * TILEW) {
					dcrash = true;
					break;
				}
			}
		}
		if (mods[MODCOUNT - 1] == 2 && snakex == snakex2 && snakey == snakey2) {
			dcrash = true; dcrash2 = true;
		}
	}
	if (mods[3]) {
		for (int i = 0; i < brickcount; i++) {
			ge_drawcroptexture(snake, bricks[i*2]*TILEW, bricks[i*2+1]*TILEW, TILEW, TILEW, 4*TILEW, mods[0]*TILEW);
			if (bricks[i*2] == snakex && bricks[i*2+1] == snakey && crashcheck) dcrash = true;
			if (mods[MODCOUNT-1] && bricks[i*2] == snakex2 && bricks[i*2+1] == snakey2 && crashcheck) dcrash2 = true;
		}
	}
	
	if (mods[9]) {
		for (int i = 0; i < SNAKELENMAX/2; i++) {
			if (frenzy[i * 2] != -1 && frenzy[i * 2 + 1] != -1) {
				ge_drawcroptexture(food, frenzy[i*2]*TILEW, frenzy[i*2+1]*TILEW, TILEW, TILEW, foodtype*TILEW, 0);
			}
		}
	}
	else if (!mods[10]) {
		if (mods[7]){
			ge_drawcroptexture(food, portalx*TILEW, portaly*TILEW, TILEW, TILEW, foodtype*TILEW, 0);
		}
		ge_drawcroptexture(food, foodx*TILEW, foody*TILEW, TILEW, TILEW, foodtype*TILEW, 0);
	}
	if (mods[10]) ge_drawcroptexture(extra, foodx*TILEW, foody*TILEW, TILEW, TILEW, ((crash > 0)?2:((enemystun > 0) ?1:0))*TILEW, 2*TILEW);
	drawscore(false);
	return (dcrash?1:0)+(dcrash2?2:0);
}

void drawmenu(int m) {
	setbgandclear(false);
	draw(false);
	SDL_Rect bottomline = {0, YMAX*TILEW-22, XMAX*TILEW, YMAX*TILEW};
	SDL_RenderFillRect(renderer, &bottomline);
	SDL_Rect textrect = {(XMAX*TILEW-76)/2-1, 20, 76, (mods[MODCOUNT-1] == 2)?54:36};
	SDL_RenderFillRect(renderer, &textrect);
	ge_drawtexture((mods[MODCOUNT-1] == 2)?text[1]:text[0], textrect.x, textrect.y, textrect.w, textrect.h);
	drawscore(true);
	SDL_Rect iconrect = {XMAX*TILEW-61, YMAX*TILEW-43, 61, 22};
	SDL_RenderFillRect(renderer, &iconrect);
	ge_drawtexture(icon[2], XMAX*TILEW-20, YMAX*TILEW-42, 20, 20);
	ge_drawcroptexture(icon[3], XMAX*TILEW-40, YMAX*TILEW-42, 20, 20, mute?20:0, 0);
	ge_drawtexture(icon[4], XMAX*TILEW-60, YMAX*TILEW-42, 20, 20);
	const byte y = YMAX*TILEW-19;
	for (int i = 0; i < MODCOUNT; i++) {
		byte x = XMAX*TILEW - (MODCOUNT-i)*19;
		switch (i) {
			case 0: quicksnek(x, y, 0, 3, false); break;
			case 1: ge_drawcroptexture(food, x, y, TILEW, TILEW, mods[1]*TILEW, 0); break;
			case 2: ge_drawcroptexture(extra, x, y, TILEW, TILEW, mods[2]*TILEW, 0); break;
			case MODCOUNT-1: ge_drawcroptexture(extra, x, y, TILEW, TILEW, mods[MODCOUNT-1]*TILEW, TILEW); break;
			default:
				if (mods[i]) ge_drawtexture(icon[0], x, y, TILEW, TILEW);
				else ge_drawcroptexture(mode, x, y, TILEW, TILEW, (i-3)*TILEW, 0);
		}
	}
	if (m >= 0) ge_drawtexture(icon[1], XMAX*TILEW - (MODCOUNT-m)*19 - 2, YMAX*TILEW-21, TILEW+4, TILEW+4);
	SDL_RenderPresent(renderer);
}

void snek() {
	foodx = 3;
	foody = 3;
	fooddir = 0;
	snakex = 9;
	snakey = mods[MODCOUNT - 1]?2:3;
	snakex2 = 9;
	snakey2 = 4;
	snakedir = 3;
	snakedir2 = 3;
	snaketurn = 0;
	snaketurn2 = 0;
	snakelen = SNAKELENINIT;
	snakelen2 = SNAKELENINIT;
	brickcount = 0;
	portalx = 12;
	portaly = 3;
	enemystun = 0;
	guncooldown = 0;
	guncooldown2 = 0;
	boost = -15;
	crash = 0;
	frenzystage = 0;
	if (mods[9]) {
		for (int i = 0; i < SNAKELENMAX/2; i++) {
			frenzy[i * 2] = rand() % XMAX;
			frenzy[i * 2 + 1] = rand() % YMAX;
		}
	}
	Uint8* keys = ge_input();
	char eventframe = 0;
	for (int i = 0; i < snakelen - 1; i++) snakebody[i] = snakedir;
	if (mods[MODCOUNT - 1]) {
		for (int i = 0; i < snakelen2 - 1; i++) snakebody2[i] = snakedir2;
	}
	while (true) {
		ge_preframe();
		keys = ge_input();
		snakeprevx = snakex; snakeprevy = snakey;
		char snakedirend = snakebody[snakelen - 2];
		char snakedirend2 = snakebody2[snakelen2 - 2];
		if (mods[MODCOUNT - 1]) {
			snakeprevx2 = snakex2; snakeprevy2 = snakey2;
		}
		char framedelay = (2-mods[2])*12+8;
		for (int i = snakelen - 1; i > 0; i--) snakebody[i] = snakebody[i - 1];
		if (snakedir > -1) snakebody[0] = snakedir;
		switch (snakebody[0]) {
			case 0: snakey -= 1; break;
			case 1: snakex += 1; break;
			case 2: snakey += 1; break;
			case 3: snakex -= 1; break;
		}
		if (mods[MODCOUNT - 1]) {
			for (int i = snakelen2 - 1; i > 0; i--) snakebody2[i] = snakebody2[i-1];
			if (snakedir2 > -1) snakebody2[0] = snakedir2;
			switch (snakebody2[0]) {
				case 0: snakey2 -= 1; break;
				case 1: snakex2 += 1; break;
				case 2: snakey2 += 1; break;
				case 3: snakex2 -= 1; break;
			}
		}
		if (mods[MODCOUNT-2]) {
			snakex = ge_mod(snakex, XMAX); snakey = ge_mod(snakey, YMAX);
			if (mods[MODCOUNT - 1]) {
				snakex2 = ge_mod(snakex2, XMAX); snakey2 = ge_mod(snakey2, YMAX);
			}
		}
		if (mods[6] && eventframe % 2 == 0) {
			if (fooddir <= 1) foodx += 1;
			else foodx -= 1;
			if (fooddir >= 1 && fooddir <= 2) foody += 1;
			else foody -= 1;
			if (foodx <= 0) {
				if (fooddir == 2) fooddir = 1;
				else fooddir = 0;
			}
			else if (foodx >= XMAX - 1) {
				if (fooddir == 0) fooddir = 3;
				else fooddir = 2;
			}
			else if (foody <= 0) {
				if (fooddir == 0) fooddir = 1;
				else fooddir = 2;
			}
			else if (foody >= YMAX - 1) {
				if (fooddir == 1) fooddir = 0;
				else fooddir = 3;
			}
		}
		if (mods[6] && mods[7] && eventframe % 8 == 0) {
			portalx = rand() % XMAX;
			portaly = rand() % YMAX;
		}
		if (mods[10]) {
			if (eventframe%4 == 0 && enemystun == 0) {
				if (!mods[MODCOUNT-1] || enemytarget1) {
					if (foodx > snakex) foodx -= 1;
					else if (foodx < snakex) foodx += 1;
					if (foody > snakey) foody -= 1;
					else if (foody < snakey) foody += 1;
				} else {
					if (foodx > snakex2) foodx -= 1;
					else if (foodx < snakex2) foodx += 1;
					if (foody > snakey2) foody -= 1;
					else if (foody < snakey2) foody += 1;
				}
			}
			if (mods[MODCOUNT-1]?((keys[SPECIALBTN] && guncooldown == 0) || (keys[SPECIALBTN2] && guncooldown2 == 0)):
					((keys[SPECIALBTN] || keys[SPECIALBTN2]) && guncooldown == 0)) {
				ge_sound(sound[3]);
				char x = snakex; char y = snakey;
				if (mods[MODCOUNT - 1] && keys[SPECIALBTN2]) {
					guncooldown2 = 2;
					x = snakex2; y = snakey2;
				} else {
					guncooldown = 2;
				}
				for (int i = 0; i < 16; i++) {
					if (x < 0 || x >= XMAX || y < 0 || y >= YMAX) break;
					else if (x == foodx && y == foody) {
						enemystun = 16; break;
					}
					switch ((mods[MODCOUNT - 1] && keys[SPECIALBTN2])?snakebody2[0]:snakebody[0]) {
						case 0: y -= 1; break;
						case 1: x += 1; break;
						case 2: y += 1; break;
						case 3: x -= 1; break;
					}
				}
			}
		}
		foodcollect = (snakex == foodx && snakey == foody) || (mods[7] && snakex == portalx && snakey == portaly);
		if (mods[MODCOUNT - 1]) {
			foodcollect2 = (snakex2 == foodx && snakey2 == foody) || (mods[7] && snakex2 == portalx && snakey2 == portaly);
		}
		if (mods[9]) {
			foodcollect = false;
			if (mods[MODCOUNT - 1]) foodcollect2 = false;
			byte frenzycollect = 0;
			for (int i = 0; i < SNAKELENMAX / 2; i++) {
				bool yes = false;
				if (frenzy[i * 2] >= 0 && frenzy[i * 2 + 1] >= 0) {
					if (frenzy[i * 2] == snakex && frenzy[i * 2 + 1] == snakey) {
						foodcollect = true; yes = true;
					}
					if (mods[MODCOUNT - 1] && frenzy[i * 2] == snakex2 && frenzy[i * 2 + 1] == snakey2) {
						foodcollect2 = true; yes = true;
					}
					if (yes) {
						frenzy[i * 2] = -1;
						frenzy[i * 2 + 1] = -1;
						frenzycollect += 1;
					}
				}
				else frenzycollect += 1;
			}
			if (frenzycollect >= SNAKELENMAX / 2) {
				frenzystage += 1;
				snakelen = frenzystage * 3 + SNAKELENINIT;
				snakedir = snakebody[0];
				for (int i = 0; i < snakelen; i++) snakebody[i] = snakedir;
				if (mods[MODCOUNT - 1]) {
					snakelen2 = frenzystage * 3 + SNAKELENINIT;
					snakedir2 = snakebody2[0];
					for (int i = 0; i < snakelen2; i++) snakebody[i] = snakedir2;
				}
				for (int i = 0; i < SNAKELENMAX / 2; i++) {
					frenzy[i * 2] = rand() % XMAX;
					frenzy[i * 2 + 1] = rand() % YMAX;
				}
			}
		}
		if (foodcollect || foodcollect2) {
			if (foodcollect) snakelenincrease(1, snakedirend, false);
			if (foodcollect2) snakelenincrease(1, snakedirend2, true);
			ge_sound(sound[1]);
			if (mods[7]) {
				if (foodcollect && snakex == portalx && snakey == portaly) {
					snakex = foodx; snakey = foody;
				} else if (foodcollect && snakex == foodx && snakey == foody) {
					snakex = portalx; snakey = portaly;
				}
				if (mods[MODCOUNT - 1]) {
					if (foodcollect2 && snakex2 == portalx && snakey2 == portaly) {
						snakex2 = foodx; snakey2 = foody;
					} else if (foodcollect2 && snakex2 == foodx && snakey2 == foody) {
						snakex2 = portalx; snakey2 = portaly;
					}
				}
				portalx = rand() % XMAX;
				portaly = rand() % YMAX;
			}
			if (mods[10]) {
				switch (rand() % 4) {
					case 0: foodx = 0; foody = 0; break;
					case 1: foodx = XMAX - 1; foody = 0; break;
					case 2: foodx = XMAX - 1; foody = YMAX - 1; break;
					case 3: foodx = 0; foody = YMAX - 1; break;
				}
				enemystun = 0;
				if (mods[MODCOUNT - 1]) {
					enemytarget1 = !enemytarget1;
				}
			}
			else if (!mods[9]) {
				foodx = rand() % XMAX;
				foody = rand() % YMAX;
				foodtype = (mods[1] == 0)?(rand() % FOODS+1):mods[1];
			}
			if (mods[3] && ((foodcollect && snakelen % 2 == 0) || (foodcollect2 && snakelen2 % 2 == 0))) {
				brickcount += 1;
				bricks[brickcount*2-2] = rand() % XMAX;
				bricks[brickcount*2-1] = rand() % YMAX;
			}
			if (mods[5]) {
				if (foodcollect) {
					for (int i = 0; i < snakelen - 1; i++) {
						switch (snakebody[i]) {
							case 0: snakey += 1; break;
							case 1: snakex -= 1; break;
							case 2: snakey -= 1; break;
							case 3: snakex += 1; break;
						}
					}
					snakedir = (snakedir + 2) % 4;
					for (int i = 0; i < floor(snakelen / 2.0); i++) {
						char tmp = (snakebody[i] + 2) % 4;
						snakebody[i] = (snakebody[snakelen-2-i]+2)%4;
						snakebody[snakelen - 2 - i] = tmp;
					}
				}
				if (foodcollect2) {
					for (int i = 0; i < snakelen2 - 1; i++) {
						switch (snakebody2[i]) {
						case 0: snakey2 += 1; break;
						case 1: snakex2 -= 1; break;
						case 2: snakey2 -= 1; break;
						case 3: snakex2 += 1; break;
						}
					}
					snakedir2 = (snakedir2 + 2) % 4;
					for (int i = 0; i < floor(snakelen2 / 2.0); i++) {
						char tmp = (snakebody2[i] + 2) % 4;
						snakebody2[i] = (snakebody2[snakelen2-2-i]+2)%4;
						snakebody2[snakelen2-2-i] = tmp;
					}
				}
				
				SDL_Delay(framedelay*8);
			}
		}
		setbgandclear(true);
		crash = draw(true);
		SDL_RenderPresent(renderer);
		if (!mods[MODCOUNT-2]){
			if (mods[10] && enemystun == 0 && snakex >= foodx - 1 && snakex <= foodx + 1 && snakey >= foody - 1 && snakey <= foody + 1) crash = 4;
			if (mods[MODCOUNT - 1] && mods[10] && enemystun == 0 &&
				snakex2 >= foodx - 1 && snakex2 <= foodx + 1 && snakey2 >= foody - 1 && snakey2 <= foody + 1) crash = 4;
		}
		if (crash > 0 && (!foodcollect || !foodcollect2)) {
			if (crash == 1 || crash == 3) {
				snakex = snakeprevx; snakey = snakeprevy;
				for (int i = 1; i < snakelen + 1; i++) snakebody[i-1] = snakebody[i];
			}
			if (crash == 2 || crash == 3) {
				snakex2 = snakeprevx2; snakey2 = snakeprevy2;
				for (int i = 1; i < snakelen2 + 1; i++) snakebody2[i-1] = snakebody2[i];
			}
			break;
		}
		snakedir = -1;
		snakedir2 = -1;
		bool turned = false;
		for (int d = 0; d < framedelay; d++) {
			Uint8* keys = ge_input();
			if (keys[STARTBTN]) {
				if (mods[MODCOUNT-2]) {
					while(ge_input()[STARTBTN]);
					snakedir = -2;
					if (mods[MODCOUNT - 1]) snakedir2 = -2;
					break;
				}
				while (frozeninput()[STARTBTN]);
				while (!frozeninput()[STARTBTN]) {
					setbgandclear(false);
					draw(false);
					SDL_Rect textrect = {(XMAX*TILEW-58)/2-1, 20, 60, 20};
					SDL_RenderFillRect(renderer, &textrect);
					ge_drawtexture(text[2], textrect.x, textrect.y, textrect.w, textrect.h);
					SDL_RenderPresent(renderer);
					SDL_Delay(40);
				}
				SDL_Delay(framedelay * 8);
				break;
			}
			if (!mods[10] || !keys[SPECIALBTN] || !keys[SPECIALBTN2]) {
				if ((boost > 0 || keys[BOOSTBTN] || keys[BOOSTBTN2]) && !mods[MODCOUNT-1]){
					if ((keys[BOOSTBTN] || keys[BOOSTBTN2]) && boost == -15) {
						ge_sound(sound[5]);
						boost = 10;
					}
					if (boost >= 0) d += boost;
				}
				if (mods[MODCOUNT - 1]) {
					if (keys[STICKLEFT] && (mods[8] || snakebody[0] != 1)) {
						snakedir = 3;
						if (p1c2) snakedir2 = 3;
					} else if (keys[STICKRIGHT] && (mods[8] || snakebody[0] != 3)) {
						snakedir = 1;
						if (p1c2) snakedir2 = 1;
					} else if (keys[STICKUP] && (mods[8] || snakebody[0] != 2)) {
						snakedir = 0;
						if (p1c2) snakedir2 = 0;
					} else if (keys[STICKDOWN] && (mods[8] || snakebody[0] != 0)) {
						snakedir = 2;
						if (p1c2) snakedir2 = 2;
					}
					if (keys[STICKLEFT2] && (mods[8] || snakebody2[0] != 1)) {
						snakedir2 = 3;
						if (p1c2) snakedir = 3;
					} else if (keys[STICKRIGHT2] && (mods[8] || snakebody2[0] != 3)) {
						snakedir2 = 1;
						if (p1c2) snakedir = 1;
					} else if (keys[STICKUP2] && (mods[8] || snakebody2[0] != 2)) {
						snakedir2 = 0;
						if (p1c2) snakedir = 0;
					} else if (keys[STICKDOWN2] && (mods[8] || snakebody2[0] != 0)) {
						snakedir2 = 2;
						if (p1c2) snakedir = 2;
					}
				} else {
					if ((keys[STICKLEFT] || keys[STICKLEFT2]) && (mods[8] || snakebody[0] != 1)) snakedir = 3;
					else if ((keys[STICKRIGHT] || keys[STICKRIGHT2]) && (mods[8] || snakebody[0] != 3)) snakedir = 1;
					else if ((keys[STICKUP] || keys[STICKUP2]) && (mods[8] || snakebody[0] != 2)) snakedir = 0;
					else if ((keys[STICKDOWN] || keys[STICKDOWN2]) && (mods[8] || snakebody[0] != 0)) snakedir = 2;
				}
			}
			if (snakedir > -1 && snakedir != snakebody[0]) {
				d += mods[MODCOUNT-1]?4:8;
				if (!turned || mods[8]) {
					if (!turned) ge_sound(sound[4]);
					if (mods[8]) {
						if (snaketurn == 0) snaketurn = (snakedir == 0) ? -1 : 1;
						snakedir = ge_mod(snakebody[0] - snaketurn, 4);
					}
				}
				turned = true;
					
			}
			if (mods[MODCOUNT-1] && snakedir2 > -1 && snakedir2 != snakebody2[0]) {
				if (snakedir <= -1 || snakedir == snakebody[0]) d += 4;
				if (!turned || mods[8]) {
					if (!turned) ge_sound(sound[4]);
					if (mods[8]) {
						if (snaketurn2 == 0) snaketurn2 = (snakedir2 == 0) ? -1 : 1;
						snakedir2 = ge_mod(snakebody2[0] - snaketurn2, 4);
					}
				}
				turned = true;
			}
			SDL_Delay(10);
		}
		if (boost > -15) boost--;
		if (snakedir < -1 || snakedir2 < -1) break;
		if (mods[10]) {
			if (guncooldown > 0) guncooldown -= 1;
			if (guncooldown2 > 0) guncooldown2 -= 1;
			if (enemystun > 0) enemystun -= 1;
		}
		eventframe = (eventframe + 1) % 8;
	}
	ge_sound(sound[0]);
	char m = 0;
	drawmenu(m);
	SDL_Delay(300);
	byte randomisebtn = 0;
	keys = ge_input();
	while (!keys[STARTBTN]) {
		ge_preframe();
		keys = frozeninput();
		bool yes = false;
		if (keys[HIDEMENUBTN]) {
			setbgandclear(false);
			draw(false);
			SDL_RenderPresent(renderer);
		} else {
			if (keys[STICKLEFT] || keys[STICKLEFT2]) {
				m -= 1; yes = true;
			}
			if (keys[STICKRIGHT] || keys[STICKRIGHT2]) {
				m += 1; yes = true;
			}
			if (keys[STICKUP] || keys[STICKUP2]) {
				mods[m] -= 1; yes = true;
			}
			if (keys[STICKDOWN] || keys[STICKDOWN2]) {
				mods[m] += 1; yes = true;
			}
			if (yes) {
				m = ge_mod(m, MODCOUNT);
				mods[m] = ge_mod(mods[m], modsmax[m]);
				if (m == 1) foodtype = (mods[1] == 0)?(rand() % FOODS+1):mods[1];
			}
			if (randomisebtn || keys[RANDOMISEBTN]) {
				if (randomisebtn == 0){
					ge_sound(sound[2]);
					randomisebtn = 10;
				}
				for (int i = 0; i < MODCOUNT-2; i++) {
					mods[i] = rand() % modsmax[i];
				}
				yes = false;
				SDL_Delay(80);
			}
			if (keys[HIDEMENUBTN]) {
			
			} else drawmenu(m);
			if (yes) {
				if (m == MODCOUNT-1) p1c2 = (keys[BOOSTBTN] && keys[SPECIALBTN]) || (keys[BOOSTBTN2] && keys[SPECIALBTN2]);
				SDL_Delay(200);
			}
			if (randomisebtn > 0) randomisebtn--;
		}
		SDL_Delay(40);
	}
	while (ge_input()[STARTBTN]);
}

int main(int argc, char *args[]) {
	ge_start("Snek Remastered", WINDOWWIDTH, WINDOWHEIGHT, windowscale);
	SDL_SetWindowIcon(window, IMG_Load("assets/logo.png"));
	extra = IMG_LoadTexture(renderer, "assets/extras.png");
	food = IMG_LoadTexture(renderer, "assets/foods.png");
	loadtextures(icon, "icon", 5);
	mode = IMG_LoadTexture(renderer, "assets/modes.png");
	snake = IMG_LoadTexture(renderer, "assets/snakes.png");
	loadtextures(text, "text", 3);
	for (int i = 0; i < 10; i++) {
		SDL_Surface* surface = ge_xbmsurface(ge_font_8x14_numerals[i], 8, 14, colors[1], colors[0]);
		font[i] = SDL_CreateTextureFromSurface(renderer, surface);
	}
	for (int i = 0; i < SOUNDS; i++) {
		sound[i] = Mix_LoadWAV(("assets/sound." + to_string(i) + ".wav").c_str());
	}
	while (true) {
		try {snek();}
		catch (ge_quit) {break;}
	}
	SDL_DestroyTexture(extra);
	SDL_DestroyTexture(food);
	destroytextures(icon, 5);
	SDL_DestroyTexture(mode);
	SDL_DestroyTexture(snake);
	destroytextures(text, 3);
	for (int i = 0; i < SOUNDS; i++) {
		Mix_FreeChunk(sound[i]);
	}
	return 0;
}
