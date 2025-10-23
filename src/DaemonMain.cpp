#include "AppImageManager/AppImageManager.h"
#include "AppImageManager/AutostartDaemon.h"

#include <QCoreApplication>
#include <QTimer>

using appimagemanager::AppImageManager;
using appimagemanager::AutostartDaemon;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    AppImageManager manager;
    AutostartDaemon daemon(manager);
    QTimer::singleShot(0, &daemon, &AutostartDaemon::start);
    return app.exec();
}