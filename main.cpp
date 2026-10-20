#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QDir>
#include <QFileInfo>
#include "database/DatabaseManager.h"

static void logMessage(QtMsgType, const QMessageLogContext &, const QString &message) {
    const QString path=qEnvironmentVariable("TECHSTORE_LOG_FILE");
    if (path.isEmpty()) return;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)) {
        QTextStream stream(&file); stream << message << '\n';
    }
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(logMessage);
    QGuiApplication app(argc, argv);
    app.setApplicationName("TechStore");
    app.setOrganizationName("TechStore Practice");
    QQuickStyle::setStyle("Basic");
    DatabaseManager database;
    if (app.arguments().contains("--self-test")) {
        auto check=[](bool condition,const QString &label) { qWarning() << (condition ? "PASS" : "FAIL") << label; return condition; };
        bool ok=true;
        ok &= check(database.dashboard().value("Categories").toInt()==8,"seed categories");
        ok &= check(database.rows("Products","RTX").size()==2,"JOIN search");
        const auto before=database.record("Products",1).value("quantity").toInt();
        const auto oversell=database.createSale(1,QVariantList{QVariantMap{{"product_id",1},{"quantity",before+1}}});
        ok &= check(!oversell.isEmpty() && database.record("Products",1).value("quantity").toInt()==before,"oversell rollback");
        ok &= check(database.createSale(1,QVariantList{QVariantMap{{"product_id",1},{"quantity",1}}}).isEmpty(),"transactional sale");
        ok &= check(database.record("Products",1).value("quantity").toInt()==before-1,"stock decrease");
        ok &= check(database.save("Categories",QVariantMap{{"name","Тестовая категория"},{"description","Проверка CRUD"}}).isEmpty(),"insert category");
        int testId=0;
        for (const auto &row:database.rows("Categories","Тестовая категория")) testId=row.toMap().value("id").toInt();
        ok &= check(testId>0,"read category");
        ok &= check(database.save("Categories",QVariantMap{{"id",testId},{"name","Тестовая категория 2"},{"description","Изменено"}}).isEmpty(),"update category");
        ok &= check(database.record("Categories",testId).value("name").toString()=="Тестовая категория 2","read update");
        ok &= check(database.remove("Categories",testId).isEmpty(),"delete category");
        ok &= check(!database.remove("Categories",1).isEmpty(),"foreign key restrict");
        for (const QString &format:{"json","csv","pdf"}) {
            const QString path=database.exportTable("Products",format);
            ok &= check(QFileInfo(path).exists() && QFileInfo(path).size()>100,"export "+format);
        }
        return ok ? 0 : 2;
    }
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("store", &database);
    engine.loadFromModule("TechStore", "Main");
    if (engine.rootObjects().isEmpty()) return -1;
    const QString capture=qEnvironmentVariable("TECHSTORE_CAPTURE_DIR");
    if (!capture.isEmpty()) {
        auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        auto *timer=new QTimer(&app);
        auto *step=new int(0);
        QObject::connect(timer,&QTimer::timeout,&app,[=,&app]() {
            static const QStringList names={"01_dashboard","02_products","03_add_product","04_edit_product","05_search","06_sales","07_export"};
            if (*step<names.size()) {
                const QString path=QDir(capture).filePath(names[*step]+".png");
                if (!window->grabWindow().save(path)) qWarning() << "Screenshot failed:" << path;
            }
            ++*step;
            if (*step>=names.size()) { timer->stop(); delete step; app.quit(); }
            else QMetaObject::invokeMethod(window,"prepareCapture",Q_ARG(QVariant,QVariant(*step)));
        });
        timer->start(1200);
    }
    return app.exec();
}
