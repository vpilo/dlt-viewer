/**
 * @licence app begin@
 * Copyright (C) 2011-2012  BMW AG
 *
 * This file is part of GENIVI Project Dlt Viewer.
 *
 * Contributions are licensed to the GENIVI Alliance under one or more
 * Contribution License Agreements.
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * \file treemodel.cpp
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

#include <QtGui>
#include <QApplication>
#include <qmessagebox.h>

#include "tablemodel.h"
#include "fieldnames.h"
#include "dltsettingsmanager.h"
#include "dltuiutils.h"
#include "optmanager.h"
#include "dlt_protocol.h"

char buffer[DLT_VIEWER_LIST_BUFFER_SIZE];

 TableModel::TableModel(const QString & /*data*/, QObject *parent)
     : QAbstractTableModel(parent)
 {
     qfile = NULL;
     project = NULL;
     pluginManager = NULL;
     lastSearchIndex = -1;
     emptyForceFlag = false;
     loggingOnlyMode = false;
 }

 TableModel::~TableModel()
 {

 }

 int TableModel::columnCount(const QModelIndex & /*parent*/) const
 {
   if(project == NULL || project->settings == NULL)
     return DLT_VIEWER_COLUMN_COUNT;

     return DLT_VIEWER_COLUMN_COUNT+project->settings->showArguments;
 }

 QVariant TableModel::data(const QModelIndex &index, int role) const
 {
     QDltMsg msg;
     QByteArray buf;

     if (!index.isValid())
         return QVariant();

     if (qfile == NULL || project == NULL || project->settings == NULL)
         return QVariant();

     if (index.row() >= qfile->sizeFilter() && index.row()<0)
         return QVariant();

     if (role == Qt::DisplayRole)
     {
         /* get the message with the selected item id */
         if(loggingOnlyMode)
         {
             msg = QDltMsg();
         }
         else if(!qfile->getMsg(qfile->getMsgFilterPos(index.row()), msg))
         {
             if(index.column() == FieldNames::Index)
             {
                 return QString("%1").arg(qfile->getMsgFilterPos(index.row()));
             }
             else if(index.column() == FieldNames::Payload)
             {
                 return QString("!!CORRUPTED MESSAGE!!");
             }
             return QVariant();
         }
         if((DltSettingsManager::getInstance()->value("startup/pluginsEnabled", true).toBool()))
         {
             pluginManager->decodeMsg(msg,!OptManager::getInstance()->issilentMode());
         }

         switch(index.column())
         {
         case FieldNames::Index:
             /* display index */
             return qfile->getMsgFilterPos(index.row());
         case FieldNames::Time:
             if( project->settings->automaticTimeSettings == 0 )
                return QString("%1.%2").arg(msg.getGmTimeWithOffsetString(project->settings->utcOffset,project->settings->dst)).arg(msg.getMicroseconds(),6,10,QLatin1Char('0'));
             else
                return QString("%1.%2").arg(msg.getTimeString()).arg(msg.getMicroseconds(),6,10,QLatin1Char('0'));
         case FieldNames::TimeStamp:
             return QString("%1.%2").arg(msg.getTimestamp()/10000).arg(msg.getTimestamp()%10000,4,10,QLatin1Char('0'));
         case FieldNames::Counter:
             return msg.getMessageCounter();
         case FieldNames::EcuId:
             return msg.getEcuid();
         case FieldNames::AppId:
             switch(project->settings->showApIdDesc){
             case 0:
                 return msg.getApid();
                 break;
             case 1:
                   for(int num = 0; num < project->ecu->topLevelItemCount (); num++)
                    {
                     EcuItem *ecuitem = (EcuItem*)project->ecu->topLevelItem(num);
                     for(int numapp = 0; numapp < ecuitem->childCount(); numapp++)
                     {
                         ApplicationItem * appitem = (ApplicationItem *) ecuitem->child(numapp);
                         if(appitem->id == msg.getApid() && !appitem->description.isEmpty())
                         {
                            return appitem->description;
                         }
                     }
                    }
                   return QString("Apid: %1 (No description)").arg(msg.getApid());
                 break;
              default:
                 return msg.getApid();
             }
         case FieldNames::ContextId:
             switch(project->settings->showCtIdDesc){
             case 0:
                 return msg.getCtid();
                 break;
             case 1:

                   for(int num = 0; num < project->ecu->topLevelItemCount (); num++)
                    {
                     EcuItem *ecuitem = (EcuItem*)project->ecu->topLevelItem(num);
                     for(int numapp = 0; numapp < ecuitem->childCount(); numapp++)
                     {
                         ApplicationItem * appitem = (ApplicationItem *) ecuitem->child(numapp);
                         for(int numcontext = 0; numcontext < appitem->childCount(); numcontext++)
                         {
                             ContextItem * conitem = (ContextItem *) appitem->child(numcontext);

                             if(appitem->id == msg.getApid() && conitem->id == msg.getCtid()
                                     && !conitem->description.isEmpty())
                             {
                                return conitem->description;
                             }
                         }
                     }
                    }
                   return  QString("Ctid: %1 (No description)").arg(msg.getCtid());
                 break;
              default:
                 return msg.getCtid();
             }
         case FieldNames::SessionId:
             switch(project->settings->showSessionName){
             case 0:
                 return QString("%1").arg(msg.getSessionid());
                 break;
             case 1:
                 if(!msg.getSessionName().isEmpty())
                    return msg.getSessionName();
                else
                    return QString("%1").arg(msg.getSessionid());
                 break;
              default:
                 return QString("%1").arg(msg.getSessionid());
             }
         case FieldNames::Type:
             return msg.getTypeString();
         case FieldNames::Subtype:
             return msg.getSubtypeString();
         case FieldNames::Mode:
             return msg.getModeString();
         case FieldNames::ArgCount:
             return msg.getNumberOfArguments();
         case FieldNames::Payload:
             if(loggingOnlyMode)
                 return QString("Logging only Mode! Disable in Project Settings!");
             /* display payload */
             return msg.toStringPayload();
         default:
             if (index.column()>=FieldNames::Arg0)
             {
                 int col=index.column()-FieldNames::Arg0; //arguments a zero based
                 QDltArgument arg;
                 if (msg.getArgument(col,arg))
                 {
                     return arg.toString();
                 }
                 else
                  return QString(" - ");

             }
         }
     }

     if ( role == Qt::ForegroundRole ) {
         int currentIdx = qfile->getMsgFilterPos(index.row());
         qfile->getMsg(currentIdx, msg);

         // Color the last search row
         if(lastSearchIndex != -1 && currentIdx == qfile->getMsgFilterPos(lastSearchIndex))
         {
             return QVariant(QBrush(DltUiUtils::optimalTextColor(searchBackgroundColor())));

         }
         else if (QColor(qfile->checkMarker(msg)).isValid())
         {
           QColor color = qfile->checkMarker(msg);

              //return QVariant(QBrush(color));
              return QVariant(QBrush(DltUiUtils::optimalTextColor(color)));
         }
         else if(project->settings->autoMarkFatalError && !QColor(qfile->checkMarker(msg)).isValid() && ( msg.getSubtypeString() == "error" || msg.getSubtypeString() == "fatal")  ){
            return QVariant(QBrush(QColor(255,255,255)));
         } else {
            return QVariant(QBrush(QColor(0,0,0)));
         }
     }

     if ( role == Qt::BackgroundRole ) {
         int currentIdx = qfile->getMsgFilterPos(index.row());
         qfile->getMsg(currentIdx, msg);

         if((DltSettingsManager::getInstance()->value("startup/pluginsEnabled", true).toBool()))
         {
             pluginManager->decodeMsg(msg,!OptManager::getInstance()->issilentMode());
         }

         QColor color = qfile->checkMarker(msg);
         if(color.isValid())
         {
            return QVariant(QBrush(color));
         }
         else
         {
             if(project->settings->autoMarkFatalError && ( msg.getSubtypeString() == "error" || msg.getSubtypeString() == "fatal") ){
                return QVariant(QBrush(QColor(255,0,0)));
             }
             if(project->settings->autoMarkWarn && msg.getSubtypeString() == "warn"){
                return QVariant(QBrush(QColor(255,255,0)));
             }
             if(project->settings->autoMarkMarker && msg.getType()==QDltMsg::DltTypeControl &&
                msg.getSubtype()==QDltMsg::DltControlResponse && msg.getCtrlServiceId() == DLT_SERVICE_ID_MARKER){
                return QVariant(QBrush(QColor(0,255,0)));
             }

             return QVariant(QBrush(QColor(255,255,255)));
         }
     }

     if ( role == Qt::TextAlignmentRole ) {
        switch(index.column())
        {
            case FieldNames::Index:
                return QVariant(Qt::AlignRight  | Qt::AlignVCenter);
            case FieldNames::Time:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::TimeStamp:
                return QVariant(Qt::AlignRight  | Qt::AlignVCenter);
            case FieldNames::Counter:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::EcuId:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::AppId:
                switch(project->settings->showApIdDesc){
                case 0:
                    return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
                    break;
                case 1:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                default:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                }
            case FieldNames::ContextId:
                switch(project->settings->showCtIdDesc){
                case 0:
                    return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
                    break;
                case 1:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                default:
                    return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
                    break;
                }
            case FieldNames::Type:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::Subtype:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::Mode:
                return QVariant(Qt::AlignCenter | Qt::AlignVCenter);
            case FieldNames::ArgCount:
                return QVariant(Qt::AlignRight  | Qt::AlignVCenter);
            case FieldNames::Payload:
                return QVariant(Qt::AlignLeft   | Qt::AlignVCenter);
        }
    }

     return QVariant();
 }

QVariant TableModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{    
    if (orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
            if (project == NULL)
              return QVariant();

            return FieldNames::getName((FieldNames::Fields)section, project->settings);
        case Qt::TextAlignmentRole:
            return (section == FieldNames::Payload) ? Qt::AlignLeft : QVariant();
         default:
            break;
        }
    }

    return QVariant();
}

 int TableModel::rowCount(const QModelIndex & /*parent*/) const
 {
     if(emptyForceFlag)
         return 0;
     else if(loggingOnlyMode)
         return 1;
     else if (qfile == NULL)
         return 0;
     else
         return qfile->sizeFilter();
 }

 void TableModel::modelChanged()
 {
     emit layoutAboutToBeChanged();

     if(emptyForceFlag && qfile != NULL)
     {
         index(0, 1);
         index(qfile->sizeFilter()-1, 0);
         index(qfile->sizeFilter()-1, columnCount() - 1);
     }else
     {
         index(0, 1);
         index(0, 0);
         index(0, columnCount() - 1);
     }

     emit layoutChanged();
 }




QColor TableModel::searchBackgroundColor() const
{
    QString color = DltSettingsManager::getInstance()->value("other/searchResultColor", QString("#00AAFF")).toString();
    QColor hlColor(color);
    return hlColor;
}

void HtmlDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem customOption = option;
    initStyleOption(&customOption, index);

    QStyle *style = customOption.widget? customOption.widget->style() : QApplication::style();

    QTextDocument doc;
    doc.setHtml(customOption.text);

    /// Painting item without text
    customOption.text = QString();
    style->drawControl(QStyle::CE_ItemViewItem, &customOption, painter);

    QAbstractTextDocumentLayout::PaintContext ctx;

    // Highlighting text if item is selected
    if (customOption.state & QStyle::State_Selected)
        ctx.palette.setColor(QPalette::Text, customOption.palette.color(QPalette::Active, QPalette::HighlightedText));

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &customOption);
    painter->save();
    painter->translate(textRect.topLeft());
    painter->setClipRect(textRect.translated(-textRect.topLeft()));
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();

}

QSize HtmlDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem customOption = option;
    initStyleOption(&customOption, index);

    QTextDocument doc;
    doc.setHtml(customOption.text);
    doc.setTextWidth(customOption.rect.width());
    return QSize(doc.idealWidth(), doc.size().height());
}
