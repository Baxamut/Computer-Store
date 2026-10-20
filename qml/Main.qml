import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: app
    width: 1280; height: 800; minimumWidth: 1100; minimumHeight: 700
    visible: true; title: "TechStore — магазин компьютерной техники"
    color: "#0d111a"
    palette.button: "#293b62"
    palette.buttonText: "#e9edf5"
    palette.base: "#202839"
    palette.text: "#e9edf5"
    property string section: "Главная"
    property string table: ""
    property var records: []
    property var stats: ({})
    property int editingId: 0
    property int deletingId: 0
    property var formFields: []
    property var formData: ({})
    property var saleLines: []
    property var productOptions: []
    property var customerOptions: []
    readonly property color panel: "#171d29"
    readonly property color panel2: "#202839"
    readonly property color textColor: "#e9edf5"
    readonly property color muted: "#9da9bd"
    readonly property color accent: "#6d9dff"
    readonly property int actionColumnWidth: 160

    function tableFor(name) {
        return ({"Товары":"Products","Категории":"Categories","Производители":"Manufacturers","Поставщики":"Suppliers","Покупатели":"Customers","Продажи":"Sales"})[name] || ""
    }
    function fieldsFor(t) {
        if (t === "Products") return [
            {key:"name",label:"Название",type:"text"}, {key:"category_id",label:"Категория",type:"combo",source:"Categories"},
            {key:"manufacturer_id",label:"Производитель",type:"combo",source:"Manufacturers"}, {key:"supplier_id",label:"Поставщик",type:"combo",source:"Suppliers"},
            {key:"article",label:"Артикул",type:"text"}, {key:"price",label:"Цена, ₽",type:"number"},
            {key:"quantity",label:"Количество",type:"integer"}, {key:"warranty_months",label:"Гарантия, мес.",type:"integer"}]
        if (t === "Categories") return [{key:"name",label:"Название",type:"text"},{key:"description",label:"Описание",type:"text"}]
        if (t === "Manufacturers") return [{key:"name",label:"Название",type:"text"},{key:"country",label:"Страна",type:"text"}]
        if (t === "Suppliers") return [{key:"name",label:"Название",type:"text"},{key:"phone",label:"Телефон",type:"text"},{key:"email",label:"Email",type:"text"}]
        if (t === "Customers") return [{key:"full_name",label:"ФИО",type:"text"},{key:"phone",label:"Телефон",type:"text"},{key:"email",label:"Email",type:"text"}]
        return []
    }
    function columnsFor(t) {
        if (t === "Products") return [{key:"name",label:"Название",w:155},{key:"category",label:"Категория",w:105},{key:"manufacturer",label:"Производитель",w:110},{key:"supplier",label:"Поставщик",w:120},{key:"article",label:"Артикул",w:115},{key:"price",label:"Цена",w:75},{key:"quantity",label:"Остаток",w:65},{key:"warranty_months",label:"Гарантия",w:75}]
        if (t === "Sales") return [{key:"id",label:"№",w:70},{key:"customer",label:"Покупатель",w:280},{key:"sale_date",label:"Дата",w:190},{key:"total_amount",label:"Сумма, ₽",w:130}]
        if (t === "Customers") return [{key:"full_name",label:"ФИО",w:260},{key:"phone",label:"Телефон",w:180},{key:"email",label:"Email",w:240}]
        if (t === "Suppliers") return [{key:"name",label:"Название",w:240},{key:"phone",label:"Телефон",w:180},{key:"email",label:"Email",w:240}]
        if (t === "Manufacturers") return [{key:"name",label:"Название",w:240},{key:"country",label:"Страна",w:240}]
        return [{key:"name",label:"Название",w:260},{key:"description",label:"Описание",w:400}]
    }
    function selectSection(name) {
        section = name; table = tableFor(name); search.text = ""; refresh()
    }
    function refresh() {
        stats = store.dashboard()
        if (table !== "") records = store.rows(table,search.text)
        productOptions = store.options("Products")
        customerOptions = store.options("Customers")
    }
    function notice(message, bad) { notification.text = message; notification.bad = bad; notification.open() }
    function openForm(id) {
        editingId = id; formFields = fieldsFor(table); formData = id > 0 ? store.record(table,id) : ({})
        formDialog.title = id > 0 ? "Редактирование записи" : "Добавление записи"
        formDialog.open()
    }
    function display(value, key) {
        if (value === undefined || value === null) return ""
        if (key === "price" || key === "total_amount") return Number(value).toLocaleString(Qt.locale("ru_RU"),'f',2)
        return String(value)
    }
    Component.onCompleted: refresh()
    function prepareCapture(step) {
        if (step === 1) selectSection("Товары")
        else if (step === 2) openForm(0)
        else if (step === 3) { formDialog.close(); openForm(records[0].id) }
        else if (step === 4) { formDialog.close(); search.text="RTX" }
        else if (step === 5) { search.text=""; selectSection("Продажи"); saleDialog.open() }
        else if (step === 6) { saleDialog.close(); selectSection("Товары"); let path=store.exportTable("Products","pdf"); notice("Экспорт PDF: "+path,false) }
    }
    Connections { target: store; function onDataChanged() { app.refresh() } }

    Rectangle { anchors.fill: parent; color: "#0d111a" }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.fillHeight: true; Layout.preferredWidth: 224; color: "#131a27"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18; spacing: 7
                Label { text: "◈  TechStore"; color: app.textColor; font.pixelSize: 25; font.bold: true; Layout.bottomMargin: 22 }
                Label { text: "УПРАВЛЕНИЕ МАГАЗИНОМ"; color: app.muted; font.pixelSize: 10; font.bold: true; Layout.bottomMargin: 10 }
                Repeater {
                    model: ["Главная","Товары","Категории","Производители","Поставщики","Покупатели","Продажи"]
                    delegate: Rectangle {
                        required property string modelData
                        Layout.fillWidth: true; height: 47; radius: 9
                        color: app.section === modelData ? "#293b62" : navHover.containsMouse ? "#202a3d" : "transparent"
                        Behavior on color { ColorAnimation { duration: 140 } }
                        Label { anchors.centerIn: parent; text: modelData; color: app.section === modelData ? "#ffffff" : app.muted; font.pixelSize: 14; font.bold: app.section === modelData }
                        MouseArea { id: navHover; anchors.fill: parent; hoverEnabled: true; onClicked: app.selectSection(modelData) }
                    }
                }
                Item { Layout.fillHeight: true }
                Label { text: "Qt 6  •  SQLite"; color: app.muted; font.pixelSize: 11 }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 78; color: "#101722"
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 28; anchors.rightMargin: 28
                    ColumnLayout { Label { text: app.section; color: app.textColor; font.pixelSize: 26; font.bold: true }
                        Label { text: app.section === "Главная" ? "Обзор показателей и состояния склада" : "Управление данными магазина"; color: app.muted; font.pixelSize: 12 } }
                    Item { Layout.fillWidth: true }
                    Button { text: "Справка"; onClicked: helpDialog.open() }
                    Label { text: Qt.formatDate(new Date(),"dd.MM.yyyy"); color: app.muted; font.pixelSize: 13 }
                }
            }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: parent.width; spacing: 18
                    Item { Layout.preferredHeight: 8 }
                    ColumnLayout {
                        visible: app.section === "Главная"; Layout.fillWidth: true; Layout.leftMargin: 28; Layout.rightMargin: 28; spacing: 18
                        Flow {
                            Layout.fillWidth: true; spacing: 14
                            Repeater {
                                model: [{label:"Товаров",key:"Products"},{label:"Категорий",key:"Categories"},{label:"Покупателей",key:"Customers"},{label:"Продаж",key:"Sales"},{label:"Выручка, ₽",key:"revenue"}]
                                delegate: Rectangle {
                                    required property var modelData
                                    width: 180; height: 110; radius: 13; color: app.panel
                                    border.color: "#283449"
                                    Column { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 18; spacing: 11
                                        Label { text: modelData.label; color: app.muted; font.pixelSize: 13 }
                                        Label { text: modelData.key === "revenue" ? Number(app.stats[modelData.key] || 0).toLocaleString(Qt.locale("ru_RU"),'f',2) : (app.stats[modelData.key] || 0); color: app.textColor; font.pixelSize: 25; font.bold: true }
                                    }
                                }
                            }
                        }
                        Label { text: "Товары с низким остатком (≤ 5)"; color: app.textColor; font.pixelSize: 19; font.bold: true; Layout.topMargin: 10 }
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: Math.max(100,lowRepeater.count*48+30); radius: 12; color: app.panel
                            Column { anchors.fill: parent; anchors.margins: 16; spacing: 5
                                Repeater { id: lowRepeater; model: app.stats.lowStock || []
                                    delegate: Rectangle { required property var modelData; required property int index; width: parent.width; height: 42; radius: 6; color: index % 2 ? "#1d2636" : "#202a3a"
                                        RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
                                            Label { text: modelData.name; color: app.textColor; Layout.fillWidth: true }
                                            Label { text: modelData.article; color: app.muted; Layout.preferredWidth: 190 }
                                            Label { text: modelData.quantity + " шт."; color: "#f4b66a"; font.bold: true }
                                        }
                                    }
                                }
                                Label { visible: lowRepeater.count === 0; text: "Все товары в достаточном количестве"; color: app.muted }
                            }
                        }
                    }
                    ColumnLayout {
                        visible: app.section !== "Главная"; Layout.fillWidth: true; Layout.leftMargin: 28; Layout.rightMargin: 28; spacing: 15
                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            TextField { id: search; Layout.fillWidth: true; Layout.maximumWidth: 360; placeholderText: app.table === "Products" ? "Название, артикул, категория, бренд" : "Поиск"; color: app.textColor; selectByMouse: true
                                background: Rectangle { radius: 8; color: app.panel; border.color: search.activeFocus ? app.accent : "#344055" }
                                onTextChanged: if (app.table !== "") app.records = store.rows(app.table,text)
                            }
                            Button { text: "Сброс"; onClicked: search.text = "" }
                            Item { Layout.fillWidth: true }
                            Button { text: "PDF"; onClicked: { let r=store.exportTable(app.table,"pdf"); app.notice(r.endsWith(".pdf") ? "Сохранено: "+r : r,!r.endsWith(".pdf")) } }
                            Button { text: "JSON"; onClicked: { let r=store.exportTable(app.table,"json"); app.notice(r.endsWith(".json") ? "Сохранено: "+r : r,!r.endsWith(".json")) } }
                            Button { text: "CSV"; onClicked: { let r=store.exportTable(app.table,"csv"); app.notice(r.endsWith(".csv") ? "Сохранено: "+r : r,!r.endsWith(".csv")) } }
                            Button { text: app.table === "Sales" ? "+ Продажа" : "+ Добавить"; highlighted: true; onClicked: app.table === "Sales" ? saleDialog.open() : app.openForm(0) }
                        }
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: Math.max(400,app.records.length*54+60); color: app.panel; radius: 12
                            Flickable {
                                anchors.fill: parent; anchors.margins: 12; contentWidth: Math.max(width, headerRow.width); contentHeight: listColumn.height; clip: true
                                Column { id: listColumn; spacing: 3
                                    Row { id: headerRow; height: 42; spacing: 0
                                        Repeater { model: app.columnsFor(app.table)
                                            delegate: Label { required property var modelData; width: modelData.w; height: 42; text: modelData.label; color: app.muted; font.bold: true; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; leftPadding: 9 }
                                        }
                                        Label { width: app.actionColumnWidth; height: 42; text: "Действия"; color: app.muted; font.bold: true; verticalAlignment: Text.AlignVCenter; leftPadding: 9 }
                                    }
                                    Repeater { model: app.records
                                        delegate: Rectangle {
                                            required property var modelData
                                            required property int index
                                            width: headerRow.width; height: 50; radius: 7; color: rowHover.containsMouse ? "#2a3448" : index % 2 ? "#1a2231" : "#202a39"
                                            Behavior on color { ColorAnimation { duration: 110 } }
                                            MouseArea { id: rowHover; anchors.fill: parent; hoverEnabled: true; onDoubleClicked: { if (app.table === "Sales") { detailTitle.text = "Продажа №"+modelData.id; detailRows.model = store.saleItems(modelData.id); detailDialog.open() } else app.openForm(modelData.id) } }
                                            Row { anchors.fill: parent
                                                Repeater { model: app.columnsFor(app.table)
                                                    delegate: Label { required property var modelData; width: modelData.w; height: 50; text: app.display(parent.parent.modelData[modelData.key],modelData.key); color: app.textColor; font.pixelSize: 12; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; leftPadding: 9; rightPadding: 7 }
                                                }
                                                Row { width: app.actionColumnWidth; height: 50; spacing: 4
                                                    Button { text: app.table === "Sales" ? "Состав" : "Изменить"; width: 100; height: 35; anchors.verticalCenter: parent.verticalCenter
                                                        onClicked: { if (app.table === "Sales") { detailTitle.text = "Продажа №"+modelData.id; detailRows.model = store.saleItems(modelData.id); detailDialog.open() } else app.openForm(modelData.id) }
                                                    }
                                                    Button { visible: app.table !== "Sales"; text: "×"; width: 44; height: 35; anchors.verticalCenter: parent.verticalCenter
                                                        onClicked: { app.deletingId=modelData.id; deleteDialog.open() } }
                                                }
                                            }
                                        }
                                    }
                                    Label { visible: app.records.length === 0; text: "Записи не найдены"; color: app.muted; font.pixelSize: 15; topPadding: 25 }
                                }
                            }
                        }
                    }
                    Item { Layout.preferredHeight: 18 }
                }
            }
        }
    }

    Dialog {
        id: formDialog; modal: true; width: 560; height: Math.min(app.height-80,630); anchors.centerIn: parent
        standardButtons: Dialog.Save | Dialog.Cancel
        onAccepted: {
            let payload={id:app.editingId}
            for (let i=0;i<formRepeater.count;i++) { let f=formRepeater.itemAt(i); payload[f.field.key]=f.value() }
            let result=store.save(app.table,payload)
            if (result !== "") { app.notice(result,true); formDialog.open() }
            else app.notice("Запись сохранена",false)
        }
        background: Rectangle { color: app.panel; radius: 12; border.color: "#344055" }
        header: Label { text: formDialog.title; color: app.textColor; font.pixelSize: 20; font.bold: true; padding: 22 }
        contentItem: ScrollView { clip: true
            ColumnLayout { width: formDialog.width-52; spacing: 12
                Repeater { id: formRepeater; model: app.formFields
                    delegate: ColumnLayout {
                        required property var modelData
                        property var field: modelData
                        function value() { return field.type === "combo" ? (choice.currentIndex >= 0 ? choice.model[choice.currentIndex].id : 0) : field.type === "number" ? Number(input.text.replace(",",".")) : field.type === "integer" ? Number(input.text) : input.text.trim() }
                        Layout.fillWidth: true; spacing: 4
                        Label { text: parent.field.label; color: app.muted; font.pixelSize: 12 }
                        TextField { id: input; visible: parent.field.type !== "combo"; Layout.fillWidth: true; color: app.textColor; selectByMouse: true
                            text: parent.field.type !== "combo" ? String(app.formData[parent.field.key] ?? "") : ""
                            validator: parent.field.type === "number" ? numberValidator : parent.field.type === "integer" ? integerValidator : null
                            background: Rectangle { radius: 7; color: app.panel2; border.color: input.activeFocus ? app.accent : "#35425a" }
                        }
                        ComboBox { id: choice; visible: parent.field.type === "combo"; Layout.fillWidth: true
                            model: visible ? store.options(parent.field.source) : []; textRole: "label"
                            currentIndex: { let id=Number(app.formData[parent.field.key] || 0); for (let i=0;i<model.length;i++) if (model[i].id===id) return i; return model.length ? 0 : -1 }
                        }
                    }
                }
            }
        }
    }
    DoubleValidator { id: numberValidator; bottom: 0; decimals: 2; notation: DoubleValidator.StandardNotation }
    IntValidator { id: integerValidator; bottom: 0 }
    Dialog { id: deleteDialog; title: "Подтверждение удаления"; modal: true; anchors.centerIn: parent; width: 420
        standardButtons: Dialog.Yes | Dialog.No
        contentItem: Label { text: "Удалить выбранную запись? Связанные данные могут запрещать удаление."; color: app.textColor; wrapMode: Text.WordWrap; padding: 20 }
        onAccepted: { let result=store.remove(app.table,app.deletingId); app.notice(result || "Запись удалена",result !== "") }
    }
    Dialog { id: saleDialog; title: "Новая продажа"; modal: true; anchors.centerIn: parent; width: 650; height: 580
        standardButtons: Dialog.Save | Dialog.Cancel
        onOpened: { app.saleLines=[]; app.productOptions=store.options("Products"); app.customerOptions=store.options("Customers"); saleQty.text="1" }
        onAccepted: {
            let id=customerChoice.currentIndex >= 0 ? app.customerOptions[customerChoice.currentIndex].id : 0
            let result=store.createSale(id,app.saleLines)
            if (result !== "") { app.notice(result,true); saleDialog.open() } else app.notice("Продажа оформлена",false)
        }
        contentItem: ColumnLayout { spacing: 10
            Label { text: "Покупатель"; color: app.muted }
            ComboBox { id: customerChoice; Layout.fillWidth: true; model: app.customerOptions; textRole: "label" }
            Label { text: "Товар"; color: app.muted }
            ComboBox { id: productChoice; Layout.fillWidth: true; model: app.productOptions; textRole: "label" }
            RowLayout { TextField { id: saleQty; Layout.preferredWidth: 130; placeholderText: "Количество"; validator: IntValidator { bottom: 1 } }
                Button { text: "Добавить позицию"; onClicked: {
                    if (productChoice.currentIndex < 0 || Number(saleQty.text)<1) { app.notice("Укажите товар и количество",true); return }
                    let p=app.productOptions[productChoice.currentIndex]
                    let copy=app.saleLines.slice(); let found=false
                    for (let i=0;i<copy.length;i++) if (copy[i].product_id===p.id) { copy[i].quantity+=Number(saleQty.text); found=true }
                    if (!found) copy.push({product_id:p.id,quantity:Number(saleQty.text),name:p.label})
                    app.saleLines=copy
                } }
            }
            Label { text: "Позиции продажи"; color: app.textColor; font.bold: true; Layout.topMargin: 12 }
            Repeater { model: app.saleLines
                delegate: RowLayout { required property var modelData; required property int index; Layout.fillWidth: true
                    Label { text: modelData.name; color: app.textColor; Layout.fillWidth: true }
                    Label { text: modelData.quantity+" шт."; color: app.muted }
                    Button { text: "×"; onClicked: { let copy=app.saleLines.slice(); copy.splice(index,1); app.saleLines=copy } }
                }
            }
            Item { Layout.fillHeight: true }
            Label { text: "Цена и итог рассчитываются по текущим ценам в базе при сохранении."; color: app.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }
    Dialog { id: detailDialog; title: "Состав продажи"; modal: true; anchors.centerIn: parent; width: 540; height: 400; standardButtons: Dialog.Close
        contentItem: ColumnLayout {
            Label { id: detailTitle; color: app.textColor; font.pixelSize: 17; font.bold: true }
            Repeater { id: detailRows
                delegate: RowLayout { required property var modelData; Layout.fillWidth: true
                    Label { text: modelData.name; color: app.textColor; Layout.fillWidth: true }
                    Label { text: modelData.quantity+" × "+Number(modelData.price).toLocaleString(Qt.locale("ru_RU"),'f',2)+" ₽"; color: app.muted }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }
    Dialog { id: helpDialog; title: "Справка TechStore"; modal: true; anchors.centerIn: parent; width: 540; height: 360; standardButtons: Dialog.Close
        contentItem: Label { color: app.textColor; wrapMode: Text.WordWrap; padding: 16
            text: "Выберите раздел в боковом меню. Используйте «+ Добавить» для новой записи, «Изменить» для правки и × для удаления. Поиск работает по полям текущего раздела. В разделе «Продажи» выберите покупателя, товары и количество; остаток и итог программа проверит при сохранении. Кнопки PDF, JSON и CSV сохраняют файл в папку exports рядом с базой данных."
        }
    }
    Popup { id: notification; property bool bad: false; property alias text: noteLabel.text
        x: app.width-width-24; y: app.height-height-25; width: Math.min(520,app.width-50); height: 62; closePolicy: Popup.CloseOnPressOutside
        background: Rectangle { radius: 10; color: notification.bad ? "#61323a" : "#254c40"; border.color: notification.bad ? "#a85460" : "#397a64" }
        contentItem: Label { id: noteLabel; color: "white"; wrapMode: Text.WordWrap; verticalAlignment: Text.AlignVCenter; padding: 12 }
        Timer { running: notification.visible; interval: 4300; onTriggered: notification.close() }
    }
}
