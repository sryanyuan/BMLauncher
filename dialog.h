#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QPoint>
#include <QPixmap>
#include <QLineEdit>
#include <QProcess>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

protected:
    void mouseMoveEvent(QMouseEvent* _e);
    void mousePressEvent(QMouseEvent* _e);
    void mouseReleaseEvent(QMouseEvent* _e);
    void paintEvent(QPaintEvent *);

private:
    void SaveConfigToFile();
    void LoadConfigToApp();
    bool CheckInputIpValid(QLineEdit* _pEdit);
    bool GetInputIp(QLineEdit* _pEdit, int* _pOutArray);
    bool LaunchServer();
    bool LaunchClient();

private slots:

    void on_m_pBtnClose_clicked();

    void on_m_pBtnMin_clicked();

    void on_m_pBtnStart_clicked();

    void slot_VerifyIpInput();

    void on_m_pBtnServer_clicked();

    void on_m_pBtnClient_clicked();

    void slot_appServer_close(int _nCode);
    void slot_appClient_close(int _nCode);

private:
    const char* GetRootPath();

private:
    Ui::Dialog *ui;
    QPoint m_xPressedPos;
    bool m_bLeftPressed;
    QPixmap m_xBkImg;
    QProcess* m_pAppServer;
    QProcess* m_pAppClient;
};

#endif // DIALOG_H
