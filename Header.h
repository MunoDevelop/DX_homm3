#pragma once

//#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>
#include <d3dx9tex.h>
#include <stdio.h>
#include <stdlib.h>


#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

#define RADIOUS_HEXAGON 44.13
#define RADIOUS_HEXAGON_FIX 0.78
#define MAP_START_X 44.13
#define MAP_START_Y 170


// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
LPD3DXSPRITE d3dspt;    // the pointer to our Direct3D Sprite interface

D3DXFONT_DESC d3dFont;
ID3DXFont* font ;

struct CUSTOMTEXTURE{
	LPDIRECT3DTEXTURE9 texture;
	D3DXIMAGE_INFO textureinfo;
};

struct IMAGESIZE {
	int width;
	int height;
};

class GameIntro {
public:
	CUSTOMTEXTURE Homm;
	CUSTOMTEXTURE pressAnyKey;

	bool isIntro = 1;
	~GameIntro() {
		
	}
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

using namespace std;


class GameState {
public:
	bool isMainMenu = 1;
	enum State{Running,InputWaiting};
	State state;

	GameState() {

	}
};

struct Location {

};

class Child {
public: 
		enum Belong{leftHero,rightHero};
		Belong belong ;
		int quantity;
		int virtualLocation_x;
		int virtualLocation_y;
		float realLocation_x;
		float realLocation_y;
		bool active = true;

		bool stateChanged = false;
		//anim
		CUSTOMTEXTURE idle;
		CUSTOMTEXTURE avatar;
		//CUSTOMTEXTURE *currentTexture;
		vector <CUSTOMTEXTURE> selected;
		vector <CUSTOMTEXTURE> move;
		vector <CUSTOMTEXTURE> attack;
		vector <CUSTOMTEXTURE> beAttacked;
		vector <CUSTOMTEXTURE> death;

		enum AnimState{E_selected,E_move,E_attack,E_beAttacked,E_death,E_idle};
		
		AnimState animstate ;
		float Anim_Key ;
		float AnimSpeed_selected = 0.05;
		float AnimSpeed_move = 0.05;
		float AnimSpeed_attack = 0.05;
		float AnimSpeed_beAttacked = 0.05;
		float AnimSpeed_death = 0.05;
		float AnimSpeed_idle = 0;

		LPCWSTR UnitName;
		int melee_attack;
		int remote_attack;
		int health;
		int defense;
		int speed;

		float getAnimSpeedByState() {
			if (animstate == AnimState::E_selected) {
				return AnimSpeed_selected;
			}
			else if (animstate == AnimState::E_move) {
				return AnimSpeed_move;
			}
			else if (animstate == AnimState::E_death) {
				return AnimSpeed_death;
			}
			else if (animstate == AnimState::E_attack) {
				return AnimSpeed_attack;
			}
			else if (animstate == AnimState::E_beAttacked) {
				return AnimSpeed_beAttacked;
			}
			else if (animstate == AnimState::E_idle) {
				return AnimSpeed_idle;
			}
		}

		void playAnim() {
			if (!stateChanged ) {
				Anim_Key += getAnimSpeedByState();
				if (Anim_Key > 1) {
					Anim_Key = 0;
				}
			}
			else if (stateChanged) {
				Anim_Key = 0;
				stateChanged = false;
			}
		}


		CUSTOMTEXTURE getCurrentTexture() {
			CUSTOMTEXTURE currentTexture;
			if (animstate == AnimState::E_selected) {
				currentTexture =   selected[selected.size()*Anim_Key];
			//	currentTexture.textureinfo = selected[(selected.size() + 1)*Anim_Key].textureinfo;
			}
			else if (animstate == AnimState::E_beAttacked) {
				currentTexture = beAttacked[beAttacked.size()*Anim_Key];
			}
			else if (animstate == AnimState::E_attack) {
			currentTexture = attack[attack.size()*Anim_Key];
			}
			else if (animstate == AnimState::E_move) {
				currentTexture = move[move.size()*Anim_Key];
			}
			else if (animstate == AnimState::E_death) {
			currentTexture = death[death.size()*Anim_Key];
			}
			else if (animstate == AnimState::E_idle) {
				currentTexture = idle;
			}


			return currentTexture;
		}
		void realLocationSetting() {
			realLocation_x = toRealLocationX();
			realLocation_y = toRealLocationY();
		}
		float toRealLocationX() {
			if (virtualLocation_y % 2 == 1) {
				return 3 * RADIOUS_HEXAGON / 2 + 3 * RADIOUS_HEXAGON*virtualLocation_x;
			}
			else if (virtualLocation_y % 2 == 0) {
				return 3 * RADIOUS_HEXAGON*virtualLocation_x;
			}
		}
		float toRealLocationY() {
			if (virtualLocation_y % 2 == 1) {
				return sqrt(3)/2 * RADIOUS_HEXAGON*RADIOUS_HEXAGON_FIX + sqrt(3)/2 *RADIOUS_HEXAGON*RADIOUS_HEXAGON_FIX * (virtualLocation_y-1);
			}
			else if (virtualLocation_y % 2 == 0) {
				return  sqrt(3)/2 * RADIOUS_HEXAGON*RADIOUS_HEXAGON_FIX*virtualLocation_y;
			}
		}
};
struct tile {
	Child * P_child;
	bool UnitOnTile;
	float realLocationX;
	float realLocationY;
	float toRealLocationX(int virtualLocation_x,int virtualLocation_y) {
		if (virtualLocation_y % 2 == 1) {
			return 3 * RADIOUS_HEXAGON / 2 + 3 * RADIOUS_HEXAGON*virtualLocation_x;
		}
		else if (virtualLocation_y % 2 == 0) {
			return 3 * RADIOUS_HEXAGON*virtualLocation_x;
		}
	}
	float toRealLocationY(int virtualLocation_x, int virtualLocation_y) {
		if (virtualLocation_y % 2 == 1) {
			return sqrt(3) / 2 * RADIOUS_HEXAGON*RADIOUS_HEXAGON_FIX + sqrt(3)/2 *RADIOUS_HEXAGON*RADIOUS_HEXAGON_FIX * (virtualLocation_y-1);
		}
		else if (virtualLocation_y % 2 == 0) {
			return  sqrt(3) / 2 * RADIOUS_HEXAGON*RADIOUS_HEXAGON_FIX*virtualLocation_y;
		}
	}
};

class Mouse {
public:
	float x;
	float y;

};


vector<vector<tile>> hexMap(17, vector<tile>(10));
 


vector <Child*> turnVector;
vector <Child> unitVector;

Mouse mouse;

struct virtualXY {
	int x;
	int y;
};

class Hero {
public:
	Child *child1;
	Child *child2;
	Child *child3;
	Child *child4;
	Child *child5;
	Child *child6;
	Child *child7;
	Child *child8;
	Hero() {
		child1 = new Child();
		child2 = new Child();
		child3 = new Child();
		child4 = new Child();
		child5 = new Child();
		child6 = new Child();
		child7 = new Child();
		child8 = new Child();

	}
};


class GameObj{
public:
	CUSTOMTEXTURE background;
	CUSTOMTEXTURE hero_left;
	CUSTOMTEXTURE hero_right;
	CUSTOMTEXTURE hexagonMap;
	CUSTOMTEXTURE ui;
	CUSTOMTEXTURE lefthero_frame;
	CUSTOMTEXTURE righthero_frame;
	CUSTOMTEXTURE testPoint;
	CUSTOMTEXTURE tileSelected;
};



// function prototypes
void initD3D(HWND hWnd, GameIntro* intro, GameObj* gameobj, Hero* lefthero, Hero* righthero);    // sets up and initializes Direct3D
void render_frame(GameIntro *intro ,GameObj* gameobj, Hero*hero_left, Hero*hero_right);    // renders a single frame
void cleanD3D(void);		// closes Direct3D and releases memory
void intro_render_frame(GameIntro *intro);
bool cmp(const Child &a, const Child &b);
CUSTOMTEXTURE* setTexture(int width,int height,const wchar_t fileLocation[], CUSTOMTEXTURE* texture);
void paint(LPD3DXSPRITE* P_d3dspt, LPDIRECT3DTEXTURE9 texture, int width, int height,int scale, int x, int y,int alpha);
//void text(LPCWSTR text, int rectX, int rectY, int alpha);
ID3DXSprite* pSprite = NULL;
bool GetImageSize(const char *fn, int *x, int *y);

void GameLogic(GameIntro *intro);

