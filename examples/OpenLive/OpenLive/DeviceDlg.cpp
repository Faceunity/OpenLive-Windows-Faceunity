// DeviceDlg.cpp : implement file
//

#include "stdafx.h"
#include "OpenLive.h"
#include "DeviceDlg.h"
#include "afxdialogex.h"
//////////////////////////ADD//////////////////////////////////////////
#include <funama.h>				//nama SDK 的头文件
#include <authpack.h>			//nama SDK 的key文件
#pragma comment(lib, "nama.lib")//nama SDK 的lib文件
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
//////////////////////////ADD//////////////////////////////////////////
// CDeviceDlg dialog

IMPLEMENT_DYNAMIC(CDeviceDlg, CDialogEx)

CDeviceDlg::CDeviceDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDeviceDlg::IDD, pParent)
{

}

CDeviceDlg::~CDeviceDlg()
{
}

void CDeviceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_SLKAIN_DEVICE, m_slkAudInTest);
	DDX_Control(pDX, IDC_SLKAOUT_DEVICE, m_slkAudOutTest);
	DDX_Control(pDX, IDC_SLKCAM_DEVICE, m_slkCamTest);

	DDX_Control(pDX, IDC_BTNCANCEL_DEVICE, m_btnCancel);
	DDX_Control(pDX, IDC_BTNCONFIRM_DEVICE, m_btnConfirm);
	DDX_Control(pDX, IDC_BTNAPPLY_DEVICE, m_btnApply);

//    DDX_Control(pDX, IDC_CMBAIN_DEVICE, m_cbxAIn);
//    DDX_Control(pDX, IDC_CMBAOUT_DEVICE, m_cbxAOut);
//    DDX_Control(pDX, IDC_CMBCAM_DEVICE, m_cbxCam);

	DDX_Control(pDX, IDC_SLDAIN_DEVICE, m_sldAInVol);
	DDX_Control(pDX, IDC_SLDAOUT_DEVICE, m_sldAOutVol);
}


BEGIN_MESSAGE_MAP(CDeviceDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_SHOWWINDOW()
	
	ON_STN_CLICKED(IDC_SLKAIN_DEVICE, &CDeviceDlg::OnStnClickedSlkainDevice)
	ON_STN_CLICKED(IDC_SLKAOUT_DEVICE, &CDeviceDlg::OnStnClickedSlkaoutDevice)
	ON_STN_CLICKED(IDC_SLKCAM_DEVICE, &CDeviceDlg::OnStnClickedSlkcamDevice)

	ON_BN_CLICKED(IDC_BTNCANCEL_DEVICE, &CDeviceDlg::OnBnClickedBtncancelDevice)
	ON_BN_CLICKED(IDC_BTNCONFIRM_DEVICE, &CDeviceDlg::OnBnClickedBtnconfirmDevice)
	ON_MESSAGE(WM_MSGID(EID_AUDIO_VOLUME_INDICATION), &CDeviceDlg::OnEIDAudioVolumeIndication)

	ON_BN_CLICKED(IDC_BTNAPPLY_DEVICE, &CDeviceDlg::OnBnClickedBtnapplyDevice)
END_MESSAGE_MAP()


// CDeviceDlg message deal with app
void CDeviceDlg::OnPaint()
{
	CPaintDC dc(this);

	DrawClient(&dc);
}
//////////////////////////ADD//////////////////////////////////////////
PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),
	1u,
	PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW,
	PFD_TYPE_RGBA,
	32u,
	0u, 0u, 0u, 0u, 0u, 0u,
	8u,
	0u,
	0u,
	0u, 0u, 0u, 0u,
	24u,
	8u,
	0u,
	PFD_MAIN_PLANE,
	0u,
	0u, 0u };

void InitOpenGL()
{
	HWND hw = CreateWindowExA(
		0, "EDIT", "", ES_READONLY,
		0, 0, 1, 1,
		NULL, NULL,
		GetModuleHandleA(NULL), NULL);
	HDC hgldc = GetDC(hw);
	int spf = ChoosePixelFormat(hgldc, &pfd);
	int ret = SetPixelFormat(hgldc, spf, &pfd);
	HGLRC hglrc = wglCreateContext(hgldc);
	wglMakeCurrent(hgldc, hglrc);

	//hglrc就是创建出的OpenGL context
	printf("hw=%08x hgldc=%08x spf=%d ret=%d hglrc=%08x\n",
		hw, hgldc, spf, ret, hglrc);
}
size_t FileSize(std::ifstream& file)
{
	std::streampos oldPos = file.tellg();
	file.seekg(0, std::ios::beg);
	std::streampos beg = file.tellg();
	file.seekg(0, std::ios::end);
	std::streampos end = file.tellg();
	file.seekg(oldPos, std::ios::beg);
	return static_cast<size_t>(end - beg);
}

bool LoadBundle(const std::string& filepath, std::vector<char>& data)
{
	std::ifstream fin(filepath, std::ios::binary);
	if (false == fin.good())
	{
		fin.close();
		return false;
	}
	size_t size = FileSize(fin);
	if (0 == size)
	{
		fin.close();
		return false;
	}
	data.resize(size);
	fin.read(reinterpret_cast<char*>(&data[0]), size);

	fin.close();
	return true;
}
static bool					m_namaInited = false;
static const std::string g_fuDataDir = "../../assets/";
static const std::string g_v3Data = "v3.bundle";
static const std::string g_faceBeautification = "face_beautification.bundle";
static int mFrameID = 0;
static int mBeautyHandles = 0;
static char* buf = NULL;
class AgoraVideoFrameObserver : public agora::media::IVideoFrameObserver
{
public:
	virtual bool onCaptureVideoFrame(VideoFrame& videoFrame) override
	{
		if (!m_namaInited)
		{
			InitOpenGL();
			//读取nama数据库，初始化nama
			std::vector<char> v3data;
			if (false == LoadBundle(g_fuDataDir + g_v3Data, v3data))
			{
				std::cout << "Error:缺少数据文件。" << g_fuDataDir + g_v3Data << std::endl;
				return false;
			}
			//CheckGLContext();
			fuSetup(reinterpret_cast<float*>(&v3data[0]), NULL, g_auth_package, sizeof(g_auth_package));
			std::vector<char> propData;
			if (false == LoadBundle(g_fuDataDir + g_faceBeautification, propData))
			{
				std::cout << "load face beautification data failed." << std::endl;
				return false;
			}
			std::cout << "load face beautification data." << std::endl;

			mBeautyHandles = fuCreateItemFromPackage(&propData[0], propData.size());

			m_namaInited = true;
		}

		int len = videoFrame.yStride * videoFrame.height * 3 / 2;
		if (!buf)
		{
			buf = new char[len];
		}
		memset(buf, 0x0, len);
		int yLength = videoFrame.yStride * videoFrame.height;
		memcpy(buf, videoFrame.yBuffer, yLength);
		int uLength = yLength / 4;
		memcpy(buf + yLength, videoFrame.uBuffer, uLength);
		memcpy(buf + yLength + uLength, videoFrame.vBuffer, uLength);

		fuRenderItemsEx2(FU_FORMAT_I420_BUFFER, reinterpret_cast<int*>(buf), FU_FORMAT_I420_BUFFER, reinterpret_cast<int*>(buf),
			videoFrame.yStride, videoFrame.height, mFrameID++, &mBeautyHandles, 1, NAMA_RENDER_FEATURE_FULL, NULL);

		memcpy(videoFrame.yBuffer, buf, yLength);
		memcpy(videoFrame.uBuffer, buf + yLength, uLength);
		memcpy(videoFrame.vBuffer, buf + yLength + uLength, uLength);

		//delete[]buf;
		return true;
	}

	virtual bool onRenderVideoFrame(unsigned int uid, VideoFrame& videoFrame) override
	{
		return true;
	}
};

static AgoraVideoFrameObserver s_videoFrameObserver;
//////////////////////////ADD//////////////////////////////////////////
BOOL CDeviceDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:
	m_lpRtcEngine = CAgoraObject::GetEngine();
	//////////////////////////ADD//////////////////////////////////////////
	agora::util::AutoPtr<agora::media::IMediaEngine> mediaEngine;
	mediaEngine.queryInterface(m_lpRtcEngine, agora::INTERFACE_ID_TYPE::AGORA_IID_MEDIA_ENGINE);
	mediaEngine->registerVideoFrameObserver(&s_videoFrameObserver);
	//////////////////////////ADD//////////////////////////////////////////
	m_ftLink.CreateFont(17, 0, 0, 0, FW_NORMAL, FALSE, TRUE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Arial"));
	m_ftDes.CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Arial"));
	m_ftBtn.CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Arial"));

	m_wndVideoTest.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 300, 40), this, IDC_VIDEOTEST_DEVICE);
	m_wndVideoTest.SetVolRange(100);

	m_cbxAIn.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 300, 40), this, IDC_CMBAIN_DEVICE);
	m_cbxAOut.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 300, 40), this, IDC_CMBAOUT_DEVICE);
	m_cbxCam.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 300, 40), this, IDC_CMBCAM_DEVICE);
	m_penFrame.CreatePen(PS_SOLID, 1, RGB(0xD8, 0xD8, 0xD8));

	SetBackgroundColor(RGB(0xFF, 0xFF, 0xFF), TRUE);
	InitCtrls();

	return TRUE;  // return TRUE unless you set the focus to a control
	// error:  OCX Attribute page return FALSE
}

void CDeviceDlg::EnableDeviceTest(BOOL bEnable)
{
	m_slkAudInTest.EnableWindow(bEnable);
	m_slkAudOutTest.EnableWindow(bEnable);
	m_slkCamTest.EnableWindow(bEnable);
}

void CDeviceDlg::InitCtrls()
{
	CRect ClientRect;

	MoveWindow(0, 0, 512, 640, 1);
	CenterWindow();

	GetClientRect(&ClientRect);

	m_cbxAIn.SetFaceColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF));
	m_cbxAIn.MoveWindow(160, 65, 215, 22, TRUE);
	
	m_cbxAOut.SetFaceColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF));
	m_cbxAOut.MoveWindow(160, 195, 215, 22, TRUE);

	m_cbxCam.SetFaceColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF));
	m_cbxCam.MoveWindow(160, 325, 215, 22, TRUE);

	m_sldAInVol.MoveWindow(155, 110, 285, 24, TRUE);
	m_sldAOutVol.MoveWindow(155, 245, 285, 24, TRUE);
	m_wndVideoTest.MoveWindow(155, 378, 192, 120, TRUE);

	m_slkAudInTest.SetFont(&m_ftLink);
	m_slkAudInTest.MoveWindow(405, 65, 72, 40, TRUE);
	m_slkAudOutTest.SetFont(&m_ftLink);
	m_slkAudOutTest.MoveWindow(405, 200, 72, 40, TRUE);
	m_slkCamTest.SetFont(&m_ftLink);
	m_slkCamTest.MoveWindow(405, 325, 72, 40, TRUE);

	m_btnCancel.MoveWindow(46, ClientRect.Height() - 88, 120, 36, TRUE);

	m_btnConfirm.MoveWindow(199, ClientRect.Height() - 88, 120, 36, TRUE);
	
	m_btnApply.MoveWindow(346, ClientRect.Height() - 88, 120, 36, TRUE);
	
	m_cbxAIn.SetFont(&m_ftDes);
	m_cbxAOut.SetFont(&m_ftDes);
	m_cbxCam.SetFont(&m_ftDes);

	m_btnCancel.SetBackColor(RGB(0, 160, 239), RGB(0, 160, 239), RGB(0, 160, 239), RGB(192, 192, 192));
	m_btnCancel.SetFont(&m_ftBtn);
	m_btnCancel.SetTextColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xC8, 0x64), RGB(0xFF, 0xC8, 0x64), RGB(0xCC, 0xCC, 0xCC));
	m_btnCancel.SetWindowText(LANG_STR("IDS_DEVICE_CANCEL"));

	m_btnConfirm.SetBackColor(RGB(248, 170, 31), RGB(248, 170, 31), RGB(248, 170, 31), RGB(0xCC, 0xCC, 0xCC));
	m_btnConfirm.SetFont(&m_ftBtn);
	m_btnConfirm.SetTextColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xC8, 0x64), RGB(0xFF, 0xC8, 0x64), RGB(0xCC, 0xCC, 0xCC));
	m_btnConfirm.SetWindowText(LANG_STR("IDS_DEVICE_CONFIRM"));

	m_cbxAIn.SetButtonImage(IDB_CMBBTN, 12, 12, RGB(0xFF, 0x00, 0xFF));
	m_cbxAOut.SetButtonImage(IDB_CMBBTN, 12, 12, RGB(0xFF, 0x00, 0xFF));
	m_cbxCam.SetButtonImage(IDB_CMBBTN, 12, 12, RGB(0xFF, 0x00, 0xFF));
	m_cbxAIn.SetFont(&m_ftDes);
	m_cbxAOut.SetFont(&m_ftDes);
	m_cbxCam.SetFont(&m_ftDes);

	m_slkAudInTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	m_slkAudOutTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	m_slkCamTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));

	m_btnCancel.SetBorderColor(RGB(0xD8, 0xD8, 0xD8), RGB(0x00, 0xA0, 0xE9), RGB(0x00, 0xA0, 0xE9), RGB(0xCC, 0xCC, 0xCC));
	m_btnCancel.SetBackColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF));
	m_btnCancel.SetFont(&m_ftBtn);
	m_btnCancel.SetTextColor(RGB(0x55, 0x58, 0x5A), RGB(0x00, 0xA0, 0xE9), RGB(0x00, 0xA0, 0xE9), RGB(0xCC, 0xCC, 0xCC));
	m_btnCancel.SetWindowText(LANG_STR("IDS_DEVICE_CANCEL"));

	m_btnConfirm.SetBackColor(RGB(0x00, 0xA0, 0xE9), RGB(0x05, 0x78, 0xAA), RGB(0x05, 0x78, 0xAA), RGB(0xE6, 0xE6, 0xE6));
	m_btnConfirm.SetFont(&m_ftBtn);
	m_btnConfirm.SetTextColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xCC, 0xCC, 0xCC));
	m_btnConfirm.SetWindowText(LANG_STR("IDS_DEVICE_CONFIRM"));

	m_btnApply.SetBackColor(RGB(0x00, 0xA0, 0xE9), RGB(0x05, 0x78, 0xAA), RGB(0x05, 0x78, 0xAA), RGB(0xE6, 0xE6, 0xE6));
	m_btnApply.SetFont(&m_ftBtn);
	m_btnApply.SetTextColor(RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xCC, 0xCC, 0xCC));
	m_btnApply.SetWindowText(LANG_STR("IDS_DEVICE_APPLY"));

	m_sldAInVol.SetThumbBitmap(IDB_SLDTHUMB_NORMAL, IDB_SLDTHUMB_HOT, IDB_SLDTHUMB_HOT, RGB(0xFF, 0, 0xFF));
	m_sldAInVol.SetRange(0, 255);

	m_sldAOutVol.SetThumbBitmap(IDB_SLDTHUMB_NORMAL, IDB_SLDTHUMB_HOT, IDB_SLDTHUMB_HOT, RGB(0xFF, 0, 0xFF));
	m_sldAOutVol.SetRange(0, 255);
}

void CDeviceDlg::DrawClient(CDC *lpDC)
{
	CRect	rcText;
	CRect	rcClient;
	LPCTSTR lpString = NULL;

	GetClientRect(&rcClient);

	CFont *lpDefFont = lpDC->SelectObject(&m_ftDes);
	CPen *lpDefPen = lpDC->SelectObject(&m_penFrame);
	
	lpDC->SetBkColor(RGB(0xFF, 0xFF, 0xFF));
	lpDC->SetTextColor(RGB(0xD8, 0xD8, 0xD8));

	rcText.SetRect(rcClient.Width() / 2 - 190, 60, rcClient.Width() / 2 + 130, 92);
	lpDC->RoundRect(&rcText, CPoint(32, 32));

	lpString = LANG_STR("IDS_DEVICE_AUDIOIN");
	lpDC->TextOut(80, 69, lpString, _tcslen(lpString));

	rcText.SetRect(rcClient.Width() / 2 - 190, 190, rcClient.Width() / 2 + 130, 222);
	lpDC->RoundRect(&rcText, CPoint(32, 32));
	lpString = LANG_STR("IDS_DEVICE_AUDIOOUT");
	lpDC->TextOut(80, 199, lpString, _tcslen(lpString));

	rcText.SetRect(rcClient.Width() / 2 - 190, 320, rcClient.Width() / 2 + 130, 352);
	lpDC->RoundRect(&rcText, CPoint(32, 32));
	lpString = LANG_STR("IDS_DEVICE_CAMERA");
	lpDC->TextOut(80, 329, lpString, _tcslen(lpString));

	lpString = LANG_STR("IDS_DEVICE_VOLUME");
	lpDC->SetTextColor(RGB(0x00, 0x00, 0x00));
	lpDC->TextOut(66, 115, lpString, _tcslen(lpString));
	lpDC->TextOut(66, 250, lpString, _tcslen(lpString));

	lpDC->SelectObject(lpDefPen);
	lpDC->SelectObject(lpDefFont);
}

void CDeviceDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 
	if (!bShow) {
		m_agPlayout.Close();
		m_agAudioin.Close();
		m_agCamera.Close();
		return;
	}

	CString strDeviceName;
	CString strDeviceID;
	CString strCurID;

	m_agPlayout.Create(m_lpRtcEngine);
	m_agAudioin.Create(m_lpRtcEngine);
	m_agCamera.Create(m_lpRtcEngine);
	
	m_sldAOutVol.SetPos(m_agPlayout.GetVolume());
	m_sldAInVol.SetPos(m_agAudioin.GetVolume());

	m_cbxAOut.ResetContent();
	strCurID = m_agPlayout.GetCurDeviceID();
	for (UINT nIndex = 0; nIndex < m_agPlayout.GetDeviceCount(); nIndex++){
		m_agPlayout.GetDevice(nIndex, strDeviceName, strDeviceID);
		m_cbxAOut.InsertString(nIndex, strDeviceName);

		if (strCurID == strDeviceID)
			m_cbxAOut.SetCurSel(nIndex);
	}

	m_cbxAIn.ResetContent();
	strCurID = m_agAudioin.GetCurDeviceID();
	for (UINT nIndex = 0; nIndex < m_agAudioin.GetDeviceCount(); nIndex++){
		m_agAudioin.GetDevice(nIndex, strDeviceName, strDeviceID);
		m_cbxAIn.InsertString(nIndex, strDeviceName);

		if (strCurID == strDeviceID)
			m_cbxAIn.SetCurSel(nIndex);
	}

	m_cbxCam.ResetContent();
	strCurID = m_agCamera.GetCurDeviceID();
	for (UINT nIndex = 0; nIndex < m_agCamera.GetDeviceCount(); nIndex++){
		m_agCamera.GetDevice(nIndex, strDeviceName, strDeviceID);
		m_cbxCam.InsertString(nIndex, strDeviceName);

		if (strCurID == strDeviceID)
			m_cbxCam.SetCurSel(nIndex);
	}
}


BOOL CDeviceDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO:  在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYDOWN){
		switch (pMsg->wParam){
		case VK_ESCAPE:
			OnBnClickedBtncancelDevice();
			return FALSE;
		case VK_RETURN:
			OnBnClickedBtnconfirmDevice();
			return FALSE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CDeviceDlg::OnStnClickedSlkainDevice()
{
	// TODO:
	if (m_agAudioin.IsTesting()) {
		m_agAudioin.TestAudInputDevice(NULL, FALSE);
		m_slkAudInTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	}
	else {
		m_agAudioin.TestAudInputDevice(GetSafeHwnd(), TRUE);
		m_slkAudInTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTESTOFF"));
	}
}


void CDeviceDlg::OnStnClickedSlkaoutDevice()
{
	// TODO:
	if (m_agPlayout.IsTesting()) {
		m_agPlayout.TestPlaybackDevice(ID_TEST_AUDIO, FALSE);
		m_slkAudOutTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	}
	else {
		m_agPlayout.TestPlaybackDevice(ID_TEST_AUDIO, TRUE);
		m_slkAudOutTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTESTOFF"));
	}
}


void CDeviceDlg::OnStnClickedSlkcamDevice()
{
	// TODO:
	if (m_agCamera.IsTesting()) {
		m_agCamera.TestCameraDevice(NULL, FALSE);
		m_slkCamTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	}
	else {
		m_agCamera.TestCameraDevice(m_wndVideoTest.GetVideoSafeHwnd(), TRUE);
		m_slkCamTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTESTOFF"));
	}
}


void CDeviceDlg::OnBnClickedBtncancelDevice()
{
	// TODO:
	m_agAudioin.TestAudInputDevice(NULL, FALSE);
	m_slkAudInTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));

	m_agPlayout.TestPlaybackDevice(ID_TEST_AUDIO, FALSE);
	m_slkAudOutTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));

	m_agCamera.TestCameraDevice(NULL, FALSE);
	m_slkCamTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));

	CDialogEx::OnCancel();
}


void CDeviceDlg::OnBnClickedBtnconfirmDevice()
{
	// TODO:
	int		nCurSel = 0;
	CString strDeviceName;
	CString strDeviceID;

	m_agAudioin.SetVolume(m_sldAInVol.GetPos());
	m_agPlayout.SetVolume(m_sldAOutVol.GetPos());

	m_agAudioin.TestAudInputDevice(NULL, FALSE);
	m_slkAudInTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	nCurSel = m_cbxAIn.GetCurSel();
	if (nCurSel != -1) {
		m_agAudioin.GetDevice(nCurSel, strDeviceName, strDeviceID);
		m_agAudioin.SetCurDevice(strDeviceID);
	}


	m_agPlayout.TestPlaybackDevice(ID_TEST_AUDIO, FALSE);
	m_slkAudOutTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	nCurSel = m_cbxAOut.GetCurSel();
	if (nCurSel != -1) {
		m_agPlayout.GetDevice(nCurSel, strDeviceName, strDeviceID);
		m_agPlayout.SetCurDevice(strDeviceID);
	}

	m_agCamera.TestCameraDevice(NULL, FALSE);
	m_slkCamTest.SetWindowText(LANG_STR("IDS_DEVICE_BTNTEST"));
	nCurSel = m_cbxCam.GetCurSel();
	if (nCurSel != -1) {
		m_agCamera.GetDevice(nCurSel, strDeviceName, strDeviceID);
		m_agCamera.SetCurDevice(strDeviceID);
	}

	CDialogEx::OnOK();
}

void CDeviceDlg::OnBnClickedBtnapplyDevice()
{
	// TODO: 
	int		nCurSel = 0;
	CString strDeviceName;
	CString strDeviceID;

	m_agAudioin.SetVolume(m_sldAInVol.GetPos());
	m_agPlayout.SetVolume(m_sldAOutVol.GetPos());

	nCurSel = m_cbxAIn.GetCurSel();
	if (nCurSel != -1) {
		m_agAudioin.GetDevice(nCurSel, strDeviceName, strDeviceID);
		m_agAudioin.SetCurDevice(strDeviceID);
	}

	nCurSel = m_cbxAOut.GetCurSel();
	if (nCurSel != -1) {
		m_agPlayout.GetDevice(nCurSel, strDeviceName, strDeviceID);
		m_agPlayout.SetCurDevice(strDeviceID);
	}

	nCurSel = m_cbxCam.GetCurSel();
	if (nCurSel != -1) {
		m_agCamera.GetDevice(nCurSel, strDeviceName, strDeviceID);
		m_agCamera.SetCurDevice(strDeviceID);
	}
}


LRESULT CDeviceDlg::OnEIDAudioVolumeIndication(WPARAM wParam, LPARAM lParam)
{
	LPAGE_AUDIO_VOLUME_INDICATION lpData = (LPAGE_AUDIO_VOLUME_INDICATION)wParam;
	m_wndVideoTest.SetCurVol(lpData->totalVolume);

	delete lpData;

	return 0;
}