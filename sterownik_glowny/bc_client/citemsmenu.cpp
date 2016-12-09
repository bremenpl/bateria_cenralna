#include "citemsmenu.h"
#include "ui_citemsmenu.h"
#include "mainwindow.h"

CItemsMenu::CItemsMenu(QWidget *parent) : CAbstractMenu(parent), ui(new Ui::CItemsMenu)
{
    // start values
    m_currentItem = 0;
    m_noOfItems = 0;

    ui->setupUi(this);

    // connect with main window
    auto mainObj = dynamic_cast<MainWindow*>(parent);
    connect(this, SIGNAL(signal_deviceSelected(const EDeviceTypes, const int)),
            mainObj, SLOT(on_DeviceSelected(const EDeviceTypes, const int)));

    // set the menu name, it has to be replaced anyways
    m_menuName = "Items Menu";

    // add generic labels to the table view
    QStringList list;
    list << QObject::tr("Slave address") << QObject::tr("Serial number");
    ui->twItems->setColumnCount(list.size());
    ui->twItems->setHeaderLabels(list);

    // resize all
    for (int i = 0; i < ui->twItems->columnCount(); i++)
        ui->twItems->header()->resizeSection(i, 200);

    // share tree widget
    mp_tw = ui->twItems;
}

CItemsMenu::~CItemsMenu()
{
    delete ui;
}

/*!
 * \brief CItemsMenu::selectNextItem: Selects next item in the tree view
 */
void CItemsMenu::selectNextItem()
{
    QItemSelectionModel *selection = new QItemSelectionModel( mp_tw->model() );
    QModelIndex index;
    for (int col = 0; col < mp_tw->model()->columnCount(); col++)
    {
       index = mp_tw->model()->index(m_currentItem, col, QModelIndex() );
       selection->select(index, QItemSelectionModel::Select);
    }

    mp_tw->setSelectionModel( selection);
    mp_tw->scrollTo(index);

    //CBcLogger::instance()->print(MLL::ELogLevel::LDebug)
            //<< "Item index changed by btn: " << m_currentItem;
}

void CItemsMenu::on_pbItemsUp_clicked()
{
    // decrement item
    if (--m_currentItem < 0)
        m_currentItem = m_noOfItems - 1;

    selectNextItem();
}

void CItemsMenu::on_pbItemsDown_clicked()
{
    // increment item
    if (++m_currentItem > (m_noOfItems - 1))
        m_currentItem = 0;

    selectNextItem();
}

/*!
 * \brief CItemsMenu::on_twItems_itemClicked: Find index of selected item in this slot
 * \param item
 * \param column
 */
void CItemsMenu::on_twItems_itemClicked(QTreeWidgetItem *item, int column)
{
    // get selected item text
    QString itemText = item->text(column);

    QTreeWidgetItemIterator it(mp_tw);
    int index = 0;

    while (*it)
    {
        if ((*it)->text(column) == itemText)
        {
            // item found, update global index
            m_currentItem = index;

            //CBcLogger::instance()->print(MLL::ELogLevel::LDebug)
                    //<< "Item index changed by click: " << m_currentItem;
            return;
        }

        ++it;
        ++index;
    }

    //CBcLogger::instance()->print(MLL::ELogLevel::LWarning)
            //<< "Item index not found by click!";
}


void CItemsMenu::on_pbItemsEnter_clicked()
{
    emit signal_deviceSelected(m_deviceType, m_currentItem + 1);
}













