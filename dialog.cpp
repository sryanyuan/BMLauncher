#include "dialog.h"
#include "ui_dialog.h"
#include <QDebug>
#include <QPainter>
#include "SimpleIni.h"
#include <QDir>
#include <string>
#include <io.h>
#include <QProcess>
#include <QMessageBox>
#include <qdebug.h>
#include <windows.h>
#ifdef UNICODE
#undef UNICODE
#endif
#include <TlHelp32.h>
#include <tchar.h>

inline DWORD GetProcessIdFromName(const char* ProcessName)
{
    PROCESSENTRY32 pe32;
    HANDLE hSnapshot = NULL;
    ZeroMemory(&pe32, sizeof(PROCESSENTRY32));

    // We want a snapshot of processes
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    // Check for a valid handle, in this case we need to check for
    // INVALID_HANDLE_VALUE instead of NULL
    if(hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    // Now we can enumerate the running process, also
    // we can't forget to set the PROCESSENTRY32.dwSize member
    // otherwise the following functions will fail
    pe32.dwSize =  sizeof(PROCESSENTRY32);

    if(Process32First(hSnapshot, &pe32) == FALSE)
    {
        // Cleanup the mess
        CloseHandle(hSnapshot);
        return 0;
    }

    // Do our first comparison
    if(_tcsicmp(pe32.szExeFile, ProcessName) == FALSE)
    {
        // Cleanup the mess
        CloseHandle(hSnapshot);
        return pe32.th32ProcessID;
    }

    // Most likely it won't match on the first try so
    // we loop through the rest of the entries until
    // we find the matching entry or not one at all
    while (Process32Next(hSnapshot, &pe32))
    {
        if(_tcsicmp(pe32.szExeFile, ProcessName) == 0)
        {
            // Cleanup the mess
            CloseHandle(hSnapshot);
            return pe32.th32ProcessID;
        }
    }

    // If we made it this far there wasn't a match
    // so we'll return 0
    CloseHandle(hSnapshot);
    return 0;
}

using std::string;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    //setWindowOpacity(1);
    setWindowFlags(Qt::FramelessWindowHint /*| Qt::WindowStaysOnTopHint*/);
    setAttribute(Qt::WA_TranslucentBackground);

    m_bLeftPressed = false;
    m_xBkImg.load(":/images/dlgbke.png");
    LoadConfigToApp();

    m_pAppClient = new QProcess();
    m_pAppServer = new QProcess();

    //  Connection
    connect(ui->m_pLeServerAddr, SIGNAL(editingFinished()), this, SLOT(slot_VerifyIpInput()));
    connect(m_pAppClient, SIGNAL(finished(int)), this, SLOT(slot_appClient_close(int)));
    connect(m_pAppServer, SIGNAL(finished(int)), this, SLOT(slot_appServer_close(int)));
}

Dialog::~Dialog()
{
    delete ui;
    if(m_pAppClient->isOpen())
    {
        m_pAppClient->close();
    }
    if(m_pAppServer->isOpen())
    {
        m_pAppServer->close();
    }
    delete m_pAppClient;
    delete m_pAppServer;
}

void Dialog::mouseMoveEvent(QMouseEvent *_e)
{
    QDialog::mouseMoveEvent(_e);

    if(m_bLeftPressed)
    {
        move(_e->globalPos() - m_xPressedPos);
    }
}

void Dialog::mousePressEvent(QMouseEvent *_e)
{
    QDialog::mousePressEvent(_e);

    if(_e->button() == Qt::LeftButton)
    {
        if(_e->pos().y() <= 20)
        {
            m_xPressedPos = _e->pos();
            m_bLeftPressed = true;
        }
    }
}

void Dialog::mouseReleaseEvent(QMouseEvent *_e)
{
    QDialog::mouseReleaseEvent(_e);

    if(_e->button() == Qt::LeftButton)
    {
        m_bLeftPressed = false;
    }
}

void Dialog::paintEvent(QPaintEvent *e)
{
    QDialog::paintEvent(e);

    QPainter painter(this);
    painter.drawPixmap(0, 0, m_xBkImg);
}

void Dialog::LoadConfigToApp()
{
    static char szCurAppPath[255] = {0};
    if(szCurAppPath[0] == 0)
    {
        QString xWorkingPath = QCoreApplication::applicationFilePath();
        //  Translate to directory
        for(int i = xWorkingPath.length(); i >= 0; --i)
        {
            if(xWorkingPath[i] == '/')
            {
                break;
            }
            else
            {
                xWorkingPath[i] = 0;
            }
        }
        string xStdStr = xWorkingPath.toStdString();
        strcpy(szCurAppPath, xStdStr.c_str());
        strcat(szCurAppPath, "/login.ini");
    }

    if(0 != access(szCurAppPath, 0))
    {
        //  Break
        return;
    }

    CSimpleIniA xIniFile;
    if(SI_OK == xIniFile.LoadFile(szCurAppPath))
    {
        int nMultiMode = xIniFile.GetLongValue("LOGIN",
                              "ENABLELOGIN");
        if(nMultiMode != 0)
        {
            ui->m_pLsGroup->setChecked(true);
        }

        const char* pszValue = xIniFile.GetValue("LOGIN",
                          "SERVER");
        if(NULL != pszValue)
        {
            ui->m_pLeServerAddr->setText(pszValue);
        }

        pszValue = xIniFile.GetValue("LOGIN",
                          "CLIENT");
        if(NULL != pszValue)
        {
            ui->m_pLeClientAddr->setText(pszValue);
        }

        pszValue = xIniFile.GetValue("LOGIN",
                          "LOGINLC");
        if(NULL != pszValue)
        {
            ui->m_pLeLoginLCAddr->setText(pszValue);
        }

        pszValue = xIniFile.GetValue("LOGIN",
                          "LOGINLS");
        if(NULL != pszValue)
        {
            ui->m_pLeLoginLSAddr->setText(pszValue);
        }
    }
}

bool Dialog::CheckInputIpValid(QLineEdit *_pEdit)
{
    QString xInput = _pEdit->text();
    int nIpAndPort[5] = {0};

    string xStdInput = xInput.toStdString();
    //  Check valid
    for(int i = 0; i < xInput.length(); ++i)
    {
        QChar cValue = xInput[i];
        if(cValue < '0' &&
                cValue > '9')
        {
            return false;
        }
    }

    if(5 == sscanf(xStdInput.c_str(), "%d.%d.%d.%d:%d",
                   &nIpAndPort[0],
                   &nIpAndPort[1],
                   &nIpAndPort[2],
                   &nIpAndPort[3],
                   &nIpAndPort[4]))
    {
        for(int i = 0; i < 4; ++i)
        {
            if(nIpAndPort[i] < 0 ||
                    nIpAndPort[i] > 255)
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool Dialog::GetInputIp(QLineEdit* _pEdit, int* _pOutArray)
{
    QString xInput = _pEdit->text();

    string xStdInput = xInput.toStdString();
    //  Check valid
    for(int i = 0; i < xInput.length(); ++i)
    {
        QChar cValue = xInput[i];
        if(cValue < '0' &&
                cValue > '9')
        {
            return false;
        }
    }

    if(5 == sscanf(xStdInput.c_str(), "%d.%d.%d.%d:%d",
                   &_pOutArray[0],
                   &_pOutArray[1],
                   &_pOutArray[2],
                   &_pOutArray[3],
                   &_pOutArray[4]))
    {
        for(int i = 0; i < 4; ++i)
        {
            if(_pOutArray[i] < 0 ||
                    _pOutArray[i] > 255)
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

void Dialog::SaveConfigToFile()
{
    static char szCurAppPath[255] = {0};
    if(szCurAppPath[0] == 0)
    {
        QString xWorkingPath = QCoreApplication::applicationFilePath();
        //  Translate to directory
        for(int i = xWorkingPath.length(); i >= 0; --i)
        {
            if(xWorkingPath[i] == '/')
            {
                break;
            }
            else
            {
                xWorkingPath[i] = 0;
            }
        }
        string xStdStr = xWorkingPath.toStdString();
        strcpy(szCurAppPath, xStdStr.c_str());
        strcat(szCurAppPath, "/login.ini");
    }

    if(0 != access(szCurAppPath, 0))
    {
        //  Create
        QFile xFile(szCurAppPath);
        xFile.open(QFile::ReadWrite);
        xFile.close();
    }

    CSimpleIniA xIniFile;
    if(SI_OK == xIniFile.LoadFile(szCurAppPath))
    {
        int nMultiMode = ui->m_pLsGroup->isChecked() ? 1 : 0;
        xIniFile.SetLongValue("LOGIN",
                              "ENABLELOGIN",
                              nMultiMode);

        std::string xValue;
        xValue = ui->m_pLeServerAddr->text().toStdString();
        const char* pszValue = xValue.c_str();
        xIniFile.SetValue("LOGIN",
                          "SERVER",
                          pszValue);

        xValue = ui->m_pLeClientAddr->text().toStdString();
        pszValue = xValue.c_str();
        xIniFile.SetValue("LOGIN",
                          "CLIENT",
                          pszValue);

        xValue = ui->m_pLeLoginLCAddr->text().toStdString();
        pszValue = xValue.c_str();
        xIniFile.SetValue("LOGIN",
                          "LOGINLC",
                          pszValue);

        xValue = ui->m_pLeLoginLSAddr->text().toStdString();
        pszValue = xValue.c_str();
        xIniFile.SetValue("LOGIN",
                          "LOGINLS",
                          pszValue);

        xIniFile.SaveFile(szCurAppPath);
    }
}

void Dialog::slot_VerifyIpInput()
{
    QObject* pSender = sender();
    return;

    if((pSender->objectName() == "m_pLeServerAddr") ||
            (pSender->objectName() == "m_pLeClientAddr"))
    {
        QLineEdit* pEdit = static_cast<QLineEdit*>(pSender);

        if(!CheckInputIpValid(pEdit))
        {
            ui->m_pLblTip->setText("请输入正确的IP地址，格式(0-255)XXX.XXX.XXX.XXX:XXX");
        }
        else
        {
            ui->m_pLblTip->setText("BackMIR游戏启动程序");
        }
    }
}


void Dialog::on_m_pBtnClose_clicked()
{
    if(m_pAppClient->isOpen() ||
            m_pAppServer->isOpen())
    {
        int nChose = QMessageBox::question(this, "询问", "您的游戏处于开启状态中,关闭本登陆器将关闭游戏,确认关闭?",
                                 QMessageBox::Ok , QMessageBox::Cancel);
        if(nChose == QMessageBox::Cancel)
        {
            return;
        }
    }
    SaveConfigToFile();
    qApp->quit();
}

void Dialog::on_m_pBtnMin_clicked()
{
    showMinimized();
}

bool Dialog::LaunchClient()
{
    return RunClient();

    if(m_pAppClient->isOpen())
    {
        int nRet = QMessageBox::question(this, "询问", "您的客户端已在运行中，若继续将退出客户端再运行新实例，要继续吗？", QMessageBox::Yes, QMessageBox::No);
        if(QMessageBox::No == nRet)
        {
            return true;
        }
    }

    int nClientInfo[5] = {0};
    char szInvokeParam[255] = {0};
    bool bMultiMode = ui->m_pSelectGroup->isChecked();

    //  Start client
    sprintf(szInvokeParam, "%s/BackMIR.exe", GetRootPath());
    QString xRunPath = szInvokeParam;

    if(bMultiMode)
    {
        bool bCanStart = GetInputIp(ui->m_pLeClientAddr, nClientInfo);
        if(bCanStart)
        {
            sprintf(szInvokeParam, "svrip=%d.%d.%d.%d|%d",
                    nClientInfo[0],
                    nClientInfo[1],
                    nClientInfo[2],
                    nClientInfo[3],
                    nClientInfo[4]);
            ui->m_pLblTip->setText("BackMIR游戏启动程序");
        }
        else
        {
            ui->m_pLblTip->setText("请输入正确的IP地址，格式(0-255)XXX.XXX.XXX.XXX:XXX");
            return false;
        }
    }
    else
    {
        strcpy(szInvokeParam, "svrip=127.0.0.1|8300");
        ui->m_pLblTip->setText("BackMIR游戏启动程序");
    }

    QStringList xRunParam;
    xRunParam.append(szInvokeParam);

    if(m_pAppClient->isOpen())
    {
        m_pAppClient->close();
    }
    m_pAppClient->start(xRunPath, xRunParam);
    bool bOk = m_pAppClient->waitForStarted();
    if(!bOk)
    {
        m_pAppClient->close();
    }
    return bOk;
}

bool Dialog::LaunchServer()
{
    return RunServer();

    if(m_pAppServer->isOpen())
    {
        int nRet = QMessageBox::question(this, "询问", "您的服务器已在运行中，若继续将退出服务器再运行新实例，要继续吗？", QMessageBox::Yes, QMessageBox::No);
        if(QMessageBox::No == nRet)
        {
            return true;
        }
    }
    int nServerInfo[5] = {0};
    char szInvokeParam[255] = {0};
    bool bMultiMode = ui->m_pSelectGroup->isChecked();

    //if(!ui->m_pBtnStartSvr->isChecked())
    if(false)
    {
        return true;
    }

    //  Start server
    sprintf(szInvokeParam, "%s/GameSvr.exe", GetRootPath());
    QString xRunPath = szInvokeParam;

    if(bMultiMode)
    {
        bool bCanStart = GetInputIp(ui->m_pLeServerAddr, nServerInfo);
        if(bCanStart)
        {
            sprintf(szInvokeParam, "listenip=%d.%d.%d.%d|%d",
                    nServerInfo[0],
                    nServerInfo[1],
                    nServerInfo[2],
                    nServerInfo[3],
                    nServerInfo[4]);
            ui->m_pLblTip->setText("BackMIR游戏启动程序");
        }
        else
        {
            ui->m_pLblTip->setText("请输入正确的IP地址，格式(0-255)XXX.XXX.XXX.XXX:XXX");
            return false;
        }
    }
    else
    {
        strcpy(szInvokeParam, "listenip=127.0.0.1|8300");
        ui->m_pLblTip->setText("BackMIR游戏启动程序");
    }

    QStringList xRunParam;
    xRunParam.append(szInvokeParam);

    if(m_pAppServer->isOpen())
    {
        m_pAppServer->close();
    }
    m_pAppServer->start(xRunPath, xRunParam);
    bool bOk = m_pAppServer->waitForStarted();
    if(!bOk)
    {
        m_pAppServer->close();
    }
    return bOk;
}

void Dialog::on_m_pBtnStart_clicked()
{
    if(!RunLogin())
    {
        QMessageBox::warning(this, "错误", "无法启动登陆服务器，请检查是否文件丢失或者被安全软件禁止启动", QMessageBox::Ok);
    }
}


const char* Dialog::GetRootPath()
{
    static char szCurAppPath[255] = {0};
    if(szCurAppPath[0] == 0)
    {
        QString xWorkingPath = QCoreApplication::applicationFilePath();
        //  Translate to directory
        for(int i = xWorkingPath.length(); i >= 0; --i)
        {
            if(xWorkingPath[i] == '/')
            {
                break;
            }
            else
            {
                xWorkingPath[i] = 0;
            }
        }

        string xPath = xWorkingPath.toStdString();
        for(size_t i = 0; i < xPath.length(); ++i)
        {
            szCurAppPath[i] = xPath[i];
        }
    }

    return szCurAppPath;
}

void Dialog::on_m_pBtnServer_clicked()
{
    if(!LaunchServer())
    {
        QMessageBox::critical(this, "错误", "无法启动游戏服务器，请检查是否文件丢失或者被安全软件禁止启动", QMessageBox::Ok);
    }
}

void Dialog::on_m_pBtnClient_clicked()
{
    if(!LaunchClient())
    {
        QMessageBox::critical(this, "错误", "无法启动游戏客户端，请检查是否文件丢失或者被安全软件禁止启动", QMessageBox::Ok);
    }
}

void Dialog::slot_appClient_close(int _nCode)
{
    if(m_pAppClient->isOpen())
    {
        m_pAppClient->close();
    }
}

void Dialog::slot_appServer_close(int _nCode)
{
    if(m_pAppServer->isOpen())
    {
        m_pAppClient->close();
    }
}

bool Dialog::RunServer()
{
    if(!CheckInputIpValid(ui->m_pLeServerAddr))
    {
        QMessageBox::warning(this, "警告", "游戏服务器IP地址输入有误", QMessageBox::Ok);
        return false;
    }
    bool bUseLogin = ui->m_pLsGroup->isChecked();
    if(bUseLogin)
    {
        if(!CheckInputIpValid(ui->m_pLeLoginLSAddr))
        {
            QMessageBox::warning(this, "警告", "登陆服务器IP地址输入有误", QMessageBox::Ok);
            return false;
        }
    }

    std::string xRunParam;
    xRunParam = "listenip=" + ui->m_pLeServerAddr->text().toStdString();
    if(bUseLogin)
    {
        xRunParam += " loginsvr=" + ui->m_pLeLoginLSAddr->text().toStdString();
    }

    char szServerPath[MAX_PATH];
    sprintf(szServerPath, "%s\\GameSvr.exe", GetRootPath());
    if(0 != access(szServerPath, 0))
    {
        QMessageBox::warning(this, "警告", "游戏服务器不存在", QMessageBox::Ok);
        return false;
    }

    STARTUPINFOA stStartInfo;
    ZeroMemory(&stStartInfo, sizeof(STARTUPINFO));
    stStartInfo.cb = sizeof(STARTUPINFO);
    stStartInfo.dwFlags = STARTF_USESHOWWINDOW;
    stStartInfo.wShowWindow = SW_NORMAL;
    PROCESS_INFORMATION stProcessInfo;

    char szCmdLine[MAX_PATH];
    strcpy(szCmdLine, xRunParam.c_str());

    BOOL bRet = CreateProcessA(szServerPath,
                                szCmdLine,
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &stStartInfo,
                                &stProcessInfo);
    if(!bRet)
    {
        QMessageBox::warning(this, "警告", "启动游戏服务器失败", QMessageBox::Ok);
        return false;
    }
    else
    {
        CloseHandle(stProcessInfo.hProcess);
    }
    return true;
}

bool Dialog::RunLogin()
{
    bool bUseLogin = ui->m_pLsGroup->isChecked();
    if(bUseLogin)
    {
        if(!CheckInputIpValid(ui->m_pLeLoginLCAddr) ||
                !CheckInputIpValid(ui->m_pLeLoginLSAddr))
        {
            QMessageBox::warning(this, "警告", "登陆服务器IP地址输入有误", QMessageBox::Ok);
            return false;
        }
    }
    else
    {
        return false;
    }

    std::string xRunParam;
    xRunParam = "-lsaddr=" + ui->m_pLeLoginLCAddr->text().toStdString();
    xRunParam += " -lsgsaddr=" + ui->m_pLeLoginLSAddr->text().toStdString();

    char szServerPath[MAX_PATH];
    sprintf(szServerPath, "%s\\BMLoginSvrApp.exe", GetRootPath());
    if(0 != access(szServerPath, 0))
    {
        QMessageBox::warning(this, "警告", "登陆服务器不存在", QMessageBox::Ok);
        return false;
    }

   /* STARTUPINFOA stStartInfo;
    ZeroMemory(&stStartInfo, sizeof(STARTUPINFO));
    stStartInfo.cb = sizeof(STARTUPINFO);
    stStartInfo.dwFlags = STARTF_USESHOWWINDOW;
    stStartInfo.wShowWindow = SW_NORMAL;
    PROCESS_INFORMATION stProcessInfo;

    char szCmdLine[MAX_PATH];
    strcpy(szCmdLine, xRunParam.c_str());
    qDebug() << szCmdLine;

    BOOL bRet = CreateProcessA(szServerPath,
                                szCmdLine,
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &stStartInfo,
                                &stProcessInfo);
    if(!bRet)
    {
        QMessageBox::warning(this, "警告", "启动登陆服务器失败", QMessageBox::Ok);
        return false;
    }
    else
    {
        CloseHandle(stProcessInfo.hProcess);
    }*/
    char szCmdLine[MAX_PATH];
    sprintf(szCmdLine, "%s\\BMLoginSvrApp.exe %s", GetRootPath(), xRunParam.c_str());
    qDebug() << szCmdLine;
    if(WinExec(szCmdLine, SW_NORMAL) <= 31)
    {
        QMessageBox::warning(this, "警告", "启动登陆服务器失败", QMessageBox::Ok);
        return false;
    }
    return true;
}

bool Dialog::RunClient()
{
    bool bUseLogin = ui->m_pLsGroup->isChecked();
    if(bUseLogin)
    {
        if(!CheckInputIpValid(ui->m_pLeLoginLCAddr))
        {
            QMessageBox::warning(this, "警告", "登陆服务器IP地址输入有误", QMessageBox::Ok);
            return false;
        }
    }
    else
    {
        if(!CheckInputIpValid(ui->m_pLeServerAddr))
        {
            QMessageBox::warning(this, "警告", "游戏服务器IP地址输入有误", QMessageBox::Ok);
            return false;
        }
    }

    std::string xRunParam;
    if(bUseLogin)
    {
        xRunParam = "svrip=" + ui->m_pLeLoginLCAddr->text().toStdString();
    }
    else
    {
        xRunParam = "svrip=" + ui->m_pLeServerAddr->text().toStdString();
    }

    char szServerPath[MAX_PATH];
    sprintf(szServerPath, "%s\\BackMir.exe", GetRootPath());
    if(0 != access(szServerPath, 0))
    {
        QMessageBox::warning(this, "警告", "游戏客户端不存在", QMessageBox::Ok);
        return false;
    }

    STARTUPINFOA stStartInfo;
    ZeroMemory(&stStartInfo, sizeof(STARTUPINFO));
    stStartInfo.cb = sizeof(STARTUPINFO);
    stStartInfo.dwFlags = STARTF_USESHOWWINDOW;
    stStartInfo.wShowWindow = SW_NORMAL;
    PROCESS_INFORMATION stProcessInfo;

    char szCmdLine[MAX_PATH];
    strcpy(szCmdLine, xRunParam.c_str());

    BOOL bRet = CreateProcessA(szServerPath,
                                szCmdLine,
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &stStartInfo,
                                &stProcessInfo);
    if(!bRet)
    {
        QMessageBox::warning(this, "警告", "启动游戏客户端失败", QMessageBox::Ok);
        return false;
    }
    else
    {
        CloseHandle(stProcessInfo.hProcess);
    }
    return true;
}

void Dialog::on_m_pBtnSingle_clicked()
{
    std::string xRunParam;
    xRunParam = "listenip=127.0.0.1:8302";

    char szServerPath[MAX_PATH];
    sprintf(szServerPath, "%s\\GameSvr.exe", GetRootPath());
    if(0 != access(szServerPath, 0))
    {
        QMessageBox::warning(this, "警告", "游戏服务器不存在", QMessageBox::Ok);
        return;
    }

    STARTUPINFOA stStartInfo;
    ZeroMemory(&stStartInfo, sizeof(STARTUPINFO));
    stStartInfo.cb = sizeof(STARTUPINFO);
    stStartInfo.dwFlags = STARTF_USESHOWWINDOW;
    stStartInfo.wShowWindow = SW_NORMAL;
    PROCESS_INFORMATION stProcessInfo;

    char szCmdLine[MAX_PATH];
    strcpy(szCmdLine, xRunParam.c_str());

    if(0 == GetProcessIdFromName("GameSvr.exe"))
    {
        BOOL bRet = CreateProcessA(szServerPath,
                                    szCmdLine,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    0,
                                    NULL,
                                    NULL,
                                    &stStartInfo,
                                    &stProcessInfo);
        if(!bRet)
        {
            QMessageBox::warning(this, "警告", "启动游戏服务器失败", QMessageBox::Ok);
            return;
        }
        else
        {
            CloseHandle(stProcessInfo.hProcess);
        }
    }

    xRunParam = "svrip=127.0.0.1:8302";
    sprintf(szServerPath, "%s\\BackMir.exe", GetRootPath());
    if(0 != access(szServerPath, 0))
    {
        QMessageBox::warning(this, "警告", "游戏客户端不存在", QMessageBox::Ok);
        return;
    }

    ZeroMemory(&stStartInfo, sizeof(STARTUPINFO));
    stStartInfo.cb = sizeof(STARTUPINFO);
    stStartInfo.dwFlags = STARTF_USESHOWWINDOW;
    stStartInfo.wShowWindow = SW_NORMAL;

    strcpy(szCmdLine, xRunParam.c_str());

    BOOL bRet = CreateProcessA(szServerPath,
                          szCmdLine,
                          NULL,
                          NULL,
                          FALSE,
                          0,
                          NULL,
                          NULL,
                          &stStartInfo,
                          &stProcessInfo);
    if(!bRet)
    {
        QMessageBox::warning(this, "警告", "启动游戏客户端失败", QMessageBox::Ok);
        return;
    }
    else
    {
        CloseHandle(stProcessInfo.hProcess);
    }
}
