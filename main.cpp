#include <ctime>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <QApplication>
#include <QCoreApplication>
#include <QObject>
#include <QClipboard>
#include <QSettings>
#include <QDir>
#include <QString>
#include <QImage>
#include <QMimeData>

// 是否开机自启动
#define AUTO_START true

class ClipboardMonitor : public QObject {
    Q_OBJECT

public:
    explicit ClipboardMonitor(QObject *parent = nullptr)
        : QObject(parent) {
        _clipboard = QGuiApplication::clipboard();
        _savingPath = std::strcat(
            std::getenv("USERPROFILE"), "\\Pictures\\screenshots");
        std::filesystem::path p(_savingPath);
        if (!exists(p)) {
            std::filesystem::create_directory(p);
        }

        connect(_clipboard, &QClipboard::dataChanged,
                this, &ClipboardMonitor::readClipboard);
    }

    static void writeError(const std::string &message) {
        std::fstream file;
        file.open("errors.log", std::ios::app | std::ios::out);

        const time_t timestamp = std::time(nullptr);
        file << ctime(&timestamp) << message;
    }

    // 允许开机自启动(包含注册表操作)
    static void enableAutoStart(
        const QString &appName, const QString &appPath = QCoreApplication::applicationDirPath()) {
        QSettings settings(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)",
                       QSettings::NativeFormat);

        std::filesystem::path exePath(appPath.toStdString());
        exePath = exePath.make_preferred();
        exePath.append((appName + ".exe").toStdString());
        // qDebug() << exePath.string();
        settings.setValue(appName, ("\"" + exePath.string() + "\"").c_str());
    }

    // 取消开机自启动
    static void disableAutoStart(
        const QString &appName, const QString &appPath = QCoreApplication::applicationDirPath()) {
        QSettings settings(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)",
                       QSettings::NativeFormat);

        settings.remove(appName);
    }

public slots:
    void readClipboard() const {
        const QMimeData *mimeData = _clipboard->mimeData();
        if (mimeData->hasImage()) {
            QImage image = _clipboard->image();
            if (!image.isNull()) {
                std::filesystem::path basePath(_savingPath);
                basePath.append(std::to_string(std::time(nullptr)) + ".png");
                // qDebug() << "Saving image: " << basePath.string();

                if (!image.save(basePath.string().c_str(), "PNG", 80)) {
                    writeError("Failed to save image, check your saving path");
                }
            } else {
                writeError("invalid picture, maybe your picture has broken");
            }
        }
    }

private:
    QClipboard *_clipboard;
    std::string _savingPath;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ClipboardMonitor cm;
    if constexpr (AUTO_START)
        ClipboardMonitor::enableAutoStart("Screenshotting");
    else
        ClipboardMonitor::disableAutoStart("Screenshotting");
    return QApplication::exec();
}

#include "main.moc"