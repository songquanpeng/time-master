#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <cmath>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    isWorking = false;
    isDoingTask = false;
    isTimerPaused = true;
    minuteTimer = startTimer(60 * 1000);
    minuteCounter = 0;

    connect(ui->workTimeSlider, SIGNAL(valueChange(int)), this, SLOT(on_workTimeSlider_valueChanged(int)));
    connect(ui->breakTimeSlider, SIGNAL(valueChange(int)), this, SLOT(on_breakTimeSlider_valueChanged(int)));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::timerEvent(QTimerEvent *event) {
    if (isWorking) {
        if (event->timerId() == minuteTimer) timeLapse();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    hide();
}

void MainWindow::on_startReminderBtn_clicked() {
    ui->startReminderBtn->setText(isTimerPaused ? "Pause" : "Start");
    isTimerPaused = !isTimerPaused;
}

void MainWindow::on_stopReminderBtn_clicked() {
    isTimerPaused = true;
    ui->startReminderBtn->setText("Start");
    minuteCounter = 0;
    isWorking = true;
}

void MainWindow::on_workTimeSlider_valueChanged(int val) {
    workTimeLength = val;
    ui->workTimeLabel->setText(QString::number(workTimeLength) + " min");
}

void MainWindow::on_breakTimeSlider_valueChanged(int val) {
    breakTimeLength = val;
    ui->breakTimeLabel->setText(QString::number(breakTimeLength) + " sec");
}

void MainWindow::timeLapse() {
    int timeLength = isWorking ? workTimeLength : breakTimeLength;
    ui->timeProgressBar->setValue(ceil(100 * minuteCounter / timeLength));
    if (minuteCounter >= timeLength) {
        minuteCounter = 0;
        remindUser(isWorking ? "Time to take a break." : "Time to move on.");
        isWorking = !isWorking;
    }
}

void MainWindow::remindUser(QString prompt) {
    trayIcon->showMessage("Hey", prompt);
}

void MainWindow::createActions() {
    controlAction = new QAction(tr("&Control"), this);
    connect(controlAction, &QAction::triggered, this, &MainWindow::on_startReminderBtn_clicked);

    stopAction = new QAction(tr("&Stop"), this);
    connect(stopAction, &QAction::triggered, this, &MainWindow::on_stopReminderBtn_clicked);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::createTrayIcon() {
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(controlAction);
    trayIconMenu->addAction(stopAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
    trayIcon->setToolTip("Rest reminder");
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/image/icon"));
    trayIcon->setVisible(true);
    trayIcon->show();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Unknown:
        break;
    case QSystemTrayIcon::Context:
        break;
    case QSystemTrayIcon::DoubleClick:
        QWidget::showNormal();
        break;
    case QSystemTrayIcon::Trigger:
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        break;
    }
}

