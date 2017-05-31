#include "Header.h"



// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";
	
	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL, L"WindowClass", L"Our Direct3D Program",
		WS_EX_TOPMOST | WS_POPUP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);

	// set up and initialize Direct3D
	GameIntro* intro = new GameIntro();
	GameObj* gameobj = new GameObj();
	Hero* hero_left = new Hero();
	Hero* hero_right = new Hero();

	initD3D(hWnd, intro,gameobj, hero_left, hero_right);

	// enter the main loop:

	MSG msg;

	while (TRUE)
	{
		DWORD starting_point = GetTickCount();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		intro_render_frame(intro);
		GameLogic(intro);
		render_frame(intro,gameobj, hero_left, hero_right);
		if (KEY_DOWN(VK_RETURN)) {
			intro->isIntro = 0;
			
		}
		// check the 'escape' key
		if (KEY_DOWN(VK_ESCAPE))
			break;

		while ((GetTickCount() - starting_point) < 25);
	}

	// clean up DirectX and COM
	cleanD3D();

	return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	 
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	} break;
	/*case WM_LBUTTONDOWN:
	{
		PostQuitMessage(0);
		return 0;
	} break;*/
	case WM_MOUSEMOVE:
	{
		
		mouse.x = LOWORD(lParam);
		mouse.y = HIWORD(lParam);
		
	}break;

	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

bool cmp(const Child &a, const Child &b) {
	return a.speed>b.speed;
}


bool GetImageSize(const char *fn, int *x, int *y)
{
	FILE *f = fopen(fn, "rb"); if (f == 0) return false;
	fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
	if (len<24) { fclose(f); return false; }

	// Strategy:
	// reading GIF dimensions requires the first 10 bytes of the file
	// reading PNG dimensions requires the first 24 bytes of the file
	// reading JPEG dimensions requires scanning through jpeg chunks
	// In all formats, the file is at least 24 bytes big, so we'll read that always
	unsigned char buf[24]; fread(buf, 1, 24, f);

	// For JPEGs, we need to read the first 12 bytes of each chunk.
	// We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
	if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE0 && buf[6] == 'J' && buf[7] == 'F' && buf[8] == 'I' && buf[9] == 'F')
	{
		long pos = 2;
		while (buf[2] == 0xFF)
		{
			if (buf[3] == 0xC0 || buf[3] == 0xC1 || buf[3] == 0xC2 || buf[3] == 0xC3 || buf[3] == 0xC9 || buf[3] == 0xCA || buf[3] == 0xCB) break;
			pos += 2 + (buf[4] << 8) + buf[5];
			if (pos + 12>len) break;
			fseek(f, pos, SEEK_SET); fread(buf + 2, 1, 12, f);
		}
	}

	fclose(f);

	// JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
	if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF)
	{
		*y = (buf[7] << 8) + buf[8];
		*x = (buf[9] << 8) + buf[10];
		return true;
	}

	// GIF: first three bytes say "GIF", next three give version number. Then dimensions
	if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F')
	{
		*x = buf[6] + (buf[7] << 8);
		*y = buf[8] + (buf[9] << 8);
		return true;
	}

	// PNG: the first frame is by definition an IHDR frame, which gives dimensions
	if (buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' && buf[4] == 0x0D && buf[5] == 0x0A && buf[6] == 0x1A && buf[7] == 0x0A
		&& buf[12] == 'I' && buf[13] == 'H' && buf[14] == 'D' && buf[15] == 'R')
	{
		*x = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + (buf[19] << 0);
		*y = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + (buf[23] << 0);
		return true;
	}

	return false;
}


CUSTOMTEXTURE* setTexture(int width, int height, const wchar_t fileLocation[], CUSTOMTEXTURE* cusTexture) {
	D3DXCreateTextureFromFileEx(d3ddev,    // the device pointer
		fileLocation,    // the file name
		width,    // default width
		height,    // default height
		D3DX_DEFAULT,    // no mip mapping
		NULL,    // regular usage
		D3DFMT_A8R8G8B8,    // 32-bit pixels with alpha
		D3DPOOL_MANAGED,    // typical memory handling
		D3DX_DEFAULT,    // no filtering
		D3DX_DEFAULT,    // no mip filtering
		D3DCOLOR_XRGB(255, 0, 255),    // the hot-pink color key
		&(*cusTexture).textureinfo,    // no image info struct
		NULL,    // not using 256 colors
		&(*cusTexture).texture);    // load to sprite

	return cusTexture;
}

void GameLogic(GameIntro *intro) {
	if (intro->isIntro != 0) {
		return;
	}
	//every turn set the front of vector'schild as selected
	//

	

	for (int i = 0; i < unitVector.size(); i++) {
		unitVector[i].playAnim();
	//	unitVector[i].Anim_Key;
	}

	unitVector[0].Anim_Key;
}

void initD3D(HWND hWnd, GameIntro* intro, GameObj* gameobj, Hero* hero_left,Hero* hero_right)
{
	

#pragma region initD3D


	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = SCREEN_WIDTH;
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;


	// create a device class using this information and the info from the d3dpp stuct
	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);

	D3DXCreateSprite(d3ddev, &d3dspt);    // create the Direct3D Sprite object

#pragma endregion


#pragma region initFont
										
	memset(&d3dFont, 0, sizeof(d3dFont));
	d3dFont.Height = 25; // in logical units
	d3dFont.Width = 12;  // in logical units
	d3dFont.Weight = 500;// boldness, range 0(light) - 1000(bold)
	d3dFont.Italic = FALSE;
	d3dFont.CharSet = DEFAULT_CHARSET;
	//may cause problem
	wcscpy(d3dFont.FaceName, L"Times New Roman");

	D3DXCreateFontIndirect(d3ddev, &d3dFont, &font);

#pragma endregion

	D3DXCreateSprite(d3ddev, &pSprite);

#pragma region IntroSprite

	setTexture(1280, 720, L"./intro/homm.jpg", &intro->Homm);

	setTexture(805, 66, L"./intro/pressAnyKey.png", &intro->pressAnyKey);
#pragma endregion

#pragma region background&hero&UI_sprite

	
	setTexture(1280, 720, L"./bg&hero&ui/background.png", &gameobj->background);

	setTexture(151, 166, L"./bg&hero&ui/heroleft.png", &gameobj->hero_left);
	
	setTexture(151, 166, L"./bg&hero&ui/heroright.png", &gameobj->hero_right);
	
	setTexture(1280, 538, L"./bg&hero&ui/hexagonmap.png", &gameobj->hexagonMap);
	
	setTexture(1280, 42, L"./bg&hero&ui/ui.jpg", &gameobj->ui);
#pragma endregion

#pragma region InitUnits&turnlist
	
	
	#pragma region ChildSetting

	int imageX, imageY;
	
		hero_left->child1->belong = Child::Belong::leftHero;
		hero_left->child1->quantity = 20;
		
		hero_left->child2->belong = Child::Belong::leftHero;
		hero_left->child2->quantity = 30;
		hero_left->child3->belong = Child::Belong::leftHero;
		hero_left->child3->quantity = 40;
		hero_left->child4->belong = Child::Belong::leftHero;
		hero_left->child4->quantity = 20;
		hero_left->child5->belong = Child::Belong::leftHero;
		hero_left->child5->quantity = 10;
		hero_left->child6->belong = Child::Belong::leftHero;
		hero_left->child6->quantity = 40;
		hero_left->child7->belong = Child::Belong::leftHero;
		hero_left->child7->quantity = 30;
		hero_left->child8->belong = Child::Belong::leftHero;
		hero_left->child8->quantity = 20;

		hero_right->child1->belong = Child::Belong::rightHero;
		hero_right->child1->quantity = 20;		 
		hero_right->child2->belong = Child::Belong::rightHero;
		hero_right->child2->quantity = 30;		   
		hero_right->child3->belong = Child::Belong::rightHero;
		hero_right->child3->quantity = 40;		   
		hero_right->child4->belong = Child::Belong::rightHero;
		hero_right->child4->quantity = 20;		   
		hero_right->child5->belong = Child::Belong::rightHero;
		hero_right->child5->quantity = 10;		   
		hero_right->child6->belong = Child::Belong::rightHero;
		hero_right->child6->quantity = 40;		   
		hero_right->child7->belong = Child::Belong::rightHero;
		hero_right->child7->quantity = 30;		   
		hero_right->child8->belong = Child::Belong::rightHero;
		hero_right->child8->quantity = 20;

		

		hero_left->child1->virtualLocation_x = 0;
		hero_left->child1->virtualLocation_y = 2;
		hero_left->child2->virtualLocation_x = 1;
		hero_left->child2->virtualLocation_y = 2;

		hero_left->child3->virtualLocation_x = 0;
		hero_left->child3->virtualLocation_y = 4;
		hero_left->child4->virtualLocation_x = 1;
		hero_left->child4->virtualLocation_y = 4;

		hero_left->child5->virtualLocation_x = 0;
		hero_left->child5->virtualLocation_y = 6;
		hero_left->child6->virtualLocation_x = 1;
		hero_left->child6->virtualLocation_y = 6;

		hero_left->child7->virtualLocation_x = 0;
		hero_left->child7->virtualLocation_y = 8;
		hero_left->child8->virtualLocation_x = 1;
		hero_left->child8->virtualLocation_y = 8;

		hero_right->child1->virtualLocation_x = 8;
		hero_right->child1->virtualLocation_y = 2;
		hero_right->child2->virtualLocation_x = 9;
		hero_right->child2->virtualLocation_y = 2;

		hero_right->child3->virtualLocation_x = 8;
		hero_right->child3->virtualLocation_y = 4;
		hero_right->child4->virtualLocation_x = 9;
		hero_right->child4->virtualLocation_y = 4;

		hero_right->child5->virtualLocation_x = 8;
		hero_right->child5->virtualLocation_y = 6;
		hero_right->child6->virtualLocation_x = 9;
		hero_right->child6->virtualLocation_y = 6;

		hero_right->child7->virtualLocation_x = 8;
		hero_right->child7->virtualLocation_y = 8;
		hero_right->child8->virtualLocation_x = 9;
		hero_right->child8->virtualLocation_y = 8;



#pragma endregion


	#pragma region hero_left_child1_DreadKnight

	
		
	
	GetImageSize("./leftchild1_DreadKnight/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild1_DreadKnight/idle.png", &hero_left->child1->idle);
	GetImageSize("./leftchild1_DreadKnight/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild1_DreadKnight/avatar.png",&hero_left->child1->avatar);
		
	GetImageSize("./leftchild1_DreadKnight/selected_1.png",&imageX,&imageY);
	hero_left->child1->selected.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/selected_2.png", &imageX, &imageY);
	hero_left->child1->selected.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/selected_3.png", &imageX, &imageY);
	hero_left->child1->selected.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/selected_4.png", &imageX, &imageY);
	hero_left->child1->selected.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/selected_4.png", new CUSTOMTEXTURE));
	
	
	GetImageSize("./leftchild1_DreadKnight/move_1.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_2.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_3.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_4.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_5.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_6.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_7.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_8.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_9.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_10.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_10.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_11.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_11.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/move_12.png", &imageX, &imageY);
	hero_left->child1->move.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/move_12.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild1_DreadKnight/attack_1.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/attack_2.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/attack_3.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/attack_4.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/attack_5.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/attack_6.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/attack_7.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/attack_8.png", &imageX, &imageY);
	hero_left->child1->attack.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/attack_8.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild1_DreadKnight/beAttacked_1.png", &imageX, &imageY);
	hero_left->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/beAttacked_2.png", &imageX, &imageY);
	hero_left->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/beAttacked_3.png", &imageX, &imageY);
	hero_left->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/beAttacked_4.png", &imageX, &imageY);
	hero_left->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/beAttacked_5.png", &imageX, &imageY);
	hero_left->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/beAttacked_6.png", &imageX, &imageY);
	hero_left->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild1_DreadKnight/death_1.png", &imageX, &imageY);
	hero_left->child1->death.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/death_2.png", &imageX, &imageY);
	hero_left->child1->death.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/death_3.png", &imageX, &imageY);
	hero_left->child1->death.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/death_4.png", &imageX, &imageY);
	hero_left->child1->death.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/death_5.png", &imageX, &imageY);
	hero_left->child1->death.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild1_DreadKnight/death_6.png", &imageX, &imageY);
	hero_left->child1->death.push_back(*setTexture(imageX, imageY, L"./leftchild1_DreadKnight/death_6.png", new CUSTOMTEXTURE));

	hero_left->child1->UnitName = L"DreadKnight";
	hero_left->child1->melee_attack = 0;
	hero_left->child1->remote_attack = 30;
	hero_left->child1->health = 120;
	hero_left->child1->defense = 18;
	hero_left->child1->speed = 9;

	#pragma endregion

	#pragma region hero_left_child2_Enchanter
	
	GetImageSize("./leftchild2_Enchanter/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild2_Enchanter/idle.png", &hero_left->child2->idle);
	GetImageSize("./leftchild2_Enchanter/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild2_Enchanter/avatar.png", &hero_left->child2->avatar);
	
	GetImageSize("./leftchild2_Enchanter/selected_1.png", &imageX, &imageY);
	hero_left->child2->selected.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/selected_2.png", &imageX, &imageY);
	hero_left->child2->selected.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/selected_3.png", &imageX, &imageY);
	hero_left->child2->selected.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/selected_3.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild2_Enchanter/move_1.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_2.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_3.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_4.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_5.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_6.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_7.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_8.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/move_9.png", &imageX, &imageY);
	hero_left->child2->move.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/move_9.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild2_Enchanter/attack_1.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_2.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_3.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_4.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_5.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_6.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_7.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_8.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/attack_9.png", &imageX, &imageY);
	hero_left->child2->attack.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/attack_9.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild2_Enchanter/beAttacked_1.png", &imageX, &imageY);
	hero_left->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/beAttacked_2.png", &imageX, &imageY);
	hero_left->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/beAttacked_3.png", &imageX, &imageY);
	hero_left->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/beAttacked_4.png", &imageX, &imageY);
	hero_left->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/beAttacked_5.png", &imageX, &imageY);
	hero_left->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/beAttacked_5.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild2_Enchanter/death_1.png", &imageX, &imageY);
	hero_left->child2->death.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/death_2.png", &imageX, &imageY);
	hero_left->child2->death.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/death_3.png", &imageX, &imageY);
	hero_left->child2->death.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/death_4.png", &imageX, &imageY);
	hero_left->child2->death.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/death_5.png", &imageX, &imageY);
	hero_left->child2->death.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild2_Enchanter/death_6.png", &imageX, &imageY);
	hero_left->child2->death.push_back(*setTexture(imageX, imageY, L"./leftchild2_Enchanter/death_6.png", new CUSTOMTEXTURE));


	hero_left->child2->UnitName = L"Enchanter";
	hero_left->child2->melee_attack = 17;
	hero_left->child2->remote_attack = 17;
	hero_left->child2->health = 30;
	hero_left->child2->defense = 12;
	hero_left->child2->speed = 9;


#pragma endregion

	#pragma region hero_left_child3_Champion

	GetImageSize("./leftchild3_Champion/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild3_Champion/idle.png", &hero_left->child3->idle);
	GetImageSize("./leftchild3_Champion/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild3_Champion/avatar.png", &hero_left->child3->avatar);

	GetImageSize("./leftchild3_Champion/selected_1.png", &imageX, &imageY);
	hero_left->child3->selected.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/selected_2.png", &imageX, &imageY);
	hero_left->child3->selected.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/selected_3.png", &imageX, &imageY);
	hero_left->child3->selected.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/selected_4.png", &imageX, &imageY);
	hero_left->child3->selected.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/selected_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/selected_5.png", &imageX, &imageY);
	hero_left->child3->selected.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/selected_5.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild3_Champion/move_1.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_2.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_3.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_4.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_5.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_6.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_7.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_8.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_9.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_10.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_10.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_11.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_11.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/move_12.png", &imageX, &imageY);
	hero_left->child3->move.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/move_12.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild3_Champion/attack_1.png", &imageX, &imageY);
	hero_left->child3->attack.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/attack_2.png", &imageX, &imageY);
	hero_left->child3->attack.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/attack_3.png", &imageX, &imageY);
	hero_left->child3->attack.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/attack_3.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild3_Champion/beAttacked_1.png", &imageX, &imageY);
	hero_left->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/beAttacked_2.png", &imageX, &imageY);
	hero_left->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/beAttacked_3.png", &imageX, &imageY);
	hero_left->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/beAttacked_4.png", &imageX, &imageY);
	hero_left->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/beAttacked_5.png", &imageX, &imageY);
	hero_left->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/beAttacked_5.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild3_Champion/death_1.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/death_2.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/death_3.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/death_4.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/death_5.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/death_6.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/death_7.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild3_Champion/death_8.png", &imageX, &imageY);
	hero_left->child3->death.push_back(*setTexture(imageX, imageY, L"./leftchild3_Champion/death_8.png", new CUSTOMTEXTURE));


	hero_left->child3->UnitName = L"Champion";
	hero_left->child3->melee_attack = 16;
	hero_left->child3->remote_attack = 0;
	hero_left->child3->health = 100;
	hero_left->child3->defense = 15;
	hero_left->child3->speed = 9;



#pragma endregion

	#pragma region hero_left_child4_Phoenix
	GetImageSize("./leftchild4_Phoenix/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild4_Phoenix/idle.png", &hero_left->child4->idle);
	GetImageSize("./leftchild4_Phoenix/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild4_Phoenix/avatar.png", &hero_left->child4->avatar);

	GetImageSize("./leftchild4_Phoenix/selected_1.png", &imageX, &imageY);
	hero_left->child4->selected.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/selected_2.png", &imageX, &imageY);
	hero_left->child4->selected.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/selected_3.png", &imageX, &imageY);
	hero_left->child4->selected.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/selected_4.png", &imageX, &imageY);
	hero_left->child4->selected.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/selected_4.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild4_Phoenix/move_1.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/move_2.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/move_3.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/move_4.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/move_5.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/move_6.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/move_7.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/move_8.png", &imageX, &imageY);
	hero_left->child4->move.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/move_8.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild4_Phoenix/attack_1.png", &imageX, &imageY);
	hero_left->child4->attack.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/attack_2.png", &imageX, &imageY);
	hero_left->child4->attack.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/attack_3.png", &imageX, &imageY);
	hero_left->child4->attack.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/attack_4.png", &imageX, &imageY);
	hero_left->child4->attack.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/attack_5.png", &imageX, &imageY);
	hero_left->child4->attack.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/attack_6.png", &imageX, &imageY);
	hero_left->child4->attack.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/attack_7.png", &imageX, &imageY);
	hero_left->child4->attack.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/attack_7.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild4_Phoenix/beAttacked_1.png", &imageX, &imageY);
	hero_left->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/beAttacked_2.png", &imageX, &imageY);
	hero_left->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/beAttacked_3.png", &imageX, &imageY);
	hero_left->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/beAttacked_4.png", &imageX, &imageY);
	hero_left->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/beAttacked_5.png", &imageX, &imageY);
	hero_left->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/beAttacked_6.png", &imageX, &imageY);
	hero_left->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/beAttacked_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/beAttacked_7.png", &imageX, &imageY);
	hero_left->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/beAttacked_7.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild4_Phoenix/death_1.png", &imageX, &imageY);
	hero_left->child4->death.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/death_2.png", &imageX, &imageY);
	hero_left->child4->death.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/death_3.png", &imageX, &imageY);
	hero_left->child4->death.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/death_4.png", &imageX, &imageY);
	hero_left->child4->death.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/death_5.png", &imageX, &imageY);
	hero_left->child4->death.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/death_6.png", &imageX, &imageY);
	hero_left->child4->death.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/death_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild4_Phoenix/death_7.png", &imageX, &imageY);
	hero_left->child4->death.push_back(*setTexture(imageX, imageY, L"./leftchild4_Phoenix/death_7.png", new CUSTOMTEXTURE));


	hero_left->child4->UnitName = L"Phoenix";
	hero_left->child4->melee_attack = 40;
	hero_left->child4->remote_attack = 0;
	hero_left->child4->health = 200;
	hero_left->child4->defense = 18;
	hero_left->child4->speed = 21;


#pragma endregion

	#pragma region hero_left_child5_MightyGorgon
	GetImageSize("./leftchild5_MightyGorgon/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/idle.png", &hero_left->child5->idle);
	GetImageSize("./leftchild5_MightyGorgon/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/avatar.png", &hero_left->child5->avatar);

	GetImageSize("./leftchild5_MightyGorgon/selected_1.png", &imageX, &imageY);
	hero_left->child5->selected.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/selected_2.png", &imageX, &imageY);
	hero_left->child5->selected.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/selected_3.png", &imageX, &imageY);
	hero_left->child5->selected.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/selected_3.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild5_MightyGorgon/move_1.png", &imageX, &imageY);
	hero_left->child5->move.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/move_2.png", &imageX, &imageY);
	hero_left->child5->move.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/move_3.png", &imageX, &imageY);
	hero_left->child5->move.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/move_4.png", &imageX, &imageY);
	hero_left->child5->move.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/move_5.png", &imageX, &imageY);
	hero_left->child5->move.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/move_6.png", &imageX, &imageY);
	hero_left->child5->move.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/move_6.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild5_MightyGorgon/attack_1.png", &imageX, &imageY);
	hero_left->child5->attack.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/attack_2.png", &imageX, &imageY);
	hero_left->child5->attack.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/attack_3.png", &imageX, &imageY);
	hero_left->child5->attack.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/attack_4.png", &imageX, &imageY);
	hero_left->child5->attack.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/attack_4.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild5_MightyGorgon/beAttacked_1.png", &imageX, &imageY);
	hero_left->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/beAttacked_2.png", &imageX, &imageY);
	hero_left->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/beAttacked_3.png", &imageX, &imageY);
	hero_left->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/beAttacked_4.png", &imageX, &imageY);
	hero_left->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/beAttacked_5.png", &imageX, &imageY);
	hero_left->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/beAttacked_6.png", &imageX, &imageY);
	hero_left->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild5_MightyGorgon/death_1.png", &imageX, &imageY);
	hero_left->child5->death.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/death_2.png", &imageX, &imageY);
	hero_left->child5->death.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/death_3.png", &imageX, &imageY);
	hero_left->child5->death.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/death_4.png", &imageX, &imageY);
	hero_left->child5->death.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild5_MightyGorgon/death_5.png", &imageX, &imageY);
	hero_left->child5->death.push_back(*setTexture(imageX, imageY, L"./leftchild5_MightyGorgon/death_5.png", new CUSTOMTEXTURE));

	hero_left->child5->UnitName = L"MightyGorgon";
	hero_left->child5->melee_attack = 16;
	hero_left->child5->remote_attack = 0;
	hero_left->child5->health = 70;
	hero_left->child5->defense = 16;
	hero_left->child5->speed = 6;


#pragma endregion

	#pragma region hero_left_child6_Sharpshooter
	GetImageSize("./leftchild6_Sharpshooter/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/idle.png", &hero_left->child6->idle);
	GetImageSize("./leftchild6_Sharpshooter/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/avatar.png", &hero_left->child6->avatar);

	GetImageSize("./leftchild6_Sharpshooter/selected_1.png", &imageX, &imageY);
	hero_left->child6->selected.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/selected_2.png", &imageX, &imageY);
	hero_left->child6->selected.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/selected_3.png", &imageX, &imageY);
	hero_left->child6->selected.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/selected_4.png", &imageX, &imageY);
	hero_left->child6->selected.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/selected_4.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild6_Sharpshooter/move_1.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_2.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_3.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_4.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_5.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_6.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_7.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_8.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_9.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_10.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_10.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_11.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_11.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/move_12.png", &imageX, &imageY);
	hero_left->child6->move.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/move_12.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild6_Sharpshooter/attack_1.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/attack_2.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/attack_3.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/attack_4.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/attack_5.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/attack_6.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/attack_7.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/attack_8.png", &imageX, &imageY);
	hero_left->child6->attack.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/attack_8.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild6_Sharpshooter/beAttacked_1.png", &imageX, &imageY);
	hero_left->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/beAttacked_2.png", &imageX, &imageY);
	hero_left->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/beAttacked_3.png", &imageX, &imageY);
	hero_left->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/beAttacked_4.png", &imageX, &imageY);
	hero_left->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/beAttacked_5.png", &imageX, &imageY);
	hero_left->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/beAttacked_6.png", &imageX, &imageY);
	hero_left->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild6_Sharpshooter/death_1.png", &imageX, &imageY);
	hero_left->child6->death.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/death_2.png", &imageX, &imageY);
	hero_left->child6->death.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/death_3.png", &imageX, &imageY);
	hero_left->child6->death.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/death_4.png", &imageX, &imageY);
	hero_left->child6->death.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild6_Sharpshooter/death_5.png", &imageX, &imageY);
	hero_left->child6->death.push_back(*setTexture(imageX, imageY, L"./leftchild6_Sharpshooter/death_5.png", new CUSTOMTEXTURE));

	hero_left->child6->UnitName = L"Sharpshooter";
	hero_left->child6->melee_attack = 10;
	hero_left->child6->remote_attack = 10;
	hero_left->child6->health = 15;
	hero_left->child6->defense = 10;
	hero_left->child6->speed = 9;

#pragma endregion

	#pragma region hero_left_child7_Archangel
	GetImageSize("./leftchild7_Archangel/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild7_Archangel/idle.png", &hero_left->child7->idle);
	GetImageSize("./leftchild7_Archangel/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild7_Archangel/avatar.png", &hero_left->child7->avatar);

	GetImageSize("./leftchild7_Archangel/selected_1.png", &imageX, &imageY);
	hero_left->child7->selected.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/selected_2.png", &imageX, &imageY);
	hero_left->child7->selected.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/selected_3.png", &imageX, &imageY);
	hero_left->child7->selected.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/selected_4.png", &imageX, &imageY);
	hero_left->child7->selected.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/selected_4.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild7_Archangel/move_1.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_2.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_3.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_4.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_5.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_6.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_7.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_8.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_9.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_10.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_10.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/move_11.png", &imageX, &imageY);
	hero_left->child7->move.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/move_11.png", new CUSTOMTEXTURE));


	GetImageSize("./leftchild7_Archangel/attack_1.png", &imageX, &imageY);
	hero_left->child7->attack.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/attack_2.png", &imageX, &imageY);
	hero_left->child7->attack.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/attack_3.png", &imageX, &imageY);
	hero_left->child7->attack.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/attack_4.png", &imageX, &imageY);
	hero_left->child7->attack.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/attack_5.png", &imageX, &imageY);
	hero_left->child7->attack.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/attack_6.png", &imageX, &imageY);
	hero_left->child7->attack.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/attack_7.png", &imageX, &imageY);
	hero_left->child7->attack.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/attack_7.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild7_Archangel/beAttacked_1.png", &imageX, &imageY);
	hero_left->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/beAttacked_2.png", &imageX, &imageY);
	hero_left->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/beAttacked_3.png", &imageX, &imageY);
	hero_left->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/beAttacked_4.png", &imageX, &imageY);
	hero_left->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/beAttacked_5.png", &imageX, &imageY);
	hero_left->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/beAttacked_6.png", &imageX, &imageY);
	hero_left->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild7_Archangel/death_1.png", &imageX, &imageY);
	hero_left->child7->death.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/death_2.png", &imageX, &imageY);
	hero_left->child7->death.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/death_3.png", &imageX, &imageY);
	hero_left->child7->death.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/death_4.png", &imageX, &imageY);
	hero_left->child7->death.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/death_5.png", &imageX, &imageY);
	hero_left->child7->death.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild7_Archangel/death_6.png", &imageX, &imageY);
	hero_left->child7->death.push_back(*setTexture(imageX, imageY, L"./leftchild7_Archangel/death_6.png", new CUSTOMTEXTURE));


	hero_left->child7->UnitName = L"Archangel";
	hero_left->child7->melee_attack = 30;
	hero_left->child7->remote_attack = 0;
	hero_left->child7->health = 250;
	hero_left->child7->defense = 30;
	hero_left->child7->speed = 18;

#pragma endregion

	#pragma region hero_left_child8_ArchDevil
	GetImageSize("./leftchild8_ArchDevil/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild8_ArchDevil/idle.png", &hero_left->child8->idle);
	GetImageSize("./leftchild8_ArchDevil/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./leftchild8_ArchDevil/avatar.png", &hero_left->child8->avatar);
	
	GetImageSize("./leftchild8_ArchDevil/selected_1.png", &imageX, &imageY);
	hero_left->child8->selected.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/selected_2.png", &imageX, &imageY);
	hero_left->child8->selected.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/selected_3.png", &imageX, &imageY);
	hero_left->child8->selected.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/selected_4.png", &imageX, &imageY);
	hero_left->child8->selected.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/selected_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/selected_5.png", &imageX, &imageY);
	hero_left->child8->selected.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/selected_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/selected_6.png", &imageX, &imageY);
	hero_left->child8->selected.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/selected_6.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild8_ArchDevil/move_1.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_2.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_3.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_4.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_5.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_6.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_7.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_8.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_9.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_10.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_10.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/move_11.png", &imageX, &imageY);
	hero_left->child8->move.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/move_11.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild8_ArchDevil/attack_1.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/attack_2.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/attack_3.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/attack_4.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/attack_5.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/attack_6.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/attack_7.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/attack_8.png", &imageX, &imageY);
	hero_left->child8->attack.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/attack_8.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild8_ArchDevil/beAttacked_1.png", &imageX, &imageY);
	hero_left->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/beAttacked_2.png", &imageX, &imageY);
	hero_left->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/beAttacked_3.png", &imageX, &imageY);
	hero_left->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/beAttacked_4.png", &imageX, &imageY);
	hero_left->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/beAttacked_5.png", &imageX, &imageY);
	hero_left->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/beAttacked_5.png", new CUSTOMTEXTURE));

	GetImageSize("./leftchild8_ArchDevil/death_1.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_2.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_3.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_4.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_5.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_6.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_6.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_7.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_7.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_8.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_8.png", new CUSTOMTEXTURE));
	GetImageSize("./leftchild8_ArchDevil/death_9.png", &imageX, &imageY);
	hero_left->child8->death.push_back(*setTexture(imageX, imageY, L"./leftchild8_ArchDevil/death_9.png", new CUSTOMTEXTURE));



	hero_left->child8->UnitName = L"ArchDevil";
	hero_left->child8->melee_attack = 40;
	hero_left->child8->remote_attack = 0;
	hero_left->child8->health = 200;
	hero_left->child8->defense = 28;
	hero_left->child8->speed = 17;


#pragma endregion

	#pragma region hero_right_child1_ArchDevil
	GetImageSize("./rightchild1_AncientBehemoth/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/idle.png", &hero_right->child1->idle);
	GetImageSize("./rightchild1_AncientBehemoth/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/avatar.png", &hero_right->child1->avatar);

	GetImageSize("./rightchild1_AncientBehemoth/selected_1.png", &imageX, &imageY);
	hero_right->child1->selected.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/selected_2.png", &imageX, &imageY);
	hero_right->child1->selected.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/selected_3.png", &imageX, &imageY);
	hero_right->child1->selected.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/selected_4.png", &imageX, &imageY);
	hero_right->child1->selected.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/selected_4.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild1_AncientBehemoth/move_1.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_2.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_3.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_4.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_5.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_6.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_7.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_8.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/move_9.png", &imageX, &imageY);
	hero_right->child1->move.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/move_9.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild1_AncientBehemoth/attack_1.png", &imageX, &imageY);
	hero_right->child1->attack.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/attack_2.png", &imageX, &imageY);
	hero_right->child1->attack.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/attack_3.png", &imageX, &imageY);
	hero_right->child1->attack.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/attack_4.png", &imageX, &imageY);
	hero_right->child1->attack.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/attack_5.png", &imageX, &imageY);
	hero_right->child1->attack.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/attack_6.png", &imageX, &imageY);
	hero_right->child1->attack.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/attack_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild1_AncientBehemoth/beAttacked_1.png", &imageX, &imageY);
	hero_right->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/beAttacked_2.png", &imageX, &imageY);
	hero_right->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/beAttacked_3.png", &imageX, &imageY);
	hero_right->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/beAttacked_4.png", &imageX, &imageY);
	hero_right->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/beAttacked_5.png", &imageX, &imageY);
	hero_right->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/beAttacked_6.png", &imageX, &imageY);
	hero_right->child1->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild1_AncientBehemoth/death_1.png", &imageX, &imageY);
	hero_right->child1->death.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/death_2.png", &imageX, &imageY);
	hero_right->child1->death.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/death_3.png", &imageX, &imageY);
	hero_right->child1->death.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/death_4.png", &imageX, &imageY);
	hero_right->child1->death.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/death_5.png", &imageX, &imageY);
	hero_right->child1->death.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild1_AncientBehemoth/death_6.png", &imageX, &imageY);
	hero_right->child1->death.push_back(*setTexture(imageX, imageY, L"./rightchild1_AncientBehemoth/death_6.png", new CUSTOMTEXTURE));


	hero_right->child1->UnitName = L"AncientBehemoth";
	hero_right->child1->melee_attack = 50;
	hero_right->child1->remote_attack = 0;
	hero_right->child1->health = 300;
	hero_right->child1->defense = 19;
	hero_right->child1->speed = 9;

#pragma endregion

	#pragma region hero_right_child2_Peasant
	GetImageSize("./rightchild2_Peasant/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild2_Peasant/idle.png", &hero_right->child2->idle);
	GetImageSize("./rightchild2_Peasant/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild2_Peasant/avatar.png", &hero_right->child2->avatar);

	GetImageSize("./rightchild2_Peasant/selected_1.png", &imageX, &imageY);
	hero_right->child2->selected.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/selected_2.png", &imageX, &imageY);
	hero_right->child2->selected.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/selected_3.png", &imageX, &imageY);
	hero_right->child2->selected.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/selected_4.png", &imageX, &imageY);
	hero_right->child2->selected.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/selected_4.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild2_Peasant/move_1.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/move_2.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/move_3.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/move_4.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/move_5.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/move_6.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/move_7.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/move_8.png", &imageX, &imageY);
	hero_right->child2->move.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/move_8.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild2_Peasant/attack_1.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/attack_2.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/attack_3.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/attack_4.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/attack_5.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/attack_6.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/attack_7.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/attack_7.png", &imageX, &imageY);
	hero_right->child2->attack.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/attack_7.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild2_Peasant/beAttacked_1.png", &imageX, &imageY);
	hero_right->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/beAttacked_2.png", &imageX, &imageY);
	hero_right->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/beAttacked_3.png", &imageX, &imageY);
	hero_right->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/beAttacked_4.png", &imageX, &imageY);
	hero_right->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/beAttacked_5.png", &imageX, &imageY);
	hero_right->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/beAttacked_6.png", &imageX, &imageY);
	hero_right->child2->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild2_Peasant/death_1.png", &imageX, &imageY);
	hero_right->child2->death.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/death_2.png", &imageX, &imageY);
	hero_right->child2->death.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/death_3.png", &imageX, &imageY);
	hero_right->child2->death.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/death_4.png", &imageX, &imageY);
	hero_right->child2->death.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/death_5.png", &imageX, &imageY);
	hero_right->child2->death.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild2_Peasant/death_6.png", &imageX, &imageY);
	hero_right->child2->death.push_back(*setTexture(imageX, imageY, L"./rightchild2_Peasant/death_6.png", new CUSTOMTEXTURE));

	hero_right->child2->UnitName = L"Peasant";
	hero_right->child2->melee_attack = 1;
	hero_right->child2->remote_attack = 0;
	hero_right->child2->health = 1;
	hero_right->child2->defense = 1;
	hero_right->child2->speed = 3;



#pragma endregion

	#pragma region hero_right_child3_Crusader
	GetImageSize("./rightchild3_Crusader/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild3_Crusader/idle.png", &hero_right->child3->idle);
	GetImageSize("./rightchild3_Crusader/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild3_Crusader/avatar.png", &hero_right->child3->avatar);

	GetImageSize("./rightchild3_Crusader/selected_1.png", &imageX, &imageY);
	hero_right->child3->selected.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/selected_2.png", &imageX, &imageY);
	hero_right->child3->selected.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/selected_3.png", &imageX, &imageY);
	hero_right->child3->selected.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/selected_3.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild3_Crusader/move_1.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_2.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_3.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_4.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_5.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_6.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_7.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_8.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_9.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_10.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_10.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_11.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_11.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/move_12.png", &imageX, &imageY);
	hero_right->child3->move.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/move_12.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild3_Crusader/attack_1.png", &imageX, &imageY);
	hero_right->child3->attack.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/attack_2.png", &imageX, &imageY);
	hero_right->child3->attack.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/attack_3.png", &imageX, &imageY);
	hero_right->child3->attack.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/attack_4.png", &imageX, &imageY);
	hero_right->child3->attack.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/attack_4.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild3_Crusader/beAttacked_1.png", &imageX, &imageY);
	hero_right->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/beAttacked_2.png", &imageX, &imageY);
	hero_right->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/beAttacked_3.png", &imageX, &imageY);
	hero_right->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/beAttacked_4.png", &imageX, &imageY);
	hero_right->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/beAttacked_5.png", &imageX, &imageY);
	hero_right->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/beAttacked_6.png", &imageX, &imageY);
	hero_right->child3->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild3_Crusader/death_1.png", &imageX, &imageY);
	hero_right->child3->death.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/death_2.png", &imageX, &imageY);
	hero_right->child3->death.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/death_3.png", &imageX, &imageY);
	hero_right->child3->death.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/death_4.png", &imageX, &imageY);
	hero_right->child3->death.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/death_5.png", &imageX, &imageY);
	hero_right->child3->death.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild3_Crusader/death_6.png", &imageX, &imageY);
	hero_right->child3->death.push_back(*setTexture(imageX, imageY, L"./rightchild3_Crusader/death_6.png", new CUSTOMTEXTURE));


	hero_right->child3->UnitName = L"Crusader";
	hero_right->child3->melee_attack = 10;
	hero_right->child3->remote_attack = 0;
	hero_right->child3->health = 35;
	hero_right->child3->defense = 12;
	hero_right->child3->speed = 6;


#pragma endregion

	#pragma region hero_right_child4_Magog
	GetImageSize("./rightchild4_Magog/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild4_Magog/idle.png", &hero_right->child4->idle);
	GetImageSize("./rightchild4_Magog/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild4_Magog/avatar.png", &hero_right->child4->avatar);

	GetImageSize("./rightchild4_Magog/selected_1.png", &imageX, &imageY);
	hero_right->child4->selected.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/selected_2.png", &imageX, &imageY);  
	hero_right->child4->selected.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/selected_3.png", &imageX, &imageY); 
	hero_right->child4->selected.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/selected_4.png", &imageX, &imageY); 
	hero_right->child4->selected.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/selected_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/selected_5.png", &imageX, &imageY); 
	hero_right->child4->selected.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/selected_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/selected_6.png", &imageX, &imageY); 
	hero_right->child4->selected.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/selected_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild4_Magog/move_1.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_2.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_3.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_4.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_5.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_6.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_7.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_8.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/move_9.png", &imageX, &imageY);
	hero_right->child4->move.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/move_9.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild4_Magog/attack_1.png", &imageX, &imageY);
	hero_right->child4->attack.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/attack_2.png", &imageX, &imageY);
	hero_right->child4->attack.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/attack_3.png", &imageX, &imageY);
	hero_right->child4->attack.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/attack_4.png", &imageX, &imageY);
	hero_right->child4->attack.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/attack_5.png", &imageX, &imageY);
	hero_right->child4->attack.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/attack_6.png", &imageX, &imageY);
	hero_right->child4->attack.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/attack_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/attack_7.png", &imageX, &imageY);
	hero_right->child4->attack.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/attack_7.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild4_Magog/beAttacked_1.png", &imageX, &imageY);
	hero_right->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/beAttacked_2.png", &imageX, &imageY);
	hero_right->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/beAttacked_3.png", &imageX, &imageY);
	hero_right->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/beAttacked_4.png", &imageX, &imageY);
	hero_right->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/beAttacked_5.png", &imageX, &imageY);
	hero_right->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/beAttacked_6.png", &imageX, &imageY);
	hero_right->child4->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild4_Magog/death_1.png", &imageX, &imageY);
	hero_right->child4->death.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/death_2.png", &imageX, &imageY);
	hero_right->child4->death.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/death_3.png", &imageX, &imageY);
	hero_right->child4->death.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/death_4.png", &imageX, &imageY);
	hero_right->child4->death.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/death_5.png", &imageX, &imageY);
	hero_right->child4->death.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild4_Magog/death_6.png", &imageX, &imageY);
	hero_right->child4->death.push_back(*setTexture(imageX, imageY, L"./rightchild4_Magog/death_6.png", new CUSTOMTEXTURE));

	hero_right->child4->UnitName = L"Magog";
	hero_right->child4->melee_attack = 7;
	hero_right->child4->remote_attack = 0;
	hero_right->child4->health = 13;
	hero_right->child4->defense = 4;
	hero_right->child4->speed = 6;

#pragma endregion

	#pragma region hero_right_child5_PowerLich
	GetImageSize("./rightchild5_PowerLich/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild5_PowerLich/idle.png", &hero_right->child5->idle);
	GetImageSize("./rightchild5_PowerLich/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild5_PowerLich/avatar.png", &hero_right->child5->avatar);

	GetImageSize("./rightchild5_PowerLich/selected_1.png", &imageX, &imageY);
	hero_right->child5->selected.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/selected_2.png", &imageX, &imageY);
	hero_right->child5->selected.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/selected_3.png", &imageX, &imageY);
	hero_right->child5->selected.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/selected_4.png", &imageX, &imageY);
	hero_right->child5->selected.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/selected_4.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild5_PowerLich/move_1.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_2.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_3.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_4.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_5.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_6.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_7.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_8.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_9.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/move_10.png", &imageX, &imageY);
	hero_right->child5->move.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/move_10.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild5_PowerLich/attack_1.png", &imageX, &imageY);
	hero_right->child5->attack.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/attack_2.png", &imageX, &imageY);
	hero_right->child5->attack.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/attack_2.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild5_PowerLich/beAttacked_1.png", &imageX, &imageY);
	hero_right->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/beAttacked_2.png", &imageX, &imageY);
	hero_right->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/beAttacked_3.png", &imageX, &imageY);
	hero_right->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/beAttacked_4.png", &imageX, &imageY);
	hero_right->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/beAttacked_5.png", &imageX, &imageY);
	hero_right->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/beAttacked_6.png", &imageX, &imageY);
	hero_right->child5->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild5_PowerLich/death_1.png", &imageX, &imageY);
	hero_right->child5->death.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild5_PowerLich/death_2.png", &imageX, &imageY);
	hero_right->child5->death.push_back(*setTexture(imageX, imageY, L"./rightchild5_PowerLich/death_2.png", new CUSTOMTEXTURE));


	hero_right->child5->UnitName = L"PowerLich";
	hero_right->child5->melee_attack = 13;
	hero_right->child5->remote_attack = 15;
	hero_right->child5->health = 40;
	hero_right->child5->defense = 4;
	hero_right->child5->speed = 7;

#pragma endregion

	#pragma region hero_right_child6_ArchMage
	GetImageSize("./rightchild6_ArchMage/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild6_ArchMage/idle.png", &hero_right->child6->idle);
	GetImageSize("./rightchild6_ArchMage/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild6_ArchMage/avatar.png", &hero_right->child6->avatar);

	GetImageSize("./rightchild6_ArchMage/selected_1.png", &imageX, &imageY);
	hero_right->child6->selected.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/selected_2.png", &imageX, &imageY);
	hero_right->child6->selected.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/selected_3.png", &imageX, &imageY);
	hero_right->child6->selected.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/selected_3.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild6_ArchMage/move_1.png", &imageX, &imageY);
	hero_right->child6->move.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/move_2.png", &imageX, &imageY);
	hero_right->child6->move.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/move_3.png", &imageX, &imageY);
	hero_right->child6->move.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/move_4.png", &imageX, &imageY);
	hero_right->child6->move.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/move_5.png", &imageX, &imageY);
	hero_right->child6->move.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/move_6.png", &imageX, &imageY);
	hero_right->child6->move.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/move_7.png", &imageX, &imageY);
	hero_right->child6->move.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/move_7.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild6_ArchMage/attack_1.png", &imageX, &imageY);
	hero_right->child6->attack.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/attack_2.png", &imageX, &imageY);
	hero_right->child6->attack.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/attack_3.png", &imageX, &imageY);
	hero_right->child6->attack.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/attack_4.png", &imageX, &imageY);
	hero_right->child6->attack.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/attack_5.png", &imageX, &imageY);
	hero_right->child6->attack.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/attack_6.png", &imageX, &imageY);
	hero_right->child6->attack.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/attack_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild6_ArchMage/beAttacked_1.png", &imageX, &imageY);
	hero_right->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/beAttacked_2.png", &imageX, &imageY);
	hero_right->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/beAttacked_3.png", &imageX, &imageY);
	hero_right->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/beAttacked_4.png", &imageX, &imageY);
	hero_right->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/beAttacked_5.png", &imageX, &imageY);
	hero_right->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/beAttacked_6.png", &imageX, &imageY);
	hero_right->child6->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild6_ArchMage/death_1.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/death_2.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/death_3.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/death_4.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/death_5.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/death_6.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/death_7.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild6_ArchMage/death_8.png", &imageX, &imageY);
	hero_right->child6->death.push_back(*setTexture(imageX, imageY, L"./rightchild6_ArchMage/death_8.png", new CUSTOMTEXTURE));

	hero_right->child6->UnitName = L"ArchMage";
	hero_right->child6->melee_attack = 9;
	hero_right->child6->remote_attack = 11;
	hero_right->child6->health = 30;
	hero_right->child6->defense = 9;
	hero_right->child6->speed = 7;

#pragma endregion

	#pragma region hero_right_child7_GoldDragon
	GetImageSize("./rightchild7_GoldDragon/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild7_GoldDragon/idle.png", &hero_right->child7->idle);
	GetImageSize("./rightchild7_GoldDragon/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild7_GoldDragon/avatar.png", &hero_right->child7->avatar);

	GetImageSize("./rightchild7_GoldDragon/selected_1.png", &imageX, &imageY);
	hero_right->child7->selected.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/selected_2.png", &imageX, &imageY);
	hero_right->child7->selected.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/selected_3.png", &imageX, &imageY);
	hero_right->child7->selected.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/selected_4.png", &imageX, &imageY);
	hero_right->child7->selected.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/selected_4.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild7_GoldDragon/move_1.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_2.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_3.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_4.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_5.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_6.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_7.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_8.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/move_9.png", &imageX, &imageY);
	hero_right->child7->move.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/move_9.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild7_GoldDragon/attack_1.png", &imageX, &imageY);
	hero_right->child7->attack.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/attack_2.png", &imageX, &imageY);
	hero_right->child7->attack.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/attack_3.png", &imageX, &imageY);
	hero_right->child7->attack.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/attack_4.png", &imageX, &imageY);
	hero_right->child7->attack.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/attack_5.png", &imageX, &imageY);
	hero_right->child7->attack.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/attack_6.png", &imageX, &imageY);
	hero_right->child7->attack.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/attack_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild7_GoldDragon/beAttacked_1.png", &imageX, &imageY);
	hero_right->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/beAttacked_2.png", &imageX, &imageY);
	hero_right->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/beAttacked_3.png", &imageX, &imageY);
	hero_right->child7->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/beAttacked_3.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild7_GoldDragon/death_1.png", &imageX, &imageY);
	hero_right->child7->death.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/death_2.png", &imageX, &imageY);
	hero_right->child7->death.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/death_3.png", &imageX, &imageY);
	hero_right->child7->death.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/death_4.png", &imageX, &imageY);
	hero_right->child7->death.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild7_GoldDragon/death_5.png", &imageX, &imageY);
	hero_right->child7->death.push_back(*setTexture(imageX, imageY, L"./rightchild7_GoldDragon/death_5.png", new CUSTOMTEXTURE));


	hero_right->child7->UnitName = L"GoldDragon";
	hero_right->child7->melee_attack = 40;
	hero_right->child7->remote_attack = 0;
	hero_right->child7->health = 250;
	hero_right->child7->defense = 27;
	hero_right->child7->speed = 16;

#pragma endregion

	#pragma region hero_right_child8_Titan
	GetImageSize("./rightchild8_Titan/idle.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild8_Titan/idle.png", &hero_right->child8->idle);
	GetImageSize("./rightchild8_Titan/avatar.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./rightchild8_Titan/avatar.png", &hero_right->child8->avatar);

	GetImageSize("./rightchild8_Titan/selected_1.png", &imageX, &imageY);
	hero_right->child8->selected.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/selected_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/selected_2.png", &imageX, &imageY);
	hero_right->child8->selected.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/selected_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/selected_3.png", &imageX, &imageY);
	hero_right->child8->selected.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/selected_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/selected_4.png", &imageX, &imageY);
	hero_right->child8->selected.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/selected_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/selected_5.png", &imageX, &imageY);
	hero_right->child8->selected.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/selected_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/selected_6.png", &imageX, &imageY);
	hero_right->child8->selected.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/selected_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/selected_7.png", &imageX, &imageY);
	hero_right->child8->selected.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/selected_7.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild8_Titan/move_1.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_2.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_3.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_4.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_5.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_6.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_7.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_8.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_8.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_9.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_9.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/move_10.png", &imageX, &imageY);
	hero_right->child8->move.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/move_10.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild8_Titan/attack_1.png", &imageX, &imageY);
	hero_right->child8->attack.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/attack_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/attack_2.png", &imageX, &imageY);
	hero_right->child8->attack.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/attack_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/attack_3.png", &imageX, &imageY);
	hero_right->child8->attack.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/attack_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/attack_4.png", &imageX, &imageY);
	hero_right->child8->attack.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/attack_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/attack_5.png", &imageX, &imageY);
	hero_right->child8->attack.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/attack_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/attack_6.png", &imageX, &imageY);
	hero_right->child8->attack.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/attack_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild8_Titan/beAttacked_1.png", &imageX, &imageY);
	hero_right->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/beAttacked_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/beAttacked_2.png", &imageX, &imageY);
	hero_right->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/beAttacked_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/beAttacked_3.png", &imageX, &imageY);
	hero_right->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/beAttacked_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/beAttacked_4.png", &imageX, &imageY);
	hero_right->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/beAttacked_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/beAttacked_5.png", &imageX, &imageY);
	hero_right->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/beAttacked_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/beAttacked_6.png", &imageX, &imageY);
	hero_right->child8->beAttacked.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/beAttacked_6.png", new CUSTOMTEXTURE));

	GetImageSize("./rightchild8_Titan/death_1.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_1.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/death_2.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_2.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/death_3.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_3.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/death_4.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_4.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/death_5.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_5.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/death_6.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_6.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/death_7.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_7.png", new CUSTOMTEXTURE));
	GetImageSize("./rightchild8_Titan/death_8.png", &imageX, &imageY);
	hero_right->child8->death.push_back(*setTexture(imageX, imageY, L"./rightchild8_Titan/death_8.png", new CUSTOMTEXTURE));


	hero_right->child8->UnitName = L"Titan";
	hero_right->child8->melee_attack = 50;
	hero_right->child8->remote_attack = 50;
	hero_right->child8->health = 300;
	hero_right->child8->defense = 24;
	hero_right->child8->speed = 11;

#pragma endregion

	#pragma region InitturnList

	GetImageSize("./InitturnList/lefthero_frame.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./InitturnList/lefthero_frame.png", &gameobj->lefthero_frame);
	GetImageSize("./InitturnList/righthero_frame.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./InitturnList/righthero_frame.png", &gameobj->righthero_frame);

	GetImageSize("./InitturnList/testPoint.png", &imageX, &imageY);
	setTexture(imageX, imageY, L"./InitturnList/testPoint.png", &gameobj->testPoint);

	#pragma region unitVector Push
	unitVector.push_back(*hero_left->child1);
	unitVector.push_back(*hero_left->child2);
	unitVector.push_back(*hero_left->child3);
	unitVector.push_back(*hero_left->child4);
	unitVector.push_back(*hero_left->child5);
	unitVector.push_back(*hero_left->child6);
	unitVector.push_back(*hero_left->child7);
	unitVector.push_back(*hero_left->child8);

	unitVector.push_back(*hero_right->child1);
	unitVector.push_back(*hero_right->child2);
	unitVector.push_back(*hero_right->child3);
	unitVector.push_back(*hero_right->child4);
	unitVector.push_back(*hero_right->child5);
	unitVector.push_back(*hero_right->child6);
	unitVector.push_back(*hero_right->child7);
	unitVector.push_back(*hero_right->child8);
#pragma endregion

	

	//sorting & inserting
	sort(unitVector.begin(), unitVector.end(), cmp);
	
	for (int j = 0; j < 200;j++) {
		for (int i = 0; i < unitVector.size(); i++) {
			turnVector.push_back(&unitVector[i]);
		}
	}
	//unit state setting
	for (int i = 0; i < unitVector.size(); i++) {
		unitVector[i].animstate = Child::AnimState::E_idle;
	}
	//first turn setting
	turnVector.front()->animstate = Child::AnimState::E_selected;


#pragma endregion

	#pragma region MapSetting
	for (int i = 0; i < hexMap.size();i++) {
		for (int j = 0; j < hexMap[i].size();j++) {
			hexMap[i][j].P_child = NULL;
			hexMap[i][j].UnitOnTile = false;
			for (int k = 0; k < unitVector.size();k++) {
				if (unitVector[k].virtualLocation_x == j&&unitVector[k].virtualLocation_y == i) {
					hexMap[i][j].P_child = &unitVector[k];
					hexMap[i][j].UnitOnTile = true;
				}
			}
		}
	}



	#pragma endregion


#pragma endregion
	return;
}


void paint(LPD3DXSPRITE* P_d3dspt, LPDIRECT3DTEXTURE9 texture,int width,int height,int scaleXY,int x,int y,int alpha) {
	RECT part;
	SetRect(&part, 0, 0, width, height);
	D3DXVECTOR3 part_center(0.0f, 0.0f, 0.0f);    // center at the upper-left corner
	D3DXVECTOR3 part_position(x, y, 0.0f);    // position at 50, 50 with no depth

	//---------------------------------------------------
	D3DXMATRIX scale;
	D3DXMATRIX rotation;
	D3DXMATRIX translate;
	D3DXMATRIX combined;

	// Initialize the Combined matrix.
	D3DXMatrixIdentity(&combined);

	// Scale the sprite.
	D3DXMatrixScaling(&scale, scaleXY, scaleXY, 1.0f);
	combined *= scale;

	//// Rotate the sprite.
	//D3DXMatrixTranslation(&translate, -fRotCenterX * scaleXY, -fRotCenterY * scaleXY, 0.0f);
	//combined *= translate;
	//D3DXMatrixRotationZ(&rotation, fRotation);
	//combined *= rotation;
	//D3DXMatrixTranslation(&translate, fRotCenterX * scaleXY, fRotCenterY * scaleXY, 0.0f);
	//combined *= translate;

	// Translate the sprite
	if (scaleXY == -1) {
		D3DXMatrixScaling(&scale, -scaleXY, scaleXY, 1.0f);
		combined *= scale;

	D3DXMatrixTranslation(&translate,2*x+width, 0, 0.0f);
	combined *= translate;

	}
	// Apply the transform.
	(*P_d3dspt)->SetTransform(&combined);


	(*P_d3dspt)->Draw(texture, &part, &part_center, &part_position, D3DCOLOR_ARGB(alpha, 255, 255, 255));
}

void intro_render_frame(GameIntro *intro){
	if (intro->isIntro != 1) {
		return;
	}
	// clear the window to a deep blue
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	d3ddev->BeginScene();    // begins the 3D scene

	d3dspt->Begin(D3DXSPRITE_ALPHABLEND);   
	

	paint(&d3dspt, intro->Homm.texture,1280,720,1,0,0,255);
	
	static int Parameter = 0;
	Parameter += 6;
	
	paint(&d3dspt, intro->pressAnyKey.texture, 805, 66,1, 270, 500, Parameter);



	d3dspt->End();    // end sprite drawing

	d3ddev->EndScene();    // ends the 3D scene

	d3ddev->Present(NULL, NULL, NULL, NULL);

	return;
}

// this is the function used to render a single frame
void render_frame(GameIntro *intro, GameObj* gameobj, Hero*hero_left, Hero*hero_right)
{
	// clear the window to a deep blue
	if (intro->isIntro != 0) {
		return;
	}
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0, 0), 1.0f, 0);

	d3ddev->BeginScene();    // begins the 3D scene
	pSprite->Begin(16);

	d3dspt->Begin(D3DXSPRITE_ALPHABLEND);   

#pragma region background&hero&UI_sprite
	//RECT part;
	//SetRect(&part, 0, 0, 100, 100);
	//D3DXVECTOR3 part_center(0.0f, 0.0f, 0.0f);    // center at the upper-left corner
	//D3DXVECTOR3 part_position(0, 0, 0.f);    // position at 50, 50 with no depth
	//d3dspt->Draw(gameobj->background.texture, &part, &part_center, &part_position, D3DCOLOR_ARGB(255, 255, 255, 255));
	//paint(&d3dspt, gameobj->background.texture, 1280, 720, 0, 0, 255);
	
	paint(&d3dspt, gameobj->hero_left.texture, 151, 166,1, 0, 0, 255);
	
	paint(&d3dspt, gameobj->hero_right.texture, 151, 166,1, 1150, 0, 255);
	
	paint(&d3dspt, gameobj->hexagonMap.texture, 1280, 538,1, 0, 140, 255);
	
	paint(&d3dspt, gameobj->ui.texture, 1280, 42,1, 0, 678, 255);


	//background need depth
	RECT rect = { 0,0,1280,720 };
	D3DXVECTOR3 v3 = { 0,0,0 };
	D3DXVECTOR3 v3_2 = { 0,0,1 };
	D3DCOLOR c = D3DCOLOR_ARGB(255, 255, 255, 255);
		pSprite->Draw(gameobj->background.texture, &rect, &v3, &v3_2, c);
#pragma endregion
	
	int turnVectorStartX = 300;
	int turnVectorStartY = 25;
//turnVector Work
	for (int i = 0; i < 10; i++) {
	paint(&d3dspt, turnVector[i]->avatar.texture, turnVector[i]->avatar.textureinfo.Width, turnVector[i]->avatar.textureinfo.Height,1, turnVectorStartX+i*68, turnVectorStartY, 255);

		if (turnVector[i]->belong == Child::Belong::leftHero){
			paint(&d3dspt, gameobj->lefthero_frame.texture, gameobj->lefthero_frame.textureinfo.Width, gameobj->lefthero_frame.textureinfo.Height,1, turnVectorStartX-5 + i * 68, turnVectorStartY - 5, 255);
		}
		else if(turnVector[i]->belong == Child::Belong::rightHero){
			paint(&d3dspt, gameobj->righthero_frame.texture, gameobj->righthero_frame.textureinfo.Width, gameobj->righthero_frame.textureinfo.Height,1, turnVectorStartX - 5 + i * 68, turnVectorStartY - 5, 255);
		}

		/*wchar_t font_buffer[256];
		wsprintfW(font_buffer, L"%d", turnVector[0].quantity);
		text(font_buffer, 30, 30, 255);*/
		//-----------------------------------may cause problem
		RECT rect = { 0,0,turnVectorStartX + i * 68+ turnVector[i]->avatar.textureinfo.Width,turnVectorStartY+ turnVector[i]->avatar.textureinfo.Height+30 };
		int u = turnVector[i]->quantity;
		wchar_t istr[32];
		_itow_s(u, istr, 10);
		font->DrawTextW(pSprite, istr, -1, &rect,
			DT_SINGLELINE | DT_RIGHT| DT_BOTTOM, D3DCOLOR_RGBA(255, 255, 255, 255));
		
	}

	//for each (Child child in unitVector)
	//{
	//	child.realLocationSetting();
	//	paint(&d3dspt, 
	//		child.getCurrentTexture().texture, 
	//		//gameobj->testPoint.texture,
	//		child.getCurrentTexture().textureinfo.Width, 
	//		child.getCurrentTexture().textureinfo.Height
	//	//	15,15
	//		, MAP_START_X+child.realLocation_x - child.getCurrentTexture().textureinfo.Width / 2,
	//		MAP_START_Y+child.realLocation_y - child.getCurrentTexture().textureinfo.Height
	//		,255 );
	////  - child.getCurrentTexture().textureinfo.Width/2
	//	//  - child.getCurrentTexture().textureinfo.Height
	//}


	for (int i = 0; i < hexMap.size(); i++) {
		for (int j = 0; j < hexMap[i].size(); j++) {
			if (hexMap[i][j].UnitOnTile == true) {
				hexMap[i][j].P_child->realLocationSetting();
				if (hexMap[i][j].P_child->belong == Child::Belong::leftHero) {

				paint(&d3dspt,
							hexMap[i][j].P_child->getCurrentTexture().texture,
							hexMap[i][j].P_child->getCurrentTexture().textureinfo.Width,
							hexMap[i][j].P_child->getCurrentTexture().textureinfo.Height,
							1
							, MAP_START_X+ hexMap[i][j].P_child->realLocation_x - hexMap[i][j].P_child->getCurrentTexture().textureinfo.Width / 2,
							MAP_START_Y+ hexMap[i][j].P_child->realLocation_y - hexMap[i][j].P_child->getCurrentTexture().textureinfo.Height
							,255 );
				}
				else if (hexMap[i][j].P_child->belong == Child::Belong::rightHero) {
					paint(&d3dspt,
						hexMap[i][j].P_child->getCurrentTexture().texture,
						hexMap[i][j].P_child->getCurrentTexture().textureinfo.Width,
						hexMap[i][j].P_child->getCurrentTexture().textureinfo.Height,
						-1
						, MAP_START_X + hexMap[i][j].P_child->realLocation_x - hexMap[i][j].P_child->getCurrentTexture().textureinfo.Width / 2,
						MAP_START_Y + hexMap[i][j].P_child->realLocation_y - hexMap[i][j].P_child->getCurrentTexture().textureinfo.Height
						, 255);
				}

			}
			
			
		}
	}



//		paint(&d3dspt, hero_left->child1->avatar.texture, 10, 10, MAP_START_X, MAP_START_Y, 255);
	
	pSprite->End();
	d3dspt->End();    // end sprite drawing

	d3ddev->EndScene();    // ends the 3D scene

	d3ddev->Present(NULL, NULL, NULL, NULL);

	return;
}


// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
	
	d3ddev->Release();
	d3d->Release();



	return;
}