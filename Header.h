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


#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

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

class Child {
public: 
		enum Belong{leftHero,rightHero};
		Belong belong ;
		int quantity;
		int location_x;
		int location_y;

		//anim
		CUSTOMTEXTURE idle;
		CUSTOMTEXTURE avatar;
		vector <CUSTOMTEXTURE> selected;
		vector <CUSTOMTEXTURE> move;
		vector <CUSTOMTEXTURE> attack;
		vector <CUSTOMTEXTURE> beAttacked;
		vector <CUSTOMTEXTURE> death;

		LPCWSTR UnitName;
		int melee_attack;
		int remote_attack;
		int health;
		int defense;
		int speed;

		
		Child() {
			
		}
};



vector <Child> turnVector;
vector <Child> unitVector;

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
};

// function prototypes
void initD3D(HWND hWnd, GameIntro* intro, GameObj* gameobj, Hero* lefthero, Hero* righthero);    // sets up and initializes Direct3D
void render_frame(GameIntro *intro ,GameObj* gameobj, Hero*hero_left, Hero*hero_right);    // renders a single frame
void cleanD3D(void);		// closes Direct3D and releases memory
void intro_render_frame(GameIntro *intro);
bool cmp(const Child &a, const Child &b);
CUSTOMTEXTURE* setTexture(int width,int height,const wchar_t fileLocation[], CUSTOMTEXTURE* texture);
void paint(LPD3DXSPRITE* P_d3dspt, LPDIRECT3DTEXTURE9 texture, int width, int height, int x, int y,int alpha);
//void text(LPCWSTR text, int rectX, int rectY, int alpha);
ID3DXSprite* pSprite = NULL;
bool GetImageSize(const char *fn, int *x, int *y);

