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

void setTexture(int width, int height, const wchar_t fileLocation[], CUSTOMTEXTURE* cusTexture) {
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
}

void text(LPCWSTR text,int rectX,int rectY,int alpha) {
	RECT rect = {100,100,100,100 };
	font->DrawTextW(NULL, text, -1, &rect,
		DT_SINGLELINE | DT_NOCLIP | DT_LEFT, D3DCOLOR_RGBA(255, 255, 255, alpha));
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

	
	#pragma region hero_left_child1_DreadKnight

	hero_left->child1->selected.push_back(*new CUSTOMTEXTURE);
	
	setTexture(85, 99, L"./leftchild1_DreadKnight/idle.png", &hero_left->child1->idle);
	
	setTexture(58, 64, L"./leftchild1_DreadKnight/avatar.png",&hero_left->child1->avatar);
		
		hero_left->child1->belong = Child::Belong::leftHero;
		hero_left->child1->quantity = 20;
		//Set Location here
		
		//
		
	#pragma endregion

	#pragma region hero_left_child2_Enchanter
	
	setTexture(45, 97, L"./leftchild2_Enchanter/idle.png", &hero_left->child2->idle);
	
	setTexture(58, 64, L"./leftchild2_Enchanter/avatar.png", &hero_left->child2->avatar);

#pragma endregion

	#pragma region hero_left_child3_Champion
	
	setTexture(93, 104, L"./leftchild3_Champion/idle.png", &hero_left->child3->idle);
	
	setTexture(58, 64, L"./leftchild3_Champion/avatar.png", &hero_left->child3->avatar);

#pragma endregion

	#pragma region hero_left_child4_Phoenix
	
	setTexture(100, 142, L"./leftchild4_Phoenix/idle.png", &hero_left->child4->idle);
	
	setTexture(58, 64, L"./leftchild4_Phoenix/avatar.png", &hero_left->child4->avatar);

#pragma endregion

	#pragma region hero_left_child5_MightyGorgon
	
	setTexture(94, 75, L"./leftchild5_MightyGorgon/idle.png", &hero_left->child5->idle);
	
	setTexture(58, 64, L"./leftchild5_MightyGorgon/avatar.png", &hero_left->child5->avatar);

#pragma endregion

	#pragma region hero_left_child6_Sharpshooter
	
	setTexture(39, 82, L"./leftchild6_Sharpshooter/idle.png", &hero_left->child6->idle);
	
	setTexture(58, 64, L"./leftchild6_Sharpshooter/avatar.png", &hero_left->child6->avatar);

#pragma endregion

	#pragma region hero_left_child7_Archangel
	
	setTexture(72, 104, L"./leftchild7_Archangel/idle.png", &hero_left->child7->idle);

	setTexture(58, 64, L"./leftchild7_Archangel/avatar.png", &hero_left->child7->avatar);

#pragma endregion

	#pragma region hero_left_child8_ArchDevil
	
	setTexture(43, 117, L"./leftchild8_ArchDevil/idle.png", &hero_left->child8->idle);
	
	setTexture(58, 64, L"./leftchild8_ArchDevil/avatar.png", &hero_left->child8->avatar);

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
	paint(&d3dspt, turnVector[0].avatar.texture, 58, 64, turnVectorStartX, turnVectorStartY, 255);

		if (turnVector[0].belong == Child::Belong::leftHero){
			paint(&d3dspt, gameobj->righthero_frame.texture, 63, 69, turnVectorStartX-5, turnVectorStartY-5, 255);
		}
		else {

		}
		/*wchar_t font_buffer[256];
		wsprintfW(font_buffer, L"%d", turnVector[0].quantity);
		text(font_buffer, 30, 30, 255);*/
		RECT rect = { 0,0,1280,720 };
		D3DXVECTOR3 v3 = {0,0,0};
		D3DXVECTOR3 v3_2 = { 0,0,1 };
		D3DCOLOR c = D3DCOLOR_ARGB(255, 255, 255, 255);
		pSprite->Draw(gameobj->background.texture, &rect, &v3, &v3_2, c);
		font->DrawTextW(pSprite, L"MUNODevelop", -1, &rect,
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