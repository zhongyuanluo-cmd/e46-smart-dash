// Qt 6 eglfs Hello World - renders a red rectangle
// Cross-compile for Core3566 aarch64, deploy via scp
// Run: ./qt-hello -platform eglfs

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    // Use eglfs platform plugin for direct DRM/KMS rendering
    qputenv("QT_QPA_PLATFORM", "eglfs");
    // Disable cursor (embedded touchscreen)
    qputenv("QT_QPA_EGLFS_HIDECURSOR", "1");

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
