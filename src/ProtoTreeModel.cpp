#include "ProtoTreeModel.h"
#include <QtGui>

static auto TREE_NODE_MIME_TYPE = "application/x-treenode";

ProtoTreeModel::ProtoTreeModel(const ::google::protobuf::Message& data,
                               QObject* parent /*= nullptr*/)
  : QAbstractItemModel(parent) {
  QStringList headers;
  headers << "Name" << "Type" << "Value";

  QVector<QVariant> rootData;
  foreach(QString header, headers) {
    rootData << header;
  }

  rootItem = new ProtoTreeItem(rootData);
  setupModelData(data, rootItem);
}

ProtoTreeModel::~ProtoTreeModel() {
  delete rootItem;
}

int ProtoTreeModel::columnCount(const QModelIndex& /* parent */) const {
  return rootItem->columnCount();
}

QVariant ProtoTreeModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  ProtoTreeItem* item = getItem(index);
  return item->data(index.column(), role);
}

Qt::ItemFlags ProtoTreeModel::flags(const QModelIndex& index) const {
  if (!index.isValid()) {
    return Qt::NoItemFlags;
  }

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  ProtoTreeItem* item = getItem(index);
  flags |= item->getFlag();

  if (index.column() != kColumnTypeValue) {
    flags = flags & ~Qt::ItemIsEditable;
  }

  return flags;
}

ProtoTreeItem* ProtoTreeModel::getItem(const QModelIndex& index) const {
  if (index.isValid()) {
    auto* item = static_cast<ProtoTreeItem*>(index.internalPointer());
    if (item) {
      return item;
    }
  }
  return rootItem;
}

std::string ProtoTreeModel::getMsgTypeName(const google::protobuf::FieldDescriptor* pFd) {
  if (pFd->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
    const google::protobuf::Descriptor* desc = pFd->message_type();
    return desc->name();
  }

  return pFd->type_name();
}

QVariant ProtoTreeModel::getMsgValue(const google::protobuf::Message& data, const google::protobuf::FieldDescriptor* fieldDesc) {
  auto type = fieldDesc->cpp_type();
  switch (type) {
  case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
    return QVariant(data.GetReflection()->GetInt32(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
    return QVariant(data.GetReflection()->GetInt64(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
    return QVariant(data.GetReflection()->GetUInt32(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
    return QVariant(data.GetReflection()->GetUInt64(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
    return QVariant(data.GetReflection()->GetDouble(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
    return QVariant(data.GetReflection()->GetFloat(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
    return QVariant(data.GetReflection()->GetBool(data, fieldDesc));
  case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
    return QVariant(data.GetReflection()->GetEnum(data, fieldDesc)->name().c_str());
  case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
    return QVariant(data.GetReflection()->GetString(data, fieldDesc).c_str());
  default:
    return QVariant();
  }
}

QVariant ProtoTreeModel::getMsgValue(const google::protobuf::Message& data, const google::protobuf::FieldDescriptor* fieldDesc, int index) {
  auto type = fieldDesc->cpp_type();
  switch (type) {
  case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
    return QVariant(data.GetReflection()->GetRepeatedInt32(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
    return QVariant(data.GetReflection()->GetRepeatedInt64(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
    return QVariant(data.GetReflection()->GetRepeatedUInt32(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
    return QVariant(data.GetReflection()->GetRepeatedUInt64(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
    return QVariant(data.GetReflection()->GetRepeatedDouble(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
    return QVariant(data.GetReflection()->GetRepeatedFloat(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
    return QVariant(data.GetReflection()->GetRepeatedBool(data, fieldDesc, index));
  case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
    return QVariant(data.GetReflection()->GetRepeatedEnum(data, fieldDesc, index)->name().c_str());
  case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
    return QVariant(data.GetReflection()->GetRepeatedString(data, fieldDesc, index).c_str());
  default:
    return QVariant();
  }
}

QVariant ProtoTreeModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return rootItem->data(section, role);

  return QVariant();
}

QModelIndex ProtoTreeModel::index(int row, int column, const QModelIndex& parent) const {
  ProtoTreeItem* parentItem = getItem(parent);

  ProtoTreeItem* childItem = parentItem->child(row);
  if (childItem) {
    return createIndex(row, column, childItem);
  }
  return QModelIndex();
}

bool ProtoTreeModel::insertRows(int position, int rows, const QModelIndex& parent) {
  ProtoTreeItem* parentItem = getItem(parent);

  beginInsertRows(parent, position, position + rows - 1);
  bool success = parentItem->insertChildren(position, rows, rootItem->columnCount());
  endInsertRows();
  return success;
}

QModelIndex ProtoTreeModel::parent(const QModelIndex& index) const {
  if (!index.isValid())
    return QModelIndex();

  ProtoTreeItem* childItem = getItem(index);
  ProtoTreeItem* parentItem = childItem->parent();

  if (parentItem == rootItem)
    return QModelIndex();

  return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool ProtoTreeModel::removeRows(int position, int rows, const QModelIndex& parent) {
  ProtoTreeItem* parentItem = getItem(parent);
  beginRemoveRows(parent, position, position + rows - 1);
  bool success = parentItem->removeChildren(position, rows);
  endRemoveRows();

  return success;
}

bool ProtoTreeModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                              const QModelIndex& destinationParent, int destinationChild) {
  ProtoTreeItem* sourceParentItem = getItem(sourceParent);
  ProtoTreeItem* destinationParentItem = getItem(destinationParent);
  if (sourceParentItem == nullptr
      || destinationParentItem == nullptr) {
    return false;
  }
  bool success = beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);
  if (!success) {
    return false;
  }
  auto listItems = sourceParentItem->takeChildren(sourceRow, count);
  if (sourceParentItem == destinationParentItem
      && destinationChild > sourceRow) {
    success = destinationParentItem->insertChildren(destinationChild - count, listItems);
  } else {
    success = destinationParentItem->insertChildren(destinationChild, listItems);
  }

  for (int i = 0; i < sourceParentItem->childCount(); ++i) {
    sourceParentItem->child(i)->setData(kColumnTypeName
                                        , QString().sprintf("%d", i));
  }

  if (sourceParentItem != destinationParentItem) {
    for (int i = 0; i < destinationParentItem->childCount(); ++i) {
      destinationParentItem->child(i)->setData(kColumnTypeName
                                               , QString().sprintf("%d", i));
    }
  }

  endMoveRows();

  return success;
}

QStringList ProtoTreeModel::mimeTypes() const {
  return QAbstractItemModel::mimeTypes() << TREE_NODE_MIME_TYPE;
}

QMimeData* ProtoTreeModel::mimeData(const QModelIndexList& indexes) const {
  auto* mimeData = new QMimeData;
  QByteArray data; //a kind of RAW format for datas
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << QCoreApplication::applicationPid();
  for (const QModelIndex& index : indexes) {
    stream << index.row();
    break;
  }
  mimeData->setData(TREE_NODE_MIME_TYPE, data);
  return mimeData;
}

bool ProtoTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const {
  if (!data->hasFormat(TREE_NODE_MIME_TYPE))
    return false;

  if (action != Qt::MoveAction
      || row == -1 || column == -1) {
    return false;
  }

  return true;
}

bool ProtoTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) {
  QByteArray byteData = data->data(TREE_NODE_MIME_TYPE);
  QDataStream stream(&byteData, QIODevice::ReadOnly);
  int64_t senderPid;
  stream >> senderPid;
  if (senderPid != QCoreApplication::applicationPid())
    return false;

  int32_t src_row;
  stream >> src_row;

  if (src_row == row)
    return false;

  bool success = moveRows(parent, src_row, 1, parent, row);
  return success;
}

Qt::DropActions ProtoTreeModel::supportedDropActions() const {
  return Qt::MoveAction;
}

Qt::DropActions ProtoTreeModel::supportedDragActions() const {
  return Qt::MoveAction;
}

int ProtoTreeModel::rowCount(const QModelIndex& parent) const {
  ProtoTreeItem* parentItem = getItem(parent);

  return parentItem->childCount();
}

bool ProtoTreeModel::setData(const QModelIndex& index, const QVariant& value,
                             int role) {
  if (role != Qt::EditRole)
    return false;

  ProtoTreeItem* item = getItem(index);
  bool result = item->setData(index.column(), value, role);

  if (result) {
    emit dataChanged(index, index);
  }

  return result;
}

bool ProtoTreeModel::setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant& value, int role) {
  if (role != Qt::EditRole || orientation != Qt::Horizontal)
    return false;

  bool result = rootItem->setData(section, value, role);

  if (result)
    emit headerDataChanged(orientation, section, section);

  return result;
}

void ProtoTreeModel::setupModelData(const ::google::protobuf::Message& data, ProtoTreeItem* parent) {
  int cnt = data.GetDescriptor()->field_count();
  for (int i = 0; i < cnt; ++i) {
    const google::protobuf::FieldDescriptor* fieldDesc = data.GetDescriptor()->field(i);
    if (fieldDesc == nullptr) {
      continue;
    }

    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
    ProtoTreeItem* child = parent->child(parent->childCount() - 1);

    QVariant keyIndex, keyData, keyType;

    keyIndex = QVariant(tr(fieldDesc->name().c_str()));
    QString typeName = getMsgTypeName(fieldDesc).c_str();
    keyType = QVariant(typeName);

    if (fieldDesc->is_repeated()) {
      keyType = QVariant("repeated");
      child->setFlag(Qt::ItemIsDropEnabled);
    } else if (google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE == fieldDesc->cpp_type()) {
#pragma push_macro("GetMessage")
#undef GetMessage
      const auto& subData = data.GetReflection()->GetMessage(data, fieldDesc);
#pragma pop_macro("GetMessage")
      setupModelData(subData, child);
    } else {
      keyData = getMsgValue(data, fieldDesc);
      child->setFlag(Qt::ItemIsEditable);
    }

    child->setData(kColumnTypeName, keyIndex);

    child->setData(kColumnTypeType, keyType);
    child->setData(kColumnTypeType, QColor("#569ad6"), Qt::ForegroundRole);

    child->setData(kColumnTypeValue, keyData);
    child->setData(kColumnTypeValue, fieldDesc->cpp_type(), Qt::UserRole);
    if (fieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_ENUM) {
      QStringList enumData;
      const auto* enumDesc = fieldDesc->enum_type();
      if (enumDesc) {
        for (int idx = 0; idx < enumDesc->value_count(); ++idx) {
          const auto* enumValue = enumDesc->value(idx);
          if (enumValue) {
            enumData.append(enumValue->name().c_str());
          }
        }
      }
      child->setData(kColumnTypeValue, enumData, Qt::UserRole + 1);
    }

    if (fieldDesc->is_repeated()) {
      auto fieldSize = data.GetReflection()->FieldSize(data, fieldDesc);
      for (int index = 0; index < fieldSize; ++index) {
        keyIndex = QVariant(index);
        keyType = typeName;

        child->insertChildren(child->childCount(), 1, rootItem->columnCount());
        ProtoTreeItem* repeatedChild = child->child(child->childCount() - 1);
        repeatedChild->setData(kColumnTypeName, keyIndex);

        repeatedChild->setData(kColumnTypeType, keyType);
        repeatedChild->setData(kColumnTypeType, QColor("#569ad6"), Qt::ForegroundRole);

        repeatedChild->setData(kColumnTypeValue, fieldDesc->cpp_type(), Qt::UserRole);
        repeatedChild->setFlag(Qt::ItemIsDragEnabled);

        if (fieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
          const auto& repMessage = data.GetReflection()->GetRepeatedMessage(data, fieldDesc, index);
          setupModelData(repMessage, repeatedChild);
        } else {
          keyData = getMsgValue(data, fieldDesc, index);
          repeatedChild->setData(kColumnTypeValue, keyData);
        }
      }
    }
  }
}

void ProtoTreeModel::loadModelData(google::protobuf::Message& data, ProtoTreeItem* parent) {
  for (int idx = 0; idx < parent->childCount(); ++idx) {
    ProtoTreeItem* child = parent->child(idx);
    if (nullptr == child) {
      continue;
    }

    const auto* fieldDesc = data.GetDescriptor()->field(idx);
    if (nullptr == fieldDesc) {
      continue;
    }

    if (fieldDesc->is_repeated()) {
      for (int subIdx = 0; subIdx < child->childCount(); ++subIdx) {
        auto subChild = child->child(subIdx);
        if (nullptr == subChild) {
          continue;
        }

        QVariant value = subChild->data(kColumnTypeValue, Qt::DisplayRole);
        auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(subChild->data(kColumnTypeValue, Qt::UserRole).toInt());
        switch (type) {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
          data.GetReflection()->AddInt32(&data, fieldDesc, value.toInt());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
          data.GetReflection()->AddInt64(&data, fieldDesc, value.toLongLong());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
          data.GetReflection()->AddUInt32(&data, fieldDesc, value.toUInt());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
          data.GetReflection()->AddUInt64(&data, fieldDesc, value.toULongLong());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
          data.GetReflection()->AddDouble(&data, fieldDesc, value.toDouble());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
          data.GetReflection()->AddFloat(&data, fieldDesc, value.toFloat());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
          data.GetReflection()->AddBool(&data, fieldDesc, value.toBool());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
          if (fieldDesc->enum_type()) {
            auto enumValueDesc
              = fieldDesc->enum_type()->FindValueByName(value.toString().toStdString());
            if (enumValueDesc) {
              data.GetReflection()->AddEnum(&data, fieldDesc, enumValueDesc);
            }
          }
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
          data.GetReflection()->AddString(&data, fieldDesc, value.toString().toStdString());
          break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
          {
            auto* subData = data.GetReflection()->AddMessage(&data, fieldDesc);
            loadModelData(*subData, subChild);
          }
          break;
        default:
          break;
        }
      }
    } else {
      QVariant value = child->data(kColumnTypeValue, Qt::DisplayRole);
      auto type = static_cast<google::protobuf::FieldDescriptor::CppType>(child->data(kColumnTypeValue, Qt::UserRole).toInt());
      switch (type) {
      case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        data.GetReflection()->SetInt32(&data, fieldDesc, value.toInt());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        data.GetReflection()->SetInt64(&data, fieldDesc, value.toLongLong());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        data.GetReflection()->SetUInt32(&data, fieldDesc, value.toUInt());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        data.GetReflection()->SetUInt64(&data, fieldDesc, value.toULongLong());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        data.GetReflection()->SetDouble(&data, fieldDesc, value.toDouble());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        data.GetReflection()->SetFloat(&data, fieldDesc, value.toFloat());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        data.GetReflection()->SetBool(&data, fieldDesc, value.toBool());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        if (fieldDesc->enum_type()) {
          auto enumValueDesc
            = fieldDesc->enum_type()->FindValueByName(value.toString().toStdString());
          if (enumValueDesc) {
            data.GetReflection()->SetEnum(&data, fieldDesc, enumValueDesc);
          }
        }
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        data.GetReflection()->SetString(&data, fieldDesc, value.toString().toStdString());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
          auto* subData = data.GetReflection()->MutableMessage(&data, fieldDesc);
          loadModelData(*subData, child);
        }
      default:
        break;;
      }
    }
  }
}

void ProtoTreeModel::duplicateTreeItem(ProtoTreeItem* source, ProtoTreeItem* destination, const QModelIndex& parent) {
  for (int i = 0; i < source->childCount(); ++i) {
    insertRow(i, parent);
    auto* sourceChild = source->child(i);
    auto* destinationChild = destination->child(i);
    auto modelIdx = index(i, 0, parent);
    duplicateTreeItem(sourceChild, destinationChild, modelIdx);
  }

  destination->copyFrom(*source);
}

bool ProtoTreeModel::insertMessageData(const google::protobuf::Descriptor* desc, const QModelIndex& parent) {
  if (!desc) {
    return false;
  }

  ProtoTreeItem* parentItem = getItem(parent);

  int fieldCnt = desc->field_count();
  for (int fieldIdx = 0; fieldIdx < fieldCnt; ++fieldIdx) {
    const google::protobuf::FieldDescriptor* targetFieldDesc = desc->field(fieldIdx);
    if (targetFieldDesc == nullptr) {
      continue;
    }

    insertRow(parentItem->childCount(), parent);
    ProtoTreeItem* fieldItem = parentItem->child(parentItem->childCount() - 1);

    QVariant keyIndex, keyData, keyType;
    keyIndex = QVariant(tr(targetFieldDesc->name().c_str()));
    QString typeName = getMsgTypeName(targetFieldDesc).c_str();
    keyType = QVariant(typeName);

    if (targetFieldDesc->is_repeated()) {
      keyType = QVariant("repeated");
      fieldItem->setFlag(Qt::ItemIsDropEnabled);
    } else if (google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE == targetFieldDesc->cpp_type()) {
      auto newModelIndex = index(fieldIdx, 0, parent);
      insertMessageData(targetFieldDesc->message_type(), newModelIndex);
    } else {
      fieldItem->setFlag(Qt::ItemIsEditable);
      switch (targetFieldDesc->cpp_type()) {
      case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        fieldItem->setData(kColumnTypeValue, targetFieldDesc->default_value_int32());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        fieldItem->setData(kColumnTypeValue, targetFieldDesc->default_value_int64());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        fieldItem->setData(kColumnTypeValue, targetFieldDesc->default_value_uint32());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        fieldItem->setData(kColumnTypeValue, targetFieldDesc->default_value_uint64());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        fieldItem->setData(kColumnTypeValue, targetFieldDesc->default_value_double());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        fieldItem->setData(kColumnTypeValue, targetFieldDesc->default_value_float());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        fieldItem->setData(kColumnTypeValue, targetFieldDesc->default_value_bool());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
          QStringList enumData;
          const auto* enumDesc = targetFieldDesc->enum_type();
          if (enumDesc) {
            for (int enumIdx = 0; enumIdx < enumDesc->value_count(); ++enumIdx) {
              const auto* enumValue = enumDesc->value(enumIdx);
              if (enumValue) {
                enumData.append(enumValue->name().c_str());
              }
            }
          }
          fieldItem->setData(kColumnTypeValue, enumData, Qt::UserRole + 1);
        }
        break;
      default:
        break;
      }
    }

    fieldItem->setData(kColumnTypeName, keyIndex);

    fieldItem->setData(kColumnTypeType, keyType);
    fieldItem->setData(kColumnTypeType, QColor("#569ad6"), Qt::ForegroundRole);

    fieldItem->setData(kColumnTypeValue, targetFieldDesc->cpp_type(), Qt::UserRole);
  }
  return true;
}

ProtoTreeItem* ProtoTreeModel::getRootItem() {
  return rootItem;
}

void ProtoTreeModel::getMessage(google::protobuf::Message& data) {
  loadModelData(data, rootItem);
}

const ProtoTreeItem* ProtoTreeModel::item(const QModelIndex& index) const {
  return getItem(index);
}

bool ProtoTreeModel::insertModelData(const google::protobuf::Descriptor* desc, const QModelIndex& parent, int32_t row) {
  ProtoTreeItem* parentItem = getItem(parent);

  std::list<int32_t> messageIdxs;
  parentItem->getIndexOfMessage(messageIdxs);

  auto it = messageIdxs.begin();
  for (size_t i = 0; it != messageIdxs.end(); ++it, ++i) {
    const auto* fieldDesc = desc->field(*it);
    if (nullptr == fieldDesc) {
      return false;
    }

    if (i != messageIdxs.size() - 1) {
      if (fieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        desc = fieldDesc->message_type();
        continue;
      }

      // repeated 的子会多一层
      if (!fieldDesc->is_repeated()) {
        continue;
      }
    }

    if (!fieldDesc->is_repeated()) {
      return false;
    }

    // reach target
    insertRow(row, parent);
    auto subIndex = this->index(row, 0, parent);
    ProtoTreeItem* repeatedChild = parentItem->child(row);
    repeatedChild->setData(kColumnTypeName, QVariant(row));
    // if not insert to last, need rename
    if (row != parentItem->childCount() - 1) {
      for (int childIdx = 0; childIdx < parentItem->childCount(); ++childIdx) {
        parentItem->child(childIdx)->setData(kColumnTypeName
                                             , QString().sprintf("%d", childIdx));
      }
    }

    repeatedChild->setData(kColumnTypeType, getMsgTypeName(fieldDesc).c_str());
    repeatedChild->setData(kColumnTypeType, QColor("#569ad6"), Qt::ForegroundRole);

    repeatedChild->setData(kColumnTypeValue, fieldDesc->cpp_type(), Qt::UserRole);
    repeatedChild->setFlag(Qt::ItemIsDragEnabled);

    if (fieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
      if (!insertMessageData(fieldDesc->message_type(), subIndex)) {
        return false;
      }
    } else {
      repeatedChild->setFlag(Qt::ItemIsEditable);
      switch (fieldDesc->cpp_type()) {
      case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        repeatedChild->setData(kColumnTypeValue, fieldDesc->default_value_int32());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        repeatedChild->setData(kColumnTypeValue, fieldDesc->default_value_int64());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        repeatedChild->setData(kColumnTypeValue, fieldDesc->default_value_uint32());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        repeatedChild->setData(kColumnTypeValue, fieldDesc->default_value_uint64());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        repeatedChild->setData(kColumnTypeValue, fieldDesc->default_value_double());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        repeatedChild->setData(kColumnTypeValue, fieldDesc->default_value_float());
        break;
      case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        repeatedChild->setData(kColumnTypeValue, fieldDesc->default_value_bool());
        break;
      default:
        break;
      }
    }
  }
  return true;
}

bool ProtoTreeModel::duplicateModelData(const QModelIndex& parent, int32_t row) {
  ProtoTreeItem* parentItem = getItem(parent);
  bool needResetIdxName = true;
  if (row >= parentItem->childCount()) {
    return false;
  }

  if (row == parentItem->childCount() - 1) {
    needResetIdxName = false;
  }

  ProtoTreeItem* sourceItem = parentItem->child(row);
  insertRow(row + 1, parent);
  auto modelIndex = index(row + 1, 0, parent);
  ProtoTreeItem* duplicateItem = parentItem->child(row + 1);

  duplicateTreeItem(sourceItem, duplicateItem, modelIndex);

  if (needResetIdxName) {
    for (int childIdx = 0; childIdx < parentItem->childCount(); ++childIdx) {
      parentItem->child(childIdx)->setData(kColumnTypeName
                                           , QString().sprintf("%d", childIdx));
    }
  }

  return true;
}

bool ProtoTreeModel::removeModelData(const QModelIndex& parent, int32_t row) {
  ProtoTreeItem* parentItem = getItem(parent);
  bool needResetName = true;
  if (row == parentItem->childCount() - 1) {
    needResetName = false;
  }

  bool ret = removeRow(row, parent);
  if (!ret) {
    return false;
  }

  if (needResetName) {
    for (int childIdx = 0; childIdx < parentItem->childCount(); ++childIdx) {
      parentItem->child(childIdx)->setData(ProtoTreeModel::kColumnTypeName
                                           , QString().sprintf("%d", childIdx));
    }
  }
  return true;
}
