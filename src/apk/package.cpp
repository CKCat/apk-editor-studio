#include "apk/package.h"
#include "apk/apkcloner.h"
#include "base/application.h"
#include "base/settings.h"
#include "base/utils.h"
#include "tools/adb.h"
#include "tools/apktool.h"
#include "tools/apksigner.h"
#include "tools/keystore.h"
#include "tools/zipalign.h"
#include <QFutureWatcher>
#include <QUuid>
#include <QDebug>

Package::Package(const QString &path)
{
    originalPath = QFileInfo(path).absoluteFilePath();
    manifest = nullptr;
    filesystemModel.setSourceModel(&resourcesModel);
    iconsProxy.setSourceModel(&resourcesModel);
    logModel.setExclusiveLoading(true);
    connect(&state, &PackageState::changed, this, &Package::stateUpdated);
}

Package::~Package()
{
    delete manifest;

    if (!contentsPath.isEmpty()) {
        qDebug() << qPrintable(QString("Removing \"%1\"...\n").arg(contentsPath));
        // Additional check to prevent accidental recursive deletion of the wrong directory:
        const bool recursive = QFile::exists(QString("%1/%2").arg(contentsPath, "AndroidManifest.xml"));
        Utils::rmdir(contentsPath, recursive);
    }
}

QString Package::getTitle() const
{
    return QFileInfo(originalPath).fileName();
}

QString Package::getOriginalPath() const
{
    return QDir::toNativeSeparators(originalPath);
}

QString Package::getContentsPath() const
{
    return QDir::toNativeSeparators(contentsPath);
}

QString Package::getPackageName() const
{
    return manifest->getPackageName();
}

QIcon Package::getThumbnail() const
{
    QIcon thumbnail = iconsProxy.getIcon();
    return !thumbnail.isNull() ? thumbnail : QIcon::fromTheme("apk-editor-studio");
}

const PackageState &Package::getState() const
{
    return state;
}

bool Package::hasSourcesUnpacked() const
{
    return withSources;
}

void Package::setApplicationIcon(const QString &path, QWidget *parent)
{
    iconsProxy.replaceApplicationIcons(path, parent);
}

void Package::setPackageName(const QString &packageName)
{
    if (!withSources) {
        qWarning() << "Warning: Changing the package name requires the sources to be decompiled";
        return;
    }

    auto cloner = new ApkCloner(getContentsPath(), getPackageName(), packageName, this);
    connect(cloner, &ApkCloner::started, this, &Package::cloningStarted);
    connect(cloner, &ApkCloner::progressed, this, &Package::cloningProgressed);
    connect(cloner, &ApkCloner::finished, this, &Package::cloningFinished);
    connect(cloner, &ApkCloner::finished, this, [=](bool success) {
        if (success) {
            state.setModified(true);
            manifest->setPackageName(packageName);
        }
        cloner->deleteLater();
    });
    cloner->start();

}

Commands *Package::createCommandChain()
{
    auto command = new Commands(this);
    connect(command, &Commands::started, &logModel, &LogModel::clear);
    connect(command, &Commands::finished, this, [this](bool success) {
        if (success) {
            logModel.add(Package::tr("Done."), LogEntry::Success);
            state.setCurrentStatus(PackageState::Status::Normal);
        } else {
            state.setCurrentStatus(PackageState::Status::Errored);
        }
        app->settings->addRecentApk(this);
    });
    return command;
}

Command *Package::createUnpackCommand()
{
    QString target;
    do {
        const QString uuid = QUuid::createUuid().toString();
        target = QDir::toNativeSeparators(QString("%1/%2").arg(Apktool::getOutputPath(), uuid));
    } while (target.isEmpty() || QDir(target).exists());

    const QString source(getOriginalPath());
    const QString frameworks = Apktool::getFrameworksPath();

    withResources = true;
    withSources = app->settings->getDecompileSources();
    withBrokenResources = app->settings->getKeepBrokenResources();
    withNoDebugInfo = app->settings->getDecompileNoDebugInfo();
    withOnlyMainClasses = app->settings->getDecompileOnlyMainClasses();

    QDir().mkpath(target);
    QDir().mkpath(frameworks);

    contentsPath = target;
    Q_ASSERT(!contentsPath.isEmpty());

    auto apktoolDecode = new Apktool::Decode(source, target, frameworks, withResources, withSources, withNoDebugInfo, withOnlyMainClasses, withBrokenResources);
    connect(apktoolDecode, &Command::finished, this, [=](bool success) {
        if (success) {
            filesystemModel.setRootPath(getContentsPath());
        } else {
            logModel.add(tr("Error unpacking APK."), apktoolDecode->output(), LogEntry::Error);
        }
    });

    auto command = new Commands(this);
    command->add(apktoolDecode, true);
    command->add(new LoadUnpackedCommand(this), true);
    connect(command, &Command::started, this, [=]() {
        qDebug() << qPrintable(QString("Unpacking\n  from: %1\n    to: %2\n").arg(source, target));
        logModel.add(tr("Unpacking APK..."));
        state.setCurrentStatus(PackageState::Status::Unpacking);
    });
    connect(command, &Command::finished, this, [=](bool success) {
        if (success) {
            connect(&resourcesModel, &ResourceItemsModel::dataChanged, this, [=]() {
                state.setModified(true);
            });
            connect(&filesystemModel, &QFileSystemModel::dataChanged, this, [=]() {
                state.setModified(true);
            });
            connect(&iconsProxy, &IconItemsModel::dataChanged, this, [=]() {
                state.setModified(true);
            });
            connect(&manifestModel, &ManifestModel::dataChanged, this,
                    [=](const QModelIndex &, const QModelIndex &, const QVector<int> &roles) {
                if (!(roles.count() == 1 && roles.contains(Qt::UserRole))) {
                    state.setModified(true);
                }
            });
        }
        state.setUnpacked(success);
    });
    return command;
}

Command *Package::createPackCommand(const QString &target)
{
    const QString source = getContentsPath();
    const QString frameworks = Apktool::getFrameworksPath();
    const bool aapt1 = app->settings->getUseAapt1();
    const bool debuggable = app->settings->getMakeDebuggable();

    auto apktoolBuild = new Apktool::Build(source, target, frameworks, aapt1, debuggable);

    connect(apktoolBuild, &Command::started, this, [=]() {
        qDebug() << qPrintable(QString("Packing\n  from: %1\n    to: %2\n").arg(source, target));
        logModel.add(tr("Packing APK..."));
        state.setCurrentStatus(PackageState::Status::Packing);
    });

    connect(apktoolBuild, &Command::finished, this, [=](bool success) {
        if (success) {
            originalPath = target;
            state.setModified(false);
        } else {
            logModel.add(tr("Error packing APK."), apktoolBuild->output(), LogEntry::Error);
        }
    });

    return apktoolBuild;
}

Command *Package::createZipalignCommand(const QString &apk)
{
    auto zipalign = new Zipalign::Align(apk.isEmpty() ? getOriginalPath() : apk);

    connect(zipalign, &Command::started, this, [=]() {
        logModel.add(tr("Optimizing APK..."));
        state.setCurrentStatus(PackageState::Status::Optimizing);
    });

    connect(zipalign, &Command::finished, this, [=](bool success) {
        if (!success) {
            logModel.add(tr("Error optimizing APK."), zipalign->output(), LogEntry::Error);
        }
    });

    return zipalign;
}

Command *Package::createSignCommand(const Keystore *keystore, const QString &apk)
{
    auto apksigner = new Apksigner::Sign(apk.isEmpty() ? getOriginalPath() : apk, keystore);

    connect(apksigner, &Command::started, this, [=]() {
        logModel.add(tr("Signing APK..."));
        state.setCurrentStatus(PackageState::Status::Signing);
    });

    connect(apksigner, &Command::finished, this, [=](bool success) {
        if (!success) {
            logModel.add(tr("Error signing APK."), apksigner->output(), LogEntry::Error);
        }
    });

    return apksigner;
}

Command *Package::createInstallCommand(const QString &serial, const QString &apk)
{
    auto install = new Adb::Install(apk.isEmpty() ? getOriginalPath() : apk, serial);

    connect(install, &Command::started, this, [=]() {
        logModel.add(tr("Installing APK..."));
        state.setCurrentStatus(PackageState::Status::Installing);
    });

    connect(install, &Command::finished, this, [=](bool success) {
        if (!success) {
            logModel.add(tr("Error installing APK."), install->output(), LogEntry::Error);
        }
    });

    return install;
}

void Package::LoadUnpackedCommand::run()
{
    emit started();

    const QString contentsPath = package->getContentsPath();

    package->logModel.add(Package::tr("Reading APK contents..."));
    package->manifest = new Manifest(
        contentsPath + "/AndroidManifest.xml",
        contentsPath + "/apktool.yml");
    package->manifestModel.initialize(package->manifest);
    package->iconsProxy.setManifestScopes(package->manifest->scopes);

    auto initResourcesFuture = package->resourcesModel.initialize(contentsPath + "/res/");
    auto initResourcesFutureWatcher = new QFutureWatcher<void>(this);
    connect(initResourcesFutureWatcher, &QFutureWatcher<void>::finished, this, [=]() {
        emit finished(true);
    });
    initResourcesFutureWatcher->setFuture(initResourcesFuture);
}
