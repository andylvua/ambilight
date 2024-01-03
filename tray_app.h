//
// Created by andrew on 12/26/23.
//

#ifndef AMBILIGHT_TRAY_APP_H
#define AMBILIGHT_TRAY_APP_H

#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMenu>
#include <QtWidgets/QSlider>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QIcon>
#include <QMessageBox>
#include <QComboBox>

#include <csignal>

#include "capturer.h"


class TrayApp : public QObject {
Q_OBJECT

public:
    static TrayApp *instance;

public:
    explicit TrayApp(AppSettings &settings) : settings(settings) {
        setupSignalHandlers();

        QApplication::setQuitOnLastWindowClosed(false);
    }

    void createTrayIcon() {
        trayIcon = new QSystemTrayIcon(QIcon("../ambilight.png"), this);
        trayIcon->setToolTip("Ambilight");

        auto *trayMenu = new QMenu();

        startAction = new QAction("Start", this);
        connect(startAction, &QAction::triggered, this, [this]() {
            startAction->setVisible(false);
            stopAction->setVisible(true);
        });
        connect(startAction, &QAction::triggered, this, &TrayApp::startCapturer);
        trayMenu->addAction(startAction);
        startAction->setVisible(false);

        stopAction = new QAction("Stop", this);
        connect(stopAction, &QAction::triggered, this, [this]() {
            startAction->setVisible(true);
            stopAction->setVisible(false);
        });
        connect(stopAction, SIGNAL(triggered()), this, SIGNAL(stopCapturer()));
        trayMenu->addAction(stopAction);

        auto *quitAction = new QAction("Quit", this);
        connect(quitAction, &QAction::triggered, this, &TrayApp::quit);
        trayMenu->addAction(quitAction);

        trayMenu->addSeparator();

        auto *settingsAction = new QAction("Settings", this);
        connect(settingsAction, &QAction::triggered, settingsWindow, &QWidget::show);
        trayMenu->addAction(settingsAction);

        trayIcon->setContextMenu(trayMenu);
        trayIcon->show();
    }

    void createSettingsWindow() {
        settingsWindow = new QWidget();
        settingsWindow->setWindowTitle("Settings");
        settingsWindow->setFixedSize(400, 300);

        auto *layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        settingsWindow->setLayout(layout);

        QFont labelFont;
        labelFont.setBold(true);
        labelFont.setPointSize(12);


        auto *brightnessLabel = new QLabel("Brightness");
        brightnessLabel->setFont(labelFont);

        layout->addWidget(brightnessLabel);

        auto *brightnessLayout = new QHBoxLayout();
        layout->addLayout(brightnessLayout);

        auto *brightnessSlider = new QSlider(Qt::Horizontal);
        brightnessSlider->setRange(0, 100);
        brightnessSlider->setValue(100);
        brightnessLayout->addWidget(brightnessSlider);
        connect(brightnessSlider, &QSlider::valueChanged, this, [this](int value) {
            settings.brightness = value;
            Q_EMIT brightnessChanged();
        });

        auto *brightnessValue = new QLabel("100%");
        brightnessLayout->addWidget(brightnessValue);
        connect(brightnessSlider, &QSlider::valueChanged, [brightnessValue](int value) {
            brightnessValue->setText(QString("%1%").arg(value));
        });

        auto *line = new QFrame;
        line->setFixedHeight(15);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);

        auto *staticColorLabel = new QLabel("Static color");
        staticColorLabel->setFont(labelFont);
        layout->addWidget(staticColorLabel);

        auto *staticColorSwitcherLayout = new QHBoxLayout();
        layout->addLayout(staticColorSwitcherLayout);

        staticColorSwitcher = new QCheckBox("Enable");
        staticColorSwitcher->setChecked(false);
        staticColorSwitcherLayout->addWidget(staticColorSwitcher);

        staticColorBox = new QPushButton();
        staticColorBox->setFixedSize(50, 25);
        staticColorBox->setStyleSheet("background-color: rgb(0, 0, 0);");

        staticColorSwitcherLayout->addWidget(staticColorBox);

        connect(staticColorSwitcher, &QCheckBox::stateChanged, [this](int state) {
            if (state == Qt::Checked) {
                settings.enableStaticColor = true;
                Q_EMIT staticColorSwitched();
            } else {
                settings.enableStaticColor = false;
                Q_EMIT staticColorSwitched();
            }
        });

        connect(staticColorBox, &QPushButton::clicked, this, [this]() {
            auto color = QColorDialog::getColor(Qt::white, settingsWindow);
            if (color.isValid()) {
                staticColorBox->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
                                                      .arg(color.red())
                                                      .arg(color.green())
                                                      .arg(color.blue()));

                settings.staticColor = color;

                if (settings.enableStaticColor) {
                    Q_EMIT staticColorChanged();
                }
            }
        }, Qt::QueuedConnection);

        auto *line2 = new QFrame;
        line2->setFixedHeight(15);
        line2->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line2);

        auto aspectRatioLabel = new QLabel("Aspect ratio");
        aspectRatioLabel->setFont(labelFont);
        layout->addWidget(aspectRatioLabel);

        auto *aspectRatioDropdown = new QComboBox();
        aspectRatioDropdown->addItem("Standard");
        aspectRatioDropdown->addItem("Wide 21:9");
        aspectRatioDropdown->addItem("Wide 24:10");
        aspectRatioDropdown->addItem("Auto");
        layout->addWidget(aspectRatioDropdown);

        connect(aspectRatioDropdown, &QComboBox::currentTextChanged, this, [this](const QString &text) {
            if (text == "Standard") {
                settings.aspectRatio.setAspectRatio(STANDARD);
            } else if (text == "Wide 21:9") {
                settings.aspectRatio.setAspectRatio(WIDE_21_9);
            } else if (text == "Wide 24:10") {
                settings.aspectRatio.setAspectRatio(WIDE_24_10);
            } else if (text == "Auto") {
                settings.aspectRatio.setAspectRatio(AUTO);
            }
            Q_EMIT aspectRatioChanged();
        });

        settingsWindow->setAttribute(Qt::WA_QuitOnClose, false);
        settingsWindow->show();
    }

    void sigtermHandler(int) {
        quit();
    }

    static void sigtermHandlerStatic(int sig) {
        instance->sigtermHandler(sig);
    }

    static void setupSignalHandlers() {
//        struct sigaction sigIntHandler{};
//        sigIntHandler.sa_handler = sigtermHandlerStatic;
//
//        sigaction(SIGTERM, &sigIntHandler, nullptr);
    }
Q_SIGNALS:

    void startCapturer();

    void stopCapturer();

    void staticColorChanged();

    void staticColorSwitched();

    void brightnessChanged();

    void aspectRatioChanged();

public Q_SLOTS:

    void start() {
        qDebug() << "TrayApp is running in thread " << QThread::currentThreadId();
        createSettingsWindow();
        createTrayIcon();
    }

    void quit() {
        qDebug() << "Executing quit() in thread " << QThread::currentThreadId();
        auto *msgBox = new QMessageBox();
        msgBox->setWindowTitle("Quit");
        msgBox->setText("Close the application?");
        msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        if (msgBox->exec() == QMessageBox::Yes) {
            Q_EMIT stopCapturer();
            QThread::sleep(1);
            QApplication::quit();
        }
    }

private:
    AppSettings &settings;

    QAction *startAction;
    QAction *stopAction;

    QWidget *settingsWindow;

    QSystemTrayIcon *trayIcon;
    QPushButton *staticColorBox;
    QCheckBox *staticColorSwitcher;
};

#endif //AMBILIGHT_TRAY_APP_H
