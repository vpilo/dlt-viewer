#include "form.h"
#include "ui_form.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include "imagepreviewdialog.h"
#include "textviewdialog.h"
#include <QtDebug>

Form::Form(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::Form)
{
    selectedFiles=0;

    ui->setupUi(this);

    ui->treeWidget->sortByColumn(COLUMN_FILEDATE, Qt::AscendingOrder); // column/order to sort by
    ui->treeWidget->setSortingEnabled(true);             // should cause sort on add
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->selectButton, SIGNAL(clicked()),this, SLOT(selectAllClicked()));
    connect(ui->deselectButton, SIGNAL(clicked()),this, SLOT(deselectAllClicked()));
    connect(ui->clearAllButton, SIGNAL(clicked()),this, SLOT(clearAllClicked()));
    connect(ui->saveButton, SIGNAL(clicked()),this, SLOT(saveClicked()));
    connect(ui->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),this, SLOT(itemChanged(QTreeWidgetItem*,int)));
    connect(ui->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
    connect(ui->treeWidget->header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(sectionInTableDoubleClicked(int)));
}

Form::~Form()
{
    delete ui;
}


QTreeWidget* Form::getTreeWidget(){
    return ui->treeWidget;
}

void Form::selectAllClicked(){
    QTreeWidgetItemIterator it(ui->treeWidget,QTreeWidgetItemIterator::NotChecked|QTreeWidgetItemIterator::NoChildren);
    while (*it) {
        File *tmp = dynamic_cast<File*>(*it);

        if (tmp != NULL) {
            if(tmp->isComplete())
            {
                tmp->setCheckState(COLUMN_CHECK, Qt::Checked);
                selectedFiles++;
            }
        }
        ++it;
    }
}


void Form::deselectAllClicked(){
    QTreeWidgetItemIterator it(ui->treeWidget,QTreeWidgetItemIterator::NoChildren );//| QTreeWidgetItemIterator::Checked);

    while (*it) {
        File *tmp = dynamic_cast<File*>(*it);

        if (tmp != NULL) {

            if(tmp->isComplete())
            {
                tmp->setCheckState(COLUMN_CHECK, Qt::Unchecked);

            }
        }
        ++it;
    }
    selectedFiles=0;
}

void Form::clearAllClicked(){
    getTreeWidget()->clear();
}

void Form::itemChanged(QTreeWidgetItem* item,int i){

    if(i == COLUMN_CHECK){
        File *tmp = dynamic_cast<File*>(item);

        if (tmp != NULL) {

            if(tmp->isComplete() && (tmp->checkState(COLUMN_CHECK) == Qt::Unchecked) )
            {
                tmp->setCheckState(COLUMN_CHECK, Qt::Checked);
                selectedFiles++;

            }
            else{
                tmp->setCheckState(COLUMN_CHECK, Qt::Unchecked);
                selectedFiles--;

            }
        }
    }

}

void Form::saveClicked(){

    if(selectedFiles <= 0){
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("No files are selected.");
        msgBox.setWindowTitle("Filtransfer Plugin");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    QString path = QFileDialog::getExistingDirectory(this, tr("Save file to directory"),
                                                     QDir::currentPath(),QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(path != NULL){

        QTreeWidgetItemIterator it(ui->treeWidget,QTreeWidgetItemIterator::NoChildren );//| QTreeWidgetItemIterator::Checked);

        QString text;
        QString infoText;
        QString detailedText;
        QMessageBox msgBox;
        msgBox.setWindowTitle("Filtransfer Plugin");
        bool errorOccured = false;

        while (*it) {
            File *tmp = dynamic_cast<File*>(*it);

            if (tmp != NULL && tmp->isComplete() && (tmp->checkState(COLUMN_CHECK) == Qt::Checked)) {
                QString absolutePath = path+"//"+tmp->getFilename();
                if(!tmp->saveFile(absolutePath) ){
                    errorOccured = true;

                    text = ("Save incomplete.");
                    infoText ="The selected files were not saved to "+path+".\n";

                    detailedText += tmp->getFilenameOnTarget() + "\n";
                    msgBox.setIcon(QMessageBox::Critical);
                }
            }

            ++it;
        }

        if(!errorOccured)
        {
            msgBox.setIcon(QMessageBox::Information);
            text += ("Save successful.");
            infoText += ("All selected files were saved to "+path+".\n");
        }

        msgBox.setText(text);
        msgBox.setInformativeText(infoText);
        msgBox.setDetailedText(detailedText);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}

void Form::itemDoubleClicked ( QTreeWidgetItem * item, int column ){

    File *tmp = dynamic_cast<File*>(item);
    if (tmp != NULL && tmp->isComplete())
    {
        ImagePreviewDialog img(tmp->getFilenameOnTarget(),tmp->getFileData(),this);
        if(img.isFileSupported())
        {
            img.exec();

        }
        else
        {
            TextviewDialog text(tmp->getFilenameOnTarget(),tmp->getFileData(),this);
            text.exec();
            //QMessageBox::information(this,"Image Preview","Could not open file for preview. File format not supported.");

        }
        tmp->freeFile();
    }


}

void Form::sectionInTableDoubleClicked(int logicalIndex){
        ui->treeWidget->resizeColumnToContents(logicalIndex);
}

void Form::on_treeWidget_customContextMenuRequested(QPoint pos)
{
    /* show custom pop menu  for configuration */
    QPoint globalPos = ui->treeWidget->mapToGlobal(pos);
    QMenu menu(ui->treeWidget);
    QAction *action;

    QList<QTreeWidgetItem *> list = ui->treeWidget->selectedItems();

    if(list.count() == 1)
    {
        action = new QAction("&Save file", this);
        connect(action, SIGNAL(triggered()), this, SLOT(on_actionSave_triggered()));
        menu.addAction(action);

        menu.addSeparator();

        action = new QAction("&Remove file from list", this);
        connect(action, SIGNAL(triggered()), this, SLOT(on_actionDelete_triggered()));
        menu.addAction(action);

        /* show popup menu */
        menu.exec(globalPos);
    }

}
void Form::on_actionSave_triggered(){
    QList<QTreeWidgetItem *> list = ui->treeWidget->selectedItems();
    if((list.count() == 1))
    {
        deselectAllClicked();
        File* tmpFile = (File*)list.at(0);
        itemChanged(tmpFile,COLUMN_CHECK);
        saveClicked();
        itemChanged(tmpFile,COLUMN_CHECK);
    }
}

void Form::on_actionDelete_triggered(){

    QList<QTreeWidgetItem *> list = ui->treeWidget->selectedItems();
    if((list.count() == 1))
    {
        File* tmpFile = (File*)list.at(0);
        int index = ui->treeWidget->indexOfTopLevelItem(tmpFile);
        ui->treeWidget->takeTopLevelItem(index);
    }
}
