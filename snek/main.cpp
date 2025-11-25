#include "GameEngine.h"
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#define WINDOWWIDTH 128
#define WINDOWHEIGHT 64
#define ZOOMMAX 14
#define FPSTARGET 25
#define TILEW 8
#define XMAX WINDOWWIDTH/TILEW
#define YMAX WINDOWHEIGHT/TILEW

#define SNAKELENMAX 128
#define SNAKELENINIT 3
#define FOODS 6
#define SKINS 7
#define MODCOUNT 11
#define SOUNDS 5

#define STARTBTN SDL_SCANCODE_RETURN
#define MUTEBTN SDL_SCANCODE_M
#define STICKUP SDL_SCANCODE_UP
#define STICKUP2 SDL_SCANCODE_W
#define STICKDOWN SDL_SCANCODE_DOWN
#define STICKDOWN2 SDL_SCANCODE_S
#define STICKLEFT SDL_SCANCODE_LEFT
#define STICKLEFT2 SDL_SCANCODE_A
#define STICKRIGHT SDL_SCANCODE_RIGHT
#define STICKRIGHT2 SDL_SCANCODE_D
#define BOOSTBTN SDL_SCANCODE_Z
#define SPECIALBTN SDL_SCANCODE_X
#define ZOOMUPBTN SDL_SCANCODE_F11
#define ZOOMDOWNBTN SDL_SCANCODE_F10

SDL_Texture* brick[SKINS-1];
SDL_Texture* enemy[3];
SDL_Texture* food[FOODS];
SDL_Texture* icon[2];
SDL_Texture* mode[MODCOUNT-2];
SDL_Texture* font[10];
SDL_Texture* snake[4][SKINS];
SDL_Texture* speed[3];
SDL_Texture* text[2];
Mix_Chunk* sound[SOUNDS];

// 0 - death
// 1 - food
// 2 - reload
// 3 - shoot
// 4 - turn

byte windowscale = 6;
const Uint16 colors[3] = {0x000, 0xffff, 0x22ff};
char snakebody[SNAKELENMAX+1], bricks[SNAKELENMAX], frenzy[SNAKELENMAX];
char foodx, foody, fooddir, foodtype, snakex, snakey, snakeprevx, snakeprevy, snakedir, snaketurn, portalx, portaly, crash;
byte snakelen, brickcount, frenzystage, enemystun, guncooldown;
bool foodcollect;
const byte modsmax[MODCOUNT] = {SKINS, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2};
byte mods[MODCOUNT] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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

void quicksnek(int x, int y, int part, int dir) {
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
	rectclear(x, y);
	ge_drawrottexture(snake[part][mods[0]], x, y, TILEW, TILEW, rot, flip);
}

void drawnumerals(unsigned int num, int x, int y) {
	for (int i = 0; i < to_string(num).length(); i++) {
		ge_drawtexture(font[to_string(num)[i]-'0'], x, y, 4, 7);
		x += 5;
	}
}

void loadtextures(SDL_Texture** arr, string name, byte max) {
	for (int i = 0; i < max; i++) {
		SDL_Surface* surface = IMG_Load(catfn(name, i).c_str());
		arr[i] = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
	}
}

void deloadtextures(SDL_Texture** arr, byte max) {
	for (int i = 0; i < max; i++) {
		SDL_DestroyTexture(arr[i]);
	}
}

void snakelenincrease(int len, int dir) {
	for (int i = 0; i < len; i++) {
		snakelen += 1;
		snakebody[snakelen-2] = dir;
	}
}

bool draw(bool crashcheck) {
	byte x = snakex*TILEW;
	byte y = snakey*TILEW;
	bool dcrash = snakex < 0 || snakex > XMAX-1 || snakey < 0 || snakey > YMAX-1;
	unsigned int* bodycrash = new unsigned int[snakelen*2];
	if (mods[MODCOUNT-1]) {
		dcrash = false; crashcheck = false;
	}
	for (int i = 0; i < snakelen; i++) {
		if (!mods[3] || i == 0 || (x/TILEW) % 2 != (y / TILEW) % 2) {
			bodycrash[i * 2] = x;
			bodycrash[i * 2 + 1] = y;
			if (i == 0) quicksnek(x, y, 0, snakebody[0]);
			else if (i == snakelen - 1) quicksnek(x, y, 3, snakebody[i-1]);
			else if (snakebody[i-1] == snakebody[i]) quicksnek(x, y, 1, snakebody[i]);
			else if (snakebody[i - 1] == 0 && snakebody[i] == 3) quicksnek(x, y, 2, 0);
			else if (snakebody[i - 1] == 3 && snakebody[i] == 0) quicksnek(x, y, 2, 2);
			else if (snakebody[i - 1] > snakebody[i]) quicksnek(x, y, 2, snakebody[i-1]);
			else if (snakebody[i - 1] < snakebody[i]) quicksnek(x, y, 2, ge_mod(snakebody[i-1]-1, 4));
		}
		switch (snakebody[i]) {
			case 0: y += TILEW; break;
			case 1: x -= TILEW; break;
			case 2: y -= TILEW; break;
			case 3: x += TILEW; break;
		}
		if (mods[MODCOUNT-1]) {
			x = ge_mod(x, XMAX*TILEW); y = ge_mod(y, YMAX*TILEW);
		}
	}
	if (crashcheck) {
		for (int i = 1; i < snakelen; i++) {
			if (bodycrash[i * 2] == snakex * TILEW && bodycrash[i * 2 + 1] == snakey * TILEW) {
				dcrash = true;
				break;
			}
		}
	}
	if (mods[2]) {
		for (int i = 0; i < brickcount; i++) {
			ge_drawtexture((mods[0] == modsmax[0]-1)?(foodcollect?NULL:food[foodtype]):brick[mods[0]], bricks[i*2]*TILEW, bricks[i*2+1]*TILEW, TILEW, TILEW);
			if (bricks[i*2] == snakex && bricks[i*2+1] == snakey && crashcheck) dcrash = true;
		}
	}
	
	if (mods[8]) {
		for (int i = 0; i < SNAKELENMAX/2; i++) {
			if (frenzy[i * 2] != -1 && frenzy[i * 2 + 1] != -1) {
				ge_drawtexture(food[foodtype], frenzy[i*2]*TILEW, frenzy[i*2+1]*TILEW, TILEW, TILEW);
			}
		}
	}
	else if (!mods[9]) {
		if (mods[6]){
			rectclear(portalx*TILEW, portaly*TILEW);
			ge_drawtexture(food[foodtype], portalx*TILEW, portaly*TILEW, TILEW, TILEW);
		}
		rectclear(foodx*TILEW, foody*TILEW);
		ge_drawtexture(food[foodtype], foodx*TILEW, foody*TILEW, TILEW, TILEW);
	}
	if (mods[9]) ge_drawtexture(enemy[(crash > 0)?2:((enemystun > 0)?1:0)], foodx*TILEW, foody*TILEW, TILEW, TILEW);
	drawnumerals((mods[8]?frenzystage:(snakelen-SNAKELENINIT)), 1, (snakex < 4 && snakey < 4)?(YMAX*TILEW-9):1);
	return dcrash;
}

void drawmenu(int m) {
	SDL_RenderClear(renderer);
	draw(false);
	ge_drawtexture(text[0], (XMAX*TILEW-40)/2, 20, 40, 9);
	const byte y = YMAX * TILEW - 9;
	for (int i = 0; i < MODCOUNT; i++) {
		byte x = XMAX*TILEW - (MODCOUNT-i)*10;
		switch (i) {
			case 0: quicksnek(x, y, 0, 3); break;
			case 1: ge_drawtexture(speed[mods[1]], x, y, TILEW, TILEW); break;
			default: ge_drawtexture(mods[i]?icon[0]:mode[i-2], x, y, TILEW, TILEW);
		}
	}
	if (m >= 0) ge_drawtexture(icon[1], XMAX*TILEW - (MODCOUNT-m)*10 - 2, YMAX*TILEW-11, TILEW+4, TILEW+4);
	SDL_RenderPresent(renderer);
}

void snek() {
	foodx = 3;
	foody = 3;
	fooddir = 0;
	snakex = 9;
	snakey = 3;
	snakedir = 3;
	snaketurn = 0;
	snakelen = SNAKELENINIT;
	brickcount = 0;
	portalx = 12;
	portaly = 3;
	enemystun = 0;
	guncooldown = 0;
	crash = 0;
	frenzystage = 0;
	if (mods[8]) {
		for (int i = 0; i < SNAKELENMAX/2; i++) {
			frenzy[i * 2] = rand() % XMAX;
			frenzy[i * 2 + 1] = rand() % YMAX;
		}
	}
	Uint8* keys = ge_input();
	char eventframe = 0;
	for (int i = 0; i < snakelen - 1; i++) snakebody[i] = snakedir;
	while (true) {
		ge_preframe();
		keys = ge_input();
		snakeprevx = snakex; snakeprevy = snakey;
		char snakedirend = snakebody[snakelen - 2];
		char framedelay = (2-mods[1])*15+5;
		for (int i = snakelen - 1; i > 0; i--) snakebody[i] = snakebody[i - 1];
		if (snakedir > -1) snakebody[0] = snakedir;
		switch (snakebody[0]) {
		case 0: snakey -= 1; break;
		case 1: snakex += 1; break;
		case 2: snakey += 1; break;
		case 3: snakex -= 1; break;
		}
		if (mods[MODCOUNT - 1]) {
			snakex = ge_mod(snakex, XMAX); snakey = ge_mod(snakey, YMAX);
		}
		if (mods[5] && eventframe % 2 == 0) {
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
		if (mods[5] && mods[6] && eventframe % 8 == 0) {
			portalx = rand() % XMAX;
			portaly = rand() % YMAX;
		}
		if (mods[9]) {
			if (eventframe % 4 == 0 && enemystun == 0) {
				if (foodx > snakex) foodx -= 1;
				else if (foodx < snakex) foodx += 1;
				if (foody > snakey) foody -= 1;
				else if (foody < snakey) foody += 1;
			}
			if (keys[SPECIALBTN] && guncooldown == 0) {
				guncooldown = 2;
				ge_sound(sound[3]);
				char x = snakex;
				char y = snakey;
				for (int i = 0; i < 16; i++) {
					if (x < 0 || x >= XMAX || y < 0 || y >= YMAX) break;
					else if (x == foodx && y == foody) {
						enemystun = 16; break;
					}
					switch (snakebody[0]) {
					case 0: y -= 1; break;
					case 1: x += 1; break;
					case 2: y += 1; break;
					case 3: x -= 1; break;
					}
				}
			}
		}
		foodcollect = (snakex == foodx && snakey == foody) || (mods[6] && snakex == portalx && snakey == portaly);
		if (mods[8]) {
			foodcollect = false;
			byte frenzycollect = 0;
			for (int i = 0; i < SNAKELENMAX / 2; i++) {
				if (frenzy[i * 2] >= 0 && frenzy[i * 2 + 1] >= 0) {
					if (frenzy[i * 2] == snakex && frenzy[i * 2 + 1] == snakey) {
						foodcollect = true;
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
				for (int i = 0; i < SNAKELENMAX / 2; i++) {
					frenzy[i * 2] = rand() % XMAX;
					frenzy[i * 2 + 1] = rand() % YMAX;
				}
			}
		}
		if (foodcollect) {
			snakelenincrease(1, snakedirend);
			ge_sound(sound[1]);
			if (mods[3]) snakelenincrease(1, snakedirend);
			if (mods[6]) {
				if (snakex == portalx && snakey == portaly) {
					snakex = foodx; snakey = foody;
				}
				else {
					snakex = portalx; snakey = portaly;
				}
				portalx = rand() % XMAX;
				portaly = rand() % YMAX;
			}
			if (mods[9]) {
				switch (rand() % 4) {
				case 0: foodx = 0; foody = 0; break;
				case 1: foodx = XMAX - 1; foody = 0; break;
				case 2: foodx = XMAX - 1; foody = YMAX - 1; break;
				case 3: foodx = 0; foody = YMAX - 1; break;
				}
				enemystun = 0;
			}
			else if (!mods[8]) {
				foodx = rand() % XMAX;
				foody = rand() % YMAX;
				foodtype = rand() % FOODS;
			}
			if (mods[2] && (snakelen % 2 == 0 || (mods[3] && snakelen % 4 == 1))) {
				brickcount += 1;
				bricks[brickcount * 2 - 2] = rand() % XMAX;
				bricks[brickcount * 2 - 1] = rand() % YMAX;
			}
			if (mods[4]) {
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
					snakebody[i] = (snakebody[snakelen - 2 - i] + 2) % 4;
					snakebody[snakelen - 2 - i] = tmp;
				}
				SDL_Delay(framedelay*8);
			}
		}
		SDL_SetRenderDrawColor(renderer, 32, 32, 255, 255);;
		SDL_RenderClear(renderer);
		crash = draw(true)?1:0;
		SDL_RenderPresent(renderer);
		if (!mods[MODCOUNT-1]){
			if(mods[9] && enemystun == 0 && snakex >= foodx - 1 && snakex <= foodx + 1 && snakey >= foody - 1 && snakey <= foody + 1) crash = 2;
		}
		if (crash > 0 && !foodcollect) {
			if (crash == 1) {
				snakex = snakeprevx; snakey = snakeprevy;
				for (int i = 1; i < snakelen + 1; i++) snakebody[i - 1] = snakebody[i];
			}
			break;
		}
		snakedir = -1;
		bool turned = false;
		for (int d = 0; d < framedelay; d++) {
			Uint8* keys = ge_input();
			if (keys[STARTBTN]) {
				if (mods[MODCOUNT-1]) {
					while(ge_input()[STARTBTN]);
					snakedir = -2;
					break;
				}
				while (frozeninput()[STARTBTN]);
				while (!frozeninput()[STARTBTN]) {
					SDL_RenderClear(renderer);
					draw(false);
					ge_drawtexture(text[1], (XMAX*TILEW-30)/2, 20, 30, 9);
					SDL_RenderPresent(renderer);
					SDL_Delay(40);
				}
				SDL_Delay(framedelay * 8);
				break;
			}
			if (!mods[9] || !keys[SPECIALBTN]) {
				if (keys[BOOSTBTN]) d += 8;
				else if ((keys[STICKLEFT] || keys[STICKLEFT2]) && (mods[7] || snakebody[0] != 1)) snakedir = 3;
				else if ((keys[STICKRIGHT] || keys[STICKRIGHT2]) && (mods[7] || snakebody[0] != 3)) snakedir = 1;
				else if ((keys[STICKUP] || keys[STICKUP2]) && (mods[7] || snakebody[0] != 2)) snakedir = 0;
				else if ((keys[STICKDOWN] || keys[STICKDOWN2]) && (mods[7] || snakebody[0] != 0)) snakedir = 2;
			}
			if (snakedir > -1) {
				if (snakedir != snakebody[0]) {
					d += 8;
					if (!turned) {
						ge_sound(sound[4]);
						if (mods[7]) {
							if (snaketurn == 0) snaketurn = (snakedir == 0) ? -1 : 1;
							snakedir = ge_mod(snakebody[0] - snaketurn, 4);
						}
					}
					turned = true;
					
				}
			}
			SDL_Delay(10);
		}
		if (snakedir < -1) break;
		if (mods[9]) {
			if (guncooldown > 0) {
				guncooldown -= 1;
				if (guncooldown == 0) ge_sound(sound[2]);
			}
			if (enemystun > 0) enemystun -= 1;
		}
		eventframe = (eventframe + 1) % 8;
	}
	ge_sound(sound[0]);
	char m = 0;
	drawmenu(m);
	SDL_Delay(300);
	keys = ge_input();
	while (!keys[STARTBTN]) {
		ge_preframe();
		keys = frozeninput();
		bool yes = false;
		if (keys[STICKLEFT] || keys[STICKLEFT2]) {
			m -= 1; yes = true;
		}
		if (keys[STICKRIGHT] || keys[STICKRIGHT2]) {
			m += 1; yes = true;
		}
		if ((keys[STICKUP] || keys[STICKUP2]) && mods[m] > 0) {
			mods[m] -= 1; yes = true;
		}
		if ((keys[STICKDOWN] || keys[STICKDOWN2]) && mods[m] < modsmax[m] - 1) {
			mods[m] += 1; yes = true;
		}
		m = ge_mod(m, MODCOUNT);
		drawmenu(m);
		if (yes) {
			SDL_SetWindowIcon(window, IMG_Load(("assets/snake.0." + to_string(mods[0]) + ".png").c_str()));
			SDL_Delay(200);
		}
		SDL_Delay(40);
	}
	while (ge_input()[STARTBTN]);
}

int main(int argc, char *args[]) {
	ge_start("Snek", WINDOWWIDTH, WINDOWHEIGHT, windowscale);
	SDL_SetWindowIcon(window, IMG_Load("assets/snake.0.0.png"));
	loadtextures(brick, "brick", SKINS-1);
	loadtextures(enemy, "enemy", 3);
	loadtextures(food, "food", FOODS);
	loadtextures(icon, "icon", 2);
	loadtextures(mode, "mode", MODCOUNT-2);
	for (int s = 0; s < 4; s++) loadtextures(snake[s], "snake." + to_string(s), SKINS);
	loadtextures(speed, "speed", 3);
	loadtextures(text, "text", 2);
	for (int i = 0; i < 10; i++) {
		SDL_Surface* surface = ge_xbmsurface(ge_font_4x7_numerals[i], 4, 7, colors[1], colors[0]);
		font[i] = SDL_CreateTextureFromSurface(renderer, surface);
	}
	for (int i = 0; i < SOUNDS; i++) {
		sound[i] = Mix_LoadWAV(("assets/sound." + to_string(i) + ".wav").c_str());
	}
	while (true) {
		try {snek();}
		catch (ge_quit) {break;}
	}
	deloadtextures(brick, SKINS-1);
	deloadtextures(enemy, 3);
	deloadtextures(food, FOODS);
	deloadtextures(icon, 2);
	deloadtextures(mode, MODCOUNT-2);
	for (int s = 0; s < 4; s++) deloadtextures(snake[s], SKINS);
	deloadtextures(speed, 3);
	deloadtextures(text, 2);
	for (int i = 0; i < SOUNDS; i++) {
		Mix_FreeChunk(sound[i]);
	}
	return 0;
}
