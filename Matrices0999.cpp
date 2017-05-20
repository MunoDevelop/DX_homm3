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
//
//void text(LPCWSTR text,int rectX,int rectY,int alpha) {
//	RECT rect = {100,100,100,100 };
//	font->DrawTextW(NULL, text, -1, &rect,
//		DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 255, 255, alpha));
//}

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
										  //UnitVector font init
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
	
	

	int imageX, imageY;

	#pragma region hero_left_child1_DreadKnight

	
		hero_left->child1->belong = Child::Belong::leftHero;
		hero_left->child1->quantity = 20;
		//-----------------Location need setting
		hero_left->child1->location_x = 0;
		hero_left->child1->location_y = 0;
	
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
		
	setTexture(87, 105, L"./rightchild1_AncientBehemoth/idle.png", &hero_right->child1->idle);

	setTexture(58, 64, L"./rightchild1_AncientBehemoth/avatar.png", &hero_right->child1->avatar);

#pragma endregion

	#pragma region hero_right_child2_Peasant
	
	setTexture(44, 78, L"./rightchild2_Peasant/idle.png", &hero_right->child2->idle);
	
	setTexture(58, 64, L"./rightchild2_Peasant/avatar.png", &hero_right->child2->avatar);

#pragma endregion

	#pragma region hero_right_child3_Crusader
	
	setTexture(41, 89, L"./rightchild3_Crusader/idle.png", &hero_right->child3->idle);

	setTexture(58, 64, L"./rightchild3_Crusader/avatar.png", &hero_right->child3->avatar);

#pragma endregion

	#pragma region hero_right_child4_Magog
	
	setTexture(47, 84, L"./rightchild4_Magog/idle.png", &hero_right->child4->idle);

	setTexture(58, 64, L"./rightchild4_Magog/avatar.png", &hero_right->child4->avatar);

#pragma endregion

	#pragma region hero_right_child5_PowerLich

	setTexture(45, 105, L"./rightchild5_PowerLich/idle.png", &hero_right->child5->idle);

	setTexture(58, 64, L"./rightchild4_Magog/avatar.png", &hero_right->child5->avatar);

#pragma endregion

	#pragma region hero_right_child6_ArchMage
	
	setTexture(43, 101, L"./rightchild6_ArchMage/idle.png", &hero_right->child6->idle);

	setTexture(58, 64, L"./rightchild6_ArchMage/avatar.png", &hero_right->child6->avatar);

#pragma endregion

	#pragma region hero_right_child7_GoldDragon
	
	setTexture(85, 119, L"./rightchild7_GoldDragon/idle.png", &hero_right->child7->idle);

	setTexture(58, 64, L"./rightchild7_GoldDragon/avatar.png", &hero_right->child7->avatar);

#pragma endregion

	#pragma region hero_right_child8_Titan
	
	setTexture(44, 103, L"./rightchild8_Titan/idle.png", &hero_right->child8->idle);
	
	setTexture(58, 64, L"./rightchild8_Titan/avatar.png", &hero_right->child8->avatar);
#pragma endregion

#pragma region InitturnList
	
	setTexture(63, 69, L"./InitturnList/lefthero_frame.png", &gameobj->lefthero_frame);
	
	setTexture(63, 69, L"./InitturnList/righthero_frame.png", &gameobj->righthero_frame);

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
			turnVector.push_back(unitVector[i]);
		}
	}
	


#pragma endregion


#pragma endregion
	return;
}


void paint(LPD3DXSPRITE* P_d3dspt, LPDIRECT3DTEXTURE9 texture,int width,int height,int x,int y,int alpha) {
	RECT part;
	SetRect(&part, 0, 0, width, height);
	D3DXVECTOR3 part_center(0.0f, 0.0f, 0.0f);    // center at the upper-left corner
	D3DXVECTOR3 part_position(x, y, 0.0f);    // position at 50, 50 with no depth
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
	

	paint(&d3dspt, intro->Homm.texture,1280,720,0,0,255);
	
	static int Parameter = 0;
	Parameter += 6;
	
	paint(&d3dspt, intro->pressAnyKey.texture, 805, 66, 270, 500, Parameter);



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

#pragma region background&hero&UI_sprite__Render_Frame
	//RECT part;
	//SetRect(&part, 0, 0, 100, 100);
	//D3DXVECTOR3 part_center(0.0f, 0.0f, 0.0f);    // center at the upper-left corner
	//D3DXVECTOR3 part_position(0, 0, 0.f);    // position at 50, 50 with no depth
	//d3dspt->Draw(gameobj->background.texture, &part, &part_center, &part_position, D3DCOLOR_ARGB(255, 255, 255, 255));

	//paint(&d3dspt, gameobj->background.texture, 1280, 720, 0, 0, 255);
	
	paint(&d3dspt, gameobj->hero_left.texture, 151, 166, 0, 0, 255);
	
	paint(&d3dspt, gameobj->hero_right.texture, 151, 166, 1150, 0, 255);
	
	paint(&d3dspt, gameobj->hexagonMap.texture, 1280, 538, 0, 140, 255);
	
	paint(&d3dspt, gameobj->ui.texture, 1280, 42, 0, 678, 255);
#pragma endregion
	
	int turnVectorStartX = 300;
	int turnVectorStartY = 65;
	for (int i = 0; i < 10; i++) {
	paint(&d3dspt, turnVector[0].avatar.texture, turnVector[0].avatar.textureinfo.Width, turnVector[0].avatar.textureinfo.Height, turnVectorStartX, turnVectorStartY, 255);

		if (turnVector[0].belong == Child::Belong::leftHero){
			paint(&d3dspt, gameobj->righthero_frame.texture, 63, 69, turnVectorStartX-5, turnVectorStartY-5, 255);
		}
		else {

		}

		paint(&d3dspt, (hero_left->child1->selected[1].texture), 58, 64, turnVectorStartX, turnVectorStartY, 255);
		/*wchar_t font_buffer[256];
		wsprintfW(font_buffer, L"%d", turnVector[0].quantity);
		text(font_buffer, 30, 30, 255);*/
		RECT rect = { 0,0,1280,720 };
		D3DXVECTOR3 v3 = {0,0,0};
		D3DXVECTOR3 v3_2 = { 0,0,1 };
		D3DCOLOR c = D3DCOLOR_ARGB(255, 255, 255, 255);
		pSprite->Draw(gameobj->background.texture, &rect, &v3, &v3_2, c);
		font->DrawTextW(pSprite, hero_left->child1->UnitName, -1, &rect,
			DT_SINGLELINE | DT_LEFT, D3DCOLOR_RGBA(255, 255, 255, 255));
		
	}
	
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