#ifndef APKTOOL_H
#define APKTOOL_H

#include "base/command.h"

namespace Apktool
{
    class Decode : public Command
    {
    public:
        Decode(const QString &source, const QString &destination, const QString &frameworks,
               bool resources, bool sources, bool noDebugInfo, bool onlyMainClasses, bool keepBrokenResources,
               QObject *parent = nullptr)
            : Command(parent)
            , source(source)
            , destination(destination)
            , frameworks(frameworks)
            , resources(resources)
            , sources(sources)
            , noDebugInfo(noDebugInfo)
            , onlyMainClasses(onlyMainClasses)
            , keepBrokenResources(keepBrokenResources) {}

        void run() override;
        const QString &output() const;

    private:
        const QString source;
        const QString destination;
        const QString frameworks;
        const bool resources;
        const bool sources;
        const bool noDebugInfo;
        const bool onlyMainClasses;
        const bool keepBrokenResources;
        QString resultOutput;
    };

    class Build : public Command
    {
    public:
        Build(const QString &source, const QString &destination,
              const QString &frameworks, bool aapt1, bool debuggable, QObject *parent = nullptr)
            : Command(parent)
            , source(source)
            , destination(destination)
            , frameworks(frameworks)
            , aapt1(aapt1)
            , debuggable(debuggable)
        {}

        void run() override;
        const QString &output() const;

    private:
        const QString source;
        const QString destination;
        const QString frameworks;
        const bool aapt1;
        const bool debuggable;
        QString resultOutput;
    };

    class InstallFramework : public Command
    {
    public:
        InstallFramework(const QString &source, const QString &destination, QObject *parent = nullptr)
            : Command(parent)
            , source(source)
            , destination(destination)
        {}

        void run() override;
        const QString &output() const;

    private:
        const QString source;
        const QString destination;
        QString resultOutput;
    };

    class Version : public Command
    {
    public:
        Version(QObject *parent = nullptr) : Command(parent) {}
        void run() override;
        const QString &version() const;

    private:
        QString resultVersion;
    };

    void reset();
    QString getPath();
    QString getDefaultPath();
    QString getOutputPath();
    QString getDefaultOutputPath();
    QString getFrameworksPath();
    QString getDefaultFrameworksPath();
}

#endif // APKTOOL_H
