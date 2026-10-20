#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

class DatabaseManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)
public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;
    QString lastError() const { return m_error; }
    Q_INVOKABLE QVariantList rows(const QString &table, const QString &search = QString()) const;
    Q_INVOKABLE QVariantMap record(const QString &table, int id) const;
    Q_INVOKABLE QVariantList options(const QString &table) const;
    Q_INVOKABLE QVariantMap dashboard() const;
    Q_INVOKABLE QVariantList saleItems(int saleId) const;
    Q_INVOKABLE QString save(const QString &table, const QVariantMap &values);
    Q_INVOKABLE QString remove(const QString &table, int id);
    Q_INVOKABLE QString createSale(int customerId, const QVariantList &items);
    Q_INVOKABLE QString exportTable(const QString &table, const QString &format);
    Q_INVOKABLE QString captureDirectory() const;
signals:
    void dataChanged();
    void errorChanged();
private:
    QSqlDatabase m_db;
    QString m_error;
    void setError(const QString &error);
    bool execute(const QString &sql);
    bool seed();
    bool init();
    QStringList fields(const QString &table) const;
    QString selectSql(const QString &table) const;
};
