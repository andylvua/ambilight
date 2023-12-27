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


#include "capturer.h"


class TrayApp : public QObject {

public:
    explicit TrayApp(Capturer &capturer) : capturer(capturer) {
        createSettingsWindow();
        createTrayIcon();
    }

    void createTrayIcon() {
        trayIcon = new QSystemTrayIcon(QIcon("../ambilight.png"), this);
        trayIcon->setToolTip("Ambilight");

        auto *trayMenu = new QMenu();

        startAction = new QAction("Start", this);
        connect(startAction, &QAction::triggered, this, &TrayApp::startCapturer);
        trayMenu->addAction(startAction);
        startAction->setVisible(false);

        stopAction = new QAction("Stop", this);
        connect(stopAction, &QAction::triggered, this, &TrayApp::stopCapturer);
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

        // brightness
        auto *brightnessLayout = new QHBoxLayout();
        layout->addLayout(brightnessLayout);

        auto *brightnessSlider = new QSlider(Qt::Horizontal);
        brightnessSlider->setRange(0, 100);
        brightnessSlider->setValue(100);
        brightnessLayout->addWidget(brightnessSlider);
        connect(brightnessSlider, &QSlider::valueChanged, this, &TrayApp::setBrightness);

        // value, next to the slider in format "value %"
        auto *brightnessValue = new QLabel("100%");
        brightnessLayout->addWidget(brightnessValue);
        connect(brightnessSlider, &QSlider::valueChanged, [brightnessValue](int value) {
            brightnessValue->setText(QString("%1%").arg(value));
        });

        // separator
        auto *line = new QFrame;
        line->setFixedHeight(15);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);


        auto *staticColorLabel = new QLabel("Static color");
        staticColorLabel->setFont(labelFont);
        layout->addWidget(staticColorLabel);

        // add switcher for static color, should be to the right of the label
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
                capturer.setStaticColor(true, staticColor);
            } else {
                capturer.setStaticColor(false);
            }
        });

        // when clicked on the static color box, open color picker and set the color
        connect(staticColorBox, &QPushButton::clicked, [this]() {
            staticColor = QColorDialog::getColor(Qt::white, settingsWindow);
            if (staticColor.isValid()) {
                staticColorBox->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
                                                      .arg(staticColor.red())
                                                      .arg(staticColor.green())
                                                      .arg(staticColor.blue()));

                if (staticColorSwitcher->isChecked()) {
                    capturer.setStaticColor(staticColor);
                }
            }
        });

        // make the cross button close the window, not the whole app
        settingsWindow->setAttribute(Qt::WA_QuitOnClose, false);

        settingsWindow->show();
    }

public slots:

    void startCapturer() {
        capturer.restart();
        startAction->setVisible(false);
        stopAction->setVisible(true);
    }

    void stopCapturer() {
        capturer.stop();
        stopAction->setVisible(false);
        startAction->setVisible(true);
    }

    void setBrightness(int value) {
        capturer.setBrightness(value);
    }

    static void quit() {
        auto *msgBox = new QMessageBox();
        msgBox->setWindowTitle("Quit");
        msgBox->setText("Close the application?");
        msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox->setDefaultButton(QMessageBox::No);
        if (msgBox->exec() == QMessageBox::Yes) {
            QApplication::quit();
        }
    }

private:
    QAction *startAction;
    QAction *stopAction;

    QWidget *settingsWindow;

    QSystemTrayIcon *trayIcon;
    QPushButton *staticColorBox;
    QColor staticColor;
    QCheckBox *staticColorSwitcher;

    Capturer &capturer;
};

#endif //AMBILIGHT_TRAY_APP_H
