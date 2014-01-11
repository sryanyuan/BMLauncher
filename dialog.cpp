#include "dialog.h"
#include "ui_dialog.h"
#include <QDebug>
#include <QPainter>
#include "SimpleIni.h"
#include <QDir>
#include <string>
//#include <io.h>
#include <QProcess>
#include <QMessageBox>

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

    //if(0 != access(szCurAppPath, 0))
    QFile xCfgFile(szCurAppPath);
    if(!xCfgFile.exists())
    {
        //  Break
        return;
    }

    CSimpleIniA xIniFile;
    if(SI_OK == xIniFile.LoadFile(szCurAppPath))
    {
        int nMultiMode = xIniFile.GetLongValue("LOGIN",
                              "MULTI");
        if(nMultiMode != 0)
        {
            ui->m_pSelectGroup->setChecked(true);
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

    //if(0 != access(szCurAppPath, 0))
    QFile xFile(szCurAppPath);
    if(!xFile.exists())
    {
        //  Create
        xFile.open(QFile::ReadWrite);
        xFile.close();
    }

    CSimpleIniA xIniFile;
    if(SI_OK == xIniFile.LoadFile(szCurAppPath))
    {
        int nMultiMode = ui->m_pSelectGroup->isChecked() ? 1 : 0;
        xIniFile.SetLongValue("LOGIN",
                              "MULTI",
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
        xIniFile.SaveFile(szCurAppPath);
    }
}

void Dialog::slot_VerifyIpInput()
{
    QObject* pSender = sender();

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
    SaveConfigToFile();
    qApp->quit();
}

void Dialog::on_m_pBtnMin_clicked()
{
    showMinimized();
}

bool Dialog::LaunchClient()
{
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
        strcpy(szInvokeParam, "svrip=127.0.0.1:8300");
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
    if(!LaunchServer())
    {
        QMessageBox::warning(this, "错误", "无法启动服务器，请检查是否文件丢失或者被安全软件禁止启动", QMessageBox::Ok);
    }
    if(!LaunchClient())
    {
        QMessageBox::warning(this, "错误", "无法启动客户端，请检查是否文件丢失或者被安全软件禁止启动", QMessageBox::Ok);
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
        QMessageBox::critical(this, "错误", "无法启动服务器，请检查是否文件丢失或者被安全软件禁止启动", QMessageBox::Ok);
    }
}

void Dialog::on_m_pBtnClient_clicked()
{
    if(!LaunchClient())
    {
        QMessageBox::critical(this, "错误", "无法启动客户端，请检查是否文件丢失或者被安全软件禁止启动", QMessageBox::Ok);
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
