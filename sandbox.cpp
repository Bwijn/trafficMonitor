#include <tchar.h>
//#include <shellapi.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <Iphlpapi.h>
#include <string>
#include <vector>
#pragma comment(lib, "IPHLPAPI.lib")
#define IDM_AAA 200
#define IDM_BBB 201
#define MENU_EXTI 202
#define ID_TRAYICON 1001
#define WM_TRAYICON (WM_USER + 1)
HWND h_TaskBar;
// ע��֪ͨ����ͼ��
HANDLE hIcon;
NOTIFYICONDATA nid;
static unsigned int MonitorWndHeight = 48;
static unsigned int MonitorWndWidth = 1920 / 10;

static unsigned int lastInOctets{ 0 };
static unsigned int realInSpeed{ 0 };

RECT speedWndRect;// Ҫ������ʾ���ڵĴ�С RECT structure 
static HMENU hMenu;// popup��menu

std::vector< IP_ADAPTER_ADDRESSES> adapterList;
std::vector<MIB_IFROW> IfList;
IP_ADAPTER_ADDRESSES selectedAda{};
int m_connection_selected{ 0 }; //Ҫ��ʾ���������ӵ����

static PIP_ADAPTER_ADDRESSES  pIpAdapterInfo;
/* variables used for GetIfTable and GetIfEntry */
MIB_IFTABLE* pIfTable;
MIB_IFROW* pIfRow;

HWND hWnd;// ���ڵ�handle
HDC hdc;//Registered Window Device Handle

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Function declare
void determineRect();

bool initApp(HINSTANCE hInstance);
bool initNetInfo();

auto getCurrentIFStates();


template <typename T>
void DrawTxt(T NetSpeed);

void DisplayNetSpeed();

void autoSelectAdapter();

// main body
template <typename T>
void DrawTxt(T NetSpeed) {
	// ��������
	//if ((NetworkSpeed <=100)&&(NetworkSpeed>=1))
	//{
	//	NetworkSpeed -= 1;
	//}
	//else
	//{
	//	return;
	//}
	RECT rect;
	GetClientRect(hWnd, &rect);

	std::wstring speedExpress = L"Net real-time: "
		+ std::to_wstring(NetSpeed)
		+ L"kb/s"
		+ L"\n"
		+ selectedAda.FriendlyName
		+ L"\n"
		+ selectedAda.Description
		+ L"\n";
	DrawText(hdc, speedExpress.c_str(), -1, &rect, DT_CENTER | DT_VCENTER);
	//DrawText(hdc, TaskBarRectString.c_str(), -1, &rect, DT_CENTER | DT_VCENTER );


}

void determineRect()
{
	GetClientRect(h_TaskBar, &speedWndRect);
	//����߷�֮һ�ĳ��� ��Ȳ���
	speedWndRect.right = speedWndRect.right / 6;



}

bool initApp(HINSTANCE hInstance)
{
	//��ʼ��popup menu
	hMenu = CreatePopupMenu();
	hIcon = LoadImage(NULL, _T("notify.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

	auto var = GetLastError();
	AppendMenu(hMenu, MF_STRING, MENU_EXTI, _T("�˳�"));



	// ��������
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = (HICON)hIcon;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _T("MyApp");
	wcex.hIconSm = NULL;
	auto retReg = RegisterClassEx(&wcex);
	if (retReg == 0)
	{

		return false;
	}

	h_TaskBar = FindWindow(_T("Shell_TrayWnd"), NULL);
	//h_TaskBar = FindWindow(_T("MSTaskSwWClass"), NULL);
	// ��ȡҪ���ƵĴ��ڵĳ��Ϳ�
	determineRect();


	return true;
}

bool initNetInfo()
{
	// ��ȡadapter ��details info ������vector��
	pIpAdapterInfo = (PIP_ADAPTER_ADDRESSES)new BYTE[sizeof(PIP_ADAPTER_ADDRESSES)];		//PIP_ADAPTER_INFO�ṹ��ָ��洢����������Ϣ
	unsigned long stSize = sizeof(PIP_ADAPTER_ADDRESSES);		//�õ��ṹ���С,����GetAdaptersInfo����

	auto ret = GetAdaptersAddresses(AF_UNSPEC, 0, 0, pIpAdapterInfo, &stSize);
	if (ret == ERROR_BUFFER_OVERFLOW)
	{
		delete[](BYTE*)pIpAdapterInfo;	//�ͷ�ԭ�����ڴ�ռ�
		pIpAdapterInfo = (PIP_ADAPTER_ADDRESSES)new BYTE[stSize];
		ret = GetAdaptersAddresses(AF_UNSPEC, 0, 0, pIpAdapterInfo, &stSize);
	}
	if (ret == ERROR_SUCCESS)
	{
		//�� vector container add a pointer which is points to 
		while (pIpAdapterInfo)
		{
			adapterList.push_back(*pIpAdapterInfo);
			pIpAdapterInfo = pIpAdapterInfo->Next;

		}
		//return true;

	}



	// Make an initial call to GetIfTable to get the
	// necessary size into dwSize

	DWORD dwSize = sizeof(MIB_IFTABLE);
	pIfTable = new MIB_IFTABLE;
	if (GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
		delete pIfTable;
		pIfTable = (MIB_IFTABLE*)new byte[dwSize];

		auto ret = GetIfTable(pIfTable, &dwSize, FALSE);
		if (ret != NO_ERROR) {

			return false;
		}
	}
	//push var in MIB_IFROW container
	for (size_t i = 0; i < pIfTable->dwNumEntries; i++)
	{
		IfList.push_back(pIfTable->table[i]);
	}
	//auto




	return true;
}

auto getCurrentIFStates()
{
	DWORD dwSize;
	//pIfTable = new MIB_IFTABLE;
	auto	ret = GetIfTable(pIfTable, &dwSize, FALSE);
	if (ret == ERROR_INSUFFICIENT_BUFFER) {
		delete[] pIfTable;
		pIfTable = (MIB_IFTABLE*)new byte[dwSize];

		auto ret = GetIfTable(pIfTable, &dwSize, FALSE);
		if (ret != NO_ERROR) {

			return false;
		}
	}
	if (ret == NO_ERROR) {
		return true;

	}
}

void DisplayNetSpeed()
{
	// calculate delta  speed 
	if (!pIfTable)
	{
		return;
	}

	if (lastInOctets == 0)
	{
		lastInOctets = pIfTable->table[m_connection_selected].dwInOctets;

	}

	auto cur_InSd = pIfTable->table[m_connection_selected].dwInOctets;

	realInSpeed = cur_InSd - lastInOctets;// finaly real-time speed per seconds
	realInSpeed = realInSpeed / 1024;

	lastInOctets = cur_InSd;
	// Draw xxx
	DrawTxt(realInSpeed);

}

void autoSelectAdapter()
{
	unsigned __int64 max_in_out_bytes{};
	unsigned __int64 in_out_bytes;
	//�Զ�ѡ������ʱ�������ѷ��ͺ��ѽ����ֽ���֮�������Ǹ����ӣ�����������Ϊ��ǰ�鿴������
	//for (auto& x : adapterList) {
	//	IfList[x.IfIndex].
	//}
	for (auto& x : adapterList)
	{
		auto table = IfList[x.IfIndex];
		if (table.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)     //ֻѡ������״̬Ϊ����������
		{
			in_out_bytes = table.dwInOctets + table.dwOutOctets;
			if (in_out_bytes > max_in_out_bytes)
			{
				max_in_out_bytes = in_out_bytes;
				m_connection_selected = x.IfIndex;
			}
		}
	}

	auto	Ada = std::find_if(adapterList.begin(),
		adapterList.end(),
		[](IP_ADAPTER_ADDRESSES var) {return m_connection_selected == var.IfIndex; }
	);
	selectedAda = *Ada;







}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case MENU_EXTI:
			// �����˳��˵���
			PostMessage(hWnd, WM_CLOSE, 0, 0); // ���͹ر���Ϣ
			break;
		default:
			break;
		}
	}
	case WM_RBUTTONDOWN: {



		POINT point;
		point.x = LOWORD(lParam);
		point.y = HIWORD(lParam);
		ClientToScreen(hWnd, &point);
		//GetCursorPos(&pt); // ��ȡ���λ��
		TrackPopupMenu(hMenu, TPM_RIGHTALIGN, point.x, point.y, 0, hWnd, NULL);
		break;

	}
	case WM_CLOSE:
		Shell_NotifyIcon(NIM_DELETE, &nid);

		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
	{

		PAINTSTRUCT ps;
		hdc = BeginPaint(hWnd, &ps);

		// ��������
		HFONT hFont = CreateFont(
			14, // �����С
			0, 0, 0, // �����ϸ��б�塢�»��ߵ�
			FW_NORMAL,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_SWISS,
			TEXT("Arial")
		);


		// ѡ���������ɫ���豸������
		HGDIOBJ hPrevFont = SelectObject(hdc, hFont);
		SetTextColor(hdc, RGB(0, 0, 0)); // ����color

		// ��ʾ����
		DisplayNetSpeed();

		// �ͷ��豸�����ĺ�������Դ
		SelectObject(hdc, hPrevFont);
		DeleteObject(hFont);
		EndPaint(hWnd, &ps);
		//UpdateWindow(h_TaskBar);
		break;
	}
	case WM_TIMER:
	{
		//SetLayeredWindowAttributes(hWnd, 0, 0, LWA_ALPHA);
		getCurrentIFStates();

		InvalidateRect(hWnd, NULL, TRUE); // Redraw window
		//MessageBox(hWnd, _T("Timmer alert"), 0, MB_OK);
		//InvalidateRect(h_TaskBar, 0, 1);

		break;
	}

	case WM_TRAYICON:
		 //����ϵͳ֪ͨ����ͼ����¼�
		if (lParam == WM_RBUTTONUP) {// �Ҽ�����¼�
			POINT pt;
			GetCursorPos(&pt); // ��ȡ���λ��
			SetForegroundWindow(hWnd); // ���õ�ǰ����Ϊǰ̨���ڣ��Ա���ղ˵���Ϣ
			TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hWnd, NULL); // ��ʾ�˵�
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	initApp(hInstance);


	//if hWndNewParent is not NULL and the window was previously a child of the desktop, you should clear the WS_POPUP style and set the WS_CHILD style before calling SetParent.
	hWnd = CreateWindowEx(WS_EX_LAYERED,//ָ����layered ���Ա������setattribute������һ�� Ҫ��Ȼ����ʾ
		_T("MyApp"), _T("My App"), WS_POPUP,//
		0, 0, speedWndRect.right, speedWndRect.bottom, nullptr, nullptr, hInstance, nullptr);
	

	//hWnd = CreateWindowW(//ָ����layered ���Ա������setattribute������һ�� Ҫ��Ȼ����ʾ
	//	_T("MyApp"), _T("My App"), WS_POPUP,//
	//	0, 0, speedWndRect.right, speedWndRect.bottom, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		//auto var = GetLastError();
		return GetLastError();

	}

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = ID_TRAYICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = (HICON)hIcon;
	std::wstring Info = L"fffff!!!";
	std::wstring tip = L"NetWork Speed running. . .";

	wcsncpy_s(nid.szTip, tip.c_str(), tip.length() + 1);
	//wcsncpy_s(nid.szInfo, Info.c_str(), Info.length() + 1);
	Shell_NotifyIcon(NIM_ADD, &nid);

	SetTimer(hWnd, 1, 1000, NULL);


	//initialize interface adapter info
	initNetInfo();

	//// �Զ�ѡ�� adapter
	autoSelectAdapter();
	//determine the rectangle

	auto previousWnd = SetParent(hWnd, h_TaskBar);// �ǲ�����������⣿

	auto ret = SetWindowPos(hWnd, HWND_TOP, 0, 0, speedWndRect.right, speedWndRect.bottom, SWP_SHOWWINDOW);

	SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);




	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


	// ������ʱ�����߳̽���ʵʱ���͸���֪ͨ����ͼ��Ĳ���


	// ��Ϣѭ��
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		//SendMessage(hWnd,)
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// ɾ��ע���
	return 0;
}

