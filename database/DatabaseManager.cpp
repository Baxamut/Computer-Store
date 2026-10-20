#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPdfWriter>
#include <QPainter>
#include <QDateTime>
#include <QDebug>
#include <cmath>

static QVariantMap asMap(const QSqlQuery &q) {
    QVariantMap result;
    const auto rec = q.record();
    for (int i = 0; i < rec.count(); ++i) result.insert(rec.fieldName(i), q.value(i));
    return result;
}

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {
    m_db = QSqlDatabase::addDatabase("QSQLITE", "techstore");
    QString folder = qEnvironmentVariable("TECHSTORE_DATA_DIR");
    if (folder.isEmpty()) folder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(folder);
    m_db.setDatabaseName(QDir(folder).filePath("techstore.sqlite"));
    if (!m_db.open()) { setError(m_db.lastError().text()); return; }
    if (!init()) qWarning() << "Database initialization:" << m_error;
}

DatabaseManager::~DatabaseManager() {
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase("techstore");
}

void DatabaseManager::setError(const QString &error) { m_error = error; emit errorChanged(); }
bool DatabaseManager::execute(const QString &sql) {
    QSqlQuery q(m_db);
    if (!q.exec(sql)) { setError(q.lastError().text()); return false; }
    return true;
}

bool DatabaseManager::init() {
    if (!execute("PRAGMA foreign_keys = ON")) return false;
    QSqlQuery pragma(m_db);
    if (!pragma.exec("PRAGMA foreign_keys") || !pragma.next() || pragma.value(0).toInt() != 1) {
        setError("Не удалось включить внешние ключи SQLite"); return false;
    }
    const QStringList schema = {
        "CREATE TABLE IF NOT EXISTS Categories (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE, description TEXT NOT NULL DEFAULT '')",
        "CREATE TABLE IF NOT EXISTS Manufacturers (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE, country TEXT NOT NULL DEFAULT '')",
        "CREATE TABLE IF NOT EXISTS Suppliers (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE, phone TEXT NOT NULL DEFAULT '', email TEXT NOT NULL DEFAULT '')",
        "CREATE TABLE IF NOT EXISTS Customers (id INTEGER PRIMARY KEY, full_name TEXT NOT NULL, phone TEXT NOT NULL DEFAULT '', email TEXT NOT NULL DEFAULT '')",
        "CREATE TABLE IF NOT EXISTS Products (id INTEGER PRIMARY KEY, name TEXT NOT NULL, category_id INTEGER NOT NULL REFERENCES Categories(id) ON DELETE RESTRICT, manufacturer_id INTEGER NOT NULL REFERENCES Manufacturers(id) ON DELETE RESTRICT, supplier_id INTEGER NOT NULL REFERENCES Suppliers(id) ON DELETE RESTRICT, article TEXT NOT NULL UNIQUE, price REAL NOT NULL CHECK(price >= 0), quantity INTEGER NOT NULL CHECK(quantity >= 0), warranty_months INTEGER NOT NULL CHECK(warranty_months >= 0))",
        "CREATE TABLE IF NOT EXISTS Sales (id INTEGER PRIMARY KEY, customer_id INTEGER NOT NULL REFERENCES Customers(id) ON DELETE RESTRICT, sale_date TEXT NOT NULL, total_amount REAL NOT NULL CHECK(total_amount >= 0))",
        "CREATE TABLE IF NOT EXISTS SaleItems (id INTEGER PRIMARY KEY, sale_id INTEGER NOT NULL REFERENCES Sales(id) ON DELETE CASCADE, product_id INTEGER NOT NULL REFERENCES Products(id) ON DELETE RESTRICT, quantity INTEGER NOT NULL CHECK(quantity > 0), price REAL NOT NULL CHECK(price >= 0))",
        "CREATE INDEX IF NOT EXISTS idx_products_category ON Products(category_id)",
        "CREATE INDEX IF NOT EXISTS idx_products_manufacturer ON Products(manufacturer_id)",
        "CREATE INDEX IF NOT EXISTS idx_sales_date ON Sales(sale_date)"
    };
    for (const auto &sql : schema) if (!execute(sql)) return false;
    return seed();
}

bool DatabaseManager::seed() {
    QSqlQuery check(m_db);
    if (!check.exec("SELECT COUNT(*) FROM Categories") || !check.next()) { setError(check.lastError().text()); return false; }
    if (check.value(0).toInt() > 0) return true;
    if (!m_db.transaction()) { setError(m_db.lastError().text()); return false; }
    const QStringList inserts = {
        "INSERT INTO Categories(name,description) VALUES ('Процессоры','Центральные процессоры'),('Видеокарты','Дискретные графические адаптеры'),('Материнские платы','Платы для сборки ПК'),('Оперативная память','Модули памяти'),('Накопители','SSD и HDD'),('Блоки питания','Источники питания ПК'),('Корпуса','Корпуса для сборки ПК'),('Периферия','Устройства ввода и вывода')",
        "INSERT INTO Manufacturers(name,country) VALUES ('Intel','США'),('AMD','США'),('NVIDIA','США'),('ASUS','Тайвань'),('MSI','Тайвань'),('Gigabyte','Тайвань'),('Kingston','США'),('Samsung','Республика Корея'),('Corsair','США')",
        "INSERT INTO Suppliers(name,phone,email) VALUES ('ТехДистрибьютор','+7 495 100-20-30','sales@techdist.example'),('КомплектОпт','+7 812 200-30-40','info@komplekt.example'),('Цифровой склад','+7 495 300-40-50','orders@digital.example')",
        "INSERT INTO Customers(full_name,phone,email) VALUES ('Иванов Пётр Сергеевич','+7 900 111-22-33','ivanov@example.com'),('Смирнова Анна Павловна','+7 900 222-33-44','smirnova@example.com'),('Кузнецов Максим Олегович','+7 900 333-44-55','kuznetsov@example.com')",
        "INSERT INTO Products(name,category_id,manufacturer_id,supplier_id,article,price,quantity,warranty_months) VALUES ('Core i5-14400F',1,1,1,'CPU-INT-14400F',18990,12,36),('Ryzen 5 7600',1,2,2,'CPU-AMD-7600',21490,9,36),('GeForce RTX 4060 Dual',2,4,1,'GPU-ASU-4060',36990,5,36),('B650 Gaming Plus',3,5,2,'MB-MSI-B650',17990,7,36),('Fury Beast 32 ГБ',4,7,3,'RAM-KIN-32',9490,18,60),('990 EVO 1 ТБ',5,8,1,'SSD-SAM-1TB',10490,3,60),('RM750e 750 Вт',6,9,3,'PSU-COR-750',11990,4,84),('GeForce RTX 4070 Windforce',2,6,1,'GPU-GIG-4070',64990,2,36),('Vengeance 16 ГБ',4,9,2,'RAM-COR-16',4990,20,60),('TUF Gaming M3',8,4,3,'MOU-ASU-M3',2490,15,24)"
    };
    for (const auto &sql : inserts) if (!execute(sql)) { m_db.rollback(); return false; }
    if (!m_db.commit()) { setError(m_db.lastError().text()); return false; }
    QVariantList first; first << QVariantMap{{"product_id",1},{"quantity",1}} << QVariantMap{{"product_id",5},{"quantity",2}};
    QVariantList second; second << QVariantMap{{"product_id",3},{"quantity",1}};
    return createSale(1,first).isEmpty() && createSale(2,second).isEmpty();
}

QStringList DatabaseManager::fields(const QString &table) const {
    if (table == "Categories") return {"name","description"};
    if (table == "Manufacturers") return {"name","country"};
    if (table == "Suppliers") return {"name","phone","email"};
    if (table == "Customers") return {"full_name","phone","email"};
    if (table == "Products") return {"name","category_id","manufacturer_id","supplier_id","article","price","quantity","warranty_months"};
    return {};
}

QString DatabaseManager::selectSql(const QString &table) const {
    if (table == "Products") return "SELECT p.*,c.name AS category,m.name AS manufacturer,s.name AS supplier FROM Products p JOIN Categories c ON p.category_id=c.id JOIN Manufacturers m ON p.manufacturer_id=m.id JOIN Suppliers s ON p.supplier_id=s.id";
    if (table == "Sales") return "SELECT s.id,s.customer_id,s.sale_date,s.total_amount,c.full_name AS customer FROM Sales s JOIN Customers c ON s.customer_id=c.id";
    if (!fields(table).isEmpty()) return "SELECT * FROM " + table;
    return {};
}

QVariantList DatabaseManager::rows(const QString &table, const QString &search) const {
    QVariantList result;
    QString sql = selectSql(table);
    if (sql.isEmpty()) return result;
    if (!search.trimmed().isEmpty()) {
        if (table == "Products") sql += " WHERE p.name LIKE :term OR p.article LIKE :term OR c.name LIKE :term OR m.name LIKE :term";
        else if (table == "Sales") sql += " WHERE c.full_name LIKE :term OR s.sale_date LIKE :term";
        else if (table == "Customers") sql += " WHERE full_name LIKE :term OR phone LIKE :term OR email LIKE :term";
        else if (table == "Categories" || table == "Manufacturers" || table == "Suppliers") sql += " WHERE name LIKE :term";
    }
    sql += table == "Sales" ? " ORDER BY s.id DESC" : table == "Products" ? " ORDER BY p.id DESC" : " ORDER BY id DESC";
    QSqlQuery q(m_db);
    q.prepare(sql);
    if (!search.trimmed().isEmpty()) q.bindValue(":term", "%" + search.trimmed() + "%");
    if (!q.exec()) { qWarning() << q.lastError(); return result; }
    while (q.next()) result << asMap(q);
    return result;
}

QVariantMap DatabaseManager::record(const QString &table, int id) const {
    const QString sql = selectSql(table);
    if (sql.isEmpty()) return {};
    QSqlQuery q(m_db);
    q.prepare(sql + (table == "Products" ? " WHERE p.id=:id" : table == "Sales" ? " WHERE s.id=:id" : " WHERE id=:id"));
    q.bindValue(":id",id);
    return q.exec() && q.next() ? asMap(q) : QVariantMap{};
}

QVariantList DatabaseManager::options(const QString &table) const {
    QVariantList result;
    QString label = table == "Customers" ? "full_name" : "name";
    if (table != "Products" && table != "Customers" && fields(table).isEmpty()) return result;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT id," + label + " AS label FROM " + table + " ORDER BY label")) return result;
    while (q.next()) result << asMap(q);
    return result;
}

QVariantMap DatabaseManager::dashboard() const {
    QVariantMap out;
    const QStringList names = {"Products","Categories","Customers","Sales"};
    for (const auto &table : names) {
        QSqlQuery q(m_db);
        if (q.exec("SELECT COUNT(*) FROM " + table) && q.next()) out.insert(table, q.value(0));
    }
    QSqlQuery total(m_db);
    if (total.exec("SELECT COALESCE(SUM(total_amount),0) FROM Sales") && total.next()) out.insert("revenue",total.value(0));
    QSqlQuery low(m_db);
    QVariantList items;
    if (low.exec("SELECT name,article,quantity FROM Products WHERE quantity <= 5 ORDER BY quantity,name")) while (low.next()) items << asMap(low);
    out.insert("lowStock", items);
    return out;
}

QVariantList DatabaseManager::saleItems(int saleId) const {
    QVariantList result;
    QSqlQuery q(m_db);
    q.prepare("SELECT si.quantity,si.price,p.name,p.article FROM SaleItems si JOIN Products p ON si.product_id=p.id WHERE si.sale_id=:id");
    q.bindValue(":id",saleId);
    if (q.exec()) while (q.next()) result << asMap(q);
    return result;
}

QString DatabaseManager::save(const QString &table, const QVariantMap &values) {
    const QStringList allowed = fields(table);
    if (allowed.isEmpty()) return "Редактирование этой таблицы не поддерживается";
    const int id = values.value("id").toInt();
    const QString name = values.value(table == "Customers" ? "full_name" : "name").toString().trimmed();
    if (name.isEmpty()) return "Заполните название или имя";
    if (table == "Products") {
        if (values.value("article").toString().trimmed().isEmpty()) return "Заполните артикул";
        if (values.value("category_id").toInt() <= 0 || values.value("manufacturer_id").toInt() <= 0 || values.value("supplier_id").toInt() <= 0) return "Выберите категорию, производителя и поставщика";
        bool okPrice=false, okQty=false, okWarranty=false;
        double price=values.value("price").toDouble(&okPrice);
        int qty=values.value("quantity").toInt(&okQty), warranty=values.value("warranty_months").toInt(&okWarranty);
        if (!okPrice || !std::isfinite(price) || price<0 || !okQty || qty<0 || !okWarranty || warranty<0) return "Цена, остаток и гарантия должны быть неотрицательными числами";
    }
    QStringList names, binds;
    for (const auto &field : allowed) { names << field; binds << ":"+field; }
    QString sql;
    if (id > 0) {
        QStringList pairs;
        for (const auto &field : allowed) pairs << field+"= :"+field;
        sql = "UPDATE "+table+" SET "+pairs.join(',')+" WHERE id=:id";
    } else sql = "INSERT INTO "+table+" ("+names.join(',')+") VALUES ("+binds.join(',')+")";
    QSqlQuery q(m_db);
    q.prepare(sql);
    for (const auto &field : allowed) q.bindValue(":"+field, values.value(field));
    if (id > 0) q.bindValue(":id",id);
    if (!q.exec()) { setError(q.lastError().text()); return m_error; }
    emit dataChanged();
    return {};
}

QString DatabaseManager::remove(const QString &table, int id) {
    if (fields(table).isEmpty() || id <= 0) return "Некорректная запись";
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM "+table+" WHERE id=:id");
    q.bindValue(":id",id);
    if (!q.exec()) { setError(q.lastError().text()); return m_error; }
    emit dataChanged();
    return {};
}

QString DatabaseManager::createSale(int customerId, const QVariantList &items) {
    if (customerId <= 0 || items.isEmpty()) return "Выберите покупателя и добавьте товары";
    if (!m_db.transaction()) return m_db.lastError().text();
    QSqlQuery customer(m_db);
    customer.prepare("SELECT id FROM Customers WHERE id=:id");
    customer.bindValue(":id", customerId);
    if (!customer.exec() || !customer.next()) { m_db.rollback(); return "Покупатель не найден"; }
    struct Line { int product; int quantity; double price; };
    QList<Line> lines;
    double total=0;
    for (const auto &v : items) {
        const auto item=v.toMap();
        const int product=item.value("product_id").toInt(), quantity=item.value("quantity").toInt();
        if (product<=0 || quantity<=0) { m_db.rollback(); return "Укажите товар и положительное количество"; }
        QSqlQuery productQuery(m_db);
        productQuery.prepare("SELECT price,quantity FROM Products WHERE id=:id");
        productQuery.bindValue(":id",product);
        if (!productQuery.exec() || !productQuery.next()) { m_db.rollback(); return "Товар не найден"; }
        if (productQuery.value(1).toInt() < quantity) { m_db.rollback(); return "Недостаточно товара на складе"; }
        const double price=productQuery.value(0).toDouble();
        total += price*quantity;
        lines.append({product,quantity,price});
    }
    QSqlQuery sale(m_db);
    sale.prepare("INSERT INTO Sales(customer_id,sale_date,total_amount) VALUES (:customer,:date,:total)");
    sale.bindValue(":customer",customerId);
    sale.bindValue(":date",QDateTime::currentDateTime().toString(Qt::ISODate));
    sale.bindValue(":total",std::round(total*100)/100);
    if (!sale.exec()) { m_db.rollback(); setError(sale.lastError().text()); return m_error; }
    const int saleId=sale.lastInsertId().toInt();
    for (const auto &line : lines) {
        QSqlQuery stock(m_db);
        stock.prepare("UPDATE Products SET quantity=quantity-:quantity WHERE id=:id AND quantity>=:quantity");
        stock.bindValue(":quantity",line.quantity);
        stock.bindValue(":id",line.product);
        if (!stock.exec() || stock.numRowsAffected()!=1) { m_db.rollback(); return "Остаток изменился; продажа отменена"; }
        QSqlQuery item(m_db);
        item.prepare("INSERT INTO SaleItems(sale_id,product_id,quantity,price) VALUES (:sale,:product,:quantity,:price)");
        item.bindValue(":sale",saleId); item.bindValue(":product",line.product);
        item.bindValue(":quantity",line.quantity); item.bindValue(":price",line.price);
        if (!item.exec()) { m_db.rollback(); setError(item.lastError().text()); return m_error; }
    }
    if (!m_db.commit()) { setError(m_db.lastError().text()); m_db.rollback(); return m_error; }
    emit dataChanged();
    return {};
}

QString DatabaseManager::exportTable(const QString &table, const QString &format) {
    if (selectSql(table).isEmpty()) return "Неизвестный раздел";
    if (format!="json" && format!="csv" && format!="pdf") return "Неизвестный формат";
    QString folder=QDir(QFileInfo(m_db.databaseName()).absolutePath()).filePath("exports");
    QDir().mkpath(folder);
    const QString path=QDir(folder).filePath("TechStore_"+table+"_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+"."+format);
    const QVariantList data=rows(table);
    if (format=="json") {
        QJsonArray array;
        for (const auto &row : data) array.append(QJsonObject::fromVariantMap(row.toMap()));
        QFile file(path); if (!file.open(QIODevice::WriteOnly)) return file.errorString();
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    } else if (format=="csv") {
        QFile file(path); if (!file.open(QIODevice::WriteOnly|QIODevice::Text)) return file.errorString();
        QTextStream stream(&file); stream.setEncoding(QStringConverter::Utf8); stream << QChar(0xfeff);
        if (!data.isEmpty()) {
            const auto keys=data.first().toMap().keys();
            auto quote=[](QString value) { value.replace('"',"\"\""); return '"'+value+'"'; };
            QStringList header; for (const auto &key:keys) header << quote(key); stream << header.join(';') << '\n';
            for (const auto &row:data) { QStringList values; for (const auto &key:keys) values << quote(row.toMap().value(key).toString()); stream << values.join(';') << '\n'; }
        }
    } else {
        QPdfWriter pdf(path); pdf.setPageSize(QPageSize(QPageSize::A4)); pdf.setResolution(96);
        QPainter painter(&pdf); if (!painter.isActive()) { qWarning() << "PDF painter failed" << path; return "Не удалось создать PDF: " + path; }
        painter.fillRect(QRect(0,0,pdf.width(),pdf.height()),Qt::white);
        painter.setPen(QColor("#17202c")); QFont title("Arial",18,QFont::Bold); painter.setFont(title);
        painter.drawText(42,62,"TechStore — "+table);
        painter.setFont(QFont("Arial",10)); painter.drawText(42,86,"Дата: "+QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm"));
        QStringList keys, labels;
        QList<int> weights;
        if (table=="Products") { keys={"name","category","manufacturer","article","price","quantity"}; labels={"Товар","Категория","Бренд","Артикул","Цена, ₽","Остаток"}; weights={190,125,105,135,80,75}; }
        else if (table=="Sales") { keys={"id","customer","sale_date","total_amount"}; labels={"№","Покупатель","Дата","Итого, ₽"}; weights={55,280,200,125}; }
        else if (table=="Customers") { keys={"full_name","phone","email"}; labels={"Покупатель","Телефон","Email"}; weights={250,180,230}; }
        else if (table=="Suppliers") { keys={"name","phone","email"}; labels={"Поставщик","Телефон","Email"}; weights={250,180,230}; }
        else if (table=="Manufacturers") { keys={"name","country"}; labels={"Производитель","Страна"}; weights={350,310}; }
        else { keys={"name","description"}; labels={"Категория","Описание"}; weights={280,380}; }
        const int width=pdf.width()-84;
        int weightSum=0; for (int v:weights) weightSum+=v;
        int y=115;
        auto printRow=[&](const QVariantMap &row, bool header) {
            if (y+35>pdf.height()-40) { pdf.newPage(); y=50; }
            painter.fillRect(QRect(42,y-17,width,28),header?QColor("#d9e4f2"):QColor(y%2?"#ffffff":"#f5f7fa"));
            painter.setFont(QFont("Arial",header?10:9,header?QFont::Bold:QFont::Normal));
            int x=45;
            for (int i=0;i<keys.size();++i) {
                const int col=width*weights[i]/weightSum;
                const QString value=header?labels[i]:row.value(keys[i]).toString();
                painter.drawText(QRect(x,y-17,col-6,28),Qt::AlignVCenter|Qt::TextSingleLine, painter.fontMetrics().elidedText(value,Qt::ElideRight,col-8));
                x+=col;
            }
            y+=29;
        };
        printRow({},true);
        for (const auto &row:data) printRow(row.toMap(),false);
        painter.end();
    }
    return path;
}

QString DatabaseManager::captureDirectory() const { return qEnvironmentVariable("TECHSTORE_CAPTURE_DIR"); }
