#include <QApplication>
#include <QLabel>
#include <QColor>
#include <QPainter>
#include <QTimer>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QDateTime>

#include <vector>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>


struct CapturerConfig {
    int ledX; // number of leds on the top and bottom of the screen
    int ledY; // number of leds on the left and right of the screen
    int screenWidth; // width of the screen
    int screenHeight; // height of the screen
    int zoneOffsetX; // the width of the zone where average color is calculated (in pixels, from the left and right)
    int zoneOffsetY; // the height of the zone where average color is calculated (in pixels, from the top and bottom)
    int captureFPS; // the number of frames per second to capture
    int targetFPS; // the number of frames per second to render
    int convolveSize; // the size of the convolution window
    int pixelsPerZone; // the number of pixels to sample per zone
};

struct Color {
    int red;
    int green;
    int blue;

    Color(int red, int green, int blue) {
        this->red = red;
        this->green = green;
        this->blue = blue;
    }

    Color() = default;

    Color operator+(const Color &other) const {
        return Color{
                red + other.red,
                green + other.green,
                blue + other.blue,
        };
    }

    Color operator-(const Color &other) const {
        return Color{
                red - other.red,
                green - other.green,
                blue - other.blue,
        };
    }

    Color operator*(const double coef) const {
        return Color{
                static_cast<int>(red * coef),
                static_cast<int>(green * coef),
                static_cast<int>(blue * coef),
        };
    }
};

class Displayer : public QWidget {

public:
    static Displayer *getInstance() {
        if (singleton == nullptr) {
            singleton = new Displayer();
        }
        return singleton;
    }

    void display(const QVector<Color> &_colors) {
        this->colors = _colors;
        repaint();
    }

    void setConfig(CapturerConfig &_config) {
        this->config = _config;
    }


    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        int width = this->width();
        int height = this->height();
        double scaleX = static_cast<double>(config.screenWidth) / width;
        double scaleY = static_cast<double>(config.screenHeight) / height;
        int xLedWidth = width / (config.ledX + 2);
        int yLedHeight = height / (config.ledY + 2);
        int xLedHeight =
                config.zoneOffsetY == 0 ? yLedHeight : yLedHeight + static_cast<int>(config.zoneOffsetY / scaleY);
        int yLedWidth =
                config.zoneOffsetX == 0 ? xLedWidth : xLedWidth + static_cast<int>(config.zoneOffsetX / scaleX);
        int ledX = config.ledX;
        int ledY = config.ledY;
        int ledNumber = 0;

        auto drawLed = [&](int x, int y, int width, int height) {
            auto color = colors[ledNumber];
            painter.fillRect(x, y, width, height, QColor(color.red, color.green, color.blue));
            painter.drawRect(x, y, width, height);
            painter.drawText(x, y, width, height, Qt::AlignCenter, QString::number(++ledNumber));
        };

        for (int i = 0; i < ledX; i++) {
            auto x = (i + 1) * xLedWidth;
            auto y = height - xLedHeight;
            drawLed(x, y, xLedWidth, xLedHeight);
        }
        for (int i = ledY - 1; i >= 0; i--) {
            auto x = width - yLedWidth;
            auto y = (i + 1) * yLedHeight;
            drawLed(x, y, yLedWidth, yLedHeight);
        }
        for (int i = ledX - 1; i >= 0; i--) {
            auto x = (i + 1) * xLedWidth;
            auto y = 0;
            drawLed(x, y, xLedWidth, xLedHeight);
        }
        for (int i = 0; i < ledY; i++) {
            auto x = 0;
            auto y = (i + 1) * yLedHeight;
            drawLed(x, y, yLedWidth, yLedHeight);
        }
    }

private:
    static Displayer *singleton;
    CapturerConfig config{};
    QVector<Color> colors;

    Displayer() = default;
};

class Capturer : public QObject {

public:
    explicit Capturer(CapturerConfig &config, Display *display, Window root) {
        this->config = config;
        this->display = display;
        this->root = root;

        initZoneProperties();

        previousColors = QVector<Color>(config.ledX * 2 + config.ledY * 2);
        colors = QVector<Color>(config.ledX * 2 + config.ledY * 2);

        QThreadPool::globalInstance()->setMaxThreadCount(4);
    }

    void initZoneProperties() {
        zoneConfig.widthY = zoneConfig.widthX = config.screenWidth / (config.ledX + 2);
        zoneConfig.widthY += config.zoneOffsetX;
        zoneConfig.heightX = zoneConfig.heightY = config.screenHeight / (config.ledY + 2);
        zoneConfig.heightX += config.zoneOffsetY;

        zoneConfig.rowOffset = (config.screenWidth % (config.ledX + 2)) / 2;
        zoneConfig.colOffset = (config.screenHeight % (config.ledY + 2)) / 2;
    }

    Color getAverageColor(XImage *image, int x, int y, int width, int height) const {
        int red = 0;
        int green = 0;
        int blue = 0;

        int counter = 0;
        int skip = width * height / config.pixelsPerZone;
        for (int i = 0; i < width * height; i += skip) {
            int pixel = XGetPixel(image, x + i % width, y + i / width);
            red += pixel >> 16 & 0xFF;
            green += pixel >> 8 & 0xFF;
            blue += pixel & 0xFF;
            counter++;
        }
        red /= counter;
        green /= counter;
        blue /= counter;
        return {red, green, blue};
    }

    [[nodiscard]] QVector<Color> getColors() const {
        QVector<Color> res;
        res.reserve(config.ledX * 2 + config.ledY * 2);

        for (int i = 0; i < config.ledX; i++) {
            auto color = getAverageColor(screenImage.bottomPanel,
                                         (i + 1) * zoneConfig.widthX + zoneConfig.rowOffset,
                                         0,
                                         zoneConfig.widthX,
                                         zoneConfig.heightX);
            res.push_back(color);
        }
        for (int i = config.ledY - 1; i >= 0; i--) {
            auto color = getAverageColor(screenImage.rightPanel,
                                         0,
                                         (i + 1) * zoneConfig.heightY + zoneConfig.colOffset,
                                         zoneConfig.widthY,
                                         zoneConfig.heightY);
            res.push_back(color);
        }
        for (int i = config.ledX - 1; i >= 0; i--) {
            auto color = getAverageColor(screenImage.topPanel,
                                         (i + 1) * zoneConfig.widthX + zoneConfig.rowOffset,
                                         0,
                                         zoneConfig.widthX,
                                         zoneConfig.heightX);
            res.push_back(color);
        }
        for (int i = 0; i < config.ledY; i++) {
            auto color = getAverageColor(screenImage.leftPanel,
                                         0,
                                         (i + 1) * zoneConfig.heightY + zoneConfig.colOffset,
                                         zoneConfig.widthY,
                                         zoneConfig.heightY);
            res.push_back(color);
        }

        return res;
    }

    void interpolateColors() {
        QVector<Color> interpolatedColors;
        interpolatedColors.resize(config.ledX * 2 + config.ledY * 2);

        size_t totalElapsed = 0;
        static auto startTimestamp = QDateTime::currentDateTime();

        auto totalFramesToInterpolate = config.targetFPS - config.captureFPS;
        auto framesToInterpolate = totalFramesToInterpolate / config.captureFPS;
        auto framesToRender = framesToInterpolate + 1;
        auto captureTimeMs = 1000 / config.captureFPS;
        auto frameDuration = captureTimeMs / framesToRender;

        QVector<Color> initColors = colors;

        for (auto i = 0; i < framesToRender; i++) {
            for (auto j = 0; j < colors.size(); j++) {
                double interpolationRate = static_cast<double>(i) / framesToInterpolate;
                auto dColor = initColors[j] - previousColors[j];
                auto interpolatedColor = previousColors[j] + dColor * interpolationRate;
                interpolatedColors[j] = interpolatedColor;
            }

            auto finish = QDateTime::currentDateTime();
            auto elapsed = startTimestamp.msecsTo(finish);
            totalElapsed += elapsed;

            colors = interpolatedColors;

            Displayer::getInstance()->display(interpolatedColors);

            startTimestamp = QDateTime::currentDateTime();

            if (totalElapsed >= frameDuration * framesToRender) {
                break;
            } else if (i != framesToInterpolate) {
                QThread::msleep(frameDuration - 3);
            }
        }
    }

    QVector<Color> convolveColors(QVector<Color> &&_colors) const {
        QVector<Color> convolvedColors;
        convolvedColors.resize(_colors.size());

        for (auto i = 0; i < _colors.size(); i++) {
            Color convolvedColor = {0, 0, 0};
            for (auto j = 0; j < config.convolveSize; j++) {
                auto index = i + j - config.convolveSize / 2;
                if (index < 0) {
                    index = 0;
                } else if (index >= _colors.size()) {
                    index = _colors.size() - 1;
                }
                convolvedColor = convolvedColor + _colors[index];
            }
            convolvedColors[i] = convolvedColor * (1.0 / config.convolveSize);
        }

        return convolvedColors;
    }

    void start() {
        captureTimer = new QTimer(this);

        connect(captureTimer, &QTimer::timeout, this, &Capturer::capture);
        captureTimer->start(1000 / config.captureFPS);

        capture();
    }

    void captureImages() {
        auto leftPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, 0, zoneConfig.heightY, zoneConfig.widthY,
                             config.screenHeight - zoneConfig.heightY, AllPlanes, ZPixmap);
        });
        auto rightPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, config.screenWidth - zoneConfig.widthY, zoneConfig.heightY,
                             zoneConfig.widthY, config.screenHeight - zoneConfig.heightY, AllPlanes, ZPixmap);
        });
        auto topPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, zoneConfig.widthX, 0, config.screenWidth - zoneConfig.widthX,
                             zoneConfig.heightX, AllPlanes, ZPixmap);
        });
        auto bottomPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, zoneConfig.widthX, config.screenHeight - zoneConfig.heightX,
                             config.screenWidth - zoneConfig.widthX, zoneConfig.heightX, AllPlanes, ZPixmap);
        });

        screenImage.leftPanel = leftPanelFuture.result();
        screenImage.rightPanel = rightPanelFuture.result();
        screenImage.topPanel = topPanelFuture.result();
        screenImage.bottomPanel = bottomPanelFuture.result();
    }

    void destroyImages() const {
        if (screenImage.leftPanel != nullptr) {
            XDestroyImage(screenImage.leftPanel);
            XDestroyImage(screenImage.rightPanel);
            XDestroyImage(screenImage.topPanel);
            XDestroyImage(screenImage.bottomPanel);
        }
    }

private slots:

    void capture() {
        destroyImages();
        captureImages();

        previousColors = colors;

        colors = convolveColors(getColors());
        interpolateColors();
    }

private:
    CapturerConfig config{};

    Display *display = nullptr;
    Window root = 0;

    struct ScreenImage {
        XImage *leftPanel = nullptr;
        XImage *rightPanel = nullptr;
        XImage *topPanel = nullptr;
        XImage *bottomPanel = nullptr;
    } screenImage;

    struct ZoneConfig {
        int widthX = 0;
        int heightY = 0;
        int widthY = 0;
        int heightX = 0;
        int rowOffset = 0;
        int colOffset = 0;
    } zoneConfig;

    QTimer *captureTimer = nullptr;

    QVector<Color> previousColors;
    QVector<Color> colors;
};

Displayer *Displayer::singleton = nullptr;

int main(int argc, char *argv[]) {
    Display *display = XOpenDisplay(nullptr);
    Window root = DefaultRootWindow(display);

    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);
    int width = gwa.width;
    int height = gwa.height;

    QApplication app(argc, argv);
    QSettings settings("../config.ini", QSettings::IniFormat);

    CapturerConfig config = {
            .ledX = settings.value("ledX", 31).toInt(),
            .ledY = settings.value("ledY", 18).toInt(),
            .screenWidth = width,
            .screenHeight = height,
            .zoneOffsetX = settings.value("zoneOffsetX", 200).toInt(),
            .zoneOffsetY = settings.value("zoneOffsetY", 80).toInt(),
            .captureFPS = settings.value("captureFPS", 3).toInt(),
            .targetFPS = settings.value("targetFPS", 60).toInt(),
            .convolveSize = settings.value("convolveSize", 5).toInt(),
            .pixelsPerZone = settings.value("pixelsPerZone", 70).toInt(),
    };

    Capturer capturer(config, display, root);
    Displayer::getInstance()->setConfig(config);

    Displayer::getInstance()->show();
    capturer.start();

    return QApplication::exec();
}
