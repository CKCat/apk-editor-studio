#ifndef UTILS_H
#define UTILS_H

#include <QList>
#include <QPixmap>

class QFileInfo;

namespace Utils
{
    enum AndroidSDKLevel {
        ANDROID_3 = 3,
        ANDROID_4,
        ANDROID_5,
        ANDROID_6,
        ANDROID_7,
        ANDROID_8,
        ANDROID_9,
        ANDROID_10,
        ANDROID_11,
        ANDROID_12,
        ANDROID_13,
        ANDROID_14,
        ANDROID_15,
        ANDROID_16,
        ANDROID_17,
        ANDROID_18,
        ANDROID_19,
        ANDROID_20,
        ANDROID_21,
        ANDROID_22,
        ANDROID_23,
        ANDROID_24,
        ANDROID_25,
        ANDROID_26,
        ANDROID_27,
        ANDROID_28,
        ANDROID_29,
        ANDROID_30,
        ANDROID_31,
        ANDROID_32,
        ANDROID_33,
        ANDROID_34,
        ANDROID_35,
        ANDROID_SDK_COUNT
    };

    // String utils:

    QString capitalize(QString string);

    // Math utils:

    int roundToNearest(int number, QList<int> numbers);

    // Color utils:

    bool isDarkTheme();
    bool isLightTheme();

    // File / Directory utils:

    bool explore(const QString &path);
    void rmdir(const QString &path, bool recursive = false);
    bool copyFile(const QString &src, QWidget *parent = nullptr);
    bool copyFile(const QString &src, QString dst, QWidget *parent = nullptr);
    bool replaceFile(const QString &what, QWidget *parent = nullptr);
    bool replaceFile(const QString &what, QString with, QWidget *parent = nullptr);
    QString normalizePath(QString path);
    QString toAbsolutePath(const QString &path);

    // Image utils:

    bool isImageReadable(const QString &path);
    bool isImageWritable(const QString &path);
    QPixmap iconToPixmap(const QIcon &icon);

    // Application utils:

    QString getAppTitle();
    QString getAppVersion();
    QString getAppTitleSlug();
    QString getAppTitleAndVersion();

    // GUI utils:

    qreal getScaleFactor();
    int scale(int value);
    qreal scale(qreal value);
    QSize scale(int width, int height);

    // Path utils:

    QString getTemporaryPath(const QString &subdirectory = QString());
    QString getLocalConfigPath(const QString &subdirectory = QString());
    QString getSharedPath(const QString &resource = QString());
    QString getBinaryPath(const QString &executable);
    QIcon getLocaleFlag(const QLocale &locale);

    // URL utils:

    QString getWebsiteUrl();
    QString getWebsiteUtmUrl();
    QString getUpdateUrl();
    QString getRepositoryUrl();
    QString getIssuesUrl();
    QString getTranslationsUrl();
    QString getDonationsUrl();
    QString getBlogPostUrl(const QString &slug);
    QString getVersionInfoUrl();
    QString getDonorsInfoUrl();

    // Android utils:

    bool isDrawableResource(const QFileInfo &file);
    QString getAndroidCodename(int api);
}

#endif // UTILS_H
